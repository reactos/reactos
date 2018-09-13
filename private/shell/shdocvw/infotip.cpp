#include "priv.h"
#include "infotip.h"
#include "resource.h"

#include <mluisupp.h>

HRESULT ReadProp(IPropertyStorage *ppropstg, PROPID propid, PROPVARIANT *ppropvar)
{
    PROPSPEC prspec = { PRSPEC_PROPID, propid };

    return ppropstg->ReadMultiple(1, &prspec, ppropvar);
}

STDAPI GetStringProp(IPropertyStorage *ppropstg, PROPID propid, LPTSTR pszBuf, DWORD cchBuf)
{
    PROPVARIANT propvar;

    *pszBuf = 0;

    if (S_OK == ReadProp(ppropstg, propid, &propvar))
    {
        if (VT_LPWSTR == propvar.vt)
        {
            SHUnicodeToTChar(propvar.pwszVal, pszBuf, cchBuf);
        }
        else if (VT_LPSTR == propvar.vt)
        {
            SHAnsiToTChar(propvar.pszVal, pszBuf, cchBuf);
        }
        PropVariantClear(&propvar);
    }

    return *pszBuf ? S_OK : S_FALSE;
}

STDAPI GetFileTimeProp(IPropertyStorage *ppropstg, PROPID propid, LPTSTR pszBuf, DWORD cchBuf)
{
    PROPVARIANT propvar;

    *pszBuf = 0;

    if (S_OK == ReadProp(ppropstg, propid, &propvar))
    {
        if (VT_FILETIME == propvar.vt)
        {
            SHFormatDateTime(&propvar.filetime, NULL, pszBuf, cchBuf);
        }
        PropVariantClear(&propvar);
    }

    return *pszBuf ? S_OK : S_FALSE;
}


DWORD AppendTipText(LPTSTR pszBuf, int cchBuf, UINT ids, ...)
{
    DWORD dwRet;
    TCHAR szFmt[64];
    va_list ArgList;

    if (ids == 0 || 0 == MLLoadString(ids, szFmt, SIZECHARS(szFmt)))
        StrCpyN(szFmt, TEXT("%s%s"), ARRAYSIZE(szFmt));

    va_start(ArgList, ids);
    dwRet = wvnsprintf(pszBuf, cchBuf, szFmt, ArgList);
    va_end(ArgList);

    return dwRet;
}

STDAPI GetInfoTipFromStorage(IPropertySetStorage *ppropsetstg, const ITEM_PROP *pip, WCHAR **ppszTip)
{
    TCHAR szTip[2048];
    LPTSTR psz = szTip;
    LPCTSTR pszCRLF = TEXT("");
    UINT cch, cchMac = SIZECHARS(szTip);
    const GUID *pfmtIdLast = NULL;
    IPropertyStorage *ppropstg = NULL;
    HRESULT hres = E_FAIL;

    *ppszTip = NULL;

    for (; pip->pfmtid; pip++)
    {
        // cache the last FMTID and reuse it if the next FMTID is the same

        if (!ppropstg || !IsEqualGUID(*pfmtIdLast, *pip->pfmtid))
        {
            if (ppropstg)
            {
                ppropstg->Release();
                ppropstg = NULL;
            }

            pfmtIdLast = pip->pfmtid;
            ppropsetstg->Open(*pip->pfmtid, STGM_READ | STGM_SHARE_EXCLUSIVE, &ppropstg);
        }

        if (ppropstg)
        {
            TCHAR szT[256];

            hres = pip->pfnRead(ppropstg, pip->idProp, szT, SIZECHARS(szT));
            if (S_OK == hres) 
            {
                cch = AppendTipText(psz, cchMac, pip->idFmtString, pszCRLF, szT);
                psz += cch;
                cchMac -= cch;
                pszCRLF = TEXT("\r\n");
            }
            else if (hres != S_FALSE)
            {
                break;  // error, exit for loop
            }
        }
    }

    if (ppropstg)
        ppropstg->Release();

    hres = S_FALSE;     // assume no tooltip

    if (psz != szTip)
    {
        hres = SHStrDup(szTip, ppszTip);
    }

    return hres;
}

class CDocFileInfoTip : public IPersistFile, public IQueryInfo
{
public:
    CDocFileInfoTip(void);

    // IUnknown methods
    STDMETHODIMP  QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IPersist methods

    STDMETHODIMP GetClassID(CLSID *pclsid);

    // IPersistFile methods

    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Save(LPCOLESTR pcwszFileName, BOOL bRemember);
    STDMETHODIMP SaveCompleted(LPCOLESTR pcwszFileName);
    STDMETHODIMP Load(LPCOLESTR pcwszFileName, DWORD dwMode);
    STDMETHODIMP GetCurFile(LPOLESTR *ppwszFileName);

    // IQueryInfo methods

    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip);
    STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);

private:

    LONG    m_cRef;
    WCHAR   m_szFile[MAX_PATH]; // Name of file we are working on

    ~CDocFileInfoTip(void);    // Prevent this class from being allocated on the stack or it will fault.
};


CDocFileInfoTip::CDocFileInfoTip(void) : m_cRef(1)
{
    DllAddRef();
}

CDocFileInfoTip::~CDocFileInfoTip(void)
{
    DllRelease();
}

STDMETHODIMP CDocFileInfoTip::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDocFileInfoTip, IQueryInfo),                     // IID_IQueryInfo
        QITABENT(CDocFileInfoTip, IPersistFile),                   // IID_IPersistFile
        QITABENTMULTI(CDocFileInfoTip, IPersist, IPersistFile),    // IID_IPersist
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CDocFileInfoTip::AddRef()
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CDocFileInfoTip::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

// IPersist methods

STDMETHODIMP CDocFileInfoTip::GetClassID(CLSID *pclsid)
{
    *pclsid = CLSID_DocFileInfoTip;
    return S_OK;
}

// IPersistFile methods

STDMETHODIMP CDocFileInfoTip::IsDirty(void)
{
    return S_FALSE;
}

STDMETHODIMP CDocFileInfoTip::Save(LPCOLESTR pwszFile, BOOL bRemember)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDocFileInfoTip::SaveCompleted(LPCOLESTR pwszFile)
{
    return S_OK;
}

STDMETHODIMP CDocFileInfoTip::Load(const WCHAR *pwszFile, DWORD dwMode)
{
    StrCpyNW(m_szFile, pwszFile, ARRAYSIZE(m_szFile));
    return S_OK;
}

STDMETHODIMP CDocFileInfoTip::GetCurFile(WCHAR **ppwszFile)
{
    return E_NOTIMPL;
}

// IQueryInfo methods

const ITEM_PROP c_rgDocProps[] = {
    { &FMTID_SummaryInformation, PIDSI_AUTHOR,       GetStringProp,      IDS_AUTHOR },
    { &FMTID_SummaryInformation, PIDSI_TITLE,        GetStringProp,      IDS_DOCTITLE },
    { &FMTID_SummaryInformation, PIDSI_SUBJECT,      GetStringProp,      IDS_SUBJECT },
    { &FMTID_SummaryInformation, PIDSI_COMMENTS,     GetStringProp,      IDS_COMMENTS },
    { NULL, 0, 0, 0 },
};


STDMETHODIMP CDocFileInfoTip::GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip)
{
    *ppwszTip = NULL;
    IStorage *pstg;
    HRESULT hres = StgOpenStorage(m_szFile, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, NULL, 0, &pstg);
    if (SUCCEEDED(hres))
    {
        IPropertySetStorage *pprop;
        hres = pstg->QueryInterface(IID_IPropertySetStorage, (void **)&pprop);
        if (SUCCEEDED(hres))
        {
            hres = GetInfoTipFromStorage(pprop, c_rgDocProps, ppwszTip);
            pprop->Release();
        }
        pstg->Release();
    }
    return hres;
}

STDMETHODIMP CDocFileInfoTip::GetInfoFlags(DWORD *pdwFlags)
{
    *pdwFlags = 0;
    return S_OK;
}

STDAPI CDocFileInfoTip_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hres = E_OUTOFMEMORY;
    CDocFileInfoTip *pis = new CDocFileInfoTip();
    if (pis)
    {
        *ppunk = SAFECAST(pis, IQueryInfo *);
        hres = S_OK;
    }
    return hres;
}
