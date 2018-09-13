#include "private.h"
#include "chanmgr.h"
#include "chanmgrp.h"
#include "shguidp.h"
#include "resource.h"

#include <mluisupp.h>

#define TF_DUMPTRIGGER              0x80000000

#define PtrDifference(x,y)          ((LPBYTE)(x)-(LPBYTE)(y))

// Invoke Command verb strings
const CHAR c_szOpen[]          = "open";
const CHAR c_szDelete[]        = "delete";
const CHAR c_szProperties[]    = "properties";
const CHAR c_szCopy[]          = "copy";
const CHAR c_szRename[]        = "rename";
const CHAR c_szPaste[]         = "paste";

static TCHAR szNone[40] = {0};
static TCHAR szUnknown[40] = {0};

void FireSubscriptionEvent(int nCmdID, const SUBSCRIPTIONCOOKIE *pCookie)
{
    HKEY hkey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WEBCHECK_REGKEY_NOTF, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        LPOLESTR pszCookie;

        if (SUCCEEDED(StringFromCLSID(*pCookie, &pszCookie)))
        {
            VARIANT varCookie;

            varCookie.vt = VT_BSTR;
            varCookie.bstrVal = SysAllocString(pszCookie);

            if (varCookie.bstrVal)
            {
                for (int i = 0; ; i++)
                {
                    TCHAR szClsid[GUIDSTR_MAX];
                    DWORD cchClsid = ARRAYSIZE(szClsid);
                    DWORD dwType;
                    DWORD dwData;
                    DWORD cbData = sizeof(dwData);

                    int result = RegEnumValue(hkey, i, szClsid, &cchClsid, NULL, &dwType, (LPBYTE)&dwData, &cbData);

                    if (ERROR_NO_MORE_ITEMS == result)
                    {
                        break;
                    }

                    if ((ERROR_SUCCESS == result) && (dwData & nCmdID))
                    {
                        WCHAR wszClsid[GUIDSTR_MAX];
                        CLSID clsid;

                        SHTCharToUnicode(szClsid, wszClsid, ARRAYSIZE(wszClsid));

                        HRESULT hr = CLSIDFromString(wszClsid, &clsid);

                        if (SUCCEEDED(hr))
                        {
                            IOleCommandTarget *pCmdTarget;

                            hr = CoCreateInstance(*(&clsid), NULL, CLSCTX_ALL, IID_IOleCommandTarget, (void **)&pCmdTarget);
                            if (SUCCEEDED(hr))
                            {
                                pCmdTarget->Exec(&CLSID_SubscriptionMgr, nCmdID, 0, &varCookie, NULL);
                                pCmdTarget->Release();
                            }
                        }
                    }
                }

                VariantClear(&varCookie);
            }

            CoTaskMemFree(pszCookie);
        }
    }
}

#ifdef UNICODE
HRESULT IExtractIcon_GetIconLocationThunk(IExtractIconW *peiw, UINT uFlags, LPSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags)
{
    HRESULT hr;
    WCHAR *pwszIconFile = new WCHAR[cchMax];

    if (NULL != pwszIconFile)
    {
        hr = peiw->GetIconLocation(uFlags, pwszIconFile, cchMax, piIndex, pwFlags);

        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, pwszIconFile, -1, szIconFile, cchMax, NULL, NULL);
        }

        delete [] pwszIconFile;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT IExtractIcon_ExtractThunk(IExtractIconW *peiw, LPCSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize)
{
    HRESULT hr;
    int len = lstrlenA(pszFile) + 1;
    WCHAR *pwszFile = new WCHAR[len];

    if (NULL != pwszFile)
    {
        MultiByteToWideChar(CP_ACP, 0, pszFile, len, pwszFile, len);

        hr = peiw->Extract(pwszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);

        delete [] pwszFile;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}
#endif

DWORD Random(DWORD nMax)
{
    static DWORD dwSeed = GetTickCount();

    if (nMax)
    {
        return dwSeed = (dwSeed * 214013L + 2531011L) % nMax;
    }
    else
    {
        return 0;
    }
}

void CreateCookie(GUID *pCookie)
{
    static DWORD dwCount = 0;

    union CUCookie
    {
        GUID guidCookie;
        struct XCookie {
            FILETIME ft;
            DWORD    dwCount;
            DWORD    dwRand;
        } x;
    };

    CUCookie *puc = (CUCookie *)pCookie;
    GetSystemTimeAsFileTime(&puc->x.ft);
    puc->x.dwCount = dwCount++;
    puc->x.dwRand = Random(0xffffffff);
}

void VariantTimeToFileTime(double dt, FILETIME& ft)
{
    SYSTEMTIME st;

    VariantTimeToSystemTime(dt, &st);
    SystemTimeToFileTime(&st, &ft);
}

void FileTimeToVariantTime(FILETIME& ft, double *pdt)
{
    SYSTEMTIME st;

    FileTimeToSystemTime(&ft, &st);
    SystemTimeToVariantTime(&st, pdt);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// Cache helper functions
//

// Caller should MemFree *lpCacheConfigInfo when done. Should pass *lpCacheConfigInfo
//  into SetCacheSize
HRESULT GetCacheInfo(
    LPINTERNET_CACHE_CONFIG_INFOA *lplpCacheConfigInfo,
    DWORD                        *pdwSizeInKB,
    DWORD                        *pdwPercent)
{
    HRESULT hr = S_OK;
    LPINTERNET_CACHE_CONFIG_INFOA lpCCI = NULL;
    DWORD dwSize;

    dwSize = sizeof(INTERNET_CACHE_CONFIG_INFOA);

    lpCCI = (LPINTERNET_CACHE_CONFIG_INFOA)MemAlloc(LPTR, dwSize);

    if (!lpCCI)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    lpCCI->dwStructSize = sizeof(INTERNET_CACHE_CONFIG_INFOA);

    if (!GetUrlCacheConfigInfoA(lpCCI, &dwSize, CACHE_CONFIG_CONTENT_PATHS_FC))
    {
        hr = E_FAIL; // HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    // there should be at least one cache path structure
    if (dwSize < sizeof(INTERNET_CACHE_CONFIG_INFOA) ||
        lpCCI->dwNumCachePaths != 1)
    {
        // something is screwed up
        hr = E_FAIL;
        goto cleanup;
    }

    *lplpCacheConfigInfo = lpCCI;
    *pdwSizeInKB = lpCCI->dwQuota;
    *pdwPercent = 10; // good faith estimate

    ASSERT(*pdwSizeInKB);   // Better not be 0...

cleanup:

    if (FAILED(hr))
    {
        SAFELOCALFREE(lpCCI);
    }

    return hr;
}

HRESULT SetCacheSize(
            LPINTERNET_CACHE_CONFIG_INFOA lpCacheConfigInfo,
            DWORD                        dwSizeInKB)
{
//  lpCacheConfigInfo->dwNumCachePaths = 1;
//  lpCacheConfigInfo->CachePaths[0].dwCacheSize = dwSizeInKB;
    lpCacheConfigInfo->dwContainer = 0; // CONTENT;
    lpCacheConfigInfo->dwQuota = dwSizeInKB;

    if (!SetUrlCacheConfigInfoA(lpCacheConfigInfo, CACHE_CONFIG_QUOTA_FC))
    {
        return E_FAIL; // HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// Registry helper functions
//
BOOL ReadRegValue(HKEY hkeyRoot, const TCHAR *pszKey, const TCHAR *pszValue,
                   void *pData, DWORD dwBytes)
{
    long    lResult;
    HKEY    hkey;
    DWORD   dwType;

    lResult = RegOpenKey(hkeyRoot, pszKey, &hkey);
    if (lResult != ERROR_SUCCESS) {
        return FALSE;
    }

    lResult = RegQueryValueEx(hkey, pszValue, NULL, &dwType, (BYTE *)pData,
        &dwBytes);
    RegCloseKey(hkey);

    if (lResult != ERROR_SUCCESS)
        return FALSE;

    if(dwType == REG_SZ) {
        // null terminate string
        ((TCHAR *)pData)[dwBytes] = 0;
    }

    return TRUE;
}

BOOL WriteRegValue(HKEY hkeyRoot, const TCHAR *pszKey, const TCHAR *pszValue,
                    void *pData, DWORD dwBytes, DWORD dwType)
{
    HKEY    hkey;
    long    lResult;
    DWORD   dwStatus;

    lResult = RegCreateKeyEx(hkeyRoot, pszKey, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkey, &dwStatus);
    if (lResult != ERROR_SUCCESS) {
        return FALSE;
    }

    lResult = RegSetValueEx(hkey, pszValue, 0, dwType, (BYTE *)pData, dwBytes);
    RegCloseKey(hkey);

    return (lResult == ERROR_SUCCESS) ? TRUE : FALSE;
}

DWORD ReadRegDWORD(HKEY hkeyRoot, const TCHAR *pszKey, const TCHAR *pszValue)
{
    DWORD dwData;
    if (ReadRegValue(hkeyRoot, pszKey, pszValue, &dwData, sizeof(dwData)))
        return dwData;
    else
        return 0;
}

HRESULT CreateShellFolderPath(LPCTSTR pszPath, LPCTSTR pszGUID, BOOL bUICLSID)
{
    if (!PathFileExists(pszPath))
    CreateDirectory(pszPath, NULL);

    // Mark the folder as a system directory
    if (SetFileAttributes(pszPath, FILE_ATTRIBUTE_READONLY))
    {
        TCHAR szDesktopIni[MAX_PATH];
        // Write in the desktop.ini the cache folder class ID
        PathCombine(szDesktopIni, pszPath, TEXT("desktop.ini"));

        // If the desktop.ini already exists, make sure it is writable
        if (PathFileExists(szDesktopIni))
            SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_NORMAL);

        // (First, flush the cache to make sure the desktop.ini
        // file is really created.)
        WritePrivateProfileString(NULL, NULL, NULL, szDesktopIni);
        WritePrivateProfileString(TEXT(".ShellClassInfo"), bUICLSID ? TEXT("UICLSID") : TEXT("CLSID"), pszGUID, szDesktopIni);
        WritePrivateProfileString(NULL, NULL, NULL, szDesktopIni);

        // Hide the desktop.ini since the shell does not selectively
        // hide it.
        SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_HIDDEN);

        return NOERROR;
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("Cannot make %s a system folder"), pszPath);
        return E_FAIL;
    }
}

void CleanupShellFolder(LPCTSTR pszPath)
{
    if (PathFileExists(pszPath))
    {
        TCHAR szDesktopIni[MAX_PATH];

        // make the history a normal folder
        SetFileAttributes(pszPath, FILE_ATTRIBUTE_NORMAL);
        PathCombine(szDesktopIni, pszPath, TEXT("desktop.ini"));

        // If the desktop.ini already exists, make sure it is writable
        if (PathFileExists(szDesktopIni))
        {
            SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(szDesktopIni);
        }

        // remove the history directory
        RemoveDirectory(pszPath);
    }
}

BOOL GetSubscriptionFolderPath(LPTSTR pszPath)
{
    DWORD dwDummy;
    HKEY hk;

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                        REGSTR_PATH_SUBSCRIPTION,
                                        0, TEXT(""),
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ|KEY_WRITE, NULL, &hk, &dwDummy))
    {
        DWORD cbData = MAX_PATH * sizeof(TCHAR);
        if (ERROR_SUCCESS != RegQueryValueEx(hk, REGSTR_VAL_DIRECTORY , NULL, NULL, (LPBYTE)pszPath, &cbData))
        {
            TCHAR szWindows[MAX_PATH];
            GetWindowsDirectory(szWindows, ARRAYSIZE(szWindows));
            PathCombine(pszPath, szWindows, TEXT("Offline Web Pages"));
        }
        RegCloseKey(hk);

        return TRUE;
    }
    return FALSE;
}

HRESULT GetChannelPath(LPCTSTR pszURL, LPTSTR pszPath, int cch,
                       IChannelMgrPriv** ppIChannelMgrPriv)
{
    ASSERT(pszURL);
    ASSERT(pszPath || 0 == cch);
    ASSERT(ppIChannelMgrPriv);

    HRESULT hr;
    BOOL    bCoinit = FALSE;

    hr = CoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER,
                          IID_IChannelMgrPriv, (void**)ppIChannelMgrPriv);

    if ((hr == CO_E_NOTINITIALIZED || hr == REGDB_E_IIDNOTREG) &&
        SUCCEEDED(CoInitialize(NULL)))
    {
        bCoinit = TRUE;
        hr = CoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER,
                          IID_IChannelMgrPriv, (void**)ppIChannelMgrPriv);
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(*ppIChannelMgrPriv);

        IChannelMgr* pIChannelMgr;

        hr = (*ppIChannelMgrPriv)->QueryInterface(IID_IChannelMgr,
                                                (void**)&pIChannelMgr);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIChannelMgr);

            WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
            MyStrToOleStrN(wszURL, ARRAYSIZE(wszURL), pszURL);

            IEnumChannels* pIEnumChannels;

            hr = pIChannelMgr->EnumChannels(CHANENUM_ALLFOLDERS | CHANENUM_PATH,
                                            wszURL, &pIEnumChannels);

            if (SUCCEEDED(hr))
            {
                ASSERT(pIEnumChannels);

                CHANNELENUMINFO ci;

                if (S_OK == pIEnumChannels->Next(1, &ci, NULL))
                {
                    MyOleStrToStrN(pszPath, cch, ci.pszPath);

                    CoTaskMemFree(ci.pszPath);
                }
                else
                {
                    hr = E_FAIL;
                }

                pIEnumChannels->Release();
            }

            pIChannelMgr->Release();
        }

    }

    if (bCoinit)
        CoUninitialize();

    ASSERT((SUCCEEDED(hr) && *ppIChannelMgrPriv) || FAILED(hr));

    return hr;
}


//  Caller is responsible for calling ILFree on *ppidl.
HRESULT ConvertPathToPidl(LPCTSTR path, LPITEMIDLIST * ppidl)
{
    WCHAR wszPath[MAX_PATH];
    IShellFolder * pDesktopFolder;
    HRESULT hr;

    ASSERT(path && ppidl);
    * ppidl = NULL;

    MyStrToOleStrN(wszPath, MAX_PATH, path);
    hr = SHGetDesktopFolder(&pDesktopFolder);
    if (hr != NOERROR)
        return hr;

    ULONG uChEaten;

    hr = pDesktopFolder->ParseDisplayName(NULL, NULL, wszPath,
                                            &uChEaten, ppidl, NULL);
    SAFERELEASE(pDesktopFolder);

    return hr;
}

LPITEMIDLIST    GetSubscriptionFolderPidl(void)
{
    TCHAR szPath[MAX_PATH];
    static LPITEMIDLIST pidlFolder = NULL;  //  We leak here.

    if (!pidlFolder)  {
        if (!(GetSubscriptionFolderPath(szPath)))
            return NULL;
        if (FAILED(ConvertPathToPidl(szPath, &pidlFolder)))
            return NULL;
        ASSERT(pidlFolder);
    }
    return (LPITEMIDLIST)pidlFolder;
}

STDAPI OfflineFolderRegisterServer(void)
{
    TCHAR szOldSubscriptionPath[MAX_PATH];

    GetWindowsDirectory(szOldSubscriptionPath, ARRAYSIZE(szOldSubscriptionPath));
    PathCombine(szOldSubscriptionPath, szOldSubscriptionPath, TEXT("Subscriptions"));
    CleanupShellFolder(szOldSubscriptionPath);

    TCHAR szPath[MAX_PATH];

    if (!(GetSubscriptionFolderPath(szPath)))
        goto CleanUp;

    // we pass FALSE because history folder uses CLSID
    if (FAILED(CreateShellFolderPath(szPath, TEXT("{F5175861-2688-11d0-9C5E-00AA00A45957}"), FALSE)))
        goto CleanUp;

    return NOERROR;

CleanUp:        // cleanup stuff if any of our reg stuff fails

    return E_FAIL;
}

STDAPI OfflineFolderUnregisterServer(void)
{
    TCHAR szPath[MAX_PATH];

    if (!(GetSubscriptionFolderPath(szPath)))
        goto CleanUp;

    // we pass FALSE because history folder uses CLSID
    CleanupShellFolder(szPath);

    return NOERROR;

CleanUp:        // cleanup stuff if any of our reg stuff fails

    return E_FAIL;
}



HMENU LoadPopupMenu(UINT id, UINT uSubOffset)
{
    HMENU hmParent, hmPopup;

    hmParent = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(id));
    if (!hmParent)
        return NULL;

    hmPopup = GetSubMenu(hmParent, uSubOffset);
    RemoveMenu(hmParent, uSubOffset, MF_BYPOSITION);
    DestroyMenu(hmParent);

    return hmPopup;
}

UINT MergePopupMenu(HMENU *phMenu, UINT idResource, UINT uSubOffset, UINT indexMenu,  UINT idCmdFirst, UINT idCmdLast)
{
    HMENU hmMerge;

    if (*phMenu == NULL)
    {
        *phMenu = CreatePopupMenu();
        if (*phMenu == NULL)
            return 0;

        indexMenu = 0;    // at the bottom
    }

    hmMerge = LoadPopupMenu(idResource, uSubOffset);
    if (!hmMerge)
        return 0;

    idCmdLast = Shell_MergeMenus(*phMenu, hmMerge, indexMenu, idCmdFirst, idCmdLast, MM_ADDSEPARATOR);

    DestroyMenu(hmMerge);
    return idCmdLast;
}

HMENU GetMenuFromID(HMENU hmenu, UINT idm)
{
    MENUITEMINFO mii = { sizeof(mii), MIIM_SUBMENU, 0, 0, 0, NULL, NULL, NULL, 0, NULL, 0 };
    GetMenuItemInfo(hmenu, idm, FALSE, &mii);
    return mii.hSubMenu;
}

UINT MergeMenuHierarchy(HMENU hmenuDst, HMENU hmenuSrc, UINT idcMin, UINT idcMax, BOOL bTop)
{
    UINT idcMaxUsed = idcMin;
    int imi = GetMenuItemCount(hmenuSrc);

    while (--imi >= 0)
    {
        MENUITEMINFO mii = {
                sizeof(MENUITEMINFO),
                MIIM_ID | MIIM_SUBMENU,
                0,/* fType */ 0,/* fState */ 0,/*wId*/ NULL,
                NULL, NULL, 0,
                NULL, 0 };

        if (GetMenuItemInfo(hmenuSrc, imi, TRUE, &mii))
        {
            UINT idcT = Shell_MergeMenus(
                            GetMenuFromID(hmenuDst, mii.wID),
                            mii.hSubMenu, (bTop)?0:1024, idcMin, idcMax,
                            MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);
            idcMaxUsed = max(idcMaxUsed, idcT);
        }
    }
    return idcMaxUsed;
}

///////////////////////////////////////////////////////////////////////////////
//
// Helper Fuctions for item.cpp and folder.cpp
//
///////////////////////////////////////////////////////////////////////////////

int _CompareURL(LPMYPIDL pooi1, LPMYPIDL pooi2)
{
    return UrlCompare(URL(&(pooi1->ooe)), URL(&(pooi2->ooe)), TRUE);
}

int _CompareShortName(LPMYPIDL pooi1, LPMYPIDL pooi2)
{
    return StrCmp(NAME(&(pooi1->ooe)), NAME(&(pooi2->ooe)));
}

int _CompareLastUpdate(LPMYPIDL pooi1, LPMYPIDL pooi2)
{
    if (pooi1->ooe.m_LastUpdated - pooi2->ooe.m_LastUpdated > 0)
        return 1;
    return -1;
}

int _CompareCookie(REFCLSID cookie1, REFCLSID cookie2)
{
    return memcmp(&cookie1, &cookie2, sizeof(CLSID));
}

int _CompareStatus(LPMYPIDL pooi1, LPMYPIDL pooi2)
{
    return StrCmp(STATUS(&(pooi1->ooe)), STATUS(&(pooi2->ooe)));
}

int _CompareIdentities(LPMYPIDL pooi1, LPMYPIDL pooi2)
{
    if (pooi1->ooe.clsidDest != pooi2->ooe.clsidDest)
        return -1;

    if (!IsNativeAgent(pooi1->ooe.clsidDest))
        return _CompareCookie(pooi1->ooe.m_Cookie, pooi2->ooe.m_Cookie);

    return _CompareURL(pooi1, pooi2);
}

BOOL _ValidateIDListArray(UINT cidl, LPCITEMIDLIST *ppidl)
{
    UINT i;

    for (i = 0; i < cidl; i++)
    {
        if (!IS_VALID_MYPIDL(ppidl[i]))
            return FALSE;
    }

    return TRUE;
}

int _LaunchApp(HWND hwnd, LPCTSTR pszPath)
{
    SHELLEXECUTEINFO ei = { 0 };

    ei.cbSize           = sizeof(SHELLEXECUTEINFO);
    ei.hwnd             = hwnd;
    ei.lpFile           = pszPath;
    ei.nShow            = SW_SHOWNORMAL;

    return ShellExecuteEx(&ei);
}

void _GenerateEvent(LONG lEventId, LPITEMIDLIST pidlIn, LPITEMIDLIST pidlNewIn, BOOL bRefresh)
{
    LPITEMIDLIST pidlFolder = GetSubscriptionFolderPidl();
    if (!pidlFolder)
        return;

    LPITEMIDLIST pidl = ILCombine(pidlFolder, pidlIn);
    if (pidl)
    {
        if (pidlNewIn)
        {
            LPITEMIDLIST pidlNew = ILCombine(pidlFolder, pidlNewIn);
            if (pidlNew)
            {
                SHChangeNotify(lEventId, SHCNF_IDLIST, pidl, pidlNew);
                ILFree(pidlNew);
            }
        }
        else
        {
            SHChangeNotify(lEventId, SHCNF_IDLIST, pidl, NULL);
        }
        if (bRefresh)
            SHChangeNotifyHandleEvents();
        ILFree(pidl);
    }
}

BOOL _InitComCtl32()
{
    static BOOL fInitialized = FALSE;

    if (!fInitialized)
    {
        INITCOMMONCONTROLSEX icc;

        icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icc.dwICC = ICC_NATIVEFNTCTL_CLASS | ICC_DATE_CLASSES;
        fInitialized = InitCommonControlsEx(&icc);
    }
    return fInitialized;
}

const struct {
    LPCSTR pszVerb;
    UINT idCmd;
} rgcmds[] = {
    { c_szOpen,         RSVIDM_OPEN },
    { c_szCopy,         RSVIDM_COPY },
    { c_szRename,       RSVIDM_RENAME},
    { c_szPaste,        RSVIDM_PASTE},
    { c_szDelete,       RSVIDM_DELETE },
    { c_szProperties,   RSVIDM_PROPERTIES }
};

int _GetCmdID(LPCSTR pszCmd)
{
    if (HIWORD(pszCmd))
    {
        int i;
        for (i = 0; i < ARRAYSIZE(rgcmds); i++)
        {
            if (lstrcmpiA(rgcmds[i].pszVerb, pszCmd) == 0)
            {
                return rgcmds[i].idCmd;
            }
        }

        return -1;  // unknown
    }
    return (int)LOWORD(pszCmd);
}

BOOL CALLBACK _AddOnePropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *) lParam;

    if (ppsh->nPages < MAX_PROP_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }
    return FALSE;
}

HRESULT _CreatePropSheet(HWND hwnd, POOEBuf pBuf)
{
    ASSERT(pBuf);

    ISubscriptionMgr    * pSub= NULL;
    HRESULT hr = CoInitialize(NULL);
    RETURN_ON_FAILURE(hr);

    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER,
                IID_ISubscriptionMgr, (void **)&pSub);
    CoUninitialize();
    RETURN_ON_FAILURE(hr);
    ASSERT(pSub);

    BSTR bstrURL = NULL;
    hr = CreateBSTRFromTSTR(&bstrURL, pBuf->m_URL);
    if (S_OK == hr)
        hr = pSub->ShowSubscriptionProperties(bstrURL, hwnd);
    SAFERELEASE(pSub);
    SAFEFREEBSTR(bstrURL);
    return hr;
}

//  Note:
//      We return FALSE on illegal DATE data.

BOOL DATE2DateTimeString(CFileTime& ft, LPTSTR pszText)
{
    SYSTEMTIME st;
    HRESULT hr;

    if (ft == 0)    {
        if (szUnknown[0] == 0)
            MLLoadString(IDS_UNKNOWN, szUnknown, ARRAYSIZE(szUnknown));

        StrCpy(pszText, szUnknown);
        return FALSE;
    }

    hr = FileTimeToSystemTime(&ft, &st);
    if (!hr)
    {
        if (szNone[0] == 0)
            MLLoadString(IDS_NONE, szNone, ARRAYSIZE(szNone));

        StrCpy(pszText, szNone);
        return FALSE;
    }
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, pszText, 64);
    pszText += lstrlen(pszText);
    *pszText++ = ' ';
    GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, pszText, 64);
    return TRUE;
}

BOOL Date2LocalDateString(SYSTEMTIME * st, LPTSTR dtStr, int size)
{
    ASSERT(dtStr);

    return GetDateFormat(LOCALE_USER_DEFAULT, 0, st, NULL, dtStr, size);
}

void CopyToOOEBuf(POOEntry pooe, POOEBuf pBuf)
{
    ASSERT(pooe);
    ASSERT(pBuf);

    pBuf->dwFlags           = pooe->dwFlags;
    pBuf->m_LastUpdated     = pooe->m_LastUpdated;
    pBuf->m_NextUpdate      = pooe->m_NextUpdate;
    pBuf->m_SizeLimit       = pooe->m_SizeLimit;
    pBuf->m_ActualSize      = pooe->m_ActualSize;
    pBuf->m_RecurseLevels   = pooe->m_RecurseLevels;
    pBuf->m_RecurseFlags    = pooe->m_RecurseFlags;
    pBuf->m_Priority        = pooe->m_Priority;
    pBuf->bDesktop          = pooe->bDesktop;
    pBuf->bChannel          = pooe->bChannel;
    pBuf->bMail             = pooe->bMail;
    pBuf->bGleam            = pooe->bGleam;
    pBuf->bChangesOnly      = pooe->bChangesOnly;
    pBuf->fChannelFlags     = pooe->fChannelFlags;
    pBuf->bNeedPassword     = pooe->bNeedPassword;
    pBuf->m_Cookie          = pooe->m_Cookie;
    pBuf->groupCookie       = pooe->groupCookie;
    pBuf->grfTaskTrigger    = pooe->grfTaskTrigger;
    pBuf->m_Trigger         = pooe->m_Trigger;
    pBuf->clsidDest         = pooe->clsidDest;
    pBuf->status            = pooe->status;

    StrCpyN(pBuf->m_URL,       URL(pooe),      MAX_URL);
    StrCpyN(pBuf->m_Name,      NAME(pooe),     MAX_NAME);
    StrCpyN(pBuf->username,    UNAME(pooe),    MAX_USERNAME);
    StrCpyN(pBuf->password,    PASSWD(pooe),   MAX_PASSWORD);
    StrCpyN(pBuf->statusStr,   STATUS(pooe),   MAX_STATUS);
}

void CopyToMyPooe(POOEBuf pBuf, POOEntry pooe)
{
    UINT    offset = sizeof(OOEntry);
    UINT    srcLen = lstrlen(pBuf->m_URL) + 1;

    ASSERT(pooe);
    ASSERT(pBuf);

    pooe->dwFlags           = pBuf->dwFlags;
    pooe->m_LastUpdated     = pBuf->m_LastUpdated;
    pooe->m_NextUpdate      = pBuf->m_NextUpdate;
    pooe->m_SizeLimit       = pBuf->m_SizeLimit;
    pooe->m_ActualSize      = pBuf->m_ActualSize;
    pooe->m_RecurseLevels   = pBuf->m_RecurseLevels;
    pooe->m_Priority        = pBuf->m_Priority;
    pooe->m_RecurseFlags    = pBuf->m_RecurseFlags;
    pooe->bDesktop          = pBuf->bDesktop;
    pooe->bChannel          = pBuf->bChannel;
    pooe->bMail             = pBuf->bMail;
    pooe->bGleam            = pBuf->bGleam;
    pooe->bChangesOnly      = pBuf->bChangesOnly;
    pooe->fChannelFlags     = pBuf->fChannelFlags;
    pooe->bNeedPassword     = pBuf->bNeedPassword;
    pooe->m_Cookie          = pBuf->m_Cookie;
    pooe->groupCookie       = pBuf->groupCookie;
    pooe->m_Trigger         = pBuf->m_Trigger;
    pooe->grfTaskTrigger    = pBuf->grfTaskTrigger;
    pooe->clsidDest         = pBuf->clsidDest;
    pooe->status            = pBuf->status;

    pooe->m_URL = (LPTSTR)((LPBYTE)pooe + offset);
    srcLen = lstrlen(pBuf->m_URL) + 1;
    StrCpyN(pooe->m_URL, pBuf->m_URL, srcLen);
    offset += srcLen * sizeof (TCHAR);
    pooe->m_URL = (LPTSTR) PtrDifference(pooe->m_URL, pooe);

    pooe->m_Name = (LPTSTR)((LPBYTE)pooe + offset);
    srcLen = lstrlen(pBuf->m_Name) + 1;
    StrCpyN(pooe->m_Name, pBuf->m_Name, srcLen);
    offset += srcLen * sizeof (TCHAR);
    pooe->m_Name = (LPTSTR) PtrDifference(pooe->m_Name, pooe);

    pooe->username = (LPTSTR)((LPBYTE)pooe + offset);
    srcLen = lstrlen(pBuf->username) + 1;
    StrCpyN(pooe->username, pBuf->username, srcLen);
    offset += srcLen * sizeof (TCHAR);
    pooe->username = (LPTSTR) PtrDifference(pooe->username, pooe);

    pooe->password = (LPTSTR)((LPBYTE)pooe + offset);
    srcLen = lstrlen(pBuf->password) + 1;
    StrCpyN(pooe->password, pBuf->password, srcLen);
    offset += srcLen * sizeof (TCHAR);
    pooe->password = (LPTSTR) PtrDifference(pooe->password, pooe);

    pooe->statusStr = (LPTSTR)((LPBYTE)pooe + offset);
    srcLen = lstrlen(pBuf->statusStr) + 1;
    StrCpyN(pooe->statusStr, pBuf->statusStr, srcLen);
    offset += srcLen * sizeof (TCHAR);
    pooe->statusStr = (LPTSTR) PtrDifference(pooe->statusStr, pooe);

    pooe->dwSize = offset;
}

UINT BufferSize(POOEBuf pBuf)
{
    UINT strLen = 0;
    ASSERT(pBuf);

    strLen += lstrlen(pBuf->m_URL)      + 1;
    strLen += lstrlen(pBuf->m_Name)     + 1;
    strLen += lstrlen(pBuf->username)   + 1;
    strLen += lstrlen(pBuf->password)   + 1;
    strLen += lstrlen(pBuf->statusStr)  + 1;

    return strLen * sizeof(TCHAR);
}


typedef struct
{
    int cItems;
    LPCTSTR pszName;
    LPCTSTR pszUrl;
} DELETE_CONFIRM_INFO;

INT_PTR CALLBACK ConfirmDeleteDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch(message) {

        case WM_INITDIALOG:
        {
            DELETE_CONFIRM_INFO* pInfo = (DELETE_CONFIRM_INFO*)lParam;
            ASSERT (pInfo);
            ASSERT(pInfo->cItems == 1);

            SetListViewToString (GetDlgItem (hDlg, IDC_NAME), pInfo->pszName);
            SetListViewToString (GetDlgItem (hDlg, IDC_LOCATION), pInfo->pszUrl);
        }
        break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDYES:
                case IDNO:
                case IDCANCEL:
                    EndDialog(hDlg, wParam);
                    break;
            }
            break;

        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_LOCATION)
            {
                NM_LISTVIEW * pnmlv = (NM_LISTVIEW *)lParam;
                if (pnmlv->hdr.code == LVN_GETINFOTIP)
                {
                    TCHAR szURL[MAX_URL];
                    LV_ITEM lvi = {0};
                    lvi.mask = LVIF_TEXT;
                    lvi.pszText = szURL;
                    lvi.cchTextMax = ARRAYSIZE(szURL);
                    if (!ListView_GetItem (GetDlgItem (hDlg, IDC_LOCATION), &lvi))
                        return FALSE;

                    NMLVGETINFOTIP  * pTip = (NMLVGETINFOTIP *)pnmlv;
                    ASSERT(pTip);
                    StrCpyN(pTip->pszText, szURL, pTip->cchTextMax);
                    return TRUE;
                }
            }
        return FALSE;

        default:
            return FALSE;

    } // end of switch

    return TRUE;
}

BOOL ConfirmDelete(HWND hwnd, UINT cItems, LPMYPIDL * ppidl)
{
    ASSERT(ppidl);
    INT_PTR iRet;

    // Check if the user is restricted from deleting URLs.
    // If they're deleting multiple, we'll fail if any can fail.
    UINT i;
    for (i = 0; i < cItems; i++)
    {
        if (ppidl[i]->ooe.bChannel)
        {
            if (SHRestricted2(REST_NoRemovingChannels, URL(&(ppidl[i]->ooe)), 0))
            {
                if (IsWindow(hwnd))
                    SGMessageBox(hwnd, IDS_RESTRICTED, MB_OK);
                return FALSE;
            }
        }

        if (!ppidl[i]->ooe.bDesktop)
        {
            // BUGBUG: What about desktop components?
            if (SHRestricted2(REST_NoRemovingSubscriptions, URL(&(ppidl[i]->ooe)), 0))
            {
                if (IsWindow(hwnd))
                    SGMessageBox(hwnd, IDS_RESTRICTED, MB_OK);
                return FALSE;
            }
        }
    }

    if (IsWindow(hwnd)) {
        DELETE_CONFIRM_INFO dci = {0};
        dci.cItems = cItems;
        if (cItems == 1)
        {
            dci.pszName = NAME(&(ppidl[0]->ooe));
            dci.pszUrl = URL(&(ppidl[0]->ooe));
            iRet = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_OBJECTDEL_WARNING),
                        hwnd, ConfirmDeleteDlgProc, (LPARAM)&dci);
        }
        else
        {

            TCHAR szFormat[200];
            //  Enough room for format string and int as string
            TCHAR szBuf[ARRAYSIZE(szFormat) + 11];

            MLLoadString(IDS_DEL_MULTIPLE_FMT, szFormat, ARRAYSIZE(szFormat));
            wnsprintf(szBuf, ARRAYSIZE(szBuf), szFormat, cItems);

            MLLoadString(IDS_DELETE_CAPTION, szFormat, ARRAYSIZE(szFormat));

            MSGBOXPARAMS mbp;

            mbp.cbSize = sizeof(MSGBOXPARAMS);
            mbp.hwndOwner = hwnd;
            mbp.hInstance = MLGetHinst();
            mbp.lpszText = szBuf;
            mbp.lpszCaption = szFormat;
            mbp.dwStyle = MB_YESNO | MB_USERICON;
            mbp.lpszIcon = MAKEINTRESOURCE(IDI_OBJECTDELETED);
            iRet = MessageBoxIndirect(&mbp);
        }
        if (iRet == IDYES)
            return TRUE;
        return FALSE;
    } else  {
        return TRUE;
    }
}

BOOL IsHTTPPrefixed(LPCTSTR szURL)
{
    TCHAR szCanonicalURL[MAX_URL];
    DWORD dwSize = MAX_URL;
    URL_COMPONENTS uc;

    memset(&uc, 0, sizeof(URL_COMPONENTS));
    uc.dwStructSize = sizeof(URL_COMPONENTS);

    // Note:  We explicitly check for and allow the "about:home" URL to pass through here.  This allows
    // the Active Desktop "My Current Home Page" component to specify that URL when creating and managing
    // it's subscription which is consistent with it's use of that form in the browser.
    if (!InternetCanonicalizeUrl(szURL, szCanonicalURL, &dwSize, ICU_DECODE) ||
        !InternetCrackUrl(szCanonicalURL, 0, 0, &uc) ||
        ((INTERNET_SCHEME_HTTP != uc.nScheme) && (INTERNET_SCHEME_HTTPS != uc.nScheme) && (0 != StrCmpI(TEXT("about:home"), szURL))))
    {
        return FALSE;
    }
    return TRUE;
}

//   Checks if global state is offline

BOOL IsGlobalOffline(void)
{
    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;
    HANDLE hModuleHandle = LoadLibraryA("wininet.dll");

    if(!hModuleHandle)
        return FALSE;

    if(InternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState,
        &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    return fRet;
}

void SetGlobalOffline(BOOL fOffline)
{
    INTERNET_CONNECTED_INFO ci;

    memset(&ci, 0, sizeof(ci));
    if(fOffline) {
        ci.dwConnectedState = INTERNET_STATE_DISCONNECTED_BY_USER;
        ci.dwFlags = ISO_FORCE_DISCONNECTED;
    } else {
        ci.dwConnectedState = INTERNET_STATE_CONNECTED;
    }

    InternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));
}


//helper function to create one column in a ListView control, add one item to that column,
//size the column to the width of the control, and color the control like a static...
//basically, like SetWindowText for a ListView.  Because we use a lot of ListViews to display
//urls that would otherwise be truncated... the ListView gives us automatic ellipsis and ToolTip.
void SetListViewToString (HWND hLV, LPCTSTR pszString)
{
    ASSERT(hLV);

    LV_COLUMN   lvc = {0};
    RECT lvRect;
    GetClientRect (hLV, &lvRect);
    lvc.mask = LVCF_WIDTH;
    lvc.cx = lvRect.right - lvRect.left;
    if (-1 == ListView_InsertColumn(hLV, 0, &lvc))   {
        ASSERT(0);
    }

    SendMessage(hLV, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_INFOTIP, LVS_EX_INFOTIP);

    LV_ITEM lvi = {0};
    lvi.iSubItem = 0;
    lvi.pszText = (LPTSTR)pszString;
    lvi.mask = LVIF_TEXT;
    ListView_InsertItem(hLV, &lvi);
    ListView_EnsureVisible(hLV, 0, TRUE);

    ListView_SetBkColor(hLV, GetSysColor(COLOR_BTNFACE));
    ListView_SetTextBkColor(hLV, GetSysColor(COLOR_BTNFACE));
}

int WCMessageBox(HWND hwnd, UINT idTextFmt, UINT idCaption, UINT uType, ...)
{
    TCHAR szCaption[256];
    TCHAR szTextFmt[512];
    LPTSTR pszText;
    int result;
    va_list va;

    va_start(va, uType);

    szCaption[0] = 0;

    MLLoadString(idTextFmt, szTextFmt, ARRAYSIZE(szTextFmt));

    if (idCaption <= 0)
    {
        if (NULL != hwnd)
        {
            GetWindowText(hwnd, szCaption, ARRAYSIZE(szCaption));
        }

        //  This handles GetWindowText failure and a NULL hwnd
        if (0 == szCaption[0])
        {
            #if IDS_DEFAULT_MSG_CAPTION < 1
            #error IDS_DEFAULT_MSG_CAPTION is defined incorrectly
            #endif

            idCaption = IDS_DEFAULT_MSG_CAPTION;
        }
    }

    if (idCaption > 0)
    {
        MLLoadString(idCaption, szCaption, ARRAYSIZE(szCaption));
    }

    ASSERT(0 != szCaption[0]);

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                  szTextFmt, 0, 0, (LPTSTR)&pszText, 0, &va);

    result = MessageBox(hwnd, pszText, szCaption, uType);

    LocalFree(pszText);

    return result;
}


/////////////////////////////////////////////////////////////////////////////
// SGMessageBox
/////////////////////////////////////////////////////////////////////////////
int SGMessageBox
(
    HWND    hwndParent,
    UINT    idStringRes,
    UINT    uType
)
{
    ASSERT(hwndParent != NULL);
    ASSERT(IsWindow(hwndParent));

    TCHAR szError[512];
    if (!MLLoadString(idStringRes, szError, ARRAYSIZE(szError)))
        return 0;

    TCHAR szTitle[128];
    szTitle[0] = 0;

    if (hwndParent != NULL)
        GetWindowText(hwndParent, szTitle, ARRAYSIZE(szTitle));

    return MessageBox(  hwndParent,
                        szError,
                        ((hwndParent != NULL) ? szTitle : NULL),
                        uType);
}

#ifdef DEBUG
/////////////////////////////////////////////////////////////////////////////
// DumpTaskTrigger
/////////////////////////////////////////////////////////////////////////////
void DumpTaskTrigger
(
    TASK_TRIGGER * pTT
)
{
    TraceMsg(TF_DUMPTRIGGER, "----- BEGIN DumpTaskTrigger -----");

    TraceMsg(TF_DUMPTRIGGER, "cbTriggerSize = %d", pTT->cbTriggerSize);
    TraceMsg(TF_DUMPTRIGGER, "Reserved1 = %d", pTT->Reserved1);
    TraceMsg(TF_DUMPTRIGGER, "wBeginYear = %d", pTT->wBeginYear);
    TraceMsg(TF_DUMPTRIGGER, "wBeginMonth = %d", pTT->wBeginMonth);
    TraceMsg(TF_DUMPTRIGGER, "wBeginDay = %d", pTT->wBeginDay);
    TraceMsg(TF_DUMPTRIGGER, "wEndYear = %d", pTT->wEndYear);
    TraceMsg(TF_DUMPTRIGGER, "wEndMonth = %d", pTT->wEndMonth);
    TraceMsg(TF_DUMPTRIGGER, "wEndDay = %d", pTT->wEndDay);
    TraceMsg(TF_DUMPTRIGGER, "wStartHour = %d", pTT->wStartHour);
    TraceMsg(TF_DUMPTRIGGER, "wStartMinute = %d", pTT->wStartMinute);
    TraceMsg(TF_DUMPTRIGGER, "MinutesDuration = %d", pTT->MinutesDuration);
    TraceMsg(TF_DUMPTRIGGER, "MinutesInterval = %d", pTT->MinutesInterval);
    TraceMsg(TF_DUMPTRIGGER, "rgFlags = %d", pTT->rgFlags);
    TraceMsg(TF_DUMPTRIGGER, "Reserved2 = %d", pTT->Reserved2);
    TraceMsg(TF_DUMPTRIGGER, "wRandomMinutesInterval = %d", pTT->wRandomMinutesInterval);

    switch (pTT->TriggerType)
    {
        case TASK_TIME_TRIGGER_DAILY:
        {
            TraceMsg(TF_DUMPTRIGGER, "DAILY");
            TraceMsg(TF_DUMPTRIGGER, "DaysInterval = %d", pTT->Type.Daily.DaysInterval);
            break;
        }

        case TASK_TIME_TRIGGER_WEEKLY:
        {
            TraceMsg(TF_DUMPTRIGGER, "WEEKLY");
            TraceMsg(TF_DUMPTRIGGER, "WeeksInterval = %d", pTT->Type.Weekly.WeeksInterval);
            TraceMsg(TF_DUMPTRIGGER, "rgfDaysOfTheWeek = %d", pTT->Type.Weekly.rgfDaysOfTheWeek);
            break;
        }

        case TASK_TIME_TRIGGER_MONTHLYDATE:
        {
            TraceMsg(TF_DUMPTRIGGER, "MONTHLY DATE");
            TraceMsg(TF_DUMPTRIGGER, "rgfDays = %d", pTT->Type.MonthlyDate.rgfDays);
            TraceMsg(TF_DUMPTRIGGER, "rgfMonths = %d", pTT->Type.MonthlyDate.rgfMonths);
            break;
        }

        case TASK_TIME_TRIGGER_MONTHLYDOW:
        {
            TraceMsg(TF_DUMPTRIGGER, "MONTHLY DOW");
            TraceMsg(TF_DUMPTRIGGER, "wWhichWeek = %d", pTT->Type.MonthlyDOW.wWhichWeek);
            TraceMsg(TF_DUMPTRIGGER, "rgfDaysOfTheWeek = %d", pTT->Type.MonthlyDOW.rgfDaysOfTheWeek);
            TraceMsg(TF_DUMPTRIGGER, "rgfMonths = %d", pTT->Type.MonthlyDOW.rgfMonths);
            break;
        }

        default:
        {
            ASSERT(FALSE);
            break;
        }
    }

    TraceMsg(TF_DUMPTRIGGER, "-----  END DumpTaskTrigger  -----");
}
#endif  // DEBUG


//////////////////////////////////////////////////////////////////////
//
// memory leak detection helpers
//
//////////////////////////////////////////////////////////////////////

#ifdef DEBUG

extern "C" void *_ReturnAddress();

#pragma intrinsic(_ReturnAddress)

LEAKDETECTFUNCS LeakDetFunctionTable;
BOOL g_fInitTable = FALSE;
void *  __cdecl operator new( size_t nSize )
{
    LPVOID pv;

    if(!g_fInitTable)
    {
        if(GetLeakDetectionFunctionTable(&LeakDetFunctionTable))
            g_fInitTable = TRUE;
    }
    // Zero init just to save some headaches
    pv = ((LPVOID)LocalAlloc(LPTR, nSize));

    if(g_fInitTable)
        LeakDetFunctionTable.pfnadd_to_memlist( g_hInst, pv, nSize, DBGMEM_UNKNOBJ, "Webcheck", (INT_PTR)_ReturnAddress());

    return pv;
}

void  __cdecl operator delete(void *pv)
{
    if(!g_fInitTable)
    {
        if(GetLeakDetectionFunctionTable(&LeakDetFunctionTable))
            g_fInitTable = TRUE;
    }

    if (pv) {
        if(g_fInitTable)
            LeakDetFunctionTable.pfnremove_from_memlist(pv);
        memset(pv, 0xfe, (int)LocalSize((HLOCAL)pv));
        LocalFree((HLOCAL)pv);
    }
}

HLOCAL MemAlloc(IN UINT fuFlags, IN UINT cbBytes)
{
    HLOCAL hBlock;

    if(!g_fInitTable)
    {
        if(GetLeakDetectionFunctionTable(&LeakDetFunctionTable))
            g_fInitTable = TRUE;
    }
    // Zero init just to save some headaches
    hBlock = LocalAlloc(fuFlags, cbBytes);

    if(g_fInitTable)
        LeakDetFunctionTable.pfnadd_to_memlist( g_hInst, (LPVOID)hBlock, cbBytes, DBGMEM_MEMORY, "Webcheck", (INT_PTR)_ReturnAddress() );

    return hBlock;
}


HLOCAL MemFree(HLOCAL hMem)
{
    HLOCAL hRet = hMem;
    if(!g_fInitTable)
    {
        if(GetLeakDetectionFunctionTable(&LeakDetFunctionTable))
            g_fInitTable = TRUE;
    }

    if (hMem)
    {
        if(g_fInitTable)
            LeakDetFunctionTable.pfnremove_from_memlist(hMem);
        memset((LPVOID)hMem, 0xfe, (int)LocalSize(hMem));
        hRet = LocalFree(hMem);
    }
    return hRet;
}

HLOCAL MemReAlloc(HLOCAL hMem, UINT uBytes, UINT uFlags)
{
    HLOCAL hNew;

    if(!g_fInitTable)
    {
        if(GetLeakDetectionFunctionTable(&LeakDetFunctionTable))
            g_fInitTable = TRUE;
    }

    if (hMem)
    {
        if(g_fInitTable)
            LeakDetFunctionTable.pfnremove_from_memlist(hMem);
    }

    hNew = LocalReAlloc(hMem, uBytes, uFlags);

    if(g_fInitTable)
        LeakDetFunctionTable.pfnadd_to_memlist( g_hInst, (LPVOID)hNew, uBytes, DBGMEM_MEMORY, "Webcheck", (INT_PTR)_ReturnAddress());

    return hNew;
}


#else
#define CPP_FUNCTIONS
#include <crtfree.h>
#endif
