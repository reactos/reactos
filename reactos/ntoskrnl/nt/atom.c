/* $Id: atom.c,v 1.4 2000/10/08 12:49:26 ekohl Exp $
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

#include <internal/debug.h>

static PRTL_ATOM_TABLE RtlpGetGlobalAtomTable (VOID);

/* GLOBALS *******************************************************************/

static PRTL_ATOM_TABLE GlobalAtomTable = NULL;

/* FUNCTIONS *****************************************************************/


NTSTATUS
STDCALL
NtAddAtom (OUT PRTL_ATOM Atom,
	   IN PUNICODE_STRING AtomString)
{
   PRTL_ATOM_TABLE AtomTable;

   AtomTable = RtlpGetGlobalAtomTable();
   if (AtomTable == NULL)
     return STATUS_ACCESS_DENIED;

   return (RtlAddAtomToAtomTable(AtomTable,
				 AtomString->Buffer,
				 Atom));
}


NTSTATUS
STDCALL
NtDeleteAtom (IN RTL_ATOM Atom)
{
   PRTL_ATOM_TABLE AtomTable;

   AtomTable = RtlpGetGlobalAtomTable();
   if (AtomTable == NULL)
     return STATUS_ACCESS_DENIED;

   return (RtlDeleteAtomFromAtomTable(AtomTable,
				      Atom));
}


NTSTATUS
STDCALL
NtFindAtom (OUT PRTL_ATOM Atom,
	    IN PUNICODE_STRING AtomString)
{
   PRTL_ATOM_TABLE AtomTable;

   AtomTable = RtlpGetGlobalAtomTable();
   if (AtomTable == NULL)
     return STATUS_ACCESS_DENIED;

   return (RtlLookupAtomInAtomTable(AtomTable,
				    AtomString,
				    Atom));
}


NTSTATUS
STDCALL
NtQueryInformationAtom (IN RTL_ATOM Atom,
	IN	CINT	AtomInformationClass,
	OUT	PVOID	AtomInformation,
	IN	ULONG	AtomInformationLength,
	OUT	PULONG	ReturnLength
	)
{
#if 0
   PRTL_ATOM_TABLE AtomTable;

   AtomTable = RtlpGetGlobalAtomTable();
   if (AtomTable == NULL)
     return STATUS_ACCESS_DENIED;

   return (RtlQueryAtomInAtomTable(AtomTable,
				   Atom,

#endif

   UNIMPLEMENTED;
   return STATUS_UNSUCCESSFUL;
}


static PRTL_ATOM_TABLE
RtlpGetGlobalAtomTable (VOID)
{
   NTSTATUS Status;

   if (GlobalAtomTable != NULL)
     return GlobalAtomTable;

   Status = RtlCreateAtomTable(37, &GlobalAtomTable);
   if (!NT_SUCCESS(Status))
     return NULL;

   return GlobalAtomTable;
}


NTSTATUS STDCALL
RtlCreateAtomTable (ULONG TableSize,
		    PRTL_ATOM_TABLE *AtomTable)
{
#if 0
   PRTL_ATOM_TABLE Table;

   /* allocate atom table */
   Table = ExAllocatePool(NonPagedPool,
			  TableSize * sizeof(RTL_ATOM_ENTRY) +
			  sizeof(RTL_ATOM_TABLE));
   if (Table == NULL)
     return STATUS_NO_MEMORY;

   Table->TableSize = TableSize;

   /* FIXME: more code here */

   *AtomTable = Table;
   return STATUS_SUCCESS;

#endif
   UNIMPLEMENTED;
   return STATUS_UNSUCCESSFUL;
}


NTSTATUS STDCALL
RtlDestroyAtomTable (IN PRTL_ATOM_TABLE AtomTable)
{
   UNIMPLEMENTED;
   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlEmptyAtomTable (IN PRTL_ATOM_TABLE AtomTable,
		   ULONG Unknown2)
{
   UNIMPLEMENTED;
   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlAddAtomToAtomTable (IN PRTL_ATOM_TABLE AtomTable,
		       IN PWSTR AtomName,
		       OUT PRTL_ATOM Atom)
{
   UNIMPLEMENTED;
   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlDeleteAtomFromAtomTable (IN PRTL_ATOM_TABLE AtomTable,
			    IN RTL_ATOM Atom)
{
   UNIMPLEMENTED;
   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlLookupAtomInAtomTable (IN PRTL_ATOM_TABLE AtomTable,
			  IN PUNICODE_STRING AtomName,
			  OUT PRTL_ATOM Atom)
{
   UNIMPLEMENTED;
   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlPinAtomInAtomTable (IN PRTL_ATOM_TABLE AtomTable,
		       IN PRTL_ATOM Atom)
{
   UNIMPLEMENTED;
   return STATUS_SUCCESS;
}


/*
NTSTATUS STDCALL
RtlQueryAtomInAtomTable (IN PRTL_ATOM_TABLE AtomTable,
			 IN RTL_ATOM Atom,
*/


/* EOF */
