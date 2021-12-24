#include "types.h"
#include "stat.h"
#include "user.h"

#define PS 4090 // page size

int thread_creator(void (*fn)(void *), void *args)
{
    void *f_ptr = malloc( 2 * PS);
    void *stack;

    if(f_ptr ==0){
        return -1;
    }

    int mod = (uint)f_ptr % PS;
    if(mod == 0){
        stack = f_ptr;
    }else{
        // aligning the stack to be ِdivisible by PS(4096 here)
        stack = f_ptr + (PS - mod);
    }

    int thread_id = thread_create((void *)stack);

    if(thread_id <0){
        printf(1,"failed to create_thread");
        return -1;
    }

    if(thread_id == 0){
        // execute the function in child thread
        (fn)(args);
        free(stack);
        exit();
    }

    return thread_id;

}