/******************************************************
*   FileName: header.h
*     Author: botter  2012-9-18 
*Description:
*******************************************************/

#ifndef HEADER_H_
#define HEADER_H_

/*
 * interheader.h
 *
 *  Created on: 2012-4-26
 *      Author: think
 *  这里主要想实现通用的协议拆分所以就定义了通用协议头，其中type为协议大类主要区分不同格式的协议的
 */

#ifndef __INTERHEADER_H__
#define __INTERHEADER_H__

#pragma pack(1)

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

////////////////////////////////// 文件管理  ///////////////////////////////////////
/**
 *  这里主要实现一个网络文件服务管理，主要实现类似本地文件存储方式的管理，
 *  实现统一的open、write、read、close四个接口来实现文件存储读写管理
 */
// 使用两个字符的版本号管理
#define BIG_VER_0    		0x01    	// 0字符值
#define BIG_VER_1	 		0x01    	// 1字符值
#define BIG_ERR_SUCCESS   	0x00		// 成功
#define BIG_ERR_FAILED		0x01		// 失败

// 文件管理命令码管理
#define BIG_OPEN_REQ  		0x0001  	// 登陆请求
#define BIG_OPEN_RSP  		0x8001  	// 登陆响应
#define BIG_WRITE_REQ  		0x0002  	// 写文件请求
#define BIG_WRITE_RSP  		0x8002  	// 写文件成功应答
#define BIG_READ_REQ		0x0003  	// 读文件请求
#define BIG_READ_RSP   		0x8003  	// 读文件应答
#define BIG_CLOSE_REQ  		0x0004 		// 退出登陆
#define BIG_CLOSE_RSP  		0x8004 		// 退出登陆成功应答

#define CTFO_TAG            0x1adf5fed

struct bigheader
{
	unsigned char  	 ver[2] ;  // 版本号管理
	unsigned int   	 seq ;	   // 统一数据序号
	unsigned short   cmd ;     // 命令码管理

	bigheader()
	{
		ver[0] = BIG_VER_0 ;
		ver[1] = BIG_VER_1 ;
		seq    = 0 ;
		cmd    = 0 ;
	}
};

struct bigend
{
    unsigned int ctfotag;
    bigend()
    {
    	ctfotag = CTFO_TAG;
    }
};

// 登陆请求
struct bigloginreq
{
	unsigned char user[20] ;
	unsigned char pwd[20] ;
};

struct bigloginreqall
{
	bigheader header;
	bigloginreq req;
	bigloginreqall()
	{

	}
	bigloginreqall(char user[20], char password[20], int seq = 0)
	{
        header.cmd = htons(BIG_OPEN_REQ);
        header.seq = htonl(seq);
        strncpy((char*)req.user, user, 20);
        strncpy((char*)req.pwd, password, 20);
	}
};

// 登陆响应
struct bigloginrsp
{
	unsigned char result ;
};

struct bigloginrspall
{
	bigheader header;
	bigloginrsp resp;
	bigloginrspall(unsigned char result = 0, int seq = 0)
	{
        header.cmd = htons(BIG_OPEN_RSP);
        header.seq = htonl(seq);
        resp.result = result;
	}
};

// 写数据请求
struct bigwritereq
{
	unsigned char path[256] ;
	unsigned int  data_len ;
	// 数据体内容
};

struct bigwritereqall
{
	bigheader header;
	bigwritereq write_req;

	bigwritereqall(){}
	bigwritereqall(string writefile, int data_len, int seq = 0)
	{
        header.cmd = htons(BIG_WRITE_REQ);
        header.seq = htonl(seq);
        write_req.data_len = htonl(data_len);
        memset(write_req.path, 0, sizeof(write_req.path));
        strncpy((char*)write_req.path, writefile.c_str(), 256 - 1);
	}
	// 数据体内容
};

// 写数据响应
struct bigwritersp
{
	unsigned char result ;
};

struct bigwriterspall
{
	bigheader header;
	bigwritersp resp;
	bigwriterspall(){}
	bigwriterspall(unsigned char result, int seq = 0)
	{
        header.cmd = htons(BIG_WRITE_RSP);
        header.seq = htonl(seq);
        resp.result = result;
	}
};

// 读数据请求
struct bigreadreq
{
	unsigned char path[256] ;
};

struct bigreadreqall
{
	bigheader header;
	bigreadreq readreq;
	bigreadreqall(){}
	bigreadreqall(string path, int seq = 0)
	{
        header.cmd = htons(BIG_READ_REQ);
        header.seq = htonl(seq);
        memset(readreq.path, 0, sizeof(readreq.path));
        strncpy((char*)readreq.path, path.c_str(), sizeof(readreq.path) -1 );
	}
};

// 读数据响应
struct bigreadrsp
{
	unsigned char result ;
	unsigned int  data_len ;


	// 响应数据内容
};

struct bigreadrspall
{
	bigheader header;
	bigreadrsp rsp;

	bigreadrspall(){}
	bigreadrspall(unsigned char ret, unsigned len, int seq = 0)
	{
        header.cmd = htons(BIG_READ_RSP);
        header.seq = htonl(seq);
        rsp.result = ret;
        rsp.data_len = htonl(len);
	}
};

#pragma pack()

#endif /* INTERHEADER_H_ */

#endif /* HEADER_H_ */
