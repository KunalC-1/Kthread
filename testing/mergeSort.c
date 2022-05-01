#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
// #include <pthread.h>
#include "../many-one/kthread.h"
struct Params
{
    int *start;
    size_t len;
    int depth;
};
void *merge_sort_thread(void *pv);
void merge(int *start, int *mid, int *end)
{
    int *res = malloc((end - start) * sizeof(*res));
    int *lhs = start, *rhs = mid, *dst = res;
    while (lhs != mid && rhs != end)
        *dst++ = (*lhs < *rhs) ? *lhs++ : *rhs++;
    while (lhs != mid)
        *dst++ = *lhs++;
    memcpy(start, res, (rhs - start) * sizeof *res);
    // free(res);
}
// punctul de intrare
void merge_sort_mt(int *start, size_t len, int depth)
{
    if (len < 2)
        return;

    if (depth <= 0 || len < 4)
    {
        merge_sort_mt(start, len / 2, 0);
        merge_sort_mt(start + len / 2, len - len / 2, 0);
    }
    else
    {
        struct Params params = {start, len / 2, depth / 2};
        // pthread_t thrd;
        // pthread_create(&thrd, NULL, merge_sort_thread, &params);
        // merge_sort_mt(start + len / 2, len - len / 2, depth / 2);
        // pthread_join(thrd, NULL);

        kthread_t thrd;
        kthread_create(&thrd, NULL, merge_sort_thread, &params);
        merge_sort_mt(start + len / 2, len - len / 2, depth / 2);
        kthread_join(thrd, NULL);
    }
    merge(start, start + len / 2, start + len);
}
void *merge_sort_thread(void *pv)
{
    struct Params *params = pv;
    merge_sort_mt(params->start, params->len, params->depth);
    return pv;
}

void merge_sort(int *start, size_t len)
{
    merge_sort_mt(start, len, 4);
}

int main()
{
    clock_t start, stop;
    static const unsigned int N = 10000;
    int *data = malloc(N * sizeof(*data));
    unsigned int i;
    srand((unsigned)time(0));
    for (i = 0; i < N; ++i)
    {
        data[i] = rand() % 500;
        printf("%4d ", data[i]);
        if ((i + 1) % 8 == 0)
            printf("\n");
    }
    printf("\n");
    start = clock();
    merge_sort(data, N);
    for (i = 0; i < N; ++i)
    {
        printf("%4d ", data[i]);
        if ((i + 1) % 8 == 0)
            printf("\n");
    }
    stop = clock();
    printf("Elapsed: %f seconds\n", (double)(stop - start) / CLOCKS_PER_SEC);
    getchar();
    return 0;
}