#include "shellprv.h"
#include "infotip.h"
#include "ids.h"
#include "prop.h"

#include <mluisupp.h>

DWORD _AppendTipText(LPTSTR pszBuf, int cchBuf, LPCTSTR pszCRLF, UINT idTitle, LPCTSTR pszValue)
{
    TCHAR szFmt[64], szTitle[128];

    if (idTitle && LoadString(g_hinst, idTitle, szTitle, SIZECHARS(szTitle)))
        LoadString(g_hinst, IDS_EXCOL_TEMPLATE, szFmt, SIZECHARS(szFmt));
    else
    {
        lstrcpy(szFmt, TEXT("%s%s%s"));
        szTitle[0] = 0;
    }

    return wnsprintf(pszBuf, cchBuf, szFmt, pszCRLF, szTitle, pszValue);
}

HRESULT _NextProp(LPCTSTR *ppszIn, LPTSTR pszProp, UINT cchProp)
{
    HRESULT hr;

    *pszProp = 0;

    if (*ppszIn)
    {
        LPTSTR pszSemi = StrChr(*ppszIn, TEXT(';'));
        if (pszSemi)
        {
            if (pszSemi > *ppszIn) // make sure well formed (no dbl slashes)
            {
                StrCpyN(pszProp, *ppszIn, (int)(pszSemi - *ppszIn) + 1);

                //  make sure that there is another segment to return
                if (!*(++pszSemi))
                    pszSemi = NULL;
                hr = S_OK;       
            }
            else
            {
                pszSemi = NULL;
                hr = E_INVALIDARG;    // bad input
            }
        }
        else
        {
            StrCpyN(pszProp, *ppszIn, cchProp);
            hr = S_OK;       
        }
        *ppszIn = pszSemi;
    }
    else
        hr = S_FALSE;     // done with loop

    return hr;
}


// generic info tip object

class CInfoTip : public IQueryInfo
{
public:
    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IQueryInfo methods.
    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR** ppwszTip);
    STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);

    CInfoTip(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPCWSTR pszProp);

private:
    LONG _cRef;
    HRESULT _GetInfoTipFromItem(WCHAR **ppszText);

    IShellFolder2 *_psf;
    LPITEMIDLIST _pidl;
    TCHAR _szText[INFOTIPSIZE];
};

#define PROP_PREFIX         TEXT("prop:")
#define PROP_PREFIX_LEN     (ARRAYSIZE(PROP_PREFIX) - 1)

CInfoTip::CInfoTip(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPCWSTR pszText) : _cRef(1)
{
    if (IS_INTRESOURCE(pszText))
        LoadString(HINST_THISDLL, LOWORD((UINT_PTR)pszText), _szText, ARRAYSIZE(_szText));
    else
        SHUnicodeToTChar(pszText, _szText, ARRAYSIZE(_szText));

    if (psf && pidl && (StrCmpNI(_szText, PROP_PREFIX, PROP_PREFIX_LEN) == 0))
    {
        // list of properties, we need the psf and pidl for this
        psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &_psf));
        _pidl = ILClone(pidl);
    }
}


HRESULT CInfoTip::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CInfoTip, IQueryInfo),           // IID_IQueryInfo
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CInfoTip::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CInfoTip::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    if (_psf)
        _psf->Release();

    if (_pidl)
        ILFree(_pidl);

    delete this;
    return 0;
}

HRESULT CInfoTip::_GetInfoTipFromItem(WCHAR **ppszText)
{
    TCHAR szTip[INFOTIPSIZE];

    szTip[0] = 0;

    LPTSTR psz = szTip;
    LPCTSTR pszCRLF = TEXT("");
    UINT cch, cchMac = SIZECHARS(szTip);

    TCHAR szName[128];

    ASSERT(StrCmpNI(_szText, PROP_PREFIX, PROP_PREFIX_LEN) == 0);

    LPCTSTR pszStr = _szText + PROP_PREFIX_LEN;

    while (S_OK == _NextProp(&pszStr, szName, ARRAYSIZE(szName)))
    {
        SHCOLUMNID scid;
        UINT idProp;
        if (ParseSCIDString(szName, &scid, &idProp))
        {
            VARIANT v;
            VariantInit(&v);

            if (S_OK == _psf->GetDetailsEx(_pidl, &scid, &v))
            {
                TCHAR szValue[128];

                // special case size and free space becase these guys return int's
                // and need to be specially formatted
                if (IsEqualSCID(scid, SCID_FREESPACE) ||  
                    IsEqualSCID(scid, SCID_SIZE))
                {
                    ASSERT(v.vt == VT_UI8);
                    StrFormatByteSize64(v.ullVal, szValue, ARRAYSIZE(szValue));
                }
                else
                {
                    VariantToStr(&v, szValue, ARRAYSIZE(szValue));
                }

                if ((NULL == pszStr) && IsEqualSCID(scid, SCID_Comment))
                    idProp = 0;     // only one comment property, don't use the label 

                if (szValue[0])
                {
                    cch = _AppendTipText(psz, cchMac, pszCRLF, idProp, szValue);
                    psz += cch;
                    cchMac -= cch;
                    pszCRLF = TEXT("\r\n");
                }

                VariantClear(&v);
            }
        }
    }
    return SHStrDup(szTip, ppszText);
}


HRESULT CInfoTip::GetInfoTip(DWORD dwFlags, WCHAR** ppszText)
{
    HRESULT hr;
    if (_psf && _pidl)
        hr = _GetInfoTipFromItem(ppszText);
    else if (_szText[0])
        hr = SHStrDup(_szText, ppszText);
    else
        hr = E_FAIL;
    return hr;
}

HRESULT CInfoTip::GetInfoFlags(DWORD *pdwFlags)
{
    *pdwFlags = 0;
    return E_NOTIMPL;
}

// in:
//      pszText - description of info tip. either
//          1) a semi separated list of property names, "Author;Size" or "{fmtid},pid;{fmtid},pid"
//          2) if no semis the tip to create
//          MAKEINTRESOURCE(id) of a resource ID

STDAPI CreateInfoTipFromItem(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPCWSTR pszText, REFIID riid, void **ppv)
{
    HRESULT hr;
    CInfoTip* pit = new CInfoTip(psf, pidl, pszText);
    if (pit)
    {
        hr = pit->QueryInterface(riid, ppv);
        pit->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *ppv = NULL;
    }
    return hr;
}

STDAPI CreateInfoTipFromText(LPCTSTR pszText, REFIID riid, void **ppv)
{
    if (IS_INTRESOURCE(pszText))
        return CreateInfoTipFromItem(NULL, NULL, (LPCWSTR)pszText, riid, ppv);
    else
    {
        WCHAR szBuf[INFOTIPSIZE];
        SHTCharToUnicode(pszText, szBuf, ARRAYSIZE(szBuf));
        return CreateInfoTipFromItem(NULL, NULL, szBuf, riid, ppv);
    }
}
