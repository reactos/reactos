/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ErrStub.c

Abstract:

    This module contains stubs for the NetErrorLog APIs.

Author:

    John Rogers (JohnRo) 11-Nov-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    11-Nov-1991 JohnRo
        Implement downlevel NetErrorLog APIs.

--*/


// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // DEVLEN, NET_API_STATUS, etc.
#include <lmerrlog.h>           // NetErrorLog APIs; needed by rxerrlog.h.

// These may be included in any order:

#include <rxerrlog.h>           // RxNetErrorLog APIs.
#include <winerror.h>           // ERROR_ equates.


NET_API_STATUS NET_API_FUNCTION
NetErrorLogClear (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR BackupFile OPTIONAL,
    IN LPBYTE  Reserved OPTIONAL
    )

{
    if ( (UncServerName == NULL) || (*UncServerName == '\0') ) {
        return (ERROR_NOT_SUPPORTED);
    }

    return (RxNetErrorLogClear(
            (LPWSTR)UncServerName,
            (LPWSTR)BackupFile,
            Reserved));

} // NetErrorLogClear



NET_API_STATUS NET_API_FUNCTION
NetErrorLogRead (
    IN LPCWSTR   UncServerName OPTIONAL,
    IN LPWSTR    Reserved1 OPTIONAL,
    IN LPHLOG    ErrorLogHandle,
    IN DWORD     Offset,
    IN LPDWORD   Reserved2 OPTIONAL,
    IN DWORD     Reserved3,
    IN DWORD     OffsetFlag,
    OUT LPBYTE * BufPtr,
    IN DWORD     PrefMaxSize,
    OUT LPDWORD  BytesRead,
    OUT LPDWORD  TotalAvailable
    )
{
    if ( (UncServerName == NULL) || (*UncServerName == '\0') ) {
        return (ERROR_NOT_SUPPORTED);
    }

    return (RxNetErrorLogRead(
            (LPWSTR)UncServerName,
            Reserved1,
            ErrorLogHandle,
            Offset,
            Reserved2,
            Reserved3,
            OffsetFlag,
            BufPtr,
            PrefMaxSize,
            BytesRead,
            TotalAvailable));

} // NetErrorLogRead


NET_API_STATUS NET_API_FUNCTION
NetErrorLogWrite (
    IN LPBYTE  Reserved1 OPTIONAL,
    IN DWORD   Code,
    IN LPCWSTR Component,
    IN LPBYTE  Buffer,
    IN DWORD   NumBytes,
    IN LPBYTE  MsgBuf,
    IN DWORD   StrCount,
    IN LPBYTE  Reserved2 OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Code);
    UNREFERENCED_PARAMETER(Component);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(NumBytes);
    UNREFERENCED_PARAMETER(MsgBuf);
    UNREFERENCED_PARAMETER(StrCount);
    UNREFERENCED_PARAMETER(Reserved2);

    return (ERROR_NOT_SUPPORTED);

} // NetErrorLogWrite
