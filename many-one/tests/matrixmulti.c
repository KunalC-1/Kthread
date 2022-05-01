#include "../kthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAT_SIZE 10
#define MAX_THREADS 100

int i, j, k;
int matrix1[MAT_SIZE][MAT_SIZE];
int matrix2[MAT_SIZE][MAT_SIZE];
int result[MAT_SIZE][MAT_SIZE];

typedef struct parameters
{
    int x, y;
} args;

void *mult(void *arg)
{

    args *p = arg;

    // Calculating Each Element in Result Matrix Using Passed Arguments
    for (int a = 0; a < j; a++)
    {
        result[p->x][p->y] += matrix1[p->x][a] * matrix2[a][p->y];
    }
    sleep(3);

    // End Of Thread
    kthread_exit(0);
    return NULL;
}

int main()
{
    for (int x = 0; x < 10; x++)
    {
        for (int y = 0; y < 10; y++)
        {
            matrix1[x][y] = 0;
            matrix2[x][y] = 0;
            result[x][y] = 0;
        }
    }
    printf(" --- Defining Matrix 1 ---\n\n");
    printf("Enter number of rows for matrix 1: ");
    scanf("%d", &i);
    printf("Enter number of columns for matrix 1: ");
    scanf("%d", &j);
    printf("\n --- Initializing Matrix 1 ---\n\n");
    int i1 = 0, i2 = 0;
    for (int x = 0; x < i; x++)
    {
        for (int y = 0; y < j; y++)
        {
            matrix1[x][y] = i1++;
        }
    }

    printf("\n --- Defining Matrix 2 ---\n\n");

    // Getting Column Number For Matrix2
    printf("Number of rows for matrix 2 : %d\n", j);
    printf("Enter number of columns for matrix 2: ");
    scanf("%d", &k);

    printf("\n --- Initializing Matrix 2 ---\n\n");
    for (int x = 0; x < j; x++)
    {
        for (int y = 0; y < k; y++)
        {
            matrix2[x][y] = i2++;
        }
    }

    // Printing Matrices - - - - - - - - - - - - - - - - - - - - - - - - - - -//

    printf("\n --- Matrix 1 ---\n\n");
    for (int x = 0; x < i; x++)
    {
        for (int y = 0; y < j; y++)
        {
            printf("%5d", matrix1[x][y]);
        }
        printf("\n\n");
    }

    printf(" --- Matrix 2 ---\n\n");
    for (int x = 0; x < j; x++)
    {
        for (int y = 0; y < k; y++)
        {
            printf("%5d", matrix2[x][y]);
        }
        printf("\n\n");
    }

    // Multiply Matrices Using Threads - - - - - - - - - - - - - - - - - - - -//

    // Defining Threads
    kthread_t thread[MAX_THREADS];

    // Counter For Thread Index
    int thread_number = 0;

    // Defining p For Passing Parameters To Function As Struct
    args p[i * k];

    // Start Timer
    time_t start = time(NULL);

    for (int x = 0; x < i; x++)
    {
        for (int y = 0; y < k; y++)
        {

            // Initializing Parameters For Passing To Function
            p[thread_number].x = x;
            p[thread_number].y = y;

            // Status For Checking Errors
            int status;

            // Create Specific Thread For Each Element In Result Matrix
            status = kthread_create(&thread[thread_number], NULL, mult, (void *)&p[thread_number]);

            if (status)
            {
                printf("Error In Threads");
                exit(0);
            }

            thread_number++;
        }
    }

    // Wait For All Threads Done - - - - - - - - - - - - - - - - - - - - - - //

    for (int z = 0; z < (i * k); z++)
        kthread_join(thread[z], NULL);

    // Print Multiplied Matrix (Result) - - - - - - - - - - - - - - - - - - -//

    printf(" --- Multiplied Matrix ---\n\n");
    for (int x = 0; x < i; x++)
    {
        for (int y = 0; y < k; y++)
        {
            printf("%5d", result[x][y]);
        }
        printf("\n\n");
    }

    // Calculate Total Time Including 3 Soconds Sleep In Each Thread - - - -//

    printf(" ---> Time Elapsed : %.2f Sec\n\n", (double)(time(NULL) - start));

    // Total Threads Used In Process - - - - - - - - - - - - - - - - - - - -//

    printf(" ---> Used Threads : %d \n\n", thread_number);
    for (int z = 0; z < thread_number; z++)
        printf(" - Thread %d ID : %d\n", z + 1, (int)thread[z]);

    return 0;
}
