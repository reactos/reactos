/* Unit tests for autocomplete
 *
 * Copyright 2007 Mikolaj Zalewski
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

#include <assert.h>
#include <stdarg.h>

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "wine/test.h"

#define stop_on_error(exp) \
{ \
    HRESULT res = (exp); \
    if (FAILED(res)) \
    { \
        ok(FALSE, #exp " failed: %x\n", res); \
        return; \
    } \
}

#define ole_ok(exp) \
{ \
    HRESULT res = (exp); \
    if (res != S_OK) \
        ok(FALSE, #exp " failed: %x\n", res); \
}

LPWSTR strdup_AtoW(LPCSTR str)
{
    int size = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    LPWSTR wstr = (LPWSTR)CoTaskMemAlloc((size + 1)*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, size+1);
    return wstr;
}

typedef struct
{
    IEnumStringVtbl *vtbl;
    IACListVtbl *aclVtbl;
    LONG ref;
    HRESULT expret;
    INT expcount;
    INT pos;
    INT limit;
    const char **data;
} TestACL;

extern IEnumStringVtbl TestACLVtbl;
extern IACListVtbl TestACL_ACListVtbl;

static TestACL *impl_from_IACList(IACList *iface)
{
    return (TestACL *)((char *)iface - FIELD_OFFSET(TestACL, aclVtbl));
}

static TestACL *TestACL_Constructor(int limit, const char **strings)
{
    TestACL *This = CoTaskMemAlloc(sizeof(TestACL));
    ZeroMemory(This, sizeof(*This));
    This->vtbl = &TestACLVtbl;
    This->aclVtbl = &TestACL_ACListVtbl;
    This->ref = 1;
    This->expret = S_OK;
    This->limit = limit;
    This->data = strings;
    return This;
}

ULONG STDMETHODCALLTYPE TestACL_AddRef(IEnumString *iface)
{
    TestACL *This = (TestACL *)iface;
    trace("ACL(%p): addref (%d)\n", This, This->ref+1);
    return InterlockedIncrement(&This->ref);
}

ULONG STDMETHODCALLTYPE TestACL_Release(IEnumString *iface)
{
    TestACL *This = (TestACL *)iface;
    ULONG res;

    res = InterlockedDecrement(&This->ref);
    trace("ACL(%p): release (%d)\n", This, res);
    return res;
}

HRESULT STDMETHODCALLTYPE TestACL_QueryInterface(IEnumString *iface, REFIID iid, LPVOID *ppvOut)
{
    TestACL *This = (TestACL *)iface;
    *ppvOut = NULL;
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IEnumString))
    {
        *ppvOut = iface;
    }
    else if (IsEqualGUID(iid, &IID_IACList))
    {
        *ppvOut = &This->aclVtbl;
    }

    if (*ppvOut)
    {
        iface->lpVtbl->AddRef(iface);
        return S_OK;
    }

#if 0   /* IID_IEnumACString not defined yet in wine */
    if (!IsEqualGUID(iid, &IID_IEnumACString))
        trace("unknown interface queried\n");
#endif
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE TestACL_Next(IEnumString *iface, ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    TestACL *This = (TestACL *)iface;
    ULONG i;

    trace("ACL(%p): read %d item(s)\n", This, celt);
    for (i = 0; i < celt; i++)
    {
        if (This->pos >= This->limit)
            break;
        rgelt[i] = strdup_AtoW(This->data[This->pos]);
        This->pos++;
    }

    if (pceltFetched)
        *pceltFetched = i;
    if (i == celt)
        return S_OK;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE TestACL_Skip(IEnumString *iface, ULONG celt)
{
    ok(FALSE, "Unexpected call to TestACL_Skip\n");
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TestACL_Clone(IEnumString *iface, IEnumString **out)
{
    ok(FALSE, "Unexpected call to TestACL_Clone\n");
    return E_OUTOFMEMORY;
}

HRESULT STDMETHODCALLTYPE TestACL_Reset(IEnumString *iface)
{
    TestACL *This = (TestACL *)iface;
    trace("ACL(%p): Reset\n", This);
    This->pos = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestACL_Expand(IACList *iface, LPCOLESTR str)
{
    TestACL *This = impl_from_IACList(iface);
    trace("ACL(%p): Expand\n", impl_from_IACList(iface));
    This->expcount++;
    return This->expret;
}

IEnumStringVtbl TestACLVtbl =
{
    TestACL_QueryInterface,
    TestACL_AddRef,
    TestACL_Release,

    TestACL_Next,
    TestACL_Skip,
    TestACL_Reset,
    TestACL_Clone
};

ULONG STDMETHODCALLTYPE TestACL_ACList_AddRef(IACList *iface)
{
    return TestACL_AddRef((IEnumString *)impl_from_IACList(iface));
}

ULONG STDMETHODCALLTYPE TestACL_ACList_Release(IACList *iface)
{
    return TestACL_Release((IEnumString *)impl_from_IACList(iface));
}

HRESULT STDMETHODCALLTYPE TestACL_ACList_QueryInterface(IACList *iface, REFIID iid, LPVOID *ppvout)
{
    return TestACL_QueryInterface((IEnumString *)impl_from_IACList(iface), iid, ppvout);
}

IACListVtbl TestACL_ACListVtbl =
{
    TestACL_ACList_QueryInterface,
    TestACL_ACList_AddRef,
    TestACL_ACList_Release,

    TestACL_Expand
};

#define expect_str(obj, str)  \
{ \
    ole_ok(obj->lpVtbl->Next(obj, 1, &wstr, &i)); \
    ok(i == 1, "Expected i == 1, got %d\n", i); \
    ok(str[0] == wstr[0], "String mismatch\n"); \
}

#define expect_end(obj) \
    ok(obj->lpVtbl->Next(obj, 1, &wstr, &i) == S_FALSE, "Unexpected return from Next\n");

static void test_ACLMulti(void)
{
    const char *strings1[] = {"a", "c", "e"};
    const char *strings2[] = {"a", "b", "d"};
    WCHAR exp[] = {'A','B','C',0};
    IEnumString *obj;
    TestACL *acl1, *acl2;
    IACList *acl;
    IObjMgr *mgr;
    LPWSTR wstr;
    LPWSTR wstrtab[15];
    LPVOID tmp;
    UINT i;

    stop_on_error(CoCreateInstance(&CLSID_ACLMulti, NULL, CLSCTX_INPROC, &IID_IEnumString, (LPVOID *)&obj));
    stop_on_error(obj->lpVtbl->QueryInterface(obj, &IID_IACList, (LPVOID *)&acl));
    ok(obj->lpVtbl->QueryInterface(obj, &IID_IACList2, &tmp) == E_NOINTERFACE,
        "Unexpected interface IACList2 in ACLMulti\n");
    stop_on_error(obj->lpVtbl->QueryInterface(obj, &IID_IObjMgr, (LPVOID *)&mgr));
#if 0        /* IID_IEnumACString not defined yet in wine */
    ole_ok(obj->lpVtbl->QueryInterface(obj, &IID_IEnumACString, &unk));
    if (unk != NULL)
        unk->lpVtbl->Release(unk);
#endif

    ok(obj->lpVtbl->Next(obj, 1, (LPOLESTR *)&tmp, &i) == S_FALSE, "Unexpected return from Next\n");
    ok(i == 0, "Unexpected fetched value %d\n", i);
    ok(obj->lpVtbl->Next(obj, 44, (LPOLESTR *)&tmp, &i) == S_FALSE, "Unexpected return from Next\n");
    ok(obj->lpVtbl->Skip(obj, 1) == E_NOTIMPL, "Unexpected return from Skip\n");
    ok(obj->lpVtbl->Clone(obj, (IEnumString **)&tmp) == E_OUTOFMEMORY, "Unexpected return from Clone\n");
    ole_ok(acl->lpVtbl->Expand(acl, exp));

    acl1 = TestACL_Constructor(3, strings1);
    acl2 = TestACL_Constructor(3, strings2);
    stop_on_error(mgr->lpVtbl->Append(mgr, (IUnknown *)acl1));
    stop_on_error(mgr->lpVtbl->Append(mgr, (IUnknown *)acl2));
    ok(mgr->lpVtbl->Append(mgr, NULL) == E_FAIL, "Unexpected return from Append\n");
    expect_str(obj, "a");
    expect_str(obj, "c");
    expect_str(obj, "e");
    expect_str(obj, "a");
    expect_str(obj, "b");
    expect_str(obj, "d");
    expect_end(obj);

    ole_ok(obj->lpVtbl->Reset(obj));
    ok(acl1->pos == 0, "acl1 not reset\n");
    ok(acl2->pos == 0, "acl2 not reset\n");

    ole_ok(acl->lpVtbl->Expand(acl, exp));
    ok(acl1->expcount == 1, "expcount - expected 1, got %d\n", acl1->expcount);
    ok(acl2->expcount == 0, "expcount - expected 0, got %d\n", acl2->expcount);

    ole_ok(obj->lpVtbl->Next(obj, 15, wstrtab, &i));
    ok(i == 1, "Expected i == 1, got %d\n", i);
    ole_ok(obj->lpVtbl->Next(obj, 15, wstrtab, &i));
    ole_ok(obj->lpVtbl->Next(obj, 15, wstrtab, &i));
    ole_ok(obj->lpVtbl->Next(obj, 15, wstrtab, &i));
    ole_ok(acl->lpVtbl->Expand(acl, exp));
    ok(acl1->expcount == 2, "expcount - expected 1, got %d\n", acl1->expcount);
    ok(acl2->expcount == 0, "expcount - expected 0, got %d\n", acl2->expcount);
    acl1->expret = S_FALSE;
    ole_ok(acl->lpVtbl->Expand(acl, exp));
    ok(acl1->expcount == 3, "expcount - expected 1, got %d\n", acl1->expcount);
    ok(acl2->expcount == 1, "expcount - expected 0, got %d\n", acl2->expcount);
    acl1->expret = E_NOTIMPL;
    ole_ok(acl->lpVtbl->Expand(acl, exp));
    ok(acl1->expcount == 4, "expcount - expected 1, got %d\n", acl1->expcount);
    ok(acl2->expcount == 2, "expcount - expected 0, got %d\n", acl2->expcount);
    acl2->expret = E_OUTOFMEMORY;
    ok(acl->lpVtbl->Expand(acl, exp) == E_OUTOFMEMORY, "Unexpected Expand return\n");
    acl2->expret = E_FAIL;
    ok(acl->lpVtbl->Expand(acl, exp) == E_FAIL, "Unexpected Expand return\n");

    stop_on_error(mgr->lpVtbl->Remove(mgr, (IUnknown *)acl1));
    ok(acl1->ref == 1, "acl1 not released\n");
    expect_end(obj);
    obj->lpVtbl->Reset(obj);
    expect_str(obj, "a");
    expect_str(obj, "b");
    expect_str(obj, "d");
    expect_end(obj);

    obj->lpVtbl->Release(obj);
    acl->lpVtbl->Release(acl);
    ok(mgr->lpVtbl->Release(mgr) == 0, "Unexpected references\n");
    ok(acl1->ref == 1, "acl1 not released\n");
    ok(acl2->ref == 1, "acl2 not released\n");
}

START_TEST(autocomplete)
{
    CoInitialize(NULL);
    test_ACLMulti();
    CoUninitialize();
}
