/* scheduler.c - Complete process scheduler implementation */

#include "scheduler.h"
#include "queue.h"
#include "util.h"
#include "interrupt.h"
#include "common.h"

/* External declarations from entry.S */
extern uint64_t time_elapsed;
extern int disable_count;

/* Ready queue for runnable processes */
queue_t ready_queue;

/* Sleeping queue for blocked processes */
queue_t sleeping_queue;

/* Current running process */
pcb_t *current_running = NULL;

/* Process table */
static pcb_t process_table[MAX_PROCESSES];
static int next_pid = 1;

/* Helper function to get ready queue (needed by sync.c) */
queue_t* get_ready_queue(void) {
    return &ready_queue;
}

/* Initialize the scheduler */
void scheduler_init(void) {
    int i;
    
    /* Initialize queues */
    queue_init(&ready_queue);
    queue_init(&sleeping_queue);
    
    /* Initialize process table */
    for (i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].status = PROCESS_FREE;
        process_table[i].priority = DEFAULT_PRIORITY;
        process_table[i].nested_count = 0;
        process_table[i].wakeup_time = 0;
        process_table[i].kernel_stack_top = 0;
    }
    
    current_running = NULL;
}

/* Allocate a new PCB from process table */
pcb_t* pcb_allocate(void) {
    int i;
    
    enter_critical();
    
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].status == PROCESS_FREE) {
            process_table[i].pid = next_pid++;
            process_table[i].status = PROCESS_READY;
            process_table[i].priority = DEFAULT_PRIORITY;
            process_table[i].nested_count = 0;
            process_table[i].wakeup_time = 0;
            
            leave_critical();
            return &process_table[i];
        }
    }
    
    leave_critical();
    return NULL; /* No free PCBs */
}

/* Free a PCB */
void pcb_free(pcb_t *pcb) {
    if (pcb == NULL) {
        return;
    }
    
    enter_critical();
    pcb->status = PROCESS_FREE;
    pcb->pid = 0;
    leave_critical();
}

/* Add a process to the ready queue */
void scheduler_add(pcb_t *pcb) {
    if (pcb == NULL) {
        return;
    }
    
    enter_critical();
    pcb->status = PROCESS_READY;
    queue_put(&ready_queue, (node_t *)pcb);
    leave_critical();
}

/* Main scheduler function - picks next process to run */
void scheduler_entry(void) {
    pcb_t *next;
    
    enter_critical();
    
    /* Get next process from ready queue */
    next = (pcb_t *)queue_get(&ready_queue);
    
    if (next == NULL) {
        /* No processes ready - idle or halt */
        /* In a real OS, we might have an idle process */
        /* For now, we'll just return and let timer interrupt handle it */
        leave_critical();
        return;
    }
    
    /* Set as current running process */
    current_running = next;
    current_running->status = PROCESS_RUNNING;
    current_running->nested_count = 0;
    
    leave_critical();
}

/* Put current running process back into ready queue (round-robin) */
void put_current_running(void) {
    enter_critical();
    
    if (current_running != NULL && current_running->status == PROCESS_RUNNING) {
        /* Add current process to end of ready queue (round-robin) */
        current_running->status = PROCESS_READY;
        queue_put(&ready_queue, (node_t *)current_running);
    }
    
    leave_critical();
}

/* Block current process for specified number of milliseconds */
void do_sleep(uint32_t milliseconds) {
    uint64_t wakeup_time;
    pcb_t *next;
    
    enter_critical();
    
    if (current_running == NULL) {
        leave_critical();
        return;
    }
    
    /* Calculate wakeup time (convert milliseconds to timer ticks) */
    /* Assuming TIMER_HZ = 100 (10ms per tick) */
    /* milliseconds / MS_PER_TICK = number of ticks */
    wakeup_time = time_elapsed + (milliseconds / MS_PER_TICK);
    if (milliseconds % MS_PER_TICK != 0) {
        wakeup_time++; /* Round up */
    }
    
    /* Store wakeup time in PCB */
    current_running->wakeup_time = wakeup_time;
    current_running->status = PROCESS_SLEEPING;
    
    /* Move current process to sleeping queue */
    queue_put(&sleeping_queue, (node_t *)current_running);
    
    /* Get next process to run */
    next = (pcb_t *)queue_get(&ready_queue);
    
    if (next != NULL) {
        current_running = next;
        current_running->status = PROCESS_RUNNING;
        current_running->nested_count = 0;
    } else {
        /* No ready processes - set current_running to NULL */
        /* Timer will continue and eventually wake up sleeping processes */
        current_running = NULL;
    }
    
    leave_critical();
    
    /* Context switch will happen when we return to assembly */
}

/* Check if any sleeping processes should be awakened */
void check_sleeping(void) {
    pcb_t *pcb;
    node_t *node;
    int count, i;
    
    enter_critical();
    
    /* Get number of sleeping processes */
    count = queue_size(&sleeping_queue);
    
    /* Check each sleeping process */
    for (i = 0; i < count; i++) {
        node = queue_get(&sleeping_queue);
        if (node == NULL) {
            break;
        }
        
        pcb = (pcb_t *)node;
        
        /* Check if it's time to wake up */
        if (time_elapsed >= pcb->wakeup_time) {
            /* Wake up process - add to ready queue */
            pcb->status = PROCESS_READY;
            queue_put(&ready_queue, node);
        } else {
            /* Not time yet - put back in sleeping queue */
            queue_put(&sleeping_queue, node);
        }
    }
    
    leave_critical();
}

/* Yield CPU to another process */
void do_yield(void) {
    pcb_t *next;
    
    enter_critical();
    
    /* Put current process back in ready queue */
    if (current_running != NULL && current_running->status == PROCESS_RUNNING) {
        current_running->status = PROCESS_READY;
        queue_put(&ready_queue, (node_t *)current_running);
    }
    
    /* Get next process */
    next = (pcb_t *)queue_get(&ready_queue);
    
    if (next != NULL) {
        current_running = next;
        current_running->status = PROCESS_RUNNING;
        current_running->nested_count = 0;
    }
    
    leave_critical();
    
    /* Context switch happens when we return */
}

/* Exit current process */
void do_exit(void) {
    pcb_t *next;
    
    enter_critical();
    
    /* Mark process as exited (don't put back in ready queue) */
    if (current_running != NULL) {
        current_running->status = PROCESS_EXITED;
        /* Could free PCB here, but might want to keep for debugging */
        /* pcb_free(current_running); */
    }
    
    /* Get next process */
    next = (pcb_t *)queue_get(&ready_queue);
    
    if (next != NULL) {
        current_running = next;
        current_running->status = PROCESS_RUNNING;
        current_running->nested_count = 0;
    } else {
        current_running = NULL;
    }
    
    leave_critical();
    
    /* Never returns to the exited process */
}

/* Get priority of current process */
int do_getpriority(void) {
    int priority;
    
    enter_critical();
    
    if (current_running == NULL) {
        priority = 0;
    } else {
        priority = current_running->priority;
    }
    
    leave_critical();
    
    return priority;
}

/* Set priority of current process */
void do_setpriority(int priority) {
    enter_critical();
    
    if (current_running != NULL) {
        /* Clamp priority to valid range */
        if (priority < MIN_PRIORITY) {
            priority = MIN_PRIORITY;
        }
        if (priority > MAX_PRIORITY) {
            priority = MAX_PRIORITY;
        }
        current_running->priority = priority;
    }
    
    leave_critical();
}

/* Get current running process */
pcb_t* get_current_process(void) {
    return current_running;
}

/* Get process by PID */
pcb_t* get_process_by_pid(int pid) {
    int i;
    
    enter_critical();
    
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && 
            process_table[i].status != PROCESS_FREE) {
            leave_critical();
            return &process_table[i];
        }
    }
    
    leave_critical();
    return NULL;
}

/* Print scheduler statistics (for debugging) */
void scheduler_print_stats(void) {
    int ready_count, sleeping_count;
    
    enter_critical();
    
    ready_count = queue_size(&ready_queue);
    sleeping_count = queue_size(&sleeping_queue);
    
    /* Use printf here if available */
    /* printf("Ready: %d, Sleeping: %d, Current: %d\n", 
           ready_count, sleeping_count, 
           current_running ? current_running->pid : 0); */
    
    leave_critical();
}
