/* $Id: rostypes.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
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
 * FILE:            include/reactos/rostypes.h
 * PURPOSE:         Defintions for Internal Reactos Types
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _ROSTYPES_H
#define _ROSTYPES_H

/* ReactOS Defined NTSTATUS Values */
#define STATUS_FS_QUERY_REQUIRED                    ((NTSTATUS)0xC1000001)
#define STATUS_HANDLE_NOT_WAITABLE                  ((NTSTATUS)0xC1000002)
#define STATUS_OBJECT_FILE_MISMATCH                 ((NTSTATUS)0xC1000003)
#define STATUS_INVALID_PARAMETER_MAX                ((NTSTATUS)0xC1000004)
#define STATUS_CONFLICTING_ADDRESS                  ((NTSTATUS)0xC1000005)
#define STATUS_NO_MEDIA_IN_DRIVE                    ((NTSTATUS)0xC1000006)

/* ReactOS NTSTATUS Macros (maybe go in helper.h?) */
#define NTSTAT_SEVERITY_SHIFT 30
#define NTSTAT_SEVERITY_MASK  0x00000003
#define NTSTAT_FACILITY_SHIFT 16
#define NTSTAT_FACILITY_MASK  0x00000FFF
#define NTSTAT_CUSTOMER_MASK  0x20000000
#define NT_SEVERITY(StatCode) (((StatCode) >> NTSTAT_SEVERITY_SHIFT) & NTSTAT_SEVERITY_MASK)
#define NT_FACILITY(StatCode) (((StatCode) >> NTSTAT_FACILITY_SHIFT) & NTSTAT_FACILITY_MASK)
#define NT_CUSTOMER(StatCode) ((StatCode) & NTSTAT_CUSTOMER_MASK)

typedef enum _TRAVERSE_METHOD {
	TraverseMethodPreorder,
	TraverseMethodInorder,
	TraverseMethodPostorder
} TRAVERSE_METHOD;

struct _BINARY_TREE_NODE;

typedef LONG STDCALL
(*PKEY_COMPARATOR)(IN PVOID  Key1,
		IN PVOID  Key2);

typedef BOOLEAN STDCALL
(*PTRAVERSE_ROUTINE)(IN PVOID  Context,
		IN PVOID  Key,
		IN PVOID  Value);

typedef struct _BINARY_TREE {
	struct _BINARY_TREE_NODE  * RootNode;
	PKEY_COMPARATOR  Compare;
	BOOLEAN  UseNonPagedPool;
	union {
		NPAGED_LOOKASIDE_LIST  NonPaged;
		PAGED_LOOKASIDE_LIST  Paged;
	} List;
	union {
		KSPIN_LOCK  NonPaged;
		FAST_MUTEX  Paged;
	} Lock;
} BINARY_TREE, *PBINARY_TREE;

struct _SPLAY_TREE_NODE;

typedef struct _SPLAY_TREE {
	struct _SPLAY_TREE_NODE  * RootNode;
	PKEY_COMPARATOR  Compare;
	BOOLEAN  Weighted;
	BOOLEAN  UseNonPagedPool;
	union {
		NPAGED_LOOKASIDE_LIST  NonPaged;
		PAGED_LOOKASIDE_LIST  Paged;
	} List;
	union {
		KSPIN_LOCK  NonPaged;
		FAST_MUTEX  Paged;
	} Lock;
	PVOID  Reserved[4];
} SPLAY_TREE, *PSPLAY_TREE;


typedef struct _HASH_TABLE {
	// Size of hash table in number of bits
	ULONG  HashTableSize;

	// Use non-paged pool memory?
	BOOLEAN  UseNonPagedPool;

	// Lock for this structure
	union {
		KSPIN_LOCK  NonPaged;
		FAST_MUTEX  Paged;
	} Lock;

	// Pointer to array of hash buckets with splay trees
	PSPLAY_TREE  HashTrees;
} HASH_TABLE, *PHASH_TABLE;

/* FIXME: This is a Windows Type which will be in the NDK. The type below however is our own implementation.
	We will eventually use Windows's */

typedef struct _RTL_ATOM_TABLE {
	ULONG TableSize;
	ULONG NumberOfAtoms;
	PVOID Lock;		/* fast mutex (kernel mode)/ critical section (user mode) */
	PVOID HandleTable;
	LIST_ENTRY Slot[0];
} RTL_ATOM_TABLE, *PRTL_ATOM_TABLE;

/* FIXME: This is a Windows Type which will be in the NDK. The type below however is our own implementation.
	We will eventually use Windows' */
typedef struct _OBJECT_TYPE {
	/*
	* PURPOSE: Tag to be used when allocating objects of this type
	*/
	ULONG Tag;

	/*
	* PURPOSE: Name of the type
	*/
	UNICODE_STRING TypeName;
  
	/*
	* PURPOSE: Total number of objects of this type
	*/
	ULONG TotalObjects;
  
	/*
	* PURPOSE: Total number of handles of this type
	*/
	ULONG TotalHandles;
  
	/*
	* PURPOSE: Maximum objects of this type
	*/
	ULONG MaxObjects;
  
	/*
	* PURPOSE: Maximum handles of this type
	*/
	ULONG MaxHandles;
  
	/*
	* PURPOSE: Paged pool charge
	*/
	ULONG PagedPoolCharge;
  
	/*
	* PURPOSE: Nonpaged pool charge
	*/
	ULONG NonpagedPoolCharge;
  
	/*
	* PURPOSE: Mapping of generic access rights
	*/
	PGENERIC_MAPPING Mapping;
  
	/*
	* PURPOSE: Dumps the object
	* NOTE: To be defined
	*/
	VOID STDCALL (*Dump)(VOID);
  
	/*
	* PURPOSE: Opens the object
	* NOTE: To be defined
	*/
	VOID STDCALL (*Open)(VOID);
  
	/*
	* PURPOSE: Called to close an object if OkayToClose returns true
	*/
	VOID STDCALL (*Close)(PVOID ObjectBody,
			ULONG HandleCount);
  
	/*
	* PURPOSE: Called to delete an object when the last reference is removed
	*/
	VOID STDCALL (*Delete)(PVOID ObjectBody);
  
	/*
	* PURPOSE: Called when an open attempts to open a file apparently
	* residing within the object
	* RETURNS
	*     STATUS_SUCCESS       NextObject was found
	*     STATUS_UNSUCCESSFUL  NextObject not found
	*     STATUS_REPARSE       Path changed, restart parsing the path
	*/
	NTSTATUS STDCALL (*Parse)(PVOID ParsedObject,
				PVOID *NextObject,
				PUNICODE_STRING FullPath,
				PWSTR *Path,
				ULONG Attributes);

	/*
	* PURPOSE: Called to set, query, delete or assign a security-descriptor
	* to the object
	* RETURNS
	*     STATUS_SUCCESS       NextObject was found
	*/
	NTSTATUS STDCALL (*Security)(PVOID ObjectBody,
				SECURITY_OPERATION_CODE OperationCode,
				SECURITY_INFORMATION SecurityInformation,
				PSECURITY_DESCRIPTOR SecurityDescriptor,
				PULONG BufferLength);

	/*
	* PURPOSE: Called to query the name of the object
	* RETURNS
	*     STATUS_SUCCESS       NextObject was found
	*/
	NTSTATUS STDCALL (*QueryName)(PVOID ObjectBody,
				POBJECT_NAME_INFORMATION ObjectNameInfo,
				ULONG Length,
				PULONG ReturnLength);

	/*
	* PURPOSE: Called when a process asks to close the object
	*/
	VOID STDCALL (*OkayToClose)(VOID);

	NTSTATUS STDCALL (*Create)(PVOID ObjectBody,
				PVOID Parent,
				PWSTR RemainingPath,
				struct _OBJECT_ATTRIBUTES* ObjectAttributes);

	VOID STDCALL (*DuplicationNotify)(PEPROCESS DuplicateTo,
				PEPROCESS DuplicateFrom,
				PVOID Object);
} OBJECT_TYPE;

#endif
