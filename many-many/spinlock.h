#ifndef SPINLOCK_H_INCLUDED
#define SPINLOCK_H_INCLUDED
typedef struct spinlock_t
{
    volatile unsigned int locked;
} spinlock_t;
void init_lock(spinlock_t *lk);
void acquire_lock(spinlock_t *lk);
void release_lock(spinlock_t *lk);
#endif