/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for LockServer
 * COPYRIGHT:   Copyright 2026 Alex Mendoza <05alex.mendozaa@gmail.com>
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
    if (!pcf)
    {
        skip("pcf is NULL, skipping LockServer tests\n");
        return;
    }

    ok(refDll == 1, "Expected refDll == 1 after init, got %ld\n", refDll);

    hr = pcf->LockServer(TRUE);
    ok(hr == S_OK, "LockServer() failed: %08lx\n", hr);
    ok(refDll == 2, "Expected refDll == 2 after lock, got %ld\n", refDll);

    hr = pcf->LockServer(FALSE);
    ok(hr == S_OK, "LockServer() failed: %08lx\n", hr);
    ok(refDll == 1, "Expected refDll == 1 after unlock, got %ld\n", refDll);

    hr = pcf->LockServer(FALSE);
    ok(hr == S_OK, "LockServer() failed: %08lx\n", hr);
    ok(refDll >= 0, "refDll went negative after unlock when not locked: %ld\n", refDll);

    hr = pcf->LockServer(TRUE);
    ok(hr == S_OK, "LockServer() failed: %08lx\n", hr);
    hr = pcf->LockServer(TRUE);
    ok(hr == S_OK, "LockServer() failed: %08lx\n", hr);
    hr = pcf->LockServer(FALSE);
    ok(hr == S_OK, "LockServer() failed: %08lx\n", hr);
    hr = pcf->LockServer(FALSE);
    ok(hr == S_OK, "LockServer() failed: %08lx\n", hr);

    pcf->Release();
    ok(refDll == 0, "Expected refDll == 0 after Release, got %ld\n", refDll);
}

START_TEST(LockServer)
{
    test_LockServer();
}