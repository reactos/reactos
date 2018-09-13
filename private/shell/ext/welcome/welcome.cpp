#include "resource.h"
#include "welcome.h"
#include "rcids.h"
#include "docobj.h"
#include <exdisp.h>
#include <htiframe.h>

#include <shdguid.h>
#include <shlguid.h>
#include <inetreg.h>
#include <regstr.h>
#include <windows.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <mshtml.h>

const VARIANT c_vaEmpty = {0};
//
// BUGBUG: Remove this ugly const to non-const casting if we can
//  figure out how to put const in IDL files.
//
#define PVAREMPTY ((VARIANT*)&c_vaEmpty)

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#define USA             0x0409
#define CANADA          0x1009

#define SHOWIE4         "ShowIE4"
#define SHOWIE4PLUS     "ShowIE4Plus"
#define DOES_NOT_EXIST  0xBAADF00D

#define STR_BSTR    0
#define STR_OLESTR  1
#define BSTRFROMANSI(x) (BSTR)MakeWideStrFromAnsi((LPSTR)(x), STR_BSTR)
#define TO_ASCII(x) (char)((unsigned char)x + 0x30)

#define REGSTR_PATH_ADVANCEDLIST REGSTR_PATH_IEXPLORER TEXT("\\AdvancedOptions\\BROWSE")

char const g_szRegTips[]        = REGSTR_PATH_EXPLORER      "\\Tips";
char const g_szRegAdvWelcome[]  = REGSTR_PATH_ADVANCEDLIST  "\\WELCOME";

//  local functions
//
int  WinMainT (HINSTANCE, HINSTANCE, LPTSTR, int);
BOOL LaunchIE4Instance (LPTSTR);
BOOL ParseCommandLine(LPCTSTR);
DWORD GetShowIE4State (void);
void SetShowIE4State (BOOL);
BOOL GetShowIE4PlusString (LPSTR, int);
void LaunchIE4PlusString (LPSTR);
void InitAdvOptRegKey (void);


EXTERN_C int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPTSTR pszCmdLine = GetCommandLine();

    // We don't want the "No disk in drive X:" requesters, so we set
    // the critical error mask such that calls will just silently fail

    SetErrorMode(SEM_FAILCRITICALERRORS);

    if (*pszCmdLine == TEXT('\"')) {                                  
        while (*++pszCmdLine && (*pszCmdLine != TEXT('\"')));
        if (*pszCmdLine == TEXT('\"'))
            pszCmdLine++;
    } else
        while (*pszCmdLine > TEXT(' ')) pszCmdLine++;

    // Skip past any white space preceeding the second token.

    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) pszCmdLine++;

    si.dwFlags = 0;
    GetStartupInfoA(&si);
    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine, si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    ExitThread(i);  // We only come here when we are not the shell...
    return(i);
}


int WinMainT(HINSTANCE hinst, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
    if (SUCCEEDED(CoInitialize(NULL))) 
    {
        BOOL bLocale;
        LCID localeID;
        TCHAR szResourceURL[MAX_PATH];
        DWORD dwShowIE4 = GetShowIE4State();
        BOOL bBrowserOnly = (WhichPlatform() != PLATFORM_INTEGRATED);

        // loadwc in browser only happens to pass us /f every single time
        BOOL bFirstTime = (dwShowIE4 == DOES_NOT_EXIST);
        if (!bBrowserOnly)
            bFirstTime |= ParseCommandLine(lpszCmdLine);

        // we don't want a /f to do this code path...just a 'true' first time.
        if (dwShowIE4 == DOES_NOT_EXIST) 
        {
            InitAdvOptRegKey();
            SetShowIE4State(TRUE);
        }

        localeID = GetUserDefaultLCID();
        bLocale = ((localeID == USA) || (localeID == CANADA));

        if (GetShowIE4PlusString(szResourceURL, ARRAYSIZE(szResourceURL))) 
        {
            LaunchIE4PlusString(szResourceURL);
        }
        else if (dwShowIE4) 
        {
            LoadString(hinst, IDS_RESOURCE_URL, szResourceURL, ARRAYSIZE(szResourceURL));
            wsprintf(szResourceURL, "%s#FirstTime=%c#Contest=%c#MinimalTour=%c", szResourceURL, TO_ASCII(bFirstTime), TO_ASCII(bLocale), TO_ASCII(bBrowserOnly));
            LaunchIE4Instance(szResourceURL);
        }

        CoUninitialize();
    }

    ExitProcess(0);
    return(0);

}   /*  end WinMainT() */


//=--------------------------------------------------------------------------=
// MakeWideFromAnsi
//=--------------------------------------------------------------------------=
// given a string, make a BSTR out of it.
//
// Parameters:
//    LPSTR         - [in]
//    BYTE          - [in]
//
// Output:
//    LPWSTR        - needs to be cast to final desired result
//
// Notes:
//
LPWSTR MakeWideStrFromAnsi (LPSTR psz, BYTE bType)
{
    int i;
    LPWSTR pwsz;

    if (!psz)
        return(NULL);
    
    if ((i = MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0)) <= 0)    // compute the length of the required BSTR
        return NULL;                                                                                            

    switch (bType) {                                                    // allocate the widestr, +1 for null
        case STR_BSTR:                                                                                                
            pwsz = (LPWSTR)SysAllocStringLen(NULL, (i - 1));            // SysAllocStringLen adds 1
            break;
        case STR_OLESTR:
            pwsz = (LPWSTR)CoTaskMemAlloc(i * sizeof(WCHAR));
            break;
        default:
            return(NULL);
    }

    if (!pwsz)
        return(NULL);

    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;

    return(pwsz);

}   /*  MakeWideStrFromAnsi() */


BOOL LaunchIE4Instance (LPTSTR szResourceURL)
{
    IWebBrowser2 *pwb;
    HRESULT hres = CoCreateInstance(CLSID_InternetExplorer, NULL,
                                    CLSCTX_LOCAL_SERVER, IID_IWebBrowser2, (void **)&pwb);
    if (SUCCEEDED(hres)) 
    {
        int dx, dy;
        DWORD dwFlags;
        //
        //  this marks this window as a third party window, 
        //  so that the window is not reused.
        //
        pwb->put_RegisterAsBrowser(VARIANT_TRUE);

        // turn off scrolling & resizing
        ITargetFrame2* ptgf;
        if (SUCCEEDED(pwb->QueryInterface(IID_ITargetFrame2, (void **) &ptgf))) 
        {
            if (SUCCEEDED(ptgf->GetFrameOptions(&dwFlags))) {
                dwFlags &= ~(FRAMEOPTIONS_SCROLL_YES | FRAMEOPTIONS_SCROLL_NO | FRAMEOPTIONS_SCROLL_AUTO);
                dwFlags |= FRAMEOPTIONS_SCROLL_NO;
                ptgf->SetFrameOptions(dwFlags);
            }
            ptgf->Release();
        }

        IServiceProvider *psp;
        if (SUCCEEDED(pwb->QueryInterface(IID_IServiceProvider, (void**) &psp))) 
        {
            IHTMLWindow2 *phw;
            if (SUCCEEDED(psp->QueryService(IID_IHTMLWindow2, IID_IHTMLWindow2, (void**)&phw))) 
            {
                VARIANT var;
                var.vt = VT_BOOL;
                var.boolVal = 666;
                phw->put_opener(var);
                phw->Release();
            } 
            psp->Release();
        }

        // turn off chrome
        pwb->put_MenuBar(FALSE);
        pwb->put_StatusBar(FALSE);
        pwb->put_ToolBar(FALSE);
        pwb->put_AddressBar(FALSE);
        pwb->put_Resizable(FALSE);


        // set client area size
        int iWidth = 466L;
        int iHeight = 286L;

        pwb->ClientToWindow(&iWidth, &iHeight);

        if (iWidth > 0)
            pwb->put_Width(iWidth);
        
        if (iHeight > 0)
            pwb->put_Height(iHeight);

        if ((dx = ((GetSystemMetrics(SM_CXSCREEN) - iWidth) / 2)) > 0)     // center the on screen window 
            pwb->put_Left(dx);

        if ((dy = ((GetSystemMetrics(SM_CYSCREEN) - iHeight) / 2)) > 0)
            pwb->put_Top(dy);    

        pwb->put_Visible(TRUE);

//        CreateParmsFile();

        BSTR bstr = BSTRFROMANSI(szResourceURL);
        HRESULT hr = pwb->Navigate(bstr, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY);

        SysFreeString(bstr);
        pwb->Release();

        return TRUE;
    }
    return FALSE;
}


BOOL ParseCommandLine(LPCTSTR pszCmdLine)
{
    return (lstrcmpi(pszCmdLine, "-f") == 0) || (lstrcmpi(pszCmdLine, "/f") == 0);
}


DWORD GetShowIE4State (void)
{
    HKEY hkey;
    DWORD dwShow = DOES_NOT_EXIST, dwTemp, dwSize = sizeof(DWORD);
    if (RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegTips, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) 
    {
        RegQueryValueEx(hkey, SHOWIE4, 0, &dwTemp, (LPBYTE)&dwShow, &dwSize);
        RegCloseKey(hkey);
    }
    return dwShow;
}


void SetShowIE4State (BOOL value)
{
    HKEY hkey;
    DWORD dwDisp;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, g_szRegTips, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwDisp) == ERROR_SUCCESS) {
        RegSetValueEx(hkey, SHOWIE4, 0, REG_DWORD, (CONST LPBYTE)&value, (DWORD)sizeof(value));
        RegCloseKey(hkey);
    }

}   /*  SetBrowserOnlyRegKey() */


BOOL GetShowIE4PlusString(LPSTR pszString, int cStringSize)
{
    HKEY hkey;
    DWORD dwTemp, dwSize = cStringSize;
    BOOL fResult = FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegTips, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hkey, SHOWIE4PLUS, 0, &dwTemp, (LPBYTE)pszString, &dwSize) == ERROR_SUCCESS) {
            fResult = TRUE;

            // IE4 Plus!'s initial experience is a one-time thing so we should
            // delete the value and make sure we never detect it again.
            RegDeleteValue(hkey, SHOWIE4PLUS);
        }
        RegCloseKey(hkey);
    }

    return(fResult);
}


// borrowed from shdocvw\util.cpp
LPTSTR _PathGetArgs(LPCTSTR pszPath)
{
    BOOL fInQuotes = FALSE;

    if (!pszPath)
        return NULL;

    while (*pszPath)
    {
        if (*pszPath == TEXT('"'))
            fInQuotes = !fInQuotes;
        else if (!fInQuotes && *pszPath == TEXT(' '))
            return (LPTSTR)pszPath+1;
        pszPath = CharNext(pszPath);
    }

    return (LPTSTR)pszPath;
}

void LaunchIE4PlusString (LPSTR pszCmdLine)
{
    LPTSTR pszArgs = NULL;

    pszArgs = _PathGetArgs(pszCmdLine);
    if(pszArgs)
        *(pszArgs - 1) = TEXT('\0');   // clobber the ' '
    ShellExecute(NULL, NULL, pszCmdLine, pszArgs, NULL, SW_SHOWNORMAL );
}


#define HELP_STRING "iexplore.hlp#00000"
#define CHECK_BOX   "checkbox"
#define VALUE_NAME  SHOWIE4

void InitAdvOptRegKey (void)
{
    HKEY hkey;
    DWORD dwDisp, value;
    char szListText[MAX_PATH];

    LoadString(GetModuleHandle(NULL), IDS_LIST_TEXT, (LPTSTR)&szListText, MAX_PATH);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, g_szRegAdvWelcome, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwDisp) == ERROR_SUCCESS) {
        RegSetValueEx(hkey, "CheckedValue",   0, REG_DWORD, (CONST LPBYTE)&(value = TRUE),       (DWORD)sizeof(value));
        RegSetValueEx(hkey, "DefaultValue",   0, REG_DWORD, (CONST LPBYTE)&(value = TRUE),       (DWORD)sizeof(value));
        RegSetValueEx(hkey, "HelpID",         0, REG_SZ,    (CONST LPBYTE)HELP_STRING,           (DWORD)sizeof(HELP_STRING));
        RegSetValueEx(hkey, "HKeyRoot",       0, REG_DWORD, (CONST LPBYTE)&(value = 0x80000001), (DWORD)sizeof(value));
        RegSetValueEx(hkey, "RegPath",        0, REG_SZ,    (CONST LPBYTE)g_szRegTips,           (DWORD)sizeof(g_szRegTips));
        RegSetValueEx(hkey, "Text",           0, REG_SZ,    (CONST LPBYTE)szListText,            (DWORD)(strlen(szListText) + 1));
        RegSetValueEx(hkey, "Type",           0, REG_SZ,    (CONST LPBYTE)CHECK_BOX,             (DWORD)sizeof(CHECK_BOX));
        RegSetValueEx(hkey, "UncheckedValue", 0, REG_DWORD, (CONST LPBYTE)&(value = FALSE),      (DWORD)sizeof(value));
        RegSetValueEx(hkey, "ValueName",      0, REG_SZ,    (CONST LPBYTE)VALUE_NAME,            (DWORD)sizeof(VALUE_NAME));
        RegCloseKey(hkey);
    }

}   /*  end InitAdvOptRegKey() */
