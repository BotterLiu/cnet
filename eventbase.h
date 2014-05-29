/******************************************************
*  CopyRight: 北京中交兴路科技有限公司(2012-2015)
*   FileName: eventbase.h
*     Author: liubo  2012-11-13 
*Description:
*******************************************************/

#ifndef EVENTBASE_H_
#define EVENTBASE_H_

#include <list>
#include <set>
#include "vos_thread.h"
#include "sockevent.h"
#include "sockmgr.h"
#include "log.h"

class SocketEvent;
class eventpool;

using namespace std;
class eventbase;
class EventBaseSockMgr;

class EventBaseSockMgr
{
public:
    friend class eventbase;

	EventBaseSockMgr(){}
	~EventBaseSockMgr(){}

	//主动连接发起连接操作
	void on_connecting(Socket *socket)
	{
		if(!check_in(socket))
		{
            _sockets.insert(socket);
            _online_queue.push(socket);
		}
	}

	//tcpserver接收到新的连接
	void on_new_connection(Socket *socket)
	{
		if(!check_in(socket))
		{
            _sockets.insert(socket);
            _online_queue.push(socket);
		}

		dlog2("WARN: EventBaseSockMgr::on_new_connection");
	}

	void on_connect_fail(Socket *socket)
	{
        dlog2("WARN: on_connect_fail");
		this->on_closed(socket);
	}
	//主动发起的连接连接成功
	void on_connected(Socket *socket)
	{

	}

	//socket断开
	void on_closed(Socket *socket)
	{
		if (check_in(socket))
		{
			_sockets.erase(socket);
            _online_queue.erase(socket);
            _waiting_queue.push(socket);
		}

		dlog2("WARN: EventBaseSockMgr::on_closed");
	}

	//有发送或是接收网络的成功事件，证明连接是正常的。
	void on_active(Socket *socket)
	{
        if(check_in(socket))
        {
            //将socket放到_online_queue队列的队尾
            _online_queue.erase(socket);
            _online_queue.push(socket);
        }
	//	dlog2("WARN: EventBaseSockMgr::on_active");
	}

	int size()
	{
		return (int)_sockets.size();
	}

	//将socket加入到重连队列中, 重连时间靠后的放到后面
	void add_reconn(Socket *socket)
	{
        Socket *head = _reconn_queue.begin();

        if(head == NULL)
        {
        	_reconn_queue.push(socket);
        	return;
        }

        if(socket->_next_reconn_time < head->_next_reconn_time)
        {
            socket->_pre = NULL;
            socket->_next = head;
            _reconn_queue._head = socket;
            _reconn_queue._size++;
            return;
        }

        for(; head != NULL ; head = head->_next)
        {
        	if(socket->_next_reconn_time < head->_next_reconn_time)
        	{
                socket->_next = head->_next;
                head->_next = socket->_next;
                socket->_pre = head;
                _reconn_queue._size++;
                return;
        	}
        }

        //说明已经找到最尾部了
        if(head == NULL)
        {
        	_reconn_queue.push(socket);
        }

        return;
	}

	vector<Socket *> get_reconn(time_t now)
    {
		vector<Socket *> vec;

		Socket *head = NULL, *temp = NULL;
		while ((head = _reconn_queue.begin()) != NULL)
		{
			if (head->_next_reconn_time <= now)
			{
                temp = _reconn_queue.pop();
                //只要取到外面操作的_pre 和 next 都置为空
                temp->_pre = temp->_next = NULL;
				vec.push_back(temp);
			}
			else
			{
				break;
			}
		}

		return vec;
    }

	vector<Socket *> get_timeout(time_t now, int timeout)
	{
        vector<Socket *> vec;
		Socket *head = NULL, *temp = NULL;
		while ((head = _online_queue.begin()) != NULL)
		{
			if (head->_last_active_time + timeout <= now)
			{
                temp = _online_queue.pop();
                //只要取到外面操作的_pre 和 next 都置为空
                temp->_pre = temp->_next = NULL;
				vec.push_back(temp);
			}
			else
			{
				break;
			}
		}

		return vec;
	}

    void add_waiting_close(Socket *socket)
    {
    	_waiting_queue.push(socket);
    }

	vector<Socket *> get_waiting_close()
	{
        vector<Socket *> vec;
		Socket *temp = NULL;

		while ((temp = _waiting_queue.begin()) != NULL)
		{
			//从当前eventbase的管理中删除
			_sockets.erase(temp);
			temp = _waiting_queue.pop();
			temp->_pre = temp->_next = NULL;
			vec.push_back(temp);
		}

		return vec;
	}

private:

    bool check_in(Socket *socket)
    {
    	return _sockets.find(socket) != _sockets.end();
    }

public:
	//所有在这里面的Socket
	set<Socket *> _sockets;
	//待关闭的Socket，以下三种队列，一个Socket只能处于其中的一个当中。
	TQueue<Socket> _waiting_queue;
	//待重连的队列
	TQueue<Socket> _reconn_queue;
	//主要是超时检测使用的
	TQueue<Socket> _online_queue;
};

class eventbase : public vos::RunThread, public SockEeventProcess
{
public:
	struct activeevent
	{
		Socket *event;
		int res;
	};

    friend struct Socket;
	eventbase(): _sock_event(NULL), _eventpool(NULL), _run(true)
    {
    }
	~eventbase();

    void   setid(int id);
    int    getid();

	int    init(SocketEvent *SocketEvent, int max_sock);
	int    stop();
    int    start();

	virtual int run_thread(int type);

	//socket读写事件回调
	virtual int event_process(Socket *socket, int event);

	bool add_event(Socket *socket, bool enableRead, bool enableWrite);
	bool set_event(Socket *socket, bool enableRead, bool enableWrite);
	bool remove(Socket *socket);

    //主动连接发起连接操作
    void on_connecting(Socket *socket)
    {
        return _sock_mgr.on_connecting(socket);
    }
    //主动发起的连接连接成功
	void on_connected(Socket *socket)
	{
        return _sock_mgr.on_connected(socket);
	}
    //连接失败
	void on_connect_fail(Socket *socket)
	{
        return this->on_closed(socket);
	}
    //tcpserver接收到新的连接
	void on_new_connection(Socket *socket)
	{
        return _sock_mgr.on_new_connection(socket);
	}
    //socket断开
	void on_closed(Socket *socket)
	{
        remove(socket);
        return _sock_mgr.on_closed(socket);
	}
    //有发送或是接收网络的成功事件，证明连接是正常的。
	void on_active(Socket *socket)
	{
        socket->_last_active_time = _syn_time.tv_sec;
        return _sock_mgr.on_active(socket);
	}

	int event_loop();

	void set_eventpool(eventpool * pool)
	{
		_eventpool = pool;
	}

	eventpool * get_eventpool()
	{
		return _eventpool;
	}

	time_t syntime(){return _syn_time.tv_sec;}
    //超时检测操作
	void timeout_process();

	//资源回收操作
	void recycle_process();

	//重连操作
	void reconn_process();

private:

    int _index;               //event base 的ID
    SocketEvent *_sock_event; //哪一个IO复用

	int _inuse_queue_size;

    eventpool *_eventpool;    //属于哪一个eventpoll

    //所有的时间操作都用这个时间
    struct timeval _syn_time;
    vos::Timer  _timer;

    EventBaseSockMgr _sock_mgr;

	bool _run;
	vos::ThreadPool _thread_pool;
};

#endif /* EVENTBASE_H_ */
