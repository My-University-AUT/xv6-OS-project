#include "types.h"
#include "stat.h"
#include "user.h"

int main(void)
{
    int proc_cnt = getProcCount();
    printf(1, "\nprocess count before fork: %d\n", proc_cnt);

    int pid = fork();

    // Error occured when creating new child process
    if (pid < 0)
    {
        printf(1, "Error occured when creating child process");
        exit();
    }
    // below else if block is executed just for child process
    else if (pid == 0)
    {
        sleep(500);
        exit();
    }

    // below code is executed just for parent process
    printf(1, "\nprocess count after fork: %d\n\n", getProcCount());

    wait(); // Wait for a child process to exit and return its pid.
            // also returns -1 if process has no children.
    printf(1, "process count after child process is over: %d\n\n", getProcCount());

    printf(1, "successfull\n");

    exit();
}