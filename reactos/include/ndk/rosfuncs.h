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
  
#endif
