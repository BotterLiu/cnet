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
	//����������������ӳɹ�
	void on_connected(Socket *socket)
	{
        socket->_last_active_time = time(0);
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
			if (head->_last_active_time + timeout >= now)
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

	vector<Socket *> get_waiting_close()
	{
        vector<Socket *> vec;
		Socket *temp = NULL;

		while ((temp = _waiting_queue.begin()) != NULL)
		{
			//�ӵ�ǰeventbase�Ĺ�����ɾ��
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
	//�������������Socket
	set<Socket *> _sockets;
	//���رյ�Socket���������ֶ��У�һ��Socketֻ�ܴ������е�һ�����С�
	TQueue<Socket> _waiting_queue;
	//�������Ķ���
	TQueue<Socket> _reconn_queue;
	//��Ҫ�ǳ�ʱ���ʹ�õ�
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

