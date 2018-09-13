/*++

Copyright (c) 1994-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    regdlg.c

Abstract:

    This module implements the region property sheet for the Regional
    Settings applet.

Revision History:

--*/



//
//  Include Files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "intl.h"
#include "winnlsp.h"
#include <windowsx.h>
#include <regstr.h>
#include <tchar.h>
#include <stdlib.h>
#include <setupapi.h>
#include <syssetup.h>
#include <winuserp.h>
#include <userenv.h>
#include "intlhlp.h"
#include "maxvals.h"
#include "locdlg.h"



//
//  Context Help Ids.
//

static int aRegionHelpIds[] =
{
    IDC_USER_LOCALE,      IDH_INTL_USER_LOCALE,
    IDC_SORTING_TEXT,     IDH_INTL_SORTING,
    IDC_SORTING,          IDH_INTL_SORTING,
    IDC_UI_LANGUAGE_TEXT, IDH_INTL_UI_LANGUAGE,
    IDC_UI_LANGUAGE,      IDH_INTL_UI_LANGUAGE,

    IDC_LANGUAGE_GROUPS,  IDH_INTL_INSTALL_LANG_GROUPS,

    IDC_SET_DEFAULT,      IDH_INTL_SYSTEM_LOCALE_BUTTON,

    IDC_ADVANCED,         IDH_INTL_ADVANCED,

    0, 0
};

static int aRegionSetDefaultHelpIds[] =
{
    IDC_SYSTEM_LOCALE_TEXT1,   NO_HELP,
    IDC_SYSTEM_LOCALE_TEXT2,   NO_HELP,
    IDC_SYSTEM_LOCALE,         IDH_INTL_SYSTEM_LOCALE,

    0, 0
};

static int aRegionAdvancedHelpIds[] =
{
    IDC_GROUPBOX1,        IDH_COMM_GROUPBOX,
    IDC_CODEPAGES,        IDH_INTL_INSTALL_CP_TABLES,

    0, 0
};




//
//  Constant Declarations.
//

#define RMI_PRIMARY          (0x1)     // this should win in event of conflict

#define ARRAYSIZE(a)         (sizeof(a) / sizeof(a[0]))

#define LANG_SPANISH_TRADITIONAL (MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH))
#define LANG_SPANISH_INTL        (MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN))
#define LCID_SPANISH_TRADITIONAL (MAKELCID(LANG_SPANISH_TRADITIONAL, SORT_DEFAULT))
#define LCID_SPANISH_INTL        (MAKELCID(LANG_SPANISH_INTL, SORT_DEFAULT))

#define ML_ORIG_INSTALLED    0x0001
#define ML_PERMANENT         0x0002
#define ML_INSTALL           0x0004
#define ML_REMOVE            0x0008
#define ML_DEFAULT           0x0010
#define ML_DISABLE           0x0020

#define ML_STATIC            (ML_PERMANENT | ML_DEFAULT | ML_DISABLE)

#define MAX_UI_LANG_GROUPS   64




//
//  Typedef Declarations.
//

typedef struct
{
    LPARAM Changes;           // flags to denote changes
    DWORD dwCurUserLocale;    // index of current user locale setting in combo box
    DWORD dwCurSorting;       // index of current sorting setting in combo box
    DWORD dwCurUILang;        // index of current UI Language setting in combo box
    DWORD dwLastUserLocale;   // index of the last user locale setting in combo box
    DWORD dwLastSorting;      // index of the last sorting setting in combo box
    BOOL Admin_Privileges;    // if the user is an admin

} REGDLGDATA, *LPREGDLGDATA;

typedef struct languagegroup_s
{
    WORD wStatus;                   // status flags
    UINT LanguageGroup;             // language group value
    HANDLE hLanguageGroup;          // handle to free for this structure
    TCHAR pszName[MAX_PATH];        // name of language group
    UINT NumLocales;                // number of locales in pLocaleList
    LCID pLocaleList[MAX_PATH];     // ptr to locale list for this group
    UINT NumAltSorts;               // number of alternate sorts in pAltSortList
    LCID pAltSortList[MAX_PATH];    // ptr to alternate sorts for this group
    struct languagegroup_s *pNext;  // ptr to next language group node

} LANGUAGEGROUP, *LPLANGUAGEGROUP;

typedef struct codepage_s
{
    WORD wStatus;                   // status flags
    UINT CodePage;                  // code page value
    HANDLE hCodePage;               // handle to free for this structure
    TCHAR pszName[MAX_PATH];        // name of code page
    struct codepage_s *pNext;       // ptr to next code page node

} CODEPAGE, *LPCODEPAGE;

typedef struct layoutlist_s
{
    DWORD dwLocale;                 // input locale id
    DWORD dwLayout;                 // layout id
    DWORD dwSubst;                  // substitution key value
    BOOL bLoaded;                   // if the layout is already loaded
    BOOL bIME;                      // if the layout is an IME

} LAYOUTLIST, *LPLAYOUTLIST;

//
//  Language group of UI languages.
//
typedef struct UI_LangGroup_Structtag
{
    int iCount;
    LGRPID lgrp[MAX_UI_LANG_GROUPS];

} UILANGUAGEGROUP, *PUILANGUAGEGROUP;




//
//  Global Variables.
//

static BOOL g_bSetupCase = FALSE;

static const TCHAR c_szInstalledLocales[] =
    TEXT("System\\CurrentControlSet\\Control\\Nls\\Locale");

static const TCHAR c_szLanguageGroups[] =
    TEXT("System\\CurrentControlSet\\Control\\Nls\\Language Groups");

static const TCHAR c_szMUILanguages[] =
    TEXT("System\\CurrentControlSet\\Control\\Nls\\MUILanguages");

static const TCHAR c_szCPanelIntl[] =
    TEXT("Control Panel\\International");

static const TCHAR c_szMUIPolicyKeyPath[] =
    TEXT("Software\\Policies\\Microsoft\\Control Panel\\Desktop");

static const TCHAR c_szMUIValue[] =
    TEXT("MultiUILanguageId");

static const TCHAR c_szIntlRun[] =
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\IntlRun");

static const TCHAR c_szSysocmgr[] =
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\IntlRun.OC");

static TCHAR szIntlInf[]          = TEXT("intl.inf");
static TCHAR szLocaleListPrefix[] = TEXT("LOCALE_LIST_");
static TCHAR szLGInstallPrefix[]  = TEXT("LG_INSTALL_");
static TCHAR szLGRemovePrefix[]   = TEXT("LG_REMOVE_");
static TCHAR szCPInstallPrefix[]  = TEXT("CODEPAGE_INSTALL_");
static TCHAR szCPRemovePrefix[]   = TEXT("CODEPAGE_REMOVE_");
static TCHAR szKbdLayoutIds[]     = TEXT("KbdLayoutIds");

static TCHAR szRegionalSettings[] = TEXT("RegionalSettings");
static TCHAR szLanguageGroup[]    = TEXT("LanguageGroup");
static TCHAR szLanguage[]         = TEXT("Language");
static TCHAR szSystemLocale[]     = TEXT("SystemLocale");
static TCHAR szUserLocale[]       = TEXT("UserLocale");
static TCHAR szInputLocale[]      = TEXT("InputLocale");

static HINF g_hIntlInf = NULL;

static LPLANGUAGEGROUP pLanguageGroups = NULL;
static LPCODEPAGE pCodePages = NULL;

static int g_NumAltSorts = 0;
static HANDLE hAltSorts = NULL;
static LPDWORD pAltSorts = NULL;

static UINT g_TextGap;
static UINT g_TextHeight;
static UINT g_cxVScroll;

static HINSTANCE hDeskCPL = NULL;
static void (*pfnUpdateCharsetChanges)(void) = NULL;

//
//  Language group of UI languages.
//
static UILANGUAGEGROUP UILangGroup;




//
//  Function Prototypes.
//

BOOL
Region_LoadLanguageGroups(
    HWND hDlg,
    LPREGDLGDATA pDlgData);

BOOL
Region_SetupLanguageGroups(
    HWND hDlg,
    LPBOOL pbReboot,
    LPBOOL pbUserCancel);

VOID
Region_RebootTheSystem();

BOOL
Region_InstallKeyboardLayout(
    HWND hDlg,
    LCID Locale,
    LPTSTR pszLocale);

int
Region_ShowMsg(
    HWND hDlg,
    UINT iMsg,
    UINT iTitle,
    UINT iType,
    LPTSTR pString);

BOOL
Region_InstallUserLocale(
    LCID Locale);

BOOL
Region_InstallDefaultLayout(
    LCID Locale,
    LPTSTR pszLocale);

BOOL
Region_InstallAllLayouts(
    HINF hIntlInf,
    LCID Locale,
    LPTSTR pszLocale);

BOOL
Region_SetPreloadLocale(
    LCID Locale,
    HINF hIntlInf,
    DWORD dwLayout);

BOOL CALLBACK
Region_EnumUILanguagesProc(
    PWSTR pwszUILanguage,
    LONG_PTR lParam);

void
Region_RunRegApps(
    LPCTSTR pszRegKey);





////////////////////////////////////////////////////////////////////////////
//
//  Region_LoadDeskCPL
//
//  Loads desk.cpl in preparation to invoke it.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_LoadDeskCPL()
{
    if (!hDeskCPL)
    {
        hDeskCPL = LoadLibrary(TEXT("desk.cpl"));
    }

    if (hDeskCPL)
    {
        pfnUpdateCharsetChanges = (void (*)(void))
            GetProcAddress(hDeskCPL, "UpdateCharsetChanges");
    }

    return (pfnUpdateCharsetChanges != NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_InvokeDeskCPL
//
//  Invoke desk.cpl entry point "UpdateCharsetChanges".
//
////////////////////////////////////////////////////////////////////////////

void Region_InvokeDeskCPL()
{
    if (pfnUpdateCharsetChanges)
    {
        (*pfnUpdateCharsetChanges)();
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_FreeDeskCPL
//
//  Unload desk.cpl.
//
////////////////////////////////////////////////////////////////////////////

void Region_FreeDeskCPL()
{
    if (hDeskCPL)
    {
        FreeLibrary(hDeskCPL);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_EnumLocales
//
////////////////////////////////////////////////////////////////////////////

void Region_EnumLocales(
    HWND hDlg,
    HWND hLocale,
    BOOL EnumSystemLocales)
{
    LPLANGUAGEGROUP pLG;
    DWORD Locale, dwIndex;
    BOOL fSpanish = FALSE;
    UINT ctr;
    TCHAR szBuf[SIZE_300];
    DWORD dwLocaleACP;
    INT iRet = TRUE;

    //
    //  Go through the language groups to see which ones are installed.
    //  Display only the locales for the groups that are either already
    //  installed or the groups the user wants to be installed.
    //
    pLG = pLanguageGroups;
    while (pLG)
    {
        //
        //  If the language group is originally installed and not marked for
        //  removal OR is marked to be installed, then add the locales for
        //  this language group to the System and User combo boxes.
        //
        if ((pLG->wStatus & ML_INSTALL) ||
            ((pLG->wStatus & ML_ORIG_INSTALLED) && !(pLG->wStatus & ML_REMOVE)))
        {
            for (ctr = 0; ctr < pLG->NumLocales; ctr++)
            {
                //
                //  Save the locale id.
                //
                Locale = (pLG->pLocaleList)[ctr];

                //
                //  See if we need to special case Spanish.
                //
                if ((LANGIDFROMLCID(Locale) == LANG_SPANISH_TRADITIONAL) ||
                    (LANGIDFROMLCID(Locale) == LANG_SPANISH_INTL))
                {
                    //
                    //  If we've already displayed Spanish (Spain), then
                    //  don't display it again.
                    //
                    if (!fSpanish)
                    {
                        //
                        //  Add the Spanish locale to the list box.
                        //
                        if (LoadString(hInstance, IDS_SPANISH_NAME, szBuf, SIZE_300))
                        {
                            dwIndex = ComboBox_AddString(hLocale, szBuf);
                            ComboBox_SetItemData( hLocale,
                                                  dwIndex,
                                                  LCID_SPANISH_INTL );

                            fSpanish = TRUE;
                        }
                    }
                }
                else
                {
                    //
                    //  Don't enum system locales that don't have an ACP.
                    //
                    if (EnumSystemLocales)
                    {
                        iRet = GetLocaleInfo( Locale,
                                              LOCALE_IDEFAULTANSICODEPAGE |
                                                LOCALE_NOUSEROVERRIDE |
                                                LOCALE_RETURN_NUMBER,
                                              (PTSTR) &dwLocaleACP,
                                              sizeof(dwLocaleACP) / sizeof(TCHAR) );
                        if (iRet)
                        {
                            iRet = dwLocaleACP;
                        }
                    }

                    if (iRet)
                    {
                        //
                        //  Get the name of the locale.
                        //
                        GetLocaleInfo(Locale, LOCALE_SLANGUAGE, szBuf, SIZE_300);

                        //
                        //  Add the new locale to the list box.
                        //
                        dwIndex = ComboBox_AddString(hLocale, szBuf);
                        ComboBox_SetItemData(hLocale, dwIndex, Locale);
                    }
                }
            }
        }
        pLG = pLG->pNext;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_EnumAlternateSorts
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_EnumAlternateSorts()
{
    LPLANGUAGEGROUP pLG;
    UINT ctr;

    //
    //  Initialize the globals for the alternate sort locales.
    //
    if (!pAltSorts)
    {
        if (!(hAltSorts = GlobalAlloc(GHND, MAX_PATH * sizeof(DWORD))))
        {
            return (FALSE);
        }
        pAltSorts = GlobalLock(hAltSorts);
    }

    //
    //  Go through the language groups to see which ones are installed.
    //  Save the alternate sorts for these language groups.
    //
    pLG = pLanguageGroups;
    while (pLG)
    {
        //
        //  If the language group is originally installed and not marked for
        //  removal OR is marked to be installed, then add the locales for
        //  this language group to the System and User combo boxes.
        //
        for (ctr = 0; ctr < pLG->NumAltSorts; ctr++)
        {
            //
            //  Save the locale id.
            //
            if (g_NumAltSorts >= MAX_PATH)
            {
                return (TRUE);
            }
            pAltSorts[g_NumAltSorts] = (pLG->pAltSortList)[ctr];
            g_NumAltSorts++;
        }
        pLG = pLG->pNext;
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_UpdateSortingCombo
//
////////////////////////////////////////////////////////////////////////////

void Region_UpdateSortingCombo(
    HWND hDlg,
    LCID NewLocale)
{
    HWND hUserLocale = GetDlgItem(hDlg, IDC_USER_LOCALE);
    HWND hSortingText = GetDlgItem(hDlg, IDC_SORTING_TEXT);
    HWND hSorting = GetDlgItem(hDlg, IDC_SORTING);
    DWORD dwIndex;
    TCHAR szBuf[SIZE_128];
    LCID LocaleID;
    LANGID LangID;
    int ctr;

    //
    //  Get the locale id of the currently selected user locale.
    //
    if (NewLocale == 0)
    {
        dwIndex = ComboBox_GetCurSel(hUserLocale);
        if ((dwIndex == CB_ERR) ||
            ((NewLocale = (LCID)ComboBox_GetItemData(hUserLocale, dwIndex)) == CB_ERR))
        {
            return;
        }
    }

    //
    //  Reset the contents of the combo box.
    //
    ComboBox_ResetContent(hSorting);

    //
    //  Get the language id from the locale id.
    //
    LangID = LANGIDFROMLCID(NewLocale);

    //
    //  Special case Spanish (Spain) - list International sort first.
    //
    if (LangID == LANG_SPANISH_TRADITIONAL)
    {
        LangID = LANG_SPANISH_INTL;
        LocaleID = LCID_SPANISH_INTL;
    }
    else
    {
        LocaleID = NewLocale;
    }

    //
    //  Store the sort name for the locale.
    //
    if (GetLocaleInfo((LCID)LangID, LOCALE_SSORTNAME, szBuf, SIZE_128))
    {
        //
        //  Add the new sorting option to the sorting combo box.
        //
        dwIndex = ComboBox_AddString(hSorting, szBuf);
        ComboBox_SetItemData(hSorting, dwIndex, LocaleID);

        //
        //  Set this as the current selection.
        //
        ComboBox_SetCurSel(hSorting, dwIndex);
    }

    //
    //  Special case Spanish (Spain) - list Traditional sort second.
    //
    if (LangID == LANG_SPANISH_INTL)
    {
        LangID = LANG_SPANISH_TRADITIONAL;
        if (GetLocaleInfo((LCID)LangID, LOCALE_SSORTNAME, szBuf, SIZE_128))
        {
            //
            //  Add the new sorting option to the sorting combo box.
            //
            dwIndex = ComboBox_AddString(hSorting, szBuf);
            ComboBox_SetItemData(hSorting, dwIndex, LCID_SPANISH_TRADITIONAL);

            //
            //  Set this as the current selection if it's the current
            //  locale id.
            //
            if ((NewLocale == LCID_SPANISH_TRADITIONAL) ||
                (UserLocaleID == LCID_SPANISH_TRADITIONAL))
            {
                ComboBox_SetCurSel(hSorting, dwIndex);
            }
        }
        LangID = LANGIDFROMLCID(NewLocale);
    }

    //
    //  Fill in the drop down if necessary.
    //
    for (ctr = 0; ctr < g_NumAltSorts; ctr++)
    {
        LocaleID = pAltSorts[ctr];
        if ((LANGIDFROMLCID(LocaleID) == LangID) &&
            (GetLocaleInfo(LocaleID, LOCALE_SSORTNAME, szBuf, SIZE_128)))
        {
            //
            //  Add the new sorting option to the sorting combo box.
            //
            dwIndex = ComboBox_AddString(hSorting, szBuf);
            ComboBox_SetItemData(hSorting, dwIndex, LocaleID);

            //
            //  Set this as the current selection if it's the current
            //  locale id.
            //
            if ((LocaleID == NewLocale) || (LocaleID == UserLocaleID))
            {
                ComboBox_SetCurSel(hSorting, dwIndex);
            }
        }
    }

    //
    //  Enable the combo box if there is more than one entry in the list.
    //  Otherwise, disable it.
    //
    if (ComboBox_GetCount(hSorting) > 1)
    {
        if ((IsWindowEnabled(hSorting) == FALSE) ||
            (IsWindowVisible(hSorting) == FALSE))
        {
            EnableWindow(hSortingText, TRUE);
            EnableWindow(hSorting, TRUE);
            ShowWindow(hSortingText, SW_SHOW);
            ShowWindow(hSorting, SW_SHOW);
        }
    }
    else
    {
        if ((IsWindowEnabled(hSorting) == TRUE) ||
            (IsWindowVisible(hSorting) == TRUE))
        {
            EnableWindow(hSortingText, FALSE);
            EnableWindow(hSorting, FALSE);
            ShowWindow(hSortingText, SW_HIDE);
            ShowWindow(hSorting, SW_HIDE);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_GetUILanguagePolicy
//
//  Checks if a policy is installed for the current user's MUI language.
//  The function assumes this is an MUI system.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_GetUILanguagePolicy()
{
    HKEY hKey;
    BYTE buf[MAX_PATH];
    DWORD dwType, dwResultLen = sizeof(buf);
    BOOL bRet = FALSE;
    DWORD Num;


    //
    //  Try to open the MUI Language policy key.
    //
    if (RegOpenKey(HKEY_CURRENT_USER,
                   c_szMUIPolicyKeyPath,
                   &hKey) == ERROR_SUCCESS)
    {
        if ((RegQueryValueEx(hKey,
                             c_szMUIValue,
                             NULL,
                             &dwType,
                             &buf[0],
                             &dwResultLen) == ERROR_SUCCESS) &&
            (dwType == REG_SZ) &&
            (dwResultLen > 2))
        {
            bRet = TRUE;
        }
        RegCloseKey(hKey);
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_UpdateUILanguageCombo
//
////////////////////////////////////////////////////////////////////////////

void Region_UpdateUILanguageCombo(
    HWND hDlg)
{
    HWND hUILangText = GetDlgItem(hDlg, IDC_UI_LANGUAGE_TEXT);
    HWND hUILang = GetDlgItem(hDlg, IDC_UI_LANGUAGE);
    HKEY hKey;
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    DWORD dwIndex, cchValue, cbData;
    DWORD UILang;
    LANGID DefaultUILang;
    LONG rc;

    //
    //  Reset the contents of the combo box.
    //
    ComboBox_ResetContent(hUILang);

    //
    //  See if this combo box should be enabled by getting the default
    //  UI language and opening the
    //  HKLM\System\CurrentControlSet\Control\Nls\MUILanguages key.
    //
    if (!(DefaultUILang = GetUserDefaultUILanguage()) ||
        (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       c_szMUILanguages,
                       0,
                       KEY_READ,
                       &hKey ) != ERROR_SUCCESS))
    {
        //
        //  No MUILanguages.  Disable and hide the UI language combo box.
        //
        EnableWindow(hUILangText, FALSE);
        EnableWindow(hUILang, FALSE);
        ShowWindow(hUILangText, SW_HIDE);
        ShowWindow(hUILang, SW_HIDE);
        return;
    }

    //
    //  Enumerate the values in the MUILanguages key.
    //
    dwIndex = 0;
    cchValue = sizeof(szValue) / sizeof(TCHAR);
    szValue[0] = TEXT('\0');
    cbData = sizeof(szData);
    szData[0] = TEXT('\0');
    rc = RegEnumValue( hKey,
                       dwIndex,
                       szValue,
                       &cchValue,
                       NULL,
                       NULL,
                       (LPBYTE)szData,
                       &cbData );

    while (rc == ERROR_SUCCESS)
    {
        //
        //  If the UI language contains data, then it is installed.
        //
        if ((szData[0] != 0) &&
            (UILang = TransNum(szValue)) &&
            (GetLocaleInfo(UILang, LOCALE_SNATIVELANGNAME, szData, MAX_PATH)))
        {
            //
            //  Add the new UI Language option to the combo box.
            //
            dwIndex = ComboBox_AddString(hUILang, szData);
            ComboBox_SetItemData(hUILang, dwIndex, UILang);

            //
            //  Set this as the current selection if it's the default.
            //
            if (UILang == (DWORD)DefaultUILang)
            {
                ComboBox_SetCurSel(hUILang, dwIndex);
            }
        }

        //
        //  Get the next enum value.
        //
        dwIndex++;
        cchValue = sizeof(szValue) / sizeof(TCHAR);
        szValue[0] = TEXT('\0');
        cbData = sizeof(szData);
        szData[0] = TEXT('\0');
        rc = RegEnumValue( hKey,
                           dwIndex,
                           szValue,
                           &cchValue,
                           NULL,
                           NULL,
                           (LPBYTE)szData,
                           &cbData );
    }

    //
    //  Close the registry key handle.
    //
    RegCloseKey(hKey);

    //
    //  Make sure there is at least one entry in the list.
    //
    if (ComboBox_GetCount(hUILang) < 1)
    {
        //
        //  No MUILanguages.  Add the default UI language option to the
        //  combo box.
        //
        if ((GetLocaleInfo(DefaultUILang, LOCALE_SNATIVELANGNAME, szData, MAX_PATH)) &&
            (ComboBox_AddString(hUILang, szData) == 0))
        {
            ComboBox_SetItemData(hUILang, 0, (DWORD)DefaultUILang);
            ComboBox_SetCurSel(hUILang, 0);
        }
    }

    //
    //  Make sure something is selected.
    //
    if (ComboBox_GetCurSel(hUILang) == CB_ERR)
    {
        ComboBox_SetCurSel(hUILang, 0);
    }

    //
    //  Enable the combo box if there is more than one entry in the list.
    //  Otherwise, disable it.
    //
    if (ComboBox_GetCount(hUILang) > 1)
    {
        if ((IsWindowEnabled(hUILang) == FALSE) ||
            (IsWindowVisible(hUILang) == FALSE))
        {
            ShowWindow(hUILangText, SW_SHOW);
            ShowWindow(hUILang, SW_SHOW);
        }

        //
        // Check if there is a policy enforced on the user, and if
        // so disable the MUI controls.
        //
        if (Region_GetUILanguagePolicy())
        {
            EnableWindow(hUILangText, FALSE);
            EnableWindow(hUILang, FALSE);
        }
        else
        {
            EnableWindow(hUILangText, TRUE);
            EnableWindow(hUILang, TRUE);
        }
    }
    else
    {
        if ((IsWindowEnabled(hUILang) == TRUE) ||
            (IsWindowVisible(hUILang) == TRUE))
        {
            EnableWindow(hUILangText, FALSE);
            EnableWindow(hUILang, FALSE);
            ShowWindow(hUILangText, SW_HIDE);
            ShowWindow(hUILang, SW_HIDE);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ClearValues
//
//  Reset each of the list boxes in the region property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Region_ClearValues(
    HWND hDlg)
{
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_USER_LOCALE));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_SORTING));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_UI_LANGUAGE));
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_SetValues
//
//  Initialize all of the controls in the region property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Region_SetValues(
    HWND hDlg,
    LPREGDLGDATA pDlgData,
    BOOL fInit)
{
    TCHAR szUserBuf[SIZE_128];
    TCHAR szSorting[SIZE_128];
    TCHAR szUILang[SIZE_128];
    TCHAR szDefaultUserBuf[SIZE_128];
    TCHAR szLastUserBuf[SIZE_128];
    TCHAR szBuf[SIZE_128];
    DWORD dwIndex;
    HWND hUserLocale = GetDlgItem(hDlg, IDC_USER_LOCALE);
    HWND hSorting = GetDlgItem(hDlg, IDC_SORTING);
    HWND hUILang = GetDlgItem(hDlg, IDC_UI_LANGUAGE);

    //
    //  Get the strings to search for in the combo boxes in order to set
    //  the current selections.
    //
    if (fInit)
    {
        //
        //  It's init time, so get the user default settings.
        //
        if ((UserLocaleID == LCID_SPANISH_TRADITIONAL) ||
            (UserLocaleID == LCID_SPANISH_INTL))
        {
            LoadString(hInstance, IDS_SPANISH_NAME, szUserBuf, SIZE_128);
        }
        else
        {
            GetLocaleInfo(UserLocaleID, LOCALE_SLANGUAGE, szUserBuf, SIZE_128);
        }
    }
    else
    {
        //
        //  It's not init time, so get the settings from the combo boxes.
        //
        ComboBox_GetLBText( hUserLocale,
                            ComboBox_GetCurSel(hUserLocale),
                            szUserBuf );
        ComboBox_GetLBText( hSorting,
                            ComboBox_GetCurSel(hSorting),
                            szSorting );
        ComboBox_GetLBText( hUILang,
                            ComboBox_GetCurSel(hUILang),
                            szUILang );

        if (pDlgData)
        {
            ComboBox_GetLBText( hUserLocale,
                                pDlgData->dwCurUserLocale,
                                szDefaultUserBuf );
            ComboBox_GetLBText( hUserLocale,
                                pDlgData->dwLastUserLocale,
                                szLastUserBuf );
        }
    }

    //
    //  Reset the combo boxes.
    //
    Region_ClearValues(hDlg);

    //
    //  Get the list of locales and fill in the user locale combo box.
    //
    Region_EnumLocales(hDlg, hUserLocale, FALSE);

    //
    //  Select the current user locale id in the list.
    //  Special case Spanish.
    //
    dwIndex = ComboBox_FindStringExact(hUserLocale, -1, szUserBuf);
    if (dwIndex == CB_ERR)
    {
        GetLocaleInfo(SysLocaleID, LOCALE_SLANGUAGE, szBuf, SIZE_128);
        dwIndex = ComboBox_FindStringExact(hUserLocale, -1, szBuf);
        if (dwIndex == CB_ERR)
        {
            GetLocaleInfo(US_LOCALE, LOCALE_SLANGUAGE, szBuf, SIZE_128);
            dwIndex = ComboBox_FindStringExact(hUserLocale, -1, szBuf);
            if (dwIndex == CB_ERR)
            {
                dwIndex = 0;
            }
        }
        if (!fInit && pDlgData)
        {
            pDlgData->Changes |= RC_UserLocale;
        }
    }
    ComboBox_SetCurSel(hUserLocale, dwIndex);

    //
    //  Fill in the appropriate Sorting name for the selected locale.
    //
    Region_UpdateSortingCombo(hDlg, 0);
    dwIndex = ComboBox_GetCurSel(hSorting);
    if (ComboBox_SetCurSel( hSorting,
                            ComboBox_FindStringExact( hSorting,
                                                      -1,
                                                      szSorting ) ) == CB_ERR)
    {
        ComboBox_SetCurSel(hSorting, dwIndex);
    }

    //
    //  Fill in the current UI Lanugage settings in the list.
    //
    Region_UpdateUILanguageCombo(hDlg);
    dwIndex = ComboBox_GetCurSel(hUILang);
    if (ComboBox_SetCurSel( hUILang,
                            ComboBox_FindStringExact( hUILang,
                                                      -1,
                                                      szUILang ) ) == CB_ERR)
    {
        ComboBox_SetCurSel(hUILang, dwIndex);
    }

    //
    //  Store the initial locale state in the pDlgData structure.
    //
    if (pDlgData)
    {
        if (fInit)
        {
            pDlgData->dwCurUserLocale = ComboBox_GetCurSel(hUserLocale);
            pDlgData->dwCurSorting = ComboBox_GetCurSel(hSorting);
            pDlgData->dwCurUILang = ComboBox_GetCurSel(hUILang);
            pDlgData->dwLastUserLocale = pDlgData->dwCurUserLocale;
            pDlgData->dwLastSorting = pDlgData->dwCurSorting;
        }
        else
        {
            pDlgData->dwCurUserLocale =
                ComboBox_FindStringExact(hUserLocale, -1, szDefaultUserBuf);
            pDlgData->dwCurSorting = ComboBox_GetCurSel(hSorting);
            pDlgData->dwCurUILang = ComboBox_GetCurSel(hUILang);
            pDlgData->dwLastUserLocale =
                ComboBox_FindStringExact(hUserLocale, -1, szLastUserBuf);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ChangeUILangForAllUsers
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_ChangeUILangForAllUsers(
    LANGID UILanguageId)
{
    HKEY hKey;
    TCHAR szData[MAX_PATH];
    TCHAR szProfile[MAX_PATH + 20];
    DWORD cchSize;
    LONG rc = 0L;
    BOOLEAN WasEnabled;
    BOOL bRet = TRUE;

    //
    //  Save the UILanguageId as a string.
    //
    wsprintf(szData, TEXT("%08x"), UILanguageId);

    //
    //  Set the value in .DEFAULT.
    //
    if ((rc = RegOpenKeyEx( HKEY_USERS,
                            TEXT(".DEFAULT\\Control Panel\\Desktop"),
                            0L,
                            KEY_READ | KEY_WRITE,
                            &hKey )) == ERROR_SUCCESS)
    {
        rc = RegSetValueEx( hKey,
                            c_szMUIValue,
                            0L,
                            REG_SZ,
                            (LPBYTE)szData,
                            (lstrlen(szData) + 1) * sizeof(TCHAR) );
        RegCloseKey(hKey);
    }

    if (rc != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    //
    //  Get the file name for the Default User profile.
    //
    cchSize = MAX_PATH;
    GetDefaultUserProfileDirectory(szProfile, &cchSize);
    lstrcat(szProfile, TEXT("\\NTUSER.DAT"));

    //
    //  Set the value in the Default User hive.
    //
    rc = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (NT_SUCCESS(rc))
    {
        //
        //  Load the hive and restore the privilege to its previous state.
        //
        rc = RegLoadKey(HKEY_USERS, TEXT("RegionalSettingsTempKey"), szProfile);
        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);

        //
        //  If the hive loaded properly, set the value.
        //
        if (rc == ERROR_SUCCESS)
        {
            if ((rc = RegOpenKeyEx( HKEY_USERS,
                                    TEXT("RegionalSettingsTempKey\\Control Panel\\Desktop"),
                                    0L,
                                    KEY_READ | KEY_WRITE,
                                    &hKey )) == ERROR_SUCCESS)
            {
                rc = RegSetValueEx( hKey,
                                    c_szMUIValue,
                                    0L,
                                    REG_SZ,
                                    (LPBYTE)szData,
                                    (lstrlen(szData) + 1) * sizeof(TCHAR) );
                RegCloseKey(hKey);
                RegFlushKey(HKEY_USERS);
            }

            //
            //  Unload the hive.
            //
            rc = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);
            if (!NT_SUCCESS(rc))
            {
                bRet = FALSE;
            }
            rc = RegUnLoadKey(HKEY_USERS, TEXT("RegionalSettingsTempKey"));
            RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
        }
    }

    if (rc != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    //
    //  Return the result.
    //
    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ApplySettings
//
//  If the Locale has changed, call Set_Locale_Values to update the
//  user locale information.   Notify the parent of changes and reset the
//  change flag stored in the property sheet page structure appropriately.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_ApplySettings(
    HWND hDlg,
    LPREGDLGDATA pDlgData)
{
    TCHAR szLCID[25];
    DWORD dwLocale, dwSorting, dwUILang;
    LCID NewLocale, SortLocale;
    LANGID UILang;
    HCURSOR hcurSave;
    HWND hUserLocale = GetDlgItem(hDlg, IDC_USER_LOCALE);
    HWND hSorting = GetDlgItem(hDlg, IDC_SORTING);
    HWND hUILang = GetDlgItem(hDlg, IDC_UI_LANGUAGE);
    BOOL bReboot = FALSE;
    DWORD dwRecipients;
    LPLANGUAGEGROUP pLG;
    BOOL bState, fUserCancel = FALSE;
    HWND hwndLV = GetDlgItem(hDlg, IDC_LANGUAGE_GROUPS);
    LVITEM lvItem;
    int iIndex=0, cCount=0;
    BOOL DeskCPLChanges = FALSE;
    BOOL InvokeSysocmgr = FALSE;

    //
    //  See if there are any changes.
    //
    if (pDlgData->Changes <= RC_EverChg)
    {
        return (TRUE);
    }

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  See if there are any changes to the language groups.
    //
    if (pDlgData->Changes & RC_LangGroups)
    {
        if (Region_SetupLanguageGroups(hDlg, &bReboot, &fUserCancel) == FALSE)
        {
            //
            //  If the user cancelled the installation, let's undo what
            //  the user did.
            //
            if (fUserCancel)
            {
                cCount = ListView_GetItemCount(hwndLV);
                while( iIndex < cCount )
                {
                    lvItem.iItem = iIndex;
                    lvItem.mask = LVIF_PARAM;
                    lvItem.iSubItem = 0;
                    ListView_GetItem(hwndLV, &lvItem);
                    pLG = (LPLANGUAGEGROUP)lvItem.lParam;
                    if (pLG)
                    {
                        if ((pLG->wStatus == ML_INSTALL) ||
                            (pLG->wStatus == (ML_REMOVE | ML_ORIG_INSTALLED)))
                        {
                            pLG->wStatus ^= (ML_INSTALL | ML_REMOVE | ML_ORIG_INSTALLED);
                            bState = ListView_GetCheckState(hwndLV, iIndex);
                            ListView_SetCheckState(hwndLV, iIndex, !bState);
                        }
                    }
                    iIndex++;
                }
                //
                //  Remove what's installled.
                //
                Region_SetupLanguageGroups(hDlg, &bReboot, &fUserCancel);

                //
                //  Update the UI.
                //
                Region_SetValues(hDlg, pDlgData, FALSE);
            }

            SetCursor(hcurSave);
            return (FALSE);
        }
        if (bReboot)
        {
            //
            //  Validate the user's Keyboard Layout Preload section.
            //
            Region_ValidateRegistryPreload();

            //
            //  Update the active locales for the Input Locales property page
            //  if the page has been initialized.
            //
            if (g_hEvent)
            {
                ResetEvent(g_hEvent);
            }
            PropSheet_QuerySiblings(GetParent(hDlg), 0, 0);
        }
    }

    //
    //  See if there are any changes to the system locale if we have
    //  admin privileges.
    //
    if ((pDlgData->Changes & RC_SystemLocale) &&
        (RegSysLocaleID != SysLocaleID) &&
        (pDlgData->Admin_Privileges))
    {
        //
        //  Get the locale id for the current selection.
        //
        NewLocale = SysLocaleID;
        wsprintf(szLCID, TEXT("%08x"), NewLocale);

        //
        //  Call setup to install the option.
        //
        if (SetupChangeLocaleEx( hDlg,
                                 NewLocale,
                                 pSetupSourcePath,
                                 (g_bSetupCase)
                                   ? SP_INSTALL_FILES_QUIETLY
                                   : 0,
                                 NULL,
                                 0 ))
        {
            //
            //  If Setup fails, put up a message.
            //
            SetCursor(hcurSave);
            Region_ShowMsg( NULL,
                            IDS_SETUP_STRING,
                            IDS_TITLE_STRING,
                            MB_OK_OOPS,
                            NULL );
            SysLocaleID = GetSystemDefaultLCID();
            return (FALSE);
        }

        //
        //  Reset the registry system locale value.
        //
        RegSysLocaleID = SysLocaleID;
        DeskCPLChanges = TRUE;

        //
        //  Need to make sure the proper keyboard layout is installed.
        //
        Region_InstallKeyboardLayout(hDlg, NewLocale, szLCID);

        //
        //  See if we need to reboot.
        //
        if (SysLocaleID != GetSystemDefaultLCID())
        {
            bReboot = TRUE;
        }

        InvokeSysocmgr = TRUE;
    }

    //
    //  See if there are any changes to the user locale.
    //
    if (pDlgData->Changes & RC_UserLocale)
    {
        //
        //  Get the current selections.
        //
        dwLocale = ComboBox_GetCurSel(hUserLocale);
        dwSorting = ComboBox_GetCurSel(hSorting);

        //
        //  See if the current selections are different from the original
        //  selections.
        //
        if ((dwLocale != CB_ERR) &&
            ((dwLocale != pDlgData->dwCurUserLocale) ||
             (dwSorting != pDlgData->dwCurSorting)))
        {
            //
            //  Get the locale id for the current selection.
            //
            NewLocale = (LCID)ComboBox_GetItemData(hUserLocale, dwLocale);

            //
            //  Get the locale id with the sort id.
            //
            SortLocale = (LCID)ComboBox_GetItemData(hSorting, dwSorting);

            //
            //  See if we've got Spanish.
            //
            if (SortLocale == LCID_SPANISH_TRADITIONAL)
            {
                NewLocale = LCID_SPANISH_TRADITIONAL;
            }

            //
            //  Make sure the sort locale is okay.
            //
            if (LANGIDFROMLCID(SortLocale) != LANGIDFROMLCID(NewLocale))
            {
                SortLocale = NewLocale;
            }

            //
            //  Set the current locale values in the pDlgData structure.
            //
            pDlgData->dwCurUserLocale = dwLocale;
            pDlgData->dwCurSorting = dwSorting;

            //
            //  Save the new locale information.
            //
            UserLocaleID = SortLocale;
            bShowRtL    = IsRtLLocale(UserLocaleID);
            bShowArabic = (bShowRtL && (PRIMARYLANGID(LANGIDFROMLCID(UserLocaleID)) != LANG_HEBREW));

            //
            //  Install the new locale by adding the appropriate information
            //  to the registry.
            //
            Region_InstallUserLocale(SortLocale);
            
            //
            // Update the NLS process cache
            //
            NlsResetProcessLocale();

            //
            //  Reset the registry user locale value.
            //
            RegUserLocaleID = UserLocaleID;
            DeskCPLChanges = TRUE;

            //
            //  Need to make sure the proper keyboard layout is installed.
            //
            wsprintf(szLCID, TEXT("%08x"), SortLocale);

            Region_InstallKeyboardLayout(hDlg, SortLocale, szLCID);

            //
            //  Broadcast the message that the international settings in the
            //  registry have changed.
            //
            dwRecipients = BSM_APPLICATIONS | BSM_ALLDESKTOPS;
            BroadcastSystemMessage( BSF_FORCEIFHUNG | BSF_IGNORECURRENTTASK |
                                      BSF_NOHANG | BSF_NOTIMEOUTIFNOTHUNG,
                                    &dwRecipients,
                                    WM_WININICHANGE,
                                    0,
                                    (LPARAM)szIntl );
        }
    }

    //
    //  See if there are any changes to the UI Language.
    //
    if (pDlgData->Changes & RC_UILanguage)
    {
        //
        //  Get the current selection.
        //
        dwUILang = ComboBox_GetCurSel(hUILang);

        //
        //  See if the current selection is different from the original
        //  selection.
        //
        if ((dwUILang != CB_ERR) &&
            ((dwUILang != pDlgData->dwCurUILang)))
        {
            //
            //  Get the UI Language id for the current selection.
            //
            UILang = (LANGID)ComboBox_GetItemData(hUILang, dwUILang);

            //
            //  Set the current UI language value in the pDlgData structure.
            //
            pDlgData->dwCurUILang = dwUILang;

            //
            //  Set the UI Language value in the user's registry.
            //
            NtSetDefaultUILanguage(UILang);
            DeskCPLChanges = TRUE;

            //
            //  If the user has admin privileges, see if they want to
            //  have this set for ALL new users of this machine.
            //
            if (pDlgData->Admin_Privileges)
            {
                if (Region_ShowMsg( hDlg,
                                    IDS_CHANGE_UI_LANG,
                                    IDS_TITLE_STRING,
                                    MB_YESNO | MB_ICONQUESTION,
                                    NULL ) == IDYES)
                {
                    if (!Region_ChangeUILangForAllUsers(UILang))
                    {
                        Region_ShowMsg( hDlg,
                                        IDS_DEFAULT_USER_ERROR,
                                        IDS_TITLE_STRING,
                                        MB_OK_OOPS,
                                        NULL );
                    }
                }
            }
            else
            {
                //
                //  If the user does not have admin privileges, then just
                //  Alert them that the UI change will take effect next time
                //  they log on.
                //
                Region_ShowMsg( hDlg,
                                IDS_CHANGE_UI_LANG_NOT_ADMIN,
                                IDS_TITLE_STRING,
                                MB_OK | MB_ICONINFORMATION,
                                NULL);

            }
        }
    }

    //
    //  If the user locale, system locale, or ui language changed,
    //  and we're not running in gui setup, then let's invoke desk.cpl.
    //
    if (!g_bSetupCase && DeskCPLChanges)
    {
        Region_InvokeDeskCPL();
    }

    //
    //  If the system locale changed and we're not running in gui setup,
    //  then let's invoke sysocmgr.exe.
    //
    if (!g_bSetupCase && InvokeSysocmgr)
    {
        //
        //  Run any necessary apps (for FSVGA/FSNEC installation).
        //
        Region_RunRegApps(c_szSysocmgr);
    }

    //
    //  Register the regional change every time so that all other property
    //  pages will be updated due to the locale settings change.
    //
    Verified_Regional_Chg = INTL_CHG;

    //
    //  Reset the property page settings.
    //
    PropSheet_UnChanged(GetParent(hDlg), hDlg);
    pDlgData->Changes = RC_EverChg;

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  See if we need to display the reboot message.
    //
    if ((!g_bSetupCase) && (bReboot))
    {
        if (Region_ShowMsg( hDlg,
                            IDS_REBOOT_STRING,
                            IDS_TITLE_STRING,
                            MB_YESNO | MB_ICONQUESTION,
                            NULL ) == IDYES)
        {
            Region_RebootTheSystem();
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ValidatePPS
//
//  Validate each of the combo boxes whose values are constrained.
//  If any of the input fails, notify the user and then return FALSE
//  to indicate validation failure.
//
//  Also, if the user locale has changed, then register the change so
//  that all other property pages will be updated with the new locale
//  settings.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_ValidatePPS(
    HWND hDlg,
    LPREGDLGDATA pDlgData)
{
    LPARAM Changes = pDlgData->Changes;

    //
    //  If nothing has changed, return TRUE immediately.
    //
    if (Changes <= RC_EverChg)
    {
        return (TRUE);
    }

    //
    //  See if the user locale has changed.
    //
    if (Changes & RC_UserLocale)
    {
        HWND hUserLocale = GetDlgItem(hDlg, IDC_USER_LOCALE);
        HWND hSorting = GetDlgItem(hDlg, IDC_SORTING);
        DWORD dwLocale = ComboBox_GetCurSel(hUserLocale);
        DWORD dwSorting = ComboBox_GetCurSel(hSorting);
        LCID NewLocale, SortLocale;

        //
        //  See if the current selections are different from the original
        //  selections.
        //
        if ((dwLocale != CB_ERR) &&
            ((dwLocale != pDlgData->dwLastUserLocale) ||
             (dwSorting != pDlgData->dwLastSorting)))
        {
            //
            //  Get the locale id for the current selection.
            //
            NewLocale = (LCID)ComboBox_GetItemData(hUserLocale, dwLocale);

            //
            //  Get the locale id with the sort id.
            //
            SortLocale = (LCID)ComboBox_GetItemData(hSorting, dwSorting);

            //
            //  See if we've got Spanish.
            //
            if (SortLocale == LCID_SPANISH_TRADITIONAL)
            {
                NewLocale = LCID_SPANISH_TRADITIONAL;
            }

            //
            //  Make sure the sort locale is okay.
            //
            if (LANGIDFROMLCID(SortLocale) != LANGIDFROMLCID(NewLocale))
            {
                SortLocale = NewLocale;
            }

            //
            //  Set the current locale values in the pDlgData structure.
            //
            pDlgData->dwLastUserLocale = dwLocale;
            pDlgData->dwLastSorting = dwSorting;

            //
            //  Set the UserLocaleID value.
            //
            UserLocaleID = SortLocale;
            bShowRtL    = IsRtLLocale(UserLocaleID);
            bShowArabic = (bShowRtL && (PRIMARYLANGID(LANGIDFROMLCID(UserLocaleID)) != LANG_HEBREW));

            //
            //  Register the regional change so that all other property
            //  pages will be updated with the new locale settings.
            //
            Verified_Regional_Chg = INTL_CHG;
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}



////////////////////////////////////////////////////////////////////////////
//
//  Region_InitPropSheet
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_InitPropSheet(
    HWND hDlg,
    LPPROPSHEETPAGE psp)
{
    HKEY hKey;
    TCHAR szData[MAX_PATH];
    DWORD cbData;
    LPREGDLGDATA pDlgData = (LPREGDLGDATA)LocalAlloc(LPTR, sizeof(REGDLGDATA));

    //
    //  Make sure we have a REGDLGDATA buffer.
    //
    if (pDlgData == NULL)
    {
        return (FALSE);
    }

    //
    //  See if we're in setup mode.
    //
    if (psp && psp->lParam)
    {
        //
        //  Set the setup special case flag.
        //
        g_bSetupCase = TRUE;

        //
        //  Use the registry system locale value for the setup case.
        //
        SysLocaleID = RegSysLocaleID;

        //
        //  Use the registry user locale value for the setup case.
        //
        UserLocaleID = RegUserLocaleID;
        bShowRtL = IsRtLLocale(UserLocaleID);
        bShowArabic = (bShowRtL && (PRIMARYLANGID(LANGIDFROMLCID(UserLocaleID)) != LANG_HEBREW));
    }
    else
    {
        //
        //  Initialize desk.cpl for the 1st time. This is done when intl.cpl
        //  is invoked interactively by the user.
        //
        Region_LoadDeskCPL();
        Region_InvokeDeskCPL();
    }

    //
    //  Get the system metrics.
    //
    g_cxVScroll = GetSystemMetrics(SM_CXVSCROLL);

    //
    //  Save the data.
    //
    psp->lParam = (LPARAM)pDlgData;
    SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)psp);

    //
    //  See if the user has Administrative privileges by checking for
    //  write permission to the registry key.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szInstalledLocales,
                      0L,
                      KEY_WRITE,
                      &hKey ) == ERROR_SUCCESS)
    {
        //
        //  We can write to the HKEY_LOCAL_MACHINE key, so the user
        //  has Admin privileges.
        //
        pDlgData->Admin_Privileges = TRUE;
        RegCloseKey(hKey);
    }
    else
    {
        //
        //  The user does not have admin privileges, so disable the
        //  appropriate items in the "Language settings for the system"
        //  group.
        //
        pDlgData->Admin_Privileges = FALSE;
        EnableWindow(GetDlgItem(hDlg, IDC_SET_DEFAULT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED), FALSE);
    }

    //
    //  Load the information into the dialog.
    //
    Region_LoadLanguageGroups(hDlg, pDlgData);
    Region_EnumAlternateSorts();
    Region_SetValues(hDlg, pDlgData, TRUE);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_EnumInstalledCPProc
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK Region_EnumInstalledCPProc(
    LPTSTR pString)
{
    UINT CodePage;
    LPCODEPAGE pCP;

    //
    //  Convert the code page string to an integer.
    //
    CodePage = StrToLong(pString);

    //
    //  Find the code page in the linked list and mark it as
    //  originally installed.
    //
    pCP = pCodePages;
    while (pCP)
    {
        if (pCP->CodePage == CodePage)
        {
            pCP->wStatus |= ML_ORIG_INSTALLED;
            break;
        }

        pCP = pCP->pNext;
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_GetLocaleLanguageGroup
//
//  Reads the Language Group Id of the given language.
//
////////////////////////////////////////////////////////////////////////////

DWORD Region_GetLanguageGroup(
    LCID lcid)
{
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    HKEY hKey;
    DWORD cbData;


    wsprintf(szValue, TEXT("%8.8x"), lcid);
    szData[0] = 0;
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szInstalledLocales,
                      0,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        cbData = sizeof(szData);
        RegQueryValueEx(hKey, szValue, NULL, NULL, (LPBYTE)szData, &cbData);
        RegCloseKey(hKey);
    }

    return (TransNum(szData));
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_GetUILanguageGroups
//
//  Reads the language groups of all the UI languages installed on this
//  machine.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_GetUILanguageGroups(
    PUILANGUAGEGROUP pUILanguageGroup)
{
    //
    //  Enumerate the installed UI languages.
    //
    pUILanguageGroup->iCount = 0L;

    EnumUILanguages(Region_EnumUILanguagesProc, 0, (LONG_PTR)pUILanguageGroup);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_EnumUILanguagesProc
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK Region_EnumUILanguagesProc(
    LPWSTR pwszUILanguage,
    LONG_PTR lParam)
{
    int Ctr = 0;
    LGRPID lgrp;
    PUILANGUAGEGROUP pUILangGroup = (PUILANGUAGEGROUP)lParam;
    LCID UILanguage = TransNum( pwszUILanguage );

    if (UILanguage)
    {
        if ((lgrp = Region_GetLanguageGroup(UILanguage)) == 0)
        {
            lgrp = 1;   // default;
        }

        while (Ctr < pUILangGroup->iCount)
        {
            if (pUILangGroup->lgrp[Ctr] == lgrp)
            {
                break;
            }
            Ctr++;
        }

        //
        //  Theoritically, we won't go over 64 language groups!
        //
        if ((Ctr == pUILangGroup->iCount) && (Ctr < MAX_UI_LANG_GROUPS))
        {
            pUILangGroup->lgrp[Ctr] = lgrp;
            pUILangGroup->iCount++;
        }
    }

    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  Region_EnumInstalledLanguageGroups
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_EnumInstalledLanguageGroups()
{
    HKEY hKey;
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    TCHAR szDefault[SIZE_64];
    DWORD dwIndex, cchValue, cbData;
    LONG rc;
    UINT LanguageGroup, OriginalGroup, DefaultGroup, UILanguageGroup;
    LPLANGUAGEGROUP pLG;
    LCID Locale;
    LANGID Language, UILanguage;
    int Ctr;
    UILANGUAGEGROUP UILangGroup;

    //
    //  Get the original install language so that we can mark that
    //  language group as permanent.
    //
    Language = GetSystemDefaultUILanguage();
    if (SUBLANGID(Language) == SUBLANG_NEUTRAL)
    {
        Language = MAKELANGID(PRIMARYLANGID(Language), SUBLANG_DEFAULT);
    }

    if ((OriginalGroup = Region_GetLanguageGroup(Language)) == 0)
    {
        OriginalGroup = 1;
    }

    //
    //  Get the default system locale so that we can mark that language
    //  group as permanent. During gui mode setup, read the system locale from
    //  the registry to make the info on the setup page consistent with intl.cpl.
    //  SysLocaleID will be the registry value in case of setup.
    //
    Locale = SysLocaleID;
    if (Locale == (LCID)Language)
    {
        DefaultGroup = OriginalGroup;
    }
    else
    {
        if ((DefaultGroup = Region_GetLanguageGroup(Locale)) == 0)
        {
            DefaultGroup = 1;
        }
    }

    //
    //  Get the UI language's language groups to disable the user from
    //  un-installing them.  MUISETUP makes sure that each installed UI
    //  language has its language group installed.
    //
    Region_GetUILanguageGroups(&UILangGroup);

    //
    //  Open the HKLM\SYSTEM\CurrentControlSet\Control\Nls\Language Groups
    //  key.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szLanguageGroups,
                      0,
                      KEY_READ,
                      &hKey ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Enumerate the values in the Language Groups key.
    //
    dwIndex = 0;
    cchValue = sizeof(szValue) / sizeof(TCHAR);
    szValue[0] = TEXT('\0');
    cbData = sizeof(szData);
    szData[0] = TEXT('\0');
    rc = RegEnumValue( hKey,
                       dwIndex,
                       szValue,
                       &cchValue,
                       NULL,
                       NULL,
                       (LPBYTE)szData,
                       &cbData );

    while (rc == ERROR_SUCCESS)
    {
        //
        //  If the language group contains data, then it is installed.
        //
        if ((szData[0] != 0) &&
            (LanguageGroup = TransNum(szValue)))
        {
            //
            //  Find the language group in the linked list and mark it as
            //  originally installed.
            //
            pLG = pLanguageGroups;
            while (pLG)
            {
                if (pLG->LanguageGroup == LanguageGroup)
                {
                    pLG->wStatus |= ML_ORIG_INSTALLED;

                    //
                    //  If this is a language group for a UI language that's
                    //  installed, then disable the un-installation of this
                    //  language group.
                    //
                    Ctr = 0;
                    while (Ctr < UILangGroup.iCount)
                    {
                        if (UILangGroup.lgrp[Ctr] == LanguageGroup)
                        {
                            pLG->wStatus |= ML_PERMANENT;
                            break;
                        }
                        Ctr++;
                    }

                    if (pLG->LanguageGroup == OriginalGroup)
                    {
                        pLG->wStatus |= ML_PERMANENT;
                    }
                    if (pLG->LanguageGroup == DefaultGroup)
                    {
                        pLG->wStatus |= (ML_PERMANENT | ML_DEFAULT);

                        if (LoadString(hInstance, IDS_DEFAULT, szDefault, SIZE_64))
                        {
                            lstrcat(pLG->pszName, szDefault);
                        }
                    }
                    break;
                }

                pLG = pLG->pNext;
            }
        }

        //
        //  Get the next enum value.
        //
        dwIndex++;
        cchValue = sizeof(szValue) / sizeof(TCHAR);
        szValue[0] = TEXT('\0');
        cbData = sizeof(szData);
        szData[0] = TEXT('\0');
        rc = RegEnumValue( hKey,
                           dwIndex,
                           szValue,
                           &cchValue,
                           NULL,
                           NULL,
                           (LPBYTE)szData,
                           &cbData );
    }

    //
    //  Close the registry key handle.
    //
    RegCloseKey(hKey);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_GetLocaleList
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_GetLocaleList(
    LPLANGUAGEGROUP pLG)
{
    TCHAR szSection[MAX_PATH];
    INFCONTEXT Context;
    int LineCount, LineNum;
    LCID Locale;

    //
    //  Get the inf section name.
    //
    wsprintf(szSection, TEXT("%ws%d"), szLocaleListPrefix, pLG->LanguageGroup);

    //
    //  Get the number of locales for the language group.
    //
    LineCount = (UINT)SetupGetLineCount(g_hIntlInf, szSection);
    if (LineCount <= 0)
    {
        return (FALSE);
    }

    //
    //  Add each locale in the list to the language group node.
    //
    for (LineNum = 0; LineNum < LineCount; LineNum++)
    {
        if (SetupGetLineByIndex(g_hIntlInf, szSection, LineNum, &Context) &&
            SetupGetIntField(&Context, 0, &Locale))
        {
            if (SORTIDFROMLCID(Locale))
            {
                //
                //  Add the locale to the alternate sort list for this
                //  language group.
                //
                if (pLG->NumAltSorts >= MAX_PATH)
                {
                    return (FALSE);
                }
                pLG->pAltSortList[pLG->NumAltSorts] = Locale;
                (pLG->NumAltSorts)++;
            }
            else
            {
                //
                //  Add the locale to the locale list for this
                //  language group.
                //
                if (pLG->NumLocales >= MAX_PATH)
                {
                    return (FALSE);
                }
                pLG->pLocaleList[pLG->NumLocales] = Locale;
                (pLG->NumLocales)++;
            }
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_LanguageGroupDirExist
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_LanguageGroupDirExist(
    PTSTR pszLangDir)
{
    TCHAR szLanguageGroupDir[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    TCHAR SavedChar;

    //
    //  If it doesn't start with lang, then this is a core language.
    //
    SavedChar = pszLangDir[4];
    pszLangDir[4] = TEXT('\0');
    if (lstrcmp(pszLangDir, TEXT("lang")))
    {
        return (TRUE);
    }
    pszLangDir[4] = SavedChar;

    //
    //  Format the path to the language group directory.
    //
    lstrcpy(szLanguageGroupDir, pSetupSourcePathWithArchitecture);
    lstrcat(szLanguageGroupDir, TEXT("\\"));
    lstrcat(szLanguageGroupDir, pszLangDir);

    //
    //  See if the language group directory exists.
    //
    FindHandle = FindFirstFile(szLanguageGroupDir, &FindData);
    if (FindHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(FindHandle);
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            return (TRUE);
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_GetSupportedLanguageGroups
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_GetSupportedLanguageGroups(
    LPREGDLGDATA pDlgData)
{
    UINT LanguageGroup;
    HANDLE hLanguageGroup;
    LPLANGUAGEGROUP pLG;
    INFCONTEXT Context;
    TCHAR szSection[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    int LineCount, LineNum;
    DWORD ItemCount;
    WORD wItemStatus;

    //
    //  Get the number of supported language groups from the inf file.
    //
    LineCount = (UINT)SetupGetLineCount(g_hIntlInf, TEXT("LanguageGroups"));
    if (LineCount <= 0)
    {
        return (FALSE);
    }

    //
    //  Go through all supported language groups in the inf file.
    //
    for (LineNum = 0; LineNum < LineCount; LineNum++)
    {
        if (SetupGetLineByIndex(g_hIntlInf, TEXT("LanguageGroups"), LineNum, &Context) &&
            SetupGetIntField(&Context, 0, &LanguageGroup))
        {
            //
            //  Create the new node.
            //
            if (!(hLanguageGroup = GlobalAlloc(GHND, sizeof(LANGUAGEGROUP))))
            {
                return (FALSE);
            }
            pLG = GlobalLock(hLanguageGroup);

            //
            //  Set the status of the item.  If we're in setup mode and the
            //  appropriate lang\xxx directory does not exist, then disable
            //  the language group.
            //
            wItemStatus = 0;
            if (pDlgData->Admin_Privileges == FALSE)
            {
                wItemStatus = ML_DISABLE;
            }
            else if (g_bSetupCase && pSetupSourcePathWithArchitecture)
            {
                if ((ItemCount = SetupGetFieldCount(&Context)) &&
                    (ItemCount >= 2))
                {
                    if (SetupGetStringField(&Context, 2, szTemp, MAX_PATH, NULL))
                    {
                        if (!Region_LanguageGroupDirExist(szTemp))
                        {
                            wItemStatus = ML_DISABLE;
                        }
                    }
                }
            }

            //
            //  Fill in the new node with the appropriate info.
            //
            pLG->wStatus = wItemStatus;
            pLG->LanguageGroup = LanguageGroup;
            pLG->hLanguageGroup = hLanguageGroup;
            (pLG->pszName)[0] = 0;
            pLG->NumLocales = 0;
            pLG->NumAltSorts = 0;

            //
            //  Get the appropriate display string.
            //
            if (!SetupGetStringField(&Context, 1, pLG->pszName, MAX_PATH, NULL))
            {
                GlobalUnlock(hLanguageGroup);
                GlobalFree(hLanguageGroup);
                continue;
            }

            //
            //  See if this language group can be removed.
            //
            wsprintf(szSection, TEXT("%ws%d"), szLGRemovePrefix, LanguageGroup);
            if ((!SetupFindFirstLine( g_hIntlInf,
                                      szSection,
                                      TEXT("AddReg"),
                                      &Context )))
            {
                //
                //  Mark it as permanent.
                //  Also mark it as originally installed to avoid problems.
                //
                pLG->wStatus |= (ML_ORIG_INSTALLED | ML_PERMANENT);
            }

            //
            //  Get the list of locales for this language group.
            //
            if (Region_GetLocaleList(pLG) == FALSE)
            {
                return (FALSE);
            }

            //
            //  Add the language group to the front of the linked list.
            //
            pLG->pNext = pLanguageGroups;
            pLanguageGroups = pLG;
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_LoadLanguageGroups
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_LoadLanguageGroups(
    HWND hDlg,
    LPREGDLGDATA pDlgData)
{
    HWND hwndLG = GetDlgItem(hDlg, IDC_LANGUAGE_GROUPS);
    LPLANGUAGEGROUP pLG;
    DWORD dwExStyle;
    RECT Rect;
    LV_COLUMN Column;
    LV_ITEM Item;
    int iIndex;

    //
    //  Open the Inf file.
    //
    g_hIntlInf = SetupOpenInfFile(szIntlInf, NULL, INF_STYLE_WIN4, NULL);
    if (g_hIntlInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    if (!SetupOpenAppendInfFile(NULL, g_hIntlInf, NULL))
    {
        SetupCloseInfFile(g_hIntlInf);
        g_hIntlInf = NULL;
        return (FALSE);
    }

    //
    //  Get all supported language groups from the inf file.
    //
    if (Region_GetSupportedLanguageGroups(pDlgData) == FALSE)
    {
        return (FALSE);
    }

    //
    //  Close the inf file.
    //
    SetupCloseInfFile(g_hIntlInf);
    g_hIntlInf = NULL;

    //
    //  Enumerate all installed language groups.
    //
    if (Region_EnumInstalledLanguageGroups() == FALSE)
    {
        return (FALSE);
    }

    //
    //  Create a column for the list view.
    //
    GetClientRect(hwndLG, &Rect);
    Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    Column.fmt = LVCFMT_LEFT;
    Column.cx = Rect.right - GetSystemMetrics(SM_CYHSCROLL);
    Column.pszText = NULL;
    Column.cchTextMax = 0;
    Column.iSubItem = 0;
    ListView_InsertColumn(hwndLG, 0, &Column);

    //
    //  Set extended list view style to use the check boxes.
    //
    dwExStyle = ListView_GetExtendedListViewStyle(hwndLG);
    ListView_SetExtendedListViewStyle( hwndLG,
                                       dwExStyle |
                                         LVS_EX_CHECKBOXES |
                                         LVS_EX_FULLROWSELECT );

    //
    //  Go through the list of language groups and add each one to the
    //  list view and set the appropriate state.
    //
    pLG = pLanguageGroups;
    while (pLG)
    {
        //
        //  Insert the item into the list view.
        //
        Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
        Item.iItem = 0;
        Item.iSubItem = 0;
        Item.state = 0;
        Item.stateMask = LVIS_STATEIMAGEMASK;
        Item.pszText = pLG->pszName;
        Item.cchTextMax = 0;
        Item.iImage = 0;
        Item.lParam = (LPARAM)pLG;

        iIndex = ListView_InsertItem(hwndLG, &Item);

        //
        //  Set the checked state.
        //
        //  There's a bug in the list view code such that the check mark
        //  isn't displayed when you set the state through InsertItem, so
        //  we have to set it explicitly using SetItemState.
        //
        if (iIndex >= 0)
        {
            ListView_SetItemState( hwndLG,
                                   iIndex,
                                   (pLG->wStatus & ML_ORIG_INSTALLED)
                                     ? INDEXTOSTATEIMAGEMASK(LVIS_SELECTED)
                                     : INDEXTOSTATEIMAGEMASK(LVIS_FOCUSED),
                                   LVIS_STATEIMAGEMASK );
        }

        //
        //  Advance to the next language group.
        //
        pLG = pLG->pNext;
    }

    //
    //  Deselect all items.
    //
    iIndex = ListView_GetItemCount(hwndLG);
    while (iIndex > 0)
    {
        ListView_SetItemState( hwndLG,
                               iIndex - 1,
                               0,
                               LVIS_FOCUSED | LVIS_SELECTED );
        iIndex--;
    }

    //
    //  Select the first one in the list.
    //
    ListView_SetItemState( hwndLG,
                           0,
                           LVIS_FOCUSED | LVIS_SELECTED,
                           LVIS_FOCUSED | LVIS_SELECTED );

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_GetSupportedCodePages
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_GetSupportedCodePages()
{
    UINT CodePage;
    HANDLE hCodePage;
    LPCODEPAGE pCP;
    INFCONTEXT Context;
    TCHAR szSection[MAX_PATH];
    int LineCount, LineNum;
    CPINFOEX Info;

    //
    //  Get the number of supported code pages from the inf file.
    //
    LineCount = (UINT)SetupGetLineCount(g_hIntlInf, TEXT("CodePages"));
    if (LineCount <= 0)
    {
        return (FALSE);
    }

    //
    //  Go through all supported code pages in the inf file.
    //
    for (LineNum = 0; LineNum < LineCount; LineNum++)
    {
        if (SetupGetLineByIndex(g_hIntlInf, TEXT("CodePages"), LineNum, &Context) &&
            SetupGetIntField(&Context, 0, &CodePage))
        {
            //
            //  Create the new node.
            //
            if (!(hCodePage = GlobalAlloc(GHND, sizeof(CODEPAGE))))
            {
                return (FALSE);
            }
            pCP = GlobalLock(hCodePage);

            //
            //  Fill in the new node with the appropriate info.
            //
            pCP->wStatus = 0;
            pCP->CodePage = CodePage;
            pCP->hCodePage = hCodePage;
            (pCP->pszName)[0] = 0;

            //
            //  Get the appropriate display string.
            //
            if (GetCPInfoEx(CodePage, 0, &Info))
            {
                lstrcpy(pCP->pszName, Info.CodePageName);
            }
            else if (!SetupGetStringField(&Context, 1, pCP->pszName, MAX_PATH, NULL))
            {
                GlobalUnlock(hCodePage);
                GlobalFree(hCodePage);
                continue;
            }

            //
            //  See if this code page can be removed.
            //
            wsprintf(szSection, TEXT("%ws%d"), szCPRemovePrefix, CodePage);
            if ((CodePage == GetACP()) ||
                (CodePage == GetOEMCP()) ||
                (!SetupFindFirstLine( g_hIntlInf,
                                      szSection,
                                      TEXT("AddReg"),
                                      &Context )))
            {
                //
                //  Mark it as permanent.
                //  Also mark it as originally installed to avoid problems.
                //
                pCP->wStatus |= (ML_ORIG_INSTALLED | ML_PERMANENT);
            }

            //
            //  Add the code page to the front of the linked list.
            //
            pCP->pNext = pCodePages;
            pCodePages = pCP;
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_LoadCodePages
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_LoadCodePages(
    HWND hDlg)
{
    HWND hwndCP = GetDlgItem(hDlg, IDC_CODEPAGES);
    LPCODEPAGE pCP;
    DWORD dwExStyle;
    RECT Rect;
    LV_COLUMN Column;
    LV_ITEM Item;
    int iIndex;

    //
    //  Open the Inf file.
    //
    g_hIntlInf = SetupOpenInfFile(szIntlInf, NULL, INF_STYLE_WIN4, NULL);
    if (g_hIntlInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    if (!SetupOpenAppendInfFile(NULL, g_hIntlInf, NULL))
    {
        SetupCloseInfFile(g_hIntlInf);
        g_hIntlInf = NULL;
        return (FALSE);
    }

    //
    //  Get all supported code pages from the inf file.
    //
    if (Region_GetSupportedCodePages() == FALSE)
    {
        return (FALSE);
    }

    //
    //  Close the inf file.
    //
    SetupCloseInfFile(g_hIntlInf);
    g_hIntlInf = NULL;

    //
    //  Enumerate all installed code pages.
    //
    if (EnumSystemCodePages(Region_EnumInstalledCPProc, CP_INSTALLED) == FALSE)
    {
        return (FALSE);
    }

    //
    //  Create a column for the list view.
    //
    GetClientRect(hwndCP, &Rect);
    Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    Column.fmt = LVCFMT_LEFT;
    Column.cx = Rect.right - GetSystemMetrics(SM_CYHSCROLL);
    Column.pszText = NULL;
    Column.cchTextMax = 0;
    Column.iSubItem = 0;
    ListView_InsertColumn(hwndCP, 0, &Column);

    //
    //  Set extended list view style to use the check boxes.
    //
    dwExStyle = ListView_GetExtendedListViewStyle(hwndCP);
    ListView_SetExtendedListViewStyle( hwndCP,
                                       dwExStyle |
                                         LVS_EX_CHECKBOXES |
                                         LVS_EX_FULLROWSELECT );

    //
    //  Go through the list of code pages and add each one to the
    //  list view and set the appropriate state.
    //
    pCP = pCodePages;
    while (pCP)
    {
        //
        //  Insert the item into the list view.
        //
        Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
        Item.iItem = 0;
        Item.iSubItem = 0;
        Item.state = 0;
        Item.stateMask = LVIS_STATEIMAGEMASK;
        Item.pszText = pCP->pszName;
        Item.cchTextMax = 0;
        Item.iImage = 0;
        Item.lParam = (LPARAM)pCP;

        iIndex = ListView_InsertItem(hwndCP, &Item);

        //
        //  Set the checked state.
        //
        //  There's a bug in the list view code such that the check mark
        //  isn't displayed when you set the state through InsertItem, so
        //  we have to set it explicitly using SetItemState.
        //
        if (iIndex >= 0)
        {
            ListView_SetItemState( hwndCP,
                                   iIndex,
                                   (pCP->wStatus & ML_ORIG_INSTALLED)
                                     ? INDEXTOSTATEIMAGEMASK(LVIS_SELECTED)
                                     : INDEXTOSTATEIMAGEMASK(LVIS_FOCUSED),
                                   LVIS_STATEIMAGEMASK );
        }

        //
        //  Advance to the next code page.
        //
        pCP = pCP->pNext;
    }

    //
    //  Deselect all items.
    //
    iIndex = ListView_GetItemCount(hwndCP);
    while (iIndex > 0)
    {
        ListView_SetItemState( hwndCP,
                               iIndex - 1,
                               0,
                               LVIS_FOCUSED | LVIS_SELECTED );
        iIndex--;
    }

    //
    //  Select the first one in the list.
    //
    ListView_SetItemState( hwndCP,
                           0,
                           LVIS_FOCUSED | LVIS_SELECTED,
                           LVIS_FOCUSED | LVIS_SELECTED );

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_FreeGlobalInfo
//
//  Processing for a WM_DESTROY message.
//
////////////////////////////////////////////////////////////////////////////

void Region_FreeGlobalInfo()
{
    LPLANGUAGEGROUP pPreLG, pCurLG;
    HANDLE hAlloc;

    //
    //  Remove Language Group info.
    //
    pCurLG = pLanguageGroups;
    pLanguageGroups = NULL;

    while (pCurLG)
    {
        pPreLG = pCurLG;
        pCurLG = pPreLG->pNext;
        hAlloc = pPreLG->hLanguageGroup;
        GlobalUnlock(hAlloc);
        GlobalFree(hAlloc);
    }

    //
    //  Remove Alternate Sorts info.
    //
    g_NumAltSorts = 0;
    pAltSorts = NULL;
    GlobalUnlock(hAltSorts);
    GlobalFree(hAltSorts);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_FreeGlobalInfoAdvanced
//
//  Processing for a WM_DESTROY message.
//
////////////////////////////////////////////////////////////////////////////

void Region_FreeGlobalInfoAdvanced()
{
    LPCODEPAGE pPreCP, pCurCP;
    HANDLE hAlloc;

    //
    //  Remove Code Page info.
    //
    pCurCP = pCodePages;
    pCodePages = NULL;

    while (pCurCP)
    {
        pPreCP = pCurCP;
        pCurCP = pPreCP->pNext;
        hAlloc = pPreCP->hCodePage;
        GlobalUnlock(hAlloc);
        GlobalFree(hAlloc);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ListViewCustomDraw
//
//  Processing for a list view NM_CUSTOMDRAW notification message.
//
////////////////////////////////////////////////////////////////////////////

void Region_ListViewCustomDraw(
    HWND hDlg,
    LPNMLVCUSTOMDRAW pDraw)
{
    LPLANGUAGEGROUP pNode;

    //
    //  Tell the list view to notify me of item draws.
    //
    if (pDraw->nmcd.dwDrawStage == CDDS_PREPAINT)
    {
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
        return;
    }

    //
    //  Handle the Item Prepaint.
    //
    pNode = (LPLANGUAGEGROUP)(pDraw->nmcd.lItemlParam);
    if ((pDraw->nmcd.dwDrawStage & CDDS_ITEMPREPAINT) &&
        (pNode) && (pNode != (LPLANGUAGEGROUP)(LB_ERR)))
    {
        if (pNode->wStatus & (ML_PERMANENT | ML_DISABLE))
        {
            pDraw->clrText = (pDraw->nmcd.uItemState & CDIS_SELECTED)
                               ? ((GetSysColor(COLOR_HIGHLIGHT) ==
                                   GetSysColor(COLOR_GRAYTEXT))
                                      ? GetSysColor(COLOR_HIGHLIGHTTEXT)
                                      : GetSysColor(COLOR_GRAYTEXT))
                               : GetSysColor(COLOR_GRAYTEXT);
        }
    }

    //
    //  Do the default action.
    //
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ListViewChanging
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_ListViewChanging(
    HWND hDlg,
    NM_LISTVIEW *pLV)
{
    LPLANGUAGEGROUP pNode;

    //
    //  Make sure it's a state change message.
    //
    if ((!(pLV->uChanged & LVIF_STATE)) ||
        ((pLV->uNewState & 0x3000) == 0))
    {
        return (FALSE);
    }

    //
    //  Get the item data for the currently selected item.
    //
    //  NOTE: Both the language group structure and the code page structure
    //        have wStatus as the first field, so just use the language
    //        group structure to access the field.
    //
    pNode = (LPLANGUAGEGROUP)pLV->lParam;

    //
    //  Make sure we're not trying to change a permanent or disabled
    //  language group or code page.  If so, return TRUE to prevent the
    //  change.
    //
    if ((pNode) && (pNode->wStatus & (ML_PERMANENT | ML_DISABLE)))
    {
        return (TRUE);
    }

    //
    //  Return FALSE to allow the change.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ListViewChanged
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_ListViewChanged(
    HWND hDlg,
    int iID,
    NM_LISTVIEW *pLV)
{
    HWND hwndLV = GetDlgItem(hDlg, iID);
    LPLANGUAGEGROUP pNode;
    BOOL bChecked;
    int iCount;

    //
    //  Make sure it's a state change message.
    //
    if ((!(pLV->uChanged & LVIF_STATE)) ||
        ((pLV->uNewState & 0x3000) == 0))
    {
        return (FALSE);
    }
    //
    //  Get the state of the check box for the currently selected item.
    //
    bChecked = ListView_GetCheckState(hwndLV, pLV->iItem) ? TRUE : FALSE;

    //
    //  Get the item data for the currently selected item.
    //
    //  NOTE: Both the language group structure and the code page structure
    //        have wStatus as the first field, so just use the language
    //        group structure to access the field.
    //
    pNode = (LPLANGUAGEGROUP)pLV->lParam;

    //
    //  Make sure we're not trying to change a permanent or disabled
    //  language group or code page.  If so, set the check box to its
    //  appropriate state.
    //
    if (pNode->wStatus & (ML_PERMANENT | ML_DISABLE))
    {
        if (pNode->wStatus & ML_PERMANENT)
        {
            if (bChecked == FALSE)
            {
                ListView_SetCheckState(hwndLV, pLV->iItem, TRUE);
            }
        }
        else            // ML_DISABLE only
        {
            if ((bChecked == FALSE) && (pNode->wStatus & ML_ORIG_INSTALLED))
            {
                ListView_SetCheckState(hwndLV, pLV->iItem, TRUE);
            }
            else if ((bChecked == TRUE) && (!(pNode->wStatus & ML_ORIG_INSTALLED)))
            {
                ListView_SetCheckState(hwndLV, pLV->iItem, FALSE);
            }
        }
        return (FALSE);
    }

    //
    //  Store the proper info in the language group or code page structure.
    //
    pNode->wStatus &= (ML_ORIG_INSTALLED | ML_STATIC);
    pNode->wStatus |= ((bChecked) ? ML_INSTALL : ML_REMOVE);

    //
    //  Deselect all items.
    //
    iCount = ListView_GetItemCount(hwndLV);
    while (iCount > 0)
    {
        ListView_SetItemState( hwndLV,
                               iCount - 1,
                               0,
                               LVIS_FOCUSED | LVIS_SELECTED );
        iCount--;
    }

    //
    //  Make sure this item is selected.
    //
    ListView_SetItemState( hwndLV,
                           pLV->iItem,
                           LVIS_FOCUSED | LVIS_SELECTED,
                           LVIS_FOCUSED | LVIS_SELECTED );

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_InitInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_InitInf(
    HWND hDlg,
    HINF *phIntlInf,
    LPTSTR pszInf,
    HSPFILEQ *pFileQueue,
    PVOID *pQueueContext)
{
    //
    //  Open the Inf file.
    //
    *phIntlInf = SetupOpenInfFile(pszInf, NULL, INF_STYLE_WIN4, NULL);
    if (*phIntlInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    if (!SetupOpenAppendInfFile(NULL, *phIntlInf, NULL))
    {
        SetupCloseInfFile(*phIntlInf);
        return (FALSE);
    }

    //
    //  Create a setup file queue and initialize default setup
    //  copy queue callback context.
    //
    *pFileQueue = SetupOpenFileQueue();
    if ((!*pFileQueue) || (*pFileQueue == INVALID_HANDLE_VALUE))
    {
        SetupCloseInfFile(*phIntlInf);
        return (FALSE);
    }

    //
    //  Don't display FileCopy progress operation during GUI mode setup.
    //
    *pQueueContext = SetupInitDefaultQueueCallbackEx( hDlg,
                                                      (g_bSetupCase ? INVALID_HANDLE_VALUE : NULL),
                                                      0L,
                                                      0L,
                                                      NULL );
    if (!*pQueueContext)
    {
        SetupCloseFileQueue(*pFileQueue);
        SetupCloseInfFile(*phIntlInf);
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_CloseInf
//
////////////////////////////////////////////////////////////////////////////

void Region_CloseInf(
    HINF hIntlInf,
    HSPFILEQ FileQueue,
    PVOID QueueContext)
{
    //
    //  Terminate the Queue.
    //
    SetupTermDefaultQueueCallback(QueueContext);

    //
    //  Close the file queue.
    //
    SetupCloseFileQueue(FileQueue);

    //
    //  Close the Inf file.
    //
    SetupCloseInfFile(hIntlInf);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_InitSystemLocales
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_InitSystemLocales(
    HWND hDlg)
{
    TCHAR szSystemBuf[SIZE_128];
    TCHAR szDefaultSystemBuf[SIZE_128];
    TCHAR szBuf[SIZE_128];
    DWORD dwIndex;
    HWND hSystemLocale = GetDlgItem(hDlg, IDC_SYSTEM_LOCALE);

    //
    //  Get the list of locales and fill in the system locale
    //  combo box.
    //
    Region_EnumLocales(hDlg, hSystemLocale, TRUE);

    //
    //  Get the string for the system default setting.
    //  Special case Spanish.
    //
    if ((SysLocaleID == LCID_SPANISH_TRADITIONAL) ||
        (SysLocaleID == LCID_SPANISH_INTL))
    {
        LoadString(hInstance, IDS_SPANISH_NAME, szSystemBuf, SIZE_128);
    }
    else
    {
        GetLocaleInfo(SysLocaleID, LOCALE_SLANGUAGE, szSystemBuf, SIZE_128);
    }

    //
    //  Select the current system default locale id in the list.
    //
    dwIndex = ComboBox_FindStringExact(hSystemLocale, -1, szSystemBuf);
    if (dwIndex == CB_ERR)
    {
        dwIndex = ComboBox_FindStringExact(hSystemLocale, -1, szDefaultSystemBuf);
        if (dwIndex == CB_ERR)
        {
            GetLocaleInfo(US_LOCALE, LOCALE_SLANGUAGE, szBuf, SIZE_128);
            dwIndex = ComboBox_FindStringExact(hSystemLocale, -1, szBuf);
            if (dwIndex == CB_ERR)
            {
                dwIndex = 0;
            }
        }
    }
    ComboBox_SetCurSel(hSystemLocale, dwIndex);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_SetDefaultCommandOK
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_SetDefaultCommandOK(
    HWND hDlg)
{
    HWND hSystemLocale = GetDlgItem(hDlg, IDC_SYSTEM_LOCALE);
    DWORD dwLocale;
    LCID NewLocale;
    HCURSOR hcurSave;

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  Get the current selection.
    //
    dwLocale = ComboBox_GetCurSel(hSystemLocale);

    //
    //  Get the locale id for the current selection and save it.
    //
    NewLocale = (LCID)ComboBox_GetItemData(hSystemLocale, dwLocale);
    if (IsValidLocale(NewLocale, LCID_SUPPORTED))
    {
        SysLocaleID = NewLocale;
    }
    else
    {
        //
        //  This shouldn't happen, since the values in the combo box
        //  should already be installed via the language groups.
        //  Put up an error message just in case.
        //
        SetCursor(hcurSave);
        Region_ShowMsg( NULL,
                        IDS_SETUP_STRING,
                        IDS_TITLE_STRING,
                        MB_OK_OOPS,
                        NULL );
        return (FALSE);
    }

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_SetDefaultProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR Region_SetDefaultProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            if (Region_InitSystemLocales(hDlg) == FALSE)
            {
                return (FALSE);
            }
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aRegionSetDefaultHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aRegionSetDefaultHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    Region_SetDefaultCommandOK(hDlg);

                    // fall thru...
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hDlg, (wParam == IDOK) ? 1 : 0);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
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
//  Region_CommandSetDefault
//
////////////////////////////////////////////////////////////////////////////

int Region_CommandSetDefault(
    HWND hDlg,
    LPREGDLGDATA pDlgData)
{
    int rc = 0;

    //
    //  Bring up the appropriate dialog box.
    //
    if (pDlgData != NULL)
    {
        //
        //  Return value can be 1:IDOK, 2:IDCANCEL or -1:Error (from USER)
        //
        if ((rc = (int)DialogBoxParam( hInstance,
                                  MAKEINTRESOURCE(DLG_REGION_SET_DEFAULT),
                                  hDlg,
                                  Region_SetDefaultProc,
                                  (LPARAM)(pDlgData) )) != IDOK)
        {
            //
            //  Failure, so need to return 0.
            //
            rc = 0;
        }
        else
        {
            //
            //  Set the RC_SystemLocale change flag.
            //
            pDlgData->Changes |= RC_SystemLocale;
            PropSheet_Changed(GetParent(hDlg), hDlg);
        }
    }
    else
    {
        MessageBeep(MB_ICONEXCLAMATION);
    }

    //
    //  Return the result.
    //
    return (rc);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_AdvancedCommandOK
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_AdvancedCommandOK(
    HWND hDlg)
{
    LPCODEPAGE pCP;
    HINF hIntlInf;
    HSPFILEQ FileQueue;
    PVOID QueueContext;
    BOOL bInitInf = FALSE;
    BOOL fAdd;
    TCHAR szSection[MAX_PATH];
    HCURSOR hcurSave;
    BOOL bRet = TRUE;

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  Go through each code page node to see if anything needs to
    //  be done.
    //
    pCP = pCodePages;
    while (pCP)
    {
        //
        //  See if any changes are necessary for this code page.
        //
        if ((pCP->wStatus == ML_INSTALL) ||
            (pCP->wStatus == (ML_ORIG_INSTALLED | ML_REMOVE)))
        {
            //
            //  See if we're installing or removing.
            //
            fAdd = (pCP->wStatus == ML_INSTALL);

            //
            //  Initialize Inf stuff.
            //
            if ((!bInitInf) &&
                (!Region_InitInf(hDlg, &hIntlInf, szIntlInf, &FileQueue, &QueueContext)))
            {
                SetCursor(hcurSave);
                return (FALSE);
            }
            bInitInf = TRUE;

            //
            //  Get the inf section name.
            //
            wsprintf( szSection,
                      TEXT("%ws%d"),
                      fAdd ? szCPInstallPrefix : szCPRemovePrefix,
                      pCP->CodePage );

            //
            //  Enqueue the code page files so that they may be
            //  copied.  This only handles the CopyFiles entries in the
            //  inf file.
            //
            if (!SetupInstallFilesFromInfSection( hIntlInf,
                                                  NULL,
                                                  FileQueue,
                                                  szSection,
                                                  pSetupSourcePath,
                                                  SP_COPY_NEWER ))
            {
                //
                //  Setup failed to find the code page.
                //  This shouldn't happen - the inf file is messed up.
                //
                Region_ShowMsg( hDlg,
                                IDS_ML_COPY_FAILED,
                                0,
                                MB_OK_OOPS,
                                pCP->pszName );
                pCP->wStatus = 0;
            }
        }

        //
        //  Go to the next code page node.
        //
        pCP = pCP->pNext;
    }

    if (bInitInf)
    {
        DWORD d;

        //
        //  See if we need to install any files.
        //
        //  d = 0: User wants new files or some files were missing;
        //         Must commit queue.
        //
        //  d = 1: User wants to use existing files and queue is empty;
        //         Can skip committing queue.
        //
        //  d = 2: User wants to use existing files, but del/ren queues
        //         not empty.  Must commit queue.  The copy queue will
        //         have been emptied, so only del/ren functions will be
        //         performed.
        //
        if ((SetupScanFileQueue( FileQueue,
                                 SPQ_SCAN_PRUNE_COPY_QUEUE |
                                   SPQ_SCAN_FILE_VALIDITY,
                                 hDlg,
                                 NULL,
                                 NULL,
                                 &d )) && (d != 1))
        {
            //
            //  Copy the files in the queue.
            //
            if (!SetupCommitFileQueue( hDlg,
                                       FileQueue,
                                       SetupDefaultQueueCallback,
                                       QueueContext ))
            {
                //
                //  This can happen if the user hits Cancel from within
                //  the setup dialog.
                //
                Region_ShowMsg( hDlg,
                                IDS_ML_SETUP_FAILED,
                                0,
                                MB_OK_OOPS,
                                NULL );
                bRet = FALSE;
                goto Region_AdvancedSetupError;
            }
        }

        //
        //  Execute all of the other code page entries in the inf file.
        //
        pCP = pCodePages;
        while (pCP)
        {
            //
            //  See if any changes are necessary for this code page.
            //
            if ((pCP->wStatus == ML_INSTALL) ||
                (pCP->wStatus == (ML_ORIG_INSTALLED | ML_REMOVE)))
            {
                fAdd = (pCP->wStatus == ML_INSTALL);

                //
                //  Get the inf section name.
                //
                wsprintf( szSection,
                          TEXT("%ws%d"),
                          fAdd ? szCPInstallPrefix : szCPRemovePrefix,
                          pCP->CodePage );

                //
                //  Call setup to install other inf info for this
                //  code page.
                //
                if (!SetupInstallFromInfSection( hDlg,
                                                 hIntlInf,
                                                 szSection,
                                                 SPINST_ALL & ~SPINST_FILES,
                                                 NULL,
                                                 pSetupSourcePath,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 NULL ))
                {
                    //
                    //  Setup failed.
                    //
                    //  Already copied the code page file, so no need to
                    //  change the status of the code page info here.
                    //
                    //  This shouldn't happen - the inf file is messed up.
                    //
                    Region_ShowMsg( hDlg,
                                    IDS_ML_INSTALL_FAILED,
                                    0,
                                    MB_OK_OOPS,
                                    pCP->pszName );
                }

                //
                //  Reset the status to show the new state of this
                //  code page.
                //
                pCP->wStatus &= (ML_STATIC);
                if (fAdd)
                {
                    pCP->wStatus |= ML_ORIG_INSTALLED;
                }
            }

            //
            //  Clear out wStatus and go to the next code page node.
            //
            pCP->wStatus &= (ML_ORIG_INSTALLED | ML_STATIC);
            pCP = pCP->pNext;
        }

Region_AdvancedSetupError:
        //
        //  Close Inf stuff.
        //
        Region_CloseInf(hIntlInf, FileQueue, QueueContext);
    }

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  Return success.
    //
    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_AdvancedProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR Region_AdvancedProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            if (Region_LoadCodePages(hDlg) == FALSE)
            {
                return (FALSE);
            }
            break;
        }
        case ( WM_DESTROY ) :
        {
            Region_FreeGlobalInfoAdvanced();
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aRegionAdvancedHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aRegionAdvancedHelpIds );
            break;
        }
        case ( WM_NOTIFY ) :
        {
            LPNMHDR psn = (NMHDR *)lParam;
            switch (psn->code)
            {
                case ( NM_CUSTOMDRAW ) :
                {
                    Region_ListViewCustomDraw(hDlg, (LPNMLVCUSTOMDRAW)lParam);
                    return (TRUE);

                    break;
                }
                case ( LVN_ITEMCHANGING ) :
                {
                    return (Region_ListViewChanging( hDlg,
                                                     (NM_LISTVIEW *)lParam ));
                    break;
                }
                case ( LVN_ITEMCHANGED ) :
                {
                    Region_ListViewChanged( hDlg,
                                            IDC_CODEPAGES,
                                            (NM_LISTVIEW *)lParam );
                    break;
                }
                case ( NM_CLICK ) :
                case ( NM_DBLCLK ) :
                {
                    LV_HITTESTINFO ht;
                    HWND hwndList = GetDlgItem(hDlg, IDC_CODEPAGES);

                    if (psn->idFrom != IDC_CODEPAGES)
                    {
                        return (FALSE);
                    }

                    //
                    //  Get where we were hit and then translate it to our
                    //  window.
                    //
                    GetCursorPos(&ht.pt);
                    ScreenToClient(hwndList, &ht.pt);
                    ListView_HitTest(hwndList, &ht);
                    if ((ht.iItem >= 0) && ((ht.flags & LVHT_ONITEM) == LVHT_ONITEMLABEL))
                    {
                        UINT state;

                        //
                        //  The user clicked on the item label.  Simulate a
                        //  state change so we can process it.
                        //
                        state = ListView_GetItemState( hwndList,
                                                       ht.iItem,
                                                       LVIS_STATEIMAGEMASK );
                        state ^= INDEXTOSTATEIMAGEMASK(LVIS_SELECTED | LVIS_FOCUSED);

                        //
                        //  The state is either selected or focused.  Flip the
                        //  bits.  The SetItemState causes the system to bounce
                        //  back a notification for LVN_ITEMCHANGED and the
                        //  code then does the right thing.  Note -- we MUST
                        //  check for LVHT_ONITEMLABEL.  If we do this code for
                        //  LVHT_ONITEMSTATEICON, the code will get 2
                        //  ITEMCHANGED notifications, and the state will stay
                        //  where it is, which is not good.  If we want this
                        //  to also fire if the guy clicks in the empty space
                        //  right of the label text, we need to look for
                        //  LVHT_ONITEM as well as LVHT_ONITEMLABEL.
                        //
                        ListView_SetItemState( hwndList,
                                               ht.iItem,
                                               state,
                                               LVIS_STATEIMAGEMASK );
                    }
                }
                default :
                {
                    return (FALSE);
                }
            }

            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    Region_AdvancedCommandOK(hDlg);

                    // fall thru...
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hDlg, (wParam == IDOK) ? 1 : 0);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
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
//  Region_CommandAdvanced
//
////////////////////////////////////////////////////////////////////////////

int Region_CommandAdvanced(
    HWND hDlg,
    LPREGDLGDATA pDlgData)
{
    int rc = 0;

    //
    //  Bring up the appropriate dialog box.
    //
    if (pDlgData != NULL)
    {
        //
        //  Return value can be 1:IDOK, 2:IDCANCEL or -1:Error (from USER)
        //
        if ((rc = (int)DialogBoxParam( hInstance,
                                  MAKEINTRESOURCE(DLG_REGION_ADVANCED),
                                  hDlg,
                                  Region_AdvancedProc,
                                  (LPARAM)(pDlgData) )) != IDOK)
        {
            //
            //  Failure, so need to return 0.
            //
            rc = 0;
        }
    }
    else
    {
        MessageBeep(MB_ICONEXCLAMATION);
    }

    //
    //  Return the result.
    //
    return (rc);
}


////////////////////////////////////////////////////////////////////////////
//
//  GeneralDlgProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK GeneralDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    LPREGDLGDATA pDlgData = lpPropSheet ? (LPREGDLGDATA)lpPropSheet->lParam : NULL;
    NM_LISTVIEW *pLV;

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            if (!Region_InitPropSheet(hDlg, (LPPROPSHEETPAGE)lParam))
            {
                PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
            }
            break;
        }
        case ( WM_DESTROY ) :
        {
            Region_FreeDeskCPL();
            Region_FreeGlobalInfo();
            if (pDlgData)
            {
                lpPropSheet->lParam = 0;
                LocalFree((HANDLE)pDlgData);
            }
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aRegionHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aRegionHelpIds );
            break;
        }
        case ( WM_NOTIFY ) :
        {
            LPNMHDR psn = (NMHDR *)lParam;
            switch (psn->code)
            {
                case ( PSN_SETACTIVE ) :
                {
                    break;
                }
                case ( PSN_KILLACTIVE ) :
                {
                    //
                    //  Validate the entries on the property page.
                    //
                    if (pDlgData)
                    {
                        SetWindowLongPtr( hDlg,
                                       DWLP_MSGRESULT,
                                       !Region_ValidatePPS( hDlg,
                                                            pDlgData ) );
                    }
                    break;
                }
                case ( PSN_APPLY ) :
                {
                    if (pDlgData)
                    {
                        //
                        //  Apply the settings.
                        //
                        if (Region_ApplySettings(hDlg, pDlgData))
                        {
                            SetWindowLongPtr( hDlg,
                                           DWLP_MSGRESULT,
                                           PSNRET_NOERROR );

                            //
                            //  Zero out the RC_EverChg bit.
                            //
                            pDlgData->Changes = 0;
                        }
                        else
                        {
                            SetWindowLongPtr( hDlg,
                                           DWLP_MSGRESULT,
                                           PSNRET_INVALID_NOCHANGEPAGE );
                        }
                    }
                    break;
                }
                case ( NM_CUSTOMDRAW ) :
                {
                    Region_ListViewCustomDraw(hDlg, (LPNMLVCUSTOMDRAW)lParam);
                    return (TRUE);

                    break;
                }
                case ( LVN_ITEMCHANGING ) :
                {
                    //
                    //  Get the list view item.
                    //
                    pLV = (NM_LISTVIEW *)lParam;

                    //
                    //  Handle the message.
                    //
                    return (Region_ListViewChanging(hDlg, pLV));

                    break;
                }
                case ( LVN_ITEMCHANGED ) :
                {
                    //
                    //  Get the list view item.
                    //
                    pLV = (NM_LISTVIEW *)lParam;

                    //
                    //  Save the change to the language groups.
                    //
                    if (Region_ListViewChanged(hDlg, IDC_LANGUAGE_GROUPS, pLV))
                    {
                        //
                        //  Reset the locale combo boxes to contain
                        //  the appropriate locales based on the chosen
                        //  language groups.
                        //
                        Region_SetValues(hDlg, pDlgData, FALSE);

                        //
                        //  Note that the language groups have changed and
                        //  enable the apply button.
                        //
                        if (pDlgData)
                        {
                            pDlgData->Changes |= RC_LangGroups;
                        }
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }
                    else
                    {
                        return (FALSE);
                    }

                    break;
                }
                case ( NM_CLICK ) :
                case ( NM_DBLCLK ) :
                {
                    LV_HITTESTINFO ht;
                    HWND hwndList = GetDlgItem(hDlg, IDC_LANGUAGE_GROUPS);
                    if (psn->idFrom != IDC_LANGUAGE_GROUPS)
                    {
                        return (FALSE);
                    }

                    //
                    //  Get where we were hit and then translate it to our
                    //  window.
                    //
                    GetCursorPos(&ht.pt);
                    ScreenToClient(hwndList, &ht.pt);
                    ListView_HitTest(hwndList, &ht);
                    if ((ht.iItem >= 0) && ((ht.flags & LVHT_ONITEM) == LVHT_ONITEMLABEL))
                    {
                        UINT state;

                        //
                        //  The user clicked on the item label.  Simulate a
                        //  state change so we can process it.
                        //
                        state = ListView_GetItemState( hwndList,
                                                       ht.iItem,
                                                       LVIS_STATEIMAGEMASK );
                        state ^= INDEXTOSTATEIMAGEMASK(LVIS_SELECTED | LVIS_FOCUSED);
                        //
                        //  The state is either selected or focused.  Flip the
                        //  bits.  The SetItemState causes the system to bounce
                        //  back a notification for LVN_ITEMCHANGED and the
                        //  code then does the right thing.  Note -- we MUST
                        //  check for LVHT_ONITEMLABEL.  If we do this code for
                        //  LVHT_ONITEMSTATEICON, the code will get 2
                        //  ITEMCHANGED notifications, and the state will stay
                        //  where it is, which is not good.  If we want this to
                        //  also fire if the guy clicks in the empty space
                        //  right of the label text, we need to look for
                        //  LVHT_ONITEM as well as LVHT_ONITEMLABEL.
                        //
                        ListView_SetItemState( hwndList,
                                               ht.iItem,
                                               state,
                                               LVIS_STATEIMAGEMASK );
                    }
                }
                default :
                {
                    return (FALSE);
                }
            }

            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDC_USER_LOCALE ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        if (pDlgData)
                        {
                            pDlgData->Changes |= RC_UserLocale;
                            Region_UpdateSortingCombo(hDlg, 0);
                        }
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }
                    break;
                }
                case ( IDC_SORTING ) :
                {
                    //
                    //  See if it's a selection change.
                    //
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        if (pDlgData)
                        {
                            pDlgData->Changes |= RC_UserLocale;
                        }
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }
                    break;
                }
                case ( IDC_UI_LANGUAGE ) :
                {
                    //
                    //  See if it's a selection change.
                    //
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        if (pDlgData)
                        {
                            pDlgData->Changes |= RC_UILanguage;
                        }
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }
                    break;
                }
                case ( IDC_SET_DEFAULT ) :
                {
                    Region_CommandSetDefault(hDlg, pDlgData);
                    break;
                }
                case ( IDC_ADVANCED ) :
                {
                    Region_CommandAdvanced(hDlg, pDlgData);
                    break;
                }
            }
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
//  Region_RunRegApps
//
////////////////////////////////////////////////////////////////////////////

void Region_RunRegApps(
    LPCTSTR pszRegKey)
{
    HKEY hkey;
    BOOL bOK = TRUE;
    DWORD cbData, cbValue, dwType, ctr;
    TCHAR szValueName[32], szCmdLine[MAX_PATH];
    STARTUPINFO startup;
    PROCESS_INFORMATION pi;

    if (RegOpenKey( HKEY_LOCAL_MACHINE,
                    pszRegKey,
                    &hkey ) == ERROR_SUCCESS)
    {
        startup.cb = sizeof(STARTUPINFO);
        startup.lpReserved = NULL;
        startup.lpDesktop = NULL;
        startup.lpTitle = NULL;
        startup.dwFlags = 0L;
        startup.cbReserved2 = 0;
        startup.lpReserved2 = NULL;
    //  startup.wShowWindow = wShowWindow;

        for (ctr = 0; ; ctr++)
        {
            LONG lEnum;

            cbValue = sizeof(szValueName) / sizeof(TCHAR);
            cbData = sizeof(szCmdLine);

            if ((lEnum = RegEnumValue( hkey,
                                       ctr,
                                       szValueName,
                                       &cbValue,
                                       NULL,
                                       &dwType,
                                       (LPBYTE)szCmdLine,
                                       &cbData )) == ERROR_MORE_DATA)
            {
                //
                //  ERROR_MORE_DATA means the value name or data was too
                //  large, so skip to the next item.
                //
                continue;
            }
            else if (lEnum != ERROR_SUCCESS)
            {
                //
                //  This could be ERROR_NO_MORE_ENTRIES, or some kind of
                //  failure.  We can't recover from any other registry
                //  problem anyway.
                //
                break;
            }

            //
            //  Found a value.
            //
            if (dwType == REG_SZ)
            {
                //
                //  Adjust for shift in value index.
                //
                ctr--;

                //
                //  Delete the value.
                //
                RegDeleteValue(hkey, szValueName);

                //
                //  Only run things marked with a "*" in clean boot.
                //
                if (CreateProcess( NULL,
                                   szCmdLine,
                                   NULL,
                                   NULL,
                                   FALSE,
                                   CREATE_NEW_PROCESS_GROUP,
                                   NULL,
                                   NULL,
                                   &startup,
                                   &pi ))
                {
                    WaitForSingleObjectEx(pi.hProcess, INFINITE, TRUE);

                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
        }
        RegCloseKey(hkey);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_SetupLanguageGroups
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_SetupLanguageGroups(
    HWND hDlg,
    LPBOOL pbReboot,
    LPBOOL pbUserCancel)
{
    LPLANGUAGEGROUP pLG;
    HINF hIntlInf;
    HSPFILEQ FileQueue;
    PVOID QueueContext;
    BOOL bInitInf = FALSE;
    BOOL fAdd;
    TCHAR szSection[MAX_PATH];
    HCURSOR hcurSave;
    BOOL bRet = TRUE;

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  Go through each language group node to see if anything needs to
    //  be done.
    //
    pLG = pLanguageGroups;
    while (pLG)
    {
        //
        //  See if any changes are necessary for this language group.
        //
        if ((pLG->wStatus == ML_INSTALL) ||
            (pLG->wStatus == (ML_ORIG_INSTALLED | ML_REMOVE)))
        {
            //
            //  See if we're installing or removing.
            //
            fAdd = (pLG->wStatus == ML_INSTALL);

            //
            //  Initialize Inf stuff.
            //
            if ((!bInitInf) &&
                (!Region_InitInf(hDlg, &hIntlInf, szIntlInf, &FileQueue, &QueueContext)))
            {
                SetCursor(hcurSave);
                return (FALSE);
            }
            bInitInf = TRUE;

            //
            //  Get the inf section name.
            //
            wsprintf( szSection,
                      TEXT("%ws%d"),
                      fAdd ? szLGInstallPrefix : szLGRemovePrefix,
                      pLG->LanguageGroup );

            //
            //  Enqueue the language group files so that they may be
            //  copied.  This only handles the CopyFiles entries in the
            //  inf file.
            //
            if (!SetupInstallFilesFromInfSection( hIntlInf,
                                                  NULL,
                                                  FileQueue,
                                                  szSection,
                                                  pSetupSourcePath,
                                                  SP_COPY_NEWER ))
            {
                //
                //  Setup failed to find the language group.
                //  This shouldn't happen - the inf file is messed up.
                //
                Region_ShowMsg( hDlg,
                                IDS_ML_COPY_FAILED,
                                0,
                                MB_OK_OOPS,
                                pLG->pszName );
                pLG->wStatus = ML_DISABLE;
            }
            else
            {
                *pbReboot = TRUE;
            }
        }

        //
        //  Go to the next language group node.
        //
        pLG = pLG->pNext;
    }

    if (bInitInf)
    {
        DWORD d;

        //
        //  See if we need to install any files.
        //
        //  d = 0: User wants new files or some files were missing;
        //         Must commit queue.
        //
        //  d = 1: User wants to use existing files and queue is empty;
        //         Can skip committing queue.
        //
        //  d = 2: User wants to use existing files, but del/ren queues
        //         not empty.  Must commit queue.  The copy queue will
        //         have been emptied, so only del/ren functions will be
        //         performed.
        //
        if ((SetupScanFileQueue( FileQueue,
                                 SPQ_SCAN_PRUNE_COPY_QUEUE |
                                   SPQ_SCAN_FILE_VALIDITY,
                                 hDlg,
                                 NULL,
                                 NULL,
                                 &d )) && (d != 1))
        {
            //
            //  Copy the files in the queue.
            //
            if (!SetupCommitFileQueue( hDlg,
                                       FileQueue,
                                       SetupDefaultQueueCallback,
                                       QueueContext ))
            {
                //
                //  This can happen if the user hits Cancel from within
                //  the setup dialog.
                //
                Region_ShowMsg( hDlg,
                                IDS_ML_SETUP_FAILED,
                                0,
                                MB_OK_OOPS,
                                NULL );
                *pbUserCancel = TRUE;
                bRet = FALSE;
                goto Region_LanguageGroupSetupError;
            }
        }

        //
        //  Execute all of the other language group entries in the inf file.
        //
        pLG = pLanguageGroups;
        while (pLG)
        {
            //
            //  See if any changes are necessary for this language group.
            //
            if ((pLG->wStatus == ML_INSTALL) ||
                (pLG->wStatus == (ML_ORIG_INSTALLED | ML_REMOVE)))
            {
                fAdd = (pLG->wStatus == ML_INSTALL);

                //
                //  Get the inf section name.
                //
                wsprintf( szSection,
                          TEXT("%ws%d"),
                          fAdd ? szLGInstallPrefix : szLGRemovePrefix,
                          pLG->LanguageGroup );

                //
                //  Call setup to install other inf info for this
                //  language group.
                //
                if (!SetupInstallFromInfSection( hDlg,
                                                 hIntlInf,
                                                 szSection,
                                                 SPINST_ALL & ~SPINST_FILES,
                                                 NULL,
                                                 pSetupSourcePath,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 NULL ))
                {
                    //
                    //  Setup failed.
                    //
                    //  Already copied the language group file, so no need to
                    //  change the status of the language group info here.
                    //
                    //  This shouldn't happen - the inf file is messed up.
                    //
                    Region_ShowMsg( hDlg,
                                    IDS_ML_INSTALL_FAILED,
                                    0,
                                    MB_OK_OOPS,
                                    pLG->pszName );
                }
                else
                {
                    *pbReboot = TRUE;
                }

                //
                //  Reset the status to show the new state of this
                //  language group.
                //
                pLG->wStatus &= (ML_STATIC);
                if (fAdd)
                {
                    pLG->wStatus |= ML_ORIG_INSTALLED;
                }
            }

            //
            //  Clear out wStatus and go to the next language group node.
            //
            pLG->wStatus &= (ML_ORIG_INSTALLED | ML_STATIC);
            pLG = pLG->pNext;
        }

        //
        //  Run any necessary apps (for IME installation).
        //
        Region_RunRegApps(c_szIntlRun);

Region_LanguageGroupSetupError:
        //
        //  Close Inf stuff.
        //
        Region_CloseInf(hIntlInf, FileQueue, QueueContext);
    }

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  Return the result.
    //
    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_RebootTheSystem
//
//  This routine enables all privileges in the token, calls ExitWindowsEx
//  to reboot the system, and then resets all of the privileges to their
//  old state.
//
////////////////////////////////////////////////////////////////////////////

VOID Region_RebootTheSystem()
{
    HANDLE Token = NULL;
    ULONG ReturnLength, Index;
    PTOKEN_PRIVILEGES NewState = NULL;
    PTOKEN_PRIVILEGES OldState = NULL;
    BOOL Result;

    Result = OpenProcessToken( GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                               &Token );
    if (Result)
    {
        ReturnLength = 4096;
        NewState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        OldState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        Result = (BOOL)((NewState != NULL) && (OldState != NULL));
        if (Result)
        {
            Result = GetTokenInformation( Token,            // TokenHandle
                                          TokenPrivileges,  // TokenInformationClass
                                          NewState,         // TokenInformation
                                          ReturnLength,     // TokenInformationLength
                                          &ReturnLength );  // ReturnLength
            if (Result)
            {
                //
                //  Set the state settings so that all privileges are
                //  enabled...
                //
                if (NewState->PrivilegeCount > 0)
                {
                    for (Index = 0; Index < NewState->PrivilegeCount; Index++)
                    {
                        NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
                    }
                }

                Result = AdjustTokenPrivileges( Token,           // TokenHandle
                                                FALSE,           // DisableAllPrivileges
                                                NewState,        // NewState
                                                ReturnLength,    // BufferLength
                                                OldState,        // PreviousState
                                                &ReturnLength ); // ReturnLength
                if (Result)
                {
                    ExitWindowsEx(EWX_REBOOT, 0);


                    AdjustTokenPrivileges( Token,
                                           FALSE,
                                           OldState,
                                           0,
                                           NULL,
                                           NULL );
                }
            }
        }
    }

    if (NewState != NULL)
    {
        LocalFree(NewState);
    }
    if (OldState != NULL)
    {
        LocalFree(OldState);
    }
    if (Token != NULL)
    {
        CloseHandle(Token);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_UpdateIndicator
//
////////////////////////////////////////////////////////////////////////////

void Region_UpdateIndicator(
    BOOL bCreate)
{
    HWND hwndIndicate;
    HKEY hKey;

    //
    //  Update the task bar indicator.
    //
    hwndIndicate = FindWindow(szIndicator, NULL);
    if (hwndIndicate && IsWindow(hwndIndicate))
    {
        SendMessage(hwndIndicate, WM_COMMAND, IDM_NEWSHELL, 0L);
    }
    else if (bCreate)
    {
        WinExec(szInternatA, SW_SHOWMINNOACTIVE);
        if (RegCreateKey( HKEY_CURRENT_USER,
                          REGSTR_PATH_RUN,
                          &hKey ) == ERROR_SUCCESS)
        {
            RegSetValueEx( hKey,
                           szInternat,
                           0,
                           REG_SZ,
                           (LPBYTE)szInternat,
                           sizeof(szInternat) );
            RegCloseKey(hKey);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_InstallKeyboardLayout
//
//  Adds the new input locale with its default layout if the input locale
//  does not already exist in the list.  Calls setup to get the new
//  keyboard layout file (if necessary).
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_InstallKeyboardLayout(
    HWND hDlg,
    LCID Locale,
    LPTSTR pszLocale)
{
    HWND hwndIndicate;
    TCHAR szData[MAX_PATH];

    if (!Region_InstallDefaultLayout(Locale, pszLocale))
    {
        GetLocaleInfo(Locale, LOCALE_SLANGUAGE, szData, MAX_PATH);
        Region_ShowMsg( hDlg,
                        IDS_KBD_LOAD_KBD_FAILED,
                        0,
                        MB_OK_OOPS,
                        szData );
        return (FALSE);
    }

    //
    //  Update the active locales for the Input Locales property page
    //  if the page has been initialized.
    //
    PropSheet_QuerySiblings(GetParent(hDlg), 0, 0);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_GetHKL
//
////////////////////////////////////////////////////////////////////////////

HKL Region_GetHKL(
    DWORD dwLocale,
    DWORD dwLayout,
    LPTSTR pszLayout,
    HINF hIntlInf)
{
    TCHAR szData[MAX_PATH];
    INFCONTEXT Context;

    //
    //  Get the HKL based on the input locale value and the layout value.
    //
    if (dwLayout == 0)
    {
        //
        //  See if it's the default layout for the input locale or an IME.
        //
        if (HIWORD(dwLocale) == 0)
        {
            return ((HKL)MAKELONG(dwLocale, dwLocale));
        }
        else if ((HIWORD(dwLocale) & 0xf000) == 0xe000)
        {
            return ((HKL)dwLocale);
        }
    }
    else
    {
        //
        //  Use the layout to make the hkl.
        //
        if (HIWORD(dwLayout) != 0)
        {
            //
            //  We have a special id.  Need to find out what the layout id
            //  should be.
            //
            if ((SetupFindFirstLine(hIntlInf, szKbdLayoutIds, pszLayout, &Context)) &&
                (SetupGetStringField(&Context, 1, szData, MAX_PATH, NULL)))
            {
                dwLayout = (DWORD)(LOWORD(TransNum(szData)) | 0xf000);
            }
        }

        //
        //  Return the hkl:
        //      loword = input locale id
        //      hiword = layout id
        //
        return ((HKL)MAKELONG(dwLocale, dwLayout));
    }

    //
    //  Return failure.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ValidateRegistryPreload
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_ValidateRegistryPreload()
{
    HKEY hKeyPreload, hKeySubst;
    HINF hIntlInf;
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    TCHAR szData2[MAX_PATH];
    DWORD cbData, cbData2;
    DWORD dwData;
    DWORD dwNum, dwNumSet;
    LONG rc;

    //
    //  Open the HKCU\Keyboard Layout\Preload key.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      szKbdPreloadKey,
                      0,
                      KEY_ALL_ACCESS,
                      &hKeyPreload ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Open the HKCU\Keyboard Layout\Substitutes key.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      szKbdSubstKey,
                      0,
                      KEY_ALL_ACCESS,
                      &hKeySubst ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyPreload);
        return (FALSE);
    }

    //
    //  Open the Inf file.
    //
    hIntlInf = SetupOpenInfFile(szIntlInf, NULL, INF_STYLE_WIN4, NULL);
    if (hIntlInf == INVALID_HANDLE_VALUE)
    {
        RegCloseKey(hKeyPreload);
        RegCloseKey(hKeySubst);
        return (FALSE);
    }
    if (!SetupOpenAppendInfFile(NULL, hIntlInf, NULL))
    {
        SetupCloseInfFile(hIntlInf);
        RegCloseKey(hKeyPreload);
        RegCloseKey(hKeySubst);
        return (FALSE);
    }

    //
    //  Go through the values in the Preload key.
    //
    dwNum = dwNumSet = 2;
    wsprintf(szValue, TEXT("%d"), dwNum);
    cbData = sizeof(szData);
    szData[0] = TEXT('\0');
    rc = RegQueryValueEx( hKeyPreload,
                          szValue,
                          NULL,
                          NULL,
                          (LPBYTE)szData,
                          &cbData );

    while (rc == ERROR_SUCCESS)
    {
        //
        //  Look to see if the input locale is installed and if the layout
        //  is valid.
        //
        dwData = TransNum(szData);
        cbData2 = sizeof(szData2);
        szData2[0] = TEXT('\0');
        if ((IsValidLocale((LCID)(LOWORD(dwData)), LCID_INSTALLED)) &&
            ((RegQueryValueEx( hKeySubst,
                               szData,
                               NULL,
                               NULL,
                               (LPBYTE)szData2,
                               &cbData2 ) != ERROR_SUCCESS) ||
             (IsValidLocale((LCID)(LOWORD(TransNum(szData2))), LCID_INSTALLED))))
        {
            //
            //  Make sure we didn't delete one before this.  If so, we
            //  need to add this one with the new preload value.
            //
            if (dwNum != dwNumSet)
            {
                RegDeleteValue(hKeyPreload, szValue);
                wsprintf(szValue, TEXT("%d"), dwNumSet);
                RegSetValueEx( hKeyPreload,
                               szValue,
                               0,
                               REG_SZ,
                               (LPBYTE)szData,
                               (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );
            }
            dwNumSet++;
        }
        else
        {
            if ((szData2[0] != 0) ||
                (RegQueryValueEx( hKeySubst,
                                  szData,
                                  NULL,
                                  NULL,
                                  (LPBYTE)szData2,
                                  &cbData2 ) == ERROR_SUCCESS))
            {
                UnloadKeyboardLayout(Region_GetHKL( dwData,
                                                    TransNum(szData2),
                                                    szData2,
                                                    hIntlInf ));
                RegDeleteValue(hKeySubst, szData);
                RegDeleteValue(hKeyPreload, szValue);
            }
            else
            {
                UnloadKeyboardLayout(Region_GetHKL(dwData, 0, NULL, hIntlInf));
                RegDeleteValue(hKeyPreload, szValue);
            }
        }

        //
        //  Query the next value.
        //
        dwNum++;
        wsprintf(szValue, TEXT("%d"), dwNum);
        cbData = sizeof(szData);
        szData[0] = TEXT('\0');
        rc = RegQueryValueEx( hKeyPreload,
                              szValue,
                              NULL,
                              NULL,
                              (LPBYTE)szData,
                              &cbData );
    }

    //
    //  Close the registry keys.
    //
    RegFlushKey(hKeyPreload);
    RegFlushKey(hKeySubst);
    RegCloseKey(hKeyPreload);
    RegCloseKey(hKeySubst);

    //
    //  Close the inf file.
    //
    SetupCloseInfFile(hIntlInf);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ShowMsg
//
////////////////////////////////////////////////////////////////////////////

int Region_ShowMsg(
    HWND hDlg,
    UINT iMsg,
    UINT iTitle,
    UINT iType,
    LPTSTR pString)
{
    TCHAR szTitle[MAX_PATH];
    TCHAR szMsg[MAX_PATH];
    TCHAR szErrMsg[MAX_PATH];
    LPTSTR pTitle = NULL;

    if (iTitle)
    {
        if (LoadString(hInstance, iTitle, szTitle, ARRAYSIZE(szTitle)))
        {
            pTitle = szTitle;
        }
    }

    if (pString)
    {
        if (LoadString(hInstance, iMsg, szMsg, ARRAYSIZE(szMsg)))
        {
            wsprintf(szErrMsg, szMsg, pString);
            return (MessageBox(hDlg, szErrMsg, pTitle, iType));
        }
    }
    else
    {
        if (LoadString(hInstance, iMsg, szErrMsg, ARRAYSIZE(szErrMsg)))
        {
            return (MessageBox(hDlg, szErrMsg, pTitle, iType));
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_InstallSystemLocale
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_InstallSystemLocale(
    LCID Locale)
{
    //
    //  Make sure the locale is valid and then call setup to install the
    //  requested locale.
    //
    if (IsValidLocale(Locale, LCID_INSTALLED))
    {
        if (!SetupChangeLocaleEx( HWND_DESKTOP,
                                  LOWORD(Locale),
                                  pSetupSourcePath,
                                  SP_INSTALL_FILES_QUIETLY,
                                  NULL,
                                  0 ))
        {
            //
            //  Update current SysLocale, so we can use it later.
            //
            SysLocaleID = LOWORD(Locale);

            //
            //  Return success.
            //
            return (TRUE);
        }
    }

    //
    //  Return failure.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_SetLocaleInfo
//
////////////////////////////////////////////////////////////////////////////

void Region_SetLocaleInfo(
    LCID Locale,
    LCTYPE LCType,
    LPTSTR lpIniStr)
{
    TCHAR pBuf[SIZE_128];


    if (GetLocaleInfo( Locale,
                       LCType | LOCALE_NOUSEROVERRIDE,
                       pBuf,
                       SIZE_128 ))
    {
        WriteProfileString(szIntl, lpIniStr, pBuf);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_InstallUserLocale
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_InstallUserLocale(
    LCID Locale)
{
    HKEY hKey = NULL;
    TCHAR szLCID[25];

    //
    //  Make sure the locale is valid.
    //
    if (!IsValidLocale(Locale, LCID_INSTALLED))
    {
        return (FALSE);
    }

    //
    //  Save the locale id as a string.
    //
    wsprintf(szLCID, TEXT("%08x"), Locale);

    //
    //  Set the locale value in the user's control panel international
    //  section of the registry.
    //
    if ((RegOpenKeyEx( HKEY_CURRENT_USER,
                       c_szCPanelIntl,
                       0L,
                       KEY_READ | KEY_WRITE,
                       &hKey ) != ERROR_SUCCESS) ||
        (RegSetValueEx( hKey,
                        TEXT("Locale"),
                        0L,
                        REG_SZ,
                        (LPBYTE)szLCID,
                        (lstrlen(szLCID) + 1) * sizeof(TCHAR) ) != ERROR_SUCCESS))
    {
        if (hKey != NULL)
        {
            RegCloseKey(hKey);
        }
        return (FALSE);
    }

    //
    //  When the locale changes, update ALL registry information.
    //
    Region_SetLocaleInfo(Locale, LOCALE_SABBREVLANGNAME,    TEXT("sLanguage"));
    Region_SetLocaleInfo(Locale, LOCALE_SCOUNTRY,           TEXT("sCountry"));
    Region_SetLocaleInfo(Locale, LOCALE_ICOUNTRY,           TEXT("iCountry"));
    Region_SetLocaleInfo(Locale, LOCALE_S1159,              TEXT("s1159"));
    Region_SetLocaleInfo(Locale, LOCALE_S2359,              TEXT("s2359"));
    Region_SetLocaleInfo(Locale, LOCALE_STIMEFORMAT,        TEXT("sTimeFormat"));
    Region_SetLocaleInfo(Locale, LOCALE_STIME,              TEXT("sTime"));
    Region_SetLocaleInfo(Locale, LOCALE_ITIME,              TEXT("iTime"));
    Region_SetLocaleInfo(Locale, LOCALE_ITLZERO,            TEXT("iTLZero"));
    Region_SetLocaleInfo(Locale, LOCALE_ITIMEMARKPOSN,      TEXT("iTimePrefix"));
    Region_SetLocaleInfo(Locale, LOCALE_SSHORTDATE,         TEXT("sShortDate"));
    Region_SetLocaleInfo(Locale, LOCALE_IDATE,              TEXT("iDate"));
    Region_SetLocaleInfo(Locale, LOCALE_SDATE,              TEXT("sDate"));
    Region_SetLocaleInfo(Locale, LOCALE_SLONGDATE,          TEXT("sLongDate"));
    Region_SetLocaleInfo(Locale, LOCALE_SCURRENCY,          TEXT("sCurrency"));
    Region_SetLocaleInfo(Locale, LOCALE_ICURRENCY,          TEXT("iCurrency"));
    Region_SetLocaleInfo(Locale, LOCALE_INEGCURR,           TEXT("iNegCurr"));
    Region_SetLocaleInfo(Locale, LOCALE_ICURRDIGITS,        TEXT("iCurrDigits"));
    Region_SetLocaleInfo(Locale, LOCALE_SDECIMAL,           TEXT("sDecimal"));
    Region_SetLocaleInfo(Locale, LOCALE_SMONDECIMALSEP,     TEXT("sMonDecimalSep"));
    Region_SetLocaleInfo(Locale, LOCALE_STHOUSAND,          TEXT("sThousand"));
    Region_SetLocaleInfo(Locale, LOCALE_SMONTHOUSANDSEP,    TEXT("sMonThousandSep"));
    Region_SetLocaleInfo(Locale, LOCALE_SLIST,              TEXT("sList"));
    Region_SetLocaleInfo(Locale, LOCALE_IDIGITS,            TEXT("iDigits"));
    Region_SetLocaleInfo(Locale, LOCALE_ILZERO,             TEXT("iLzero"));
    Region_SetLocaleInfo(Locale, LOCALE_INEGNUMBER,         TEXT("iNegNumber"));
    Region_SetLocaleInfo(Locale, LOCALE_SNATIVEDIGITS,      TEXT("sNativeDigits"));
    Region_SetLocaleInfo(Locale, LOCALE_IDIGITSUBSTITUTION, TEXT("NumShape"));
    Region_SetLocaleInfo(Locale, LOCALE_IMEASURE,           TEXT("iMeasure"));
    Region_SetLocaleInfo(Locale, LOCALE_ICALENDARTYPE,      TEXT("iCalendarType"));
    Region_SetLocaleInfo(Locale, LOCALE_IFIRSTDAYOFWEEK,    TEXT("iFirstDayOfWeek"));
    Region_SetLocaleInfo(Locale, LOCALE_IFIRSTWEEKOFYEAR,   TEXT("iFirstWeekOfYear"));
    Region_SetLocaleInfo(Locale, LOCALE_SGROUPING,          TEXT("sGrouping"));
    Region_SetLocaleInfo(Locale, LOCALE_SMONGROUPING,       TEXT("sMonGrouping"));
    Region_SetLocaleInfo(Locale, LOCALE_SPOSITIVESIGN,      TEXT("sPositiveSign"));
    Region_SetLocaleInfo(Locale, LOCALE_SNEGATIVESIGN,      TEXT("sNegativeSign"));

    //
    //  Set the user's default locale in the system so that any new
    //  process will use the new locale.
    //
    NtSetDefaultLocale(TRUE, Locale);

    //
    //  Flush the International key.
    //
    if (hKey != NULL)
    {
        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_IsValidLayout
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_IsValidLayout(
    DWORD dwLayout)
{
    HKEY hKey1, hKey2;
    TCHAR szLayout[MAX_PATH];

    //
    //  Get the layout id as a string.
    //
    wsprintf(szLayout, TEXT("%08x"), dwLayout);

    //
    //  Open the Keyboard Layouts key.
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, szLayoutPath, &hKey1) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Try to open the layout id key under the Keyboard Layouts key.
    //
    if (RegOpenKey(hKey1, szLayout, &hKey2) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey1);
        return (FALSE);
    }

    //
    //  Close the keys.
    //
    RegCloseKey(hKey1);
    RegCloseKey(hKey2);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_SetFEHotkey
//
////////////////////////////////////////////////////////////////////////////

void Region_SetFEHotkey(
    BOOL bThai)
{
    HKEY hKey;
    TCHAR szData[MAX_PATH];
    DWORD cbData;


    //
    //  If there is no hotkey switch, set it to Ctrl+Shift.  Otherwise, the
    //  user cannot switch to an IME without setting the value first.
    //

    szData[0] = TEXT('\0');
    if (RegOpenKey( HKEY_CURRENT_USER,
                    szKbdToggleKey,
                    &hKey ) == ERROR_SUCCESS)
    {
        cbData = sizeof(szData);
        RegQueryValueEx( hKey,
                         TEXT("Hotkey"),
                         NULL,
                         NULL,
                         (LPBYTE)szData,
                         &cbData );

        switch (szData[0])
        {
            case TEXT('1'):
            case TEXT('2'):
            {
                //
                //  Currently ALT/SHIFT or CTRL/SHIFT.  Do not change.
                //
                break;
            }
            case TEXT('3'):
            {
                //
                //  Default hotkey for FE locale switch.
                //
                szData[0] = bThai ? TEXT('4') : TEXT('1');
                szData[1] = TEXT('\0');
                RegSetValueEx( hKey,
                               TEXT("Hotkey"),
                               0,
                               REG_SZ,
                               (LPBYTE)szData,
                               (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );
                break;
            }
            case TEXT('4'):
            {
                //
                //  Currently Grave.  Change to 1 if not Thai.
                //
                if (!bThai)
                {
                    szData[0] = TEXT('1');
                    szData[1] = TEXT('\0');

                    RegSetValueEx( hKey,
                                   TEXT("Hotkey"),
                                   0,
                                   REG_SZ,
                                   (LPBYTE)szData,
                                   (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );
                }
                break;
            }
        }

        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }
    else if (RegCreateKey( HKEY_CURRENT_USER,
                           szKbdToggleKey,
                           &hKey ) == ERROR_SUCCESS)
    {
        //
        //  We don't have a Toggle key yet.  Create one and set the
        //  correct value.
        //
        szData[0] = bThai? TEXT('4'): TEXT('1');
        szData[1] = 0;
        RegSetValueEx( hKey,
                       TEXT("Hotkey"),
                       0,
                       REG_SZ,
                       (LPBYTE)szData,
                       (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_InstallInputLocales
//
////////////////////////////////////////////////////////////////////////////

void Region_InstallInputLocales(
    LPLAYOUTLIST pLayoutList,
    DWORD dwNum)
{
    HKEY hKeyPreload, hKeySubst;
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    TCHAR szData2[MAX_PATH];
    DWORD dwIndex, cchValue, cbData, cbData2;
    DWORD dwValue, dwData, dwData2, dwCtr, dwCtr2;
    DWORD dwPreloadNum = 0;
    BOOL bHasIME = FALSE;
    LONG rc;
    BOOL bThai = FALSE;

    //
    //  Open the HKCU\Keyboard Layout\Preload key.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      szKbdPreloadKey,
                      0,
                      KEY_ALL_ACCESS,
                      &hKeyPreload ) != ERROR_SUCCESS)
    {
        return;
    }

    //
    //  Open the HKCU\Keyboard Layout\Substitutes key.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      szKbdSubstKey,
                      0,
                      KEY_ALL_ACCESS,
                      &hKeySubst ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyPreload);
        return;
    }

    //
    //  Enumerate the values in the Preload key.
    //
    dwIndex = 0;
    cchValue = sizeof(szValue) / sizeof(TCHAR);
    cbData = sizeof(szData);
    rc = RegEnumValue( hKeyPreload,
                       dwIndex,
                       szValue,
                       &cchValue,
                       NULL,
                       NULL,
                       (LPBYTE)szData,
                       &cbData );

    while (rc == ERROR_SUCCESS)
    {
        //
        //  Save the preload number if it's higher than the highest one
        //  found so far.
        //
        dwValue = TransNum(szValue);
        if (dwValue > dwPreloadNum)
        {
            dwPreloadNum = dwValue;
        }

        //
        //  Save the preload data - input locale.
        //
        dwValue = TransNum(szData);

        if (PRIMARYLANGID(LOWORD(dwValue)) == LANG_THAI)
        {
            bThai = TRUE;
        }

        //
        //  See if there is a substitute value.
        //
        dwData = 0;
        cbData2 = sizeof(szData2);
        if (RegQueryValueEx( hKeySubst,
                             szData,
                             NULL,
                             NULL,
                             (LPBYTE)szData2,
                             &cbData2 ) == ERROR_SUCCESS)
        {
            dwData = TransNum(szData2);
        }

        //
        //  Go through each of the requested input locales and make sure
        //  they don't already exist.
        //
        for (dwCtr = 0; dwCtr < dwNum; dwCtr++)
        {
            if (LOWORD(pLayoutList[dwCtr].dwLocale) == LOWORD(dwValue))
            {
                if (dwData)
                {
                    if (pLayoutList[dwCtr].dwLayout == dwData)
                    {
                        pLayoutList[dwCtr].bLoaded = TRUE;
                    }
                }
                else if (pLayoutList[dwCtr].dwLayout == dwValue)
                {
                    pLayoutList[dwCtr].bLoaded = TRUE;
                }

                //
                //  Save the highest 0xd000 value for this input locale.
                //
                if (pLayoutList[dwCtr].bIME == FALSE)
                {
                    dwData2 = (DWORD)(HIWORD(dwValue));
                    if (((dwData2 & 0xf000) != 0xe000) &&
                        (pLayoutList[dwCtr].dwSubst <= dwData2))
                    {
                        if (dwData2 == 0)
                        {
                            pLayoutList[dwCtr].dwSubst = 0xd000;
                        }
                        else if ((dwData2 & 0xf000) == 0xd000)
                        {
                            pLayoutList[dwCtr].dwSubst = dwData2 + 1;
                        }
                    }
                }
            }
        }

        //
        //  Get the next enum value.
        //
        dwIndex++;
        cchValue = sizeof(szValue) / sizeof(TCHAR);
        szValue[0] = TEXT('\0');
        cbData = sizeof(szData);
        szData[0] = TEXT('\0');
        rc = RegEnumValue( hKeyPreload,
                           dwIndex,
                           szValue,
                           &cchValue,
                           NULL,
                           NULL,
                           (LPBYTE)szData,
                           &cbData );
    }

    //
    //  Increase the maximum preload value by one so that it represents the
    //  next available value to use.
    //
    dwPreloadNum++;

    //
    //  Go through the list of layouts and add them.
    //
    for (dwCtr = 0; dwCtr < dwNum; dwCtr++)
    {
        if ((pLayoutList[dwCtr].bLoaded == FALSE) &&
            (IsValidLocale(pLayoutList[dwCtr].dwLocale, LCID_INSTALLED)) &&
            (Region_IsValidLayout(pLayoutList[dwCtr].dwLayout)))
        {
            //
            //  Save the preload number as a string so that it can be
            //  written into the registry.
            //
            wsprintf(szValue, TEXT("%d"), dwPreloadNum);

            if (PRIMARYLANGID(LOWORD(pLayoutList[dwCtr].dwLocale)) == LANG_THAI)
            {
                bThai = TRUE;
            }

            //
            //  Save the locale id as a string so that it can be written
            //  into the registry.
            //
            if (pLayoutList[dwCtr].bIME == TRUE)
            {
                wsprintf(szData, TEXT("%08x"), pLayoutList[dwCtr].dwLayout);
                bHasIME = TRUE;
            }
            else
            {
                //
                //  Get the 0xd000 value, if necessary.
                //
                if (dwCtr != 0)
                {
                    dwCtr2 = dwCtr;
                    do
                    {
                        dwCtr2--;
                        if ((pLayoutList[dwCtr2].bLoaded == FALSE) &&
                            (pLayoutList[dwCtr].dwLocale ==
                             pLayoutList[dwCtr2].dwLocale) &&
                            (pLayoutList[dwCtr2].bIME == FALSE))
                        {
                            dwData2 = pLayoutList[dwCtr2].dwSubst;
                            if (dwData2 == 0)
                            {
                                pLayoutList[dwCtr].dwSubst = 0xd000;
                            }
                            else
                            {
                                pLayoutList[dwCtr].dwSubst = dwData2 + 1;
                            }
                            break;
                        }
                    } while (dwCtr2 != 0);
                }

                //
                //  Save the locale id as a string.
                //
                dwData2 = pLayoutList[dwCtr].dwLocale;
                dwData2 |= (DWORD)(pLayoutList[dwCtr].dwSubst << 16);
                wsprintf(szData, TEXT("%08x"), dwData2);
            }

            //
            //  Set the value in the Preload section of the registry.
            //
            RegSetValueEx( hKeyPreload,
                           szValue,
                           0,
                           REG_SZ,
                           (LPBYTE)szData,
                           (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

            //
            //  Increment the preload value.
            //
            dwPreloadNum++;

            //
            //  See if we need to add a substitute for this input locale.
            //
            if (((pLayoutList[dwCtr].dwLocale != pLayoutList[dwCtr].dwLayout) ||
                 (pLayoutList[dwCtr].dwSubst != 0)) &&
                (pLayoutList[dwCtr].bIME == FALSE))
            {
                wsprintf(szData2, TEXT("%08x"), pLayoutList[dwCtr].dwLayout);
                RegSetValueEx( hKeySubst,
                               szData,
                               0,
                               REG_SZ,
                               (LPBYTE)szData2,
                               (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );
            }

            //
            //  Make sure all of the changes are written to disk.
            //
            RegFlushKey(hKeySubst);
            RegFlushKey(hKeyPreload);

            //
            //  Load the keyboard layout.
            //  If it fails, there isn't much we can do at this point.
            //
            LoadKeyboardLayout(szData, KLF_SUBSTITUTE_OK | KLF_NOTELLSHELL);
        }
    }

    //
    //  If there is an IME and there is no hotkey switch, set it to
    //  Ctrl+Shift.  Otherwise, the user cannot switch to an IME without
    //  setting the value first.
    //

    if (bHasIME || (dwPreloadNum > 2))
    {
        Region_SetFEHotkey((PRIMARYLANGID(LANGIDFROMLCID(SysLocaleID)) == LANG_THAI) && bThai);
    }

    //
    //  Update the taskbar indicator.
    //
    Region_UpdateIndicator(dwPreloadNum > 2);

    //
    //  Close the registry keys.
    //
    RegCloseKey(hKeyPreload);
    RegCloseKey(hKeySubst);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_GetInputLocaleList
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_GetInputLocaleList(
    PINFCONTEXT pContext,
    DWORD dwStartField,
    LPDWORD pdwLocale,
    LPDWORD pdwLayout)
{
    DWORD dwNumFields, dwNumList, dwCtr, dwCtr2, dwCtr3;
    LPLAYOUTLIST pLayoutList;
    TCHAR szBuffer[MAX_PATH];
    LPTSTR pPos;

    //
    //  Get the number of items in the list.
    //
    dwNumFields = SetupGetFieldCount(pContext);
    if (dwNumFields < dwStartField)
    {
        return (FALSE);
    }
    dwNumList = dwNumFields - dwStartField + 1;

    //
    //  Create the array to store the list of input locales.
    //
    pLayoutList = (LPLAYOUTLIST)LocalAlloc( LPTR,
                                            sizeof(LAYOUTLIST) * dwNumList );
    if (pLayoutList == NULL)
    {
        return (FALSE);
    }

    //
    //  Save the input locale information in the array.
    //
    dwCtr2 = 0;
    for (dwCtr = dwStartField; dwCtr <= dwNumFields; dwCtr++)
    {
        if (SetupGetStringField( pContext,
                                 dwCtr,
                                 szBuffer,
                                 MAX_PATH,
                                 NULL ))
        {
            //
            //  Find the colon in order to save the input locale
            //  and layout values separately.
            //
            pPos = szBuffer;
            while (*pPos)
            {
                if ((*pPos == CHAR_COLON) && (pPos != szBuffer))
                {
                    *pPos = 0;
                    pPos++;

                    if ((pLayoutList[dwCtr2].dwLocale = TransNum(szBuffer)) &&
                        (pLayoutList[dwCtr2].dwLayout = TransNum(pPos)))
                    {
                        //
                        //  See if it's an IME.
                        //
                        if ((HIWORD(pLayoutList[dwCtr2].dwLayout) & 0xf000) == 0xe000)
                        {
                            pLayoutList[dwCtr2].bIME = TRUE;

                            if (LOWORD(pLayoutList[dwCtr2].dwLocale) !=
                                LOWORD(pLayoutList[dwCtr2].dwLayout))
                            {
                                break;
                            }
                        }

                        //
                        //  Make sure it's not a duplicate.
                        //
                        for (dwCtr3 = 0; dwCtr3 < dwCtr2; dwCtr3++)
                        {
                            if ((pLayoutList[dwCtr2].dwLocale ==
                                 pLayoutList[dwCtr3].dwLocale) &&
                                (pLayoutList[dwCtr2].dwLayout ==
                                 pLayoutList[dwCtr3].dwLayout))
                            {
                                break;
                            }
                        }

                        //
                        //  Only increment the count if it's not
                        //  a duplicate.
                        //
                        if (dwCtr3 == dwCtr2)
                        {
                            dwCtr2++;
                        }
                    }

                    break;
                }

                pPos++;
            }
        }
    }

    //
    //  Install the input locales.
    //
    if (dwCtr2 > 0)
    {
        Region_InstallInputLocales(pLayoutList, dwCtr2);
    }

    //
    //  Return the first layout - the default.
    //
    if (pdwLocale)
    {
        *pdwLocale = pLayoutList[0].dwLocale;
    }
    if (pdwLayout)
    {
        *pdwLayout = pLayoutList[0].dwLayout;
    }

    //
    //  Free the list of input locales.
    //
    LocalFree(pLayoutList);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_RemoveActiveImmKey
//
//  This is called during keyboard upgrade to remove the keyboard layout
//  entry if Active IMM has been installed.  Active IMM is only installed
//  on NT4 systems and isn't supported in NT5.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_RemoveActiveImmKey(
    PTSTR szLayoutKey,
    PTSTR szLayoutId)
{
    HKEY hLayoutKey;
    BOOL bDelete = FALSE;
    DWORD dwLayoutId = TransNum(szLayoutId);

    if ((dwLayoutId == 0x0404) ||
        (dwLayoutId == 0x0804) ||
        (dwLayoutId == 0x0411) ||
        (dwLayoutId == 0x0412))
    {
        if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          szLayoutKey,
                          0,
                          KEY_ALL_ACCESS,
                          &hLayoutKey ) == ERROR_SUCCESS)
        {
            if ((RegQueryValueEx( hLayoutKey,
                                  TEXT("AIMMFlags"),
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL ) == ERROR_SUCCESS))
            {
                bDelete = TRUE;
            }

            RegCloseKey(hLayoutKey);

            if (bDelete)
            {
                RegDeleteKey(HKEY_LOCAL_MACHINE, szLayoutKey);
            }
        }
    }

    return (bDelete);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_FileExists
//
//  Determines if a file exists or not
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_FileExists(
    PTSTR szFile)
{
    HANDLE hFile;
    WIN32_FIND_DATA FindFileData;

    hFile = FindFirstFile(szFile, &FindFileData);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }
    FindClose(hFile);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_UpgradeKeyboardLayoutList
//
//  Installs the necessary language groups to enable the installed
//  keyboard layouts.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_UpgradeKeyboardLayoutList(
    HINF *phIntlInf,
    HSPFILEQ *pFileQueue,
    PVOID *pQueueContext)
{
    INFCONTEXT LineContext;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    TCHAR szSystemDirectory[MAX_PATH];
    TCHAR szKeyboardLayoutFilePath[MAX_PATH];
    PTSTR pszKbdLayoutFileName, pszFileName;
    HKEY hKeyLayouts, hKeyKeyboardLayout;
    DWORD dwIndex, dwData, dwMaxLayouts, dwCtr, cb;
    UINT InstallLanguageGroups[MAX_UI_LANG_GROUPS];
    BOOL IsKbdFileInstalled;
    UINT SysDirLen, Len;
    LONG rc;

    //
    //  Let's install language groups for any installed keyboard layout,
    //  in case we're doing an upgrade.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      szLayoutPath,
                      0,
                      KEY_ALL_ACCESS,
                      &hKeyLayouts ) == ERROR_SUCCESS)
    {
        IsKbdFileInstalled = FALSE;
        dwMaxLayouts = 0;
        dwIndex = 0;
        szValue[0] = TEXT('\0');
        szData[0] = TEXT('\0');

        rc = RegEnumKey( hKeyLayouts,
                         dwIndex,
                         szData,
                         sizeof(szData) / sizeof(TCHAR) );

        SysDirLen = GetSystemDirectory( szSystemDirectory,
                                        (sizeof(szSystemDirectory) / sizeof(TCHAR)) );
        if (SysDirLen != 0)
        {
            if (SysDirLen > ((sizeof(szSystemDirectory) / sizeof(TCHAR))-1))
            {
                SysDirLen = 0;
            }
            else
            {
                if (szSystemDirectory[SysDirLen - 1] != TEXT('\\'))
                {
                    szSystemDirectory[SysDirLen] = TEXT('\\');
                    SysDirLen++;
                    szSystemDirectory[SysDirLen] = TEXT('\0');
                }

                //
                //  Establish pointers to reduce strcpy and strcat calls.
                //
                pszFileName = &szSystemDirectory[SysDirLen];

                lstrcpy(szKeyboardLayoutFilePath, szLayoutPath);
                lstrcat(szKeyboardLayoutFilePath, TEXT("\\"));
                Len = lstrlen(szKeyboardLayoutFilePath);
                pszKbdLayoutFileName = &szKeyboardLayoutFilePath[Len];
            }
        }

        while ((rc == ERROR_SUCCESS) && (SysDirLen != 0))
        {
            //
            //  Validate the layout.
            //
            dwData = TransNum(szData);

            if (IsValidLocale((LCID)(LOWORD(dwData)), LCID_SUPPORTED))
            {
                *pszKbdLayoutFileName = TEXT('\0');
                lstrcpy(pszKbdLayoutFileName, szData);

                //
                //  Make sure the kbd files are on the disk.
                //
                if (RegOpenKey( HKEY_LOCAL_MACHINE,
                                szKeyboardLayoutFilePath,
                                &hKeyKeyboardLayout ) == ERROR_SUCCESS)
                {
                    cb = sizeof(szValue);
                    if ((RegQueryValueEx( hKeyKeyboardLayout,
                                          szLayoutFile,
                                          NULL,
                                          NULL,
                                          (LPBYTE) szValue,
                                          &cb ) == ERROR_SUCCESS))
                    {
                        Len = lstrlen(szValue);

                        if ((Len + SysDirLen) <
                            (sizeof(szSystemDirectory) / sizeof(TCHAR)))
                        {
                            *pszFileName = TEXT('\0');

                            lstrcpy(pszFileName, szValue);

                            if (Region_FileExists(szSystemDirectory))
                            {
                                IsKbdFileInstalled = TRUE;
                            }
                        }
                    }
                    RegCloseKey(hKeyKeyboardLayout);
                }

                //
                //  Get its language group to install.
                //
                if (IsKbdFileInstalled)
                {
                    szValue[0] = TEXT('\0');

                    //
                    //  ActiveIMM is not supported in W2K, so delete its
                    //  keys if present and don't install corresponding
                    //  language group.
                    //
                    if (!Region_RemoveActiveImmKey( szKeyboardLayoutFilePath,
                                                    szData ))
                    {
                        if (SetupFindFirstLine( *phIntlInf,
                                                L"Locales",
                                                szData,
                                                &LineContext ) &&
                            SetupGetStringField( &LineContext,
                                                 3,
                                                 szValue,
                                                 sizeof(szValue) / sizeof(TCHAR),
                                                 NULL ))
                        {
                            UINT LanguageGroup = StrToLong(szValue);

                            //
                            //  Make sure it doesn't need a Lang directory
                            //  because setup will explode if it does.
                            //  It's ok to ignore the ones that need a Lang
                            //  directory because they should already be
                            //  installed at this point on an upgrade anyway.
                            //  If they're not already installed, then the
                            //  user is still no worse off than they were
                            //  before the upgrade.
                            //
                            //  This is only needed because US Terminal
                            //  Server NT 4 installs the Chinese input locale
                            //  with the US layout (for some strange reason).
                            //
                            szBuffer[0] = 0;
                            if (SetupFindFirstLine( *phIntlInf,
                                                    L"LanguageGroups",
                                                    szValue,
                                                    &LineContext ))
                            {
                                if (SetupGetStringField( &LineContext,
                                                         2,
                                                         szBuffer,
                                                         MAX_PATH,
                                                         NULL ))
                                {
                                    szBuffer[4] = TEXT('\0');
                                }
                            }
                            if (lstrcmp(szBuffer, TEXT("lang")) != 0)
                            {
                                //
                                //  Add it to the list, if not already
                                //  added.
                                //
                                for (dwCtr = 0; dwCtr < dwMaxLayouts; dwCtr++)
                                {
                                    if (InstallLanguageGroups[dwCtr] == LanguageGroup)
                                    {
                                        break;
                                    }
                                }

                                if (dwCtr == dwMaxLayouts)
                                {
                                    if (dwMaxLayouts <
                                        (sizeof(InstallLanguageGroups) / sizeof(LGRPID)))
                                    {
                                        InstallLanguageGroups[dwMaxLayouts++] = LanguageGroup;
                                    }
                                }
                            }
                        }
                    }

                    IsKbdFileInstalled = FALSE;
                }
            }

            //
            //  Get the next enum value.
            //
            dwIndex++;
            szValue[0] = TEXT('\0');
            szData[0] = TEXT('\0');
            rc = RegEnumKey( hKeyLayouts,
                             dwIndex,
                             szData,
                             sizeof(szData) / sizeof(TCHAR));
        }

        //
        //  Close the registry key handle.
        //
        RegCloseKey(hKeyLayouts);

        //
        //  Fast verification loop.
        //
        for (dwCtr = 0; dwCtr < dwMaxLayouts; dwCtr++)
        {
            //
            //  Remove from the list if already installed or not supported.
            //
            if (!IsValidLanguageGroup( InstallLanguageGroups[dwCtr],
                                       LGRPID_SUPPORTED ))
            {
                InstallLanguageGroups[dwCtr] = LGRPID_WESTERN_EUROPE;
            }
            else if (IsValidLanguageGroup( InstallLanguageGroups[dwCtr],
                                           LGRPID_INSTALLED ))
            {
                InstallLanguageGroups[dwCtr] = LGRPID_WESTERN_EUROPE;
            }
        }

        //
        //  Let's try to install any needed language group.
        //
        for (dwCtr = 0; dwCtr < dwMaxLayouts; dwCtr++)
        {
            //
            //  Western Europe is always installed by syssetup.
            //
            if (InstallLanguageGroups[dwCtr] != LGRPID_WESTERN_EUROPE)
            {
                //
                //  Enqueue the language group files so that they may be
                //  copied.  This only handles the CopyFiles entries in the
                //  inf file.
                //
                wsprintf( szBuffer,
                          TEXT("%ws%d"),
                          szLGInstallPrefix,
                          InstallLanguageGroups[dwCtr] );

                SetupInstallFilesFromInfSection( *phIntlInf,
                                                 NULL,
                                                 *pFileQueue,
                                                 szBuffer,
                                                 pSetupSourcePath,
                                                 SP_COPY_NEWER );
            }
        }

        //
        //  See if we need to install any files.
        //
        if ((SetupScanFileQueue( *pFileQueue,
                                 SPQ_SCAN_PRUNE_COPY_QUEUE |
                                   SPQ_SCAN_FILE_VALIDITY,
                                 HWND_DESKTOP,
                                 NULL,
                                 NULL,
                                 &dwCtr )) && (dwCtr != 1))
        {
            //
            //  Copy the files in the queue.
            //
            if (!SetupCommitFileQueue( HWND_DESKTOP,
                                       *pFileQueue,
                                       SetupDefaultQueueCallback,
                                       *pQueueContext ))
            {
                //
                //  This can happen if the user hits Cancel from
                //  within the setup dialog.
                //
                return (FALSE);
            }
        }

        //
        //  Call setup to install other inf info for the various
        //  language groups.
        //
        for (dwCtr = 0; dwCtr < dwMaxLayouts; dwCtr++)
        {
            if (InstallLanguageGroups[dwCtr] != LGRPID_WESTERN_EUROPE)
            {
                wsprintf( szBuffer,
                          TEXT("%ws%d"),
                          szLGInstallPrefix,
                          InstallLanguageGroups[dwCtr] );

                SetupInstallFromInfSection( HWND_DESKTOP,
                                            *phIntlInf,
                                            szBuffer,
                                            SPINST_ALL & ~SPINST_FILES,
                                            NULL,
                                            pSetupSourcePath,
                                            0,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL );
            }
        }

        //
        //  Run any necessary apps (for IME installation).
        //
        Region_RunRegApps(c_szIntlRun);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_UpdateShortDate
//
//  Updates the user's short date setting to contain a 4-digit year.
//  The setting is only updated if it is the same as the default setting
//  for the current locale (except for the 2-digit vs. 4-digit year).
//
////////////////////////////////////////////////////////////////////////////

void Region_UpdateShortDate()
{
    TCHAR szBufCur[SIZE_64];
    TCHAR szBufDef[SIZE_64];
    LPTSTR pCur, pDef;
    BOOL bChange = FALSE;

    //
    //  Get the current short date format setting and the default short date
    //  format setting.
    //
    if ((GetLocaleInfo( LOCALE_USER_DEFAULT,
                        LOCALE_SSHORTDATE,
                        szBufCur,
                        SIZE_64 )) &&
        (GetLocaleInfo( LOCALE_USER_DEFAULT,
                        LOCALE_SSHORTDATE | LOCALE_NOUSEROVERRIDE,
                        szBufDef,
                        SIZE_64 )))
    {
        //
        //  See if the current setting and the default setting only differ
        //  in a 2-digit year ("yy") vs. a 4-digit year ("yyyy").
        //
        //  Note: For this, we want an Exact match, so we don't need to
        //        use CompareString to compare the formats.
        //
        pCur = szBufCur;
        pDef = szBufDef;
        while ((*pCur) && (*pCur == *pDef))
        {
            //
            //  See if it's a 'y'.
            //
            if (*pCur == CHAR_SML_Y)
            {
                if (((*(pCur + 1)) == CHAR_SML_Y) &&
                    ((*(pDef + 1)) == CHAR_SML_Y) &&
                    ((*(pDef + 2)) == CHAR_SML_Y) &&
                    ((*(pDef + 3)) == CHAR_SML_Y))
                {
                    bChange = TRUE;
                    pCur += 1;
                    pDef += 3;
                }
            }
            pCur++;
            pDef++;
        }

        //
        //  Set the default short date format as the user's setting.
        //
        if (bChange && (*pCur == *pDef))
        {
            SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, szBufDef);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_WinntUpgrade
//
//  Determines whether NT-Setup is doing an upgrade or a fresh install.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_WinntUpgrade(
    PCTSTR pUnattendFile)
{
    TCHAR Buffer[MAX_PATH];
    BOOL bRet = FALSE;


    Buffer[0] = TEXT('\0');
    GetPrivateProfileString( TEXT("data"),
                             TEXT("winntupgrade"),
                             TEXT("no"),
                             Buffer,
                             sizeof(Buffer) / sizeof(TCHAR),
                             pUnattendFile );

    if (!lstrcmpi(Buffer, TEXT("yes")))
    {
        bRet = TRUE;
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_DoUnattendModeSetup
//
//  NOTE: The unattend mode file contains strings rather than integer
//        values, so we must get the string field and then convert it
//        to the appropriate integer format.  The Setup APIs won't just
//        do the right thing, so we have to roll our own.
//
////////////////////////////////////////////////////////////////////////////

void Region_DoUnattendModeSetup(
    LPCTSTR pUnattendFile,
    BOOL AllowProgressBar)
{
    HINF hFile, hIntlInf;
    HSPFILEQ FileQueue;
    PVOID QueueContext;
    INFCONTEXT Context;
    DWORD dwNum, dwCtr, dwLocale, dwLayout;
    UINT LanguageGroup, Language, SystemLocale, UserLocale;
    TCHAR szBuffer[MAX_PATH];
    BOOL bWinntUpgrade;
    BOOL bFound = FALSE;
    BOOL bLangGroup = FALSE;


    //
    //  Open the unattend mode file.
    //
    hFile = SetupOpenInfFile(pUnattendFile, NULL, INF_STYLE_OLDNT, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    //
    //  Don't display the FileCopy progress bar unless the user
    //  specifies it wants it.
    //
    if (!AllowProgressBar)
    {
        g_bSetupCase = TRUE;
    }
    else
    {
        g_bSetupCase = FALSE;
    }

    //
    //  Open the intl.inf file.
    //
    if (!Region_InitInf(0, &hIntlInf, szIntlInf, &FileQueue, &QueueContext))
    {
        SetupCloseInfFile(hFile);
        return;
    }

    //
    //  Now that the INF has been initialized, set this to TRUE to show
    //  that we're in setup mode.
    //
    g_bSetupCase = TRUE;

    //
    //  Check if we're doing an upgrade or fresh install.
    //
    bWinntUpgrade = Region_WinntUpgrade(pUnattendFile);

    //
    //  Install all requested Language Groups.
    //
    if ((SetupFindFirstLine( hFile,
                             szRegionalSettings,
                             szLanguageGroup,
                             &Context )) &&
        (dwNum = SetupGetFieldCount(&Context)))
    {
        bLangGroup = TRUE;

        for (dwCtr = 1; dwCtr <= dwNum; dwCtr++)
        {
            if (SetupGetStringField(&Context, dwCtr, szBuffer, MAX_PATH, NULL))
            {
                //
                //  Get the Language Group as an integer.
                //
                LanguageGroup = StrToLong(szBuffer);

                //
                //  Enqueue the language group files so that they may be
                //  copied.  This only handles the CopyFiles entries in the
                //  inf file.
                //
                wsprintf( szBuffer,
                          TEXT("%ws%d"),
                          szLGInstallPrefix,
                          LanguageGroup );

                SetupInstallFilesFromInfSection( hIntlInf,
                                                 NULL,
                                                 FileQueue,
                                                 szBuffer,
                                                 pSetupSourcePath,
                                                 SP_COPY_NEWER );
            }
        }

        //
        //  See if we need to install any files.
        //
        if ((SetupScanFileQueue( FileQueue,
                                 SPQ_SCAN_PRUNE_COPY_QUEUE |
                                   SPQ_SCAN_FILE_VALIDITY,
                                 HWND_DESKTOP,
                                 NULL,
                                 NULL,
                                 &dwCtr )) && (dwCtr != 1))
        {
            //
            //  Copy the files in the queue.
            //
            if (!SetupCommitFileQueue( HWND_DESKTOP,
                                       FileQueue,
                                       SetupDefaultQueueCallback,
                                       QueueContext ))
            {
                //
                //  This can happen if the user hits Cancel from
                //  within the setup dialog.
                //
                goto Region_UnattendModeExit;
            }
        }

        //
        //  Call setup to install other inf info for the various
        //  language groups.
        //
        for (dwCtr = 1; dwCtr <= dwNum; dwCtr++)
        {
            if (SetupGetStringField(&Context, dwCtr, szBuffer, MAX_PATH, NULL))
            {
                LanguageGroup = StrToLong(szBuffer);

                wsprintf( szBuffer,
                          TEXT("%ws%d"),
                          szLGInstallPrefix,
                          LanguageGroup );

                SetupInstallFromInfSection( HWND_DESKTOP,
                                            hIntlInf,
                                            szBuffer,
                                            SPINST_ALL & ~SPINST_FILES,
                                            NULL,
                                            pSetupSourcePath,
                                            0,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL );
            }
        }

        //
        //  Run any necessary apps (for IME installation).
        //
        Region_RunRegApps(c_szIntlRun);
    }

    //
    //  Add language groups for the user's keyboard layouts.
    //
    if (bWinntUpgrade)
    {
        Region_UpgradeKeyboardLayoutList( &hIntlInf,
                                          &FileQueue,
                                          &QueueContext);
    }

    //
    //  Install the requested Language/Region information.  If a
    //  Language/Region was not specified, then install the requested
    //  System Locale, User Locale, and Input Locales.
    //
    if ((SetupFindFirstLine( hFile,
                             szRegionalSettings,
                             szLanguage,
                             &Context )) &&
        (SetupGetStringField(&Context, 1, szBuffer, MAX_PATH, NULL)))
    {
        //
        //  Get the Language as an integer.
        //
        Language = TransNum(szBuffer);

        //
        //  Install the Language as the System Locale and the User Locale,
        //  and then install all layouts associated with the Language.
        //
        if (Region_InstallSystemLocale(Language))
        {
            bFound = TRUE;
        }

        //
        //  If we're doing an upgrade, then don't touch per-user settings.
        //
        if (!bWinntUpgrade)
        {
            if (Region_InstallUserLocale(Language))
            {
                bFound = TRUE;
            }

            if (Region_InstallAllLayouts( hIntlInf,
                                          (DWORD)(LOWORD(Language)),
                                          NULL ))
            {
                bFound = TRUE;

                if (IsValidLocale(Language, LCID_INSTALLED))
                {
                    Region_SetPreloadLocale(Language, hIntlInf, 0);
                }
            }
        }
    }

    //
    //  Make sure there was a valid Language setting.  If not, then look
    //  for the individual keywords.
    //
    if (!bFound)
    {
        //
        //  Init the locale variables.
        //
        SystemLocale = 0;
        UserLocale = 0;

        //
        //  Install the requested System Locale.
        //
        if ((SetupFindFirstLine( hFile,
                                 szRegionalSettings,
                                 szSystemLocale,
                                 &Context )) &&
            (SetupGetStringField(&Context, 1, szBuffer, MAX_PATH, NULL)))
        {
            SystemLocale = TransNum(szBuffer);
            if (Region_InstallSystemLocale(SystemLocale))
            {
                bFound = TRUE;
            }
        }

        //
        //  Install the requested User Locale.
        //
        if ((SetupFindFirstLine( hFile,
                                 szRegionalSettings,
                                 szUserLocale,
                                 &Context )) &&
            (SetupGetStringField(&Context, 1, szBuffer, MAX_PATH, NULL)))
        {
            UserLocale = TransNum(szBuffer);
            if ((!bWinntUpgrade) && (Region_InstallUserLocale(UserLocale)))
            {
                bFound = TRUE;
            }
        }

        //
        //  Install the requested Input Locales.
        //
        if (SetupFindFirstLine( hFile,
                                szRegionalSettings,
                                szInputLocale,
                                &Context ))
        {
            if (Region_GetInputLocaleList(&Context, 1, &dwLocale, &dwLayout))
            {
                bFound = TRUE;
                Region_SetPreloadLocale(dwLocale, hIntlInf, dwLayout);
            }
        }
        else
        {
            //
            //  No input locales are specified, so install the default
            //  input locale for the system locale and/or user locale if
            //  they were specified.
            //
            if (SystemLocale != 0)
            {
                Region_InstallDefaultLayout((DWORD)(LOWORD(SystemLocale)), NULL);
            }
            if ((UserLocale != 0) && (UserLocale != SystemLocale))
            {
                Region_InstallDefaultLayout((DWORD)(LOWORD(UserLocale)), NULL);
            }
        }
    }

    //
    //  If we still didn't find anything, then load the default locale for
    //  the installation.  It will be the equivalent of:
    //      LanguageGroup = "x"
    //      Language = "y"
    //  where x is the language group for the default locale and y is the
    //  default locale.
    //
    if (!bFound && !bLangGroup)
    {
        //
        //  Get the default locale.
        //
        if ((SetupFindFirstLine( hIntlInf,
                                 L"DefaultValues",
                                 L"Locale",
                                 &Context )) &&
            (SetupGetStringField(&Context, 1, szBuffer, MAX_PATH, NULL)))
        {
            //
            //  Get the Language as an integer.
            //
            Language = TransNum(szBuffer);

            //
            //  Install the Language Group needed for this Language.
            //
            if ((SetupFindFirstLine( hIntlInf,
                                     L"Locales",
                                     szBuffer,
                                     &Context )) &&
                (SetupGetStringField(&Context, 3, szBuffer, MAX_PATH, NULL)))
            {
                //
                //  Get the Language Group as an integer.
                //
                LanguageGroup = StrToLong(szBuffer);

                //
                //  Enqueue the language group files so that they may be
                //  copied.  This only handles the CopyFiles entries in the
                //  inf file.
                //
                wsprintf( szBuffer,
                          TEXT("%ws%d"),
                          szLGInstallPrefix,
                          LanguageGroup );

                SetupInstallFilesFromInfSection( hIntlInf,
                                                 NULL,
                                                 FileQueue,
                                                 szBuffer,
                                                 pSetupSourcePath,
                                                 SP_COPY_NEWER );
                //
                //  See if we need to install any files.
                //
                if ((SetupScanFileQueue( FileQueue,
                                         SPQ_SCAN_PRUNE_COPY_QUEUE |
                                           SPQ_SCAN_FILE_VALIDITY,
                                         HWND_DESKTOP,
                                         NULL,
                                         NULL,
                                         &dwCtr )) && (dwCtr != 1))
                {
                    //
                    //  Copy the files in the queue.
                    //
                    if (!SetupCommitFileQueue( HWND_DESKTOP,
                                               FileQueue,
                                               SetupDefaultQueueCallback,
                                               QueueContext ))
                    {
                        //
                        //  This can happen if the user hits Cancel from
                        //  within the setup dialog.
                        //
                        goto Region_UnattendModeExit;
                    }
                }

                //
                //  Call setup to install other inf info for the language
                //  group.
                //
                SetupInstallFromInfSection( HWND_DESKTOP,
                                            hIntlInf,
                                            szBuffer,
                                            SPINST_ALL & ~SPINST_FILES,
                                            NULL,
                                            pSetupSourcePath,
                                            0,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL );

                //
                //  Run any necessary apps (for IME installation).
                //
                Region_RunRegApps(c_szIntlRun);
            }

            //
            //  Install the Language as the System Locale and the User Locale,
            //  and then install all layouts associated with the Language.
            //
            Region_InstallSystemLocale(Language);

            //
            //  If we're doing an upgrade, then don't touch per-user settings.
            //
            if (!bWinntUpgrade)
            {
                Region_InstallUserLocale(Language);

                if (Region_InstallAllLayouts(hIntlInf, (DWORD)(LOWORD(Language)), NULL))
                {
                    if (IsValidLocale(Language, LCID_INSTALLED))
                    {
                        Region_SetPreloadLocale(Language, hIntlInf, 0);
                    }
                }
            }
        }
    }

    //
    //  Run any necessary apps (for FSVGA/FSNEC installation).
    //
    Region_RunRegApps(c_szSysocmgr);

Region_UnattendModeExit:
    //
    //  Close the inf file.
    //
    Region_CloseInf(hIntlInf, FileQueue, QueueContext);

    //
    //  Close the unattend mode file.
    //
    SetupCloseInfFile(hFile);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_OpenInfFile
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_OpenIntlInfFile(HINF *phInf)
{
    HINF hIntlInf;

    //
    //  Open the intl.inf file.
    //
    hIntlInf = SetupOpenInfFile(szIntlInf, NULL, INF_STYLE_WIN4, NULL);
    if (hIntlInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }
    if (!SetupOpenAppendInfFile(NULL, hIntlInf, NULL))
    {
        SetupCloseInfFile(hIntlInf);
        return (FALSE);
    }

    *phInf = hIntlInf;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  RegionCloseInfFile
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_CloseInfFile(HINF *phInf)
{
    SetupCloseInfFile(*phInf);
    *phInf = INVALID_HANDLE_VALUE;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ReadDefaultLayoutFromInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_ReadDefaultLayoutFromInf(
    LPTSTR pszLocale,
    LPDWORD pdwLocale,
    LPDWORD pdwLayout,
    LPDWORD pdwLocale2,
    LPDWORD pdwLayout2,
    HINF hIntlInf)
{
    INFCONTEXT Context;
    TCHAR szPair[MAX_PATH * 2];
    LPTSTR pPos;
    DWORD dwLangIn = LANGIDFROMLCID(TransNum(pszLocale));
    int iField;

    //
    //  Get the first (default) LANGID:HKL pair for the given locale.
    //    Example String: "0409:00000409"
    //
    szPair[0] = 0;
    if (SetupFindFirstLine( hIntlInf,
                            TEXT("Locales"),
                            pszLocale,
                            &Context ))
    {
        SetupGetStringField(&Context, 5, szPair, MAX_PATH, NULL);
    }

    //
    //  Make sure we have a string.
    //
    if (szPair[0] == 0)
    {
        return (FALSE);
    }

    //
    //  Find the colon in the string and then set the position
    //  pointer to the next character.
    //
    pPos = szPair;
    while (*pPos)
    {
        if ((*pPos == CHAR_COLON) && (pPos != szPair))
        {
            *pPos = 0;
            pPos++;
            break;
        }
        pPos++;
    }

    if (pdwLayout2)
        *pdwLayout2 = 0;
    if (pdwLocale2)
        *pdwLocale2 = 0;

    //
    //  If there is a layout, then return the input locale and the layout.
    //
    if ((*pPos) &&
        (*pdwLocale = TransNum(szPair)) &&
        (*pdwLayout = TransNum(pPos)))
    {
        if ((!pdwLocale2) ||
            (!pdwLayout2) ||
            (dwLangIn == LANGIDFROMLCID(*pdwLocale)))
        {
            return (TRUE);
        }

        //
        //  If we get here, the language has a default layout that has a
        //  different locale than the language (e.g. Thai).  We want the
        //  default locale to be English (so that logon can occur with a US
        //  keyboard), but the first Thai keyboard layout should be installed
        //  when the Thai locale is chosen.  This is why we have two locales
        //  and layouts passed back to the caller.
        //
        iField = 6;
        while (SetupGetStringField(&Context, iField, szPair, MAX_PATH, NULL))
        {
            DWORD dwLoc, dwLay;

            //
            //  Make sure we have a string.
            //
            if (szPair[0] == 0)
            {
                iField++;
                continue;
            }

            //
            //  Find the colon in the string and then set the position
            //  pointer to the next character.
            //
            pPos = szPair;

            while (*pPos)
            {
                if ((*pPos == CHAR_COLON) && (pPos != szPair))
                {
                    *pPos = 0;
                    pPos++;
                    break;
                }
                pPos++;
            }

            if (*pPos == 0)
            {
                iField++;
                continue;
            }

            dwLoc = TransNum(szPair);
            dwLay = TransNum(pPos);
            if ((dwLoc == 0) || (dwLay == 0))
            {
                iField++;
                continue;
            }
            if (LANGIDFROMLCID(dwLoc) == dwLangIn)
            {
                *pdwLayout2 = dwLay;
                *pdwLocale2 = dwLoc;
                return (TRUE);
            }
            iField++;
        }

        //
        //  If we get here, then no matching locale could be found.
        //  This should not happen, but do the right thing and
        //  only pass back the default layout if it does.
        //
        return (TRUE);
    }

    //
    //  Return failure.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_GetDefaultLayoutFromInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_GetDefaultLayoutFromInf(
    LPTSTR pszLocale,
    LPDWORD pdwLocale,
    LPDWORD pdwLayout,
    LPDWORD pdwLocale2,
    LPDWORD pdwLayout2)
{
    BOOL bRet = TRUE;
    HINF hIntlInf;

    if (Region_OpenIntlInfFile(&hIntlInf))
    {
        bRet = Region_ReadDefaultLayoutFromInf( pszLocale,
                                                pdwLocale,
                                                pdwLayout,
                                                pdwLocale2,
                                                pdwLayout2,
                                                hIntlInf );
        Region_CloseInfFile(&hIntlInf);
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_InstallDefaultLayout
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_InstallDefaultLayout(
    LCID Locale,
    LPTSTR pszLocale)
{
    TCHAR szLCID[25];
    DWORD dwLocale;
    DWORD dwLocale2;
    DWORD dwLayout;
    DWORD dwLayout2;
    LAYOUTLIST LayoutList;

    //
    //  Save the locale id as a string.
    //
    if (pszLocale == NULL)
    {
        wsprintf(szLCID, TEXT("%08x"), Locale);
        pszLocale = szLCID;
    }

    //
    //  Get the given locale's default layout from the inf file.
    //
    if (Region_GetDefaultLayoutFromInf( pszLocale,
                                        &dwLocale,
                                        &dwLayout,
                                        &dwLocale2,
                                        &dwLayout2 ) == FALSE)
    {
        //
        //  Try just the language id.
        //
        if (HIWORD(Locale) != 0)
        {
            wsprintf(szLCID, TEXT("%08x"), LANGIDFROMLCID(Locale));
            if (Region_GetDefaultLayoutFromInf( szLCID,
                                                &dwLocale,
                                                &dwLayout,
                                                &dwLocale2,
                                                &dwLayout2 ) == FALSE)
            {
                return (FALSE);
            }
        }
        else
        {
            return (FALSE);
        }
    }

    //
    //  Save the information in the LayoutList structure.
    //
    LayoutList.dwLocale = dwLocale;
    LayoutList.dwLayout = dwLayout;
    LayoutList.dwSubst = 0;
    LayoutList.bLoaded = FALSE;
    LayoutList.bIME = ((HIWORD(dwLayout) & 0xf000) == 0xe000) ? TRUE : FALSE;

    //
    //  Install the input locale / keyboard layout pair if it's not already
    //  installed.
    //
    Region_InstallInputLocales(&LayoutList, 1);

    if (dwLayout2)
    {
        //
        //  The first layout was for a different locale, so also install
        //  the next layout.
        //
        LayoutList.dwLocale = dwLocale2;
        LayoutList.dwLayout = dwLayout2;
        LayoutList.dwSubst = 0;
        LayoutList.bLoaded = FALSE;
        LayoutList.bIME = ((HIWORD(dwLayout2) & 0xf000) == 0xe000) ? TRUE : FALSE;
        Region_InstallInputLocales(&LayoutList, 1);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//   Region_ActivateDefaultKeyboardLayout
//
//   Sets the default input layout on the system, and broadcast to all
//   running apps about this change.
//
////////////////////////////////////////////////////////////////////////////

HKL Region_ActivateDefaultKeyboardLayout(
    DWORD dwLocale,
    DWORD dwLayout,
    LPTSTR pszLayout,
    HINF hIntlInf)
{
    HKL hkl;

    hkl = Region_GetHKL( dwLocale,
                         dwLayout,
                         pszLayout,
                         hIntlInf );
    if (hkl)
    {
        if (SystemParametersInfo( SPI_SETDEFAULTINPUTLANG,
                                  0,
                                  (LPVOID)((LPDWORD) &hkl),
                                  0 ))
        {
            DWORD dwRecipients = BSM_APPLICATIONS | BSM_ALLDESKTOPS;
            BroadcastSystemMessage( BSF_POSTMESSAGE,
                                    &dwRecipients,
                                    WM_INPUTLANGCHANGEREQUEST,
                                    1,
                                    (LPARAM) hkl );
        }
    }

    return (hkl);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_SetPreloadLocale
//
//  Change the preload sort order to ensure that either the default
//  keyboard or given keyboard for the locale is preloaded first.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_SetPreloadLocale(
    LCID Locale,
    HINF hIntlInf,
    DWORD dwLayout)
{
    HKEY hKeyPreload,hKeySubst;
    TCHAR szValue[MAX_PATH];
    TCHAR szValue0[MAX_PATH];
    TCHAR szData[MAX_PATH];
    TCHAR szData2[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    DWORD dwIndex, cchValue, cbData, dwLayout2, dwLocale;
    DWORD dwLayoutNew, dwData;
    LONG rc;

    //
    //  Get the default Layout for the Locale.
    //
    if (dwLayout == 0)
    {
        wsprintf(szValue, TEXT("%08x"), Locale);
        if (Region_GetDefaultLayoutFromInf( szValue,
                                            &dwLocale,
                                            &dwLayout,
                                            NULL,
                                            NULL ) == FALSE)
        {
            return (FALSE);
        }
    }
    else
    {
        dwLocale = (DWORD)(LOWORD(Locale));
    }

    //
    //  Open the HKCU\Keyboard Layout\Preload key.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      szKbdPreloadKey,
                      0,
                      KEY_ALL_ACCESS,
                      &hKeyPreload ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Open the HKCU\Keyboard Layout\Substitutes key.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      szKbdSubstKey,
                      0,
                      KEY_ALL_ACCESS,
                      &hKeySubst ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyPreload);
        return (FALSE);
    }

    //
    //  Enumerate the values in the Preload key.
    //
    dwIndex = 0;
    cchValue = sizeof(szValue) / sizeof(TCHAR);
    cbData = sizeof(szData2);
    rc = RegEnumValue( hKeyPreload,
                       dwIndex,
                       szValue,
                       &cchValue,
                       NULL,
                       NULL,
                       (LPBYTE)szData2,
                       &cbData );

    if (rc != ERROR_SUCCESS)
    {
        //
        //  We should never get here.  This means that there were no preload
        //  records at all.  Time to bail.
        //
        RegCloseKey(hKeyPreload);
        RegCloseKey(hKeySubst);
        return (FALSE);
    }

    wcscpy(szValue0, szValue);
    dwLayout2 = TransNum(szData2);

    //
    //  See if there is a substitute value.
    //
    dwData = 0;
    cbData = sizeof(szData);
    if (RegQueryValueEx( hKeySubst,
                         szData2,
                         NULL,
                         NULL,
                         (LPBYTE)szData,
                         &cbData ) == ERROR_SUCCESS)
    {
        dwData = TransNum(szData);
    }

    //
    //  See if dwLayout matches the layout or substitute value.
    //
    if (((dwData) && (dwData == dwLayout) && (LOWORD(dwLayout2) == dwLocale)) ||
        ((!dwData) && (dwLayout2 == dwLayout) && (LOWORD(dwLayout2) == dwLocale)))
    {
        //
        //  No need to do anything.  The new language is the first preload.
        //  All is happy.  Cleanup and depart.
        //
        Region_ActivateDefaultKeyboardLayout( dwLayout2,
                                              dwData,
                                              szData,
                                              hIntlInf );
        RegCloseKey(hKeyPreload);
        RegCloseKey(hKeySubst);
        return (TRUE);
    }

    szTemp[0] = TEXT('\0');

    //
    //  Start by putting the new default dwLayout into the first preload
    //  record.
    //
    dwLayoutNew = dwLayout;
    while (rc == ERROR_SUCCESS)
    {
        wsprintf(szData, TEXT("%08x"), dwLayoutNew);
        dwLayout2 = TransNum(szData2);

        RegSetValueEx( hKeyPreload,
                       szValue,
                       0,
                       REG_SZ,
                       (LPBYTE)szData,
                       (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

        //
        //  See if dwLayout matches the layout or substitute value.
        //
        if (((dwData) && (dwData == dwLayout) && (LOWORD(dwLayout2) == dwLocale)) ||
            ((!dwData) && (dwLayout2 == dwLayout) && (LOWORD(dwLayout2) == dwLocale)))
        {
            //
            //  We are done.  We have moved the new language up to the front
            //  of the list.  Time to clean up and leave.  First, though, we
            //  need to update the first entry, since the HIWORD may contain
            //  IME codes and the like.
            //
            wsprintf(szData, TEXT("%08x"), dwLayout2);
            RegSetValueEx( hKeyPreload,
                           szValue0,
                           0,
                           REG_SZ,
                           (LPBYTE)szData,
                           (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );

            Region_ActivateDefaultKeyboardLayout( dwLayout2,
                                                  dwData,
                                                  szTemp,
                                                  hIntlInf );
            RegFlushKey(hKeyPreload);
            RegCloseKey(hKeyPreload);
            RegCloseKey(hKeySubst);
            return (TRUE);
        }

        //
        //  Preserve the value from this record to stuff into the next
        //  record.
        //
        dwLayoutNew = dwLayout2;

        //
        //  Get the next enum value.
        //
        dwIndex++;
        cchValue = sizeof(szValue) / sizeof(TCHAR);
        szValue[0] = TEXT('\0');
        cbData = sizeof(szData2);
        szData2[0] = TEXT('\0');
        rc = RegEnumValue( hKeyPreload,
                           dwIndex,
                           szValue,
                           &cchValue,
                           NULL,
                           NULL,
                           (LPBYTE)szData2,
                           &cbData );

        //
        //  See if there is a substitute value.
        //
        dwData = 0;
        cbData = sizeof(szData);
        szTemp[0] = TEXT('\0');
        if (RegQueryValueEx( hKeySubst,
                             szData2,
                             NULL,
                             NULL,
                             (LPBYTE)szData,
                             &cbData ) == ERROR_SUCCESS)
        {
            lstrcpy(szTemp, szData);
            dwData = TransNum(szData);
        }

    }

    //
    //  We should never get here.  This means that the new default layout
    //  was not in the preload list at all.
    //
    RegFlushKey(hKeyPreload);
    RegCloseKey(hKeyPreload);
    RegCloseKey(hKeySubst);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_InstallAllLayouts
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_InstallAllLayouts(
    HINF hIntlInf,
    LCID Locale,
    LPTSTR pszLocale)
{
    TCHAR szLCID[25];
    INFCONTEXT Context;

    //
    //  Save the locale id as a string.
    //
    if (pszLocale == NULL)
    {
        wsprintf(szLCID, TEXT("%08x"), Locale);
        pszLocale = szLCID;
    }

    //
    //  Get the line in the inf file that contains the list of default
    //  input locales for the given locale.
    //
    if (SetupFindFirstLine( hIntlInf,
                            TEXT("Locales"),
                            pszLocale,
                            &Context ))
    {
        //
        //  Install the list of input locale / keyboard layout pairs for
        //  the given locale.
        //
        return (Region_GetInputLocaleList(&Context, 5, NULL, NULL));
    }

    //
    //  Return failure.
    //
    return (FALSE);
}
