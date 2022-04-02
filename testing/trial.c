#include "../one-one/kthread.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
void *myturn(void *arg)
{
    int *iptr = (int *)malloc(sizeof(int));
    *iptr = 5;
    printf("before\n");
    for (int i = 0; i < 8; i++)
    {
        sleep(1);
        printf("My Turn! %d\n", i);
    }
    // what if we return a value?-
    printf("Ans :%d", *iptr);
    return iptr;
}
void yourturn()
{
    for (int i = 0; i < 3; i++)
    {
        sleep(2);
        printf("your turn\n");
    }
}
int main()
{
    kthread_t new_thread;
    int *result;
    kthread_create(&new_thread, NULL, myturn, NULL);
    yourturn();
    kthread_join(new_thread, (void **)&result);

    printf("%d", *result);
}