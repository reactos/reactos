// RunOnceCheckBox.cpp : Implementation of CRunOnceCheckBox
#include "stdafx.h"
#include "RunOnce.h"
#include "RunOnceCB.h"

#include <regstr.h>

/////////////////////////////////////////////////////////////////////////////
// CRunOnceCheckBox

char const g_szRegTips[]     = REGSTR_PATH_EXPLORER "\\Tips";
char const g_szWelcomeShow[] = "ShowIE4";

STDMETHODIMP CRunOnceCheckBox::get_ShowIE4State (BOOL *pVal)
{
    HKEY hkey;
    DWORD dwTemp, dwSize = sizeof(DWORD);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegTips, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {
        RegQueryValueEx(hkey, g_szWelcomeShow, 0, &dwTemp, (LPBYTE)pVal, &dwSize);
        RegCloseKey(hkey);
    }

    return(S_OK);

}   /* end  CRunOnceCheckBox::get_ShowIE4State() */


STDMETHODIMP CRunOnceCheckBox::put_ShowIE4State (BOOL newVal)
{
    HKEY hkey;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegTips, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
        RegSetValueEx(hkey, g_szWelcomeShow, 0, REG_DWORD, (CONST LPBYTE)&newVal, (DWORD)sizeof(DWORD));
        RegCloseKey(hkey);
    }

    return(S_OK);

}   /* end  CRunOnceCheckBox::put_ShowIE4State() */
