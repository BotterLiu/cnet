/******************************************************
 *   FileName: sockevent.h
 *     Author: liubo  2012-9-13 
 *Description:
 *******************************************************/

#ifndef EVENTOP_H____
#define EVENTOP_H____

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

class Socket;
class eventbase;
class SocketEvent;

//socket事件发生时的处理函数
class SockEeventProcess
{
public:
	virtual ~SockEeventProcess(){}
	virtual int event_process(Socket *sock_event, int event) = 0;
};

class SocketEvent
{
public:
	SocketEvent(){}
	virtual ~SocketEvent(){}

	virtual bool add_event(Socket *socket, bool enableRead, bool enableWrite) = 0;
	virtual bool set_event(Socket *socket, bool enableRead, bool enableWrite) = 0;
	virtual bool remove(Socket *socket) = 0;
	virtual int dispatch(int timeout, SockEeventProcess *process) = 0;
};

//class SelectEvent: public SocketEvent
//{
//public:
//	virtual bool add_event(Socket *socket, bool enableRead, bool enableWrite) = 0;
//	virtual bool set_event(Socket *socket, bool enableRead, bool enableWrite) = 0;
//	virtual bool remove(Socket *socket) = 0;
//	virtual int dispatch(int timeout, SockEeventProcess *process);
//public:
//	SelectEvent();
//	virtual ~SelectEvent();
//private:
//	int select_resize(int fdsz);
//private:
//	int _event_max_fds; /* Highest fd in fd set */
//	int _event_fd_size;
//
//	fd_set *_event_readset_in;
//	fd_set *_event_writeset_in;
//	fd_set *_event_readset_out;
//	fd_set *_event_writeset_out;
//
//	/**FD 跟 event 的对应关系 ****/
//	struct Socket **_event_fds;
//	eventbase *_event_base;
//};

class EpollEvent: public SocketEvent
{
#define MAX_EPOLL_EVENT   0xffffff  //初始支持的socket数目

#define MAX_MONITOR       10240     //每次能检测的最大数目

public:
	EpollEvent(int max_event = MAX_EPOLL_EVENT);
	virtual ~EpollEvent();

	virtual bool add_event(Socket *socket, bool enableRead, bool enableWrite);
	virtual bool set_event(Socket *socket, bool enableRead, bool enableWrite);
	virtual bool remove(Socket *socket);
	virtual int dispatch(int timeout, SockEeventProcess *process);

private:
	bool init();
private:
    bool           _inited;
    int            _max_event;
    int            _events_num;
	struct         epoll_event *_events;
	int            _epoll_fd;
};

#endif /* EVENTOP_H_ */
