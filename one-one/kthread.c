#include "kthread.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
    k->kernel_thread_id = clone(clone_adjuster, (void *)(stack + STACK_SIZE), SIGCHLD | CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND, k);
    append_ll(k);
    *kt = k->tid;
}
// wrapper function to satisfy clone's argument types
FILE *f;
void init_ll()
{
    kthread_list.head = NULL;
    f = fopen("log.txt", "a+");
}

int kthread_join(kthread_t thread, void **retval)
{
    // find the thread using tid in LL
    int r, status;
    fprintf(f, "In kthread_join %llu\n", thread);
    kthread_node *p = kthread_list.head;
    while (p != NULL)
    {
        if (p->tid == thread)
        {
            break;
        }
        p = p->next;
    }
    if (p == NULL)
    {
        fprintf(f, "Invalid thread id\n");
        return -1;
    }
    r = waitpid(p->kernel_thread_id, &status, 0);
    if (r == -1)
    {
        perror("Error:");
        return -1;
    }
    fprintf(f, "Wait Return : %d\n", r);

    *retval = p->return_value;
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
        p = p->next;
    }
    if (p == NULL)
    {
        fprintf(f, "Invalid thread id\n");
        return -1;
    }
    if (kill(p->kernel_thread_id, sig) < 0)
    {
        perror("Error in kthread_kill");
        return -1;
    }
    delete_ll(p);
}
