#include "./kthread.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define STACK_SIZE 4096
FILE *f;
static kthread_t next_tid = 1;
void append_ll(kthread_node *k)
{
    if (kthread_list.tail == NULL)
    {
        kthread_list.head = kthread_list.tail = k;
    }
    else
    {
        kthread_list.tail->next = k;
        k->prev = kthread_list.tail;
        kthread_list.tail = k;
    }
}
void delete_ll(kthread_node *del)
{
    kthread_node *p = kthread_list.head;
    if (p == NULL || del == NULL)
        return;
    if (kthread_list.head == del)
        kthread_list.head = del->next;
    if (kthread_list.tail == del)
        kthread_list.tail = del->prev;
    if (del->next != NULL)
        del->next->prev = del->prev;
    if (del->prev != NULL)
        del->prev->next = del->next;
    // Free other memory
    free(del);
}
int clone_adjuster(void *node)
{
    kthread_node *t = (kthread_node *)node;
    void *r = t->f(t->args);
    // acquire_lock(&kthread_list.lock);
    t->return_value = r;
    // release_lock(&kthread_list.lock);
    return 0;
}
kthread_node *allocate_kthread_node()
{
    kthread_node *k = (kthread_node *)malloc(sizeof(kthread_node));
    acquire_lock(&kthread_list.lock);
    if (next_tid == 1)
    {
        f = fopen("log.txt", "a+");
    }
    k->tid = next_tid++;
    fprintf(f, "append %lld\n", k->tid);
    release_lock(&kthread_list.lock);
    k->stack = NULL;
    k->next = NULL;
    k->prev = NULL;
    k->return_value = NULL;
    return k;
}
int kthread_create(kthread_t *kt, attr *attr, void *(*f)(void *), void *args)
{
    //  use mmap as memory allocation function passing -1 as file descriptor..
    // MAP_ANONYMOUS + MAP_PRIVATE: ->purpose of using this kind of mapping is to allocate a new zeroized memory
    if (!kt || !f)
        return EINVAL;
    init_lock(&kthread_list.lock);
    if (next_tid == 1)
        kthread_list.head = NULL;
    void *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    kthread_node *k = allocate_kthread_node();
    k->f = f;
    k->args = args;
    k->stack = stack;
    // If CLONE_FS is set, the caller and the child process share the same filesystem information.  This includes the root of the filesystem, the current working directory, and the umask.
    // If CLONE_VM is set, the calling process and the child process run in the same memory space.
    k->kernel_thread_id = clone(clone_adjuster, (void *)(stack + STACK_SIZE), SIGCHLD | CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND, k);
    acquire_lock(&kthread_list.lock);
    append_ll(k);
    release_lock(&kthread_list.lock);
    *kt = k->tid;
    return 0;
}
// wrapper function to satisfy clone's argument types
kthread_node *search_thread(kthread_t tid)
{
    if (!kthread_list.head)
        return NULL;
    kthread_node *p = kthread_list.head;
    while (p)
    {
        if (p->tid == tid)
        {
            return p;
        }
        p = p->next;
    }
    return NULL;
}
int kthread_join(kthread_t thread, void **retval)
{
    if (gettid() == thread || thread == 0)
        return EINVAL;
    // find the thread using tid in LL
    int r, status;
    // fprintf(f, "In kthread_join %llu\n", thread);
    acquire_lock(&kthread_list.lock);
    kthread_node *p = search_thread(thread);
    if (p == NULL)
    {
        // fprintf(f, "Invalid thread id\n");
        release_lock(&kthread_list.lock);
        return EINVAL;
    }
    release_lock(&kthread_list.lock);
    r = waitpid(p->kernel_thread_id, &status, 0);
    if (r == -1)
    {
        perror("Error:");
        return -1;
    }
    acquire_lock(&kthread_list.lock);
    if (retval)
    {
        if (p->return_value)
        {
            fprintf(f, "Return : %d, tid : %lld\n", *(int *)(p->return_value), p->tid);
            *retval = p->return_value;
        }
        else
            *retval = NULL;
    }
    delete_ll(p);
    release_lock(&kthread_list.lock);
    return 0;
}

int kthread_kill(kthread_t tid, int signal_num)
{
    if (signal_num < 0 || signal_num > 64)
        return EINVAL;
    acquire_lock(&kthread_list.lock);
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
        release_lock(&kthread_list.lock);
        return -1;
    }
    int r = kill(p->kernel_thread_id, signal_num);
    if (r == -1)
    {
        perror("thread kill");
        release_lock(&kthread_list.lock);
        return r;
    }
    release_lock(&kthread_list.lock);
    return 0;
}
void delete_all_threads()
{
    kthread_node *p = kthread_list.head;
    while (p != NULL)
    {
        delete_ll(p);
        p = p->next;
    }
    kthread_list.head = kthread_list.tail = NULL;
    next_tid = 1;
}
void kthread_exit(void *return_value)
{
    if (return_value == NULL)
        return;
    pid_t cur_tid = gettid();
    kthread_node *p = kthread_list.head;
    while (p)
    {
        if (p->kernel_thread_id == cur_tid)
            break;
        p = p->next;
    }
    if (p)
    {
        p->return_value = return_value;
        kill(cur_tid, SIGKILL);
        delete_ll(p);
    }
    return;
}
