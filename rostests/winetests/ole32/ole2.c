/*
 * Object Linking and Embedding Tests
 *
 * Copyright 2005 Robert Shearman
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define CONST_VTABLE
#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wingdi.h>
#include <winreg.h>
#include <ole2.h>
//#include "objbase.h"
//#include "shlguid.h"

#include <wine/test.h>

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08x\n", hr)

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
DEFINE_EXPECT(Storage_SetClass);
DEFINE_EXPECT(Storage_CreateStream_CompObj);
DEFINE_EXPECT(Storage_OpenStream_Ole);

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

#define TEST_OPTIONAL 0x1
#define TEST_TODO     0x2

struct expected_method
{
    const char *method;
    unsigned int flags;
};

static const struct expected_method *expected_method_list;
static FORMATETC *g_expected_fetc = NULL;

static BOOL g_showRunnable = TRUE;
static BOOL g_isRunning = TRUE;
static BOOL g_failGetMiscStatus;
static HRESULT g_QIFailsWith;

static UINT cf_test_1, cf_test_2, cf_test_3;

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
    DWORD unknown3; /* 4, possibly TYMED_ISTREAM */
    DVASPECT dvAspect;
    DWORD lindex;
    DWORD tymed;
    DWORD unknown7; /* 0 */
    DWORD dwObjectExtentX;
    DWORD dwObjectExtentY;
    DWORD dwSize;
} PresentationDataHeader;

#define CHECK_EXPECTED_METHOD(method_name) \
    do { \
        trace("%s\n", method_name); \
        ok(expected_method_list->method != NULL, "Extra method %s called\n", method_name); \
        if (!strcmp(expected_method_list->method, "WINE_EXTRA")) \
        { \
            todo_wine ok(0, "Too many method calls.\n"); \
            break; \
        } \
        if (expected_method_list->method) \
        { \
            while (expected_method_list->flags & TEST_OPTIONAL && \
                   strcmp(expected_method_list->method, method_name) != 0) \
                expected_method_list++; \
            if (expected_method_list->flags & TEST_TODO) \
                todo_wine \
                    ok(!strcmp(expected_method_list->method, method_name), \
                       "Expected %s to be called instead of %s\n", \
                       expected_method_list->method, method_name); \
            else \
                ok(!strcmp(expected_method_list->method, method_name), \
                   "Expected %s to be called instead of %s\n", \
                   expected_method_list->method, method_name); \
            expected_method_list++; \
        } \
    } while(0)

#define CHECK_NO_EXTRA_METHODS() \
    do { \
        while (expected_method_list->flags & TEST_OPTIONAL) \
            expected_method_list++; \
        ok(!expected_method_list->method, "Method sequence starting from %s not called\n", expected_method_list->method); \
    } while (0)

static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    CHECK_EXPECTED_METHOD("OleObject_QueryInterface");

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

    trace("OleObject_QueryInterface: returning E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    CHECK_EXPECTED_METHOD("OleObject_AddRef");
    return 2;
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    CHECK_EXPECTED_METHOD("OleObject_Release");
    return 1;
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
    DWORD dwAspect,
    DWORD *pdwStatus
)
{
    CHECK_EXPECTED_METHOD("OleObject_GetMiscStatus");
    if(!g_failGetMiscStatus)
    {
        *pdwStatus = OLEMISC_RECOMPOSEONRESIZE;
        return S_OK;
    }
    else
    {
        *pdwStatus = 0x1234;
        return E_FAIL;
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
    trace("OleObjectPersistStg_QueryInterface\n");
    return IUnknown_QueryInterface((IUnknown *)&OleObject, riid, ppv);
}

static ULONG WINAPI OleObjectPersistStg_AddRef(IPersistStorage *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_AddRef");
    return 2;
}

static ULONG WINAPI OleObjectPersistStg_Release(IPersistStorage *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_Release");
    return 1;
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
    return IUnknown_QueryInterface((IUnknown *)&OleObject, riid, ppv);
}

static ULONG WINAPI OleObjectCache_AddRef(IOleCache *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_AddRef");
    return 2;
}

static ULONG WINAPI OleObjectCache_Release(IOleCache *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_Release");
    return 1;
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
                    "dwAspect: %x\n", pformatetc->dwAspect);
            ok(pformatetc->lindex == g_expected_fetc->lindex,
                    "lindex: %x\n", pformatetc->lindex);
            ok(pformatetc->tymed == g_expected_fetc->tymed,
                    "tymed: %x\n", pformatetc->tymed);
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
    return IUnknown_QueryInterface((IUnknown *)&OleObject, riid, ppv);
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
    return IUnknown_QueryInterface((IUnknown *)&OleObject, riid, ppv);
}

static ULONG WINAPI OleObjectRunnable_AddRef(IRunnableObject *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_AddRef");
    return 2;
}

static ULONG WINAPI OleObjectRunnable_Release(IRunnableObject *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_Release");
    return 1;
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
    ok(index == -1, "index=%d\n", index);
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
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", TEST_OPTIONAL },
        { "OleObject_Release", TEST_OPTIONAL },
        { "OleObject_QueryInterface", TEST_OPTIONAL },
        { "OleObjectPersistStg_AddRef", 0 },
        { "OleObjectPersistStg_InitNew", 0 },
        { "OleObjectPersistStg_Release", 0 },
        { "OleObject_Release", 0 },
        { "OleObject_Release", TEST_OPTIONAL },
        { NULL, 0 }
    };
    static const struct expected_method methods_olerender_draw[] =
    {
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", TEST_OPTIONAL /* NT4 only */ },
        { "OleObject_Release", TEST_OPTIONAL /* NT4 only */ },
        { "OleObject_QueryInterface", TEST_OPTIONAL /* NT4 only */ },
        { "OleObjectPersistStg_AddRef", 0 },
        { "OleObjectPersistStg_InitNew", 0 },
        { "OleObjectPersistStg_Release", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObjectRunnable_AddRef", 0 },
        { "OleObjectRunnable_Run", 0 },
        { "OleObjectRunnable_Release", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObjectCache_AddRef", 0 },
        { "OleObjectCache_Cache", 0 },
        { "OleObjectCache_Release", 0 },
        { "OleObject_Release", 0 },
        { "OleObject_Release", TEST_OPTIONAL /* NT4 only */ },
        { NULL, 0 }
    };
    static const struct expected_method methods_olerender_format[] =
    {
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", 0 },
        { "OleObject_GetMiscStatus", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObjectPersistStg_AddRef", 0 },
        { "OleObjectPersistStg_InitNew", 0 },
        { "OleObjectPersistStg_Release", 0 },
        { "OleObject_SetClientSite", 0 },
        { "OleObject_Release", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObjectRunnable_AddRef", 0 },
        { "OleObjectRunnable_Run", 0 },
        { "OleObjectRunnable_Release", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObjectCache_AddRef", 0 },
        { "OleObjectCache_Cache", 0 },
        { "OleObjectCache_Release", 0 },
        { "OleObject_Release", 0 },
        { NULL, 0 }
    };
    static const struct expected_method methods_olerender_asis[] =
    {
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", TEST_OPTIONAL /* NT4 only */ },
        { "OleObject_Release", TEST_OPTIONAL /* NT4 only */ },
        { "OleObject_QueryInterface", TEST_OPTIONAL /* NT4 only */ },
        { "OleObjectPersistStg_AddRef", 0 },
        { "OleObjectPersistStg_InitNew", 0 },
        { "OleObjectPersistStg_Release", 0 },
        { "OleObject_Release", 0 },
        { "OleObject_Release", TEST_OPTIONAL /* NT4 only */ },
        { NULL, 0 }
    };
    static const struct expected_method methods_olerender_draw_no_runnable[] =
    {
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", TEST_OPTIONAL /* NT4 only */ },
        { "OleObject_Release", TEST_OPTIONAL /* NT4 only */ },
        { "OleObject_QueryInterface", TEST_OPTIONAL /* NT4 only */ },
        { "OleObjectPersistStg_AddRef", 0 },
        { "OleObjectPersistStg_InitNew", 0 },
        { "OleObjectPersistStg_Release", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObjectCache_AddRef", 0 },
        { "OleObjectCache_Cache", 0 },
        { "OleObjectCache_Release", 0 },
        { "OleObject_Release", 0 },
        { "OleObject_Release", TEST_OPTIONAL /* NT4 only */ },
        { NULL, 0 },
    };
    static const struct expected_method methods_olerender_draw_no_cache[] =
    {
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", TEST_OPTIONAL /* NT4 only */ },
        { "OleObject_Release", TEST_OPTIONAL /* NT4 only */ },
        { "OleObject_QueryInterface", TEST_OPTIONAL /* NT4 only */ },
        { "OleObjectPersistStg_AddRef", 0 },
        { "OleObjectPersistStg_InitNew", 0 },
        { "OleObjectPersistStg_Release", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObjectRunnable_AddRef", 0 },
        { "OleObjectRunnable_Run", 0 },
        { "OleObjectRunnable_Release", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_Release", 0 },
        { "OleObject_Release", TEST_OPTIONAL /* NT4 only */ },
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
    trace("OleCreate with OLERENDER_NONE:\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_NONE, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();

    expected_method_list = methods_olerender_draw;
    trace("OleCreate with OLERENDER_DRAW:\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_DRAW, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();

    formatetc.cfFormat = CF_TEXT;
    formatetc.ptd = NULL;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = -1;
    formatetc.tymed = TYMED_HGLOBAL;
    expected_method_list = methods_olerender_format;
    trace("OleCreate with OLERENDER_FORMAT:\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_FORMAT, &formatetc, (IOleClientSite *)0xdeadbeef, pStorage, (void **)&pObject);
    ok(hr == S_OK ||
       broken(hr == E_INVALIDARG), /* win2k */
       "OleCreate failed with error 0x%08x\n", hr);
    if (pObject)
    {
        IOleObject_Release(pObject);
        CHECK_NO_EXTRA_METHODS();
    }

    expected_method_list = methods_olerender_asis;
    trace("OleCreate with OLERENDER_ASIS:\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_ASIS, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();

    formatetc.cfFormat = 0;
    formatetc.tymed = TYMED_NULL;
    runnable = NULL;
    expected_method_list = methods_olerender_draw_no_runnable;
    trace("OleCreate with OLERENDER_DRAW (no IRunnableObject):\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_DRAW, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();

    runnable = &OleObjectRunnable;
    cache = NULL;
    expected_method_list = methods_olerender_draw_no_cache;
    trace("OleCreate with OLERENDER_DRAW (no IOleCache):\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_DRAW, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    CHECK_NO_EXTRA_METHODS();
    trace("end\n");
    g_expected_fetc = NULL;
}

static void test_OleLoad(IStorage *pStorage)
{
    HRESULT hr;
    IOleObject *pObject;
    DWORD fmt;

    static const struct expected_method methods_oleload[] =
    {
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", 0 },
        { "OleObject_GetMiscStatus", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObjectPersistStg_AddRef", 0 },
        { "OleObjectPersistStg_Load", 0 },
        { "OleObjectPersistStg_Release", 0 },
        { "OleObject_SetClientSite", 0 },
        { "OleObject_Release", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_GetMiscStatus", 0 },
        { "OleObject_Release", 0 },
        { NULL, 0 }
    };

    /* Test once with IOleObject_GetMiscStatus failing */
    expected_method_list = methods_oleload;
    g_failGetMiscStatus = TRUE;
    trace("OleLoad:\n");
    hr = OleLoad(pStorage, &IID_IOleObject, (IOleClientSite *)0xdeadbeef, (void **)&pObject);
    ok(hr == S_OK ||
       broken(hr == E_INVALIDARG), /* win98 and win2k */
       "OleLoad failed with error 0x%08x\n", hr);
    if(pObject)
    {
        DWORD dwStatus = 0xdeadbeef;
        hr = IOleObject_GetMiscStatus(pObject, DVASPECT_CONTENT, &dwStatus);
        ok(hr == E_FAIL, "Got 0x%08x\n", hr);
        ok(dwStatus == 0x1234, "Got 0x%08x\n", dwStatus);

        IOleObject_Release(pObject);
        CHECK_NO_EXTRA_METHODS();
    }

    /* Test again, let IOleObject_GetMiscStatus succeed. */
    g_failGetMiscStatus = FALSE;
    expected_method_list = methods_oleload;
    trace("OleLoad:\n");
    hr = OleLoad(pStorage, &IID_IOleObject, (IOleClientSite *)0xdeadbeef, (void **)&pObject);
    ok(hr == S_OK ||
       broken(hr == E_INVALIDARG), /* win98 and win2k */
       "OleLoad failed with error 0x%08x\n", hr);
    if (pObject)
    {
        DWORD dwStatus = 0xdeadbeef;
        hr = IOleObject_GetMiscStatus(pObject, DVASPECT_CONTENT, &dwStatus);
        ok(hr == S_OK, "Got 0x%08x\n", hr);
        ok(dwStatus == 1, "Got 0x%08x\n", dwStatus);

        IOleObject_Release(pObject);
        CHECK_NO_EXTRA_METHODS();
    }

    for (fmt = CF_TEXT; fmt < CF_MAX; fmt++)
    {
        static const WCHAR olrepres[] = { 2,'O','l','e','P','r','e','s','0','0','0',0 };
        IStorage *stg;
        IStream *stream;
        IUnknown *obj;
        DWORD data, i, tymed, data_size;
        PresentationDataHeader header;
        HDC hdc;
        HGDIOBJ hobj;
        RECT rc;
        char buf[256];

        for (i = 0; i < 7; i++)
        {
            hr = StgCreateDocfile(NULL, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &stg);
            ok(hr == S_OK, "StgCreateDocfile error %#x\n", hr);

            hr = IStorage_SetClass(stg, &CLSID_WineTest);
            ok(hr == S_OK, "SetClass error %#x\n", hr);

            hr = IStorage_CreateStream(stg, olrepres, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, 0, &stream);
            ok(hr == S_OK, "CreateStream error %#x\n", hr);

            data = ~0;
            hr = IStream_Write(stream, &data, sizeof(data), NULL);
            ok(hr == S_OK, "Write error %#x\n", hr);

            data = fmt;
            hr = IStream_Write(stream, &data, sizeof(data), NULL);
            ok(hr == S_OK, "Write error %#x\n", hr);

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

            tymed = 1 << i;

            header.unknown3 = 4;
            header.dvAspect = DVASPECT_CONTENT;
            header.lindex = -1;
            header.tymed = tymed;
            header.unknown7 = 0;
            header.dwObjectExtentX = 1;
            header.dwObjectExtentY = 1;
            header.dwSize = data_size;
            hr = IStream_Write(stream, &header, sizeof(header), NULL);
            ok(hr == S_OK, "Write error %#x\n", hr);

            hr = IStream_Write(stream, buf, data_size, NULL);
            ok(hr == S_OK, "Write error %#x\n", hr);

            IStream_Release(stream);

            hr = OleLoad(stg, &IID_IUnknown, NULL, (void **)&obj);
            /* FIXME: figure out stream format */
            if (fmt == CF_BITMAP && hr != S_OK)
            {
                IStorage_Release(stg);
                continue;
            }
            ok(hr == S_OK, "OleLoad error %#x: cfFormat = %u, tymed = %u\n", hr, fmt, tymed);

            hdc = CreateCompatibleDC(0);
            SetRect(&rc, 0, 0, 100, 100);
            hr = OleDraw(obj, DVASPECT_CONTENT, hdc, &rc);
            DeleteDC(hdc);
            if (fmt == CF_METAFILEPICT)
                ok(hr == S_OK, "OleDraw error %#x: cfFormat = %u, tymed = %u\n", hr, fmt, tymed);
            else if (fmt == CF_ENHMETAFILE)
todo_wine
                ok(hr == S_OK, "OleDraw error %#x: cfFormat = %u, tymed = %u\n", hr, fmt, tymed);
            else
                ok(hr == OLE_E_BLANK || hr == OLE_E_NOTRUNNING || hr == E_FAIL, "OleDraw should fail: %#x, cfFormat = %u, tymed = %u\n", hr, fmt, header.tymed);

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
    CHECK_EXPECTED_METHOD("DataObject_QueryInterface");

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
    CHECK_EXPECTED_METHOD("DataObject_AddRef");
    return 2;
}

static ULONG WINAPI DataObject_Release(
            IDataObject*     iface)
{
    CHECK_EXPECTED_METHOD("DataObject_Release");
    return 1;
}

static HRESULT WINAPI DataObject_GetData(
        IDataObject*     iface,
        LPFORMATETC      pformatetcIn,
        STGMEDIUM*       pmedium)
{
    CHECK_EXPECTED_METHOD("DataObject_GetData");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_GetDataHere(
        IDataObject*     iface,
        LPFORMATETC      pformatetc,
        STGMEDIUM*       pmedium)
{
    CHECK_EXPECTED_METHOD("DataObject_GetDataHere");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_QueryGetData(
        IDataObject*     iface,
        LPFORMATETC      pformatetc)
{
    CHECK_EXPECTED_METHOD("DataObject_QueryGetData");
    return S_OK;
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
        U(stgmedium).hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 4);
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

static void test_data_cache(void)
{
    HRESULT hr;
    IOleCache2 *pOleCache;
    IStorage *pStorage;
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
        { "DataObject_GetData", 0 },
        { "DataObject_GetData", 0 },
        { "DataObject_GetData", 0 },
        { NULL, 0 }
    };
    static const struct expected_method methods_cachethenrun[] =
    {
        { "DataObject_DAdvise", 0 },
        { "DataObject_DAdvise", 0 },
        { "DataObject_DAdvise", 0 },
        { "DataObject_QueryGetData", 1 }, /* called by win9x and nt4 */
        { "DataObject_DAdvise", 0 },
        { "DataObject_DUnadvise", 0 },
        { "DataObject_DUnadvise", 0 },
        { "DataObject_DUnadvise", 0 },
        { "DataObject_DUnadvise", 0 },
        { NULL, 0 }
    };

    GetSystemDirectoryA(szSystemDir, sizeof(szSystemDir)/sizeof(szSystemDir[0]));

    expected_method_list = methods_cacheinitnew;

    fmtetc.cfFormat = CF_METAFILEPICT;
    fmtetc.dwAspect = DVASPECT_ICON;
    fmtetc.lindex = -1;
    fmtetc.ptd = NULL;
    fmtetc.tymed = TYMED_MFPICT;

    hr = StgCreateDocfile(NULL, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &pStorage);
    ok_ole_success(hr, "StgCreateDocfile");

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
    ok(hr == OLE_E_NOCONNECTION, "IOleCache_Uncache with invalid value should return OLE_E_NOCONNECTION instead of 0x%x\n", hr);

    /* Both tests crash on NT4 and below. StgCreatePropSetStg is only available on w2k and above. */
    if (GetProcAddress(GetModuleHandleA("ole32.dll"), "StgCreatePropSetStg"))
    {
        hr = IOleCache2_Cache(pOleCache, NULL, 0, &dwConnection);
        ok(hr == E_INVALIDARG, "IOleCache_Cache with NULL fmtetc should have returned E_INVALIDARG instead of 0x%08x\n", hr);

        hr = IOleCache2_Cache(pOleCache, NULL, 0, NULL);
        ok(hr == E_INVALIDARG, "IOleCache_Cache with NULL pdwConnection should have returned E_INVALIDARG instead of 0x%08x\n", hr);
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
                ok(hr == S_OK, "IOleCache_Cache cfFormat = %d, tymed = %d should have returned S_OK instead of 0x%08x\n",
                    fmtetc.cfFormat, fmtetc.tymed, hr);
            else if (fmtetc.tymed == TYMED_HGLOBAL)
                ok(hr == CACHE_S_FORMATETC_NOTSUPPORTED ||
                   broken(hr == S_OK && fmtetc.cfFormat == CF_BITMAP) /* Win9x & NT4 */,
                    "IOleCache_Cache cfFormat = %d, tymed = %d should have returned CACHE_S_FORMATETC_NOTSUPPORTED instead of 0x%08x\n",
                    fmtetc.cfFormat, fmtetc.tymed, hr);
            else
                ok(hr == DV_E_TYMED, "IOleCache_Cache cfFormat = %d, tymed = %d should have returned DV_E_TYMED instead of 0x%08x\n",
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

    MultiByteToWideChar(CP_ACP, 0, szSystemDir, -1, wszPath, sizeof(wszPath)/sizeof(wszPath[0]));
    memcpy(wszPath+lstrlenW(wszPath), wszShell32, sizeof(wszShell32));

    fmtetc.cfFormat = CF_METAFILEPICT;
    stgmedium.tymed = TYMED_MFPICT;
    U(stgmedium).hMetaFilePict = OleMetafilePictFromIconAndLabel(
        LoadIconA(NULL, (LPSTR)IDI_APPLICATION), wszPath, wszPath, 0);
    stgmedium.pUnkForRelease = NULL;

    fmtetc.dwAspect = DVASPECT_CONTENT;
    hr = IOleCache2_SetData(pOleCache, &fmtetc, &stgmedium, FALSE);
    ok(hr == OLE_E_BLANK, "IOleCache_SetData for aspect not in cache should have return OLE_E_BLANK instead of 0x%08x\n", hr);

    fmtetc.dwAspect = DVASPECT_ICON;
    hr = IOleCache2_SetData(pOleCache, &fmtetc, &stgmedium, FALSE);
    ok_ole_success(hr, "IOleCache_SetData");
    ReleaseStgMedium(&stgmedium);

    hr = IViewObject_Freeze(pViewObject, DVASPECT_ICON, -1, NULL, &dwFreeze);
    todo_wine {
    ok_ole_success(hr, "IViewObject_Freeze");
    hr = IViewObject_Freeze(pViewObject, DVASPECT_CONTENT, -1, NULL, &dwFreeze);
    ok(hr == OLE_E_BLANK, "IViewObject_Freeze with uncached aspect should have returned OLE_E_BLANK instead of 0x%08x\n", hr);
    }

    rcBounds.left = 0;
    rcBounds.top = 0;
    rcBounds.right = 100;
    rcBounds.bottom = 100;
    hdcMem = CreateCompatibleDC(NULL);

    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");

    hr = IViewObject_Draw(pViewObject, DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok(hr == OLE_E_BLANK, "IViewObject_Draw with uncached aspect should have returned OLE_E_BLANK instead of 0x%08x\n", hr);

    /* a NULL draw_continue fn ptr */
    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, NULL, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");

    /* draw_continue that returns FALSE to abort drawing */
    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue_false, 0xdeadbeef);
    ok(hr == E_ABORT ||
       broken(hr == S_OK), /* win9x may skip the callbacks */
       "IViewObject_Draw with draw_continue_false returns 0x%08x\n", hr);

    DeleteDC(hdcMem);

    hr = IOleCacheControl_OnRun(pOleCacheControl, &DataObject);
    ok_ole_success(hr, "IOleCacheControl_OnRun");

    hr = IPersistStorage_Save(pPS, pStorage, TRUE);
    ok_ole_success(hr, "IPersistStorage_Save");

    hr = IPersistStorage_SaveCompleted(pPS, NULL);
    ok_ole_success(hr, "IPersistStorage_SaveCompleted");

    hr = IPersistStorage_IsDirty(pPS);
    ok(hr == S_FALSE, "IPersistStorage_IsDirty should have returned S_FALSE instead of 0x%x\n", hr);

    IPersistStorage_Release(pPS);
    IViewObject_Release(pViewObject);
    IOleCache2_Release(pOleCache);
    IOleCacheControl_Release(pOleCacheControl);

    CHECK_NO_EXTRA_METHODS();

    /* Test with loaded data */
    trace("Testing loaded data with CreateDataCache:\n");
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
    ok(hr == S_FALSE, "IPersistStorage_IsDirty should have returned S_FALSE instead of 0x%x\n", hr);

    fmtetc.cfFormat = 0;
    fmtetc.dwAspect = DVASPECT_ICON;
    fmtetc.lindex = -1;
    fmtetc.ptd = NULL;
    fmtetc.tymed = TYMED_MFPICT;
    hr = IOleCache2_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok(hr == CACHE_S_SAMECACHE, "IOleCache_Cache with already loaded data format type should return CACHE_S_SAMECACHE instead of 0x%x\n", hr);

    rcBounds.left = 0;
    rcBounds.top = 0;
    rcBounds.right = 100;
    rcBounds.bottom = 100;
    hdcMem = CreateCompatibleDC(NULL);

    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");

    hr = IViewObject_Draw(pViewObject, DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok(hr == OLE_E_BLANK, "IViewObject_Draw with uncached aspect should have returned OLE_E_BLANK instead of 0x%08x\n", hr);

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
    ok(hr == OLE_E_BLANK, "IViewObject_Draw with uncached aspect should have returned OLE_E_BLANK instead of 0x%08x\n", hr);

    DeleteDC(hdcMem);

    todo_wine {
    hr = IOleCache2_InitCache(pOleCache, &DataObject);
    ok(hr == CACHE_E_NOCACHE_UPDATED, "IOleCache_InitCache should have returned CACHE_E_NOCACHE_UPDATED instead of 0x%08x\n", hr);
    }

    IPersistStorage_Release(pPS);
    IViewObject_Release(pViewObject);
    IOleCache2_Release(pOleCache);

    todo_wine {
    CHECK_NO_EXTRA_METHODS();
    }

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
    ok(hr == OLE_E_BLANK, "got %08x\n", hr);

    fmtetc.cfFormat = cf_test_1;
    fmtetc.dwAspect = DVASPECT_CONTENT;
    fmtetc.tymed = TYMED_HGLOBAL;

    hr = IOleCache2_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok(hr == CACHE_S_FORMATETC_NOTSUPPORTED, "got %08x\n", hr);

    hr = IDataObject_GetData(pCacheDataObject, &fmtetc, &stgmedium);
    ok(hr == OLE_E_BLANK, "got %08x\n", hr);

    fmtetc.cfFormat = cf_test_2;
    hr = IOleCache2_Cache(pOleCache, &fmtetc, ADVF_PRIMEFIRST, &dwConnection);
    ok(hr == CACHE_S_FORMATETC_NOTSUPPORTED, "got %08x\n", hr);

    hr = IDataObject_GetData(pCacheDataObject, &fmtetc, &stgmedium);
    ok(hr == OLE_E_BLANK, "got %08x\n", hr);

    hr = IOleCacheControl_OnRun(pOleCacheControl, &DataObject);
    ok_ole_success(hr, "IOleCacheControl_OnRun");

    fmtetc.cfFormat = cf_test_3;
    hr = IOleCache2_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok(hr == CACHE_S_FORMATETC_NOTSUPPORTED, "got %08x\n", hr);

    fmtetc.cfFormat = cf_test_1;
    hr = IDataObject_GetData(pCacheDataObject, &fmtetc, &stgmedium);
    ok(hr == OLE_E_BLANK, "got %08x\n", hr);

    fmtetc.cfFormat = cf_test_2;
    hr = IDataObject_GetData(pCacheDataObject, &fmtetc, &stgmedium);
    ok(hr == S_OK, "got %08x\n", hr);
    ReleaseStgMedium(&stgmedium);

    fmtetc.cfFormat = cf_test_3;
    hr = IDataObject_GetData(pCacheDataObject, &fmtetc, &stgmedium);
    ok(hr == OLE_E_BLANK, "got %08x\n", hr);

    IOleCacheControl_Release(pOleCacheControl);
    IDataObject_Release(pCacheDataObject);
    IOleCache2_Release(pOleCache);

    CHECK_NO_EXTRA_METHODS();

    IStorage_Release(pStorage);
}


static const WCHAR CONTENTS[] = {'C','O','N','T','E','N','T','S',0};

/* 2 x 1 x 32 bpp dib. PelsPerMeter = 200x400 */
static BYTE dib[] =
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
    ok( hr == S_OK, "got %08x\n", hr);
    hr = IStorage_SetClass( stg, &CLSID_Picture_Dib );
    ok( hr == S_OK, "got %08x\n", hr);
    hr = IStorage_CreateStream( stg, CONTENTS, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, 0, &stm );
    ok( hr == S_OK, "got %08x\n", hr);
    if (num == 1) /* Set biXPelsPerMeter = 0 */
    {
        dib[0x26] = 0;
        dib[0x27] = 0;
    }
    hr = IStream_Write( stm, dib, sizeof(dib), &written );
    ok( hr == S_OK, "got %08x\n", hr);
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
    FORMATETC fmt = {CF_DIB, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM med;
    CLSID cls;
    SIZEL sz;

    hr = CreateDataCache( NULL, &CLSID_Picture_Metafile, &IID_IUnknown, (void *)&unk );
    ok( SUCCEEDED(hr), "got %08x\n", hr );
    hr = IUnknown_QueryInterface( unk, &IID_IPersistStorage, (void *)&persist );
    ok( SUCCEEDED(hr), "got %08x\n", hr );
    hr = IUnknown_QueryInterface( unk, &IID_IDataObject, (void *)&data );
    ok( SUCCEEDED(hr), "got %08x\n", hr );
    hr = IUnknown_QueryInterface( unk, &IID_IViewObject2, (void *)&view );
    ok( SUCCEEDED(hr), "got %08x\n", hr );

    stg = create_storage( num );

    hr = IPersistStorage_Load( persist, stg );
    ok( SUCCEEDED(hr), "got %08x\n", hr );
    IStorage_Release( stg );

    hr = IPersistStorage_GetClassID( persist, &cls );
    ok( SUCCEEDED(hr), "got %08x\n", hr );
    ok( IsEqualCLSID( &cls, &CLSID_Picture_Dib ), "class id mismatch\n" );

    hr = IDataObject_GetData( data, &fmt, &med );
    ok( SUCCEEDED(hr), "got %08x\n", hr );
    if (SUCCEEDED(hr))
    {
        ok( med.tymed == TYMED_HGLOBAL, "got %x\n", med.tymed );
        ReleaseStgMedium( &med );
    }

    hr = IViewObject2_GetExtent( view, DVASPECT_CONTENT, -1, NULL, &sz );
    ok( SUCCEEDED(hr), "got %08x\n", hr );
    if (num == 0)
    {
        ok( sz.cx == 1000, "got %d\n", sz.cx );
        ok( sz.cy == 250, "got %d\n", sz.cy );
    }
    else
    {
        HDC hdc = GetDC( 0 );
        LONG x = 2 * 2540 / GetDeviceCaps( hdc, LOGPIXELSX );
        LONG y = 1 * 2540 / GetDeviceCaps( hdc, LOGPIXELSY );
        ok( sz.cx == x, "got %d %d\n", sz.cx, x );
        ok( sz.cy == y, "got %d %d\n", sz.cy, y );

        ReleaseDC( 0, hdc );
    }

    IViewObject2_Release( view );
    IDataObject_Release( data );
    IPersistStorage_Release( persist );
    IUnknown_Release( unk );
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
        { "OleObject_QueryInterface", 0 },
        { "OleObject_AddRef", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_QueryInterface", TEST_TODO },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_QueryInterface", 0 },
        { "OleObject_QueryInterface", TEST_OPTIONAL }, /* Win95/98/NT4 */
        { "OleObject_Release", TEST_TODO },
        { "WINE_EXTRA", TEST_OPTIONAL },
        { NULL, 0 }
    };

    hr = CoCreateInstance(&CLSID_WineTest, NULL, CLSCTX_INPROC_HANDLER, &IID_IOleObject, (void **)&pObject);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance should have failed with REGDB_E_CLASSNOTREG instead of 0x%08x\n", hr);

    hr = OleCreateDefaultHandler(&CLSID_WineTest, NULL, &IID_IOleObject, (void **)&pObject);
    ok_ole_success(hr, "OleCreateDefaultHandler");

    hr = IOleObject_QueryInterface(pObject, &IID_IOleInPlaceObject, (void **)&pInPlaceObj);
    ok(hr == E_NOINTERFACE, "IOleObject_QueryInterface(&IID_IOleInPlaceObject) should return E_NOINTERFACE instead of 0x%08x\n", hr);

    hr = IOleObject_Advise(pObject, &AdviseSink, &dwAdvConn);
    ok_ole_success(hr, "IOleObject_Advise");

    hr = IOleObject_Close(pObject, OLECLOSE_NOSAVE);
    ok_ole_success(hr, "IOleObject_Close");

    /* FIXME: test IOleObject_EnumAdvise */

    hr = IOleObject_EnumVerbs(pObject, &pEnumVerbs);
    ok(hr == REGDB_E_CLASSNOTREG, "IOleObject_EnumVerbs should have returned REGDB_E_CLASSNOTREG instead of 0x%08x\n", hr);

    hr = IOleObject_GetClientSite(pObject, &pClientSite);
    ok_ole_success(hr, "IOleObject_GetClientSite");

    hr = IOleObject_SetClientSite(pObject, pClientSite);
    ok_ole_success(hr, "IOleObject_SetClientSite");

    hr = IOleObject_GetClipboardData(pObject, 0, &pDataObject);
    ok(hr == OLE_E_NOTRUNNING,
       "IOleObject_GetClipboardData should have returned OLE_E_NOTRUNNING instead of 0x%08x\n",
       hr);

    hr = IOleObject_GetExtent(pObject, DVASPECT_CONTENT, &sizel);
    ok(hr == OLE_E_BLANK, "IOleObject_GetExtent should have returned OLE_E_BLANK instead of 0x%08x\n",
       hr);

    hr = IOleObject_GetMiscStatus(pObject, DVASPECT_CONTENT, &dwStatus);
    ok(hr == REGDB_E_CLASSNOTREG, "IOleObject_GetMiscStatus should have returned REGDB_E_CLASSNOTREG instead of 0x%08x\n", hr);

    hr = IOleObject_GetUserClassID(pObject, &clsid);
    ok_ole_success(hr, "IOleObject_GetUserClassID");
    ok(IsEqualCLSID(&clsid, &CLSID_WineTest), "clsid != CLSID_WineTest\n");

    hr = IOleObject_GetUserType(pObject, USERCLASSTYPE_FULL, &pszUserType);
    todo_wine {
    ok_ole_success(hr, "IOleObject_GetUserType");
    ok(!lstrcmpW(pszUserType, wszUnknown), "Retrieved user type was wrong\n");
    }

    hr = IOleObject_InitFromData(pObject, NULL, TRUE, 0);
    ok(hr == OLE_E_NOTRUNNING, "IOleObject_InitFromData should have returned OLE_E_NOTRUNNING instead of 0x%08x\n", hr);

    hr = IOleObject_IsUpToDate(pObject);
    ok(hr == OLE_E_NOTRUNNING, "IOleObject_IsUpToDate should have returned OLE_E_NOTRUNNING instead of 0x%08x\n", hr);

    palette.palNumEntries = 1;
    palette.palVersion = 2;
    memset(&palette.palPalEntry[0], 0, sizeof(palette.palPalEntry[0]));
    hr = IOleObject_SetColorScheme(pObject, &palette);
    ok(hr == OLE_E_NOTRUNNING, "IOleObject_SetColorScheme should have returned OLE_E_NOTRUNNING instead of 0x%08x\n", hr);

    sizel.cx = sizel.cy = 0;
    hr = IOleObject_SetExtent(pObject, DVASPECT_CONTENT, &sizel);
    ok(hr == OLE_E_NOTRUNNING, "IOleObject_SetExtent should have returned OLE_E_NOTRUNNING instead of 0x%08x\n", hr);

    hr = IOleObject_SetHostNames(pObject, wszHostName, NULL);
    ok_ole_success(hr, "IOleObject_SetHostNames");

    hr = CreateItemMoniker(wszDelim, wszHostName, &pMoniker);
    ok_ole_success(hr, "CreateItemMoniker");
    hr = IOleObject_SetMoniker(pObject, OLEWHICHMK_CONTAINER, pMoniker);
    ok_ole_success(hr, "IOleObject_SetMoniker");
    IMoniker_Release(pMoniker);

    hr = IOleObject_GetMoniker(pObject, OLEGETMONIKER_ONLYIFTHERE, OLEWHICHMK_CONTAINER, &pMoniker);
    ok(hr == E_FAIL, "IOleObject_GetMoniker should have returned E_FAIL instead of 0x%08x\n", hr);

    hr = IOleObject_Update(pObject);
    todo_wine
    ok(hr == REGDB_E_CLASSNOTREG, "IOleObject_Update should have returned REGDB_E_CLASSNOTREG instead of 0x%08x\n", hr);

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
    ok(hr == OLE_E_NOTRUNNING, "IDataObject_QueryGetData should have returned OLE_E_NOTRUNNING instead of 0x%08x\n", hr);

    fmtetc.cfFormat = CF_TEXT;
    fmtetc.ptd = NULL;
    fmtetc.dwAspect = DVASPECT_CONTENT;
    fmtetc.lindex = -1;
    fmtetc.tymed = TYMED_NULL;
    hr = IDataObject_QueryGetData(pDataObject, &fmtetc);
    ok(hr == OLE_E_NOTRUNNING, "IDataObject_QueryGetData should have returned OLE_E_NOTRUNNING instead of 0x%08x\n", hr);

    hr = IOleObject_QueryInterface(pObject, &IID_IRunnableObject, (void **)&pRunnableObject);
    ok_ole_success(hr, "IOleObject_QueryInterface");

    hr = IRunnableObject_SetContainedObject(pRunnableObject, TRUE);
    ok_ole_success(hr, "IRunnableObject_SetContainedObject");

    hr = IRunnableObject_Run(pRunnableObject, NULL);
    ok(hr == REGDB_E_CLASSNOTREG, "IOleObject_Run should have returned REGDB_E_CLASSNOTREG instead of 0x%08x\n", hr);

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
            ok(hr == E_FAIL, "Got 0x%08x\n", hr);

            g_QIFailsWith = E_NOINTERFACE;
            hr = IOleObject_QueryInterface(pObject, &IID_WineTest, (void**)&punk);
            ok(hr == E_NOINTERFACE, "Got 0x%08x\n", hr);

            g_QIFailsWith = CO_E_OBJNOTCONNECTED;
            hr = IOleObject_QueryInterface(pObject, &IID_WineTest, (void**)&punk);
            ok(hr == CO_E_OBJNOTCONNECTED, "Got 0x%08x\n", hr);

            g_QIFailsWith = 0x87654321;
            hr = IOleObject_QueryInterface(pObject, &IID_WineTest, (void**)&punk);
            ok(hr == 0x87654321, "Got 0x%08x\n", hr);

            IOleObject_Release(pObject);
        }

        CHECK_NO_EXTRA_METHODS();

        hr = CoRevokeClassObject(dwRegister);
        ok_ole_success(hr, "CoRevokeClassObject");
    }
}

static void test_runnable(void)
{
    static const struct expected_method methods_query_runnable[] =
    {
        { "OleObject_QueryInterface", 0 },
        { "OleObjectRunnable_AddRef", 0 },
        { "OleObjectRunnable_IsRunning", 0 },
        { "OleObjectRunnable_Release", 0 },
        { NULL, 0 }
    };

    static const struct expected_method methods_no_runnable[] =
    {
        { "OleObject_QueryInterface", 0 },
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

    g_isRunning = FALSE;
    expected_method_list = methods_query_runnable;
    ret = OleIsRunning(object);
    ok(ret == FALSE, "Object should not be running\n");
    CHECK_NO_EXTRA_METHODS();

    g_showRunnable = FALSE;  /* QueryInterface(IID_IRunnableObject, ...) will fail */
    expected_method_list = methods_no_runnable;
    ret = OleIsRunning(object);
    ok(ret == TRUE, "Object without IRunnableObject should be running\n");
    CHECK_NO_EXTRA_METHODS();

    g_isRunning = TRUE;
    g_showRunnable = TRUE;
}

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

static IUnknown unknown = { &UnknownVtbl };
static IRunnableObject testrunnable = { &oleruntestvtbl };

static void test_OleRun(void)
{
    HRESULT hr;

    /* doesn't support IRunnableObject */
    hr = OleRun(&unknown);
    ok(hr == S_OK, "OleRun failed 0x%08x\n", hr);

    hr = OleRun((IUnknown*)&testrunnable);
    ok(hr == 0xdeadc0de, "got 0x%08x\n", hr);
}

static void test_OleLockRunning(void)
{
    HRESULT hr;

    hr = OleLockRunning((LPUNKNOWN)&unknown, TRUE, FALSE);
    ok(hr == S_OK, "OleLockRunning failed 0x%08x\n", hr);
}

static void test_OleDraw(void)
{
    HRESULT hr;
    RECT rect;

    hr = OleDraw((IUnknown*)&viewobject, 0, (HDC)0x1, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = OleDraw(NULL, 0, (HDC)0x1, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = OleDraw(NULL, 0, (HDC)0x1, &rect);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
}

static const WCHAR comp_objW[] = {1,'C','o','m','p','O','b','j',0};
static IStream *comp_obj_stream;
static IStream *ole_stream;

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

    CHECK_EXPECT(Storage_CreateStream_CompObj);
    ok(!lstrcmpW(pwcsName, comp_objW), "pwcsName = %s\n", wine_dbgstr_w(pwcsName));
    todo_wine ok(grfMode == (STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE), "grfMode = %x\n", grfMode);
    ok(!reserved1, "reserved1 = %x\n", reserved1);
    ok(!reserved2, "reserved2 = %x\n", reserved2);
    ok(!!ppstm, "ppstm = NULL\n");

    *ppstm = comp_obj_stream;
    IStream_AddRef(comp_obj_stream);
    hr = IStream_Seek(comp_obj_stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek returned %x\n", hr);
    hr = IStream_SetSize(comp_obj_stream, size);
    ok(hr == S_OK, "IStream_SetSize returned %x\n", hr);
    return S_OK;
}

static HRESULT WINAPI Storage_OpenStream(IStorage *iface, LPCOLESTR pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm)
{
    static  const WCHAR ole1W[] = {1,'O','l','e',0};

    LARGE_INTEGER pos = {{0}};
    HRESULT hr;

    ok(!reserved1, "reserved1 = %p\n", reserved1);
    ok(!reserved2, "reserved2 = %x\n", reserved2);
    ok(!!ppstm, "ppstm = NULL\n");

    if(!lstrcmpW(pwcsName, comp_objW)) {
        CHECK_EXPECT2(Storage_OpenStream_CompObj);
        ok(grfMode == STGM_SHARE_EXCLUSIVE, "grfMode = %x\n", grfMode);

        *ppstm = comp_obj_stream;
        IStream_AddRef(comp_obj_stream);
        hr = IStream_Seek(comp_obj_stream, pos, STREAM_SEEK_SET, NULL);
        ok(hr == S_OK, "IStream_Seek returned %x\n", hr);
        return S_OK;
    }else if(!lstrcmpW(pwcsName, ole1W)) {
        CHECK_EXPECT(Storage_OpenStream_Ole);
        ok(grfMode == (STGM_SHARE_EXCLUSIVE|STGM_READWRITE), "grfMode = %x\n", grfMode);

        *ppstm = ole_stream;
        IStream_AddRef(ole_stream);
        hr = IStream_Seek(ole_stream, pos, STREAM_SEEK_SET, NULL);
        ok(hr == S_OK, "IStream_Seek returned %x\n", hr);
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
    ok(0, "unexpected call to DestroyElement\n");
    return E_NOTIMPL;
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
    ok(IsEqualIID(clsid, &CLSID_WineTest), "clsid = %s\n", wine_dbgstr_guid(clsid));
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
    ok(grfStatFlag == STATFLAG_NONAME, "grfStatFlag = %x\n", grfStatFlag);

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
    ok(hr == S_OK, "CreateStreamOnHGlobal returned %x\n", hr);
    hr = IStream_Write(comp_obj_stream, (char*)&comp_obj_data, sizeof(comp_obj_data), NULL);
    ok(hr == S_OK, "IStream_Write returned %x\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &ole_stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal returned %x\n", hr);
    hr = IStream_Write(ole_stream, (char*)&ole_data, sizeof(ole_data), NULL);
    ok(hr == S_OK, "IStream_Write returned %x\n", hr);

    clsid = IID_WineTest;
    hr = OleDoAutoConvert(NULL, &clsid);
    ok(hr == E_INVALIDARG, "OleDoAutoConvert returned %x\n", hr);
    ok(IsEqualIID(&clsid, &IID_NULL), "clsid = %s\n", wine_dbgstr_guid(&clsid));

    if(0) /* crashes on Win7 */
        OleDoAutoConvert(&Storage, NULL);

    clsid = IID_WineTest;
    SET_EXPECT(Storage_Stat);
    hr = OleDoAutoConvert(&Storage, &clsid);
    ok(hr == REGDB_E_CLASSNOTREG, "OleDoAutoConvert returned %x\n", hr);
    CHECK_CALLED(Storage_Stat);
    ok(IsEqualIID(&clsid, &CLSID_WineTestOld), "clsid = %s\n", wine_dbgstr_guid(&clsid));

    lstrcpyW(buf, clsidW);
    StringFromGUID2(&CLSID_WineTestOld, buf+6, 39);

    ret = RegCreateKeyExW(HKEY_CLASSES_ROOT, buf, 0, NULL, 0,
            KEY_READ | KEY_WRITE | KEY_CREATE_SUB_KEY, NULL, &root, NULL);
    if(ret != ERROR_SUCCESS) {
        win_skip("not enough permissions to create CLSID key (%u)\n", ret);
        return;
    }

    clsid = IID_WineTest;
    SET_EXPECT(Storage_Stat);
    hr = OleDoAutoConvert(&Storage, &clsid);
    ok(hr == REGDB_E_KEYMISSING, "OleDoAutoConvert returned %x\n", hr);
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
    ok(hr == S_OK, "OleDoAutoConvert returned %x\n", hr);
    CHECK_CALLED(Storage_Stat);
    CHECK_CALLED(Storage_OpenStream_CompObj);
    CHECK_CALLED(Storage_SetClass);
    CHECK_CALLED(Storage_CreateStream_CompObj);
    CHECK_CALLED(Storage_OpenStream_Ole);
    ok(IsEqualIID(&clsid, &CLSID_WineTest), "clsid = %s\n", wine_dbgstr_guid(&clsid));

    hr = IStream_Seek(comp_obj_stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek returned %x\n", hr);
    hr = IStream_Read(comp_obj_stream, &comp_obj_data, sizeof(comp_obj_data), NULL);
    ok(hr == S_OK, "IStream_Read returned %x\n", hr);
    ok(comp_obj_data.reserved1 == 0xfffe0001, "reserved1 = %x\n", comp_obj_data.reserved1);
    ok(comp_obj_data.version == 0xa03, "version = %x\n", comp_obj_data.version);
    ok(comp_obj_data.reserved2[0] == -1, "reserved2[0] = %x\n", comp_obj_data.reserved2[0]);
    ok(IsEqualIID(comp_obj_data.reserved2+1, &CLSID_WineTestOld), "reserved2 = %s\n", wine_dbgstr_guid((CLSID*)(comp_obj_data.reserved2+1)));
    ok(!comp_obj_data.ansi_user_type_len, "ansi_user_type_len = %d\n", comp_obj_data.ansi_user_type_len);
    ok(!comp_obj_data.ansi_clipboard_format_len, "ansi_clipboard_format_len = %d\n", comp_obj_data.ansi_clipboard_format_len);
    ok(!comp_obj_data.reserved3, "reserved3 = %x\n", comp_obj_data.reserved3);
    ok(comp_obj_data.unicode_marker == 0x71b239f4, "unicode_marker = %x\n", comp_obj_data.unicode_marker);
    ok(!comp_obj_data.unicode_user_type_len, "unicode_user_type_len = %d\n", comp_obj_data.unicode_user_type_len);
    ok(!comp_obj_data.unicode_clipboard_format_len, "unicode_clipboard_format_len = %d\n", comp_obj_data.unicode_clipboard_format_len);
    ok(!comp_obj_data.reserved4, "reserved4 %d\n", comp_obj_data.reserved4);

    hr = IStream_Seek(ole_stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek returned %x\n", hr);
    hr = IStream_Read(ole_stream, &ole_data, sizeof(ole_data), NULL);
    ok(hr == S_OK, "IStream_Read returned %x\n", hr);
    ok(ole_data.version == 0, "version = %x\n", ole_data.version);
    ok(ole_data.flags == 4, "flags = %x\n", ole_data.flags);
    for(i=2; i<sizeof(ole_data)/sizeof(DWORD); i++)
        ok(((DWORD*)&ole_data)[i] == 0, "ole_data[%d] = %x\n", i, ((DWORD*)&ole_data)[i]);

    SET_EXPECT(Storage_OpenStream_Ole);
    hr = SetConvertStg(&Storage, TRUE);
    ok(hr == S_OK, "SetConvertStg returned %x\n", hr);
    CHECK_CALLED(Storage_OpenStream_Ole);

    SET_EXPECT(Storage_OpenStream_CompObj);
    SET_EXPECT(Storage_Stat);
    SET_EXPECT(Storage_CreateStream_CompObj);
    hr = WriteFmtUserTypeStg(&Storage, 0, NULL);
    ok(hr == S_OK, "WriteFmtUserTypeStg returned %x\n", hr);
    todo_wine CHECK_CALLED(Storage_OpenStream_CompObj);
    CHECK_CALLED(Storage_Stat);
    CHECK_CALLED(Storage_CreateStream_CompObj);
    hr = IStream_Seek(comp_obj_stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek returned %x\n", hr);
    hr = IStream_Read(comp_obj_stream, &comp_obj_data, sizeof(comp_obj_data), NULL);
    ok(hr == S_OK, "IStream_Read returned %x\n", hr);
    ok(comp_obj_data.reserved1 == 0xfffe0001, "reserved1 = %x\n", comp_obj_data.reserved1);
    ok(comp_obj_data.version == 0xa03, "version = %x\n", comp_obj_data.version);
    ok(comp_obj_data.reserved2[0] == -1, "reserved2[0] = %x\n", comp_obj_data.reserved2[0]);
    ok(IsEqualIID(comp_obj_data.reserved2+1, &CLSID_WineTestOld), "reserved2 = %s\n", wine_dbgstr_guid((CLSID*)(comp_obj_data.reserved2+1)));
    ok(!comp_obj_data.ansi_user_type_len, "ansi_user_type_len = %d\n", comp_obj_data.ansi_user_type_len);
    ok(!comp_obj_data.ansi_clipboard_format_len, "ansi_clipboard_format_len = %d\n", comp_obj_data.ansi_clipboard_format_len);
    ok(!comp_obj_data.reserved3, "reserved3 = %x\n", comp_obj_data.reserved3);
    ok(comp_obj_data.unicode_marker == 0x71b239f4, "unicode_marker = %x\n", comp_obj_data.unicode_marker);
    ok(!comp_obj_data.unicode_user_type_len, "unicode_user_type_len = %d\n", comp_obj_data.unicode_user_type_len);
    ok(!comp_obj_data.unicode_clipboard_format_len, "unicode_clipboard_format_len = %d\n", comp_obj_data.unicode_clipboard_format_len);
    ok(!comp_obj_data.reserved4, "reserved4 %d\n", comp_obj_data.reserved4);

    ret = IStream_Release(comp_obj_stream);
    ok(!ret, "comp_obj_stream was not freed\n");
    ret = IStream_Release(ole_stream);
    ok(!ret, "ole_stream was not freed\n");

    ret = RegDeleteKeyA(root, "AutoConvertTo");
    ok(ret == ERROR_SUCCESS, "RegDeleteKey error %u\n", ret);
    ret = RegDeleteKeyA(root, "");
    ok(ret == ERROR_SUCCESS, "RegDeleteKey error %u\n", ret);
    RegCloseKey(root);
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

    test_data_cache();
    test_data_cache_dib_contents_stream( 0 );
    test_data_cache_dib_contents_stream( 1 );
    test_default_handler();
    test_runnable();
    test_OleRun();
    test_OleLockRunning();
    test_OleDraw();
    test_OleDoAutoConvert();

    CoUninitialize();
}
