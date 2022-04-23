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

TEST_LIST = {
    {"1 : Thread Creation with Invalid Arguments", test_invalid_arguments},
    {"2 : Thread Creation with Without Attributes", test_without_attributes},
    {0}};