/******************************************************
*  CopyRight: botter (botter.liu@163.com) Version 0.l
*   FileName: net.h
*     Author: botter  2012-2-5 
*Description:
*******************************************************/

#ifndef ___SOCK_H_
#define ___SOCK_H_

#include "vos.h"
#include <string>


#ifdef __WINDOWS
#include <winsock2.h>
/***
 * ws2_32.lib�ļ����ṩ�˶������������API��֧�֣���ʹ�����е�API����Ӧ�ý�ws2_32.lib���빤�̣�������Ҫ��̬����ws2_32.dll����
 */
#pragma comment(lib,"WS2_32.LIB")
#pragma warning(disable:4996)
#pragma warning(disable:4786)

#else
#include <netinet/in.h>
#endif

using namespace std;

namespace vos
{

#define MAX_FD_OFFSET 0xffff

enum socktype{tcp, udp};

extern bool set_resource_limit(int limit =  MAX_FD_OFFSET );

extern void close(int fd);

extern char *get_ip_str(unsigned int ip);

extern int set_reuse_addr( int sockfd);

extern int init_start_up();

extern int sock(socktype type);

/* �ڲ��ӿڣ��ɵ����߱�֤�����������ȷ�ԡ�*/
extern int bind_sock(int sock_fd, const char* ip, unsigned short port);

extern int listen_sock(int sock_fd, int backlog);

extern int set_block(int sock_fd);

extern int set_non_block(int sock_fd);

/* *************************************************
 * SO_KEEPALIVE  : �Ƿ������ʱ����1Ϊ������
 * TCP_KEEPIDLE  : ��һ�����ӽ�����Ч��̽��֮ǰ���е����ǻ�Ծʱ�������೤ʱ��û�л�󣬷�����·̽Ѱ��Ϣ)
 * TCP_KEEPINTVL : ������·̽Ѱ��Ϣ��ʱ����
 * TCP_KEEPCNT   : ������·̽Ѱʱ��Ĵ�����
 * �� 5sΪ��λ��������·̽Ѱ����Ϣ�ļ��5s�� timeout��ȡֵ��Χ���� > 10��
 * ֻ����һ����ʱʱ�䣬��η���time_outʱ���Ǹ����⣺
 * opt_keepidle:��СֵΪ5���Ժ�Ҳ��ÿ��5����һ�Ρ�
 * ************************************************/
extern int set_sock_alive(int sock_fd, int time_out);


/* *************************************************
 * output : conn_fd��������ӳɹ����fd)
 * return : success (OK) error(errcode)
 * comment: Ĭ�ϵĳ�ʱʱ���� 1s
 * ************************************************/
extern int non_block_conn(int *conn_fd, const char *ip, unsigned short port);

/* *************************************************
 * output :
 * return :
 * comment:�򵥵����ݶ�ȡ������select�����ݵȴ�������ȡ����Ϊbuffer_len,����
 * ��һ��û�ж�����û�ж�����ʱ������������Ǻ���ò�Ʒ����ļ��ʺ�С��
 * �˳�������
 * ��1������������
 * ��2������ϣ��������ݺ�Ȼ���ٶ���EAGAIN)
 * (3)��buffer���ˡ�
 * ************************************************/
extern int read_sock(int sockfd, char *buffer, int buffer_len, int *read_len);

/* *************************************************
 * output : read_len������д�ɹ������ݳ��ȡ�
 * return :
 * comment: ��epoll�ڵײ�֧�ţ�sockfd����ͳһ��epoll�����У��ǲ��ò���timeout�ķ�ʽ�ġ�
 * �˳�������
 * (1)���������󡣷��ش����롣
 * (2)�������ݣ�������һֱ����buffer����)������OK
 * (3)�ȴ����ݵ���ʱ�䳬ʱ��
 * ************************************************/

extern int write_sock(int sockfd, const char *buffer, int buffer_len, int *send_len);

extern int recv_tmout(int sockfd, char *buffer, int buffer_len, int timeout);

extern int send_tmout(int sockfd, const char *buffer, int buffer_len, int timeout);

extern int recvfrom_sock(int sockfd, char *buffer, int len, string &ip, unsigned short &port);

extern int sendto_sock(int sockfd, const char *ip, unsigned short port, const char *buffer, int buffer_len);

extern int accept_sock(int sockfd, int *client_sock, struct sockaddr_in *sock_addr,
	int *sock_addr_len);

}

/*static int g_epoll_handle = 0;*/
#endif /* NET_H_ */
