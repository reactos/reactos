/*
 * Compound Storage (32 bit version)
 * Storage implementation
 *
 * This file contains the compound file implementation
 * of the storage interface.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 1999 Thuy Nguyen
 * Copyright 2005 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 *  The compound file implementation of IStorage used for create
 *  and manage substorages and streams within a storage object
 *  residing in a compound file object.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "wine/debug.h"

#include "storage32.h"
#include "ole2.h"      /* For Write/ReadClassStm */

#include "winreg.h"
#include "wine/wingdi16.h"
#include "compobj_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);


/*
 * These are signatures to detect the type of Document file.
 */
static const BYTE STORAGE_magic[8]    ={0xd0,0xcf,0x11,0xe0,0xa1,0xb1,0x1a,0xe1};
static const BYTE STORAGE_oldmagic[8] ={0xd0,0xcf,0x11,0xe0,0x0e,0x11,0xfc,0x0d};

extern const IPropertySetStorageVtbl IPropertySetStorage_Vtbl;


/****************************************************************************
 * StorageInternalImpl definitions.
 *
 * Definition of the implementation structure for the IStorage interface.
 * This one implements the IStorage interface for storage that are
 * inside another storage.
 */
typedef struct StorageInternalImpl
{
  struct StorageBaseImpl base;

  /*
   * Entry in the parent's stream tracking list
   */
  struct list ParentListEntry;

  StorageBaseImpl *parentStorage;
} StorageInternalImpl;

static const IStorageVtbl StorageInternalImpl_Vtbl;
static StorageInternalImpl* StorageInternalImpl_Construct(StorageBaseImpl*,DWORD,DirRef);

typedef struct TransactedDirEntry
{
  /* If applicable, a reference to the original DirEntry in the transacted
   * parent. If this is a newly-created entry, DIRENTRY_NULL. */
  DirRef transactedParentEntry;

  /* True if this entry is being used. */
  BOOL inuse;

  /* True if data is up to date. */
  BOOL read;

  /* True if this entry has been modified. */
  BOOL dirty;

  /* True if this entry's stream has been modified. */
  BOOL stream_dirty;

  /* True if this entry has been deleted in the transacted storage, but the
   * delete has not yet been committed. */
  BOOL deleted;

  /* If this entry's stream has been modified, a reference to where the stream
   * is stored in the snapshot file. */
  DirRef stream_entry;

  /* This directory entry's data, including any changes that have been made. */
  DirEntry data;

  /* A reference to the parent of this node. This is only valid while we are
   * committing changes. */
  DirRef parent;

  /* A reference to a newly-created entry in the transacted parent. This is
   * always equal to transactedParentEntry except when committing changes. */
  DirRef newTransactedParentEntry;
} TransactedDirEntry;


/****************************************************************************
 * Transacted storage object.
 */
typedef struct TransactedSnapshotImpl
{
  struct StorageBaseImpl base;

  /*
   * Modified streams are temporarily saved to the scratch file.
   */
  StorageBaseImpl *scratch;

  /* The directory structure is kept here, so that we can track how these
   * entries relate to those in the parent storage. */
  TransactedDirEntry *entries;
  ULONG entries_size;
  ULONG firstFreeEntry;

  /*
   * Changes are committed to the transacted parent.
   */
  StorageBaseImpl *transactedParent;

  /* The transaction signature from when we last committed */
  ULONG lastTransactionSig;
} TransactedSnapshotImpl;

static const IStorageVtbl TransactedSnapshotImpl_Vtbl;
static HRESULT Storage_ConstructTransacted(StorageBaseImpl*,BOOL,StorageBaseImpl**);

typedef struct TransactedSharedImpl
{
  struct StorageBaseImpl base;

  /*
   * Snapshot and uncommitted changes go here.
   */
  TransactedSnapshotImpl *scratch;

  /*
   * Changes are committed to the transacted parent.
   */
  StorageBaseImpl *transactedParent;

  /* The transaction signature from when we last committed */
  ULONG lastTransactionSig;
} TransactedSharedImpl;


/****************************************************************************
 * BlockChainStream definitions.
 *
 * The BlockChainStream class is a utility class that is used to create an
 * abstraction of the big block chains in the storage file.
 */

struct BlockChainRun
{
  /* This represents a range of blocks that happen reside in consecutive sectors. */
  ULONG firstSector;
  ULONG firstOffset;
  ULONG lastOffset;
};

typedef struct BlockChainBlock
{
  ULONG index;
  ULONG sector;
  BOOL  read;
  BOOL  dirty;
  BYTE data[MAX_BIG_BLOCK_SIZE];
} BlockChainBlock;

struct BlockChainStream
{
  StorageImpl* parentStorage;
  ULONG*       headOfStreamPlaceHolder;
  DirRef       ownerDirEntry;
  struct BlockChainRun* indexCache;
  ULONG        indexCacheLen;
  ULONG        indexCacheSize;
  BlockChainBlock cachedBlocks[2];
  ULONG        blockToEvict;
  ULONG        tailIndex;
  ULONG        numBlocks;
};

/* Returns the number of blocks that comprises this chain.
 * This is not the size of the stream as the last block may not be full!
 */
static inline ULONG BlockChainStream_GetCount(BlockChainStream* This)
{
  return This->numBlocks;
}

static BlockChainStream* BlockChainStream_Construct(StorageImpl*,ULONG*,DirRef);
static void BlockChainStream_Destroy(BlockChainStream*);
static HRESULT BlockChainStream_ReadAt(BlockChainStream*,ULARGE_INTEGER,ULONG,void*,ULONG*);
static HRESULT BlockChainStream_WriteAt(BlockChainStream*,ULARGE_INTEGER,ULONG,const void*,ULONG*);
static HRESULT BlockChainStream_Flush(BlockChainStream*);
static ULARGE_INTEGER BlockChainStream_GetSize(BlockChainStream*);
static BOOL BlockChainStream_SetSize(BlockChainStream*,ULARGE_INTEGER);


/****************************************************************************
 * SmallBlockChainStream definitions.
 *
 * The SmallBlockChainStream class is a utility class that is used to create an
 * abstraction of the small block chains in the storage file.
 */

struct SmallBlockChainStream
{
  StorageImpl* parentStorage;
  DirRef         ownerDirEntry;
  ULONG*         headOfStreamPlaceHolder;
};

static SmallBlockChainStream* SmallBlockChainStream_Construct(StorageImpl*,ULONG*,DirRef);
static void SmallBlockChainStream_Destroy(SmallBlockChainStream*);
static HRESULT SmallBlockChainStream_ReadAt(SmallBlockChainStream*,ULARGE_INTEGER,ULONG,void*,ULONG*);
static HRESULT SmallBlockChainStream_WriteAt(SmallBlockChainStream*,ULARGE_INTEGER,ULONG,const void*,ULONG*);
static ULARGE_INTEGER SmallBlockChainStream_GetSize(SmallBlockChainStream*);
static BOOL SmallBlockChainStream_SetSize(SmallBlockChainStream*,ULARGE_INTEGER);


/************************************************************************
 * STGM Functions
 ***********************************************************************/

/************************************************************************
 * This method validates an STGM parameter that can contain the values below
 *
 * The stgm modes in 0x0000ffff are not bit masks, but distinct 4 bit values.
 * The stgm values contained in 0xffff0000 are bitmasks.
 *
 * STGM_DIRECT               0x00000000
 * STGM_TRANSACTED           0x00010000
 * STGM_SIMPLE               0x08000000
 *
 * STGM_READ                 0x00000000
 * STGM_WRITE                0x00000001
 * STGM_READWRITE            0x00000002
 *
 * STGM_SHARE_DENY_NONE      0x00000040
 * STGM_SHARE_DENY_READ      0x00000030
 * STGM_SHARE_DENY_WRITE     0x00000020
 * STGM_SHARE_EXCLUSIVE      0x00000010
 *
 * STGM_PRIORITY             0x00040000
 * STGM_DELETEONRELEASE      0x04000000
 *
 * STGM_CREATE               0x00001000
 * STGM_CONVERT              0x00020000
 * STGM_FAILIFTHERE          0x00000000
 *
 * STGM_NOSCRATCH            0x00100000
 * STGM_NOSNAPSHOT           0x00200000
 */
static HRESULT validateSTGM(DWORD stgm)
{
  DWORD access = STGM_ACCESS_MODE(stgm);
  DWORD share  = STGM_SHARE_MODE(stgm);
  DWORD create = STGM_CREATE_MODE(stgm);

  if (stgm&~STGM_KNOWN_FLAGS)
  {
    ERR("unknown flags %#lx\n", stgm);
    return E_FAIL;
  }

  switch (access)
  {
  case STGM_READ:
  case STGM_WRITE:
  case STGM_READWRITE:
    break;
  default:
    return E_FAIL;
  }

  switch (share)
  {
  case STGM_SHARE_DENY_NONE:
  case STGM_SHARE_DENY_READ:
  case STGM_SHARE_DENY_WRITE:
  case STGM_SHARE_EXCLUSIVE:
    break;
  case 0:
    if (!(stgm & STGM_TRANSACTED))
      return E_FAIL;
    break;
  default:
    return E_FAIL;
  }

  switch (create)
  {
  case STGM_CREATE:
  case STGM_FAILIFTHERE:
    break;
  default:
    return E_FAIL;
  }

  /*
   * STGM_DIRECT | STGM_TRANSACTED | STGM_SIMPLE
   */
  if ( (stgm & STGM_TRANSACTED) && (stgm & STGM_SIMPLE) )
      return E_FAIL;

  /*
   * STGM_CREATE | STGM_CONVERT
   * if both are false, STGM_FAILIFTHERE is set to TRUE
   */
  if ( create == STGM_CREATE && (stgm & STGM_CONVERT) )
    return E_FAIL;

  /*
   * STGM_NOSCRATCH requires STGM_TRANSACTED
   */
  if ( (stgm & STGM_NOSCRATCH) && !(stgm & STGM_TRANSACTED) )
    return E_FAIL;

  /*
   * STGM_NOSNAPSHOT requires STGM_TRANSACTED and
   * not STGM_SHARE_EXCLUSIVE or STGM_SHARE_DENY_WRITE`
   */
  if ( (stgm & STGM_NOSNAPSHOT) &&
        (!(stgm & STGM_TRANSACTED) ||
         share == STGM_SHARE_EXCLUSIVE ||
         share == STGM_SHARE_DENY_WRITE) )
    return E_FAIL;

  return S_OK;
}

/************************************************************************
 *      GetShareModeFromSTGM
 *
 * This method will return a share mode flag from a STGM value.
 * The STGM value is assumed valid.
 */
static DWORD GetShareModeFromSTGM(DWORD stgm)
{
  switch (STGM_SHARE_MODE(stgm))
  {
  case 0:
    assert(stgm & STGM_TRANSACTED);
    /* fall-through */
  case STGM_SHARE_DENY_NONE:
    return FILE_SHARE_READ | FILE_SHARE_WRITE;
  case STGM_SHARE_DENY_READ:
    return FILE_SHARE_WRITE;
  case STGM_SHARE_DENY_WRITE:
  case STGM_SHARE_EXCLUSIVE:
    return FILE_SHARE_READ;
  }
  ERR("Invalid share mode!\n");
  assert(0);
  return 0;
}

/************************************************************************
 *      GetAccessModeFromSTGM
 *
 * This method will return an access mode flag from a STGM value.
 * The STGM value is assumed valid.
 */
static DWORD GetAccessModeFromSTGM(DWORD stgm)
{
  switch (STGM_ACCESS_MODE(stgm))
  {
  case STGM_READ:
    return GENERIC_READ;
  case STGM_WRITE:
  case STGM_READWRITE:
    return GENERIC_READ | GENERIC_WRITE;
  }
  ERR("Invalid access mode!\n");
  assert(0);
  return 0;
}

/************************************************************************
 *      GetCreationModeFromSTGM
 *
 * This method will return a creation mode flag from a STGM value.
 * The STGM value is assumed valid.
 */
static DWORD GetCreationModeFromSTGM(DWORD stgm)
{
  switch(STGM_CREATE_MODE(stgm))
  {
  case STGM_CREATE:
    return CREATE_ALWAYS;
  case STGM_CONVERT:
    FIXME("STGM_CONVERT not implemented!\n");
    return CREATE_NEW;
  case STGM_FAILIFTHERE:
    return CREATE_NEW;
  }
  ERR("Invalid create mode!\n");
  assert(0);
  return 0;
}


/************************************************************************
 * IDirectWriterLock implementation
 ***********************************************************************/

static inline StorageBaseImpl *impl_from_IDirectWriterLock( IDirectWriterLock *iface )
{
    return CONTAINING_RECORD(iface, StorageBaseImpl, IDirectWriterLock_iface);
}

static HRESULT WINAPI directwriterlock_QueryInterface(IDirectWriterLock *iface, REFIID riid, void **obj)
{
  StorageBaseImpl *This = impl_from_IDirectWriterLock(iface);
  return IStorage_QueryInterface(&This->IStorage_iface, riid, obj);
}

static ULONG WINAPI directwriterlock_AddRef(IDirectWriterLock *iface)
{
  StorageBaseImpl *This = impl_from_IDirectWriterLock(iface);
  return IStorage_AddRef(&This->IStorage_iface);
}

static ULONG WINAPI directwriterlock_Release(IDirectWriterLock *iface)
{
  StorageBaseImpl *This = impl_from_IDirectWriterLock(iface);
  return IStorage_Release(&This->IStorage_iface);
}

static HRESULT WINAPI directwriterlock_WaitForWriteAccess(IDirectWriterLock *iface, DWORD timeout)
{
  StorageBaseImpl *This = impl_from_IDirectWriterLock(iface);
  FIXME("%p, %ld: stub\n", This, timeout);
  return E_NOTIMPL;
}

static HRESULT WINAPI directwriterlock_ReleaseWriteAccess(IDirectWriterLock *iface)
{
  StorageBaseImpl *This = impl_from_IDirectWriterLock(iface);
  FIXME("(%p): stub\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI directwriterlock_HaveWriteAccess(IDirectWriterLock *iface)
{
  StorageBaseImpl *This = impl_from_IDirectWriterLock(iface);
  FIXME("(%p): stub\n", This);
  return E_NOTIMPL;
}

static const IDirectWriterLockVtbl DirectWriterLockVtbl =
{
  directwriterlock_QueryInterface,
  directwriterlock_AddRef,
  directwriterlock_Release,
  directwriterlock_WaitForWriteAccess,
  directwriterlock_ReleaseWriteAccess,
  directwriterlock_HaveWriteAccess
};


/************************************************************************
 * StorageBaseImpl implementation : Tree helper functions
 ***********************************************************************/

/****************************************************************************
 *
 * Internal Method
 *
 * Case insensitive comparison of DirEntry.name by first considering
 * their size.
 *
 * Returns <0 when name1 < name2
 *         >0 when name1 > name2
 *          0 when name1 == name2
 */
static LONG entryNameCmp(
    const OLECHAR *name1,
    const OLECHAR *name2)
{
  LONG diff      = lstrlenW(name1) - lstrlenW(name2);

  while (diff == 0 && *name1 != 0)
  {
    /*
     * We compare the string themselves only when they are of the same length
     */
    diff = towupper(*name1++) - towupper(*name2++);
  }

  return diff;
}

/****************************************************************************
 *
 * Internal Method
 *
 * Find and read the element of a storage with the given name.
 */
static DirRef findElement(StorageBaseImpl *storage, DirRef storageEntry,
    const OLECHAR *name, DirEntry *data)
{
  DirRef currentEntry;

  /* Read the storage entry to find the root of the tree. */
  StorageBaseImpl_ReadDirEntry(storage, storageEntry, data);

  currentEntry = data->dirRootEntry;

  while (currentEntry != DIRENTRY_NULL)
  {
    LONG cmp;

    StorageBaseImpl_ReadDirEntry(storage, currentEntry, data);

    cmp = entryNameCmp(name, data->name);

    if (cmp == 0)
      /* found it */
      break;

    else if (cmp < 0)
      currentEntry = data->leftChild;

    else if (cmp > 0)
      currentEntry = data->rightChild;
  }

  return currentEntry;
}

/****************************************************************************
 *
 * Internal Method
 *
 * Find and read the binary tree parent of the element with the given name.
 *
 * If there is no such element, find a place where it could be inserted and
 * return STG_E_FILENOTFOUND.
 */
static HRESULT findTreeParent(StorageBaseImpl *storage, DirRef storageEntry,
    const OLECHAR *childName, DirEntry *parentData, DirRef *parentEntry,
    ULONG *relation)
{
  DirRef childEntry;
  DirEntry childData;

  /* Read the storage entry to find the root of the tree. */
  StorageBaseImpl_ReadDirEntry(storage, storageEntry, parentData);

  *parentEntry = storageEntry;
  *relation = DIRENTRY_RELATION_DIR;

  childEntry = parentData->dirRootEntry;

  while (childEntry != DIRENTRY_NULL)
  {
    LONG cmp;

    StorageBaseImpl_ReadDirEntry(storage, childEntry, &childData);

    cmp = entryNameCmp(childName, childData.name);

    if (cmp == 0)
      /* found it */
      break;

    else if (cmp < 0)
    {
      *parentData = childData;
      *parentEntry = childEntry;
      *relation = DIRENTRY_RELATION_PREVIOUS;

      childEntry = parentData->leftChild;
    }

    else if (cmp > 0)
    {
      *parentData = childData;
      *parentEntry = childEntry;
      *relation = DIRENTRY_RELATION_NEXT;

      childEntry = parentData->rightChild;
    }
  }

  if (childEntry == DIRENTRY_NULL)
    return STG_E_FILENOTFOUND;
  else
    return S_OK;
}

static void setEntryLink(DirEntry *entry, ULONG relation, DirRef new_target)
{
  switch (relation)
  {
    case DIRENTRY_RELATION_PREVIOUS:
      entry->leftChild = new_target;
      break;
    case DIRENTRY_RELATION_NEXT:
      entry->rightChild = new_target;
      break;
    case DIRENTRY_RELATION_DIR:
      entry->dirRootEntry = new_target;
      break;
    default:
      assert(0);
  }
}

/****************************************************************************
 *
 * Internal Method
 *
 * Add a directory entry to a storage
 */
static HRESULT insertIntoTree(
  StorageBaseImpl *This,
  DirRef        parentStorageIndex,
  DirRef        newEntryIndex)
{
  DirEntry currentEntry;
  DirEntry newEntry;

  /*
   * Read the inserted entry
   */
  StorageBaseImpl_ReadDirEntry(This,
                               newEntryIndex,
                               &newEntry);

  /*
   * Read the storage entry
   */
  StorageBaseImpl_ReadDirEntry(This,
                               parentStorageIndex,
                               &currentEntry);

  if (currentEntry.dirRootEntry != DIRENTRY_NULL)
  {
    /*
     * The root storage contains some element, therefore, start the research
     * for the appropriate location.
     */
    BOOL found = FALSE;
    DirRef current, next, previous, currentEntryId;

    /*
     * Keep a reference to the root of the storage's element tree
     */
    currentEntryId = currentEntry.dirRootEntry;

    /*
     * Read
     */
    StorageBaseImpl_ReadDirEntry(This,
                                 currentEntry.dirRootEntry,
                                 &currentEntry);

    previous = currentEntry.leftChild;
    next     = currentEntry.rightChild;
    current  = currentEntryId;

    while (!found)
    {
      LONG diff = entryNameCmp( newEntry.name, currentEntry.name);

      if (diff < 0)
      {
        if (previous != DIRENTRY_NULL)
        {
          StorageBaseImpl_ReadDirEntry(This,
                                       previous,
                                       &currentEntry);
          current = previous;
        }
        else
        {
          currentEntry.leftChild = newEntryIndex;
          StorageBaseImpl_WriteDirEntry(This,
                                        current,
                                        &currentEntry);
          found = TRUE;
        }
      }
      else if (diff > 0)
      {
        if (next != DIRENTRY_NULL)
        {
          StorageBaseImpl_ReadDirEntry(This,
                                       next,
                                       &currentEntry);
          current = next;
        }
        else
        {
          currentEntry.rightChild = newEntryIndex;
          StorageBaseImpl_WriteDirEntry(This,
                                        current,
                                        &currentEntry);
          found = TRUE;
        }
      }
      else
      {
	/*
	 * Trying to insert an item with the same name in the
	 * subtree structure.
	 */
	return STG_E_FILEALREADYEXISTS;
      }

      previous = currentEntry.leftChild;
      next     = currentEntry.rightChild;
    }
  }
  else
  {
    /*
     * The storage is empty, make the new entry the root of its element tree
     */
    currentEntry.dirRootEntry = newEntryIndex;
    StorageBaseImpl_WriteDirEntry(This,
                                  parentStorageIndex,
                                  &currentEntry);
  }

  return S_OK;
}

/*************************************************************************
 *
 * Internal Method
 *
 * This method removes a directory entry from its parent storage tree without
 * freeing any resources attached to it.
 */
static HRESULT removeFromTree(
  StorageBaseImpl *This,
  DirRef        parentStorageIndex,
  DirRef        deletedIndex)
{
  DirEntry   entryToDelete;
  DirEntry   parentEntry;
  DirRef parentEntryRef;
  ULONG typeOfRelation;
  HRESULT hr;

  hr = StorageBaseImpl_ReadDirEntry(This, deletedIndex, &entryToDelete);

  if (hr != S_OK)
    return hr;

  /*
   * Find the element that links to the one we want to delete.
   */
  hr = findTreeParent(This, parentStorageIndex, entryToDelete.name,
    &parentEntry, &parentEntryRef, &typeOfRelation);

  if (hr != S_OK)
    return hr;

  if (entryToDelete.leftChild != DIRENTRY_NULL)
  {
    /*
     * Replace the deleted entry with its left child
     */
    setEntryLink(&parentEntry, typeOfRelation, entryToDelete.leftChild);

    hr = StorageBaseImpl_WriteDirEntry(
            This,
            parentEntryRef,
            &parentEntry);
    if(FAILED(hr))
    {
      return hr;
    }

    if (entryToDelete.rightChild != DIRENTRY_NULL)
    {
      /*
       * We need to reinsert the right child somewhere. We already know it and
       * its children are greater than everything in the left tree, so we
       * insert it at the rightmost point in the left tree.
       */
      DirRef newRightChildParent = entryToDelete.leftChild;
      DirEntry newRightChildParentEntry;

      do
      {
        hr = StorageBaseImpl_ReadDirEntry(
                This,
                newRightChildParent,
                &newRightChildParentEntry);
        if (FAILED(hr))
        {
          return hr;
        }

        if (newRightChildParentEntry.rightChild != DIRENTRY_NULL)
          newRightChildParent = newRightChildParentEntry.rightChild;
      } while (newRightChildParentEntry.rightChild != DIRENTRY_NULL);

      newRightChildParentEntry.rightChild = entryToDelete.rightChild;

      hr = StorageBaseImpl_WriteDirEntry(
              This,
              newRightChildParent,
              &newRightChildParentEntry);
      if (FAILED(hr))
      {
        return hr;
      }
    }
  }
  else
  {
    /*
     * Replace the deleted entry with its right child
     */
    setEntryLink(&parentEntry, typeOfRelation, entryToDelete.rightChild);

    hr = StorageBaseImpl_WriteDirEntry(
            This,
            parentEntryRef,
            &parentEntry);
    if(FAILED(hr))
    {
      return hr;
    }
  }

  return hr;
}


/************************************************************************
 * IEnumSTATSTGImpl implementation for StorageBaseImpl_EnumElements
 ***********************************************************************/

/*
 * IEnumSTATSTGImpl definitions.
 *
 * Definition of the implementation structure for the IEnumSTATSTGImpl interface.
 * This class allows iterating through the content of a storage and finding
 * specific items inside it.
 */
struct IEnumSTATSTGImpl
{
  IEnumSTATSTG   IEnumSTATSTG_iface;

  LONG           ref;                   /* Reference count */
  StorageBaseImpl* parentStorage;         /* Reference to the parent storage */
  DirRef         storageDirEntry;     /* Directory entry of the storage to enumerate */

  WCHAR	         name[DIRENTRY_NAME_MAX_LEN]; /* The most recent name visited */
};

static inline IEnumSTATSTGImpl *impl_from_IEnumSTATSTG(IEnumSTATSTG *iface)
{
  return CONTAINING_RECORD(iface, IEnumSTATSTGImpl, IEnumSTATSTG_iface);
}

static void IEnumSTATSTGImpl_Destroy(IEnumSTATSTGImpl* This)
{
  IStorage_Release(&This->parentStorage->IStorage_iface);
  HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI IEnumSTATSTGImpl_QueryInterface(
  IEnumSTATSTG*     iface,
  REFIID            riid,
  void**            ppvObject)
{
  IEnumSTATSTGImpl* const This = impl_from_IEnumSTATSTG(iface);

  TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), ppvObject);

  if (ppvObject==0)
    return E_INVALIDARG;

  *ppvObject = 0;

  if (IsEqualGUID(&IID_IUnknown, riid) ||
      IsEqualGUID(&IID_IEnumSTATSTG, riid))
  {
    *ppvObject = &This->IEnumSTATSTG_iface;
    IEnumSTATSTG_AddRef(&This->IEnumSTATSTG_iface);
    TRACE("<-- %p\n", *ppvObject);
    return S_OK;
  }

  TRACE("<-- E_NOINTERFACE\n");
  return E_NOINTERFACE;
}

static ULONG   WINAPI IEnumSTATSTGImpl_AddRef(
  IEnumSTATSTG* iface)
{
  IEnumSTATSTGImpl* const This = impl_from_IEnumSTATSTG(iface);
  return InterlockedIncrement(&This->ref);
}

static ULONG   WINAPI IEnumSTATSTGImpl_Release(
  IEnumSTATSTG* iface)
{
  IEnumSTATSTGImpl* const This = impl_from_IEnumSTATSTG(iface);

  ULONG newRef;

  newRef = InterlockedDecrement(&This->ref);

  if (newRef==0)
  {
    IEnumSTATSTGImpl_Destroy(This);
  }

  return newRef;
}

static HRESULT IEnumSTATSTGImpl_GetNextRef(
  IEnumSTATSTGImpl* This,
  DirRef *ref)
{
  DirRef result = DIRENTRY_NULL;
  DirRef searchNode;
  DirEntry entry;
  HRESULT hr;
  WCHAR result_name[DIRENTRY_NAME_MAX_LEN];

  TRACE("%p,%p\n", This, ref);

  hr = StorageBaseImpl_ReadDirEntry(This->parentStorage,
    This->parentStorage->storageDirEntry, &entry);
  searchNode = entry.dirRootEntry;

  while (SUCCEEDED(hr) && searchNode != DIRENTRY_NULL)
  {
    hr = StorageBaseImpl_ReadDirEntry(This->parentStorage, searchNode, &entry);

    if (SUCCEEDED(hr))
    {
      LONG diff = entryNameCmp( entry.name, This->name);

      if (diff <= 0)
      {
        searchNode = entry.rightChild;
      }
      else
      {
        result = searchNode;
        memcpy(result_name, entry.name, sizeof(result_name));
        searchNode = entry.leftChild;
      }
    }
  }

  if (SUCCEEDED(hr))
  {
    *ref = result;
    if (result != DIRENTRY_NULL)
      memcpy(This->name, result_name, sizeof(result_name));
  }

  TRACE("<-- %#lx\n", hr);
  return hr;
}

static HRESULT WINAPI IEnumSTATSTGImpl_Next(
  IEnumSTATSTG* iface,
  ULONG             celt,
  STATSTG*          rgelt,
  ULONG*            pceltFetched)
{
  IEnumSTATSTGImpl* const This = impl_from_IEnumSTATSTG(iface);

  DirEntry    currentEntry;
  STATSTG*    currentReturnStruct = rgelt;
  ULONG       objectFetched       = 0;
  DirRef      currentSearchNode;
  HRESULT     hr=S_OK;

  TRACE("%p, %lu, %p, %p.\n", iface, celt, rgelt, pceltFetched);

  if ( (rgelt==0) || ( (celt!=1) && (pceltFetched==0) ) )
    return E_INVALIDARG;

  if (This->parentStorage->reverted)
  {
    TRACE("<-- STG_E_REVERTED\n");
    return STG_E_REVERTED;
  }

  /*
   * To avoid the special case, get another pointer to a ULONG value if
   * the caller didn't supply one.
   */
  if (pceltFetched==0)
    pceltFetched = &objectFetched;

  /*
   * Start the iteration, we will iterate until we hit the end of the
   * linked list or until we hit the number of items to iterate through
   */
  *pceltFetched = 0;

  while ( *pceltFetched < celt )
  {
    hr = IEnumSTATSTGImpl_GetNextRef(This, &currentSearchNode);

    if (FAILED(hr) || currentSearchNode == DIRENTRY_NULL)
    {
      memset(currentReturnStruct, 0, sizeof(*currentReturnStruct));
      break;
    }

    /*
     * Read the entry from the storage.
     */
    hr = StorageBaseImpl_ReadDirEntry(This->parentStorage,
      currentSearchNode,
      &currentEntry);
    if (FAILED(hr)) break;

    /*
     * Copy the information to the return buffer.
     */
    StorageUtl_CopyDirEntryToSTATSTG(This->parentStorage,
      currentReturnStruct,
      &currentEntry,
      STATFLAG_DEFAULT);

    /*
     * Step to the next item in the iteration
     */
    (*pceltFetched)++;
    currentReturnStruct++;
  }

  if (SUCCEEDED(hr) && *pceltFetched != celt)
    hr = S_FALSE;

  TRACE("<-- %#lx (asked %lu, got %lu)\n", hr, celt, *pceltFetched);
  return hr;
}


static HRESULT WINAPI IEnumSTATSTGImpl_Skip(
  IEnumSTATSTG* iface,
  ULONG             celt)
{
  IEnumSTATSTGImpl* const This = impl_from_IEnumSTATSTG(iface);

  ULONG       objectFetched = 0;
  DirRef      currentSearchNode;
  HRESULT     hr=S_OK;

  TRACE("%p, %lu.\n", iface, celt);

  if (This->parentStorage->reverted)
  {
    TRACE("<-- STG_E_REVERTED\n");
    return STG_E_REVERTED;
  }

  while ( (objectFetched < celt) )
  {
    hr = IEnumSTATSTGImpl_GetNextRef(This, &currentSearchNode);

    if (FAILED(hr) || currentSearchNode == DIRENTRY_NULL)
      break;

    objectFetched++;
  }

  if (SUCCEEDED(hr) && objectFetched != celt)
    return S_FALSE;

  TRACE("<-- %#lx\n", hr);
  return hr;
}

static HRESULT WINAPI IEnumSTATSTGImpl_Reset(
  IEnumSTATSTG* iface)
{
  IEnumSTATSTGImpl* const This = impl_from_IEnumSTATSTG(iface);

  TRACE("%p\n", iface);

  if (This->parentStorage->reverted)
  {
    TRACE("<-- STG_E_REVERTED\n");
    return STG_E_REVERTED;
  }

  This->name[0] = 0;

  return S_OK;
}

static IEnumSTATSTGImpl* IEnumSTATSTGImpl_Construct(StorageBaseImpl*,DirRef);

static HRESULT WINAPI IEnumSTATSTGImpl_Clone(
  IEnumSTATSTG* iface,
  IEnumSTATSTG**    ppenum)
{
  IEnumSTATSTGImpl* const This = impl_from_IEnumSTATSTG(iface);
  IEnumSTATSTGImpl* newClone;

  TRACE("%p,%p\n", iface, ppenum);

  if (This->parentStorage->reverted)
  {
    TRACE("<-- STG_E_REVERTED\n");
    return STG_E_REVERTED;
  }

  if (ppenum==0)
    return E_INVALIDARG;

  newClone = IEnumSTATSTGImpl_Construct(This->parentStorage,
               This->storageDirEntry);
  if (!newClone)
  {
    *ppenum = NULL;
    return E_OUTOFMEMORY;
  }

  /*
   * The new clone enumeration must point to the same current node as
   * the old one.
   */
  memcpy(newClone->name, This->name, sizeof(newClone->name));

  *ppenum = &newClone->IEnumSTATSTG_iface;

  return S_OK;
}

/*
 * Virtual function table for the IEnumSTATSTGImpl class.
 */
static const IEnumSTATSTGVtbl IEnumSTATSTGImpl_Vtbl =
{
    IEnumSTATSTGImpl_QueryInterface,
    IEnumSTATSTGImpl_AddRef,
    IEnumSTATSTGImpl_Release,
    IEnumSTATSTGImpl_Next,
    IEnumSTATSTGImpl_Skip,
    IEnumSTATSTGImpl_Reset,
    IEnumSTATSTGImpl_Clone
};

static IEnumSTATSTGImpl* IEnumSTATSTGImpl_Construct(
  StorageBaseImpl* parentStorage,
  DirRef         storageDirEntry)
{
  IEnumSTATSTGImpl* newEnumeration;

  newEnumeration = HeapAlloc(GetProcessHeap(), 0, sizeof(IEnumSTATSTGImpl));

  if (newEnumeration)
  {
    newEnumeration->IEnumSTATSTG_iface.lpVtbl = &IEnumSTATSTGImpl_Vtbl;
    newEnumeration->ref = 1;
    newEnumeration->name[0] = 0;

    /*
     * We want to nail-down the reference to the storage in case the
     * enumeration out-lives the storage in the client application.
     */
    newEnumeration->parentStorage = parentStorage;
    IStorage_AddRef(&newEnumeration->parentStorage->IStorage_iface);

    newEnumeration->storageDirEntry = storageDirEntry;
  }

  return newEnumeration;
}


/************************************************************************
 * StorageBaseImpl implementation
 ***********************************************************************/

static inline StorageBaseImpl *impl_from_IStorage( IStorage *iface )
{
    return CONTAINING_RECORD(iface, StorageBaseImpl, IStorage_iface);
}

/************************************************************************
 * StorageBaseImpl_QueryInterface (IUnknown)
 *
 * This method implements the common QueryInterface for all IStorage
 * implementations contained in this file.
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI StorageBaseImpl_QueryInterface(
  IStorage*        iface,
  REFIID             riid,
  void**             ppvObject)
{
  StorageBaseImpl *This = impl_from_IStorage(iface);

  TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), ppvObject);

  if (!ppvObject)
    return E_INVALIDARG;

  *ppvObject = 0;

  if (IsEqualGUID(&IID_IUnknown, riid) ||
      IsEqualGUID(&IID_IStorage, riid))
  {
    *ppvObject = &This->IStorage_iface;
  }
  else if (IsEqualGUID(&IID_IPropertySetStorage, riid))
  {
    *ppvObject = &This->IPropertySetStorage_iface;
  }
  /* locking interface is reported for writer only */
  else if (IsEqualGUID(&IID_IDirectWriterLock, riid) && This->lockingrole == SWMR_Writer)
  {
    *ppvObject = &This->IDirectWriterLock_iface;
  }
  else
  {
    TRACE("<-- E_NOINTERFACE\n");
    return E_NOINTERFACE;
  }

  IStorage_AddRef(iface);
  TRACE("<-- %p\n", *ppvObject);
  return S_OK;
}

/************************************************************************
 * StorageBaseImpl_AddRef (IUnknown)
 *
 * This method implements the common AddRef for all IStorage
 * implementations contained in this file.
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI StorageBaseImpl_AddRef(
            IStorage* iface)
{
  StorageBaseImpl *This = impl_from_IStorage(iface);
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("%p, refcount %lu.\n", iface, ref);

  return ref;
}

/************************************************************************
 * StorageBaseImpl_Release (IUnknown)
 *
 * This method implements the common Release for all IStorage
 * implementations contained in this file.
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI StorageBaseImpl_Release(
      IStorage* iface)
{
  StorageBaseImpl *This = impl_from_IStorage(iface);

  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("%p, refcount %lu.\n", iface, ref);

  if (ref == 0)
  {
    /*
     * Since we are using a system of base-classes, we want to call the
     * destructor of the appropriate derived class. To do this, we are
     * using virtual functions to implement the destructor.
     */
    StorageBaseImpl_Destroy(This);
  }

  return ref;
}

static HRESULT StorageBaseImpl_CopyStorageEntryTo(StorageBaseImpl *This,
    DirRef srcEntry, BOOL skip_storage, BOOL skip_stream,
    SNB snbExclude, IStorage *pstgDest);

static HRESULT StorageBaseImpl_CopyChildEntryTo(StorageBaseImpl *This,
    DirRef srcEntry, BOOL skip_storage, BOOL skip_stream,
    SNB snbExclude, IStorage *pstgDest)
{
  DirEntry data;
  HRESULT hr;
  BOOL skip = FALSE;
  IStorage *pstgTmp;
  IStream *pstrChild, *pstrTmp;
  STATSTG strStat;

  if (srcEntry == DIRENTRY_NULL)
    return S_OK;

  hr = StorageBaseImpl_ReadDirEntry( This, srcEntry, &data );

  if (FAILED(hr))
    return hr;

  if ( snbExclude )
  {
    WCHAR **snb = snbExclude;

    while ( *snb != NULL && !skip )
    {
      if ( wcscmp(data.name, *snb) == 0 )
        skip = TRUE;
      ++snb;
    }
  }

  if (!skip)
  {
    if (data.stgType == STGTY_STORAGE && !skip_storage)
    {
      /*
       * create a new storage in destination storage
       */
      hr = IStorage_CreateStorage( pstgDest, data.name,
                                   STGM_FAILIFTHERE|STGM_WRITE|STGM_SHARE_EXCLUSIVE,
                                   0, 0,
                                   &pstgTmp );

      /*
       * if it already exist, don't create a new one use this one
       */
      if (hr == STG_E_FILEALREADYEXISTS)
      {
        hr = IStorage_OpenStorage( pstgDest, data.name, NULL,
                                   STGM_WRITE|STGM_SHARE_EXCLUSIVE,
                                   NULL, 0, &pstgTmp );
      }

      if (SUCCEEDED(hr))
      {
        hr = StorageBaseImpl_CopyStorageEntryTo( This, srcEntry, skip_storage,
                                                 skip_stream, NULL, pstgTmp );

        IStorage_Release(pstgTmp);
      }
    }
    else if (data.stgType == STGTY_STREAM && !skip_stream)
    {
      /*
       * create a new stream in destination storage. If the stream already
       * exist, it will be deleted and a new one will be created.
       */
      hr = IStorage_CreateStream( pstgDest, data.name,
                                  STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE,
                                  0, 0, &pstrTmp );

      /*
       * open child stream storage. This operation must succeed even if the
       * stream is already open, so we use internal functions to do it.
       */
      if (hr == S_OK)
      {
        StgStreamImpl *streamimpl = StgStreamImpl_Construct(This, STGM_READ|STGM_SHARE_EXCLUSIVE, srcEntry);

        if (streamimpl)
        {
          pstrChild = &streamimpl->IStream_iface;
          if (pstrChild)
            IStream_AddRef(pstrChild);
        }
        else
        {
          pstrChild = NULL;
          hr = E_OUTOFMEMORY;
        }
      }

      if (hr == S_OK)
      {
        /*
         * Get the size of the source stream
         */
        IStream_Stat( pstrChild, &strStat, STATFLAG_NONAME );

        /*
         * Set the size of the destination stream.
         */
        IStream_SetSize(pstrTmp, strStat.cbSize);

        /*
         * do the copy
         */
        hr = IStream_CopyTo( pstrChild, pstrTmp, strStat.cbSize,
                             NULL, NULL );

        IStream_Release( pstrChild );
      }

      IStream_Release( pstrTmp );
    }
  }

  /* copy siblings */
  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_CopyChildEntryTo( This, data.leftChild, skip_storage,
                                           skip_stream, snbExclude, pstgDest );

  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_CopyChildEntryTo( This, data.rightChild, skip_storage,
                                           skip_stream, snbExclude, pstgDest );

  TRACE("<-- %#lx\n", hr);
  return hr;
}

static BOOL StorageBaseImpl_IsStreamOpen(StorageBaseImpl * stg, DirRef streamEntry)
{
  StgStreamImpl *strm;

  TRACE("%p, %ld.\n", stg, streamEntry);

  LIST_FOR_EACH_ENTRY(strm, &stg->strmHead, StgStreamImpl, StrmListEntry)
  {
    if (strm->dirEntry == streamEntry)
    {
      return TRUE;
    }
  }

  return FALSE;
}

static BOOL StorageBaseImpl_IsStorageOpen(StorageBaseImpl * stg, DirRef storageEntry)
{
  StorageInternalImpl *childstg;

  TRACE("%p, %ld.\n", stg, storageEntry);

  LIST_FOR_EACH_ENTRY(childstg, &stg->storageHead, StorageInternalImpl, ParentListEntry)
  {
    if (childstg->base.storageDirEntry == storageEntry)
    {
      return TRUE;
    }
  }

  return FALSE;
}

/************************************************************************
 * StorageBaseImpl_OpenStream (IStorage)
 *
 * This method will open the specified stream object from the current storage.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageBaseImpl_OpenStream(
  IStorage*        iface,
  const OLECHAR*   pwcsName,  /* [string][in] */
  void*            reserved1, /* [unique][in] */
  DWORD            grfMode,   /* [in]  */
  DWORD            reserved2, /* [in]  */
  IStream**        ppstm)     /* [out] */
{
  StorageBaseImpl *This = impl_from_IStorage(iface);
  StgStreamImpl*    newStream;
  DirEntry          currentEntry;
  DirRef            streamEntryRef;
  HRESULT           res = STG_E_UNKNOWN;

  TRACE("%p, %s, %p, %#lx, %ld, %p.\n", iface, debugstr_w(pwcsName), reserved1, grfMode, reserved2, ppstm);

  if ( (pwcsName==NULL) || (ppstm==0) )
  {
    res = E_INVALIDARG;
    goto end;
  }

  *ppstm = NULL;

  if ( FAILED( validateSTGM(grfMode) ) ||
       STGM_SHARE_MODE(grfMode) != STGM_SHARE_EXCLUSIVE)
  {
    res = STG_E_INVALIDFLAG;
    goto end;
  }

  /*
   * As documented.
   */
  if ( (grfMode & STGM_DELETEONRELEASE) || (grfMode & STGM_TRANSACTED) )
  {
    res = STG_E_INVALIDFUNCTION;
    goto end;
  }

  if (This->reverted)
  {
    res = STG_E_REVERTED;
    goto end;
  }

  /*
   * Check that we're compatible with the parent's storage mode, but
   * only if we are not in transacted mode
   */
  if(!(This->openFlags & STGM_TRANSACTED)) {
    if ( STGM_ACCESS_MODE( grfMode ) > STGM_ACCESS_MODE( This->openFlags ) )
    {
      res = STG_E_INVALIDFLAG;
      goto end;
    }
  }

  /*
   * Search for the element with the given name
   */
  streamEntryRef = findElement(
    This,
    This->storageDirEntry,
    pwcsName,
    &currentEntry);

  /*
   * If it was found, construct the stream object and return a pointer to it.
   */
  if ( (streamEntryRef!=DIRENTRY_NULL) &&
       (currentEntry.stgType==STGTY_STREAM) )
  {
    if (StorageBaseImpl_IsStreamOpen(This, streamEntryRef))
    {
      /* A single stream cannot be opened a second time. */
      res = STG_E_ACCESSDENIED;
      goto end;
    }

    newStream = StgStreamImpl_Construct(This, grfMode, streamEntryRef);

    if (newStream)
    {
      newStream->grfMode = grfMode;
      *ppstm = &newStream->IStream_iface;

      IStream_AddRef(*ppstm);

      res = S_OK;
      goto end;
    }

    res = E_OUTOFMEMORY;
    goto end;
  }

  res = STG_E_FILENOTFOUND;

end:
  if (res == S_OK)
    TRACE("<-- IStream %p\n", *ppstm);
  TRACE("<-- %#lx\n", res);
  return res;
}

/************************************************************************
 * StorageBaseImpl_OpenStorage (IStorage)
 *
 * This method will open a new storage object from the current storage.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageBaseImpl_OpenStorage(
  IStorage*        iface,
  const OLECHAR*   pwcsName,      /* [string][unique][in] */
  IStorage*        pstgPriority,  /* [unique][in] */
  DWORD            grfMode,       /* [in] */
  SNB              snbExclude,    /* [unique][in] */
  DWORD            reserved,      /* [in] */
  IStorage**       ppstg)         /* [out] */
{
  StorageBaseImpl *This = impl_from_IStorage(iface);
  StorageInternalImpl*   newStorage;
  StorageBaseImpl*       newTransactedStorage;
  DirEntry               currentEntry;
  DirRef                 storageEntryRef;
  HRESULT                res = STG_E_UNKNOWN;

  TRACE("%p, %s, %p, %#lx, %p, %ld, %p.\n", iface, debugstr_w(pwcsName), pstgPriority,
      grfMode, snbExclude, reserved, ppstg);

  if ((pwcsName==NULL) || (ppstg==0) )
  {
    res = E_INVALIDARG;
    goto end;
  }

  if (This->openFlags & STGM_SIMPLE)
  {
    res = STG_E_INVALIDFUNCTION;
    goto end;
  }

  /* as documented */
  if (snbExclude != NULL)
  {
    res = STG_E_INVALIDPARAMETER;
    goto end;
  }

  if ( FAILED( validateSTGM(grfMode) ))
  {
    res = STG_E_INVALIDFLAG;
    goto end;
  }

  /*
   * As documented.
   */
  if ( STGM_SHARE_MODE(grfMode) != STGM_SHARE_EXCLUSIVE ||
        (grfMode & STGM_DELETEONRELEASE) ||
        (grfMode & STGM_PRIORITY) )
  {
    res = STG_E_INVALIDFUNCTION;
    goto end;
  }

  if (This->reverted)
    return STG_E_REVERTED;

  /*
   * Check that we're compatible with the parent's storage mode,
   * but only if we are not transacted
   */
  if(!(This->openFlags & STGM_TRANSACTED)) {
    if ( STGM_ACCESS_MODE( grfMode ) > STGM_ACCESS_MODE( This->openFlags ) )
    {
      res = STG_E_ACCESSDENIED;
      goto end;
    }
  }

  *ppstg = NULL;

  storageEntryRef = findElement(
                         This,
                         This->storageDirEntry,
                         pwcsName,
                         &currentEntry);

  if ( (storageEntryRef!=DIRENTRY_NULL) &&
       (currentEntry.stgType==STGTY_STORAGE) )
  {
    if (StorageBaseImpl_IsStorageOpen(This, storageEntryRef))
    {
      /* A single storage cannot be opened a second time. */
      res = STG_E_ACCESSDENIED;
      goto end;
    }

    newStorage = StorageInternalImpl_Construct(
                   This,
                   grfMode,
                   storageEntryRef);

    if (newStorage != 0)
    {
      if (grfMode & STGM_TRANSACTED)
      {
        res = Storage_ConstructTransacted(&newStorage->base, FALSE, &newTransactedStorage);

        if (FAILED(res))
        {
          HeapFree(GetProcessHeap(), 0, newStorage);
          goto end;
        }

        *ppstg = &newTransactedStorage->IStorage_iface;
      }
      else
      {
        *ppstg = &newStorage->base.IStorage_iface;
      }

      list_add_tail(&This->storageHead, &newStorage->ParentListEntry);

      res = S_OK;
      goto end;
    }

    res = STG_E_INSUFFICIENTMEMORY;
    goto end;
  }

  res = STG_E_FILENOTFOUND;

end:
  TRACE("<-- %#lx\n", res);
  return res;
}

/************************************************************************
 * StorageBaseImpl_EnumElements (IStorage)
 *
 * This method will create an enumerator object that can be used to
 * retrieve information about all the elements in the storage object.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageBaseImpl_EnumElements(
  IStorage*       iface,
  DWORD           reserved1, /* [in] */
  void*           reserved2, /* [size_is][unique][in] */
  DWORD           reserved3, /* [in] */
  IEnumSTATSTG**  ppenum)    /* [out] */
{
  StorageBaseImpl *This = impl_from_IStorage(iface);
  IEnumSTATSTGImpl* newEnum;

  TRACE("%p, %ld, %p, %ld, %p.\n", iface, reserved1, reserved2, reserved3, ppenum);

  if (!ppenum)
    return E_INVALIDARG;

  if (This->reverted)
    return STG_E_REVERTED;

  newEnum = IEnumSTATSTGImpl_Construct(
              This,
              This->storageDirEntry);

  if (newEnum)
  {
    *ppenum = &newEnum->IEnumSTATSTG_iface;
    return S_OK;
  }

  return E_OUTOFMEMORY;
}

/************************************************************************
 * StorageBaseImpl_Stat (IStorage)
 *
 * This method will retrieve information about this storage object.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageBaseImpl_Stat(
  IStorage*        iface,
  STATSTG*         pstatstg,     /* [out] */
  DWORD            grfStatFlag)  /* [in] */
{
  StorageBaseImpl *This = impl_from_IStorage(iface);
  DirEntry       currentEntry;
  HRESULT        res = STG_E_UNKNOWN;

  TRACE("%p, %p, %#lx.\n", iface, pstatstg, grfStatFlag);

  if (!pstatstg)
  {
    res = E_INVALIDARG;
    goto end;
  }

  if (This->reverted)
  {
    res = STG_E_REVERTED;
    goto end;
  }

  res = StorageBaseImpl_ReadDirEntry(
                    This,
                    This->storageDirEntry,
                    &currentEntry);

  if (SUCCEEDED(res))
  {
    StorageUtl_CopyDirEntryToSTATSTG(
      This,
      pstatstg,
      &currentEntry,
      grfStatFlag);

    pstatstg->grfMode = This->openFlags;
    pstatstg->grfStateBits = This->stateBits;
  }

end:
  if (res == S_OK)
  {
    TRACE("<-- STATSTG: pwcsName: %s, type: %ld, cbSize.Low/High: %ld/%ld, grfMode: %#lx, grfLocksSupported: %ld, grfStateBits: %#lx\n", debugstr_w(pstatstg->pwcsName), pstatstg->type, pstatstg->cbSize.LowPart, pstatstg->cbSize.HighPart, pstatstg->grfMode, pstatstg->grfLocksSupported, pstatstg->grfStateBits);
  }
  TRACE("<-- %#lx\n", res);
  return res;
}

/************************************************************************
 * StorageBaseImpl_RenameElement (IStorage)
 *
 * This method will rename the specified element.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageBaseImpl_RenameElement(
            IStorage*        iface,
            const OLECHAR*   pwcsOldName,  /* [in] */
            const OLECHAR*   pwcsNewName)  /* [in] */
{
  StorageBaseImpl *This = impl_from_IStorage(iface);
  DirEntry          currentEntry;
  DirRef            currentEntryRef;

  TRACE("(%p, %s, %s)\n",
	iface, debugstr_w(pwcsOldName), debugstr_w(pwcsNewName));

  if (This->reverted)
    return STG_E_REVERTED;

  currentEntryRef = findElement(This,
                                   This->storageDirEntry,
                                   pwcsNewName,
                                   &currentEntry);

  if (currentEntryRef != DIRENTRY_NULL)
  {
    /*
     * There is already an element with the new name
     */
    return STG_E_FILEALREADYEXISTS;
  }

  /*
   * Search for the old element name
   */
  currentEntryRef = findElement(This,
                                   This->storageDirEntry,
                                   pwcsOldName,
                                   &currentEntry);

  if (currentEntryRef != DIRENTRY_NULL)
  {
    if (StorageBaseImpl_IsStreamOpen(This, currentEntryRef) ||
        StorageBaseImpl_IsStorageOpen(This, currentEntryRef))
    {
      WARN("Element is already open; cannot rename.\n");
      return STG_E_ACCESSDENIED;
    }

    /* Remove the element from its current position in the tree */
    removeFromTree(This, This->storageDirEntry,
        currentEntryRef);

    /* Change the name of the element */
    lstrcpyW(currentEntry.name, pwcsNewName);

    /* Delete any sibling links */
    currentEntry.leftChild = DIRENTRY_NULL;
    currentEntry.rightChild = DIRENTRY_NULL;

    StorageBaseImpl_WriteDirEntry(This, currentEntryRef,
        &currentEntry);

    /* Insert the element in a new position in the tree */
    insertIntoTree(This, This->storageDirEntry,
        currentEntryRef);
  }
  else
  {
    /*
     * There is no element with the old name
     */
    return STG_E_FILENOTFOUND;
  }

  return StorageBaseImpl_Flush(This);
}

/************************************************************************
 * StorageBaseImpl_CreateStream (IStorage)
 *
 * This method will create a stream object within this storage
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageBaseImpl_CreateStream(
            IStorage*        iface,
            const OLECHAR*   pwcsName,  /* [string][in] */
            DWORD            grfMode,   /* [in] */
            DWORD            reserved1, /* [in] */
            DWORD            reserved2, /* [in] */
            IStream**        ppstm)     /* [out] */
{
  StorageBaseImpl *This = impl_from_IStorage(iface);
  StgStreamImpl*    newStream;
  DirEntry          currentEntry, newStreamEntry;
  DirRef            currentEntryRef, newStreamEntryRef;
  HRESULT hr;

  TRACE("%p, %s, %#lx, %ld, %ld, %p.\n", iface, debugstr_w(pwcsName), grfMode, reserved1, reserved2, ppstm);

  if (ppstm == 0)
    return STG_E_INVALIDPOINTER;

  if (pwcsName == 0)
    return STG_E_INVALIDNAME;

  if (reserved1 || reserved2)
    return STG_E_INVALIDPARAMETER;

  if ( FAILED( validateSTGM(grfMode) ))
    return STG_E_INVALIDFLAG;

  if (STGM_SHARE_MODE(grfMode) != STGM_SHARE_EXCLUSIVE)
    return STG_E_INVALIDFLAG;

  if (This->reverted)
    return STG_E_REVERTED;

  /*
   * As documented.
   */
  if ((grfMode & STGM_DELETEONRELEASE) ||
      (grfMode & STGM_TRANSACTED))
    return STG_E_INVALIDFUNCTION;

  /*
   * Don't worry about permissions in transacted mode, as we can always write
   * changes; we just can't always commit them.
   */
  if(!(This->openFlags & STGM_TRANSACTED)) {
    /* Can't create a stream on read-only storage */
    if ( STGM_ACCESS_MODE( This->openFlags ) == STGM_READ )
      return STG_E_ACCESSDENIED;

    /* Can't create a stream with greater access than the parent. */
    if ( STGM_ACCESS_MODE( grfMode ) > STGM_ACCESS_MODE( This->openFlags ) )
      return STG_E_ACCESSDENIED;
  }

  if(This->openFlags & STGM_SIMPLE)
    if(grfMode & STGM_CREATE) return STG_E_INVALIDFLAG;

  *ppstm = 0;

  currentEntryRef = findElement(This,
                                   This->storageDirEntry,
                                   pwcsName,
                                   &currentEntry);

  if (currentEntryRef != DIRENTRY_NULL)
  {
    /*
     * An element with this name already exists
     */
    if (STGM_CREATE_MODE(grfMode) == STGM_CREATE)
    {
      IStorage_DestroyElement(iface, pwcsName);
    }
    else
      return STG_E_FILEALREADYEXISTS;
  }

  /*
   * memset the empty entry
   */
  memset(&newStreamEntry, 0, sizeof(DirEntry));

  newStreamEntry.sizeOfNameString =
      ( lstrlenW(pwcsName)+1 ) * sizeof(WCHAR);

  if (newStreamEntry.sizeOfNameString > DIRENTRY_NAME_BUFFER_LEN)
    return STG_E_INVALIDNAME;

  lstrcpyW(newStreamEntry.name, pwcsName);

  newStreamEntry.stgType       = STGTY_STREAM;
  newStreamEntry.startingBlock = BLOCK_END_OF_CHAIN;
  newStreamEntry.size.LowPart  = 0;
  newStreamEntry.size.HighPart = 0;

  newStreamEntry.leftChild        = DIRENTRY_NULL;
  newStreamEntry.rightChild       = DIRENTRY_NULL;
  newStreamEntry.dirRootEntry     = DIRENTRY_NULL;

  /* call CoFileTime to get the current time
  newStreamEntry.ctime
  newStreamEntry.mtime
  */

  /*  newStreamEntry.clsid */

  /*
   * Create an entry with the new data
   */
  hr = StorageBaseImpl_CreateDirEntry(This, &newStreamEntry, &newStreamEntryRef);
  if (FAILED(hr))
    return hr;

  /*
   * Insert the new entry in the parent storage's tree.
   */
  hr = insertIntoTree(
    This,
    This->storageDirEntry,
    newStreamEntryRef);
  if (FAILED(hr))
  {
    StorageBaseImpl_DestroyDirEntry(This, newStreamEntryRef);
    return hr;
  }

  /*
   * Open the stream to return it.
   */
  newStream = StgStreamImpl_Construct(This, grfMode, newStreamEntryRef);

  if (newStream)
  {
    *ppstm = &newStream->IStream_iface;
    IStream_AddRef(*ppstm);
  }
  else
  {
    return STG_E_INSUFFICIENTMEMORY;
  }

  return StorageBaseImpl_Flush(This);
}

/************************************************************************
 * StorageBaseImpl_SetClass (IStorage)
 *
 * This method will write the specified CLSID in the directory entry of this
 * storage.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageBaseImpl_SetClass(
  IStorage*        iface,
  REFCLSID         clsid) /* [in] */
{
  StorageBaseImpl *This = impl_from_IStorage(iface);
  HRESULT hRes;
  DirEntry currentEntry;

  TRACE("(%p, %s)\n", iface, wine_dbgstr_guid(clsid));

  if (This->reverted)
    return STG_E_REVERTED;

  hRes = StorageBaseImpl_ReadDirEntry(This,
                                      This->storageDirEntry,
                                      &currentEntry);
  if (SUCCEEDED(hRes))
  {
    currentEntry.clsid = *clsid;

    hRes = StorageBaseImpl_WriteDirEntry(This,
                                         This->storageDirEntry,
                                         &currentEntry);
  }

  if (SUCCEEDED(hRes))
    hRes = StorageBaseImpl_Flush(This);

  return hRes;
}

/************************************************************************
 * StorageBaseImpl_CreateStorage (IStorage)
 *
 * This method will create the storage object within the provided storage.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageBaseImpl_CreateStorage(
  IStorage*      iface,
  const OLECHAR  *pwcsName, /* [string][in] */
  DWORD            grfMode,   /* [in] */
  DWORD            reserved1, /* [in] */
  DWORD            reserved2, /* [in] */
  IStorage       **ppstg)   /* [out] */
{
  StorageBaseImpl* This = impl_from_IStorage(iface);

  DirEntry         currentEntry;
  DirEntry         newEntry;
  DirRef           currentEntryRef;
  DirRef           newEntryRef;
  HRESULT          hr;

  TRACE("%p, %s, %#lx, %ld, %ld, %p.\n", iface, debugstr_w(pwcsName), grfMode,
      reserved1, reserved2, ppstg);

  if (ppstg == 0)
    return STG_E_INVALIDPOINTER;

  if (This->openFlags & STGM_SIMPLE)
  {
    return STG_E_INVALIDFUNCTION;
  }

  if (pwcsName == 0)
    return STG_E_INVALIDNAME;

  *ppstg = NULL;

  if ( FAILED( validateSTGM(grfMode) ) ||
       (grfMode & STGM_DELETEONRELEASE) )
  {
    WARN("bad grfMode: %#lx\n", grfMode);
    return STG_E_INVALIDFLAG;
  }

  if (This->reverted)
    return STG_E_REVERTED;

  /*
   * Check that we're compatible with the parent's storage mode
   */
  if ( !(This->openFlags & STGM_TRANSACTED) &&
       STGM_ACCESS_MODE( grfMode ) > STGM_ACCESS_MODE( This->openFlags ) )
  {
    WARN("access denied\n");
    return STG_E_ACCESSDENIED;
  }

  currentEntryRef = findElement(This,
                                   This->storageDirEntry,
                                   pwcsName,
                                   &currentEntry);

  if (currentEntryRef != DIRENTRY_NULL)
  {
    /*
     * An element with this name already exists
     */
    if (STGM_CREATE_MODE(grfMode) == STGM_CREATE &&
        ((This->openFlags & STGM_TRANSACTED) ||
         STGM_ACCESS_MODE(This->openFlags) != STGM_READ))
    {
      hr = IStorage_DestroyElement(iface, pwcsName);
      if (FAILED(hr))
        return hr;
    }
    else
    {
      WARN("file already exists\n");
      return STG_E_FILEALREADYEXISTS;
    }
  }
  else if (!(This->openFlags & STGM_TRANSACTED) &&
           STGM_ACCESS_MODE(This->openFlags) == STGM_READ)
  {
    WARN("read-only storage\n");
    return STG_E_ACCESSDENIED;
  }

  memset(&newEntry, 0, sizeof(DirEntry));

  newEntry.sizeOfNameString = (lstrlenW(pwcsName)+1)*sizeof(WCHAR);

  if (newEntry.sizeOfNameString > DIRENTRY_NAME_BUFFER_LEN)
  {
    FIXME("name too long\n");
    return STG_E_INVALIDNAME;
  }

  lstrcpyW(newEntry.name, pwcsName);

  newEntry.stgType       = STGTY_STORAGE;
  newEntry.startingBlock = BLOCK_END_OF_CHAIN;
  newEntry.size.LowPart  = 0;
  newEntry.size.HighPart = 0;

  newEntry.leftChild        = DIRENTRY_NULL;
  newEntry.rightChild       = DIRENTRY_NULL;
  newEntry.dirRootEntry     = DIRENTRY_NULL;

  /* call CoFileTime to get the current time
  newEntry.ctime
  newEntry.mtime
  */

  /*  newEntry.clsid */

  /*
   * Create a new directory entry for the storage
   */
  hr = StorageBaseImpl_CreateDirEntry(This, &newEntry, &newEntryRef);
  if (FAILED(hr))
    return hr;

  /*
   * Insert the new directory entry into the parent storage's tree
   */
  hr = insertIntoTree(
    This,
    This->storageDirEntry,
    newEntryRef);
  if (FAILED(hr))
  {
    StorageBaseImpl_DestroyDirEntry(This, newEntryRef);
    return hr;
  }

  /*
   * Open it to get a pointer to return.
   */
  hr = IStorage_OpenStorage(iface, pwcsName, 0, grfMode, 0, 0, ppstg);

  if( (hr != S_OK) || (*ppstg == NULL))
  {
    return hr;
  }

  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_Flush(This);

  return S_OK;
}

static HRESULT StorageBaseImpl_CopyStorageEntryTo(StorageBaseImpl *This,
    DirRef srcEntry, BOOL skip_storage, BOOL skip_stream,
    SNB snbExclude, IStorage *pstgDest)
{
  DirEntry data;
  HRESULT hr;

  hr = StorageBaseImpl_ReadDirEntry( This, srcEntry, &data );

  if (SUCCEEDED(hr))
    hr = IStorage_SetClass( pstgDest, &data.clsid );

  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_CopyChildEntryTo( This, data.dirRootEntry, skip_storage,
      skip_stream, snbExclude, pstgDest );

  TRACE("<-- %#lx\n", hr);
  return hr;
}

/*************************************************************************
 * CopyTo (IStorage)
 */
static HRESULT WINAPI StorageBaseImpl_CopyTo(
  IStorage*   iface,
  DWORD       ciidExclude,  /* [in] */
  const IID*  rgiidExclude, /* [size_is][unique][in] */
  SNB         snbExclude,   /* [unique][in] */
  IStorage*   pstgDest)     /* [unique][in] */
{
  StorageBaseImpl *This = impl_from_IStorage(iface);

  BOOL         skip_storage = FALSE, skip_stream = FALSE;
  DWORD        i;

  TRACE("%p, %ld, %p, %p, %p.\n", iface, ciidExclude, rgiidExclude, snbExclude, pstgDest);

  if ( pstgDest == 0 )
    return STG_E_INVALIDPOINTER;

  for(i = 0; i < ciidExclude; ++i)
  {
    if(IsEqualGUID(&IID_IStorage, &rgiidExclude[i]))
        skip_storage = TRUE;
    else if(IsEqualGUID(&IID_IStream, &rgiidExclude[i]))
        skip_stream = TRUE;
    else
        WARN("Unknown excluded GUID: %s\n", debugstr_guid(&rgiidExclude[i]));
  }

  if (!skip_storage)
  {
    /* Give up early if it looks like this would be infinitely recursive.
     * Oddly enough, this includes some cases that aren't really recursive, like
     * copying to a transacted child. */
    IStorage *pstgDestAncestor = pstgDest;
    IStorage *pstgDestAncestorChild = NULL;

    /* Go up the chain from the destination until we find the source storage. */
    while (pstgDestAncestor != iface) {
      pstgDestAncestorChild = pstgDest;

      if (pstgDestAncestor->lpVtbl == &TransactedSnapshotImpl_Vtbl)
      {
        TransactedSnapshotImpl *snapshot = (TransactedSnapshotImpl*) pstgDestAncestor;

        pstgDestAncestor = &snapshot->transactedParent->IStorage_iface;
      }
      else if (pstgDestAncestor->lpVtbl == &StorageInternalImpl_Vtbl)
      {
        StorageInternalImpl *internal = (StorageInternalImpl*) pstgDestAncestor;

        pstgDestAncestor = &internal->parentStorage->IStorage_iface;
      }
      else
        break;
    }

    if (pstgDestAncestor == iface)
    {
      BOOL fail = TRUE;

      if (pstgDestAncestorChild && snbExclude)
      {
        StorageBaseImpl *ancestorChildBase = (StorageBaseImpl*)pstgDestAncestorChild;
        DirEntry data;
        WCHAR **snb = snbExclude;

        StorageBaseImpl_ReadDirEntry(ancestorChildBase, ancestorChildBase->storageDirEntry, &data);

        while ( *snb != NULL && fail )
        {
          if ( wcscmp(data.name, *snb) == 0 )
            fail = FALSE;
          ++snb;
        }
      }

      if (fail)
        return STG_E_ACCESSDENIED;
    }
  }

  return StorageBaseImpl_CopyStorageEntryTo( This, This->storageDirEntry,
    skip_storage, skip_stream, snbExclude, pstgDest );
}

/*************************************************************************
 * MoveElementTo (IStorage)
 */
static HRESULT WINAPI StorageBaseImpl_MoveElementTo(
  IStorage*     iface,
  const OLECHAR *pwcsName,   /* [string][in] */
  IStorage      *pstgDest,   /* [unique][in] */
  const OLECHAR *pwcsNewName,/* [string][in] */
  DWORD           grfFlags)    /* [in] */
{
  FIXME("%p, %s, %p, %s, %#lx: stub\n", iface, debugstr_w(pwcsName), pstgDest,
      debugstr_w(pwcsNewName), grfFlags);
  return E_NOTIMPL;
}

/*************************************************************************
 * Commit (IStorage)
 *
 * Ensures that any changes made to a storage object open in transacted mode
 * are reflected in the parent storage
 *
 * In a non-transacted mode, this ensures all cached writes are completed.
 */
static HRESULT WINAPI StorageBaseImpl_Commit(
  IStorage*   iface,
  DWORD         grfCommitFlags)/* [in] */
{
  StorageBaseImpl* This = impl_from_IStorage(iface);
  TRACE("%p, %#lx.\n", iface, grfCommitFlags);
  return StorageBaseImpl_Flush(This);
}

/*************************************************************************
 * Revert (IStorage)
 *
 * Discard all changes that have been made since the last commit operation
 */
static HRESULT WINAPI StorageBaseImpl_Revert(
  IStorage* iface)
{
  TRACE("(%p)\n", iface);
  return S_OK;
}

/*********************************************************************
 *
 * Internal helper function for StorageBaseImpl_DestroyElement()
 *
 * Delete the contents of a storage entry.
 *
 */
static HRESULT deleteStorageContents(
  StorageBaseImpl *parentStorage,
  DirRef       indexToDelete,
  DirEntry     entryDataToDelete)
{
  IEnumSTATSTG *elements     = 0;
  IStorage   *childStorage = 0;
  STATSTG      currentElement;
  HRESULT      hr;
  HRESULT      destroyHr = S_OK;
  StorageInternalImpl *stg, *stg2;

  TRACE("%p, %ld.\n", parentStorage, indexToDelete);

  /* Invalidate any open storage objects. */
  LIST_FOR_EACH_ENTRY_SAFE(stg, stg2, &parentStorage->storageHead, StorageInternalImpl, ParentListEntry)
  {
    if (stg->base.storageDirEntry == indexToDelete)
    {
      StorageBaseImpl_Invalidate(&stg->base);
    }
  }

  /*
   * Open the storage and enumerate it
   */
  hr = IStorage_OpenStorage(
        &parentStorage->IStorage_iface,
        entryDataToDelete.name,
        0,
        STGM_WRITE | STGM_SHARE_EXCLUSIVE,
        0,
        0,
        &childStorage);

  if (hr != S_OK)
  {
    TRACE("<-- %#lx\n", hr);
    return hr;
  }

  /*
   * Enumerate the elements
   */
  hr = IStorage_EnumElements(childStorage, 0, 0, 0, &elements);
  if (FAILED(hr))
  {
    IStorage_Release(childStorage);
    TRACE("<-- %#lx\n", hr);
    return hr;
  }

  do
  {
    /*
     * Obtain the next element
     */
    hr = IEnumSTATSTG_Next(elements, 1, &currentElement, NULL);
    if (hr==S_OK)
    {
      destroyHr = IStorage_DestroyElement(childStorage, currentElement.pwcsName);

      CoTaskMemFree(currentElement.pwcsName);
    }

    /*
     * We need to Reset the enumeration every time because we delete elements
     * and the enumeration could be invalid
     */
    IEnumSTATSTG_Reset(elements);

  } while ((hr == S_OK) && (destroyHr == S_OK));

  IStorage_Release(childStorage);
  IEnumSTATSTG_Release(elements);

  TRACE("%#lx\n", hr);
  return destroyHr;
}

/*********************************************************************
 *
 * Internal helper function for StorageBaseImpl_DestroyElement()
 *
 * Perform the deletion of a stream's data
 *
 */
static HRESULT deleteStreamContents(
  StorageBaseImpl *parentStorage,
  DirRef        indexToDelete,
  DirEntry      entryDataToDelete)
{
  IStream      *pis;
  HRESULT        hr;
  ULARGE_INTEGER size;
  StgStreamImpl *strm, *strm2;

  /* Invalidate any open stream objects. */
  LIST_FOR_EACH_ENTRY_SAFE(strm, strm2, &parentStorage->strmHead, StgStreamImpl, StrmListEntry)
  {
    if (strm->dirEntry == indexToDelete)
    {
      TRACE("Stream deleted %p\n", strm);
      strm->parentStorage = NULL;
      list_remove(&strm->StrmListEntry);
    }
  }

  size.HighPart = 0;
  size.LowPart = 0;

  hr = IStorage_OpenStream(&parentStorage->IStorage_iface,
        entryDataToDelete.name, NULL, STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, &pis);

  if (hr!=S_OK)
  {
    TRACE("<-- %#lx\n", hr);
    return(hr);
  }

  /*
   * Zap the stream
   */
  hr = IStream_SetSize(pis, size);

  if(hr != S_OK)
  {
    TRACE("<-- %#lx\n", hr);
    return hr;
  }

  /*
   * Release the stream object.
   */
  IStream_Release(pis);
  TRACE("<-- %#lx\n", hr);
  return S_OK;
}

/*************************************************************************
 * DestroyElement (IStorage)
 *
 * Strategy: This implementation is built this way for simplicity not for speed.
 *          I always delete the topmost element of the enumeration and adjust
 *          the deleted element pointer all the time.  This takes longer to
 *          do but allows reinvoking DestroyElement whenever we encounter a
 *          storage object.  The optimisation resides in the usage of another
 *          enumeration strategy that would give all the leaves of a storage
 *          first. (postfix order)
 */
static HRESULT WINAPI StorageBaseImpl_DestroyElement(
  IStorage*     iface,
  const OLECHAR *pwcsName)/* [string][in] */
{
  StorageBaseImpl *This = impl_from_IStorage(iface);

  HRESULT           hr = S_OK;
  DirEntry          entryToDelete;
  DirRef            entryToDeleteRef;

  TRACE("(%p, %s)\n",
	iface, debugstr_w(pwcsName));

  if (pwcsName==NULL)
    return STG_E_INVALIDPOINTER;

  if (This->reverted)
    return STG_E_REVERTED;

  if ( !(This->openFlags & STGM_TRANSACTED) &&
       STGM_ACCESS_MODE( This->openFlags ) == STGM_READ )
    return STG_E_ACCESSDENIED;

  entryToDeleteRef = findElement(
    This,
    This->storageDirEntry,
    pwcsName,
    &entryToDelete);

  if ( entryToDeleteRef == DIRENTRY_NULL )
  {
    TRACE("<-- STG_E_FILENOTFOUND\n");
    return STG_E_FILENOTFOUND;
  }

  if ( entryToDelete.stgType == STGTY_STORAGE )
  {
    hr = deleteStorageContents(
           This,
           entryToDeleteRef,
           entryToDelete);
  }
  else if ( entryToDelete.stgType == STGTY_STREAM )
  {
    hr = deleteStreamContents(
           This,
           entryToDeleteRef,
           entryToDelete);
  }

  if (hr!=S_OK)
  {
    TRACE("<-- %#lx\n", hr);
    return hr;
  }

  /*
   * Remove the entry from its parent storage
   */
  hr = removeFromTree(
        This,
        This->storageDirEntry,
        entryToDeleteRef);

  /*
   * Invalidate the entry
   */
  if (SUCCEEDED(hr))
    StorageBaseImpl_DestroyDirEntry(This, entryToDeleteRef);

  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_Flush(This);

  TRACE("<-- %#lx\n", hr);
  return hr;
}

static void StorageBaseImpl_DeleteAll(StorageBaseImpl * stg)
{
  struct list *cur, *cur2;
  StgStreamImpl *strm=NULL;
  StorageInternalImpl *childstg=NULL;

  LIST_FOR_EACH_SAFE(cur, cur2, &stg->strmHead) {
    strm = LIST_ENTRY(cur,StgStreamImpl,StrmListEntry);
    TRACE("Streams invalidated (stg=%p strm=%p next=%p prev=%p)\n", stg,strm,cur->next,cur->prev);
    strm->parentStorage = NULL;
    list_remove(cur);
  }

  LIST_FOR_EACH_SAFE(cur, cur2, &stg->storageHead) {
    childstg = LIST_ENTRY(cur,StorageInternalImpl,ParentListEntry);
    StorageBaseImpl_Invalidate( &childstg->base );
  }

  if (stg->transactedChild)
  {
    StorageBaseImpl_Invalidate(stg->transactedChild);

    stg->transactedChild = NULL;
  }
}

/******************************************************************************
 * SetElementTimes (IStorage)
 */
static HRESULT WINAPI StorageBaseImpl_SetElementTimes(
  IStorage*     iface,
  const OLECHAR *pwcsName,/* [string][in] */
  const FILETIME  *pctime,  /* [in] */
  const FILETIME  *patime,  /* [in] */
  const FILETIME  *pmtime)  /* [in] */
{
  FIXME("(%s,...), stub!\n",debugstr_w(pwcsName));
  return S_OK;
}

/******************************************************************************
 * SetStateBits (IStorage)
 */
static HRESULT WINAPI StorageBaseImpl_SetStateBits(
  IStorage*   iface,
  DWORD         grfStateBits,/* [in] */
  DWORD         grfMask)     /* [in] */
{
  StorageBaseImpl *This = impl_from_IStorage(iface);

  if (This->reverted)
    return STG_E_REVERTED;

  This->stateBits = (This->stateBits & ~grfMask) | (grfStateBits & grfMask);
  return S_OK;
}

/******************************************************************************
 * Internal stream list handlers
 */

void StorageBaseImpl_AddStream(StorageBaseImpl * stg, StgStreamImpl * strm)
{
  TRACE("Stream added (stg=%p strm=%p)\n", stg, strm);
  list_add_tail(&stg->strmHead,&strm->StrmListEntry);
}

void StorageBaseImpl_RemoveStream(StorageBaseImpl * stg, StgStreamImpl * strm)
{
  TRACE("Stream removed (stg=%p strm=%p)\n", stg,strm);
  list_remove(&(strm->StrmListEntry));
}

static HRESULT StorageBaseImpl_CopyStream(
  StorageBaseImpl *dst, DirRef dst_entry,
  StorageBaseImpl *src, DirRef src_entry)
{
  HRESULT hr;
  BYTE data[4096];
  DirEntry srcdata;
  ULARGE_INTEGER bytes_copied;
  ULONG bytestocopy, bytesread, byteswritten;

  hr = StorageBaseImpl_ReadDirEntry(src, src_entry, &srcdata);

  if (SUCCEEDED(hr))
  {
    hr = StorageBaseImpl_StreamSetSize(dst, dst_entry, srcdata.size);

    bytes_copied.QuadPart = 0;
    while (bytes_copied.QuadPart < srcdata.size.QuadPart && SUCCEEDED(hr))
    {
      bytestocopy = min(4096, srcdata.size.QuadPart - bytes_copied.QuadPart);

      hr = StorageBaseImpl_StreamReadAt(src, src_entry, bytes_copied, bytestocopy,
        data, &bytesread);
      if (SUCCEEDED(hr) && bytesread != bytestocopy) hr = STG_E_READFAULT;

      if (SUCCEEDED(hr))
        hr = StorageBaseImpl_StreamWriteAt(dst, dst_entry, bytes_copied, bytestocopy,
          data, &byteswritten);
      if (SUCCEEDED(hr))
      {
        if (byteswritten != bytestocopy) hr = STG_E_WRITEFAULT;
        bytes_copied.QuadPart += byteswritten;
      }
    }
  }

  return hr;
}

static HRESULT StorageBaseImpl_DupStorageTree(
  StorageBaseImpl *dst, DirRef *dst_entry,
  StorageBaseImpl *src, DirRef src_entry)
{
  HRESULT hr;
  DirEntry data;
  BOOL has_stream=FALSE;

  if (src_entry == DIRENTRY_NULL)
  {
    *dst_entry = DIRENTRY_NULL;
    return S_OK;
  }

  hr = StorageBaseImpl_ReadDirEntry(src, src_entry, &data);
  if (SUCCEEDED(hr))
  {
    has_stream = (data.stgType == STGTY_STREAM && data.size.QuadPart != 0);
    data.startingBlock = BLOCK_END_OF_CHAIN;
    data.size.QuadPart = 0;

    hr = StorageBaseImpl_DupStorageTree(dst, &data.leftChild, src, data.leftChild);
  }

  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_DupStorageTree(dst, &data.rightChild, src, data.rightChild);

  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_DupStorageTree(dst, &data.dirRootEntry, src, data.dirRootEntry);

  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_CreateDirEntry(dst, &data, dst_entry);

  if (SUCCEEDED(hr) && has_stream)
    hr = StorageBaseImpl_CopyStream(dst, *dst_entry, src, src_entry);

  return hr;
}

static HRESULT StorageBaseImpl_CopyStorageTree(
  StorageBaseImpl *dst, DirRef dst_entry,
  StorageBaseImpl *src, DirRef src_entry)
{
  HRESULT hr;
  DirEntry src_data, dst_data;
  DirRef new_root_entry;

  hr = StorageBaseImpl_ReadDirEntry(src, src_entry, &src_data);

  if (SUCCEEDED(hr))
  {
    hr = StorageBaseImpl_DupStorageTree(dst, &new_root_entry, src, src_data.dirRootEntry);
  }

  if (SUCCEEDED(hr))
  {
    hr = StorageBaseImpl_ReadDirEntry(dst, dst_entry, &dst_data);
    dst_data.clsid = src_data.clsid;
    dst_data.ctime = src_data.ctime;
    dst_data.mtime = src_data.mtime;
    dst_data.dirRootEntry = new_root_entry;
  }

  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_WriteDirEntry(dst, dst_entry, &dst_data);

  return hr;
}

static HRESULT StorageBaseImpl_DeleteStorageTree(StorageBaseImpl *This, DirRef entry, BOOL include_siblings)
{
  HRESULT hr;
  DirEntry data;
  ULARGE_INTEGER zero;

  if (entry == DIRENTRY_NULL)
    return S_OK;

  zero.QuadPart = 0;

  hr = StorageBaseImpl_ReadDirEntry(This, entry, &data);

  if (SUCCEEDED(hr) && include_siblings)
    hr = StorageBaseImpl_DeleteStorageTree(This, data.leftChild, TRUE);

  if (SUCCEEDED(hr) && include_siblings)
    hr = StorageBaseImpl_DeleteStorageTree(This, data.rightChild, TRUE);

  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_DeleteStorageTree(This, data.dirRootEntry, TRUE);

  if (SUCCEEDED(hr) && data.stgType == STGTY_STREAM)
    hr = StorageBaseImpl_StreamSetSize(This, entry, zero);

  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_DestroyDirEntry(This, entry);

  return hr;
}


/************************************************************************
 * StorageImpl implementation
 ***********************************************************************/

static HRESULT StorageImpl_ReadAt(StorageImpl* This,
  ULARGE_INTEGER offset,
  void*          buffer,
  ULONG          size,
  ULONG*         bytesRead)
{
    return ILockBytes_ReadAt(This->lockBytes,offset,buffer,size,bytesRead);
}

static HRESULT StorageImpl_WriteAt(StorageImpl* This,
  ULARGE_INTEGER offset,
  const void*    buffer,
  const ULONG    size,
  ULONG*         bytesWritten)
{
    return ILockBytes_WriteAt(This->lockBytes,offset,buffer,size,bytesWritten);
}

/******************************************************************************
 *      StorageImpl_LoadFileHeader
 *
 * This method will read in the file header
 */
static HRESULT StorageImpl_LoadFileHeader(
          StorageImpl* This)
{
  HRESULT hr;
  BYTE    headerBigBlock[HEADER_SIZE];
  int     index;
  ULARGE_INTEGER offset;
  DWORD bytes_read;

  TRACE("\n");
  /*
   * Get a pointer to the big block of data containing the header.
   */
  offset.HighPart = 0;
  offset.LowPart = 0;
  hr = StorageImpl_ReadAt(This, offset, headerBigBlock, HEADER_SIZE, &bytes_read);
  if (SUCCEEDED(hr) && bytes_read != HEADER_SIZE)
    hr = STG_E_FILENOTFOUND;

  /*
   * Extract the information from the header.
   */
  if (SUCCEEDED(hr))
  {
    /*
     * Check for the "magic number" signature and return an error if it is not
     * found.
     */
    if (memcmp(headerBigBlock, STORAGE_oldmagic, sizeof(STORAGE_oldmagic))==0)
    {
      return STG_E_OLDFORMAT;
    }

    if (memcmp(headerBigBlock, STORAGE_magic, sizeof(STORAGE_magic))!=0)
    {
      return STG_E_INVALIDHEADER;
    }

    StorageUtl_ReadWord(
      headerBigBlock,
      OFFSET_BIGBLOCKSIZEBITS,
      &This->bigBlockSizeBits);

    StorageUtl_ReadWord(
      headerBigBlock,
      OFFSET_SMALLBLOCKSIZEBITS,
      &This->smallBlockSizeBits);

    StorageUtl_ReadDWord(
      headerBigBlock,
      OFFSET_BBDEPOTCOUNT,
      &This->bigBlockDepotCount);

    StorageUtl_ReadDWord(
      headerBigBlock,
      OFFSET_ROOTSTARTBLOCK,
      &This->rootStartBlock);

    StorageUtl_ReadDWord(
      headerBigBlock,
      OFFSET_TRANSACTIONSIG,
      &This->transactionSig);

    StorageUtl_ReadDWord(
      headerBigBlock,
      OFFSET_SMALLBLOCKLIMIT,
      &This->smallBlockLimit);

    StorageUtl_ReadDWord(
      headerBigBlock,
      OFFSET_SBDEPOTSTART,
      &This->smallBlockDepotStart);

    StorageUtl_ReadDWord(
      headerBigBlock,
      OFFSET_EXTBBDEPOTSTART,
      &This->extBigBlockDepotStart);

    StorageUtl_ReadDWord(
      headerBigBlock,
      OFFSET_EXTBBDEPOTCOUNT,
      &This->extBigBlockDepotCount);

    for (index = 0; index < COUNT_BBDEPOTINHEADER; index ++)
    {
      StorageUtl_ReadDWord(
        headerBigBlock,
        OFFSET_BBDEPOTSTART + (sizeof(ULONG)*index),
        &(This->bigBlockDepotStart[index]));
    }

    /*
     * Make the bitwise arithmetic to get the size of the blocks in bytes.
     */
    This->bigBlockSize   = 0x000000001 << (DWORD)This->bigBlockSizeBits;
    This->smallBlockSize = 0x000000001 << (DWORD)This->smallBlockSizeBits;

    /*
     * Right now, the code is making some assumptions about the size of the
     * blocks, just make sure they are what we're expecting.
     */
    if ((This->bigBlockSize != MIN_BIG_BLOCK_SIZE && This->bigBlockSize != MAX_BIG_BLOCK_SIZE) ||
	This->smallBlockSize != DEF_SMALL_BLOCK_SIZE ||
	This->smallBlockLimit != LIMIT_TO_USE_SMALL_BLOCK)
    {
	FIXME("Broken OLE storage file? bigblock=%#lx, smallblock=%#lx, sblimit=%#lx\n",
	    This->bigBlockSize, This->smallBlockSize, This->smallBlockLimit);
	hr = STG_E_INVALIDHEADER;
    }
    else
	hr = S_OK;
  }

  return hr;
}

/******************************************************************************
 *      StorageImpl_SaveFileHeader
 *
 * This method will save to the file the header
 */
static void StorageImpl_SaveFileHeader(
          StorageImpl* This)
{
  BYTE   headerBigBlock[HEADER_SIZE];
  int    index;
  ULARGE_INTEGER offset;
  DWORD bytes_written;
  DWORD major_version, dirsectorcount;

  if (This->bigBlockSizeBits == 0x9)
    major_version = 3;
  else if (This->bigBlockSizeBits == 0xc)
    major_version = 4;
  else
  {
    ERR("invalid big block shift 0x%x\n", This->bigBlockSizeBits);
    major_version = 4;
  }

  memset(headerBigBlock, 0, HEADER_SIZE);
  memcpy(headerBigBlock, STORAGE_magic, sizeof(STORAGE_magic));

  /*
   * Write the information to the header.
   */
  StorageUtl_WriteWord(
    headerBigBlock,
    OFFSET_MINORVERSION,
    0x3e);

  StorageUtl_WriteWord(
    headerBigBlock,
    OFFSET_MAJORVERSION,
    major_version);

  StorageUtl_WriteWord(
    headerBigBlock,
    OFFSET_BYTEORDERMARKER,
    (WORD)-2);

  StorageUtl_WriteWord(
    headerBigBlock,
    OFFSET_BIGBLOCKSIZEBITS,
    This->bigBlockSizeBits);

  StorageUtl_WriteWord(
    headerBigBlock,
    OFFSET_SMALLBLOCKSIZEBITS,
    This->smallBlockSizeBits);

  if (major_version >= 4)
  {
    if (This->rootBlockChain)
      dirsectorcount = BlockChainStream_GetCount(This->rootBlockChain);
    else
      /* This file is being created, and it will start out with one block. */
      dirsectorcount = 1;
  }
  else
    /* This field must be 0 in versions older than 4 */
    dirsectorcount = 0;

  StorageUtl_WriteDWord(
    headerBigBlock,
    OFFSET_DIRSECTORCOUNT,
    dirsectorcount);

  StorageUtl_WriteDWord(
    headerBigBlock,
    OFFSET_BBDEPOTCOUNT,
    This->bigBlockDepotCount);

  StorageUtl_WriteDWord(
    headerBigBlock,
    OFFSET_ROOTSTARTBLOCK,
    This->rootStartBlock);

  StorageUtl_WriteDWord(
    headerBigBlock,
    OFFSET_TRANSACTIONSIG,
    This->transactionSig);

  StorageUtl_WriteDWord(
    headerBigBlock,
    OFFSET_SMALLBLOCKLIMIT,
    This->smallBlockLimit);

  StorageUtl_WriteDWord(
    headerBigBlock,
    OFFSET_SBDEPOTSTART,
    This->smallBlockDepotStart);

  StorageUtl_WriteDWord(
    headerBigBlock,
    OFFSET_SBDEPOTCOUNT,
    This->smallBlockDepotChain ?
     BlockChainStream_GetCount(This->smallBlockDepotChain) : 0);

  StorageUtl_WriteDWord(
    headerBigBlock,
    OFFSET_EXTBBDEPOTSTART,
    This->extBigBlockDepotStart);

  StorageUtl_WriteDWord(
    headerBigBlock,
    OFFSET_EXTBBDEPOTCOUNT,
    This->extBigBlockDepotCount);

  for (index = 0; index < COUNT_BBDEPOTINHEADER; index ++)
  {
    StorageUtl_WriteDWord(
      headerBigBlock,
      OFFSET_BBDEPOTSTART + (sizeof(ULONG)*index),
      (This->bigBlockDepotStart[index]));
  }

  offset.QuadPart = 0;
  StorageImpl_WriteAt(This, offset, headerBigBlock, HEADER_SIZE, &bytes_written);
}


/************************************************************************
 * StorageImpl implementation : DirEntry methods
 ***********************************************************************/

/******************************************************************************
 *      StorageImpl_ReadRawDirEntry
 *
 * This method will read the raw data from a directory entry in the file.
 *
 * buffer must be RAW_DIRENTRY_SIZE bytes long.
 */
static HRESULT StorageImpl_ReadRawDirEntry(StorageImpl *This, ULONG index, BYTE *buffer)
{
  ULARGE_INTEGER offset;
  HRESULT hr;
  ULONG bytesRead;

  offset.QuadPart  = (ULONGLONG)index * RAW_DIRENTRY_SIZE;

  hr = BlockChainStream_ReadAt(
                    This->rootBlockChain,
                    offset,
                    RAW_DIRENTRY_SIZE,
                    buffer,
                    &bytesRead);

  if (bytesRead != RAW_DIRENTRY_SIZE)
    return STG_E_READFAULT;

  return hr;
}

/******************************************************************************
 *      StorageImpl_WriteRawDirEntry
 *
 * This method will write the raw data from a directory entry in the file.
 *
 * buffer must be RAW_DIRENTRY_SIZE bytes long.
 */
static HRESULT StorageImpl_WriteRawDirEntry(StorageImpl *This, ULONG index, const BYTE *buffer)
{
  ULARGE_INTEGER offset;
  ULONG bytesRead;

  offset.QuadPart  = (ULONGLONG)index * RAW_DIRENTRY_SIZE;

  return BlockChainStream_WriteAt(
                    This->rootBlockChain,
                    offset,
                    RAW_DIRENTRY_SIZE,
                    buffer,
                    &bytesRead);
}

/***************************************************************************
 *
 * Internal Method
 *
 * Mark a directory entry in the file as free.
 */
static HRESULT StorageImpl_DestroyDirEntry(
  StorageBaseImpl *base,
  DirRef index)
{
  BYTE emptyData[RAW_DIRENTRY_SIZE];
  StorageImpl *storage = (StorageImpl*)base;

  memset(emptyData, 0, RAW_DIRENTRY_SIZE);

  return StorageImpl_WriteRawDirEntry(storage, index, emptyData);
}

/******************************************************************************
 *      UpdateRawDirEntry
 *
 * Update raw directory entry data from the fields in newData.
 *
 * buffer must be RAW_DIRENTRY_SIZE bytes long.
 */
static void UpdateRawDirEntry(BYTE *buffer, const DirEntry *newData)
{
  memset(buffer, 0, RAW_DIRENTRY_SIZE);

  memcpy(
    buffer + OFFSET_PS_NAME,
    newData->name,
    DIRENTRY_NAME_BUFFER_LEN );

  memcpy(buffer + OFFSET_PS_STGTYPE, &newData->stgType, 1);

  StorageUtl_WriteWord(
    buffer,
      OFFSET_PS_NAMELENGTH,
      newData->sizeOfNameString);

  StorageUtl_WriteDWord(
    buffer,
      OFFSET_PS_LEFTCHILD,
      newData->leftChild);

  StorageUtl_WriteDWord(
    buffer,
      OFFSET_PS_RIGHTCHILD,
      newData->rightChild);

  StorageUtl_WriteDWord(
    buffer,
      OFFSET_PS_DIRROOT,
      newData->dirRootEntry);

  StorageUtl_WriteGUID(
    buffer,
      OFFSET_PS_GUID,
      &newData->clsid);

  StorageUtl_WriteDWord(
    buffer,
      OFFSET_PS_CTIMELOW,
      newData->ctime.dwLowDateTime);

  StorageUtl_WriteDWord(
    buffer,
      OFFSET_PS_CTIMEHIGH,
      newData->ctime.dwHighDateTime);

  StorageUtl_WriteDWord(
    buffer,
      OFFSET_PS_MTIMELOW,
      newData->mtime.dwLowDateTime);

  StorageUtl_WriteDWord(
    buffer,
      OFFSET_PS_MTIMEHIGH,
      newData->mtime.dwHighDateTime);

  StorageUtl_WriteDWord(
    buffer,
      OFFSET_PS_STARTBLOCK,
      newData->startingBlock);

  StorageUtl_WriteDWord(
    buffer,
      OFFSET_PS_SIZE,
      newData->size.LowPart);

  StorageUtl_WriteDWord(
    buffer,
      OFFSET_PS_SIZE_HIGH,
      newData->size.HighPart);
}

/***************************************************************************
 *
 * Internal Method
 *
 * Reserve a directory entry in the file and initialize it.
 */
static HRESULT StorageImpl_CreateDirEntry(
  StorageBaseImpl *base,
  const DirEntry *newData,
  DirRef *index)
{
  StorageImpl *storage = (StorageImpl*)base;
  ULONG       currentEntryIndex    = 0;
  ULONG       newEntryIndex        = DIRENTRY_NULL;
  HRESULT hr = S_OK;
  BYTE currentData[RAW_DIRENTRY_SIZE];
  WORD sizeOfNameString;

  do
  {
    hr = StorageImpl_ReadRawDirEntry(storage,
                                     currentEntryIndex,
                                     currentData);

    if (SUCCEEDED(hr))
    {
      StorageUtl_ReadWord(
        currentData,
        OFFSET_PS_NAMELENGTH,
        &sizeOfNameString);

      if (sizeOfNameString == 0)
      {
        /*
         * The entry exists and is available, we found it.
         */
        newEntryIndex = currentEntryIndex;
      }
    }
    else
    {
      /*
       * We exhausted the directory entries, we will create more space below
       */
      newEntryIndex = currentEntryIndex;
    }
    currentEntryIndex++;

  } while (newEntryIndex == DIRENTRY_NULL);

  /*
   * grow the directory stream
   */
  if (FAILED(hr))
  {
    BYTE           emptyData[RAW_DIRENTRY_SIZE];
    ULARGE_INTEGER newSize;
    ULONG          entryIndex;
    ULONG          lastEntry     = 0;
    ULONG          blockCount    = 0;

    /*
     * obtain the new count of blocks in the directory stream
     */
    blockCount = BlockChainStream_GetCount(
                   storage->rootBlockChain)+1;

    /*
     * initialize the size used by the directory stream
     */
    newSize.QuadPart  = (ULONGLONG)storage->bigBlockSize * blockCount;

    /*
     * add a block to the directory stream
     */
    BlockChainStream_SetSize(storage->rootBlockChain, newSize);

    /*
     * memset the empty entry in order to initialize the unused newly
     * created entries
     */
    memset(emptyData, 0, RAW_DIRENTRY_SIZE);

    /*
     * initialize them
     */
    lastEntry = storage->bigBlockSize / RAW_DIRENTRY_SIZE * blockCount;

    for(
      entryIndex = newEntryIndex + 1;
      entryIndex < lastEntry;
      entryIndex++)
    {
      StorageImpl_WriteRawDirEntry(
        storage,
        entryIndex,
        emptyData);
    }

    StorageImpl_SaveFileHeader(storage);
  }

  UpdateRawDirEntry(currentData, newData);

  hr = StorageImpl_WriteRawDirEntry(storage, newEntryIndex, currentData);

  if (SUCCEEDED(hr))
    *index = newEntryIndex;

  return hr;
}

/******************************************************************************
 *      StorageImpl_ReadDirEntry
 *
 * This method will read the specified directory entry.
 */
static HRESULT StorageImpl_ReadDirEntry(
  StorageImpl* This,
  DirRef         index,
  DirEntry*      buffer)
{
  BYTE           currentEntry[RAW_DIRENTRY_SIZE];
  HRESULT        readRes;

  readRes = StorageImpl_ReadRawDirEntry(This, index, currentEntry);

  if (SUCCEEDED(readRes))
  {
    memset(buffer->name, 0, sizeof(buffer->name));
    memcpy(
      buffer->name,
      (WCHAR *)currentEntry+OFFSET_PS_NAME,
      DIRENTRY_NAME_BUFFER_LEN );
    TRACE("storage name: %s\n", debugstr_w(buffer->name));

    memcpy(&buffer->stgType, currentEntry + OFFSET_PS_STGTYPE, 1);

    StorageUtl_ReadWord(
      currentEntry,
      OFFSET_PS_NAMELENGTH,
      &buffer->sizeOfNameString);

    StorageUtl_ReadDWord(
      currentEntry,
      OFFSET_PS_LEFTCHILD,
      &buffer->leftChild);

    StorageUtl_ReadDWord(
      currentEntry,
      OFFSET_PS_RIGHTCHILD,
      &buffer->rightChild);

    StorageUtl_ReadDWord(
      currentEntry,
      OFFSET_PS_DIRROOT,
      &buffer->dirRootEntry);

    StorageUtl_ReadGUID(
      currentEntry,
      OFFSET_PS_GUID,
      &buffer->clsid);

    StorageUtl_ReadDWord(
      currentEntry,
      OFFSET_PS_CTIMELOW,
      &buffer->ctime.dwLowDateTime);

    StorageUtl_ReadDWord(
      currentEntry,
      OFFSET_PS_CTIMEHIGH,
      &buffer->ctime.dwHighDateTime);

    StorageUtl_ReadDWord(
      currentEntry,
      OFFSET_PS_MTIMELOW,
      &buffer->mtime.dwLowDateTime);

    StorageUtl_ReadDWord(
      currentEntry,
      OFFSET_PS_MTIMEHIGH,
      &buffer->mtime.dwHighDateTime);

    StorageUtl_ReadDWord(
      currentEntry,
      OFFSET_PS_STARTBLOCK,
      &buffer->startingBlock);

    StorageUtl_ReadDWord(
      currentEntry,
      OFFSET_PS_SIZE,
      &buffer->size.LowPart);

    if (This->bigBlockSize < 4096)
    {
      /* Version 3 files may have junk in the high part of size. */
      buffer->size.HighPart = 0;
    }
    else
    {
      StorageUtl_ReadDWord(
        currentEntry,
        OFFSET_PS_SIZE_HIGH,
        &buffer->size.HighPart);
    }
  }

  return readRes;
}

/*********************************************************************
 * Write the specified directory entry to the file
 */
static HRESULT StorageImpl_WriteDirEntry(
  StorageImpl*          This,
  DirRef                index,
  const DirEntry*       buffer)
{
  BYTE currentEntry[RAW_DIRENTRY_SIZE];

  UpdateRawDirEntry(currentEntry, buffer);

  return StorageImpl_WriteRawDirEntry(This, index, currentEntry);
}


/************************************************************************
 * StorageImpl implementation : Block methods
 ***********************************************************************/

static ULONGLONG StorageImpl_GetBigBlockOffset(StorageImpl* This, ULONG index)
{
    return (ULONGLONG)(index+1) * This->bigBlockSize;
}

static HRESULT StorageImpl_ReadBigBlock(
  StorageImpl* This,
  ULONG          blockIndex,
  void*          buffer,
  ULONG*         out_read)
{
  ULARGE_INTEGER ulOffset;
  DWORD  read=0;
  HRESULT hr;

  ulOffset.QuadPart = StorageImpl_GetBigBlockOffset(This, blockIndex);

  hr = StorageImpl_ReadAt(This, ulOffset, buffer, This->bigBlockSize, &read);

  if (SUCCEEDED(hr) &&  read < This->bigBlockSize)
  {
    /* File ends during this block; fill the rest with 0's. */
    memset((LPBYTE)buffer+read, 0, This->bigBlockSize-read);
  }

  if (out_read) *out_read = read;

  return hr;
}

static BOOL StorageImpl_ReadDWordFromBigBlock(
  StorageImpl*  This,
  ULONG         blockIndex,
  ULONG         offset,
  DWORD*        value)
{
  ULARGE_INTEGER ulOffset;
  DWORD  read;
  DWORD  tmp;

  ulOffset.QuadPart = StorageImpl_GetBigBlockOffset(This, blockIndex);
  ulOffset.QuadPart += offset;

  StorageImpl_ReadAt(This, ulOffset, &tmp, sizeof(DWORD), &read);
  *value = lendian32toh(tmp);
  return (read == sizeof(DWORD));
}

static BOOL StorageImpl_WriteBigBlock(
  StorageImpl*  This,
  ULONG         blockIndex,
  const void*   buffer)
{
  ULARGE_INTEGER ulOffset;
  DWORD  wrote;

  ulOffset.QuadPart = StorageImpl_GetBigBlockOffset(This, blockIndex);

  StorageImpl_WriteAt(This, ulOffset, buffer, This->bigBlockSize, &wrote);
  return (wrote == This->bigBlockSize);
}

static BOOL StorageImpl_WriteDWordToBigBlock(
  StorageImpl* This,
  ULONG         blockIndex,
  ULONG         offset,
  DWORD         value)
{
  ULARGE_INTEGER ulOffset;
  DWORD  wrote;

  ulOffset.QuadPart = StorageImpl_GetBigBlockOffset(This, blockIndex);
  ulOffset.QuadPart += offset;

  value = htole32(value);
  StorageImpl_WriteAt(This, ulOffset, &value, sizeof(DWORD), &wrote);
  return (wrote == sizeof(DWORD));
}

/******************************************************************************
 *              Storage32Impl_SmallBlocksToBigBlocks
 *
 * This method will convert a small block chain to a big block chain.
 * The small block chain will be destroyed.
 */
static BlockChainStream* Storage32Impl_SmallBlocksToBigBlocks(
                      StorageImpl* This,
                      SmallBlockChainStream** ppsbChain)
{
  ULONG bbHeadOfChain = BLOCK_END_OF_CHAIN;
  ULARGE_INTEGER size, offset;
  ULONG cbRead, cbWritten;
  ULARGE_INTEGER cbTotalRead;
  DirRef streamEntryRef;
  HRESULT resWrite = S_OK;
  HRESULT resRead;
  DirEntry streamEntry;
  BYTE *buffer;
  BlockChainStream *bbTempChain = NULL;
  BlockChainStream *bigBlockChain = NULL;

  /*
   * Create a temporary big block chain that doesn't have
   * an associated directory entry. This temporary chain will be
   * used to copy data from small blocks to big blocks.
   */
  bbTempChain = BlockChainStream_Construct(This,
                                           &bbHeadOfChain,
                                           DIRENTRY_NULL);
  if(!bbTempChain) return NULL;
  /*
   * Grow the big block chain.
   */
  size = SmallBlockChainStream_GetSize(*ppsbChain);
  BlockChainStream_SetSize(bbTempChain, size);

  /*
   * Copy the contents of the small block chain to the big block chain
   * by small block size increments.
   */
  offset.LowPart = 0;
  offset.HighPart = 0;
  cbTotalRead.QuadPart = 0;

  buffer = HeapAlloc(GetProcessHeap(),0,DEF_SMALL_BLOCK_SIZE);
  do
  {
    resRead = SmallBlockChainStream_ReadAt(*ppsbChain,
                                           offset,
                                           min(This->smallBlockSize, size.LowPart - offset.LowPart),
                                           buffer,
                                           &cbRead);
    if (FAILED(resRead))
        break;

    if (cbRead > 0)
    {
        cbTotalRead.QuadPart += cbRead;

        resWrite = BlockChainStream_WriteAt(bbTempChain,
                                            offset,
                                            cbRead,
                                            buffer,
                                            &cbWritten);

        if (FAILED(resWrite))
            break;

        offset.LowPart += cbRead;
    }
    else
    {
        resRead = STG_E_READFAULT;
        break;
    }
  } while (cbTotalRead.QuadPart < size.QuadPart);
  HeapFree(GetProcessHeap(),0,buffer);

  size.HighPart = 0;
  size.LowPart  = 0;

  if (FAILED(resRead) || FAILED(resWrite))
  {
    ERR("conversion failed: resRead = %#lx, resWrite = %#lx\n", resRead, resWrite);
    BlockChainStream_SetSize(bbTempChain, size);
    BlockChainStream_Destroy(bbTempChain);
    return NULL;
  }

  /*
   * Destroy the small block chain.
   */
  streamEntryRef = (*ppsbChain)->ownerDirEntry;
  SmallBlockChainStream_SetSize(*ppsbChain, size);
  SmallBlockChainStream_Destroy(*ppsbChain);
  *ppsbChain = 0;

  /*
   * Change the directory entry. This chain is now a big block chain
   * and it doesn't reside in the small blocks chain anymore.
   */
  StorageImpl_ReadDirEntry(This, streamEntryRef, &streamEntry);

  streamEntry.startingBlock = bbHeadOfChain;

  StorageImpl_WriteDirEntry(This, streamEntryRef, &streamEntry);

  /*
   * Destroy the temporary entryless big block chain.
   * Create a new big block chain associated with this entry.
   */
  BlockChainStream_Destroy(bbTempChain);
  bigBlockChain = BlockChainStream_Construct(This,
                                             NULL,
                                             streamEntryRef);

  return bigBlockChain;
}

/******************************************************************************
 *              Storage32Impl_BigBlocksToSmallBlocks
 *
 * This method will convert a big block chain to a small block chain.
 * The big block chain will be destroyed on success.
 */
static SmallBlockChainStream* Storage32Impl_BigBlocksToSmallBlocks(
                           StorageImpl* This,
                           BlockChainStream** ppbbChain,
                           ULARGE_INTEGER newSize)
{
    ULARGE_INTEGER size, offset, cbTotalRead;
    ULONG cbRead, cbWritten, sbHeadOfChain = BLOCK_END_OF_CHAIN;
    DirRef streamEntryRef;
    HRESULT resWrite = S_OK, resRead = S_OK;
    DirEntry streamEntry;
    BYTE* buffer;
    SmallBlockChainStream* sbTempChain;

    TRACE("%p %p\n", This, ppbbChain);

    sbTempChain = SmallBlockChainStream_Construct(This, &sbHeadOfChain,
            DIRENTRY_NULL);

    if(!sbTempChain)
        return NULL;

    SmallBlockChainStream_SetSize(sbTempChain, newSize);
    size = BlockChainStream_GetSize(*ppbbChain);
    size.QuadPart = min(size.QuadPart, newSize.QuadPart);

    offset.HighPart = 0;
    offset.LowPart = 0;
    cbTotalRead.QuadPart = 0;
    buffer = HeapAlloc(GetProcessHeap(), 0, This->bigBlockSize);
    while(cbTotalRead.QuadPart < size.QuadPart)
    {
        resRead = BlockChainStream_ReadAt(*ppbbChain, offset,
                min(This->bigBlockSize, size.LowPart - offset.LowPart),
                buffer, &cbRead);

        if(FAILED(resRead))
            break;

        if(cbRead > 0)
        {
            cbTotalRead.QuadPart += cbRead;

            resWrite = SmallBlockChainStream_WriteAt(sbTempChain, offset,
                    cbRead, buffer, &cbWritten);

            if(FAILED(resWrite))
                break;

            offset.LowPart += cbRead;
        }
        else
        {
            resRead = STG_E_READFAULT;
            break;
        }
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    size.HighPart = 0;
    size.LowPart = 0;

    if(FAILED(resRead) || FAILED(resWrite))
    {
        ERR("conversion failed: resRead = %#lx, resWrite = %#lx\n", resRead, resWrite);
        SmallBlockChainStream_SetSize(sbTempChain, size);
        SmallBlockChainStream_Destroy(sbTempChain);
        return NULL;
    }

    /* destroy the original big block chain */
    streamEntryRef = (*ppbbChain)->ownerDirEntry;
    BlockChainStream_SetSize(*ppbbChain, size);
    BlockChainStream_Destroy(*ppbbChain);
    *ppbbChain = NULL;

    StorageImpl_ReadDirEntry(This, streamEntryRef, &streamEntry);
    streamEntry.startingBlock = sbHeadOfChain;
    StorageImpl_WriteDirEntry(This, streamEntryRef, &streamEntry);

    SmallBlockChainStream_Destroy(sbTempChain);
    return SmallBlockChainStream_Construct(This, NULL, streamEntryRef);
}

/******************************************************************************
 *      Storage32Impl_AddBlockDepot
 *
 * This will create a depot block, essentially it is a block initialized
 * to BLOCK_UNUSEDs.
 */
static void Storage32Impl_AddBlockDepot(StorageImpl* This, ULONG blockIndex, ULONG depotIndex)
{
  BYTE blockBuffer[MAX_BIG_BLOCK_SIZE];
  ULONG rangeLockIndex = RANGELOCK_FIRST / This->bigBlockSize - 1;
  ULONG blocksPerDepot = This->bigBlockSize / sizeof(ULONG);
  ULONG rangeLockDepot = rangeLockIndex / blocksPerDepot;

  /*
   * Initialize blocks as free
   */
  memset(blockBuffer, BLOCK_UNUSED, This->bigBlockSize);

  /* Reserve the range lock sector */
  if (depotIndex == rangeLockDepot)
  {
    ((ULONG*)blockBuffer)[rangeLockIndex % blocksPerDepot] = BLOCK_END_OF_CHAIN;
  }

  StorageImpl_WriteBigBlock(This, blockIndex, blockBuffer);
}

/******************************************************************************
 *      Storage32Impl_GetExtDepotBlock
 *
 * Returns the index of the block that corresponds to the specified depot
 * index. This method is only for depot indexes equal or greater than
 * COUNT_BBDEPOTINHEADER.
 */
static ULONG Storage32Impl_GetExtDepotBlock(StorageImpl* This, ULONG depotIndex)
{
  ULONG depotBlocksPerExtBlock = (This->bigBlockSize / sizeof(ULONG)) - 1;
  ULONG numExtBlocks           = depotIndex - COUNT_BBDEPOTINHEADER;
  ULONG extBlockCount          = numExtBlocks / depotBlocksPerExtBlock;
  ULONG extBlockOffset         = numExtBlocks % depotBlocksPerExtBlock;
  ULONG blockIndex             = BLOCK_UNUSED;
  ULONG extBlockIndex;
  BYTE depotBuffer[MAX_BIG_BLOCK_SIZE];
  int index, num_blocks;

  assert(depotIndex >= COUNT_BBDEPOTINHEADER);

  if (extBlockCount >= This->extBigBlockDepotCount)
    return BLOCK_UNUSED;

  if (This->indexExtBlockDepotCached != extBlockCount)
  {
    extBlockIndex = This->extBigBlockDepotLocations[extBlockCount];

    StorageImpl_ReadBigBlock(This, extBlockIndex, depotBuffer, NULL);

    num_blocks = This->bigBlockSize / 4;

    for (index = 0; index < num_blocks; index++)
    {
      StorageUtl_ReadDWord(depotBuffer, index*sizeof(ULONG), &blockIndex);
      This->extBlockDepotCached[index] = blockIndex;
    }

    This->indexExtBlockDepotCached = extBlockCount;
  }

  blockIndex = This->extBlockDepotCached[extBlockOffset];

  return blockIndex;
}

/******************************************************************************
 *      Storage32Impl_SetExtDepotBlock
 *
 * Associates the specified block index to the specified depot index.
 * This method is only for depot indexes equal or greater than
 * COUNT_BBDEPOTINHEADER.
 */
static void Storage32Impl_SetExtDepotBlock(StorageImpl* This, ULONG depotIndex, ULONG blockIndex)
{
  ULONG depotBlocksPerExtBlock = (This->bigBlockSize / sizeof(ULONG)) - 1;
  ULONG numExtBlocks           = depotIndex - COUNT_BBDEPOTINHEADER;
  ULONG extBlockCount          = numExtBlocks / depotBlocksPerExtBlock;
  ULONG extBlockOffset         = numExtBlocks % depotBlocksPerExtBlock;
  ULONG extBlockIndex;

  assert(depotIndex >= COUNT_BBDEPOTINHEADER);

  assert(extBlockCount < This->extBigBlockDepotCount);

  extBlockIndex = This->extBigBlockDepotLocations[extBlockCount];

  if (extBlockIndex != BLOCK_UNUSED)
  {
    StorageImpl_WriteDWordToBigBlock(This, extBlockIndex,
                        extBlockOffset * sizeof(ULONG),
                        blockIndex);
  }

  if (This->indexExtBlockDepotCached == extBlockCount)
  {
    This->extBlockDepotCached[extBlockOffset] = blockIndex;
  }
}

/******************************************************************************
 *      Storage32Impl_AddExtBlockDepot
 *
 * Creates an extended depot block.
 */
static ULONG Storage32Impl_AddExtBlockDepot(StorageImpl* This)
{
  ULONG numExtBlocks           = This->extBigBlockDepotCount;
  ULONG nextExtBlock           = This->extBigBlockDepotStart;
  BYTE  depotBuffer[MAX_BIG_BLOCK_SIZE];
  ULONG index                  = BLOCK_UNUSED;
  ULONG nextBlockOffset        = This->bigBlockSize - sizeof(ULONG);
  ULONG blocksPerDepotBlock    = This->bigBlockSize / sizeof(ULONG);
  ULONG depotBlocksPerExtBlock = blocksPerDepotBlock - 1;

  index = (COUNT_BBDEPOTINHEADER + (numExtBlocks * depotBlocksPerExtBlock)) *
          blocksPerDepotBlock;

  if ((numExtBlocks == 0) && (nextExtBlock == BLOCK_END_OF_CHAIN))
  {
    /*
     * The first extended block.
     */
    This->extBigBlockDepotStart = index;
  }
  else
  {
    /*
     * Find the last existing extended block.
     */
    nextExtBlock = This->extBigBlockDepotLocations[This->extBigBlockDepotCount-1];

    /*
     * Add the new extended block to the chain.
     */
    StorageImpl_WriteDWordToBigBlock(This, nextExtBlock, nextBlockOffset,
                                     index);
  }

  /*
   * Initialize this block.
   */
  memset(depotBuffer, BLOCK_UNUSED, This->bigBlockSize);
  StorageImpl_WriteBigBlock(This, index, depotBuffer);

  /* Add the block to our cache. */
  if (This->extBigBlockDepotLocationsSize == numExtBlocks)
  {
    ULONG new_cache_size = (This->extBigBlockDepotLocationsSize+1)*2;
    ULONG *new_cache = HeapAlloc(GetProcessHeap(), 0, sizeof(ULONG) * new_cache_size);

    memcpy(new_cache, This->extBigBlockDepotLocations, sizeof(ULONG) * This->extBigBlockDepotLocationsSize);
    HeapFree(GetProcessHeap(), 0, This->extBigBlockDepotLocations);

    This->extBigBlockDepotLocations = new_cache;
    This->extBigBlockDepotLocationsSize = new_cache_size;
  }
  This->extBigBlockDepotLocations[numExtBlocks] = index;

  return index;
}

/************************************************************************
 * StorageImpl_GetNextBlockInChain
 *
 * This method will retrieve the block index of the next big block in
 * in the chain.
 *
 * Params:  This       - Pointer to the Storage object.
 *          blockIndex - Index of the block to retrieve the chain
 *                       for.
 *          nextBlockIndex - receives the return value.
 *
 * Returns: This method returns the index of the next block in the chain.
 *          It will return the constants:
 *              BLOCK_SPECIAL - If the block given was not part of a
 *                              chain.
 *              BLOCK_END_OF_CHAIN - If the block given was the last in
 *                                   a chain.
 *              BLOCK_UNUSED - If the block given was not past of a chain
 *                             and is available.
 *              BLOCK_EXTBBDEPOT - This block is part of the extended
 *                                 big block depot.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT StorageImpl_GetNextBlockInChain(
  StorageImpl* This,
  ULONG        blockIndex,
  ULONG*       nextBlockIndex)
{
  ULONG offsetInDepot    = blockIndex * sizeof (ULONG);
  ULONG depotBlockCount  = offsetInDepot / This->bigBlockSize;
  ULONG depotBlockOffset = offsetInDepot % This->bigBlockSize;
  BYTE depotBuffer[MAX_BIG_BLOCK_SIZE];
  ULONG read;
  ULONG depotBlockIndexPos;
  int index, num_blocks;

  *nextBlockIndex   = BLOCK_SPECIAL;

  if(depotBlockCount >= This->bigBlockDepotCount)
  {
    WARN("depotBlockCount %ld, bigBlockDepotCount %ld\n", depotBlockCount, This->bigBlockDepotCount);
    return STG_E_READFAULT;
  }

  /*
   * Cache the currently accessed depot block.
   */
  if (depotBlockCount != This->indexBlockDepotCached)
  {
    This->indexBlockDepotCached = depotBlockCount;

    if (depotBlockCount < COUNT_BBDEPOTINHEADER)
    {
      depotBlockIndexPos = This->bigBlockDepotStart[depotBlockCount];
    }
    else
    {
      /*
       * We have to look in the extended depot.
       */
      depotBlockIndexPos = Storage32Impl_GetExtDepotBlock(This, depotBlockCount);
    }

    StorageImpl_ReadBigBlock(This, depotBlockIndexPos, depotBuffer, &read);

    if (!read)
      return STG_E_READFAULT;

    num_blocks = This->bigBlockSize / 4;

    for (index = 0; index < num_blocks; index++)
    {
      StorageUtl_ReadDWord(depotBuffer, index*sizeof(ULONG), nextBlockIndex);
      This->blockDepotCached[index] = *nextBlockIndex;
    }
  }

  *nextBlockIndex = This->blockDepotCached[depotBlockOffset/sizeof(ULONG)];

  return S_OK;
}

/******************************************************************************
 *      Storage32Impl_GetNextExtendedBlock
 *
 * Given an extended block this method will return the next extended block.
 *
 * NOTES:
 * The last ULONG of an extended block is the block index of the next
 * extended block. Extended blocks are marked as BLOCK_EXTBBDEPOT in the
 * depot.
 *
 * Return values:
 *    - The index of the next extended block
 *    - BLOCK_UNUSED: there is no next extended block.
 *    - Any other return values denotes failure.
 */
static ULONG Storage32Impl_GetNextExtendedBlock(StorageImpl* This, ULONG blockIndex)
{
  ULONG nextBlockIndex   = BLOCK_SPECIAL;
  ULONG depotBlockOffset = This->bigBlockSize - sizeof(ULONG);

  StorageImpl_ReadDWordFromBigBlock(This, blockIndex, depotBlockOffset,
                        &nextBlockIndex);

  return nextBlockIndex;
}

/******************************************************************************
 *      StorageImpl_SetNextBlockInChain
 *
 * This method will write the index of the specified block's next block
 * in the big block depot.
 *
 * For example: to create the chain 3 -> 1 -> 7 -> End of Chain
 *              do the following
 *
 * StorageImpl_SetNextBlockInChain(This, 3, 1);
 * StorageImpl_SetNextBlockInChain(This, 1, 7);
 * StorageImpl_SetNextBlockInChain(This, 7, BLOCK_END_OF_CHAIN);
 *
 */
static void StorageImpl_SetNextBlockInChain(
          StorageImpl* This,
          ULONG          blockIndex,
          ULONG          nextBlock)
{
  ULONG offsetInDepot    = blockIndex * sizeof (ULONG);
  ULONG depotBlockCount  = offsetInDepot / This->bigBlockSize;
  ULONG depotBlockOffset = offsetInDepot % This->bigBlockSize;
  ULONG depotBlockIndexPos;

  assert(depotBlockCount < This->bigBlockDepotCount);
  assert(blockIndex != nextBlock);

  if (blockIndex == (RANGELOCK_FIRST / This->bigBlockSize) - 1)
    /* This should never happen (storage file format spec forbids it), but
     * older versions of Wine may have generated broken files. We don't want to
     * assert and potentially lose data, but we do want to know if this ever
     * happens in a newly-created file. */
    ERR("Using range lock page\n");

  if (depotBlockCount < COUNT_BBDEPOTINHEADER)
  {
    depotBlockIndexPos = This->bigBlockDepotStart[depotBlockCount];
  }
  else
  {
    /*
     * We have to look in the extended depot.
     */
    depotBlockIndexPos = Storage32Impl_GetExtDepotBlock(This, depotBlockCount);
  }

  StorageImpl_WriteDWordToBigBlock(This, depotBlockIndexPos, depotBlockOffset,
                        nextBlock);
  /*
   * Update the cached block depot, if necessary.
   */
  if (depotBlockCount == This->indexBlockDepotCached)
  {
    This->blockDepotCached[depotBlockOffset/sizeof(ULONG)] = nextBlock;
  }
}

/******************************************************************************
 *      StorageImpl_GetNextFreeBigBlock
 *
 * Returns the index of the next free big block.
 * If the big block depot is filled, this method will enlarge it.
 *
 */
static ULONG StorageImpl_GetNextFreeBigBlock(
  StorageImpl* This, ULONG neededAddNumBlocks)
{
  ULONG depotBlockIndexPos;
  BYTE depotBuffer[MAX_BIG_BLOCK_SIZE];
  ULONG depotBlockOffset;
  ULONG blocksPerDepot    = This->bigBlockSize / sizeof(ULONG);
  ULONG nextBlockIndex    = BLOCK_SPECIAL;
  int   depotIndex        = 0;
  ULONG freeBlock         = BLOCK_UNUSED;
  ULONG read;
  ULARGE_INTEGER neededSize;
  STATSTG statstg;

  depotIndex = This->prevFreeBlock / blocksPerDepot;
  depotBlockOffset = (This->prevFreeBlock % blocksPerDepot) * sizeof(ULONG);

  /*
   * Scan the entire big block depot until we find a block marked free
   */
  while (nextBlockIndex != BLOCK_UNUSED)
  {
    if (depotIndex < COUNT_BBDEPOTINHEADER)
    {
      depotBlockIndexPos = This->bigBlockDepotStart[depotIndex];

      /*
       * Grow the primary depot.
       */
      if (depotBlockIndexPos == BLOCK_UNUSED)
      {
        depotBlockIndexPos = depotIndex*blocksPerDepot;

        /*
         * Add a block depot.
         */
        Storage32Impl_AddBlockDepot(This, depotBlockIndexPos, depotIndex);
        This->bigBlockDepotCount++;
        This->bigBlockDepotStart[depotIndex] = depotBlockIndexPos;

        /*
         * Flag it as a block depot.
         */
        StorageImpl_SetNextBlockInChain(This,
                                          depotBlockIndexPos,
                                          BLOCK_SPECIAL);

        /* Save new header information.
         */
        StorageImpl_SaveFileHeader(This);
      }
    }
    else
    {
      depotBlockIndexPos = Storage32Impl_GetExtDepotBlock(This, depotIndex);

      if (depotBlockIndexPos == BLOCK_UNUSED)
      {
        /*
         * Grow the extended depot.
         */
        ULONG extIndex       = BLOCK_UNUSED;
        ULONG numExtBlocks   = depotIndex - COUNT_BBDEPOTINHEADER;
        ULONG extBlockOffset = numExtBlocks % (blocksPerDepot - 1);

        if (extBlockOffset == 0)
        {
          /* We need an extended block.
           */
          extIndex = Storage32Impl_AddExtBlockDepot(This);
          This->extBigBlockDepotCount++;
          depotBlockIndexPos = extIndex + 1;
        }
        else
          depotBlockIndexPos = depotIndex * blocksPerDepot;

        /*
         * Add a block depot and mark it in the extended block.
         */
        Storage32Impl_AddBlockDepot(This, depotBlockIndexPos, depotIndex);
        This->bigBlockDepotCount++;
        Storage32Impl_SetExtDepotBlock(This, depotIndex, depotBlockIndexPos);

        /* Flag the block depot.
         */
        StorageImpl_SetNextBlockInChain(This,
                                          depotBlockIndexPos,
                                          BLOCK_SPECIAL);

        /* If necessary, flag the extended depot block.
         */
        if (extIndex != BLOCK_UNUSED)
          StorageImpl_SetNextBlockInChain(This, extIndex, BLOCK_EXTBBDEPOT);

        /* Save header information.
         */
        StorageImpl_SaveFileHeader(This);
      }
    }

    StorageImpl_ReadBigBlock(This, depotBlockIndexPos, depotBuffer, &read);

    if (read)
    {
      while ( ( (depotBlockOffset/sizeof(ULONG) ) < blocksPerDepot) &&
              ( nextBlockIndex != BLOCK_UNUSED))
      {
        StorageUtl_ReadDWord(depotBuffer, depotBlockOffset, &nextBlockIndex);

        if (nextBlockIndex == BLOCK_UNUSED)
        {
          freeBlock = (depotIndex * blocksPerDepot) +
                      (depotBlockOffset/sizeof(ULONG));
        }

        depotBlockOffset += sizeof(ULONG);
      }
    }

    depotIndex++;
    depotBlockOffset = 0;
  }

  /*
   * make sure that the block physically exists before using it
   */
  neededSize.QuadPart = StorageImpl_GetBigBlockOffset(This, freeBlock)+This->bigBlockSize * neededAddNumBlocks;

  ILockBytes_Stat(This->lockBytes, &statstg, STATFLAG_NONAME);

  if (neededSize.QuadPart > statstg.cbSize.QuadPart)
    ILockBytes_SetSize(This->lockBytes, neededSize);

  This->prevFreeBlock = freeBlock;

  return freeBlock;
}

/******************************************************************************
 *      StorageImpl_FreeBigBlock
 *
 * This method will flag the specified block as free in the big block depot.
 */
static void StorageImpl_FreeBigBlock(
  StorageImpl* This,
  ULONG          blockIndex)
{
  StorageImpl_SetNextBlockInChain(This, blockIndex, BLOCK_UNUSED);

  if (blockIndex < This->prevFreeBlock)
    This->prevFreeBlock = blockIndex;
}


static HRESULT StorageImpl_BaseWriteDirEntry(StorageBaseImpl *base,
  DirRef index, const DirEntry *data)
{
  StorageImpl *This = (StorageImpl*)base;
  return StorageImpl_WriteDirEntry(This, index, data);
}

static HRESULT StorageImpl_BaseReadDirEntry(StorageBaseImpl *base,
  DirRef index, DirEntry *data)
{
  StorageImpl *This = (StorageImpl*)base;
  return StorageImpl_ReadDirEntry(This, index, data);
}

static BlockChainStream **StorageImpl_GetFreeBlockChainCacheEntry(StorageImpl* This)
{
  int i;

  for (i=0; i<BLOCKCHAIN_CACHE_SIZE; i++)
  {
    if (!This->blockChainCache[i])
    {
      return &This->blockChainCache[i];
    }
  }

  i = This->blockChainToEvict;

  BlockChainStream_Destroy(This->blockChainCache[i]);
  This->blockChainCache[i] = NULL;

  This->blockChainToEvict++;
  if (This->blockChainToEvict == BLOCKCHAIN_CACHE_SIZE)
    This->blockChainToEvict = 0;

  return &This->blockChainCache[i];
}

static BlockChainStream **StorageImpl_GetCachedBlockChainStream(StorageImpl *This,
    DirRef index)
{
  int i, free_index=-1;

  for (i=0; i<BLOCKCHAIN_CACHE_SIZE; i++)
  {
    if (!This->blockChainCache[i])
    {
      if (free_index == -1) free_index = i;
    }
    else if (This->blockChainCache[i]->ownerDirEntry == index)
    {
      return &This->blockChainCache[i];
    }
  }

  if (free_index == -1)
  {
    free_index = This->blockChainToEvict;

    BlockChainStream_Destroy(This->blockChainCache[free_index]);
    This->blockChainCache[free_index] = NULL;

    This->blockChainToEvict++;
    if (This->blockChainToEvict == BLOCKCHAIN_CACHE_SIZE)
      This->blockChainToEvict = 0;
  }

  This->blockChainCache[free_index] = BlockChainStream_Construct(This, NULL, index);
  return &This->blockChainCache[free_index];
}

static void StorageImpl_DeleteCachedBlockChainStream(StorageImpl *This, DirRef index)
{
  int i;

  for (i=0; i<BLOCKCHAIN_CACHE_SIZE; i++)
  {
    if (This->blockChainCache[i] && This->blockChainCache[i]->ownerDirEntry == index)
    {
      BlockChainStream_Destroy(This->blockChainCache[i]);
      This->blockChainCache[i] = NULL;
      return;
    }
  }
}

static HRESULT StorageImpl_StreamReadAt(StorageBaseImpl *base, DirRef index,
  ULARGE_INTEGER offset, ULONG size, void *buffer, ULONG *bytesRead)
{
  StorageImpl *This = (StorageImpl*)base;
  DirEntry data;
  HRESULT hr;
  ULONG bytesToRead;

  hr = StorageImpl_ReadDirEntry(This, index, &data);
  if (FAILED(hr)) return hr;

  if (data.size.QuadPart == 0)
  {
    *bytesRead = 0;
    return S_OK;
  }

  if (offset.QuadPart + size > data.size.QuadPart)
  {
    bytesToRead = data.size.QuadPart - offset.QuadPart;
  }
  else
  {
    bytesToRead = size;
  }

  if (data.size.QuadPart < LIMIT_TO_USE_SMALL_BLOCK)
  {
    SmallBlockChainStream *stream;

    stream = SmallBlockChainStream_Construct(This, NULL, index);
    if (!stream) return E_OUTOFMEMORY;

    hr = SmallBlockChainStream_ReadAt(stream, offset, bytesToRead, buffer, bytesRead);

    SmallBlockChainStream_Destroy(stream);

    return hr;
  }
  else
  {
    BlockChainStream *stream = NULL;

    stream = *StorageImpl_GetCachedBlockChainStream(This, index);
    if (!stream) return E_OUTOFMEMORY;

    hr = BlockChainStream_ReadAt(stream, offset, bytesToRead, buffer, bytesRead);

    return hr;
  }
}

static HRESULT StorageImpl_StreamSetSize(StorageBaseImpl *base, DirRef index,
  ULARGE_INTEGER newsize)
{
  StorageImpl *This = (StorageImpl*)base;
  DirEntry data;
  HRESULT hr;
  SmallBlockChainStream *smallblock=NULL;
  BlockChainStream **pbigblock=NULL, *bigblock=NULL;

  hr = StorageImpl_ReadDirEntry(This, index, &data);
  if (FAILED(hr)) return hr;

  /* In simple mode keep the stream size above the small block limit */
  if (This->base.openFlags & STGM_SIMPLE)
    newsize.QuadPart = max(newsize.QuadPart, LIMIT_TO_USE_SMALL_BLOCK);

  if (data.size.QuadPart == newsize.QuadPart)
    return S_OK;

  /* Create a block chain object of the appropriate type */
  if (data.size.QuadPart == 0)
  {
    if (newsize.QuadPart < LIMIT_TO_USE_SMALL_BLOCK)
    {
      smallblock = SmallBlockChainStream_Construct(This, NULL, index);
      if (!smallblock) return E_OUTOFMEMORY;
    }
    else
    {
      pbigblock = StorageImpl_GetCachedBlockChainStream(This, index);
      bigblock = *pbigblock;
      if (!bigblock) return E_OUTOFMEMORY;
    }
  }
  else if (data.size.QuadPart < LIMIT_TO_USE_SMALL_BLOCK)
  {
    smallblock = SmallBlockChainStream_Construct(This, NULL, index);
    if (!smallblock) return E_OUTOFMEMORY;
  }
  else
  {
    pbigblock = StorageImpl_GetCachedBlockChainStream(This, index);
    bigblock = *pbigblock;
    if (!bigblock) return E_OUTOFMEMORY;
  }

  /* Change the block chain type if necessary. */
  if (smallblock && newsize.QuadPart >= LIMIT_TO_USE_SMALL_BLOCK)
  {
    bigblock = Storage32Impl_SmallBlocksToBigBlocks(This, &smallblock);
    if (!bigblock)
    {
      SmallBlockChainStream_Destroy(smallblock);
      return E_FAIL;
    }

    pbigblock = StorageImpl_GetFreeBlockChainCacheEntry(This);
    *pbigblock = bigblock;
  }
  else if (bigblock && newsize.QuadPart < LIMIT_TO_USE_SMALL_BLOCK)
  {
    smallblock = Storage32Impl_BigBlocksToSmallBlocks(This, pbigblock, newsize);
    if (!smallblock)
      return E_FAIL;
  }

  /* Set the size of the block chain. */
  if (smallblock)
  {
    SmallBlockChainStream_SetSize(smallblock, newsize);
    SmallBlockChainStream_Destroy(smallblock);
  }
  else
  {
    BlockChainStream_SetSize(bigblock, newsize);
  }

  /* Set the size in the directory entry. */
  hr = StorageImpl_ReadDirEntry(This, index, &data);
  if (SUCCEEDED(hr))
  {
    data.size = newsize;

    hr = StorageImpl_WriteDirEntry(This, index, &data);
  }
  return hr;
}

static HRESULT StorageImpl_StreamWriteAt(StorageBaseImpl *base, DirRef index,
  ULARGE_INTEGER offset, ULONG size, const void *buffer, ULONG *bytesWritten)
{
  StorageImpl *This = (StorageImpl*)base;
  DirEntry data;
  HRESULT hr;
  ULARGE_INTEGER newSize;

  hr = StorageImpl_ReadDirEntry(This, index, &data);
  if (FAILED(hr)) return hr;

  /* Grow the stream if necessary */
  newSize.QuadPart = offset.QuadPart + size;

  if (newSize.QuadPart > data.size.QuadPart)
  {
    hr = StorageImpl_StreamSetSize(base, index, newSize);
    if (FAILED(hr))
      return hr;

    hr = StorageImpl_ReadDirEntry(This, index, &data);
    if (FAILED(hr)) return hr;
  }

  if (data.size.QuadPart < LIMIT_TO_USE_SMALL_BLOCK)
  {
    SmallBlockChainStream *stream;

    stream = SmallBlockChainStream_Construct(This, NULL, index);
    if (!stream) return E_OUTOFMEMORY;

    hr = SmallBlockChainStream_WriteAt(stream, offset, size, buffer, bytesWritten);

    SmallBlockChainStream_Destroy(stream);

    return hr;
  }
  else
  {
    BlockChainStream *stream;

    stream = *StorageImpl_GetCachedBlockChainStream(This, index);
    if (!stream) return E_OUTOFMEMORY;

    return BlockChainStream_WriteAt(stream, offset, size, buffer, bytesWritten);
  }
}

static HRESULT StorageImpl_StreamLink(StorageBaseImpl *base, DirRef dst,
  DirRef src)
{
  StorageImpl *This = (StorageImpl*)base;
  DirEntry dst_data, src_data;
  HRESULT hr;

  hr = StorageImpl_ReadDirEntry(This, dst, &dst_data);

  if (SUCCEEDED(hr))
    hr = StorageImpl_ReadDirEntry(This, src, &src_data);

  if (SUCCEEDED(hr))
  {
    StorageImpl_DeleteCachedBlockChainStream(This, src);
    dst_data.startingBlock = src_data.startingBlock;
    dst_data.size = src_data.size;

    hr = StorageImpl_WriteDirEntry(This, dst, &dst_data);
  }

  return hr;
}

static HRESULT StorageImpl_Refresh(StorageImpl *This, BOOL new_object, BOOL create)
{
  HRESULT hr=S_OK;
  DirEntry currentEntry;
  DirRef      currentEntryRef;
  BlockChainStream *blockChainStream;

  if (create)
  {
    ULARGE_INTEGER size;
    BYTE bigBlockBuffer[MAX_BIG_BLOCK_SIZE];

    /* Discard any existing data. */
    size.QuadPart = 0;
    ILockBytes_SetSize(This->lockBytes, size);

    /*
     * Initialize all header variables:
     * - The big block depot consists of one block and it is at block 0
     * - The directory table starts at block 1
     * - There is no small block depot
     */
    memset( This->bigBlockDepotStart,
            BLOCK_UNUSED,
            sizeof(This->bigBlockDepotStart));

    This->bigBlockDepotCount    = 1;
    This->bigBlockDepotStart[0] = 0;
    This->rootStartBlock        = 1;
    This->smallBlockLimit       = LIMIT_TO_USE_SMALL_BLOCK;
    This->smallBlockDepotStart  = BLOCK_END_OF_CHAIN;
    if (This->bigBlockSize == 4096)
      This->bigBlockSizeBits    = MAX_BIG_BLOCK_SIZE_BITS;
    else
      This->bigBlockSizeBits    = MIN_BIG_BLOCK_SIZE_BITS;
    This->smallBlockSizeBits    = DEF_SMALL_BLOCK_SIZE_BITS;
    This->extBigBlockDepotStart = BLOCK_END_OF_CHAIN;
    This->extBigBlockDepotCount = 0;

    StorageImpl_SaveFileHeader(This);

    /*
     * Add one block for the big block depot and one block for the directory table
     */
    size.HighPart = 0;
    size.LowPart  = This->bigBlockSize * 3;
    ILockBytes_SetSize(This->lockBytes, size);

    /*
     * Initialize the big block depot
     */
    memset(bigBlockBuffer, BLOCK_UNUSED, This->bigBlockSize);
    StorageUtl_WriteDWord(bigBlockBuffer, 0, BLOCK_SPECIAL);
    StorageUtl_WriteDWord(bigBlockBuffer, sizeof(ULONG), BLOCK_END_OF_CHAIN);
    StorageImpl_WriteBigBlock(This, 0, bigBlockBuffer);
  }
  else
  {
    /*
     * Load the header for the file.
     */
    hr = StorageImpl_LoadFileHeader(This);

    if (FAILED(hr))
    {
      return hr;
    }
  }

  /*
   * There is no block depot cached yet.
   */
  This->indexBlockDepotCached = 0xFFFFFFFF;
  This->indexExtBlockDepotCached = 0xFFFFFFFF;

  /*
   * Start searching for free blocks with block 0.
   */
  This->prevFreeBlock = 0;

  This->firstFreeSmallBlock = 0;

  /* Read the extended big block depot locations. */
  if (This->extBigBlockDepotCount != 0)
  {
    ULONG current_block = This->extBigBlockDepotStart;
    ULONG cache_size = This->extBigBlockDepotCount * 2;
    ULONG i;

    This->extBigBlockDepotLocations = HeapAlloc(GetProcessHeap(), 0, sizeof(ULONG) * cache_size);
    if (!This->extBigBlockDepotLocations)
    {
      return E_OUTOFMEMORY;
    }

    This->extBigBlockDepotLocationsSize = cache_size;

    for (i=0; i<This->extBigBlockDepotCount; i++)
    {
      if (current_block == BLOCK_END_OF_CHAIN)
      {
        WARN("File has too few extended big block depot blocks.\n");
        return STG_E_DOCFILECORRUPT;
      }
      This->extBigBlockDepotLocations[i] = current_block;
      current_block = Storage32Impl_GetNextExtendedBlock(This, current_block);
    }
  }
  else
  {
    This->extBigBlockDepotLocations = NULL;
    This->extBigBlockDepotLocationsSize = 0;
  }

  /*
   * Create the block chain abstractions.
   */
  if(!(blockChainStream =
       BlockChainStream_Construct(This, &This->rootStartBlock, DIRENTRY_NULL)))
  {
    return STG_E_READFAULT;
  }
  if (!new_object)
    BlockChainStream_Destroy(This->rootBlockChain);
  This->rootBlockChain = blockChainStream;

  if(!(blockChainStream =
       BlockChainStream_Construct(This, &This->smallBlockDepotStart,
				  DIRENTRY_NULL)))
  {
    return STG_E_READFAULT;
  }
  if (!new_object)
    BlockChainStream_Destroy(This->smallBlockDepotChain);
  This->smallBlockDepotChain = blockChainStream;

  /*
   * Write the root storage entry (memory only)
   */
  if (create)
  {
    DirEntry rootEntry;
    /*
     * Initialize the directory table
     */
    memset(&rootEntry, 0, sizeof(rootEntry));
    lstrcpyW(rootEntry.name, L"Root Entry");
    rootEntry.sizeOfNameString = sizeof(L"Root Entry");
    rootEntry.stgType          = STGTY_ROOT;
    rootEntry.leftChild        = DIRENTRY_NULL;
    rootEntry.rightChild       = DIRENTRY_NULL;
    rootEntry.dirRootEntry     = DIRENTRY_NULL;
    rootEntry.startingBlock    = BLOCK_END_OF_CHAIN;
    rootEntry.size.HighPart    = 0;
    rootEntry.size.LowPart     = 0;

    StorageImpl_WriteDirEntry(This, 0, &rootEntry);
  }

  /*
   * Find the ID of the root storage.
   */
  currentEntryRef = 0;

  do
  {
    hr = StorageImpl_ReadDirEntry(
                      This,
                      currentEntryRef,
                      &currentEntry);

    if (SUCCEEDED(hr))
    {
      if ( (currentEntry.sizeOfNameString != 0 ) &&
           (currentEntry.stgType          == STGTY_ROOT) )
      {
        This->base.storageDirEntry = currentEntryRef;
      }
    }

    currentEntryRef++;

  } while (SUCCEEDED(hr) && (This->base.storageDirEntry == DIRENTRY_NULL) );

  if (FAILED(hr))
  {
    return STG_E_READFAULT;
  }

  /*
   * Create the block chain abstraction for the small block root chain.
   */
  if(!(blockChainStream =
       BlockChainStream_Construct(This, NULL, This->base.storageDirEntry)))
  {
    return STG_E_READFAULT;
  }
  if (!new_object)
    BlockChainStream_Destroy(This->smallBlockRootChain);
  This->smallBlockRootChain = blockChainStream;

  if (!new_object)
  {
    int i;
    for (i=0; i<BLOCKCHAIN_CACHE_SIZE; i++)
    {
      BlockChainStream_Destroy(This->blockChainCache[i]);
      This->blockChainCache[i] = NULL;
    }
  }

  return hr;
}

static HRESULT StorageImpl_GetTransactionSig(StorageBaseImpl *base,
  ULONG* result, BOOL refresh)
{
  StorageImpl *This = (StorageImpl*)base;
  HRESULT hr=S_OK;
  DWORD oldTransactionSig = This->transactionSig;

  if (refresh)
  {
    ULARGE_INTEGER offset;
    ULONG bytes_read;
    BYTE data[4];

    offset.HighPart = 0;
    offset.LowPart = OFFSET_TRANSACTIONSIG;
    hr = StorageImpl_ReadAt(This, offset, data, 4, &bytes_read);

    if (SUCCEEDED(hr))
    {
      StorageUtl_ReadDWord(data, 0, &This->transactionSig);

      if (oldTransactionSig != This->transactionSig)
      {
        /* Someone else wrote to this, so toss all cached information. */
        TRACE("signature changed\n");

        hr = StorageImpl_Refresh(This, FALSE, FALSE);
      }

      if (FAILED(hr))
        This->transactionSig = oldTransactionSig;
    }
  }

  *result = This->transactionSig;

  return hr;
}

static HRESULT StorageImpl_SetTransactionSig(StorageBaseImpl *base,
  ULONG value)
{
  StorageImpl *This = (StorageImpl*)base;

  This->transactionSig = value;
  StorageImpl_SaveFileHeader(This);

  return S_OK;
}

static HRESULT StorageImpl_LockRegion(StorageImpl *This, ULARGE_INTEGER offset,
    ULARGE_INTEGER cb, DWORD dwLockType, BOOL *supported)
{
    if ((dwLockType & This->locks_supported) == 0)
    {
        if (supported) *supported = FALSE;
        return S_OK;
    }

    if (supported) *supported = TRUE;
    return ILockBytes_LockRegion(This->lockBytes, offset, cb, dwLockType);
}

static HRESULT StorageImpl_UnlockRegion(StorageImpl *This, ULARGE_INTEGER offset,
    ULARGE_INTEGER cb, DWORD dwLockType)
{
    if ((dwLockType & This->locks_supported) == 0)
        return S_OK;

    return ILockBytes_UnlockRegion(This->lockBytes, offset, cb, dwLockType);
}

/* Internal function */
static HRESULT StorageImpl_LockRegionSync(StorageImpl *This, ULARGE_INTEGER offset,
    ULARGE_INTEGER cb, DWORD dwLockType, BOOL *supported)
{
    HRESULT hr;
    int delay = 0;
    DWORD start_time = GetTickCount();
    DWORD last_sanity_check = start_time;
    ULARGE_INTEGER sanity_offset, sanity_cb;

    sanity_offset.QuadPart = RANGELOCK_UNK1_FIRST;
    sanity_cb.QuadPart = RANGELOCK_UNK1_LAST - RANGELOCK_UNK1_FIRST + 1;

    do
    {
        hr = StorageImpl_LockRegion(This, offset, cb, dwLockType, supported);

        if (hr == STG_E_ACCESSDENIED || hr == STG_E_LOCKVIOLATION)
        {
            DWORD current_time = GetTickCount();
            if (current_time - start_time >= 20000)
            {
                /* timeout */
                break;
            }
            if (current_time - last_sanity_check >= 500)
            {
                /* Any storage implementation with the file open in a
                 * shared mode should not lock these bytes for writing. However,
                 * some programs (LibreOffice Writer) will keep ALL bytes locked
                 * when opening in exclusive mode. We can use a read lock to
                 * detect this case early, and not hang a full 20 seconds.
                 *
                 * This can collide with another attempt to open the file in
                 * exclusive mode, but it's unlikely, and someone would fail anyway. */
                hr = StorageImpl_LockRegion(This, sanity_offset, sanity_cb, WINE_LOCK_READ, NULL);
                if (hr == STG_E_ACCESSDENIED || hr == STG_E_LOCKVIOLATION)
                    break;
                if (SUCCEEDED(hr))
                {
                    StorageImpl_UnlockRegion(This, sanity_offset, sanity_cb, WINE_LOCK_READ);
                    hr = STG_E_ACCESSDENIED;
                }

                last_sanity_check = current_time;
            }
            Sleep(delay);
            if (delay < 150) delay++;
        }
    } while (hr == STG_E_ACCESSDENIED || hr == STG_E_LOCKVIOLATION);

    return hr;
}

static HRESULT StorageImpl_LockTransaction(StorageBaseImpl *base, BOOL write)
{
  StorageImpl *This = (StorageImpl*)base;
  HRESULT hr;
  ULARGE_INTEGER offset, cb;

  if (write)
  {
    /* Synchronous grab of second priority range, the commit lock, and the
     * lock-checking lock. */
    offset.QuadPart = RANGELOCK_TRANSACTION_FIRST;
    cb.QuadPart = RANGELOCK_TRANSACTION_LAST - RANGELOCK_TRANSACTION_FIRST + 1;
  }
  else
  {
    offset.QuadPart = RANGELOCK_COMMIT;
    cb.QuadPart = 1;
  }

  hr = StorageImpl_LockRegionSync(This, offset, cb, LOCK_ONLYONCE, NULL);

  return hr;
}

static HRESULT StorageImpl_UnlockTransaction(StorageBaseImpl *base, BOOL write)
{
  StorageImpl *This = (StorageImpl*)base;
  HRESULT hr;
  ULARGE_INTEGER offset, cb;

  if (write)
  {
    offset.QuadPart = RANGELOCK_TRANSACTION_FIRST;
    cb.QuadPart = RANGELOCK_TRANSACTION_LAST - RANGELOCK_TRANSACTION_FIRST + 1;
  }
  else
  {
    offset.QuadPart = RANGELOCK_COMMIT;
    cb.QuadPart = 1;
  }

  hr = StorageImpl_UnlockRegion(This, offset, cb, LOCK_ONLYONCE);

  return hr;
}

static HRESULT StorageImpl_GetFilename(StorageBaseImpl* iface, LPWSTR *result)
{
  StorageImpl *This = (StorageImpl*) iface;
  STATSTG statstg;
  HRESULT hr;

  hr = ILockBytes_Stat(This->lockBytes, &statstg, 0);

  *result = statstg.pwcsName;

  return hr;
}

static HRESULT StorageImpl_CheckLockRange(StorageImpl *This, ULONG start,
    ULONG end, HRESULT fail_hr)
{
    HRESULT hr;
    ULARGE_INTEGER offset, cb;

    offset.QuadPart = start;
    cb.QuadPart = 1 + end - start;

    hr = StorageImpl_LockRegion(This, offset, cb, LOCK_ONLYONCE, NULL);
    if (SUCCEEDED(hr)) StorageImpl_UnlockRegion(This, offset, cb, LOCK_ONLYONCE);

    if (FAILED(hr))
        return fail_hr;
    else
        return S_OK;
}

static HRESULT StorageImpl_LockOne(StorageImpl *This, ULONG start, ULONG end)
{
    HRESULT hr=S_OK;
    int i, j;
    ULARGE_INTEGER offset, cb;

    cb.QuadPart = 1;

    for (i=start; i<=end; i++)
    {
        offset.QuadPart = i;
        hr = StorageImpl_LockRegion(This, offset, cb, LOCK_ONLYONCE, NULL);
        if (hr != STG_E_ACCESSDENIED && hr != STG_E_LOCKVIOLATION)
            break;
    }

    if (SUCCEEDED(hr))
    {
        for (j = 0; j < ARRAY_SIZE(This->locked_bytes); j++)
        {
            if (This->locked_bytes[j] == 0)
            {
                This->locked_bytes[j] = i;
                break;
            }
        }
    }

    return hr;
}

static HRESULT StorageImpl_GrabLocks(StorageImpl *This, DWORD openFlags)
{
    HRESULT hr;
    ULARGE_INTEGER offset;
    ULARGE_INTEGER cb;
    DWORD share_mode = STGM_SHARE_MODE(openFlags);
    BOOL supported, ro_denyw;

    if (openFlags & STGM_NOSNAPSHOT)
    {
        /* STGM_NOSNAPSHOT implies deny write */
        if (share_mode == STGM_SHARE_DENY_READ) share_mode = STGM_SHARE_EXCLUSIVE;
        else if (share_mode != STGM_SHARE_EXCLUSIVE) share_mode = STGM_SHARE_DENY_WRITE;
    }

    ro_denyw = (STGM_ACCESS_MODE(openFlags) == STGM_READ) && (share_mode == STGM_SHARE_DENY_WRITE);

    /* Wrap all other locking inside a single lock so we can check ranges safely */
    offset.QuadPart = RANGELOCK_CHECKLOCKS;
    cb.QuadPart = 1;
    hr = StorageImpl_LockRegionSync(This, offset, cb, LOCK_ONLYONCE, &supported);

    /* If the ILockBytes doesn't support locking that's ok. */
    if (!supported) return S_OK;
    else if (FAILED(hr)) return hr;

    hr = S_OK;

    /* First check for any conflicting locks. */
    if ((openFlags & STGM_PRIORITY) == STGM_PRIORITY)
        hr = StorageImpl_CheckLockRange(This, RANGELOCK_COMMIT, RANGELOCK_COMMIT, STG_E_LOCKVIOLATION);

    if (SUCCEEDED(hr) && (STGM_ACCESS_MODE(openFlags) != STGM_WRITE))
        hr = StorageImpl_CheckLockRange(This, RANGELOCK_DENY_READ_FIRST, RANGELOCK_DENY_READ_LAST, STG_E_SHAREVIOLATION);

    if (SUCCEEDED(hr) && (STGM_ACCESS_MODE(openFlags) != STGM_READ))
        hr = StorageImpl_CheckLockRange(This, RANGELOCK_DENY_WRITE_FIRST, RANGELOCK_DENY_WRITE_LAST, STG_E_SHAREVIOLATION);

    if (SUCCEEDED(hr) && (share_mode == STGM_SHARE_DENY_READ || share_mode == STGM_SHARE_EXCLUSIVE))
        hr = StorageImpl_CheckLockRange(This, RANGELOCK_READ_FIRST, RANGELOCK_READ_LAST, STG_E_LOCKVIOLATION);

    if (SUCCEEDED(hr) && (share_mode == STGM_SHARE_DENY_WRITE || share_mode == STGM_SHARE_EXCLUSIVE))
        hr = StorageImpl_CheckLockRange(This, RANGELOCK_WRITE_FIRST, RANGELOCK_WRITE_LAST, STG_E_LOCKVIOLATION);

    if (SUCCEEDED(hr) && STGM_ACCESS_MODE(openFlags) == STGM_READ && share_mode == STGM_SHARE_EXCLUSIVE)
    {
        hr = StorageImpl_CheckLockRange(This, 0, RANGELOCK_CHECKLOCKS-1, STG_E_LOCKVIOLATION);

        if (SUCCEEDED(hr))
            hr = StorageImpl_CheckLockRange(This, RANGELOCK_CHECKLOCKS+1, RANGELOCK_LAST, STG_E_LOCKVIOLATION);
    }

    /* Then grab our locks. */
    if (SUCCEEDED(hr) && (openFlags & STGM_PRIORITY) == STGM_PRIORITY)
    {
        hr = StorageImpl_LockOne(This, RANGELOCK_PRIORITY1_FIRST, RANGELOCK_PRIORITY1_LAST);
        if (SUCCEEDED(hr))
            hr = StorageImpl_LockOne(This, RANGELOCK_PRIORITY2_FIRST, RANGELOCK_PRIORITY2_LAST);
    }

    if (SUCCEEDED(hr) && (STGM_ACCESS_MODE(openFlags) != STGM_WRITE) && !ro_denyw)
        hr = StorageImpl_LockOne(This, RANGELOCK_READ_FIRST, RANGELOCK_READ_LAST);

    if (SUCCEEDED(hr) && (STGM_ACCESS_MODE(openFlags) != STGM_READ))
        hr = StorageImpl_LockOne(This, RANGELOCK_WRITE_FIRST, RANGELOCK_WRITE_LAST);

    if (SUCCEEDED(hr) && (share_mode == STGM_SHARE_DENY_READ || share_mode == STGM_SHARE_EXCLUSIVE))
        hr = StorageImpl_LockOne(This, RANGELOCK_DENY_READ_FIRST, RANGELOCK_DENY_READ_LAST);

    if (SUCCEEDED(hr) && (share_mode == STGM_SHARE_DENY_WRITE || share_mode == STGM_SHARE_EXCLUSIVE) && !ro_denyw)
        hr = StorageImpl_LockOne(This, RANGELOCK_DENY_WRITE_FIRST, RANGELOCK_DENY_WRITE_LAST);

    if (SUCCEEDED(hr) && (openFlags & STGM_NOSNAPSHOT) == STGM_NOSNAPSHOT)
        hr = StorageImpl_LockOne(This, RANGELOCK_NOSNAPSHOT_FIRST, RANGELOCK_NOSNAPSHOT_LAST);

    offset.QuadPart = RANGELOCK_CHECKLOCKS;
    cb.QuadPart = 1;
    StorageImpl_UnlockRegion(This, offset, cb, LOCK_ONLYONCE);

    return hr;
}

static HRESULT StorageImpl_Flush(StorageBaseImpl *storage)
{
  StorageImpl *This = (StorageImpl*)storage;
  int i;
  HRESULT hr;
  TRACE("(%p)\n", This);

  hr = BlockChainStream_Flush(This->smallBlockRootChain);

  if (SUCCEEDED(hr))
    hr = BlockChainStream_Flush(This->rootBlockChain);

  if (SUCCEEDED(hr))
    hr = BlockChainStream_Flush(This->smallBlockDepotChain);

  for (i=0; SUCCEEDED(hr) && i<BLOCKCHAIN_CACHE_SIZE; i++)
    if (This->blockChainCache[i])
      hr = BlockChainStream_Flush(This->blockChainCache[i]);

  if (SUCCEEDED(hr))
    hr = ILockBytes_Flush(This->lockBytes);

  return hr;
}

static void StorageImpl_Invalidate(StorageBaseImpl* iface)
{
  StorageImpl *This = (StorageImpl*) iface;

  StorageBaseImpl_DeleteAll(&This->base);

  This->base.reverted = TRUE;
}

static void StorageImpl_Destroy(StorageBaseImpl* iface)
{
  StorageImpl *This = (StorageImpl*) iface;
  int i;
  TRACE("(%p)\n", This);

  StorageImpl_Flush(iface);

  StorageImpl_Invalidate(iface);

  HeapFree(GetProcessHeap(), 0, This->extBigBlockDepotLocations);

  BlockChainStream_Destroy(This->smallBlockRootChain);
  BlockChainStream_Destroy(This->rootBlockChain);
  BlockChainStream_Destroy(This->smallBlockDepotChain);

  for (i = 0; i < BLOCKCHAIN_CACHE_SIZE; i++)
    BlockChainStream_Destroy(This->blockChainCache[i]);

  for (i = 0; i < ARRAY_SIZE(This->locked_bytes); i++)
  {
    ULARGE_INTEGER offset, cb;
    cb.QuadPart = 1;
    if (This->locked_bytes[i] != 0)
    {
      offset.QuadPart = This->locked_bytes[i];
      StorageImpl_UnlockRegion(This, offset, cb, LOCK_ONLYONCE);
    }
  }

  if (This->lockBytes)
    ILockBytes_Release(This->lockBytes);
  HeapFree(GetProcessHeap(), 0, This);
}


static const StorageBaseImplVtbl StorageImpl_BaseVtbl =
{
  StorageImpl_Destroy,
  StorageImpl_Invalidate,
  StorageImpl_Flush,
  StorageImpl_GetFilename,
  StorageImpl_CreateDirEntry,
  StorageImpl_BaseWriteDirEntry,
  StorageImpl_BaseReadDirEntry,
  StorageImpl_DestroyDirEntry,
  StorageImpl_StreamReadAt,
  StorageImpl_StreamWriteAt,
  StorageImpl_StreamSetSize,
  StorageImpl_StreamLink,
  StorageImpl_GetTransactionSig,
  StorageImpl_SetTransactionSig,
  StorageImpl_LockTransaction,
  StorageImpl_UnlockTransaction
};


/*
 * Virtual function table for the IStorageBaseImpl class.
 */
static const IStorageVtbl StorageImpl_Vtbl =
{
    StorageBaseImpl_QueryInterface,
    StorageBaseImpl_AddRef,
    StorageBaseImpl_Release,
    StorageBaseImpl_CreateStream,
    StorageBaseImpl_OpenStream,
    StorageBaseImpl_CreateStorage,
    StorageBaseImpl_OpenStorage,
    StorageBaseImpl_CopyTo,
    StorageBaseImpl_MoveElementTo,
    StorageBaseImpl_Commit,
    StorageBaseImpl_Revert,
    StorageBaseImpl_EnumElements,
    StorageBaseImpl_DestroyElement,
    StorageBaseImpl_RenameElement,
    StorageBaseImpl_SetElementTimes,
    StorageBaseImpl_SetClass,
    StorageBaseImpl_SetStateBits,
    StorageBaseImpl_Stat
};

static HRESULT StorageImpl_Construct(
  HANDLE       hFile,
  LPCOLESTR    pwcsName,
  ILockBytes*  pLkbyt,
  DWORD        openFlags,
  BOOL         fileBased,
  BOOL         create,
  ULONG        sector_size,
  StorageImpl** result)
{
  StorageImpl* This;
  HRESULT hr = S_OK;
  STATSTG stat;

  if ( FAILED( validateSTGM(openFlags) ))
    return STG_E_INVALIDFLAG;

  This = HeapAlloc(GetProcessHeap(), 0, sizeof(StorageImpl));
  if (!This)
    return E_OUTOFMEMORY;

  memset(This, 0, sizeof(StorageImpl));

  list_init(&This->base.strmHead);

  list_init(&This->base.storageHead);

  This->base.IStorage_iface.lpVtbl = &StorageImpl_Vtbl;
  This->base.IPropertySetStorage_iface.lpVtbl = &IPropertySetStorage_Vtbl;
  This->base.IDirectWriterLock_iface.lpVtbl = &DirectWriterLockVtbl;
  This->base.baseVtbl = &StorageImpl_BaseVtbl;
  This->base.openFlags = (openFlags & ~STGM_CREATE);
  This->base.ref = 1;
  This->base.create = create;

  if (openFlags == (STGM_DIRECT_SWMR|STGM_READWRITE|STGM_SHARE_DENY_WRITE))
    This->base.lockingrole = SWMR_Writer;
  else if (openFlags == (STGM_DIRECT_SWMR|STGM_READ|STGM_SHARE_DENY_NONE))
    This->base.lockingrole = SWMR_Reader;
  else
    This->base.lockingrole = SWMR_None;

  This->base.reverted = FALSE;

  /*
   * Initialize the big block cache.
   */
  This->bigBlockSize   = sector_size;
  This->smallBlockSize = DEF_SMALL_BLOCK_SIZE;
  if (hFile)
    hr = FileLockBytesImpl_Construct(hFile, openFlags, pwcsName, &This->lockBytes);
  else
  {
    This->lockBytes = pLkbyt;
    ILockBytes_AddRef(pLkbyt);
  }

  if (SUCCEEDED(hr))
    hr = ILockBytes_Stat(This->lockBytes, &stat, STATFLAG_NONAME);

  if (SUCCEEDED(hr))
  {
    This->locks_supported = stat.grfLocksSupported;
    if (!hFile)
        /* Don't try to use wine-internal locking flag with custom ILockBytes */
        This->locks_supported &= ~WINE_LOCK_READ;

    hr = StorageImpl_GrabLocks(This, openFlags);
  }

  if (SUCCEEDED(hr))
    hr = StorageImpl_Refresh(This, TRUE, create);

  if (FAILED(hr))
  {
    IStorage_Release(&This->base.IStorage_iface);
    *result = NULL;
  }
  else
  {
    StorageImpl_Flush(&This->base);
    *result = This;
  }

  return hr;
}


/************************************************************************
 * StorageInternalImpl implementation
 ***********************************************************************/

static void StorageInternalImpl_Invalidate( StorageBaseImpl *base )
{
  StorageInternalImpl* This = (StorageInternalImpl*) base;

  if (!This->base.reverted)
  {
    TRACE("Storage invalidated (stg=%p)\n", This);

    This->base.reverted = TRUE;

    This->parentStorage = NULL;

    StorageBaseImpl_DeleteAll(&This->base);

    list_remove(&This->ParentListEntry);
  }
}

static void StorageInternalImpl_Destroy( StorageBaseImpl *iface)
{
  StorageInternalImpl* This = (StorageInternalImpl*) iface;

  StorageInternalImpl_Invalidate(&This->base);

  HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT StorageInternalImpl_Flush(StorageBaseImpl* iface)
{
  StorageInternalImpl* This = (StorageInternalImpl*) iface;

  return StorageBaseImpl_Flush(This->parentStorage);
}

static HRESULT StorageInternalImpl_GetFilename(StorageBaseImpl* iface, LPWSTR *result)
{
  StorageInternalImpl* This = (StorageInternalImpl*) iface;

  return StorageBaseImpl_GetFilename(This->parentStorage, result);
}

static HRESULT StorageInternalImpl_CreateDirEntry(StorageBaseImpl *base,
  const DirEntry *newData, DirRef *index)
{
  StorageInternalImpl* This = (StorageInternalImpl*) base;

  return StorageBaseImpl_CreateDirEntry(This->parentStorage,
    newData, index);
}

static HRESULT StorageInternalImpl_WriteDirEntry(StorageBaseImpl *base,
  DirRef index, const DirEntry *data)
{
  StorageInternalImpl* This = (StorageInternalImpl*) base;

  return StorageBaseImpl_WriteDirEntry(This->parentStorage,
    index, data);
}

static HRESULT StorageInternalImpl_ReadDirEntry(StorageBaseImpl *base,
  DirRef index, DirEntry *data)
{
  StorageInternalImpl* This = (StorageInternalImpl*) base;

  return StorageBaseImpl_ReadDirEntry(This->parentStorage,
    index, data);
}

static HRESULT StorageInternalImpl_DestroyDirEntry(StorageBaseImpl *base,
  DirRef index)
{
  StorageInternalImpl* This = (StorageInternalImpl*) base;

  return StorageBaseImpl_DestroyDirEntry(This->parentStorage,
    index);
}

static HRESULT StorageInternalImpl_StreamReadAt(StorageBaseImpl *base,
  DirRef index, ULARGE_INTEGER offset, ULONG size, void *buffer, ULONG *bytesRead)
{
  StorageInternalImpl* This = (StorageInternalImpl*) base;

  return StorageBaseImpl_StreamReadAt(This->parentStorage,
    index, offset, size, buffer, bytesRead);
}

static HRESULT StorageInternalImpl_StreamWriteAt(StorageBaseImpl *base,
  DirRef index, ULARGE_INTEGER offset, ULONG size, const void *buffer, ULONG *bytesWritten)
{
  StorageInternalImpl* This = (StorageInternalImpl*) base;

  return StorageBaseImpl_StreamWriteAt(This->parentStorage,
    index, offset, size, buffer, bytesWritten);
}

static HRESULT StorageInternalImpl_StreamSetSize(StorageBaseImpl *base,
  DirRef index, ULARGE_INTEGER newsize)
{
  StorageInternalImpl* This = (StorageInternalImpl*) base;

  return StorageBaseImpl_StreamSetSize(This->parentStorage,
    index, newsize);
}

static HRESULT StorageInternalImpl_StreamLink(StorageBaseImpl *base,
  DirRef dst, DirRef src)
{
  StorageInternalImpl* This = (StorageInternalImpl*) base;

  return StorageBaseImpl_StreamLink(This->parentStorage,
    dst, src);
}

static HRESULT StorageInternalImpl_GetTransactionSig(StorageBaseImpl *base,
  ULONG* result, BOOL refresh)
{
  return E_NOTIMPL;
}

static HRESULT StorageInternalImpl_SetTransactionSig(StorageBaseImpl *base,
  ULONG value)
{
  return E_NOTIMPL;
}

static HRESULT StorageInternalImpl_LockTransaction(StorageBaseImpl *base, BOOL write)
{
  return E_NOTIMPL;
}

static HRESULT StorageInternalImpl_UnlockTransaction(StorageBaseImpl *base, BOOL write)
{
  return E_NOTIMPL;
}

/******************************************************************************
**
** StorageInternalImpl_Commit
**
*/
static HRESULT WINAPI StorageInternalImpl_Commit(
  IStorage*            iface,
  DWORD                  grfCommitFlags)  /* [in] */
{
  StorageBaseImpl* This = impl_from_IStorage(iface);
  TRACE("%p, %#lx.\n", iface, grfCommitFlags);
  return StorageBaseImpl_Flush(This);
}

/******************************************************************************
**
** StorageInternalImpl_Revert
**
*/
static HRESULT WINAPI StorageInternalImpl_Revert(
  IStorage*            iface)
{
  FIXME("(%p): stub\n", iface);
  return S_OK;
}

/*
 * Virtual function table for the StorageInternalImpl class.
 */
static const IStorageVtbl StorageInternalImpl_Vtbl =
{
    StorageBaseImpl_QueryInterface,
    StorageBaseImpl_AddRef,
    StorageBaseImpl_Release,
    StorageBaseImpl_CreateStream,
    StorageBaseImpl_OpenStream,
    StorageBaseImpl_CreateStorage,
    StorageBaseImpl_OpenStorage,
    StorageBaseImpl_CopyTo,
    StorageBaseImpl_MoveElementTo,
    StorageInternalImpl_Commit,
    StorageInternalImpl_Revert,
    StorageBaseImpl_EnumElements,
    StorageBaseImpl_DestroyElement,
    StorageBaseImpl_RenameElement,
    StorageBaseImpl_SetElementTimes,
    StorageBaseImpl_SetClass,
    StorageBaseImpl_SetStateBits,
    StorageBaseImpl_Stat
};

static const StorageBaseImplVtbl StorageInternalImpl_BaseVtbl =
{
  StorageInternalImpl_Destroy,
  StorageInternalImpl_Invalidate,
  StorageInternalImpl_Flush,
  StorageInternalImpl_GetFilename,
  StorageInternalImpl_CreateDirEntry,
  StorageInternalImpl_WriteDirEntry,
  StorageInternalImpl_ReadDirEntry,
  StorageInternalImpl_DestroyDirEntry,
  StorageInternalImpl_StreamReadAt,
  StorageInternalImpl_StreamWriteAt,
  StorageInternalImpl_StreamSetSize,
  StorageInternalImpl_StreamLink,
  StorageInternalImpl_GetTransactionSig,
  StorageInternalImpl_SetTransactionSig,
  StorageInternalImpl_LockTransaction,
  StorageInternalImpl_UnlockTransaction
};

static StorageInternalImpl* StorageInternalImpl_Construct(
  StorageBaseImpl* parentStorage,
  DWORD        openFlags,
  DirRef       storageDirEntry)
{
  StorageInternalImpl* newStorage;

  newStorage = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(StorageInternalImpl));

  if (newStorage!=0)
  {
    list_init(&newStorage->base.strmHead);

    list_init(&newStorage->base.storageHead);

    /*
     * Initialize the virtual function table.
     */
    newStorage->base.IStorage_iface.lpVtbl = &StorageInternalImpl_Vtbl;
    newStorage->base.IPropertySetStorage_iface.lpVtbl = &IPropertySetStorage_Vtbl;
    newStorage->base.baseVtbl = &StorageInternalImpl_BaseVtbl;
    newStorage->base.openFlags = (openFlags & ~STGM_CREATE);

    newStorage->base.reverted = FALSE;

    newStorage->base.ref = 1;

    newStorage->parentStorage = parentStorage;

    /*
     * Keep a reference to the directory entry of this storage
     */
    newStorage->base.storageDirEntry = storageDirEntry;

    newStorage->base.create = FALSE;

    return newStorage;
  }

  return 0;
}


/************************************************************************
 * TransactedSnapshotImpl implementation
 ***********************************************************************/

static DirRef TransactedSnapshotImpl_FindFreeEntry(TransactedSnapshotImpl *This)
{
  DirRef result=This->firstFreeEntry;

  while (result < This->entries_size && This->entries[result].inuse)
    result++;

  if (result == This->entries_size)
  {
    ULONG new_size = This->entries_size * 2;
    TransactedDirEntry *new_entries;

    new_entries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TransactedDirEntry) * new_size);
    if (!new_entries) return DIRENTRY_NULL;

    memcpy(new_entries, This->entries, sizeof(TransactedDirEntry) * This->entries_size);
    HeapFree(GetProcessHeap(), 0, This->entries);

    This->entries = new_entries;
    This->entries_size = new_size;
  }

  This->entries[result].inuse = TRUE;

  This->firstFreeEntry = result+1;

  return result;
}

static DirRef TransactedSnapshotImpl_CreateStubEntry(
  TransactedSnapshotImpl *This, DirRef parentEntryRef)
{
  DirRef stubEntryRef;
  TransactedDirEntry *entry;

  stubEntryRef = TransactedSnapshotImpl_FindFreeEntry(This);

  if (stubEntryRef != DIRENTRY_NULL)
  {
    entry = &This->entries[stubEntryRef];

    entry->newTransactedParentEntry = entry->transactedParentEntry = parentEntryRef;

    entry->read = FALSE;
  }

  return stubEntryRef;
}

static HRESULT TransactedSnapshotImpl_EnsureReadEntry(
  TransactedSnapshotImpl *This, DirRef entry)
{
  HRESULT hr=S_OK;
  DirEntry data;

  if (!This->entries[entry].read)
  {
    hr = StorageBaseImpl_ReadDirEntry(This->transactedParent,
        This->entries[entry].transactedParentEntry,
        &data);

    if (SUCCEEDED(hr) && data.leftChild != DIRENTRY_NULL)
    {
      data.leftChild = TransactedSnapshotImpl_CreateStubEntry(This, data.leftChild);

      if (data.leftChild == DIRENTRY_NULL)
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr) && data.rightChild != DIRENTRY_NULL)
    {
      data.rightChild = TransactedSnapshotImpl_CreateStubEntry(This, data.rightChild);

      if (data.rightChild == DIRENTRY_NULL)
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr) && data.dirRootEntry != DIRENTRY_NULL)
    {
      data.dirRootEntry = TransactedSnapshotImpl_CreateStubEntry(This, data.dirRootEntry);

      if (data.dirRootEntry == DIRENTRY_NULL)
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
      memcpy(&This->entries[entry].data, &data, sizeof(DirEntry));
      This->entries[entry].read = TRUE;
    }
  }

  return hr;
}

static HRESULT TransactedSnapshotImpl_MakeStreamDirty(
  TransactedSnapshotImpl *This, DirRef entry)
{
  HRESULT hr = S_OK;

  if (!This->entries[entry].stream_dirty)
  {
    DirEntry new_entrydata;

    memset(&new_entrydata, 0, sizeof(DirEntry));
    new_entrydata.name[0] = 'S';
    new_entrydata.sizeOfNameString = 1;
    new_entrydata.stgType = STGTY_STREAM;
    new_entrydata.startingBlock = BLOCK_END_OF_CHAIN;
    new_entrydata.leftChild = DIRENTRY_NULL;
    new_entrydata.rightChild = DIRENTRY_NULL;
    new_entrydata.dirRootEntry = DIRENTRY_NULL;

    hr = StorageBaseImpl_CreateDirEntry(This->scratch, &new_entrydata,
      &This->entries[entry].stream_entry);

    if (SUCCEEDED(hr) && This->entries[entry].transactedParentEntry != DIRENTRY_NULL)
    {
      hr = StorageBaseImpl_CopyStream(
        This->scratch, This->entries[entry].stream_entry,
        This->transactedParent, This->entries[entry].transactedParentEntry);

      if (FAILED(hr))
        StorageBaseImpl_DestroyDirEntry(This->scratch, This->entries[entry].stream_entry);
    }

    if (SUCCEEDED(hr))
      This->entries[entry].stream_dirty = TRUE;

    if (This->entries[entry].transactedParentEntry != DIRENTRY_NULL)
    {
      /* Since this entry is modified, and we aren't using its stream data, we
       * no longer care about the original entry. */
      DirRef delete_ref;
      delete_ref = TransactedSnapshotImpl_CreateStubEntry(This, This->entries[entry].transactedParentEntry);

      if (delete_ref != DIRENTRY_NULL)
        This->entries[delete_ref].deleted = TRUE;

      This->entries[entry].transactedParentEntry = This->entries[entry].newTransactedParentEntry = DIRENTRY_NULL;
    }
  }

  return hr;
}

/* Find the first entry in a depth-first traversal. */
static DirRef TransactedSnapshotImpl_FindFirstChild(
  TransactedSnapshotImpl* This, DirRef parent)
{
  DirRef cursor, prev;
  TransactedDirEntry *entry;

  cursor = parent;
  entry = &This->entries[cursor];
  while (entry->read)
  {
    if (entry->data.leftChild != DIRENTRY_NULL)
    {
      prev = cursor;
      cursor = entry->data.leftChild;
      entry = &This->entries[cursor];
      entry->parent = prev;
    }
    else if (entry->data.rightChild != DIRENTRY_NULL)
    {
      prev = cursor;
      cursor = entry->data.rightChild;
      entry = &This->entries[cursor];
      entry->parent = prev;
    }
    else if (entry->data.dirRootEntry != DIRENTRY_NULL)
    {
      prev = cursor;
      cursor = entry->data.dirRootEntry;
      entry = &This->entries[cursor];
      entry->parent = prev;
    }
    else
      break;
  }

  return cursor;
}

/* Find the next entry in a depth-first traversal. */
static DirRef TransactedSnapshotImpl_FindNextChild(
  TransactedSnapshotImpl* This, DirRef current)
{
  DirRef parent;
  TransactedDirEntry *parent_entry;

  parent = This->entries[current].parent;
  parent_entry = &This->entries[parent];

  if (parent != DIRENTRY_NULL && parent_entry->data.dirRootEntry != current)
  {
    if (parent_entry->data.rightChild != current && parent_entry->data.rightChild != DIRENTRY_NULL)
    {
      This->entries[parent_entry->data.rightChild].parent = parent;
      return TransactedSnapshotImpl_FindFirstChild(This, parent_entry->data.rightChild);
    }

    if (parent_entry->data.dirRootEntry != DIRENTRY_NULL)
    {
      This->entries[parent_entry->data.dirRootEntry].parent = parent;
      return TransactedSnapshotImpl_FindFirstChild(This, parent_entry->data.dirRootEntry);
    }
  }

  return parent;
}

/* Return TRUE if we've made a copy of this entry for committing to the parent. */
static inline BOOL TransactedSnapshotImpl_MadeCopy(
  TransactedSnapshotImpl* This, DirRef entry)
{
  return entry != DIRENTRY_NULL &&
    This->entries[entry].newTransactedParentEntry != This->entries[entry].transactedParentEntry;
}

/* Destroy the entries created by CopyTree. */
static void TransactedSnapshotImpl_DestroyTemporaryCopy(
  TransactedSnapshotImpl* This, DirRef stop)
{
  DirRef cursor;
  TransactedDirEntry *entry;
  ULARGE_INTEGER zero;

  zero.QuadPart = 0;

  if (!This->entries[This->base.storageDirEntry].read)
    return;

  cursor = This->entries[This->base.storageDirEntry].data.dirRootEntry;

  if (cursor == DIRENTRY_NULL)
    return;

  cursor = TransactedSnapshotImpl_FindFirstChild(This, cursor);

  while (cursor != DIRENTRY_NULL && cursor != stop)
  {
    if (TransactedSnapshotImpl_MadeCopy(This, cursor))
    {
      entry = &This->entries[cursor];

      if (entry->stream_dirty)
        StorageBaseImpl_StreamSetSize(This->transactedParent,
          entry->newTransactedParentEntry, zero);

      StorageBaseImpl_DestroyDirEntry(This->transactedParent,
        entry->newTransactedParentEntry);

      entry->newTransactedParentEntry = entry->transactedParentEntry;
    }

    cursor = TransactedSnapshotImpl_FindNextChild(This, cursor);
  }
}

/* Make a copy of our edited tree that we can use in the parent. */
static HRESULT TransactedSnapshotImpl_CopyTree(TransactedSnapshotImpl* This)
{
  DirRef cursor;
  TransactedDirEntry *entry;
  HRESULT hr = S_OK;

  cursor = This->base.storageDirEntry;
  entry = &This->entries[cursor];
  entry->parent = DIRENTRY_NULL;
  entry->newTransactedParentEntry = entry->transactedParentEntry;

  if (entry->data.dirRootEntry == DIRENTRY_NULL)
    return S_OK;

  This->entries[entry->data.dirRootEntry].parent = DIRENTRY_NULL;

  cursor = TransactedSnapshotImpl_FindFirstChild(This, entry->data.dirRootEntry);
  entry = &This->entries[cursor];

  while (cursor != DIRENTRY_NULL)
  {
    /* Make a copy of this entry in the transacted parent. */
    if (!entry->read ||
        (!entry->dirty && !entry->stream_dirty &&
         !TransactedSnapshotImpl_MadeCopy(This, entry->data.leftChild) &&
         !TransactedSnapshotImpl_MadeCopy(This, entry->data.rightChild) &&
         !TransactedSnapshotImpl_MadeCopy(This, entry->data.dirRootEntry)))
      entry->newTransactedParentEntry = entry->transactedParentEntry;
    else
    {
      DirEntry newData;

      memcpy(&newData, &entry->data, sizeof(DirEntry));

      newData.size.QuadPart = 0;
      newData.startingBlock = BLOCK_END_OF_CHAIN;

      if (newData.leftChild != DIRENTRY_NULL)
        newData.leftChild = This->entries[newData.leftChild].newTransactedParentEntry;

      if (newData.rightChild != DIRENTRY_NULL)
        newData.rightChild = This->entries[newData.rightChild].newTransactedParentEntry;

      if (newData.dirRootEntry != DIRENTRY_NULL)
        newData.dirRootEntry = This->entries[newData.dirRootEntry].newTransactedParentEntry;

      hr = StorageBaseImpl_CreateDirEntry(This->transactedParent, &newData,
        &entry->newTransactedParentEntry);
      if (FAILED(hr))
      {
        TransactedSnapshotImpl_DestroyTemporaryCopy(This, cursor);
        return hr;
      }

      if (entry->stream_dirty)
      {
        hr = StorageBaseImpl_CopyStream(
          This->transactedParent, entry->newTransactedParentEntry,
          This->scratch, entry->stream_entry);
      }
      else if (entry->data.size.QuadPart)
      {
        hr = StorageBaseImpl_StreamLink(
          This->transactedParent, entry->newTransactedParentEntry,
          entry->transactedParentEntry);
      }

      if (FAILED(hr))
      {
        cursor = TransactedSnapshotImpl_FindNextChild(This, cursor);
        TransactedSnapshotImpl_DestroyTemporaryCopy(This, cursor);
        return hr;
      }
    }

    cursor = TransactedSnapshotImpl_FindNextChild(This, cursor);
    entry = &This->entries[cursor];
  }

  return hr;
}

static HRESULT WINAPI TransactedSnapshotImpl_Commit(
  IStorage*            iface,
  DWORD                  grfCommitFlags)  /* [in] */
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*)impl_from_IStorage(iface);
  TransactedDirEntry *root_entry;
  DirRef i, dir_root_ref;
  DirEntry data;
  ULARGE_INTEGER zero;
  HRESULT hr;
  ULONG transactionSig;

  zero.QuadPart = 0;

  TRACE("%p, %#lx.\n", iface, grfCommitFlags);

  /* Cannot commit a read-only transacted storage */
  if ( STGM_ACCESS_MODE( This->base.openFlags ) == STGM_READ )
    return STG_E_ACCESSDENIED;

  hr = StorageBaseImpl_LockTransaction(This->transactedParent, TRUE);
  if (hr == E_NOTIMPL) hr = S_OK;
  if (SUCCEEDED(hr))
  {
    hr = StorageBaseImpl_GetTransactionSig(This->transactedParent, &transactionSig, TRUE);
    if (SUCCEEDED(hr))
    {
      if (transactionSig != This->lastTransactionSig)
      {
        ERR("file was externally modified\n");
        hr = STG_E_NOTCURRENT;
      }

      if (SUCCEEDED(hr))
      {
        This->lastTransactionSig = transactionSig+1;
        hr = StorageBaseImpl_SetTransactionSig(This->transactedParent, This->lastTransactionSig);
      }
    }
    else if (hr == E_NOTIMPL)
      hr = S_OK;

    if (FAILED(hr)) goto end;

    /* To prevent data loss, we create the new structure in the file before we
     * delete the old one, so that in case of errors the old data is intact. We
     * shouldn't do this if STGC_OVERWRITE is set, but that flag should only be
     * needed in the rare situation where we have just enough free disk space to
     * overwrite the existing data. */

    root_entry = &This->entries[This->base.storageDirEntry];

    if (!root_entry->read)
      goto end;

    hr = TransactedSnapshotImpl_CopyTree(This);
    if (FAILED(hr)) goto end;

    if (root_entry->data.dirRootEntry == DIRENTRY_NULL)
      dir_root_ref = DIRENTRY_NULL;
    else
      dir_root_ref = This->entries[root_entry->data.dirRootEntry].newTransactedParentEntry;

    hr = StorageBaseImpl_Flush(This->transactedParent);

    /* Update the storage to use the new data in one step. */
    if (SUCCEEDED(hr))
      hr = StorageBaseImpl_ReadDirEntry(This->transactedParent,
        root_entry->transactedParentEntry, &data);

    if (SUCCEEDED(hr))
    {
      data.dirRootEntry = dir_root_ref;
      data.clsid = root_entry->data.clsid;
      data.ctime = root_entry->data.ctime;
      data.mtime = root_entry->data.mtime;

      hr = StorageBaseImpl_WriteDirEntry(This->transactedParent,
        root_entry->transactedParentEntry, &data);
    }

    /* Try to flush after updating the root storage, but if the flush fails, keep
     * going, on the theory that it'll either succeed later or the subsequent
     * writes will fail. */
    StorageBaseImpl_Flush(This->transactedParent);

    if (SUCCEEDED(hr))
    {
      /* Destroy the old now-orphaned data. */
      for (i=0; i<This->entries_size; i++)
      {
        TransactedDirEntry *entry = &This->entries[i];
        if (entry->inuse)
        {
          if (entry->deleted)
          {
            StorageBaseImpl_StreamSetSize(This->transactedParent,
              entry->transactedParentEntry, zero);
            StorageBaseImpl_DestroyDirEntry(This->transactedParent,
              entry->transactedParentEntry);
            memset(entry, 0, sizeof(TransactedDirEntry));
            This->firstFreeEntry = min(i, This->firstFreeEntry);
          }
          else if (entry->read && entry->transactedParentEntry != entry->newTransactedParentEntry)
          {
            if (entry->transactedParentEntry != DIRENTRY_NULL)
              StorageBaseImpl_DestroyDirEntry(This->transactedParent,
                entry->transactedParentEntry);
            if (entry->stream_dirty)
            {
              StorageBaseImpl_StreamSetSize(This->scratch, entry->stream_entry, zero);
              StorageBaseImpl_DestroyDirEntry(This->scratch, entry->stream_entry);
              entry->stream_dirty = FALSE;
            }
            entry->dirty = FALSE;
            entry->transactedParentEntry = entry->newTransactedParentEntry;
          }
        }
      }
    }
    else
    {
      TransactedSnapshotImpl_DestroyTemporaryCopy(This, DIRENTRY_NULL);
    }

    if (SUCCEEDED(hr))
      hr = StorageBaseImpl_Flush(This->transactedParent);
end:
    StorageBaseImpl_UnlockTransaction(This->transactedParent, TRUE);
  }

  TRACE("<-- %#lx\n", hr);
  return hr;
}

static HRESULT WINAPI TransactedSnapshotImpl_Revert(
  IStorage*            iface)
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*)impl_from_IStorage(iface);
  ULARGE_INTEGER zero;
  ULONG i;

  TRACE("(%p)\n", iface);

  /* Destroy the open objects. */
  StorageBaseImpl_DeleteAll(&This->base);

  /* Clear out the scratch file. */
  zero.QuadPart = 0;
  for (i=0; i<This->entries_size; i++)
  {
    if (This->entries[i].stream_dirty)
    {
      StorageBaseImpl_StreamSetSize(This->scratch, This->entries[i].stream_entry,
        zero);

      StorageBaseImpl_DestroyDirEntry(This->scratch, This->entries[i].stream_entry);
    }
  }

  memset(This->entries, 0, sizeof(TransactedDirEntry) * This->entries_size);

  This->firstFreeEntry = 0;
  This->base.storageDirEntry = TransactedSnapshotImpl_CreateStubEntry(This, This->transactedParent->storageDirEntry);

  return S_OK;
}

static void TransactedSnapshotImpl_Invalidate(StorageBaseImpl* This)
{
  if (!This->reverted)
  {
    TRACE("Storage invalidated (stg=%p)\n", This);

    This->reverted = TRUE;

    StorageBaseImpl_DeleteAll(This);
  }
}

static void TransactedSnapshotImpl_Destroy( StorageBaseImpl *iface)
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*) iface;

  IStorage_Revert(&This->base.IStorage_iface);
  IStorage_Release(&This->transactedParent->IStorage_iface);
  IStorage_Release(&This->scratch->IStorage_iface);
  HeapFree(GetProcessHeap(), 0, This->entries);
  HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT TransactedSnapshotImpl_Flush(StorageBaseImpl* iface)
{
  /* We only need to flush when committing. */
  return S_OK;
}

static HRESULT TransactedSnapshotImpl_GetFilename(StorageBaseImpl* iface, LPWSTR *result)
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*) iface;

  return StorageBaseImpl_GetFilename(This->transactedParent, result);
}

static HRESULT TransactedSnapshotImpl_CreateDirEntry(StorageBaseImpl *base,
  const DirEntry *newData, DirRef *index)
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*) base;
  DirRef new_ref;
  TransactedDirEntry *new_entry;

  new_ref = TransactedSnapshotImpl_FindFreeEntry(This);
  if (new_ref == DIRENTRY_NULL)
    return E_OUTOFMEMORY;

  new_entry = &This->entries[new_ref];

  new_entry->newTransactedParentEntry = new_entry->transactedParentEntry = DIRENTRY_NULL;
  new_entry->read = TRUE;
  new_entry->dirty = TRUE;
  memcpy(&new_entry->data, newData, sizeof(DirEntry));

  *index = new_ref;

  TRACE("%s l=%lx r=%lx d=%lx <-- %lx\n", debugstr_w(newData->name), newData->leftChild, newData->rightChild, newData->dirRootEntry, *index);

  return S_OK;
}

static HRESULT TransactedSnapshotImpl_WriteDirEntry(StorageBaseImpl *base,
  DirRef index, const DirEntry *data)
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*) base;
  HRESULT hr;

  TRACE("%lx %s l=%lx r=%lx d=%lx\n", index, debugstr_w(data->name), data->leftChild, data->rightChild, data->dirRootEntry);

  hr = TransactedSnapshotImpl_EnsureReadEntry(This, index);
  if (FAILED(hr))
  {
    TRACE("<-- %#lx\n", hr);
    return hr;
  }

  memcpy(&This->entries[index].data, data, sizeof(DirEntry));

  if (index != This->base.storageDirEntry)
  {
    This->entries[index].dirty = TRUE;

    if (data->size.QuadPart == 0 &&
        This->entries[index].transactedParentEntry != DIRENTRY_NULL)
    {
      /* Since this entry is modified, and we aren't using its stream data, we
       * no longer care about the original entry. */
      DirRef delete_ref;
      delete_ref = TransactedSnapshotImpl_CreateStubEntry(This, This->entries[index].transactedParentEntry);

      if (delete_ref != DIRENTRY_NULL)
        This->entries[delete_ref].deleted = TRUE;

      This->entries[index].transactedParentEntry = This->entries[index].newTransactedParentEntry = DIRENTRY_NULL;
    }
  }
  TRACE("<-- S_OK\n");
  return S_OK;
}

static HRESULT TransactedSnapshotImpl_ReadDirEntry(StorageBaseImpl *base,
  DirRef index, DirEntry *data)
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*) base;
  HRESULT hr;

  hr = TransactedSnapshotImpl_EnsureReadEntry(This, index);
  if (FAILED(hr))
  {
    TRACE("<-- %#lx\n", hr);
    return hr;
  }

  memcpy(data, &This->entries[index].data, sizeof(DirEntry));

  TRACE("%lx %s l=%lx r=%lx d=%lx\n", index, debugstr_w(data->name), data->leftChild, data->rightChild, data->dirRootEntry);

  return S_OK;
}

static HRESULT TransactedSnapshotImpl_DestroyDirEntry(StorageBaseImpl *base,
  DirRef index)
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*) base;

  if (This->entries[index].transactedParentEntry == DIRENTRY_NULL ||
      This->entries[index].data.size.QuadPart != 0)
  {
    /* If we deleted this entry while it has stream data. We must have left the
     * data because some other entry is using it, and we need to leave the
     * original entry alone. */
    memset(&This->entries[index], 0, sizeof(TransactedDirEntry));
    This->firstFreeEntry = min(index, This->firstFreeEntry);
  }
  else
  {
    This->entries[index].deleted = TRUE;
  }

  return S_OK;
}

static HRESULT TransactedSnapshotImpl_StreamReadAt(StorageBaseImpl *base,
  DirRef index, ULARGE_INTEGER offset, ULONG size, void *buffer, ULONG *bytesRead)
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*) base;

  if (This->entries[index].stream_dirty)
  {
    return StorageBaseImpl_StreamReadAt(This->scratch,
        This->entries[index].stream_entry, offset, size, buffer, bytesRead);
  }
  else if (This->entries[index].transactedParentEntry == DIRENTRY_NULL)
  {
    /* This stream doesn't live in the parent, and we haven't allocated storage
     * for it yet */
    *bytesRead = 0;
    return S_OK;
  }
  else
  {
    return StorageBaseImpl_StreamReadAt(This->transactedParent,
        This->entries[index].transactedParentEntry, offset, size, buffer, bytesRead);
  }
}

static HRESULT TransactedSnapshotImpl_StreamWriteAt(StorageBaseImpl *base,
  DirRef index, ULARGE_INTEGER offset, ULONG size, const void *buffer, ULONG *bytesWritten)
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*) base;
  HRESULT hr;

  hr = TransactedSnapshotImpl_EnsureReadEntry(This, index);
  if (FAILED(hr))
  {
    TRACE("<-- %#lx\n", hr);
    return hr;
  }

  hr = TransactedSnapshotImpl_MakeStreamDirty(This, index);
  if (FAILED(hr))
  {
    TRACE("<-- %#lx\n", hr);
    return hr;
  }

  hr = StorageBaseImpl_StreamWriteAt(This->scratch,
    This->entries[index].stream_entry, offset, size, buffer, bytesWritten);

  if (SUCCEEDED(hr) && size != 0)
    This->entries[index].data.size.QuadPart = max(
        This->entries[index].data.size.QuadPart,
        offset.QuadPart + size);

  TRACE("<-- %#lx\n", hr);
  return hr;
}

static HRESULT TransactedSnapshotImpl_StreamSetSize(StorageBaseImpl *base,
  DirRef index, ULARGE_INTEGER newsize)
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*) base;
  HRESULT hr;

  hr = TransactedSnapshotImpl_EnsureReadEntry(This, index);
  if (FAILED(hr))
  {
    TRACE("<-- %#lx\n", hr);
    return hr;
  }

  if (This->entries[index].data.size.QuadPart == newsize.QuadPart)
    return S_OK;

  if (newsize.QuadPart == 0)
  {
    /* Destroy any parent references or entries in the scratch file. */
    if (This->entries[index].stream_dirty)
    {
      ULARGE_INTEGER zero;
      zero.QuadPart = 0;
      StorageBaseImpl_StreamSetSize(This->scratch,
        This->entries[index].stream_entry, zero);
      StorageBaseImpl_DestroyDirEntry(This->scratch,
        This->entries[index].stream_entry);
      This->entries[index].stream_dirty = FALSE;
    }
    else if (This->entries[index].transactedParentEntry != DIRENTRY_NULL)
    {
      DirRef delete_ref;
      delete_ref = TransactedSnapshotImpl_CreateStubEntry(This, This->entries[index].transactedParentEntry);

      if (delete_ref != DIRENTRY_NULL)
        This->entries[delete_ref].deleted = TRUE;

      This->entries[index].transactedParentEntry = This->entries[index].newTransactedParentEntry = DIRENTRY_NULL;
    }
  }
  else
  {
    hr = TransactedSnapshotImpl_MakeStreamDirty(This, index);
    if (FAILED(hr)) return hr;

    hr = StorageBaseImpl_StreamSetSize(This->scratch,
      This->entries[index].stream_entry, newsize);
  }

  if (SUCCEEDED(hr))
    This->entries[index].data.size = newsize;

  TRACE("<-- %#lx\n", hr);
  return hr;
}

static HRESULT TransactedSnapshotImpl_StreamLink(StorageBaseImpl *base,
  DirRef dst, DirRef src)
{
  TransactedSnapshotImpl* This = (TransactedSnapshotImpl*) base;
  HRESULT hr;
  TransactedDirEntry *dst_entry, *src_entry;

  hr = TransactedSnapshotImpl_EnsureReadEntry(This, src);
  if (FAILED(hr))
  {
    TRACE("<-- %#lx\n", hr);
    return hr;
  }

  hr = TransactedSnapshotImpl_EnsureReadEntry(This, dst);
  if (FAILED(hr))
  {
    TRACE("<-- %#lx\n", hr);
    return hr;
  }

  dst_entry = &This->entries[dst];
  src_entry = &This->entries[src];

  dst_entry->stream_dirty = src_entry->stream_dirty;
  dst_entry->stream_entry = src_entry->stream_entry;
  dst_entry->transactedParentEntry = src_entry->transactedParentEntry;
  dst_entry->newTransactedParentEntry = src_entry->newTransactedParentEntry;
  dst_entry->data.size = src_entry->data.size;

  return S_OK;
}

static HRESULT TransactedSnapshotImpl_GetTransactionSig(StorageBaseImpl *base,
  ULONG* result, BOOL refresh)
{
  return E_NOTIMPL;
}

static HRESULT TransactedSnapshotImpl_SetTransactionSig(StorageBaseImpl *base,
  ULONG value)
{
  return E_NOTIMPL;
}

static HRESULT TransactedSnapshotImpl_LockTransaction(StorageBaseImpl *base, BOOL write)
{
  return E_NOTIMPL;
}

static HRESULT TransactedSnapshotImpl_UnlockTransaction(StorageBaseImpl *base, BOOL write)
{
  return E_NOTIMPL;
}

static const IStorageVtbl TransactedSnapshotImpl_Vtbl =
{
    StorageBaseImpl_QueryInterface,
    StorageBaseImpl_AddRef,
    StorageBaseImpl_Release,
    StorageBaseImpl_CreateStream,
    StorageBaseImpl_OpenStream,
    StorageBaseImpl_CreateStorage,
    StorageBaseImpl_OpenStorage,
    StorageBaseImpl_CopyTo,
    StorageBaseImpl_MoveElementTo,
    TransactedSnapshotImpl_Commit,
    TransactedSnapshotImpl_Revert,
    StorageBaseImpl_EnumElements,
    StorageBaseImpl_DestroyElement,
    StorageBaseImpl_RenameElement,
    StorageBaseImpl_SetElementTimes,
    StorageBaseImpl_SetClass,
    StorageBaseImpl_SetStateBits,
    StorageBaseImpl_Stat
};

static const StorageBaseImplVtbl TransactedSnapshotImpl_BaseVtbl =
{
  TransactedSnapshotImpl_Destroy,
  TransactedSnapshotImpl_Invalidate,
  TransactedSnapshotImpl_Flush,
  TransactedSnapshotImpl_GetFilename,
  TransactedSnapshotImpl_CreateDirEntry,
  TransactedSnapshotImpl_WriteDirEntry,
  TransactedSnapshotImpl_ReadDirEntry,
  TransactedSnapshotImpl_DestroyDirEntry,
  TransactedSnapshotImpl_StreamReadAt,
  TransactedSnapshotImpl_StreamWriteAt,
  TransactedSnapshotImpl_StreamSetSize,
  TransactedSnapshotImpl_StreamLink,
  TransactedSnapshotImpl_GetTransactionSig,
  TransactedSnapshotImpl_SetTransactionSig,
  TransactedSnapshotImpl_LockTransaction,
  TransactedSnapshotImpl_UnlockTransaction
};

static HRESULT TransactedSnapshotImpl_Construct(StorageBaseImpl *parentStorage,
  TransactedSnapshotImpl** result)
{
  HRESULT hr;

  *result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TransactedSnapshotImpl));
  if (*result)
  {
    IStorage *scratch;

    (*result)->base.IStorage_iface.lpVtbl = &TransactedSnapshotImpl_Vtbl;

    /* This is OK because the property set storage functions use the IStorage functions. */
    (*result)->base.IPropertySetStorage_iface.lpVtbl = parentStorage->IPropertySetStorage_iface.lpVtbl;
    (*result)->base.baseVtbl = &TransactedSnapshotImpl_BaseVtbl;

    list_init(&(*result)->base.strmHead);

    list_init(&(*result)->base.storageHead);

    (*result)->base.ref = 1;

    (*result)->base.openFlags = parentStorage->openFlags;

    /* This cannot fail, except with E_NOTIMPL in which case we don't care */
    StorageBaseImpl_GetTransactionSig(parentStorage, &(*result)->lastTransactionSig, FALSE);

    /* Create a new temporary storage to act as the scratch file. */
    hr = StgCreateDocfile(NULL, STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_DELETEONRELEASE,
        0, &scratch);
    (*result)->scratch = impl_from_IStorage(scratch);

    if (SUCCEEDED(hr))
    {
        ULONG num_entries = 20;

        (*result)->entries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TransactedDirEntry) * num_entries);
        (*result)->entries_size = num_entries;
        (*result)->firstFreeEntry = 0;

        if ((*result)->entries)
        {
            /* parentStorage already has 1 reference, which we take over here. */
            (*result)->transactedParent = parentStorage;

            parentStorage->transactedChild = &(*result)->base;

            (*result)->base.storageDirEntry = TransactedSnapshotImpl_CreateStubEntry(*result, parentStorage->storageDirEntry);
        }
        else
        {
            IStorage_Release(scratch);

            hr = E_OUTOFMEMORY;
        }
    }

    if (FAILED(hr)) HeapFree(GetProcessHeap(), 0, *result);

    return hr;
  }
  else
    return E_OUTOFMEMORY;
}


/************************************************************************
 * TransactedSharedImpl implementation
 ***********************************************************************/

static void TransactedSharedImpl_Invalidate(StorageBaseImpl* This)
{
  if (!This->reverted)
  {
    TRACE("Storage invalidated (stg=%p)\n", This);

    This->reverted = TRUE;

    StorageBaseImpl_DeleteAll(This);
  }
}

static void TransactedSharedImpl_Destroy( StorageBaseImpl *iface)
{
  TransactedSharedImpl* This = (TransactedSharedImpl*) iface;

  TransactedSharedImpl_Invalidate(&This->base);
  IStorage_Release(&This->transactedParent->IStorage_iface);
  IStorage_Release(&This->scratch->base.IStorage_iface);
  HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT TransactedSharedImpl_Flush(StorageBaseImpl* iface)
{
  /* We only need to flush when committing. */
  return S_OK;
}

static HRESULT TransactedSharedImpl_GetFilename(StorageBaseImpl* iface, LPWSTR *result)
{
  TransactedSharedImpl* This = (TransactedSharedImpl*) iface;

  return StorageBaseImpl_GetFilename(This->transactedParent, result);
}

static HRESULT TransactedSharedImpl_CreateDirEntry(StorageBaseImpl *base,
  const DirEntry *newData, DirRef *index)
{
  TransactedSharedImpl* This = (TransactedSharedImpl*) base;

  return StorageBaseImpl_CreateDirEntry(&This->scratch->base,
    newData, index);
}

static HRESULT TransactedSharedImpl_WriteDirEntry(StorageBaseImpl *base,
  DirRef index, const DirEntry *data)
{
  TransactedSharedImpl* This = (TransactedSharedImpl*) base;

  return StorageBaseImpl_WriteDirEntry(&This->scratch->base,
    index, data);
}

static HRESULT TransactedSharedImpl_ReadDirEntry(StorageBaseImpl *base,
  DirRef index, DirEntry *data)
{
  TransactedSharedImpl* This = (TransactedSharedImpl*) base;

  return StorageBaseImpl_ReadDirEntry(&This->scratch->base,
    index, data);
}

static HRESULT TransactedSharedImpl_DestroyDirEntry(StorageBaseImpl *base,
  DirRef index)
{
  TransactedSharedImpl* This = (TransactedSharedImpl*) base;

  return StorageBaseImpl_DestroyDirEntry(&This->scratch->base,
    index);
}

static HRESULT TransactedSharedImpl_StreamReadAt(StorageBaseImpl *base,
  DirRef index, ULARGE_INTEGER offset, ULONG size, void *buffer, ULONG *bytesRead)
{
  TransactedSharedImpl* This = (TransactedSharedImpl*) base;

  return StorageBaseImpl_StreamReadAt(&This->scratch->base,
    index, offset, size, buffer, bytesRead);
}

static HRESULT TransactedSharedImpl_StreamWriteAt(StorageBaseImpl *base,
  DirRef index, ULARGE_INTEGER offset, ULONG size, const void *buffer, ULONG *bytesWritten)
{
  TransactedSharedImpl* This = (TransactedSharedImpl*) base;

  return StorageBaseImpl_StreamWriteAt(&This->scratch->base,
    index, offset, size, buffer, bytesWritten);
}

static HRESULT TransactedSharedImpl_StreamSetSize(StorageBaseImpl *base,
  DirRef index, ULARGE_INTEGER newsize)
{
  TransactedSharedImpl* This = (TransactedSharedImpl*) base;

  return StorageBaseImpl_StreamSetSize(&This->scratch->base,
    index, newsize);
}

static HRESULT TransactedSharedImpl_StreamLink(StorageBaseImpl *base,
  DirRef dst, DirRef src)
{
  TransactedSharedImpl* This = (TransactedSharedImpl*) base;

  return StorageBaseImpl_StreamLink(&This->scratch->base,
    dst, src);
}

static HRESULT TransactedSharedImpl_GetTransactionSig(StorageBaseImpl *base,
  ULONG* result, BOOL refresh)
{
  return E_NOTIMPL;
}

static HRESULT TransactedSharedImpl_SetTransactionSig(StorageBaseImpl *base,
  ULONG value)
{
  return E_NOTIMPL;
}

static HRESULT TransactedSharedImpl_LockTransaction(StorageBaseImpl *base, BOOL write)
{
  return E_NOTIMPL;
}

static HRESULT TransactedSharedImpl_UnlockTransaction(StorageBaseImpl *base, BOOL write)
{
  return E_NOTIMPL;
}

static HRESULT WINAPI TransactedSharedImpl_Commit(
  IStorage*            iface,
  DWORD                  grfCommitFlags)  /* [in] */
{
  TransactedSharedImpl* This = (TransactedSharedImpl*)impl_from_IStorage(iface);
  DirRef new_storage_ref, prev_storage_ref;
  DirEntry src_data, dst_data;
  HRESULT hr;
  ULONG transactionSig;

  TRACE("%p, %#lx\n", iface, grfCommitFlags);

  /* Cannot commit a read-only transacted storage */
  if ( STGM_ACCESS_MODE( This->base.openFlags ) == STGM_READ )
    return STG_E_ACCESSDENIED;

  hr = StorageBaseImpl_LockTransaction(This->transactedParent, TRUE);
  if (hr == E_NOTIMPL) hr = S_OK;
  if (SUCCEEDED(hr))
  {
    hr = StorageBaseImpl_GetTransactionSig(This->transactedParent, &transactionSig, TRUE);
    if (SUCCEEDED(hr))
    {
      if ((grfCommitFlags & STGC_ONLYIFCURRENT) && transactionSig != This->lastTransactionSig)
        hr = STG_E_NOTCURRENT;

      if (SUCCEEDED(hr))
        hr = StorageBaseImpl_SetTransactionSig(This->transactedParent, transactionSig+1);
    }
    else if (hr == E_NOTIMPL)
      hr = S_OK;

    if (SUCCEEDED(hr))
      hr = StorageBaseImpl_ReadDirEntry(&This->scratch->base, This->scratch->base.storageDirEntry, &src_data);

    /* FIXME: If we're current, we should be able to copy only the changes in scratch. */
    if (SUCCEEDED(hr))
      hr = StorageBaseImpl_DupStorageTree(This->transactedParent, &new_storage_ref, &This->scratch->base, src_data.dirRootEntry);

    if (SUCCEEDED(hr))
      hr = StorageBaseImpl_Flush(This->transactedParent);

    if (SUCCEEDED(hr))
      hr = StorageBaseImpl_ReadDirEntry(This->transactedParent, This->transactedParent->storageDirEntry, &dst_data);

    if (SUCCEEDED(hr))
    {
      prev_storage_ref = dst_data.dirRootEntry;
      dst_data.dirRootEntry = new_storage_ref;
      dst_data.clsid = src_data.clsid;
      dst_data.ctime = src_data.ctime;
      dst_data.mtime = src_data.mtime;
      hr = StorageBaseImpl_WriteDirEntry(This->transactedParent, This->transactedParent->storageDirEntry, &dst_data);
    }

    if (SUCCEEDED(hr))
    {
      /* Try to flush after updating the root storage, but if the flush fails, keep
       * going, on the theory that it'll either succeed later or the subsequent
       * writes will fail. */
      StorageBaseImpl_Flush(This->transactedParent);

      hr = StorageBaseImpl_DeleteStorageTree(This->transactedParent, prev_storage_ref, TRUE);
    }

    if (SUCCEEDED(hr))
      hr = StorageBaseImpl_Flush(This->transactedParent);

    StorageBaseImpl_UnlockTransaction(This->transactedParent, TRUE);

    if (SUCCEEDED(hr))
      hr = IStorage_Commit(&This->scratch->base.IStorage_iface, STGC_DEFAULT);

    if (SUCCEEDED(hr))
    {
      This->lastTransactionSig = transactionSig+1;
    }
  }
  TRACE("<-- %#lx\n", hr);
  return hr;
}

static HRESULT WINAPI TransactedSharedImpl_Revert(
  IStorage*            iface)
{
  TransactedSharedImpl* This = (TransactedSharedImpl*)impl_from_IStorage(iface);

  TRACE("(%p)\n", iface);

  /* Destroy the open objects. */
  StorageBaseImpl_DeleteAll(&This->base);

  return IStorage_Revert(&This->scratch->base.IStorage_iface);
}

static const IStorageVtbl TransactedSharedImpl_Vtbl =
{
    StorageBaseImpl_QueryInterface,
    StorageBaseImpl_AddRef,
    StorageBaseImpl_Release,
    StorageBaseImpl_CreateStream,
    StorageBaseImpl_OpenStream,
    StorageBaseImpl_CreateStorage,
    StorageBaseImpl_OpenStorage,
    StorageBaseImpl_CopyTo,
    StorageBaseImpl_MoveElementTo,
    TransactedSharedImpl_Commit,
    TransactedSharedImpl_Revert,
    StorageBaseImpl_EnumElements,
    StorageBaseImpl_DestroyElement,
    StorageBaseImpl_RenameElement,
    StorageBaseImpl_SetElementTimes,
    StorageBaseImpl_SetClass,
    StorageBaseImpl_SetStateBits,
    StorageBaseImpl_Stat
};

static const StorageBaseImplVtbl TransactedSharedImpl_BaseVtbl =
{
  TransactedSharedImpl_Destroy,
  TransactedSharedImpl_Invalidate,
  TransactedSharedImpl_Flush,
  TransactedSharedImpl_GetFilename,
  TransactedSharedImpl_CreateDirEntry,
  TransactedSharedImpl_WriteDirEntry,
  TransactedSharedImpl_ReadDirEntry,
  TransactedSharedImpl_DestroyDirEntry,
  TransactedSharedImpl_StreamReadAt,
  TransactedSharedImpl_StreamWriteAt,
  TransactedSharedImpl_StreamSetSize,
  TransactedSharedImpl_StreamLink,
  TransactedSharedImpl_GetTransactionSig,
  TransactedSharedImpl_SetTransactionSig,
  TransactedSharedImpl_LockTransaction,
  TransactedSharedImpl_UnlockTransaction
};

static HRESULT TransactedSharedImpl_Construct(StorageBaseImpl *parentStorage,
  TransactedSharedImpl** result)
{
  HRESULT hr;

  *result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TransactedSharedImpl));
  if (*result)
  {
    IStorage *scratch;

    (*result)->base.IStorage_iface.lpVtbl = &TransactedSharedImpl_Vtbl;

    /* This is OK because the property set storage functions use the IStorage functions. */
    (*result)->base.IPropertySetStorage_iface.lpVtbl = parentStorage->IPropertySetStorage_iface.lpVtbl;
    (*result)->base.baseVtbl = &TransactedSharedImpl_BaseVtbl;

    list_init(&(*result)->base.strmHead);

    list_init(&(*result)->base.storageHead);

    (*result)->base.ref = 1;

    (*result)->base.openFlags = parentStorage->openFlags;

    hr = StorageBaseImpl_LockTransaction(parentStorage, FALSE);

    if (SUCCEEDED(hr))
    {
      STGOPTIONS stgo;

      /* This cannot fail, except with E_NOTIMPL in which case we don't care */
      StorageBaseImpl_GetTransactionSig(parentStorage, &(*result)->lastTransactionSig, FALSE);

      stgo.usVersion = 1;
      stgo.reserved = 0;
      stgo.ulSectorSize = 4096;
      stgo.pwcsTemplateFile = NULL;

      /* Create a new temporary storage to act as the scratch file. */
      hr = StgCreateStorageEx(NULL, STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_DELETEONRELEASE|STGM_TRANSACTED,
          STGFMT_DOCFILE, 0, &stgo, NULL, &IID_IStorage, (void**)&scratch);
      (*result)->scratch = (TransactedSnapshotImpl*)impl_from_IStorage(scratch);

      if (SUCCEEDED(hr))
      {
        hr = StorageBaseImpl_CopyStorageTree(&(*result)->scratch->base, (*result)->scratch->base.storageDirEntry,
          parentStorage, parentStorage->storageDirEntry);

        if (SUCCEEDED(hr))
        {
          hr = IStorage_Commit(scratch, STGC_DEFAULT);

          (*result)->base.storageDirEntry = (*result)->scratch->base.storageDirEntry;
          (*result)->transactedParent = parentStorage;
        }

        if (FAILED(hr))
          IStorage_Release(scratch);
      }

      StorageBaseImpl_UnlockTransaction(parentStorage, FALSE);
    }

    if (FAILED(hr)) HeapFree(GetProcessHeap(), 0, *result);

    return hr;
  }
  else
    return E_OUTOFMEMORY;
}

static HRESULT Storage_ConstructTransacted(StorageBaseImpl *parentStorage,
  BOOL toplevel, StorageBaseImpl** result)
{
  static int fixme_flags=STGM_NOSCRATCH|STGM_NOSNAPSHOT;

  if (parentStorage->openFlags & fixme_flags)
  {
    fixme_flags &= ~parentStorage->openFlags;
    FIXME("Unimplemented flags %lx\n", parentStorage->openFlags);
  }

  if (toplevel && !(parentStorage->openFlags & STGM_NOSNAPSHOT) &&
      STGM_SHARE_MODE(parentStorage->openFlags) != STGM_SHARE_DENY_WRITE &&
      STGM_SHARE_MODE(parentStorage->openFlags) != STGM_SHARE_EXCLUSIVE)
  {
    /* Need to create a temp file for the snapshot */
    return TransactedSharedImpl_Construct(parentStorage, (TransactedSharedImpl**)result);
  }

  return TransactedSnapshotImpl_Construct(parentStorage,
    (TransactedSnapshotImpl**)result);
}

static HRESULT Storage_Construct(
  HANDLE       hFile,
  LPCOLESTR    pwcsName,
  ILockBytes*  pLkbyt,
  DWORD        openFlags,
  BOOL         fileBased,
  BOOL         create,
  ULONG        sector_size,
  StorageBaseImpl** result)
{
  StorageImpl *newStorage;
  StorageBaseImpl *newTransactedStorage;
  HRESULT hr;

  hr = StorageImpl_Construct(hFile, pwcsName, pLkbyt, openFlags, fileBased, create, sector_size, &newStorage);
  if (FAILED(hr)) goto end;

  if (openFlags & STGM_TRANSACTED)
  {
    hr = Storage_ConstructTransacted(&newStorage->base, TRUE, &newTransactedStorage);
    if (FAILED(hr))
      IStorage_Release(&newStorage->base.IStorage_iface);
    else
      *result = newTransactedStorage;
  }
  else
    *result = &newStorage->base;

end:
  return hr;
}


/************************************************************************
 * StorageUtl helper functions
 ***********************************************************************/

void StorageUtl_ReadWord(const BYTE* buffer, ULONG offset, WORD* value)
{
  WORD tmp;

  memcpy(&tmp, buffer+offset, sizeof(WORD));
  *value = lendian16toh(tmp);
}

void StorageUtl_WriteWord(void *buffer, ULONG offset, WORD value)
{
    value = htole16(value);
    memcpy((BYTE *)buffer + offset, &value, sizeof(WORD));
}

void StorageUtl_ReadDWord(const BYTE* buffer, ULONG offset, DWORD* value)
{
  DWORD tmp;

  memcpy(&tmp, buffer+offset, sizeof(DWORD));
  *value = lendian32toh(tmp);
}

void StorageUtl_WriteDWord(void *buffer, ULONG offset, DWORD value)
{
    value = htole32(value);
    memcpy((BYTE *)buffer + offset, &value, sizeof(DWORD));
}

void StorageUtl_ReadULargeInteger(const BYTE* buffer, ULONG offset,
 ULARGE_INTEGER* value)
{
#ifdef WORDS_BIGENDIAN
    ULARGE_INTEGER tmp;

    memcpy(&tmp, buffer + offset, sizeof(ULARGE_INTEGER));
    value->u.LowPart = htole32(tmp.HighPart);
    value->u.HighPart = htole32(tmp.LowPart);
#else
    memcpy(value, buffer + offset, sizeof(ULARGE_INTEGER));
#endif
}

void StorageUtl_WriteULargeInteger(void *buffer, ULONG offset, const ULARGE_INTEGER *value)
{
#ifdef WORDS_BIGENDIAN
    ULARGE_INTEGER tmp;

    tmp.LowPart = htole32(value->u.HighPart);
    tmp.HighPart = htole32(value->u.LowPart);
    memcpy((BYTE *)buffer + offset, &tmp, sizeof(ULARGE_INTEGER));
#else
    memcpy((BYTE *)buffer + offset, value, sizeof(ULARGE_INTEGER));
#endif
}

void StorageUtl_ReadGUID(const BYTE* buffer, ULONG offset, GUID* value)
{
  StorageUtl_ReadDWord(buffer, offset, (DWORD *)&value->Data1);
  StorageUtl_ReadWord(buffer,  offset+4, &(value->Data2));
  StorageUtl_ReadWord(buffer,  offset+6, &(value->Data3));

  memcpy(value->Data4, buffer+offset+8, sizeof(value->Data4));
}

void StorageUtl_WriteGUID(void *buffer, ULONG offset, const GUID* value)
{
  StorageUtl_WriteDWord(buffer, offset,   value->Data1);
  StorageUtl_WriteWord(buffer,  offset+4, value->Data2);
  StorageUtl_WriteWord(buffer,  offset+6, value->Data3);

  memcpy((BYTE *)buffer + offset + 8, value->Data4, sizeof(value->Data4));
}

void StorageUtl_CopyDirEntryToSTATSTG(
  StorageBaseImpl*      storage,
  STATSTG*              destination,
  const DirEntry*       source,
  int                   statFlags)
{
  /*
   * The copy of the string occurs only when the flag is not set
   */
  if (!(statFlags & STATFLAG_NONAME) && source->stgType == STGTY_ROOT)
  {
    /* Use the filename for the root storage. */
    destination->pwcsName = 0;
    StorageBaseImpl_GetFilename(storage, &destination->pwcsName);
  }
  else if( ((statFlags & STATFLAG_NONAME) != 0) ||
       (source->name[0] == 0) )
  {
    destination->pwcsName = 0;
  }
  else
  {
    destination->pwcsName =
      CoTaskMemAlloc((lstrlenW(source->name)+1)*sizeof(WCHAR));

    lstrcpyW(destination->pwcsName, source->name);
  }

  switch (source->stgType)
  {
    case STGTY_STORAGE:
    case STGTY_ROOT:
      destination->type = STGTY_STORAGE;
      break;
    case STGTY_STREAM:
      destination->type = STGTY_STREAM;
      break;
    default:
      destination->type = STGTY_STREAM;
      break;
  }

  destination->cbSize            = source->size;
/*
  currentReturnStruct->mtime     = {0}; TODO
  currentReturnStruct->ctime     = {0};
  currentReturnStruct->atime     = {0};
*/
  destination->grfMode           = 0;
  destination->grfLocksSupported = 0;
  destination->clsid             = source->clsid;
  destination->grfStateBits      = 0;
  destination->reserved          = 0;
}


/************************************************************************
 * BlockChainStream implementation
 ***********************************************************************/

/******************************************************************************
 *      BlockChainStream_GetHeadOfChain
 *
 * Returns the head of this stream chain.
 * Some special chains don't have directory entries, their heads are kept in
 * This->headOfStreamPlaceHolder.
 *
 */
static ULONG BlockChainStream_GetHeadOfChain(BlockChainStream* This)
{
  DirEntry  chainEntry;
  HRESULT   hr;

  if (This->headOfStreamPlaceHolder != 0)
    return *(This->headOfStreamPlaceHolder);

  if (This->ownerDirEntry != DIRENTRY_NULL)
  {
    hr = StorageImpl_ReadDirEntry(
                      This->parentStorage,
                      This->ownerDirEntry,
                      &chainEntry);

    if (SUCCEEDED(hr) && chainEntry.startingBlock < BLOCK_FIRST_SPECIAL)
      return chainEntry.startingBlock;
  }

  return BLOCK_END_OF_CHAIN;
}

/* Read and save the index of all blocks in this stream. */
static HRESULT BlockChainStream_UpdateIndexCache(BlockChainStream* This)
{
  ULONG  next_sector, next_offset;
  HRESULT hr;
  struct BlockChainRun *last_run;

  if (This->indexCacheLen == 0)
  {
    last_run = NULL;
    next_offset = 0;
    next_sector = BlockChainStream_GetHeadOfChain(This);
  }
  else
  {
    last_run = &This->indexCache[This->indexCacheLen-1];
    next_offset = last_run->lastOffset+1;
    hr = StorageImpl_GetNextBlockInChain(This->parentStorage,
        last_run->firstSector + last_run->lastOffset - last_run->firstOffset,
        &next_sector);
    if (FAILED(hr)) return hr;
  }

  while (next_sector != BLOCK_END_OF_CHAIN)
  {
    if (!last_run || next_sector != last_run->firstSector + next_offset - last_run->firstOffset)
    {
      /* Add the current block to the cache. */
      if (This->indexCacheSize == 0)
      {
        This->indexCache = HeapAlloc(GetProcessHeap(), 0, sizeof(struct BlockChainRun)*16);
        if (!This->indexCache) return E_OUTOFMEMORY;
        This->indexCacheSize = 16;
      }
      else if (This->indexCacheSize == This->indexCacheLen)
      {
        struct BlockChainRun *new_cache;
        ULONG new_size;

        new_size = This->indexCacheSize * 2;
        new_cache = HeapAlloc(GetProcessHeap(), 0, sizeof(struct BlockChainRun)*new_size);
        if (!new_cache) return E_OUTOFMEMORY;
        memcpy(new_cache, This->indexCache, sizeof(struct BlockChainRun)*This->indexCacheLen);

        HeapFree(GetProcessHeap(), 0, This->indexCache);
        This->indexCache = new_cache;
        This->indexCacheSize = new_size;
      }

      This->indexCacheLen++;
      last_run = &This->indexCache[This->indexCacheLen-1];
      last_run->firstSector = next_sector;
      last_run->firstOffset = next_offset;
    }

    last_run->lastOffset = next_offset;

    /* Find the next block. */
    next_offset++;
    hr = StorageImpl_GetNextBlockInChain(This->parentStorage, next_sector, &next_sector);
    if (FAILED(hr)) return hr;
  }

  if (This->indexCacheLen)
  {
    This->tailIndex = last_run->firstSector + last_run->lastOffset - last_run->firstOffset;
    This->numBlocks = last_run->lastOffset+1;
  }
  else
  {
    This->tailIndex = BLOCK_END_OF_CHAIN;
    This->numBlocks = 0;
  }

  return S_OK;
}

/* Locate the nth block in this stream. */
static ULONG BlockChainStream_GetSectorOfOffset(BlockChainStream *This, ULONG offset)
{
  ULONG min_offset = 0, max_offset = This->numBlocks-1;
  ULONG min_run = 0, max_run = This->indexCacheLen-1;

  if (offset >= This->numBlocks)
    return BLOCK_END_OF_CHAIN;

  while (min_run < max_run)
  {
    ULONG run_to_check = min_run + (offset - min_offset) * (max_run - min_run) / (max_offset - min_offset);
    if (offset < This->indexCache[run_to_check].firstOffset)
    {
      max_offset = This->indexCache[run_to_check].firstOffset-1;
      max_run = run_to_check-1;
    }
    else if (offset > This->indexCache[run_to_check].lastOffset)
    {
      min_offset = This->indexCache[run_to_check].lastOffset+1;
      min_run = run_to_check+1;
    }
    else
      /* Block is in this run. */
      min_run = max_run = run_to_check;
  }

  return This->indexCache[min_run].firstSector + offset - This->indexCache[min_run].firstOffset;
}

static HRESULT BlockChainStream_GetBlockAtOffset(BlockChainStream *This,
    ULONG index, BlockChainBlock **block, ULONG *sector, BOOL create)
{
  BlockChainBlock *result=NULL;
  int i;

  for (i=0; i<2; i++)
    if (This->cachedBlocks[i].index == index)
    {
      *sector = This->cachedBlocks[i].sector;
      *block = &This->cachedBlocks[i];
      return S_OK;
    }

  *sector = BlockChainStream_GetSectorOfOffset(This, index);
  if (*sector == BLOCK_END_OF_CHAIN)
    return STG_E_DOCFILECORRUPT;

  if (create)
  {
    if (This->cachedBlocks[0].index == 0xffffffff)
      result = &This->cachedBlocks[0];
    else if (This->cachedBlocks[1].index == 0xffffffff)
      result = &This->cachedBlocks[1];
    else
    {
      result = &This->cachedBlocks[This->blockToEvict++];
      if (This->blockToEvict == 2)
        This->blockToEvict = 0;
    }

    if (result->dirty)
    {
      if (!StorageImpl_WriteBigBlock(This->parentStorage, result->sector, result->data))
        return STG_E_WRITEFAULT;
      result->dirty = FALSE;
    }

    result->read = FALSE;
    result->index = index;
    result->sector = *sector;
  }

  *block = result;
  return S_OK;
}

BlockChainStream* BlockChainStream_Construct(
  StorageImpl* parentStorage,
  ULONG*         headOfStreamPlaceHolder,
  DirRef         dirEntry)
{
  BlockChainStream* newStream;

  newStream = HeapAlloc(GetProcessHeap(), 0, sizeof(BlockChainStream));
  if(!newStream)
    return NULL;

  newStream->parentStorage           = parentStorage;
  newStream->headOfStreamPlaceHolder = headOfStreamPlaceHolder;
  newStream->ownerDirEntry           = dirEntry;
  newStream->indexCache              = NULL;
  newStream->indexCacheLen           = 0;
  newStream->indexCacheSize          = 0;
  newStream->cachedBlocks[0].index = 0xffffffff;
  newStream->cachedBlocks[0].dirty = FALSE;
  newStream->cachedBlocks[1].index = 0xffffffff;
  newStream->cachedBlocks[1].dirty = FALSE;
  newStream->blockToEvict          = 0;

  if (FAILED(BlockChainStream_UpdateIndexCache(newStream)))
  {
    HeapFree(GetProcessHeap(), 0, newStream->indexCache);
    HeapFree(GetProcessHeap(), 0, newStream);
    return NULL;
  }

  return newStream;
}

HRESULT BlockChainStream_Flush(BlockChainStream* This)
{
  int i;
  if (!This) return S_OK;
  for (i=0; i<2; i++)
  {
    if (This->cachedBlocks[i].dirty)
    {
      if (StorageImpl_WriteBigBlock(This->parentStorage, This->cachedBlocks[i].sector, This->cachedBlocks[i].data))
        This->cachedBlocks[i].dirty = FALSE;
      else
        return STG_E_WRITEFAULT;
    }
  }
  return S_OK;
}

void BlockChainStream_Destroy(BlockChainStream* This)
{
  if (This)
  {
    BlockChainStream_Flush(This);
    HeapFree(GetProcessHeap(), 0, This->indexCache);
  }
  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 *      BlockChainStream_Shrink
 *
 * Shrinks this chain in the big block depot.
 */
static BOOL BlockChainStream_Shrink(BlockChainStream* This,
                                    ULARGE_INTEGER    newSize)
{
  ULONG blockIndex;
  ULONG numBlocks;
  int i;

  /*
   * Figure out how many blocks are needed to contain the new size
   */
  numBlocks = newSize.QuadPart / This->parentStorage->bigBlockSize;

  if ((newSize.QuadPart % This->parentStorage->bigBlockSize) != 0)
    numBlocks++;

  if (numBlocks)
  {
    /*
     * Go to the new end of chain
     */
    blockIndex = BlockChainStream_GetSectorOfOffset(This, numBlocks-1);

    /* Mark the new end of chain */
    StorageImpl_SetNextBlockInChain(
      This->parentStorage,
      blockIndex,
      BLOCK_END_OF_CHAIN);

    This->tailIndex = blockIndex;
  }
  else
  {
    if (This->headOfStreamPlaceHolder != 0)
    {
      *This->headOfStreamPlaceHolder = BLOCK_END_OF_CHAIN;
    }
    else
    {
      DirEntry chainEntry;
      assert(This->ownerDirEntry != DIRENTRY_NULL);

      StorageImpl_ReadDirEntry(
        This->parentStorage,
        This->ownerDirEntry,
        &chainEntry);

      chainEntry.startingBlock = BLOCK_END_OF_CHAIN;

      StorageImpl_WriteDirEntry(
        This->parentStorage,
        This->ownerDirEntry,
        &chainEntry);
    }

    This->tailIndex = BLOCK_END_OF_CHAIN;
  }

  This->numBlocks = numBlocks;

  /*
   * Mark the extra blocks as free
   */
  while (This->indexCacheLen && This->indexCache[This->indexCacheLen-1].lastOffset >= numBlocks)
  {
    struct BlockChainRun *last_run = &This->indexCache[This->indexCacheLen-1];
    StorageImpl_FreeBigBlock(This->parentStorage,
      last_run->firstSector + last_run->lastOffset - last_run->firstOffset);
    if (last_run->lastOffset == last_run->firstOffset)
      This->indexCacheLen--;
    else
      last_run->lastOffset--;
  }

  /*
   * Reset the last accessed block cache.
   */
  for (i=0; i<2; i++)
  {
    if (This->cachedBlocks[i].index >= numBlocks)
    {
      This->cachedBlocks[i].index = 0xffffffff;
      This->cachedBlocks[i].dirty = FALSE;
    }
  }

  return TRUE;
}

/******************************************************************************
 *      BlockChainStream_Enlarge
 *
 * Grows this chain in the big block depot.
 */
static BOOL BlockChainStream_Enlarge(BlockChainStream* This,
                                     ULARGE_INTEGER    newSize)
{
  ULONG blockIndex, currentBlock;
  ULONG newNumBlocks;
  ULONG oldNumBlocks = 0;

  blockIndex = BlockChainStream_GetHeadOfChain(This);

  /*
   * Empty chain. Create the head.
   */
  if (blockIndex == BLOCK_END_OF_CHAIN)
  {
    blockIndex = StorageImpl_GetNextFreeBigBlock(This->parentStorage, 1);
    StorageImpl_SetNextBlockInChain(This->parentStorage,
                                      blockIndex,
                                      BLOCK_END_OF_CHAIN);

    if (This->headOfStreamPlaceHolder != 0)
    {
      *(This->headOfStreamPlaceHolder) = blockIndex;
    }
    else
    {
      DirEntry chainEntry;
      assert(This->ownerDirEntry != DIRENTRY_NULL);

      StorageImpl_ReadDirEntry(
        This->parentStorage,
        This->ownerDirEntry,
        &chainEntry);

      chainEntry.startingBlock = blockIndex;

      StorageImpl_WriteDirEntry(
        This->parentStorage,
        This->ownerDirEntry,
        &chainEntry);
    }

    This->tailIndex = blockIndex;
    This->numBlocks = 1;
  }

  /*
   * Figure out how many blocks are needed to contain this stream
   */
  newNumBlocks = newSize.QuadPart / This->parentStorage->bigBlockSize;

  if ((newSize.QuadPart % This->parentStorage->bigBlockSize) != 0)
    newNumBlocks++;

  /*
   * Go to the current end of chain
   */
  if (This->tailIndex == BLOCK_END_OF_CHAIN)
  {
    currentBlock = blockIndex;

    while (blockIndex != BLOCK_END_OF_CHAIN)
    {
      This->numBlocks++;
      currentBlock = blockIndex;

      if(FAILED(StorageImpl_GetNextBlockInChain(This->parentStorage, currentBlock,
						&blockIndex)))
	return FALSE;
    }

    This->tailIndex = currentBlock;
  }

  currentBlock = This->tailIndex;
  oldNumBlocks = This->numBlocks;

  /*
   * Add new blocks to the chain
   */
  if (oldNumBlocks < newNumBlocks)
  {
    while (oldNumBlocks < newNumBlocks)
    {
      blockIndex = StorageImpl_GetNextFreeBigBlock(This->parentStorage, newNumBlocks - oldNumBlocks);

      StorageImpl_SetNextBlockInChain(
	This->parentStorage,
	currentBlock,
	blockIndex);

      StorageImpl_SetNextBlockInChain(
        This->parentStorage,
	blockIndex,
	BLOCK_END_OF_CHAIN);

      currentBlock = blockIndex;
      oldNumBlocks++;
    }

    This->tailIndex = blockIndex;
    This->numBlocks = newNumBlocks;
  }

  if (FAILED(BlockChainStream_UpdateIndexCache(This)))
    return FALSE;

  return TRUE;
}


/******************************************************************************
 *      BlockChainStream_GetSize
 *
 * Returns the size of this chain.
 * Will return the block count if this chain doesn't have a directory entry.
 */
static ULARGE_INTEGER BlockChainStream_GetSize(BlockChainStream* This)
{
  DirEntry chainEntry;

  if(This->headOfStreamPlaceHolder == NULL)
  {
    /*
     * This chain has a directory entry so use the size value from there.
     */
    StorageImpl_ReadDirEntry(
      This->parentStorage,
      This->ownerDirEntry,
      &chainEntry);

    return chainEntry.size;
  }
  else
  {
    /*
     * this chain is a chain that does not have a directory entry, figure out the
     * size by making the product number of used blocks times the
     * size of them
     */
    ULARGE_INTEGER result;
    result.QuadPart =
      (ULONGLONG)BlockChainStream_GetCount(This) *
      This->parentStorage->bigBlockSize;

    return result;
  }
}

/******************************************************************************
 *      BlockChainStream_SetSize
 *
 * Sets the size of this stream. The big block depot will be updated.
 * The file will grow if we grow the chain.
 *
 * TODO: Free the actual blocks in the file when we shrink the chain.
 *       Currently, the blocks are still in the file. So the file size
 *       doesn't shrink even if we shrink streams.
 */
BOOL BlockChainStream_SetSize(
  BlockChainStream* This,
  ULARGE_INTEGER    newSize)
{
  ULARGE_INTEGER size = BlockChainStream_GetSize(This);

  if (newSize.QuadPart == size.QuadPart)
    return TRUE;

  if (newSize.QuadPart < size.QuadPart)
  {
    BlockChainStream_Shrink(This, newSize);
  }
  else
  {
    BlockChainStream_Enlarge(This, newSize);
  }

  return TRUE;
}

/******************************************************************************
 *      BlockChainStream_ReadAt
 *
 * Reads a specified number of bytes from this chain at the specified offset.
 * bytesRead may be NULL.
 * Failure will be returned if the specified number of bytes has not been read.
 */
HRESULT BlockChainStream_ReadAt(BlockChainStream* This,
  ULARGE_INTEGER offset,
  ULONG          size,
  void*          buffer,
  ULONG*         bytesRead)
{
  ULONG blockNoInSequence = offset.QuadPart / This->parentStorage->bigBlockSize;
  ULONG offsetInBlock     = offset.QuadPart % This->parentStorage->bigBlockSize;
  ULONG bytesToReadInBuffer;
  ULONG blockIndex;
  BYTE* bufferWalker;
  ULARGE_INTEGER stream_size;
  HRESULT hr;
  BlockChainBlock *cachedBlock;

  TRACE("%p, %li, %p, %lu, %p.\n",This, offset.LowPart, buffer, size, bytesRead);

  /*
   * Find the first block in the stream that contains part of the buffer.
   */
  blockIndex = BlockChainStream_GetSectorOfOffset(This, blockNoInSequence);

  *bytesRead   = 0;

  stream_size = BlockChainStream_GetSize(This);
  if (stream_size.QuadPart > offset.QuadPart)
    size = min(stream_size.QuadPart - offset.QuadPart, size);
  else
    return S_OK;

  /*
   * Start reading the buffer.
   */
  bufferWalker = buffer;

  while (size > 0)
  {
    ULARGE_INTEGER ulOffset;
    DWORD bytesReadAt;

    /*
     * Calculate how many bytes we can copy from this big block.
     */
    bytesToReadInBuffer =
      min(This->parentStorage->bigBlockSize - offsetInBlock, size);

    hr = BlockChainStream_GetBlockAtOffset(This, blockNoInSequence, &cachedBlock, &blockIndex, size == bytesToReadInBuffer);

    if (FAILED(hr))
      return hr;

    if (!cachedBlock)
    {
      /* Not in cache, and we're going to read past the end of the block. */
      ulOffset.QuadPart = StorageImpl_GetBigBlockOffset(This->parentStorage, blockIndex) +
                               offsetInBlock;

      StorageImpl_ReadAt(This->parentStorage,
           ulOffset,
           bufferWalker,
           bytesToReadInBuffer,
           &bytesReadAt);
    }
    else
    {
      if (!cachedBlock->read)
      {
        ULONG read;
        if (FAILED(StorageImpl_ReadBigBlock(This->parentStorage, cachedBlock->sector, cachedBlock->data, &read)) && !read)
          return STG_E_READFAULT;

        cachedBlock->read = TRUE;
      }

      memcpy(bufferWalker, cachedBlock->data+offsetInBlock, bytesToReadInBuffer);
      bytesReadAt = bytesToReadInBuffer;
    }

    blockNoInSequence++;
    bufferWalker += bytesReadAt;
    size         -= bytesReadAt;
    *bytesRead   += bytesReadAt;
    offsetInBlock = 0;  /* There is no offset on the next block */

    if (bytesToReadInBuffer != bytesReadAt)
        break;
  }

  return S_OK;
}

/******************************************************************************
 *      BlockChainStream_WriteAt
 *
 * Writes the specified number of bytes to this chain at the specified offset.
 * Will fail if not all specified number of bytes have been written.
 */
HRESULT BlockChainStream_WriteAt(BlockChainStream* This,
  ULARGE_INTEGER    offset,
  ULONG             size,
  const void*       buffer,
  ULONG*            bytesWritten)
{
  ULONG blockNoInSequence = offset.QuadPart / This->parentStorage->bigBlockSize;
  ULONG offsetInBlock     = offset.QuadPart % This->parentStorage->bigBlockSize;
  ULONG bytesToWrite;
  ULONG blockIndex;
  const BYTE* bufferWalker;
  HRESULT hr;
  BlockChainBlock *cachedBlock;

  *bytesWritten   = 0;
  bufferWalker = buffer;

  while (size > 0)
  {
    ULARGE_INTEGER ulOffset;
    DWORD bytesWrittenAt;

    /*
     * Calculate how many bytes we can copy to this big block.
     */
    bytesToWrite =
      min(This->parentStorage->bigBlockSize - offsetInBlock, size);

    hr = BlockChainStream_GetBlockAtOffset(This, blockNoInSequence, &cachedBlock, &blockIndex, size == bytesToWrite);

    /* BlockChainStream_SetSize should have already been called to ensure we have
     * enough blocks in the chain to write into */
    if (FAILED(hr))
    {
      ERR("not enough blocks in chain to write data\n");
      return hr;
    }

    if (!cachedBlock)
    {
      /* Not in cache, and we're going to write past the end of the block. */
      ulOffset.QuadPart = StorageImpl_GetBigBlockOffset(This->parentStorage, blockIndex) +
                               offsetInBlock;

      StorageImpl_WriteAt(This->parentStorage,
           ulOffset,
           bufferWalker,
           bytesToWrite,
           &bytesWrittenAt);
    }
    else
    {
      if (!cachedBlock->read && bytesToWrite != This->parentStorage->bigBlockSize)
      {
        ULONG read;
        if (FAILED(StorageImpl_ReadBigBlock(This->parentStorage, cachedBlock->sector, cachedBlock->data, &read)) && !read)
          return STG_E_READFAULT;
      }

      memcpy(cachedBlock->data+offsetInBlock, bufferWalker, bytesToWrite);
      bytesWrittenAt = bytesToWrite;
      cachedBlock->read = TRUE;
      cachedBlock->dirty = TRUE;
    }

    blockNoInSequence++;
    bufferWalker  += bytesWrittenAt;
    size          -= bytesWrittenAt;
    *bytesWritten += bytesWrittenAt;
    offsetInBlock  = 0;      /* There is no offset on the next block */

    if (bytesWrittenAt != bytesToWrite)
      break;
  }

  return (size == 0) ? S_OK : STG_E_WRITEFAULT;
}


/************************************************************************
 * SmallBlockChainStream implementation
 ***********************************************************************/

SmallBlockChainStream* SmallBlockChainStream_Construct(
  StorageImpl* parentStorage,
  ULONG*         headOfStreamPlaceHolder,
  DirRef         dirEntry)
{
  SmallBlockChainStream* newStream;

  newStream = HeapAlloc(GetProcessHeap(), 0, sizeof(SmallBlockChainStream));

  newStream->parentStorage      = parentStorage;
  newStream->headOfStreamPlaceHolder = headOfStreamPlaceHolder;
  newStream->ownerDirEntry      = dirEntry;

  return newStream;
}

void SmallBlockChainStream_Destroy(
  SmallBlockChainStream* This)
{
  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 *      SmallBlockChainStream_GetHeadOfChain
 *
 * Returns the head of this chain of small blocks.
 */
static ULONG SmallBlockChainStream_GetHeadOfChain(
  SmallBlockChainStream* This)
{
  DirEntry  chainEntry;
  HRESULT   hr;

  if (This->headOfStreamPlaceHolder != NULL)
    return *(This->headOfStreamPlaceHolder);

  if (This->ownerDirEntry)
  {
    hr = StorageImpl_ReadDirEntry(
                      This->parentStorage,
                      This->ownerDirEntry,
                      &chainEntry);

    if (SUCCEEDED(hr) && chainEntry.startingBlock < BLOCK_FIRST_SPECIAL)
      return chainEntry.startingBlock;
  }

  return BLOCK_END_OF_CHAIN;
}

/******************************************************************************
 *      SmallBlockChainStream_GetNextBlockInChain
 *
 * Returns the index of the next small block in this chain.
 *
 * Return Values:
 *    - BLOCK_END_OF_CHAIN: end of this chain
 *    - BLOCK_UNUSED: small block 'blockIndex' is free
 */
static HRESULT SmallBlockChainStream_GetNextBlockInChain(
  SmallBlockChainStream* This,
  ULONG                  blockIndex,
  ULONG*                 nextBlockInChain)
{
  ULARGE_INTEGER offsetOfBlockInDepot;
  DWORD  buffer;
  ULONG  bytesRead;
  HRESULT res;

  *nextBlockInChain = BLOCK_END_OF_CHAIN;

  offsetOfBlockInDepot.QuadPart  = (ULONGLONG)blockIndex * sizeof(ULONG);

  /*
   * Read those bytes in the buffer from the small block file.
   */
  res = BlockChainStream_ReadAt(
              This->parentStorage->smallBlockDepotChain,
              offsetOfBlockInDepot,
              sizeof(DWORD),
              &buffer,
              &bytesRead);

  if (SUCCEEDED(res) && bytesRead != sizeof(DWORD))
    res = STG_E_READFAULT;

  if (SUCCEEDED(res))
  {
    StorageUtl_ReadDWord((BYTE *)&buffer, 0, nextBlockInChain);
    return S_OK;
  }

  return res;
}

/******************************************************************************
 *       SmallBlockChainStream_SetNextBlockInChain
 *
 * Writes the index of the next block of the specified block in the small
 * block depot.
 * To set the end of chain use BLOCK_END_OF_CHAIN as nextBlock.
 * To flag a block as free use BLOCK_UNUSED as nextBlock.
 */
static void SmallBlockChainStream_SetNextBlockInChain(
  SmallBlockChainStream* This,
  ULONG                  blockIndex,
  ULONG                  nextBlock)
{
  ULARGE_INTEGER offsetOfBlockInDepot;
  DWORD  buffer;
  ULONG  bytesWritten;

  offsetOfBlockInDepot.QuadPart  = (ULONGLONG)blockIndex * sizeof(ULONG);

  StorageUtl_WriteDWord(&buffer, 0, nextBlock);

  /*
   * Read those bytes in the buffer from the small block file.
   */
  BlockChainStream_WriteAt(
    This->parentStorage->smallBlockDepotChain,
    offsetOfBlockInDepot,
    sizeof(DWORD),
    &buffer,
    &bytesWritten);
}

/******************************************************************************
 *      SmallBlockChainStream_FreeBlock
 *
 * Flag small block 'blockIndex' as free in the small block depot.
 */
static void SmallBlockChainStream_FreeBlock(
  SmallBlockChainStream* This,
  ULONG                  blockIndex)
{
  SmallBlockChainStream_SetNextBlockInChain(This, blockIndex, BLOCK_UNUSED);
}

/******************************************************************************
 *      SmallBlockChainStream_GetNextFreeBlock
 *
 * Returns the index of a free small block. The small block depot will be
 * enlarged if necessary. The small block chain will also be enlarged if
 * necessary.
 */
static ULONG SmallBlockChainStream_GetNextFreeBlock(
  SmallBlockChainStream* This)
{
  ULARGE_INTEGER offsetOfBlockInDepot;
  DWORD buffer;
  ULONG bytesRead;
  ULONG blockIndex = This->parentStorage->firstFreeSmallBlock;
  ULONG nextBlockIndex = BLOCK_END_OF_CHAIN;
  HRESULT res = S_OK;
  ULONG smallBlocksPerBigBlock;
  DirEntry rootEntry;
  ULONG blocksRequired;
  ULARGE_INTEGER old_size, size_required;

  offsetOfBlockInDepot.HighPart = 0;

  /*
   * Scan the small block depot for a free block
   */
  while (nextBlockIndex != BLOCK_UNUSED)
  {
    offsetOfBlockInDepot.QuadPart = (ULONGLONG)blockIndex * sizeof(ULONG);

    res = BlockChainStream_ReadAt(
                This->parentStorage->smallBlockDepotChain,
                offsetOfBlockInDepot,
                sizeof(DWORD),
                &buffer,
                &bytesRead);

    /*
     * If we run out of space for the small block depot, enlarge it
     */
    if (SUCCEEDED(res) && bytesRead == sizeof(DWORD))
    {
      StorageUtl_ReadDWord((BYTE *)&buffer, 0, &nextBlockIndex);

      if (nextBlockIndex != BLOCK_UNUSED)
        blockIndex++;
    }
    else
    {
      ULONG count =
        BlockChainStream_GetCount(This->parentStorage->smallBlockDepotChain);

      BYTE smallBlockDepot[MAX_BIG_BLOCK_SIZE];
      ULARGE_INTEGER newSize, offset;
      ULONG bytesWritten;

      newSize.QuadPart = (ULONGLONG)(count + 1) * This->parentStorage->bigBlockSize;
      BlockChainStream_Enlarge(This->parentStorage->smallBlockDepotChain, newSize);

      /*
       * Initialize all the small blocks to free
       */
      memset(smallBlockDepot, BLOCK_UNUSED, This->parentStorage->bigBlockSize);
      offset.QuadPart = (ULONGLONG)count * This->parentStorage->bigBlockSize;
      BlockChainStream_WriteAt(This->parentStorage->smallBlockDepotChain,
        offset, This->parentStorage->bigBlockSize, smallBlockDepot, &bytesWritten);

      StorageImpl_SaveFileHeader(This->parentStorage);
    }
  }

  This->parentStorage->firstFreeSmallBlock = blockIndex+1;

  smallBlocksPerBigBlock =
    This->parentStorage->bigBlockSize / This->parentStorage->smallBlockSize;

  /*
   * Verify if we have to allocate big blocks to contain small blocks
   */
  blocksRequired = (blockIndex / smallBlocksPerBigBlock) + 1;

  size_required.QuadPart = (ULONGLONG)blocksRequired * This->parentStorage->bigBlockSize;

  old_size = BlockChainStream_GetSize(This->parentStorage->smallBlockRootChain);

  if (size_required.QuadPart > old_size.QuadPart)
  {
    BlockChainStream_SetSize(
      This->parentStorage->smallBlockRootChain,
      size_required);

    StorageImpl_ReadDirEntry(
      This->parentStorage,
      This->parentStorage->base.storageDirEntry,
      &rootEntry);

    rootEntry.size = size_required;

    StorageImpl_WriteDirEntry(
      This->parentStorage,
      This->parentStorage->base.storageDirEntry,
      &rootEntry);
  }

  return blockIndex;
}

/******************************************************************************
 *      SmallBlockChainStream_ReadAt
 *
 * Reads a specified number of bytes from this chain at the specified offset.
 * bytesRead may be NULL.
 * Failure will be returned if the specified number of bytes has not been read.
 */
HRESULT SmallBlockChainStream_ReadAt(
  SmallBlockChainStream* This,
  ULARGE_INTEGER         offset,
  ULONG                  size,
  void*                  buffer,
  ULONG*                 bytesRead)
{
  HRESULT rc = S_OK;
  ULARGE_INTEGER offsetInBigBlockFile;
  ULONG blockNoInSequence =
    offset.LowPart / This->parentStorage->smallBlockSize;

  ULONG offsetInBlock = offset.LowPart % This->parentStorage->smallBlockSize;
  ULONG bytesToReadInBuffer;
  ULONG blockIndex;
  ULONG bytesReadFromBigBlockFile;
  BYTE* bufferWalker;
  ULARGE_INTEGER stream_size;

  /*
   * This should never happen on a small block file.
   */
  assert(offset.HighPart==0);

  *bytesRead   = 0;

  stream_size = SmallBlockChainStream_GetSize(This);
  if (stream_size.QuadPart > offset.QuadPart)
    size = min(stream_size.QuadPart - offset.QuadPart, size);
  else
    return S_OK;

  /*
   * Find the first block in the stream that contains part of the buffer.
   */
  blockIndex = SmallBlockChainStream_GetHeadOfChain(This);

  while ( (blockNoInSequence > 0) &&  (blockIndex != BLOCK_END_OF_CHAIN))
  {
    rc = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex, &blockIndex);
    if(FAILED(rc))
      return rc;
    blockNoInSequence--;
  }

  /*
   * Start reading the buffer.
   */
  bufferWalker = buffer;

  while ( (size > 0) && (blockIndex != BLOCK_END_OF_CHAIN) )
  {
    /*
     * Calculate how many bytes we can copy from this small block.
     */
    bytesToReadInBuffer =
      min(This->parentStorage->smallBlockSize - offsetInBlock, size);

    /*
     * Calculate the offset of the small block in the small block file.
     */
    offsetInBigBlockFile.QuadPart   =
      (ULONGLONG)blockIndex * This->parentStorage->smallBlockSize;

    offsetInBigBlockFile.QuadPart  += offsetInBlock;

    /*
     * Read those bytes in the buffer from the small block file.
     * The small block has already been identified so it shouldn't fail
     * unless the file is corrupt.
     */
    rc = BlockChainStream_ReadAt(This->parentStorage->smallBlockRootChain,
      offsetInBigBlockFile,
      bytesToReadInBuffer,
      bufferWalker,
      &bytesReadFromBigBlockFile);

    if (FAILED(rc))
      return rc;

    if (!bytesReadFromBigBlockFile)
      return STG_E_DOCFILECORRUPT;

    /*
     * Step to the next big block.
     */
    rc = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex, &blockIndex);
    if(FAILED(rc))
      return STG_E_DOCFILECORRUPT;

    bufferWalker += bytesReadFromBigBlockFile;
    size         -= bytesReadFromBigBlockFile;
    *bytesRead   += bytesReadFromBigBlockFile;
    offsetInBlock = (offsetInBlock + bytesReadFromBigBlockFile) % This->parentStorage->smallBlockSize;
  }

  return S_OK;
}

/******************************************************************************
 *       SmallBlockChainStream_WriteAt
 *
 * Writes the specified number of bytes to this chain at the specified offset.
 * Will fail if not all specified number of bytes have been written.
 */
HRESULT SmallBlockChainStream_WriteAt(
  SmallBlockChainStream* This,
  ULARGE_INTEGER offset,
  ULONG          size,
  const void*    buffer,
  ULONG*         bytesWritten)
{
  ULARGE_INTEGER offsetInBigBlockFile;
  ULONG blockNoInSequence =
    offset.LowPart / This->parentStorage->smallBlockSize;

  ULONG offsetInBlock = offset.LowPart % This->parentStorage->smallBlockSize;
  ULONG bytesToWriteInBuffer;
  ULONG blockIndex;
  ULONG bytesWrittenToBigBlockFile;
  const BYTE* bufferWalker;
  HRESULT res;

  /*
   * This should never happen on a small block file.
   */
  assert(offset.HighPart==0);

  /*
   * Find the first block in the stream that contains part of the buffer.
   */
  blockIndex = SmallBlockChainStream_GetHeadOfChain(This);

  while ( (blockNoInSequence > 0) &&  (blockIndex != BLOCK_END_OF_CHAIN))
  {
    if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This, blockIndex, &blockIndex)))
      return STG_E_DOCFILECORRUPT;
    blockNoInSequence--;
  }

  /*
   * Start writing the buffer.
   */
  *bytesWritten   = 0;
  bufferWalker = buffer;
  while ( (size > 0) && (blockIndex != BLOCK_END_OF_CHAIN) )
  {
    /*
     * Calculate how many bytes we can copy to this small block.
     */
    bytesToWriteInBuffer =
      min(This->parentStorage->smallBlockSize - offsetInBlock, size);

    /*
     * Calculate the offset of the small block in the small block file.
     */
    offsetInBigBlockFile.QuadPart   =
      (ULONGLONG)blockIndex * This->parentStorage->smallBlockSize;

    offsetInBigBlockFile.QuadPart  += offsetInBlock;

    /*
     * Write those bytes in the buffer to the small block file.
     */
    res = BlockChainStream_WriteAt(
      This->parentStorage->smallBlockRootChain,
      offsetInBigBlockFile,
      bytesToWriteInBuffer,
      bufferWalker,
      &bytesWrittenToBigBlockFile);
    if (FAILED(res))
      return res;

    /*
     * Step to the next big block.
     */
    res = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex, &blockIndex);
    if (FAILED(res))
      return res;
    bufferWalker  += bytesWrittenToBigBlockFile;
    size          -= bytesWrittenToBigBlockFile;
    *bytesWritten += bytesWrittenToBigBlockFile;
    offsetInBlock  = (offsetInBlock + bytesWrittenToBigBlockFile) % This->parentStorage->smallBlockSize;
  }

  return (size == 0) ? S_OK : STG_E_WRITEFAULT;
}

/******************************************************************************
 *       SmallBlockChainStream_Shrink
 *
 * Shrinks this chain in the small block depot.
 */
static BOOL SmallBlockChainStream_Shrink(
  SmallBlockChainStream* This,
  ULARGE_INTEGER newSize)
{
  ULONG blockIndex, extraBlock;
  ULONG numBlocks;
  ULONG count = 0;

  numBlocks = newSize.LowPart / This->parentStorage->smallBlockSize;

  if ((newSize.LowPart % This->parentStorage->smallBlockSize) != 0)
    numBlocks++;

  blockIndex = SmallBlockChainStream_GetHeadOfChain(This);

  /*
   * Go to the new end of chain
   */
  while (count < numBlocks)
  {
    if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This, blockIndex,
							&blockIndex)))
      return FALSE;
    count++;
  }

  /*
   * If the count is 0, we have a special case, the head of the chain was
   * just freed.
   */
  if (count == 0)
  {
    DirEntry chainEntry;

    StorageImpl_ReadDirEntry(This->parentStorage,
			     This->ownerDirEntry,
			     &chainEntry);

    chainEntry.startingBlock = BLOCK_END_OF_CHAIN;

    StorageImpl_WriteDirEntry(This->parentStorage,
			      This->ownerDirEntry,
			      &chainEntry);

    /*
     * We start freeing the chain at the head block.
     */
    extraBlock = blockIndex;
  }
  else
  {
    /* Get the next block before marking the new end */
    if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This, blockIndex,
							&extraBlock)))
      return FALSE;

    /* Mark the new end of chain */
    SmallBlockChainStream_SetNextBlockInChain(
      This,
      blockIndex,
      BLOCK_END_OF_CHAIN);
  }

  /*
   * Mark the extra blocks as free
   */
  while (extraBlock != BLOCK_END_OF_CHAIN)
  {
    if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This, extraBlock,
							&blockIndex)))
      return FALSE;
    SmallBlockChainStream_FreeBlock(This, extraBlock);
    This->parentStorage->firstFreeSmallBlock = min(This->parentStorage->firstFreeSmallBlock, extraBlock);
    extraBlock = blockIndex;
  }

  return TRUE;
}

/******************************************************************************
 *      SmallBlockChainStream_Enlarge
 *
 * Grows this chain in the small block depot.
 */
static BOOL SmallBlockChainStream_Enlarge(
  SmallBlockChainStream* This,
  ULARGE_INTEGER newSize)
{
  ULONG blockIndex, currentBlock;
  ULONG newNumBlocks;
  ULONG oldNumBlocks = 0;

  blockIndex = SmallBlockChainStream_GetHeadOfChain(This);

  /*
   * Empty chain. Create the head.
   */
  if (blockIndex == BLOCK_END_OF_CHAIN)
  {
    blockIndex = SmallBlockChainStream_GetNextFreeBlock(This);
    SmallBlockChainStream_SetNextBlockInChain(
        This,
        blockIndex,
        BLOCK_END_OF_CHAIN);

    if (This->headOfStreamPlaceHolder != NULL)
    {
      *(This->headOfStreamPlaceHolder) = blockIndex;
    }
    else
    {
      DirEntry chainEntry;

      StorageImpl_ReadDirEntry(This->parentStorage, This->ownerDirEntry,
                                   &chainEntry);

      chainEntry.startingBlock = blockIndex;

      StorageImpl_WriteDirEntry(This->parentStorage, This->ownerDirEntry,
                                  &chainEntry);
    }
  }

  currentBlock = blockIndex;

  /*
   * Figure out how many blocks are needed to contain this stream
   */
  newNumBlocks = newSize.LowPart / This->parentStorage->smallBlockSize;

  if ((newSize.LowPart % This->parentStorage->smallBlockSize) != 0)
    newNumBlocks++;

  /*
   * Go to the current end of chain
   */
  while (blockIndex != BLOCK_END_OF_CHAIN)
  {
    oldNumBlocks++;
    currentBlock = blockIndex;
    if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This, currentBlock, &blockIndex)))
      return FALSE;
  }

  /*
   * Add new blocks to the chain
   */
  while (oldNumBlocks < newNumBlocks)
  {
    blockIndex = SmallBlockChainStream_GetNextFreeBlock(This);
    SmallBlockChainStream_SetNextBlockInChain(This, currentBlock, blockIndex);

    SmallBlockChainStream_SetNextBlockInChain(
      This,
      blockIndex,
      BLOCK_END_OF_CHAIN);

    currentBlock = blockIndex;
    oldNumBlocks++;
  }

  return TRUE;
}

/******************************************************************************
 *      SmallBlockChainStream_SetSize
 *
 * Sets the size of this stream.
 * The file will grow if we grow the chain.
 *
 * TODO: Free the actual blocks in the file when we shrink the chain.
 *       Currently, the blocks are still in the file. So the file size
 *       doesn't shrink even if we shrink streams.
 */
BOOL SmallBlockChainStream_SetSize(
                SmallBlockChainStream* This,
                ULARGE_INTEGER    newSize)
{
  ULARGE_INTEGER size = SmallBlockChainStream_GetSize(This);

  if (newSize.LowPart == size.LowPart)
    return TRUE;

  if (newSize.LowPart < size.LowPart)
  {
    SmallBlockChainStream_Shrink(This, newSize);
  }
  else
  {
    SmallBlockChainStream_Enlarge(This, newSize);
  }

  return TRUE;
}

/******************************************************************************
 *       SmallBlockChainStream_GetCount
 *
 * Returns the number of small blocks that comprises this chain.
 * This is not the size of the stream as the last block may not be full!
 *
 */
static ULONG SmallBlockChainStream_GetCount(SmallBlockChainStream* This)
{
    ULONG blockIndex;
    ULONG count = 0;

    blockIndex = SmallBlockChainStream_GetHeadOfChain(This);

    while(blockIndex != BLOCK_END_OF_CHAIN)
    {
        count++;

        if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This,
                        blockIndex, &blockIndex)))
            return 0;
    }

    return count;
}

/******************************************************************************
 *      SmallBlockChainStream_GetSize
 *
 * Returns the size of this chain.
 */
static ULARGE_INTEGER SmallBlockChainStream_GetSize(SmallBlockChainStream* This)
{
  DirEntry chainEntry;

  if(This->headOfStreamPlaceHolder != NULL)
  {
    ULARGE_INTEGER result;
    result.HighPart = 0;

    result.LowPart = SmallBlockChainStream_GetCount(This) *
        This->parentStorage->smallBlockSize;

    return result;
  }

  StorageImpl_ReadDirEntry(
    This->parentStorage,
    This->ownerDirEntry,
    &chainEntry);

  return chainEntry.size;
}


/************************************************************************
 * Miscellaneous storage functions
 ***********************************************************************/

static HRESULT create_storagefile(
  LPCOLESTR pwcsName,
  DWORD       grfMode,
  DWORD       grfAttrs,
  STGOPTIONS* pStgOptions,
  REFIID      riid,
  void**      ppstgOpen)
{
  StorageBaseImpl* newStorage = 0;
  HANDLE       hFile      = INVALID_HANDLE_VALUE;
  HRESULT        hr         = STG_E_INVALIDFLAG;
  DWORD          shareMode;
  DWORD          accessMode;
  DWORD          creationMode;
  DWORD          fileAttributes;
  WCHAR          tempFileName[MAX_PATH];

  if (ppstgOpen == 0)
    return STG_E_INVALIDPOINTER;

  if (pStgOptions->ulSectorSize != MIN_BIG_BLOCK_SIZE && pStgOptions->ulSectorSize != MAX_BIG_BLOCK_SIZE)
    return STG_E_INVALIDPARAMETER;

  /* if no share mode given then DENY_NONE is the default */
  if (STGM_SHARE_MODE(grfMode) == 0)
      grfMode |= STGM_SHARE_DENY_NONE;

  if ( FAILED( validateSTGM(grfMode) ))
    goto end;

  /* StgCreateDocFile seems to refuse readonly access, despite MSDN */
  switch(STGM_ACCESS_MODE(grfMode))
  {
  case STGM_WRITE:
  case STGM_READWRITE:
    break;
  default:
    goto end;
  }

  /* in direct mode, can only use SHARE_EXCLUSIVE */
  if (!(grfMode & STGM_TRANSACTED) && (STGM_SHARE_MODE(grfMode) != STGM_SHARE_EXCLUSIVE))
    goto end;

  /* but in transacted mode, any share mode is valid */

  /*
   * Generate a unique name.
   */
  if (pwcsName == 0)
  {
    WCHAR tempPath[MAX_PATH];

    memset(tempPath, 0, sizeof(tempPath));
    memset(tempFileName, 0, sizeof(tempFileName));

    if ((GetTempPathW(MAX_PATH, tempPath)) == 0 )
      tempPath[0] = '.';

    if (GetTempFileNameW(tempPath, L"STO", 0, tempFileName) != 0)
      pwcsName = tempFileName;
    else
    {
      hr = STG_E_INSUFFICIENTMEMORY;
      goto end;
    }

    creationMode = TRUNCATE_EXISTING;
  }
  else
  {
    creationMode = GetCreationModeFromSTGM(grfMode);
  }

  /*
   * Interpret the STGM value grfMode
   */
  shareMode    = GetShareModeFromSTGM(grfMode);
  accessMode   = GetAccessModeFromSTGM(grfMode);

  if (grfMode & STGM_DELETEONRELEASE)
    fileAttributes = FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_DELETE_ON_CLOSE;
  else
    fileAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;

  *ppstgOpen = 0;

  hFile = CreateFileW(pwcsName,
                        accessMode,
                        shareMode,
                        NULL,
                        creationMode,
                        fileAttributes,
                        0);

  if (hFile == INVALID_HANDLE_VALUE)
  {
    if(GetLastError() == ERROR_FILE_EXISTS)
      hr = STG_E_FILEALREADYEXISTS;
    else
      hr = E_FAIL;
    goto end;
  }

  /*
   * Allocate and initialize the new IStorage object.
   */
  hr = Storage_Construct(
         hFile,
        pwcsName,
         NULL,
         grfMode,
         TRUE,
         TRUE,
         pStgOptions->ulSectorSize,
         &newStorage);

  if (FAILED(hr))
  {
    goto end;
  }

  hr = IStorage_QueryInterface(&newStorage->IStorage_iface, riid, ppstgOpen);
  IStorage_Release(&newStorage->IStorage_iface);

end:
  TRACE("<-- %p  r = %#lx\n", *ppstgOpen, hr);

  return hr;
}

/******************************************************************************
 *    StgCreateDocfile  [OLE32.@]
 * Creates a new compound file storage object
 *
 * PARAMS
 *  pwcsName  [ I] Unicode string with filename (can be relative or NULL)
 *  grfMode   [ I] Access mode for opening the new storage object (see STGM_ constants)
 *  reserved  [ ?] unused?, usually 0
 *  ppstgOpen [IO] A pointer to IStorage pointer to the new object
 *
 * RETURNS
 *  S_OK if the file was successfully created
 *  some STG_E_ value if error
 * NOTES
 *  if pwcsName is NULL, create file with new unique name
 *  the function can returns
 *  STG_S_CONVERTED if the specified file was successfully converted to storage format
 *  (unrealized now)
 */
HRESULT WINAPI StgCreateDocfile(
  LPCOLESTR pwcsName,
  DWORD       grfMode,
  DWORD       reserved,
  IStorage  **ppstgOpen)
{
  STGOPTIONS stgoptions = {1, 0, 512};

  TRACE("%s, %#lx, %ld, %p.\n", debugstr_w(pwcsName), grfMode, reserved, ppstgOpen);

  if (ppstgOpen == 0)
    return STG_E_INVALIDPOINTER;
  if (reserved != 0)
    return STG_E_INVALIDPARAMETER;

  return create_storagefile(pwcsName, grfMode, 0, &stgoptions, &IID_IStorage, (void**)ppstgOpen);
}

/******************************************************************************
 *              StgCreateStorageEx        [OLE32.@]
 */
HRESULT WINAPI StgCreateStorageEx(const WCHAR* pwcsName, DWORD grfMode, DWORD stgfmt, DWORD grfAttrs, STGOPTIONS* pStgOptions, void* reserved, REFIID riid, void** ppObjectOpen)
{
    TRACE("%s, %#lx, %#lx, %#lx, %p, %p, %p, %p.\n", debugstr_w(pwcsName),
          grfMode, stgfmt, grfAttrs, pStgOptions, reserved, riid, ppObjectOpen);

    if (stgfmt != STGFMT_FILE && grfAttrs != 0)
    {
        ERR("grfAttrs must be 0 if stgfmt != STGFMT_FILE\n");
        return STG_E_INVALIDPARAMETER;
    }

    if (stgfmt == STGFMT_FILE && grfAttrs != 0 && grfAttrs != FILE_FLAG_NO_BUFFERING)
    {
        ERR("grfAttrs must be 0 or FILE_FLAG_NO_BUFFERING if stgfmt == STGFMT_FILE\n");
        return STG_E_INVALIDPARAMETER;
    }

    if (stgfmt == STGFMT_FILE)
    {
        ERR("Cannot use STGFMT_FILE - this is NTFS only\n");
        return STG_E_INVALIDPARAMETER;
    }

    if (stgfmt == STGFMT_STORAGE || stgfmt == STGFMT_DOCFILE)
    {
        STGOPTIONS defaultOptions = {1, 0, 512};

        if (!pStgOptions) pStgOptions = &defaultOptions;
        return create_storagefile(pwcsName, grfMode, grfAttrs, pStgOptions, riid, ppObjectOpen);
    }


    ERR("Invalid stgfmt argument\n");
    return STG_E_INVALIDPARAMETER;
}

/******************************************************************************
 *              StgOpenStorageEx      [OLE32.@]
 */
HRESULT WINAPI StgOpenStorageEx(const WCHAR* pwcsName, DWORD grfMode, DWORD stgfmt, DWORD grfAttrs, STGOPTIONS* pStgOptions, void* reserved, REFIID riid, void** ppObjectOpen)
{
    TRACE("%s, %#lx, %#lx, %#lx, %p, %p, %p, %p.\n", debugstr_w(pwcsName),
          grfMode, stgfmt, grfAttrs, pStgOptions, reserved, riid, ppObjectOpen);

    if (stgfmt != STGFMT_DOCFILE && grfAttrs != 0)
    {
        ERR("grfAttrs must be 0 if stgfmt != STGFMT_DOCFILE\n");
        return STG_E_INVALIDPARAMETER;
    }

    switch (stgfmt)
    {
    case STGFMT_FILE:
        ERR("Cannot use STGFMT_FILE - this is NTFS only\n");
        return STG_E_INVALIDPARAMETER;

    case STGFMT_STORAGE:
        break;

    case STGFMT_DOCFILE:
        if (grfAttrs && grfAttrs != FILE_FLAG_NO_BUFFERING)
        {
            ERR("grfAttrs must be 0 or FILE_FLAG_NO_BUFFERING if stgfmt == STGFMT_DOCFILE\n");
            return STG_E_INVALIDPARAMETER;
        }
        FIXME("Stub: calling StgOpenStorage, but ignoring pStgOptions and grfAttrs\n");
        break;

    case STGFMT_ANY:
        WARN("STGFMT_ANY assuming storage\n");
        break;

    default:
        return STG_E_INVALIDPARAMETER;
    }

    return StgOpenStorage(pwcsName, NULL, grfMode, NULL, 0, (IStorage **)ppObjectOpen);
}


/******************************************************************************
 *              StgOpenStorage        [OLE32.@]
 */
HRESULT WINAPI StgOpenStorage(
  const OLECHAR *pwcsName,
  IStorage      *pstgPriority,
  DWORD          grfMode,
  SNB            snbExclude,
  DWORD          reserved,
  IStorage     **ppstgOpen)
{
  StorageBaseImpl* newStorage = 0;
  HRESULT        hr = S_OK;
  HANDLE         hFile = 0;
  DWORD          shareMode;
  DWORD          accessMode;
  LPWSTR         temp_name = NULL;

  TRACE("%s, %p, %#lx, %p, %ld, %p.\n", debugstr_w(pwcsName), pstgPriority, grfMode,
          snbExclude, reserved, ppstgOpen);

  if (pstgPriority)
  {
    /* FIXME: Copy ILockBytes instead? But currently for STGM_PRIORITY it'll be read-only. */
    hr = StorageBaseImpl_GetFilename((StorageBaseImpl*)pstgPriority, &temp_name);
    if (FAILED(hr)) goto end;
    pwcsName = temp_name;
    TRACE("using filename %s\n", debugstr_w(temp_name));
  }

  if (pwcsName == 0)
  {
    hr = STG_E_INVALIDNAME;
    goto end;
  }

  if (ppstgOpen == 0)
  {
    hr = STG_E_INVALIDPOINTER;
    goto end;
  }

  if (reserved)
  {
    hr = STG_E_INVALIDPARAMETER;
    goto end;
  }

  if (grfMode & STGM_PRIORITY)
  {
    if (grfMode & (STGM_TRANSACTED|STGM_SIMPLE|STGM_NOSCRATCH|STGM_NOSNAPSHOT))
      return STG_E_INVALIDFLAG;
    if (grfMode & STGM_DELETEONRELEASE)
      return STG_E_INVALIDFUNCTION;
    if(STGM_ACCESS_MODE(grfMode) != STGM_READ)
      return STG_E_INVALIDFLAG;
    grfMode &= ~0xf0; /* remove the existing sharing mode */
    grfMode |= STGM_SHARE_DENY_NONE;
  }

  /*
   * Validate the sharing mode
   */
  if (grfMode & STGM_DIRECT_SWMR)
  {
    if ((STGM_SHARE_MODE(grfMode) != STGM_SHARE_DENY_WRITE) &&
        (STGM_SHARE_MODE(grfMode) != STGM_SHARE_DENY_NONE))
    {
      hr = STG_E_INVALIDFLAG;
      goto end;
    }
  }
  else if (!(grfMode & (STGM_TRANSACTED|STGM_PRIORITY)))
    switch(STGM_SHARE_MODE(grfMode))
    {
      case STGM_SHARE_EXCLUSIVE:
      case STGM_SHARE_DENY_WRITE:
        break;
      default:
        hr = STG_E_INVALIDFLAG;
        goto end;
    }

  if ( FAILED( validateSTGM(grfMode) ) ||
       (grfMode&STGM_CREATE))
  {
    hr = STG_E_INVALIDFLAG;
    goto end;
  }

  /* shared reading requires transacted or single writer mode */
  if( STGM_SHARE_MODE(grfMode) == STGM_SHARE_DENY_WRITE &&
      STGM_ACCESS_MODE(grfMode) == STGM_READWRITE &&
     !(grfMode & STGM_TRANSACTED) && !(grfMode & STGM_DIRECT_SWMR))
  {
    hr = STG_E_INVALIDFLAG;
    goto end;
  }

  /*
   * Interpret the STGM value grfMode
   */
  shareMode    = GetShareModeFromSTGM(grfMode);
  accessMode   = GetAccessModeFromSTGM(grfMode);

  *ppstgOpen = 0;

  hFile = CreateFileW( pwcsName,
                       accessMode,
                       shareMode,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                       0);

  if (hFile==INVALID_HANDLE_VALUE)
  {
    DWORD last_error = GetLastError();

    hr = E_FAIL;

    switch (last_error)
    {
      case ERROR_FILE_NOT_FOUND:
        hr = STG_E_FILENOTFOUND;
        break;

      case ERROR_PATH_NOT_FOUND:
        hr = STG_E_PATHNOTFOUND;
        break;

      case ERROR_ACCESS_DENIED:
      case ERROR_WRITE_PROTECT:
        hr = STG_E_ACCESSDENIED;
        break;

      case ERROR_SHARING_VIOLATION:
        hr = STG_E_SHAREVIOLATION;
        break;

      default:
        hr = E_FAIL;
    }

    goto end;
  }

  /*
   * Refuse to open the file if it's too small to be a structured storage file
   * FIXME: verify the file when reading instead of here
   */
  if (GetFileSize(hFile, NULL) < HEADER_SIZE)
  {
    CloseHandle(hFile);
    hr = STG_E_FILEALREADYEXISTS;
    goto end;
  }

  /*
   * Allocate and initialize the new IStorage object.
   */
  hr = Storage_Construct(
         hFile,
         pwcsName,
         NULL,
         grfMode,
         TRUE,
         FALSE,
         512,
         &newStorage);

  if (FAILED(hr))
  {
    /*
     * According to the docs if the file is not a storage, return STG_E_FILEALREADYEXISTS
     */
    if(hr == STG_E_INVALIDHEADER)
	hr = STG_E_FILEALREADYEXISTS;
    goto end;
  }

  *ppstgOpen = &newStorage->IStorage_iface;

end:
  CoTaskMemFree(temp_name);
  if (pstgPriority) IStorage_Release(pstgPriority);
  TRACE("<-- %#lx, IStorage %p\n", hr, ppstgOpen ? *ppstgOpen : NULL);
  return hr;
}

/******************************************************************************
 *    StgCreateDocfileOnILockBytes    [OLE32.@]
 */
HRESULT WINAPI StgCreateDocfileOnILockBytes(
      ILockBytes *plkbyt,
      DWORD grfMode,
      DWORD reserved,
      IStorage** ppstgOpen)
{
  StorageBaseImpl* newStorage = 0;
  HRESULT        hr         = S_OK;

  if ((ppstgOpen == 0) || (plkbyt == 0))
    return STG_E_INVALIDPOINTER;

  /*
   * Allocate and initialize the new IStorage object.
   */
  hr = Storage_Construct(
         0,
        0,
         plkbyt,
         grfMode,
         FALSE,
         TRUE,
         512,
         &newStorage);

  if (FAILED(hr))
  {
    return hr;
  }

  *ppstgOpen = &newStorage->IStorage_iface;

  return hr;
}

/******************************************************************************
 *    StgOpenStorageOnILockBytes    [OLE32.@]
 */
HRESULT WINAPI StgOpenStorageOnILockBytes(
      ILockBytes *plkbyt,
      IStorage *pstgPriority,
      DWORD grfMode,
      SNB snbExclude,
      DWORD reserved,
      IStorage **ppstgOpen)
{
  StorageBaseImpl* newStorage = 0;
  HRESULT        hr = S_OK;

  if ((plkbyt == 0) || (ppstgOpen == 0))
    return STG_E_INVALIDPOINTER;

  if ( FAILED( validateSTGM(grfMode) ))
    return STG_E_INVALIDFLAG;

  *ppstgOpen = 0;

  /*
   * Allocate and initialize the new IStorage object.
   */
  hr = Storage_Construct(
         0,
         0,
         plkbyt,
         grfMode,
         FALSE,
         FALSE,
         512,
         &newStorage);

  if (FAILED(hr))
  {
    return hr;
  }

  *ppstgOpen = &newStorage->IStorage_iface;

  return hr;
}

/******************************************************************************
 *              StgSetTimes [ole32.@]
 *              StgSetTimes [OLE32.@]
 *
 *
 */
HRESULT WINAPI StgSetTimes(OLECHAR const *str, FILETIME const *pctime,
                           FILETIME const *patime, FILETIME const *pmtime)
{
  IStorage *stg = NULL;
  HRESULT r;

  TRACE("%s %p %p %p\n", debugstr_w(str), pctime, patime, pmtime);

  r = StgOpenStorage(str, NULL, STGM_READWRITE | STGM_SHARE_DENY_WRITE,
                     0, 0, &stg);
  if( SUCCEEDED(r) )
  {
    r = IStorage_SetElementTimes(stg, NULL, pctime, patime, pmtime);
    IStorage_Release(stg);
  }

  return r;
}

/***********************************************************************
 *    OleLoadFromStream (OLE32.@)
 *
 * This function loads an object from stream
 */
HRESULT  WINAPI OleLoadFromStream(IStream *pStm,REFIID iidInterface,void** ppvObj)
{
    CLSID	clsid;
    HRESULT	res;
    LPPERSISTSTREAM	xstm;

    TRACE("(%p,%s,%p)\n",pStm,debugstr_guid(iidInterface),ppvObj);

    res=ReadClassStm(pStm,&clsid);
    if (FAILED(res))
	return res;
    res=CoCreateInstance(&clsid,NULL,CLSCTX_INPROC_SERVER,iidInterface,ppvObj);
    if (FAILED(res))
	return res;
    res=IUnknown_QueryInterface((IUnknown*)*ppvObj,&IID_IPersistStream,(LPVOID*)&xstm);
    if (FAILED(res)) {
	IUnknown_Release((IUnknown*)*ppvObj);
	return res;
    }
    res=IPersistStream_Load(xstm,pStm);
    IPersistStream_Release(xstm);
    /* FIXME: all refcounts ok at this point? I think they should be:
     * 		pStm	: unchanged
     *		ppvObj	: 1
     *		xstm	: 0 (released)
     */
    return res;
}

/***********************************************************************
 *    OleSaveToStream (OLE32.@)
 *
 * This function saves an object with the IPersistStream interface on it
 * to the specified stream.
 */
HRESULT  WINAPI OleSaveToStream(IPersistStream *pPStm,IStream *pStm)
{

    CLSID clsid;
    HRESULT res;

    TRACE("(%p,%p)\n",pPStm,pStm);

    res=IPersistStream_GetClassID(pPStm,&clsid);

    if (SUCCEEDED(res)){

        res=WriteClassStm(pStm,&clsid);

        if (SUCCEEDED(res))

            res=IPersistStream_Save(pPStm,pStm,TRUE);
    }

    TRACE("Finished Save\n");
    return res;
}

/*************************************************************************
 * STORAGE_CreateOleStream [Internal]
 *
 * Creates the "\001OLE" stream in the IStorage if necessary.
 *
 * PARAMS
 *     storage     [I] Dest storage to create the stream in
 *     flags       [I] flags to be set for newly created stream
 *
 * RETURNS
 *     HRESULT return value
 *
 * NOTES
 *
 *     This stream is still unknown, MS Word seems to have extra data
 *     but since the data is stored in the OLESTREAM there should be
 *     no need to recreate the stream.  If the stream is manually
 *     deleted it will create it with this default data.
 *
 */
HRESULT STORAGE_CreateOleStream(IStorage *storage, DWORD flags)
{
    static const DWORD version_magic = 0x02000001;
    IStream *stream;
    HRESULT hr;

    hr = IStorage_CreateStream(storage, L"\1Ole", STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stream);
    if (hr == S_OK)
    {
        struct empty_1ole_stream {
            DWORD version_magic;
            DWORD flags;
            DWORD update_options;
            DWORD reserved;
            DWORD mon_stream_size;
        };
        struct empty_1ole_stream stream_data;

        stream_data.version_magic = version_magic;
        stream_data.flags = flags;
        stream_data.update_options = 0;
        stream_data.reserved = 0;
        stream_data.mon_stream_size = 0;

        hr = IStream_Write(stream, &stream_data, sizeof(stream_data), NULL);
        IStream_Release(stream);
    }

    return hr;
}

/* write a string to a stream, preceded by its length */
static HRESULT STREAM_WriteString( IStream *stm, LPCWSTR string )
{
    HRESULT r;
    LPSTR str;
    DWORD len = 0;

    if( string )
        len = WideCharToMultiByte( CP_ACP, 0, string, -1, NULL, 0, NULL, NULL);
    r = IStream_Write( stm, &len, sizeof(len), NULL);
    if( FAILED( r ) )
        return r;
    if(len == 0)
        return r;
    str = CoTaskMemAlloc( len );
    WideCharToMultiByte( CP_ACP, 0, string, -1, str, len, NULL, NULL);
    r = IStream_Write( stm, str, len, NULL);
    CoTaskMemFree( str );
    return r;
}

/* read a string preceded by its length from a stream */
static HRESULT STREAM_ReadString( IStream *stm, LPWSTR *string )
{
    HRESULT r;
    DWORD len, count = 0;
    LPSTR str;
    LPWSTR wstr;

    r = IStream_Read( stm, &len, sizeof(len), &count );
    if( FAILED( r ) )
        return r;
    if( count != sizeof(len) )
        return E_OUTOFMEMORY;

    TRACE("%ld bytes\n",len);

    str = CoTaskMemAlloc( len );
    if( !str )
        return E_OUTOFMEMORY;
    count = 0;
    r = IStream_Read( stm, str, len, &count );
    if( FAILED( r ) )
    {
        CoTaskMemFree( str );
        return r;
    }
    if( count != len )
    {
        CoTaskMemFree( str );
        return E_OUTOFMEMORY;
    }

    TRACE("Read string %s\n",debugstr_an(str,len));

    len = MultiByteToWideChar( CP_ACP, 0, str, count, NULL, 0 );
    wstr = CoTaskMemAlloc( (len + 1)*sizeof (WCHAR) );
    if( wstr )
    {
         MultiByteToWideChar( CP_ACP, 0, str, count, wstr, len );
         wstr[len] = 0;
    }
    CoTaskMemFree( str );

    *string = wstr;

    return r;
}


static HRESULT STORAGE_WriteCompObj( LPSTORAGE pstg, CLSID *clsid,
    LPCWSTR lpszUserType, LPCWSTR szClipName, LPCWSTR szProgIDName )
{
    IStream *pstm;
    HRESULT r = S_OK;

    static const BYTE unknown1[12] =
       { 0x01, 0x00, 0xFE, 0xFF, 0x03, 0x0A, 0x00, 0x00,
         0xFF, 0xFF, 0xFF, 0xFF};
    static const BYTE unknown2[16] =
       { 0xF4, 0x39, 0xB2, 0x71, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    TRACE("%p %s %s %s %s\n", pstg, debugstr_guid(clsid),
           debugstr_w(lpszUserType), debugstr_w(szClipName),
           debugstr_w(szProgIDName));

    /*  Create a CompObj stream */
    r = IStorage_CreateStream(pstg, L"\1CompObj",
        STGM_CREATE | STGM_WRITE  | STGM_SHARE_EXCLUSIVE, 0, 0, &pstm );
    if( FAILED (r) )
        return r;

    /* Write CompObj Structure to stream */
    r = IStream_Write(pstm, unknown1, sizeof(unknown1), NULL);

    if( SUCCEEDED( r ) )
        r = WriteClassStm( pstm, clsid );

    if( SUCCEEDED( r ) )
        r = STREAM_WriteString( pstm, lpszUserType );
    if( SUCCEEDED( r ) )
        r = STREAM_WriteString( pstm, szClipName );
    if( SUCCEEDED( r ) )
        r = STREAM_WriteString( pstm, szProgIDName );
    if( SUCCEEDED( r ) )
        r = IStream_Write(pstm, unknown2, sizeof(unknown2), NULL);

    IStream_Release( pstm );

    return r;
}

/***********************************************************************
 *               WriteFmtUserTypeStg (OLE32.@)
 */
HRESULT WINAPI WriteFmtUserTypeStg(
	  LPSTORAGE pstg, CLIPFORMAT cf, LPOLESTR lpszUserType)
{
    STATSTG stat;
    HRESULT r;
    WCHAR szwClipName[0x40];
    CLSID clsid;
    LPWSTR wstrProgID = NULL;
    DWORD n;

    TRACE("(%p,%x,%s)\n",pstg,cf,debugstr_w(lpszUserType));

    /* get the clipboard format name */
    if( cf )
    {
        n = GetClipboardFormatNameW(cf, szwClipName, ARRAY_SIZE(szwClipName));
        szwClipName[n]=0;
    }

    TRACE("Clipboard name is %s\n", debugstr_w(szwClipName));

    r = IStorage_Stat(pstg, &stat, STATFLAG_NONAME);
    if(SUCCEEDED(r))
        clsid = stat.clsid;
    else
        clsid = CLSID_NULL;

    ProgIDFromCLSID(&clsid, &wstrProgID);

    TRACE("progid is %s\n",debugstr_w(wstrProgID));

    r = STORAGE_WriteCompObj( pstg, &clsid, lpszUserType,
            cf ? szwClipName : NULL, wstrProgID );

    CoTaskMemFree(wstrProgID);

    return r;
}


/******************************************************************************
 *              ReadFmtUserTypeStg        [OLE32.@]
 */
HRESULT WINAPI ReadFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT* pcf, LPOLESTR* lplpszUserType)
{
    HRESULT r;
    IStream *stm = 0;
    unsigned char unknown1[12];
    unsigned char unknown2[16];
    DWORD count;
    LPWSTR szProgIDName = NULL, szCLSIDName = NULL, szOleTypeName = NULL;
    CLSID clsid;

    TRACE("(%p,%p,%p)\n", pstg, pcf, lplpszUserType);

    r = IStorage_OpenStream( pstg, L"\1CompObj", NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm );
    if( FAILED ( r ) )
    {
        WARN("Failed to open stream r = %#lx\n", r);
        return r;
    }

    /* read the various parts of the structure */
    r = IStream_Read( stm, unknown1, sizeof(unknown1), &count );
    if( FAILED( r ) || ( count != sizeof(unknown1) ) )
        goto end;
    r = ReadClassStm( stm, &clsid );
    if( FAILED( r ) )
        goto end;

    r = STREAM_ReadString( stm, &szCLSIDName );
    if( FAILED( r ) )
        goto end;

    r = STREAM_ReadString( stm, &szOleTypeName );
    if( FAILED( r ) )
        goto end;

    r = STREAM_ReadString( stm, &szProgIDName );
    if( FAILED( r ) )
        goto end;

    r = IStream_Read( stm, unknown2, sizeof(unknown2), &count );
    if( FAILED( r ) || ( count != sizeof(unknown2) ) )
        goto end;

    /* ok, success... now we just need to store what we found */
    if( pcf )
        *pcf = RegisterClipboardFormatW( szOleTypeName );

    if( lplpszUserType )
    {
        *lplpszUserType = szCLSIDName;
        szCLSIDName = NULL;
    }

end:
    CoTaskMemFree( szCLSIDName );
    CoTaskMemFree( szOleTypeName );
    CoTaskMemFree( szProgIDName );
    IStream_Release( stm );

    return r;
}

/************************************************************************
 * OleConvert Functions
 ***********************************************************************/

#define OLESTREAM_ID 0x501
#define OLESTREAM_MAX_STR_LEN 255

/* OLESTREAM memory structure to use for Get and Put Routines */
typedef struct
{
    DWORD dwOleID;
    DWORD dwTypeID;
    DWORD dwOleTypeNameLength;
    CHAR  strOleTypeName[OLESTREAM_MAX_STR_LEN];
    CHAR  *pstrOleObjFileName;
    DWORD dwOleObjFileNameLength;
    DWORD dwMetaFileWidth;
    DWORD dwMetaFileHeight;
    CHAR  strUnknown[8]; /* don't know what this 8 byte information in OLE stream is. */
    DWORD dwDataLength;
    BYTE *pData;
} OLECONVERT_OLESTREAM_DATA;

/* CompObj Stream structure */
typedef struct
{
    BYTE byUnknown1[12];
    CLSID clsid;
    DWORD dwCLSIDNameLength;
    CHAR strCLSIDName[OLESTREAM_MAX_STR_LEN];
    DWORD dwOleTypeNameLength;
    CHAR strOleTypeName[OLESTREAM_MAX_STR_LEN];
    DWORD dwProgIDNameLength;
    CHAR strProgIDName[OLESTREAM_MAX_STR_LEN];
    BYTE byUnknown2[16];
} OLECONVERT_ISTORAGE_COMPOBJ;

/* Ole Presentation Stream structure */
typedef struct
{
    BYTE byUnknown1[28];
    DWORD dwExtentX;
    DWORD dwExtentY;
    DWORD dwSize;
    BYTE *pData;
} OLECONVERT_ISTORAGE_OLEPRES;


/*************************************************************************
 * OLECONVERT_LoadOLE10 [Internal]
 *
 * Loads the OLE10 STREAM to memory
 *
 * PARAMS
 *     pOleStream   [I] The OLESTREAM
 *     pData        [I] Data Structure for the OLESTREAM Data
 *
 * RETURNS
 *     Success:  S_OK
 *     Failure:  CONVERT10_E_OLESTREAM_GET for invalid Get
 *               CONVERT10_E_OLESTREAM_FMT if the OLEID is invalid
 *
 * NOTES
 *     This function is used by OleConvertOLESTREAMToIStorage only.
 *
 *     Memory allocated for pData must be freed by the caller
 */
static HRESULT OLECONVERT_LoadOLE10(LPOLESTREAM pOleStream, OLECONVERT_OLESTREAM_DATA *pData, BOOL bStream1)
{
	DWORD dwSize;
	HRESULT hRes = S_OK;
	int nTryCnt=0;
	int max_try = 6;

	pData->pData = NULL;
	pData->pstrOleObjFileName = NULL;

	for( nTryCnt=0;nTryCnt < max_try; nTryCnt++)
	{
	/* Get the OleID */
	dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwOleID), sizeof(pData->dwOleID));
	if(dwSize != sizeof(pData->dwOleID))
	{
		hRes = CONVERT10_E_OLESTREAM_GET;
	}
	else if(pData->dwOleID != OLESTREAM_ID)
	{
		hRes = CONVERT10_E_OLESTREAM_FMT;
	}
		else
		{
			hRes = S_OK;
			break;
		}
	}

	if(hRes == S_OK)
	{
		/* Get the TypeID... more info needed for this field */
		dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwTypeID), sizeof(pData->dwTypeID));
		if(dwSize != sizeof(pData->dwTypeID))
		{
			hRes = CONVERT10_E_OLESTREAM_GET;
		}
	}
	if(hRes == S_OK)
	{
		if(pData->dwTypeID != 0)
		{
			/* Get the length of the OleTypeName */
			dwSize = pOleStream->lpstbl->Get(pOleStream, (void *) &(pData->dwOleTypeNameLength), sizeof(pData->dwOleTypeNameLength));
			if(dwSize != sizeof(pData->dwOleTypeNameLength))
			{
				hRes = CONVERT10_E_OLESTREAM_GET;
			}

			if(hRes == S_OK)
			{
				if(pData->dwOleTypeNameLength > 0)
				{
					/* Get the OleTypeName */
					dwSize = pOleStream->lpstbl->Get(pOleStream, pData->strOleTypeName, pData->dwOleTypeNameLength);
					if(dwSize != pData->dwOleTypeNameLength)
					{
						hRes = CONVERT10_E_OLESTREAM_GET;
					}
				}
			}
			if(bStream1)
			{
				dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwOleObjFileNameLength), sizeof(pData->dwOleObjFileNameLength));
				if(dwSize != sizeof(pData->dwOleObjFileNameLength))
				{
					hRes = CONVERT10_E_OLESTREAM_GET;
				}
			if(hRes == S_OK)
			{
					if(pData->dwOleObjFileNameLength < 1) /* there is no file name exist */
						pData->dwOleObjFileNameLength = sizeof(pData->dwOleObjFileNameLength);
					pData->pstrOleObjFileName = HeapAlloc(GetProcessHeap(), 0, pData->dwOleObjFileNameLength);
					if(pData->pstrOleObjFileName)
					{
						dwSize = pOleStream->lpstbl->Get(pOleStream, pData->pstrOleObjFileName, pData->dwOleObjFileNameLength);
						if(dwSize != pData->dwOleObjFileNameLength)
						{
							hRes = CONVERT10_E_OLESTREAM_GET;
						}
					}
					else
						hRes = CONVERT10_E_OLESTREAM_GET;
				}
			}
			else
			{
				/* Get the Width of the Metafile */
				dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwMetaFileWidth), sizeof(pData->dwMetaFileWidth));
				if(dwSize != sizeof(pData->dwMetaFileWidth))
				{
					hRes = CONVERT10_E_OLESTREAM_GET;
				}
			if(hRes == S_OK)
			{
				/* Get the Height of the Metafile */
				dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwMetaFileHeight), sizeof(pData->dwMetaFileHeight));
				if(dwSize != sizeof(pData->dwMetaFileHeight))
				{
					hRes = CONVERT10_E_OLESTREAM_GET;
				}
			}
			}
			if(hRes == S_OK)
			{
				/* Get the Length of the Data */
				dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwDataLength), sizeof(pData->dwDataLength));
				if(dwSize != sizeof(pData->dwDataLength))
				{
					hRes = CONVERT10_E_OLESTREAM_GET;
				}
			}

			if(hRes == S_OK) /* I don't know what this 8 byte information is. We have to figure out */
			{
				if(!bStream1) /* if it is a second OLE stream data */
				{
					pData->dwDataLength -= 8;
					dwSize = pOleStream->lpstbl->Get(pOleStream, pData->strUnknown, sizeof(pData->strUnknown));
					if(dwSize != sizeof(pData->strUnknown))
					{
						hRes = CONVERT10_E_OLESTREAM_GET;
					}
				}
			}
			if(hRes == S_OK)
			{
				if(pData->dwDataLength > 0)
				{
					pData->pData = HeapAlloc(GetProcessHeap(),0,pData->dwDataLength);

					/* Get Data (ex. IStorage, Metafile, or BMP) */
					if(pData->pData)
					{
						dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)pData->pData, pData->dwDataLength);
						if(dwSize != pData->dwDataLength)
						{
							hRes = CONVERT10_E_OLESTREAM_GET;
						}
					}
					else
					{
						hRes = CONVERT10_E_OLESTREAM_GET;
					}
				}
			}
		}
	}
	return hRes;
}

/*************************************************************************
 * OLECONVERT_SaveOLE10 [Internal]
 *
 * Saves the OLE10 STREAM From memory
 *
 * PARAMS
 *     pData        [I] Data Structure for the OLESTREAM Data
 *     pOleStream   [I] The OLESTREAM to save
 *
 * RETURNS
 *     Success:  S_OK
 *     Failure:  CONVERT10_E_OLESTREAM_PUT for invalid Put
 *
 * NOTES
 *     This function is used by OleConvertIStorageToOLESTREAM only.
 *
 */
static HRESULT OLECONVERT_SaveOLE10(OLECONVERT_OLESTREAM_DATA *pData, LPOLESTREAM pOleStream)
{
    DWORD dwSize;
    HRESULT hRes = S_OK;


   /* Set the OleID */
    dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwOleID), sizeof(pData->dwOleID));
    if(dwSize != sizeof(pData->dwOleID))
    {
        hRes = CONVERT10_E_OLESTREAM_PUT;
    }

    if(hRes == S_OK)
    {
        /* Set the TypeID */
        dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwTypeID), sizeof(pData->dwTypeID));
        if(dwSize != sizeof(pData->dwTypeID))
        {
            hRes = CONVERT10_E_OLESTREAM_PUT;
        }
    }

    if(pData->dwOleID == OLESTREAM_ID && pData->dwTypeID != 0 && hRes == S_OK)
    {
        /* Set the Length of the OleTypeName */
        dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwOleTypeNameLength), sizeof(pData->dwOleTypeNameLength));
        if(dwSize != sizeof(pData->dwOleTypeNameLength))
        {
            hRes = CONVERT10_E_OLESTREAM_PUT;
        }

        if(hRes == S_OK)
        {
            if(pData->dwOleTypeNameLength > 0)
            {
                /* Set the OleTypeName */
                dwSize = pOleStream->lpstbl->Put(pOleStream, pData->strOleTypeName, pData->dwOleTypeNameLength);
                if(dwSize != pData->dwOleTypeNameLength)
                {
                    hRes = CONVERT10_E_OLESTREAM_PUT;
                }
            }
        }

        if(hRes == S_OK)
        {
            /* Set the width of the Metafile */
            dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwMetaFileWidth), sizeof(pData->dwMetaFileWidth));
            if(dwSize != sizeof(pData->dwMetaFileWidth))
            {
                hRes = CONVERT10_E_OLESTREAM_PUT;
            }
        }

        if(hRes == S_OK)
        {
            /* Set the height of the Metafile */
            dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwMetaFileHeight), sizeof(pData->dwMetaFileHeight));
            if(dwSize != sizeof(pData->dwMetaFileHeight))
            {
                hRes = CONVERT10_E_OLESTREAM_PUT;
            }
        }

        if(hRes == S_OK)
        {
            /* Set the length of the Data */
            dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwDataLength), sizeof(pData->dwDataLength));
            if(dwSize != sizeof(pData->dwDataLength))
            {
                hRes = CONVERT10_E_OLESTREAM_PUT;
            }
        }

        if(hRes == S_OK)
        {
            if(pData->dwDataLength > 0)
            {
                /* Set the Data (eg. IStorage, Metafile, Bitmap) */
                dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)  pData->pData, pData->dwDataLength);
                if(dwSize != pData->dwDataLength)
                {
                    hRes = CONVERT10_E_OLESTREAM_PUT;
                }
            }
        }
    }
    return hRes;
}

/*************************************************************************
 * OLECONVERT_GetOLE20FromOLE10[Internal]
 *
 * This function copies OLE10 Data (the IStorage in the OLESTREAM) to disk,
 * opens it, and copies the content to the dest IStorage for
 * OleConvertOLESTREAMToIStorage
 *
 *
 * PARAMS
 *     pDestStorage  [I] The IStorage to copy the data to
 *     pBuffer       [I] Buffer that contains the IStorage from the OLESTREAM
 *     nBufferLength [I] The size of the buffer
 *
 * RETURNS
 *     Nothing
 *
 * NOTES
 *
 *
 */
static void OLECONVERT_GetOLE20FromOLE10(LPSTORAGE pDestStorage, const BYTE *pBuffer, DWORD nBufferLength)
{
    HRESULT hRes;
    HANDLE hFile;
    IStorage *pTempStorage;
    DWORD dwNumOfBytesWritten;
    WCHAR wstrTempDir[MAX_PATH], wstrTempFile[MAX_PATH];

    /* Create a temp File */
    GetTempPathW(MAX_PATH, wstrTempDir);
    GetTempFileNameW(wstrTempDir, L"sis", 0, wstrTempFile);
    hFile = CreateFileW(wstrTempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if(hFile != INVALID_HANDLE_VALUE)
    {
        /* Write IStorage Data to File */
        WriteFile(hFile, pBuffer, nBufferLength, &dwNumOfBytesWritten, NULL);
        CloseHandle(hFile);

        /* Open and copy temp storage to the Dest Storage */
        hRes = StgOpenStorage(wstrTempFile, NULL, STGM_READ, NULL, 0, &pTempStorage);
        if(hRes == S_OK)
        {
            hRes = IStorage_CopyTo(pTempStorage, 0, NULL, NULL, pDestStorage);
            IStorage_Release(pTempStorage);
        }
        DeleteFileW(wstrTempFile);
    }
}


/*************************************************************************
 * OLECONVERT_WriteOLE20ToBuffer [Internal]
 *
 * Saves the OLE10 STREAM From memory
 *
 * PARAMS
 *     pStorage  [I] The Src IStorage to copy
 *     pData     [I] The Dest Memory to write to.
 *
 * RETURNS
 *     The size in bytes allocated for pData
 *
 * NOTES
 *     Memory allocated for pData must be freed by the caller
 *
 *     Used by OleConvertIStorageToOLESTREAM only.
 *
 */
static DWORD OLECONVERT_WriteOLE20ToBuffer(LPSTORAGE pStorage, BYTE **pData)
{
    HANDLE hFile;
    HRESULT hRes;
    DWORD nDataLength = 0;
    IStorage *pTempStorage;
    WCHAR wstrTempDir[MAX_PATH], wstrTempFile[MAX_PATH];

    *pData = NULL;

    /* Create temp Storage */
    GetTempPathW(MAX_PATH, wstrTempDir);
    GetTempFileNameW(wstrTempDir, L"sis", 0, wstrTempFile);
    hRes = StgCreateDocfile(wstrTempFile, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pTempStorage);

    if(hRes == S_OK)
    {
        /* Copy Src Storage to the Temp Storage */
        IStorage_CopyTo(pStorage, 0, NULL, NULL, pTempStorage);
        IStorage_Release(pTempStorage);

        /* Open Temp Storage as a file and copy to memory */
        hFile = CreateFileW(wstrTempFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if(hFile != INVALID_HANDLE_VALUE)
        {
            nDataLength = GetFileSize(hFile, NULL);
            *pData = HeapAlloc(GetProcessHeap(),0,nDataLength);
            ReadFile(hFile, *pData, nDataLength, &nDataLength, 0);
            CloseHandle(hFile);
        }
        DeleteFileW(wstrTempFile);
    }
    return nDataLength;
}

/*************************************************************************
 * OLECONVERT_CreateCompObjStream [Internal]
 *
 * Creates a "\001CompObj" is the destination IStorage if necessary.
 *
 * PARAMS
 *     pStorage       [I] The dest IStorage to create the CompObj Stream
 *                        if necessary.
 *     strOleTypeName [I] The ProgID
 *
 * RETURNS
 *     Success:  S_OK
 *     Failure:  REGDB_E_CLASSNOTREG if cannot reconstruct the stream
 *
 * NOTES
 *     This function is used by OleConvertOLESTREAMToIStorage only.
 *
 *     The stream data is stored in the OLESTREAM and there should be
 *     no need to recreate the stream.  If the stream is manually
 *     deleted it will attempt to create it by querying the registry.
 *
 *
 */
HRESULT OLECONVERT_CreateCompObjStream(LPSTORAGE pStorage, LPCSTR strOleTypeName)
{
    IStream *pStream;
    HRESULT hStorageRes, hRes = S_OK;
    OLECONVERT_ISTORAGE_COMPOBJ IStorageCompObj;
    WCHAR bufferW[OLESTREAM_MAX_STR_LEN];

    static const BYTE pCompObjUnknown1[] = {0x01, 0x00, 0xFE, 0xFF, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    static const BYTE pCompObjUnknown2[] = {0xF4, 0x39, 0xB2, 0x71};

    /* Initialize the CompObj structure */
    memset(&IStorageCompObj, 0, sizeof(IStorageCompObj));
    memcpy(IStorageCompObj.byUnknown1, pCompObjUnknown1, sizeof(pCompObjUnknown1));
    memcpy(IStorageCompObj.byUnknown2, pCompObjUnknown2, sizeof(pCompObjUnknown2));


    /*  Create a CompObj stream if it doesn't exist */
    hStorageRes = IStorage_CreateStream(pStorage, L"\1CompObj",
        STGM_WRITE  | STGM_SHARE_EXCLUSIVE, 0, 0, &pStream );
    if(hStorageRes == S_OK)
    {
        /* copy the OleTypeName to the compobj struct */
        IStorageCompObj.dwOleTypeNameLength = strlen(strOleTypeName)+1;
        strcpy(IStorageCompObj.strOleTypeName, strOleTypeName);

        /* copy the OleTypeName to the compobj struct */
        /* Note: in the test made, these were Identical      */
        IStorageCompObj.dwProgIDNameLength = strlen(strOleTypeName)+1;
        strcpy(IStorageCompObj.strProgIDName, strOleTypeName);

        /* Get the CLSID */
        MultiByteToWideChar( CP_ACP, 0, IStorageCompObj.strProgIDName, -1,
                             bufferW, OLESTREAM_MAX_STR_LEN );
        hRes = CLSIDFromProgID(bufferW, &(IStorageCompObj.clsid));

        if(hRes == S_OK)
        {
            HKEY hKey;
            LONG hErr;
            /* Get the CLSID Default Name from the Registry */
            hErr = open_classes_key(HKEY_CLASSES_ROOT, bufferW, MAXIMUM_ALLOWED, &hKey);
            if(hErr == ERROR_SUCCESS)
            {
                char strTemp[OLESTREAM_MAX_STR_LEN];
                IStorageCompObj.dwCLSIDNameLength = OLESTREAM_MAX_STR_LEN;
                hErr = RegQueryValueA(hKey, NULL, strTemp, (LONG*) &(IStorageCompObj.dwCLSIDNameLength));
                if(hErr == ERROR_SUCCESS)
                {
                    strcpy(IStorageCompObj.strCLSIDName, strTemp);
                }
                RegCloseKey(hKey);
            }
        }

        /* Write CompObj Structure to stream */
        hRes = IStream_Write(pStream, IStorageCompObj.byUnknown1, sizeof(IStorageCompObj.byUnknown1), NULL);

        WriteClassStm(pStream,&(IStorageCompObj.clsid));

        hRes = IStream_Write(pStream, &(IStorageCompObj.dwCLSIDNameLength), sizeof(IStorageCompObj.dwCLSIDNameLength), NULL);
        if(IStorageCompObj.dwCLSIDNameLength > 0)
        {
            hRes = IStream_Write(pStream, IStorageCompObj.strCLSIDName, IStorageCompObj.dwCLSIDNameLength, NULL);
        }
        hRes = IStream_Write(pStream, &(IStorageCompObj.dwOleTypeNameLength) , sizeof(IStorageCompObj.dwOleTypeNameLength), NULL);
        if(IStorageCompObj.dwOleTypeNameLength > 0)
        {
            hRes = IStream_Write(pStream, IStorageCompObj.strOleTypeName , IStorageCompObj.dwOleTypeNameLength, NULL);
        }
        hRes = IStream_Write(pStream, &(IStorageCompObj.dwProgIDNameLength) , sizeof(IStorageCompObj.dwProgIDNameLength), NULL);
        if(IStorageCompObj.dwProgIDNameLength > 0)
        {
            hRes = IStream_Write(pStream, IStorageCompObj.strProgIDName , IStorageCompObj.dwProgIDNameLength, NULL);
        }
        hRes = IStream_Write(pStream, IStorageCompObj.byUnknown2 , sizeof(IStorageCompObj.byUnknown2), NULL);
        IStream_Release(pStream);
    }
    return hRes;
}


/*************************************************************************
 * OLECONVERT_CreateOlePresStream[Internal]
 *
 * Creates the "\002OlePres000" Stream with the Metafile data
 *
 * PARAMS
 *     pStorage     [I] The dest IStorage to create \002OLEPres000 stream in.
 *     dwExtentX    [I] Width of the Metafile
 *     dwExtentY    [I] Height of the Metafile
 *     pData        [I] Metafile data
 *     dwDataLength [I] Size of the Metafile data
 *
 * RETURNS
 *     Success:  S_OK
 *     Failure:  CONVERT10_E_OLESTREAM_PUT for invalid Put
 *
 * NOTES
 *     This function is used by OleConvertOLESTREAMToIStorage only.
 *
 */
static void OLECONVERT_CreateOlePresStream(LPSTORAGE pStorage, DWORD dwExtentX, DWORD dwExtentY , BYTE *pData, DWORD dwDataLength)
{
    HRESULT hRes;
    IStream *pStream;
    static const BYTE pOlePresStreamHeader[] =
    {
        0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    static const BYTE pOlePresStreamHeaderEmpty[] =
    {
        0x00, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    /* Create the OlePres000 Stream */
    hRes = IStorage_CreateStream(pStorage, L"\2OlePres000",
        STGM_CREATE | STGM_WRITE  | STGM_SHARE_EXCLUSIVE, 0, 0, &pStream );

    if(hRes == S_OK)
    {
        DWORD nHeaderSize;
        OLECONVERT_ISTORAGE_OLEPRES OlePres;

        memset(&OlePres, 0, sizeof(OlePres));
        /* Do we have any metafile data to save */
        if(dwDataLength > 0)
        {
            memcpy(OlePres.byUnknown1, pOlePresStreamHeader, sizeof(pOlePresStreamHeader));
            nHeaderSize = sizeof(pOlePresStreamHeader);
        }
        else
        {
            memcpy(OlePres.byUnknown1, pOlePresStreamHeaderEmpty, sizeof(pOlePresStreamHeaderEmpty));
            nHeaderSize = sizeof(pOlePresStreamHeaderEmpty);
        }
        /* Set width and height of the metafile */
        OlePres.dwExtentX = dwExtentX;
        OlePres.dwExtentY = -dwExtentY;

        /* Set Data and Length */
        if(dwDataLength > sizeof(METAFILEPICT16))
        {
            OlePres.dwSize = dwDataLength - sizeof(METAFILEPICT16);
            OlePres.pData = &(pData[8]);
        }
        /* Save OlePres000 Data to Stream */
        hRes = IStream_Write(pStream, OlePres.byUnknown1, nHeaderSize, NULL);
        hRes = IStream_Write(pStream, &(OlePres.dwExtentX), sizeof(OlePres.dwExtentX), NULL);
        hRes = IStream_Write(pStream, &(OlePres.dwExtentY), sizeof(OlePres.dwExtentY), NULL);
        hRes = IStream_Write(pStream, &(OlePres.dwSize), sizeof(OlePres.dwSize), NULL);
        if(OlePres.dwSize > 0)
        {
            hRes = IStream_Write(pStream, OlePres.pData, OlePres.dwSize, NULL);
        }
        IStream_Release(pStream);
    }
}

/*************************************************************************
 * OLECONVERT_CreateOle10NativeStream [Internal]
 *
 * Creates the "\001Ole10Native" Stream (should contain a BMP)
 *
 * PARAMS
 *     pStorage     [I] Dest storage to create the stream in
 *     pData        [I] Ole10 Native Data (ex. bmp)
 *     dwDataLength [I] Size of the Ole10 Native Data
 *
 * RETURNS
 *     Nothing
 *
 * NOTES
 *     This function is used by OleConvertOLESTREAMToIStorage only.
 *
 *     Might need to verify the data and return appropriate error message
 *
 */
static void OLECONVERT_CreateOle10NativeStream(LPSTORAGE pStorage, const BYTE *pData, DWORD dwDataLength)
{
    HRESULT hRes;
    IStream *pStream;

    /* Create the Ole10Native Stream */
    hRes = IStorage_CreateStream(pStorage, L"\1Ole10Native",
        STGM_CREATE | STGM_WRITE  | STGM_SHARE_EXCLUSIVE, 0, 0, &pStream );

    if(hRes == S_OK)
    {
        /* Write info to stream */
        hRes = IStream_Write(pStream, &dwDataLength, sizeof(dwDataLength), NULL);
        hRes = IStream_Write(pStream, pData, dwDataLength, NULL);
        IStream_Release(pStream);
    }

}

/*************************************************************************
 * OLECONVERT_GetOLE10ProgID [Internal]
 *
 * Finds the ProgID (or OleTypeID) from the IStorage
 *
 * PARAMS
 *     pStorage        [I] The Src IStorage to get the ProgID
 *     strProgID       [I] the ProgID string to get
 *     dwSize          [I] the size of the string
 *
 * RETURNS
 *     Success:  S_OK
 *     Failure:  REGDB_E_CLASSNOTREG if cannot reconstruct the stream
 *
 * NOTES
 *     This function is used by OleConvertIStorageToOLESTREAM only.
 *
 *
 */
static HRESULT OLECONVERT_GetOLE10ProgID(LPSTORAGE pStorage, char *strProgID, DWORD *dwSize)
{
    HRESULT hRes;
    IStream *pStream;
    LARGE_INTEGER iSeekPos;
    OLECONVERT_ISTORAGE_COMPOBJ CompObj;

    /* Open the CompObj Stream */
    hRes = IStorage_OpenStream(pStorage, L"\1CompObj", NULL,
        STGM_READ  | STGM_SHARE_EXCLUSIVE, 0, &pStream );
    if(hRes == S_OK)
    {

        /*Get the OleType from the CompObj Stream */
        iSeekPos.LowPart = sizeof(CompObj.byUnknown1) + sizeof(CompObj.clsid);
        iSeekPos.HighPart = 0;

        IStream_Seek(pStream, iSeekPos, STREAM_SEEK_SET, NULL);
        IStream_Read(pStream, &CompObj.dwCLSIDNameLength, sizeof(CompObj.dwCLSIDNameLength), NULL);
        iSeekPos.LowPart = CompObj.dwCLSIDNameLength;
        IStream_Seek(pStream, iSeekPos, STREAM_SEEK_CUR , NULL);
        IStream_Read(pStream, &CompObj.dwOleTypeNameLength, sizeof(CompObj.dwOleTypeNameLength), NULL);
        iSeekPos.LowPart = CompObj.dwOleTypeNameLength;
        IStream_Seek(pStream, iSeekPos, STREAM_SEEK_CUR , NULL);

        IStream_Read(pStream, dwSize, sizeof(*dwSize), NULL);
        if(*dwSize > 0)
        {
            IStream_Read(pStream, strProgID, *dwSize, NULL);
        }
        IStream_Release(pStream);
    }
    else
    {
        STATSTG stat;
        LPOLESTR wstrProgID;

        /* Get the OleType from the registry */
        REFCLSID clsid = &(stat.clsid);
        IStorage_Stat(pStorage, &stat, STATFLAG_NONAME);
        hRes = ProgIDFromCLSID(clsid, &wstrProgID);
        if(hRes == S_OK)
        {
            *dwSize = WideCharToMultiByte(CP_ACP, 0, wstrProgID, -1, strProgID, *dwSize, NULL, FALSE);
            CoTaskMemFree(wstrProgID);
        }

    }
    return hRes;
}

/*************************************************************************
 * OLECONVERT_GetOle10PresData [Internal]
 *
 * Converts IStorage "/001Ole10Native" stream to a OLE10 Stream
 *
 * PARAMS
 *     pStorage     [I] Src IStroage
 *     pOleStream   [I] Dest OleStream Mem Struct
 *
 * RETURNS
 *     Nothing
 *
 * NOTES
 *     This function is used by OleConvertIStorageToOLESTREAM only.
 *
 *     Memory allocated for pData must be freed by the caller
 *
 *
 */
static void OLECONVERT_GetOle10PresData(LPSTORAGE pStorage, OLECONVERT_OLESTREAM_DATA *pOleStreamData)
{

    HRESULT hRes;
    IStream *pStream;

    /* Initialize Default data for OLESTREAM */
    pOleStreamData[0].dwOleID = OLESTREAM_ID;
    pOleStreamData[0].dwTypeID = 2;
    pOleStreamData[1].dwOleID = OLESTREAM_ID;
    pOleStreamData[1].dwTypeID = 0;
    pOleStreamData[0].dwMetaFileWidth = 0;
    pOleStreamData[0].dwMetaFileHeight = 0;
    pOleStreamData[0].pData = NULL;
    pOleStreamData[1].pData = NULL;

    /* Open Ole10Native Stream */
    hRes = IStorage_OpenStream(pStorage, L"\1Ole10Native", NULL,
        STGM_READ  | STGM_SHARE_EXCLUSIVE, 0, &pStream );
    if(hRes == S_OK)
    {

        /* Read Size and Data */
        IStream_Read(pStream, &(pOleStreamData->dwDataLength), sizeof(pOleStreamData->dwDataLength), NULL);
        if(pOleStreamData->dwDataLength > 0)
        {
            pOleStreamData->pData = HeapAlloc(GetProcessHeap(),0,pOleStreamData->dwDataLength);
            IStream_Read(pStream, pOleStreamData->pData, pOleStreamData->dwDataLength, NULL);
        }
        IStream_Release(pStream);
    }

}


/*************************************************************************
 * OLECONVERT_GetOle20PresData[Internal]
 *
 * Converts IStorage "/002OlePres000" stream to a OLE10 Stream
 *
 * PARAMS
 *     pStorage         [I] Src IStroage
 *     pOleStreamData   [I] Dest OleStream Mem Struct
 *
 * RETURNS
 *     Nothing
 *
 * NOTES
 *     This function is used by OleConvertIStorageToOLESTREAM only.
 *
 *     Memory allocated for pData must be freed by the caller
 */
static void OLECONVERT_GetOle20PresData(LPSTORAGE pStorage, OLECONVERT_OLESTREAM_DATA *pOleStreamData)
{
    HRESULT hRes;
    IStream *pStream;
    OLECONVERT_ISTORAGE_OLEPRES olePress;

    /* Initialize Default data for OLESTREAM */
    pOleStreamData[0].dwOleID = OLESTREAM_ID;
    pOleStreamData[0].dwTypeID = 2;
    pOleStreamData[0].dwMetaFileWidth = 0;
    pOleStreamData[0].dwMetaFileHeight = 0;
    pOleStreamData[0].dwDataLength = OLECONVERT_WriteOLE20ToBuffer(pStorage, &(pOleStreamData[0].pData));
    pOleStreamData[1].dwOleID = OLESTREAM_ID;
    pOleStreamData[1].dwTypeID = 0;
    pOleStreamData[1].dwOleTypeNameLength = 0;
    pOleStreamData[1].strOleTypeName[0] = 0;
    pOleStreamData[1].dwMetaFileWidth = 0;
    pOleStreamData[1].dwMetaFileHeight = 0;
    pOleStreamData[1].pData = NULL;
    pOleStreamData[1].dwDataLength = 0;


    /* Open OlePress000 stream */
    hRes = IStorage_OpenStream(pStorage, L"\2OlePres000", NULL,
        STGM_READ  | STGM_SHARE_EXCLUSIVE, 0, &pStream );
    if(hRes == S_OK)
    {
        LARGE_INTEGER iSeekPos;
        METAFILEPICT16 MetaFilePict;
        static const char strMetafilePictName[] = "METAFILEPICT";

        /* Set the TypeID for a Metafile */
        pOleStreamData[1].dwTypeID = 5;

        /* Set the OleTypeName to Metafile */
        pOleStreamData[1].dwOleTypeNameLength = strlen(strMetafilePictName) +1;
        strcpy(pOleStreamData[1].strOleTypeName, strMetafilePictName);

        iSeekPos.HighPart = 0;
        iSeekPos.LowPart = sizeof(olePress.byUnknown1);

        /* Get Presentation Data */
        IStream_Seek(pStream, iSeekPos, STREAM_SEEK_SET, NULL);
        IStream_Read(pStream, &(olePress.dwExtentX), sizeof(olePress.dwExtentX), NULL);
        IStream_Read(pStream, &(olePress.dwExtentY), sizeof(olePress.dwExtentY), NULL);
        IStream_Read(pStream, &(olePress.dwSize), sizeof(olePress.dwSize), NULL);

        /*Set width and Height */
        pOleStreamData[1].dwMetaFileWidth = olePress.dwExtentX;
        pOleStreamData[1].dwMetaFileHeight = -olePress.dwExtentY;
        if(olePress.dwSize > 0)
        {
            /* Set Length */
            pOleStreamData[1].dwDataLength  = olePress.dwSize + sizeof(METAFILEPICT16);

            /* Set MetaFilePict struct */
            MetaFilePict.mm = 8;
            MetaFilePict.xExt = olePress.dwExtentX;
            MetaFilePict.yExt = olePress.dwExtentY;
            MetaFilePict.hMF = 0;

            /* Get Metafile Data */
            pOleStreamData[1].pData = HeapAlloc(GetProcessHeap(),0,pOleStreamData[1].dwDataLength);
            memcpy(pOleStreamData[1].pData, &MetaFilePict, sizeof(MetaFilePict));
            IStream_Read(pStream, &(pOleStreamData[1].pData[sizeof(MetaFilePict)]), pOleStreamData[1].dwDataLength-sizeof(METAFILEPICT16), NULL);
        }
        IStream_Release(pStream);
    }
}

/*************************************************************************
 * OleConvertOLESTREAMToIStorage [OLE32.@]
 *
 * Read info on MSDN
 *
 * TODO
 *      DVTARGETDEVICE parameter is not handled
 *      Still unsure of some mem fields for OLE 10 Stream
 *      Still some unknowns for the IStorage: "\002OlePres000", "\001CompObj",
 *      and "\001OLE" streams
 *
 */
HRESULT WINAPI OleConvertOLESTREAMToIStorage (
    LPOLESTREAM pOleStream,
    LPSTORAGE pstg,
    const DVTARGETDEVICE* ptd)
{
    int i;
    HRESULT hRes=S_OK;
    OLECONVERT_OLESTREAM_DATA pOleStreamData[2];

    TRACE("%p %p %p\n", pOleStream, pstg, ptd);

    memset(pOleStreamData, 0, sizeof(pOleStreamData));

    if(ptd != NULL)
    {
        FIXME("DVTARGETDEVICE is not NULL, unhandled parameter\n");
    }

    if(pstg == NULL || pOleStream == NULL)
    {
        hRes = E_INVALIDARG;
    }

    if(hRes == S_OK)
    {
        /* Load the OLESTREAM to Memory */
        hRes = OLECONVERT_LoadOLE10(pOleStream, &pOleStreamData[0], TRUE);
    }

    if(hRes == S_OK)
    {
        /* Load the OLESTREAM to Memory (part 2)*/
        hRes = OLECONVERT_LoadOLE10(pOleStream, &pOleStreamData[1], FALSE);
    }

    if(hRes == S_OK)
    {

        if(pOleStreamData[0].dwDataLength > sizeof(STORAGE_magic))
        {
            /* Do we have the IStorage Data in the OLESTREAM */
            if(memcmp(pOleStreamData[0].pData, STORAGE_magic, sizeof(STORAGE_magic)) ==0)
            {
                OLECONVERT_GetOLE20FromOLE10(pstg, pOleStreamData[0].pData, pOleStreamData[0].dwDataLength);
                OLECONVERT_CreateOlePresStream(pstg, pOleStreamData[1].dwMetaFileWidth, pOleStreamData[1].dwMetaFileHeight, pOleStreamData[1].pData, pOleStreamData[1].dwDataLength);
            }
            else
            {
                /* It must be an original OLE 1.0 source */
                OLECONVERT_CreateOle10NativeStream(pstg, pOleStreamData[0].pData, pOleStreamData[0].dwDataLength);
            }
        }
        else
        {
            /* It must be an original OLE 1.0 source */
            OLECONVERT_CreateOle10NativeStream(pstg, pOleStreamData[0].pData, pOleStreamData[0].dwDataLength);
        }

        /* Create CompObj Stream if necessary */
        hRes = OLECONVERT_CreateCompObjStream(pstg, pOleStreamData[0].strOleTypeName);
        if(hRes == S_OK)
        {
            /*Create the Ole Stream if necessary */
            STORAGE_CreateOleStream(pstg, 0);
        }
    }


    /* Free allocated memory */
    for(i=0; i < 2; i++)
    {
        HeapFree(GetProcessHeap(),0,pOleStreamData[i].pData);
        HeapFree(GetProcessHeap(),0,pOleStreamData[i].pstrOleObjFileName);
        pOleStreamData[i].pstrOleObjFileName = NULL;
    }
    return hRes;
}

/*************************************************************************
 * OleConvertIStorageToOLESTREAM [OLE32.@]
 *
 * Read info on MSDN
 *
 * Read info on MSDN
 *
 * TODO
 *      Still unsure of some mem fields for OLE 10 Stream
 *      Still some unknowns for the IStorage: "\002OlePres000", "\001CompObj",
 *      and "\001OLE" streams.
 *
 */
HRESULT WINAPI OleConvertIStorageToOLESTREAM (
    LPSTORAGE pstg,
    LPOLESTREAM pOleStream)
{
    int i;
    HRESULT hRes = S_OK;
    IStream *pStream;
    OLECONVERT_OLESTREAM_DATA pOleStreamData[2];

    TRACE("%p %p\n", pstg, pOleStream);

    memset(pOleStreamData, 0, sizeof(pOleStreamData));

    if(pstg == NULL || pOleStream == NULL)
    {
        hRes = E_INVALIDARG;
    }
    if(hRes == S_OK)
    {
        /* Get the ProgID */
        pOleStreamData[0].dwOleTypeNameLength = OLESTREAM_MAX_STR_LEN;
        hRes = OLECONVERT_GetOLE10ProgID(pstg, pOleStreamData[0].strOleTypeName, &(pOleStreamData[0].dwOleTypeNameLength));
    }
    if(hRes == S_OK)
    {
        /* Was it originally Ole10 */
        hRes = IStorage_OpenStream(pstg, L"\1Ole10Native", 0, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream);
        if(hRes == S_OK)
        {
            IStream_Release(pStream);
            /* Get Presentation Data for Ole10Native */
            OLECONVERT_GetOle10PresData(pstg, pOleStreamData);
        }
        else
        {
            /* Get Presentation Data (OLE20) */
            OLECONVERT_GetOle20PresData(pstg, pOleStreamData);
        }

        /* Save OLESTREAM */
        hRes = OLECONVERT_SaveOLE10(&(pOleStreamData[0]), pOleStream);
        if(hRes == S_OK)
        {
            hRes = OLECONVERT_SaveOLE10(&(pOleStreamData[1]), pOleStream);
        }

    }

    /* Free allocated memory */
    for(i=0; i < 2; i++)
    {
        HeapFree(GetProcessHeap(),0,pOleStreamData[i].pData);
    }

    return hRes;
}

enum stream_1ole_flags {
    OleStream_LinkedObject = 0x00000001,
    OleStream_Convert      = 0x00000004
};

/*************************************************************************
 * OleConvertIStorageToOLESTREAMEx [OLE32.@]
 */
HRESULT WINAPI OleConvertIStorageToOLESTREAMEx ( LPSTORAGE stg, CLIPFORMAT cf, LONG width, LONG height,
                                                 DWORD size, LPSTGMEDIUM medium, LPOLESTREAM olestream )
{
    FIXME("%p, %x, %ld, %ld, %ld, %p, %p: stub\n", stg, cf, width, height, size, medium, olestream);

    return E_NOTIMPL;
}

/***********************************************************************
 *		SetConvertStg (OLE32.@)
 */
HRESULT WINAPI SetConvertStg(IStorage *storage, BOOL convert)
{
    DWORD flags = convert ? OleStream_Convert : 0;
    IStream *stream;
    DWORD header[2];
    HRESULT hr;

    TRACE("(%p, %d)\n", storage, convert);

    hr = IStorage_OpenStream(storage, L"\1Ole", NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &stream);
    if (FAILED(hr))
    {
        if (hr != STG_E_FILENOTFOUND)
            return hr;

        return STORAGE_CreateOleStream(storage, flags);
    }

    hr = IStream_Read(stream, header, sizeof(header), NULL);
    if (FAILED(hr))
    {
        IStream_Release(stream);
        return hr;
    }

    /* update flag if differs */
    if ((header[1] ^ flags) & OleStream_Convert)
    {
        LARGE_INTEGER pos = {{0}};

        if (header[1] & OleStream_Convert)
            flags = header[1] & ~OleStream_Convert;
        else
            flags = header[1] |  OleStream_Convert;

        pos.QuadPart = sizeof(DWORD);
        hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
        if (FAILED(hr))
        {
            IStream_Release(stream);
            return hr;
        }

        hr = IStream_Write(stream, &flags, sizeof(flags), NULL);
    }

    IStream_Release(stream);
    return hr;
}
