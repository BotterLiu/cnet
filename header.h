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
 *  ������Ҫ��ʵ��ͨ�õ�Э�������ԾͶ�����ͨ��Э��ͷ������typeΪЭ�������Ҫ���ֲ�ͬ��ʽ��Э���
 */

#ifndef __INTERHEADER_H__
#define __INTERHEADER_H__

#pragma pack(1)

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

////////////////////////////////// �ļ�����  ///////////////////////////////////////
/**
 *  ������Ҫʵ��һ�������ļ����������Ҫʵ�����Ʊ����ļ��洢��ʽ�Ĺ���
 *  ʵ��ͳһ��open��write��read��close�ĸ��ӿ���ʵ���ļ��洢��д����
 */
// ʹ�������ַ��İ汾�Ź���
#define BIG_VER_0    		0x01    	// 0�ַ�ֵ
#define BIG_VER_1	 		0x01    	// 1�ַ�ֵ
#define BIG_ERR_SUCCESS   	0x00		// �ɹ�
#define BIG_ERR_FAILED		0x01		// ʧ��

// �ļ��������������
#define BIG_OPEN_REQ  		0x0001  	// ��½����
#define BIG_OPEN_RSP  		0x8001  	// ��½��Ӧ
#define BIG_WRITE_REQ  		0x0002  	// д�ļ�����
#define BIG_WRITE_RSP  		0x8002  	// д�ļ��ɹ�Ӧ��
#define BIG_READ_REQ		0x0003  	// ���ļ�����
#define BIG_READ_RSP   		0x8003  	// ���ļ�Ӧ��
#define BIG_CLOSE_REQ  		0x0004 		// �˳���½
#define BIG_CLOSE_RSP  		0x8004 		// �˳���½�ɹ�Ӧ��

#define CTFO_TAG            0x1adf5fed

struct bigheader
{
	unsigned char  	 ver[2] ;  // �汾�Ź���
	unsigned int   	 seq ;	   // ͳһ�������
	unsigned short   cmd ;     // ���������

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

// ��½����
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

// ��½��Ӧ
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

// д��������
struct bigwritereq
{
	unsigned char path[256] ;
	unsigned int  data_len ;
	// ����������
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
	// ����������
};

// д������Ӧ
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

// ����������
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

// ��������Ӧ
struct bigreadrsp
{
	unsigned char result ;
	unsigned int  data_len ;


	// ��Ӧ��������
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
