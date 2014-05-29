/******************************************************
 *   FileName: sock_buffer.h
 *     Author: liubo  2012-5-23 
 *Description:
 *Description: sock���Ļ��������������и����ֵ��û�ζ���socketΪ�ջ���buffer��ʱֹͣ�����ݡ�
 *Description: �����յ������ݽ��д����ṩ���ϲ㴦��������Ϻ󡣼�����ȡ���ݡ�
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
 * 1. ��Ҫ�������ݵ���������顣
 * (1)���ⲿ������add_data,�����絽�������У�ͨ���ְ�ȡ�����ݣ�ȡ����������ָ���ķֶ���Ϣ�������ǽ����ݿ���������
 * (2)Ҳ���Բ�ͬ��add_data, ����ͨ��read_pos,��absolute_avail��ֱ�ӿ�����ȥ��
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

    //���ص���д��ĳ���, -1 ��ʾ�д�����
    int add_data(const char *data, int len)
	{
		//���пռ�д�롣
		if (data == NULL || len < 0)
			return 0;
        //��󿽱�������Ϊabsolute_avail()��С��

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
	 * ��������buffer, �����ݿ�������ͷ����
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

    //�ɻ�õľ��Կռ䣬�ռ��ܴ�С��
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

//    //Ĭ�Ͻ����е����ݶ�ȡ����������Ӧ��ʱ��Ҫ�������д, ע�⿼���м���ֵ��쳣�����
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
// * ���ݷ���ʱ��ֱ�ӷ��ͣ�ֻ�з��Ͳ��ɹ��Ĳż��뵽���ͻ���������һ���ټ������͡�
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
//    //�Ƿ��ͷ�����
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
//    //�Ƿ��ͷ�����
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
