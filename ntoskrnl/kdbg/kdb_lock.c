/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kdbg/kdb_lock.c
 * PURPOSE:         Kernel Debugger Lock Tracking
 *
 * PROGRAMMERS:     arty
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* DEFINES *******************************************************************/

#define START_OFFSET 1
#define NUM_ADDRS 2

/* TYPES *********************************************************************/

#ifdef DBG

typedef struct _REACTOS_LIST_COMMON_HEAD {
	LIST_ENTRY Entry;
	PVOID Object;
} REACTOS_LIST_COMMON_HEAD, *PREACTOS_LIST_COMMON_HEAD;
typedef struct _REACTOS_NAMED_WAIT {
	REACTOS_LIST_COMMON_HEAD Head;
	UNICODE_STRING Name;
} REACTOS_NAMED_WAIT, *PREACTOS_NAMED_WAIT;

typedef struct _REACTOS_HELD_WAIT {
	REACTOS_LIST_COMMON_HEAD Head;
	ULONG Thread;
	PVOID Where[NUM_ADDRS];
} REACTOS_HELD_WAIT, *PREACTOS_HELD_WAIT;

typedef struct _REACTOS_WAITING {
	REACTOS_LIST_COMMON_HEAD Head;
	ULONG Thread;
	PVOID Where[NUM_ADDRS];
} REACTOS_WAITING, *PREACTOS_WAITING;

static KSPIN_LOCK WaiterListLock;
static LIST_ENTRY NamedListHead;
static LIST_ENTRY HeldListHead;
static LIST_ENTRY WaiterListHead;

VOID KdbgInitWaitReporting()
{
	KeInitializeSpinLock(&WaiterListLock);
	InitializeListHead(&NamedListHead);
	InitializeListHead(&HeldListHead);
	InitializeListHead(&WaiterListHead);
}

/* Allowing many strings means we can add the name of a file or whatever
 * we like */
VOID KdbgRegisterNamedObject
(PUNICODE_STRING Strings, 
 ULONG NumberOfStrings, 
 PVOID Address)
{
	ULONG i;
	UNICODE_STRING Comma = RTL_CONSTANT_STRING(L":");
	PREACTOS_NAMED_WAIT NamedEntry = 
		ExAllocatePool(NonPagedPool, sizeof(*NamedEntry));
	KIRQL OldIrql;

	ASSERT(NamedEntry);
	NamedEntry->Name.MaximumLength = sizeof(WCHAR) * 2 * NumberOfStrings - 1;
	for (i = 0; i < NumberOfStrings; i++)
	{
		NamedEntry->Name.MaximumLength += Strings[i].Length;
	}
	NamedEntry->Name.Length = 0;
	NamedEntry->Name.Buffer = ExAllocatePool
		(NonPagedPool, NamedEntry->Name.MaximumLength);
	for (i = 0; i < NumberOfStrings; i++)
	{
		if (i) RtlAppendUnicodeStringToString(&NamedEntry->Name, &Comma);
		RtlAppendUnicodeStringToString(&NamedEntry->Name, &Strings[i]);
	}
	NamedEntry->Head.Object = Address;

	KeAcquireSpinLock(&WaiterListLock, &OldIrql);
	InsertTailList(&NamedListHead, &NamedEntry->Head.Entry);
	KeReleaseSpinLock(&WaiterListLock, OldIrql);
}

static VOID RemoveObjectFromList(PLIST_ENTRY ListHead, PVOID Address, BOOLEAN FreeName)
{
	PLIST_ENTRY Entry;
	PREACTOS_NAMED_WAIT NamedEntry;

	for (Entry = ListHead->Flink; 
		 Entry != ListHead;
		 Entry = Entry->Flink)
	{
		NamedEntry = CONTAINING_RECORD(Entry, REACTOS_NAMED_WAIT, Head.Entry);
		if (NamedEntry->Head.Object == Address)
		{
			if (FreeName) ExFreePool(NamedEntry->Name.Buffer);
			RemoveEntryList(&NamedEntry->Head.Entry);
			ExFreePool(NamedEntry);
			return;
		}
	}
	ASSERT(FALSE);
}

static PVOID FindObjectInList(PLIST_ENTRY ListHead, PVOID Address)
{
	PLIST_ENTRY Entry;
	PREACTOS_LIST_COMMON_HEAD NamedEntry;

	for (Entry = ListHead->Flink; 
		 Entry != ListHead;
		 Entry = Entry->Flink)
	{
		NamedEntry = CONTAINING_RECORD(Entry, REACTOS_LIST_COMMON_HEAD, Entry);
		if (NamedEntry->Object == Address) return NamedEntry;
	}
	return NULL;
}

static PUNICODE_STRING GetNameOfObject(PVOID Address)
{
	PREACTOS_NAMED_WAIT NamedWait = FindObjectInList(&NamedListHead, Address);
	if (NamedWait) return &NamedWait->Name;
	else return NULL;
}

VOID KdbgDeleteNamedObject(PVOID Address)
{
	KIRQL OldIrql;
	KeAcquireSpinLock(&WaiterListLock, &OldIrql);
	RemoveObjectFromList(&NamedListHead, Address, TRUE);
	KeReleaseSpinLock(&WaiterListLock, OldIrql);
}

VOID KdbgEnterWaitable(PVOID Address)
{
	KIRQL OldIrql;
	PREACTOS_HELD_WAIT HeldWait;
	KeAcquireSpinLock(&WaiterListLock, &OldIrql);
	HeldWait = ExAllocatePool(NonPagedPool, sizeof(*HeldWait));
	ASSERT(HeldWait);
	HeldWait->Head.Object = Address;
	HeldWait->Thread = (ULONG)PsGetCurrentThreadId();
	HeldWait->Where[0] = __builtin_return_address(START_OFFSET);
	HeldWait->Where[1] = __builtin_return_address(START_OFFSET+1);
	InsertTailList(&HeldListHead, &HeldWait->Head.Entry);
	KeReleaseSpinLock(&WaiterListLock, OldIrql);
}

VOID KdbgLeaveWaitable(PVOID Address)
{
	KIRQL OldIrql;
	KeAcquireSpinLock(&WaiterListLock, &OldIrql);
	RemoveObjectFromList(&HeldListHead, Address, FALSE);
	KeReleaseSpinLock(&WaiterListLock, OldIrql);
}

VOID KdbgDeclareWait(PVOID Address)
{
	KIRQL OldIrql;
	PREACTOS_WAITING Waiter;
	KeAcquireSpinLock(&WaiterListLock, &OldIrql);
	Waiter = ExAllocatePool(NonPagedPool, sizeof(*Waiter));
	ASSERT(Waiter);
	Waiter->Head.Object = Address;
	Waiter->Thread = (ULONG)PsGetCurrentThreadId();
	Waiter->Where[0] = __builtin_return_address(START_OFFSET);
	Waiter->Where[1] = __builtin_return_address(START_OFFSET+1);
	InsertTailList(&WaiterListHead, &Waiter->Head.Entry);
	KeReleaseSpinLock(&WaiterListLock, OldIrql);
}

void KdbgDeclareMultiWait(PVOID *Addresses, ULONG Count)
{
	ULONG i;
	for (i = 0; i < Count; i++)
	{
		KdbgDeclareWait(Addresses[i]);
	}
}

VOID KdbgSatisfyWait(PVOID Address)
{
	KIRQL OldIrql;
	KeAcquireSpinLock(&WaiterListLock, &OldIrql);
	RemoveObjectFromList(&WaiterListHead, Address, FALSE);
	KeReleaseSpinLock(&WaiterListLock, OldIrql);
}

void KdbgSatisfyMultiWait(PVOID *Addresses, ULONG Count)
{
	ULONG i;
	for (i = 0; i < Count; i++)
	{
		KdbgSatisfyWait(Addresses[i]);
	}
}

VOID KdbgWaitDescribeThread(ULONG Thread)
{
	KIRQL OldIrql;
	PLIST_ENTRY Entry;
	PUNICODE_STRING ObjectName;
	PREACTOS_HELD_WAIT Held;
	PREACTOS_WAITING Waiter;
	UNICODE_STRING Unnamed = RTL_CONSTANT_STRING(L"unnamed");
	
	KeAcquireSpinLock(&WaiterListLock, &OldIrql);
	DbgPrint("Thread %x Waiting on:\n", Thread);
	for (Entry = WaiterListHead.Flink;
		 Entry != &WaiterListHead;
		 Entry = Entry->Flink)
	{
		Waiter = CONTAINING_RECORD(Entry, REACTOS_WAITING, Head.Entry);
		if (Waiter->Thread == Thread)
		{
			ObjectName = GetNameOfObject(Waiter->Head.Object);
			DbgPrint
				(" %x (%wZ) from %x from %x\n", 
				 Waiter->Head.Object, 
				 ObjectName ? ObjectName : &Unnamed,
				 Waiter->Where[0], Waiter->Where[1]);
		}
	}
	DbgPrint("Thread %x Holding:\n", Thread);
	for (Entry = HeldListHead.Flink;
		 Entry != &HeldListHead;
		 Entry = Entry->Flink)
	{
		Held = CONTAINING_RECORD(Entry, REACTOS_HELD_WAIT, Head.Entry);
		if (Held->Thread == Thread)
		{
			ObjectName = GetNameOfObject(Held->Head.Object);
			DbgPrint
				(" %x (%wZ) from %x from %x\n",
				 Held->Head.Object,
				 ObjectName ? ObjectName : &Unnamed,
				 Held->Where[0], Held->Where[1]);
		}
	}
	KeReleaseSpinLock(&WaiterListLock, OldIrql);
}

void KdbgWaitDescribeObject(PVOID Object)
{
	KIRQL OldIrql;
	PLIST_ENTRY Entry;
	PUNICODE_STRING ObjectName;
	PREACTOS_HELD_WAIT Held;
	PREACTOS_WAITING Waiter;
	UNICODE_STRING Unnamed = RTL_CONSTANT_STRING(L"unnamed");
	
	KeAcquireSpinLock(&WaiterListLock, &OldIrql);
	ObjectName = GetNameOfObject(Object);
	DbgPrint
		("Threads waiting on %x %wZ\n", 
		 Object, ObjectName ? ObjectName : &Unnamed);
	for (Entry = WaiterListHead.Flink;
		 Entry != &WaiterListHead;
		 Entry = Entry->Flink)
	{
		Waiter = CONTAINING_RECORD(Entry, REACTOS_WAITING, Head.Entry);
		if (Waiter->Head.Object == Object)
		{
			DbgPrint
				(" %x from %x from %x\n", 
				 Waiter->Thread, Waiter->Where[0], Waiter->Where[1]);
		}
	}
	DbgPrint
		("Threads holding %x %wZ\n", 
		 Object, ObjectName ? ObjectName : &Unnamed);
	for (Entry = HeldListHead.Flink;
		 Entry != &HeldListHead;
		 Entry = Entry->Flink)
	{
		Held = CONTAINING_RECORD(Entry, REACTOS_HELD_WAIT, Head.Entry);
		if (Held->Head.Object == Object)
		{
			DbgPrint
				(" %x from %x from %x\n", 
				 Held->Thread, Held->Where[0], Held->Where[1]);
		}
	}	
	KeReleaseSpinLock(&WaiterListLock, OldIrql);
}

#endif
