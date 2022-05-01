#ifndef KTHREAD_H_INCLUDED
#define KTHREAD_H_INCLUDED
#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/syscall.h>
#include <setjmp.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
typedef unsigned long long int kthread_t;
#define INTERVAL 50
#define STACK_SIZE 40960
#define JB_SP 6
#define JB_PC 7

enum status
{
    READY,
    RUNNING,
    FINISHED,
    CANCELLED,
    BLOCKED_JOIN,
    STOPPED,
    BLOCKED_MUTEX
};

typedef struct kthread_node
{
    kthread_t tid;
    void *args;
    void *(*f)(void *);
    struct kthread_node *next;
    struct kthread_node *prev;
    sigset_t signals;
    int status;
    jmp_buf env;
    void *return_value;
    unsigned long *stack_top;
    int stack_size;
    int block_join_tid;
} kthread_node;
struct kthread_list
{
    kthread_node *head;
    kthread_node *tail;
    kthread_node *current;
    kthread_node *master;

} kthread_list;
typedef struct attr
{
    int novalue;
} attr;

void init_ll();
void append_ll(kthread_node *k);
void delete_ll(kthread_node *k);
kthread_t *allocate_kthread();
struct kthread_node *allocate_kthread_node();
int kthread_create(kthread_t *k, attr *attr, void *(*f)(void *), void *args);
int kthread_join(kthread_t thread, void **retval);
int kthread_kill(kthread_t thread, int sig);
void kthread_exit();
typedef struct
{
    int lock; /*stores 1 if locked else 0*/
} kthread_mutex_t;
typedef struct kthread_mutexattr_t
{
    int novalue;
} kthread_mutexattr_t;

int kthread_mutex_init(kthread_mutex_t *mutex, const kthread_mutexattr_t *attr);
int kthread_mutex_destroy(kthread_mutex_t *mutex);
int kthread_mutex_lock(kthread_mutex_t *mutex);
int kthread_mutex_unlock(kthread_mutex_t *mutex);
int kthread_mutex_trylock(kthread_mutex_t *mutex);
#endif