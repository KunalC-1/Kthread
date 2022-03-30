#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/syscall.h>

struct kthread_node
{
    struct kthread_t *thread;
    struct kthread_node *next;
} kthread_node;
struct kthread_list
{
    struct kthread_node *head;
} kthread_list;
typedef struct kthread_t
{
    int tid;
    void *stack;
    void *(*f)(void *);
    int kernel_thread_id;
} kthread_t;
typedef struct attr
{
    int novalue;
} attr;
int next_tid = 1;
void init_ll();
kthread_t *create_kthread();
struct kthread_node *create_kthread_node();
void *kthread_create(kthread_t *k, attr *attr, void *(*f)(void *), void *arg);
