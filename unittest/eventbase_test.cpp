/******************************************************
*   FileName: eventbase_test.cpp
*     Author: liubo  2013-10-16 
*Description:
*******************************************************/
#include "socket.h"
#include "log.h"

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
		socket->_last_active_time = time(0);
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
        socket->_last_active_time = time(0);
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
            socket->_last_active_time = time(0);
            _online_queue.erase(socket);
            _online_queue.push(socket);
        }
		dlog2("WARN: EventBaseSockMgr::on_active");
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
			if (head->_last_active_time + timeout >= now)
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

	vector<Socket *> get_waiting_close()
	{
        vector<Socket *> vec;
		Socket *temp = NULL;

		while ((temp = _waiting_queue.begin()) != NULL)
		{
			//从当前eventbase的管理中删除
			_sockets.erase(temp);
			temp = _online_queue.pop();
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

int main()
{
	Socket *socket = new Socket;
    Socket *socket1 = new Socket;
    Socket *socket2 = new Socket;
    Socket *socket3 = new Socket;

    EventBaseSockMgr sockmgr;

    sockmgr.on_connecting(socket);
    sockmgr.on_connected(socket);
    sockmgr.on_new_connection(socket);
    sockmgr.on_active(socket);

    Socket *sock = NULL;

    socket1->_next_reconn_time = time(0);
    socket2->_next_reconn_time = time(0) + 1;
    socket3->_next_reconn_time = time(0) + 2;

    sockmgr.add_reconn(socket2);
    sockmgr.add_reconn(socket1);
    sockmgr.add_reconn(socket3);
//
    sock = sockmgr._reconn_queue.begin();
    for(; sock != NULL; sock = sockmgr._reconn_queue.next(sock))
    {
    	printf("sock next_reconn_time = %d \n", sock->_next_reconn_time);
    }

//    sleep(1);
    vector<Socket *> vec = sockmgr.get_reconn(time(0));
    for(int i = 0; i < vec.size(); i++)
    {
    	printf("reconn %d \n", vec[i]->_next_reconn_time);
        vec[i]->_last_active_time = time(0);
        sockmgr.on_connecting(vec[i]);
    }

    vec = sockmgr.get_timeout(time(0), 1);
    for(int i = 0; i < vec.size(); i++)
    {
    	printf("timeout %d \n", vec[i]->_next_reconn_time);
    }
//
//    printf("--------------------------------------------\n");

    return 0;
}

