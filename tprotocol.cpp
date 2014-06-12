/******************************************************
 *   FileName: tprotocol.cpp
 *     Author: Triones  2013-9-5 
 *Description:
 *******************************************************/

#include "tprotocol.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

static const char *__TEXT_PACK_END = "\r\n";
static int __TEXT_PACK_END_LEN = strlen(__TEXT_PACK_END);

TransProtocolFac __trans_protocol;

Packet::Packet()
{
	_next = NULL;
	_expire_time = -1;
    _data = NULL;
    _len = 0;
}

Packet::~Packet()
{
    free();
}

void Packet::free()
{
    if(_data != NULL)
    {
    	::free(_data);
    	_data = NULL;
    }
}

int64_t Packet::expire_time()
{
	return _expire_time;
}

void Packet::copy(const char *data, int len)
{
	if (data == NULL || len < 0 || len > DEFAULT_PACKET_MAX_SIZE)
	{
		return ;
	}

    this->free();

    _data = (char *)malloc(len);
    if(_data != NULL)
    {
        memcpy(_data, data, len);
        _len = len;
    }

    return;
}

char *Packet::encode_data()
{
    return _data;
}

int Packet::encode_length()
{
    return _len;
}

char *Packet::decode_data()
{
    return _data;
}
int Packet::decode_length()
{
    return _len;
}

//encode出一个包来
Packet *TransProtocol::encode_pack(const char *data, int len)
{
	if(data == NULL || len < 0 || len > DEFAULT_PACKET_MAX_SIZE)
	{
        return NULL;
	}

	Packet *pack = new Packet;
    pack->_data = (char *)malloc(len);
    if(pack->_data != NULL)
    {
        memcpy(pack->_data, data, len);
        pack->_len = len;
    }
    else
    {
        delete pack;
    	return NULL;
    }

    return pack;
}

//解析出一个包来
Packet *TransProtocol::decode_pack(const char *data, int len, int &decode_len)
{
	if(data == NULL || len < 0 || len > DEFAULT_PACKET_MAX_SIZE)
	{
        return NULL;
	}

	decode_len = 0;

	Packet *pack = new Packet;
    pack->_data = (char *)malloc(len);
    if(pack->_data != NULL)
    {
        memcpy(pack->_data, data, len);
        pack->_len = len;
        decode_len = len;
    }
    else
    {
        delete pack;
    	return NULL;
    }

    return pack;
}

//data, len, 需要编码的数据，pack 编码后的结果。
bool TransProtocol::encode(const char *data, int len, PacketQueue *pack_queue)
{
    Packet *pack = encode_pack(data, len);
    if(pack != NULL)
    {
    	pack_queue->push(pack);
    	return true;
    }
    return false;
}

//socket 中读取到的数据，解析成的pakcet push到pack_queue中。
int TransProtocol::decode(const char *data, int len, PacketQueue *pack_queue)
{
    int decode_len;
    Packet *pack = NULL;
    int offset = 0;

    while((pack = decode_pack(data + offset, len - offset, decode_len)) != NULL)
    {
    	pack_queue->push(pack);
        offset += decode_len;
    }
    return offset;
}

Packet *TextTransProtocol::encode_pack(const char *data, int len)
{
	if(data == NULL || len < 0 || len > DEFAULT_PACKET_MAX_SIZE)
	{
        return NULL;
	}

	Packet *pack = new TextPacket;
    pack->_data = (char *)malloc(len + __TEXT_PACK_END_LEN);
    if(pack->_data != NULL)
    {
        memcpy(pack->_data, data, len);
        memcpy(pack->_data + len, __TEXT_PACK_END, __TEXT_PACK_END_LEN);
        pack->_len = len + __TEXT_PACK_END_LEN;
    }
    else
    {
        delete pack;
    	return NULL;
    }

    return pack;

}

Packet *TextTransProtocol::decode_pack(const char *data, int len, int &decode_len)
{
    int offset = 0;
    decode_len = 0;

    while(offset < len - 1)
    {
    	if(data[offset] == '\r' && data[offset + 1] == '\n')
    	{
            if(offset == 0)
            {
            	offset++;
            	continue;
            }

            decode_len = offset + 2;
        	Packet *pack = new TextPacket;
            pack->_data = (char *)malloc(decode_len);
            pack->_len = decode_len;
            if(pack->_data == NULL)
            {
            	delete pack;
            	return NULL;
            }
            memcpy(pack->_data, data, decode_len);
            return pack;
    	}
        offset++;
    }

    return NULL;
}

static unsigned char CheckSum(unsigned char *data, unsigned int len)
{
	unsigned char sum = 0;
	for(unsigned int i = 0; i < len; i++)
	{
		sum ^= data[i];
	}

	return sum;
}

#pragma pack(1)
struct CTFOHEADER
{

#define CTFO_TAG            0x1adf5fed

    unsigned int  ctfo_begin;
    unsigned int  len;
    unsigned char checkbit;

    CTFOHEADER():ctfo_begin(CTFO_TAG), len (0){}

    CTFOHEADER(unsigned int data_len) : ctfo_begin(CTFO_TAG)
    {
        len = htonl(data_len);
        checkbit = check_sum();
    }

    unsigned char check_sum()
    {
    	return CheckSum((unsigned char*) &ctfo_begin, sizeof(ctfo_begin) + sizeof(len));
    }

    bool check_header()
    {
    	return (ctfo_begin == CTFO_TAG) && (checkbit == check_sum());
    }
};
#pragma pack()

char *CtfoPacket::decode_data()
{
    return _data + sizeof(CTFOHEADER);
}

int CtfoPacket::decode_length()
{
    return _len - sizeof(CTFOHEADER);
}


Packet *CtfoTransProtocol::encode_pack(const char *data, int len)
{
	if(data == NULL || len < 0 || len > DEFAULT_PACKET_MAX_SIZE)
	{
        return NULL;
	}

	Packet *pack = new CtfoPacket;
    pack->_data = (char *)malloc(len + sizeof(CTFOHEADER));
    if(pack->_data != NULL)
    {
    	CTFOHEADER header(len);
    	memcpy(pack->_data, &header, sizeof(header));
    	memcpy(pack->_data + sizeof(CTFOHEADER), data, len);
        pack->_len = len + sizeof(CTFOHEADER);
        return pack;
    }
    else
    {
        delete pack;
    	return NULL;
    }
}

Packet *CtfoTransProtocol::decode_pack(const char *data, int len, int &decode_len)
{
    decode_len = 0;

    if(data == NULL || len < (int)sizeof(CTFOHEADER))
    	return NULL;

    int offset = 0;
	CTFOHEADER *header = NULL;

	while (offset < len - (int)sizeof(CTFOHEADER))
	{
		header = (CTFOHEADER*) (data + offset);
		if (header->check_header())
			break;
		else
			offset++;
	}

	//设置已经解析了的数据的位置
	decode_len = offset;

	//没有找到数据包头，直接返回
	if(offset == len - (int)sizeof(CTFOHEADER))
	{
        return NULL;
	}

	header = (CTFOHEADER*)(data + offset);
	unsigned int data_len = ntohl(header->len);

	//一个包还没有完全解析完，直接返回
	if (len - decode_len < (int)sizeof(CTFOHEADER) + (int)data_len)
	{
		return NULL;
	}

	Packet *pack = new CtfoPacket;
	pack->_data = (char *) malloc(data_len + sizeof(CTFOHEADER));
	if (pack->_data != NULL)
	{
		pack->_len = data_len + (int)sizeof(CTFOHEADER);
		decode_len = pack->_len;
		memcpy(pack->_data, data + offset, decode_len);
		return pack;
	}
    else
    {
        delete pack;
    	return NULL;
    }

    return pack;
}

char *TextPacket::decode_data()
{
    return _data;
}

int TextPacket::decode_length()
{
    return _len - __TEXT_PACK_END_LEN;
}

PacketQueue::PacketQueue()
{
	_head = NULL;
	_tail = NULL;
	_size = 0;
	_mutex = NULL;
}

PacketQueue::~PacketQueue()
{
	clear();
}

Packet *PacketQueue::pop()
{
	vos::CAutoGuard g(_mutex);

	if (_head == NULL)
	{
		return NULL;
	}
	Packet *packet = _head;
	_head = _head->_next;
	if (_head == NULL)
	{
		_tail = NULL;
	}
	_size--;
	return packet;
}

void PacketQueue::clear()
{
	vos::CAutoGuard g(_mutex);

	if (_head == NULL)
	{
		return;
	}
	while (_head != NULL)
	{
		Packet *packet = _head;
		_head = packet->_next;
        delete packet;
	}
	_head = _tail = NULL;
	_size = 0;
}

void PacketQueue::push(Packet *packet)
{
	vos::CAutoGuard g(_mutex);

	if (packet == NULL)
	{
		return;
	}
	packet->_next = NULL;
	if (_tail == NULL)
	{
		_head = packet;
	}
	else
	{
		_tail->_next = packet;
	}
	_tail = packet;
	_size++;
}

int PacketQueue::size()
{
	return _size;
}

bool PacketQueue::empty()
{
	return (_size == 0);
}

void PacketQueue::moveto(PacketQueue *destQueue)
{
	vos::CAutoGuard g(_mutex);

	if (_head == NULL)
	{ // 是空链
		return;
	}
	if (destQueue->_tail == NULL)
	{
		destQueue->_head = _head;
	}
	else
	{
		destQueue->_tail->_next = _head;
	}
	destQueue->_tail = _tail;
	destQueue->_size += _size;
	_head = _tail = NULL;
	_size = 0;
}

Packet *PacketQueue::get_timeout_list(int64_t now)
{
	vos::CAutoGuard g(_mutex);

	Packet *list, *tail;
	list = tail = NULL;
	while (_head != NULL)
	{
		int64_t t = _head->expire_time();
		if (t == 0 || t >= now)
			break;
		if (tail == NULL)
		{
			list = _head;
		}
		else
		{
			tail->_next = _head;
		}
		tail = _head;

		_head = _head->_next;
		if (_head == NULL)
		{
			_tail = NULL;
		}
		_size--;
	}
	if (tail)
	{
		tail->_next = NULL;
	}
	return list;
}

Packet *PacketQueue::head()
{
	return _head;
}

Packet* PacketQueue::tail()
{
	return _tail;
}
