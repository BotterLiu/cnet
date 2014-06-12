/******************************************************
 *   FileName: vos_sem.cpp
 *     Author: Triones  2012-7-6 
 *Description:
 *******************************************************/

#include "vos.h"
#include "vos_sem.h"
#include <errno.h>
#include <time.h>
#include <stdio.h>

namespace vos
{
Sem::Sem(unsigned int value /*= 0*/, unsigned int max_value /*= 0*/)
{
	_inited = false;
	init(value, max_value);
}

Sem::~Sem()
{
	uninit();
}

int Sem::init(unsigned int value, unsigned int max_value)
{
	if (_inited)
		return NI_ERR_SEM_ALREADYINIT;

#ifndef __WINDOWS
	int ret = sem_init(&_sem, 0, value);
	if (ret == -1)
		return NI_ERR_SEM_SYS;
#else
	_sem = CreateSemaphore(
			NULL, // default security attributes - lpSemaphoreAttributes���ź����İ�ȫ����
			value,// initial count - lInitialCount�ǳ�ʼ�����ź���
			max_value,// maximum count - lMaximumCount�������ź������ӵ����ֵ
			NULL);// unna
	if (_sem == NULL)
	{
		printf("CreateSemaphore error: %d\n", GetLastError());
		return NI_ERR_SEM_SYS;
	}
#endif

	_inited = true;
	_value = value;
	return NI_ERR_SEM_OK;
}

int Sem::uninit()
{
	int ret = NI_ERR_SEM_SYS;

#ifndef __WINDOWS
	ret = sem_destroy(&_sem);
#else
	CloseHandle(_sem);
	ret = NI_ERR_SEM_OK;
#endif

	_inited = false;
	_value = 0;
	return ret;
}

int Sem::sem_wait()
{
	if (!_inited)
		return NI_ERR_SEM_NOTINIT;

#ifndef __WINDOWS
	while (0 != ::sem_wait(&_sem))
	{
		if (errno == EINTR)
			continue;
		else
		{
			return NI_ERR_SEM_SYS;
		}
	}
#else
	WaitForSingleObject(_sem, INFINITE);
#endif

	_value--;
	return NI_ERR_SEM_OK;
}

int Sem::sem_trywait()
{
	if (!_inited)
		return NI_ERR_SEM_NOTINIT;

	int ret = NI_ERR_SEM_SYS;

#ifndef __WINDOWS
	ret = ::sem_trywait(&_sem);
#else
	DWORD dw_ret = WaitForSingleObject(_sem, 0);
	if ( dw_ret == WAIT_OBJECT_0 )
	ret = NI_ERR_SEM_OK;
	else
	ret = NI_ERR_SEM_SYS;
#endif

	if (ret == NI_ERR_SEM_OK)
		_value--;

	return ret;
}

int Sem::sem_timewait(unsigned int time_out)
{
	if (!_inited)
		return NI_ERR_SEM_NOTINIT;

	int ret = NI_ERR_SEM_SYS;

#ifndef __WINDOWS
	struct timespec time_spec;

	time_spec.tv_sec = time(NULL) + time_out / 1000;
	time_spec.tv_nsec = (time_out % 1000) * 1000000;

	while (0 != ::sem_timedwait(&_sem, &time_spec))
	{
		if (errno == EINTR)
			continue;
		else
		{
			printf("sem_wait() failed! errno:%d(110 timeout)\n", errno);
			return NI_ERR_SEM_SYS;
		}
	}

	ret = NI_ERR_SEM_OK;
#else
	DWORD dw_ret = WaitForSingleObject(_sem, 0);
	if ( dw_ret == WAIT_OBJECT_0 )
	ret = NI_ERR_SEM_OK;
	else
	ret = NI_ERR_SEM_SYS;
#endif

	if (ret == NI_ERR_SEM_OK)
		_value--;

	return ret;
}

int Sem::sem_post()
{
	if (!_inited)
		return NI_ERR_SEM_NOTINIT;

#ifndef __WINDOWS
	::sem_post(&_sem);
#else
	ReleaseSemaphore(_sem, 1, NULL);
#endif

	_value++;

	return NI_ERR_SEM_OK;
}

int Sem::sem_getvalue()
{
	if (!_inited)
		return NI_ERR_SEM_SYS;

	int value = 0;

#ifndef __WINDOWS
	::sem_getvalue(&_sem, &value);
#else
	value = _value;
#endif

	return value;
}

} /* namespace vos */
