/*
 * Synchronization primitives
 * Copyright (c) 2001,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.13 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/kthread.h>
#include <geekos/int.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/synch.h>

/*
 * NOTES:
 * - The GeekOS mutex and condition variable APIs are based on those
 *   in pthreads.
 * - Unlike disabling interrupts, mutexes offer NO protection against
 *   concurrent execution of interrupt handlers.  Mutexes and
 *   condition variables should only be used from kernel threads,
 *   with interrupts enabled.
 */

/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */

static struct Semaphore_List s_semaphoreList;
	
int Create_Semaphore(char* name, int ival)
{
	struct Semaphore* sema = (&s_semaphoreList)->head;
	static ulong_t sid = 1;
	
	//find sem by sname
	while (sema != 0)
	{
		if (strcmp(sema->name, name) == 0)
		{
			//Print("exist sema->name: %s, %d\n", sema->name, sema->sid);
			return sema->sid;
		}
		sema = Get_Next_In_Semaphore_List(sema);
	}

	//if there is no sem, create sem
	sema = (struct Semaphore*)Malloc(sizeof(struct Semaphore));
	sema->sid = sid++;
	memcpy(sema->name, name, 25);
	sema->count = ival;
	//sema->refCount = 1;
	Mutex_Init(&(sema->mutex));
	Cond_Init(&(sema->cond));
	Add_To_Back_Of_Semaphore_List(&s_semaphoreList, sema);
	//Print("new sema->name: %s, %d\n", sema->name, sema->sid);
	return sema->sid;
	
}

int Destroy_Semaphore(int sid)
{
	struct Semaphore* sema = (&s_semaphoreList)->head;

	//find sem by sid
	while (sema != 0)
	{
		if (sema->sid == sid)
		{
			Remove_From_Semaphore_List(&s_semaphoreList, sema);
			Free(sema);
			return 0;
		}
		sema = Get_Next_In_Semaphore_List(sema);
	}
	
	return -1;
}

int P(int sid)
{
	struct Semaphore* sema = (&s_semaphoreList)->head;
	struct Mutex* mutex;
	struct Condition* cond;

	//find sem by sid
	while (sema != 0)
	{
		if (sema->sid == sid)
		{
			mutex = &(sema->mutex); // important
			cond = &(sema->cond);

			Enable_Interrupts();
			Mutex_Lock(mutex);
			while(sema->count <= 0)
			{
				Cond_Wait(cond, mutex);
				//Print("WAKE UP!");
			}
			sema->count--;
			Mutex_Unlock(mutex);
			Disable_Interrupts();
			
			return 0;
		}
		sema = Get_Next_In_Semaphore_List(sema);
	}
	
	return -1;
}

int V(int sid)
{
	struct Semaphore* sema = (&s_semaphoreList)->head;
	struct Mutex* mutex;
	struct Condition* cond;

	//find sem by sid
	while (sema != 0)
	{
		if (sema->sid == sid)
		{
			mutex = &(sema->mutex);
			cond = &(sema->cond);
			
			Enable_Interrupts();
			Mutex_Lock(mutex);
			sema->count = sema->count + 1;
			//Print("wait queue : %d", cond->waitQueue);
			Cond_Broadcast(cond);
			Mutex_Unlock(mutex);
			Disable_Interrupts();
			
			return 0;
		}
		sema = Get_Next_In_Semaphore_List(sema);
	}
	
	return -1;
}

/*
 * The mutex is currently locked.
 * Atomically reenable preemption and wait in the
 * mutex's wait queue.
 */
static void Mutex_Wait(struct Mutex *mutex)
{
    KASSERT(mutex->state == MUTEX_LOCKED);
    KASSERT(g_preemptionDisabled);

    Disable_Interrupts();
    g_preemptionDisabled = false;
    Wait(&mutex->waitQueue);
    g_preemptionDisabled = true;
    Enable_Interrupts();
}

/*
 * Lock given mutex.
 * Preemption must be disabled.
 */
static __inline__ void Mutex_Lock_Imp(struct Mutex* mutex)
{
    KASSERT(g_preemptionDisabled);

    /* Make sure we're not already holding the mutex */
    KASSERT(!IS_HELD(mutex));

    /* Wait until the mutex is in an unlocked state */
    while (mutex->state == MUTEX_LOCKED) {
	Mutex_Wait(mutex);
    }

    /* Now it's ours! */
    mutex->state = MUTEX_LOCKED;
    mutex->owner = g_currentThread;
}

/*
 * Unlock given mutex.
 * Preemption must be disabled.
 */
static __inline__ void Mutex_Unlock_Imp(struct Mutex* mutex)
{
    KASSERT(g_preemptionDisabled);

    /* Make sure mutex was actually acquired by this thread. */
    KASSERT(IS_HELD(mutex));

    /* Unlock the mutex. */
    mutex->state = MUTEX_UNLOCKED;
    mutex->owner = 0;

    /*
     * If there are threads waiting to acquire the mutex,
     * wake one of them up.  Note that it is legal to inspect
     * the queue with interrupts enabled because preemption
     * is disabled, and therefore we know that no thread can
     * concurrently add itself to the queue.
     */
    if (!Is_Thread_Queue_Empty(&mutex->waitQueue)) {
	Disable_Interrupts();
	Wake_Up_One(&mutex->waitQueue);
	Enable_Interrupts();
    }
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

/*
 * Initialize given mutex.
 */
void Mutex_Init(struct Mutex* mutex)
{
    mutex->state = MUTEX_UNLOCKED;
    mutex->owner = 0;
    Clear_Thread_Queue(&mutex->waitQueue);
}

/*
 * Lock given mutex.
 */
void Mutex_Lock(struct Mutex* mutex)
{
    KASSERT(Interrupts_Enabled());

    g_preemptionDisabled = true;
    Mutex_Lock_Imp(mutex);
    g_preemptionDisabled = false;
}

/*
 * Unlock given mutex.
 */
void Mutex_Unlock(struct Mutex* mutex)
{
    KASSERT(Interrupts_Enabled());

    g_preemptionDisabled = true;
    Mutex_Unlock_Imp(mutex);
    g_preemptionDisabled = false;
}

/*
 * Initialize given condition.
 */
void Cond_Init(struct Condition* cond)
{
    Clear_Thread_Queue(&cond->waitQueue);
}

/*
 * Wait on given condition (protected by given mutex).
 */
void Cond_Wait(struct Condition* cond, struct Mutex* mutex)
{
    KASSERT(Interrupts_Enabled());

    /* Ensure mutex is held. */
    KASSERT(IS_HELD(mutex));

    /* Turn off scheduling. */
    g_preemptionDisabled = true;

    /*
     * Release the mutex, but leave preemption disabled.
     * No other threads will be able to run before this thread
     * is able to wait.  Therefore, this thread will not
     * miss the eventual notification on the condition.
     */
    Mutex_Unlock_Imp(mutex);

    /*
     * Atomically reenable preemption and wait in the condition wait queue.
     * Other threads can run while this thread is waiting,
     * and eventually one of them will call Cond_Signal() or Cond_Broadcast()
     * to wake up this thread.
     * On wakeup, disable preemption again.
     */
    Disable_Interrupts();
    g_preemptionDisabled = false;
    Wait(&cond->waitQueue);
    g_preemptionDisabled = true;
    Enable_Interrupts();

    /* Reacquire the mutex. */
    Mutex_Lock_Imp(mutex);

    /* Turn scheduling back on. */
    g_preemptionDisabled = false;
}

/*
 * Wake up one thread waiting on the given condition.
 * The mutex guarding the condition should be held!
 */
void Cond_Signal(struct Condition* cond)
{
    KASSERT(Interrupts_Enabled());
    Disable_Interrupts();  /* prevent scheduling */
    Wake_Up_One(&cond->waitQueue);
    Enable_Interrupts();  /* resume scheduling */
}

/*
 * Wake up all threads waiting on the given condition.
 * The mutex guarding the condition should be held!
 */
void Cond_Broadcast(struct Condition* cond)
{
    KASSERT(Interrupts_Enabled());
    Disable_Interrupts();  /* prevent scheduling */
    Wake_Up(&cond->waitQueue);
    Enable_Interrupts();  /* resume scheduling */
}
