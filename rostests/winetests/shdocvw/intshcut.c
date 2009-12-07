/*
 * Unit tests to document InternetShortcut's behaviour
 *
 * Copyright 2008 Damjan Jovanovic
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"

#include "shlobj.h"
#include "shobjidl.h"
#include "shlguid.h"
#include "ole2.h"
#include "initguid.h"
#include "isguids.h"
#include "intshcut.h"

#include "wine/test.h"

static HRESULT WINAPI Unknown_QueryInterface(IUnknown *pUnknown, REFIID riid, void **ppvObject)
{
    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppvObject = pUnknown;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI Unknown_AddRef(IUnknown *pUnknown)
{
    return 2;
}

static ULONG WINAPI Unknown_Release(IUnknown *pUnknown)
{
    return 1;
}

static IUnknownVtbl unknownVtbl = {
    Unknown_QueryInterface,
    Unknown_AddRef,
    Unknown_Release
};

static IUnknown unknown = {
    &unknownVtbl
};

static const char *printGUID(const GUID *guid)
{
    static char guidSTR[39];

    if (!guid) return NULL;

    sprintf(guidSTR, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
     guid->Data1, guid->Data2, guid->Data3,
     guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
     guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return guidSTR;
}

static void test_Aggregability(void)
{
    HRESULT hr;
    IUnknown *pUnknown = NULL;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUnknown, (void**)&pUnknown);
    ok(SUCCEEDED(hr), "could not create instance of CLSID_InternetShortcut with IID_IUnknown, hr = 0x%x\n", hr);
    if (pUnknown)
        IUnknown_Release(pUnknown);

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&pUnknown);
    ok(SUCCEEDED(hr), "could not create instance of CLSID_InternetShortcut with IID_IUniformResourceLocatorA, hr = 0x%x\n", hr);
    if (pUnknown)
        IUnknown_Release(pUnknown);

    hr = CoCreateInstance(&CLSID_InternetShortcut, &unknown, CLSCTX_ALL, &IID_IUnknown, (void**)&pUnknown);
    ok(FAILED(hr), "aggregation didn't fail like it should, hr = 0x%x\n", hr);
    if (pUnknown)
        IUnknown_Release(pUnknown);
}

static void can_query_interface(IUnknown *pUnknown, REFIID riid)
{
    HRESULT hr;
    IUnknown *newInterface;
    hr = IUnknown_QueryInterface(pUnknown, riid, (void**)&newInterface);
    ok(SUCCEEDED(hr), "interface %s could not be queried\n", printGUID(riid));
    if (SUCCEEDED(hr))
        IUnknown_Release(newInterface);
}

static void test_QueryInterface(void)
{
    HRESULT hr;
    IUnknown *pUnknown;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUnknown, (void**)&pUnknown);
    if (SUCCEEDED(hr))
    {
        can_query_interface(pUnknown, &IID_IUniformResourceLocatorA);
        can_query_interface(pUnknown, &IID_IUniformResourceLocatorW);
        can_query_interface(pUnknown, &IID_IPersistFile);
        IUnknown_Release(pUnknown);
    }
    else
        skip("could not create a CLSID_InternetShortcut for QueryInterface tests, hr=0x%x\n", hr);
}

static CHAR *set_and_get_url(IUniformResourceLocatorA *urlA, LPCSTR input, DWORD flags)
{
    HRESULT hr;
    hr = urlA->lpVtbl->SetURL(urlA, input, flags);
    if (SUCCEEDED(hr))
    {
        CHAR *output;
        hr = urlA->lpVtbl->GetURL(urlA, &output);
        if (SUCCEEDED(hr))
            return output;
        else
            skip("GetUrl failed, hr=0x%x\n", hr);
    }
    else
        skip("SetUrl (%s, 0x%x) failed, hr=0x%x\n", input, flags, hr);
    return NULL;
}

static void check_string_transform(IUniformResourceLocatorA *urlA, LPCSTR input, DWORD flags, LPCSTR expectedOutput)
{
    CHAR *output = set_and_get_url(urlA, input, flags);
    if (output != NULL)
    {
        ok(lstrcmpA(output, expectedOutput) == 0, "unexpected URL change %s -> %s (expected %s)\n",
            input, output, expectedOutput);
        CoTaskMemFree(output);
    }
}

static void test_NullURLs(void)
{
    HRESULT hr;
    IUniformResourceLocatorA *urlA;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&urlA);
    if (SUCCEEDED(hr))
    {
        LPSTR url = NULL;

        hr = urlA->lpVtbl->GetURL(urlA, &url);
        ok(SUCCEEDED(hr), "getting uninitialized URL unexpectedly failed, hr=0x%x\n", hr);
        ok(url == NULL, "uninitialized URL is not NULL but %s\n", url);

        hr = urlA->lpVtbl->SetURL(urlA, NULL, 0);
        ok(SUCCEEDED(hr), "setting NULL URL unexpectedly failed, hr=0x%x\n", hr);

        hr = urlA->lpVtbl->GetURL(urlA, &url);
        ok(SUCCEEDED(hr), "getting NULL URL unexpectedly failed, hr=0x%x\n", hr);
        ok(url == NULL, "URL unexpectedly not NULL but %s\n", url);

        urlA->lpVtbl->Release(urlA);
    }
    else
        skip("could not create a CLSID_InternetShortcut for NullURL tests, hr=0x%x\n", hr);
}

static void test_SetURLFlags(void)
{
    HRESULT hr;
    IUniformResourceLocatorA *urlA;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&urlA);
    if (SUCCEEDED(hr))
    {
        check_string_transform(urlA, "somerandomstring", 0, "somerandomstring");
        check_string_transform(urlA, "www.winehq.org", 0, "www.winehq.org");

        todo_wine
        {
            check_string_transform(urlA, "www.winehq.org", IURL_SETURL_FL_GUESS_PROTOCOL, "http://www.winehq.org/");
            check_string_transform(urlA, "ftp.winehq.org", IURL_SETURL_FL_GUESS_PROTOCOL, "ftp://ftp.winehq.org/");
        }

        urlA->lpVtbl->Release(urlA);
    }
    else
        skip("could not create a CLSID_InternetShortcut for SetUrl tests, hr=0x%x\n", hr);
}

static void test_InternetShortcut(void)
{
    test_Aggregability();
    test_QueryInterface();
    test_NullURLs();
    test_SetURLFlags();
}

START_TEST(intshcut)
{
    OleInitialize(NULL);
    test_InternetShortcut();
    OleUninitialize();
}
