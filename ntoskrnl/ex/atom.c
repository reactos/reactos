/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/atom.c
 * PURPOSE:         Executive Atom Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Gunnar Dalsnes
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

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

/*++
 * @name ExpGetGlobalAtomTable
 *
 * Gets pointer to a global atom table, creates it if not already created
 *
 * @return Pointer to the RTL_ATOM_TABLE, or NULL if it's impossible
 *         to create atom table
 *
 * @remarks Internal function
 *
 *--*/
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

/*++
 * @name NtAddAtom
 * @implemented
 *
 * Function NtAddAtom creates new Atom in Global Atom Table. If Atom
 * with the same name already exist, internal Atom counter is incremented.
 * See: http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Atoms/NtAddAtom.html
 *
 * @param AtomName
 *        Atom name in Unicode
 *
 * @param AtomNameLength
 *        Length of the atom name
 *
 * @param Atom
 *        Pointer to RTL_ATOM
 *
 * @return STATUS_SUCCESS in case of success, proper error code
 *         otherwise.
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
NtAddAtom(IN PWSTR AtomName,
          IN ULONG AtomNameLength,
          OUT PRTL_ATOM Atom)
{
    PRTL_ATOM_TABLE AtomTable = ExpGetGlobalAtomTable();
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LPWSTR CapturedName;
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

    /* Check if we're called from user-mode or kernel-mode */
    if (PreviousMode == KernelMode)
    {
        /* Re-use the given name if kernel mode */
        CapturedName = AtomName;
    }
    else
    {
        /* Check if we have a name */
        if (AtomName)
        {
            /* Allocate an aligned buffer + the null char */
            CapturedSize = ((AtomNameLength + sizeof(WCHAR)) &
                            ~(sizeof(WCHAR) -1));
            CapturedName = ExAllocatePoolWithTag(PagedPool,
                                                 CapturedSize,
                                                 TAG_ATOM);

            if (!CapturedName)
            {
                /* Fail the call */
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* Enter SEH */
            _SEH2_TRY
            {
                /* Probe the atom */
                ProbeForRead(AtomName, AtomNameLength, sizeof(WCHAR));

                /* Copy the name and null-terminate it */
                RtlCopyMemory(CapturedName, AtomName, AtomNameLength);
                CapturedName[AtomNameLength / sizeof(WCHAR)] = UNICODE_NULL;

                /* Probe the atom too */
                if (Atom) ProbeForWriteUshort(Atom);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Return the exception code */
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;
        }
        else
        {
            /* No name */
            CapturedName = NULL;
        }
    }

    /* Call the runtime function */
    Status = RtlAddAtomToAtomTable(AtomTable, CapturedName, &SafeAtom);
    if (NT_SUCCESS(Status) && (Atom))
    {
        /* Success and caller wants the atom back.. .enter SEH */
        _SEH2_TRY
        {
            /* Return the atom */
            *Atom = SafeAtom;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* If we captured anything, free it */
    if ((CapturedName != NULL) && (CapturedName != AtomName))
        ExFreePoolWithTag(CapturedName, TAG_ATOM);

    /* Return to caller */
    return Status;
}

/*++
 * @name NtDeleteAtom
 * @implemented
 *
 * Removes Atom from Global Atom Table. If Atom's reference counter
 * is greater then 1, function decrements this counter, but Atom
 * stayed in Global Atom Table.
 * See: http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Atoms/NtDeleteAtom.html
 *
 * @param Atom
 *        Atom identifier
 *
 * @return STATUS_SUCCESS in case of success, proper error code
 *         otherwise.
 *
 * @remarks None
 *
 *--*/
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

/*++
 * @name NtFindAtom
 * @implemented
 *
 * Retrieves existing Atom's identifier without incrementing Atom's
 * internal counter
 * See: http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Atoms/NtFindAtom.html
 *
 * @param AtomName
 *        Atom name in Unicode
 *
 * @param AtomNameLength
 *        Length of the atom name
 *
 * @param Atom
 *        Pointer to RTL_ATOM
 *
 * @return STATUS_SUCCESS in case of success, proper error code
 *         otherwise.
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
NtFindAtom(IN PWSTR AtomName,
           IN ULONG AtomNameLength,
           OUT PRTL_ATOM Atom)
{
    PRTL_ATOM_TABLE AtomTable = ExpGetGlobalAtomTable();
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    _SEH2_VOLATILE LPWSTR CapturedName;
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

    /* Re-use the given name if kernel mode or no atom name */
    CapturedName = AtomName;

    /* Check if we're called from user-mode*/
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH */
        _SEH2_TRY
        {
            /* Check if we have a name */
            if (AtomName)
            {
                /* Probe the atom */
                ProbeForRead(AtomName, AtomNameLength, sizeof(WCHAR));

                /* Allocate an aligned buffer + the null char */
                CapturedSize = ((AtomNameLength + sizeof(WCHAR)) &~
                                (sizeof(WCHAR) -1));
                CapturedName = ExAllocatePoolWithQuotaTag(PagedPool,
                                                          CapturedSize,
                                                          TAG_ATOM);
                /* Copy the name and null-terminate it */
                RtlCopyMemory(CapturedName, AtomName, AtomNameLength);
                CapturedName[AtomNameLength / sizeof(WCHAR)] = UNICODE_NULL;

                /* Probe the atom too */
                if (Atom) ProbeForWriteUshort(Atom);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            if (CapturedName != AtomName)
            {
                ExFreePoolWithTag(CapturedName, TAG_ATOM);
            }

            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Call the runtime function */
    Status = RtlLookupAtomInAtomTable(AtomTable, CapturedName, &SafeAtom);
    if (NT_SUCCESS(Status) && (Atom))
    {
        /* Success and caller wants the atom back... enter SEH */
        _SEH2_TRY
        {
            /* Return the atom */
            *Atom = SafeAtom;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* If we captured anything, free it */
    if ((CapturedName) && (CapturedName != AtomName))
        ExFreePoolWithTag(CapturedName, TAG_ATOM);

    /* Return to caller */
    return Status;
}

/*++
 * @name NtQueryInformationAtom
 * @implemented
 *
 * Gets single Atom properties or reads Global Atom Table
 * See: http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Atoms/NtQueryInformationAtom.html
 *
 * @param Atom
 *        Atom to query. If AtomInformationClass parameter is
 *        AtomTableInformation, Atom parameter is not used.
 *
 * @param AtomInformationClass
 *        See ATOM_INFORMATION_CLASS enumeration type for details
 *
 * @param AtomInformation
 *        Result of call - pointer to user's allocated buffer for data
 *
 * @param AtomInformationLength
 *        Size of AtomInformation buffer, in bytes
 *
 * @param ReturnLength
 *        Pointer to ULONG value containing required AtomInformation
 *        buffer size
 *
 * @return STATUS_SUCCESS in case of success, proper error code
 *         otherwise.
 *
 * @remarks None
 *
 *--*/
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

    _SEH2_TRY
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
                    _SEH2_LEAVE;
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
                    _SEH2_LEAVE;
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
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return to caller */
    return Status;
}

/* EOF */
