#include "acutest.h"
#include "../kthread.h"
int counting;
void *thread1()
{
    int x = 0;
    for (int i = 0; i < 18; i++)
    {
        x += i;
    }
    return NULL;
}
void *thread2(void *arg)
{
    int x = *((int *)arg);
    for (int i = 0; i < 18; i++)
    {
        x += i;
    }
    return NULL;
}
void *thread3(void *arg)
{
    int *x = (int *)malloc(sizeof(int));
    *x = *((int *)arg);
    for (int i = 0; i < 18; i++)
    {
        *x += i;
    }
    return x;
}
void *thread4()
{
    int *i = (int *)malloc(sizeof(int));
    *i = 0;
    while (counting)
    {
        i++;
    };
    return i;
}
void *thread5()
{
    while (1)
        ;
    return NULL;
}
void *thread6()
{
    int r = 899;
    kthread_exit(&r);
    return NULL;
}
void sigusr1_handler()
{
    counting = 0;
}

void test_invalid_arguments()
{
    kthread_t tid;
    TEST_CHECK_(kthread_create(NULL, NULL, NULL, NULL) == EINVAL, "kthread_create(NULL, NULL, NULL, NULL)");
    TEST_CHECK_(kthread_create(NULL, NULL, thread1, NULL) == EINVAL, "kthread_create(NULL, NULL, thread1, NULL)");
    TEST_CHECK_(kthread_create(&tid, NULL, NULL, NULL) == EINVAL, "kthread_create(&tid, NULL, NULL, NULL)");
}

void test_without_attributes()
{
    kthread_t tid;
    TEST_CHECK_(kthread_create(&tid, NULL, thread1, NULL) == 0, "kthread_create(&tid, NULL, thread1, NULL)");
    TEST_CHECK_(kthread_join(tid, NULL) == 0, "kthread_join(tid, NULL) : %lld", tid);
}
void test_thread_create()
{
    kthread_t tid[30];
    int t, num = 20;

    for (int i = 0; i < 30; i++)
    {
        t = kthread_create(&tid[i], NULL, thread2, (void *)&num);
        TEST_CHECK_(t == 0, "Thread created with thread id : %lld", tid[i]);
    }
}
void test_invalid_join()
{
    kthread_t tid = -1;
    int *r;
    TEST_CHECK_(kthread_join(0, NULL) == EINVAL, "Join with null arguments");
    TEST_CHECK_(kthread_join(tid, NULL) == EINVAL, "Join with invalid thread Id : %lld", tid);
    kthread_create(&tid, NULL, thread1, NULL);
    kthread_join(tid, NULL);
    TEST_CHECK_(kthread_join(tid, (void **)&r) == EINVAL, "Join already joined thread");
}
void test_thread_join()
{
    kthread_t tid[8];
    int *r, t, num = 5;
    int arg[] = {10, 15, 20};
    printf("\n\t\033[1m1. Joining threads immediately after creation\033[0m\n\n");
    for (int i = 0; i < 8; i++)
    {
        t = kthread_create(&tid[i], NULL, thread2, (void *)&num);
        TEST_CHECK_(t == 0, "Thread created with thread id : %lld", tid[i]);
        TEST_CHECK_(kthread_join(tid[i], (void **)&r) == 0, "Join thread with id  : %lld", tid[i]);
    }
    printf("\n\t\033[1m2. Joining threads collectively after creation\033[0m\n\n");
    for (int i = 0; i < 8; i++)
    {
        t = kthread_create(&tid[i], NULL, thread2, (void *)&num);
        TEST_CHECK_(t == 0, "Thread created with thread id : %lld", tid[i]);
    }
    for (int i = 0; i < 8; i++)
        TEST_CHECK_(kthread_join(tid[i], (void **)&r) == 0, "Join thread with id  : %lld", tid[i]);
    printf("\n\t\033[1m3. Joining thread and Checking return value of thread\033[0m\n\n");
    for (int i = 0; i < 3; i++)
    {
        t = kthread_create(&tid[i], NULL, thread3, (void *)&arg[i]);
        TEST_CHECK_(t == 0, "Thread created : %lld with function argument : %d", tid[i], i);
    }
    int *ret[3];
    for (int i = 0; i < 3; i++)
    {
        kthread_join(tid[i], (void **)&ret[i]);
        int *ans = (int *)(thread3(&arg[i]));
        TEST_CHECK_(*ans == *ret[i], "Return value matched for tid : %lld and return value : %d", tid[i], *ret[i]);
        TEST_MSG("Expected value : %d , Return Value : %d", *ans, *ret[i]);
        free(ans);
        free(ret[i]);
    }
}
void test_thread_kill()
{
    void *ret;
    printf("\n\t\033[1m1. Send invalid signal to thread\033[0m\n\n");
    kthread_t tid;
    struct sigaction action;
    action.sa_handler = sigusr1_handler;
    sigaction(SIGUSR1, &action, NULL);
    // global variable
    counting = 1;
    kthread_create(&tid, NULL, thread4, NULL);
    TEST_CHECK_(kthread_kill(tid, -1) == EINVAL, "Thread kill with invalid argument");
    counting = 0;
    kthread_join(tid, NULL);

    printf("\n\t\033[1m2. Send signal to a thread, check user signal handler\033[0m\n\n");
    counting = 1;
    kthread_create(&tid, NULL, thread4, NULL);
    kthread_kill(tid, SIGUSR1);
    TEST_CHECK_(kthread_join(tid, &ret) == 0, "User Signal handler, signal handled");

    printf("\n\t\033[1m3. Checking signal handling for SIGTSTP SIGCONT SIGTERM SIGKILL\033[0m\n\n");
    kthread_create(&tid, NULL, thread5, NULL);
    printf("Sending SIGTSTP signal\n");
    TEST_CHECK_(kthread_kill(tid, SIGSTOP) == 0, "SIGSTOP handled");
    printf("Sending SIGCONT signal\n");
    int m = kthread_kill(tid, SIGCONT);
    TEST_CHECK_(m == 0, "SIGCONT handled : %d", m);
    printf("Sending SIGTERM signal\n");
    TEST_CHECK_(kthread_kill(tid, SIGTERM) == 0, "SIGTERM handled");
    kthread_join(tid, NULL);
    kthread_create(&tid, NULL, thread5, NULL);
    printf("Sending SIGKILL signal\n");
    TEST_CHECK_(kthread_kill(tid, SIGKILL) == 0, "SIGKILL handled");
    kthread_join(tid, NULL);
}

void test_thread_exit()
{
    printf("\n\t\033[1m1. Created thread calls kthread_exit \033[0m\n\n");
    void *ret;
    kthread_t tid;
    kthread_create(&tid, NULL, thread6, NULL);
    kthread_join(tid, &ret);
    TEST_CHECK_(*(int *)ret == 899, "kthread exit changes return value");
}
TEST_LIST = {
    {"1 : Thread Creation with Invalid Arguments", test_invalid_arguments},
    {"2 : Thread Creation Without Attributes", test_without_attributes},
    {"3 : Multiple Thread Creation", test_thread_create},
    {"4 : Thread Join with Invalid Arguments", test_invalid_join},
    {"5 : Multiple Thread Joining", test_thread_join},
    {"5 : Thread Exit Testing", test_thread_exit},
    {"6 : Thread Kill Testing", test_thread_kill},
    {0}};