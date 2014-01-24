#ifndef CYG_KAPI_H
#define CYG_KAPI_H

#include <stdint.h>
#include <stdbool.h>

typedef int cyg_addrword_t;
typedef uint32_t cyg_ucount32;
typedef void (cyg_thread_entry_t)(cyg_addrword_t data);
typedef int cyg_handle_t;
typedef int cyg_thread;
typedef bool cyg_bool_t;

struct cyg_mutex {
	bool initialized;
};
typedef struct cyg_mutex cyg_mutex_t;

void cyg_thread_create(
	cyg_addrword_t sched_info, cyg_thread_entry_t* entry,
	cyg_addrword_t entry_data, char* name,
	void* stack_base, cyg_ucount32 stack_size,
	cyg_handle_t* handle, cyg_thread* thread
);

void cyg_thread_resume(cyg_handle_t thread);

void cyg_mutex_init(cyg_mutex_t* mutex);

cyg_bool_t cyg_mutex_lock(cyg_mutex_t* mutex);

void cyg_mutex_unlock(cyg_mutex_t* mutex);

#endif
