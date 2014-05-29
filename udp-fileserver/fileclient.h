/******************************************************
 *  CopyRight: �����н���·�Ƽ����޹�˾(2012-2015)
 *   FileName: fileclient.h
 *     Author: liubo  2012-9-17 
 *Description: �������ͻ��ˣ���Ҫ��������Ŀ⡣
 (1) ���紫�䣺
 ���ݷ��ͺͶ�ȡ���ó��Է��ͣ��������ʧ�ܣ� socket��������ʱ������select/epoll�� IO���ü�⡣
 ͬ�������ӿڣ����ݻỰͨ����ʱʱ����ȷ������ֹ�û��߳�������
 ��2������������������
 ������������������û�����ӵ�����²����ᷢ�������� ���������Ŀ����Ժ�С��
 ���ڷ���˽������ݻ��������µ�д��������ʵ������Ӧ�ò㣬ÿ����һ��ָ�����Ҫ�õ�һ��ָ��ظ������ܽ�����һ���Ĳ��������������Ƶ����á�
 ����˽��յ����ݺ�û�и�����Ӧ��
 ��3���û��ӿڣ�
 δ��¶�������ӽӿڣ����ݲ���ʱ�������������δ���ӣ��Զ��������ӡ��û�ֻ�豣���ļ����ͻ�ȡ�ļ����ݼ��ɡ�
 ��¶���û��Ĵ����壺
 ��·���󡢲��������ļ���������
 *******************************************************/

#ifndef FILECLIENT_H_
#define FILECLIENT_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <stdlib.h>
#include "netop.h"
#include "eventpool.h"
#include "socket.h"

using namespace std;

#define FILE_SUCCESS             ( 0)
#define FILE_FAIL                (-1)

#define FILE_PARAM_ERROR         (-2)

#define SOCK_REMOTE_CLOSE        (-6)
#define SOCK_ERROR               (-7)
#define SOCK_TIMEOUT_ERROR       (-8)
#define SOCK_CONNECT_ERROR       (-9)

struct FileData
{
	char *data;
	int len;

	FileData()
	{
		data = NULL;
		len = 0;
	}

	~FileData()
	{
		if (data != NULL)
		{
			free(data);
			data = NULL;
		}
		len = 0;
	}
};

class FileClient;

enum USER_STATAE{OFF_LINE, ON_LINE};

class User
{
public:
	User(): _socket(NULL), _state(OFF_LINE){}

	Socket *_socket;
    int _state;
};

class FileClient : public NetService
{
public:

	virtual int on_connect(Socket *sock_event);

	virtual int on_close(Socket *sock_event);

	virtual int on_read(Socket *sock_event);

    FileClient(eventpool &pool) : _event_pool(pool){}

    bool start(const char *ip, unsigned short port);

    int put_file(const char *local_file, const char *remote_file);

	//������д�뵽Զ���ļ�
	int writefile(const char *data, int data_len, const char *remote_file);

private:
    User _user;
	eventpool &_event_pool;
};


#endif /* FILECLIENT_H_ */
