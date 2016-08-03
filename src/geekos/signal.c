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
  //TODO("implement Send_Signal");
}

/* Called when signal handling is complete. */
void Complete_Handler(struct Kernel_Thread *kthread, 
		      struct Interrupt_State* esp) {
  //TODO("implement Complete_Handler");
}

void Set_Handler(struct Kernel_Thread *kthread, int sig, signal_handler handler)
{
  //TODO("implement Set_Handler");
}

int Check_Pending_Signal(struct Kernel_Thread *kthread, struct Interrupt_State* esp)
{
  //TODO("implement Check_Pending_Signal");
  // You can make this return 0 while working on the non-delivery
  // parts of signal handling ...
  return 0;
}

#if 0
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
  Print("Setup_Frame: Stack dump\n");
  for (i = 0;i < 25;i++) {
    Print("[%x]: %x\n",(unsigned int)&esp[i] - ofs,esp[i]);
  }
}
#endif

void Setup_Frame(struct Kernel_Thread *kthread, struct Interrupt_State *esp)
{
  TODO("implement Setup_Frame");
}

