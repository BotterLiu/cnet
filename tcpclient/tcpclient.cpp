/******************************************************
*   FileName: tcpclient.cpp
*     Author: Triones  2013-8-31 
*Description:
*******************************************************/
#include "netop.h"
#include "eventpool.h"
#include "socket.h"
#include "sockmgr.h"
#include "log.h"
#include "vos_timer.h"
#include <sys/time.h>
#include "runinfo.h"
#include <sys/types.h>
#include <unistd.h>

static const char *__test_file = "./text.txt";

static const char *LOGIN_MSG = "LOGI SEND wasv6 wasv6 ";

static const char * __phone_number[] =
{
		"13000000111", "13000000112", "13000000113"
};

static int __phone_count = 3;

static const char *__server_ip = "127.0.0.1";
static unsigned short __server_port = 7006;
static int __send_speed = 20000;

static void get_test_data(const char *filename, vector<string> &vec)
{
	char buf[1024 * 4] = { 0 };
	FILE *fp = NULL;
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		return ;
	}

	while (fgets(buf, sizeof(buf), fp))
	{
		string line = buf;
		if(line.length() > 4 &&
				(line.substr(0, 5) == "CAITS" || line.substr(0, 5) == "CAITR"))
		{
            //去掉最后一个\n,
            line.assign(line, 0, line.length() - 1);
			line += " \r\n";
            vec.push_back(line);
		}
        memset(buf, 0, sizeof(buf));
	}

	fclose(fp);
	fp = NULL;

	return ;
}

static string get_test_data()
{
    static int phone_index = 0;

    if((++phone_index) == __phone_count - 1)
    {
    	phone_index = 0;
    }

    string gpsinfo = "CAITS 0_0 4C54_";
    gpsinfo += __phone_number[phone_index];
    gpsinfo += " 0 U_REPT {TYPE:0,120:423,124:2,121:0,122:0,123:1,1:64484545,2:16254365,20:0,3:0,4:20130815/014028,5:0,6:1069,8:819200,999:1376502052}";

    return gpsinfo;
}

static void function_test(Socket *socket)
{
    vector<string> vec;
    get_test_data(__test_file, vec);

    for(int i = 0; i < (int)vec.size(); i++)
    {
    	socket->send((const void*)vec[i].c_str(), vec[i].length());
    }
}

static vos::Timer __vos_timer;
//单独开辟出一个线程，或者是写一个定时器。 暂时先单独启动一个定时器吧
static void efficient_test(Socket *socket, int num)
{
    while(num--)
    {
    	string gpsinfo = get_test_data();
    	socket->send((const void*)gpsinfo.c_str(), (int)gpsinfo.length());
    }
}

struct EfficientArg
{
    Socket *sock;
    int num;
};

void efficient_fun(void *arg)
{
	EfficientArg *effarg = (EfficientArg*)arg;
	efficient_test(effarg->sock, effarg->num);
}

void display_fun(void *arg)
{
    __net_runinfo.show();
}

class TcpClient: public NetService
{
public:

	TcpClient(){}

	virtual int on_connect(Socket *socket);

	virtual int on_close(Socket *socket);

	virtual int handle_packet(Socket *socket, const char *data, int len);

	bool start(const char *ip, unsigned short port);

private:
	eventpool _event_pool;
};

int TcpClient::on_connect(Socket *socket)
{
    printf("on _connect -----------\n");
    printf("%u \n\n", getpid());
    socket->send((const void*)LOGIN_MSG, strlen(LOGIN_MSG));
    return FSM_END;
}

int TcpClient::on_close(Socket *socket)
{
    dlog1("socket close local addr %s %d peer addr %s %d", socket->_local_host, socket->_local_port,
    		socket->_peer_host, socket->_peer_port);

    return FSM_END;
}

int TcpClient::handle_packet(Socket *socket, const char *data, int len)
{
	const char *ptr = ( const char *) data ;

    if ( strncmp( ptr, "LACK", 4 ) == 0 )
	{
		int ret = atoi( ptr+5 ) ;
		switch(ret)
		{
		case 0:
		case 1:
		case 2:
			{
				dlog1("login check success, up-link ON_LINE");
                socket->send((const void*)"START \r\n", 8 );

                int interval = 1, num = 1;
                if(__send_speed < 1000)
                {
                	interval = 1000 / __send_speed;
                }
                else
                {
                    num = __send_speed / 1000;
                }

                EfficientArg *arg = new EfficientArg;
                arg->sock = socket;
                arg->num = num;
                printf("num = %d, interval = %d \n", num, interval);
                __vos_timer.timer_add(interval, efficient_fun, arg, ABSOLUTE_TIMER, 0);
                //每隔5s输出一次
                __vos_timer.timer_add(5000, display_fun, arg, ABSOLUTE_TIMER, 0);
			}
			break;
		case -1:
			dlog1("login check fail,password error");
			break;
		case -2:
			dlog1("login check fail,user already login");
			break;
		case -3:
			dlog1("login check fail,account disable" );
			break;
		case -4:
			dlog1("login check fail, username is invalid");
			break;
		default:
			dlog1("login check fail,other error,close it");
			break;
		}
	}
	else
	{
		dlog1("recv %.*s \n", len, data);
	}

    return FSM_END;
}

bool TcpClient::start(const char *ip, unsigned short port)
{
	int num = 1;

	_event_pool.init(1);
	_event_pool.start();

	while (num--)
	{
        TcpSocket *tcpsock = (TcpSocket*)__singleton_sockmgr.get(TCPSOCK);
		tcpsock->set_net_service(this);
		tcpsock->set_timeout(30);
        tcpsock->set_reconn(true, 5);
		tcpsock->set_trans_protocol(TEXT_TPROTOCOL);
		tcpsock->set_peer_addr(ip, port);
        tcpsock->set_event_pool(&_event_pool);
		tcpsock->asyn_connect();
	}

	return true;
}

int main(int argc, char *argv[])
{
	INIT_LOG(NULL, LOG_CONSOLE, "test.log", 1);

	if(argc >= 4)
	{
        __server_ip = argv[1];
        __server_port = atoi(argv[2]);
        __send_speed = atoi(argv[3]);
	}

	TcpClient tcp_client;
    if( ! tcp_client.start(__server_ip, __server_port))
    {
    	return false;
    }

    while(true)
    {
//    	usleep(10);
    	__vos_timer.timer_process();
    }

    return 0;
}

