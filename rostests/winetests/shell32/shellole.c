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

static HRESULT (WINAPI *pSHPropStgCreate)(IPropertySetStorage*, REFFMTID, const CLSID*,
        DWORD, DWORD, DWORD, IPropertyStorage**, UINT*);
static HRESULT (WINAPI *pSHPropStgReadMultiple)(IPropertyStorage*, UINT,
        ULONG, const PROPSPEC*, PROPVARIANT*);
static HRESULT (WINAPI *pSHPropStgWriteMultiple)(IPropertyStorage*, UINT*,
        ULONG, const PROPSPEC*, PROPVARIANT*, PROPID);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("shell32.dll");

    pSHPropStgCreate = (void*)GetProcAddress(hmod, "SHPropStgCreate");
    pSHPropStgReadMultiple = (void*)GetProcAddress(hmod, "SHPropStgReadMultiple");
    pSHPropStgWriteMultiple = (void*)GetProcAddress(hmod, "SHPropStgWriteMultiple");
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

START_TEST(shellole)
{
    init();

    test_SHPropStg_functions();
}
