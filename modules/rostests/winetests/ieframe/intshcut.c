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
#include "mshtml.h"
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

static void test_Aggregability(void)
{
    HRESULT hr;
    IUnknown *pUnknown = NULL;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUnknown, (void**)&pUnknown);
    ok(hr == S_OK, "could not create instance of CLSID_InternetShortcut with IID_IUnknown, hr = 0x%lx\n", hr);
    if (pUnknown)
        IUnknown_Release(pUnknown);

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&pUnknown);
    ok(hr == S_OK, "could not create instance of CLSID_InternetShortcut with IID_IUniformResourceLocatorA, hr = 0x%lx\n", hr);
    if (pUnknown)
        IUnknown_Release(pUnknown);

    hr = CoCreateInstance(&CLSID_InternetShortcut, &unknown, CLSCTX_ALL, &IID_IUnknown, (void**)&pUnknown);
    ok(hr == CLASS_E_NOAGGREGATION, "aggregation didn't fail like it should, hr = 0x%lx\n", hr);
    if (pUnknown)
        IUnknown_Release(pUnknown);
}

static void can_query_interface(IUnknown *pUnknown, REFIID riid)
{
    HRESULT hr;
    IUnknown *newInterface;
    hr = IUnknown_QueryInterface(pUnknown, riid, (void**)&newInterface);
    ok(hr == S_OK, "interface %s could not be queried\n", wine_dbgstr_guid(riid));
    if (SUCCEEDED(hr))
        IUnknown_Release(newInterface);
}

static void test_QueryInterface(void)
{
    HRESULT hr;
    IUnknown *pUnknown;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUnknown, (void**)&pUnknown);
    ok(hr == S_OK, "Could not create InternetShortcut object: %08lx\n", hr);

    can_query_interface(pUnknown, &IID_IUniformResourceLocatorA);
    can_query_interface(pUnknown, &IID_IUniformResourceLocatorW);
    can_query_interface(pUnknown, &IID_IPersistFile);
    IUnknown_Release(pUnknown);
}

#define test_shortcut_url(a,b) _test_shortcut_url(__LINE__,a,b)
static void _test_shortcut_url(unsigned line, IUnknown *unk, const char *exurl)
{
    IUniformResourceLocatorA *locator_a;
    char *url_a;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IUniformResourceLocatorA, (void**)&locator_a);
    ok_(__FILE__,line)(hres == S_OK, "Could not get IUniformResourceLocatorA iface: %08lx\n", hres);

    hres = locator_a->lpVtbl->GetURL(locator_a, &url_a);
    ok_(__FILE__,line)(hres == S_OK, "GetURL failed: %08lx\n", hres);
    ok_(__FILE__,line)(!strcmp(url_a, exurl), "unexpected URL, got %s, expected %s\n", url_a, exurl);
    CoTaskMemFree(url_a);

    IUnknown_Release((IUnknown*)locator_a);
}

#define check_string_transform(a,b,c,d,e) _check_string_transform(__LINE__,a,b,c,d,e)
static void _check_string_transform(unsigned line, IUniformResourceLocatorA *urlA, LPCSTR input, DWORD flags,
        LPCSTR expectedOutput, BOOL is_todo)
{
    CHAR *output;
    HRESULT hr;

    hr = urlA->lpVtbl->SetURL(urlA, input, flags);
    ok_(__FILE__,line)(hr == S_OK, "SetUrl failed, hr=0x%lx\n", hr);
    if (FAILED(hr))
        return;

    output = (void*)0xdeadbeef;
    hr = urlA->lpVtbl->GetURL(urlA, &output);
    if(expectedOutput) {
        todo_wine_if(is_todo) {
            ok_(__FILE__,line)(hr == S_OK, "GetUrl failed, hr=0x%lx\n", hr);
        }
        todo_wine
        ok_(__FILE__,line)(!lstrcmpA(output, expectedOutput), "unexpected URL change %s -> %s (expected %s)\n",
            input, output, expectedOutput);
    }else {
        todo_wine
        ok_(__FILE__,line)(hr == S_FALSE, "GetUrl failed, hr=0x%lx\n", hr);
        todo_wine
        ok_(__FILE__,line)(!output, "GetUrl returned %s\n", output);
    }
    if (hr == S_OK)
        CoTaskMemFree(output);
}

static void test_ReadAndWriteProperties(void)
{
    int iconIndex = 7;
    PROPSPEC ps[2];
    HRESULT hr;
    IUniformResourceLocatorA *urlA;
    IUniformResourceLocatorA *urlAFromFile;
    WCHAR fileNameW[MAX_PATH];
    static WCHAR iconPath[] = L"file:///C:/arbitrary/icon/path";
    char testurl[] = "http://some/bogus/url.html";
    PROPVARIANT pv[2], pvread[2];

    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = PID_IS_ICONFILE;
    ps[1].ulKind = PRSPEC_PROPID;
    ps[1].propid = PID_IS_ICONINDEX;

    pv[0].vt = VT_LPWSTR;
    pv[0].pwszVal = iconPath;
    pv[1].vt = VT_I4;
    pv[1].lVal = iconIndex;

    /* Make sure we have a valid temporary directory */
    GetTempPathW(MAX_PATH, fileNameW);
    lstrcatW(fileNameW, L"testshortcut.url");

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&urlA);
    ok(hr == S_OK, "Could not create CLSID_InternetShortcut instance: %08lx\n", hr);
    if (hr == S_OK)
    {
        IPersistFile *pf;
        IPropertyStorage *pPropStgWrite;
        IPropertySetStorage *pPropSetStg;

        /* We need to set a URL -- IPersistFile refuses to save without one. */
        hr = urlA->lpVtbl->SetURL(urlA, testurl, 0);
        ok(hr == S_OK, "Failed to set a URL.  hr=0x%lx\n", hr);

        /* Write this shortcut out to a file so that we can test reading it in again. */
        hr = urlA->lpVtbl->QueryInterface(urlA, &IID_IPersistFile, (void **) &pf);
        ok(hr == S_OK, "Failed to get the IPersistFile for writing.  hr=0x%lx\n", hr);

        hr = IPersistFile_Save(pf, fileNameW, TRUE);
        ok(hr == S_OK, "Failed to save via IPersistFile. hr=0x%lx\n", hr);

        IPersistFile_Release(pf);

        hr = urlA->lpVtbl->QueryInterface(urlA, &IID_IPropertySetStorage, (void **) &pPropSetStg);
        ok(hr == S_OK, "Unable to get an IPropertySetStorage, hr=0x%lx\n", hr);

        hr = IPropertySetStorage_Open(pPropSetStg, &FMTID_Intshcut, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, &pPropStgWrite);
        ok(hr == S_OK, "Unable to get an IPropertyStorage for writing, hr=0x%lx\n", hr);

        hr = IPropertyStorage_WriteMultiple(pPropStgWrite, 2, ps, pv, 0);
        ok(hr == S_OK, "Unable to set properties, hr=0x%lx\n", hr);

        memset(pvread, 0, sizeof(pvread));
        hr = IPropertyStorage_ReadMultiple(pPropStgWrite, 2, ps, pvread);
        ok(hr == S_OK, "Unable to read properties, hr=0x%lx\n", hr);
        ok(pvread[1].vt == VT_I4, "got %d\n", pvread[1].vt);
        ok(pvread[1].lVal == iconIndex, "Read wrong icon index: %d\n", pvread[1].iVal);
        ok(pvread[0].vt == VT_LPWSTR, "got %d\n", pvread[0].vt);
        ok(lstrcmpW(pvread[0].pwszVal, iconPath) == 0, "Wrong icon path read: %s\n", wine_dbgstr_w(pvread[0].pwszVal));
        PropVariantClear(&pvread[0]);
        PropVariantClear(&pvread[1]);

        hr = IPropertyStorage_Commit(pPropStgWrite, STGC_DEFAULT);
        ok(hr == S_OK, "Failed to commit properties, hr=0x%lx\n", hr);

        pPropStgWrite->lpVtbl->Release(pPropStgWrite);

        /* Test with STGM_WRITE */
        hr = IPropertySetStorage_Open(pPropSetStg, &FMTID_Intshcut, STGM_WRITE, &pPropStgWrite);
        ok(hr == S_OK, "Unable to get an IPropertyStorage for writing, hr=0x%lx\n", hr);

        if (hr == S_OK)
        {
            memset(pvread, 0, sizeof(pvread));
            hr = IPropertyStorage_ReadMultiple(pPropStgWrite, 2, ps, pvread);
            ok(hr == S_OK, "Unable to read properties, hr=0x%lx\n", hr);
            ok(pvread[1].vt == VT_I4, "got %d\n", pvread[1].vt);
            ok(pvread[1].lVal == iconIndex, "Read wrong icon index: %d\n", pvread[1].iVal);
            ok(pvread[0].vt == VT_LPWSTR, "got %d\n", pvread[0].vt);
            ok(lstrcmpW(pvread[0].pwszVal, iconPath) == 0, "Wrong icon path read: %s\n", wine_dbgstr_w(pvread[0].pwszVal));
            PropVariantClear(&pvread[0]);
            PropVariantClear(&pvread[1]);

            IPropertyStorage_Release(pPropStgWrite);
        }
        urlA->lpVtbl->Release(urlA);
        IPropertySetStorage_Release(pPropSetStg);
    }

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&urlAFromFile);
    ok(hr == S_OK, "Could not create CLSID_InternetShortcut instance: %08lx\n", hr);
    if (hr == S_OK)
    {
        IPropertySetStorage *pPropSetStg;
        IPropertyStorage *pPropStgRead;
        PROPVARIANT pvread[2];
        IPersistFile *pf;
        LPSTR url = NULL;

        /* Now read that .url file back in. */
        hr = urlAFromFile->lpVtbl->QueryInterface(urlAFromFile, &IID_IPersistFile, (void **) &pf);
        ok(hr == S_OK, "Failed to get the IPersistFile for reading.  hr=0x%lx\n", hr);

        hr = IPersistFile_Load(pf, fileNameW, 0);
        ok(hr == S_OK, "Failed to load via IPersistFile. hr=0x%lx\n", hr);
        IPersistFile_Release(pf);


        hr = urlAFromFile->lpVtbl->GetURL(urlAFromFile, &url);
        ok(hr == S_OK, "Unable to get url from file, hr=0x%lx\n", hr);
        ok(lstrcmpA(url, testurl) == 0, "Wrong url read from file: %s\n",url);
        CoTaskMemFree(url);

        hr = urlAFromFile->lpVtbl->QueryInterface(urlAFromFile, &IID_IPropertySetStorage, (void **) &pPropSetStg);
        ok(hr == S_OK, "Unable to get an IPropertySetStorage, hr=0x%lx\n", hr);

        hr = IPropertySetStorage_Open(pPropSetStg, &FMTID_Intshcut, STGM_READ | STGM_SHARE_EXCLUSIVE, &pPropStgRead);
        ok(hr == S_OK, "Unable to get an IPropertyStorage for reading, hr=0x%lx\n", hr);

        memset(pvread, 0, sizeof(pvread));
        hr = IPropertyStorage_ReadMultiple(pPropStgRead, 2, ps, pvread);
    todo_wine /* Wine doesn't yet support setting properties after save */
    {
        ok(hr == S_OK, "Unable to read properties, hr=0x%lx\n", hr);
        ok(pvread[1].vt == VT_I4, "got %d\n", pvread[1].vt);
        ok(pvread[1].lVal == iconIndex, "Read wrong icon index: %d\n", pvread[1].iVal);
        ok(pvread[0].vt == VT_LPWSTR, "got %d\n", pvread[0].vt);
        ok(lstrcmpW(pvread[0].pwszVal, iconPath) == 0, "Wrong icon path read: %s\n", wine_dbgstr_w(pvread[0].pwszVal));
    }
        PropVariantClear(&pvread[0]);
        PropVariantClear(&pvread[1]);

        hr = IPropertyStorage_WriteMultiple(pPropStgRead, 2, ps, pv, 0);
        ok(hr == STG_E_ACCESSDENIED, "setting properties should return STG_E_ACCESSDENIED instead of 0x%lx\n", hr);

        IPropertyStorage_Release(pPropStgRead);
        IPropertySetStorage_Release(pPropSetStg);
        urlAFromFile->lpVtbl->Release(urlAFromFile);
        DeleteFileW(fileNameW);
    }
}

static void test_NullURLs(void)
{
    LPSTR url = NULL;
    HRESULT hr;
    IUniformResourceLocatorA *urlA;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&urlA);
    ok(hr == S_OK, "Could not create InternetShortcut object: %08lx\n", hr);

    hr = urlA->lpVtbl->GetURL(urlA, &url);
    ok(hr == S_FALSE, "getting uninitialized URL unexpectedly failed, hr=0x%lx\n", hr);
    ok(url == NULL, "uninitialized URL is not NULL but %s\n", url);

    hr = urlA->lpVtbl->SetURL(urlA, NULL, 0);
    ok(hr == S_OK, "setting NULL URL unexpectedly failed, hr=0x%lx\n", hr);

    hr = urlA->lpVtbl->GetURL(urlA, &url);
    ok(hr == S_FALSE, "getting NULL URL unexpectedly failed, hr=0x%lx\n", hr);
    ok(url == NULL, "URL unexpectedly not NULL but %s\n", url);

    urlA->lpVtbl->Release(urlA);
}

typedef struct {
    const char *data;
    const char *url;
} load_test_t;

static const load_test_t load_tests[] = {
    {"[InternetShortcut]\n"
     "URL=http://www.winehq.org/\n"
     "HotKey=0\n"
     "IDList=\n"
     "[{000214A0-0000-0000-C000-000000000046}]\n"
     "Prop0=1,2\n",

     "http://www.winehq.org/"
    }
};

static void test_Load(void)
{
    IPersistFile *persist_file;
    const load_test_t *test;
    WCHAR file_path[MAX_PATH];
    DWORD size;
    HANDLE file;
    HRESULT hres;

    GetTempPathW(MAX_PATH, file_path);
    lstrcatW(file_path, L"test.url");

    for(test = load_tests; test < load_tests + ARRAY_SIZE(load_tests); test++) {
        IPropertySetStorage *propsetstorage;
        IPropertyStorage *propstorage;
        PROPVARIANT v;
        PROPSPEC ps;

        file = CreateFileW(file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        ok(file != INVALID_HANDLE_VALUE, "could not create test file\n");
        if(file == INVALID_HANDLE_VALUE)
            continue;

        WriteFile(file, test->data, strlen(test->data), &size, NULL);
        CloseHandle(file);

        hres = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IPersistFile, (void**)&persist_file);
        ok(hres == S_OK, "Could not create InternetShortcut instance: %08lx\n", hres);

        hres = IPersistFile_Load(persist_file, file_path, 0);
        ok(hres == S_OK, "Load failed: %08lx\n", hres);

        test_shortcut_url((IUnknown*)persist_file, test->url);

        hres = IPersistFile_QueryInterface(persist_file, &IID_IPropertySetStorage, (void **)&propsetstorage);
        ok(hres == S_OK, "Unable to get an IPropertySetStorage, hr=0x%lx\n", hres);

        hres = IPropertySetStorage_Open(propsetstorage, &FMTID_Intshcut, STGM_READ | STGM_SHARE_EXCLUSIVE, &propstorage);
        ok(hres == S_OK, "Unable to get an IPropertyStorage for reading, hr=0x%lx\n", hres);

        ps.ulKind = PRSPEC_PROPID;
        ps.propid = PID_IS_ICONFILE;
        v.vt = VT_NULL;
        hres = IPropertyStorage_ReadMultiple(propstorage, 1, &ps, &v);
        ok(hres == S_FALSE, "got 0x%08lx\n", hres);
        ok(v.vt == VT_EMPTY, "got %d\n", v.vt);

        ps.ulKind = PRSPEC_PROPID;
        ps.propid = PID_IS_ICONINDEX;
        v.vt = VT_EMPTY;
        hres = IPropertyStorage_ReadMultiple(propstorage, 1, &ps, &v);
        ok(hres == S_FALSE, "got 0x%08lx\n", hres);
        ok(v.vt == VT_EMPTY, "got %d\n", v.vt);

        IPropertyStorage_Release(propstorage);
        IPropertySetStorage_Release(propsetstorage);

        IPersistFile_Release(persist_file);
        DeleteFileW(file_path);
    }
}

static void test_SetURLFlags(void)
{
    HRESULT hr;
    IUniformResourceLocatorA *urlA;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&urlA);
    ok(hr == S_OK, "Could not create InternetShortcut object: %08lx\n", hr);

    check_string_transform(urlA, "somerandomstring", 0, NULL, TRUE);
    check_string_transform(urlA, "www.winehq.org", 0, NULL, TRUE);

    check_string_transform(urlA, "www.winehq.org", IURL_SETURL_FL_GUESS_PROTOCOL, "http://www.winehq.org/", FALSE);
    check_string_transform(urlA, "ftp.winehq.org", IURL_SETURL_FL_GUESS_PROTOCOL, "ftp://ftp.winehq.org/", FALSE);

    urlA->lpVtbl->Release(urlA);
}

static void test_InternetShortcut(void)
{
    IUniformResourceLocatorA *url;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&url);
    ok(hres == S_OK, "Could not create CLSID_InternetShortcut instance: %08lx\n", hres);
    if(FAILED(hres))
        return;
    url->lpVtbl->Release(url);

    test_Aggregability();
    test_QueryInterface();
    test_NullURLs();
    test_SetURLFlags();
    test_ReadAndWriteProperties();
    test_Load();
}

START_TEST(intshcut)
{
    OleInitialize(NULL);

    test_InternetShortcut();

    OleUninitialize();
}
