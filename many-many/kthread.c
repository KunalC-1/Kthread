#include "./kthread.h"

static kthread_t next_tid = 0;
int count_kernel_threads = 0;
kernel_thread all_kernel_threads[MAX_KERNEL_THREAD];
kthread_list ready;
kthread_list terminated;
void init_ll(kthread_list *list)
{
    list->head = list->tail = NULL;
}
void append_ll(kthread_list *list, kthread_node *k)
{
    if (list->tail == NULL)
    {
        list->head = list->tail = k;
    }
    else
    {
        list->tail->next = k;
        k->prev = list->tail;
        list->tail = k;
    }
}
void delete_ll(kthread_list *list, kthread_node *del)
{
    if (list->head == NULL || del == NULL)
        return;
    if (list->head == del)
        list->head = del->next;
    if (list->tail == del)
        list->tail = del->prev;
    if (del->next != NULL)
        del->next->prev = del->prev;
    if (del->prev != NULL)
        del->prev->next = del->next;
    // Free other memory
    free(del);
}
kthread_node *remove_by_tid_ll(kthread_list *list, kthread_t tid)
{
    if (list->head == NULL)
        return NULL;
    kthread_node *p = search_thread(list, tid, 0);
    if (p == NULL)
        return NULL;
    if (list->head == p)
        list->head = p->next;
    if (list->tail == p)
        list->tail = p->prev;
    if (p->next != NULL)
        p->next->prev = p->prev;
    if (p->prev != NULL)
        p->prev->next = p->next;
    return p;
}
kthread_node *remove_by_pid_ll(kthread_list *list, int pid)
{
    if (list->head == NULL)
        return NULL;
    kthread_node *p = search_thread(list, pid, 1);
    if (p == NULL)
        return NULL;
    if (list->head == p)
        list->head = p->next;
    if (list->tail == p)
        list->tail = p->prev;
    if (p->next != NULL)
        p->next->prev = p->prev;
    if (p->prev != NULL)
        p->prev->next = p->next;
    return p;
}
kthread_node *dequeue_ll(kthread_list *list)
{
    if (!list->head)
        return NULL;
    kthread_node *p = list->head;
    list->head = p->next;
    if (list->tail == p)
        list->tail = p->prev;
    return p;
}

kthread_node *search_thread(kthread_list *list, kthread_t id, int ispid)
{
    if (!list->head)
        return NULL;
    kthread_node *p = list->head;
    while (p)
    {
        if (ispid)
        {
            if (p->pid == id)
            {
                return p;
            }
        }
        else
        {
            if (p->tid == id)
            {
                return p;
            }
        }
        p = p->next;
    }
    return NULL;
}
kernel_thread *search_kernel_thread(int pid)
{
    for (int i = 0; i < count_kernel_threads; i++)
    {
        if (all_kernel_threads[i].pid == pid)
            return &all_kernel_threads[i];
    }
    return NULL;
}

void begin_timer()
{
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 50000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 50000;
    setitimer(ITIMER_REAL, &timer, NULL);
    printf("Timer Started\n");
}

void end_timer()
{
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    printf("Timer Stopped\n");
}
// These machines are equipped with a libc that includes a security feature to protect the addresses stored in jump envfers.
// This security feature "mangles" (i.e., encrypts) a pointer before saving it in a jmp_env. Thus, we also have to mangle our new stack pointer and program counter before we can write it into a jump envfer, otherwise decryption (and subsequent uses) will fail.
// To mangle a pointer before writing it into the jump envfer, make use of the following function:
static long int manglex64(long int p)
{
    long int ret;
    asm(" mov %1, %%rax;\n"
        " xor %%fs:0x30, %%rax;"
        " rol $0x11, %%rax;"
        " mov %%rax, %0;"
        : "=r"(ret)
        : "r"(p)
        : "%rax");
    return ret;
}
void timer_handler()
{
}
void init_timer()
{
    struct itimerval timer;
    struct sigaction sa;
    sigset_t sig;
    sigemptyset(&sig);
    sigaddset(&sig, SIGALRM);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler;
    sa.sa_flags = SA_RESTART;
    sa.sa_mask = sig;
    sigaction(SIGALRM, &sa, 0);
}

void kthread_init()
{
    printf("In init\n");
    init_ll(&ready);
    init_ll(&terminated);
}