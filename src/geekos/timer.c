/*
 * GeekOS timer interrupt support
 * Copyright (c) 2001,2003 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * $Revision: 1.22 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <limits.h>
#include <geekos/io.h>
#include <geekos/int.h>
#include <geekos/irq.h>
#include <geekos/kthread.h>
#include <geekos/timer.h>
#include <geekos/signal.h>

#define __VBOX__
#define MAX_TIMER_EVENTS	1

static int timerDebug = 0;
static uint_t timeEventCount;
static int nextEventID;
timerEvent pendingTimerEvents[MAX_TIMER_EVENTS];
static uint_t front = -1;
static uint_t rear = -1;

/*
 * Global tick counter
 */
volatile ulong_t g_numTicks;

/*
 * Number of times the spin loop can execute during one timer tick
 */
static int s_spinCountPerTick;

/*
 * Number of ticks to wait before calibrating the delay loop.
 */
#define CALIBRATE_NUM_TICKS	3

/*
 * The default quantum; maximum number of ticks a thread can use before
 * we suspend it and choose another.
 */
#define DEFAULT_MAX_TICKS 4

/*
 * Settable quantum.
 */
int g_Quantum = DEFAULT_MAX_TICKS;

/*
 * Ticks per second.
 * FIXME: should set this to something more reasonable, like 100.
 */
#ifdef __VBOX__
#define TICKS_PER_SEC 1000
#elif defined __BOCHS__
#define TICKS_PER_SEC 18
#endif

//#define DEBUG_TIMER
#ifdef DEBUG_TIMER
#  define Debug(args...) Print(args)
#else
#  define Debug(args...)
#endif

/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */
static void Timer_Interrupt_Handler(struct Interrupt_State* state)
{
    int i;
    struct Kernel_Thread* current = g_currentThread;

    Begin_IRQ(state);

    /* Update global and per-thread number of ticks */
    ++g_numTicks;
    ++current->numTicks;

    /* update timer events */
    for (i=0; i < timeEventCount; i++) {
    	if(pendingTimerEvents[i].id < 0)
    		continue;
    		
		if (pendingTimerEvents[i].ticks <= 0) {
			if (timerDebug) Print("timer: event %d expired (%d ticks)\n", 
			        pendingTimerEvents[i].id, pendingTimerEvents[i].origTicks);

			struct Kernel_Thread* kthread = 0;
			kthread = Lookup_Thread(pendingTimerEvents[i].pid, false);

			Send_Signal(kthread, SIGALRM);
			if(kthread->blocked == true){
				Wake_Up_Process(kthread);
			}
			if(current->pid != kthread->owner) kthread->refCount-- ; /* deref */

			pendingTimerEvents[i].id = -1;
			g_needReschedule = true;
		} 
		else {
		    pendingTimerEvents[i].ticks--;
		}
    }

    /*
     * If thread has been running for an entire quantum,
     * inform the interrupt return code that we want
     * to choose a new thread.
     */
    if (current->numTicks >= g_Quantum) {
		g_needReschedule = true;
		/*
		 * The current process is moved to a lower priority queue,
		 * since it consumed a full quantum.
		 */
        if (current->currentReadyQueue < (MAX_QUEUE_LEVEL - 1)) {
            current->currentReadyQueue++;
            Debug("process %d moved to ready queue %d\n", current->pid, current->currentReadyQueue); 
        }

    }
    End_IRQ(state);
}

/*
 * Temporary timer interrupt handler used to calibrate
 * the delay loop.
 */
static void Timer_Calibrate(struct Interrupt_State* state)
{
    Begin_IRQ(state);
    if (g_numTicks < CALIBRATE_NUM_TICKS + 100)
		++g_numTicks;
    else {
		/*
		 * Now we can look at EAX, which reflects how many times
		 * the loop has executed
		 */
		/*Print("Timer_Calibrate: eax==%d\n", state->eax);*/
		s_spinCountPerTick = (INT_MAX  - state->eax)/100;
		state->eax = 0;  /* make the loop terminate */
    }
    End_IRQ(state);
}

/*
 * Delay loop; spins for given number of iterations.
 */
static void Spin(int count)
{
    /*
     * The assembly code is the logical equivalent of
     *      while (count-- > 0) { // waste some time }
     * We rely on EAX being used as the counter
     * variable.
     */

    int result;
    __asm__ __volatile__ (
	"1: decl %%eax\n\t"
	"cmpl $0, %%eax\n\t"
	"nop; nop; nop; nop; nop; nop\n\t"
	"nop; nop; nop; nop; nop; nop\n\t"
	"jg 1b"
	: "=a" (result)
	: "a" (count)
    );
}

/*
 * Calibrate the delay loop.
 * This will initialize s_spinCountPerTick, which indicates
 * how many iterations of the loop are executed per timer tick.
 */
static void Calibrate_Delay(void)
{
    Disable_Interrupts();

    /* Install temporarily interrupt handler */
    Install_IRQ(TIMER_IRQ, &Timer_Calibrate);
    Enable_IRQ(TIMER_IRQ);

    Enable_Interrupts();

    /* Wait a few ticks */
    while (g_numTicks < CALIBRATE_NUM_TICKS)
	;

    /*
     * Execute the spin loop.
     * The temporary interrupt handler will overwrite the
     * loop counter when the next tick occurs.
     */
    Spin(INT_MAX);

    Disable_Interrupts();

    /*
     * Mask out the timer IRQ again,
     * since we will be installing a real timer interrupt handler.
     */
    Disable_IRQ(TIMER_IRQ);
    Enable_Interrupts();
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

void Init_Timer(void)
{
    /*
     * TODO: reprogram the timer to set the frequency.
     * In bochs, it defaults to 18Hz, which is actually pretty
     * reasonable.
     */
    Print("Initializing timer...\n");    
    /* configure for default clock */
    #ifdef __VBOX__
	unsigned long val = 1193180/TICKS_PER_SEC;
    Out_Byte(0x43, 0x36);
    Out_Byte(0x40, val&0xff);
    Out_Byte(0x40, (val>>8)&0xff);
	#elif defined __BOCHS__
    Out_Byte(0x43, 0x36);
    Out_Byte(0x40, 0x00);
    Out_Byte(0x40, 0x00);
	#endif

    /* Calibrate for delay loop */
    Calibrate_Delay();
    Print("Delay loop: %d iterations per tick\n", s_spinCountPerTick);

    /* Install an interrupt handler for the timer IRQ */
    Install_IRQ(TIMER_IRQ, &Timer_Interrupt_Handler);
    Enable_IRQ(TIMER_IRQ);
}

int Start_Timer(int ticks, timerCallback cb)
{
    int ret;

    KASSERT(!Interrupts_Enabled());

    //if (timeEventCount == MAX_TIMER_EVENTS) {
	//	return -1;
    //} 
    //else 
    {
		ret = nextEventID++;
		pendingTimerEvents[0].id = ret;
		pendingTimerEvents[0].callBack = cb;
		pendingTimerEvents[0].ticks = ticks;
		pendingTimerEvents[0].origTicks = ticks;
		pendingTimerEvents[0].pid = Get_Current()->pid;
		timeEventCount = 1;
		//timeEventCount++;

		return ret;
    }
}

int Get_Remaing_Timer_Ticks(int id)
{
    int i;

    KASSERT(!Interrupts_Enabled());
    for (i=0; i < timeEventCount; i++) {
		if (pendingTimerEvents[i].id == id) {
		    return pendingTimerEvents[i].ticks;
		}
    }

    return -1;
}

int Cancel_Timer(int id)
{
    int i;
    KASSERT(!Interrupts_Enabled());
    for (i=0; i < timeEventCount; i++) {
		if (pendingTimerEvents[i].id == id) {
		    pendingTimerEvents[i] = pendingTimerEvents[timeEventCount-1];
		    timeEventCount--;
		    return 0;
		}
    }

    Print("timer: unable to find timer id %d to cancel it\n", id);
    return -1;
}

#define US_PER_TICK (1000000 / TICKS_PER_SEC)

/*
 * Spin for at least given number of microseconds.
 * FIXME: I'm sure this implementation leaves a lot to
 * be desired.
 */
void Micro_Delay(int us)
{
	#if 0
    int denom = US_PER_TICK;
    uint_t k = INT_MAX / s_spinCountPerTick + 1;
    uint_t n = us / k;
    Print("n=%d\n", n);O
    uint_t m = us % k;
    Print("m=%d\n", m);
    int numSpins = n*(k*s_spinCountPerTick/US_PER_TICK); //+ m*(us-k)*s_spinCountPerTick/US_PER_TICK;
    #endif  

    int ticks = (us < US_PER_TICK)? us : us / US_PER_TICK;
    //int numSpins = ticks * s_spinCountPerTick;
    Debug("Micro_Delay(): us = %d, spinCountPerTick=%d, ticks=%d\n", us, s_spinCountPerTick, ticks);

	int i;
	for(i = 0; i < ticks; i++)
   		Spin(s_spinCountPerTick);
}
