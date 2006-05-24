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

#define TAG_ATOM TAG('A', 't', 'o', 'm')

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
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LPWSTR CapturedName = NULL;
    ULONG CapturedSize;
    RTL_ATOM SafeAtom;
    PAGED_CODE();

    /* Check for the table */
    if (AtomTable == NULL) return STATUS_ACCESS_DENIED;

    /* Check for valid name */
    if (AtomNameLength > (RTL_MAXIMUM_ATOM_LENGTH * sizeof(WCHAR)))
    {
        /* Fail */
        DPRINT1("Atom name too long\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we're called from user-mode*/
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH */
        _SEH_TRY
        {
            /* Check if we have a name */
            if (AtomName)
            {
                /* Probe the atom */
                ProbeForRead(AtomName, AtomNameLength, sizeof(WCHAR));

                /* Allocate an aligned buffer + the null char */
                CapturedSize = ((AtomNameLength + sizeof(WCHAR)) &~
                                (sizeof(WCHAR) -1));
                CapturedName = ExAllocatePoolWithTag(PagedPool,
                                                     CapturedSize,
                                                     TAG_ATOM);
                if (!CapturedName)
                {
                    /* Fail the call */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    /* Copy the name and null-terminate it */
                    RtlMoveMemory(CapturedName, AtomName, AtomNameLength);
                    CapturedName[AtomNameLength / sizeof(WCHAR)] = UNICODE_NULL;
                }

                /* Probe the atom too */
                if (Atom) ProbeForWriteUshort(Atom);
            }
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }
    else
    {
        /* Simplify code and re-use one variable */
        if (AtomName) CapturedName = AtomName;
    }

    /* Make sure probe worked */
    if (NT_SUCCESS(Status))
    {
        /* Call the runtime function */
        Status = RtlAddAtomToAtomTable(AtomTable, CapturedName, &SafeAtom);
        if (NT_SUCCESS(Status) && (Atom))
        {
            /* Success and caller wants the atom back.. .enter SEH */
            _SEH_TRY
            {
                /* Return the atom */
                *Atom = SafeAtom;
            }
            _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
    }

    /* If we captured anything, free it */
    if ((CapturedName) && (CapturedName != AtomName)) ExFreePool(CapturedName);

    /* Return to caller */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtDeleteAtom(IN RTL_ATOM Atom)
{
    PRTL_ATOM_TABLE AtomTable = ExpGetGlobalAtomTable();
    PAGED_CODE();

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
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LPWSTR CapturedName = NULL;
    ULONG CapturedSize;
    RTL_ATOM SafeAtom;
    PAGED_CODE();

    /* Check for the table */
    if (AtomTable == NULL) return STATUS_ACCESS_DENIED;

    /* Check for valid name */
    if (AtomNameLength > (RTL_MAXIMUM_ATOM_LENGTH * sizeof(WCHAR)))
    {
        /* Fail */
        DPRINT1("Atom name too long\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we're called from user-mode*/
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH */
        _SEH_TRY
        {
            /* Check if we have a name */
            if (AtomName)
            {
                /* Probe the atom */
                ProbeForRead(AtomName, AtomNameLength, sizeof(WCHAR));

                /* Allocate an aligned buffer + the null char */
                CapturedSize = ((AtomNameLength + sizeof(WCHAR)) &~
                                (sizeof(WCHAR) -1));
                CapturedName = ExAllocatePoolWithTag(PagedPool,
                                                     CapturedSize,
                                                     TAG_ATOM);
                if (!CapturedName)
                {
                    /* Fail the call */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    /* Copy the name and null-terminate it */
                    RtlMoveMemory(CapturedName, AtomName, AtomNameLength);
                    CapturedName[AtomNameLength / sizeof(WCHAR)] = UNICODE_NULL;
                }

                /* Probe the atom too */
                if (Atom) ProbeForWriteUshort(Atom);
            }
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }
    else
    {
        /* Simplify code and re-use one variable */
        if (AtomName) CapturedName = AtomName;
    }

    /* Make sure probe worked */
    if (NT_SUCCESS(Status))
    {
        /* Call the runtime function */
        Status = RtlLookupAtomInAtomTable(AtomTable, CapturedName, &SafeAtom);
        if (NT_SUCCESS(Status) && (Atom))
        {
            /* Success and caller wants the atom back.. .enter SEH */
            _SEH_TRY
            {
                /* Return the atom */
                *Atom = SafeAtom;
            }
            _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
    }

    /* If we captured anything, free it */
    if ((CapturedName) && (CapturedName != AtomName)) ExFreePool(CapturedName);

    /* Return to caller */
    return Status;
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
    PATOM_BASIC_INFORMATION BasicInformation = AtomInformation;
    PATOM_TABLE_INFORMATION TableInformation = AtomInformation;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Flags, UsageCount, NameLength, RequiredLength = 0;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    /* Check for valid table */
    if (AtomTable == NULL) return STATUS_ACCESS_DENIED;

    PreviousMode = ExGetPreviousMode();

    _SEH_TRY
    {
        /* Probe the parameters */
        if (PreviousMode != KernelMode)
        {
            ProbeForWrite(AtomInformation,
                          AtomInformationLength,
                          sizeof(ULONG));

            if (ReturnLength != NULL)
            {
                ProbeForWriteUlong(ReturnLength);
            }
        }

        /* Choose class */
        switch (AtomInformationClass)
        {
            /* Caller requested info about an atom */
            case AtomBasicInformation:

                /* Size check */
                RequiredLength = FIELD_OFFSET(ATOM_BASIC_INFORMATION, Name);
                if (RequiredLength > AtomInformationLength)
                {
                    /* Fail */
                    DPRINT1("Buffer too small\n");
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH_LEAVE;
                }

                /* Prepare query */
                UsageCount = 0;
                NameLength = AtomInformationLength - RequiredLength;
                BasicInformation->Name[0] = UNICODE_NULL;

                /* Query the data */
                Status = RtlQueryAtomInAtomTable(AtomTable,
                                                 Atom,
                                                 &UsageCount,
                                                 &Flags,
                                                 BasicInformation->Name,
                                                 &NameLength);
                if (NT_SUCCESS(Status))
                {
                    /* Return data */
                    BasicInformation->UsageCount = (USHORT)UsageCount;
                    BasicInformation->Flags = (USHORT)Flags;
                    BasicInformation->NameLength = (USHORT)NameLength;
                    RequiredLength += NameLength + sizeof(WCHAR);
                }
                break;

            /* Caller requested info about an Atom Table */
            case AtomTableInformation:

                /* Size check */
                RequiredLength = FIELD_OFFSET(ATOM_TABLE_INFORMATION, Atoms);
                if (RequiredLength > AtomInformationLength)
                {
                    /* Fail */
                    DPRINT1("Buffer too small\n");
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH_LEAVE;
                }

                /* Query the data */
                Status = RtlQueryAtomListInAtomTable(AtomTable,
                                                     (AtomInformationLength - RequiredLength) /
                                                     sizeof(RTL_ATOM),
                                                     &TableInformation->NumberOfAtoms,
                                                     TableInformation->Atoms);
                if (NT_SUCCESS(Status))
                {
                    /* Update the return length */
                    RequiredLength += TableInformation->NumberOfAtoms * sizeof(RTL_ATOM);
                }
                break;

            /* Caller was on crack */
            default:

                /* Unrecognized class */
                Status = STATUS_INVALID_INFO_CLASS;
                break;
        }

        /* Return the required size */
        if (ReturnLength != NULL)
        {
            *ReturnLength = RequiredLength;
        }
    }
    _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Return to caller */
    return Status;
}

/* EOF */
