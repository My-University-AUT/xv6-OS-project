#include "types.h"
#include "stat.h"
#include "user.h"

#define num_of_children 30

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
    *policy = 1;
    printf(1,"policy is %d\n", *policy);
    setSchedulerPolicy((void *)policy);

    struct pData *pdata_arr = malloc(num_of_children * sizeof(struct pData));

    int parent_pid = getpid();
    int i;
    for (i = 0; i < num_of_children; i++)
    {
        if (fork() == 0)
        {
            setPriority(6 - (int)(i / 5));
            break;
        }
    }
    sleep(10);
    int pid = getpid();

    doSomeDummyWork(250);

    if (pid == parent_pid)
    {
        int i;
        for (i = 0; i < num_of_children; i++)
        {
            waitWithPData((void *)&pdata_arr[i]);
            // wait();
        }

        int TAT_average=0;
        int CBT_average=0;
        int WT_average=0;
        printf(1, "after all the children are done:\n");
        for ( i = 0; i < num_of_children; i++)
        {
            int TAT = pdata_arr[i].runningTime +  pdata_arr[i].readyTime + pdata_arr[i].sleepingTime;
            int WT =  pdata_arr[i].sleepingTime;
            int CBT = pdata_arr[i].runningTime;
            TAT_average+= TAT;
            WT_average+= WT;
            CBT_average+= CBT;
            
            printf(1, "========================================\n");
            printf(1, "value of pid: %d | priority: %d\n", pdata_arr[i].pid, pdata_arr[i].priority);
            printf(1, "value of creation time: %d\n", pdata_arr[i].creationTime);
            printf(1, "value of termination time: %d\n", pdata_arr[i].terminationTime);
            printf(1,"-------------\n");
            printf(1, "value of running time(CBT): %d\n", CBT);
            printf(1, "value of waiting time: %d\n", WT);
            printf(1, "value of turnAroundTime(TAT): %d\n", TAT);
        }
        TAT_average /= num_of_children;
        CBT_average /= num_of_children;
        WT_average /= num_of_children;
        printf(1, "=========================\n");
        printf(1, "TAT average: %d\n", TAT_average);
        printf(1, "CBT average: %d\n", CBT_average);
        printf(1, "waiting time average: %d\n", WT_average);


        exit();
    }
    else
    {
        exit();
    }
}