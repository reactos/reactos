/* $Id: atom.c,v 1.4 2002/09/08 10:23:04 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/atom.c
 * PURPOSE:         Atom managment
 * PROGRAMMER:      Nobody
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntos/heap.h>

#define NDEBUG
#include <ntdll/ntdll.h>


/* LOCAL TYPES ***************************************************************/

typedef struct _RTL_ATOM_ENTRY
{
   LIST_ENTRY List;
   UNICODE_STRING Name;
   ULONG RefCount;
   BOOLEAN Locked;
   ULONG Index;
   PRTL_HANDLE Handle;
} RTL_ATOM_ENTRY, *PRTL_ATOM_ENTRY;

typedef struct _RTL_ATOM_HANDLE
{
   RTL_HANDLE Handle;
   PRTL_ATOM_ENTRY Entry;
} RTL_ATOM_HANDLE, *PRTL_ATOM_HANDLE;


/* PROTOTYPES ****************************************************************/

static ULONG RtlpHashAtomName(ULONG TableSize, PWSTR AtomName);
static BOOLEAN RtlpCheckIntegerAtom(PWSTR AtomName, PUSHORT AtomValue);

static NTSTATUS RtlpInitAtomTableLock(PRTL_ATOM_TABLE AtomTable);
static VOID RtlpDestroyAtomTableLock(PRTL_ATOM_TABLE AtomTable);
static BOOLEAN RtlpLockAtomTable(PRTL_ATOM_TABLE AtomTable);
static VOID RtlpUnlockAtomTable(PRTL_ATOM_TABLE AtomTable);

static BOOLEAN RtlpCreateAtomHandleTable(PRTL_ATOM_TABLE AtomTable);
static VOID RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable);


/* FUNCTIONS *****************************************************************/


NTSTATUS STDCALL
RtlCreateAtomTable(ULONG TableSize,
		   PRTL_ATOM_TABLE *AtomTable)
{
   PRTL_ATOM_TABLE Table;
   ULONG i;
   NTSTATUS Status;

   DPRINT("RtlCreateAtomTable(TableSize %lu AtomTable %p)\n",
	  TableSize, AtomTable);

   if (*AtomTable != NULL)
     {
	return STATUS_SUCCESS;
     }

   /* allocate atom table */
   Table = RtlAllocateHeap(RtlGetProcessHeap(),
			   HEAP_ZERO_MEMORY,
			   TableSize * sizeof(LIST_ENTRY) +
			   sizeof(RTL_ATOM_TABLE));
   if (Table == NULL)
     {
	return STATUS_NO_MEMORY;
     }

   /* initialize atom table */
   Table->TableSize = TableSize;

   for (i = 0; i < TableSize; i++)
     {
	InitializeListHead(&Table->Slot[i]);
     }

   Status = RtlpInitAtomTableLock(Table);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    Table);
	return Status;
     }

   if (RtlpCreateAtomHandleTable(Table) == FALSE)
     {
	RtlpDestroyAtomTableLock(Table);
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    Table);
	return STATUS_NO_MEMORY;
     }

   *AtomTable = Table;
   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlDestroyAtomTable(IN PRTL_ATOM_TABLE AtomTable)
{
   PLIST_ENTRY Current;
   PRTL_ATOM_ENTRY AtomEntry;
   ULONG i;

   if (RtlpLockAtomTable(AtomTable) == FALSE)
     {
	return (STATUS_INVALID_PARAMETER);
     }

   /* delete all atoms */
   for (i = 0; i < AtomTable->TableSize; i++)
     {

	Current = AtomTable->Slot[i].Flink;
	while (Current != &AtomTable->Slot[i])
	  {
	     AtomEntry = (PRTL_ATOM_ENTRY)Current;
	     RtlFreeUnicodeString(&AtomEntry->Name);
	     RemoveEntryList(&AtomEntry->List);
	     RtlFreeHeap(RtlGetProcessHeap(),
			 0,
			 AtomEntry);
	     Current = AtomTable->Slot[i].Flink;
	  }

     }

   RtlpDestroyAtomHandleTable(AtomTable);

   RtlpUnlockAtomTable(AtomTable);

   RtlpDestroyAtomTableLock(AtomTable);

   RtlFreeHeap(RtlGetProcessHeap(),
	       0,
	       AtomTable);

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlEmptyAtomTable(PRTL_ATOM_TABLE AtomTable,
		  BOOLEAN DeletePinned)
{
   PLIST_ENTRY Current, Next;
   PRTL_ATOM_ENTRY AtomEntry;
   ULONG i;

   DPRINT("RtlEmptyAtomTable (AtomTable %p DeletePinned %x)\n",
	  AtomTable, DeletePinned);

   if (RtlpLockAtomTable(AtomTable) == FALSE)
     {
	return (STATUS_INVALID_PARAMETER);
     }

   /* delete all atoms */
   for (i = 0; i < AtomTable->TableSize; i++)
     {
	Current = AtomTable->Slot[i].Flink;
	while (Current != &AtomTable->Slot[i])
	  {
	     Next = Current->Flink;
	     AtomEntry = (PRTL_ATOM_ENTRY)Current;

	     if ((AtomEntry->Locked == FALSE) ||
		 ((AtomEntry->Locked == TRUE) && (DeletePinned == TRUE)))
	       {
		  RtlFreeUnicodeString(&AtomEntry->Name);

		  RtlFreeHandle(AtomTable->HandleTable,
				AtomEntry->Handle);

		  RemoveEntryList(&AtomEntry->List);
		  RtlFreeHeap(RtlGetProcessHeap(),
			      0,
			      AtomEntry);
	       }
	     Current = Next;
	  }

     }

   RtlpUnlockAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlAddAtomToAtomTable(IN PRTL_ATOM_TABLE AtomTable,
		      IN PWSTR AtomName,
		      OUT PRTL_ATOM Atom)
{
   ULONG Hash;
   PLIST_ENTRY Current;
   PRTL_ATOM_ENTRY Entry;
   USHORT AtomValue;
   NTSTATUS Status;
   PRTL_ATOM_HANDLE AtomHandle;
   ULONG AtomIndex;

   DPRINT("RtlAddAtomToAtomTable (AtomTable %p AtomName %S Atom %p)\n",
	  AtomTable, AtomName, Atom);

   if (RtlpCheckIntegerAtom (AtomName, &AtomValue))
     {
	/* integer atom */
	if (AtomValue >= 0xC000)
	  {
	     AtomValue = 0;
	     Status = STATUS_INVALID_PARAMETER;
	  }
	else
	  {
	     Status = STATUS_SUCCESS;
	  }

	if (Atom)
	  *Atom = (RTL_ATOM)AtomValue;

	return Status;
     }

   RtlpLockAtomTable(AtomTable);

   /* string atom */
   Hash = RtlpHashAtomName(AtomTable->TableSize, AtomName);

   /* search for existing atom */
   Current = AtomTable->Slot[Hash].Flink;
   while (Current != &AtomTable->Slot[Hash])
     {
	Entry = (PRTL_ATOM_ENTRY)Current;

	DPRINT("Comparing %S and %S\n", Entry->Name.Buffer, AtomName);
	if (_wcsicmp(Entry->Name.Buffer, AtomName) == 0)
	  {
	     Entry->RefCount++;
	     if (Atom)
	       *Atom = (RTL_ATOM)(Entry->Index + 0xC000);
	     RtlpUnlockAtomTable(AtomTable);
	     return STATUS_SUCCESS;
	  }
	Current = Current->Flink;
     }

   /* insert new atom */
   Entry = RtlAllocateHeap(RtlGetProcessHeap(),
			   HEAP_ZERO_MEMORY,
			   sizeof(RTL_ATOM_ENTRY));
   if (Entry == NULL)
     {
	RtlpUnlockAtomTable(AtomTable);
	return STATUS_NO_MEMORY;
     }

   InsertTailList(&AtomTable->Slot[Hash], &Entry->List)
   RtlCreateUnicodeString (&Entry->Name,
			   AtomName);
   Entry->RefCount = 1;
   Entry->Locked = FALSE;

   /* FIXME: use general function instead !! */
   AtomHandle = (PRTL_ATOM_HANDLE)RtlAllocateHandle(AtomTable->HandleTable,
						    &AtomIndex);

   DPRINT("AtomHandle %p AtomIndex %x\n", AtomHandle, AtomIndex);

   AtomHandle->Entry = Entry;
   Entry->Index = AtomIndex;
   Entry->Handle = (PRTL_HANDLE)AtomHandle;

   if (Atom)
     *Atom = (RTL_ATOM)(AtomIndex + 0xC000);

   RtlpUnlockAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlDeleteAtomFromAtomTable(IN PRTL_ATOM_TABLE AtomTable,
			   IN RTL_ATOM Atom)
{
   PRTL_ATOM_HANDLE AtomHandle;
   PRTL_ATOM_ENTRY AtomEntry;

   DPRINT("RtlDeleteAtomFromAtomTable (AtomTable %p Atom %x)\n",
	  AtomTable, Atom);

   if (Atom < 0xC000)
     {
	return STATUS_SUCCESS;
     }

   RtlpLockAtomTable(AtomTable);

   /* FIXME: use general function instead !! */
   if (!RtlIsValidIndexHandle(AtomTable->HandleTable,
			      (PRTL_HANDLE *)&AtomHandle,
			      (ULONG)Atom - 0xC000))
     {
	RtlpUnlockAtomTable(AtomTable);
	return STATUS_INVALID_HANDLE;
     }

   DPRINT("AtomHandle %x\n", AtomHandle);
   DPRINT("AtomHandle->Entry %x\n", AtomHandle->Entry);

   AtomEntry = AtomHandle->Entry;

   DPRINT("Atom name: %wZ\n", &AtomEntry->Name);

   AtomEntry->RefCount--;

   if (AtomEntry->RefCount == 0)
     {
	if (AtomEntry->Locked == TRUE)
	  {
	     DPRINT("Atom %wZ is locked!\n", &AtomEntry->Name);

	     RtlpUnlockAtomTable(AtomTable);
	     return STATUS_WAS_LOCKED;
	  }

	DPRINT("Removing atom: %wZ\n", &AtomEntry->Name);

	RtlFreeUnicodeString(&AtomEntry->Name);
	RemoveEntryList(&AtomEntry->List);
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    AtomEntry);
	RtlFreeHandle(AtomTable->HandleTable,
		      (PRTL_HANDLE)AtomHandle);
     }

   RtlpUnlockAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlLookupAtomInAtomTable(IN PRTL_ATOM_TABLE AtomTable,
			 IN PWSTR AtomName,
			 OUT PRTL_ATOM Atom)
{
   ULONG Hash;
   PLIST_ENTRY Current;
   PRTL_ATOM_ENTRY Entry;
   USHORT AtomValue;
   NTSTATUS Status;

   DPRINT("RtlLookupAtomInAtomTable (AtomTable %p AtomName %S Atom %p)\n",
	  AtomTable, AtomName, Atom);

   if (RtlpCheckIntegerAtom (AtomName, &AtomValue))
     {
	/* integer atom */
	if (AtomValue >= 0xC000)
	  {
	     AtomValue = 0;
	     Status = STATUS_INVALID_PARAMETER;
	  }
	else
	  {
	     Status = STATUS_SUCCESS;
	  }

	if (Atom)
	  *Atom = (RTL_ATOM)AtomValue;

	return Status;
     }

   RtlpLockAtomTable(AtomTable);

   /* string atom */
   Hash = RtlpHashAtomName(AtomTable->TableSize, AtomName);

   /* search for existing atom */
   Current = AtomTable->Slot[Hash].Flink;
   while (Current != &AtomTable->Slot[Hash])
     {
	Entry = (PRTL_ATOM_ENTRY)Current;

	DPRINT("Comparing %S and %S\n", Entry->Name.Buffer, AtomName);
	if (_wcsicmp(Entry->Name.Buffer, AtomName) == 0)
	  {
	     if (Atom)
	       *Atom = (RTL_ATOM)(Entry->Index + 0xC000);
	     RtlpUnlockAtomTable(AtomTable);
	     return STATUS_SUCCESS;
	  }

	Current = Current->Flink;
     }

   return STATUS_OBJECT_NAME_NOT_FOUND;
}


NTSTATUS STDCALL
RtlPinAtomInAtomTable(IN PRTL_ATOM_TABLE AtomTable,
		      IN RTL_ATOM Atom)
{
   PRTL_ATOM_HANDLE AtomHandle;
   PRTL_ATOM_ENTRY AtomEntry;

   DPRINT("RtlPinAtomInAtomTable (AtomTable %p Atom %x)\n",
	  AtomTable, Atom);

   if (Atom < 0xC000)
     {
	return STATUS_SUCCESS;
     }

   RtlpLockAtomTable(AtomTable);

   /* FIXME: use general function instead !! */
   if (!RtlIsValidIndexHandle(AtomTable->HandleTable,
			      (PRTL_HANDLE *)&AtomHandle,
			      (ULONG)Atom - 0xC000))
     {
	RtlpUnlockAtomTable(AtomTable);
	return STATUS_INVALID_HANDLE;
     }

   DPRINT("AtomHandle %x\n", AtomHandle);
   DPRINT("AtomHandle->Entry %x\n", AtomHandle->Entry);

   AtomEntry = AtomHandle->Entry;

   DPRINT("Atom name: %wZ\n", &AtomEntry->Name);

   AtomEntry->Locked = TRUE;

   RtlpUnlockAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlQueryAtomInAtomTable(PRTL_ATOM_TABLE AtomTable,
			RTL_ATOM Atom,
			PULONG RefCount,
			PULONG PinCount,
			PWSTR AtomName,
			PULONG NameLength)
{
   ULONG Length;
   PRTL_ATOM_HANDLE AtomHandle;
   PRTL_ATOM_ENTRY AtomEntry;

   if (Atom < 0xC000)
     {
	if (RefCount != NULL)
	  {
	     *RefCount = 1;
	  }

	if (PinCount != NULL)
	  {
	     *PinCount = 1;
	  }

	if ((AtomName != NULL) && (NameLength != NULL) && (NameLength > 0))
	  {
	     Length = swprintf(AtomName, L"#%lu", (ULONG)Atom);
	     *NameLength = Length * sizeof(WCHAR);
	  }

	return STATUS_SUCCESS;
     }

   RtlpLockAtomTable(AtomTable);

   /* FIXME: use general function instead !! */
   if (!RtlIsValidIndexHandle(AtomTable->HandleTable,
			      (PRTL_HANDLE *)&AtomHandle,
			      (ULONG)Atom - 0xC000))
     {
	RtlpUnlockAtomTable(AtomTable);
	return STATUS_INVALID_HANDLE;
     }

   DPRINT("AtomHandle %x\n", AtomHandle);
   DPRINT("AtomHandle->Entry %x\n", AtomHandle->Entry);

   AtomEntry = AtomHandle->Entry;

   DPRINT("Atom name: %wZ\n", &AtomEntry->Name);

   if (RefCount != NULL)
     {
	*RefCount = AtomEntry->RefCount;
     }

   if (PinCount != NULL)
     {
	*PinCount = (ULONG)AtomEntry->Locked;
     }

   if ((AtomName != NULL) && (NameLength != NULL))
     {
	if (*NameLength < AtomEntry->Name.Length)
	  {
	     *NameLength = AtomEntry->Name.Length;
	     RtlpUnlockAtomTable(AtomTable);
	     return STATUS_BUFFER_TOO_SMALL;
	  }

	Length = swprintf(AtomName, L"%s", AtomEntry->Name.Buffer);
	*NameLength = Length * sizeof(WCHAR);
     }

   RtlpUnlockAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


/* INTERNAL FUNCTIONS ********************************************************/

static ULONG
RtlpHashAtomName(ULONG TableSize,
		 PWSTR AtomName)
{
   ULONG q = 0;
   PWCHAR p;

   DPRINT("RtlpHashAtomName(TableSize %ld AtomName '%S')\n",
	  TableSize, AtomName);

   /* convert the string to an internal representation */
   p = AtomName;
   while (*p != 0)
     {
	q += (ULONG)towupper(*p);
	p++;
     }

   DPRINT("q %lu Hash %lu\n", q, q % TableSize);

   return (q % TableSize);
}


static BOOLEAN
RtlpCheckIntegerAtom(PWSTR AtomName,
		     PUSHORT AtomValue)
{
   UNICODE_STRING AtomString;
   USHORT LoValue;
   ULONG LongValue;
   PWCHAR p;

   DPRINT("RtlpCheckIntegerAtom(AtomName '%S' AtomValue %p)\n",
	  AtomName, AtomValue);

   if (!((ULONG)AtomName & 0xFFFF0000))
     {
	LoValue = (USHORT)((ULONG)AtomName & 0xFFFF);

	if (LoValue >= 0xC000)
	  return FALSE;

	if (LoValue == 0)
	  LoValue = 0xC000;

	if (AtomValue != NULL)
	  *AtomValue = LoValue;

	return TRUE;
     }

   if (*AtomName != L'#')
     return FALSE;

   p = AtomName;
   p++;
   while (*p)
     {
	if ((*p < L'0') || (*p > L'9'))
	  return FALSE;
	p++;
     }

   p = AtomName;
   p++;
   RtlInitUnicodeString(&AtomString,
			p);

   RtlUnicodeStringToInteger(&AtomString,10, &LongValue);

   *AtomValue = (USHORT)(LongValue & 0x0000FFFF);

   return TRUE;
}


/* lock functions */

static NTSTATUS
RtlpInitAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   AtomTable->Lock = RtlAllocateHeap(RtlGetProcessHeap(),
				     HEAP_ZERO_MEMORY,
				     sizeof(CRITICAL_SECTION));
   if (AtomTable->Lock == NULL)
     return STATUS_NO_MEMORY;

   RtlInitializeCriticalSection((PCRITICAL_SECTION)AtomTable->Lock);

   return STATUS_SUCCESS;
}


static VOID
RtlpDestroyAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   if (AtomTable->Lock)
     {
	RtlDeleteCriticalSection((PCRITICAL_SECTION)AtomTable->Lock);
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    AtomTable->Lock);
	AtomTable->Lock = NULL;
     }
}


static BOOLEAN
RtlpLockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlEnterCriticalSection((PCRITICAL_SECTION)AtomTable->Lock);
   return TRUE;
}


static VOID
RtlpUnlockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlLeaveCriticalSection((PCRITICAL_SECTION)AtomTable->Lock);
}


/* handle functions */

static BOOLEAN
RtlpCreateAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   AtomTable->HandleTable = RtlAllocateHeap(RtlGetProcessHeap(),
					    HEAP_ZERO_MEMORY,
					    sizeof(RTL_HANDLE_TABLE));
   if (AtomTable->HandleTable == NULL)
     return FALSE;

   RtlInitializeHandleTable(0xCFFF,
			    sizeof(RTL_ATOM_HANDLE),
			    (PRTL_HANDLE_TABLE)AtomTable->HandleTable);

   return TRUE;
}

static VOID
RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   if (AtomTable->HandleTable)
     {
	RtlDestroyHandleTable((PRTL_HANDLE_TABLE)AtomTable->HandleTable);
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    AtomTable->HandleTable);
	AtomTable->HandleTable = NULL;
     }
}

/* EOF */
