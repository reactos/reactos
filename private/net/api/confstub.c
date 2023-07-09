/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    ConfStub.c

Abstract:

    This module contains stubs for the NetConfig APIs.

Author:

    John Rogers (JohnRo) 23-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    23-Oct-1991 JohnRo
        Created.
    28-Oct-1991 JohnRo
        Use <winerror.h> if <lmerr.h> isn't needed.
    20-Nov-1991 JohnRo
        Work with old or new lmconfig.h for now (based on REVISED_CONFIG_APIS).
    02-Dec-1991 JohnRo
        Implement local NetConfig APIs.
    11-Mar-1992 JohnRo
        Fixed bug in get all where array wasn't terminated.
        Added real NetConfigSet() handling.
    21-Oct-1992 JohnRo
        RAID 9357: server mgr: can't add to alerts list on downlevel.

--*/

// These must be included first:

#include <nt.h>                 // IN, etc.  (Only needed by temporary config.h)
#include <ntrtl.h>              // (Only needed by temporary config.h)
#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // DEVLEN, NET_API_STATUS, etc.
#include <netdebug.h>           // (Needed by config.h)

// These may be included in any order:

#include <config.h>             // NetpOpenConfigData(), etc.
#include <lmapibuf.h>           // NetApiBufferFree().
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <lmconfig.h>           // NetConfig APIs.
#include <netlib.h>             // NetpMemoryReallocate().
#include <rxconfig.h>           // RxNetConfig APIs.
#include <tstring.h>            // STRSIZE(), TCHAR_EOS, etc.


#define INITIAL_ALLOC_AMOUNT    512  // arbitrary
#define INCR_ALLOC_AMOUNT       512  // arbitrary


NET_API_STATUS NET_API_FUNCTION
NetConfigGet (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR Component,
    IN LPCWSTR Parameter,
#ifdef REVISED_CONFIG_APIS
    OUT LPBYTE *BufPtr
#else
    OUT LPBYTE *BufPtr,
    OUT LPDWORD TotalAvailable
#endif
    )

{
    NET_API_STATUS Status;
    LPNET_CONFIG_HANDLE ConfigHandle;
    BOOL TryDownLevel;

#ifndef REVISED_CONFIG_APIS
    UNREFERENCED_PARAMETER(TotalAvailable);
#endif

    *BufPtr = NULL;  // Check caller and make error handling easier.

    Status = NetpOpenConfigData(
            & ConfigHandle,
            (LPWSTR)UncServerName,
            (LPWSTR)Component,
            TRUE);              // just want read-only access

    if (Status != NERR_Success) {

        Status = NetpHandleConfigFailure(
                "NetConfigGet",  // debug name
                Status,          // result of NetpOpenConfigData
                (LPWSTR)UncServerName,
                & TryDownLevel);

        if (TryDownLevel) {
            return (RxNetConfigGet(
                    (LPWSTR)UncServerName,
                    (LPWSTR)Component,
                    (LPWSTR)Parameter,
                    BufPtr));
        } else {
            return (Status);    // result of NetpHandleConfigFailure
        }
    }

    Status = NetpGetConfigValue(
            ConfigHandle,
            (LPWSTR)Parameter,          // keyword
            (LPTSTR *) (LPVOID) BufPtr);     // alloc and set ptr

    if (Status == NERR_Success) {
        Status = NetpCloseConfigData( ConfigHandle );
        NetpAssert(Status == NERR_Success);
    } else {
        NetpAssert(*BufPtr == NULL);
        (void) NetpCloseConfigData( ConfigHandle );
    }

    return (Status);

} // NetConfigGet



NET_API_STATUS NET_API_FUNCTION
NetConfigGetAll (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR Component,
#ifdef REVISED_CONFIG_APIS
    OUT LPBYTE *BufPtr
#else
    OUT LPBYTE *BufPtr,
    OUT LPDWORD TotalAvailable
#endif
    )

{
    DWORD BufSize;                      // Bytes allocated at *BufPtr (so far).
    DWORD BufUsed;                      // Bytes used      at *BufPtr (so far).
    LPNET_CONFIG_HANDLE ConfigHandle;
    BOOL FirstTime;
    LPVOID NewBuffPtr;
    NET_API_STATUS Status;
    BOOL TryDownLevel;

#ifndef REVISED_CONFIG_APIS
    UNREFERENCED_PARAMETER(TotalAvailable);
#endif

    *BufPtr = NULL;  // Check caller and make error handling easier.

    Status = NetpOpenConfigData(
            & ConfigHandle,
            (LPWSTR)UncServerName,
            (LPWSTR)Component,
            TRUE);                      // just want read-only access

    if (Status != NERR_Success) {

        Status = NetpHandleConfigFailure(
                "NetConfigGetAll",      // debug name
                Status,                 // result of NetpOpenConfigData
                (LPWSTR)UncServerName,
                & TryDownLevel);

        if (TryDownLevel) {
            return (RxNetConfigGetAll(
                    (LPWSTR)UncServerName,
                    (LPWSTR)Component,
                    BufPtr));

        } else {
            return (Status);            // result of NetpHandleConfigFailure
        }
    }

    // Even if there aren't any entries, we'll need to store a null at
    // end of array.  So allocate initial one now.
    BufSize = INITIAL_ALLOC_AMOUNT;
    NewBuffPtr = NetpMemoryReallocate(
            (LPVOID) *BufPtr,           // old address
            BufSize);                   // new size
    if (NewBuffPtr == NULL) { // out of memory
        (void) NetpCloseConfigData( ConfigHandle );
        return (ERROR_NOT_ENOUGH_MEMORY);
    }
    *BufPtr = NewBuffPtr;
    BufUsed = 0;

    // Loop once per entry (at least once if no entries).
    FirstTime = TRUE;
    do {
        LPTSTR KeywordBuffer;
        LPTSTR ValueBuffer;

        Status = NetpEnumConfigSectionValues(
                ConfigHandle,
                & KeywordBuffer,        // Alloc and set ptr.
                & ValueBuffer,          // Alloc and set ptr.
                FirstTime);

        FirstTime = FALSE;

        if (Status == NERR_Success) {

            DWORD SrcSize =
                    (STRLEN(KeywordBuffer) + 1 + STRLEN(ValueBuffer) + 1)
                    * sizeof(TCHAR);
            if (BufSize < (BufUsed+SrcSize) ) {
                if (SrcSize <= INCR_ALLOC_AMOUNT) {
                    BufSize += INCR_ALLOC_AMOUNT;
                } else {
                    BufSize += SrcSize;
                }
                NewBuffPtr = NetpMemoryReallocate(
                    (LPVOID) *BufPtr, /* old address */
                    BufSize);  /* new size */
                if (NewBuffPtr == NULL) { /* out of memory */
                    (void) NetpCloseConfigData( ConfigHandle );
                    return (ERROR_NOT_ENOUGH_MEMORY);
                }
                *BufPtr = NewBuffPtr;
            }

#define AddString( lptstrSrc, CharCount ) \
            { \
                LPTSTR lptstrDest; \
                NetpAssert( CharCount > 0 ); \
                lptstrDest = (LPTSTR)NetpPointerPlusSomeBytes( *BufPtr, BufUsed); \
                NetpAssert( lptstrDest != NULL ); \
                (void) STRNCPY( lptstrDest, lptstrSrc, CharCount ); \
                BufUsed += (CharCount * sizeof(TCHAR) ); \
                NetpAssert( BufUsed <= BufSize ); \
            }

            AddString( KeywordBuffer, STRLEN(KeywordBuffer) );
            (void) NetApiBufferFree( KeywordBuffer );

            AddString( TEXT("="), 1 );

            AddString( ValueBuffer, STRLEN(ValueBuffer) );
            (void) NetApiBufferFree( ValueBuffer );

#define AddNullChar( ) \
    { \
        AddString( TEXT(""), 1 ); \
    }

            AddNullChar();              // Terminate this entry.

        }

    } while (Status == NERR_Success);

    if (Status == NERR_CfgParamNotFound) {
        AddNullChar();                  // Terminate the array.
        Status = NetpCloseConfigData( ConfigHandle );
        NetpAssert(Status == NERR_Success);
    } else {
        NetpAssert( Status != NO_ERROR );
        NetpAssert( *BufPtr != NULL );
        NetpMemoryFree( *BufPtr );
        *BufPtr = NULL;
        (void) NetpCloseConfigData( ConfigHandle );
    }

    return (Status);

} // NetConfigGetAll



NET_API_STATUS NET_API_FUNCTION
NetConfigSet (
    IN LPCWSTR UncServerName OPTIONAL,
    IN LPCWSTR Reserved1 OPTIONAL,
    IN LPCWSTR Component,
    IN DWORD Level,
    IN DWORD Reserved2,
    IN LPBYTE Buf,
    IN DWORD Reserved3
    )
{
    LPCONFIG_INFO_0 Info = (LPVOID) Buf;
    LPNET_CONFIG_HANDLE ConfigHandle;
    NET_API_STATUS Status;
    BOOL TryDownLevel;

    if (Buf == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if (Level != 0) {
        return (ERROR_INVALID_LEVEL);
    } else if (Info->cfgi0_key == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if (Info->cfgi0_data == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if (Reserved1 != NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if (Reserved2 != 0) {
        return (ERROR_INVALID_PARAMETER);
    } else if (Reserved3 != 0) {
        return (ERROR_INVALID_PARAMETER);
    }

    Status = NetpOpenConfigData(
            & ConfigHandle,
            (LPWSTR)UncServerName,
            (LPWSTR)Component,
            FALSE);             // don't want _read-only _access

    if (Status != NERR_Success) {

        Status = NetpHandleConfigFailure(
                "NetConfigSet",  // debug name
                Status,          // result of NetpOpenConfigData
                (LPWSTR)UncServerName,
                & TryDownLevel);

        if (TryDownLevel) {
            return (RxNetConfigSet(
                    (LPWSTR)UncServerName,
                    (LPWSTR)Reserved1,
                    (LPWSTR)Component,
                    Level,
                    Reserved2,
                    Buf,
                    Reserved3));
        } else {
            return (Status);    // result of NetpHandleConfigFailure
        }
    }

    Status = NetpSetConfigValue(
            ConfigHandle,
            Info->cfgi0_key,    // keyword
            Info->cfgi0_data);  // new value

    if (Status == NERR_Success) {
        Status = NetpCloseConfigData( ConfigHandle );
        NetpAssert(Status == NERR_Success);
    } else {
        (void) NetpCloseConfigData( ConfigHandle );
    }

    return (Status);

} // NetConfigSet
