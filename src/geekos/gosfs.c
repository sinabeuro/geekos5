/*
 * GeekOS file system
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.54 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <limits.h>
#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/bitset.h>
#include <geekos/synch.h>
#include <geekos/bufcache.h>
#include <geekos/gosfs.h>

/* ----------------------------------------------------------------------
 * Private data and functions
 * ---------------------------------------------------------------------- */
int debugGOSFS = 0;
#define Debug(args...) if (debugGOSFS) Print("debugGOSFS: " args)

struct GOSFS_File;
DEFINE_LIST(GOSFS_File_List, GOSFS_File);

typedef struct {
	Super_Block fsinfo;
	struct FS_Buffer_Cache* fscache;
	struct Mutex lock;
	struct GOSFS_File_List fileList;
} GOSFS_Instance;

struct GOSFS_File {
    DEFINE_LINK(GOSFS_File_List, GOSFS_File);
};
IMPLEMENT_LIST(GOSFS_File_List, GOSFS_File);


/* ----------------------------------------------------------------------
 * Implementation of VFS operations
 * ---------------------------------------------------------------------- */

/*
 * Get metadata for given file.
 */
static int GOSFS_FStat(struct File *file, struct VFS_File_Stat *stat)
{
    TODO("GeekOS filesystem FStat operation");
}

/*
 * Read data from current position in file.
 */
static int GOSFS_Read(struct File *file, void *buf, ulong_t numBytes)
{
    TODO("GeekOS filesystem read operation");
}

/*
 * Write data to current position in file.
 */
static int GOSFS_Write(struct File *file, void *buf, ulong_t numBytes)
{
    TODO("GeekOS filesystem write operation");
}

/*
 * Seek to a position in file.
 */
static int GOSFS_Seek(struct File *file, ulong_t pos)
{
    TODO("GeekOS filesystem seek operation");
}

/*
 * Close a file.
 */
static int GOSFS_Close(struct File *file)
{
    TODO("GeekOS filesystem close operation");
}

/*static*/ struct File_Ops s_gosfsFileOps = {
    &GOSFS_FStat,
    &GOSFS_Read,
    &GOSFS_Write,
    &GOSFS_Seek,
    &GOSFS_Close,
    0, /* Read_Entry */
};

/*
 * Stat operation for an already open directory.
 */
static int GOSFS_FStat_Directory(struct File *dir, struct VFS_File_Stat *stat)
{
    TODO("GeekOS filesystem FStat directory operation");
}

/*
 * Directory Close operation.
 */
static int GOSFS_Close_Directory(struct File *dir)
{
    TODO("GeekOS filesystem Close directory operation");
}

/*
 * Read a directory entry from an open directory.
 */
static int GOSFS_Read_Entry(struct File *dir, struct VFS_Dir_Entry *entry)
{
    TODO("GeekOS filesystem Read_Entry operation");
}

/*static*/ struct File_Ops s_gosfsDirOps = {
    &GOSFS_FStat_Directory,
    0, /* Read */
    0, /* Write */
    0, /* Seek */
    &GOSFS_Close_Directory,
    &GOSFS_Read_Entry,
};

/*
 * Open a file named by given path.
 */
static int GOSFS_Open(struct Mount_Point *mountPoint, const char *path, int mode, struct File **pFile)
{
    TODO("GeekOS filesystem open operation");
}

/*
 * Create a directory named by given path.
 */
static int GOSFS_Create_Directory(struct Mount_Point *mountPoint, const char *path)
{
    TODO("GeekOS filesystem create directory operation");
}

/*
 * Open a directory named by given path.
 */
static int GOSFS_Open_Directory(struct Mount_Point *mountPoint, const char *path, struct File **pDir)
{
    TODO("GeekOS filesystem open directory operation");
}

/*
 * Open a directory named by given path.
 */
static int GOSFS_Delete(struct Mount_Point *mountPoint, const char *path)
{
    TODO("GeekOS filesystem delete operation");
}

/*
 * Get metadata (size, permissions, etc.) of file named by given path.
 */
static int GOSFS_Stat(struct Mount_Point *mountPoint, const char *path, struct VFS_File_Stat *stat)
{
    TODO("GeekOS filesystem stat operation");
}

/*
 * Synchronize the filesystem data with the disk
 * (i.e., flush out all buffered filesystem data).
 */
static int GOSFS_Sync(struct Mount_Point *mountPoint)
{
    TODO("GeekOS filesystem sync operation");
}

/*static*/ struct Mount_Point_Ops s_gosfsMountPointOps = {
    &GOSFS_Open,
    &GOSFS_Create_Directory,
    &GOSFS_Open_Directory,
    &GOSFS_Stat,
    &GOSFS_Sync,
    &GOSFS_Delete,
};

static int GOSFS_Format(struct Block_Device *blockDev)
{
	Super_Block* super_block = (Super_Block*)Malloc(sizeof(Super_Block));
	//struct GOSFS_Dir_Entry* root_dir_entry =
	//(struct GOSFS_Dir_Entry*) Malloc(sizeof(struct GOSFS_Dir_Entry*));

	memset(super_block->bitmap, 0, sizeof(super_block->bitmap));
	Set_Bit((void*)super_block->bitmap, 0); // superblock
	Set_Bit((void*)super_block->bitmap, 1); // root dir
	Print("%d\n",Is_Bit_Set(super_block->bitmap, 1));
	super_block->size = Get_Num_Blocks(blockDev)/GOSFS_SECTORS_PER_FS_BLOCK;
	super_block->rootDirectoryPointer = 1;
	super_block->magic = GOSFS_MAGIC;	
	Block_Write(blockDev, 0, (void*)super_block);

	//Block_Read(blockDev, 0, (void*)super_block);

	//Print("magic : %x\n", super_block->magic);
    //TODO("GeekOS filesystem format operation");

    Free(super_block);
    //Free(root_dir_entry);
    return 0;
}

static int GOSFS_Mount(struct Mount_Point *mountPoint)
{
	GOSFS_Instance *instance = 0;
	Super_Block *fsinfo;
	void *superBlock = 0;
	int rc;
	int i;
	
	instance = (GOSFS_Instance*) Malloc(sizeof(*instance));
    if (instance == 0)
		goto memfail;
	memset(instance, '\0', sizeof(*instance));
	fsinfo = &instance->fsinfo;

	superBlock = Malloc(GOSFS_FS_BLOCK_SIZE);
    if (superBlock == 0)
		goto memfail;

    /* Read Super block */
    if ((rc = Block_Read(mountPoint->dev, 0, superBlock)) < 0)
		goto fail;

    /* Copy filesystem parameters from super block */
    memcpy(&instance->fsinfo, ((char*)superBlock), sizeof(Super_Block));
    
    /* Does magic number match? */
    if (fsinfo->magic != GOSFS_MAGIC) {
		Print("Bad magic number (%x) for GOSFS filesystem\n", fsinfo->magic);
		goto invalidfs;
    }
 	/* Initialize instance lock and GOSFS_File list. */
    Mutex_Init(&instance->lock);
    Clear_GOSFS_File_List(&instance->fileList);

	/*
	 * Create a cache of filesystem buffers.
	 */
    instance->fscache = Create_FS_Buffer_Cache(mountPoint->dev, fsinfo->size);

    /*
     * Success!
     * This mount point is now ready
     * to handle file accesses.
     */
    mountPoint->ops = &s_gosfsMountPointOps;
    mountPoint->fsData = instance;
    return 0;


	memfail:
		rc = ENOMEM; goto fail;
  	
    invalidfs:
    	rc = EINVALIDFS; goto fail;
   	
    fail:
    	return rc;
    	

    //TODO("GeekOS filesystem mount operation");
}

static struct Filesystem_Ops s_gosfsFilesystemOps = {
    &GOSFS_Format,
    &GOSFS_Mount,
};

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

void Init_GOSFS(void)
{
    Register_Filesystem("gosfs", &s_gosfsFilesystemOps);
}

