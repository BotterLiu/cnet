/******************************************************
 *   FileName: sock_event.cpp
 *     Author: liubo  2012-8-22
 *Description:
 *******************************************************/
#include "socket.h"
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
#include "eventpool.h"
#include "log.h"


static SockEventFsm __sock_fsm[SOCKSTATE_NUM][SOCKEVENT_NUM];
static bool __fsm_init = false;

static const char *strstate(int s)
{
	switch (s)
	{
	case CLOSED:
		return "CLOSED";
	case CONNECTING:
		return "CONNECTING";
	case ESTABLISHED:
		return "ESTABLISHED";
	case LISTEN:
		return "LISTEN";
	default:
		return "STATE-INVALID";
	}
}

static const char *strevent(int e)
{
	switch (e)
	{
	case TOCLOSE:
		return "TOCLOSE";
	case TOREAD:
		return "TOREAD";
	case TOWRITE:
		return "TOWRITE";
	default:
		return "EVENT-INVALID";
	}
}

static void init_fsm()
{
    if(!__fsm_init)
    {
    	__sock_fsm[CLOSED][TOCLOSE].set(CLOSED, NULL);
        __sock_fsm[CLOSED][TOREAD].set(CLOSED, NULL);
        __sock_fsm[CLOSED][TOWRITE].set(CLOSED, NULL);

    	__sock_fsm[CONNECTING][TOCLOSE].set(CLOSED, &SockEventHandler::to_close);
        __sock_fsm[CONNECTING][TOREAD].set(CONNECTING, &SockEventHandler::to_read);
        __sock_fsm[CONNECTING][TOWRITE].set(CONNECTING, &SockEventHandler::to_write);

    	__sock_fsm[ESTABLISHED][TOCLOSE].set(CLOSED, &SockEventHandler::to_close);
        __sock_fsm[ESTABLISHED][TOREAD].set(ESTABLISHED, &SockEventHandler::to_read);
        __sock_fsm[ESTABLISHED][TOWRITE].set(ESTABLISHED, &SockEventHandler::to_write);

    	__sock_fsm[LISTEN][TOCLOSE].set(CLOSED, &SockEventHandler::to_close);
        __sock_fsm[LISTEN][TOREAD].set(LISTEN, &SockEventHandler::to_read);
        __sock_fsm[LISTEN][TOWRITE].set(LISTEN, NULL);

        __fsm_init = true;
    }

}

Socket::Socket()
{
	_fd = -1;
	_state = CLOSED; //socket 的状态
	memset(&_local_addr, 0, sizeof(struct sockaddr_in));
	memset(&_peer_addr, 0, sizeof(struct sockaddr_in));
    _tprotocol = __trans_protocol.get(DEFAULT_TPROTOCOL);

	_timeout = -1;
	_need_reconn = false;
	_reconn_timeval = 30;

	_net_service = NULL;
	_last_active_time = time(0);
	_event_base = NULL;
    ptr = NULL;
}

Socket::~Socket()
{
	release();
}

int Socket::type()
{
//	__sock_fsm[CLOSED][ESTABLISHED].next_state = CLOSED;
    return TCPSOCK;
}

bool Socket::check_sockfd()
{
    return false;
}

int Socket::get_state()
{
	return _state;
}

int Socket::run_fsm(int event)
{
	init_fsm();

    int e = event;
    int s = CLOSED;
    EventFun  next_handler = NULL;

    while(e != FSM_END)
    {
    	s = this->_state;
    	this->_state = __sock_fsm[s][e].next_state;
    	next_handler = __sock_fsm[s][e].next_handler;

    	if(next_handler == NULL)
    	{
    		dlog1("can not find hanlder state %s event %s", strstate(s), strevent(e));
    		e = FSM_END;
    	}
    	else
    	{
            dlog10("run fsm state %s event %s ", strstate(s), strevent(e));
    		e = (this->*(next_handler))();
    	}
    }
//    sleep(1);
    return FSM_END;
}

int Socket::event_process(int event)
{
    dlog10("into Socket::event_process fd %d, event %d", _fd, event);
    if(event & EV_READ)
    	run_fsm(TOREAD);
    if(event & EV_WRITE)
    	run_fsm(TOWRITE);

//    sleep(1);
    return FSM_END;
}

bool Socket::set_reuse_address(bool on)
{
	return set_int_option(SO_REUSEADDR, on ? 1 : 0);
}

static bool is_ipaddr(const char *host)
{
	char c;
	const char *p = host;
	bool ret = true;

	// 是ip地址格式吗?
	while ((c = (*p++)) != '\0')
	{
		if ((c != '.') && (!((c >= '0') && (c <= '9'))))
		{
			ret = false;
			break;
		}
	}

	return ret;
}

static bool set_addr(const char *host, unsigned short port, struct sockaddr &sock_addr)
{
    memset((void*)&sock_addr, 0, sizeof(sock_addr));
    struct sockaddr_in *addr = (struct sockaddr_in *)&sock_addr;
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    if(host == NULL || host[0] == 0)
    {
    	addr->sin_addr.s_addr = htonl(INADDR_ANY);
    	return true;
    }

    if(is_ipaddr(host))
    {
    	addr->sin_addr.s_addr = inet_addr(host);
    	return true;
    }
    else
    {
		// 是域名，解析一下
		struct hostent *myhostent = gethostbyname(host);
		if (myhostent != NULL)
		{
			memcpy(&(addr->sin_addr), *(myhostent->h_addr_list), sizeof(struct in_addr));
			return true;
		}
        return false;
    }
}

bool Socket::set_address(const char *host, unsigned short port)
{
    strncpy(_local_host, host, sizeof(_local_host) - 1);
    _local_port = port;
	return set_addr(host, port, _local_addr);
}

//设置本地地址
void Socket::set_address(const struct sockaddr *addr)
{
    memcpy(&_local_addr, addr, sizeof(struct sockaddr));
    const struct sockaddr_in* addr_in = (const struct sockaddr_in*)addr;
    char *ip = inet_ntoa(addr_in->sin_addr);
    if(ip != NULL)
    {
        strcpy(_local_host, ip);
    }
    _local_port = ntohs(addr_in->sin_port);
}

//设置对端地址
bool Socket::set_peer_addr(const char *host, unsigned short port)
{
    strncpy(_peer_host, host, sizeof(_peer_host) - 1);
    _peer_port = port;
	return set_addr(host, port, _peer_addr);
}

//设置对端地址
bool Socket::set_peer_addr(const struct sockaddr *addr)
{
    memcpy(&_peer_addr, addr, sizeof(struct sockaddr));
    const struct sockaddr_in* addr_in = (const struct sockaddr_in*)addr;
    char *ip = inet_ntoa(addr_in->sin_addr);
    if(ip != NULL)
    {
        strcpy(_peer_host, ip);
    }
    _peer_port = ntohs(addr_in->sin_port);
    return true;
}

//设置重连标志和重连时间间隔
void Socket::set_reconn(bool need, int interval)
{
    _need_reconn = need;
    _reconn_timeval = interval;
}

bool Socket::Socket::set_int_option(int option, int value)
{
	bool rc = false;
	if (check_sockfd())
	{
		rc = (setsockopt(_fd, SOL_SOCKET, option, (const void *) (&value), sizeof(value)) == 0);
	}
	return rc;
}

bool Socket::get_int_option(int option, int &value)
{
    value = 0;

	bool rc = false;
    int len = sizeof(value);
	if (check_sockfd())
	{
		rc = (getsockopt(_fd, SOL_SOCKET, option, (void *) (&value), (socklen_t *)&len) == 0);
	}
	return rc;
}

bool Socket::set_time_option(int option, int milliseconds)
{
	bool rc = false;
	if (check_sockfd())
	{
		struct timeval timeout;
		timeout.tv_sec = (int) (milliseconds / 1000);
		timeout.tv_usec = (milliseconds % 1000) * 1000000;
		rc = (setsockopt(_fd, SOL_SOCKET, option, (const void *) (&timeout), sizeof(timeout)) == 0);
	}
	return rc;
}

bool Socket::set_non_block(bool on)
{
	bool rc = false;

	int flags = fcntl(_fd, F_GETFL, NULL);
	if (flags >= 0)
	{
		if (!on)
		{
			flags &= ~O_NONBLOCK; // clear nonblocking
		}
		else
		{
			flags |= O_NONBLOCK; // set nonblocking
		}

		if (fcntl(_fd, F_SETFL, flags) >= 0)
		{
			rc = true;
		}
	}

	return rc;
}


int Socket::get_soerror()
{
	if (_fd == -1)
	{
		return EINVAL;
	}

	int soError = 0;
	socklen_t soErrorLen = sizeof(soError);
	if (getsockopt(_fd, SOL_SOCKET, SO_ERROR, (void *) (&soError), &soErrorLen) != 0)
	{
		return errno;
	}
	if (soErrorLen != sizeof(soError))
		return EINVAL;

	return soError;
}

bool Socket::bind(const char *ip, unsigned short port)
{
    if (!check_sockfd())
    	return false;

	if (!set_address(ip, port))
		return false;

	set_reuse_address(true);

	if (::bind(_fd, (struct sockaddr *) &_local_addr, sizeof(_local_addr)) < 0)
	{
		dlog1("bind %s %d error : %s", ip, port, strerror(errno));
		return false;
	}

	return true;
}

//设置socket传输的通信协议
void Socket::set_trans_protocol(int type)
{
    _tprotocol = __trans_protocol.get(type);
}

void Socket::set_timeout(int t)
{
    _timeout = t;
}

void Socket::set_net_service(NetService *ns)
{
	_net_service = ns;
}

void Socket::set_event_base(eventbase *eb)
{
	_event_base = eb;
}

//设置属于哪个event_pool;
void Socket::set_event_pool(eventpool *ep)
{
	_event_pool = ep;
}

bool Socket::check_timeout()
{
	return (time(0) - _last_active_time) > _timeout;
}

int Socket::send(const void *data, int len)
{
	return FSM_END;
}

void Socket::release()
{
	return;
}
