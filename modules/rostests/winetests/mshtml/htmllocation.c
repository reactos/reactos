/*
 * Copyright 2009 Andrew Eikum for CodeWeavers
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

#include "mshtml.h"
#include "wininet.h"

struct location_test {
    const char *name;
    const char *url;

    const char *href;
    const char *protocol;
    const char *host;
    const char *hostname;
    const char *port;
    const char *pathname;
    const char *search;
    const char *hash;
};

static const struct location_test location_tests[] = {
    {
        "Empty",
        NULL,
        "about:blank",
        "about:",
        NULL,
        NULL,
        NULL,
        "blank",
        NULL,
        NULL
    },
    {
        "HTTP",
        "http://www.winehq.org?search#hash",
        "http://www.winehq.org/?search#hash",
        "http:",
        "www.winehq.org:80",
        "www.winehq.org",
        "80",
        "/",
        "?search",
        "#hash"
    },
    {
        "HTTP with file",
        "http://www.winehq.org/file?search#hash",
        "http://www.winehq.org/file?search#hash",
        "http:",
        "www.winehq.org:80",
        "www.winehq.org",
        "80",
        "/file",
        "?search",
        "#hash"
    },
    {
        "FTP",
        "ftp://ftp.winehq.org/",
        "ftp://ftp.winehq.org/",
        "ftp:",
        "ftp.winehq.org:21",
        "ftp.winehq.org",
        "21",
        "/",
        NULL,
        NULL
    },
    {
        "FTP with file",
        "ftp://ftp.winehq.org/file",
        "ftp://ftp.winehq.org/file",
        "ftp:",
        "ftp.winehq.org:21",
        "ftp.winehq.org",
        "21",
        "/file",
        NULL,
        NULL
    },
};

static int str_eq_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];

    if(!strw || !stra)
        return (void*)strw == (void*)stra;

    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);
    return !lstrcmpA(stra, buf);
}

static void test_href(IHTMLLocation *loc, const struct location_test *test)
{
    HRESULT hres;
    BSTR str;

    hres = IHTMLLocation_get_href(loc, NULL);
    ok(hres == E_POINTER,
            "%s: get_href should have failed with E_POINTER (0x%08lx), was: 0x%08lx\n",
            test->name, E_POINTER, hres);

    hres = IHTMLLocation_get_href(loc, &str);
    ok(hres == S_OK, "%s: get_href failed: 0x%08lx\n", test->name, hres);
    if(hres == S_OK)
        ok(str_eq_wa(str, test->href),
                "%s: expected retrieved href to be L\"%s\", was: %s\n",
                test->name, test->href, wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLLocation_toString(loc, &str);
    ok(hres == S_OK, "%s: toString failed: 0x%08lx\n", test->name, hres);
    ok(str_eq_wa(str, test->href), "%s: toString returned %s, expected %s\n",
       test->name, wine_dbgstr_w(str), test->href);
    SysFreeString(str);
}

static void test_protocol(IHTMLLocation *loc, const struct location_test *test)
{
    HRESULT hres;
    BSTR str;

    hres = IHTMLLocation_get_protocol(loc, NULL);
    ok(hres == E_POINTER,
            "%s: get_protocol should have failed with E_POINTER (0x%08lx), was: 0x%08lx\n",
            test->name, E_POINTER, hres);

    hres = IHTMLLocation_get_protocol(loc, &str);
    ok(hres == S_OK, "%s: get_protocol failed: 0x%08lx\n", test->name, hres);
    if(hres == S_OK)
        ok(str_eq_wa(str, test->protocol),
                "%s: expected retrieved protocol to be L\"%s\", was: %s\n",
                test->name, test->protocol, wine_dbgstr_w(str));
    SysFreeString(str);
}

static void test_host(IHTMLLocation *loc, const struct location_test *test)
{
    HRESULT hres;
    BSTR str;

    hres = IHTMLLocation_get_host(loc, NULL);
    ok(hres == E_POINTER,
            "%s: get_host should have failed with E_POINTER (0x%08lx), was: 0x%08lx\n",
            test->name, E_POINTER, hres);

    hres = IHTMLLocation_get_host(loc, &str);
    ok(hres == S_OK, "%s: get_host failed: 0x%08lx\n", test->name, hres);
    if(hres == S_OK){
        int len = test->host ? strlen(test->host) : 0;
        ok(str_eq_wa(str, test->host),
                "%s: expected retrieved host to be L\"%s\", was: %s\n",
                test->name, test->host, wine_dbgstr_w(str));
        ok(SysStringLen(str) == len, "%s: unexpected string length %u, expected %u\n",
                test->name, SysStringLen(str), len);
    }

    SysFreeString(str);
}

static void test_hostname(IHTMLLocation *loc, IHTMLDocument2 *doc, const struct location_test *test)
{
    HRESULT hres;
    BSTR str;

    hres = IHTMLLocation_get_hostname(loc, NULL);
    ok(hres == E_POINTER,
            "%s: get_hostname should have failed with E_POINTER (0x%08lx), was: 0x%08lx\n",
            test->name, E_POINTER, hres);

    hres = IHTMLLocation_get_hostname(loc, &str);
    ok(hres == S_OK, "%s: get_hostname failed: 0x%08lx\n", test->name, hres);
    if(hres == S_OK)
        ok(str_eq_wa(str, test->hostname),
                "%s: expected retrieved hostname to be L\"%s\", was: %s\n",
                test->name, test->hostname, wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLDocument2_get_domain(doc, &str);
    ok(hres == (test->url ? S_OK : E_FAIL), "%s: get_domain failed: 0x%08lx\n", test->name, hres);
    if(hres == S_OK)
        ok(str_eq_wa(str, test->hostname ? test->hostname : ""),
                "%s: expected retrieved domain to be L\"%s\", was: %s\n",
                test->name, test->hostname, wine_dbgstr_w(str));
    SysFreeString(str);
}

static void test_port(IHTMLLocation *loc, const struct location_test *test)
{
    HRESULT hres;
    BSTR str;

    hres = IHTMLLocation_get_port(loc, NULL);
    ok(hres == E_POINTER,
            "%s: get_port should have failed with E_POINTER (0x%08lx), was: 0x%08lx\n",
            test->name, E_POINTER, hres);

    hres = IHTMLLocation_get_port(loc, &str);
    ok(hres == S_OK, "%s: get_port failed: 0x%08lx\n", test->name, hres);
    if(hres == S_OK)
        ok(str_eq_wa(str, test->port)
           || (!str && !*test->port), /* IE11 */
                "%s: expected retrieved port to be L\"%s\", was: %s\n",
                test->name, test->port, wine_dbgstr_w(str));
    SysFreeString(str);
}

static void test_pathname(IHTMLLocation *loc, const struct location_test *test)
{
    HRESULT hres;
    BSTR str;

    hres = IHTMLLocation_get_pathname(loc, NULL);
    ok(hres == E_POINTER,
            "%s: get_pathname should have failed with E_POINTER (0x%08lx), was: 0x%08lx\n",
            test->name, E_POINTER, hres);

    hres = IHTMLLocation_get_pathname(loc, &str);
    ok(hres == S_OK, "%s: get_pathname failed: 0x%08lx\n", test->name, hres);
    if(hres == S_OK)
        /* FIXME: We seem to be testing location in a buggy state. Path names do not
         * include leading slash, while other tests confirm that it should be there */
        todo_wine_if(*test->pathname == '/')
        ok(str_eq_wa(str, *test->pathname == '/' ? test->pathname + 1 : test->pathname),
                "%s: expected retrieved pathname to be L\"%s\", was: %s\n",
                test->name, test->pathname, wine_dbgstr_w(str));
    SysFreeString(str);
}

static void test_search(IHTMLLocation *loc, const struct location_test *test)
{
    HRESULT hres;
    BSTR str;

    hres = IHTMLLocation_get_search(loc, NULL);
    ok(hres == E_POINTER,
            "%s: get_search should have failed with E_POINTER (0x%08lx), was: 0x%08lx\n",
            test->name, E_POINTER, hres);

    hres = IHTMLLocation_get_search(loc, &str);
    ok(hres == S_OK, "%s: get_search failed: 0x%08lx\n", test->name, hres);
    if(hres == S_OK)
        ok(str_eq_wa(str, test->search),
                "%s: expected retrieved search to be L\"%s\", was: %s\n",
                test->name, test->search, wine_dbgstr_w(str));
    SysFreeString(str);
}

static void test_hash(IHTMLLocation *loc, const struct location_test *test)
{
    HRESULT hres;
    BSTR str;

    hres = IHTMLLocation_get_hash(loc, NULL);
    ok(hres == E_POINTER,
            "%s: get_hash should have failed with E_POINTER (0x%08lx), was: 0x%08lx\n",
            test->name, E_POINTER, hres);

    hres = IHTMLLocation_get_hash(loc, &str);
    ok(hres == S_OK, "%s: get_hash failed: 0x%08lx\n", test->name, hres);
    if(hres == S_OK)
        ok(str_eq_wa(str, test->hash),
                "%s: expected retrieved hash to be L\"%s\", was: %s\n",
                test->name, test->hash, wine_dbgstr_w(str));
    SysFreeString(str);
}

static void perform_test(const struct location_test* test)
{
    WCHAR url[INTERNET_MAX_URL_LENGTH];
    HRESULT hres;
    IBindCtx *bc;
    IMoniker *url_mon = NULL;
    IPersistMoniker *persist_mon;
    IHTMLDocument2 *doc;
    IHTMLDocument6 *doc6;
    IHTMLLocation *location;

    hres = CreateBindCtx(0, &bc);
    ok(hres == S_OK, "%s: CreateBindCtx failed: 0x%08lx\n", test->name, hres);
    if(FAILED(hres))
        return;

    if(test->url) {
        MultiByteToWideChar(CP_ACP, 0, test->url, -1, url, ARRAY_SIZE(url));
        hres = CreateURLMoniker(NULL, url, &url_mon);
        ok(hres == S_OK, "%s: CreateURLMoniker failed: 0x%08lx\n", test->name, hres);
        if(FAILED(hres)){
            IBindCtx_Release(bc);
            return;
        }
    }

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER, &IID_IHTMLDocument2,
            (void**)&doc);
#if !defined(__i386__) && !defined(__x86_64__)
    todo_wine
#endif
    ok(hres == S_OK, "%s: CoCreateInstance failed: 0x%08lx\n", test->name, hres);
    if(FAILED(hres)){
        if(url_mon) IMoniker_Release(url_mon);
        IBindCtx_Release(bc);
        return;
    }

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument6, (void**)&doc6);
    if(hres == S_OK){
        IHTMLDocument6_Release(doc6);
    }else{
        win_skip("%s: Could not get IHTMLDocument6, probably too old IE. Requires IE 8+\n", test->name);
        if(url_mon) IMoniker_Release(url_mon);
        IBindCtx_Release(bc);
        return;
    }

    if(url_mon) {
        hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistMoniker,
                (void**)&persist_mon);
        ok(hres == S_OK, "%s: IHTMlDocument2_QueryInterface failed: 0x%08lx\n", test->name, hres);
        if(FAILED(hres)){
            IHTMLDocument2_Release(doc);
            IMoniker_Release(url_mon);
            IBindCtx_Release(bc);
            return;
        }

        hres = IPersistMoniker_Load(persist_mon, FALSE, url_mon, bc,
                STGM_SHARE_EXCLUSIVE | STGM_READWRITE);
        ok(hres == S_OK, "%s: IPersistMoniker_Load failed: 0x%08lx\n", test->name, hres);
        IPersistMoniker_Release(persist_mon);
        IMoniker_Release(url_mon);

        if(FAILED(hres)){
            IHTMLDocument2_Release(doc);
            IBindCtx_Release(bc);
            return;
        }
    }

    hres = IHTMLDocument2_get_location(doc, &location);
    ok(hres == S_OK, "%s: IHTMLDocument2_get_location failed: 0x%08lx\n", test->name, hres);
    if(FAILED(hres)){
        IHTMLDocument2_Release(doc);
        IBindCtx_Release(bc);
        return;
    }

    test_href(location, test);
    test_protocol(location, test);
    test_host(location, test);
    test_hostname(location, doc, test);
    test_port(location, test);
    test_pathname(location, test);
    test_search(location, test);
    test_hash(location, test);

    IHTMLLocation_Release(location);
    IHTMLDocument2_Release(doc);
    IBindCtx_Release(bc);
}

START_TEST(htmllocation)
{
    int i;

    CoInitialize(NULL);

    for(i=0; i < ARRAY_SIZE(location_tests); i++)
        perform_test(location_tests+i);

    CoUninitialize();
}
