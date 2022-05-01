#include "../kthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAT_SIZE 20
#define MAX_THREADS 226

int i, j, k;
int matrix1[MAT_SIZE][MAT_SIZE];
int matrix2[MAT_SIZE][MAT_SIZE];
int result[MAT_SIZE][MAT_SIZE];
int ans[MAT_SIZE][MAT_SIZE];

typedef struct parameters
{
    int x, y;
} args;

void *mult(void *arg)
{

    args *p = arg;

    for (int a = 0; a < j; a++)
    {
        result[p->x][p->y] += matrix1[p->x][a] * matrix2[a][p->y];
    }
    sleep(3);

    return NULL;
}

int main()
{
    printf("\n\t\033[1mMatrix Multiplication Test\033[0m\n\n");
    for (int x = 0; x < 10; x++)
    {
        for (int y = 0; y < 10; y++)
        {
            matrix1[x][y] = 0;
            matrix2[x][y] = 0;
            result[x][y] = 0;
        }
    }
    i = 10;
    j = 10;
    k = 10;
    int i1 = 0, i2 = 0;
    for (int x = 0; x < i; x++)
    {
        for (int y = 0; y < j; y++)
        {
            matrix1[x][y] = i1++;
        }
    }

    for (int x = 0; x < j; x++)
    {
        for (int y = 0; y < k; y++)
        {
            matrix2[x][y] = i2++;
        }
    }
    kthread_t thread[MAX_THREADS];

    int kthread_number = 0;

    args p[i * k];

    time_t start = time(NULL);

    for (int x = 0; x < i; x++)
    {
        for (int y = 0; y < k; y++)
        {

            p[kthread_number].x = x;
            p[kthread_number].y = y;

            int status;

            status = kthread_create(&thread[kthread_number], NULL, mult, (void *)&p[kthread_number]);

            if (status)
            {
                printf("Error In Threads");
                exit(0);
            }

            kthread_number++;
        }
    }

    for (int z = 0; z < (i * k); z++)
        kthread_join(thread[z], NULL);

    for (int x = 0; x < i; x++)
    {
        for (int z = 0; z < k; z++)
        {

            for (int y = 0; y < j; y++)
            {

                {
                    ans[x][z] += matrix1[x][y] * matrix2[y][z];
                }
            }
        }
    }

    printf(" ---> Time Elapsed : %.2f Sec\n\n", (double)(time(NULL) - start));

    printf(" ---> Used Threads : %d \n\n", kthread_number);
    for (int z = 0; z < kthread_number; z++)

        for (int x = 0; x < i; x++)
        {
            for (int y = 0; y < k; y++)
            {
                if (result[x][y] != ans[x][y])
                {

                    printf("Matrix Multiplication Wrong \n");
                    exit(0);
                }
            }
        }
    printf("Matrix Multiplication Matched\n\n");
    return 0;
}