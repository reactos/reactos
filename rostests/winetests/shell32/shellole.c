/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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
#define NONAMELESSUNION

#include <stdio.h>
#include <wine/test.h>

#include "winbase.h"
#include "shlobj.h"
#include "initguid.h"

DEFINE_GUID(FMTID_Test,0x12345678,0x1234,0x1234,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12);
DEFINE_GUID(FMTID_NotExisting, 0x12345678,0x1234,0x1234,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x13);
DEFINE_GUID(CLSID_ClassMoniker, 0x0000031a,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

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

DEFINE_EXPECT(Create);
DEFINE_EXPECT(Delete);
DEFINE_EXPECT(Open);
DEFINE_EXPECT(ReadMultiple);
DEFINE_EXPECT(ReadMultipleCodePage);
DEFINE_EXPECT(Release);
DEFINE_EXPECT(Stat);
DEFINE_EXPECT(WriteMultiple);

DEFINE_EXPECT(autoplay_BindToObject);
DEFINE_EXPECT(autoplay_GetClassObject);

static HRESULT (WINAPI *pSHPropStgCreate)(IPropertySetStorage*, REFFMTID, const CLSID*,
        DWORD, DWORD, DWORD, IPropertyStorage**, UINT*);
static HRESULT (WINAPI *pSHPropStgReadMultiple)(IPropertyStorage*, UINT,
        ULONG, const PROPSPEC*, PROPVARIANT*);
static HRESULT (WINAPI *pSHPropStgWriteMultiple)(IPropertyStorage*, UINT*,
        ULONG, const PROPSPEC*, PROPVARIANT*, PROPID);
static HRESULT (WINAPI *pSHCreateQueryCancelAutoPlayMoniker)(IMoniker**);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("shell32.dll");

    pSHPropStgCreate = (void*)GetProcAddress(hmod, "SHPropStgCreate");
    pSHPropStgReadMultiple = (void*)GetProcAddress(hmod, "SHPropStgReadMultiple");
    pSHPropStgWriteMultiple = (void*)GetProcAddress(hmod, "SHPropStgWriteMultiple");
    pSHCreateQueryCancelAutoPlayMoniker = (void*)GetProcAddress(hmod, "SHCreateQueryCancelAutoPlayMoniker");
}

static HRESULT WINAPI PropertyStorage_QueryInterface(IPropertyStorage *This,
        REFIID riid, void **ppvObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI PropertyStorage_AddRef(IPropertyStorage *This)
{
    ok(0, "unexpected call\n");
    return 2;
}

static ULONG WINAPI PropertyStorage_Release(IPropertyStorage *This)
{
    CHECK_EXPECT(Release);
    return 1;
}

static HRESULT WINAPI PropertyStorage_ReadMultiple(IPropertyStorage *This, ULONG cpspec,
        const PROPSPEC *rgpspec, PROPVARIANT *rgpropvar)
{
    if(cpspec == 1) {
        CHECK_EXPECT(ReadMultipleCodePage);

        ok(rgpspec != NULL, "rgpspec = NULL\n");
        ok(rgpropvar != NULL, "rgpropvar = NULL\n");

        ok(rgpspec[0].ulKind == PRSPEC_PROPID, "rgpspec[0].ulKind = %d\n", rgpspec[0].ulKind);
        ok(rgpspec[0].u.propid == PID_CODEPAGE, "rgpspec[0].propid = %d\n", rgpspec[0].u.propid);

        rgpropvar[0].vt = VT_I2;
        rgpropvar[0].u.iVal = 1234;
    } else {
        CHECK_EXPECT(ReadMultiple);

        ok(cpspec == 10, "cpspec = %u\n", cpspec);
        ok(rgpspec == (void*)0xdeadbeef, "rgpspec = %p\n", rgpspec);
        ok(rgpropvar != NULL, "rgpropvar = NULL\n");

        ok(rgpropvar[0].vt==0 || broken(rgpropvar[0].vt==VT_BSTR), "rgpropvar[0].vt = %d\n", rgpropvar[0].vt);

        rgpropvar[0].vt = VT_BSTR;
        rgpropvar[0].u.bstrVal = (void*)0xdeadbeef;
        rgpropvar[1].vt = VT_LPSTR;
        rgpropvar[1].u.pszVal = (void*)0xdeadbeef;
        rgpropvar[2].vt = VT_BYREF|VT_I1;
        rgpropvar[2].u.pcVal = (void*)0xdeadbeef;
        rgpropvar[3].vt = VT_BYREF|VT_VARIANT;
        rgpropvar[3].u.pvarVal = (void*)0xdeadbeef;
    }

    return S_OK;
}

static HRESULT WINAPI PropertyStorage_WriteMultiple(IPropertyStorage *This, ULONG cpspec,
        const PROPSPEC *rgpspec, const PROPVARIANT *rgpropvar,
        PROPID propidNameFirst)
{
    CHECK_EXPECT(WriteMultiple);

    ok(cpspec == 20, "cpspec = %d\n", cpspec);
    ok(rgpspec == (void*)0xdeadbeef, "rgpspec = %p\n", rgpspec);
    ok(rgpropvar == (void*)0xdeadbeef, "rgpropvar = %p\n", rgpspec);
    ok(propidNameFirst == PID_FIRST_USABLE, "propidNameFirst = %d\n", propidNameFirst);
    return S_OK;
}

static HRESULT WINAPI PropertyStorage_DeleteMultiple(IPropertyStorage *This, ULONG cpspec,
        const PROPSPEC *rgpspec)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_ReadPropertyNames(IPropertyStorage *This, ULONG cpropid,
        const PROPID *rgpropid, LPOLESTR *rglpwstrName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_WritePropertyNames(IPropertyStorage *This, ULONG cpropid,
        const PROPID *rgpropid, const LPOLESTR *rglpwstrName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_DeletePropertyNames(IPropertyStorage *This, ULONG cpropid,
        const PROPID *rgpropid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_Commit(IPropertyStorage *This, DWORD grfCommitFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_Revert(IPropertyStorage *This)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_Enum(IPropertyStorage *This, IEnumSTATPROPSTG **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_SetTimes(IPropertyStorage *This, const FILETIME *pctime,
        const FILETIME *patime, const FILETIME *pmtime)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_SetClass(IPropertyStorage *This, REFCLSID clsid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_Stat(IPropertyStorage *This, STATPROPSETSTG *statpsstg)
{
    CHECK_EXPECT(Stat);

    memset(statpsstg, 0, sizeof(STATPROPSETSTG));
    memcpy(&statpsstg->fmtid, &FMTID_Test, sizeof(FMTID));
    statpsstg->grfFlags = PROPSETFLAG_ANSI;
    return S_OK;
}

static IPropertyStorageVtbl PropertyStorageVtbl = {
    PropertyStorage_QueryInterface,
    PropertyStorage_AddRef,
    PropertyStorage_Release,
    PropertyStorage_ReadMultiple,
    PropertyStorage_WriteMultiple,
    PropertyStorage_DeleteMultiple,
    PropertyStorage_ReadPropertyNames,
    PropertyStorage_WritePropertyNames,
    PropertyStorage_DeletePropertyNames,
    PropertyStorage_Commit,
    PropertyStorage_Revert,
    PropertyStorage_Enum,
    PropertyStorage_SetTimes,
    PropertyStorage_SetClass,
    PropertyStorage_Stat
};

static IPropertyStorage PropertyStorage = { &PropertyStorageVtbl };

static HRESULT WINAPI PropertySetStorage_QueryInterface(IPropertySetStorage *This,
        REFIID riid, void **ppvObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI PropertySetStorage_AddRef(IPropertySetStorage *This)
{
    ok(0, "unexpected call\n");
    return 2;
}

static ULONG WINAPI PropertySetStorage_Release(IPropertySetStorage *This)
{
    ok(0, "unexpected call\n");
    return 1;
}

static HRESULT WINAPI PropertySetStorage_Create(IPropertySetStorage *This,
        REFFMTID rfmtid, const CLSID *pclsid, DWORD grfFlags,
        DWORD grfMode, IPropertyStorage **ppprstg)
{
    CHECK_EXPECT(Create);
    ok(IsEqualGUID(rfmtid, &FMTID_Test) || IsEqualGUID(rfmtid, &FMTID_NotExisting),
            "Incorrect rfmtid value\n");
    ok(pclsid == NULL, "pclsid != NULL\n");
    ok(grfFlags == PROPSETFLAG_ANSI, "grfFlags = %x\n", grfFlags);
    ok(grfMode == STGM_READ, "grfMode = %x\n", grfMode);

    *ppprstg = &PropertyStorage;
    return S_OK;
}

static HRESULT WINAPI PropertySetStorage_Open(IPropertySetStorage *This,
        REFFMTID rfmtid, DWORD grfMode, IPropertyStorage **ppprstg)
{
    CHECK_EXPECT(Open);

    if(IsEqualGUID(rfmtid, &FMTID_Test)) {
        ok(grfMode == STGM_READ, "grfMode = %x\n", grfMode);

        *ppprstg = &PropertyStorage;
        return S_OK;
    }

    return STG_E_FILENOTFOUND;
}

static HRESULT WINAPI PropertySetStorage_Delete(IPropertySetStorage *This,
        REFFMTID rfmtid)
{
    CHECK_EXPECT(Delete);
    ok(IsEqualGUID(rfmtid, &FMTID_Test), "wrong rfmtid value\n");
    return S_OK;
}

static HRESULT WINAPI PropertySetStorage_Enum(IPropertySetStorage *This,
        IEnumSTATPROPSETSTG **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IPropertySetStorageVtbl PropertySetStorageVtbl = {
    PropertySetStorage_QueryInterface,
    PropertySetStorage_AddRef,
    PropertySetStorage_Release,
    PropertySetStorage_Create,
    PropertySetStorage_Open,
    PropertySetStorage_Delete,
    PropertySetStorage_Enum
};

static IPropertySetStorage PropertySetStorage = { &PropertySetStorageVtbl };

static void test_SHPropStg_functions(void)
{
    IPropertyStorage *property_storage;
    UINT codepage;
    PROPVARIANT read[10];
    HRESULT hres;

    if(!pSHPropStgCreate || !pSHPropStgReadMultiple || !pSHPropStgWriteMultiple) {
        win_skip("SHPropStg* functions are missing\n");
        return;
    }

    if(0) {
        /* Crashes on Windows */
        pSHPropStgCreate(NULL, &FMTID_Test, NULL, PROPSETFLAG_DEFAULT,
                STGM_READ, OPEN_EXISTING, &property_storage, &codepage);
        pSHPropStgCreate(&PropertySetStorage, NULL, NULL, PROPSETFLAG_DEFAULT,
                STGM_READ, OPEN_EXISTING, &property_storage, &codepage);
        pSHPropStgCreate(&PropertySetStorage, &FMTID_Test, NULL, PROPSETFLAG_DEFAULT,
                STGM_READ, OPEN_EXISTING, NULL, &codepage);
    }

    SET_EXPECT(Open);
    SET_EXPECT(ReadMultipleCodePage);
    hres = pSHPropStgCreate(&PropertySetStorage, &FMTID_Test, NULL, PROPSETFLAG_DEFAULT,
            STGM_READ, OPEN_EXISTING, &property_storage, &codepage);
    ok(codepage == 1234, "codepage = %d\n", codepage);
    ok(hres == S_OK, "hres = %x\n", hres);
    CHECK_CALLED(Open);
    CHECK_CALLED(ReadMultipleCodePage);

    SET_EXPECT(Open);
    hres = pSHPropStgCreate(&PropertySetStorage, &FMTID_NotExisting, NULL,
            PROPSETFLAG_DEFAULT, STGM_READ, OPEN_EXISTING, &property_storage, &codepage);
    ok(hres == STG_E_FILENOTFOUND, "hres = %x\n", hres);
    CHECK_CALLED(Open);

    SET_EXPECT(Open);
    SET_EXPECT(Release);
    SET_EXPECT(Delete);
    SET_EXPECT(Create);
    SET_EXPECT(ReadMultipleCodePage);
    hres = pSHPropStgCreate(&PropertySetStorage, &FMTID_Test, NULL, PROPSETFLAG_ANSI,
            STGM_READ, CREATE_ALWAYS, &property_storage, &codepage);
    ok(codepage == 1234, "codepage = %d\n", codepage);
    ok(hres == S_OK, "hres = %x\n", hres);
    CHECK_CALLED(Open);
    CHECK_CALLED(Release);
    CHECK_CALLED(Delete);
    CHECK_CALLED(Create);
    CHECK_CALLED(ReadMultipleCodePage);

    SET_EXPECT(Open);
    SET_EXPECT(Create);
    SET_EXPECT(ReadMultipleCodePage);
    hres = pSHPropStgCreate(&PropertySetStorage, &FMTID_NotExisting, NULL, PROPSETFLAG_ANSI,
            STGM_READ, CREATE_ALWAYS, &property_storage, &codepage);
    ok(codepage == 1234, "codepage = %d\n", codepage);
    ok(hres == S_OK, "hres = %x\n", hres);
    CHECK_CALLED(Open);
    CHECK_CALLED(Create);
    CHECK_CALLED(ReadMultipleCodePage);

    SET_EXPECT(Open);
    hres = pSHPropStgCreate(&PropertySetStorage, &FMTID_Test, &FMTID_NotExisting,
            PROPSETFLAG_DEFAULT, STGM_READ, OPEN_EXISTING, &property_storage, NULL);
    ok(hres == S_OK, "hres = %x\n", hres);
    CHECK_CALLED(Open);

    SET_EXPECT(Stat);
    SET_EXPECT(ReadMultipleCodePage);
    SET_EXPECT(WriteMultiple);
    codepage = 0;
    hres = pSHPropStgWriteMultiple(property_storage, &codepage, 20, (void*)0xdeadbeef, (void*)0xdeadbeef, PID_FIRST_USABLE);
    ok(hres == S_OK, "hres = %x\n", hres);
    ok(codepage == 1234, "codepage = %d\n", codepage);
    CHECK_CALLED(Stat);
    CHECK_CALLED(ReadMultipleCodePage);
    CHECK_CALLED(WriteMultiple);

    SET_EXPECT(Stat);
    SET_EXPECT(ReadMultipleCodePage);
    SET_EXPECT(WriteMultiple);
    hres = pSHPropStgWriteMultiple(property_storage, NULL, 20, (void*)0xdeadbeef, (void*)0xdeadbeef, PID_FIRST_USABLE);
    ok(hres == S_OK, "hres = %x\n", hres);
    CHECK_CALLED(Stat);
    CHECK_CALLED(ReadMultipleCodePage);
    CHECK_CALLED(WriteMultiple);

    SET_EXPECT(Stat);
    SET_EXPECT(WriteMultiple);
    codepage = 1000;
    hres = pSHPropStgWriteMultiple(property_storage, &codepage, 20, (void*)0xdeadbeef, (void*)0xdeadbeef, PID_FIRST_USABLE);
    ok(hres == S_OK, "hres = %x\n", hres);
    ok(codepage == 1000, "codepage = %d\n", codepage);
    CHECK_CALLED(Stat);
    CHECK_CALLED(WriteMultiple);

    read[0].vt = VT_BSTR;
    read[0].u.bstrVal = (void*)0xdeadbeef;
    SET_EXPECT(ReadMultiple);
    SET_EXPECT(ReadMultipleCodePage);
    SET_EXPECT(Stat);
    hres = pSHPropStgReadMultiple(property_storage, 0, 10, (void*)0xdeadbeef, read);
    ok(hres == S_OK, "hres = %x\n", hres);
    CHECK_CALLED(ReadMultiple);
    CHECK_CALLED(ReadMultipleCodePage);
    CHECK_CALLED(Stat);

    SET_EXPECT(ReadMultiple);
    SET_EXPECT(Stat);
    hres = pSHPropStgReadMultiple(property_storage, 1251, 10, (void*)0xdeadbeef, read);
    ok(hres == S_OK, "hres = %x\n", hres);
    CHECK_CALLED(ReadMultiple);
    CHECK_CALLED(Stat);
}

static HRESULT WINAPI test_activator_QI(IClassActivator *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassActivator))
    {
        *ppv = iface;
    }

    if (!*ppv) return E_NOINTERFACE;

    IClassActivator_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI test_activator_AddRef(IClassActivator *iface)
{
    return 2;
}

static ULONG WINAPI test_activator_Release(IClassActivator *iface)
{
    return 1;
}

static HRESULT WINAPI test_activator_GetClassObject(IClassActivator *iface, REFCLSID clsid,
    DWORD context, LCID locale, REFIID riid, void **ppv)
{
    CHECK_EXPECT(autoplay_GetClassObject);
    ok(IsEqualGUID(clsid, &CLSID_QueryCancelAutoPlay), "clsid %s\n", wine_dbgstr_guid(clsid));
    ok(IsEqualIID(riid, &IID_IQueryCancelAutoPlay), "riid %s\n", wine_dbgstr_guid(riid));
    return E_NOTIMPL;
}

static const IClassActivatorVtbl test_activator_vtbl = {
    test_activator_QI,
    test_activator_AddRef,
    test_activator_Release,
    test_activator_GetClassObject
};

static IClassActivator test_activator = { &test_activator_vtbl };

static HRESULT WINAPI test_moniker_QueryInterface(IMoniker* iface, REFIID riid, void **ppvObject)
{
    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IPersist, riid) ||
        IsEqualIID(&IID_IPersistStream, riid) ||
        IsEqualIID(&IID_IMoniker, riid))
    {
        *ppvObject = iface;
    }

    if (!*ppvObject)
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI test_moniker_AddRef(IMoniker* iface)
{
    return 2;
}

static ULONG WINAPI test_moniker_Release(IMoniker* iface)
{
    return 1;
}

static HRESULT WINAPI test_moniker_GetClassID(IMoniker* iface, CLSID *pClassID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_IsDirty(IMoniker* iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Load(IMoniker* iface, IStream* pStm)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_BindToObject(IMoniker* iface,
                                                IBindCtx* pbc,
                                                IMoniker* moniker_to_left,
                                                REFIID riid,
                                                void** ppv)
{
    CHECK_EXPECT(autoplay_BindToObject);
    ok(pbc != NULL, "got %p\n", pbc);
    ok(moniker_to_left == NULL, "got %p\n", moniker_to_left);
    ok(IsEqualIID(riid, &IID_IClassActivator), "got riid %s\n", wine_dbgstr_guid(riid));

    if (IsEqualIID(riid, &IID_IClassActivator))
    {
        *ppv = &test_activator;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_BindToStorage(IMoniker* iface,
                                             IBindCtx* pbc,
                                             IMoniker* pmkToLeft,
                                             REFIID riid,
                                             VOID** ppvResult)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Reduce(IMoniker* iface,
                                      IBindCtx* pbc,
                                      DWORD dwReduceHowFar,
                                      IMoniker** ppmkToLeft,
                                      IMoniker** ppmkReduced)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_ComposeWith(IMoniker* iface,
                                           IMoniker* pmkRight,
                                           BOOL fOnlyIfNotGeneric,
                                           IMoniker** ppmkComposite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_IsEqual(IMoniker* iface, IMoniker* pmkOtherMoniker)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Hash(IMoniker* iface, DWORD* pdwHash)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_IsRunning(IMoniker* iface,
                                         IBindCtx* pbc,
                                         IMoniker* pmkToLeft,
                                         IMoniker* pmkNewlyRunning)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_GetTimeOfLastChange(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   FILETIME* pItemTime)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Inverse(IMoniker* iface, IMoniker** ppmk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,IMoniker** ppmkPrefix)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_GetDisplayName(IMoniker* iface,
                                              IBindCtx* pbc,
                                              IMoniker* pmkToLeft,
                                              LPOLESTR *ppszDisplayName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_ParseDisplayName(IMoniker* iface,
                                                IBindCtx* pbc,
                                                IMoniker* pmkToLeft,
                                                LPOLESTR pszDisplayName,
                                                ULONG* pchEaten,
                                                IMoniker** ppmkOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IMonikerVtbl test_moniker_vtbl =
{
    test_moniker_QueryInterface,
    test_moniker_AddRef,
    test_moniker_Release,
    test_moniker_GetClassID,
    test_moniker_IsDirty,
    test_moniker_Load,
    test_moniker_Save,
    test_moniker_GetSizeMax,
    test_moniker_BindToObject,
    test_moniker_BindToStorage,
    test_moniker_Reduce,
    test_moniker_ComposeWith,
    test_moniker_Enum,
    test_moniker_IsEqual,
    test_moniker_Hash,
    test_moniker_IsRunning,
    test_moniker_GetTimeOfLastChange,
    test_moniker_Inverse,
    test_moniker_CommonPrefixWith,
    test_moniker_RelativePathTo,
    test_moniker_GetDisplayName,
    test_moniker_ParseDisplayName,
    test_moniker_IsSystemMoniker
};

static IMoniker test_moniker = { &test_moniker_vtbl };

static void test_SHCreateQueryCancelAutoPlayMoniker(void)
{
    IBindCtx *ctxt;
    IMoniker *mon;
    IUnknown *unk;
    CLSID clsid;
    HRESULT hr;
    DWORD sys;

    if (!pSHCreateQueryCancelAutoPlayMoniker)
    {
        win_skip("SHCreateQueryCancelAutoPlayMoniker is not available, skipping tests.\n");
        return;
    }

    hr = pSHCreateQueryCancelAutoPlayMoniker(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = pSHCreateQueryCancelAutoPlayMoniker(&mon);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    sys = -1;
    hr = IMoniker_IsSystemMoniker(mon, &sys);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(sys == MKSYS_CLASSMONIKER, "got %d\n", sys);

    memset(&clsid, 0, sizeof(clsid));
    hr = IMoniker_GetClassID(mon, &clsid);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualGUID(&clsid, &CLSID_ClassMoniker), "got %s\n", wine_dbgstr_guid(&clsid));

    /* extract used CLSID that implements this hook */
    SET_EXPECT(autoplay_BindToObject);
    SET_EXPECT(autoplay_GetClassObject);

    CreateBindCtx(0, &ctxt);
    hr = IMoniker_BindToObject(mon, ctxt, &test_moniker, &IID_IQueryCancelAutoPlay, (void**)&unk);
    ok(hr == E_NOTIMPL, "got 0x%08x\n", hr);
    IBindCtx_Release(ctxt);

    CHECK_CALLED(autoplay_BindToObject);
    CHECK_CALLED(autoplay_GetClassObject);

    IMoniker_Release(mon);
}

#define DROPTEST_FILENAME "c:\\wintest.bin"
struct DragParam {
    HWND hwnd;
    HANDLE ready;
};

static LRESULT WINAPI drop_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wparam;
        char filename[MAX_PATH] = "dummy";
        UINT num;
        num = DragQueryFileA(hDrop, 0xffffffff, NULL, 0);
        ok(num == 1, "expected 1, got %u\n", num);
        num = DragQueryFileA(hDrop, 0xffffffff, (char*)0xdeadbeef, 0xffffffff);
        ok(num == 1, "expected 1, got %u\n", num);
        num = DragQueryFileA(hDrop, 0, filename, sizeof(filename));
        ok(num == strlen(DROPTEST_FILENAME), "got %u\n", num);
        ok(!strcmp(filename, DROPTEST_FILENAME), "got %s\n", filename);
        DragFinish(hDrop);
        return 0;
    }
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static DWORD WINAPI drop_window_therad(void *arg)
{
    struct DragParam *param = arg;
    WNDCLASSA cls;
    WINDOWINFO info;
    BOOL r;
    MSG msg;

    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = drop_window_proc;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = "drop test";
    RegisterClassA(&cls);

    param->hwnd = CreateWindowA("drop test", NULL, 0, 0, 0, 0, 0,
                                NULL, 0, NULL, 0);
    ok(param->hwnd != NULL, "CreateWindow failed: %d\n", GetLastError());

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    r = GetWindowInfo(param->hwnd, &info);
    ok(r, "got %d\n", r);
    ok(!(info.dwExStyle & WS_EX_ACCEPTFILES), "got %08x\n", info.dwExStyle);

    DragAcceptFiles(param->hwnd, TRUE);

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    r = GetWindowInfo(param->hwnd, &info);
    ok(r, "got %d\n", r);
    ok((info.dwExStyle & WS_EX_ACCEPTFILES), "got %08x\n", info.dwExStyle);

    SetEvent(param->ready);

    while ((r = GetMessageA(&msg, NULL, 0, 0)) != 0) {
        if (r == (BOOL)-1) {
            ok(0, "unexpected return value, got %d\n", r);
            break;
        }
        DispatchMessageA(&msg);
    }

    DestroyWindow(param->hwnd);
    UnregisterClassA("drop test", GetModuleHandleA(NULL));
    return 0;
}

static void test_DragQueryFile(void)
{
    struct DragParam param;
    HANDLE hThread;
    DWORD rc;
    HGLOBAL hDrop;
    DROPFILES *pDrop;
    int ret;
    BOOL r;

    param.ready = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(param.ready != NULL, "can't create event\n");
    hThread = CreateThread(NULL, 0, drop_window_therad, &param, 0, NULL);

    rc = WaitForSingleObject(param.ready, 5000);
    ok(rc == WAIT_OBJECT_0, "got %u\n", rc);

    hDrop = GlobalAlloc(GHND, sizeof(DROPFILES) + (strlen(DROPTEST_FILENAME) + 2) * sizeof(WCHAR));
    pDrop = GlobalLock(hDrop);
    pDrop->pFiles = sizeof(DROPFILES);
    ret = MultiByteToWideChar(CP_ACP, 0, DROPTEST_FILENAME, -1,
                              (LPWSTR)(pDrop + 1), strlen(DROPTEST_FILENAME) + 1);
    ok(ret > 0, "got %d\n", ret);
    pDrop->fWide = TRUE;
    GlobalUnlock(hDrop);

    r = PostMessageA(param.hwnd, WM_DROPFILES, (WPARAM)hDrop, 0);
    ok(r, "got %d\n", r);

    r = PostMessageA(param.hwnd, WM_QUIT, 0, 0);
    ok(r, "got %d\n", r);

    rc = WaitForSingleObject(hThread, 5000);
    ok(rc == WAIT_OBJECT_0, "got %d\n", rc);

    CloseHandle(param.ready);
    CloseHandle(hThread);
}
#undef DROPTEST_FILENAME

START_TEST(shellole)
{
    init();

    test_SHPropStg_functions();
    test_SHCreateQueryCancelAutoPlayMoniker();
    test_DragQueryFile();
}
