#include "./kthread.h"

int next_tid = 0;

struct itimerval timer;
struct sigaction sa;

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

kthread_node *dequeue_ll()
{
    if (!kthread_list.head)
        return NULL;
    kthread_node *p = kthread_list.head;
    kthread_list.head = p->next;
    if (kthread_list.tail == p)
        kthread_list.tail = p->prev;
    return p;
}

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
// round-robin implementation
kthread_node *chose_task_from_scheduler()
{
    kthread_node *t = dequeue_ll();
    append_ll(t);
    return t;
}

void begin_timer()
{
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 500000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 500000;
    setitimer(ITIMER_REAL, &timer, NULL);
    printf("Timer Started\n");
}

void end_timer()
{
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

void scheduler()
{
    end_timer();
    if (kthread_list.current->status == RUNNING)
    {
        kthread_list.current->status = READY;
        printf("Process -> Ready : %llu\n", kthread_list.current->tid);
    }
    if (setjmp(kthread_list.current->env))
    {
        kthread_list.current->status = RUNNING;
        printf("Process -> Running : %llu\n", kthread_list.current->tid);
        return;
    }
    else
    {
        kthread_node *next;
        while (1)
        {
            next = chose_task_from_scheduler();
            if (next->status == READY)
                break;
        }
        kthread_list.current = next;
        begin_timer();
        printf("Process -> Longjump : %llu\n", kthread_list.current->tid);
        longjmp(kthread_list.current->env, 1);
    }
}

void delete_all_threads()
{
    end_timer();
    kthread_node *p = kthread_list.head;
    while (p != NULL)
    {
        delete_ll(p);
        p = p->next;
    }
    kthread_list.head = kthread_list.tail = NULL;
    next_tid = 0;
}
