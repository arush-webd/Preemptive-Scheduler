/* sync.h - Synchronization primitives for the kernel */

#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>
#include "queue.h"

/*
 * This code assumes:
 * - pcb_t can be enqueued as node_t (same assumption as scheduler.c)
 * - enter_critical()/leave_critical() exist (interrupt-based critical sections)
 * - get_ready_queue(), get_current_process() exist (from scheduler.c / headers)
 * - pcb status values include PROCESS_READY / PROCESS_RUNNING / PROCESS_BLOCKED
 */

/* ---------- Semaphore ---------- */
typedef struct {
    int value;
    queue_t waiters;   /* queue of blocked PCBs waiting on this semaphore */
} sem_t;

void sem_init(sem_t *s, int initial_value);
void sem_wait(sem_t *s);
void sem_signal(sem_t *s);

/* ---------- Condition Variable ---------- */
/*
 * Simplified kernel condition variable:
 * - No mutex parameter here (toy-kernel style).
 * - Caller is responsible for ensuring correct usage with their own locking
 *   (often via enter_critical/leave_critical or a separate mutex if provided elsewhere).
 */
typedef struct {
    queue_t waiters;   /* queue of blocked PCBs waiting on this condition */
} cond_t;

void cond_init(cond_t *c);
void cond_wait(cond_t *c);
void cond_signal(cond_t *c);
void cond_broadcast(cond_t *c);

/* ---------- Barrier ---------- */
typedef struct {
    int total;         /* number of threads required */
    int count;         /* number currently arrived */
    queue_t waiters;   /* blocked PCBs waiting for barrier to release */
} barrier_t;

void barrier_init(barrier_t *b, int total_threads);
void barrier_wait(barrier_t *b);

#endif /* SYNC_H */
