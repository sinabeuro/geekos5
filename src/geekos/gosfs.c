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
    Dir_Entry_Ptr dirEntryPtr;		/* Entry informations of file */
    struct FS_Buffer *pBuf;			/* Buffer of file */
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
static struct GOSFS_Dir_Entry* Get_Entry_By_Ptr(GOSFS_Instance *instance, Dir_Entry_Ptr* dirEntryPtr)
{
	struct FS_Buffer *pBuf;

	if(dirEntryPtr->base == 0){ /* Check whether root directory */
		return &instance->rootDirEntry;
	}

	if(Get_FS_Buffer(instance->fscache, dirEntryPtr->base, &pBuf) != 0)
	{		
		Print("Get_FS_Buffer Error.\n");
		return NULL;
	}

	Release_FS_Buffer(instance->fscache, pBuf);
	return &((struct GOSFS_Dir_Entry*)pBuf->data)[dirEntryPtr->offset];
}

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
     * will remain in the GOSFS_Instance object, to speed up
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
    //TODO("GeekOS filesystem Close directory operation");
}

/*
 * Read a directory entry from an open directory.
 */
static int GOSFS_Read_Entry(struct File *dir, struct VFS_Dir_Entry *entry)
{
	Dir_Entry_Ptr dentryPtr = ((struct GOSFS_File*)dir->fsData)->dirEntryPtr;
    GOSFS_Instance *instance = (GOSFS_Instance*) dir->mountPoint->fsData;
    struct FS_Buffer_Cache* fscache = instance->fscache;
    struct FS_Buffer *pBuf;
    struct GOSFS_Dir_Entry *dentry;
   	struct VFS_File_Stat* stats;
    int blockNum;
	int rc = 0;
	int i;

	dentry = Get_Entry_By_Ptr(instance, &dentryPtr);
	blockNum = dentry->blockList[0];
	//Release_FS_Buffer(fscache, pBuf);
	
	if(Get_FS_Buffer(fscache, blockNum, &pBuf) != 0)
	{    	
		Print("Get_FS_Buffer Error.\n");
		rc = EUNSPECIFIED;
		goto fail;
	}
	
	while(dir->filePos < dir->endPos){
		i = dir->filePos/sizeof(struct GOSFS_Dir_Entry);
		dentry = &((struct GOSFS_Dir_Entry *)pBuf->data)[i];
		dir->filePos += sizeof(struct GOSFS_Dir_Entry);
		if(	dentry->flags == GOSFS_DIRENTRY_USED ||
			dentry->flags == GOSFS_DIRENTRY_ISDIRECTORY ||
			dentry->flags == GOSFS_DIRENTRY_SETUID ){ /* weak */
			break;
		}
	}
	
	if (dir->filePos >= dir->endPos){
		rc = VFS_NO_MORE_DIR_ENTRIES;
		goto fail; /* Reached the end of the directory. */
	}
	
    /*
     * Note: we don't need to bounds check here, because
     * generic struct VFS_Dir_Entry objects have much more space for filenames
     * than GOSFS directoryEntry objects.
     */
    strncpy(entry->name, dentry->filename, sizeof(dentry->filename));
    entry->name[sizeof(dentry->filename)] = '\0';

	stats = &entry->stats;
	stats->isDirectory = (dentry->flags & GOSFS_DIRENTRY_ISDIRECTORY)? 1 : 0;
	stats->size = dentry->size;
	memcpy(stats->acls, dentry->acl, VFS_MAX_ACL_ENTRIES);
	stats->isSetuid = 0; /* weak */
	
	fail:
	Release_FS_Buffer(fscache, pBuf);
	return rc;
	
	//TODO("GeekOS filesystem Read_Entry operation");
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
static int Do_GOSFS_Lookup(GOSFS_Instance *instance, Path_Info* pathInfo)
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
    if (strcmp(path, "/") == 0){
		return 0;
	}
	
	while(strcmp(suffix, "/") != 0){ /* weak */
		Unpack_Path(path, prefix, &suffix);
		Debug("%s, %s\n", prefix, suffix);
		minFreeEntry = -1;

		KASSERT(base != 0);
		if(Get_FS_Buffer(fscache, base, &pBuf) != 0)
		{		
			Print("Get_FS_Buffer Error.\n");
			return EUNSPECIFIED;
		}
		
		entry = dir = (struct GOSFS_Dir_Entry*)(pBuf->data);
		#if 0
		Print("blkNum : %d\n", pBuf->fsBlockNum);
		Print("size0 : %d\n", (&entry[0])->size);
		Print("entry0 : %s\n", (&entry[0])->filename);
		Print("entry1 : %s\n", (&entry[1])->filename);
		#endif
	    for (i = 0; i < GOSFS_DIR_ENTRIES_PER_BLOCK; ++i) {
	    	entry = &dir[i];
			if (strcmp(entry->filename, prefix) == 0 &&
				((entry->flags == GOSFS_DIRENTRY_USED && strcmp(suffix, "/") == 0) ||
				 entry->flags == GOSFS_DIRENTRY_ISDIRECTORY)){
				Debug("entry offset : %d\n", i);
				break;
			}
			/* weak : need to fix */
			else if(minFreeEntry == -1 &&
					!(entry->flags & GOSFS_DIRENTRY_USED) && 
					!(entry->flags & GOSFS_DIRENTRY_ISDIRECTORY)){
						minFreeEntry = i;
						Debug("minFreeEntry : %d, %d\n", minFreeEntry, entry->flags);
					}
			else{
				;
			}
	    }

	    if(i == GOSFS_DIR_ENTRIES_PER_BLOCK){ // There is no entry matched
	    	Debug("There is no entry matched\n");
	    	pathInfo->dirEntryPtr.base = base; // need to modify
			pathInfo->dirEntryPtr.offset = minFreeEntry;
			strcpy(pathInfo->suffix, prefix);
	    	retval = (strcmp(suffix,"/") == 0)? EUNSPECIFIED : ENOTFOUND;
			Release_FS_Buffer(fscache, pBuf);
			break;
	    }
   	    else if(i == PREV_DIR){ /* weak */
   	    	Dir_Entry_Ptr temp;
   	    	int blk = entry->blockList[0];
   	    	path = suffix;
   	    	Release_FS_Buffer(fscache, pBuf);
			temp.base = blk;
			temp.offset = 0;
			entry = Get_Entry_By_Ptr(instance, &temp); 
			temp.base = entry[PREV_DIR].blockList[0];
			temp.offset = 0;
			entry = Get_Entry_By_Ptr(instance, &temp); 

			int j;
			for (j = 0; j < GOSFS_DIR_ENTRIES_PER_BLOCK; ++j) {
				if(entry[j].blockList[0] == blk){
					break;
				}
			}
			strcpy(pathInfo->suffix, entry[j].filename);
			pathInfo->dirEntryPtr.base = temp.base; // need to modify
			pathInfo->dirEntryPtr.offset = j; 
			base = entry[j].blockList[0];
	    }
	   	else{
	   		path = suffix;
   			pathInfo->dirEntryPtr.base = base; // need to modify
			pathInfo->dirEntryPtr.offset = i; // weak
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
static struct GOSFS_File *Get_GOSFS_File(GOSFS_Instance* instance, Dir_Entry_Ptr* dirEntryPtr)
{
    ulong_t numBlocks;
	struct GOSFS_File *gosfsFile = 0;

    KASSERT(dirEntryPtr != 0);
    KASSERT(instance != 0);

    Mutex_Lock(&instance->lock);

    /*
     * See if this file has already been opened.
     * If so, use the existing GOSFS_File object.
     */
    for (gosfsFile= Get_Front_Of_GOSFS_File_List(&instance->fileList);
			gosfsFile != 0;
	 		gosfsFile = Get_Next_In_GOSFS_File_List(gosfsFile)) {
		if (gosfsFile->dirEntryPtr.base == dirEntryPtr->base &&
			gosfsFile->dirEntryPtr.offset == dirEntryPtr->offset )
		    break;
    }
			
	if (gosfsFile == 0) {
		/* Determine size of data block cache for file. */
		//numBlocks = Round_Up_To_Block(entry->size) / SECTOR_SIZE;

		/*
		 * Allocate File object, GOSFS_File object.
		 */
		if ((gosfsFile = (struct GOSFS_File *) Malloc(sizeof(struct GOSFS_File))) == 0 ) {
			goto memfail;
		}

		/* Populate GOSFS_File */
		memcpy(&(gosfsFile->dirEntryPtr), dirEntryPtr, sizeof(Dir_Entry_Ptr));
		//gosfsFile->numBlocks = numBlocks;
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
	struct FS_Buffer *pBuf;
	Path_Info pathInfo;
	strcpy(&pathInfo, path);
	pathInfo.dirEntryPtr.base = GOSFS_SUPER_BLOCK;

    /* Look up the directory entry */
    if ((retval = Do_GOSFS_Lookup(instance, &pathInfo)) < 0){ 
    	if(retval == ENOTFOUND){ /* Wrong path, weak */
			rc = ENOTFOUND;
			goto fail;
		}
		else
		{	
			if(Get_FS_Buffer(fscache, pathInfo.dirEntryPtr.base, &pBuf) != 0)
		    {    	
				Print("Get_FS_Buffer Error.\n");
		    	rc = EUNSPECIFIED;
		    	goto fail;
		    }
		    entry = &((struct GOSFS_Dir_Entry*)pBuf->data)[pathInfo.dirEntryPtr.offset];

		    /* There is no file, so create file */
		    if (mode & O_CREATE){
		   		Debug("EACCESS\n");
		   		Debug("filename : %s\n", pathInfo.suffix);
		   		strcpy(entry->filename, pathInfo.suffix);
				entry->size = 0; /* not reasonable.. */
				entry->flags = GOSFS_DIRENTRY_USED;

				Super_Block* superBlock = (Super_Block*)instance->fsinfo->data;
				int freeBit = Find_First_Free_Bit(superBlock->bitmap, GOSFS_FS_BLOCK_SIZE - 12);
				Debug("freeBit : %d\n", freeBit);
				entry->blockList[0] = freeBit;
				Set_Bit(superBlock->bitmap, freeBit);

				Modify_FS_Buffer(fscache, pBuf); // need to wrapper
				Modify_FS_Buffer(fscache, instance->fsinfo); // Superblock
				Sync_FS_Buffer_Cache(fscache);
				// need alloc data block
			}
		}
	}
	else{
		Debug("FOUND\n");
		if(Get_FS_Buffer(fscache, pathInfo.dirEntryPtr.base, &pBuf) != 0)
	    {    	
			Print("Get_FS_Buffer Error.\n");
			rc = EUNSPECIFIED;
			goto fail;

	    }
	    entry = &((struct GOSFS_Dir_Entry*)pBuf->data)[pathInfo.dirEntryPtr.offset];
	    int flags = entry->flags;

	    Debug("blockList[0] : %d\n", entry->blockList[0]);

	    if(flags == GOSFS_DIRENTRY_ISDIRECTORY){
	    	rc = EACCESS;
	    	goto fail;
	    }	
	}

	/* Get GOSFS_File object */
    gosfsFile = Get_GOSFS_File(instance, &(pathInfo.dirEntryPtr));
    if (gosfsFile == 0){
    	rc = EUNSPECIFIED;
    	goto fail;
    }

	/* Create the file object. */
	file = Allocate_File(&s_gosfsFileOps, 0, entry->size, gosfsFile, O_READ, mountPoint);
	if (file == 0) {
		rc = ENOMEM;
		goto fail;
	}

	/* Success! */
	*pFile = file;

	fail:
	
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
	pathInfo.dirEntryPtr.base = GOSFS_SUPER_BLOCK;

    /* Look up the directory entry */
    if ((retval = Do_GOSFS_Lookup(instance, &pathInfo)) < 0){ 
    	if(retval == ENOTFOUND){ /* Wrong path, weak */
			Debug("ENOTFOUND\n");
			rc = ENOTFOUND;
			goto done;
		}
		else
		{	
			if(Get_FS_Buffer(fscache, pathInfo.dirEntryPtr.base, &pBuf) != 0)
			{		
				Debug("Get_FS_Buffer Error.\n");
				rc = EUNSPECIFIED;
				goto done;
			}
			entry = &((struct GOSFS_Dir_Entry*)pBuf->data)[pathInfo.dirEntryPtr.offset];

			/* There is no directory, so create directory */
			Debug("EACCESS\n");
			Debug("filename : %s\n", pathInfo.suffix);
			strcpy(entry->filename, pathInfo.suffix);
			entry->size = GOSFS_FS_BLOCK_SIZE; /* not reasonable.. */
			entry->flags = GOSFS_DIRENTRY_ISDIRECTORY;

			Super_Block* superBlock = (Super_Block*)instance->fsinfo->data;
			int freeBit = Find_First_Free_Bit(superBlock->bitmap, GOSFS_FS_BLOCK_SIZE - 12);
			Debug("freeBit : %d\n", freeBit);
			entry->blockList[0] = freeBit;
			Set_Bit(superBlock->bitmap, freeBit);

			Modify_FS_Buffer(fscache, pBuf); // need to wrapper
			Modify_FS_Buffer(fscache, instance->fsinfo); // Superblock
			Sync_FS_Buffer_Cache(fscache);
			Release_FS_Buffer(fscache, pBuf);
			
			if(Get_FS_Buffer(fscache, freeBit, &pBuf) != 0)
			{		
				Debug("Get_FS_Buffer Error.\n");
				rc = EUNSPECIFIED;
				goto done;
			}			
			entry = (struct GOSFS_Dir_Entry*)pBuf->data;
			memset(pBuf->data, '\0', GOSFS_FS_BLOCK_SIZE); /* fill zero in the block */
			strcpy(entry[0].filename, ".");
			entry[0].flags = GOSFS_DIRENTRY_ISDIRECTORY;
			entry[0].blockList[0] = freeBit;
			entry[0].size = 1*GOSFS_FS_BLOCK_SIZE;
			strcpy(entry[1].filename, "..");
			entry[1].flags = GOSFS_DIRENTRY_ISDIRECTORY;
			entry[1].blockList[0] = pathInfo.dirEntryPtr.base;
			entry[1].size = 1*GOSFS_FS_BLOCK_SIZE;

			Modify_FS_Buffer(fscache, pBuf); // need to wrapper
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
    int rc = 0;
    int retval = 0;
	struct GOSFS_Dir_Entry* entry = 0;
	GOSFS_Instance *instance = (GOSFS_Instance*) mountPoint->fsData;
	struct GOSFS_File *gosfsFile = 0;
	struct File *file = 0;
	struct FS_Buffer_Cache* fscache = instance->fscache;
	struct FS_Buffer *pBuf;
	Path_Info pathInfo;
	strcpy(&pathInfo, path);
	pathInfo.dirEntryPtr.base = GOSFS_SUPER_BLOCK;

    /* Look up the directory entry */
    if ((retval = Do_GOSFS_Lookup(instance, &pathInfo)) < 0){ 
		Debug("ENOTFOUND\n");
		rc = ENOTFOUND;
		goto done;
	}
	else{
		Debug("FOUND\n");
		entry = Get_Entry_By_Ptr(instance, &pathInfo.dirEntryPtr);
	    int flags = entry->flags;

	    Debug("blockList[0] : %d\n", entry->blockList[0]);

	    if(flags != GOSFS_DIRENTRY_ISDIRECTORY){
	    	rc = ENOTDIR;
	    	goto fail;
	    }	
	}

	/* Get GOSFS_File object */
    gosfsFile = Get_GOSFS_File(instance, &(pathInfo.dirEntryPtr));
    if (gosfsFile == 0){
    	rc = EUNSPECIFIED;
    	goto fail;
    }

	/* Create the file object. */
	file = Allocate_File(&s_gosfsDirOps, 0, entry->size, gosfsFile, 0, mountPoint);
	if (file == 0) {
		rc = ENOMEM;
		goto fail;
	}

	/* Success! */
	*pDir = file;

	fail:
	
	done:
	    //Release_FS_Buffer(fscache, pBuf);
		return rc;

    TODO("GeekOS filesystem open directory operation");
}

/*
 * Open a directory named by given path.
 */
static int GOSFS_Delete(struct Mount_Point *mountPoint, const char *path)
{
	int i;
	int rc = 0;
	int retval = 0;
	struct GOSFS_Dir_Entry* entry = 0;
	GOSFS_Instance *instance = (GOSFS_Instance*) mountPoint->fsData;
	struct FS_Buffer_Cache* fscache = instance->fscache;
	Path_Info pathInfo;
	struct FS_Buffer *pBuf, *pBuf_1;
	strcpy(&pathInfo, path);
	pathInfo.dirEntryPtr.base = GOSFS_SUPER_BLOCK;

    /* Look up the directory entry */
    if ((retval = Do_GOSFS_Lookup(instance, &pathInfo)) < 0){ 
		Debug("ENOTFOUND\n");
		rc = ENOTFOUND;
		goto done;
	}
	else{
		Debug("FOUND\n");		

		if(Get_FS_Buffer(fscache, pathInfo.dirEntryPtr.base, &pBuf) != 0)
	    {    	
			Print("Get_FS_Buffer Error.\n");
	    	return -1;
	    }
	    
	    entry = &((struct GOSFS_Dir_Entry*)pBuf->data)[pathInfo.dirEntryPtr.offset];
		Debug("flags : %d\n", entry->flags);

	    if(entry->flags & GOSFS_DIRENTRY_USED)
		{
			;
		}
		else if(entry->flags & GOSFS_DIRENTRY_ISDIRECTORY)
		{
			Dir_Entry_Ptr dirEntryPtr;
			struct GOSFS_Dir_Entry* nentry = 0;;
			dirEntryPtr.base = entry->blockList[0];
			dirEntryPtr.offset = 0;
			nentry = Get_Entry_By_Ptr(instance, &dirEntryPtr);
			/* Check whether empty.. */
			for (i = 2; i < GOSFS_DIR_ENTRIES_PER_BLOCK; ++i) { /* weak */
				if ((&nentry[i])->flags != 0){
					Print("entry %d is not empty : %d \n", i, (&nentry[i])->flags);
					rc = EUNSPECIFIED;
					Release_FS_Buffer(fscache, pBuf);
					goto done;
				}
			}	
		}
		
		/* Need to consider crash */
		Super_Block* superBlock = (Super_Block*)instance->fsinfo->data;
		Clear_Bit(superBlock->bitmap, entry->blockList[0]);
		Modify_FS_Buffer(fscache, instance->fsinfo); // Superblock
		
		if(Get_FS_Buffer(fscache, entry->blockList[0], &pBuf_1) != 0)
	    {    	
			Print("Get_FS_Buffer Error.\n");
	    	return -1;
	    }
	    memset(pBuf_1->data, '\0', GOSFS_FS_BLOCK_SIZE); /* fill zero in the block */
	    memset(entry, '\0', sizeof(struct GOSFS_Dir_Entry)); /* fill zero in the entry */

		Modify_FS_Buffer(fscache, pBuf); // need to wrapper
		Modify_FS_Buffer(fscache, pBuf_1); // need to wrapper

		Sync_FS_Buffer_Cache(fscache);
		Release_FS_Buffer(fscache, pBuf);
		Release_FS_Buffer(fscache, pBuf_1);	

		rc = 0;


	}
	
	done:
		return rc;

    //TODO("GeekOS filesystem delete operation");
}

/*
 * Get metadata (size, permissions, etc.) of file named by given path.
 */
static int GOSFS_Stat(struct Mount_Point *mountPoint, const char *path, struct VFS_File_Stat *stat)
{
	int i;
	int rc = 0;
	int retval = 0;
	struct GOSFS_Dir_Entry* entry = 0;
	GOSFS_Instance *instance = (GOSFS_Instance*) mountPoint->fsData;
	struct FS_Buffer_Cache* fscache = instance->fscache;
	Path_Info pathInfo;
	struct FS_Buffer *pBuf;
	strcpy(&pathInfo, path);
	pathInfo.dirEntryPtr.base = GOSFS_SUPER_BLOCK;

    /* Look up the directory entry */
    if ((retval = Do_GOSFS_Lookup(instance, &pathInfo)) < 0){ 
		Debug("ENOTFOUND\n");
		rc = ENOTFOUND;
		goto done;
	}
	else{
		Debug("FOUND\n");
	    
	    entry = Get_Entry_By_Ptr(instance, &pathInfo.dirEntryPtr);
	    stat->isDirectory = (entry->flags & GOSFS_DIRENTRY_ISDIRECTORY)? 1 : 0;
	    stat->size = entry->size;
	    memcpy(stat->acls, entry->acl, VFS_MAX_ACL_ENTRIES);
	    stat->isSetuid = 0; /* weak */
	}
	
	done:
		return rc;

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

static GOSFS_Get_Path(struct Mount_Point *mountPoint, void *dentry, char *path)
{
	GOSFS_Instance *instance = (GOSFS_Instance*)mountPoint->fsData;
	struct FS_Buffer_Cache* fscache = instance->fscache;
	Dir_Entry_Ptr* dentryPtr = (Dir_Entry_Ptr*)dentry;
	Dir_Entry_Ptr* temp = (Dir_Entry_Ptr*)Malloc(sizeof(Dir_Entry_Ptr));
	struct GOSFS_Dir_Entry* gosfsDentry;
	struct GOSFS_Dir_Entry* prevBlkEntry; /* weak : Naming is not reasonable*/
	int i;
	int curBlkNum, prevBlkNum;
	int rc = 0;

	//Print("GOSFS_Get_Path %d, %d\n", dentryPtr->base, dentryPtr->offset);
	//Print("filename : %s\n", Get_Entry_By_Ptr(instance, dentryPtr)->filename);

	/* Root directory */
	if(Get_Entry_By_Ptr(instance, dentryPtr)->blockList[0] == ((Super_Block*)(instance->fsinfo->data))->rootDirectoryPointer)
	{
		goto done;
	}
	strcat(path, "/");
	path++;
		
	if(strcmp(Get_Entry_By_Ptr(instance, dentryPtr)->filename, "..") == 0){
		temp->base = dentryPtr->base;
		temp->offset = 1; /* This means '..' */
		prevBlkEntry = Get_Entry_By_Ptr(instance, temp); 
		prevBlkNum = prevBlkEntry->blockList[0];
	}
	else if(strcmp(Get_Entry_By_Ptr(instance, dentryPtr)->filename, ".") == 0){
		temp->base = dentryPtr->base;
		temp->offset = 0; /* This means '..' */
		prevBlkEntry = Get_Entry_By_Ptr(instance, temp); 
		prevBlkNum = prevBlkEntry->blockList[0];
	}
	else{

		strcpy(path, Get_Entry_By_Ptr(instance, dentryPtr)->filename);
		prevBlkNum = dentryPtr->base;
	}
	
	while(true){
		temp->base = curBlkNum = prevBlkNum;
		temp->offset = 1; /* This means '..' */
		//Print("get prev block num\n");
		prevBlkEntry = Get_Entry_By_Ptr(instance, temp); 
		prevBlkNum = prevBlkEntry->blockList[0];

		if(temp->base == prevBlkNum) /* Reach to root? */
			break;

		//Print("get prev block\n");
		temp->base = prevBlkNum;
		temp->offset = 0; /* Previous block's first entry */
		gosfsDentry = Get_Entry_By_Ptr(instance, temp);

		//Print("Find dentry at previous block\n");
		/* Find dentry at previous block*/
		for (i = 0; i < GOSFS_DIR_ENTRIES_PER_BLOCK; ++i) {
			if(gosfsDentry[i].blockList[0] == curBlkNum){
				break;
			}
		}

		/* There is no reference to next block */
		if(i == GOSFS_DIR_ENTRIES_PER_BLOCK){
			//Print("There is no reference to next block\n");
			rc =  EUNSPECIFIED;
			goto done;
		}

		/* Add to path */
		//Print("filename : %s\n", gosfsDentry[i].filename);
		//Print("strlen : %d\n", strlen(path));
		memmove(path + strlen(gosfsDentry[i].filename)+1, path, strlen(path)+1);
		memcpy(path, gosfsDentry[i].filename, strlen(gosfsDentry[i].filename));
		memcpy(path+strlen(gosfsDentry[i].filename), "/", 1);
		if(strcmp(path+strlen(gosfsDentry[i].filename), "/") == 0)
			strcpy(path+strlen(gosfsDentry[i].filename), "");
		//Print("%s\n", path);	
	}

	done:
	Free(temp);
	return rc;	
}

static GOSFS_Lookup(struct Mount_Point *mountPoint, char *path, void** dentry)
{
	GOSFS_Instance *instance = (GOSFS_Instance*) mountPoint->fsData;
	Dir_Entry_Ptr *dirEntryPtr = (Dir_Entry_Ptr *)Malloc(sizeof(Dir_Entry_Ptr));
	Path_Info pathInfo;
	strcpy(&pathInfo, path);
	pathInfo.dirEntryPtr.base = GOSFS_SUPER_BLOCK;
	int rc = 0;
	
    /* Look up the directory entry */
    if (Do_GOSFS_Lookup(instance, &pathInfo) < 0){ 
		Debug("ENOTFOUND\n");
		Free(dirEntryPtr);
		rc = ENOTFOUND;
		goto done;
	}

	memcpy(dirEntryPtr, &pathInfo.dirEntryPtr, sizeof(Dir_Entry_Ptr));

	*dentry = (void*)dirEntryPtr;
	Debug("GOSFS_Lookup %d, %d, %d\n", pathInfo.dirEntryPtr.base, pathInfo.dirEntryPtr.offset, g_currentThread->pid);

	done:
	return rc;


}

/*static*/ struct Mount_Point_Ops s_gosfsMountPointOps = {
    &GOSFS_Open,
    &GOSFS_Create_Directory,
    &GOSFS_Open_Directory,
    &GOSFS_Stat,
    &GOSFS_Sync,
    &GOSFS_Delete,
    &GOSFS_Get_Path,
    &GOSFS_Lookup,
};

static int GOSFS_Format(struct Block_Device *blockDev)
{
	Super_Block* super_block = (Super_Block*)Malloc(sizeof(Super_Block));
	struct GOSFS_Dir_Entry root_dir_entry[GOSFS_DIR_ENTRIES_PER_BLOCK];
	uint_t offset;
	int blockNum;
	//(struct GOSFS_Dir_Entry*) Malloc(sizeof(struct GOSFS_Dir_Entry*));

	/* Make Superblock */
	memset(super_block->bitmap, 0, sizeof(super_block->bitmap));
	Set_Bit((void*)super_block->bitmap, GOSFS_SUPER_BLOCK); // superblock
	Set_Bit((void*)super_block->bitmap, GOSFS_ROOT_DIR_BLOCK); // root dir
	super_block->size = Get_Num_Blocks(blockDev)/GOSFS_SECTORS_PER_FS_BLOCK;
	super_block->rootDirectoryPointer = GOSFS_ROOT_DIR_BLOCK;
	super_block->magic = GOSFS_MAGIC;	

	blockNum = GOSFS_SUPER_BLOCK * GOSFS_SECTORS_PER_FS_BLOCK;
	for (offset = 0; offset < GOSFS_FS_BLOCK_SIZE; offset += SECTOR_SIZE) {
		int rc = Block_Write(blockDev, blockNum, (char*)super_block + offset);
		if (rc != 0)
		    return rc;
		++blockNum;
    }
    
	/* Make Root diretory entry */
	strcpy(root_dir_entry[0].filename, ".");
	root_dir_entry[0].flags = GOSFS_DIRENTRY_ISDIRECTORY;
	root_dir_entry[0].blockList[0] = GOSFS_ROOT_DIR_BLOCK;
	root_dir_entry[0].size = 1*GOSFS_FS_BLOCK_SIZE;
	strcpy(root_dir_entry[1].filename, "..");
	root_dir_entry[1].flags = GOSFS_DIRENTRY_ISDIRECTORY;
	root_dir_entry[1].blockList[0] = GOSFS_ROOT_DIR_BLOCK;
	root_dir_entry[1].size = 1*GOSFS_FS_BLOCK_SIZE;

	blockNum = GOSFS_ROOT_DIR_BLOCK * GOSFS_SECTORS_PER_FS_BLOCK;
	for (offset = 0; offset < GOSFS_FS_BLOCK_SIZE; offset += SECTOR_SIZE) {
		int rc = Block_Write(blockDev, blockNum, (char*)root_dir_entry + offset);
		if (rc != 0)
		    return rc;
		++blockNum;
    }
	
    //TODO("GeekOS filesystem format operation");

    Free(super_block);
    //Free(root_dir_entry);
    return 0;
}

static int GOSFS_Mount(struct Mount_Point *mountPoint)
{
	struct GOSFS_Dir_Entry rentry[GOSFS_DIR_ENTRIES_PER_BLOCK];

	GOSFS_Instance *instance = 0;
	Super_Block *superBlock;
	struct FS_Buffer *pBuf;
	int rc;	
	
	instance = (GOSFS_Instance*)Malloc(sizeof(GOSFS_Instance));
    if (instance == 0)
		goto memfail;
	memset(instance, '\0', sizeof(GOSFS_Instance));
	
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
	//Print("rootDir : %d\n", superBlock->rootDirectoryPointer);

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

	Get_FS_Buffer(instance->fscache, 1, &pBuf);
	struct GOSFS_Dir_Entry* entry = 0;

	entry = (struct GOSFS_Dir_Entry*)(pBuf->data);
			Print("blkNum : %d\n", pBuf->fsBlockNum);
			Print("size0 : %d\n", (&entry[0])->size);
			Print("entry0 : %s\n", (&entry[0])->filename);
			Print("entry1 : %s\n", (&entry[1])->filename);

	Release_FS_Buffer(instance->fscache, pBuf);
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

