/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for SHCreateDefClassObject / IClassFactory::LockServer
 * FILE:        modules/rostests/apitests/shell32/SHCreateDefClassObject.cpp
 */

#include "shelltest.h"
#include <undocshell.h>

static HRESULT CALLBACK CreateInstanceStub(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

static void test_LockServer(void)
{
    IClassFactory *pcf = NULL;
    LONG refDll = 0;
    HRESULT hr;

    hr = SHCreateDefClassObject(IID_IClassFactory,
                                (LPVOID*)&pcf,
                                CreateInstanceStub,
                                (LPDWORD)&refDll,
                                IID_IUnknown);
    ok(hr == S_OK, "SHCreateDefClassObject failed: %08lx\n", hr);
    ok(pcf != NULL, "pcf is NULL\n");
    if (!pcf) return;

    hr = pcf->LockServer(TRUE);
    ok(hr == S_OK, "LockServer(TRUE) returned %08lx, expected S_OK\n", hr);
    ok(refDll > 0, "pcRefDll not incremented after LockServer(TRUE), got %ld\n", refDll);

    hr = pcf->LockServer(FALSE);
    ok(hr == S_OK, "LockServer(FALSE) returned %08lx, expected S_OK\n", hr);
    ok(refDll >= 0, "pcRefDll went negative: %ld\n", refDll);

    hr = pcf->LockServer(TRUE);
    ok(hr == S_OK, "LockServer(TRUE) cycle 2: %08lx\n", hr);
    hr = pcf->LockServer(TRUE);
    ok(hr == S_OK, "LockServer(TRUE) cycle 3: %08lx\n", hr);
    hr = pcf->LockServer(FALSE);
    ok(hr == S_OK, "LockServer(FALSE) cycle 2: %08lx\n", hr);
    hr = pcf->LockServer(FALSE);
    ok(hr == S_OK, "LockServer(FALSE) cycle 3: %08lx\n", hr);

    pcf->Release();
}

START_TEST(LockServer)
{
    test_LockServer();
}