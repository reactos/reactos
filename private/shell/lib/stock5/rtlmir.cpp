/****************************** Module*Header *****************************\
* Module Name: rtlmir.c                                                    *
*                                                                          *
* This module contains all the Right-To-Left (RTL) Mirroring support       *
* routines which are used across the whole IShell project. It abstracts    *
* platform-support routines of RTL mirroring (NT5 and Memphis) and removes *
* linkage depedenency with the Mirroring APIs.                             *
*                                                                          *
* Functions prefixed with Mirror, deal with the new Mirroring APIs         *
*                                                                          *
*                                                                          *
* Created: 01-Feb-1998 8:41:18 pm                                          *
* Author: Samer Arafeh [samera]                                            *
*                                                                          *
* Copyright (c) 1998 Microsoft Corporation                                 *
\**************************************************************************/


#include "local.h"

const DWORD dwNoMirrorBitmap = NOMIRRORBITMAP;
const DWORD dwExStyleRTLMirrorWnd = WS_EX_LAYOUTRTL;
const DWORD dwExStyleNoInheritLayout = WS_EX_NOINHERITLAYOUT; 
const DWORD dwPreserveBitmap = LAYOUT_BITMAPORIENTATIONPRESERVED;

/*
 * Remove linkage dependecy for the RTL mirroring APIs, by retreiving
 * their addresses at runtime.
 */
typedef DWORD (WINAPI *PFNGETLAYOUT)(HDC);                   // gdi32!GetLayout
typedef DWORD (WINAPI *PFNSETLAYOUT)(HDC, DWORD);            // gdi32!SetLayout
typedef BOOL  (WINAPI *PFNSETPROCESSDEFLAYOUT)(DWORD);       // user32!SetProcessDefaultLayout
typedef BOOL  (WINAPI *PFNGETPROCESSDEFLAYOUT)(DWORD*);      // user32!GetProcessDefaultLayout
typedef LANGID (WINAPI *PFNGETUSERDEFAULTUILANGUAGE)(void);  // kernel32!GetUserDefaultUILanguage
typedef BOOL (WINAPI *PFNENUMUILANGUAGES)(UILANGUAGE_ENUMPROC, DWORD, LONG_PTR); // kernel32!EnumUILanguages

typedef struct {
    LANGID LangID;
    BOOL   bInstalled;
    } MUIINSTALLLANG, *LPMUIINSTALLLANG;

#ifdef UNICODE
#define ConvertHexStringToInt ConvertHexStringToIntW
#else
#define ConvertHexStringToInt ConvertHexStringToIntA
#endif


/***************************************************************************\
* ConvertHexStringToIntA
*
* Converts a hex numeric string into an integer.
*
* History:
* 04-Feb-1998 samera    Created
\***************************************************************************/
BOOL ConvertHexStringToIntA( CHAR *pszHexNum , int *piNum )
{
    int   n=0L;
    CHAR  *psz=pszHexNum;

  
    for(n=0 ; ; psz=CharNextA(psz))
    {
        if( (*psz>='0') && (*psz<='9') )
            n = 0x10 * n + *psz - '0';
        else
        {
            CHAR ch = *psz;
            int n2;

            if(ch >= 'a')
                ch -= 'a' - 'A';

            n2 = ch - 'A' + 0xA;
            if (n2 >= 0xA && n2 <= 0xF)
                n = 0x10 * n + n2;
            else
                break;
        }
    }

    /*
     * Update results
     */
    *piNum = n;

    return (psz != pszHexNum);
}

/***************************************************************************\
* ConvertHexStringToIntW
*
* Converts a hex numeric string into an integer.
*
* History:
* 14-June-1998 msadek    Created
\***************************************************************************/
BOOL ConvertHexStringToIntW( WCHAR *pszHexNum , int *piNum )
{
    int   n=0L;
    WCHAR  *psz=pszHexNum;

  
    for(n=0 ; ; psz=CharNextW(psz))
    {
        if( (*psz>='0') && (*psz<='9') )
            n = 0x10 * n + *psz - '0';
        else
        {
            WCHAR ch = *psz;
            int n2;

            if(ch >= 'a')
                ch -= 'a' - 'A';

            n2 = ch - 'A' + 0xA;
            if (n2 >= 0xA && n2 <= 0xF)
                n = 0x10 * n + n2;
            else
                break;
        }
    }

    /*
     * Update results
     */
    *piNum = n;

    return (psz != pszHexNum);
}


/***************************************************************************\
* IsBiDiLocalizedSystemEx
*
* returns TRUE if running on a lozalized BiDi (Arabic/Hebrew) NT5 or Memphis.
* Should be called whenever SetProcessDefaultLayout is to be called.
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL IsBiDiLocalizedSystemEx( LANGID *pLangID )
{
    HKEY          hKey;
    DWORD         dwType;
    CHAR          szResourceLocale[12];
    DWORD         dwSize = sizeof(szResourceLocale)/sizeof(CHAR);
    int           iLCID=0L;
    static BOOL   bRet = (BOOL)(DWORD)-1;
    static LANGID langID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    if (bRet != (BOOL)(DWORD)-1)
    {
        if (bRet && pLangID)
        {
            *pLangID = langID;
        }
        return bRet;
    }

    bRet = FALSE;
    if( staticIsOS( OS_NT5 ) )
    {
        /*
         * Need to use NT5 detection method (Multiligual UI ID)
         */
        langID = Mirror_GetUserDefaultUILanguage();

        if( langID )
        {
            WCHAR wchLCIDFontSignature[16];
            iLCID = MAKELCID( langID , SORT_DEFAULT );

            /*
             * Let's verify this is a RTL (BiDi) locale. Since reg value is a hex string, let's
             * convert to decimal value and call GetLocaleInfo afterwards.
             * LOCALE_FONTSIGNATURE always gives back 16 WCHARs.
             */

            if( GetLocaleInfoW( iLCID , 
                                LOCALE_FONTSIGNATURE , 
                                (WCHAR *) &wchLCIDFontSignature[0] ,
                                (sizeof(wchLCIDFontSignature)/sizeof(WCHAR))) )
            {
      
                /* Let's verify the bits we have a BiDi UI locale */
                if(( wchLCIDFontSignature[7] & (WCHAR)0x0800) && Mirror_IsUILanguageInstalled(langID) )
                {
                    bRet = TRUE;
                }
            }
        }
    } else {

        /*
         * Check if BiDi-Memphis is running with Lozalized Resources (
         * i.e. Arabic/Hebrew systems) -It should be enabled ofcourse-.
         */
        if( (staticIsOS(OS_MEMPHIS)) && (GetSystemMetrics(SM_MIDEASTENABLED)) )
        {

            if( RegOpenKeyExA( HKEY_CURRENT_USER , 
                               "Control Panel\\Desktop\\ResourceLocale" , 
                               0, 
                               KEY_READ, &hKey) == ERROR_SUCCESS) 
            {
                RegQueryValueExA( hKey , "" , 0 , &dwType , (LPBYTE)szResourceLocale , &dwSize );
                szResourceLocale[(sizeof(szResourceLocale)/sizeof(CHAR))-1] = 0;

                RegCloseKey(hKey);

                if( ConvertHexStringToIntA( szResourceLocale , &iLCID ) )
                {
                    iLCID = PRIMARYLANGID(LANGIDFROMLCID(iLCID));
                    if( (LANG_ARABIC == iLCID) || (LANG_HEBREW == iLCID) )
                    {
                        bRet = TRUE;
                        langID = LANGIDFROMLCID(iLCID);
                    }
                }
            }
        }
    }

    if (bRet && pLangID)
    {
        *pLangID = langID;
    }
    return bRet;
}

BOOL IsBiDiLocalizedSystem( void )
{
    return IsBiDiLocalizedSystemEx(NULL);
}

/***************************************************************************\
* IsBiDiLocalizedWin95
*
* returns TRUE if running on a lozalized BiDi (Arabic/Hebrew) Win95.
* Needed for legacy operating system check for needed RTL UI elements
* For example, DefView ListView, TreeView,...etc 
* History:
* 12-June-1998 a-msadek    Created
\***************************************************************************/
BOOL IsBiDiLocalizedWin95(BOOL bArabicOnly)
{
    HKEY  hKey;
    DWORD dwType;
    BOOL  bRet = FALSE;
    CHAR  szResourceLocale[12];
    DWORD dwSize = sizeof(szResourceLocale)/sizeof(CHAR);
    int   iLCID=0L;

         /*
         * Check if BiDi-Win95 is running with Lozalized Resources (
         * i.e. Arabic/Hebrew systems) -It should be enabled ofcourse-.
         */
        if( (staticIsOS(OS_WIN95)) && (!staticIsOS(OS_MEMPHIS)) && (GetSystemMetrics(SM_MIDEASTENABLED)) )
        {

            if( RegOpenKeyExA( HKEY_CURRENT_USER , 
                               "Control Panel\\Desktop\\ResourceLocale" , 
                               0, 
                               KEY_READ, &hKey) == ERROR_SUCCESS) 
            {
                RegQueryValueExA( hKey , "" , 0 , &dwType , (LPBYTE)szResourceLocale , &dwSize );
                szResourceLocale[(sizeof(szResourceLocale)/sizeof(CHAR))-1] = 0;

                RegCloseKey(hKey);

                if( ConvertHexStringToIntA( szResourceLocale , &iLCID ) )
                {
                    iLCID = PRIMARYLANGID(LANGIDFROMLCID(iLCID));
                    //
                    //If bArabicOnly we will return true if it a Arabic Win95 localized. 
                    //
                    if( (LANG_ARABIC == iLCID) || ((LANG_HEBREW == iLCID) && !bArabicOnly ))
                    {
                        bRet = TRUE;
                    }
                }
            }
        }
    
    return bRet;
}

/***************************************************************************\
* Mirror_IsEnabledOS
*
* returns TRUE if the mirroring APIs are enabled on the current OS.
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL Mirror_IsEnabledOS( void )
{
    BOOL bRet = FALSE;

    if( staticIsOS(OS_NT5) )
    {
        bRet = TRUE;
    } else if( staticIsOS(OS_MEMPHIS) && GetSystemMetrics(SM_MIDEASTENABLED)) {
        bRet=TRUE;
    }

    return bRet;
}


/***************************************************************************\
* Mirror_GetUserDefaultUILanguage
*
* Reads the User UI language on NT5
*
* History:
* 22-June-1998 samera    Created
\***************************************************************************/
LANGID Mirror_GetUserDefaultUILanguage( void )
{
    LANGID langId=0;
    static PFNGETUSERDEFAULTUILANGUAGE pfnGetUserDefaultUILanguage=NULL;

    if( NULL == pfnGetUserDefaultUILanguage )
    {
        HMODULE hmod = GetModuleHandleA("KERNEL32");

        if( hmod )
            pfnGetUserDefaultUILanguage = (PFNGETUSERDEFAULTUILANGUAGE)
                                          GetProcAddress(hmod, "GetUserDefaultUILanguage");
    }

    if( pfnGetUserDefaultUILanguage )
        langId = pfnGetUserDefaultUILanguage();

    return langId;
}

/***************************************************************************\
* Mirror_IsUILanguageInstalled
*
* Verifies that the User UI language is installed on W2k
*
* History:
* 14-June-1999 msadek    Created
\***************************************************************************/
BOOL Mirror_IsUILanguageInstalled( LANGID langId )
{

    MUIINSTALLLANG MUILangInstalled = {0};
    MUILangInstalled.LangID = langId;
    
    static PFNENUMUILANGUAGES pfnEnumUILanguages=NULL;

    if( NULL == pfnEnumUILanguages )
    {
        HMODULE hmod = GetModuleHandleA("KERNEL32");

        if( hmod )
            pfnEnumUILanguages = (PFNENUMUILANGUAGES)
                                          GetProcAddress(hmod, "EnumUILanguagesW");
    }

    if( pfnEnumUILanguages )
        pfnEnumUILanguages(Mirror_EnumUILanguagesProc, 0, (LONG_PTR)&MUILangInstalled);

    return MUILangInstalled.bInstalled;
}

/***************************************************************************\
* Mirror_EnumUILanguagesProc
*
* Enumerates MUI installed languages on W2k
* History:
* 14-June-1999 msadek    Created
\***************************************************************************/

BOOL CALLBACK Mirror_EnumUILanguagesProc(LPTSTR lpUILanguageString, LONG_PTR lParam)
{
    int langID = 0;

    ConvertHexStringToInt(lpUILanguageString, &langID);

    if((LANGID)langID == ((LPMUIINSTALLLANG)lParam)->LangID)
    {
        ((LPMUIINSTALLLANG)lParam)->bInstalled = TRUE;
        return FALSE;
    }
    return TRUE;
}


/***************************************************************************\
* Mirror_IsWindowMirroredRTL
*
* returns TRUE if the window is RTL mirrored
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL Mirror_IsWindowMirroredRTL( HWND hWnd )
{
    return (GetWindowLongA( hWnd , GWL_EXSTYLE ) & WS_EX_LAYOUTRTL );
}




/***************************************************************************\
* Mirror_GetLayout
*
* returns TRUE if the hdc is RTL mirrored
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
DWORD Mirror_GetLayout( HDC hdc )
{
    DWORD dwRet=0;
    static PFNGETLAYOUT pfnGetLayout=NULL;

    if( NULL == pfnGetLayout )
    {
        HMODULE hmod = GetModuleHandleA("GDI32");

        if( hmod )
            pfnGetLayout = (PFNGETLAYOUT)GetProcAddress(hmod, "GetLayout");
    }

    if( pfnGetLayout )
        dwRet = pfnGetLayout( hdc );

    return dwRet;
}

DWORD Mirror_IsDCMirroredRTL( HDC hdc )
{
    return (Mirror_GetLayout( hdc ) & LAYOUT_RTL);
}



/***************************************************************************\
* Mirror_SetLayout
*
* RTL Mirror the hdc
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
DWORD Mirror_SetLayout( HDC hdc , DWORD dwLayout )
{
    DWORD dwRet=0;
    static PFNSETLAYOUT pfnSetLayout=NULL;

    if( NULL == pfnSetLayout )
    {
        HMODULE hmod = GetModuleHandleA("GDI32");

        if( hmod )
            pfnSetLayout = (PFNSETLAYOUT)GetProcAddress(hmod, "SetLayout");
    }

    if( pfnSetLayout )
        dwRet = pfnSetLayout( hdc , dwLayout );

    return dwRet;
}

DWORD Mirror_MirrorDC( HDC hdc )
{
    return Mirror_SetLayout( hdc , LAYOUT_RTL );
}


/***************************************************************************\
* Mirror_SetProcessDefaultLayout
*
* Set the process-default layout.
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL Mirror_SetProcessDefaultLayout( DWORD dwDefaultLayout )
{
    BOOL bRet=0;
    static PFNSETPROCESSDEFLAYOUT pfnSetProcessDefLayout=NULL;

    if( NULL == pfnSetProcessDefLayout )
    {
        HMODULE hmod = GetModuleHandleA("USER32");

        if( hmod )
            pfnSetProcessDefLayout = (PFNSETPROCESSDEFLAYOUT)
                                     GetProcAddress(hmod, "SetProcessDefaultLayout");
    }

    if( pfnSetProcessDefLayout )
        bRet = pfnSetProcessDefLayout( dwDefaultLayout );

    return bRet;
}

BOOL Mirror_MirrorProcessRTL( void )
{
    return Mirror_SetProcessDefaultLayout( LAYOUT_RTL );
}


/***************************************************************************\
* Mirror_GetProcessDefaultLayout
*
* Get the process-default layout.
*
* History:
* 26-Feb-1998 samera    Created
\***************************************************************************/
BOOL Mirror_GetProcessDefaultLayout( DWORD *pdwDefaultLayout )
{
    BOOL bRet=0;
    static PFNGETPROCESSDEFLAYOUT pfnGetProcessDefLayout=NULL;

    if( NULL == pfnGetProcessDefLayout )
    {
        HMODULE hmod = GetModuleHandleA("USER32");

        if( hmod )
            pfnGetProcessDefLayout = (PFNGETPROCESSDEFLAYOUT)
                                     GetProcAddress(hmod, "GetProcessDefaultLayout");
    }

    if( pfnGetProcessDefLayout )
        bRet = pfnGetProcessDefLayout( pdwDefaultLayout );

    return bRet;
}

BOOL Mirror_IsProcessRTL( void )
{
    DWORD dwDefLayout=0;

    return (Mirror_GetProcessDefaultLayout(&dwDefLayout) && (dwDefLayout&LAYOUT_RTL));
}

////////////////////////////////////////////////////////////////////////////
// Skip_IDorString
//
// Skips string (or ID) and returns the next aligned WORD.
////////////////////////////////////////////////////////////////////////////
PBYTE Skip_IDorString(LPBYTE pb)
{
    LPWORD pw = (LPWORD)pb;

    if (*pw == 0xFFFF)
        return (LPBYTE)(pw + 2);

    while (*pw++ != 0)
        ;

    return (LPBYTE)pw;
}

////////////////////////////////////////////////////////////////////////////
// Skip_DialogHeader
//
// Skips the dialog header and returns the next aligned WORD. 
////////////////////////////////////////////////////////////////////////////
PBYTE Skip_DialogHeader(LPDLGTEMPLATE pdt)
{
    LPBYTE pb;

    pb = (LPBYTE)(pdt + 1);

    // If there is a menu ordinal, add 4 bytes skip it. Otherwise it is a string or just a 0.
    pb = Skip_IDorString(pb);

    // Skip window class and window text, adjust to next word boundary.
    pb = Skip_IDorString(pb);    // class
    pb = Skip_IDorString(pb);    // window text

    // Skip font type, size and name, adjust to next dword boundary.
    if (pdt->style & DS_SETFONT)
    {
        pb += sizeof(WORD);
        pb = Skip_IDorString(pb);
    }
    pb = (LPBYTE)(((ULONG_PTR)pb + 3) & ~3);    // DWORD align

    return pb;
}

////////////////////////////////////////////////////////////////////////////
// EditBiDiDLGTemplate
//
// Edits a dialog template for BiDi stuff.
// Optionally, skipping some controls.
// Works only with DLGTEMPLATE.
////////////////////////////////////////////////////////////////////////////
void EditBiDiDLGTemplate(LPDLGTEMPLATE pdt, DWORD dwFlags, PWORD pwIgnoreList, int cIgnore)
{
    LPBYTE pb;
    UINT cItems;

    if (!pdt)
        return;
    // we should never get an extended template
    ASSERT (((LPDLGTEMPLATEEX)pdt)->wSignature != 0xFFFF);
    
    if(dwFlags & EBDT_NOMIRROR)
    {
        // Turn off the mirroring styles for the dialog.
        pdt->dwExtendedStyle &= ~(WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT);
    }
    cItems = pdt->cdit;

    // skip DLGTEMPLATE part
    pb = Skip_DialogHeader(pdt);

    while (cItems--)
    {
        UINT cbCreateParams;
        int i = 0;
        BOOL bIgnore = FALSE;

        if(pwIgnoreList && cIgnore)
        {
            for(i = 0;i < cIgnore; i++)
            {
                if((((LPDLGITEMTEMPLATE)pb)->id == *(pwIgnoreList +i)))
                {
                    bIgnore = TRUE;
                }
            }
        }
        
        if((dwFlags & EBDT_NOMIRROR) && !bIgnore)
        {
            // Turn off the mirroring styles for this item.
            ((LPDLGITEMTEMPLATE)pb)->dwExtendedStyle &= ~(WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT); 
        }    

        if((dwFlags & EBDT_FLIP) && !bIgnore)
        {
            ((LPDLGITEMTEMPLATE)pb)->x = pdt->cx - (((LPDLGITEMTEMPLATE)pb)->x + ((LPDLGITEMTEMPLATE)pb)->cx);
            // BUGBUG: Should we force RTL reading order for title as well ?
            // The client has the option of doining this already by PSH_RTLREADING
        }
        pb += sizeof(DLGITEMTEMPLATE);

        // Skip the dialog control class name.
        pb = Skip_IDorString(pb);

        // Look at window text now.
        pb = Skip_IDorString(pb);

        cbCreateParams = *((LPWORD)pb);

        // skip any CreateParams which include the generated size WORD.
        if (cbCreateParams)
            pb += cbCreateParams;

        pb += sizeof(WORD);

        // Point at the next dialog item. (DWORD aligned)
        pb = (LPBYTE)(((ULONG_PTR)pb + 3) & ~3);

        bIgnore = FALSE;
    }
}




