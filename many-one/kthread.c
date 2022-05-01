#include "./kthread.h"

static kthread_t next_tid = 0;

struct itimerval timer;
struct sigaction sa;
void raise_signals();
void append_ll(kthread_node *k)
{
    if (!k)
        return;
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
    {
        return NULL;
    }
    kthread_node *p = kthread_list.head;
    kthread_list.head = p->next;
    if (!kthread_list.head)
        kthread_list.tail = NULL;
    else
        kthread_list.head->prev = NULL;
    p->next = p->prev = NULL;
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
    // printf("Timer Started\n");
}

void end_timer()
{
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    // printf("Timer Stopped\n");
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
    // printf("In scheduler\n");
    end_timer();
    // initially master process is marked running in init, which is made to ready state here
    if (kthread_list.current->status == RUNNING)
    {
        kthread_list.current->status = READY;
        // printf("Process -> Ready : %llu\n", kthread_list.current->tid);
    }
    // initially setjmp returns 0 and we run the else part
    if (setjmp(kthread_list.current->env))
    {
        kthread_list.current->status = RUNNING;
        raise_signals();
        // printf("Process -> Running : %llu\n", kthread_list.current->tid);
        return;
    }
    else
    {
        kthread_node *next;
        while (1)
        {
            next = chose_task_from_scheduler();
            if (!next || next->status == READY)
                break;
        }
        if (next)
            kthread_list.current = next;
        begin_timer();
        // printf("Process -> Longjump : %llu\n", kthread_list.current->tid);
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
    // printf("In init\n");
    kthread_list.head = kthread_list.tail = NULL;
    // kthread_list.current = (kthread_node *)malloc(sizeof(kthread_node));

    // setting the main thread
    kthread_list.master = (kthread_node *)malloc(sizeof(kthread_node));
    kthread_list.master->tid = 0;
    kthread_list.master->status = RUNNING;
    kthread_list.master->f = NULL;
    kthread_list.master->args = NULL;
    kthread_list.master->stack_top = NULL;
    kthread_list.master->block_join_tid = -1;
    kthread_list.current = kthread_list.master;
    append_ll(kthread_list.master);

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = scheduler;
    sa.sa_flags = SA_NODEFER;
    sigaction(SIGALRM, &sa, 0);
    begin_timer();
}

void wrapper()
{
    // printf("In wrapper\n");
    begin_timer();
    void *r = kthread_list.current->f(kthread_list.current->args);
    kthread_list.current->return_value = r;

    // printf("retval in wrapper:%d\n", *(int *)kthread_list.current->return_value);
    kthread_exit();
}

int kthread_create(kthread_t *thread, attr *attr, void *(*f)(void *), void *arg)
{
    // printf("in create\n");
    if (!thread || !f)
        return EINVAL;
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
    new_thread->f = f;
    new_thread->args = arg;
    new_thread->block_join_tid = -1;
    new_thread->stack_size = STACK_SIZE;
    new_thread->stack_top = (unsigned long *)malloc(STACK_SIZE);
    append_ll(new_thread);
    if (setjmp(new_thread->env) == 0)
    {
        new_thread->env[0].__jmpbuf[JB_SP] = manglex64((unsigned long)(new_thread->stack_top + (STACK_SIZE - 8) / 8 - 2));
        new_thread->env[0].__jmpbuf[JB_PC] = manglex64((unsigned long)wrapper);
    }

    begin_timer();
    return 0;
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
    if (kthread_list.current)
        printf("in join %lld %lld\n", thread, kthread_list.current->tid);
    end_timer();

    if (thread == 0)
    {
        // printf("Tried to join itself");
        begin_timer();
        // waiting on itself is not a successful operation
        return EINVAL;
    }

    kthread_node *join_thread = search_thread(thread);

    printf("hererwsdfasdf %d\n", join_thread == NULL);
    if (!join_thread)
    {
        // printf("Tried to join non-existing thread");
        begin_timer();
        return EINVAL;
    }
    // kthreadlist.current will be main[function calling the kthread_join]  program's tid[i.e 0]
    join_thread->block_join_tid = kthread_list.current->tid;

    if (join_thread->status == FINISHED)
    {
        // the thread is no longer active, so simply return
        // printf("in join,status=FINISHED for tid: %lld\n", join_thread->tid);
        begin_timer();
        if (retval)
        {
            if (join_thread->return_value)
            {
                *retval = join_thread->return_value;
                // printf("retval in join:%d\n", *(int *)retval);
            }
            else
                *retval = NULL;
        }
        delete_ll(join_thread);
        return 0;
    }
    else
    {
        // changing status of main thread as blocked, waiting for the child thread to complete
        kthread_list.current->status = BLOCKED_JOIN;
        // printf("in join,status=BLOCKED_JOIN for tid: %lld\n", join_thread->tid);
        // pause so that scheduling can be done
        scheduler();
    }
    if (retval)
    {
        if (join_thread->return_value)
        {
            *retval = join_thread->return_value;
            // printf("retval in join:%d\n", *(int *)retval);
        }
        else
            *retval = NULL;
    }
    delete_ll(join_thread);
    return 0;
}

void kthread_exit()
{
    // printf("in kthread exit\n");
    end_timer();

    kthread_list.current->status = FINISHED;
    int block_tid = kthread_list.current->block_join_tid;
    if (block_tid != -1)
    {
        // printf("block_tid %d\n", block_tid);
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
int kthread_kill(kthread_t thread, int sig)
{
    end_timer();
    // printf("in kill\n");
    if (sig < 0 || sig > 64)
    {
        begin_timer();
        return EINVAL;
    }
    // main thread
    if (kthread_list.current->tid == thread)
    {
        raise(sig);
        end_timer();
        return 0;
    }
    kthread_node *thread_to_signal = search_thread(thread);
    if (!thread_to_signal)
    {
        // printf("thread not found\n");
        begin_timer();
        return ESRCH;
    }
    sigaddset(&thread_to_signal->signals, sig);
    // printf("added signal %d\n", sig);
    begin_timer();
    return 0;
}
void raise_signals()
{
    // printf("raising signals\n");
    sigset_t all_signals;
    sigfillset(&all_signals);
    // not possible to block SIGKILL or SIGSTOP.Attempts to do so are silently ignored.
    sigdelset(&all_signals, SIGALRM);
    sigdelset(&all_signals, SIGKILL);
    sigdelset(&all_signals, SIGSTOP);
    sigprocmask(SIG_UNBLOCK, &all_signals, NULL);
    // NSIG is total number of signals defined
    for (int j = 0; j < NSIG; j++)
    {
        if (sigismember(&(kthread_list.current->signals), j))
        {
            raise(j);
            sigdelset(&(kthread_list.current->signals), j);
        }
    }
    return;
}
