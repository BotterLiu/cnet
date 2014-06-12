/******************************************************
 *   FileName: log.cpp
 *     Author: Triones  2012-5-31 
 *Description:
 *******************************************************/

#include "log.h"

bool CLogger::ready()
{
	if (_log2where == LOG_FILE)
	{
		return _fp != NULL;
	}
	if (_log2where == LOG_CLOSE)
	{
		return false;
	}
	if (_log2where == LOG_CONSOLE)
	{
		return true;
	}
	return false;
}

bool CLogger::backup()
{
	if (!_fp)
	{
		return false;
	}
	if (++_back_cnt > _back_line_cnt)
	{
		struct stat st;
		if (stat(_log2file, &st) != 0)
			return false;
		if (st.st_size > (int) _back_size * 1024 * 1024)
		{
			fclose(_fp);
			_fp = NULL;
			char newname[200] = {0};
			time_t now = time(0);
			struct tm* tmNow = localtime(&now);
			sprintf(newname, "%s-%02d%02d%02d%02d%02d%02d", _log2file, tmNow->tm_year + 1900,
					tmNow->tm_mon, tmNow->tm_mday, tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec);
			rename(_log2file, newname);
			_back_cnt = 0;
			if (!reopen())
			{
				return false;
			}
			fprintf(_fp, "rotated to new log[%s], at %s \r\n", _logtitle, newname);
		}
	}
	return true;
}

bool CLogger::reopen()
{
	if (_fp)
	{
		return _fp != NULL;
	}
	return (_fp = fopen(_log2file, "ab+")) != NULL;
}

bool CLogger::init(const char* logtitle, unsigned short log2where, const char* log2file, unsigned short loglevel)
{
	bool ret = false;
	_logtitle = logtitle;
	_log2where = log2where;
	_log2file = log2file;
	_loglevel = loglevel;
	_endtag = "\n";
	_timefmt = "%Y-%m-%d %H:%M:%S";

	switch (_log2where)
	{
	case LOG_FILE:
		reopen();
		dlog1(
				"\r\n\r\n\r\n\t\t\t%s \r\n\t\t\tLog title:[%s] level[%d] ==> log2file:%s \r\n\t\t\t%s",
				"***********************************************************************",
				_logtitle, _loglevel, _log2file,
				"***********************************************************************");
		ret = true;
		break;
	case LOG_CONSOLE:
		dlog1(
				"\r\n\r\n\r\n\t\t\t%s \r\n\t\t\tLog title:[%s] level[%d] ==> print to console \r\n\t\t\t%s",
				"***********************************************************************",
				_logtitle, _loglevel,
				"***********************************************************************");
		ret = true;
		break;
	case LOG_CLOSE:
		ret = false;
		break;
	default:
		break;
	}
	return ret;
}

const char* CLogger::header()
{
	unsigned int tid = 0;
	static char buf[64] = {0};
    memset(buf, sizeof(buf), 0);

#ifdef WIN32
	tid = ::GetCurrentThreadId();
	sprintf(buf, "[%4u]", tid);
#else
	tid = (uint)pthread_self();
	sprintf(buf, "[%4u]", tid);
#endif
	time_t now = time(0);
	struct tm* tmNow = localtime(&now);
	strftime(buf + strlen(buf), 128, _timefmt, tmNow);
	sprintf(buf + strlen(buf), " : ");
	return buf;
}

CLogger __g_logger__;


//int main()
//{
//	INIT_LOG(NULL, LOG_FILE, "test.log", 7);
//    dlog1("%s", "log1111111111111111111111111111");
//    dlog2("%s", "log2222222222222222222222222222");
//    dlog3("%s", "log3333333333333333333333333333");
//    dlog4("%s", "log4444444444444444444444444444");
//    dlog5("%s", "log5555555555555555555555555555");
//    dlog6("%s", "log6666666666666666666666666666");
//    dlog7("%s", "log7777777777777777777777777777");
//    dlog8("%s", "log8888888888888888888888888888");
//    dlog9("%s", "log9999999999999999999999999999");
//    dlog10("%s","log000000000000000000000000000");
//}
