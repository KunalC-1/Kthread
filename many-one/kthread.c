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
    timer.it_value.tv_usec = 50000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 50000;
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

void kthread_init()
{
    next_tid = 0;
    kthread_list.head = kthread_list.tail = NULL;
    kthread_list.current = (kthread_node *)malloc(sizeof(kthread_node));

    // setting the main thread
    kthread_list.master = (kthread_node *)malloc(sizeof(kthread_node));
    kthread_list.master->tid = 0;
    kthread_list.master->status = RUNNING;
    kthread_list.master->f = NULL;
    kthread_list.master->args = NULL;
    kthread_list.master->stack_top = NULL;
    kthread_list.master->block_join_tid = -1;
    kthread_list.current = kthread_list.master;
    // push_array(thread_list, kthread_list.master);
    append_ll(kthread_list.master);

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = scheduler;
    sa.sa_flags = SA_NODEFER;
    sigaction(SIGALRM, &sa, 0);
    begin_timer();
}

void wrapper()
{
    begin_timer();
    void *r = kthread_list.current->f(kthread_list.current->args);
    kthread_list.current->return_value = r;
    // put exit thread here
    kthread_exit();
}

int kthread_create(kthread_t *thread, attr *attr, void *(*function)(void *), void *arg)
{
    end_timer();
    if (next_tid == 0)
    {
        kthread_init();
    }
    kthread_node *new_thread;
    new_thread = (kthread_node *)malloc(sizeof(kthread_node));
    new_thread->tid = ++next_tid; // tID starts from 1
    *thread = next_tid;
    new_thread->status = READY;
    new_thread->f = function;
    new_thread->args = arg;
    new_thread->block_join_tid = -1;
    new_thread->stack_size = STACK_SIZE;
    new_thread->stack_top = (unsigned long *)malloc(STACK_SIZE);
    // push_array(thread_list, new_thread);
    append_ll(new_thread);
    if (setjmp(new_thread->env) == 0)
    {
        new_thread->env[0].__jmpbuf[JB_SP] = manglex64((unsigned long)(new_thread->stack_top + (STACK_SIZE - 8) / 8 - 2));
        new_thread->env[0].__jmpbuf[JB_PC] = manglex64((unsigned long)wrapper);
    }

    begin_timer();
}

// Give up the CPU and allow the next thread to run.
void kthread_yield()
{
    end_timer();
    kthread_list.current->status = READY;
    begin_timer();
    scheduler();
}

// The calling thread will not continue until the thread with tid threadhas finished executing.

int kthread_join(kthread_t thread, void **retval)
{
    end_timer();

    if (thread == kthread_list.current->tid)
    {
        // waiting on itself is not a successful operation
        return -1;
    }

    kthread_node *join_thread = search_thread(thread);
    if (!join_thread)
    {
        printf("Tried to join non-existing thread");
        exit(0);
    }

    join_thread->block_join_tid = kthread_list.current->tid;

    if (join_thread->status == FINISHED)
    {
        // the thread is no longer active, so simply return
        begin_timer();
        if (retval)
            *retval = join_thread->return_value;
        return 0;
    }
    else
    {
        kthread_list.current->status = BLOCKED_JOIN;
        begin_timer();
        // pause so that scheduling can be done
        scheduler();
    }
    if (retval)
        *retval = join_thread->return_value;
    return 0;
}

void kthread_exit()
{
    end_timer();

    kthread_list.current->status = FINISHED;
    int block_tid = kthread_list.current->block_join_tid;
    if (block_tid != -1)
    {
        kthread_node *block_thread = search_thread(block_tid);
        if (block_thread)
            block_thread->status = READY;
    }
    begin_timer();
    scheduler();
}

int kthread_cancel(kthread_t thread)
{
    end_timer();

    if (thread == kthread_list.current->tid)
    {
        // If it is the kthread_list.current thread, reschedule.
        begin_timer();
        scheduler();
        return 1;
    }

    kthread_node *t = search_thread(thread);
    if (!t)
        return 1;

    kthread_list.current->status = CANCELLED;
    int block_tid = kthread_list.current->block_join_tid;
    if (block_tid != -1)
    {
        kthread_node *block_thread = search_thread(block_tid);
        if (block_thread)
            block_thread->status = READY;
    }
    begin_timer();
    scheduler();
    return 0;
}
