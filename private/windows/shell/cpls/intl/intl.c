/*++

Copyright (c) 1994-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    intl.c

Abstract:

    This module contains the main routines for the Regional Options applet.

Revision History:

--*/



//
//  Include Files.
//

#include "intl.h"
#include <cpl.h>


//
//  Constant Declarations.
//

#define MAX_PAGES 6          // limit on the number of pages

#define LANGUAGE_PACK_KEY    TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\LanguagePack")
#define LANGUAGE_PACK_VALUE  TEXT("COMPLEXSCRIPTS")
#define LANGUAGE_PACK_DLL    TEXT("lpk.dll")

static const TCHAR c_szLanguages[] =
    TEXT("System\\CurrentControlSet\\Control\\Nls\\Language");

static const TCHAR c_szControlPanelIntl[] =
    TEXT("Control Panel\\International");




//
//  Global Variables.
//

HANDLE g_hMutex = NULL;
TCHAR szMutexName[] = TEXT("RegionalSettings_InputLocaleMutex");

HANDLE g_hEvent = NULL;
TCHAR szEventName[] = TEXT("RegionalSettings_InputLocaleEvent");

TCHAR aInt_Str[cInt_Str][3] = { TEXT("0"),
                                TEXT("1"),
                                TEXT("2"),
                                TEXT("3"),
                                TEXT("4"),
                                TEXT("5"),
                                TEXT("6"),
                                TEXT("7"),
                                TEXT("8"),
                                TEXT("9")
                              };

TCHAR szSample_Number[] = TEXT("123456789.00");
TCHAR szNegSample_Number[] = TEXT("-123456789.00");
TCHAR szTimeChars[]  = TEXT(" Hhmst,-./:;\\ ");
TCHAR szTCaseSwap[]  = TEXT("   MST");
TCHAR szTLetters[]   = TEXT("Hhmst");
TCHAR szSDateChars[] = TEXT(" dgMy,-./:;\\ ");
TCHAR szSDCaseSwap[] = TEXT(" DGmY");
TCHAR szSDLetters[]  = TEXT("dgMy");
TCHAR szLDateChars[] = TEXT(" dgMy,-./:;\\");
TCHAR szLDCaseSwap[] = TEXT(" DGmY");
TCHAR szLDLetters[]  = TEXT("dgHhMmsty");
TCHAR szStyleH[3];
TCHAR szStyleh[3];
TCHAR szStyleM[3];
TCHAR szStylem[3];
TCHAR szStyles[3];
TCHAR szStylet[3];
TCHAR szStyled[3];
TCHAR szStyley[3];
TCHAR szLocaleGetError[SIZE_128];
TCHAR szIntl[] = TEXT("intl");

TCHAR szInvalidSDate[] = TEXT("Mdyg'");
TCHAR szInvalidSTime[] = TEXT("Hhmst'");

HINSTANCE hInstance;
int Verified_Regional_Chg;
BOOL Styles_Localized;
LCID UserLocaleID;
LCID SysLocaleID;
LCID RegUserLocaleID;
LCID RegSysLocaleID;
BOOL bShowArabic;
BOOL bShowRtL;
BOOL bLPKInstalled;
TCHAR szSetupSourcePath[MAX_PATH];
TCHAR szSetupSourcePathWithArchitecture[MAX_PATH];
LPTSTR pSetupSourcePath = NULL;
LPTSTR pSetupSourcePathWithArchitecture = NULL;


BOOL g_bCDROM = FALSE;




//
//  Function Prototypes.
//

void
DoProperties(
    HWND hwnd,
    LPCTSTR pCmdLine);





////////////////////////////////////////////////////////////////////////////
//
//  LibMain
//
//  This routine is called from LibInit to perform any initialization that
//  is required.
//
////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY LibMain(
    HANDLE hDll,
    DWORD dwReason,
    LPVOID lpReserved)
{
    switch (dwReason)
    {
        case ( DLL_PROCESS_ATTACH ) :
        {
            hInstance = hDll;

            //
            //  Create the mutex used for the Input Locale property page.
            //
            g_hMutex = CreateMutex(NULL, FALSE, szMutexName);
            g_hEvent = CreateEvent(NULL, TRUE, TRUE, szEventName);

            DisableThreadLibraryCalls(hDll);

            break;
        }
        case ( DLL_PROCESS_DETACH ) :
        {
            if (g_hMutex)
            {
                CloseHandle(g_hMutex);
            }
            if (g_hEvent)
            {
                CloseHandle(g_hEvent);
            }
            break;
        }

        case ( DLL_THREAD_DETACH ) :
        {
            break;
        }

        case ( DLL_THREAD_ATTACH ) :
        default :
        {
            break;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateGlobals
//
////////////////////////////////////////////////////////////////////////////

BOOL CreateGlobals()
{
    HKEY hKey;
    TCHAR szData[MAX_PATH];
    DWORD cbData;

    //
    //  Get the localized strings.
    //
    LoadString(hInstance, IDS_LOCALE_GET_ERROR, szLocaleGetError, SIZE_128);
    LoadString(hInstance, IDS_STYLEUH,          szStyleH,         3);
    LoadString(hInstance, IDS_STYLELH,          szStyleh,         3);
    LoadString(hInstance, IDS_STYLEUM,          szStyleM,         3);
    LoadString(hInstance, IDS_STYLELM,          szStylem,         3);
    LoadString(hInstance, IDS_STYLELS,          szStyles,         3);
    LoadString(hInstance, IDS_STYLELT,          szStylet,         3);
    LoadString(hInstance, IDS_STYLELD,          szStyled,         3);
    LoadString(hInstance, IDS_STYLELY,          szStyley,         3);

    Styles_Localized = (szStyleH[0] != TEXT('H') || szStyleh[0] != TEXT('h') ||
                        szStyleM[0] != TEXT('M') || szStylem[0] != TEXT('m') ||
                        szStyles[0] != TEXT('s') || szStylet[0] != TEXT('t') ||
                        szStyled[0] != TEXT('d') || szStyley[0] != TEXT('y'));

    //
    //  Initialize globals.
    //
    Verified_Regional_Chg = 0;

    //
    //  Get the user and system default locale ids.
    //
    UserLocaleID = GetUserDefaultLCID();
    SysLocaleID = GetSystemDefaultLCID();

    //
    //  Get the system locale id from the registry.  This may be
    //  different from the current system default locale id if the user
    //  changed the system locale and chose not to reboot.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szLanguages,
                      0L,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        //
        //  Query the default locale id.
        //
        szData[0] = 0;
        cbData = sizeof(szData);
        RegQueryValueEx(hKey, TEXT("Default"), NULL, NULL, (LPBYTE)szData, &cbData);
        RegCloseKey(hKey);

        if ((RegSysLocaleID = TransNum(szData)) == 0)
        {
            RegSysLocaleID = SysLocaleID;
        }
    }
    else
    {
        RegSysLocaleID = SysLocaleID;
    }

    //
    //  Get the user locale id from the registry.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      c_szControlPanelIntl,
                      0L,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        //
        //  Query the locale id.
        //
        szData[0] = 0;
        cbData = sizeof(szData);
        RegQueryValueEx(hKey, TEXT("Locale"), NULL, NULL, (LPBYTE)szData, &cbData);
        RegCloseKey(hKey);

        if ((RegUserLocaleID = TransNum(szData)) == 0)
        {
            RegUserLocaleID = UserLocaleID;
        }
    }
    else
    {
        RegUserLocaleID = UserLocaleID;
    }

    //
    //  See if the user locale id is Arabic or/and right to left.
    //
    bShowRtL = IsRtLLocale(UserLocaleID);
    bShowArabic = (bShowRtL &&
                   (PRIMARYLANGID(LANGIDFROMLCID(UserLocaleID)) != LANG_HEBREW));

    //
    //  See if there is an LPK installed.
    //
    if (GetModuleHandle(LANGUAGE_PACK_DLL))
    {
        bLPKInstalled = TRUE;
    }
    else
    {
        bLPKInstalled = FALSE;
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DestroyGlobals
//
////////////////////////////////////////////////////////////////////////////

void DestroyGlobals()
{
}


////////////////////////////////////////////////////////////////////////////
//
//  CPlApplet
//
////////////////////////////////////////////////////////////////////////////

LONG CALLBACK CPlApplet(
    HWND hwnd,
    UINT Msg,
    LPARAM lParam1,
    LPARAM lParam2)
{
    switch (Msg)
    {
        case ( CPL_INIT ) :
        {
            //
            //  First message to CPlApplet(), sent once only.
            //  Perform all control panel applet initialization and return
            //  true for further processing.
            //
            return (CreateGlobals());
        }
        case ( CPL_GETCOUNT ) :
        {
            //
            //  Second message to CPlApplet(), sent once only.
            //  Return the number of control applets to be displayed in the
            //  control panel window.  For this applet, return 1.
            //
            return (1);
        }
        case ( CPL_INQUIRE ) :
        {
            //
            //  Third message to CPlApplet().
            //  It is sent as many times as the number of applets returned by
            //  CPL_GETCOUNT message.  Each applet must register by filling
            //  in the CPLINFO structure referenced by lParam2 with the
            //  applet's icon, name, and information string.  Since there is
            //  only one applet, simply set the information for this
            //  singular case.
            //
            LPCPLINFO lpCPlInfo = (LPCPLINFO)lParam2;

            lpCPlInfo->idIcon = IDI_ICON;
            lpCPlInfo->idName = IDS_NAME;
            lpCPlInfo->idInfo = IDS_INFO;
            lpCPlInfo->lData  = 0;

            break;
        }
        case ( CPL_NEWINQUIRE ) :
        {
            //
            //  Third message to CPlApplet().
            //  It is sent as many times as the number of applets returned by
            //  CPL_GETCOUNT message.  Each applet must register by filling
            //  in the NEWCPLINFO structure referenced by lParam2 with the
            //  applet's icon, name, and information string.  Since there is
            //  only one applet, simply set the information for this
            //  singular case.
            //
            LPNEWCPLINFO lpNewCPlInfo = (LPNEWCPLINFO)lParam2;

            lpNewCPlInfo->dwSize = sizeof(NEWCPLINFO);
            lpNewCPlInfo->dwFlags = 0;
            lpNewCPlInfo->dwHelpContext = 0UL;
            lpNewCPlInfo->lData = 0;
            lpNewCPlInfo->hIcon = LoadIcon( hInstance,
                                            (LPCTSTR)MAKEINTRESOURCE(IDI_ICON) );
            LoadString(hInstance, IDS_NAME, lpNewCPlInfo->szName, 32);
            LoadString(hInstance, IDS_INFO, lpNewCPlInfo->szInfo, 64);
            lpNewCPlInfo->szHelpFile[0] = CHAR_NULL;

            break;
        }
        case ( CPL_SELECT ) :
        {
            //
            //  Applet has been selected, do nothing.
            //
            break;
        }
        case ( CPL_DBLCLK ) :
        {
            //
            //  Applet icon double clicked -- invoke property sheet with
            //  the first property sheet page on top.
            //
            DoProperties(hwnd, (LPCTSTR)NULL);
            break;
        }
        case ( CPL_STARTWPARMS ) :
        {
            //
            //  Same as CPL_DBLCLK, but lParam2 is a long pointer to
            //  a string of extra directions that are to be supplied to
            //  the property sheet that is to be initiated.
            //
            DoProperties(hwnd, (LPCTSTR)lParam2);
            break;
        }
        case ( CPL_STOP ) :
        {
            //
            //  Sent once for each applet prior to the CPL_EXIT msg.
            //  Perform applet specific cleanup.
            //
            break;
        }
        case ( CPL_EXIT ) :
        {
            //
            //  Last message, sent once only, before MMCPL.EXE calls
            //  FreeLibrary() on this DLL.  Do non-applet specific cleanup.
            //
            DestroyGlobals();
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  AddPage
//
////////////////////////////////////////////////////////////////////////////

void AddPage(
    LPPROPSHEETHEADER ppsh,
    UINT id,
    DLGPROC pfn,
    LPARAM lParam)
{
    if (ppsh->nPages < MAX_PAGES)
    {
        PROPSHEETPAGE psp;

        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(id);
        psp.pfnDlgProc = pfn;
        psp.lParam = lParam;

        ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
        if (ppsh->phpage[ppsh->nPages])
        {
            ppsh->nPages++;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  DoProperties
//
////////////////////////////////////////////////////////////////////////////

void DoProperties(
    HWND hwnd,
    LPCTSTR pCmdLine)
{
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    LPARAM lParam = SETUP_SWITCH_NONE;
    LPTSTR pStartPage;
    LPTSTR pSrc;
    LPTSTR pSrcDrv;
    BOOL bUnattend = FALSE;
    BOOL bInSetupMode = FALSE;
    BOOL bShortDate = FALSE;
    BOOL bNoUI = FALSE;
    TCHAR szUnattendFile[MAX_PATH * 2];
    HKEY hkey;
    TCHAR szSetupSourceDrive[MAX_PATH];
    BOOL bProgressBarDisplay = FALSE;

    //
    //  See if there is a command line switch from Setup.
    //
    psh.nStartPage = (UINT)-1;
    while (pCmdLine && *pCmdLine)
    {
        if (*pCmdLine == TEXT('/'))
        {
            //
            //  Legend:
            //    gG: allow progress bar to show when setup is copying files
            //    iI: bring up the Input Locale page only
            //    rR: bring up the General page on top
            //    sS: setup source string passed on command line
            //            [example: /s:"c:\winnt"]
            //
            //  NO UI IS SHOWN IF THE FOLLOWING OPTIONS ARE SPECIFIED:
            //    fF: unattend mode file - no UI is shown
            //            [example: /f:"c:\unattend.txt"]
            //    uU: update short date format to 4-digit year - no UI is shown
            //        (registry only updated if current setting is the
            //         same as the default setting except for the
            //         "yy" vs. "yyyy")
            //
            switch (*++pCmdLine)
            {
                case ( TEXT('g') ) :
                case ( TEXT('G') ) :
                {
                    bProgressBarDisplay = TRUE;
                    pCmdLine++;
                    break;
                }
                case ( TEXT('i') ) :
                case ( TEXT('I') ) :
                {
                    if (lParam == SETUP_SWITCH_NONE)
                    {
                        lParam = SETUP_SWITCH_I;
                    }
                    psh.nStartPage = 0;
                    pCmdLine++;
                    break;
                }
                case ( TEXT('r') ) :
                case ( TEXT('R') ) :
                {
                    lParam = SETUP_SWITCH_R;
                    psh.nStartPage = 0;
                    pCmdLine++;
                    break;
                }
                case ( TEXT('s') ) :
                case ( TEXT('S') ) :
                {
                    //
                    //  Get the name of the setup source path.
                    //
                    bInSetupMode = TRUE;
                    if ((*++pCmdLine == TEXT(':')) && (*++pCmdLine == TEXT('"')))
                    {
                        pCmdLine++;
                        pSrc = szSetupSourcePath;
                        pSrcDrv = szSetupSourceDrive;
                        while (*pCmdLine && (*pCmdLine != TEXT('"')))
                        {
                            *pSrc = *pCmdLine;
                            pSrc++;
                            *pSrcDrv = *pCmdLine;
                            pSrcDrv++;
                            pCmdLine++;
                        }
                        *pSrc = 0;
                        *pSrcDrv = 0;
                        wcscpy(szSetupSourcePathWithArchitecture, szSetupSourcePath);
                        pSetupSourcePathWithArchitecture = szSetupSourcePathWithArchitecture;

                        //
                        //  Remove the architecture-specific portion of
                        //  the source path (that gui-mode setup sent us).
                        //
                        pSrc = wcsrchr(szSetupSourcePath, TEXT('\\'));
                        if (pSrc)
                        {
                            *pSrc = TEXT('\0');
                        }
                        pSetupSourcePath = szSetupSourcePath;
                    }
                    if (*pCmdLine == TEXT('"'))
                    {
                        pCmdLine++;
                    }
                    pSrcDrv = szSetupSourceDrive;
                    while (*pSrcDrv)
                    {
                        if (*pSrcDrv == TEXT('\\'))
                        {
                            pSrcDrv[1] = 0;
                        }
                        pSrcDrv++;
                    }
                    g_bCDROM = (GetDriveType(szSetupSourceDrive) == DRIVE_CDROM);
                    break;
                }
                case ( TEXT('f') ) :
                case ( TEXT('F') ) :
                {
                    //
                    //  Get the name of the unattend file.
                    //
                    bInSetupMode = TRUE;
                    bUnattend = TRUE;
                    bNoUI = TRUE;
                    szUnattendFile[0] = 0;
                    if ((*++pCmdLine == TEXT(':')) && (*++pCmdLine == TEXT('"')))
                    {
                        pCmdLine++;
                        pSrc = szUnattendFile;
                        while (*pCmdLine && (*pCmdLine != TEXT('"')))
                        {
                            *pSrc = *pCmdLine;
                            pSrc++;
                            pCmdLine++;
                        }
                        *pSrc = 0;
                    }
                    if (*pCmdLine == TEXT('"'))
                    {
                        pCmdLine++;
                    }
                    break;
                }
                case ( TEXT('u') ) :
                case ( TEXT('U') ) :
                {
                    bShortDate = TRUE;
                    bNoUI = TRUE;
                    break;
                }
                default :
                {
                    //
                    //  Fall out, maybe it's a number...
                    //
                    break;
                }
            }
        }
        else if (*pCmdLine == TEXT(' '))
        {
            pCmdLine++;
        }
        else
        {
            break;
        }
    }

    //
    //  If we're in setup mode, we need to remove the hard coded LPK
    //  registry key.
    //
    if (bInSetupMode)
    {
        if (RegOpenKey( HKEY_LOCAL_MACHINE,
                        LANGUAGE_PACK_KEY,
                        &hkey ) == ERROR_SUCCESS)
        {
            RegDeleteValue(hkey, LANGUAGE_PACK_VALUE);
            RegCloseKey(hkey);
        }
    }

    //
    //  If the unattend mode file switch was used, use the unattend mode file
    //  to carry out the appropriate commands.
    //
    if (bUnattend)
    {
        Region_DoUnattendModeSetup(szUnattendFile, bProgressBarDisplay);
    }

    //
    //  If the update to 4-digit year switch was used and the user's short
    //  date setting is still set to the default for the chosen locale, then
    //  update the current user's short date setting to the new 4-digit year
    //  default.
    //
    if (bShortDate)
    {
        Region_UpdateShortDate();
    }

    //
    // This is the best place to validate the user's Keyboard Layout 
    // Preload section.
    // Especially after language groups have been installed by 
    // "Region_DoUnattendModeSetup" during gui setup.
    //
    Region_ValidateRegistryPreload();

    //
    //  If we're not to show any UI, then return.
    //
    if (bNoUI)
    {
        return;
    }

    //
    //  Make sure we have a start page.
    //
    if (psh.nStartPage == (UINT)-1)
    {
        psh.nStartPage = 0;
        if (pCmdLine && *pCmdLine)
        {
            //
            //  Get the start page from the command line.
            //
            pStartPage = (LPTSTR)pCmdLine;
            while ((*pStartPage >= TEXT('0')) && (*pStartPage <= TEXT('9')))
            {
                psh.nStartPage *= 10;
                psh.nStartPage += *pStartPage++ - CHAR_ZERO;
            }

            //
            //  Make sure that the requested starting page is less than
            //  the max page for the selected applet.
            //
            if (psh.nStartPage >= MAX_PAGES)
            {
                psh.nStartPage = 0;
            }
        }
    }

    //
    //  Set up the property sheet information.
    //
    psh.dwSize = sizeof(psh);
    psh.dwFlags = 0;
    psh.hwndParent = hwnd;
    psh.hInstance = hInstance;
    psh.pszCaption = MAKEINTRESOURCE(IDS_NAME);
    psh.nPages = 0;
    psh.phpage = rPages;

    //
    //  Add the appropriate property pages.
    //
    switch (lParam)
    {
        case ( SETUP_SWITCH_I ) :
        {
            AddPage(&psh, DLG_KEYBOARD_LOCALES, LocaleDlgProc, lParam);
            break;
        }
        default :
        {
            AddPage(&psh, DLG_GENERAL, GeneralDlgProc, lParam);
            AddPage(&psh, DLG_NUMBER, NumberDlgProc, lParam);
            AddPage(&psh, DLG_CURRENCY, CurrencyDlgProc, lParam);
            AddPage(&psh, DLG_TIME, TimeDlgProc, lParam);
            AddPage(&psh, DLG_DATE, DateDlgProc, lParam);
            AddPage(&psh, DLG_KEYBOARD_LOCALES, LocaleDlgProc, lParam);
            break;
        }
    }

    //
    //  Make the property sheet.
    //
    PropertySheet(&psh);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsRtLLocale
//
////////////////////////////////////////////////////////////////////////////

#define MAX_FONTSIGNATURE    16   // length of font signature string

BOOL IsRtLLocale(
    LCID iLCID)
{
    WORD wLCIDFontSignature[MAX_FONTSIGNATURE];
    BOOL bRet = FALSE;

    //
    //  Verify that this is an RTL (BiDi) locale.  Call GetLocaleInfo with
    //  LOCALE_FONTSIGNATURE which always gives back 16 WORDs.
    //
    if (GetLocaleInfo( iLCID,
                       LOCALE_FONTSIGNATURE,
                       (LPTSTR) &wLCIDFontSignature,
                       (sizeof(wLCIDFontSignature) / sizeof(TCHAR)) ))
    {
        //
        //  Verify the bits show a BiDi UI locale.
        //
        if (wLCIDFontSignature[7] & 0x0800)
        {
            bRet = TRUE;
        }
    }

    return (bRet);
}
