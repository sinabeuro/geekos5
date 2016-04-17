/*
 * Paging (virtual memory) support
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.55 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/string.h>
#include <geekos/int.h>
#include <geekos/idt.h>
#include <geekos/kthread.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/mem.h>
#include <geekos/malloc.h>
#include <geekos/gdt.h>
#include <geekos/segment.h>
#include <geekos/user.h>
#include <geekos/vfs.h>
#include <geekos/crc32.h>
#include <geekos/paging.h>

/* ----------------------------------------------------------------------
 * Public data
 * ---------------------------------------------------------------------- */
extern int sh_pid;

/* ----------------------------------------------------------------------
 * Private functions/data
 * ---------------------------------------------------------------------- */
static char* swapMap;
static uint_t totalPage;
static ulong_t startSector;
struct Block_Device* dev;

#define SECTORS_PER_PAGE (PAGE_SIZE / SECTOR_SIZE)

/*
 * flag to indicate if debugging paging code
 */
int debugFaults = 0;
#define Debug(args...) if (debugFaults) Print(args)


void checkPaging()
{
  unsigned long reg=0;
  __asm__ __volatile__( "movl %%cr0, %0" : "=a" (reg));
  Print("Paging on ? : %d\n", (reg & (1<<31)) != 0);
}


/*
 * Print diagnostic information for a page fault.
 */
static void Print_Fault_Info(uint_t address, faultcode_t faultCode)
{
    extern uint_t g_freePageCount;

    Print("Pid %d, Page Fault received, at address %x (%d pages free)\n",
        g_currentThread->pid, address, g_freePageCount);
    if (faultCode.protectionViolation)
        Print ("   Protection Violation, ");
    else
        Print ("   Non-present page, ");
    if (faultCode.writeFault)
        Print ("Write Fault, ");
    else
        Print ("Read Fault, ");
    if (faultCode.userModeFault)
        Print ("in User Mode\n");
    else
        Print ("in Supervisor Mode\n");
}

/*
 * Handler for page faults.
 * You should call the Install_Interrupt_Handler() function to
 * register this function as the handler for interrupt 14.
 */
/*static*/ void Page_Fault_Handler(struct Interrupt_State* state)
{
    ulong_t address;
    faultcode_t faultCode;

    KASSERT(!Interrupts_Enabled());

    /* Get the address that caused the page fault */
    address = Get_Page_Fault_Address();
    Debug("Page fault @%lx\n", address);
	
    /* Get the fault code */
    faultCode = *((faultcode_t *) &(state->errorCode));

    /* rest of your handling code here */
    Print ("Unexpected Page Fault received\n");
    Print_Fault_Info(address, faultCode);
    //Dump_Interrupt_State(state);

    if(faultCode.protectionViolation == 0) // Non-present page
    {
    	pde_t* pde;
    	pte_t* pte;
 		void* paddr;
 		uint_t kernelInfo;
 		int j, k = 0;
 		
		j = PAGE_DIRECTORY_INDEX(address);
		pde = &(Get_PDBR()[j]);
		if(pde->pageTableBaseAddr == '\0')
		{
			pte = (pte_t*)Alloc_Page();
			memset(pte,'\0',PAGE_SIZE);
			pde->pageTableBaseAddr = (uint_t)PAGE_ALLIGNED_ADDR(pte);
			pde->present = 1;
			pde->flags = VM_USER | VM_WRITE;
		}
		else
		{
			pte = pde->pageTableBaseAddr<<12;
		}

		k = PAGE_TABLE_INDEX(address);
		kernelInfo = pte[k].kernelInfo;
		paddr = Alloc_Pageable_Page(&pte[k], PAGE_ADDR(address));
		if(paddr == 0){
			if(g_currentThread->pid != sh_pid)
				Exit(-1);
		}
	
		if(kernelInfo == KINFO_PAGE_ON_DISK) // case 2
		{
			Print ("KINFO_PAGE_ON_DISK\n");
			Enable_Interrupts();
			Read_From_Paging_File(paddr, address, pte[k].pageBaseAddr);
			Disable_Interrupts();
			Free_Space_On_Paging_File(pte[k].pageBaseAddr);
		}
		
		pte[k].present = 1;
		pte[k].flags = VM_USER | VM_WRITE;			
		pte[k].pageBaseAddr = PAGE_ALLIGNED_ADDR(paddr);	
		
		return 0;
    }
    /* user faults just kill the process */
    if (!faultCode.userModeFault) KASSERT(0);

    /* For now, just kill the thread/process. */
    Exit(-1);
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */


/*
 * Initialize virtual memory by building page tables
 * for the kernel and physical memory.
 */
void Init_VM(struct Boot_Info *bootInfo)
{
    /*
     * Hints:
     * - Build kernel page directory and page tables
     * - Call Enable_Paging() with the kernel page directory
     * - Install an interrupt handler for interrupt 14,
     *   page fault
     * - Do not map a page at address 0; this will help trap
     *   null pointer references
     */
    extern struct Page* g_pageList;
    struct Page (*pageList)[NUM_PAGE_TABLE_ENTRIES] 
    				= (struct Page (*)[NUM_PAGE_TABLE_ENTRIES])Get_Page(0); // this equals &g_pageList
    int i, j = 0;
    uint_t memSizeB = (bootInfo->memSizeKB) << 10;
	pde_t* pde = 0;
	pte_t* pte = 0;

	pde = (pde_t*)Alloc_Page();
	memset(pde,'\0',PAGE_SIZE);
	// alloc ptable
    for (i=0; i < PAGE_DIRECTORY_INDEX(memSizeB); i++) {
		pte = (pte_t*)Alloc_Page();
		memset(pte,'\0',PAGE_SIZE);
		pde[i].pageTableBaseAddr = (uint_t)PAGE_ALLIGNED_ADDR(pte);
		pde[i].present = 1;
		pde[i].flags = VM_WRITE;
		//Print ("pde : %x\n", &pde[i]);

		for(j=0; j < NUM_PAGE_TABLE_ENTRIES; j++) {
			pageList[i][j].entry = &pte[j]; // ...
			pte[j].pageBaseAddr = i*NUM_PAGE_TABLE_ENTRIES+j;
			pte[j].present = 1;
			pte[j].flags = VM_WRITE;
		  	//Print ("pde : %x\n", pde[1]);
			//Print ("%x\n", pte[j]);
		}
	}

	Enable_Paging(pde);
	Install_Interrupt_Handler(PAGING_IRQ, Page_Fault_Handler);
	
    //TODO("Build initial kernel page directory and page tables");
}

/**
 * Initialize paging file data structures.
 * All filesystems should be mounted before this function
 * is called, to ensure that the paging file is available.
 */
void Init_Paging(void)
{
	struct Paging_Device *pagedev = Get_Paging_Device();
	dev = pagedev->dev;
	totalPage = (pagedev->numSectors)/SECTORS_PER_PAGE; 
	startSector = pagedev->startSector;
	swapMap = (char*)Malloc(totalPage);
	memset(swapMap,0,totalPage);
	
	//TODO("Initialize paging file data structures");
}

/**
 * Find a free bit of disk on the paging file for this page.
 * Interrupts must be disabled.
 * @return index of free page sized chunk of disk space in
 *   the paging file, or -1 if the paging file is full
 */
int Find_Space_On_Paging_File(void)
{
	int i;
    KASSERT(!Interrupts_Enabled());

	for(i = 0; i < totalPage; i++){
		if(swapMap[i] == 0){
			return i;
		}
	}
	return -1;
    //TODO("Find free page in paging file");
}

/**
 * Free a page-sized chunk of disk space in the paging file.
 * Interrupts must be disabled.
 * @param pagefileIndex index of the chunk of disk space
 */
void Free_Space_On_Paging_File(int pagefileIndex)
{
    KASSERT(!Interrupts_Enabled());
	swapMap[pagefileIndex] = 0;

    //TODO("Free page in paging file");
}

/**
 * Write the contents of given page to the indicated block
 * of space in the paging file.
 * @param paddr a pointer to the physical memory of the page
 * @param vaddr virtual address where page is mapped in user memory
 * @param pagefileIndex the index of the page sized chunk of space
 *   in the paging file
 */
void Write_To_Paging_File(void *paddr, ulong_t vaddr, int pagefileIndex)
{
	int i;
	block_t* block = (block_t*)paddr;
	struct Page *page = Get_Page((ulong_t) paddr);
    KASSERT(!(page->flags & PAGE_PAGEABLE)); /* Page must be locked! */
	for(i = 0; i < SECTORS_PER_PAGE; i++)
		Block_Write(dev, startSector+pagefileIndex*SECTORS_PER_PAGE+i, &block[i]);
	swapMap[pagefileIndex] = 1;
    //TODO("Write page data to paging file");
}

/**
 * Read the contents of the indicated block
 * of space in the paging file into the given page.
 * @param paddr a pointer to the physical memory of the page
 * @param vaddr virtual address where page will be re-mapped in
 *   user memory
 * @param pagefileIndex the index of the page sized chunk of space
 *   in the paging file
 */
void Read_From_Paging_File(void *paddr, ulong_t vaddr, int pagefileIndex)
{
	int i;
	block_t* block = (block_t*)paddr;
    struct Page *page = Get_Page((ulong_t) paddr);
    for(i = 0; i < SECTORS_PER_PAGE; i++)
		Block_Read(dev, startSector+pagefileIndex*SECTORS_PER_PAGE+i, &block[i]);
	//Free_Space_On_Paging_File(pagefileIndex);
    //TODO("Read page data from paging file");
}

