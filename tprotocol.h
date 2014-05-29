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
 * client ʹ�����̣�
 * ��1������Transport protocol.
 * (2)send data, TransPortprotocol ����ŵ�send_packqueue �У���epoll��write�������͡�
 * (3)recv data, TransPortPortocol ����ŵ�read_packqueue �У�
 *
 * �������ַ�ʽ�������Ͻ����ݶ���һ�ο����Ĺ��̡�read ��������split�ŵ�
 *
 * �ڴ濽�������ʲ��ԡ�
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
    //encode��һ������
	virtual Packet *encode_pack(const char *data, int len);
    //������һ������
    virtual Packet *decode_pack(const char *data, int len, int &decode_len);
	//data, len, ��Ҫ��������ݣ�pack �����Ľ����
	bool encode(const char *data, int len, PacketQueue *pack_queue);
    //socket �ж�ȡ�������ݣ������ɵ�pakcet push��pack_queue�С�
	int decode(const char *data, int len, PacketQueue *pack_queue);
};

class TextTransProtocol : public TransProtocol
{
public:
    //encode��һ������
	virtual Packet *encode_pack(const char *data, int len);
    //������һ������
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
	//encode��һ������
	virtual Packet *encode_pack(const char *data, int len);
	//������һ������
	virtual Packet *decode_pack(const char *data, int len, int &decode_len);
};


//�൱�ھ�̬�洢
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

	//֧�������Ͳ���������״̬�����������set_mutex����ô�����������ġ�
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

	Packet *_head; // ��ͷ
	Packet *_tail; // ��β
	int _size; // Ԫ������
};

#endif /* TPROTOCOL_H_ */
