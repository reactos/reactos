/*
 *    General still image implementation
 *
 * Copyright 2009 Damjan Jovanovic
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#include <initguid.h>
#include <sti.h>
#include <guiddef.h>
#include <devguid.h>
#include <stdio.h>

#include "wine/test.h"

static HMODULE sti_dll;
static HRESULT (WINAPI *pStiCreateInstance)(HINSTANCE,DWORD,PSTIW*,LPUNKNOWN);
static HRESULT (WINAPI *pStiCreateInstanceA)(HINSTANCE,DWORD,PSTIA*,LPUNKNOWN);
static HRESULT (WINAPI *pStiCreateInstanceW)(HINSTANCE,DWORD,PSTIW*,LPUNKNOWN);

static BOOL aggregator_addref_called;

static HRESULT WINAPI aggregator_QueryInterface(IUnknown *iface, REFIID riid, void **ppvObject)
{
    return E_NOTIMPL;
}

static ULONG WINAPI aggregator_AddRef(IUnknown *iface)
{
    aggregator_addref_called = TRUE;
    return 2;
}

static ULONG WINAPI aggregator_Release(IUnknown *iface)
{
    return 1;
}

static struct IUnknownVtbl aggregator_vtbl =
{
    aggregator_QueryInterface,
    aggregator_AddRef,
    aggregator_Release
};

static BOOL init_function_pointers(void)
{
    sti_dll = LoadLibraryA("sti.dll");
    if (sti_dll)
    {
        pStiCreateInstance = (void*)
            GetProcAddress(sti_dll, "StiCreateInstance");
        pStiCreateInstanceA = (void*)
            GetProcAddress(sti_dll, "StiCreateInstanceA");
        pStiCreateInstanceW = (void*)
            GetProcAddress(sti_dll, "StiCreateInstanceW");
        return TRUE;
    }
    return FALSE;
}

static void test_version_flag_versus_aw(void)
{
    HRESULT hr;

    /* Who wins, the STI_VERSION_FLAG_UNICODE or the A/W function? And what about the neutral StiCreateInstance function? */

    if (pStiCreateInstance)
    {
        PSTIW pStiW;
        hr = pStiCreateInstance(GetModuleHandleA(NULL), STI_VERSION_REAL, &pStiW, NULL);
        if (SUCCEEDED(hr))
        {
            IUnknown *pUnknown;
            hr = IUnknown_QueryInterface((IUnknown*)pStiW, &IID_IStillImageW, (void**)&pUnknown);
            if (SUCCEEDED(hr))
            {
                ok(pUnknown == (IUnknown*)pStiW, "created interface was not IID_IStillImageW\n");
                IUnknown_Release(pUnknown);
            }
            IUnknown_Release((IUnknown*)pStiW);
        }
        else
            ok(0, "could not create StillImageA, hr = 0x%lX\n", hr);
        hr = pStiCreateInstance(GetModuleHandleA(NULL), STI_VERSION_REAL | STI_VERSION_FLAG_UNICODE, &pStiW, NULL);
        if (SUCCEEDED(hr))
        {
            IUnknown *pUnknown;
            hr = IUnknown_QueryInterface((IUnknown*)pStiW, &IID_IStillImageW, (void**)&pUnknown);
            if (SUCCEEDED(hr))
            {
                ok(pUnknown == (IUnknown*)pStiW, "created interface was not IID_IStillImageW\n");
                IUnknown_Release(pUnknown);
            }
            IUnknown_Release((IUnknown*)pStiW);
        }
        else
            ok(0, "could not create StillImageW, hr = 0x%lX\n", hr);
    }
    else
        win_skip("No StiCreateInstance function\n");

    if (pStiCreateInstanceA)
    {
        PSTIA pStiA;
        hr = pStiCreateInstanceA(GetModuleHandleA(NULL), STI_VERSION_REAL | STI_VERSION_FLAG_UNICODE, &pStiA, NULL);
        if (SUCCEEDED(hr))
        {
            IUnknown *pUnknown;
            hr = IUnknown_QueryInterface((IUnknown*)pStiA, &IID_IStillImageA, (void**)&pUnknown);
            if (SUCCEEDED(hr))
            {
                ok(pUnknown == (IUnknown*)pStiA, "created interface was not IID_IStillImageA\n");
                IUnknown_Release(pUnknown);
            }
            IUnknown_Release((IUnknown*)pStiA);
        }
        else
            todo_wine ok(0, "could not create StillImageA, hr = 0x%lX\n", hr);
    }
    else
        win_skip("No StiCreateInstanceA function\n");

    if (pStiCreateInstanceW)
    {
        PSTIW pStiW;
        hr = pStiCreateInstanceW(GetModuleHandleA(NULL), STI_VERSION_REAL, &pStiW, NULL);
        if (SUCCEEDED(hr))
        {
            IUnknown *pUnknown;
            hr = IUnknown_QueryInterface((IUnknown*)pStiW, &IID_IStillImageW, (void**)&pUnknown);
            if (SUCCEEDED(hr))
            {
                ok(pUnknown == (IUnknown*)pStiW, "created interface was not IID_IStillImageW\n");
                IUnknown_Release(pUnknown);
            }
            IUnknown_Release((IUnknown*)pStiW);
        }
        else
            ok(0, "could not create StillImageW, hr = 0x%lX\n", hr);
    }
    else
        win_skip("No StiCreateInstanceW function\n");
}

static void test_stillimage_aggregation(void)
{
    if (pStiCreateInstanceW)
    {
        IUnknown aggregator = { &aggregator_vtbl };
        IStillImageW *pStiW;
        IUnknown *pUnknown;
        HRESULT hr;

        /* When aggregating, the outer object must get the non-delegating IUnknown to be
           able to control the inner object's reference count and query its interfaces.
           But StiCreateInstance* only take PSTI. So how does the non-delegating IUnknown
           come back to the outer object calling this function? */

        hr = pStiCreateInstanceW(GetModuleHandleA(NULL), STI_VERSION_REAL, &pStiW, &aggregator);
        if (SUCCEEDED(hr))
        {
            IStillImageW *pStiW2 = NULL;

            /* Does this interface delegate? */
            aggregator_addref_called = FALSE;
            IStillImage_AddRef(pStiW);
            ok(!aggregator_addref_called, "the aggregated IStillImageW shouldn't delegate\n");
            IStillImage_Release(pStiW);

            /* Tests show calling IStillImageW_WriteToErrorLog on the interface segfaults on Windows, so I guess it's an IUnknown.
               But querying for an IUnknown returns a different interface, which also delegates.
               So much for COM being reflexive...
               Anyway I doubt apps depend on any of this. */

            /* And what about the IStillImageW interface? */
            hr = IStillImage_QueryInterface(pStiW, &IID_IStillImageW, (void**)&pStiW2);
            if (SUCCEEDED(hr))
            {
                ok(pStiW != pStiW2, "the aggregated IStillImageW and its queried IStillImageW unexpectedly match\n");
                /* Does it delegate? */
                aggregator_addref_called = FALSE;
                IStillImage_AddRef(pStiW2);
                ok(aggregator_addref_called, "the created IStillImageW's IStillImageW should delegate\n");
                IStillImage_Release(pStiW2);
                IStillImage_Release(pStiW2);
            }
            else
                ok(0, "could not query for IID_IStillImageW, hr = 0x%lx\n", hr);

            IStillImage_Release(pStiW);
        }
        else
            ok(0, "could not create StillImageW, hr = 0x%lX\n", hr);

        /* Now do the above tests prove that STI.DLL isn't picky about querying for IUnknown
           in CoCreateInterface when aggregating? */
        hr = CoCreateInstance(&CLSID_Sti, &aggregator, CLSCTX_ALL, &IID_IStillImageW, (void**)&pStiW);
        ok(FAILED(hr), "CoCreateInstance unexpectedly succeeded when querying for IStillImageW during aggregation\n");
        if (SUCCEEDED(hr))
            IStillImage_Release(pStiW);
        hr = CoCreateInstance(&CLSID_Sti, &aggregator, CLSCTX_ALL, &IID_IUnknown, (void**)&pUnknown);
        ok(SUCCEEDED(hr) ||
            broken(hr == CLASS_E_NOAGGREGATION), /* Win 2000 */
                "CoCreateInstance unexpectedly failed when querying for IUnknown during aggregation, hr = 0x%lx\n", hr);
        if (SUCCEEDED(hr))
            IUnknown_Release(pUnknown);
    }
    else
        win_skip("No StiCreateInstanceW function\n");
}

static void test_launch_app_registry(void)
{
    static WCHAR appName[] = L"winestitestapp";
    IStillImageW *pStiW = NULL;
    HRESULT hr;

    if (pStiCreateInstanceW == NULL)
    {
        win_skip("No StiCreateInstanceW function\n");
        return;
    }

    hr = pStiCreateInstance(GetModuleHandleA(NULL), STI_VERSION_REAL | STI_VERSION_FLAG_UNICODE, &pStiW, NULL);
    if (SUCCEEDED(hr))
    {
        hr = IStillImage_RegisterLaunchApplication(pStiW, appName, appName);
        if (hr == E_ACCESSDENIED)
            skip("Not authorized to register a launch application\n");
        else if (SUCCEEDED(hr))
        {
            hr = IStillImage_UnregisterLaunchApplication(pStiW, appName);
            ok(SUCCEEDED(hr), "could not unregister launch application, error 0x%lX\n", hr);
        }
        else
            ok(0, "could not register launch application, error 0x%lX\n", hr);
        IStillImage_Release(pStiW);
    }
    else
        ok(0, "could not create StillImageW, hr = 0x%lX\n", hr);
}

START_TEST(sti)
{
    if (SUCCEEDED(CoInitialize(NULL)))
    {
        if (init_function_pointers())
        {
            test_version_flag_versus_aw();
            test_stillimage_aggregation();
            test_launch_app_registry();
            FreeLibrary(sti_dll);
        }
        else
            win_skip("could not load sti.dll\n");
        CoUninitialize();
    }
    else
        skip("CoInitialize failed\n");
}
