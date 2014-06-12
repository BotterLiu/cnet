/******************************************************
 *   FileName: tcp_socket.h
 *     Author: Triones  2013-5-20 
 *Description:
 *******************************************************/

#ifndef TCP_SOCKET_H_
#define TCP_SOCKET_H_

#include "socket.h"
#include "sock_buffer.h"
#include "tprotocol.h"

//Ĭ��socket��С
#define DEFAULT_READ_BUFFER (10 * 1024)

class TcpListenSocket;
class TcpSocket;

class TcpListenSocket: public Socket
{
public:
    virtual ~TcpListenSocket(){}

	virtual int type()
	{
		return TCPLISTEN;
	}

	bool listen(const char *host, unsigned short port, int backlog = 32);
	//״̬��������
	virtual int to_close();
	virtual int to_read();
	virtual int to_write();

	virtual bool check_sockfd();
    void dispath(void *socket);
};

class TcpSocket : public Socket
{
public:
    //��Ҫʵ�ֵĹ��ܽӿ�
	virtual int type(){return TCPSOCK;}
    virtual int send(const void *data, int len);
    virtual void asyn_connect();

    //״̬��������
	virtual int to_close();
	virtual int to_read();
	virtual int to_write();

public:
	TcpSocket();

	virtual ~TcpSocket()
	{
		this->release();
	}

	bool connect(const char *ip, unsigned short port, int timeout = 0);

	void shutdown();

	void setUp(int socketHandle, struct sockaddr *hostAddress);

	bool set_keep_alive(bool on);

	bool set_reuse_address(bool on);

	bool set_solinger(bool doLinger, int seconds);

	bool set_tcp_nodelay(bool noDelay);

	bool set_recv_buffer_size(int s);

    bool set_sndbuffer_max_size(int s);

	bool check_sockfd();

	int get_soerror();

    void close();

	void release();

	int to_conn_resp();
public:

	FifoBuffer read_buffer;      //��buffer��

	PacketQueue _send_queue;     //���Ͷ���

	vos::Mutex  _mutex;
};

#endif /* TCP_SOCKET_H_ */
