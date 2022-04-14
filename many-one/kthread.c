#include "kthread.h"

struct itimerval timer;
struct sigaction sa;
void scheduler()
{
    if (setjmp(SCHEDULER) == 0)
        longjmp(MAIN, 1);
    kthread_list.current = kthread_list.current->next;
    longjmp(kthread_list.current->env, 1);
}
void thread_yield()
{
    if (setjmp(kthread_list.current->env) == 0)
        longjmp(SCHEDULER, 1);
}
void *kthread_create(kthread_t *kt, attr *attr, void *(*f)(void *), void *args)
{
    kthread_node *k = allocate_kthread_node();
    k->f = f;
    k->args = args;
    // If CLONE_FS is set, the caller and the child process share the same filesystem information.  This includes the root of the filesystem, the current working directory, and the umask.
    // If CLONE_VM is set, the calling process and the child process run in the same memory space.
    acquire_lock(&kthread_list.lock);
    append_ll(k);
    release_lock(&kthread_list.lock);

    *kt = k->tid;
}
void myThread_init()
{
    static int cur_tID = 0;
    kthread_list.current = (kthread_node *)malloc(sizeof(kthread_node));
    // setting the main thread
    kthread_list.master = (kthread_node *)malloc(sizeof(kthread_node));
    kthread_list.master->tid = 0;
    kthread_list.master->status = RUNNING;
    kthread_list.master->f = NULL;
    kthread_list.master->args = NULL;
    kthread_list.master->stackPointer = NULL;
    kthread_list.master->blocked_join_on_tid = -1;
    kthread_list.current = kthread_list.master;
    // push_array(thread_list, kthread_list.master);

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = timer_handler;
    sa.sa_flags = SA_NODEFER;
    sigaction(SIGALRM, &sa, 0);

    timer.it_value.tv_sec = INTERVAL / 1000;
    timer.it_value.tv_usec = (INTERVAL * 1000) % 1000000;
    timer.it_interval = timer.it_value;
    start_time();
}