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
static const ULONG OFFSET_BIGBLOCKSIZEBITS   = 0x0000001e;
static const ULONG OFFSET_SMALLBLOCKSIZEBITS = 0x00000020;
static const ULONG OFFSET_BBDEPOTCOUNT	     = 0x0000002C;
static const ULONG OFFSET_ROOTSTARTBLOCK     = 0x00000030;
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
static const WORD  DEF_BIG_BLOCK_SIZE_BITS   = 0x0009;
static const WORD  MIN_BIG_BLOCK_SIZE_BITS   = 0x0009;
static const WORD  MAX_BIG_BLOCK_SIZE_BITS   = 0x000c;
static const WORD  DEF_SMALL_BLOCK_SIZE_BITS = 0x0006;
static const WORD  DEF_BIG_BLOCK_SIZE        = 0x0200;
static const WORD  DEF_SMALL_BLOCK_SIZE      = 0x0040;
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

/*************************************************************************
 * Big Block File support
 *
 * The big block file is an abstraction of a flat file separated in
 * same sized blocks. The implementation for the methods described in
 * this section appear in stg_bigblockfile.c
 */

typedef struct BigBlockFile BigBlockFile,*LPBIGBLOCKFILE;

/*
 * Declaration of the functions used to manipulate the BigBlockFile
 * data structure.
 */
BigBlockFile*  BIGBLOCKFILE_Construct(HANDLE hFile,
                                      ILockBytes* pLkByt,
                                      DWORD openFlags,
                                      BOOL fileBased);
void           BIGBLOCKFILE_Destructor(LPBIGBLOCKFILE This);
HRESULT        BIGBLOCKFILE_Expand(LPBIGBLOCKFILE This, ULARGE_INTEGER newSize);
HRESULT        BIGBLOCKFILE_SetSize(LPBIGBLOCKFILE This, ULARGE_INTEGER newSize);
HRESULT        BIGBLOCKFILE_ReadAt(LPBIGBLOCKFILE This, ULARGE_INTEGER offset,
           void* buffer, ULONG size, ULONG* bytesRead);
HRESULT        BIGBLOCKFILE_WriteAt(LPBIGBLOCKFILE This, ULARGE_INTEGER offset,
           const void* buffer, ULONG size, ULONG* bytesRead);

/*************************************************************************
 * Ole Convert support
 */

void OLECONVERT_CreateOleStream(LPSTORAGE pStorage);
HRESULT OLECONVERT_CreateCompObjStream(LPSTORAGE pStorage, LPCSTR strOleTypeName);


/****************************************************************************
 * Storage32BaseImpl definitions.
 *
 * This structure defines the base information contained in all implementations
 * of IStorage32 contained in this file storage implementation.
 *
 * In OOP terms, this is the base class for all the IStorage32 implementations
 * contained in this file.
 */
struct StorageBaseImpl
{
  const IStorageVtbl *lpVtbl;    /* Needs to be the first item in the struct
			    * since we want to cast this in a Storage32 pointer */

  const IPropertySetStorageVtbl *pssVtbl; /* interface for adding a properties stream */

  /*
   * Stream tracking list
   */

  struct list strmHead;

  /*
   * Storage tracking list
   */
  struct list storageHead;

  /*
   * Reference count of this object
   */
  LONG ref;

  /*
   * TRUE if this object has been invalidated
   */
  int reverted;

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

  /* If set, this overrides the root storage name returned by IStorage_Stat */
  LPCWSTR          filename;

  BOOL             create;     /* Was the storage created or opened.
                                  The behaviour of STGM_SIMPLE depends on this */
  /*
   * If this storage was opened in transacted mode, the object that implements
   * the transacted snapshot or cache.
   */
  StorageBaseImpl *transactedChild;
};

/* virtual methods for StorageBaseImpl objects */
struct StorageBaseImplVtbl {
  void (*Destroy)(StorageBaseImpl*);
  void (*Invalidate)(StorageBaseImpl*);
  HRESULT (*CreateDirEntry)(StorageBaseImpl*,const DirEntry*,DirRef*);
  HRESULT (*WriteDirEntry)(StorageBaseImpl*,DirRef,const DirEntry*);
  HRESULT (*ReadDirEntry)(StorageBaseImpl*,DirRef,DirEntry*);
  HRESULT (*DestroyDirEntry)(StorageBaseImpl*,DirRef);
  HRESULT (*StreamReadAt)(StorageBaseImpl*,DirRef,ULARGE_INTEGER,ULONG,void*,ULONG*);
  HRESULT (*StreamWriteAt)(StorageBaseImpl*,DirRef,ULARGE_INTEGER,ULONG,const void*,ULONG*);
  HRESULT (*StreamSetSize)(StorageBaseImpl*,DirRef,ULARGE_INTEGER);
};

static inline void StorageBaseImpl_Destroy(StorageBaseImpl *This)
{
  This->baseVtbl->Destroy(This);
}

static inline void StorageBaseImpl_Invalidate(StorageBaseImpl *This)
{
  This->baseVtbl->Invalidate(This);
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

/****************************************************************************
 * StorageBaseImpl stream list handlers
 */

void StorageBaseImpl_AddStream(StorageBaseImpl * stg, StgStreamImpl * strm);
void StorageBaseImpl_RemoveStream(StorageBaseImpl * stg, StgStreamImpl * strm);

/* Number of BlockChainStream objects to cache in a StorageImpl */
#define BLOCKCHAIN_CACHE_SIZE 4

/****************************************************************************
 * Storage32Impl definitions.
 *
 * This implementation of the IStorage32 interface represents a root
 * storage. Basically, a document file.
 */
struct StorageImpl
{
  struct StorageBaseImpl base;

  /*
   * The following data members are specific to the Storage32Impl
   * class
   */
  HANDLE           hFile;      /* Physical support for the Docfile */
  LPOLESTR         pwcsName;   /* Full path of the document file */

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
  ULONG extBigBlockDepotCount;
  ULONG bigBlockDepotStart[COUNT_BBDEPOTINHEADER];

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

  /*
   * Pointer to the big block file abstraction
   */
  BigBlockFile* bigBlockFile;
};

HRESULT StorageImpl_ReadRawDirEntry(
            StorageImpl *This,
            ULONG index,
            BYTE *buffer);

void UpdateRawDirEntry(
    BYTE *buffer,
    const DirEntry *newData);

HRESULT StorageImpl_WriteRawDirEntry(
            StorageImpl *This,
            ULONG index,
            const BYTE *buffer);

HRESULT StorageImpl_ReadDirEntry(
            StorageImpl*    This,
            DirRef          index,
            DirEntry*       buffer);

HRESULT StorageImpl_WriteDirEntry(
            StorageImpl*        This,
            DirRef              index,
            const DirEntry*     buffer);

BlockChainStream* Storage32Impl_SmallBlocksToBigBlocks(
                      StorageImpl* This,
                      SmallBlockChainStream** ppsbChain);

SmallBlockChainStream* Storage32Impl_BigBlocksToSmallBlocks(
                      StorageImpl* This,
                      BlockChainStream** ppbbChain);

/****************************************************************************
 * StgStreamImpl definitions.
 *
 * This class implements the IStream32 interface and represents a stream
 * located inside a storage object.
 */
struct StgStreamImpl
{
  const IStreamVtbl *lpVtbl;  /* Needs to be the first item in the struct
			 * since we want to cast this to an IStream pointer */

  /*
   * We are an entry in the storage object's stream handler list
   */

  struct list StrmListEntry;

  /*
   * Reference count
   */
  LONG		     ref;

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

/*
 * Method definition for the StgStreamImpl class.
 */
StgStreamImpl* StgStreamImpl_Construct(
		StorageBaseImpl* parentStorage,
    DWORD            grfMode,
    DirRef           dirEntry);


/******************************************************************************
 * Endian conversion macros
 */
#ifdef WORDS_BIGENDIAN

#define htole32(x) RtlUlongByteSwap(x)
#define htole16(x) RtlUshortByteSwap(x)
#define lendian32toh(x) RtlUlongByteSwap(x)
#define lendian16toh(x) RtlUshortByteSwap(x)

#else

#define htole32(x) (x)
#define htole16(x) (x)
#define lendian32toh(x) (x)
#define lendian16toh(x) (x)

#endif

/******************************************************************************
 * The StorageUtl_ functions are miscellaneous utility functions. Most of which
 * are abstractions used to read values from file buffers without having to
 * worry about bit order
 */
void StorageUtl_ReadWord(const BYTE* buffer, ULONG offset, WORD* value);
void StorageUtl_WriteWord(BYTE* buffer, ULONG offset, WORD value);
void StorageUtl_ReadDWord(const BYTE* buffer, ULONG offset, DWORD* value);
void StorageUtl_WriteDWord(BYTE* buffer, ULONG offset, DWORD value);
void StorageUtl_ReadULargeInteger(const BYTE* buffer, ULONG offset,
 ULARGE_INTEGER* value);
void StorageUtl_WriteULargeInteger(BYTE* buffer, ULONG offset,
 const ULARGE_INTEGER *value);
void StorageUtl_ReadGUID(const BYTE* buffer, ULONG offset, GUID* value);
void StorageUtl_WriteGUID(BYTE* buffer, ULONG offset, const GUID* value);
void StorageUtl_CopyDirEntryToSTATSTG(StorageBaseImpl *storage,STATSTG* destination,
 const DirEntry* source, int statFlags);

/****************************************************************************
 * BlockChainStream definitions.
 *
 * The BlockChainStream class is a utility class that is used to create an
 * abstraction of the big block chains in the storage file.
 */
struct BlockChainStream
{
  StorageImpl* parentStorage;
  ULONG*       headOfStreamPlaceHolder;
  DirRef       ownerDirEntry;
  ULONG        lastBlockNoInSequence;
  ULONG        lastBlockNoInSequenceIndex;
  ULONG        tailIndex;
  ULONG        numBlocks;
};

/*
 * Methods for the BlockChainStream class.
 */
BlockChainStream* BlockChainStream_Construct(
		StorageImpl* parentStorage,
		ULONG*         headOfStreamPlaceHolder,
		DirRef         dirEntry);

void BlockChainStream_Destroy(
		BlockChainStream* This);

HRESULT BlockChainStream_ReadAt(
		BlockChainStream* This,
		ULARGE_INTEGER offset,
		ULONG          size,
		void*          buffer,
		ULONG*         bytesRead);

HRESULT BlockChainStream_WriteAt(
		BlockChainStream* This,
		ULARGE_INTEGER offset,
		ULONG          size,
		const void*    buffer,
		ULONG*         bytesWritten);

BOOL BlockChainStream_SetSize(
		BlockChainStream* This,
		ULARGE_INTEGER    newSize);

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

/*
 * Methods of the SmallBlockChainStream class.
 */
SmallBlockChainStream* SmallBlockChainStream_Construct(
           StorageImpl*   parentStorage,
           ULONG*         headOfStreamPlaceHolder,
           DirRef         dirEntry);

void SmallBlockChainStream_Destroy(
	       SmallBlockChainStream* This);

HRESULT SmallBlockChainStream_ReadAt(
	       SmallBlockChainStream* This,
	       ULARGE_INTEGER offset,
	       ULONG          size,
	       void*          buffer,
	       ULONG*         bytesRead);

HRESULT SmallBlockChainStream_WriteAt(
	       SmallBlockChainStream* This,
	       ULARGE_INTEGER offset,
	       ULONG          size,
	       const void*    buffer,
	       ULONG*         bytesWritten);

BOOL SmallBlockChainStream_SetSize(
	       SmallBlockChainStream* This,
	       ULARGE_INTEGER          newSize);


#endif /* __STORAGE32_H__ */
