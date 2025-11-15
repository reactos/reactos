/*
 * Clipboard unit tests
 *
 * Copyright 2006 Kevin Koltzau
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

#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "shellapi.h"
#include "shlobj.h"

#include "wine/test.h"

#define InitFormatEtc(fe, cf, med) \
        {\
        (fe).cfFormat=cf;\
        (fe).dwAspect=DVASPECT_CONTENT;\
        (fe).ptd=NULL;\
        (fe).tymed=med;\
        (fe).lindex=-1;\
        };

static inline char *dump_fmtetc(FORMATETC *fmt)
{
    static char buf[100];

    sprintf(buf, "cf %04x ptd %p aspect %lx lindex %ld tymed %lx",
             fmt->cfFormat, fmt->ptd, fmt->dwAspect, fmt->lindex, fmt->tymed);
    return buf;
}

typedef struct DataObjectImpl {
    IDataObject IDataObject_iface;
    LONG ref;

    FORMATETC *fmtetc;
    UINT fmtetc_cnt;

    HANDLE text;
    IStream *stm;
    IStorage *stg;
    HMETAFILEPICT hmfp;
} DataObjectImpl;

typedef struct EnumFormatImpl {
    IEnumFORMATETC IEnumFORMATETC_iface;
    LONG ref;

    FORMATETC *fmtetc;
    UINT fmtetc_cnt;

    UINT cur;
} EnumFormatImpl;

static BOOL expect_DataObjectImpl_QueryGetData = TRUE;
static ULONG DataObjectImpl_GetData_calls = 0;
static ULONG DataObjectImpl_GetDataHere_calls = 0;
static ULONG DataObjectImpl_EnumFormatEtc_calls = 0;

static UINT cf_stream, cf_storage, cf_global, cf_another, cf_onemore;

static HRESULT EnumFormatImpl_Create(FORMATETC *fmtetc, UINT size, LPENUMFORMATETC *lplpformatetc);

static HMETAFILE create_mf(void)
{
    RECT rect = {0, 0, 100, 100};
    HDC hdc = CreateMetaFileA(NULL);
    ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &rect, "Test String", strlen("Test String"), NULL);
    return CloseMetaFile(hdc);
}

static HMETAFILEPICT create_metafilepict(void)
{
    HGLOBAL ret = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAFILEPICT));
    METAFILEPICT *mf = GlobalLock(ret);
    mf->mm = MM_ANISOTROPIC;
    mf->xExt = 100;
    mf->yExt = 200;
    mf->hMF = create_mf();
    GlobalUnlock(ret);
    return ret;
}

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
    return ref;
}

static ULONG WINAPI EnumFormatImpl_Release(IEnumFORMATETC *iface)
{
    EnumFormatImpl *This = impl_from_IEnumFORMATETC(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if(!ref) {
        free(This->fmtetc);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI EnumFormatImpl_Next(IEnumFORMATETC *iface, ULONG celt,
                                          FORMATETC *rgelt, ULONG *pceltFetched)
{
    EnumFormatImpl *This = impl_from_IEnumFORMATETC(iface);
    ULONG count, i;

    if (winetest_debug > 1)
        trace("next: count %ld cur %d\n", celt, This->cur);

    if(!rgelt)
        return E_INVALIDARG;

    count = min(celt, This->fmtetc_cnt - This->cur);
    for(i = 0; i < count; i++, This->cur++, rgelt++)
    {
        *rgelt = This->fmtetc[This->cur];
        if(rgelt->ptd)
        {
            DWORD size = This->fmtetc[This->cur].ptd->tdSize;
            rgelt->ptd = CoTaskMemAlloc(size);
            memcpy(rgelt->ptd, This->fmtetc[This->cur].ptd, size);
        }
    }
    if(pceltFetched)
        *pceltFetched = count;
    return count == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumFormatImpl_Skip(IEnumFORMATETC *iface, ULONG celt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumFormatImpl_Reset(IEnumFORMATETC *iface)
{
    EnumFormatImpl *This = impl_from_IEnumFORMATETC(iface);

    This->cur = 0;
    return S_OK;
}

static HRESULT WINAPI EnumFormatImpl_Clone(IEnumFORMATETC *iface, IEnumFORMATETC **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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

static HRESULT EnumFormatImpl_Create(FORMATETC *fmtetc, UINT fmtetc_cnt, IEnumFORMATETC **lplpformatetc)
{
    EnumFormatImpl *ret;

    ret = malloc(sizeof(EnumFormatImpl));
    ret->IEnumFORMATETC_iface.lpVtbl = &VT_EnumFormatImpl;
    ret->ref = 1;
    ret->cur = 0;
    ret->fmtetc_cnt = fmtetc_cnt;
    ret->fmtetc = malloc(fmtetc_cnt * sizeof(FORMATETC));
    memcpy(ret->fmtetc, fmtetc, fmtetc_cnt*sizeof(FORMATETC));
    *lplpformatetc = &ret->IEnumFORMATETC_iface;
    return S_OK;
}

static HRESULT WINAPI DataObjectImpl_QueryInterface(IDataObject *iface, REFIID riid, LPVOID *ppvObj)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);

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
    return ref;
}

static ULONG WINAPI DataObjectImpl_Release(IDataObject* iface)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if(!ref)
    {
        int i;
        if(This->text) GlobalFree(This->text);
        for(i = 0; i < This->fmtetc_cnt; i++)
            free(This->fmtetc[i].ptd);
        free(This->fmtetc);
        if(This->stm) IStream_Release(This->stm);
        if(This->stg) IStorage_Release(This->stg);
        if(This->hmfp) {
            METAFILEPICT *mfp = GlobalLock(This->hmfp);
            DeleteMetaFile(mfp->hMF);
            GlobalUnlock(This->hmfp);
            GlobalFree(This->hmfp);
        }
        free(This);
    }

    return ref;
}

static HRESULT WINAPI DataObjectImpl_GetData(IDataObject* iface, FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    UINT i;

    trace("getdata: %s\n", dump_fmtetc(pformatetc));

    DataObjectImpl_GetData_calls++;

    ok(pmedium->tymed == 0, "pmedium->tymed = %lu\n", pmedium->tymed);
    ok(pmedium->hGlobal == NULL, "pmedium->hGlobal = %p\n", pmedium->hGlobal);
    ok(pmedium->pUnkForRelease == NULL, "pmedium->pUnkForRelease = %p\n", pmedium->pUnkForRelease);

    if(pformatetc->lindex != -1)
        return DV_E_FORMATETC;

    for(i = 0; i < This->fmtetc_cnt; i++)
    {
        if(This->fmtetc[i].cfFormat == pformatetc->cfFormat)
        {
            if(This->fmtetc[i].tymed & pformatetc->tymed)
            {
                pmedium->pUnkForRelease = (LPUNKNOWN)iface;
                IUnknown_AddRef(pmedium->pUnkForRelease);

                if(pformatetc->cfFormat == CF_TEXT || pformatetc->cfFormat == cf_global)
                {
                    pmedium->tymed = TYMED_HGLOBAL;
                    pmedium->hGlobal = This->text;
                }
                else if(pformatetc->cfFormat == cf_stream)
                {
                    pmedium->tymed = TYMED_ISTREAM;
                    IStream_AddRef(This->stm);
                    pmedium->pstm = This->stm;
                }
                else if(pformatetc->cfFormat == cf_storage || pformatetc->cfFormat == cf_another)
                {
                    pmedium->tymed = TYMED_ISTORAGE;
                    IStorage_AddRef(This->stg);
                    pmedium->pstg = This->stg;
                }
                else if(pformatetc->cfFormat == CF_METAFILEPICT)
                {
                    pmedium->tymed = TYMED_MFPICT;
                    pmedium->hMetaFilePict = This->hmfp;
                }
                return S_OK;
            }
        }
    }

    return E_FAIL;
}

static HRESULT WINAPI DataObjectImpl_GetDataHere(IDataObject* iface, FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    trace("getdatahere: %s\n", dump_fmtetc(pformatetc));
    DataObjectImpl_GetDataHere_calls++;

    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_QueryGetData(IDataObject* iface, FORMATETC *pformatetc)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);
    UINT i;
    BOOL foundFormat = FALSE;

    trace("querygetdata: %s\n", dump_fmtetc(pformatetc));
    if (!expect_DataObjectImpl_QueryGetData)
        ok(0, "unexpected call to DataObjectImpl_QueryGetData\n");

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

static HRESULT WINAPI DataObjectImpl_GetCanonicalFormatEtc(IDataObject* iface, FORMATETC *pformatectIn,
                                                           FORMATETC *pformatetcOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_SetData(IDataObject* iface, FORMATETC *pformatetc,
                                             STGMEDIUM *pmedium, BOOL fRelease)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_EnumFormatEtc(IDataObject* iface, DWORD dwDirection,
                                                   IEnumFORMATETC **ppenumFormatEtc)
{
    DataObjectImpl *This = impl_from_IDataObject(iface);

    DataObjectImpl_EnumFormatEtc_calls++;

    if(dwDirection != DATADIR_GET) {
        ok(0, "unexpected direction %ld\n", dwDirection);
        return E_NOTIMPL;
    }
    return EnumFormatImpl_Create(This->fmtetc, This->fmtetc_cnt, ppenumFormatEtc);
}

static HRESULT WINAPI DataObjectImpl_DAdvise(IDataObject* iface, FORMATETC *pformatetc, DWORD advf,
                                             IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_DUnadvise(IDataObject* iface, DWORD dwConnection)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_EnumDAdvise(IDataObject* iface, IEnumSTATDATA **ppenumAdvise)
{
    ok(0, "unexpected call\n");
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

static HRESULT DataObjectImpl_CreateFromHGlobal(HGLOBAL text, LPDATAOBJECT *dataobj)
{
    DataObjectImpl *obj;

    obj = malloc(sizeof(DataObjectImpl));
    obj->IDataObject_iface.lpVtbl = &VT_DataObjectImpl;
    obj->ref = 1;
    obj->text = text;
    obj->stm = NULL;
    obj->stg = NULL;
    obj->hmfp = NULL;

    obj->fmtetc_cnt = 1;
    obj->fmtetc = malloc(obj->fmtetc_cnt * sizeof(FORMATETC));
    InitFormatEtc(obj->fmtetc[0], CF_TEXT, TYMED_HGLOBAL);

    *dataobj = &obj->IDataObject_iface;
    return S_OK;
}

static HRESULT DataObjectImpl_CreateText(LPCSTR text, LPDATAOBJECT *lplpdataobj)
{
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, strlen(text) + 1);
    strcpy(GlobalLock(h), text);
    GlobalUnlock(h);
    return DataObjectImpl_CreateFromHGlobal(h, lplpdataobj);
}

static const char *cmpl_stm_data = "complex stream";
static const char *cmpl_text_data = "complex text";
static const WCHAR device_name[] = {'m','y','d','e','v',0};

static HRESULT DataObjectImpl_CreateComplex(LPDATAOBJECT *lplpdataobj)
{
    DataObjectImpl *obj;
    ILockBytes *lbs;
    DEVMODEW dm;

    obj = malloc(sizeof(DataObjectImpl));
    obj->IDataObject_iface.lpVtbl = &VT_DataObjectImpl;
    obj->ref = 1;
    obj->text = GlobalAlloc(GMEM_MOVEABLE, strlen(cmpl_text_data) + 1);
    strcpy(GlobalLock(obj->text), cmpl_text_data);
    GlobalUnlock(obj->text);
    CreateStreamOnHGlobal(NULL, TRUE, &obj->stm);
    IStream_Write(obj->stm, cmpl_stm_data, strlen(cmpl_stm_data), NULL);

    CreateILockBytesOnHGlobal(NULL, TRUE, &lbs);
    StgCreateDocfileOnILockBytes(lbs, STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &obj->stg);
    ILockBytes_Release(lbs);

    obj->hmfp = create_metafilepict();

    obj->fmtetc_cnt = 9;
    /* zeroing here since FORMATETC has a hole in it, and it's confusing to have this uninitialised. */
    obj->fmtetc = calloc(obj->fmtetc_cnt, sizeof(FORMATETC));
    InitFormatEtc(obj->fmtetc[0], CF_TEXT, TYMED_HGLOBAL);
    InitFormatEtc(obj->fmtetc[1], cf_stream, TYMED_ISTREAM);
    InitFormatEtc(obj->fmtetc[2], cf_storage, TYMED_ISTORAGE);
    InitFormatEtc(obj->fmtetc[3], cf_another, TYMED_ISTORAGE|TYMED_ISTREAM|TYMED_HGLOBAL);
    if (0)  /* Causes crashes on both Wine and Windows */
    {
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmDriverExtra = 0;
        lstrcpyW(dm.dmDeviceName, device_name);
        obj->fmtetc[3].ptd = malloc(FIELD_OFFSET(DVTARGETDEVICE, tdData) + sizeof(device_name) + dm.dmSize + dm.dmDriverExtra);
        obj->fmtetc[3].ptd->tdSize = FIELD_OFFSET(DVTARGETDEVICE, tdData) + sizeof(device_name) + dm.dmSize + dm.dmDriverExtra;
        obj->fmtetc[3].ptd->tdDriverNameOffset = FIELD_OFFSET(DVTARGETDEVICE, tdData);
        obj->fmtetc[3].ptd->tdDeviceNameOffset = 0;
        obj->fmtetc[3].ptd->tdPortNameOffset   = 0;
        obj->fmtetc[3].ptd->tdExtDevmodeOffset = obj->fmtetc[3].ptd->tdDriverNameOffset + sizeof(device_name);
        lstrcpyW((WCHAR*)obj->fmtetc[3].ptd->tdData, device_name);
        memcpy(obj->fmtetc[3].ptd->tdData + sizeof(device_name), &dm, dm.dmSize + dm.dmDriverExtra);
    }

    InitFormatEtc(obj->fmtetc[4], cf_global, TYMED_HGLOBAL);
    InitFormatEtc(obj->fmtetc[5], cf_another, TYMED_HGLOBAL);
    InitFormatEtc(obj->fmtetc[6], cf_another, 0xfffff);
    InitFormatEtc(obj->fmtetc[7], cf_another, 0xfffff);
    obj->fmtetc[7].dwAspect = DVASPECT_ICON;
    InitFormatEtc(obj->fmtetc[8], CF_METAFILEPICT, TYMED_MFPICT);

    *lplpdataobj = &obj->IDataObject_iface;
    return S_OK;
}

static void test_get_clipboard_uninitialized(void)
{
    REFCLSID rclsid = &CLSID_InternetZoneManager;
    IDataObject *pDObj;
    IUnknown *pUnk;
    HRESULT hr;

    pDObj = (IDataObject *)0xdeadbeef;
    hr = OleGetClipboard(&pDObj);
    ok(hr == S_OK, "OleGetClipboard() got 0x%08lx instead of 0x%08lx\n", hr, S_OK);
    ok(!!pDObj && pDObj != (IDataObject *)0xdeadbeef, "Got unexpected pDObj %p.\n", pDObj);

    /* COM is still not initialized. */
    hr = CoCreateInstance(rclsid, NULL, 0x17, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "Got unexpected hr %#lx.\n", hr);

    hr = OleFlushClipboard();
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = OleIsCurrentClipboard(pDObj);
    ok(hr == S_FALSE, "Got unexpected hr %#lx.\n", hr);

    IDataObject_Release(pDObj);
}

static void test_get_clipboard(void)
{
    HRESULT hr;
    IDataObject *data_obj;
    FORMATETC fmtetc;
    STGMEDIUM stgmedium;

    hr = OleGetClipboard(NULL);
    ok(hr == E_INVALIDARG, "OleGetClipboard(NULL) should return E_INVALIDARG instead of 0x%08lx\n", hr);

    hr = OleGetClipboard(&data_obj);
    ok(hr == S_OK, "OleGetClipboard failed with error 0x%08lx\n", hr);

    /* test IDataObject_QueryGetData */

    /* clipboard's IDataObject_QueryGetData shouldn't defer to our IDataObject_QueryGetData */
    expect_DataObjectImpl_QueryGetData = FALSE;

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == S_OK, "IDataObject_QueryGetData failed with error 0x%08lx\n", hr);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.dwAspect = 0xdeadbeef;
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == DV_E_FORMATETC, "IDataObject_QueryGetData should have failed with DV_E_FORMATETC instead of 0x%08lx\n", hr);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.dwAspect = DVASPECT_THUMBNAIL;
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == DV_E_FORMATETC, "IDataObject_QueryGetData should have failed with DV_E_FORMATETC instead of 0x%08lx\n", hr);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.lindex = 256;
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == DV_E_FORMATETC || broken(hr == S_OK),
        "IDataObject_QueryGetData should have failed with DV_E_FORMATETC instead of 0x%08lx\n", hr);

    InitFormatEtc(fmtetc, CF_RIFF, TYMED_HGLOBAL);
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == DV_E_CLIPFORMAT, "IDataObject_QueryGetData should have failed with DV_E_CLIPFORMAT instead of 0x%08lx\n", hr);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_FILE);
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == S_OK, "IDataObject_QueryGetData failed with error 0x%08lx\n", hr);

    expect_DataObjectImpl_QueryGetData = TRUE;

    /* test IDataObject_GetData */

    DataObjectImpl_GetData_calls = 0;

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == S_OK, "IDataObject_GetData failed with error 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&stgmedium);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.dwAspect = 0xdeadbeef;
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == S_OK, "IDataObject_GetData failed with error 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&stgmedium);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.dwAspect = DVASPECT_THUMBNAIL;
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == S_OK, "IDataObject_GetData failed with error 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&stgmedium);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.lindex = 256;
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == DV_E_FORMATETC || broken(hr == S_OK), "IDataObject_GetData should have failed with DV_E_FORMATETC instead of 0x%08lx\n", hr);
    if (hr == S_OK)
    {
        /* undo the unexpected success */
        DataObjectImpl_GetData_calls--;
        ReleaseStgMedium(&stgmedium);
    }

    InitFormatEtc(fmtetc, CF_RIFF, TYMED_HGLOBAL);
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == DV_E_FORMATETC, "IDataObject_GetData should have failed with DV_E_FORMATETC instead of 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&stgmedium);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_FILE);
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == DV_E_TYMED, "IDataObject_GetData should have failed with DV_E_TYMED instead of 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&stgmedium);

    ok(DataObjectImpl_GetData_calls == 6, "DataObjectImpl_GetData should have been called 6 times instead of %ld times\n", DataObjectImpl_GetData_calls);

    IDataObject_Release(data_obj);
}

static void test_enum_fmtetc(IDataObject *src)
{
    HRESULT hr;
    IDataObject *data;
    IEnumFORMATETC *enum_fmt, *src_enum;
    FORMATETC fmt, src_fmt;
    DWORD count = 0;

    hr = OleGetClipboard(&data);
    ok(hr == S_OK, "OleGetClipboard failed with error 0x%08lx\n", hr);

    hr = IDataObject_EnumFormatEtc(data, DATADIR_SET, &enum_fmt);
    ok(hr == E_NOTIMPL ||
       broken(hr == E_INVALIDARG), /* win98 (not win98SE) */
       "got %08lx\n", hr);

    DataObjectImpl_EnumFormatEtc_calls = 0;
    hr = IDataObject_EnumFormatEtc(data, DATADIR_GET, &enum_fmt);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(DataObjectImpl_EnumFormatEtc_calls == 0, "EnumFormatEtc was called\n");
    if (FAILED(hr))
    {
        skip("EnumFormatEtc failed, skipping tests.\n");
        return;
    }

    if(src) IDataObject_EnumFormatEtc(src, DATADIR_GET, &src_enum);

    while((hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL)) == S_OK)
    {
        ok(src != NULL, "shouldn't be here\n");
        hr = IEnumFORMATETC_Next(src_enum, 1, &src_fmt, NULL);
        ok(hr == S_OK, "%ld: got %08lx\n", count, hr);
        trace("%ld: %s\n", count, dump_fmtetc(&fmt));
        ok(fmt.cfFormat == src_fmt.cfFormat, "%ld: %04x %04x\n", count, fmt.cfFormat, src_fmt.cfFormat);
        ok(fmt.dwAspect == src_fmt.dwAspect, "%ld: %08lx %08lx\n", count, fmt.dwAspect, src_fmt.dwAspect);
        ok(fmt.lindex == src_fmt.lindex, "%ld: %08lx %08lx\n", count, fmt.lindex, src_fmt.lindex);
        ok(fmt.tymed == src_fmt.tymed, "%ld: %08lx %08lx\n", count, fmt.tymed, src_fmt.tymed);
        if(fmt.ptd)
        {
            ok(src_fmt.ptd != NULL, "%ld: expected non-NULL\n", count);
            CoTaskMemFree(fmt.ptd);
            CoTaskMemFree(src_fmt.ptd);
        }
        count++;
    }

    ok(hr == S_FALSE, "%ld: got %08lx\n", count, hr);

    if(src)
    {
        hr = IEnumFORMATETC_Next(src_enum, 1, &src_fmt, NULL);
        ok(hr == S_FALSE, "%ld: got %08lx\n", count, hr);
        IEnumFORMATETC_Release(src_enum);
    }

    hr = IEnumFORMATETC_Reset(enum_fmt);
    ok(hr == S_OK, "got %08lx\n", hr);

    if(src) /* Exercise the enumerator a bit */
    {
        IEnumFORMATETC *clone;
        FORMATETC third_fmt;

        hr = IEnumFORMATETC_Next(enum_fmt, 1, &third_fmt, NULL);
        ok(hr == S_OK, "got %08lx\n", hr);
        hr = IEnumFORMATETC_Next(enum_fmt, 1, &third_fmt, NULL);
        ok(hr == S_OK, "got %08lx\n", hr);
        hr = IEnumFORMATETC_Next(enum_fmt, 1, &third_fmt, NULL);
        ok(hr == S_OK, "got %08lx\n", hr);

        hr = IEnumFORMATETC_Reset(enum_fmt);
        ok(hr == S_OK, "got %08lx\n", hr);
        hr = IEnumFORMATETC_Skip(enum_fmt, 2);
        ok(hr == S_OK, "got %08lx\n", hr);

        hr = IEnumFORMATETC_Clone(enum_fmt, &clone);
        ok(hr == S_OK, "got %08lx\n", hr);
        hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(fmt.cfFormat == third_fmt.cfFormat, "formats don't match\n");
        hr = IEnumFORMATETC_Next(clone, 1, &fmt, NULL);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(fmt.cfFormat == third_fmt.cfFormat, "formats don't match\n");
        IEnumFORMATETC_Release(clone);
    }

    IEnumFORMATETC_Release(enum_fmt);
    IDataObject_Release(data);
}

static void test_no_cf_dataobject(void)
{
    UINT cf_dataobject = RegisterClipboardFormatA("DataObject");
    UINT cf_ole_priv_data = RegisterClipboardFormatA("Ole Private Data");
    HANDLE h;
    OpenClipboard(NULL);

    h = GetClipboardData(cf_dataobject);
    ok(!h, "got %p\n", h);
    h = GetClipboardData(cf_ole_priv_data);
    ok(!h, "got %p\n", h);

    CloseClipboard();
}

static void test_cf_dataobject(IDataObject *data)
{
    UINT cf = 0;
    UINT cf_dataobject = RegisterClipboardFormatA("DataObject");
    UINT cf_ole_priv_data = RegisterClipboardFormatA("Ole Private Data");
    BOOL found_dataobject = FALSE, found_priv_data = FALSE;

    OpenClipboard(NULL);
    do
    {
        cf = EnumClipboardFormats(cf);
        if(cf == cf_dataobject)
        {
            HGLOBAL h = GetClipboardData(cf);
            HWND *ptr = GlobalLock(h);
            DWORD size = GlobalSize(h);
            HWND clip_owner = GetClipboardOwner();

            found_dataobject = TRUE;
            ok(size >= sizeof(*ptr), "size %ld\n", size);
            if(data)
                ok(*ptr == clip_owner, "hwnd %p clip_owner %p\n", *ptr, clip_owner);
            else /* ole clipboard flushed */
                ok(*ptr == NULL, "hwnd %p\n", *ptr);
            GlobalUnlock(h);
        }
        else if(cf == cf_ole_priv_data)
        {
            found_priv_data = TRUE;
            if(data)
            {
                HGLOBAL h = GetClipboardData(cf);
                DWORD *ptr = GlobalLock(h);
                DWORD size = GlobalSize(h);

                if(size != ptr[1])
                    win_skip("Ole Private Data in win9x format\n");
                else
                {
                    HRESULT hr;
                    IEnumFORMATETC *enum_fmt;
                    DWORD count = 0;
                    FORMATETC fmt;
                    struct formatetcetc
                    {
                        FORMATETC fmt;
                        BOOL first_use_of_cf;
                        DWORD res[2];
                    } *fmt_ptr;
                    struct priv_data
                    {
                        DWORD res1;
                        DWORD size;
                        DWORD res2;
                        DWORD count;
                        DWORD res3[2];
                        struct formatetcetc fmts[1];
                    } *priv = (struct priv_data*)ptr;
                    CLIPFORMAT cfs_seen[10];

                    hr = IDataObject_EnumFormatEtc(data, DATADIR_GET, &enum_fmt);
                    ok(hr == S_OK, "got %08lx\n", hr);
                    fmt_ptr = priv->fmts;

                    while(IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL) == S_OK)
                    {
                        int i;
                        BOOL seen_cf = FALSE;

                        ok(fmt_ptr->fmt.cfFormat == fmt.cfFormat,
                           "got %08x expected %08x\n", fmt_ptr->fmt.cfFormat, fmt.cfFormat);
                        ok(fmt_ptr->fmt.dwAspect == fmt.dwAspect, "got %08lx expected %08lx\n",
                           fmt_ptr->fmt.dwAspect, fmt.dwAspect);
                        ok(fmt_ptr->fmt.lindex == fmt.lindex, "got %08lx expected %08lx\n",
                           fmt_ptr->fmt.lindex, fmt.lindex);
                        ok(fmt_ptr->fmt.tymed == fmt.tymed, "got %08lx expected %08lx\n",
                           fmt_ptr->fmt.tymed, fmt.tymed);
                        for(i = 0; i < count; i++)
                            if(fmt_ptr->fmt.cfFormat == cfs_seen[i])
                            {
                                seen_cf = TRUE;
                                break;
                            }
                        cfs_seen[count] = fmt.cfFormat;
                        ok(fmt_ptr->first_use_of_cf != seen_cf, "got %08x expected %08x\n",
                           fmt_ptr->first_use_of_cf, !seen_cf);
                        ok(fmt_ptr->res[0] == 0, "got %08lx\n", fmt_ptr->res[0]);
                        ok(fmt_ptr->res[1] == 0, "got %08lx\n", fmt_ptr->res[1]);
                        if(fmt.ptd)
                        {
                            DVTARGETDEVICE *target;

                            ok(fmt_ptr->fmt.ptd != NULL, "target device offset zero\n");
                            target = (DVTARGETDEVICE*)((char*)priv + (DWORD_PTR)fmt_ptr->fmt.ptd);
                            ok(!memcmp(target, fmt.ptd, fmt.ptd->tdSize), "target devices differ\n");
                            CoTaskMemFree(fmt.ptd);
                        }
                        fmt_ptr++;
                        count++;
                    }
                    ok(priv->res1 == 0, "got %08lx\n", priv->res1);
                    ok(priv->res2 == 1, "got %08lx\n", priv->res2);
                    ok(priv->count == count, "got %08lx expected %08lx\n", priv->count, count);
                    ok(priv->res3[0] == 0, "got %08lx\n", priv->res3[0]);

                    /* win64 sets the lsb */
                    if(sizeof(fmt_ptr->fmt.ptd) == 8)
                        todo_wine ok(priv->res3[1] == 1, "got %08lx\n", priv->res3[1]);
                    else
                        ok(priv->res3[1] == 0, "got %08lx\n", priv->res3[1]);

                    GlobalUnlock(h);
                    IEnumFORMATETC_Release(enum_fmt);
                }
            }
        }
        else if(cf == cf_stream)
        {
            HGLOBAL h;
            void *ptr;
            DWORD size;

            DataObjectImpl_GetDataHere_calls = 0;
            h = GetClipboardData(cf);
            ok(DataObjectImpl_GetDataHere_calls == 1, "got %ld\n", DataObjectImpl_GetDataHere_calls);
            ptr = GlobalLock(h);
            size = GlobalSize(h);
            ok(size == strlen(cmpl_stm_data),
               "expected %d got %ld\n", lstrlenA(cmpl_stm_data), size);
            ok(!memcmp(ptr, cmpl_stm_data, strlen(cmpl_stm_data)), "mismatch\n");
            GlobalUnlock(h);
        }
        else if(cf == cf_global)
        {
            HGLOBAL h;
            void *ptr;
            DWORD size;

            DataObjectImpl_GetDataHere_calls = 0;
            h = GetClipboardData(cf);
            ok(DataObjectImpl_GetDataHere_calls == 0, "got %ld\n", DataObjectImpl_GetDataHere_calls);
            ptr = GlobalLock(h);
            size = GlobalSize(h);
            ok(size == strlen(cmpl_text_data) + 1,
               "expected %d got %ld\n", lstrlenA(cmpl_text_data) + 1, size);
            ok(!memcmp(ptr, cmpl_text_data, strlen(cmpl_text_data) + 1), "mismatch\n");
            GlobalUnlock(h);
        }
    } while(cf);
    CloseClipboard();
    ok(found_dataobject, "didn't find cf_dataobject\n");
    ok(found_priv_data, "didn't find cf_ole_priv_data\n");
}

static void test_complex_get_clipboard(void)
{
    HRESULT hr;
    IDataObject *data_obj;
    FORMATETC fmtetc;
    STGMEDIUM stgmedium;

    hr = OleGetClipboard(&data_obj);
    ok(hr == S_OK, "OleGetClipboard failed with error 0x%08lx\n", hr);

    DataObjectImpl_GetData_calls = 0;

    InitFormatEtc(fmtetc, CF_METAFILEPICT, TYMED_MFPICT);
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == S_OK, "IDataObject_GetData failed with error 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&stgmedium);

    InitFormatEtc(fmtetc, CF_METAFILEPICT, TYMED_HGLOBAL);
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == DV_E_TYMED, "IDataObject_GetData failed with error 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&stgmedium);

    InitFormatEtc(fmtetc, CF_ENHMETAFILE, TYMED_HGLOBAL);
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == DV_E_TYMED, "IDataObject_GetData failed with error 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&stgmedium);

    InitFormatEtc(fmtetc, CF_ENHMETAFILE, TYMED_ENHMF);
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == S_OK, "IDataObject_GetData failed with error 0x%08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&stgmedium);

    ok(DataObjectImpl_GetData_calls == 5,
            "DataObjectImpl_GetData called 5 times instead of %ld times\n",
            DataObjectImpl_GetData_calls);
    IDataObject_Release(data_obj);
}

static void test_set_clipboard(void)
{
    HRESULT hr;
    ULONG ref;
    LPDATAOBJECT data1, data2, data_cmpl;
    HGLOBAL hblob, h;
    void *ptr;

    cf_stream = RegisterClipboardFormatA("stream format");
    cf_storage = RegisterClipboardFormatA("storage format");
    cf_global = RegisterClipboardFormatA("global format");
    cf_another = RegisterClipboardFormatA("another format");
    cf_onemore = RegisterClipboardFormatA("one more format");

    hr = DataObjectImpl_CreateText("data1", &data1);
    ok(hr == S_OK, "Failed to create data1 object: 0x%08lx\n", hr);
    if(FAILED(hr))
        return;
    hr = DataObjectImpl_CreateText("data2", &data2);
    ok(hr == S_OK, "Failed to create data2 object: 0x%08lx\n", hr);
    if(FAILED(hr))
        return;
    hr = DataObjectImpl_CreateComplex(&data_cmpl);
    ok(hr == S_OK, "Failed to create complex data object: 0x%08lx\n", hr);
    if(FAILED(hr))
        return;

    hr = OleSetClipboard(data1);
    ok(hr == CO_E_NOTINITIALIZED, "OleSetClipboard should have failed with CO_E_NOTINITIALIZED instead of 0x%08lx\n", hr);

    CoInitialize(NULL);
    hr = OleSetClipboard(data1);
    ok(hr == CO_E_NOTINITIALIZED, "OleSetClipboard failed with 0x%08lx\n", hr);
    CoUninitialize();

    hr = OleInitialize(NULL);
    ok(hr == S_OK, "OleInitialize failed with error 0x%08lx\n", hr);

    hr = OleSetClipboard(data1);
    ok(hr == S_OK, "failed to set clipboard to data1, hr = 0x%08lx\n", hr);

    test_cf_dataobject(data1);

    hr = OleIsCurrentClipboard(data1);
    ok(hr == S_OK, "expected current clipboard to be data1, hr = 0x%08lx\n", hr);
    hr = OleIsCurrentClipboard(data2);
    ok(hr == S_FALSE, "did not expect current clipboard to be data2, hr = 0x%08lx\n", hr);
    hr = OleIsCurrentClipboard(NULL);
    ok(hr == S_FALSE, "expect S_FALSE, hr = 0x%08lx\n", hr);

    test_get_clipboard();

    hr = OleSetClipboard(data2);
    ok(hr == S_OK, "failed to set clipboard to data2, hr = 0x%08lx\n", hr);
    hr = OleIsCurrentClipboard(data1);
    ok(hr == S_FALSE, "did not expect current clipboard to be data1, hr = 0x%08lx\n", hr);
    hr = OleIsCurrentClipboard(data2);
    ok(hr == S_OK, "expected current clipboard to be data2, hr = 0x%08lx\n", hr);
    hr = OleIsCurrentClipboard(NULL);
    ok(hr == S_FALSE, "expect S_FALSE, hr = 0x%08lx\n", hr);

    /* put a format directly onto the clipboard to show
       OleFlushClipboard doesn't empty the clipboard */
    hblob = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE|GMEM_ZEROINIT, 10);
    ptr = GlobalLock( hblob );
    ok( ptr && ptr != hblob, "got fixed block %p / %p\n", ptr, hblob );
    GlobalUnlock( hblob );
    ok( OpenClipboard(NULL), "OpenClipboard failed\n" );
    h = SetClipboardData(cf_onemore, hblob);
    ok(h == hblob, "got %p\n", h);
    h = GetClipboardData(cf_onemore);
    ok(h == hblob, "got %p / %p\n", h, hblob);
    ptr = GlobalLock( h );
    ok( ptr && ptr != h, "got fixed block %p / %p\n", ptr, h );
    GlobalUnlock( hblob );
    ok( CloseClipboard(), "CloseClipboard failed\n" );

    hr = OleFlushClipboard();
    ok(hr == S_OK, "failed to flush clipboard, hr = 0x%08lx\n", hr);
    hr = OleIsCurrentClipboard(data1);
    ok(hr == S_FALSE, "did not expect current clipboard to be data1, hr = 0x%08lx\n", hr);
    hr = OleIsCurrentClipboard(data2);
    ok(hr == S_FALSE, "did not expect current clipboard to be data2, hr = 0x%08lx\n", hr);
    hr = OleIsCurrentClipboard(NULL);
    ok(hr == S_FALSE, "expect S_FALSE, hr = 0x%08lx\n", hr);

    /* format should survive the flush */
    ok( OpenClipboard(NULL), "OpenClipboard failed\n" );
    h = GetClipboardData(cf_onemore);
    ok(h == hblob, "got %p\n", h);
    ptr = GlobalLock( h );
    ok( ptr && ptr != h, "got fixed block %p / %p\n", ptr, h );
    GlobalUnlock( hblob );
    ok( CloseClipboard(), "CloseClipboard failed\n" );

    test_cf_dataobject(NULL);

    hr = OleSetClipboard(NULL);
    ok(hr == S_OK, "Failed to clear clipboard, hr = 0x%08lx\n", hr);

    OpenClipboard(NULL);
    h = GetClipboardData(cf_onemore);
    ok(h == NULL, "got %p\n", h);
    CloseClipboard();

    trace("setting complex\n");
    hr = OleSetClipboard(data_cmpl);
    ok(hr == S_OK, "failed to set clipboard to complex data, hr = 0x%08lx\n", hr);
    test_complex_get_clipboard();
    test_cf_dataobject(data_cmpl);
    test_enum_fmtetc(data_cmpl);

    hr = OleSetClipboard(NULL);
    ok(hr == S_OK, "failed to clear clipboard, hr = 0x%08lx.\n", hr);

    test_no_cf_dataobject();
    test_enum_fmtetc(NULL);

    ref = IDataObject_Release(data1);
    ok(ref == 0, "expected data1 ref=0, got %ld\n", ref);
    ref = IDataObject_Release(data2);
    ok(ref == 0, "expected data2 ref=0, got %ld\n", ref);
    ref = IDataObject_Release(data_cmpl);
    ok(ref == 0, "expected data_cmpl ref=0, got %ld\n", ref);

    OleUninitialize();
}

static LPDATAOBJECT clip_data;
static HWND next_wnd;
static UINT wm_drawclipboard;

static LRESULT CALLBACK clipboard_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT ret;

    switch (msg)
    {
    case WM_DRAWCLIPBOARD:
        wm_drawclipboard++;
        if (clip_data)
        {
            /* if this is the WM_DRAWCLIPBOARD of a previous change, the data isn't current yet */
            /* this demonstrates an issue in Qt where it will free the data while it's being set */
            HRESULT hr = OleIsCurrentClipboard( clip_data );
            ok( hr == (wm_drawclipboard > 1) ? S_OK : S_FALSE,
                "OleIsCurrentClipboard returned %lx\n", hr );
        }
        break;
    case WM_CHANGECBCHAIN:
        if (next_wnd == (HWND)wp) next_wnd = (HWND)lp;
        else if (next_wnd) SendMessageA( next_wnd, msg, wp, lp );
        break;
    case WM_USER:
        ret = wm_drawclipboard;
        wm_drawclipboard = 0;
        return ret;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

static DWORD CALLBACK set_clipboard_thread(void *arg)
{
    OpenClipboard( GetDesktopWindow() );
    EmptyClipboard();
    SetClipboardData( CF_WAVE, 0 );
    CloseClipboard();
    return 0;
}

/* test that WM_DRAWCLIPBOARD can be delivered for a previous change during OleSetClipboard */
static void test_set_clipboard_DRAWCLIPBOARD(void)
{
    LPDATAOBJECT data;
    HRESULT hr;
    WNDCLASSA cls;
    HWND viewer;
    int ret;
    HANDLE thread;

    hr = DataObjectImpl_CreateText("data", &data);
    ok(hr == S_OK, "Failed to create data object: 0x%08lx\n", hr);

    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = clipboard_wnd_proc;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = "clipboard_test";
    RegisterClassA(&cls);

    viewer = CreateWindowA("clipboard_test", NULL, 0, 0, 0, 0, 0, NULL, 0, NULL, 0);
    ok(viewer != NULL, "CreateWindow failed: %ld\n", GetLastError());
    next_wnd = SetClipboardViewer( viewer );

    ret = SendMessageA( viewer, WM_USER, 0, 0 );
    ok( ret == 1, "%u WM_DRAWCLIPBOARD received\n", ret );

    hr = OleInitialize(NULL);
    ok(hr == S_OK, "OleInitialize failed with error 0x%08lx\n", hr);

    ret = SendMessageA( viewer, WM_USER, 0, 0 );
    ok( !ret, "%u WM_DRAWCLIPBOARD received\n", ret );

    thread = CreateThread(NULL, 0, set_clipboard_thread, NULL, 0, NULL);
    ok(thread != NULL, "CreateThread failed (%ld)\n", GetLastError());
    ret = WaitForSingleObject(thread, 5000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", ret);

    clip_data = data;
    hr = OleSetClipboard(data);
    ok(hr == S_OK, "failed to set clipboard to data, hr = 0x%08lx\n", hr);

    ret = SendMessageA( viewer, WM_USER, 0, 0 );
    ok( ret == 2, "%u WM_DRAWCLIPBOARD received\n", ret );

    clip_data = NULL;
    hr = OleFlushClipboard();
    ok(hr == S_OK, "failed to flush clipboard, hr = 0x%08lx\n", hr);
    ret = IDataObject_Release(data);
    ok(ret == 0, "got %d\n", ret);

    OleUninitialize();
    ChangeClipboardChain( viewer, next_wnd );
    DestroyWindow( viewer );
}

static inline ULONG count_refs(IDataObject *d)
{
    IDataObject_AddRef(d);
    return IDataObject_Release(d);
}

static void test_consumer_refs(void)
{
    HRESULT hr;
    IDataObject *src, *src2, *get1, *get2, *get3;
    ULONG refs, old_refs;
    FORMATETC fmt;
    STGMEDIUM med;

    InitFormatEtc(fmt, CF_TEXT, TYMED_HGLOBAL);

    OleInitialize(NULL);

    /* First show that each clipboard state results in
       a different data object */

    hr = DataObjectImpl_CreateText("data1", &src);
    ok(hr == S_OK, "got %08lx\n", hr);
    hr = DataObjectImpl_CreateText("data2", &src2);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = OleSetClipboard(src);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = OleGetClipboard(&get1);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = OleGetClipboard(&get2);
    ok(hr == S_OK, "got %08lx\n", hr);

    ok(get1 == get2, "data objects differ\n");
    refs = IDataObject_Release(get2);
    ok(refs == (get1 == get2 ? 1 : 0), "got %ld\n", refs);

    OleFlushClipboard();

    DataObjectImpl_GetData_calls = 0;
    hr = IDataObject_GetData(get1, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(DataObjectImpl_GetData_calls == 0, "GetData called\n");
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    hr = OleGetClipboard(&get2);
    ok(hr == S_OK, "got %08lx\n", hr);

    ok(get1 != get2, "data objects match\n");

    hr = OleSetClipboard(NULL);
    ok(hr == S_OK, "Failed to clear clipboard, hr %#lx.\n", hr);

    hr = OleGetClipboard(&get3);
    ok(hr == S_OK, "got %08lx\n", hr);

    ok(get1 != get3, "data objects match\n");
    ok(get2 != get3, "data objects match\n");

    IDataObject_Release(get3);
    IDataObject_Release(get2);
    IDataObject_Release(get1);

    /* Now call GetData before the flush and show that this
       takes a ref on our src data obj. */

    hr = OleSetClipboard(src);
    ok(hr == S_OK, "got %08lx\n", hr);

    old_refs = count_refs(src);

    hr = OleGetClipboard(&get1);
    ok(hr == S_OK, "got %08lx\n", hr);

    refs = count_refs(src);
    ok(refs == old_refs, "%ld %ld\n", refs, old_refs);

    DataObjectImpl_GetData_calls = 0;
    hr = IDataObject_GetData(get1, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(DataObjectImpl_GetData_calls == 1, "GetData not called\n");
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);
    refs = count_refs(src);
    ok(refs == old_refs + 1, "%ld %ld\n", refs, old_refs);

    OleFlushClipboard();

    DataObjectImpl_GetData_calls = 0;
    hr = IDataObject_GetData(get1, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(DataObjectImpl_GetData_calls == 1, "GetData not called\n");
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    refs = count_refs(src);
    ok(refs == 2, "%ld\n", refs);

    IDataObject_Release(get1);

    refs = count_refs(src);
    ok(refs == 1, "%ld\n", refs);

    /* Now set a second src object before the call to GetData
       and show that GetData calls that second src. */

    hr = OleSetClipboard(src);
    ok(hr == S_OK, "got %08lx\n", hr);

    old_refs = count_refs(src);

    hr = OleGetClipboard(&get1);
    ok(hr == S_OK, "got %08lx\n", hr);

    refs = count_refs(src);
    ok(refs == old_refs, "%ld %ld\n", refs, old_refs);

    hr = OleSetClipboard(src2);
    ok(hr == S_OK, "got %08lx\n", hr);

    old_refs = count_refs(src2);

    DataObjectImpl_GetData_calls = 0;
    hr = IDataObject_GetData(get1, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(DataObjectImpl_GetData_calls == 1, "GetData not called\n");
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    refs = count_refs(src);
    ok(refs == 1, "%ld\n", refs);
    refs = count_refs(src2);
    ok(refs == old_refs + 1, "%ld %ld\n", refs, old_refs);

    hr = OleSetClipboard(NULL);
    ok(hr == S_OK, "Failed to clear clipboard, hr %#lx.\n", hr);

    refs = count_refs(src2);
    ok(refs == 2, "%ld\n", refs);

    IDataObject_Release(get1);

    IDataObject_Release(src2);

    /* Show that OleUninitialize() doesn't release the
       dataobject's ref, and thus the object is leaked. */
    old_refs = count_refs(src);
    ok(old_refs == 1, "%ld\n", old_refs);

    hr = OleSetClipboard(src);
    ok(hr == S_OK, "Failed to clear clipboard, hr %#lx.\n", hr);
    refs = count_refs(src);
    ok(refs > old_refs, "%ld %ld\n", refs, old_refs);

    OleUninitialize();
    refs = count_refs(src);
    ok(refs == 2, "%ld\n", refs);

    OleInitialize(NULL);
    hr = OleSetClipboard(NULL);
    ok(hr == S_OK, "Failed to clear clipboard, hr %#lx.\n", hr);

    OleUninitialize();

    refs = count_refs(src);
    ok(refs == 2, "%ld\n", refs);

    IDataObject_Release(src);
}

static HGLOBAL create_storage(void)
{
    ILockBytes *ilb;
    IStorage *stg;
    HGLOBAL hg;
    HRESULT hr;

    hr = CreateILockBytesOnHGlobal(NULL, FALSE, &ilb);
    ok(hr == S_OK, "got %08lx\n", hr);
    hr = StgCreateDocfileOnILockBytes(ilb, STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &stg);
    ok(hr == S_OK, "got %08lx\n", hr);
    IStorage_Release(stg);
    hr = GetHGlobalFromILockBytes(ilb, &hg);
    ok(hr == S_OK, "got %08lx\n", hr);
    ILockBytes_Release(ilb);
    return hg;
}

static void test_flushed_getdata(void)
{
    HRESULT hr;
    IDataObject *src, *get;
    FORMATETC fmt;
    STGMEDIUM med;
    STATSTG stat;
    DEVMODEW dm;

    OleInitialize(NULL);

    hr = DataObjectImpl_CreateComplex(&src);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = OleSetClipboard(src);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = OleFlushClipboard();
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = OleGetClipboard(&get);
    ok(hr == S_OK, "got %08lx\n", hr);

    /* global format -> global & stream */

    InitFormatEtc(fmt, CF_TEXT, TYMED_HGLOBAL);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    InitFormatEtc(fmt, CF_TEXT, TYMED_ISTREAM);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTREAM, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    InitFormatEtc(fmt, CF_TEXT, TYMED_ISTORAGE);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    InitFormatEtc(fmt, CF_TEXT, 0xffff);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    /* stream format -> global & stream */

    InitFormatEtc(fmt, cf_stream, TYMED_ISTREAM);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTREAM, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    InitFormatEtc(fmt, cf_stream, TYMED_ISTORAGE);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    InitFormatEtc(fmt, cf_stream, TYMED_HGLOBAL);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    InitFormatEtc(fmt, cf_stream, 0xffff);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTREAM, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    /* storage format -> global, stream & storage */

    InitFormatEtc(fmt, cf_storage, TYMED_ISTORAGE);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTORAGE, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) {
        hr = IStorage_Stat(med.pstg, &stat, STATFLAG_NONAME);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(stat.grfMode == (STGM_SHARE_EXCLUSIVE | STGM_READWRITE), "got %08lx\n", stat.grfMode);
        ReleaseStgMedium(&med);
    }

    InitFormatEtc(fmt, cf_storage, TYMED_ISTREAM);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTREAM, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    InitFormatEtc(fmt, cf_storage, TYMED_HGLOBAL);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    InitFormatEtc(fmt, cf_storage, TYMED_HGLOBAL | TYMED_ISTREAM);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    InitFormatEtc(fmt, cf_storage, 0xffff);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTORAGE, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    /* complex format with target device */

    InitFormatEtc(fmt, cf_another, 0xffff);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    if (0)  /* Causes crashes on both Wine and Windows */
    {
        InitFormatEtc(fmt, cf_another, 0xffff);
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmDriverExtra = 0;
        lstrcpyW(dm.dmDeviceName, device_name);
        fmt.ptd = malloc(FIELD_OFFSET(DVTARGETDEVICE, tdData) + sizeof(device_name) + dm.dmSize + dm.dmDriverExtra);
        fmt.ptd->tdSize = FIELD_OFFSET(DVTARGETDEVICE, tdData) + sizeof(device_name) + dm.dmSize + dm.dmDriverExtra;
        fmt.ptd->tdDriverNameOffset = FIELD_OFFSET(DVTARGETDEVICE, tdData);
        fmt.ptd->tdDeviceNameOffset = 0;
        fmt.ptd->tdPortNameOffset   = 0;
        fmt.ptd->tdExtDevmodeOffset = fmt.ptd->tdDriverNameOffset + sizeof(device_name);
        lstrcpyW((WCHAR*)fmt.ptd->tdData, device_name);
        memcpy(fmt.ptd->tdData + sizeof(device_name), &dm, dm.dmSize + dm.dmDriverExtra);

        hr = IDataObject_GetData(get, &fmt, &med);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(med.tymed == TYMED_ISTORAGE, "got %lx\n", med.tymed);
        if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

        free(fmt.ptd);
    }

    /* CF_ENHMETAFILE format */
    InitFormatEtc(fmt, CF_ENHMETAFILE, TYMED_ENHMF);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    IDataObject_Release(get);
    IDataObject_Release(src);

    hr = DataObjectImpl_CreateFromHGlobal(create_storage(), &src);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = OleSetClipboard(src);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = OleGetClipboard(&get);
    ok(hr == S_OK, "got %08lx\n", hr);
    InitFormatEtc(fmt, CF_TEXT, TYMED_ISTORAGE);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTORAGE, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);
    IDataObject_Release(get);

    hr = OleFlushClipboard();
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = OleGetClipboard(&get);
    ok(hr == S_OK, "got %08lx\n", hr);

    InitFormatEtc(fmt, CF_TEXT, TYMED_ISTORAGE);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTORAGE, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    InitFormatEtc(fmt, CF_TEXT, 0xffff);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    IDataObject_Release(get);
    IDataObject_Release(src);

    OleUninitialize();
}

static HGLOBAL create_text(void)
{
    HGLOBAL h = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, 5);
    char *p = GlobalLock(h);
    strcpy(p, "test");
    GlobalUnlock(h);
    return h;
}

static HENHMETAFILE create_emf(void)
{
    const RECT rect = {0, 0, 100, 100};
    HDC hdc = CreateEnhMetaFileA(NULL, NULL, &rect, "HENHMETAFILE Ole Clipboard Test\0Test\0\0");
    ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &rect, "Test String", strlen("Test String"), NULL);
    return CloseEnhMetaFile(hdc);
}

static HDROP create_dropped_file(void)
{
    WCHAR path[] = L"C:\\testfile1\0";
    DROPFILES *dropfiles;
    DWORD offset;
    HDROP hdrop;

    offset = sizeof(DROPFILES);
    hdrop = GlobalAlloc(GHND, offset + sizeof(path));
    dropfiles = GlobalLock(hdrop);
    dropfiles->pFiles = offset;
    dropfiles->fWide = TRUE;
    memcpy((char *)dropfiles + offset, path, sizeof(path));
    GlobalUnlock(hdrop);

    return hdrop;
}

static void test_nonole_clipboard(void)
{
    HRESULT hr;
    BOOL r;
    IDataObject *get;
    IEnumFORMATETC *enum_fmt;
    FORMATETC fmt;
    HGLOBAL h, hblob, htext, hstorage;
    HENHMETAFILE emf;
    STGMEDIUM med;
    DWORD obj_type;
    HDROP hdrop;

    r = OpenClipboard(NULL);
    ok(r, "gle %ld\n", GetLastError());
    r = EmptyClipboard();
    ok(r, "gle %ld\n", GetLastError());
    r = CloseClipboard();
    ok(r, "gle %ld\n", GetLastError());

    OleInitialize(NULL);

    /* empty clipboard */
    hr = OleGetClipboard(&get);
    ok(hr == S_OK, "got %08lx\n", hr);
    hr = IDataObject_EnumFormatEtc(get, DATADIR_GET, &enum_fmt);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
    ok(hr == S_FALSE, "got %08lx\n", hr);
    IEnumFORMATETC_Release(enum_fmt);

    IDataObject_Release(get);

    /* set a user defined clipboard type */

    htext = create_text();
    hblob = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE|GMEM_ZEROINIT, 10);
    emf = create_emf();
    hstorage = create_storage();
    hdrop = create_dropped_file();

    r = OpenClipboard(NULL);
    ok(r, "gle %ld\n", GetLastError());
    h = SetClipboardData(CF_TEXT, htext);
    ok(h == htext, "got %p\n", h);
    h = SetClipboardData(cf_onemore, hblob);
    ok(h == hblob, "got %p\n", h);
    h = SetClipboardData(CF_ENHMETAFILE, emf);
    ok(h == emf, "got %p\n", h);
    h = SetClipboardData(cf_storage, hstorage);
    ok(h == hstorage, "got %p\n", h);
    h = SetClipboardData(CF_HDROP, hdrop);
    ok(h == hdrop, "got %p\n", h);
    r = CloseClipboard();
    ok(r, "gle %ld\n", GetLastError());

    hr = OleGetClipboard(&get);
    ok(hr == S_OK, "got %08lx\n", hr);
    hr = IDataObject_EnumFormatEtc(get, DATADIR_GET, &enum_fmt);
    ok(hr == S_OK, "got %08lx\n", hr);
    if (FAILED(hr))
    {
        skip("EnumFormatEtc failed, skipping tests.\n");
        return;
    }

    hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(fmt.cfFormat == CF_TEXT, "cf %04x\n", fmt.cfFormat);
    ok(fmt.ptd == NULL, "ptd %p\n", fmt.ptd);
    ok(fmt.dwAspect == DVASPECT_CONTENT, "aspect %lx\n", fmt.dwAspect);
    ok(fmt.lindex == -1, "lindex %ld\n", fmt.lindex);
    ok(fmt.tymed == (TYMED_ISTREAM | TYMED_HGLOBAL), "tymed %lx\n", fmt.tymed);

    hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(fmt.cfFormat == cf_onemore, "cf %04x\n", fmt.cfFormat);
    ok(fmt.ptd == NULL, "ptd %p\n", fmt.ptd);
    ok(fmt.dwAspect == DVASPECT_CONTENT, "aspect %lx\n", fmt.dwAspect);
    ok(fmt.lindex == -1, "lindex %ld\n", fmt.lindex);
    ok(fmt.tymed == (TYMED_ISTREAM | TYMED_HGLOBAL), "tymed %lx\n", fmt.tymed);

    hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(fmt.cfFormat == CF_ENHMETAFILE, "cf %04x\n", fmt.cfFormat);
    ok(fmt.ptd == NULL, "ptd %p\n", fmt.ptd);
    ok(fmt.dwAspect == DVASPECT_CONTENT, "aspect %lx\n", fmt.dwAspect);
    ok(fmt.lindex == -1, "lindex %ld\n", fmt.lindex);
    ok(fmt.tymed == TYMED_ENHMF, "tymed %lx\n", fmt.tymed);

    hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(fmt.cfFormat == cf_storage, "cf %04x\n", fmt.cfFormat);
    ok(fmt.ptd == NULL, "ptd %p\n", fmt.ptd);
    ok(fmt.dwAspect == DVASPECT_CONTENT, "aspect %lx\n", fmt.dwAspect);
    ok(fmt.lindex == -1, "lindex %ld\n", fmt.lindex);
    ok(fmt.tymed == (TYMED_ISTREAM | TYMED_HGLOBAL), "tymed %lx\n", fmt.tymed);

    hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(fmt.cfFormat == CF_HDROP, "cf %04x\n", fmt.cfFormat);
    ok(fmt.ptd == NULL, "ptd %p\n", fmt.ptd);
    ok(fmt.dwAspect == DVASPECT_CONTENT, "aspect %lx\n", fmt.dwAspect);
    ok(fmt.lindex == -1, "lindex %ld\n", fmt.lindex);
    ok(fmt.tymed == (TYMED_ISTREAM | TYMED_HGLOBAL), "tymed %lx\n", fmt.tymed);

    hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
    ok(hr == S_OK, "got %08lx\n", hr); /* User32 adds some synthesised formats */

    ok(fmt.cfFormat == CF_LOCALE, "cf %04x\n", fmt.cfFormat);
    ok(fmt.ptd == NULL, "ptd %p\n", fmt.ptd);
    ok(fmt.dwAspect == DVASPECT_CONTENT, "aspect %lx\n", fmt.dwAspect);
    ok(fmt.lindex == -1, "lindex %ld\n", fmt.lindex);
    todo_wine ok(fmt.tymed == (TYMED_ISTREAM | TYMED_HGLOBAL), "tymed %lx\n", fmt.tymed);

    hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);

    ok(fmt.cfFormat == CF_OEMTEXT, "cf %04x\n", fmt.cfFormat);
    ok(fmt.ptd == NULL, "ptd %p\n", fmt.ptd);
    ok(fmt.dwAspect == DVASPECT_CONTENT, "aspect %lx\n", fmt.dwAspect);
    ok(fmt.lindex == -1, "lindex %ld\n", fmt.lindex);
    ok(fmt.tymed == (TYMED_ISTREAM | TYMED_HGLOBAL), "tymed %lx\n", fmt.tymed);

    hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(fmt.cfFormat == CF_UNICODETEXT, "cf %04x\n", fmt.cfFormat);
    ok(fmt.ptd == NULL, "ptd %p\n", fmt.ptd);
    ok(fmt.dwAspect == DVASPECT_CONTENT, "aspect %lx\n", fmt.dwAspect);
    ok(fmt.lindex == -1, "lindex %ld\n", fmt.lindex);
    ok(fmt.tymed == (TYMED_ISTREAM | TYMED_HGLOBAL), "tymed %lx\n", fmt.tymed);

    hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(fmt.cfFormat == CF_METAFILEPICT, "cf %04x\n", fmt.cfFormat);
    ok(fmt.ptd == NULL, "ptd %p\n", fmt.ptd);
    ok(fmt.dwAspect == DVASPECT_CONTENT, "aspect %lx\n", fmt.dwAspect);
    ok(fmt.lindex == -1, "lindex %ld\n", fmt.lindex);
    ok(fmt.tymed == TYMED_MFPICT, "tymed %lx\n", fmt.tymed);

    hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
    ok(hr == S_FALSE, "got %08lx\n", hr);
    IEnumFORMATETC_Release(enum_fmt);

    InitFormatEtc(fmt, CF_ENHMETAFILE, TYMED_ENHMF);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    obj_type = GetObjectType(med.hEnhMetaFile);
    ok(obj_type == OBJ_ENHMETAFILE, "got %ld\n", obj_type);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    InitFormatEtc(fmt, cf_storage, TYMED_ISTORAGE);
    hr = IDataObject_GetData(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTORAGE, "got %lx\n", med.tymed);
    if(SUCCEEDED(hr)) ReleaseStgMedium(&med);

    IDataObject_Release(get);

    r = OpenClipboard(NULL);
    ok(r, "gle %ld\n", GetLastError());
    r = EmptyClipboard();
    ok(r, "gle %ld\n", GetLastError());
    r = CloseClipboard();
    ok(r, "gle %ld\n", GetLastError());

    OleUninitialize();
}

static void test_getdatahere(void)
{
    HRESULT hr;
    IDataObject *src, *get;
    FORMATETC fmt;
    STGMEDIUM med;

    OleInitialize(NULL);

    hr = DataObjectImpl_CreateComplex(&src);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = OleSetClipboard(src);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = OleGetClipboard(&get);
    ok(hr == S_OK, "got %08lx\n", hr);

    /* global format -> global & stream */

    DataObjectImpl_GetData_calls = 0;
    DataObjectImpl_GetDataHere_calls = 0;

    InitFormatEtc(fmt, CF_TEXT, TYMED_HGLOBAL);

    med.pUnkForRelease = NULL;
    med.tymed = TYMED_HGLOBAL;
    med.hGlobal = GlobalAlloc(GMEM_MOVEABLE, 100);
    hr = IDataObject_GetDataHere(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed);
    ReleaseStgMedium(&med);
    ok(DataObjectImpl_GetDataHere_calls == 1, "called %ld\n", DataObjectImpl_GetDataHere_calls);
    ok(DataObjectImpl_GetData_calls == 1, "called %ld\n", DataObjectImpl_GetData_calls);

    InitFormatEtc(fmt, CF_TEXT, 0);

    med.pUnkForRelease = NULL;
    med.tymed = TYMED_HGLOBAL;
    med.hGlobal = GlobalAlloc(GMEM_MOVEABLE, 100);
    hr = IDataObject_GetDataHere(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed);
    ReleaseStgMedium(&med);
    ok(DataObjectImpl_GetDataHere_calls == 2, "called %ld\n", DataObjectImpl_GetDataHere_calls);
    ok(DataObjectImpl_GetData_calls == 1, "called %ld\n", DataObjectImpl_GetData_calls);

    med.pUnkForRelease = NULL;
    med.tymed = TYMED_HGLOBAL;
    med.hGlobal = GlobalAlloc(GMEM_MOVEABLE, 1);
    hr = IDataObject_GetDataHere(get, &fmt, &med);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    ok(med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed);
    ReleaseStgMedium(&med);
    ok(DataObjectImpl_GetDataHere_calls == 3, "called %ld\n", DataObjectImpl_GetDataHere_calls);
    ok(DataObjectImpl_GetData_calls == 1, "called %ld\n", DataObjectImpl_GetData_calls);

    med.pUnkForRelease = NULL;
    med.tymed = TYMED_ISTREAM;
    CreateStreamOnHGlobal(NULL, TRUE, &med.pstm);
    hr = IDataObject_GetDataHere(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTREAM, "got %lx\n", med.tymed);
    ReleaseStgMedium(&med);
    ok(DataObjectImpl_GetDataHere_calls == 4, "called %ld\n", DataObjectImpl_GetDataHere_calls);
    ok(DataObjectImpl_GetData_calls == 1, "called %ld\n", DataObjectImpl_GetData_calls);

    med.pUnkForRelease = NULL;
    med.tymed = TYMED_ISTORAGE;
    StgCreateDocfile(NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &med.pstg);
    hr = IDataObject_GetDataHere(get, &fmt, &med);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTORAGE, "got %lx\n", med.tymed);
    ReleaseStgMedium(&med);
    ok(DataObjectImpl_GetDataHere_calls == 5, "called %ld\n", DataObjectImpl_GetDataHere_calls);
    ok(DataObjectImpl_GetData_calls == 1, "called %ld\n", DataObjectImpl_GetData_calls);

    InitFormatEtc(fmt, cf_stream, 0);

    med.pUnkForRelease = NULL;
    med.tymed = TYMED_HGLOBAL;
    med.hGlobal = GlobalAlloc(GMEM_MOVEABLE, 100);
    hr = IDataObject_GetDataHere(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed);
    ReleaseStgMedium(&med);
    ok(DataObjectImpl_GetDataHere_calls == 7, "called %ld\n", DataObjectImpl_GetDataHere_calls);
    ok(DataObjectImpl_GetData_calls == 2, "called %ld\n", DataObjectImpl_GetData_calls);

    med.pUnkForRelease = NULL;
    med.tymed = TYMED_ISTREAM;
    CreateStreamOnHGlobal(NULL, TRUE, &med.pstm);
    hr = IDataObject_GetDataHere(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTREAM, "got %lx\n", med.tymed);
    ReleaseStgMedium(&med);
    ok(DataObjectImpl_GetDataHere_calls == 8, "called %ld\n", DataObjectImpl_GetDataHere_calls);
    ok(DataObjectImpl_GetData_calls == 2, "called %ld\n", DataObjectImpl_GetData_calls);

    med.pUnkForRelease = NULL;
    med.tymed = TYMED_ISTORAGE;
    StgCreateDocfile(NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &med.pstg);
    hr = IDataObject_GetDataHere(get, &fmt, &med);
    ok(hr == E_FAIL, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTORAGE, "got %lx\n", med.tymed);
    ReleaseStgMedium(&med);
    ok(DataObjectImpl_GetDataHere_calls == 9, "called %ld\n", DataObjectImpl_GetDataHere_calls);
    ok(DataObjectImpl_GetData_calls == 2, "called %ld\n", DataObjectImpl_GetData_calls);

    InitFormatEtc(fmt, cf_storage, 0);

    med.pUnkForRelease = NULL;
    med.tymed = TYMED_HGLOBAL;
    med.hGlobal = GlobalAlloc(GMEM_MOVEABLE, 3000);
    hr = IDataObject_GetDataHere(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed);
    ReleaseStgMedium(&med);
    ok(DataObjectImpl_GetDataHere_calls == 11, "called %ld\n", DataObjectImpl_GetDataHere_calls);
    ok(DataObjectImpl_GetData_calls == 3, "called %ld\n", DataObjectImpl_GetData_calls);

    med.pUnkForRelease = NULL;
    med.tymed = TYMED_ISTREAM;
    CreateStreamOnHGlobal(NULL, TRUE, &med.pstm);
    hr = IDataObject_GetDataHere(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTREAM, "got %lx\n", med.tymed);
    ReleaseStgMedium(&med);
    ok(DataObjectImpl_GetDataHere_calls == 12, "called %ld\n", DataObjectImpl_GetDataHere_calls);
    ok(DataObjectImpl_GetData_calls == 3, "called %ld\n", DataObjectImpl_GetData_calls);

    med.pUnkForRelease = NULL;
    med.tymed = TYMED_ISTORAGE;
    StgCreateDocfile(NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &med.pstg);
    hr = IDataObject_GetDataHere(get, &fmt, &med);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(med.tymed == TYMED_ISTORAGE, "got %lx\n", med.tymed);
    ReleaseStgMedium(&med);
    ok(DataObjectImpl_GetDataHere_calls == 13, "called %ld\n", DataObjectImpl_GetDataHere_calls);
    ok(DataObjectImpl_GetData_calls == 3, "called %ld\n", DataObjectImpl_GetData_calls);


    IDataObject_Release(get);
    IDataObject_Release(src);

    OleUninitialize();

}

static DWORD CALLBACK test_data_obj(void *arg)
{
    IDataObject *data_obj = arg;

    IDataObject_Release(data_obj);
    return 0;
}

static void test_multithreaded_clipboard(void)
{
    IDataObject *data_obj;
    HANDLE thread;
    HRESULT hr;
    DWORD ret;

    OleInitialize(NULL);

    hr = OleGetClipboard(&data_obj);
    ok(hr == S_OK, "OleGetClipboard returned %lx\n", hr);

    thread = CreateThread(NULL, 0, test_data_obj, data_obj, 0, NULL);
    ok(thread != NULL, "CreateThread failed (%ld)\n", GetLastError());
    ret = WaitForSingleObject(thread, 5000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %lx\n", ret);

    hr = OleGetClipboard(&data_obj);
    ok(hr == S_OK, "OleGetClipboard returned %lx\n", hr);
    IDataObject_Release(data_obj);

    OleUninitialize();
}

static void test_get_clipboard_locked(void)
{
    HRESULT hr;
    IDataObject *pDObj;

    OleInitialize(NULL);

    pDObj = (IDataObject *)0xdeadbeef;
    /* lock clipboard */
    OpenClipboard(NULL);
    hr = OleGetClipboard(&pDObj);
    todo_wine ok(hr == CLIPBRD_E_CANT_OPEN, "OleGetClipboard() got 0x%08lx instead of 0x%08lx\n", hr, CLIPBRD_E_CANT_OPEN);
    todo_wine ok(pDObj == NULL, "OleGetClipboard() got 0x%p instead of NULL\n",pDObj);
    if (pDObj) IDataObject_Release(pDObj);
    CloseClipboard();

    OleUninitialize();
}

START_TEST(clipboard)
{
    test_get_clipboard_uninitialized();
    test_set_clipboard();
    test_set_clipboard_DRAWCLIPBOARD();
    test_consumer_refs();
    test_flushed_getdata();
    test_nonole_clipboard();
    test_getdatahere();
    test_multithreaded_clipboard();
    test_get_clipboard_locked();
}
