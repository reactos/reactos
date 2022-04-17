/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IEnumFORMATETC, IDataObject implementation
 * COPYRIGHT:   Copyright 1998, 1999 <juergen.schmied@metronet.de>
 *              Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/***********************************************************************
*   IEnumFORMATETC implementation
*/

class IEnumFORMATETCImpl :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumFORMATETC
{
private:
    UINT                        posFmt;
    UINT                        countFmt;
    LPFORMATETC                    pFmt;
public:
    IEnumFORMATETCImpl();
    ~IEnumFORMATETCImpl();
    HRESULT WINAPI Initialize(UINT cfmt, const FORMATETC afmt[]);

    // *****************
    virtual HRESULT WINAPI Next(ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed);
    virtual HRESULT WINAPI Skip(ULONG celt);
    virtual HRESULT WINAPI Reset();
    virtual HRESULT WINAPI Clone(LPENUMFORMATETC* ppenum);

BEGIN_COM_MAP(IEnumFORMATETCImpl)
    COM_INTERFACE_ENTRY_IID(IID_IEnumFORMATETC, IEnumFORMATETC)
END_COM_MAP()
};

IEnumFORMATETCImpl::IEnumFORMATETCImpl()
{
    posFmt = 0;
    countFmt = 0;
    pFmt = NULL;
}

IEnumFORMATETCImpl::~IEnumFORMATETCImpl()
{
}

HRESULT WINAPI IEnumFORMATETCImpl::Initialize(UINT cfmt, const FORMATETC afmt[])
{
    DWORD size;

    size = cfmt * sizeof(FORMATETC);
    countFmt = cfmt;
    pFmt = (LPFORMATETC)SHAlloc(size);
    if (pFmt == NULL)
        return E_OUTOFMEMORY;

    memcpy(pFmt, afmt, size);
    return S_OK;
}

HRESULT WINAPI IEnumFORMATETCImpl::Next(ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed)
{
    UINT i;

    TRACE("(%p)->(%u,%p)\n", this, celt, rgelt);

    if (!pFmt)
        return S_FALSE;
    if (!rgelt)
        return E_INVALIDARG;
    if (pceltFethed)
        *pceltFethed = 0;

    for (i = 0; posFmt < countFmt && celt > i; i++)
    {
        *rgelt++ = pFmt[posFmt++];
    }

    if (pceltFethed)
        *pceltFethed = i;

    return ((i == celt) ? S_OK : S_FALSE);
}

HRESULT WINAPI IEnumFORMATETCImpl::Skip(ULONG celt)
{
    TRACE("(%p)->(num=%u)\n", this, celt);

    if (posFmt + celt >= countFmt)
        return S_FALSE;
    posFmt += celt;
    return S_OK;
}

HRESULT WINAPI IEnumFORMATETCImpl::Reset()
{
    TRACE("(%p)->()\n", this);

    posFmt = 0;
    return S_OK;
}

HRESULT WINAPI IEnumFORMATETCImpl::Clone(LPENUMFORMATETC* ppenum)
{
    HRESULT hResult;

    TRACE("(%p)->(ppenum=%p)\n", this, ppenum);

    if (!ppenum) return E_INVALIDARG;
    hResult = IEnumFORMATETC_Constructor(countFmt, pFmt, ppenum);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return (*ppenum)->Skip(posFmt);
}

HRESULT IEnumFORMATETC_Constructor(UINT cfmt, const FORMATETC afmt[], IEnumFORMATETC **ppFormat)
{
    return ShellObjectCreatorInit<IEnumFORMATETCImpl>(cfmt, afmt, IID_PPV_ARG(IEnumFORMATETC, ppFormat));
}


/***********************************************************************
*   IDataObject implementation
*   For now (2019-10-12) it's compatible with 2k3's data object
*   See shell32_apitest!CIDLData for changes between versions
*/

class CIDLDataObj :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDataObject,
    public IAsyncOperation
{
private:
    CSimpleArray<FORMATETC> m_Formats;
    CSimpleArray<STGMEDIUM> m_Storage;
    UINT m_cfShellIDList;
    BOOL m_doasync;
public:
    CIDLDataObj();
    ~CIDLDataObj();
    HRESULT WINAPI Initialize(HWND hwndOwner, PCIDLIST_ABSOLUTE pMyPidl, PCUIDLIST_RELATIVE_ARRAY apidlx, UINT cidlx, BOOL bAddAdditionalFormats);

    // *** IDataObject methods ***
    virtual HRESULT WINAPI GetData(LPFORMATETC pformatetcIn, STGMEDIUM *pmedium);
    virtual HRESULT WINAPI GetDataHere(LPFORMATETC pformatetc, STGMEDIUM *pmedium);
    virtual HRESULT WINAPI QueryGetData(LPFORMATETC pformatetc);
    virtual HRESULT WINAPI GetCanonicalFormatEtc(LPFORMATETC pformatectIn, LPFORMATETC pformatetcOut);
    virtual HRESULT WINAPI SetData(LPFORMATETC pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
    virtual HRESULT WINAPI EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
    virtual HRESULT WINAPI DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
    virtual HRESULT WINAPI DUnadvise(DWORD dwConnection);
    virtual HRESULT WINAPI EnumDAdvise(IEnumSTATDATA **ppenumAdvise);

    // *** IAsyncOperation methods ***
    virtual HRESULT WINAPI SetAsyncMode(BOOL fDoOpAsync);
    virtual HRESULT WINAPI GetAsyncMode(BOOL *pfIsOpAsync);
    virtual HRESULT WINAPI StartOperation(IBindCtx *pbcReserved);
    virtual HRESULT WINAPI InOperation(BOOL *pfInAsyncOp);
    virtual HRESULT WINAPI EndOperation(HRESULT hResult, IBindCtx *pbcReserved, DWORD dwEffects);

BEGIN_COM_MAP(CIDLDataObj)
    COM_INTERFACE_ENTRY_IID(IID_IDataObject, IDataObject)
    COM_INTERFACE_ENTRY_IID(IID_IAsyncOperation,  IAsyncOperation)
END_COM_MAP()
};

CIDLDataObj::CIDLDataObj()
{
    m_cfShellIDList = 0;
    m_doasync = FALSE;
}

CIDLDataObj::~CIDLDataObj()
{
    TRACE(" destroying IDataObject(%p)\n", this);

    for (int n = 0; n < m_Storage.GetSize(); ++n)
    {
        ReleaseStgMedium(&m_Storage[n]);
    }
    m_Formats.RemoveAll();
    m_Storage.RemoveAll();
}

HRESULT WINAPI CIDLDataObj::Initialize(HWND hwndOwner, PCIDLIST_ABSOLUTE pMyPidl, PCUIDLIST_RELATIVE_ARRAY apidlx, UINT cidlx, BOOL bAddAdditionalFormats)
{
    HGLOBAL hida = RenderSHELLIDLIST((LPITEMIDLIST)pMyPidl, (LPITEMIDLIST*)apidlx, cidlx);
    if (!hida)
    {
        ERR("Failed to render " CFSTR_SHELLIDLISTA "\n");
        return E_OUTOFMEMORY;
    }

    m_cfShellIDList = RegisterClipboardFormatW(CFSTR_SHELLIDLIST);

    FORMATETC Format = { (CLIPFORMAT)m_cfShellIDList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = hida;
    HRESULT hr = SetData(&Format, &medium, TRUE);
    if (!FAILED_UNEXPECTEDLY(hr) && bAddAdditionalFormats)
    {
        Format.cfFormat = CF_HDROP;
        medium.hGlobal = RenderHDROP((LPITEMIDLIST)pMyPidl, (LPITEMIDLIST*)apidlx, cidlx);
        hr = SetData(&Format, &medium, TRUE);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        Format.cfFormat = RegisterClipboardFormatA(CFSTR_FILENAMEA);
        medium.hGlobal = RenderFILENAMEA((LPITEMIDLIST)pMyPidl, (LPITEMIDLIST*)apidlx, cidlx);
        hr = SetData(&Format, &medium, TRUE);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        Format.cfFormat = RegisterClipboardFormatW(CFSTR_FILENAMEW);
        medium.hGlobal = RenderFILENAMEW((LPITEMIDLIST)pMyPidl, (LPITEMIDLIST*)apidlx, cidlx);
        hr = SetData(&Format, &medium, TRUE);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    return hr;
}


HRESULT WINAPI CIDLDataObj::GetData(LPFORMATETC pformatetcIn, STGMEDIUM *pmedium)
{
    if (TRACE_ON(shell))
    {
        char szTemp[256] = {0};
        GetClipboardFormatNameA (pformatetcIn->cfFormat, szTemp, 256);
        TRACE("(%p)->(%p %p format=%s)\n", this, pformatetcIn, pmedium, szTemp);
    }
    pmedium->hGlobal = NULL;
    pmedium->pUnkForRelease = NULL;
    for (int n = 0; n < m_Formats.GetSize(); ++n)
    {
        const FORMATETC& fmt = m_Formats[n];
        if (fmt.cfFormat == pformatetcIn->cfFormat &&
            fmt.dwAspect == pformatetcIn->dwAspect &&
            fmt.tymed == pformatetcIn->tymed)
        {
            if (pformatetcIn->tymed != TYMED_HGLOBAL)
            {
                UNIMPLEMENTED;
                return E_INVALIDARG;
            }
            else
            {
                *pmedium = m_Storage[n];
                return QueryInterface(IID_PPV_ARG(IUnknown, &pmedium->pUnkForRelease));
            }
        }
    }

    return E_INVALIDARG;
}

HRESULT WINAPI CIDLDataObj::GetDataHere(LPFORMATETC pformatetc, STGMEDIUM *pmedium)
{
    FIXME("(%p)->()\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CIDLDataObj::QueryGetData(LPFORMATETC pformatetc)
{
    TRACE("(%p)->(fmt=0x%08x tym=0x%08x)\n", this, pformatetc->cfFormat, pformatetc->tymed);

    for (int n = 0; n < m_Formats.GetSize(); ++n)
    {
        const FORMATETC& fmt = m_Formats[n];
        if (fmt.cfFormat == pformatetc->cfFormat &&
            fmt.dwAspect == pformatetc->dwAspect &&
            fmt.tymed == pformatetc->tymed)
        {
            return S_OK;
        }
    }

    return S_FALSE;
}

HRESULT WINAPI CIDLDataObj::GetCanonicalFormatEtc(LPFORMATETC pformatectIn, LPFORMATETC pformatetcOut)
{
    //FIXME("(%p)->()\n", this);
    return DATA_S_SAMEFORMATETC;
}

HRESULT WINAPI CIDLDataObj::SetData(LPFORMATETC pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    if (!fRelease)
        return E_INVALIDARG;

    for (int n = 0; n < m_Formats.GetSize(); ++n)
    {
        const FORMATETC& fmt = m_Formats[n];
        if (fmt.cfFormat == pformatetc->cfFormat &&
            fmt.dwAspect == pformatetc->dwAspect &&
            fmt.tymed == pformatetc->tymed)
        {
            ReleaseStgMedium(&m_Storage[n]);
            m_Storage[n] = *pmedium;
            return S_OK;
        }
    }

    m_Formats.Add(*pformatetc);
    m_Storage.Add(*pmedium);

    return S_OK;
}

HRESULT WINAPI CIDLDataObj::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
    TRACE("(%p)->()\n", this);
    *ppenumFormatEtc = NULL;

    /* only get data */
    if (DATADIR_GET == dwDirection)
    {
        return IEnumFORMATETC_Constructor(m_Formats.GetSize(), m_Formats.GetData(), ppenumFormatEtc);
    }

    return E_NOTIMPL;
}

HRESULT WINAPI CIDLDataObj::DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT WINAPI CIDLDataObj::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT WINAPI CIDLDataObj::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT WINAPI CIDLDataObj::GetAsyncMode(BOOL *pfIsOpAsync)
{
    TRACE("(%p)->()\n", this);
    *pfIsOpAsync = m_doasync;
    return S_OK;
}
HRESULT WINAPI CIDLDataObj::InOperation(BOOL *pfInAsyncOp)
{
    FIXME("(%p)->()\n", this);
    return E_NOTIMPL;
}
HRESULT WINAPI CIDLDataObj::SetAsyncMode(BOOL fDoOpAsync)
{
    TRACE("(%p)->()\n", this);
    m_doasync = fDoOpAsync;
    return S_OK;
}

HRESULT WINAPI CIDLDataObj::StartOperation(IBindCtx *pbcReserved)
{
    TRACE("(%p)->()\n", this);
    return E_NOTIMPL;
}
HRESULT WINAPI CIDLDataObj::EndOperation(HRESULT hResult, IBindCtx *pbcReserved, DWORD dwEffects)
{
    TRACE("(%p)->()\n", this);
    return E_NOTIMPL;
}



/**************************************************************************
 *  IDataObject_Constructor
 */
HRESULT IDataObject_Constructor(HWND hwndOwner, PCIDLIST_ABSOLUTE pMyPidl, PCUIDLIST_RELATIVE_ARRAY apidl, UINT cidl, BOOL bExtendedObject, IDataObject **dataObject)
{
    if (!dataObject)
        return E_INVALIDARG;
    return ShellObjectCreatorInit<CIDLDataObj>(hwndOwner, pMyPidl, apidl, cidl, bExtendedObject, IID_PPV_ARG(IDataObject, dataObject));
}

/*************************************************************************
 * SHCreateDataObject            [SHELL32.@]
 *
 */

HRESULT WINAPI SHCreateDataObject(PCIDLIST_ABSOLUTE pidlFolder, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, IDataObject *pdtInner, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IDataObject))
    {
        if (pdtInner)
            UNIMPLEMENTED;
        return IDataObject_Constructor(NULL, pidlFolder, apidl, cidl, TRUE, (IDataObject **)ppv);
    }
    return E_FAIL;
}

/*************************************************************************
 * SHCreateFileDataObject       [SHELL32.740]
 *
 */

HRESULT WINAPI SHCreateFileDataObject(PCIDLIST_ABSOLUTE pidlFolder, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, IDataObject* pDataInner, IDataObject** ppDataObj)
{
    if (pDataInner)
        UNIMPLEMENTED;
    return IDataObject_Constructor(NULL, pidlFolder, apidl, cidl, TRUE, ppDataObj);
}
