/*
 * ELF executable loading
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.29 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/ktypes.h>
#include <geekos/screen.h>  /* for debug Print() statements */
#include <geekos/pfat.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/user.h>
#include <geekos/fileio.h>
#include <geekos/elf.h>


/**
 * From the data of an ELF executable, determine how its segments
 * need to be loaded into memory.
 * @param exeFileData buffer containing the executable file
 * @param exeFileLength length of the executable file in bytes
 * @param exeFormat structure describing the executable's segments
 *   and entry address; to be filled in
 * @return 0 if successful, < 0 on error
 */
int Parse_ELF_Executable(char *exeFileData, ulong_t exeFileLength,
	struct Exe_Format *exeFormat)
{
	int i = 0;
	int j = 0;
	struct Exe_Segment* segment;
	elfHeader* eHeader = (elfHeader*)exeFileData;
	programHeader* pheader = (programHeader*)(exeFileData + eHeader->phoff);
	exeFormat->numSegments = eHeader->phnum;
	exeFormat->entryAddr = eHeader->entry;
	
	//Print("\n *** Segment List *** \n\n");
	for(; i < exeFormat->numSegments; i++)
	{
		segment = &exeFormat->segmentList[i];
		segment->lengthInFile = pheader->fileSize;
		segment->offsetInFile = pheader->offset;
		segment->protFlags = pheader->flags;
		segment->sizeInMemory = pheader->memSize;
		segment->startAddress = pheader->vaddr;
	 /*
		Print("lengthInFile : %x\n", (int)(segment->lengthInFile));
		Print("offsetInFile : %x\n", (int)(segment->offsetInFile));
		Print("protFlags : %x\n", (int)(segment->protFlags));
		Print("sizeInMemory : %x\n", (int)(segment->sizeInMemory));
		Print("startAddress : %x\n\n", (int)(segment->startAddress));
	 */
		pheader = (programHeader*)((char*)pheader + eHeader->phentsize);
	}
	return 0;

	//TODO("Parse an ELF executable image");
}


