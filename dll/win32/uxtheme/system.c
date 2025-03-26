/*
 * Win32 5.1 Theme system
 *
 * Copyright (C) 2003 Kevin Koltzau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "uxthemep.h"

#include <stdio.h>
#include <winreg.h>
#include <uxundoc.h>

DWORD gdwErrorInfoTlsIndex = TLS_OUT_OF_INDEXES;

/***********************************************************************
 * Defines and global variables
 */

static const WCHAR szThemeManager[] = {
    'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'T','h','e','m','e','M','a','n','a','g','e','r','\0'
};
static const WCHAR szThemeActive[] = {'T','h','e','m','e','A','c','t','i','v','e','\0'};
static const WCHAR szSizeName[] = {'S','i','z','e','N','a','m','e','\0'};
static const WCHAR szColorName[] = {'C','o','l','o','r','N','a','m','e','\0'};
static const WCHAR szDllName[] = {'D','l','l','N','a','m','e','\0'};

static const WCHAR szIniDocumentation[] = {'d','o','c','u','m','e','n','t','a','t','i','o','n','\0'};

HINSTANCE hDllInst;
ATOM atDialogThemeEnabled;

static DWORD dwThemeAppProperties = STAP_ALLOW_NONCLIENT | STAP_ALLOW_CONTROLS;
ATOM atWindowTheme;
static ATOM atSubAppName;
static ATOM atSubIdList;
ATOM atWndContext;

PTHEME_FILE g_ActiveThemeFile;

RTL_HANDLE_TABLE g_UxThemeHandleTable;
int g_cHandles;

/***********************************************************************/

static BOOL CALLBACK UXTHEME_send_theme_changed (HWND hWnd, LPARAM enable)
{
    SendMessageW(hWnd, WM_THEMECHANGED, enable, 0);
    return TRUE;
}

/* Broadcast WM_THEMECHANGED to *all* windows, including children */
BOOL CALLBACK UXTHEME_broadcast_theme_changed (HWND hWnd, LPARAM enable)
{
    if (hWnd == NULL)
    {
        EnumWindows (UXTHEME_broadcast_theme_changed, enable);
    }
    else
    {
        UXTHEME_send_theme_changed(hWnd, enable);
        EnumChildWindows (hWnd, UXTHEME_send_theme_changed, enable);
    }
    return TRUE;
}

/* At the end of the day this is a subset of what SHRegGetPath() does - copied
 * here to avoid linking against shlwapi. */
static DWORD query_reg_path (HKEY hKey, LPCWSTR lpszValue,
                             LPVOID pvData)
{
  DWORD dwRet, dwType, dwUnExpDataLen = MAX_PATH * sizeof(WCHAR), dwExpDataLen;

  TRACE("(hkey=%p,%s,%p)\n", hKey, debugstr_w(lpszValue),
        pvData);

  dwRet = RegQueryValueExW(hKey, lpszValue, 0, &dwType, pvData, &dwUnExpDataLen);
  if (dwRet!=ERROR_SUCCESS && dwRet!=ERROR_MORE_DATA)
      return dwRet;

  if (dwType == REG_EXPAND_SZ)
  {
    DWORD nBytesToAlloc;

    /* Expand type REG_EXPAND_SZ into REG_SZ */
    LPWSTR szData;

    /* If the caller didn't supply a buffer or the buffer is too small we have
     * to allocate our own
     */
    if (dwRet == ERROR_MORE_DATA)
    {
      WCHAR cNull = '\0';
      nBytesToAlloc = dwUnExpDataLen;

      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc);
      RegQueryValueExW (hKey, lpszValue, 0, NULL, (LPBYTE)szData, &nBytesToAlloc);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, &cNull, 1);
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
    else
    {
      nBytesToAlloc = (lstrlenW(pvData) + 1) * sizeof(WCHAR);
      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc );
      lstrcpyW(szData, pvData);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, pvData, MAX_PATH );
      if (dwExpDataLen > MAX_PATH) dwRet = ERROR_MORE_DATA;
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
  }

  return dwRet;
}

static HRESULT UXTHEME_SetActiveTheme(PTHEME_FILE tf)
{
    if(g_ActiveThemeFile)
        MSSTYLES_CloseThemeFile(g_ActiveThemeFile);
    g_ActiveThemeFile = tf;
    if (g_ActiveThemeFile)
    {
        MSSTYLES_ReferenceTheme(g_ActiveThemeFile);
        MSSTYLES_ParseThemeIni(g_ActiveThemeFile);
    }
    return S_OK;
}

static BOOL bIsThemeActive(LPCWSTR pszTheme, LPCWSTR pszColor, LPCWSTR pszSize)
{
    if (g_ActiveThemeFile == NULL)
        return FALSE;

    if (wcscmp(pszTheme, g_ActiveThemeFile->szThemeFile) != 0)
        return FALSE;

    if (!pszColor[0])
    {
        if (g_ActiveThemeFile->pszAvailColors != g_ActiveThemeFile->pszSelectedColor)
            return FALSE;
    }
    else
    {
        if (wcscmp(pszColor, g_ActiveThemeFile->pszSelectedColor) != 0)
            return FALSE;
    }

    if (!pszSize[0])
    {
        if (g_ActiveThemeFile->pszAvailSizes != g_ActiveThemeFile->pszSelectedSize)
            return FALSE;
    }
    else
    {
        if (wcscmp(pszSize, g_ActiveThemeFile->pszSelectedSize) != 0)
            return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *      UXTHEME_LoadTheme
 *
 * Set the current active theme from the registry
 */
void UXTHEME_LoadTheme(BOOL bLoad)
{
    HKEY hKey;
    DWORD buffsize;
    HRESULT hr;
    WCHAR tmp[10];
    PTHEME_FILE pt;
    WCHAR szCurrentTheme[MAX_PATH];
    WCHAR szCurrentColor[64];
    WCHAR szCurrentSize[64];
    BOOL bThemeActive = FALSE;

    if ((bLoad != FALSE) && g_bThemeHooksActive) 
    {
        /* Get current theme configuration */
        if(!RegOpenKeyW(HKEY_CURRENT_USER, szThemeManager, &hKey)) {
            TRACE("Loading theme config\n");
            buffsize = sizeof(tmp);
            if(!RegQueryValueExW(hKey, szThemeActive, NULL, NULL, (LPBYTE)tmp, &buffsize)) {
                bThemeActive = (tmp[0] != '0');
            }
            else {
                bThemeActive = FALSE;
                TRACE("Failed to get ThemeActive: %d\n", GetLastError());
            }
            buffsize = sizeof(szCurrentColor);
            if(RegQueryValueExW(hKey, szColorName, NULL, NULL, (LPBYTE)szCurrentColor, &buffsize))
                szCurrentColor[0] = '\0';
            buffsize = sizeof(szCurrentSize);
            if(RegQueryValueExW(hKey, szSizeName, NULL, NULL, (LPBYTE)szCurrentSize, &buffsize))
                szCurrentSize[0] = '\0';
            if (query_reg_path (hKey, szDllName, szCurrentTheme))
                szCurrentTheme[0] = '\0';
            RegCloseKey(hKey);
        }
        else
            TRACE("Failed to open theme registry key\n");
    }
    else
    {
        bThemeActive = FALSE;
    }

    if(bThemeActive)
    {
        if( bIsThemeActive(szCurrentTheme, szCurrentColor, szCurrentSize) )
        {
            TRACE("Tried to load active theme again\n");
            return;
        }

        /* Make sure the theme requested is actually valid */
        hr = MSSTYLES_OpenThemeFile(szCurrentTheme,
                                    szCurrentColor[0]?szCurrentColor:NULL,
                                    szCurrentSize[0]?szCurrentSize:NULL,
                                    &pt);
        if(FAILED(hr)) {
            bThemeActive = FALSE;
        }
        else {
            TRACE("Theme active: %s %s %s\n", debugstr_w(szCurrentTheme),
                debugstr_w(szCurrentColor), debugstr_w(szCurrentSize));

            UXTHEME_SetActiveTheme(pt);
            MSSTYLES_CloseThemeFile(pt);
        }
    }
    if(!bThemeActive) {
        UXTHEME_SetActiveTheme(NULL);
        TRACE("Theming not active\n");
    }
}

/***********************************************************************/

static const char * const SysColorsNames[] =
{
    "Scrollbar",                /* COLOR_SCROLLBAR */
    "Background",               /* COLOR_BACKGROUND */
    "ActiveTitle",              /* COLOR_ACTIVECAPTION */
    "InactiveTitle",            /* COLOR_INACTIVECAPTION */
    "Menu",                     /* COLOR_MENU */
    "Window",                   /* COLOR_WINDOW */
    "WindowFrame",              /* COLOR_WINDOWFRAME */
    "MenuText",                 /* COLOR_MENUTEXT */
    "WindowText",               /* COLOR_WINDOWTEXT */
    "TitleText",                /* COLOR_CAPTIONTEXT */
    "ActiveBorder",             /* COLOR_ACTIVEBORDER */
    "InactiveBorder",           /* COLOR_INACTIVEBORDER */
    "AppWorkSpace",             /* COLOR_APPWORKSPACE */
    "Hilight",                  /* COLOR_HIGHLIGHT */
    "HilightText",              /* COLOR_HIGHLIGHTTEXT */
    "ButtonFace",               /* COLOR_BTNFACE */
    "ButtonShadow",             /* COLOR_BTNSHADOW */
    "GrayText",                 /* COLOR_GRAYTEXT */
    "ButtonText",               /* COLOR_BTNTEXT */
    "InactiveTitleText",        /* COLOR_INACTIVECAPTIONTEXT */
    "ButtonHilight",            /* COLOR_BTNHIGHLIGHT */
    "ButtonDkShadow",           /* COLOR_3DDKSHADOW */
    "ButtonLight",              /* COLOR_3DLIGHT */
    "InfoText",                 /* COLOR_INFOTEXT */
    "InfoWindow",               /* COLOR_INFOBK */
    "ButtonAlternateFace",      /* COLOR_ALTERNATEBTNFACE */
    "HotTrackingColor",         /* COLOR_HOTLIGHT */
    "GradientActiveTitle",      /* COLOR_GRADIENTACTIVECAPTION */
    "GradientInactiveTitle",    /* COLOR_GRADIENTINACTIVECAPTION */
    "MenuHilight",              /* COLOR_MENUHILIGHT */
    "MenuBar",                  /* COLOR_MENUBAR */
};
static const WCHAR strColorKey[] = 
    { 'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\',
      'C','o','l','o','r','s',0 };
static const WCHAR keyFlatMenus[] = { 'F','l','a','t','M','e','n','u', 0};
static const WCHAR keyGradientCaption[] = { 'G','r','a','d','i','e','n','t',
                                            'C','a','p','t','i','o','n', 0 };
static const WCHAR keyNonClientMetrics[] = { 'N','o','n','C','l','i','e','n','t',
                                             'M','e','t','r','i','c','s',0 };
static const WCHAR keyIconTitleFont[] = { 'I','c','o','n','T','i','t','l','e',
					  'F','o','n','t',0 };

static const struct BackupSysParam
{
    int spiGet, spiSet;
    const WCHAR* keyName;
} backupSysParams[] = 
{
    {SPI_GETFLATMENU, SPI_SETFLATMENU, keyFlatMenus},
    {SPI_GETGRADIENTCAPTIONS, SPI_SETGRADIENTCAPTIONS, keyGradientCaption},
    {-1, -1, 0}
};

#define NUM_SYS_COLORS     (COLOR_MENUBAR+1)

static void save_sys_colors (HKEY baseKey)
{
    char colorStr[13];
    HKEY hKey;
    int i;

    if (RegCreateKeyExW( baseKey, strColorKey,
                         0, 0, 0, KEY_ALL_ACCESS,
                         0, &hKey, 0 ) == ERROR_SUCCESS)
    {
        for (i = 0; i < NUM_SYS_COLORS; i++)
        {
            COLORREF col = GetSysColor (i);
        
            sprintf (colorStr, "%d %d %d", 
                GetRValue (col), GetGValue (col), GetBValue (col));

            RegSetValueExA (hKey, SysColorsNames[i], 0, REG_SZ, 
                (BYTE*)colorStr, strlen (colorStr)+1);
        }
        RegCloseKey (hKey);
    }
}

/* Before activating a theme, query current system colors, certain settings 
 * and backup them in the registry, so they can be restored when the theme 
 * is deactivated */
static void UXTHEME_BackupSystemMetrics(void)
{
    HKEY hKey;
    const struct BackupSysParam* bsp = backupSysParams;

    if (RegCreateKeyExW( HKEY_CURRENT_USER, szThemeManager,
                         0, 0, 0, KEY_ALL_ACCESS,
                         0, &hKey, 0) == ERROR_SUCCESS)
    {
        NONCLIENTMETRICSW ncm;
        LOGFONTW iconTitleFont;
        
        /* back up colors */
        save_sys_colors (hKey);
    
        /* back up "other" settings */
        while (bsp->spiGet >= 0)
        {
            DWORD value;
            
            SystemParametersInfoW (bsp->spiGet, 0, &value, 0);
            RegSetValueExW (hKey, bsp->keyName, 0, REG_DWORD, 
                (LPBYTE)&value, sizeof (value));
        
            bsp++;
        }
        
	/* back up non-client metrics */
        memset (&ncm, 0, sizeof (ncm));
        ncm.cbSize = sizeof (ncm);
        SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, sizeof (ncm), &ncm, 0);
        RegSetValueExW (hKey, keyNonClientMetrics, 0, REG_BINARY, (LPBYTE)&ncm,
            sizeof (ncm));
	memset (&iconTitleFont, 0, sizeof (iconTitleFont));
	SystemParametersInfoW (SPI_GETICONTITLELOGFONT, sizeof (iconTitleFont),
	    &iconTitleFont, 0);
	RegSetValueExW (hKey, keyIconTitleFont, 0, REG_BINARY, 
	    (LPBYTE)&iconTitleFont, sizeof (iconTitleFont));
    
        RegCloseKey (hKey);
    }
}

/* Read back old settings after a theme was deactivated */
static void UXTHEME_RestoreSystemMetrics(void)
{
    HKEY hKey;
    const struct BackupSysParam* bsp = backupSysParams;

    if (RegOpenKeyExW (HKEY_CURRENT_USER, szThemeManager,
                       0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) 
    {
        HKEY colorKey;
    
        /* read backed-up colors */
        if (RegOpenKeyExW (hKey, strColorKey,
                           0, KEY_QUERY_VALUE, &colorKey) == ERROR_SUCCESS) 
        {
            int i;
            COLORREF sysCols[NUM_SYS_COLORS];
            int sysColsIndices[NUM_SYS_COLORS];
            int sysColCount = 0;
        
            for (i = 0; i < NUM_SYS_COLORS; i++)
            {
                DWORD type;
                char colorStr[13];
                DWORD count = sizeof(colorStr);
            
                if (RegQueryValueExA (colorKey, SysColorsNames[i], 0,
                    &type, (LPBYTE) colorStr, &count) == ERROR_SUCCESS)
                {
                    int r, g, b;
                    if (sscanf (colorStr, "%d %d %d", &r, &g, &b) == 3)
                    {
                        sysColsIndices[sysColCount] = i;
                        sysCols[sysColCount] = RGB(r, g, b);
                        sysColCount++;
                    }
                }
            }
            RegCloseKey (colorKey);
          
            SetSysColors (sysColCount, sysColsIndices, sysCols);
        }
    
        /* read backed-up other settings */
        while (bsp->spiGet >= 0)
        {
            DWORD value;
            DWORD count = sizeof(value);
            DWORD type;
            
            if (RegQueryValueExW (hKey, bsp->keyName, 0,
                &type, (LPBYTE)&value, &count) == ERROR_SUCCESS)
            {
                SystemParametersInfoW (bsp->spiSet, 0, UlongToPtr(value), SPIF_UPDATEINIFILE);
            }
        
            bsp++;
        }
    
        /* read backed-up non-client metrics */
        {
            NONCLIENTMETRICSW ncm;
            LOGFONTW iconTitleFont;
            DWORD count = sizeof(ncm);
            DWORD type;
            
	    if (RegQueryValueExW (hKey, keyNonClientMetrics, 0,
		&type, (LPBYTE)&ncm, &count) == ERROR_SUCCESS)
	    {
		SystemParametersInfoW (SPI_SETNONCLIENTMETRICS, 
                    count, &ncm, SPIF_UPDATEINIFILE);
	    }
	    
            count = sizeof(iconTitleFont);
            
	    if (RegQueryValueExW (hKey, keyIconTitleFont, 0,
		&type, (LPBYTE)&iconTitleFont, &count) == ERROR_SUCCESS)
	    {
		SystemParametersInfoW (SPI_SETICONTITLELOGFONT, 
                    count, &iconTitleFont, SPIF_UPDATEINIFILE);
	    }
	}
      
        RegCloseKey (hKey);
    }
}

/* Make system settings persistent, so they're in effect even w/o uxtheme 
 * loaded.
 * For efficiency reasons, only the last SystemParametersInfoW sets
 * SPIF_SENDCHANGE */
static void UXTHEME_SaveSystemMetrics(void)
{
    const struct BackupSysParam* bsp = backupSysParams;
    NONCLIENTMETRICSW ncm;
    LOGFONTW iconTitleFont;

    save_sys_colors (HKEY_CURRENT_USER);

    while (bsp->spiGet >= 0)
    {
        DWORD value;
        
        SystemParametersInfoW (bsp->spiGet, 0, &value, 0);
        SystemParametersInfoW (bsp->spiSet, 0, UlongToPtr(value), SPIF_UPDATEINIFILE);
        bsp++;
    }
    
    memset (&ncm, 0, sizeof (ncm));
    ncm.cbSize = sizeof (ncm);
    SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, sizeof (ncm), &ncm, 0);
    SystemParametersInfoW (SPI_SETNONCLIENTMETRICS, sizeof (ncm), &ncm,
        SPIF_UPDATEINIFILE);

    memset (&iconTitleFont, 0, sizeof (iconTitleFont));
    SystemParametersInfoW (SPI_GETICONTITLELOGFONT, sizeof (iconTitleFont),
        &iconTitleFont, 0);
    SystemParametersInfoW (SPI_SETICONTITLELOGFONT, sizeof (iconTitleFont),
        &iconTitleFont, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

/***********************************************************************
 *      UXTHEME_ApplyTheme
 *
 * Change the current active theme
 */
static HRESULT UXTHEME_ApplyTheme(PTHEME_FILE tf)
{
    HKEY hKey;
    WCHAR tmp[2];
    HRESULT hr;

    TRACE("UXTHEME_ApplyTheme\n");

    if (tf && !g_ActiveThemeFile)
    {
        UXTHEME_BackupSystemMetrics();
    }

    hr = UXTHEME_SetActiveTheme(tf);
    if (FAILED(hr))
        return hr;

    if (!tf) 
    {
        UXTHEME_RestoreSystemMetrics();
    }

    TRACE("Writing theme config to registry\n");
    if(!RegCreateKeyW(HKEY_CURRENT_USER, szThemeManager, &hKey)) {
        tmp[0] = g_ActiveThemeFile ? '1' : '0';
        tmp[1] = '\0';
        RegSetValueExW(hKey, szThemeActive, 0, REG_SZ, (const BYTE*)tmp, sizeof(WCHAR)*2);
        if (g_ActiveThemeFile) {
            RegSetValueExW(hKey, szColorName, 0, REG_SZ, (const BYTE*)tf->pszSelectedColor, 
		(lstrlenW(tf->pszSelectedColor)+1)*sizeof(WCHAR));
            RegSetValueExW(hKey, szSizeName, 0, REG_SZ, (const BYTE*)tf->pszSelectedSize, 
		(lstrlenW(tf->pszSelectedSize)+1)*sizeof(WCHAR));
            RegSetValueExW(hKey, szDllName, 0, REG_SZ, (const BYTE*)tf->szThemeFile, 
		(lstrlenW(tf->szThemeFile)+1)*sizeof(WCHAR));
        }
        else {
            RegDeleteValueW(hKey, szColorName);
            RegDeleteValueW(hKey, szSizeName);
            RegDeleteValueW(hKey, szDllName);

        }
        RegCloseKey(hKey);
    }
    else
        TRACE("Failed to open theme registry key\n");
    
    UXTHEME_SaveSystemMetrics ();
    
    return hr;
}

/***********************************************************************
 *      UXTHEME_InitSystem
 */
void UXTHEME_InitSystem(HINSTANCE hInst)
{
    static const WCHAR szWindowTheme[] = {
        'u','x','_','t','h','e','m','e','\0'
    };
    static const WCHAR szSubAppName[] = {
        'u','x','_','s','u','b','a','p','p','\0'
    };
    static const WCHAR szSubIdList[] = {
        'u','x','_','s','u','b','i','d','l','s','t','\0'
    };
    static const WCHAR szDialogThemeEnabled[] = {
        'u','x','_','d','i','a','l','o','g','t','h','e','m','e','\0'
    };

    hDllInst = hInst;

    atWindowTheme        = GlobalAddAtomW(szWindowTheme);
    atSubAppName         = GlobalAddAtomW(szSubAppName);
    atSubIdList          = GlobalAddAtomW(szSubIdList);
    atDialogThemeEnabled = GlobalAddAtomW(szDialogThemeEnabled);
    atWndContext        = GlobalAddAtomW(L"ux_WndContext");

    RtlInitializeHandleTable(0xFFF, sizeof(UXTHEME_HANDLE), &g_UxThemeHandleTable);
    g_cHandles = 0;

    gdwErrorInfoTlsIndex = TlsAlloc();
}

/***********************************************************************
 *      UXTHEME_UnInitSystem
 */
void UXTHEME_UnInitSystem(HINSTANCE hInst)
{
    UXTHEME_DeleteParseErrorInfo();

    TlsFree(gdwErrorInfoTlsIndex);
    gdwErrorInfoTlsIndex = TLS_OUT_OF_INDEXES;
}

/***********************************************************************
 *      IsAppThemed                                         (UXTHEME.@)
 */
BOOL WINAPI IsAppThemed(void)
{
    TRACE("\n");
    SetLastError(ERROR_SUCCESS);
    return (g_ActiveThemeFile != NULL);
}

/***********************************************************************
 *      IsThemeActive                                       (UXTHEME.@)
 */
BOOL WINAPI IsThemeActive(void)
{
    BOOL bActive;
    LRESULT Result;
    HKEY hKey;
    WCHAR tmp[10];
    DWORD buffsize;

    TRACE("IsThemeActive\n");
    SetLastError(ERROR_SUCCESS);

    if (g_ActiveThemeFile) 
        return TRUE;

    if (g_bThemeHooksActive)
        return FALSE;

    bActive = FALSE;
    Result = RegOpenKeyW(HKEY_CURRENT_USER, szThemeManager, &hKey);
    if (Result == ERROR_SUCCESS)
    {
        buffsize = sizeof(tmp);
        if (!RegQueryValueExW(hKey, szThemeActive, NULL, NULL, (LPBYTE)tmp, &buffsize)) 
            bActive = (tmp[0] != '0');
        RegCloseKey(hKey);
    }

    return bActive;
}

/***********************************************************************
 *      EnableTheming                                       (UXTHEME.@)
 *
 * NOTES
 * This is a global and persistent change
 */
HRESULT WINAPI EnableTheming(BOOL fEnable)
{
    HKEY hKey;
    WCHAR szEnabled[] = {'0','\0'};

    TRACE("(%d)\n", fEnable);

    if (fEnable != (g_ActiveThemeFile != NULL)) {
        if(fEnable) 
            UXTHEME_BackupSystemMetrics();
        else
            UXTHEME_RestoreSystemMetrics();
        UXTHEME_SaveSystemMetrics ();

        if (fEnable) szEnabled[0] = '1';
        if(!RegOpenKeyW(HKEY_CURRENT_USER, szThemeManager, &hKey)) {
            RegSetValueExW(hKey, szThemeActive, 0, REG_SZ, (LPBYTE)szEnabled, sizeof(WCHAR));
            RegCloseKey(hKey);
        }
	UXTHEME_broadcast_theme_changed (NULL, fEnable);
    }
    return S_OK;
}

/***********************************************************************
 *      UXTHEME_SetWindowProperty
 *
 * I'm using atoms as there may be large numbers of duplicated strings
 * and they do the work of keeping memory down as a cause of that quite nicely
 */
static HRESULT UXTHEME_SetWindowProperty(HWND hwnd, ATOM aProp, LPCWSTR pszValue)
{
    ATOM oldValue = (ATOM)(size_t)RemovePropW(hwnd, (LPCWSTR)MAKEINTATOM(aProp));
    if(oldValue)
    {
        DeleteAtom(oldValue);
    }

    if(pszValue) 
    {
        ATOM atValue;

        /* A string with zero lenght is not acceptatble in AddAtomW but we want to support
           users passing an empty string meaning they want no theme for the specified window */
        if(!pszValue[0])
            pszValue = L"0";

        atValue = AddAtomW(pszValue);
        if (!atValue)
        {
            ERR("AddAtomW for %S failed with last error %d!\n", pszValue, GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (!SetPropW(hwnd, (LPCWSTR)MAKEINTATOM(aProp), (LPWSTR)MAKEINTATOM(atValue)))
        {
            ERR("SetPropW for atom %d failed with last error %d\n", aProp, GetLastError());
            if(atValue) DeleteAtom(atValue);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return S_OK;
}

static LPWSTR UXTHEME_GetWindowProperty(HWND hwnd, ATOM aProp, LPWSTR pszBuffer, int dwLen)
{
    ATOM atValue = (ATOM)(size_t)GetPropW(hwnd, (LPCWSTR)MAKEINTATOM(aProp));
    if(atValue) {
        if(GetAtomNameW(atValue, pszBuffer, dwLen))
            return pszBuffer;
        TRACE("property defined, but unable to get value\n");
    }
    return NULL;
}

PTHEME_CLASS ValidateHandle(HTHEME hTheme)
{
    PUXTHEME_HANDLE pHandle;

    if (!g_bThemeHooksActive || !hTheme || hTheme == INVALID_HANDLE_VALUE)
        return NULL;

    if (!RtlIsValidHandle(&g_UxThemeHandleTable, (PRTL_HANDLE_TABLE_ENTRY)hTheme))
    {
        ERR("Invalid handle 0x%x!\n", hTheme);
        return NULL;
    }

    pHandle = hTheme;
    return pHandle->pClass;
}

static HTHEME WINAPI
OpenThemeDataInternal(PTHEME_FILE ThemeFile, HWND hwnd, LPCWSTR pszClassList, DWORD flags)
{
    WCHAR szAppBuff[256];
    WCHAR szClassBuff[256];
    LPCWSTR pszAppName;
    LPCWSTR pszUseClassList;
    HTHEME hTheme = NULL;
    TRACE("(%p,%s, %x)\n", hwnd, debugstr_w(pszClassList), flags);

    if(!pszClassList)
    {
        SetLastError(E_POINTER);
        return NULL;
    }

    if ((flags & OTD_NONCLIENT) && !(dwThemeAppProperties & STAP_ALLOW_NONCLIENT))
    {
        SetLastError(E_PROP_ID_UNSUPPORTED);
        return NULL;
    }

    if (!(flags & OTD_NONCLIENT) && !(dwThemeAppProperties & STAP_ALLOW_CONTROLS))
    {
        SetLastError(E_PROP_ID_UNSUPPORTED);
        return NULL;
    }

    if (ThemeFile)
    {
        pszAppName = UXTHEME_GetWindowProperty(hwnd, atSubAppName, szAppBuff, sizeof(szAppBuff)/sizeof(szAppBuff[0]));
        /* If SetWindowTheme was used on the window, that overrides the class list passed to this function */
        pszUseClassList = UXTHEME_GetWindowProperty(hwnd, atSubIdList, szClassBuff, sizeof(szClassBuff)/sizeof(szClassBuff[0]));
        if(!pszUseClassList)
            pszUseClassList = pszClassList;

         if (pszUseClassList)
         {
            PTHEME_CLASS pClass;
            PUXTHEME_HANDLE pHandle;

             if (!ThemeFile->classes)
                 MSSTYLES_ParseThemeIni(ThemeFile);
            pClass = MSSTYLES_OpenThemeClass(ThemeFile, pszAppName, pszUseClassList);

            if (pClass)
            {
                pHandle = (PUXTHEME_HANDLE)RtlAllocateHandle(&g_UxThemeHandleTable, NULL);
                if (pHandle)
                {
                    g_cHandles++;
                    TRACE("Created handle 0x%x for class 0x%x, app %S, class %S. Count: %d\n", pHandle, pClass, pszAppName, pszUseClassList, g_cHandles);
                    pHandle->pClass = pClass;
                    pHandle->Handle.Flags = RTL_HANDLE_VALID;
                    hTheme = pHandle;
                }
                else
                {
                    MSSTYLES_CloseThemeClass(pClass);
                }
            }
        }
    }

    if(IsWindow(hwnd))
    {
        if ((flags & OTD_NONCLIENT) == 0)
        {
            SetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atWindowTheme), hTheme);
        }
    }
    else
    {
        SetLastError(E_PROP_ID_UNSUPPORTED);
    }

    SetLastError(hTheme ? ERROR_SUCCESS : E_PROP_ID_UNSUPPORTED);

    TRACE(" = %p\n", hTheme);
    return hTheme;
}

/***********************************************************************
 *      OpenThemeDataEx                                     (UXTHEME.61)
 */
HTHEME WINAPI OpenThemeDataEx(HWND hwnd, LPCWSTR pszClassList, DWORD flags)
{
    return OpenThemeDataInternal(g_ActiveThemeFile, hwnd, pszClassList, flags);
}

/***********************************************************************
 *      OpenThemeDataFromFile                               (UXTHEME.16)
 */
HTHEME WINAPI OpenThemeDataFromFile(HTHEMEFILE hThemeFile, HWND hwnd, LPCWSTR pszClassList, DWORD flags)
{
    return OpenThemeDataInternal((PTHEME_FILE)hThemeFile, hwnd, pszClassList, flags);
}

/***********************************************************************
 *      OpenThemeData                                       (UXTHEME.@)
 */
HTHEME WINAPI OpenThemeData(HWND hwnd, LPCWSTR classlist)
{
    return OpenThemeDataInternal(g_ActiveThemeFile, hwnd, classlist, 0);
}

/***********************************************************************
 *      GetWindowTheme                                      (UXTHEME.@)
 *
 * Retrieve the last theme opened for a window.
 *
 * PARAMS
 *  hwnd  [I] window to retrieve the theme for
 *
 * RETURNS
 *  The most recent theme.
 */
HTHEME WINAPI GetWindowTheme(HWND hwnd)
{
    TRACE("(%p)\n", hwnd);

	if(!IsWindow(hwnd))
    {
		SetLastError(E_HANDLE);
        return NULL;
    }

    return GetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atWindowTheme));
}

/***********************************************************************
 *      SetWindowTheme                                      (UXTHEME.@)
 *
 * Persistent through the life of the window, even after themes change
 */
HRESULT WINAPI SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName,
                              LPCWSTR pszSubIdList)
{
    HRESULT hr;
    TRACE("(%p,%s,%s)\n", hwnd, debugstr_w(pszSubAppName),
        debugstr_w(pszSubIdList));
    
    if(!IsWindow(hwnd))
        return E_HANDLE;

    hr = UXTHEME_SetWindowProperty(hwnd, atSubAppName, pszSubAppName);
    if (!SUCCEEDED(hr))
        return hr;

    hr = UXTHEME_SetWindowProperty(hwnd, atSubIdList, pszSubIdList);
    if (!SUCCEEDED(hr))
        return hr;

    UXTHEME_broadcast_theme_changed (hwnd, TRUE);
    return hr;
}

/***********************************************************************
 *      GetCurrentThemeName                                 (UXTHEME.@)
 */
HRESULT WINAPI GetCurrentThemeName(LPWSTR pszThemeFileName, int dwMaxNameChars,
                                   LPWSTR pszColorBuff, int cchMaxColorChars,
                                   LPWSTR pszSizeBuff, int cchMaxSizeChars)
{
    int cchar;

    if(g_ActiveThemeFile == NULL)
         return E_PROP_ID_UNSUPPORTED;

    if (pszThemeFileName && dwMaxNameChars) 
    {
        cchar = lstrlenW(g_ActiveThemeFile->szThemeFile) + 1;
        if(cchar > dwMaxNameChars)
           return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        lstrcpynW(pszThemeFileName, g_ActiveThemeFile->szThemeFile, cchar);
    }

    if (pszColorBuff && cchMaxColorChars) 
    {
        cchar = lstrlenW(g_ActiveThemeFile->pszSelectedColor) + 1;
        if(cchar > cchMaxColorChars)
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        lstrcpynW(pszColorBuff, g_ActiveThemeFile->pszSelectedColor, cchar);
    }

   if (pszSizeBuff && cchMaxSizeChars) 
    {
        cchar = lstrlenW(g_ActiveThemeFile->pszSelectedSize) + 1;
        if(cchar > cchMaxSizeChars)
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        lstrcpynW(pszSizeBuff, g_ActiveThemeFile->pszSelectedSize, cchar);
    }

    return S_OK;
}

/***********************************************************************
 *      GetThemeAppProperties                               (UXTHEME.@)
 */
DWORD WINAPI GetThemeAppProperties(void)
{
    return dwThemeAppProperties;
}

/***********************************************************************
 *      SetThemeAppProperties                               (UXTHEME.@)
 */
void WINAPI SetThemeAppProperties(DWORD dwFlags)
{
    TRACE("(0x%08x)\n", dwFlags);
    dwThemeAppProperties = dwFlags;
}

/***********************************************************************
 *      CloseThemeData                                      (UXTHEME.@)
 */
HRESULT WINAPI CloseThemeData(HTHEME hTheme)
{
    PUXTHEME_HANDLE pHandle = hTheme;
    HRESULT hr;

    TRACE("(%p)\n", hTheme);

    if (!RtlIsValidHandle(&g_UxThemeHandleTable, (PRTL_HANDLE_TABLE_ENTRY)hTheme))
        return E_HANDLE;

    hr = MSSTYLES_CloseThemeClass(pHandle->pClass);
    if (SUCCEEDED(hr))
    {
        RtlFreeHandle(&g_UxThemeHandleTable, (PRTL_HANDLE_TABLE_ENTRY)pHandle);
        g_cHandles--;
        TRACE("Destroying handle 0x%x for class 0x%x. Count: %d\n", pHandle, pHandle->pClass, g_cHandles);
    }
    return hr;
}

/***********************************************************************
 *      HitTestThemeBackground                              (UXTHEME.@)
 */
HRESULT WINAPI HitTestThemeBackground(HTHEME hTheme, HDC hdc, int iPartId,
                                     int iStateId, DWORD dwOptions,
                                     const RECT *pRect, HRGN hrgn,
                                     POINT ptTest, WORD *pwHitTestCode)
{
    FIXME("%d %d 0x%08x: stub\n", iPartId, iStateId, dwOptions);
    if (!ValidateHandle(hTheme))
        return E_HANDLE;
    return E_NOTIMPL;
}

/***********************************************************************
 *      IsThemePartDefined                                  (UXTHEME.@)
 */
BOOL WINAPI IsThemePartDefined(HTHEME hTheme, int iPartId, int iStateId)
{
    PTHEME_CLASS pClass;

    TRACE("(%p,%d,%d)\n", hTheme, iPartId, iStateId);

    pClass = ValidateHandle(hTheme);
    if (!pClass)
    {
        SetLastError(E_HANDLE);
        return FALSE;
    }
    if(MSSTYLES_FindPartState(pClass, iPartId, iStateId, NULL))
        return TRUE;
    return FALSE;
}

/***********************************************************************
 *      GetThemeDocumentationProperty                       (UXTHEME.@)
 *
 * Try and retrieve the documentation property from string resources
 * if that fails, get it from the [documentation] section of themes.ini
 */
HRESULT WINAPI GetThemeDocumentationProperty(LPCWSTR pszThemeName,
                                             LPCWSTR pszPropertyName,
                                             LPWSTR pszValueBuff,
                                             int cchMaxValChars)
{
    const WORD wDocToRes[] = {
        TMT_DISPLAYNAME,5000,
        TMT_TOOLTIP,5001,
        TMT_COMPANY,5002,
        TMT_AUTHOR,5003,
        TMT_COPYRIGHT,5004,
        TMT_URL,5005,
        TMT_VERSION,5006,
        TMT_DESCRIPTION,5007
    };

    PTHEME_FILE pt;
    HRESULT hr;
    unsigned int i;
    int iDocId;
    TRACE("(%s,%s,%p,%d)\n", debugstr_w(pszThemeName), debugstr_w(pszPropertyName),
          pszValueBuff, cchMaxValChars);

    if (!g_bThemeHooksActive)
        return E_FAIL;

    hr = MSSTYLES_OpenThemeFile(pszThemeName, NULL, NULL, &pt);
    if(FAILED(hr)) return hr;

    /* Try to load from string resources */
    hr = E_PROP_ID_UNSUPPORTED;
    if(MSSTYLES_LookupProperty(pszPropertyName, NULL, &iDocId)) {
        for(i=0; i<sizeof(wDocToRes)/sizeof(wDocToRes[0]); i+=2) {
            if(wDocToRes[i] == iDocId) {
                if(LoadStringW(pt->hTheme, wDocToRes[i+1], pszValueBuff, cchMaxValChars)) {
                    hr = S_OK;
                    break;
                }
            }
        }
    }
    /* If loading from string resource failed, try getting it from the theme.ini */
    if(FAILED(hr)) {
        PUXINI_FILE uf = MSSTYLES_GetThemeIni(pt);
        if(UXINI_FindSection(uf, szIniDocumentation)) {
            LPCWSTR lpValue;
            DWORD dwLen;
            if(UXINI_FindValue(uf, pszPropertyName, &lpValue, &dwLen)) {
                lstrcpynW(pszValueBuff, lpValue, min(dwLen+1,cchMaxValChars));
                hr = S_OK;
            }
        }
        UXINI_CloseINI(uf);
    }

    MSSTYLES_CloseThemeFile(pt);
    return hr;
}

/**********************************************************************
 *      Undocumented functions
 */

/**********************************************************************
 *      QueryThemeServices                                 (UXTHEME.1)
 *
 * RETURNS
 *     some kind of status flag
 */
DWORD WINAPI QueryThemeServices(void)
{
    FIXME("stub\n");
    return 3; /* This is what is returned under XP in most cases */
}


/**********************************************************************
 *      OpenThemeFile                                      (UXTHEME.2)
 *
 * Opens a theme file, which can be used to change the current theme, etc
 *
 * PARAMS
 *     pszThemeFileName    Path to a msstyles theme file
 *     pszColorName        Color defined in the theme, eg. NormalColor
 *     pszSizeName         Size defined in the theme, eg. NormalSize
 *     hThemeFile          Handle to theme file
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 */
HRESULT WINAPI OpenThemeFile(LPCWSTR pszThemeFileName, LPCWSTR pszColorName,
                             LPCWSTR pszSizeName, HTHEMEFILE *hThemeFile,
                             DWORD unknown)
{
    TRACE("(%s,%s,%s,%p,%d)\n", debugstr_w(pszThemeFileName),
          debugstr_w(pszColorName), debugstr_w(pszSizeName),
          hThemeFile, unknown);

    if (!g_bThemeHooksActive)
        return E_FAIL;

    return MSSTYLES_OpenThemeFile(pszThemeFileName, pszColorName, pszSizeName, (PTHEME_FILE*)hThemeFile);
}

/**********************************************************************
 *      CloseThemeFile                                     (UXTHEME.3)
 *
 * Releases theme file handle returned by OpenThemeFile
 *
 * PARAMS
 *     hThemeFile           Handle to theme file
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 */
HRESULT WINAPI CloseThemeFile(HTHEMEFILE hThemeFile)
{
    TRACE("(%p)\n", hThemeFile);
    MSSTYLES_CloseThemeFile(hThemeFile);
    return S_OK;
}

/**********************************************************************
 *      ApplyTheme                                         (UXTHEME.4)
 *
 * Set a theme file to be the currently active theme
 *
 * PARAMS
 *     hThemeFile           Handle to theme file
 *     unknown              See notes
 *     hWnd                 Window requesting the theme change
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 *
 * NOTES
 * I'm not sure what the second parameter is (the datatype is likely wrong), other then this:
 * Under XP if I pass
 * char b[] = "";
 *   the theme is applied with the screen redrawing really badly (flickers)
 * char b[] = "\0"; where \0 can be one or more of any character, makes no difference
 *   the theme is applied smoothly (screen does not flicker)
 * char *b = "\0" or NULL; where \0 can be zero or more of any character, makes no difference
 *   the function fails returning invalid parameter... very strange
 */
HRESULT WINAPI ApplyTheme(HTHEMEFILE hThemeFile, char *unknown, HWND hWnd)
{
    HRESULT hr;
    TRACE("(%p,%s,%p)\n", hThemeFile, unknown, hWnd);
    hr = UXTHEME_ApplyTheme(hThemeFile);
    UXTHEME_broadcast_theme_changed (NULL, (g_ActiveThemeFile != NULL));
    return hr;
}

/**********************************************************************
 *      GetThemeDefaults                                   (UXTHEME.7)
 *
 * Get the default color & size for a theme
 *
 * PARAMS
 *     pszThemeFileName    Path to a msstyles theme file
 *     pszColorName        Buffer to receive the default color name
 *     dwColorNameLen      Length, in characters, of color name buffer
 *     pszSizeName         Buffer to receive the default size name
 *     dwSizeNameLen       Length, in characters, of size name buffer
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 */
HRESULT WINAPI GetThemeDefaults(LPCWSTR pszThemeFileName, LPWSTR pszColorName,
                                DWORD dwColorNameLen, LPWSTR pszSizeName,
                                DWORD dwSizeNameLen)
{
    PTHEME_FILE pt;
    HRESULT hr;
    TRACE("(%s,%p,%d,%p,%d)\n", debugstr_w(pszThemeFileName),
          pszColorName, dwColorNameLen,
          pszSizeName, dwSizeNameLen);

    if (!g_bThemeHooksActive)
        return E_FAIL;

    hr = MSSTYLES_OpenThemeFile(pszThemeFileName, NULL, NULL, &pt);
    if(FAILED(hr)) return hr;

    lstrcpynW(pszColorName, pt->pszSelectedColor, dwColorNameLen);
    lstrcpynW(pszSizeName, pt->pszSelectedSize, dwSizeNameLen);

    MSSTYLES_CloseThemeFile(pt);
    return S_OK;
}

/**********************************************************************
 *      EnumThemes                                         (UXTHEME.8)
 *
 * Enumerate available themes, calls specified EnumThemeProc for each
 * theme found. Passes lpData through to callback function.
 *
 * PARAMS
 *     pszThemePath        Path containing themes
 *     callback            Called for each theme found in path
 *     lpData              Passed through to callback
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 */
HRESULT WINAPI EnumThemes(LPCWSTR pszThemePath, ENUMTHEMEPROC callback,
                          LPVOID lpData)
{
    WCHAR szDir[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    static const WCHAR szStar[] = {'*','.','*','\0'};
    static const WCHAR szFormat[] = {'%','s','%','s','\\','%','s','.','m','s','s','t','y','l','e','s','\0'};
    static const WCHAR szDisplayName[] = {'d','i','s','p','l','a','y','n','a','m','e','\0'};
    static const WCHAR szTooltip[] = {'t','o','o','l','t','i','p','\0'};
    WCHAR szName[60];
    WCHAR szTip[60];
    HANDLE hFind;
    WIN32_FIND_DATAW wfd;
    HRESULT hr;
    size_t pathLen;

    TRACE("(%s,%p,%p)\n", debugstr_w(pszThemePath), callback, lpData);

    if(!pszThemePath || !callback)
        return E_POINTER;

    lstrcpyW(szDir, pszThemePath);
    pathLen = lstrlenW (szDir);
    if ((pathLen > 0) && (pathLen < MAX_PATH-1) && (szDir[pathLen - 1] != '\\'))
    {
        szDir[pathLen] = '\\';
        szDir[pathLen+1] = 0;
    }

    lstrcpyW(szPath, szDir);
    lstrcatW(szPath, szStar);
    TRACE("searching %s\n", debugstr_w(szPath));

    hFind = FindFirstFileW(szPath, &wfd);
    if(hFind != INVALID_HANDLE_VALUE) {
        do {
            if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
               && !(wfd.cFileName[0] == '.' && ((wfd.cFileName[1] == '.' && wfd.cFileName[2] == 0) || wfd.cFileName[1] == 0))) {
                wsprintfW(szPath, szFormat, szDir, wfd.cFileName, wfd.cFileName);

                hr = GetThemeDocumentationProperty(szPath, szDisplayName, szName, sizeof(szName)/sizeof(szName[0]));
                if(FAILED(hr))
                {
                    ERR("Failed to get theme name from %S\n", szPath);
                    continue;
                }

                hr = GetThemeDocumentationProperty(szPath, szTooltip, szTip, sizeof(szTip)/sizeof(szTip[0]));
                if (FAILED(hr))
                    szTip[0] = 0;

                TRACE("callback(%s,%s,%s,%p)\n", debugstr_w(szPath), debugstr_w(szName), debugstr_w(szTip), lpData);
                if(!callback(NULL, szPath, szName, szTip, NULL, lpData)) {
                    TRACE("callback ended enum\n");
                    break;
                }
            }
        } while(FindNextFileW(hFind, &wfd));
        FindClose(hFind);
    }
    return S_OK;
}


/**********************************************************************
 *      EnumThemeColors                                    (UXTHEME.9)
 *
 * Enumerate theme colors available with a particular size
 *
 * PARAMS
 *     pszThemeFileName    Path to a msstyles theme file
 *     pszSizeName         Theme size to enumerate available colors
 *                         If NULL the default theme size is used
 *     dwColorNum          Color index to retrieve, increment from 0
 *     pszColorNames       Output color names
 *
 * RETURNS
 *     S_OK on success
 *     E_PROP_ID_UNSUPPORTED when dwColorName does not refer to a color
 *          or when pszSizeName does not refer to a valid size
 *
 * NOTES
 * XP fails with E_POINTER when pszColorNames points to a buffer smaller than 
 * sizeof(THEMENAMES).
 *
 * Not very efficient that I'm opening & validating the theme every call, but
 * this is undocumented and almost never called..
 * (and this is how windows works too)
 */
HRESULT WINAPI EnumThemeColors(LPWSTR pszThemeFileName, LPWSTR pszSizeName,
                               DWORD dwColorNum, PTHEMENAMES pszColorNames)
{
    PTHEME_FILE pt;
    HRESULT hr;
    LPWSTR tmp;
    UINT resourceId = dwColorNum + 1000;
    TRACE("(%s,%s,%d)\n", debugstr_w(pszThemeFileName),
          debugstr_w(pszSizeName), dwColorNum);

    if (!g_bThemeHooksActive)
        return E_FAIL;

    hr = MSSTYLES_OpenThemeFile(pszThemeFileName, NULL, pszSizeName, &pt);
    if(FAILED(hr)) return hr;

    tmp = pt->pszAvailColors;
    while(dwColorNum && *tmp) {
        dwColorNum--;
        tmp += lstrlenW(tmp)+1;
    }
    if(!dwColorNum && *tmp) {
        TRACE("%s\n", debugstr_w(tmp));
        lstrcpyW(pszColorNames->szName, tmp);
        LoadStringW (pt->hTheme, resourceId,
            pszColorNames->szDisplayName,
            sizeof (pszColorNames->szDisplayName) / sizeof (WCHAR));
        LoadStringW (pt->hTheme, resourceId+1000,
            pszColorNames->szTooltip,
            sizeof (pszColorNames->szTooltip) / sizeof (WCHAR));
    }
    else
        hr = E_PROP_ID_UNSUPPORTED;

    MSSTYLES_CloseThemeFile(pt);
    return hr;
}

/**********************************************************************
 *      EnumThemeSizes                                     (UXTHEME.10)
 *
 * Enumerate theme colors available with a particular size
 *
 * PARAMS
 *     pszThemeFileName    Path to a msstyles theme file
 *     pszColorName        Theme color to enumerate available sizes
 *                         If NULL the default theme color is used
 *     dwSizeNum           Size index to retrieve, increment from 0
 *     pszSizeNames        Output size names
 *
 * RETURNS
 *     S_OK on success
 *     E_PROP_ID_UNSUPPORTED when dwSizeName does not refer to a size
 *          or when pszColorName does not refer to a valid color
 *
 * NOTES
 * XP fails with E_POINTER when pszSizeNames points to a buffer smaller than 
 * sizeof(THEMENAMES).
 *
 * Not very efficient that I'm opening & validating the theme every call, but
 * this is undocumented and almost never called..
 * (and this is how windows works too)
 */
HRESULT WINAPI EnumThemeSizes(LPWSTR pszThemeFileName, LPWSTR pszColorName,
                              DWORD dwSizeNum, PTHEMENAMES pszSizeNames)
{
    PTHEME_FILE pt;
    HRESULT hr;
    LPWSTR tmp;
    UINT resourceId = dwSizeNum + 3000;
    TRACE("(%s,%s,%d)\n", debugstr_w(pszThemeFileName),
          debugstr_w(pszColorName), dwSizeNum);

    if (!g_bThemeHooksActive)
        return E_FAIL;

    hr = MSSTYLES_OpenThemeFile(pszThemeFileName, pszColorName, NULL, &pt);
    if(FAILED(hr)) return hr;

    tmp = pt->pszAvailSizes;
    while(dwSizeNum && *tmp) {
        dwSizeNum--;
        tmp += lstrlenW(tmp)+1;
    }
    if(!dwSizeNum && *tmp) {
        TRACE("%s\n", debugstr_w(tmp));
        lstrcpyW(pszSizeNames->szName, tmp);
        LoadStringW (pt->hTheme, resourceId,
            pszSizeNames->szDisplayName,
            sizeof (pszSizeNames->szDisplayName) / sizeof (WCHAR));
        LoadStringW (pt->hTheme, resourceId+1000,
            pszSizeNames->szTooltip,
            sizeof (pszSizeNames->szTooltip) / sizeof (WCHAR));
    }
    else
        hr = E_PROP_ID_UNSUPPORTED;

    MSSTYLES_CloseThemeFile(pt);
    return hr;
}

/**********************************************************************
 *      ParseThemeIniFile                                  (UXTHEME.11)
 *
 * Enumerate data in a theme INI file.
 *
 * PARAMS
 *     pszIniFileName      Path to a theme ini file
 *     pszUnknown          Cannot be NULL, L"" is valid
 *     callback            Called for each found entry
 *     lpData              Passed through to callback
 *
 * RETURNS
 *     S_OK on success
 *     0x800706488 (Unknown property) when enumeration is canceled from callback
 *
 * NOTES
 * When pszUnknown is NULL the callback is never called, the value does not seem to serve
 * any other purpose
 */
HRESULT WINAPI ParseThemeIniFile(LPCWSTR pszIniFileName, LPWSTR pszUnknown,
                                 PARSETHEMEINIFILEPROC callback, LPVOID lpData)
{
    FIXME("%s %s: stub\n", debugstr_w(pszIniFileName), debugstr_w(pszUnknown));
    return E_NOTIMPL;
}

/**********************************************************************
 *      CheckThemeSignature                                (UXTHEME.29)
 *
 * Validates the signature of a theme file
 *
 * PARAMS
 *     pszIniFileName      Path to a theme file
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 */
HRESULT WINAPI CheckThemeSignature(LPCWSTR pszThemeFileName)
{
    PTHEME_FILE pt;
    HRESULT hr;
    TRACE("(%s)\n", debugstr_w(pszThemeFileName));

    if (!g_bThemeHooksActive)
        return E_FAIL;

    hr = MSSTYLES_OpenThemeFile(pszThemeFileName, NULL, NULL, &pt);
    if(FAILED(hr))
        return hr;
    MSSTYLES_CloseThemeFile(pt);
    return S_OK;
}
