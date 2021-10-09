#include "types.h"
#include "stat.h"
#include "user.h"

int main(void)
{
    int proc_cnt = getProcCount();
    printf(1, "\nprocess count: %d\n", proc_cnt);
    printf(1, "\nsuccessfull\n");
    exit();
}