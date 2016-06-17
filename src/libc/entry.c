/*
 * User program entry point function
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.10 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/argblock.h>
#include <libuser.h>

int main(int argc, char **argv);
void Exit(int exitCode);

/*
 * Entry point.  Calls user program's main() routine, then exits.
 */
void _Entry(void)
{
    struct Argument_Block *argBlock;
    int startHeap;
    void* ptr;

    /* The argument block pointer is in the ESI register. */
    __asm__ __volatile__ ("movl %%esi, %0" : "=r" (argBlock));
	/* The end of data segment address is in the EAX register. */
    __asm__ __volatile__ ("movl %%eax, %0" : "=r" (startHeap));

    Init_Heap((void*) startHeap, 4*4096*4096);
	//Print("%x\n", Malloc(8192));

    /* Call main(), and then exit with whatever value it returns. */
    Exit(main(argBlock->argc, argBlock->argv));
}

