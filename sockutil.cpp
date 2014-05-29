/******************************************************
*  CopyRight: liubo (botter.liu@163.com) Version 0.l
*   FileName: net.c
*     Author: liubo  2012-2-5
*Description:
*******************************************************/
#include "sockutil.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "log.h"

#ifdef __WINDOWS
# include <stdio.h>
# include <winsock2.h>
# include <errno.h>
#else

#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>

#endif

/****
��һ���֪��Щ����
|----------------------------------------------------------|
| ҵ��ģ�飬
|----------------------------------------------------------|
|SOCK_ITEM:RELEASE, EPOLL,
|--------------------------------|-------------------------|------------------
|connect, bind, listen, accept, read, write. close. EPOLL  |��������                                         |
|epoll����֪ SOCK_ITEM���ǻὫ��������С�                                                                                              |                 |
|----------------------------------------------------------|------------------

����ģ�ͣ�
|----|
|��ʱ�� |------>|EPOLL->LOOP|---->APP
|----|

����ģ�͵ļ�����:
*******************/

using namespace std;
using namespace vos;
namespace vos
{
bool set_resource_limit(int limit /* =  MAX_FD_OFFSET */)
{
#ifndef __WINDOWS
	struct rlimit rt;
	rt.rlim_max = rt.rlim_cur = limit;
	if (setrlimit(RLIMIT_NOFILE, &rt) == -1)
	{
		perror("setrlimit\n");
		return false;
	}
#endif

	return true;
}

void close(int fd)
{
#ifdef __WINDOWS
	closesocket(fd);
#else
	::close(fd);
#endif
}

/* ip Ϊ�����ֽ���, �˺��������̰߳�ȫ�ġ� */
char *get_ip_str(unsigned int ip)
{
	static char strip[16] = {0};
	memset(strip, 0, sizeof(strip));
	sprintf(strip,  "%d.%d.%d.%d", ip << 24, (ip << 16) & 0xff,(ip << 8) & 0xff, ip & 0xff);
	return strip;
}

int set_reuse_addr( int sockfd)
{
	int bReuseaddr = 1;
	return setsockopt(sockfd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(int));
}

int init_start_up()
{
#ifdef __WINDOWS
	WSADATA wsaData;
	if ( WSAStartup(MAKEWORD(2,0), &wsaData) || LOBYTE(wsaData.wVersion) != 2 ) {
		return ERROR_SOCK_INIT;
	}
	return 0;
#else
	return 0;
#endif
}

int sock(socktype type)
{
	if(0 != init_start_up())
		return ERROR_SOCK_INIT;
	int sockfd = -1;

	if(type == vos::tcp)
	{
	    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	}
	else
	{
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	}
	return sockfd;
}

/* �ڲ��ӿڣ��ɵ����߱�֤�����������ȷ�ԡ�*/
int bind_sock(int sock_fd, const char* ip, unsigned short port)
{
	int ret = OK;

    struct sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = inet_addr(ip);
    //inet_aton(ip, &sock_addr.sin_addr);
    sock_addr.sin_port = htons(port);

    ret =  bind(sock_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if(ret < 0)
    {
    	ret = ERROR_SOCK_BIND;
    	printf("bind error: %s\n", strerror(errno));
        return ret;
    }

    return ret;
}

int listen_sock(int sock_fd, int backlog)
{
    int ret = listen(sock_fd, backlog);
    if(ret < 0)
    {
    	return ERROR_SOCK_LISTEN;
    }
    else
    {
    	return OK;
    }
}

int set_block(int sock_fd)
{
//	int flags = fcntl(sock_fd, F_GETFL, 0);
//    fcntl(sock_fd, F_SETFL, flags & (~O_NONBLOCK));
    return OK;
}

int set_non_block(int sock_fd)
{
#ifdef __WINDOWS
	unsigned long flag = 1;
	return (ioctlsocket((SOCKET) sock_fd, FIONBIO, &flag) == 0);
#else
	/* O_NONBLOCK�� O_RDONLY, O_WRONLY����ͬһ������ϵĶ���*/
	int flags = fcntl(sock_fd, F_GETFL, 0);
	fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
#endif
	return OK;
}

/* *************************************************
 * SO_KEEPALIVE  : �Ƿ������ʱ����1Ϊ������
 * TCP_KEEPIDLE  : ��һ�����ӽ�����Ч��̽��֮ǰ���е����ǻ�Ծʱ�������೤ʱ��û�л�󣬷�����·̽Ѱ��Ϣ)
 * TCP_KEEPINTVL : ������·̽Ѱ��Ϣ��ʱ����
 * TCP_KEEPCNT   : ������·̽Ѱʱ��Ĵ�����
 * �� 5sΪ��λ��������·̽Ѱ����Ϣ�ļ��5s�� timeout��ȡֵ��Χ���� > 10��
 * ֻ����һ����ʱʱ�䣬��η���time_outʱ���Ǹ����⣺
 * opt_keepidle:��СֵΪ5���Ժ�Ҳ��ÿ��5����һ�Ρ�
 * ************************************************/
int set_sock_alive(int sock_fd, int time_out)
{
	//windows��ʵ�ֱ��ʱ����
#ifndef __WINDOWS
	int opt_val = 1;
	int opt_keepidle = 0;
	int opt_keepintvl = 0;
	int opt_keepcnt = 0;

	int opt_len = sizeof(opt_val);
	if (time_out < 10)
		time_out = 10;
	else
		time_out = (time_out / 5) * 5;

	opt_keepidle = 5;
	opt_keepintvl = 5;
	opt_keepcnt = (time_out - 5) / 5;

	if (setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&opt_val, opt_len) < 0)
	{
		goto EXTOUT;
	}
	if (setsockopt(sock_fd, SOL_TCP, TCP_KEEPCNT, (const char*)&opt_keepcnt, opt_len) < 0)
	{
		goto EXTOUT;
	}
	if (setsockopt(sock_fd, SOL_TCP, TCP_KEEPIDLE, (const char*)&opt_keepidle, opt_len) < 0)
	{
		goto EXTOUT;
	}
	if (setsockopt(sock_fd, SOL_TCP, TCP_KEEPINTVL, (const char*)&opt_keepintvl, opt_len) < 0)
	{
		goto EXTOUT;
	}

	return OK;
#endif

EXTOUT:
	return ERROR_SOCK_SET_OPT;
}

/* *************************************************
 * output : conn_fd��������ӳɹ����fd)
 * return : success (OK) error(errcode)
 * comment: Ĭ�ϵĳ�ʱʱ���� 1s
 * ************************************************/
int non_block_conn(int *conn_fd, const char *ip, unsigned short port)
{
    int ret = OK;
	int sockfd = 0;
	struct sockaddr_in server_addr;
    socklen_t sockaddr_len = sizeof(server_addr);
	int epoll_handle = -1;
	struct epoll_event sock_event;
    struct epoll_event events[8];
    int error = 0;
    int len = 0;

	if ((epoll_handle = epoll_create(32)) == -1)
	{
		printf("epoll_create: %s\n", strerror(errno));
		return ERROR;
	}

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket: %s\n", strerror(errno));
		goto EXPOUT;
	}

	len = sizeof(server_addr);
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family     = AF_INET;
	server_addr.sin_addr.s_addr= inet_addr(ip);
	server_addr.sin_port       = htons(port);

	sock_event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLPRI;
	sock_event.data.fd = sockfd;

	(void)set_non_block(sockfd);

	epoll_ctl(epoll_handle, EPOLL_CTL_ADD, sockfd, &sock_event);
	if ((ret = connect(sockfd, (struct sockaddr*) &server_addr, sockaddr_len)) < 0)
	{
		if (errno != EINPROGRESS)
		{
            printf("connect %s : %d fail, reason: %s\n", ip, port, strerror(errno));
            goto EXPOUT;
		}
	}
	else
	{
		*conn_fd = sockfd;
		goto DONE;
	}

	/* ֻ�����һ��socket */
	ret = epoll_wait(epoll_handle, events, 8, 5*1000);
	if(ret == 0) /* û���κ��¼����������س�ʱ���� */
	{
        ret = ERROR_SOCK_CONN_TIMEOUT;
        goto EXPOUT;
	}
	else if(ret < 0)
	{
		ret = ERROR_EPOLL_WAIT;
		goto EXPOUT;
	}
	else /* ���¼�����������Ϊ�˸���ȫ�����������Ƿ��Ǵ����¼� */
	{
		*conn_fd = sockfd;
		error = 0;
		len = sizeof(int);
		if(0 != getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t*)&len))
		{
			ret = ERROR;
		}
		else
		{
            if(error != 0)
            {
            	printf("error: %s \n", strerror(error));
                errno = error;
            	return ERROR;
            }
            else
            {
			    ret = OK;
            }
		}
	}


DONE:
    if (epoll_handle > 0)
	{
		close(epoll_handle);
	}
    dlog1("connect %s %d success \n", ip, port);
	return OK;
EXPOUT: *conn_fd = -1;
	if (sockfd > 0)
		close(sockfd);
	return ret;
}

/* *************************************************
 * output :
 * return :
 * comment:�򵥵����ݶ�ȡ������select�����ݵȴ�������ȡ����Ϊbuffer_len,����
 * ��һ��û�ж�����û�ж�����ʱ������������Ǻ���ò�Ʒ����ļ��ʺ�С��
 * �˳�������
 * ��1������������
 * ��2������ϣ��������ݺ�Ȼ���ٶ���EAGAIN)
 * (3)��buffer���ˡ�
 * ************************************************/
int read_sock(int sockfd, char *buffer, int buffer_len, int *read_len)
{
    int ret = OK;
    int offset = 0;
    int len = 0;

    while(offset < buffer_len)
    {
#ifdef __WINDOWS
		len =  recv(sockfd, buffer + offset, buffer_len, 0);
#else
		len =  read(sockfd, buffer + offset, buffer_len);
#endif
       if(len > 0)
       {
    	   offset += len;
    	   continue;
       }
       else if(len == 0)
       {
    	   ret = ERROR_SOCK_REMOTE_CLOSE;
    	   break;
       }
       else /* ret < 0 �����*/
       {
    	   switch(errno)
    	   {
    	   case EAGAIN:
    		   ret = OK;  break;
    	   case EINTR:
    		   continue; break;/* �����ж� */
    	   default:
               printf("read_sock errno: %d \n", errno);
    		   ret = ERROR_SOCK_RD_DEFAULT;
    	   }
    	   break;
       }
    }

    *read_len = offset;

    return ret;
}

int recv_tmout(int sockfd, char *buffer, int buffer_len, int timeout)
{
	fd_set read_fds;
    int offset = 0;

    if(sockfd < 0 || buffer == NULL || buffer_len < 0)
    	return -1;

	struct timeval tval;
	tval.tv_sec = timeout / 1000;
	tval.tv_usec = (timeout % 1000)*1000;

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
            return ERROR_SOCK_CLOSE;
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
				else if (res == 0)
				{
                    return ERROR_SOCK_READ_TIMEOUT;
				}
				else
				{
					return ERROR_SOCK_SELECT;
				}
            }
            dlog1("read socket %d error : %s", sockfd, strerror(errno));
            return ERROR_SOCK_BASE;
		}
	}

	return offset;
}

int send_tmout(int sockfd, const char *buffer, int buffer_len, int timeout)
{
    int len = 0;

	fd_set write_fds;
    int offset = 0;
	struct timeval tval;

    if(sockfd < 0 || buffer == NULL || buffer_len < 0)
    	return -1;

	tval.tv_sec = timeout / 1000;
	tval.tv_usec = (timeout % 1000)*1000;

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
                	 dlog1("socket %d select error : %s", sockfd, strerror(errno));
                    return ERROR_SOCK_SELECT;
                }
                else
                {

                	return ERROR_SOCK_READ_TIMEOUT;
                }
        	}
            dlog1("write socket %d error : %s", sockfd, strerror(errno));
            return ERROR_SOCK_BASE;
        }
    }
    return offset;
}

int write_sock(int sockfd, const char *buffer, int buffer_len, int *send_len)
{
    int offset = 0;
    int ret = OK;
    int len = 0;
    while(offset < buffer_len)
    {
#ifdef __WINDOWS
    	len = send(sockfd, buffer + offset, buffer_len - offset, 0);
#else
		len = write(sockfd, buffer + offset, buffer_len - offset);
#endif
        if(len > 0)
        {
        	offset += len;
        	continue;
        }
        else
        {
        	switch(errno)
        	{
        	case EAGAIN:
                ret = ERROR_SOCK_EAGAIN; break;
        	case EPIPE:
        		ret = ERROR_SOCK_EPIPE; break;
        	default:
        		ret = ERROR_SOCK_WR_DEFAULT; break;
        	}
        	break;
        }
    }

    *send_len = offset;
    return ret;
}

//UDP�����ݷ���
int sendto_sock(int sockfd, const char *ip, unsigned short port, const char *buffer, int buffer_len)
{
	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	return sendto(sockfd, buffer, buffer_len, 0, (const struct sockaddr *) &server_addr,
			sizeof(server_addr));
}

int recvfrom_sock(int sockfd, char *buffer, int len, string &ip, unsigned short &port)
{
	struct sockaddr_in sock_addr;
	int sock_len =  sizeof(sock_addr);
    return recvfrom(sockfd, buffer, len, 0, (struct sockaddr*)&sock_addr, (socklen_t *)&sock_len);
}

int accept_sock(int sockfd, int *client_sock, struct sockaddr_in *sock_addr, int *sock_addr_len)
{
	int ret = 0;
	ret = accept(sockfd, (struct sockaddr*)sock_addr, (socklen_t*)sock_addr_len);
    if( ret < 0)
    {
        return ERROR_SOCK_ACCEPT;
    }

    *client_sock = ret;
    return OK;
}
}
