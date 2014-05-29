/******************************************************
 *   FileName: log.h
 *     Author: liubo  2012-5-31 
 *Description:
 *******************************************************/

#ifndef  __CLOGGER_H__
#define  __CLOGGER_H__

#include <stdio.h>
#include <string>
#include <map>
#include <stdarg.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#ifdef __WINDOWS
#include <Windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#ifdef WIN32
#if _MSC_VER > 1300
#define __func__ __FUNCTION__
#else
#define __func__  ""
#endif
#endif

enum LOG_MODE
{
	LOG_CLOSE, LOG_CONSOLE, LOG_FILE
};

#define DEFAULT_LOG_LEVEL 10

class CLogger
{

#define LOG_LEVEL(_level)                     \
{                                             \
		int level = (int) _level;             \
		if (!ready())                         \
			return;                           \
		if (level > _loglevel)                \
			return;                           \
		va_list ap;                           \
		va_start(ap, format);                 \
		if (_log2where == LOG_FILE)           \
		{                                     \
			if (backup())                     \
			{                                 \
				fprintf(_fp, header());       \
				vfprintf(_fp, format, ap);    \
				fprintf(_fp, _endtag);        \
				::fflush(_fp);                \
			}                                 \
		}                                     \
		else                                  \
		{                                     \
			printf(header());                 \
			vprintf(format, ap);              \
			printf("%s", _endtag);            \
            ::fflush(stdout);                 \
		}                                     \
		va_end(ap);                           \
}                                             \

public:
	CLogger()
	{
        _logtitle = NULL;
        _log2where = LOG_CONSOLE;
        _fp = NULL;
        _log2file = NULL;
        _loglevel = DEFAULT_LOG_LEVEL;
        _back_line_cnt = 20000;
        _back_size = 50;
        _endtag = "\n";
        _back_cnt = 1;
        _timefmt = "%Y-%m-%d %H:%M:%S";
	}

	bool init(const char* logtitle, unsigned short log2where, const char* log2file, unsigned short loglevel);
	void setbacksize(int size) {_back_size = size;} // MB

public:
	void log1(const char* format, ...)
	{
		LOG_LEVEL(1);
	}
	void log2(const char* format, ...)
	{
		LOG_LEVEL(2);
	}
	void log3(const char* format, ...)
	{
		LOG_LEVEL(3);
	}
	void log4(const char* format, ...)
	{
		LOG_LEVEL(4);
	}
	void log5(const char* format, ...)
	{
		LOG_LEVEL(5);
	}
	void log6(const char* format, ...)
	{
		LOG_LEVEL(6);
	}
	void log7(const char* format, ...)
	{
		LOG_LEVEL(7);
	}
	void log8(const char* format, ...)
	{
		LOG_LEVEL(8);
	}
	void log9(const char* format, ...)
	{
		LOG_LEVEL(9);
	}
	void log10(const char* format, ...)
	{
		LOG_LEVEL(10);
	}
private:
	bool ready();
	bool backup();
	bool reopen();
	const char* header();
private:
	const char* _logtitle;
	unsigned short _log2where;
	const char* _log2file;
	unsigned short _loglevel;
	const char* _endtag;
	const char* _timefmt;
	size_t _back_size;
	unsigned long _back_line_cnt;
	unsigned long _back_cnt;
	FILE* _fp;
};

extern "C" CLogger __g_logger__;

#define INIT_LOG		__g_logger__.init
#define LOG_SIZE        __g_logger__.setbacksize
#define dlog1			__g_logger__.log1	// start or stop log
#define dlog2			__g_logger__.log2	// critical error or fatal error
#define dlog3			__g_logger__.log3	// warning or notice
#define dlog4			__g_logger__.log4	// general log
#define dlog5			__g_logger__.log5	// trace log
#define dlog6			__g_logger__.log6	// packet
#define dlog7			__g_logger__.log7	// the devel model
#define dlog8			__g_logger__.log8	// the devel model
#define dlog9			__g_logger__.log9	// the devel model
#define dlog10			__g_logger__.log10	// the devel model

#ifdef WIN32
#		if _MSC_VER > 1300
#			define TRACEF() dlog7("[%s], line:[%d], file[%s]", __func__, __LINE__, __FILE__)
#		else
#			define TRACEF() {}
#		endif
#else
#		define TRACEF() dlog7("[%s], line:[%d],, file[%s]", __func__, __LINE__, __FILE__)
#endif

#endif

