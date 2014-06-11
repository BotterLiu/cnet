/******************************************************
 *   FileName: udp_socket.cpp
 *     Author: botter  2013-5-30 
 *Description:
 *******************************************************/

#include "udp_socket.h"

#include "socket.h"
#include "sockutil.h"
#include "sockevent.h"
#include "eventbase.h"
#include "eventpool.h"
#include "netop.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include "log.h"

//Udp socket FifoBuffer 的默认大小
#define DEFAULT_READ_BUFFER   (1024 * 1024)

//每次对一个socket的读写最多为5次，防止某一个socket的数据过来，阻塞其他socket
#define MAX_ONE_WR_OPER        (16)

//UDP TCP发送的每个包的大小, 应用层进行拆包，
#define WR_SOCK_LEN            (2048)

static unsigned short get_port(struct sockaddr_in *sockaddr)
{
    return htons(sockaddr->sin_port);
}

static string get_ip(struct sockaddr_in *sockaddr)
{
    return inet_ntoa(sockaddr->sin_addr);
}

UdpSocket::UdpSocket()
{
    memset(&peer_addr, 0, sizeof(peer_addr));
	read_buffer.set_max_size(DEFAULT_READ_BUFFER);
    udp_type = UDP_TYPE_SERVER;
}

UdpSocket::UdpSocket(int sock, SOCKSTATE s, struct sockaddr_in *address)
{
    _fd = sock;
    _state = s;
    memset((void*)&peer_addr, 0, sizeof(peer_addr));
    memcpy(&peer_addr, address, sizeof(struct sockaddr_in));
    dlog1("new udp socket %s %d ", get_ip(&peer_addr).c_str(), get_port(&peer_addr));
	read_buffer.set_max_size(DEFAULT_READ_BUFFER);
}

int UdpSocket::get_one_packet(FifoBuffer::Packet *packet)
{
    return read_buffer.get_one_packet(packet);
}

void UdpSocket::close()
{
	if (udp_type != UDP_TYPE_VIRTUAL)
	{
		if (_fd > 0)
		{
			::close(_fd);
			_fd = -1;
		}
	}
}

void UdpSocket::release()
{
	if (udp_type != UDP_TYPE_VIRTUAL)
	{
		event_base->remove(this);

		if (_fd > 0)
		{
			vos::close(_fd);
			_fd = -1;
		}

		_state = CLOSED;

		_net_service = NULL;

		send_buffer.clear();
	}
}

int UdpSocket::send(const void *data, int len)
{
	if (udp_type == UDP_TYPE_VIRTUAL)
	{
        dlog1("udp-client add ctfo pack %s %d  len %d", get_ip(&peer_addr).c_str(), get_port(&peer_addr), len);
		server_sock->send_buffer.add_ctfo_pack(data, len, peer_addr);
		server_sock->event_base->set_event(server_sock, true, true);
	}
	else
	{
        dlog1("udp-server add ctfo pack %s %d  len %d", get_ip(&peer_addr).c_str(), get_port(&peer_addr), len);

		send_buffer.add_ctfo_pack(data, len, peer_addr);
		event_base->set_event(this, true, true);
	}

	return OK;
}

//状态机处理函数
int UdpSocket::to_close()
{
    release();
    return FSM_END;
}

static long long __recv_sum = 0;

int UdpSocket::to_read()
{
	int ret = FSM_END;

	char *buffer = new char[WR_SOCK_LEN];

	struct sockaddr recvaddr;
	int addr_len = sizeof(recvaddr);

	memset(&recvaddr, 0, sizeof(recvaddr));
	int recv_bytes = ::recvfrom(_fd, buffer, WR_SOCK_LEN, 0, (struct sockaddr *) &(recvaddr), (socklen_t*) (&addr_len));

	if (recv_bytes <= 0)
	{
		dlog1("recvfrom  %s %d  ret %d error(%s)", get_ip((struct sockaddr_in*) &recvaddr).c_str(),
				get_port((struct sockaddr_in*) &recvaddr), recv_bytes, strerror(errno));
		if (errno != EAGAIN)
			ret = FSM_TOCLOSE;

		delete[] buffer;
		return ret;
	}

	__recv_sum += recv_bytes;
	dlog1("recv from %s %d %d bytes, total recv : %lld", get_ip((struct sockaddr_in*) &recvaddr).c_str(),
			get_port((struct sockaddr_in*) &recvaddr), recv_bytes, __recv_sum);

	if (udp_type == UDP_TYPE_CONN)
	{
		read_buffer.add_data(buffer, recv_bytes);
		_net_service->on_read(this);
	}
	else // UDP_TYPE_SERVER, 找到对应的客户端Socket;
	{
		long long index = get_index((const struct sockaddr_in *) &recvaddr);

		if (virtual_udpsock.find(index) == virtual_udpsock.end())
		{
			UdpSocket *udp_socket = new UdpSocket(_fd, ESTABLISHED, (struct sockaddr_in*) &recvaddr);
			udp_socket->set_split(&__ctfo_split);
			udp_socket->set_net_service(_net_service);
			udp_socket->udp_type = UDP_TYPE_VIRTUAL;
			udp_socket->server_sock = this;
			virtual_udpsock.insert(make_pair(index, udp_socket));
		}

		UdpSocket *socket = virtual_udpsock[index];
        dlog1("get udp socket %s %d, index %lld", get_ip(&(socket->peer_addr)).c_str(),
        		get_port(&(socket->peer_addr)), index);
		socket->read_buffer.add_data(buffer, recv_bytes);
		socket->_net_service->on_read(socket);
	}

	delete[] buffer;

	return ret;
}

static long long __send_sum = 0;

int UdpSocket::to_write()
{
    dlog10("into int NetService::to_write(Socket *socket)");

    if(_fd < 0 || _state != ESTABLISHED)
    {
    	dlog1("to_write fail, _fd : %d , _state : %d", _fd, _state);
    	return FSM_END;
    }

    //一次event_loop 只发送一个send_data 的数据包，减少发送速度。

    SendBuffer::send_buffer *send_data = send_buffer.get_data();
    const char *buffer = &(send_data->data);
    int buffer_len = send_data->len;

    int offset = 0;
    while(offset < buffer_len)
    {
        int send_bytes = ::sendto(_fd, buffer + offset, min(WR_SOCK_LEN, buffer_len - offset), 0,
			            (struct sockaddr *)&(send_data->sock_addr), sizeof(send_data->sock_addr));
	    if(send_bytes <= 0)
	    {
	        dlog1("sendto %s %d ret %d error(%s)",get_ip(&(send_data->sock_addr)).c_str(),
		    		get_port(&(send_data->sock_addr)),  send_bytes, strerror(errno));

            if(errno == EAGAIN)
            {
	            send_buffer.add_data(buffer + offset, buffer_len - offset, true);
	            SendBuffer::free(send_data);
                return FSM_END;
            }
            SendBuffer::free(send_data);
            return FSM_TOCLOSE;

	    }

        offset += send_bytes;
	    __send_sum += send_bytes;

	    dlog1("sendto %s %d %d bytes, total send len : %lld", get_ip(&(send_data->sock_addr)).c_str(),
	    		get_port(&(send_data->sock_addr)),  send_bytes, __send_sum);
    }

    if(send_buffer.is_empty())
    {
        event_base->set_event(this, true, false);
    }

    SendBuffer::free(send_data);
    return FSM_END;
}

bool UdpSocket::start_server(const char *ip, unsigned short port, NetService *appop)
{
    if(!check_sockfd())
    	return false;

    if( this->bind(ip, port))
    {
    	_net_service = appop;
        udp_type = UDP_TYPE_SERVER;
        _state = ESTABLISHED;
        return true;
    }

    return false;
}

bool UdpSocket::connect(const char *ip, unsigned short port)
{
	if (!check_sockfd())
		return false;

	memset(static_cast<void *>(&peer_addr), 0, sizeof(peer_addr));

	peer_addr.sin_family = AF_INET;
	peer_addr.sin_port = htons(static_cast<short>(port));

	bool rc = true;
	// 是空字符，设置成INADDR_ANY

	if (ip == NULL || ip[0] == '\0')
	{
		peer_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else
	{
		char c;
		const char *p = ip;
		bool isIPAddr = true;

		// 是ip地址格式吗?
		while ((c = (*p++)) != '\0')
		{
			if ((c != '.') && (!((c >= '0') && (c <= '9'))))
			{
				isIPAddr = false;
				break;
			}
		}

		if (isIPAddr)
		{
			peer_addr.sin_addr.s_addr = inet_addr(ip);
		}
		else
		{
			// 是域名，解析一下
			struct hostent *myHostEnt = gethostbyname(ip);
			if (myHostEnt != NULL)
			{
				memcpy(&(peer_addr.sin_addr), *(myHostEnt->h_addr_list), sizeof(struct in_addr));
			}
			else
			{
				rc = false;
			}
		}
	}

    _state = ESTABLISHED;
	udp_type = UDP_TYPE_CONN;

	return rc;
}

void UdpSocket::shutdown()
{
	if (_fd != -1)
	{
		::shutdown(_fd, SHUT_WR);
	}
}

bool UdpSocket::set_recv_buffer_size(int s)
{
    read_buffer.set_max_size(s);
    return true;
}

bool UdpSocket::set_sndbuffer_max_size(int s)
{
    return true;
}

bool UdpSocket::check_sockfd()
{
	if (_fd == -1 && (_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		return false;
	}

	int optval = 1024 * 1024;
	int optlen = sizeof(optval);

    set_non_block(true);
	setsockopt(_fd, SOL_SOCKET, SO_RCVBUF, (char*) &optval, optlen);
	setsockopt(_fd, SOL_SOCKET, SO_SNDBUF, (char*) &optval, optlen);

	return true;
}

long long UdpSocket::get_index(const struct sockaddr_in *addr)
{
    long long index = 0;
    index = (uint32_t)(addr->sin_addr.s_addr);
    index = (index << 4) | addr->sin_port;

    return index;
}

long long UdpSocket::get_index(const char *ip, unsigned short port)
{
	long long index = inet_addr(ip);
	index = (index << 4) | htons(port);
	return index;
}



