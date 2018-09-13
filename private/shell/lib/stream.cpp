//***   stream.cpp -- stream (IStream, IPropertyBag, registry, etc.) helpers
// DESCRIPTION
//  CRegStrPropBag  registry-based property bag
//  utils           various utils
//
#include "priv.h"
#include "stream.h"

// {
//***   CRegStrPropBag -- registry-based property bag implementation
// NOTES
//  could be better, but good enough for now

CRegStrPropBag *CRegStrPropBag_Create(HKEY hkey, DWORD grfMode)
{
    CRegStrPropBag *prspb = new CRegStrPropBag;
    if (prspb && FAILED(prspb->Initialize(hkey, grfMode))) {
        delete prspb;
        prspb = NULL;
    }
    return prspb;
}

HRESULT CRegStrPropBag::Initialize(HKEY hkey, DWORD grfMode)
{
    HRESULT hr;

    hr = SetRoot(hkey, grfMode);
    return hr;
}

CRegStrPropBag::~CRegStrPropBag()
{
    HRESULT hr = SetRoot(0, STGM_READ);         // close
    ASSERT(SUCCEEDED(hr) && !_hkey);
}

//***   CRegStrPropBag::IUnknown::* {

ULONG CRegStrPropBag::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CRegStrPropBag::Release()
{
    ASSERT(_cRef > 0);

    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CRegStrPropBag::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CRegStrPropBag, IPropertyBag),     // IID_IPropertyBag
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

// }

//***   CRegStrPropBag::IPropertyBag::* {

//***   Read, Write --
// NOTES
//  BUGBUG perf: szName+szVal is big, but the chkstk call should still
// be faster than alloc/free i think...
HRESULT CRegStrPropBag::Read(/*[in]*/ LPCOLESTR pwzPropName,
        /*[inout]*/ VARIANT *pVar, /*[in]*/ IErrorLog *pErrorLog)
{
    ASSERT(pVar->vt == VT_EMPTY || pVar->vt == VT_BSTR);

    HRESULT hr;

    // convert to ansi since we want REG_SZ and thus must use RegSetValueA
    TCHAR szName[MAX_PATH];
    TCHAR szVal[MAX_URL_STRING];
    DWORD cbTmp;

    SHUnicodeToTChar(pwzPropName, szName, ARRAYSIZE(szName));

    szVal[0] = 0;           // paranoia
    cbTmp = SIZEOF(szVal);
    hr = QueryValueStr(szName, szVal, &cbTmp);
    if (SUCCEEDED(hr)) {
        pVar->vt = VT_BSTR;
        pVar->bstrVal = SysAllocStringLen(NULL, cbTmp); // n.b. cb->cch
        SHTCharToUnicode(szVal, pVar->bstrVal, cbTmp + 1);  // cbTmp does not include null
    }

    return hr;
}

#if 0   // currently unused (and untested)
HRESULT CRegStrPropBag::Write(/*[in]*/ LPCOLESTR pwzPropName,
        /*[in]*/ VARIANT *pVar)
{
    HRESULT hr;

    ASSERT(pVar->vt == VT_BSTR);

    // convert to ansi since we want REG_SZ and thus must use RegSetValueA
    char szName[MAX_PATH];
    char szVal[MAX_URL_STRING];

    SHUnicodeToAnsi(pwzPropName, szName, ARRAYSIZE(szName));
    SHUnicodeToAnsi(pVar->bstrVal, szVal, ARRAYSIZE(szVal));

    hr = SetValueStr(szName, szVal);

    return hr;
}
#endif

// }

//***   CRegStrPropBag::THISCLASS::* {

//***
//  hkey        e.g. HKLM
//  pszSubKey   e.g. "...\\Explorer\\Instance\\{...}"
//  grfMode     subset of STGM_* values
HRESULT CRegStrPropBag::SetRoot(HKEY hkeyNew, DWORD grfMode)
{
    ASSERT(grfMode == STGM_READ || grfMode == STGM_WRITE);
    if (_hkey) {
        RegCloseKey(_hkey);
        _grfMode = 0;
        _hkey = 0;
    }

    if (hkeyNew) {
        _grfMode = grfMode;
        _hkey = SHRegDuplicateHKey(hkeyNew);    // xfer ownership (and up khey refcnt)
        if (_hkey == NULL)
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CRegStrPropBag::ChDir(LPCTSTR pszSubKey)
{
    HRESULT hr = E_FAIL;
    HKEY hkeyNew;

    ASSERT(_hkey);
    ASSERT(_grfMode == STGM_READ || _grfMode == STGM_WRITE);

    hkeyNew = Reg_CreateOpenKey(_hkey, pszSubKey, _grfMode);
    if (hkeyNew) {
        RegCloseKey(_hkey);
        _hkey = hkeyNew;
        hr = S_OK;
    }

    return hr;
}

HRESULT CRegStrPropBag::QueryValueStr(LPCTSTR pszName, LPTSTR pszVal, LPDWORD pcbVal)
{
    long i;
    CHAR szSubKey[MAXIMUM_SUB_KEY_LENGTH];
    CHAR szStrData[MAX_URL_STRING];
    DWORD cbSize = sizeof(szStrData);

    SHTCharToAnsi(pszName, szSubKey, ARRAYSIZE(szSubKey));
    i = SHQueryValueExA(_hkey, szSubKey, NULL, NULL, (BYTE*)szStrData, &cbSize);
    ASSERT((int)*pcbVal > lstrlenA(szStrData));    // I better be using a big enough buffer.
    SHAnsiToTChar(szStrData, pszVal, *pcbVal / sizeof(TCHAR));

    if (pcbVal)
        *pcbVal = lstrlen(pszVal);

    return (i == ERROR_SUCCESS) ? S_OK : E_FAIL;
}

HRESULT CRegStrPropBag::SetValueStr(LPCTSTR pszName, LPCTSTR pszVal)
{
    return SetValueStrEx(pszName, REG_SZ, pszVal);
}

HRESULT CRegStrPropBag::SetValueStrEx(LPCTSTR pszName, DWORD dwType, LPCTSTR pszVal)
{
    long i;
    int cbTmp;
    CHAR szSubKey[MAXIMUM_SUB_KEY_LENGTH];
    CHAR szStrData[MAX_URL_STRING];

    SHTCharToAnsi(pszName, szSubKey, ARRAYSIZE(szSubKey));
    SHTCharToAnsi(pszVal, szStrData, ARRAYSIZE(szStrData));

    ASSERT(_grfMode == STGM_WRITE);
    cbTmp = (lstrlenA(szStrData) + 1);
    i = RegSetValueExA(_hkey, szSubKey, NULL, dwType, (BYTE*)szStrData, cbTmp);
    return (i == ERROR_SUCCESS) ? S_OK : E_FAIL;
}

// }

// }

// {
//***   CRegStrFS -- file-system-like view of registry
// DESCRIPTION
//  basically keeps track of where we are and does 'relative' opens from
// there.  NYI: intent is to eventually support 'chdir' ops.
// NOTES
//  BUGBUG: can only 'chdir' down, not up (no '..').
//

CRegStrFS *CRegStrFS_Create(HKEY hk, DWORD grfMode)
{
    CRegStrFS *prsfs = new CRegStrFS;
    if (prsfs && FAILED(prsfs->Initialize(hk, grfMode))) {
        prsfs->Release();
        prsfs = NULL;
    }

    return prsfs;
}

//***   IsREG_XX_SZ -- see if ansi/unicode is an issue
//
#define IsREG_XX_SZ(dwTyp) \
    ((dwTyp) == REG_SZ || (dwTyp) == REG_MULTI_SZ || (dwTyp) == REG_EXPAND_SZ)

HRESULT CRegStrFS::QueryValue(LPCTSTR pszName, BYTE *pbData, LPDWORD pcbData)
{
    long i;
    DWORD dwType;
    CHAR szaTmp[MAX_URL_STRING];

    // we need to thunk since no RegXxxValueW on win95
    SHTCharToAnsi(pszName, szaTmp, ARRAYSIZE(szaTmp));

    i = SHQueryValueExA(_hkey, szaTmp, NULL, &dwType, pbData, pcbData);
    ASSERT(i != ERROR_SUCCESS || !IsREG_XX_SZ(dwType));
    return (i == ERROR_SUCCESS) ? S_OK : E_FAIL;
}

HRESULT CRegStrFS::SetValue(LPCTSTR pszName, DWORD dwType, const BYTE *pbData, DWORD cbData)
{
    long i;
    CHAR szaTmp[MAX_URL_STRING];

    ASSERT(_grfMode == STGM_WRITE);

    // we need to thunk since no RegXxxValueW on win95
    SHTCharToAnsi(pszName, szaTmp, ARRAYSIZE(szaTmp));
    ASSERT(!IsREG_XX_SZ(dwType));

    i = RegSetValueExA(_hkey, szaTmp, NULL, dwType, pbData, cbData);
    return (i == ERROR_SUCCESS) ? S_OK : E_FAIL;
}

HRESULT CRegStrFS::DeleteValue(LPCTSTR pszName)
{
    long i;

    ASSERT(_grfMode == STGM_WRITE);
    i = SHDeleteValue(_hkey, NULL, pszName);
    return (i == ERROR_SUCCESS) ? S_OK : E_FAIL;
}

HRESULT CRegStrFS::RmDir(LPCTSTR pszName, BOOL fRecurse)
{
    HRESULT hr = E_FAIL;
    DWORD i;

    CHAR szaTmp[MAX_URL_STRING];

    ASSERT(fRecurse);   // others NYI

    ASSERT(_grfMode == STGM_WRITE);

    SHTCharToAnsi(pszName, szaTmp, ARRAYSIZE(szaTmp));

    if (fRecurse) {
        i = SHDeleteKeyA(_hkey, szaTmp);
    }
    else {
        ASSERT(0);          // unused
        // not sure what to do, since we want a non-recursive delete
        // but we do want to handle presence of values (which shlwapi
        // doesn't support)
        //i = DeleteEmptyKey(_hkey, pszName);
        i = -1;
    }

    return (i == ERROR_SUCCESS) ? S_OK : E_FAIL;
}


// }

//***   utils {

//***   IPBag_ReadStr, WriteStr -- IPBag->Read/Write w/o the hassles
// NOTES
//  'hassles' means variant and OLESTR stuff
//  WARNING: pwzPropName is an OLESTR, buf is a TSTR (we use an OLESTR since
//  name is usually static so no sense doing extra runtime convert here).
HRESULT IPBag_ReadStr(IPropertyBag *pPBag, LPCOLESTR pwzPropName, LPTSTR buf, int cch)
{
    HRESULT hr;
    VARIANT va = { 0 };

    va.vt = VT_BSTR;
    hr = pPBag->Read(pwzPropName, &va, NULL);
    if (SUCCEEDED(hr))
    {    
#ifdef UNICODE
        StrCpyNW(buf, va.bstrVal, cch);
#else
        SHUnicodeToAnsi(va.bstrVal, buf, cch);
#endif
    }
    
    VariantClear(&va);

    return hr;
}

#if 0 // (currently) unused (but has been tested)
HRESULT IPBag_WriteStr(IPropertyBag *pPBag, LPCOLESTR pwzPropName, LPTSTR buf)
{
    HRESULT hr;
    VARIANT va = { 0 };

    va.vt = VT_BSTR;
    int cch = lstrlen(buf) + 1;
    va.bstrVal = SysAllocStringLen(NULL, cch);
#ifdef UNICODE
    StrCpyNW(va.bstrVal, buf, cch);
#else
    SHAnsiToUnicode(buf, va.bstrVal, cch);
#endif
    hr = pPBag->Write(pwzPropName, &va);

    VariantClear(&va);  // SysFreeString

    return hr;
}
#endif

//***   Reg_CreateOpenKey -- merged RegCreate/RegOpen
//
HKEY Reg_CreateOpenKey(HKEY hkey, LPCTSTR pszSubKey, DWORD grfMode)
{
    HKEY hkeyNew = 0;
    long i;
    CHAR szSubKey[MAXIMUM_SUB_KEY_LENGTH];

    SHTCharToAnsi(pszSubKey, szSubKey, ARRAYSIZE(szSubKey));
    ASSERT(grfMode == STGM_READ || grfMode == STGM_WRITE);

    ASSERT(pszSubKey != NULL);
    if (pszSubKey != NULL) {
        if (grfMode == STGM_READ) {
            i = RegOpenKeyA(hkey, szSubKey, &hkeyNew);
        }
        else {
            i = RegCreateKeyA(hkey, szSubKey, &hkeyNew);
        }
        // the following ASSERT is not req'd but is currently always true.
        // if you hit it, the bug is someplace in the caller.
        // i.e. do *not* remove this ASSERT to 'fix' your pblm.
        // a typical cause of failure is that CoCreateInst is failing because
        // something isn't properly regsvr32'ed.
        ASSERT(i == ERROR_SUCCESS || hkeyNew == 0);
    }

    return hkeyNew;
}

// }
