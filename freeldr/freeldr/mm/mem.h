/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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


#ifndef __MEM_H
#define __MEM_H


//
// Define this to 1 if you want the entire contents
// of the memory allocation bitmap displayed
// when a chunk is allocated or freed
//
#define DUMP_MEM_MAP_ON_VERIFY	0

#define MEM_BLOCK_SIZE	256

typedef struct
{
	BOOL	MemBlockAllocated;		// Is this block allocated or free
	ULONG	BlocksAllocated;		// Block length, in multiples of MEM_BLOCK_SIZE
} MEMBLOCK, *PMEMBLOCK;


extern	ULONG		RealFreeLoaderModuleEnd;

extern	PVOID		HeapBaseAddress;
extern	ULONG		HeapLengthInBytes;
extern	ULONG		HeapMemBlockCount;
extern	PMEMBLOCK	HeapMemBlockArray;

#endif // defined __MEM_H
