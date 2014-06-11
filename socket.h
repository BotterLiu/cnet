/******************************************************
 *  CopyRight: Opensource
 *   FileName: sock_event.h
 *     Author: botter
 *Description: 根据libevent修改实现
 *******************************************************/

#ifndef SOCK_EVENT_H_
#define SOCK_EVENT_H_

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/queue.h>
#include <netinet/in.h>

#include "sock_buffer.h"
#include "vos_timer.h"
#include "vos_lock.h"
#include "tprotocol.h"
#include "tqueue.h"
#include <map>
#include <set>

using namespace std;
using namespace vos;

class eventbase;
class eventpool;
class SocketEvent;
class netop;
class appop;

#define EV_TIMEOUT	0x01
#define EV_READ		(0x01 << 1)
#define EV_WRITE	(0x01 << 2)
#define EV_SIGNAL	(0x01 << 3)
#define EV_RPERSIST	(0x01 << 4) /* 是否每次都加入读事件  */
#define EV_WPERSIST (0x01 << 5) /* 写事件是否持续性的     */

#define FSM_END     (-1)
#define FSM_TOCLOSE (-2)

#define DEFAUT_READ_BUFFER  (10 * 1024)

enum SOCKTYPE
{
    TCPLISTEN = 0, TCPSOCK, UDPSOCK, SOCKTYPE_NUM
};

enum SOCKSTATE
{
	CLOSED, CONNECTING, ESTABLISHED, LISTEN, SOCKSTATE_NUM
};

enum SOCKEVENT
{
	TOCLOSE, TOREAD, TOWRITE, SOCKEVENT_NUM
};

class Socket;
class TcpSocket;
class TcpListenSocket;
class UdpSocket;

class SockEventHandler
{
public:
	virtual ~SockEventHandler() {};
	virtual int to_close() = 0;
	virtual int to_read() = 0;
	virtual int to_write() = 0;
};

typedef int (SockEventHandler::*EventFun)();

struct SockEventFsm
{
    SOCKSTATE next_state;
    EventFun  next_handler;

    void set(SOCKSTATE n, EventFun h)
    {
    	next_state = n;
    	next_handler = h;
    }
};

class NetService;

class Socket : public SockEventHandler
{
public:
	Socket();
	Socket(int sock, int state, struct sockaddr_in addr);

	virtual ~Socket();

	//数据发送函数，由具体socket类实现，可以直接发送socket，也可以加入发送缓冲队列中，由write事件触发。
	virtual int send(const void *data, int len);

	//异步连接接口
	virtual void asyn_connect(){return ;}

	virtual bool check_sockfd();

	virtual int type();

	virtual void release();

	//状态机处理函数, 由具体的socket实现
	virtual int to_close() {return FSM_END;}
	virtual int to_read() {return FSM_END;}
	virtual int to_write() {return FSM_END;}

	virtual int event_process(int event);
	
	//bind local address's host and port;
	bool bind(const char *ip, unsigned short port);

public:   //公用方法类

	//设置socket传输的通信协议
	void set_trans_protocol(int type);
	//设置这个socket的超时时间，如果在timeout的时间内没有数据到来，触发超时时间
	void set_timeout(int timeout);
	//设置这个socket的业务处理函数
	void set_net_service(NetService *ns);
	//设置在哪个event_base中出
	void set_event_base(eventbase *eb);
    //设置属于哪个event_pool;
    void set_event_pool(eventpool *ep);
    //设置地址重用
	bool set_reuse_address(bool on);

    void set_last_active_time(time_t time){_last_active_time = time;}

    //设置地址，本地地址。
	bool set_address(const char *host, unsigned short port);
	//设置本地地址
	void set_address(const struct sockaddr *addr);
	//设置对端地址
	bool set_peer_addr(const char *host, unsigned short port);
	//设置对端地址
    bool set_peer_addr(const struct sockaddr *addr);
    //获取socket的本地地址
    const char *get_local_host() {return _local_host;}
    //获取本端port
    unsigned short get_local_port() {return _local_port;}
    //获取对端host
    const char *get_peer_host() {return _peer_host;}
    //获取对端port
    unsigned short get_peer_port(){return _peer_port;}
    //设置重连标志和重连时间间隔
    void set_reconn(bool need, int interval = 30);

    //get sockopt parameter
	bool get_int_option(int option, int &value);
    //set sockopt parameter
	bool set_int_option(int option, int value);
    //设置读写等的超时时间
	bool set_time_option(int option, int milliseconds);
    //set block or nonblock
	bool set_non_block(bool on);
    //get socket's error
	int get_soerror();
    //长时间没有活动的socket, 检查是否超时
	bool check_timeout();
    //获取当前socket的状态
	int get_state();
    //状态机循环函数
	int run_fsm(int event);

public:
	//挂载到_sockmgr的那一个链表下面
	int _mount_queue;

	Socket *_next;
	Socket *_pre;

	int _fd;

	//socket 的状态
	SOCKSTATE _state;

    //本地端的IP、Port
	struct sockaddr _local_addr;
    char _local_host[16];
    unsigned short _local_port;

    //对端IP、 Port
	struct sockaddr _peer_addr;
	char _peer_host[16];
	unsigned short _peer_port;

	//设置上层网络处理函数
	NetService *_net_service;

	//上次活动的时间
	time_t _last_active_time;
	time_t _timeout_time;
	//多长时间内没有活动就删掉
	int _timeout;

	bool _need_reconn;
	int _reconn_timeval;
    int _next_reconn_time;

	//属于哪一个eventbase管理
	eventbase *_event_base;

	//属于哪一个eventpool管理;
	eventpool *_event_pool;

	TransProtocol *_tprotocol;

	//用户数据的连接
	void *ptr;
};

TAILQ_HEAD (SOCK_LIST, Socket);


#endif /* SOCK_EVENT_H_ */
