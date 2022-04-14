#include <stdio.h>
#include "spinlock.h"
/*
https : // attractivechaos.wordpress.com/2011/10/06/multi-threaded-programming-efficiency-of-locking/
*/
void init_lock(spinlock_t *lk)
{
    lk->locked = 0;
}

void acquire_lock(spinlock_t *lk)
{
    while (__sync_lock_test_and_set(&lk->locked, 1))
        while (lk->locked)
            ;
    __sync_synchronize();
}
void release_lock(spinlock_t *lk)
{
    __sync_synchronize();
    __sync_lock_release(&lk->locked);
}