/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/atom.c
 * PURPOSE:         Atom functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/
#include <k32.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PRTL_ATOM_TABLE BaseLocalAtomTable = NULL;

/* FUNCTIONS *****************************************************************/

PVOID
WINAPI
InternalInitAtomTable(VOID)
{
    /* Create or return the local table */
    if (!BaseLocalAtomTable) RtlCreateAtomTable(0, &BaseLocalAtomTable);
    return BaseLocalAtomTable;
}

ATOM
WINAPI
InternalAddAtom(BOOLEAN Local,
                BOOLEAN Unicode,
                LPCSTR AtomName)
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING AtomNameString;
    ATOM Atom = INVALID_ATOM;

    /* Check if it's an integer atom */
    if ((ULONG_PTR)AtomName <= 0xFFFF)
    {
        /* Convert the name to an atom */
        Atom = (ATOM)PtrToShort((PVOID)AtomName);
        if (Atom >= MAXINTATOM)
        {
            /* Fail, atom number too large */
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return INVALID_ATOM;
        }

        /* Return it */
        return Atom;
    }
    else
    {
        /* Check if this is a unicode atom */
        if (Unicode)
        {
            /* Use a unicode string */
            AtomNameString = &UnicodeString;
            RtlInitUnicodeString(AtomNameString, (LPWSTR)AtomName);
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Use an ansi string */
            RtlInitAnsiString(&AnsiString, AtomName );

            /* Check if we can abuse the TEB */
            if (AnsiString.MaximumLength > 260)
            {
                /* We can't, allocate a new string */
                AtomNameString = &UnicodeString;
                Status = RtlAnsiStringToUnicodeString(AtomNameString,
                                                      &AnsiString,
                                                      TRUE);
            }
            else
            {
                /* We can! Get the TEB String */
                AtomNameString = &NtCurrentTeb()->StaticUnicodeString;

                /* Convert it into the TEB */
                Status = RtlAnsiStringToUnicodeString(AtomNameString,
                                                      &AnsiString,
                                                      FALSE);
            }
        }

        /* Check for failure */
        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            return Atom;
        }
    }

    /* Check if we're doing local add */
    if (Local)
    {
        /* Do a local add */
        Status = RtlAddAtomToAtomTable(InternalInitAtomTable(),
                                       AtomNameString->Buffer,
                                       &Atom);
    }
    else
    {
        /* Do a global add */
        Status = NtAddAtom(AtomNameString->Buffer,
                           AtomNameString->Length,
                           &Atom);
    }

    /* Check for failure */
    if (!NT_SUCCESS(Status)) BaseSetLastNTError(Status);

    /* Check if we were non-static ANSI */
    if (!(Unicode) && (AtomNameString == &UnicodeString))
    {
        /* Free the allocated buffer */
        RtlFreeUnicodeString(AtomNameString);
    }

    /* Return the atom */
    return Atom;
}

ATOM
WINAPI
InternalFindAtom(BOOLEAN Local,
                 BOOLEAN Unicode,
                 LPCSTR AtomName)
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING AtomNameString;
    ATOM Atom = INVALID_ATOM;

    /* Check if it's an integer atom */
    if ((ULONG_PTR)AtomName <= 0xFFFF)
    {
        /* Convert the name to an atom */
        Atom = (ATOM)PtrToShort((PVOID)AtomName);
        if (Atom >= MAXINTATOM)
        {
            /* Fail, atom number too large */
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            DPRINT1("Invalid atom\n");
        }

        /* Return it */
        return Atom;
    }
    else
    {
        /* Check if this is a unicode atom */
        if (Unicode)
        {
            /* Use a unicode string */
            AtomNameString = &UnicodeString;
            RtlInitUnicodeString(AtomNameString, (LPWSTR)AtomName);
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Use an ansi string */
            RtlInitAnsiString(&AnsiString, AtomName);

            /* Check if we can abuse the TEB */
            if (AnsiString.MaximumLength > 260)
            {
                /* We can't, allocate a new string */
                AtomNameString = &UnicodeString;
                Status = RtlAnsiStringToUnicodeString(AtomNameString,
                                                      &AnsiString,
                                                      TRUE);
            }
            else
            {
                /* We can! Get the TEB String */
                AtomNameString = &NtCurrentTeb()->StaticUnicodeString;

                /* Convert it into the TEB */
                Status = RtlAnsiStringToUnicodeString(AtomNameString,
                                                      &AnsiString,
                                                      FALSE);
            }
        }

        /* Check for failure */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed\n");
            BaseSetLastNTError(Status);
            return Atom;
        }
    }

    /* Check if we're doing local lookup */
    if (Local)
    {
        /* Do a local lookup */
        Status = RtlLookupAtomInAtomTable(InternalInitAtomTable(),
                                          AtomNameString->Buffer,
                                          &Atom);
    }
    else
    {
        /* Do a global search */
        if (!AtomNameString->Length)
        {
            /* This is illegal in win32 */
            DPRINT1("No name given\n");
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
        else
        {
            /* Call the global function */
            Status = NtFindAtom(AtomNameString->Buffer,
                                AtomNameString->Length,
                                &Atom);
        }
    }

    /* Check for failure */
    if (!NT_SUCCESS(Status)) BaseSetLastNTError(Status);

    /* Check if we were non-static ANSI */
    if (!(Unicode) && (AtomNameString == &UnicodeString))
    {
        /* Free the allocated buffer */
        RtlFreeUnicodeString(AtomNameString);
    }

    /* Return the atom */
    return Atom;
}

ATOM
WINAPI
InternalDeleteAtom(BOOLEAN Local,
                   ATOM Atom)
{
    NTSTATUS Status;

    /* Validate it */
    if (Atom >= MAXINTATOM)
    {
        /* Check if it's a local delete */
        if (Local)
        {
            /* Delete it locally */
            Status = RtlDeleteAtomFromAtomTable(InternalInitAtomTable(), Atom);
        }
        else
        {
            /* Delete it globall */
            Status = NtDeleteAtom(Atom);
        }

        /* Check for success */
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            BaseSetLastNTError(Status);
            return INVALID_ATOM;
        }
    }

    /* Return failure */
    return 0;
}

UINT
WINAPI
InternalGetAtomName(BOOLEAN Local,
                    BOOLEAN Unicode,
                    ATOM Atom,
                    LPSTR AtomName,
                    DWORD Size)
{
    NTSTATUS Status;
    DWORD RetVal = 0;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PVOID TempBuffer = NULL;
    PWSTR AtomNameString;
    ULONG AtomInfoLength;
    ULONG AtomNameLength;
    PATOM_BASIC_INFORMATION AtomInfo;

    /* Normalize the size as not to overflow */
    if (!Unicode && Size > 0x7000) Size = 0x7000;

    /* Make sure it's valid too */
    if (!Size)
    {
        BaseSetLastNTError(STATUS_BUFFER_OVERFLOW);
        return 0;
    }
    if (!Atom)
    {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return 0;
    }

    /* Check if this is a global query */
    if (Local)
    {
        /* Set the query length */
        AtomNameLength = Size * sizeof(WCHAR);

        /* If it's unicode, just keep the name */
        if (Unicode)
        {
            AtomNameString = (PWSTR)AtomName;
        }
        else
        {
            /* Allocate memory for the ansi buffer */
            TempBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                         0,
                                         AtomNameLength);
            AtomNameString = TempBuffer;
        }

        /* Query the name */
        Status = RtlQueryAtomInAtomTable(InternalInitAtomTable(),
                                         Atom,
                                         NULL,
                                         NULL,
                                         AtomNameString,
                                         &AtomNameLength);
    }
    else
    {
        /* We're going to do a global query, so allocate a buffer */
        AtomInfoLength = sizeof(ATOM_BASIC_INFORMATION) +
                         (Size * sizeof(WCHAR));
        AtomInfo = TempBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                                0,
                                                AtomInfoLength);

        if (!AtomInfo)
        {
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return 0;
        }

        /* Query the name */
        Status = NtQueryInformationAtom(Atom,
                                        AtomBasicInformation,
                                        AtomInfo,
                                        AtomInfoLength,
                                        &AtomInfoLength);
        if (NT_SUCCESS(Status))
        {
            /* Success. Update the length and get the name */
            AtomNameLength = (ULONG)AtomInfo->NameLength;
            AtomNameString = AtomInfo->Name;
        }
    }

    /* Check for global success */
    if (NT_SUCCESS(Status))
    {
        /* Check if it was unicode */
        if (Unicode)
        {
            /* We return the length in chars */
            RetVal = AtomNameLength / sizeof(WCHAR);

            /* Copy the memory if this was a global query */
            if (AtomNameString != (PWSTR)AtomName)
            {
                RtlMoveMemory(AtomName, AtomNameString, AtomNameLength);
            }

            /* And null-terminate it if the buffer was too large */
            if (RetVal < Size)
            {
                ((PWCHAR)AtomName)[RetVal] = UNICODE_NULL;
            }
        }
        else
        {
            /* First create a unicode string with our data */
            UnicodeString.Buffer = AtomNameString;
            UnicodeString.Length = (USHORT)AtomNameLength;
            UnicodeString.MaximumLength = (USHORT)(UnicodeString.Length +
                                                   sizeof(WCHAR));

            /* Now prepare an ansi string for conversion */
            AnsiString.Buffer = AtomName;
            AnsiString.Length = 0;
            AnsiString.MaximumLength = (USHORT)Size;

            /* Convert it */
            Status = RtlUnicodeStringToAnsiString(&AnsiString,
                                                  &UnicodeString,
                                                  FALSE);

            /* Return the length */
            if (NT_SUCCESS(Status)) RetVal = AnsiString.Length;
        }
    }

    /* Free the temporary buffer if we have one */
    if (TempBuffer) RtlFreeHeap(RtlGetProcessHeap(), 0, TempBuffer);

    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        DPRINT("Failed: %lx\n", Status);
        BaseSetLastNTError(Status);
    }

    /* Return length */
    return RetVal;
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
ATOM
WINAPI
GlobalAddAtomA(LPCSTR lpString)
{
    return InternalAddAtom(FALSE, FALSE, lpString);
}

/*
 * @implemented
 */
ATOM
WINAPI
GlobalAddAtomW(LPCWSTR lpString)
{
    return InternalAddAtom(FALSE, TRUE, (LPSTR)lpString);
}

/*
 * @implemented
 */
ATOM
WINAPI
GlobalDeleteAtom(ATOM nAtom)
{
    return InternalDeleteAtom(FALSE, nAtom);
}

/*
 * @implemented
 */
ATOM
WINAPI
GlobalFindAtomA(LPCSTR lpString)
{
    return InternalFindAtom(FALSE, FALSE, lpString);
}

/*
 * @implemented
 */
ATOM
WINAPI
GlobalFindAtomW(LPCWSTR lpString)
{
    return InternalFindAtom(FALSE, TRUE, (LPSTR)lpString);
}

/*
 * @implemented
 */
UINT
WINAPI
GlobalGetAtomNameA(ATOM nAtom,
                   LPSTR lpBuffer,
                   int nSize)
{
    return InternalGetAtomName(FALSE, FALSE, nAtom, lpBuffer, (DWORD)nSize);
}

/*
 * @implemented
 */
UINT
WINAPI
GlobalGetAtomNameW(ATOM nAtom,
                   LPWSTR lpBuffer,
                   int nSize)
{
    return InternalGetAtomName(FALSE,
                               TRUE,
                               nAtom,
                               (LPSTR)lpBuffer,
                               (DWORD)nSize);
}

/*
 * @implemented
 */
BOOL
WINAPI
InitAtomTable(DWORD nSize)
{
    /* Normalize size */
    if (nSize < 4 || nSize > 511) nSize = 37;

    DPRINT("Here\n");
    return NT_SUCCESS(RtlCreateAtomTable(nSize, &BaseLocalAtomTable));
}

/*
 * @implemented
 */
ATOM
WINAPI
AddAtomA(LPCSTR lpString)
{
    return InternalAddAtom(TRUE, FALSE, lpString);
}

/*
 * @implemented
 */
ATOM
WINAPI
AddAtomW(LPCWSTR lpString)
{
    return InternalAddAtom(TRUE, TRUE, (LPSTR)lpString);
}

/*
 * @implemented
 */
ATOM
WINAPI
DeleteAtom(ATOM nAtom)
{
    return InternalDeleteAtom(TRUE, nAtom);
}

/*
 * @implemented
 */
ATOM
WINAPI
FindAtomA(LPCSTR lpString)
{
    return InternalFindAtom(TRUE, FALSE, lpString);
}

/*
 * @implemented
 */
ATOM
WINAPI
FindAtomW(LPCWSTR lpString)
{
    return InternalFindAtom(TRUE, TRUE, (LPSTR)lpString);

}

/*
 * @implemented
 */
UINT
WINAPI
GetAtomNameA(ATOM nAtom,
             LPSTR lpBuffer,
             int nSize)
{
    return InternalGetAtomName(TRUE, FALSE, nAtom, lpBuffer, (DWORD)nSize);
}

/*
 * @implemented
 */
UINT
WINAPI
GetAtomNameW(ATOM nAtom,
             LPWSTR lpBuffer,
             int nSize)
{
    return InternalGetAtomName(TRUE,
                               TRUE,
                               nAtom,
                               (LPSTR)lpBuffer,
                               (DWORD)nSize);
}
/* EOF */
