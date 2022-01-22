#include "types.h"
#include "stat.h"
#include "user.h"
// #include "spinlock.h"

// #include "semaphore.h"
#define num_of_children 10
// struct spinlock lock;

// // process data
// struct pData
// {
//     int pid;
//     uint readyTime;
//     uint runningTime;
//     uint sleepingTime;
// };

int main(int argc, char *argv[])
{
    // struct pData *data = malloc(sizeof(struct pData));
    // initlock(&lock, "simple_lock");
    // printf(1, "before settign: %d\n", data->readyTime);
    // data->readyTime = 1;
    // printf(1, "after settign: %d\n", data->readyTime);

    int parent_pid = getpid();
    for (int i = 0; i < num_of_children; i++)
    {
        if (fork() == 0)
        {
            break;
        }
    }
    // // sleep(5);
    int pid = getpid();

    doSomeDummyWork();

    if (pid == parent_pid)
    {
        for (int i = 0; i < num_of_children; i++)
        {
            // waitWithPData(data);
            wait();
        }
        exit();
    }
    else
    {
        printProcessTime();
        exit();
    }
}