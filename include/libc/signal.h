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
 * User-mode signals
 * Copyright (c) 2005, Michael Hicks <mwh@cs.umd.edu>
 * $Rev$
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef SIGNAL_H
#define SIGNAL_H

#include <stddef.h>
#include <geekos/signal.h>

int Kill(int pid, int sig);
int Signal(signal_handler h, int sig);

/* For initialization of the signal subsystem */
void Def_Child_Handler(void);
int Sig_Init(void);

#endif  /* SIGNAL_H */

