/******************************************************
*   FileName: sockmgr.h
*     Author: botter  2013-9-19 
*Description:
*******************************************************/

#ifndef SOCKMGR_H_
#define SOCKMGR_H_

#include "tcp_socket.h"
#include "udp_socket.h"

class SockMgr
{
public:

    typedef TQueue<Socket> SockQueue;

	SockMgr();

	virtual ~SockMgr();

	Socket *get(int type);

	void recycle(Socket *socket);

	int size()
	{
		CAutoGuard g(_mutex);
		return _recycle_queue_size + _inuse_queue_size;
	}

	int inuse_size()
	{
		CAutoGuard g(_mutex);
		return _inuse_queue_size;
	}

	int recycle_size()
	{
		CAutoGuard g(_mutex);
		return _recycle_queue_size;
	}

private:
	Socket *sock_new(int type);

	void destroy();

private:
	int _mount_queue;
	int _inuse_queue_size;
	int _recycle_queue_size;
	TQueue<Socket> _recycle_queue[SOCKTYPE_NUM];
    vos::Mutex _mutex;
};

extern SockMgr __singleton_sockmgr;

#endif /* SOCKMGR_H_ */
