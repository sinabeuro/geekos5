#include <geekos/errno.h>
#include <conio.h>
#include <process.h>

int main(int argc, char **argv)
{
	struct Process_Info procInfo[50] = {0, }; 
    struct Process_Info* current = 0;
    int count = 1;
    
	PS(procInfo, 50);

 	Print("PID PPID PRIO STAT COMMAND\n");
	while(true){
		current = &procInfo[count];
		if(current->pid == 0) /* weak */
			break;
		Print("%3d %4d %4d %4c %s\n", 
				current->pid,
				current->parent_pid,
				current->priority,
				(current->status)? 'B':'R',
				(strcmp(current->name, ""))? current->name : "{kernel}"
				);
		++count;
	}


	
}
