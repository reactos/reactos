/* $Id: rosfuncs.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS Internal Headers
 * FILE:            include/reactos/rosfuncs.h
 * PURPOSE:         Function Prototypes for Internal Reactos Functions
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _ROSFUNCS_H
#define _ROSFUNCS_H

#include "rostypes.h"
  
NTSTATUS
STDCALL
CcRosInitializeFileCache(
    PFILE_OBJECT FileObject,
    ULONG CacheSegmentSize
);

NTSTATUS
STDCALL
CcRosReleaseFileCache(
    PFILE_OBJECT FileObject
);

BOOLEAN
STDCALL
ExInitializeBinaryTree(
	IN PBINARY_TREE  Tree,
	IN PKEY_COMPARATOR  Compare,
	IN BOOLEAN  UseNonPagedPool
);

VOID
STDCALL
ExDeleteBinaryTree(
	IN PBINARY_TREE  Tree
);

VOID
STDCALL
ExInsertBinaryTree(
	IN PBINARY_TREE  Tree,
	IN PVOID  Key,
	IN PVOID  Value
);

BOOLEAN
STDCALL
ExSearchBinaryTree(
	IN PBINARY_TREE  Tree,
	IN PVOID  Key,
	OUT PVOID  * Value
);

BOOLEAN
STDCALL
ExRemoveBinaryTree(
	IN PBINARY_TREE  Tree,
	IN PVOID  Key,
	IN PVOID  * Value
);

BOOLEAN
STDCALL
ExTraverseBinaryTree(
	IN PBINARY_TREE  Tree,
	IN TRAVERSE_METHOD  Method,
	IN PTRAVERSE_ROUTINE  Routine,
	IN PVOID  Context
);

BOOLEAN
STDCALL
ExInitializeSplayTree(
	IN PSPLAY_TREE  Tree,
	IN PKEY_COMPARATOR  Compare,
	IN BOOLEAN  Weighted,
	IN BOOLEAN  UseNonPagedPool
);

VOID
STDCALL
ExDeleteSplayTree(
	IN PSPLAY_TREE  Tree
);

VOID
STDCALL
ExInsertSplayTree(
	IN PSPLAY_TREE  Tree,
	IN PVOID  Key,
	IN PVOID  Value
);

BOOLEAN
STDCALL
ExSearchSplayTree(
	IN PSPLAY_TREE  Tree,
	IN PVOID  Key,
	OUT PVOID  * Value
);

BOOLEAN
STDCALL
ExRemoveSplayTree(
	IN PSPLAY_TREE  Tree,
	IN PVOID  Key,
	IN PVOID  * Value
);

BOOLEAN
STDCALL
ExWeightOfSplayTree(
	IN PSPLAY_TREE  Tree,
	OUT PULONG  Weight
);

BOOLEAN
STDCALL
ExTraverseSplayTree(
	IN PSPLAY_TREE  Tree,
	IN TRAVERSE_METHOD  Method,
	IN PTRAVERSE_ROUTINE  Routine,
	IN PVOID  Context
);

BOOLEAN
STDCALL
ExInitializeHashTable(
	IN PHASH_TABLE  HashTable,
	IN ULONG  HashTableSize,
	IN PKEY_COMPARATOR  Compare  OPTIONAL,
	IN BOOLEAN  UseNonPagedPool
);

VOID
STDCALL
ExDeleteHashTable(
	IN PHASH_TABLE  HashTable
);

VOID
STDCALL
ExInsertHashTable(
	IN PHASH_TABLE  HashTable,
	IN PVOID  Key,
	IN ULONG  KeyLength,
	IN PVOID  Value
);

BOOLEAN
STDCALL
ExSearchHashTable(
	IN PHASH_TABLE  HashTable,
	IN PVOID  Key,
	IN ULONG  KeyLength,
	OUT PVOID  * Value
);

BOOLEAN
STDCALL
ExRemoveHashTable(
	IN PHASH_TABLE  HashTable,
	IN PVOID  Key,
	IN ULONG  KeyLength,
	IN PVOID  * Value
);
  
PVOID
STDCALL
MmAllocateContiguousAlignedMemory(
	IN ULONG NumberOfBytes,
	IN PHYSICAL_ADDRESS LowestAcceptableAddress,
	IN PHYSICAL_ADDRESS HighestAcceptableAddress,
	IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
	IN MEMORY_CACHING_TYPE CacheType OPTIONAL,
	IN ULONG Alignment
);

#define RosInitializeString( \
 __PDEST_STRING__, \
 __LENGTH__, \
 __MAXLENGTH__, \
 __BUFFER__ \
) \
{ \
 (__PDEST_STRING__)->Length = (__LENGTH__); \
 (__PDEST_STRING__)->MaximumLength = (__MAXLENGTH__); \
 (__PDEST_STRING__)->Buffer = (__BUFFER__); \
}

#define RtlRosInitStringFromLiteral( \
 __PDEST_STRING__, __SOURCE_STRING__) \
 RosInitializeString( \
  (__PDEST_STRING__), \
  sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
  sizeof(__SOURCE_STRING__), \
  (__SOURCE_STRING__) \
 )
 
#define RtlRosInitUnicodeStringFromLiteral \
 RtlRosInitStringFromLiteral

#define ROS_STRING_INITIALIZER(__SOURCE_STRING__) \
{ \
 sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
 sizeof(__SOURCE_STRING__), \
 (__SOURCE_STRING__) \
}

#define ROS_EMPTY_STRING {0, 0, NULL}

NTSTATUS 
STDCALL
RosAppendUnicodeString(
    PUNICODE_STRING ResultFirst,
    PUNICODE_STRING Second,
    BOOL Deallocate
);
  
#endif
