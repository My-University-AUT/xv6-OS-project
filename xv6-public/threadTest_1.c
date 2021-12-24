#include "types.h"
#include "stat.h"
#include "user.h"

#define PS 4096

// int stack[PS] __attribute__((aligned (PS)));
int stack[4096] __attribute__((aligned(4096)));
int x = 0;

int main(int argc, char *argv[])
{
    printf(1, "stackkk %d\n", stack);
    int tid = thread_create(stack);

    if (tid < 0)
    {
        printf(1, "can not create thread!\n");
    }
    else if (tid == 0)
    {
        sleep(100);
        printf(1, "========> im here\n");
        for (;;)
        {
            x++;
            printf(1, "x is %d\n", x);
            sleep(100);
            if(x==10){
                exit();
            }
        }
    }
    else
    {
        // for (;;)
        // {
        //     printf(1, "x is %d\n", x);
        //     sleep(100);
        // }

        printf(1,"wait for child thread to reachs to x=10\n");
        thread_wait();
        printf(1,"after ending the child thread, x is %d", x);
    }
    exit();
}