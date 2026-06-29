/*
 * Compound Storage (32 bit version)
 *
 * Implemented using the documentation of the LAOLA project at
 * <URL:http://wwwwbs.cs.tu-berlin.de/~schwartz/pmh/index.html>
 * (Thanks to Martin Schwartz <schwartz@cs.tu-berlin.de>)
 *
 * This include file contains definitions of types and function
 * prototypes that are used in the many files implementing the
 * storage functionality
 *
 * Copyright 1998,1999 Francis Beaudet
 * Copyright 1998,1999 Thuy Nguyen
 * Copyright 2010 Vincent Povirk for CodeWeavers
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
 */
#ifndef __STORAGE32_H__
#define __STORAGE32_H__

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "objbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/list.h"

/*
 * Definitions for the file format offsets.
 */
static const ULONG OFFSET_MINORVERSION       = 0x00000018;
static const ULONG OFFSET_MAJORVERSION       = 0x0000001a;
static const ULONG OFFSET_BYTEORDERMARKER    = 0x0000001c;
static const ULONG OFFSET_BIGBLOCKSIZEBITS   = 0x0000001e;
static const ULONG OFFSET_SMALLBLOCKSIZEBITS = 0x00000020;
static const ULONG OFFSET_DIRSECTORCOUNT     = 0x00000028;
static const ULONG OFFSET_BBDEPOTCOUNT	     = 0x0000002C;
static const ULONG OFFSET_ROOTSTARTBLOCK     = 0x00000030;
static const ULONG OFFSET_TRANSACTIONSIG     = 0x00000034;
static const ULONG OFFSET_SMALLBLOCKLIMIT    = 0x00000038;
static const ULONG OFFSET_SBDEPOTSTART	     = 0x0000003C;
static const ULONG OFFSET_SBDEPOTCOUNT       = 0x00000040;
static const ULONG OFFSET_EXTBBDEPOTSTART    = 0x00000044;
static const ULONG OFFSET_EXTBBDEPOTCOUNT    = 0x00000048;
static const ULONG OFFSET_BBDEPOTSTART	     = 0x0000004C;
static const ULONG OFFSET_PS_NAME            = 0x00000000;
static const ULONG OFFSET_PS_NAMELENGTH	     = 0x00000040;
static const ULONG OFFSET_PS_STGTYPE         = 0x00000042;
static const ULONG OFFSET_PS_LEFTCHILD       = 0x00000044;
static const ULONG OFFSET_PS_RIGHTCHILD      = 0x00000048;
static const ULONG OFFSET_PS_DIRROOT	     = 0x0000004C;
static const ULONG OFFSET_PS_GUID            = 0x00000050;
static const ULONG OFFSET_PS_CTIMELOW        = 0x00000064;
static const ULONG OFFSET_PS_CTIMEHIGH       = 0x00000068;
static const ULONG OFFSET_PS_MTIMELOW        = 0x0000006C;
static const ULONG OFFSET_PS_MTIMEHIGH       = 0x00000070;
static const ULONG OFFSET_PS_STARTBLOCK	     = 0x00000074;
static const ULONG OFFSET_PS_SIZE	     = 0x00000078;
static const ULONG OFFSET_PS_SIZE_HIGH	     = 0x0000007C;
static const WORD  DEF_BIG_BLOCK_SIZE_BITS   = 0x0009;
static const WORD  MIN_BIG_BLOCK_SIZE_BITS   = 0x0009;
static const WORD  MAX_BIG_BLOCK_SIZE_BITS   = 0x000c;
static const WORD  DEF_SMALL_BLOCK_SIZE_BITS = 0x0006;
static const WORD  DEF_BIG_BLOCK_SIZE        = 0x0200;
static const WORD  DEF_SMALL_BLOCK_SIZE      = 0x0040;
static const ULONG BLOCK_FIRST_SPECIAL       = 0xFFFFFFFB;
static const ULONG BLOCK_EXTBBDEPOT          = 0xFFFFFFFC;
static const ULONG BLOCK_SPECIAL             = 0xFFFFFFFD;
static const ULONG BLOCK_END_OF_CHAIN        = 0xFFFFFFFE;
static const ULONG BLOCK_UNUSED              = 0xFFFFFFFF;
static const ULONG DIRENTRY_NULL             = 0xFFFFFFFF;

#define DIRENTRY_NAME_MAX_LEN    0x20
#define DIRENTRY_NAME_BUFFER_LEN 0x40

#define RAW_DIRENTRY_SIZE 0x00000080

#define HEADER_SIZE 512

#define MIN_BIG_BLOCK_SIZE 0x200
#define MAX_BIG_BLOCK_SIZE 0x1000

/*
 * Type of child entry link
 */
#define DIRENTRY_RELATION_PREVIOUS 0
#define DIRENTRY_RELATION_NEXT     1
#define DIRENTRY_RELATION_DIR      2

/*
 * type constant used in files for the root storage
 */
#define STGTY_ROOT 0x05

#define COUNT_BBDEPOTINHEADER    109

/* FIXME: This value is stored in the header, but we hard-code it to 0x1000. */
#define LIMIT_TO_USE_SMALL_BLOCK 0x1000

#define STGM_ACCESS_MODE(stgm)   ((stgm)&0x0000f)
#define STGM_SHARE_MODE(stgm)    ((stgm)&0x000f0)
#define STGM_CREATE_MODE(stgm)   ((stgm)&0x0f000)

#define STGM_KNOWN_FLAGS (0xf0ff | \
     STGM_TRANSACTED | STGM_CONVERT | STGM_PRIORITY | STGM_NOSCRATCH | \
     STGM_NOSNAPSHOT | STGM_DIRECT_SWMR | STGM_DELETEONRELEASE | STGM_SIMPLE)

/*
 * Forward declarations of all the structures used by the storage
 * module.
 */
typedef struct StorageBaseImpl     StorageBaseImpl;
typedef struct StorageBaseImplVtbl StorageBaseImplVtbl;
typedef struct StorageImpl         StorageImpl;
typedef struct BlockChainStream      BlockChainStream;
typedef struct SmallBlockChainStream SmallBlockChainStream;
typedef struct IEnumSTATSTGImpl      IEnumSTATSTGImpl;
typedef struct DirEntry              DirEntry;
typedef struct StgStreamImpl         StgStreamImpl;

/*
 * A reference to a directory entry in the file or a transacted cache.
 */
typedef ULONG DirRef;

/*
 * This utility structure is used to read/write the information in a directory
 * entry.
 */
struct DirEntry
{
  WCHAR	         name[DIRENTRY_NAME_MAX_LEN];
  WORD	         sizeOfNameString;
  BYTE	         stgType;
  DirRef         leftChild;
  DirRef         rightChild;
  DirRef         dirRootEntry;
  GUID           clsid;
  FILETIME       ctime;
  FILETIME       mtime;
  ULONG          startingBlock;
  ULARGE_INTEGER size;
};

HRESULT FileLockBytesImpl_Construct(HANDLE hFile, DWORD openFlags, LPCWSTR pwcsName, ILockBytes **pLockBytes);

/*************************************************************************
 * Ole Convert support
 */

HRESULT STORAGE_CreateOleStream(IStorage*, DWORD);
HRESULT OLECONVERT_CreateCompObjStream(LPSTORAGE pStorage, LPCSTR strOleTypeName);

enum swmr_mode
{
  SWMR_None,
  SWMR_Writer,
  SWMR_Reader
};

/****************************************************************************
 * StorageBaseImpl definitions.
 *
 * This structure defines the base information contained in all implementations
 * of IStorage contained in this file storage implementation.
 *
 * In OOP terms, this is the base class for all the IStorage implementations
 * contained in this file.
 */
struct StorageBaseImpl
{
  IStorage IStorage_iface;
  IPropertySetStorage IPropertySetStorage_iface; /* interface for adding a properties stream */
  IDirectWriterLock IDirectWriterLock_iface;
  LONG ref;

  /*
   * Stream tracking list
   */

  struct list strmHead;

  /*
   * Storage tracking list
   */
  struct list storageHead;

  /*
   * TRUE if this object has been invalidated
   */
  BOOL reverted;

  /*
   * Index of the directory entry of this storage
   */
  DirRef storageDirEntry;

  /*
   * virtual methods.
   */
  const StorageBaseImplVtbl *baseVtbl;

  /*
   * flags that this storage was opened or created with
   */
  DWORD openFlags;

  /*
   * State bits appear to only be preserved while running. No in the stream
   */
  DWORD stateBits;

  BOOL  create;     /* Was the storage created or opened.
                       The behaviour of STGM_SIMPLE depends on this */
  /*
   * If this storage was opened in transacted mode, the object that implements
   * the transacted snapshot or cache.
   */
  StorageBaseImpl *transactedChild;
  enum swmr_mode lockingrole;
};

/* virtual methods for StorageBaseImpl objects */
struct StorageBaseImplVtbl {
  void (*Destroy)(StorageBaseImpl*);
  void (*Invalidate)(StorageBaseImpl*);
  HRESULT (*Flush)(StorageBaseImpl*);
  HRESULT (*GetFilename)(StorageBaseImpl*,LPWSTR*);
  HRESULT (*CreateDirEntry)(StorageBaseImpl*,const DirEntry*,DirRef*);
  HRESULT (*WriteDirEntry)(StorageBaseImpl*,DirRef,const DirEntry*);
  HRESULT (*ReadDirEntry)(StorageBaseImpl*,DirRef,DirEntry*);
  HRESULT (*DestroyDirEntry)(StorageBaseImpl*,DirRef);
  HRESULT (*StreamReadAt)(StorageBaseImpl*,DirRef,ULARGE_INTEGER,ULONG,void*,ULONG*);
  HRESULT (*StreamWriteAt)(StorageBaseImpl*,DirRef,ULARGE_INTEGER,ULONG,const void*,ULONG*);
  HRESULT (*StreamSetSize)(StorageBaseImpl*,DirRef,ULARGE_INTEGER);
  HRESULT (*StreamLink)(StorageBaseImpl*,DirRef,DirRef);
  HRESULT (*GetTransactionSig)(StorageBaseImpl*,ULONG*,BOOL);
  HRESULT (*SetTransactionSig)(StorageBaseImpl*,ULONG);
  HRESULT (*LockTransaction)(StorageBaseImpl*,BOOL);
  HRESULT (*UnlockTransaction)(StorageBaseImpl*,BOOL);
};

static inline void StorageBaseImpl_Destroy(StorageBaseImpl *This)
{
  This->baseVtbl->Destroy(This);
}

static inline void StorageBaseImpl_Invalidate(StorageBaseImpl *This)
{
  This->baseVtbl->Invalidate(This);
}

static inline HRESULT StorageBaseImpl_Flush(StorageBaseImpl *This)
{
  return This->baseVtbl->Flush(This);
}

static inline HRESULT StorageBaseImpl_GetFilename(StorageBaseImpl *This, LPWSTR *result)
{
  return This->baseVtbl->GetFilename(This, result);
}

static inline HRESULT StorageBaseImpl_CreateDirEntry(StorageBaseImpl *This,
  const DirEntry *newData, DirRef *index)
{
  return This->baseVtbl->CreateDirEntry(This, newData, index);
}

static inline HRESULT StorageBaseImpl_WriteDirEntry(StorageBaseImpl *This,
  DirRef index, const DirEntry *data)
{
  return This->baseVtbl->WriteDirEntry(This, index, data);
}

static inline HRESULT StorageBaseImpl_ReadDirEntry(StorageBaseImpl *This,
  DirRef index, DirEntry *data)
{
  return This->baseVtbl->ReadDirEntry(This, index, data);
}

static inline HRESULT StorageBaseImpl_DestroyDirEntry(StorageBaseImpl *This,
  DirRef index)
{
  return This->baseVtbl->DestroyDirEntry(This, index);
}

/* Read up to size bytes from this directory entry's stream at the given offset. */
static inline HRESULT StorageBaseImpl_StreamReadAt(StorageBaseImpl *This,
  DirRef index, ULARGE_INTEGER offset, ULONG size, void *buffer, ULONG *bytesRead)
{
  return This->baseVtbl->StreamReadAt(This, index, offset, size, buffer, bytesRead);
}

/* Write size bytes to this directory entry's stream at the given offset,
 * growing the stream if necessary. */
static inline HRESULT StorageBaseImpl_StreamWriteAt(StorageBaseImpl *This,
  DirRef index, ULARGE_INTEGER offset, ULONG size, const void *buffer, ULONG *bytesWritten)
{
  return This->baseVtbl->StreamWriteAt(This, index, offset, size, buffer, bytesWritten);
}

static inline HRESULT StorageBaseImpl_StreamSetSize(StorageBaseImpl *This,
  DirRef index, ULARGE_INTEGER newsize)
{
  return This->baseVtbl->StreamSetSize(This, index, newsize);
}

/* Make dst point to the same stream that src points to. Other stream operations
 * will not work properly for entries that point to the same stream, so this
 * must be a very temporary state, and only one entry pointing to a given stream
 * may be reachable at any given time. */
static inline HRESULT StorageBaseImpl_StreamLink(StorageBaseImpl *This,
  DirRef dst, DirRef src)
{
  return This->baseVtbl->StreamLink(This, dst, src);
}

static inline HRESULT StorageBaseImpl_GetTransactionSig(StorageBaseImpl *This,
  ULONG* result, BOOL refresh)
{
  return This->baseVtbl->GetTransactionSig(This, result, refresh);
}

static inline HRESULT StorageBaseImpl_SetTransactionSig(StorageBaseImpl *This,
  ULONG value)
{
  return This->baseVtbl->SetTransactionSig(This, value);
}

static inline HRESULT StorageBaseImpl_LockTransaction(StorageBaseImpl *This, BOOL write)
{
  return This->baseVtbl->LockTransaction(This, write);
}

static inline HRESULT StorageBaseImpl_UnlockTransaction(StorageBaseImpl *This, BOOL write)
{
  return This->baseVtbl->UnlockTransaction(This, write);
}

/****************************************************************************
 * StorageBaseImpl stream list handlers
 */

void StorageBaseImpl_AddStream(StorageBaseImpl * stg, StgStreamImpl * strm);
void StorageBaseImpl_RemoveStream(StorageBaseImpl * stg, StgStreamImpl * strm);

/* Number of BlockChainStream objects to cache in a StorageImpl */
#define BLOCKCHAIN_CACHE_SIZE 4

/****************************************************************************
 * StorageImpl definitions.
 *
 * This implementation of the IStorage interface represents a root
 * storage. Basically, a document file.
 */
struct StorageImpl
{
  struct StorageBaseImpl base;

  /*
   * File header
   */
  WORD  bigBlockSizeBits;
  WORD  smallBlockSizeBits;
  ULONG bigBlockSize;
  ULONG smallBlockSize;
  ULONG bigBlockDepotCount;
  ULONG rootStartBlock;
  ULONG smallBlockLimit;
  ULONG smallBlockDepotStart;
  ULONG extBigBlockDepotStart;
  ULONG *extBigBlockDepotLocations;
  ULONG extBigBlockDepotLocationsSize;
  ULONG extBigBlockDepotCount;
  ULONG bigBlockDepotStart[COUNT_BBDEPOTINHEADER];
  ULONG transactionSig;

  ULONG extBlockDepotCached[MAX_BIG_BLOCK_SIZE / 4];
  ULONG indexExtBlockDepotCached;

  ULONG blockDepotCached[MAX_BIG_BLOCK_SIZE / 4];
  ULONG indexBlockDepotCached;
  ULONG prevFreeBlock;

  /* All small blocks before this one are known to be in use. */
  ULONG firstFreeSmallBlock;

  /*
   * Abstraction of the big block chains for the chains of the header.
   */
  BlockChainStream* rootBlockChain;
  BlockChainStream* smallBlockDepotChain;
  BlockChainStream* smallBlockRootChain;

  /* Cache of block chain streams objects for directory entries */
  BlockChainStream* blockChainCache[BLOCKCHAIN_CACHE_SIZE];
  UINT blockChainToEvict;

  ULONG locks_supported;

  ILockBytes* lockBytes;

  ULONG locked_bytes[8];
};

/****************************************************************************
 * StgStreamImpl definitions.
 *
 * This class implements the IStream interface and represents a stream
 * located inside a storage object.
 */
struct StgStreamImpl
{
  IStream IStream_iface;
  LONG ref;

  /*
   * We are an entry in the storage object's stream handler list
   */
  struct list StrmListEntry;

  /*
   * Storage that is the parent(owner) of the stream
   */
  StorageBaseImpl* parentStorage;

  /*
   * Access mode of this stream.
   */
  DWORD grfMode;

  /*
   * Index of the directory entry that owns (points to) this stream.
   */
  DirRef             dirEntry;

  /*
   * This is the current position of the cursor in the stream
   */
  ULARGE_INTEGER     currentPosition;
};

static inline StgStreamImpl *impl_from_IStream( IStream *iface )
{
    return CONTAINING_RECORD(iface, StgStreamImpl, IStream_iface);
}

/*
 * Method definition for the StgStreamImpl class.
 */
StgStreamImpl* StgStreamImpl_Construct(
		StorageBaseImpl* parentStorage,
    DWORD            grfMode,
    DirRef           dirEntry);


/* Range lock constants.
 *
 * The storage format reserves the region from 0x7fffff00-0x7fffffff for
 * locking and synchronization. Because it reserves the entire block containing
 * that range, and the minimum block size is 512 bytes, 0x7ffffe00-0x7ffffeff
 * also cannot be used for any other purpose.
 * Unfortunately, the spec doesn't say which bytes
 * within that range are used, and for what. These are guesses based on testing.
 * In particular, ends of ranges may be wrong.

 0x0 through 0x57: Unknown. Causes read-only exclusive opens to fail.
 0x58 through 0x6b: Priority mode.
 0x6c through 0x7f: No snapshot mode.
 0x80: Commit lock.
 0x81 through 0x91: Priority mode, again. Not sure why it uses two regions.
 0x92: Lock-checking lock. Held while opening so ranges can be tested without
  causing spurious failures if others try to grab or test those ranges at the
  same time.
 0x93 through 0xa6: Read mode.
 0xa7 through 0xba: Write mode.
 0xbb through 0xce: Deny read.
 0xcf through 0xe2: Deny write.
 0xe2 through 0xff: Unknown. Causes read-only exclusive opens to fail.
*/

#define RANGELOCK_UNK1_FIRST            0x7ffffe00
#define RANGELOCK_UNK1_LAST             0x7fffff57
#define RANGELOCK_PRIORITY1_FIRST       0x7fffff58
#define RANGELOCK_PRIORITY1_LAST        0x7fffff6b
#define RANGELOCK_NOSNAPSHOT_FIRST      0x7fffff6c
#define RANGELOCK_NOSNAPSHOT_LAST       0x7fffff7f
#define RANGELOCK_COMMIT                0x7fffff80
#define RANGELOCK_PRIORITY2_FIRST       0x7fffff81
#define RANGELOCK_PRIORITY2_LAST        0x7fffff91
#define RANGELOCK_CHECKLOCKS            0x7fffff92
#define RANGELOCK_READ_FIRST            0x7fffff93
#define RANGELOCK_READ_LAST             0x7fffffa6
#define RANGELOCK_WRITE_FIRST           0x7fffffa7
#define RANGELOCK_WRITE_LAST            0x7fffffba
#define RANGELOCK_DENY_READ_FIRST       0x7fffffbb
#define RANGELOCK_DENY_READ_LAST        0x7fffffce
#define RANGELOCK_DENY_WRITE_FIRST      0x7fffffcf
#define RANGELOCK_DENY_WRITE_LAST       0x7fffffe2
#define RANGELOCK_UNK2_FIRST            0x7fffffe3
#define RANGELOCK_UNK2_LAST             0x7fffffff
#define RANGELOCK_TRANSACTION_FIRST     RANGELOCK_COMMIT
#define RANGELOCK_TRANSACTION_LAST      RANGELOCK_CHECKLOCKS
#define RANGELOCK_FIRST                 RANGELOCK_UNK1_FIRST
#define RANGELOCK_LAST                  RANGELOCK_UNK2_LAST

/* internal value for LockRegion/UnlockRegion */
#define WINE_LOCK_READ                  0x80000000


/******************************************************************************
 * Endian conversion macros
 */
#ifdef WORDS_BIGENDIAN

#ifndef htole32
#define htole32(x) RtlUlongByteSwap(x)
#endif
#ifndef htole16
#define htole16(x) RtlUshortByteSwap(x)
#endif
#define lendian32toh(x) RtlUlongByteSwap(x)
#define lendian16toh(x) RtlUshortByteSwap(x)

#else

#ifndef htole32
#define htole32(x) (x)
#endif
#ifndef htole16
#define htole16(x) (x)
#endif
#define lendian32toh(x) (x)
#define lendian16toh(x) (x)

#endif

/******************************************************************************
 * The StorageUtl_ functions are miscellaneous utility functions. Most of which
 * are abstractions used to read values from file buffers without having to
 * worry about bit order
 */
void StorageUtl_ReadWord(const BYTE* buffer, ULONG offset, WORD* value);
void StorageUtl_WriteWord(void *buffer, ULONG offset, WORD value);
void StorageUtl_ReadDWord(const BYTE* buffer, ULONG offset, DWORD* value);
void StorageUtl_WriteDWord(void *buffer, ULONG offset, DWORD value);
void StorageUtl_ReadULargeInteger(const BYTE* buffer, ULONG offset,
 ULARGE_INTEGER* value);
void StorageUtl_WriteULargeInteger(void *buffer, ULONG offset, const ULARGE_INTEGER *value);
void StorageUtl_ReadGUID(const BYTE* buffer, ULONG offset, GUID* value);
void StorageUtl_WriteGUID(void *buffer, ULONG offset, const GUID* value);
void StorageUtl_CopyDirEntryToSTATSTG(StorageBaseImpl *storage,STATSTG* destination,
 const DirEntry* source, int statFlags);


#endif /* __STORAGE32_H__ */
