#include "./kthread.h"
static kthread_t next_tid = 0;
int count_kernel_threads = 5;
FILE *f;
kernel_thread all_kernel_threads[MAX_KERNEL_THREAD];
void *kernel_thread_stacks[MAX_KERNEL_THREAD];
int exit_all = 0;
int start_all = 0;
spinlock_t listlock;
spinlock_t terminatedlock;
void init_q()
{
    kthread_list.head = kthread_list.tail = NULL;
}
void enqueue_ll(kthread_node *k)
{
    if (kthread_list.head == NULL)
    {
        kthread_list.head = kthread_list.tail = k;
        k->prev = k->next = NULL;
    }
    else
    {
        kthread_list.tail->next = k;
        k->prev = kthread_list.tail;
        kthread_list.tail = k;
        k->next = NULL;
    }
}
void delete_ll(kthread_node *del)
{
    if (kthread_list.head == NULL || del == NULL)
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
kthread_node *dequeue_by_id_ll(kthread_t id, int ispid)
{
    if (kthread_list.head == NULL)
    {
        return NULL;
    }
    kthread_node *p = search_thread(id, ispid);
    if (p == NULL)
    {
        return NULL;
    }
    if (kthread_list.head == p)
        kthread_list.head = p->next;
    if (kthread_list.tail == p)
        kthread_list.tail = p->prev;
    if (p->next != NULL)
        p->next->prev = p->prev;
    if (p->prev != NULL)
        p->prev->next = p->next;
    p->next = p->prev = NULL;
    return p;
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

kthread_node *search_thread(kthread_t id, int ispid)
{
    if (!kthread_list.head)
        return NULL;
    kthread_node *p = kthread_list.head;
    while (p)
    {
        if (ispid)
        {
            if (p->k_tid == id)
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
kernel_thread *search_kernel_thread(int k_tid)
{
    for (int i = 0; i < count_kernel_threads; i++)
    {
        if (all_kernel_threads[i].k_tid == k_tid)
            return &all_kernel_threads[i];
    }
    return NULL;
}
int is_queue_empty()
{
    return !kthread_list.head;
}
void begin_timer()
{
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 100000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 100000;
    setitimer(ITIMER_REAL, &timer, NULL);
}

void end_timer()
{
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
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
void init_timer()
{
    // struct itimerval timer;
    struct sigaction sa;
    sigset_t sig;
    sigemptyset(&sig);
    sigaddset(&sig, SIGALRM);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &scheduler;
    sa.sa_mask = sig;
    /*This flag controls what happens when a signal is delivered during certain primitives (such as open, read or write), and the signal handler returns normally. There are two alternatives: the library function can resume, or it can return failure with error code EINTR.
    The choice is controlled by the SA_RESTART flag for the particular kind of signal that was delivered. If the flag is set, returning from a handler resumes the library function. If the flag is clear, returning from a handler makes the function fail*/
    sa.sa_flags = SA_RESTART;
    // mask signal SIGALRM for scheduler
    sigaction(SIGALRM, &sa, 0);
}

void kthread_init()
{
    // printf("In init\n");
    // f = fopen("log.txt", "a+");
    init_q();
    init_lock(&listlock);
    for (int i = 0; i < count_kernel_threads; i++)
        kernel_thread_stacks[i] = malloc(KERNEL_THREAD_STACK_SIZE);
    for (int i = 0; i < count_kernel_threads && i < MAX_KERNEL_THREAD; i++)
    {
        int kernel_tid = clone(thread_runner, kernel_thread_stacks[i] + (KERNEL_THREAD_STACK_SIZE), SIGCHLD | CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND, NULL);
        if (kernel_tid == -1)
        {
            exit(0);
        }
        all_kernel_threads[i].current = NULL;
        all_kernel_threads[i].k_tid = kernel_tid;
    }
    start_all = 1;
}
void wrapper(int signum)
{
    // fprintf(f, "In wrapper\n");
    kernel_thread *kt = search_kernel_thread(gettid());
    if (!kt)
        exit(0);
    kthread_node *cur = kt->current;
    void *(*fun)(void *) = cur->f;
    void *args = cur->args;
    // printf("thread starting : %lld , pid : %d, mypid : %d, kernel tid : %d\n", kt->current->tid, gettid(), kt->current->k_tid, kt->k_tid);
    // fprintf(f, "thread starting : %lld , pid : %d, mypid : %d\n", kt->current->tid, gettid(), kt->current->k_tid);
    begin_timer();
    void *r = fun(args);
    end_timer();
    cur->return_value = r;
    kt = search_kernel_thread(gettid());
    // acquire_lock(&listlock);
    kt->current->status = FINISHED;
    // release_lock(&listlock);
    kt->current = NULL;
    // printf("kernel thread in wrapper : %d , pid : %d\n", kt->k_tid, gettid());
    // printf("Long Jmp kernel thread: %d , pid : %d\n", kt->k_tid, gettid());
    longjmp(kt->env, 1);
}
int thread_runner(void *args)
{
    // fprintf(f, "Thread Runner started : %d \n", gettid());
    init_timer();
    kernel_thread *kt = NULL;
    kthread_node *next;
    while (!start_all)
        ;
    kt = search_kernel_thread(gettid());
    while (1)
    {
        acquire_lock(&listlock);
        next = kthread_list.head;
        while (next)
        {
            if (next->status == READY)
            {
                break;
            }
            next = next->next;
        }
        if (next)
        {
            kt->current = next;
            kt->current->status = RUNNING;
            kt->current->k_tid = gettid();
            if (kthread_list.head == next)
                kthread_list.head = next->next;
            if (kthread_list.tail == next)
                kthread_list.tail = next->prev;
            if (next->next != NULL)
                next->next->prev = next->prev;
            if (next->prev != NULL)
                next->prev->next = next->next;
            next->next = next->prev = NULL;
            enqueue_ll(next);
            if (!setjmp(kt->env))
            {
                release_lock(&listlock);
                longjmp(kt->current->env, 1);
                // fprintf(f, "Long Jmp from thread runner to local thread: %lld , pid : %d\n", kt->current->tid, gettid());
            }
            else
                acquire_lock(&listlock);

            // printf("Returned successfully, %d\n", kt->k_tid);
        }
        release_lock(&listlock);
        if (exit_all)
        {
            break;
        }
    }
    return 0;
}

void scheduler()
{
    // fprintf(f, "in hadle alarm for tid : %d\n", gettid());
    // printf("in hadle alarm for tid : %d\n", gettid());
    kernel_thread *kt = search_kernel_thread(gettid());
    /* Storing the current context */

    if (setjmp(kt->current->env))
    {
        raise_signals(kt->current);
        return;
    }
    else
    {
        acquire_lock(&listlock);
        kthread_node *next = kthread_list.head;
        while (next)
        {
            if (next->status == READY)
            {
                break;
            }
            next = next->next;
        }
        if (next)
        {

            if (kt->current->status == RUNNING)
            {
                kt->current->status = READY;
                kt->current->k_tid = 0;
            }
            kt->current = next;
            kt->current->status = RUNNING;
            kt->current->k_tid = gettid();
            if (kthread_list.head == next)
                kthread_list.head = next->next;
            if (kthread_list.tail == next)
                kthread_list.tail = next->prev;
            if (next->next != NULL)
                next->next->prev = next->prev;
            if (next->prev != NULL)
                next->prev->next = next->next;
            next->next = next->prev = NULL;
            enqueue_ll(next);
        }
        release_lock(&listlock);
        if (kt->current->status == RUNNING)
            longjmp(kt->current->env, 1);
        else
        {
            kt->current = NULL;
            longjmp(kt->env, 1);
        }
    }
}
int kthread_create(kthread_t *thread, attr *attr, void *(*fun)(void *), void *arg)
{
    if (!thread || !fun)
        return EINVAL;
    if (next_tid == 0)
    {
        kthread_init();
    }
    // fprintf(f, "in thread create\n");

    kthread_node *new_thread;
    new_thread = (kthread_node *)malloc(sizeof(kthread_node));
    new_thread->tid = ++next_tid; // tID starts from 1
    if (thread)
        *thread = next_tid;
    new_thread->f = fun;
    new_thread->args = arg;
    new_thread->next = new_thread->prev = NULL;
    new_thread->stack_size = THREAD_STACK_SIZE;
    new_thread->stack_top = malloc(THREAD_STACK_SIZE) + THREAD_STACK_SIZE;
    if (setjmp(new_thread->env) == 0)
    {
        new_thread->env[0].__jmpbuf[JB_SP] = manglex64((unsigned long)(new_thread->stack_top));
        new_thread->env[0].__jmpbuf[JB_PC] = manglex64((unsigned long)wrapper);
    }
    acquire_lock(&listlock);
    enqueue_ll(new_thread);
    release_lock(&listlock);
    // printf("Exit from thread_Create\n");
    return 0;
}
int is_current_running()
{
    for (int i = 0; i < count_kernel_threads; i++)
    {
        if (all_kernel_threads[i].current)
            return 1;
    }
    return 0;
}
int kthread_join(kthread_t tid, void **retval)
{
    kthread_node *p = NULL;
    acquire_lock(&listlock);
    p = search_thread(tid, 0);
    release_lock(&listlock);
    if (!p)
        return EINVAL;
    while (p->status != FINISHED)
        ;
    // printf("found terminated thread with id : %lld\n", p->tid);
    // fprintf(f, "found terminated thread with id : %lld\n", p->tid);
    if (retval)
    {
        if (p->return_value)
        {
            // fprintf(f, "Return : %d, tid : %lld\n", *(int *)(p->return_value), p->tid);
            // printf("Return : %d, tid : %lld\n", *(int *)(p->return_value), p->tid);
            *retval = p->return_value;
        }
        else
            *retval = NULL;
    }
    delete_ll(p);
    // acquire_lock(&listlock);
    // acquire_lock(&terminatedlock);
    // if (is_queue_empty(kthread_list) && is_queue_empty(terminated) && !is_current_running())
    // {
    //     exit_all = 1;
    //     for (int i = 0; i < count_kernel_threads; i++)
    //     {
    //         waitpid(all_kernel_threads[i].k_tid, NULL, 0);
    //     }
    // }
    // release_lock(&terminatedlock);
    // release_lock(&listlock);
    return 0;
}

void kthread_exit(void *return_value)
{
    if (return_value == NULL)
        return;
    kthread_node *t = search_thread(gettid(), 1);
    t->return_value = return_value;
    if (!t)
        return;
    t->status = FINISHED;
    t->k_tid = 0;
    scheduler();
}

void kthread_exit1()
{
    kthread_node *t = search_thread(gettid(), 1);
    if (!t)
        return;
    t->status = FINISHED;
    t->k_tid = 0;
    scheduler();
}
int kthread_kill(kthread_t thread, int sig)
{
    if (sig < 0 || sig > 64)
    {
        return EINVAL;
    }
    acquire_lock(&listlock);
    kthread_node *thread_to_signal = search_thread(thread, 0);
    if (!thread_to_signal)
    {
        release_lock(&listlock);
        return EINVAL;
    }
    if (thread_to_signal->k_tid != 0 && sig != SIGKILL && sig != SIGINT && sig != SIGTERM && sig != SIGSTOP)
    {
        raise(sig);
    }
    else
    {

        sigaddset(&thread_to_signal->signals, sig);
    }
    release_lock(&listlock);
    return 0;
}
void raise_signals(kthread_node *t)
{
    // NSIG is total number of signals defined
    for (int j = 0; j < NSIG; j++)
    {
        if (sigismember(&(t->signals), j))
        {
            if (j == SIGKILL || j == SIGINT || j == SIGTERM)
            {
                kthread_exit1();
            }
            else if (j == SIGSTOP)
            {
                // printf("stopped\n");
                t->status = STOPPED;
                // so scheduler won't schedule this now
            }
            else if (j == SIGCONT)
            {
                // printf("sig cont\n");
                t->status = READY;
            }
            else
            {
                raise(j);
            }
            sigdelset(&(t->signals), j);
        }
    }
    return;
}