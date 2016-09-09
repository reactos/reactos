/*
 * Ole 2 Create functions implementation
 *
 * Copyright (C) 1999-2000 Abey George
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

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/******************************************************************************
 *		OleQueryCreateFromData [OLE32.@]
 *
 * Checks whether an object can become an embedded object.
 * the clipboard or OLE drag and drop.
 * Returns  : S_OK - Format that supports Embedded object creation are present.
 *            OLE_E_STATIC - Format that supports static object creation are present.
 *            S_FALSE - No acceptable format is available.
 */

HRESULT WINAPI OleQueryCreateFromData(IDataObject *data)
{
    IEnumFORMATETC *enum_fmt;
    FORMATETC fmt;
    BOOL found_static = FALSE;
    HRESULT hr;

    hr = IDataObject_EnumFormatEtc(data, DATADIR_GET, &enum_fmt);

    if(FAILED(hr)) return hr;

    do
    {
        hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
        if(hr == S_OK)
        {
            if(fmt.cfFormat == embedded_object_clipboard_format ||
               fmt.cfFormat == embed_source_clipboard_format ||
               fmt.cfFormat == filename_clipboard_format)
            {
                IEnumFORMATETC_Release(enum_fmt);
                return S_OK;
            }

            if(fmt.cfFormat == CF_METAFILEPICT ||
               fmt.cfFormat == CF_BITMAP ||
               fmt.cfFormat == CF_DIB)
                found_static = TRUE;
        }
    } while (hr == S_OK);

    IEnumFORMATETC_Release(enum_fmt);

    return found_static ? OLE_S_STATIC : S_FALSE;
}

static inline void init_fmtetc(FORMATETC *fmt, CLIPFORMAT cf, TYMED tymed)
{
    fmt->cfFormat = cf;
    fmt->ptd = NULL;
    fmt->dwAspect = DVASPECT_CONTENT;
    fmt->lindex = -1;
    fmt->tymed = tymed;
}

/***************************************************************************
 *         get_storage
 *
 * Retrieve an object's storage from a variety of sources.
 *
 * FIXME: CF_FILENAME.
 */
static HRESULT get_storage(IDataObject *data, IStorage *stg, UINT *src_cf, BOOL other_fmts)
{
    static const UINT fmt_id[] = { CF_METAFILEPICT, CF_BITMAP, CF_DIB };
    UINT i;
    HRESULT hr;
    FORMATETC fmt;
    STGMEDIUM med;
    IPersistStorage *persist;
    CLSID clsid;

    if (src_cf) *src_cf = 0;

    /* CF_EMBEDEDOBJECT */
    init_fmtetc(&fmt, embedded_object_clipboard_format, TYMED_ISTORAGE);
    med.tymed = TYMED_ISTORAGE;
    med.u.pstg = stg;
    med.pUnkForRelease = NULL;
    hr = IDataObject_GetDataHere(data, &fmt, &med);
    if(SUCCEEDED(hr))
    {
        if (src_cf) *src_cf = embedded_object_clipboard_format;
        return hr;
    }

    /* CF_EMBEDSOURCE */
    init_fmtetc(&fmt, embed_source_clipboard_format, TYMED_ISTORAGE);
    med.tymed = TYMED_ISTORAGE;
    med.u.pstg = stg;
    med.pUnkForRelease = NULL;
    hr = IDataObject_GetDataHere(data, &fmt, &med);
    if(SUCCEEDED(hr))
    {
        if (src_cf) *src_cf = embed_source_clipboard_format;
        return hr;
    }

    if (other_fmts)
    {
        for (i = 0; i < sizeof(fmt_id)/sizeof(fmt_id[0]); i++)
        {
            init_fmtetc(&fmt, fmt_id[i], TYMED_ISTORAGE);
            hr = IDataObject_QueryGetData(data, &fmt);
            if (SUCCEEDED(hr))
            {
                if (src_cf) *src_cf = fmt_id[i];
                return hr;
            }
        }
    }

    /* IPersistStorage */
    hr = IDataObject_QueryInterface(data, &IID_IPersistStorage, (void**)&persist);
    if(FAILED(hr)) return hr;

    hr = IPersistStorage_GetClassID(persist, &clsid);
    if(FAILED(hr)) goto end;

    hr = IStorage_SetClass(stg, &clsid);
    if(FAILED(hr)) goto end;

    hr = IPersistStorage_Save(persist, stg, FALSE);
    if(FAILED(hr)) goto end;

    hr = IPersistStorage_SaveCompleted(persist, NULL);

end:
    IPersistStorage_Release(persist);

    return hr;
}

/******************************************************************************
 *		OleCreateFromDataEx        [OLE32.@]
 *
 * Creates an embedded object from data transfer object retrieved from
 * the clipboard or OLE drag and drop.
 */
HRESULT WINAPI OleCreateFromDataEx(IDataObject *data, REFIID iid, DWORD flags,
                                   DWORD renderopt, ULONG num_cache_fmts, DWORD *adv_flags, FORMATETC *cache_fmts,
                                   IAdviseSink *sink, DWORD *conns,
                                   IOleClientSite *client_site, IStorage *stg, void **obj)
{
    HRESULT hr;
    UINT src_cf;

    FIXME("(%p, %s, %08x, %08x, %d, %p, %p, %p, %p, %p, %p, %p): stub\n",
          data, debugstr_guid(iid), flags, renderopt, num_cache_fmts, adv_flags, cache_fmts,
          sink, conns, client_site, stg, obj);

    hr = get_storage(data, stg, &src_cf, TRUE);
    if(FAILED(hr)) return hr;

    hr = OleLoad(stg, iid, client_site, obj);
    if(FAILED(hr)) return hr;

    /* FIXME: Init cache */

    return hr;
}

/******************************************************************************
 *		OleCreateFromData        [OLE32.@]
 */
HRESULT WINAPI OleCreateFromData(LPDATAOBJECT data, REFIID iid,
                                 DWORD renderopt, LPFORMATETC fmt,
                                 LPOLECLIENTSITE client_site, LPSTORAGE stg,
                                 LPVOID* obj)
{
    DWORD advf = ADVF_PRIMEFIRST;

    return OleCreateFromDataEx(data, iid, 0, renderopt, fmt ? 1 : 0, fmt ? &advf : NULL,
                               fmt, NULL, NULL, client_site, stg, obj);
}

/******************************************************************************
 *              OleCreateLinkFromData        [OLE32.@]
 */
HRESULT WINAPI OleCreateLinkFromData(IDataObject *data, REFIID iid,
                                     DWORD renderopt, FORMATETC *fmt,
                                     IOleClientSite *client_site, IStorage *stg,
                                     void **obj)
{
    FIXME("%p,%s,%08x,%p,%p,%p,%p: semi-stub\n",
          data, debugstr_guid(iid), renderopt, fmt, client_site, stg, obj);
    return OleCreateFromData(data, iid, renderopt, fmt, client_site, stg, obj);
}

/******************************************************************************
 *              OleCreateStaticFromData        [OLE32.@]
 */
HRESULT WINAPI OleCreateStaticFromData(IDataObject *data, REFIID iid,
                                       DWORD renderopt, FORMATETC *fmt,
                                       IOleClientSite *client_site, IStorage *stg,
                                       void **obj)
{
    FIXME("%p,%s,%08x,%p,%p,%p,%p: semi-stub\n",
          data, debugstr_guid(iid), renderopt, fmt, client_site, stg, obj);
    return OleCreateFromData(data, iid, renderopt, fmt, client_site, stg, obj);
}

/******************************************************************************
 *              OleCreateFromFileEx        [OLE32.@]
 */
HRESULT WINAPI OleCreateFromFileEx(REFCLSID clsid, const OLECHAR *filename, REFIID iid, DWORD flags,
                                   DWORD renderopt, ULONG num_fmts, DWORD *adv_flags, FORMATETC *fmts, IAdviseSink *sink,
                                   DWORD *conns, IOleClientSite *client_site, IStorage *stg, void **obj)
{
    HRESULT hr;
    IMoniker *mon;
    IDataObject *data;
    IUnknown *unk = NULL;
    IOleCache *cache = NULL;
    ULONG i;

    TRACE("cls %s, %s, iid %s, flags %d, render opts %d, num fmts %d, adv flags %p, fmts %p\n", debugstr_guid(clsid),
          debugstr_w(filename), debugstr_guid(iid), flags, renderopt, num_fmts, adv_flags, fmts);
    TRACE("sink %p, conns %p, client site %p, storage %p, obj %p\n", sink, conns, client_site, stg, obj);
    for (i = 0; i < num_fmts; i++)
        TRACE("\t%d: fmt %s adv flags %d\n", i, debugstr_formatetc(fmts + i), adv_flags[i]);

    hr = CreateFileMoniker( filename, &mon );
    if (FAILED(hr)) return hr;

    hr = BindMoniker( mon, 0, &IID_IDataObject, (void**)&data );
    IMoniker_Release( mon );
    if (FAILED(hr)) return hr;

    hr = get_storage( data, stg, NULL, FALSE );
    if (FAILED(hr)) goto end;

    hr = OleLoad( stg, &IID_IUnknown, client_site, (void**)&unk );
    if (FAILED(hr)) goto end;

    if (renderopt == OLERENDER_FORMAT)
    {
        hr = IUnknown_QueryInterface( unk, &IID_IOleCache, (void**)&cache );
        if (FAILED(hr)) goto end;

        for (i = 0; i < num_fmts; i++)
        {
            STGMEDIUM med;
            DWORD dummy_conn;

            memset( &med, 0, sizeof(med) );
            hr = IDataObject_GetData( data, fmts + i, &med );
            if (FAILED(hr)) goto end;
            hr = IOleCache_Cache( cache, fmts + i, adv_flags[i], &dummy_conn );
            if (SUCCEEDED(hr))
                hr = IOleCache_SetData( cache, fmts + i, &med, TRUE );
            if (FAILED(hr))
            {
                ReleaseStgMedium( &med );
                goto end;
            }
        }
    }

    hr = IUnknown_QueryInterface( unk, iid, obj );

end:
    if (cache) IOleCache_Release( cache );
    if (unk) IUnknown_Release( unk );
    IDataObject_Release( data );
    return hr;
}

/******************************************************************************
 *              OleCreateFromFile        [OLE32.@]
 */
HRESULT WINAPI OleCreateFromFile(REFCLSID clsid, const OLECHAR *filename, REFIID iid, DWORD renderopt,
                                 FORMATETC *fmt, IOleClientSite *client_site, IStorage *storage, void **obj)
{
    DWORD advf = ADVF_PRIMEFIRST;

    return OleCreateFromFileEx(clsid, filename, iid, 0, renderopt, fmt ? 1 : 0, fmt ? &advf : NULL, fmt,
                               NULL, NULL, client_site, storage, obj);
}

/******************************************************************************
 *              OleDuplicateData        [OLE32.@]
 *
 * Duplicates clipboard data.
 *
 * PARAMS
 *  hSrc     [I] Handle of the source clipboard data.
 *  cfFormat [I] The clipboard format of hSrc.
 *  uiFlags  [I] Flags to pass to GlobalAlloc.
 *
 * RETURNS
 *  Success: handle to the duplicated data.
 *  Failure: NULL.
 */
HANDLE WINAPI OleDuplicateData(HANDLE hSrc, CLIPFORMAT cfFormat,
	                          UINT uiFlags)
{
    HANDLE hDst = NULL;

    TRACE("(%p,%x,%x)\n", hSrc, cfFormat, uiFlags);

    if (!uiFlags) uiFlags = GMEM_MOVEABLE;

    switch (cfFormat)
    {
    case CF_ENHMETAFILE:
        hDst = CopyEnhMetaFileW(hSrc, NULL);
        break;
    case CF_METAFILEPICT:
        hDst = CopyMetaFileW(hSrc, NULL);
        break;
    case CF_PALETTE:
        {
            LOGPALETTE * logpalette;
            UINT nEntries = GetPaletteEntries(hSrc, 0, 0, NULL);
            if (!nEntries) return NULL;
            logpalette = HeapAlloc(GetProcessHeap(), 0,
                FIELD_OFFSET(LOGPALETTE, palPalEntry[nEntries]));
            if (!logpalette) return NULL;
            if (!GetPaletteEntries(hSrc, 0, nEntries, logpalette->palPalEntry))
            {
                HeapFree(GetProcessHeap(), 0, logpalette);
                return NULL;
            }
            logpalette->palVersion = 0x300;
            logpalette->palNumEntries = (WORD)nEntries;

            hDst = CreatePalette(logpalette);

            HeapFree(GetProcessHeap(), 0, logpalette);
            break;
        }
    case CF_BITMAP:
        {
            LONG size;
            BITMAP bm;
            if (!GetObjectW(hSrc, sizeof(bm), &bm))
                return NULL;
            size = GetBitmapBits(hSrc, 0, NULL);
            if (!size) return NULL;
            bm.bmBits = HeapAlloc(GetProcessHeap(), 0, size);
            if (!bm.bmBits) return NULL;
            if (GetBitmapBits(hSrc, size, bm.bmBits))
                hDst = CreateBitmapIndirect(&bm);
            HeapFree(GetProcessHeap(), 0, bm.bmBits);
            break;
        }
    default:
        {
            SIZE_T size = GlobalSize(hSrc);
            LPVOID pvSrc = NULL;
            LPVOID pvDst = NULL;

            /* allocate space for object */
            if (!size) return NULL;
            hDst = GlobalAlloc(uiFlags, size);
            if (!hDst) return NULL;

            /* lock pointers */
            pvSrc = GlobalLock(hSrc);
            if (!pvSrc)
            {
                GlobalFree(hDst);
                return NULL;
            }
            pvDst = GlobalLock(hDst);
            if (!pvDst)
            {
                GlobalUnlock(hSrc);
                GlobalFree(hDst);
                return NULL;
            }
            /* copy data */
            memcpy(pvDst, pvSrc, size);

            /* cleanup */
            GlobalUnlock(hDst);
            GlobalUnlock(hSrc);
        }
    }

    TRACE("returning %p\n", hDst);
    return hDst;
}
