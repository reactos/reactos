/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    version.h

Abstract:

    Declares the structures used for version checkings.

Author:

    Calin Negreanu (calinn) 01/20/1999

Revision History:

--*/

#pragma once

#include <windows.h>
#include <winnt.h>

#define MAX_TRANSLATION             32

typedef struct {
    WORD CodePage;
    WORD Language;
} TRANSLATION, *PTRANSLATION;

typedef struct {
    PBYTE VersionBuffer;
    PTRANSLATION Translations;
    PBYTE StringBuffer;
    UINT Size;
    DWORD Handle;
    VS_FIXEDFILEINFO *FixedInfo;
    UINT FixedInfoSize;
    TCHAR TranslationStr[MAX_TRANSLATION];
    UINT MaxTranslations;
    UINT CurrentTranslation;
    UINT CurrentDefaultTranslation;
    PCTSTR FileSpec;
    PCTSTR VersionField;
} VERSION_STRUCT, *PVERSION_STRUCT;

BOOL
ShCreateVersionStruct (
    OUT     PVERSION_STRUCT VersionStruct,
    IN      PCTSTR FileSpec
    );

VOID
ShDestroyVersionStruct (
    IN      PVERSION_STRUCT VersionStruct
    );

ULONGLONG
ShVerGetFileVer (
    IN      PVERSION_STRUCT VersionStruct
    );

ULONGLONG
ShVerGetProductVer (
    IN      PVERSION_STRUCT VersionStruct
    );

DWORD
ShVerGetFileDateLo (
    IN      PVERSION_STRUCT VersionStruct
    );

DWORD
ShVerGetFileDateHi (
    IN      PVERSION_STRUCT VersionStruct
    );

DWORD
ShVerGetFileVerOs (
    IN      PVERSION_STRUCT VersionStruct
    );

DWORD
ShVerGetFileVerType (
    IN      PVERSION_STRUCT VersionStruct
    );

BOOL
ShGlobalVersionCheck (
    IN      PVERSION_STRUCT VersionData,
    IN      PCTSTR NameToCheck,
    IN      PCTSTR ValueToCheck
    );
