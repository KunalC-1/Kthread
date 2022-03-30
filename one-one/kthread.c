#include "kthread.h"

#define STACK_SIZE 4096
void *kthread_create(kthread_t *kt, attr *attr, void *(*f)(void *), void *arg)
{
    //  use mmap as memory allocation function passing -1 as file descriptor..
    void *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    struct kthread_node *k = create_kthread_node();
    // If CLONE_FS is set, the caller and the child process share the same filesystem information.  This includes the root of the filesystem, the current working directory, and the umask.
    k->thread->kernel_thread_id = clone(....., f, (void *)(stack + STACK_SIZE), CLONE_FS | CLONE_FILES | CLONE_VM);
    append_ll(k);
}
int clone_adjuster(void *thread)
{
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
