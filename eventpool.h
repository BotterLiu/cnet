/******************************************************
 *   FileName: eventpool.h
 *     Author: liubo  2012-11-13 
 *Description:
 *******************************************************/

#ifndef EVENTPOOL_H_
#define EVENTPOOL_H_

#include "eventbase.h"
#include <vector>

class eventpool
{
public:
	eventpool();

	virtual ~eventpool();

	bool init(int eventbase_num = 1);

	bool start();

    bool stop();

	int event_add(Socket *event, bool readable = true, bool writeable = false);

public:

    //选择eventbase的策略, 用户是可以重写的。
	virtual eventbase *select_eventbase(Socket *event);
private:

    bool _run;

	int _max_num;
    std::vector<eventbase *> _event_bases;
};

#endif /* EVENTPOOL_H_ */
