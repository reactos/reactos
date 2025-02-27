/*
 * Unit tests for IDxDiagProvider
 *
 * Copyright (C) 2009 Andrew Nguyen
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

#include "initguid.h"
#include "dxdiag.h"
#include "wine/test.h"

static void test_Initialize(void)
{
    HRESULT hr;
    IDxDiagProvider *pddp;
    DXDIAG_INIT_PARAMS params;

    hr = CoCreateInstance(&CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDxDiagProvider, (LPVOID*)&pddp);
    ok(hr == S_OK ||
       broken(hr == REGDB_E_CLASSNOTREG), /* Clean W2K3 */
       "Creating a IDxDiagProvider instance failed with %lx\n", hr);
    if (FAILED(hr))
    {
        skip("Failed to create a IDxDiagProvider instance\n");
        return;
    }

    /* Test passing a NULL DXDIAG_INIT_PARAMS pointer. */
    hr = IDxDiagProvider_Initialize(pddp, NULL);
    ok(hr == E_POINTER,
       "Expected IDxDiagProvider::Initialize to return E_POINTER, got %lx\n", hr);

    /* Test passing invalid dwSize values. */
    params.dwSize = 0;
    hr = IDxDiagProvider_Initialize(pddp, &params);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagProvider::Initialize to return E_INVALIDARG, got %lx\n", hr);

    params.dwSize = sizeof(params) + 1;
    hr = IDxDiagProvider_Initialize(pddp, &params);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagProvider::Initialize to return E_INVALIDARG, got %lx\n", hr);

    /* Test passing an unexpected dwDxDiagHeaderVersion value. */
    params.dwSize = sizeof(params);
    params.dwDxDiagHeaderVersion = 0;
    params.bAllowWHQLChecks = FALSE;
    params.pReserved = NULL;
    hr = IDxDiagProvider_Initialize(pddp, &params);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagProvider::Initialize to return E_INVALIDARG, got %lx\n", hr);

    /* Setting pReserved to a non-NULL value causes a crash on Windows. */
    if (0)
    {
        params.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
        params.bAllowWHQLChecks = FALSE;
        params.pReserved = (VOID*)0xdeadbeef;
        hr = IDxDiagProvider_Initialize(pddp, &params);
        trace("IDxDiagProvider::Initialize returned %lx\n", hr);
    }

    /* Test passing an appropriately initialized DXDIAG_INIT_PARAMS. */
    params.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
    params.bAllowWHQLChecks = FALSE;
    params.pReserved = NULL;
    hr = IDxDiagProvider_Initialize(pddp, &params);
    ok(hr == S_OK, "Expected IDxDiagProvider::Initialize to return S_OK, got %lx\n", hr);

    /* Test initializing multiple times. */
    hr = IDxDiagProvider_Initialize(pddp, &params);
    ok(hr == S_OK, "Expected IDxDiagProvider::Initialize to return S_OK, got %lx\n", hr);

    IDxDiagProvider_Release(pddp);
}

static void test_GetRootContainer(void)
{
    HRESULT hr;
    IDxDiagProvider *pddp;
    IDxDiagContainer *pddc, *pddc2;
    DXDIAG_INIT_PARAMS params;

    hr = CoCreateInstance(&CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDxDiagProvider, (LPVOID*)&pddp);
    ok(hr == S_OK ||
       broken(hr == REGDB_E_CLASSNOTREG), /* Clean W2K3 */
       "Creating a IDxDiagProvider instance failed with %lx\n", hr);
    if (FAILED(hr))
    {
        skip("Failed to create a IDxDiagProvider instance\n");
        return;
    }

    /* Test calling IDxDiagProvider::GetRootContainer before initialization. */
    hr = IDxDiagProvider_GetRootContainer(pddp, NULL);
    ok(hr == CO_E_NOTINITIALIZED,
       "Expected IDxDiagProvider::GetRootContainer to return CO_E_NOTINITIALIZED, got %lx\n", hr);

    hr = IDxDiagProvider_GetRootContainer(pddp, &pddc);
    ok(hr == CO_E_NOTINITIALIZED,
       "Expected IDxDiagProvider::GetRootContainer to return CO_E_NOTINITIALIZED, got %lx\n", hr);

    params.dwSize = sizeof(params);
    params.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
    params.bAllowWHQLChecks = FALSE;
    params.pReserved = NULL;
    hr = IDxDiagProvider_Initialize(pddp, &params);
    ok(hr == S_OK, "Expected IDxDiagProvider::Initialize to return S_OK, got %lx\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagProvider::Initialize failed\n");
        IDxDiagProvider_Release(pddp);
        return;
    }

    /* Passing NULL causes a crash on Windows. */
    if (0)
    {
        hr = IDxDiagProvider_GetRootContainer(pddp, NULL);
        trace("IDxDiagProvider::GetRootContainer returned %lx\n", hr);
    }

    hr = IDxDiagProvider_GetRootContainer(pddp, &pddc);
    ok(hr == S_OK, "Expected IDxDiagProvider::GetRootContainer to return S_OK, got %lx\n", hr);

    /* IDxDiagProvider::GetRootContainer creates new instances of the root
     * container rather than maintain a static root container. */
    hr = IDxDiagProvider_GetRootContainer(pddp, &pddc2);
    ok(hr == S_OK, "Expected IDxDiagProvider::GetRootContainer to return S_OK, got %lx\n", hr);
    ok(pddc != pddc2, "Expected the two pointers (%p vs. %p) to be unequal\n", pddc, pddc2);

    IDxDiagContainer_Release(pddc2);
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

START_TEST(provider)
{
    CoInitialize(NULL);
    test_Initialize();
    test_GetRootContainer();
    CoUninitialize();
}
