        .text
	.globl Return_Signal
# This code snippet invokes syscall 14, to indicate that the signal
# handler has completed.  Assumes that the current stack contains the
# saved context of the code that was interrupted to handle the
# signal
#
# !!! Assumes syscall 14 is Return_Signal!
        .type   Return_Signal,@function
Return_Signal:
	movl  $14,%eax
	int   $0x90
#@endi
