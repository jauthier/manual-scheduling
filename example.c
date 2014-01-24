#include <stdio.h>
#include "kapi.h"
#include "mythread.h"

#define UNUSED(A) A __attribute__((unused))

struct cyg_thread {int i;};

static cyg_mutex_t g_mutex;

static void thread_proc1(UNUSED(cyg_addrword_t data))
{
	while(true)
	{
		cyg_mutex_lock(&g_mutex);
		printf("T1\n");
		cyg_mutex_unlock(&g_mutex);
	}
}

static void thread_proc2(UNUSED(cyg_addrword_t data))
{
	while(true)
	{
		cyg_mutex_lock(&g_mutex);
		printf("T2\n");
		cyg_mutex_unlock(&g_mutex);
	}
}

int main(UNUSED(int argc), UNUSED(char** argv))
{
	char stack1[1000];
	cyg_thread thread1;
	cyg_handle_t thread_handle1;
	char stack2[1000];
	cyg_thread thread2;
	cyg_handle_t thread_handle2;

	mythread_init();

	cyg_mutex_init(&g_mutex);

	cyg_thread_create(0, thread_proc1, 0, "T1", &stack1, sizeof(stack1), &thread_handle1, &thread1);
	cyg_thread_resume(thread_handle1);

	cyg_thread_create(0, thread_proc2, 0, "T2", &stack2, sizeof(stack2), &thread_handle2, &thread2);
	cyg_thread_resume(thread_handle2);

	//mythread_schedule_roundrobin();
	mythread_schedule_random();

	return 0;
}
