/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    version.c

Abstract:

    Implements calls to version APIs.

Author:

    Calin Negreanu (calinn)  20-Jan-1999

Revision History:

    <alias> <date> <comments>

--*/

#include "utils.h"
#include "version.h"

static PCTSTR g_DefaultTranslations[] = {
    TEXT("04090000"),
    TEXT("040904E4"),
    TEXT("040904B0"),
    NULL
};

BOOL
ShCreateVersionStruct (
    OUT     PVERSION_STRUCT VersionStruct,
    IN      PCTSTR FileSpec
    )

/*++

Routine Description:

  ShCreateVersionStruct is called to load a version structure from a file
  and to obtain the fixed version stamp info that is language-independent.

  The caller must call ShDestroyVersionStruct after the VersionStruct is no
  longer needed.

Arguments:

  VersionStruct - Receives the version stamp info to be used by other
                  functions in this module

  FileSpec - Specifies the file to obtain version info from

Return Value:

  TRUE if the routine was able to get version info, or FALSE if an
  error occurred.

--*/

{
    ZeroMemory (VersionStruct, sizeof (VERSION_STRUCT));
    VersionStruct->FileSpec = FileSpec;

    //
    // Allocate enough memory for the version stamp
    //

    VersionStruct->Size = GetFileVersionInfoSize (
                                (PTSTR) FileSpec,
                                &VersionStruct->Handle
                                );
    if (!VersionStruct->Size) {
        return FALSE;
    }

    VersionStruct->VersionBuffer = HeapAlloc (GetProcessHeap (), 0, VersionStruct->Size);

    if (!VersionStruct->VersionBuffer) {
        return FALSE;
    }

    VersionStruct->StringBuffer = HeapAlloc (GetProcessHeap (), 0, VersionStruct->Size);

    if (!VersionStruct->StringBuffer) {
        return FALSE;
    }


    //
    // Now get the version info from the file
    //

    if (!GetFileVersionInfo (
             (PTSTR) FileSpec,
             VersionStruct->Handle,
             VersionStruct->Size,
             VersionStruct->VersionBuffer
             )) {
        ShDestroyVersionStruct (VersionStruct);
        return FALSE;
    }

    //
    // Extract the fixed info
    //

    VerQueryValue (
        VersionStruct->VersionBuffer,
        TEXT("\\"),
        &VersionStruct->FixedInfo,
        &VersionStruct->FixedInfoSize
        );

    return TRUE;
}


VOID
ShDestroyVersionStruct (
    IN      PVERSION_STRUCT VersionStruct
    )

/*++

Routine Description:

  ShDestroyVersionStruct cleans up all memory allocated by the routines
  in this module.

Arguments:

  VersionStruct - Specifies the structure to clean up

Return Value:

  none

--*/

{
    if (VersionStruct->VersionBuffer) {
        HeapFree (GetProcessHeap (), 0, VersionStruct->VersionBuffer);
    }
    if (VersionStruct->StringBuffer) {
        HeapFree (GetProcessHeap (), 0, VersionStruct->StringBuffer);
    }

    ZeroMemory (VersionStruct, sizeof (VERSION_STRUCT));
}


ULONGLONG
ShVerGetFileVer (
    IN      PVERSION_STRUCT VersionStruct
    )
{
    ULONGLONG result = 0;
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        *((PDWORD) (&result)) = VersionStruct->FixedInfo->dwFileVersionLS;
        *(((PDWORD) (&result)) + 1) = VersionStruct->FixedInfo->dwFileVersionMS;
    }
    return result;
}

ULONGLONG
ShVerGetProductVer (
    IN      PVERSION_STRUCT VersionStruct
    )
{
    ULONGLONG result = 0;
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        *((PDWORD) (&result)) = VersionStruct->FixedInfo->dwProductVersionLS;
        *(((PDWORD) (&result)) + 1) = VersionStruct->FixedInfo->dwProductVersionMS;
    }
    return result;
}

DWORD
ShVerGetFileDateLo (
    IN      PVERSION_STRUCT VersionStruct
    )
{
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VersionStruct->FixedInfo->dwFileDateLS;
    }
    return 0;
}

DWORD
ShVerGetFileDateHi (
    IN      PVERSION_STRUCT VersionStruct
    )
{
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VersionStruct->FixedInfo->dwFileDateMS;
    }
    return 0;
}

DWORD
ShVerGetFileVerOs (
    IN      PVERSION_STRUCT VersionStruct
    )
{
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VersionStruct->FixedInfo->dwFileOS;
    }
    return 0;
}

DWORD
ShVerGetFileVerType (
    IN      PVERSION_STRUCT VersionStruct
    )
{
    if (VersionStruct->FixedInfoSize >= sizeof (VS_FIXEDFILEINFO)) {
        return VersionStruct->FixedInfo->dwFileType;
    }
    return 0;
}

PCTSTR
pShEnumVersionValueCommon (
    IN OUT  PVERSION_STRUCT VersionStruct
    );

PCTSTR
pShEnumNextVersionTranslation (
    IN OUT  PVERSION_STRUCT VersionStruct
    );

PCTSTR
pShEnumFirstVersionTranslation (
    IN OUT  PVERSION_STRUCT VersionStruct
    )
{
    UINT ArraySize;

    if (!VerQueryValue (
            VersionStruct->VersionBuffer,
            TEXT("\\VarFileInfo\\Translation"),
            &VersionStruct->Translations,
            &ArraySize
            )) {
        //
        // No translations are available
        //

        ArraySize = 0;
    }

    //
    // Return a pointer to the first translation
    //

    VersionStruct->CurrentDefaultTranslation = 0;
    VersionStruct->MaxTranslations = ArraySize / sizeof (TRANSLATION);
    VersionStruct->CurrentTranslation = 0;

    return pShEnumNextVersionTranslation (VersionStruct);
}

BOOL
pShIsDefaultTranslation (
    IN      PCTSTR TranslationStr
    )
{
    INT i;

    for (i = 0 ; g_DefaultTranslations[i] ; i++) {
        if (lstrcmpi (TranslationStr, g_DefaultTranslations[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

PCTSTR
pShEnumNextVersionTranslation (
    IN OUT  PVERSION_STRUCT VersionStruct
    )
{
    PTRANSLATION Translation;

    if (g_DefaultTranslations[VersionStruct->CurrentDefaultTranslation]) {

        lstrcpy (VersionStruct->TranslationStr, g_DefaultTranslations[VersionStruct->CurrentDefaultTranslation]);
        VersionStruct->CurrentDefaultTranslation++;

    } else {

        do {
            if (VersionStruct->CurrentTranslation == VersionStruct->MaxTranslations) {
                return NULL;
            }

            Translation = &VersionStruct->Translations[VersionStruct->CurrentTranslation];

            wsprintf (
                VersionStruct->TranslationStr,
                TEXT("%04x%04x"),
                Translation->CodePage,
                Translation->Language
                );

            VersionStruct->CurrentTranslation++;

        } while (pShIsDefaultTranslation (VersionStruct->TranslationStr));
    }

    return VersionStruct->TranslationStr;
}


PCTSTR
pShEnumNextVersionValue (
    IN OUT  PVERSION_STRUCT VersionStruct
    )
{
    PCTSTR rc = NULL;

    do {
        if (!pShEnumNextVersionTranslation (VersionStruct)) {
            break;
        }

        rc = pShEnumVersionValueCommon (VersionStruct);

    } while (!rc);

    return rc;
}

PCTSTR
pShEnumFirstVersionValue (
    IN OUT  PVERSION_STRUCT VersionStruct,
    IN      PCTSTR VersionField
    )
{
    PCTSTR rc;

    if (!pShEnumFirstVersionTranslation (VersionStruct)) {
        return NULL;
    }

    VersionStruct->VersionField = VersionField;

    rc = pShEnumVersionValueCommon (VersionStruct);

    if (!rc) {
        rc = pShEnumNextVersionValue (VersionStruct);
    }

    return rc;
}

PCTSTR
pShEnumVersionValueCommon (
    IN OUT  PVERSION_STRUCT VersionStruct
    )
{
    PTSTR Text;
    UINT StringLen;
    PBYTE String;
    PCTSTR Result = NULL;

    //
    // Prepare sub block for VerQueryValue API
    //
    Text = HeapAlloc (GetProcessHeap (), 0, (18 + lstrlen (VersionStruct->TranslationStr) + lstrlen (VersionStruct->VersionField)) * sizeof (TCHAR));

    if (!Text) {
        return NULL;
    }

    wsprintf (
        Text,
        TEXT("\\StringFileInfo\\%s\\%s"),
        VersionStruct->TranslationStr,
        VersionStruct->VersionField
        );

    __try {
        //
        // Get the value from the version stamp
        //

        if (!VerQueryValue (
                VersionStruct->VersionBuffer,
                Text,
                &String,
                &StringLen
                )) {
            //
            // No value is available
            //

            return NULL;
        }
        CopyMemory (VersionStruct->StringBuffer, String, StringLen * sizeof (TCHAR));
        VersionStruct->StringBuffer [StringLen * sizeof (TCHAR)] = 0;
        Result = (PTSTR) VersionStruct->StringBuffer;

    }
    __finally {
        HeapFree (GetProcessHeap (), 0, Text);
    }

    return Result;
}

BOOL
ShGlobalVersionCheck (
    IN      PVERSION_STRUCT VersionData,
    IN      PCTSTR NameToCheck,
    IN      PCTSTR ValueToCheck
    )
{
    PCTSTR CurrentStr;
    BOOL result = FALSE;

    CurrentStr = pShEnumFirstVersionValue (VersionData, NameToCheck);
    while (CurrentStr) {
        if (ShIsPatternMatch (ValueToCheck, CurrentStr)) {
            result = TRUE;
            break;
        }
        CurrentStr = pShEnumNextVersionValue (VersionData);
    }
    return result;
}

