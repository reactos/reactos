/******************************************************************************
 *
 * BigBlockFile
 *
 * This is the implementation of a file that consists of blocks of 
 * a predetermined size.
 * This class is used in the Compound File implementation of the 
 * IStorage and IStream interfaces. It provides the functionality 
 * to read and write any blocks in the file as well as setting and 
 * obtaining the size of the file.
 * The blocks are indexed sequentially from the start of the file
 * starting with -1.
 * 
 * TODO:
 * - Support for a transacted mode
 *
 * Copyright 1999 Thuy Nguyen
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <windows.h>
#include <ole32/ole32.h>
#include "storage32.h"

#include <debug.h>

/***********************************************************
 * Data structures used internally by the BigBlockFile
 * class.
 */

/* We map in PAGE_SIZE-sized chunks. Must be a multiple of 4096. */
#define PAGE_SIZE       131072

#define BLOCKS_PER_PAGE (PAGE_SIZE / BIG_BLOCK_SIZE)

/* We keep a list of recently-discarded pages. This controls the
 * size of that list. */
#define MAX_VICTIM_PAGES 16

/* This structure provides one bit for each block in a page.
 * Use BIGBLOCKFILE_{Test,Set,Clear}Bit to manipulate it. */
typedef struct
{
    unsigned int bits[BLOCKS_PER_PAGE / (CHAR_BIT * sizeof(unsigned int))];
} BlockBits;

/***
 * This structure identifies the paged that are mapped
 * from the file and their position in memory. It is 
 * also used to hold a reference count to those pages.
 *
 * page_index identifies which PAGE_SIZE chunk from the
 * file this mapping represents. (The mappings are always
 * PAGE_SIZE-aligned.)
 */
struct MappedPage
{
    MappedPage *next;
    MappedPage *prev;

    DWORD  page_index;
    LPVOID lpBytes;
    LONG   refcnt;

    BlockBits readable_blocks;
    BlockBits writable_blocks;
};

/***********************************************************
 * Prototypes for private methods
 */
static void*     BIGBLOCKFILE_GetMappedView(LPBIGBLOCKFILE This,
                                            DWORD          page_index);
static void      BIGBLOCKFILE_ReleaseMappedPage(LPBIGBLOCKFILE This,
                                                MappedPage *page);
static void      BIGBLOCKFILE_FreeAllMappedPages(LPBIGBLOCKFILE This);
static void      BIGBLOCKFILE_UnmapAllMappedPages(LPBIGBLOCKFILE This);
static void      BIGBLOCKFILE_RemapAllMappedPages(LPBIGBLOCKFILE This);
static void*     BIGBLOCKFILE_GetBigBlockPointer(LPBIGBLOCKFILE This,
                                                 ULONG          index,
                                                 DWORD          desired_access);
static MappedPage* BIGBLOCKFILE_GetPageFromPointer(LPBIGBLOCKFILE This, 
						   void*         pBlock);
static MappedPage* BIGBLOCKFILE_CreatePage(LPBIGBLOCKFILE This,
					   ULONG page_index);
static DWORD     BIGBLOCKFILE_GetProtectMode(DWORD openFlags);
static BOOL      BIGBLOCKFILE_FileInit(LPBIGBLOCKFILE This, HANDLE hFile);
static BOOL      BIGBLOCKFILE_MemInit(LPBIGBLOCKFILE This, ILockBytes* plkbyt);

/* Note that this evaluates a and b multiple times, so don't
 * pass expressions with side effects. */
#define ROUNDUP(a, b) ((((a) + (b) - 1)/(b))*(b))

/***********************************************************
 * Blockbits functions.
 */
static inline BOOL BIGBLOCKFILE_TestBit(const BlockBits *bb,
					unsigned int index)
{
    unsigned int array_index = index / (CHAR_BIT * sizeof(unsigned int));
    unsigned int bit_index = index % (CHAR_BIT * sizeof(unsigned int));

    return bb->bits[array_index] & (1 << bit_index);
}

static inline void BIGBLOCKFILE_SetBit(BlockBits *bb, unsigned int index)
{
    unsigned int array_index = index / (CHAR_BIT * sizeof(unsigned int));
    unsigned int bit_index = index % (CHAR_BIT * sizeof(unsigned int));

    bb->bits[array_index] |= (1 << bit_index);
}

static inline void BIGBLOCKFILE_ClearBit(BlockBits *bb, unsigned int index)
{
    unsigned int array_index = index / (CHAR_BIT * sizeof(unsigned int));
    unsigned int bit_index = index % (CHAR_BIT * sizeof(unsigned int));

    bb->bits[array_index] &= ~(1 << bit_index);
}

static inline void BIGBLOCKFILE_Zero(BlockBits *bb)
{
    memset(bb->bits, 0, sizeof(bb->bits));
}

/******************************************************************************
 *      BIGBLOCKFILE_Construct
 *
 * Construct a big block file. Create the file mapping object. 
 * Create the read only mapped pages list, the writable mapped page list
 * and the blocks in use list.
 */
BigBlockFile * BIGBLOCKFILE_Construct(
  HANDLE   hFile,
  ILockBytes* pLkByt,
  DWORD    openFlags,
  ULONG    blocksize,
  BOOL     fileBased)
{
  LPBIGBLOCKFILE This;

  This = (LPBIGBLOCKFILE)HeapAlloc(GetProcessHeap(), 0, sizeof(BigBlockFile));

  if (This == NULL)
    return NULL;

  This->fileBased = fileBased;

  This->flProtect = BIGBLOCKFILE_GetProtectMode(openFlags);

  This->blocksize = blocksize;

  This->maplist = NULL;
  This->victimhead = NULL;
  This->victimtail = NULL;
  This->num_victim_pages = 0;

  if (This->fileBased)
  {
    if (!BIGBLOCKFILE_FileInit(This, hFile))
    {
      HeapFree(GetProcessHeap(), 0, This);
      return NULL;
    }
  }
  else
  {
    if (!BIGBLOCKFILE_MemInit(This, pLkByt))
    {
      HeapFree(GetProcessHeap(), 0, This);
      return NULL;
    }
  }

  return This;
}

/******************************************************************************
 *      BIGBLOCKFILE_FileInit
 *
 * Initialize a big block object supported by a file.
 */
static BOOL BIGBLOCKFILE_FileInit(LPBIGBLOCKFILE This, HANDLE hFile)
{
  This->pLkbyt     = NULL;
  This->hbytearray = 0;
  This->pbytearray = NULL;

  This->hfile = hFile;

  if (This->hfile == INVALID_HANDLE_VALUE)
    return FALSE;

  /* create the file mapping object
   */
  This->hfilemap = CreateFileMappingA(This->hfile,
                                      NULL,
                                      This->flProtect,
                                      0, 0,
                                      NULL);

  if (!This->hfilemap)
  {
    CloseHandle(This->hfile);
    return FALSE;
  }

  This->filesize.u.LowPart = GetFileSize(This->hfile,
					 &This->filesize.u.HighPart);

  This->maplist = NULL;

  Print(MAX_TRACE, ("file len %lu\n", This->filesize.u.LowPart));

  return TRUE;
}

/******************************************************************************
 *      BIGBLOCKFILE_MemInit
 *
 * Initialize a big block object supported by an ILockBytes on HGLOABL.
 */
static BOOL BIGBLOCKFILE_MemInit(LPBIGBLOCKFILE This, ILockBytes* plkbyt)
{
  This->hfile       = 0;
  This->hfilemap    = 0;

  /*
   * Retrieve the handle to the byte array from the LockByte object.
   */
  if (GetHGlobalFromILockBytes(plkbyt, &(This->hbytearray)) != S_OK)
  {
    Print(MIN_TRACE, ("May not be an ILockBytes on HGLOBAL\n"));
    return FALSE;
  }

  This->pLkbyt = plkbyt;

  /*
   * Increment the reference count of the ILockByte object since
   * we're keeping a reference to it.
   */
  ILockBytes_AddRef(This->pLkbyt);

  This->filesize.u.LowPart = GlobalSize(This->hbytearray);
  This->filesize.u.HighPart = 0;

  This->pbytearray = GlobalLock(This->hbytearray);

  Print(MAX_TRACE, ("mem on %p len %lu\n", This->pbytearray, This->filesize.u.LowPart));

  return TRUE;
}

/******************************************************************************
 *      BIGBLOCKFILE_Destructor
 *
 * Destructor. Clean up, free memory.
 */
void BIGBLOCKFILE_Destructor(
  LPBIGBLOCKFILE This)
{
  BIGBLOCKFILE_FreeAllMappedPages(This);

  if (This->fileBased)
  {
    CloseHandle(This->hfilemap);
    CloseHandle(This->hfile);
  }
  else
  {
    GlobalUnlock(This->hbytearray);
    ILockBytes_Release(This->pLkbyt);
  }

  /* destroy this
   */
  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 *      BIGBLOCKFILE_GetROBigBlock
 *
 * Returns the specified block in read only mode.
 * Will return NULL if the block doesn't exists.
 */
void* BIGBLOCKFILE_GetROBigBlock(
  LPBIGBLOCKFILE This,
  ULONG          index)
{
  /*
   * block index starts at -1
   * translate to zero based index
   */
  if (index == 0xffffffff)
    index = 0;
  else
    index++;

  /*
   * validate the block index
   * 
   */
  if (This->blocksize * (index + 1)
      > ROUNDUP(This->filesize.u.LowPart, This->blocksize))
  {
    Print(MAX_TRACE, ("out of range %lu vs %lu\n", This->blocksize * (index + 1),
	    This->filesize.u.LowPart));
    return NULL;
  }

  return BIGBLOCKFILE_GetBigBlockPointer(This, index, FILE_MAP_READ);
}

/******************************************************************************
 *      BIGBLOCKFILE_GetBigBlock
 *
 * Returns the specified block.
 * Will grow the file if necessary.
 */
void* BIGBLOCKFILE_GetBigBlock(LPBIGBLOCKFILE This, ULONG index)
{
  /*
   * block index starts at -1
   * translate to zero based index
   */
  if (index == 0xffffffff)
    index = 0;
  else
    index++;

  /*
   * make sure that the block physically exists
   */
  if ((This->blocksize * (index + 1)) > This->filesize.u.LowPart)
  {
    ULARGE_INTEGER newSize;

    newSize.u.HighPart = 0;
    newSize.u.LowPart = This->blocksize * (index + 1);

    BIGBLOCKFILE_SetSize(This, newSize);
  }

  return BIGBLOCKFILE_GetBigBlockPointer(This, index, FILE_MAP_WRITE);
}

/******************************************************************************
 *      BIGBLOCKFILE_ReleaseBigBlock
 *
 * Releases the specified block.
 */
void BIGBLOCKFILE_ReleaseBigBlock(LPBIGBLOCKFILE This, void *pBlock)
{
    MappedPage *page;

    if (pBlock == NULL)
	return;

    page = BIGBLOCKFILE_GetPageFromPointer(This, pBlock);

    if (page == NULL)
	return;

    BIGBLOCKFILE_ReleaseMappedPage(This, page);
}

/******************************************************************************
 *      BIGBLOCKFILE_SetSize
 *
 * Sets the size of the file.
 * 
 */
void BIGBLOCKFILE_SetSize(LPBIGBLOCKFILE This, ULARGE_INTEGER newSize)
{
  if (This->filesize.u.LowPart == newSize.u.LowPart)
    return;

  Print(MAX_TRACE, ("from %lu to %lu\n", This->filesize.u.LowPart, newSize.u.LowPart));
  /*
   * unmap all views, must be done before call to SetEndFile
   */
  BIGBLOCKFILE_UnmapAllMappedPages(This);
  
  if (This->fileBased)
  {
    char buf[10];

    /*
     * close file-mapping object, must be done before call to SetEndFile
     */
    CloseHandle(This->hfilemap);
    This->hfilemap = 0;

    /*
     * BEGIN HACK
     * This fixes a bug when saving through smbfs.
     * smbmount a Windows shared directory, save a structured storage file
     * to that dir: crash.
     *
     * The problem is that the SetFilePointer-SetEndOfFile combo below
     * doesn't always succeed. The file is not grown. It seems like the 
     * operation is cached. By doing the WriteFile, the file is actually
     * grown on disk.
     * This hack is only needed when saving to smbfs.
     */
    memset(buf, '0', 10);
    SetFilePointer(This->hfile, newSize.u.LowPart, NULL, FILE_BEGIN);
    WriteFile(This->hfile, buf, 10, NULL, NULL);
    /*
     * END HACK 
     */

    /*
     * set the new end of file
     */
    SetFilePointer(This->hfile, newSize.u.LowPart, NULL, FILE_BEGIN);
    SetEndOfFile(This->hfile);
  
    /*
     * re-create the file mapping object
     */
    This->hfilemap = CreateFileMappingA(This->hfile,
                                        NULL,
                                        This->flProtect,
                                        0, 0, 
                                        NULL);
  }
  else
  {
    GlobalUnlock(This->hbytearray);

    /*
     * Resize the byte array object.
     */
    ILockBytes_SetSize(This->pLkbyt, newSize);

    /*
     * Re-acquire the handle, it may have changed.
     */
    GetHGlobalFromILockBytes(This->pLkbyt, &This->hbytearray);
    This->pbytearray = GlobalLock(This->hbytearray);
  }

  This->filesize.u.LowPart = newSize.u.LowPart;
  This->filesize.u.HighPart = newSize.u.HighPart;

  BIGBLOCKFILE_RemapAllMappedPages(This);
}

/******************************************************************************
 *      BIGBLOCKFILE_GetSize
 *
 * Returns the size of the file.
 * 
 */
ULARGE_INTEGER BIGBLOCKFILE_GetSize(LPBIGBLOCKFILE This)
{
  return This->filesize;
}

/******************************************************************************
 *      BIGBLOCKFILE_AccessCheck     [PRIVATE]
 *
 * block_index is the index within the page.
 */
static BOOL BIGBLOCKFILE_AccessCheck(MappedPage *page, ULONG block_index,
				     DWORD desired_access)
{
    assert(block_index < BLOCKS_PER_PAGE);

    if (desired_access == FILE_MAP_READ)
    {
	if (BIGBLOCKFILE_TestBit(&page->writable_blocks, block_index))
	    return FALSE;

	BIGBLOCKFILE_SetBit(&page->readable_blocks, block_index);
    }
    else
    {
	assert(desired_access == FILE_MAP_WRITE);

	if (BIGBLOCKFILE_TestBit(&page->readable_blocks, block_index))
	    return FALSE;

	BIGBLOCKFILE_SetBit(&page->writable_blocks, block_index);
    }

    return TRUE;
}

/******************************************************************************
 *      BIGBLOCKFILE_GetBigBlockPointer     [PRIVATE]
 *
 * Returns a pointer to the specified block.
 */
static void* BIGBLOCKFILE_GetBigBlockPointer(
  LPBIGBLOCKFILE This, 
  ULONG          block_index, 
  DWORD          desired_access)
{
    DWORD page_index = block_index / BLOCKS_PER_PAGE;
    DWORD block_on_page = block_index % BLOCKS_PER_PAGE;

    MappedPage *page = BIGBLOCKFILE_GetMappedView(This, page_index);
    if (!page || !page->lpBytes) return NULL;

    if (!BIGBLOCKFILE_AccessCheck(page, block_on_page, desired_access))
    {
	BIGBLOCKFILE_ReleaseMappedPage(This, page);
	return NULL;
    }

    return (LPBYTE)page->lpBytes + (block_on_page * This->blocksize);
}

/******************************************************************************
 *      BIGBLOCKFILE_GetMappedPageFromPointer     [PRIVATE]
 *
 * pBlock is a pointer to a block on a page.
 * The page has to be on the in-use list. (As oppsed to the victim list.)
 *
 * Does not increment the usage count.
 */
static MappedPage *BIGBLOCKFILE_GetPageFromPointer(LPBIGBLOCKFILE This,
						   void *pBlock)
{
    MappedPage *page;

    for (page = This->maplist; page != NULL; page = page->next)
    {
	if ((LPBYTE)pBlock >= (LPBYTE)page->lpBytes
	    && (LPBYTE)pBlock <= (LPBYTE)page->lpBytes + PAGE_SIZE)
	    break;

    }

    return page;
}

/******************************************************************************
 *      BIGBLOCKFILE_FindPageInList      [PRIVATE]
 *
 */
static MappedPage *BIGBLOCKFILE_FindPageInList(MappedPage *head,
					       ULONG page_index)
{
    for (; head != NULL; head = head->next)
    {
	if (head->page_index == page_index)
	{
	    InterlockedIncrement(&head->refcnt);
	    break;
	}
    }

    return head;

}

static void BIGBLOCKFILE_UnlinkPage(MappedPage *page)
{
    if (page->next) page->next->prev = page->prev;
    if (page->prev) page->prev->next = page->next;
}

static void BIGBLOCKFILE_LinkHeadPage(MappedPage **head, MappedPage *page)
{
    if (*head) (*head)->prev = page;
    page->next = *head;
    page->prev = NULL;
    *head = page;
}

/******************************************************************************
 *      BIGBLOCKFILE_GetMappedView      [PRIVATE]
 *
 * Gets the page requested if it is already mapped.
 * If it's not already mapped, this method will map it
 */
static void * BIGBLOCKFILE_GetMappedView(
  LPBIGBLOCKFILE This,
  DWORD          page_index)
{
    MappedPage *page;

    page = BIGBLOCKFILE_FindPageInList(This->maplist, page_index);
    if (!page)
    {
	page = BIGBLOCKFILE_FindPageInList(This->victimhead, page_index);
	if (page)
	{
	    This->num_victim_pages--;

	    BIGBLOCKFILE_Zero(&page->readable_blocks);
	    BIGBLOCKFILE_Zero(&page->writable_blocks);
	}
    }

    if (page)
    {
	/* If the page is not already at the head of the list, move
	 * it there. (Also moves pages from victim to main list.) */
	if (This->maplist != page)
	{
	    if (This->victimhead == page) This->victimhead = page->next;
	    if (This->victimtail == page) This->victimtail = page->prev;

	    BIGBLOCKFILE_UnlinkPage(page);

	    BIGBLOCKFILE_LinkHeadPage(&This->maplist, page);
	}

	return page;
    }

    page = BIGBLOCKFILE_CreatePage(This, page_index);
    if (!page) return NULL;

    BIGBLOCKFILE_LinkHeadPage(&This->maplist, page);

    return page;
}

static BOOL BIGBLOCKFILE_MapPage(LPBIGBLOCKFILE This, MappedPage *page)
{
    DWORD lowoffset = PAGE_SIZE * page->page_index;

    if (This->fileBased)
    {
	DWORD numBytesToMap;
	DWORD desired_access;

	if (lowoffset + PAGE_SIZE > This->filesize.u.LowPart)
	    numBytesToMap = This->filesize.u.LowPart - lowoffset;
	else
	    numBytesToMap = PAGE_SIZE;

	if (This->flProtect == PAGE_READONLY)
	    desired_access = FILE_MAP_READ;
	else
	    desired_access = FILE_MAP_WRITE;

	page->lpBytes = MapViewOfFile(This->hfilemap, desired_access, 0,
				      lowoffset, numBytesToMap);
    }
    else
    {
	page->lpBytes = (LPBYTE)This->pbytearray + lowoffset;
    }

    Print(MAX_TRACE, ("mapped page %lu to %p\n", page->page_index, page->lpBytes));

    return page->lpBytes != NULL;
}

static MappedPage *BIGBLOCKFILE_CreatePage(LPBIGBLOCKFILE This,
					   ULONG page_index)
{
    MappedPage *page;

    page = HeapAlloc(GetProcessHeap(), 0, sizeof(MappedPage));
    if (page == NULL)
      return NULL;

    page->page_index = page_index;
    page->refcnt = 1;

    page->next = NULL;
    page->prev = NULL;

    BIGBLOCKFILE_MapPage(This, page);

    BIGBLOCKFILE_Zero(&page->readable_blocks);
    BIGBLOCKFILE_Zero(&page->writable_blocks);

    return page;
}

static void BIGBLOCKFILE_UnmapPage(LPBIGBLOCKFILE This, MappedPage *page)
{
    Print(MAX_TRACE, ("%ld at %p\n", page->page_index, page->lpBytes));
    if (page->refcnt > 0)
	Print(MIN_TRACE, ("unmapping inuse page %p\n", page->lpBytes));

    if (This->fileBased && page->lpBytes)
	UnmapViewOfFile(page->lpBytes);

    page->lpBytes = NULL;
}

static void BIGBLOCKFILE_DeletePage(LPBIGBLOCKFILE This, MappedPage *page)
{
    BIGBLOCKFILE_UnmapPage(This, page);

    HeapFree(GetProcessHeap(), 0, page);
}

/******************************************************************************
 *      BIGBLOCKFILE_ReleaseMappedPage      [PRIVATE]
 *
 * Decrements the reference count of the mapped page.
 */
static void BIGBLOCKFILE_ReleaseMappedPage(
  LPBIGBLOCKFILE This,
  MappedPage    *page)
{
    assert(This != NULL);
    assert(page != NULL);

    /* If the page is no longer refenced, move it to the victim list.
     * If the victim list is too long, kick somebody off. */
    if (!InterlockedDecrement(&page->refcnt))
    {
	if (This->maplist == page) This->maplist = page->next;

	BIGBLOCKFILE_UnlinkPage(page);

	if (MAX_VICTIM_PAGES > 0)
	{
	    if (This->num_victim_pages >= MAX_VICTIM_PAGES)
	    {
		MappedPage *victim = This->victimtail;
		if (victim)
		{
		    This->victimtail = victim->prev;
		    if (This->victimhead == victim)
			This->victimhead = victim->next;

		    BIGBLOCKFILE_UnlinkPage(victim);
		    BIGBLOCKFILE_DeletePage(This, victim);
		}
	    }
	    else This->num_victim_pages++;

	    BIGBLOCKFILE_LinkHeadPage(&This->victimhead, page);
	    if (This->victimtail == NULL) This->victimtail = page;
	}
	else
	    BIGBLOCKFILE_DeletePage(This, page);
    }
}

static void BIGBLOCKFILE_DeleteList(LPBIGBLOCKFILE This, MappedPage *list)
{
    while (list != NULL)
    {
	MappedPage *next = list->next;

	BIGBLOCKFILE_DeletePage(This, list);

	list = next;
    }
}

/******************************************************************************
 *      BIGBLOCKFILE_FreeAllMappedPages     [PRIVATE]
 *
 * Unmap all currently mapped pages.
 * Empty mapped pages list.
 */
static void BIGBLOCKFILE_FreeAllMappedPages(
  LPBIGBLOCKFILE This)
{
    BIGBLOCKFILE_DeleteList(This, This->maplist);
    BIGBLOCKFILE_DeleteList(This, This->victimhead);

    This->maplist = NULL;
    This->victimhead = NULL;
    This->victimtail = NULL;
    This->num_victim_pages = 0;
}

static void BIGBLOCKFILE_UnmapList(LPBIGBLOCKFILE This, MappedPage *list)
{
    for (; list != NULL; list = list->next)
    {
	BIGBLOCKFILE_UnmapPage(This, list);
    }
}

static void BIGBLOCKFILE_UnmapAllMappedPages(LPBIGBLOCKFILE This)
{
    BIGBLOCKFILE_UnmapList(This, This->maplist);
    BIGBLOCKFILE_UnmapList(This, This->victimhead);
}

static void BIGBLOCKFILE_RemapList(LPBIGBLOCKFILE This, MappedPage *list)
{
    while (list != NULL)
    {
	MappedPage *next = list->next;

	if (list->page_index * PAGE_SIZE > This->filesize.u.LowPart)
	{
	    Print(MAX_TRACE, ("discarding %lu\n", list->page_index));

	    /* page is entirely outside of the file, delete it */
	    BIGBLOCKFILE_UnlinkPage(list);
	    BIGBLOCKFILE_DeletePage(This, list);
	}
	else
	{
	    /* otherwise, remap it */
	    BIGBLOCKFILE_MapPage(This, list);
	}

	list = next;
    }
}

static void BIGBLOCKFILE_RemapAllMappedPages(LPBIGBLOCKFILE This)
{
    BIGBLOCKFILE_RemapList(This, This->maplist);
    BIGBLOCKFILE_RemapList(This, This->victimhead);
}

/****************************************************************************
 *      BIGBLOCKFILE_GetProtectMode
 *
 * This function will return a protection mode flag for a file-mapping object
 * from the open flags of a file.
 */
static DWORD BIGBLOCKFILE_GetProtectMode(DWORD openFlags)
{
    if (openFlags & (STGM_WRITE | STGM_READWRITE))
	return PAGE_READWRITE;
    else
	return PAGE_READONLY;
}
