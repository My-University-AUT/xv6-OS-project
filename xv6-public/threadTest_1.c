#include "types.h"
#include "stat.h"
#include "user.h"

#define PS 4096

// int stack[PS] __attribute__((aligned (PS)));
int stack[4096] __attribute__((aligned (4096)));
int x = 0;

int main(int argc, char *argv[])
{
    printf(1,"stackkk %d\n",stack);
    int tid = thread_create(stack);

    if (tid < 0)
    {
        printf(1, "can not create thread!\n");
    }
    else if (tid == 0)
    {
        for (;;)
        {
            x++;
            sleep(100);
        }
    }
    else
    {
        for (;;)
        {
            printf(1, "x is %d\n", x);
            sleep(100);
        }
    }
    exit();
}