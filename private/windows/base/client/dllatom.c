/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    atom.c

Abstract:

    This module contains the Win32 Atom Management APIs

Author:

    Steve Wood (stevewo) 24-Sep-1990

Revision History:

--*/

#include "basedll.h"

typedef ATOM *PATOM;

BOOL
InternalGetIntAtom(
    PUNICODE_STRING UnicodeAtomName,
    PATOM Atom
    );

ATOM
InternalAddAtom(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    LPCSTR AtomName
    );

ATOM
InternalFindAtom(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    LPCSTR AtomName
    );

ATOM
InternalDeleteAtom(
    BOOLEAN UseLocalAtomTable,
    ATOM nAtom
    );

UINT
InternalGetAtomName(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    ATOM nAtom,
    LPSTR AtomName,
    DWORD nSize
    );


ATOM
GlobalAddAtomA(
    LPCSTR lpString
    )
{
    return( InternalAddAtom( FALSE, FALSE, lpString ) );
}

ATOM
GlobalFindAtomA(
    LPCSTR lpString
    )
{
    return( InternalFindAtom( FALSE, FALSE, lpString) );
}

ATOM
GlobalDeleteAtom(
    ATOM nAtom
    )
{
    return( InternalDeleteAtom( FALSE, nAtom ) );
}

UINT
GlobalGetAtomNameA(
    ATOM nAtom,
    LPSTR lpBuffer,
    int nSize
    )
{
    return( InternalGetAtomName( FALSE, FALSE, nAtom, lpBuffer, (DWORD)nSize ) );
}

ATOM
APIENTRY
GlobalAddAtomW(
    LPCWSTR lpString
    )
{
    return( InternalAddAtom( FALSE, TRUE, (LPSTR)lpString ) );
}

ATOM
APIENTRY
GlobalFindAtomW(
    LPCWSTR lpString
    )
{
    return( InternalFindAtom( FALSE, TRUE, (LPSTR)lpString) );
}

UINT
APIENTRY
GlobalGetAtomNameW(
    ATOM nAtom,
    LPWSTR lpBuffer,
    int nSize
    )
{
    return( InternalGetAtomName( FALSE, TRUE, nAtom, (LPSTR)lpBuffer, (DWORD)nSize ) );
}

PVOID BaseLocalAtomTable;

BOOL
APIENTRY
InitAtomTable(
    DWORD nSize
    )
{
    if (nSize < 4 || nSize > 511) {
        nSize = 37;
        }

    return RtlCreateAtomTable( nSize, &BaseLocalAtomTable ) == STATUS_SUCCESS;
}

ATOM
AddAtomA(
    LPCSTR lpString
    )
{
    return( InternalAddAtom( TRUE, FALSE, lpString ) );
}

ATOM
FindAtomA(
    LPCSTR lpString
    )
{
    return( InternalFindAtom( TRUE, FALSE, lpString ) );
}

ATOM
DeleteAtom(
    ATOM nAtom
    )
{
    return( InternalDeleteAtom( TRUE, nAtom ) );
}

UINT
GetAtomNameA(
    ATOM nAtom,
    LPSTR lpBuffer,
    int nSize
    )
{
    return( InternalGetAtomName( TRUE, FALSE, nAtom, lpBuffer, (DWORD)nSize ) );
}

ATOM
APIENTRY
AddAtomW(
    LPCWSTR lpString
    )
{
    return( InternalAddAtom( TRUE, TRUE, (LPSTR)lpString ) );
}

ATOM
APIENTRY
FindAtomW(
    LPCWSTR lpString
    )
{
    return( InternalFindAtom( TRUE, TRUE, (LPSTR)lpString ) );
}

UINT
APIENTRY
GetAtomNameW(
    ATOM nAtom,
    LPWSTR lpBuffer,
    int nSize
    )
{
    return( InternalGetAtomName( TRUE, TRUE, nAtom, (LPSTR)lpBuffer, (DWORD)nSize ) );
}

PVOID
InternalInitAtomTable( void )
{
    NTSTATUS Status;

    if (BaseLocalAtomTable == NULL) {
        Status = RtlCreateAtomTable( 0, &BaseLocalAtomTable );
        }

    return BaseLocalAtomTable;
}

ATOM
InternalAddAtom(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    LPCSTR AtomName
    )
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING UnicodeAtomName;
    ATOM Atom;

    if ( (ULONG_PTR)AtomName <= 0xFFFF ) {
        Atom = (ATOM)PtrToShort((PVOID)AtomName);
        if (Atom >= MAXINTATOM) {
            BaseSetLastNTError( STATUS_INVALID_PARAMETER );
            return( INVALID_ATOM );
            }
        else {
            return( (ATOM)Atom );
            }
        }
    else {
        try {
            if (IsUnicodeAtomName) {
                UnicodeAtomName = &UnicodeString;
                RtlInitUnicodeString( UnicodeAtomName, (PWSTR)AtomName );
                Status = STATUS_SUCCESS;
                }
            else {
                RtlInitAnsiString( &AnsiString, AtomName );
                if (AnsiString.MaximumLength > STATIC_UNICODE_BUFFER_LENGTH) {
                    UnicodeAtomName = &UnicodeString;
                    Status = RtlAnsiStringToUnicodeString( UnicodeAtomName, &AnsiString, TRUE );
                    }
                else {
                    UnicodeAtomName = &NtCurrentTeb()->StaticUnicodeString;
                    Status = RtlAnsiStringToUnicodeString( UnicodeAtomName, &AnsiString, FALSE );
                    }
                }
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            }

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError( Status );
            return( INVALID_ATOM );
            }
        }

    Atom = INVALID_ATOM;
    try {
        if (UseLocalAtomTable) {
            Status = RtlAddAtomToAtomTable( InternalInitAtomTable(),
                                            UnicodeAtomName->Buffer,
                                            &Atom
                                          );
            }
        else {
            Status = NtAddAtom( UnicodeAtomName->Buffer,
                                UnicodeAtomName->Length,
                                &Atom
                              );
            }

        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            Atom = INVALID_ATOM;
            }
        }
    finally {
        if (!IsUnicodeAtomName && UnicodeAtomName == &UnicodeString) {
            RtlFreeUnicodeString( UnicodeAtomName );
            }
        }

    return( (ATOM)Atom );
}

ATOM
InternalFindAtom(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    LPCSTR AtomName
    )
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PUNICODE_STRING UnicodeAtomName;
    ATOM Atom;

    if ( (ULONG_PTR)AtomName <= 0xFFFF ) {
        Atom = (ATOM)PtrToShort((PVOID)AtomName);
        if (Atom >= MAXINTATOM) {
            BaseSetLastNTError( STATUS_INVALID_PARAMETER );
            return( INVALID_ATOM );
            }
        else {
            return( (ATOM)Atom );
            }
        }
    else {
        try {
            if (IsUnicodeAtomName) {
                UnicodeAtomName = &UnicodeString;
                RtlInitUnicodeString( UnicodeAtomName, (PWSTR)AtomName );
                Status = STATUS_SUCCESS;
                }
            else {
                RtlInitAnsiString( &AnsiString, AtomName );
                if (AnsiString.MaximumLength > STATIC_UNICODE_BUFFER_LENGTH) {
                    UnicodeAtomName = &UnicodeString;
                    Status = RtlAnsiStringToUnicodeString( UnicodeAtomName, &AnsiString, TRUE );
                    }
                else {
                    UnicodeAtomName = &NtCurrentTeb()->StaticUnicodeString;
                    Status = RtlAnsiStringToUnicodeString( UnicodeAtomName, &AnsiString, FALSE );
                    }
                }
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            }

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError( Status );
            return( INVALID_ATOM );
            }
        }

    Atom =  INVALID_ATOM;
    try {
        if (UseLocalAtomTable) {
            Status = RtlLookupAtomInAtomTable( InternalInitAtomTable(),
                                               UnicodeAtomName->Buffer,
                                               &Atom
                                             );
            }
        else {
            if (UnicodeAtomName->Length == 0) {
                SetLastError( ERROR_INVALID_NAME );
                leave;
                }

            Status = NtFindAtom( UnicodeAtomName->Buffer,
                                 UnicodeAtomName->Length,
                                 &Atom
                               );
            }
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            Atom =  INVALID_ATOM;
            leave;
            }
        }
    finally {
        if (!IsUnicodeAtomName && UnicodeAtomName == &UnicodeString) {
            RtlFreeUnicodeString( UnicodeAtomName );
            }
        }


    return( (ATOM)Atom );
}

ATOM
InternalDeleteAtom(
    BOOLEAN UseLocalAtomTable,
    ATOM nAtom
    )
{
    NTSTATUS Status;

    if (nAtom >= MAXINTATOM) {
        if (UseLocalAtomTable) {
            Status = RtlDeleteAtomFromAtomTable( InternalInitAtomTable(), nAtom );
            }
        else {
            Status = NtDeleteAtom( nAtom );
            }

        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            return( INVALID_ATOM );
            }
        }

    return( 0 );
}


UINT
InternalGetAtomName(
    BOOLEAN UseLocalAtomTable,
    BOOLEAN IsUnicodeAtomName,
    ATOM nAtom,
    LPSTR AtomName,
    DWORD nSize
    )
{
    NTSTATUS Status;
    PVOID FreeBuffer = NULL;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PWSTR UnicodeAtomName;
    ULONG AtomInfoLength, AtomNameLength;
    DWORD ReturnValue;
    PATOM_BASIC_INFORMATION AtomInfo;

    //
    // Trim nSize so that it will not overflow the 16-bit unicode string
    // maximum length field. This prevents idiots that call us with a >=32KB
    // query buffer from stubbing their toes when they call the Ansi version
    // of the GetAtomName API
    //

    if (!IsUnicodeAtomName && nSize > 0x7000) {
        nSize = 0x7000;
        }

    if (nSize == 0) {
        BaseSetLastNTError( STATUS_BUFFER_OVERFLOW );
        return( 0 );
        }

    if (UseLocalAtomTable) {
        if (IsUnicodeAtomName) {
            UnicodeAtomName = (PWSTR)AtomName;
            }
        else {
            FreeBuffer = RtlAllocateHeap( RtlProcessHeap(),
                                          MAKE_TAG( TMP_TAG ),
                                          nSize * sizeof( WCHAR )
                                        );
            if (FreeBuffer == NULL) {
                BaseSetLastNTError( STATUS_NO_MEMORY );
                return( 0 );
                }

            UnicodeAtomName = (PWSTR)FreeBuffer;
            }

        AtomNameLength = nSize * sizeof( WCHAR );
        Status = RtlQueryAtomInAtomTable( InternalInitAtomTable(),
                                          nAtom,
                                          NULL,
                                          NULL,
                                          UnicodeAtomName,
                                          &AtomNameLength
                                        );
        }
    else {
        AtomInfoLength = sizeof( *AtomInfo ) + (nSize * sizeof( WCHAR ));
        FreeBuffer = RtlAllocateHeap( RtlProcessHeap(),
                                      MAKE_TAG( TMP_TAG ),
                                      AtomInfoLength
                                    );
        if (FreeBuffer == NULL) {
            BaseSetLastNTError( STATUS_NO_MEMORY );
            return( 0 );
            }
        AtomInfo = (PATOM_BASIC_INFORMATION)FreeBuffer;

        Status = NtQueryInformationAtom( nAtom,
                                         AtomBasicInformation,
                                         AtomInfo,
                                         AtomInfoLength,
                                         &AtomInfoLength
                                       );
        if (NT_SUCCESS( Status )) {
            AtomNameLength = (ULONG)AtomInfo->NameLength;
            UnicodeAtomName = AtomInfo->Name;
            }
        }

    if (NT_SUCCESS( Status )) {
        if (IsUnicodeAtomName) {
            ReturnValue = AtomNameLength / sizeof( WCHAR );
            if (UnicodeAtomName != (PWSTR)AtomName) {
                RtlMoveMemory( AtomName, UnicodeAtomName, AtomNameLength );
                }
            if (ReturnValue < nSize) {
                *((PWSTR)AtomName + ReturnValue) = UNICODE_NULL;
                }
            }
        else {
            UnicodeString.Buffer = UnicodeAtomName;
            UnicodeString.Length = (USHORT)AtomNameLength;
            UnicodeString.MaximumLength = (USHORT)(UnicodeString.Length + sizeof( UNICODE_NULL ));
            AnsiString.Buffer = AtomName;
            AnsiString.Length = 0;
            AnsiString.MaximumLength = (USHORT)nSize;
            Status = RtlUnicodeStringToAnsiString( &AnsiString, &UnicodeString, FALSE );
            if (NT_SUCCESS( Status )) {
                ReturnValue = AnsiString.Length;
                }
            }
        }

    if (FreeBuffer != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, FreeBuffer );
        }

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( 0 );
        }
    else {
        return( ReturnValue );
        }
}
