/*
 * Registry Management
 *
 * HTREGMNG.C
 *
 * Copyright (c) 1995 Microsoft Corporation
 *
 */

#include "priv.h"
#include "htregmng.h"
#include "resource.h"
#include "regapix.h"
#include <filetype.h>

#include <advpub.h>
#include <mluisupp.h>

//  This file contains the auto-registration code, which smartly performs 
//  the install/uninstall of registry keys and values.  While an inf file
//  is sufficient most of the time, IE needs to be smart about what
//  sort of values to set, based upon certain conditions.  An inf file
//  does not offer this depth of support.  Additionally, IE requires 
//  code to be run when it detects that it is not the default browser,
//  so it can make it the default browser.  Any settings that determine
//  this should be placed here, rather than the inf file.
//
//  This code is table driven.  The idea is simple.  You have a RegSet
//  which is the "Registry Set".  The Registry Set indicates the 
//  root key and contains a list of RegEntries.  Each RegEntry 
//  specifies a command, flags, key and value names, and optional data
//  that provides the essential info to set/change/delete a registry
//  value or key.
//
//  
// NOTE: NOTE: NOTE: NOTE: NOTE: NOTE: NOTE: 
//-------------------------------------------
// Any new Icon check that uses HTReg_UrlIconProc that gets added
// to any of the Assoc arrays and is REQUIRED for Default Browser check to
// succeed has to be added to the c_rlAssoc_FixIcon[] array also.
//


// Make the tables more compact
#define HKCR    HKEY_CLASSES_ROOT
#define HKLM    HKEY_LOCAL_MACHINE
#define HKCU    HKEY_CURRENT_USER


#define IDEFICON_STD    0
#define IDEFICON_NEWS   1
#define IDEFICON_MAIL   2

#ifndef UNIX

#define IEXPLORE_APP    "IExplore"
#define IEXPLORE_EXE    "iexplore.exe"
#define EXPLORER_EXE    "explorer.exe"
#define RUNDLL_CMD_FMT  "rundll32.exe %s"

#else

#define IEXPLORE_APP    "iexplorer"
#define IEXPLORE_EXE    "iexplorer"
#define EXPLORER_EXE    "explorer"
#define RUNDLL_CMD_FMT  "rundll32 %s"

#endif

BOOL    InstallRegSet(const RegSet *prs, BOOL bDontIntrude);

#ifndef UNIX
const CHAR  c_szIexploreKey[]         = "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE";
#else
const CHAR  c_szIexploreKey[]         = "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORER";
#endif

#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Return a string path composed of hkey\pszKey\pszValueName.

Returns: 
Cond:    --
*/
LPTSTR
Dbg_RegStr(
    IN const RegEntry * pre,          
    IN LPTSTR pszBuf)
{
    TCHAR szRoot[5];
    TCHAR szTempKey[MAXIMUM_SUB_KEY_LENGTH];
    TCHAR szTempValue[MAXIMUM_VALUE_NAME_LENGTH];

    ASSERT(pre);
    ASSERT(pszBuf);

    if (HKEY_CLASSES_ROOT == pre->hkeyRoot)
        {
        StrCpyN(szRoot, TEXT("HKCR"), ARRAYSIZE(szRoot));
        }
    else if (HKEY_CURRENT_USER == pre->hkeyRoot)
        {
        StrCpyN(szRoot, TEXT("HKCU"), ARRAYSIZE(szRoot));
        }
    else if (HKEY_LOCAL_MACHINE == pre->hkeyRoot)
        {
        StrCpyN(szRoot, TEXT("HKLM"), ARRAYSIZE(szRoot));
        }
    else
        {
        StrCpyN(szRoot, TEXT("????"), ARRAYSIZE(szRoot));
        ASSERT(0);
        }

    AnsiToTChar(pre->pszKey, szTempKey, ARRAYSIZE(szTempKey));

    szTempValue[0] = TEXT('\0');
    if (pre->pszValName)
        AnsiToTChar(pre->pszValName, szTempValue, ARRAYSIZE(szTempValue));

    ASSERT(lstrlen(pszBuf) < MAX_PATH);
    wnsprintf(pszBuf, MAX_PATH, TEXT("%s\\%hs\\%hs"), szRoot, szTempKey, szTempValue);

    return pszBuf;
}

#else

#define Dbg_RegStr(x, y)    0

#endif // DEBUG



/*----------------------------------------------------------
Purpose: Queries the registry for the location of the path
         of Internet Explorer and returns it in pszBuf.

Returns: TRUE on success
         FALSE if path cannot be determined

Cond:    --
*/

BOOL
GetIEPath2(
    OUT LPSTR pszBuf,
    IN  DWORD cchBuf,
    IN  BOOL  bInsertQuotes)
{
    BOOL bRet = FALSE;
    HKEY hkey;

    *pszBuf = '\0';

    // Get the path of Internet Explorer 
    if (NO_ERROR != RegOpenKeyA(HKEY_LOCAL_MACHINE, c_szIexploreKey, &hkey))  
    {
#ifndef UNIX
        TraceMsg(TF_ERROR, "InstallRegSet(): RegOpenKey( %s ) Failed", c_szIexploreKey) ;
#endif
    }
    else
    {
        DWORD cbBrowser;
        DWORD dwType;

        if (bInsertQuotes)
            StrCatBuffA(pszBuf, "\"", cchBuf);

        cbBrowser = CbFromCchA(cchBuf - lstrlenA(" -nohome") - 4);
        if (NO_ERROR != SHQueryValueExA(hkey, "", NULL, &dwType,
                                         (LPBYTE)&pszBuf[bInsertQuotes?1:0], &cbBrowser))
        {
            TraceMsg(TF_ERROR, "InstallRegSet(): RegQueryValueEx() for Iexplore path failed");
        }
        else
        {
            bRet = TRUE;
        }

        if (bInsertQuotes)
            StrCatBuffA(pszBuf, "\"", cchBuf);

        RegCloseKey(hkey);
    }

    return bRet;
}

BOOL
GetIEPath(
    OUT LPSTR pszBuf,
    IN  DWORD cchBuf)
{
    return GetIEPath2(pszBuf, cchBuf, TRUE);
}

/*----------------------------------------------------------
Purpose: Queries the registry for the location of the path
         of the shell's Explorer and returns it in pszBuf.

Returns: TRUE on success
         FALSE if path cannot be determined

Cond:    --
*/
BOOL
GetExplorerPath(
    OUT LPSTR pszBuf,
    IN  DWORD cchBuf, DWORD dwType)
{
    BOOL bRet;

    // Get the path of the Explorer 
    if (dwType == REG_EXPAND_SZ) 
        {
        StrCpyNA (pszBuf, "%SystemRoot%", cchBuf);
        bRet = TRUE;
        }
    else
        bRet = (0 < GetWindowsDirectoryA(pszBuf, cchBuf));
    if (bRet)
        {
        StrCatBuffA(pszBuf, "\\Explorer.exe", cchBuf);
        }
    return bRet;
}

// Callback messages

#define RSCB_QUERY          1
#define RSCB_INSTALL        2

typedef BOOL (CALLBACK* RSPECPROC)(UINT nMsg, const RegEntry * pre, LPVOID pvData, DWORD dwData);


// Win9x to NT5 migration generated file.
#define MIGICONS    "migicons.exe"


/*----------------------------------------------------------
Purpose: This callback sets the default icon to point to a
         given index in url.dll.

Returns: varies
Cond:    --
*/
BOOL
CALLBACK
HTReg_UrlIconProc(
    IN UINT       nMsg,
    IN const RegEntry * pre,
    IN LPVOID     pvData,       OPTIONAL
    IN DWORD      dwData)
{
    BOOL bRet = TRUE;
    CHAR sz[MAX_PATH + 20];    // Need a bit extra
    LPCSTR pszPath = (LPCSTR) pvData;
    int cch;
    DWORD dwType;  //local type.
    DEBUG_CODE( TCHAR szDbg[MAX_PATH]; )
    
    ASSERT(RSCB_QUERY == nMsg && pvData ||
           RSCB_INSTALL == nMsg && !pvData);

    if (!g_fRunningOnNT) {
        ASSERT(REG_EXPAND_SZ == pre->dwType);
        dwType = REG_SZ;
    } else
        dwType = (DWORD)pre->dwType;        
    
    if (dwType == REG_EXPAND_SZ) 
        StrCpyNA (sz, "%SystemRoot%\\system32", ARRAYSIZE(sz));
    else
        GetSystemDirectoryA(sz, SIZECHARS(sz));
    cch = lstrlenA(sz);

    // We still have to use url.dll as the source of the internet shortcut
    // icons because the icons need to still be valid on uninstall.
    wnsprintfA(&sz[cch], ARRAYSIZE(sz) - cch, "\\url.dll,%d", (int)pre->DUMMYUNION_MEMBER(lParam));

    switch (nMsg)
    {
    case RSCB_QUERY:
        if (0 != StrCmpNIA(sz, pszPath, dwData / SIZEOF(CHAR)) &&
            0 != StrCmpIA(PathFindFileNameA(sz), PathFindFileNameA(pszPath)))  
        {
            // Failed the Url.Dll test. Check if this is NT5. In that case
            // maybe the icons are in migicons.exe (Win9x to NT5 upgrade).
            if (g_bRunOnNT5 && StrStrIA(pszPath,MIGICONS)!= NULL)
            {    // NT5 and 'migicons.exe' => upgrade. Set global to fix this.
                g_bNT5Upgrade = TRUE;
            }
            else
            {
                TraceMsg(TF_REGCHECK, "IsRegSetInstalled: %s is %hs, expecting %hs", Dbg_RegStr(pre, szDbg), pszPath, sz);
                bRet = FALSE;
            }
        }
        break;

    case RSCB_INSTALL:
        if (NO_ERROR != SHSetValueA(pre->hkeyRoot, pre->pszKey,
                                    pre->pszValName, dwType,
                                    sz, CbFromCchA(lstrlenA(sz) + 1)))
        {
            TraceMsg(TF_ERROR, "InstallRegSet(): SHSetValue(%s) Failed", Dbg_RegStr(pre, szDbg));
            bRet = FALSE;
        }
        else
        {
            DEBUG_CODE( TraceMsg(TF_REGCHECK, "Setting %s", Dbg_RegStr(pre, szDbg)); )
        }
        break;
    }
    return bRet;
}

/*----------------------------------------------------------
Purpose: This callback sets the default icon to point to a
         given index in iexplore.exe
Returns: varies
Cond:    --
*/
BOOL
CALLBACK
HTReg_IEIconProc(
    IN UINT       nMsg,
    IN const RegEntry * pre,
    IN LPVOID     pvData,       OPTIONAL
    IN DWORD      dwData)
{
    BOOL bRet = TRUE;
    CHAR sz[MAX_PATH + 20];    // Need a bit extra
    LPCSTR pszPath = (LPCSTR) pvData;
    int cch;
    DWORD dwType;  //local type.
    DEBUG_CODE( TCHAR szDbg[MAX_PATH]; )
    
    ASSERT(RSCB_QUERY == nMsg && pvData ||
           RSCB_INSTALL == nMsg && !pvData);

    if (!g_fRunningOnNT) {
        // Sanity check that we don't coerce to REG_SZ wrongfully.
        // If you hit this assert, it means the table entry has the
        // wrong type in it.
        ASSERT(REG_EXPAND_SZ == pre->dwType || REG_SZ == pre->dwType);
        dwType = REG_SZ;
    } else
        dwType = (DWORD)pre->dwType;        
    
    if (!GetIEPath2(sz, SIZECHARS(sz), FALSE))
        return FALSE;
        
    cch = lstrlenA(sz);
    wnsprintfA(&sz[cch], ARRAYSIZE(sz) - cch, ",%d", (int)pre->DUMMYUNION_MEMBER(lParam));

    switch (nMsg)
    {
    case RSCB_QUERY:
        if (0 != StrCmpNIA(sz, pszPath, dwData / SIZEOF(CHAR)) &&
            0 != StrCmpA(PathFindFileNameA(sz), PathFindFileNameA(pszPath)))  
        {
            TraceMsg(TF_REGCHECK, "IsRegSetInstalled: %s is %hs, expecting %hs", Dbg_RegStr(pre, szDbg), pszPath, sz);
            bRet = FALSE;
        }
        break;

    case RSCB_INSTALL:
        if (NO_ERROR != SHSetValueA(pre->hkeyRoot, pre->pszKey,
                                    pre->pszValName, dwType,
                                    sz, CbFromCchA(lstrlenA(sz) + 1)))
        {
            TraceMsg(TF_ERROR, "InstallRegSet(): SHSetValue(%s) Failed", Dbg_RegStr(pre, szDbg));
            bRet = FALSE;
        }
        else
        {
            DEBUG_CODE( TraceMsg(TF_REGCHECK, "Setting %s", Dbg_RegStr(pre, szDbg)); )
        }
        break;
    }
    return bRet;
}


/*----------------------------------------------------------
Purpose: This callback sets the IExplore path.

Returns: varies
Cond:    --
*/
BOOL
CALLBACK
HTReg_IEPathProc(
    IN UINT       nMsg,
    IN const RegEntry * pre,
    IN LPVOID     pvData,       OPTIONAL
    IN DWORD      dwData)
{
    BOOL bRet = TRUE;
    CHAR sz[MAX_PATH + 20];    // Need a bit extra
    CHAR szOther[MAX_PATH + 20];    // Need a bit extra
    LPCSTR pszPath = (LPCSTR) pvData;
    int cch;
    DWORD dwType;
    
    DEBUG_CODE( TCHAR szDbg[MAX_PATH]; )
    
    ASSERT(RSCB_QUERY == nMsg && pvData ||
           RSCB_INSTALL == nMsg && !pvData);
    
    ASSERT(REG_EXPAND_SZ == pre->dwType || REG_SZ == pre->dwType);
    
    if (!g_fRunningOnNT)
    {
        // Expand string is not supported on Win95
        dwType = REG_SZ;
    }
    else 
    {
        dwType = pre->dwType;        
    }
    
    if (GetIEPath(sz, SIZECHARS(sz))) {
        // sz contains the path as listed in AppPaths\IExplore.
        // NOTE NOTE: GetIEPath() uses the default value which has no
        // terminating ';'. Hence this check is not needed. Anyway, do it and
        // then convert to other form.
        cch = lstrlenA(sz) - 1;

        if (*sz && sz[cch] == ';')
            sz[cch] = '\0';

        // Convert this to LFN or SFN as the case may be.
        GetPathOtherFormA(sz, szOther, ARRAYSIZE(szOther));

        if (pre->DUMMYUNION_MEMBER(lParam))
        {
            StrCatBuffA(sz, (LPSTR)pre->DUMMYUNION_MEMBER(lParam), ARRAYSIZE(sz));
            StrCatBuffA(szOther, (LPSTR)pre->DUMMYUNION_MEMBER(lParam), ARRAYSIZE(szOther));
        }

        switch (nMsg)
        {
        case RSCB_QUERY:
            if ((0 != StrCmpNIA(pszPath, sz, dwData / SIZEOF(CHAR)))
                && (0 != StrCmpNIA(pszPath, szOther, dwData / SIZEOF(CHAR))))  
            {
                TraceMsg(TF_REGCHECK, "IsRegSetInstalled: %s string is \"%hs\", expecting \"%hs\"", Dbg_RegStr(pre, szDbg), pszPath, sz);
                bRet = FALSE;
            }
            break;

        case RSCB_INSTALL:
            if (NO_ERROR != SHSetValueA(pre->hkeyRoot, pre->pszKey,
                                        pre->pszValName, dwType,
                                        sz, CbFromCchA(lstrlenA(sz) + 1)))
            {
                TraceMsg(TF_ERROR, "InstallRegSet(): SHSetValue(%hs) Failed", pre->pszValName);
                bRet = FALSE;
            }
            else
            {
                DEBUG_CODE( TraceMsg(TF_REGCHECK, "Setting %s", Dbg_RegStr(pre, szDbg)); )
            }
            break;
        }
    }
    
    return bRet;
}


/*----------------------------------------------------------
Purpose: This callback sets the Explorer path.

Returns: varies
Cond:    --
*/
BOOL
CALLBACK
HTReg_ShellPathProc(
    IN UINT       nMsg,
    IN const RegEntry * pre,
    IN LPVOID     pvData,       OPTIONAL
    IN DWORD      dwData)
{

    DWORD dwType;
    BOOL bRet = TRUE;
    CHAR sz[MAX_PATH + 20];    // Need a bit extra
    CHAR szOther[MAX_PATH + 20];    // Need a bit extra
    CHAR szExplorerPath[MAX_PATH+20];
    CHAR szExplorerOtherPath[MAX_PATH+20];
    CHAR szIEPath[MAX_PATH+20]; 
    CHAR szIEOtherPath[MAX_PATH+20]; 
    LPCSTR pszPath = (LPCSTR) pvData;
    BOOL  fPathError = FALSE;

    // If we're in DllInstall() on NT5, we should take the code branch of "BrowseNewProcess == TRUE".  This will write the 
    // correct (IEXPLORE.EXE) values into the registry.
    //
    BOOL  fBrowseNewProcess = (g_bRunOnNT5 && g_bInDllInstall) || IsBrowseNewProcess();
    DEBUG_CODE( TCHAR szDbg[MAX_PATH]; )
    
    ASSERT(RSCB_QUERY == nMsg && pvData ||
           RSCB_INSTALL == nMsg && !pvData);

    if (!g_fRunningOnNT) {
        dwType = REG_SZ;
    } else
        dwType = (DWORD)pre->dwType;        

    *szIEPath = '\0';
    *szIEOtherPath = '\0';
    *szExplorerPath = '\0';
    *szExplorerOtherPath = '\0';

    // Get the paths for IExplore and Explorer in szIePath and szExplorerPath.
    if (!(GetIEPath(szIEPath, SIZECHARS(szIEPath)) && GetExplorerPath(szExplorerPath, SIZECHARS(szExplorerPath), dwType)))
        return FALSE;
        
    GetPathOtherFormA(szIEPath, szIEOtherPath, ARRAYSIZE(szIEOtherPath));
    GetPathOtherFormA(szExplorerPath, szExplorerOtherPath, ARRAYSIZE(szExplorerOtherPath));

    // set sz to point to what the proper path should be based
    // on the current browse new process mode 
    if (fBrowseNewProcess)
    {
        // If we are in browse new process mode, the paths should be to IExplore
        fPathError = GetIEPath(sz, SIZECHARS(sz));
    } else
    {
        // else they can be Explorer for same process browsing.
        fPathError = GetExplorerPath(sz, SIZECHARS(sz), dwType);
    }

    if (fPathError)
    {
        GetPathOtherFormA(sz, szOther, ARRAYSIZE(szOther));

        if (!fBrowseNewProcess && pre->DUMMYUNION_MEMBER(lParam))
        {
            // If Iexplore.exe is our server (BrowseNewProcess true), don't
            // tack on this command line goop required by Explorer.
            // ??? Shouldn't we tack to szExplorerPath too so that the silent
            // fix check works correctly.
            StrCatBuffA(sz, (LPSTR)pre->DUMMYUNION_MEMBER(lParam), ARRAYSIZE(sz));
            StrCatBuffA(szOther, (LPSTR)pre->DUMMYUNION_MEMBER(lParam), ARRAYSIZE(szOther));
        }

        switch (nMsg)
        {
            case RSCB_QUERY:
                // Check given path against both SFN and LFN versions of
                // AppPath. If both fail, means something is wrong.
                if ((0 != StrCmpNIA(pszPath, sz, dwData / SIZEOF(CHAR))) &&  
                    (0 != StrCmpNIA(pszPath, szOther, dwData / SIZEOF(CHAR))) ) 
                {
                    // If BrowseNewProcess mode AND
                    // given path matches ExplorerPath in either one form,
                    // means we have the path set for the wrong mode.
                    if (fBrowseNewProcess && 
                        (0 == StrCmpNIA(pszPath, szExplorerPath, min(lstrlenA(szExplorerPath),(int)(dwData / SIZEOF(CHAR)))) ||
                        0 == StrCmpNIA(pszPath, szExplorerOtherPath, min(lstrlenA(szExplorerOtherPath),(int)(dwData / SIZEOF(CHAR))))))
                    {
                        // The path is set for the wrong mode. Just silently fix it up.
                        SHSetValueA(pre->hkeyRoot, pre->pszKey,
                                    pre->pszValName, dwType,
                                    sz, CbFromCchA(lstrlenA(sz) + 1));
                        goto Cleanup;
                    }

                    // If NOT BrowseNewProcess mode AND
                    // given path matches IEPath in either one form,
                    // means we have the path set for the wrong mode.
                    if (!fBrowseNewProcess && 
                        (0 == StrCmpNIA(pszPath, szIEPath, min(lstrlenA(szIEPath),(int)(dwData / SIZEOF(CHAR)))) ||
                        0 == StrCmpNIA(pszPath, szIEOtherPath, min(lstrlenA(szIEOtherPath),(int)(dwData / SIZEOF(CHAR))))))
                    {
                        // The path is set for the wrong mode. Just silently fix it up.
                        SHSetValueA(pre->hkeyRoot, pre->pszKey,
                                    pre->pszValName, dwType,
                                    sz, CbFromCchA(lstrlenA(sz) + 1));
                        goto Cleanup;
                    }

                    TraceMsg(TF_REGCHECK, "IsRegSetInstalled: %s string is \"%hs\", expecting \"%hs\"", Dbg_RegStr(pre, szDbg), pszPath, sz);                   
                    bRet = FALSE;
                }
                break;

            case RSCB_INSTALL:
                if (NO_ERROR != SHSetValueA(pre->hkeyRoot, pre->pszKey,
                                            pre->pszValName, dwType,
                                            sz, CbFromCchA(lstrlenA(sz) + 1)))
                {
                    TraceMsg(TF_ERROR, "InstallRegSet(): SHSetValue(%hs) Failed", pre->pszValName);
                    bRet = FALSE;
                }
                else
                {
                    DEBUG_CODE( TraceMsg(TF_REGCHECK, "Setting %s", Dbg_RegStr(pre, szDbg)); )
                }
                break;
        }
    }
Cleanup:
    return bRet;
}


/*----------------------------------------------------------
Purpose: This callback checks for the existence of the string
         value "Exchange" at HKLM\Software\Microsoft.  If it
         exists, the value is copied into the default value
         of HKLM\Software\Clients\Mail\Exchange\shell\open\command.

         This is for Athena.  It only happens when setup is run, 
         not when the browser checks to see if it is the default.

Returns: varies
Cond:    --
*/
BOOL
CALLBACK
HTReg_ExchangeProc(
    IN UINT       nMsg,
    IN const RegEntry * pre,
    IN LPVOID     pvData,
    IN DWORD      dwData)
{
    TCHAR sz[MAX_PATH+2];  // +2 because we may need to wrap the path in quotes.
    DWORD cbSize;
    
    switch (nMsg)
    {
    case RSCB_QUERY:
        // We shouldn't be called for this one
        ASSERT(0);      
        break;

    case RSCB_INSTALL:
        // Does the Exchange value exist at "HKLM\Software\Microsoft"?  
        cbSize = sizeof(sz);
        if (NO_ERROR == SHGetValue(HKEY_LOCAL_MACHINE, 
            TEXT("Software\\Microsoft"), TEXT("Exchange"), NULL, sz, &cbSize))
        {
            // Yes; copy it to HKLM\Software\Clients\Mail\Exchange\shell\open\command
            TCHAR szT[MAX_PATH+2];

            // Wrap the path in quotes.  Don't wrap any args though!
            StrCpyN(szT, sz, ARRAYSIZE(szT));
            PathProcessCommand(szT, sz, ARRAYSIZE(szT), PPCF_ADDQUOTES|PPCF_ADDARGUMENTS);

            // Set the size again
            cbSize = CbFromCch(lstrlen(sz)+1);

            SHSetValue(HKEY_LOCAL_MACHINE, 
                TEXT("Software\\Clients\\Mail\\Exchange\\shell\\open\\command"),
                TEXT(""), REG_SZ, sz, cbSize);

            TraceMsg(TF_REGCHECK, "Copying \"%s\" to HKLM\\Software\\Clients\\Mail\\Exchange", sz);

            // Set any other settings in this condition too?
            if (pre->DUMMYUNION_MEMBER(lParam))
                InstallRegSet((const RegSet *)pre->DUMMYUNION_MEMBER(lParam), TRUE);

            // In OSR2 installs, the mailto handler will get out of
            // sync with the actual default mail client.  (Athena installs
            // itself as the default mail client, but exchange remains 
            // the mailto: handler.)  In this case, if exchange is the
            // mailto: handler, change the default mail client to be
            // exchange.

            // Is Exchange the mailto handler?
            cbSize = SIZEOF(sz);
            if (NO_ERROR == SHGetValue(HKEY_CLASSES_ROOT, TEXT("mailto\\shell\\open\\command"),
                                       TEXT(""), NULL, sz, &cbSize) &&
                StrStrI(sz, TEXT("url.dll,MailToProtocolHandler")))
            {
                // Yes; make it be the default mail client too
                SHSetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Clients\\Mail"),
                           TEXT(""), REG_SZ, TEXT("Exchange"), sizeof(TEXT("Exchange")));

                TraceMsg(TF_REGCHECK, "Setting Exchange to be the default mail client.");
            }
        }
        break;
    }
    return TRUE;
}


/*----------------------------------------------------------
Purpose: Uninstall certain keys, as specified by pre->pszKey.

         We do not uninstall a key if the class\shell\open\command
         does not have iexplore.exe.

         If someone else registered themselves to add more
         verbs under class\shell (other than open) or class\shellex,
         then we remove everything but their keys.

Returns: varies
Cond:    --
*/
BOOL
CALLBACK
HTReg_UninstallProc(
    IN UINT       nMsg,
    IN const RegEntry * pre,
    IN LPVOID     pvData,
    IN DWORD      dwData)
{
    TCHAR szKey[MAX_PATH];
    TCHAR sz[MAX_PATH + 20];        // add some padding for arguments
    DWORD cbSize;

    switch (nMsg)
    {
    case RSCB_QUERY:
        // We shouldn't be called for this one
        ASSERT(0);      
        break;

    case RSCB_INSTALL:
        ASSERT(pre->pszKey);

        // Does the shell\open\command value have a microsoft browser?
        wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("%hs\\shell\\open\\command"), pre->pszKey);

        cbSize = sizeof(sz);
        if (NO_ERROR == SHGetValue(pre->hkeyRoot, szKey, TEXT(""),
                                   NULL, sz, &cbSize) &&
            (StrStrI(sz, TEXT(IEXPLORE_EXE)) || StrStrI(sz, TEXT(EXPLORER_EXE))))
        {
            // Yes; proceed to prune this key of all of our values
            TraceMsg(TF_REGCHECK, "Pruning HKCR\\%hs", pre->pszKey);

            ASSERT(pre->DUMMYUNION_MEMBER(lParam));

            InstallRegSet((const RegSet *)pre->DUMMYUNION_MEMBER(lParam), FALSE);
        } 
        break;
    }
    return TRUE;
}


// NOTE: these are ANSI strings by design.

const DWORD c_dwEditFlags2            = FTA_Show;
const CHAR  c_szTelnetHandler[]       = "url.dll,TelnetProtocolHandler %l";
const CHAR  c_szMailToHandler[]       = "url.dll,MailToProtocolHandler %l";
const CHAR  c_szNewsHandler[]         = "url.dll,NewsProtocolHandler %l";
const CHAR  c_szFileHandler[]         = "url.dll,FileProtocolHandler %l";
const CHAR  c_szOpenURL[]             = "url.dll,OpenURL %l";
const CHAR  c_szOpenURLNash[]         = "shdocvw.dll,OpenURL %l";
const CHAR  c_szURL[]                 = "url.dll";
const CHAR  c_szShdocvw[]             = "shdocvw.dll";
const CHAR  c_szCheckAssnSwitch[]     = "Software\\Microsoft\\Internet Explorer\\Main";
const CHAR  c_szDDE_Default[]         = "\"%1\",,-1,0,,,,";
const CHAR  c_szDDE_FileDefault[]     = "\"file://%1\",,-1,,,,,";
const CHAR  c_szViewFolderTemp[]      = "[ViewFolder(\"%l\",\"%l\",%S)]";
const CHAR  c_szViewAppendage[]       = " /idlist,%l,%L";


// BUGBUG (scotth): a lot of the strings below have substrings that 
//  are repeated over and over and over again.  Should add some 
//  smarter RC_ values that will concatenate the common strings
//  together to save data space.

const CHAR c_szHTTP[]                = "http";
const CHAR c_szHTTPDefIcon[]         = "http\\DefaultIcon";
const CHAR c_szHTTPOpenCmd[]         = "http\\shell\\open\\command";
const CHAR c_szHTTPDdeexec[]         = "http\\shell\\open\\ddeexec";
const CHAR c_szHTTPDdeTopic[]        = "http\\shell\\open\\ddeexec\\Topic";
const CHAR c_szHTTPDdeApp[]          = "http\\shell\\open\\ddeexec\\Application";

const CHAR c_szHTTPS[]               = "https";
const CHAR c_szHTTPSDefIcon[]        = "https\\DefaultIcon";
const CHAR c_szHTTPSOpenCmd[]        = "https\\shell\\open\\command";
const CHAR c_szHTTPSDdeexec[]        = "https\\shell\\open\\ddeexec";
const CHAR c_szHTTPSDdeTopic[]       = "https\\shell\\open\\ddeexec\\Topic";
const CHAR c_szHTTPSDdeApp[]         = "https\\shell\\open\\ddeexec\\Application";

const CHAR c_szFTP[]                 = "ftp";
const CHAR c_szFTPDefIcon[]          = "ftp\\DefaultIcon";
const CHAR c_szFTPOpenCmd[]          = "ftp\\shell\\open\\command";
const CHAR c_szFTPDdeexec[]          = "ftp\\shell\\open\\ddeexec";
const CHAR c_szFTPDdeTopic[]         = "ftp\\shell\\open\\ddeexec\\Topic";
const CHAR c_szFTPDdeApp[]           = "ftp\\shell\\open\\ddeexec\\Application";
const CHAR c_szFTPDdeifExec[]        = "ftp\\shell\\open\\ddeexec\\ifExec";

const CHAR c_szGOPHER[]              = "gopher";
const CHAR c_szGOPHERDefIcon[]       = "gopher\\DefaultIcon";
const CHAR c_szGOPHEROpenCmd[]       = "gopher\\shell\\open\\command";
const CHAR c_szGOPHERDdeexec[]       = "gopher\\shell\\open\\ddeexec";
const CHAR c_szGOPHERDdeTopic[]      = "gopher\\shell\\open\\ddeexec\\Topic";
const CHAR c_szGOPHERDdeApp[]        = "gopher\\shell\\open\\ddeexec\\Application";

const CHAR c_szMailTo[]              = "mailto";
const CHAR c_szMailToDefIcon[]       = "mailto\\DefaultIcon";
const CHAR c_szMailToOpenCmd[]       = "mailto\\shell\\open\\command";

const CHAR c_szTelnet[]              = "telnet";
const CHAR c_szTelnetDefIcon[]       = "telnet\\DefaultIcon";
const CHAR c_szTelnetOpenCmd[]       = "telnet\\shell\\open\\command";

const CHAR c_szRLogin[]              = "rlogin";
const CHAR c_szRLoginDefIcon[]       = "rlogin\\DefaultIcon";
const CHAR c_szRLoginOpenCmd[]       = "rlogin\\shell\\open\\command";

const CHAR c_szTN3270[]              = "tn3270";
const CHAR c_szTN3270DefIcon[]       = "tn3270\\DefaultIcon";
const CHAR c_szTN3270OpenCmd[]       = "tn3270\\shell\\open\\command";

const CHAR c_szNews[]                = "news";
const CHAR c_szNewsDefIcon[]         = "news\\DefaultIcon";
const CHAR c_szNewsOpenCmd[]         = "news\\shell\\open\\command";

const CHAR c_szFile[]                = "file";
const CHAR c_szFileDefIcon[]         = "file\\DefaultIcon";
const CHAR c_szFileOpenCmd[]         = "file\\shell\\open\\command";
const CHAR c_szFileDdeexec[]         = "file\\shell\\open\\ddeexec";
const CHAR c_szFileDdeTopic[]        = "file\\shell\\open\\ddeexec\\Topic";
const CHAR c_szFileDdeApp[]          = "file\\shell\\open\\ddeexec\\Application";

const CHAR c_szHTMDefIcon[]          = "htmlfile\\DefaultIcon";
const CHAR c_szHTMShell[]            = "htmlfile\\shell";
const CHAR c_szHTMOpen[]             = "htmlfile\\shell\\open";
const CHAR c_szHTMOpenCmd[]          = "htmlfile\\shell\\open\\command";
const CHAR c_szHTMOpenDdeexec[]      = "htmlfile\\shell\\open\\ddeexec";
const CHAR c_szHTMOpenDdeTopic[]     = "htmlfile\\shell\\open\\ddeexec\\Topic";
const CHAR c_szHTMOpenDdeApp[]       = "htmlfile\\shell\\open\\ddeexec\\Application";

const CHAR c_szMHTMDefIcon[]         = "mhtmlfile\\DefaultIcon";
const CHAR c_szMHTMShell[]           = "mhtmlfile\\shell";
const CHAR c_szMHTMOpen[]            = "mhtmlfile\\shell\\open";
const CHAR c_szMHTMOpenCmd[]         = "mhtmlfile\\shell\\open\\command";
const CHAR c_szMHTMOpenDdeexec[]     = "mhtmlfile\\shell\\open\\ddeexec";
const CHAR c_szMHTMOpenDdeTopic[]    = "mhtmlfile\\shell\\open\\ddeexec\\Topic";
const CHAR c_szMHTMOpenDdeApp[]      = "mhtmlfile\\shell\\open\\ddeexec\\Application";

const CHAR c_szOpenNew[]             = "opennew";
const CHAR c_szHTMOpenNew[]          = "htmlfile\\shell\\opennew";
const CHAR c_szHTMOpenNewCmd[]       = "htmlfile\\shell\\opennew\\command";
const CHAR c_szHTMOpenNewDdeexec[]   = "htmlfile\\shell\\opennew\\ddeexec";
const CHAR c_szHTMOpenNewDdeTopic[]  = "htmlfile\\shell\\opennew\\ddeexec\\Topic";
const CHAR c_szHTMOpenNewDdeApp[]    = "htmlfile\\shell\\opennew\\ddeexec\\Application";

const CHAR c_szMHTMOpenNew[]          = "mhtmlfile\\shell\\opennew";
const CHAR c_szMHTMOpenNewCmd[]       = "mhtmlfile\\shell\\opennew\\command";
const CHAR c_szMHTMOpenNewDdeexec[]   = "mhtmlfile\\shell\\opennew\\ddeexec";
const CHAR c_szMHTMOpenNewDdeTopic[]  = "mhtmlfile\\shell\\opennew\\ddeexec\\Topic";
const CHAR c_szMHTMOpenNewDdeApp[]    = "mhtmlfile\\shell\\opennew\\ddeexec\\Application";

const CHAR c_szIntShcut[]            = "InternetShortcut";
const CHAR c_szIntShcutDefIcon[]     = "InternetShortcut\\DefaultIcon";
const CHAR c_szIntShcutCLSID[]       = "InternetShortcut\\CLSID";
const CHAR c_szIntShcutOpen[]        = "InternetShortcut\\shell\\open";
const CHAR c_szIntShcutOpenCmd[]     = "InternetShortcut\\shell\\open\\command";
const CHAR c_szIntShcutIconHandler[] = "InternetShortcut\\shellex\\IconHandler";
const CHAR c_szIntShcutPrshtHandler[]= "InternetShortcut\\shellex\\PropertySheetHandlers\\{FBF23B40-E3F0-101B-8488-00AA003E56F8}";
const CHAR c_szIntShcutPropHandler[] = "InternetShortcut\\shellex\\PropertyHandler";
const CHAR c_szIntShcutCMHandler[]   = "InternetShortcut\\shellex\\ContextMenuHandlers\\{FBF23B40-E3F0-101B-8488-00AA003E56F8}";

const CHAR c_szCLSIDCmdFile[]       = "{57651662-CE3E-11D0-8D77-00C04FC99D61}";
const CHAR c_szCLSIDIntshcut[]      = "{FBF23B40-E3F0-101B-8488-00AA003E56F8}";
const CHAR c_szCLSIDURLExecHook[]   = "{AEB6717E-7E19-11d0-97EE-00C04FD91972}";

const CHAR c_szIntshcutInproc[]      = "CLSID\\{FBF23B40-E3F0-101B-8488-00AA003E56F8}\\InProcServer32";
const CHAR c_szIEFrameAuto[]         = "CLSID\\{0002DF01-0000-0000-C000-000000000046}\\LocalServer32";
const CHAR c_szIENameSpaceOpen[]     = "CLSID\\{FBF23B42-E3F0-101B-8488-00AA003E56F8}\\shell\\open\\command";
const CHAR c_szCLSIDURLRoot[]        = "CLSID\\{3DC7A020-0ACD-11CF-A9BB-00AA004AE837}";
const CHAR c_szIntshcutMayChange[]   = "CLSID\\{FBF23B40-E3F0-101B-8488-00AA003E56F8}\\shellex\\MayChangeDefaultMenu";

// 
// General associations shared across browser-only and full-shell
//

const RegEntry c_rlAssoc[] = {
    // HTTP
    { RC_ADD,      REF_NOTNEEDED, HKCR, c_szHTTP, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_HTTPNAME) },
    { RC_ADD,      REF_NORMAL,    HKCR, c_szHTTP, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_NORMAL,    HKCR, c_szHTTP, "URL Protocol", REG_SZ, 1, "" },
    { RC_CALLBACK, REF_NOTNEEDED
                   |REF_DONTINTRUDE, HKCR, c_szHTTPDefIcon, "", REG_EXPAND_SZ, IDEFICON_STD, HTReg_UrlIconProc },

    // HTTPS
    { RC_ADD,      REF_NOTNEEDED, HKCR, c_szHTTPS, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_HTTPSNAME) },
    { RC_ADD,      REF_NORMAL,    HKCR, c_szHTTPS, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_NORMAL,    HKCR, c_szHTTPS, "URL Protocol", REG_SZ, 1, "" },
    { RC_CALLBACK, REF_NOTNEEDED|REF_DONTINTRUDE, HKCR, c_szHTTPSDefIcon, "", REG_EXPAND_SZ, IDEFICON_STD, HTReg_UrlIconProc },

    // FTP
    { RC_ADD,      REF_NOTNEEDED, HKCR, c_szFTP, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_FTPNAME) },
    { RC_ADD,      REF_NORMAL,    HKCR, c_szFTP, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_NORMAL,    HKCR, c_szFTP, "URL Protocol", REG_SZ, 1, "" },
    { RC_CALLBACK, REF_NOTNEEDED|REF_DONTINTRUDE, HKCR, c_szFTPDefIcon, "", REG_EXPAND_SZ, IDEFICON_STD, HTReg_UrlIconProc },

    // Gopher
    { RC_ADD,      REF_NOTNEEDED, HKCR, c_szGOPHER, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_GOPHERNAME) },
    { RC_ADD,      REF_NORMAL,    HKCR, c_szGOPHER, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_NORMAL,    HKCR, c_szGOPHER, "URL Protocol", REG_SZ, 1, "" },
    { RC_CALLBACK, REF_NOTNEEDED|REF_DONTINTRUDE, HKCR, c_szGOPHERDefIcon, "", REG_EXPAND_SZ, IDEFICON_STD, HTReg_UrlIconProc },

    // File protocol
    { RC_ADD,      REF_NORMAL,    HKCR, c_szFile, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_FILENAME) },
    { RC_ADD,      REF_NORMAL,    HKCR, c_szFile, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_NORMAL,    HKCR, c_szFile, "URL Protocol", REG_SZ, 1, "" },
    { RC_CALLBACK, REF_NOTNEEDED, HKCR, c_szFileDefIcon, "", REG_EXPAND_SZ, IDEFICON_STD, HTReg_UrlIconProc },

    { RC_RUNDLL,   REF_NORMAL,    HKCR, c_szFileOpenCmd, "", REG_SZ, sizeof(c_szFileHandler), c_szFileHandler },

    // Telnet
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szTelnet, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_TELNETNAME) },
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szTelnet, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szTelnet, "URL Protocol", REG_SZ, 1, "" },
    { RC_CALLBACK, REF_IFEMPTY,   HKCR, c_szTelnetDefIcon, "", REG_EXPAND_SZ, IDEFICON_STD, HTReg_UrlIconProc },
    { RC_RUNDLL,   REF_IFEMPTY,   HKCR, c_szTelnetOpenCmd, "", REG_SZ, sizeof(c_szTelnetHandler), c_szTelnetHandler },

    // RLogin
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szRLogin, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_RLOGINNAME) },
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szRLogin, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szRLogin, "URL Protocol", REG_SZ, 1, "" },
    { RC_CALLBACK, REF_IFEMPTY,   HKCR, c_szRLoginDefIcon, "", REG_EXPAND_SZ, IDEFICON_STD, HTReg_UrlIconProc },
    { RC_RUNDLL,   REF_IFEMPTY,   HKCR, c_szRLoginOpenCmd, "", REG_SZ, sizeof(c_szTelnetHandler), c_szTelnetHandler },

    // TN3270
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szTN3270, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_TN3270NAME) },
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szTN3270, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szTN3270, "URL Protocol", REG_SZ, 1, "" },
    { RC_CALLBACK, REF_IFEMPTY,   HKCR, c_szTN3270DefIcon, "", REG_EXPAND_SZ, IDEFICON_STD, HTReg_UrlIconProc },
    { RC_RUNDLL,   REF_IFEMPTY,   HKCR, c_szTN3270OpenCmd, "", REG_SZ, sizeof(c_szTelnetHandler), c_szTelnetHandler },

    // Mailto protocol
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szMailTo, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_MAILTONAME) },
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szMailTo, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szMailTo, "URL Protocol", REG_SZ, 1, "" },
    { RC_CALLBACK, REF_IFEMPTY,   HKCR, c_szMailToDefIcon, "", REG_EXPAND_SZ, IDEFICON_MAIL, HTReg_UrlIconProc },
    { RC_RUNDLL,   REF_IFEMPTY,   HKCR, c_szMailToOpenCmd, "", REG_SZ, sizeof(c_szMailToHandler), c_szMailToHandler },

    // News protocol
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szNews, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_NEWSNAME) },
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szNews, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_IFEMPTY,   HKCR, c_szNews, "URL Protocol", REG_SZ, 1, "" },
    { RC_CALLBACK, REF_IFEMPTY,   HKCR, c_szNewsDefIcon, "", REG_EXPAND_SZ, IDEFICON_NEWS, HTReg_UrlIconProc },
    { RC_RUNDLL,   REF_IFEMPTY,   HKCR, c_szNewsOpenCmd, "", REG_SZ, sizeof(c_szNewsHandler), c_szNewsHandler },

    // Internet shortcut
    { RC_ADD,      REF_NORMAL,      HKCR, ".url", "", REG_SZ, sizeof(c_szIntShcut), c_szIntShcut },
    { RC_ADD,      REF_NORMAL,      HKCR, c_szIntShcut, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_INTSHNAME) },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szIntShcut, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_NORMAL,      HKCR, c_szIntShcut, "IsShortcut", REG_SZ, 1, "" },
    { RC_ADD,      REF_NORMAL,      HKCR, c_szIntShcut, "NeverShowExt", REG_SZ, 1, "" },
    { RC_ADD,      REF_NORMAL,      HKCR, c_szIntShcutCLSID, "", REG_SZ, sizeof(c_szCLSIDIntshcut), c_szCLSIDIntshcut },
    { RC_CALLBACK, REF_DONTINTRUDE, HKCR, c_szIntShcutDefIcon, "", REG_EXPAND_SZ, IDEFICON_STD, HTReg_UrlIconProc },
    { RC_ADD,      REF_NORMAL,      HKCR, c_szIntShcutIconHandler, "", REG_SZ, sizeof(c_szCLSIDIntshcut), c_szCLSIDIntshcut },
    { RC_ADD,      REF_NORMAL,      HKCR, c_szIntShcutPrshtHandler, "", REG_SZ, 1, "" },
    { RC_ADD,      REF_NORMAL,      HKCR, "CLSID\\{FBF23B40-E3F0-101B-8488-00AA003E56F8}", "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_INTSHNAME) },
    { RC_ADD,      REF_NORMAL,      HKCR, c_szIntshcutInproc, "", REG_SZ, sizeof(c_szShdocvw), c_szShdocvw },
    { RC_ADD,      REF_NORMAL,      HKCR, c_szIntshcutInproc, "ThreadingModel", REG_SZ, sizeof("Apartment"), "Apartment" },
    { RC_ADD,      REF_NORMAL,      HKCR, c_szIntshcutInproc, "LoadWithoutCOM", REG_SZ, 1, ""},

    // HTM file type
    { RC_CALLBACK,      REF_NOTNEEDED,   HKCR, c_szHTMDefIcon, "", REG_SZ, (LPARAM)1, HTReg_IEIconProc },

    // MHTML file type
    { RC_CALLBACK,      REF_NOTNEEDED,   HKCR, c_szMHTMDefIcon, "", REG_SZ, (LPARAM)22, HTReg_IEIconProc },
};


const RegSet c_rsAssoc = {
    ARRAYSIZE(c_rlAssoc),
    c_rlAssoc
};


//
// .htm, .html associations for full-shell and browser-only installs
// 

// This is run when the browser is opened, and considered a requirement
// to make IE be the default browser.
#ifdef UNIX
const RegEntry c_rlAssocHTM[] = {
#else
const RegList c_rlAssocHTM = {
#endif
    // .html
    { RC_ADD,      REF_DONTINTRUDE, HKCR, ".htm", "", REG_SZ, sizeof("htmlfile"), "htmlfile" },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, ".htm", "Content Type", REG_SZ, sizeof("text/html"), "text/html" },

    { RC_ADD,      REF_DONTINTRUDE, HKCR, ".html", "", REG_SZ, sizeof("htmlfile"), "htmlfile" },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, ".html", "Content Type", REG_SZ, sizeof("text/html"), "text/html" },
};

const RegSet c_rsAssocHTM = {
    ARRAYSIZE(c_rlAssocHTM),
    c_rlAssocHTM
};


// This is needed just to insure webview works.
//
const RegEntry c_rlAssocHTM_WV[] = {
    // .html
    { RC_ADD,      REF_IFEMPTY, HKCR, ".htm", "", REG_SZ, sizeof("htmlfile"), "htmlfile" },
    { RC_ADD,      REF_IFEMPTY, HKCR, ".htm", "Content Type", REG_SZ, sizeof("text/html"), "text/html" },

    { RC_ADD,      REF_IFEMPTY, HKCR, ".html", "", REG_SZ, sizeof("htmlfile"), "htmlfile" },
    { RC_ADD,      REF_IFEMPTY, HKCR, ".html", "Content Type", REG_SZ, sizeof("text/html"), "text/html" },
};


const RegSet c_rsAssocHTM_WV = {
    ARRAYSIZE(c_rlAssocHTM_WV),
    c_rlAssocHTM_WV
};


//
// Browser-only specific association settings
// 

const RegEntry c_rlAssoc_Alone[] = {
    // HTTP
    { RC_CALLBACK, REF_DONTINTRUDE,  HKCR, c_szHTTPOpenCmd,   "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPDdeexec,   "", REG_SZ, sizeof(c_szDDE_Default), c_szDDE_Default },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPDdeApp,    "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPDdeTopic,  "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    // HTTPS
    { RC_CALLBACK, REF_DONTINTRUDE,  HKCR, c_szHTTPSOpenCmd,   "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPSDdeexec,   "", REG_SZ, sizeof(c_szDDE_Default), c_szDDE_Default },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPSDdeApp,    "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPSDdeTopic,  "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    // FTP
    { RC_CALLBACK, REF_DONTINTRUDE,  HKCR, c_szFTPOpenCmd, "", REG_SZ, (LPARAM)" %1", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szFTPDdeexec, "", REG_SZ, sizeof(c_szDDE_Default), c_szDDE_Default },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szFTPDdeApp, "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szFTPDdeTopic, "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szFTPDdeifExec, "", REG_SZ, sizeof(""), "" },

    // Gopher
    { RC_CALLBACK, REF_DONTINTRUDE,  HKCR, c_szGOPHEROpenCmd, "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szGOPHERDdeexec, "", REG_SZ, sizeof(c_szDDE_Default), c_szDDE_Default },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szGOPHERDdeApp, "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szGOPHERDdeTopic, "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    // .htm
    //
    //  APPCOMPAT:
    //  HTMOpenCmd needs to be REG_SZ because Office97 reads it out using RegQueryValue.
    //  WebFerret requires the string to be of type REG_SZ.
    { RC_CALLBACK, REF_DONTINTRUDE,  HKCR, c_szHTMOpenCmd, "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTMOpenDdeexec, "", REG_SZ, sizeof(c_szDDE_FileDefault), c_szDDE_FileDefault },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTMOpenDdeApp, "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTMOpenDdeTopic, "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    // .mht, .mhtml
    { RC_CALLBACK, REF_DONTINTRUDE,  HKCR, c_szMHTMOpenCmd, "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szMHTMOpenDdeexec, "", REG_SZ, sizeof(c_szDDE_FileDefault), c_szDDE_FileDefault },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szMHTMOpenDdeApp, "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szMHTMOpenDdeTopic, "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    // Internet shortcut
    { RC_ADD,      REF_NORMAL, HKCR, c_szIntShcutCMHandler, "", REG_SZ, 1, "" },

    // Other stuff
    { RC_RUNDLL,   REF_NORMAL,       HKCR, c_szIntShcutOpenCmd, "", REG_SZ, sizeof(c_szOpenURLNash), c_szOpenURLNash },
    { RC_CALLBACK, REF_NORMAL,       HKCR, c_szIEFrameAuto, "", REG_SZ, 0, HTReg_IEPathProc },
    { RC_CALLBACK, REF_NORMAL,       HKCR, c_szIENameSpaceOpen, "", REG_SZ, 0, HTReg_IEPathProc },
};

const RegSet c_rsAssoc_Alone = {
    ARRAYSIZE(c_rlAssoc_Alone),
    c_rlAssoc_Alone
};


// The reg entries for the browser only case for http, https, and ftp are duplicated here
const RegEntry c_rlAssoc_Quick[] = {
    // HTTP
    { RC_CALLBACK, REF_DONTINTRUDE,  HKCR, c_szHTTPOpenCmd,   "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPDdeexec,   "", REG_SZ, sizeof(c_szDDE_Default), c_szDDE_Default },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPDdeApp,    "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPDdeTopic,  "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    // HTTPS
    { RC_CALLBACK, REF_DONTINTRUDE,  HKCR, c_szHTTPSOpenCmd,   "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPSDdeexec,   "", REG_SZ, sizeof(c_szDDE_Default), c_szDDE_Default },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPSDdeApp,    "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szHTTPSDdeTopic,  "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    // FTP
    { RC_CALLBACK, REF_DONTINTRUDE,  HKCR, c_szFTPOpenCmd, "", REG_SZ, (LPARAM)" %1", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szFTPDdeexec, "", REG_SZ, sizeof(c_szDDE_Default), c_szDDE_Default },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szFTPDdeApp, "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szFTPDdeTopic, "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szFTPDdeifExec, "", REG_SZ, sizeof(""), "" },

};

const RegSet c_rsAssoc_Quick = {
    ARRAYSIZE(c_rlAssoc_Quick),
    c_rlAssoc_Quick
};




//
// Full-shell specific association settings
// 

const RegEntry c_rlAssoc_Full[] = {
    // HTTP
    { RC_CALLBACK, REF_DONTINTRUDE, HKCR, c_szHTTPOpenCmd, "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szHTTPDdeexec, "", REG_SZ, sizeof(c_szDDE_Default), c_szDDE_Default },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szHTTPDdeexec, "NoActivateHandler", REG_SZ, 1, "" },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szHTTPDdeApp, "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szHTTPDdeTopic, "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    // HTTPS
    { RC_CALLBACK, REF_DONTINTRUDE, HKCR, c_szHTTPSOpenCmd, "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szHTTPSDdeexec, "", REG_SZ, sizeof(c_szDDE_Default), c_szDDE_Default },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szHTTPSDdeexec, "NoActivateHandler", REG_SZ, 1, "" },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szHTTPSDdeApp, "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szHTTPSDdeTopic, "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    // FTP
    { RC_CALLBACK, REF_DONTINTRUDE, HKCR, c_szFTPOpenCmd, "", REG_SZ, (LPARAM)" %1", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szFTPDdeexec, "", REG_SZ, sizeof(c_szDDE_Default), c_szDDE_Default },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szFTPDdeexec, "NoActivateHandler", REG_SZ, 1, "" },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szFTPDdeApp, "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szFTPDdeTopic, "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szFTPDdeifExec, "", REG_SZ, sizeof(""), "" },

    // Gopher
    { RC_CALLBACK, REF_DONTINTRUDE, HKCR, c_szGOPHEROpenCmd, "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szGOPHERDdeexec, "", REG_SZ, sizeof(c_szDDE_Default), c_szDDE_Default },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szGOPHERDdeexec, "NoActivateHandler", REG_SZ, 1, "" },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szGOPHERDdeApp, "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szGOPHERDdeTopic, "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    // File protocol
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szFileDdeexec, "", REG_SZ, sizeof(c_szViewFolderTemp), c_szViewFolderTemp },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szFileDdeexec, "NoActivateHandler", REG_SZ, 1, "" },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szFileDdeApp, "", REG_SZ, sizeof("Folders"), "Folders" },
    { RC_ADD,      REF_DONTINTRUDE, HKCR, c_szFileDdeTopic, "", REG_SZ, sizeof("AppProperties"), "AppProperties" },

    // .htm
    //
    //  APPCOMPAT:
    //  HTMOpenCmd needs to be REG_SZ because Office97 reads it out using RegQueryValue.
    //  WebFerret requires the string to be of type REG_SZ.
    //  Visual Source Safe reads the Ddeexec string, puts a file in the %1 (NOT %l!),
    //  and performs a dde transaction, so we are pretty much stuck with the "file:%1,,-1,,,,," string now.
    //
    { RC_ADD,      REF_NORMAL, HKCR, c_szHTMShell, "", REG_SZ, sizeof(c_szOpenNew), c_szOpenNew },

    { RC_ADD,      REF_NORMAL, HKCR, c_szHTMOpen, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_OPENSAME)},
    { RC_CALLBACK, REF_NORMAL, HKCR, c_szHTMOpenCmd, "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_NORMAL, HKCR, c_szHTMOpenDdeexec, "", REG_SZ, sizeof(c_szDDE_FileDefault), c_szDDE_FileDefault },
    { RC_ADD,      REF_NORMAL, HKCR, c_szHTMOpenDdeexec, "NoActivateHandler", REG_SZ, 1, "" },
    { RC_ADD,      REF_NORMAL, HKCR, c_szHTMOpenDdeApp, "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_NORMAL, HKCR, c_szHTMOpenDdeTopic, "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    { RC_ADD,      REF_NORMAL, HKCR, c_szMHTMShell, "", REG_SZ, sizeof(c_szOpenNew), c_szOpenNew },
   
    { RC_ADD,      REF_NORMAL, HKCR, c_szHTMOpenNew, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_OPEN)},
    { RC_CALLBACK, REF_NORMAL, HKCR, c_szHTMOpenNewCmd, "", REG_SZ, (LPARAM)c_szViewAppendage, HTReg_ShellPathProc },
    { RC_ADD,      REF_NORMAL, HKCR, c_szHTMOpenNewDdeexec, "", REG_SZ, sizeof(c_szViewFolderTemp), c_szViewFolderTemp },
    { RC_ADD,      REF_NORMAL, HKCR, c_szHTMOpenNewDdeexec, "NoActivateHandler", REG_SZ, 1, "" },
    { RC_ADD,      REF_NORMAL, HKCR, c_szHTMOpenNewDdeApp, "", REG_SZ, sizeof("Folders"), "Folders" },
    { RC_ADD,      REF_NORMAL, HKCR, c_szHTMOpenNewDdeTopic, "", REG_SZ, sizeof("AppProperties"), "AppProperties" },

    // .mht, .mhtml
    { RC_ADD,      REF_NORMAL, HKCR, c_szMHTMOpen, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_OPENSAME)},
    { RC_CALLBACK, REF_DONTINTRUDE,  HKCR, c_szMHTMOpenCmd, "", REG_SZ, (LPARAM)" -nohome", HTReg_IEPathProc },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szMHTMOpenDdeexec, "", REG_SZ, sizeof(c_szDDE_FileDefault), c_szDDE_FileDefault },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szMHTMOpenDdeApp, "", REG_SZ, sizeof(IEXPLORE_APP), IEXPLORE_APP },
    { RC_ADD,      REF_DONTINTRUDE,  HKCR, c_szMHTMOpenDdeTopic, "", REG_SZ, sizeof("WWW_OpenURL"), "WWW_OpenURL" },

    { RC_ADD,      REF_NORMAL, HKCR, c_szMHTMOpenNew, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_OPEN)},
    { RC_CALLBACK, REF_NORMAL, HKCR, c_szMHTMOpenNewCmd, "", REG_SZ, (LPARAM)c_szViewAppendage, HTReg_ShellPathProc },
    { RC_ADD,      REF_NORMAL, HKCR, c_szMHTMOpenNewDdeexec, "", REG_SZ, sizeof(c_szViewFolderTemp), c_szViewFolderTemp },
    { RC_ADD,      REF_NORMAL, HKCR, c_szMHTMOpenNewDdeexec, "NoActivateHandler", REG_SZ, 1, "" },
    { RC_ADD,      REF_NORMAL, HKCR, c_szMHTMOpenNewDdeApp, "", REG_SZ, sizeof("Folders"), "Folders" },
    { RC_ADD,      REF_NORMAL, HKCR, c_szMHTMOpenNewDdeTopic, "", REG_SZ, sizeof("AppProperties"), "AppProperties" },

    // Internet shortcut
    { RC_ADD,      REF_NORMAL, HKCR, c_szCLSIDURLRoot, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_THEINTERNET) },
    { RC_RUNDLL,   REF_NORMAL, HKCR, c_szIntShcutOpenCmd, "", REG_SZ, sizeof(c_szOpenURLNash), c_szOpenURLNash },
    { RC_ADD,      REF_NORMAL, HKCR, c_szIntShcutOpen, "CLSID", REG_SZ, sizeof(c_szCLSIDIntshcut), c_szCLSIDIntshcut },
    { RC_ADD,      REF_NORMAL, HKCR, c_szIntShcutOpen, "LegacyDisable", REG_SZ, 1, ""},
    { RC_ADD,      REF_NORMAL, HKCR, c_szIntShcutCMHandler, "", REG_SZ, 1, "" },
    { RC_ADD,      REF_NORMAL, HKCR, c_szIntshcutMayChange, "", REG_SZ, 1, "" },
    { RC_ADD,      REF_NORMAL, HKCR, c_szIntShcutPropHandler, "", REG_SZ, sizeof(c_szCLSIDIntshcut), c_szCLSIDIntshcut },

    //  add ourselves to the applications key
    { RC_CALLBACK, REF_NORMAL, HKCR, "Applications\\iexplore.exe\\shell\\open\\command", "", REG_SZ, (LPARAM)" ""%1""", HTReg_IEPathProc},

    // Other stuff
    { RC_CALLBACK, REF_NORMAL, HKCR, c_szIEFrameAuto, "", REG_SZ, 0, HTReg_IEPathProc },
};


const RegSet c_rsAssoc_Full = {
    ARRAYSIZE(c_rlAssoc_Full),
    c_rlAssoc_Full
};


//
// On upgrading from Win9x to NT5, the icons are shifted to newly created
// file called "migicons.exe". This breaks our Assoc checks. Hence this is
// a list of all HTReg_UrlIconProc checks from the various Assoc arrays that
// MUST BELONG TO US FOR US TO BE DEFAULT BROWSER (REF_NOTNEEDED and
// REF_IFEMPTY ==> not used for check purposes). 
// This list is used to fix the icons.
//
// NOTE: NOTE: NOTE: NOTE: NOTE: NOTE: NOTE: 
// Any new Icon check that uses HTReg_UrlIconProc that gets added
// to any of the Assoc arrays and is REQUIRED for Default Browser check to
// succeed has to be added here also.
//

const RegEntry c_rlAssoc_FixIcon[] = {
    // Icon checks from c_rlAssoc that are essential for us to be Default
    // Browser
    { RC_CALLBACK, REF_DONTINTRUDE, HKCR, c_szIntShcutDefIcon, "", REG_EXPAND_SZ, IDEFICON_STD, HTReg_UrlIconProc }
};

const RegSet c_rsAssoc_FixIcon = {
    ARRAYSIZE(c_rlAssoc_FixIcon),
    c_rlAssoc_FixIcon
};


// 
// General browser-only settings
//

const CHAR c_szCLSIDMIME[]           = "{FBF23B41-E3F0-101B-8488-00AA003E56F8}";
const CHAR c_szIEOnDesktop[]         = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace\\{FBF23B42-E3F0-101B-8488-00AA003E56F8}";
const CHAR c_szShellExecHook[]       = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellExecuteHooks";
const CHAR c_szFileTypesHook[]       = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileTypesPropertySheetHook";

const RegEntry c_rlGeneral_Alone[] = {
    { RC_ADD,   REF_NORMAL, HKLM, c_szIEOnDesktop,   "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_THEINTERNET) },
    { RC_ADD,   REF_NORMAL, HKLM, c_szFileTypesHook, "", REG_SZ, sizeof(c_szCLSIDMIME), c_szCLSIDMIME },

    // URL Exec Hook (this replaces the old overloaded intshcut CLSID)
    { RC_DEL,   REF_NORMAL, HKLM, c_szShellExecHook, c_szCLSIDIntshcut, REG_SZ, 1, "" },
    { RC_ADD,   REF_NORMAL, HKLM, c_szShellExecHook, c_szCLSIDURLExecHook, REG_SZ, 1, "" },
    { RC_ADD,   REF_NORMAL, HKCR, "CLSID\\{AEB6717E-7E19-11d0-97EE-00C04FD91972}", "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_URLEXECHOOK) },
    { RC_ADD,   REF_NORMAL, HKCR, "CLSID\\{AEB6717E-7E19-11d0-97EE-00C04FD91972}\\InProcServer32", "", REG_SZ, sizeof("url.dll"), "url.dll" },
    { RC_ADD,   REF_NORMAL, HKCR, "CLSID\\{AEB6717E-7E19-11d0-97EE-00C04FD91972}\\InProcServer32", "ThreadingModel", REG_SZ, sizeof("Apartment"), "Apartment" },
};

const RegSet c_rsGeneral_Alone = {
    ARRAYSIZE(c_rlGeneral_Alone),
    c_rlGeneral_Alone
};


//
// General full-shell only settings
//

const RegEntry c_rlGeneral_Full[] = {
    { RC_DEL,   REF_NORMAL, HKLM, c_szIEOnDesktop,        "", REG_SZ, 0, NULL },
    { RC_DEL,   REF_NUKE,   HKLM, c_szFileTypesHook,      "", REG_SZ, sizeof(c_szCLSIDMIME), c_szCLSIDMIME },

    // URL Exec Hook (this replaces the old overloaded intshcut CLSID)
    { RC_DEL,   REF_NORMAL, HKLM, c_szShellExecHook, c_szCLSIDIntshcut, REG_SZ, 1, "" },
    { RC_ADD,   REF_NORMAL, HKLM, c_szShellExecHook, c_szCLSIDURLExecHook, REG_SZ, 1, "" },
    { RC_ADD,   REF_NORMAL, HKCR, "CLSID\\{AEB6717E-7E19-11d0-97EE-00C04FD91972}", "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_URLEXECHOOK) },
    { RC_ADD,   REF_NORMAL, HKCR, "CLSID\\{AEB6717E-7E19-11d0-97EE-00C04FD91972}\\InProcServer32", "", REG_SZ, sizeof("shell32.dll"), "shell32.dll" },
    { RC_ADD,   REF_NORMAL, HKCR, "CLSID\\{AEB6717E-7E19-11d0-97EE-00C04FD91972}\\InProcServer32", "ThreadingModel", REG_SZ, sizeof("Apartment"), "Apartment" },
};

const RegSet c_rsGeneral_Full = {
    ARRAYSIZE(c_rlGeneral_Full),
    c_rlGeneral_Full
};


/*
 * S P E C I A L   D Y N A M I C   S E T T I N G S 
 *
 */

#define SZ_EXMAILTO     "Software\\Clients\\Mail\\Exchange\\Protocols\\Mailto"

const RegEntry c_rlExchange[] = {
    { RC_ADD,      REF_NORMAL,    HKLM, "Software\\Clients\\Mail\\Exchange", "", REG_SZ, 0, MAKEINTRESOURCE(IDS_EXCHANGE) },

    { RC_ADD,      REF_IFEMPTY,   HKLM, SZ_EXMAILTO, "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_MAILTONAME) },
    { RC_ADD,      REF_IFEMPTY,   HKLM, SZ_EXMAILTO, "EditFlags", REG_DWORD, sizeof(c_dwEditFlags2), &c_dwEditFlags2 },
    { RC_ADD,      REF_IFEMPTY,   HKLM, SZ_EXMAILTO, "URL Protocol", REG_SZ, 1, "" },
    { RC_CALLBACK, REF_IFEMPTY,   HKLM, SZ_EXMAILTO "\\DefaultIcon", "", REG_EXPAND_SZ, IDEFICON_MAIL, HTReg_UrlIconProc },
    { RC_RUNDLL,   REF_IFEMPTY,   HKLM, SZ_EXMAILTO "\\Shell\\Open\\Command", "", REG_SZ, sizeof(c_szMailToHandler), c_szMailToHandler },
};

const RegSet c_rsExchange = {
    ARRAYSIZE(c_rlExchange),
    c_rlExchange
};

#ifdef UNIX
const RegEntry c_rlAthena[] = {
#else
const RegList c_rlAthena = {
#endif
   { RC_CALLBACK, REF_NORMAL, HKLM, "", "", REG_SZ, (LPARAM)&c_rsExchange, HTReg_ExchangeProc },
};

const RegSet c_rsAthena = {
    ARRAYSIZE(c_rlAthena),
    c_rlAthena
};


/*
 * U N I N S T A L L   S E T T I N G S
 *
 */


// Protocol-specific uninstall (for both full-shell and browser-only)

const RegEntry c_rlUnHTTP[] = {
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTTP, "URL Protocol", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTTPDefIcon, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTTPDdeApp, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTTPDdeTopic, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTTPDdeexec, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_PRUNE,     HKCR, c_szHTTPOpenCmd, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_EDITFLAGS, HKCR, c_szHTTP, "", REG_SZ, 0, NULL },
};

const RegSet c_rsUnHTTP = {
    ARRAYSIZE(c_rlUnHTTP),
    c_rlUnHTTP
};

#ifdef UNIX
const RegEntry c_rlUnHTTPS[] = {
#else
const RegList c_rlUnHTTPS = {
#endif
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTTPS, "URL Protocol", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTTPSDefIcon, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTTPSDdeApp, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTTPSDdeTopic, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTTPSDdeexec, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_PRUNE,     HKCR, c_szHTTPSOpenCmd, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_EDITFLAGS, HKCR, c_szHTTPS, "", REG_SZ, 0, NULL },
};

const RegSet c_rsUnHTTPS = {
    ARRAYSIZE(c_rlUnHTTPS),
    c_rlUnHTTPS
};

#ifdef UNIX
const RegEntry c_rlUnFTP[] = { 
#else  
const RegList c_rlUnFTP = {
#endif
    { RC_DEL, REF_NORMAL,    HKCR, c_szFTP, "URL Protocol", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szFTPDefIcon, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szFTPDdeApp, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szFTPDdeTopic, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szFTPDdeexec, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_PRUNE,     HKCR, c_szFTPOpenCmd, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_EDITFLAGS, HKCR, c_szFTP, "", REG_SZ, 0, NULL },
};

const RegSet c_rsUnFTP = {
    ARRAYSIZE(c_rlUnFTP),
    c_rlUnFTP
};

#ifdef UNIX
const RegEntry c_rlUnGopher[] = {
#else
const RegList c_rlUnGopher = {
#endif
    { RC_DEL, REF_NORMAL,    HKCR, c_szGOPHER, "URL Protocol", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szGOPHERDefIcon, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szGOPHERDdeApp, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szGOPHERDdeTopic, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szGOPHERDdeexec, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_PRUNE,     HKCR, c_szGOPHEROpenCmd, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_EDITFLAGS, HKCR, c_szGOPHER, "", REG_SZ, 0, NULL },
};

const RegSet c_rsUnGopher = {
    ARRAYSIZE(c_rlUnGopher),
    c_rlUnGopher
};

#ifdef UNIX 
const RegEntry c_rlUnHTM[] = {
#else
const RegList c_rlUnHTM = {
#endif
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTMDefIcon, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTMOpenDdeApp, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTMOpenDdeTopic, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL,    HKCR, c_szHTMOpenDdeexec, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_PRUNE,     HKCR, c_szHTMOpenCmd, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_EDITFLAGS, HKCR, "htmlfile", "", REG_SZ, 0, NULL },

    { RC_DEL, REF_NORMAL,    HKCR, c_szMHTMDefIcon, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_EDITFLAGS, HKCR, "mhtmlfile", "", REG_SZ, 0, NULL },
};

const RegSet c_rsUnHTM = {
    ARRAYSIZE(c_rlUnHTM),
    c_rlUnHTM
};


// Protocol-specific uninstall for full-shell

#ifdef UNIX
const RegEntry c_rlUnHTTP_Full[] = {
#else
const RegList c_rlUnHTTP_Full = {
#endif
    { RC_DEL, REF_NORMAL, HKCR, c_szHTTPDdeexec, "NoActivateHandler", REG_SZ, 0, NULL }
};

const RegSet c_rsUnHTTP_Full = {
    ARRAYSIZE(c_rlUnHTTP_Full),
    c_rlUnHTTP_Full
};

#ifdef UNIX
const RegEntry c_rlUnHTTPS_Full[] = {
#else
const RegList c_rlUnHTTPS_Full = {
#endif
    { RC_DEL, REF_NORMAL, HKCR, c_szHTTPSDdeexec, "NoActivateHandler", REG_SZ, 0, NULL }
};

const RegSet c_rsUnHTTPS_Full = {
    ARRAYSIZE(c_rlUnHTTPS_Full),
    c_rlUnHTTPS_Full
};

#ifdef UNIX
const RegEntry c_rlUnFTP_Full[] = {
#else
const RegList c_rlUnFTP_Full = {
#endif
    { RC_DEL, REF_NORMAL, HKCR, c_szFTPDdeexec, "NoActivateHandler", REG_SZ, 0, NULL },
};

const RegSet c_rsUnFTP_Full = {
    ARRAYSIZE(c_rlUnFTP_Full),
    c_rlUnFTP_Full
};

#ifdef UNIX
const RegEntry c_rlUnGopher_Full[] = {
#else
const RegList c_rlUnGopher_Full = {
#endif
    { RC_DEL, REF_NORMAL, HKCR, c_szGOPHERDdeexec, "NoActivateHandler", REG_SZ, 0, NULL },
};

const RegSet c_rsUnGopher_Full = {
    ARRAYSIZE(c_rlUnGopher_Full),
    c_rlUnGopher_Full
};

#ifdef UNIX
const RegEntry c_rlUnHTM_Full[] = {
#else
const RegList c_rlUnHTM_Full = {
    // remove the default context menu items
#endif
    { RC_DEL, REF_NORMAL, HKCR, c_szHTMShell, NULL, REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL, HKCR, c_szMHTMShell, NULL, REG_SZ, 0, NULL },

    // remove the default values
    { RC_DEL, REF_NORMAL, HKCR, c_szHTMOpenNew, NULL, REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL, HKCR, c_szMHTMOpenNew, NULL, REG_SZ, 0, NULL },

    { RC_DEL, REF_NORMAL, HKCR, c_szHTMOpenNewDdeApp, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL, HKCR, c_szHTMOpenNewDdeTopic, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL, HKCR, c_szHTMOpenNewDdeexec, "NoActivateHandler", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL, HKCR, c_szHTMOpenNewDdeexec, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_PRUNE,  HKCR, c_szHTMOpenNewCmd, "", REG_SZ, 0, NULL },

    { RC_DEL, REF_NORMAL, HKCR, c_szMHTMOpenNewDdeApp, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL, HKCR, c_szMHTMOpenNewDdeTopic, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL, HKCR, c_szMHTMOpenNewDdeexec, "NoActivateHandler", REG_SZ, 0, NULL },
    { RC_DEL, REF_NORMAL, HKCR, c_szMHTMOpenNewDdeexec, "", REG_SZ, 0, NULL },
    { RC_DEL, REF_PRUNE,  HKCR, c_szMHTMOpenNewCmd, "", REG_SZ, 0, NULL },
};

const RegSet c_rsUnHTM_Full = {
    ARRAYSIZE(c_rlUnHTM_Full),
    c_rlUnHTM_Full
};


//
// Browser-only uninstall
//

#ifdef UNIX
const RegEntry c_rlUninstall_Alone[] = {
#else
const RegList c_rlUninstall_Alone = {
#endif
    { RC_DEL,      REF_NORMAL, HKLM, c_szIEOnDesktop,   "", REG_SZ, 0, MAKEINTRESOURCE(IDS_REG_THEINTERNET) },
    { RC_DEL,      REF_PRUNE,  HKCR, c_szIENameSpaceOpen,   "", REG_SZ, 0, NULL },

    // InternetShortcut
    { RC_DEL,    REF_NORMAL, HKCR, c_szIntShcutOpen, "CLSID", REG_SZ, sizeof(c_szCLSIDIntshcut), c_szCLSIDIntshcut },
    { RC_DEL,    REF_PRUNE,  HKCR, c_szIntShcutCMHandler, "", REG_SZ, 1, "" },
    { RC_DEL,    REF_PRUNE,  HKCR, c_szIntshcutMayChange, "", REG_SZ, 1, "" },
    { RC_DEL,    REF_PRUNE,  HKCR, c_szIntShcutPropHandler, "", REG_SZ, sizeof(c_szCLSIDIntshcut), c_szCLSIDIntshcut },

    // Change the inprocserver after removing "MayChangeDefaultMenu" above.  
    // Do this so url.dll doesn't repatch the registry.
    { RC_ADD,    REF_NORMAL, HKCR, c_szIntshcutInproc, "", REG_SZ, sizeof(c_szURL), c_szURL },
    { RC_RUNDLL, REF_NORMAL, HKCR, c_szIntShcutOpenCmd, "", REG_SZ, sizeof(c_szOpenURL), c_szOpenURL },

    { RC_CALLBACK, REF_NORMAL, HKCR, "http",     "", PLATFORM_BROWSERONLY, (LPARAM)&c_rsUnHTTP, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "https",    "", PLATFORM_BROWSERONLY, (LPARAM)&c_rsUnHTTPS, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "ftp",      "", PLATFORM_BROWSERONLY, (LPARAM)&c_rsUnFTP, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "gopher",   "", PLATFORM_BROWSERONLY, (LPARAM)&c_rsUnGopher, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "htmlfile", "", PLATFORM_BROWSERONLY, (LPARAM)&c_rsUnHTM, HTReg_UninstallProc },
};

const RegSet c_rsUninstall_Alone = {
    ARRAYSIZE(c_rlUninstall_Alone),
    c_rlUninstall_Alone
};


// 
// Full-shell uninstall
//

#ifdef UNIX
const RegEntry c_rlUninstall_Full[] = {
#else
const RegList c_rlUninstall_Full = {
#endif
    // InternetShortcut
    { RC_DEL,    REF_NORMAL, HKCR, c_szIntShcutOpen, "CLSID", REG_SZ, sizeof(c_szCLSIDIntshcut), c_szCLSIDIntshcut },
    { RC_DEL,    REF_NORMAL, HKCR, c_szIntShcutOpen, "LegacyDisable", REG_SZ, 1, ""},
    { RC_DEL,    REF_PRUNE,  HKCR, c_szIntShcutCMHandler, "", REG_SZ, 1, "" },
    { RC_DEL,    REF_PRUNE,  HKCR, c_szIntshcutMayChange, "", REG_SZ, 1, "" },
    { RC_DEL,    REF_PRUNE,  HKCR, c_szIntShcutPropHandler, "", REG_SZ, sizeof(c_szCLSIDIntshcut), c_szCLSIDIntshcut },

    // Change the inprocserver after removing "MayChangeDefaultMenu" above.  
    // Do this so url.dll doesn't repatch the registry.
    { RC_ADD,    REF_NORMAL, HKCR, c_szIntshcutInproc, "", REG_SZ, sizeof(c_szURL), c_szURL },
    { RC_RUNDLL, REF_NORMAL, HKCR, c_szIntShcutOpenCmd, "", REG_SZ, sizeof(c_szOpenURL), c_szOpenURL },

    // File protocol
    { RC_DEL,    REF_NORMAL, HKCR, c_szFileDdeApp, "", REG_SZ, 0, NULL },
    { RC_DEL,    REF_NORMAL, HKCR, c_szFileDdeTopic, "", REG_SZ, 0, NULL },
    { RC_DEL,    REF_NORMAL, HKCR, c_szFileDdeexec, "NoActivateHandler", REG_SZ, 0, NULL },
    { RC_DEL,    REF_PRUNE,  HKCR, c_szFileDdeexec, "", REG_SZ, 0, NULL },

    // Protocol associations
    { RC_CALLBACK, REF_NORMAL, HKCR, "http",      "", PLATFORM_INTEGRATED, (LPARAM)&c_rsUnHTTP_Full, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "http",      "", PLATFORM_INTEGRATED, (LPARAM)&c_rsUnHTTP, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "https",     "", PLATFORM_INTEGRATED, (LPARAM)&c_rsUnHTTPS_Full, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "https",     "", PLATFORM_INTEGRATED, (LPARAM)&c_rsUnHTTPS, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "ftp",       "", PLATFORM_INTEGRATED, (LPARAM)&c_rsUnFTP_Full, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "ftp",       "", PLATFORM_INTEGRATED, (LPARAM)&c_rsUnFTP, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "gopher",    "", PLATFORM_INTEGRATED, (LPARAM)&c_rsUnGopher_Full, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "gopher",    "", PLATFORM_INTEGRATED, (LPARAM)&c_rsUnGopher, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "htmlfile",  "", PLATFORM_INTEGRATED, (LPARAM)&c_rsUnHTM_Full, HTReg_UninstallProc },
    { RC_CALLBACK, REF_NORMAL, HKCR, "htmlfile",  "", PLATFORM_INTEGRATED, (LPARAM)&c_rsUnHTM, HTReg_UninstallProc },
};

const RegSet c_rsUninstall_Full = {
    ARRAYSIZE(c_rlUninstall_Full),
    c_rlUninstall_Full
};



/*
 *   D E F A U L T    S E T    O F    R E G   S E T S
 *
 */


// Common association settings for both browser-only and full-shell

// This is the minimum set that is queried every time a shell window opens.  
// WARNING: this should be small to reduce the time it takes to open a folder.

const RegSet * const g_rgprsWebview[] = {
    &c_rsAssocHTM_WV,
};

// This is the minimum set that is queried every time a browser window
// opens (but not an explorer window).


// This is the required set of entries to make IE be the default 
// browser.  Only used if the user hasn't turned this off.

const RegSet * const g_rgprsDefault[] = {
    &c_rsAssoc,
    &c_rsAssocHTM,
};

// Browser-only specific associations

const RegSet * const g_rgprsDefault_Alone[] = {
    &c_rsAssoc_Alone,
};

// Browser-only specific associations for a quick check
const RegSet * const g_rgprsDefault_Quick[] = {
    &c_rsAssoc_Quick,
};


// Full-shell specific associations

const RegSet * const g_rgprsDefault_Full[] = {
    &c_rsAssoc_Full,
};

// This is the set of icon entries that need to be fixed in case of a 
// Win9x to NT5 upgrade.

const RegSet * const g_rgprsDefault_FixIcon[] = {
    &c_rsAssoc_FixIcon,
};



//
// Other registry settings
//

const RegSet * const g_rgprsIE30Only[] = 
    {
    &c_rsGeneral_Alone,
    &c_rsAthena,
    };


const RegSet * const g_rgprsNashOnly[] = 
    {
    &c_rsGeneral_Full,
    &c_rsAthena,
    };


const RegSet * const g_rgprsUninstallIE30[] = 
    {
    &c_rsUninstall_Alone,
    };


const RegSet * const g_rgprsUninstallNash[] = 
    {
    &c_rsUninstall_Full,
    };


/*----------------------------------------------------------
Purpose: Determine if a particular RegSet is installed

Returns: 
Cond:    --
*/
BOOL
IsRegSetInstalled( 
    IN const RegSet * prs)
    {
    BOOL        bRet = FALSE;
    UINT        i;
    HKEY        hkey;
    const RegEntry * pre;
    CHAR        szBuffer[1024];         // Registry Data Holder
    CHAR        szT[MAX_PATH + 20]; // Need a bit extra for pszIExpAppendage
    DWORD       dwType;
    DWORD       dwSize;
    DWORD       dwSizeExpect;
    DEBUG_CODE( TCHAR szDbg[MAX_PATH]; )

    // Check each registry entry.  Stop when we encounter the first
    // entry which doesn't match (no need to waste time looking at
    // other entries).
    //
    // In the debug build, we enumerate the whole list, so we can
    // see all the differences at once.
    //

#ifdef DEBUG
    #define BAIL_OUT    bRet = TRUE; continue
#else
    #define BAIL_OUT    goto Bail
#endif


    for (i = 0; i < prs->cre; i++)  
        {
        pre = &(prs->pre[i]);

        // Is this regentry not needed, or can it be set by some third 
        // party?
        if (IsFlagSet(pre->dwFlags, REF_NOTNEEDED))
            {
            // Yes; skip to next
            continue;
            }

        // Does the key exist?
        if (NO_ERROR != RegOpenKeyA(pre->hkeyRoot, pre->pszKey, &hkey))  
        {
            // No; should it?
            if (RC_DEL == pre->regcmd)
            {
                // No; skip to next
                continue;
            }
            else
            {
                // Yes
                DEBUG_CODE( TraceMsg(TF_REGCHECK, "%s doesn't exist and should", Dbg_RegStr(pre, szDbg)); )
                BAIL_OUT;
            }
        }
        // Yes; should it?
        else if (RC_DEL == pre->regcmd && !*pre->pszValName)
        {
            // No
            DEBUG_CODE( TraceMsg(TF_REGCHECK, "%s exists and shouldn't", Dbg_RegStr(pre, szDbg)); )
            RegCloseKey(hkey);
            BAIL_OUT;
        }

        // Does the value exist?
        dwSize = SIZEOF(szBuffer);
        if (NO_ERROR != RegQueryValueExA(hkey, pre->pszValName, NULL, 
                                         &dwType, (BYTE *)szBuffer, &dwSize))  
        {
            // No; should it?
            if (RC_DEL != pre->regcmd)
            {
                // Yes
                TraceMsg(TF_REGCHECK, "IsRegSetInstalled: RegQueryValueEx( %hs, %hs ) Failed", pre->pszKey, pre->pszValName);
                RegCloseKey(hkey);
                BAIL_OUT;
            }
        }
        // Yes; should it?
        else if (RC_DEL == pre->regcmd)
        {
            // No
            ASSERT(pre->pszValName && *pre->pszValName);

            DEBUG_CODE( TraceMsg(TF_REGCHECK, "%s exists and shouldn't", 
                                 Dbg_RegStr(pre, szDbg)); )
            RegCloseKey(hkey);
            BAIL_OUT;
        }
        RegCloseKey(hkey);

        // Is this a value that cannot be stomped (ie, a 3rd party might have
        // set its value, and that's okay with us)?
        if (IsFlagSet(pre->dwFlags, REF_IFEMPTY))
            {
            // Yes; the existence of the value is good enough for us,
            // skip to next
            continue;
            }

        switch (pre->regcmd)  
            {
        case RC_ADD:
        case RC_RUNDLL:
            if (dwType == REG_SZ)  
            {
                LPCVOID pvValue;

                // Is this a resource string?
                if (0 == HIWORD64(pre->pvValue))
                {
                    // Yes; load it 
                    dwSizeExpect = LoadStringA(g_hinst, PtrToUlong(pre->pvValue), szT, SIZECHARS(szT));

                    // Add null and convert to bytes
                    dwSizeExpect = CbFromCchA(dwSizeExpect + 1);
                    pvValue = szT;
                }
                else
                {
                    // No
                    ASSERT(pre->pvValue);

                    if (RC_RUNDLL == pre->regcmd)
                    {
                        wnsprintfA(szT, ARRAYSIZE(szT), RUNDLL_CMD_FMT, (LPSTR)pre->pvValue);
                        pvValue = szT;

                        // Add null and convert to bytes
                        dwSizeExpect = CbFromCchA(lstrlenA(szT) + 1);
                    }
                    else
                    {
                        pvValue = pre->pvValue;

                        if (0 == pre->DUMMYUNION_MEMBER(dwSize))
                            dwSizeExpect = (DWORD)CbFromCchA(lstrlenA((LPCSTR)pvValue) + 1);
                        else
                            dwSizeExpect = (DWORD)pre->DUMMYUNION_MEMBER(dwSize);
                    }
                }

                if (dwSize != dwSizeExpect)
                {
                    TraceMsg(TF_REGCHECK, "IsRegSetInstalled: %s string size is %d, expecting %d", Dbg_RegStr(pre, szDbg), dwSize, dwSizeExpect);
                    BAIL_OUT;
                }

                // Compare case-insensitive (otherwise we'd just use 
                // memcmp below)
                if (0 != StrCmpNIA((LPSTR)pvValue, szBuffer, dwSize / SIZEOF(CHAR)))  
                    {
                    TraceMsg(TF_REGCHECK, "IsRegSetInstalled: %s string is \"%hs\", expecting \"%hs\"", Dbg_RegStr(pre, szDbg), szBuffer, pvValue);
                    BAIL_OUT;
                    }
                } 
            else 
                {
                // Non-string case
                if (dwSize != pre->DUMMYUNION_MEMBER(dwSize))  
                    {
                    TraceMsg(TF_REGCHECK, "IsRegSetInstalled: %s size is %d, expecting %d", Dbg_RegStr(pre, szDbg), dwSize, pre->DUMMYUNION_MEMBER(dwSize));
                    BAIL_OUT;
                    }

                if (0 != memcmp(pre->pvValue, (BYTE *)szBuffer, dwSize))  
                    {
                    TraceMsg(TF_REGCHECK, "IsRegSetInstalled: %s value is different, expecting %#08x", Dbg_RegStr(pre, szDbg), *(LPDWORD)pre->pvValue);
                    BAIL_OUT;
                    }
                }
            break;

        case RC_CALLBACK:
        {
            RSPECPROC pfn = (RSPECPROC)pre->pvValue;

            ASSERT(IS_VALID_CODE_PTR(pfn, RSPECPROC));

            // If the callback returns false, it means we're not the
            // default browser.
            if ( !pfn(RSCB_QUERY, pre, szBuffer, dwSize) )
                BAIL_OUT;
            break;
        }

        case RC_DEL:
            // Work is done before the switch statement.  Do nothing here.
            break;

        default:
            ASSERT(0);
            TraceMsg(TF_ERROR, "IsRegSetInstalled: Unhandled Special Type");
            break;
            }
        }

#ifdef DEBUG
    // In the debug build, leaving the above loop with bRet == TRUE means
    // something doesn't match, so we need to flip the boolean value.
    bRet ^= TRUE;
#else
    bRet = TRUE;

Bail:
#endif
    return bRet;
    }
                    

/*----------------------------------------------------------
Purpose: Returns TRUE if the key is empty of all subkeys and
         all (non-default) values.

         If dwFlags has REF_EDITFLAGS set, then this function 
         ignores the EditFlags value.

Returns: see above
Cond:    --
*/
BOOL
IsKeyPsuedoEmpty(
    IN HKEY   hkey,
    IN LPCSTR pszSubKey,
    IN DWORD  dwFlags)          // REF_ flags
{
    BOOL bRet = FALSE;
    DWORD dwRet;
    HKEY hkeyNew;

    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, KEY_READ, &hkeyNew);
    if (NO_ERROR == dwRet)
    {
        DWORD ckeys;
        DWORD cvalues;

        // Are the any subkeys?
        if (NO_ERROR == RegQueryInfoKey(hkeyNew, NULL, NULL, NULL, &ckeys,
                                        NULL, NULL, &cvalues, NULL, NULL,
                                        NULL, NULL) &&
            0 == ckeys)
        {
            // No; how about non-default values?
            DWORD dwRetDef = SHGetValueA(hkey, pszSubKey, "", NULL, NULL, NULL);

            bRet = (0 == cvalues || (1 == cvalues && NO_ERROR == dwRetDef));

            // Should we ignore edit flags?
            if (!bRet && IsFlagSet(dwFlags, REF_EDITFLAGS))
            {
                // Yes
                DWORD dwRetEdit = SHGetValueA(hkey, pszSubKey, "EditFlags", NULL, NULL, NULL);

                bRet = ((1 == cvalues && NO_ERROR == dwRetEdit) || 
                        (2 == cvalues && NO_ERROR == dwRetEdit && 
                         NO_ERROR == dwRetDef));
            }
        }
        RegCloseKey(hkeyNew);
    }
    return bRet;
}


/*----------------------------------------------------------
Purpose: Prune the key of our keys and values.  Walk back up
         the tree and delete empty keys below us.

Returns: 
Cond:    --
*/
void
PruneKey(
    IN HKEY    hkeyRoot,
    IN LPCSTR  pszKey)
{
    CHAR szPath[MAX_PATH];

    ASSERT(hkeyRoot);
    ASSERT(pszKey);

    StrCpyNA(szPath, pszKey, ARRAYSIZE(szPath));

    while (PathRemoveFileSpecA(szPath) && *szPath)
    {
        SHDeleteOrphanKeyA(hkeyRoot, szPath);
    }
}


/*----------------------------------------------------------
Purpose: Install a regset (set of registry entries).

         If bDontIntrude is TRUE , then behave such that any
         REF_DONTINTRUDE entry is not forcefully installed (i.e., it
         will only get installed if the key doesn't already
         have a value in it).  

Returns: 
Cond:    --
*/
BOOL
InstallRegSet( 
    IN const RegSet *prs,
    IN BOOL  bDontIntrude)
    {
    BOOL        bRet = TRUE;
    UINT        i;
    HKEY        hkey;
    const RegEntry * pre;
    CHAR        szBuffer[MAX_PATH + 20];    // Need additional space for pszIExpAppendage
    DWORD       dwSize;
    LPCVOID     pvValue;
    DEBUG_CODE( TCHAR szDbg[MAX_PATH]; )

    /*
     * Install each registry entry
     */
    for (i = 0; i < prs->cre; i++)  
        {
        pre = &(prs->pre[i]);

        // Stomp on this value?
        if (bDontIntrude && IsFlagSet(pre->dwFlags, REF_DONTINTRUDE))
            continue;

        if (IsFlagSet(pre->dwFlags, REF_IFEMPTY))
        {
            // No
            if (NO_ERROR == RegOpenKeyA(pre->hkeyRoot, pre->pszKey, &hkey))
            {
                BOOL bSkip;

                // Are we checking the default value?
                if (0 == *pre->pszValName)
                {
                    // Yes; check the size, because default values 
                    // always exist with at least a null terminator.
                    dwSize = 0;
                    RegQueryValueExA(hkey, pre->pszValName, NULL, NULL, NULL, &dwSize);
                    bSkip = (1 < dwSize);
                }
                else
                {
                    // No
                    bSkip = (NO_ERROR == RegQueryValueExA(hkey, pre->pszValName, 
                                            NULL, NULL, NULL, NULL));
                }

                RegCloseKey(hkey);

                // Does it exist?
                if (bSkip)
                {               
                    // Yes; skip it
                    DEBUG_CODE( TraceMsg(TF_REGCHECK, "%s already exists, skipping", 
                                         Dbg_RegStr(pre, szDbg)); )
                    continue;
                }
            }
        }

        switch (pre->regcmd)  
        {
        case RC_ADD:
        case RC_RUNDLL:
            if (NO_ERROR != RegCreateKeyA(pre->hkeyRoot, pre->pszKey, &hkey))  
            {
                TraceMsg(TF_ERROR, "InstallRegSet(): RegCreateKey(%hs) Failed", pre->pszKey);
                bRet = FALSE;
            }
            else
            {
                // Is the value a resource string? 
                if (REG_SZ == pre->dwType && 0 == HIWORD64(pre->pvValue))
                {
                    UINT idRes = PtrToUlong(pre->pvValue);
                    // Yes; load it
                    dwSize = LoadStringA(g_hinst, idRes, szBuffer, SIZECHARS(szBuffer));

                    // Add null and convert to bytes
                    dwSize = CbFromCchA(dwSize + 1);     
                    pvValue = szBuffer;
                }
                else
                {
                    // No
                    if (RC_RUNDLL == pre->regcmd)
                    {
                        ASSERT(pre->pvValue);
                        ASSERT(REG_SZ == pre->dwType);

                        wnsprintfA(szBuffer, ARRAYSIZE(szBuffer), RUNDLL_CMD_FMT, (LPSTR)pre->pvValue);
                        pvValue = szBuffer;
                        dwSize = CbFromCchA(lstrlenA(szBuffer) + 1);
                    }
                    else
                    {
                        // Normal case
                        pvValue = pre->pvValue;

                        if (0 == pre->DUMMYUNION_MEMBER(dwSize) && REG_SZ == pre->dwType)
                            dwSize = CbFromCchA(lstrlenA((LPCSTR)pvValue) + 1);
                        else
                            dwSize = pre->DUMMYUNION_MEMBER(dwSize);
                    }
                }

                ASSERT(0 < dwSize);

                if (NO_ERROR != RegSetValueExA(hkey, pre->pszValName, 0, pre->dwType, (BYTE*)pvValue, dwSize))  
                {
                    TraceMsg(TF_ERROR, "InstallRegSet(): RegSetValueEx(%hs) Failed", pre->pszValName );
                    bRet = FALSE;
                }
                else
                {
                    DEBUG_CODE( TraceMsg(TF_REGCHECK, "Setting %s", Dbg_RegStr(pre, szDbg)); )
                }
                RegCloseKey(hkey);
            }
            break;

        case RC_CALLBACK:
        {
            RSPECPROC pfn = (RSPECPROC)pre->pvValue;

            ASSERT(IS_VALID_CODE_PTR(pfn, RSPECPROC));

            pfn(RSCB_INSTALL, pre, NULL, 0);
            break;
        }

        case RC_DEL:
            // Delete the default value, a named value, or the key? 
            if (pre->pszValName == NULL)
            {
                // Default value
                DEBUG_CODE( TraceMsg(TF_REGCHECK, "Deleting default value %s", Dbg_RegStr(pre, szDbg)); )

                SHDeleteValueA(pre->hkeyRoot, pre->pszKey, pre->pszValName);
            }
            else if (*pre->pszValName)
            {
                // Named value
                DEBUG_CODE( TraceMsg(TF_REGCHECK, "Deleting value %s", Dbg_RegStr(pre, szDbg)); )

                SHDeleteValueA(pre->hkeyRoot, pre->pszKey, pre->pszValName);
            }
            else
            {
                // Key
                if (IsFlagSet(pre->dwFlags, REF_NUKE))
                {
                    DEBUG_CODE( TraceMsg(TF_REGCHECK, "Deleting key %s", Dbg_RegStr(pre, szDbg)); )

                    SHDeleteKeyA(pre->hkeyRoot, pre->pszKey);
                }
                // If there are keys or values (other than the
                // default value) that are set, then we don't want
                // to delete either the default value or the
                // key.
                else if (IsKeyPsuedoEmpty(pre->hkeyRoot, pre->pszKey, pre->dwFlags))
                {
                    // Delete the default value so SHDeleteOrphanKey 
                    // will work
                    SHDeleteValueA(pre->hkeyRoot, pre->pszKey, "");

                    // Delete the EditFlags value?  (Without the EditFlags,
                    // the user will not be able to specify associations
                    // for this class in the FileTypes dialog, b/c that
                    // dialog requires this value.  So the rule is, this
                    // function will delete the EditFlags if there is
                    // nothing else in the key.)
                    if (IsFlagSet(pre->dwFlags, REF_EDITFLAGS))
                    {
                        DEBUG_CODE( TraceMsg(TF_REGCHECK, "Deleting %s\\EditFlags", Dbg_RegStr(pre, szDbg)); )
                        
                        SHDeleteValueA(pre->hkeyRoot, pre->pszKey, "EditFlags");
                    }

                    DEBUG_CODE( TraceMsg(TF_REGCHECK, "Deleting empty key %s", Dbg_RegStr(pre, szDbg)); )
                    
                    SHDeleteOrphanKeyA(pre->hkeyRoot, pre->pszKey);

                    // Should we prune?  (This mean we'll walk back up
                    // the tree and try deleting empty keys that lead
                    // to this key.)
                    if (IsFlagSet(pre->dwFlags, REF_PRUNE))
                        PruneKey(pre->hkeyRoot, pre->pszKey);
                }
            }
            break;

        default:
            ASSERT(0);
            TraceMsg(TF_ERROR, "InstallRegSet(): Unhandled Special Case");
            break;
            }
        }

    return bRet;
    }


/****************************************************************************

    FUNCTION: CenterWindow (HWND, HWND)

    PURPOSE:  Center one window over another

    COMMENTS:

    Dialog boxes take on the screen position that they were designed at,
    which is not always appropriate. Centering the dialog over a particular
    window usually results in a better position.

****************************************************************************/
BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
    RECT    rChild, rParent;
    int     wChild, hChild, wParent, hParent;
    int     wScreen, hScreen, xNew, yNew;
    HDC     hdc;

    // Get the Height and Width of the child window
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    // Get the Height and Width of the parent window
    GetWindowRect (hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    // Get the display limits
    hdc = GetDC (hwndChild);
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC (hwndChild, hdc);

    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0) {
        xNew = 0;
    } else if ((xNew+wChild) > wScreen) {
        xNew = wScreen - wChild;
    }

    // Calculate new Y position, then adjust for screen
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < 0) {
        yNew = 0;
    } else if ((yNew+hChild) > hScreen) {
        yNew = hScreen - hChild;
    }

    // Set it, and return
    return SetWindowPos (hwndChild, NULL,
        xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}


/*----------------------------------------------------------
Purpose: Dialog proc 

*/
BOOL
CALLBACK
AssociationDialogProc(HWND hdlg, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
    BOOL bMsgHandled = FALSE;

    /* uMsg may be any value. */
    /* wparam may be any value. */
    /* lparam may be any value. */

    switch (uMsg){
    case WM_INITDIALOG:
        CenterWindow( hdlg, GetDesktopWindow());

        if (g_bRunOnNT5)
        {
            // Initialize Checkbox
            // uncheck by default for the first time we show this dialog, 
            // user's action is required. we still persist user's last choice.
            if (FALSE == SHRegGetBoolUSValue(REGSTR_PATH_MAIN, TEXT("ShowedCheckBrowser"), 
                                     FALSE, FALSE)) 
            {
                Button_SetCheck(GetDlgItem(hdlg, IDC_ASSOC_CHECK), FALSE);

                  // mark we have showed this dialog once.
                LPTSTR sz = TEXT("Yes");
                SHRegSetUSValue(REGSTR_PATH_MAIN, TEXT("ShowedCheckBrowser"), REG_SZ, 
                    (LPBYTE)sz, CbFromCch(lstrlen(sz)+1), SHREGSET_HKCU | SHREGSET_FORCE_HKCU);

            }
            else
            {
                Button_SetCheck(GetDlgItem(hdlg, IDC_ASSOC_CHECK), IsCheckAssociationsOn());
            }
        }
        else
            Button_SetCheck(GetDlgItem(hdlg, IDC_ASSOC_CHECK), IsCheckAssociationsOn());

        bMsgHandled  = TRUE;
        break;

    //
    // MSN mucks with the registry in a way that causes IE to ask if it's the
    // default browser.  Then after they launch IE they maximize the active
    // window.  Since the default browsre dialog is active, it gets maximized.
    // Handeling the WM_GETMINMAXINFO prevents this dialog from maximizing.
    //
    case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpmmi = (LPMINMAXINFO)lparam;

            if (lpmmi)
            {
                RECT rc;

                if (GetWindowRect(hdlg, &rc))
                {
                    lpmmi->ptMaxSize.x = rc.right - rc.left;
                    lpmmi->ptMaxSize.y = rc.bottom - rc.top;

                    lpmmi->ptMaxPosition.x = rc.left;
                    lpmmi->ptMaxPosition.y = rc.top;

                    bMsgHandled = TRUE;
                }
            }
        }
        break;


    case WM_COMMAND:
        switch (LOWORD(wparam))  {
        case IDYES:
        case IDNO:
            SetCheckAssociations( Button_GetCheck(GetDlgItem(hdlg, IDC_ASSOC_CHECK)) );
            EndDialog( hdlg, LOWORD(wparam));
            break;

        }

    default:
        break;
    }
    return(bMsgHandled);
}


/*----------------------------------------------------------
Purpose: Asks the user whether to make IE be the default browser

*/
BOOL 
AskUserShouldFixReg()
{
    return IDYES == DialogBox(MLGetHinst(),
                              MAKEINTRESOURCE(IDD_ASSOC),
                              NULL,
                              (DLGPROC)AssociationDialogProc);
}


HRESULT InstallFTPAssociations(void)
{
    IFtpInstaller * pfi;
    HRESULT hr = CoCreateInstance(CLSID_FtpInstaller, NULL, CLSCTX_INPROC_SERVER, IID_IFtpInstaller, (void **) &pfi);

    if (SUCCEEDED(hr))
    {
        hr = pfi->MakeIEDefautlFTPClient();
        pfi->Release();
    }
    else
    {
        // This may fail to create if FTP wasn't installed, which is
        // a valid install case.
        hr = S_OK;
    }

    return hr;
}


/*----------------------------------------------------------
Purpose: Install file and protocol association settings in
         registry.

*/
HRESULT
InstallRegAssoc(
    UINT nInstall,          // One of PLATFORM_*
    BOOL bDontIntrude)     // TRUE: be non-intrusive
{
    int i;

    // Install associations common across both IE and Nashville

    for (i = 0; i < ARRAYSIZE(g_rgprsDefault); i++)
        InstallRegSet(g_rgprsDefault[i], bDontIntrude);

    if (PLATFORM_UNKNOWN == nInstall)
    {
        nInstall = WhichPlatform();
    }

    switch (nInstall)
    {
    case PLATFORM_BROWSERONLY:
        for (i = 0; i < ARRAYSIZE(g_rgprsDefault_Alone); i++)
            InstallRegSet(g_rgprsDefault_Alone[i], bDontIntrude);
        break;

    case PLATFORM_INTEGRATED:
        for (i = 0; i < ARRAYSIZE(g_rgprsDefault_Full); i++)
            InstallRegSet(g_rgprsDefault_Full[i], bDontIntrude);
        break;

    default:
        ASSERT(0);
        break;
    }

    InstallFTPAssociations();

    // Notify shell that the associations have changed.
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    return NOERROR;
}


/*----------------------------------------------------------
Purpose: Set the CheckAssocation setting in the registry

*/
void
SetCheckAssociations( 
    BOOL fCheck)
{
    HKEY    hk;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, c_szCheckAssnSwitch, 0, KEY_WRITE, &hk) == ERROR_SUCCESS)  {
        LPTSTR szStr;
        DWORD dwSize;

        if (fCheck)
            szStr = TEXT("Yes");
        else
            szStr = TEXT("No");
        dwSize = CbFromCch( lstrlen( szStr ) + 1 );
        RegSetValueEx( hk, TEXT("Check_Associations"), 0, REG_SZ, (LPBYTE) szStr, dwSize );
        RegCloseKey( hk );
    }
}



/*----------------------------------------------------------
Purpose: Determines if the user has turned off the "check for
         default browser" in the registry.

*/
BOOL IsCheckAssociationsOn()
{
    BOOL    rval = TRUE;
    CHAR   szBuf[20];
    DWORD   dwSize = sizeof(szBuf);
    DWORD   dwValType;

    if (NO_ERROR == SHGetValueA(HKEY_CURRENT_USER, c_szCheckAssnSwitch, 
                                 "Check_Associations", &dwValType, 
                                 szBuf, &dwSize))
    {
        if ((dwValType == REG_SZ) && (dwSize < sizeof(szBuf)))  {
            if (StrCmpIA( szBuf, "No") == 0)
                rval = FALSE;
        }
    }

    return( rval );
}

/***********************************************************************

  These routines are used to repair damage done to Internet Explorer's 
  settings by "Netscape Navigator" and "Netscape TuneUp For IE"

 ***********************************************************************/

//
// Prototype for advpack functions
//
HRESULT RunSetupCommand(HWND hWnd, LPCSTR szCmdName, LPCSTR szInfSection, LPCSTR szDir, LPCSTR lpszTitle, HANDLE *phEXE, DWORD dwFlags, LPVOID pvReserved);

//
// This flag tells us whether it's ok to used the
// cached BOOL for IsResetWebSettingsRequired.
//
BOOL g_fAlreadyCheckedForClobber = FALSE;

HRESULT RunSetupCommandW(HWND hWnd, LPCWSTR szCmdName, LPCWSTR szInfSection, LPCWSTR szDir, LPCWSTR lpszTitle, HANDLE *phEXE, DWORD dwFlags, LPVOID pvReserved)
{

    CHAR szCmdNameA[MAX_PATH];
    CHAR szInfSectionA[MAX_PATH];
    CHAR szDirA[MAX_PATH];
    
    SHUnicodeToAnsi(szCmdName,szCmdNameA,ARRAYSIZE(szCmdNameA));
    SHUnicodeToAnsi(szInfSection,szInfSectionA,ARRAYSIZE(szInfSectionA));
    SHUnicodeToAnsi(szDir,szDirA,ARRAYSIZE(szDirA));

    ASSERT(NULL == pvReserved);
    ASSERT(NULL == lpszTitle);

    return RunSetupCommand(hWnd, szCmdNameA, szInfSectionA, szDirA, NULL, phEXE, dwFlags, NULL);
}


//
// Path to the inf file
//
#define IERESTORE_FILENAME  TEXT("iereset.inf")
#define INF_PATH            TEXT("inf")

//
// Names of the sections in our inf file
//
#define INFSECTION_HOMEPAGE  TEXT("RestoreHomePage")
#define INFSECTION_SETTINGS  TEXT("RestoreBrowserSettings")

#define IE_VERIFY_REGKEY     TEXT("Software\\Microsoft\\Internet Explorer\\Main")
#define IE_VERIFY_REGVALUE   TEXT("Default_Page_URL")

#define INFSECTION_VERIFY    TEXT("Strings")
#define IE_VERIFY_INFKEY     TEXT("START_PAGE_URL")

void GetIEResetInfFileName(LPWSTR pszBuffer)
{
    TCHAR szWindowsDir[MAX_PATH];

    if (NULL == pszBuffer)
        return;

    GetWindowsDirectory(szWindowsDir,ARRAYSIZE(szWindowsDir));

    wnsprintfW(
        pszBuffer,
        MAX_PATH,
        TEXT("%s\\%s\\%s"),
        szWindowsDir,INF_PATH,IERESTORE_FILENAME);

    return;
}

/*
 * CheckIESettings
 *
 * This function will try to determine whether or not IE's settings
 * have been clobbered by another browser.
 *
 * Returns S_OK if IE settings are intact.
 * Returns S_FALSE if someone has mucked with the IE settings
 * Returns E_FAIL on error
 * 
 */
HRESULT CheckWebSettings(void)
{

    TCHAR szInfPath[MAX_PATH];
    TCHAR szDataFromInf[MAX_PATH];
    TCHAR szDataFromReg[MAX_PATH];
    LONG retval;

    HKEY hkey;
    DWORD dwType;
    DWORD dwSize = sizeof(szDataFromReg);
    //
    // Get the path to the inf file
    //
    GetIEResetInfFileName(szInfPath);

    //
    // Read the string from the inf file
    //
    retval = SHGetIniString(
        INFSECTION_VERIFY,
        IE_VERIFY_INFKEY,
        szDataFromInf,
        ARRAYSIZE(szDataFromInf),
        szInfPath);

    if (retval <= 0)
        return E_FAIL;

    //
    // Open the corresponding key in the registry
    //
    retval = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        IE_VERIFY_REGKEY,
        NULL,
        KEY_READ,
        &hkey);

    if (retval != ERROR_SUCCESS)
        return E_FAIL;

    //
    // Read the data from the registry
    //
    retval = RegQueryValueEx(
        hkey,
        IE_VERIFY_REGVALUE,
        NULL,
        &dwType,
        (LPBYTE)szDataFromReg,
        &dwSize);

    if (retval != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        return E_FAIL;
    }

    ASSERT(dwType == REG_SZ);

    RegCloseKey(hkey);

    //
    // Return S_OK if they match, S_FALSE if they don't
    //
    return StrCmp(szDataFromReg,szDataFromInf) ? S_FALSE : S_OK;

}

extern "C" BOOL IsResetWebSettingsRequired(void)
{
    static BOOL fRequired;

    if (!g_fAlreadyCheckedForClobber)
    {
        fRequired = (S_FALSE == CheckWebSettings());
        g_fAlreadyCheckedForClobber = TRUE;
    }

    return fRequired;

}

HRESULT ResetWebSettingsHelper(BOOL fRestoreHomePage)
{

    HRESULT hr;
    TCHAR szTempPath[MAX_PATH];
    TCHAR szInfPath[MAX_PATH];

    GetTempPath(ARRAYSIZE(szTempPath),szTempPath);
    GetIEResetInfFileName(szInfPath);

    g_fAlreadyCheckedForClobber = FALSE;

    //
    // Run the main part of the inf file
    //
    hr = RunSetupCommandW(
        NULL, 
        szInfPath, 
        INFSECTION_SETTINGS, 
        szTempPath, 
        NULL, 
        NULL, 
        RSC_FLAG_INF|RSC_FLAG_QUIET, 
        NULL);

    //
    // Also, reset their homepage if requested to do so
    //
    if (SUCCEEDED(hr) && fRestoreHomePage)
        hr = RunSetupCommandW(
            NULL, 
            szInfPath, 
            INFSECTION_HOMEPAGE, 
            szTempPath, 
            NULL, 
            NULL, 
            RSC_FLAG_INF|RSC_FLAG_QUIET, 
            NULL);

    return hr;
}

//
// Dialog Procedure for the "reset web settings" dialog
//
// Return values are:
//
//    -1   Something went wrong
//     0   The user changes his/her mind
//     1   We reset everything except the homepage
//     2   We reset everything including the homepage
//

BOOL CALLBACK ResetWebSettingsDlgProc(HWND hdlg, UINT uMsg, WPARAM wparam, LPARAM lparam)
{

    switch (uMsg)
    {
    case WM_INITDIALOG:

        CenterWindow(hdlg, GetDesktopWindow());
        CheckDlgButton(hdlg,IDC_RESET_WEB_SETTINGS_HOMEPAGE,BST_CHECKED);
        return TRUE;

    case WM_COMMAND:
        
        switch(LOWORD(wparam))
        {
        case IDYES:
            {
                HRESULT hr;
                BOOL fResetHomePage = (BST_CHECKED == IsDlgButtonChecked(hdlg,IDC_RESET_WEB_SETTINGS_HOMEPAGE));

                //
                // Restore the settings to their IE defaults
                //
                hr = ResetWebSettingsHelper(fResetHomePage);

                if (!IsIEDefaultBrowser())
                    InstallRegAssoc(WhichPlatform(), FALSE);

                if (FAILED(hr))
                    EndDialog(hdlg, -1);

                else if (fResetHomePage)
                    EndDialog(hdlg, 2);

                else
                    EndDialog(hdlg, 1);

            }
            return TRUE;
            
        case IDCANCEL:
        case IDNO:
            
            EndDialog(hdlg, 0);
            return TRUE;

        default:
            return FALSE;
        }

    default:
        return FALSE;
    }

}

HRESULT ResetWebSettings(HWND hwnd, BOOL *pfChangedHomePage)
{

    HRESULT hr;

    if (pfChangedHomePage)
        *pfChangedHomePage = FALSE;

    switch (DialogBox(
        MLGetHinst(),
        MAKEINTRESOURCE(IDD_RESET_WEB_SETTINGS),
        hwnd,
        (DLGPROC)ResetWebSettingsDlgProc))
    {
    case -1:
        hr = E_FAIL;
        break;

    case 0:
        hr = S_FALSE;
        break;

    case 1:
        hr = S_OK;
        break;

    case 2:
        if (pfChangedHomePage)
            *pfChangedHomePage = TRUE;
        hr = S_OK;
        break;

    default:
        ASSERT(0);
        hr = E_FAIL;
        break;
    }

    if (FAILED(hr))
    {
        MLShellMessageBox(
            hwnd,
            MAKEINTRESOURCE(IDS_RESET_WEB_SETTINGS_FAILURE),
            MAKEINTRESOURCE(IDS_RESET_WEB_SETTINGS_TITLE),
            MB_OK | MB_ICONEXCLAMATION);
    }
    else if (hr == S_OK)
    {
        MLShellMessageBox(
            hwnd,
            MAKEINTRESOURCE(IDS_RESET_WEB_SETTINGS_SUCCESS),
            MAKEINTRESOURCE(IDS_RESET_WEB_SETTINGS_TITLE),
            MB_OK | MB_ICONINFORMATION);
    }

    return hr;
}

/*----------------------------------------------------------
Purpose: Queries a registry set.  If the registry doesn't have
         the right values, this function applies the registry
         set changes to the registry.

*/
void
QueryAndApplyRegSet(
    IN RegSet * const * rgprs,
    IN UINT      cprs)
{
    UINT i;
    
    for (i = 0; i < cprs; i++) 
    {
        // Does the registry have the right settings?
        if (!IsRegSetInstalled(rgprs[i])) 
        {
            // No; apply the settings
            for (i = 0; i < ARRAYSIZE(rgprs); i++)  
                InstallRegSet(rgprs[i], FALSE);
            break;
        }
    }
}

void EnsureWebViewRegSettings()
{
    // We do the following mini-check regardless of the user's settings,
    // and for every window we open
    QueryAndApplyRegSet((RegSet* const*)g_rgprsWebview, ARRAYSIZE(g_rgprsWebview));
}

void
FixIcons()
{
    int i;

    for (i = 0; i < ARRAYSIZE(g_rgprsDefault_FixIcon); i++)  
    {
        // 2nd param FALSE ==> always intrude.
        InstallRegSet(g_rgprsDefault_FixIcon[i], FALSE);
    }
}


/*----------------------------------------------------------
Purpose: Function that determines if we are the default browser.
         If not the default browser, this function will
         ask the user to if they want to become the default
         browser and make those changes.

*/
void 
DetectAndFixAssociations()
{
    TraceMsg(TF_REGCHECK, "Performing expensive registry query for default browser!");

    // We will become the Default browser if:
    // 1. The User has "Check Associations" On,
    // 2. We don't own the associations, and
    // 3. The user said Yes when we displayed the dialog.
    if (IsCheckAssociationsOn() &&
        !IsIEDefaultBrowser() &&
        AskUserShouldFixReg())
    {
        InstallRegAssoc(WhichPlatform(), FALSE);
    }
}

/*
A really quick - non - through check to see if IE is likely the
default browser
*/

BOOL IsIEDefaultBrowserQuick(void)
{

    int i;
    BOOL bAssociated = TRUE;

    TraceMsg(TF_REGCHECK, "Performing expensive registry query for default browser!");

    // Check the settings common to all platforms
    for (i = 0; i < ARRAYSIZE(g_rgprsDefault_Quick); i++)  
    {
        if (! IsRegSetInstalled(g_rgprsDefault_Quick[i]))  
            bAssociated = FALSE;
    }
    return bAssociated;

}

/*----------------------------------------------------------
Purpose: Function that determines if we are the default browser.

*/
BOOL
IsIEDefaultBrowser(void)
{
    int i;
    BOOL bAssociated = TRUE;
    UINT nInstall = WhichPlatform();

    TraceMsg(TF_REGCHECK, "Performing expensive registry query for default browser!");

    // Check the settings common to all platforms
    for (i = 0; i < ARRAYSIZE(g_rgprsDefault); i++)  
    {
        if (! IsRegSetInstalled(g_rgprsDefault[i]))  
            bAssociated = FALSE;
    }

    if (bAssociated)
    {
        // Check specific to IE or Nashville
        switch (nInstall)
        {
        case PLATFORM_BROWSERONLY:
            for (i = 0; i < ARRAYSIZE(g_rgprsDefault_Alone); i++)  
            {
                if (! IsRegSetInstalled(g_rgprsDefault_Alone[i]))  
                {
                    bAssociated = FALSE;
                    break;
                }
            }
            break;

        case PLATFORM_INTEGRATED:
            for (i = 0; i < ARRAYSIZE(g_rgprsDefault_Full); i++)  
            {
                if (! IsRegSetInstalled(g_rgprsDefault_Full[i]))  
                {
                    bAssociated = FALSE;
                    break;
                }
            }
            break;

        default:
            ASSERT(0);
            break;
        }
    }
        
    // If IE is the default browser and this was an NT5Upgrade scenario,
    // fix the Icons references.
    if (g_bNT5Upgrade && bAssociated)
    {
        FixIcons();
    }

    return bAssociated;
}


/*----------------------------------------------------------
Purpose: Checks if we're installing over IE.  This function
         looks at the associated shell\open\command handler
         for the http protocol.  

Returns: TRUE if we're installing over IE
*/
BOOL
AreWeInstallingOverIE(void)
{
    BOOL bRet = FALSE;
    CHAR sz[MAX_PATH + 20];    // add some padding for arguments
    DWORD cbData = SIZEOF(sz);

    if (NO_ERROR == SHGetValueA(HKEY_CLASSES_ROOT, c_szHTTPOpenCmd, "",
                               NULL, sz, &cbData) &&
        StrStrIA(sz, IEXPLORE_EXE) || StrStrIA(sz, EXPLORER_EXE))
    {
        TraceMsg(TF_REGCHECK, "Installing over IEXPLORE.EXE");
        bRet = TRUE;
    }
    return bRet;
}    

BOOL ShouldIEBeDefaultBrowser(void)
{
    BOOL bRet = TRUE;          // default to TRUE (eg take over the association)
    CHAR sz[MAX_PATH + 20];    // add some padding for arguments
    DWORD cbData = ARRAYSIZE(sz);
    sz[0] = '\0';

    if (NO_ERROR == SHGetValueA(HKEY_CLASSES_ROOT, ".htm", "", NULL, sz, &cbData))
    {
        if (!sz[0])
        {
            // null key so return TRUE
            return bRet;
        }
        else if (!StrCmpIA(sz, "htmlfile"))
        {
            // Maybe, make sure further
            sz[0] = '\0';
            cbData = ARRAYSIZE(sz);

            if (NO_ERROR == SHGetValueA(HKEY_CLASSES_ROOT, c_szHTMOpenCmd, "",
                                       NULL, sz, &cbData))
            {
                if (!sz[0] ||  // if sz[0] is NULL, we will take it over  (probably broken reg)
                    StrStrIA(sz, IEXPLORE_EXE))
                {
                    // Default browser was IE, so we return TRUE 
                    TraceMsg(TF_REGCHECK, "IEXPLORE.EXE is the default browser");
                }
                else
                {
                    TraceMsg(TF_REGCHECK, "%s is the default browser (NOT iexplore.exe)", sz);
                    bRet = FALSE;
                }
            }
        }
        else
        {
            // the progid does not point to "htmlfile", so IE cant be the default browser
            TraceMsg(TF_REGCHECK, "%s is the .htm progid (NOT htmlfile)", sz);
            bRet = FALSE;
        }
    }
    // .htm progid key does not exist, so we return TRUE

    return bRet;
}


#define SZ_REGKEY_FTPSHELLOPEN          TEXT("ftp\\shell\\open")
#define SZ_REGKEY_COMMAND               TEXT("command")
#define SZ_REGKEY_DDEEXEC               TEXT("ddeexec\\ifExec")

#define SZ_IEXPLORE_FTP_NEW          TEXT("iexplore.exe\" %1")
#define SZ_IEXPLORE_FTP_OLD          TEXT("iexplore.exe\" -nohome")

HRESULT UpgradeSettings(void)
{
    HRESULT hr = S_OK;
    HKEY hKey;
        
    hr = HRESULT_FROM_WIN32(RegOpenKey(HKEY_CLASSES_ROOT, SZ_REGKEY_FTPSHELLOPEN, &hKey));
    if (hKey)
    {
        TCHAR szData[MAX_PATH];
        LONG cbSize = sizeof(szData);
        
        hr = HRESULT_FROM_WIN32(RegQueryValue(hKey, SZ_REGKEY_COMMAND, szData, &cbSize));
        if (SUCCEEDED(hr))
        {
            DWORD cchStart = (lstrlen(szData) - ARRAYSIZE(SZ_IEXPLORE_FTP_OLD) + 1);

            // Do we own it?
            if (0 == StrCmp(SZ_IEXPLORE_FTP_OLD, &szData[cchStart]))
            {
                // Yes, so we can upgrade it.

                // Buffer Overflow isn't a problem because I know SZ_IEXPLORE_FTP_NEW is smaller
                // than SZ_IEXPLORE_FTP_OLD.
                StrCpyN(&szData[cchStart], SZ_IEXPLORE_FTP_NEW, ARRAYSIZE(szData));

                // Yes, so let's upgrade.
                hr = HRESULT_FROM_WIN32(RegSetValue(hKey, SZ_REGKEY_COMMAND, REG_SZ, szData, lstrlen(szData)));
                if (SUCCEEDED(hr))
                    hr = HRESULT_FROM_WIN32(RegSetValue(hKey, SZ_REGKEY_DDEEXEC, REG_SZ, TEXT(""), ARRAYSIZE(TEXT(""))));
            }
        }

        RegCloseKey(hKey);
    }

    return hr;
}


/*----------------------------------------------------------
Purpose: Install registry info based upon which shell we're running

*/
HRESULT InstallIEAssociations(DWORD dwFlags)   // IEA_* flags
{
    int i;
    UINT nInstall = WhichPlatform();
    BOOL bDontIntrude = TRUE;
    
    // If IE was the default browser before (or the registry is messed up)
    // or setup is forcing us to register then we want to force IE to be 
    // the default browser
    if (ShouldIEBeDefaultBrowser() || IsFlagSet(dwFlags, IEA_FORCEIE))
        bDontIntrude = FALSE;
    
    // Install file and protocol associations
    
    InstallRegAssoc(nInstall, bDontIntrude);
    
    // Install other registry settings
    
    switch (nInstall)
    {
    case PLATFORM_BROWSERONLY:
        for (i = 0; i < ARRAYSIZE(g_rgprsIE30Only); i++)  
        {
            InstallRegSet(g_rgprsIE30Only[i], bDontIntrude);
        }
        break;
        
    case PLATFORM_INTEGRATED:
        for (i = 0; i < ARRAYSIZE(g_rgprsNashOnly); i++)  
        {
            InstallRegSet(g_rgprsNashOnly[i], bDontIntrude);
        }
        break;
        
    default:
        ASSERT(0);
        break;
    }
    
    InstallFTPAssociations();
    
    return NOERROR;
}


HRESULT UninstallPlatformRegItems(BOOL bIntegrated)
{
    int i;
    UINT uPlatform = bIntegrated ? PLATFORM_INTEGRATED : PLATFORM_BROWSERONLY;
    
    switch (uPlatform)
    {
    case PLATFORM_BROWSERONLY:
        for (i = 0; i < ARRAYSIZE(g_rgprsUninstallIE30); i++)  
        {
            InstallRegSet(g_rgprsUninstallIE30[i], FALSE);
        }
        break;
        
    case PLATFORM_INTEGRATED:
        for (i = 0; i < ARRAYSIZE(g_rgprsUninstallNash); i++)  
        {
            InstallRegSet(g_rgprsUninstallNash[i], FALSE);
        }
        break;
        
    default:
        // Don't do anything
        break;
    }
    
    return NOERROR;
}

void UninstallCurrentPlatformRegItems()
{
    CHAR sz[MAX_PATH + 20];    // add some padding for arguments
    DWORD cbData = SIZEOF(sz);
    if (NO_ERROR == SHGetValueA(HKEY_CLASSES_ROOT, c_szHTMOpenNewCmd, "",
                               NULL, sz, &cbData))
    {
        // Remove IE4 shell integrated settings
        UninstallPlatformRegItems(TRUE);
    }
    else if (AreWeInstallingOverIE())
    {
        // Remove IE3 / browser only settings
        UninstallPlatformRegItems(FALSE);
    }
}
