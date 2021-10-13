#include "types.h"
#include "stat.h"
#include "user.h"

int main(void)
{
    int proc_cnt = getProcCount();
    printf(1, "\nprocess count before fork: %d\n", proc_cnt);

    int pid = fork();

    if (pid > 0)
    {
        printf(1, "\nprocess count after fork: %d\n", getProcCount());

        wait(); // Wait for a child process to exit and return its pid.
                // also returns -1 if process has no children.
        printf(1, "\nprocess count after child process is over: %d\n", getProcCount());

        printf(1, "\nsuccessfull\n");
    }

    exit();
}