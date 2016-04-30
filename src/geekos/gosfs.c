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
	struct FS_Buffer * fsinfo; /* Superblock */
	struct FS_Buffer_Cache* fscache;
	struct GOSFS_Dir_Entry rootDirEntry;
	struct Mutex lock;
	struct GOSFS_File_List fileList;
} GOSFS_Instance;

struct GOSFS_File {
    struct GOSFS_Dir_Entry *entry;	/* Directory entry of the file */
    struct FS_Buffer *pBuf;			/* Buffer of file */
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
static int GOSFS_Lookup(GOSFS_Instance *instance, Path_Info* pathInfo)
{
    Super_Block *superBlock = (Super_Block *)instance->fsinfo->data;
	struct FS_Buffer_Cache* fscache = instance->fscache;
    struct GOSFS_Dir_Entry* dir, *entry;
    struct FS_Buffer *pBuf;
    char prefix[MAX_PREFIX_LEN + 1];
    char *suffix = 0;
    int i;
    char* path = pathInfo->path;
	int retval = 0;
	int base = superBlock->rootDirectoryPointer;

	int minFreeEntry;

    KASSERT(*path == '/');

    /* Special case: root directory. */
    if (strcmp(path, "/") == 0)
		return &instance->rootDirEntry;

	while(strcmp(suffix, "/") != 0){ // weak
		Unpack_Path(path, prefix, &suffix);
		Print("%s, %s\n", prefix, suffix);
		minFreeEntry = -1;

		if(Get_FS_Buffer(fscache, base, &pBuf) != 0)
		{		
			Print("Get_FS_Buffer Error.\n");
			return EUNSPECIFIED;
		}
		
		entry = dir = (struct GOSFS_Dir_Entry*)pBuf->data;
		
	    for (i = 0; i < GOSFS_DIR_ENTRIES_PER_BLOCK; ++i) {
	    	entry = &dir[i];
			if (strcmp(entry->filename, prefix) == 0 &&
				((entry->flags == GOSFS_DIRENTRY_USED && strcmp(suffix, "/") == 0) ||
				 entry->flags == GOSFS_DIRENTRY_ISDIRECTORY)){
				Print("entry offset : %d\n", i);
				break;
			}
			// need to fix
			else if(minFreeEntry == -1 &&
					!(entry->flags & GOSFS_DIRENTRY_USED) && 
					!(entry->flags & GOSFS_DIRENTRY_ISDIRECTORY)){
						minFreeEntry = i;
						Print("minFreeEntry : %d\n", minFreeEntry);
					}
			else{
				;
			}
	    }

	    if(i == GOSFS_DIR_ENTRIES_PER_BLOCK){ // There is no entry matched
	    	Print("There is no entry matched\n");
	    	pathInfo->base = base; // need to modify
			pathInfo->offset = minFreeEntry;
			strcpy(pathInfo->suffix, prefix);
	    	retval = (strcmp(suffix,"/") == 0)? EUNSPECIFIED : ENOTFOUND;
			Release_FS_Buffer(fscache, pBuf);
			break;
	    }
	   	else{
	   		path = suffix;
   			pathInfo->base = base; // need to modify
			pathInfo->offset = i; // weak
			strcpy(pathInfo->suffix, prefix);
			Release_FS_Buffer(fscache, pBuf);
			base = entry->blockList[0];
	   	}
    }

    return retval;
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
		 * Allocate File object, GOSFS_File object.
		 */
		if ((gosfsFile = (struct GOSFS_File *) Malloc(sizeof(struct GOSFS_File))) == 0 ) {
			goto memfail;
		}

		/* Populate GOSFS_File */
		gosfsFile->entry = entry;
		gosfsFile->numBlocks = numBlocks;
		Mutex_Init(&gosfsFile->lock);

		/* Add to instance's list of GOSFS_File objects. */
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
    int retval = 0;
	struct GOSFS_Dir_Entry* entry = 0;
	GOSFS_Instance *instance = (GOSFS_Instance*) mountPoint->fsData;
	struct GOSFS_File *gosfsFile = 0;
	struct File *file = 0;
	struct FS_Buffer_Cache* fscache = instance->fscache;
	Path_Info pathInfo;
	struct FS_Buffer *pBuf;
	strcpy(&pathInfo, path);
	pathInfo.base = GOSFS_SUPER_BLOCK;

    /* Look up the directory entry */
    if ((retval = GOSFS_Lookup(instance, &pathInfo)) < 0){ 
    	if(retval == ENOTFOUND){ /* Wrong path, weak */
			return ENOTFOUND;
		}
		else
		{	
			if(Get_FS_Buffer(fscache, pathInfo.base, &pBuf) != 0)
		    {    	
				Print("Get_FS_Buffer Error.\n");
		    	return -1;
		    }
		    entry = &((struct GOSFS_Dir_Entry*)pBuf->data)[pathInfo.offset];

		    /* There is no file, so create file */
		    if (mode & O_CREATE){
		   		Print("EACCESS\n");
		   		Print("filename : %s\n", pathInfo.suffix);
		   		strcpy(entry->filename, pathInfo.suffix);
				entry->size = 0; /* not reasonable.. */
				entry->flags = GOSFS_DIRENTRY_USED;

				Super_Block* superBlock = (Super_Block*)instance->fsinfo->data;
				int freeBit = Find_First_Free_Bit(superBlock->bitmap, GOSFS_FS_BLOCK_SIZE - 12);
				Print("freeBit : %d\n", freeBit);
				entry->blockList[0] = freeBit;
				Set_Bit(superBlock->bitmap, freeBit);

				Modify_FS_Buffer(fscache, pBuf); // need to wrapper
				Modify_FS_Buffer(fscache, instance->fsinfo); // Superblock
				Sync_FS_Buffer_Cache(fscache);
				// need alloc data block
			}
		}
	}
	else
	{
		Print("FOUND\n");
		if(Get_FS_Buffer(fscache, pathInfo.base, &pBuf) != 0)
	    {    	
			Print("Get_FS_Buffer Error.\n");
	    	return -1;
	    }
	    entry = &((struct GOSFS_Dir_Entry*)pBuf->data)[pathInfo.offset];
	    int flags = entry->flags;

	    Print("blockList[0] : %d\n", entry->blockList[0]);

	    if(flags == GOSFS_DIRENTRY_ISDIRECTORY){
	    	rc = EACCESS;
	    	goto done;
	    }	
	}

	/* Get GOSFS_File object */
    gosfsFile = Get_GOSFS_File(instance, entry);
    if (gosfsFile == 0)
    	goto done;

	/* Create the file object. */
	file = Allocate_File(&s_gosfsFileOps, 0, entry->size, gosfsFile, mode, mountPoint);
	if (file == 0) {
		rc = ENOMEM;
		goto done;
	}

	/* Success! */
	*pFile = file;
	
	done:
	    Release_FS_Buffer(fscache, pBuf);
		return rc;

    //TODO("GeekOS filesystem open operation");
}

/*
 * Create a directory named by given path.
 */
static int GOSFS_Create_Directory(struct Mount_Point *mountPoint, const char *path)
{
	int rc = 0;
	int retval = 0;
	struct GOSFS_Dir_Entry* entry = 0;
	GOSFS_Instance *instance = (GOSFS_Instance*) mountPoint->fsData;
	struct GOSFS_File *gosfsFile = 0;
	struct File *file = 0;
	struct FS_Buffer_Cache* fscache = instance->fscache;
	Path_Info pathInfo;
	struct FS_Buffer *pBuf;
	strcpy(&pathInfo, path);
	pathInfo.base = GOSFS_SUPER_BLOCK;

    /* Look up the directory entry */
    if ((retval = GOSFS_Lookup(instance, &pathInfo)) < 0){ 
    	if(retval == ENOTFOUND){ /* Wrong path, weak */
			Print("ENOTFOUND\n");
			rc = ENOTFOUND;
			goto done;
		}
		else
		{	
			if(Get_FS_Buffer(fscache, pathInfo.base, &pBuf) != 0)
			{		
				Print("Get_FS_Buffer Error.\n");
				rc = EUNSPECIFIED;
				goto done;
			}
			entry = &((struct GOSFS_Dir_Entry*)pBuf->data)[pathInfo.offset];

			/* There is no directory, so create directory */
			Print("EACCESS\n");
			Print("filename : %s\n", pathInfo.suffix);
			strcpy(entry->filename, pathInfo.suffix);
			entry->size = 0; /* not reasonable.. */
			entry->flags = GOSFS_DIRENTRY_ISDIRECTORY;

			Super_Block* superBlock = (Super_Block*)instance->fsinfo->data;
			int freeBit = Find_First_Free_Bit(superBlock->bitmap, GOSFS_FS_BLOCK_SIZE - 12);
			Print("freeBit : %d\n", freeBit);
			entry->blockList[0] = freeBit;
			Set_Bit(superBlock->bitmap, freeBit);

			Modify_FS_Buffer(fscache, pBuf); // need to wrapper
			Modify_FS_Buffer(fscache, instance->fsinfo); // Superblock
			Sync_FS_Buffer_Cache(fscache);
			Release_FS_Buffer(fscache, pBuf);
			rc = 0;
			goto done;

		}
	}
	else
	{
		rc = EEXIST;
		goto done;
	}
	
	done:
		return rc;

    //TODO("GeekOS filesystem create directory operation");
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
	Set_Bit((void*)super_block->bitmap, GOSFS_SUPER_BLOCK); // superblock
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
	Super_Block *superBlock;
	struct FS_Buffer *pBuf;
	int rc;
	int i;
	
	instance = (GOSFS_Instance*) Malloc(sizeof(*instance));
    if (instance == 0)
		goto memfail;
	memset(instance, '\0', sizeof(*instance));
	
	/*
	 * Create a cache of filesystem buffers.
	 */
    instance->fscache = Create_FS_Buffer_Cache(mountPoint->dev, GOSFS_FS_BLOCK_SIZE);
    /* Superblock buffer will not be released for guaranty always in memory  */
	if((rc = Get_FS_Buffer(instance->fscache, GOSFS_SUPER_BLOCK, &pBuf)) != 0) 
	{	   
		goto fail;
	}

	instance->fsinfo = pBuf;
	superBlock = (Super_Block *)instance->fsinfo->data; 
	Print("rootDir : %d\n", superBlock->rootDirectoryPointer);

    /* Does magic number match? */
    if (superBlock->magic != GOSFS_MAGIC) {
		Print("Bad magic number (%x) for GOSFS filesystem\n", superBlock->magic);
		goto invalidfs;
    }

    /* Create the fake root directory entry. */
    memset(&instance->rootDirEntry, '\0', sizeof(struct GOSFS_Dir_Entry));
    instance->rootDirEntry.flags = GOSFS_DIRENTRY_ISDIRECTORY;
    instance->rootDirEntry.blockList[0] = superBlock->rootDirectoryPointer;
    instance->rootDirEntry.size = 1*GOSFS_FS_BLOCK_SIZE;

 	/* Initialize instance lock and GOSFS_File list. */
    Mutex_Init(&instance->lock);
    Clear_GOSFS_File_List(&instance->fileList);

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

