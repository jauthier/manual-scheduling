#include "kapi.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define UNUSED(A) A __attribute__((unused))
#define ENTER(fmt, ...) fprintf(stderr, "+%s: " fmt "\n", __func__ __VA_ARGS__)
#define LEAVE(fmt, ...) fprintf(stderr, "-%s: " fmt "\n", __func__ __VA_ARGS__)

typedef int mythread_id;

static pthread_key_t g_self;
static pthread_mutex_t g_mutex;
static pthread_cond_t g_schedule;

struct mythread_thread {
	mythread_id id;
	bool should_run;
	bool exited;
	pthread_cond_t cv;
	cyg_thread_entry_t* entry;
	cyg_addrword_t entry_data;
	pthread_t pthread;
};

struct mythread_thread_list {
	struct mythread_thread threads[10];
	size_t used;
} g_threads = {{}, 0};

struct mythread_mutex {
	cyg_mutex_t* cyg_mutex;
	bool locked;
	mythread_id owner;
};

struct mythread_mutex_list {
	struct mythread_mutex mutexes[10];
	size_t used;
} g_mutexes = {{}, 0};

static struct mythread_mutex* get_shadow_mutex(struct cyg_mutex* cyg_mutex)
{
	size_t i;
	for (i=0; i<g_mutexes.used; i++) {
		if (g_mutexes.mutexes[i].cyg_mutex == cyg_mutex) {
			return &g_mutexes.mutexes[i];
		}
	}
	return NULL;
}

struct mythread_thread* get_self(void)
{
	return pthread_getspecific(g_self);
}

static void block(void)
{
	struct mythread_thread* self = get_self();
	pthread_mutex_lock(&g_mutex);
	self->should_run = false;
	pthread_cond_signal(&g_schedule);
	while (!self->should_run) {
		pthread_cond_wait(&self->cv, &g_mutex);
	}
	pthread_mutex_unlock(&g_mutex);
}

static void resume_thread(struct mythread_thread* thread)
{
	pthread_mutex_lock(&g_mutex);
	thread->should_run = true;
	pthread_cond_signal(&thread->cv);
	pthread_cond_wait(&g_schedule, &g_mutex);
	pthread_mutex_unlock(&g_mutex);
}

static void* start_thread(void* arg)
{
	struct mythread_thread* self = arg;
	ENTER("");
	pthread_setspecific(g_self, self);
	pthread_cond_init(&self->cv, NULL);
	self->should_run = false;
	block();
	self->entry(self->entry_data);
	pthread_mutex_lock(&g_mutex);
	self->exited = true;
	pthread_cond_signal(&g_schedule);
	pthread_mutex_unlock(&g_mutex);
	pthread_cond_destroy(&self->cv);
	LEAVE("");
	return NULL;
}

void cyg_thread_create(
	UNUSED(cyg_addrword_t sched_info), cyg_thread_entry_t* entry,
	cyg_addrword_t entry_data, UNUSED(char* name),
	UNUSED(void* stack_base), UNUSED(cyg_ucount32 stack_size),
	UNUSED(cyg_handle_t* handle), UNUSED(cyg_thread* thread)
)
{
	struct mythread_thread* self;
	ENTER("");
	self = &g_threads.threads[g_threads.used++];
	self->entry = entry;
	self->entry_data = entry_data;
	self->exited = false;
	pthread_create(&self->pthread, NULL, start_thread, self);
	LEAVE("");
}

void cyg_thread_resume(UNUSED(cyg_handle_t thread))
{
}

void cyg_mutex_init(cyg_mutex_t* cyg_mutex)
{
	struct mythread_mutex* shadow = &g_mutexes.mutexes[g_mutexes.used];
	shadow->cyg_mutex = cyg_mutex;
	g_mutexes.used++;
}

cyg_bool_t cyg_mutex_lock(cyg_mutex_t* mutex)
{
	struct mythread_mutex* shadow = get_shadow_mutex(mutex);
	struct mythread_thread* self = get_self();

	block();
	while (shadow->locked) {
		block();
	}
	shadow->owner = self->id;
	shadow->locked = true;

	return true;
}

void cyg_mutex_unlock(cyg_mutex_t* mutex)
{
	struct mythread_mutex* shadow = get_shadow_mutex(mutex);
	block();
	shadow->locked = false;
}

void mythread_schedule_roundrobin(void)
{
	while (true) {
		size_t i;
		for (i=0; i<g_threads.used; i++) {
			if (!g_threads.threads[i].exited) {
				resume_thread(&g_threads.threads[i]);
			}
		}
	}
}

void mythread_schedule_random(void)
{
	while (true) {
		size_t num_threads=0;
		size_t i;
		size_t to_run;
		for (i=0; i<g_threads.used; i++) {
			if (!g_threads.threads[i].exited) {
				num_threads++;
			}
		}
		to_run = random() % num_threads;
		for (i=0; i<g_threads.used; i++) {
			if (!g_threads.threads[i].exited) {
				if (to_run == 0) {
					resume_thread(&g_threads.threads[i]);
					break;
				}
				to_run--;
			}
		}
	}
}

void mythread_init(void)
{
	pthread_key_create(&g_self, NULL);
	pthread_mutex_init(&g_mutex, NULL);
	pthread_cond_init(&g_schedule, NULL);
	g_threads.used = 0;
	g_mutexes.used = 0;
}
