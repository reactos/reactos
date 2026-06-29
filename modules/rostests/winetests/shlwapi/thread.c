/* Tests for Thread and SHGlobalCounter functions
 *
 * Copyright 2010 Detlef Riekenberg
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

#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS
#define CONST_VTABLE

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "ole2.h"
#include "shlwapi.h"

#include "wine/test.h"

static HRESULT (WINAPI *pSHCreateThreadRef)(LONG*, IUnknown**);
static HRESULT (WINAPI *pSHGetThreadRef)(IUnknown**);
static HRESULT (WINAPI *pSHSetThreadRef)(IUnknown*);

static DWORD AddRef_called;

typedef struct
{
  IUnknown IUnknown_iface;
  LONG  *ref;
} threadref;

static inline threadref *impl_from_IUnknown(IUnknown *iface)
{
  return CONTAINING_RECORD(iface, threadref, IUnknown_iface);
}

static HRESULT WINAPI threadref_QueryInterface(IUnknown *iface, REFIID riid, LPVOID *ppvObj)
{
    threadref * This = impl_from_IUnknown(iface);

    trace("unexpected QueryInterface(%p, %s, %p) called\n", This, wine_dbgstr_guid(riid), ppvObj);
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI threadref_AddRef(IUnknown *iface)
{
    threadref * This = impl_from_IUnknown(iface);

    AddRef_called++;
    return InterlockedIncrement(This->ref);
}

static ULONG WINAPI threadref_Release(IUnknown *iface)
{
    threadref * This = impl_from_IUnknown(iface);

    trace("unexpected Release(%p) called\n", This);
    return InterlockedDecrement(This->ref);
}

/* VTable */
static const IUnknownVtbl threadref_vt =
{
  threadref_QueryInterface,
  threadref_AddRef,
  threadref_Release
};

static void init_threadref(threadref* iface, LONG *refcount)
{
  iface->IUnknown_iface.lpVtbl = &threadref_vt;
  iface->ref = refcount;
}

/* ##### */

static void test_SHCreateThreadRef(void)
{
    IUnknown *pobj;
    IUnknown *punk;
    LONG refcount;
    HRESULT hr;

    /* Not present before IE 6_XP_sp2 */
    if (!pSHCreateThreadRef) {
        win_skip("SHCreateThreadRef not found\n");
        return;
    }

    /* start with a clean state */
    hr = pSHSetThreadRef(NULL);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    pobj = NULL;
    refcount = 0xdeadbeef;
    hr = pSHCreateThreadRef(&refcount, &pobj);
    ok((hr == S_OK) && pobj && (refcount == 1),
        "got 0x%lx and %p with %ld (expected S_OK and '!= NULL' with 1)\n",
        hr, pobj, refcount);

    /* the object is not automatic set as ThreadRef */
    punk = NULL;
    hr = pSHGetThreadRef(&punk);
    ok( (hr == E_NOINTERFACE) && (punk == NULL),
        "got 0x%lx and %p (expected E_NOINTERFACE and NULL)\n", hr, punk);

    /* set the object */
    hr = pSHSetThreadRef(pobj);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    /* read back */
    punk = NULL;
    hr = pSHGetThreadRef(&punk);
    ok( (hr == S_OK) && (punk == pobj) && (refcount == 2),
        "got 0x%lx and %p with %ld (expected S_OK and %p with 2)\n",
        hr, punk, refcount, pobj);

    /* free the ref from SHGetThreadRef */
    if (SUCCEEDED(hr)) {
        hr = IUnknown_Release(pobj);
        ok((hr == 1) && (hr == refcount),
            "got %ld with %ld (expected 1 with 1)\n", hr, refcount);
    }

    /* free the object */
    if (pobj) {
        hr = IUnknown_Release(pobj);
        ok((hr == 0) && (hr == refcount),
            "got %ld with %ld (expected 0 with 0)\n", hr, refcount);
    }

    if (0) {
        /* the ThreadRef has still the pointer,
           but the object no longer exist after the *_Release */
        punk = NULL;
        hr = pSHGetThreadRef(&punk);
        trace("got 0x%lx and %p with %ld\n", hr, punk, refcount);
    }

    /* remove the dead object pointer */
    hr = pSHSetThreadRef(NULL);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    /* parameter check */
    if (0) {
        /* vista: E_INVALIDARG, XP: crash */
        pobj = NULL;
        hr = pSHCreateThreadRef(NULL, &pobj);
        ok(hr == E_INVALIDARG, "got 0x%lx (expected E_INVALIDARG)\n", hr);

        refcount = 0xdeadbeef;
        /* vista: E_INVALIDARG, XP: crash */
        hr = pSHCreateThreadRef(&refcount, NULL);
        ok( (hr == E_INVALIDARG) && (refcount == 0xdeadbeef),
            "got 0x%lx with 0x%lx (expected E_INVALIDARG and oxdeadbeef)\n",
            hr, refcount);
    }
}


static void test_SHGetThreadRef(void)
{
    IUnknown *punk;
    HRESULT hr;

    /* Not present before IE 5 */
    if (!pSHGetThreadRef) {
        win_skip("SHGetThreadRef not found\n");
        return;
    }

    punk = NULL;
    hr = pSHGetThreadRef(&punk);
    ok( (hr == E_NOINTERFACE) && (punk == NULL),
        "got 0x%lx and %p (expected E_NOINTERFACE and NULL)\n", hr, punk);

    if (0) {
        /* this crash on Windows */
        pSHGetThreadRef(NULL);
    }
}

static void test_SHSetThreadRef(void)
{
    threadref ref;
    IUnknown *punk;
    HRESULT hr;
    LONG refcount;

    /* Not present before IE 5 */
    if (!pSHSetThreadRef) {
        win_skip("SHSetThreadRef not found\n");
        return;
    }

    /* start with a clean state */
    hr = pSHSetThreadRef(NULL);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    /* build and set out object */
    init_threadref(&ref, &refcount);
    AddRef_called = 0;
    refcount = 1;
    hr = pSHSetThreadRef(&ref.IUnknown_iface);
    ok( (hr == S_OK) && (refcount == 1) && (!AddRef_called),
        "got 0x%lx with %ld, %ld (expected S_OK with 1, 0)\n",
        hr, refcount, AddRef_called);

    /* read back our object */
    AddRef_called = 0;
    refcount = 1;
    punk = NULL;
    hr = pSHGetThreadRef(&punk);
    ok( (hr == S_OK) && (punk == &ref.IUnknown_iface) && (refcount == 2) && (AddRef_called == 1),
        "got 0x%lx and %p with %ld, %ld (expected S_OK and %p with 2, 1)\n",
        hr, punk, refcount, AddRef_called, &ref);

    /* clear the object pointer */
    hr = pSHSetThreadRef(NULL);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    /* verify, that our object is no longer known as ThreadRef */
    hr = pSHGetThreadRef(&punk);
    ok( (hr == E_NOINTERFACE) && (punk == NULL),
        "got 0x%lx and %p (expected E_NOINTERFACE and NULL)\n", hr, punk);

}

START_TEST(thread)
{
    HMODULE hshlwapi = GetModuleHandleA("shlwapi.dll");

    pSHCreateThreadRef = (void *) GetProcAddress(hshlwapi, "SHCreateThreadRef");
    pSHGetThreadRef = (void *) GetProcAddress(hshlwapi, "SHGetThreadRef");
    pSHSetThreadRef = (void *) GetProcAddress(hshlwapi, "SHSetThreadRef");

    test_SHCreateThreadRef();
    test_SHGetThreadRef();
    test_SHSetThreadRef();

}
