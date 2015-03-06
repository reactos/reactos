/*
 *    IEnumFORMATETC, IDataObject
 *
 * selecting and droping objects within the shell and/or common dialogs
 *
 *    Copyright 1998, 1999    <juergen.schmied@metronet.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
    DWORD                        size;

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

    if(!pFmt)return S_FALSE;
    if(!rgelt) return E_INVALIDARG;
    if (pceltFethed)  *pceltFethed = 0;

    for(i = 0; posFmt < countFmt && celt > i; i++)
    {
      *rgelt++ = pFmt[posFmt++];
    }

    if (pceltFethed) *pceltFethed = i;

    return ((i == celt) ? S_OK : S_FALSE);
}

HRESULT WINAPI IEnumFORMATETCImpl::Skip(ULONG celt)
{
    TRACE("(%p)->(num=%u)\n", this, celt);

    if (posFmt + celt >= countFmt) return S_FALSE;
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
    HRESULT                                    hResult;

    TRACE("(%p)->(ppenum=%p)\n", this, ppenum);

    if (!ppenum) return E_INVALIDARG;
    hResult = IEnumFORMATETC_Constructor(countFmt, pFmt, ppenum);
    if (FAILED (hResult))
        return hResult;
    return (*ppenum)->Skip(posFmt);
}

HRESULT IEnumFORMATETC_Constructor(UINT cfmt, const FORMATETC afmt[], IEnumFORMATETC **ppFormat)
{
    return ShellObjectCreatorInit<IEnumFORMATETCImpl>(cfmt, afmt, IID_IEnumFORMATETC, ppFormat);
}


/***********************************************************************
*   IDataObject implementation
*/

/* number of supported formats */
#define MAX_FORMATS 5

class CIDLDataObj :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDataObject,
    public IAsyncOperation 
{
private:
    LPITEMIDLIST    pidl;
    LPITEMIDLIST *    apidl;
    UINT        cidl;
    DWORD        dropeffect;

    FORMATETC    pFormatEtc[MAX_FORMATS];
    UINT        cfShellIDList;
    UINT        cfFileNameA;
    UINT        cfFileNameW;
    UINT        cfPreferredDropEffect;
    BOOL        doasync;
public:
    CIDLDataObj();
    ~CIDLDataObj();
    HRESULT WINAPI Initialize(HWND hwndOwner, LPCITEMIDLIST pMyPidl, LPCITEMIDLIST * apidlx, UINT cidlx);

    ///////////
    virtual HRESULT WINAPI GetData(LPFORMATETC pformatetcIn, STGMEDIUM *pmedium);
    virtual HRESULT WINAPI GetDataHere(LPFORMATETC pformatetc, STGMEDIUM *pmedium);
    virtual HRESULT WINAPI QueryGetData(LPFORMATETC pformatetc);
    virtual HRESULT WINAPI GetCanonicalFormatEtc(LPFORMATETC pformatectIn, LPFORMATETC pformatetcOut);
    virtual HRESULT WINAPI SetData(LPFORMATETC pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
    virtual HRESULT WINAPI EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
    virtual HRESULT WINAPI DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
    virtual HRESULT WINAPI DUnadvise(DWORD dwConnection);
    virtual HRESULT WINAPI EnumDAdvise(IEnumSTATDATA **ppenumAdvise);
    virtual HRESULT WINAPI GetAsyncMode(BOOL *pfIsOpAsync);
    virtual HRESULT WINAPI InOperation(BOOL *pfInAsyncOp);
    virtual HRESULT WINAPI SetAsyncMode(BOOL fDoOpAsync);
    virtual HRESULT WINAPI StartOperation(IBindCtx *pbcReserved);
    virtual HRESULT WINAPI EndOperation(HRESULT hResult, IBindCtx *pbcReserved, DWORD dwEffects);

BEGIN_COM_MAP(CIDLDataObj)
    COM_INTERFACE_ENTRY_IID(IID_IDataObject, IDataObject)
    COM_INTERFACE_ENTRY_IID(IID_IAsyncOperation,  IAsyncOperation)
END_COM_MAP()
};

CIDLDataObj::CIDLDataObj()
{
    pidl = NULL;
    apidl = NULL;
    cidl = 0;
    dropeffect = 0;
    cfShellIDList = 0;
    cfFileNameA = 0;
    cfFileNameW = 0;
    cfPreferredDropEffect = 0;
    doasync = FALSE;
}

CIDLDataObj::~CIDLDataObj()
{
    TRACE(" destroying IDataObject(%p)\n",this);
    _ILFreeaPidl(apidl, cidl);
    ILFree(pidl);
}

HRESULT WINAPI CIDLDataObj::Initialize(HWND hwndOwner, LPCITEMIDLIST pMyPidl, LPCITEMIDLIST * apidlx, UINT cidlx)
{
    pidl = ILClone(pMyPidl);
    apidl = _ILCopyaPidl(apidlx, cidlx);
    if (pidl == NULL || apidl == NULL)
        return E_OUTOFMEMORY;
    cidl = cidlx;
    dropeffect = DROPEFFECT_COPY;

    cfShellIDList = RegisterClipboardFormatW(CFSTR_SHELLIDLIST);
    cfFileNameA = RegisterClipboardFormatA(CFSTR_FILENAMEA);
    cfFileNameW = RegisterClipboardFormatW(CFSTR_FILENAMEW);
    cfPreferredDropEffect = RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECTW);
    InitFormatEtc(pFormatEtc[0], cfShellIDList, TYMED_HGLOBAL);
    InitFormatEtc(pFormatEtc[1], CF_HDROP, TYMED_HGLOBAL);
    InitFormatEtc(pFormatEtc[2], cfFileNameA, TYMED_HGLOBAL);
    InitFormatEtc(pFormatEtc[3], cfFileNameW, TYMED_HGLOBAL);
    InitFormatEtc(pFormatEtc[4], cfPreferredDropEffect, TYMED_HGLOBAL);
    return S_OK;
}

static HGLOBAL RenderPREFEREDDROPEFFECT (DWORD dwFlags)
{
    DWORD * pdwFlag;
    HGLOBAL hGlobal;

    TRACE("(0x%08x)\n", dwFlags);

    hGlobal = GlobalAlloc(GHND|GMEM_SHARE, sizeof(DWORD));
    if(!hGlobal) return hGlobal;
        pdwFlag = (DWORD*)GlobalLock(hGlobal);
    *pdwFlag = dwFlags;
    GlobalUnlock(hGlobal);
    return hGlobal;
}

/**************************************************************************
* IDataObject_fnGetData
*/
HRESULT WINAPI CIDLDataObj::GetData(LPFORMATETC pformatetcIn, STGMEDIUM *pmedium)
{
    char    szTemp[256];

    szTemp[0] = 0;
    GetClipboardFormatNameA (pformatetcIn->cfFormat, szTemp, 256);
    TRACE("(%p)->(%p %p format=%s)\n", this, pformatetcIn, pmedium, szTemp);

    if (pformatetcIn->cfFormat == cfShellIDList)
    {
      if (cidl < 1) return(E_UNEXPECTED);
      pmedium->hGlobal = RenderSHELLIDLIST(pidl, apidl, cidl);
    }
    else if    (pformatetcIn->cfFormat == CF_HDROP)
    {
      if (cidl < 1) return(E_UNEXPECTED);
      pmedium->hGlobal = RenderHDROP(pidl, apidl, cidl);
    }
    else if    (pformatetcIn->cfFormat == cfFileNameA)
    {
      if (cidl < 1) return(E_UNEXPECTED);
      pmedium->hGlobal = RenderFILENAMEA(pidl, apidl, cidl);
    }
    else if    (pformatetcIn->cfFormat == cfFileNameW)
    {
      if (cidl < 1) return(E_UNEXPECTED);
      pmedium->hGlobal = RenderFILENAMEW(pidl, apidl, cidl);
    }
    else if    (pformatetcIn->cfFormat == cfPreferredDropEffect)
    {
      pmedium->hGlobal = RenderPREFEREDDROPEFFECT(dropeffect);
    }
    else
    {
      FIXME("-- expected clipformat not implemented\n");
      return (E_INVALIDARG);
    }
    if (pmedium->hGlobal)
    {
      pmedium->tymed = TYMED_HGLOBAL;
      pmedium->pUnkForRelease = NULL;
      return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT WINAPI CIDLDataObj::GetDataHere(LPFORMATETC pformatetc, STGMEDIUM *pmedium)
{
    FIXME("(%p)->()\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CIDLDataObj::QueryGetData(LPFORMATETC pformatetc)
{
    UINT i;

    TRACE("(%p)->(fmt=0x%08x tym=0x%08x)\n", this, pformatetc->cfFormat, pformatetc->tymed);

    if(!(DVASPECT_CONTENT & pformatetc->dwAspect))
      return DV_E_DVASPECT;

    /* check our formats table what we have */
    for (i=0; i<MAX_FORMATS; i++)
    {
      if ((pFormatEtc[i].cfFormat == pformatetc->cfFormat)
       && (pFormatEtc[i].tymed == pformatetc->tymed))
      {
        return S_OK;
      }
    }

    return DV_E_TYMED;
}

HRESULT WINAPI CIDLDataObj::GetCanonicalFormatEtc(LPFORMATETC pformatectIn, LPFORMATETC pformatetcOut)
{
    FIXME("(%p)->()\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CIDLDataObj::SetData(LPFORMATETC pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    if (pformatetc->cfFormat == cfPreferredDropEffect)
    {
      const DWORD *src = (const DWORD *)GlobalLock(pmedium->hGlobal);
      if (src != 0)
      {
        dropeffect = *src;
        GlobalUnlock(pmedium->hGlobal);
        return S_OK;
      }
      FIXME("Error setting data");
      return E_FAIL;
    }

    FIXME("(%p)->()\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CIDLDataObj::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
    TRACE("(%p)->()\n", this);
    *ppenumFormatEtc = NULL;

    /* only get data */
    if (DATADIR_GET == dwDirection)
    {
        return IEnumFORMATETC_Constructor(MAX_FORMATS, pFormatEtc, ppenumFormatEtc);
    }

    return E_NOTIMPL;
}

HRESULT WINAPI CIDLDataObj::DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    FIXME("(%p)->()\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CIDLDataObj::DUnadvise(DWORD dwConnection)
{
    FIXME("(%p)->()\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CIDLDataObj::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
    FIXME("(%p)->()\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CIDLDataObj::GetAsyncMode(BOOL *pfIsOpAsync)
{
    TRACE("(%p)->()\n", this);
    *pfIsOpAsync = doasync;
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
    doasync = fDoOpAsync;
    return S_OK;
}

HRESULT WINAPI CIDLDataObj::StartOperation(IBindCtx *pbcReserved)
{
    FIXME("(%p)->()\n", this);
    return E_NOTIMPL;
}
HRESULT WINAPI CIDLDataObj::EndOperation(HRESULT hResult, IBindCtx *pbcReserved, DWORD dwEffects)
{
    FIXME("(%p)->()\n", this);
    return E_NOTIMPL;
}



/**************************************************************************
*  IDataObject_Constructor
*/
HRESULT IDataObject_Constructor(HWND hwndOwner, LPCITEMIDLIST pMyPidl, LPCITEMIDLIST * apidl, UINT cidl, IDataObject **dataObject)
{
    return ShellObjectCreatorInit<CIDLDataObj>(hwndOwner, pMyPidl, apidl, cidl, IID_IDataObject, dataObject);
}

/*************************************************************************
 * SHCreateDataObject            [SHELL32.@]
 *
 */

HRESULT WINAPI SHCreateDataObject(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST* apidl, IDataObject *pdtInner, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IDataObject))
    {
        return CIDLData_CreateFromIDArray(pidlFolder, cidl, apidl, (IDataObject **)ppv);
    }
    return E_FAIL;
}
