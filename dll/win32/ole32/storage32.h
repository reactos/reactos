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
static const ULONG OFFSET_SBDEPOTSTART	     = 0x0000003C;
static const ULONG OFFSET_SBDEPOTCOUNT       = 0x00000040;
static const ULONG OFFSET_EXTBBDEPOTSTART    = 0x00000044;
static const ULONG OFFSET_EXTBBDEPOTCOUNT    = 0x00000048;
static const ULONG OFFSET_BBDEPOTSTART	     = 0x0000004C;
static const ULONG OFFSET_PS_NAME            = 0x00000000;
static const ULONG OFFSET_PS_NAMELENGTH	     = 0x00000040;
static const ULONG OFFSET_PS_PROPERTYTYPE    = 0x00000042;
static const ULONG OFFSET_PS_PREVIOUSPROP    = 0x00000044;
static const ULONG OFFSET_PS_NEXTPROP        = 0x00000048;
static const ULONG OFFSET_PS_DIRPROP	     = 0x0000004C;
static const ULONG OFFSET_PS_GUID            = 0x00000050;
static const ULONG OFFSET_PS_TSS1	     = 0x00000064;
static const ULONG OFFSET_PS_TSD1            = 0x00000068;
static const ULONG OFFSET_PS_TSS2            = 0x0000006C;
static const ULONG OFFSET_PS_TSD2            = 0x00000070;
static const ULONG OFFSET_PS_STARTBLOCK	     = 0x00000074;
static const ULONG OFFSET_PS_SIZE	     = 0x00000078;
static const WORD  DEF_BIG_BLOCK_SIZE_BITS   = 0x0009;
static const WORD  DEF_SMALL_BLOCK_SIZE_BITS = 0x0006;
static const WORD  DEF_BIG_BLOCK_SIZE        = 0x0200;
static const WORD  DEF_SMALL_BLOCK_SIZE      = 0x0040;
static const ULONG BLOCK_EXTBBDEPOT          = 0xFFFFFFFC;
static const ULONG BLOCK_SPECIAL             = 0xFFFFFFFD;
static const ULONG BLOCK_END_OF_CHAIN        = 0xFFFFFFFE;
static const ULONG BLOCK_UNUSED              = 0xFFFFFFFF;
static const ULONG PROPERTY_NULL             = 0xFFFFFFFF;

#define PROPERTY_NAME_MAX_LEN    0x20
#define PROPERTY_NAME_BUFFER_LEN 0x40

#define PROPSET_BLOCK_SIZE 0x00000080

/*
 * Property type of relation
 */
#define PROPERTY_RELATION_PREVIOUS 0
#define PROPERTY_RELATION_NEXT     1
#define PROPERTY_RELATION_DIR      2

/*
 * Property type constants
 */
#define PROPTYPE_STORAGE 0x01
#define PROPTYPE_STREAM  0x02
#define PROPTYPE_ROOT    0x05

/*
 * These defines assume a hardcoded blocksize. The code will assert
 * if the blocksize is different. Some changes will have to be done if it
 * becomes the case.
 */
#define BIG_BLOCK_SIZE           0x200
#define COUNT_BBDEPOTINHEADER    109
#define LIMIT_TO_USE_SMALL_BLOCK 0x1000
#define NUM_BLOCKS_PER_DEPOT_BLOCK 128

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
typedef struct StorageImpl         StorageImpl;
typedef struct BlockChainStream      BlockChainStream;
typedef struct SmallBlockChainStream SmallBlockChainStream;
typedef struct IEnumSTATSTGImpl      IEnumSTATSTGImpl;
typedef struct StgProperty           StgProperty;
typedef struct StgStreamImpl         StgStreamImpl;

/*
 * This utility structure is used to read/write the information in a storage
 * property.
 */
struct StgProperty
{
  WCHAR	         name[PROPERTY_NAME_MAX_LEN];
  WORD	         sizeOfNameString;
  BYTE	         propertyType;
  ULONG	         previousProperty;
  ULONG	         nextProperty;
  ULONG          dirProperty;
  GUID           propertyUniqueID;
  ULONG          timeStampS1;
  ULONG          timeStampD1;
  ULONG          timeStampS2;
  ULONG          timeStampD2;
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
                                      ULONG blocksize,
                                      BOOL fileBased);
void           BIGBLOCKFILE_Destructor(LPBIGBLOCKFILE This);
HRESULT        BIGBLOCKFILE_EnsureExists(LPBIGBLOCKFILE This, ULONG index);
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
   * Reference count of this object
   */
  LONG ref;

  /*
   * Ancestor storage (top level)
   */
  StorageImpl* ancestorStorage;

  /*
   * Index of the property for the root of
   * this storage
   */
  ULONG rootPropertySetIndex;

  /*
   * virtual Destructor method.
   */
  void (*v_destructor)(StorageBaseImpl*);

  /*
   * flags that this storage was opened or created with
   */
  DWORD openFlags;

  /*
   * State bits appear to only be preserved while running. No in the stream
   */
  DWORD stateBits;
};

/****************************************************************************
 * StorageBaseImpl stream list handlers
 */

void StorageBaseImpl_AddStream(StorageBaseImpl * stg, StgStreamImpl * strm);
void StorageBaseImpl_RemoveStream(StorageBaseImpl * stg, StgStreamImpl * strm);

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

  /* FIXME: should this be in Storage32BaseImpl ? */
  WCHAR            filename[PROPERTY_NAME_BUFFER_LEN];

  /*
   * File header
   */
  WORD  bigBlockSizeBits;
  WORD  smallBlockSizeBits;
  ULONG bigBlockSize;
  ULONG smallBlockSize;
  ULONG bigBlockDepotCount;
  ULONG rootStartBlock;
  ULONG smallBlockDepotStart;
  ULONG extBigBlockDepotStart;
  ULONG extBigBlockDepotCount;
  ULONG bigBlockDepotStart[COUNT_BBDEPOTINHEADER];

  ULONG blockDepotCached[NUM_BLOCKS_PER_DEPOT_BLOCK];
  ULONG indexBlockDepotCached;
  ULONG prevFreeBlock;

  /*
   * Abstraction of the big block chains for the chains of the header.
   */
  BlockChainStream* rootBlockChain;
  BlockChainStream* smallBlockDepotChain;
  BlockChainStream* smallBlockRootChain;

  /*
   * Pointer to the big block file abstraction
   */
  BigBlockFile* bigBlockFile;
};

BOOL StorageImpl_ReadProperty(
            StorageImpl*    This,
            ULONG           index,
            StgProperty*    buffer);

BOOL StorageImpl_WriteProperty(
            StorageImpl*        This,
            ULONG               index,
            const StgProperty*  buffer);

BlockChainStream* Storage32Impl_SmallBlocksToBigBlocks(
                      StorageImpl* This,
                      SmallBlockChainStream** ppsbChain);

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
   * Index of the property that owns (points to) this stream.
   */
  ULONG              ownerProperty;

  /*
   * Helper variable that contains the size of the stream
   */
  ULARGE_INTEGER     streamSize;

  /*
   * This is the current position of the cursor in the stream
   */
  ULARGE_INTEGER     currentPosition;

  /*
   * The information in the stream is represented by a chain of small blocks
   * or a chain of large blocks. Depending on the case, one of the two
   * following variables points to that information.
   */
  BlockChainStream*      bigBlockChain;
  SmallBlockChainStream* smallBlockChain;
};

/*
 * Method definition for the StgStreamImpl class.
 */
StgStreamImpl* StgStreamImpl_Construct(
		StorageBaseImpl* parentStorage,
    DWORD            grfMode,
    ULONG            ownerProperty);


/******************************************************************************
 * Endian conversion macros
 */
#ifdef WORDS_BIGENDIAN

#define htole32(x) RtlUlongByteSwap(x)
#define htole16(x) RtlUshortByteSwap(x)
#define le32toh(x) RtlUlongByteSwap(x)
#define le16toh(x) RtlUshortByteSwap(x)

#else

#define htole32(x) (x)
#define htole16(x) (x)
#define le32toh(x) (x)
#define le16toh(x) (x)

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
void StorageUtl_CopyPropertyToSTATSTG(STATSTG* destination, const StgProperty* source,
 int statFlags);

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
  ULONG        ownerPropertyIndex;
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
		ULONG          propertyIndex);

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
  ULONG          ownerPropertyIndex;
};

/*
 * Methods of the SmallBlockChainStream class.
 */
SmallBlockChainStream* SmallBlockChainStream_Construct(
	       StorageImpl* parentStorage,
	       ULONG          propertyIndex);

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
