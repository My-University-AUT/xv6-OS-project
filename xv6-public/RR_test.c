#include "types.h"
#include "stat.h"
#include "user.h"

// TODO: change to 10 after test
#define num_of_children 10

// // process data
struct pData
{
    int pid;
    uint creationTime;
    uint terminationTime;
    uint readyTime;
    uint runningTime;
    uint sleepingTime;
};

int main(int argc, char *argv[])
{
    // struct pData *data = malloc(sizeof(struct pData));
    // printf(1, "before settign: %d\n", data->readyTime);
    // data->readyTime = 1;
    // data->runningTime = 2;
    // data->sleepingTime = 3;
    // data->pid = 4;
    // printf(1, "after settign: %d\n", data->readyTime);


    // set Scheduler Policy to Round-Robin Scheduling
    int *policy = (int *)malloc(sizeof(int *));
    *policy = 0;
    printf(1,"policy is %d\n", *policy);
    setSchedulerPolicy((void *)policy);

    struct pData *pdata_arr = malloc(num_of_children * sizeof(struct pData));

    int parent_pid = getpid();
    int i;
    for (i = 0; i < num_of_children; i++)
    {
        if (fork() == 0)
        {
            break;
        }
    }
    // // sleep(5);
    int pid = getpid();

    doSomeDummyWork(1000);

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
            printf(1, "value of termination time: %d\n", pdata_arr[i].terminationTime);
            printf(1, "value of ready time: %d\n", pdata_arr[i].readyTime);
            printf(1, "value of running time: %d\n", pdata_arr[i].runningTime);
            printf(1, "value of sleeping time: %d\n", pdata_arr[i].sleepingTime);
            printf(1, "value of pid: %d\n", pdata_arr[i].pid);
        }

        exit();
    }
    else
    {
        // printProcessTime();
        exit();
    }
}