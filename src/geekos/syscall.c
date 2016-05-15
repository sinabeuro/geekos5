/*
 * System call handlers
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003,2004 David Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.59 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/syscall.h>
#include <geekos/errno.h>
#include <geekos/kthread.h>
#include <geekos/int.h>
#include <geekos/elf.h>
#include <geekos/malloc.h>
#include <geekos/screen.h>
#include <geekos/keyboard.h>
#include <geekos/string.h>
#include <geekos/user.h>
#include <geekos/timer.h>
#include <geekos/vfs.h>


/*
 * Null system call.
 * Does nothing except immediately return control back
 * to the interrupted user program.
 * Params:
 *  state - processor registers from user mode
 *
 * Returns:
 *   always returns the value 0 (zero)
 */
static int Sys_Null(struct Interrupt_State* state)
{
    return 0;
}

/*
 * Exit system call.
 * The interrupted user process is terminated.
 * Params:
 *   state->ebx - process exit code
 * Returns:
 *   Never returns to user mode!
 */
static int Sys_Exit(struct Interrupt_State* state)
{
	Exit(state->ebx);
    //TODO("Exit system call");
}

/*
 * Print a string to the console.
 * Params:
 *   state->ebx - user pointer of string to be printed
 *   state->ecx - number of characters to print
 * Returns: 0 if successful, -1 if not
 */
static int Sys_PrintString(struct Interrupt_State* state)
{
   	char string[100] = {'\0'};
	int len = state->ecx;	
	int i;

	Copy_From_User(string, state->ebx, len);

 	Print(string);
 	return 0;
}

/*
 * Get a single key press from the console.
 * Suspends the user process until a key press is available.
 * Params:
 *   state - processor registers from user mode
 * Returns: the key code
 */
static int Sys_GetKey(struct Interrupt_State* state)
{
	return Wait_For_Key();
    //TODO("GetKey system call");
}

/*
 * Set the current text attributes.
 * Params:
 *   state->ebx - character attributes to use
 * Returns: always returns 0
 */
static int Sys_SetAttr(struct Interrupt_State* state)
{
	Set_Current_Attr(state->ebx);
    //TODO("SetAttr system call");
}

/*
 * Get the current cursor position.
 * Params:
 *   state->ebx - pointer to user int where row value should be stored
 *   state->ecx - pointer to user int where column value should be stored
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_GetCursor(struct Interrupt_State* state)
{
	int row, col;
	Get_Cursor(&row, &col);
	Copy_To_User(state->ebx, &row, sizeof(int));
	Copy_To_User(state->ecx, &col, sizeof(int));

	return 0;
    //TODO("GetCursor system call");
}

/*
 * Set the current cursor position.
 * Params:
 *   state->ebx - new row value
 *   state->ecx - new column value
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_PutCursor(struct Interrupt_State* state)
{
    TODO("PutCursor system call");
}

/*
 * Create a new user process.
 * Params:
 *   state->ebx - user address of name of executable
 *   state->ecx - length of executable name
 *   state->edx - user address of command string
 *   state->esi - length of command string
 * Returns: pid of process if successful, error code (< 0) otherwise
 */
static int Sys_Spawn(struct Interrupt_State* state)
{
	char program[256] = {'\0',}; 
	char command[256] = {'\0',};
	int pid;
	struct Kernel_Thread **pThread;
	Copy_From_User(program, state->ebx, state->ecx);
	Copy_From_User(command, state->edx, state->esi);
	
	//Print("%s\n", program);
	//Print("%s\n", command);
	Enable_Interrupts();
	pid = Spawn(program, command, pThread);
	Disable_Interrupts();
	return pid;
    //TODO("Spawn system call");
}

/*
 * Wait for a process to exit.
 * Params:
 *   state->ebx - pid of process to wait for
 * Returns: the exit code of the process,
 *   or error code (< 0) on error
 */
static int Sys_Wait(struct Interrupt_State* state)
{
	int exitcode;
	Enable_Interrupts();
	exitcode = Join(Lookup_Thread(state->ebx));
	Disable_Interrupts();
	return exitcode;

    //TODO("Wait system call");
}

/*
 * Get pid (process id) of current thread.
 * Params:
 *   state - processor registers from user mode
 * Returns: the pid of the current thread
 */
static int Sys_GetPID(struct Interrupt_State* state)
{
	return g_currentThread->pid;
}

/*
 * Set the scheduling policy.
 * Params:
 *   state->ebx - policy,
 *   state->ecx - number of ticks in quantum
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_SetSchedulingPolicy(struct Interrupt_State* state)
{
	g_Quantum = state->ecx;

	// MLF -> RR
	if(state->ebx == 0)
	{
    	Switch_To_RR();
    	return 0;
	}
   	// RR -> MLF
    else if(state->ebx == 1)
    {
		Switch_To_MLF();
		return 0;
	}
    else 
    	return -1;

   //TODO("SetSchedulingPolicy system call");
}

/*
 * Get the time of day.
 * Params:
 *   state - processor registers from user mode
 *
 * Returns: value of the g_numTicks global variable
 */
static int Sys_GetTimeOfDay(struct Interrupt_State* state)
{
    return g_numTicks;
    //TODO("GetTimeOfDay system call");
}

/*
 * Create a semaphore.
 * Params:
 *   state->ebx - user address of name of semaphore
 *   state->ecx - length of semaphore name
 *   state->edx - initial semaphore count
 * Returns: the global semaphore id
 */
static int Sys_CreateSemaphore(struct Interrupt_State* state)
{
	char name[25];
	Copy_From_User(name, state->ebx, state->ecx);
	return Create_Semaphore(name, state->edx);   
    //TODO("CreateSemaphore system call");
}

/*
 * Acquire a semaphore.
 * Assume that the process has permission to access the semaphore,
 * the call will block until the semaphore count is >= 0.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_P(struct Interrupt_State* state)
{
	return P(state->ebx);
    //TODO("P (semaphore acquire) system call");
}

/*
 * Release a semaphore.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_V(struct Interrupt_State* state)
{
	return V(state->ebx);
    //TODO("V (semaphore release) system call");
}

/*
 * Destroy a semaphore.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_DestroySemaphore(struct Interrupt_State* state)
{
	return Destroy_Semaphore(state->ebx);
    TODO("DestroySemaphore system call");
}

/*
 * Mount a filesystem.
 * Params:
 * state->ebx - contains a pointer to the Mount_Syscall_Args structure
 *   which contains the block device name, mount prefix,
 *   and filesystem type
 *
 * Returns:
 *   0 if successful, error code if unsuccessful
 */
static int Sys_Mount(struct Interrupt_State *state)
{
    int rc = 0;
    struct VFS_Mount_Request *args = 0;

    /* Allocate space for VFS_Mount_Request struct. */
    if ((args = (struct VFS_Mount_Request *) Malloc(sizeof(struct VFS_Mount_Request))) == 0) {
		rc = ENOMEM;
		goto done;
    }

    /* Copy the mount arguments structure from user space. */
    if (!Copy_From_User(args, state->ebx, sizeof(struct VFS_Mount_Request))) {
		rc = EINVALID;
		goto done;
    }

    /*
     * Hint: use devname, prefix, and fstype from the args structure
     * and invoke the Mount() VFS function.  You will need to check
     * to make sure they are correctly nul-terminated.
     */
    Enable_Interrupts();
    Mount(args->devname, args->prefix, args->fstype);
	Disable_Interrupts();
	
done:
    if (args != 0) Free(args);
    return rc;
}

/*
 * Open a file.
 * Params:
 *   state->ebx - address of user string containing path of file to open
 *   state->ecx - length of path
 *   state->edx - file mode flags
 *
 * Returns: a file descriptor (>= 0) if successful,
 *   or an error code (< 0) if unsuccessful
 */
static int Sys_Open(struct Interrupt_State *state)
{
	char path[VFS_MAX_PATH_LEN];
	struct File **pFile;
	int fd;
	int rc;
	struct File** fileList = g_currentThread->userContext->fileList;

	for(fd = 0; fd < USER_MAX_FILES; fd++){
		if(fileList[fd] == NULL){
			pFile = &fileList[fd];
			break;
		}
	}
	
	if(fd == USER_MAX_FILES)
	{
		return -1;
	}
	
	Copy_From_User(path, state->ebx, state->ecx);
    Enable_Interrupts();
	if((rc = Open(path, state->edx, pFile)) < 0){
		fd = rc;
	}
	Disable_Interrupts();

	return fd;
    //TODO("Open system call");
}

/*
 * Open a directory.
 * Params:
 *   state->ebx - address of user string containing path of directory to open
 *   state->ecx - length of path
 *
 * Returns: a file descriptor (>= 0) if successful,
 *   or an error code (< 0) if unsuccessful
 */
static int Sys_OpenDirectory(struct Interrupt_State *state)
{
	char path[VFS_MAX_PATH_LEN];
	struct File **pDir; /* important */
	int fd;
	int rc;
	struct File** fileList = g_currentThread->userContext->fileList;

	for(fd = 0; fd < USER_MAX_FILES; fd++){
		if(fileList[fd] == NULL){
			pDir = &fileList[fd];
			break;
		}
	}
	
	if(fd == USER_MAX_FILES)
	{
		return -1;
	}

	Copy_From_User(path, state->ebx, state->ecx);
    Enable_Interrupts();			
	if((rc = Open_Directory(path, pDir)) < 0){ /* important */
		fd = rc;
	}
	//Print("%d, fileList[fd] : %x\n", fd, fileList[fd]);
	Disable_Interrupts();

	return fd;
    //TODO("Open directory system call");
}

/*
 * Close an open file or directory.
 * Params:
 *   state->ebx - file descriptor of the open file or directory
 * Returns: 0 if successful, or an error code (< 0) if unsuccessful
 */
static int Sys_Close(struct Interrupt_State *state)
{
	struct File *pFile;
	int fd;
	int rc;
	struct File** fileList = g_currentThread->userContext->fileList;

	rc = Close(fileList[state->ebx]);
	fileList[state->ebx] = NULL;
	return rc;
}

/*
 * Delete a file.
 * Params:
 *   state->ebx - address of user string containing path to delete
 *   state->ecx - length of path
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_Delete(struct Interrupt_State *state)
{
	char path[VFS_MAX_PATH_LEN];
	int rc;

	Copy_From_User(path, state->ebx, state->ecx);
	Enable_Interrupts();
	if((rc = Delete(path)) < 0){
		;
	}
	Disable_Interrupts();
	return rc;

    //TODO("Delete system call");
}

/*
 * Read from an open file.
 * Params:
 *   state->ebx - file descriptor to read from
 *   state->ecx - user address of buffer to read into
 *   state->edx - number of bytes to read
 *
 * Returns: number of bytes read, 0 if end of file,
 *   or error code (< 0) on error
 */
static int Sys_Read(struct Interrupt_State *state)
{
    TODO("Read system call");
}

/*
 * Read a directory entry from an open directory handle.
 * Params:
 *   state->ebx - file descriptor of the directory
 *   state->ecx - user address of struct VFS_Dir_Entry to copy entry into
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_ReadEntry(struct Interrupt_State *state)
{
	int rc;
	struct File** fileList = g_currentThread->userContext->fileList;

	Enable_Interrupts();
	//Print("%d, fileList[state->ebx] : %x\n", state->ebx, fileList[state->ebx]);
	rc = Read_Entry(fileList[state->ebx], (struct VFS_File_Stat *)(USER_BASE_ADRR + state->ecx)); // weak
	Disable_Interrupts();
	return rc;

    //TODO("ReadEntry system call");
}

/*
 * Write to an open file.
 * Params:
 *   state->ebx - file descriptor to write to
 *   state->ecx - user address of buffer get data to write from
 *   state->edx - number of bytes to write
 *
 * Returns: number of bytes written,
 *   or error code (< 0) on error
 */
static int Sys_Write(struct Interrupt_State *state)
{
    TODO("Write system call");
}

/*
 * Get file metadata.
 * Params:
 *   state->ebx - address of user string containing path of file
 *   state->ecx - length of path
 *   state->edx - user address of struct VFS_File_Stat object to store metadata in
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_Stat(struct Interrupt_State *state)
{
	char path[VFS_MAX_PATH_LEN];
	Copy_From_User(path, state->ebx, state->ecx);
	
    Enable_Interrupts();
	Stat(path, (struct VFS_File_Stat *)(USER_BASE_ADRR + state->edx)); // weak
	Disable_Interrupts();
	return 0;
    //TODO("Stat system call");
}

/*
 * Get metadata of an open file.
 * Params:
 *   state->ebx - file descriptor to get metadata for
 *   state->ecx - user address of struct VFS_File_Stat object to store metadata in
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_FStat(struct Interrupt_State *state)
{
    TODO("FStat system call");
}

/*
 * Change the access position in a file
 * Params:
 *   state->ebx - file descriptor 
 *   state->ecx - position in file
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_Seek(struct Interrupt_State *state)
{
    TODO("Seek system call");
}

/*
 * Create directory
 * Params:
 *   state->ebx - address of user string containing path of new directory
 *   state->ecx - length of path
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_CreateDir(struct Interrupt_State *state)
{
	char path[VFS_MAX_PATH_LEN];
	int rc;

	Copy_From_User(path, state->ebx, state->ecx);
    Enable_Interrupts();
	if((rc = Create_Directory(path)) < 0){
		;
	}
	Disable_Interrupts();
	return rc;
    //TODO("CreateDir system call");
}

/*
 * Flush filesystem buffers
 * Params: none 
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_Sync(struct Interrupt_State *state)
{
    TODO("Sync system call");
}

/*
 * Format a device
 * Params:
 *   state->ebx - address of user string containing device to format
 *   state->ecx - length of device name string
 *   state->edx - address of user string containing filesystem type 
 *   state->esi - length of filesystem type string

 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_Format(struct Interrupt_State *state)
{
    char devname[BLOCKDEV_MAX_NAME_LEN];
    char fstype[VFS_MAX_FS_NAME_LEN];

    if (!Copy_From_User(devname, state->ebx, state->ecx))
		return EINVALID;

	if (!Copy_From_User(fstype, state->edx, state->esi))
		return EINVALID;

	Enable_Interrupts();
	Format(devname, fstype);
	Disable_Interrupts();
    //TODO("Format system call");
    return 0;
}

static Sys_GetCwd(struct Interrupt_State *state)
{
	char spath[VFS_MAX_PATH_LEN];
	Enable_Interrupts();
	Get_Path(Get_Cwd(), spath);
	Disable_Interrupts();
	Copy_To_User(state->ebx, spath, state->ecx);
	return 0;
}

static Sys_ChangeDir(struct Interrupt_State *state)
{
	char spath[VFS_MAX_PATH_LEN];
	struct VFS_File_Stat vfsFileStat;
	struct path *path;
	void* dentry;
	int rc = 0; 
	Copy_String_From_User(spath, state->ebx);

	/* Check whether path is vaild */
    Enable_Interrupts();
    path = Get_Cwd();
    dentry = path->dentry;
	Lookup(spath, path); /* weak */
    Free(dentry);
    
	fail:
	
	Disable_Interrupts();

	return rc;
}



/*
 * Global table of system call handler functions.
 */
const Syscall g_syscallTable[] = {
    Sys_Null,
    Sys_Exit,
    Sys_PrintString,
    Sys_GetKey,
    Sys_SetAttr,
    Sys_GetCursor,
    Sys_PutCursor,
    Sys_Spawn,
    Sys_Wait,
    Sys_GetPID,
    /* Scheduling and semaphore system calls. */
    Sys_SetSchedulingPolicy,
    Sys_GetTimeOfDay,
    Sys_CreateSemaphore,
    Sys_P,
    Sys_V,
    Sys_DestroySemaphore,
    /* File I/O system calls. */
    Sys_Mount,
    Sys_Open,
    Sys_OpenDirectory,
    Sys_Close,
    Sys_Delete,
    Sys_Read,
    Sys_ReadEntry,
    Sys_Write,
    Sys_Stat,
    Sys_FStat,
    Sys_Seek,
    Sys_CreateDir,
    Sys_Sync,
    Sys_Format,
    /* Pwd */
    Sys_GetCwd,
    Sys_ChangeDir,
};

/*
 * Number of system calls implemented.
 */
const int g_numSyscalls = sizeof(g_syscallTable) / sizeof(Syscall);
