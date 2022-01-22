#include "types.h"
#include "stat.h"
#include "user.h"
// #include "spinlock.h"

// #include "semaphore.h"
#define num_of_children 10
// struct spinlock lock;

int main(int argc, char *argv[])
{
    // initlock(&lock, "simple_lock");

    int parent_pid = getpid();
    for (int i = 0; i < num_of_children; i++)
    {
        if (fork() == 0)
        {
            // printProcessTime();
            break;
        }
    }
    sleep(5);
    int pid = getpid();
    int k = 1;
    for (int i = 1; i < 10000000; i++)
    {
        k = k * i;
        k = k / i;
        k = k * i;
        k = k / i;
        k = k * i;
        // printf(1, "/PID = %d/ : /i = %d/\n", pid, i);
    }

    if (pid == parent_pid)
    {
        for (int i = 0; i < num_of_children; i++)
        {
            wait();
        }
    }
    else
    {
        printProcessTime();
        exit();
    }
}