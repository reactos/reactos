#include "shole.h"
#include "ids.h"
#include "scguid.h"

extern TEXT("C") const TCHAR c_szCLSID[];

class CTemplateFolder : public IShellFolder, public IPersistFolder
{
public:
    CTemplateFolder();
    ~CTemplateFolder();

protected:
    // IUnKnown
    virtual HRESULT __stdcall QueryInterface(REFIID,void **);
    virtual ULONG   __stdcall AddRef(void);
    virtual ULONG   __stdcall Release(void);

    // IShellFolder
    virtual HRESULT __stdcall ParseDisplayName(HWND hwndOwner,
        LPBC pbcReserved, LPOLESTR lpszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);

    virtual HRESULT __stdcall EnumObjects( THIS_ HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList);

    virtual HRESULT __stdcall BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvOut);
    virtual HRESULT __stdcall BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvObj);
    virtual HRESULT __stdcall CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    virtual HRESULT __stdcall CreateViewObject (HWND hwndOwner, REFIID riid, LPVOID * ppvOut);
    virtual HRESULT __stdcall GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl,
                                    ULONG * rgfInOut);
    virtual HRESULT __stdcall GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                 REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
    virtual HRESULT __stdcall GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    virtual HRESULT __stdcall SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,
                                 LPCOLESTR lpszName, DWORD uFlags,
                                 LPITEMIDLIST * ppidlOut);

    // IPersistFolder
    virtual HRESULT __stdcall GetClassID(LPCLSID lpClassID);
    virtual HRESULT __stdcall Initialize(LPCITEMIDLIST pidl);

    // Defview callback
    friend HRESULT CALLBACK DefViewCallback(
                                LPSHELLVIEW psvOuter, LPSHELLFOLDER psf,
                                HWND hwndOwner, UINT uMsg,
                                WPARAM wParam, LPARAM lParam);

    BOOL IsMyPidl(LPCITEMIDLIST pidl)
        { return pidl->mkid.cb == SIZEOF(pidl->mkid.cb)+SIZEOF(CLSID); }

    UINT _cRef;
};

class CEnumTemplate : public IEnumIDList
{
public:
    CEnumTemplate();
    ~CEnumTemplate();

protected:
    // IUnKnown
    virtual HRESULT __stdcall QueryInterface(REFIID,void **);
    virtual ULONG   __stdcall AddRef(void);
    virtual ULONG   __stdcall Release(void);

    virtual HRESULT __stdcall Next(ULONG celt,
                      LPITEMIDLIST *rgelt,
                      ULONG *pceltFetched);
    virtual HRESULT __stdcall Skip(ULONG celt);
    virtual HRESULT __stdcall Reset();
    virtual HRESULT __stdcall Clone(IEnumIDList **ppenum);

    UINT        _cRef;
    UINT        _iCur;
    HKEY        _hkeyCLSID;
    struct {
        ITEMIDLIST  idl;
        BYTE        __abRest[255];      // Enough for CLSID or ProgID
    } _tidl;
};


class CTemplateUIObj : public IExtractIcon, public IDataObject
{
public:
    static HRESULT Create(REFCLSID, REFIID, LPVOID*);
protected:
    CTemplateUIObj(REFCLSID rclsid)
                    : _clsid(rclsid), _cRef(1)
                                { g_cRefThisDll++; }
    ~CTemplateUIObj()           { g_cRefThisDll--; }
    HRESULT _CreateInstance(IStorage* pstg);

    // IUnKnown
    virtual HRESULT __stdcall QueryInterface(REFIID,void **);
    virtual ULONG   __stdcall AddRef(void);
    virtual ULONG   __stdcall Release(void);

    // *** IExtractIcon methods ***
    virtual HRESULT __stdcall GetIconLocation(
                         UINT   uFlags, LPTSTR  szIconFile,
                         UINT   cchMax, int   * piIndex,
                         UINT  * pwFlags);

    virtual HRESULT __stdcall Extract(
                           LPCTSTR pszFile, UINT          nIconIndex,
                           HICON   *phiconLarge, HICON   *phiconSmall,
                           UINT    nIconSize);

    // IDataObject
    virtual HRESULT __stdcall GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
    virtual HRESULT __stdcall GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
    virtual HRESULT __stdcall QueryGetData(FORMATETC *pformatetc);
    virtual HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut);
    virtual HRESULT __stdcall SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
    virtual HRESULT __stdcall EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
    virtual HRESULT __stdcall DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
    virtual HRESULT __stdcall DUnadvise(DWORD dwConnection);
    virtual HRESULT __stdcall EnumDAdvise(IEnumSTATDATA **ppenumAdvise);

    UINT        _cRef;
    CLSID       _clsid;
};


CTemplateFolder::CTemplateFolder() : _cRef(1)
{
    OleInitialize(NULL);
    g_cRefThisDll++;
}

CTemplateFolder::~CTemplateFolder()
{
    OleUninitialize();
    g_cRefThisDll--;
}

HRESULT CTemplateFolder::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IShellFolder) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IShellFolder*)this;
        _cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersistFolder))
    {
        *ppvObj = (IPersistFolder*)this;
        _cRef++;
        return S_OK;
    }
    *ppvObj = NULL;

    return E_NOINTERFACE;
}

ULONG CTemplateFolder::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CTemplateFolder::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTemplateFolder_CreateInstnace(LPUNKNOWN * ppunk)
{
    CTemplateFolder* ptfld = new CTemplateFolder();
    if (ptfld) {
        *ppunk = (IShellFolder *)ptfld;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT CTemplateFolder::ParseDisplayName(HWND hwndOwner,
        LPBC pbcReserved, LPOLESTR lpszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes)
{
    return E_NOTIMPL;
}

HRESULT CTemplateFolder::EnumObjects(HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList)
{
    *ppenumIDList = new CEnumTemplate();
    return S_OK;
}

HRESULT CTemplateFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvOut)
{
    return E_NOTIMPL;
}

HRESULT CTemplateFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvObj)
{
    return E_NOTIMPL;
}

HRESULT CTemplateFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    return E_NOTIMPL;
}


HRESULT CALLBACK DefViewCallback(LPSHELLVIEW psvOuter, LPSHELLFOLDER psf,
                                HWND hwndOwner, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // BUGBUG: DefView GPF if I don't pass the callback function!
    // BUGBUG: DefView GPF if it returns S_FALSE as the default!
    return E_FAIL; // S_FALSE;
}


HRESULT CTemplateFolder::CreateViewObject (HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    if (IsEqualIID(riid, IID_IShellView))
    {
        CSFV csfv = {
            SIZEOF(CSFV),       // cbSize
            this,               // pshf
            NULL,               // psvOuter
            NULL,               // pidl
            0,
            DefViewCallback,    // pfnCallback
            FVM_ICON,
        };
        return SHCreateShellFolderViewEx(&csfv, (LPSHELLVIEW *)ppvOut);
    }
    return E_NOINTERFACE;
}

HRESULT CTemplateFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl,
                                    ULONG * rgfInOut)
{
    UINT rgfOut = SFGAO_CANCOPY | SFGAO_HASPROPSHEET;
    *rgfInOut = rgfOut;
    return S_OK;
}

HRESULT CTemplateFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                 REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    HRESULT hres = E_INVALIDARG;
    if (cidl==1 && IsMyPidl(apidl[0]))
    {
        const CLSID* pclsid = (const CLSID*)&apidl[0]->mkid.abID;
        hres = CTemplateUIObj::Create(*pclsid, riid, ppvOut);
    }

    return hres;
}

HRESULT _KeyNameFromCLSID(REFCLSID rclsid, LPTSTR pszKey, UINT cchMax)
{
    LPWSTR pwszKey;
    HRESULT hres = StringFromCLSID(rclsid, &pwszKey);
    if (FAILED(hres)) {
        return E_INVALIDARG;
    }

    lstrcpyn(pszKey, TEXT("CLSID\\"), cchMax);
    WideCharToMultiByte(CP_ACP, 0, pwszKey, -1, pszKey+6, cchMax-6, NULL, NULL);

    // BUGBUG: Use TASK ALLOCATOR!!!
    LocalFree(pwszKey);
    return S_OK;
}

HRESULT CTemplateFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName)
{
    if (!IsMyPidl(pidl)) {
        return E_INVALIDARG;
    }

    LPCLSID const pclsid = (LPCLSID)pidl->mkid.abID;
    TCHAR szKey[128];
    HRESULT hres = _KeyNameFromCLSID(*pclsid, szKey, ARRAYSIZE(szKey));
    if (FAILED(hres)) {
        return hres;
    }

    LONG dwSize = ARRAYSIZE(lpName->cStr);
    if (RegQueryValue(HKEY_CLASSES_ROOT, szKey, lpName->cStr, &dwSize) == ERROR_SUCCESS)
    {
        lpName->uType = STRRET_CSTR;
        return S_OK;
    }

    return E_FAIL;
}

HRESULT CTemplateFolder::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,
                                 LPCOLESTR lpszName, DWORD uFlags,
                                 LPITEMIDLIST * ppidlOut)
{
    return E_NOTIMPL;
}

HRESULT __stdcall CTemplateFolder::GetClassID(LPCLSID lpClassID)
{
    *lpClassID = CLSID_CTemplateFolder;
    return S_OK;
}

HRESULT __stdcall CTemplateFolder::Initialize(LPCITEMIDLIST pidl)
{
    return S_OK;
}

CEnumTemplate::CEnumTemplate() : _cRef(1), _iCur(0), _hkeyCLSID(NULL)
{
    g_cRefThisDll++;
}

CEnumTemplate::~CEnumTemplate()
{
    if (_hkeyCLSID) {
        RegCloseKey(_hkeyCLSID);
    }
    g_cRefThisDll--;
}

HRESULT CEnumTemplate::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IEnumIDList) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IEnumIDList*)this;
        _cRef++;
        return S_OK;
    }
    *ppvObj = NULL;

    return E_NOINTERFACE;
}

ULONG CEnumTemplate::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CEnumTemplate::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CEnumTemplate::Next(ULONG celt,
                      LPITEMIDLIST *rgelt,
                      ULONG *pceltFetched)
{
    // Assume error
    if (pceltFetched) {
        *pceltFetched = 0;
    }

    if (!_hkeyCLSID)
    {
        if (RegOpenKey(HKEY_CLASSES_ROOT, c_szCLSID, &_hkeyCLSID) != ERROR_SUCCESS)
        {
            return E_FAIL;
        }
    }

    TCHAR szKey[40];    // enough for {CLSID}
    while (RegEnumKey(_hkeyCLSID, _iCur++, szKey, ARRAYSIZE(szKey)) == ERROR_SUCCESS)
    {
        TCHAR szInsertable[128];        // enough for "{CLSID}/Insertable"
        wsprintf(szInsertable, TEXT("%s\\Insertable"), szKey);
        HKEY hkeyT;

        if (RegOpenKey(_hkeyCLSID, szInsertable, &hkeyT) == ERROR_SUCCESS)
        {
            RegCloseKey(hkeyT);

            CLSID clsid;
            WCHAR wszKey[40];
            MultiByteToWideChar(CP_ACP, 0, szKey, -1, wszKey, ARRAYSIZE(wszKey));
            HRESULT hres = CLSIDFromString(wszKey, &clsid);
            if (SUCCEEDED(hres))
            {
                _tidl.idl.mkid.cb = SIZEOF(_tidl.idl.mkid.cb) + SIZEOF(clsid);
                memcpy(_tidl.idl.mkid.abID, &clsid, SIZEOF(clsid));
                LPITEMIDLIST pidl = _ILNext(&_tidl.idl);
                pidl->mkid.cb = 0;      // Terminator
                rgelt[0] = ILClone(&_tidl.idl);
                *pceltFetched = 1;
                return S_OK;
            }
        }
    }

    return S_FALSE;     // no more element
}

HRESULT CEnumTemplate::Skip(ULONG celt)
{
    return E_NOTIMPL;
}

HRESULT CEnumTemplate::Reset()
{
    return E_NOTIMPL;
}

HRESULT CEnumTemplate::Clone(IEnumIDList **ppenum)
{
    return E_NOTIMPL;
}


//==========================================================================
// CTemplateUIObj members (IUnknown override)
//==========================================================================

HRESULT CTemplateUIObj::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IExtractIcon) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IExtractIcon*)this;
        _cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IDataObject))
    {
        *ppvObj = (IDataObject*)this;
        _cRef++;
        return S_OK;
    }
    *ppvObj = NULL;

    return E_NOINTERFACE;
}

ULONG CTemplateUIObj::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CTemplateUIObj::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

//
// NOTES: This logic MUST be identical to the one in the shell.
//
inline int _ParseIconLocation(LPTSTR pszIconFile)
{
    int iIndex = 0;
    LPTSTR pszComma = StrChr(pszIconFile, TEXT(','));

    if (pszComma) {
        *pszComma++ = 0;            // terminate the icon file name.
        iIndex = StrToInt(pszComma);
    }
    PathRemoveBlanks(pszIconFile);
    return iIndex;
}


//==========================================================================
// CTemplateUIObj members (IExtractIcon override)
//==========================================================================

HRESULT CTemplateUIObj::GetIconLocation(
                         UINT   uFlags, LPTSTR  szIconFile,
                         UINT   cchMax, int   * piIndex,
                         UINT  * pwFlags)
{
    TCHAR szKey[128];
    HRESULT hres = _KeyNameFromCLSID(_clsid, szKey, ARRAYSIZE(szKey));
    if (SUCCEEDED(hres))
    {
        lstrcat(szKey, TEXT("\\DefaultIcon"));  // BUGBUG: lstrcatn?
        TCHAR szValue[MAX_PATH+40];
        LONG dwSize = ARRAYSIZE(szValue);
        if (RegQueryValue(HKEY_CLASSES_ROOT, szKey, szValue, &dwSize) == ERROR_SUCCESS)
        {
            *pwFlags = GIL_PERCLASS;
            *piIndex = _ParseIconLocation(szValue);
            lstrcpyn(szIconFile, szValue, cchMax);
            hres = S_OK;
        }
    }
    return hres;
}

HRESULT CTemplateUIObj::Extract(
                           LPCTSTR pszFile, UINT          nIconIndex,
                           HICON   *phiconLarge, HICON   *phiconSmall,
                           UINT    nIconSize)
{
    return S_FALSE;
}

HRESULT CTemplateUIObj::Create(REFCLSID rclsid, REFIID riid, LPVOID* ppvOut)
{
    CTemplateUIObj *pti = new CTemplateUIObj(rclsid);
    if (pti) {
        pti->QueryInterface(riid, ppvOut);
        pti->Release();
        return S_OK;
    }

    *ppvOut=NULL;
    return E_OUTOFMEMORY;
}

//==========================================================================
// CTemplateUIObj members (IDataObject override)
//==========================================================================

HRESULT CTemplateUIObj::_CreateInstance(IStorage* pstg)
{
    HRESULT hres;
    IPersistStorage* pps = NULL;
    hres = OleCreate(_clsid, IID_IPersistStorage, OLERENDER_DRAW,
                     NULL, NULL, pstg, (LPVOID*)&pps);
    DebugMsg(DM_TRACE, TEXT("so TR - TUO:CI OleCreate returned (%x)"), hres);

    if (SUCCEEDED(hres))
    {
        hres = OleSave(pps, pstg, TRUE);
        DebugMsg(DM_TRACE, TEXT("so TR - TUO:CI OleSave returned (%x)"), hres);
        pps->HandsOffStorage();
        pps->Release();

        if (SUCCEEDED(hres))
        {
            hres = pstg->Commit(STGC_OVERWRITE);
            DebugMsg(DM_TRACE, TEXT("so TR - TUO:CI pstg->Commit returned (%x)"), hres);
        }
    }

    return hres;
}

HRESULT CTemplateUIObj::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    HRESULT hres = DATA_E_FORMATETC;

    pmedium->pUnkForRelease = NULL;
    pmedium->pstg = NULL;

    //
    // NOTES: We should avoid calling _OpenStorage if we don't support
    //  the format.
    //

    if (pformatetcIn->cfFormat == CF_EMBEDDEDOBJECT
        && pformatetcIn->tymed == TYMED_ISTORAGE)
    {
        IStorage* pstg = NULL;
        hres = StgCreateDocfile(NULL, STGM_DIRECT | STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pstg);
        DebugMsg(DM_TRACE, TEXT("so TR - TUO:GD StgCreateDocfile returned (%x)"), hres);
        if (SUCCEEDED(hres))
        {
            hres = _CreateInstance(pstg);
            if (SUCCEEDED(hres)) {
                pmedium->tymed = TYMED_ISTORAGE;
                pmedium->pstg = pstg;
            } else {
                pstg->Release();
            }
        }
    }
    return hres;
}

HRESULT CTemplateUIObj::GetDataHere(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    HRESULT hres = DATA_E_FORMATETC;

    if (pformatetcIn->cfFormat == CF_EMBEDDEDOBJECT
        && pformatetcIn->tymed == TYMED_ISTORAGE && pmedium->tymed == TYMED_ISTORAGE)
    {
        hres = _CreateInstance(pmedium->pstg);
    }

    return hres;
}

HRESULT CTemplateUIObj::QueryGetData(LPFORMATETC pformatetcIn)
{
    if (pformatetcIn->cfFormat == CF_EMBEDDEDOBJECT
        && pformatetcIn->tymed == TYMED_ISTORAGE)
    {
        return S_OK;
    }
    return DATA_E_FORMATETC;
}

HRESULT CTemplateUIObj::GetCanonicalFormatEtc(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
    //
    //  This is the simplest implemtation. It means we always return
    // the data in the format requested.
    //
    return ResultFromScode(DATA_S_SAMEFORMATETC);
}

HRESULT CTemplateUIObj::SetData(LPFORMATETC pformatetc, STGMEDIUM  * pmedium, BOOL fRelease)
{
    return E_FAIL;
}

HRESULT CTemplateUIObj::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC * ppenumFormatEtc)
{
    static FORMATETC s_afmt[] = { CF_EMBEDDEDOBJECT };
    return SHCreateStdEnumFmtEtc(ARRAYSIZE(s_afmt), s_afmt, ppenumFormatEtc);
}

HRESULT CTemplateUIObj::DAdvise(FORMATETC * pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD * pdwConnection)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

HRESULT CTemplateUIObj::DUnadvise(DWORD dwConnection)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

HRESULT CTemplateUIObj::EnumDAdvise(LPENUMSTATDATA * ppenumAdvise)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}
