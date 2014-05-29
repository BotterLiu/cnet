/******************************************************
 *   FileName: tprotocol.h
 *     Author: liubo  2013-9-5 
 *Description:
 *******************************************************/

#ifndef TPROTOCOL_H_
#define TPROTOCOL_H_

#include <string.h>
#include <stdio.h>
#include <vector>
#include "vos_lock.h"

#define DEFAULT_PACKET_MAX_SIZE (1024 * 1024)
#define TEXT_PACKET_MAX_SIZE    (1024 * 10)

/***********************************
 * client 使用流程：
 * （1）设置Transport protocol.
 * (2)send data, TransPortprotocol 编码放到send_packqueue 中，由epoll的write驱动发送。
 * (3)recv data, TransPortPortocol 解码放到read_packqueue 中，
 *
 * 采用这种方式，总体上将数据多了一次拷贝的过程。read 出来后，由split放到
 *
 * 内存拷贝的速率测试。
 ***********************************/

enum TRANS_PROTOCOL_TYPE{DEFAULT_TPROTOCOL, TEXT_TPROTOCOL, CTFO_TPROTOCOL, TPROTOCOL_NUM};

using namespace std;

class PacketQueue;
class TransProtocol;

extern  TransProtocol * tprotocol_new(int TYPE);

class Packet
{
public:
	Packet();

	void copy(const char *data, int len);

	virtual ~Packet();

	virtual char *encode_data();
	virtual int encode_length();

	virtual char *decode_data();
	virtual int decode_length();

	void free();
	int64_t expire_time();
public:
	Packet *_next;
    char *_data;
    int _len;
	int _expire_time;
};

class TextPacket: public Packet
{
public:
	virtual char *decode_data();
	virtual int decode_length();
};

class TransProtocol
{
public:
	TransProtocol(){}
	virtual ~TransProtocol(){}
    //encode出一个包来
	virtual Packet *encode_pack(const char *data, int len);
    //解析出一个包来
    virtual Packet *decode_pack(const char *data, int len, int &decode_len);
	//data, len, 需要编码的数据，pack 编码后的结果。
	bool encode(const char *data, int len, PacketQueue *pack_queue);
    //socket 中读取到的数据，解析成的pakcet push到pack_queue中。
	int decode(const char *data, int len, PacketQueue *pack_queue);
};

class TextTransProtocol : public TransProtocol
{
public:
    //encode出一个包来
	virtual Packet *encode_pack(const char *data, int len);
    //解析出一个包来
    virtual Packet *decode_pack(const char *data, int len, int &decode_len);
};

class CtfoPacket: public Packet
{
public:
	virtual char *decode_data();
	virtual int decode_length();
};

class CtfoTransProtocol: public TransProtocol
{
public:
	//encode出一个包来
	virtual Packet *encode_pack(const char *data, int len);
	//解析出一个包来
	virtual Packet *decode_pack(const char *data, int len, int &decode_len);
};


//相当于静态存储
class TransProtocolFac
{
public:
	TransProtocolFac()
	{
		//DEFAULT_TPROTOCOL, TEXT_TPROTOCOL, CTFO_TPROTOCOL,
		_tp[DEFAULT_TPROTOCOL] = new TransProtocol;
		_tp[TEXT_TPROTOCOL] = new TextTransProtocol;
		_tp[CTFO_TPROTOCOL] = new CtfoTransProtocol;
	}
	~TransProtocolFac()
	{
        for(int i = 0; i < TPROTOCOL_NUM; i++)
        {
            delete _tp[i];
        }
	}
	TransProtocol *get(int type)
	{
		return _tp[type];
	}
private:
	TransProtocol* _tp[TPROTOCOL_NUM];
};

extern TransProtocolFac __trans_protocol;

class PacketQueue
{
public:
	PacketQueue();

	~PacketQueue();

	//支持上锁和不上锁两种状态，如果不调用set_mutex，那么操作是无锁的。
	void set_mutex(vos::Mutex *m)
	{
		_mutex = m;
	}

	Packet *pop();

	void clear();

	void push(Packet *packet);

	int size();

	bool empty();

	void moveto(PacketQueue *destQueue);

	Packet *get_timeout_list(int64_t now);

	Packet *head();

	Packet* tail();

protected:
    vos::Mutex *_mutex;

	Packet *_head; // 链头
	Packet *_tail; // 链尾
	int _size; // 元素数量
};

#endif /* TPROTOCOL_H_ */
