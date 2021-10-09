#include "types.h"
#include "stat.h"
#include "user.h"

int main(void)
{
    int read_syscall_cnt = getReadCount();
    printf(1, "\nread syscall count: %d\n", read_syscall_cnt);
    printf(1, "\nsuccessfull\n");
    exit();
}