/******************************************************
*  CopyRight: �����н���·�Ƽ����޹�˾(2012-2015)
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

	//�������ӷ������Ӳ���
	void on_connecting(Socket *socket)
	{
		if(!check_in(socket))
		{
            _sockets.insert(socket);
            _online_queue.push(socket);
		}
	}

	//tcpserver���յ��µ�����
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
	//����������������ӳɹ�
	void on_connected(Socket *socket)
	{

	}

	//socket�Ͽ�
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

	//�з��ͻ��ǽ�������ĳɹ��¼���֤�������������ġ�
	void on_active(Socket *socket)
	{
        if(check_in(socket))
        {
            //��socket�ŵ�_online_queue���еĶ�β
            _online_queue.erase(socket);
            _online_queue.push(socket);
        }
	//	dlog2("WARN: EventBaseSockMgr::on_active");
	}

	int size()
	{
		return (int)_sockets.size();
	}

	//��socket���뵽����������, ����ʱ�俿��ķŵ�����
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

        //˵���Ѿ��ҵ���β����
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
                //ֻҪȡ�����������_pre �� next ����Ϊ��
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
                //ֻҪȡ�����������_pre �� next ����Ϊ��
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
			//�ӵ�ǰeventbase�Ĺ�����ɾ��
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
	//�������������Socket
	set<Socket *> _sockets;
	//���رյ�Socket���������ֶ��У�һ��Socketֻ�ܴ������е�һ�����С�
	TQueue<Socket> _waiting_queue;
	//�������Ķ���
	TQueue<Socket> _reconn_queue;
	//��Ҫ�ǳ�ʱ���ʹ�õ�
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

	//socket��д�¼��ص�
	virtual int event_process(Socket *socket, int event);

	bool add_event(Socket *socket, bool enableRead, bool enableWrite);
	bool set_event(Socket *socket, bool enableRead, bool enableWrite);
	bool remove(Socket *socket);

    //�������ӷ������Ӳ���
    void on_connecting(Socket *socket)
    {
        return _sock_mgr.on_connecting(socket);
    }
    //����������������ӳɹ�
	void on_connected(Socket *socket)
	{
        return _sock_mgr.on_connected(socket);
	}
    //����ʧ��
	void on_connect_fail(Socket *socket)
	{
        return this->on_closed(socket);
	}
    //tcpserver���յ��µ�����
	void on_new_connection(Socket *socket)
	{
        return _sock_mgr.on_new_connection(socket);
	}
    //socket�Ͽ�
	void on_closed(Socket *socket)
	{
        remove(socket);
        return _sock_mgr.on_closed(socket);
	}
    //�з��ͻ��ǽ�������ĳɹ��¼���֤�������������ġ�
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
    //��ʱ������
	void timeout_process();

	//��Դ���ղ���
	void recycle_process();

	//��������
	void reconn_process();

private:

    int _index;               //event base ��ID
    SocketEvent *_sock_event; //��һ��IO����

	int _inuse_queue_size;

    eventpool *_eventpool;    //������һ��eventpoll

    //���е�ʱ������������ʱ��
    struct timeval _syn_time;
    vos::Timer  _timer;

    EventBaseSockMgr _sock_mgr;

	bool _run;
	vos::ThreadPool _thread_pool;
};

#endif /* EVENTBASE_H_ */
