/*
 * opendir - Open a directory
 * Copyright (c) 2003 Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2016, Beom-jin Kim <riddler117@gmail.com>
 * $Revision: 0.1 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <conio.h>
#include <process.h>
#include <fileio.h>

int main(int argc, char *argv[])
{
    int fd;

    if (argc != 2) {
        Print("Usage: opendir <file or directory>\n");
		Exit(1);
    }

    fd = Open_Directory(argv[1]);
	if (fd < 0) {
		Print("Could not open %s: %s\n", argv[1], Get_Error_String(fd));
		Exit(1);
	}
	Print("fd : %d\n", fd);
	struct VFS_Dir_Entry dirent;
	int rc = Read_Entry(fd, &dirent);
	Print("filename : %s", dirent.name);

    return 0;
}
