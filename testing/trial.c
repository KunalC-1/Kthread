#include "../one-one/kthread.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
void *myturn(void *arg)
{
    int *iptr = (int *)malloc(sizeof(int));
    *iptr = 5;
    for (int i = 0; i < 6; i++)
    {
        sleep(1);
        printf("My Turn! %d\n", i);
    }
    // what if we return a value?-
    printf("Ans :%d", *iptr);
    return iptr;
}
void *yourturn()
{
    for (int i = 0; i < 7; i++)
    {
        sleep(2);
        printf("your turn %d\n", i);
    }
}
void *histurn()
{
    for (int i = 0; i < 7; i++)
    {
        sleep(1);
        printf("his turn %d\n", i);
    }
}
void *herturn()
{
    for (int i = 0; i < 4; i++)
    {
        sleep(1);
        printf("her turn %d\n", i);
    }
}
int main()
{
    kthread_t new_thread, two, three;
    int *result, r;
    kthread_create(&new_thread, NULL, myturn, NULL);
    kthread_create(&two, NULL, histurn, NULL);
    kthread_create(&three, NULL, herturn, NULL);

    yourturn();
    kthread_join(new_thread, (void **)&result);
    kthread_join(two, NULL);
    kthread_join(three, NULL);

    printf("Final Ans: %d", *result);
}