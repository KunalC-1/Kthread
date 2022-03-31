#include "kthread.h"

#define STACK_SIZE 4096
void *kthread_create(kthread_t *kt, attr *attr, void *(*f)(void *), void *arg)
{
    //  use mmap as memory allocation function passing -1 as file descriptor..
    // MAP_ANONYMOUS + MAP_PRIVATE: ->purpose of using this kind of mapping is to allocate a new zeroized memory
    void *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    struct kthread_node *k = create_kthread_node();
    // If CLONE_FS is set, the caller and the child process share the same filesystem information.  This includes the root of the filesystem, the current working directory, and the umask.
    k->thread->kernel_thread_id = clone(....., f, (void *)(stack + STACK_SIZE), CLONE_FS | CLONE_FILES | CLONE_VM);
    append_ll(k);
}
// wrapper function to satisfy clone's argument types
int clone_adjuster(void *thread)
{
    kthread_t *t = (kthread_t *)thread;
    t->return_value = t->f();
    return 0;
}
void append_ll(struct kthread_node *k)
{
    if (kthread_list.head == NULL)
    {
        kthread_list.head = k;
    }
    else
    {
        struct kthread_node *p = kthread_list.head;
        while (p->next)
        {
            p = p->next;
        }
        p->next = k;
    }
}
void init_ll()
{
    kthread_list.head = NULL;
}
kthread_t *create_kthread()
{
    kthread_t *thread = (kthread_t *)malloc(sizeof(kthread_t));
    // Initialize
    thread->tid = next_tid++;
    thread->stack = NULL;
    return thread;
}
struct kthread_node *create_kthread_node()
{
    struct kthread_node *k = (struct kthread_node *)malloc(sizeof(struct kthread_node));
    k->thread = create_kthread();
    k->next = NULL;
}
int kthread_join(kthread_t thread, void **retval)
{
    // find the thread using tid in LL
    struct kthread_node *p = kthread_list.head;
    while (p != NULL)
    {
        if (p->thread->tid == thread.tid)
        {
            break;
        }
    }
    if (p == NULL)
        return -1;
    int status;
    waitpid(p->thread->kernel_thread_id, &status, 0);
}