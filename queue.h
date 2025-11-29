/* queue.h - Complete queue data structure interface */

#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

/**
 * struct node - Queue node structure
 * @prev: Pointer to previous node in queue
 * @next: Pointer to next node in queue
 *
 * This is a doubly-linked list node that should be embedded
 * in other structures (like PCB). The node itself does not
 * contain data - instead, data structures contain a node.
 *
 * Example usage:
 *   typedef struct {
 *       int data1;
 *       int data2;
 *       node_t node;  // Embedded node for queue operations
 *   } my_struct_t;
 *
 * To get the containing structure from a node pointer:
 *   my_struct_t *ptr = (my_struct_t *)node;
 */
typedef struct node {
    struct node *prev;  /* Previous node in queue (NULL for head) */
    struct node *next;  /* Next node in queue (NULL for tail) */
} node_t;

/**
 * struct queue - Queue structure
 * @head: Pointer to first node in queue
 * @tail: Pointer to last node in queue
 * @size: Number of nodes currently in queue
 *
 * Implements a FIFO (First-In-First-Out) queue using a doubly-linked list.
 * All operations are O(1) except traversal operations.
 */
typedef struct {
    node_t *head;  /* First node (front of queue) */
    node_t *tail;  /* Last node (back of queue) */
    int size;      /* Number of nodes in queue */
} queue_t;

/* ========== BASIC QUEUE OPERATIONS ========== */

/**
 * queue_init - Initialize an empty queue
 * @queue: Pointer to queue structure to initialize
 *
 * Must be called before using any other queue operations.
 */
void queue_init(queue_t *queue);

/**
 * queue_put - Add node to end of queue (enqueue)
 * @queue: Pointer to queue
 * @node: Pointer to node to add
 *
 * Adds the node to the tail of the queue (FIFO ordering).
 * Time complexity: O(1)
 */
void queue_put(queue_t *queue, node_t *node);

/**
 * queue_get - Remove and return node from front of queue (dequeue)
 * @queue: Pointer to queue
 *
 * Removes the node from the head of the queue (FIFO ordering).
 * Time complexity: O(1)
 *
 * Return: Pointer to removed node, or NULL if queue is empty
 */
node_t *queue_get(queue_t *queue);

/**
 * queue_peek - Look at front node without removing it
 * @queue: Pointer to queue
 *
 * Returns the node at the head without removing it.
 * Time complexity: O(1)
 *
 * Return: Pointer to head node, or NULL if queue is empty
 */
node_t *queue_peek(queue_t *queue);

/**
 * queue_size - Get number of elements in queue
 * @queue: Pointer to queue
 *
 * Time complexity: O(1)
 *
 * Return: Number of nodes in queue
 */
int queue_size(queue_t *queue);

/**
 * queue_empty - Check if queue is empty
 * @queue: Pointer to queue
 *
 * Time complexity: O(1)
 *
 * Return: 1 if empty, 0 otherwise
 */
int queue_empty(queue_t *queue);

/* ========== ADVANCED QUEUE OPERATIONS ========== */

/**
 * queue_remove - Remove a specific node from queue
 * @queue: Pointer to queue
 * @node: Pointer to node to remove
 *
 * Removes the specified node from anywhere in the queue.
 * Time complexity: O(n) for verification, O(1) for removal
 *
 * Return: 1 if node was removed, 0 if not found
 */
int queue_remove(queue_t *queue, node_t *node);

/**
 * queue_clear - Remove all nodes from queue
 * @queue: Pointer to queue
 *
 * Makes the queue empty. Does not free the nodes.
 * Time complexity: O(1)
 */
void queue_clear(queue_t *queue);

/**
 * queue_contains - Check if queue contains a specific node
 * @queue: Pointer to queue
 * @node: Pointer to node to search for
 *
 * Time complexity: O(n)
 *
 * Return: 1 if node is in queue, 0 otherwise
 */
int queue_contains(queue_t *queue, node_t *node);

/**
 * queue_for_each - Iterate over all nodes in queue
 * @queue: Pointer to queue
 * @func: Function to call for each node
 * @arg: Additional argument to pass to function
 *
 * Calls func(node, arg) for each node in the queue.
 * Time complexity: O(n)
 */
void queue_for_each(queue_t *queue, void (*func)(node_t *, void *), void *arg);

/**
 * queue_insert_after - Insert node after specified node
 * @queue: Pointer to queue
 * @after: Node to insert after (NULL for head)
 * @node: Node to insert
 *
 * Time complexity: O(1)
 *
 * Return: 1 on success, 0 on failure
 */
int queue_insert_after(queue_t *queue, node_t *after, node_t *node);

/**
 * queue_insert_before - Insert node before specified node
 * @queue: Pointer to queue
 * @before: Node to insert before (NULL for tail)
 * @node: Node to insert
 *
 * Time complexity: O(1)
 *
 * Return: 1 on success, 0 on failure
 */
int queue_insert_before(queue_t *queue, node_t *before, node_t *node);

/* ========== HELPER MACROS ========== */

/**
 * container_of - Get pointer to containing structure
 * @ptr: Pointer to member
 * @type: Type of container structure
 * @member: Name of member within container
 *
 * Example:
 *   node_t *n = queue_get(&q);
 *   pcb_t *pcb = container_of(n, pcb_t, node);
 */
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/**
 * offsetof - Get offset of member within structure
 * @type: Type of structure
 * @member: Name of member
 */
#ifndef offsetof
#define offsetof(type, member) ((uint32_t)&((type *)0)->member)
#endif

/**
 * queue_entry - Get the struct for this entry
 * @ptr: Pointer to node_t member
 * @type: Type of container structure
 * @member: Name of node_t member within container
 *
 * Similar to container_of but specifically for queue nodes.
 * This is a convenience macro for common queue operations.
 */
#define queue_entry(ptr, type, member) container_of(ptr, type, member)

/**
 * queue_for_each_entry - Iterate over queue entries
 * @pos: Loop cursor (pointer to container type)
 * @queue: Pointer to queue
 * @member: Name of node_t member within container
 *
 * Example:
 *   pcb_t *pcb;
 *   queue_for_each_entry(pcb, &ready_queue, node) {
 *       printf("PID: %d\n", pcb->pid);
 *   }
 */
#define queue_for_each_entry(pos, queue, member) \
    for (pos = queue_entry((queue)->head, typeof(*pos), member); \
         &pos->member != NULL; \
         pos = queue_entry(pos->member.next, typeof(*pos), member))

#endif /* QUEUE_H */
