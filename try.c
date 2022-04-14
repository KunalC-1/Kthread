#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define START 0
#define FROM_CTRL_C 1
#define FROM_ALARM 2
#define ALARM 5
jmp_buf Buf;
void INT(int);
void ALRM(int);
void INT(int sig)
{
    char c;
    signal(SIGALRM, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    printf("Want to quite?");
    c = getchar();
    if (c == 'y' || c == 'Y')
        exit(0);
    signal(SIGINT, INT);
    signal(SIGALRM, ALRM);
    longjmp(Buf, FROM_CTRL_C);
}
void ALRM(int sig)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    printf("got and alarm\n");
    alarm(0); /* reset alarm */
    signal(SIGALRM, ALRM);
    signal(SIGINT, INT);
    longjmp(Buf, FROM_ALARM);
}
int main(void)
{
    int Return;
    signal(SIGINT, INT);
    signal(SIGALRM, ALRM);
    while (1)
    {
        if ((Return = setjmp(Buf)) == START)
        {
            alarm(ALARM);
            pause();
        }
        else if (Return == FROM_CTRL_C)
        {
        }
        else if (Return == FROM_ALARM)
        {
            printf("Alarm reset to % d sec.\n",
                   ALARM);
            alarm(ALARM);
        }
    }
}