/*
 * Object Linking and Embedding Tests
 *
 * Copyright 2005 Robert Shearman
 * Copyright 2017 Dmitry Timoshkov
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
#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "objbase.h"
#include "shlguid.h"

#include "wine/test.h"

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error %#08lx\n", hr)

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(Storage_Stat);
DEFINE_EXPECT(Storage_OpenStream_CompObj);
DEFINE_EXPECT(Storage_OpenStream_OlePres);
DEFINE_EXPECT(Storage_SetClass);
DEFINE_EXPECT(Storage_CreateStream_CompObj);
DEFINE_EXPECT(Storage_CreateStream_OlePres);
DEFINE_EXPECT(Storage_OpenStream_Ole);
DEFINE_EXPECT(Storage_DestroyElement);

static const CLSID *Storage_SetClass_CLSID;
static int Storage_DestroyElement_limit;

static IPersistStorage OleObjectPersistStg;
static IOleCache *cache;
static IRunnableObject *runnable;

static const CLSID CLSID_WineTestOld =
{ /* 9474ba1a-258b-490b-bc13-516e9239acd0 */
    0x9474ba1a,
    0x258b,
    0x490b,
    {0xbc, 0x13, 0x51, 0x6e, 0x92, 0x39, 0xac, 0xd0}
};

static const CLSID CLSID_WineTest =
{ /* 9474ba1a-258b-490b-bc13-516e9239ace0 */
    0x9474ba1a,
    0x258b,
    0x490b,
    {0xbc, 0x13, 0x51, 0x6e, 0x92, 0x39, 0xac, 0xe0}
};

static const IID IID_WineTest =
{ /* 9474ba1a-258b-490b-bc13-516e9239ace1 */
    0x9474ba1a,
    0x258b,
    0x490b,
    {0xbc, 0x13, 0x51, 0x6e, 0x92, 0x39, 0xac, 0xe1}
};

#define TEST_TODO     0x2

struct expected_method
{
    const char *method;
    unsigned int flags;
    FORMATETC fmt;
};

static const struct expected_method *expected_method_list;
static FORMATETC *g_expected_fetc = NULL;

static BOOL g_showRunnable = TRUE;
static BOOL g_isRunning = TRUE;
static HRESULT g_GetMiscStatusFailsWith = S_OK;
static HRESULT g_QIFailsWith;

static UINT cf_test_1, cf_test_2, cf_test_3;

static FORMATETC *data_object_format;
static const BYTE *data_object_dib;

/****************************************************************************
 * PresentationDataHeader
 *
 * This structure represents the header of the \002OlePresXXX stream in
 * the OLE object storage.
 */
typedef struct PresentationDataHeader
{
    /* clipformat:
     *  - standard clipformat:
     *  DWORD length = 0xffffffff;
     *  DWORD cfFormat;
     *  - or custom clipformat:
     *  DWORD length;
     *  CHAR format_name[length]; (null-terminated)
     */
    DWORD tdSize; /* This is actually a truncated DVTARGETDEVICE, if tdSize > sizeof(DWORD)
                     then there are tdSize - sizeof(DWORD) more bytes before dvAspect */
    DVASPECT dvAspect;
    DWORD lindex;
    DWORD advf;
    DWORD unknown7; /* 0 */
    DWORD dwObjectExtentX;
    DWORD dwObjectExtentY;
    DWORD dwSize;
} PresentationDataHeader;

#ifdef __REACTOS__
static inline void check_expected_method_fmt(const char *method_name, const FORMATETC *fmt)
#else
static void inline check_expected_method_fmt(const char *method_name, const FORMATETC *fmt)
#endif
{
    if (winetest_debug > 1) trace("%s\n", method_name);
    ok(expected_method_list->method != NULL, "Extra method %s called\n", method_name);
    if (expected_method_list->method)
    {
        todo_wine_if (expected_method_list->flags & TEST_TODO)
        {
            ok(!strcmp(expected_method_list->method, method_name),
               "Expected %s to be called instead of %s\n",
               expected_method_list->method, method_name);
            if (fmt)
            {
                ok(fmt->cfFormat == expected_method_list->fmt.cfFormat, "got cf %04x vs %04x\n",
                   fmt->cfFormat, expected_method_list->fmt.cfFormat );
                ok(fmt->dwAspect == expected_method_list->fmt.dwAspect, "got aspect %ld vs %ld\n",
                   fmt->dwAspect, expected_method_list->fmt.dwAspect );
                ok(fmt->lindex == expected_method_list->fmt.lindex, "got lindex %ld vs %ld\n",
                   fmt->lindex, expected_method_list->fmt.lindex );
                ok(fmt->tymed == expected_method_list->fmt.tymed, "got tymed %ld vs %ld\n",
                   fmt->tymed, expected_method_list->fmt.tymed );
            }
        }
        expected_method_list++;
    }
}

#define CHECK_EXPECTED_METHOD(method_name) check_expected_method_fmt(method_name, NULL)
#define CHECK_EXPECTED_METHOD_FMT(method_name, fmt) check_expected_method_fmt(method_name, fmt)

#define CHECK_NO_EXTRA_METHODS() \
    do { \
        ok(!expected_method_list->method, "Method sequence starting from %s not called\n", expected_method_list->method); \
    } while (0)

static const BYTE dib_white[] =
{
    0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc8, 0x00, 0x00, 0x00, 0x90, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static const BYTE dib_black[] =
{
    0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc8, 0x00, 0x00, 0x00, 0x90, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static void create_dib( STGMEDIUM *med )
{
    void *ptr;

    med->tymed = TYMED_HGLOBAL;
    med->hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeof(dib_white));
    ptr = GlobalLock( med->hGlobal );
    memcpy(ptr, dib_white, sizeof(dib_white));
    GlobalUnlock( med->hGlobal );
    med->pUnkForRelease = NULL;
}

static void create_bitmap( STGMEDIUM *med )
{
    med->tymed = TYMED_GDI;
    med->hBitmap = CreateBitmap( 1, 1, 1, 1, NULL );
    med->pUnkForRelease = NULL;
}

static void create_emf(STGMEDIUM *med)
{
    HDC hdc = CreateEnhMetaFileW(NULL, NULL, NULL, NULL);

    Rectangle(hdc, 0, 0, 150, 300);
    med->tymed = TYMED_ENHMF;
    med->hEnhMetaFile = CloseEnhMetaFile(hdc);
    med->pUnkForRelease = NULL;
}

static void create_mfpict(STGMEDIUM *med)
{
    METAFILEPICT *mf;
    HDC hdc = CreateMetaFileW(NULL);

    Rectangle(hdc, 0, 0, 100, 200);

    med->tymed = TYMED_MFPICT;
    med->hMetaFilePict = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAFILEPICT));
    mf = GlobalLock(med->hMetaFilePict);
    mf->mm = MM_ANISOTROPIC;
    mf->xExt = 100;
    mf->yExt = 200;
    mf->hMF = CloseMetaFile(hdc);
    GlobalUnlock(med->hMetaFilePict);
    med->pUnkForRelease = NULL;
}

static void create_text(STGMEDIUM *med)
{
    HGLOBAL handle;
    char *p;

    handle = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, 5);
    p = GlobalLock(handle);
    strcpy(p, "test");
    GlobalUnlock(handle);

    med->tymed = TYMED_HGLOBAL;
    med->hGlobal = handle;
    med->pUnkForRelease = NULL;
}

static LONG ole_object_refcount;

static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    if (winetest_debug > 1) trace("IOleObject::QueryInterface(%s)\n", debugstr_guid(riid));

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleObject))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IPersistStorage))
        *ppv = &OleObjectPersistStg;
    else if (IsEqualIID(riid, &IID_IOleCache))
        *ppv = cache;
    else if (IsEqualIID(riid, &IID_IRunnableObject) && g_showRunnable)
        *ppv = runnable;
    else if (IsEqualIID(riid, &IID_WineTest))
        return g_QIFailsWith;

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    return InterlockedIncrement(&ole_object_refcount);
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    return InterlockedDecrement(&ole_object_refcount);
}

static HRESULT WINAPI OleObject_SetClientSite
    (
        IOleObject *iface,
        IOleClientSite *pClientSite
    )
{
    CHECK_EXPECTED_METHOD("OleObject_SetClientSite");
    return S_OK;
}

static HRESULT WINAPI OleObject_GetClientSite
    (
        IOleObject *iface,
        IOleClientSite **ppClientSite
    )
{
    CHECK_EXPECTED_METHOD("OleObject_GetClientSite");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetHostNames
    (
        IOleObject *iface,
        LPCOLESTR szContainerApp,
        LPCOLESTR szContainerObj
    )
{
    CHECK_EXPECTED_METHOD("OleObject_SetHostNames");
    return S_OK;
}

static HRESULT WINAPI OleObject_Close
    (
        IOleObject *iface,
        DWORD dwSaveOption
    )
{
    CHECK_EXPECTED_METHOD("OleObject_Close");
    return S_OK;
}

static HRESULT WINAPI OleObject_SetMoniker
    (
        IOleObject *iface,
        DWORD dwWhichMoniker,
        IMoniker *pmk
    )
{
    CHECK_EXPECTED_METHOD("OleObject_SetMoniker");
    return S_OK;
}

static HRESULT WINAPI OleObject_GetMoniker
    (
        IOleObject *iface,
        DWORD dwAssign,
        DWORD dwWhichMoniker,
        IMoniker **ppmk
    )
{
    CHECK_EXPECTED_METHOD("OleObject_GetMoniker");
    return S_OK;
}

static HRESULT WINAPI OleObject_InitFromData
    (
        IOleObject *iface,
        IDataObject *pDataObject,
        BOOL fCreation,
        DWORD dwReserved
    )
{
    CHECK_EXPECTED_METHOD("OleObject_InitFromData");
    return S_OK;
}

static HRESULT WINAPI OleObject_GetClipboardData
    (
        IOleObject *iface,
        DWORD dwReserved,
        IDataObject **ppDataObject
    )
{
    CHECK_EXPECTED_METHOD("OleObject_GetClipboardData");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb
    (
        IOleObject *iface,
        LONG iVerb,
        LPMSG lpmsg,
        IOleClientSite *pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect
    )
{
    CHECK_EXPECTED_METHOD("OleObject_DoVerb");
    return S_OK;
}

static HRESULT WINAPI OleObject_EnumVerbs
    (
        IOleObject *iface,
        IEnumOLEVERB **ppEnumOleVerb
    )
{
    CHECK_EXPECTED_METHOD("OleObject_EnumVerbs");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Update
    (
        IOleObject *iface
    )
{
    CHECK_EXPECTED_METHOD("OleObject_Update");
    return S_OK;
}

static HRESULT WINAPI OleObject_IsUpToDate
    (
        IOleObject *iface
    )
{
    CHECK_EXPECTED_METHOD("OleObject_IsUpToDate");
    return S_OK;
}

static HRESULT WINAPI OleObject_GetUserClassID
(
    IOleObject *iface,
    CLSID *pClsid
)
{
    CHECK_EXPECTED_METHOD("OleObject_GetUserClassID");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserType
(
    IOleObject *iface,
    DWORD dwFormOfType,
    LPOLESTR *pszUserType
)
{
    CHECK_EXPECTED_METHOD("OleObject_GetUserType");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetExtent
(
    IOleObject *iface,
    DWORD dwDrawAspect,
    SIZEL *psizel
)
{
    CHECK_EXPECTED_METHOD("OleObject_SetExtent");
    return S_OK;
}

static HRESULT WINAPI OleObject_GetExtent
(
    IOleObject *iface,
    DWORD dwDrawAspect,
    SIZEL *psizel
)
{
    CHECK_EXPECTED_METHOD("OleObject_GetExtent");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Advise
(
    IOleObject *iface,
    IAdviseSink *pAdvSink,
    DWORD *pdwConnection
)
{
    CHECK_EXPECTED_METHOD("OleObject_Advise");
    return S_OK;
}

static HRESULT WINAPI OleObject_Unadvise
(
    IOleObject *iface,
    DWORD dwConnection
)
{
    CHECK_EXPECTED_METHOD("OleObject_Unadvise");
    return S_OK;
}

static HRESULT WINAPI OleObject_EnumAdvise
(
    IOleObject *iface,
    IEnumSTATDATA **ppenumAdvise
)
{
    CHECK_EXPECTED_METHOD("OleObject_EnumAdvise");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMiscStatus
(
    IOleObject *iface,
    DWORD aspect,
    DWORD *pdwStatus
)
{
    CHECK_EXPECTED_METHOD("OleObject_GetMiscStatus");

    ok(aspect == DVASPECT_CONTENT, "got aspect %ld\n", aspect);

    if (g_GetMiscStatusFailsWith == S_OK)
    {
        *pdwStatus = OLEMISC_RECOMPOSEONRESIZE;
        return S_OK;
    }
    else
    {
        *pdwStatus = 0x1234;
        return g_GetMiscStatusFailsWith;
    }
}

static HRESULT WINAPI OleObject_SetColorScheme
(
    IOleObject *iface,
    LOGPALETTE *pLogpal
)
{
    CHECK_EXPECTED_METHOD("OleObject_SetColorScheme");
    return E_NOTIMPL;
}

static const IOleObjectVtbl OleObjectVtbl =
{
    OleObject_QueryInterface,
    OleObject_AddRef,
    OleObject_Release,
    OleObject_SetClientSite,
    OleObject_GetClientSite,
    OleObject_SetHostNames,
    OleObject_Close,
    OleObject_SetMoniker,
    OleObject_GetMoniker,
    OleObject_InitFromData,
    OleObject_GetClipboardData,
    OleObject_DoVerb,
    OleObject_EnumVerbs,
    OleObject_Update,
    OleObject_IsUpToDate,
    OleObject_GetUserClassID,
    OleObject_GetUserType,
    OleObject_SetExtent,
    OleObject_GetExtent,
    OleObject_Advise,
    OleObject_Unadvise,
    OleObject_EnumAdvise,
    OleObject_GetMiscStatus,
    OleObject_SetColorScheme
};

static IOleObject OleObject = { &OleObjectVtbl };

static HRESULT WINAPI OleObjectPersistStg_QueryInterface(IPersistStorage *iface, REFIID riid, void **ppv)
{
    return IOleObject_QueryInterface(&OleObject, riid, ppv);
}

static ULONG WINAPI OleObjectPersistStg_AddRef(IPersistStorage *iface)
{
    return IOleObject_AddRef(&OleObject);
}

static ULONG WINAPI OleObjectPersistStg_Release(IPersistStorage *iface)
{
    return IOleObject_Release(&OleObject);
}

static HRESULT WINAPI OleObjectPersistStg_GetClassId(IPersistStorage *iface, CLSID *clsid)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_GetClassId");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObjectPersistStg_IsDirty
(
    IPersistStorage *iface
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_IsDirty");
    return S_OK;
}

static HRESULT WINAPI OleObjectPersistStg_InitNew
(
    IPersistStorage *iface,
    IStorage *pStg
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_InitNew");
    return S_OK;
}

static HRESULT WINAPI OleObjectPersistStg_Load
(
    IPersistStorage *iface,
    IStorage *pStg
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_Load");
    return S_OK;
}

static HRESULT WINAPI OleObjectPersistStg_Save
(
    IPersistStorage *iface,
    IStorage *pStgSave,
    BOOL fSameAsLoad
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_Save");
    return S_OK;
}

static HRESULT WINAPI OleObjectPersistStg_SaveCompleted
(
    IPersistStorage *iface,
    IStorage *pStgNew
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_SaveCompleted");
    return S_OK;
}

static HRESULT WINAPI OleObjectPersistStg_HandsOffStorage
(
    IPersistStorage *iface
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_HandsOffStorage");
    return S_OK;
}

static const IPersistStorageVtbl OleObjectPersistStgVtbl =
{
    OleObjectPersistStg_QueryInterface,
    OleObjectPersistStg_AddRef,
    OleObjectPersistStg_Release,
    OleObjectPersistStg_GetClassId,
    OleObjectPersistStg_IsDirty,
    OleObjectPersistStg_InitNew,
    OleObjectPersistStg_Load,
    OleObjectPersistStg_Save,
    OleObjectPersistStg_SaveCompleted,
    OleObjectPersistStg_HandsOffStorage
};

static IPersistStorage OleObjectPersistStg = { &OleObjectPersistStgVtbl };

static HRESULT WINAPI OleObjectCache_QueryInterface(IOleCache *iface, REFIID riid, void **ppv)
{
    return IOleObject_QueryInterface(&OleObject, riid, ppv);
}

static ULONG WINAPI OleObjectCache_AddRef(IOleCache *iface)
{
    return IOleObject_AddRef(&OleObject);
}

static ULONG WINAPI OleObjectCache_Release(IOleCache *iface)
{
    return IOleObject_Release(&OleObject);
}

static HRESULT WINAPI OleObjectCache_Cache
(
    IOleCache *iface,
    FORMATETC *pformatetc,
    DWORD advf,
    DWORD *pdwConnection
)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_Cache");
    if (g_expected_fetc) {
        ok(pformatetc != NULL, "pformatetc should not be NULL\n");
        if (pformatetc) {
            ok(pformatetc->cfFormat == g_expected_fetc->cfFormat,
                    "cfFormat: %x\n", pformatetc->cfFormat);
            ok((pformatetc->ptd != NULL) == (g_expected_fetc->ptd != NULL),
                    "ptd: %p\n", pformatetc->ptd);
            ok(pformatetc->dwAspect == g_expected_fetc->dwAspect,
                    "dwAspect: %lx\n", pformatetc->dwAspect);
            ok(pformatetc->lindex == g_expected_fetc->lindex,
                    "lindex: %lx\n", pformatetc->lindex);
            ok(pformatetc->tymed == g_expected_fetc->tymed,
                    "tymed: %lx\n", pformatetc->tymed);
        }
    } else
        ok(pformatetc == NULL, "pformatetc should be NULL\n");
    return S_OK;
}

static HRESULT WINAPI OleObjectCache_Uncache
(
    IOleCache *iface,
    DWORD dwConnection
)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_Uncache");
    return S_OK;
}

static HRESULT WINAPI OleObjectCache_EnumCache
(
    IOleCache *iface,
    IEnumSTATDATA **ppenumSTATDATA
)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_EnumCache");
    return S_OK;
}


static HRESULT WINAPI OleObjectCache_InitCache
(
    IOleCache *iface,
    IDataObject *pDataObject
)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_InitCache");
    return S_OK;
}


static HRESULT WINAPI OleObjectCache_SetData
(
    IOleCache *iface,
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium,
    BOOL fRelease
)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_SetData");
    return S_OK;
}


static const IOleCacheVtbl OleObjectCacheVtbl =
{
    OleObjectCache_QueryInterface,
    OleObjectCache_AddRef,
    OleObjectCache_Release,
    OleObjectCache_Cache,
    OleObjectCache_Uncache,
    OleObjectCache_EnumCache,
    OleObjectCache_InitCache,
    OleObjectCache_SetData
};

static IOleCache OleObjectCache = { &OleObjectCacheVtbl };

static HRESULT WINAPI OleObjectCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI OleObjectCF_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI OleObjectCF_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI OleObjectCF_CreateInstance(IClassFactory *iface, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return IOleObject_QueryInterface(&OleObject, riid, ppv);
}

static HRESULT WINAPI OleObjectCF_LockServer(IClassFactory *iface, BOOL lock)
{
    return S_OK;
}

static const IClassFactoryVtbl OleObjectCFVtbl =
{
    OleObjectCF_QueryInterface,
    OleObjectCF_AddRef,
    OleObjectCF_Release,
    OleObjectCF_CreateInstance,
    OleObjectCF_LockServer
};

static IClassFactory OleObjectCF = { &OleObjectCFVtbl };

static HRESULT WINAPI OleObjectRunnable_QueryInterface(IRunnableObject *iface, REFIID riid, void **ppv)
{
    return IOleObject_QueryInterface(&OleObject, riid, ppv);
}

static ULONG WINAPI OleObjectRunnable_AddRef(IRunnableObject *iface)
{
    return IOleObject_AddRef(&OleObject);
}

static ULONG WINAPI OleObjectRunnable_Release(IRunnableObject *iface)
{
    return IOleObject_Release(&OleObject);
}

static HRESULT WINAPI OleObjectRunnable_GetRunningClass(
    IRunnableObject *iface,
    LPCLSID lpClsid)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_GetRunningClass");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObjectRunnable_Run(
    IRunnableObject *iface,
    LPBINDCTX pbc)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_Run");
    return S_OK;
}

static BOOL WINAPI OleObjectRunnable_IsRunning(IRunnableObject *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_IsRunning");
    return g_isRunning;
}

static HRESULT WINAPI OleObjectRunnable_LockRunning(
    IRunnableObject *iface,
    BOOL fLock,
    BOOL fLastUnlockCloses)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_LockRunning");
    return S_OK;
}

static HRESULT WINAPI OleObjectRunnable_SetContainedObject(
    IRunnableObject *iface,
    BOOL fContained)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_SetContainedObject");
    return S_OK;
}

static const IRunnableObjectVtbl OleObjectRunnableVtbl =
{
    OleObjectRunnable_QueryInterface,
    OleObjectRunnable_AddRef,
    OleObjectRunnable_Release,
    OleObjectRunnable_GetRunningClass,
    OleObjectRunnable_Run,
    OleObjectRunnable_IsRunning,
    OleObjectRunnable_LockRunning,
    OleObjectRunnable_SetContainedObject
};

static IRunnableObject OleObjectRunnable = { &OleObjectRunnableVtbl };

static const CLSID CLSID_Equation3 = {0x0002CE02, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };

static HRESULT WINAPI viewobject_QueryInterface(IViewObject *iface, REFIID riid, void **obj)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IViewObject))
    {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI viewobject_AddRef(IViewObject *iface)
{
    return 2;
}

static ULONG WINAPI viewobject_Release(IViewObject *iface)
{
    return 1;
}

static HRESULT WINAPI viewobject_Draw(IViewObject *iface, DWORD aspect, LONG index,
    void *paspect, DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw,
    LPCRECTL bounds, LPCRECTL wbounds, BOOL (STDMETHODCALLTYPE *pfnContinue)(ULONG_PTR dwContinue),
    ULONG_PTR dwContinue)
{
    ok(index == -1, "index=%ld\n", index);
    return S_OK;
}

static HRESULT WINAPI viewobject_GetColorSet(IViewObject *iface, DWORD draw_aspect, LONG index,
    void *aspect, DVTARGETDEVICE *ptd, HDC hicTargetDev, LOGPALETTE **colorset)
{
    ok(0, "unexpected call GetColorSet\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI viewobject_Freeze(IViewObject *iface, DWORD draw_aspect, LONG index,
    void *aspect, DWORD *freeze)
{
    ok(0, "unexpected call Freeze\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI viewobject_Unfreeze(IViewObject *iface, DWORD freeze)
{
    ok(0, "unexpected call Unfreeze\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI viewobject_SetAdvise(IViewObject *iface, DWORD aspects, DWORD advf, IAdviseSink *sink)
{
    ok(0, "unexpected call SetAdvise\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI viewobject_GetAdvise(IViewObject *iface, DWORD *aspects, DWORD *advf,
    IAdviseSink **sink)
{
    ok(0, "unexpected call GetAdvise\n");
    return E_NOTIMPL;
}

static const struct IViewObjectVtbl viewobjectvtbl = {
    viewobject_QueryInterface,
    viewobject_AddRef,
    viewobject_Release,
    viewobject_Draw,
    viewobject_GetColorSet,
    viewobject_Freeze,
    viewobject_Unfreeze,
    viewobject_SetAdvise,
    viewobject_GetAdvise
};

static IViewObject viewobject = { &viewobjectvtbl };

static void test_OleCreate(IStorage *pStorage)
{
    HRESULT hr;
    IOleObject *pObject;
    FORMATETC formatetc;
    static const struct expected_method methods_olerender_none[] =
    {
        { "OleObjectPersistStg_InitNew", 0 },
        { NULL, 0 }
    };
    static const struct expected_method methods_olerender_draw[] =
    {
        { "OleObjectPersistStg_InitNew", 0 },
        { "OleObjectRunnable_Run", 0 },
        { "OleObjectCache_Cache", 0 },
        { NULL, 0 }
    };
    static const struct expected_method methods_olerender_draw_with_site[] =
    {
        { "OleObject_GetMiscStatus", 0 },
        { "OleObjectPersistStg_InitNew", 0 },
        { "OleObject_SetClientSite", 0 },
        { "OleObjectRunnable_Run", 0 },
        { "OleObjectCache_Cache", 0 },
        { NULL, 0 }
    };
    static const struct expected_method methods_olerender_format[] =
    {
        { "OleObject_GetMiscStatus", 0 },
        { "OleObjectPersistStg_InitNew", 0 },
        { "OleObject_SetClientSite", 0 },
        { "OleObjectRunnable_Run", 0 },
        { "OleObjectCache_Cache", 0 },
        { NULL, 0 }
    };
    static const struct expected_method methods_olerender_asis[] =
    {
        { "OleObjectPersistStg_InitNew", 0 },
        { NULL, 0 }
    };
    static const struct expected_method methods_olerender_draw_no_runnable[] =
    {
        { "OleObjectPersistStg_InitNew", 0 },
        { "OleObjectCache_Cache", 0 },
        { NULL, 0 },
    };
    static const struct expected_method methods_olerender_draw_no_cache[] =
    {
        { "OleObjectPersistStg_InitNew", 0 },
        { "OleObjectRunnable_Run", 0 },
        { NULL, 0 }
    };

    g_expected_fetc = &formatetc;
    formatetc.cfFormat = 0;
    formatetc.ptd = NULL;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = -1;
    formatetc.tymed = TYMED_NULL;
    runnable = &OleObjectRunnable;
    cache = &OleObjectCache;
    expected_method_list = methods_olerender_none;
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_NONE, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();
    ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);

    expected_method_list = methods_olerender_draw;
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_DRAW, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();
    ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);

    expected_method_list = methods_olerender_draw_with_site;
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_DRAW, NULL, (IOleClientSite*)0xdeadbeef, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();
    ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);

    /* GetMiscStatus fails */
    g_GetMiscStatusFailsWith = 0x8fafefaf;
    expected_method_list = methods_olerender_draw_with_site;
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_DRAW, NULL, (IOleClientSite*)0xdeadbeef, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();
    ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);
    g_GetMiscStatusFailsWith = S_OK;

    formatetc.cfFormat = CF_TEXT;
    formatetc.ptd = NULL;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = -1;
    formatetc.tymed = TYMED_HGLOBAL;
    expected_method_list = methods_olerender_format;
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_FORMAT, &formatetc, (IOleClientSite *)0xdeadbeef, pStorage, (void **)&pObject);
    ok(hr == S_OK ||
       broken(hr == E_INVALIDARG), /* win2k */
       "OleCreate failed with error 0x%08lx\n", hr);
    if (pObject)
    {
        IOleObject_Release(pObject);
        CHECK_NO_EXTRA_METHODS();
    }
    ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);

    expected_method_list = methods_olerender_asis;
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_ASIS, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();
    ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);

    formatetc.cfFormat = 0;
    formatetc.tymed = TYMED_NULL;
    runnable = NULL;
    expected_method_list = methods_olerender_draw_no_runnable;
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_DRAW, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();

    runnable = &OleObjectRunnable;
    cache = NULL;
    expected_method_list = methods_olerender_draw_no_cache;
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_DRAW, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();
    ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);
    g_expected_fetc = NULL;
}

static void test_OleLoad(IStorage *pStorage)
{
    HRESULT hr;
    IOleObject *pObject;
    DWORD fmt;

    static const struct expected_method methods_oleload[] =
    {
        { "OleObject_GetMiscStatus", 0 },
        { "OleObjectPersistStg_Load", 0 },
        { "OleObject_SetClientSite", 0 },
        { "OleObject_GetMiscStatus", 0 },
        { NULL, 0 }
    };

    /* Test once with IOleObject_GetMiscStatus failing */
    expected_method_list = methods_oleload;
    g_GetMiscStatusFailsWith = E_FAIL;
    hr = OleLoad(pStorage, &IID_IOleObject, (IOleClientSite *)0xdeadbeef, (void **)&pObject);
    ok(hr == S_OK ||
       broken(hr == E_INVALIDARG), /* win98 and win2k */
       "OleLoad failed with error 0x%08lx\n", hr);
    if(pObject)
    {
        DWORD dwStatus = 0xdeadbeef;
        hr = IOleObject_GetMiscStatus(pObject, DVASPECT_CONTENT, &dwStatus);
        ok(hr == E_FAIL, "Got 0x%08lx\n", hr);
        ok(dwStatus == 0x1234, "Got 0x%08lx\n", dwStatus);

        IOleObject_Release(pObject);
        CHECK_NO_EXTRA_METHODS();
    }
    ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);
    g_GetMiscStatusFailsWith = S_OK;

    /* Test again, let IOleObject_GetMiscStatus succeed. */
    expected_method_list = methods_oleload;
    hr = OleLoad(pStorage, &IID_IOleObject, (IOleClientSite *)0xdeadbeef, (void **)&pObject);
    ok(hr == S_OK ||
       broken(hr == E_INVALIDARG), /* win98 and win2k */
       "OleLoad failed with error 0x%08lx\n", hr);
    if (pObject)
    {
        DWORD dwStatus = 0xdeadbeef;
        hr = IOleObject_GetMiscStatus(pObject, DVASPECT_CONTENT, &dwStatus);
        ok(hr == S_OK, "Got 0x%08lx\n", hr);
        ok(dwStatus == 1, "Got 0x%08lx\n", dwStatus);

        IOleObject_Release(pObject);
        CHECK_NO_EXTRA_METHODS();
    }
    ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);

    for (fmt = CF_TEXT; fmt < CF_MAX; fmt++)
    {
        static const WCHAR olrepres[] = { 2,'O','l','e','P','r','e','s','0','0','0',0 };
        IStorage *stg;
        IStream *stream;
        IUnknown *obj;
        DWORD data, i, data_size;
        PresentationDataHeader header;
        HDC hdc;
        HGDIOBJ hobj;
        RECT rc;
        char buf[256];

        for (i = 0; i < 7; i++)
        {
            hr = StgCreateDocfile(NULL, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &stg);
            ok(hr == S_OK, "StgCreateDocfile error %#lx\n", hr);

            hr = IStorage_SetClass(stg, &CLSID_WineTest);
            ok(hr == S_OK, "SetClass error %#lx\n", hr);

            hr = IStorage_CreateStream(stg, olrepres, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, 0, &stream);
            ok(hr == S_OK, "CreateStream error %#lx\n", hr);

            data = ~0;
            hr = IStream_Write(stream, &data, sizeof(data), NULL);
            ok(hr == S_OK, "Write error %#lx\n", hr);

            data = fmt;
            hr = IStream_Write(stream, &data, sizeof(data), NULL);
            ok(hr == S_OK, "Write error %#lx\n", hr);

            switch (fmt)
            {
            case CF_BITMAP:
                /* FIXME: figure out stream format */
                hobj = CreateBitmap(1, 1, 1, 1, NULL);
                data_size = GetBitmapBits(hobj, sizeof(buf), buf);
                DeleteObject(hobj);
                break;

            case CF_METAFILEPICT:
            case CF_ENHMETAFILE:
                hdc = CreateMetaFileA(NULL);
                hobj = CloseMetaFile(hdc);
                data_size = GetMetaFileBitsEx(hobj, sizeof(buf), buf);
                DeleteMetaFile(hobj);
                break;

            default:
                data_size = sizeof(buf);
                memset(buf, 'A', sizeof(buf));
                break;
            }

            header.tdSize = sizeof(header.tdSize);
            header.dvAspect = DVASPECT_CONTENT;
            header.lindex = -1;
            header.advf = 1 << i;
            header.unknown7 = 0;
            header.dwObjectExtentX = 1;
            header.dwObjectExtentY = 1;
            header.dwSize = data_size;
            hr = IStream_Write(stream, &header, sizeof(header), NULL);
            ok(hr == S_OK, "Write error %#lx\n", hr);

            hr = IStream_Write(stream, buf, data_size, NULL);
            ok(hr == S_OK, "Write error %#lx\n", hr);

            IStream_Release(stream);

            hr = OleLoad(stg, &IID_IUnknown, NULL, (void **)&obj);
            /* FIXME: figure out stream format */
            if (fmt == CF_BITMAP && hr != S_OK)
            {
                IStorage_Release(stg);
                continue;
            }
            ok(hr == S_OK, "OleLoad error %#lx: cfFormat = %lu, advf = %#lx\n", hr, fmt, header.advf);

            hdc = CreateCompatibleDC(0);
            SetRect(&rc, 0, 0, 100, 100);
            hr = OleDraw(obj, DVASPECT_CONTENT, hdc, &rc);
            DeleteDC(hdc);
            if (fmt == CF_METAFILEPICT)
                ok(hr == S_OK, "OleDraw error %#lx: cfFormat = %lu, advf = %#lx\n", hr, fmt, header.advf);
            else if (fmt == CF_ENHMETAFILE)
                todo_wine
                ok(hr == S_OK, "OleDraw error %#lx: cfFormat = %lu, advf = %#lx\n", hr, fmt, header.advf);
            else
                ok(hr == OLE_E_BLANK || hr == OLE_E_NOTRUNNING || hr == E_FAIL, "OleDraw should fail: %#lx, cfFormat = %lu, advf = %#lx\n", hr, fmt, header.advf);

            IUnknown_Release(obj);
            IStorage_Release(stg);
        }
    }
}

static BOOL STDMETHODCALLTYPE draw_continue(ULONG_PTR param)
{
    CHECK_EXPECTED_METHOD("draw_continue");
    return TRUE;
}

static BOOL STDMETHODCALLTYPE draw_continue_false(ULONG_PTR param)
{
    CHECK_EXPECTED_METHOD("draw_continue_false");
    return FALSE;
}

static HRESULT WINAPI AdviseSink_QueryInterface(IAdviseSink *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IAdviseSink) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = iface;
        IAdviseSink_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI AdviseSink_AddRef(IAdviseSink *iface)
{
    return 2;
}

static ULONG WINAPI AdviseSink_Release(IAdviseSink *iface)
{
    return 1;
}


static void WINAPI AdviseSink_OnDataChange(
    IAdviseSink *iface,
    FORMATETC *pFormatetc,
    STGMEDIUM *pStgmed)
{
    CHECK_EXPECTED_METHOD("AdviseSink_OnDataChange");
}

static void WINAPI AdviseSink_OnViewChange(
    IAdviseSink *iface,
    DWORD dwAspect,
    LONG lindex)
{
    CHECK_EXPECTED_METHOD("AdviseSink_OnViewChange");
}

static void WINAPI AdviseSink_OnRename(
    IAdviseSink *iface,
    IMoniker *pmk)
{
    CHECK_EXPECTED_METHOD("AdviseSink_OnRename");
}

static void WINAPI AdviseSink_OnSave(IAdviseSink *iface)
{
    CHECK_EXPECTED_METHOD("AdviseSink_OnSave");
}

static void WINAPI AdviseSink_OnClose(IAdviseSink *iface)
{
    CHECK_EXPECTED_METHOD("AdviseSink_OnClose");
}

static const IAdviseSinkVtbl AdviseSinkVtbl =
{
    AdviseSink_QueryInterface,
    AdviseSink_AddRef,
    AdviseSink_Release,
    AdviseSink_OnDataChange,
    AdviseSink_OnViewChange,
    AdviseSink_OnRename,
    AdviseSink_OnSave,
    AdviseSink_OnClose
};

static IAdviseSink AdviseSink = { &AdviseSinkVtbl };

static HRESULT WINAPI DataObject_QueryInterface(
            IDataObject*     iface,
            REFIID           riid,
            void**           ppvObject)
{
    if (IsEqualIID(riid, &IID_IDataObject) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
        return S_OK;
    }
    *ppvObject = NULL;
    return S_OK;
}

static ULONG WINAPI DataObject_AddRef(
            IDataObject*     iface)
{
    return 2;
}

static ULONG WINAPI DataObject_Release(
            IDataObject*     iface)
{
    return 1;
}

static inline BOOL fmtetc_equal( const FORMATETC *a, const FORMATETC *b )
{
    /* FIXME ptd */
    return a->cfFormat == b->cfFormat && a->dwAspect == b->dwAspect &&
        a->lindex == b->lindex && a->tymed == b->tymed;

}

static HRESULT WINAPI DataObject_GetData(IDataObject *iface, FORMATETC *format, STGMEDIUM *medium)
{
    if (winetest_debug > 1) trace("IDataObject::GetData(cf %u)\n", format->cfFormat);

    if (data_object_format && fmtetc_equal(format, data_object_format))
    {
        switch (format->cfFormat)
        {
        case CF_DIB:
            medium->tymed = TYMED_HGLOBAL;
            medium->pUnkForRelease = NULL;
            medium->hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeof(dib_white));
            memcpy(GlobalLock(medium->hGlobal), data_object_dib, sizeof(dib_white));
            GlobalUnlock(medium->hGlobal);
            return S_OK;
        case CF_BITMAP:
            create_bitmap(medium);
            return S_OK;
        case CF_ENHMETAFILE:
            create_emf(medium);
            return S_OK;
        case CF_METAFILEPICT:
            create_mfpict(medium);
            return S_OK;
        case CF_TEXT:
            create_text(medium);
            return S_OK;
        }
    }

    return S_FALSE;
}

static HRESULT WINAPI DataObject_GetDataHere(
        IDataObject*     iface,
        LPFORMATETC      pformatetc,
        STGMEDIUM*       pmedium)
{
    CHECK_EXPECTED_METHOD("DataObject_GetDataHere");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_QueryGetData(IDataObject *iface, FORMATETC *format)
{
    if (winetest_debug > 1) trace("IDataObject::QueryGetData(cf %u)\n", format->cfFormat);

    return (data_object_format && fmtetc_equal(format, data_object_format)) ? S_OK : S_FALSE;
}

static HRESULT WINAPI DataObject_GetCanonicalFormatEtc(
        IDataObject*     iface,
        LPFORMATETC      pformatectIn,
        LPFORMATETC      pformatetcOut)
{
    CHECK_EXPECTED_METHOD("DataObject_GetCanonicalFormatEtc");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_SetData(
        IDataObject*     iface,
        LPFORMATETC      pformatetc,
        STGMEDIUM*       pmedium,
        BOOL             fRelease)
{
    CHECK_EXPECTED_METHOD("DataObject_SetData");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_EnumFormatEtc(
        IDataObject*     iface,
        DWORD            dwDirection,
        IEnumFORMATETC** ppenumFormatEtc)
{
    CHECK_EXPECTED_METHOD("DataObject_EnumFormatEtc");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_DAdvise(
        IDataObject*     iface,
        FORMATETC*       pformatetc,
        DWORD            advf,
        IAdviseSink*     pAdvSink,
        DWORD*           pdwConnection)
{
    STGMEDIUM stgmedium;

    CHECK_EXPECTED_METHOD("DataObject_DAdvise");
    *pdwConnection = 1;

    if(advf & ADVF_PRIMEFIRST)
    {
        ok(pformatetc->cfFormat == cf_test_2, "got %04x\n", pformatetc->cfFormat);
        stgmedium.tymed = TYMED_HGLOBAL;
        stgmedium.hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 4);
        stgmedium.pUnkForRelease = NULL;
        IAdviseSink_OnDataChange(pAdvSink, pformatetc, &stgmedium);
    }

    return S_OK;
}

static HRESULT WINAPI DataObject_DUnadvise(
        IDataObject*     iface,
        DWORD            dwConnection)
{
    CHECK_EXPECTED_METHOD("DataObject_DUnadvise");
    return S_OK;
}

static HRESULT WINAPI DataObject_EnumDAdvise(
        IDataObject*     iface,
        IEnumSTATDATA**  ppenumAdvise)
{
    CHECK_EXPECTED_METHOD("DataObject_EnumDAdvise");
    return OLE_E_ADVISENOTSUPPORTED;
}

static IDataObjectVtbl DataObjectVtbl =
{
    DataObject_QueryInterface,
    DataObject_AddRef,
    DataObject_Release,
    DataObject_GetData,
    DataObject_GetDataHere,
    DataObject_QueryGetData,
    DataObject_GetCanonicalFormatEtc,
    DataObject_SetData,
    DataObject_EnumFormatEtc,
    DataObject_DAdvise,
    DataObject_DUnadvise,
    DataObject_EnumDAdvise
};

static IDataObject DataObject = { &DataObjectVtbl };

static HRESULT WINAPI Unknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)) *ppv = iface;
    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI Unknown_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI Unknown_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl UnknownVtbl =
{
    Unknown_QueryInterface,
    Unknown_AddRef,
    Unknown_Release
};

static IUnknown unknown = { &UnknownVtbl };

static void check_enum_cache(IOleCache2 *cache, const STATDATA *expect, int num)
{
    IEnumSTATDATA *enum_stat;
    STATDATA stat;
    HRESULT hr;

    hr = IOleCache2_EnumCache( cache, &enum_stat );
    ok( hr == S_OK, "got %08lx\n", hr );

    while (IEnumSTATDATA_Next(enum_stat, 1, &stat, NULL) == S_OK)
    {
        ok( stat.formatetc.cfFormat == expect->formatetc.cfFormat, "got %d expect %d\n",
            stat.formatetc.cfFormat, expect->formatetc.cfFormat );
        ok( !stat.formatetc.ptd == !expect->formatetc.ptd, "got %p expect %p\n",
            stat.formatetc.ptd, expect->formatetc.ptd );
        ok( stat.formatetc.dwAspect == expect->formatetc.dwAspect, "got %ld expect %ld\n",
            stat.formatetc.dwAspect, expect->formatetc.dwAspect );
        ok( stat.formatetc.lindex == expect->formatetc.lindex, "got %ld expect %ld\n",
            stat.formatetc.lindex, expect->formatetc.lindex );
        ok( stat.formatetc.tymed == expect->formatetc.tymed, "got %ld expect %ld\n",
            stat.formatetc.tymed, expect->formatetc.tymed );
        ok( stat.advf == expect->advf, "got %ld expect %ld\n", stat.advf, expect->advf );
        ok( stat.pAdvSink == 0, "got %p\n", stat.pAdvSink );
        ok( stat.dwConnection == expect->dwConnection, "got %ld expect %ld\n", stat.dwConnection, expect->dwConnection );
        num--;
        expect++;
    }

    ok( num == 0, "incorrect number. num %d\n", num );

    IEnumSTATDATA_Release( enum_stat );
}

static void test_data_cache(void)
{
    HRESULT hr;
    IOleCache2 *pOleCache;
    IOleCache *olecache;
    IStorage *pStorage;
    IUnknown *unk, *unk2;
    IPersistStorage *pPS;
    IViewObject *pViewObject;
    IOleCacheControl *pOleCacheControl;
    IDataObject *pCacheDataObject;
    FORMATETC fmtetc;
    STGMEDIUM stgmedium;
    DWORD dwConnection;
    DWORD dwFreeze;
    RECTL rcBounds;
    HDC hdcMem;
    CLSID clsid;
    char szSystemDir[MAX_PATH];
    WCHAR wszPath[MAX_PATH];
    static const WCHAR wszShell32[] = {'\\','s','h','e','l','l','3','2','.','d','l','l',0};

    static const struct expected_method methods_cacheinitnew[] =
    {
        { "AdviseSink_OnViewChange", 0 },
        { "AdviseSink_OnViewChange", 0 },
        { "draw_continue", 1 },
        { "draw_continue_false", 1 },
        { "DataObject_DAdvise", 0 },
        { "DataObject_DAdvise", 0 },
        { "DataObject_DUnadvise", 0 },
        { "DataObject_DUnadvise", 0 },
        { NULL, 0 }
    };
    static const struct expected_method methods_cacheload[] =
    {
        { "AdviseSink_OnViewChange", 0 },
        { "draw_continue", 1 },
        { "draw_continue", 1 },
        { "draw_continue", 1 },
        { NULL, 0 }
    };
    static const struct expected_method methods_cachethenrun[] =
    {
        { "DataObject_DAdvise", 0 },
        { "DataObject_DAdvise", 0 },
        { "DataObject_DAdvise", 0 },
        { "DataObject_DAdvise", 0 },
        { "DataObject_DUnadvise", 0 },
        { "DataObject_DUnadvise", 0 },
        { "DataObject_DUnadvise", 0 },
        { "DataObject_DUnadvise", 0 },
        { NULL, 0 }
    };

    GetSystemDirectoryA(szSystemDir, ARRAY_SIZE(szSystemDir));

    expected_method_list = methods_cacheinitnew;

    fmtetc.cfFormat = CF_METAFILEPICT;
    fmtetc.dwAspect = DVASPECT_ICON;
    fmtetc.lindex = -1;
    fmtetc.ptd = NULL;
    fmtetc.tymed = TYMED_MFPICT;

    hr = StgCreateDocfile(NULL, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &pStorage);
    ok_ole_success(hr, "StgCreateDocfile");

    /* aggregation */

    /* requested is not IUnknown */
    hr = CreateDataCache(&unknown, &CLSID_NULL, &IID_IOleCache2, (void**)&pOleCache);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = CreateDataCache(&unknown, &CLSID_NULL, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IOleCache, (void**)&olecache);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IUnknown_QueryInterface(unk, &IID_IOleCache2, (void**)&pOleCache);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(unk != (IUnknown*)olecache, "got %p, expected %p\n", olecache, unk);
    ok(unk != (IUnknown*)pOleCache, "got %p, expected %p\n", pOleCache, unk);
    IOleCache2_Release(pOleCache);
    IOleCache_Release(olecache);
    IUnknown_Release(unk);

    hr = CreateDataCache(NULL, &CLSID_NULL, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IUnknown_QueryInterface(unk, &IID_IOleCache, (void**)&olecache);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IUnknown_QueryInterface(unk, &IID_IOleCache2, (void**)&pOleCache);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void**)&unk2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(unk == (IUnknown*)olecache, "got %p, expected %p\n", olecache, unk);
    ok(unk == (IUnknown*)pOleCache, "got %p, expected %p\n", pOleCache, unk);
    ok(unk == unk2, "got %p, expected %p\n", unk2, unk);
    IUnknown_Release(unk2);
    IOleCache2_Release(pOleCache);
    IOleCache_Release(olecache);
    IUnknown_Release(unk);

    /* Test with new data */

    hr = CreateDataCache(NULL, &CLSID_NULL, &IID_IOleCache2, (LPVOID *)&pOleCache);
    ok_ole_success(hr, "CreateDataCache");

    hr = IOleCache2_QueryInterface(pOleCache, &IID_IPersistStorage, (LPVOID *)&pPS);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IPersistStorage)");
    hr = IOleCache2_QueryInterface(pOleCache, &IID_IViewObject, (LPVOID *)&pViewObject);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IViewObject)");
    hr = IOleCache2_QueryInterface(pOleCache, &IID_IOleCacheControl, (LPVOID *)&pOleCacheControl);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IOleCacheControl)");

    hr = IViewObject_SetAdvise(pViewObject, DVASPECT_ICON, ADVF_PRIMEFIRST, &AdviseSink);
    ok_ole_success(hr, "IViewObject_SetAdvise");

    hr = IPersistStorage_InitNew(pPS, pStorage);
    ok_ole_success(hr, "IPersistStorage_InitNew");

    hr = IPersistStorage_IsDirty(pPS);
    ok_ole_success(hr, "IPersistStorage_IsDirty");

    hr = IPersistStorage_GetClassID(pPS, &clsid);
    ok_ole_success(hr, "IPersistStorage_GetClassID");
    ok(IsEqualCLSID(&clsid, &IID_NULL), "clsid should be blank\n");

    hr = IOleCache2_Uncache(pOleCache, 0xdeadbeef);
    ok(hr == OLE_E_NOCONNECTION, "IOleCache_Uncache with invalid value should return OLE_E_NOCONNECTION instead of 0x%lx\n", hr);

    /* Both tests crash on NT4 and below. StgCreatePropSetStg is only available on w2k and above. */
    if (GetProcAddress(GetModuleHandleA("ole32.dll"), "StgCreatePropSetStg"))
    {
        hr = IOleCache2_Cache(pOleCache, NULL, 0, &dwConnection);
        ok(hr == E_INVALIDARG, "IOleCache_Cache with NULL fmtetc should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

        hr = IOleCache2_Cache(pOleCache, NULL, 0, NULL);
        ok(hr == E_INVALIDARG, "IOleCache_Cache with NULL pdwConnection should have returned E_INVALIDARG instead of 0x%08lx\n", hr);
    }
    else
    {
        skip("tests with NULL parameters will crash on NT4 and below\n");
    }

    for (fmtetc.cfFormat = CF_TEXT; fmtetc.cfFormat < CF_MAX; fmtetc.cfFormat++)
    {
        int i;
        fmtetc.dwAspect = DVASPECT_THUMBNAIL;
        for (i = 0; i < 7; i++)
        {
            fmtetc.tymed = 1 << i;
            hr = IOleCache2_Cache(pOleCache, &fmtetc, 0, &dwConnection);
            if ((fmtetc.cfFormat == CF_METAFILEPICT && fmtetc.tymed == TYMED_MFPICT) ||
                (fmtetc.cfFormat == CF_BITMAP && fmtetc.tymed == TYMED_GDI) ||
                (fmtetc.cfFormat == CF_DIB && fmtetc.tymed == TYMED_HGLOBAL) ||
                (fmtetc.cfFormat == CF_ENHMETAFILE && fmtetc.tymed == TYMED_ENHMF))
                ok(hr == S_OK, "IOleCache_Cache cfFormat = %d, tymed = %ld should have returned S_OK instead of 0x%08lx\n",
                    fmtetc.cfFormat, fmtetc.tymed, hr);
            else if (fmtetc.tymed == TYMED_HGLOBAL)
                ok(hr == CACHE_S_FORMATETC_NOTSUPPORTED ||
                   broken(hr == S_OK && fmtetc.cfFormat == CF_BITMAP) /* Win9x & NT4 */,
                    "IOleCache_Cache cfFormat = %d, tymed = %ld should have returned CACHE_S_FORMATETC_NOTSUPPORTED instead of 0x%08lx\n",
                    fmtetc.cfFormat, fmtetc.tymed, hr);
            else
                ok(hr == DV_E_TYMED, "IOleCache_Cache cfFormat = %d, tymed = %ld should have returned DV_E_TYMED instead of 0x%08lx\n",
                    fmtetc.cfFormat, fmtetc.tymed, hr);
            if (SUCCEEDED(hr))
            {
                hr = IOleCache2_Uncache(pOleCache, dwConnection);
                ok_ole_success(hr, "IOleCache_Uncache");
            }
        }
    }

    fmtetc.cfFormat = CF_BITMAP;
    fmtetc.dwAspect = DVASPECT_THUMBNAIL;
    fmtetc.tymed = TYMED_GDI;
    hr = IOleCache2_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok_ole_success(hr, "IOleCache_Cache");

    fmtetc.cfFormat = 0;
    fmtetc.dwAspect = DVASPECT_ICON;
    fmtetc.tymed = TYMED_MFPICT;
    hr = IOleCache2_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok_ole_success(hr, "IOleCache_Cache");

    MultiByteToWideChar(CP_ACP, 0, szSystemDir, -1, wszPath, ARRAY_SIZE(wszPath));
    memcpy(wszPath+lstrlenW(wszPath), wszShell32, sizeof(wszShell32));

    fmtetc.cfFormat = CF_METAFILEPICT;
    stgmedium.tymed = TYMED_MFPICT;
    stgmedium.hMetaFilePict = OleMetafilePictFromIconAndLabel(
        LoadIconA(NULL, (LPSTR)IDI_APPLICATION), wszPath, wszPath, 0);
    stgmedium.pUnkForRelease = NULL;

    fmtetc.dwAspect = DVASPECT_CONTENT;
    hr = IOleCache2_SetData(pOleCache, &fmtetc, &stgmedium, FALSE);
    ok(hr == OLE_E_BLANK, "IOleCache_SetData for aspect not in cache should have return OLE_E_BLANK instead of 0x%08lx\n", hr);

    fmtetc.dwAspect = DVASPECT_ICON;
    hr = IOleCache2_SetData(pOleCache, &fmtetc, &stgmedium, FALSE);
    ok_ole_success(hr, "IOleCache_SetData");
    ReleaseStgMedium(&stgmedium);

    hr = IViewObject_Freeze(pViewObject, DVASPECT_ICON, -1, NULL, &dwFreeze);
    todo_wine {
    ok_ole_success(hr, "IViewObject_Freeze");
    hr = IViewObject_Freeze(pViewObject, DVASPECT_CONTENT, -1, NULL, &dwFreeze);
    ok(hr == OLE_E_BLANK, "IViewObject_Freeze with uncached aspect should have returned OLE_E_BLANK instead of 0x%08lx\n", hr);
    }

    rcBounds.left = 0;
    rcBounds.top = 0;
    rcBounds.right = 100;
    rcBounds.bottom = 100;
    hdcMem = CreateCompatibleDC(NULL);

    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");

    hr = IViewObject_Draw(pViewObject, DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok(hr == OLE_E_BLANK, "IViewObject_Draw with uncached aspect should have returned OLE_E_BLANK instead of 0x%08lx\n", hr);

    /* a NULL draw_continue fn ptr */
    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, NULL, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");

    /* draw_continue that returns FALSE to abort drawing */
    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue_false, 0xdeadbeef);
    ok(hr == E_ABORT ||
       broken(hr == S_OK), /* win9x may skip the callbacks */
       "IViewObject_Draw with draw_continue_false returns 0x%08lx\n", hr);

    DeleteDC(hdcMem);

    hr = IOleCacheControl_OnRun(pOleCacheControl, &DataObject);
    ok_ole_success(hr, "IOleCacheControl_OnRun");

    hr = IPersistStorage_Save(pPS, pStorage, TRUE);
    ok_ole_success(hr, "IPersistStorage_Save");

    hr = IPersistStorage_SaveCompleted(pPS, NULL);
    ok_ole_success(hr, "IPersistStorage_SaveCompleted");

    hr = IPersistStorage_IsDirty(pPS);
    ok(hr == S_FALSE, "IPersistStorage_IsDirty should have returned S_FALSE instead of 0x%lx\n", hr);

    IPersistStorage_Release(pPS);
    IViewObject_Release(pViewObject);
    IOleCache2_Release(pOleCache);
    IOleCacheControl_Release(pOleCacheControl);

    CHECK_NO_EXTRA_METHODS();

    /* Test with loaded data */
    expected_method_list = methods_cacheload;

    hr = CreateDataCache(NULL, &CLSID_NULL, &IID_IOleCache2, (LPVOID *)&pOleCache);
    ok_ole_success(hr, "CreateDataCache");

    hr = IOleCache2_QueryInterface(pOleCache, &IID_IPersistStorage, (LPVOID *)&pPS);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IPersistStorage)");
    hr = IOleCache2_QueryInterface(pOleCache, &IID_IViewObject, (LPVOID *)&pViewObject);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IViewObject)");

    hr = IViewObject_SetAdvise(pViewObject, DVASPECT_ICON, ADVF_PRIMEFIRST, &AdviseSink);
    ok_ole_success(hr, "IViewObject_SetAdvise");

    hr = IPersistStorage_Load(pPS, pStorage);
    ok_ole_success(hr, "IPersistStorage_Load");

    hr = IPersistStorage_IsDirty(pPS);
    ok(hr == S_FALSE, "IPersistStorage_IsDirty should have returned S_FALSE instead of 0x%lx\n", hr);

    fmtetc.cfFormat = 0;
    fmtetc.dwAspect = DVASPECT_ICON;
    fmtetc.lindex = -1;
    fmtetc.ptd = NULL;
    fmtetc.tymed = TYMED_MFPICT;
    hr = IOleCache2_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok(hr == CACHE_S_SAMECACHE, "IOleCache_Cache with already loaded data format type should return CACHE_S_SAMECACHE instead of 0x%lx\n", hr);

    rcBounds.left = 0;
    rcBounds.top = 0;
    rcBounds.right = 100;
    rcBounds.bottom = 100;
    hdcMem = CreateCompatibleDC(NULL);

    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");

    hr = IViewObject_Draw(pViewObject, DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok(hr == OLE_E_BLANK, "IViewObject_Draw with uncached aspect should have returned OLE_E_BLANK instead of 0x%08lx\n", hr);

    /* unload the cached storage object, causing it to be reloaded */
    hr = IOleCache2_DiscardCache(pOleCache, DISCARDCACHE_NOSAVE);
    ok_ole_success(hr, "IOleCache2_DiscardCache");
    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");

    /* unload the cached storage object, but don't allow it to be reloaded */
    hr = IPersistStorage_HandsOffStorage(pPS);
    ok_ole_success(hr, "IPersistStorage_HandsOffStorage");
    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");
    hr = IOleCache2_DiscardCache(pOleCache, DISCARDCACHE_NOSAVE);
    ok_ole_success(hr, "IOleCache2_DiscardCache");
    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok(hr == OLE_E_BLANK, "IViewObject_Draw with uncached aspect should have returned OLE_E_BLANK instead of 0x%08lx\n", hr);

    DeleteDC(hdcMem);

    hr = IOleCache2_InitCache(pOleCache, &DataObject);
    ok(hr == CACHE_E_NOCACHE_UPDATED, "IOleCache_InitCache should have returned CACHE_E_NOCACHE_UPDATED instead of 0x%08lx\n", hr);

    IPersistStorage_Release(pPS);
    IViewObject_Release(pViewObject);
    IOleCache2_Release(pOleCache);

    CHECK_NO_EXTRA_METHODS();

    hr = CreateDataCache(NULL, &CLSID_NULL, &IID_IOleCache2, (LPVOID *)&pOleCache);
    ok_ole_success(hr, "CreateDataCache");

    expected_method_list = methods_cachethenrun;

    hr = IOleCache2_QueryInterface(pOleCache, &IID_IDataObject, (LPVOID *)&pCacheDataObject);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IDataObject)");
    hr = IOleCache2_QueryInterface(pOleCache, &IID_IOleCacheControl, (LPVOID *)&pOleCacheControl);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IOleCacheControl)");

    fmtetc.cfFormat = CF_METAFILEPICT;
    fmtetc.dwAspect = DVASPECT_CONTENT;
    fmtetc.tymed = TYMED_MFPICT;

    hr = IOleCache2_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok_ole_success(hr, "IOleCache_Cache");

    hr = IDataObject_GetData(pCacheDataObject, &fmtetc, &stgmedium);
    ok(hr == OLE_E_BLANK, "got %08lx\n", hr);

    fmtetc.cfFormat = cf_test_1;
    fmtetc.dwAspect = DVASPECT_CONTENT;
    fmtetc.tymed = TYMED_HGLOBAL;

    hr = IOleCache2_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok(hr == CACHE_S_FORMATETC_NOTSUPPORTED, "got %08lx\n", hr);

    hr = IDataObject_GetData(pCacheDataObject, &fmtetc, &stgmedium);
    ok(hr == OLE_E_BLANK, "got %08lx\n", hr);

    fmtetc.cfFormat = cf_test_2;
    hr = IOleCache2_Cache(pOleCache, &fmtetc, ADVF_PRIMEFIRST, &dwConnection);
    ok(hr == CACHE_S_FORMATETC_NOTSUPPORTED, "got %08lx\n", hr);

    hr = IDataObject_GetData(pCacheDataObject, &fmtetc, &stgmedium);
    ok(hr == OLE_E_BLANK, "got %08lx\n", hr);

    hr = IOleCacheControl_OnRun(pOleCacheControl, &DataObject);
    ok_ole_success(hr, "IOleCacheControl_OnRun");

    fmtetc.cfFormat = cf_test_3;
    hr = IOleCache2_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok(hr == CACHE_S_FORMATETC_NOTSUPPORTED, "got %08lx\n", hr);

    fmtetc.cfFormat = cf_test_1;
    hr = IDataObject_GetData(pCacheDataObject, &fmtetc, &stgmedium);
    ok(hr == OLE_E_BLANK, "got %08lx\n", hr);

    fmtetc.cfFormat = cf_test_2;
    hr = IDataObject_GetData(pCacheDataObject, &fmtetc, &stgmedium);
    ok(hr == S_OK, "got %08lx\n", hr);
    ReleaseStgMedium(&stgmedium);

    fmtetc.cfFormat = cf_test_3;
    hr = IDataObject_GetData(pCacheDataObject, &fmtetc, &stgmedium);
    ok(hr == OLE_E_BLANK, "got %08lx\n", hr);

    IOleCacheControl_Release(pOleCacheControl);
    IDataObject_Release(pCacheDataObject);
    IOleCache2_Release(pOleCache);

    CHECK_NO_EXTRA_METHODS();

    IStorage_Release(pStorage);
}

static const WCHAR CONTENTS[] = {'C','O','N','T','E','N','T','S',0};

/* 2 x 1 x 32 bpp dib. PelsPerMeter = 200x400 */
static BYTE file_dib[] =
{
    0x42, 0x4d, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00,
    0x00, 0x00, 0x90, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static IStorage *create_storage( int num )
{
    IStorage *stg;
    IStream *stm;
    HRESULT hr;
    ULONG written;

    hr = StgCreateDocfile( NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_DELETEONRELEASE, 0, &stg );
    ok( hr == S_OK, "got %08lx\n", hr);
    hr = IStorage_SetClass( stg, &CLSID_Picture_Dib );
    ok( hr == S_OK, "got %08lx\n", hr);
    hr = IStorage_CreateStream( stg, CONTENTS, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, 0, &stm );
    ok( hr == S_OK, "got %08lx\n", hr);
    if (num == 1) /* Set biXPelsPerMeter = 0 */
    {
        file_dib[0x26] = 0;
        file_dib[0x27] = 0;
    }
    hr = IStream_Write( stm, file_dib, sizeof(file_dib), &written );
    ok( hr == S_OK, "got %08lx\n", hr);
    IStream_Release( stm );
    return stg;
}

static void test_data_cache_dib_contents_stream(int num)
{
    HRESULT hr;
    IUnknown *unk;
    IPersistStorage *persist;
    IDataObject *data;
    IViewObject2 *view;
    IStorage *stg;
    IOleCache2 *cache;
    FORMATETC fmt = {CF_DIB, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM med;
    CLSID cls;
    SIZEL sz;
    BYTE *ptr;
    BITMAPINFOHEADER expect_info;
    STATDATA enum_expect[] =
    {
        {{ CF_DIB,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, 0, NULL, 1 },
        {{ CF_BITMAP,       0, DVASPECT_CONTENT, -1, TYMED_GDI },     0, NULL, 1 },
    };

    hr = CreateDataCache( NULL, &CLSID_Picture_Metafile, &IID_IUnknown, (void **)&unk );
    ok( SUCCEEDED(hr), "got %08lx\n", hr );
    hr = IUnknown_QueryInterface( unk, &IID_IPersistStorage, (void **)&persist );
    ok( SUCCEEDED(hr), "got %08lx\n", hr );
    hr = IUnknown_QueryInterface( unk, &IID_IDataObject, (void **)&data );
    ok( SUCCEEDED(hr), "got %08lx\n", hr );
    hr = IUnknown_QueryInterface( unk, &IID_IViewObject2, (void **)&view );
    ok( SUCCEEDED(hr), "got %08lx\n", hr );
    hr = IUnknown_QueryInterface( unk, &IID_IOleCache2, (void **)&cache );
    ok( SUCCEEDED(hr), "got %08lx\n", hr );

    stg = create_storage( num );

    hr = IPersistStorage_Load( persist, stg );
    ok( SUCCEEDED(hr), "got %08lx\n", hr );
    IStorage_Release( stg );

    hr = IPersistStorage_GetClassID( persist, &cls );
    ok( SUCCEEDED(hr), "got %08lx\n", hr );
    ok( IsEqualCLSID( &cls, &CLSID_Picture_Dib ), "class id mismatch\n" );

    hr = IDataObject_GetData( data, &fmt, &med );
    ok( SUCCEEDED(hr), "got %08lx\n", hr );
    ok( med.tymed == TYMED_HGLOBAL, "got %lx\n", med.tymed );
    ok( GlobalSize( med.hGlobal ) >= sizeof(dib_white) - sizeof(BITMAPFILEHEADER),
        "got %Iu\n", GlobalSize( med.hGlobal ) );
    ptr = GlobalLock( med.hGlobal );

    expect_info = *(BITMAPINFOHEADER *)(file_dib + sizeof(BITMAPFILEHEADER));
    if (expect_info.biXPelsPerMeter == 0 || expect_info.biYPelsPerMeter == 0)
    {
        HDC hdc = GetDC( 0 );
        expect_info.biXPelsPerMeter = MulDiv( GetDeviceCaps( hdc, LOGPIXELSX ), 10000, 254 );
        expect_info.biYPelsPerMeter = MulDiv( GetDeviceCaps( hdc, LOGPIXELSY ), 10000, 254 );
        ReleaseDC( 0, hdc );
    }
    ok( !memcmp( ptr, &expect_info, sizeof(expect_info) ), "mismatch\n" );
    ok( !memcmp( ptr + sizeof(expect_info), file_dib + sizeof(BITMAPFILEHEADER) + sizeof(expect_info),
                 sizeof(file_dib) - sizeof(BITMAPFILEHEADER) - sizeof(expect_info) ), "mismatch\n" );
    GlobalUnlock( med.hGlobal );
    ReleaseStgMedium( &med );

    check_enum_cache( cache, enum_expect, 2 );

    hr = IViewObject2_GetExtent( view, DVASPECT_CONTENT, -1, NULL, &sz );
    ok( SUCCEEDED(hr), "got %08lx\n", hr );
    if (num == 0)
    {
        ok( sz.cx == 1000, "got %ld\n", sz.cx );
        ok( sz.cy == 250, "got %ld\n", sz.cy );
    }
    else
    {
        HDC hdc = GetDC( 0 );
        LONG x = 2 * 2540 / GetDeviceCaps( hdc, LOGPIXELSX );
        LONG y = 1 * 2540 / GetDeviceCaps( hdc, LOGPIXELSY );
        ok( sz.cx == x, "got %ld %ld\n", sz.cx, x );
        ok( sz.cy == y, "got %ld %ld\n", sz.cy, y );

        ReleaseDC( 0, hdc );
    }

    IOleCache2_Release( cache );
    IViewObject2_Release( view );
    IDataObject_Release( data );
    IPersistStorage_Release( persist );
    IUnknown_Release( unk );
}

static void check_bitmap_size( HBITMAP h, int cx, int cy )
{
    BITMAP bm;

    GetObjectW( h, sizeof(bm), &bm );
    ok( bm.bmWidth == cx, "got %d expect %d\n", bm.bmWidth, cx );
    ok( bm.bmHeight == cy, "got %d expect %d\n", bm.bmHeight, cy );
}

static void check_dib_size( HGLOBAL h, int cx, int cy )
{
    BITMAPINFO *info;

    info = GlobalLock( h );
    ok( info->bmiHeader.biWidth == cx, "got %ld expect %d\n", info->bmiHeader.biWidth, cx );
    ok( info->bmiHeader.biHeight == cy, "got %ld expect %d\n", info->bmiHeader.biHeight, cy );
    GlobalUnlock( h );
}

static void test_data_cache_cache(void)
{
    HRESULT hr;
    IOleCache2 *cache;
    IDataObject *data;
    FORMATETC fmt;
    DWORD conn;
    STGMEDIUM med;
    STATDATA expect[] =
    {
        {{ CF_DIB,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, 0, NULL, 0 },
        {{ CF_BITMAP,       0, DVASPECT_CONTENT, -1, TYMED_GDI },     0, NULL, 0 },
        {{ CF_METAFILEPICT, 0, DVASPECT_CONTENT, -1, TYMED_MFPICT },  0, NULL, 0 },
        {{ CF_ENHMETAFILE,  0, DVASPECT_CONTENT, -1, TYMED_ENHMF },   0, NULL, 0 }
    };
    STATDATA view_caching[] =
    {
        {{ 0,               0, DVASPECT_CONTENT,   -1, TYMED_ENHMF },   0, NULL, 0 },
        {{ 0,               0, DVASPECT_THUMBNAIL, -1, TYMED_HGLOBAL }, 0, NULL, 0 },
        {{ 0,               0, DVASPECT_DOCPRINT,  -1, TYMED_HGLOBAL }, 0, NULL, 0 },
        {{ CF_METAFILEPICT, 0, DVASPECT_ICON,      -1, TYMED_MFPICT },  0, NULL, 0 }
    };

    hr = CreateDataCache( NULL, &CLSID_NULL, &IID_IOleCache2, (void **)&cache );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* create a dib entry which will also create a bitmap entry too */
    fmt.cfFormat = CF_DIB;
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( conn == 2, "got %ld\n", conn );
    expect[0].dwConnection = conn;
    expect[1].dwConnection = conn;

    check_enum_cache( cache, expect, 2 );

    /* now try to add a bitmap */
    fmt.cfFormat = CF_BITMAP;
    fmt.tymed = TYMED_GDI;

    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == CACHE_S_SAMECACHE, "got %08lx\n", hr );

    /* metafile */
    fmt.cfFormat = CF_METAFILEPICT;
    fmt.tymed = TYMED_MFPICT;

    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( conn == 3, "got %ld\n", conn );
    expect[2].dwConnection = conn;

    check_enum_cache( cache, expect,  3);

    /* enhmetafile */
    fmt.cfFormat = CF_ENHMETAFILE;
    fmt.tymed = TYMED_ENHMF;

    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( conn == 4, "got %ld\n", conn );
    expect[3].dwConnection = conn;

    check_enum_cache( cache, expect, 4 );

    /* uncache everything */
    hr = IOleCache2_Uncache( cache, expect[3].dwConnection );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = IOleCache2_Uncache( cache, expect[2].dwConnection );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = IOleCache2_Uncache( cache, expect[0].dwConnection );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = IOleCache2_Uncache( cache, expect[0].dwConnection );
    ok( hr == OLE_E_NOCONNECTION, "got %08lx\n", hr );

    check_enum_cache( cache, expect, 0 );

    /* just create a bitmap entry which again adds both dib and bitmap */
    fmt.cfFormat = CF_BITMAP;
    fmt.tymed = TYMED_GDI;

    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == S_OK, "got %08lx\n", hr );

    expect[0].dwConnection = conn;
    expect[1].dwConnection = conn;

    check_enum_cache( cache, expect, 2 );

    /* Try setting a 1x1 bitmap */
    hr = IOleCache2_QueryInterface( cache, &IID_IDataObject, (void **) &data );
    ok( hr == S_OK, "got %08lx\n", hr );

    create_bitmap( &med );

    hr = IOleCache2_SetData( cache, &fmt, &med, TRUE );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData( data, &fmt, &med );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( med.tymed == TYMED_GDI, "got %ld\n", med.tymed );
    check_bitmap_size( med.hBitmap, 1, 1 );
    ReleaseStgMedium( &med );

    fmt.cfFormat = CF_DIB;
    fmt.tymed = TYMED_HGLOBAL;
    hr = IDataObject_GetData( data, &fmt, &med );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( med.tymed == TYMED_HGLOBAL, "got %ld\n", med.tymed );
    check_dib_size( med.hGlobal, 1, 1 );
    ReleaseStgMedium( &med );

    /* Now set a 2x1 dib */
    fmt.cfFormat = CF_DIB;
    fmt.tymed = TYMED_HGLOBAL;
    create_dib( &med );

    hr = IOleCache2_SetData( cache, &fmt, &med, TRUE );
    ok( hr == S_OK, "got %08lx\n", hr );

    fmt.cfFormat = CF_BITMAP;
    fmt.tymed = TYMED_GDI;
    hr = IDataObject_GetData( data, &fmt, &med );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( med.tymed == TYMED_GDI, "got %ld\n", med.tymed );
    check_bitmap_size( med.hBitmap, 2, 1 );
    ReleaseStgMedium( &med );

    fmt.cfFormat = CF_DIB;
    fmt.tymed = TYMED_HGLOBAL;
    hr = IDataObject_GetData( data, &fmt, &med );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( med.tymed == TYMED_HGLOBAL, "got %ld\n", med.tymed );
    check_dib_size( med.hGlobal, 2, 1 );
    ReleaseStgMedium( &med );

    /* uncache everything */
    hr = IOleCache2_Uncache( cache, conn );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* view caching */
    fmt.cfFormat = 0;
    fmt.tymed = TYMED_ENHMF;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == S_OK, "got %08lx\n", hr );
    view_caching[0].dwConnection = conn;

    fmt.tymed = TYMED_HGLOBAL;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == CACHE_S_SAMECACHE, "got %08lx\n", hr );

    fmt.dwAspect = DVASPECT_THUMBNAIL;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == S_OK, "got %08lx\n", hr );
    view_caching[1].dwConnection = conn;

    fmt.dwAspect = DVASPECT_DOCPRINT;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == S_OK, "got %08lx\n", hr );
    view_caching[2].dwConnection = conn;

    /* DVASPECT_ICON view cache gets mapped to CF_METAFILEPICT */
    fmt.dwAspect = DVASPECT_ICON;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == S_OK, "got %08lx\n", hr );
    view_caching[3].dwConnection = conn;

    check_enum_cache( cache, view_caching, 4 );

    /* uncache everything */
    hr = IOleCache2_Uncache( cache, view_caching[3].dwConnection );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = IOleCache2_Uncache( cache, view_caching[2].dwConnection );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = IOleCache2_Uncache( cache, view_caching[1].dwConnection );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = IOleCache2_Uncache( cache, view_caching[0].dwConnection );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* Only able to set cfFormat == CF_METAFILEPICT (or == 0, see above) for DVASPECT_ICON */
    fmt.dwAspect = DVASPECT_ICON;
    fmt.cfFormat = CF_DIB;
    fmt.tymed = TYMED_HGLOBAL;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == DV_E_FORMATETC, "got %08lx\n", hr );
    fmt.cfFormat = CF_BITMAP;
    fmt.tymed = TYMED_GDI;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == DV_E_FORMATETC, "got %08lx\n", hr );
    fmt.cfFormat = CF_ENHMETAFILE;
    fmt.tymed = TYMED_ENHMF;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == DV_E_FORMATETC, "got %08lx\n", hr );
    fmt.cfFormat = CF_METAFILEPICT;
    fmt.tymed = TYMED_MFPICT;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* uncache everything */
    hr = IOleCache2_Uncache( cache, conn );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* tymed == 0 */
    fmt.cfFormat = CF_ENHMETAFILE;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.tymed = 0;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == DV_E_TYMED, "got %08lx\n", hr );

    IDataObject_Release( data );
    IOleCache2_Release( cache );

    /* tests for a static class cache */
    hr = CreateDataCache( NULL, &CLSID_Picture_Dib, &IID_IOleCache2, (void **)&cache );

    fmt.cfFormat = CF_DIB;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.tymed = TYMED_HGLOBAL;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == CACHE_S_SAMECACHE, "got %08lx\n", hr );

    /* aspect other than DVASPECT_CONTENT should fail */
    fmt.dwAspect = DVASPECT_THUMBNAIL;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( FAILED(hr), "got %08lx\n", hr );

    fmt.dwAspect = DVASPECT_DOCPRINT;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( FAILED(hr), "got %08lx\n", hr );

    /* try caching another clip format */
    fmt.cfFormat = CF_METAFILEPICT;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.tymed = TYMED_MFPICT;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( FAILED(hr), "got %08lx\n", hr );

    /* As an exception, it's possible to add an icon aspect */
    fmt.cfFormat = CF_METAFILEPICT;
    fmt.dwAspect = DVASPECT_ICON;
    fmt.tymed = TYMED_MFPICT;
    hr = IOleCache2_Cache( cache, &fmt, 0, &conn );
    ok( hr == S_OK, "got %08lx\n", hr );

    IOleCache2_Release( cache );
}

/* The CLSID_Picture_ classes automatically create appropriate cache entries */
static void test_data_cache_init(void)
{
    HRESULT hr;
    IOleCache2 *cache;
    IPersistStorage *persist;
    int i;
    CLSID clsid;
    static const STATDATA enum_expect[] =
    {
        {{ CF_DIB,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, 0, NULL, 1 },
        {{ CF_BITMAP,       0, DVASPECT_CONTENT, -1, TYMED_GDI },     0, NULL, 1 },
        {{ CF_METAFILEPICT, 0, DVASPECT_CONTENT, -1, TYMED_MFPICT },  0, NULL, 1 },
        {{ CF_ENHMETAFILE,  0, DVASPECT_CONTENT, -1, TYMED_ENHMF },   0, NULL, 1 }
    };
    static const struct
    {
        const CLSID *clsid;
        int enum_start, enum_num;
    } data[] =
    {
        { &CLSID_NULL, 0, 0 },
        { &CLSID_WineTestOld, 0, 0 },
        { &CLSID_Picture_Dib, 0, 2 },
        { &CLSID_Picture_Metafile, 2, 1 },
        { &CLSID_Picture_EnhMetafile, 3, 1 }
    };

    for (i = 0; i < ARRAY_SIZE(data); i++)
    {
        hr = CreateDataCache( NULL, data[i].clsid, &IID_IOleCache2, (void **)&cache );
        ok( hr == S_OK, "got %08lx\n", hr );

        check_enum_cache( cache, enum_expect + data[i].enum_start , data[i].enum_num );

        IOleCache2_QueryInterface( cache, &IID_IPersistStorage, (void **) &persist );
        hr = IPersistStorage_GetClassID( persist, &clsid );
        ok( hr == S_OK, "got %08lx\n", hr );
        ok( IsEqualCLSID( &clsid, data[i].clsid ), "class id mismatch %s %s\n", wine_dbgstr_guid( &clsid ),
            wine_dbgstr_guid( data[i].clsid ) );

        IPersistStorage_Release( persist );
        IOleCache2_Release( cache );
    }
}

static void test_data_cache_initnew(void)
{
    HRESULT hr;
    IOleCache2 *cache;
    IPersistStorage *persist;
    IStorage *stg_dib, *stg_mf, *stg_wine;
    CLSID clsid;
    static const STATDATA initnew_expect[] =
    {
        {{ CF_DIB,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, 0, NULL, 1 },
        {{ CF_BITMAP,       0, DVASPECT_CONTENT, -1, TYMED_GDI },     0, NULL, 1 },
    };
    static const STATDATA initnew2_expect[] =
    {
        {{ CF_METAFILEPICT, 0, DVASPECT_CONTENT, -1, TYMED_MFPICT },  0, NULL, 1 },
        {{ CF_DIB,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, 0, NULL, 2 },
        {{ CF_BITMAP,       0, DVASPECT_CONTENT, -1, TYMED_GDI },     0, NULL, 2 },
    };
    static const STATDATA initnew3_expect[] =
    {
        {{ CF_DIB,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, 0, NULL, 1 },
        {{ CF_BITMAP,       0, DVASPECT_CONTENT, -1, TYMED_GDI },     0, NULL, 1 },
        {{ CF_DIB,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, 0, NULL, 2 },
        {{ CF_BITMAP,       0, DVASPECT_CONTENT, -1, TYMED_GDI },     0, NULL, 2 },
        {{ CF_METAFILEPICT, 0, DVASPECT_CONTENT, -1, TYMED_MFPICT },  0, NULL, 3 },
    };
    static const STATDATA initnew4_expect[] =
    {
        {{ CF_DIB,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, 0, NULL, 2 },
        {{ CF_BITMAP,       0, DVASPECT_CONTENT, -1, TYMED_GDI },     0, NULL, 2 },
        {{ CF_METAFILEPICT, 0, DVASPECT_CONTENT, -1, TYMED_MFPICT },  0, NULL, 3 },
        {{ CF_DIB,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, 0, NULL, 4 },
        {{ CF_BITMAP,       0, DVASPECT_CONTENT, -1, TYMED_GDI },     0, NULL, 4 },
    };

    hr = StgCreateDocfile( NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_DELETEONRELEASE, 0, &stg_dib );
    ok( hr == S_OK, "got %08lx\n", hr);
    hr = IStorage_SetClass( stg_dib, &CLSID_Picture_Dib );
    ok( hr == S_OK, "got %08lx\n", hr);

    hr = StgCreateDocfile( NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_DELETEONRELEASE, 0, &stg_mf );
    ok( hr == S_OK, "got %08lx\n", hr);
    hr = IStorage_SetClass( stg_mf, &CLSID_Picture_Metafile );
    ok( hr == S_OK, "got %08lx\n", hr);

    hr = StgCreateDocfile( NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_DELETEONRELEASE, 0, &stg_wine );
    ok( hr == S_OK, "got %08lx\n", hr);
    hr = IStorage_SetClass( stg_wine, &CLSID_WineTestOld );
    ok( hr == S_OK, "got %08lx\n", hr);

    hr = CreateDataCache( NULL, &CLSID_WineTestOld, &IID_IOleCache2, (void **)&cache );
    ok( hr == S_OK, "got %08lx\n", hr );
    IOleCache2_QueryInterface( cache, &IID_IPersistStorage, (void **) &persist );

    hr = IPersistStorage_InitNew( persist, stg_dib );
    ok( hr == S_OK, "got %08lx\n", hr);

    hr = IPersistStorage_GetClassID( persist, &clsid );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( IsEqualCLSID( &clsid, &CLSID_Picture_Dib ), "got %s\n", wine_dbgstr_guid( &clsid ) );

    check_enum_cache( cache, initnew_expect, 2 );

    hr = IPersistStorage_InitNew( persist, stg_mf );
    ok( hr == CO_E_ALREADYINITIALIZED, "got %08lx\n", hr);

    hr = IPersistStorage_HandsOffStorage( persist );
    ok( hr == S_OK, "got %08lx\n", hr);

    hr = IPersistStorage_GetClassID( persist, &clsid );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( IsEqualCLSID( &clsid, &CLSID_Picture_Dib ), "got %s\n", wine_dbgstr_guid( &clsid ) );

    hr = IPersistStorage_InitNew( persist, stg_mf );
    ok( hr == S_OK, "got %08lx\n", hr);

    hr = IPersistStorage_GetClassID( persist, &clsid );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( IsEqualCLSID( &clsid, &CLSID_Picture_Metafile ), "got %s\n", wine_dbgstr_guid( &clsid ) );

    check_enum_cache( cache, initnew2_expect, 3 );

    hr = IPersistStorage_HandsOffStorage( persist );
    ok( hr == S_OK, "got %08lx\n", hr);

    hr = IPersistStorage_InitNew( persist, stg_dib );
    ok( hr == S_OK, "got %08lx\n", hr);

    hr = IPersistStorage_GetClassID( persist, &clsid );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( IsEqualCLSID( &clsid, &CLSID_Picture_Dib ), "got %s\n", wine_dbgstr_guid( &clsid ) );

    check_enum_cache( cache, initnew3_expect, 5 );

    hr = IPersistStorage_HandsOffStorage( persist );
    ok( hr == S_OK, "got %08lx\n", hr);

    hr = IPersistStorage_InitNew( persist, stg_wine );
    ok( hr == S_OK, "got %08lx\n", hr);

    hr = IPersistStorage_GetClassID( persist, &clsid );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( IsEqualCLSID( &clsid, &CLSID_WineTestOld ), "got %s\n", wine_dbgstr_guid( &clsid ) );

    check_enum_cache( cache, initnew4_expect, 5 );

    IStorage_Release( stg_wine );
    IStorage_Release( stg_mf );
    IStorage_Release( stg_dib );

    IPersistStorage_Release( persist );
    IOleCache2_Release( cache );
}

static BOOL compare_global(HGLOBAL handle, const void *data, SIZE_T size)
{
    const void *mem = GlobalLock(handle);
    BOOL ret = GlobalSize(handle) == size && !memcmp(data, mem, size);
    GlobalUnlock(handle);
    return ret;
}

static void test_data_cache_updatecache( void )
{
    FORMATETC dib_fmt = {CF_DIB, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    FORMATETC emf_fmt = {CF_ENHMETAFILE, NULL, DVASPECT_CONTENT, -1, TYMED_ENHMF};
    FORMATETC wmf_fmt = {CF_METAFILEPICT, NULL, DVASPECT_CONTENT, -1, TYMED_MFPICT};
    FORMATETC view_fmt = {0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    IDataObject *data;
    HRESULT hr;
    IOleCache2 *cache;
    STGMEDIUM medium;
    DWORD conn[4];

    static STATDATA view_cache[] =
    {
        {{ 0,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, 0, NULL, 0 }
    };
    static STATDATA view_cache_after_dib[] =
    {
        {{ CF_DIB,          0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, 0, NULL, 0 },
        {{ CF_BITMAP,       0, DVASPECT_CONTENT, -1, TYMED_GDI },     0, NULL, 0 }
    };

    hr = CreateDataCache( NULL, &CLSID_WineTestOld, &IID_IOleCache2, (void **)&cache );
    ok( hr == S_OK, "got %08lx\n", hr );
    IOleCache2_QueryInterface(cache, &IID_IDataObject, (void **)&data);

    data_object_format = NULL;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ALL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IOleCache2_Cache( cache, &dib_fmt, 0, &conn[0] );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ALL, NULL );
    ok( hr == CACHE_E_NOCACHE_UPDATED, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    data_object_format = &dib_fmt;
    data_object_dib = dib_white;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ALL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.tymed == TYMED_HGLOBAL, "Got unexpected tymed %lu.\n", medium.tymed);
    ok(compare_global(medium.hGlobal, dib_white, sizeof(dib_white)), "Media didn't match.\n");

    hr = IOleCache2_Cache(cache, &emf_fmt, 0, &conn[1]);
    ok( hr == S_OK, "got %08lx\n", hr );

    data_object_dib = dib_black;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ALL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.tymed == TYMED_HGLOBAL, "Got unexpected tymed %lu.\n", medium.tymed);
    ok(compare_global(medium.hGlobal, dib_black, sizeof(dib_black)), "Media didn't match.\n");

    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    hr = IDataObject_GetData(data, &wmf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    data_object_format = &emf_fmt;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ALL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.tymed == TYMED_HGLOBAL, "Got unexpected tymed %lu.\n", medium.tymed);

    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.tymed == TYMED_ENHMF, "Got unexpected tymed %lu.\n", medium.tymed);

    hr = IDataObject_GetData(data, &wmf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    data_object_format = &wmf_fmt;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ALL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.tymed == TYMED_HGLOBAL, "Got unexpected tymed %lu.\n", medium.tymed);

    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.tymed == TYMED_ENHMF, "Got unexpected tymed %lu.\n", medium.tymed);

    hr = IDataObject_GetData(data, &wmf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    hr = IOleCache2_Uncache( cache, conn[1] );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &wmf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IOleCache2_Cache( cache, &wmf_fmt, 0, &conn[1] );
    ok( hr == S_OK, "got %08lx\n", hr );

    data_object_format = &emf_fmt;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ALL, NULL );
    ok( hr == CACHE_E_NOCACHE_UPDATED, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &wmf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    data_object_format = &wmf_fmt;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ALL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &wmf_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IOleCache2_Uncache( cache, conn[1] );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = IOleCache2_Uncache( cache, conn[0] );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* Test view caching. */

    hr = IOleCache2_Cache( cache, &view_fmt, 0, &conn[0] );
    ok( hr == S_OK, "got %08lx\n", hr );
    view_cache[0].dwConnection = conn[0];

    data_object_format = NULL;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ALL, NULL );
    ok( hr == CACHE_E_NOCACHE_UPDATED, "got %08lx\n", hr );

    check_enum_cache( cache, view_cache, 1 );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &view_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    data_object_format = &dib_fmt;
    data_object_dib = dib_white;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ALL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    view_cache_after_dib[0].dwConnection = view_cache_after_dib[1].dwConnection = view_cache[0].dwConnection;
    check_enum_cache( cache, view_cache_after_dib, 2 );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.tymed == TYMED_HGLOBAL, "Got unexpected tymed %lu.\n", medium.tymed);
    ok(compare_global(medium.hGlobal, dib_white, sizeof(dib_white)), "Media didn't match.\n");
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &view_fmt, &medium);
    todo_wine ok(hr == DV_E_CLIPFORMAT, "Got hr %#lx.\n", hr);

    data_object_format = &emf_fmt;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ALL, NULL );
    ok( hr == CACHE_E_NOCACHE_UPDATED, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    hr = IOleCache2_Uncache( cache, conn[0] );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* Try some different flags */

    hr = IOleCache2_Cache( cache, &dib_fmt, 0, &conn[0] );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IOleCache2_Cache( cache, &emf_fmt, ADVF_NODATA, &conn[1] );
    ok( hr == S_OK, "got %08lx\n", hr );

    data_object_format = &dib_fmt;
    data_object_dib = dib_white;
    hr = IOleCache2_UpdateCache(cache, &DataObject, UPDFCACHE_IFBLANK, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.tymed == TYMED_HGLOBAL, "Got unexpected tymed %lu.\n", medium.tymed);
    ok(compare_global(medium.hGlobal, dib_white, sizeof(dib_white)), "Media didn't match.\n");
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    data_object_dib = dib_black;
    hr = IOleCache2_UpdateCache(cache, &DataObject, UPDFCACHE_IFBLANK, NULL);
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.tymed == TYMED_HGLOBAL, "Got unexpected tymed %lu.\n", medium.tymed);
    ok(compare_global(medium.hGlobal, dib_white, sizeof(dib_white)), "Media didn't match.\n");
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    data_object_format = &emf_fmt;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_IFBLANK , NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(medium.tymed == TYMED_HGLOBAL, "Got unexpected tymed %lu.\n", medium.tymed);
    ok(compare_global(medium.hGlobal, dib_white, sizeof(dib_white)), "Media didn't match.\n");
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    hr = IOleCache2_DiscardCache( cache, DISCARDCACHE_NOSAVE );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    data_object_format = &emf_fmt;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_IFBLANK | UPDFCACHE_NODATACACHE, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IOleCache2_DiscardCache( cache, DISCARDCACHE_NOSAVE );
    ok( hr == S_OK, "got %08lx\n", hr );

    data_object_format = &dib_fmt;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ONLYIFBLANK | UPDFCACHE_NORMALCACHE, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    data_object_format = &emf_fmt;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ONLYIFBLANK | UPDFCACHE_NORMALCACHE, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_IFBLANK | UPDFCACHE_NORMALCACHE, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    hr = IOleCache2_DiscardCache( cache, DISCARDCACHE_NOSAVE );
    ok( hr == S_OK, "got %08lx\n", hr );

    data_object_format = &dib_fmt;
    hr = IOleCache2_InitCache( cache, &DataObject );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    data_object_format = &emf_fmt;
    hr = IOleCache2_UpdateCache( cache, &DataObject, UPDFCACHE_ONLYIFBLANK | UPDFCACHE_NORMALCACHE, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IDataObject_GetData(data, &dib_fmt, &medium);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDataObject_GetData(data, &emf_fmt, &medium);
    ok(hr == OLE_E_BLANK, "Got hr %#lx.\n", hr);

    IDataObject_Release(data);
    IOleCache2_Release( cache );
}

static void test_default_handler(void)
{
    HRESULT hr;
    IOleObject *pObject;
    IRunnableObject *pRunnableObject;
    IOleClientSite *pClientSite;
    IDataObject *pDataObject;
    SIZEL sizel;
    DWORD dwStatus;
    CLSID clsid;
    LPOLESTR pszUserType;
    LOGPALETTE palette;
    DWORD dwAdvConn;
    IMoniker *pMoniker;
    FORMATETC fmtetc;
    IOleInPlaceObject *pInPlaceObj;
    IEnumOLEVERB *pEnumVerbs;
    DWORD dwRegister;
    static const WCHAR wszUnknown[] = {'U','n','k','n','o','w','n',0};
    static const WCHAR wszHostName[] = {'W','i','n','e',' ','T','e','s','t',' ','P','r','o','g','r','a','m',0};
    static const WCHAR wszDelim[] = {'!',0};

    static const struct expected_method methods_embeddinghelper[] =
    {
        { NULL, 0 }
    };

    hr = CoCreateInstance(&CLSID_WineTest, NULL, CLSCTX_INPROC_HANDLER, &IID_IOleObject, (void **)&pObject);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance should have failed with REGDB_E_CLASSNOTREG instead of 0x%08lx\n", hr);

    hr = OleCreateDefaultHandler(&CLSID_WineTest, NULL, &IID_IOleObject, (void **)&pObject);
    ok_ole_success(hr, "OleCreateDefaultHandler");

    hr = IOleObject_QueryInterface(pObject, &IID_IOleInPlaceObject, (void **)&pInPlaceObj);
    ok(hr == E_NOINTERFACE, "IOleObject_QueryInterface(&IID_IOleInPlaceObject) should return E_NOINTERFACE instead of 0x%08lx\n", hr);

    hr = IOleObject_Advise(pObject, &AdviseSink, &dwAdvConn);
    ok_ole_success(hr, "IOleObject_Advise");

    hr = IOleObject_Close(pObject, OLECLOSE_NOSAVE);
    ok_ole_success(hr, "IOleObject_Close");

    /* FIXME: test IOleObject_EnumAdvise */

    hr = IOleObject_EnumVerbs(pObject, &pEnumVerbs);
    ok(hr == REGDB_E_CLASSNOTREG, "IOleObject_EnumVerbs should have returned REGDB_E_CLASSNOTREG instead of 0x%08lx\n", hr);

    hr = IOleObject_GetClientSite(pObject, &pClientSite);
    ok_ole_success(hr, "IOleObject_GetClientSite");

    hr = IOleObject_SetClientSite(pObject, pClientSite);
    ok_ole_success(hr, "IOleObject_SetClientSite");

    hr = IOleObject_GetClipboardData(pObject, 0, &pDataObject);
    ok(hr == OLE_E_NOTRUNNING,
       "IOleObject_GetClipboardData should have returned OLE_E_NOTRUNNING instead of 0x%08lx\n",
       hr);

    hr = IOleObject_GetExtent(pObject, DVASPECT_CONTENT, &sizel);
    ok(hr == OLE_E_BLANK, "IOleObject_GetExtent should have returned OLE_E_BLANK instead of 0x%08lx\n",
       hr);

    hr = IOleObject_GetMiscStatus(pObject, DVASPECT_CONTENT, &dwStatus);
    ok(hr == REGDB_E_CLASSNOTREG, "IOleObject_GetMiscStatus should have returned REGDB_E_CLASSNOTREG instead of 0x%08lx\n", hr);

    hr = IOleObject_GetUserClassID(pObject, &clsid);
    ok_ole_success(hr, "IOleObject_GetUserClassID");
    ok(IsEqualCLSID(&clsid, &CLSID_WineTest), "clsid != CLSID_WineTest\n");

    hr = IOleObject_GetUserType(pObject, USERCLASSTYPE_FULL, &pszUserType);
    todo_wine {
    ok_ole_success(hr, "IOleObject_GetUserType");
    ok(!lstrcmpW(pszUserType, wszUnknown), "Retrieved user type was wrong\n");
    }

    hr = IOleObject_InitFromData(pObject, NULL, TRUE, 0);
    ok(hr == OLE_E_NOTRUNNING, "IOleObject_InitFromData should have returned OLE_E_NOTRUNNING instead of 0x%08lx\n", hr);

    hr = IOleObject_IsUpToDate(pObject);
    ok(hr == OLE_E_NOTRUNNING, "IOleObject_IsUpToDate should have returned OLE_E_NOTRUNNING instead of 0x%08lx\n", hr);

    palette.palNumEntries = 1;
    palette.palVersion = 2;
    memset(&palette.palPalEntry[0], 0, sizeof(palette.palPalEntry[0]));
    hr = IOleObject_SetColorScheme(pObject, &palette);
    ok(hr == OLE_E_NOTRUNNING, "IOleObject_SetColorScheme should have returned OLE_E_NOTRUNNING instead of 0x%08lx\n", hr);

    sizel.cx = sizel.cy = 0;
    hr = IOleObject_SetExtent(pObject, DVASPECT_CONTENT, &sizel);
    ok(hr == OLE_E_NOTRUNNING, "IOleObject_SetExtent should have returned OLE_E_NOTRUNNING instead of 0x%08lx\n", hr);

    hr = IOleObject_SetHostNames(pObject, wszHostName, NULL);
    ok_ole_success(hr, "IOleObject_SetHostNames");

    hr = CreateItemMoniker(wszDelim, wszHostName, &pMoniker);
    ok_ole_success(hr, "CreateItemMoniker");
    hr = IOleObject_SetMoniker(pObject, OLEWHICHMK_CONTAINER, pMoniker);
    ok_ole_success(hr, "IOleObject_SetMoniker");
    IMoniker_Release(pMoniker);

    hr = IOleObject_GetMoniker(pObject, OLEGETMONIKER_ONLYIFTHERE, OLEWHICHMK_CONTAINER, &pMoniker);
    ok(hr == E_FAIL, "IOleObject_GetMoniker should have returned E_FAIL instead of 0x%08lx\n", hr);

    hr = IOleObject_Update(pObject);
    todo_wine
    ok(hr == REGDB_E_CLASSNOTREG, "IOleObject_Update should have returned REGDB_E_CLASSNOTREG instead of 0x%08lx\n", hr);

    hr = IOleObject_QueryInterface(pObject, &IID_IDataObject, (void **)&pDataObject);
    ok_ole_success(hr, "IOleObject_QueryInterface");

    fmtetc.cfFormat = CF_TEXT;
    fmtetc.ptd = NULL;
    fmtetc.dwAspect = DVASPECT_CONTENT;
    fmtetc.lindex = -1;
    fmtetc.tymed = TYMED_NULL;
    hr = IDataObject_DAdvise(pDataObject, &fmtetc, 0, &AdviseSink, &dwAdvConn);
    ok_ole_success(hr, "IDataObject_DAdvise");

    fmtetc.cfFormat = CF_ENHMETAFILE;
    fmtetc.ptd = NULL;
    fmtetc.dwAspect = DVASPECT_CONTENT;
    fmtetc.lindex = -1;
    fmtetc.tymed = TYMED_ENHMF;
    hr = IDataObject_DAdvise(pDataObject, &fmtetc, 0, &AdviseSink, &dwAdvConn);
    ok_ole_success(hr, "IDataObject_DAdvise");

    fmtetc.cfFormat = CF_ENHMETAFILE;
    fmtetc.ptd = NULL;
    fmtetc.dwAspect = DVASPECT_CONTENT;
    fmtetc.lindex = -1;
    fmtetc.tymed = TYMED_ENHMF;
    hr = IDataObject_QueryGetData(pDataObject, &fmtetc);
    ok(hr == OLE_E_NOTRUNNING, "IDataObject_QueryGetData should have returned OLE_E_NOTRUNNING instead of 0x%08lx\n", hr);

    fmtetc.cfFormat = CF_TEXT;
    fmtetc.ptd = NULL;
    fmtetc.dwAspect = DVASPECT_CONTENT;
    fmtetc.lindex = -1;
    fmtetc.tymed = TYMED_NULL;
    hr = IDataObject_QueryGetData(pDataObject, &fmtetc);
    ok(hr == OLE_E_NOTRUNNING, "IDataObject_QueryGetData should have returned OLE_E_NOTRUNNING instead of 0x%08lx\n", hr);

    hr = IOleObject_QueryInterface(pObject, &IID_IRunnableObject, (void **)&pRunnableObject);
    ok_ole_success(hr, "IOleObject_QueryInterface");

    hr = IRunnableObject_SetContainedObject(pRunnableObject, TRUE);
    ok_ole_success(hr, "IRunnableObject_SetContainedObject");

    hr = IRunnableObject_Run(pRunnableObject, NULL);
    ok(hr == REGDB_E_CLASSNOTREG, "IOleObject_Run should have returned REGDB_E_CLASSNOTREG instead of 0x%08lx\n", hr);

    hr = IOleObject_Close(pObject, OLECLOSE_NOSAVE);
    ok_ole_success(hr, "IOleObject_Close");

    IRunnableObject_Release(pRunnableObject);
    IOleObject_Release(pObject);

    /* Test failure propagation from delegate ::QueryInterface */
    hr = CoRegisterClassObject(&CLSID_WineTest, (IUnknown*)&OleObjectCF,
                               CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &dwRegister);
    ok_ole_success(hr, "CoRegisterClassObject");
    if(SUCCEEDED(hr))
    {
        expected_method_list = methods_embeddinghelper;
        hr = OleCreateEmbeddingHelper(&CLSID_WineTest, NULL, EMBDHLP_INPROC_SERVER,
                                      &OleObjectCF, &IID_IOleObject, (void**)&pObject);
        ok_ole_success(hr, "OleCreateEmbeddingHelper");
        if(SUCCEEDED(hr))
        {
            IUnknown *punk;

            g_QIFailsWith = E_FAIL;
            hr = IOleObject_QueryInterface(pObject, &IID_WineTest, (void**)&punk);
            ok(hr == E_FAIL, "Got 0x%08lx\n", hr);

            g_QIFailsWith = E_NOINTERFACE;
            hr = IOleObject_QueryInterface(pObject, &IID_WineTest, (void**)&punk);
            ok(hr == E_NOINTERFACE, "Got 0x%08lx\n", hr);

            g_QIFailsWith = CO_E_OBJNOTCONNECTED;
            hr = IOleObject_QueryInterface(pObject, &IID_WineTest, (void**)&punk);
            ok(hr == CO_E_OBJNOTCONNECTED, "Got 0x%08lx\n", hr);

            g_QIFailsWith = 0x87654321;
            hr = IOleObject_QueryInterface(pObject, &IID_WineTest, (void**)&punk);
            ok(hr == 0x87654321, "Got 0x%08lx\n", hr);

            IOleObject_Release(pObject);
        }

        CHECK_NO_EXTRA_METHODS();
        todo_wine ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);

        hr = CoRevokeClassObject(dwRegister);
        ok_ole_success(hr, "CoRevokeClassObject");
    }
}

static void test_runnable(void)
{
    static const struct expected_method methods_query_runnable[] =
    {
        { "OleObjectRunnable_IsRunning", 0 },
        { NULL, 0 }
    };

    static const struct expected_method methods_no_runnable[] =
    {
        { NULL, 0 }
    };

    BOOL ret;
    IOleObject *object = &OleObject;

    /* null argument */
    ret = OleIsRunning(NULL);
    ok(ret == FALSE, "got %d\n", ret);

    expected_method_list = methods_query_runnable;
    ret = OleIsRunning(object);
    ok(ret == TRUE, "Object should be running\n");
    CHECK_NO_EXTRA_METHODS();
    todo_wine ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);

    g_isRunning = FALSE;
    expected_method_list = methods_query_runnable;
    ret = OleIsRunning(object);
    ok(ret == FALSE, "Object should not be running\n");
    CHECK_NO_EXTRA_METHODS();
    todo_wine ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);

    g_showRunnable = FALSE;  /* QueryInterface(IID_IRunnableObject, ...) will fail */
    expected_method_list = methods_no_runnable;
    ret = OleIsRunning(object);
    ok(ret == TRUE, "Object without IRunnableObject should be running\n");
    CHECK_NO_EXTRA_METHODS();
    todo_wine ok(!ole_object_refcount, "Got outstanding refcount %ld.\n", ole_object_refcount);

    g_isRunning = TRUE;
    g_showRunnable = TRUE;
}


static HRESULT WINAPI OleRun_QueryInterface(IRunnableObject *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IRunnableObject)) {
        *ppv = iface;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI OleRun_AddRef(IRunnableObject *iface)
{
    return 2;
}

static ULONG WINAPI OleRun_Release(IRunnableObject *iface)
{
    return 1;
}

static HRESULT WINAPI OleRun_GetRunningClass(IRunnableObject *iface, CLSID *clsid)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleRun_Run(IRunnableObject *iface, LPBINDCTX ctx)
{
    ok(ctx == NULL, "got %p\n", ctx);
    return 0xdeadc0de;
}

static BOOL WINAPI OleRun_IsRunning(IRunnableObject *iface)
{
    ok(0, "unexpected\n");
    return FALSE;
}

static HRESULT WINAPI OleRun_LockRunning(IRunnableObject *iface, BOOL lock,
    BOOL last_unlock_closes)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleRun_SetContainedObject(IRunnableObject *iface, BOOL contained)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static const IRunnableObjectVtbl oleruntestvtbl =
{
    OleRun_QueryInterface,
    OleRun_AddRef,
    OleRun_Release,
    OleRun_GetRunningClass,
    OleRun_Run,
    OleRun_IsRunning,
    OleRun_LockRunning,
    OleRun_SetContainedObject
};

static IRunnableObject testrunnable = { &oleruntestvtbl };

static void test_OleRun(void)
{
    HRESULT hr;

    /* doesn't support IRunnableObject */
    hr = OleRun(&unknown);
    ok(hr == S_OK, "OleRun failed 0x%08lx\n", hr);

    hr = OleRun((IUnknown*)&testrunnable);
    ok(hr == 0xdeadc0de, "got 0x%08lx\n", hr);
}

static void test_OleLockRunning(void)
{
    HRESULT hr;

    hr = OleLockRunning(&unknown, TRUE, FALSE);
    ok(hr == S_OK, "OleLockRunning failed 0x%08lx\n", hr);
}

static void test_OleDraw(void)
{
    HRESULT hr;
    RECT rect;

    hr = OleDraw((IUnknown*)&viewobject, 0, (HDC)0x1, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = OleDraw(NULL, 0, (HDC)0x1, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = OleDraw(NULL, 0, (HDC)0x1, &rect);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
}

static const WCHAR olepres0W[] = {2,'O','l','e','P','r','e','s','0','0','0',0};
static const WCHAR comp_objW[] = {1,'C','o','m','p','O','b','j',0};
static IStream *comp_obj_stream;
static IStream *ole_stream;
static IStream *olepres_stream;
static IStream *contents_stream;

static HRESULT WINAPI Storage_QueryInterface(IStorage *iface, REFIID riid, void **ppvObject)
{
    ok(0, "unexpected call to QueryInterface\n");
    return E_NOTIMPL;
}

static ULONG WINAPI Storage_AddRef(IStorage *iface)
{
    ok(0, "unexpected call to AddRef\n");
    return 2;
}

static ULONG WINAPI Storage_Release(IStorage *iface)
{
    ok(0, "unexpected call to Release\n");
    return 1;
}

static HRESULT WINAPI Storage_CreateStream(IStorage *iface, LPCOLESTR pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm)
{
    ULARGE_INTEGER size = {{0}};
    LARGE_INTEGER pos = {{0}};
    HRESULT hr;

    if (!lstrcmpW(pwcsName, comp_objW))
    {
        CHECK_EXPECT(Storage_CreateStream_CompObj);
        *ppstm = comp_obj_stream;

        todo_wine ok(grfMode == (STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE), "grfMode = %lx\n", grfMode);
    }
    else if (!lstrcmpW(pwcsName, olepres0W))
    {
        CHECK_EXPECT(Storage_CreateStream_OlePres);
        *ppstm = olepres_stream;

        todo_wine ok(grfMode == (STGM_SHARE_EXCLUSIVE|STGM_READWRITE), "grfMode = %lx\n", grfMode);
    }
    else
    {
        todo_wine
        ok(0, "unexpected stream name %s\n", wine_dbgstr_w(pwcsName));
#if 0   /* FIXME: return NULL once Wine is fixed */
        *ppstm = NULL;
        return E_NOTIMPL;
#else
        *ppstm = contents_stream;
#endif
    }

    ok(!reserved1, "reserved1 = %lx\n", reserved1);
    ok(!reserved2, "reserved2 = %lx\n", reserved2);
    ok(!!ppstm, "ppstm = NULL\n");

    IStream_AddRef(*ppstm);
    hr = IStream_Seek(*ppstm, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek returned %lx\n", hr);
    hr = IStream_SetSize(*ppstm, size);
    ok(hr == S_OK, "IStream_SetSize returned %lx\n", hr);
    return S_OK;
}

static HRESULT WINAPI Storage_OpenStream(IStorage *iface, LPCOLESTR pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm)
{
    static  const WCHAR ole1W[] = {1,'O','l','e',0};

    LARGE_INTEGER pos = {{0}};
    HRESULT hr;

    ok(!reserved1, "reserved1 = %p\n", reserved1);
    ok(!reserved2, "reserved2 = %lx\n", reserved2);
    ok(!!ppstm, "ppstm = NULL\n");

    if(!lstrcmpW(pwcsName, comp_objW)) {
        CHECK_EXPECT2(Storage_OpenStream_CompObj);
        ok(grfMode == STGM_SHARE_EXCLUSIVE, "grfMode = %lx\n", grfMode);

        *ppstm = comp_obj_stream;
        IStream_AddRef(comp_obj_stream);
        hr = IStream_Seek(comp_obj_stream, pos, STREAM_SEEK_SET, NULL);
        ok(hr == S_OK, "IStream_Seek returned %lx\n", hr);
        return S_OK;
    }else if(!lstrcmpW(pwcsName, ole1W)) {
        CHECK_EXPECT(Storage_OpenStream_Ole);

        if (!ole_stream)
        {
            ok(grfMode == (STGM_SHARE_EXCLUSIVE|STGM_READ), "grfMode = %lx\n", grfMode);

            *ppstm = NULL;
            return STG_E_FILENOTFOUND;
        }

        ok(grfMode == (STGM_SHARE_EXCLUSIVE|STGM_READWRITE), "grfMode = %lx\n", grfMode);

        *ppstm = ole_stream;
        IStream_AddRef(ole_stream);
        hr = IStream_Seek(ole_stream, pos, STREAM_SEEK_SET, NULL);
        ok(hr == S_OK, "IStream_Seek returned %lx\n", hr);
        return S_OK;

    }else if(!lstrcmpW(pwcsName, olepres0W)) {
        CHECK_EXPECT(Storage_OpenStream_OlePres);
        ok(grfMode == (STGM_SHARE_EXCLUSIVE|STGM_READWRITE), "grfMode = %lx\n", grfMode);

        *ppstm = olepres_stream;
        IStream_AddRef(olepres_stream);
        hr = IStream_Seek(olepres_stream, pos, STREAM_SEEK_SET, NULL);
        ok(hr == S_OK, "IStream_Seek returned %lx\n", hr);
        return S_OK;
    }

    ok(0, "unexpected call to OpenStream: %s\n", wine_dbgstr_w(pwcsName));
    return E_NOTIMPL;
}

static HRESULT WINAPI Storage_CreateStorage(IStorage *iface, LPCOLESTR pwcsName, DWORD grfMode, DWORD dwStgFmt, DWORD reserved2, IStorage **ppstg)
{
    ok(0, "unexpected call to CreateStorage\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Storage_OpenStorage(IStorage *iface, LPCOLESTR pwcsName, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg)
{
    ok(0, "unexpected call to OpenStorage\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Storage_CopyTo(IStorage *iface, DWORD ciidExclude, const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest)
{
    ok(0, "unexpected call to CopyTo\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Storage_MoveElementTo(IStorage *iface, LPCOLESTR pwcsName, IStorage *pstgDest, LPCOLESTR pwcsNewName, DWORD grfFlags)
{
    ok(0, "unexpected call to MoveElementTo\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Storage_Commit(IStorage *iface, DWORD grfCommitFlags)
{
    ok(0, "unexpected call to Commit\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Storage_Revert(IStorage *iface)
{
    ok(0, "unexpected call to Revert\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Storage_EnumElements(IStorage *iface, DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum)
{
    ok(0, "unexpected call to EnumElements\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Storage_DestroyElement(IStorage *iface, LPCOLESTR pwcsName)
{
    char name[32];
    int stream_n, cmp;

    CHECK_EXPECT2(Storage_DestroyElement);
    cmp = CompareStringW(LOCALE_NEUTRAL, 0, pwcsName, 8, olepres0W, 8);
    ok(cmp == CSTR_EQUAL,
       "unexpected call to DestroyElement(%s)\n", wine_dbgstr_w(pwcsName));

    WideCharToMultiByte(CP_ACP, 0, pwcsName, -1, name, sizeof(name), NULL, NULL);
    stream_n = atol(name + 8);
    if (stream_n <= Storage_DestroyElement_limit)
        return S_OK;

    return STG_E_FILENOTFOUND;
}

static HRESULT WINAPI Storage_RenameElement(IStorage *iface, LPCOLESTR pwcsOldName, LPCOLESTR pwcsNewName)
{
    ok(0, "unexpected call to RenameElement\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Storage_SetElementTimes(IStorage *iface, LPCOLESTR pwcsName, const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime)
{
    ok(0, "unexpected call to SetElementTimes\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Storage_SetClass(IStorage *iface, REFCLSID clsid)
{
    CHECK_EXPECT(Storage_SetClass);
    ok(IsEqualIID(clsid, Storage_SetClass_CLSID), "expected %s, got %s\n",
       wine_dbgstr_guid(Storage_SetClass_CLSID), wine_dbgstr_guid(clsid));
    return S_OK;
}

static HRESULT WINAPI Storage_SetStateBits(IStorage *iface, DWORD grfStateBits, DWORD grfMask)
{
    ok(0, "unexpected call to SetStateBits\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Storage_Stat(IStorage *iface, STATSTG *pstatstg, DWORD grfStatFlag)
{
    CHECK_EXPECT2(Storage_Stat);
    ok(pstatstg != NULL, "pstatstg = NULL\n");
    ok(grfStatFlag == STATFLAG_NONAME, "grfStatFlag = %lx\n", grfStatFlag);

    memset(pstatstg, 0, sizeof(STATSTG));
    pstatstg->type = STGTY_STORAGE;
    pstatstg->clsid = CLSID_WineTestOld;
    return S_OK;
}

static IStorageVtbl StorageVtbl =
{
    Storage_QueryInterface,
    Storage_AddRef,
    Storage_Release,
    Storage_CreateStream,
    Storage_OpenStream,
    Storage_CreateStorage,
    Storage_OpenStorage,
    Storage_CopyTo,
    Storage_MoveElementTo,
    Storage_Commit,
    Storage_Revert,
    Storage_EnumElements,
    Storage_DestroyElement,
    Storage_RenameElement,
    Storage_SetElementTimes,
    Storage_SetClass,
    Storage_SetStateBits,
    Storage_Stat
};

static IStorage Storage = { &StorageVtbl };

static void test_OleDoAutoConvert(void)
{
    static const WCHAR clsidW[] = {'C','L','S','I','D','\\',0};
    static struct {
        DWORD reserved1;
        DWORD version;
        DWORD reserved2[5];
        DWORD ansi_user_type_len;
        DWORD ansi_clipboard_format_len;
        DWORD reserved3;
        DWORD unicode_marker;
        DWORD unicode_user_type_len;
        DWORD unicode_clipboard_format_len;
        DWORD reserved4;
    } comp_obj_data;
    static struct {
        DWORD version;
        DWORD flags;
        DWORD link_update_option;
        DWORD reserved1;
        DWORD reserved_moniker_stream_size;
        DWORD relative_source_moniker_stream_size;
        DWORD absolute_source_moniker_stream_size;
        DWORD clsid_indicator;
        CLSID clsid;
        DWORD reserved_display_name;
        DWORD reserved2;
        DWORD local_update_time;
        DWORD local_check_update_time;
        DWORD remote_update_time;
    } ole_data;

    LARGE_INTEGER pos = {{0}};
    WCHAR buf[39+6];
    DWORD i, ret;
    HKEY root;
    CLSID clsid;
    HRESULT hr;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &comp_obj_stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal returned %lx\n", hr);
    hr = IStream_Write(comp_obj_stream, (char*)&comp_obj_data, sizeof(comp_obj_data), NULL);
    ok(hr == S_OK, "IStream_Write returned %lx\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &ole_stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal returned %lx\n", hr);
    hr = IStream_Write(ole_stream, (char*)&ole_data, sizeof(ole_data), NULL);
    ok(hr == S_OK, "IStream_Write returned %lx\n", hr);

    clsid = IID_WineTest;
    hr = OleDoAutoConvert(NULL, &clsid);
    ok(hr == E_INVALIDARG, "OleDoAutoConvert returned %lx\n", hr);
    ok(IsEqualIID(&clsid, &IID_NULL), "clsid = %s\n", wine_dbgstr_guid(&clsid));

    if(0) /* crashes on Win7 */
        OleDoAutoConvert(&Storage, NULL);

    clsid = IID_WineTest;
    SET_EXPECT(Storage_Stat);
    hr = OleDoAutoConvert(&Storage, &clsid);
    ok(hr == REGDB_E_CLASSNOTREG, "OleDoAutoConvert returned %lx\n", hr);
    CHECK_CALLED(Storage_Stat);
    ok(IsEqualIID(&clsid, &CLSID_WineTestOld), "clsid = %s\n", wine_dbgstr_guid(&clsid));

    lstrcpyW(buf, clsidW);
    StringFromGUID2(&CLSID_WineTestOld, buf+6, 39);

    ret = RegCreateKeyExW(HKEY_CLASSES_ROOT, buf, 0, NULL, 0,
            KEY_READ | KEY_WRITE | KEY_CREATE_SUB_KEY, NULL, &root, NULL);
    if(ret != ERROR_SUCCESS) {
        win_skip("not enough permissions to create CLSID key (%lu)\n", ret);
        return;
    }

    clsid = IID_WineTest;
    SET_EXPECT(Storage_Stat);
    hr = OleDoAutoConvert(&Storage, &clsid);
    ok(hr == REGDB_E_KEYMISSING, "OleDoAutoConvert returned %lx\n", hr);
    CHECK_CALLED(Storage_Stat);
    ok(IsEqualIID(&clsid, &CLSID_WineTestOld), "clsid = %s\n", wine_dbgstr_guid(&clsid));

    hr = OleSetAutoConvert(&CLSID_WineTestOld, &CLSID_WineTest);
    ok_ole_success(hr, "OleSetAutoConvert");

    hr = OleGetAutoConvert(&CLSID_WineTestOld, &clsid);
    ok_ole_success(hr, "OleGetAutoConvert");
    ok(IsEqualIID(&clsid, &CLSID_WineTest), "incorrect clsid: %s\n", wine_dbgstr_guid(&clsid));

    clsid = IID_WineTest;
    SET_EXPECT(Storage_Stat);
    SET_EXPECT(Storage_OpenStream_CompObj);
    SET_EXPECT(Storage_SetClass);
    SET_EXPECT(Storage_CreateStream_CompObj);
    SET_EXPECT(Storage_OpenStream_Ole);
    hr = OleDoAutoConvert(&Storage, &clsid);
    ok(hr == S_OK, "OleDoAutoConvert returned %lx\n", hr);
    CHECK_CALLED(Storage_Stat);
    CHECK_CALLED(Storage_OpenStream_CompObj);
    CHECK_CALLED(Storage_SetClass);
    CHECK_CALLED(Storage_CreateStream_CompObj);
    CHECK_CALLED(Storage_OpenStream_Ole);
    ok(IsEqualIID(&clsid, &CLSID_WineTest), "clsid = %s\n", wine_dbgstr_guid(&clsid));

    hr = IStream_Seek(comp_obj_stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek returned %lx\n", hr);
    hr = IStream_Read(comp_obj_stream, &comp_obj_data, sizeof(comp_obj_data), NULL);
    ok(hr == S_OK, "IStream_Read returned %lx\n", hr);
    ok(comp_obj_data.reserved1 == 0xfffe0001, "reserved1 = %lx\n", comp_obj_data.reserved1);
    ok(comp_obj_data.version == 0xa03, "version = %lx\n", comp_obj_data.version);
    ok(comp_obj_data.reserved2[0] == -1, "reserved2[0] = %lx\n", comp_obj_data.reserved2[0]);
    ok(IsEqualIID(comp_obj_data.reserved2+1, &CLSID_WineTestOld), "reserved2 = %s\n", wine_dbgstr_guid((CLSID*)(comp_obj_data.reserved2+1)));
    ok(!comp_obj_data.ansi_user_type_len, "ansi_user_type_len = %ld\n", comp_obj_data.ansi_user_type_len);
    ok(!comp_obj_data.ansi_clipboard_format_len, "ansi_clipboard_format_len = %ld\n", comp_obj_data.ansi_clipboard_format_len);
    ok(!comp_obj_data.reserved3, "reserved3 = %lx\n", comp_obj_data.reserved3);
    ok(comp_obj_data.unicode_marker == 0x71b239f4, "unicode_marker = %lx\n", comp_obj_data.unicode_marker);
    ok(!comp_obj_data.unicode_user_type_len, "unicode_user_type_len = %ld\n", comp_obj_data.unicode_user_type_len);
    ok(!comp_obj_data.unicode_clipboard_format_len, "unicode_clipboard_format_len = %ld\n", comp_obj_data.unicode_clipboard_format_len);
    ok(!comp_obj_data.reserved4, "reserved4 %ld\n", comp_obj_data.reserved4);

    hr = IStream_Seek(ole_stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek returned %lx\n", hr);
    hr = IStream_Read(ole_stream, &ole_data, sizeof(ole_data), NULL);
    ok(hr == S_OK, "IStream_Read returned %lx\n", hr);
    ok(ole_data.version == 0, "version = %lx\n", ole_data.version);
    ok(ole_data.flags == 4, "flags = %lx\n", ole_data.flags);
    for(i=2; i<sizeof(ole_data)/sizeof(DWORD); i++)
        ok(((DWORD*)&ole_data)[i] == 0, "ole_data[%ld] = %lx\n", i, ((DWORD*)&ole_data)[i]);

    SET_EXPECT(Storage_OpenStream_Ole);
    hr = SetConvertStg(&Storage, TRUE);
    ok(hr == S_OK, "SetConvertStg returned %lx\n", hr);
    CHECK_CALLED(Storage_OpenStream_Ole);

    SET_EXPECT(Storage_OpenStream_CompObj);
    SET_EXPECT(Storage_Stat);
    SET_EXPECT(Storage_CreateStream_CompObj);
    hr = WriteFmtUserTypeStg(&Storage, 0, NULL);
    ok(hr == S_OK, "WriteFmtUserTypeStg returned %lx\n", hr);
    todo_wine CHECK_CALLED(Storage_OpenStream_CompObj);
    CHECK_CALLED(Storage_Stat);
    CHECK_CALLED(Storage_CreateStream_CompObj);
    hr = IStream_Seek(comp_obj_stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek returned %lx\n", hr);
    hr = IStream_Read(comp_obj_stream, &comp_obj_data, sizeof(comp_obj_data), NULL);
    ok(hr == S_OK, "IStream_Read returned %lx\n", hr);
    ok(comp_obj_data.reserved1 == 0xfffe0001, "reserved1 = %lx\n", comp_obj_data.reserved1);
    ok(comp_obj_data.version == 0xa03, "version = %lx\n", comp_obj_data.version);
    ok(comp_obj_data.reserved2[0] == -1, "reserved2[0] = %lx\n", comp_obj_data.reserved2[0]);
    ok(IsEqualIID(comp_obj_data.reserved2+1, &CLSID_WineTestOld), "reserved2 = %s\n", wine_dbgstr_guid((CLSID*)(comp_obj_data.reserved2+1)));
    ok(!comp_obj_data.ansi_user_type_len, "ansi_user_type_len = %ld\n", comp_obj_data.ansi_user_type_len);
    ok(!comp_obj_data.ansi_clipboard_format_len, "ansi_clipboard_format_len = %ld\n", comp_obj_data.ansi_clipboard_format_len);
    ok(!comp_obj_data.reserved3, "reserved3 = %lx\n", comp_obj_data.reserved3);
    ok(comp_obj_data.unicode_marker == 0x71b239f4, "unicode_marker = %lx\n", comp_obj_data.unicode_marker);
    ok(!comp_obj_data.unicode_user_type_len, "unicode_user_type_len = %ld\n", comp_obj_data.unicode_user_type_len);
    ok(!comp_obj_data.unicode_clipboard_format_len, "unicode_clipboard_format_len = %ld\n", comp_obj_data.unicode_clipboard_format_len);
    ok(!comp_obj_data.reserved4, "reserved4 %ld\n", comp_obj_data.reserved4);

    ret = IStream_Release(comp_obj_stream);
    ok(!ret, "comp_obj_stream was not freed\n");
    ret = IStream_Release(ole_stream);
    ok(!ret, "ole_stream was not freed\n");

    ret = RegDeleteKeyA(root, "AutoConvertTo");
    ok(ret == ERROR_SUCCESS, "RegDeleteKey error %lu\n", ret);
    ret = RegDeleteKeyA(root, "");
    ok(ret == ERROR_SUCCESS, "RegDeleteKey error %lu\n", ret);
    RegCloseKey(root);
}

/* 1x1 pixel bmp */
static const unsigned char bmpimage[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,
    0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,
    0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xff,0xff,
    0xff,0x00,0xff,0xff,0xff,0x00,0x00,0x00,
    0x00,0x00
};

static const unsigned char mf_blank_bits[] =
{
    0x01,0x00,0x09,0x00,0x00,0x03,0x0c,0x00,
    0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00
};

static void test_data_cache_save(void)
{
    static const WCHAR contentsW[] = { 'C','o','n','t','e','n','t','s',0 };
    HRESULT hr;
    ILockBytes *ilb;
    IStorage *doc;
    IStream *stm;
    IOleCache2 *cache;
    IPersistStorage *stg;
    DWORD clipformat[2];
    PresentationDataHeader hdr;

    hr = CreateILockBytesOnHGlobal(0, TRUE, &ilb);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    hr = StgCreateDocfileOnILockBytes(ilb, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0,  &doc);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    ILockBytes_Release(ilb);

    hr = IStorage_SetClass(doc, &CLSID_WineTest);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    hr = IStorage_CreateStream(doc, contentsW, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    hr = IStream_Write(stm, bmpimage, sizeof(bmpimage), NULL);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    IStream_Release(stm);

    hr = IStorage_CreateStream(doc, olepres0W, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    clipformat[0] = -1;
    clipformat[1] = CF_METAFILEPICT;
    hr = IStream_Write(stm, clipformat, sizeof(clipformat), NULL);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    hdr.tdSize = sizeof(hdr.tdSize);
    hdr.dvAspect = DVASPECT_CONTENT;
    hdr.lindex = -1;
    hdr.advf = ADVF_PRIMEFIRST;
    hdr.unknown7 = 0;
    hdr.dwObjectExtentX = 0;
    hdr.dwObjectExtentY = 0;
    hdr.dwSize = sizeof(mf_blank_bits);
    hr = IStream_Write(stm, &hdr, sizeof(hdr), NULL);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    hr = IStream_Write(stm, mf_blank_bits, sizeof(mf_blank_bits), NULL);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    IStream_Release(stm);

    hr = CreateDataCache(NULL, &CLSID_NULL, &IID_IUnknown, (void **)&cache);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    hr = IOleCache2_QueryInterface(cache, &IID_IPersistStorage, (void **)&stg);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    hr = IPersistStorage_Load(stg, doc);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    IStorage_Release(doc);

    hr = IPersistStorage_IsDirty(stg);
    ok(hr == S_FALSE, "unexpected %#lx\n", hr);

    ole_stream = NULL;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &olepres_stream);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    /* FIXME: remove this stream once Wine is fixed */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &contents_stream);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    SET_EXPECT(Storage_CreateStream_OlePres);
    SET_EXPECT(Storage_OpenStream_OlePres);
    SET_EXPECT(Storage_OpenStream_Ole);
    SET_EXPECT(Storage_DestroyElement);
    Storage_DestroyElement_limit = 50;
    Storage_SetClass_CLSID = &CLSID_NULL;
    hr = IPersistStorage_Save(stg, &Storage, FALSE);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    CHECK_CALLED(Storage_CreateStream_OlePres);
    todo_wine
    CHECK_CALLED(Storage_OpenStream_OlePres);
    todo_wine
    CHECK_CALLED(Storage_OpenStream_Ole);
    todo_wine
    CHECK_CALLED(Storage_DestroyElement);

    IStream_Release(olepres_stream);
    IStream_Release(contents_stream);

    IPersistStorage_Release(stg);
    IOleCache2_Release(cache);
}

#define MAX_STREAM 16

struct stream_def
{
    const char *name;
    int cf;
    DVASPECT dvAspect;
    ADVF advf;
    const void *data;
    size_t data_size;
};

struct storage_def
{
    const CLSID *clsid;
    int stream_count;
    struct stream_def stream[MAX_STREAM];
};

static const struct storage_def stg_def_0 =
{
    &CLSID_NULL, 1,
    {{ "Contents", -1, 0, 0, bmpimage, sizeof(bmpimage) }}
};
static const struct storage_def stg_def_0_saved =
{
    &CLSID_NULL, 0, {{ 0 }}
};
static const struct storage_def stg_def_1 =
{
    &CLSID_NULL, 2,
    {{ "Contents", -1, 0, 0, NULL, 0 },
    { "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 }}
};
static const struct storage_def stg_def_1_saved =
{
    &CLSID_NULL, 1,
    {{ "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 }}
};
static const struct storage_def stg_def_2 =
{
    &CLSID_ManualResetEvent, 2,
    {{ "Contents", -1, 0, 0, bmpimage, sizeof(bmpimage) },
    { "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 }}
};
static const struct storage_def stg_def_2_saved =
{
    &CLSID_NULL, 1,
    {{ "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 }}
};
static const struct storage_def stg_def_3 =
{
    &CLSID_NULL, 5,
    {{ "Contents", -1, 0, 0, bmpimage, sizeof(bmpimage) },
    { "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 },
    { "\2OlePres001", CF_METAFILEPICT, DVASPECT_CONTENT, ADVF_PRIMEFIRST, mf_blank_bits, sizeof(mf_blank_bits) },
    { "\2OlePres002", CF_DIB, DVASPECT_CONTENT, ADVF_PRIMEFIRST, bmpimage, sizeof(bmpimage) },
    { "MyStream", -1, 0, 0, "Hello World!", 13 }}
};
static const struct storage_def stg_def_3_saved =
{
    &CLSID_NULL, 3,
    {{ "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 },
    { "\2OlePres001", CF_METAFILEPICT, DVASPECT_CONTENT, ADVF_PRIMEFIRST, mf_blank_bits, sizeof(mf_blank_bits) },
    { "\2OlePres002", CF_DIB, DVASPECT_CONTENT, ADVF_PRIMEFIRST, bmpimage, sizeof(bmpimage) }}
};
static const struct storage_def stg_def_4 =
{
    &CLSID_Picture_EnhMetafile, 5,
    {{ "Contents", -1, 0, 0, bmpimage, sizeof(bmpimage) },
    { "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 },
    { "\2OlePres001", CF_METAFILEPICT, DVASPECT_CONTENT, ADVF_PRIMEFIRST, mf_blank_bits, sizeof(mf_blank_bits) },
    { "\2OlePres002", CF_DIB, DVASPECT_CONTENT, ADVF_PRIMEFIRST, bmpimage, sizeof(bmpimage) },
    { "MyStream", -1, 0, 0, "Hello World!", 13 }}
};
static const struct storage_def stg_def_4_saved =
{
    &CLSID_NULL, 1,
    {{ "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 }}
};
static const struct storage_def stg_def_5 =
{
    &CLSID_Picture_Dib, 5,
    {{ "Contents", -1, 0, 0, bmpimage, sizeof(bmpimage) },
    { "\2OlePres002", CF_DIB, DVASPECT_CONTENT, ADVF_PRIMEFIRST, bmpimage, sizeof(bmpimage) },
    { "\2OlePres001", CF_METAFILEPICT, DVASPECT_CONTENT, ADVF_PRIMEFIRST, mf_blank_bits, sizeof(mf_blank_bits) },
    { "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 },
    { "MyStream", -1, 0, 0, "Hello World!", 13 }}
};
static const struct storage_def stg_def_5_saved =
{
    &CLSID_NULL, 1,
    {{ "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 }}
};
static const struct storage_def stg_def_6 =
{
    &CLSID_Picture_Metafile, 5,
    {{ "Contents", -1, 0, 0, bmpimage, sizeof(bmpimage) },
    { "\2OlePres001", CF_METAFILEPICT, DVASPECT_CONTENT, ADVF_PRIMEFIRST, mf_blank_bits, sizeof(mf_blank_bits) },
    { "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 },
    { "\2OlePres002", CF_DIB, DVASPECT_CONTENT, ADVF_PRIMEFIRST, bmpimage, sizeof(bmpimage) },
    { "MyStream", -1, 0, 0, "Hello World!", 13 }}
};
static const struct storage_def stg_def_6_saved =
{
    &CLSID_NULL, 1,
    {{ "\2OlePres000", 0, DVASPECT_ICON, ADVF_PRIMEFIRST | ADVF_ONLYONCE, NULL, 0 }}
};
static const struct storage_def stg_def_7 =
{
    &CLSID_Picture_Dib, 1,
    {{ "Contents", -1, 0, 0, bmpimage, sizeof(bmpimage) }}
};
static const struct storage_def stg_def_7_saved =
{
    &CLSID_NULL, 0, {{ 0 }}
};
static const struct storage_def stg_def_8 =
{
    &CLSID_Picture_Metafile, 1,
    {{ "Contents", -1, 0, 0, mf_blank_bits, sizeof(mf_blank_bits) }}
};
static const struct storage_def stg_def_8_saved =
{
    &CLSID_NULL, 0, {{ 0 }}
};
static const struct storage_def stg_def_9 =
{
    &CLSID_Picture_EnhMetafile, 1,
    {{ "Contents", -1, 0, 0, bmpimage, sizeof(bmpimage) }}
};
static const struct storage_def stg_def_9_saved =
{
    &CLSID_NULL, 0, {{ 0 }}
};

static int read_clipformat(IStream *stream)
{
    HRESULT hr;
    ULONG bytes;
    int length, clipformat = -2;

    hr = IStream_Read(stream, &length, sizeof(length), &bytes);
    if (hr != S_OK || bytes != sizeof(length))
        return -2;
    if (length == 0)
        return 0;
    if (length == -1)
    {
        hr = IStream_Read(stream, &clipformat, sizeof(clipformat), &bytes);
        if (hr != S_OK || bytes != sizeof(clipformat))
            return -2;
    }
    else
        ok(0, "unhandled clipformat length %d\n", length);

    return clipformat;
}

static void check_storage_contents(IStorage *stg, const struct storage_def *stg_def,
        int *enumerated_streams, int *matched_streams)
{
    HRESULT hr;
    IEnumSTATSTG *enumstg;
    IStream *stream;
    STATSTG stat;
    int i, seen_stream[MAX_STREAM] = { 0 };

    if (winetest_debug > 1)
        trace("check_storage_contents:\n=============================================\n");

    *enumerated_streams = 0;
    *matched_streams = 0;

    hr = IStorage_Stat(stg, &stat, STATFLAG_NONAME);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    ok(IsEqualCLSID(stg_def->clsid, &stat.clsid), "expected %s, got %s\n",
       wine_dbgstr_guid(stg_def->clsid), wine_dbgstr_guid(&stat.clsid));

    hr = IStorage_EnumElements(stg, 0, NULL, 0, &enumstg);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    for (;;)
    {
        ULONG bytes;
        int clipformat = -1;
        PresentationDataHeader header;
        char name[32];
        BYTE data[1024];

        memset(&header, 0, sizeof(header));

        hr = IEnumSTATSTG_Next(enumstg, 1, &stat, NULL);
        if(hr == S_FALSE) break;
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        if (winetest_debug > 1)
            trace("name %s, type %lu, size %ld, clsid %s\n",
                wine_dbgstr_w(stat.pwcsName), stat.type, stat.cbSize.u.LowPart, wine_dbgstr_guid(&stat.clsid));

        ok(stat.type == STGTY_STREAM, "unexpected %#lx\n", stat.type);

        WideCharToMultiByte(CP_ACP, 0, stat.pwcsName, -1, name, sizeof(name), NULL, NULL);

        hr = IStorage_OpenStream(stg, stat.pwcsName, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stream);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        if (!memcmp(name, "\2OlePres", 8))
        {
            ULONG header_size = sizeof(header);

            clipformat = read_clipformat(stream);

            if (clipformat == 0) /* view cache */
                header_size = FIELD_OFFSET(PresentationDataHeader, unknown7);

            hr = IStream_Read(stream, &header, header_size, &bytes);
            ok(hr == S_OK, "unexpected %#lx\n", hr);
            ok(bytes == header_size, "read %lu bytes, expected %lu\n", bytes, header_size);

            if (winetest_debug > 1)
                trace("header: tdSize %#lx, dvAspect %#x, lindex %#lx, advf %#lx, unknown7 %#lx, dwObjectExtentX %#lx, dwObjectExtentY %#lx, dwSize %#lx\n",
                    header.tdSize, header.dvAspect, header.lindex, header.advf, header.unknown7,
                    header.dwObjectExtentX, header.dwObjectExtentY, header.dwSize);
        }

        memset(data, 0, sizeof(data));
        hr = IStream_Read(stream, data, sizeof(data), &bytes);
        ok(hr == S_OK, "unexpected %#lx\n", hr);
        if (winetest_debug > 1)
            trace("stream data (%lu bytes): %02x %02x %02x %02x\n", bytes, data[0], data[1], data[2], data[3]);

        for (i = 0; i < stg_def->stream_count; i++)
        {
            if (seen_stream[i]) continue;

            if (winetest_debug > 1)
                trace("%s/%s, %d/%d, %d/%d, %d/%ld\n",
                    stg_def->stream[i].name, name,
                    stg_def->stream[i].cf, clipformat,
                    stg_def->stream[i].dvAspect, header.dvAspect,
                    stg_def->stream[i].advf, header.advf);

            if (!strcmp(stg_def->stream[i].name, name) &&
                stg_def->stream[i].cf == clipformat &&
                stg_def->stream[i].dvAspect == header.dvAspect &&
                stg_def->stream[i].advf == header.advf &&
                stg_def->stream[i].data_size <= bytes &&
                (!stg_def->stream[i].data_size ||
                    (!memcmp(stg_def->stream[i].data, data, min(stg_def->stream[i].data_size, bytes)))))
            {
                if (winetest_debug > 1)
                    trace("stream %d matches def stream %d\n", *enumerated_streams, i);
                seen_stream[i] = 1;
                *matched_streams += 1;
            }
        }

        CoTaskMemFree(stat.pwcsName);
        IStream_Release(stream);

        *enumerated_streams += 1;
    }

    IEnumSTATSTG_Release(enumstg);
}

static HRESULT stgmedium_cmp(const STGMEDIUM *med1, STGMEDIUM *med2)
{
    BYTE *data1, *data2;
    ULONG datasize1, datasize2;

    if (med1->tymed != med2->tymed)
        return E_FAIL;

    if (med1->tymed == TYMED_MFPICT)
    {
        METAFILEPICT *mfpict1 = GlobalLock(med1->hMetaFilePict);
        METAFILEPICT *mfpict2 = GlobalLock(med2->hMetaFilePict);

        datasize1 = GetMetaFileBitsEx(mfpict1->hMF, 0, NULL);
        datasize2 = GetMetaFileBitsEx(mfpict2->hMF, 0, NULL);
        if (datasize1 == datasize2)
        {
            data1 = malloc(datasize1);
            data2 = malloc(datasize2);
            GetMetaFileBitsEx(mfpict1->hMF, datasize1, data1);
            GetMetaFileBitsEx(mfpict2->hMF, datasize2, data2);
        }
        else return E_FAIL;
    }
    else if (med1->tymed == TYMED_ENHMF)
    {
        datasize1 = GetEnhMetaFileBits(med1->hEnhMetaFile, 0, NULL);
        datasize2 = GetEnhMetaFileBits(med2->hEnhMetaFile, 0, NULL);
        if (datasize1 == datasize2)
        {
            data1 = malloc(datasize1);
            data2 = malloc(datasize2);
            GetEnhMetaFileBits(med1->hEnhMetaFile, datasize1, data1);
            GetEnhMetaFileBits(med2->hEnhMetaFile, datasize2, data2);
        }
        else return E_FAIL;
    }
    else if (med1->tymed == TYMED_HGLOBAL)
    {
        datasize1 = GlobalSize(med1->hGlobal);
        datasize2 = GlobalSize(med2->hGlobal);

        if (datasize1 == datasize2)
        {
            data1 = GlobalLock(med1->hGlobal);
            data2 = GlobalLock(med2->hGlobal);
        }
        else
            return E_FAIL;
    }
    else
        return E_NOTIMPL;

    if (memcmp(data1, data2, datasize1) != 0)
        return E_FAIL;

    if (med1->tymed == TYMED_HGLOBAL)
    {
        GlobalUnlock(med1->hGlobal);
        GlobalUnlock(med2->hGlobal);
    }
    else if (med1->tymed == TYMED_MFPICT)
    {
        free(data1);
        free(data2);
        GlobalUnlock(med1->hMetaFilePict);
        GlobalUnlock(med2->hMetaFilePict);
    }
    else
    {
        free(data1);
        free(data2);
    }

    return S_OK;
}

static IStorage *create_storage_from_def(const struct storage_def *stg_def)
{
    HRESULT hr;
    IStorage *stg;
    IStream *stm;
    int i;

    hr = StgCreateDocfile(NULL, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &stg);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    hr = IStorage_SetClass(stg, stg_def->clsid);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    for (i = 0; i < stg_def->stream_count; i++)
    {
        WCHAR name[32];

        MultiByteToWideChar(CP_ACP, 0, stg_def->stream[i].name, -1, name, 32);
        hr = IStorage_CreateStream(stg, name, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        if (stg_def->stream[i].cf != -1)
        {
            int clipformat[2];
            PresentationDataHeader hdr;

            if (stg_def->stream[i].cf)
            {
                clipformat[0] = -1;
                clipformat[1] = stg_def->stream[i].cf;
                hr = IStream_Write(stm, clipformat, sizeof(clipformat), NULL);
            }
            else
            {
                clipformat[0] = 0;
                hr = IStream_Write(stm, &clipformat[0], sizeof(clipformat[0]), NULL);
            }
            ok(hr == S_OK, "unexpected %#lx\n", hr);

            hdr.tdSize = sizeof(hdr.tdSize);
            hdr.dvAspect = stg_def->stream[i].dvAspect;
            hdr.lindex = -1;
            hdr.advf = stg_def->stream[i].advf;
            hdr.unknown7 = 0;
            hdr.dwObjectExtentX = 0;
            hdr.dwObjectExtentY = 0;
            hdr.dwSize = stg_def->stream[i].data_size;
            hr = IStream_Write(stm, &hdr, sizeof(hdr), NULL);
            ok(hr == S_OK, "unexpected %#lx\n", hr);
        }

        if (stg_def->stream[i].data_size)
        {
            hr = IStream_Write(stm, stg_def->stream[i].data, stg_def->stream[i].data_size, NULL);
            ok(hr == S_OK, "unexpected %#lx\n", hr);
        }

        IStream_Release(stm);
    }

    return stg;
}

static const BYTE dib_inf[] =
{
    0x42, 0x4d, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x36, 0x00, 0x00, 0x00
};

static const BYTE mf_rec[] =
{
    0xd7, 0xcd, 0xc6, 0x9a, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x16, 0x00, 0x2d, 0x00, 0x40, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x6a, 0x55
};

static void get_stgdef(struct storage_def *stg_def, CLIPFORMAT cf, STGMEDIUM *stg_med, int stm_idx)
{
    BYTE *data;
    int data_size;
    METAFILEPICT *mfpict;
    HDC hdc;

    switch (cf)
    {
    case CF_DIB:
        data_size = sizeof(dib_white);
        if (!strcmp(stg_def->stream[stm_idx].name, "CONTENTS"))
        {
            data_size += sizeof(dib_inf);
            data = malloc(data_size);
            memcpy(data, dib_inf, sizeof(dib_inf));
            memcpy(data + sizeof(dib_inf), dib_white, sizeof(dib_white));
        }
        else
        {
            data = malloc(data_size);
            memcpy(data, dib_white, sizeof(dib_white));
        }
        stg_def->stream[stm_idx].data = data;
        stg_def->stream[stm_idx].data_size = data_size;
        break;
    case CF_METAFILEPICT:
        mfpict = GlobalLock(stg_med->hMetaFilePict);
        data_size = GetMetaFileBitsEx(mfpict->hMF, 0, NULL);
        if (!strcmp(stg_def->stream[stm_idx].name, "CONTENTS"))
        {
            data = malloc(data_size + sizeof(mf_rec));
            memcpy(data, mf_rec, sizeof(mf_rec));
            GetMetaFileBitsEx(mfpict->hMF, data_size, data + sizeof(mf_rec));
            data_size += sizeof(mf_rec);
        }
        else
        {
            data = malloc(data_size);
            GetMetaFileBitsEx(mfpict->hMF, data_size, data);
        }
        GlobalUnlock(stg_med->hMetaFilePict);
        stg_def->stream[stm_idx].data_size = data_size;
        stg_def->stream[stm_idx].data = data;
        break;
    case CF_ENHMETAFILE:
        if (!strcmp(stg_def->stream[stm_idx].name, "CONTENTS"))
        {
            data_size = GetEnhMetaFileBits(stg_med->hEnhMetaFile, 0, NULL);
            data = malloc(sizeof(DWORD) + sizeof(ENHMETAHEADER) + data_size);
            *((DWORD *)data) = sizeof(ENHMETAHEADER);
            GetEnhMetaFileBits(stg_med->hEnhMetaFile, data_size, data + sizeof(DWORD) + sizeof(ENHMETAHEADER));
            memcpy(data + sizeof(DWORD), data + sizeof(DWORD) + sizeof(ENHMETAHEADER), sizeof(ENHMETAHEADER));
            data_size += sizeof(DWORD) + sizeof(ENHMETAHEADER);
        }
        else
        {
            hdc = GetDC(NULL);
            data_size = GetWinMetaFileBits(stg_med->hEnhMetaFile, 0, NULL, MM_ANISOTROPIC, hdc);
            data = malloc(data_size);
            GetWinMetaFileBits(stg_med->hEnhMetaFile, data_size, data, MM_ANISOTROPIC, hdc);
            ReleaseDC(NULL, hdc);
        }
        stg_def->stream[stm_idx].data_size = data_size;
        stg_def->stream[stm_idx].data = data;
        break;
    }
}

static void get_stgmedium(CLIPFORMAT cfFormat, STGMEDIUM *stgmedium)
{
    switch (cfFormat)
    {
    case CF_DIB:
        create_dib(stgmedium);
        break;
    case CF_METAFILEPICT:
        create_mfpict(stgmedium);
        break;
    case CF_ENHMETAFILE:
        create_emf(stgmedium);
        break;
    default:
        ok(0, "cf %x not implemented\n", cfFormat);
    }
}

#define MAX_FMTS 5
static void test_data_cache_save_data(void)
{
    HRESULT hr;
    STGMEDIUM stgmed;
    ILockBytes *ilb;
    IStorage *doc;
    IOleCache2 *cache;
    IPersistStorage *persist;
    IDataObject *odata;
    int enumerated_streams, matched_streams, i;
    DWORD dummy;
    STGMEDIUM stgmeds[MAX_FMTS];
    struct tests_data_cache
    {
        FORMATETC fmts[MAX_FMTS];
        int num_fmts, num_set;
        const CLSID *clsid;
        struct storage_def stg_def;
    };

    static struct tests_data_cache *pdata, data[] =
    {
        {
            {
                { CF_DIB, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
                { CF_METAFILEPICT, 0, DVASPECT_CONTENT, -1, TYMED_MFPICT },
                { CF_ENHMETAFILE, 0, DVASPECT_CONTENT, -1, TYMED_ENHMF },
                { 0, 0, DVASPECT_DOCPRINT, -1, TYMED_HGLOBAL },
            },
            4, 3, &CLSID_WineTest,
            {
                &CLSID_WineTestOld, 4, { { "\2OlePres000", CF_DIB, DVASPECT_CONTENT, 0, NULL, 0 },
                                         { "\2OlePres001", CF_METAFILEPICT, DVASPECT_CONTENT, 0, NULL, 0 },
                                         { "\2OlePres002", CF_ENHMETAFILE, DVASPECT_CONTENT, 0, NULL, 0 },
                                         { "\2OlePres003", 0, DVASPECT_DOCPRINT, 0, NULL, 0 } }
            }
        },
        /* without setting data */
        {
            {
                { CF_DIB, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
                { CF_METAFILEPICT, 0, DVASPECT_CONTENT, -1, TYMED_MFPICT },
                { CF_ENHMETAFILE, 0, DVASPECT_CONTENT, -1, TYMED_ENHMF },
            },
            3, 0, &CLSID_WineTest,
            {
                &CLSID_WineTestOld, 3, { { "\2OlePres000", CF_DIB, DVASPECT_CONTENT, 0, NULL, 0 },
                                         { "\2OlePres001", CF_METAFILEPICT, DVASPECT_CONTENT, 0, NULL, 0 },
                                         { "\2OlePres002", CF_ENHMETAFILE, DVASPECT_CONTENT, 0, NULL, 0 } }
            }
        },
        /* static picture clsids */
        {
            {
                { CF_DIB, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            },
            1, 1, &CLSID_Picture_Dib,
            {
                &CLSID_WineTestOld, 1, { { "CONTENTS", -1, 0, 0, NULL, 0 } }
            }
        },
        {
            {
                { CF_METAFILEPICT, 0, DVASPECT_CONTENT, -1, TYMED_MFPICT },
            },
            1, 1, &CLSID_Picture_Metafile,
            {
                &CLSID_WineTestOld, 1, { { "CONTENTS", -1, 0, 0, NULL, 0 } }
            }
        },
        {
            {
                { CF_ENHMETAFILE, 0, DVASPECT_CONTENT, -1, TYMED_ENHMF },
            },
            1, 1, &CLSID_Picture_EnhMetafile,
            {
                &CLSID_WineTestOld, 1, { { "CONTENTS", -1, 0, 0, NULL, 0 } }
            }
        },
        /* static picture clsids without setting any data */
        {
            {
                { CF_DIB, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            },
            1, 0, &CLSID_Picture_Dib,
            {
                &CLSID_WineTestOld, 1, { { "CONTENTS", -1, 0, 0, NULL, 0 } }
            }
        },
        {
            {
                { CF_METAFILEPICT, 0, DVASPECT_CONTENT, -1, TYMED_MFPICT },
            },
            1, 0, &CLSID_Picture_Metafile,
            {
                &CLSID_WineTestOld, 1, { { "CONTENTS", -1, 0, 0, NULL, 0 } }
            }
        },
        {
            {
                { CF_ENHMETAFILE, 0, DVASPECT_CONTENT, -1, TYMED_ENHMF },
            },
            1, 0, &CLSID_Picture_EnhMetafile,
            {
                &CLSID_WineTestOld, 1, { { "CONTENTS", -1, 0, 0, NULL, 0 } }
            }
        },
        {
            {
                { 0 }
            }
        }
    };

    /* test _Save after caching directly through _Cache + _SetData */
    for (pdata = data; pdata->clsid != NULL; pdata++)
    {
        hr = CreateDataCache(NULL, pdata->clsid, &IID_IOleCache2, (void **)&cache);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        for (i = 0; i < pdata->num_fmts; i++)
        {
            hr = IOleCache2_Cache(cache, &pdata->fmts[i], 0, &dummy);
            ok(SUCCEEDED(hr), "unexpected %#lx\n", hr);
            if (i < pdata->num_set)
            {
                get_stgmedium(pdata->fmts[i].cfFormat, &stgmeds[i]);
                get_stgdef(&pdata->stg_def, pdata->fmts[i].cfFormat, &stgmeds[i], i);
                hr = IOleCache2_SetData(cache, &pdata->fmts[i], &stgmeds[i], FALSE);
                ok(hr == S_OK, "unexpected %#lx\n", hr);
            }
        }

        /* create Storage in memory where we'll save cache */
        hr = CreateILockBytesOnHGlobal(0, TRUE, &ilb);
        ok(hr == S_OK, "unexpected %#lx\n", hr);
        hr = StgCreateDocfileOnILockBytes(ilb, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &doc);
        ok(hr == S_OK, "unexpected %#lx\n", hr);
        ILockBytes_Release(ilb);
        hr = IStorage_SetClass(doc, &CLSID_WineTestOld);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        hr = IOleCache2_QueryInterface(cache, &IID_IPersistStorage, (void **)&persist);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        /* cache entries are dirty. test saving them to stg */
        hr = IPersistStorage_Save(persist, doc, FALSE);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        hr = IPersistStorage_IsDirty(persist);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        check_storage_contents(doc, &pdata->stg_def, &enumerated_streams, &matched_streams);
        ok(enumerated_streams == matched_streams, "enumerated %d != matched %d\n",
           enumerated_streams, matched_streams);
        ok(enumerated_streams == pdata->stg_def.stream_count, "created %d != def streams %d\n",
           enumerated_streams, pdata->stg_def.stream_count);

        IPersistStorage_Release(persist);
        IOleCache2_Release(cache);

        /* now test _Load/_GetData using the storage we used for _Save */
        hr = CreateDataCache(NULL, pdata->clsid, &IID_IOleCache2, (void **)&cache);
        ok(hr == S_OK, "unexpected %#lx\n", hr);
        hr = IOleCache2_QueryInterface(cache, &IID_IPersistStorage, (void **)&persist);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        hr = IStorage_SetClass(doc, pdata->clsid);
        ok(hr == S_OK, "unexpected %#lx\n", hr);
        hr = IPersistStorage_Load(persist, doc);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        hr = IOleCache2_QueryInterface(cache, &IID_IDataObject, (void **)&odata);
        ok(hr == S_OK, "unexpected %#lx\n", hr);
        for (i = 0; i < pdata->num_set; i++)
        {
            hr = IDataObject_GetData(odata, &pdata->fmts[i], &stgmed);
            ok(hr == S_OK, "unexpected %#lx\n", hr);

            hr = stgmedium_cmp(&stgmeds[i], &stgmed);
            ok(hr == S_OK, "unexpected %#lx\n", hr);
            ReleaseStgMedium(&stgmed);
            ReleaseStgMedium(&stgmeds[i]);
        }

        IDataObject_Release(odata);
        IPersistStorage_Release(persist);
        IStorage_Release(doc);
        IOleCache2_Release(cache);
        for (i = 0; i < pdata->num_set; i++)
            free((void *)pdata->stg_def.stream[i].data);

    }
}

static void test_data_cache_contents(void)
{
    HRESULT hr;
    IStorage *doc1, *doc2;
    IOleCache2 *cache;
    IPersistStorage *stg;
    int i, enumerated_streams, matched_streams;
    static const struct
    {
        const struct storage_def *in;
        const struct storage_def *out;
    } test_data[] =
    {
        { &stg_def_0, &stg_def_0_saved },
        { &stg_def_1, &stg_def_1_saved },
        { &stg_def_2, &stg_def_2_saved },
        { &stg_def_3, &stg_def_3_saved },
        { &stg_def_4, &stg_def_4_saved },
        { &stg_def_5, &stg_def_5_saved },
        { &stg_def_6, &stg_def_6_saved },
        { &stg_def_7, &stg_def_7_saved },
        { &stg_def_8, &stg_def_8_saved },
        { &stg_def_9, &stg_def_9_saved },
    };

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        if (winetest_debug > 1)
            trace("start testing storage def %d\n", i);

        doc1 = create_storage_from_def(test_data[i].in);
        if (!doc1) continue;

        enumerated_streams = matched_streams = -1;
        check_storage_contents(doc1, test_data[i].in, &enumerated_streams, &matched_streams);
        ok(enumerated_streams == matched_streams, "%d in: enumerated %d != matched %d\n", i,
           enumerated_streams, matched_streams);
        ok(enumerated_streams == test_data[i].in->stream_count, "%d: created %d != def streams %d\n", i,
           enumerated_streams, test_data[i].in->stream_count);

        hr = CreateDataCache(NULL, &CLSID_NULL, &IID_IUnknown, (void **)&cache);
        ok(hr == S_OK, "unexpected %#lx\n", hr);
        hr = IOleCache2_QueryInterface(cache, &IID_IPersistStorage, (void **)&stg);
        ok(hr == S_OK, "unexpected %#lx\n", hr);
        hr = IPersistStorage_Load(stg, doc1);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        IStorage_Release(doc1);

        hr = StgCreateDocfile(NULL, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &doc2);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        hr = IPersistStorage_IsDirty(stg);
        ok(hr == S_FALSE, "%d: unexpected %#lx\n", i, hr);

        hr = IPersistStorage_Save(stg, doc2, FALSE);
        ok(hr == S_OK, "unexpected %#lx\n", hr);

        IPersistStorage_Release(stg);

        enumerated_streams = matched_streams = -1;
        check_storage_contents(doc2, test_data[i].out, &enumerated_streams, &matched_streams);
        todo_wine_if(!(test_data[i].in == &stg_def_0 || test_data[i].in == &stg_def_1 || test_data[i].in == &stg_def_2))
        ok(enumerated_streams == matched_streams, "%d out: enumerated %d != matched %d\n", i,
           enumerated_streams, matched_streams);
        todo_wine_if(!(test_data[i].in == &stg_def_0 || test_data[i].in == &stg_def_4 || test_data[i].in == &stg_def_5
                 || test_data[i].in == &stg_def_6))
        ok(enumerated_streams == test_data[i].out->stream_count, "%d: saved streams %d != def streams %d\n", i,
            enumerated_streams, test_data[i].out->stream_count);

        IStorage_Release(doc2);

        if (winetest_debug > 1)
            trace("done testing storage def %d\n", i);
    }
}

static void test_OleCreateStaticFromData(void)
{
    FORMATETC dib_fmt = {CF_DIB, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    FORMATETC emf_fmt = {CF_ENHMETAFILE, NULL, DVASPECT_CONTENT, -1, TYMED_ENHMF};
    FORMATETC text_fmt = {CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr;
    IOleObject *ole_obj = NULL;
    IStorage *storage;
    ILockBytes *ilb;
    IPersist *persist;
    CLSID clsid;
    STATSTG statstg;
    int enumerated_streams, matched_streams;
    STGMEDIUM stgmed;
    static const struct expected_method methods_create_from_dib[] =
    {
        { "DataObject_EnumFormatEtc", TEST_TODO },
        { "DataObject_GetDataHere", 0 },
        { NULL }
    };
    static struct storage_def stg_def_dib =
    {
        &CLSID_Picture_Dib, 3,
        {{ "\1Ole", -1, 0, 0, NULL, 0 },
         { "\1CompObj", -1, 0, 0, NULL, 0 },
         { "CONTENTS", -1, 0, 0, NULL, 0 }}
    };
    static struct storage_def stg_def_emf =
    {
        &CLSID_Picture_EnhMetafile, 3,
        {{ "\1Ole", -1, 0, 0, NULL, 0 },
         { "\1CompObj", -1, 0, 0, NULL, 0 },
         { "CONTENTS", -1, 0, 0, NULL, 0 }}
    };


    hr = CreateILockBytesOnHGlobal(NULL, TRUE, &ilb);
    ok(hr == S_OK, "CreateILockBytesOnHGlobal failed: 0x%08lx.\n", hr);
    hr = StgCreateDocfileOnILockBytes(ilb, STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE,
                                      0, &storage);
    ok(hr == S_OK, "StgCreateDocfileOnILockBytes failed: 0x%08lx.\n", hr);
    ILockBytes_Release(ilb);

    hr = OleCreateStaticFromData(&DataObject, &IID_IOleObject, OLERENDER_FORMAT,
                                 &dib_fmt, NULL, NULL, (void **)&ole_obj);
    ok(hr == E_INVALIDARG, "OleCreateStaticFromData should fail: 0x%08lx.\n", hr);

    hr = OleCreateStaticFromData(&DataObject, &IID_IOleObject, OLERENDER_FORMAT,
                                 &dib_fmt, NULL, storage, NULL);
    ok(hr == E_INVALIDARG, "OleCreateStaticFromData should fail: 0x%08lx.\n", hr);

    /* CF_DIB */
    data_object_format = &dib_fmt;
    data_object_dib = dib_white;
    hr = OleCreateStaticFromData(&DataObject, &IID_IOleObject, OLERENDER_FORMAT,
                                 &dib_fmt, NULL, storage, (void **)&ole_obj);
    ok(hr == S_OK, "OleCreateStaticFromData failed: 0x%08lx.\n", hr);
    hr = IOleObject_QueryInterface(ole_obj, &IID_IPersist, (void **)&persist);
    ok(hr == S_OK, "IOleObject_QueryInterface failed: 0x%08lx.\n", hr);
    hr = IPersist_GetClassID(persist, &clsid);
    ok(hr == S_OK, "IPersist_GetClassID failed: 0x%08lx.\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_Picture_Dib), "Got wrong clsid: %s, expected: %s.\n",
       wine_dbgstr_guid(&clsid), wine_dbgstr_guid(&CLSID_Picture_Dib));
    hr = IStorage_Stat(storage, &statstg, STATFLAG_NONAME);
    ok_ole_success(hr, "IStorage_Stat");
    ok(IsEqualCLSID(&CLSID_Picture_Dib, &statstg.clsid), "Wrong CLSID in storage.\n");
    enumerated_streams = matched_streams = -1;
    get_stgmedium(CF_DIB, &stgmed);
    get_stgdef(&stg_def_dib, CF_DIB, &stgmed, 2);
    check_storage_contents(storage, &stg_def_dib, &enumerated_streams, &matched_streams);
    ok(enumerated_streams == matched_streams, "enumerated %d != matched %d\n",
       enumerated_streams, matched_streams);
    ok(enumerated_streams == stg_def_dib.stream_count, "created %d != def streams %d\n",
       enumerated_streams, stg_def_dib.stream_count);
    ReleaseStgMedium(&stgmed);
    IPersist_Release(persist);
    IStorage_Release(storage);
    IOleObject_Release(ole_obj);
    free((void *)stg_def_dib.stream[2].data);

    /* CF_ENHMETAFILE */
    hr = CreateILockBytesOnHGlobal(NULL, TRUE, &ilb);
    ok(hr == S_OK, "CreateILockBytesOnHGlobal failed: 0x%08lx.\n", hr);
    hr = StgCreateDocfileOnILockBytes(ilb, STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE,
                                      0, &storage);
    ok(hr == S_OK, "StgCreateDocfileOnILockBytes failed: 0x%08lx.\n", hr);
    ILockBytes_Release(ilb);
    data_object_format = &emf_fmt;
    hr = OleCreateStaticFromData(&DataObject, &IID_IOleObject, OLERENDER_FORMAT,
                                 &emf_fmt, NULL, storage, (void **)&ole_obj);
    ok(hr == S_OK, "OleCreateStaticFromData failed: 0x%08lx.\n", hr);
    hr = IOleObject_QueryInterface(ole_obj, &IID_IPersist, (void **)&persist);
    ok(hr == S_OK, "IOleObject_QueryInterface failed: 0x%08lx.\n", hr);
    hr = IPersist_GetClassID(persist, &clsid);
    ok(hr == S_OK, "IPersist_GetClassID failed: 0x%08lx.\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_Picture_EnhMetafile), "Got wrong clsid: %s, expected: %s.\n",
       wine_dbgstr_guid(&clsid), wine_dbgstr_guid(&CLSID_Picture_EnhMetafile));
    hr = IStorage_Stat(storage, &statstg, STATFLAG_NONAME);
    ok_ole_success(hr, "IStorage_Stat");
    ok(IsEqualCLSID(&CLSID_Picture_EnhMetafile, &statstg.clsid), "Wrong CLSID in storage.\n");
    enumerated_streams = matched_streams = -1;
    get_stgmedium(CF_ENHMETAFILE, &stgmed);
    get_stgdef(&stg_def_emf, CF_ENHMETAFILE, &stgmed, 2);
    check_storage_contents(storage, &stg_def_emf, &enumerated_streams, &matched_streams);
    ok(enumerated_streams == matched_streams, "enumerated %d != matched %d\n",
       enumerated_streams, matched_streams);
    ok(enumerated_streams == stg_def_emf.stream_count, "created %d != def streams %d\n",
       enumerated_streams, stg_def_emf.stream_count);
    ReleaseStgMedium(&stgmed);
    IPersist_Release(persist);
    IStorage_Release(storage);
    IOleObject_Release(ole_obj);
    free((void *)stg_def_emf.stream[2].data);

    /* CF_TEXT */
    hr = CreateILockBytesOnHGlobal(NULL, TRUE, &ilb);
    ok(hr == S_OK, "CreateILockBytesOnHGlobal failed: 0x%08lx.\n", hr);
    hr = StgCreateDocfileOnILockBytes(ilb, STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE,
                                      0, &storage);
    ok(hr == S_OK, "StgCreateDocfileOnILockBytes failed: 0x%08lx.\n", hr);
    ILockBytes_Release(ilb);
    data_object_format = &text_fmt;
    hr = OleCreateStaticFromData(&DataObject, &IID_IOleObject, OLERENDER_FORMAT,
                                 &text_fmt, NULL, storage, (void **)&ole_obj);
    ok(hr == DV_E_CLIPFORMAT, "OleCreateStaticFromData should fail: 0x%08lx.\n", hr);
    IStorage_Release(storage);

    hr = CreateILockBytesOnHGlobal(NULL, TRUE, &ilb);
    ok(hr == S_OK, "CreateILockBytesOnHGlobal failed: 0x%08lx.\n", hr);
    hr = StgCreateDocfileOnILockBytes(ilb, STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE,
                                      0, &storage);
    ok(hr == S_OK, "StgCreateDocfileOnILockBytes failed: 0x%08lx.\n", hr);
    ILockBytes_Release(ilb);
    data_object_format = &dib_fmt;
    expected_method_list = methods_create_from_dib;
    hr = OleCreateFromData(&DataObject, &IID_IOleObject, OLERENDER_FORMAT, &dib_fmt, NULL,
                           storage, (void **)&ole_obj);
    todo_wine ok(hr == DV_E_FORMATETC, "OleCreateFromData should failed: 0x%08lx.\n", hr);
    IStorage_Release(storage);
}

static void test_ReleaseStgMedium( void )
{
    ReleaseStgMedium( NULL );
}

START_TEST(ole2)
{
    DWORD dwRegister;
    IStorage *pStorage;
    STATSTG statstg;
    HRESULT hr;

    cf_test_1 = RegisterClipboardFormatA("cf_winetest_1");
    cf_test_2 = RegisterClipboardFormatA("cf_winetest_2");
    cf_test_3 = RegisterClipboardFormatA("cf_winetest_3");

    CoInitialize(NULL);

    hr = CoRegisterClassObject(&CLSID_Equation3, (IUnknown *)&OleObjectCF, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &dwRegister);
    ok_ole_success(hr, "CoRegisterClassObject");

    hr = StgCreateDocfile(NULL, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &pStorage);
    ok_ole_success(hr, "StgCreateDocfile");

    test_OleCreate(pStorage);

    hr = IStorage_Stat(pStorage, &statstg, STATFLAG_NONAME);
    ok_ole_success(hr, "IStorage_Stat");
    ok(IsEqualCLSID(&CLSID_Equation3, &statstg.clsid), "Wrong CLSID in storage\n");

    test_OleLoad(pStorage);

    IStorage_Release(pStorage);

    hr = CoRevokeClassObject(dwRegister);
    ok_ole_success(hr, "CoRevokeClassObject");

    Storage_SetClass_CLSID = &CLSID_WineTest;

    test_data_cache();
    test_data_cache_dib_contents_stream( 0 );
    test_data_cache_dib_contents_stream( 1 );
    test_data_cache_cache();
    test_data_cache_init();
    test_data_cache_initnew();
    test_data_cache_updatecache();
    test_default_handler();
    test_runnable();
    test_OleRun();
    test_OleLockRunning();
    test_OleDraw();
    test_OleDoAutoConvert();
    test_data_cache_save();
    test_data_cache_save_data();
    test_data_cache_contents();
    test_OleCreateStaticFromData();
    test_ReleaseStgMedium();

    CoUninitialize();
}
