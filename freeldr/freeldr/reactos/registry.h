/*
 *  FreeLoader - registry.h
 *
 *  Copyright (C) 2001  Eric Kohl
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

#ifndef __REGISTRY_H
#define __REGISTRY_H

#define INVALID_HANDLE_VALUE  NULL

typedef struct _LIST_ENTRY
{
  struct _LIST_ENTRY *Flink;
  struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;


typedef struct _REG_KEY
{
  LIST_ENTRY KeyList;
  LIST_ENTRY SubKeyList;
  LIST_ENTRY ValueList;

  ULONG NameSize;
  PUCHAR Name;

  /* default data */
  ULONG DataType;
  ULONG DataSize;
  PUCHAR Data;
} KEY, *HKEY, **PHKEY;


typedef struct _REG_VALUE
{
  LIST_ENTRY ValueList;

  /* value name */
  ULONG NameSize;
  PUCHAR Name;

  /* value data */
  ULONG DataType;
  ULONG DataSize;
  PUCHAR Data;
} VALUE, *PVALUE;


#define ERROR_SUCCESS                    0L
#define ERROR_PATH_NOT_FOUND             2L
#define ERROR_OUTOFMEMORY                14L
#define ERROR_INVALID_PARAMETER          87L
#define ERROR_MORE_DATA                  234L
#define ERROR_NO_MORE_ITEMS              259L


#define assert(x)

/*
 * VOID
 * InitializeListHead (
 *		PLIST_ENTRY	ListHead
 *		);
 *
 * FUNCTION: Initializes a double linked list
 * ARGUMENTS:
 *         ListHead = Caller supplied storage for the head of the list
 */
#define InitializeListHead(ListHead) \
{ \
	(ListHead)->Flink = (ListHead); \
	(ListHead)->Blink = (ListHead); \
}


/*
 * VOID
 * InsertHeadList (
 *		PLIST_ENTRY	ListHead,
 *		PLIST_ENTRY	Entry
 *		);
 *
 * FUNCTION: Inserts an entry in a double linked list
 * ARGUMENTS:
 *        ListHead = Head of the list
 *        Entry = Entry to insert
 */
#define InsertHeadList(ListHead, ListEntry) \
{ \
	PLIST_ENTRY OldFlink; \
	OldFlink = (ListHead)->Flink; \
	(ListEntry)->Flink = OldFlink; \
	(ListEntry)->Blink = (ListHead); \
	OldFlink->Blink = (ListEntry); \
	(ListHead)->Flink = (ListEntry); \
	assert((ListEntry) != NULL); \
	assert((ListEntry)->Blink!=NULL); \
	assert((ListEntry)->Blink->Flink == (ListEntry)); \
	assert((ListEntry)->Flink != NULL); \
	assert((ListEntry)->Flink->Blink == (ListEntry)); \
}


/*
 * VOID
 * InsertTailList (
 *		PLIST_ENTRY	ListHead,
 *		PLIST_ENTRY	Entry
 *		);
 *
 * FUNCTION:
 *	Inserts an entry in a double linked list
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
 *	Entry = Entry to insert
 */
#define InsertTailList(ListHead, ListEntry) \
{ \
	PLIST_ENTRY OldBlink; \
	OldBlink = (ListHead)->Blink; \
	(ListEntry)->Flink = (ListHead); \
	(ListEntry)->Blink = OldBlink; \
	OldBlink->Flink = (ListEntry); \
	(ListHead)->Blink = (ListEntry); \
	assert((ListEntry) != NULL); \
	assert((ListEntry)->Blink != NULL); \
	assert((ListEntry)->Blink->Flink == (ListEntry)); \
	assert((ListEntry)->Flink != NULL); \
	assert((ListEntry)->Flink->Blink == (ListEntry)); \
}

/*
 * BOOLEAN
 * IsListEmpty (
 *	PLIST_ENTRY	ListHead
 *	);
 *
 * FUNCTION:
 *	Checks if a double linked list is empty
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
*/
#define IsListEmpty(ListHead) \
	((ListHead)->Flink == (ListHead))


/*
 *VOID
 *RemoveEntryList (
 *	PLIST_ENTRY	Entry
 *	);
 *
 * FUNCTION:
 *	Removes an entry from a double linked list
 *
 * ARGUMENTS:
 *	ListEntry = Entry to remove
 */
#define RemoveEntryList(ListEntry) \
{ \
	PLIST_ENTRY OldFlink; \
	PLIST_ENTRY OldBlink; \
	assert((ListEntry) != NULL); \
	assert((ListEntry)->Blink!=NULL); \
	assert((ListEntry)->Blink->Flink == (ListEntry)); \
	assert((ListEntry)->Flink != NULL); \
	assert((ListEntry)->Flink->Blink == (ListEntry)); \
	OldFlink = (ListEntry)->Flink; \
	OldBlink = (ListEntry)->Blink; \
	OldFlink->Blink = OldBlink; \
	OldBlink->Flink = OldFlink; \
	(ListEntry)->Flink = NULL; \
	(ListEntry)->Blink = NULL; \
}

/*
 * PURPOSE: Returns the byte offset of a field within a structure
 */
#define FIELD_OFFSET(Type,Field) (LONG)(&(((Type *)(0))->Field))

/*
 * PURPOSE: Returns the base address structure if the caller knows the 
 * address of a field within the structure
 * ARGUMENTS:
 *          Address = address of the field
 *          Type = Type of the whole structure
 *          Field = Name of the field whose address is none
 */
#define CONTAINING_RECORD(Address,Type,Field) \
	(Type *)(((LONG)Address) - FIELD_OFFSET(Type,Field))


#define REG_NONE 0
#define REG_SZ 1
#define REG_EXPAND_SZ 2
#define REG_BINARY 3
#define REG_DWORD 4
#define REG_DWORD_BIG_ENDIAN 5
#define REG_DWORD_LITTLE_ENDIAN 4
#define REG_LINK 6
#define REG_MULTI_SZ 7
#define REG_RESOURCE_LIST 8
#define REG_FULL_RESOURCE_DESCRIPTOR 9
#define REG_RESOURCE_REQUIREMENTS_LIST 10



VOID
RegInitializeRegistry(VOID);

LONG
RegInitCurrentControlSet(BOOL LastKnownGood);


LONG
RegCreateKey(HKEY ParentKey,
	     PCHAR KeyName,
	     PHKEY Key);

LONG
RegDeleteKey(HKEY Key,
	     PCHAR Name);

LONG
RegEnumKey(HKEY Key,
	   ULONG Index,
	   PCHAR Name,
	   PULONG NameSize);

LONG
RegOpenKey(HKEY ParentKey,
	   PCHAR KeyName,
	   PHKEY Key);


LONG
RegSetValue(HKEY Key,
	    PCHAR ValueName,
	    ULONG Type,
	    PUCHAR Data,
	    ULONG DataSize);

LONG
RegQueryValue(HKEY Key,
	      PCHAR ValueName,
	      PULONG Type,
	      PUCHAR Data,
	      PULONG DataSize);

LONG
RegDeleteValue(HKEY Key,
	       PCHAR ValueName);

LONG
RegEnumValue(HKEY Key,
	     ULONG Index,
	     PCHAR ValueName,
	     PULONG NameSize,
	     PULONG Type,
	     PUCHAR Data,
	     PULONG DataSize);


VOID
RegImportHive(PCHAR ChunkBase,
	      ULONG ChunkSize);


#endif /* __REGISTRY_H */

/* EOF */

