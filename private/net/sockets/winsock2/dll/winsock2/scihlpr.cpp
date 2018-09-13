/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    scihlpr.cpp

Abstract:

    This module contains the implementation of the helper functions for
    translating WSASERVICECLASSINFO structs from the ansi to unicode and uncode
    to ansi

Author:

    Dirk Brandewie dirk@mink.intel.com  12-1-1995

[Environment:]

[Notes:]

    $Revision:   1.0  $

    $Modtime:   29 Jan 1996 08:58:54  $

Revision History:

    25-Jan-1996 dirk@mink.intel.com
        Initial Revision

--*/

#include "precomp.h"


DWORD
CalculateBufferSize(
    BOOL IsAnsi,
    LPVOID Buffer
    )
{
    LPWSASERVICECLASSINFOW UnicodeBuffer;
    LPWSASERVICECLASSINFOA AnsiBuffer;
    LPWSTR                 Wstring=NULL;
    LPSTR                  Astring=NULL;
    INT                    StringLen=0;
    DWORD                  Index;
    DWORD                  Count;

    DWORD Size=0;

    if (IsAnsi){
        AnsiBuffer = (LPWSASERVICECLASSINFOA) Buffer;
    } //if
    else{
        UnicodeBuffer =(LPWSASERVICECLASSINFOW) Buffer;
    } //else

    //Size of the fixed portion of the buffer
    if (IsAnsi){
        Size += sizeof(WSASERVICECLASSINFO);
        Size += (sizeof(WSANSCLASSINFO) * AnsiBuffer->dwCount);
    } //if
    else{
        Size += sizeof(WSASERVICECLASSINFO);
        Size += (sizeof(WSANSCLASSINFO) * UnicodeBuffer->dwCount);
    } //else

    //The toplevel GUID
    Size += sizeof(GUID);
    //The GUID's in the NSCLLASSINFO's
    if (IsAnsi){
        Size += (sizeof(GUID) * AnsiBuffer->dwCount);
    } //if
    else{
        Size += (sizeof(GUID) * UnicodeBuffer->dwCount);
    } //else

    //The toplevle string
    if (IsAnsi){
        StringLen =0;
        StringLen = MultiByteToWideChar(
            CP_ACP,                      // CodePage (Ansi)
            0,                           // dwFlags
            AnsiBuffer->lpszServiceClassName,  // lpMultiByteStr
            -1,                          // cchMultiByte
            NULL,                     // lpWideCharStr
            StringLen);                  // cchWideChar
        Size += ((StringLen+1) * sizeof(WCHAR));
    } //if
    else{
        StringLen =0;
        StringLen = WideCharToMultiByte(
            CP_ACP,                        // CodePage (Ansi)
            0,                             // dwFlags
            UnicodeBuffer->lpszServiceClassName, // lpWideCharStr
            -1,                            // cchWideChar
            Astring,                       // lpMultiByteStr
            StringLen,                     // cchMultiByte
            NULL,                          // lpDefaultChar
            NULL);                         // lpUsedDefaultChar
        Size += (StringLen+1);
    } //else


    if (IsAnsi){
        Count = AnsiBuffer->dwCount;
    } //if
    else{
        Count = UnicodeBuffer->dwCount;
    } //else

    // The variable parts of NSCLASSINFO
    for (Index = 0; Index < Count ;Index++ ){
        if (IsAnsi){
            StringLen = 0;
            StringLen = MultiByteToWideChar(
                CP_ACP,                      // CodePage (Ansi)
                0,                           // dwFlags
                AnsiBuffer->lpClassInfos[Index].lpszName, // lpMultiByteStr
                -1,                          // cchMultiByte
                NULL,                     // lpWideCharStr
                StringLen);                  // cchWideChar
            Size += ((StringLen+1) * sizeof(WCHAR));
        } //if
        else{
            StringLen = 0;
            StringLen = WideCharToMultiByte(
                CP_ACP,                        // CodePage (Ansi)
                0,                             // dwFlags
                UnicodeBuffer->lpClassInfos[Index].lpszName,  // lpWideCharStr
                -1,                            // cchWideChar
                Astring,                       // lpMultiByteStr
                StringLen,                     // cchMultiByte
                NULL,                          // lpDefaultChar
                NULL);                         // lpUsedDefaultChar
            Size += (StringLen+1);
        } //else

        if (IsAnsi){
            Size += AnsiBuffer->lpClassInfos[Index].dwValueSize;
        } //if
        else{
            Size += UnicodeBuffer->lpClassInfos[Index].dwValueSize;
        } //else
    } //for
    return(Size);
}


INT
MapAnsiServiceClassInfoToUnicode(
    IN     LPWSASERVICECLASSINFOA Source,
    IN OUT LPDWORD                lpTargetSize,
    IN     LPWSASERVICECLASSINFOW Target
    )
{
    DWORD RequiredBufferSize;
    LPBYTE FreeSpace;
    INT    StringLen;
    DWORD  Index;

    __try {
        // Find the size of buffer we will need
        RequiredBufferSize = CalculateBufferSize(
            TRUE, // Ansi Source
            Source);
        if (RequiredBufferSize > *lpTargetSize){
            *lpTargetSize = RequiredBufferSize;
            return(WSAEFAULT);
        } //if

        // Copy known size portions of buffer

        // Toplevel structure
        FreeSpace = (LPBYTE)Target;
        CopyMemory(Target,
                   Source,
                   sizeof(WSASERVICECLASSINFOW));
        FreeSpace += sizeof(WSASERVICECLASSINFOW);

        // The array of WSANSCLASSINFO's
        Target->lpClassInfos = (LPWSANSCLASSINFOW)FreeSpace;
        CopyMemory(Target->lpClassInfos,
                   Source->lpClassInfos,
                   (sizeof(WSANSCLASSINFO) * Source->dwCount));
        FreeSpace += (sizeof(WSANSCLASSINFO) * Source->dwCount);

        // The service class ID GUID
        Target->lpServiceClassId = (LPGUID)FreeSpace;
        CopyMemory(Target->lpServiceClassId,
                   Source->lpServiceClassId,
                   sizeof(GUID));
        FreeSpace += sizeof(GUID);

        // Copy variable portion

        Target->lpszServiceClassName = (LPWSTR)FreeSpace;
        StringLen = MultiByteToWideChar(
            CP_ACP,                        // CodePage (Ansi)
            0,                             // dwFlags
            Source->lpszServiceClassName,  // lpMultiByteStr
            -1,                            // cchMultiByte
            NULL,                          // lpWideCharStr
            0);                            // cchWideChar

        FreeSpace += ((StringLen+1) * sizeof(WCHAR));
        MultiByteToWideChar(
            CP_ACP,                        // CodePage (Ansi)
            0,                             // dwFlags
            Source->lpszServiceClassName,  // lpMultiByteStr
            -1,                            // cchMultiByte
            Target->lpszServiceClassName,  // lpWideCharStr
            StringLen);  // cchWideChar

        for (Index=0;Index < Source->dwCount ;Index++ ){
            LPSTR SourceString;
            LPWSTR TargetString;

            SourceString = Source->lpClassInfos[Index].lpszName;
            Target->lpClassInfos[Index].lpszName = (LPWSTR)FreeSpace;
            TargetString = Target->lpClassInfos[Index].lpszName;

            StringLen = MultiByteToWideChar(
                CP_ACP,                        // CodePage (Ansi)
                0,                             // dwFlags
                SourceString,                  // lpMultiByteStr
                -1,                            // cchMultiByte
                NULL,                          // lpWideCharStr
                0);                            // cchWideChar

            FreeSpace += ((StringLen +1) * sizeof(WCHAR));
            MultiByteToWideChar(
                CP_ACP,                        // CodePage (Ansi)
                0,                             // dwFlags
                SourceString,                  // lpMultiByteStr
                -1,                            // cchMultiByte
                TargetString,                  // lpWideCharStr
                StringLen);                    // cchWideChar

            Target->lpClassInfos[Index].lpValue = FreeSpace;
            CopyMemory(
                Target->lpClassInfos[Index].lpValue,
                Source->lpClassInfos[Index].lpValue,
                Source->lpClassInfos[Index].dwValueSize);
            FreeSpace += Source->lpClassInfos[Index].dwValueSize;
        } //for
        return(ERROR_SUCCESS);
    }
    __except (WS2_EXCEPTION_FILTER()) {
        return (WSAEFAULT);
    }
}


INT
MapUnicodeServiceClassInfoToAnsi(
    IN     LPWSASERVICECLASSINFOW Source,
    IN OUT LPDWORD                lpTargetSize,
    IN     LPWSASERVICECLASSINFOA Target
    )
{
    DWORD RequiredBufferSize;
    LPBYTE FreeSpace;
    INT    StringLen;
    DWORD  Index;

    __try {
        // Find the size of buffer we will need
        RequiredBufferSize = CalculateBufferSize(
            TRUE, // Ansi Source
            Source);
        if (RequiredBufferSize > *lpTargetSize){
            *lpTargetSize = RequiredBufferSize;
            return(WSAEFAULT);
        } //if

        // Copy known size portions of buffer

        // Toplevel structure
        FreeSpace = (LPBYTE)Target;
        CopyMemory(Target,
                   Source,
                   sizeof(WSASERVICECLASSINFOA));
        FreeSpace += sizeof(WSASERVICECLASSINFOA);

        // The array of WSANSCLASSINFO's
        Target->lpClassInfos = (LPWSANSCLASSINFOA)FreeSpace;
        CopyMemory(Target->lpClassInfos,
                   Source->lpClassInfos,
                   (sizeof(WSANSCLASSINFOA) * Source->dwCount));
        FreeSpace += (sizeof(WSANSCLASSINFOA) * Source->dwCount);

        // The service class ID GUID
        Target->lpServiceClassId = (LPGUID)FreeSpace;
        CopyMemory(Target->lpServiceClassId,
                   Source->lpServiceClassId,
                   sizeof(GUID));
        FreeSpace += sizeof(GUID);

        // Copy variable portion

        Target->lpszServiceClassName = (LPSTR)FreeSpace;
        StringLen = WideCharToMultiByte(
            CP_ACP,                          // CodePage (Ansi)
            0,                               // dwFlags
            Source->lpszServiceClassName,    // lpWideCharStr
            -1,                              // cchWideChar
            NULL         ,                   // lpMultiByteStr
            0,                               // cchMultiByte
            NULL,                            // lpDefaultChar
            NULL);                           // lpUsedDefaultChar

        FreeSpace += (StringLen+1);
        WideCharToMultiByte(
            CP_ACP,                          // CodePage (Ansi)
            0,                               // dwFlags
            Source->lpszServiceClassName,    // lpWideCharStr
            -1,                              // cchWideChar
            Target->lpszServiceClassName,    // lpMultiByteStr
            StringLen,                       // cchMultiByte
            NULL,                            // lpDefaultChar
            NULL);                           // lpUsedDefaultChar

        for (Index=0;Index < Source->dwCount ;Index++ ){
            LPWSTR SourceString;
            LPSTR TargetString;

            SourceString = Source->lpClassInfos[Index].lpszName;
            Target->lpClassInfos[Index].lpszName = (LPSTR)FreeSpace;
            TargetString = Target->lpClassInfos[Index].lpszName;

            StringLen = WideCharToMultiByte(
                CP_ACP,                          // CodePage (Ansi)
                0,                               // dwFlags
                SourceString,                    // lpWideCharStr
                -1,                              // cchWideChar
                NULL         ,                   // lpMultiByteStr
                0,                               // cchMultiByte
                NULL,                            // lpDefaultChar
                NULL);                           // lpUsedDefaultChar

            FreeSpace += (StringLen+1);
            WideCharToMultiByte(
                CP_ACP,                          // CodePage (Ansi)
                0,                               // dwFlags
                SourceString,                    // lpWideCharStr
                -1,                              // cchWideChar
                TargetString,                    // lpMultiByteStr
                StringLen,                       // cchMultiByte
                NULL,                            // lpDefaultChar
                NULL);                           // lpUsedDefaultChar

            Target->lpClassInfos[Index].lpValue = FreeSpace;
            CopyMemory(
                Target->lpClassInfos[Index].lpValue,
                Source->lpClassInfos[Index].lpValue,
                Source->lpClassInfos[Index].dwValueSize);
            FreeSpace += Source->lpClassInfos[Index].dwValueSize;
        } //for
        return(ERROR_SUCCESS);
    }
    __except (WS2_EXCEPTION_FILTER()) {
        return (WSAEFAULT);
    }
}




