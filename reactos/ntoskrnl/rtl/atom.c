/* $Id: atom.c,v 1.5 2003/07/11 01:23:15 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/atom.c
 * PURPOSE:         Atom managment
 * PROGRAMMER:      Nobody
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/handle.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>



typedef struct _RTL_ATOM_ENTRY
{
   LIST_ENTRY List;
   UNICODE_STRING Name;
   ULONG RefCount;
   BOOLEAN Locked;
   ULONG Index;
} RTL_ATOM_ENTRY, *PRTL_ATOM_ENTRY;


/* PROTOTYPES ****************************************************************/

static PRTL_ATOM_TABLE RtlpGetGlobalAtomTable(VOID);

static ULONG RtlpHashAtomName(ULONG TableSize, PWSTR AtomName);
static BOOLEAN RtlpCheckIntegerAtom(PWSTR AtomName, PUSHORT AtomValue);

static NTSTATUS RtlpInitAtomTableLock(PRTL_ATOM_TABLE AtomTable);
static VOID RtlpDestroyAtomTableLock(PRTL_ATOM_TABLE AtomTable);
static BOOLEAN RtlpLockAtomTable(PRTL_ATOM_TABLE AtomTable);
static VOID RtlpUnlockAtomTable(PRTL_ATOM_TABLE AtomTable);

static BOOLEAN RtlpCreateAtomHandleTable(PRTL_ATOM_TABLE AtomTable);
static VOID RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable);

static NTSTATUS RtlpQueryAtomInformation(PRTL_ATOM_TABLE AtomTable,
					 RTL_ATOM Atom,
					 PATOM_BASIC_INFORMATION AtomInformation,
					 ULONG AtomInformationLength,
					 PULONG ReturnLength);

static NTSTATUS RtlpQueryAtomTableInformation(PRTL_ATOM_TABLE AtomTable,
					      RTL_ATOM Atom,
					      PATOM_TABLE_INFORMATION AtomInformation,
					      ULONG AtomInformationLength,
					      PULONG ReturnLength);


/* GLOBALS *******************************************************************/

static PRTL_ATOM_TABLE GlobalAtomTable = NULL;

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
NTSTATUS STDCALL
NtAddAtom(IN PWSTR AtomName,
	  OUT PRTL_ATOM Atom)
{
   PRTL_ATOM_TABLE AtomTable;

   AtomTable = RtlpGetGlobalAtomTable();
   if (AtomTable == NULL)
     return STATUS_ACCESS_DENIED;

   return (RtlAddAtomToAtomTable(AtomTable,
				 AtomName,
				 Atom));
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtDeleteAtom(IN RTL_ATOM Atom)
{
   PRTL_ATOM_TABLE AtomTable;

   AtomTable = RtlpGetGlobalAtomTable();
   if (AtomTable == NULL)
     return STATUS_ACCESS_DENIED;

   return (RtlDeleteAtomFromAtomTable(AtomTable,
				      Atom));
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtFindAtom(IN PWSTR AtomName,
	   OUT PRTL_ATOM Atom)
{
   PRTL_ATOM_TABLE AtomTable;

   AtomTable = RtlpGetGlobalAtomTable();
   if (AtomTable == NULL)
     return STATUS_ACCESS_DENIED;

   return (RtlLookupAtomInAtomTable(AtomTable,
				    AtomName,
				    Atom));
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtQueryInformationAtom(RTL_ATOM Atom,
		       ATOM_INFORMATION_CLASS AtomInformationClass,
		       PVOID AtomInformation,
		       ULONG AtomInformationLength,
		       PULONG ReturnLength)
{
   PRTL_ATOM_TABLE AtomTable;
   NTSTATUS Status;

   AtomTable = RtlpGetGlobalAtomTable();
   if (AtomTable == NULL)
     return STATUS_ACCESS_DENIED;

   switch (AtomInformationClass)
     {
     case AtomBasicInformation:
	Status = RtlpQueryAtomInformation(AtomTable,
					  Atom,
					  AtomInformation,
					  AtomInformationLength,
					  ReturnLength);
	break;

     case AtomTableInformation:
	Status = RtlpQueryAtomTableInformation(AtomTable,
					       Atom,
					       AtomInformation,
					       AtomInformationLength,
					       ReturnLength);
	break;

     default:
	Status = STATUS_INVALID_INFO_CLASS;
     }

   return Status;
}


/*
 * @implemented
 */
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
   Table = ExAllocatePool(NonPagedPool,
			  TableSize * sizeof(RTL_ATOM_ENTRY) +
			  sizeof(RTL_ATOM_TABLE));
   if (Table == NULL)
     return STATUS_NO_MEMORY;

   /* initialize atom table */
   Table->TableSize = TableSize;
   Table->NumberOfAtoms = 0;

   for (i = 0; i < TableSize; i++)
     {
	InitializeListHead(&Table->Slot[i]);
     }

   Status = RtlpInitAtomTableLock(Table);
   if (!NT_SUCCESS(Status))
     {
	ExFreePool(Table);
	return Status;
     }

   if (RtlpCreateAtomHandleTable(Table) == FALSE)
     {
	RtlpDestroyAtomTableLock(Table);
	ExFreePool(Table);
	return STATUS_NO_MEMORY;
     }

   *AtomTable = Table;
   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
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
	     ExFreePool(AtomEntry);
	     Current = AtomTable->Slot[i].Flink;
	  }

     }

   RtlpDestroyAtomHandleTable(AtomTable);

   RtlpUnlockAtomTable(AtomTable);

   RtlpDestroyAtomTableLock(AtomTable);

   ExFreePool(AtomTable);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlEmptyAtomTable(IN PRTL_ATOM_TABLE AtomTable,
		  IN BOOLEAN DeletePinned)
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

		  RtlpFreeHandle(AtomTable->HandleTable,
				 AtomEntry->Index);

		  RemoveEntryList(&AtomEntry->List);
		  ExFreePool(AtomEntry);
	       }
	     Current = Next;
	  }

     }

   AtomTable->NumberOfAtoms = 0;

   RtlpUnlockAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
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
   Entry = ExAllocatePool(NonPagedPool,
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
   RtlpAllocateHandle(AtomTable->HandleTable,
		      (PVOID)Entry,
		      &AtomIndex);

   DPRINT("AtomIndex %x\n", AtomIndex);

   Entry->Index = AtomIndex;
   AtomTable->NumberOfAtoms++;

   if (Atom)
     *Atom = (RTL_ATOM)(AtomIndex + 0xC000);

   RtlpUnlockAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlDeleteAtomFromAtomTable(IN PRTL_ATOM_TABLE AtomTable,
			   IN RTL_ATOM Atom)
{
   PRTL_ATOM_ENTRY AtomEntry;

   DPRINT("RtlDeleteAtomFromAtomTable (AtomTable %p Atom %x)\n",
	  AtomTable, Atom);

   if (Atom < 0xC000)
     {
	return STATUS_SUCCESS;
     }

   RtlpLockAtomTable(AtomTable);

   /* FIXME: use general function instead !! */
   AtomEntry = (PRTL_ATOM_ENTRY)RtlpMapHandleToPointer(AtomTable->HandleTable,
						       (ULONG)Atom - 0xC000);
   if (AtomEntry == NULL)
     {
	RtlpUnlockAtomTable(AtomTable);
	return STATUS_INVALID_HANDLE;
     }

   DPRINT("AtomEntry %x\n", AtomEntry);
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
	ExFreePool(AtomEntry);
	RtlpFreeHandle(AtomTable->HandleTable,
		       (ULONG)Atom - 0xC000);
	AtomTable->NumberOfAtoms++;
     }

   RtlpUnlockAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
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


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlPinAtomInAtomTable(IN PRTL_ATOM_TABLE AtomTable,
		      IN RTL_ATOM Atom)
{
   PRTL_ATOM_ENTRY AtomEntry;

   DPRINT("RtlPinAtomInAtomTable (AtomTable %p Atom %x)\n",
	  AtomTable, Atom);

   if (Atom < 0xC000)
     {
	return STATUS_SUCCESS;
     }

   RtlpLockAtomTable(AtomTable);

   /* FIXME: use general function instead !! */
   AtomEntry = (PRTL_ATOM_ENTRY)RtlpMapHandleToPointer(AtomTable->HandleTable,
						       (ULONG)Atom - 0xC000);
   if (AtomEntry == NULL)
     {
	RtlpUnlockAtomTable(AtomTable);
	return STATUS_INVALID_HANDLE;
     }

   DPRINT("AtomEntry %x\n", AtomEntry);
   DPRINT("Atom name: %wZ\n", &AtomEntry->Name);

   AtomEntry->Locked = TRUE;

   RtlpUnlockAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlQueryAtomInAtomTable(IN PRTL_ATOM_TABLE AtomTable,
			IN RTL_ATOM Atom,
			IN OUT PULONG RefCount,
			IN OUT PULONG PinCount,
			IN OUT PWSTR AtomName,
			IN OUT PULONG NameLength)
{
   ULONG Length;
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
   AtomEntry = (PRTL_ATOM_ENTRY)RtlpMapHandleToPointer(AtomTable->HandleTable,
						       (ULONG)Atom - 0xC000);
   if (AtomEntry == NULL)
     {
	RtlpUnlockAtomTable(AtomTable);
	return STATUS_INVALID_HANDLE;
     }

   DPRINT("AtomEntry %x\n", AtomEntry);
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

static PRTL_ATOM_TABLE
RtlpGetGlobalAtomTable(VOID)
{
   NTSTATUS Status;

   if (GlobalAtomTable != NULL)
     return GlobalAtomTable;

   Status = RtlCreateAtomTable(37, &GlobalAtomTable);
   if (!NT_SUCCESS(Status))
     return NULL;

   return GlobalAtomTable;
}


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
   ULONG LongValue;
   USHORT LoValue;
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

   DPRINT1("AtomString: %wZ\n", &AtomString);

   RtlUnicodeStringToInteger(&AtomString,10, &LongValue);

   DPRINT1("LongValue: %lu\n", LongValue);

   *AtomValue = (USHORT)(LongValue & 0x0000FFFF);

   return TRUE;
}


/* lock functions */

static NTSTATUS
RtlpInitAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   AtomTable->Lock = ExAllocatePool(NonPagedPool,
				    sizeof(FAST_MUTEX));
   if (AtomTable->Lock == NULL)
     return STATUS_NO_MEMORY;

   ExInitializeFastMutex((PFAST_MUTEX)AtomTable->Lock);

   return STATUS_SUCCESS;
}


static VOID
RtlpDestroyAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   if (AtomTable->Lock)
     ExFreePool(AtomTable->Lock);
}


static BOOLEAN
RtlpLockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
//   ExAcquireFastMutex((PFAST_MUTEX)AtomTable->Lock);
   return TRUE;
}

static VOID
RtlpUnlockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
//   ExReleaseFastMutex((PFAST_MUTEX)AtomTable->Lock);
}

#if 0
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
#endif

/* handle functions */

static BOOLEAN
RtlpCreateAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   AtomTable->HandleTable = ExAllocatePool(NonPagedPool,
					   sizeof(RTL_HANDLE_TABLE));
   if (AtomTable->HandleTable == NULL)
     return FALSE;

   RtlpInitializeHandleTable(0xCFFF,
			     (PRTL_HANDLE_TABLE)AtomTable->HandleTable);

   return TRUE;
}


static VOID
RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   if (AtomTable->HandleTable)
     {
	RtlpDestroyHandleTable((PRTL_HANDLE_TABLE)AtomTable->HandleTable);
	ExFreePool(AtomTable->HandleTable);
	AtomTable->HandleTable = NULL;
     }
}


static NTSTATUS
RtlpQueryAtomInformation(PRTL_ATOM_TABLE AtomTable,
			 RTL_ATOM Atom,
			 PATOM_BASIC_INFORMATION AtomInformation,
			 ULONG AtomInformationLength,
			 PULONG ReturnLength)
{
   NTSTATUS Status;
   ULONG UsageCount;
   ULONG Flags;
   ULONG NameLength;

   NameLength = AtomInformationLength - sizeof(ATOM_BASIC_INFORMATION) + sizeof(WCHAR);
   Status = RtlQueryAtomInAtomTable(AtomTable,
				    Atom,
				    &UsageCount,
				    &Flags,
				    AtomInformation->Name,
				    &NameLength);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

   DPRINT1("NameLength: %lu\n", NameLength);

   if (ReturnLength != NULL)
     {
	*ReturnLength = NameLength + sizeof(ATOM_BASIC_INFORMATION);
     }

   if (NameLength + sizeof(ATOM_BASIC_INFORMATION) > AtomInformationLength)
     {
	return STATUS_BUFFER_TOO_SMALL;
     }

   AtomInformation->UsageCount = (USHORT)UsageCount;
   AtomInformation->Flags = (USHORT)Flags;
   AtomInformation->NameLength = (USHORT)NameLength;

   return STATUS_SUCCESS;
}


static NTSTATUS
RtlpQueryAtomTableInformation(PRTL_ATOM_TABLE AtomTable,
			      RTL_ATOM Atom,
			      PATOM_TABLE_INFORMATION AtomInformation,
			      ULONG AtomInformationLength,
			      PULONG ReturnLength)
{
   PLIST_ENTRY Current, Next;
   PRTL_ATOM_ENTRY AtomEntry;
   ULONG Length;
   ULONG i, j;

   Length = sizeof(ATOM_TABLE_INFORMATION);
   if (AtomTable->NumberOfAtoms > 1)
     {
	Length += ((AtomTable->NumberOfAtoms - 1)* sizeof(RTL_ATOM));
     }

   DPRINT1("RequiredLength: %lu\n", Length);

   if (ReturnLength != NULL)
     {
	*ReturnLength = Length;
     }

   if (Length > AtomInformationLength)
     {
	return STATUS_BUFFER_TOO_SMALL;
     }

   AtomInformation->NumberOfAtoms = AtomTable->NumberOfAtoms;

   j = 0;
   for (i = 0; i < AtomTable->TableSize; i++)
     {
	Current = AtomTable->Slot[i].Flink;
	while (Current != &AtomTable->Slot[i])
	  {
	     Next = Current->Flink;
	     AtomEntry = (PRTL_ATOM_ENTRY)Current;

	     AtomInformation->Atoms[j] = AtomEntry->Index + 0xC000;
	     j++;

	     Current = Next;
	  }
     }

   return STATUS_SUCCESS;
}

/* EOF */
