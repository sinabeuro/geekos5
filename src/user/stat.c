/*
 * stat - Stat a file or directory.
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
    int rc;
    struct VFS_File_Stat stat;
    const char *filename;

	if (argc != 2) {
	Print("Usage: stat <filename>\n");
	return 1;
    }

    filename = argv[1];

    rc = Stat(filename, &stat);
    if (rc != 0) {
		Print("Could not stat %s: %s\n", argv[1], Get_Error_String(rc));
		return 1;
    }

    struct VFS_ACL_Entry owner = stat.acls[0];
    bool isDir = stat.isDirectory;

    Print("%c%c%c  % 6d  % 10d  %s%s%s\n",
	isDir ? 'd' : '-',
	owner.permission & O_READ ? 'r' : '-',
	owner.permission & O_WRITE ? 'w' : '-',
	owner.uid,
	stat.size,
	isDir ? "[" : "",
	filename,
	isDir ? "]" : "");

    return !(rc == 0);
}
