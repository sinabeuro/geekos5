/*
 * Process creation and management
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.17 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <stddef.h>
#include <geekos/ktypes.h>
#include <geekos/syscall.h>
#include <geekos/errno.h>
#include <string.h>
#include <process.h>

/* System call wrappers */
DEF_SYSCALL(Null,SYS_NULL,int,(void),,SYSCALL_REGS_0)
DEF_SYSCALL(Exit,SYS_EXIT,int,(int exitCode), int arg0 = exitCode;, SYSCALL_REGS_1)
DEF_SYSCALL(Spawn_Program,SYS_SPAWN,int,
    (const char *program, const char *command, bool bg),
    const char *arg0 = program; size_t arg1 = strlen(program); const char *arg2 = command; size_t arg3 = strlen(command);
    bool arg4 = bg;, SYSCALL_REGS_5)
DEF_SYSCALL(Wait,SYS_WAIT,int,(int pid),int arg0 = pid;,SYSCALL_REGS_1)
DEF_SYSCALL(Get_PID,SYS_GETPID,int,(void),,SYSCALL_REGS_0)	
DEF_SYSCALL(getcwd,SYS_GETCWD,int,(char* buf, int size), char *arg0 = buf; int arg1 = size;, SYSCALL_REGS_2)
DEF_SYSCALL(chdir,SYS_CHDIR,int,(const char* dirname), char *arg0 = dirname;, SYSCALL_REGS_1)
DEF_SYSCALL(usleep,SYS_USLEEP,void,(int us), char *arg0 = us;, SYSCALL_REGS_1)
DEF_SYSCALL(alarm,SYS_ALARM,void,(int us, int* cb), int arg0 = us; int *arg1 = cb;, SYSCALL_REGS_2)
DEF_SYSCALL(PS,SYS_PS,int,(struct Process_Info *ptable, int len),struct Process_Info *arg0 = ptable; int arg1 = len;,SYSCALL_REGS_2)
DEF_SYSCALL(WaitNoPID,SYS_WAITNOPID,int,(int *status),int *arg0 = status;,SYSCALL_REGS_1)

#define CMDLEN 79

bool Ends_With(const char *name, const char *suffix)
{
    size_t nameLen = strlen(name);
    size_t suffixLen = strlen(suffix);
    size_t start, i;

    if (suffixLen > nameLen)
		return false;
    start = nameLen - suffixLen;

    for (i = 0; i < suffixLen; ++i) {
		if (name[start + i] != suffix[i])
		    return false;
    }
    return true;
}

int Spawn_With_Path(const char *program, const char *command,
    const char *path, bool bg)
{
    int pid;
    char exeName[(CMDLEN*2)+5];

    /* Try executing program as specified */
    pid = Spawn_Program(program, command, bg);

    if (pid == ENOTFOUND && strchr(program, '/') == 0) {
		/* Search for program on path. */
		for (;;) {
		    char *p;

		    while (*path == ':')
				++path;

		    if (strcmp(path, "") == 0)
				break;

		    p = strchr(path, ':');
		    if (p != 0) {
				memcpy(exeName, path, p - path);
				exeName[p - path] = '\0';
				path = p + 1;
		    } else {
				strcpy(exeName, path);
				path = "";
		    }

		    strcat(exeName, "/");
		    strcat(exeName, program);

		    if (!Ends_With(exeName, ".exe"))
			strcat(exeName, ".exe");

		    /*Print("exeName=%s\n", exeName);*/
		    pid = Spawn_Program(exeName, command, bg);
		    if (pid != ENOTFOUND)
			break;
		}
    }

    return pid;
}

