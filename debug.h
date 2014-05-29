/******************************************************
 *   FileName: debug.h
 *     Author: liubo  2013-10-17
 *Description:
 *******************************************************/

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdio.h>
#include <execinfo.h>
class debug
{
public:
	static void print_trace(void)
	{
        printf("into print_trace \n");
		int i;
		const int MAX_CALLSTACK_DEPTH = 32; /* ��Ҫ��ӡ��ջ�������� */
		void *traceback[MAX_CALLSTACK_DEPTH]; /* �����洢���ö�ջ�еĵ�ַ */
		/* ���� addr2line ������Դ�ӡ��һ��������ַ���ڵ�Դ����λ��
		 * ���ø�ʽΪ�� addr2line -f -e /tmp/a.out 0x400618
		 * ʹ��ǰ��Դ�������ʱҪ���� -rdynamic -g ѡ��
		 */
		char cmd[512] = "addr2line -f -e ";
		char *prog = cmd + strlen(cmd);
		/* �õ���ǰ��ִ�г����·�����ļ��� */
		int r = readlink("/proc/self/exe", prog, sizeof(cmd) - (prog - cmd) - 1);
		/* popen��fork��һ���ӽ���������/bin/sh, ��ִ��cmd�ַ����е����
		 * ͬʱ���ᴴ��һ���ܵ������ڲ�����'w', �ܵ������׼���������ӣ�
		 * ������һ��FILE��ָ��fpָ���������Ĺܵ����Ժ�ֻҪ��fp��������д�κ����ݣ�
		 * ���ݶ��ᱻ��������׼���룬
		 * ������Ĵ����У��Ὣ���ö�ջ�еĺ�����ַд��ܵ��У�
		 * addr2line�����ӱ�׼�����еõ��ú�����ַ��Ȼ����ݵ�ַ��ӡ��Դ����λ�úͺ�������
		 */
		FILE *fp = popen(cmd, "w");
		/* �õ���ǰ���ö�ջ�е����к�����ַ���ŵ�traceback������ */
		int depth = backtrace(traceback, MAX_CALLSTACK_DEPTH);
		for (i = 0; i < depth; i++)
		{
			/* �õ����ö�ջ�еĺ����ĵ�ַ��Ȼ�󽫵�ַ���͸� addr2line */
			fprintf(fp, "%p/n", traceback[i]);
			/* addr2line �������յ���ַ�󣬻Ὣ������ַ���ڵ�Դ����λ�ô�ӡ����׼��� */
		}
		fclose(fp);
	}
};

#endif /* DEBUG_H_ */
