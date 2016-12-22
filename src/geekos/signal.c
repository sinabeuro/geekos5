/*************************************************************************/
/*
 * GeekOS master source distribution and/or project solution
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003 Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 *
 * This file is not distributed under the standard GeekOS license.
 * Publication or redistribution of this file without permission of
 * the author(s) is prohibited.
 */
/*************************************************************************/
/*
 * Signals
 * $Rev $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/kassert.h>
#include <geekos/defs.h>
#include <geekos/screen.h>
#include <geekos/int.h>
#include <geekos/mem.h>
#include <geekos/symbol.h>
#include <geekos/string.h>
#include <geekos/kthread.h>
#include <geekos/malloc.h>
#include <geekos/user.h>
#include <geekos/signal.h>


void Send_Signal(struct Kernel_Thread *kthread, int sig)
{
	kthread->userContext->signal = sig;
	//TODO("implement Send_Signal");
}

/* Called when signal handling is complete. */
void Complete_Handler(struct Kernel_Thread *kthread, struct Interrupt_State* esp)
{
	void **p = (void **)(((struct Interrupt_State *)esp)+1);
 	//Print("esp+n[0]=%x\n",(unsigned int)p[0]);
	//Print_IS(p[0]+USER_BASE_ADDR);
	memcpy(esp, p[0]+USER_BASE_ADDR, sizeof(struct Interrupt_State)+2*sizeof(uint_t));
	
	//TODO("implement Complete_Handler");
}

void Set_Handler(struct Kernel_Thread *kthread, int sig, signal_handler handler)
{
	kthread->userContext->saHandler[sig] = handler;
	//TODO("implement Set_Handler");
}

int Check_Pending_Signal(struct Kernel_Thread *kthread, struct Interrupt_State* esp)
{
	static int i = 0;
	//TODO("implement Check_Pending_Signal");
	// You can make this return 0 while working on the non-delivery
	// parts of signal handling ...

	if(kthread->pid != i && (kthread->pid == 8)) {
		//Print("%d, [%x]: %x\n", kthread->pid, 0xffffef84, *((unsigned int*)0xffffef84));
		//TODO("");
	}
	//Print("Check_Pending_Signal, pid : (%d)\n", kthread->pid);
	
	i = kthread->pid;
	KASSERT(!Interrupts_Enabled());
	/*
	 * The process is about to start executing in user space. 
	 * if it is not the kernel's cs register then the process is about to return to user space.
	 */
	if(esp->cs == KERNEL_CS)
	{
		if(!kthread->userContext)
			return 0;

		//Print_IS( (char*)kthread->stackPage + PAGE_SIZE - (sizeof(struct Interrupt_State)+2*sizeof(uint_t)));
		//Print("pid :%d, %d\n", kthread->pid, kthread->userContext->signal);
		if(kthread->userContext->signal != 0 && kthread->waitQueue){
			//Print("Check_Pending_Signal START: %d\n", kthread->userContext->signal);
			//Print_IS(esp);
			//TODO("");
			return kthread->userContext->signal;
		}
		return 0;
	}
	else{
		if(kthread->userContext)
		{
			//Print("pid :%d, %d\n", kthread->pid, kthread->userContext->signal);
			if(kthread->userContext->signal)
			{
				//Print("Case B, (%d)\n", kthread->pid);
				return kthread->userContext->signal;
			}
		}
	}

	return 0;
  	
}

#if 1
void Print_IS(struct Interrupt_State *esp) {
  void **p;
  Print("esp=%x:\n",(unsigned int)esp);
  Print("  gs=%x\n",(unsigned int)esp->gs);
  Print("  fs=%x\n",(unsigned int)esp->fs);
  Print("  es=%x\n",(unsigned int)esp->es);
  Print("  ds=%x\n",(unsigned int)esp->ds);
  Print("  ebp=%x\n",(unsigned int)esp->ebp);
  Print("  edi=%x\n",(unsigned int)esp->edi);
  Print("  esi=%x\n",(unsigned int)esp->esi);
  Print("  edx=%x\n",(unsigned int)esp->edx);
  Print("  ecx=%x\n",(unsigned int)esp->ecx);
  Print("  ebx=%x\n",(unsigned int)esp->ebx);
  Print("  eax=%x\n",(unsigned int)esp->eax);
  Print("  intNum=%x\n",(unsigned int)esp->intNum);
  Print("  errorCode=%x\n",(unsigned int)esp->errorCode);
  Print("  eip=%x\n",(unsigned int)esp->eip);
  Print("  cs=%x\n",(unsigned int)esp->cs);
  Print("  eflags=%x\n",(unsigned int)esp->eflags);
  p = (void **)(((struct Interrupt_State *)esp)+1);
  Print("esp+n=%x\n",(unsigned int)p);
  Print("esp+n[0]=%x\n",(unsigned int)p[0]);
  Print("esp+n[1]=%x\n",(unsigned int)p[1]);
}

void dump_stack(unsigned int* esp, unsigned int ofs) {
  int i;
  for (i = 0;i < 15;i++) {
    Print("[%x]: %x\n",(unsigned int)&esp[i] - ofs,esp[i]);
  }
}
#endif

void Setup_Frame(struct Kernel_Thread *kthread, struct Interrupt_State *esp)
{	
	volatile void **p = 0;
	volatile char* userStackPtr = 0;
	volatile char* kernStackPtr = 0;
	volatile char* userStackDs = 0;
	//Print("Setup_Frame, pid : (%d)\n", kthread->pid);
	
	KASSERT(!Interrupts_Enabled());
	//Print_IS( (char*)kthread->stackPage + PAGE_SIZE - (sizeof(struct Interrupt_State)+2*sizeof(uint_t)));

	//Print("1.esp=%x\n", esp);
	//esp = (char*)kthread->stackPage + PAGE_SIZE - (sizeof(struct Interrupt_State)+2*sizeof(uint_t));
	#if 1
	if(kthread->waitQueue){
		memcpy(esp, (char*)kthread->stackPage + PAGE_SIZE - (sizeof(struct Interrupt_State)+2*sizeof(uint_t)),
		(sizeof(struct Interrupt_State)+2*sizeof(uint_t)));
		//Print("2.esp=%x\n", esp );
	}
	#endif
	//Print("2.esp=%x\n", esp );
	//Print("%s", (char*)(kthread->userContext->stackPointerAddr));
	p = (void **)(((struct Interrupt_State *)esp)+1);
	userStackPtr = (char*)(USER_BASE_ADDR + (unsigned int)p[0]);
	userStackDs = (char*)((unsigned int)p[1]);

 	userStackPtr -= sizeof(ulong_t);
    *((ulong_t *) userStackPtr) = userStackDs;

    userStackPtr -= sizeof(ulong_t);
    *((ulong_t *) userStackPtr) = p[0];
    
	userStackPtr -= sizeof(struct Interrupt_State);
	
	memcpy(userStackPtr, esp, sizeof(struct Interrupt_State));

	//Print("%s", (char*)(kthread->userContext->stackPointerAddr));
    //Print("%s", (char*)(kthread->userContext->stackPointerAddr));

    userStackPtr -= sizeof(signal_handler*);
    *((signal_handler *) userStackPtr) = kthread->userContext->returnSignal;

	//Print("returnSignal %x : %x\n", userStackPtr, *((signal_handler *) userStackPtr));
    //kernStackPtr = ((char*)kthread->stackPage) + PAGE_SIZE - sizeof(struct Interrupt_State);
    // need to analyze
    esp->eip = kthread->userContext->saHandler[kthread->userContext->signal];
    //Print("%x\n", esp->eip);
    //Print("addr=%x\n", p[0]);

    *((ulong_t *)&p[0]) = userStackPtr - USER_BASE_ADDR;
    kthread->userContext->signal = 0;
    //Print("addr=%x\n", p[0]);
	//Print("%x,\n", *((ulong_t *)0x7ffff010));
    //Print("%x : %x\n", kernStackPtr, *((ulong_t *) kernStackPtr));
    //Print("%x : %x\n", (char*)esp+2*sizeof(ulong_t), *((ulong_t *)((char*)esp+2*sizeof(ulong_t))));
	kthread->waitQueue = NULL;

	//Print("[%x]: %x\n", userStackPtr, *((unsigned int*)userStackPtr));
	//dump_stack(userStackPtr, 0);
	//TODO("");

}

