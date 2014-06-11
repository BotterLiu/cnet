/******************************************************
 *  CopyRight: Opensource
 *   FileName: sock_event.h
 *     Author: botter
 *Description: ����libevent�޸�ʵ��
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
#define EV_RPERSIST	(0x01 << 4) /* �Ƿ�ÿ�ζ�������¼�  */
#define EV_WPERSIST (0x01 << 5) /* д�¼��Ƿ�����Ե�     */

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

	//���ݷ��ͺ������ɾ���socket��ʵ�֣�����ֱ�ӷ���socket��Ҳ���Լ��뷢�ͻ�������У���write�¼�������
	virtual int send(const void *data, int len);

	//�첽���ӽӿ�
	virtual void asyn_connect(){return ;}

	virtual bool check_sockfd();

	virtual int type();

	virtual void release();

	//״̬��������, �ɾ����socketʵ��
	virtual int to_close() {return FSM_END;}
	virtual int to_read() {return FSM_END;}
	virtual int to_write() {return FSM_END;}

	virtual int event_process(int event);
	
	//bind local address's host and port;
	bool bind(const char *ip, unsigned short port);

public:   //���÷�����

	//����socket�����ͨ��Э��
	void set_trans_protocol(int type);
	//�������socket�ĳ�ʱʱ�䣬�����timeout��ʱ����û�����ݵ�����������ʱʱ��
	void set_timeout(int timeout);
	//�������socket��ҵ������
	void set_net_service(NetService *ns);
	//�������ĸ�event_base�г�
	void set_event_base(eventbase *eb);
    //���������ĸ�event_pool;
    void set_event_pool(eventpool *ep);
    //���õ�ַ����
	bool set_reuse_address(bool on);

    void set_last_active_time(time_t time){_last_active_time = time;}

    //���õ�ַ�����ص�ַ��
	bool set_address(const char *host, unsigned short port);
	//���ñ��ص�ַ
	void set_address(const struct sockaddr *addr);
	//���öԶ˵�ַ
	bool set_peer_addr(const char *host, unsigned short port);
	//���öԶ˵�ַ
    bool set_peer_addr(const struct sockaddr *addr);
    //��ȡsocket�ı��ص�ַ
    const char *get_local_host() {return _local_host;}
    //��ȡ����port
    unsigned short get_local_port() {return _local_port;}
    //��ȡ�Զ�host
    const char *get_peer_host() {return _peer_host;}
    //��ȡ�Զ�port
    unsigned short get_peer_port(){return _peer_port;}
    //����������־������ʱ����
    void set_reconn(bool need, int interval = 30);

    //get sockopt parameter
	bool get_int_option(int option, int &value);
    //set sockopt parameter
	bool set_int_option(int option, int value);
    //���ö�д�ȵĳ�ʱʱ��
	bool set_time_option(int option, int milliseconds);
    //set block or nonblock
	bool set_non_block(bool on);
    //get socket's error
	int get_soerror();
    //��ʱ��û�л��socket, ����Ƿ�ʱ
	bool check_timeout();
    //��ȡ��ǰsocket��״̬
	int get_state();
    //״̬��ѭ������
	int run_fsm(int event);

public:
	//���ص�_sockmgr����һ����������
	int _mount_queue;

	Socket *_next;
	Socket *_pre;

	int _fd;

	//socket ��״̬
	SOCKSTATE _state;

    //���ض˵�IP��Port
	struct sockaddr _local_addr;
    char _local_host[16];
    unsigned short _local_port;

    //�Զ�IP�� Port
	struct sockaddr _peer_addr;
	char _peer_host[16];
	unsigned short _peer_port;

	//�����ϲ����紦����
	NetService *_net_service;

	//�ϴλ��ʱ��
	time_t _last_active_time;
	time_t _timeout_time;
	//�೤ʱ����û�л��ɾ��
	int _timeout;

	bool _need_reconn;
	int _reconn_timeval;
    int _next_reconn_time;

	//������һ��eventbase����
	eventbase *_event_base;

	//������һ��eventpool����;
	eventpool *_event_pool;

	TransProtocol *_tprotocol;

	//�û����ݵ�����
	void *ptr;
};

TAILQ_HEAD (SOCK_LIST, Socket);


#endif /* SOCK_EVENT_H_ */
