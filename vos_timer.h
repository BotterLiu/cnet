/******************************************************
 *   FileName: vos_timer.h
 *     Author: liubo  2012-8-15
 *Description: ��ֲ��libevent�Ķ�ʱ��ʵ�֣���װ��ͳһ�Ľӿڡ�
 *******************************************************/

#ifndef VOS_TIMER_H_
#define VOS_TIMER_H_

#define RELATIVE_TIMER 1
#define ABSOLUTE_TIMER 2

#ifndef __WINDOWS
#include <sys/time.h>
#else
#include <windows.h>
#endif

namespace vos
{

struct event;
typedef struct min_heap
{
	struct event** p;
	unsigned n, a;
} min_heap_t;

class Timer
{
public:
	Timer();
	virtual ~Timer();
    /**************************************
     * input: interval: ÿ��ִ�е�ʱ�����, ��λ�Ǻ��롣
     *        fun arg : �ص������Լ�������
     *        flag    : ���Զ�ʱ��������Զ�ʱ�����������Զ�ʱ��
     *        exe_num : ֻ������Զ�ʱ������Ч����ʾִ�еĴ���������Ϊ1��
     * return: ���ɶ�ʱ����ID
     **************************************/
	unsigned int timer_add(int interval, void (*fun)(void*), void *arg,  int flag = ABSOLUTE_TIMER,
			int exe_num = 0);
    /***************************************
     * description:
     * ȥ���Ѿ�����Ķ�ʱ�������������ʱ����ĸ���Ѿ������ˣ�������֮��Ҫ����ɾ����
     * ��Զ�ʱ����������ɺ��Timer���Լ��ͷŵ���
     ***************************************/
	bool timer_remove(unsigned int timer_id);
    /***************************************
     * description: Timer���ڱ�������û���Լ���ִ���̣߳����ڱ������ߡ�������Ҫ��Ϊ�˱�������߳�ͬ����
     * ��ʱ����ѭ�����������ɶ�ʱ����ӵ���߽���ѭ�����á�������Сʱ���������˶�ʱ���ľ��ȡ�
     ***************************************/
	int timer_process(struct timeval *now);
//
	int timer_process();

private:
	struct min_heap _min_heap;
    unsigned int _timer_id;
};

} /* namespace vos */
#endif /* VOS_TIMER_H_ */
