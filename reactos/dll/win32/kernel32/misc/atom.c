/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/atom.c
 * PURPOSE:         Atom functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/
#include <k32.h>
#include "wine/server.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PRTL_ATOM_TABLE BaseLocalAtomTable = NULL;

/* DEAR GOD, FORGIVE ME ******************************************************/

#define MAX_ATOM_LEN              255

/******************************************************************
 *		is_integral_atom
 * Returns STATUS_SUCCESS if integral atom and 'pAtom' is filled
 *         STATUS_INVALID_PARAMETER if 'atomstr' is too long
 *         STATUS_MORE_ENTRIES otherwise
 */
static NTSTATUS is_integral_atom( LPCWSTR atomstr, size_t len, RTL_ATOM* pAtom )
{
    RTL_ATOM atom;

    if (HIWORD( atomstr ))
    {
        const WCHAR* ptr = atomstr;
        if (!len) return STATUS_OBJECT_NAME_INVALID;

        if (*ptr++ == '#')
        {
            atom = 0;
            while (ptr < atomstr + len && *ptr >= '0' && *ptr <= '9')
            {
                atom = atom * 10 + *ptr++ - '0';
            }
            if (ptr > atomstr + 1 && ptr == atomstr + len) goto done;
        }
        if (len > MAX_ATOM_LEN) return STATUS_INVALID_PARAMETER;
        return STATUS_MORE_ENTRIES;
    }
    else atom = LOWORD( atomstr );
done:
    if (!atom || atom >= MAXINTATOM) return STATUS_INVALID_PARAMETER;
    *pAtom = atom;
    return STATUS_SUCCESS;
}

/******************************************************************
 *		integral_atom_name (internal)
 *
 * Helper for fetching integral (local/global) atoms names.
 */
static ULONG integral_atom_name(WCHAR* buffer, ULONG len, RTL_ATOM atom)
{
    static const WCHAR fmt[] = {'#','%','u',0};
    WCHAR tmp[16];
    int ret;

    ret = swprintf( tmp, fmt, atom );
    if (!len) return ret * sizeof(WCHAR);
    if (len <= ret) ret = len - 1;
    memcpy( buffer, tmp, ret * sizeof(WCHAR) );
    buffer[ret] = 0;
    return ret * sizeof(WCHAR);
}

/*************************************************
 *        Global handle table management
 *************************************************/

/******************************************************************
 *		NtAddAtom (NTDLL.@)
 */
NTSTATUS WINAPI WineNtAddAtom( const WCHAR* name, ULONG length, RTL_ATOM* atom )
{
    NTSTATUS    status;

    status = is_integral_atom( name, length / sizeof(WCHAR), atom );
    if (status == STATUS_MORE_ENTRIES)
    {
        SERVER_START_REQ( add_atom )
        {
            wine_server_add_data( req, name, length );
            req->table = 0;
            status = wine_server_call( req );
            *atom = reply->atom;
        }
        SERVER_END_REQ;
    }
    //TRACE( "%s -> %x\n",
    //       debugstr_wn(name, length/sizeof(WCHAR)), status == STATUS_SUCCESS ? *atom : 0 );
    return status;
}

/******************************************************************
 *		NtDeleteAtom (NTDLL.@)
 */
NTSTATUS WINAPI WineNtDeleteAtom(RTL_ATOM atom)
{
    NTSTATUS    status;

    SERVER_START_REQ( delete_atom )
    {
        req->atom = atom;
        req->table = 0;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************
 *		NtFindAtom (NTDLL.@)
 */
NTSTATUS WINAPI WineNtFindAtom( const WCHAR* name, ULONG length, RTL_ATOM* atom )
{
    NTSTATUS    status;

    status = is_integral_atom( name, length / sizeof(WCHAR), atom );
    if (status == STATUS_MORE_ENTRIES)
    {
        SERVER_START_REQ( find_atom )
        {
            wine_server_add_data( req, name, length );
            req->table = 0;
            status = wine_server_call( req );
            *atom = reply->atom;
        }
        SERVER_END_REQ;
    }
    //TRACE( "%s -> %x\n",
    //       debugstr_wn(name, length/sizeof(WCHAR)), status == STATUS_SUCCESS ? *atom : 0 );
    return status;
}

/******************************************************************
 *		NtQueryInformationAtom (NTDLL.@)
 */
NTSTATUS WINAPI WineNtQueryInformationAtom( RTL_ATOM atom, ATOM_INFORMATION_CLASS class,
                                        PVOID ptr, ULONG size, PULONG psize )
{
    NTSTATUS status;

    switch (class)
    {
    case AtomBasicInformation:
        {
            ULONG name_len;
            ATOM_BASIC_INFORMATION* abi = ptr;

            if (size < sizeof(ATOM_BASIC_INFORMATION))
                return STATUS_INVALID_PARAMETER;
            name_len = size - sizeof(ATOM_BASIC_INFORMATION);

            if (atom < MAXINTATOM)
            {
                if (atom)
                {
                    abi->NameLength = integral_atom_name( abi->Name, name_len, atom );
                    status = (name_len) ? STATUS_SUCCESS : STATUS_BUFFER_TOO_SMALL;
                    //abi->ReferenceCount = 1;
                    //abi->Pinned = 1;
                }
                else status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                SERVER_START_REQ( get_atom_information )
                {
                    req->atom = atom;
                    req->table = 0;
                    if (name_len) wine_server_set_reply( req, abi->Name, name_len );
                    status = wine_server_call( req );
                    if (status == STATUS_SUCCESS)
                    {
                        name_len = wine_server_reply_size( reply );
                        if (name_len)
                        {
                            abi->NameLength = name_len;
                            abi->Name[name_len / sizeof(WCHAR)] = '\0';
                        }
                        else
                        {
                            name_len = reply->total;
                            abi->NameLength = name_len;
                            status = STATUS_BUFFER_TOO_SMALL;
                        }
                        //abi->ReferenceCount = reply->count;
                        //abi->Pinned = reply->pinned;
                    }
                    else name_len = 0;
                }
                SERVER_END_REQ;
            }
            //TRACE( "%x -> %s (%u)\n", 
            //       atom, debugstr_wn(abi->Name, abi->NameLength / sizeof(WCHAR)),
            //       status );
            if (psize)
                *psize = sizeof(ATOM_BASIC_INFORMATION) + name_len;
        }
        break;
    default:
        DPRINT1( "Unsupported class %u\n", class );
        status = STATUS_INVALID_INFO_CLASS;
        break;
    }
    return status;
}

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

    if (Unicode)
        DPRINT("InternalAddAtom local %d name %S\n", Local, AtomName);
    else
        DPRINT("InternalAddAtom local %d name %s\n", Local, AtomName);

    /* Check if it's an integer atom */
    if ((ULONG_PTR)AtomName <= 0xFFFF)
    {
        /* Convert the name to an atom */
        Atom = (ATOM)PtrToShort((PVOID)AtomName);
        if (Atom >= MAXINTATOM)
        {
            /* Fail, atom number too large */
            SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
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
            SetLastErrorByStatus(Status);
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
        Status = WineNtAddAtom(AtomNameString->Buffer,
                           AtomNameString->Length,
                           &Atom);
    }

    /* Check for failure */
    if (!NT_SUCCESS(Status)) SetLastErrorByStatus(Status);

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

    if (Unicode)
        DPRINT("InternalFindAtom local %d name %S\n", Local, AtomName);
    else
        DPRINT("InternalFindAtom local %d name %s\n", Local, AtomName);

    /* Check if it's an integer atom */
    if ((ULONG_PTR)AtomName <= 0xFFFF)
    {
        /* Convert the name to an atom */
        Atom = (ATOM)PtrToShort((PVOID)AtomName);
        if (Atom >= MAXINTATOM)
        {
            /* Fail, atom number too large */
            SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
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
            SetLastErrorByStatus(Status);
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
            Status = WineNtFindAtom(AtomNameString->Buffer,
                                AtomNameString->Length,
                                &Atom);
        }
    }

    /* Check for failure */
    if (!NT_SUCCESS(Status)) SetLastErrorByStatus(Status);

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

    DPRINT("InternalDeleteAtom local %d atom %x\n", Local, Atom);

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
            Status = WineNtDeleteAtom(Atom);
        }

        /* Check for success */
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            SetLastErrorByStatus(Status);
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

    DPRINT("InternalGetAtomName local %d atom %x size %x\n", Local, Atom, Size);

    /* Normalize the size as not to overflow */
    if (!Unicode && Size > 0x7000) Size = 0x7000;

    /* Make sure it's valid too */
    if (!Size)
    {
        SetLastErrorByStatus(STATUS_BUFFER_OVERFLOW);
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
            SetLastErrorByStatus(STATUS_NO_MEMORY);
            return 0;
        }

        /* Query the name */
        Status = WineNtQueryInformationAtom(Atom,
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
        SetLastErrorByStatus(Status);
    }

    if (Unicode)
        DPRINT("InternalGetAtomName name %S\n", AtomName);
    else
        DPRINT("InternalGetAtomName name %s\n", AtomName);

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
