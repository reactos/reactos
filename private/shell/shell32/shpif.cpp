//
//  CPifFile class
//
//  supports:
//
//      IPersistFile    - "load" a PIF file
//      IExtractIcon    - extract a icon from a PIF file.
//

#define NO_INCLUDE_UNION

#include "shellprv.h"

////////////////////////////////////////////////////////////////////////
//  PifFile class
////////////////////////////////////////////////////////////////////////

class PifFile : IShellExtInit, IExtractIcon, IPersistFile
#ifdef UNICODE
    , IExtractIconA
#endif
{

public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IShellExtInit methods ***
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder,LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    // *** IPersist methods ***
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // *** IPersistFile methods ***
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName);
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName);

    // *** IExtractIcon methods ***
    STDMETHODIMP GetIconLocation(UINT uFlags,LPTSTR szIconFile,UINT cchMax,int *piIndex,UINT * pwFlags);
    STDMETHODIMP ExtractIcon(LPCTSTR pszFile,UINT nIconIndex,HICON *phiconLarge,HICON *phiconSmall,UINT nIcons);

#ifdef UNICODE
    // *** IExtractIconA methods ***
    STDMETHODIMP GetIconLocation(UINT uFlags,LPSTR szIconFile,UINT cchMax,int *piIndex,UINT * pwFlags);
    STDMETHODIMP ExtractIcon(LPCSTR pszFile,UINT nIconIndex,HICON *phiconLarge,HICON *phiconSmall,UINT nIcons);
#endif

    PifFile();
    ~PifFile();

    //
    // data
    //
private:
    UINT                cRef;
    int                 hPifProps;
};

////////////////////////////////////////////////////////////////////////
//
//  CPifFile_CreateInstance
//
//      public function to create a instance of a CPifFile
//
////////////////////////////////////////////////////////////////////////

STDAPI CPifFile_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, LPVOID * ppvOut)
{
    HRESULT hres;
    PifFile *p;

    // does not support aggregation.
    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    p = new PifFile();

    if (p == NULL)
        return E_FAIL;

    //
    // Note that the Release member will free the object, if QueryInterface
    // failed.
    //
    hres = p->QueryInterface(riid, ppvOut);
    p->Release();

    return hres;        // S_OK or E_NOINTERFACE
}

////////////////////////////////////////////////////////////////////////
//  constuct/destruct
////////////////////////////////////////////////////////////////////////

PifFile::PifFile()
{
    this->cRef = 1;
}

PifFile::~PifFile()
{
    if (hPifProps)
        PifMgr_CloseProperties(hPifProps, 0);
    hPifProps=0;
}

////////////////////////////////////////////////////////////////////////
//  IUnknown
////////////////////////////////////////////////////////////////////////

STDMETHODIMP PifFile::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        DebugMsg(DM_TRACE, TEXT("PifFile::QueryInterface(IUnknown)"));
        *ppvObj = (IUnknown *)this;
    }
    else if (IsEqualIID(riid, IID_IShellExtInit))
    {
        DebugMsg(DM_TRACE, TEXT("PifFile::QueryInterface(IShellExtInit)"));
        *ppvObj = (IShellExtInit*)this;
    }
    else if (IsEqualIID(riid, IID_IPersistFile))
    {
        DebugMsg(DM_TRACE, TEXT("PifFile::QueryInterface(IPersistFile)"));
        *ppvObj = (IPersistFile*)this;
    }
    else if (IsEqualIID(riid, IID_IExtractIcon))
    {
        DebugMsg(DM_TRACE, TEXT("PifFile::QueryInterface(IExtractIcon)"));
        *ppvObj = (IExtractIcon*)this;
    }
#ifdef UNICODE
    else if (IsEqualIID(riid, IID_IExtractIconA))
    {
        DebugMsg(DM_TRACE, TEXT("PifFile::QueryInterface(IExtractIconA)"));
        *ppvObj = (IExtractIconA*)this;
    }
#endif
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) PifFile::AddRef()
{
    DebugMsg(DM_TRACE, TEXT("PifFile::AddRef() ==> %d"), this->cRef+1);

    this->cRef++;

    return this->cRef;
}

STDMETHODIMP_(ULONG) PifFile::Release()
{
    DebugMsg(DM_TRACE, TEXT("PifFile::Release() ==> %d"), this->cRef-1);

    this->cRef--;

    if (this->cRef>0)
    {
        return this->cRef;
    }

    delete this;

    return 0;
}

////////////////////////////////////////////////////////////////////////
//  IShellExtInit
////////////////////////////////////////////////////////////////////////

STDMETHODIMP PifFile::Initialize(LPCITEMIDLIST pidlFolder,LPDATAOBJECT lpdobj, HKEY hkeyProgID)
{
    DebugMsg(DM_TRACE, TEXT("PifFile::Initialize()"));
    return S_OK;
}

////////////////////////////////////////////////////////////////////////
//  IPersistFile
////////////////////////////////////////////////////////////////////////

STDMETHODIMP PifFile::GetClassID(LPCLSID lpClassID)
{
    DebugMsg(DM_TRACE, TEXT("PifFile::GetClass()"));

    *lpClassID = CLSID_PifFile;
    return NOERROR;
}

STDMETHODIMP PifFile::IsDirty()
{
    DebugMsg(DM_TRACE, TEXT("PifFile::IsDirty()"));

    return S_FALSE;
}

STDMETHODIMP PifFile::Load(LPCOLESTR pwszFile, DWORD grfMode)
{
    TCHAR szPath[MAX_PATH];

    SHUnicodeToTChar(pwszFile, szPath, ARRAYSIZE(szPath));

    DebugMsg(DM_TRACE, TEXT("PifFile::Load(%s)"), szPath);

    if (hPifProps)
        PifMgr_CloseProperties(hPifProps, 0);

    hPifProps = PifMgr_OpenProperties(szPath, NULL, 0, 0);

    return hPifProps == 0 ? E_FAIL : S_OK;
}

STDMETHODIMP PifFile::Save(LPCOLESTR pwszFile, BOOL fRemember)
{
    return E_NOTIMPL;
}

STDMETHODIMP PifFile::SaveCompleted(LPCOLESTR pwszFile)
{
    return E_NOTIMPL;
}

STDMETHODIMP PifFile::GetCurFile(LPOLESTR * lplpszFileName)
{
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////
//  IExtractIcon
////////////////////////////////////////////////////////////////////////

STDMETHODIMP PifFile::GetIconLocation(UINT uFlags, LPTSTR szIconFile,UINT cchMax,int *piIndex,UINT * pwFlags)
{
    PROPPRG ProgramProps;

    VDATEINPUTBUF(szIconFile, TCHAR, cchMax);

    if (hPifProps == 0)
        return E_FAIL;

    if (!PifMgr_GetProperties(hPifProps,MAKEINTATOM(GROUP_PRG), &ProgramProps, SIZEOF(ProgramProps), 0))
    {
        DebugMsg(DM_TRACE, TEXT("PifFile::GetIconLocation() PifMgr_GetProperties *failed*"));
        return E_FAIL;
    }

    if (ProgramProps.achIconFile[0] == 0)
    {
        lstrcpy(szIconFile, ICONFILE_DEFAULT);
        *piIndex = ICONINDEX_DEFAULT;
    }
    else
    {
        lstrcpy(szIconFile, ProgramProps.achIconFile);
        *piIndex = ProgramProps.wIconIndex;
    }
    *pwFlags = 0;

    DebugMsg(DM_TRACE, TEXT("PifFile::GetIconLocation() ==> %s!%d"), szIconFile, *piIndex);
    return S_OK;
}

STDMETHODIMP PifFile::ExtractIcon(LPCTSTR pszFile,UINT nIconIndex,HICON *phiconLarge,HICON *phiconSmall,UINT nIcons)
{
    DebugMsg(DM_TRACE, TEXT("PifFile::ExtractIcon()"));
    return E_NOTIMPL;
}

#ifdef UNICODE
////////////////////////////////////////////////////////////////////////
//  IExtractIconA
////////////////////////////////////////////////////////////////////////

STDMETHODIMP PifFile::GetIconLocation(UINT uFlags, LPSTR pszIconFile, UINT cchMax,int *piIndex, UINT * pwFlags)
{
    WCHAR szIconFile[MAX_PATH];
    HRESULT hres;

    VDATEINPUTBUF(pszIconFile, TCHAR, cchMax);

    DebugMsg(DM_TRACE, TEXT("PifFile::IExtractIconA::GetIconLocation()"));

    hres = this->GetIconLocation(uFlags,szIconFile,ARRAYSIZE(szIconFile), piIndex, pwFlags);

    //
    // We don't want to copy the icon file name on the S_FALSE case
    //
    if (SUCCEEDED(hres) && hres != S_FALSE)
    {
        SHUnicodeToAnsi(szIconFile, pszIconFile, cchMax);
    }
    return hres;
}

STDMETHODIMP PifFile::ExtractIcon(LPCSTR pszFile,UINT nIconIndex,HICON *phiconLarge,HICON *phiconSmall,UINT nIcons)
{
    DebugMsg(DM_TRACE, TEXT("PifFile::IExtractIconA::ExtractIcon()"));
    return E_NOTIMPL;
}
#endif
