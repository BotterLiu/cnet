/******************************************************
*  CopyRight: 北京中交兴路科技有限公司(2012-2015)
*   FileName: netop.h
*     Author: liubo  2012-11-13 
*Description:
*******************************************************/

#ifndef __NETOP_H___
#define __NETOP_H___

#include "socket.h"

class Socket;

class NetService
{
public:
	virtual ~NetService() {};
	virtual int on_connect(Socket *socket) = 0;
	virtual int on_close(Socket *socket) = 0;
	virtual int handle_packet(Socket *socket, const char *data, int len) = 0;
};

class CreateClientSock
{
public:
	virtual ~CreateClientSock() {};
    virtual Socket *create_client_sock() = 0;
};


#endif
