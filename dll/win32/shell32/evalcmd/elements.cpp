/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     AssocCreateElement
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#include <windef.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <undocshell.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shlwapi_undoc.h>
#include <pathcch.h>
#include <strsafe.h>
#include <shellutils.h>
#include <new>
#include "evalcmd.h"
#include "resource.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(evalcmd);

typedef HRESULT (CALLBACK* QUERY_CALLBACK)(IQuerySourceOld*, ASSOCQUERY query, PCWSTR keyName,
                                           PCWSTR valueName, PVOID pValue);

typedef struct QUERYKEYVAL
{
    ASSOCQUERY query;
    PCWSTR key;
    PCWSTR value;
} QUERYKEYVAL, *PQUERYKEYVAL;

#define ASSOCQUERY_DE    (ASSOCQUERY_DIRECT | ASSOCQUERY_EXISTS)
#define ASSOCQUERY_SDE   (ASSOCQUERY_STRING | ASSOCQUERY_DE)
#define ASSOCQUERY_SDED  (ASSOCQUERY_SDE | ASSOCQUERY_DWORD)
#define ASSOCQUERY_SDEI  (ASSOCQUERY_SDE | ASSOCQUERY_INDIRECT)
#define ASSOCQUERY_SDEEV (ASSOCQUERY_SDE | ASSOCQUERY_EXTRA_VERB)

#define ASSOCQUERY_DEFAULTICON (ASSOCQUERY_SDE | 0x1)
#define ASSOCQUERY_CLSID (ASSOCQUERY_SDE | 0x3)
#define ASSOCQUERY_PROGID (ASSOCQUERY_SDE | 0x4)
#define ASSOCQUERY_SHELLEX (ASSOCQUERY_SDE | ASSOCQUERY_FALLBACK | ASSOCQUERY_EXTRA_NON_VERB | 0x2)
#define ASSOCQUERY_NOACTIVATEHANDLER (ASSOCQUERY_DE | ASSOCQUERY_EXTRA_VERB | 0x5)
#define ASSOCQUERY_COMMAND_COMMAND (ASSOCQUERY_DE | ASSOCQUERY_EXTRA_VERB | 0x6)
#define ASSOCQUERY_FRIENDLYAPPNAME ASSOCQUERY_SDEI
#define ASSOCQUERY_VERB_FRIENDLYAPPNAME (ASSOCQUERY_SDEI | ASSOCQUERY_EXTRA_VERB | 0x8)
#define ASSOCQUERY_COMMAND ASSOCQUERY_SDEEV
#define ASSOCQUERY_DDEEXEC (ASSOCQUERY_SDEEV | 0x1)
#define ASSOCQUERY_DDEEXEC_IFEXEC (ASSOCQUERY_SDEEV | 0x2)
#define ASSOCQUERY_DDEEXEC_APPLICATION (ASSOCQUERY_SDEEV | 0x3)
#define ASSOCQUERY_DDEEXEC_TOPIC (ASSOCQUERY_SDEEV | 0x4)
#define ASSOCQUERY_LOCALIZEDSTRING (ASSOCQUERY_SDEI | ASSOCQUERY_EXTRA_NON_VERB | 0x1)
#define ASSOCQUERY_SEV_7 (ASSOCQUERY_STRING | ASSOCQUERY_EXTRA_VERB | 0x7)
#define ASSOCQUERY_APPKEY_FRIENDLYAPPNAME (ASSOCQUERY_SDEEV | ASSOCQUERY_INDIRECT | 0x8)
#define ASSOCQUERY_CONTENT_TYPE (ASSOCQUERY_SDE | ASSOCQUERY_FALLBACK | 0x2)
#define ASSOCQUERY_DELEGATE (ASSOCQUERY_STRING | ASSOCQUERY_EXTRA_VERB | 0x7)
#define ASSOCQUERY_OBJECT_EV_1 (ASSOCQUERY_OBJECT | ASSOCQUERY_EXTRA_VERB | 0x1)

static const QUERYKEYVAL g_perceivedKeyVals[] =
{
    { ASSOCQUERY_FRIENDLYAPPNAME, NULL, L"FriendlyTypeName" },
    { ASSOCQUERY_DEFAULTICON, L"DefaultIcon", NULL },
};

static const QUERYKEYVAL g_shellKeyVals[] =
{
    { ASSOCQUERY_FRIENDLYAPPNAME, NULL, L"FriendlyTypeName" },
    { ASSOCQUERY_DEFAULTICON, L"DefaultIcon", NULL },
    { ASSOCQUERY_CLSID, L"Clsid", NULL },
    { ASSOCQUERY_PROGID, L"Progid", NULL },
    { ASSOCQUERY_SHELLEX, L"ShellEx\\%s", NULL },
};

static const QUERYKEYVAL g_shellVerbKeyVals[] =
{
    { ASSOCQUERY_COMMAND, L"command", NULL },
    { ASSOCQUERY_DDEEXEC, L"ddeexec", NULL },
    { ASSOCQUERY_DDEEXEC_IFEXEC, L"ddeexec\\ifexec", NULL },
    { ASSOCQUERY_DDEEXEC_APPLICATION, L"ddeexec\\application", NULL },
    { ASSOCQUERY_DDEEXEC_TOPIC, L"ddeexec\\topic", NULL },
    { ASSOCQUERY_NOACTIVATEHANDLER, L"ddeexec", L"NoActivateHandler" },
    { ASSOCQUERY_COMMAND_COMMAND, L"command", L"command" },
    { ASSOCQUERY_VERB_FRIENDLYAPPNAME, NULL, L"FriendlyAppName" },
};

static const QUERYKEYVAL g_shellAppKeyVals[] =
{
    { ASSOCQUERY_APPKEY_FRIENDLYAPPNAME, NULL, L"FriendlyAppName" },
};

static const QUERYKEYVAL g_progidKeyVals[2] =
{
    { ASSOCQUERY_SHELLEX, L"ShellEx\\%s", NULL },
    { ASSOCQUERY_CONTENT_TYPE, NULL, L"Content Type" },
};

static const QUERYKEYVAL*
_FindKeyVal(ASSOCQUERY query, const QUERYKEYVAL* pItems, UINT cItems)
{
    for (UINT iItem = 0; iItem < cItems; ++iItem)
    {
        if (pItems[iItem].query == query)
            return &pItems[iItem];
    }
    return NULL;
}

static HRESULT SHAllocMUI(PWSTR* ppwsz)
{
    WCHAR szOutBuf[1024];
    HRESULT hr = SHLoadIndirectString(*ppwsz, szOutBuf, _countof(szOutBuf), NULL);
    CoTaskMemFree(*ppwsz);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        *ppwsz = NULL;
        return hr;
    }
    return SHStrDupW(szOutBuf, ppwsz);
}

static inline HRESULT _SHAllocLoadString(HINSTANCE hInstance, UINT uID, PWSTR* ppwsz)
{
    WCHAR szBuff[MAX_PATH];
    LoadStringW(hInstance, uID, szBuff, _countof(szBuff));
    return SHStrDupW(szBuff, ppwsz);
}

static HRESULT QSOpen2(
    IQuerySourceOld* pSource,
    PCWSTR key,
    PCWSTR pszSrc,
    BOOL bCreate,
    IQuerySourceOld** ppSource)
{
    WCHAR szDest[MAX_PATH];
    PathCchCombineEx(szDest, _countof(szDest), key, pszSrc, PATHCCH_NONE);
    return pSource->OpenSource(szDest, bCreate, ppSource);
}

static HRESULT _GetFileTypeName(PWSTR lpsz, PWSTR* ppwsz)
{
    WCHAR szDest[MAX_PATH], szBuff[128];
    HINSTANCE hSHLWAPI = GetModuleHandleW(L"shlwapi.dll");

    if (!lpsz || *lpsz != '.' || !lpsz[1])
        return _SHAllocLoadString(hSHLWAPI, IDS_FILE, ppwsz);

    CharUpperW(lpsz);
    LoadStringW(hSHLWAPI, IDS_ANY_FILE, szBuff, _countof(szBuff));
    StringCchPrintfW(szDest, _countof(szDest), szBuff, lpsz + 1);
    return SHStrDupW(szDest, ppwsz);
}

static inline LSTATUS
_RegQueryString(HKEY hKey, PCWSTR lpSubKey, PWSTR lpData, LONG cchData)
{
    LONG cbData = cchData * sizeof(WCHAR);
    return RegQueryValueW(hKey, lpSubKey, lpData, &cbData);
}

static LSTATUS
_RegSetVolatileString(HKEY hKey, PCWSTR lpSubKey, PCWSTR lpString)
{
    LSTATUS error;
    if (lpSubKey && *lpSubKey)
        error = RegCreateKeyExW(hKey, lpSubKey, 0, NULL, REG_OPTION_VOLATILE, KEY_WRITE,
                                NULL, &hKey, NULL);
    else
        error = RegOpenKeyExW(hKey, NULL, 0, KEY_WRITE, &hKey);
    if (error)
        return error;

    INT cch = lstrlenW(lpString);
    error = RegSetValueExW(hKey, NULL, 0, REG_SZ, (PBYTE)lpString, (cch + 1) * sizeof(WCHAR));
    RegCloseKey(hKey);
    return error;
}

static HRESULT CALLBACK
_QuerySourceDirect(
    IQuerySourceOld* pSource,
    ASSOCQUERY query,
    PCWSTR keyName,
    PCWSTR valueName,
    PVOID pValue)
{
    return pSource->QueryValueDirect(keyName, valueName, (FLAGGED_BYTE_BLOB**)pValue);
}

static HRESULT CALLBACK
_QuerySourceDword(
    IQuerySourceOld* pSource,
    ASSOCQUERY query,
    PCWSTR keyName,
    PCWSTR valueName,
    PVOID pValue)
{
    return pSource->QueryValueDword(keyName, valueName, (PDWORD)pValue);
}

static HRESULT CALLBACK
_QuerySourceExists(
    IQuerySourceOld* pSource,
    ASSOCQUERY query,
    PCWSTR keyName,
    PCWSTR valueName,
    PVOID pValue)
{
    return pSource->QueryValueExists(keyName, valueName);
}

static HRESULT CALLBACK
_QuerySourceString(
    IQuerySourceOld* pSource,
    ASSOCQUERY query,
    PCWSTR keyName,
    PCWSTR valueName,
    PVOID pValue)
{
    HRESULT hr = pSource->QueryValueString(keyName, valueName, static_cast<PWSTR*>(pValue));
    if (FAILED_UNEXPECTEDLY(hr) || !(query & ASSOCQUERY_INDIRECT))
        return hr;
    return SHAllocMUI(static_cast<PWSTR*>(pValue));
}

static inline HRESULT
_QuerySourceCreateFromKey(HKEY hKey, PCWSTR lpSubKey, BOOL bCreate, IQuerySourceOld** ppSource)
{
    return QuerySourceCreateFromKey(hKey, lpSubKey, bCreate,
                                    IID_PPV_ARG(IQuerySourceOld, ppSource));
}

static HRESULT
_QuerySourceCreateFromKey2(
    HKEY    hKey,
    PCWSTR  key1,
    PCWSTR  key2,
    IQuerySourceOld** ppSource)
{
    WCHAR szBuff[MAX_PATH];

    if (key1)
        PathCchCombineEx(szBuff, _countof(szBuff), key1, key2, PATHCCH_NONE);
    else
        StringCchCopyW(szBuff, _countof(szBuff), key2);

    return _QuerySourceCreateFromKey(hKey, szBuff, FALSE, ppSource);
}

static HRESULT
_AssocOpenRegKey(HKEY hKey, PCWSTR lpSubKey, PHKEY phkResult, BOOL bCreate)
{
    *phkResult = NULL;
    if (!hKey)
        return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);

    LSTATUS error;
    if ( bCreate )
        error = RegCreateKeyExW(hKey, lpSubKey, 0, NULL, 0, MAXIMUM_ALLOWED, NULL,
                                phkResult, NULL);
    else
        error = RegOpenKeyExW(hKey, lpSubKey, 0, MAXIMUM_ALLOWED, phkResult);

    if (error == ERROR_SUCCESS)
        return S_OK;

    return HRESULT_FROM_WIN32(error);
}

static HKEY _OpenProgidKey(PCWSTR psz)
{
    HKEY hKeyBase;
    HRESULT hr = _AssocOpenRegKey(HKEY_CLASSES_ROOT, psz, &hKeyBase, FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    WCHAR szData[64];
    DWORD cbData = sizeof(szData);
    if (StrCmpIW(L"Excel.Sheet.8", psz) == 0)
        return hKeyBase;

    LSTATUS error = SHGetValueW(hKeyBase, L"CurVer", NULL, NULL, szData, &cbData);
    if (error || cbData <= 2)
        return hKeyBase;

    HKEY hKeyCurVer;
    hr = _AssocOpenRegKey(HKEY_CLASSES_ROOT, szData, &hKeyCurVer, FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hKeyBase;

    HKEY hKeyShell;
    hr = _AssocOpenRegKey(hKeyCurVer, L"shell", &hKeyShell, FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        hr = _AssocOpenRegKey(hKeyBase, L"shell", &hKeyShell, FALSE);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            RegCloseKey(hKeyBase);
            return hKeyCurVer;
        }

        RegCloseKey(hKeyShell);
        RegCloseKey(hKeyCurVer);
        return hKeyBase;
    }

    RegCloseKey(hKeyShell);
    RegCloseKey(hKeyBase);
    return hKeyCurVer;
}

static inline void _MakeApplicationsKey(PCWSTR pszPath, PWSTR pszDest, UINT cchDest)
{
    HRESULT hr = PathCchCombineEx(pszDest, cchDest, L"Applications", pszPath, PATHCCH_NONE);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    PathCchAddExtension(pszDest, cchDest, L".exe");
}

static inline BOOL _ParamIsApp(PCWSTR psz)
{
    return !StrCmpNW(psz, L"%1", 2) || !StrCmpNW(psz, L"\"%1\"", 4);
}

static HRESULT _PathFileExists(PCWSTR lpFileName)
{
    DWORD attrs;
    if ( !PathFileExistsAndAttributesW(lpFileName, &attrs) || (attrs & FILE_ATTRIBUTE_DIRECTORY))
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    return S_OK;
}

static HRESULT _ExeFromCmd(PCWSTR pszCmdTemplate, PWSTR* ppszApplication)
{
    if (_ParamIsApp(pszCmdTemplate))
        return SHStrDupW(L"%1", ppszApplication);

    PWSTR pszParameters = NULL;
    HRESULT hr = SHEvaluateSystemCommandTemplate(pszCmdTemplate, ppszApplication,
                                                 NULL, &pszParameters);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (hr != S_OK || StrCmpIW(PathFindFileNameW(*ppszApplication), L"rundll32.exe") != 0)
    {
        CoTaskMemFree(pszParameters);
        return hr;
    }

    CoTaskMemFree(*ppszApplication);
    *ppszApplication = NULL;

    PWSTR pchComma = StrChrW(pszParameters, L',');
    if (!pchComma)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        CoTaskMemFree(pszParameters);
        return hr;
    }

    WCHAR szExe[MAX_PATH];
    hr = _PathCopyExeAndTrimWhiteSpaces(szExe, _countof(szExe), pszParameters,
                                        pchComma - pszParameters);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        CoTaskMemFree(pszParameters);
        return hr;
    }

    PathUnquoteSpacesW(szExe);

    PathCchAddExtension(szExe, _countof(szExe), L".dll");

    if (PathIsAbsolute(szExe))
    {
        hr = _PathFileExists(szExe);
    }
    else if (PathIsFileSpecW(szExe))
    {
        hr = _PathFindInSystem(szExe, _countof(szExe));
    }
    else
    {
        CoTaskMemFree(pszParameters);
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (SUCCEEDED(hr))
        hr = SHStrDupW(szExe, ppszApplication);

    CoTaskMemFree(pszParameters);
    return hr;
}

/******************************************************************************
 * CAssocElement
 */
class CAssocElement
    : public IObjectWithQuerySourceOld
    , public IAssociationElementOld
{
protected:
    LONG m_cRefs = 1;
    IQuerySourceOld* m_pSource = NULL;

    HRESULT _QueryKeyValAny(
        QUERY_CALLBACK callback,
        const QUERYKEYVAL* pItems,
        UINT cItems,
        IQuerySourceOld* pQS,
        ASSOCQUERY query,
        PCWSTR valueName,
        PVOID pValue);

    HRESULT _QuerySourceAny(
        QUERY_CALLBACK callback,
        IQuerySourceOld* pSource,
        DWORD dwFlags,
        ASSOCQUERY query,
        PCWSTR valueName,
        PVOID pValue);

public:
    virtual ~CAssocElement();

    /*** IUnknown ***/
    STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    /*** IObjectWithQuerySourceOld ***/
    STDMETHODIMP SetSource(IQuerySourceOld* pSource) override;
    STDMETHODIMP GetSource(REFIID riid, PVOID* ppSource) override;
    /*** IAssociationElementOld ***/
    STDMETHODIMP QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue) override;
    STDMETHODIMP QueryDword(ASSOCQUERY query, PCWSTR key, DWORD* pdwValue) override;
    STDMETHODIMP QueryExists(ASSOCQUERY query, PCWSTR key) override;
    STDMETHODIMP QueryDirect(ASSOCQUERY query, PCWSTR key, FLAGGED_BYTE_BLOB** ppBlob) override;
    STDMETHODIMP QueryObject(ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID* ppvObj) override;

protected:
    virtual UINT _GetQueryKeyVal(const QUERYKEYVAL** ppItems);
};

/******************************************************************************
 * CAssocShellElement
 */
class CAssocShellElement
    : public CAssocElement
    , public IPersistString2
{
protected:
    PWSTR m_pszName = NULL;
    WCHAR m_szBuff[64] = L"";

    UINT _GetQueryKeyVal(const QUERYKEYVAL** ppItems) override;
    HRESULT _DefaultVerbSource(IQuerySourceOld** ppSource);

public:
    ~CAssocShellElement();

    /*** IUnknown ***/
    STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    /*** IAssociationElementOld ***/
    STDMETHODIMP QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue) override;
    STDMETHODIMP QueryDword(ASSOCQUERY query, PCWSTR key, DWORD* pdwValue) override;
    STDMETHODIMP QueryExists(ASSOCQUERY query, PCWSTR key) override;
    STDMETHODIMP QueryDirect(ASSOCQUERY query, PCWSTR key, FLAGGED_BYTE_BLOB** ppBlob) override;
    STDMETHODIMP QueryObject(ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID* ppvObj) override;
    /*** IPersist ***/
    STDMETHODIMP GetClassID(CLSID* pClassID) override;
    /*** IPersistString2 ***/
    STDMETHODIMP SetString(PCWSTR psz) override;
    STDMETHODIMP GetString(PWSTR* ppsz) override;

    virtual BOOL _UseEnumForDefaultVerb();
    virtual HRESULT _InitSource();
    virtual BOOL _IsAppSource();
    virtual HRESULT _GetVerbDelegate(PCWSTR pszSrc, IAssociationElementOld** ppElement);
};

/******************************************************************************
 * CAssocShellVerbElement
 */
class CAssocShellVerbElement
    : public CAssocElement
{
protected:
    BOOL m_bAppSource = FALSE;

    UINT _GetQueryKeyVal(const QUERYKEYVAL** ppItems) override;
    HRESULT _GetAppDelegate(REFIID riid, PVOID* ppv);

public:
    CAssocShellVerbElement(BOOL bAppSource);

    STDMETHODIMP QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppwsz) override;
    STDMETHODIMP QueryObject(ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID* ppvObj) override;
};

/******************************************************************************
 * CAssocApplicationElement
 */
class CAssocApplicationElement
    : public CAssocShellElement
{
protected:
    BOOL m_bHasDir = FALSE;

    UINT _GetQueryKeyVal(const QUERYKEYVAL** ppItems) override;
    HRESULT _InitSource() override;
    HRESULT _GetAppDisplayName(PWSTR* ppsz);

public:
    STDMETHODIMP GetClassID(CLSID* pClsid) override;
    STDMETHODIMP QueryObject(ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID* ppv) override;
    STDMETHODIMP QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppsz) override;
};

/******************************************************************************
 * CAssocProgidElement
 */
class CAssocProgidElement
    : public CAssocShellElement
{
    IQuerySourceOld* m_pSource2 = NULL;

public:
    ~CAssocProgidElement() override;

    /*** IAssociationElementOld ***/
    STDMETHODIMP QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue) override;
    /*** IPersist ***/
    STDMETHODIMP GetClassID(CLSID* pClassID) override;

    BOOL _UseEnumForDefaultVerb() override;
    HRESULT _InitSource() override;
};

/******************************************************************************
 * CAssocClsidElement
 */
class CAssocClsidElement
    : public CAssocShellElement
{
public:
    /*** IPersist ***/
    STDMETHODIMP GetClassID(CLSID* pClassID) override;

    HRESULT _InitSource() override;
};

/******************************************************************************
 * CAssocSystemExtElement
 */
class CAssocSystemExtElement
    : public CAssocShellElement
{
public:
    /*** IPersist ***/
    STDMETHODIMP GetClassID(CLSID* pClassID) override;

    HRESULT _InitSource() override;
};

/******************************************************************************
 * CAssocFolderElement
 */
class CAssocFolderElement
    : public CAssocShellElement
{
protected:

public:
    /*** IAssociationElementOld ***/
    STDMETHODIMP QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue) override;
    /*** IPersist ***/
    STDMETHODIMP GetClassID(CLSID* pClassID) override;

    HRESULT _InitSource() override;
};

/******************************************************************************
 * CAssocStarElement
 */
class CAssocStarElement
    : public CAssocShellElement
{
protected:

public:
    /*** IAssociationElementOld ***/
    STDMETHODIMP QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue) override;
    /*** IPersist ***/
    STDMETHODIMP GetClassID(CLSID* pClassID) override;

    HRESULT _InitSource() override;
};

/******************************************************************************
 * CAssocPerceivedElement
 */
class CAssocPerceivedElement
    : public CAssocShellElement
{
protected:
    BOOL m_bPerceived = FALSE;

    UINT _GetQueryKeyVal(const QUERYKEYVAL** ppItems) override;

public:
    /*** IPersist ***/
    STDMETHODIMP GetClassID(CLSID* pClassID) override;
    /*** IObjectWithQuerySourceOld ***/
    STDMETHODIMP GetSource(REFIID riid, PVOID* ppSource) override;

    HRESULT _InitSource() override;
    HRESULT _GetVerbDelegate(PCWSTR pszSrc, IAssociationElementOld** ppElement) override;
};

/******************************************************************************
 * CAssocClientElement
 */
class CAssocClientElement
    : public CAssocShellElement
{
public:
    /*** IAssociationElementOld ***/
    STDMETHODIMP QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue) override;
    /*** IPersist ***/
    STDMETHODIMP GetClassID(CLSID* pClassID) override;

    HRESULT _InitSource() override;
    HRESULT _FixNetscapeRegistration();
    BOOL _CreateRepairedNetscapeRegistration(HKEY hKey);
    HRESULT _InitSourceFromKey(HKEY hKey, PCWSTR lpSubKey, DWORD dwFlags);
};

/******************************************************************************
 * CAssocElement methods
 */

CAssocElement::~CAssocElement()
{
    if (m_pSource)
    {
        m_pSource->Release();
        m_pSource = NULL;
    }
}

UINT CAssocElement::_GetQueryKeyVal(const QUERYKEYVAL** ppItems)
{
    *ppItems = NULL;
    return 0;
}

HRESULT
CAssocElement::_QueryKeyValAny(
    QUERY_CALLBACK callback,
    const QUERYKEYVAL* pItems,
    UINT cItems,
    IQuerySourceOld* pQS,
    ASSOCQUERY query,
    PCWSTR valueName,
    PVOID pValue)
{
    const QUERYKEYVAL* pItem = _FindKeyVal(query, pItems, cItems);
    if (!pItem)
        return E_INVALIDARG;

    WCHAR szBuff[128];
    PCWSTR key = pItem->key;
    if ((query & ASSOCQUERY_EXTRA_NON_VERB) && key)
    {
        StringCchPrintfW(szBuff, _countof(szBuff), key, valueName);
        key = szBuff;
    }
    return callback(pQS, query, key, pItem->value, pValue);
}

HRESULT
CAssocElement::_QuerySourceAny(
    QUERY_CALLBACK callback,
    IQuerySourceOld* pSource,
    DWORD dwFlags,
    ASSOCQUERY query,
    PCWSTR valueName,
    PVOID pValue)
{
    if (!pSource)
        return E_INVALIDARG;

    if (query == (ASSOCQUERY_SDED | ASSOCQUERY_EXTRA_NON_VERB) ||
        query == ASSOCQUERY_LOCALIZEDSTRING)
    {
        return callback(pSource, query, NULL, valueName, pValue);
    }

    if ((query & dwFlags) != dwFlags)
        return E_INVALIDARG;

    const QUERYKEYVAL* pItems = NULL;
    UINT cItems = _GetQueryKeyVal(&pItems);
    if (!cItems)
        return E_INVALIDARG;

    return _QueryKeyValAny(callback, pItems, cItems, pSource, query, valueName, pValue);
}

STDMETHODIMP CAssocElement::QueryInterface(REFIID riid, PVOID* ppv)
{
    if (riid == IID_IObjectWithQuerySourceOld)
    {
        *ppv = static_cast<IObjectWithQuerySourceOld*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_IAssociationElementOld)
    {
        *ppv = static_cast<IAssociationElementOld*>(this);
        AddRef();
        return S_OK;
    }
    ERR("E_NOINTERFACE: %s\n", wine_dbgstr_guid(&riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CAssocElement::AddRef()
{
    return InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CAssocElement::Release()
{
    LONG refs = InterlockedDecrement(&m_cRefs);
    if (!refs)
        delete this;
    return refs;
}

STDMETHODIMP CAssocElement::SetSource(IQuerySourceOld* pSource)
{
    if (m_pSource)
        return E_UNEXPECTED;
    m_pSource = pSource;
    m_pSource->AddRef();
    return S_OK;
}

STDMETHODIMP CAssocElement::GetSource(REFIID riid, PVOID* ppSource)
{
    if (!m_pSource)
    {
        *ppSource = NULL;
        return E_NOINTERFACE;
    }
    return m_pSource->QueryInterface(riid, ppSource);
}

STDMETHODIMP CAssocElement::QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue)
{
    *ppszValue = NULL;
    return _QuerySourceAny(_QuerySourceString, m_pSource, (ASSOCQUERY_STRING | ASSOCQUERY_DIRECT),
                           query, key, ppszValue);
}

STDMETHODIMP CAssocElement::QueryDword(ASSOCQUERY query, PCWSTR key, DWORD* pdwValue)
{
    return _QuerySourceAny(_QuerySourceDword, m_pSource, (ASSOCQUERY_DWORD | ASSOCQUERY_DIRECT),
                           query, key, pdwValue);
}

STDMETHODIMP CAssocElement::QueryExists(ASSOCQUERY query, PCWSTR key)
{
    return _QuerySourceAny(_QuerySourceExists, m_pSource, (ASSOCQUERY_STRING | ASSOCQUERY_DIRECT),
                           query, key, NULL);
}

STDMETHODIMP CAssocElement::QueryDirect(ASSOCQUERY query, PCWSTR key, FLAGGED_BYTE_BLOB** ppBlob)
{
    *ppBlob = NULL;
    return _QuerySourceAny(_QuerySourceDirect, m_pSource, ASSOCQUERY_DIRECT, query, key, ppBlob);
}

STDMETHODIMP CAssocElement::QueryObject(ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID* ppvObj)
{
    *ppvObj = NULL;
    return E_NOTIMPL;
}

/******************************************************************************
 * CAssocShellElement methods
 */

CAssocShellElement::~CAssocShellElement()
{
    if (m_pszName && m_pszName != m_szBuff)
        LocalFree(m_pszName);
}

STDMETHODIMP CAssocShellElement::QueryInterface(REFIID riid, PVOID* ppv)
{
    if (riid == IID_IPersist)
    {
        *ppv = static_cast<IPersist*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_IPersistString2)
    {
        *ppv = static_cast<IPersistString2*>(this);
        AddRef();
        return S_OK;
    }
    return CAssocElement::QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CAssocShellElement::AddRef()
{
    return CAssocElement::AddRef();
}

STDMETHODIMP_(ULONG) CAssocShellElement::Release()
{
    return CAssocElement::Release();
}

HRESULT CAssocShellElement::_InitSource()
{
    return _QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, m_pszName, 0, &m_pSource);
}

UINT CAssocShellElement::_GetQueryKeyVal(const QUERYKEYVAL** ppItems)
{
    *ppItems = g_shellKeyVals;
    return _countof(g_shellKeyVals);
}

BOOL CAssocShellElement::_UseEnumForDefaultVerb()
{
    return FALSE;
}

BOOL CAssocShellElement::_IsAppSource()
{
    return FALSE;
}

HRESULT CAssocShellElement::_GetVerbDelegate(PCWSTR pszSrc, IAssociationElementOld** ppElement)
{
    if (!m_pSource)
        return E_FAIL;

    HRESULT hr;
    IQuerySourceOld* pSource;
    if (pszSrc)
        hr = QSOpen2(m_pSource, L"shell", pszSrc, 0, &pSource);
    else
        hr = _DefaultVerbSource(&pSource);

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    BOOL bAppSource = _IsAppSource();
    auto pElement = new(std::nothrow) CAssocShellVerbElement(bAppSource);
    if (!pElement)
    {
        ERR("E_OUTOFMEMORY\n");
        pSource->Release();
        return E_OUTOFMEMORY;
    }

    hr = pElement->SetSource(pSource);
    *ppElement = static_cast<IAssociationElementOld*>(pElement);
    pSource->Release();
    return hr;
}

STDMETHODIMP CAssocShellElement::QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue)
{
    if (!(query & ASSOCQUERY_EXTRA_VERB))
        return CAssocElement::QueryString(query, key, ppszValue);

    IAssociationElementOld* pDelegate;
    HRESULT hr = _GetVerbDelegate(key, &pDelegate);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pDelegate->QueryString(query, NULL, ppszValue);
    pDelegate->Release();
    return hr;
}

STDMETHODIMP CAssocShellElement::QueryDword(ASSOCQUERY query, PCWSTR key, DWORD* pdwValue)
{
    if (!(query & ASSOCQUERY_EXTRA_VERB))
        return CAssocElement::QueryDword(query, key, pdwValue);

    IAssociationElementOld* pDelegate;
    HRESULT hr = _GetVerbDelegate(key, &pDelegate);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pDelegate->QueryDword(query, NULL, pdwValue);
    pDelegate->Release();
    return hr;
}

STDMETHODIMP CAssocShellElement::QueryExists(ASSOCQUERY query, PCWSTR key)
{
    if (!(query & ASSOCQUERY_EXTRA_VERB))
        return CAssocElement::QueryExists(query, key);

    IAssociationElementOld* pDelegate;
    HRESULT hr = _GetVerbDelegate(key, &pDelegate);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pDelegate->QueryExists(query, NULL);
    pDelegate->Release();
    return hr;
}

STDMETHODIMP
CAssocShellElement::QueryDirect(ASSOCQUERY query, PCWSTR key, FLAGGED_BYTE_BLOB** ppBlob)
{
    if (!(query & ASSOCQUERY_EXTRA_VERB))
        return CAssocElement::QueryDirect(query, key, ppBlob);

    IAssociationElementOld* pDelegate;
    HRESULT hr = _GetVerbDelegate(key, &pDelegate);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pDelegate->QueryDirect(query, NULL, ppBlob);
    pDelegate->Release();
    return hr;
}

STDMETHODIMP
CAssocShellElement::QueryObject(ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID* ppvObj)
{
    if (!(query & ASSOCQUERY_EXTRA_VERB))
    {
        *ppvObj = NULL;
        return E_INVALIDARG;
    }

    IAssociationElementOld* pDelegate;
    HRESULT hr = _GetVerbDelegate(key, &pDelegate);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (query == (ASSOCQUERY_OBJECT | ASSOCQUERY_EXTRA_VERB))
        hr = pDelegate->QueryInterface(riid, ppvObj);
    else
        hr = pDelegate->QueryObject(query, NULL, riid, ppvObj);

    pDelegate->Release();
    return hr;
}

STDMETHODIMP CAssocShellElement::GetClassID(CLSID* pClassID)
{
    *pClassID = CLSID_AssocShellElement;
    return S_OK;
}

STDMETHODIMP CAssocShellElement::SetString(PCWSTR psz)
{
    if (m_pszName)
        return E_UNEXPECTED;

    UINT cchBuff, cchString = lstrlenW(psz);
    PWSTR pszBuff;
    if (cchString >= _countof(m_szBuff))
    {
        cchBuff = cchString + 1;
        pszBuff = (PWSTR)LocalAlloc(LPTR, cchBuff * sizeof(WCHAR));
    }
    else
    {
        pszBuff = m_szBuff;
        cchBuff = _countof(m_szBuff);
    }

    m_pszName = pszBuff;
    if (!m_pszName)
        return E_UNEXPECTED;

    StringCchCopyW(pszBuff, cchBuff, psz);
    return _InitSource();
}

STDMETHODIMP CAssocShellElement::GetString(PWSTR* ppsz)
{
    return SHStrDupW(m_pszName, ppsz);
}

HRESULT CAssocShellElement::_DefaultVerbSource(IQuerySourceOld** ppSource)
{
    IQuerySourceOld* pShellSource;
    HRESULT hr = m_pSource->OpenSource(L"shell", FALSE, &pShellSource);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    PWSTR pszValue;
    PCWSTR pszVerb = L"open";
    if (SUCCEEDED(pShellSource->QueryValueString(NULL, NULL, &pszValue)))
        pszVerb = pszValue;

    hr = pShellSource->OpenSource(pszVerb, FALSE, ppSource);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        if (pszValue)
        {
            INT nDelim = StrCSpnW(pszValue, L" ,");
            if (nDelim != lstrlenW(pszValue))
            {
                pszValue[nDelim] = UNICODE_NULL;
                hr = pShellSource->OpenSource(pszValue, FALSE, ppSource);
            }
        }
        else
        {
            IEnumString* pEnum;
            if (_UseEnumForDefaultVerb() && SUCCEEDED(pShellSource->EnumSources(&pEnum)))
            {
                PWSTR pszFirst = NULL;
                ULONG cFetched = 0;
                if (SUCCEEDED(pEnum->Next(1, &pszFirst, &cFetched)))
                {
                    hr = pShellSource->OpenSource(pszFirst, FALSE, ppSource);
                    CoTaskMemFree(pszFirst);
                }
                pEnum->Release();
            }
        }
    }

    if (pszValue)
        CoTaskMemFree(pszValue);

    pShellSource->Release();
    return hr;
}

/******************************************************************************
 * CAssocShellVerbElement methods
 */

CAssocShellVerbElement::CAssocShellVerbElement(BOOL bAppSource)
    : m_bAppSource(bAppSource)
{
}

STDMETHODIMP CAssocShellVerbElement::QueryString(
    ASSOCQUERY query,
    PCWSTR key,
    PWSTR* ppwsz)
{
    HRESULT hr = CAssocElement::QueryString(query, key, ppwsz);
    if (SUCCEEDED(hr))
        return hr;

    PWSTR psz;
    if (query == ASSOCQUERY_SEV_7)
    {
        hr = CAssocElement::QueryString(ASSOCQUERY_COMMAND, 0, (PWSTR*)&psz);
        if (SUCCEEDED(hr))
            hr = _ExeFromCmd(psz, ppwsz);
        if (psz)
            CoTaskMemFree(psz);
        return hr;
    }

    if (query == ASSOCQUERY_DDEEXEC_APPLICATION)
    {
        hr = QueryString(ASSOCQUERY_DELEGATE, NULL, &psz);
        if (SUCCEEDED(hr))
        {
            PathStripPathW(psz);
            PathRemoveExtensionW(psz);
        }
        *ppwsz = psz;
        return hr;
    }

    if (query == ASSOCQUERY_DDEEXEC_TOPIC)
        return SHStrDupW(L"System", ppwsz);

    if (!m_bAppSource && query == ASSOCQUERY_VERB_FRIENDLYAPPNAME)
    {
        IAssociationElementOld* pElement;
        hr = _GetAppDelegate(IID_PPV_ARG(IAssociationElementOld, &pElement));
        if (SUCCEEDED(hr))
        {
            hr = pElement->QueryString(ASSOCQUERY_APPKEY_FRIENDLYAPPNAME, NULL, ppwsz);
            pElement->Release();
        }
    }

    return hr;
}

UINT CAssocShellVerbElement::_GetQueryKeyVal(const QUERYKEYVAL** ppItems)
{
    *ppItems = g_shellVerbKeyVals;
    return _countof(g_shellVerbKeyVals);
}

HRESULT CAssocShellVerbElement::_GetAppDelegate(REFIID riid, PVOID* ppv)
{
    PWSTR psz = NULL;
    HRESULT hr = CAssocElement::QueryString(ASSOCQUERY_DELEGATE, NULL, &psz);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    IPersistString2* pPS2;
    hr = AssocCreateElement(CLSID_AssocApplicationElement, IID_PPV_ARG(IPersistString2, &pPS2));
    if (SUCCEEDED(hr))
    {
        hr = pPS2->SetString(psz);
        if (SUCCEEDED(hr))
            hr = pPS2->QueryInterface(riid, ppv);
        pPS2->Release();
    }

    if (psz)
        CoTaskMemFree(psz);

    return hr;
}

STDMETHODIMP
CAssocShellVerbElement::QueryObject(ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID* ppvObj)
{
    if (query != ASSOCQUERY_OBJECT_EV_1)
        return E_INVALIDARG;
    return _GetAppDelegate(riid, ppvObj);
}

/******************************************************************************
 * CAssocApplicationElement methods
 */

UINT CAssocApplicationElement::_GetQueryKeyVal(const QUERYKEYVAL** ppItems)
{
    *ppItems = g_shellAppKeyVals;
    return _countof(g_shellAppKeyVals);
}

HRESULT CAssocApplicationElement::_InitSource()
{
    PWSTR pchFileName = PathFindFileNameW(m_pszName);

    WCHAR szSubKey[MAX_PATH];
    _MakeApplicationsKey(pchFileName, szSubKey, _countof(szSubKey));

    HRESULT hr = QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, szSubKey, FALSE,
                                          IID_PPV_ARG(IQuerySourceOld, &m_pSource));
    m_bHasDir = (pchFileName != m_pszName);
    if (FAILED_UNEXPECTEDLY(hr) && m_bHasDir && PathFileExistsW(m_pszName))
        return S_FALSE;

    return hr;
}

STDMETHODIMP CAssocApplicationElement::GetClassID(CLSID* pClsid)
{
    *pClsid = CLSID_AssocApplicationElement;
    return S_OK;
}

STDMETHODIMP
CAssocApplicationElement::QueryObject(ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID* ppv)
{
    if (query == ASSOCQUERY_OBJECT_EV_1)
        return CAssocElement::QueryInterface(riid, ppv);
    return CAssocShellElement::QueryObject(query, key, riid, ppv);
}

STDMETHODIMP CAssocApplicationElement::QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppsz)
{
    HRESULT hr = CAssocShellElement::QueryString(query, key, ppsz);
    if (FAILED_UNEXPECTEDLY(hr) && query == ASSOCQUERY_APPKEY_FRIENDLYAPPNAME)
        return _GetAppDisplayName(ppsz);

    return hr;
}

HRESULT CAssocApplicationElement::_GetAppDisplayName(PWSTR* ppsz)
{
    PWSTR pszName;
    if (!m_bHasDir)
    {
        HRESULT hr = QueryString(ASSOCQUERY_DELEGATE, NULL, &pszName);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }
    else
    {
        pszName = m_pszName;
    }

    WCHAR szData[MAX_PATH];
    DWORD cbData = sizeof(szData);
    DWORD flags = SHKEY_Subkey_MUICache | SHKEY_Key_ShellNoRoam | SHKEY_Root_HKCU;
    HRESULT hr = SKGetValueW(flags, NULL, pszName, 0, szData, &cbData);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        UINT cchData = MAX_PATH;
        if (SHGetFileDescriptionW(pszName, NULL, NULL, szData, &cchData))
        {
            INT cchName = lstrlenW(szData);
            UINT cbName = (cchName + 1) * sizeof(WCHAR);
            SKSetValueW(flags, NULL, pszName, REG_SZ, (PBYTE)szData, cbName);
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
        hr = SHStrDupW(szData, ppsz);

    if (!m_bHasDir)
        CoTaskMemFree(pszName);

    return hr;
}

/******************************************************************************
 * CAssocProgidElement methods
 */

CAssocProgidElement::~CAssocProgidElement()
{
    if (m_pSource2)
    {
        m_pSource2->Release();
        m_pSource2 = NULL;
    }
}

STDMETHODIMP CAssocProgidElement::QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue)
{
    HRESULT hr = CAssocShellElement::QueryString(query, key, ppszValue);
    if (SUCCEEDED(hr))
        return hr;

    if (m_pSource2 && (query & ASSOCQUERY_FALLBACK))
    {
        return _QueryKeyValAny(_QuerySourceString,
                               g_progidKeyVals, _countof(g_progidKeyVals),
                               m_pSource2,
                               query,
                               key,
                               ppszValue);
    }

    if (m_pSource && query == ASSOCQUERY_FRIENDLYAPPNAME)
        return m_pSource->QueryValueString(NULL, NULL, ppszValue);

    return hr;
}

STDMETHODIMP CAssocProgidElement::GetClassID(CLSID* pClassID)
{
    *pClassID = CLSID_AssocProgidElement;
    return S_OK;
}

BOOL CAssocProgidElement::_UseEnumForDefaultVerb()
{
    return TRUE;
}

HRESULT CAssocProgidElement::_InitSource()
{
    HRESULT hr = S_OK;
    PWSTR pszName;

    if (*m_pszName == L'.')
    {
        hr = _QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, m_pszName, 0, &m_pSource2);
        if (FAILED_UNEXPECTEDLY(hr))
            goto Exit;

        hr = m_pSource2->QueryValueString(NULL, NULL, &pszName);
    }
    else
    {
        pszName = m_pszName;
    }

    if (SUCCEEDED(hr))
    {
        HKEY hKey = _OpenProgidKey(pszName);
        if (hKey)
        {
            hr = _QuerySourceCreateFromKey(hKey, NULL, NULL, &m_pSource);
            RegCloseKey(hKey);
        }
        else
        {
            hr = E_UNEXPECTED;
        }

        if (pszName != m_pszName)
            CoTaskMemFree(pszName);
    }

Exit:
    if (FAILED_UNEXPECTEDLY(hr) && m_pSource2)
    {
        m_pSource = m_pSource2;
        m_pSource2 = NULL;
        return S_FALSE;
    }

    return hr;
}

/******************************************************************************
 * CAssocClsidElement methods
 */

STDMETHODIMP CAssocClsidElement::GetClassID(CLSID* pClassID)
{
    *pClassID = CLSID_AssocClsidElement;
    return S_OK;
}

HRESULT CAssocClsidElement::_InitSource()
{
    return _QuerySourceCreateFromKey2(HKEY_CLASSES_ROOT, L"CLSID", m_pszName, &m_pSource);
}

/******************************************************************************
 * CAssocSystemExtElement methods
 */

STDMETHODIMP CAssocSystemExtElement::GetClassID(CLSID* pClassID)
{
    *pClassID = CLSID_AssocSystemElement;
    return S_OK;
}

HRESULT CAssocSystemExtElement::_InitSource()
{
    return _QuerySourceCreateFromKey2(HKEY_CLASSES_ROOT, L"SystemFileAssociations", m_pszName,
                                      &m_pSource);
}

/******************************************************************************
 * CAssocFolderElement methods
 */

STDMETHODIMP CAssocFolderElement::QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue)
{
    if (query == ASSOCQUERY_FRIENDLYAPPNAME)
        return _SHAllocLoadString(GetModuleHandleW(L"shlwapi.dll"), IDS_FOLDER, ppszValue);
    return CAssocShellElement::QueryString(query, key, ppszValue);
}

STDMETHODIMP CAssocFolderElement::GetClassID(CLSID* pClassID)
{
    *pClassID = CLSID_AssocFolderElement;
    return S_OK;
}

HRESULT CAssocFolderElement::_InitSource()
{
    return QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, L"Folder", FALSE,
                                    IID_PPV_ARG(IQuerySourceOld, &m_pSource));
}

/******************************************************************************
 * CAssocStarElement methods
 */

STDMETHODIMP CAssocStarElement::QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue)
{
    if (query == ASSOCQUERY_FRIENDLYAPPNAME)
        return _GetFileTypeName(m_pszName, ppszValue);

    return CAssocShellElement::QueryString(query, key, ppszValue);
}

STDMETHODIMP CAssocStarElement::GetClassID(CLSID* pClassID)
{
    *pClassID = CLSID_AssocStarElement;
    return S_OK;
}

HRESULT CAssocStarElement::_InitSource()
{
    return QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, L"*", FALSE,
                                    IID_PPV_ARG(IQuerySourceOld, &m_pSource));
}

/******************************************************************************
 * CAssocPerceivedElement methods
 */

UINT CAssocPerceivedElement::_GetQueryKeyVal(const QUERYKEYVAL** ppItems)
{
    if (m_bPerceived)
    {
        *ppItems = g_perceivedKeyVals;
        return _countof(g_perceivedKeyVals);
    }
    else
    {
        *ppItems = g_shellKeyVals;
        return _countof(g_shellKeyVals);
    }
}

STDMETHODIMP CAssocPerceivedElement::GetClassID(CLSID* pClassID)
{
    *pClassID = CLSID_AssocPerceivedElement;
    return S_OK;
}

STDMETHODIMP CAssocPerceivedElement::GetSource(REFIID riid, PVOID* ppSource)
{
    *ppSource = NULL;
    if (m_bPerceived)
        return E_FAIL;

    return CAssocElement::GetSource(riid, ppSource);
}

HRESULT CAssocPerceivedElement::_InitSource()
{
    PERCEIVED type;
    PERCEIVEDFLAG flags;
    PWSTR pszType;
    HRESULT hr = AssocGetPerceivedType(m_pszName, &type, &flags, &pszType);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    m_bPerceived = !(flags & PERCEIVEDFLAG_NATIVESUPPORT);
    hr = _QuerySourceCreateFromKey2(HKEY_CLASSES_ROOT, L"SystemFileAssociations", pszType,
                                    &m_pSource);
    CoTaskMemFree(pszType);
    return hr;
}

HRESULT CAssocPerceivedElement::_GetVerbDelegate(PCWSTR pszSrc, IAssociationElementOld** ppElement)
{
    if (m_bPerceived)
        return E_FAIL;

    return CAssocShellElement::_GetVerbDelegate(pszSrc, ppElement);
}

/******************************************************************************
 * CAssocClientElement methods
 */

STDMETHODIMP CAssocClientElement::QueryString(ASSOCQUERY query, PCWSTR key, PWSTR* ppszValue)
{
    HRESULT hr;

    if (query == ASSOCQUERY_DEFAULTICON)
    {
        hr = CAssocElement::QueryString(ASSOCQUERY_DEFAULTICON, key, ppszValue);
        if (SUCCEEDED(hr))
            return hr;

        IAssociationElementOld* pDelegate;
        HRESULT hr = _GetVerbDelegate(key, &pDelegate);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = pDelegate->QueryString(ASSOCQUERY_DELEGATE, L"open", ppszValue);
        pDelegate->Release();
        return hr;
    }

    if (query == ASSOCQUERY_FRIENDLYAPPNAME)
    {
        hr = CAssocElement::QueryString(ASSOCQUERY_LOCALIZEDSTRING, L"LocalizedString", ppszValue);
        if (SUCCEEDED(hr))
            return hr;
        return CAssocElement::QueryString((ASSOCQUERY_SDED | ASSOCQUERY_EXTRA_NON_VERB), NULL, ppszValue);
    }

    return CAssocShellElement::QueryString(query, key, ppszValue);
}

STDMETHODIMP CAssocClientElement::GetClassID(CLSID* pClassID)
{
    *pClassID = CLSID_AssocClientElement;
    return S_OK;
}

HRESULT CAssocClientElement::_InitSource()
{
    WCHAR szBuff[MAX_PATH];
    StringCchPrintfW(szBuff, _countof(szBuff), L"SOFTWARE\\Clients\\%s", m_pszName);

    HRESULT hr = _InitSourceFromKey(HKEY_CURRENT_USER, szBuff, 0);
    if (SUCCEEDED(hr))
        return hr;
    return _InitSourceFromKey(HKEY_LOCAL_MACHINE, szBuff, 0);
}

HRESULT CAssocClientElement::_FixNetscapeRegistration()
{
    HKEY hKeyParent;
    LSTATUS error = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Clients\\Mail", 0, NULL, 0,
                                    KEY_ALL_ACCESS, NULL, &hKeyParent, NULL);
    if (error)
        return E_FAIL;

    DWORD dwDisposition;
    HKEY hKey;
    error = RegCreateKeyExW(hKeyParent, L"Netscape Messenger", 0, NULL, REG_OPTION_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
    if (error)
    {
        SHDeleteKeyW(hKeyParent, L"Netscape Messenger");
        RegCloseKey(hKeyParent);
        return E_FAIL;
    }

    HRESULT hr = E_FAIL;
    if (dwDisposition == REG_OPENED_EXISTING_KEY || _CreateRepairedNetscapeRegistration(hKey))
    {
        IQuerySourceOld* pNewSource = NULL;
        hr = _QuerySourceCreateFromKey(hKey, NULL, NULL, &pNewSource);
        if (SUCCEEDED(hr))
        {
            if (m_pSource)
                m_pSource->Release();
            m_pSource = pNewSource;
        }
    }

    RegCloseKey(hKey);

    if (FAILED_UNEXPECTEDLY(hr))
        SHDeleteKeyW(hKeyParent, L"Netscape Messenger");

    RegCloseKey(hKeyParent);
    return hr;
}

BOOL CAssocClientElement::_CreateRepairedNetscapeRegistration(HKEY hKey)
{
    HKEY hkResult;
    LSTATUS error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  L"Software\\Clients\\Mail\\Netscape Messenger",
                                  0, KEY_READ, &hkResult);
    if (error)
        return FALSE;

    WCHAR szIconFile[MAX_PATH];
    error = _RegQueryString(hkResult, L"Protocols\\mailto\\DefaultIcon",
                            szIconFile, _countof(szIconFile));
    if (error == ERROR_SUCCESS)
    {
        PathParseIconLocationW(szIconFile);
        StringCchCatW(szIconFile, _countof(szIconFile), L",-1349");
        _RegSetVolatileString(hKey, L"DefaultIcon", szIconFile);
    }

    BOOL ret = FALSE;
    PWSTR lpString2 = NULL;
    if (_RegQueryString(hkResult, 0, szIconFile, _countof(szIconFile)) == ERROR_SUCCESS &&
        _RegSetVolatileString(hKey, 0, szIconFile) == ERROR_SUCCESS &&
        _RegQueryString(hkResult, L"Protocols\\mailto\\shell\\open\\command",
                        szIconFile, _countof(szIconFile)) == ERROR_SUCCESS &&
        SUCCEEDED(_ExeFromCmd(szIconFile, &lpString2)))
    {
        StringCchCopyW(szIconFile, _countof(szIconFile), lpString2);
        CoTaskMemFree(lpString2);
        PathQuoteSpacesW(szIconFile);
        StringCchCatW(szIconFile, _countof(szIconFile), L" -mail");
        if (_RegSetVolatileString(hKey, L"shell\\open\\command", szIconFile) == ERROR_SUCCESS)
            ret = TRUE;
    }

    RegCloseKey(hkResult);
    return ret;
}

HRESULT CAssocClientElement::_InitSourceFromKey(HKEY hKey, PCWSTR lpSubKey, DWORD dwFlags)
{
    HKEY hKey2;
    LSTATUS error = RegOpenKeyExW(hKey, lpSubKey, 0, KEY_READ | dwFlags, &hKey2);
    if (error)
        return E_INVALIDARG;

    HRESULT hr = E_INVALIDARG;
    HKEY hkeySrc = NULL;
    WCHAR szData[80];
    DWORD dwType, cbData = sizeof(szData);
    error = RegQueryValueExW(hKey2, NULL, NULL, &dwType, (PBYTE)szData, &cbData);
    if (error == ERROR_SUCCESS && dwType == REG_SZ && szData[0] &&
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, lpSubKey, 0, KEY_READ, &hkeySrc) == ERROR_SUCCESS)
    {
        hr = _QuerySourceCreateFromKey2(hkeySrc, NULL, szData, &m_pSource);
        RegCloseKey(hkeySrc);
    }

    RegCloseKey(hKey2);

    if (FAILED_UNEXPECTEDLY(hr) ||
        lstrcmpiW(m_pszName, L"mail") != 0 ||
        lstrcmpiW(szData, L"Netscape Messenger") != 0 ||
        SUCCEEDED(QueryExists(ASSOCQUERY_COMMAND, L"open")))
    {
        return hr;
    }

    return _FixNetscapeRegistration();
}

/******************************************************************************
 * AssocCreateElement [SHELL32.764] (Vista+)
 * AssocCreateElement [SHLWAPI internal] (XP)
 *
 * @see AssocCreate
 * @see https://www.geoffchappell.com/studies/windows/shell/shell32/api/assocelem/createelement.htm
 */
EXTERN_C HRESULT WINAPI
AssocCreateElement(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ PVOID* ppvObj)
{
    IAssociationElementOld* pElement;
    if (rclsid == CLSID_AssocShellElement)
    {
        auto pNewElement = new(std::nothrow) CAssocShellElement();
        pElement = static_cast<IAssociationElementOld*>(pNewElement);
    }
    else if (rclsid == CLSID_AssocApplicationElement)
    {
        auto pNewElement = new(std::nothrow) CAssocApplicationElement();
        pElement = static_cast<IAssociationElementOld*>(pNewElement);
    }
    else if (rclsid == CLSID_AssocProgidElement)
    {
        auto pNewElement = new(std::nothrow) CAssocProgidElement();
        pElement = static_cast<IAssociationElementOld*>(pNewElement);
    }
    else if (rclsid == CLSID_AssocClsidElement)
    {
        auto pNewElement = new(std::nothrow) CAssocClsidElement();
        pElement = static_cast<IAssociationElementOld*>(pNewElement);
    }
    else if (rclsid == CLSID_AssocSystemElement)
    {
        auto pNewElement = new(std::nothrow) CAssocSystemExtElement();
        pElement = static_cast<IAssociationElementOld*>(pNewElement);
    }
    else if (rclsid == CLSID_AssocFolderElement)
    {
        auto pNewElement = new(std::nothrow) CAssocFolderElement();
        pElement = static_cast<IAssociationElementOld*>(pNewElement);
    }
    else if (rclsid == CLSID_AssocStarElement)
    {
        auto pNewElement = new(std::nothrow) CAssocStarElement();
        pElement = static_cast<IAssociationElementOld*>(pNewElement);
    }
    else if (rclsid == CLSID_AssocPerceivedElement)
    {
        auto pNewElement = new(std::nothrow) CAssocPerceivedElement();
        pElement = static_cast<IAssociationElementOld*>(pNewElement);
    }
    else if (rclsid == CLSID_AssocClientElement)
    {
        auto pNewElement = new(std::nothrow) CAssocClientElement();
        pElement = static_cast<IAssociationElementOld*>(pNewElement);
    }
    else
    {
        ERR("rclsid: %s\n", wine_dbgstr_guid(&rclsid));
        *ppvObj = NULL;
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if (!pElement)
    {
        ERR("E_OUTOFMEMORY\n");
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pElement->QueryInterface(riid, ppvObj);
    pElement->Release();
    return hr;
}
