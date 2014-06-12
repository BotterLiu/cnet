/******************************************************
 *   FileName: sockmgr.cpp
 *     Author: Triones  2013-9-19
 *Description:
 *******************************************************/

#include "sockmgr.h"

SockMgr __singleton_sockmgr;

enum MOUNT_QUEUE_TYPE{INUSE, RECYCLE, MOUNT_QUEUE_TYPE_TYPE_NUM};

SockMgr::SockMgr()
{
	_recycle_queue_size = 0;
	_inuse_queue_size = 0;
}

SockMgr::~SockMgr()
{
	destroy();
}

Socket *SockMgr::get(int type)
{
	CAutoGuard g(_mutex);

	Socket *socket = NULL;

	if(_recycle_queue[type].size() == 0)
	{
		socket = sock_new(type);
	}
	else
	{
		socket = _recycle_queue[type].pop();
		_recycle_queue_size--;
	}

	_inuse_queue_size++;
    socket->_mount_queue = INUSE;

	return socket;
}

void SockMgr::recycle(Socket *socket)
{
	CAutoGuard g(_mutex);

    if(socket->_mount_queue != RECYCLE)
    	return;
    _recycle_queue[socket->type()].push(socket);
    _recycle_queue_size++;

    return;
}

Socket *SockMgr::sock_new(int type)
{
	switch (type)
	{
	case TCPLISTEN:
		return new TcpListenSocket;
//	case UDPSOCK:
//		return new UdpSocket;
	case TCPSOCK:
        return new TcpSocket;
	default:
		return new TcpSocket;
	}
}

void SockMgr::destroy()
{
	CAutoGuard g(_mutex);

    Socket *socket = NULL;

    for(int i = 0; i < SOCKTYPE_NUM; i++)
    {
    	_recycle_queue[i].clear();
    }
}
