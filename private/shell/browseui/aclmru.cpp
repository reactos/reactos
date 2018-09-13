/* Copyright 1996-1997 Microsoft */


#include <priv.h>
#include "sccls.h"
#include "dbgmem.h"
#include "aclisf.h"
#include "shellurl.h"

#define AC_GENERAL          TF_GENERAL + TF_AUTOCOMPLETE

//
// CACLMRU -- An AutoComplete List COM object that
//                  enumerates the Type-in MRU.
//


class CACLMRU
                : public IEnumString
                , public IACList
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IEnumString ***
    virtual STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    virtual STDMETHODIMP Skip(ULONG celt) {return E_NOTIMPL;}
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Clone(IEnumString **ppenum) {return E_NOTIMPL;}

    // *** IACList ***
    virtual STDMETHODIMP Expand(LPCOLESTR pszExpand) {return E_NOTIMPL;}

private:
    // Constructor / Destructor (protected so we can't create on stack)
    CACLMRU(LPCTSTR pszMRURegKey, BOOL fUseStaertRun);
    ~CACLMRU(void);

    // Instance creator
    friend HRESULT CACLMRU_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
    friend HRESULT CACLMRU_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi, LPCTSTR pszMRU);

    // Private variables
    DWORD           m_cRef;      // COM reference count
    HKEY            m_hKey;      // HKey of MRU Location
    DWORD           m_nMRUIndex; // Current Index into MRU
    LPTSTR          m_pszMRURegKey; // RegKey Name of MRU Location

    BITBOOL         m_fInRunMRU : 1; // Are we in the RunMRU?
    DWORD           m_dwRunMRUIndex; // Index into the Run MRU.
    DWORD           m_dwRunMRUSize;
    HANDLE          m_hMRURun;
};




/* IUnknown methods */

HRESULT CACLMRU::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumString))
    {
        *ppvObj = SAFECAST(this, IEnumString*);
    }
    else if (IsEqualIID(riid, IID_IACList))
    {
        *ppvObj = SAFECAST(this, IACList*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG CACLMRU::AddRef(void)
{
    m_cRef++;
    return m_cRef;
}

ULONG CACLMRU::Release(void)
{
    ASSERT(m_cRef > 0);

    m_cRef--;

    if (m_cRef > 0)
    {
        return m_cRef;
    }

    delete this;
    return 0;
}

/* IEnumString methods */

HRESULT CACLMRU::Reset(void)
{
    HRESULT hr = S_OK;
    TraceMsg(AC_GENERAL, "CACLMRU::Reset()");
    m_nMRUIndex = 0;
    m_fInRunMRU = FALSE;
    m_dwRunMRUIndex = 0;

    // If we haven't yet opened the key, open it.
    if (!m_hKey && (ERROR_SUCCESS != RegCreateKey(HKEY_CURRENT_USER, m_pszMRURegKey, &m_hKey)))
        hr = E_FAIL;

    return hr;
}


HRESULT CACLMRU::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    TCHAR szMRUEntry[MAX_URL_STRING+1];
    LPWSTR pwzMRUEntry = NULL;

    *pceltFetched = 0;
    if (!celt)
        return S_OK;

    if (!rgelt)
        return S_FALSE;

    if (!m_fInRunMRU)
    {
        hr = GetMRUEntry(m_hKey, m_nMRUIndex++, szMRUEntry, SIZECHARS(szMRUEntry), NULL);
        if (S_OK != hr)
        {
            if (m_hMRURun)
                m_fInRunMRU = TRUE; // Switch to using the Run MRU List now.
            else
                hr = S_FALSE; // This will indicate that no more items are in the list.
        }
    }

    if (m_fInRunMRU)
    {
        if (m_dwRunMRUIndex >= m_dwRunMRUSize)
            hr = S_FALSE;  // No more.
        else
        {
            if (EVAL(m_hMRURun) && EnumMRUList(m_hMRURun, m_dwRunMRUIndex++, szMRUEntry, ARRAYSIZE(szMRUEntry)) > 0)
            {
                // old MRU format has a slash at the end with the show cmd
                LPTSTR pszField = StrRChr(szMRUEntry, NULL, TEXT('\\'));
                if (pszField)
                    pszField[0] = TEXT('\0');
                hr = S_OK;
            }
            else
                hr = S_FALSE;
        }
    }

    if (S_OK == hr)
    {
        DWORD cchSize = lstrlen(szMRUEntry)+1;
        //
        // Allocate a return buffer (caller will free it).
        //
        pwzMRUEntry = (LPOLESTR)CoTaskMemAlloc(cchSize * SIZEOF(WCHAR));
        if (pwzMRUEntry)
        {
            //
            // Convert the display name into an OLESTR.
            //
#ifdef UNICODE
            StrCpyN(pwzMRUEntry, szMRUEntry, cchSize);
#else   // ANSI
            MultiByteToWideChar(CP_ACP, 0, szMRUEntry, -1, pwzMRUEntry, cchSize);
#endif  // ANSI
            rgelt[0] = pwzMRUEntry;
            *pceltFetched = 1;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}




/* Constructor / Destructor / CreateInstance */

CACLMRU::CACLMRU(LPCTSTR pszMRURegKey, BOOL fUseStaertRun)
{
    DllAddRef();
    // Require object to be in heap and Zero-Inited
    ASSERT(!m_hKey);
    ASSERT(!m_nMRUIndex);
    ASSERT(!m_fInRunMRU);
    ASSERT(!m_dwRunMRUIndex);
    ASSERT(!m_hMRURun);

    if (fUseStaertRun)
    {
        MRUINFO mi =  {
            SIZEOF(MRUINFO),
            26,
            MRU_CACHEWRITE,
            HKEY_CURRENT_USER,
            SZ_REGKEY_TYPEDCMDMRU,
            NULL        // NOTE: use default string compare
                        // since this is a GLOBAL MRU
        };

        m_hMRURun = CreateMRUList(&mi);
        if (EVAL(m_hMRURun))
            m_dwRunMRUSize = EnumMRUList(m_hMRURun, -1, NULL, 0);
    }

    // Init Member Variables
    Str_SetPtr(&m_pszMRURegKey, pszMRURegKey);

    m_cRef = 1;
}


CACLMRU::~CACLMRU()
{
    if (m_hKey)
        RegCloseKey(m_hKey);

    if (m_hMRURun)
        FreeMRUList(m_hMRURun);

    Str_SetPtr(&m_pszMRURegKey, NULL);
    DllRelease();
}

/****************************************************\
    FUNCTION: CACLMRU_CreateInstance

    DESCRIPTION:
        This function create an instance of the AutoComplete
    List "MRU".  The caller didn't specify which MRU
    list to use, so we default to the TYPE-IN CMD
    MRU, which is used in the Start->Run dialog and
    in AddressBars that are floating or in the Taskbar.
\****************************************************/
HRESULT CACLMRU_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    return CACLMRU_CreateInstance(punkOuter, ppunk, poi, SZ_REGKEY_TYPEDCMDMRU);
}

/****************************************************\
    FUNCTION: CACLMRU_CreateInstance

    DESCRIPTION:
        This function create an instance of the AutoComplete
    List "MRU".  This will point to either the MRU for
    a browser or for a non-browser (Start->Run or
    the AddressBar in the Taskbar or floating) depending
    on the pszMRU parameter.
\****************************************************/
HRESULT CACLMRU_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi, LPCTSTR pszMRU)
{
    *ppunk = NULL;
    BOOL fUseRunDlgMRU = (StrCmpI(pszMRU, SZ_REGKEY_TYPEDCMDMRU) ? FALSE : TRUE);

    CACLMRU *paclSF = new CACLMRU(pszMRU, fUseRunDlgMRU);
    if (paclSF)
    {
        *ppunk = SAFECAST(paclSF, IEnumString *);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}
