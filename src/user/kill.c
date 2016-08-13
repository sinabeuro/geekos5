#include <conio.h>
#include <libuser.h>
#include <signal.h>


int main(int argc, char **argv)
{
	if (argc != 3) {
		Print("usage: %s <pid> <signum>\n", argv[0]);
		return -1;
	}
	/* pid, signum */
	Kill(atoi(argv[1]), atoi(argv[2]));
}
