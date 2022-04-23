#include "acutest.h"
#include "../one-one/kthread.h"
void *thread1()
{
    int x = 0;
    for (int i = 0; i < 18; i++)
    {
        x += i;
    }
}
void *thread2(void *arg)
{
    int x = *((int *)arg);
    for (int i = 0; i < 18; i++)
    {
        x += i;
    }
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
    TEST_CHECK_(kthread_join(tid, NULL) == 0, "kthread_join(tid, NULL) : %d", tid);
}
void test_thread_create()
{
    kthread_t tid[30];
    int t, num = 20;

    for (int i = 0; i < 30; i++)
    {
        t = kthread_create(&tid[i], NULL, thread2, (void *)&num);
        TEST_CHECK_(t == 0, "Thread created with thread id : %d", tid[i]);
    }
}
void test_invalid_join()
{
    kthread_t tid = rand();
    int *r;
    TEST_CHECK_(kthread_join(NULL, NULL) == -1, "Join with null arguments");
    TEST_CHECK_(kthread_join(tid, NULL) == -1, "Join with invalid thread Id : %d", tid);
    TEST_CHECK_(kthread_join(NULL, &r) == -1, "Join with invalid args");
    kthread_create(&tid, NULL, thread1, NULL);
    kthread_join(tid, NULL);
    TEST_CHECK_(kthread_join(tid, &r) == -1, "Join already joined thread");
}
void test_thread_join()
{
    kthread_t tid[8];
    int *r, t, num = 5;
    printf("\n\t\033[1m1. Joining threads immediately after creation\033[0m\n\n");
    for (int i = 0; i < 8; i++)
    {
        t = kthread_create(&tid[i], NULL, thread2, (void *)&num);
        TEST_CHECK_(t == 0, "Thread created with thread id : %d", tid[i]);
        TEST_CHECK_(kthread_join(tid[i], &r) == 0, "Join thread with id  : %d", tid[i]);
    }
    printf("\n\t\033[1m2. Joining threads collectively after creation\033[0m\n\n");
    for (int i = 0; i < 8; i++)
    {
        t = kthread_create(&tid[i], NULL, thread2, (void *)&num);
        TEST_CHECK_(t == 0, "Thread created with thread id : %d", tid[i]);
    }
    for (int i = 0; i < 8; i++)
        TEST_CHECK_(kthread_join(tid[i], &r) == 0, "Join thread with id  : %d", tid[i]);
    printf("\n\t\033[1m3. Joining thread and Checking return value of thread\033[0m\n\n");
    for (int i = 0; i < 3; i++)
    {
        t = kthread_create(&tid[i], NULL, thread3, (void *)&i);
        TEST_CHECK_(t == 0, "Thread created : %d with function argument : %d", tid[i], i);
    }
    for (int i = 0; i < 3; i++)
    {
        kthread_join(tid[i], &r);
        int *ans = (int *)(thread3(&i));
        TEST_CHECK_(*ans == *r, "Return value matched for tid : %d and return value : %d", tid[i], *r);
        free(ans);
        free(r);
    }
}

TEST_LIST = {
    {"1 : Thread Creation with Invalid Arguments", test_invalid_arguments},
    {"2 : Thread Creation Without Attributes", test_without_attributes},
    {"3 : Multiple Thread Creation", test_thread_create},
    {"4 : Thread Join with Invalid Arguments", test_invalid_join},
    {"5 : Multiple Thread Joining", test_thread_join},
    {0}};