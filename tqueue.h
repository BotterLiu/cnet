/*
 * tqueue.h
 *
 *  Created on: 2012-11-24
 *      Author: humingqing
 *  ͨ�ö��в���������
 */

#ifndef __TQUEUE_H__
#define __TQUEUE_H__

template<typename T>
class TQueue
{
public:
	TQueue()
	{
		_head = _tail = NULL;
		_size = 0;
	}

	~TQueue()
	{
		clear();
	}

	// ������й���
	void push(T *o)
	{
		o->_next = o->_pre = NULL;

		if (_head == NULL)
		{
			_head = _tail = o;
		}
		else
		{
			_tail->_next = o;
			o->_pre = _tail;
			_tail = o;
		}
		_size = _size + 1;
	}

	// �Ƴ�����
	T * pop(void)
	{
		if (_size == 0)
			return NULL;

		T *p = _head;
		if (_head == _tail)
		{
			_head = _tail = NULL;
		}
		else
		{
			_head = _head->_next;
			_head->_pre = NULL;
		}
		_size = _size - 1;

		p->_next = p->_pre = NULL;

		return p;
	}

	// ȫ������
	T * move(int &size)
	{
		T *p = _head;
		size = _size;
		_head = _tail = NULL;
		_size = 0;
		return p;
	}

	// �Ӷ���������
	T * erase(T *o)
	{
		if (o == _head)
		{
			if (_head == _tail)
			{
				_head = _tail = NULL;
			}
			else
			{
				_head = o->_next;
				_head->_pre = NULL;
			}
		}
		else if (o == _tail)
		{
			_tail = o->_pre;
			_tail->_next = NULL;
		}
		else
		{
			o->_pre->_next = o->_next;
			o->_next->_pre = o->_pre;
		}
		_size = _size - 1;
		return o;
	}

	// ��ͷ��ָ��
	T * begin(void)
	{
		return _head;
	}
	// ��βָ��
	T * end(void)
	{
		return _tail;
	}
	// ȡ����һ���ڵ�
	T * next(T *cur)
	{
		return cur->_next;
	}
	// ȡ��ǰһ���ڵ�
	T * pre(T *cur)
	{
		return cur->_pre;
	}
	// ȡ�ö��г���
	int size(void)
	{
		return _size;
	}

//protected:
	// ������е���������
	void clear(void)
	{
		if (_size == 0)
		{
			return;
		}

		T *t = NULL;
		T *p = _head;
		while (p != NULL)
		{
			t = p;
			p = p->_next;
			delete t;
		}
		_head = _tail = NULL;
		_size = 0;
	}

//protected:
	// ���ݶ���ͷ
	T *_head;
	// �������ݶ�β
	T *_tail;
	// �����и���
	int _size;
};

#endif /* TQUEUE_H_ */
