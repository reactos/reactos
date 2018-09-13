/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    proc.h

Abstract:

    This file contains global procedure declarations for the HTTP API
    project.

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

--*/


#ifndef _PROC_H_
#define _PROC_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define SECURITY_WIN32
#include <sspi.h>
#include <issperr.h>

DWORD
pHttpGetUrlLen(
    IN INTERNET_SCHEME SchemeType,
    IN LPSTR lpszTargetName,
    IN LPSTR lpszObjectName,
    IN DWORD dwPort,
    OUT LPDWORD lpdwUrlLen
    );

DWORD
pHttpGetUrlString(
    IN INTERNET_SCHEME SchemeType,
    IN LPSTR lpszTargetName,
    IN LPSTR lpszCWD,
    IN LPSTR lpszObjectName,
    IN LPSTR lpszExtension,
    IN DWORD dwPort,
    OUT LPSTR * lplpUrlName,
    OUT LPDWORD lpdwUrlLen
    );

DWORD
pHttpBuildUrl(
    IN INTERNET_SCHEME SchemeType,
    IN LPSTR lpszTargetName,
    IN LPSTR lpszObjectName,
    IN DWORD dwPort,
    IN LPSTR lpszUrl,
    IN OUT LPDWORD lpdwBuffSize
    );

BOOL FParseHttpDate(
    FILETIME *lpFt,
    LPCSTR lpcszDateStr
    );

BOOL FFileTimetoHttpDateTime(
    FILETIME *lpft,       // output filetime in GMT
    LPSTR   lpszBuff,
    LPDWORD lpdwSize
    );

#if defined(__cplusplus)
}
#endif

#endif  // _PROC_H_
