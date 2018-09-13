/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1991
 *  All Rights Reserved.
 *
 *
 *  PIFINF.C
 *  INF file processing for PIFMGR.DLL
 *
 *  History:
 *  Created 07-Jan-1993 11:20am by Jeff Parsons
 */

#include "shellprv.h"
#pragma hdrstop
#include <setupapi.h>

#ifdef UNICODE
const CHAR szIpOpen          []= "SetupOpenInfFileW";
const CHAR szIpFindFirstLine []= "SetupFindFirstLineW";
const CHAR szIpFindNextLine  []= "SetupFindNextLine";
const CHAR szIpGetStringField[]= "SetupGetStringFieldW";
const CHAR szIpGetIntField   []= "SetupGetIntField";
const CHAR szIpClose         []= "SetupCloseInfFile";
#else
const CHAR szIpOpen          []= "SetupOpenInfFileA";
const CHAR szIpFindFirstLine []= "SetupFindFirstLineA";
const CHAR szIpFindNextLine  []= "SetupFindNextLine";
const CHAR szIpGetStringField[]= "SetupGetStringFieldA";
const CHAR szIpGetIntField   []= "SetupGetIntField";
const CHAR szIpClose         []= "SetupCloseInfFile";
#endif
const TCHAR szOne             []= TEXT("1");

//
//  WARNING: Do not change the format of this structure (BPSTR followed by
//  FARPROC) unless you also plan on the changing the code below that fills
//  the structure in.  The sole purpose of this structure is and should
//  remain to obtain various exported procedure addresses from SETUPX.DLL!
//

typedef const CHAR *BPSTR;
struct {
    BPSTR   pszIpOpen;
    HINF   (WINAPI *lpIpOpen)(LPCTSTR pszFileSpec, LPCTSTR InfClass, DWORD InfStyle, UINT *ErrorLine);
    BPSTR   pszIpFindFirstLine;
    BOOL   (WINAPI *lpIpFindFirstLine)(HINF hInf, LPCTSTR lpszSect, LPCTSTR lpszKey, PINFCONTEXT Context);
    BPSTR   pszIpFindNextLine;
    BOOL   (WINAPI *lpIpFindNextLine)(PINFCONTEXT ContextIn, PINFCONTEXT ContextOut);
    BPSTR   pszIpGetStringField;
    BOOL   (WINAPI *lpIpGetStringField)(PINFCONTEXT Context, DWORD FieldIndex, LPTSTR ReturnBuffer, DWORD ReturnBufferSize, DWORD * RequiredSize);
    BPSTR   pszIpGetIntField;
    BOOL   (WINAPI *lpIpGetIntField)(PINFCONTEXT Context, DWORD FieldIndex, INT * IntegerValue);
    BPSTR   pszIpClose;
    VOID   (WINAPI *lpIpClose)(HINF hInf);
} sfnIp = {
    szIpOpen          ,NULL,
    szIpFindFirstLine ,NULL,
    szIpFindNextLine  ,NULL,
    szIpGetStringField,NULL,
    szIpGetIntField   ,NULL,
    szIpClose         ,NULL,
};

static UINT g_SXUseCount = 0;
static HINSTANCE g_hSetupDLL = NULL;

const TCHAR szRegKeyMSDOSApps[] = REGSTR_PATH_NEWDOSBOX;

const TCHAR szParams[]          = KEY_PARAMS;
const TCHAR szBatchFile[]       = KEY_BATCHFILE;
const TCHAR szLowMem[]          = KEY_LOWMEM;
const TCHAR szEmsMem[]          = KEY_EMSMEM;
const TCHAR szXmsMem[]          = KEY_XMSMEM;
const TCHAR szDpmiMem[]         = KEY_DPMIMEM;
const TCHAR szEnable[]          = KEY_ENABLE;
const TCHAR szDisable[]         = KEY_DISABLE;
#ifndef WINNT
const TCHAR szAppHack[]         = KEY_APPHACK;
#endif

const TCHAR szWindowed[]        = KEYVAL_WINDOWED;
const TCHAR szBackground[]      = KEYVAL_BACKGROUND;
const TCHAR szExclusive[]       = KEYVAL_EXCLUSIVE;
const TCHAR szDetectIdle[]      = KEYVAL_DETECTIDLE;
const TCHAR szLowLocked[]       = KEYVAL_LOWLOCKED;
const TCHAR szEMSLocked[]       = KEYVAL_EMSLOCKED;
const TCHAR szXMSLocked[]       = KEYVAL_XMSLOCKED;
const TCHAR szUseHMA[]          = KEYVAL_USEHMA;
const TCHAR szEmulateROM[]      = KEYVAL_EMULATEROM;
const TCHAR szRetainVRAM[]      = KEYVAL_RETAINVRAM;
const TCHAR szFastPaste[]       = KEYVAL_FASTPASTE;
const TCHAR szALTTAB[]          = KEYVAL_ALTTAB;
const TCHAR szALTESC[]          = KEYVAL_ALTESC;
const TCHAR szCTRLESC[]         = KEYVAL_CTRLESC;
const TCHAR szPRTSCRN[]         = KEYVAL_PRTSCRN;
const TCHAR szALTPRTSCRN[]      = KEYVAL_ALTPRTSCRN;
const TCHAR szALTSPACE[]        = KEYVAL_ALTSPACE;
const TCHAR szALTENTER[]        = KEYVAL_ALTENTER;
const TCHAR szWinLie[]          = KEYVAL_WINLIE;
const TCHAR szGlobalMem[]       = KEYVAL_GLOBALMEM;
const TCHAR szRealMode[]        = KEYVAL_REALMODE;
const TCHAR szMouse[]           = KEYVAL_MOUSE;
const TCHAR szEMS[]             = KEYVAL_EMS;
const TCHAR szCDROM[]           = KEYVAL_CDROM;
const TCHAR szNetwork[]         = KEYVAL_NETWORK;
const TCHAR szDiskLock[]        = KEYVAL_DISKLOCK;
const TCHAR szPrivateCFG[]      = KEYVAL_PRIVATECFG;
#ifndef WINNT
const TCHAR szVesa[]            = KEYVAL_VESA;
#endif
const TCHAR szCloseOnExit[]     = KEYVAL_CLOSEONEXIT;
const TCHAR szAllowSSaver[]     = KEYVAL_ALLOWSSAVER;
const TCHAR szUniqueSettings[]  = KEYVAL_UNIQUESETTINGS;
#ifdef LATER
const TCHAR szDisplayTBar[]     = KEYVAL_DISPLAYTBAR;
const TCHAR szRestoreWin[]      = KEYVAL_RESTOREWIN;
const TCHAR szQuickEdit[]       = KEYVAL_QUICKEDIT;
const TCHAR szExclMouse[]       = KEYVAL_EXCLMOUSE;
const TCHAR szWarnIfActive[]    = KEYVAL_WARNIFACTIVE;
#endif


const LPCTSTR apszKey[] = {
    szParams,
    szBatchFile,
    szLowMem,
    szEmsMem,
    szXmsMem,
    szDpmiMem,
    szEnable,
    szDisable,
#ifndef WINNT
    szAppHack,
#endif
};

const LPCTSTR apszKeyVal[] = {
    szWindowed,         // abKeyValIDBits[0]
    szBackground,       // abKeyValIDBits[1]
    szExclusive,        // abKeyValIDBits[2]
    szDetectIdle,       // abKeyValIDBits[3]
    szLowLocked,        // abKeyValIDBits[4]
    szEMSLocked,        // abKeyValIDBits[5]
    szXMSLocked,        // abKeyValIDBits[6]
    szUseHMA,           // abKeyValIDBits[7]
    szEmulateROM,       // abKeyValIDBits[8]
    szRetainVRAM,       // abKeyValIDBits[9]
    szFastPaste,        // abKeyValIDBits[10]
    szALTTAB,           // abKeyValIDBits[11]
    szALTESC,           // abKeyValIDBits[12]
    szCTRLESC,          // abKeyValIDBits[13]
    szPRTSCRN,          // abKeyValIDBits[14]
    szALTPRTSCRN,       // abKeyValIDBits[15]
    szALTSPACE,         // abKeyValIDBits[16]
    szALTENTER,         // abKeyValIDBits[17]
    szWinLie,           // abKeyValIDBits[18]
    szGlobalMem,        // abKeyValIDBits[19]
    szRealMode,         // abKeyValIDBits[20]
    szMouse,            // abRMKeyValIDBits[0]
    szEMS,              // abRMKeyValIDBits[1]
    szCDROM,            // abRMKeyValIDBits[2]
    szNetwork,          // abRMKeyValIDBits[3]
    szDiskLock,         // abRMKeyValIDBits[4]
    szPrivateCFG,       // abRMKeyValIDBits[5]
#ifndef WINNT
    szVesa,             // abRMKeyValIDBIts[6]
#endif
    szCloseOnExit,      // special case 0 (see "special case 0" below)
    szAllowSSaver,      // special case 1 (see "special case 1" below)
    szUniqueSettings,   // Never transferred to PIF - Used to populate registry
#ifdef LATER
    szDisplayTBar,
    szRestoreWin,
    szQuickEdit,
    szExclMouse,
    szWarnIfActive,
#endif
};

//  Array of bit numbers that must be kept in sync with KEYVALIDs
//
//  0x80 means bit must be inverted
//  0x40 means bit must be set in PfW386Flags2 instead of PfW386Flags

const BYTE abKeyValIDBits[] = {
    BITNUM(fFullScreen)     | 0x80,
    BITNUM(fBackground),
    BITNUM(fExclusive),
    BITNUM(fPollingDetect),
    BITNUM(fVMLocked),
    BITNUM(fEMSLocked),
    BITNUM(fXMSLocked),
    BITNUM(fNoHMA)          | 0x80,
    BITNUM(fVidTxtEmulate)  | 0x40,
    BITNUM(fVidRetainAllo)  | 0x40,
    BITNUM(fINT16Paste),
    BITNUM(fALTTABdis)      | 0x80,
    BITNUM(fALTESCdis)      | 0x80,
    BITNUM(fCTRLESCdis)     | 0x80,
    BITNUM(fPRTSCdis)       | 0x80,
    BITNUM(fALTPRTSCdis)    | 0x80,
    BITNUM(fALTSPACEdis)    | 0x80,
    BITNUM(fALTENTERdis)    | 0x80,
    BITNUM(fWinLie),
    BITNUM(fGlobalProtect),
    BITNUM(fRealMode),
};

const BYTE abRMKeyValIDBits[] = {
    BITNUM(RMOPT_MOUSE),
    BITNUM(RMOPT_EMS),
    BITNUM(RMOPT_CDROM),
    BITNUM(RMOPT_NETWORK),
    BITNUM(RMOPT_DISKLOCK),
    BITNUM(RMOPT_PRIVATECFG),
    BITNUM(RMOPT_VESA),
};

//  BUGBUG: other bits to be supported (maybe):
//      WIN_TOOLBAR,
//      WIN_SAVESETTINGS,
//      MSE_WINDOWENABLE | 0x80,
//      MSE_EXCLUSIVE,
//      TSK_NOWARNTERMINATE | 0x80,


//
//  Load setupx.dll and get proc address for all entry points in table
//
BOOL InitSetupxDll(void)
{
    BOOL    fSuccess = TRUE;    // Assume it works
    if (g_SXUseCount == 0) {
        //
        // Dynamically load SETUPX.DLL
        //
        UINT uOldMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
#ifdef WINNT
        g_hSetupDLL = LoadLibrary(TEXT("SETUPAPI.DLL"));
#else
        g_hSetupDLL = LoadLibrary("SETUPX.DLL");
#endif
        SetErrorMode(uOldMode);
        fSuccess = (g_hSetupDLL != NULL);

        if (fSuccess) {
            int i;
            BPSTR *ppsz;
            FARPROC *plpfn;

            ppsz = (BPSTR *)&sfnIp;

#ifdef WINNT
            // BUGBUG: HACK: 6 is number of functions on NT
            for (i=0; i< 6; i++) {
#else
            for (i=0; i< ARRAYSIZE(sfnIp); i++) {
#endif
                plpfn = (FARPROC *)(ppsz+1);
                *plpfn = GetProcAddress(g_hSetupDLL, *ppsz);
                if (*plpfn == NULL) {
                    FreeLibrary(g_hSetupDLL);
                    g_hSetupDLL = NULL;
                    fSuccess = FALSE;
                    break;
                }
                ppsz = (BPSTR *)(plpfn+1);
            }
        }
    }
    if (fSuccess) {
        g_SXUseCount++;
    }
    return(fSuccess);
}

//
//  When the last user of setupx.dll calls this function, the library is freed.
//
void FreeSetupxDll(void)
{
    g_SXUseCount--;
    if (g_SXUseCount == 0) {
        FreeLibrary(g_hSetupDLL);
        g_hSetupDLL = NULL;
    }
}


#ifdef UNICODE
void InitWorkDir(PPROPLINK ppl, LPPROPPRG lpPrg, LPPROPNT40 lpnt40)
#else
void InitWorkDir(PPROPLINK ppl, LPPROPPRG lpPrg)
#endif
{
    int i;

#ifdef UNICODE
    if (lpnt40)
    {
        lstrcpyn((LPTSTR)lpnt40->awchWorkDir,
                 ppl->szPathName,
                 min(ARRAYSIZE(lpnt40->awchWorkDir),ppl->iFileName+1));

        // Working directories like C:\ are ok, but C:\FOO\ are not,
        // so remove trailing '\' in that case

        i = lstrlen((LPTSTR)lpnt40->awchWorkDir)-1;
        if (i > 2 && lpnt40->awchWorkDir[i] == TEXT('\\'))
            lpnt40->awchWorkDir[i] = TEXT('\0');

        WideCharToMultiByte( CP_ACP, 0, (LPWSTR)lpnt40->awchWorkDir, -1, lpPrg->achWorkDir, ARRAYSIZE(lpPrg->achWorkDir), NULL, NULL );
    }
    else
    {
        WideCharToMultiByte( CP_ACP, 0,
                             ppl->szPathName,
                             min(ARRAYSIZE(lpPrg->achWorkDir),ppl->iFileName+1),
                             (LPSTR)lpPrg->achWorkDir,
                             ARRAYSIZE(lpPrg->achWorkDir),
                             NULL,
                             NULL
                            );

        // Working directories like C:\ are ok, but C:\FOO\ are not,
        // so remove trailing '\' in that case

        i = lstrlenA(lpPrg->achWorkDir)-1;
        if (i > 2 && lpPrg->achWorkDir[i] == '\\')
            lpPrg->achWorkDir[i] = '\0';
    }
#else

    lstrcpynA(lpPrg->achWorkDir,
              ppl->szPathName,
              min(ARRAYSIZE(lpPrg->achWorkDir),ppl->iFileName+1));

    // Working directories like C:\ are ok, but C:\FOO\ are not,
    // so remove trailing '\' in that case

    i = lstrlenA(lpPrg->achWorkDir)-1;
    if (i > 2 && lpPrg->achWorkDir[i] == TEXT('\\'))
        lpPrg->achWorkDir[i] = TEXT('\0');
#endif
}


#ifdef UNICODE
BOOL FAR GetAppsInfData(PPROPLINK ppl, LPPROPPRG lpPrg, LPPROPNT40 lpnt40, HINF hInf, LPCTSTR lpszApp, BOOL fNotAmbiguous, int flOpt)
#else
BOOL FAR GetAppsInfData(PPROPLINK ppl, LPPROPPRG lpPrg, HINF hInf, LPCTSTR lpszApp, BOOL fNotAmbiguous, int flOpt)
#endif
{
    HINF hinfApps;
    int id, i;
    TCHAR szTmp[MAX_PATH];
    TCHAR szPIFSection[MAX_KEY_SIZE];
    BOOL fSuccess = FALSE;
    INFCONTEXT InfContext;
    DWORD dwSize;
    FunctionName(GetAppsInfData);

    //
    // Although not strictly part of INF processing, it's most
    // convenient here to search for any ICO file that might exist
    // in the same directory as the app, and select it for our default icon.
    //
    lstrcpyn(szTmp, ppl->szPathName, ppl->iFileExt+1);
    lstrcpy(szTmp + ppl->iFileExt, TEXT(".ICO"));
    if ((int)GetFileAttributes(szTmp) != -1) {
#ifdef UNICODE
        lstrcpyn((LPTSTR)lpnt40->awchIconFile, szTmp, ARRAYSIZE(lpnt40->awchIconFile));
        WideCharToMultiByte( CP_ACP, 0, (LPWSTR)lpnt40->awchIconFile, -1, lpPrg->achIconFile, ARRAYSIZE(lpPrg->achIconFile), NULL, NULL );
#else
        lstrcpyn(lpPrg->achIconFile, szTmp, ARRAYSIZE(lpPrg->achIconFile));
#endif

        lpPrg->wIconIndex = 0;
        PifMgr_SetProperties(ppl, MAKELP(0,GROUP_PRG),
                        lpPrg, SIZEOF(*lpPrg), SETPROPS_CACHE);
#ifdef UNICODE
        PifMgr_SetProperties(ppl, MAKELP(0,GROUP_NT40),
                        lpnt40, SIZEOF(*lpnt40), SETPROPS_CACHE);
#endif
    }

    //
    // Dynamically load SETUPX.DLL
    //
    if (!InitSetupxDll())
        goto DllLoadFalied;

    if (hInf)
        hinfApps = hInf;
    else
        hinfApps = (*sfnIp.lpIpOpen)(LoadStringSafe(NULL,
                                                IDS_APPSINF,
                                                szTmp,
                                                ARRAYSIZE(szTmp)),
                                     0, INF_STYLE_WIN4, NULL );

    if (hinfApps==INVALID_HANDLE_VALUE) {
        id = IDS_CANTOPENAPPSINF;
        if (GetLastError()==ERROR_FILE_NOT_FOUND)
            id = IDS_NOAPPSINF;
        Warning((HWND)ppl, (WORD)id, MB_ICONEXCLAMATION | MB_OK | MB_NOFOCUS);
        goto CloseDLL;
    }

    // OK, now we have APPS.INF open, so let's bounce around the [pif95]
    // section and try to find the app of interest.

    if (!(*sfnIp.lpIpFindFirstLine)(hinfApps, TEXT("pif95"), NULL, &InfContext)) {
        Warning((HWND)ppl, IDS_APPSINFERROR, MB_ICONEXCLAMATION | MB_OK | MB_NOFOCUS);
        goto CloseInf;
    }

    // OK, we've found the [pif95] section, so let's go to it

    do {


        if (!(*sfnIp.lpIpGetStringField)(&InfContext, APPSINF_FILENAME, szTmp, ARRAYSIZE(szTmp), &dwSize))
            continue;

        // We need to read the rest of the fields now, before we do any
        // more processing, because otherwise we lose our place in the file

        if (lstrcmpi(szTmp, ppl->szPathName+ppl->iFileName) == 0) {

            // See if Other File was specified, and then make sure it
            // exists.  If it doesn't, then we need to continue the search.

            // Initialize szTmp with only the path portion of the app's
            // fully-qualified name.  Giving lstrcpyn a length of iFileName+1
            // insures that szTmp[ppl->iFileName] will be NULL.

            lstrcpyn(szTmp, ppl->szPathName, ppl->iFileName+1);

            (*sfnIp.lpIpGetStringField)(&InfContext, APPSINF_OTHERFILE,
                        &szTmp[ppl->iFileName], ARRAYSIZE(lpPrg->achOtherFile), &dwSize);

            // If szTmp[ppl->iFileName] is no longer NULL, then
            // GetStringField filled it in.  See if the file exists.

            if (szTmp[ppl->iFileName]) {
                if ((int)GetFileAttributes(szTmp) == -1)
                    continue;       // Other File didn't exist, continue search
            }

            // If the PIF data we have is ambiguous, and it has already
            // been initialized with data from this APPS.INF entry, then just
            // leave the PIF data alone and LEAVE.

            if (lpPrg->flPrgInit & PRGINIT_AMBIGUOUSPIF) {

#ifdef UNICODE
                if (lstrcmpi((LPWSTR)lpnt40->awchOtherFile, szTmp+ppl->iFileName) == 0) {
#else
                if (lstrcmpiA(lpPrg->achOtherFile, szTmp+ppl->iFileName) == 0) {
#endif

                    if (!szTmp[ppl->iFileName]) {

                        // The comparison was inconclusive;  both filenames
                        // are blank.  See if the filename contained in
                        // lpPrg->achCmdLine matches lpszApp;  if not, again
                        // we should fail the search.
                        //
                        // It's ok to whack lpPrg->achCmdLine with a null;
                        // OpenProperties (our only caller) doesn't depend on
                        // that data in lpPrg.

#ifdef UNICODE
                        lpnt40->awchCmdLine[lstrskipfnameA(lpPrg->achCmdLine)] = L'\0';

                        if (lstrcmpi((LPWSTR)lpnt40->awchCmdLine, lpszApp) != 0)

#else
                        lpPrg->achCmdLine[lstrskipfnameA(lpPrg->achCmdLine)] = '\0';

                        if (lstrcmpiA(lpPrg->achCmdLine, lpszApp) != 0)
#endif

                            goto CloseInf;  // unsuccessful search
                    }
                    fSuccess++;             // successful search
                }

                // Otherwise, this APPS.INF entry isn't a match, implying
                // some of the PIF's settings don't really apply.  We need
                // to fail this search, get back to OpenProperties, look ONLY
                // for _DEFAULT.PIF, and let it try to call GetAppsInfData
                // one more time.

                goto CloseInf;
            }

            // Otherwise, update Other File.  THIS is the APPS.INF entry
            // we're going to use!

#ifdef UNICODE
            lstrcpyn((LPWSTR)lpnt40->awchOtherFile, szTmp + ppl->iFileName, ARRAYSIZE(lpnt40->awchOtherFile));
            WideCharToMultiByte( CP_ACP, 0, (LPWSTR)lpnt40->awchOtherFile, -1, lpPrg->achOtherFile, ARRAYSIZE( lpPrg->achOtherFile ), NULL, NULL );

            (*sfnIp.lpIpGetStringField)(&InfContext, APPSINF_TITLE, (LPWSTR)lpnt40->awchTitle, ARRAYSIZE(lpnt40->awchTitle), &dwSize);
            WideCharToMultiByte( CP_ACP, 0, (LPWSTR)lpnt40->awchTitle, -1, lpPrg->achTitle, ARRAYSIZE( lpPrg->achTitle ), NULL, NULL );

            lstrcpyn((LPWSTR)lpnt40->awchCmdLine, lpszApp, ARRAYSIZE(lpnt40->awchCmdLine));
            WideCharToMultiByte( CP_ACP, 0, (LPWSTR)lpnt40->awchCmdLine, -1, lpPrg->achCmdLine, ARRAYSIZE( lpPrg->achCmdLine ), NULL, NULL );
#else
            lstrcpyn((LPSTR)lpPrg->achOtherFile, szTmp + ppl->iFileName, ARRAYSIZE(lpPrg->achOtherFile));
            (*sfnIp.lpIpGetStringField)(&InfContext, APPSINF_TITLE,
                        (LPSTR)lpPrg->achTitle, ARRAYSIZE(lpPrg->achTitle), &dwSize);

            lstrcpyn((LPSTR)lpPrg->achCmdLine, lpszApp, ARRAYSIZE(lpPrg->achCmdLine));
#endif


            i = 0;
            (*sfnIp.lpIpGetIntField)(&InfContext, APPSINF_NOWORKDIR, &i);

            // Only set the working directory if "NoWorkDir" in the INF
            // is FALSE and no working directory was supplied by the caller.

#ifdef UNICODE
            if (i == 0 && !lpnt40->awchWorkDir[0]) {
                // No hard-coded working directory, so let's provide one

                InitWorkDir(ppl, lpPrg, lpnt40);
#else
            if (i == 0 && !lpPrg->achWorkDir[0]) {
                // No hard-coded working directory, so let's provide one

                InitWorkDir(ppl, lpPrg );
#endif

            }

            szTmp[0] = 0;
            sfnIp.lpIpGetStringField(&InfContext, APPSINF_ICONFILE, szTmp, ARRAYSIZE(szTmp), &dwSize);

            if (!szTmp[0])
                lstrcpy(szTmp, TEXT("SHELL32.DLL"));

            i = 0;
            sfnIp.lpIpGetIntField(&InfContext, APPSINF_ICONINDEX, &i);

            // Update the icon info now, if it's valid

            if (i != 0) {
#ifdef UNICODE
                lstrcpy((LPWSTR)lpnt40->awchIconFile, szTmp);
                WideCharToMultiByte( CP_ACP, 0, (LPWSTR)lpnt40->awchIconFile, -1, lpPrg->achIconFile, ARRAYSIZE( lpPrg->achIconFile ), NULL, NULL );
#else
                lstrcpy(lpPrg->achIconFile, szTmp);
#endif
                lpPrg->wIconIndex = (WORD) i;
            }

            (*sfnIp.lpIpGetStringField)(&InfContext, APPSINF_SECTIONID,
                        szPIFSection, ARRAYSIZE(szPIFSection), &dwSize);

            szTmp[0] = TEXT('\0');
            (*sfnIp.lpIpGetStringField)(&InfContext, APPSINF_NOPIF,
                        szTmp, ARRAYSIZE(szTmp), &dwSize);

            // This code used to set INHBITPIF if the app was NOT on a
            // fixed disk, knowing that we would otherwise try to create
            // a PIF in the PIF directory instead of the app's directory;
            // in other words, NOPIF really meant "no PIF in the PIF
            // directory please, because this app is ambiguously named".

            // Now, we want to always allow PIF creation, so the user
            // always has a place to save properties for an app.  But we
            // also need to propagate the old NOPIF flag to AMBIGUOUSPIF,
            // so that we'll always check to see if the PIF should be
            // regenerated (based on the presence of a NEW Other File).

            lpPrg->flPrgInit &= ~PRGINIT_AMBIGUOUSPIF;
            if (!fNotAmbiguous && szTmp[0] == TEXT('1'))
                lpPrg->flPrgInit |= PRGINIT_AMBIGUOUSPIF;

            if (flOpt & OPENPROPS_FORCEREALMODE)
                lpPrg->flPrgInit |= PRGINIT_REALMODE;

            // Time to dirty those properties!

            PifMgr_SetProperties(ppl, MAKELP(0,GROUP_PRG),
                            lpPrg, SIZEOF(*lpPrg), SETPROPS_CACHE);
#ifdef UNICODE
            PifMgr_SetProperties(ppl, MAKELP(0,GROUP_NT40),
                            lpnt40, SIZEOF(*lpnt40), SETPROPS_CACHE);
#endif

            GetAppsInfSectionData(&InfContext, APPSINF_DEFAULT_SECTION, ppl);

            if (*szPIFSection)
                GetAppsInfSectionData(&InfContext, szPIFSection, ppl);

            // Make a note that we found INF settings (appwiz cares)

            ppl->flProp |= PROP_INFSETTINGS;

            // GetAppsInfSectionData affects program props, so get fresh copy

            PifMgr_GetProperties(ppl, MAKELP(0,GROUP_PRG),
                            lpPrg, SIZEOF(*lpPrg), GETPROPS_NONE);

            // Now call appwiz in "silent configuration mode", to create the
            // per-app config and autoexec images, if app runs in real mode;
            // BUT don't do this if the caller (NOT the INF) specified no PIF,
            // to avoid unwanted dialog boxes popping up from appwiz.  Yes, I'm
            // telling appwiz to be quiet, but sometimes he just can't contain
            // himself (ie, silent configuration may not be possible given the
            // the real-mode configuration required).

            if (!(ppl->flProp & PROP_INHIBITPIF)) {
                if (lpPrg->flPrgInit & PRGINIT_REALMODE)
                    AppWizard(NULL, ppl, WIZACTION_SILENTCONFIGPROP);
            }
            FlushPIFData(ppl, FALSE);

            fSuccess++;             // successful search
            goto CloseInf;
        }

    } while ((*sfnIp.lpIpFindNextLine)(&InfContext, &InfContext));

  CloseInf:
    if (!hInf)
        (*sfnIp.lpIpClose)(hinfApps);

  CloseDLL:
    FreeSetupxDll();

  DllLoadFalied:
    return fSuccess;
}


void GetAppsInfSectionData(PINFCONTEXT pInfContext, LPCTSTR lpszSection, PPROPLINK ppl)
{
    int i, j, idKey;
    LPSTDPIF lpstd;
    LPW386PIF30 lp386;
    LPWENHPIF40 lpenh;
    TCHAR szVal[MAX_KEYVAL_SIZE];
    TCHAR szVal2[MAX_KEYVAL_SIZE];
    FunctionName(GetAppsInfSectionData);

    if (!(*sfnIp.lpIpFindFirstLine)(pInfContext, lpszSection, NULL, NULL))
        return;

    ppl->cLocks++;

    lpstd = (LPSTDPIF)ppl->lpPIFData;

    // lp386 may or may not exist, but we'll create if not

    lp386 = GetGroupData(ppl, szW386HDRSIG30, NULL, NULL);
    if (!lp386) {
        if (AddGroupData(ppl, szW386HDRSIG30, NULL, SIZEOF(W386PIF30))) {
            lp386 = GetGroupData(ppl, szW386HDRSIG30, NULL, NULL);
            if (!lp386)
                goto UnlockPIF;
        }
    }

    // lpenh may or may not exist, but we'll create if not

    lpenh = GetGroupData(ppl, szWENHHDRSIG40, NULL, NULL);
    if (!lpenh) {
        if (AddGroupData(ppl, szWENHHDRSIG40, NULL, SIZEOF(WENHPIF40))) {
            lpenh = GetGroupData(ppl, szWENHHDRSIG40, NULL, NULL);
            if (!lpenh)
                goto UnlockPIF;
        }
    }

    do {
        BYTE bInvert;
        DWORD dwSize;

        idKey = GetKeyID(pInfContext);

        if (!(*sfnIp.lpIpGetStringField)(pInfContext, APPSINF_KEYVAL, szVal, ARRAYSIZE(szVal), &dwSize))
            continue;

        szVal2[0] = TEXT('\0');
        if (idKey >= KEYID_LOWMEM && idKey <= KEYID_DPMIMEM)
            (*sfnIp.lpIpGetStringField)(pInfContext, APPSINF_KEYVAL2, szVal2, ARRAYSIZE(szVal2), &dwSize);

        bInvert = 0;

        switch (idKey)
        {
        case KEYID_UNKNOWN:
            ASSERTFAIL();
            break;

        case KEYID_NONE:
            break;

        case KEYID_PARAMS:
#ifdef UNICODE
            {
            WCHAR szTmp[ ARRAYSIZE(lp386->PfW386params) ];

            MultiByteToWideChar( CP_ACP, 0, (LPSTR)lp386->PfW386params, -1, szTmp, ARRAYSIZE(szTmp) );
            (*sfnIp.lpIpGetStringField)(pInfContext, APPSINF_KEYVAL, szTmp, SIZEOF(lp386->PfW386params), &dwSize);
            }

#else
            (*sfnIp.lpIpGetStringField)(pInfContext, APPSINF_KEYVAL, (LPTSTR)lp386->PfW386params, SIZEOF(lp386->PfW386params), &dwSize);
#endif
            break;

        case KEYID_BATCHFILE:
#ifdef UNICODE
            //(*sfnIp.lpIpGetStringField)(hinfApps, NULL, APPSINF_KEYVAL, lpenh->envProp.achBatchFile, ARRAYSIZE(lpenh->envProp.achBatchFile), NULL);
#else
            (*sfnIp.lpIpGetStringField)(pInfContext, APPSINF_KEYVAL, lpenh->envProp.achBatchFile, ARRAYSIZE(lpenh->envProp.achBatchFile), &dwSize);
#endif
            break;

        case KEYID_LOWMEM:
            if (!lstrcmpi(szVal, g_szAuto))
                lp386->PfW386minmem = 0xFFFF;
            else
                lp386->PfW386minmem = (WORD) _fatoi(szVal);

            if (!szVal2[0])
                lp386->PfW386maxmem = 0xFFFF;
            else
                lp386->PfW386maxmem = (WORD) _fatoi(szVal2);
            break;

        case KEYID_EMSMEM:
            if (!lstrcmpi(szVal, g_szNone)) {
                lp386->PfMaxEMMK = lp386->PfMinEMMK = 0;
            }
            if (!lstrcmpi(szVal, g_szAuto)) {
                lp386->PfMinEMMK = 0;
                lp386->PfMaxEMMK = 0xFFFF;
            }
            else
                lp386->PfMaxEMMK = lp386->PfMinEMMK = (WORD) _fatoi(szVal);

            if (szVal2[0])
                lp386->PfMaxEMMK = (WORD) _fatoi(szVal2);
            break;

        case KEYID_XMSMEM:
            if (!lstrcmpi(szVal, g_szNone)) {
                lp386->PfMaxXmsK = lp386->PfMinXmsK = 0;
            }
            if (!lstrcmpi(szVal, g_szAuto)) {
                lp386->PfMinXmsK = 0;
                lp386->PfMaxXmsK = 0xFFFF;
            }
            else
                lp386->PfMaxXmsK = lp386->PfMinXmsK = (WORD) _fatoi(szVal);

            if (szVal2[0])
                lp386->PfMaxXmsK = (WORD) _fatoi(szVal2);
            break;

        case KEYID_DPMIMEM:
            if (!lstrcmpi(szVal, g_szAuto))
                lpenh->envProp.wMaxDPMI = 0;
            else
                lpenh->envProp.wMaxDPMI = (WORD) _fatoi(szVal);
            break;

        case KEYID_DISABLE:
            bInvert = 0x80;
            // fall into KEYID_ENABLE...

        case KEYID_ENABLE:
            for (i=1; 0 != (j = GetKeyValID(pInfContext, i)); i++)
            {
                int s;
                BYTE b;

                if (j == KEYVAL_ID_UNKNOWN) {
                    ASSERTFAIL();
                    continue;
                }

                if (j == KEYVAL_ID_UNIQUESETTINGS) {
                    continue;
                }

                j--;

                if (j < ARRAYSIZE(abKeyValIDBits)) {

                    b = abKeyValIDBits[j];

                    s = b & 0x3F;
                    b ^= bInvert;
                    if (!(b & 0x80)) {
                        if (!(b & 0x40)) {
                            lp386->PfW386Flags |= 1L << s;
                        }
                        else
                            lp386->PfW386Flags2 |= 1L << s;
                    }
                    else {
                        if (!(b & 0x40))
                            lp386->PfW386Flags &= ~(1L << s);
                        else
                            lp386->PfW386Flags2 &= ~(1L << s);
                    }
                }
                else {
                    j -= ARRAYSIZE(abKeyValIDBits);

                    if (j < ARRAYSIZE(abRMKeyValIDBits)) {

                        b = abRMKeyValIDBits[j];

                        s = b & 0x3F;
                        b ^= bInvert;

                        if (!(b & 0x80))
                            lpenh->dwRealModeFlagsProp |= 1L << s;
                        else
                            lpenh->dwRealModeFlagsProp &= ~(1L << s);
                    }
                    else {
                        j -= ARRAYSIZE(abRMKeyValIDBits);

                        switch(j) {
                        case 0:         // special case 0
                            if (!bInvert)
                                lpstd->MSflags |= EXITMASK;
                            else
                                lpstd->MSflags &= ~EXITMASK;
                            break;

                        case 1:         // special case 1
                            if (bInvert)
                                lpenh->tskProp.flTsk |= TSK_NOSCREENSAVER;
                            else
                                lpenh->tskProp.flTsk &= ~TSK_NOSCREENSAVER;
                            break;

                        default:
                            ASSERTFAIL();
                            break;
                        }
                    }
                }
            }
            break;
        }
    } while ((*sfnIp.lpIpFindNextLine)(pInfContext, pInfContext));

  UnlockPIF:
    ppl->cLocks--;

}


int GetKeyID(PINFCONTEXT pInfContext)
{
    int i;
    TCHAR szCurKey[MAX_KEY_SIZE];
    DWORD dwSize;
    FunctionName(GetKeyID);

    if ((*sfnIp.lpIpGetStringField)(pInfContext, APPSINF_KEY, szCurKey, ARRAYSIZE(szCurKey), &dwSize)) {
        for (i=0; i<ARRAYSIZE(apszKey); i++) {
            if (!lstrcmpi(szCurKey, apszKey[i]))
                return i+1;
        }
        return KEYID_UNKNOWN;
    }
    return KEYID_NONE;
}


int GetKeyValID(PINFCONTEXT pInfContext, int i)
{
    TCHAR szCurKeyVal[MAX_KEYVAL_SIZE];
    DWORD dwSize;
    FunctionName(GetKeyValID);

    if ((*sfnIp.lpIpGetStringField)(pInfContext, i, szCurKeyVal, ARRAYSIZE(szCurKeyVal), &dwSize)) {
        for (i=0; i<ARRAYSIZE(apszKeyVal); i++) {
            if (!lstrcmpi(szCurKeyVal, apszKeyVal[i]))
                return i+1;
        }
        return KEYVAL_ID_UNKNOWN;
    }
    return KEYVAL_ID_NONE;
}




DWORD GetSettingsFlags(PINFCONTEXT pInfContext, LPCTSTR lpszSection)
{
#ifndef WINNT
    DWORD dwSettings = 0;
    DWORD dwSize;

    (*sfnIp.lpIpSaveRestorePosition)(pInfContext, TRUE);
    if (!(*sfnIp.lpIpFindFirstLine)(pInfContext, lpszSection, NULL, NULL)) {
        do {
            switch (GetKeyID(hinfApps)) {
            case KEYID_ENABLE:
                if (!(dwSettings & DAHF_SPECIALSETTINGS)) {
                    int i, j;
                    for (i=1; j = GetKeyValID(pInfContext, i); i++) {
                        switch(j) {
                            case KEYVAL_ID_UNIQUESETTINGS:
                            case KEYVAL_ID_REALMODE:
                            case KEYVAL_ID_WINLIE:
                                dwSettings |= DAHF_SPECIALSETTINGS;
                                goto NextLine;
                        }
                    }
                }
                break;

            case KEYID_APPHACK:
                {
                    TCHAR szKeyVal[20];
                    if ((*sfnIp.lpIpGetStringField)(pInfContext, NULL, 1,
                                    szKeyVal, ARRAYSIZE(szKeyVal), &dwSize)) {
                        dwSettings |= _fatol(szKeyVal);
                    }
                }
                break;
            }
            NextLine: {}
        } while ((*sfnIp.lpIpFindNextLine)(pInfContext, pInfContext));
    }

    (*sfnIp.lpIpSaveRestorePosition)(pInfContext, FALSE);
    return(dwSettings);
#else
    return(0);
#endif
}





BOOL Initialize1AppReg(HWND hwndParent)
{
    BOOL    fSuccess = FALSE;  // Assume failure
    TCHAR   szTmp[MAX_PATH];
    HINF    hinfApps;
    HKEY    hk1App;
    INFCONTEXT InfContext;

    if (!InitSetupxDll()) {
        goto DllLoadFailed;
    }


    hinfApps = (*sfnIp.lpIpOpen)(LoadStringSafe(NULL,
                                            IDS_APPSINF,
                                            szTmp,
                                            ARRAYSIZE(szTmp)),
                               0, INF_STYLE_WIN4, NULL );

    if (hinfApps==INVALID_HANDLE_VALUE) {
        int id = IDS_CANTOPENAPPSINF;
        if (GetLastError()==ERROR_FILE_NOT_FOUND)
            id = IDS_NOAPPSINF;
        Warning(hwndParent, (WORD)id, MB_ICONEXCLAMATION | MB_OK | MB_NOFOCUS);
        goto CloseDLL;
    }

    // OK, now we have APPS.INF open, so let's find [PIF95]

    if (!(*sfnIp.lpIpFindFirstLine)(hinfApps, TEXT("pif95"), NULL, &InfContext)) {
        Warning(hwndParent, IDS_APPSINFERROR, MB_ICONEXCLAMATION | MB_OK | MB_NOFOCUS);
        goto CloseInf;
    }

    // OK, we've found the [pif95] section, so now we'll create the reg branch

    //
    //  Always start with a clean slate!
    //
    RegDeleteKey(HKEY_LOCAL_MACHINE, szRegKeyMSDOSApps);

    if (RegCreateKey(HKEY_LOCAL_MACHINE, szRegKeyMSDOSApps, &hk1App) !=
        ERROR_SUCCESS) {
        goto CloseInf;
    }

    do {
        DWORD   dwSettings;
        DWORD   dwSize;
        TCHAR   szSectionName[MAX_KEY_SIZE];
        if (!(*sfnIp.lpIpGetStringField)(&InfContext, APPSINF_FILENAME,
                        szTmp, ARRAYSIZE(szTmp), &dwSize)) {
            continue;
        }
        (*sfnIp.lpIpGetStringField)(&InfContext, APPSINF_SECTIONID,
                    szSectionName, ARRAYSIZE(szSectionName), &dwSize);
        dwSettings = GetSettingsFlags(&InfContext, szSectionName);
        if (dwSettings) {
            HKEY hkThisApp;
            if (RegCreateKey(hk1App, szTmp, &hkThisApp) == ERROR_SUCCESS) {
                (*sfnIp.lpIpGetStringField)(&InfContext, APPSINF_OTHERFILE,
                            szTmp, ARRAYSIZE(szTmp), &dwSize);
                if (szTmp[0] == 0) {
                    (*sfnIp.lpIpGetStringField)(&InfContext, APPSINF_FILENAME,
                                    szTmp, ARRAYSIZE(szTmp), &dwSize);
                    #ifdef ANNOYEVERYONE
                        MessageBox(hwndParent,
                                   TEXT("No 'other file' specified for this application which requires special settings.  Adding 'other file' of same name."),
                                   szTmp, MB_ICONEXCLAMATION | MB_OK);
                    #endif
                }
                RegSetValueEx(hkThisApp, szTmp, 0, REG_BINARY, (LPSTR)&dwSettings, sizeof(dwSettings));
                RegCloseKey(hkThisApp);
            }
        }
    } while ((*sfnIp.lpIpFindNextLine)(&InfContext, &InfContext));
    fSuccess = TRUE;

    RegCloseKey(hk1App);
CloseInf:
    (*sfnIp.lpIpClose)(hinfApps);
CloseDLL:
    FreeSetupxDll();
DllLoadFailed:
    return(fSuccess);
}



//
//  A RUNDLL entry point that is called at boot time to initialize the registry.
//
void WINAPI InitPIFRegEntries(HWND hwnd, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    Initialize1AppReg(hwnd);
}
