/******************************************************
*  CopyRight: 北京中交兴路科技有限公司(2012-2015)
*   FileName: netop.cpp
*     Author: liubo  2012-11-13 
*Description:
*******************************************************/

#include "netop.h"
#include "log.h"
#include "sockutil.h"
#include "socket.h"
#include <errno.h>
#include "eventbase.h"
#include "eventpool.h"
#include "header.h"

int NetService::on_connect(Socket *socket)
{
    return FSM_END;
}

int NetService::on_close(Socket *socket)
{
    return FSM_END;
}

int NetService:: handle_packet(Socket *socket, const char *data, int len)
{
    return FSM_END;
}


