/*
 * A user mode program which uses deep recursion
 * to test stack growth.
 */

#include <conio.h>
#include <string.h>

int Recurse(int x)
{
    int stuff[512];

    if (x == 0) return 0;

    stuff[0] = x;
    Print("Calling Recurse %d\n", x);
    return Recurse(x-1) + 1;
}

int main(int argc, char **argv)
{
    /* change recurse to 5-10 to see stack faults without page outs */
    int depth = 512;

    if (argc > 1) {
	depth = atoi(argv[1]);
	Print("Depth is %d\n", depth);
    }

    Print("Result is %d\n", Recurse(depth));

    return 0;
}

