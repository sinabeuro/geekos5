/*************************************************************************/
/*
 * GeekOS master source distribution and/or project solution
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003 Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2004 Iulian Neamtiu <neamtiu@cs.umd.edu>
 *
 * This file is not distributed under the standard GeekOS license.
 * Publication or redistribution of this file without permission of
 * the author(s) is prohibited.
 */
/*************************************************************************/
/*
 * A test program for GeekOS user mode
 */

#include <process.h>
#include <signal.h>
#include <conio.h>

static int g = 0;
static int count = 0;

void my_handler(void) {
  int i;
  Print("In my_handler\n");
  g = 1;
  for (i=0;i<500000;i++) Null();
  Print("Leaving my_handler\n");
  return;
}

void my_handler2(void) {
  Print("In my_handler2\n");
  Print("Leaving my_handler2\n");
  return;
}

int main(int argc, char** argv)
{
  Signal(my_handler,SIGUSR1);
  Signal(my_handler2,SIGUSR2);

  Print("entering g loop\n");
  for(;g == 0;) count++;
  Print("done with g loop!\n");

  return 0;
}
