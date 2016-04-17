/*
 * Paging-based user mode implementation
 * Copyright (c) 2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.50 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/int.h>
#include <geekos/mem.h>
#include <geekos/paging.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/argblock.h>
#include <geekos/kthread.h>
#include <geekos/range.h>
#include <geekos/vfs.h>
#include <geekos/user.h>
#include <geekos/segment.h>
#include <geekos/gdt.h>

#define DEFAULT_USER_STACK_SIZE 4096
#define END_OF_VM 0xFFFFFFFF

/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */
/*
 * flag to indicate if debugging paging code
 */
extern int debugFaults;
#define Debug(args...) if (debugFaults) Print(args)

// TODO: Add private functions
/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */
extern void Load_LDTR(ushort_t LDTselector);

ulong_t dtob(ulong_t decimal){

	if(decimal <= 1)
		return decimal;
		
	return 10*dtob(decimal/2)+decimal%2;
}

void DisplayMemory(pde_t* pde)
{
	int i;
	char binary[32];
	pte_t* pte = 0;
	Set_Current_Attr(ATTRIB(BLACK, AMBER|BRIGHT));
	Print("Page Directory\n");
	Set_Current_Attr(ATTRIB(BLACK, GRAY));	
	Print("%10s\t%10s\n", "pde", "value" );
	Print(	"%10x\t%10x\n", 
			&pde[PAGE_DIRECTORY_INDEX(0xFFFFFFFF)], 
			pde[PAGE_DIRECTORY_INDEX(0xFFFFFFFF)]);
	Set_Current_Attr(ATTRIB(BLACK, AMBER|BRIGHT));
	Print("Page Table\n");
	Set_Current_Attr(ATTRIB(BLACK, GRAY));
	Print("%4s\t%10s\t%10s\t%10s\t%10s\n", "idx", "pte addr", "pte value", "pf addr", "pf value" );
	pte = pde[PAGE_DIRECTORY_INDEX(0xFFFFFFFF)].pageTableBaseAddr<<12;
	for(i=1022; i<1024; i++)
	{
		Print(	"%10d\t%10x\t%10x\t%10x\t%10x\n",
				i, 
				&pte[i], 
				pte[i], 
				pte[i].pageBaseAddr<<12,
				*(int*)(pte[i].pageBaseAddr<<12));
	}
}

/*
 * Destroy a User_Context object, including all memory
 * and other resources allocated within it.
 */
void Destroy_User_Context(struct User_Context* context)
{
    /*
     * Hints:
     * - Free all pages, page tables, and page directory for
     *   the process (interrupts must be disabled while you do this,
     *   otherwise those pages could be stolen by other processes)
     * - Free semaphores, files, and other resources used
     *   by the process
     */

    // this function called in kernel mode
    int i, j;
    pde_t* pde = context->pageDir;
	pte_t* pte;

	for(i = PAGE_DIRECTORY_INDEX(USER_BASE_ADRR); i < NUM_PAGE_DIR_ENTRIES; ++i)
	{
		if(pde[i].pageTableBaseAddr == '\0')
			continue;
		pte = pde[i].pageTableBaseAddr<<12; // pte is not pageable
		for(j=0; j < NUM_PAGE_TABLE_ENTRIES; ++j)
		{
			Disable_Interrupts();
			if(pte[j].present == 0 && pte[j].kernelInfo == KINFO_PAGE_ON_DISK)
			{
				// this page is in the swap space
				Free_Space_On_Paging_File(pte[j].pageBaseAddr);
			}
			else if(pte[j].present == 1)
			{
				//Print("free page : %x\n", pte[j].pageBaseAddr<<12);
				Free_Page(pte[j].pageBaseAddr<<12);
			}
			Enable_Interrupts();
		}
		Free_Page(pte);		
	}
	Free_Page(pde);	

	//Print("complete to free page\n");
    Free_Segment_Descriptor(context->ldtDescriptor);
    //Free(context->memory);
    Free(context);
	return 0; 

    //TODO("Destroy User_Context data structure after process exits");
}

/*
 * Load a user executable into memory by creating a User_Context
 * data structure.
 * Params:
 * exeFileData - a buffer containing the executable to load
 * exeFileLength - number of bytes in exeFileData
 * exeFormat - parsed ELF segment information describing how to
 *   load the executable's text and data segments, and the
 *   code entry point address
 * command - string containing the complete command to be executed:
 *   this should be used to create the argument block for the
 *   process
 * pUserContext - reference to the pointer where the User_Context
 *   should be stored
 *
 * Returns:
 *   0 if successful, or an error code (< 0) if unsuccessful
 */
int Load_User_Program(char *exeFileData, ulong_t exeFileLength,
    struct Exe_Format *exeFormat, const char *command,
    struct User_Context **pUserContext)
{
    /*
     * Hints:
     * - This will be similar to the same function in userseg.c
     * - Determine space requirements for code, data, argument block,
     *   and stack
     * - Allocate pages for above, map them into user address
     *   space (allocating page directory and page tables as needed)
     * - Fill in initial stack pointer, argument block address,
     *   and code entry point fields in User_Context
     */
	struct Segment_Descriptor* desc;
	unsigned long virtSize;
	unsigned short LDTSelector;
	unsigned short codeSelector, dataSelector;
	int i, j, k;
	ulong_t maxva = 0;
	unsigned numArgs;
	ulong_t argBlockSize;
	unsigned long stackPointerAddr;
	ulong_t vaddr = 0;
	void* paddr = 0;

	/* Determine space requirements argument block */
	Get_Argument_Block_Size(command, &numArgs, &argBlockSize);

	/* Find maximum virtual address */
	for (i = 0; i < exeFormat->numSegments; ++i) {
	  struct Exe_Segment *segment = &exeFormat->segmentList[i];
	  ulong_t topva = segment->startAddress + segment->sizeInMemory; 
	  
	  if (topva > maxva)
		maxva = topva;
	}
	
	/* Setup some memory space for the program */
	virtSize = Round_Up_To_Page(maxva);
	stackPointerAddr = PAGE_ADDR(END_OF_VM);
	
    pde_t* base_pde = 0;
	pde_t* pde = 0;
	pte_t* base_pte = 0;
	pte_t* pte = 0;

	/* Copy all of the mappings from the kernel mode page directory */ 
	base_pde = (pde_t*)Alloc_Page();
	memset(base_pde,'\0',PAGE_SIZE); /* Null char means that there is no entry */
	memcpy(base_pde, Get_PDBR(), PAGE_SIZE/2); // very important
	
	/* Alloc user page dir entry(start from 0x80000000) */
	for(i=0; i < exeFormat->numSegments; ++i){
		struct Exe_Segment *segment = &exeFormat->segmentList[i];
		ulong_t startAddress = USER_BASE_ADRR + segment->startAddress;
		ulong_t offsetInFile = segment->offsetInFile;	
		Debug("\nSegment %d : \n", i);
		Debug( "%6s %-12s%-15s%-15s\n",
			   "Idx", "Start VAddr", "Offset In File", "Sample Data" );
		Debug( "%6d %-12x%-15x0x%-15x\n",
			   PAGE_DIRECTORY_INDEX(startAddress), startAddress,
			   offsetInFile, *(int*)(exeFileData + offsetInFile));
		pde = &base_pde[PAGE_DIRECTORY_INDEX(startAddress)];
		/* Alloc page table */
	    for(j=0; j<=PAGE_DIRECTORY_INDEX(segment->lengthInFile); j++) 
	    {
	    	/* Page table is not exist */
	    	if(pde[j].pageTableBaseAddr == '\0') 
	    	{
				base_pte = (pte_t*)Alloc_Page();
			 	memset(base_pte,'\0',PAGE_SIZE);
			}
			else
			{
				base_pte = pde[j].pageTableBaseAddr<<12;
			}
			
			pde[j].pageTableBaseAddr = (uint_t)PAGE_ALLIGNED_ADDR(base_pte);
			pde[j].present = 1;
			pde[j].flags = VM_USER | VM_WRITE;

			pte = &base_pte[PAGE_TABLE_INDEX(startAddress)];
			Debug( "\n%6s %-12s%-15s%-15s%-15s\n",
			   "Idx", "VAddr", "PAddr", "Offset", "Sample Data");

			/* Alloc page table entry and map to physical address */
			for(k=0;
				k <= PAGE_TABLE_INDEX(segment->lengthInFile); k++)
			{
				vaddr = PAGE_ADDR(startAddress + PAGE_ADDR_BY_IDX(j, k)); // vaddr?
				paddr = Alloc_Pageable_Page(&pte[k], vaddr);
				pte[k].pageBaseAddr = PAGE_ALLIGNED_ADDR(paddr); 
				memcpy(paddr,
					   exeFileData + PAGE_ADDR(segment->offsetInFile + PAGE_ADDR_BY_IDX(j, k)),
					   PAGE_SIZE);
				Debug( "%6d %-12x%-15x%-15x%-15x\n", k, vaddr, paddr,
						PAGE_ADDR(segment->offsetInFile + PAGE_ADDR_BY_IDX(j, k)),
						*(int*)paddr);
				pte[k].present = 1;
				pte[k].flags = VM_USER | VM_WRITE;
			}
		}
	}

	// Alloc stack..
	j = PAGE_DIRECTORY_INDEX(stackPointerAddr);
	pde = &base_pde[j];
	pte = (pte_t*)Alloc_Page();
	memset(pte,'\0',PAGE_SIZE);
	pde->pageTableBaseAddr = (uint_t)PAGE_ALLIGNED_ADDR(pte);
	pde->present = 1;
	pde->flags = VM_USER | VM_WRITE;

	// support up to 4MB
	for(k = NUM_PAGE_TABLE_ENTRIES-PAGE_TABLE_INDEX(DEFAULT_USER_STACK_SIZE + argBlockSize)-1; 
		k < NUM_PAGE_TABLE_ENTRIES; k++){
		vaddr = PAGE_ADDR_BY_IDX(j, k);
		paddr = Alloc_Pageable_Page(&pte[k], vaddr);
		pte[k].pageBaseAddr = PAGE_ALLIGNED_ADDR(paddr);    
		pte[k].present = 1;
		pte[k].flags = VM_USER | VM_WRITE;
		//Print("stack VA : %x\n", vaddr);
	}

	stackPointerAddr -= USER_BASE_ADRR; // This means virtual(logical) addr
	Format_Argument_Block(	paddr, numArgs, 
							stackPointerAddr, command); 

	*pUserContext = (struct User_Context*)Malloc(sizeof(struct User_Context));
	(*pUserContext)->memory = (char*)NULL; // i dont know..
	(*pUserContext)->size = virtSize;
	(*pUserContext)->entryAddr = exeFormat->entryAddr;
	(*pUserContext)->stackPointerAddr = stackPointerAddr;
	(*pUserContext)->argBlockAddr = stackPointerAddr; // just start from stack pointer
	(*pUserContext)->refCount = 0; // important
	(*pUserContext)->pageDir = base_pde; // important


	// setup LDT
	// alloc LDT seg desc in GDT
	desc = Allocate_Segment_Descriptor();
	Init_LDT_Descriptor(
					 desc,
					 ((*pUserContext)->ldt), // base address
					 2  // num pages
					 );
	LDTSelector = Selector(KERNEL_PRIVILEGE, true, Get_Descriptor_Index( desc )); //
	(*pUserContext)->ldtDescriptor = desc;
	(*pUserContext)->ldtSelector = LDTSelector;

	desc = &((*pUserContext)->ldt)[0];
	Init_Code_Segment_Descriptor(
					 desc,
					 (unsigned long)USER_BASE_ADRR, // base address
					 (USER_BASE_ADRR/PAGE_SIZE),  // need to modify
					 USER_PRIVILEGE		   // privilege level (0 == kernel)
					 );
	codeSelector = Selector(USER_PRIVILEGE, false, 0);
	(*pUserContext)->csSelector = codeSelector;

	desc = &((*pUserContext)->ldt)[1];
	Init_Data_Segment_Descriptor(
					 desc,
					 (unsigned long)USER_BASE_ADRR, // base address
					 (USER_BASE_ADRR/PAGE_SIZE),  // num pages
					 USER_PRIVILEGE		   // privilege level (0 == kernel)
					 );
	dataSelector = Selector(USER_PRIVILEGE, false, 1);
	(*pUserContext)->dsSelector = dataSelector;	
	
	//Print("virtSize : %d\n", exeFormat->entryAddr);
	//DisplayMemory(base_pde);
	//TODO("");
	return 0;
}

/*
 * Copy data from user buffer into kernel buffer.
 * Returns true if successful, false otherwise.
 */
bool Copy_From_User(void* destInKernel, ulong_t srcInUser, ulong_t numBytes)
{
    /*
     * Hints:
     * - Make sure that user page is part of a valid region
     *   of memory
     * - Remember that you need to add 0x80000000 to user addresses
     *   to convert them to kernel addresses, because of how the
     *   user code and data segments are defined
     * - User pages may need to be paged in from disk before being accessed.
     * - Before you touch (read or write) any data in a user
     *   page, **disable the PAGE_PAGEABLE bit**.
     *
     * Be very careful with race conditions in reading a page from disk.
     * Kernel code must always assume that if the struct Page for
     * a page of memory has the PAGE_PAGEABLE bit set,
     * IT CAN BE STOLEN AT ANY TIME.  The only exception is if
     * interrupts are disabled; because no other process can run,
     * the page is guaranteed not to be stolen.
     */
    //Print("Source address : %x\n", srcInUser); 
    //KASSERT(PAGE_ADDR(USER_BASE_ADRR + srcInUser))
    struct User_Context* userContext = g_currentThread->userContext;
	//struct Page* page = Get_Page(USER_BASE_ADRR + srcInUser);

	KASSERT(!Interrupts_Enabled());
    memcpy(destInKernel, (void*)(USER_BASE_ADRR + srcInUser), numBytes); // because kernel mode

    return true;    
    //TODO("Copy user data to kernel buffer");
}

/*
 * Copy data from kernel buffer into user buffer.
 * Returns true if successful, false otherwise.
 */
bool Copy_To_User(ulong_t destInUser, void* srcInKernel, ulong_t numBytes)
{
    /*
     * Hints:
     * - Same as for Copy_From_User()
     * - Also, make sure the memory is mapped into the user
     *   address space with write permission enabled
     */
	struct User_Context* userContext = g_currentThread->userContext;
	
	memcpy((void*)(USER_BASE_ADRR + destInUser), srcInKernel, numBytes);
	//Print("%x, %x\n", (USER_BASE_ADRR + destInUser), srcInKernel);
	return true;
     
    //TODO("Copy kernel data to user buffer");
}

/*
 * Switch to user address space.
 */
void Switch_To_Address_Space(struct User_Context *userContext)
{
    /*
     * - If you are still using an LDT to define your user code and data
     *   segments, switch to the process's LDT
     * - 
     */

	Load_LDTR(userContext->ldtSelector);
	Set_PDBR(userContext->pageDir);
	
	//Print("0x1000: %x\n", *(int*)(0x34000));

	
}


