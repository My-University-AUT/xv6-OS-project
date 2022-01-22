#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int sys_fork(void)
{
  return fork();
}

int sys_exit(void)
{
  exit();
  return 0; // not reached
}

int sys_wait(void)
{
  return wait();
}

int sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int sys_getpid(void)
{
  return myproc()->pid;
}

int sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

int sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// FOR TEST
int sys_getHelloWorld(void)
{
  return getHelloWorld();
}

// NEW CODE
int sys_getProcCount(void)
{
  return getProcCount();
}

int sys_getReadCount(void)
{
  return getReadCount();
}

int sys_thread_create(void)
{
  int stack;
  // if (argptr(3, (void *)&stack, sizeof(void *)) < 0)
  //   return -1;
  // return thread_create(stack);
  if (argint(0, &stack) < 0)
  {
    return -1;
  }
  return thread_create((void *)stack);
}

int sys_thread_wait(void)
{
  return thread_wait();
}

int sys_setPriority(void)
{
  int priority;
  if(argint(0, &priority) < 0)
    return -1;

  return setPriority(priority);
}

// set the scheduling policy.
int sys_setSchedulerPolicy(void)
{
  int policy;

  if (argint(0, &policy) < 0)
  {
    return -1;
  }
  return setSchedulerPolicy((void *)policy);
}
int sys_printProcessTime(void)
{
  printProcessTime();
  return 0;
}
int sys_doSomeDummyWork(void)
{
  int lineNum;
  if(argint(0,&lineNum) < 0){
    return -1;
  }

  doSomeDummyWork(lineNum);
  return 0;
}

int sys_waitWithPData(void)
{
  struct pData *data;
  argptr(0, (void *)&data, sizeof(*data));
  return waitWithPData((void *)data);
}