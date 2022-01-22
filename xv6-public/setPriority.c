#include "types.h"
#include "stat.h"
#include "user.h"


int main(int argc, char *argv[])
{
    if(argc < 2){
        printf(2,"Too few arguments!\n");
        exit();
    }
    printf(1,"number of args: %d\n", argc);
    printf(1,"1th arg value: %s\n", argv[1]);

    int priority = atoi(argv[1]);
    printf(1,"priority is %d\n", priority);
    setPriority(priority);    
    exit();
}