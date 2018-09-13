/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    proto.h

Abstract:

    Contains proto type definitions of several functions.

Author:

    Madan Appiah (madana) 15-Nov-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PROTO_
#define _PROTO_

extern "C"
{
    VOID CacheScavenger(LPVOID Parameter);
}

LONGLONG GetGmtTime(VOID);

DWORD GetFileSizeAndTimeByName(
    LPCTSTR FileName,
    WIN32_FILE_ATTRIBUTE_DATA *lpFileAttrData
    );

DWORD
GetFileSizeByName(
    LPCTSTR pszFileName,
    DWORD *pdwFileSize
    );

BOOL InitGlobals (void);

void LaunchScavenger (void);

DWORD
CreateUniqueFile(
    LPCSTR UrlName,
    LPTSTR Path,
    LPTSTR FileName,
    LPTSTR Extension,
    HANDLE *phfHandle
    );

#endif  // _PROTO_

