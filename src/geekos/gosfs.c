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
#include <geekos/vfs.h>


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
	struct GOSFS_Dir_Entry rootDirEntry;
	struct Mutex lock;
	struct GOSFS_File_List fileList;
} GOSFS_Instance;

struct GOSFS_File {
    struct GOSFS_Dir_Entry *entry;	/* Directory entry of the file */
    ulong_t numBlocks;			 	/* Number of blocks used by file */
    struct Mutex lock;			 	/* Synchronize concurrent accesses */
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
    /*
     * The GOSFS_File object caching the contents of the file
     * will remain in the PFAT_Instance object, to speed up
     * future accesses to this file.
     */
    return 0;

    //TODO("GeekOS filesystem close operation");
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
 * Look up a directory entry in a GOSFS filesystem.
 */
static struct GOSFS_Dir_Entry *GOSFS_Lookup(GOSFS_Instance *instance, const char **path)
{
    Super_Block *fsinfo = &instance->fsinfo;
	struct FS_Buffer_Cache* fscache = instance->fscache;
    struct GOSFS_Dir_Entry *rootDir;
    struct GOSFS_Dir_Entry *dir;
    struct GOSFS_Dir_Entry *entry;
    struct FS_Buffer *pBuf;
    char prefix[16 + 1];
    const char *suffix;
    int i;

    KASSERT(**path == '/');

    if(Get_FS_Buffer(fscache, fsinfo->rootDirectoryPointer, &pBuf) != 0)
    {    	
		Print("Get_FS_Buffer Error.\n");
    	return -1;
    }

    rootDir = (struct GOSFS_Dir_Entry*)pBuf->data;

    /* Special case: root directory. */
    if (strcmp(*path, "/") == 0)
		return &instance->rootDirEntry;

	entry = dir = rootDir;
	while(true){
		Unpack_Path(*path, prefix, &suffix);
		Print("%s, %s\n", prefix, suffix);
		struct GOSFS_Dir_Entry *minFreeEntry = 0;
		
	    for (i = 0; i < GOSFS_DIR_ENTRIES_PER_BLOCK; ++i) {
	    	entry = &dir[i];
			if (strcmp(entry->filename, prefix) == 0 &&
				entry->flags == (GOSFS_DIRENTRY_USED | GOSFS_DIRENTRY_ISDIRECTORY)){
				*path = suffix;
				break;
			}
			// need to fix
			else if(minFreeEntry == 0 && (entry->flags & GOSFS_DIRENTRY_USED) == 0)
				minFreeEntry = entry;
	    }

	    if(i == GOSFS_DIR_ENTRIES_PER_BLOCK){ // There is no entry matched
			Print("i == GOSFS_DIR_ENTRIES_PER_BLOCK\n");
			if(*suffix == '/'){
				Print("strcmp(suffix, \"/\") == 0\n");
				return minFreeEntry;
			}
			else{
				Print("strcmp(suffix, \"/\") != 0\n");
	    		return 0; // wrong path
	    	}
	    }
	    else{ // There is entry matched
	    	Print("i != GOSFS_DIR_ENTRIES_PER_BLOCK\n");
		   	if(*suffix == '/'){ // terminal
				Print("strcmp(suffix, \"/\") == 0\n");
				return entry;
			}
			else{
				Print("strcmp(suffix, \"/\") != 0\n");
				; // check sub dir
			}
		}
    }
}

/*
 * Get a GOSFS_File object representing the file whose directory entry
 * is given.
 */
static struct GOSFS_File *Get_GOSFS_File(GOSFS_Instance *instance, struct GOSFS_Dir_Entry *entry)
{
    ulong_t numBlocks;
	struct GOSFS_File *gosfsFile = 0;

    KASSERT(entry != 0);
    KASSERT(instance != 0);

    Mutex_Lock(&instance->lock);

    /*
     * See if this file has already been opened.
     * If so, use the existing GOSFS_File object.
     */
    for (gosfsFile= Get_Front_Of_GOSFS_File_List(&instance->fileList);
			gosfsFile != 0;
	 		gosfsFile = Get_Next_In_GOSFS_File_List(gosfsFile)) {
		if (gosfsFile->entry == entry)
		    break;
    }
			
	if (gosfsFile == 0) {
		/* Determine size of data block cache for file. */
		numBlocks = Round_Up_To_Block(entry->size) / SECTOR_SIZE;

		/*
		 * Allocate File object, PFAT_File object, file block data cache,
		 * and valid cache block bitset
		 */
		if ((gosfsFile = (struct GOSFS_File *) Malloc(sizeof(*gosfsFile))) == 0 ) {
			goto memfail;
		}

		/* Populate PFAT_File */
		gosfsFile->entry = entry;
		gosfsFile->numBlocks = numBlocks;
		Mutex_Init(&gosfsFile->lock);

		/* Add to instance's list of PFAT_File objects. */
		Add_To_Back_Of_GOSFS_File_List(&instance->fileList, gosfsFile);
		KASSERT(gosfsFile->nextGOSFS_File_List == 0);
	}

	/* Success! */
	goto done;

memfail:
	if (gosfsFile != 0)
	Free(gosfsFile);

done:
	Mutex_Unlock(&instance->lock);
	return gosfsFile;

}

/*
 * Open a file named by given path.
 */
static int GOSFS_Open(struct Mount_Point *mountPoint, const char *path, int mode, struct File **pFile)
{
    int rc = 0;
	struct GOSFS_Dir_Entry* entry;
	GOSFS_Instance *instance = (GOSFS_Instance*) mountPoint->fsData;
	struct GOSFS_File *gosfsFile = 0;
	struct File *file = 0;

    /* Look up the directory entry */
    entry = GOSFS_Lookup(instance, &path);
    if (entry == 0){ /* Wrong path */
    	Print("ENOTFOUND\n");
		return ENOTFOUND;
	}

	/* There is no file, so create file */
    if (entry->flags == GOSFS_DIRENTRY_ISDIRECTORY){
   		Print("EACCESS\n");

   		strcpy(entry->filename, path);
		entry->size = 1; /* not reasonable.. */
		entry->flags = GOSFS_DIRENTRY_USED;
    	//Free(entry); 
		//return EACCESS;
	}

	/* Get GOSFS_File object */
    gosfsFile = Get_GOSFS_File(instance, entry);
    if (gosfsFile == 0)
    	goto done;

	/* Create the file object. */
	file = Allocate_File(&s_gosfsFileOps, 0, entry->size, gosfsFile, 0, 0);
	if (file == 0) {
		rc = ENOMEM;
		goto done;
	}

	/* Success! */
	*pFile = file;
	
	done:
		Print("Done\n");
		return rc;

    //TODO("GeekOS filesystem open operation");
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
	Set_Bit((void*)super_block->bitmap, GOSFS_ROOT_DIR_BLOCK); // root dir
	Print("%d\n",Is_Bit_Set(super_block->bitmap, 1));
	super_block->size = Get_Num_Blocks(blockDev)/GOSFS_SECTORS_PER_FS_BLOCK;
	super_block->rootDirectoryPointer = GOSFS_ROOT_DIR_BLOCK;
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

    /* Create the fake root directory entry. */
    memset(&instance->rootDirEntry, '\0', sizeof(struct GOSFS_Dir_Entry));
    instance->rootDirEntry.flags = GOSFS_DIRENTRY_ISDIRECTORY;
    instance->rootDirEntry.blockList[0] = fsinfo->rootDirectoryPointer;
    instance->rootDirEntry.size = 1*GOSFS_FS_BLOCK_SIZE;

 	/* Initialize instance lock and GOSFS_File list. */
    Mutex_Init(&instance->lock);
    Clear_GOSFS_File_List(&instance->fileList);

	/*
	 * Create a cache of filesystem buffers.
	 */
    instance->fscache = Create_FS_Buffer_Cache(mountPoint->dev, GOSFS_FS_BLOCK_SIZE);

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

