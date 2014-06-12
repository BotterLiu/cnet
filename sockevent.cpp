/******************************************************
 *   FileName: SocketEvent.cpp
 *     Author: Triones  2012-9-13 
 *Description:
 *******************************************************/

#include <stdlib.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include "socket.h"
#include "eventbase.h"
#include "sockevent.h"
#include "log.h"
#include "debug.h"

static int convert_epoll_event(int epoll_event)
{
	int e = 0;
    if(epoll_event & EPOLLIN)
    	e |= EV_READ;
    if(epoll_event & EPOLLOUT)
    	e |= EV_WRITE;

    return e;
}

EpollEvent::EpollEvent(int max_event /* = MAX_EPOLL_EVENT */)
{
    _max_event = max_event;
    _inited = false;
    _events_num = 0;
    _events = NULL;
    _epoll_fd = -1;
}

EpollEvent::~EpollEvent()
{
    if(_epoll_fd > 0)
    {
    	close(_epoll_fd);
    	_epoll_fd = -1;
    }

    if(_events != NULL)
    {
    	free(_events);
    	_events = NULL;
    }
}

bool EpollEvent::init()
{
	if(!_inited)
	{
		_epoll_fd = epoll_create(_max_event);
        if(_epoll_fd < 0)
        {
        	dlog1("epoll_create error : %s", strerror(errno));
        	return false;
        }

        _events_num = min<int>(_max_event, 10240);

        _events = (epoll_event*)malloc(sizeof(struct epoll_event) * _events_num);
        if(_events == NULL)
        {
        	dlog1("%s: %d malloc error : %s ", __FILE__, __LINE__, strerror(errno));
            close(_epoll_fd);
            return false;
        }

        _inited = true;
	}

    return true;
}

bool EpollEvent::add_event(Socket *socket, bool enableRead, bool enableWrite)
{
    if(!init())
    	return false;

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.ptr = socket;
    // 设置要处理的事件类型
    ev.events = 0;

    if (enableRead) {
        ev.events |= EPOLLIN;
    }
    if (enableWrite) {
        ev.events |= EPOLLOUT;
    }

    dlog10("epoll add fd %d event : %d", socket->_fd, ev.events);

    /*
     *        EPOLLIN
              The associated file is available for read(2) operations.

       EPOLLOUT
              The associated file is available for write(2) operations.

       EPOLLRDHUP
              Stream  socket  peer closed connection, or shut down writing half of connection.  (This flag is especially
              useful for writing simple code to detect peer shutdown when using Edge Triggered monitoring.)

       EPOLLPRI
              There is urgent data available for read(2) operations.

       EPOLLERR
              Error condition happened on the associated file descriptor.   epoll_wait(2)  will  always  wait  for  this
              event; it is not necessary to set it in events.

       EPOLLHUP
              Hang  up happened on the associated file descriptor.  epoll_wait(2) will always wait for this event; it is
              not necessary to set it in events.

#define EPOLLRDNORM EPOLLRDNORM
    EPOLLRDBAND = 0x080,
#define EPOLLRDBAND EPOLLRDBAND
    EPOLLWRNORM = 0x100,
#define EPOLLWRNORM EPOLLWRNORM
    EPOLLWRBAND = 0x200,
#define EPOLLWRBAND EPOLLWRBAND
    EPOLLMSG = 0x400,
#define EPOLLMSG EPOLLMSG

       EPOLLET
     */
    ev.events |= EPOLLRDNORM | EPOLLRDBAND | EPOLLWRNORM | EPOLLWRBAND|EPOLLMSG|EPOLLERR|EPOLLPRI;

    bool rc = (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, socket->_fd, &ev) == 0);
    if(!rc)
    {
    	dlog1("epoll_ctl fail EPOLL_CTL_ADD : %s, fd %d", strerror(errno), socket->_fd);
    }

    return rc;
}

static string strepollevent(int event)
{
    string str;
    if(event & EPOLLIN) str += "-EPOLLIN";
    if(event & EPOLLOUT) str += "-EPOLLOUT";
    if(event & EPOLLRDNORM) str += "-EPOLLRDNORM";
    if(event & EPOLLRDBAND) str += "-EPOLLRDBAND";
    if(event & EPOLLWRNORM) str += "-EPOLLWRNORM";
    if(event & EPOLLWRBAND) str += "-EPOLLWRBAND";
    if(event & EPOLLPRI) str += "-EPOLLPRI";
    if(event & EPOLLHUP) str += "-EPOLLHUP";
    if(event & EPOLLERR) str += "-EPOLLERR";
    return str;

}

bool EpollEvent::set_event(Socket *socket, bool enableRead, bool enableWrite)
{
    if(!init())
    	return false;

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.ptr = socket;
    // 设置要处理的事件类型
    ev.events = 0;

    if (enableRead) {
        ev.events |= EPOLLIN;
    }
    if (enableWrite) {
        ev.events |= EPOLLOUT;
    }

    bool rc = (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, socket->_fd, &ev) == 0);
    if(!rc)
    {
        debug::print_trace();
    	dlog1("epoll_ctl fail EPOLL_CTL_ADD : %s, fd %d", strerror(errno), socket->_fd);
    }

    return rc;
}

bool EpollEvent::remove(Socket *socket)
{
    if(!init())
    	return false;

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.ptr = socket;
    ev.events = 0;

    dlog10("epoll remove fd %d event : %d", socket->_fd, ev.events);
    bool rc = (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, socket->_fd, &ev) == 0);
    if(!rc)
    {
    	dlog1("epoll_ctl fail EPOLL_CTL_DEL : %s, fd %d", strerror(errno), socket->_fd);
    }

    return rc;
}

int EpollEvent::dispatch(int timeout, SockEeventProcess *process)
{
    if(!init())
    	return false;

    int res = epoll_wait(_epoll_fd, _events, _events_num, timeout);
	if (res == -1)
	{
        dlog1("epoll_wait error : %s", strerror(errno));
        return errno != EINTR ? 0 : -1 ;
	}
	else if(res == 0)
	{
        dlog10("epoll_wait timeout");
		return EV_TIMEOUT;
	}
	else
	{
        dlog10("epoll_wait res = %d", res);
		for(int i = 0; i < res; i++)
		{
			dlog1("epoll_wait fd %d event %s ",((Socket*)(_events[i].data.ptr))->_fd, strepollevent(_events[i].events).c_str());
            process->event_process((Socket*)(_events[i].data.ptr), convert_epoll_event(_events[i].events));
		}
	}

	return 0;
}
