/******************************************************
 *   FileName: eventpool.cpp
 *     Author: Triones  2012-11-13
 *Description:
 *******************************************************/

#include "eventpool.h"
#include "sockevent.h"
#include "socket.h"
#include <stdlib.h>

eventpool::eventpool() : _run(false)
{
    _max_num = 2;
}

eventpool::~eventpool()
{
    stop();
    for(int i = 0; i < (int)_event_bases.size(); i++)
    {
    	delete _event_bases[i];
    	_event_bases[i] = NULL;
    }
}

bool eventpool::init(int eventbase_num)
{
	if(eventbase_num < 1)
		_max_num = 1;
	_max_num = eventbase_num;
	for(int i = 0; i < _max_num; i++)
	{
	    eventbase *base = new eventbase;
	    base->init(new EpollEvent, 1024);
        base->setid(i);
        base->set_eventpool(this);
        _event_bases.push_back(base);
	}

	return true;
}

bool eventpool::start()
{
	if (!_run)
	{
		_run = true;
		for (int i = 0; i < _max_num; i++)
		{
			_event_bases[i]->start();
		}
	}

    return true;
}

bool eventpool::stop()
{
	if (_run)
	{
		_run = false;
		for (int i = 0; i < _max_num; i++)
		{
			_event_bases[i]->stop();
		}
	}

    return true;
}

//怎么样分配sockevent event
eventbase *eventpool::select_eventbase(Socket *socket)
{
	//在1到max_num - 1 之间均匀分布。
    return _event_bases.size() > 1 ?
    		_event_bases[(socket->_fd % (_max_num - 1)) + 1] : _event_bases[0];
}

int eventpool::event_add(Socket * socket, bool readable, bool writeable)
{
	return select_eventbase(socket)->add_event(socket, readable, writeable);
}
