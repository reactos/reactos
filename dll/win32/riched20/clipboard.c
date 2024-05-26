/*
 * Richedit clipboard handling
 *
 * Copyright (C) 2006 Kevin Koltzau
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

#include "editor.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

static UINT cfRTF = 0;

typedef struct DataObjectImpl {
    IDataObject IDataObject_iface;
    LONG ref;

    FORMATETC *fmtetc;
    UINT fmtetc_cnt;

    HANDLE unicode;
    HANDLE rtf;
} DataObjectImpl;

typedef struct EnumFormatImpl {
    IEnumFORMATETC IEnumFORMATETC_iface;
    LONG ref;

    FORMATETC *fmtetc;
    UINT fmtetc_cnt;

    UINT cur;
} EnumFormatImpl;

static HRESULT EnumFormatImpl_Create(const FORMATETC *fmtetc, UINT size, LPENUMFORMATETC *lplpformatetc);

static inline DataObjectImpl *impl_from_IDataObject(IDataObject *iface)
{
    return CONTAINING_RECORD(iface, DataObjectImpl, IDataObject_iface);
}

static inline EnumFormatImpl *impl_from_IEnumFORMATETC(IEnumFORMATETC *iface)
{
    return CONTAINING_RECORD(iface, EnumFormatImpl, IEnumFORMATETC_iface);
}

static HRESULT WINAPI EnumFormatImpl_QueryInterface(IEnumFORMATETC *iface, REFIID riid, LPVOID *ppvObj)
{
    EnumFormatImpl *This = impl_from_IEnumFORMATETC(iface);
    TRACE("%p %s\n", This, debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IEnumFORMATETC)) {
        IEnumFORMATETC_AddRef(iface);
        *ppvObj = &This->IEnumFORMATETC_iface;
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumFormatImpl_AddRef(IEnumFORMATETC *iface)
{
    EnumFormatImpl *This = impl_from_IEnumFORMATETC(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI EnumFormatImpl_Release(IEnumFORMATETC *iface)
{
    EnumFormatImpl *This = impl_from_IEnumFORMATETC(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        GlobalFree(This->fmtetc);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI EnumFormatImpl_Next(IEnumFORMATETC *iface, ULONG celt,
                                          FORMATETC *rgelt, ULONG *pceltFetched)
{
    EnumFormatImpl *This = impl_from_IEnumFORMATETC(iface);
    ULONG count = 0;
    TRACE("(%p)->(%ld %p %p)\n", This, celt, rgelt, pceltFetched);

    if(!rgelt)
        return E_INVALIDARG;

    count = min(celt, This->fmtetc_cnt-This->cur);
    if(count > 0) {
        memcpy(rgelt, This->fmtetc+This->cur, count*sizeof(FORMATETC));
        This->cur += count;
    }
    if(pceltFetched)
        *pceltFetched = count;
    return count == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumFormatImpl_Skip(IEnumFORMATETC *iface, ULONG celt)
{
    EnumFormatImpl *This = impl_from_IEnumFORMATETC(iface);
    ULONG count = 0;
    TRACE("(%p)->(%ld)\n", This, celt);

    count = min(celt, This->fmtetc_cnt-This->cur);
    This->cur += count;
    return count == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumFormatImpl_Reset(IEnumFORMATETC *iface)
{
    EnumFormatImpl *This = impl_from_IEnumFORMATETC(iface);
    TRACE("(%p)\n", This);

    This->cur = 0;
    return S_OK;
}

static HRESULT WINAPI EnumFormatImpl_Clone(IEnumFORMATETC *iface, IEnumFORMATETC **ppenum)
{
    EnumFormatImpl *This = impl_from_IEnumFORMATETC(iface);
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, ppenum);

    if(!ppenum)
        return E_INVALIDARG;
    hr = EnumFormatImpl_Create(This->fmtetc, This->fmtetc_cnt, ppenum);
    if(SUCCEEDED(hr))
        hr = IEnumFORMATETC_Skip(*ppenum, This->cur);
    return hr;
}

static const IEnumFORMATETCVtbl VT_EnumFormatImpl = {
    EnumFormatImpl_QueryInterface,
    EnumFormatImpl_AddRef,
    EnumFormatImpl_Release,
    EnumFormatImpl_Next,
    EnumFormatImpl_Skip,
    EnumFormatImpl_Reset,
    EnumFormatImpl_Clone
};

static HRESULT EnumFormatImpl_Create(const FORMATETC *fmtetc, UINT fmtetc_cnt,
                                     IEnumFORMATETC **formatetc)
{
    EnumFormatImpl *ret;
    TRACE("\n");

    ret = malloc(sizeof(EnumFormatImpl));
    ret->IEnumFORMATETC_iface.lpVtbl = &VT_EnumFormatImpl;
    ret->ref = 1;
    ret->cur = 0;
    ret->fmtetc_cnt = fmtetc_cnt;
    ret->fmtetc = GlobalAlloc(GMEM_ZEROINIT, fmtetc_cnt*sizeof(FORMATETC));
    memcpy(ret->fmtetc, fmtetc, fmtetc_cnt*sizeof(FORMATETC));
    *formatetc = &ret->IEnumFORMATETC_iface;
    return S_OK;
}

static HRESULT WINAPI DataObjectImpl_QueryInterface(IDataObject *iface, REFIID riid, LPVOID *ppvObj)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDataObject)) {
        IDataObject_AddRef(iface);
        *ppvObj = &This->IDataObject_iface;
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI DataObjectImpl_AddRef(IDataObject* iface)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI DataObjectImpl_Release(IDataObject* iface)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) ref=%ld\n",This, ref);

    if(!ref) {
        if(This->unicode) GlobalFree(This->unicode);
        if(This->rtf) GlobalFree(This->rtf);
        if(This->fmtetc) GlobalFree(This->fmtetc);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI DataObjectImpl_GetData(IDataObject* iface, FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    TRACE("(%p)->(fmt=0x%08x tym=0x%08lx)\n", This, pformatetc->cfFormat, pformatetc->tymed);

    if(pformatetc->lindex != -1)
        return DV_E_LINDEX;

    if(!(pformatetc->tymed & TYMED_HGLOBAL))
        return DV_E_TYMED;

    if(This->unicode && pformatetc->cfFormat == CF_UNICODETEXT)
        pmedium->hGlobal = This->unicode;
    else if(This->rtf && pformatetc->cfFormat == cfRTF)
        pmedium->hGlobal = This->rtf;
    else
        return DV_E_FORMATETC;

    pmedium->tymed = TYMED_HGLOBAL;
    pmedium->pUnkForRelease = (LPUNKNOWN)iface;
    IUnknown_AddRef(pmedium->pUnkForRelease);
    return S_OK;
}

static HRESULT WINAPI DataObjectImpl_GetDataHere(IDataObject* iface, FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_QueryGetData(IDataObject* iface, FORMATETC *pformatetc)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    UINT i;
    BOOL foundFormat = FALSE;
    TRACE("(%p)->(fmt=0x%08x tym=0x%08lx)\n", This, pformatetc->cfFormat, pformatetc->tymed);

    if(pformatetc->lindex != -1)
        return DV_E_LINDEX;

    for(i=0; i<This->fmtetc_cnt; i++) {
        if(This->fmtetc[i].cfFormat == pformatetc->cfFormat) {
            foundFormat = TRUE;
            if(This->fmtetc[i].tymed == pformatetc->tymed)
                return S_OK;
        }
    }
    return foundFormat?DV_E_FORMATETC:DV_E_TYMED;
}

static HRESULT WINAPI DataObjectImpl_GetCanonicalFormatEtc(IDataObject* iface, FORMATETC *pformatetcIn,
                                                           FORMATETC *pformatetcOut)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    TRACE("(%p)->(%p,%p)\n", This, pformatetcIn, pformatetcOut);

    if(pformatetcOut) {
        *pformatetcOut = *pformatetcIn;
        pformatetcOut->ptd = NULL;
    }
    return DATA_S_SAMEFORMATETC;
}

static HRESULT WINAPI DataObjectImpl_SetData(IDataObject* iface, FORMATETC *pformatetc,
                                             STGMEDIUM *pmedium, BOOL fRelease)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_EnumFormatEtc(IDataObject* iface, DWORD dwDirection,
                                                   IEnumFORMATETC **ppenumFormatEtc)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    TRACE("(%p)->(%ld)\n", This, dwDirection);

    if(dwDirection != DATADIR_GET) {
        FIXME("Unsupported direction: %ld\n", dwDirection);
        /* WinXP riched20 also returns E_NOTIMPL in this case */
        *ppenumFormatEtc = NULL;
        return E_NOTIMPL;
    }
    return EnumFormatImpl_Create(This->fmtetc, This->fmtetc_cnt, ppenumFormatEtc);
}

static HRESULT WINAPI DataObjectImpl_DAdvise(IDataObject* iface, FORMATETC *pformatetc, DWORD advf,
                                             IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_DUnadvise(IDataObject* iface, DWORD dwConnection)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_EnumDAdvise(IDataObject* iface, IEnumSTATDATA **ppenumAdvise)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static const IDataObjectVtbl VT_DataObjectImpl =
{
    DataObjectImpl_QueryInterface,
    DataObjectImpl_AddRef,
    DataObjectImpl_Release,
    DataObjectImpl_GetData,
    DataObjectImpl_GetDataHere,
    DataObjectImpl_QueryGetData,
    DataObjectImpl_GetCanonicalFormatEtc,
    DataObjectImpl_SetData,
    DataObjectImpl_EnumFormatEtc,
    DataObjectImpl_DAdvise,
    DataObjectImpl_DUnadvise,
    DataObjectImpl_EnumDAdvise
};

static HGLOBAL get_unicode_text(ME_TextEditor *editor, const ME_Cursor *start, int nChars)
{
    int pars = 0;
    WCHAR *data;
    HANDLE ret;
    ME_Paragraph *para;
    int nEnd = ME_GetCursorOfs(start) + nChars;

    /* count paragraphs in range */
    para = start->para;
    while ((para = para_next( para )) && para->nCharOfs <= nEnd)
        pars++;

    ret = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * (nChars + pars + 1));
    data = GlobalLock(ret);
    ME_GetTextW(editor, data, nChars + pars, start, nChars, TRUE, FALSE);
    GlobalUnlock(ret);
    return ret;
}

typedef struct tagME_GlobalDestStruct
{
  HGLOBAL hData;
  int nLength;
} ME_GlobalDestStruct;

static DWORD CALLBACK ME_AppendToHGLOBAL(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG cb, LONG *pcb)
{
    ME_GlobalDestStruct *pData = (ME_GlobalDestStruct *)dwCookie;
    int nMaxSize;
    BYTE *pDest;

    nMaxSize = GlobalSize(pData->hData);
    if (pData->nLength+cb+1 >= cb) {
        /* round up to 2^17 */
        int nNewSize = (((nMaxSize+cb+1)|0x1FFFF)+1) & 0xFFFE0000;
        pData->hData = GlobalReAlloc(pData->hData, nNewSize, GMEM_MOVEABLE);
    }
    pDest = GlobalLock(pData->hData);
    memcpy(pDest + pData->nLength, lpBuff, cb);
    pData->nLength += cb;
    pDest[pData->nLength] = '\0';
    GlobalUnlock(pData->hData);
    *pcb = cb;

    return 0;
}

static HGLOBAL get_rtf_text(ME_TextEditor *editor, const ME_Cursor *start, int nChars)
{
    EDITSTREAM es;
    ME_GlobalDestStruct gds;

    gds.hData = GlobalAlloc(GMEM_MOVEABLE, 0);
    gds.nLength = 0;
    es.dwCookie = (DWORD_PTR)&gds;
    es.pfnCallback = ME_AppendToHGLOBAL;
    ME_StreamOutRange(editor, SF_RTF, start, nChars, &es);
    GlobalReAlloc(gds.hData, gds.nLength+1, GMEM_MOVEABLE);
    return gds.hData;
}

HRESULT ME_GetDataObject(ME_TextEditor *editor, const ME_Cursor *start, int nChars,
                         IDataObject **dataobj)
{
    DataObjectImpl *obj;
    TRACE("(%p,%d,%d)\n", editor, ME_GetCursorOfs(start), nChars);

    obj = malloc(sizeof(DataObjectImpl));
    if(cfRTF == 0)
        cfRTF = RegisterClipboardFormatA("Rich Text Format");

    obj->IDataObject_iface.lpVtbl = &VT_DataObjectImpl;
    obj->ref = 1;
    obj->unicode = get_unicode_text(editor, start, nChars);
    obj->rtf = NULL;

    obj->fmtetc_cnt = 1;
    if(editor->mode & TM_RICHTEXT)
        obj->fmtetc_cnt++;
    obj->fmtetc = GlobalAlloc(GMEM_ZEROINIT, obj->fmtetc_cnt*sizeof(FORMATETC));
    InitFormatEtc(obj->fmtetc[0], CF_UNICODETEXT, TYMED_HGLOBAL);
    if(editor->mode & TM_RICHTEXT) {
        obj->rtf = get_rtf_text(editor, start, nChars);
        InitFormatEtc(obj->fmtetc[1], cfRTF, TYMED_HGLOBAL);
    }

    *dataobj = &obj->IDataObject_iface;
    return S_OK;
}
