/******************************************************
 *   FileName: sock_buffer.h
 *     Author: liubo  2012-5-23 
 *Description:
 *Description: sock读的缓冲区，缓冲区有个最大值，没次读到socket为空或是buffer满时停止读数据。
 *Description: 对于收到的数据进行处理提供给上层处理，处理完毕后。继续读取数据。
 *******************************************************/
#ifndef SOCK_BUFFER_H_
#define SOCK_BUFFER_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <string>
#include "vos_lock.h"

using namespace std;

/*******************************************************
 * 1. 主要用于数据的整合与分组。
 * (1)将外部的数据add_data,加入如到缓冲区中，通过分包取得数据，取出来的数据指明的分段信息，而不是将数据拷贝出来。
 * (2)也可以不同过add_data, 而是通过read_pos,和absolute_avail，直接拷贝进去。
 *******************************************************/

class FifoBuffer
{
public:
	class Packet
	{
	public:
		Packet()
		{
			data = NULL;
			offset = 0;
		}
		const char *data;
		int offset;
	};

    FifoBuffer():_read_offset(0), _write_offset(0), _size(0), _buffer(NULL)
    {

    }

    virtual ~FifoBuffer()
    {
    	delete []_buffer;
    	_read_offset = _write_offset = _size = 0;
    }

//    void set_split( SplitPack *split_fun)
//    {
//    	_split_fun = split_fun;
//    }

    void set_max_size(int max_size = 4096)
    {
        if(_buffer != NULL)
        {
        	delete []_buffer;
        	_buffer = NULL;
        }

        _size = max_size;
        _buffer = new char[max_size];

        return;
    }

    //返回的是写入的长度, -1 表示有错误发生
    int add_data(const char *data, int len)
	{
		//还有空间写入。
		if (data == NULL || len < 0)
			return 0;
        //最大拷贝的数据为absolute_avail()大小。

		if (len > (int)absolute_avail())
		{
            printf("len = %d absolute_avail = %u \n", len , absolute_avail());
			len = (int)absolute_avail();
		}

		if (len > (int)relative_avail())
		{
//			 printf("len = %d relative_avail = %u to reset   _read_offset = %u _write_offset = %u \n",
//					 len , relative_avail(), _read_offset, _write_offset);

			 reset();
//             printf("after reset _read_offset = %u _write_offset = %u \n", _read_offset, _write_offset);
		}

		memcpy(_buffer + _write_offset, data, len);
		_write_offset += len;

		return len;
	}

	/********************************
	 * 重新排列buffer, 将数据拷贝到开头处。
	 ********************************/
	void reset()
	{
		memcpy(_buffer, _buffer + _read_offset, _write_offset - _read_offset);
		_write_offset = _write_offset - _read_offset;
		_read_offset = 0;
	}

	void clear()
	{
		_read_offset = _write_offset = 0;
	}

    //可获得的绝对空间，空间总大小。
	unsigned int absolute_avail()
    {
    	return _size - (_write_offset - _read_offset);
    }

    unsigned int relative_avail()
    {
    	return _size - _write_offset;
    }

    char *get_buffer()
    {
    	return _buffer;
    }

    char *read_pos()
    {
        return _buffer + _read_offset;
    }

    char *write_pos()
    {
    	return _buffer + _write_offset;
    }

    unsigned int get_read_offset()
	{
		return _read_offset;
	}

	void read_offset_add(int i)
	{
		_read_offset += i;
	}

	void set_read_offset(int i)
	{
		_read_offset = i;
	}

	unsigned int get_write_offset()
	{
		return _write_offset;
	}

	void set_write_offset(int i)
	{
		_write_offset = i;
	}

	void write_offset_add(int i)
	{
		_write_offset += i;
	}

	unsigned int use()
    {
    	return _write_offset - _read_offset;
    }

	unsigned int get_size()
    {
    	return _size;
    }

//    //默认将所有的数据都取出来，具体应用时需要将这个重写, 注意考虑中间出现的异常情况。
//    virtual  int get_one_packet(Packet *packet)
//    {
//    	return _split_fun->split_pack((void*)this, (void*)packet);
//    }

    string info()
    {
        char buffer[128] = {0};
        snprintf(buffer, sizeof(buffer) - 1, "size : %d read pos : %d write pos : %d ",
        		_size, _read_offset, _write_offset);
        return buffer;
    }

private:
//    SplitPack *_split_fun;
    unsigned int _read_offset;
    unsigned int _write_offset;
    unsigned int _size;
    char *_buffer;
};

///***********************************************
// *
// * 数据发送时，直接发送，只有发送不成功的才加入到发送缓冲区，过一会再继续发送。
// *
// ***********************************************/
//
//class SendBuffer
//{
//#define MAX_SEND_BUFFER_SIZE  (100 * 1024 * 1024)
//
//public:
//
//    struct send_buffer
//    {
//        struct sockaddr_in  sock_addr;
//    	int len;
//    	char data;
//
//    	send_buffer(){len = 0;}
//    };
//
//    SendBuffer(int max_size = MAX_SEND_BUFFER_SIZE):_max_size(max_size), _cur_size(0){}
//
//    ~SendBuffer()
//    {
//        clear();
//        _cur_size = 0;
//    }
//
//    void clear()
//    {
//    	vos::CAutoGuard g(_mutex);
//
//        list<send_buffer*>::iterator iter = _list_data.begin();
//        for(; iter != _list_data.end(); ++iter)
//        {
//        	send_buffer *node = *iter;
//        	delete node;
//        }
//
//        _list_data.clear();
//    }
//
//    //是否从头部添加
//	bool add_data(const void *data, int len, bool header = false)
//	{
//        if(data == NULL || len < 0 || len + _cur_size > _max_size)
//        {
//        	return false;
//        }
//
//        vos::CAutoGuard g(_mutex);
//
//		send_buffer *buffer = (send_buffer*) malloc(sizeof(send_buffer) - 1 + len);
//		if (buffer == NULL)
//		{
//			return false;
//		}
//
//        buffer->len = len;
//        memcpy(&(buffer->data), data, len);
//		if (header){
//			_list_data.push_front(buffer);
//		}else{
//			_list_data.push_back(buffer);
//		}
//        _cur_size += len;
//
//        return true;
//	}
//
//	bool is_empty()
//	{
//	    vos::CAutoGuard g(_mutex);
//	    return _list_data.empty();
//	}
//
//	bool add_ctfo_pack(const void *data, int len)
//	{
//        if(data == NULL || len < 0 || len + _cur_size > _max_size)
//        {
//        	return false;
//        }
//
//        vos::CAutoGuard g(_mutex);
//		send_buffer *buffer = (send_buffer*) malloc(sizeof(CTFOHEADER) + sizeof(send_buffer) - 1 + len);
//		if (buffer == NULL)
//		{
//			return false;
//		}
//
//		buffer->len = len + sizeof(CTFOHEADER);
//		CTFOHEADER header(len);
//		memcpy(&(buffer->data), &header, sizeof(header));
//
//		memcpy(&(buffer->data) + sizeof(header), data, len);
//		_list_data.push_back(buffer);
//		_cur_size += len;
//
//		return true;
//
//	}
//
//    //是否从头部添加
//	bool add_data(const void *data, int len, struct sockaddr_in &sock_addr, bool header = false)
//	{
//        if(data == NULL || len < 0 || len + _cur_size > _max_size)
//        {
//        	return false;
//        }
//
//        vos::CAutoGuard g(_mutex);
//
//		send_buffer *buffer = (send_buffer*) malloc(sizeof(send_buffer) - 1 + len);
//		if (buffer == NULL)
//		{
//			return false;
//		}
////		buffer->sock_addr = sock_addr;
//        memcpy(&(buffer->sock_addr), &sock_addr, sizeof(sock_addr));
//
//        buffer->len = len;
//        memcpy(&(buffer->data), data, len);
//		if (header){
//			_list_data.push_front(buffer);
//		}else{
//			_list_data.push_back(buffer);
//		}
//        _cur_size += len;
//
//        return true;
//	}
//
//	bool add_ctfo_pack(const void *data, int len, struct sockaddr_in &sock_addr)
//	{
//        if(data == NULL || len < 0 || len + _cur_size > _max_size)
//        {
//        	return false;
//        }
//
//        vos::CAutoGuard g(_mutex);
//		send_buffer *buffer = (send_buffer*) malloc(sizeof(CTFOHEADER) + sizeof(send_buffer) - 1 + len);
//		if (buffer == NULL)
//		{
//			return false;
//		}
//		//		buffer->sock_addr = sock_addr;
//		memcpy(&(buffer->sock_addr), &sock_addr, sizeof(sock_addr));
//		buffer->len = len + sizeof(CTFOHEADER);
//		CTFOHEADER header(len);
//		memcpy(&(buffer->data), &header, sizeof(header));
//
//		memcpy(&(buffer->data) + sizeof(header), data, len);
//		_list_data.push_back(buffer);
//		_cur_size += buffer->len;
//
//		return true;
//
//	}
//
//	send_buffer *get_data()
//	{
//		vos::CAutoGuard g(_mutex);
//
//		if (_list_data.empty())
//			return NULL;
//		send_buffer *buffer = _list_data.front();
//		_list_data.pop_front();
//		if (buffer != NULL)
//		{
//			_cur_size -= buffer->len;
//		}
//		return buffer;
//	}
//
//    static void free(send_buffer *b)
//    {
//        if(b != NULL)
//        {
//    	    ::free(b);
//            b = NULL;
//        }
//    }
//
//private:
//    unsigned	int _max_size;
//    unsigned	int _cur_size;
//	list<send_buffer *> _list_data;
//	vos::Mutex _mutex;
//};

#endif /* SOCK_BUFFER_H_ */
