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
 * ws2_32.lib文件，提供了对以下网络相关API的支持，若使用其中的API，则应该将ws2_32.lib加入工程（否则需要动态载入ws2_32.dll）。
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

/* 内部接口，由调用者保证输入参数的正确性。*/
extern int bind_sock(int sock_fd, const char* ip, unsigned short port);

extern int listen_sock(int sock_fd, int backlog);

extern int set_block(int sock_fd);

extern int set_non_block(int sock_fd);

/* *************************************************
 * SO_KEEPALIVE  : 是否开启保活定时器，1为开启。
 * TCP_KEEPIDLE  : 对一个连接进行有效性探测之前运行的最大非活跃时间间隔（多长时间没有活动后，发送链路探寻消息)
 * TCP_KEEPINTVL : 发送链路探寻消息的时间间隔
 * TCP_KEEPCNT   : 发送链路探寻时间的次数。
 * 以 5s为单位，发送链路探寻的消息的间隔5s。 timeout的取值范围大于 > 10。
 * 只给出一个超时时间，如何分配time_out时间是个问题：
 * opt_keepidle:最小值为5；以后也是每隔5发送一次。
 * ************************************************/
extern int set_sock_alive(int sock_fd, int time_out);


/* *************************************************
 * output : conn_fd（存放连接成功后的fd)
 * return : success (OK) error(errcode)
 * comment: 默认的超时时间是 1s
 * ************************************************/
extern int non_block_conn(int *conn_fd, const char *ip, unsigned short port);

/* *************************************************
 * output :
 * return :
 * comment:简单的数据读取，不做select的数据等待，最大读取长度为buffer_len,可以
 * 第一次没有读完且没有读满的时候继续读，但是好像貌似发生的几率很小。
 * 退出条件：
 * （1）发生读错误
 * （2）读完毕（读到数据后，然后再读到EAGAIN)
 * (3)读buffer满了。
 * ************************************************/
extern int read_sock(int sockfd, char *buffer, int buffer_len, int *read_len);

/* *************************************************
 * output : read_len，返回写成功的数据长度。
 * return :
 * comment: 在epoll在底层支撑，sockfd处于统一的epoll管理中，是不用采用timeout的方式的。
 * 退出条件：
 * (1)发生读错误。返回错误码。
 * (2)读到数据（而不是一直读到buffer放满)，返回OK
 * (3)等待数据到来时间超时。
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
