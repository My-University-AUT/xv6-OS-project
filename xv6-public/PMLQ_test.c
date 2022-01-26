#include "types.h"
#include "stat.h"
#include "user.h"

#define num_of_children 60

// process data
struct pData
{
    int pid;
    uint creationTime;
    uint startingTime;
    uint terminationTime;
    uint readyTime;
    uint runningTime;
    uint sleepingTime;
    uint priority;
};

int main(int argc, char *argv[])
{

    // set Scheduler Policy to Non-Preemptive Priority Scheduling
    int *policy = (int *)malloc(sizeof(int *));
    *policy = 2;
    printf(1,"policy is %d\n", *policy);
    setSchedulerPolicy((void *)policy);

    struct pData *pdata_arr = malloc(num_of_children * sizeof(struct pData));

    int parent_pid = getpid();
    int i;
    for (i = 0; i < num_of_children; i++)
    {
        if (fork() == 0)
        {
            setPriority(6 - (int)(i / 10));
            break;
        }
    }
    sleep(10);
    int pid = getpid();

    doSomeDummyWork(200);

    if (pid == parent_pid)
    {
        int i;
        for (i = 0; i < num_of_children; i++)
        {
            waitWithPData((void *)&pdata_arr[i]);
            // wait();
        }

        printf(1, "after all the children are done:\n");
        for ( i = 0; i < num_of_children; i++)
        {
            printf(1, "========================================\n");
            printf(1, "value of creation time: %d\n", pdata_arr[i].creationTime);
            printf(1, "value of starting time: %d\n", pdata_arr[i].startingTime);
            printf(1, "value of termination time: %d\n", pdata_arr[i].terminationTime);
            printf(1, "value of ready time: %d\n", pdata_arr[i].readyTime);
            printf(1, "value of running time: %d\n", pdata_arr[i].runningTime);
            printf(1, "value of sleeping time: %d\n", pdata_arr[i].sleepingTime);
            printf(1, "value of pid: %d | priority: %d\n", pdata_arr[i].pid, pdata_arr[i].priority);
        }

        exit();
    }
    else
    {
        exit();
    }
}