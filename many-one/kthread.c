#include "./kthread.h"

int next_tid = 0;

/*Timer starting*/
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
    free(del);
    // Free other memory
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
