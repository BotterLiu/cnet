/******************************************************
 *   FileName: udp_socket.h
 *     Author: botter  2013-5-30 
 *Description:
 *******************************************************/

#ifndef UDP_SOCKET_H_
#define UDP_SOCKET_H_

#include "socket.h"
#include "sock_buffer.h"
#include "tprotocol.h"

class UdpSocket : public Socket
{
public:
    //需要实现的功能接口
	virtual int type(){return UDPSOCK;}
    virtual int send(const void *data, int len);

    //状态机处理函数
	virtual int to_close();
	virtual int to_read();
	virtual int to_write();

	int get_one_packet(FifoBuffer::Packet *packet);

public:

	UdpSocket();

	UdpSocket(int sock, SOCKSTATE s, struct sockaddr_in *peer_address);

	bool start_server(const char *ip, unsigned short port, NetService *appop);

	virtual ~UdpSocket()
	{
		this->release();
	}

	bool connect(const char *ip, unsigned short port);

	void shutdown();

	bool set_recv_buffer_size(int s);

    bool set_sndbuffer_max_size(int s);

	bool check_sockfd();

	int get_soerror();

    void close();

	void release();
private:
	long long get_index(const struct sockaddr_in *addr);

    long long get_index(const char *ip, unsigned short port);

public:

#define UDP_TYPE_SERVER  0
#define UDP_TYPE_CONN    1
#define UDP_TYPE_VIRTUAL 2

	int udp_type;
	//对端
	struct sockaddr_in  peer_addr;

	FifoBuffer read_buffer;       //读buffer的

	//对于客户端来讲，派生其的socket
	UdpSocket *server_sock;

	//udpserver派生出的socket
    map<long long, UdpSocket *> virtual_udpsock;
};

#endif /* UDP_SOCKET_H_ */
