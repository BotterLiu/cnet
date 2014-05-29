/******************************************************
*   FileName: udpclient.cpp
*     Author: liubo  2013-12-16 
*Description:
*******************************************************/


#include "sockutil.h"
#include <pthread.h>

using namespace std;
using namespace vos;

const char *send_msg = "NOOP";

void set_send_buff(int fd, int size)
{
	int snd_size = size;
	socklen_t optlen = sizeof(snd_size);

	int err = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &snd_size, &optlen);
	if (err < 0)
	{
		printf("获取发送缓冲区大小错误\n");
	}
	return;
}

void set_recv_buff(int fd, int size)
{
	int snd_size = size;
	socklen_t optlen = sizeof(snd_size);

	int err = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &snd_size, &optlen);
	if (err < 0)
	{
		printf("获取发送缓冲区大小错误\n");
	}
	return;
}


void *udp_client(void *arg)
{
    int fd = sock(vos::udp);
    if(fd < 0)
    {
    	printf("create sock fail \n");
    	return 0;
    }

    char data[5000];
    memset(data, sizeof(data), 0x31);

    int num = 10;
    while(num --)
    {
    	int len = sendto_sock(fd, "127.0.0.1", 8908, data, sizeof(data));
        printf("udp-client sent %d \n", len);
    }

    sleep(100);
}

void *udp_server(void*arg)
{
    int fd = sock(vos::udp);
    if(fd < 0)
    {
    	printf("create sock fail \n");
    	return 0;
    }


    bind_sock(fd, "127.0.0.1", 8908);
    set_recv_buff(fd, 2048);

    sleep(5);

    char buffer[4096] = {0};
    string ip;
    unsigned short port;
    while(1)
    {
        int ret = recvfrom_sock(fd, buffer, sizeof(buffer), ip, port);
        if(ret > 0)
        {
        	printf("udp-server recv : len = %d \n", ret);
        }
    }

    return 0;
}

int main()
{

    pthread_t t1, t2;

    pthread_create(&t1, NULL, udp_server, NULL);
    pthread_create(&t1, NULL, udp_client, NULL);

    while(1)
    {
    	sleep(10);
    }

    return 0;
}
