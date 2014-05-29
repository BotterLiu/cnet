/******************************************************
 *  CopyRight: �����н���·�Ƽ����޹�˾(2012-2015)
 *   FileName: fileserver.cpp
 *     Author: liubo  2012-10-15
 *Description:
 *******************************************************/
#include "fileserver.h"
#include "header.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "log.h"
#include "vos.h"
#include "netop.h"
#include "sockevent.h"
#include "sockutil.h"
#include "../header.h"
#include "tcp_socket.h"

static bool WriteFile(const char *szName, const char *szBuffer, const int nLen);
static char *ReadFile(const char *szFile, int &nLen);

//��ʼ���������������ֻ�г�ʼ���Ĺ��ܣ�û��ִ�ж����Ĺ��ܡ�
bool FileServer::start(const char *ip, unsigned short port, NetService *appop)
{
	TcpListenSocket *e = new TcpListenSocket;

    if(! e->start_server(ip, port, appop))
    {
    	delete e;
    	return false;
    }

    _event_pool.init(1);
    _event_pool.event_add(e);

    _event_pool.start();

	dlog1("start server %s %d  ", ip, port);
	return true;
}

bool FileServer::stop()
{
	_event_pool.stop();
    return true;
}

int FileService::on_connect(Socket *socket)
{
	TcpSocket * tcpsock = dynamic_cast<TcpSocket *> (socket);
    tcpsock->set_recv_buffer_size(1000 * 1024);

//dlog1("FileServer new connection %s %d \n", socket->remote_addr.get_ip().c_str(), socket->remote_addr.get_port());
	return FSM_END;
}

int FileService::on_close(Socket *socket)
{
    return FSM_END;
}

int FileService::on_read(Socket *socket)
{
    dlog10("into int FileService::on_read(Socket *socket)");

	FifoBuffer::Packet packet;

	while (socket->get_one_packet(&packet))
	{
		char *data = (char*) packet.data;

		bigheader *header = (bigheader*) data;
		int cmd = ntohs(header->cmd);

		//    printf("cmd = %d \n", cmd);
		if (cmd == BIG_OPEN_REQ)
		{
			//����֤��ֱ��Ĭ��ȫ��ͨ��
			bigloginrspall resp;
			socket->send((const char*) &resp, sizeof(bigloginrspall));
		}
		else if (cmd == BIG_WRITE_REQ)
		{
			bigwritereq * req = (bigwritereq*) (data + sizeof(bigheader));
			        printf("write file %s \n", req->path);
			WriteFile((const char*) (req->path), (const char*) ((char*) req + sizeof(bigwritereq)),
					ntohl(req->data_len));
			bigwriterspall resp(0);
			socket->send((const char*) &resp, sizeof(resp));
		}
		else if (cmd == BIG_READ_REQ)
		{

			bigreadreq *req = (bigreadreq*) (data + sizeof(bigheader));
			int len = 0;
			char *buffer = ReadFile((const char*) (req->path), len);
			if (buffer == NULL)
			{
				bigreadrspall resp(1, 0);
				socket->send((const char*) &resp, sizeof(resp));
			}
			else
			{
				bigreadrspall resp(0, len);
				char *data = (char*) malloc(sizeof(bigreadrspall) + len);
				if (data == NULL)
				{
					resp.rsp.result = 1;
					resp.rsp.data_len = 0;
					socket->send((const char*) &resp, sizeof(resp));
				}
				else
				{
					memcpy(data, (void*) &resp, sizeof(bigreadrspall));
					memcpy(data + sizeof(bigreadrspall), data, len);
					socket->send((const char*) &resp, sizeof(resp) + len);
					free(data);
				}
			}
		}
		else if (cmd == BIG_CLOSE_REQ)
		{

		}
		else
		{
			printf("cmd = %d , can not parse \n", cmd);
		}
	}
	return FSM_END;
}


bool WriteFile(const char *szName, const char *szBuffer, const int nLen)
{
#ifdef _WIN32
	FILE *fp = fopen(szName, "wb");
	if (fp == NULL)
	{
		return false;
	}
	fwrite(szBuffer, nLen, 1, fp);
	fclose(fp);
#else
	int fp = open( szName, O_CREAT | O_TRUNC | O_WRONLY );
	if ( fp < 0 )
	{
		return false;
	}
	write( fp, szBuffer, nLen );
	::close( fp );
	chmod( szName, 0777 );
#endif

	return true;
}

// ��ȡ�ļ�
char *ReadFile(const char *szFile, int &nLen)
{
	char *szBuffer = NULL;
#ifdef _WIN32

	FILE* fp = fopen(szFile, "rb");
	if (fp != NULL)
	{
		int len = 0;
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);

		szBuffer = new char[len + 1];

		fseek(fp, 0, SEEK_SET);
		fread(szBuffer, 1, len, fp);
		szBuffer[len] = 0;

		nLen = len;

		fclose(fp);
	}
	else
	{
		nLen = 0;
		return NULL;
	}

#else

	int fp = open( szFile , O_RDONLY );

	if ( fp >= 0 )
	{
		// �ȵõ��ļ��Ĵ�С
		struct stat buf;
		fstat( fp , &buf );

		int len = buf.st_size;

		szBuffer = new char[len+1];

		read( fp , szBuffer , len );
		szBuffer[len] = 0;

		nLen = len;

		::close( fp );
	}
	else
	{
		nLen = 0;
		return NULL;
	}

#endif

	return szBuffer;
}

int main()
{
	INIT_LOG(NULL, LOG_CONSOLE, "test.log", 10);
	FileServer file_server;
	file_server.start("127.0.0.1", 8808, new FileService);

	while (1)
	{
		sleep(5);
	}

    return 0;
}

