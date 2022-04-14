#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/syscall.h>
#include "../spinlock.h"
#include <setjmp.h>
typedef unsigned long long int kthread_t;

typedef enum threadStatus
{
    READY,
    RUNNING,
    FINISHED,
    BLOCKED_JOIN,
    BLOCKED_SEMAPHORE,
    CANCELLED
} status_t;

typedef struct kthread_node
{
    kthread_t tid;
    void *args;
    void *(*f)(void *);
    int kernel_thread_id;
    void *return_value;
    struct kthread_node *next;
    struct kthread_node *prev;
    jmp_buf env;
} kthread_node;
struct kthread_list
{
    spinlock_t lock;
    struct kthread_node *head;
    struct kthread_node *current;
    struct kthread_node *master;

} kthread_list = {.lock.locked = 0, .head = NULL, .current = NULL, .master = NULL};
typedef struct attr
{
    int novalue;
} attr;

void init_ll();
kthread_t *allocate_kthread();
struct kthread_node *allocate_kthread_node();
void *kthread_create(kthread_t *k, attr *attr, void *(*f)(void *), void *args);
int kthread_join(kthread_t thread, void **retval);
int kthread_kill(kthread_t thread, int sig);