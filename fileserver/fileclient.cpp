/******************************************************
 *   FileName: fileclient.cpp
 *     Author: botter  2012-9-17 
 *Description:
 *******************************************************/
#include "fileclient.h"
#include <errno.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "header.h"

static bool WriteFile( const char *szName, const char *szBuffer, const int nLen );

static char *ReadFile( const char *szFile , int &nLen );

static int inline set_non_block(int sock_fd)
{
	/* O_NONBLOCK， O_RDONLY, O_WRONLY等是同一个层次上的东东*/
	int flags = fcntl(sock_fd, F_GETFL, 0);
	fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

	return 0;
}

fileclient::fileclient(const char *ip, unsigned short port, const char *user_name,
		const char *passwd, const char *rootpath)
{
	_serveraddr.sin_addr.s_addr= inet_addr(ip);
	_serveraddr.sin_port       = htons(port);
    _serveraddr.sin_family = AF_INET;
	memset(_user_name, 0, sizeof(_user_name));
	memset(_password, 0, sizeof(_password));
    strncpy(_user_name, user_name, sizeof(_user_name) - 1);
    strncpy(_password, passwd, sizeof(_password) - 1);

    if(rootpath != NULL)
    {
    	_local_root = rootpath;
    }

    _connected = false;
    _sock_fd = -1;
    _seq = 1;
}

fileclient::~fileclient()
{
	disconnection();
}

bool fileclient::connection()
{
    if(_sock_fd < 0)
    {
    	if((_sock_fd = connect_server()) < 0)
    		return false;
    }

    bigloginreqall login(_user_name, _password, _seq++);
    int msg_len = sizeof(bigloginreqall);

    if(msg_len != send_tmout(_sock_fd, (const char*)&login, msg_len, 5000))
    {
        return false;
    }

    bigloginrspall resp;
    int recv_len = sizeof(resp);

    if(recv_len != recv_tmout(_sock_fd, (char *)&resp, recv_len, 5000))
    {
        return false;
    }

    if(resp.resp.result == BIG_ERR_SUCCESS)
    {
//        printf("login %s %d file server success \n", inet_ntoa(_serveraddr.sin_addr), ntohs(_serveraddr.sin_port));
    	_connected = true;
        return true;
    }
    else
    {
    	::close(_sock_fd);
    	_sock_fd = -1;
    }

    return false;
}

void fileclient::disconnection()
{
    if(_sock_fd > 0)
    {
    	::close(_sock_fd);
    	_sock_fd = -1;
    	_connected = false;
    }
}

//将本地文件保存到远程，
int fileclient::putfile(const char *local_file_name, const char *remote_file_name)
{
	if(local_file_name == NULL || remote_file_name == NULL)
		return FILE_PARAM_ERROR;

    int len = 0;
    string name = _local_root + local_file_name;
    char *buffer = ReadFile(name.c_str() , len );
    if(buffer == NULL || len <= 0)
    {
    	return FILE_FAIL;
    }

    int ret = writefile(buffer, len, remote_file_name);
    free(buffer);
    return ret;
}

//将远程文件Get到本地
int fileclient::getfile(const char *local_file, const char *remote_file)
{
    FileData file_data;
    int ret = readfile(remote_file, &file_data);
    if(ret != FILE_SUCCESS)
    	return ret;

    string name = _local_root + local_file;
    bool r = WriteFile(name.c_str(), file_data.data, file_data.len);

    //这个地方也可以不明确指明释放，FileData的析构函数会自己释放
    free(file_data.data);
    file_data.data = NULL;

    return r == true ? FILE_SUCCESS  : FILE_FAIL;
}

//将数据写入到远程文件
int fileclient::writefile(const char *data, int data_len,
		const char *remote_file)
{
	if (data == NULL || data_len <= 0 || remote_file == NULL)
		return FILE_PARAM_ERROR;

	if (!_connected)
	{
		if (connection() < 0)
			return SOCK_CONNECT_ERROR;
	}

	bigwritereqall writereq(remote_file, data_len);

//	printf("write remote file %s \n", remote_file);

	int msg_len =  data_len + sizeof(bigwritereqall);
	char *send_data = (char *) malloc(msg_len);
	if (data == NULL)
	{
		free(send_data);
		printf("malloc fail \n");
		return FILE_FAIL;
	}

	memcpy(send_data, (char *) &writereq, sizeof(bigwritereqall));
	memcpy(send_data + sizeof(bigwritereqall), data, data_len);

	(void )send_tmout(_sock_fd, send_data, msg_len, 5000);

	free(send_data);
	bigwriterspall resp;
	msg_len = sizeof(bigwriterspall);

	if (msg_len != recv_tmout(_sock_fd, (char*) &resp, msg_len, 5000))
	{
        printf("recv_packet error \n");
		return SOCK_ERROR;
	}

	printf("savefile %s resp = %d \n", remote_file, resp.resp.result);

	return resp.resp.result == 0 ? FILE_SUCCESS : FILE_FAIL;
}

//读取远程文件的数据
int fileclient::readfile(const char *remote_file, FileData *file_data)
{
	if (!_connected)
	{
		if (connection() < 0)
			return SOCK_CONNECT_ERROR;
	}

	file_data->len = 0;
	file_data->data = NULL;

	bigreadreqall read_req(remote_file, _seq++);
	int msg_len = sizeof(bigreadreqall);

	if (msg_len != send_tmout(_sock_fd, (const char*) &read_req, msg_len, 5000))

	{
        return SOCK_ERROR;
	}

	bigreadrspall resp;
	int recv_len = sizeof(resp);

	if (recv_len != recv_tmout(_sock_fd, (char *) &resp, recv_len, 5000))
	{
        return SOCK_ERROR;
	}

	if (resp.rsp.result != 0)
	{
		return FILE_FAIL;
	}

	unsigned int data_len = ntohl(resp.rsp.data_len);
	if (data_len > 10 * 1024 * 1024)
	{
		printf("getfile data len  is too big : %d \n", data_len);
		return FILE_FAIL;
	}

	file_data->data = (char*) malloc(data_len);
	if (file_data->data == NULL)
	{
		printf("fileclient::readfile malloc fail \n");
		return FILE_FAIL;
	}

	if (data_len != (unsigned int)recv_tmout(_sock_fd, (char *) file_data->data, data_len, 5000))
	{
		free(file_data->data);
		return SOCK_ERROR;
	}

	file_data->len = data_len;
	return FILE_SUCCESS;
}

//精确的表示接收多大的长度,如果读不到这个长度是会超时退出来的。
int fileclient::recv_tmout(int sockfd, char *buffer, int buffer_len, int timeout)
{
	fd_set read_fds;
    int offset = 0;

    if(sockfd < 0 || buffer == NULL || buffer_len < 0)
    	return -1;

	struct timeval tval;
	tval.tv_sec = timeout / 1000;
	tval.tv_usec = (timeout % 1000)*1000;
    int tryconn = 3;

	while (offset < buffer_len)
	{
		int len = read(sockfd, buffer, buffer_len - offset);

		if(len > 0)
		{
			offset += len;
            continue;
		}
		else if(len == 0)
		{
            printf("read = 0, remote close, disconnection \n");
        	disconnection();
//            if(tryconn-- > 0)
//            {
//            	if(connection())
//            	{
//            		printf("reconnection success \n");
//            	    continue;
//            	}
//            }
            return SOCK_REMOTE_CLOSE;
		}
		else
		{
            if(errno == EAGAIN)
            {
				FD_ZERO(&read_fds);
				FD_SET(sockfd, &read_fds);
				int res = select(sockfd + 1, &read_fds, NULL, NULL, &tval);
				if (res == 1)
				{
					if (FD_ISSET(sockfd, &read_fds))
						continue;
				}
				else if (res <= 0)
				{
                    printf("recv_tmout fileclient select timeout \n");
                    return SOCK_ERROR;
				}
            }
            else
            {
            	printf("read error disconnection : %s \n", strerror(errno));
            	disconnection();
                if(tryconn-- > 0)
                {
                	if(connection())
                	{
                		printf("reconnection success \n");
                	    continue;
                	}
                }
                return SOCK_ERROR;
            }
		}
	}

	return offset;
}

int fileclient::send_tmout(int sockfd, const char *buffer, int buffer_len, int timeout)
{
    int len = 0;

	fd_set write_fds;
    int offset = 0;
	struct timeval tval;

    if(sockfd < 0 || buffer == NULL || buffer_len < 0)
    	return -1;

	tval.tv_sec = timeout / 1000;
	tval.tv_usec = (timeout % 1000)*1000;

	int tryconn = 3;

    while(offset < buffer_len)
    {
		len = write(sockfd, buffer + offset, buffer_len - offset);
        if(len > 0)
        {
        	offset += len;
        	continue;
        }
        else
        {
            if(errno == EAGAIN)
            {
				FD_ZERO(&write_fds);
				FD_SET(sockfd, &write_fds);
				int res = select(sockfd + 1,  NULL, &write_fds, NULL, &tval);
                if(res > 0)
                {
                	continue;
                }
                else if(res < 0)
                {
                	printf("send_tmout select error %d %s \n", errno, strerror(errno));
                }
                else
                {
                	printf("send_tmout select timeout %u : %u \n", (unsigned int)tval.tv_sec, (unsigned int)tval.tv_usec);
                	return SOCK_ERROR;
                }
        	}
            else //在其他情况下说明遇到写错误EPIPE。对端主动发起关闭，和socket收到reset消息。
            {
                printf("write error disconnection : %s \n", strerror(errno));
            	disconnection();
                if(tryconn-- > 0)
                {
                	if(connection())
                	{
                		printf("reconnection success \n");
                	    continue;
                	}
                }
            }
            return SOCK_ERROR;
        }
    }

    return offset;
}

int fileclient::connect_server()
{
    int ret = -1;

	int epoll_handle = -1;
	struct epoll_event sock_event;
    struct epoll_event events[8];
    int error = 0;
    int len = 0;
    int sockfd = -1;
    int epoll_ret = -1;

	if ((epoll_handle = epoll_create(8)) == -1)
	{
        return false;
	}

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket: %s\n", strerror(errno));
		goto EXPOUT;
	}

	len = sizeof(_serveraddr);

	sock_event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLPRI;
	sock_event.data.fd = sockfd;

	(void)set_non_block(sockfd);

	epoll_ctl(epoll_handle, EPOLL_CTL_ADD, sockfd, &sock_event);
	if ((::connect(sockfd, (struct sockaddr*) &_serveraddr, (socklen_t)len)) < 0)
	{
		if (errno != EINPROGRESS)
		{
            printf("connect %s : %d fail, reason: %s\n", inet_ntoa(_serveraddr.sin_addr),
            		ntohs(_serveraddr.sin_port), strerror(errno));
            goto EXPOUT;
		}
	}
	else
	{
        ret = sockfd;
		goto EXPOUT;
	}

	/* 只检测这一个socket */
    epoll_ret = epoll_wait(epoll_handle, events, 8, 1000);
	if(epoll_ret == 0) /* 没有任何事件发生，返回超时错误 */
	{
        goto EXPOUT;
	}
	else if(epoll_ret < 0)
	{
		goto EXPOUT;
	}
	else /* 有事件发生，但是为了更安全起见，检测下是否是错误事件 */
	{
		error = 0;
		len = sizeof(int);
		if(0 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t*)&len))
		{
            if(error != 0)
            {
            	printf("connect  %s %d error: %s \n", inet_ntoa(_serveraddr.sin_addr),
                		ntohs(_serveraddr.sin_port),  strerror(error));
                errno = error;
                ret = SOCK_CONNECT_ERROR;
            }
            else
            {
			    ret = sockfd;
            }
		}
	}

EXPOUT:
	if (epoll_handle > 0)
	{
		close(epoll_handle);
	}
	if (ret <= 0)
	{
		if (sockfd > 0)
			close(sockfd);
	}
	else
	{
//		printf("connect %s %d success \n", inet_ntoa(_serveraddr.sin_addr),
//        		ntohs(_serveraddr.sin_port));
	}
	return ret;
}

bool WriteFile( const char *szName, const char *szBuffer, const int nLen )
{
#ifdef _WIN32
	FILE *fp = fopen( szName, "wb" ) ;
	if ( fp == NULL )
	{
		return false;
	}
	fwrite( szBuffer, nLen, 1, fp ) ;
	fclose( fp ) ;
#else
	int fp = open( szName, O_CREAT | O_TRUNC | O_WRONLY ) ;
	if ( fp < 0 )
	{
		return false;
	}
	write( fp, szBuffer, nLen ) ;
	close( fp ) ;
	chmod( szName, 0777 ) ;
#endif

	return true;
}

// 读取文件
char *ReadFile( const char *szFile , int &nLen )
{
	char *szBuffer = NULL ;
#ifdef _WIN32

	FILE* fp = fopen( szFile , "rb" ) ;
	if ( fp != NULL )
	{
		int len = 0 ;
		fseek( fp , 0 , SEEK_END ) ;
		len = ftell( fp ) ;

		szBuffer = new char[len+1];

		fseek( fp , 0 , SEEK_SET ) ;
		fread( szBuffer , 1 , len , fp ) ;
		szBuffer[len] = 0 ;

		nLen  = len ;

		fclose( fp );
	}
	else
	{
		nLen  = 0 ;
		return NULL ;
	}

#else

	int fp = open( szFile , O_RDONLY ) ;

	if ( fp >= 0 )
	{
		// 先得到文件的大小
		struct stat buf ;
		fstat( fp , &buf ) ;

		int len = buf.st_size ;

		szBuffer = new char[len+1];

		read( fp , szBuffer , len ) ;
		szBuffer[len] = 0 ;

		nLen  = len ;

		close( fp );
	}
	else
	{
		nLen =  0 ;
		return NULL ;
	}

#endif

	return szBuffer;
}

#define defaultip   "127.0.0.1"
#define defaultport "8809"
#include <errno.h>

void check()
{
	char buffer[32] = {0};
	int num = 1000;
	while(num --)
	{
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "../fileserver/test%d.jpeg", num);
	    FILE *fp = NULL;
	    if((fp = fopen(buffer, "r")) == NULL)
	    {
	    	printf("fopen %s %s \n", buffer, strerror(errno));
	    }
	}

	printf("----------------check finished ---------------------\n");
}

void *thread_fun(void *arg)
{
	int a = 0;
    if(arg != NULL)
    	a = 10000;

	int num = 100;
	char buffer[32] = {0};
	time_t btime = time(0);
	fileclient fc("127.0.0.1", 8809, "admin", "123456");
	while(num --)
	{
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "./test%d.jpeg", num + a);
	    fc.putfile("test.jpeg", buffer);
	}
	printf("use time %d \n", (int)(time(0) - btime));
//    sleep(2);
    return NULL;
}

int main(int argc, char *argv[])
{
	const char *ip = NULL;
	const char *port = NULL;
    if(argc < 3)
    {
        ip = defaultip;
        port = defaultport;
    }
    else
    {
    	ip = argv[1];
    	port = argv[2];
    }

//    vos::make_thread(thread_fun, NULL);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread_fun, 0);

//    pthread_create(&t2, NULL, thread_fun, (void*)10000);

    sleep(5);
    check();

    return 0;
}

