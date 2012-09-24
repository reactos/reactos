/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/atom.c
 * PURPOSE:         Atom functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/


#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/ntndk.h>
#include "wine/debug.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(useratom);

#define SetLastErrorByStatus(__S__) \
 ((void)SetLastError(RtlNtStatusToDosError(__S__)))

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
 * Helper for fetching integral atoms names.
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
        ERR( "Unsupported class %u\n", class );
        status = STATUS_INVALID_INFO_CLASS;
        break;
    }
    return status;
}

/* FUNCTIONS *****************************************************************/

ATOM
WINAPI
InternalAddAtom(BOOLEAN Unicode,
                LPCSTR AtomName)
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING AtomNameString;
    ATOM Atom = INVALID_ATOM;

    if (Unicode)
        WARN("InternalAddAtom name %S\n", AtomName);
    else
        WARN("InternalAddAtom name %s\n", AtomName);

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

    /* Do a global add */
    Status = WineNtAddAtom(AtomNameString->Buffer,
                       AtomNameString->Length,
                       &Atom);

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
InternalFindAtom(BOOLEAN Unicode,LPCSTR AtomName)
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING AtomNameString;
    ATOM Atom = INVALID_ATOM;

    if (Unicode)
        WARN("InternalFindAtom name %S\n", AtomName);
    else
        WARN("InternalFindAtom name %s\n", AtomName);

    /* Check if it's an integer atom */
    if ((ULONG_PTR)AtomName <= 0xFFFF)
    {
        /* Convert the name to an atom */
        Atom = (ATOM)PtrToShort((PVOID)AtomName);
        if (Atom >= MAXINTATOM)
        {
            /* Fail, atom number too large */
            SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
            ERR("Invalid atom\n");
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
            ERR("Failed\n");
            SetLastErrorByStatus(Status);
            return Atom;
        }
    }

    /* Do a global search */
    if (!AtomNameString->Length)
    {
        /* This is illegal in win32 */
        ERR("No name given\n");
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    else
    {
        /* Call the global function */
        Status = WineNtFindAtom(AtomNameString->Buffer,
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
InternalDeleteAtom(ATOM Atom)
{
    NTSTATUS Status;

    WARN("InternalDeleteAtom atom %x\n", Atom);

    /* Validate it */
    if (Atom >= MAXINTATOM)
    {
        /* Delete it globall */
        Status = WineNtDeleteAtom(Atom);

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
InternalGetAtomName(BOOLEAN Unicode,
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

    WARN("InternalGetAtomName atom %x size %x\n", Atom, Size);

    /* Normalize the size as not to overflow */
    if (!Unicode && Size > 0x7000) Size = 0x7000;

    /* Make sure it's valid too */
    if (!Size)
    {
        SetLastErrorByStatus(STATUS_BUFFER_OVERFLOW);
        return 0;
    }
    if (!Atom)
    {
        SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
        return 0;
    }

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
        WARN("Failed: %lx\n", Status);
        SetLastErrorByStatus(Status);
    }

    if (Unicode)
        WARN("InternalGetAtomName name %S\n", AtomName);
    else
        WARN("InternalGetAtomName name %s\n", AtomName);

    /* Return length */
    return RetVal;
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
ATOM
UserAddAtomA(LPCSTR lpString)
{
    return InternalAddAtom(FALSE, lpString);
}

/*
 * @implemented
 */
ATOM
UserAddAtomW(LPCWSTR lpString)
{
    return InternalAddAtom(TRUE, (LPSTR)lpString);
}

/*
 * @implemented
 */
ATOM
UserDeleteAtom(ATOM nAtom)
{
    return InternalDeleteAtom(nAtom);
}

/*
 * @implemented
 */
ATOM
UserFindAtomA(LPCSTR lpString)
{
    return InternalFindAtom(FALSE, lpString);
}

/*
 * @implemented
 */
ATOM
UserFindAtomW(LPCWSTR lpString)
{
    return InternalFindAtom(TRUE, (LPSTR)lpString);
}

/*
 * @implemented
 */
UINT
UserGetAtomNameA(ATOM nAtom,
                   LPSTR lpBuffer,
                   int nSize)
{
    return InternalGetAtomName(FALSE, nAtom, lpBuffer, (DWORD)nSize);
}

/*
 * @implemented
 */
UINT
UserGetAtomNameW(ATOM nAtom,
                   LPWSTR lpBuffer,
                   int nSize)
{
    return InternalGetAtomName(TRUE,
                               nAtom,
                               (LPSTR)lpBuffer,
                               (DWORD)nSize);
}


/* EOF */
