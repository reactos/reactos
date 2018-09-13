/******************************Module*Header*******************************\
* Module Name: fntsweep.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* This file contains font sweeper related stuff.
* On the boot of ths system, i.e.  initialization of userk, the
* [Fonts] section of win.ini is checked to
* find out if any new fonts have been added by any font installers.
* If third party installers have installed fonts in the system directory
* those are copied to fonts directory. Any fot entries are replaced
* by appropriate *.ttf entries, any fot files are deleted if they were
* ever installed.
*
\**************************************************************************/


#include "precomp.h"
#pragma hdrstop
#include <setupbat.h>      // in sdkinc

CONST WCHAR pwszType1Key[]      = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Type 1 Installer\\Type 1 Fonts";
CONST WCHAR pwszSweepType1Key[] = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Type 1 Installer\\LastType1Sweep";
CONST WCHAR pwszUpdType1Key[]   = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Type 1 Installer\\Upgraded Type1";

CONST WCHAR pwszFontsKey[] = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
CONST WCHAR pwszSweepKey[] = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\LastFontSweep";
CONST WCHAR pwszFontDrivers[] = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Font Drivers";

#define LAST_SWEEP_TIME L"LastSweepTime"
#define UPGRADED_TYPE1 L"UpgradedType1"

#define DWORDALIGN(X) (((X) + 3) & ~3)

WCHAR *gpwcSystemDir;
WCHAR *gpwcFontsDir;
BOOL   gbWin31Upgrade;


BOOL bCheckIfDualBootingWithWin31()
{
    WCHAR Buffer[32];
    WCHAR awcWindowsDir[MAX_PATH];
    DWORD dwRet;
    UINT  cwchWinPath = GetSystemWindowsDirectoryW(awcWindowsDir, MAX_PATH);

// the cwchWinPath value does not include the terminating zero

    if (awcWindowsDir[cwchWinPath - 1] == L'\\')
    {
        cwchWinPath -= 1;
    }
    awcWindowsDir[cwchWinPath] = L'\0'; // make sure to zero terminated

    lstrcatW(awcWindowsDir, L"\\system32\\");
    lstrcatW(awcWindowsDir, WINNT_GUI_FILE_W);

    dwRet = GetPrivateProfileStringW(
                WINNT_DATA_W,
                WINNT_D_WIN31UPGRADE_W,
                WINNT_A_NO_W,
                Buffer,
                sizeof(Buffer)/sizeof(WCHAR),
                awcWindowsDir
                );

    #if DBGSWEEP
    DbgPrint("\n dwRet = %ld, win31upgrade = %ws\n\n", dwRet, Buffer);
    #endif

    return (BOOL)(dwRet ? (!lstrcmpiW(Buffer,WINNT_A_YES)) : 0);
}


/******************************Public*Routine******************************\
*
* VOID vNullTermWideString (WCHAR *pwcDest, WCHAR *pwcSrc, ULONG ulLength)
*
* Given pwcSrc, which is not necessarily null-terminated, copy ulLength characters
* the into pwcDest and place a null character after it.
*
* History:
*  03-Feb-99 -by- Donald Chinn [dchinn]
* Wrote it.
\**************************************************************************/
VOID vNullTermWideString (WCHAR *pwcDest, WCHAR *pwcSrc, ULONG ulLength)
{
    ULONG index;

    for (index = 0; index < ulLength; index++) {
        *pwcDest++ = *pwcSrc++;
    }
    *pwcDest = '\0';
}


/******************************Public*Routine******************************\
*
* BOOL bCheckFontEntry(WCHAR *pwcName, WCHAR *pwcExtension)
*
* This function assumes that both pwcName and pwcExtension are null-terminated.
*
* History:
*  25-Oct-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL bCheckFontEntry(WCHAR *pwcName, WCHAR *pwcExtension)
{
    BOOL bRet = FALSE;
    LONG cwc = (LONG)wcslen(pwcName) - (LONG)wcslen(pwcExtension);
    if (cwc > 0)
    {
        bRet = !_wcsicmp(&pwcName[cwc], pwcExtension);
    }
    return bRet;

}



/******************************Public*Routine******************************\
*   Process win.ini line
*
* History:
*  24-Oct-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#define EXT_TRUETYPE  L"(TrueType)"
#define EXT_FOT       L".FOT"


VOID vProcessFontEntry(
    HKEY   hkey,
    WCHAR *pwcValueName,
    ULONG ulValueNameLength,
    WCHAR *pwcFileName,
    ULONG ulFileNameLength
)
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    BOOL  bFot = FALSE;
    WCHAR awcTTF[MAX_PATH];
    WCHAR awcTmpBuf[MAX_PATH];
    WCHAR *pwcTTF;
    FLONG fl, fl2;
    FLONG flEmbed;
    DWORD dwPidTid;
    WCHAR awcValueName[MAX_PATH];  // null-terminated pwcValueName
    WCHAR awcFileName[MAX_PATH];   // null-terminated pwcFileName
    

    // Make sure the ValueName is null-terminated
    ulValueNameLength = min(MAX_PATH - 1, ulValueNameLength);
    vNullTermWideString (awcValueName, pwcValueName, ulValueNameLength);
    
    // Make sure the FileName is null-terminated
    ulFileNameLength = min(MAX_PATH - 1, ulFileNameLength);
    vNullTermWideString (awcFileName, pwcFileName, ulFileNameLength);
    
    if (bCheckFontEntry(awcValueName, EXT_TRUETYPE))
    {
    // This is a tt entry, either .fot or .ttf

        if (bFot = bCheckFontEntry(awcFileName, EXT_FOT))
        {
        // this is an .fot entry, must find ttf pointed to by .fot,
        // but first must get the full path to the .fot file
        // for cGetTTFFromFOT routine expects it. We will also need
        // the full path to the .fot file so that we can delete it
        // eventually.

            if (bMakePathNameW(awcTmpBuf, awcFileName, NULL, &fl2))
            {
                if (cGetTTFFromFOT(awcTmpBuf, MAX_PATH, awcTTF, &fl, &flEmbed, &dwPidTid, TRUE) &&
                    !(fl & FONT_ISNOT_FOT))
                {
                // fix the entry to point to .ttf file. At this point
                // awcTTF points to the FULL path to the .ttf file.
                // However, we will only need a relative path to the
                // .ttf file, when the .ttf file is in the %windir%\system
                // or %windir%\fonts directories. In case the file is in the
                // %windir%\system directory we shall copy it to %windir%\fonts
                // directory and write the relative path to the registry.
                // In case it is in the %windir%\fonts directory we do not
                // touch the file and also just write the relative path to the
                // registry. In any other case we just write the full .ttf
                // path to the registry.

                // first delete the .fot file, it is no longer needed

                    if (bFot && !gbWin31Upgrade)
                    {
                        UserVerify(DeleteFileW(awcTmpBuf));
                    }

                    if ((fl & (FONT_IN_FONTS_DIR | FONT_IN_SYSTEM_DIR)) == 0)
                    {
                    // if ttf file is not in either the system or the fonts
                    // directories, just write the full path to the registry

                        pwcTTF = awcTTF;
                    }
                    else
                    {
                    // find the bare file part, this is what will be written
                    // in the registry

                        pwcTTF = &awcTTF[wcslen(awcTTF) - 1];
                        while ((pwcTTF >= awcTTF) && (*pwcTTF != L'\\') && (*pwcTTF != L':'))
                            pwcTTF--;
                        pwcTTF++;

                        if (fl & FONT_IN_SYSTEM_DIR)
                        {
                        // need to move the ttf to fonts dir, can reuse the
                        // buffer on the stack:

                            wcscpy(awcTmpBuf, gpwcFontsDir);
                            lstrcatW(awcTmpBuf, L"\\");
                            lstrcatW(awcTmpBuf, pwcTTF);

                        // note that MoveFile should succeed, for if there was
                        // a ttf file of the same file name in %windir%\fonts dir
                        // we would not have been in this code path.

                                RIPMSG2(RIP_VERBOSE, "Moving %ws to %ws", awcTTF, awcTmpBuf);
                                if (!gbWin31Upgrade)
                                {
                                    UserVerify(MoveFileW(awcTTF, awcTmpBuf));
                                }
                                else
                                {
                                // Boolean value TRUE means "do not copy if target exists"

                                    UserVerify(CopyFileW(awcTTF, awcTmpBuf, TRUE));
                                }
                        }
                    }

                    RIPMSG2(RIP_VERBOSE, "writing to the registry:\n    %ws=%ws", pwcValueName, pwcTTF);
                    RtlInitUnicodeString(&UnicodeString, awcValueName);
                    Status = NtSetValueKey(hkey,
                                           &UnicodeString,
                                           0,
                                           REG_SZ,
                                           pwcTTF,
                                           (wcslen(pwcTTF)+1) * sizeof(WCHAR));
                    UserAssert(NT_SUCCESS(Status));
                }
                #if DBG
                else
                {
                    RIPMSG1(RIP_WARNING, "Could not locate ttf pointed to by %ws", awcTmpBuf);
                }
                #endif
            }
            #if DBG
            else
            {
                RIPMSG1(RIP_WARNING, "Could not locate .fot:  %ws", awcFileName);
            }
            #endif
        }
    }
    else
    {
    // not a true type case. little bit simpler,
    // we will use awcTTF buffer for the full path name, and pwcTTF
    // as local variable even though these TTF names are misnomer
    // for these are not tt fonts

        if (bMakePathNameW(awcTTF, awcFileName,NULL, &fl))
        {
        // At this point
        // awcTTF points to the FULL path to the font file.

        // If the font is in the system subdirectory we will just move it
        // to the fonts subdirectory. If the path in the registry is relative
        // we will leave it alone. If it is an absolute path, we shall
        // fix the registry entry to only contain relative path, the
        // absolute path is redundant.

            if (fl & (FONT_IN_SYSTEM_DIR | FONT_IN_FONTS_DIR))
            {
            // find the bare file part, this is what will be written
            // in the registry

                pwcTTF = &awcTTF[wcslen(awcTTF) - 1];
                while ((pwcTTF >= awcTTF) && (*pwcTTF != L'\\') && (*pwcTTF != L':'))
                    pwcTTF--;
                pwcTTF++;

                if (fl & FONT_IN_SYSTEM_DIR)
                {
                // need to move the font to fonts dir, can reuse the
                // buffer on the stack to build the full destination path

                    wcscpy(awcTmpBuf, gpwcFontsDir);
                    lstrcatW(awcTmpBuf, L"\\");
                    lstrcatW(awcTmpBuf, pwcTTF);

                // note that MoveFile should succeed, for if there was
                // a font file of the same file name in %windir%\fonts dir
                // we would not have been in this code path. The only time
                // it could fail if the path in the registry is absolute.

                    RIPMSG2(RIP_VERBOSE, "Moving %ws to %ws", awcTTF, awcTmpBuf);
                    if (!gbWin31Upgrade)
                    {
                        UserVerify(MoveFileW(awcTTF, awcTmpBuf));
                    }
                    else
                    {
                    // Boolean value TRUE means "do not copy if target exists"

                        UserVerify(CopyFileW(awcTTF, awcTmpBuf, TRUE));
                    }
                }

            // check if the file path in the registry is absolute,
            // if so make it relative:

                if (!(fl & FONT_RELATIVE_PATH))
                {
                    RIPMSG2(RIP_VERBOSE, "writing to the registry:\n    %ws=%ws", pwcValueName, pwcTTF);
                    RtlInitUnicodeString(&UnicodeString, awcValueName);
                    Status = NtSetValueKey(hkey,
                                           &UnicodeString,
                                           0,
                                           REG_SZ,
                                           pwcTTF,
                                           (wcslen(pwcTTF)+1) * sizeof(WCHAR));
                    UserAssert(NT_SUCCESS(Status));
                }
            }
        }
    }
}


/******************************Public*Routine******************************\
*
* VOID vMoveFileFromSystemToFontsDir(WCHAR *pwcFile)
*
* History:
*  24-Apr-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




VOID vMoveFileFromSystemToFontsDir(WCHAR *pwcFile)
{
    WCHAR awcTmpBuf[MAX_PATH];
    WCHAR awcTmp[MAX_PATH];
    FLONG fl;
    WCHAR *pwcTmp;

#if DBG
    BOOL  bOk;
#endif

    if (bMakePathNameW(awcTmp, pwcFile,NULL, &fl))
    {
    // If the font is in the system subdirectory we will just move it
    // to the fonts subdirectory. The path in the registry is relative
    // and we will leave it alone.

        if
        (
            (fl & (FONT_IN_SYSTEM_DIR | FONT_RELATIVE_PATH)) ==
            (FONT_IN_SYSTEM_DIR | FONT_RELATIVE_PATH)
        )
        {
        // find the bare file part, this is what will be written
        // in the registry

            pwcTmp = &awcTmp[wcslen(awcTmp) - 1];
            while ((pwcTmp >= awcTmp) && (*pwcTmp != L'\\') && (*pwcTmp != L':'))
                pwcTmp--;

            if (pwcTmp > awcTmp)
                pwcTmp++;

        // need to move the font to fonts dir, can reuse the
        // buffer on the stack to build the full destination path

            wcscpy(awcTmpBuf, gpwcFontsDir);
            lstrcatW(awcTmpBuf, L"\\");
            lstrcatW(awcTmpBuf, pwcTmp);

        // note that MoveFile should succeed, for if there was
        // a font file of the same file name in %windir%\fonts dir
        // we would not have been in this code path.

            #if DBG
                bOk =
            #endif
                MoveFileW(awcTmp, awcTmpBuf);

            RIPMSG3(RIP_VERBOSE,
                    "move %ws to %ws %s",
                    awcTmp,
                    awcTmpBuf,
                    (bOk) ? "succeeded" : "failed");
        }
        #if DBG
        else
        {
            RIPMSG2(RIP_WARNING,
                    "File %ws not in system directory, fl = 0x%lx\n",
                    awcTmp, fl);
        }
        #endif

    }
    #if DBG
    else
    {
        RIPMSG1(RIP_WARNING, "Could not locate %ws", pwcFile);
    }
    #endif
}



/******************************Public*Routine******************************\
*
* VOID vProcessType1FontEntry
*
*
* Effects: All this routine does is to check if pwcPFM and pwcPFB pointed to
*          by pwcValueData point to files in the %windir%system directory
*          and if so copies these type 1 files to %windir%\fonts directory
*
* History:
*  20-Nov-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID vProcessType1FontEntry(
    HKEY   hkey,
    WCHAR *pwcValueName,
    ULONG ulValueNameLength,
    WCHAR *pwcValueData,
    ULONG ulValueDataLength
)
{
    WCHAR *pwcPFM, *pwcPFB;

    UNREFERENCED_PARAMETER(hkey);
    UNREFERENCED_PARAMETER(pwcValueName);
    UNREFERENCED_PARAMETER(ulValueNameLength);
    UNREFERENCED_PARAMETER(ulValueDataLength);

// skip unused boolean value in this multi reg_sz string:

    if ((pwcValueData[0] != L'\0') && (pwcValueData[1] == L'\0'))
    {
        pwcPFM = &pwcValueData[2];
        pwcPFB = pwcPFM + wcslen(pwcPFM) + 1; // add 1 for zero separator

        vMoveFileFromSystemToFontsDir(pwcPFM);
        vMoveFileFromSystemToFontsDir(pwcPFB);
    }
}


/******************************Public*Routine******************************\
*
* VOID vAddRemote/LocalType1Font
*
* History:
*  25-Apr-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vAddType1Font(
    WCHAR *pwcValueData,
    DWORD  dwFlags
)
{
    WCHAR *pwcPFM, *pwcPFB, *pwcMMM;

    #if DBG
    int iRet;
    #endif

// skip unused boolean value in this multi reg_sz string:

    if ((pwcValueData[0] != L'\0') && (pwcValueData[1] == L'\0'))
    {
        pwcPFM = &pwcValueData[2];
        pwcPFB = pwcPFM + wcslen(pwcPFM) + 1; // add 1 for zero separator
        pwcMMM = pwcPFB + wcslen(pwcPFB) + 1; // may of may not be there.

    // replace space by separator and call addfontresourcew

        pwcPFB[-1] = PATH_SEPARATOR;

    // if this is a multiple master font, need one more separator:

        if (pwcMMM[0] != L'\0')
            pwcMMM[-1] = PATH_SEPARATOR;

        #if DBG
            iRet =
        #endif

        GdiAddFontResourceW(pwcPFM, dwFlags, NULL);

        #if DBGSWEEP
            DbgPrint("%ld = GdiAddFontResourceW(%ws, 0x%lx);\n",
                iRet, pwcPFM, dwFlags);
        #endif
    }
}


VOID vAddRemoteType1Font(
    HKEY   hkey,
    WCHAR *pwcValueName,
    ULONG ulValueNameLength,
    WCHAR *pwcValueData,
    ULONG ulValueDataLength
)
{
    UNREFERENCED_PARAMETER(hkey);
    UNREFERENCED_PARAMETER(pwcValueName);
    UNREFERENCED_PARAMETER(ulValueNameLength);
    UNREFERENCED_PARAMETER(ulValueDataLength);
    vAddType1Font(pwcValueData, AFRW_ADD_REMOTE_FONT);
}

VOID vAddLocalType1Font(
    HKEY   hkey,
    WCHAR *pwcValueName,
    ULONG ulValueNameLength,
    WCHAR *pwcValueData,
    ULONG ulValueDataLength
)
{
    UNREFERENCED_PARAMETER(hkey);
    UNREFERENCED_PARAMETER(pwcValueName);
    UNREFERENCED_PARAMETER(ulValueNameLength);
    UNREFERENCED_PARAMETER(ulValueDataLength);
    vAddType1Font(pwcValueData, AFRW_ADD_LOCAL_FONT);
}


typedef  VOID (*PFNENTRY)(HKEY hkey, WCHAR *, ULONG, WCHAR *, ULONG);


/******************************Public*Routine******************************\
*
* VOID vFontSweep()
*
* This is the main routine in this module. Checks if the fonts need to be
* "sweeped" and does so if need be.
*
* History:
*  27-Oct-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vSweepFonts(
    PCWSTR   pwszFontListKey,       // font list key
    PCWSTR   pwszFontSweepKey,      // the corresponding sweep key
    PFNENTRY pfnProcessFontEntry,   // function that processes individual entry
    BOOL     bForceEnum             // force enumeration
    )
{
    DWORD      cjMaxValueName;
    DWORD      iFont;
    NTSTATUS   Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjA;
    KEY_FULL_INFORMATION KeyInfo;
    DWORD      dwReturnLength;

    PKEY_VALUE_FULL_INFORMATION KeyValueInfo;
    BYTE       *pjValueData;

    HKEY       hkey = NULL;
    struct {
        KEY_VALUE_PARTIAL_INFORMATION;
        LARGE_INTEGER;
    } SweepValueInfo;
    LARGE_INTEGER LastSweepTime;
    BOOL       bSweep = FALSE;

    HKEY       hkeyLastSweep;
    DWORD      cjData;

    if (!bForceEnum)
    {
    // first check if anything needs to be done, that is, if anybody
    // touched the [Fonts] section of the registry since the last time we sweeped it.
    // get the time of the last sweep of the fonts section of the registry:

        RtlInitUnicodeString(&UnicodeString, pwszFontSweepKey);
        InitializeObjectAttributes(&ObjA,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtOpenKey(&hkeyLastSweep,
                           KEY_READ | KEY_WRITE,
                           &ObjA);

        if (!NT_SUCCESS(Status))
        {
            DWORD  dwDisposition;

        // We are running for the first time, we need to create the key
        // for it does not exist as yet at this time

            bSweep = TRUE;

        // Create the key, open it for writing, since we will have to
        // store the time when the [Fonts] section of the registry was last swept

            Status = NtCreateKey(&hkeyLastSweep,
                                 KEY_WRITE,
                                 &ObjA,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 &dwDisposition);

            if (!NT_SUCCESS(Status))
                return;
        }
        else
        {
            RtlInitUnicodeString(&UnicodeString, LAST_SWEEP_TIME);
            Status = NtQueryValueKey(hkeyLastSweep,
                                     &UnicodeString,
                                     KeyValuePartialInformation,
                                     &SweepValueInfo,
                                     sizeof(SweepValueInfo),
                                     &dwReturnLength);
            if (!NT_SUCCESS(Status))
            {
                bSweep = TRUE; // force sweep, something is suspicious
            }
            else
            {
                UserAssert(SweepValueInfo.Type == REG_BINARY);
                UserAssert(SweepValueInfo.DataLength == sizeof(LastSweepTime));
                RtlCopyMemory(&LastSweepTime, &SweepValueInfo.Data, sizeof(LastSweepTime));
            }
        }
    }
    else
    {
        bSweep = TRUE;
    }

// now open the Fonts key and get the time the key last changed:
// now get the time of the time of the last change is bigger than
// the time of last sweep, must sweep again:

    RtlInitUnicodeString(&UnicodeString, pwszFontListKey);
    InitializeObjectAttributes(&ObjA,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&hkey,
                       KEY_READ | KEY_WRITE,
                       &ObjA);

    if (NT_SUCCESS(Status))
    {
    // get the number of entries in the [Fonts] section

        Status = NtQueryKey(hkey,
                    KeyFullInformation,
                    &KeyInfo,
                    sizeof(KeyInfo),
                    &dwReturnLength);

        if (NT_SUCCESS(Status) && KeyInfo.Values)
        {
            UserAssert(!(KeyInfo.ClassLength | KeyInfo.SubKeys | KeyInfo.MaxNameLen | KeyInfo.MaxClassLen));

        // now let us check if the fonts need to be sweeped. This is the case
        // when the registry last write time is bigger than the last sweep time

            if (!bSweep)
            {
                if (KeyInfo.LastWriteTime.QuadPart != LastSweepTime.QuadPart ) {
                    bSweep = TRUE;
                }
            }

        // init system dir, we will need it:

            if (bSweep &&
                bInitSystemAndFontsDirectoriesW(&gpwcSystemDir, &gpwcFontsDir))
            {
            // alloc buffer big enough to hold the biggest ValueName and ValueData

                cjMaxValueName = DWORDALIGN(KeyInfo.MaxValueNameLen + sizeof(WCHAR));

            // allocate temporary buffer into which we are going to suck the contents
            // of the registry

                KeyInfo.MaxValueDataLen = DWORDALIGN(KeyInfo.MaxValueDataLen);
                cjData = cjMaxValueName +    // space for the value name
                         KeyInfo.MaxValueDataLen ;    // space for the value data

                if (KeyValueInfo = UserLocalAlloc(0, sizeof(*KeyValueInfo) + cjData))
                {
                    for (iFont = 0; iFont < KeyInfo.Values; iFont++)
                    {
                        Status = NtEnumerateValueKey(
                                    hkey,
                                    iFont,
                                    KeyValueFullInformation,
                                    KeyValueInfo,
                                    sizeof(*KeyValueInfo) + cjData,
                                    &dwReturnLength);

                        if (NT_SUCCESS(Status))
                        {
                            UserAssert(KeyValueInfo->NameLength <= KeyInfo.MaxValueNameLen);
                            UserAssert(KeyValueInfo->DataLength <= KeyInfo.MaxValueDataLen);
                            UserAssert((KeyValueInfo->Type == REG_SZ) || (KeyValueInfo->Type == REG_MULTI_SZ));

                        // data goes into the second half of the buffer

                            pjValueData = (BYTE *)KeyValueInfo + KeyValueInfo->DataOffset;

                        // see if the font files are where the registry claims they are.
                        // It is unfortunate we have to do this because SearchPathW
                        // is slow because it touches the disk.

                            (*pfnProcessFontEntry)(hkey,
                                                   KeyValueInfo->Name,
                                                   KeyValueInfo->NameLength / sizeof(WCHAR),
                                                   (WCHAR *)pjValueData,
                                                   KeyValueInfo->DataLength / sizeof(WCHAR));
                        }
                    }

                    if (!bForceEnum)
                    {
                    // now that the sweep is completed, get the last write time
                    // and store it as the LastSweepTime at the appropriate location

                        Status = NtQueryKey(hkey,
                                    KeyFullInformation,
                                    &KeyInfo,
                                    sizeof(KeyInfo),
                                    &dwReturnLength);
                        UserAssert(NT_SUCCESS(Status));

                    // now remember the result

                        RtlInitUnicodeString(&UnicodeString, LAST_SWEEP_TIME);
                        Status = NtSetValueKey(hkeyLastSweep,
                                               &UnicodeString,
                                               0,
                                               REG_BINARY,
                                               &KeyInfo.LastWriteTime,
                                               sizeof(KeyInfo.LastWriteTime));
                        UserAssert(NT_SUCCESS(Status));
                    }

                // free the memory that will be no longer needed

                    UserLocalFree(KeyValueInfo);
                }
            }
        }
        NtClose(hkey);
    }

    if (!bForceEnum)
    {
        NtClose(hkeyLastSweep);
    }
}


/******************************Public*Routine******************************\
*
* BOOL bLoadableFontDrivers()
*
* open the font drivers key and check if there are any entries, if so
* return true. If that is the case we will call AddFontResourceW on
* Type 1 fonts at boot time, right after user had logged on
* PostScript printer drivers are not initialized at this time yet,
* it is safe to do it at this time.
* Effects:
*
* Warnings:
*
* History:
*  24-Apr-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL bLoadableFontDrivers()
{
    NTSTATUS             Status;
    UNICODE_STRING       UnicodeString;
    OBJECT_ATTRIBUTES    ObjA;
    KEY_FULL_INFORMATION KeyInfo;
    DWORD                dwReturnLength;
    HKEY                 hkey = NULL;
    BOOL                 bRet = FALSE;

// open the font drivers key and check if there are any entries, if so
// return true. If that is the case we will call AddFontResourceW on
// Type 1 fonts at boot time, right after user had logged on
// PostScript printer drivers are not initialized at this time yet,
// it is safe to do it at this time.

    RtlInitUnicodeString(&UnicodeString, pwszFontDrivers);
    InitializeObjectAttributes(&ObjA,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&hkey,
                       KEY_READ,
                       &ObjA);

    if (NT_SUCCESS(Status))
    {
    // get the number of entries in the [Fonts] section

        Status = NtQueryKey(hkey,
                    KeyFullInformation,
                    &KeyInfo,
                    sizeof(KeyInfo),
                    &dwReturnLength);

        if (NT_SUCCESS(Status) && KeyInfo.Values)
        {
            UserAssert(!(KeyInfo.ClassLength | KeyInfo.SubKeys | KeyInfo.MaxNameLen | KeyInfo.MaxClassLen));

        // externally loadable drivers are present, force sweep

            bRet = TRUE;
        }

        NtClose(hkey);
    }
    return bRet;
}



/***********************Public*Routine******************************\
*
* BOOL bCheckAndDeleteTTF()
*
* Checks whether there is a converted TTF corresponding to
* a Type1 font. Delete the TTF file and the reg entry if there is.
*
* History:
*  29-Jan-1998 -by- Xudong Wu [TessieW]
* Wrote it.
\*******************************************************************/
BOOL bCheckAndDeleteTTF
(
    HKEY    hkey,
    PKEY_FULL_INFORMATION pKeyInfo,
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo,
    PKEY_VALUE_BASIC_INFORMATION KeyValueBasicInfo,
    DWORD   cjData
)
{
    NTSTATUS    Status;
    UNICODE_STRING UnicodeString;
    DWORD       dwReturnLength;
    ULONG       iFont;
    WCHAR       awcTmp[MAX_PATH], *pFontName, *pType1Name, *pwcFile;
    BOOL        bDelTTFfile, bRet = TRUE;
    FLONG       fl;
    WCHAR       awcType1Name[MAX_PATH];  // null-terminated pType1Name
    WCHAR       awcFontName[MAX_PATH];   // null-terminated pFontName
    WCHAR       awcFile[MAX_PATH];       // null-terminated pwcFile

    // pKeyInfo holds the full info on the key "Fonts"
    for (iFont = 0; iFont < pKeyInfo->Values; iFont++)
    {
        RtlZeroMemory(KeyValueInfo->Name, cjData);
        Status = NtEnumerateValueKey(
                    hkey,
                    iFont,
                    KeyValueFullInformation,
                    KeyValueInfo,
                    sizeof(*KeyValueInfo) + cjData,
                    &dwReturnLength);

        if (NT_SUCCESS(Status))
        {
            UserAssert(KeyValueInfo->NameLength <= pKeyInfo->MaxValueNameLen);
            UserAssert(KeyValueInfo->DataLength <= pKeyInfo->MaxValueDataLen);
            UserAssert(KeyValueInfo->Type == REG_SZ);

            bDelTTFfile = FALSE;

            // Make sure we use null-terminated strings
            vNullTermWideString (awcType1Name,
                                 KeyValueBasicInfo->Name,
                                 KeyValueBasicInfo->NameLength / sizeof(WCHAR));
            vNullTermWideString (awcFontName,
                                 KeyValueInfo->Name,
                                 KeyValueInfo->NameLength / sizeof(WCHAR));
            vNullTermWideString (awcFile,
                                 (WCHAR *) ((BYTE *)KeyValueInfo + KeyValueInfo->DataOffset),
                                 KeyValueInfo->DataLength / sizeof(WCHAR));
            pType1Name = awcType1Name;
            pFontName = awcFontName;
            pwcFile = awcFile;

            while((*pType1Name) && (*pType1Name++ == *pFontName++))
                ;

            // if the font name match the type1 name
            // check whether this is a ttf font
            if ((*pType1Name == 0) && (*pFontName++ == L' '))
            {
                WCHAR *pTrueType = L"(TrueType)";

                while(*pTrueType && (*pTrueType++ == *pFontName++))
                    ;
                if (*pTrueType == 0)
                {
                    bDelTTFfile = TRUE;
                }
            }

            if (bDelTTFfile)
            {
                // delete the converted TTF file
                if (bRet = bMakePathNameW(awcTmp, pwcFile, NULL, &fl))
                {
                    UserVerify((bRet = DeleteFileW(awcTmp)));
                }

                // remove the reg entry
                *pFontName = 0;
                RtlInitUnicodeString(&UnicodeString, awcFontName);
                Status = NtDeleteValueKey(hkey, (PUNICODE_STRING)&UnicodeString);

                // decrement the number of values under [Fonts]
                if (NT_SUCCESS(Status))
                    pKeyInfo->Values--;
                else
                    bRet = FALSE;

                break;
            }
        }
        else
        {
            bRet = FALSE;
            break;
        }
    }

    return bRet;
}


/***********************Public*Routine**************************\
*
* BOOL bCleanConvertedTTFs()
*
* Enumerate each entry under "Upgrade Type1" key, call
* bCheckAndDeleteTTF() to remove the coverted TTFs.
*
* History:
*  29-Jan-1998 -by- Xudong Wu [TessieW]
* Wrote it.
\***************************************************************/
BOOL bCleanConvertedTTFs()
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjA;
    NTSTATUS    Status;
    HKEY        hkeyFonts = NULL, hkeyType1 = NULL;
    DWORD       dwReturnLength;
    DWORD       iFontT1, cjData;
    DWORD       cjMaxValueNameT1, cjMaxValueNameFonts;
    BOOL        bRet = FALSE, bError = FALSE;

    KEY_FULL_INFORMATION KeyInfoType1, KeyInfoFonts;
    PKEY_VALUE_BASIC_INFORMATION KeyValueBasicInfo;
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo;

    // Open and query the value from the "Type1 Fonts" key
    // No need to continue if not succeed or no Type1 font listed
    RtlInitUnicodeString(&UnicodeString, pwszType1Key);
    InitializeObjectAttributes(&ObjA,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

    Status = NtOpenKey(&hkeyType1,
                    KEY_READ | KEY_WRITE,
                    &ObjA);

    if (NT_SUCCESS(Status))
    {
        Status = NtQueryKey(hkeyType1,
                    KeyFullInformation,
                    &KeyInfoType1,
                    sizeof(KeyInfoType1),
                    &dwReturnLength);

        if (NT_SUCCESS(Status) && KeyInfoType1.Values)
        {
            UserAssert(!(KeyInfoType1.ClassLength | KeyInfoType1.SubKeys |
                         KeyInfoType1.MaxNameLen | KeyInfoType1.MaxClassLen));

            cjMaxValueNameT1 = DWORDALIGN(KeyInfoType1.MaxValueNameLen + sizeof(WCHAR));

            // Alloc buffer big enough to hold the longest Name
            if (KeyValueBasicInfo = UserLocalAlloc(0, sizeof(*KeyValueBasicInfo) + cjMaxValueNameT1))
            {
                RtlInitUnicodeString(&UnicodeString, pwszFontsKey);
                InitializeObjectAttributes(&ObjA,
                                    &UnicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);
                Status = NtOpenKey(&hkeyFonts,
                                KEY_READ | KEY_WRITE,
                                &ObjA);

                if (NT_SUCCESS(Status))
                {
                    Status = NtQueryKey(hkeyFonts,
                                KeyFullInformation,
                                &KeyInfoFonts,
                                sizeof(KeyInfoFonts),
                                &dwReturnLength);

                    if (NT_SUCCESS(Status) && KeyInfoFonts.Values)
                    {
                        UserAssert(!(KeyInfoFonts.ClassLength | KeyInfoFonts.SubKeys |
                                     KeyInfoFonts.MaxNameLen | KeyInfoFonts.MaxClassLen));

                        cjMaxValueNameFonts = DWORDALIGN(KeyInfoFonts.MaxValueNameLen + sizeof(WCHAR));
                        KeyInfoFonts.MaxValueDataLen = DWORDALIGN(KeyInfoFonts.MaxValueDataLen);
                        cjData = cjMaxValueNameFonts + KeyInfoFonts.MaxValueDataLen;

                        // Alloc buffer big enough to hold the longest Name and Value
                        if (KeyValueInfo = UserLocalAlloc(0, sizeof(*KeyValueInfo) + cjData))
                        {
                            // Enum the "Type1 Fonts" key
                            for (iFontT1 = 0; iFontT1 < KeyInfoType1.Values; iFontT1++)
                            {
                                RtlZeroMemory(KeyValueBasicInfo->Name, cjMaxValueNameT1);
                                Status = NtEnumerateValueKey(
                                            hkeyType1,
                                            iFontT1,
                                            KeyValueBasicInformation,
                                            KeyValueBasicInfo,
                                            sizeof(*KeyValueBasicInfo) + cjMaxValueNameT1,
                                            &dwReturnLength);

                                if (NT_SUCCESS(Status))
                                {
                                    UserAssert(KeyValueBasicInfo->NameLength <= KeyInfoType1.MaxValueNameLen);
                                    UserAssert(KeyValueBasicInfo->Type == REG_MULTI_SZ);

                                    // For each Type1 font, check to see whether
                                    // there is corresponding converted TTF
                                    // Delete the TTF file and reg entry if any

                                    bRet = bCheckAndDeleteTTF(hkeyFonts, &KeyInfoFonts, KeyValueInfo,
                                                            KeyValueBasicInfo, cjData);
                                    if (!bRet)
                                    {
                                        bError = TRUE;
                                    }
                                }
                            }
                            UserLocalFree(KeyValueInfo);
                            // no type1 fonts installed
                            if (KeyInfoType1.Values == 0)
                            {
                                bRet = TRUE;
                            }
                        }
                    }
                    NtClose(hkeyFonts);
                }  // NtOpenKey (hkeyFonts)
                UserLocalFree(KeyValueBasicInfo);
            }
        }  // NtQueryKey (hkeyType1)
        NtClose(hkeyType1);
    }

    return (bRet && !bError);
}


/***********************Public*Routine******************************\
*
* VOID vCleanConvertedTTFs()
*
* Delete the converted TTFs and clean the registry if there is any
* TTFs generated from Type1 fonts.
*
* History:
*  29-Jan-1998 -by- Xudong Wu [TessieW]
* Wrote it.
\*******************************************************************/
VOID vCleanConvertedTTFs()
{
    BOOL    bNeedUpgrade = FALSE;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjA;
    DWORD      dwReturnLength;
    NTSTATUS   Status;
    HKEY       hkeyUpgradeType1 = NULL;

    struct {
        KEY_VALUE_PARTIAL_INFORMATION;
        LARGE_INTEGER;
    } UpgradeValueInfo;
    DWORD      UpgradeValue = 0;

    RtlInitUnicodeString(&UnicodeString, pwszUpdType1Key);
    InitializeObjectAttributes(&ObjA,
                           &UnicodeString,
                           OBJ_CASE_INSENSITIVE,
                           NULL,
                           NULL);

    Status = NtOpenKey(&hkeyUpgradeType1,
                   KEY_READ | KEY_WRITE,
                   &ObjA);

    if (!NT_SUCCESS(Status))
    {
        // Key doesn't exist, run for the first time
        // Create the key, open it for writing

        DWORD  dwDisposition;

        Status = NtCreateKey(&hkeyUpgradeType1,
                         KEY_WRITE,
                         &ObjA,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &dwDisposition);
        if (NT_SUCCESS(Status))
        {
            bNeedUpgrade = TRUE;
        }
    }
    else
    {
        RtlInitUnicodeString(&UnicodeString, UPGRADED_TYPE1);
        Status = NtQueryValueKey(hkeyUpgradeType1,
                         &UnicodeString,
                         KeyValuePartialInformation,
                         &UpgradeValueInfo,
                         sizeof(UpgradeValueInfo),
                         &dwReturnLength);

        if (NT_SUCCESS(Status))
        {
            UserAssert(UpgradeValueInfo.Type == REG_DWORD);
            UserAssert(UpgradeValueInfo.DataLength == sizeof(UpgradeValue));
            RtlCopyMemory(&UpgradeValue, &UpgradeValueInfo.Data, sizeof(UpgradeValue));

            // Done if the value is non-zero.
            if (UpgradeValue == 0)
            {
               bNeedUpgrade = TRUE;
            }
        }
    }

    if (bNeedUpgrade)
    {
        if (bCleanConvertedTTFs())
        {
            UpgradeValue = 1;
        }

        RtlInitUnicodeString(&UnicodeString, UPGRADED_TYPE1);
        Status = NtSetValueKey(hkeyUpgradeType1,
              &UnicodeString,
              0,
              REG_DWORD,
              &UpgradeValue,
              sizeof(UpgradeValue));
        UserAssert(NT_SUCCESS(Status));
    }

    if (hkeyUpgradeType1)
    {
        NtClose(hkeyUpgradeType1);
    }
}


/******************************Public*Routine******************************\
*
* VOID vFontSweep()
*
* Effects: The main routine, calls vSweepFonts to sweep "regular" fonts
*          and then to sweep type 1 fonts
*
* History:
*  20-Nov-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vFontSweep()
{
// check if shared windows directory installation:

    gbWin31Upgrade = bCheckIfDualBootingWithWin31();

// before we sweep the files to the Fonts directory,
// check whether the 'converted' ttf's have been rmoved

    vCleanConvertedTTFs();

// sweep fonts in the [Fonts] key

    vSweepFonts(pwszFontsKey, pwszSweepKey, vProcessFontEntry, FALSE);

// now sweep type 1 fonts, if any

    vSweepFonts(pwszType1Key, pwszSweepType1Key, vProcessType1FontEntry, FALSE);

// one of the two routines above may have initialized %windir%\system
// and %windir%\fonts directories. Free the memory associated with this

    if (gpwcSystemDir)
    {
        LocalFree(gpwcSystemDir);
        gpwcSystemDir = NULL;
    }

}


/******************************Public*Routine******************************\
*
* VOID vLoadLocal/RemoteT1Fonts()
*
* History:
*  30-Apr-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vLoadT1Fonts(PFNENTRY pfnProcessFontEntry)
{

    if (bLoadableFontDrivers())
    {
    #if DBGSWEEP
        DbgPrint("vLoadT1Fonts(0x%lx) was called\n", pfnProcessFontEntry);
    #endif
    // now enum and add remote type1 fonts if any

        vSweepFonts(pwszType1Key, pwszSweepType1Key, pfnProcessFontEntry, TRUE);

    // if the routines above initialized %windir%\system
    // and %windir%\fonts directories. Free the memory associated with this

        if (gpwcSystemDir)
        {
            LocalFree(gpwcSystemDir);
            gpwcSystemDir = NULL;
        }
    }
}

VOID vLoadLocalT1Fonts()
{
    vLoadT1Fonts(vAddLocalType1Font);
}

VOID vLoadRemoteT1Fonts()
{
    vLoadT1Fonts(vAddRemoteType1Font);
}
