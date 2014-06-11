/******************************************************
 *   FileName: tcp_socket.cpp
 *     Author: botter  2013-5-20 
 *Description:
 *******************************************************/

#include "sockmgr.h"
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
#include "tprotocol.h"
#include "runinfo.h"

#define ONE_SEND_MAX (16)

bool TcpListenSocket::listen(const char *host, unsigned short port, int backlog/* = 32 */)
{
	if (!bind(host, port))
		return false;
	if (::listen(_fd, backlog) < 0)
	{
		dlog1("listen %s %d error : %s ", host, port, strerror(errno));
		return false;
	}

	_state = LISTEN;

    return true;
}

void TcpListenSocket::dispath(void *socket)
{
	dlog10("into void TcpListenSocket::dispath(void *socket)");

    if(_event_base->get_eventpool() != NULL)
    	_event_base->get_eventpool()->event_add((Socket*)socket);
    else
    	_event_base->add_event((Socket*)socket, true, false);

    return;
}

//状态机处理函数
int TcpListenSocket::to_close()
{
	dlog10("into int TcpListenSocket::to_close()");
	::close(_fd);
	_fd = -1;
    return FSM_END;
}

int TcpListenSocket::to_read()
{
	dlog10("into TcpListenSocket::to_read()");

	struct sockaddr_in client_addr;
	int len = sizeof(client_addr);
    int clientfd = ::accept(_fd, (struct sockaddr*) &client_addr, (socklen_t*)&len);
    if(clientfd < 0)
    {
    	dlog1("accept error : %s \n", strerror(errno));
    	return FSM_END;
    }

    set_non_block(clientfd);
    TcpSocket *tcpsock = (TcpSocket*)__singleton_sockmgr.get(TCPSOCK);
    tcpsock->_fd = clientfd;
    tcpsock->_state = ESTABLISHED;
    tcpsock->set_peer_addr((struct sockaddr*)&client_addr);
    tcpsock->set_net_service(_net_service);

    _net_service->on_connect(tcpsock);

    dispath(tcpsock);

    return FSM_END;
}

int TcpListenSocket::to_write()
{
    return FSM_END;
}

bool TcpListenSocket::check_sockfd()
{
	if (_fd == -1 && (_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		return false;
	}
	return true;
}

TcpSocket::TcpSocket()
{
	read_buffer.set_max_size(DEFAULT_READ_BUFFER);
	_fd = socket(AF_INET, SOCK_STREAM, 0);
    _send_queue.set_mutex(&_mutex);
}

void TcpSocket::close()
{
	dlog10("into TcpSocket::close()");

	if(_fd > 0)
	{
		::close(_fd);
		_fd = -1;
	}
}

void TcpSocket::release()
{
	dlog10("into TcpSocket::release()");
	if (_fd > 0)
	{
		vos::close(_fd);
		_fd = -1;
	}
	_state = CLOSED;
//	_net_service = NULL;
	_send_queue.clear();
}

int TcpSocket::send(const void *data, int len)
{
    if(_state != ESTABLISHED)
    {
        dlog1("tcpsocket::send socket is not ESTABLISHED");
    	return FSM_END;
    }

    dlog10("into int TcpSocket::send(const void *data, int len) ");
    _tprotocol->encode((const char *)data, len, &_send_queue);
	_event_base->set_event(this, true, true);
	return OK;
}

static bool check_asynconn_evn(TcpSocket *socket)
{
    if(socket->_event_pool == NULL)
    {
    	dlog1("socket's _event_pool is NULL, can not do asyn-conn");
    }

    if(socket->_event_base == NULL)
    {
    	socket->_event_base = socket->_event_pool->select_eventbase(socket);
    }

    if(socket->_peer_host[0] == 0 || socket->_peer_port == 0)
    {
    	dlog1("peer addr is not set, can not do asyn-conn");
        return false;
    }

    if(!socket->check_sockfd())
    {
    	dlog1("check sockfd fail, can not do asyn-conn");
        return false;
    }

    return true;
}

void TcpSocket::asyn_connect()
{
	dlog10("into TcpSocket::asyn_connect");
    if(!check_asynconn_evn(this))
    {
    	return;
    }

	errno = 0;
	set_non_block(true);

	_state = CONNECTING;
	if (0 != ::connect(_fd, (struct sockaddr *) &_peer_addr, sizeof(_peer_addr)))
	{
		if(errno == EINPROGRESS)
		{
            dlog1("asyn-connecting %s %d, fd %d", _peer_host, _peer_port, _fd);
            _event_base->add_event(this, true, true);
            _event_base->on_connecting(this);
		}
		else
		{
        	dlog1("connect %s %d fail : %s", _peer_host, _peer_port, strerror(errno));
            release();
		}
		return;
	}

	_state = ESTABLISHED;
	dlog1("connect %s %d success", _peer_host, _peer_port);
    _event_base->on_connected(this);
	_net_service->on_connect(this);

	return ;
}

//状态机处理函数
int TcpSocket::to_close()
{
	dlog10("TcpSocket::to_close()");
//    release();
    _event_base->on_closed(this);
    return FSM_END;
}

int TcpSocket::to_read()
{
	dlog10("into TcpSocket::to_read()");

    if(_state == CONNECTING)
    {
        return to_conn_resp();
    }

    int len = 0;
	if(read_buffer.relative_avail() == 0)
	{
        read_buffer.reset();
	}

    int ret = FSM_END;

	len = ::read(_fd, read_buffer.write_pos(),
			read_buffer.relative_avail());

	if (len > 0)
	{
        _event_base->on_active(this);

		read_buffer.write_offset_add(len);
		PacketQueue recv_queue;
		int offset = _tprotocol->decode(read_buffer.read_pos(), (int)read_buffer.use(), &recv_queue);
		read_buffer.read_offset_add(offset);
        Packet *pack = NULL;

        while((pack = recv_queue.pop()) != NULL)
        {
    		__net_runinfo.on_read(pack->decode_length());
        	_net_service->handle_packet(this, pack->decode_data(), pack->decode_length());
        	delete pack;
        }
	}
	else if (len < 0 && (errno == EAGAIN || errno == EINTR))
	{
		return FSM_END;
	}
	else
	{
		if(len == 0)
		{
			dlog1("recv = 0, close fd %d, errno : %d ", _fd, errno);
		}
		else
		{
			dlog1("recv = -1, close fd %d, reason : %s ", _fd, strerror(errno));
		}
        return TOCLOSE;
	}

	return ret;
}

int TcpSocket::to_write()
{
    dlog10("into int NetService::to_write(Socket *socket)");

    if(_state == CONNECTING)
    {
        return to_conn_resp();
    }

    if(_send_queue.size() == 0)
    {
    	_event_base->set_event(this, true, false);
    	return true;
    }

    int offset = 0;
    int len = 0;
    const char *buffer = NULL;
    int buffer_len = 0;

    Packet *pack = NULL;

    int num = 0;
    while(++num <= ONE_SEND_MAX && (pack = _send_queue.pop()) != NULL)
    {
        buffer = pack->encode_data();
		buffer_len = pack->encode_length();

        offset = 0;
		while (offset < buffer_len)
		{
			len = ::write(_fd, buffer + offset, buffer_len - offset);
			if (len > 0)
			{
				_event_base->on_active(this);
                dlog10("send len %d, _fd %d", len, _fd);
				offset += len;
				continue;
			}
			else
			{
                int err = errno;
                if(err == EINTR)
                	continue;
                if(err == EAGAIN)
                {
					dlog1("_fd %d already send %d write EAGAIN", _fd, offset);
					//把整个包重发一遍而不是半个包，半个包会引起包序的混乱len
                    Packet *p = new Packet;
                    p->copy(buffer + offset, buffer_len - offset);
                    _send_queue.push(p);
                    delete pack;
					return FSM_END;
                }
                else
                {
					dlog1("_fd %d write error : %s toclose", _fd, strerror(errno));
					delete pack;
					return TOCLOSE;
                }
			}
		}
		__net_runinfo.on_send(buffer_len);

		delete pack;
	}

    //说明没有数据了
    if(num <= ONE_SEND_MAX)
    {
        _event_base->set_event(this, true, false);
    }

    return FSM_END;
}

//非阻塞连接
bool TcpSocket::connect(const char *ip, unsigned short port, int timeout /* = 0 */)
{
	dlog10("into bool TcpSocket::connect(const char *ip, unsigned short port, int timeout =0)");

    if(!set_peer_addr(ip, port) || !check_sockfd())
    	return false;

    if(timeout > 0)
    {
        set_time_option(SO_RCVTIMEO, timeout);
        set_time_option(SO_SNDTIMEO, timeout);
    }

    if(0 != ::connect(_fd, (struct sockaddr *)&_peer_addr, sizeof(_peer_addr)))
    {
        dlog1("connect %s %d error : %s ", ip, port,  strerror(errno));
        return false;
    }

    //重新设置回来
    set_time_option(SO_RCVTIMEO, 0);
    set_time_option(SO_SNDTIMEO, 0);

    _state = ESTABLISHED;

    return true;
}

void TcpSocket::shutdown()
{
	dlog10("TcpSocket::to_close()");

	if (_fd != -1)
	{
		::shutdown(_fd, SHUT_WR);
	}
}

void TcpSocket::setUp(int socketHandle, struct sockaddr *hostAddress)
{
	dlog10("TcpSocket::to_close()");

	close();
	_fd = socketHandle;
	memcpy(&_local_addr, hostAddress, sizeof(_local_addr));
}

bool TcpSocket::set_keep_alive(bool on)
{
	return set_int_option(SO_KEEPALIVE, on ? 1 : 0);
}

bool TcpSocket::set_solinger(bool doLinger, int seconds)
{
	bool rc = false;
	struct linger lingerTime;
	lingerTime.l_onoff = doLinger ? 1 : 0;
	lingerTime.l_linger = seconds;
	if (check_sockfd())
	{
		rc = (setsockopt(_fd, SOL_SOCKET, SO_LINGER, (const void *) (&lingerTime), sizeof(lingerTime)) == 0);
	}

	return rc;
}

bool TcpSocket::set_tcp_nodelay(bool noDelay)
{
	bool rc = false;
	int noDelayInt = noDelay ? 1 : 0;
	if (check_sockfd())
	{
		rc = (setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (const void *) (&noDelayInt), sizeof(noDelayInt)) == 0);
	}
	return rc;
}

bool TcpSocket::set_recv_buffer_size(int s)
{
    read_buffer.set_max_size(s);
    return true;
}

bool TcpSocket::set_sndbuffer_max_size(int s)
{
    return true;
}

bool TcpSocket::check_sockfd()
{
	if (_fd == -1 && (_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		return false;
	}
	return true;
}

int TcpSocket::get_soerror()
{
	if (_fd == -1)
	{
		return EINVAL;
	}

	int lastError = errno;
	int soError = 0;
	socklen_t soErrorLen = sizeof(soError);
	if (getsockopt(_fd, SOL_SOCKET, SO_ERROR, (void *) (&soError), &soErrorLen) != 0)
	{
		return lastError;
	}
	if (soErrorLen != sizeof(soError))
		return EINVAL;

	return soError;
}

int TcpSocket::to_conn_resp()
{
    dlog10("into TcpSocket::to_conn_resp");
    int err = get_soerror();
    if(err == 0)
    {
    	_state = ESTABLISHED;
        _event_base->set_event(this, true, false);
    	dlog1("asyn-connect %s %d success, fd %d", _peer_host, _peer_port, _fd);
        _last_active_time = _event_base->syntime();
        _event_base->on_connected(this);
    	_net_service->on_connect(this);
    }
    else
    {
    	dlog1("asyn-connect %s %d fail : %s, fd %d", _peer_host, _peer_port, strerror(err), _fd);
        _event_base->on_connect_fail(this);
    }
    return FSM_END;
}
