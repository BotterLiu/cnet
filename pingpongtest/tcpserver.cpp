/******************************************************
*   FileName: tcpserver.cpp
*     Author: botter  2013-8-31 
*Description:
*******************************************************/
#include "netop.h"
#include "eventpool.h"
#include "socket.h"
#include "tcp_socket.h"
#include "sockmgr.h";
#include "log.h"

class TcpService : public NetService
{
public:
	virtual int on_connect(Socket *socket);
	virtual int on_close(Socket *socket);
	virtual int handle_packet(Socket *socket, const char *data, int len);

private:
	TcpService *_file_server;
};

int TcpService::on_connect(Socket *socket)
{
    printf("%s %d \n", socket->_peer_host, socket->_peer_port);
    dlog1("new connnection from %s %d", socket->get_peer_host(), socket->get_peer_port());
    socket->set_trans_protocol(TEXT_TPROTOCOL);
    return FSM_END;
}

int TcpService::on_close(Socket *socket)
{
    dlog1("onclose fd = %d , peer addr : %s %d", socket->get_peer_host(), socket->get_peer_port());
    return FSM_END;
}

int TcpService::handle_packet(Socket *socket, const char *data, int len)
{
	printf("recv len %d : %.*s \n", len, len, data);
//
//    char noop[1024] = {'1'};
//    socket->send(noop, sizeof(noop));
//    if(strncmp(data, "LOGI", 4) == 0)
//    {
//        const char *resp = "LACK 0 ";
//    	socket->send((const void *)resp, strlen(resp));
//    }

    sleep(1);

	socket->send((const void*) "ACK", 3);


	sleep(1);

	socket->release();

	return FSM_END;
}

class TcpServer
{
public:
	TcpServer()
    {
		_thread_num = 1;
		_net_service = NULL;
		_listen_sock = NULL;
    }

	virtual ~TcpServer(){}

	void set_process(NetService *appop)
	{
		_net_service = appop;
	}

	void set_max_thread(int thread_num)
	{
		_thread_num = thread_num;
	}

	bool start(const char *ip, unsigned short port, NetService *appop);

	bool stop();

private:
	eventpool _event_pool;

    int _thread_num;
    NetService *_net_service;
    TcpListenSocket *_listen_sock;
};

bool TcpServer::start(const char *ip, unsigned short port, NetService *appop)
{
	_listen_sock = (TcpListenSocket*)__singleton_sockmgr.get(TCPLISTEN);

	if (!_listen_sock->listen(ip, port, 128))
	{
		__singleton_sockmgr.recycle(_listen_sock);
		return false;
	}

    _listen_sock->set_net_service(appop);

    _event_pool.init(_thread_num);
    _event_pool.start();
    _event_pool.event_add(_listen_sock);


	dlog1("start server %s %d  ", ip, port);

	return true;
}

bool TcpServer::stop()
{

}

int main()
{
	INIT_LOG(NULL, LOG_CONSOLE, "test.log", 3);

    TcpServer tcpserver;
    if( ! tcpserver.start("127.0.0.1", 8080, new TcpService()))
    {
    	return -1;
    }

    while(1)
    {
    	sleep(100);
    }

    return 0;
}

