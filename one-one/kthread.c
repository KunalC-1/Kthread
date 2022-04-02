#include "kthread.h"

#define STACK_SIZE 4096

void append_ll(kthread_node *k)
{
    if (kthread_list.head == NULL)
    {
        kthread_list.head = k;
    }
    else
    {
        kthread_node *p = kthread_list.head;
        while (p->next)
        {
            p = p->next;
        }
        p->next = k;
        k->prev = p;
    }
}
void delete_ll(kthread_node *del)
{
    kthread_node *p = kthread_list.head;
    if (p == NULL || del == NULL)
        return;
    if (p == del)
        p = del->next;
    if (del->next != NULL)
        del->next->prev = del->prev;
    if (del->prev != NULL)
        del->prev->next = del->next;
    free(del);
    // Free other memory
}
int clone_adjuster(void *node)
{
    kthread_node *t = (kthread_node *)node;
    t->return_value = t->f(t->args);
    return 0;
}
kthread_node *allocate_kthread_node()
{
    static kthread_t next_tid = 1;
    if (next_tid == 1)
        init_ll();
    kthread_node *k = (kthread_node *)malloc(sizeof(kthread_node));
    k->tid = next_tid++;
    k->stack = NULL;
    k->next = NULL;
    k->prev = NULL;
    k->return_value = NULL;
    return k;
}
void *kthread_create(kthread_t *kt, attr *attr, void *(*f)(void *), void *args)
{
    //  use mmap as memory allocation function passing -1 as file descriptor..
    // MAP_ANONYMOUS + MAP_PRIVATE: ->purpose of using this kind of mapping is to allocate a new zeroized memory
    void *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    kthread_node *k = allocate_kthread_node();
    k->f = f;
    k->args = args;
    k->stack = stack;
    // If CLONE_FS is set, the caller and the child process share the same filesystem information.  This includes the root of the filesystem, the current working directory, and the umask.
    // If CLONE_VM is set, the calling process and the child process run in the same memory space.
    k->kernel_thread_id = clone(clone_adjuster, (void *)(stack + STACK_SIZE), CLONE_FS | CLONE_FILES | CLONE_VM, k);
    append_ll(k);
    *kt = k->tid;
}
// wrapper function to satisfy clone's argument types

void init_ll()
{
    kthread_list.head = NULL;
}

int kthread_join(kthread_t thread, void **retval)
{
    // find the thread using tid in LL
    printf("In kthread_join\n");
    kthread_node *p = kthread_list.head;
    while (p != NULL)
    {
        if (p->tid == thread)
        {
            break;
        }
    }
    if (p == NULL)
        return -1;
    // printf("Tid : %d , %d", p->tid, p->kernel_thread_id);
    int status;
    waitpid(p->kernel_thread_id, &status, 0);
    *retval = p->return_value;
    // printf("Return Val : %d", *((int *)*retval));
    delete_ll(p);
}

int kthread_kill(kthread_t tid, int sig)
{
    kthread_node *p = kthread_list.head;
    while (p != NULL)
    {
        if (p->tid == tid)
        {
            break;
        }
    }
    if (p == NULL)
    {
        perror("Invalid thread id\n");
        return -1;
    }
    if (kill(tid, sig) < 0)
    {
        perror("Error in kthread_kill\n");
        return -1;
    }
    delete_ll(p);
}
