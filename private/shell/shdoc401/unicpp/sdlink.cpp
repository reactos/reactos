#include "stdafx.h"
#pragma hdrstop

//#include "dutil.h"
//#include "sdspatch.h"


class CSDShellLink : public IShellLinkDual, protected CImpIDispatch
{
public:
    ULONG           m_cRef; //Public for debug checks


protected:
    HWND            m_hwndFldr;         // Hwnd of the main folder window
    LPSHELLFOLDER   m_psfFldr;
    LPITEMIDLIST    m_pidl;
    IShellLink      *m_psl;             // The link object...

public:
    CSDShellLink(HWND hwndFldr, LPSHELLFOLDER psfFldr);
    ~CSDShellLink(void);

    BOOL Init(LPITEMIDLIST pidl);

    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IDispatch members
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // IShellLinkDual methods
    STDMETHODIMP get_Path(BSTR *pbs);
    STDMETHODIMP put_Path(BSTR bs);
    STDMETHODIMP get_Description(BSTR *pbs);
    STDMETHODIMP put_Description(BSTR bs);
    STDMETHODIMP get_WorkingDirectory(BSTR *pbs);
    STDMETHODIMP put_WorkingDirectory(BSTR bs);
    STDMETHODIMP get_Arguments(BSTR *pbs);
    STDMETHODIMP put_Arguments(BSTR bs);
    STDMETHODIMP get_Hotkey(int *piHK);
    STDMETHODIMP put_Hotkey(int iHK);
    STDMETHODIMP get_ShowCommand(int *piShowCommand);
    STDMETHODIMP put_ShowCommand(int iShowCommand);
    STDMETHODIMP Resolve(int fFlags);
    STDMETHODIMP GetIconLocation(BSTR *pbs, int *piIcon);
    STDMETHODIMP SetIconLocation(BSTR bs, int iIcon);
    STDMETHODIMP Save(VARIANT vWhere);
};


HRESULT CSDShellLink_CreateIDispatch(HWND hwndFldr, LPSHELLFOLDER psfFldr, LPITEMIDLIST pidl, IDispatch ** ppid)
{
    HRESULT hres = E_OUTOFMEMORY;

    *ppid = NULL;

    CSDShellLink* psdf = new CSDShellLink(hwndFldr, psfFldr);

    if (psdf)
    {
        if (psdf->Init(pidl))
        {
             hres = psdf->QueryInterface(IID_IDispatch, (LPVOID *)ppid);
        }
        psdf->Release();
    }
    return hres;
}

CSDShellLink::CSDShellLink(HWND hwndFldr, LPSHELLFOLDER psfFldr) :
        CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_IShellLinkDual)
{
    DllAddRef();
    m_cRef = 1;
    m_hwndFldr = hwndFldr;
    m_psfFldr = psfFldr;
    m_psfFldr->AddRef();
    m_pidl = NULL;
    return;
}


CSDShellLink::~CSDShellLink(void)
{
    DllRelease();
    TraceMsg(DM_TRACE, "CSDShellLink::~CSDShellLink called");

    if (m_pidl)
        ILFree(m_pidl);
    m_psfFldr->Release();

    if (m_psl)
        m_psl->Release();

    return;
}

BOOL CSDShellLink::Init(LPITEMIDLIST pidl)
{
    LPUNKNOWN       pIUnknown=this;
    IPersistFile    *ppf;

    TraceMsg(DM_TRACE, "CSDShellLink::Init called");

    m_pidl = ILClone(pidl);
    if (!m_pidl)
        return FALSE;

    // Try to bind to this item with IShellLink
    LPCLASSFACTORY pcf;

    // BUGBUG: Revisit.  Why not just use CoCreateInstance()?
    HRESULT hres = SHDllGetClassObject(CLSID_ShellLink, IID_IClassFactory,
            (LPVOID*)&pcf);
    if (FAILED(hres))
        return FALSE;

    hres = pcf->CreateInstance(NULL, IID_IShellLink, (LPVOID*)&m_psl);
    pcf->Release();

    if (FAILED(hres))
        return FALSE;

    if (SUCCEEDED(m_psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf)))
    {
        // Now get fullpath for idlist
        TCHAR szPath[MAX_PATH];
        OLECHAR wszPath[MAX_PATH];
        STRRET strret;

        // Get path, conversion here is not optimal.
        m_psfFldr->GetDisplayNameOf(m_pidl, SHGDN_FORPARSING, &strret);
        StrRetToStrN(szPath, ARRAYSIZE(szPath), &strret, m_pidl);
        SHTCharToUnicodeCP(CP_ACP, szPath, wszPath, ARRAYSIZE(wszPath));
        hres = ppf->Load(wszPath, 0);
        ppf->Release();
    }

    return SUCCEEDED(hres);
}

STDMETHODIMP CSDShellLink::QueryInterface(REFIID riid, LPVOID * ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IDispatch) || 
        IsEqualIID(riid, IID_IShellLinkDual))
    {
        *ppv = SAFECAST(this, IUnknown*);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;

}

STDMETHODIMP_(ULONG) CSDShellLink::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSDShellLink::Release(void)
{
    if (0!=--m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}


STDMETHODIMP CSDShellLink::get_Path(BSTR *pbs)
{
    TCHAR szPath[MAX_PATH];

    HRESULT hres;

    *pbs = NULL;
    TraceMsg(DM_TRACE, "CSDShellLink::Get_GetPath called");
    if (SUCCEEDED(hres = m_psl->GetPath(szPath, ARRAYSIZE(szPath), NULL, 0)))
        *pbs = AllocBStrFromString(szPath);

    return hres;
}

STDMETHODIMP CSDShellLink::put_Path(BSTR bs)
{
    TCHAR szPath[MAX_PATH];

    if (bs)
    {
        SHUnicodeToTChar(bs, szPath, ARRAYSIZE(szPath));
        return m_psl->SetPath(szPath);
    }
    else
        return m_psl->SetPath(NULL);
}

STDMETHODIMP CSDShellLink::get_Description(BSTR *pbs)
{
    TCHAR szDescription[MAX_PATH];
    HRESULT hres = m_psl->GetDescription(szDescription, ARRAYSIZE(szDescription));
    if (SUCCEEDED(hres))
        *pbs = AllocBStrFromString(szDescription);
    else
        *pbs = NULL;
    return hres;
}

STDMETHODIMP CSDShellLink::put_Description(BSTR bs)
{
    TCHAR szDesc[MAX_PATH];

    if (bs)
    {
        SHUnicodeToTChar(bs, szDesc, ARRAYSIZE(szDesc));
        return m_psl->SetDescription(szDesc);
    }
    else
        return m_psl->SetDescription(NULL);
}

STDMETHODIMP CSDShellLink::get_WorkingDirectory(BSTR *pbs)
{
    TCHAR szWorkingDir[MAX_PATH];

    HRESULT hres;

    *pbs = NULL;
    TraceMsg(DM_TRACE, "CSDShellLink::Get_GetWorkingDirectory called");
    if (SUCCEEDED(hres = m_psl->GetWorkingDirectory(szWorkingDir, ARRAYSIZE(szWorkingDir))))
        *pbs = AllocBStrFromString(szWorkingDir);
    return hres;
}

STDMETHODIMP CSDShellLink::put_WorkingDirectory(BSTR bs)
{
    TCHAR szWorkingDir[MAX_PATH];

    if (bs)
    {
        SHUnicodeToTChar(bs, szWorkingDir, ARRAYSIZE(szWorkingDir));
        return m_psl->SetWorkingDirectory(szWorkingDir);
    }
    else
        return m_psl->SetWorkingDirectory(NULL);
}

STDMETHODIMP CSDShellLink::get_Arguments(BSTR *pbs)
{
    TCHAR szArgs[MAX_PATH];

    HRESULT hres;

    *pbs = NULL;
    TraceMsg(DM_TRACE, "CSDShellLink::Get_GetArguments called");
    if (SUCCEEDED(hres = m_psl->GetArguments(szArgs, ARRAYSIZE(szArgs))))
        *pbs = AllocBStrFromString(szArgs);
    return hres;
}

STDMETHODIMP CSDShellLink::put_Arguments(BSTR bs)
{
    TCHAR szArgs[MAX_PATH];

    if (bs)
    {
        SHUnicodeToTChar(bs, szArgs, ARRAYSIZE(szArgs));
        return m_psl->SetArguments(szArgs);
    }
    else
        return m_psl->SetArguments(NULL);
}

STDMETHODIMP CSDShellLink::get_Hotkey(int *piHK)
{

    return m_psl->GetHotkey((WORD*)piHK);
}

STDMETHODIMP CSDShellLink::put_Hotkey(int iHK)
{
    return m_psl->SetHotkey((WORD)iHK);
}

STDMETHODIMP CSDShellLink::get_ShowCommand(int *piShowCommand)
{
    return m_psl->GetShowCmd(piShowCommand);
}

STDMETHODIMP CSDShellLink::put_ShowCommand(int iShowCommand)
{
    return m_psl->SetShowCmd(iShowCommand);
}

STDMETHODIMP CSDShellLink::Resolve(int fFlags)
{
    return m_psl->Resolve(m_hwndFldr, (DWORD)fFlags);
}


STDMETHODIMP CSDShellLink::GetIconLocation(BSTR *pbs, int *piIcon)
{
    TCHAR szIconPath[MAX_PATH];

    HRESULT hres;

    *pbs = NULL;
    TraceMsg(DM_TRACE, "CSDShellLink::GetIconLocation called");
    if (SUCCEEDED(hres = m_psl->GetIconLocation(szIconPath, ARRAYSIZE(szIconPath), piIcon)))
        *pbs = AllocBStrFromString(szIconPath);
    return hres;
}

STDMETHODIMP CSDShellLink::SetIconLocation(BSTR bs, int iIcon)
{
    TCHAR szArgs[MAX_PATH];

    TraceMsg(DM_TRACE, "CSDShellLink::SetIconLocation called");

    if (bs)
    {
        SHUnicodeToTChar(bs, szArgs, ARRAYSIZE(szArgs));
        return m_psl->SetIconLocation(szArgs, iIcon);
    }
    else
        return m_psl->SetIconLocation(NULL, iIcon);
    return E_NOTIMPL;
}


STDMETHODIMP CSDShellLink::Save(VARIANT vWhere)
{
    HRESULT hres;
    IPersistFile *ppf;

    // BUGBUG:: Ignoring the vWhere for now...
    if (SUCCEEDED(hres = m_psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf)))
    {
#if 0   // Don't need to pass in path now until support for SaveAs...
        // Now get fullpath for idlist
        char szPath[MAX_PATH];
        OLECHAR wszPath[MAX_PATH];
        STRRET strret;

        // Get path, conversion here is not optimal.
        m_psfFldr->GetDisplayNameOf(m_pidl, SHGDN_FORPARSING, &strret);
        StrRetToStrN(szPath, ARRAYSIZE(szPath), &strret, m_pidl);
        MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, ARRAYSIZE(wszPath));
        hres = ppf->Save(wszPath, TRUE);
#else
        hres = ppf->Save(NULL, TRUE);
#endif
        ppf->Release();
    }

    return hres;
}

