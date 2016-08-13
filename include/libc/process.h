/*
 * Process creation and management
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.14 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef PROCESS_H
#define PROCESS_H

#include <geekos/user.h>
#include <geekos/ktypes.h>

int Null(void);
int Exit(int exitCode);
bool Ends_With(const char *name, const char *suffix);
int Spawn_Program(const char* program, const char* command, bool bg);
int Spawn_With_Path(const char *program, const char *command, const char *path, bool bg);
int Wait(int pid);
int Get_PID(void);
int getcwd(char* buf, int size);
int chdir(const char* dirname);
void usleep(int us);
void alarm(int ms, int* cb);
int PS(struct Process_Info *ptable, int len);
int WaitNoPID(int *status);


#endif  /* PROCESS_H */

