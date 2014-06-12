/******************************************************
 *   FileName: fileclient.h
 *     Author: Triones  2012-9-17 
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
    	data = NULL; len = 0;
    }

    ~FileData()
    {
    	if(data != NULL)
    	{
    		free(data);
    		data = NULL;
    	}
    	len = 0;
    }
};


class fileclient
{
public:
	fileclient(const char *ip, unsigned short port, const char *user_name,
			const char *password, const char *rootpath = NULL);

	virtual ~fileclient();

	//�������ļ����浽Զ�̣�
	int putfile(const char *local_file_name, const char *remote_file_name);
	//��Զ���ļ�Get������
	int getfile(const char *local_file, const char *remote_file);

	//������д�뵽Զ���ļ�
	int writefile(const char *data, int data_len, const char *remote_file);
	//��ȡԶ���ļ�������
	int readfile(const char *remote_file, FileData *file_data);

	//��������дsocket ERROR��ʱ����Զ�����disconnection�� ����û���д�ļ�ʧ�ܻ���������Ͽ����ӵ�����������ֶ�����disconnection.
	bool connection();
	void disconnection();
private:
    int connect_server();

    int send_tmout(int sockfd, const char *buffer, int buffer_len, int timeout);

    int recv_tmout(int sockfd, char *buffer, int buffer_len, int timeout);
private:
    struct sockaddr_in _serveraddr;
    char _user_name[20];
    char _password[20];

    bool _connected;
    int _sock_fd;
    int _seq;

    /* ���ñ���·�����������·��  */
    string _local_root;
};

#endif /* FILECLIENT_H_ */
