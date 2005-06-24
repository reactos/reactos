/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/atom.c
 * PURPOSE:         Atom managment
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* PROTOTYPES ****************************************************************/

static PRTL_ATOM_TABLE RtlpGetGlobalAtomTable(VOID);
static NTSTATUS
RtlpQueryAtomInformation(PRTL_ATOM_TABLE AtomTable,
			 RTL_ATOM Atom,
			 PATOM_BASIC_INFORMATION AtomInformation,
			 ULONG AtomInformationLength,
			 PULONG ReturnLength);
static NTSTATUS
RtlpQueryAtomTableInformation(PRTL_ATOM_TABLE AtomTable,
			      RTL_ATOM Atom,
			      PATOM_TABLE_INFORMATION AtomInformation,
			      ULONG AtomInformationLength,
			      PULONG ReturnLength);

extern NTSTATUS STDCALL
RtlQueryAtomListInAtomTable(IN PRTL_ATOM_TABLE AtomTable,
                            IN ULONG MaxAtomCount,
                            OUT ULONG *AtomCount,
                            OUT RTL_ATOM *AtomList);

/* GLOBALS *******************************************************************/

static PRTL_ATOM_TABLE GlobalAtomTable = NULL;

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
NTSTATUS STDCALL
NtAddAtom(
   IN PWSTR AtomName,
   IN ULONG AtomNameLength,
   OUT PRTL_ATOM Atom)
{
   PRTL_ATOM_TABLE AtomTable;

   AtomTable = RtlpGetGlobalAtomTable();
   if (AtomTable == NULL)
      return STATUS_ACCESS_DENIED;

   return RtlAddAtomToAtomTable(AtomTable, AtomName, Atom);
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
           IN ULONG AtomNameLength,
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

   DPRINT("NameLength: %lu\n", NameLength);

   if (ReturnLength != NULL)
     {
	*ReturnLength = NameLength + sizeof(ATOM_BASIC_INFORMATION);
     }

   if (NameLength + sizeof(ATOM_BASIC_INFORMATION) > AtomInformationLength)
     {
	return STATUS_INFO_LENGTH_MISMATCH;
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
   ULONG Length;
   NTSTATUS Status;

   Length = sizeof(ATOM_TABLE_INFORMATION);

   DPRINT("RequiredLength: %lu\n", Length);

   if (ReturnLength != NULL)
     {
	*ReturnLength = Length;
     }

   if (Length > AtomInformationLength)
     {
	return STATUS_INFO_LENGTH_MISMATCH;
     }

   Status = RtlQueryAtomListInAtomTable(AtomTable,
				        (AtomInformationLength - Length) / sizeof(RTL_ATOM),
				        &AtomInformation->NumberOfAtoms,
				        AtomInformation->Atoms);
   if (NT_SUCCESS(Status))
     {
        ReturnLength += AtomInformation->NumberOfAtoms * sizeof(RTL_ATOM);
      
        if (ReturnLength != NULL)
          {
	     *ReturnLength = Length;
          }
     }

   return Status;
}

/* EOF */
