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
#include <sys/resource.h>
#include <setjmp.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include "spinlock.h"
#define MAX_KERNEL_THREAD 50
#define KERNEL_THREAD_STACK_SIZE 40960
#define THREAD_STACK_SIZE 40960
#define JB_SP 6
#define JB_PC 7

typedef unsigned long long int kthread_t;
typedef struct kthread_node
{
    kthread_t tid;
    int k_tid;
    void *args;
    void *(*f)(void *);
    struct kthread_node *next;
    struct kthread_node *prev;
    jmp_buf env;
    void *return_value;
    unsigned long *stack_top;
    int stack_size;
} kthread_node;

typedef struct kernel_thread
{
    jmp_buf env;
    int k_tid;
    // which thread is running on kernel thread
    kthread_node *current;
} kernel_thread;
typedef struct kthread_list
{
    kthread_node *head;
    kthread_node *tail;
} kthread_list;
typedef struct attr
{
    int novalue;
} attr;
void init_q(kthread_list *list);
void enqueue_ll(kthread_list *list, kthread_node *k);
void delete_ll(kthread_list *list, kthread_node *del);
kthread_node *dequeue_by_id_ll(kthread_list *list, kthread_t id, int ispid);
kthread_node *dequeue_ll(kthread_list *list);
kthread_node *search_thread(kthread_list *list, kthread_t id, int ispid);
kernel_thread *search_kernel_thread(int k_tid);
void begin_timer();
void end_timer();
void init_timer();
void kthread_init();
void wrapper();
int thread_runner(void *args);
void scheduler();
int kthread_create(kthread_t *thread, attr *attr, void *(*f)(void *), void *arg);
int kthread_join(kthread_t thread, void **retval);
void kthread_exit();

#endif