#include "stdafx.h"
#pragma hdrstop
//#include "clsobj.h"
//#include "resource.h"
//#include "deskstat.h"
//#include "deskhtml.h"
//#include "dback.h"
//#include "dutil.h"
//#include "dsubscri.h"
//#include "dcomp.h"
//#include "..\security.h"
//#include <mmhelper.h>
//#include "webcheck.h"
//#include "clsobj.h"
//#include <shlwapip.h>

#include <mluisupp.h>

#ifdef POSTSPLIT

#define THISCLASS CActiveDesktop
#define DXA_GROWTH_CONST 10
#define ZINDEX_START 1000

#if 0
#define TF_DESKSTAT     TF_CUSTOM2
#else
#define TF_DESKSTAT     0
#endif

IActiveDesktop *g_pActiveDesk = NULL;

#define c_szRegStrDesktop REGSTR_PATH_DESKTOP
#define c_szWallpaper  REG_VAL_GENERAL_WALLPAPER
#define c_szBackupWallpaper REG_VAL_GENERAL_BACKUPWALLPAPER
#define c_szPattern TEXT("Pattern")
#define c_szTileWall REG_VAL_GENERAL_TILEWALLPAPER
#define c_szWallpaperStyle REG_VAL_GENERAL_WALLPAPERSTYLE
#define c_szWallpaperTime REG_VAL_GENERAL_WALLPAPERTIME
#define c_szRefreshDesktop TEXT("RefreshDesktop")
#define c_szBufferedRefresh TEXT("BufferedRefresh")

#define COMP_TYPE               0x00000003
#define COMP_SELECTED           0x00002000
#define COMP_NOSCROLL           0x00004000



#ifdef DEBUG

#define ENTERPROC EnterProcDS
#define EXITPROC ExitProcDS

void EnterProcDS(DWORD dwTraceLevel, LPSTR szFmt, ...);
void ExitProcDS(DWORD dwTraceLevel, LPSTR szFmt, ...);

extern DWORD g_dwDeskStatTrace;

#else

#ifndef CCOVER
#pragma warning(disable:4002)
#define ENTERPROC()
#define EXITPROC()

#else // work around a bug in preprocessing by cl.exe
#define ENTERPROC 1 ? (void) 0 : (void)
#define EXITPROC 1 ? (void) 0 : (void)
#endif

#endif

STDAPI ParseDesktopComponent(HWND hwndOwner, LPWSTR wszURL, COMPONENT *pInfo);

int GetIntFromReg(HKEY    hKey,
                  LPCTSTR lpszSubkey,
                  LPCTSTR lpszNameValue,
                  int     iDefault)
{
    TCHAR szValue[20];
    DWORD dwSizeofValueBuff = SIZEOF(szValue);
    int iRetValue = iDefault;
    DWORD dwType;

    if ((SHGetValue(hKey, lpszSubkey, lpszNameValue, &dwType,(LPBYTE)szValue,
                   &dwSizeofValueBuff) == ERROR_SUCCESS) && dwSizeofValueBuff)
    {
        if (dwType == REG_SZ)
        {
            iRetValue = (int)StrToInt(szValue);
        }
    }

    return iRetValue;
}

BOOL GetStringFromReg(HKEY    hkey,
              LPCTSTR lpszSubkey,
              LPCTSTR lpszValueName,
              LPCTSTR lpszDefault,
              LPTSTR  lpszValue,
              DWORD   cchSizeofValueBuff)
{
    BOOL fRet = FALSE;
    DWORD dwType;

    cchSizeofValueBuff *= sizeof(TCHAR);
    if (SHGetValue(hkey, lpszSubkey, lpszValueName, &dwType, lpszValue, &cchSizeofValueBuff) == ERROR_SUCCESS)
    {
        fRet = TRUE;
    }

    //
    // On failure use the default string.
    //
    if (!fRet && lpszDefault)
    {
        lstrcpy(lpszValue, lpszDefault);
    }

    return fRet;
}

void GetWallpaperFileTime(LPCTSTR pszWallpaper, LPFILETIME lpftFileTime)
{
    HANDLE   hFile;
    BOOL     fSuccess = FALSE;

    if((hFile = CreateFile(pszWallpaper, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, 0)) != INVALID_HANDLE_VALUE)
    {
        if(GetFileTime(hFile, NULL, NULL, lpftFileTime))
            fSuccess = TRUE;

        CloseHandle(hFile);
    }

    if(!fSuccess)
        ZeroMemory(lpftFileTime, SIZEOF(*lpftFileTime));
}

BOOL  HasWallpaperReallyChanged(LPCTSTR pszRegKey, LPTSTR pszOldWallpaper, LPTSTR pszBackupWallpaper, LPFILETIME lpftFileTime,
                                DWORD dwOldWallpaperStyle, DWORD dwNewWallpaperStyle)
{
    BOOL    fRet = FALSE; //Assume that the wallpaper hasn't changed.
    FILETIME ftBackupFileTime;
    DWORD   cchBuffSize;
    DWORD   dwType;

    GetWallpaperFileTime(pszOldWallpaper, lpftFileTime);

    if(lstrcmpi(pszOldWallpaper, pszBackupWallpaper))
        return (TRUE);   //The name of the wallpaper changed.

    // The wallpaper filename hasn't changed. But, the content of this file
    // could have changed. See if the content has changed by looking at the 
    // last-written date and time stamp on this file.
     
    cchBuffSize = SIZEOF(ftBackupFileTime);
    //Get the last written time of the backup wallpaper from registry
    if (SHGetValue(HKEY_CURRENT_USER, pszRegKey, c_szWallpaperTime, &dwType, &ftBackupFileTime, &cchBuffSize) != ERROR_SUCCESS)
        ZeroMemory(&ftBackupFileTime, SIZEOF(ftBackupFileTime));

    if((lpftFileTime->dwLowDateTime != ftBackupFileTime.dwLowDateTime) ||
       (lpftFileTime->dwHighDateTime != ftBackupFileTime.dwHighDateTime))
        fRet = TRUE; //The wallpaper content has changed

    //See if the wallpaper style has changed.
    if(dwOldWallpaperStyle != dwNewWallpaperStyle)
        fRet = TRUE;
    
    return (fRet);
}

//-------------------------------------------------------------------------------------------------------------//
//  Function: ReadWallpaperStyleFromReg()
//
// This function reads the "TileWallpaper" and the "WallpaperStyle" from the given location
// in the registry.
//
//-------------------------------------------------------------------------------------------------------------//

void ReadWallpaperStyleFromReg(LPCTSTR pszRegKey, DWORD *pdwWallpaperStyle, BOOL fIgnorePlatforms)
{
    //Do not read "Stretch" bits for older platforms.
    if(fIgnorePlatforms || g_bRunOnMemphis || g_bRunOnNT5)
        *pdwWallpaperStyle = GetIntFromReg(HKEY_CURRENT_USER, pszRegKey, c_szWallpaperStyle, WPSTYLE_CENTER);
    else
        *pdwWallpaperStyle = WPSTYLE_CENTER;

    if (GetIntFromReg(HKEY_CURRENT_USER, pszRegKey, c_szTileWall, WPSTYLE_TILE))
    {
        //
        // "Tile" overrides the "Stretch" style.
        //
        *pdwWallpaperStyle = WPSTYLE_TILE;
    }
    // else, STRETCH or CENTER.
}

void THISCLASS::_ReadWallpaper(BOOL fActiveDesktop)
{
    ENTERPROC(2, "DS ReadWallpaper()");

    TCHAR lpszDeskcomp[MAX_PATH];

    GetRegLocation(lpszDeskcomp, REG_DESKCOMP_GENERAL, _pszScheme);

    //
    // Read in the wallpaper and style from the appropriate registry location.
    //
    LPCTSTR pszRegKey;
    if (fActiveDesktop)
    {
        pszRegKey = (LPCTSTR)lpszDeskcomp;
        TCHAR   szOldWallpaper[MAX_PATH];
        DWORD   dwOldWallpaperStyle;

        // Read the Wallpaper from the Old location.
        if(!GetStringFromReg(HKEY_CURRENT_USER, c_szRegStrDesktop, c_szWallpaper, c_szNULL, szOldWallpaper, ARRAYSIZE(szOldWallpaper)))
            szOldWallpaper[0] = TEXT('\0');

        // Read wallpaper style from the old location.
        ReadWallpaperStyleFromReg((LPCTSTR)c_szRegStrDesktop, &dwOldWallpaperStyle, FALSE);

        // Read the wallpaper from the new location too!
        if (!GetStringFromReg(HKEY_CURRENT_USER, pszRegKey, c_szWallpaper, c_szNULL, _szSelectedWallpaper, ARRAYSIZE(_szSelectedWallpaper)))
        {
            pszRegKey = c_szRegStrDesktop;
            lstrcpy(_szSelectedWallpaper, szOldWallpaper);
        }

        //Read wallpaper style from the new location too!
        ReadWallpaperStyleFromReg(pszRegKey, &_wpo.dwStyle, FALSE);
        
        //If there is a Safe mode scheme here do NOT attempt to change wallpaper
        WCHAR wszDisplay[MAX_PATH];
        DWORD dwcch = MAX_PATH;

        if(FAILED(GetScheme(wszDisplay, &dwcch, SCHEME_GLOBAL | SCHEME_DISPLAY)) ||
                 (StrCmpW(wszDisplay, REG_DESKCOMP_SAFEMODE_SUFFIX_L) != 0))
        {
            DWORD     dwType;
            DWORD     cchBuffSize;

            //We are not in Safe mode.
            cchBuffSize = SIZEOF(_ftWallpaperFileTime);
            //Read what is currently stored as wallpaper last-written time.
            if (SHGetValue(HKEY_CURRENT_USER, pszRegKey, c_szWallpaperTime, &dwType, &_ftWallpaperFileTime, &cchBuffSize) != ERROR_SUCCESS)
                ZeroMemory(&_ftWallpaperFileTime, SIZEOF(_ftWallpaperFileTime));

            FILETIME ftOldFileTime;

            //Read what is stored as "Backup" wallpaper.
            if (!GetStringFromReg(HKEY_CURRENT_USER, pszRegKey, c_szBackupWallpaper, c_szNULL, _szBackupWallpaper, ARRAYSIZE(_szBackupWallpaper)))
                lstrcpy(_szBackupWallpaper, szOldWallpaper);
    
            //See if the Old wallpaper is differnet from the backed up wallpaper
            if(HasWallpaperReallyChanged(pszRegKey, szOldWallpaper, _szBackupWallpaper, &ftOldFileTime, dwOldWallpaperStyle, _wpo.dwStyle))
            {
                //They are different. This means that some other app has changed the "Old" wallpaper
                //after the last time we backed it up in the registry.
                // Make this wallpaper as the Selected wallpaper!

                // This is a kinda hack, but the best possible solution right now. The scenario is as follows.
                // The Memphis setup guys replace what the user specifies as the wallpaper in the old location
                // and restore it after setup is complete. But, SetDefaultWallpaper() gets called bet. these
                // two times and we are supposed to take a decision on whether to set the default htm wallpaper or not,
                // depending on what the user had set before the installation. The solution is to delay making
                // this decision until after the setup guys have restored the user's wallpaper. We do this in
                // CActiveDesktop::_ReadWallpaper(). We specify that SetDefaultWallpaper() was called by setting
                // the backup wallpaper in the new location to the default wallpaper.
                TCHAR szDefaultWallpaper[INTERNET_MAX_URL_LENGTH];
                GetWallpaperDirName(szDefaultWallpaper, ARRAYSIZE(szDefaultWallpaper));
                lstrcat(szDefaultWallpaper, TEXT("\\"));
                TCHAR szWP[INTERNET_MAX_URL_LENGTH];
                GetDefaultWallpaper(szWP);
                lstrcat(szDefaultWallpaper, szWP);
        
                if(lstrcmp(_szBackupWallpaper, szDefaultWallpaper) == 0
                    && (!szOldWallpaper[0] || lstrcmp(szOldWallpaper, g_szNone) == 0))
                {
                    lstrcpy(_szSelectedWallpaper, szDefaultWallpaper);
                }
                else
                {
                    lstrcpy(_szSelectedWallpaper, szOldWallpaper);
                }
                _ftWallpaperFileTime = ftOldFileTime;
                _wpo.dwStyle = dwOldWallpaperStyle;

                _fWallpaperDirty = TRUE;
                _fWallpaperChangedDuringInit = TRUE;
            }

            //Make a backup of the "Old" wallpaper
            lstrcpy(_szBackupWallpaper, szOldWallpaper);
        }
        else
        {
            lstrcpy(_szBackupWallpaper, szOldWallpaper);
            GetWallpaperFileTime(_szBackupWallpaper, &_ftWallpaperFileTime);
        }
    }
    else
    {
        pszRegKey = c_szRegStrDesktop; //Get it from the old location!

        //Since active desktop is not available, read wallpaper from old location.
        GetStringFromReg(HKEY_CURRENT_USER, pszRegKey, c_szWallpaper, c_szNULL, _szSelectedWallpaper, ARRAYSIZE(_szSelectedWallpaper));

        //Make a backup of the "Old" wallpaper
        lstrcpy(_szBackupWallpaper, _szSelectedWallpaper);
        GetWallpaperFileTime(_szBackupWallpaper, &_ftWallpaperFileTime);

        //Read the wallpaper style
        ReadWallpaperStyleFromReg(pszRegKey, &_wpo.dwStyle, TRUE);
    }

    EXITPROC(2, "DS ReadWallpaper! (_szSelectedWP=>%s<)", _szSelectedWallpaper);
}

void THISCLASS::_ReadPattern(void)
{
    ENTERPROC(2, "DS ReadPattern()");

    GetStringFromReg(HKEY_CURRENT_USER, c_szRegStrDesktop, c_szPattern, c_szNULL, _szSelectedPattern, ARRAYSIZE(_szSelectedPattern));

    EXITPROC(2, "DS ReadPattern! (_szSelectedPattern=>%s<)", _szSelectedPattern);
}

void THISCLASS::_ReadComponent(HKEY hkey, LPCTSTR pszComp)
{
    ENTERPROC(2, "DS ReadComponent(hk=%08X,pszComp=>%s<)", hkey, pszComp);

    HKEY hkeyComp;

    if (RegOpenKeyEx(hkey, pszComp, 0, KEY_READ, &hkeyComp) == ERROR_SUCCESS)
    {
        DWORD cbSize, dwType;
        COMPONENTA comp;
        comp.dwSize = sizeof(COMPONENTA);

        //
        // Read in the source string.
        //
        cbSize = SIZEOF(comp.szSource);
        if (RegQueryValueEx(hkeyComp, REG_VAL_COMP_SOURCE, NULL, &dwType, (LPBYTE)&comp.szSource, &cbSize) != ERROR_SUCCESS)
        {
            comp.szSource[0] = TEXT('\0');
        }

        //
        // Read in the SubscribedURL string.
        //
        cbSize = SIZEOF(comp.szSubscribedURL);
        if (RegQueryValueEx(hkeyComp, REG_VAL_COMP_SUBSCRIBED_URL, NULL, &dwType, (LPBYTE)&comp.szSubscribedURL, &cbSize) != ERROR_SUCCESS)
        {
            comp.szSubscribedURL[0] = TEXT('\0');
        }

        //
        // Read in the Friendly name string.
        //
        cbSize = SIZEOF(comp.szFriendlyName);
        if (RegQueryValueEx(hkeyComp, REG_VAL_COMP_NAME, NULL, &dwType, (LPBYTE)&comp.szFriendlyName, &cbSize) != ERROR_SUCCESS)
        {
            comp.szFriendlyName[0] = TEXT('\0');
        }

        //
        // Read in and parse the flags.
        //
        DWORD dwFlags;
        cbSize = SIZEOF(dwFlags);
        if (RegQueryValueEx(hkeyComp, REG_VAL_COMP_FLAGS, NULL, &dwType, (LPBYTE)&dwFlags, &cbSize) != ERROR_SUCCESS)
        {
            dwFlags = 0;
        }
        comp.iComponentType = dwFlags & COMP_TYPE;
        comp.fChecked = (dwFlags & COMP_SELECTED) != 0;
        comp.fNoScroll = (dwFlags & COMP_NOSCROLL) != 0;
        comp.fDirty = FALSE;    //Reading it fresh from registry; Can't be dirty!

        //
        // Read in the location.
        //
        cbSize = SIZEOF(comp.cpPos);
        if (RegQueryValueEx(hkeyComp, REG_VAL_COMP_POSITION, NULL, &dwType, (LPBYTE)&comp.cpPos, &cbSize) != ERROR_SUCCESS)
        {
            ZeroMemory(&comp.cpPos, SIZEOF(comp.cpPos));
        }

        //
        // In IE4.x, we have a very huge positive number (0x7fffffff) as the COMPONENT_TOP;
        // As a result some component's z-index overflowed into the negative range (0x80000003)
        // To fix this, we halved the COMPONENT_TOP (0x3fffffff) and also check for negative z-index
        // values and covert them to postive values.
        if(comp.cpPos.izIndex < 0)
            comp.cpPos.izIndex = COMPONENT_TOP;
        //
        // Make sure the cpPos.dwSize is set to correct value
        //
        comp.cpPos.dwSize = sizeof(COMPPOS);

        //
        //  Read in the current ItemState
        //
        cbSize = SIZEOF(comp.dwCurItemState);
        if (RegQueryValueEx(hkeyComp, REG_VAL_COMP_CURSTATE, NULL, &dwType, (LPBYTE)&comp.dwCurItemState, &cbSize) != ERROR_SUCCESS)
        {
            //If the item state is missing, we must be reading from IE4 machine.
            comp.dwCurItemState = IS_NORMAL;
        }

        //
        //  Read in the Original state info.
        //
        cbSize = SIZEOF(comp.csiOriginal);
        if (RegQueryValueEx(hkeyComp, REG_VAL_COMP_ORIGINALSTATEINFO, NULL, &dwType, (LPBYTE)&comp.csiOriginal, &cbSize) != ERROR_SUCCESS)
        {
            //If the item state is missing, we must be reading from IE4 machine.
            // Set the OriginalState to the default info.
            SetStateInfo(&comp.csiOriginal, &comp.cpPos, IS_NORMAL);
        }

        //
        //  Read in the Restored state info.
        //
        cbSize = SIZEOF(comp.csiRestored);
        if (RegQueryValueEx(hkeyComp, REG_VAL_COMP_RESTOREDSTATEINFO, NULL, &dwType, (LPBYTE)&comp.csiRestored, &cbSize) != ERROR_SUCCESS)
        {
            //If the item state is missing, we must be reading from IE4 machine.
            // Set the restored State to the default info.
            SetStateInfo(&comp.csiRestored, &comp.cpPos, IS_NORMAL);
        }

        //
        // Add the component to the component list.
        //
        AddComponentPrivate(&comp, StrToInt(pszComp));

        //
        // Increment our counter so we know where to add any new
        // components after we're done.
        //
        _dwNextID++;

        RegCloseKey(hkeyComp);
    }

    EXITPROC(2, "DS ReadComponent!");
}

typedef struct _tagSortStruct {
    int ihdsaIndex;
    int izIndex;
} SORTSTRUCT;

int CALLBACK pfnComponentSort(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    SORTSTRUCT * pss1 = (SORTSTRUCT *)p1;
    SORTSTRUCT * pss2 = (SORTSTRUCT *)p2;

    if (pss1->izIndex > pss2->izIndex)
        return 1;

    if (pss1->izIndex < pss2->izIndex)
        return -1;

    return(0);
}

//
// ModifyZIndex
//
// Little helper function to put the zindex of the windowed and windowless components
// into correct buckets so that zorting will produce a correct order by zindex.
//
// If we don't do this then windowless components may end up zordering above windowed ones.
//
void ModifyZIndex(COMPONENTA * pcomp)
{
    if (pcomp->cpPos.izIndex != COMPONENT_TOP) {
        if (!IsWindowLessComponent(pcomp))
            pcomp->cpPos.izIndex += COMPONENT_TOP_WINDOWLESS;
    }
    else
    {
        if (IsWindowLessComponent(pcomp))
            pcomp->cpPos.izIndex = COMPONENT_TOP_WINDOWLESS;
    }
}

//
// SortAndRationalize
//
// SortAndRationalize will take an unsorted component list and sort it such that the components
// come out in the correct z-index indicated order.  It will also rebase the z-index values at
// a known constant so that the z-index values will not grow endlessly.  SortAndRationalize also
// imposes windowed vs. windowless criteria to the zindex values such that windowless components
// always zorder under windowed ones.
//
void THISCLASS::_SortAndRationalize(void)
{
    int icComponents;
    HDPA hdpa;

    if (_hdsaComponent && ((icComponents = DSA_GetItemCount(_hdsaComponent)) > 1) && (hdpa = DPA_Create(0))) {
        COMPONENTA * pcomp;
        SORTSTRUCT * pss;
        int i, iCur = ZINDEX_START;
        BOOL fInsertFailed = FALSE;
        HDSA hdsaOld;

        // Go through each component and insert it's hdsa-index and zindex into the hdpa
        for (i = 0; i < icComponents; i++)
        {
            if (!(pss = (SORTSTRUCT *)LocalAlloc(LPTR, sizeof(SORTSTRUCT))))
                break;

            pcomp = (COMPONENTA *)DSA_GetItemPtr(_hdsaComponent, i);
            ModifyZIndex(pcomp);
            pss->ihdsaIndex = i;
            pss->izIndex = pcomp->cpPos.izIndex;
            if (DPA_AppendPtr(hdpa, (void FAR *)pss) == -1) {
                LocalFree((HANDLE)pss);
                break;
            }
        }

        // Sort the hdpa by zindex
        DPA_Sort(hdpa, pfnComponentSort, 0);

        // Save old values
        hdsaOld = _hdsaComponent;

        // Null out the old hdsa, so AddComponentPrivate will create a new one
        _hdsaComponent = NULL;

        // Now go through the sorted hdpa and update the component zindex with a ZINDEX_START based zindex, then
        // add the component to the new hdsa in sorted order.
        for (i = 0; i < icComponents; i++) {
            if (!(pss = (SORTSTRUCT *)DPA_GetPtr(hdpa, i)))
                break;
            // Get component and update it's zIndex and id
            pcomp = (COMPONENTA *)DSA_GetItemPtr(hdsaOld, pss->ihdsaIndex);
            pcomp->cpPos.izIndex = iCur;
            iCur += 2;

            // Free ptr
            LocalFree((HANDLE)pss);

            // Add to new hdsa in sorted order
            if (!fInsertFailed) {
                fInsertFailed = !AddComponentPrivate(pcomp, pcomp->dwID);
            }
        }

        // If we're completely successfull then destroy the old hdsa.  Otherwise we need
        // to destroy the new one and restore the old one.
        if ((i == icComponents) && !fInsertFailed) {
            DSA_Destroy(hdsaOld);
        } else {
            if (_hdsaComponent)
            DSA_Destroy(_hdsaComponent);
            _hdsaComponent = hdsaOld;
        }

        DPA_Destroy(hdpa);
    }
}

void THISCLASS::_ReadComponents(BOOL fActiveDesktop)
{
    ENTERPROC(2, "DS ReadComponents()");

    HKEY hkey;
    TCHAR lpszDeskcomp[MAX_PATH];

    GetRegLocation(lpszDeskcomp, REG_DESKCOMP_COMPONENTS, _pszScheme);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, lpszDeskcomp, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        DWORD cbSize, dwType;
        int i = 0;
        TCHAR lpszSubkey[MAX_PATH];

        //
        // Read in the general settings.
        //
        DWORD dwSettings;
        cbSize = SIZEOF(dwSettings);
        if (RegQueryValueEx(hkey, REG_VAL_COMP_SETTINGS, NULL, &dwType, (LPBYTE)&dwSettings, &cbSize) == ERROR_SUCCESS)
        {
            _co.fEnableComponents = (dwSettings & COMPSETTING_ENABLE) != 0;
        }
        _co.fActiveDesktop = fActiveDesktop;

        //
        // Read in all the desktop components
        //
        while (RegEnumKey(hkey, i, lpszSubkey, ARRAYSIZE(lpszSubkey)) == ERROR_SUCCESS)
        {
            _ReadComponent(hkey, lpszSubkey);
            i++;
        }

        _SortAndRationalize();

        RegCloseKey(hkey);
    }

    EXITPROC(2, "DS ReadComponents!");
}

void THISCLASS::_Initialize(void)
{
    ENTERPROC(2, "DS Initialize()");

    if (!_fInitialized)
    {
        _fInitialized = TRUE;
        InitDeskHtmlGlobals();

        SHELLSTATE ss = {0};
        SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE);
        
        BOOL fActiveDesktop = BOOLIFY(ss.fDesktopHTML);
        
        _co.dwSize = SIZEOF(_co);
        _wpo.dwSize = SIZEOF(_wpo);

        //
        // This per-user registry branch may not exist for this user. Or, even if
        // it does exist, it may have some stale info. So ensure that atlreast the 
        // default components are there and that the html version is current for this
        // branch of the registry!
        //  If everything is current, the following function does nothing!
        //
        CDeskHtmlProp_RegUnReg(TRUE);  //TRUE => install.

        _ReadWallpaper(fActiveDesktop);
        _ReadPattern();
        _ReadComponents(fActiveDesktop);

        _fDirty = FALSE;
        _fNeedBodyEnd = FALSE;
    }

    EXITPROC(2, "DS Initialize!");
}

void THISCLASS::_SaveWallpaper(void)
{
    ENTERPROC(2, "DS SaveWallpaper");
    TCHAR lpszDeskcomp[MAX_PATH];
    BOOL    fNormalWallpaper;

    GetRegLocation(lpszDeskcomp, REG_DESKCOMP_GENERAL, _pszScheme);

    //
    // Compute tiling string.
    //
    TCHAR szTiled[2];
    lstrcpy(szTiled, c_szZero);
    szTiled[0] = szTiled[0] + (TCHAR)(_wpo.dwStyle & WPSTYLE_TILE);

    //
    // Compute the Wallpaper styling string
    //
    TCHAR       szWPStyle[2];
    lstrcpy(szWPStyle, c_szZero);
    //
    // NOTE: If WPSTYLE_TILE is set, we still want to say WallpaperStyle="0"; This won't hurt
    // because TileWallpaper="1" will over-ride this anyway.
    // The reason for this hack is that during memphis setup, they put a tiled wallpaper. Then we
    // write WallpaperStyle=1 and TileWallpaper=1 in new and old locations. Then, then change
    // the wallpaper and set TileWallpaper=0. Since the WallpaperStyle continues to be 1, they 
    // get a tiled wallpaper finally. The following is to avoid this problem!
    // 
    szWPStyle[0] = szWPStyle[0] + (TCHAR)(_wpo.dwStyle & WPSTYLE_STRETCH);
    

    //
    // Write out wallpaper settings in new active desktop area.
    //
    if(_fWallpaperDirty || _fWallpaperChangedDuringInit)
    {
        SHSetValue(HKEY_CURRENT_USER, lpszDeskcomp,
            c_szTileWall, REG_SZ, (LPBYTE)szTiled, SIZEOF(TCHAR)*(lstrlen(szTiled)+1));

        //
        // Note: We do not write the Wallpaper Style string for older systems because we do not
        // want to over-write what PlusPack writes. However, for newer Operating systems, we 
        // want to write the WallpaperStyle also.
        //
        if(g_bRunOnMemphis || g_bRunOnNT5)
        {
            SHSetValue(HKEY_CURRENT_USER, lpszDeskcomp,
                c_szWallpaperStyle, REG_SZ, (LPBYTE)szWPStyle, SIZEOF(TCHAR)*(lstrlen(szWPStyle)+1));
        }
    
        SHSetValue(HKEY_CURRENT_USER, lpszDeskcomp,
                    c_szWallpaper, REG_SZ, (LPBYTE)_szSelectedWallpaper,
                    SIZEOF(TCHAR)*(lstrlen(_szSelectedWallpaper)+1));
    }

    if(fNormalWallpaper = IsNormalWallpaper(_szSelectedWallpaper))
    {
        lstrcpyn(_szBackupWallpaper, _szSelectedWallpaper, ARRAYSIZE(_szBackupWallpaper));
        GetWallpaperFileTime(_szSelectedWallpaper, &_ftWallpaperFileTime);
    }

    // Backup the "Old type" wallpaper's name here in the new location
    // sothat we can detect when this gets changed by some other app.
    SHSetValue(HKEY_CURRENT_USER, lpszDeskcomp,
                c_szBackupWallpaper, REG_SZ, (LPBYTE)_szBackupWallpaper,
                SIZEOF(TCHAR)*(lstrlen(_szBackupWallpaper)+1));

    SHSetValue(HKEY_CURRENT_USER, lpszDeskcomp,
                c_szWallpaperTime, REG_BINARY, (LPBYTE)&_ftWallpaperFileTime,
                SIZEOF(_ftWallpaperFileTime));    
    //
    // Even if this wallpaper is not valid in normal desktop (i.e., even if it is not a .BMP),
    // write it out in normal desktop registry area.
    //
    if (_fWallpaperDirty)
    {
        SHSetValue(HKEY_CURRENT_USER, c_szRegStrDesktop,
                    c_szTileWall, REG_SZ, (LPBYTE)szTiled, SIZEOF(TCHAR)*(lstrlen(szTiled)+1));
        //
        // Note: We do not write the Wallpaper Style string for older systems because we do not
        // want to over-write what PlusPack writes. However, for newer Operating systems, we 
        // want to write the WallpaperStyle also.
        //
        if(g_bRunOnMemphis || g_bRunOnNT5)
        {
            SHSetValue(HKEY_CURRENT_USER, c_szRegStrDesktop,
                        c_szWallpaperStyle, REG_SZ, (LPBYTE)szWPStyle, SIZEOF(TCHAR)*(lstrlen(szWPStyle)+1));
        }

        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, 
                    (fNormalWallpaper ? _szSelectedWallpaper : _szBackupWallpaper),
                     SPIF_UPDATEINIFILE);

    }

    EXITPROC(2, "DS SaveWallpaper");
}

void THISCLASS::_SaveComponent(HKEY hkey, int iIndex, COMPONENTA *pcomp)
{
    ENTERPROC(2, "DS SaveComponent(hkey=%08X,iIndex=%d,pcomp=%08X)", hkey, iIndex, pcomp);

    TCHAR szSubKey[8];
    HKEY hkeySub;

    wsprintf(szSubKey, TEXT("%d"), iIndex);
    if (RegCreateKey(hkey, szSubKey, &hkeySub) == ERROR_SUCCESS)
    {
        pcomp->fDirty = FALSE; //Since we are saving in the registry, reset this!
        //
        // Write out the source string and Friendly name string.
        //
        RegSetValueEx(hkeySub, REG_VAL_COMP_SOURCE, 0, REG_SZ, (LPBYTE)pcomp->szSource, (lstrlen(pcomp->szSource)+1)*SIZEOF(TCHAR));
        RegSetValueEx(hkeySub, REG_VAL_COMP_SUBSCRIBED_URL, 0, REG_SZ, (LPBYTE)pcomp->szSubscribedURL, (lstrlen(pcomp->szSubscribedURL)+1)*SIZEOF(TCHAR));
        RegSetValueEx(hkeySub, REG_VAL_COMP_NAME, 0, REG_SZ, (LPBYTE)pcomp->szFriendlyName, (lstrlen(pcomp->szFriendlyName)+1)*SIZEOF(TCHAR));

        //
        // Compute and write out flags.
        //
        DWORD dwFlags = 0;
        dwFlags |= pcomp->iComponentType;
        if (pcomp->fChecked)
        {
            dwFlags |= COMP_SELECTED;
        }
        if (pcomp->fNoScroll)
        {
            dwFlags |= COMP_NOSCROLL;
        }
        RegSetValueEx(hkeySub, REG_VAL_COMP_FLAGS, 0, REG_DWORD, (LPBYTE)&dwFlags, SIZEOF(dwFlags));

        //
        // Write out the position.
        //
        RegSetValueEx(hkeySub, REG_VAL_COMP_POSITION, 0, REG_BINARY, (LPBYTE)&pcomp->cpPos, SIZEOF(pcomp->cpPos));

        //  Write out the Current state
        RegSetValueEx(hkeySub, REG_VAL_COMP_CURSTATE, 0, REG_DWORD, (LPBYTE)&pcomp->dwCurItemState, SIZEOF(pcomp->dwCurItemState));

        //  Write out the Original State Info
        RegSetValueEx(hkeySub, REG_VAL_COMP_ORIGINALSTATEINFO, 0, REG_BINARY, (LPBYTE)&pcomp->csiOriginal, SIZEOF(pcomp->csiOriginal));
        
        //  Write out the Restored State Info
        RegSetValueEx(hkeySub, REG_VAL_COMP_RESTOREDSTATEINFO, 0, REG_BINARY, (LPBYTE)&pcomp->csiRestored, SIZEOF(pcomp->csiRestored));

        RegCloseKey(hkeySub);
    }

    EXITPROC(2, "DS SaveComponent!");
}

void THISCLASS::_SaveComponents(void)
{
    ENTERPROC(2, "DS SaveComponents");

    int i;
    TCHAR lpszDeskcomp[MAX_PATH];

    GetRegLocation(lpszDeskcomp, REG_DESKCOMP_COMPONENTS, _pszScheme);

    //
    // Delete the entire registry key.
    //
    SHDeleteKey(HKEY_CURRENT_USER, lpszDeskcomp);

    //
    // Recreate the registry key.
    //
    HKEY hkey;
    if (RegCreateKey(HKEY_CURRENT_USER, lpszDeskcomp, &hkey) == ERROR_SUCCESS)
    {
        //
        // Write out the version number.
        //
        DWORD dw = CUR_DESKHTML_VERSION;
        RegSetValueEx(hkey, REG_VAL_COMP_VERSION, 0, REG_DWORD, (LPBYTE)(&dw), SIZEOF(dw));

        dw = CUR_DESKHTML_MINOR_VERSION;
        RegSetValueEx(hkey, REG_VAL_COMP_MINOR_VERSION, 0, REG_DWORD, (LPBYTE)(&dw), SIZEOF(dw));
    
        //
        // Write out the general settings.
        //
        DWORD dwSettings = 0;
        if (_co.fEnableComponents)
        {
            dwSettings |= COMPSETTING_ENABLE;
        }
        RegSetValueEx(hkey, REG_VAL_COMP_SETTINGS, 0, REG_DWORD, (LPBYTE)&dwSettings, SIZEOF(dwSettings));

        if (_hdsaComponent)
        {
            //
            // Write out the settings for each component
            //
            for (i=0; i<DSA_GetItemCount(_hdsaComponent); i++)
            {
                COMPONENTA * pcomp;

                if (pcomp = (COMPONENTA *)DSA_GetItemPtr(_hdsaComponent, i))
                {
                    pcomp->dwID = i;
                    _SaveComponent(hkey, i, pcomp);
                }
            }
        }

        RegCloseKey(hkey);
    }

    EXITPROC(2, "DS SaveComponents");
}

void THISCLASS::_SavePattern(DWORD dwFlags)
{
    ENTERPROC(2, "DS SavePattern()");

    if (_fPatternDirty && (dwFlags & SAVE_PATTERN_NAME))
    {
        //
        // Write out the pattern to the registry and INI files.
        //
        SystemParametersInfo(SPI_SETDESKPATTERN, 0, _szSelectedPattern, SPIF_UPDATEINIFILE);
    }

    if (IsValidPattern(_szSelectedPattern) && (dwFlags & GENERATE_PATTERN_FILE))
    {
        //
        // Write out the pattern as a BMP file for use in HTML.
        //
        TCHAR szBitmapFile[MAX_PATH];

        GetPerUserFileName(szBitmapFile, ARRAYSIZE(szBitmapFile), PATTERN_FILENAME);

        HANDLE hFileBitmap;
        hFileBitmap = CreateFile(szBitmapFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);

        if (hFileBitmap != INVALID_HANDLE_VALUE)
        {
            DWORD cbWritten;

            BITMAPFILEHEADER bmfh = {0};
            bmfh.bfType = 0x4D42;   // 'BM'
            bmfh.bfSize = SIZEOF(BITMAPFILEHEADER) + SIZEOF(BITMAPINFOHEADER) + 2*SIZEOF(RGBQUAD) + 8*SIZEOF(DWORD);
            bmfh.bfOffBits = SIZEOF(BITMAPFILEHEADER) + SIZEOF(BITMAPINFOHEADER) + 2*SIZEOF(RGBQUAD);
            WriteFile(hFileBitmap, &bmfh, SIZEOF(bmfh), &cbWritten, NULL);

            BITMAPINFOHEADER bmih = {0};
            bmih.biSize = SIZEOF(BITMAPINFOHEADER);
            bmih.biWidth = 8;
            bmih.biHeight = 8;
            bmih.biPlanes = 1;
            bmih.biBitCount = 1;
            bmih.biCompression = BI_RGB;
            WriteFile(hFileBitmap, &bmih, SIZEOF(bmih), &cbWritten, NULL);

            RGBQUAD argbTable[2] = {0};
            DWORD rgb;
            rgb = GetSysColor(COLOR_BACKGROUND);
            argbTable[0].rgbBlue = GetBValue(rgb);
            argbTable[0].rgbGreen = GetGValue(rgb);
            argbTable[0].rgbRed = GetRValue(rgb);
            rgb = GetSysColor(COLOR_WINDOWTEXT);
            argbTable[1].rgbBlue = GetBValue(rgb);
            argbTable[1].rgbGreen = GetGValue(rgb);
            argbTable[1].rgbRed = GetRValue(rgb);
            WriteFile(hFileBitmap, argbTable, SIZEOF(argbTable), &cbWritten, NULL);

            DWORD adwBits[8];
            PatternToDwords(_szSelectedPattern, adwBits);
            WriteFile(hFileBitmap, adwBits, SIZEOF(adwBits), &cbWritten, NULL);

            CloseHandle(hFileBitmap);
        }
    }

    EXITPROC(2, "DS SavePattern!");
}

void THISCLASS::_WriteHtmlFromString(LPCTSTR psz)
{
    ENTERPROC(3, "DS WriteHtmlFromString(psz=>%s<)", psz);

    DWORD cbWritten;
#ifdef UNICODE
    //if the string is unicode then convert it to ascii before writing it to
    // the file.
    char szBuf[INTERNET_MAX_URL_LENGTH];
    int cch = SHUnicodeToAnsi(psz, szBuf, ARRAYSIZE(szBuf));
    ASSERT(cch == lstrlenW(psz) + 1);       
    WriteFile(_hFileHtml, szBuf, lstrlenA(szBuf), &cbWritten, NULL);

#else
    WriteFile(_hFileHtml, psz, lstrlen(psz), &cbWritten, NULL);
#endif

    EXITPROC(3, "DS WriteHtmlFromString!");
}

void THISCLASS::_WriteHtmlFromId(UINT uid)
{
    ENTERPROC(3, "DS WriteHtmlFromId(uid=%d)", uid);

    TCHAR szBuf[INTERNET_MAX_URL_LENGTH];
    MLLoadString(uid, szBuf, ARRAYSIZE(szBuf));
    _WriteHtmlFromString(szBuf);

    EXITPROC(3, "DS WriteHtmlFromId!");
}

void THISCLASS::_WriteHtmlFromIdF(UINT uid, ...)
{
    ENTERPROC(3, "DS WriteHtmlFromIdF(uid=%d,...)", uid);

    TCHAR szBufFmt[INTERNET_MAX_URL_LENGTH];
    TCHAR szBuf[INTERNET_MAX_URL_LENGTH];

    MLLoadString(uid, szBufFmt, ARRAYSIZE(szBufFmt));

    va_list arglist;
    va_start(arglist, uid);
    wvsprintf(szBuf, szBufFmt, arglist);
    va_end(arglist);

    _WriteHtmlFromString(szBuf);

    EXITPROC(3, "DS WriteHtmlFromIdF!");
}

void THISCLASS::_WriteHtmlFromFile(LPCTSTR pszContents)
{
    CHAR szContents[MAX_PATH];
    ENTERPROC(3, "DS WriteHtmlFromFile(pszContents=>%s<)", pszContents);

    SHTCharToAnsi(pszContents, szContents, ARRAYSIZE(szContents));
    HFILE hfileContents = _lopen(szContents, OF_READ);
    if (hfileContents != HFILE_ERROR)
    {
        UINT cb;
        BYTE rgbBuf[INTERNET_MAX_URL_LENGTH];
        while ((cb = _lread(hfileContents, rgbBuf, SIZEOF(rgbBuf))) != HFILE_ERROR)
        {
            DWORD cbWritten;

            WriteFile(_hFileHtml, rgbBuf, cb, &cbWritten, NULL);
            if (cb < SIZEOF(rgbBuf))
            {
                break;
            }
        }
    }
    _lclose(hfileContents);

    EXITPROC(3, "DS WriteHtmlFromFile!");
}

void THISCLASS::_WriteHtmlFromHfile(HFILE hfile, int iOffsetStart, int iOffsetEnd)
{
    ENTERPROC(3, "DS WriteHtmlFromHfile(hfile=%08X,iOffsetStart=%d,iOffsetEnd=%d)", hfile, iOffsetStart, iOffsetEnd);

    if (iOffsetStart != -1)
    {
        _llseek(hfile, iOffsetStart, FILE_BEGIN);
    }
    else
    {
        ASSERT(iOffsetEnd == -1);
        iOffsetEnd = -1;
    }

    DWORD cbWrite = (iOffsetEnd == -1) ? 0xFFFFFFFF : (iOffsetEnd - iOffsetStart);

    while (cbWrite)
    {
        CHAR rgbBuf[INTERNET_MAX_URL_LENGTH];

        //
        // Read a chunk.
        //
        DWORD cbTryRead = min(cbWrite, SIZEOF(rgbBuf));
        DWORD cbActualRead = _lread(hfile, rgbBuf, cbTryRead);
        if (cbActualRead != HFILE_ERROR)
        {
            //
            // Write a chunk.
            //
            DWORD cbWritten;
            WriteFile(_hFileHtml, rgbBuf, cbActualRead, &cbWritten, NULL);

            if (cbActualRead < cbTryRead)
            {
                //
                // End of file, all done.
                //
                break;
            }

            cbWrite -= cbActualRead;
        }
        else
        {
            //
            // Error reading from file, all done.
            //
            break;
        }
    }

    EXITPROC(3, "DS WriteHtmlFromHfile!");
}

int THISCLASS::_ScanForTag(HFILE hfile, int iOffsetStart, LPCSTR pszTagA)
{
    ENTERPROC(2, "DS ScanForTag(hfile=%08X,iOffsetStart=%d,pszTagA=>%s<)",
    hfile, iOffsetStart, pszTagA);

    int iRet = -1;
    BOOL fDoneReading = FALSE;
    int iOffset;
    DWORD cchTag = lstrlenA(pszTagA);

    _llseek(hfile, iOffsetStart, FILE_BEGIN);
    iOffset = iOffsetStart;

    DWORD cbBuf = 0;
    while (!fDoneReading)
    {
        CHAR szBufA[INTERNET_MAX_URL_LENGTH];

        //
        // Fill in the buffer.
        //
        DWORD cbTryRead = SIZEOF(szBufA) - cbBuf - 1;
        DWORD cbRead = _lread(_hfileHtmlBackground, &szBufA[cbBuf], cbTryRead);
        if (cbRead != HFILE_ERROR)
        {
            cbBuf += cbRead;

            //
            // Terminate the string.
            //
            szBufA[cbBuf] = '\0';

            //
            // Scan for the tag.
            //
            LPSTR pszTagInBufA = StrStrIA(szBufA, pszTagA);

            if (pszTagInBufA)
            {
                //
                // Found the tag, compute the offset.
                //
                iRet = iOffset + pszTagInBufA - szBufA;
                fDoneReading = TRUE;
            }
            else if (cbRead < cbTryRead)
            {
                //
                // Ran out of file without finding tag.
                //
                fDoneReading = TRUE;
            }
            else
            {
                //
                // Compute how many bytes we want to throw away
                // from this buffer so we can read in more data.
                // We don't want to throw away all the bytes because
                // the tag we want may span two buffers.
                //
                DWORD cbSkip = cbBuf - cchTag;

                //
                // Advance the file offset.
                //
                iOffset += cbSkip;

                //
                // Reduce the buffer size.
                //
                cbBuf -= cbSkip;

                //
                // Move the kept bytes to the beginning of the buffer.
                //
                MoveMemory(szBufA, szBufA + cbSkip, cbBuf);
            }
        }
        else
        {
            fDoneReading = TRUE;
        }
    }

    EXITPROC(2, "DS ScanForTag=%d", iRet);
    return iRet;
}

int THISCLASS::_ScanTagEntries(HFILE hfile, int iOffsetStart, TAGENTRY *pte, int cte)
{
    ENTERPROC(2, "DS ScanTagEntries(hfile=%08X,iOffsetStart=%d,pte=%08X,cte=%d)",
    hfile, iOffsetStart, pte, cte);

    int iRet = -1;
    int i;

    for (i=0; i<cte; i++,pte++)
    {
        iRet = _ScanForTag(hfile, iOffsetStart, pte->pszTag);
        if (iRet != -1)
        {
            if (pte->fSkipPast)
            {
                iRet += lstrlenA(pte->pszTag);
            }
            break;
        }
    }

    EXITPROC(2, "DS ScanTagEntries=%d", iRet);
    return iRet;
}

void THISCLASS::_GenerateHtmlHeader(void)
{
    ENTERPROC(2, "DS GenerateHtmlHeader()");

    EnumMonitorsArea ema;
    GetMonitorSettings(&ema);

    RECT rcViewAreas[LV_MAX_WORKAREAS];  // WorkArea minus toolbar/tray areas
    int nViewAreas = ARRAYSIZE(rcViewAreas);
    // Get the ViewAreas
    if (!GetViewAreas(rcViewAreas, &nViewAreas))
    {
        nViewAreas = 0;
    }

    //
    // Write out the background and color.
    //
    TCHAR szSelectedWallpaper[INTERNET_MAX_URL_LENGTH];
    // If the wallpaper does not have a directory specified (this may happen if other apps. change this value),
    // we have to figure it out.
    GetWallpaperWithPath(_szSelectedWallpaper, szSelectedWallpaper, ARRAYSIZE(szSelectedWallpaper));
    
    BOOL fValidWallpaper = GetFileAttributes(szSelectedWallpaper) != 0xFFFFFFFF;
    if (_fSingleItem || IsWallpaperPicture(szSelectedWallpaper) || !fValidWallpaper)
    {
        // To account for the vagaries of the desktop browser (it's TopLeft starts from the TopLeft
        // of the Desktop ViewArea instead of the TopLeft of the monitor, as might be expected)
        // which happens only in the case of one active monitor systems, we add the width of the
        // tray/toolbars to the co-ordinates of the DIV section of each monitor's wallpaper.
        int iLeft, iTop;
        if(nViewAreas == 1)
        {
            iLeft = rcViewAreas[0].left - ema.rcVirtualMonitor.left;
            iTop = rcViewAreas[0].top - ema.rcVirtualMonitor.top;
        }
        else
        {
            iLeft = 0;
            iTop = 0;
        }

        //
        // Write out the standard header.
        //
        UINT i;
        for (i=IDS_COMMENT_BEGIN; i<IDS_BODY_BEGIN; i++)
        {
            _WriteHtmlFromIdF(i);
        }

        //
        // Write out the body tag, with background bitmap.
        //
        DWORD rgbDesk;
        rgbDesk = GetSysColor(COLOR_DESKTOP);

        TCHAR szBitmapFile[MAX_PATH];
        GetPerUserFileName(szBitmapFile, ARRAYSIZE(szBitmapFile), PATTERN_FILENAME);

        if (!_fSingleItem && _szSelectedWallpaper[0] && fValidWallpaper)
        {
            TCHAR szWallpaperUrl[INTERNET_MAX_URL_LENGTH];
            DWORD cch = ARRAYSIZE(szWallpaperUrl);
            UrlCreateFromPath(szSelectedWallpaper, szWallpaperUrl, &cch, URL_INTERNAL_PATH);

            switch (_wpo.dwStyle)
            {
                case WPSTYLE_TILE:
                    //
                    // Ignore the pattern, tile the wallpaper as background.
                    //
                    _WriteHtmlFromIdF(IDS_BODY_BEGIN2, szWallpaperUrl, GetRValue(rgbDesk), GetGValue(rgbDesk), GetBValue(rgbDesk));
                    break;

                case WPSTYLE_CENTER:
                    if (IsValidPattern(_szSelectedPattern))
                    {
                        //
                        // Tile the pattern as the main background.
                        //
                        _WriteHtmlFromIdF(IDS_BODY_BEGIN2, szBitmapFile, GetRValue(rgbDesk), GetGValue(rgbDesk), GetBValue(rgbDesk));
                        if(_fBackgroundHtml)   // We are generating the HTML for preview
                        {
                            _WriteHtmlFromIdF(IDS_BODY_PATTERN_AND_WP, szWallpaperUrl);
                        }
                        else
                        {
                            //
                            // Write out a DIV section for a centered, untiled wallpaper.
                            //
                            // write it out for each monitor.
                            for(int i = 0; i < ema.iMonitors; i++)
                            {
                                _WriteHtmlFromIdF(IDS_BODY_PATTERN_AND_WP2,
                                            ema.rcMonitor[i].left - ema.rcVirtualMonitor.left - iLeft,
                                            ema.rcMonitor[i].top - ema.rcVirtualMonitor.top - iTop,
                                            ema.rcMonitor[i].right - ema.rcMonitor[i].left,
                                            ema.rcMonitor[i].bottom - ema.rcMonitor[i].top,
                                            szWallpaperUrl);
                            }
                        }
                    }
                    else
                    {
                        //
                        // Write out a non-tiled, centered wallpaper as background.
                        //
                        if(_fBackgroundHtml)   // We are generating the HTML for preview
                        {
                            _WriteHtmlFromIdF(IDS_BODY_CENTER_WP, szWallpaperUrl, GetRValue(rgbDesk), GetGValue(rgbDesk), GetBValue(rgbDesk));
                        }
                        else
                        {
                            _WriteHtmlFromIdF(IDS_BODY_CENTER_WP2, GetRValue(rgbDesk), GetGValue(rgbDesk), GetBValue(rgbDesk));
                            // write it out for each monitor.
                            for(int i = 0; i < ema.iMonitors; i++)
                            {
                                _WriteHtmlFromIdF(IDS_BODY_PATTERN_AND_WP2,
                                                    ema.rcMonitor[i].left - ema.rcVirtualMonitor.left - iLeft,
                                                    ema.rcMonitor[i].top - ema.rcVirtualMonitor.top - iTop,
                                                    ema.rcMonitor[i].right - ema.rcMonitor[i].left,
                                                    ema.rcMonitor[i].bottom - ema.rcMonitor[i].top,
                                                    szWallpaperUrl);
                            }
                        }
                    }
                    break;

                case WPSTYLE_STRETCH:
                    //
                    // Ignore the pattern, create a DIV section of the wallpaper
                    // stretched to 100% of the screen.
                    //
                    _WriteHtmlFromIdF(IDS_BODY_BEGIN2, c_szNULL, GetRValue(rgbDesk), GetGValue(rgbDesk), GetBValue(rgbDesk));
                    if(_fBackgroundHtml)   // We are generating the HTML for preview
                    {
                        _WriteHtmlFromIdF(IDS_STRETCH_WALLPAPER, szWallpaperUrl);
                    }
                    else
                    {
                        // stretch it for each monitor.
                        for(int i = 0; i < ema.iMonitors; i++)
                        {
                            _WriteHtmlFromIdF(IDS_DIV_START3, ema.rcMonitor[i].left - ema.rcVirtualMonitor.left - iLeft,
                                                ema.rcMonitor[i].top - ema.rcVirtualMonitor.top - iTop,
                                                ema.rcMonitor[i].right - ema.rcMonitor[i].left,
                                                ema.rcMonitor[i].bottom - ema.rcMonitor[i].top);
                            _WriteHtmlFromIdF(IDS_STRETCH_WALLPAPER, szWallpaperUrl);
                            _WriteHtmlFromId(IDS_DIV_END);
                        }
                    }
                    break;
            }
        }
        else
        {
            //
            // Ignore the wallpaper, generate either a tiled pattern
            // or solid color background.
            //
            _WriteHtmlFromIdF(IDS_BODY_BEGIN2, !_fSingleItem && IsValidPattern(_szSelectedPattern) ? szBitmapFile : c_szNULL, GetRValue(rgbDesk), GetGValue(rgbDesk), GetBValue(rgbDesk));
        }
    }
    else
    {
        CHAR szTempFileName[MAX_PATH];
        SHTCharToAnsi(szSelectedWallpaper, szTempFileName, ARRAYSIZE(szTempFileName));
        _hfileHtmlBackground = _lopen(szTempFileName, OF_READ);
        if (_hfileHtmlBackground != HFILE_ERROR)
        {
            //
            // Figure out where to insert the base href tag.
            //
            int iOffsetBase = 0;
            BOOL fUseBaseHref = (_ScanForTag(_hfileHtmlBackground, 0, "<BASE") == -1);
            if (fUseBaseHref)
            {
                TAGENTRY rgteBase[] = {
                                        { "<HEAD>", TRUE, },
                                        { "<BODY", FALSE, },
                                        { "<HTML>", TRUE, },
                                      };
                iOffsetBase = _ScanTagEntries(_hfileHtmlBackground, 0, rgteBase, ARRAYSIZE(rgteBase));
                if (iOffsetBase == -1)
                {
                    iOffsetBase = 0;
                }
            }

            //
            // Figure out where to insert the components.
            //
            TAGENTRY rgteComponents[] = {
                                            { "</BODY>", FALSE, },
                                            { "</HTML>", FALSE, },
                                        };
            int iOffsetComponents = _ScanTagEntries(_hfileHtmlBackground, iOffsetBase, rgteComponents, ARRAYSIZE(rgteComponents));

            //
            // Write out the initial HTML up to the <HEAD> tag.
            //
            _WriteHtmlFromHfile(_hfileHtmlBackground, 0, iOffsetBase);

            //
            // Write out the base tag.
            //
            if (fUseBaseHref)
            {
                //BASE tag must point to the base "URL". So, don't strip out the filename.
                _WriteHtmlFromIdF(IDS_BASE_TAG, szSelectedWallpaper);
            }

            // Figure out where to insert the DIV clause
            TAGENTRY rgteBodyStart[] = {
                                        { "<BODY", FALSE, },
                                       };
            int iOffsetBodyStart = _ScanTagEntries(_hfileHtmlBackground, iOffsetBase, rgteBodyStart, ARRAYSIZE(rgteBodyStart));
            // Write out HTML until after the <BODY ......>
            if (iOffsetBodyStart == -1)
            {   // the <BODY> tag is not found, so we need to insert it.
                // Copy over stuff until </HEAD>
                TAGENTRY rgteHeadEnd[] = {
                                            { "</HEAD>", TRUE, },
                                         };
                int iOffsetHeadEnd = _ScanTagEntries(_hfileHtmlBackground, iOffsetBase, rgteHeadEnd, ARRAYSIZE(rgteHeadEnd));
                if(iOffsetHeadEnd != -1)
                {
                    _WriteHtmlFromHfile(_hfileHtmlBackground, iOffsetBase, iOffsetHeadEnd);
                    iOffsetBase = iOffsetHeadEnd;
                }
                _WriteHtmlFromIdF(IDS_BODY_CENTER_WP2); // "<BODY>"
                _fNeedBodyEnd = TRUE;
            }
            else
            {
                TAGENTRY rgteBodyEnd[] = {
                                            { ">", TRUE, },
                                         };
                int iOffsetBodyEnd = _ScanTagEntries(_hfileHtmlBackground, iOffsetBodyStart, rgteBodyEnd, ARRAYSIZE(rgteBodyEnd));
                if (iOffsetBodyEnd == -1)
                {   // An error in the HTML.
                    iOffsetBodyEnd = iOffsetBodyStart;  // BUGBUG: We need a better recovery idea.
                }
                _WriteHtmlFromHfile(_hfileHtmlBackground, iOffsetBase, iOffsetBodyEnd);
                iOffsetBase = iOffsetBodyEnd;
            }
            // Insert the DIV clause
            if(ema.iMonitors > 1)
            {
                if(nViewAreas <= 0 || rcViewAreas[0].right == rcViewAreas[0].left)
                // The second case could occur on bootup
                {
                    // Some error occured when getting the ViewAreas. Recover from the error by using the workarea.
                    // Get the workarea of the primary monitor, since HTML wallpapers are displayed only there.
                    HMONITOR hMonPrimary = GetPrimaryMonitor();
                    GetMonitorWorkArea(hMonPrimary, &rcViewAreas[0]);
                }
                _WriteHtmlFromIdF(IDS_DIV_START3, rcViewAreas[0].left - ema.rcVirtualMonitor.left,
                rcViewAreas[0].top - ema.rcVirtualMonitor.top,
                rcViewAreas[0].right - rcViewAreas[0].left, rcViewAreas[0].bottom - rcViewAreas[0].top);
            }

            //
            // Write out HTML from after <HEAD> tag to just before </BODY> tag.
            //
            _WriteHtmlFromHfile(_hfileHtmlBackground, iOffsetBase, iOffsetComponents);

            if(ema.iMonitors > 1)
            {
                _WriteHtmlFromId(IDS_DIV_END);
            }
        }
        else
        {
            _hfileHtmlBackground = NULL;
        }
    }

    EXITPROC(2, "DS GenerateHtmlHeader!");
}

void THISCLASS::_WriteResizeable(COMPONENTA *pcomp)
{
    TCHAR   szResizeable[3];

    szResizeable[0] = TEXT('\0');

    //If Resize is set, then the comp is resizeable in both X and Y directions!
    if(pcomp->cpPos.fCanResize)
        lstrcat(szResizeable, TEXT("XY"));
    else
    {
        if(pcomp->cpPos.fCanResizeX)
            lstrcat(szResizeable, TEXT("X"));

        if(pcomp->cpPos.fCanResizeY)
            lstrcat(szResizeable, TEXT("Y"));
    }

    _WriteHtmlFromIdF(IDS_RESIZEABLE, szResizeable);
}

void THISCLASS::_GenerateHtmlPicture(COMPONENTA *pcomp)
{
    ENTERPROC(2, "DS GenerateHtmlPicture(pcomp=%08X)");

    //
    // Write out the image src HTML.
    //
    TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
    DWORD cch=ARRAYSIZE(szUrl);
    if (FAILED(UrlCreateFromPath(pcomp->szSource, szUrl, &cch, 0)))
    {
        lstrcpy(szUrl, pcomp->szSource);
    }
    _WriteHtmlFromIdF(IDS_IMAGE_BEGIN2, pcomp->dwID, szUrl);

    //
    // Write out whether this image is resizeable or not!
    //
    _WriteResizeable(pcomp);

    //
    // Write out the URL that must be used for subscription purposes.
    //
    _WriteHtmlFromIdF(IDS_SUBSCRIBEDURL, pcomp->szSubscribedURL);

    //
    // Write out the image location HTML.
    //
    if ((pcomp->cpPos.dwWidth == COMPONENT_DEFAULT_WIDTH) &&
        (pcomp->cpPos.dwHeight == COMPONENT_DEFAULT_HEIGHT))
    {
        _WriteHtmlFromIdF(IDS_IMAGE_LOCATION, _fSingleItem ? 0 : pcomp->cpPos.iLeft, _fSingleItem ? 0 : pcomp->cpPos.iTop, pcomp->cpPos.izIndex);
    }
    else
    {
        _WriteHtmlFromIdF(IDS_IMAGE_SIZE, _fSingleItem ? 0 : pcomp->cpPos.iLeft, _fSingleItem ? 0 : pcomp->cpPos.iTop,
                            pcomp->cpPos.dwWidth, pcomp->cpPos.dwHeight, pcomp->cpPos.izIndex);
    }

    EXITPROC(2, "DS GenerateHtmlPicture!");
}

void THISCLASS::_GenerateHtmlDoc(COMPONENTA *pcomp)
{
    ENTERPROC(2, "DS GenerateHtmlDoc(pcomp=%08X)");

    //
    // Write out the DIV header HTML.
    //
    _WriteHtmlFromIdF(IDS_DIV_START2, pcomp->dwID);

    //
    // Write out whether this component is resizeable or not!
    //
    _WriteResizeable(pcomp);

    //
    // Write out the DIV location HTML.
    //
    _WriteHtmlFromIdF(IDS_DIV_SIZE, pcomp->cpPos.dwHeight, _fSingleItem ? 0 : pcomp->cpPos.iLeft,
    _fSingleItem ? 0 : pcomp->cpPos.iTop, pcomp->cpPos.dwWidth, pcomp->cpPos.izIndex);

    //
    // Extract the doc contents directly into the HTML.
    //
    _WriteHtmlFromFile(pcomp->szSource);

    //
    // Close the DIV section.
    //
    _WriteHtmlFromId(IDS_DIV_END);

    EXITPROC(2, "DS GenerateHtmlDoc!");
}

void THISCLASS::_GenerateHtmlSite(COMPONENTA *pcomp)
{
    ENTERPROC(2, "DS GenerateHtmlSite(pcomp=%08X)");

    //
    // Write out the frame src HTML.
    //
    TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
    DWORD cch=ARRAYSIZE(szUrl);
    if (FAILED(UrlCreateFromPath(pcomp->szSource, szUrl, &cch, 0)))
    {
        lstrcpy(szUrl, pcomp->szSource);
    }
    _WriteHtmlFromIdF(IDS_IFRAME_BEGIN2, pcomp->dwID, szUrl, pcomp->fNoScroll ? TEXT("scrolling=no") : c_szNULL);

    //
    // Write out whether this Component is resizeable or not!
    //
    _WriteResizeable(pcomp);

    //
    // Write out the URL that must be used for subscription purposes.
    //
    _WriteHtmlFromIdF(IDS_SUBSCRIBEDURL, pcomp->szSubscribedURL);

    //
    // Write out the frame location HTML.
    //
    _WriteHtmlFromIdF(IDS_IFRAME_SIZE, _fSingleItem ? 0 : pcomp->cpPos.iLeft, _fSingleItem ? 0 : pcomp->cpPos.iTop,
    pcomp->cpPos.dwWidth, pcomp->cpPos.dwHeight, pcomp->cpPos.izIndex);

    EXITPROC(2, "DS GenerateHtmlSite!");
}

void THISCLASS::_GenerateHtmlControl(COMPONENTA *pcomp)
{
    ENTERPROC(2, "DS GenerateHtmlControl(pcomp=%08X)");
    ASSERT(pcomp);
    
    // Did the Administrator restrict "Channel UI"?
    if (SHRestricted2W(REST_NoChannelUI, NULL, 0))
    {
        TCHAR szChannelOCGUID[GUIDSTR_MAX];

        SHStringFromGUID(CLSID_ChannelOC, szChannelOCGUID, ARRAYSIZE(szChannelOCGUID));
        if (!StrCmpNI(pcomp->szSource, &(szChannelOCGUID[1]), lstrlen(pcomp->szSource)-3))
        {
            // Yes, so we need to hide the Channel Desktop Component.
            // Return here before we generate it.
            return;
        }        
    }
    
    //
    // Write out the control HTML.
    //

    // First the control header
    _WriteHtmlFromIdF(IDS_CONTROL_1, pcomp->dwID);
    // then the size
    _WriteHtmlFromIdF(IDS_CONTROL_2, pcomp->cpPos.dwHeight, _fSingleItem ? 0 : pcomp->cpPos.iLeft,
    _fSingleItem ? 0 : pcomp->cpPos.iTop, pcomp->cpPos.dwWidth, pcomp->cpPos.izIndex);
    //
    // Write out whether this Control is resizeable or not!
    //
    _WriteResizeable(pcomp);

    // Finally the rest of the control
    _WriteHtmlFromIdF(IDS_CONTROL_3, pcomp->szSource);

    EXITPROC(2, "DS GenerateHtmlControl!");
}

void THISCLASS::_GenerateHtmlComponent(COMPONENTA *pcomp)
{
    ENTERPROC(2, "DS GenerateHtmlComponent(pcomp=%08X)");

    switch(pcomp->iComponentType)
    {
        case COMP_TYPE_PICTURE:
            _GenerateHtmlPicture(pcomp);
            break;

        case COMP_TYPE_HTMLDOC:
            _GenerateHtmlDoc(pcomp);
            break;

        case COMP_TYPE_WEBSITE:
            _GenerateHtmlSite(pcomp);
            break;

        case COMP_TYPE_CONTROL:
            _GenerateHtmlControl(pcomp);
            break;
    }

    EXITPROC(2, "DS GenerateHtmlComponent!");
}

void THISCLASS::_GenerateHtmlFooter(void)
{
    ENTERPROC(2, "DS GenerateHtmlFooter()");

    //
    // Write out the deskmovr object.
    //
    if (!_fNoDeskMovr)
    {
        TCHAR szDeskMovrFile[MAX_PATH];

        GetWindowsDirectory(szDeskMovrFile, ARRAYSIZE(szDeskMovrFile));
        lstrcat(szDeskMovrFile, DESKMOVR_FILENAME);
        _WriteHtmlFromFile(szDeskMovrFile);
    }

    //
    // Write out the concluding HTML tags.
    //
    if (_hfileHtmlBackground)
    {
        if(_fNeedBodyEnd)
        {    // We had introduced the <BODY> tag by ourselves.
            _WriteHtmlFromId(IDS_BODY_END2);
            _fNeedBodyEnd = FALSE;
        }
        _WriteHtmlFromHfile(_hfileHtmlBackground, -1, -1);
        _lclose(_hfileHtmlBackground);
        _hfileHtmlBackground = NULL;
    }
    else
    {
        _WriteHtmlFromId(IDS_BODY_END);
    }

    EXITPROC(2, "DS GenerateHtmlFooter!");
}

void THISCLASS::_GenerateHtml(void)
{
    ENTERPROC(2, "DS GenerateHtml()");

    TCHAR szHtmlFile[MAX_PATH];

    //
    // Compute the filename.
    //
    szHtmlFile[0] = TEXT('\0');

    GetPerUserFileName(szHtmlFile, ARRAYSIZE(szHtmlFile), DESKTOPHTML_FILENAME);

    //
    // Recreate the file.
    //
    _hFileHtml = CreateFile(szHtmlFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);
    if (_hFileHtml != INVALID_HANDLE_VALUE)
    {
        _GenerateHtmlHeader();

        if (_co.fEnableComponents && _hdsaComponent && DSA_GetItemCount(_hdsaComponent))
        {
            int i;

            for (i=0; i<DSA_GetItemCount(_hdsaComponent); i++)
            {
                COMPONENTA comp;
                comp.dwSize = sizeof(COMPONENTA);

                if ((DSA_GetItem(_hdsaComponent, i, &comp) != -1) && (comp.fChecked))
                {
                    _GenerateHtmlComponent(&comp);
                }
            }
        }

        _GenerateHtmlFooter();
        CloseHandle(_hFileHtml);
    }
    
    SetDesktopFlags(COMPONENTS_DIRTY, 0);

    EXITPROC(2, "DS GenerateHtml!");
}

HRESULT THISCLASS::GenerateDesktopItemHtml(LPCWSTR pwszFileName, COMPONENT *pcomp, DWORD dwReserved)
{
    HRESULT hres = E_FAIL;
    ENTERPROC(2, "DS GenerateComponentHtml(pcomp=%08X)", pcomp);
    LPTSTR  pszFileName;

    //Check for the input parameters
    if(!pwszFileName || (pcomp && (pcomp->dwSize != SIZEOF(*pcomp)) && (pcomp->dwSize != SIZEOF(IE4COMPONENT))))
        return E_INVALIDARG;

    ASSERT(!dwReserved);     // These should be 0

#ifndef UNICODE
    CHAR    szFileName[MAX_PATH];

    SHUnicodeToAnsi(pwszFileName, szFileName, ARRAYSIZE(szFileName));
    pszFileName = szFileName;
#else
    pszFileName = (LPTSTR)pwszFileName;
#endif

    //
    // Create the file.
    //
    _hFileHtml = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, NULL);
    if (_hFileHtml != INVALID_HANDLE_VALUE)
    {
        _fNoDeskMovr = TRUE;
        _fBackgroundHtml = TRUE;
        //Check if we need to add a component
        if(pcomp)
        {
            COMPONENTA  CompA;

            CompA.dwSize = sizeof(CompA);
            WideCompToMultiComp(pcomp, &CompA);

            _fSingleItem = TRUE;
            _GenerateHtmlHeader();

            _GenerateHtmlComponent(&CompA);
            _GenerateHtmlFooter();
            _fSingleItem = FALSE;
        }
        else
        {
            //generate just the header and the footer with proper
            // wallpaper and pattern info!
            _GenerateHtmlHeader();
            _GenerateHtmlFooter();
        }
        _fBackgroundHtml = FALSE;
        _fNoDeskMovr = FALSE;

        CloseHandle(_hFileHtml);

        hres = S_OK;
    }
    _hFileHtml = NULL;

    EXITPROC(2, "DS GenerateComponentHtml=%d", hres);
    return hres;
}

//
// AddUrl
//
//

HRESULT THISCLASS::AddUrl(HWND hwnd, LPCWSTR pszSourceW, LPCOMPONENT pcomp, DWORD dwFlags)
{
    LPTSTR pszExt;
    HRESULT fOkay = TRUE;
    BOOL fExtIsCdf,fPathIsUrl;
    BOOL fSubscribed = FALSE;
    COMPONENT   compLocal;
    COMPONENTA  compA;
    TCHAR szSource[INTERNET_MAX_URL_LENGTH];

    compLocal.dwSize = sizeof(compLocal);
    compA.dwSize = sizeof(compA);

    //Check for the input parameters.
    if(!pszSourceW || (pcomp && (pcomp->dwSize != SIZEOF(*pcomp)) && (pcomp->dwSize != SIZEOF(IE4COMPONENT))))
        return E_INVALIDARG;

    // Catch folks that call our API's to add components and prevent them from doing
    // so if the restriction is in place.
    if (SHIsRestricted(NULL, REST_NOADDDESKCOMP))
        return E_ACCESSDENIED;
    
    g_pActiveDesk = this;
    SHUnicodeToTChar(pszSourceW, szSource, ARRAYSIZE(szSource));
    pszExt = PathFindExtension(szSource);
    fExtIsCdf = lstrcmpi(pszExt, TEXT(".CDF")) == 0;
    fPathIsUrl = PathIsURL(szSource) && !UrlIsFileUrl(szSource);

    if (!pcomp)
    {
        pcomp = &compLocal;
        pcomp->dwSize = SIZEOF(*pcomp);
    }

    if (FindComponent(szSource))
    {
        if (dwFlags & ADDURL_SILENT)  
        {
            lstrcpy(compA.szSource, szSource);
            MultiCompToWideComp(&compA, pcomp);
            RemoveDesktopItem(pcomp, 0);
        }
        else  
        {
            // This is a long string. So,...
            TCHAR szMsg[512];
            TCHAR szMsg2[256];
            TCHAR szTitle[128];
            MLLoadString(IDS_COMP_EXISTS, szMsg, ARRAYSIZE(szMsg));
            MLLoadString(IDS_COMP_EXISTS_2, szMsg2, ARRAYSIZE(szMsg2));
            lstrcat(szMsg, szMsg2);
            MLLoadString(IDS_COMP_TITLE, szTitle, ARRAYSIZE(szTitle));
            MessageBox(hwnd, szMsg, szTitle, MB_OK);

            fOkay = FALSE;
        }
    }

    if (fOkay && CheckForExistingSubscription(szSource))
    {
        if ((dwFlags & ADDURL_SILENT) ||
            (ShellMessageBox(MLGetHinst(), hwnd, MAKEINTRESOURCE(IDS_COMP_SUBSCRIBED), 
                 MAKEINTRESOURCE(IDS_COMP_TITLE), MB_YESNO) == IDYES))
        {
            DeleteFromSubscriptionList(szSource);
        }
        else
        {
            fOkay = FALSE;
        }
    }

    if (fOkay)
    {
        if (fPathIsUrl || fExtIsCdf)
        {
            WCHAR szUrlW[INTERNET_MAX_URL_LENGTH];
            SHTCharToUnicode(szSource, szUrlW, ARRAYSIZE(szUrlW));

            HRESULT hr = ParseDesktopComponent(hwnd, szUrlW, pcomp);
            if (SUCCEEDED(hr))
            {
                //
                // Convert ed's wide thinggy to multi.
                //
                WideCompToMultiComp(pcomp, &compA);
                PositionComponent(&compA.cpPos, compA.iComponentType);

                fSubscribed = TRUE;
            }
            else if (!fExtIsCdf)
            {
                //
                // This is some non-CDF url.
                //
                CreateComponent(&compA, szSource);
            }
            else
            {
                //
                // We barfed on a CDF, bring up an error message.
                //
                if (!(dwFlags & ADDURL_SILENT))
                {
                    ShellMessageBox(MLGetHinst(), hwnd, MAKEINTRESOURCE(IDS_COMP_BADURL), 
                                    MAKEINTRESOURCE(IDS_COMP_TITLE), MB_OK);
                }
                fOkay = FALSE;
            }
        }
        else
        {
            //
            // This is just some local file.
            //
            CreateComponent(&compA, szSource);
        }
    }

    if (fOkay && fPathIsUrl && !fSubscribed)
    {
        //
        // Run subscription code on URLs if CDF code hasn't already.
        //
        if (dwFlags & ADDURL_SILENT)
        {
            ISubscriptionMgr *psm;

            if (SUCCEEDED(CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                            CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr, (void**)&psm)))
            {
                WCHAR wzURL[INTERNET_MAX_URL_LENGTH];
                LPWSTR pwzURL = wzURL;
                //We need to zero init this structure except the cbSize field.
                SUBSCRIPTIONINFO siDefault = {sizeof(SUBSCRIPTIONINFO)};

                #ifndef UNICODE
                    SHAnsiToUnicode(szSource, wzURL, ARRAYSIZE(wzURL));
                #else // UNICODE
                    pwzURL = szSource;
                #endif // UNICODE

                //This field is already initialized above.
                //siDefault.cbSize = sizeof(siDefault);
                psm->CreateSubscription(hwnd, pwzURL, pwzURL, CREATESUBS_NOUI, SUBSTYPE_DESKTOPURL, &siDefault);
                psm->UpdateSubscription(pwzURL);
                psm->Release();
            }
        }
        else
        {
            HRESULT hres = CreateSubscriptionsWizard(SUBSTYPE_DESKTOPURL, szSource, NULL, hwnd);
            if(!SUCCEEDED(hres))  //Some error, or the user chose Cancel - we should fail.
            {
                ShellMessageBox(MLGetHinst(), hwnd, MAKEINTRESOURCE(IDS_COMP_BADSUBSCRIBE), 
                                MAKEINTRESOURCE(IDS_COMP_TITLE), MB_OK);
            }
            fOkay = (hres == S_OK);    //could be S_FALSE, which means CreateSubscription was cancelled
            //so we don't display the above error, but we don't create the DTI
        }
    }

    MultiCompToWideComp(&compA, pcomp);
 
    if (fOkay)
    {
        AddDesktopItem(pcomp, 0);
        return S_OK;
    }
    else  
    {
        return E_FAIL;
    }
}

void THISCLASS::_SaveSettings(DWORD dwFlags)
{
    ENTERPROC(2, "DS SaveSettings()");

    if (dwFlags & AD_APPLY_SAVE)
    {
        // Don't ever modify the safemode settings
        TCHAR lpszDeskcomp[MAX_PATH];
        GetRegLocation(lpszDeskcomp, REG_DESKCOMP_GENERAL, _pszScheme);
        if (!StrStr(lpszDeskcomp, REG_DESKCOMP_SAFEMODE_SUFFIX))
        {
            //
            // Write out registry settings.
            //
            _SaveWallpaper();
            _SaveComponents();
            _SavePattern(SAVE_PATTERN_NAME);
        }
    };

    if (dwFlags & AD_APPLY_HTMLGEN)
    {
        //We need to generate the Patten.bmp file too!
        _SavePattern(GENERATE_PATTERN_FILE);

        //
        // Write out HTML file.
        //
        _GenerateHtml();
    }

    if (dwFlags & AD_APPLY_REFRESH)
    {
        HWND    hwndShell = GetShellWindow();
        //
        // Broadcast the world that the settings have changed.
        //
        SHELLSTATE ss = {0};
        SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE);
        BOOL fWasActiveDesktop = BOOLIFY(ss.fDesktopHTML);
        BOOL fIsActiveDesktop = BOOLIFY(_co.fActiveDesktop);
        if (fIsActiveDesktop != fWasActiveDesktop)
        {
            SendMessage(hwndShell, WM_WININICHANGE, SPI_SETDESKWALLPAPER, (LPARAM)TEXT("ToggleDesktop"));
            //Force a SHRefresh with this dummy call
            SHGetSetSettings(NULL, 0, TRUE);
        }
        else
        {

            SendMessage(hwndShell, WM_WININICHANGE, SPI_SETDESKWALLPAPER, 
                    (LPARAM)((dwFlags & AD_APPLY_BUFFERED_REFRESH) ? c_szBufferedRefresh : c_szRefreshDesktop));
        }
    }

    //
    // Data is no longer dirty.
    //
    _fDirty = FALSE;
    _fWallpaperDirty = FALSE;
    _fWallpaperChangedDuringInit = FALSE;
    _fPatternDirty = FALSE;

    EXITPROC(2, "DS SaveSettings!");
}

ULONG THISCLASS::AddRef(void)
{
    ENTERPROC(1, "DS AddRef()");

    _cRef++;

    EXITPROC(1, "DS AddRef=%d", _cRef);
    return _cRef;
}

HRESULT THISCLASS::ApplyChanges(DWORD dwFlags)
{
    HRESULT hres = E_FAIL;
    ENTERPROC(1, "DS Apply(dwFlags=%08X)", dwFlags);

    if (dwFlags & AD_APPLY_FORCE)
    {
        _fDirty = TRUE;
        _fWallpaperDirty = TRUE;
        _fPatternDirty = TRUE;
    }

    if (_fDirty || _fWallpaperChangedDuringInit)
    {
        _SaveSettings(dwFlags);
    }

    hres = S_OK;

    EXITPROC(1, "DS ApplyChanges=%d", hres);

    return hres;
}

ULONG THISCLASS::Release(void)
{
    UINT nRet = --_cRef;
    ENTERPROC(1, "DS Release()");

    if (_cRef == 0)
    {
        delete this;
    }

    EXITPROC(1, "DS Release=%d", nRet);
    return nRet;
}

THISCLASS::CActiveDesktop()
{
    _cRef = 1;
    _fNoDeskMovr = FALSE;
    _fBackgroundHtml = FALSE;
    DllAddRef();
}

THISCLASS::~CActiveDesktop()
{
    if (_hdsaComponent)
    {
        DSA_Destroy(_hdsaComponent);
    }
    if (_pszScheme)
    {
        LocalFree((HANDLE)_pszScheme);
    }
    DllRelease();
}

HRESULT THISCLASS::GetWallpaper(LPWSTR pwszWallpaper, UINT cchWallpaper, DWORD dwReserved)
{
    HRESULT hres = E_INVALIDARG;
    ENTERPROC(1, "DS GetWallpaper(pszWallpaper=%08X,cchWallpaper=%d)", pwszWallpaper, cchWallpaper);

    ASSERT(!dwReserved);     // These should be 0

    if (pwszWallpaper && cchWallpaper)
    {
#ifndef UNICODE
        MultiByteToWideChar(CP_ACP, 0, _szSelectedWallpaper, -1, pwszWallpaper, cchWallpaper);
#else
        StrCpyNW(pwszWallpaper, _szSelectedWallpaper, cchWallpaper);
#endif
        hres = S_OK;
    }
    else
    {
        TraceMsg(TF_WARNING, "DS GetWallpaper unable to return wallpaper");
    }

    EXITPROC(1, "DS GetWallpaper=%d", hres);
    return hres;
}

HRESULT THISCLASS::SetWallpaper(LPCWSTR pwszWallpaper, DWORD dwReserved)
{
    HRESULT hres = E_FAIL;
    LPCTSTR pszWallpaper;

    ASSERT(!dwReserved);     // These should be 0

    if (pwszWallpaper)
    {
#ifdef UNICODE
        pszWallpaper = pwszWallpaper;
#else
        TCHAR szWallpaper[INTERNET_MAX_URL_LENGTH];
        WideCharToMultiByte(CP_ACP, 0, pwszWallpaper, -1, szWallpaper, ARRAYSIZE(szWallpaper), NULL, NULL);
        pszWallpaper = szWallpaper;
#endif

        if (lstrcmp(_szSelectedWallpaper, pszWallpaper) != 0)
        {
            lstrcpyn(_szSelectedWallpaper, pszWallpaper, ARRAYSIZE(_szSelectedWallpaper));
            _fWallpaperDirty = TRUE;
            _fDirty = TRUE;
        }
        hres = S_OK;
    }

    ENTERPROC(1, "DS SetWallpaper(pszWallpaper=>%s<)", pwszWallpaper ? pszWallpaper : TEXT("(NULL)"));

    EXITPROC(1, "DS SetWallpaper=%d", hres);
    return hres;
}

HRESULT THISCLASS::GetWallpaperOptions(WALLPAPEROPT *pwpo, DWORD dwReserved)
{
    HRESULT hres = E_INVALIDARG;
    ENTERPROC(1, "DS GetWallpaperOptions(pwpo=%08X)");

    ASSERT(!dwReserved);     // These should be 0

    if ((pwpo) && (pwpo->dwSize == SIZEOF(*pwpo)))
    {
        *pwpo = _wpo;
        hres = S_OK;
    }
    else
    {
        TraceMsg(TF_WARNING, "DS GetWallpaperOptions could not return options");
    }

    EXITPROC(1, "DS GetWallpaperOptions=%d", hres);
    return hres;
}

HRESULT THISCLASS::SetWallpaperOptions(LPCWALLPAPEROPT pwpo, DWORD dwReserved)
{
    HRESULT hres = E_INVALIDARG;
    ENTERPROC(1, "DS SetWallpaperOptions(pwpo=%08X)", pwpo);

    ASSERT(!dwReserved);     // These should be 0

    if ((pwpo) && (pwpo->dwSize == SIZEOF(*pwpo)))
    {
        _wpo = *pwpo;
        _fWallpaperDirty = TRUE;
        _fDirty = TRUE;
        hres = S_OK;
    }
    else
    {
        TraceMsg(TF_WARNING, "DS SetWallpaperOptions could not set options");
    }

    EXITPROC(1, "DS SetWallpaperOptions=%d", hres);
    return hres;
}

HRESULT THISCLASS::GetPattern(LPWSTR pwszPattern, UINT cchPattern, DWORD dwReserved)
{
    HRESULT hres = S_OK;
    ENTERPROC(1, "DS GetPattern(psz=%08X,cch=%d)", pwszPattern, cchPattern);

    ASSERT(!dwReserved);     // These should be 0

    if(!pwszPattern || (cchPattern == 0))
        return (E_INVALIDARG);

#ifndef UNICODE
    MultiByteToWideChar(CP_ACP, 0, _szSelectedPattern, -1, pwszPattern, cchPattern);
#else
    StrCpyNW(pwszPattern, _szSelectedPattern, cchPattern);
#endif

    EXITPROC(1, "DS GetPattern=%d", hres);
    return hres;
}

HRESULT THISCLASS::SetPattern(LPCWSTR pwszPattern, DWORD dwReserved)
{
    HRESULT hres = E_FAIL;
    LPCTSTR pszPattern;

    ASSERT(!dwReserved);     // These should be 0

    if (pwszPattern)
    {
#ifndef UNICODE
        TCHAR szPattern[INTERNET_MAX_URL_LENGTH];
        SHUnicodeToAnsi(pwszPattern, szPattern, ARRAYSIZE(szPattern));
        pszPattern = szPattern;
#else
        pszPattern = pwszPattern;
#endif

        if (lstrcmp(_szSelectedPattern, pszPattern) != 0)
        {
            lstrcpyn(_szSelectedPattern, pszPattern, ARRAYSIZE(_szSelectedPattern));

            _fPatternDirty = TRUE;
            _fDirty = TRUE;

            hres = S_OK;
        }
    }

    ENTERPROC(1, "DS SetPattern(psz=>%s<)", pwszPattern ? pszPattern : TEXT("(NULL)"));

    EXITPROC(1, "DS SetPattern=%d", hres);
    return hres;
}

HRESULT THISCLASS::GetDesktopItemOptions(COMPONENTSOPT *pco, DWORD dwReserved)
{
    HRESULT hres = E_INVALIDARG;
    ENTERPROC(1, "DS GetComponentsOptions(pco=%08X)", pco);

    ASSERT(!dwReserved);     // These should be 0

    if (pco && (pco->dwSize == SIZEOF(*pco)))
    {
        *pco = _co;
        hres = S_OK;
    }
    else
    {
        TraceMsg(TF_WARNING, "DS GetComponentsOptions unable to return options");
    }

    EXITPROC(1, "DS GetComponentsOptions=%d", hres);
    return hres;
}

HRESULT THISCLASS::SetDesktopItemOptions(LPCCOMPONENTSOPT pco, DWORD dwReserved)
{
    HRESULT hres = E_INVALIDARG;
    ENTERPROC(1, "DS SetComponentsOptions(pco=%08X)", pco);

    ASSERT(!dwReserved);     // These should be 0

    if (pco && (pco->dwSize == SIZEOF(*pco)))
    {
        _co = *pco;
        _fDirty = TRUE;
        hres = S_OK;
    }
    else
    {
        TraceMsg(TF_WARNING, "DS SetComponentsOptions unable to set options");
    }

    EXITPROC(1, "DS SetComponentsOptions=%d", hres);
    return hres;
}

//
// SetStateInfo()
//      This function simply sets up the COMPSTATEINFO structure passed using the current
// position and size from the COMPPOS structure and the itemState passed.
//
void SetStateInfo(COMPSTATEINFO *pCompStateInfo, COMPPOS *pCompPos, DWORD dwItemState)
{
    pCompStateInfo->dwSize   = sizeof(*pCompStateInfo);
    pCompStateInfo->iLeft    = pCompPos->iLeft;
    pCompStateInfo->iTop     = pCompPos->iTop;
    pCompStateInfo->dwWidth  = pCompPos->dwWidth;
    pCompStateInfo->dwHeight = pCompPos->dwHeight;
    pCompStateInfo->dwItemState = dwItemState;
}

void ConvertCompStruct(COMPONENTA *pCompDest, COMPONENTA *pCompSrc, BOOL fPubToPriv)
{
    pCompDest -> dwID = pCompSrc -> dwID;
    pCompDest -> iComponentType = pCompSrc -> iComponentType;
    pCompDest -> fChecked = pCompSrc -> fChecked;
    pCompDest -> fDirty = pCompSrc -> fDirty;
    pCompDest -> fNoScroll = pCompSrc -> fNoScroll;
    pCompDest -> cpPos = pCompSrc -> cpPos;

    if(fPubToPriv)
    {
        COMPONENT *pComp = (COMPONENT *)pCompSrc;

        pCompDest->dwSize = sizeof(COMPONENTA);
        SHUnicodeToTChar(pComp->wszSource, pCompDest->szSource, ARRAYSIZE(pCompDest->szSource));
        SHUnicodeToTChar(pComp->wszFriendlyName, pCompDest->szFriendlyName, ARRAYSIZE(pCompDest->szFriendlyName));
        SHUnicodeToTChar(pComp->wszSubscribedURL, pCompDest->szSubscribedURL, ARRAYSIZE(pCompDest->szSubscribedURL));
        
        //Check to see if the public component is from IE4 app (old size)
        if(pCompSrc->dwSize == sizeof(COMPONENT))
        {
            // Since the dest component is the same size as the most current structure, all fields
            // are valid.
            // CAUTION: The following fields are at a different offset in public and private 
            // structures. So, you need to use pcomp instead of pCompSrc for example.
            pCompDest->dwCurItemState = pComp->dwCurItemState;
            pCompDest->csiOriginal = pComp->csiOriginal;
            pCompDest->csiRestored = pComp->csiRestored;
        }
        else
        {
            // Since the size did not match, we assume that this is an older structure.
            // Since the older struct does not have any Original and Restored sizes, let's copy 
            // the default values.
            IE4COMPONENT   *pIE4Comp = (IE4COMPONENT *)pCompSrc;
            pCompDest->dwCurItemState = IS_NORMAL;
            SetStateInfo(&pCompDest->csiOriginal, &pIE4Comp->cpPos, IS_NORMAL);
            SetStateInfo(&pCompDest->csiRestored, &pIE4Comp->cpPos, IS_NORMAL);
        }
    }
    else
    {
        COMPONENT *pComp = (COMPONENT *)pCompDest;
        
        if(pCompDest->dwSize != sizeof(COMPONENT))
            pCompDest->dwSize = sizeof(IE4COMPONENT);
        SHTCharToUnicode(pCompSrc->szSource, pComp->wszSource, ARRAYSIZE(pComp->wszSource));
        SHTCharToUnicode(pCompSrc->szFriendlyName, pComp->wszFriendlyName, ARRAYSIZE(pComp->wszFriendlyName));
        SHTCharToUnicode(pCompSrc->szSubscribedURL, pComp->wszSubscribedURL, ARRAYSIZE(pComp->wszSubscribedURL));
        
        //Check to see if the public component is from IE4 app (old size)
        if(pComp->dwSize == sizeof(COMPONENT))
        {
            // Since the dest component is the same size as the most current structure, all fields
            // are valid.
            // CAUTION: The following fields are at a different offset in public and private 
            // structures. So, you need to use pcomp instead of pCompDest for example.
            pComp->dwCurItemState = pCompSrc->dwCurItemState;
            pComp->csiOriginal = pCompSrc->csiOriginal;
            pComp->csiRestored = pCompSrc->csiRestored;
        }
        // else, the dest component is IE4COMPONENT and the additional fields are not there.
    }
}


HRESULT THISCLASS::_AddDTIWithUIPrivateA(HWND hwnd, LPCCOMPONENT pComp, DWORD dwFlags)
{
    HRESULT hres = E_FAIL;
    TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
    int nScheme;

#ifdef UNICODE
    StrCpyN(szUrl, pComp->wszSource, ARRAYSIZE(szUrl));
#else // UNICODE
    SHUnicodeToAnsi(pComp->wszSource, szUrl, ARRAYSIZE(szUrl));
#endif // UNICODE

    nScheme = GetUrlScheme(szUrl);
    if ((URL_SCHEME_INVALID == nScheme) || (URL_SCHEME_UNKNOWN == nScheme))
    {
        TCHAR szFullyQualified[INTERNET_MAX_URL_LENGTH];
        DWORD cchSize = ARRAYSIZE(szFullyQualified);

        if (SUCCEEDED(ParseURLFromOutsideSource(szUrl, szFullyQualified, &cchSize, NULL)))
            nScheme = GetUrlScheme(szFullyQualified);
    }

    // Is this URL valid to subscribe to?  Did the caller specify they want use
    // to try to subscribe to it?
    if ((URL_SCHEME_FILE != nScheme) && (URL_SCHEME_ABOUT != nScheme) && 
        IsFlagSet(dwFlags, DTI_ADDUI_DISPSUBWIZARD) && hwnd)
    {
        //Create a subscription.
        hres = CreateSubscriptionsWizard(SUBSTYPE_DESKTOPURL, szUrl, NULL, hwnd);
        if(hres != S_OK)
        {
            return hres;
        }
    }

    //
    // Add the component to the registry.
    //

    // BUGBUG: This sucks because this function creates a second COM objects.  
    //         We need to Inline the functionality.
    hres = AddDesktopComponentNoUI(AD_APPLY_ALL, szUrl, NULL, pComp->iComponentType, 
        pComp->cpPos.iLeft, pComp->cpPos.iTop, pComp->cpPos.dwWidth, pComp->cpPos.dwHeight, NULL, TRUE) ? S_OK : E_FAIL;

    return hres;
}



#define     STC_DESKTOPCOMPONENT    0x00000002
EXTERN_C STDAPI_(HRESULT) SubscribeToCDF(HWND hwndParent, LPCWSTR pwzUrl, DWORD dwCDFTypes);

HRESULT THISCLASS::AddDesktopItemWithUI(HWND hwnd, LPCOMPONENT pComp, DWORD dwFlags)
{
    HRESULT hres = E_FAIL;

    // We need to support IE4 apps calling with the old component structure too!
    // We use the size field to detect IE4 v/s newer apps!
    if(!pComp || ((pComp->dwSize != SIZEOF(*pComp)) && (pComp->dwSize != SIZEOF(IE4COMPONENT))))
        return E_INVALIDARG;

    // Catch folks that call our API's to add components and prevent them from doing
    // so if the restriction is in place.
    if (SHIsRestricted(NULL, REST_NOADDDESKCOMP))
        return E_ACCESSDENIED;

    if (dwFlags & DTI_ADDUI_POSITIONITEM)
        PositionComponent(&pComp->cpPos, pComp->iComponentType);
    //
    // BUGBUG -- WhichPlatform just looks back at shell32. which we are.
    // So maybe the answer is always the same.  -raymondc
    //

    //
    // Check if we are in INTEGRATED mode
    //
    if(WhichPlatform() != PLATFORM_INTEGRATED)
    {
        if (hwnd)
        {
            //Desktop channels can't be added in BrowserOnly mode!
            ShellMessageBox(MLGetHinst(), hwnd, MAKEINTRESOURCE(IDS_CANTADD_BROWSERONLY), 
                                MAKEINTRESOURCE(IDS_INTERNET_EXPLORER), MB_OK);
        }

        return E_FAIL;
    }

    // Check if the component already exists.
    BOOL fCompExists = FALSE;
    int cComp;
    GetDesktopItemCount(&cComp, 0);
    int i;
    COMPONENT comp;
    comp.dwSize = sizeof(COMPONENT);  //This needs to be initialized for ConvertCompStruc to work!
    COMPONENTA compA;
    TCHAR   szSource[INTERNET_MAX_URL_LENGTH];
    SHUnicodeToTChar(pComp->wszSource, szSource, ARRAYSIZE(szSource));

    for (i=0; i<cComp && !fCompExists; i++)
    {
        compA.dwSize = SIZEOF(compA);
        if(GetComponentPrivate(i, &compA)
                && lstrcmpi(szSource, compA.szSource) == 0)
        {
            fCompExists = TRUE;
            ConvertCompStruct((COMPONENTA *)&comp, &compA, FALSE);
            break;
        }
    }

    BOOL fAskToInstall;
    if(ZoneCheckUrlW(pComp->wszSource, URLACTION_SHELL_INSTALL_DTITEMS, (PUAF_NOUI), NULL) == S_OK)
    {
        fAskToInstall = TRUE;
    }
    else
    {
        fAskToInstall = FALSE;
    }

    if (S_OK != ZoneCheckUrlW(pComp->wszSource, URLACTION_SHELL_INSTALL_DTITEMS, (hwnd ? (PUAF_FORCEUI_FOREGROUND | PUAF_WARN_IF_DENIED) : PUAF_NOUI), NULL))
        return E_ACCESSDENIED;

    BOOL fCompSubDeleted = FALSE;
    SUBSCRIPTIONINFO si = {sizeof(SUBSCRIPTIONINFO)};
    // si.bstrUserName = NULL;
    // si.bstrPassword = NULL;
    // si.bstrFriendlyName = NULL;
    //
    // Confirmation dialog.
    //
    if(hwnd)
    {
        if(fCompExists)
        {
            //Prompt the user to delete the existing ADI.
            // This is a long string. So,...
            TCHAR szMsg[512];
            TCHAR szMsg2[256];
            TCHAR szTitle[128];
            MLLoadString(IDS_COMP_EXISTS, szMsg, ARRAYSIZE(szMsg));
            MLLoadString(IDS_COMP_EXISTS_2, szMsg2, ARRAYSIZE(szMsg2));
            lstrcat(szMsg, szMsg2);
            MLLoadString(IDS_COMP_TITLE, szTitle, ARRAYSIZE(szTitle));
            MessageBox(hwnd, szMsg, szTitle, MB_OK);

            return E_FAIL;

#if 0

            comp.dwSize = SIZEOF(comp);

            //Prompt the user to reinstall the ADI.
            if(ShellMessageBox(MLGetHinst(), hwnd, MAKEINTRESOURCE(IDS_CONFIRM_ADI_REINSTALL), 
                MAKEINTRESOURCE(IDS_COMP_TITLE), MB_YESNO) != IDYES)
            {
                return E_FAIL; //User choses not to install this desktop component!
            }
            else
            {
                ISubscriptionMgr *psm;
                if (SUCCEEDED(hres = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                        CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr, (void**)&psm)))
                {
                    si.cbSize = sizeof(si);
                    si.fUpdateFlags = SUBSINFO_ALLFLAGS;
                    //Backup and remove the subscription also
                    hres = psm->GetSubscriptionInfo(comp.wszSubscribedURL, &si);
                    if(SUCCEEDED(hres))
                    {
                        hres = RemoveDesktopItem(&comp, 0);
                        if(SUCCEEDED(hres))
                        {
                            psm->DeleteSubscription(comp.wszSubscribedURL, NULL);
                            ApplyChanges(AD_APPLY_SAVE);
                            fCompSubDeleted = TRUE;
                            // Set the new component to be enabled
                            pComp->fChecked = TRUE;
                        }
                    }
                    psm->Release();
                }
                else
                {
                    TraceMsg(TF_WARNING, "CActiveDesktop::AddDesktopItemWithUI : CoCreateInstance for CLSID_SubscriptionMgr failed.");
                }
            }
#endif
        }
        else if(fAskToInstall)
        {
            if(ShellMessageBox(MLGetHinst(), hwnd, MAKEINTRESOURCE(IDS_CONFIRM_ADD), 
                                MAKEINTRESOURCE(IDS_INTERNET_EXPLORER), MB_YESNO) != IDYES)
            {
                return E_FAIL; //User choses not to install this desktop component!
            }
        }
    }

    if(hwnd)
    {
        //If the active desktop is currently OFF, prompt the user to turn it on.
        SHELLSTATE ss = {0};
        SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE);
        if(!ss.fDesktopHTML)
        {
            if(ShellMessageBox(MLGetHinst(), hwnd, MAKEINTRESOURCE(IDS_CONFIRM_AD_ON), 
                            MAKEINTRESOURCE(IDS_INTERNET_EXPLORER), MB_YESNO) != IDNO)
            {
                //The end-user agreed to turn ON the active desktop.
                COMPONENTSOPT co;
                co.dwSize = sizeof(COMPONENTSOPT);
                GetDesktopItemOptions(&co, 0);
                co.fActiveDesktop = TRUE;
                SetDesktopItemOptions(&co, 0);
                ApplyChanges(AD_APPLY_REFRESH);
            }
        }
    }

    hres = SubscribeToCDF(hwnd, pComp->wszSubscribedURL, STC_DESKTOPCOMPONENT);
    switch(hres)
    {
        case E_INVALIDARG:
        {
            // E_UNEXPECTED is returned from SubscribeToCDFUrlA() when the URL doesn't point to
            // a CDF file, so we assume it's a web page.

            hres = _AddDTIWithUIPrivateA(hwnd, pComp, dwFlags);
            if(hres != S_OK && fCompSubDeleted)    // Restore the old component
            {
                hres = AddDesktopItem(&comp, 0);
                if(SUCCEEDED(hres))
                {
                    ISubscriptionMgr *psm;
                    if (SUCCEEDED(CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                            CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr, (void**)&psm)))
                    {
                        si.cbSize = sizeof(si);
                        psm->CreateSubscription(hwnd, comp.wszSubscribedURL, si.bstrFriendlyName, CREATESUBS_NOUI, SUBSTYPE_DESKTOPURL, &si);
                        psm->Release();
                    }
                    else
                    {
                        TraceMsg(TF_WARNING, "CActiveDesktop::AddDesktopItemWithUI : CoCreateInstance for CLSID_SubscriptionMgr failed.");
                    }
                }
            }
            ApplyChanges(AD_APPLY_ALL);
        }
        break;

        case E_ACCESSDENIED:
            // The file was a CDF but didn't contain Desktop Component Information
            if (hwnd)
            {
                TCHAR szMsg[MAX_PATH];
                TCHAR szTitle[MAX_PATH];

                MLLoadString(IDS_ADDCOMP_ERROR_CDFNODTI, szMsg, ARRAYSIZE(szMsg));
                MLLoadString(IDS_INTERNET_EXPLORER, szTitle, ARRAYSIZE(szTitle));
                MessageBox(hwnd, szMsg, szTitle, MB_OK);
            }
            break;
        case E_UNEXPECTED:      
            // This was a CDF but it was misauthored.
            if (hwnd)
            {
                TCHAR szMsg[MAX_PATH];
                TCHAR szTitle[MAX_PATH];

                MLLoadString(IDS_ADDCOMP_ERROR_CDFINALID, szMsg, ARRAYSIZE(szMsg));
                MLLoadString(IDS_INTERNET_EXPLORER, szTitle, ARRAYSIZE(szTitle));
                MessageBox(hwnd, szMsg, szTitle, MB_OK);
            }
            break;
        default:
            break;
    }

    if(fCompSubDeleted)
    {
        if(si.bstrUserName)
        {
            SysFreeString(si.bstrUserName);
        }
        if(si.bstrPassword)
        {
            SysFreeString(si.bstrPassword);
        }
        if(si.bstrFriendlyName)
        {
            SysFreeString(si.bstrFriendlyName);
        }
    }
    return hres;
}


HRESULT THISCLASS::AddDesktopItem(LPCCOMPONENT pComp, DWORD dwReserved)
{
    HRESULT     hres = E_FAIL;
    COMPONENTA  CompA;
    CompA.dwSize = SIZEOF(CompA);

    ASSERT(!dwReserved);     // These should be 0

    // We need to support IE4 apps calling with the old component structure too!
    // We use the size field to detect IE4 v/s newer apps!
    if(!pComp || ((pComp->dwSize != SIZEOF(*pComp)) && (pComp->dwSize != SIZEOF(IE4COMPONENT))))
        return E_INVALIDARG;

    // Catch folks that call our API's to add components and prevent them from doing
    // so if the restriction is in place.
    if (SHIsRestricted(NULL, REST_NOADDDESKCOMP))
        return E_ACCESSDENIED;

    // Convert the external structure to the internal format
    ConvertCompStruct(&CompA, (COMPONENTA *)pComp, TRUE);
 
    // If the component is already present, then fail the call!
    if(_FindComponentBySource(CompA.szSource, &CompA) > -1) 
        return hres;

    //Make sure that the COMPPOS size field is set before we add it!
    CompA.cpPos.dwSize = sizeof(COMPPOS);

    PositionComponent(&CompA.cpPos, CompA.iComponentType);

    //Make sure the this component's fDirty flag is off.
    CompA.fDirty = FALSE;

    if(AddComponentPrivate(&CompA, _dwNextID++))
    {
        // It might be cheaper to attempt to insert the component in the
        // correct z-order but it's less code to just call _SortAndRationalize
        // after the insertion is done.
        _SortAndRationalize();
        hres = S_OK;
    }

    return(hres);
}

BOOL THISCLASS::AddComponentPrivate(COMPONENTA *pcomp, DWORD dwID)
{
    BOOL fRet = FALSE;
    ENTERPROC(1, "DS AddComponent(pcomp=%08X)", pcomp);

    if (pcomp)
    {
        if (_hdsaComponent == NULL)
        {
            _hdsaComponent = DSA_Create(SIZEOF(COMPONENTA), DXA_GROWTH_CONST);
        }

        if (_hdsaComponent)
        {
            pcomp->dwID = dwID;

            if (DSA_AppendItem(_hdsaComponent, pcomp) != -1)
            {
                _fDirty = TRUE;
                fRet = TRUE;
            }
            else
            {
                TraceMsg(TF_WARNING, "DS AddComponent unable to append DSA");
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "DS AddComponent unable to create DSA");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "DS AddComponent unable to add a component");
    }

    EXITPROC(1, "DS AddComponent=%d", fRet);
    return fRet;
}

//
// This finds out if a given component already exists by comparing the szSource
// If so, it fills out the correct dwID and returns the index.
//
int THISCLASS::_FindComponentBySource(LPTSTR lpszSource, COMPONENTA *pComp)
{
    int iRet = -1;
    ENTERPROC(2, "DS FindComponentIdBySource(pComp=%8X)", pComp);

    if (_hdsaComponent)
    {
        int i;

        for (i=0; i<DSA_GetItemCount(_hdsaComponent); i++)
        {
            COMPONENTA comp;
            comp.dwSize = sizeof(COMPONENTA);

            if (DSA_GetItem(_hdsaComponent, i, &comp) != -1)
            {
                if (!lstrcmpi(comp.szSource, lpszSource))
                {
                    *pComp = comp;
                    iRet = i;
                    break;
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "DS FindComponentIndexByID unable to get a component");
            }
        }
    }

    EXITPROC(2, "DS FindComponentIdBySource=%d", iRet);
    return iRet;
}

int THISCLASS::_FindComponentIndexByID(DWORD dwID)
{
    int iRet = -1;
    ENTERPROC(2, "DS FindComponentIndexByID(dwID=%d)", dwID);

    if (_hdsaComponent)
    {
        int i;

        for (i=0; i<DSA_GetItemCount(_hdsaComponent); i++)
        {
            COMPONENTA comp;
            comp.dwSize = sizeof(COMPONENTA);

            if (DSA_GetItem(_hdsaComponent, i, &comp) != -1)
            {
                if (comp.dwID == dwID)
                {
                    iRet = i;
                    break;
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "DS FindComponentIndexByID unable to get a component");
            }
        }
    }

    EXITPROC(2, "DS FindComponent=%d", iRet);
    return iRet;
}

HRESULT THISCLASS:: GetDesktopItemByID(DWORD dwID, COMPONENT *pcomp, DWORD dwReserved)
{
    HRESULT hres = E_FAIL;
    ENTERPROC(1, "DS GetComponentByID(dwID=%d,pcomp=%08X)", dwID, pcomp);
    COMPONENTA  CompA;

    ASSERT(!dwReserved);     // These should be 0

    // We need to support IE4 apps calling with the old component structure too!
    // We use the size field to detect IE4 v/s newer apps!
    if(!pcomp || ((pcomp->dwSize != SIZEOF(*pcomp)) && (pcomp->dwSize != SIZEOF(IE4COMPONENT))))
        return E_INVALIDARG;

    if (pcomp)
    {
        int index = _FindComponentIndexByID(dwID);
        if (index != -1)
        {
            if (DSA_GetItem(_hdsaComponent, index, &CompA) != -1)
            {
                hres = S_OK;
            }
            else
            {
                TraceMsg(TF_WARNING, "DS GetComponentByID unable to get component");
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "DS GetComponentByID unable to find component");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "DS GetComponentByID given NULL pcomp");
    }

    if(SUCCEEDED(hres))
    {
        MultiCompToWideComp(&CompA, pcomp);
    }

    EXITPROC(1, "DS GetComponentByID=%d", hres);
    return hres;
}

HRESULT THISCLASS::RemoveDesktopItem(LPCCOMPONENT pComp, DWORD dwReserved)
{
    COMPONENTA  CompA, CompToDelete;
    int         iIndex;
    HRESULT     hres = E_FAIL;

    ASSERT(!dwReserved);     // These should be 0

    // We need to support IE4 apps calling with the old component structure too!
    // We use the size field to detect IE4 v/s newer apps!
    if(!pComp || ((pComp->dwSize != SIZEOF(*pComp)) && (pComp->dwSize != SIZEOF(IE4COMPONENT))))
        return E_INVALIDARG;

    CompA.dwSize = SIZEOF(CompA);
    CompToDelete.dwSize = SIZEOF(CompToDelete);

    //Convert the struct to internal struct.
    ConvertCompStruct(&CompA, (COMPONENTA *)pComp, TRUE);

    // See if the component already exists.
    iIndex = _FindComponentBySource(CompA.szSource, &CompToDelete);

    if(iIndex > -1)
    {
        if(RemoveComponentPrivate(iIndex, &CompToDelete))
        {
            hres = S_OK;
        }
    }

    return(hres);
}

BOOL THISCLASS::RemoveComponentPrivate(int iIndex, COMPONENTA *pcomp)
{
    BOOL fRet = FALSE;
    ENTERPROC(1, "DS RemoveComponent(pcomp=%08X)", pcomp);

    if (_hdsaComponent)
    {
        if(iIndex == -1)
            iIndex = _FindComponentIndexByID(pcomp->dwID);
        if (iIndex != -1)
        {
            if (DSA_DeleteItem(_hdsaComponent, iIndex) != -1)
            {
                _fDirty = TRUE;
                fRet = TRUE;
            }
            else
            {
                TraceMsg(TF_WARNING, "DS RemoveComponent could not remove an item");
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "DS RemoveComponent could not find item to remove");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "DS RemoveComponent has no components to remove");
    }

    EXITPROC(1, "DS RemoveComponent=%d", fRet);
    return fRet;
}

HRESULT THISCLASS::_CopyComponent(COMPONENTA *pCompDest, COMPONENTA *pCompSrc, DWORD dwFlags)
{
    //Copy only those elements mentioned in the flag!

//    if(dwFlags & COMP_ELEM_ID)
//        pCompDest->dwID = pCompSrc->dwID;
    if(dwFlags & COMP_ELEM_TYPE)
        pCompDest-> iComponentType = pCompSrc->iComponentType;
    if(dwFlags & COMP_ELEM_CHECKED)
        pCompDest-> fChecked = pCompSrc->fChecked;
    if(dwFlags & COMP_ELEM_DIRTY)
        pCompDest-> fDirty = pCompSrc-> fDirty;
    if(dwFlags & COMP_ELEM_NOSCROLL)
        pCompDest-> fNoScroll = pCompSrc-> fNoScroll;
    if(dwFlags & COMP_ELEM_POS_LEFT)
        pCompDest-> cpPos.iLeft= pCompSrc->cpPos.iLeft;
    if(dwFlags & COMP_ELEM_POS_TOP)
        pCompDest-> cpPos.iTop= pCompSrc->cpPos.iTop;
    if(dwFlags & COMP_ELEM_SIZE_WIDTH)
        pCompDest-> cpPos.dwWidth= pCompSrc->cpPos.dwWidth;
    if(dwFlags & COMP_ELEM_SIZE_HEIGHT)
        pCompDest-> cpPos.dwHeight= pCompSrc->cpPos.dwHeight;
    if(dwFlags & COMP_ELEM_POS_ZINDEX)
        pCompDest-> cpPos.izIndex= pCompSrc->cpPos.izIndex;
    if(dwFlags & COMP_ELEM_SOURCE)
        lstrcpy(pCompDest->szSource, pCompSrc->szSource);
    if(dwFlags & COMP_ELEM_FRIENDLYNAME)
        lstrcpy(pCompDest->szFriendlyName, pCompSrc->szFriendlyName);
    if(dwFlags & COMP_ELEM_SUBSCRIBEDURL)
        lstrcpy(pCompDest->szSubscribedURL, pCompSrc->szSubscribedURL);

    return(S_OK);
}

HRESULT THISCLASS::GetDesktopItemBySource(LPCWSTR lpcwszSource, LPCOMPONENT pComp, DWORD dwFlags)
{
    COMPONENTA CompNew; 
    HRESULT   hres = E_FAIL;
    int       iIndex;

    //Passing a NULL to SHUnicodeToTChar causes a fault. So, let's fail it.
    if(lpcwszSource == NULL)
        return E_INVALIDARG;
        
    // We need to support IE4 apps calling with the old component structure too!
    // We use the size field to detect IE4 v/s newer apps!
    if(!pComp || ((pComp->dwSize != SIZEOF(*pComp)) && (pComp->dwSize != SIZEOF(IE4COMPONENT))))
        return E_INVALIDARG;

    CompNew.dwSize = sizeof(COMPONENTA);

    SHUnicodeToTChar(lpcwszSource, CompNew.szSource, ARRAYSIZE(CompNew.szSource));

    iIndex = _FindComponentBySource(CompNew.szSource, &CompNew);

    if(iIndex > -1)
    {
        MultiCompToWideComp(&CompNew, pComp);
        hres = S_OK;
    }

    return(hres);
}

HRESULT THISCLASS::ModifyDesktopItem(LPCCOMPONENT pComp, DWORD dwFlags)
{
    COMPONENTA  CompA, CompNew;
    HRESULT     hres = E_FAIL;
    int         iIndex = -1;

    // We need to support IE4 apps calling with the old component structure too!
    // We use the size field to detect IE4 v/s newer apps!
    if(!pComp || ((pComp->dwSize != SIZEOF(*pComp)) && (pComp->dwSize != SIZEOF(IE4COMPONENT))))
        return E_INVALIDARG;

    CompA.dwSize = sizeof(COMPONENTA);
    CompNew.dwSize = sizeof(COMPONENTA);

    //Convert public param structure to private param structure.
    ConvertCompStruct(&CompA, (COMPONENTA *)pComp, TRUE);

    //See if this component already exists.
    iIndex = _FindComponentBySource(CompA.szSource, &CompNew);
    if(iIndex > -1)
    {
        _CopyComponent(&CompNew, &CompA, dwFlags);
        if (dwFlags & (COMP_ELEM_POS_LEFT | COMP_ELEM_POS_TOP | COMP_ELEM_SIZE_WIDTH | COMP_ELEM_SIZE_HEIGHT))
            PositionComponent(&CompNew.cpPos, CompNew.iComponentType);
        CompNew.fDirty = TRUE; //Since the component is modified, we set the dirty bit!
        if(UpdateComponentPrivate(iIndex, &CompNew))
            hres = S_OK;
    }

    return(hres);
}

BOOL THISCLASS::UpdateComponentPrivate(int iIndex, COMPONENTA *pcomp)
{
    BOOL fRet = FALSE;
    ENTERPROC(1, "DS UpdateComponentPrivate(pcomp=%08X)", pcomp);

    if (_hdsaComponent)
    {
        if(iIndex == -1)
            iIndex = _FindComponentIndexByID(pcomp->dwID);

        if (iIndex != -1)
        {
            if (DSA_SetItem(_hdsaComponent, iIndex, pcomp) != -1)
            {
                _fDirty = TRUE;
                fRet = TRUE;
            }
            else
            {
                TraceMsg(TF_WARNING, "DS UpdateComponent could not update an item");
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "DS UpdateComponent could not find item to update");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "DS UpdateComponent has no components to update");
    }

    EXITPROC(1, "DS UpdateComponent=%d", fRet);
    return fRet;
}

HRESULT THISCLASS::GetDesktopItemCount(LPINT lpiCount, DWORD dwReserved)
{
    if(!lpiCount)
        return (E_INVALIDARG);

    *lpiCount = 0;

    ENTERPROC(1, "DS GetComponentsCount()");

    ASSERT(!dwReserved);     // These should be 0

    if (_hdsaComponent)
    {
        *lpiCount = DSA_GetItemCount(_hdsaComponent);
    }

    EXITPROC(1, "DS GetComponentsCount=%d", *lpiCount);
    return S_OK;
}

HRESULT THISCLASS::GetDesktopItem(int nComponent, COMPONENT *pComp, DWORD dwReserved)
{
    COMPONENTA  CompA;

    ASSERT(!dwReserved);     // These should be 0

    // We need to support IE4 apps calling with the old component structure too!
    // We use the size field to detect IE4 v/s newer apps!
    if((nComponent < 0) || !pComp || ((pComp->dwSize != SIZEOF(*pComp)) && (pComp->dwSize != SIZEOF(IE4COMPONENT))))
        return E_INVALIDARG;

    CompA.dwSize = sizeof(COMPONENTA);

    if(GetComponentPrivate(nComponent, &CompA))
    {
        //Convert the structure to the Public form.
        ConvertCompStruct((COMPONENTA *)pComp, &CompA, FALSE);
        return(S_OK);
    }
    else
        return(E_FAIL);
}

BOOL THISCLASS::GetComponentPrivate(int nComponent, COMPONENTA *pcomp)
{
    BOOL fRet = FALSE;
    ENTERPROC(1, "DS GetComponent(nComponent=%d,pcomp=%08X)", nComponent, pcomp);

    if (_hdsaComponent && pcomp && (nComponent < DSA_GetItemCount(_hdsaComponent)))
    {
        if (DSA_GetItem(_hdsaComponent, nComponent, pcomp) != -1)
        {
            fRet = TRUE;
        }
        else
        {
            TraceMsg(TF_WARNING, "DS GetComponent unable to get a component");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "DS GetComponent does not have a DSA");
    }

    EXITPROC(1, "DS GetComponent=%d", fRet);
    return fRet;
}

HRESULT CActiveDesktop::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    if(IsEqualIID(riid, IID_IActiveDesktop))
    {
        *ppvObj = (IActiveDesktop *)this;
        _Initialize();
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IActiveDesktop *)this;
    }
    else if (IsEqualIID(riid, IID_IActiveDesktopP))
    {
        *ppvObj = (IActiveDesktopP *)this;
    }
    else if (IsEqualIID(riid, IID_IADesktopP2))
    {
        *ppvObj = (IADesktopP2 *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

// Helper function so that it's easy to create one internally
// Actually, it's not ver much help any more...
STDAPI CActiveDesktop_InternalCreateInstance(LPUNKNOWN * ppunk, REFIID riid)
{
    return CActiveDesktop_CreateInstance(NULL, riid, (void **)ppunk);
}

// Our class factory create instance code
STDAPI CActiveDesktop_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    TraceMsg(TF_DESKSTAT, "CActiveDesktop- CreateInstance");

    CActiveDesktop *pad = new CActiveDesktop();

    if(pad)
    {
        HRESULT hres = pad->QueryInterface(riid, ppvOut);
        pad->Release();
        return hres;
    }

    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}


#ifdef DEBUG

//
// BUGBUG - Move g_dwDeskStatTrace into ccshell.ini to prevent recompiles.
//
DWORD g_dwDeskStatTrace = 2;
static DWORD g_dwIndent = 0;
static const TCHAR c_szDotDot[] = TEXT("..");

#define MAX_INDENTATION_VALUE   0x10

void EnterProcDS(DWORD dwTraceLevel, LPSTR pszFmt, ...)
{
    TCHAR szFmt[1000];
    TCHAR szOutput[1000];
    va_list arglist;

    SHAnsiToTChar(pszFmt, szFmt, ARRAYSIZE(szFmt));
    if (dwTraceLevel <= g_dwDeskStatTrace)
    {
        szOutput[0] = TEXT('\0');

        for (DWORD i=0; i<g_dwIndent; i++)
        {
            lstrcat(szOutput, c_szDotDot);
        }

        va_start(arglist, pszFmt);
        wvsprintf(szOutput + lstrlen(szOutput), szFmt, arglist);
        va_end(arglist);

        TraceMsg(TF_DESKSTAT, "%s", szOutput);

        // This value can get out of hand if EnterProc and ExitProc
        // calls do not match. This can trash the stack.
        if(g_dwIndent < MAX_INDENTATION_VALUE)
            g_dwIndent++;
    }
}

void ExitProcDS(DWORD dwTraceLevel, LPSTR pszFmt, ...)
{
    TCHAR szFmt[1000];
    TCHAR szOutput[1000];
    va_list arglist;

    SHAnsiToTChar(pszFmt, szFmt, ARRAYSIZE(szFmt));
    if (dwTraceLevel <= g_dwDeskStatTrace)
    {
        // This can happen if the EnterProc and 
        // ExitProc calls do not match.
        if(g_dwIndent > 0)
            g_dwIndent--;

        szOutput[0] = TEXT('\0');

        for (DWORD i=0; i<g_dwIndent; i++)
        {
            lstrcat(szOutput, c_szDotDot);
        }

        va_start(arglist, pszFmt);
        wvsprintf(szOutput + lstrlen(szOutput), szFmt, arglist);
        va_end(arglist);

        TraceMsg(TF_DESKSTAT, "%s", szOutput);
    }
}

#endif

/*************************************************************************
 *
 *  IActiveDesktopP methods and helper functions
 *
 *  IActiveDesktopP is a private interface used to implement helper
 *  functionality that is used internally by the various shell binaries.
 *
 *  Notes:
 *      Getting an interface to IActiveDesktopP does not initialize the state
 *  of the object such that member functions are able to call IActiveDesktop
 *  member functions.  This is so that it is a more lightweight implementation
 *  and also simplifies the implementation of SetScheme.  If a subsequent QI for
 *  IActiveDesktop is performed then it will initialize properly and any member
 *  function can then be called.
 *
 *************************************************************************/

//
// SetScheme
//
// Used to set the current scheme that the object will read and write to
// when it is initialized.  This method must be called before a subsequent
// QI to IActiveDesktop is made.
//
HRESULT THISCLASS::SetScheme(LPCWSTR pwszSchemeName, DWORD dwFlags)
{
    LPTSTR pszSchemeName, pszAlloc;
    int icch;

    // Can't set the local scheme after we've been initialized...we can fix this
    // later if necessary but for now it's simplest this way.
    if (_fInitialized && (dwFlags & SCHEME_LOCAL))
        return E_FAIL;

    // Sanity checks
    if (!pwszSchemeName || ((icch = lstrlenW(pwszSchemeName)) > MAX_PATH - 1))
        return E_INVALIDARG;

#ifndef UNICODE
    CHAR    szName[MAX_PATH];

    SHUnicodeToAnsi(pwszSchemeName, szName, ARRAYSIZE(szName));
    pszSchemeName = szName;
#else
    pszSchemeName = (LPTSTR)pwszSchemeName;
#endif

    if (dwFlags & SCHEME_CREATE)
    {
        HRESULT hres;
        HKEY hkey, hkey2;

        if (ERROR_SUCCESS == (hres = RegCreateKey(HKEY_CURRENT_USER, REG_DESKCOMP_SCHEME_LOCATION, &hkey)))
        {
            if (ERROR_SUCCESS == (hres = RegCreateKey(hkey, pszSchemeName, &hkey2)))
                RegCloseKey(hkey2);
            RegCloseKey(hkey);
        }
        if (FAILED(hres))
            return hres;
    }

    if (dwFlags & SCHEME_LOCAL)
    {
        // The local case is easy - just copy the string to our local variable,
        // it will be used when IActiveDesktop is initialized.
        if (!(pszAlloc = (LPTSTR)LocalAlloc(LPTR, (icch + 1) * sizeof(TCHAR))))
            return E_OUTOFMEMORY;

        if (_pszScheme)
            LocalFree((HANDLE)_pszScheme);

        _pszScheme = pszAlloc;

        lstrcpy(_pszScheme, pszSchemeName);
    }

    if (dwFlags & SCHEME_GLOBAL)
    {
        // Update the registry with the new global scheme value
        if (dwFlags & SCHEME_DISPLAY)
            SHSetValue(HKEY_CURRENT_USER, REG_DESKCOMP_SCHEME, REG_VAL_SCHEME_DISPLAY,
                        REG_SZ, (LPBYTE)pszSchemeName, (lstrlen(pszSchemeName) + 1) * sizeof(TCHAR));
        if (dwFlags & SCHEME_EDIT)
            SHSetValue(HKEY_CURRENT_USER, REG_DESKCOMP_SCHEME, REG_VAL_SCHEME_EDIT,
                        REG_SZ, (LPBYTE)pszSchemeName, (lstrlen(pszSchemeName) + 1) * sizeof(TCHAR));
    }

    if (dwFlags & (SCHEME_REFRESH | SCHEME_UPDATE))
    {
        DWORD dwUpdateFlags = AD_APPLY_FORCE | AD_APPLY_HTMLGEN | AD_APPLY_SAVE;
        if (dwFlags & SCHEME_REFRESH)
            dwUpdateFlags |= AD_APPLY_REFRESH;
        _Initialize();
        ApplyChanges(dwUpdateFlags);
    }

    return S_OK;
}


HRESULT GetGlobalScheme(LPWSTR pwszScheme, LPDWORD lpdwcchBuffer, DWORD dwFlags)
{
    DWORD dwType, dwcbBuffer;
    LONG lret;
    TCHAR szScheme[MAX_PATH];
    
    // Just pull it from the registry
    dwcbBuffer = sizeof(szScheme);

    if (ERROR_SUCCESS ==
        (lret = SHGetValue(HKEY_CURRENT_USER, REG_DESKCOMP_SCHEME,
                    (dwFlags & SCHEME_EDIT) ? REG_VAL_SCHEME_EDIT : REG_VAL_SCHEME_DISPLAY, &dwType,
                    (LPBYTE)szScheme, &dwcbBuffer)))
    {
        SHTCharToUnicode(szScheme, pwszScheme, *lpdwcchBuffer);
        // Fortunately, even Win95 implements lstrlenW
        *lpdwcchBuffer = lstrlenW(pwszScheme);
    }

    return (lret == ERROR_SUCCESS ? S_OK : E_FAIL);
}


//
// GetScheme
//
//
HRESULT THISCLASS::GetScheme(LPWSTR pwszSchemeName, LPDWORD lpdwcchBuffer, DWORD dwFlags)
{
    // Sanity checks
    if (!pwszSchemeName || *lpdwcchBuffer == 0)
        return E_INVALIDARG;

    if (dwFlags & SCHEME_LOCAL)
    {
        if (!_pszScheme)
        {
            HRESULT hres;
            // Special case if no local scheme has explicitly been selected yet.
            // The default scheme is the global display scheme in this case.
            if (SUCCEEDED(hres = GetGlobalScheme(pwszSchemeName, lpdwcchBuffer, SCHEME_DISPLAY)))
            {
                hres = SetScheme(pwszSchemeName, SCHEME_LOCAL);
            }
            return hres;
        }

#ifdef UNICODE
        lstrcpyn(pwszSchemeName, _pszScheme, *lpdwcchBuffer);
#else
        SHAnsiToUnicode(_pszScheme, pwszSchemeName, *lpdwcchBuffer);
#endif
        *lpdwcchBuffer = lstrlenW(pwszSchemeName);
        return S_OK;
    }


    if (dwFlags & SCHEME_GLOBAL)
    {
        return GetGlobalScheme(pwszSchemeName, lpdwcchBuffer, dwFlags);
    }

    return E_INVALIDARG;
}

BOOL UpdateAllDesktopSubscriptions();
HRESULT THISCLASS::UpdateAllDesktopSubscriptions()
{
    ::UpdateAllDesktopSubscriptions();
    return S_OK;
}

//
// This function takes a pointer to the ActiveDesktop's ole obj, reads all the changes to be done
// from the registry and makes those changes to the various elements through dynamic HTML interfaces.
// NOTE: This function is implemented in NT5 version of shell32.dll. Since shdoc401.dll and 
// NT5's shell32.dll share the same IDL, I have to add the following dummy implementation here.
HRESULT CActiveDesktop::MakeDynamicChanges(IOleObject *pOleObj)
{
    return(E_NOTIMPL);
}

//
// SetSafeMode
//
// Either puts the active desktop in safemode or restores it to the previous
// scheme before safemode was entered.
//
HRESULT THISCLASS::SetSafeMode(DWORD dwFlags)
{
    //
    // Make sure we are in active desktop mode.
    //
    SHELLSTATE ss = {0};
    BOOL fSetSafeMode = (dwFlags & SSM_SET) != 0;

    SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE);
    if (ss.fDesktopHTML)
    {
        //
        // All we need to do is switch the "display" scheme to "safemode" in order to
        // go into safemode.  To go out, we just switch the "display" scheme back to the
        // previous "edit" scheme.
        //
        WCHAR wszEdit[MAX_PATH];
        WCHAR wszDisplay[MAX_PATH];
        DWORD dwcch = MAX_PATH;

        if (SUCCEEDED(GetScheme(wszEdit, &dwcch, SCHEME_GLOBAL | SCHEME_EDIT)))
        {
            dwcch = MAX_PATH;
            if (SUCCEEDED(GetScheme(wszDisplay, &dwcch, SCHEME_GLOBAL | SCHEME_DISPLAY)))
            {
                BOOL fInSafeMode = (StrCmpW(wszDisplay, REG_DESKCOMP_SAFEMODE_SUFFIX_L) == 0);

                if (fSetSafeMode != fInSafeMode)
                {
                    LPWSTR lpwstr;
                    DWORD dwSchemeFlags = SCHEME_GLOBAL | SCHEME_DISPLAY;
                    if (dwFlags & SSM_REFRESH)
                        dwSchemeFlags |= SCHEME_REFRESH;
                    if (dwFlags & SSM_UPDATE)
                        dwSchemeFlags |= SCHEME_UPDATE;

                    lpwstr = fSetSafeMode ? REG_DESKCOMP_SAFEMODE_SUFFIX_L : wszEdit;

                    SetScheme(lpwstr, dwSchemeFlags);
                }
            }
        }
    }
    return S_OK;
}

//
// EnsureUpdateHTML
//
// Ensures that the current html file present on the disk is in sync
// with the registry information for the current active desktop scheme.  If
// it is not in sync then a fresh copy of the file is generated from the
// registry for the current scheme.
//
HRESULT THISCLASS::EnsureUpdateHTML(void)
{
    DWORD dwFlags = 0;
    DWORD dwDataLength = sizeof(DWORD);
    DWORD dwType;
    LONG lRet;
    TCHAR lpszDeskcomp[MAX_PATH];
    DWORD dwRestrictUpdate;
    DWORD dwRestrict = SHRestricted2W(REST_NoChannelUI, NULL, 0);
    DWORD dwSize = SIZEOF(dwRestrictUpdate);
    BOOL  fComponentsDirty = FALSE;  //Assume that the components are NOT dirty!
    DWORD dwVersion;
    DWORD dwMinorVersion;
    BOOL  fStaleInfoInReg = FALSE;

    if (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, REG_DESKCOMP_COMPONENTS_ROOT, REG_VAL_GENERAL_RESTRICTUPDATE, NULL, (LPVOID) &dwRestrictUpdate, &dwSize))
        dwRestrictUpdate = 0;

    GetRegLocation(lpszDeskcomp, REG_DESKCOMP_COMPONENTS, NULL);

    //See if this branch of registry is old
    if ((lRet = SHGetValue(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, REG_VAL_COMP_VERSION, &dwType,
                            (LPBYTE)(&dwVersion), &dwDataLength)) == ERROR_SUCCESS)
    {
        if(dwVersion < CUR_DESKHTML_VERSION)
            fStaleInfoInReg = TRUE;
        else
        {
            //Major versions are equal. Check minor versions.
            if ((lRet = SHGetValue(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, REG_VAL_COMP_MINOR_VERSION, &dwType,
                                    (LPBYTE)(&dwMinorVersion), &dwDataLength)) == ERROR_SUCCESS)
            {
                if(dwMinorVersion != CUR_DESKHTML_MINOR_VERSION)
                    fStaleInfoInReg = TRUE;
            }
            else
                fStaleInfoInReg = TRUE;
        }
    }
    else
        fStaleInfoInReg = TRUE;

    dwDataLength = SIZEOF(DWORD);

    //Check the dirty bit to see if we need to re-generate the desktop html
    if ((lRet = SHGetValue(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, REG_VAL_COMP_GENFLAGS, &dwType,
                            (LPBYTE)(&dwFlags), &dwDataLength)) == ERROR_SUCCESS)
    {
        if (IsFlagSet(dwFlags, COMPONENTS_DIRTY))
            fComponentsDirty = TRUE;
    }

    TCHAR   szDesktopFile[MAX_PATH];

    GetPerUserFileName(szDesktopFile, ARRAYSIZE(szDesktopFile), DESKTOPHTML_FILENAME);

    if (fComponentsDirty ||
         fStaleInfoInReg ||
         (dwRestrictUpdate != dwRestrict) ||
         (!PathFileExists(szDesktopFile)))  //See if the file exists!
    {
        _Initialize();
        // NOTE #1: The above initialization would have changed the Z-order because of
        // SortAndRationalize and so we need to APPLY_SAVE here.
        // Warning: APPLY_SAVE changes the dwID field of components. This should not
        // be a problem because we do this just before generating a new HTML file.
        // NOTE #2: Do NOT use AD_APPLY_FORCE here. That sets the _fPatternDirty too and
        // that causes a SystemParametersInfo() call which results in WM_SYSCOLORCHANGE
        // and this causes a refresh. So, we set the dirty bit explicitly here.

        _fDirty = TRUE;  // See Note#2 above.

        ApplyChanges(AD_APPLY_SAVE | AD_APPLY_HTMLGEN);
        lRet = ERROR_SUCCESS;
        if (dwRestrictUpdate != dwRestrict)
            SHSetValue(HKEY_CURRENT_USER, REG_DESKCOMP_COMPONENTS_ROOT, REG_VAL_GENERAL_RESTRICTUPDATE, NULL, (LPVOID) &dwRestrict, SIZEOF(dwRestrict));
    }

    return (lRet == ERROR_SUCCESS ? S_OK : E_FAIL);
}

//
//  ReReadWallpaper()
//      If the wallpaper was read when the active desktop was disabled, we would have read it from
//  the old location. Now, if the active desktop is turned ON, then we need to re-read the wallpaper
//  from the new location. We need to do this iff the wallpaper has not been changed in the mean-while
//
HRESULT THISCLASS::ReReadWallpaper(void)
{
    if((!_fDirty) || (!_co.fActiveDesktop))  //If nothing has changed OR if active desktop is OFF, 
        return(S_FALSE);                        // then nothing to do!

    //ActiveDesktop is ON in our object. Read current shell state.
    SHELLSTATE ss = {0};
    
    SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE);
    if (ss.fDesktopHTML)
        return(S_FALSE);        // Active Desktop state hasn't changed. So, nothing to do!

    //So, Active desktop was originally OFF and now it is turned ON.
    //If if someone changed the wallpaper, we should not mess with it.
    if(_fWallpaperDirty || _fWallpaperChangedDuringInit)
        return(S_FALSE); 

    // No one has changed the wallpaper. So, we must re-read it from the new wallpaper location
    // sothat we get the correct wallpaper for the active desktop mode.
    _ReadWallpaper(TRUE);

    return(S_OK);
}

//
//  GetADObjectFlags()
//
//      Get the Active Desktop object's internal flags
//
HRESULT THISCLASS::GetADObjectFlags(LPDWORD lpdwFlags, DWORD dwMask)
{
    ASSERT(lpdwFlags);
    
    *lpdwFlags = 0; //Init the flags
    
    if((dwMask & GADOF_DIRTY) && _fDirty)
        *lpdwFlags |= GADOF_DIRTY;

    return(S_OK);
}

#endif
