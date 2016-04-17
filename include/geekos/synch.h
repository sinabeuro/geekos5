/*
 * Synchronization primitives
 * Copyright (c) 2001, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.13 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef GEEKOS_SYNCH_H
#define GEEKOS_SYNCH_H

#include <geekos/kthread.h>

/*
 * mutex states
 */
enum { MUTEX_UNLOCKED, MUTEX_LOCKED };

struct Mutex {
    int state;
    struct Kernel_Thread* owner;
    struct Thread_Queue waitQueue;
};

#define MUTEX_INITIALIZER { MUTEX_UNLOCKED, 0, THREAD_QUEUE_INITIALIZER }

struct Condition {
    struct Thread_Queue waitQueue;
};

/*
 * semaphore
 */

DEFINE_LIST(Semaphore_List, Semaphore);

struct Semaphore {
    ulong_t count;
    ulong_t sid;
    char name[25];
    ulong_t refCount;
    struct Mutex mutex;
    struct Condition cond;
    DEFINE_LINK(Semaphore_List, Semaphore)
};

IMPLEMENT_LIST(Semaphore_List, Semaphore);

void Mutex_Init(struct Mutex* mutex);
void Mutex_Lock(struct Mutex* mutex);
void Mutex_Unlock(struct Mutex* mutex);

void Cond_Init(struct Condition* cond);
void Cond_Wait(struct Condition* cond, struct Mutex* mutex);
void Cond_Signal(struct Condition* cond);
void Cond_Broadcast(struct Condition* cond);

int Create_Semaphore(char* name, int ival);
int P(int sid);
int V(int sid);
int Destroy_Semaphore(int sid);

#define IS_HELD(mutex) \
    ((mutex)->state == MUTEX_LOCKED && (mutex)->owner == g_currentThread)

#endif  /* GEEKOS_SYNCH_H */
