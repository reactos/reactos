/*
 * PropVariant Tests
 *
 * Copyright 2004 Robert Shearman
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

#include "windows.h"
#include "wtypes.h"
#include "ddeml.h"

#include "wine/test.h"

/* invalid in all versions */
#define PROP_INV 0x7f
/* valid in v0 and above (NT4+) */
#define PROP_V0  0
/* valid in v1 and above (Win2k+) */
#define PROP_V1  1
/* valid in v1a and above (WinXP+) */
#define PROP_V1A 2
#define PROP_TODO 0x80

static const struct valid_mapping
{
    BYTE simple;
    BYTE with_array;
    BYTE with_vector;
    BYTE byref;
} valid_types[] =
{
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_EMPTY */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_NULL */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_I2 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_I4 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_R4 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_R8 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_CY */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_DATE */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_BSTR */
    { PROP_V1 , PROP_V1 , PROP_INV, PROP_V1 | PROP_TODO  }, /* VT_DISPATCH */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_ERROR */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_BOOL */
    { PROP_V1 | PROP_TODO , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_VARIANT */
    { PROP_V1 , PROP_V1, PROP_INV, PROP_V1 | PROP_TODO  }, /* VT_UNKNOWN */
    { PROP_V1 , PROP_V1 , PROP_INV, PROP_V1 | PROP_TODO  }, /* VT_DECIMAL */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 15 */
    { PROP_V1 , PROP_V1 , PROP_V1 , PROP_V1 | PROP_TODO  }, /* VT_I1 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_UI1 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_UI2 */
    { PROP_V0 , PROP_V1 , PROP_V0 , PROP_V1 | PROP_TODO  }, /* VT_UI4 */
    { PROP_V0 , PROP_V1A | PROP_TODO, PROP_V0 , PROP_V1A | PROP_TODO }, /* VT_I8 */
    { PROP_V0 , PROP_V1A | PROP_TODO, PROP_V0 , PROP_V1A | PROP_TODO }, /* VT_UI8 */
    { PROP_V1 , PROP_V1 , PROP_INV, PROP_V1 | PROP_TODO  }, /* VT_INT */
    { PROP_V1 , PROP_V1 , PROP_INV, PROP_V1 | PROP_TODO  }, /* VT_UINT */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_VOID */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_HRESULT */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_PTR */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_SAFEARRAY */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_CARRAY */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_USERDEFINED */
    { PROP_V0 , PROP_INV, PROP_V0 , PROP_INV }, /* VT_LPSTR */
    { PROP_V0 , PROP_INV, PROP_V0 , PROP_INV }, /* VT_LPWSTR */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 32 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 33 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 34 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 35 */
    { PROP_V1 | PROP_TODO , PROP_V1 | PROP_TODO , PROP_INV, PROP_V1 | PROP_TODO  }, /* VT_RECORD */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_INT_PTR */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* VT_UINT_PTR */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 39 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 40 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 41 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 42 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 43 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 44 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 45 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 46 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 47 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 48 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 49 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 50 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 51 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 52 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 53 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 54 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 55 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 56 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 57 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 58 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 59 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 60 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 61 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 62 */
    { PROP_INV, PROP_INV, PROP_INV, PROP_INV }, /* 63 */
    { PROP_V0 , PROP_INV, PROP_V0 , PROP_INV }, /* VT_FILETIME */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_BLOB */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_STREAM */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_STORAGE */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_STREAMED_OBJECT */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_STORED_OBJECT */
    { PROP_V0 , PROP_INV, PROP_INV, PROP_INV }, /* VT_BLOB_OBJECT */
    { PROP_V0 , PROP_INV, PROP_V0 , PROP_INV }  /* VT_CF */
};

static const char* wine_vtypes[VT_CLSID+1] =
{
  "VT_EMPTY","VT_NULL","VT_I2","VT_I4","VT_R4","VT_R8","VT_CY","VT_DATE",
  "VT_BSTR","VT_DISPATCH","VT_ERROR","VT_BOOL","VT_VARIANT","VT_UNKNOWN",
  "VT_DECIMAL","15","VT_I1","VT_UI1","VT_UI2","VT_UI4","VT_I8","VT_UI8",
  "VT_INT","VT_UINT","VT_VOID","VT_HRESULT","VT_PTR","VT_SAFEARRAY",
  "VT_CARRAY","VT_USERDEFINED","VT_LPSTR","VT_LPWSTR","32","33","34","35",
  "VT_RECORD","VT_INT_PTR","VT_UINT_PTR","39","40","41","42","43","44","45",
  "46","47","48","49","50","51","52","53","54","55","56","57","58","59","60",
  "61","62","63","VT_FILETIME","VT_BLOB","VT_STREAM","VT_STORAGE",
  "VT_STREAMED_OBJECT","VT_STORED_OBJECT","VT_BLOB_OBJECT","VT_CF","VT_CLSID"
};


static void expect(HRESULT hr, VARTYPE vt, BOOL copy, int line)
{
    int idx = vt & VT_TYPEMASK;
    BYTE flags;
    const char *modifier;

    if(vt & VT_BYREF)
    {
        flags = valid_types[idx].byref;
        modifier = "byref";
    }
    else if(vt & VT_ARRAY)
    {
        flags = valid_types[idx].with_array;
        modifier = "array";
    }
    else if(vt & VT_VECTOR)
    {
        flags = valid_types[idx].with_vector;
        modifier = "vector";
    }
    else
    {
        flags = valid_types[idx].simple;
        modifier = "simple";
    }

    if(flags == PROP_INV)
    {
        if (copy && (vt & VT_VECTOR))
            ok(hr == DISP_E_BADVARTYPE || hr == STG_E_INVALIDPARAMETER, "%s (%s): got %08lx (line %d)\n", wine_vtypes[idx], modifier, hr, line);
        else
            ok(hr == (copy ? DISP_E_BADVARTYPE : STG_E_INVALIDPARAMETER), "%s (%s): got %08lx (line %d)\n", wine_vtypes[idx], modifier, hr, line);
    }
    else if(flags == PROP_V0)
        ok(hr == S_OK, "%s (%s): got %08lx\n", wine_vtypes[idx], modifier, hr);
    else todo_wine_if(flags & PROP_TODO)
    {
        if(hr != S_OK)
            win_skip("%s (%s): unsupported\n", wine_vtypes[idx], modifier);
        else ok(hr == S_OK, "%s (%s): got %08lx\n", wine_vtypes[idx], modifier, hr);
    }
}

static void test_validtypes(void)
{
    PROPVARIANT propvar, copy, uninit;
    HRESULT hr;
    unsigned int i, ret;

    memset(&uninit, 0x77, sizeof(uninit));

    memset(&propvar, 0x55, sizeof(propvar));
    hr = PropVariantClear(&propvar);
    ok(hr == STG_E_INVALIDPARAMETER, "expected STG_E_INVALIDPARAMETER, got %08lx\n", hr);
    ok(propvar.vt == 0, "expected 0, got %d\n", propvar.vt);
    ok(propvar.uhVal.QuadPart == 0, "expected 0, got %#lx/%#lx\n",
       propvar.uhVal.u.LowPart, propvar.uhVal.u.HighPart);

    for (i = 0; i < ARRAY_SIZE(valid_types); i++)
    {
        VARTYPE vt;

        memset(&propvar, 0x55, sizeof(propvar));
        if (i == VT_RECORD)
            memset(&propvar, 0, sizeof(propvar));
        else if (i == VT_BLOB || i == VT_BLOB_OBJECT)
        {
            propvar.blob.cbSize = 0;
            propvar.blob.pBlobData = NULL;
        }
        else
            propvar.pszVal = NULL;
        vt = propvar.vt = i;
        memset(&copy, 0x77, sizeof(copy));
        hr = PropVariantCopy(&copy, &propvar);
        expect(hr, vt, TRUE, __LINE__);
        if (hr == S_OK)
        {
            ok(copy.vt == propvar.vt, "expected %d, got %d\n", propvar.vt, copy.vt);
            ok(copy.uhVal.QuadPart == propvar.uhVal.QuadPart, "%u: expected %#lx/%#lx, got %#lx/%#lx\n",
               i, propvar.uhVal.u.LowPart, propvar.uhVal.u.HighPart,
               copy.uhVal.u.LowPart, copy.uhVal.u.HighPart);
        }
        else
        {
            ret = memcmp(&copy, &uninit, sizeof(copy));
            ok(!ret || broken(ret) /* win2000 */, "%d: copy should stay unchanged\n", i);
        }
        hr = PropVariantClear(&propvar);
        expect(hr, vt, FALSE, __LINE__);
        ok(propvar.vt == 0, "expected 0, got %d\n", propvar.vt);
        ok(propvar.uhVal.QuadPart == 0, "%u: expected 0, got %#lx/%#lx\n",
           i, propvar.uhVal.u.LowPart, propvar.uhVal.u.HighPart);

        memset(&propvar, 0x55, sizeof(propvar));
        propvar.pszVal = NULL;
        vt = propvar.vt = i | VT_ARRAY;
        memset(&copy, 0x77, sizeof(copy));
        hr = PropVariantCopy(&copy, &propvar);
        expect(hr, vt, TRUE, __LINE__);
        if (hr == S_OK)
        {
            ok(copy.vt == propvar.vt, "expected %d, got %d\n", propvar.vt, copy.vt);
            ok(copy.uhVal.QuadPart == 0, "%u: expected 0, got %#lx/%#lx\n",
               i, copy.uhVal.u.LowPart, copy.uhVal.u.HighPart);
        }
        else
        {
            ret = memcmp(&copy, &uninit, sizeof(copy));
            ok(!ret || broken(ret) /* win2000 */, "%d: copy should stay unchanged\n", i);
        }
        hr = PropVariantClear(&propvar);
        expect(hr, vt, FALSE, __LINE__);
        ok(propvar.vt == 0, "expected 0, got %d\n", propvar.vt);
        ok(propvar.uhVal.QuadPart == 0, "%u: expected 0, got %#lx/%#lx\n",
           i, propvar.uhVal.u.LowPart, propvar.uhVal.u.HighPart);

        memset(&propvar, 0x55, sizeof(propvar));
        propvar.caub.cElems = 0;
        propvar.caub.pElems = NULL;
        vt = propvar.vt = i | VT_VECTOR;
        memset(&copy, 0x77, sizeof(copy));
        hr = PropVariantCopy(&copy, &propvar);
        expect(hr, vt, TRUE, __LINE__);
        if (hr == S_OK)
        {
            ok(copy.vt == propvar.vt, "expected %d, got %d\n", propvar.vt, copy.vt);
            ok(!copy.caub.cElems, "%u: expected 0, got %ld\n", i, copy.caub.cElems);
            ok(!copy.caub.pElems, "%u: expected NULL, got %p\n", i, copy.caub.pElems);
        }
        else
        {
            ret = memcmp(&copy, &uninit, sizeof(copy));
            ok(!ret || broken(ret) /* win2000 */, "%d: copy should stay unchanged\n", i);
        }
        hr = PropVariantClear(&propvar);
        expect(hr, vt, FALSE, __LINE__);
        ok(propvar.vt == 0, "expected 0, got %d\n", propvar.vt);
        ok(propvar.uhVal.QuadPart == 0, "%u: expected 0, got %#lx/%#lx\n",
           i, propvar.uhVal.u.LowPart, propvar.uhVal.u.HighPart);

        memset(&propvar, 0x55, sizeof(propvar));
        propvar.pszVal = NULL;
        vt = propvar.vt = i | VT_BYREF;
        memset(&copy, 0x77, sizeof(copy));
        hr = PropVariantCopy(&copy, &propvar);
        expect(hr, vt, TRUE, __LINE__);
        if (hr == S_OK)
        {
            ok(copy.vt == propvar.vt, "expected %d, got %d\n", propvar.vt, copy.vt);
            ok(copy.uhVal.QuadPart == propvar.uhVal.QuadPart, "%u: expected %#lx/%#lx, got %#lx/%#lx\n",
               i, propvar.uhVal.u.LowPart, propvar.uhVal.u.HighPart,
               copy.uhVal.u.LowPart, copy.uhVal.u.HighPart);
        }
        else
        {
            ret = memcmp(&copy, &uninit, sizeof(copy));
            ok(!ret || broken(ret) /* win2000 */, "%d: copy should stay unchanged\n", i);
        }
        hr = PropVariantClear(&propvar);
        expect(hr, vt, FALSE, __LINE__);
        ok(propvar.vt == 0, "expected 0, got %d\n", propvar.vt);
        ok(propvar.uhVal.QuadPart == 0, "%u: expected 0, got %#lx/%#lx\n",
           i, propvar.uhVal.u.LowPart, propvar.uhVal.u.HighPart);
    }
}

struct unk_impl
{
    IUnknown IUnknown_iface;
    LONG ref;
};

static inline struct unk_impl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct unk_impl, IUnknown_iface);
}

static HRESULT WINAPI unk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    struct unk_impl *This = impl_from_IUnknown(iface);
    if(winetest_debug > 1)
        trace("Call to unk_QueryInterface()\n");
    *ppv = &This->IUnknown_iface;
    IUnknown_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI unk_AddRef(IUnknown *iface)
{
    struct unk_impl *This = impl_from_IUnknown(iface);
    if(winetest_debug > 1)
        trace("Call to unk_AddRef()\n");
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI unk_Release(IUnknown *iface)
{
    struct unk_impl *This = impl_from_IUnknown(iface);
    if(winetest_debug > 1)
        trace("Call to unk_Release()\n");
    return InterlockedDecrement(&This->ref);
}

static const IUnknownVtbl unk_vtbl =
{
    unk_QueryInterface,
    unk_AddRef,
    unk_Release
};

static void test_copy(void)
{
    static char szTestString[] = "Test String";
    static WCHAR wszTestString[] = {'T','e','s','t',' ','S','t','r','i','n','g',0};
    struct unk_impl unk_obj = {{&unk_vtbl}, 1};
    PROPVARIANT propvarSrc;
    PROPVARIANT propvarDst;
    SAFEARRAY *sa;
    SAFEARRAYBOUND sabound;
    LONG saindex;
    HRESULT hr;

    propvarSrc.vt = VT_BSTR;
    propvarSrc.bstrVal = SysAllocString(wszTestString);

    hr = PropVariantCopy(&propvarDst, &propvarSrc);
    ok(hr == S_OK, "PropVariantCopy(...VT_BSTR...) failed\n");
    ok(!lstrcmpW(propvarSrc.bstrVal, propvarDst.bstrVal), "BSTR not copied properly\n");
    hr = PropVariantClear(&propvarSrc);
    ok(hr == S_OK, "PropVariantClear(...VT_BSTR...) failed\n");
    hr = PropVariantClear(&propvarDst);
    ok(hr == S_OK, "PropVariantClear(...VT_BSTR...) failed\n");

    /* BSTR with embedded null */
    propvarSrc.vt = VT_BSTR;
    propvarSrc.bstrVal = SysAllocStringLen(L"Test Str\0ing", 12);
    hr = PropVariantCopy(&propvarDst, &propvarSrc);
    ok(hr == S_OK, "Failed to copy propvar, hr %#lx.\n", hr);
    ok(SysStringLen(propvarDst.bstrVal) == 8, "Unexpected copy length.\n");
    ok(SysStringLen(propvarSrc.bstrVal) == 12, "Unexpected source length.\n");
    ok(!lstrcmpW(propvarSrc.bstrVal, propvarDst.bstrVal), "BSTR not copied properly\n");
    hr = PropVariantClear(&propvarSrc);
    ok(hr == S_OK, "Failed to clear propvar, hr %#lx.\n", hr);
    hr = PropVariantClear(&propvarDst);
    ok(hr == S_OK, "Failed to clear propvar, hr %#lx.\n", hr);

    propvarSrc.vt = VT_LPWSTR;
    propvarSrc.pwszVal = wszTestString;
    hr = PropVariantCopy(&propvarDst, &propvarSrc);
    ok(hr == S_OK, "PropVariantCopy(...VT_LPWSTR...) failed\n");
    ok(!lstrcmpW(propvarSrc.pwszVal, propvarDst.pwszVal), "Wide string not copied properly\n");
    hr = PropVariantClear(&propvarDst);
    ok(hr == S_OK, "PropVariantClear(...VT_LPWSTR...) failed\n");
    memset(&propvarSrc, 0, sizeof(propvarSrc));

    propvarSrc.vt = VT_LPSTR;
    propvarSrc.pszVal = szTestString;
    hr = PropVariantCopy(&propvarDst, &propvarSrc);
    ok(hr == S_OK, "PropVariantCopy(...VT_LPSTR...) failed\n");
    ok(!strcmp(propvarSrc.pszVal, propvarDst.pszVal), "String not copied properly\n");
    hr = PropVariantClear(&propvarDst);
    ok(hr == S_OK, "PropVariantClear(...VT_LPSTR...) failed\n");
    memset(&propvarSrc, 0, sizeof(propvarSrc));

    propvarSrc.vt = VT_UNKNOWN;
    propvarSrc.punkVal = &unk_obj.IUnknown_iface;
    hr = PropVariantCopy(&propvarDst, &propvarSrc);
    ok(hr == S_OK, "PropVariantCopy(...VT_UNKNOWN...) failed: 0x%08lx.\n", hr);
    ok(propvarDst.punkVal == &unk_obj.IUnknown_iface, "Got wrong IUnknown pointer\n");
    ok(unk_obj.ref == 2, "got wrong refcount: %ld.\n", unk_obj.ref);
    hr = PropVariantClear(&propvarDst);
    ok(hr == S_OK, "PropVariantClear(...VT_UNKNOWN...) failed: 0x%08lx.\n", hr);
    ok(unk_obj.ref == 1, "got wrong refcount: %ld.\n", unk_obj.ref);
    memset(&propvarSrc, 0, sizeof(propvarSrc));

    sabound.lLbound = 0;
    sabound.cElements = 2;
    sa = SafeArrayCreate(VT_UNKNOWN, 1, &sabound);
    saindex = 0;
    SafeArrayPutElement(sa, &saindex, &unk_obj.IUnknown_iface);
    saindex = 1;
    SafeArrayPutElement(sa, &saindex, &unk_obj.IUnknown_iface);
    ok(unk_obj.ref == 3, "got wrong refcount: %ld.\n", unk_obj.ref);

    propvarSrc.vt = VT_ARRAY | VT_UNKNOWN;
    propvarSrc.parray = sa;
    hr = PropVariantCopy(&propvarDst, &propvarSrc);
    ok(hr == S_OK, "PropVariantCopy(...VT_ARRAY|VT_UNKNOWN...) failed: 0x%08lx.\n", hr);
    ok(unk_obj.ref == 5, "got wrong refcount: %ld.\n", unk_obj.ref);
    hr = PropVariantClear(&propvarDst);
    ok(hr == S_OK, "PropVariantClear(...VT_ARRAY|VT_UNKNOWN...) failed: 0x%08lx.\n", hr);
    ok(unk_obj.ref == 3, "got wrong refcount: %ld.\n", unk_obj.ref);
    hr = PropVariantClear(&propvarSrc);
    ok(hr == S_OK, "PropVariantClear(...VT_ARRAY|VT_UNKNOWN...) failed: 0x%08lx.\n", hr);
    ok(unk_obj.ref == 1, "got wrong refcount: %ld.\n", unk_obj.ref);
    memset(&propvarSrc, 0, sizeof(propvarSrc));
}

struct _PMemoryAllocator_vtable {
    void *Allocate; /* virtual void* Allocate(ULONG cbSize); */
    void *Free; /* virtual void Free(void *pv); */
};

typedef struct _PMemoryAllocator {
    struct _PMemoryAllocator_vtable *vt;
} PMemoryAllocator;

static void * WINAPI PMemoryAllocator_Allocate(PMemoryAllocator *_this, ULONG cbSize)
{
    return CoTaskMemAlloc(cbSize);
}

static void WINAPI PMemoryAllocator_Free(PMemoryAllocator *_this, void *pv)
{
    CoTaskMemFree(pv);
}

#ifdef __i386__

#include "pshpack1.h"
typedef struct
{
    BYTE pop_eax;  /* popl  %eax  */
    BYTE push_ecx; /* pushl %ecx  */
    BYTE push_eax; /* pushl %eax  */
    BYTE jmp_func; /* jmp   $func */
    DWORD func;
} THISCALL_TO_STDCALL_THUNK;
#include "poppack.h"

static THISCALL_TO_STDCALL_THUNK *wrapperCodeMem = NULL;

static void fill_thunk(THISCALL_TO_STDCALL_THUNK *thunk, void *fn)
{
    thunk->pop_eax = 0x58;
    thunk->push_ecx = 0x51;
    thunk->push_eax = 0x50;
    thunk->jmp_func = 0xe9;
    thunk->func = (char*)fn - (char*)(&thunk->func + 1);
}

static void setup_vtable(struct _PMemoryAllocator_vtable *vtable)
{
    wrapperCodeMem = VirtualAlloc(NULL, 2 * sizeof(*wrapperCodeMem),
                                  MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    fill_thunk(&wrapperCodeMem[0], PMemoryAllocator_Allocate);
    fill_thunk(&wrapperCodeMem[1], PMemoryAllocator_Free);

    vtable->Allocate = &wrapperCodeMem[0];
    vtable->Free = &wrapperCodeMem[1];
}

#else

static void setup_vtable(struct _PMemoryAllocator_vtable *vtable)
{
    vtable->Allocate = PMemoryAllocator_Allocate;
    vtable->Free = PMemoryAllocator_Free;
}

#endif

static const char serialized_empty[] = {
    0,0, /* VT_EMPTY */
    0,0, /* padding */
};

static const char serialized_null[] = {
    1,0, /* VT_NULL */
    0,0, /* padding */
};

static const char serialized_i4[] = {
    3,0, /* VT_I4 */
    0,0, /* padding */
    0xef,0xcd,0xab,0xfe
};

static const char serialized_bstr_wc[] = {
    8,0, /* VT_BSTR */
    0,0, /* padding */
    10,0,0,0, /* size */
    't',0,'e',0,
    's',0,'t',0,
    0,0,0,0
};

static const char serialized_bstr_mb[] = {
    8,0, /* VT_BSTR */
    0,0, /* padding */
    5,0,0,0, /* size */
    't','e','s','t',
    0,0,0,0
};

static void test_propertytovariant(void)
{
    HANDLE hole32;
    BOOLEAN (__stdcall *pStgConvertPropertyToVariant)(const SERIALIZEDPROPERTYVALUE*,USHORT,PROPVARIANT*,PMemoryAllocator*);
    PROPVARIANT propvar;
    PMemoryAllocator allocator;
    struct _PMemoryAllocator_vtable vtable;
    BOOLEAN ret;
    static const WCHAR test_string[] = {'t','e','s','t',0};

    hole32 = GetModuleHandleA("ole32");

    pStgConvertPropertyToVariant = (void*)GetProcAddress(hole32, "StgConvertPropertyToVariant");

    if (!pStgConvertPropertyToVariant)
    {
        win_skip("StgConvertPropertyToVariant not available\n");
        return;
    }

    setup_vtable(&vtable);
    allocator.vt = &vtable;

    ret = pStgConvertPropertyToVariant((SERIALIZEDPROPERTYVALUE*)serialized_empty,
        CP_WINUNICODE, &propvar, &allocator);

    ok(ret == 0, "StgConvertPropertyToVariant returned %i\n", ret);
    ok(propvar.vt == VT_EMPTY, "unexpected vt %x\n", propvar.vt);

    ret = pStgConvertPropertyToVariant((SERIALIZEDPROPERTYVALUE*)serialized_null,
        CP_WINUNICODE, &propvar, &allocator);

    ok(ret == 0, "StgConvertPropertyToVariant returned %i\n", ret);
    ok(propvar.vt == VT_NULL, "unexpected vt %x\n", propvar.vt);

    ret = pStgConvertPropertyToVariant((SERIALIZEDPROPERTYVALUE*)serialized_i4,
        CP_WINUNICODE, &propvar, &allocator);

    ok(ret == 0, "StgConvertPropertyToVariant returned %i\n", ret);
    ok(propvar.vt == VT_I4, "unexpected vt %x\n", propvar.vt);
    ok(propvar.lVal == 0xfeabcdef, "unexpected lVal %lx\n", propvar.lVal);

    ret = pStgConvertPropertyToVariant((SERIALIZEDPROPERTYVALUE*)serialized_bstr_wc,
        CP_WINUNICODE, &propvar, &allocator);

    ok(ret == 0, "StgConvertPropertyToVariant returned %i\n", ret);
    ok(propvar.vt == VT_BSTR, "unexpected vt %x\n", propvar.vt);
    ok(!lstrcmpW(propvar.bstrVal, test_string), "unexpected string value\n");
    PropVariantClear(&propvar);

    ret = pStgConvertPropertyToVariant((SERIALIZEDPROPERTYVALUE*)serialized_bstr_mb,
        CP_UTF8, &propvar, &allocator);

    ok(ret == 0, "StgConvertPropertyToVariant returned %i\n", ret);
    ok(propvar.vt == VT_BSTR, "unexpected vt %x\n", propvar.vt);
    ok(!lstrcmpW(propvar.bstrVal, test_string), "unexpected string value\n");
    PropVariantClear(&propvar);
}

static void test_varianttoproperty(void)
{
    HANDLE hole32;
    PROPVARIANT propvar;
    SERIALIZEDPROPERTYVALUE *propvalue, *own_propvalue;
    SERIALIZEDPROPERTYVALUE* (__stdcall *pStgConvertVariantToProperty)(
        const PROPVARIANT*,USHORT,SERIALIZEDPROPERTYVALUE*,ULONG*,PROPID,BOOLEAN,ULONG*);
    ULONG len;
    static const WCHAR test_string[] = {'t','e','s','t',0};
    BSTR test_string_bstr;

    hole32 = GetModuleHandleA("ole32");

    pStgConvertVariantToProperty = (void*)GetProcAddress(hole32, "StgConvertVariantToProperty");

    if (!pStgConvertVariantToProperty)
    {
        win_skip("StgConvertVariantToProperty not available\n");
        return;
    }

    own_propvalue = malloc(sizeof(SERIALIZEDPROPERTYVALUE) + 20);

    PropVariantInit(&propvar);

    propvar.vt = VT_I4;
    propvar.lVal = 0xfeabcdef;

    len = 0xdeadbeef;
    propvalue = pStgConvertVariantToProperty(&propvar, CP_WINUNICODE, NULL, &len,
        0, FALSE, 0);

    ok(propvalue == NULL, "got nonnull propvalue\n");
    todo_wine ok(len == 8, "unexpected length %ld\n", len);

    if (len == 0xdeadbeef)
    {
        free(own_propvalue);
        return;
    }

    len = 20;
    propvalue = pStgConvertVariantToProperty(&propvar, CP_WINUNICODE, own_propvalue, &len,
        0, FALSE, 0);

    ok(propvalue == own_propvalue, "unexpected propvalue %p\n", propvalue);
    ok(len == 8, "unexpected length %ld\n", len);
    ok(!memcmp(propvalue, serialized_i4, 8), "got wrong data\n");

    propvar.vt = VT_EMPTY;
    len = 20;
    own_propvalue->dwType = 0xdeadbeef;
    propvalue = pStgConvertVariantToProperty(&propvar, CP_WINUNICODE, own_propvalue, &len,
        0, FALSE, 0);

    ok(propvalue == own_propvalue, "unexpected propvalue %p\n", propvalue);
    ok(len == 4 || broken(len == 0) /* before Vista */, "unexpected length %ld\n", len);
    if (len) ok(!memcmp(propvalue, serialized_empty, 4), "got wrong data\n");
    else ok(propvalue->dwType == 0xdeadbeef, "unexpected type %ld\n", propvalue->dwType);

    propvar.vt = VT_NULL;
    len = 20;
    propvalue = pStgConvertVariantToProperty(&propvar, CP_WINUNICODE, own_propvalue, &len,
        0, FALSE, 0);

    ok(propvalue == own_propvalue, "unexpected propvalue %p\n", propvalue);
    ok(len == 4, "unexpected length %ld\n", len);
    ok(!memcmp(propvalue, serialized_null, 4), "got wrong data\n");

    test_string_bstr = SysAllocString(test_string);

    propvar.vt = VT_BSTR;
    propvar.bstrVal = test_string_bstr;
    len = 20;
    propvalue = pStgConvertVariantToProperty(&propvar, CP_WINUNICODE, own_propvalue, &len,
        0, FALSE, 0);

    ok(propvalue == own_propvalue, "unexpected propvalue %p\n", propvalue);
    ok(len == 20, "unexpected length %ld\n", len);
    ok(!memcmp(propvalue, serialized_bstr_wc, 20), "got wrong data\n");

    len = 20;
    propvalue = pStgConvertVariantToProperty(&propvar, CP_UTF8, own_propvalue, &len,
        0, FALSE, 0);

    ok(propvalue == own_propvalue, "unexpected propvalue %p\n", propvalue);
    ok(len == 16, "unexpected length %ld\n", len);
    ok(!memcmp(propvalue, serialized_bstr_mb, 16), "got wrong data\n");

    SysFreeString(test_string_bstr);

    free(own_propvalue);
}

START_TEST(propvariant)
{
    test_validtypes();
    test_copy();
    test_propertytovariant();
    test_varianttoproperty();
}
