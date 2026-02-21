/* sync.c - Synchronization primitives for the kernel */

#include "sync.h"
#include "scheduler.h"
#include "interrupt.h"   /* enter_critical(), leave_critical() */
#include "common.h"

#include <stddef.h>

/* Provided by scheduler.c (you already had this helper there) */
extern queue_t* get_ready_queue(void);
extern pcb_t* get_current_process(void);

/* Current running PCB is used in scheduler.c; we need to update it when blocking/waking */
extern pcb_t *current_running;

/* Queue ops */
extern void queue_init(queue_t *q);
extern void queue_put(queue_t *q, node_t *n);
extern node_t* queue_get(queue_t *q);
extern int queue_size(queue_t *q);

/* ---------- Internal helpers ---------- */

static inline void make_ready(pcb_t *p) {
    if (!p) return;
    p->status = PROCESS_READY;
    queue_put(get_ready_queue(), (node_t*)p);
}

/*
 * Block current_running on a wait queue and switch current_running to the next READY process.
 * This matches the style used in do_sleep()/do_yield(): it updates current_running and returns;
 * the actual register/stack switch happens when control returns to assembly.
 */
static void block_current_on(queue_t *wait_q) {
    pcb_t *curr = current_running;
    pcb_t *next = NULL;

    if (curr == NULL) {
        return;
    }

    /* Mark blocked and enqueue on the primitive's wait queue */
    curr->status = PROCESS_BLOCKED;
    queue_put(wait_q, (node_t*)curr);

    /* Pick next runnable */
    next = (pcb_t*)queue_get(get_ready_queue());
    if (next != NULL) {
        current_running = next;
        current_running->status = PROCESS_RUNNING;
        current_running->nested_count = 0;
    } else {
        current_running = NULL;
    }
}

static void wake_one(queue_t *wait_q) {
    node_t *n = queue_get(wait_q);
    if (n == NULL) return;
    make_ready((pcb_t*)n);
}

static void wake_all(queue_t *wait_q) {
    int i, count = queue_size(wait_q);
    for (i = 0; i < count; i++) {
        node_t *n = queue_get(wait_q);
        if (!n) break;
        make_ready((pcb_t*)n);
    }
}

/* ---------- Semaphore ---------- */

void sem_init(sem_t *s, int initial_value) {
    if (!s) return;
    enter_critical();
    s->value = initial_value;
    queue_init(&s->waiters);
    leave_critical();
}

void sem_wait(sem_t *s) {
    if (!s) return;

    enter_critical();

    if (s->value > 0) {
        s->value--;
        leave_critical();
        return;
    }

    /*
     * No permits: block the current process.
     * Note: When this process is woken, it will resume after returning from the syscall/interrupt
     * path; the permit is conceptually granted by the signaler waking a waiter instead of value++.
     */
    block_current_on(&s->waiters);

    leave_critical();
}

void sem_signal(sem_t *s) {
    if (!s) return;

    enter_critical();

    if (queue_size(&s->waiters) > 0) {
        /* Give permit directly to a waiting process */
        wake_one(&s->waiters);
    } else {
        /* No waiters: increase available permits */
        s->value++;
    }

    leave_critical();
}

/* ---------- Condition Variable ---------- */

void cond_init(cond_t *c) {
    if (!c) return;
    enter_critical();
    queue_init(&c->waiters);
    leave_critical();
}

void cond_wait(cond_t *c) {
    if (!c) return;

    enter_critical();
    /*
     * Simplified condvar wait: block until signaled/broadcast.
     * In a full design, you'd atomically release a mutex here.
     */
    block_current_on(&c->waiters);
    leave_critical();
}

void cond_signal(cond_t *c) {
    if (!c) return;
    enter_critical();
    wake_one(&c->waiters);
    leave_critical();
}

void cond_broadcast(cond_t *c) {
    if (!c) return;
    enter_critical();
    wake_all(&c->waiters);
    leave_critical();
}

/* ---------- Barrier ---------- */

void barrier_init(barrier_t *b, int total_threads) {
    if (!b) return;

    enter_critical();
    b->total = (total_threads < 1) ? 1 : total_threads;
    b->count = 0;
    queue_init(&b->waiters);
    leave_critical();
}

void barrier_wait(barrier_t *b) {
    if (!b) return;

    enter_critical();

    b->count++;

    if (b->count < b->total) {
        /* Not enough arrivals yet: block */
        block_current_on(&b->waiters);
        leave_critical();
        return;
    }

    /* Last thread arrives: release everyone */
    b->count = 0;
    wake_all(&b->waiters);

    leave_critical();
}
