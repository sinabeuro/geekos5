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
 * GeekOS signals
 * Copyright (c) 2005 Michael Hicks <mwh@cs.umd.edu>
 * $Rev$
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef GEEKOS_SIGNAL_H
#define GEEKOS_SIGNAL_H

/* Signal numbers */
#define SIGKILL  1 /* can't be handled by users */
#define SIGUSR1  2
#define SIGUSR2  3
#define SIGCHLD  4

/* The largest signal number supported */
#define MAXSIG   4

/* Definition of a signal handler */
typedef void (*signal_handler)(void);

/* Default handlers */
#define SIG_DFL  (signal_handler)0
#define SIG_IGN  (signal_handler)1

#ifdef GEEKOS

struct Interrupt_State;

void Send_Signal(struct Kernel_Thread *kthread, int sig);
void Set_Handler(struct Kernel_Thread *kthread, int sig, signal_handler handler);
int Check_Pending_Signal(struct Kernel_Thread *kthread, struct Interrupt_State* esp);
void Complete_Handler(struct Kernel_Thread *kthread, struct Interrupt_State* esp);

#endif

#endif
