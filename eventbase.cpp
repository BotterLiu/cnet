/******************************************************
*   FileName: eventbase.cpp
*     Author: Triones  2012-11-13
*Description: 主要负责的功能：
*Description: （1）sock_event的加入和删除。
*******************************************************/
#include "sockmgr.h"
#include "sockevent.h"
#include "netop.h"
#include "eventbase.h"
#include "log.h"

using namespace std;

#define FSM_END    (-1)

static void timer_timeout_check(void *fun)
{
    eventbase *eb = (eventbase*)fun;
    eb->timeout_process();
    return;
}

eventbase::~eventbase()
{
    if(_sock_event != NULL)
    {
    	delete _sock_event;
    	_sock_event = NULL;
    }
}

void eventbase::setid(int id)
{
	_index = id;
}

int eventbase::getid()
{
	return _index;
}

int eventbase::init(SocketEvent *SocketEvent, int max_sock)
{
	_sock_event = SocketEvent;
	_thread_pool.regist(0, this, 1);
    return 0;
}

int eventbase::start()
{
   _run = true;
   return _thread_pool.start();
}

int eventbase::run_thread(int type)
{
	event_loop();
    return -1;
}

bool eventbase::add_event(Socket *socket, bool enableRead, bool enableWrite)
{
    dlog10("into eventbase::add_event fd %d", socket->_fd);
//    TAILQ_INSERT_TAIL(&_online_queue, socket, _eventbase_next);
    socket->set_event_base(this);
	return _sock_event->add_event(socket, enableRead, enableWrite);
}

bool eventbase::set_event(Socket *socket, bool enableRead, bool enableWrite)
{
    dlog10("into eventbase::set_event fd %d", socket->_fd);
	return _sock_event->set_event(socket, enableRead, enableWrite);
}

bool eventbase::remove(Socket *socket)
{
    dlog10("into eventbase::remove fd %d", socket->_fd);
    return _sock_event->remove(socket);
}

int eventbase::event_loop()
{
    dlog10("into int eventbase::event_loop()");

	_timer.timer_add(5000, timer_timeout_check, (void*) this);
	while (_run)
	{
		gettimeofday(&_syn_time, NULL);
		timeout_process();
		recycle_process();
        reconn_process();
		_sock_event->dispatch(1000, this);
	}

    return 0;
}

int eventbase::stop()
{
    _run = false;
    return 0;
}

int eventbase::event_process(Socket *socket, int event)
{
    socket->event_process(event);
	return 0;
}

#define EVENTBASE_TIMEOUT (30)

void eventbase::timeout_process()
{
    time_t now = _syn_time.tv_sec;
    vector<Socket *> vec = _sock_mgr.get_timeout(now, EVENTBASE_TIMEOUT);

	for (size_t i = 0; i < vec.size(); i++)
	{
		Socket *sock = vec[i];
		if (sock->type() == LISTEN)
		{
			sock->_last_active_time = now;
			_sock_mgr._online_queue.push(sock);
			dlog10("sock type = LISTEN push back");
		}
		else
		{
			if (sock->_state == CONNECTING)
			{
				dlog1("fd %d connect %s %d timeout", sock->_fd, sock->_peer_host, sock->_peer_port);
			}
			else
			{
				dlog1("fd %d timeout", sock->_fd);
			}

			remove(sock);
			_sock_mgr.add_waiting_close(sock);
		}
	}
}

void eventbase::recycle_process()
{
	time_t now = _syn_time.tv_sec;
	vector<Socket *> vec = _sock_mgr.get_waiting_close();
    Socket *sock;
    for(size_t i = 0; i < vec.size(); i++)
    {
    	sock = vec[i];
    	sock->release();
    	if(sock->_need_reconn)
    	{
            dlog10("add reconn, peer addr : %s %d", sock->_peer_host, sock->_peer_port);
            sock->_next_reconn_time = now + sock->_reconn_timeval;
    		_sock_mgr.add_reconn(sock);
    	}
    	else
    	{
    		//回收
    		__singleton_sockmgr.recycle(sock);
    	}
    }
}

//重连操作
void eventbase::reconn_process()
{
	time_t now = _syn_time.tv_sec;
	vector<Socket *> vec = _sock_mgr.get_reconn(now);
	for(size_t i = 0; i < vec.size(); ++i)
	{
       vec[i]->_next_reconn_time = syntime() + vec[i]->_reconn_timeval;
		vec[i]->asyn_connect();
	}
}
