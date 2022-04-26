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
#define MAX_KERNEL_THREAD 10
#define STACK_SIZE 40960
#define JB_SP 6
#define JB_PC 7

typedef unsigned long long int kthread_t;
typedef struct kthread_node
{
    kthread_t tid;
    int pid;
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
    int pid;
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

#endif