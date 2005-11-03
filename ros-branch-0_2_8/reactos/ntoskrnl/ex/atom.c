/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/atom.c
 * PURPOSE:         Executive Atom Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Gunnar Dalsnes
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ****************************************************************/

/* 
 * FIXME: this is WRONG! The global atom table should live in the WinSta struct
 * and accessed through a win32k callout (received in PsEstablishWin32Callouts)
 * NOTE: There is a session/win32k global atom table also, but its private to
 * win32k. Its used for RegisterWindowMessage() and for window classes.
 * -Gunnar
 */
PRTL_ATOM_TABLE GlobalAtomTable;

/* PRIVATE FUNCTIONS *********************************************************/

PRTL_ATOM_TABLE
NTAPI
ExpGetGlobalAtomTable(VOID)
{
    NTSTATUS Status;

    /* Return it if we have one */
    if (GlobalAtomTable) return GlobalAtomTable;

    /* Create it */
    Status = RtlCreateAtomTable(37, &GlobalAtomTable);

    /* If we couldn't create it, return NULL */
    if (!NT_SUCCESS(Status)) return NULL;

    /* Return the newly created one */
    return GlobalAtomTable;
}

NTSTATUS
NTAPI
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

    if (!NT_SUCCESS(Status)) return Status;
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

NTSTATUS
NTAPI
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

    if (ReturnLength) *ReturnLength = Length;

    if (Length > AtomInformationLength) return STATUS_INFO_LENGTH_MISMATCH;

    Status = RtlQueryAtomListInAtomTable(AtomTable,
                                         (AtomInformationLength - Length) /
                                         sizeof(RTL_ATOM),
                                         &AtomInformation->NumberOfAtoms,
                                         AtomInformation->Atoms);
    if (NT_SUCCESS(Status))
    {
        ReturnLength += AtomInformation->NumberOfAtoms * sizeof(RTL_ATOM);
        if (ReturnLength != NULL) *ReturnLength = Length;
    }

    return Status;
}

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtAddAtom(IN PWSTR AtomName,
          IN ULONG AtomNameLength,
          OUT PRTL_ATOM Atom)
{
    PRTL_ATOM_TABLE AtomTable = ExpGetGlobalAtomTable();

    /* Check for the table */
    if (AtomTable == NULL) return STATUS_ACCESS_DENIED;

    /* FIXME: SEH! */

    /* Call the worker function */
    return RtlAddAtomToAtomTable(AtomTable, AtomName, Atom);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtDeleteAtom(IN RTL_ATOM Atom)
{
    PRTL_ATOM_TABLE AtomTable = ExpGetGlobalAtomTable();

    /* Check for valid table */
    if (AtomTable == NULL) return STATUS_ACCESS_DENIED;

    /* Call worker function */
    return RtlDeleteAtomFromAtomTable(AtomTable, Atom);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtFindAtom(IN PWSTR AtomName,
           IN ULONG AtomNameLength,
           OUT PRTL_ATOM Atom)
{
    PRTL_ATOM_TABLE AtomTable = ExpGetGlobalAtomTable();

    /* Check for valid table */
    if (AtomTable == NULL) return STATUS_ACCESS_DENIED;

    /* FIXME: SEH!!! */

    /* Call worker function */
    return RtlLookupAtomInAtomTable(AtomTable, AtomName, Atom);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryInformationAtom(RTL_ATOM Atom,
                       ATOM_INFORMATION_CLASS AtomInformationClass,
                       PVOID AtomInformation,
                       ULONG AtomInformationLength,
                       PULONG ReturnLength)
{
    PRTL_ATOM_TABLE AtomTable = ExpGetGlobalAtomTable();
    NTSTATUS Status;

    /* Check for valid table */
    if (AtomTable == NULL) return STATUS_ACCESS_DENIED;

    /* FIXME: SEH! */

    /* Choose class */
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

    /* Return to caller */
    return Status;
}

/* EOF */
