///////////////////////////////////////////////////////////////////////
//                     Microsoft Windows                             //
//              Copyright(c) Microsoft Corp., 1995                   //
///////////////////////////////////////////////////////////////////////
//
// LANG.CPP - "Language" property page for InetCpl
//

// HISTORY:
//
// 1/10/97  beomoh      created
//

#include "inetcplp.h"

#include <tchar.h>
#include <mlang.h>
#include "psapi.h"
#include "tlhelp32.h"
#include "process.h"
#include <mluisupp.h>
#include <shdocvw.h>

#define ARRAYSIZE(a)        (sizeof(a)/sizeof(a[0]))
#define FORMAT_STR          TEXT("%s [%s]")
#define MAX_LIST_STRING_LEN MAX_LOCALE_NAME + MAX_RFC1766_NAME + 3
#define MAX_ACCEPT_LANG_LEN 2048

// used as the return value from setlang dialog
#define RETURN_SETLANG_ENDLANGDIALOG    2
#define RETURN_SETLANG_CLOSEDNORMAL     1
#define RETURN_SETLANG_CANCELED         0

typedef HRESULT (* PCOINIT) (LPVOID);
typedef VOID (* PCOUNINIT) (VOID);
typedef VOID (* PCOMEMFREE) (LPVOID);
typedef HRESULT (* PCOCREINST) (REFCLSID, LPUNKNOWN, DWORD,     REFIID, LPVOID * );

extern HMODULE hOLE32;
extern PCOINIT pCoInitialize;
extern PCOUNINIT pCoUninitialize;
extern PCOMEMFREE pCoTaskMemFree;
extern PCOCREINST pCoCreateInstance;

extern BOOL _StartOLE32();

class CUILangList;
INT_PTR KickSetLang(HWND hDlg, CUILangList * pLangList);

static const TCHAR s_szResourceLocale[] = TEXT("ResourceLocale");
// HKLM\Software\Microsoft\Internet Explorer\International used for url string
static const TCHAR s_szUrlSPK[] 
= TEXT("http://www.microsoft.com/isapi/redir.dll?prd=ie&pver=5&ar=plugui&sba=install");
static const TCHAR c_szInstall[] 
= TEXT("Software\\Microsoft\\Active Setup\\Installed Components\\{89820200-ECBD-11CF-8B85-00AA005B4383}");
static const TCHAR c_szLocale[] = TEXT("Locale");
static const TCHAR s_szLangPackPath[]   = TEXT("Software\\Microsoft\\Internet Explorer");
static const TCHAR s_szVersion[] = TEXT("LPKInstalled");

typedef struct 
{
    WORD wlangid;
    BOOL fValid;
    TCHAR szName[MAX_LOCALE_NAME];
} LANGLIST;

static LANGLIST s_arryLangList[] = 
{
    {0x0409, FALSE, {0}},
    {0x0407, FALSE, {0}},
    {0x0411, FALSE, {0}},
    {0x0412, FALSE, {0}},
    {0x0404, FALSE, {0}},
    {0x0804, FALSE, {0}},
    {0x040c, FALSE, {0}},
    {0x0c0a, FALSE, {0}},
    {0x0416, FALSE, {0}},
    {0x0410, FALSE, {0}},
    {0x0413, FALSE, {0}},
    {0x041d, FALSE, {0}},
    {0x0406, FALSE, {0}},
    {0x040b, FALSE, {0}},
    {0x040e, FALSE, {0}},
    {0x0414, FALSE, {0}},
    {0x0408, FALSE, {0}},
    {0x0415, FALSE, {0}},
    {0x0419, FALSE, {0}},
    {0x0405, FALSE, {0}},
    {0x0816, FALSE, {0}},
    {0x041f, FALSE, {0}},
    {0x041b, FALSE, {0}},
    {0x0424, FALSE, {0}},
    {0x0401, FALSE, {0}},
    {0x040d, FALSE, {0}},
};

//
//  ISO639 ID table
//
typedef struct tagISO639
{
    LPCTSTR ISO639;
    LANGID LangID;
}   ISO639, *LPISO639;

const ISO639 c_ISO639[] =
{
    { TEXT("EN"), 0x0409 },
    { TEXT("DE"), 0x0407 },
    { TEXT("JA"), 0x0411 },
    { TEXT("KO"), 0x0412 },
    { TEXT("TW"), 0x0404 },
    { TEXT("CN"), 0x0804 },
    { TEXT("FR"), 0x040C },
    { TEXT("ES"), 0x0C0A },
    { TEXT("BR"), 0x0416 },
    { TEXT("IT"), 0x0410 },
    { TEXT("NL"), 0x0413 },
    { TEXT("SV"), 0x041D },
    { TEXT("DA"), 0x0406 },
    { TEXT("FI"), 0x040B },
    { TEXT("HU"), 0x040E },
    { TEXT("NO"), 0x0414 },
    { TEXT("EL"), 0x0408 },
    { TEXT("PL"), 0x0415 },
    { TEXT("RU"), 0x0419 },
    { TEXT("CS"), 0x0405 },
    { TEXT("PT"), 0x0816 },
    { TEXT("TR"), 0x041F },
    { TEXT("SK"), 0x041B },
    { TEXT("SL"), 0x0424 },
    { TEXT("AR"), 0x0401 },
    { TEXT("HE"), 0x040D }
};

// GetInstallLanguage
//
// synopsis - borrowed this function from shlwapi. we can remove this
//            once we have it exported from shlwapi.dll
//
LANGID GetInstallLanguage(void)
{
    static LANGID LangID = 0;
    TCHAR szISO639[3];
    DWORD cb;

    if (0 == LangID)
    {
        cb = sizeof(szISO639);
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, c_szInstall, c_szLocale, NULL, szISO639, &cb))
        {
            int i;

            for (i = 0; i < ARRAYSIZE(c_ISO639); i++)
            {
                if (!StrCmpNI(szISO639, c_ISO639[i].ISO639, ARRAYSIZE(szISO639)))
                {
                    LangID = c_ISO639[i].LangID;
                    break;
                }
            }
        }
    }
    return LangID;
}

// CUILangList
// 
// maintains the list of UI languages for user to choose
//
class CUILangList
{
public:
    CUILangList() {_iLangIdx = -1; lang = s_arryLangList; 
                   _nLangList = ARRAYSIZE(s_arryLangList);
                   _fOffice9Installed = -1;};
    
    void    ValidateLangList();
    BOOL    IsValidLang(int idx) { return (idx < _nLangList) ? lang[idx].fValid: FALSE; };
    int     GetCurrentLangIdx();
    void    SetCurrentLangIdx(int idx);
    LPCTSTR GetCurrentLangName();
    LPCTSTR GetLangNameOfIdx(int idx);
    WORD    GetLangIdOfIdx(int idx) { return (idx < _nLangList) ? lang[idx].wlangid:0; };
    UINT    GetIds(int idx); 
    int     GetListSize() {return _nLangList;};
    BOOL    IsOffice9Installed();
    static  HRESULT GetLangList(HWND hdlg, CUILangList ** ppLangList);
    static  HRESULT RemoveLangList(HWND hdlg);
private:
    int _iLangIdx;
    int _nLangList;
    int _fOffice9Installed;
    LANGLIST *lang;
};

// CShutDownProcInfo
// 
// manages information about processes we want
// to shutdown/restart.
//
typedef enum 
{
    PS_UNKNOWN=0, 
    PS_CANDIDATE, 
    PS_TO_BE_SHUTDOWN, 
    PS_IGNORE, 
    PS_SHUTDOWN_OK, 
    PS_WAITING, 
    PS_TO_BE_SHUTDOWN_WITH_NO_RELAUNCH, 
    PS_SHUTDOWN_OK_NO_RELAUNCH_NEEDED, 
} PROCSTATE; 

class CShutDownProcInfo : public CProcessInfo
{
public:
    CShutDownProcInfo(HWND hdlgParent);
    ~CShutDownProcInfo();
    HRESULT EnsureProcList();
    HRESULT IncreaseProcList();
    HRESULT NotifyShutDownToFolks(int *nProccess);
    HRESULT AddToProcList(HWND hwndShutDown);
    HRESULT WaitForOneProcess(int iProc);
    HRESULT WaitForFolksShutDown();
    HRESULT GetRestartAppPath(LPTSTR szPath, int cchPath, int iProc);
    HRESULT RestartFolks();
    static DWORD CALLBACK ShutDownThreadProc(void *pv);
protected:
    typedef struct
    {
        DWORD dwPID;
        TCHAR szExeName[32];
        PROCSTATE State; 
    } PROCLIST;
    PROCLIST *_pProcList;
    int _nAlloced;
    int _iProcList;
    HWND _hdlgParent;
    BOOL _fAllShutDown;
};
// this always fills '0' to empty digits
// caller has to make sure sz has cdigit+1 of buffer
void IntToHex(OUT LPTSTR sz, IN int cdigit, IN int value)
{
    int i, idigit;

    if (sz && value > 0 && cdigit > 0)
    {
        // nul terminate the buffer
        sz[cdigit] = TEXT('\0');
        
        for (i = cdigit-1; i >= 0; i--, value /= 16)
        {
            idigit = value%16;
            if (idigit < 10)
                sz[i] = (TCHAR)idigit + TEXT('0');
            else 
                sz[i] = (TCHAR)idigit - 10 + TEXT('A');
        }
    }
} 

// set valid flags for the lang list
// very expensive so expects to be called only once in a session
// from CUILangList::GetLangList
//
#define MAX_SATELLITEPACKS 30 // 30 must be a practical number for satellite packs
void CUILangList::ValidateLangList()
{
    HKEY hKey;
    HRESULT hr;
    TCHAR szValueName[32];
    WORD aryValidLang[MAX_SATELLITEPACKS +1+1] = {0}; // +1 for install lang, 
                                                      // +1 for terminator

    int  nMaxValidLang = ARRAYSIZE(aryValidLang)-1;   // -1 for terminator
    WORD *pwValid = aryValidLang;
    
    // make the install language always valid
    *pwValid = GetInstallLanguage();
    if (*pwValid != 0)
    {
       *(pwValid+1) = 0; // terminator
       pwValid++;
       nMaxValidLang--;
    }

    if (ERROR_SUCCESS == 
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_INTERNATIONAL, NULL, KEY_READ, &hKey))
    {
        int i = 0;
        do {
            // see if the value has a match in the list
            DWORD dwType;
            DWORD cb = ARRAYSIZE(szValueName)-2;

            hr = SHEnumValue(hKey, i++, szValueName+2, &cb, &dwType, NULL, NULL);
            if (SUCCEEDED(hr) && dwType == REG_SZ)
            {
                UINT uiInstalled ;

                szValueName[0] = TEXT('0');
                szValueName[1] = TEXT('x');
                StrToIntEx(szValueName, STIF_SUPPORT_HEX, (LPINT)&uiInstalled);
                if (uiInstalled > 0)
                {
                    *pwValid     = (unsigned short) uiInstalled;
                    *(pwValid+1) = 0; // terminator
                    pwValid++;
                }
            }
        } while(hr == ERROR_SUCCESS && i < nMaxValidLang);
        RegCloseKey(hKey);
    }

    // this assumes we can use StrChrW to search a value in 
    // a word array, it also assumes we never have 0 as a langid
    //
    Assert(sizeof(WORD) == sizeof(WCHAR)); // unix?

    int nValidLang = (int)(pwValid-aryValidLang);
    for(int idx = 0; idx < GetListSize(); idx++ )
    {
        // abusing the string function but this is a fast way
        if (StrChrW((WCHAR *)aryValidLang, (WCHAR)lang[idx].wlangid))
        {
            lang[idx].fValid = TRUE;
            if(--nValidLang <= 0)
                break;
        }
    }
}

static const TCHAR s_szPropLangList[] = TEXT("langlist");
HRESULT CUILangList::GetLangList(HWND hdlg, CUILangList ** ppLangList)
{
    HRESULT hr=S_OK;
    
    CUILangList *pLangList = (CUILangList *)GetProp(hdlg, s_szPropLangList);
    if (!pLangList)
    {
        pLangList = new CUILangList();
        if (pLangList)
        {
            pLangList->ValidateLangList();
            SetProp(hdlg, s_szPropLangList, (HANDLE)pLangList);
        }
        else
            hr = E_FAIL;
    }
    
    ASSERT(ppLangList);
    if (ppLangList)
        *ppLangList = pLangList;
    
    return hr;
}

HRESULT CUILangList::RemoveLangList(HWND hdlg)
{
    HRESULT hr = S_OK;
    CUILangList *pLangList = (CUILangList *)GetProp(hdlg, s_szPropLangList);

    if (pLangList)
    {
        delete pLangList;
        RemoveProp(hdlg, s_szPropLangList);    
    }
    else
        hr = S_FALSE;

    return hr;
}

void CUILangList::SetCurrentLangIdx(int idx)
{
    TCHAR sz[4+1];
    if (idx != _iLangIdx)
    {
        // the resource id is always 4 digit
        IntToHex(sz, 4, lang[idx].wlangid);
        SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_INTERNATIONAL, 
                   s_szResourceLocale, REG_SZ, (void *)sz, sizeof(sz));
        _iLangIdx = idx;
    }
}
// returns idx to the lang array
int CUILangList::GetCurrentLangIdx()
{
    // show the current selection
    TCHAR sz[64];
    DWORD dwType;
    int   isel;
    
    // see if it's cached already
    if (_iLangIdx == -1)
    {
        // We basically wants what we've set in the registry,
        // but if Office9 is installed we'll show whatever
        // Office sets, and we can't change the Office setting anyway
        // MLGetUILanguage returns Office's setting if its there
        // Also I suppose we want to show NT5's UI language here
        //
        if (IsOffice9Installed() || IsOS(OS_NT5))
            isel = MLGetUILanguage();
        else
        {
            DWORD dwcbData = sizeof(sz);

            HRESULT hr =  SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_INTERNATIONAL, 
                                  s_szResourceLocale, &dwType, (void *)&sz[2], &dwcbData);
                   
            if (hr == ERROR_SUCCESS && dwType == REG_SZ)
            {
                sz[0] = TEXT('0');
                sz[1] = TEXT('x');
                StrToIntEx(sz, STIF_SUPPORT_HEX, (LPINT)&isel);
            }
            else
            {
                isel = GetInstallLanguage();
            }
        }
        
        for(int i = 0; i < GetListSize(); i++ )
        {
            if (isel == lang[i].wlangid)
            {
                _iLangIdx = i;
                break;
            }
        }
            
        // english for error case
        if (_iLangIdx < 0) 
            _iLangIdx = 0;
    }
    return _iLangIdx;
}

LPCTSTR CUILangList::GetLangNameOfIdx(int idx)
{
    LPCTSTR pszRet = NULL;
    IMultiLanguage2 *pML2;
    HRESULT hr;
    RFC1766INFO Rfc1766Info={0};

    if(!hOLE32)
    {
        if(!_StartOLE32())
        {
            ASSERT(FALSE);
            return NULL;
        }
    }
    hr = pCoInitialize(NULL);

    if (FAILED(hr))
        return NULL;

    hr = pCoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (LPVOID *) &pML2);

    if (SUCCEEDED(hr))
    {
        if (idx >= 0)
        {
            if (!lang[idx].szName[0])
            {
                pML2->GetRfc1766Info(lang[idx].wlangid, MLGetUILanguage(), &Rfc1766Info);
                StrCpyNW(lang[idx].szName, Rfc1766Info.wszLocaleName, ARRAYSIZE(lang[0].szName));            
            }
            pszRet = lang[idx].szName;        
        }
        pML2->Release();
    }

    pCoUninitialize();
    return pszRet;
}
 
LPCTSTR CUILangList::GetCurrentLangName()
{
    int idx = GetCurrentLangIdx();
    return GetLangNameOfIdx(idx);
}

BOOL CUILangList::IsOffice9Installed()
{
    DWORD dwVersion;
    DWORD cb = sizeof(dwVersion);
    if (_fOffice9Installed < 0)
    {
        _fOffice9Installed ++;
        if (ERROR_SUCCESS ==
            SHGetValue(HKEY_LOCAL_MACHINE, s_szLangPackPath, s_szVersion, NULL, &dwVersion, &cb)
          && dwVersion > 0) // magic number - christw tells me so
            _fOffice9Installed ++;
    }
    return (BOOL)_fOffice9Installed;
}

void InitCurrentUILang(HWND hDlg)
{
    BOOL fChanged = FALSE;
    CUILangList *pLangList;  
    LPCTSTR pszLangSel = NULL;
    HRESULT hr;
    
    hr = CUILangList::GetLangList(hDlg, &pLangList);
    
    if (SUCCEEDED(hr))
        pszLangSel = pLangList->GetCurrentLangName();
    
    if (pszLangSel)
    {
        TCHAR szBig[1024], szSmall[256];

        GetDlgItemText(hDlg, IDC_LANG_CURSEL, szBig, ARRAYSIZE(szBig));
        if (szBig[0])
            fChanged = (StrStr(szBig, pszLangSel) == NULL); 

        if (MLLoadString((fChanged)? IDS_LANG_FUTUREUSE: IDS_LANG_CURRENTUSE, szSmall, ARRAYSIZE(szSmall)) > 0)
        {
            wnsprintf(szBig, ARRAYSIZE(szBig), szSmall, pszLangSel);
            Static_SetText(GetDlgItem(hDlg, IDC_LANG_CURSEL), szBig);
        }
    }
}


//
// FillAcceptListBox()
//
// Fills the accept language listbox with names of selected language
//
void FillAcceptListBox(IN HWND hDlg)
{
    IMultiLanguage2 *pML2;
    HRESULT hr;
    HKEY hKey;
    DWORD cb;
    TCHAR sz[MAX_LIST_STRING_LEN], szBuf[MAX_ACCEPT_LANG_LEN], *p1, *p2, *p3;
    HWND hwndList = GetDlgItem(hDlg, IDC_LANG_ACCEPT_LIST);

    if(!hOLE32)
    {
        if(!_StartOLE32())
        {
            ASSERT(FALSE);
            return;
        }
    }
    hr = pCoInitialize(NULL);
    if (FAILED(hr))
        return;

    hr = pCoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (LPVOID *) &pML2);
    if (SUCCEEDED(hr))
    {
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_INTERNATIONAL, NULL, NULL, NULL, KEY_SET_VALUE|KEY_READ, NULL, &hKey, NULL))
        {
            LCID lcid;
            RFC1766INFO Rfc1766Info;
            TCHAR sz1[MAX_LIST_STRING_LEN], sz2[MAX_RFC1766_NAME];

            cb = sizeof(szBuf);
            if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_ACCEPT_LANGUAGE, NULL, NULL, (LPBYTE)szBuf, &cb))
            {
                p1 = p2 = szBuf;
                while (NULL != *p1)
                {
                    WCHAR wsz[MAX_LIST_STRING_LEN];
                    BOOL bEnd = FALSE;

                    while (TEXT(',') != *p2 && NULL != *p2)
                        p2 = CharNext(p2);
                    if (NULL != *p2)
                        *p2 = NULL;
                    else
                        bEnd = TRUE;
                    p3 = p1;
                    while (TEXT(';') != *p3 && NULL != *p3)
                        p3 = CharNext(p3);
                    if (NULL != *p3)
                        *p3 = NULL;
#ifdef UNICODE
                    StrCpyN(wsz, p1, ARRAYSIZE(wsz));
#else
                    MultiByteToWideChar(CP_ACP, 0, p1, -1, wsz, MAX_RFC1766_NAME);
#endif
                    hr = pML2->GetLcidFromRfc1766(&lcid, wsz);
                    if (SUCCEEDED(hr))
                    {
                        hr = pML2->GetRfc1766Info(lcid, MLGetUILanguage(), &Rfc1766Info);
                        if (SUCCEEDED(hr))
                        {
#ifdef UNICODE
                            StrCpyN(sz1, Rfc1766Info.wszLocaleName, ARRAYSIZE(sz1));
#else
                            WideCharToMultiByte(CP_ACP, 0, Rfc1766Info.wszLocaleName, -1, sz1, MAX_LIST_STRING_LEN, NULL, NULL);
#endif
                            wnsprintf(sz, ARRAYSIZE(sz), FORMAT_STR, sz1, p1);
                        }
                    }
                    else
                    {
                        MLLoadString(IDS_USER_DEFINED, sz1, ARRAYSIZE(sz1));
                        wnsprintf(sz, ARRAYSIZE(sz), FORMAT_STR, sz1, p1);
                    }
                    ListBox_AddString(hwndList, sz);
                    if (TRUE == bEnd)
                        p1 = p2;
                    else
                        p1 = p2 = p2 + 1;
                }
            }
            else
            {
                lcid = GetUserDefaultLCID();

                hr = pML2->GetRfc1766Info(lcid, MLGetUILanguage(), &Rfc1766Info);
                if (SUCCEEDED(hr))
                {
#ifdef UNICODE
                    StrCpyN(sz1, Rfc1766Info.wszLocaleName,  ARRAYSIZE(sz1));
                    StrCpyN(sz2, Rfc1766Info.wszRfc1766,  ARRAYSIZE(sz2));
#else
                    WideCharToMultiByte(CP_ACP, 0, Rfc1766Info.wszLocaleName, -1, sz1, MAX_LIST_STRING_LEN, NULL, NULL);
                    WideCharToMultiByte(CP_ACP, 0, Rfc1766Info.wszRfc1766, -1, sz2, MAX_RFC1766_NAME, NULL, NULL);
#endif
                    wnsprintf(sz, ARRAYSIZE(sz), FORMAT_STR, sz1, sz2);
                    ListBox_AddString(hwndList, sz);
                }
            }
            RegCloseKey(hKey);
        }
        pML2->Release();
    }
    pCoUninitialize();
}

//
// LanguageDlgInit()
//
// Initializes the Language dialog.
//
BOOL LanguageDlgInit(IN HWND hDlg)
{
    if (!hDlg)
        return FALSE;   // nothing to initialize

    FillAcceptListBox(hDlg);

    EnableWindow(GetDlgItem(hDlg, IDC_LANG_REMOVE_BUTTON), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_LANG_MOVE_UP_BUTTON), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_LANG_MOVE_DOWN_BUTTON), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_LANG_ADD_BUTTON), !g_restrict.fInternational);
    
    // On NT5, we use NT5's MUI feature instead of IE5 plugui
    if (IsOS(OS_NT5))
        ShowWindow(GetDlgItem(hDlg, IDC_LANG_UI_PREF), SW_HIDE);
    else
        EnableWindow(GetDlgItem(hDlg, IDC_LANG_UI_PREF), !g_restrict.fInternational);

    // show the current UI lang
    InitCurrentUILang(hDlg);
    
    // everything ok
    return TRUE;
}

//
// SaveLanguageData()
//
// Save the new language settings into regestry
//
void SaveLanguageData(IN HWND hDlg)
{
    HKEY hKey;
    DWORD dw;
    int i, iNumItems, iQ, n;
    TCHAR szBuf[MAX_ACCEPT_LANG_LEN];

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_INTERNATIONAL, NULL, NULL, NULL, KEY_WRITE, NULL, &hKey, &dw ))
    {
        HWND hwndList = GetDlgItem(hDlg, IDC_LANG_ACCEPT_LIST);

        iNumItems = ListBox_GetCount(hwndList);

        for (n = 1, iQ = 10; iQ < iNumItems; iQ *= 10, n++)
            ;

        szBuf[0] = NULL;
        for (i = 0; i < iNumItems; i++)
        {
            TCHAR sz[MAX_LIST_STRING_LEN], *p1, *p2;

            ListBox_GetText(hwndList, i, sz);
            p1 = sz;
            // We can assume safely there is '[' and ']' in this string.
            while (TEXT('[') != *p1)
                p1 = CharNext(p1);
            p1 = p2 = p1 + 1;
            while (TEXT(']') != *p2)
                p2 = CharNext(p2);
            *p2 = NULL;
            if (0 == i)
                StrCpyN(szBuf, p1, ARRAYSIZE(szBuf));
            else
            {
                TCHAR szF[MAX_ACCEPT_LANG_LEN], szQ[MAX_ACCEPT_LANG_LEN];

                int len = lstrlen(szBuf);
                StrCpyN(szBuf + len, TEXT(","), ARRAYSIZE(szBuf) - len);
                len++;
                StrCpyN(szBuf + len, p1, ARRAYSIZE(szBuf) - len);
                wnsprintf(szF, ARRAYSIZE(szF), TEXT(";q=0.%%0%dd"), n);
                wnsprintf(szQ, ARRAYSIZE(szQ), szF, ((iNumItems - i) * iQ + (iNumItems / 2)) / iNumItems);
                len = lstrlen(szBuf);
                StrCpyN(szBuf + len , szQ, ARRAYSIZE(szBuf) - len);
            }
        }
        RegSetValueEx(hKey, REGSTR_VAL_ACCEPT_LANGUAGE, NULL, REG_SZ, (LPBYTE)szBuf, (lstrlen(szBuf)+1)*sizeof(TCHAR));
        RegCloseKey(hKey);
    }
}

// MoveUpDownListItem()
//
// Move selected list item up or down
//
void MoveUpDownListItem(HWND hDlg, HWND hwndList, BOOL bUp)
{
    int i, iNumItems;
    TCHAR sz[MAX_LIST_STRING_LEN];

    i = ListBox_GetCurSel(hwndList);
    iNumItems = ListBox_GetCount(hwndList);
    ListBox_GetText(hwndList, i, sz);
    ListBox_DeleteString(hwndList, i);

    i += (bUp)? -1: 1;
    if (i < 0)
        i = 0;
    else if (i >= iNumItems)
        i = iNumItems - 1;
    ListBox_InsertString(hwndList, i, sz);
    ListBox_SetSel(hwndList, TRUE, i);
    ListBox_SetCurSel(hwndList, i);

    EnableWindow(GetDlgItem(hDlg, IDC_LANG_MOVE_UP_BUTTON), i != 0);
    EnableWindow(GetDlgItem(hDlg, IDC_LANG_MOVE_DOWN_BUTTON), i < iNumItems - 1);

    if (NULL == GetFocus()) // This prevent keyboard access disable
        SetFocus(hwndList);
}


//
// FillLanguageListBox()
//
// Fills the language listbox with the names of available languages
//
BOOL FillLanguageListBox(IN HWND hDlg)
{
    IMultiLanguage2 *pML2;
    HRESULT hr;
    TCHAR sz[MAX_LIST_STRING_LEN], sz1[MAX_LOCALE_NAME], sz2[MAX_RFC1766_NAME];
    HWND hwndEdit = GetDlgItem(hDlg, IDC_LANG_USER_DEFINED_EDIT);
    HWND hwndList = GetDlgItem(hDlg, IDC_LANG_AVAILABLE_LIST);
    HWND hwndAccept = GetDlgItem(GetParent(hDlg), IDC_LANG_ACCEPT_LIST);
    
    SendMessage(hwndEdit, EM_SETLIMITTEXT, 16, 0L); // Set Limit text as 16 characters

    if(!hOLE32)
    {
        if(!_StartOLE32())
        {
            ASSERT(FALSE);
            return FALSE;
        }
    }
    hr = pCoInitialize(NULL);
    if (FAILED(hr))
        return FALSE;

    hr = pCoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (LPVOID *) &pML2);
    if (SUCCEEDED(hr))
    {
        IEnumRfc1766 *pEnumRfc1766;
        RFC1766INFO Rfc1766Info;

        if (SUCCEEDED(pML2->EnumRfc1766(MLGetUILanguage(), &pEnumRfc1766)))
        {
            while (S_OK == pEnumRfc1766->Next(1, &Rfc1766Info, NULL))
            {
#ifdef UNICODE
                StrCpyN(sz1, Rfc1766Info.wszLocaleName, ARRAYSIZE(sz1));
                StrCpyN(sz2, Rfc1766Info.wszRfc1766,  ARRAYSIZE(sz2));
#else
                WideCharToMultiByte(CP_ACP, 0, Rfc1766Info.wszLocaleName, -1, sz1, MAX_LOCALE_NAME, NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, Rfc1766Info.wszRfc1766, -1, sz2, MAX_RFC1766_NAME, NULL, NULL);
#endif
                wnsprintf(sz, ARRAYSIZE(sz), FORMAT_STR, sz1, sz2);
                if (LB_ERR == ListBox_FindStringExact(hwndAccept, -1, sz))
                    ListBox_AddString(hwndList, sz);
            }
            pEnumRfc1766->Release();
        }
        pML2->Release();
    }
    pCoUninitialize();
    
    // everything ok
    return TRUE;
}

//
// AddLanguage()
//
// Add selected language to accept language listbox.
//
void AddLanguage(IN HWND hDlg)
{
    int i, j, *pItems, iNumItems, iIndex;
    TCHAR sz[MAX_LIST_STRING_LEN];
    HWND hdlgParent = GetParent(hDlg);
    HWND hwndFrom = GetDlgItem(hDlg, IDC_LANG_AVAILABLE_LIST);
    HWND hwndTo = GetDlgItem(hdlgParent, IDC_LANG_ACCEPT_LIST);

    i = ListBox_GetSelCount(hwndFrom);
    if (0 < i && (pItems = (PINT)LocalAlloc(LPTR, sizeof(int)*i)))
    {
        ListBox_GetSelItems(hwndFrom, i, pItems);
        for (j = 0; j < i; j++)
        {
            ListBox_GetText(hwndFrom, pItems[j], sz);
            ListBox_AddString(hwndTo, sz);
        }
        LocalFree(pItems);
    }
    if (GetWindowTextLength(GetDlgItem(hDlg, IDC_LANG_USER_DEFINED_EDIT)))
    {
        TCHAR *p, sz1[MAX_LIST_STRING_LEN], sz2[MAX_LIST_STRING_LEN];
        BOOL fValid = TRUE;

        GetWindowText(GetDlgItem(hDlg, IDC_LANG_USER_DEFINED_EDIT), sz2, ARRAYSIZE(sz2));
        p = sz2;
        while (NULL != *p && TRUE == fValid)
        {
            switch (*p)
            {
                // Invalid characters for user-defined string
                case TEXT(','):
                case TEXT(';'):
                case TEXT('['):
                case TEXT(']'):
                case TEXT('='):
                    fValid = FALSE;
                    break;

                default:
                    p = CharNext(p);
            }
        }
        if (FALSE == fValid)
        {
            TCHAR szTitle[256], szErr[1024];

            MLLoadShellLangString(IDS_USER_DEFINED_ERR, szErr, ARRAYSIZE(szErr));
            GetWindowText(hDlg, szTitle, ARRAYSIZE(szTitle));
            MessageBox(hDlg, szErr, szTitle, MB_OK | MB_ICONHAND);
        }
        else
        {
            MLLoadString(IDS_USER_DEFINED, sz1, ARRAYSIZE(sz1));
            wnsprintf(sz, ARRAYSIZE(sz), FORMAT_STR, sz1, sz2);
            ListBox_AddString(hwndTo, sz);
        }
    }
    iIndex = ListBox_GetCurSel(hwndTo);
    if (LB_ERR != iIndex)
    {
        iNumItems = ListBox_GetCount(hwndTo);
        EnableWindow(GetDlgItem(hdlgParent, IDC_LANG_REMOVE_BUTTON), iNumItems > 0);
        EnableWindow(GetDlgItem(hdlgParent, IDC_LANG_MOVE_UP_BUTTON), iIndex > 0);
        EnableWindow(GetDlgItem(hdlgParent, IDC_LANG_MOVE_DOWN_BUTTON), iIndex < iNumItems - 1);
    }
}

int ComboBoxEx_AddString(IN HWND hwndCtl, IN LPCTSTR sz)
{
    COMBOBOXEXITEM cbexItem = {0};
    
    int csz = _tcslen(sz);

    cbexItem.mask = CBEIF_TEXT;
    cbexItem.pszText = (LPTSTR)sz;
    cbexItem.cchTextMax = csz;
    
    // sort the string based on the current locale
    // we don't bother to use binary search because
    // the list is up to 25 item
    TCHAR szItem[MAX_LOCALE_NAME];
    int i, itemCount = ComboBox_GetCount(hwndCtl);
    for (i = 0; i < itemCount; i++)
    {
        ComboBox_GetLBText(hwndCtl, i, szItem);
        if (CompareString(MLGetUILanguage(), 
                          0,
                          sz,
                          csz,
                          szItem,
                          ARRAYSIZE(szItem)) == CSTR_LESS_THAN)
        {
            break;
        }
    }
    cbexItem.iItem = i;
    
    SendMessage(hwndCtl, CBEM_INSERTITEM, (WPARAM)0, (LPARAM)(LPVOID)&cbexItem);
    return i;
}

#define CP_THAI     874
#define CP_ARABIC   1256
#define CP_HEBREW   1255

BOOL FillUILangListBox(IN HWND hDlg, CUILangList *pLangList)
{
    HWND hwndCombo = GetDlgItem(hDlg, IDC_COMBO_UILANG);
    BOOL bNT5 = IsOS(OS_NT5);
    DWORD dwAcp = GetACP();
    LPCTSTR pszLangName;
    
    if (!pLangList)
        return FALSE;

    // fill the list up.
    for (int i = 0; i < pLangList->GetListSize(); i++)
    {
        if (!pLangList->IsValidLang(i))
            continue;

        if (!bNT5)
        {
            LANGID lid = pLangList->GetLangIdOfIdx(i);

            if (dwAcp == CP_THAI || dwAcp == CP_ARABIC || dwAcp == CP_HEBREW)
            {
                // do not support cross codepage PlugUI
                // on Thai or Middle East platform(Arabic/Hebrew)
                static DWORD dwDefCP = 0;

                if (dwDefCP == 0)
                {
                    TCHAR szLcData[6+1]; // +2 for '0x' +1 for terminator

                    GetLocaleInfo( MAKELCID(lid, SUBLANG_NEUTRAL),
                        LOCALE_IDEFAULTANSICODEPAGE, szLcData, ARRAYSIZE(szLcData));
                                       
                    dwDefCP = StrToInt(szLcData);
                }
                if (dwDefCP != dwAcp && lid != 0x0409 && lid != GetInstallLanguage())
                    continue;
            }
            else
            {
                // skip Arabic and Hebrew on non-supporting platform
                if (lid == 0x401 || lid == 0x40d)
                    continue;
            }
        }

        pszLangName = pLangList->GetLangNameOfIdx(i);

        // BUGBUG: ComboBox_FindStringExact has problems to handle DBCS Unicode characters 
        if (pszLangName /*&& CB_ERR == ComboBox_FindStringExact(hwndCombo, -1, pszLangName)*/)
        {
            int ipos = ComboBoxEx_AddString(hwndCombo, pszLangName);
            if (ipos >= 0)
            {
                ComboBox_SetItemData(hwndCombo, ipos, i);
            }
        }
    }

    // show the current selection
    pszLangName = pLangList->GetCurrentLangName();
    if (pszLangName)
    {
        int iPosCurSel = ComboBox_FindStringExact(hwndCombo, -1, pszLangName);
        if (iPosCurSel >= 0)
            ComboBox_SetCurSel(hwndCombo, iPosCurSel);
    }
    return TRUE;
}

//
// Shutdown/reboot procedures implementation
//
// synopsis: CShutDownInfo class implements the method and the process list
//           which handle the sequence.
//           s_arryClsNames[] holds the list of target application
//           ChangeLanguage() (global) triggers the sequence being called from
//           LangChangeDlgProc().
//
static const LPTSTR s_arryClsNames[] =  
{
    TEXT("IEFrame"),                       // browser instance
    TEXT("ThorBrowserWndClass"),           // OE 
    TEXT("HH Parent"),                     // Html Help
    TEXT("MPWClass"),                      // 
    TEXT("Outlook Express Browser Class"), // OE
    TEXT("ATH_Note"),                      // OE?
    TEXT("WABBrowseView"),                 // WAB
    TEXT("Afx:400000:8:10008:0:900d6"),
    TEXT("Media Player 2"),
    TEXT("FrontPageExpressWindow"), 
    TEXT("MSBLUIManager"),                 // Messenger
};

//
// CShutDownInfo
// class methods implementation
//
#define SHUTDOWN_TIMEOUT 2000 // 2 sec
#define RELAUNCH_TIMEOUT 1000 // 1 sec
CShutDownProcInfo::CShutDownProcInfo(HWND hDlg)
{
    _pProcList = NULL;
    _nAlloced = 0;
    _iProcList = 0;
    _hdlgParent = hDlg;
    _fAllShutDown = FALSE;
}

CShutDownProcInfo::~CShutDownProcInfo()
{
    if (_pProcList)
        LocalFree(_pProcList);
}


HRESULT CShutDownProcInfo::EnsureProcList()
{
    HRESULT hr = S_OK;
    if (!_pProcList)
    {
        // alloc mem for practical # of processes
        _nAlloced = ARRAYSIZE(s_arryClsNames);
        _pProcList = (PROCLIST *)LocalAlloc(LPTR, sizeof(PROCLIST)*_nAlloced);
    }
    if (!_pProcList) 
    {
        _nAlloced = 0;
        hr = E_FAIL;
    }

    return hr;
}
HRESULT CShutDownProcInfo::IncreaseProcList()
{
    HRESULT hr = S_OK;
    PROCLIST * pl = NULL;
    // realloc mem every so often
    if (_iProcList+1 > _nAlloced)
    {
        pl = (PROCLIST *)LocalReAlloc(_pProcList, sizeof(PROCLIST)*(ARRAYSIZE(s_arryClsNames)+_nAlloced), 
                                      LMEM_MOVEABLE | LMEM_ZEROINIT);
        if (pl)
        {
            _nAlloced += ARRAYSIZE(s_arryClsNames);
            _pProcList =  pl;
        }
        else
           hr = E_FAIL;
    }

    if (hr == S_OK)
        _iProcList++;

    return hr;
}
// CShutDownProcInfo::AddToProcList()
//
// synopsis: Get process info from given window handle
//           store it for shutdown procedure
//
//
//
HRESULT CShutDownProcInfo::AddToProcList(HWND hwnd)
{
    HRESULT hr = S_OK;

    hr = EnsureProcList();
    if (SUCCEEDED(hr) && hwnd)
    {
        DWORD dwPID;
        BOOL  fFoundDup = FALSE;

        GetWindowThreadProcessId(hwnd, &dwPID);
        
        // check to see if we already have the PID in the list
        for (int i=0; i < _iProcList; i++)
        {
            if (_pProcList[i].dwPID == dwPID)
            {
                fFoundDup = TRUE;
                break;
            }
        }

        // add proccess info only if we don't have it already
        if (!fFoundDup)
        {
            hr = IncreaseProcList();
            if (SUCCEEDED(hr))
            {
                int iCur = _iProcList-1;

                GetExeNameFromPID(dwPID, 
                    _pProcList[iCur].szExeName, 
                    ARRAYSIZE(_pProcList[iCur].szExeName));

                _pProcList[iCur].dwPID = dwPID;
                _pProcList[iCur].State = PS_UNKNOWN;
            }
        }
    }
    return hr;    
}

// CShutDownProcInfo::WaitForOneProcess
//
// synopsis: ensures the given process 
//           has terminated
//
//
HRESULT CShutDownProcInfo::WaitForOneProcess(int iProc)
{
    HRESULT hr = S_OK;
    if (iProc < _iProcList && _pProcList[iProc].State != PS_SHUTDOWN_OK)
    {
        DWORD dwProcessFlags = PROCESS_ALL_ACCESS | 
                               (_fNT ? SYNCHRONIZE : 0 );

        HANDLE hProc = OpenProcess(dwProcessFlags,
                                   FALSE,    
                                   _pProcList[iProc].dwPID);

        // pressume it has terminated, get it marked so
        _pProcList[iProc].State = PS_SHUTDOWN_OK;

        if (hProc) 
        {
            // if the proccess in query is still alive,
            // we'll wait with time out here
            //
            DWORD dwRet = WaitForSingleObject (hProc, SHUTDOWN_TIMEOUT);
            if (dwRet == WAIT_TIMEOUT)
            {
                _pProcList[iProc].State = PS_WAITING;
            }
            
            CloseHandle(hProc);
        }
    }
    return hr;
}

// CShutDownProcInfo::WaitForFolksShutDown
//
// synopsis: ensure the nominated processes terminate. If anyone 
//           doesn't want to terminate, wait for her retrying a couple of
//           times and note her name so we can show it to the user. 
//          
//
#define MAXSHUTDOWNTRY 10
HRESULT CShutDownProcInfo::WaitForFolksShutDown()
{
    HRESULT hr = S_OK;
    int    iTry = 0;
    do
    {
        // pressume all will be fine
        _fAllShutDown = TRUE;
        // waiting loop
        for (int i = 0; i < _iProcList; i++)
        {
            WaitForOneProcess(i);
            if (_pProcList[i].State != PS_SHUTDOWN_OK)
                _fAllShutDown = FALSE;
        }
    }
    while( !_fAllShutDown && iTry++ < MAXSHUTDOWNTRY  );
    // BUGBUG: here we should put up a dialog
    //         to ask user if they want to wait
    //         for the apps 

    return hr;
}

// CShutDownProcInfo::NotifyShutDownToFolks
//
// synopsis: send POI_OFFICE_COMMAND to possible candidates on the desktop
//           if a candidate replies with valid value, save the proccess
//           information for the later restart procedure.
//
HRESULT CShutDownProcInfo::NotifyShutDownToFolks(int *pnProcess)
{
    HWND hwndShutDown, hwndAfter;
    PLUGUI_QUERY pq;
    HRESULT hr = S_OK;
    int     nProcToShutDown = 0;

    for (int i = 0; i < ARRAYSIZE(s_arryClsNames); i++)
    {
        hwndAfter = NULL; 
        while (hwndShutDown = FindWindowEx(NULL, hwndAfter, s_arryClsNames[i], NULL))
        {
            pq.uQueryVal = (UINT)SendMessage(hwndShutDown, PUI_OFFICE_COMMAND, PLUGUI_CMD_QUERY, 0);
            if (pq.uQueryVal)
            {
                if(pq.PlugUIInfo.uMajorVersion == OFFICE_VERSION_9)
                {
                    PostMessage(hwndShutDown, PUI_OFFICE_COMMAND, (WPARAM)PLUGUI_CMD_SHUTDOWN, 0);

                    // store the information about the process which this window belongs to
                    // we only need to remember non OLE processes here for re-starting.
                    if (!pq.PlugUIInfo.uOleServer)
                    {
                        AddToProcList(hwndShutDown);
                        nProcToShutDown ++;
                    }
                }
            }
            hwndAfter = hwndShutDown;
        }
    }
    if (!nProcToShutDown)
        hr = S_FALSE;

    if (pnProcess)
        *pnProcess = nProcToShutDown;

    return hr;
}

const TCHAR c_szRegAppPaths[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
HRESULT CShutDownProcInfo::GetRestartAppPath(LPTSTR szPath, int cchPath, int iProc)
{
    HRESULT hr = S_OK;
    TCHAR szAppPath[MAX_PATH];
    TCHAR szRegKey[MAX_PATH];

    ASSERT(szPath && cchPath > 0);

    if (iProc < _iProcList)
    {
        _tcscpy(szRegKey, c_szRegAppPaths);
        _tcscat(szRegKey, _pProcList[iProc].szExeName);
        
        DWORD cb = sizeof(szAppPath);
        if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, szRegKey, NULL, NULL, szAppPath, &cb))
        {
            szPath[0] = TEXT('0');
            hr = E_FAIL;
        }
        else
            _tcsncpy(szPath, szAppPath, cchPath);
    }
    return hr;
}

HRESULT CShutDownProcInfo::RestartFolks()
{
    HRESULT hr = S_OK;
    PROCESS_INFORMATION pi;

    for (int i = 0; i < _iProcList; i++)
    {

        STARTUPINFO si = {0};

        si.cb = sizeof(si);
        if (_pProcList[i].State == PS_SHUTDOWN_OK)
        {
            TCHAR szAppPath[MAX_PATH];    
            HRESULT hr = GetRestartAppPath(szAppPath, ARRAYSIZE(szAppPath), i);

            if (hr == S_OK)
            {
                BOOL fLaunchedOK = 
                CreateProcess (szAppPath,               // name of app to launch
                                NULL,                   // lpCmdLine
                                NULL,                   // lpProcessAttributes
                                NULL,                   // lpThreadAttributes
                                TRUE,                   // bInheritHandles
                                NORMAL_PRIORITY_CLASS,  // dwCreationFlags
                                NULL,                   // lpEnvironment
                                NULL,                   // lpCurrentDirectory
                                &si,                    // lpStartupInfo
                                &pi);                   // lpProcessInformation

                if (fLaunchedOK)
                { 
                    DWORD dwRet = WaitForInputIdle (pi.hProcess,
                                                    RELAUNCH_TIMEOUT);
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
        }
    }
    return hr;
}



// 
//   CShutDownProcInfo::ShutDownThreadProc
//
//   synopsis: launched from changelang dialog so the dialog
//             wouldn't get blocked when we're waiting for our apps
//             to shutdown/restart. this is a static proc
//             so we should be able to delete the class instance
//             in this proc.
//
DWORD CALLBACK CShutDownProcInfo::ShutDownThreadProc(void *pv)
{
    CShutDownProcInfo *pspi = (CShutDownProcInfo *)pv;
    
    if (pspi)
    {
        HRESULT hr;
        int     nToShutDown;
        // send PUI_OFFICE_COMMAND to corresponding folks...
        hr = pspi->NotifyShutDownToFolks(&nToShutDown);

        // and wait until all processes shutdown
        if (SUCCEEDED(hr) && nToShutDown > 0)
        {
            hr = pspi->WaitForFolksShutDown();

            // then restart here
            if (SUCCEEDED(hr))
               pspi->RestartFolks();
        }
    
        // now the parent dialog should go away
        int iret = (nToShutDown > 0) ? 
                   RETURN_SETLANG_ENDLANGDIALOG: RETURN_SETLANG_CLOSEDNORMAL;
        
        EndDialog(pspi->_hdlgParent, iret);
    
        // delete this class instance
        delete pspi;
    }
    return 0;
}

void OpenSatelliteDownloadUrl(HWND hDlg)
{
    // get the default Url from registry
    TCHAR szSatelliteUrl[INTERNET_MAX_URL_LENGTH];

    // reg api needs size in byte
    DWORD dwType, dwcbData = sizeof(szSatelliteUrl);
    
    DWORD dwRet =  SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_INTERNATIONAL, 
                             NULL, &dwType, (void *)szSatelliteUrl, &dwcbData);
    if (dwRet != ERROR_SUCCESS || !szSatelliteUrl[0])
    {
       // use the hard coded Url instead
       _tcscpy(szSatelliteUrl, s_szUrlSPK);
    }

    if(!hOLE32)
    {
        if(!_StartOLE32())
        {
            ASSERT(FALSE);
            return;
        }
    }

    HRESULT hr = pCoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        NavToUrlUsingIE(szSatelliteUrl, TRUE);
        pCoUninitialize();
    }
}

INT_PTR CALLBACK LangMsgDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDYES:
                case IDNO:
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
                    break;
            }
            return TRUE;
        }

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:    // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);        
            break;
    }
    return FALSE;
}

BOOL ChangeLanguage(IN HWND hDlg, CUILangList *pLangList)
{
    HWND hwndCombo = GetDlgItem(hDlg, IDC_COMBO_UILANG);
    int iSel = ComboBox_GetCurSel(hwndCombo);
    INT_PTR idxSel = 0;
    int idxCur;
    
    if (iSel != CB_ERR)
        idxSel = ComboBox_GetItemData(hwndCombo, iSel);

    if ( idxSel != CB_ERR 
        && idxSel < pLangList->GetListSize())
    {
        idxCur = pLangList->GetCurrentLangIdx();

        if (idxCur != idxSel)
        {
            INT_PTR iRet = DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_LANG_WARNING), hDlg, LangMsgDlgProc);

            if (IDCANCEL != iRet)
            {
                pLangList->SetCurrentLangIdx((int)idxSel);

                if (IDYES == iRet)
                {
                    CShutDownProcInfo  *pspi = new CShutDownProcInfo(hDlg);
                    if (!SHCreateThread(pspi->ShutDownThreadProc, (void *)pspi, 0, NULL))
                        delete pspi;

                    // returning TRUE to indicate that we do shutdown/restart
                    return TRUE;
                }
                else
                {
                    DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_LANG_INFO), hDlg, LangMsgDlgProc);
                }
            }
        }
    }
    // returning FALSE to indicate that we haven't changed the language
    return FALSE;
}

//
// LangChangeDlgProc()
//
// Message handler for the "Change Language" subdialog.
//
INT_PTR CALLBACK LangChangeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CUILangList *pLangList;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            CUILangList::GetLangList(GetParent(hDlg), &pLangList);
            return FillUILangListBox(hDlg, pLangList);
    
        case WM_DESTROY:
            break;

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_LANG_ADDSPK:
                    // open url from resource
                    OpenSatelliteDownloadUrl(hDlg);
                    EndDialog(hDlg, RETURN_SETLANG_ENDLANGDIALOG);
                    break;
                case IDOK:
                    if(!SUCCEEDED(CUILangList::GetLangList(GetParent(hDlg), &pLangList))
                      || !ChangeLanguage(hDlg, pLangList))
                      EndDialog(hDlg, 0);

                    // EndDialog() is called in separate thread 
                    // when shutdown/restart is done
                    // 
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;
            }
            break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:    // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);        
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

//
// LangAddDlgProc()
//
// Message handler for the "Add Language" subdialog.
//
INT_PTR CALLBACK LangAddDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return FillLanguageListBox(hDlg);
    
        case WM_DESTROY:
            break;

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                    AddLanguage(hDlg);
                    EndDialog(hDlg, 0);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;
            }
            break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:    // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);        
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

// put any cleanup procedures for language dialog here
void LangDlgCleanup(HWND hDlg)
{
    // also delete and remove the instance of
    // UI language list from window prop
    CUILangList::RemoveLangList(hDlg);
}
//
// LanguageDlgProc()
//
// Message handler for the "Language Preference" subdialog.
//
INT_PTR CALLBACK LanguageDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CUILangList *pLangList;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return LanguageDlgInit(hDlg);
    
        case WM_DESTROY:
            LangDlgCleanup(hDlg);
            break;

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
                HWND hwndList;
                int iIndex, iNumItems;
                INT_PTR iret;

                case IDOK:
                    SaveLanguageData(hDlg);
                    EndDialog(hDlg, 0);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                case IDC_LANG_ADD_BUTTON:
                    DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_LANG_ADD), hDlg, LangAddDlgProc);
                    break;

                case IDC_LANG_UI_PREF:
                    CUILangList::GetLangList(hDlg, &pLangList);
                    iret = KickSetLang(hDlg, pLangList);
                    if (iret == RETURN_SETLANG_ENDLANGDIALOG)
                    {
                        // we're outa job
                        EndDialog(hDlg, 0);
                    }
                    else
                    {
                        InitCurrentUILang(hDlg);
                    }
                    break;

                case IDC_LANG_REMOVE_BUTTON:
                    hwndList = GetDlgItem(hDlg, IDC_LANG_ACCEPT_LIST);
                    iIndex = ListBox_GetCurSel(hwndList);
                    ListBox_DeleteString(hwndList, iIndex);
                    iNumItems = ListBox_GetCount(hwndList);
                    if (iNumItems == iIndex)
                        iIndex--;
                    ListBox_SetCurSel(hwndList, iIndex);
                    EnableWindow(GetDlgItem(hDlg, IDC_LANG_REMOVE_BUTTON), (iNumItems > 0) && !g_restrict.fInternational);
                    EnableWindow(GetDlgItem(hDlg, IDC_LANG_MOVE_UP_BUTTON), (iIndex > 0) && !g_restrict.fInternational);
                    EnableWindow(GetDlgItem(hDlg, IDC_LANG_MOVE_DOWN_BUTTON), (iIndex < iNumItems - 1) && !g_restrict.fInternational);

                    if (NULL == GetFocus()) // This prevent keyboard access disable
                        SetFocus(hwndList);
                    break;

                case IDC_LANG_ACCEPT_LIST:
                    hwndList = GetDlgItem(hDlg, IDC_LANG_ACCEPT_LIST);
                    iIndex = ListBox_GetCurSel(hwndList);
                    if (0 <= iIndex)
                    {
                        iNumItems = ListBox_GetCount(hwndList);
                        EnableWindow(GetDlgItem(hDlg, IDC_LANG_REMOVE_BUTTON), (iNumItems > 0) && !g_restrict.fInternational);
                        EnableWindow(GetDlgItem(hDlg, IDC_LANG_MOVE_UP_BUTTON), (iIndex > 0) && !g_restrict.fInternational);
                        EnableWindow(GetDlgItem(hDlg, IDC_LANG_MOVE_DOWN_BUTTON), (iIndex < iNumItems - 1) && !g_restrict.fInternational);
                    }
                    break;

                case IDC_LANG_MOVE_UP_BUTTON:
                    MoveUpDownListItem(hDlg, GetDlgItem(hDlg, IDC_LANG_ACCEPT_LIST), TRUE);
                    break;

                case IDC_LANG_MOVE_DOWN_BUTTON:
                    MoveUpDownListItem(hDlg, GetDlgItem(hDlg, IDC_LANG_ACCEPT_LIST), FALSE);
                    break;
            }
            break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:    // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);        
            break;

        default:
            return FALSE;
    }
    return TRUE;
}


//
// KickLanguageDialog
//
// synopsis : used for launching Language Preference sub dialog.
//            we need to launch the dialogbox as a separate process if inetcpl is 
//            invoked from Tools->Internet options. 
//            The reason: we shutdown every browser instances on desktop
//                        user chooses different UI language than the current,
//                        including the browser that launched inetcpl.
//
static const TCHAR  s_szRunDll32[] = TEXT("RunDll32.exe");
static const TCHAR  s_szKickLangDialog[] = TEXT(" inetcpl.cpl,OpenLanguageDialog");
void KickLanguageDialog(HWND hDlg)
{
    // 1: here we want to check to see if inetcpl was launched 
    //         as a rundll32 process already, which would happen if user 
    //         clicks on it at control panel folder
    //
    //
    BOOL fLaunchedOnBrowser = FALSE;
    
    // this tells me whether we got invoked from Tools->Internet Options...
    if (g_szCurrentURL[0])
    {
        fLaunchedOnBrowser = TRUE;
    }
    
    if (fLaunchedOnBrowser)
    {
        TCHAR szCommandLine[MAX_PATH];
        TCHAR szTitle[MAX_PATH];

        HWND hwndParent = GetParent(hDlg);
        
        StrCpy(szCommandLine, s_szRunDll32);
        StrCat(szCommandLine, s_szKickLangDialog);
        
        if (GetWindowText(hwndParent, szTitle, ARRAYSIZE(szTitle)) > 0)
        {
            StrCat(szCommandLine, TEXT(" "));
            StrCat(szCommandLine, szTitle);
        }
        
#ifdef USE_CREATE_PROCESS
        PROCESS_INFORMATION pi;
        STARTUPINFO si = {0};

        si.cb = sizeof(si);
        BOOL fLaunchedOK = 
        CreateProcess (szCommandLine,          // name of app to launch
                       NULL,                   // lpCmdLine
                       NULL,                   // lpProcessAttributes
                       NULL,                   // lpThreadAttributes
                       TRUE,                   // bInheritHandles
                       NORMAL_PRIORITY_CLASS,  // dwCreationFlags
                       NULL,                   // lpEnvironment
                       NULL,                   // lpCurrentDirectory
                       &si,                    // lpStartupInfo
                       &pi);                   // lpProcessInformation
#else
        char szAnsiPath[MAX_PATH];
        SHUnicodeToAnsi(szCommandLine, szAnsiPath, ARRAYSIZE(szAnsiPath));
        WinExec(szAnsiPath, SW_SHOWNORMAL);
#endif
    }
    else
    {
        DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_LANG), hDlg, LanguageDlgProc);
    }
}

//
// KickSetLang
//
// synopsis : tries to find setlang.exe of Office9 first, if found it'll be kicked
//            if not, it uses our own setlang dialog.
//
//
static const TCHAR s_szOfficeInstallRoot[] = TEXT("Software\\Microsoft\\Office\\9.0\\Common\\InstallRoot");
static const TCHAR s_szPath[] = TEXT("Path");
static const TCHAR s_szSetLangExe[] = TEXT("setlang.exe");

INT_PTR KickSetLang(HWND hDlg, CUILangList *pLangList)
{
    BOOL fOfficeSetLangInstalled = FALSE;
    INT_PTR iret;
    
    TCHAR szSetLangPath[MAX_PATH];    
    
    // try to get Office's setlang path
    if(pLangList && pLangList->IsOffice9Installed()) 
    {
        DWORD cb = sizeof(szSetLangPath);
        if (ERROR_SUCCESS == 
            SHGetValue(HKEY_LOCAL_MACHINE, s_szOfficeInstallRoot, s_szPath, NULL, szSetLangPath, &cb))
        {
            StrCat(szSetLangPath, s_szSetLangExe);
            if (PathFileExists(szSetLangPath) == TRUE)
                fOfficeSetLangInstalled = TRUE;
        }
    }
    
    if (fOfficeSetLangInstalled)
    {
        PROCESS_INFORMATION pi;
        STARTUPINFO si = {0};

        si.cb = sizeof(si);
        BOOL fLaunchedOK = CreateProcess(
                              szSetLangPath,     // name of app to launch
                                       NULL,     // lpCmdLine
                                       NULL,     // lpProcessAttributes
                                       NULL,     // lpThreadAttributes
                                       TRUE,     // bInheritHandles
                      NORMAL_PRIORITY_CLASS,     // dwCreationFlags
                                       NULL,     // lpEnvironment
                                       NULL,     // lpCurrentDirectory
                                       &si,      // lpStartupInfo
                                       &pi);     // lpProcessInformation
        // just wait a while
        if (fLaunchedOK)
        { 
            WaitForInputIdle (pi.hProcess, RELAUNCH_TIMEOUT);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        iret = RETURN_SETLANG_ENDLANGDIALOG;
    }
    else
    {
        iret = DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_LANG_CHANGE), hDlg, LangChangeDlgProc);
    }
    
    return iret;
}

//
// entry point for rundll32
// NOTE: the following function was written intentionally as non-Unicode
//       mainly because we don't have Wide wrapper mechanism for rundll32
//       function on win95
//
extern void GetRestrictFlags(RESTRICT_FLAGS *pRestrict);
void CALLBACK OpenLanguageDialog(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    // hinst is ignored because we set it at our LibMain()
    INITCOMMONCONTROLSEX icex;

    GetRestrictFlags(&g_restrict);
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_USEREX_CLASSES|ICC_NATIVEFNTCTL_CLASS;
    InitCommonControlsEx(&icex);
    
    if (lpszCmdLine && *lpszCmdLine)
    {
        HWND hwndParent = FindWindowA(NULL, lpszCmdLine);
        if (hwndParent)
            hwnd = hwndParent;
    }
    DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_LANG), hwnd, LanguageDlgProc);
}
