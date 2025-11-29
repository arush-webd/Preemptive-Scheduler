/* syslib.h - System call library interface */

#ifndef SYSLIB_H
#define SYSLIB_H

#include "common.h"

/* System call wrappers */
void sys_yield(void);
void sys_exit(void);
void sys_sleep(uint32_t milliseconds);
int sys_getpriority(void);
void sys_setpriority(int priority);

/* Thread management */
int sys_create_thread(void (*entry)(void), int priority);

/* Lock system calls */
void sys_lock_init(void *lock);
void sys_lock_acquire(void *lock);
void sys_lock_release(void *lock);

/* Condition variable system calls */
void sys_condition_init(void *cond);
void sys_condition_wait(void *lock, void *cond);
void sys_condition_signal(void *cond);
void sys_condition_broadcast(void *cond);

/* Semaphore system calls */
void sys_semaphore_init(void *sem, int value);
void sys_semaphore_down(void *sem);
void sys_semaphore_up(void *sem);

/* Barrier system calls */
void sys_barrier_init(void *bar, int n);
void sys_barrier_wait(void *bar);

#endif /* SYSLIB_H */
