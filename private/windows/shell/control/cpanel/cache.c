/** FILE: cache.c ********** Module Header ********************************
 *
 *  Control Panel caching functions and procedures.
 *
 * History:
 *  21 Sept 1993  -by-  Steve Cathcart   [stevecat]
 *        Added Caching of Control Panel modules
 *
 *
 *  Copyright (C) 1990-1993 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                           Include files
//==========================================================================
// Windows SDK
#include <windows.h>
#include <shellapi.h>

// Application specific
#include "cpl.h"
#include "cpl_defs.h"
#include "uniconv.h"

//==========================================================================
//                          Local Definitions
//==========================================================================
#define BMPSIZE     4096

//==========================================================================
//                            External Declarations
//==========================================================================
// External functions

// External data

//==========================================================================
//                            Local Data Declarations
//==========================================================================
TCHAR    szAA[]             = TEXT("aaaaaa");

// Control Panel registry cache entry key
TCHAR *pszCplCache = TEXT("Control Panel\\Cache\\%s");
TCHAR *pszRegCache = TEXT("Control Panel\\Cache");

// Control Panel cache - validation flag key 
TCHAR *pszCacheValid   = TEXT("Cache Valid");
TCHAR *pszDisplayType  = TEXT("Display Type");

// Control Panel cache - display flag keys
TCHAR *pszBitspixel    = TEXT("Bits/Pixel");
TCHAR *pszColorres     = TEXT("ColorRes");
TCHAR *pszPlanes       = TEXT("Planes");
TCHAR *pszAveCharWidth = TEXT("tmAveCharWidth");
TCHAR *pszHeight       = TEXT("tmHeight");
TCHAR *pszWeight       = TEXT("tmWeight");

//
//  Global flag to indicate that an applet is active
//

BOOL  bAppletActive = FALSE;

//
//  Global flag to indicate that the cache is invalid and needs to be
//  rebuilt.
//

BOOL  bRebuildCache = FALSE;

//  Global flag to indicate all 2nd thread validation of modules is done
//  Initial state is TRUE, because 2nd thread will control its state
//  but it may never be invoked.

BOOL  bValidationDone = TRUE;

// Control Panel cache module data value keys
TCHAR *pszFileSize     = TEXT("File Size");
TCHAR *pszFileTimeHigh = TEXT("High File Time");
TCHAR *pszFileTimeLow  = TEXT("Low File Time");
TCHAR *pszModulePath   = TEXT("Module Path");
TCHAR *pszNumApplets   = TEXT("Number Applets");

// Control Panel cache module applet data value keys
TCHAR *pszAppletIconC  = TEXT("Icon Color");
TCHAR *pszAppletIconM  = TEXT("Icon Mask");
TCHAR *pszAppletIconX  = TEXT("IconX");
TCHAR *pszAppletIconY  = TEXT("IconY");
TCHAR *pszAppletInfoC  = TEXT("Color Info");
TCHAR *pszAppletInfoM  = TEXT("Mask Info");
TCHAR *pszAppletFull   = TEXT("Applet Full Name");
TCHAR *pszAppletName   = TEXT("Applet Name");
TCHAR *pszAppletInfo   = TEXT("Description");
TCHAR *pszAppletHelp   = TEXT("Help File");
TCHAR *pszAppletCntx   = TEXT("Help Context");
TCHAR *pszAppletData   = TEXT("Applet Data");


//==========================================================================
//                            Local Function Prototypes
//==========================================================================


//==========================================================================
//                                Functions
//==========================================================================


////////////////////////////////////////////////////////////////////////////
//
//  Replace "\" chars in path with "/" chars because MUTEX objects do not
//  like it but we want to maintain the uniqueness of mutex object names
//  based on modules full pathname.
//
//  Also, since MUTEX objects are in global, flat name space, make the
//  name unique by prepending User's name to it.
//
//  Assumes pszMutex string is at least same size as pszPathname string
//  plus MAX_PATH to hold User's logon name and pszPathname.
//
////////////////////////////////////////////////////////////////////////////

void CreateMutexNameFromPath (LPTSTR pszPathname, LPTSTR pszMutex)
{
    TCHAR  szMutexName[MAX_PATH * 2];
    LPTSTR pszTemp;
    int    iLen2 = MAX_PATH / 2;
    int    iLen4 = MAX_PATH / 4;
    int    iLen8 = MAX_PATH / 8;

    lstrcpy (szMutexName, szUsername);
    lstrcat (szMutexName, pszPathname);

    if (lstrlen (szMutexName) > MAX_PATH - 1)
    {
        //  Name is too long, just use parts of it

        // Use first and last MAX_PATH / 8 (approx. 30) chars of Username

        if (lstrlen (szUsername) > iLen4 + 3)
        {
            _tcsncpy (szMutexName, szUsername, iLen8);
            lstrcat (szMutexName, TEXT("..."));
            pszTemp = szUsername + (lstrlen (szUsername) - iLen8) * sizeof(TCHAR);
            lstrcat (szMutexName, pszTemp);
        }
        else
        {
            lstrcpy (szMutexName, szUsername);
        }

        // Fill in rest with (parts of) pathname

        if (lstrlen (pszPathname) > iLen2 + 3)
        {
            _tcsncat (szMutexName, pszPathname, iLen4);
            lstrcat (szMutexName, TEXT("..."));
            pszTemp = pszPathname + (lstrlen (pszPathname) - iLen4) * sizeof(TCHAR);
            lstrcat (szMutexName, pszTemp);
        }
        else
        {
            lstrcpy (szMutexName, pszPathname);
        }
    }

#if DBG
    if (lstrlen (szMutexName) > MAX_PATH - 1)
        RIP(TEXT("CreateMutexNameFromPath - MUTEX name too long."));
#endif  //  DBG

    lstrcpy (pszMutex, szMutexName);
    _tcsupr (pszMutex);

    while (pszTemp = _tcschr(pszMutex, TEXT('\\')))
        *pszTemp = TEXT('/');

    return;
}


////////////////////////////////////////////////////////////////////////////
//
//  Load module into memory and pass initial messages to it, fill in
//  necessary fields of PCPLMODULE struct
//
////////////////////////////////////////////////////////////////////////////

BOOL LoadCachedModule (PCPLMODULE  pCPlMod)
{
    TCHAR  szLoadingName[64+ CharSizeOf(szLoading)];
    int    numApplets;
    BOOL   bLoaded=FALSE;


    if (pCPlMod && !pCPlMod->bLoaded)
    {
        if (hCPlWnd)
        {
            /* update feedback on what applet library is currently loading */
#ifdef JAPAN    /* V-KeijiY  July.6.7 */
            lstrcpy(szLoadingName, pCPlMod->szPathname);
            lstrcat(szLoadingName, szLoading);
#else
            lstrcpy(szLoadingName, szLoading);
            lstrcat(szLoadingName, pCPlMod->szPathname);
#endif
            SetWindowText(hCPlTB, szLoadingName);
            UpdateWindow(hCPlWnd);
        }

        if (pCPlMod->hLibrary = LoadLibrary(pCPlMod->szPathname))
        {
            if ((pCPlMod->lpfnCPlApplet =
                 (APPLET_PROC)GetProcAddress(pCPlMod->hLibrary, szCPlApplet)) &&
                 (*pCPlMod->lpfnCPlApplet)(hCPlWnd, CPL_INIT, 0L, 0L))
            {
    
                numApplets = (int)(*pCPlMod->lpfnCPlApplet)(hCPlWnd, CPL_GETCOUNT, 0L, 0L);
    
                bLoaded = TRUE;
    
                ASSERT(pCPlMod->numApplets == numApplets);
            }
            else
            {
                //
                //  Error after library loaded, so free it and force a
                //  rebuild of applet cache.
                //

                FreeLibrary (pCPlMod->hLibrary);
                pCPlMod->hLibrary = NULL;
                pCPlMod->lpfnCPlApplet = NULL;
                goto  SetFaultCondition;
            }
        }
        else
        {
SetFaultCondition:
            //
            //  Failure to load an applet that we thought was valid.
            //  Set global to force CACHE rebuild on LoadLibrary failure.
            //

            bRebuildCache = TRUE;
        }
    }
    else
    {
        return (pCPlMod->bLoaded);
    }

    return bLoaded;
}


////////////////////////////////////////////////////////////////////////////
//
//  Create a unique key value for this pathname.
//
//
////////////////////////////////////////////////////////////////////////////

BOOL CreateCacheKey (LPTSTR szReturn, LPTSTR szPathname, int iUnique)
{
    LPTSTR pszTemp;
    TCHAR  szUnq[10];

    // Find last backslash and copy from there
    pszTemp = _tcsrchr (szPathname, TEXT('\\'));

    // Increment past "\" char
    pszTemp++;

    //  If this is "MAIN.CPL" make a special key for it so it will
    //  always be first in the registry.  This will insure that it
    //  is always loaded first from the cache area.

    if (!_tcsicmp (pszTemp, szMAINCPL))
    {
        lstrcpy (szReturn, szAA);
    }
    else
    {
        lstrcpy (szReturn, pszTemp);
    }

    if (iUnique != 0)
    {
        MyItoa (iUnique, szUnq, 10);
        lstrcat (szReturn, szUnq);
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Free the .CPL module and load it in later, only when asked for again
//  by User.  This will help to significantly decrease the Control Panel's
//  working set and memory allocation.
//
//
////////////////////////////////////////////////////////////////////////////

void FreeCachedModule (PCPLMODULE  pCPlMod)
{
    if (pCPlMod && pCPlMod->bLoaded && pCPlMod->hLibrary)
    {
        // no longer in need of your services
        if (*pCPlMod->lpfnCPlApplet)
            (*pCPlMod->lpfnCPlApplet) (hCPlWnd, CPL_EXIT, 0L, 0L);

        if (FreeLibrary (pCPlMod->hLibrary))
            pCPlMod->hLibrary = NULL;

        pCPlMod->lpfnCPlApplet = NULL;
        pCPlMod->bLoaded = FALSE;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Clear "Cache Valid" flag in registry
//
////////////////////////////////////////////////////////////////////////////

void ClearCacheValid()
{
    int    j;
    HKEY   hkeyRCache = NULL;


    //////////////////////////////////////////////////////////////////////
    //  Clear "Cache Valid" flag
    //////////////////////////////////////////////////////////////////////

    if (RegOpenKeyEx (HKEY_CURRENT_USER, pszRegCache, 0L, KEY_ALL_ACCESS,
                        &hkeyRCache) == ERROR_SUCCESS)
    {
        j = 0;
        if ((RegSetValueEx (hkeyRCache, pszCacheValid, 0L, REG_DWORD,
                        (LPBYTE) &j, sizeof(DWORD)))
                != ERROR_SUCCESS)
        {
            RIPREG();
        }

        RegCloseKey (hkeyRCache);
    }

    return;
}


////////////////////////////////////////////////////////////////////////////
//
//  Set "Cache Valid" flag in registry
//
////////////////////////////////////////////////////////////////////////////

void SetCacheValid()
{
    int    j;
    HKEY   hkeyRCache;


    //////////////////////////////////////////////////////////////////////
    //  Indicate "Cache Valid"
    //////////////////////////////////////////////////////////////////////

    if (RegOpenKeyEx (HKEY_CURRENT_USER, pszRegCache, 0L, KEY_ALL_ACCESS,
                        &hkeyRCache) == ERROR_SUCCESS)
    {
        j = 1;
        if ((RegSetValueEx (hkeyRCache, pszCacheValid, 0L, REG_DWORD,
                        (LPBYTE) &j, sizeof(DWORD)))
                != ERROR_SUCCESS)
        {
            RIPREG();
        }

        RegCloseKey (hkeyRCache);
    }

    return;
}


////////////////////////////////////////////////////////////////////////////
//
//  Return state of "Cache Valid" flag in registry
//
//  Note:  This routine will create the "Control Panel\Cache" key if it
//         does not exist.
//
//  Returns:
//          TRUE  - if CacheValid flag is TRUE
//          FALSE - if CacheValid flag is FALSE
//                  cache should be updated, some items do not match
//
////////////////////////////////////////////////////////////////////////////

BOOL IsCacheValid()
{
    HKEY  hkeyRCache = NULL;
    DWORD j = 0;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwDisposition;

    //////////////////////////////////////////////////////////////////////
    //  Check "Cache Valid" flag
    //////////////////////////////////////////////////////////////////////

    if (RegCreateKeyEx (HKEY_CURRENT_USER,         // Root key
                        pszRegCache,               // Subkey to open/create
                        0L,                        // Reserved
                        NULL,                      // Class string
                        0L,                        // Options
                        KEY_ALL_ACCESS,            // SAM
                        NULL,                      // ptr to Security struct
                        &hkeyRCache,               // return handle
                        &dwDisposition)            // return disposition
         == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);

        RegQueryValueEx (hkeyRCache,
                         pszCacheValid,
                         0L,
                         &dwType,
                         (LPBYTE) &j,
                         &dwSize);

        RegCloseKey (hkeyRCache);
    }

    return (j != 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Return DWORD value of "pszCacheEntry" registry value under the "Cache"
//  key in registry.
//
////////////////////////////////////////////////////////////////////////////

DWORD GetCachedValue (LPTSTR pszCacheEntry)
{
    HKEY  hkeyRCache = NULL;
    DWORD j = 0;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwDisposition;

    //////////////////////////////////////////////////////////////////////
    //  Check "Cache Valid" flag
    //////////////////////////////////////////////////////////////////////

    if (RegCreateKeyEx (HKEY_CURRENT_USER,         // Root key
                        pszRegCache,               // Subkey to open/create
                        0L,                        // Reserved
                        NULL,                      // Class string
                        0L,                        // Options
                        KEY_ALL_ACCESS,            // SAM
                        NULL,                      // ptr to Security struct
                        &hkeyRCache,               // return handle
                        &dwDisposition)            // return disposition
         == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);

        RegQueryValueEx (hkeyRCache,
                         pszCacheEntry,
                         0L,
                         &dwType,
                         (LPBYTE) &j,
                         &dwSize);

        RegCloseKey (hkeyRCache);
    }

    return j;
}


////////////////////////////////////////////////////////////////////////////
//
//  Write DWORD value of "pszCacheEntry" under the "Cache" key in registry.
//
////////////////////////////////////////////////////////////////////////////

void SetCachedValue (LPTSTR pszCacheEntry, DWORD dwValue)
{
    HKEY   hkeyRCache;


    if (RegOpenKeyEx (HKEY_CURRENT_USER, pszRegCache, 0L, KEY_ALL_ACCESS,
                        &hkeyRCache) == ERROR_SUCCESS)
    {
        if ((RegSetValueEx (hkeyRCache, pszCacheEntry, 0L, REG_DWORD,
                        (LPBYTE) &dwValue, sizeof(DWORD)))
                != ERROR_SUCCESS)
        {
            RIPREG();
        }

        RegCloseKey (hkeyRCache);
    }

    return;
}


////////////////////////////////////////////////////////////////////////////
//
//  Free all memory allocated for linked list of all CPL Module keys.
//
////////////////////////////////////////////////////////////////////////////

BOOL FreeModuleKeyList (REGKEY *prkFirst)
{
    REGKEY *prkRegKey;


    //  Traverse the list, freeing list memory

    prkRegKey = prkFirst;

    while (prkRegKey)
    {
        prkFirst  = prkRegKey;
        prkRegKey = prkRegKey->prkNext;

        if (LocalFree ((HLOCAL) prkFirst->pszKeyName))
        {
            RIPMEM();
            return FALSE;
        }

        if (LocalFree ((HLOCAL) prkFirst))
        {
            RIPMEM();
            return FALSE;
        }
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Return a linked list of all CPL Module keys listed in the registry
//  under the "Cache" key.
//
////////////////////////////////////////////////////////////////////////////

REGKEY *GetModuleKeyList (HKEY hkeyRCache)
{
    int     i;
    DWORD   dwBufz;
    REGKEY *prkFirst = NULL;
    REGKEY *prkRegKey = NULL;
    TCHAR   szModKey[MAX_PATH];


    i = 0;
    dwBufz = CharSizeOf(szModKey);

    //  Create a linked list of all "Module keys"

    while (RegEnumKeyEx (hkeyRCache, i, szModKey, &dwBufz, 0L,
                          NULL, NULL, NULL) != ERROR_NO_MORE_ITEMS)
    {
        if (prkFirst)
        {
            prkRegKey->prkNext = (REGKEY *) LocalAlloc (LPTR, sizeof(REGKEY));
            prkRegKey = prkRegKey->prkNext;
        }
        else        // First time thru
        {
            prkFirst  =
            prkRegKey = (REGKEY *) LocalAlloc (LPTR, sizeof(REGKEY));
        }

        if (prkRegKey)
            prkRegKey->prkNext = NULL;
        else
        {
            RIPMEM();
            goto ErrorExit;
        }

        if (prkRegKey->pszKeyName = (LPTSTR) LocalAlloc (LPTR,
                                    ByteCountOf(lstrlen((LPTSTR)szModKey)+1)))
        {
            lstrcpy(prkRegKey->pszKeyName, (LPTSTR)szModKey);
        }
        else
        {
            RIPMEM();
            goto ErrorExit;
        }

        //  Get next Key name
        i++;
        dwBufz = CharSizeOf(szModKey);
    }

    return (prkFirst);

    //////////////////////////////////////////////////////////////////////
    //  Routine error handling
    //////////////////////////////////////////////////////////////////////

ErrorExit:

    FreeModuleKeyList (prkFirst);
    return NULL;
}


////////////////////////////////////////////////////////////////////////////
//
//  Find (but do not load) all of the .CPL modules that would be found
//  during the normal filesystem search.  This routine uses the same
//  algorithm to find modules as LoadApplets().
//
//  Get the keynames under [MMCPL] in CONTROL.INI and cycle
//  through all such keys to create cpl file structs for each.
//  
////////////////////////////////////////////////////////////////////////////

int CheckFindModules (CPLFILES *pcfCPlFiles)
{
    LPTSTR  pstr;
    int     numMods = 0;
    TCHAR   szKeys[256];    // array size = 256 is assumed by GetSysDir()!
    TCHAR   szName[64];
    HANDLE  hFile = NULL;
    BOOL    b;

    WIN32_FIND_DATA FindFileData;

    BY_HANDLE_FILE_INFORMATION bhfiMod;


    //  Find the modules specified in CONTROL.INI under [MMCPL]

    //  First, enumerate all keys under szMMCPL
    GetPrivateProfileString(szMMCPL, NULL, szNULL, szKeys, CharSizeOf(szKeys), aszControlIniPath);

    for (pstr = szKeys; *pstr && (numMods < CPL_MAXMODS-1); pstr += lstrlen(pstr) + 1)
    {
        //
        //  For each key, get its data
        //

        GetPrivateProfileString(szMMCPL, pstr, szNULL, szName, 64, aszControlIniPath);

        //
        //  If pstr NOT EQUAL to our known values under szMMCPL,
        //  we have found a module to be added to our list
        //

        if (_tcsicmp(pstr, szNumApps) && _tcsicmp(pstr, szMaxWidth) &&
            !((*(pstr+1) == (TCHAR) 0)  &&     // skip over Wnd size keynames
               ((*pstr == *szWndDim[0]) || (*pstr == *szWndDim[1]) ||
                (*pstr == *szWndDim[2]) || (*pstr == *szWndDim[3])) ))
        {
            //  We have found a potential module to load - get file info

            lstrcpy(pcfCPlFiles->szPathname, szName);

            //  Open File and get info on it
            if ((hFile = CreateFile (pcfCPlFiles->szPathname,
                                     0,
                                     FILE_SHARE_WRITE,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL)) == INVALID_HANDLE_VALUE)
            {
                RIPGEN();
                //
                //  Ignore errors - file may not exist (i.e. it may just be
                //  a bad entry under [MMCPL] key).  This is not a reason to
                //  to force cache invalid.
                //
                continue;
            }

            if (!GetFileInformationByHandle (hFile, &bhfiMod))
            {
                RIPGEN();
                //
                //  Same as above.
                //

                if (hFile != NULL)
                    CloseHandle (hFile);

                continue;
            }

            CloseHandle (hFile);

            hFile = NULL;

            pcfCPlFiles->dwSize = bhfiMod.nFileSizeLow;

            pcfCPlFiles->ftModule = bhfiMod.ftLastWriteTime;

            pcfCPlFiles->numApplets = 0;

            pcfCPlFiles->bAlreadyMatched = FALSE;

            numMods++;       // Increment module count
            pcfCPlFiles++;
        }
    }

    //  Find applets in the system directory (this will include MAIN.CPL)

    MakeSysPath (szKeys, szCPL);

    if ((hFile = FindFirstFile(szKeys, &FindFileData)) != INVALID_HANDLE_VALUE)
    {
        b = TRUE;

        while (b && (numMods < CPL_MAXMODS - 1))
        {
            CatPath(szKeys, FindFileData.cFileName, lstrlen(szKeys));

            lstrcpy(pcfCPlFiles->szPathname, szKeys);

            pcfCPlFiles->dwSize = FindFileData.nFileSizeLow;

            pcfCPlFiles->ftModule = FindFileData.ftLastWriteTime;

            pcfCPlFiles->numApplets = 0;

            pcfCPlFiles->bAlreadyMatched = FALSE;

            numMods++;
            pcfCPlFiles++;

            b = FindNextFile(hFile, &FindFileData);
        }
        FindClose(hFile);
    }

    return (numMods);
}


////////////////////////////////////////////////////////////////////////////
//
//  Get a list of all of the Cached .CPL modules from registry
//  
////////////////////////////////////////////////////////////////////////////

int CheckGetCacheMods (CPLFILES *pcfCPlFiles)
{
    int    numMods = 0;
    DWORD  dwSize;
    DWORD  dwType;

    HKEY   hkeyCache  = NULL;
    HKEY   hkeyRCache = NULL;

    REGKEY *prkFirst  = NULL;
    REGKEY *prkRegKey = NULL;


    //////////////////////////////////////////////////////////////////////
    //  Open Cache
    //////////////////////////////////////////////////////////////////////

    if (RegOpenKeyEx (HKEY_CURRENT_USER, pszRegCache, 0L, KEY_ALL_ACCESS,
                        &hkeyRCache)
            != ERROR_SUCCESS)
    {
        RIPREG();
        goto RegistryError;
    }


    //////////////////////////////////////////////////////////////////////
    //  Get linked list of all "Module keys" for use below
    //////////////////////////////////////////////////////////////////////

    if (!(prkFirst = GetModuleKeyList (hkeyRCache)))
    {
        RIPMEM();
        goto MemoryError;
    }


    //////////////////////////////////////////////////////////////////////
    //  Now go thru module key list and fill-in cpl file structs
    //////////////////////////////////////////////////////////////////////

    prkRegKey = prkFirst;

    while (prkRegKey)
    {
        //  Open cache of module applets
        if (RegOpenKeyEx (hkeyRCache,                // Root key
                          prkRegKey->pszKeyName,     // Subkey to open/create
                          0L,                        // Reserved
                          KEY_ALL_ACCESS,            // SAM
                          &hkeyCache)                // return handle
                != ERROR_SUCCESS)
        {
            RIPREG();
            goto RegistryError;
        }

        //  Get cached Module info

        //  Number of applets
        pcfCPlFiles->numApplets = 0;
        dwSize = sizeof(DWORD);

        if (RegQueryValueEx (hkeyCache, pszNumApplets, 0L, &dwType,
                             (LPBYTE) &pcfCPlFiles->numApplets, &dwSize)
                != ERROR_SUCCESS)
        {
            RIPREG();
            goto RegistryError;
        }

        //  Module pathname 
        dwSize = sizeof(pcfCPlFiles->szPathname);

        if ((RegQueryValueEx (hkeyCache, pszModulePath, 0L, &dwType,
                              (LPBYTE) pcfCPlFiles->szPathname, &dwSize))
                != ERROR_SUCCESS)
        {
            RIPREG();
            goto RegistryError;
        }

        //  Module size 
        dwSize = sizeof(DWORD);

        if ((RegQueryValueEx (hkeyCache, pszFileSize, 0L, &dwType,
                              (LPBYTE) &pcfCPlFiles->dwSize, &dwSize))
                != ERROR_SUCCESS)
        {
            RIPREG();
            goto RegistryError;
        }

        //  Module filetime low
        dwSize = sizeof(DWORD);

        if ((RegQueryValueEx (hkeyCache, pszFileTimeLow, 0L, &dwType,
                              (LPBYTE) &pcfCPlFiles->ftModule.dwLowDateTime,
                               &dwSize))
                != ERROR_SUCCESS)
        {
            RIPREG();
            goto RegistryError;
        }

        //  Module filetime high
        dwSize = sizeof(DWORD);

        if ((RegQueryValueEx (hkeyCache, pszFileTimeHigh, 0L, &dwType,
                              (LPBYTE) &pcfCPlFiles->ftModule.dwHighDateTime,
                               &dwSize))
                != ERROR_SUCCESS)
        {
            RIPREG();
            goto RegistryError;
        }

        pcfCPlFiles->bAlreadyMatched = FALSE;

        pcfCPlFiles++;

        RegCloseKey (hkeyCache);
        hkeyCache = NULL;

        //  Next key name in list
        prkRegKey = prkRegKey->prkNext;

        //  Keep a count of modules for return to caller
        numMods++;
    }

    //////////////////////////////////////////////////////////////////////
    //  Clean up and leave
    //////////////////////////////////////////////////////////////////////

    //  Free list

    if (!FreeModuleKeyList (prkFirst))
        goto MemoryError;

    if (hkeyRCache != NULL)
        RegCloseKey (hkeyRCache);

    return numMods;


    //////////////////////////////////////////////////////////////////////
    //  Routine error handling
    //////////////////////////////////////////////////////////////////////

RegistryError:
MemoryError:

    if (hkeyRCache != NULL)
        RegCloseKey (hkeyRCache);

    if (hkeyCache != NULL)
        RegCloseKey (hkeyCache);

    FreeModuleKeyList (prkFirst);

    return 0;
}


////////////////////////////////////////////////////////////////////////////
//
//  Validate Control Panel module (i.e. *.CPL) cache entries.  Check all of
//  the modules found during filesystem search against the date, time, size
//  and number of applets of those stored in registry cache entries.
//
//  Also, check the display type and resolution to see if we need to force
//  a cache rebuild due to ICON size/type change.
//
////////////////////////////////////////////////////////////////////////////

BOOL CheckCache()
{
    DWORD  i, j;
    DWORD  numFiles = 0;
    DWORD  numCache = 0;
    BOOL   bRet = TRUE;
    HDC    hdc;

    CPLFILES  *cfCPlFiles;
    CPLFILES  *cfCPlCache;

    TEXTMETRIC tm;


    cfCPlFiles = cfCPlCache = NULL;

    //////////////////////////////////////////////////////////////////////
    //  Check "Cache Valid" flag
    //////////////////////////////////////////////////////////////////////

    if (!IsCacheValid())
        return FALSE;

    //////////////////////////////////////////////////////////////////////
    //  Check "Display Type" flags against current display type
    //////////////////////////////////////////////////////////////////////

    hdc = GetDC (NULL);

    GetTextMetrics (hdc, &tm);

    if ((GetDeviceCaps (hdc, BITSPIXEL) != (int) GetCachedValue (pszBitspixel))
       || (GetDeviceCaps (hdc, COLORRES) != (int) GetCachedValue (pszColorres))
       || (GetDeviceCaps (hdc, PLANES) != (int) GetCachedValue (pszPlanes))
       || (tm.tmAveCharWidth != (LONG) GetCachedValue (pszAveCharWidth))
       || (tm.tmHeight != (LONG) GetCachedValue (pszHeight))
       || (tm.tmWeight != (LONG) GetCachedValue (pszWeight)))
    {
        bRet = FALSE;
        goto CommonExit;
    }

    ReleaseDC (NULL, hdc);


    //////////////////////////////////////////////////////////////////////
    //  Check Filesystem .CPL modules against Cached Modules
    //////////////////////////////////////////////////////////////////////

    //  Get some storage for modules
    cfCPlFiles = (CPLFILES *) LocalAlloc (LPTR, sizeof(CPLFILES)*CPL_MAXMODS);

    cfCPlCache = (CPLFILES *) LocalAlloc (LPTR, sizeof(CPLFILES)*CPL_MAXMODS);

    if (!cfCPlFiles || !cfCPlCache)
    {
        RIPMEM();
        bRet = FALSE;
        goto CommonExit;
    }

    //  Now get info on CPl files and Cached modules
    if ((numFiles = CheckFindModules (cfCPlFiles)) == 0)
    {
        bRet = FALSE;
        goto CommonExit;
    }

    if ((numCache = CheckGetCacheMods (cfCPlCache)) == 0)
    {
        bRet = FALSE;
        goto CommonExit;
    }

    //  Compare until everything matches up, if error invalid cache

    //  Check for quick exits

    if (numFiles != numCache)
    {
        bRet = FALSE;
        goto CommonExit;
    }


    //////////////////////////////////////////////////////////////////////
    //  Now match up and compare cached and filesystem modules
    //////////////////////////////////////////////////////////////////////

    for (i = 0; i < numFiles; i++)
    {
//        if (cfCPlFiles[i].bAlreadyMatched)
//            continue;

        for (j = 0; j < numCache; j++)
        {
            if (cfCPlCache[j].bAlreadyMatched)
                continue;

            if (!_tcsicmp (cfCPlCache[j].szPathname, cfCPlFiles[i].szPathname))
            {
                //  We found a name match
                cfCPlCache[j].bAlreadyMatched = TRUE;
                cfCPlFiles[i].bAlreadyMatched = TRUE;

                //  Check size and then filetimes
                if (cfCPlCache[j].dwSize != cfCPlFiles[i].dwSize)
                {
                    bRet = FALSE;
                    goto CommonExit;
                }

                if (CompareFileTime (&cfCPlCache[j].ftModule,
                                      &cfCPlFiles[i].ftModule))
                {
                    bRet = FALSE;
                    goto CommonExit;
                }
            }
        }
    }

    //  Are there any unmatched files?

    for (i = 0; i < numFiles; i++)
    {
        if (!cfCPlFiles[i].bAlreadyMatched)
        {
            bRet = FALSE;
            goto CommonExit;
        }
    }

    for (i = 0; i < numCache; i++)
    {
        if (!cfCPlCache[i].bAlreadyMatched)
        {
            bRet = FALSE;
            goto CommonExit;
        }
    }

    bRet = TRUE;


    //////////////////////////////////////////////////////////////////////
    //  Common exit code
    //////////////////////////////////////////////////////////////////////

CommonExit:

    if (cfCPlFiles)
        LocalFree ((HLOCAL) cfCPlFiles);

    if (cfCPlCache)
        LocalFree ((HLOCAL) cfCPlCache);

    if (!bRet)
        ClearCacheValid ();

    return bRet;
}


////////////////////////////////////////////////////////////////////////////
//
// Init the icon handle, and ptrs to the strings.
//
////////////////////////////////////////////////////////////////////////////

void CacheSetAppItems(PCPLMODULE pCPlMod, PCPLAPPLET pCPlApp, int iIndex)
{
    HMENU   hMenu;
    HDC     hDC;
    HFONT   hFont;
    TCHAR   szBuffer[128];
    LPNTCPL lpNtCPl;

    if (!hCPlWnd)           // don't add the LB stuff if we aren't showing
        return;             // the main window

    hDC = GetDC (NULL);
    hFont = SelectObject (hDC, hCPlFont);

    lstrcpy(szBuffer, pCPlApp->pszFullName);

    // longest name determines column width

    //  Get the name length without '&' and adjust the longest name
    //  Also add & before the first char if there is none

    if ( (pCPlApp->iNameW = GetNameExtent(hDC, szBuffer)) > iWidth)
        iWidth = pCPlApp->iNameW;

    SelectObject (hDC, hFont);
    ReleaseDC (NULL, hDC);

    ScanName (szBuffer);    // adds '&'

    //  Append a new menu item to the Settings pop-up menu
    lstrcat (szBuffer, szDots);
    hMenu = GetMenu (hCPlWnd);
    hMenu = GetSubMenu (hMenu, 0);
    InsertMenu (hMenu, numApps,
                (!numApps || (numApps & 0x0F)) ?
                MF_STRING|MF_BYPOSITION : MF_STRING|MF_MENUBARBREAK|MF_BYPOSITION,
                numApps+MENU_SETTINGS, szBuffer);

    numApps++;

    lpNtCPl = (LPNTCPL) LocalAlloc (LPTR, sizeof(NTCPL));

    if (lpNtCPl)
    {
        lpNtCPl->pCPlMod = pCPlMod;
        lpNtCPl->iApplet = iIndex;
        SendMessage (hCPlLB, LB_ADDSTRING, 0, (LONG)lpNtCPl);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Load cached Control Panel items from registry keys
//
////////////////////////////////////////////////////////////////////////////

BOOL LoadFromCache()
{
    int    i, j;
    DWORD  nApplets;
    DWORD  dwBits;
    DWORD  dwSize;
    DWORD  dwType;

    HKEY   hkey       = NULL;
    HKEY   hkeyCache  = NULL;
    HKEY   hkeyRCache = NULL;
    HANDLE hMod       = NULL;
    HANDLE hFile      = NULL;
    HLOCAL lhndBits   = NULL;
    LPBYTE lpBits     = NULL;

    PCPLMODULE  pCPlMod;
    PCPLAPPLET  pCPlApp;
    ICONINFO    iconinfo;
    BITMAP      bmInfo;
    REGKEY     *prkFirst = NULL;
    REGKEY     *prkRegKey = NULL;

    TCHAR szTemp[MAX_PATH];


    //////////////////////////////////////////////////////////////////////
    //  Open cache of modules and applets, create local modules
    //
    //  Enumerate each "Module" key under "Cache" and then
    //  enumerate each "Applet" key under "Module" key and
    //  build linked list of Modules and Applets from this
    //  instead of from searching file system.
    //
    //////////////////////////////////////////////////////////////////////

    if (RegOpenKeyEx (HKEY_CURRENT_USER, pszRegCache, 0L, KEY_ALL_ACCESS,
                        &hkeyRCache)
            != ERROR_SUCCESS)
    {
        RIPREG();
        goto RegistryError;
    }

    if (lhndBits = LocalAlloc (LMEM_FIXED, BMPSIZE))
        lpBits = LocalLock (lhndBits);
    else
    {
        RIPMEM();
        goto MemoryError;
    }

    //  Get linked list of all "Module keys" for use below
    if (!(prkFirst = GetModuleKeyList (hkeyRCache)))
    {
        RIPMEM();
        goto MemoryError;
    }


    //////////////////////////////////////////////////////////////////////
    //  Now go thru list of modules
    //////////////////////////////////////////////////////////////////////

    prkRegKey = prkFirst;
    i = 0;

    while (prkRegKey)
    {
        //  Open cache of module applets
        if (RegOpenKeyEx (hkeyRCache,                // Root key
                          prkRegKey->pszKeyName,     // Subkey to open/create
                          0L,                        // Reserved
                          KEY_ALL_ACCESS,            // SAM
                          &hkeyCache)                // return handle
                != ERROR_SUCCESS)
        {
            RIPREG();
            goto RegistryError;
        }

        //  Get useful Module info - like number of applets

        nApplets = 0;
        dwSize = sizeof(DWORD);

        if (RegQueryValueEx (hkeyCache, pszNumApplets, 0L, &dwType,
                             (LPBYTE) &nApplets, &dwSize)
                != ERROR_SUCCESS)
        {
            RIPREG();
            goto RegistryError;
        }

        //////////////////////////////////////////////////////////////////
        //  If numApplets == 0 - this is a Cache entry for an invalid
        //  or bad .CPL file/module - just ignore it and continue
        //////////////////////////////////////////////////////////////////

        if (nApplets == 0)
            goto GetNextCacheEntry;

        if (!(pCPlMod = (PCPLMODULE)LocalAlloc (LPTR, sizeof(CPLMODULE))))
        {
            RIPMEM();
            goto MemoryError;
        }

        pCPlMod->hLibrary = NULL;
        pCPlMod->bLoaded  = FALSE;

        pCPlMod->lpfnCPlApplet = NULL;
        pCPlMod->pCPlApps      = NULL;

        CPlMods[i] = pCPlMod;

        //  Module pathname 
        dwSize = sizeof(pCPlMod->szPathname);

        if ((RegQueryValueEx (hkeyCache, pszModulePath, 0L, &dwType,
                              (LPBYTE) pCPlMod->szPathname, &dwSize))
                != ERROR_SUCCESS)
        {
            RIPREG();
            goto RegistryError;
        }

        //  Get some space for this module's applets

        pCPlMod->numApplets = nApplets;

        if (!(pCPlMod->pCPlApps = (PCPLAPPLET)LocalAlloc(LPTR,
                                            nApplets*sizeof(CPLAPPLET))))
        {
            RIPGEN();
            goto GeneralError;
        }

        //  Open each applet subkey and get info, create icon, etc.

        for (j = 0, pCPlApp = pCPlMod->pCPlApps; j < (int) nApplets; j++, pCPlApp++)
        {
            wsprintf (szTemp, TEXT("%d"), j);

            if (RegOpenKey (hkeyCache, szTemp, &hkey) == ERROR_SUCCESS)
            {
                //  Get all applet info into pCPlApp struct and create
                //  ICON from binary data

                //  Get icon info

                iconinfo.fIcon = TRUE;

                //  IconX
                dwSize = sizeof(DWORD);

                if ((RegQueryValueEx (hkey, pszAppletIconX, 0L,
                                      &dwType,
                                      (LPBYTE) &iconinfo.xHotspot,
                                      &dwSize))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }

                //  IconY
                dwSize = sizeof(DWORD);

                if ((RegQueryValueEx (hkey,
                                      pszAppletIconY,
                                      0L,
                                      &dwType,
                                      (LPBYTE) &iconinfo.yHotspot,
                                      &dwSize))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }

                //  Create Mask bitmap
                dwSize = sizeof(BITMAP);

                if ((RegQueryValueEx (hkey,
                                      pszAppletInfoM,
                                      0L,
                                      &dwType,
                                      (LPBYTE) &bmInfo,
                                      &dwSize))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }

                dwBits = BMPSIZE;
                if ((RegQueryValueEx (hkey,
                                      pszAppletIconM,
                                      0L,
                                      &dwType,
                                      (LPBYTE) lpBits,
                                      &dwBits))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }
                bmInfo.bmBits = lpBits;

                if (!(iconinfo.hbmMask = CreateBitmapIndirect (&bmInfo)))
                {
                    RIPGEN();
                    goto BitmapError;
                }

                //  Create Color bitmap
                dwSize = sizeof(BITMAP);

                if ((RegQueryValueEx (hkey,
                                      pszAppletInfoC,
                                      0L,
                                      &dwType,
                                      (LPBYTE) &bmInfo,
                                      &dwSize))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }

                dwBits = BMPSIZE;
                if ((RegQueryValueEx (hkey,
                                      pszAppletIconC,
                                      0L,
                                      &dwType,
                                      (LPBYTE) lpBits,
                                      &dwBits))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }
                bmInfo.bmBits = lpBits;

                if (!(iconinfo.hbmColor = CreateBitmapIndirect (&bmInfo)))
                {
                    RIPGEN();
                    goto BitmapError;
                }

                pCPlApp->hIcon = CreateIconIndirect (&iconinfo);

                // Now cleanup and get rid of our old bitmaps

                DeleteObject (iconinfo.hbmMask);
                DeleteObject (iconinfo.hbmColor);

                //  Get other applet string and help info

                //  Name
                dwSize = BMPSIZE;

                if ((RegQueryValueEx (hkey, pszAppletName, 0L, &dwType,
                                      (LPBYTE) lpBits, &dwSize))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }

                if (pCPlApp->pszName = (LPTSTR) LocalAlloc (LPTR,
                                       ByteCountOf(lstrlen((LPTSTR)lpBits)+1)))
                {
                    lstrcpy(pCPlApp->pszName, (LPTSTR)lpBits);
                }
                else
                {
                    RIPMEM();
                    goto MemoryError;
                }

                //  Full Namewith '&' char
                dwSize = BMPSIZE;

                if ((RegQueryValueEx (hkey, pszAppletFull, 0L, &dwType,
                                      (LPBYTE) lpBits, &dwSize))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }

                if (pCPlApp->pszFullName = (LPTSTR) LocalAlloc (LPTR,
                                       ByteCountOf(lstrlen((LPTSTR)lpBits)+1)))
                {
                    lstrcpy(pCPlApp->pszFullName, (LPTSTR)lpBits);
                }
                else
                {
                    RIPMEM();
                    goto MemoryError;
                }

                //  Info
                dwSize = BMPSIZE;

                if ((RegQueryValueEx (hkey, pszAppletInfo, 0L, &dwType,
                                      (LPBYTE) lpBits, &dwSize))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }

                if (pCPlApp->pszInfo = (LPTSTR) LocalAlloc (LPTR,
                                       ByteCountOf(lstrlen((LPTSTR)lpBits)+1)))
                {
                    lstrcpy(pCPlApp->pszInfo, (LPTSTR)lpBits);
                }
                else
                {
                    RIPMEM();
                    goto MemoryError;
                }

                //  Helpfile name
                //
                //  NOTE: This is an optional value, it is not an ERROR to
                //        not read this value, just set ptr to NULL
                //                  
                dwSize = BMPSIZE;

                if ((RegQueryValueEx (hkey, pszAppletHelp, 0L, &dwType,
                                      (LPBYTE) lpBits, &dwSize))
                        == ERROR_SUCCESS)
                {
                    if (pCPlApp->pszHelpFile = (LPTSTR) LocalAlloc (LPTR,
                                      ByteCountOf(lstrlen((LPTSTR)lpBits)+1)))
                    {
                        lstrcpy(pCPlApp->pszHelpFile, (LPTSTR)lpBits);
                    }
                    else
                    {
                        RIPMEM();
                        goto MemoryError;
                    }
                }
                else
                {
                    pCPlApp->pszHelpFile = NULL;
                }

                //  Help context
                dwSize = sizeof(DWORD);

                if ((RegQueryValueEx (hkey, pszAppletCntx, 0L, &dwType,
                                 (LPBYTE) &pCPlApp->dwContext, &dwSize))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }

                //  lData value
                dwSize = sizeof(DWORD);

                if ((RegQueryValueEx (hkey, pszAppletData, 0L, &dwType,
                                 (LPBYTE) &pCPlApp->lData, &dwSize))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }
            }
            else
            {
                RIPREG();
                goto RegistryError;
            }

            //  Set all application items for Listbox control and drawing
            CacheSetAppItems(pCPlMod, pCPlApp, j);

            RegCloseKey (hkey);
            hkey = NULL;
        }

        i++;

GetNextCacheEntry:

        RegCloseKey (hkeyCache);
        hkeyCache = NULL;

        //  Next key name in list
        prkRegKey = prkRegKey->prkNext;
    }

    //////////////////////////////////////////////////////////////////////
    //  Clean up and leave
    //////////////////////////////////////////////////////////////////////

    //  Free list

    if (!FreeModuleKeyList (prkFirst))
    {
        RIPMEM();
        goto MemoryError;
    }

    if (lhndBits != NULL)
    {
        LocalUnlock (lhndBits);
        if (LocalFree (lhndBits))
        {
            RIPMEM();
            goto MemoryError;
        }
    }

    if (hkeyRCache != NULL)
        RegCloseKey (hkeyRCache);

    return TRUE;


    //////////////////////////////////////////////////////////////////////
    //  Routine error handling
    //////////////////////////////////////////////////////////////////////

BitmapError:
GeneralError:
MemoryError:
RegistryError:

    if (hkeyRCache != NULL)
        RegCloseKey (hkeyRCache);

    if (hkey != NULL)
        RegCloseKey (hkey);

    if (hkeyCache != NULL)
        RegCloseKey (hkeyCache);

    if (lhndBits != NULL)
    {
        LocalUnlock (lhndBits);
        LocalFree (lhndBits);
    }

    FreeModuleKeyList (prkFirst);

    return FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Clear all registry Module Cache entries under "Control Panel\Cache"
//  key.  Also, delete their associated Applet cache entries.
//
//  This routine returns no errors because there is the possibility of an
//  empty cache and; hence, nothing for it to do. (i.e. Initial run)
//
////////////////////////////////////////////////////////////////////////////

void FlushCache()
{
    int    i;
    DWORD  dwRes;
    DWORD  dwBufz;
    TCHAR  szTemp[MAX_PATH];

    HKEY   hkeyCache  = NULL;
    HKEY   hkeyRCache = NULL;

    REGKEY   *prkFirst = NULL;
    REGKEY   *prkRegKey = NULL;
    FILETIME  ftReg;


    //////////////////////////////////////////////////////////////////////
    //  Open cache of modules and applets
    //////////////////////////////////////////////////////////////////////

    if (RegOpenKeyEx (HKEY_CURRENT_USER, pszRegCache, 0L, KEY_ALL_ACCESS,
                        &hkeyRCache)
            != ERROR_SUCCESS)
    {
        return;
    }

    //  Get linked list of all "Module keys" for use below
    prkFirst = GetModuleKeyList (hkeyRCache);


    //////////////////////////////////////////////////////////////////////
    //  Now go thru list of modules
    //////////////////////////////////////////////////////////////////////

    prkRegKey = prkFirst;

    while (prkRegKey)
    {
        //  Open cache of module applets
        if (RegOpenKeyEx (hkeyRCache,                // Root key
                          prkRegKey->pszKeyName,     // Subkey to open/create
                          0L,                        // Reserved
                          KEY_ALL_ACCESS,            // SAM
                          &hkeyCache)                // return handle
                != ERROR_SUCCESS)
        {
            continue;
        }

        dwRes = 0;
        dwBufz = CharSizeOf(szTemp);

        //////////////////////////////////////////////////////////////
        //  Delete each applet subkey under the Module key
        //////////////////////////////////////////////////////////////

        //  Get number of subkeys
        if (RegQueryInfoKey (hkeyCache, // handle of key to query
                             szTemp,    // ptr class string
                             &dwBufz,   // ptr size class string buffer
                             NULL,      // reserved
                             &dwRes,    // ptr number of subkeys
                             &dwBufz,   // ptr longest subkey name length
                             &dwBufz,   // ptr longest class string length
                             &dwBufz,   // ptr number of value entries
                             &dwBufz,   // ptr longest value name length
                             &dwBufz,   // ptr longest value data length
                             &dwBufz,   // ptr security descriptor length
                             &ftReg)    // ptr last write time
                == ERROR_SUCCESS)
        {
            //  Now, since we have a count on the number of
            //  keys, delete them

            for (i = 0; i < (int) dwRes; i++)
            {
                wsprintf (szTemp, TEXT("%d"), i);
                RegDeleteKey (hkeyCache, szTemp);
            }
        }

        RegCloseKey (hkeyCache);
        hkeyCache = NULL;

        //////////////////////////////////////////////////////////////
        //  Now delete this Module key
        //////////////////////////////////////////////////////////////

        RegDeleteKey (hkeyRCache, prkRegKey->pszKeyName);

        //  Get next key name in list
        prkRegKey = prkRegKey->prkNext;
    }

    //////////////////////////////////////////////////////////////////////
    //  Clean up and leave
    //////////////////////////////////////////////////////////////////////

    //  Free list
    FreeModuleKeyList (prkFirst);

    ASSERT(hkeyCache == NULL);

    if (hkeyRCache != NULL)
        RegCloseKey (hkeyRCache);

    return;
}


////////////////////////////////////////////////////////////////////////////
//
//  Force a complete rebuild of registry cache by setting cache invalid
//  and reloading in all modules from scratch.
//
////////////////////////////////////////////////////////////////////////////

void RebuildCache()
{
    HMENU hMenu;
    int   i, iApps;


    //
    //  If an applet is currently displayed, just set global
    //  flag to force CACHE rebuild after if goes away.
    //

    if (bAppletActive)
    {
        bRebuildCache = TRUE;
        return;
    }

    //  Make a copy becaue LB_RESETCONTENT causes it decrement to 0
    iApps = numApps;

    //  Clear current list box contents
    SendMessage (hCPlLB, LB_RESETCONTENT, 0, 0L);

    UpdateWindow (hCPlLB);

    //  Delete current menu items

    hMenu = GetMenu (hCPlWnd);
    hMenu = GetSubMenu (hMenu, 0);

    for (i = 0; i < iApps; i++)
        DeleteMenu (hMenu, MENU_SETTINGS+i, MF_BYCOMMAND);

    //  Dirty the cache
    ClearCacheValid ();

    //  Re-init control panel module structs and refresh cache
    if (!LoadAndSizeApplets())
    {
        if (hCPlWnd)
            MessageBox (hCPlWnd, szErrNoApps, szAppName,
                            MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
    }

    bRebuildCache = FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Update Control Panel module/applet cache in registry.  Delete any       
//  existing cache entries and create new keys based on module chain
//  created during initial search and loading of modules/applets.
//
////////////////////////////////////////////////////////////////////////////

void UpdateCache()
{
    int    i, j, k, nApplets;
    DWORD  dwDisposition;
    DWORD  dwBits;
    DWORD  numFiles = 0;
    DWORD  numCache = 0;

    HDC    hdc;
    HKEY   hkey      = NULL;
    HKEY   hkeyCache = NULL;
    HANDLE hFile     = NULL;
    HLOCAL lhndBits  = NULL;
    LPBYTE lpBits    = NULL;

    PCPLMODULE  pCPlMod;
    PCPLAPPLET  pCPlApp;
    ICONINFO    iconinfo;
    BITMAP      bmInfo;

    BY_HANDLE_FILE_INFORMATION bhfiMod;

    TCHAR szTemp[MAX_PATH];
    TCHAR szCacheKey[MAX_PATH];

    CPLFILES  *cfCPlFiles;
    CPLFILES  *cfCPlCache;

    TEXTMETRIC tm;


    //////////////////////////////////////////////////////////////////////
    //  Clear "Cache Valid" flag
    //////////////////////////////////////////////////////////////////////

    cfCPlFiles = cfCPlCache = NULL;

    ClearCacheValid ();


    //////////////////////////////////////////////////////////////////////
    //  First thing to do is flush cache of all existing cache entries
    //////////////////////////////////////////////////////////////////////

    FlushCache ();

    //////////////////////////////////////////////////////////////////////
    //  Create cache of modules and applets
    //////////////////////////////////////////////////////////////////////

    if (lhndBits = LocalAlloc (LMEM_FIXED, BMPSIZE))
        lpBits = LocalLock (lhndBits);
    else
    {
        RIPMEM();
        goto MemoryError;
    }

    i = 0;

    while (CPlMods[i] != NULL)
    {
        pCPlMod = CPlMods[i];

        k = 0;

CreateKeyAgain:

        CreateCacheKey (szTemp, pCPlMod->szPathname, k);

        hkeyCache = NULL;
        wsprintf (szCacheKey, pszCplCache, szTemp);

        if (RegCreateKeyEx (HKEY_CURRENT_USER,         // Root key
                            szCacheKey,                // Subkey to open/create
                            0L,                        // Reserved
                            NULL,                      // Class string
                            0L,                        // Options
                            KEY_ALL_ACCESS,            // SAM
                            NULL,                      // ptr to Security struct
                            &hkeyCache,                // return handle
                            &dwDisposition)            // return disposition
                != ERROR_SUCCESS)
        {
            //  Try it with next value until we get a unique key
            if (++k > 5)
            {
                //  If key creation fails 5 times we have a serious problem
                //  because it is unlikely that we will try to load 5 modules
                //  with the same name.
                RIPREG();
                goto RegistryError;
            }
            else 
                goto CreateKeyAgain;
        }
        else
        {
            //  All existing subkeys should have been deleted
            ASSERT(dwDisposition != REG_OPENED_EXISTING_KEY);

            // Open File and get info on it
            if ((hFile = CreateFile (pCPlMod->szPathname,
                                     0,
                                     FILE_SHARE_WRITE,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL)) == INVALID_HANDLE_VALUE)
            {
                RIPGEN();
                goto GeneralError;
            }

            //  Put some data values into the cache
            //  Get File size, time, num applets, full pathname

            if (!GetFileInformationByHandle (hFile, &bhfiMod))
            {
                RIPGEN();
                goto GeneralError;
            }

            CloseHandle (hFile);

                    // Set "Module Path" value
            if ((RegSetValueEx  (hkeyCache, pszModulePath, 0L, REG_SZ,
                               (LPBYTE) pCPlMod->szPathname,
                               ByteCountOf(lstrlen(pCPlMod->szPathname)+1)))
                ||  // Set "Low File Time" value
                (RegSetValueEx (hkeyCache, pszFileTimeLow, 0L, REG_DWORD,
                               (LPBYTE) &bhfiMod.ftLastWriteTime.dwLowDateTime,
                               sizeof(DWORD)))
                ||  // Set "High File Time" value
                (RegSetValueEx (hkeyCache, pszFileTimeHigh, 0L, REG_DWORD,
                                (LPBYTE) &bhfiMod.ftLastWriteTime.dwHighDateTime,
                                sizeof(DWORD)))
                ||  // Set "File Size" value
                (RegSetValueEx (hkeyCache, pszFileSize, 0L, REG_DWORD,
                                (LPBYTE) &bhfiMod.nFileSizeLow, sizeof(DWORD)))
                || // Set "Number Applets" value
                (RegSetValueEx (hkeyCache, pszNumApplets, 0L, REG_DWORD,
                                (LPBYTE) &pCPlMod->numApplets, sizeof(DWORD)))
                    != ERROR_SUCCESS)
            {
                RIPREG();
                goto RegistryError;
            }

            nApplets = pCPlMod->numApplets;

            //  Create subkey for each applet (use # for speed)

            for (j = 0, pCPlApp = pCPlMod->pCPlApps; j < nApplets; j++, pCPlApp++)
            {
                wsprintf (szTemp, TEXT("%d"), j);

                if (RegCreateKey (hkeyCache, szTemp, &hkey) == ERROR_SUCCESS)
                {
                    //  Save applet items

#ifdef ICONID
    DWORD  dwRes;

//         Copy, duplicate, save icon and store something in registry
//  NOTE:  This is the preferred way to save an icon from the resources
//         that were used to create it initially.  This is both faster
//         and has the benefit of saving all of the icon information.
//         When we would go to create the icon during LoadFromCache()
//         USER would automatically choose the best icon for this display.
            //++++ NEED iICON value from ICON Handle (pCPlApp->hIcon)

                    iIcon = ResIdfromHandle (pCPlMod->hLibrary, pCPlApp->hIcon);

                    hIconRes = FindResource (pCPlMod->hLibrary, MAKEINTRESOURCE(iIcon), MAKEINTRESOURCE(RT_ICON))

                    dwRes = SizeofResource (pCPlMod->hLibrary, hIconRes);
                    hIconRes = LoadResource (pCPlMod->hLibrary, hIconRes);
                    lpIconRes = LockResource (hIconRes);

                    //  Icon
                    if ((RegSetValueEx (hkey, pszAppletIcon, 0L, REG_BINARY,
                                    (LPBYTE) lpIconRes, dwRes))
                        != ERROR_SUCCESS)
                    {
                        RIPREG();
                        goto RegistryError;
                    }

                    dwRes = SizeofResource (pCPlMod->hLibrary, pCPlApp->hIcon);
                    // dwIconId = ExtractIcon (pCPlMod->hLibrary, pCPlApp->hIcon);
#endif  // ICONID

                    //  Save all icon info
                    if (GetIconInfo (pCPlApp->hIcon, &iconinfo))
                    {
                        //  IconX
                        if ((RegSetValueEx (hkey, pszAppletIconX, 0L,
                                            REG_DWORD,
                                            (LPBYTE) &iconinfo.xHotspot,
                                            sizeof(DWORD)))
                                != ERROR_SUCCESS)
                        {
                            RIPREG();
                            goto RegistryError;
                        }
                        //  IconY
                        if ((RegSetValueEx (hkey,
                                            pszAppletIconY,
                                            0L,
                                            REG_DWORD,
                                            (LPBYTE) &iconinfo.yHotspot,
                                            sizeof(DWORD)))
                                != ERROR_SUCCESS)
                        {
                            RIPREG();
                            goto RegistryError;
                        }

                        if (iconinfo.hbmMask)
                        {
                            if (dwBits = GetObject (iconinfo.hbmMask,
                                          sizeof(BITMAP), (LPTSTR) &bmInfo))
                            {
                                if ((RegSetValueEx (hkey,
                                                    pszAppletInfoM,
                                                    0L,
                                                    REG_BINARY,
                                                    (LPBYTE) &bmInfo,
                                                    dwBits))
                                        != ERROR_SUCCESS)
                                {
                                    RIPREG();
                                    goto RegistryError;
                                }

                            }

                            dwBits = GetBitmapBits (iconinfo.hbmMask,
                                                    BMPSIZE,
                                                    (LPVOID) lpBits);
    
                            if ((RegSetValueEx (hkey,
                                                pszAppletIconM,
                                                0L,
                                                REG_BINARY,
                                                (LPBYTE) lpBits,
                                                dwBits))
                                    != ERROR_SUCCESS)
                            {
                                RIPREG();
                                goto RegistryError;
                            }
                            DeleteObject (iconinfo.hbmMask);
                        }

                        if (iconinfo.hbmColor)
                        {
                            if (dwBits = GetObject (iconinfo.hbmColor,
                                          sizeof(BITMAP), (LPTSTR) &bmInfo))
                            {
                                if ((RegSetValueEx (hkey,
                                                    pszAppletInfoC,
                                                    0L,
                                                    REG_BINARY,
                                                    (LPBYTE) &bmInfo,
                                                    dwBits))
                                        != ERROR_SUCCESS)
                                {
                                    RIPREG();
                                    goto RegistryError;
                                }

                            }

                            dwBits = GetBitmapBits (iconinfo.hbmColor,
                                                    BMPSIZE,
                                                    (LPVOID) lpBits);
    
                            if ((RegSetValueEx (hkey,
                                                pszAppletIconC,
                                                0L,
                                                REG_BINARY,
                                                (LPBYTE) lpBits,
                                                dwBits))
                                    != ERROR_SUCCESS)
                            {
                                RIPREG();
                                goto RegistryError;
                            }
                            DeleteObject (iconinfo.hbmColor);
                        }
                    }
                            //  Name
                    if ((RegSetValueEx (hkey, pszAppletName, 0L, REG_SZ,
                                    (LPBYTE) pCPlApp->pszName,
                                    ByteCountOf(lstrlen (pCPlApp->pszName)+1)))
                        ||  //  Full Name includes '&' char
                        (RegSetValueEx (hkey, pszAppletFull, 0L, REG_SZ,
                                    (LPBYTE) pCPlApp->pszFullName,
                                    ByteCountOf(lstrlen (pCPlApp->pszFullName)+1)))
                        ||  //  Info
                        (RegSetValueEx (hkey, pszAppletInfo, 0L, REG_SZ,
                                    (LPBYTE) pCPlApp->pszInfo,
                                    ByteCountOf(lstrlen (pCPlApp->pszInfo)+1)))
                        ||  //  Help context
                        (RegSetValueEx (hkey, pszAppletCntx, 0L, REG_DWORD,
                                    (LPBYTE) &pCPlApp->dwContext, sizeof(DWORD)))
                        ||  //  lData value
                        (RegSetValueEx (hkey, pszAppletData, 0L, REG_DWORD,
                                    (LPBYTE) &pCPlApp->lData, sizeof(DWORD)))
                            != ERROR_SUCCESS)
                    {
                        RIPREG();
                        goto RegistryError;
                    }

                    //  Helpfile name - Optional
                    if (pCPlApp->pszHelpFile != NULL)
                    {
                        if ((RegSetValueEx (hkey, pszAppletHelp, 0L, REG_SZ,
                                (LPBYTE) pCPlApp->pszHelpFile,
                                ByteCountOf(lstrlen (pCPlApp->pszHelpFile)+1)))
                            != ERROR_SUCCESS)
                        {
                            RIPREG();
                            goto RegistryError;
                        }
                    }
                }
                else
                {
                    RIPREG();
                    goto RegistryError;
                }

                RegCloseKey (hkey);
                hkey = NULL;
            }   
        }

        RegCloseKey (hkeyCache);
        hkeyCache = NULL;
        i++;
    }

    //////////////////////////////////////////////////////////////////////
    //  Now create a cache entry for any .CPL modules found that either
    //  did not load or are not valid modules
    //////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////
    //  Check Filesystem .CPL modules against Cached Modules
    //////////////////////////////////////////////////////////////////////

    //  Get some storage for modules
    cfCPlFiles = (CPLFILES *) LocalAlloc (LPTR, sizeof(CPLFILES)*CPL_MAXMODS);

    cfCPlCache = (CPLFILES *) LocalAlloc (LPTR, sizeof(CPLFILES)*CPL_MAXMODS);

    if (!cfCPlFiles || !cfCPlCache)
    {
        RIPMEM();
        goto MemoryError;
    }

    //  Now get info on CPl files and Cached modules
    if ((numFiles = CheckFindModules (cfCPlFiles)) == 0)
    {
        RIPGEN();
        goto GeneralError;
    }

    if ((numCache = CheckGetCacheMods (cfCPlCache)) == 0)
    {
        RIPGEN();
        goto GeneralError;
    }

    //  Check for quick exits

    if (numFiles == numCache)
        goto CleanExit;

    ASSERT(numCache <= numFiles);


    //////////////////////////////////////////////////////////////////////
    //  At this point we have an extra module found during the filesystem
    //  search.  Identify it and create a cache entry for it.
    //////////////////////////////////////////////////////////////////////

    for (i = 0; i < (int) numFiles; i++)
    {
        if (cfCPlFiles[i].bAlreadyMatched)
            continue;

        for (j = 0; j < (int) numCache; j++)
        {
            if (cfCPlCache[j].bAlreadyMatched)
                continue;

            if (!_tcsicmp (cfCPlCache[j].szPathname, cfCPlFiles[i].szPathname))
            {
                //  We found a name match
                cfCPlCache[j].bAlreadyMatched = TRUE;
                cfCPlFiles[i].bAlreadyMatched = TRUE;
            }
        }
    }

    //  Find the extra .CPL files/modules

    for (i = 0; i < (int) numFiles; i++)
    {
        if (!cfCPlFiles[i].bAlreadyMatched)
        {
            //  Create a cache entry for this unmatched file/module
            k = 0;

TryCreateKeyAgain:

            CreateCacheKey (szTemp, cfCPlFiles[i].szPathname, k);

            hkeyCache = NULL;
            wsprintf (szCacheKey, pszCplCache, szTemp);

            if (RegCreateKeyEx (HKEY_CURRENT_USER,         // Root key
                                szCacheKey,                // Subkey to open/create
                                0L,                        // Reserved
                                NULL,                      // Class string
                                0L,                        // Options
                                KEY_ALL_ACCESS,            // SAM
                                NULL,                      // ptr to Security struct
                                &hkeyCache,                // return handle
                                &dwDisposition)            // return disposition
                    != ERROR_SUCCESS)
            {
                //  Try it with next value until we get a unique key
                if (++k > 5)
                {
                    //  If key creation fails 5 times we have a serious problem
                    //  because it is unlikely that we will try to load 5 modules
                    //  with the same name.
                    RIPREG();
                    goto RegistryError;
                }
                else 
                    goto TryCreateKeyAgain;
            }
            else
            {
                //  This condition would be a serious problem
                ASSERT(dwDisposition != REG_OPENED_EXISTING_KEY);


                ////////////////////////////////////////////////////////////
                //  SPECIAL NOTE:  Here we set the cache entries for the
                //  invalid module.  We know it is invalid because the
                //  numApplets ("Number Applets") field is 0.  This is
                //  the default set by CheckFindModules().
                ////////////////////////////////////////////////////////////
            
                        // Set "Module Path" value
                if ((RegSetValueEx  (hkeyCache, pszModulePath, 0L, REG_SZ,
                                   (LPBYTE) cfCPlFiles[i].szPathname,
                                   ByteCountOf(lstrlen(cfCPlFiles[i].szPathname)+1)))
                    ||  // Set "Low File Time" value
                    (RegSetValueEx (hkeyCache, pszFileTimeLow, 0L, REG_DWORD,
                                   (LPBYTE) &cfCPlFiles[i].ftModule.dwLowDateTime,
                                   sizeof(DWORD)))
                    ||  // Set "High File Time" value
                    (RegSetValueEx (hkeyCache, pszFileTimeHigh, 0L, REG_DWORD,
                                    (LPBYTE) &cfCPlFiles[i].ftModule.dwHighDateTime,
                                    sizeof(DWORD)))
                    ||  // Set "File Size" value
                    (RegSetValueEx (hkeyCache, pszFileSize, 0L, REG_DWORD,
                                    (LPBYTE) &cfCPlFiles[i].dwSize, sizeof(DWORD)))
                    || // Set "Number Applets" value
                    (RegSetValueEx (hkeyCache, pszNumApplets, 0L, REG_DWORD,
                                    (LPBYTE) &cfCPlFiles[i].numApplets, sizeof(DWORD)))
                        != ERROR_SUCCESS)
                {
                    RIPREG();
                    goto RegistryError;
                }
            }

            RegCloseKey (hkeyCache);
            hkeyCache = NULL;
        }
    }

#if DBG
    // NOTE:  This should only be in the debug or "checked" build
    //   
    //  We should never have this condition

    for (i = 0; i < (int) numCache; i++)
    {
        if (!cfCPlCache[i].bAlreadyMatched)
        {
            RIPGEN();
            goto GeneralError;
        }
    }
#endif  // DBG

    //////////////////////////////////////////////////////////////////////
    //  Indicate "Cache Valid"
    //////////////////////////////////////////////////////////////////////
CleanExit:

    //  Set "Display Type" flags for current display type

    hdc = GetDC (NULL);

    SetCachedValue (pszBitspixel, GetDeviceCaps (hdc, BITSPIXEL));
    SetCachedValue (pszColorres, GetDeviceCaps (hdc, COLORRES));
    SetCachedValue (pszPlanes, GetDeviceCaps (hdc, PLANES));

    GetTextMetrics (hdc, &tm);

    SetCachedValue (pszAveCharWidth, tm.tmAveCharWidth);
    SetCachedValue (pszHeight, tm.tmHeight);
    SetCachedValue (pszWeight, tm.tmWeight);

    ReleaseDC (NULL, hdc);

    SetCacheValid ();


    //////////////////////////////////////////////////////////////////////
    //  Cleanup and leave
    //////////////////////////////////////////////////////////////////////

    ASSERT(lhndBits != NULL)

    LocalUnlock (lhndBits);
    LocalFree (lhndBits);

    if (cfCPlFiles)
        if (LocalFree ((HLOCAL) cfCPlFiles))
        {
            RIPMEM();
            goto MemoryError;
        }

    if (cfCPlCache)
        if (LocalFree ((HLOCAL) cfCPlCache))
        {
            RIPMEM();
            goto MemoryError;
        }

    return;


    //////////////////////////////////////////////////////////////////////
    //  Routine error handling
    //////////////////////////////////////////////////////////////////////

GeneralError:
MemoryError:
RegistryError:

    if (hkey != NULL)
        RegCloseKey (hkey);

    if (hkeyCache != NULL)
        RegCloseKey (hkeyCache);

    if (lhndBits != NULL)
    {
        LocalUnlock (lhndBits);
        LocalFree (lhndBits);
    }

    if (cfCPlFiles)
        LocalFree ((HLOCAL) cfCPlFiles);

    if (cfCPlCache)
        LocalFree ((HLOCAL) cfCPlCache);

    return;
}


////////////////////////////////////////////////////////////////////////////
//
//  Pass initial messages to the indicated module, and match its' responses
//  against what we know about it from our cache entry for it.
//
//  NOTE: We cannot even get to this routine unless we have already verified
//        that this module exists in the file system, and that we already
//        have a cache entry for it.  What we are doing in this routine is
//        verifying that it matches what we know about it.
//
// Arguments:
//    name        -  the name of the .CPL module to verify
//    pcfCPlCache -  ptr to a struct containing cache entry info that
//                   matches the name of the module
//
////////////////////////////////////////////////////////////////////////////

//
//  This value is a special value only sent out in LPARAM2 of the
//  CPL_INIT, CPL_GETCOUNT and CPL_EXIT messages only during the
//  Control Panels Validation thread to let certain CPL applets
//  know that they are in the validation portion of the Control
//  Panel.  This will allow them to respond quicker and not start-
//  up unnecessary services, timers, etc.
//

#define VALIDATE_PARAM   ((LPARAM)(-1))

BOOL ValidateAskModule (LPTSTR name, CPLFILES *pcfCPlCache)
{
    int         numApplets = 0;
    BOOL        bValid=FALSE;
    HANDLE      hCpl;
    APPLET_PROC lpfnCPlApplet;


    ////////////////////////////////////////////////////////////////////////
    //  Load module, initialize it and get applet count to test against
    ////////////////////////////////////////////////////////////////////////

    if (hCpl = LoadLibrary(name))
    {
        if ((lpfnCPlApplet = (APPLET_PROC)GetProcAddress(hCpl, szCPlApplet))
             && (*lpfnCPlApplet)(hCPlWnd, CPL_INIT, 0L, VALIDATE_PARAM))
        {
            numApplets = (int)(*lpfnCPlApplet)(hCPlWnd, CPL_GETCOUNT, 0L, VALIDATE_PARAM);

            //  If the module loads and the number of applets are
            //  the same as my Cached module, then everything is OK.

            if (pcfCPlCache->numApplets == numApplets)
                bValid = TRUE;

            //  Send exit message before freeing the module
            (*lpfnCPlApplet) (hCPlWnd, CPL_EXIT, 0L, VALIDATE_PARAM);
        }

        FreeLibrary(hCpl);
    }

    ////////////////////////////////////////////////////////////////////////
    //  Check for the case where we already know about a module and have
    //  marked it with 0 applets.  If this module fails to load, it's OK.
    ////////////////////////////////////////////////////////////////////////

    if (!bValid && (pcfCPlCache->numApplets == 0) && numApplets == 0)
        bValid = TRUE;

    return bValid;
}


////////////////////////////////////////////////////////////////////////////
//
//  Load all modules found during Filesystem search and check them against
//  what we loaded from cache to be "Absolutely" certain that what we think
//  is a complete set of applet icons is indeed what would load if had done
//  a simple startup.
//
//  NOTE:  This routine runs on a separate thread to allow the User to
//         start applets while this check is being done.
//
////////////////////////////////////////////////////////////////////////////

void AbsoluteValidation ()
{
    DWORD  i, j;
    DWORD  numFiles = 0;
    DWORD  numCache = 0;
    BOOL   bRet = TRUE;
    TCHAR  szMutex[MAX_PATH];
    HANDLE hmutex;
    UINT   uError;

    CPLFILES  *cfCPlFiles;
    CPLFILES  *cfCPlCache;

    //
    //  Disable WIN32 Error Popup Message box on attempt to load bad
    //  images.
    //
    //  Note:  Since the ErrorMode state is maintained on a per process
    //         basis, this call is placed at the start of this thread to
    //         avoid collisions, race conditions, or any sort of foul-up
    //         between the Error Mode state of a running CPL applet and
    //         whatever may happen to the Error Mode during this background
    //         check.
    //

    uError = SetErrorMode (SEM_FAILCRITICALERRORS);


    //////////////////////////////////////////////////////////////////////
    //  Check Filesystem .CPL modules against Cached Modules
    //////////////////////////////////////////////////////////////////////

    SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_IDLE);

    cfCPlFiles = cfCPlCache = NULL;

    //  Get some storage for modules
    cfCPlFiles = (CPLFILES *) LocalAlloc (LPTR, sizeof(CPLFILES)*CPL_MAXMODS);

    cfCPlCache = (CPLFILES *) LocalAlloc (LPTR, sizeof(CPLFILES)*CPL_MAXMODS);

    if (!cfCPlFiles || !cfCPlCache)
    {
        RIPMEM();
        bRet = FALSE;
        goto CommonExit;
    }

    //  Now get info on CPl files and Cached modules
    if ((numFiles = CheckFindModules (cfCPlFiles)) == 0)
    {
        RIP(TEXT("CheckFindModules failed"));
        bRet = FALSE;
        goto CommonExit;
    }

    if ((numCache = CheckGetCacheMods (cfCPlCache)) == 0)
    {
        RIP(TEXT("CheckGetCacheMods failed"));
        bRet = FALSE;
        goto CommonExit;
    }

    for (i = 0; i < numFiles; i++)
    {
        for (j = 0; j < numCache; j++)
        {
            if (cfCPlCache[j].bAlreadyMatched)
                continue;

            if (!_tcsicmp (cfCPlCache[j].szPathname, cfCPlFiles[i].szPathname))
            {
                //  We found a name match
                cfCPlCache[j].bAlreadyMatched = TRUE;
                cfCPlFiles[i].bAlreadyMatched = TRUE;

                //////////////////////////////////////////////////////////////
                //  Before loading this module, make sure that it is not
                //  in use by the Main thread.
                //////////////////////////////////////////////////////////////

                CreateMutexNameFromPath (cfCPlCache[j].szPathname, szMutex);
                hmutex = CreateMutex (NULL, FALSE, szMutex);

                WaitForSingleObject (hmutex, INFINITE);

                if (!ValidateAskModule (cfCPlFiles[i].szPathname, &cfCPlCache[j]))
                    bRet = FALSE;

                ReleaseMutex (hmutex);
                CloseHandle (hmutex);

                if (!bRet)
                    goto CommonExit;
            }
        }
    }

    //  Are there any unmatched files?

    for (i = 0; i < numFiles; i++)
    {
        if (!cfCPlFiles[i].bAlreadyMatched)
        {
            bRet = FALSE;
            goto CommonExit;
        }
    }


CommonExit:

    if (cfCPlFiles)
        LocalFree ((HLOCAL) cfCPlFiles);

    if (cfCPlCache)
        LocalFree ((HLOCAL) cfCPlCache);

    //
    //  Restore prior error mode
    //

    SetErrorMode (uError);


    //////////////////////////////////////////////////////////////////////
    //  For FALSE return - force system to re-load all modules and cache
    //////////////////////////////////////////////////////////////////////

    if (!bRet)
        PostMessage (hCPlWnd, WM_COMMAND, MENU_CACHE, 0);

    bValidationDone = TRUE;

    return;
}


////////////////////////////////////////////////////////////////////////////
//
//  Build the Control Panel Cache in the registry without a window.
//
//
////////////////////////////////////////////////////////////////////////////

void BuildCache ()
{
    DWORD  i;
    DWORD  numFiles = 0;
    int    numMods = 0;

    CPLFILES  *cfCPlFiles = NULL;


    //////////////////////////////////////////////////////////////////////////
    //  Check and Update cache as necessary
    //////////////////////////////////////////////////////////////////////////

    if (IsCacheValid())
        goto CommonExit;


    //////////////////////////////////////////////////////////////////////////
    //  Check Filesystem .CPL modules
    //////////////////////////////////////////////////////////////////////////

    //  Get some storage for modules
    cfCPlFiles = (CPLFILES *) LocalAlloc (LPTR, sizeof(CPLFILES)*CPL_MAXMODS);

    if (!cfCPlFiles)
    {
        RIPMEM();
        goto CommonExit;
    }

    //  Now get info on CPl files and Cached modules
    if ((numFiles = CheckFindModules (cfCPlFiles)) == 0)
    {
        RIP(TEXT("CheckFindModules failed"));
        goto CommonExit;
    }


    //////////////////////////////////////////////////////////////////////////
    //  Create global CPlMods[] array
    //////////////////////////////////////////////////////////////////////////

    numMods = 0;

    for (i = 0; i < numFiles; i++)
    {
        if (AskModule (cfCPlFiles[i].szPathname, numMods))
           numMods += 1;
    }

    //////////////////////////////////////////////////////////////////////////
    //  Now build the cache
    //////////////////////////////////////////////////////////////////////////

    UpdateCache();


    //////////////////////////////////////////////////////////////////////////
    //  Free all of the modules 
    //////////////////////////////////////////////////////////////////////////

    FreeApps ();

CommonExit:

    if (cfCPlFiles)
        LocalFree ((HLOCAL) cfCPlFiles);

    return;
}
