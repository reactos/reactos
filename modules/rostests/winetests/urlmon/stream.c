/*
 * Copyright 2007 Robert Shearman for CodeWeavers
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

#include <wine/test.h>
#include <stdarg.h>
#include <stddef.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "urlmon.h"
#include "wininet.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        expect_ ## func = FALSE; \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_NOT_CALLED(func) \
    do { \
        ok(!called_ ## func, "unexpected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(QueryInterface_IServiceProvider);
DEFINE_EXPECT(OnStartBinding);
DEFINE_EXPECT(OnProgress_FINDINGRESOURCE);
DEFINE_EXPECT(OnProgress_CONNECTING);
DEFINE_EXPECT(OnProgress_SENDINGREQUEST);
DEFINE_EXPECT(OnProgress_MIMETYPEAVAILABLE);
DEFINE_EXPECT(OnProgress_BEGINDOWNLOADDATA);
DEFINE_EXPECT(OnProgress_DOWNLOADINGDATA);
DEFINE_EXPECT(OnProgress_ENDDOWNLOADDATA);
DEFINE_EXPECT(OnStopBinding);
DEFINE_EXPECT(OnDataAvailable);
DEFINE_EXPECT(GetBindInfo);

static const CHAR wszIndexHtmlA[] = "index.html";
static WCHAR INDEX_HTML[MAX_PATH];
static const char szHtmlDoc[] = "<HTML></HTML>";

static HRESULT WINAPI statusclb_QueryInterface(IBindStatusCallback *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(&IID_IBindStatusCallback, riid) ||
        IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppv = iface;
        return S_OK;
    }
    else if (IsEqualGUID(&IID_IServiceProvider, riid))
    {
        CHECK_EXPECT(QueryInterface_IServiceProvider);
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI statusclb_AddRef(IBindStatusCallback *iface)
{
    return 2;
}

static ULONG WINAPI statusclb_Release(IBindStatusCallback *iface)
{
    return 1;
}

static HRESULT WINAPI statusclb_OnStartBinding(IBindStatusCallback *iface, DWORD dwReserved,
                                               IBinding *pib)
{
    HRESULT hres;
    IMoniker *mon;

    CHECK_EXPECT(OnStartBinding);

    ok(pib != NULL, "pib should not be NULL\n");

    hres = IBinding_QueryInterface(pib, &IID_IMoniker, (void**)&mon);
    ok(hres == E_NOINTERFACE, "IBinding should not have IMoniker interface\n");
    if(SUCCEEDED(hres))
        IMoniker_Release(mon);

    return S_OK;
}

static HRESULT WINAPI statusclb_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
                                           ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    switch(ulStatusCode) {
        case BINDSTATUS_FINDINGRESOURCE:
            CHECK_EXPECT(OnProgress_FINDINGRESOURCE);
            break;
        case BINDSTATUS_CONNECTING:
            CHECK_EXPECT(OnProgress_CONNECTING);
            break;
        case BINDSTATUS_SENDINGREQUEST:
            CHECK_EXPECT(OnProgress_SENDINGREQUEST);
            break;
        case BINDSTATUS_MIMETYPEAVAILABLE:
            CHECK_EXPECT(OnProgress_MIMETYPEAVAILABLE);
            break;
        case BINDSTATUS_BEGINDOWNLOADDATA:
            CHECK_EXPECT(OnProgress_BEGINDOWNLOADDATA);
            ok(szStatusText != NULL, "szStatusText == NULL\n");
            break;
        case BINDSTATUS_DOWNLOADINGDATA:
            CHECK_EXPECT2(OnProgress_DOWNLOADINGDATA);
            break;
        case BINDSTATUS_ENDDOWNLOADDATA:
            CHECK_EXPECT(OnProgress_ENDDOWNLOADDATA);
            ok(szStatusText != NULL, "szStatusText == NULL\n");
            break;
        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            ok(szStatusText != NULL, "szStatusText == NULL\n");
            break;
        default:
            todo_wine { ok(0, "unexpected code %ld\n", ulStatusCode); }
    };
    return S_OK;
}

static HRESULT WINAPI statusclb_OnStopBinding(IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    CHECK_EXPECT(OnStopBinding);

    /* ignore DNS failure */
    if (hresult != HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED))
    {
        ok(SUCCEEDED(hresult), "Download failed: %08lx\n", hresult);
        ok(szError == NULL, "szError should be NULL\n");
    }

    return S_OK;
}

static HRESULT WINAPI statusclb_GetBindInfo(IBindStatusCallback *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    DWORD cbSize;

    CHECK_EXPECT(GetBindInfo);

    *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    cbSize = pbindinfo->cbSize;
    memset(pbindinfo, 0, cbSize);
    pbindinfo->cbSize = cbSize;

    return S_OK;
}

static HRESULT WINAPI statusclb_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
                                                DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    HRESULT hres;
    DWORD read;
    BYTE buf[512];

    CHECK_EXPECT2(OnDataAvailable);

    if (0)
    {
        /* FIXME: Uncomment after removing BindToStorage hack. */
        ok(pformatetc != NULL, "pformatetc == NULL\n");
        if(pformatetc) {
            ok(pformatetc->cfFormat == 0xc02d, "clipformat=%x\n", pformatetc->cfFormat);
            ok(pformatetc->ptd == NULL, "ptd = %p\n", pformatetc->ptd);
            ok(pformatetc->dwAspect == 1, "dwAspect=%lu\n", pformatetc->dwAspect);
            ok(pformatetc->lindex == -1, "lindex=%ld\n", pformatetc->lindex);
            ok(pformatetc->tymed == TYMED_ISTREAM, "tymed=%lu\n", pformatetc->tymed);
        }

        ok(pstgmed != NULL, "stgmeg == NULL\n");
        if(pstgmed) {
            ok(pstgmed->tymed == TYMED_ISTREAM, "tymed=%lu\n", pstgmed->tymed);
            ok(pstgmed->pstm != NULL, "pstm == NULL\n");
            ok(pstgmed->pUnkForRelease != NULL, "pUnkForRelease == NULL\n");
        }
    }

    if(pstgmed->pstm) {
        do hres = IStream_Read(pstgmed->pstm, buf, 512, &read);
        while(hres == S_OK);
        ok(hres == S_FALSE || hres == E_PENDING, "IStream_Read returned %08lx\n", hres);
    }

    return S_OK;
}

static HRESULT WINAPI statusclb_OnObjectAvailable(IBindStatusCallback *iface, REFIID riid, IUnknown *punk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IBindStatusCallbackVtbl BindStatusCallbackVtbl = {
    statusclb_QueryInterface,
    statusclb_AddRef,
    statusclb_Release,
    statusclb_OnStartBinding,
    statusclb_GetPriority,
    statusclb_OnLowResource,
    statusclb_OnProgress,
    statusclb_OnStopBinding,
    statusclb_GetBindInfo,
    statusclb_OnDataAvailable,
    statusclb_OnObjectAvailable
};

static IBindStatusCallback BindStatusCallback = { &BindStatusCallbackVtbl };

static void set_file_url(char *path)
{
    char INDEX_HTML_A[MAX_PATH];

    lstrcpyA(INDEX_HTML_A, "file:///");
    lstrcatA(INDEX_HTML_A, path);
    MultiByteToWideChar(CP_ACP, 0, INDEX_HTML_A, -1, INDEX_HTML, MAX_PATH);
}

static void create_file(void)
{
    HANDLE file;
    DWORD size;
    CHAR path[MAX_PATH];

    file = CreateFileA(wszIndexHtmlA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return;

    WriteFile(file, szHtmlDoc, sizeof(szHtmlDoc)-1, &size, NULL);
    CloseHandle(file);

    GetCurrentDirectoryA(MAX_PATH, path);
    lstrcatA(path, "\\");
    lstrcatA(path, wszIndexHtmlA);
    set_file_url(path);
}

static void test_URLOpenBlockingStreamW(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    char buffer[256];

    hr = URLOpenBlockingStreamW(NULL, NULL, &pStream, 0, &BindStatusCallback);
    ok(hr == E_INVALIDARG, "URLOpenBlockingStreamW should have failed with E_INVALIDARG instead of 0x%08lx\n", hr);
    if (0)  /* crashes on Win2k */
    {
        hr = URLOpenBlockingStreamW(NULL, INDEX_HTML, NULL, 0, &BindStatusCallback);
        ok(hr == E_INVALIDARG, "URLOpenBlockingStreamW should have failed with E_INVALIDARG instead of 0x%08lx\n", hr);
    }

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(QueryInterface_IServiceProvider);
    SET_EXPECT(OnStartBinding);
    SET_EXPECT(OnProgress_SENDINGREQUEST);
    SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
    SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
    SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
    SET_EXPECT(OnStopBinding);

    hr = URLOpenBlockingStreamW(NULL, INDEX_HTML, &pStream, 0, &BindStatusCallback);
    ok(hr == S_OK, "URLOpenBlockingStreamW failed with error 0x%08lx\n", hr);

    CHECK_CALLED(GetBindInfo);
    todo_wine CHECK_CALLED(QueryInterface_IServiceProvider);
    CHECK_CALLED(OnStartBinding);
    CHECK_CALLED(OnProgress_SENDINGREQUEST);
    CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
    CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
    CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
    CHECK_CALLED(OnStopBinding);

    ok(pStream != NULL, "pStream is NULL\n");
    if(pStream)
    {
        buffer[0] = 0;
        hr = IStream_Read(pStream, buffer, sizeof(buffer), NULL);
        ok(hr == S_OK, "IStream_Read failed with error 0x%08lx\n", hr);
        ok(!memcmp(buffer, szHtmlDoc, sizeof(szHtmlDoc)-1), "read data differs from file\n");

        IStream_Release(pStream);
    }

    hr = URLOpenBlockingStreamW(NULL, INDEX_HTML, &pStream, 0, NULL);
    ok(hr == S_OK, "URLOpenBlockingStreamW failed with error 0x%08lx\n", hr);

    ok(pStream != NULL, "pStream is NULL\n");
    if(pStream)
    {
        buffer[0] = 0;
        hr = IStream_Read(pStream, buffer, sizeof(buffer), NULL);
        ok(hr == S_OK, "IStream_Read failed with error 0x%08lx\n", hr);
        ok(!memcmp(buffer, szHtmlDoc, sizeof(szHtmlDoc)-1), "read data differs from file\n");

        IStream_Release(pStream);
    }
}

static void test_URLOpenStreamW(void)
{
    HRESULT hr;

    hr = URLOpenStreamW(NULL, NULL, 0, &BindStatusCallback);
    ok(hr == E_INVALIDARG, "URLOpenStreamW should have failed with E_INVALIDARG instead of 0x%08lx\n", hr);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(QueryInterface_IServiceProvider);
    SET_EXPECT(OnStartBinding);
    SET_EXPECT(OnProgress_SENDINGREQUEST);
    SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
    SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
    SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
    SET_EXPECT(OnDataAvailable);
    SET_EXPECT(OnStopBinding);

    hr = URLOpenStreamW(NULL, INDEX_HTML, 0, &BindStatusCallback);
    ok(hr == S_OK, "URLOpenStreamW failed with error 0x%08lx\n", hr);

    CHECK_CALLED(GetBindInfo);
    todo_wine CHECK_CALLED(QueryInterface_IServiceProvider);
    CHECK_CALLED(OnStartBinding);
    CHECK_CALLED(OnProgress_SENDINGREQUEST);
    CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
    CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
    CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
    CHECK_CALLED(OnDataAvailable);
    CHECK_CALLED(OnStopBinding);

    hr = URLOpenStreamW(NULL, INDEX_HTML, 0, NULL);
    ok(hr == S_OK, "URLOpenStreamW failed with error 0x%08lx\n", hr);
}

START_TEST(stream)
{
    if(!GetProcAddress(GetModuleHandleA("urlmon.dll"), "CompareSecurityIds")) {
        win_skip("Too old IE\n");
        return;
    }

    create_file();
    test_URLOpenBlockingStreamW();
    test_URLOpenStreamW();
    DeleteFileA(wszIndexHtmlA);
}
