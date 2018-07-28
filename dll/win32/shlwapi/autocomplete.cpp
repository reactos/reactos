// PROJECT:   ReactOS Shell
// LICENSE:   GPL - See COPYING in the top level directory
// PURPOSE:   helper function for SHAutoComplete
// COPYRIGHT: Copyright 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>

#include <shlwapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlsimpcoll.h>

///////////////////////////////////////////////////////////////////////////////
// CSHEnumString --- IEnumString for SHAutoComplete

class CSHEnumString : public IEnumString
{
public:
    CSHEnumString();
    virtual ~CSHEnumString();

    bool AddString(const WCHAR *str);
    void DeleteString(INT index);
    void ResetContent();

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID iid, LPVOID* ppInterface);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumString
    STDMETHODIMP         Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    STDMETHODIMP         Skip(ULONG celt);
    STDMETHODIMP         Reset(void);
    STDMETHODIMP         Clone(IEnumString **ppenum);

    // add candidates
    void AddDirs();
    void AddFilesAndDirs();
    void AddURLHist();
    void AddURLMRU();

protected:
    ULONG                   m_cRef;
    INT                     m_istr;
    ATL::CSimpleArray<BSTR> m_strs;
};

///////////////////////////////////////////////////////////////////////////////

CSHEnumString::CSHEnumString()
{
    m_cRef = 1;
    m_istr = 0;
}

CSHEnumString::~CSHEnumString()
{
    ResetContent();
}

bool CSHEnumString::AddString(const WCHAR* str)
{
    if (BSTR bstr = ::SysAllocString(str))
    {
        m_strs.Add(bstr);
        return true;
    }
    return false;
}

void CSHEnumString::ResetContent()
{
    for (INT i = 0; i < m_strs.GetSize(); ++i)
    {
        ::SysFreeString(m_strs[i]);
    }
    m_strs.RemoveAll();
}

void CSHEnumString::DeleteString(INT index)
{
    ::SysFreeString(m_strs[index]);
    m_strs.RemoveAt(index);
}

STDMETHODIMP CSHEnumString::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IEnumString == riid)
    {
        *ppv = (LPVOID)this;
    }

    if (*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSHEnumString::AddRef(void)
{
    ++m_cRef;

    return m_cRef;
}

STDMETHODIMP_(ULONG) CSHEnumString::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CSHEnumString::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    if (!rgelt || !pceltFetched)
        return E_POINTER;

    *pceltFetched = 0;
    *rgelt = NULL;

    if (m_istr >= m_strs.GetSize())
        return S_FALSE;

    size_t ielt = 0;
    for (; ielt < celt && m_istr < m_strs.GetSize(); ++ielt, ++m_istr)
    {
        size_t cch = (wcslen(m_strs[m_istr]) + 1);

        rgelt[ielt] = (LPWSTR)CoTaskMemAlloc(cch * sizeof(WCHAR));
        if (rgelt[ielt])
        {
            wcscpy(rgelt[ielt], m_strs[m_istr]);
        }
    }

    *pceltFetched = ielt;

    if (ielt == celt)
        return S_OK;

    return S_FALSE;
}

STDMETHODIMP CSHEnumString::Skip(ULONG cSkip)
{
    if (m_istr + cSkip >= (ULONG)m_strs.GetSize() || 0 == m_strs.GetSize())
    {
        return S_FALSE;
    }
    m_istr += cSkip;
    return S_OK;
}

STDMETHODIMP CSHEnumString::Reset(void)
{
    m_istr = 0;
    return S_OK;
}

STDMETHODIMP CSHEnumString::Clone(LPENUMSTRING *ppEnum)
{
    *ppEnum = NULL;

    CSHEnumString *pNew = new CSHEnumString();
    if (!pNew)
        return E_OUTOFMEMORY;

    for (INT i = 0; i < m_strs.GetSize(); ++i)
    {
        pNew->AddString(m_strs[i]);
    }

    pNew->AddRef();
    pNew->m_istr = m_istr;

    *ppEnum = pNew;
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CreateShlwapiEnumString

typedef struct SHLWAPI_ENUMSTRING
{
    char dummy;
} SHLWAPI_ENUMSTRING;

extern "C"
SHLWAPI_ENUMSTRING *CreateShlwapiEnumString(DWORD dwFlags)
{
    CSHEnumString *pEnumString = new CSHEnumString();

    if (dwFlags & (SHACF_FILESYS_ONLY | SHACF_FILESYSTEM))
    {
        if (dwFlags & SHACF_FILESYS_DIRS)
        {
            pEnumString->AddDirs();
        }
        else
        {
            pEnumString->AddFilesAndDirs();
        }
    }

    if (!(dwFlags & (SHACF_FILESYS_ONLY)))
    {
        if (dwFlags & SHACF_URLHISTORY)
        {
            pEnumString->AddURLHist();
        }
        if (dwFlags & SHACF_URLMRU)
        {
            pEnumString->AddURLMRU();
        }
    }

    return (SHLWAPI_ENUMSTRING *)pEnumString;
}

///////////////////////////////////////////////////////////////////////////////
// add candidates

void CSHEnumString::AddDirs()
{
    // TODO:
}

void CSHEnumString::AddFilesAndDirs()
{
    // TODO:
}

void CSHEnumString::AddURLHist()
{
    // TODO:
}

void CSHEnumString::AddURLMRU()
{
    // TODO:
}
