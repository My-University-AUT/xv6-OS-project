#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

// #define PS 4096 // page size

struct
{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct spinlock thread_lock;

struct spinlock printProcessTime_lock;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void pinit(void)
{
  initlock(&ptable.lock, "ptable");
  initlock(&thread_lock, "thlock");
  initlock(&printProcessTime_lock, "proccessTimeLock");
}

// Must be called with interrupts disabled
int cpuid()
{
  return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void)
{
  int apicid, i;

  if (readeflags() & FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i)
  {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void)
{
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

// PAGEBREAK: 32
//  Look in the process table for an UNUSED proc.
//  If found, change state to EMBRYO and initialize
//  state required to run in the kernel.
//  Otherwise return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  // NEW CODE
  // assgin readCount to 0 because this is the time that process get allocated
  p->readCount = 0;

  // NEW CODE
  // -1 means process have no threads at first
  p->threads = -1;
  p->topOfStack = -1;

  // PHASE 3:
  p->readyTime = 0;
  p->runningTime = 0;
  p->creationTime = 0;
  p->sleepingTime = 0;
  p->terminationTime = 0;

  release(&ptable.lock);

  // Allocate kernel stack.
  if ((p->kstack = kalloc()) == 0)
  {
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe *)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint *)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context *)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

// PAGEBREAK: 32
//  Set up first user process.
void userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if ((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  // p->topOfStack = PGSIZE;
  p->tf->eip = 0; // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  acquire(&thread_lock);
  sz = curproc->sz;
  if (n > 0)
  {
    if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
    {
      release(&thread_lock);
      return -1;
    }
  }
  else if (n < 0)
  {
    if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
    {
      release(&thread_lock);
      return -1;
    }
  }
  curproc->sz = sz;

  acquire(&ptable.lock);
  struct proc *p;
  int numOfChildren;
  if (curproc->threads == -1)
  {
    curproc->parent->sz = sz;
    numOfChildren = curproc->parent->threads - 2;
    if (numOfChildren <= 0)
    {
      release(&ptable.lock);
      release(&thread_lock);
      switchuvm(curproc);
      return 0;
    }
    // update sibling of process
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p != curproc && p->parent == curproc->parent && p->threads == -1)
      {
        p->sz = sz;
        numOfChildren--;
      }
    }
  }
  else
  {
    numOfChildren = curproc->threads - 1;
    if (numOfChildren <= 0)
    {
      release(&ptable.lock);
      release(&thread_lock);
      switchuvm(curproc);
      return 0;
    }
    else
    {
      for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      {
        if (p->parent == curproc && p->threads == -1)
        {
          p->sz = sz;
          numOfChildren--;
        }
      }
    }
  }

  release(&ptable.lock);
  release(&thread_lock);
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  // Copy process state from proc.
  if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0)
  {
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  (np->threads)++;
  np->topOfStack = curproc->topOfStack;
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for (i = 0; i < NOFILE; i++)
    if (curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if (curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for (fd = 0; fd < NOFILE; fd++)
  {
    if (curproc->ofile[fd])
    {
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->parent == curproc)
    {
      p->parent = initproc;
      if (p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->parent != curproc)
        continue;
      // NEW CODE ADDED
      if (p->threads < 0) // if p->threads is less than zero means that child is 'thread child' and we should NOT wait for him
        continue;
      havekids = 1;
      if (p->state == ZOMBIE)
      {
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        p->threads = -1;
        p->topOfStack = -1;

        // PHASE 3:
        p->readyTime = 0;
        p->runningTime = 0;
        p->creationTime = 0;
        p->sleepingTime = 0;
        p->terminationTime = 0;

        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || curproc->killed)
    {
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock); // DOC: wait-sleep
  }
}

// PAGEBREAK: 42
//  Per-CPU process scheduler.
//  Each CPU calls scheduler() after setting itself up.
//  Scheduler never returns.  It loops, doing:
//   - choose a process to run
//   - swtch to start running that process
//   - eventually that process transfers control
//       via swtch back to the scheduler.
void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for (;;)
  {
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->state != RUNNABLE)
        continue;

      // if (schedulerPolicy == RR)
      // {
      //   int i = 0;
      //   for (; i < QUANTUM; i++)
      //   {
      //     if (p->state != RUNNABLE)
      //       break;

      //     c->proc = p;
      //     switchuvm(p);
      //     p->state = RUNNING;

      //     swtch(&(c->scheduler), p->context);
      //     switchkvm();

      //     c->proc = 0;
      //   }
      //   continue;
      // }

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&ptable.lock))
    panic("sched ptable.lock");
  if (mycpu()->ncli != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (readeflags() & FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  acquire(&ptable.lock); // DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first)
  {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if (p == 0)
    panic("sleep");

  if (lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if (lk != &ptable.lock)
  {                        // DOC: sleeplock0
    acquire(&ptable.lock); // DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if (lk != &ptable.lock)
  { // DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// PAGEBREAK!
//  Wake up all processes sleeping on chan.
//  The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      p->killed = 1;
      // Wake process from sleep if necessary.
      if (p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// PAGEBREAK: 36
//  Print a process listing to console.  For debugging.
//  Runs when user types ^P on console.
//  No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [EMBRYO] "embryo",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if (p->state == SLEEPING)
    {
      getcallerpcs((uint *)p->context->ebp + 2, pc);
      for (i = 0; i < 10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// FOR TEST

int getHelloWorld(void)
{
  cprintf("heeelooo man!");
  return 0;
}

// NEW CODE
int getProcCount(void)
{

  struct proc *p;
  int num_of_proc = 0;

  acquire(&ptable.lock);

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state != UNUSED)
      num_of_proc++;
  release(&ptable.lock);

  return num_of_proc;
}

int getReadCount(void)
{
  struct proc *p;

  acquire(&ptable.lock);
  int read_cnt = 0;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    // if(p->readCount> 0){
    read_cnt += p->readCount;
    cprintf("process id: %d --- process count: %d\n", p->pid, p->readCount);
  }
  release(&ptable.lock);

  return read_cnt;
}

int has_shared_pgdir(struct proc *proc)
{
  // TODO
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (proc != p && proc->pgdir == p->pgdir)
    {
      return 1;
    }
  }
  return 0;
}

int thread_create(void *stack)
{
  cprintf("hello 1\n");
  int pid;

  struct proc *curr_proc = myproc();

  struct proc *new_proc;
  // allocate memory to new process
  new_proc = allocproc();
  if (new_proc == 0)
  {
    return -1;
  }

  (curr_proc->threads)++;

  new_proc->topOfStack = (int)((char *)stack + PGSIZE);

  acquire(&ptable.lock);
  new_proc->pgdir = curr_proc->pgdir;
  new_proc->sz = curr_proc->sz;
  release(&ptable.lock);

  // topOfStack minus stackPointer gives us the amount of bytes that occupied by curr_proc stack
  int stackBytes = curr_proc->topOfStack - curr_proc->tf->esp;

  *new_proc->tf = *curr_proc->tf;

  new_proc->tf->eax = 0;
  // set the stack pointer to new process(virtually)
  new_proc->tf->esp = new_proc->topOfStack - stackBytes;
  // set base pointer for new process
  new_proc->tf->ebp = new_proc->topOfStack - (curr_proc->topOfStack - curr_proc->tf->ebp);

  // copy the stack content from curr_proc to new_proc as much as stackBytes
  memmove((void *)new_proc->tf->esp, (void *)curr_proc->tf->esp, stackBytes);

  new_proc->parent = curr_proc;

  int i = 0;
  for (i = 0; i < NOFILE; i++)
  {
    if (curr_proc->ofile)
    {
      new_proc->ofile[i] = filedup(curr_proc->ofile[i]);
    }
  }
  new_proc->cwd = idup(curr_proc->cwd);

  safestrcpy(new_proc->name, curr_proc->name, sizeof(curr_proc->name));
  pid = new_proc->pid;

  acquire(&ptable.lock);
  new_proc->state = RUNNABLE;
  release(&ptable.lock);

  return pid;
}

int thread_wait(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for (;;)
  {
    havekids = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->parent != curproc) // means that proc is not child of curr_proc. so continue...
        continue;
      if (p->threads != -1) // if p->threads 'is greather than -1' means that child is 'process child' and we should NOT wait for him
        continue;
      // if code reaches this line means curr_proc has 'thread child'
      havekids = 1;
      if (p->state == ZOMBIE)
      {

        if (!has_shared_pgdir(p))
        {
          freevm(p->pgdir);
        }
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        p->threads = -1;
        p->topOfStack = -1;

        // PHASE 3:
        p->readyTime = 0;
        p->runningTime = 0;
        p->creationTime = 0;
        p->sleepingTime = 0;
        p->terminationTime = 0;

        release(&ptable.lock);
        return pid;
      }
    }
    // No point waiting if we don't have any children.
    if (!havekids || curproc->killed)
    {
      release(&ptable.lock);
      return -1;
    }

    sleep(curproc, &ptable.lock); // DOC: wait-sleep
  }
}

void printPolicy()
{
  if (schedulerPolicy == RR)
  {
    cprintf("policy is round robin (RR)\n");
  }
  else if (schedulerPolicy == NPPS)
  {
    cprintf("policy is non preemptive priority scheduling (NPPS)\n");
  }
  else if (schedulerPolicy == PMLQ)
  {
    cprintf("policy is preemptive multi level queue (PMLQ)\n");
  }
  else if (schedulerPolicy == DMLQ)
  {
    cprintf("policy is dynamic multi level queue (DMLQ)\n");
  }
}

int setSchedulerPolicy(void *policy)
{
  cprintf("default scheduler policy: %d\n", schedulerPolicy);
  if (*((int *)policy) >= 0 && *((int *)policy) <= 3)
  {
    schedulerPolicy = *((int *)policy);
    cprintf("after setting scheduler policy: %d\n", schedulerPolicy);
    printPolicy();
    return 0;
  }
  else
  {
    schedulerPolicy = 0;
    cprintf("after setting scheduler policy: %d\n", schedulerPolicy);
    return -1;
  }
}

void updateProccessTime()
{
  struct proc *p;
  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    switch (p->state)
    {
    case RUNNING:
      p->runningTime++;
      break;
    case RUNNABLE:
      p->readyTime++;
      break;
    case SLEEPING:
      p->sleepingTime++;
      break;
    default:
      break;
    }
  }
  release(&ptable.lock);
}

void printProcessTime(void)
{
  acquire(&printProcessTime_lock);
  // PHASE 3
  struct proc *curproc = myproc();

  // cprintf("#### proccess: %d ######\n", curproc->pid);
  cprintf("pid: %d -- ready time: %d\n", curproc->pid, curproc->readyTime);
  cprintf("pid: %d -- running time: %d\n", curproc->pid, curproc->runningTime);
  cprintf("pid: %d -- sleeping time: %d\n", curproc->pid, curproc->sleepingTime);
  cprintf("########################\n");
  release(&printProcessTime_lock);
}