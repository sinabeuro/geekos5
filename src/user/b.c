/*
 * A test program for GeekOS user mode
 */

#include <conio.h>

int handler_flag;

void alarm_handler (void)
{
	static int i = 0;
	static long freq = 500;
	//Print("received signal: (%d)\n", i++);
	alarm(freq, alarm_handler);
}

int main(int argc, char** argv)
{
	#if 0
    int i;
    Print("Addr : %x\n", main);
	Print("Addr : %x\n", &i);
    Print_String("I am the b program\n");
    for (i = 0; i < argc; ++i) {
	Print("Arg %d is %s\n", i,argv[i]);
    }
    #endif

    alarm_handler();
    while(true)
    {;
    	//Print("Get key\n");
    	//Print("%d\n", Get_Key());
    }
    return 1;
}
