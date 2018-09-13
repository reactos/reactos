#include "stdafx.h"
#pragma hdrstop

#ifdef _WIN64
extern "C" char * __cdecl StrTokEx(char ** pstring, const char * control);
#else
#include <iert.h>
#endif

#define DXA_GROWTH_CONST 10
#define ZINDEX_START 1000

#define MAXID_LENGTH    10  //Maximum number of digits in ID string plus 1.

#if 0
#define TF_DESKSTAT     TF_CUSTOM2
#define TF_DYNAMICHTML  TF_CUSTOM1
#else
#define TF_DESKSTAT     0
#define TF_DYNAMICHTML  0
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
#else // ccover buildi
// these are needed because of a bug in cl.exe that results in improper processing
// of #pragma when run with cl -P, and then compiling
#define ENTERPROC 1 ? (void) 0 : (void)
#define EXITPROC 1 ? (void) 0 : (void)
#endif //end of ccover 
#endif

MAKE_CONST_BSTR(s_sstrBeforeEnd,       L"BeforeEnd");
MAKE_CONST_BSTR(s_sstrDeskMovr,        L"DeskMovr");
MAKE_CONST_BSTR(s_sstrDeskMovrW,       L"DeskMovrW");
MAKE_CONST_BSTR(s_sstrclassid,         L"classid");
MAKE_CONST_BSTR(s_sstrEmpty,           L"");


STDAPI ParseDesktopComponent(HWND hwndOwner, LPWSTR wszURL, COMPONENT *pInfo);

WCHAR   wUnicodeBOM =  0xfeff; // Little endian unicode Byte Order Mark.First byte:0xff, Second byte: 0xfe.


CReadFileObj::CReadFileObj(LPCTSTR lpszFileName)
{
    //Open the file 
    if((_hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, 
                            NULL, OPEN_EXISTING, 
                            FILE_ATTRIBUTE_NORMAL, 0)) != INVALID_HANDLE_VALUE)
    {
        WCHAR  wBOM;
        DWORD  dwBytesRead = 0;
        
        if((ReadFile(_hFile, (LPVOID)&wBOM, sizeof(WCHAR), &dwBytesRead, NULL)) && 
           (dwBytesRead == sizeof(WCHAR)))
        {
            if(wBOM == wUnicodeBOM)
                _iCharset = UNICODE_HTML_CHARSET;
            else
            {
                //Note: Anything other than the little endian unicode file is treated as ansi.
                _iCharset = ANSI_HTML_CHARSET;
                SetFilePointer(_hFile, 0L, NULL, FILE_BEGIN);  //Seek to the begining of the file
            }
        }
    }
}

CReadFileObj::~CReadFileObj()
{
    if(_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hFile);
        _hFile = NULL;
    }
}

//
// This will read and if necessary convert between ANSI and UNICODE
//
//
// NOTE: The uiCharsToRead must be atleast one less than the size of the buffer (lpwszBuff)
// because one null may be written at the end of the buffer by SHAnsiToUnicode().
//

HRESULT CReadFileObj::FileReadAndConvertChars(int iDestCharset, LPWSTR lpwszBuff, UINT uiCharsToRead, UINT *puiCharsActuallyRead, UINT *puiCharsConverted)
{
    HRESULT hres = S_OK;
    DWORD dwCharsRead = 0;
    DWORD dwTotalCharsConverted = 0;
    if(_hFile != INVALID_HANDLE_VALUE)
    {
        if(_iCharset == UNICODE_HTML_CHARSET)
        {
            if(iDestCharset == UNICODE_HTML_CHARSET)
            {
                hres = FileReadCharsW(lpwszBuff, uiCharsToRead, (UINT *)&dwCharsRead);
                dwTotalCharsConverted = dwCharsRead;
            }
            else
            {
                //Destination is ansi; Read the UNICODE source and convert to ANSI.
                WCHAR  wszBuf[INTERNET_MAX_URL_LENGTH + 1];  //Temp buffer to read the UNICODE chars into.
                LPSTR lpszBuff = (LPSTR)lpwszBuff;

                DWORD  dwTotalCharsToRead = (DWORD)uiCharsToRead;
    
                while(dwTotalCharsToRead)
                {
                    DWORD dwCount;
                    DWORD dwActuallyRead;
                    
                    // - 1 to give room for a null character at the end.
                    dwCount = min(dwTotalCharsToRead, (ARRAYSIZE(wszBuf) - 1));
                    if(ReadFile(_hFile, (LPSTR)wszBuf, dwCount*sizeof(WCHAR), &dwActuallyRead, NULL))
                    {
                        DWORD dwConverted;
                        dwActuallyRead = dwActuallyRead/sizeof(WCHAR);
                        
                        //Null terminate the source buffer.
                        wszBuf[dwActuallyRead] = L'\0';  //UNICODE null terminate the source.
                        //Convert what we just read.
                        dwConverted = SHUnicodeToAnsi(wszBuf, lpszBuff, dwActuallyRead+1); //+1 for null termination

                        //Update the count & stuff.
                        lpszBuff += dwConverted - 1;  //Subtract the null.
                        dwTotalCharsToRead -= dwActuallyRead;
                        dwCharsRead += dwActuallyRead;
                        dwTotalCharsConverted += dwConverted - 1; //Subtract the null.
                    
                        if(dwActuallyRead < dwCount)
                            break;  //We have reached the end of file.
                    }
                    else
                    {
                        hres = E_FAIL;
                        break;
                    }
                }
            }
        }
        else
        {
            //Source file is ANSI. Check the Destination.
            if(iDestCharset == ANSI_HTML_CHARSET)
            {
                //Destination is ANSI too! Cool! No need for conversion!
                hres = FileReadCharsA((LPSTR)lpwszBuff, uiCharsToRead, (UINT *)&dwCharsRead);
                dwTotalCharsConverted = dwCharsRead;
            }
            else
            {
                //Destination is UNICODE!  Read the ansi and convert it to UNICODE!
                char  szBuf[INTERNET_MAX_URL_LENGTH + 1];  //Temp buffer to read the ansi chars into.
                DWORD  dwTotalCharsToRead = (DWORD)uiCharsToRead;

                while(dwTotalCharsToRead)
                {
                    DWORD dwCount;
                    DWORD dwActuallyRead;

                    // - 1 to give room for a null character at the end.
                    dwCount = min(dwTotalCharsToRead, (ARRAYSIZE(szBuf) - 1));

                    if(ReadFile(_hFile, (LPSTR)szBuf, dwCount, &dwActuallyRead, NULL))
                    {
                        DWORD dwConverted;
                        //Null terminate the source buffer.
                        szBuf[dwActuallyRead] = '\0';  //ANSI null terminate the source.
                        //Convert what we just read.
                        dwConverted = SHAnsiToUnicode(szBuf, lpwszBuff, dwActuallyRead+1); //+1 for null termination

                        //Update the count & stuff.
                        lpwszBuff += dwConverted - 1;  //Subtract the null.
                        dwTotalCharsToRead -= dwActuallyRead;
                        dwCharsRead += dwActuallyRead;
                        dwTotalCharsConverted += dwConverted - 1; //Subtract the null.
                    
                        if(dwActuallyRead < dwCount)
                            break;  //We have reached the end of file.
                    }
                    else
                    {
                        hres = E_FAIL;
                        break;
                    }
                } //while
            }
        }
    }
    else
        hres = E_FAIL;  //The file handle is bad.

    *puiCharsActuallyRead = (UINT)dwCharsRead;
    *puiCharsConverted = (UINT)dwTotalCharsConverted;
    return hres; 
}


HRESULT CReadFileObj::FileReadCharsA(LPSTR lpszBuff, UINT uiCharsToRead, UINT *puiCharsActuallyRead)
{
    HRESULT hres = E_FAIL;
    DWORD dwCharsRead = 0;
    
    if ((_hFile != INVALID_HANDLE_VALUE) && 
        (_iCharset == ANSI_HTML_CHARSET) &&
        ReadFile(_hFile, (LPVOID)lpszBuff, (DWORD)(uiCharsToRead), &dwCharsRead, NULL))
    {
        dwCharsRead = dwCharsRead; //get the number of wchars read.
        hres = S_OK;
    }
    *puiCharsActuallyRead = (UINT)dwCharsRead;
    return hres; 
}


HRESULT CReadFileObj::FileReadCharsW(LPWSTR lpwszBuff, UINT uiCharsToRead, UINT *puiCharsActuallyRead)
{
    HRESULT hres = E_FAIL;
    DWORD dwCharsRead = 0;
    
    if ((_hFile != INVALID_HANDLE_VALUE) && 
        (_iCharset == UNICODE_HTML_CHARSET) &&
        ReadFile(_hFile, (LPVOID)lpwszBuff, (DWORD)(uiCharsToRead*sizeof(WCHAR)), &dwCharsRead, NULL))
    {
        dwCharsRead = dwCharsRead/sizeof(WCHAR); //get the number of wchars read.
        hres = S_OK;
    }
    *puiCharsActuallyRead = (UINT)dwCharsRead;
    return hres; 
}

HRESULT CReadFileObj::FileSeekChars(LONG  lCharOffset, DWORD dwOrigin)
{
    HRESULT hres = E_FAIL;

    if(_hFile != INVALID_HANDLE_VALUE)
    {
        if(SetFilePointer(_hFile, 
                    lCharOffset*((_iCharset == UNICODE_HTML_CHARSET) ? sizeof(WCHAR) : sizeof(char)),
                    NULL,
                    dwOrigin) != INVALID_SET_FILE_POINTER)
            hres = S_OK;
    }

    return hres;
}

HRESULT CReadFileObj::FileGetCurCharOffset(LONG  *plCharOffset)
{
    HRESULT hres = E_FAIL;
    DWORD   dwByteOffset = 0;

    *plCharOffset = 0;
    if(_hFile != INVALID_HANDLE_VALUE)
    {
        if((dwByteOffset = SetFilePointer(_hFile, 
                                            0L,
                                            NULL,
                                            FILE_CURRENT)) != INVALID_SET_FILE_POINTER)
        {
            *plCharOffset = dwByteOffset/((_iCharset == UNICODE_HTML_CHARSET) ? sizeof(WCHAR) : sizeof(char));
            hres = S_OK;
        }
    }

    return hres;
}

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
    BOOL fRet = FALSE;

    if((hFile = CreateFile(pszWallpaper, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, 0)) != INVALID_HANDLE_VALUE)
    {
        fRet = GetFileTime(hFile, NULL, NULL, lpftFileTime);

        CloseHandle(hFile);
    }

    if (!fRet)
        ZeroMemory(lpftFileTime, SIZEOF(FILETIME));

    //  no return value
}

BOOL  HasWallpaperReallyChanged(LPCTSTR pszRegKey, LPTSTR pszOldWallpaper, LPTSTR pszBackupWallpaper, DWORD dwOldWallpaperStyle, DWORD dwNewWallpaperStyle)
{
    //  we default to TRUE here.
    
    if ((dwOldWallpaperStyle == dwNewWallpaperStyle)
    && (0 == lstrcmpi(pszOldWallpaper, pszBackupWallpaper)))
    {
        // The wallpaper filename and style hasn't changed. 
        //  But, the content of this file could have changed. 
        //  See if the content has changed by looking at the 
        // last-written date and time stamp on this file.
        FILETIME ftOld, ftBack;
        DWORD dwType, cbBack = SIZEOF(ftBack);

        //  if either of these fail, then they will
        //  remain Zero  so the compare will
        //  be successful.
        GetWallpaperFileTime(pszOldWallpaper, &ftOld);
        if (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, pszRegKey, c_szWallpaperTime, &dwType, &ftBack, &cbBack))
            ZeroMemory(&ftBack, SIZEOF(ftBack));


        //Get the last written time of the backup wallpaper from registry
        if (ftOld.dwLowDateTime == ftBack.dwLowDateTime
        && (ftOld.dwHighDateTime == ftBack.dwHighDateTime))
            //  everything is the same!
            return FALSE;
    }
    
    return TRUE;
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

BOOL CActiveDesktop::_IsDisplayInSafeMode(void)
{
    WCHAR wszDisplay[MAX_PATH];
    DWORD dwcch = MAX_PATH;

    return (SUCCEEDED(GetScheme(wszDisplay, &dwcch, SCHEME_GLOBAL | SCHEME_DISPLAY)) 
            && (0 == StrCmpW(wszDisplay, REG_DESKCOMP_SAFEMODE_SUFFIX_L)));
}

BOOL ReadPolicyForWallpaper(LPTSTR  lpszPolicyForWallpaper, DWORD dwSizeofBuff)
{
    TCHAR   szWallpaperName[MAX_PATH];
    DWORD   dwType;
    
    if(!lpszPolicyForWallpaper)
    {
        lpszPolicyForWallpaper = szWallpaperName;
        dwSizeofBuff = ARRAYSIZE(szWallpaperName);
    }

    if((SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_WP_POLICY, c_szWallpaper, &dwType,(LPBYTE)lpszPolicyForWallpaper,
                   &dwSizeofBuff) == ERROR_SUCCESS) && dwSizeofBuff)
        return TRUE;  //Policy is there!
    else
        return FALSE; //No policy is set!
}

BOOL ReadPolicyForWPStyle(LPDWORD  lpdwStyle)
{
    DWORD   dwStyle;
    DWORD   dwType;
    TCHAR   szValue[20];
    DWORD   dwSizeofValueBuff = ARRAYSIZE(szValue);
    BOOL    fRet = FALSE;

    // The caller can passin a NULL, if they are not interested in the actual value and they just
    // want to know if this policy is set or not.
    if(!lpdwStyle)  
        lpdwStyle = &dwStyle;

    if ((SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_WP_POLICY, c_szWallpaperStyle, &dwType,(LPBYTE)szValue,
                   &dwSizeofValueBuff) == ERROR_SUCCESS) && dwSizeofValueBuff)
    {
        if(dwType == REG_SZ)
        {
            *lpdwStyle = (DWORD)StrToInt(szValue);
            fRet = TRUE;
        }
    }

    return fRet;
}

void CActiveDesktop::_ReadWallpaper(BOOL fActiveDesktop)
{
    ENTERPROC(2, "DS ReadWallpaper()");

    TCHAR lpszDeskcomp[MAX_PATH];

    GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_GENERAL, _pszScheme);

    _fPolicyForWPName = ReadPolicyForWallpaper(_szSelectedWallpaper, ARRAYSIZE(_szSelectedWallpaper));
    _fPolicyForWPStyle = ReadPolicyForWPStyle(&_wpo.dwStyle);
    
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
        GetStringFromReg(HKEY_CURRENT_USER, c_szRegStrDesktop, c_szWallpaper, c_szNULL, szOldWallpaper, ARRAYSIZE(szOldWallpaper));

        // Read wallpaper style from the old location.
        ReadWallpaperStyleFromReg((LPCTSTR)c_szRegStrDesktop, &dwOldWallpaperStyle, FALSE);

        // Read the wallpaper from the new location too!
        if((!_fPolicyForWPName) || (_IsDisplayInSafeMode()))
        {
            if (!GetStringFromReg(HKEY_CURRENT_USER, pszRegKey, c_szWallpaper, szOldWallpaper, _szSelectedWallpaper, ARRAYSIZE(_szSelectedWallpaper)))
            {
                pszRegKey = c_szRegStrDesktop;
            }
        }

        //Read wallpaper style from the new location too!
        if(!_fPolicyForWPStyle)
            ReadWallpaperStyleFromReg(pszRegKey, &_wpo.dwStyle, FALSE);
        
        //If there is a Safe mode scheme here do NOT attempt to change wallpaper
        if((!_IsDisplayInSafeMode()) && (!_fPolicyForWPName))
        {
            //Read what is stored as "Backup" wallpaper.
            GetStringFromReg(HKEY_CURRENT_USER, pszRegKey, c_szBackupWallpaper, szOldWallpaper, _szBackupWallpaper, ARRAYSIZE(_szBackupWallpaper));
    
            //See if the Old wallpaper is differnet from the backed up wallpaper
            if(HasWallpaperReallyChanged(pszRegKey, szOldWallpaper, _szBackupWallpaper, dwOldWallpaperStyle, _wpo.dwStyle))
            {
                //They are different. This means that some other app has changed the "Old" wallpaper
                //after the last time we backed it up in the registry.
                // Make this wallpaper as the Selected wallpaper!

#ifdef WE_WANT_DEFAULT_WALLPAPER
                // The following hack is needed only if we want default wallpapers. For example, when
                // we port shell code to Win2000 millenium, we may want a default wallpaper in which case,
                // we need to enable the following hack and also in the deskcls.cpp file.
                //
                // This is a kinda hack, but the best possible solution right now. The scenario is as follows.
                // The Memphis setup guys replace what the user specifies as the wallpaper in the old location
                // and restore it after setup is complete. But, SetDefaultWallpaper() gets called bet. these
                // two times and we are supposed to take a decision on whether to set the default htm wallpaper or not,
                // depending on what the user had set before the installation. The solution is to delay making
                // this decision until after the setup guys have restored the user's wallpaper. We do this in
                // CActiveDesktop::_ReadWallpaper(). We specify that SetDefaultWallpaper() was called by setting
                // the backup wallpaper in the new location to the default wallpaper.
                TCHAR szDefaultWallpaper[MAX_PATH];
                GetDefaultWallpaper(szDefaultWallpaper, SIZECHARS(szDefaultWallpaper));

                if(lstrcmp(_szBackupWallpaper, szDefaultWallpaper) == 0
                    && (!szOldWallpaper[0] || lstrcmp(szOldWallpaper, g_szNone) == 0))
                {
                    lstrcpy(_szSelectedWallpaper, szDefaultWallpaper);
                }
                else
                {
                    lstrcpy(_szSelectedWallpaper, szOldWallpaper);
                }
#else  // WE_WANT_DEFAULT_WALLPAPER
                lstrcpy(_szSelectedWallpaper, szOldWallpaper);
#endif // WE_WANT_DEFAULT_WALLPAPER
                _wpo.dwStyle = dwOldWallpaperStyle;

                _fWallpaperDirty = TRUE;
                _fWallpaperChangedDuringInit = TRUE;
            }

        }
        //Make a backup of the "Old" wallpaper
        lstrcpy(_szBackupWallpaper, szOldWallpaper);
    }
    else
    {
        pszRegKey = c_szRegStrDesktop; //Get it from the old location!

        //Since active desktop is not available, read wallpaper from old location.
        if(!_fPolicyForWPName)
            GetStringFromReg(HKEY_CURRENT_USER, pszRegKey, c_szWallpaper, c_szNULL, _szSelectedWallpaper, ARRAYSIZE(_szSelectedWallpaper));

        //Make a backup of the "Old" wallpaper
        lstrcpy(_szBackupWallpaper, _szSelectedWallpaper);

        //Read the wallpaper style
        if(!_fPolicyForWPStyle)
            ReadWallpaperStyleFromReg(pszRegKey, &_wpo.dwStyle, TRUE);
    }

    EXITPROC(2, "DS ReadWallpaper! (_szSelectedWP=>%s<)", _szSelectedWallpaper);
}

void CActiveDesktop::_ReadPattern(void)
{
    ENTERPROC(2, "DS ReadPattern()");

    GetStringFromReg(HKEY_CURRENT_USER, c_szRegStrDesktop, c_szPattern, c_szNULL, _szSelectedPattern, ARRAYSIZE(_szSelectedPattern));

    EXITPROC(2, "DS ReadPattern! (_szSelectedPattern=>%s<)", _szSelectedPattern);
}

void CActiveDesktop::_ReadComponent(HKEY hkey, LPCTSTR pszComp)
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
        if (SHQueryValueEx(hkeyComp, REG_VAL_COMP_SOURCE, NULL, &dwType, (LPBYTE)&comp.szSource, &cbSize) != ERROR_SUCCESS)
        {
            comp.szSource[0] = TEXT('\0');
        }

        //
        // Read in the SubscribedURL string.
        //
        cbSize = SIZEOF(comp.szSubscribedURL);
        if (SHQueryValueEx(hkeyComp, REG_VAL_COMP_SUBSCRIBED_URL, NULL, &dwType, (LPBYTE)&comp.szSubscribedURL, &cbSize) != ERROR_SUCCESS)
        {
            comp.szSubscribedURL[0] = TEXT('\0');
        }

        //
        // Read in the Friendly name string.
        //
        cbSize = SIZEOF(comp.szFriendlyName);
        if (SHQueryValueEx(hkeyComp, REG_VAL_COMP_NAME, NULL, &dwType, (LPBYTE)&comp.szFriendlyName, &cbSize) != ERROR_SUCCESS)
        {
            comp.szFriendlyName[0] = TEXT('\0');
        }

        //
        // Read in and parse the flags.
        //
        DWORD dwFlags;
        cbSize = SIZEOF(dwFlags);
        if (SHQueryValueEx(hkeyComp, REG_VAL_COMP_FLAGS, NULL, &dwType, (LPBYTE)&dwFlags, &cbSize) != ERROR_SUCCESS)
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
        if (SHQueryValueEx(hkeyComp, REG_VAL_COMP_POSITION, NULL, &dwType, (LPBYTE)&comp.cpPos, &cbSize) != ERROR_SUCCESS)
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
        if (SHQueryValueEx(hkeyComp, REG_VAL_COMP_CURSTATE, NULL, &dwType, (LPBYTE)&comp.dwCurItemState, &cbSize) != ERROR_SUCCESS)
        {
            //If the item state is missing, we must be reading from IE4 machine.
            comp.dwCurItemState = IS_NORMAL;
        }

        //
        //  Read in the Original state info.
        //
        cbSize = SIZEOF(comp.csiOriginal);
        if ((SHQueryValueEx(hkeyComp, REG_VAL_COMP_ORIGINALSTATEINFO, NULL, &dwType, (LPBYTE)&comp.csiOriginal, &cbSize) != ERROR_SUCCESS) ||
            (comp.csiOriginal.dwSize != SIZEOF(comp.csiOriginal)))
        {
            //If the item state is missing, we must be reading from IE4 machine.
            // Set the OriginalState to the default info.
            SetStateInfo(&comp.csiOriginal, &comp.cpPos, IS_NORMAL);
            comp.csiOriginal.dwHeight = comp.csiOriginal.dwWidth = COMPONENT_DEFAULT_WIDTH;
        }

        //
        //  Read in the Restored state info.
        //
        cbSize = SIZEOF(comp.csiRestored);
        if (SHQueryValueEx(hkeyComp, REG_VAL_COMP_RESTOREDSTATEINFO, NULL, &dwType, (LPBYTE)&comp.csiRestored, &cbSize) != ERROR_SUCCESS)
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
void CActiveDesktop::_SortAndRationalize(void)
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

void CActiveDesktop::_ReadComponents(BOOL fActiveDesktop)
{
    ENTERPROC(2, "DS ReadComponents()");

    HKEY hkey;
    TCHAR lpszDeskcomp[MAX_PATH];

    GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_COMPONENTS, _pszScheme);

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
        if (SHQueryValueEx(hkey, REG_VAL_COMP_SETTINGS, NULL, &dwType, (LPBYTE)&dwSettings, &cbSize) == ERROR_SUCCESS)
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

void CActiveDesktop::_Initialize(void)
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

        // If we are in safemode, the we can not use Dynamic Html to make updates because
        // updates involve complete change of background Html.
        if(_IsDisplayInSafeMode())
            _fUseDynamicHtml = FALSE;
        else
            _fUseDynamicHtml = TRUE;        //Any component added after the initialization must go through dynamic html.

        _fDirty = FALSE;
        _fNeedBodyEnd = FALSE;
    }

    EXITPROC(2, "DS Initialize!");
}

void CActiveDesktop::_SaveWallpaper(void)
{
    ENTERPROC(2, "DS SaveWallpaper");
    TCHAR lpszDeskcomp[MAX_PATH];
    BOOL    fNormalWallpaper;

    GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_GENERAL, _pszScheme);

    //
    // Compute tiling string.
    //
    TCHAR szTiled[2];
    lstrcpy(szTiled, TEXT("0"));
    szTiled[0] = szTiled[0] + (TCHAR)(_wpo.dwStyle & WPSTYLE_TILE);

    //
    // Compute the Wallpaper styling string
    //
    TCHAR       szWPStyle[2];
    lstrcpy(szWPStyle, TEXT("0"));
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
        if(!_fPolicyForWPStyle)
        {
            SHSetValue(HKEY_CURRENT_USER, lpszDeskcomp,
                c_szTileWall, REG_SZ, (LPBYTE)szTiled, SIZEOF(TCHAR)*(lstrlen(szTiled)+1));
        }

        //
        // Note: We do not write the Wallpaper Style string for older systems because we do not
        // want to over-write what PlusPack writes. However, for newer Operating systems, we 
        // want to write the WallpaperStyle also.
        //
        if((g_bRunOnMemphis || g_bRunOnNT5) && (!_fPolicyForWPStyle))
        {
            SHSetValue(HKEY_CURRENT_USER, lpszDeskcomp,
                c_szWallpaperStyle, REG_SZ, (LPBYTE)szWPStyle, SIZEOF(TCHAR)*(lstrlen(szWPStyle)+1));
        }

        if(!_fPolicyForWPName)
            SHRegSetPath(HKEY_CURRENT_USER, lpszDeskcomp, c_szWallpaper, _szSelectedWallpaper, 0);
    }

    if(fNormalWallpaper = IsNormalWallpaper(_szSelectedWallpaper))
    {
        lstrcpyn(_szBackupWallpaper, _szSelectedWallpaper, ARRAYSIZE(_szBackupWallpaper));
    }

    if(!_fPolicyForWPName)
    {
        FILETIME ft;
        GetWallpaperFileTime(_szBackupWallpaper, &ft);

        // Backup the "Old type" wallpaper's name here in the new location
        // sothat we can detect when this gets changed by some other app.
        SHRegSetPath(HKEY_CURRENT_USER, lpszDeskcomp, c_szBackupWallpaper, _szBackupWallpaper, 0);

        SHSetValue(HKEY_CURRENT_USER, lpszDeskcomp,
                c_szWallpaperTime, REG_BINARY, (LPBYTE)&ft,
                SIZEOF(ft));    
    }
    
    //
    // Even if this wallpaper is not valid in normal desktop (i.e., even if it is not a .BMP),
    // write it out in normal desktop registry area.
    //
    if (_fWallpaperDirty)
    {
        if(!_fPolicyForWPStyle)
        {
            SHSetValue(HKEY_CURRENT_USER, c_szRegStrDesktop,
                    c_szTileWall, REG_SZ, (LPBYTE)szTiled, SIZEOF(TCHAR)*(lstrlen(szTiled)+1));
        }
        //
        // Note: We do not write the Wallpaper Style string for older systems because we do not
        // want to over-write what PlusPack writes. However, for newer Operating systems, we 
        // want to write the WallpaperStyle also.
        //
        if((g_bRunOnMemphis || g_bRunOnNT5) && (!_fPolicyForWPStyle))
        {
            SHSetValue(HKEY_CURRENT_USER, c_szRegStrDesktop,
                        c_szWallpaperStyle, REG_SZ, (LPBYTE)szWPStyle, SIZEOF(TCHAR)*(lstrlen(szWPStyle)+1));
        }

        if(!_fPolicyForWPName)
        {
            SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, 
                    (fNormalWallpaper ? _szSelectedWallpaper : _szBackupWallpaper),
                     SPIF_UPDATEINIFILE);
        }
    }

    EXITPROC(2, "DS SaveWallpaper");
}

void CActiveDesktop::_SaveComponent(HKEY hkey, int iIndex, COMPONENTA *pcomp)
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


void CActiveDesktop::_SaveComponents(void)
{
    ENTERPROC(2, "DS SaveComponents");
    DWORD dwType, dwFlags = 0, dwDataLength = SIZEOF(dwFlags);
    int i;
    TCHAR lpszDeskcomp[MAX_PATH];

    GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_COMPONENTS, _pszScheme);

    //
    // We need to preserve the old GENFLAGS, so read them now before we roach them.
    //
    SHGetValue(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, REG_VAL_COMP_GENFLAGS, &dwType,
                            (LPBYTE)(&dwFlags), &dwDataLength);

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

        //
        // Write out the general flags
        //
        RegSetValueEx(hkey, REG_VAL_COMP_GENFLAGS, 0, REG_DWORD, (LPBYTE)&dwFlags, SIZEOF(dwFlags));

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

void CActiveDesktop::_SavePattern(DWORD dwFlags)
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

void CActiveDesktop::_WriteHtmlFromString(LPCTSTR psz)
{
    ENTERPROC(3, "DS WriteHtmlFromString(psz=>%s<)", psz);
    LPCWSTR  pwsz;
    WCHAR   szBuf[INTERNET_MAX_URL_LENGTH];
    UINT    uiLen;
    int     cch;

#ifdef UNICODE
    if((_pStream == NULL) && (_iDestFileCharset == ANSI_HTML_CHARSET))
    {
        cch = SHUnicodeToAnsi(psz, (LPSTR)szBuf, ARRAYSIZE(szBuf));
        ASSERT(cch == lstrlenW((LPWSTR)psz)+1);
        pwsz = (LPCWSTR)szBuf;
        uiLen = lstrlenA((LPSTR)szBuf);
    }
    else
    {
        pwsz = psz;
        uiLen = lstrlenW(pwsz);
    }
#else
    if((_pStream == NULL) && (_iDestFileCharset == ANSI_HTML_CHARSET))
    {
        pwsz = (LPWSTR)psz;
        uiLen = lstrlenA(psz);
    }
    else
    {
        cch = SHAnsiToUnicode(psz, szBuf, ARRAYSIZE(szBuf));
        ASSERT(cch == lstrlenA(psz)+1);
        pwsz = (LPCWSTR)szBuf;
        uiLen = lstrlenW(pwsz);
    }
#endif

    UINT cbWritten;

    _WriteHtmlW(pwsz, uiLen, &cbWritten);
    
    EXITPROC(3, "DS WriteHtmlFromString!");
}


void CActiveDesktop::_WriteHtmlFromId(UINT uid)
{
    ENTERPROC(3, "DS WriteHtmlFromId(uid=%d)", uid);

    TCHAR szBuf[INTERNET_MAX_URL_LENGTH];
    LoadString(HINST_THISDLL, uid, szBuf, ARRAYSIZE(szBuf));
    _WriteHtmlFromString(szBuf);

    EXITPROC(3, "DS WriteHtmlFromId!");
}

void CActiveDesktop::_WriteHtmlFromIdF(UINT uid, ...)
{
    ENTERPROC(3, "DS WriteHtmlFromIdF(uid=%d,...)", uid);

    TCHAR szBufFmt[INTERNET_MAX_URL_LENGTH];
    TCHAR szBuf[INTERNET_MAX_URL_LENGTH];

    LoadString(HINST_THISDLL, uid, szBufFmt, ARRAYSIZE(szBufFmt));

    va_list arglist;
    va_start(arglist, uid);
    wvsprintf(szBuf, szBufFmt, arglist);
    va_end(arglist);

    _WriteHtmlFromString(szBuf);

    EXITPROC(3, "DS WriteHtmlFromIdF!");
}

void CActiveDesktop::_WriteHtmlFromFile(LPCTSTR pszContents)
{
    ENTERPROC(3, "DS WriteHtmlFromFile(pszContents=>%s<)", pszContents);
    
    CReadFileObj *pReadFileObj = new CReadFileObj(pszContents);

    if (pReadFileObj)
    {
        if(pReadFileObj->_hFile != INVALID_HANDLE_VALUE)
        {
            WCHAR wcBuf[INTERNET_MAX_URL_LENGTH + 1];
            UINT uiCharCount = ARRAYSIZE(wcBuf) -1; //Leave room for null termination.
            UINT uiCharsRead;
            UINT uiCharsConverted;
            int iDestCharset = (_pStream ? UNICODE_HTML_CHARSET : _iDestFileCharset);
            while (SUCCEEDED(pReadFileObj->FileReadAndConvertChars(iDestCharset, wcBuf, uiCharCount, &uiCharsRead, &uiCharsConverted)) && uiCharsRead)
            {
                UINT cbWritten;
            
                _WriteHtmlW(wcBuf, uiCharsConverted, &cbWritten);
            
                if (uiCharsRead < uiCharCount)
                {
                    break;
                }
            }
        }
        delete pReadFileObj;
    }
    
    EXITPROC(3, "DS WriteHtmlFromFile!");
}

void CActiveDesktop::_WriteHtmlFromReadFileObj(CReadFileObj *pFileObj, int iOffsetStart, int iOffsetEnd)
{
    ENTERPROC(3, "DS WriteHtmlFromReadFileObj(pFileObj=%08X,iOffsetStart=%d,iOffsetEnd=%d)", pFileObj, iOffsetStart, iOffsetEnd);

    if (iOffsetStart != -1)
    {
        pFileObj->FileSeekChars(iOffsetStart, FILE_BEGIN);
    }
    else
    {
        ASSERT(iOffsetEnd == -1);
        iOffsetEnd = -1;
    }

    //Get the number of WIDECHARs to be written
    UINT cchWrite = (iOffsetEnd == -1) ? 0xFFFFFFFF : (iOffsetEnd - iOffsetStart);

    while (cchWrite)
    {
        WCHAR wcBuf[INTERNET_MAX_URL_LENGTH+1];

        //
        // Read a chunk.
        //
        UINT cchTryRead = min(cchWrite, (ARRAYSIZE(wcBuf) - 1));
        UINT cchActualRead;
        HRESULT hres;

        //Note: if we are reading ANSI, we still use the unicode buff; but cast it!
        if(_iDestFileCharset == ANSI_HTML_CHARSET)
            hres = pFileObj->FileReadCharsA((LPSTR)wcBuf, cchTryRead, &cchActualRead);
        else
            hres = pFileObj->FileReadCharsW(wcBuf, cchTryRead, &cchActualRead);
            
        if(SUCCEEDED(hres) && cchActualRead)
        {
            //
            // Write a chunk.
            //
            UINT cchWritten;
            
            _WriteHtmlW(wcBuf, cchActualRead, &cchWritten);
            
            if (cchActualRead < cchTryRead)
            {
                //
                // End of file, all done.
                //
                break;
            }

            cchWrite -= cchActualRead;
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

int CActiveDesktop::_ScanForTagA(CReadFileObj *pFileObj, int iOffsetStart, LPCSTR pszTag)
{
    ENTERPROC(2, "DS ScanForTagA(pFileObj=%08X,iOffsetStart=%d,pszTagA=>%s<)",
    pFileObj, iOffsetStart, pszTag);

    int iRet = -1;
    BOOL fDoneReading = FALSE;
    int iOffset;
    DWORD cchTag = lstrlenA(pszTag);

    pFileObj->FileSeekChars(iOffsetStart, FILE_BEGIN);
    iOffset = iOffsetStart;

    DWORD cchBuf = 0;
    while (!fDoneReading)
    {
        char szBuf[INTERNET_MAX_URL_LENGTH+1];

        //
        // Fill in the buffer.
        //
        UINT cchTryRead = ARRAYSIZE(szBuf) - cchBuf - 1;
        UINT cchRead;
        if(SUCCEEDED(pFileObj->FileReadCharsA(&szBuf[cchBuf], cchTryRead, &cchRead)) && cchRead)
        {
            cchBuf += cchRead;

            //
            // Terminate the string.
            //
            szBuf[cchBuf] = '\0';

            //
            // Scan for the tag.
            //
            LPSTR pszTagInBuf = StrStrIA(szBuf, pszTag);

            if (pszTagInBuf)
            {
                //
                // Found the tag, compute the offset.
                //
                iRet = (int) (iOffset + pszTagInBuf - szBuf);
                fDoneReading = TRUE;
            }
            else if (cchRead < cchTryRead)
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
                DWORD cchSkip = cchBuf - cchTag;

                //
                // Advance the file offset.
                //
                iOffset += cchSkip;

                //
                // Reduce the buffer size.
                //
                cchBuf -= cchSkip;

                //
                // Move the kept bytes to the beginning of the buffer.
                //
                MoveMemory(szBuf, szBuf + cchSkip, cchBuf);
            }
        }
        else
        {
            fDoneReading = TRUE;
        }
    }

    EXITPROC(2, "DS ScanForTagA=%d", iRet);
    return iRet;
}

int CActiveDesktop::_ScanForTagW(CReadFileObj *pFileObj, int iOffsetStart, LPCWSTR pwszTag)
{
    ENTERPROC(2, "DS ScanForTag(pFileObj=%08X,iOffsetStart=%d,pszTagA=>%s<)",
    pFileObj, iOffsetStart, pwszTag);

    int iRet = -1;
    BOOL fDoneReading = FALSE;
    int iOffset;
    DWORD cchTag = lstrlenW(pwszTag);

    pFileObj->FileSeekChars(iOffsetStart, FILE_BEGIN);
    iOffset = iOffsetStart;

    DWORD cchBuf = 0;
    while (!fDoneReading)
    {
        WCHAR wszBuf[INTERNET_MAX_URL_LENGTH+1];

        //
        // Fill in the buffer.
        //
        UINT cchTryRead = ARRAYSIZE(wszBuf) - cchBuf - 1;
        UINT cchRead;
        if(SUCCEEDED(pFileObj->FileReadCharsW(&wszBuf[cchBuf], cchTryRead, &cchRead)) && cchRead)
        {
            cchBuf += cchRead;

            //
            // Terminate the string.
            //
            wszBuf[cchBuf] = L'\0';

            //
            // Scan for the tag.
            //
            LPWSTR pwszTagInBuf = StrStrIW(wszBuf, pwszTag);

            if (pwszTagInBuf)
            {
                //
                // Found the tag, compute the offset.
                //
                iRet = (int) (iOffset + pwszTagInBuf - wszBuf);
                fDoneReading = TRUE;
            }
            else if (cchRead < cchTryRead)
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
                DWORD cchSkip = cchBuf - cchTag;

                //
                // Advance the file offset.
                //
                iOffset += cchSkip;

                //
                // Reduce the buffer size.
                //
                cchBuf -= cchSkip;

                //
                // Move the kept bytes to the beginning of the buffer.
                //
                MoveMemory(wszBuf, wszBuf + cchSkip, cchBuf*sizeof(WCHAR));
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

int CActiveDesktop::_ScanTagEntriesA(CReadFileObj *pReadFileObj, int iOffsetStart, TAGENTRYA *pte, int cte)
{
    ENTERPROC(2, "DS ScanTagEntriesA(pReadFileObj=%08X,iOffsetStart=%d,pte=%08X,cte=%d)",
                pReadFileObj, iOffsetStart, pte, cte);

    int iRet = -1;
    int i;

    for (i=0; i<cte; i++,pte++)
    {
        iRet = _ScanForTagA(pReadFileObj, iOffsetStart, pte->pszTag);
        if (iRet != -1)
        {
            if (pte->fSkipPast)
            {
                iRet += lstrlenA(pte->pszTag);
            }
            break;
        }
    }

    EXITPROC(2, "DS ScanTagEntriesA=%d", iRet);
    return iRet;
}

int CActiveDesktop::_ScanTagEntriesW(CReadFileObj *pReadFileObj, int iOffsetStart, TAGENTRYW *pte, int cte)
{
    ENTERPROC(2, "DS ScanTagEntriesW(pReadFileObj=%08X,iOffsetStart=%d,pte=%08X,cte=%d)",
                pReadFileObj, iOffsetStart, pte, cte);

    int iRet = -1;
    int i;

    for (i=0; i<cte; i++,pte++)
    {
        iRet = _ScanForTagW(pReadFileObj, iOffsetStart, pte->pwszTag);
        if (iRet != -1)
        {
            if (pte->fSkipPast)
            {
                iRet += lstrlenW(pte->pwszTag);
            }
            break;
        }
    }

    EXITPROC(2, "DS ScanTagEntriesW=%d", iRet);
    return iRet;
}

void CActiveDesktop::_ParseAnsiInputHtmlFile( LPTSTR szSelectedWallpaper, int *piOffsetBase, int *piOffsetComp)
{
    //
    // Figure out where to insert the base href tag.
    //
    int     iOffsetBase = 0, iBaseTagStart;
    BOOL    fUseBaseHref;
    LONG    lOffsetDueToBOM = 0; //Character Offset due to the Byte Order Mark.
                                 //1 for UNICODE and 0 for ANSI files.

//  98/11/11 #248047 vtan: This code looks for a <BASE HREF=...> tag.
//  It used to use a scan for "<BASE" and assume that this was the
//  desired tag. HTML allows a "<BASEFONT>" tag which was being
//  mistaken for a "<BASE HREF=...>" tag. The code now looks for the
//  same string but looks at the character following the "<BASE" to
//  see if it's a white-space character.

    fUseBaseHref = TRUE;
    _pReadFileObjHtmlBkgd->FileGetCurCharOffset(&lOffsetDueToBOM);
    iOffsetBase = (int)lOffsetDueToBOM;
    iBaseTagStart = _ScanForTagA(_pReadFileObjHtmlBkgd, (int)lOffsetDueToBOM, "<BASE");
                
    if (iBaseTagStart != -1)
    {
        UINT   uiCountChars, uiTryToRead;
        char   szBaseTagBuffer[6+1];     // allow for "<BASEx" plus a NULL.

        _pReadFileObjHtmlBkgd->FileSeekChars(iBaseTagStart, FILE_BEGIN);
        uiTryToRead = ARRAYSIZE(szBaseTagBuffer) - 1;
        if(SUCCEEDED(_pReadFileObjHtmlBkgd->FileReadCharsA(szBaseTagBuffer, uiTryToRead, &uiCountChars)) && uiCountChars)
        {
            char    ch;

            ch = szBaseTagBuffer[5];
            fUseBaseHref = ((ch != ' ') &&
                            (ch != '\r') &&
                            (ch != '\n') &&      // this covers the UNIX line break scheme
                            (ch != '\t'));
        }
    }
    if (fUseBaseHref)
    {
        TAGENTRYA rgteBase[] = {
                                   { "<HEAD>", TRUE, },
                                   { "<BODY", FALSE, },
                                   { "<HTML>", TRUE, },
                               };
        iOffsetBase = _ScanTagEntriesA(_pReadFileObjHtmlBkgd, (int)lOffsetDueToBOM, rgteBase, ARRAYSIZE(rgteBase));
        if (iOffsetBase == -1)
        {
            iOffsetBase = (int)lOffsetDueToBOM;
        }
    }

    //
    // Figure out where to insert the components.
    //
    TAGENTRYA rgteComponents[] = {
                                     { "</BODY>", FALSE, },
                                     { "</HTML>", FALSE, },
                                 };
    int iOffsetComponents = _ScanTagEntriesA(_pReadFileObjHtmlBkgd, iOffsetBase, rgteComponents, ARRAYSIZE(rgteComponents));

    //
    // Write out the initial HTML up to the <HEAD> tag.
    //
    _WriteHtmlFromReadFileObj(_pReadFileObjHtmlBkgd, (int)lOffsetDueToBOM, iOffsetBase);

    //
    // Write out the base tag.
    //
    if (fUseBaseHref)
    {
        //BASE tag must point to the base "URL". So, don't strip out the filename.
        _WriteHtmlFromIdF(IDS_BASE_TAG, szSelectedWallpaper);
    }

    // Figure out where to insert the DIV clause
    TAGENTRYA rgteBodyStart[] = {
                                    { "<BODY", FALSE, },
                                };
    int iOffsetBodyStart = _ScanTagEntriesA(_pReadFileObjHtmlBkgd, iOffsetBase, rgteBodyStart, ARRAYSIZE(rgteBodyStart));
    // Write out HTML until after the <BODY ......>
    if (iOffsetBodyStart == -1)
    {   // the <BODY> tag is not found, so we need to insert it.
        // Copy over stuff until </HEAD>
        TAGENTRYA rgteHeadEnd[] = {
                                      { "</HEAD>", TRUE, },
                                  };
        int iOffsetHeadEnd = _ScanTagEntriesA(_pReadFileObjHtmlBkgd, iOffsetBase, rgteHeadEnd, ARRAYSIZE(rgteHeadEnd));
        if(iOffsetHeadEnd != -1)
        {
            _WriteHtmlFromReadFileObj(_pReadFileObjHtmlBkgd, iOffsetBase, iOffsetHeadEnd);
            iOffsetBase = iOffsetHeadEnd;
        }
        _WriteHtmlFromIdF(IDS_BODY_CENTER_WP2); // "<BODY>"
        _fNeedBodyEnd = TRUE;
    }
    else
    {
        TAGENTRYA rgteBodyEnd[] = {
                                       { ">", TRUE, },
                                  };
        int iOffsetBodyEnd = _ScanTagEntriesA(_pReadFileObjHtmlBkgd, iOffsetBodyStart, rgteBodyEnd, ARRAYSIZE(rgteBodyEnd));
        if (iOffsetBodyEnd == -1)
        {   // An error in the HTML.
            iOffsetBodyEnd = iOffsetBodyStart;  // BUGBUG: We need a better recovery idea.
        }
        _WriteHtmlFromReadFileObj(_pReadFileObjHtmlBkgd, iOffsetBase, iOffsetBodyEnd);
        iOffsetBase = iOffsetBodyEnd;
    }

    *piOffsetBase = iOffsetBase;
    *piOffsetComp = iOffsetComponents;
}


void CActiveDesktop::_GenerateHtmlHeader(void)
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

    //Assume that the final Deskstat.htt that we generate is going to be in UNICODE.
    //This will change to ANSI only if the background html wallpaper is ANSI (determined later)
    _iDestFileCharset = UNICODE_HTML_CHARSET;
    
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
        //
        //  Write the BOM for UNICODE
        //
        if(_hFileHtml)
        {
            DWORD cbWritten;
            
            WriteFile(_hFileHtml, (LPCSTR)&wUnicodeBOM, sizeof(wUnicodeBOM), &cbWritten, NULL);
        }
    
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
        if ((_pReadFileObjHtmlBkgd = new CReadFileObj(szSelectedWallpaper)) &&
            (_pReadFileObjHtmlBkgd->_hFile != INVALID_HANDLE_VALUE))
        {
            //The final Desktop.htt will be in ANSI only if the source html file is also in ansi.
            //So, get the type from the selected wallpaper object.
            _iDestFileCharset = _pReadFileObjHtmlBkgd->_iCharset;
            //
            //  Write the BOM for UNICODE
            //
            if(_hFileHtml && (_iDestFileCharset == UNICODE_HTML_CHARSET))
            {
                DWORD cbWritten;
            
                WriteFile(_hFileHtml, (LPCSTR)&wUnicodeBOM, sizeof(wUnicodeBOM), &cbWritten, NULL);
            }
    
            //
            // Figure out where to insert the base href tag.
            //
            int     iOffsetBase = 0;
            int     iOffsetComponents;
//  98/11/11 #248047 vtan: This code looks for a <BASE HREF=...> tag.
//  It used to use a scan for "<BASE" and assume that this was the
//  desired tag. HTML allows a "<BASEFONT>" tag which was being
//  mistaken for a "<BASE HREF=...>" tag. The code now looks for the
//  same string but looks at the character following the "<BASE" to
//  see if it's a white-space character.

            if(_iDestFileCharset == ANSI_HTML_CHARSET)
            {
                //The following function parses the ANSI input html file and finds various offsets
                _ParseAnsiInputHtmlFile(szSelectedWallpaper, &iOffsetBase, &iOffsetComponents);
            }
            else
            {
                //The following code parses the UNICODE input html wallpaper file.
                int iBaseTagStart;
                BOOL    fUseBaseHref;
                LONG    lOffsetDueToBOM = 0; //Character Offset due to the Byte Order Mark.
                                         //1 for UNICODE and 0 for ANSI files.
                fUseBaseHref = TRUE;
                _pReadFileObjHtmlBkgd->FileGetCurCharOffset(&lOffsetDueToBOM);
                iOffsetBase = (int)lOffsetDueToBOM;
                iBaseTagStart = _ScanForTagW(_pReadFileObjHtmlBkgd, (int)lOffsetDueToBOM, L"<BASE");
                
                if (iBaseTagStart != -1)
                {
                    UINT   uiCountChars, uiTryToRead;
                    WCHAR  wszBaseTagBuffer[6+1];     // allow for "<BASEx" plus a NULL.

                    _pReadFileObjHtmlBkgd->FileSeekChars(iBaseTagStart, FILE_BEGIN);
                    uiTryToRead = ARRAYSIZE(wszBaseTagBuffer) - 1;
                    if(SUCCEEDED(_pReadFileObjHtmlBkgd->FileReadCharsW(wszBaseTagBuffer, uiTryToRead, &uiCountChars)) && uiCountChars)
                    {
                        WCHAR    wc;

                        wc = wszBaseTagBuffer[5];
                        fUseBaseHref = ((wc != L' ') &&
                                        (wc != L'\r') &&
                                        (wc != L'\n') &&      // this covers the UNIX line break scheme
                                        (wc != L'\t'));
                    }
                }
                if (fUseBaseHref)
                {
                    TAGENTRYW rgteBase[] = {
                                            { L"<HEAD>", TRUE, },
                                            { L"<BODY", FALSE, },
                                            { L"<HTML>", TRUE, },
                                           };
                    iOffsetBase = _ScanTagEntriesW(_pReadFileObjHtmlBkgd, (int)lOffsetDueToBOM, rgteBase, ARRAYSIZE(rgteBase));
                    if (iOffsetBase == -1)
                    {
                        iOffsetBase = (int)lOffsetDueToBOM;
                    }
                }

                //
                // Figure out where to insert the components.
                //
                TAGENTRYW rgteComponents[] = {
                                                { L"</BODY>", FALSE, },
                                                { L"</HTML>", FALSE, },
                                             };
                iOffsetComponents = _ScanTagEntriesW(_pReadFileObjHtmlBkgd, iOffsetBase, rgteComponents, ARRAYSIZE(rgteComponents));

                //
                // Write out the initial HTML up to the <HEAD> tag.
                //
                _WriteHtmlFromReadFileObj(_pReadFileObjHtmlBkgd, (int)lOffsetDueToBOM, iOffsetBase);

                //
                // Write out the base tag.
                //
                if (fUseBaseHref)
                {
                    //BASE tag must point to the base "URL". So, don't strip out the filename.
                    _WriteHtmlFromIdF(IDS_BASE_TAG, szSelectedWallpaper);
                }

                // Figure out where to insert the DIV clause
                TAGENTRYW rgteBodyStart[] = {
                                                { L"<BODY", FALSE, },
                                            };
                int iOffsetBodyStart = _ScanTagEntriesW(_pReadFileObjHtmlBkgd, iOffsetBase, rgteBodyStart, ARRAYSIZE(rgteBodyStart));
                // Write out HTML until after the <BODY ......>
                if (iOffsetBodyStart == -1)
                {   // the <BODY> tag is not found, so we need to insert it.
                    // Copy over stuff until </HEAD>
                    TAGENTRYW rgteHeadEnd[] = {
                                                { L"</HEAD>", TRUE, },
                                              };
                    int iOffsetHeadEnd = _ScanTagEntriesW(_pReadFileObjHtmlBkgd, iOffsetBase, rgteHeadEnd, ARRAYSIZE(rgteHeadEnd));
                    if(iOffsetHeadEnd != -1)
                    {
                        _WriteHtmlFromReadFileObj(_pReadFileObjHtmlBkgd, iOffsetBase, iOffsetHeadEnd);
                        iOffsetBase = iOffsetHeadEnd;
                    }
                    _WriteHtmlFromIdF(IDS_BODY_CENTER_WP2); // "<BODY>"
                    _fNeedBodyEnd = TRUE;
                }
                else
                {
                    TAGENTRYW rgteBodyEnd[] = {
                                                { L">", TRUE, },
                                              };
                    int iOffsetBodyEnd = _ScanTagEntriesW(_pReadFileObjHtmlBkgd, iOffsetBodyStart, rgteBodyEnd, ARRAYSIZE(rgteBodyEnd));
                    if (iOffsetBodyEnd == -1)
                    {   // An error in the HTML.
                        iOffsetBodyEnd = iOffsetBodyStart;  // BUGBUG: We need a better recovery idea.
                    }
                    _WriteHtmlFromReadFileObj(_pReadFileObjHtmlBkgd, iOffsetBase, iOffsetBodyEnd);
                    iOffsetBase = iOffsetBodyEnd;
                }
            }
            // Insert the DIV clause
            if(ema.iMonitors > 1)
            {
                int         iIndexPrimaryMonitor;
                HMONITOR    hMonitorPrimary;
                MONITORINFO monitorInfo;

                // 99/03/23 #275429 vtan: We used GetViewAreas() to fill in rcViewAreas above.
                // The code here used to assume that [0] ALWAYS referred to the primary monitor.
                // This isn't the case if the monitor settings are changed without a restart.
                // In order to compensate for this and always render the wallpaper into the
                // primary monitor, a search is performed to find a (left, top) that matches
                // one of the work areas and this is used as the primary monitor. If none can
                // be found then default to the old algorithm.

                hMonitorPrimary = GetPrimaryMonitor();
                monitorInfo.cbSize = sizeof(monitorInfo);
                TBOOL(GetMonitorInfo(hMonitorPrimary, &monitorInfo));
                iIndexPrimaryMonitor = -1;
                for (int i = 0; (iIndexPrimaryMonitor < 0) && (i < nViewAreas); ++i)
                {
                    if ((monitorInfo.rcWork.left == rcViewAreas[i].left) && (monitorInfo.rcWork.top == rcViewAreas[i].top))
                    {
                        iIndexPrimaryMonitor = i;
                    }
                }
                if (iIndexPrimaryMonitor < 0)
                    iIndexPrimaryMonitor = 0;
                if ((nViewAreas <= 0) || (rcViewAreas[iIndexPrimaryMonitor].right == rcViewAreas[iIndexPrimaryMonitor].left))
                // The second case could occur on bootup
                {
                    // Some error occured when getting the ViewAreas. Recover from the error by using the workarea.
                    // Get the workarea of the primary monitor, since HTML wallpapers are displayed only there.
                    GetMonitorWorkArea(hMonitorPrimary, &rcViewAreas[iIndexPrimaryMonitor]);
                }
                _WriteHtmlFromIdF(IDS_DIV_START3,
                                  rcViewAreas[iIndexPrimaryMonitor].left - ema.rcVirtualMonitor.left,
                                  rcViewAreas[iIndexPrimaryMonitor].top - ema.rcVirtualMonitor.top,
                                  rcViewAreas[iIndexPrimaryMonitor].right - rcViewAreas[iIndexPrimaryMonitor].left,
                                  rcViewAreas[iIndexPrimaryMonitor].bottom - rcViewAreas[iIndexPrimaryMonitor].top);
            }

            //
            // Write out HTML from after <HEAD> tag to just before </BODY> tag.
            //
            _WriteHtmlFromReadFileObj(_pReadFileObjHtmlBkgd, iOffsetBase, iOffsetComponents);

            if(ema.iMonitors > 1)
            {
                _WriteHtmlFromId(IDS_DIV_END);
            }
        }
        else
        {
            if(_pReadFileObjHtmlBkgd)
                delete _pReadFileObjHtmlBkgd;
            _pReadFileObjHtmlBkgd = NULL;
        }
    }

    EXITPROC(2, "DS GenerateHtmlHeader!");
}

void CActiveDesktop::_WriteResizeable(COMPONENTA *pcomp)
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

void CActiveDesktop::_WriteHtmlW(LPCWSTR wcBuf, UINT cchToWrite, UINT *pcchWritten)
{
    ULONG    cchWritten = 0;
    UINT     uiSize;
    
    if(_pStream)
    {
        uiSize = sizeof(WCHAR);
        _pStream->Write((LPVOID)wcBuf, cchToWrite * uiSize, &cchWritten);
    }
    else
    {
        ASSERT(_hFileHtml);
        uiSize = (_iDestFileCharset == ANSI_HTML_CHARSET) ? sizeof(char) : sizeof(WCHAR);
        WriteFile(_hFileHtml, (LPCVOID)wcBuf, cchToWrite * uiSize, &cchWritten, NULL);
    }
    *pcchWritten = (UINT)(cchWritten/uiSize);  //Convert to number of chars.
}

void CActiveDesktop::_GenerateHtmlPicture(COMPONENTA *pcomp)
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

void CActiveDesktop::_GenerateHtmlDoc(COMPONENTA *pcomp)
{
    ENTERPROC(2, "DS GenerateHtmlDoc(pcomp=%08X)");
    
    TCHAR   szUrl[INTERNET_MAX_URL_LENGTH];
    DWORD   dwSize = ARRAYSIZE(szUrl);
    LPTSTR  lpszUrl = szUrl;

    if(FAILED(UrlCreateFromPath(pcomp->szSource, szUrl, &dwSize, 0)))
        lpszUrl = pcomp->szSource;

    //
    // Write out the DIV header HTML.
    //
    _WriteHtmlFromIdF(IDS_DIV_START2, pcomp->dwID, lpszUrl);

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

void CActiveDesktop::_GenerateHtmlSite(COMPONENTA *pcomp)
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

    DWORD   currentURLLength, maximumURLLength;
    TCHAR   *pURL, formatBuffer[0x0100];

//  98/09/29 #211384 vtan: There is a limitation in wvsprintf.
//  It only allows 2048 bytes in its buffer. If the URL is
//  longer than 1024 characters less the IDS_IFRAME_BEGIN2
//  string length less the component ID less "scrolling=no"
//  if the component cannot be scrolled then the URL string
//  will not be correctly inserted into the IDS_IFRAME_BEGIN2
//  string and there will be a missing end-quote and trident
//  will fail to render desktop.htt correctly.

//  To correct against this the followING limits the length of
//  the URL to this maximum and truncates any characters
//  beyond the limit so that the IDS_IFRAME_BEGIN2 string
//  contains its end-quote and trident does not barf.

//  The above condition is a boundary condition and this
//  check is quick so that the calculations that follow do
//  not have to be executed repeatedly.

    currentURLLength = lstrlen(szUrl);
    if (currentURLLength > 768)                                 // a hard-coded limit
    {
        maximumURLLength = 1024;                                // wvsprintf limit
        LoadString(HINST_THISDLL, IDS_IFRAME_BEGIN2, formatBuffer, ARRAYSIZE(formatBuffer));
        maximumURLLength -= lstrlen(formatBuffer);              // IDS_IFRAME_BEGIN2
        maximumURLLength -= 16;                                 // pcomp->dwID
        maximumURLLength -= lstrlen(TEXT("scrolling=no"));      // pcomp->fNoScroll
        if (currentURLLength > maximumURLLength)
            szUrl[maximumURLLength] = static_cast<TCHAR>('\0');
    }
    _WriteHtmlFromIdF(IDS_IFRAME_BEGIN2, pcomp->dwID, szUrl, pcomp->fNoScroll ? TEXT("scrolling=no") : c_szNULL);

    //
    // Write out whether this Component is resizeable or not!
    //
    _WriteResizeable(pcomp);

//  98/09/29 #211384 vtan: See above.

    currentURLLength = lstrlen(pcomp->szSubscribedURL);
    if (currentURLLength > 768)
    {
        lstrcpy(szUrl, pcomp->szSubscribedURL);
        maximumURLLength = 1024;
        LoadString(HINST_THISDLL, IDS_SUBSCRIBEDURL, formatBuffer, ARRAYSIZE(formatBuffer));
        maximumURLLength -= lstrlen(formatBuffer);              // IDS_SUBSCRIBEDURL
        if (currentURLLength > maximumURLLength)
            szUrl[maximumURLLength] = static_cast<TCHAR>('\0');
        pURL = szUrl;
    }
    else
        pURL = pcomp->szSubscribedURL;
    //
    // Write out the URL that must be used for subscription purposes.
    //
    _WriteHtmlFromIdF(IDS_SUBSCRIBEDURL, pURL);

    //
    // Write out the frame location HTML.
    //
    _WriteHtmlFromIdF(IDS_IFRAME_SIZE, _fSingleItem ? 0 : pcomp->cpPos.iLeft, _fSingleItem ? 0 : pcomp->cpPos.iTop,
        pcomp->cpPos.dwWidth, pcomp->cpPos.dwHeight, pcomp->cpPos.izIndex);

    EXITPROC(2, "DS GenerateHtmlSite!");
}

void CActiveDesktop::_GenerateHtmlControl(COMPONENTA *pcomp)
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

void CActiveDesktop::_GenerateHtmlComponent(COMPONENTA *pcomp)
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

void CActiveDesktop::_GenerateHtmlFooter(void)
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
    if (_pReadFileObjHtmlBkgd)
    {
        if(_fNeedBodyEnd)
        {    // We had introduced the <BODY> tag by ourselves.
            _WriteHtmlFromId(IDS_BODY_END2);
            _fNeedBodyEnd = FALSE;
        }
        _WriteHtmlFromReadFileObj(_pReadFileObjHtmlBkgd, -1, -1);
        delete _pReadFileObjHtmlBkgd;   //Close the file and cleanup!
        _pReadFileObjHtmlBkgd = NULL;
    }
    else
    {
        _WriteHtmlFromId(IDS_BODY_END);
    }

    EXITPROC(2, "DS GenerateHtmlFooter!");
}

void CActiveDesktop::_GenerateHtml(void)
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

        if (_co.fEnableComponents && _hdsaComponent && DSA_GetItemCount(_hdsaComponent) && !SHRestricted(REST_NODESKCOMP))
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
        SetDesktopFlags(COMPONENTS_DIRTY, 0);
    }
    else
    {

        // 99/05/19 #340772 vtan: If unable to open desktop.htt it's probably
        // in use by another process or task (perhaps trident is trying to
        // render it). In this case mark it dirty so that it will get recreated
        // - yet again but this time with more current data.

        SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);
    }

    EXITPROC(2, "DS GenerateHtml!");
}

HRESULT CActiveDesktop::GenerateDesktopItemHtml(LPCWSTR pwszFileName, COMPONENT *pcomp, DWORD dwReserved)
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

HRESULT CActiveDesktop::AddUrl(HWND hwnd, LPCWSTR pszSourceW, LPCOMPONENT pcomp, DWORD dwFlags)
{
    LPTSTR pszExt;
    HRESULT fOkay = TRUE;
    BOOL fExtIsCdf,fPathIsUrl;
    BOOL fSubscribed = FALSE;
    COMPONENT   compLocal;
    COMPONENTA  compA;
    TCHAR szSource[INTERNET_MAX_URL_LENGTH];

//  98/08/28 vtan #202777: The following if statement sanitizes parameters
//  passed to AddUrl(). The statements following the "||" are executed
//  despite the for pcomp against NULL. This causes an access violation
//  and an exception to be thrown.

#if     0
    //Check for the input parameters.
    if(!pszSourceW || (pcomp &&
       ((pcomp->dwSize != SIZEOF(*pcomp)) && (pcomp->dwSize != SIZEOF(IE4COMPONENT))) ||
       ((pcomp->dwSize == SIZEOF(*pcomp)) && !VALIDATESTATE(pcomp->dwCurItemState))))
        return E_INVALIDARG;
#else

//  The following performs the same comparison but is spread into three
//  separate comparisons. As performance is not a critical issue here
//  but correctness is this makes the tests clear and understandable.
//  The invalid conditions are described.

//  Validate input parameters. Invalid parameters are:
//      1) NULL pszSourceW
//      2) pcomp->dwSize for a COMPONENT struct but invalid pcomp->dwCurItemState
//      3) pcomp->dwSize is not for a COMPONENT struct nor for a IE4COMPONENT struct

    if (pszSourceW == NULL)
        return(E_INVALIDARG);
    if (pcomp != NULL)
    {
        if ((pcomp->dwSize == sizeof(*pcomp)) && !VALIDATESTATE(pcomp->dwCurItemState))
            return(E_INVALIDARG);
        if ((pcomp->dwSize != sizeof(*pcomp)) && (pcomp->dwSize != sizeof(IE4COMPONENT)))
            return(E_INVALIDARG);
    }
#endif

    // Catch folks that call our API's to add components and prevent them from doing
    // so if the restriction is in place.
    if (SHIsRestricted(NULL, REST_NOADDDESKCOMP))
        return E_ACCESSDENIED;

    if (!pcomp)
    {
        pcomp = &compLocal;
        pcomp->dwSize = sizeof(compLocal);
        pcomp->dwCurItemState = IS_NORMAL;
    }

    // Attempt to come up with a reasonable window handle if none is passed in.  ParseDesktopComponent
    // will fail to attempt to create a subscription if a NULL window handle is passed in.
    if (!hwnd)
        hwnd = GetLastActivePopup(GetActiveWindow());

    compA.dwSize = sizeof(compA);
    compA.dwCurItemState = (pcomp->dwSize != SIZEOF(IE4COMPONENT)) ? pcomp->dwCurItemState : IS_NORMAL;

    g_pActiveDesk = this;
    SHUnicodeToTChar(pszSourceW, szSource, ARRAYSIZE(szSource));
    pszExt = PathFindExtension(szSource);
    fExtIsCdf = lstrcmpi(pszExt, TEXT(".CDF")) == 0;
    fPathIsUrl = PathIsURL(szSource) && !UrlIsFileUrl(szSource);


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
            LoadString(HINST_THISDLL, IDS_COMP_EXISTS, szMsg, ARRAYSIZE(szMsg));
            LoadString(HINST_THISDLL, IDS_COMP_EXISTS_2, szMsg2, ARRAYSIZE(szMsg2));
            lstrcat(szMsg, szMsg2);
            LoadString(HINST_THISDLL, IDS_COMP_TITLE, szTitle, ARRAYSIZE(szTitle));
            MessageBox(hwnd, szMsg, szTitle, MB_OK);

            fOkay = FALSE;
        }
    }

    if (fOkay && CheckForExistingSubscription(szSource))
    {
        if ((dwFlags & ADDURL_SILENT) ||
            (ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_COMP_SUBSCRIBED), 
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

            HRESULT hr;
            IProgressDialog * pProgressDlg = NULL;
            DECLAREWAITCURSOR;

//  98/12/16 vtan #250938: Cannot add new components that are not
//  local with ICW run to completion. Tell the user and launch ICW.

            if (!IsICWCompleted())
            {
                if ((dwFlags & ADDURL_SILENT) == 0)
                {
                    ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_COMP_ICW_ADD), MAKEINTRESOURCE(IDS_COMP_ICW_TITLE), MB_OK);
                    LaunchICW();
                }
                fOkay = FALSE;
            }
            else
            {
                SetWaitCursor();
                // ParseDesktopComponent can hang for a long time, we need some sort of progress
                // UI up before we call it.
                if (!(dwFlags & ADDURL_SILENT) && !fExtIsCdf)
                {
                    if (pProgressDlg = CProgressDialog_CreateInstance(IDS_COMP_TITLE, IDA_ISEARCH, g_hinst))
                    {
                        WCHAR szConnectingW[80];
                        TCHAR szConnecting[80];
                        LoadString(HINST_THISDLL, IDS_CONNECTING, szConnecting, ARRAYSIZE(szConnecting));
                        SHTCharToUnicode(szConnecting, szConnectingW, ARRAYSIZE(szConnectingW));
                        pProgressDlg->SetLine(1, szConnectingW, FALSE, NULL);
                        pProgressDlg->SetLine(2, szUrlW, TRUE, NULL);
                        pProgressDlg->StartProgressDialog(hwnd, NULL, PROGDLG_AUTOTIME | PROGDLG_NOPROGRESSBAR, NULL);
                    }
                }

                hr = ParseDesktopComponent(hwnd, szUrlW, pcomp);

                if (pProgressDlg)
                {
                    pProgressDlg->StopProgressDialog();
                    fOkay = !pProgressDlg->HasUserCancelled();  //  User may have cancelled the progress dialog
                    pProgressDlg->Release();
                }
                ResetWaitCursor();

                if (hr == S_FALSE) // User cancelled operation via subscription download dialog
                    fOkay = FALSE;

                if (fOkay)
                {
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Convert ed's wide thinggy to multi.
                        //
                        WideCompToMultiComp(pcomp, &compA);
    
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
                            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_COMP_BADURL), 
                                            MAKEINTRESOURCE(IDS_COMP_TITLE), MB_OK);
                        }
                        fOkay = FALSE;
                    }
                }
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
                //We need to zero init this structure except the cbSize field.
                SUBSCRIPTIONINFO siDefault = {sizeof(SUBSCRIPTIONINFO)};

                SHTCharToUnicode(szSource, wzURL, ARRAYSIZE(wzURL));

                //This field is already initialized above.
                //siDefault.cbSize = sizeof(siDefault);
                psm->CreateSubscription(hwnd, wzURL, wzURL, CREATESUBS_NOUI, SUBSTYPE_DESKTOPURL, &siDefault);
                psm->UpdateSubscription(wzURL);
                psm->Release();
            }
        }
        else
        {
            HRESULT hres = CreateSubscriptionsWizard(SUBSTYPE_DESKTOPURL, szSource, NULL, hwnd);
            if(!SUCCEEDED(hres))  //Some error, or the user chose Cancel - we should fail.
            {
                ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_COMP_BADSUBSCRIBE), 
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

void CActiveDesktop::_SaveSettings(DWORD dwFlags)
{
    ENTERPROC(2, "DS SaveSettings()");

    if (dwFlags & AD_APPLY_SAVE)
    {
        // Don't ever modify the safemode settings
        TCHAR lpszDeskcomp[MAX_PATH];
        GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_GENERAL, _pszScheme);
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
        SHELLSTATE ss = {0};

        SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE);
        BOOL fWasActiveDesktop = BOOLIFY(ss.fDesktopHTML);
        BOOL fIsActiveDesktop = BOOLIFY(_co.fActiveDesktop);

        if (fIsActiveDesktop && !IsICWCompleted())
            TBOOL(DisableUndisplayableComponents(this));
        if (fIsActiveDesktop != fWasActiveDesktop)
        {
            SendMessage(hwndShell, WM_WININICHANGE, SPI_SETDESKWALLPAPER, (LPARAM)TEXT("ToggleDesktop"));
            //Force a SHRefresh with this dummy call
            SHGetSetSettings(NULL, 0, TRUE);
        }
        else if (fIsActiveDesktop)
        {
            //See if we can simply make the changes dynamically instead of refreshing the whole page

//  98/09/22 #182982 vtan: Use dynamic HTML to refresh only if specifically told by a flag.

            if(_fUseDynamicHtml && (dwFlags & AD_APPLY_DYNAMICREFRESH))
            {
                SendMessage(hwndShell, DTM_MAKEHTMLCHANGES, (WPARAM)0, (LPARAM)0L);
            }
            else
            {
                //Can't use dynamic html. We have to refresh the whole page.
                SendMessage(hwndShell, WM_WININICHANGE, SPI_SETDESKWALLPAPER, 
                    (LPARAM)((dwFlags & AD_APPLY_BUFFERED_REFRESH) ? c_szBufferedRefresh : c_szRefreshDesktop));
            }
        }

        _fUseDynamicHtml = TRUE;
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

ULONG CActiveDesktop::AddRef(void)
{
    ENTERPROC(1, "DS AddRef()");

    _cRef++;

    EXITPROC(1, "DS AddRef=%d", _cRef);
    return _cRef;
}

HRESULT CActiveDesktop::ApplyChanges(DWORD dwFlags)
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

ULONG CActiveDesktop::Release(void)
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

CActiveDesktop::CActiveDesktop()
{
    _cRef = 1;
    _fNoDeskMovr = FALSE;
    _fBackgroundHtml = FALSE;
    _fUseDynamicHtml = TRUE;
    DllAddRef();
}

CActiveDesktop::~CActiveDesktop()
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

HRESULT CActiveDesktop::GetWallpaper(LPWSTR pwszWallpaper, UINT cchWallpaper, DWORD dwReserved)
{
    HRESULT hres = E_INVALIDARG;
    ENTERPROC(1, "DS GetWallpaper(pszWallpaper=%08X,cchWallpaper=%d)", pwszWallpaper, cchWallpaper);

    ASSERT(!dwReserved);     // These should be 0

    if (pwszWallpaper && cchWallpaper)
    {
        SHTCharToUnicode(_szSelectedWallpaper, pwszWallpaper, cchWallpaper);
        hres = S_OK;
    }
    else
    {
        TraceMsg(TF_WARNING, "DS GetWallpaper unable to return wallpaper");
    }

    EXITPROC(1, "DS GetWallpaper=%d", hres);
    return hres;
}

HRESULT CActiveDesktop::SetWallpaper(LPCWSTR pwszWallpaper, DWORD dwReserved)
{
    HRESULT hres = E_INVALIDARG;
    LPCTSTR pszWallpaper;

    ASSERT(!dwReserved);     // These should be 0

    if(_fPolicyForWPName)    // If a policy exists, the caller can not change the wallpaper.
        return S_FALSE;  
        
    if (pwszWallpaper)
    {
#ifdef UNICODE
        pszWallpaper = pwszWallpaper;
#else
        TCHAR szWallpaper[INTERNET_MAX_URL_LENGTH];
        SHUnicodeToTChar(pwszWallpaper, szWallpaper, ARRAYSIZE(szWallpaper));
        pszWallpaper = szWallpaper;
#endif

        if (lstrcmp(_szSelectedWallpaper, pszWallpaper) != 0)
        {
            lstrcpyn(_szSelectedWallpaper, pszWallpaper, ARRAYSIZE(_szSelectedWallpaper));
            _fWallpaperDirty = TRUE;
            _fDirty = TRUE;
            _fUseDynamicHtml = FALSE;  //Setting wallpaper causes a lot of change; So, can't use dynamic html
        }
        hres = S_OK;
    }

    ENTERPROC(1, "DS SetWallpaper(pszWallpaper=>%s<)", pwszWallpaper ? pszWallpaper : TEXT("(NULL)"));

    EXITPROC(1, "DS SetWallpaper=%d", hres);
    return hres;
}

HRESULT CActiveDesktop::GetWallpaperOptions(WALLPAPEROPT *pwpo, DWORD dwReserved)
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

HRESULT CActiveDesktop::SetWallpaperOptions(LPCWALLPAPEROPT pwpo, DWORD dwReserved)
{
    HRESULT hres = E_INVALIDARG;
    ENTERPROC(1, "DS SetWallpaperOptions(pwpo=%08X)", pwpo);

    ASSERT(!dwReserved);     // These should be 0

    if(_fPolicyForWPStyle)  //If a policy exists for wallpaper style, the caller can not change it.
        return S_FALSE;
        

    if ((pwpo) && (pwpo->dwSize == SIZEOF(*pwpo)))
    {
        _wpo = *pwpo;
        _fWallpaperDirty = TRUE;
        _fDirty = TRUE;
        _fUseDynamicHtml = FALSE; //Changing wallpaper options causes us to regenerate the whole thing.
        hres = S_OK;
    }
    else
    {
        TraceMsg(TF_WARNING, "DS SetWallpaperOptions could not set options");
    }

    EXITPROC(1, "DS SetWallpaperOptions=%d", hres);
    return hres;
}

HRESULT CActiveDesktop::GetPattern(LPWSTR pwszPattern, UINT cchPattern, DWORD dwReserved)
{
    HRESULT hres = S_OK;
    ENTERPROC(1, "DS GetPattern(psz=%08X,cch=%d)", pwszPattern, cchPattern);

    ASSERT(!dwReserved);     // These should be 0

    if(!pwszPattern || (cchPattern == 0))
        return (E_INVALIDARG);

    SHTCharToUnicode(_szSelectedPattern, pwszPattern, cchPattern);

    EXITPROC(1, "DS GetPattern=%d", hres);
    return hres;
}

HRESULT CActiveDesktop::SetPattern(LPCWSTR pwszPattern, DWORD dwReserved)
{
    HRESULT hres = E_INVALIDARG;
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
            _fUseDynamicHtml = FALSE; //Setting pattern causes us to regenerate the whole thing.

            hres = S_OK;
        }
        else
            hres = E_FAIL;
    }

    ENTERPROC(1, "DS SetPattern(psz=>%s<)", pwszPattern ? pszPattern : TEXT("(NULL)"));

    EXITPROC(1, "DS SetPattern=%d", hres);
    return hres;
}

HRESULT CActiveDesktop::GetDesktopItemOptions(COMPONENTSOPT *pco, DWORD dwReserved)
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

HRESULT CActiveDesktop::SetDesktopItemOptions(LPCCOMPONENTSOPT pco, DWORD dwReserved)
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


HRESULT CActiveDesktop::_AddDTIWithUIPrivateA(HWND hwnd, LPCCOMPONENT pComp, DWORD dwFlags)
{
    HRESULT hres = E_FAIL;
    TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
    int nScheme;
    DWORD dwCurItemState;

    SHUnicodeToTChar(pComp->wszSource, szUrl, ARRAYSIZE(szUrl));

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
    if (pComp->dwSize == SIZEOF(IE4COMPONENT))
        dwCurItemState = IS_NORMAL;
    else
        dwCurItemState = pComp->dwCurItemState;
    hres = AddDesktopComponentNoUI(AD_APPLY_ALL | AD_APPLY_DYNAMICREFRESH, szUrl, NULL, pComp->iComponentType, 
        pComp->cpPos.iLeft, pComp->cpPos.iTop, pComp->cpPos.dwWidth, pComp->cpPos.dwHeight, TRUE, dwCurItemState) ? S_OK : E_FAIL;

    return hres;
}



#define     STC_DESKTOPCOMPONENT    0x00000002
EXTERN_C STDAPI_(HRESULT) SubscribeToCDF(HWND hwndParent, LPCWSTR pwzUrl, DWORD dwCDFTypes);

HRESULT CActiveDesktop::AddDesktopItemWithUI(HWND hwnd, LPCOMPONENT pComp, DWORD dwFlags)
{
    HRESULT hres = E_FAIL;

    // We need to support IE4 apps calling with the old component structure too!
    // We use the size field to detect IE4 v/s newer apps!
    if(!pComp ||
       ((pComp->dwSize != SIZEOF(*pComp)) && (pComp->dwSize != SIZEOF(IE4COMPONENT))) ||
       ((pComp->dwSize == SIZEOF(*pComp)) && !VALIDATESTATE(pComp->dwCurItemState)) ||
       ((pComp->iComponentType < 0) || (pComp->iComponentType > COMP_TYPE_MAX)))  //Validate the component type
        return E_INVALIDARG;

    // Catch folks that call our API's to add components and prevent them from doing
    // so if the restriction is in place.
    if (SHIsRestricted(NULL, REST_NOADDDESKCOMP))
        return E_ACCESSDENIED;

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
            LoadString(HINST_THISDLL, IDS_COMP_EXISTS, szMsg, ARRAYSIZE(szMsg));
            LoadString(HINST_THISDLL, IDS_COMP_EXISTS_2, szMsg2, ARRAYSIZE(szMsg2));
            lstrcat(szMsg, szMsg2);
            LoadString(HINST_THISDLL, IDS_COMP_TITLE, szTitle, ARRAYSIZE(szTitle));
            MessageBox(hwnd, szMsg, szTitle, MB_OK);

            return E_FAIL;

#if 0

            comp.dwSize = SIZEOF(comp);

            //Prompt the user to reinstall the ADI.
            if(ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_CONFIRM_ADI_REINSTALL), 
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
            if(ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_CONFIRM_ADD), 
                                MAKEINTRESOURCE(IDS_INTERNET_EXPLORER), MB_YESNO) != IDYES)
            {
                return E_FAIL; //User choses not to install this desktop component!
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
            ApplyChanges(AD_APPLY_ALL | AD_APPLY_DYNAMICREFRESH);
        }
        break;

        case E_ACCESSDENIED:
            // The file was a CDF but didn't contain Desktop Component Information
            if (hwnd)
            {
                TCHAR szMsg[MAX_PATH];
                TCHAR szTitle[MAX_PATH];

                LoadString(HINST_THISDLL, IDS_ADDCOMP_ERROR_CDFNODTI, szMsg, ARRAYSIZE(szMsg));
                LoadString(HINST_THISDLL, IDS_INTERNET_EXPLORER, szTitle, ARRAYSIZE(szTitle));
                MessageBox(hwnd, szMsg, szTitle, MB_OK);
            }
            break;
        case E_UNEXPECTED:      
            // This was a CDF but it was misauthored.
            if (hwnd)
            {
                TCHAR szMsg[MAX_PATH];
                TCHAR szTitle[MAX_PATH];

                LoadString(HINST_THISDLL, IDS_ADDCOMP_ERROR_CDFINALID, szMsg, ARRAYSIZE(szMsg));
                LoadString(HINST_THISDLL, IDS_INTERNET_EXPLORER, szTitle, ARRAYSIZE(szTitle));
                MessageBox(hwnd, szMsg, szTitle, MB_OK);
            }
            break;
        default:
            break;
    }

    if(hwnd && SUCCEEDED(hres))
    {
        // If the active desktop is currently OFF, we need to turn it ON
        SHELLSTATE ss = {0};
        SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE);
        if(!ss.fDesktopHTML)
        {
            COMPONENTSOPT co;
            co.dwSize = sizeof(COMPONENTSOPT);
            GetDesktopItemOptions(&co, 0);
            co.fActiveDesktop = TRUE;
            SetDesktopItemOptions(&co, 0);
            ApplyChanges(AD_APPLY_REFRESH | AD_APPLY_DYNAMICREFRESH);
        }
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

void RestoreComponent(HDSA hdsaComponents, COMPONENTA * pcomp)
{
    int i;

    // If we are split then set the bit saying that the listview needs to be adjusted.  This is done
    // when we check the state of desktop.htm in EnsureUpdateHtml.
    if (pcomp->dwCurItemState & IS_SPLIT)
    {
        pcomp->dwCurItemState |= IS_ADJUSTLISTVIEW;
        SetDesktopFlags(COMPONENTS_ZOOMDIRTY, COMPONENTS_ZOOMDIRTY);
    }

    for (i = 0; i < DSA_GetItemCount(hdsaComponents); i++)
    {
        COMPONENTA * pcompT;
    
        if (pcompT = (COMPONENTA *)DSA_GetItemPtr(hdsaComponents, i))
        {
            // If this component is split/fullscreen and is different from the source component
            // but it is at the same location then it must be on this monitor (work area) so restore it.
            if (ISZOOMED(pcompT) &&
                lstrcmpi(pcomp->szSource, pcompT->szSource) &&
                (pcomp->cpPos.iTop  == pcompT->cpPos.iTop) &&
                ((pcomp->cpPos.iLeft + pcomp->cpPos.dwWidth) == (pcompT->cpPos.iLeft + pcompT->cpPos.dwWidth)))
                {
                    pcompT->dwCurItemState = pcompT->csiRestored.dwItemState;
                    pcompT->cpPos.iLeft = pcompT->csiRestored.iLeft;
                    pcompT->cpPos.iTop = pcompT->csiRestored.iTop;
                    pcompT->cpPos.dwWidth = pcompT->csiRestored.dwWidth;
                    pcompT->cpPos.dwHeight = pcompT->csiRestored.dwHeight;
                    pcompT->cpPos.izIndex = COMPONENT_TOP;
                    pcompT->fDirty = TRUE;
                    break;
                }
        }
    }
}

HRESULT CActiveDesktop::AddDesktopItem(LPCCOMPONENT pComp, DWORD dwReserved)
{
    HRESULT     hres = E_FAIL;
    COMPONENTA  CompA;
    CompA.dwSize = SIZEOF(CompA);

    ASSERT(!dwReserved);     // These should be 0

    // We need to support IE4 apps calling with the old component structure too!
    // We use the size field to detect IE4 v/s newer apps!
    if(!pComp ||
       ((pComp->dwSize != SIZEOF(*pComp)) && (pComp->dwSize != SIZEOF(IE4COMPONENT))) ||
       ((pComp->dwSize == SIZEOF(*pComp)) && !VALIDATESTATE(pComp->dwCurItemState)))
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

    PositionComponent(&CompA, &CompA.cpPos, CompA.iComponentType, TRUE);

    if (_hdsaComponent && ISZOOMED(&CompA))
        RestoreComponent(_hdsaComponent, &CompA);

    //Make sure the this component's fDirty flag is off.
    CompA.fDirty = FALSE;

    // Set the dummy bit here - this forces folks to do bitwise testing on the dwCurItemState field
    // instead of testing for equality.  This will allow us to expand use of the field down the
    // road without compatibility problems.
    CompA.dwCurItemState |= IS_INTERNALDUMMYBIT;

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

BOOL CActiveDesktop::AddComponentPrivate(COMPONENTA *pcomp, DWORD_PTR dwID)
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
int CActiveDesktop::_FindComponentBySource(LPTSTR lpszSource, COMPONENTA *pComp)
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

int CActiveDesktop::_FindComponentIndexByID(DWORD_PTR dwID)
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


//
// This function is to be used only in special situations. Given a Url, it finds a component
// that has the src= pointed to that url. Note that what we have is szSource is something like
// "c:\foo\bar.bmp"; But, what is passed to this function is "file://c:/foo/bar.htm"
//
// Warning: This function does a conversion from Path to Url for every component before
// comparing with the given Url. This is in-efficient. We do it this way because converting
// the given Url ,which was converted to url from path, back to Path may not result in the 
// original path. In other words a round-trip from path to Url and back to path may not result
// in the path that was originally entered by the end-user.
//
int CActiveDesktop::_FindComponentBySrcUrl(LPTSTR lpszSrcUrl, COMPONENTA *pComp)
{
    int iRet = -1;
    ENTERPROC(2, "DS FindComponentBySrcUrl(pComp=%8X)", pComp);

    if (_hdsaComponent)
    {
        int i;

        for (i=0; i<DSA_GetItemCount(_hdsaComponent); i++)
        {
            COMPONENTA comp;
            comp.dwSize = sizeof(COMPONENTA);

            if (DSA_GetItem(_hdsaComponent, i, &comp) != -1)
            {
                TCHAR   szUrl[INTERNET_MAX_URL_LENGTH];
                LPTSTR  lpszUrl = szUrl;
                DWORD   dwSize;

                //Convert the szSource to Url
                dwSize = ARRAYSIZE(szUrl);
                
                if(FAILED(UrlCreateFromPath(comp.szSource, lpszUrl, &dwSize, 0)))
                    lpszUrl = comp.szSource;
                    
                if (!lstrcmpi(lpszUrl, lpszSrcUrl))
                {
                    *pComp = comp;
                    iRet = i;
                    break;
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "DS FindComponentBySrcUrl unable to get a component");
            }
        }
    }

    EXITPROC(2, "DS FindComponentBySrcUrl=%d", iRet);
    return iRet;
}

HRESULT CActiveDesktop:: GetDesktopItemByID(ULONG_PTR dwID, COMPONENT *pcomp, DWORD dwReserved)
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

HRESULT CActiveDesktop::RemoveDesktopItem(LPCCOMPONENT pComp, DWORD dwReserved)
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

BOOL CActiveDesktop::RemoveComponentPrivate(int iIndex, COMPONENTA *pcomp)
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

HRESULT CActiveDesktop::_CopyComponent(COMPONENTA *pCompDest, COMPONENTA *pCompSrc, DWORD dwFlags)
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
    if(dwFlags & COMP_ELEM_ORIGINAL_CSI)
        pCompDest->csiOriginal = pCompSrc->csiOriginal;
    if(dwFlags & COMP_ELEM_RESTORED_CSI)
    {
        pCompDest->csiRestored = pCompSrc->csiRestored;

//  98/08/21 vtan #174542: When changing csiRestored using the Active
//  Desktop API and the component is zoomed the csiRestored information
//  needs to be copied to the cpPos field as well as this is where the
//  actual information is stored when the component is restored. This
//  is only applicable to the case when the component is zoomed.

        if (ISZOOMED(pCompDest))
        {
            pCompDest->cpPos.iLeft = pCompSrc->csiRestored.iLeft;
            pCompDest->cpPos.iTop = pCompSrc->csiRestored.iTop;
            pCompDest->cpPos.dwWidth = pCompSrc->csiRestored.dwWidth;
            pCompDest->cpPos.dwHeight = pCompSrc->csiRestored.dwHeight;
        }
    }
    if(dwFlags & COMP_ELEM_CURITEMSTATE)  // Only allow the modification of the public bits - propagate the internal bits unchanged.
        pCompDest->dwCurItemState = (pCompDest->dwCurItemState & IS_VALIDINTERNALBITS) | (pCompSrc->dwCurItemState & ~IS_VALIDINTERNALBITS);

    return(S_OK);
}

HRESULT CActiveDesktop::GetDesktopItemBySource(LPCWSTR lpcwszSource, LPCOMPONENT pComp, DWORD dwFlags)
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

HRESULT CActiveDesktop::ModifyDesktopItem(LPCCOMPONENT pComp, DWORD dwFlags)
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

        if (dwFlags & (COMP_ELEM_POS_LEFT | COMP_ELEM_POS_TOP | COMP_ELEM_SIZE_WIDTH | COMP_ELEM_SIZE_HEIGHT | COMP_ELEM_CHECKED | COMP_ELEM_CURITEMSTATE))
            PositionComponent(&CompNew, &CompNew.cpPos, CompNew.iComponentType, FALSE);
        if (ISZOOMED(&CompNew))
            RestoreComponent(_hdsaComponent, &CompNew);

        CompNew.fDirty = TRUE; //Since the component is modified, we set the dirty bit!
        if(UpdateComponentPrivate(iIndex, &CompNew))
            hres = S_OK;
    }

    return(hres);
}

BOOL CActiveDesktop::UpdateComponentPrivate(int iIndex, COMPONENTA *pcomp)
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

HRESULT CActiveDesktop::GetDesktopItemCount(LPINT lpiCount, DWORD dwReserved)
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

HRESULT CActiveDesktop::GetDesktopItem(int nComponent, COMPONENT *pComp, DWORD dwReserved)
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

BOOL CActiveDesktop::GetComponentPrivate(int nComponent, COMPONENTA *pcomp)
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

#define MAX_INDENTATION_VALUE    0x20

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

        // We don't want this value to get out of hand because of 
        // unmatched Enter and Exit calls in functions (which will 
        // trash the stack).
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
        //This can happen if the Enter and Exit procs are unmatched.
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
HRESULT CActiveDesktop::SetScheme(LPCWSTR pwszSchemeName, DWORD dwFlags)
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
            dwUpdateFlags |= (AD_APPLY_REFRESH | AD_APPLY_DYNAMICREFRESH);
        _Initialize();
        _fUseDynamicHtml=FALSE;  
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
HRESULT CActiveDesktop::GetScheme(LPWSTR pwszSchemeName, LPDWORD lpdwcchBuffer, DWORD dwFlags)
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

        SHTCharToUnicode(_pszScheme, pwszSchemeName, *lpdwcchBuffer);
        
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
HRESULT CActiveDesktop::UpdateAllDesktopSubscriptions()
{
    ::UpdateAllDesktopSubscriptions();
    return S_OK;
}

void CActiveDesktop::_GenerateHtmlBStrForComp(COMPONENTA *pComp, BSTR *pbstr)
{
    ENTERPROC(2, "DS _GenerateHtmlBstrForComp");
    
    if(_pStream = SHCreateMemStream(NULL, 0)) //Create a mem stream.
    {
        LARGE_INTEGER libMove = {0};
        ULARGE_INTEGER libCurPos;
        // Since _pStream is setup, the following call will generate the component HTML into
        // that stream.
        _GenerateHtmlComponent(pComp);

        //Get the size of the stream generated.
        _pStream->Seek(libMove, STREAM_SEEK_CUR, &libCurPos);

        //Allocare a BSTR big enough to hold our component HTML code.
        if(*pbstr = SysAllocStringLen(NULL, (libCurPos.LowPart)/sizeof(WCHAR)))
        {
            _pStream->Seek(libMove, STREAM_SEEK_SET, NULL);
            _pStream->Read(*pbstr, libCurPos.LowPart, NULL);
        }

        //NOTE: The bStr is released by the caller.
        
        ATOMICRELEASE(_pStream);
    }
    else
        TraceMsg(TF_WARNING, "DS _GenerateHtmlBstrForComp unable to create a mem stream");
        
    EXITPROC(2, "DS _GenerateHtmlBstrForComp");
}


void CActiveDesktop::_UpdateStyleOfElement(IHTMLElement *pElem, LPCOMPONENTA lpCompA)
{
    IHTMLStyle  *pHtmlStyle;
    
    if(SUCCEEDED(pElem->get_style(&pHtmlStyle)))
    {
        long    lPixelVal;
        VARIANT vVal;
        VARIANT vValNew;

        
        if(SUCCEEDED(pHtmlStyle->get_pixelLeft(&lPixelVal)) && (lPixelVal != lpCompA->cpPos.iLeft))
        {
            TraceMsg(TF_DYNAMICHTML, "iLeft changes from %d to %d", lPixelVal, lpCompA->cpPos.iLeft);
            pHtmlStyle->put_pixelLeft((long)(lpCompA->cpPos.iLeft));
        }
        
        if(SUCCEEDED(pHtmlStyle->get_pixelTop(&lPixelVal)) && (lPixelVal != lpCompA->cpPos.iTop))
        {
            TraceMsg(TF_DYNAMICHTML, "iTop changes from %d to %d", lPixelVal, lpCompA->cpPos.iTop);
            pHtmlStyle->put_pixelTop((long)(lpCompA->cpPos.iTop));
        }

        VariantInit(&vVal);
       
        if(SUCCEEDED(pHtmlStyle->get_width(&vVal))) //Get the width as BSTR to see if width attribute exists
        {
            //See if the width attribute exists now.
            if((vVal.vt == VT_BSTR) && (vVal.bstrVal == NULL))
            {
                // Width attribute does not exist for this element; This means that
                // this element has the default width (may be a picture shown in it's original width).
                if(lpCompA->cpPos.dwWidth != COMPONENT_DEFAULT_WIDTH)
                {
                    //Component's new width is different from the default width. So, set the new width.
                    TraceMsg(TF_DYNAMICHTML, "dwWidth changes from default to %d", lpCompA->cpPos.dwWidth);
                    pHtmlStyle->put_pixelWidth((long)(lpCompA->cpPos.dwWidth));
                }
                //else, nothing to do! (the widths match exactly).
                
            }
            else
            {
                // Width attribute exists! That means that this element has a width other than the
                // default width.
                // See if the new width is the default width.
                if(lpCompA->cpPos.dwWidth == COMPONENT_DEFAULT_WIDTH)
                {
                    // The old width is NOT default; But, the new width is default. So, let's just
                    // remove the width attribute.
                    VariantInit(&vValNew);
                    vValNew.vt = VT_BSTR;
                    vValNew.bstrVal = NULL;
                    pHtmlStyle->put_width(vValNew);

                    VariantClear(&vValNew);
                }
                else
                {
                    //Get the existing width in pixels.
                    if(SUCCEEDED(pHtmlStyle->get_pixelWidth(&lPixelVal)) && (((DWORD)lPixelVal) != lpCompA->cpPos.dwWidth))
                    {
                        TraceMsg(TF_DYNAMICHTML, "dwWidth changes from %d to %d", lPixelVal, lpCompA->cpPos.dwWidth);
                        pHtmlStyle->put_pixelWidth((long)(lpCompA->cpPos.dwWidth));
                    }
                    //else, nothing else to do because the widths match!
                }
                
            }
            VariantClear(&vVal);
        }
        
        if(SUCCEEDED(pHtmlStyle->get_height(&vVal))) //Get the height as BSTR to see if height attribute exists
        {
            // See if the height attribute exists.
            if((vVal.vt == VT_BSTR) && (vVal.bstrVal == NULL))
            {
                // Height attribute does not exist for this element; This means that
                // this element has the default height (may be a picture shown in it's original height).
                if(lpCompA->cpPos.dwHeight != COMPONENT_DEFAULT_HEIGHT)
                {
                    //Component's new height is different from the default height. So, set the new height.
                    TraceMsg(TF_DYNAMICHTML, "dwHeight changes from default to %d", lpCompA->cpPos.dwHeight);
                    pHtmlStyle->put_pixelHeight((long)(lpCompA->cpPos.dwHeight));
                }
                //else, nothing to do! (the heights match exactly).
                
            }
            else
            {
                // Height attribute exists! That means that this element has a height other than the
                // default height.
                // See if the new height is the default height.
                if(lpCompA->cpPos.dwHeight == COMPONENT_DEFAULT_HEIGHT)
                {
                    // The old height is NOT default; But, the new height is default. So, let's just
                    // remove the height attribute.
                    VariantInit(&vValNew);
                    vValNew.vt = VT_BSTR;
                    vValNew.bstrVal = NULL;
                    pHtmlStyle->put_height(vValNew);  //remove the height attribute!

                    VariantClear(&vValNew);
                }
                else
                {
                    //Get the existing height in pixels and see if it is different.
                    if(SUCCEEDED(pHtmlStyle->get_pixelHeight(&lPixelVal)) && (((DWORD)lPixelVal) != lpCompA->cpPos.dwHeight))
                    {
                        //Since the new height is different, let's use set the new height!
                        TraceMsg(TF_DYNAMICHTML, "dwHeight changes from %d to %d", lPixelVal, lpCompA->cpPos.dwHeight);
                        pHtmlStyle->put_pixelHeight((long)(lpCompA->cpPos.dwHeight));
                    }
                    //else, nothing else to do because the heights match!
                }
                
            }
            VariantClear(&vVal);
        }
        
        
        if(SUCCEEDED(pHtmlStyle->get_zIndex(&vVal)) && (vVal.vt == VT_I4) && (vVal.lVal != lpCompA->cpPos.izIndex))
        {
            TraceMsg(TF_DYNAMICHTML, "ZIndex changes from %d to %d", vVal.lVal, lpCompA->cpPos.izIndex);
            vVal.lVal = lpCompA->cpPos.izIndex;
            pHtmlStyle->put_zIndex(vVal);
        }
        
        VariantClear(&vVal);
        
        pHtmlStyle->Release();
    }

    //BUGBUG: Should we check for and set the other attributes like "resizeable" etc.,?
}

BOOL  CActiveDesktop::_UpdateIdOfElement(IHTMLElement *pElem, LPCOMPONENTA lpCompA)
{
    BSTR    bstrId;
    BOOL    fWholeElementReplaced = FALSE;  //Assume that the item id does not change.
            
    //Check if the Id of the component and the element matches.
    if(SUCCEEDED(pElem->get_id(&bstrId)))   //get the old id
    {

        if(((DWORD)StrToIntW(bstrId)) != lpCompA->dwID)
        {
            // The following technic does not work in some versions of MSHTML.DLL
            // because IHTMLElement->put_id() does not work unless the doc
            // is in "design mode".
            TCHAR   szNewId[MAXID_LENGTH];
            BSTR    bstrNewId;
            HRESULT hr = S_OK;

            wsprintf(szNewId, TEXT("%d"), lpCompA->dwID);

#ifdef DEBUG
            {
                TCHAR  szOldId[MAXID_LENGTH];
                wsprintf(szOldId, TEXT("%d"), StrToIntW(bstrId));
                TraceMsg(TF_DYNAMICHTML, "DHTML: Id changes from %s to %s", szOldId, szNewId);
            }
#endif //DEBUG
                
            //The Ids do not match. So, let's set the new ID.
                
            if(bstrNewId = SysAllocStringT(szNewId))
            {
                hr = pElem->put_id(bstrNewId);
                SysFreeString(bstrNewId);
            }

            if(FAILED(hr))
            {
                //Replace the whole element's HTML with the newly generated HTML
                BSTR    bstrComp = 0;
            
                _GenerateHtmlBStrForComp(lpCompA, &bstrComp);
                if(bstrComp)
                {
                    if(FAILED(hr = pElem->put_outerHTML(bstrComp)))
                        TraceMsg(TF_DYNAMICHTML, "DHTML: put_outerHTML failed");
                    fWholeElementReplaced = TRUE;
                    SysFreeString(bstrComp);
                }
                else
                {
                    AssertMsg(FALSE, TEXT("DHTML: Unable to create html for comp"));
                }
            }
        }
        //else the ids match; nothing to do!
        SysFreeString(bstrId);      //free the old id.
    }
    else
    {
        AssertMsg(FALSE, TEXT("DS Unable to get the id of the element"));
    }

    return fWholeElementReplaced;
}

HRESULT CActiveDesktop::_UpdateHtmlElement(IHTMLElement *pElem)
{
    VARIANT vData;
    TCHAR   szUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR   szSrcPath[INTERNET_MAX_URL_LENGTH];
    LPTSTR  lpszSrcPath;
    COMPONENTA  CompA;
    int         iIndex;

    //If all components are disabled, then we nuke this component from HTML page.
    if(!_co.fEnableComponents)
    {
        TraceMsg(TF_DYNAMICHTML, "DHTML: No item shown in this mode; so, deleting items");
        pElem->put_outerHTML((BSTR)s_sstrEmpty.wsz);
        
        return S_OK; //Nothing else to do!
    }

    VariantInit(&vData);
    
    //First determine if the given element is currently a desktop item. (It could have been deleted)
    //Get the element's "src" attribute.
    if(FAILED(pElem->getAttribute((BSTR)s_sstrSRCMember.wsz, VARIANT_FALSE, &vData)) ||
            (vData.vt == VT_NULL) ||
            (vData.bstrVal == NULL))
    {
        //If the subscribed_url is not present, then it could be an object with a classid.
        if(FAILED(pElem->getAttribute((BSTR)s_sstrclassid.wsz, VARIANT_FALSE, &vData)) ||
            (vData.vt == VT_NULL))
        {
            //This element is does not have "src=" or "classid=" attributes. How did this ever
            // become a desktop item with "name=deskmovr" or "name=deskmovrw"?? Hmmmmmm....!!!
#ifdef DEBUG
            {
                BSTR    bstrHtmlForElem;
                // Get the HTML corresponding to the element that does not have a subscribed URL
                if(SUCCEEDED(pElem->get_outerHTML(&bstrHtmlForElem)))
                {
                    TraceMsg(TF_DYNAMICHTML, "DHTML: Rogue element: %s", bstrHtmlForElem);
                    SysFreeString(bstrHtmlForElem);
                }
            }
            TraceMsg(TF_WARNING, "DHTML: Unable to get the subscribed_url or classid");
#endif
            //Since this element does not seem to be a valid desktop item, let's nuke it!
            pElem->put_outerHTML((BSTR)s_sstrEmpty.wsz);  //delete this element.
            
            return (E_FAIL);  //Nothing else to for this element! It's gone!!! 
        }
        
        if((vData.vt == VT_NULL) || (vData.bstrVal == NULL))
            return E_FAIL;
            
        ASSERT(vData.vt == VT_BSTR);
        ASSERT(StrCmpNW(vData.bstrVal, L"clsid:", lstrlen(TEXT("clsid:"))) == 0);
        SHUnicodeToTChar(vData.bstrVal + lstrlen(TEXT("clsid:")), szUrl, ARRAYSIZE(szUrl));
        lpszSrcPath = szUrl;  //For classid, the SrcPath and the Url are the same.
    }
    else
    {
        DWORD dwSize; 
        
        if(vData.bstrVal == NULL)
            return (E_FAIL);
            
        ASSERT(vData.vt == VT_BSTR);
        SHUnicodeToTChar((LPWSTR)vData.bstrVal, szUrl, ARRAYSIZE(szUrl));

        dwSize = ARRAYSIZE(szSrcPath);
        lpszSrcPath = szSrcPath;
        if(FAILED(PathCreateFromUrl(szUrl, lpszSrcPath, &dwSize, 0)))
        {
            lpszSrcPath = szUrl;
        }
    }

    VariantClear(&vData); //We made a TCHAR copy above. So, ok to free this.

    CompA.dwSize = sizeof(CompA);

    // First use the Source path to Find the component; This is much more efficient because it
    // involves no conversion from Path to Url and vice-versa.
    if((iIndex = _FindComponentBySource(lpszSrcPath, &CompA)) < 0)
    {
        // Could not find component using SrcPath!
        // Let's try using the SrcUrl; This is less efficient.
        iIndex = _FindComponentBySrcUrl(szUrl, &CompA);
    }
    
    if((iIndex>= 0) && (CompA.fChecked))
    {
        //The component is found and it is enabled.
        TraceMsg(TF_DYNAMICHTML, "DHTML:Updating desktop item with URL: %s", szUrl);

        // If the id changes, we replace the whole HTML for that element, so, no need to check for
        // the individual styles.
        if(!_UpdateIdOfElement(pElem, &CompA))
            _UpdateStyleOfElement(pElem, &CompA);
        CompA.fDirty = TRUE; //Mark the component sothat we know that it had been updated.
        UpdateComponentPrivate(iIndex, &CompA);
    }
    else
    {
        ASSERT((iIndex == -1) || (!CompA.fChecked));  //Component not found OR it is disabled!

        TraceMsg(TF_DYNAMICHTML, "DHTML: Deleting desktop item with URL: %s, SrcPath:%s", szUrl, lpszSrcPath);

        //The component is not present now. So, delete this element from the html page.
        pElem->put_outerHTML((BSTR)s_sstrEmpty.wsz);
    }

    return S_OK;
}

//
// This code enumerates and then updates all the desktop item elements in the active desktop based 
// on the current status of the active desktop items in the CActiveDesktop object (The current 
// status is initialized by reading from the registry when ActiveDesktop object is initialized).
//
HRESULT CActiveDesktop::_UpdateDesktopItemHtmlElements(IHTMLDocument2 *pDoc)
{
    HRESULT hres = S_OK;
    
    IHTMLElementCollection  *pAllElements;

    TraceMsg(TF_DYNAMICHTML, "DHTML: Updating Desktop html elements dynamically");

    if(!_fInitialized)  //If not yet initialized, initialize now because we need _co.fEnableComponents.
        _Initialize();

    // We need to check for a change in the background color only if there is no wallpaper or
    // the wallpaper is a picture.
    if(IsWallpaperPicture(_szSelectedWallpaper))
    {
        COLORREF    rgbDesktop;
        TCHAR       szRgbDesktop[10];
        VARIANT     vColor;
        
        //Check to see if the background color has changed
        rgbDesktop = GetSysColor(COLOR_DESKTOP);
        wsprintf(szRgbDesktop, TEXT("#%02lx%02lx%02lx"), GetRValue(rgbDesktop), 
                                                         GetGValue(rgbDesktop), 
                                                         GetBValue(rgbDesktop));
        if(SUCCEEDED(pDoc->get_bgColor(&vColor)) && (vColor.vt == VT_BSTR))
        {
            BSTR    bstrNewBgColor = SysAllocStringT(szRgbDesktop);

            //Compare the new and the old strings.
            if(StrCmpW(vColor.bstrVal, bstrNewBgColor))
            {
                BSTR bstrOldBgColor = vColor.bstrVal;  //Save the old bstr.
                //So, the colors are different. Set the new color.
                vColor.bstrVal = bstrNewBgColor;
                bstrNewBgColor = bstrOldBgColor;   //Set it here sothat it is freed later.

                if(FAILED(pDoc->put_bgColor(vColor)))
                {
                    TraceMsg(TF_DYNAMICHTML, "DHTML: Unable to change the background color");
                }
            }

            if(bstrNewBgColor)
                SysFreeString(bstrNewBgColor);
        
            VariantClear(&vColor);
        }
    }

    //Get a collection of All elements in the Document
    if(SUCCEEDED(pDoc->get_all(&pAllElements)))
    {
        VARIANT vName, vIndex;
        IDispatch   *pDisp;
        int     i; 
        long    lItemsEnumerated = 0;
        long    lLength = 0;


#ifdef DEBUG
        pAllElements->get_length(&lLength);
        TraceMsg(TF_DYNAMICHTML, "DHTML: Length of All elements:%d", lLength);
#endif

        for(i = 0; i <= 1; i++)
        {
            //Collect all the elements that have the name="DeskMovr" and then name="DeskMovrW"
            vName.vt = VT_BSTR;
            vName.bstrVal = (BSTR)((i == 0) ? s_sstrDeskMovr.wsz : s_sstrDeskMovrW.wsz);

            VariantInit(&vIndex); //We want to get all elements. So, vIndex is set to VT_EMPTY
        
            if(SUCCEEDED(pAllElements->item(vName, vIndex, &pDisp)) && pDisp) //Collect all elements we want
            {
                IHTMLElementCollection  *pDeskCollection;
                if(SUCCEEDED(pDisp->QueryInterface(IID_IHTMLElementCollection, (void **)&pDeskCollection)))
                {
                    IUnknown    *pUnk;
                    IEnumVARIANT    *pEnumVar;
                
                    if(SUCCEEDED(pDeskCollection->get_length(&lLength)))  //Number of elements.
                        lItemsEnumerated += lLength; //Total number of items enumerated.

                    TraceMsg(TF_DYNAMICHTML, "DHTML: Enumerated %d number of elements", lLength);
                    
                    //Get the enumerator
                    if(SUCCEEDED(pDeskCollection->get__newEnum(&pUnk)))
                    {
                        if(SUCCEEDED(pUnk->QueryInterface(IID_IEnumVARIANT, (void **)&pEnumVar)))
                        {
                            VARIANT vElem;
                            long    lEnumCount = 0;
                            DWORD   cElementsFetched;
                        
                            while(SUCCEEDED(pEnumVar->Next(1, &vElem, &cElementsFetched)) && (cElementsFetched == 1))
                            {
                                IHTMLElement *pElem;
                                lEnumCount++;
                                //  Access the element from the variant.....!
                                if((vElem.vt == VT_DISPATCH) && SUCCEEDED(vElem.pdispVal->QueryInterface(IID_IHTMLElement, (void **)&pElem)))
                                {
                                    _UpdateHtmlElement(pElem); //Update the desktop element's attributes.
                                    pElem->Release();
                                }
                                VariantClear(&vElem);
                            }
                            //Number of items enumerated must be the same as the length
                            ASSERT(lEnumCount == lLength);
                            
                            pEnumVar->Release();
                        }
                        pUnk->Release();
                    }
                
                    pDeskCollection->Release();
                }
                else
                {
                    IHTMLElement    *pElem;
                    
                    // The QI(IID_IHTMLElementCollection) has failed. It may be because only one item 
                    // was returned rather than a collection.
                    if(SUCCEEDED(pDisp->QueryInterface(IID_IHTMLElement, (void **)&pElem)))
                    {
                        _UpdateHtmlElement(pElem); //Update the desktop element's attributes.
                        pElem->Release();
                    }
                    else
                        TraceMsg(TF_WARNING, "DHTML: Unable to get a collection or a single element");
                }
                pDisp->Release();
            }
        } // for loop enumeating "DeskMovr" and "DeskMovrW" items.
        
        pAllElements->Release();
    }

    // All the elements already present in the Doc have been updated. Now, let's add the
    // new elements, if any.
    if(_co.fEnableComponents)
        _InsertNewDesktopItems(pDoc);
    else
    {
        TraceMsg(TF_DYNAMICHTML, "DHTML: No components are to be shown in this mode;");
    }

    return hres;
}

HRESULT CActiveDesktop::_InsertNewDesktopItems(IHTMLDocument2  *pDoc)
{
    IHTMLElement    *pBody;
    
    if(SUCCEEDED(pDoc->get_body(&pBody)))
    {
        if (_hdsaComponent)
        {
            int i, iCount;

            iCount = DSA_GetItemCount(_hdsaComponent);
            for (i=0; i<iCount; i++)
            {
                COMPONENTA comp;
                comp.dwSize = sizeof(comp);

                if (DSA_GetItem(_hdsaComponent, i, &comp) != -1)
                {
                    //Check if this is a newly added component AND it is enabled.
                    if((!comp.fDirty) && comp.fChecked)
                    {
                        TraceMsg(TF_DYNAMICHTML, "DHTML: Inserted comp: %s", comp.szSource);
                        
                        //Yup! This is a newly added component!
                        BSTR  bstrComp = 0;
                        //This is a new component. Generate the HTML for the component.
                        _GenerateHtmlBStrForComp(&comp, &bstrComp);

                        //Insert the component.
                        pBody->insertAdjacentHTML((BSTR)s_sstrBeforeEnd.wsz, (BSTR)bstrComp);

                        //Free the string.
                        SysFreeString(bstrComp);
                    }
                }
                else
                {
                    TraceMsg(TF_WARNING, "DHTML: InsertNewComp: Unable to get component %d.", i);
                }
            }
        }
        
        pBody->Release();
    }

    return S_OK;
}

//
// This function takes a pointer to the ActiveDesktop's ole obj, reads all the changes to be done
// from the registry and makes those changes to the various elements through dynamic HTML interfaces.
//
HRESULT CActiveDesktop::MakeDynamicChanges(IOleObject *pOleObj)
{

    IHTMLDocument2  *pDoc;
    HRESULT         hres = E_FAIL;

    ENTERPROC(2, "MakeDynamicChanges");

    if(pOleObj && SUCCEEDED(pOleObj->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc)))
    {
        // Enumerate all the active desktop components and ensure they are up to date.
        _UpdateDesktopItemHtmlElements(pDoc);

        pDoc->Release();
    }
    else
    {
        TraceMsg(TF_WARNING, "DHTML: MakeDynamicChanges: Unable to get IHTMLDocument2");
    }
    
    EXITPROC(2, "MakeDynamicChanges");

    return(hres);
}

//
// SetSafeMode
//
// Either puts the active desktop in safemode or restores it to the previous
// scheme before safemode was entered.
//
HRESULT CActiveDesktop::SetSafeMode(DWORD dwFlags)
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
HRESULT CActiveDesktop::EnsureUpdateHTML(void)
{
    DWORD dwFlags = 0;
    DWORD dwDataLength = sizeof(DWORD);
    DWORD dwType;
    LONG lRet;
    TCHAR lpszDeskcomp[MAX_PATH];
    TCHAR szDesktopFile[MAX_PATH];
    DWORD dwRestrictUpdate;
    DWORD dwRestrict = SHRestricted2W(REST_NoChannelUI, NULL, 0);
    DWORD dwSize = SIZEOF(dwRestrictUpdate);
    BOOL  fComponentsDirty = FALSE;  //Assume that the components are NOT dirty!
    DWORD dwVersion;
    DWORD dwMinorVersion;
    BOOL  fStaleInfoInReg = FALSE;
    BOOL  fComponentsZoomDirty = FALSE;
    static BOOL s_fNoDeskComp = (BOOL)-1;
    static BOOL s_fNoWallpaper = (BOOL)-1;
    BOOL fNoDeskComp = SHRestricted(REST_NODESKCOMP);
    BOOL fNoWallpaper = SHRestricted(REST_NOHTMLWALLPAPER);
    BOOL fAdminComponent = FALSE;
    HKEY hkey = NULL;


    if (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, REG_DESKCOMP_COMPONENTS_ROOT, REG_VAL_GENERAL_RESTRICTUPDATE, NULL, (LPVOID) &dwRestrictUpdate, &dwSize))
        dwRestrictUpdate = 0;

    GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_COMPONENTS, NULL);

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
        if (IsFlagSet(dwFlags, COMPONENTS_ZOOMDIRTY))
            fComponentsZoomDirty = TRUE;
    }

    // See if we need to add/delete an administrator added desktop component now
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REG_DESKCOMP_ADMINCOMP_ROOT, 0, KEY_READ, &hkey))
    {
        WCHAR wszDisplay[MAX_PATH];
        DWORD dwcch = MAX_PATH;

        if(FAILED(GetScheme(wszDisplay, &dwcch, SCHEME_GLOBAL | SCHEME_DISPLAY)) ||
                 (StrCmpW(wszDisplay, REG_DESKCOMP_SAFEMODE_SUFFIX_L) != 0))
        {
            // We're not in safe mode, it's OK to add the components
            fAdminComponent = TRUE;
        }
    }

    // 99/03/23 #237632 vtan: If the monitor arrangement has been changed from underneath
    // the user (perhaps by another user) then make sure that the components are in valid
    // positions. If not then snap them back into the visible space and mark them as dirty
    // so that the desktop.htt file is regenerated.

    _Initialize();
    if (_hdsaComponent)
    {
        int     i, iComponentCount;;

        iComponentCount = DSA_GetItemCount(_hdsaComponent);
        for (i = 0; i < iComponentCount; ++i)
        {
            COMPONENTA  *pComponent;

            pComponent = reinterpret_cast<COMPONENTA*>(DSA_GetItemPtr(_hdsaComponent, i));
            if (pComponent != NULL)
            {
                bool    bChangedPosition, bChangedSize;

                ValidateComponentPosition(&pComponent->cpPos, pComponent->dwCurItemState, pComponent->iComponentType, &bChangedPosition, &bChangedSize);
                if (bChangedPosition || bChangedSize)
                {
                    TBOOL(UpdateComponentPrivate(i, pComponent));
                    fComponentsDirty = true;
                }
            }
        }
    }

    GetPerUserFileName(szDesktopFile, ARRAYSIZE(szDesktopFile), DESKTOPHTML_FILENAME);

    if (fComponentsDirty ||
         fComponentsZoomDirty ||
         fStaleInfoInReg ||
         fAdminComponent ||
         fNoDeskComp != s_fNoDeskComp ||
         fNoWallpaper != s_fNoWallpaper ||
         (dwRestrictUpdate != dwRestrict) ||
         (!PathFileExistsAndAttributes(szDesktopFile, NULL)))  //See if the file exists!
    {

        // Clear out any html wallpaper if it exists and the restriction is set
        if (fNoWallpaper != s_fNoWallpaper)
        {
            if (fNoWallpaper && !IsWallpaperPicture(_szSelectedWallpaper))
                SetWallpaper(L"", 0);
            s_fNoWallpaper = fNoWallpaper;
        }

        // Disable components if the restriction is set
        if (fNoDeskComp != s_fNoDeskComp)
        {
            // We can't set fEnableComponents to FALSE because there is no way via the UI
            // for the user to turn it back on again if the restriction is lifted.  Instead we add
            // special case code to _GenerateHtml that checks the restriction too.

            // _co.fEnableComponents = !fNoDeskComp; 
            s_fNoDeskComp = fNoDeskComp;
        }

        if (fAdminComponent)
        {
            COMPONENT comp;
            TCHAR pszAdminComp[INTERNET_MAX_URL_LENGTH];
            CHAR szUrl[INTERNET_MAX_URL_LENGTH];
            CHAR * pszUrl;
            CHAR * pszUrlList;
            TCHAR * aszAdminComp[] = {REG_VAL_ADMINCOMP_ADD, REG_VAL_ADMINCOMP_DELETE, NULL};
            int i = 0;

            comp.dwSize = SIZEOF(comp);
            comp.dwCurItemState = IS_SPLIT | IS_ADJUSTLISTVIEW;

            while (aszAdminComp[i])
            {
                dwDataLength = SIZEOF(pszAdminComp);
                // The reg value contains an array of space separated urls - currently we support adding and deleting
                // a desktop item via this mechanism.
                if (SHQueryValueEx(hkey, aszAdminComp[i], NULL, &dwType, (LPBYTE)pszAdminComp, &dwDataLength) == ERROR_SUCCESS)
                {
                    SHTCharToAnsi(pszAdminComp, szUrl, ARRAYSIZE(szUrl));
                    pszUrlList = szUrl;
                    while (pszUrl = StrTokEx(&pszUrlList, " ")) {
                        SHAnsiToUnicode(pszUrl, comp.wszSource, ARRAYSIZE(comp.wszSource));
                        dwDataLength = ARRAYSIZE(comp.wszSource);
                        ParseURLFromOutsideSourceW(comp.wszSource, comp.wszSource, &dwDataLength, NULL);
                        if (lstrcmp(aszAdminComp[i], REG_VAL_ADMINCOMP_ADD) == 0)
                        {
                            AddUrl(NULL, (LPCWSTR)comp.wszSource, &comp, ADDURL_SILENT);
                            fComponentsZoomDirty = TRUE;
                        }
                        else
                        {
                            RemoveDesktopItem((LPCOMPONENT)&comp, 0);
                        }
                    }
                }
                i++;
            }
        }

        // Go through the entire list of desktop components and ensure any split/fullscreen
        // components are at their correct size/location.
        if (fComponentsZoomDirty)
        {
            if (_hdsaComponent)
            {
                int i;
                for (i = 0; i < DSA_GetItemCount(_hdsaComponent); i++)
                {
                    COMPONENTA * pcompT;
                
                    if (pcompT = (COMPONENTA *)DSA_GetItemPtr(_hdsaComponent, i))
                    {
                        if (ISZOOMED(pcompT))
                        {
                            BOOL fAdjustListview = (pcompT->dwCurItemState & IS_ADJUSTLISTVIEW);
                            ZoomComponent(&pcompT->cpPos, pcompT->dwCurItemState, fAdjustListview);
                            if (fAdjustListview)
                                pcompT->dwCurItemState &= ~IS_ADJUSTLISTVIEW;
                        }
                    }
                }
                SetDesktopFlags(COMPONENTS_ZOOMDIRTY, 0);
            }
        }

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

    if (hkey)
    {
        RegCloseKey(hkey);
        SHDeleteKey(HKEY_CURRENT_USER, REG_DESKCOMP_ADMINCOMP_ROOT);
    }

    return (lRet == ERROR_SUCCESS ? S_OK : E_FAIL);
}

//
//  ReReadWallpaper()
//      If the wallpaper was read when the active desktop was disabled, we would have read it from
//  the old location. Now, if the active desktop is turned ON, then we need to re-read the wallpaper
//  from the new location. We need to do this iff the wallpaper has not been changed in the mean-while
//
HRESULT CActiveDesktop::ReReadWallpaper(void)
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
HRESULT CActiveDesktop::GetADObjectFlags(LPDWORD lpdwFlags, DWORD dwMask)
{
    ASSERT(lpdwFlags);
    
    *lpdwFlags = 0; //Init the flags
    
    if((dwMask & GADOF_DIRTY) && _fDirty)
        *lpdwFlags |= GADOF_DIRTY;

    return(S_OK);
}

#ifdef _WIN64
/***
*char *StrTokEx(pstring, control) - tokenize string with delimiter in control
*
*Purpose:
*       StrTokEx considers the string to consist of a sequence of zero or more
*       text tokens separated by spans of one or more control chars. the first
*       call, with string specified, returns a pointer to the first char of the
*       first token, and will write a null char into pstring immediately
*       following the returned token. when no tokens remain
*       in pstring a NULL pointer is returned. remember the control chars with a
*       bit map, one bit per ascii char. the null char is always a control char.
*
*Entry:
*       char **pstring - ptr to ptr to string to tokenize
*       char *control - string of characters to use as delimiters
*
*Exit:
*       returns pointer to first token in string,
*       returns NULL when no more tokens remain.
*       pstring points to the beginning of the next token.
*
*WARNING!!!
*       upon exit, the first delimiter in the input string will be replaced with '\0'
*
*       copied from iert.lib
*******************************************************************************/

extern "C" char * __cdecl StrTokEx(char ** spstring, const char * scontrol)
{
        unsigned char **pstring = (unsigned char**) spstring;
        unsigned char *control = (unsigned char*) scontrol;

        unsigned char *str;
        const unsigned char *ctrl = control;
        unsigned char map[32];
        int count;

        unsigned char *tokenstr;

        if(*pstring == NULL)
            return NULL;
            
        /* Clear control map */
        for (count = 0; count < 32; count++)
                map[count] = 0;

        /* Set bits in delimiter table */
        do
        {
            map[*ctrl >> 3] |= (1 << (*ctrl & 7));
        } while (*ctrl++);

        /* Initialize str. */
        str = *pstring;
        
        /* Find beginning of token (skip over leading delimiters). Note that
         * there is no token if this loop sets str to point to the terminal
         * null (*str == '\0') */
        while ( (map[*str >> 3] & (1 << (*str & 7))) && *str )
            str++;

        tokenstr = str;

        /* Find the end of the token. If it is not the end of the string,
         * put a null there. */
        for ( ; *str ; str++ )
        {
            if ( map[*str >> 3] & (1 << (*str & 7)) ) 
            {
                *str++ = '\0';
                break;
            }
        }

        /* string now points to beginning of next token */
        *pstring = str;

        /* Determine if a token has been found. */
        if ( tokenstr == str )
            return NULL;
        else
            return (char*)tokenstr;
}

#endif //_WIN64