/*
 * ls - List file or directory
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.18 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <conio.h>
#include <fileio.h>
#include <process.h>

static void List_File(const char *filename, struct VFS_File_Stat *stat)
{
    struct VFS_ACL_Entry owner = stat->acls[0];
    bool isDir = stat->isDirectory;

    Print("%c%c%c  % 6d  % 10d  %s%s%s\n",
	isDir ? 'd' : '-',
	owner.permission & O_READ ? 'r' : '-',
	owner.permission & O_WRITE ? 'w' : '-',
	owner.uid,
	stat->size,
	isDir ? "[" : "",
	filename,
	isDir ? "]" : "");
}

int main(int argc, char **argv)
{
    int rc;
    struct VFS_File_Stat stat;
    char filename[VFS_MAX_PATH_LEN];

    if (argc != 2) {
		//Print("Usage: ls <filename>\n");
		//return 1;
		getcwd(filename, VFS_MAX_PATH_LEN);
    }
	else{
    	strcpy(filename, argv[1]);
	}
	
    rc = Stat(filename, &stat);
    if (rc != 0) {
		Print("Could not stat %s: %s\n", filename, Get_Error_String(rc));
		return 1;
    }

    if (!stat.isDirectory) {
		List_File(filename, &stat);
    } 
	else {
		int fd = Open_Directory(filename);
		struct VFS_Dir_Entry dirent;

		if (fd < 0) {
			Print("Could not open %s: %s\n", filename, Get_Error_String(fd));
			Exit(1);
		}

		Print("Directory %s\n", filename);
		for (;;) {
			int rc = Read_Entry(fd, &dirent);
			if (rc > 0)
				break;
			else if (rc == 0)
				List_File(dirent.name, &dirent.stats);
			else {
				Print("Could not read directory entry: %s\n", Get_Error_String(rc));
				Exit(1);
			}
		}

		if ((rc = Close(fd)) < 0) {
		    Print("Could not close directory %s: %s\n", filename, Get_Error_String(rc));
		    Exit(1);
		}
    }
    return 0;
}
