/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "urlmon.h"

#include "initguid.h"

DEFINE_GUID(CLSID_AboutProtocol, 0x3050F406, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);

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

DEFINE_EXPECT(ParseUrl);
DEFINE_EXPECT(ParseUrl_ENCODE);
DEFINE_EXPECT(ParseUrl_UNESCAPE);
DEFINE_EXPECT(QI_IInternetProtocolInfo);
DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(unk_Release);

static HRESULT (WINAPI *pCoInternetCompareUrl)(LPCWSTR, LPCWSTR, DWORD);
static HRESULT (WINAPI *pCoInternetGetSecurityUrl)(LPCWSTR, LPWSTR*, PSUACTION, DWORD);
static HRESULT (WINAPI *pCoInternetGetSession)(DWORD, IInternetSession **, DWORD);
static HRESULT (WINAPI *pCoInternetParseUrl)(LPCWSTR, PARSEACTION, DWORD, LPWSTR, DWORD, DWORD *, DWORD);
static HRESULT (WINAPI *pCoInternetQueryInfo)(LPCWSTR, QUERYOPTION, DWORD, LPVOID, DWORD, DWORD *, DWORD);
static HRESULT (WINAPI *pCopyStgMedium)(const STGMEDIUM *, STGMEDIUM *);
static HRESULT (WINAPI *pCopyBindInfo)(const BINDINFO *, BINDINFO *);
static HRESULT (WINAPI *pFindMimeFromData)(LPBC, LPCWSTR, LPVOID, DWORD, LPCWSTR,
                        DWORD, LPWSTR*, DWORD);
static HRESULT (WINAPI *pObtainUserAgentString)(DWORD, LPSTR, DWORD*);
static HRESULT (WINAPI *pReleaseBindInfo)(BINDINFO*);
static HRESULT (WINAPI *pUrlMkGetSessionOption)(DWORD, LPVOID, DWORD, DWORD *, DWORD);
static HRESULT (WINAPI *pCompareSecurityIds)(BYTE*,DWORD,BYTE*,DWORD,DWORD);
static HRESULT (WINAPI *pCoInternetIsFeatureEnabled)(INTERNETFEATURELIST,DWORD);
static HRESULT (WINAPI *pCoInternetSetFeatureEnabled)(INTERNETFEATURELIST,DWORD,BOOL);
static HRESULT (WINAPI *pIEInstallScope)(DWORD*);
static HRESULT (WINAPI *pMapBrowserEmulationModeToUserAgent)(const void*,WCHAR**);

static WCHAR *a2co(const char *str)
{
    WCHAR *ret;
    int len;

    if(!str)
        return NULL;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = CoTaskMemAlloc(len*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static void test_CreateFormatEnum(void)
{
    IEnumFORMATETC *fenum = NULL, *fenum2 = NULL;
    FORMATETC fetc[5];
    ULONG ul;
    HRESULT hres;

    static DVTARGETDEVICE dev = {sizeof(dev),0,0,0,0,{0}};
    static FORMATETC formatetc[] = {
        {0,&dev,0,0,0},
        {0,&dev,0,1,0},
        {0,NULL,0,2,0},
        {0,NULL,0,3,0},
        {0,NULL,0,4,0}
    };

    hres = CreateFormatEnumerator(0, formatetc, &fenum);
    ok(hres == E_FAIL, "CreateFormatEnumerator failed: %08lx, expected E_FAIL\n", hres);
    hres = CreateFormatEnumerator(0, formatetc, NULL);
    ok(hres == E_INVALIDARG, "CreateFormatEnumerator failed: %08lx, expected E_INVALIDARG\n", hres);
    hres = CreateFormatEnumerator(5, formatetc, NULL);
    ok(hres == E_INVALIDARG, "CreateFormatEnumerator failed: %08lx, expected E_INVALIDARG\n", hres);


    hres = CreateFormatEnumerator(5, formatetc, &fenum);
    ok(hres == S_OK, "CreateFormatEnumerator failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IEnumFORMATETC_Next(fenum, 2, NULL, &ul);
    ok(hres == E_INVALIDARG, "Next failed: %08lx, expected E_INVALIDARG\n", hres);
    ul = 100;
    hres = IEnumFORMATETC_Next(fenum, 0, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(ul == 0, "ul=%ld, expected 0\n", ul);

    hres = IEnumFORMATETC_Next(fenum, 2, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(fetc[0].lindex == 0, "fetc[0].lindex=%ld, expected 0\n", fetc[0].lindex);
    ok(fetc[1].lindex == 1, "fetc[1].lindex=%ld, expected 1\n", fetc[1].lindex);
    ok(fetc[0].ptd == &dev, "fetc[0].ptd=%p, expected %p\n", fetc[0].ptd, &dev);
    ok(ul == 2, "ul=%ld, expected 2\n", ul);

    hres = IEnumFORMATETC_Skip(fenum, 1);
    ok(hres == S_OK, "Skip failed: %08lx\n", hres);

    hres = IEnumFORMATETC_Next(fenum, 4, fetc, &ul);
    ok(hres == S_FALSE, "Next failed: %08lx, expected S_FALSE\n", hres);
    ok(fetc[0].lindex == 3, "fetc[0].lindex=%ld, expected 3\n", fetc[0].lindex);
    ok(fetc[1].lindex == 4, "fetc[1].lindex=%ld, expected 4\n", fetc[1].lindex);
    ok(fetc[0].ptd == NULL, "fetc[0].ptd=%p, expected NULL\n", fetc[0].ptd);
    ok(ul == 2, "ul=%ld, expected 2\n", ul);

    hres = IEnumFORMATETC_Next(fenum, 4, fetc, &ul);
    ok(hres == S_FALSE, "Next failed: %08lx, expected S_FALSE\n", hres);
    ok(ul == 0, "ul=%ld, expected 0\n", ul);
    ul = 100;
    hres = IEnumFORMATETC_Next(fenum, 0, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(ul == 0, "ul=%ld, expected 0\n", ul);

    hres = IEnumFORMATETC_Skip(fenum, 3);
    ok(hres == S_FALSE, "Skip failed: %08lx, expected S_FALSE\n", hres);

    hres = IEnumFORMATETC_Reset(fenum);
    ok(hres == S_OK, "Reset failed: %08lx\n", hres);

    hres = IEnumFORMATETC_Next(fenum, 5, fetc, NULL);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(fetc[0].lindex == 0, "fetc[0].lindex=%ld, expected 0\n", fetc[0].lindex);

    hres = IEnumFORMATETC_Reset(fenum);
    ok(hres == S_OK, "Reset failed: %08lx\n", hres);

    hres = IEnumFORMATETC_Skip(fenum, 2);
    ok(hres == S_OK, "Skip failed: %08lx\n", hres);

    hres = IEnumFORMATETC_Clone(fenum, NULL);
    ok(hres == E_INVALIDARG, "Clone failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = IEnumFORMATETC_Clone(fenum, &fenum2);
    ok(hres == S_OK, "Clone failed: %08lx\n", hres);

    if(SUCCEEDED(hres)) {
        ok(fenum != fenum2, "fenum == fenum2\n");

        hres = IEnumFORMATETC_Next(fenum2, 2, fetc, &ul);
        ok(hres == S_OK, "Next failed: %08lx\n", hres);
        ok(fetc[0].lindex == 2, "fetc[0].lindex=%ld, expected 2\n", fetc[0].lindex);

        IEnumFORMATETC_Release(fenum2);
    }

    hres = IEnumFORMATETC_Next(fenum, 2, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(fetc[0].lindex == 2, "fetc[0].lindex=%ld, expected 2\n", fetc[0].lindex);

    hres = IEnumFORMATETC_Skip(fenum, 1);
    ok(hres == S_OK, "Skip failed: %08lx\n", hres);
    
    IEnumFORMATETC_Release(fenum);
}

static void test_RegisterFormatEnumerator(void)
{
    IBindCtx *bctx = NULL;
    IEnumFORMATETC *format = NULL, *format2 = NULL;
    IUnknown *unk = NULL;
    HRESULT hres;

    static FORMATETC formatetc = {0,NULL,0,0,0};
    static WCHAR wszEnumFORMATETC[] =
        {'_','E','n','u','m','F','O','R','M','A','T','E','T','C','_',0};

    CreateBindCtx(0, &bctx);

    hres = CreateFormatEnumerator(1, &formatetc, &format);
    ok(hres == S_OK, "CreateFormatEnumerator failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = RegisterFormatEnumerator(NULL, format, 0);
    ok(hres == E_INVALIDARG,
            "RegisterFormatEnumerator failed: %08lx, expected E_INVALIDARG\n", hres);
    hres = RegisterFormatEnumerator(bctx, NULL, 0);
    ok(hres == E_INVALIDARG,
            "RegisterFormatEnumerator failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = RegisterFormatEnumerator(bctx, format, 0);
    ok(hres == S_OK, "RegisterFormatEnumerator failed: %08lx\n", hres);

    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == S_OK, "GetObjectParam failed: %08lx\n", hres);
    ok(unk == (IUnknown*)format, "unk != format\n");

    hres = RevokeFormatEnumerator(NULL, format);
    ok(hres == E_INVALIDARG,
            "RevokeFormatEnumerator failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = RevokeFormatEnumerator(bctx, format);
    ok(hres == S_OK, "RevokeFormatEnumerator failed: %08lx\n", hres);

    hres = RevokeFormatEnumerator(bctx, format);
    ok(hres == E_FAIL, "RevokeFormatEnumerator failed: %08lx, expected E_FAIL\n", hres);

    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == E_FAIL, "GetObjectParam failed: %08lx, expected E_FAIL\n", hres);

    hres = RegisterFormatEnumerator(bctx, format, 0);
    ok(hres == S_OK, "RegisterFormatEnumerator failed: %08lx\n", hres);

    hres = CreateFormatEnumerator(1, &formatetc, &format2);
    ok(hres == S_OK, "CreateFormatEnumerator failed: %08lx\n", hres);

    if(SUCCEEDED(hres)) {
        hres = RevokeFormatEnumerator(bctx, format);
        ok(hres == S_OK, "RevokeFormatEnumerator failed: %08lx\n", hres);

        IEnumFORMATETC_Release(format2);
    }

    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == E_FAIL, "GetObjectParam failed: %08lx, expected E_FAIL\n", hres);

    IEnumFORMATETC_Release(format);

    hres = RegisterFormatEnumerator(bctx, format, 0);
    ok(hres == S_OK, "RegisterFormatEnumerator failed: %08lx\n", hres);
    hres = RevokeFormatEnumerator(bctx, NULL);
    ok(hres == S_OK, "RevokeFormatEnumerator failed: %08lx\n", hres);
    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == E_FAIL, "GetObjectParam failed: %08lx, expected E_FAIL\n", hres);

    IEnumFORMATETC_Release(format);
    IBindCtx_Release(bctx);
}
static const WCHAR url1[] = {'r','e','s',':','/','/','m','s','h','t','m','l','.','d','l','l',
        '/','b','l','a','n','k','.','h','t','m',0};
static const WCHAR url2[] = {'i','n','d','e','x','.','h','t','m',0};
static const WCHAR url3[] = {'f','i','l','e',':','/','/','c',':','\\','I','n','d','e','x','.','h','t','m',0};
static const WCHAR url4[] = {'f','i','l','e',':','s','o','m','e','%','2','0','f','i','l','e',
        '%','2','e','j','p','g',0};
static const WCHAR url5[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q',
        '.','o','r','g',0};
static const WCHAR url6[] = {'a','b','o','u','t',':','b','l','a','n','k',0};
static const WCHAR url7[] = {'f','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g','/',
        'f','i','l','e','.','t','e','s','t',0};
static const WCHAR url8[] = {'t','e','s','t',':','1','2','3','a','b','c',0};
static const WCHAR url9[] =
    {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g',
     '/','s','i','t','e','/','a','b','o','u','t',0};
static const WCHAR url10[] = {'h','t','t','p',':','/','/','g','o','o','g','l','e','.','*','.',
        'c','o','m',0};
static const WCHAR url4e[] = {'f','i','l','e',':','s','o','m','e',' ','f','i','l','e',
        '.','j','p','g',0};

static const WCHAR path3[] = {'c',':','\\','I','n','d','e','x','.','h','t','m',0};
static const WCHAR path4[] = {'s','o','m','e',' ','f','i','l','e','.','j','p','g',0};

static const WCHAR wszRes[] = {'r','e','s',0};
static const WCHAR wszFile[] = {'f','i','l','e',0};
static const WCHAR wszHttp[] = {'h','t','t','p',0};
static const WCHAR wszAbout[] = {'a','b','o','u','t',0};
static const WCHAR wszEmpty[] = {0};
static const WCHAR wszGoogle[] = {'g','o','o','g','l','e','.','*','.','c','o','m',0};

static const WCHAR wszWineHQ[] = {'w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR wszHttpWineHQ[] = {'h','t','t','p',':','/','/','w','w','w','.',
    'w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR wszHttpGoogle[] = {'h','t','t','p',':','/','/','g','o','o','g','l','e',
    '.','*','.','c','o','m',0};

struct parse_test {
    LPCWSTR url;
    HRESULT secur_hres;
    LPCWSTR encoded_url;
    HRESULT path_hres;
    LPCWSTR path;
    LPCWSTR schema;
    LPCWSTR domain;
    HRESULT domain_hres;
    LPCWSTR rootdocument;
    HRESULT rootdocument_hres;
};

static const struct parse_test parse_tests[] = {
    {url1, S_OK,   url1,  E_INVALIDARG, NULL, wszRes, NULL, E_FAIL, NULL, E_FAIL},
    {url2, E_FAIL, url2,  E_INVALIDARG, NULL, wszEmpty, NULL, E_FAIL, NULL, E_FAIL},
    {url3, E_FAIL, url3,  S_OK, path3,        wszFile, wszEmpty, S_OK, NULL, E_FAIL},
    {url4, E_FAIL, url4e, S_OK, path4,        wszFile, wszEmpty, S_OK, NULL, E_FAIL},
    {url5, E_FAIL, url5,  E_INVALIDARG, NULL, wszHttp, wszWineHQ, S_OK, wszHttpWineHQ, S_OK},
    {url6, S_OK,   url6,  E_INVALIDARG, NULL, wszAbout, NULL, E_FAIL, NULL, E_FAIL},
    {url10, E_FAIL, url10, E_INVALIDARG,NULL, wszHttp, wszGoogle, S_OK, wszHttpGoogle, S_OK}
};

static void test_CoInternetParseUrl(void)
{
    HRESULT hres;
    DWORD size;
    int i;

    static WCHAR buf[4096];

    memset(buf, 0xf0, sizeof(buf));
    hres = pCoInternetParseUrl(parse_tests[0].url, PARSE_SCHEMA, 0, buf,
            3, &size, 0);
    ok(hres == E_POINTER, "schema failed: %08lx, expected E_POINTER\n", hres);

    for(i = 0; i < ARRAY_SIZE(parse_tests); i++) {
        memset(buf, 0xf0, sizeof(buf));
        hres = pCoInternetParseUrl(parse_tests[i].url, PARSE_SECURITY_URL, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == parse_tests[i].secur_hres, "[%d] security url failed: %08lx, expected %08lx\n",
                i, hres, parse_tests[i].secur_hres);

        memset(buf, 0xf0, sizeof(buf));
        hres = pCoInternetParseUrl(parse_tests[i].url, PARSE_ENCODE, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == S_OK, "[%d] encoding failed: %08lx\n", i, hres);
        ok(size == lstrlenW(parse_tests[i].encoded_url), "[%d] wrong size\n", i);
        ok(!lstrcmpW(parse_tests[i].encoded_url, buf), "[%d] wrong encoded url\n", i);

        memset(buf, 0xf0, sizeof(buf));
        hres = pCoInternetParseUrl(parse_tests[i].url, PARSE_UNESCAPE, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == S_OK, "[%d] encoding failed: %08lx\n", i, hres);
        ok(size == lstrlenW(parse_tests[i].encoded_url), "[%d] wrong size\n", i);
        ok(!lstrcmpW(parse_tests[i].encoded_url, buf), "[%d] wrong encoded url\n", i);

        memset(buf, 0xf0, sizeof(buf));
        hres = pCoInternetParseUrl(parse_tests[i].url, PARSE_PATH_FROM_URL, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == parse_tests[i].path_hres, "[%d] path failed: %08lx, expected %08lx\n",
                i, hres, parse_tests[i].path_hres);
        if(parse_tests[i].path) {
            ok(size == lstrlenW(parse_tests[i].path), "[%d] wrong size\n", i);
            ok(!lstrcmpW(parse_tests[i].path, buf), "[%d] wrong path\n", i);
        }

        memset(buf, 0xf0, sizeof(buf));
        hres = pCoInternetParseUrl(parse_tests[i].url, PARSE_SCHEMA, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == S_OK, "[%d] schema failed: %08lx\n", i, hres);
        ok(size == lstrlenW(parse_tests[i].schema), "[%d] wrong size\n", i);
        ok(!lstrcmpW(parse_tests[i].schema, buf), "[%d] wrong schema\n", i);

        if(memcmp(parse_tests[i].url, wszRes, 3*sizeof(WCHAR))
                && memcmp(parse_tests[i].url, wszAbout, 5*sizeof(WCHAR))) {
            memset(buf, 0xf0, sizeof(buf));
            hres = pCoInternetParseUrl(parse_tests[i].url, PARSE_DOMAIN, 0, buf,
                    ARRAY_SIZE(buf), &size, 0);
            ok(hres == parse_tests[i].domain_hres, "[%d] domain failed: %08lx\n", i, hres);
            if(parse_tests[i].domain)
                ok(!lstrcmpW(parse_tests[i].domain, buf), "[%d] wrong domain, received %s\n", i, wine_dbgstr_w(buf));
        }

        memset(buf, 0xf0, sizeof(buf));
        hres = pCoInternetParseUrl(parse_tests[i].url, PARSE_ROOTDOCUMENT, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == parse_tests[i].rootdocument_hres, "[%d] rootdocument failed: %08lx\n", i, hres);
        if(parse_tests[i].rootdocument)
            ok(!lstrcmpW(parse_tests[i].rootdocument, buf), "[%d] wrong rootdocument, received %s\n", i, wine_dbgstr_w(buf));
    }

    size = 0xdeadbeef;
    hres = pCoInternetParseUrl(L"http://a/b/../c", PARSE_CANONICALIZE, 0, buf, 3, &size, 0);
    ok(hres == E_POINTER, "got %#lx\n", hres);
    ok(size == wcslen(L"http://a/c") + 1, "got %lu\n", size);

    size = 0xdeadbeef;
    hres = pCoInternetParseUrl(L"http://a/b/../c", PARSE_CANONICALIZE, 0, buf, sizeof(buf), &size, 0);
    ok(hres == S_OK, "got %#lx\n", hres);
    ok(!wcscmp(buf, L"http://a/c"), "got %s\n", debugstr_w(buf));
    ok(size == wcslen(buf), "got %lu\n", size);
}

static void test_CoInternetCompareUrl(void)
{
    HRESULT hres;

    hres = pCoInternetCompareUrl(url1, url1, 0);
    ok(hres == S_OK, "CoInternetCompareUrl failed: %08lx\n", hres);

    hres = pCoInternetCompareUrl(url1, url3, 0);
    ok(hres == S_FALSE, "CoInternetCompareUrl failed: %08lx\n", hres);

    hres = pCoInternetCompareUrl(url3, url1, 0);
    ok(hres == S_FALSE, "CoInternetCompareUrl failed: %08lx\n", hres);
}

static const struct {
    LPCWSTR url;
    DWORD uses_net;
} query_info_tests[] = {
    {url1, 0},
    {url2, 0},
    {url3, 0},
    {url4, 0},
    {url5, 0},
    {url6, 0},
    {url7, 0},
    {url8, 0}
};

static void test_CoInternetQueryInfo(void)
{
    BYTE buf[100];
    DWORD cb, i;
    HRESULT hres;

    for(i = 0; i < ARRAY_SIZE(query_info_tests); i++) {
        cb = 0xdeadbeef;
        memset(buf, '?', sizeof(buf));
        hres = pCoInternetQueryInfo(query_info_tests[0].url, QUERY_USES_NETWORK, 0, buf, sizeof(buf), &cb, 0);
        ok(hres == S_OK, "[%ld] CoInternetQueryInfo failed: %08lx\n", i, hres);
        ok(cb == sizeof(DWORD), "[%ld] cb = %ld\n", i, cb);
        ok(*(DWORD*)buf == query_info_tests[i].uses_net, "[%ld] ret %lx, expected %lx\n",
           i, *(DWORD*)buf, query_info_tests[i].uses_net);

        hres = pCoInternetQueryInfo(query_info_tests[0].url, QUERY_USES_NETWORK, 0, buf, 3, &cb, 0);
        ok(hres == E_FAIL, "[%ld] CoInternetQueryInfo failed: %08lx, expected E_FAIL\n", i, hres);
        hres = pCoInternetQueryInfo(query_info_tests[0].url, QUERY_USES_NETWORK, 0, NULL, sizeof(buf), &cb, 0);
        ok(hres == E_FAIL, "[%ld] CoInternetQueryInfo failed: %08lx, expected E_FAIL\n", i, hres);

        memset(buf, '?', sizeof(buf));
        hres = pCoInternetQueryInfo(query_info_tests[0].url, QUERY_USES_NETWORK, 0, buf, sizeof(buf), NULL, 0);
        ok(hres == S_OK, "[%ld] CoInternetQueryInfo failed: %08lx\n", i, hres);
        ok(*(DWORD*)buf == query_info_tests[i].uses_net, "[%ld] ret %lx, expected %lx\n",
           i, *(DWORD*)buf, query_info_tests[i].uses_net);
    }
}

static const struct {
    const WCHAR *url;
    const WCHAR *mime;
    HRESULT hres;
    BOOL broken_failure;
    const WCHAR *broken_mime;
} mime_tests[] = {
    {L"res://mshtml.dll/blank.htm", L"text/html", S_OK},
    {L"index.htm", L"text/html", S_OK},
    {L"file://c:\\Index.htm", L"text/html", S_OK},
    {L"file://c:\\Index.htm?q=test", L"text/html", S_OK, TRUE},
    {L"file://c:\\Index.htm#hash_part", L"text/html", S_OK, TRUE},
    {L"file://c:\\Index.htm#hash_part.txt", L"text/html", S_OK, FALSE, L"text/plain"},
    {L"file://some%20file%2ejpg", NULL, E_FAIL},
    {L"http://www.winehq.org", NULL, __HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)},
    {L"about:blank", NULL, E_FAIL},
    {L"ftp://winehq.org/file.test", NULL, __HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)}
};

static BYTE data1[] = "test data\n";
static BYTE data2[] = {31,'t','e','s',0xfa,'t',' ','d','a','t','a','\n',0};
static BYTE data3[] = {0,0,0};
static BYTE data4[] = {'t','e','s',0xfa,'t',' ','d','a','t','a','\n',0,0};
static BYTE data5[] = {0xa,0xa,0xa,'x',32,'x',0};
static BYTE data6[] = {0xfa,0xfa,0xfa,0xfa,'\n','\r','\t','x','x','x',1};
static BYTE data7[] = "<html>blahblah";
static BYTE data8[] = {'t','e','s',0xfa,'t',' ','<','h','t','m','l','>','d','a','t','a','\n',0,0};
static BYTE data9[] = {'t','e',0,'s',0xfa,'t',' ','<','h','t','m','l','>','d','a','t','a','\n',0,0};
static BYTE data10[] = "<HtmL>blahblah";
static BYTE data11[] = "blah<HTML>blahblah";
static BYTE data12[] = "blah<HTMLblahblah";
static BYTE data13[] = "blahHTML>blahblah";
static BYTE data14[] = "blah<HTMblahblah";
static BYTE data15[] = {0xff,0xd8};
static BYTE data16[] = {0xff,0xd8,'h'};
static BYTE data17[] = {0,0xff,0xd8};
static BYTE data18[] = {0xff,0xd8,'<','h','t','m','l','>'};
static BYTE data19[] = {'G','I','F','8','7','a'};
static BYTE data20[] = {'G','I','F','8','9','a'};
static BYTE data21[] = {'G','I','F','8','7'};
static BYTE data22[] = {'G','i','F','8','7','a'};
static BYTE data23[] = {'G','i','F','8','8','a'};
static BYTE data24[] = {'g','i','f','8','7','a'};
static BYTE data25[] = {'G','i','F','8','7','A'};
static BYTE data26[] = {'G','i','F','8','7','a','<','h','t','m','l','>'};
static BYTE data27[] = {0x30,'G','i','F','8','7','A'};
static BYTE data28[] = {0x42,0x4d,0x6e,0x42,0x1c,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00};
static BYTE data29[] = {0x42,0x4d,'x','x','x','x',0x00,0x00,0x00,0x00,'x','x','x','x'};
static BYTE data30[] = {0x42,0x4d,'x','x','x','x',0x00,0x01,0x00,0x00,'x','x','x','x'};
static BYTE data31[] = {0x42,0x4d,'x','x','x','x',0x00,0x00,0x00,0x00,'<','h','t','m','l','>'};
static BYTE data32[] = {0x42,0x4d,'x','x','x','x',0x00,0x00,0x00,0x00,'x','x','x'};
static BYTE data33[] = {0x00,0x42,0x4d,'x','x','x','x',0x00,0x00,0x00,0x00,'x','x','x'};
static BYTE data34[] = {0x89,'P','N','G',0x0d,0x0a,0x1a,0x0a,'x'};
static BYTE data35[] = {0x89,'P','N','G',0x0d,0x0a,0x1a,0x0a,'x','x','x','x',0};
static BYTE data36[] = {0x89,'P','N','G',0x0d,0x0a,0x1a,'x','x'};
static BYTE data37[] = {0x89,'P','N','G',0x0d,0x0a,0x1a,0x0a,'<','h','t','m','l','>'};
static BYTE data38[] = {0x00,0x89,'P','N','G',0x0d,0x0a,0x1a,0x0a,'x'};
static BYTE data39[] = {0x4d,0x4d,0x00,0x2a,0xff};
static BYTE data40[] = {0x4d,0x4d,0x00,0x2a,'<','h','t','m','l','>',0};
static BYTE data41[] = {0x4d,0x4d,0xff};
static BYTE data42[] = {0x4d,0x4d};
static BYTE data43[] = {0x00,0x4d,0x4d,0x00};
static BYTE data44[] = {'R','I','F','F',0xff,0xff,0xff,0xff,'A','V','I',0x20,0xff};
static BYTE data45[] = {'R','I','F','f',0xff,0xff,0xff,0xff,'A','V','I',0x20,0xff};
static BYTE data46[] = {'R','I','F','F',0xff,0xff,0xff,0xff,'A','V','I',0x20};
static BYTE data47[] = {'R','I','F','F',0xff,0xff,0xff,0xff,'A','V','I',0x21,0xff};
static BYTE data48[] = {'R','I','F','F',0xff,0xff,0xff,0xff,'A','V','I',0x20,'<','h','t','m','l','>'};
static BYTE data49[] = {'R','I','F','F',0x0f,0x0f,0xf0,0xf0,'A','V','I',0x20,0xf0,0x00};
static BYTE data50[] = {0x00,0x00,0x01,0xb3,0xff};
static BYTE data51[] = {0x00,0x00,0x01,0xba,0xff};
static BYTE data52[] = {0x00,0x00,0x01,0xb8,0xff};
static BYTE data53[] = {0x00,0x00,0x01,0xba};
static BYTE data54[] = {0x00,0x00,0x01,0xba,'<','h','t','m','l','>'};
static BYTE data55[] = {0x1f,0x8b,'x'};
static BYTE data56[] = {0x1f};
static BYTE data57[] = {0x1f,0x8b,'<','h','t','m','l','>','t','e','s','t',0};
static BYTE data58[] = {0x1f,0x8b};
static BYTE data59[] = {0x50,0x4b,'x'};
static BYTE data60[] = {0x50,0x4b};
static BYTE data61[] = {0x50,0x4b,'<','h','t','m','l','>',0};
static BYTE data62[] = {0xca,0xfe,0xba,0xbe,'x'};
static BYTE data63[] = {0xca,0xfe,0xba,0xbe};
static BYTE data64[] = {0xca,0xfe,0xba,0xbe,'<','h','t','m','l','>',0};
static BYTE data65[] = {0x25,0x50,0x44,0x46,'x'};
static BYTE data66[] = {0x25,0x50,0x44,0x46};
static BYTE data67[] = {0x25,0x50,0x44,0x46,'x','<','h','t','m','l','>'};
static BYTE data68[] = {'M','Z','x'};
static BYTE data69[] = {'M','Z'};
static BYTE data70[] = {'M','Z','<','h','t','m','l','>',0xff};
static BYTE data71[] = {'{','\\','r','t','f',0};
static BYTE data72[] = {'{','\\','r','t','f'};
static BYTE data73[] = {' ','{','\\','r','t','f',' '};
static BYTE data74[] = {'{','\\','r','t','f','<','h','t','m','l','>',' '};
static BYTE data75[] = {'R','I','F','F',0xff,0xff,0xff,0xff,'W','A','V','E',0xff};
static BYTE data76[] = {'R','I','F','F',0xff,0xff,0xff,0xff,'W','A','V','E'};
static BYTE data77[] = {'R','I','F','F',0xff,0xff,0xff,0xff,'W','A','V',0xff,0xff};
static BYTE data78[] = {'R','I','F','F',0xff,0xff,0xff,0xff,'<','h','t','m','l','>',0xff};
static BYTE data79[] = {'%','!',0xff};
static BYTE data80[] = {'%','!'};
static BYTE data81[] = {'%','!','P','S','<','h','t','m','l','>'};
static BYTE data82[] = {'.','s','n','d',0};
static BYTE data83[] = {'.','s','n','d'};
static BYTE data84[] = {'.','s','n','d',0,'<','h','t','m','l','>',1,1};
static BYTE data85[] = {'.','S','N','D',0};
static BYTE data86[] = {0x49,0x49,0x2a,0xff};
static BYTE data87[] = {' ','<','h','e','a','d'};
static BYTE data88[] = {' ','<','h','e','a','d','>'};
static BYTE data89[] = {'\t','\r','<','h','e','a','d','>'};
static BYTE data90[] = {'<','H','e','A','d',' '};
static BYTE data91[] = {'<','?','x','m','l',' ',0};
static BYTE data92[] = {'a','b','c','<','?','x','m','l',' ',' '};
static BYTE data93[] = {'<','?','x','m','l',' ',' ','<','h','t','m','l','>'};
static BYTE data94[] = {'<','h','t','m','l','>','<','?','x','m','l',' ',' '};
static BYTE data95[] = {'{','\\','r','t','f','<','?','x','m','l',' ',' '};
static BYTE data96[] = {'<','?','x','m','l',' '};
static BYTE data97[] = "<body";
static BYTE data98[] = "blah<BoDyblahblah";

static const struct {
    BYTE *data;
    DWORD size;
    const WCHAR *mime;
    const WCHAR *mime_pjpeg;
    const WCHAR *broken_mime;
    const WCHAR *url;
    const WCHAR *proposed_mime;
} mime_tests2[] = {
    {data1, sizeof(data1), L"text/plain"},
    {data2, sizeof(data2), L"application/octet-stream", L"image/pjpeg"},
    {data3, sizeof(data3), L"application/octet-stream", L"image/pjpeg"},
    {data4, sizeof(data4), L"application/octet-stream", L"image/pjpeg"},
    {data5, sizeof(data5), L"text/plain"},
    {data6, sizeof(data6), L"text/plain"},
    {data7, sizeof(data7), L"text/html", L"text/plain"},
    {data8, sizeof(data8), L"text/html", L"text/plain"},
    {data9, sizeof(data9), L"text/html", L"image/pjpeg"},
    {data10, sizeof(data10), L"text/html", L"text/plain"},
    {data11, sizeof(data11), L"text/html", L"text/plain"},
    {data12, sizeof(data12), L"text/html", L"text/plain"},
    {data13, sizeof(data13), L"text/plain"},
    {data14, sizeof(data14), L"text/plain"},
    {data15, sizeof(data15), L"text/plain"},
    {data16, sizeof(data16), L"image/pjpeg"},
    {data17, sizeof(data17), L"application/octet-stream", L"image/pjpeg"},
    {data18, sizeof(data18), L"text/html", L"image/pjpeg"},
    {data19, sizeof(data19), L"image/gif"},
    {data20, sizeof(data20), L"image/gif"},
    {data21, sizeof(data21), L"text/plain"},
    {data22, sizeof(data22), L"image/gif"},
    {data23, sizeof(data23), L"text/plain"},
    {data24, sizeof(data24), L"image/gif"},
    {data25, sizeof(data25), L"image/gif"},
    {data26, sizeof(data26), L"text/html", L"image/gif"},
    {data27, sizeof(data27), L"text/plain"},
    {data28, sizeof(data28), L"image/bmp"},
    {data29, sizeof(data29), L"image/bmp"},
    {data30, sizeof(data30), L"application/octet-stream", L"image/pjpeg"},
    {data31, sizeof(data31), L"text/html", L"image/bmp"},
    {data32, sizeof(data32), L"application/octet-stream", L"image/pjpeg"},
    {data33, sizeof(data33), L"application/octet-stream", L"image/pjpeg"},
    {data34, sizeof(data34), L"image/x-png"},
    {data35, sizeof(data35), L"image/x-png"},
    {data36, sizeof(data36), L"application/octet-stream", L"image/pjpeg"},
    {data37, sizeof(data37), L"text/html", L"image/x-png"},
    {data38, sizeof(data38), L"application/octet-stream", L"image/pjpeg"},
    {data39, sizeof(data39), L"image/tiff"},
    {data40, sizeof(data40), L"text/html", L"image/tiff"},
    {data41, sizeof(data41), L"text/plain", NULL, L"image/tiff"},
    {data42, sizeof(data42), L"text/plain"},
    {data43, sizeof(data43), L"application/octet-stream", L"image/pjpeg"},
    {data44, sizeof(data44), L"video/avi"},
    {data45, sizeof(data45), L"text/plain"},
    {data46, sizeof(data46), L"text/plain"},
    {data47, sizeof(data47), L"text/plain"},
    {data48, sizeof(data48), L"text/html", L"video/avi"},
    {data49, sizeof(data49), L"video/avi"},
    {data50, sizeof(data50), L"video/mpeg"},
    {data51, sizeof(data51), L"video/mpeg"},
    {data52, sizeof(data52), L"application/octet-stream", L"image/pjpeg"},
    {data53, sizeof(data53), L"application/octet-stream", L"image/pjpeg", L"image/x-icon"},
    {data54, sizeof(data54), L"text/html", L"video/mpeg"},
    {data55, sizeof(data55), L"application/x-gzip-compressed"},
    {data56, sizeof(data56), L"text/plain"},
    {data57, sizeof(data57), L"text/html", L"application/x-gzip-compressed"},
    {data58, sizeof(data58), L"application/octet-stream", L"image/pjpeg"},
    {data59, sizeof(data59), L"application/x-zip-compressed"},
    {data60, sizeof(data60), L"text/plain"},
    {data61, sizeof(data61), L"text/html", L"application/x-zip-compressed"},
    {data62, sizeof(data62), L"application/java"},
    {data63, sizeof(data63), L"text/plain"},
    {data64, sizeof(data64), L"text/html", L"application/java"},
    {data65, sizeof(data65), L"application/pdf"},
    {data66, sizeof(data66), L"text/plain"},
    {data67, sizeof(data67), L"text/html", L"application/pdf"},
    {data68, sizeof(data68), L"application/x-msdownload"},
    {data69, sizeof(data69), L"text/plain"},
    {data70, sizeof(data70), L"text/html", L"application/x-msdownload"},
    {data71, sizeof(data71), L"text/richtext"},
    {data72, sizeof(data72), L"text/plain"},
    {data73, sizeof(data73), L"text/plain"},
    {data74, sizeof(data74), L"text/html", L"text/richtext"},
    {data75, sizeof(data75), L"audio/wav"},
    {data76, sizeof(data76), L"text/plain"},
    {data77, sizeof(data77), L"text/plain"},
    {data78, sizeof(data78), L"text/html", L"text/plain"},
    {data79, sizeof(data79), L"application/postscript"},
    {data80, sizeof(data80), L"text/plain"},
    {data81, sizeof(data81), L"text/html", L"application/postscript"},
    {data82, sizeof(data82), L"audio/basic"},
    {data83, sizeof(data83), L"text/plain"},
    {data84, sizeof(data84), L"text/html", L"audio/basic"},
    {data85, sizeof(data85), L"text/plain"},
    {data86, sizeof(data86), L"image/tiff", NULL, L"text/plain"},
    {data87, sizeof(data87), L"text/plain"},
    {data88, sizeof(data88), L"text/html", L"text/plain"},
    {data89, sizeof(data89), L"text/html", L"text/plain"},
    {data90, sizeof(data90), L"text/html", L"text/plain"},
    {data91, sizeof(data91), L"text/xml", L"text/plain"},
    {data92, sizeof(data92), L"text/xml", L"text/plain"},
    {data93, sizeof(data93), L"text/xml", L"text/plain"},
    {data94, sizeof(data94), L"text/html", L"text/plain"},
    {data95, sizeof(data95), L"text/xml", L"text/richtext"},
    {data96, sizeof(data96), L"text/plain"},
    {data97, sizeof(data97), L"text/html", L"text/plain"},
    {data98, sizeof(data98), L"text/html", L"text/plain"},
    {data1, sizeof(data1), L"text/plain", NULL, NULL, L"res://mshtml.dll/blank.htm"},
    {NULL, 0, L"text/html", NULL, NULL, L"res://mshtml.dll/blank.htm"},
    {data1, sizeof(data1), L"text/plain", NULL, NULL, L"res://mshtml.dll/blank.htm", L"application/octet-stream"},
    {data1, sizeof(data1), L"text/plain", NULL, NULL, L"file:some%20file%2ejpg", L"application/octet-stream"},
    {NULL, sizeof(data1), L"text/html", NULL, NULL, L"res://mshtml.dll/blank.htm"},
    {data1, sizeof(data1), L"text/css", NULL, NULL, L"http://www.winehq.org/test.css"},
    {data2, sizeof(data2), L"text/css", NULL, NULL, L"http://www.winehq.org/test.css"},
    {data10, sizeof(data10), L"text/html", NULL, NULL, L"http://www.winehq.org/test.css"},
    {data1, sizeof(data1), L"text/css", NULL, NULL, L"http://www.winehq.org/test.css", L"text/plain"},
    {data1, sizeof(data1), L"text/css", NULL, NULL, L"http://www.winehq.org/test.css", L"application/octet-stream"},
    {data1, sizeof(data1), L"text/test", NULL, NULL, L"http://www.winehq.org/test.css", L"text/test"}
};

static void test_FindMimeFromData(void)
{
    WCHAR *mime;
    HRESULT hres;
    BYTE b;
    int i;

    static const WCHAR app_octet_streamW[] =
        {'a','p','p','l','i','c','a','t','i','o','n','/','o','c','t','e','t','-','s','t','r','e','a','m',0};
    static const WCHAR image_pjpegW[] = {'i','m','a','g','e','/','p','j','p','e','g',0};
    static const WCHAR text_htmlW[] = {'t','e','x','t','/','h','t','m','l',0};
    static const WCHAR text_plainW[] = {'t','e','x','t','/','p','l','a','i','n',0};

    for(i = 0; i < ARRAY_SIZE(mime_tests); i++) {
        mime = (LPWSTR)0xf0f0f0f0;
        hres = pFindMimeFromData(NULL, mime_tests[i].url, NULL, 0, NULL, 0, &mime, 0);
        if(mime_tests[i].mime) {
            ok(hres == S_OK || broken(mime_tests[i].broken_failure), "[%d] FindMimeFromData failed: %08lx\n", i, hres);
            if(hres == S_OK) {
                ok(!lstrcmpW(mime, mime_tests[i].mime)
                   || broken(mime_tests[i].broken_mime && !lstrcmpW(mime, mime_tests[i].broken_mime)),
                   "[%d] wrong mime: %s\n", i, wine_dbgstr_w(mime));
                CoTaskMemFree(mime);
            }
        }else {
            ok(hres == E_FAIL || hres == mime_tests[i].hres,
               "[%d] FindMimeFromData failed: %08lx, expected %08lx\n",
               i, hres, mime_tests[i].hres);
            ok(mime == (LPWSTR)0xf0f0f0f0, "[%d] mime != 0xf0f0f0f0\n", i);
        }

        mime = (LPWSTR)0xf0f0f0f0;
        hres = pFindMimeFromData(NULL, mime_tests[i].url, NULL, 0, text_plainW, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08lx\n", i, hres);
        ok(!lstrcmpW(mime, L"text/plain"), "[%d] wrong mime: %s\n", i, wine_dbgstr_w(mime));
        CoTaskMemFree(mime);

        mime = (LPWSTR)0xf0f0f0f0;
        hres = pFindMimeFromData(NULL, mime_tests[i].url, NULL, 0, app_octet_streamW, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08lx\n", i, hres);
        ok(!lstrcmpW(mime, L"application/octet-stream"), "[%d] wrong mime: %s\n", i, wine_dbgstr_w(mime));
        CoTaskMemFree(mime);
    }

    for(i = 0; i < ARRAY_SIZE(mime_tests2); i++) {
        hres = pFindMimeFromData(NULL, mime_tests2[i].url, mime_tests2[i].data, mime_tests2[i].size,
                mime_tests2[i].proposed_mime, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08lx\n", i, hres);
        b = !lstrcmpW(mime, mime_tests2[i].mime);
        ok(b || broken(mime_tests2[i].broken_mime && !lstrcmpW(mime, mime_tests2[i].broken_mime)),
            "[%d] wrong mime: %s\n", i, wine_dbgstr_w(mime));
        CoTaskMemFree(mime);
        if(!b || mime_tests2[i].url || mime_tests2[i].proposed_mime)
            continue;

        hres = pFindMimeFromData(NULL, NULL, mime_tests2[i].data, mime_tests2[i].size,
                app_octet_streamW, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08lx\n", i, hres);
        ok(!lstrcmpW(mime, mime_tests2[i].mime) || broken(mime_tests2[i].broken_mime
                        && !lstrcmpW(mime, mime_tests2[i].broken_mime)),
                    "[%d] wrong mime: %s\n", i, wine_dbgstr_w(mime));
        CoTaskMemFree(mime);

        hres = pFindMimeFromData(NULL, NULL, mime_tests2[i].data, mime_tests2[i].size,
                text_plainW, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08lx\n", i, hres);
        ok(!lstrcmpW(mime, mime_tests2[i].mime) || broken(mime_tests2[i].broken_mime
                    && !lstrcmpW(mime, mime_tests2[i].broken_mime)),
                "[%d] wrong mime: %s\n", i, wine_dbgstr_w(mime));
        CoTaskMemFree(mime);

        hres = pFindMimeFromData(NULL, NULL, mime_tests2[i].data, mime_tests2[i].size,
                text_htmlW, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08lx\n", i, hres);
        if(!lstrcmpW(L"application/octet-stream", mime_tests2[i].mime)
           || !lstrcmpW(L"text/plain", mime_tests2[i].mime) || i==92)
            ok(!lstrcmpW(mime, L"text/html"), "[%d] wrong mime: %s\n", i, wine_dbgstr_w(mime));
        else
            ok(!lstrcmpW(mime, mime_tests2[i].mime), "[%d] wrong mime: %s\n", i, wine_dbgstr_w(mime));
        CoTaskMemFree(mime);

        hres = pFindMimeFromData(NULL, NULL, mime_tests2[i].data, mime_tests2[i].size,
                image_pjpegW, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08lx\n", i, hres);
        ok(!lstrcmpW(mime, mime_tests2[i].mime_pjpeg ? mime_tests2[i].mime_pjpeg : mime_tests2[i].mime)
           || broken(!lstrcmpW(mime, mime_tests2[i].mime)),
           "[%d] wrong mime, got %s\n", i, wine_dbgstr_w(mime));
        CoTaskMemFree(mime);
    }

    hres = pFindMimeFromData(NULL, NULL, NULL, 0, NULL, 0, &mime, 0);
    ok(hres == E_INVALIDARG, "FindMimeFromData failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = pFindMimeFromData(NULL, NULL, NULL, 0, text_plainW, 0, &mime, 0);
    ok(hres == E_INVALIDARG, "FindMimeFromData failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = pFindMimeFromData(NULL, NULL, data1, 0, NULL, 0, &mime, 0);
    ok(hres == E_FAIL, "FindMimeFromData failed: %08lx, expected E_FAIL\n", hres);

    hres = pFindMimeFromData(NULL, url1, data1, 0, NULL, 0, &mime, 0);
    ok(hres == E_FAIL, "FindMimeFromData failed: %08lx, expected E_FAIL\n", hres);

    hres = pFindMimeFromData(NULL, NULL, data1, 0, text_plainW, 0, &mime, 0);
    ok(hres == S_OK, "FindMimeFromData failed: %08lx\n", hres);
    ok(!lstrcmpW(mime, L"text/plain"), "wrong mime: %s\n", wine_dbgstr_w(mime));
    CoTaskMemFree(mime);

    hres = pFindMimeFromData(NULL, NULL, data1, 0, text_plainW, 0, NULL, 0);
    ok(hres == E_INVALIDARG, "FindMimeFromData failed: %08lx, expected E_INVALIDARG\n", hres);
}

static void register_protocols(void)
{
    IInternetSession *session;
    IClassFactory *factory;
    HRESULT hres;

    static const WCHAR wszAbout[] = {'a','b','o','u','t',0};

    hres = pCoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = CoGetClassObject(&CLSID_AboutProtocol, CLSCTX_INPROC_SERVER, NULL,
            &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get AboutProtocol factory: %08lx\n", hres);
    if(FAILED(hres))
        return;

    IInternetSession_RegisterNameSpace(session, factory, &CLSID_AboutProtocol,
                                       wszAbout, 0, NULL, 0);
    IClassFactory_Release(factory);

    IInternetSession_Release(session);
}

static HRESULT WINAPI InternetProtocolInfo_QueryInterface(IInternetProtocolInfo *iface,
                                                          REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InternetProtocolInfo_AddRef(IInternetProtocolInfo *iface)
{
    return 2;
}

static ULONG WINAPI InternetProtocolInfo_Release(IInternetProtocolInfo *iface)
{
    return 1;
}

static HRESULT WINAPI InternetProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD *pcchResult, DWORD dwReserved)
{
    switch(ParseAction) {
    case PARSE_SECURITY_URL:
        CHECK_EXPECT2(ParseUrl);
        if(pcchResult)
            *pcchResult = ARRAY_SIZE(url1);

        if(cchResult < ARRAY_SIZE(url1))
            return S_FALSE;

        memcpy(pwzResult, url1, sizeof(url1));
        return S_OK;
    case PARSE_ENCODE:
        CHECK_EXPECT2(ParseUrl_ENCODE);
        break;

    case PARSE_UNESCAPE:
        CHECK_EXPECT2(ParseUrl_UNESCAPE);
        break;

    default:
        CHECK_EXPECT2(ParseUrl);
        break;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocolInfo_CombineUrl(IInternetProtocolInfo *iface,
        LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags,
        LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocolInfo_CompareUrl(IInternetProtocolInfo *iface,
        LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocolInfo_QueryInfo(IInternetProtocolInfo *iface,
        LPCWSTR pwzUrl, QUERYOPTION OueryOption, DWORD dwQueryFlags, LPVOID pBuffer,
        DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IInternetProtocolInfoVtbl InternetProtocolInfoVtbl = {
    InternetProtocolInfo_QueryInterface,
    InternetProtocolInfo_AddRef,
    InternetProtocolInfo_Release,
    InternetProtocolInfo_ParseUrl,
    InternetProtocolInfo_CombineUrl,
    InternetProtocolInfo_CompareUrl,
    InternetProtocolInfo_QueryInfo
};

static IInternetProtocolInfo protocol_info = { &InternetProtocolInfoVtbl };

static HRESULT qiret;
static IClassFactory *expect_cf;

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IInternetProtocolInfo, riid)) {
        CHECK_EXPECT2(QI_IInternetProtocolInfo);
        ok(iface == expect_cf, "unexpected iface\n");
        *ppv = &protocol_info;
        return qiret;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                        REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                        REFIID riid, void **ppv)
{
    CHECK_EXPECT(CreateInstance);

    ok(iface == expect_cf, "unexpected iface\n");
    ok(pOuter == NULL, "pOuter = %p\n", pOuter);
    ok(IsEqualGUID(&IID_IInternetProtocolInfo, riid), "unexpected riid\n");
    ok(ppv != NULL, "ppv == NULL\n");

    *ppv = &protocol_info;
    return S_OK;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static const IClassFactoryVtbl ProtocolCFVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ProtocolCF_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory test_protocol_cf = { &ProtocolCFVtbl };
static IClassFactory test_protocol_cf2 = { &ProtocolCFVtbl };
static IClassFactory test_cf = { &ClassFactoryVtbl };

static void test_NameSpace(void)
{
    IInternetSession *session;
    WCHAR buf[200];
    LPWSTR sec_url;
    DWORD size;
    HRESULT hres;

    static const WCHAR wszTest[] = {'t','e','s','t',0};

    hres = pCoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetSession_RegisterNameSpace(session, NULL, &IID_NULL,
                                              wszTest, 0, NULL, 0);
    ok(hres == E_INVALIDARG, "RegisterNameSpace failed: %08lx\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &test_protocol_cf, &IID_NULL,
                                              NULL, 0, NULL, 0);
    ok(hres == E_INVALIDARG, "RegisterNameSpace failed: %08lx\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &test_protocol_cf, &IID_NULL,
                                              wszTest, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08lx\n", hres);

    qiret = E_NOINTERFACE;
    expect_cf = &test_protocol_cf;
    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(ParseUrl_ENCODE);

    hres = pCoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, ARRAY_SIZE(buf), &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08lx\n", hres);

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(ParseUrl_ENCODE);

    qiret = S_OK;
    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(ParseUrl_ENCODE);

    hres = pCoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, ARRAY_SIZE(buf), &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08lx\n", hres);

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(ParseUrl_ENCODE);

    qiret = S_OK;
    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(ParseUrl_UNESCAPE);

    hres = pCoInternetParseUrl(url8, PARSE_UNESCAPE, 0, buf, ARRAY_SIZE(buf), &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08lx\n", hres);

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(ParseUrl_UNESCAPE);

    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(ParseUrl);

    hres = pCoInternetParseUrl(url8, PARSE_SECURITY_URL, 0, buf, ARRAY_SIZE(buf), &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08lx\n", hres);
    ok(size == ARRAY_SIZE(url1), "Size = %ld\n", size);
    if(size == ARRAY_SIZE(url1))
        ok(!memcmp(buf, url1, sizeof(url1)), "Encoded url = %s\n", wine_dbgstr_w(buf));

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(ParseUrl);

    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(ParseUrl);

    if (pCoInternetGetSecurityUrl) {
        hres = pCoInternetGetSecurityUrl(url8, &sec_url, PSU_SECURITY_URL_ONLY, 0);
        ok(hres == S_OK, "CoInternetGetSecurityUrl failed: %08lx\n", hres);
        if(hres == S_OK) {
            ok(lstrlenW(sec_url) > ARRAY_SIZE(wszFile) &&
                    !memcmp(sec_url, wszFile, sizeof(wszFile)-sizeof(WCHAR)),
                    "Encoded url = %s\n", wine_dbgstr_w(sec_url));
            CoTaskMemFree(sec_url);
        }

        CHECK_CALLED(QI_IInternetProtocolInfo);
        CHECK_CALLED(ParseUrl);
    }

    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08lx\n", hres);

    hres = pCoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, ARRAY_SIZE(buf), &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08lx\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &test_protocol_cf2, &IID_NULL,
                                              wszTest, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08lx\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &test_protocol_cf, &IID_NULL,
                                              wszTest, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08lx\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &test_protocol_cf, &IID_NULL,
                                              wszTest, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08lx\n", hres);

    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(ParseUrl_ENCODE);

    hres = pCoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, ARRAY_SIZE(buf), &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08lx\n", hres);

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(ParseUrl_ENCODE);

    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08lx\n", hres);

    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(ParseUrl_ENCODE);

    hres = pCoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, ARRAY_SIZE(buf), &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08lx\n", hres);

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(ParseUrl_ENCODE);

    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08lx\n", hres);

    expect_cf = &test_protocol_cf2;
    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(ParseUrl_ENCODE);

    hres = pCoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, ARRAY_SIZE(buf), &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08lx\n", hres);

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(ParseUrl_ENCODE);

    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08lx\n", hres);
    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08lx\n", hres);
    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, NULL);
    ok(hres == E_INVALIDARG, "UnregisterNameSpace failed: %08lx\n", hres);
    hres = IInternetSession_UnregisterNameSpace(session, NULL, wszTest);
    ok(hres == E_INVALIDARG, "UnregisterNameSpace failed: %08lx\n", hres);

    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf2, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08lx\n", hres);

    hres = pCoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, ARRAY_SIZE(buf), &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08lx\n", hres);

    IInternetSession_Release(session);
}

static void test_MimeFilter(void)
{
    IInternetSession *session;
    HRESULT hres;

    static const WCHAR mimeW[] = {'t','e','s','t','/','m','i','m','e',0};

    hres = pCoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetSession_RegisterMimeFilter(session, &test_cf, &IID_NULL, mimeW);
    ok(hres == S_OK, "RegisterMimeFilter failed: %08lx\n", hres);

    hres = IInternetSession_UnregisterMimeFilter(session, &test_cf, mimeW);
    ok(hres == S_OK, "UnregisterMimeFilter failed: %08lx\n", hres);

    hres = IInternetSession_UnregisterMimeFilter(session, &test_cf, mimeW);
    ok(hres == S_OK, "UnregisterMimeFilter failed: %08lx\n", hres);

    hres = IInternetSession_UnregisterMimeFilter(session, (void*)0xdeadbeef, mimeW);
    ok(hres == S_OK, "UnregisterMimeFilter failed: %08lx\n", hres);

    IInternetSession_Release(session);
}

static ULONG WINAPI unk_Release(IUnknown *iface)
{
    CHECK_EXPECT(unk_Release);
    return 0;
}

static const IUnknownVtbl unk_vtbl = {
    (void*)0xdeadbeef,
    (void*)0xdeadbeef,
    unk_Release
};

static void test_ReleaseBindInfo(void)
{
    BINDINFO bi;
    IUnknown unk = { &unk_vtbl };

    pReleaseBindInfo(NULL); /* shouldn't crash */

    memset(&bi, 0, sizeof(bi));
    bi.cbSize = sizeof(BINDINFO);
    bi.pUnk = &unk;
    SET_EXPECT(unk_Release);
    pReleaseBindInfo(&bi);
    ok(bi.cbSize == sizeof(BINDINFO), "bi.cbSize=%ld\n", bi.cbSize);
    ok(bi.pUnk == NULL, "bi.pUnk=%p, expected NULL\n", bi.pUnk);
    CHECK_CALLED(unk_Release);

    memset(&bi, 0, sizeof(bi));
    bi.cbSize = offsetof(BINDINFO, pUnk);
    bi.pUnk = &unk;
    pReleaseBindInfo(&bi);
    ok(bi.cbSize == offsetof(BINDINFO, pUnk), "bi.cbSize=%ld\n", bi.cbSize);
    ok(bi.pUnk == &unk, "bi.pUnk=%p, expected %p\n", bi.pUnk, &unk);

    memset(&bi, 0, sizeof(bi));
    bi.pUnk = &unk;
    pReleaseBindInfo(&bi);
    ok(!bi.cbSize, "bi.cbSize=%ld, expected 0\n", bi.cbSize);
    ok(bi.pUnk == &unk, "bi.pUnk=%p, expected %p\n", bi.pUnk, &unk);
}

static void test_CopyStgMedium(void)
{
    STGMEDIUM src, dst;
    HGLOBAL empty, hg;
    char *ptr1, *ptr2;
    HRESULT hres;
    int size;

    static WCHAR fileW[] = {'f','i','l','e',0};

    memset(&src, 0xf0, sizeof(src));
    memset(&dst, 0xe0, sizeof(dst));
    memset(&empty, 0xf0, sizeof(empty));
    src.tymed = TYMED_NULL;
    src.pUnkForRelease = NULL;
    hres = pCopyStgMedium(&src, &dst);
    ok(hres == S_OK, "CopyStgMedium failed: %08lx\n", hres);
    ok(dst.tymed == TYMED_NULL, "tymed=%ld\n", dst.tymed);
    ok(dst.hGlobal == empty, "u=%p\n", dst.hGlobal);
    ok(!dst.pUnkForRelease, "pUnkForRelease=%p, expected NULL\n", dst.pUnkForRelease);

    memset(&dst, 0xe0, sizeof(dst));
    src.tymed = TYMED_ISTREAM;
    src.pstm = NULL;
    src.pUnkForRelease = NULL;
    hres = pCopyStgMedium(&src, &dst);
    ok(hres == S_OK, "CopyStgMedium failed: %08lx\n", hres);
    ok(dst.tymed == TYMED_ISTREAM, "tymed=%ld\n", dst.tymed);
    ok(!dst.pstm, "pstm=%p\n", dst.pstm);
    ok(!dst.pUnkForRelease, "pUnkForRelease=%p, expected NULL\n", dst.pUnkForRelease);

    memset(&dst, 0xe0, sizeof(dst));
    src.tymed = TYMED_FILE;
    src.lpszFileName = fileW;
    src.pUnkForRelease = NULL;
    hres = pCopyStgMedium(&src, &dst);
    ok(hres == S_OK, "CopyStgMedium failed: %08lx\n", hres);
    ok(dst.tymed == TYMED_FILE, "tymed=%ld\n", dst.tymed);
    ok(dst.lpszFileName && dst.lpszFileName != fileW, "lpszFileName=%p\n", dst.lpszFileName);
    ok(!lstrcmpW(dst.lpszFileName, fileW), "wrong file name\n");
    ok(!dst.pUnkForRelease, "pUnkForRelease=%p, expected NULL\n", dst.pUnkForRelease);
    ReleaseStgMedium(&dst);

    /* TYMED_HGLOBAL */
    hg = GlobalAlloc(GMEM_MOVEABLE, 10);
    ptr1 = GlobalLock(hg);
    memset(ptr1, 0xfa, 10);
    memset(&dst, 0xe0, sizeof(dst));
    src.tymed = TYMED_HGLOBAL;
    src.hGlobal = hg;
    hres = pCopyStgMedium(&src, &dst);
    ok(hres == S_OK, "CopyStgMedium failed: %08lx\n", hres);
    ok(dst.tymed == TYMED_HGLOBAL, "tymed=%ld\n", dst.tymed);
    ok(dst.hGlobal != hg, "got %p, %p\n", dst.hGlobal, hg);
    size = GlobalSize(dst.hGlobal);
    ok(size == 10, "got size %d\n", size);
    /* compare contents */
    ptr2 = GlobalLock(dst.hGlobal);
    ok(!memcmp(ptr1, ptr2, 10), "got wrong data\n");
    GlobalUnlock(ptr2);
    GlobalUnlock(ptr1);
    ok(GlobalFlags(dst.hGlobal) == 0, "got 0x%08x\n", GlobalFlags(dst.hGlobal));
    GlobalFree(hg);
    ReleaseStgMedium(&dst);

    memset(&dst, 0xe0, sizeof(dst));
    src.tymed = TYMED_HGLOBAL;
    src.hGlobal = NULL;
    hres = pCopyStgMedium(&src, &dst);
    ok(hres == S_OK, "CopyStgMedium failed: %08lx\n", hres);
    ok(dst.hGlobal == NULL, "got %p\n", dst.hGlobal);

    hres = pCopyStgMedium(&src, NULL);
    ok(hres == E_POINTER, "CopyStgMedium failed: %08lx, expected E_POINTER\n", hres);
    hres = pCopyStgMedium(NULL, &dst);
    ok(hres == E_POINTER, "CopyStgMedium failed: %08lx, expected E_POINTER\n", hres);
}

static void test_CopyBindInfo(void)
{
    BINDINFO src[2], dest[2];
    SECURITY_DESCRIPTOR sec_desc;
    HRESULT hres;
    int i;

    hres = pCopyBindInfo(NULL, NULL);
    ok(hres == E_POINTER, "CopyBindInfo returned %08lx, expected E_POINTER\n", hres);

    memset(src, 0, sizeof(BINDINFO[2]));
    memset(dest, 0xde, sizeof(BINDINFO[2]));
    hres = pCopyBindInfo(src, dest);
    ok(hres == E_INVALIDARG, "CopyBindInfo returned: %08lx, expected E_INVALIDARG\n", hres);

    memset(src, 0, sizeof(BINDINFO[2]));
    memset(dest, 0xde, sizeof(BINDINFO[2]));
    src[0].cbSize = sizeof(BINDINFO);
    dest[0].cbSize = 0;
    hres = pCopyBindInfo(src, dest);
    ok(hres == E_INVALIDARG, "CopyBindInfo returned: %08lx, expected E_INVALIDARG\n", hres);

    memset(src, 0, sizeof(BINDINFO[2]));
    memset(dest, 0xde, sizeof(BINDINFO[2]));
    src[0].cbSize = 1;
    dest[0].cbSize = sizeof(BINDINFO)+sizeof(DWORD);
    hres = pCopyBindInfo(src, dest);
    ok(hres == S_OK, "CopyBindInfo failed: %08lx\n", hres);
    ok(dest[0].cbSize == sizeof(BINDINFO)+sizeof(DWORD), "incorrect cbSize: %ld\n", dest[0].cbSize);
    for(i=1; i<dest[0].cbSize/sizeof(int); i++)
        ok(((int*)dest)[i] == 0, "unset values should be set to 0, got %d on %d\n", ((int*)dest)[i], i);

    memset(src, 0, sizeof(BINDINFO[2]));
    memset(dest, 0xde, sizeof(BINDINFO[2]));
    src[0].cbSize = sizeof(BINDINFO)+2*sizeof(DWORD);
    dest[0].cbSize = sizeof(BINDINFO)+sizeof(DWORD);
    hres = pCopyBindInfo(src, dest);
    ok(hres == S_OK, "CopyBindInfo failed: %08lx\n", hres);
    ok(dest[1].cbSize == src[1].cbSize, "additional data should be copied\n");
    ok(dest[1].szExtraInfo != src[1].szExtraInfo,
            "data not fitting in destination buffer should not be copied\n");

    memset(src, 0xf0, sizeof(BINDINFO[2]));
    memset(dest, 0xde, sizeof(BINDINFO[2]));
    src[0].cbSize = sizeof(BINDINFO);
    src[0].szExtraInfo = CoTaskMemAlloc(sizeof(WCHAR));
    src[0].szExtraInfo[0] = 0;
    src[0].szCustomVerb = NULL;
    src[0].pUnk = NULL;
    src[0].stgmedData.tymed = TYMED_NULL;
    src[0].stgmedData.pUnkForRelease = NULL;
    dest[0].cbSize = sizeof(BINDINFO);
    hres = pCopyBindInfo(src, dest);
    ok(hres == S_OK, "CopyBindInfo failed: %08lx\n", hres);

    ok(dest[0].cbSize == sizeof(BINDINFO), "incorrect cbSize: %ld\n", dest[0].cbSize);
    ok(dest[0].szExtraInfo && !dest[0].szExtraInfo[0] && dest[0].szExtraInfo!=src[0].szExtraInfo,
            "incorrect szExtraInfo: (%p!=%p) %d\n", dest[0].szExtraInfo,
            src[0].szExtraInfo, dest[0].szExtraInfo[0]);
    ok(!memcmp(&dest[0].stgmedData, &src[0].stgmedData, sizeof(STGMEDIUM)),
            "incorrect stgmedData value\n");
    ok(src[0].grfBindInfoF == dest[0].grfBindInfoF, "grfBindInfoF = %lx, expected %lx\n",
            dest[0].grfBindInfoF, src[0].grfBindInfoF);
    ok(src[0].dwBindVerb == dest[0].dwBindVerb, "dwBindVerb = %lx, expected %lx\n",
            dest[0].dwBindVerb, src[0].dwBindVerb);
    ok(!dest[0].szCustomVerb, "szCustmoVerb != NULL\n");
    ok(src[0].cbstgmedData == dest[0].cbstgmedData, "cbstgmedData = %lx, expected %lx\n",
            dest[0].cbstgmedData, src[0].cbstgmedData);
    ok(src[0].dwOptions == dest[0].dwOptions, "dwOptions = %lx, expected %lx\n",
            dest[0].dwOptions, src[0].dwOptions);
    ok(src[0].dwOptionsFlags == dest[0].dwOptionsFlags, "dwOptionsFlags = %lx, expected %lx\n",
            dest[0].dwOptionsFlags, src[0].dwOptionsFlags);
    ok(src[0].dwCodePage == dest[0].dwCodePage, "dwCodePage = %lx, expected %lx\n",
            dest[0].dwCodePage, src[0].dwCodePage);
    ok(!dest[0].securityAttributes.nLength,
            "unexpected securityAttributes.nLength value: %ld\n",
            dest[0].securityAttributes.nLength);
    ok(!dest[0].securityAttributes.lpSecurityDescriptor,
            "unexpected securityAttributes.lpSecurityDescriptor value: %p\n",
            dest[0].securityAttributes.lpSecurityDescriptor);
    ok(!dest[0].securityAttributes.bInheritHandle,
            "unexpected securityAttributes.bInheritHandle value: %d\n",
            dest[0].securityAttributes.bInheritHandle);
    ok(!memcmp(&dest[0].iid, &src[0].iid, sizeof(IID)),
            "incorrect iid value\n");
    ok(!dest[0].pUnk, "pUnk != NULL\n");
    ok(src[0].dwReserved == dest[0].dwReserved, "dwReserved = %lx, expected %lx\n",
            dest[0].dwReserved, src[0].dwReserved);

    CoTaskMemFree(src[0].szExtraInfo);
    CoTaskMemFree(dest[0].szExtraInfo);

    src[0].szExtraInfo = NULL;
    src[0].securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    ok(InitializeSecurityDescriptor(&sec_desc, SECURITY_DESCRIPTOR_REVISION),
            "InitializeSecurityDescriptor failed\n");
    src[0].securityAttributes.lpSecurityDescriptor = (void*)&sec_desc;
    src[0].securityAttributes.bInheritHandle = TRUE;
    hres = pCopyBindInfo(src, dest);
    ok(hres == S_OK, "CopyBindInfo failed: %08lx\n", hres);
    ok(!dest[0].securityAttributes.nLength,
            "unexpected securityAttributes.nLength value: %ld\n",
            dest[0].securityAttributes.nLength);
    ok(!dest[0].securityAttributes.lpSecurityDescriptor,
            "unexpected securityAttributes.lpSecurityDescriptor value: %p\n",
            dest[0].securityAttributes.lpSecurityDescriptor);
    ok(!dest[0].securityAttributes.bInheritHandle,
            "unexpected securityAttributes.bInheritHandle value: %d\n",
            dest[0].securityAttributes.bInheritHandle);
}

static void test_UrlMkGetSessionOption(void)
{
    DWORD encoding, size;
    HRESULT hres;

    size = encoding = 0xdeadbeef;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, &encoding,
                                 sizeof(encoding), &size, 0);
    ok(hres == S_OK, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(encoding != 0xdeadbeef, "encoding not changed\n");
    ok(size == sizeof(encoding), "size=%ld\n", size);

    size = encoding = 0xdeadbeef;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, &encoding,
                                 sizeof(encoding)+1, &size, 0);
    ok(hres == S_OK, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(encoding != 0xdeadbeef, "encoding not changed\n");
    ok(size == sizeof(encoding), "size=%ld\n", size);

    size = encoding = 0xdeadbeef;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, &encoding,
                                 sizeof(encoding)-1, &size, 0);
    ok(hres == E_INVALIDARG, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(encoding == 0xdeadbeef, "encoding = %08lx, exepcted 0xdeadbeef\n", encoding);
    ok(size == 0xdeadbeef, "size=%ld\n", size);

    size = encoding = 0xdeadbeef;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, NULL,
                                 sizeof(encoding)-1, &size, 0);
    ok(hres == E_INVALIDARG, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(encoding == 0xdeadbeef, "encoding = %08lx, exepcted 0xdeadbeef\n", encoding);
    ok(size == 0xdeadbeef, "size=%ld\n", size);

    encoding = 0xdeadbeef;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, &encoding,
                                 sizeof(encoding)-1, NULL, 0);
    ok(hres == E_INVALIDARG, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(encoding == 0xdeadbeef, "encoding = %08lx, exepcted 0xdeadbeef\n", encoding);
}

static size_t check_prefix(const char *str, const char *prefix)
{
    size_t len = strlen(prefix);
    if(memcmp(str, prefix, len)) {
        ok(0, "no %s prefix in %s\n", wine_dbgstr_a(prefix), wine_dbgstr_a(str));
        return 0;
    }
    return len;
}

static void test_user_agent(void)
{
    static const CHAR expected[] = "Mozilla/4.0 (compatible; MSIE ";
    static char test_str[] = "test";
    static char test2_str[] = "test\0test";
    static CHAR str[3];
    OSVERSIONINFOW os_info = {sizeof(os_info)};
    char ua[1024], buf[64];
    unsigned int i;
    LPSTR str2 = NULL;
    BOOL is_wow = FALSE;
    HRESULT hres;
    DWORD size, saved;

    hres = pObtainUserAgentString(0, NULL, NULL);
    ok(hres == E_INVALIDARG, "ObtainUserAgentString failed: %08lx\n", hres);

    size = 100;
    hres = pObtainUserAgentString(0, NULL, &size);
    ok(hres == E_INVALIDARG, "ObtainUserAgentString failed: %08lx\n", hres);
    ok(size == 100, "size=%ld, expected %d\n", size, 100);

    size = 0;
    hres = pObtainUserAgentString(0, str, &size);
    ok(hres == E_OUTOFMEMORY, "ObtainUserAgentString failed: %08lx\n", hres);
    ok(size > 0, "size=%ld, expected non-zero\n", size);

    size = 2;
    str[0] = 'a';
    hres = pObtainUserAgentString(0, str, &size);
    ok(hres == E_OUTOFMEMORY, "ObtainUserAgentString failed: %08lx\n", hres);
    ok(size > 0, "size=%ld, expected non-zero\n", size);
    ok(str[0] == 'a', "str[0]=%c, expected 'a'\n", str[0]);

    size = 0;
    hres = pObtainUserAgentString(1, str, &size);
    ok(hres == E_OUTOFMEMORY, "ObtainUserAgentString failed: %08lx\n", hres);
    ok(size > 0, "size=%ld, expected non-zero\n", size);

    str2 = malloc(size + 20);
    saved = size;
    hres = pObtainUserAgentString(0, str2, &size);
    ok(hres == S_OK, "ObtainUserAgentString failed: %08lx\n", hres);
    ok(size == saved, "size=%ld, expected %ld\n", size, saved);
    ok(strlen(expected) <= strlen(str2) &&
       !memcmp(expected, str2, strlen(expected)*sizeof(CHAR)),
       "user agent was \"%s\", expected to start with \"%s\"\n",
       str2, expected);

    GetVersionExW(&os_info);
    if (sizeof(void*) == 4) IsWow64Process(GetCurrentProcess(), &is_wow);

    for(i = 1; i < 12; i++) {
        const char *p = ua;

        if (i != 7) {
            size = sizeof(ua);
            hres = pObtainUserAgentString(i | UAS_EXACTLEGACY, ua, &size);
            ok(hres == S_OK, "ObtainUserAgentString failed: %08lx\n", hres);
            ok(size == strlen(ua) + 1, "unexpected size %lu, expected %Iu\n", size, strlen(ua) + 1);
            ok(!strcmp(ua, str2), "unexpected UA for version %u %s, expected %s\n",
               i, wine_dbgstr_a(ua), wine_dbgstr_a(str2));
        }

        size = sizeof(ua);
        hres = pObtainUserAgentString(i != 1 ? i : UAS_EXACTLEGACY | 7, ua, &size);
        ok(hres == S_OK, "ObtainUserAgentString failed: %08lx\n", hres);
        ok(size == strlen(ua) + 1, "unexpected size %lu, expected %Iu\n", size, strlen(ua) + 1);
        if(i < 8 && i != 1)
            ok(!strcmp(ua, str2), "unexpected UA for version %u %s, expected %s\n",
               i, wine_dbgstr_a(ua), wine_dbgstr_a(str2));
        if (winetest_debug > 1)
            trace("version=%u user-agent=%s\n", i, wine_dbgstr_a(ua));

        p += check_prefix(p, "Mozilla/");
        p += check_prefix(p, i < 9 ? "4.0 (" : "5.0 (");
        if(i < 11) {
            p += check_prefix(p, "compatible; ");
            sprintf(buf, "MSIE %u.0; ", max(i, 7));
            p += check_prefix(p, buf);
        }
        sprintf(buf, "Windows NT %lu.%lu; ", os_info.dwMajorVersion, os_info.dwMinorVersion);
        p += check_prefix(p, buf);
        if(is_wow) {
            p += check_prefix(p, "WOW64; ");
        }else if(sizeof(void*) == 8) {
            p += check_prefix(p, "Win64; ");
#ifdef __x86_64__
            todo_wine
            p += check_prefix(p, "x64; ");
#endif
        }
        if(i != 1) {
            p += check_prefix(p, "Trident/");
            ok('5' <= *p && *p <= '8', "unexpected version '%c'\n", *p);
            if(*p < '7') {
                win_skip("skipping UA tests, too old IE\n");
                break;
            }
            p++; /* skip version number */
            p += check_prefix(p, ".0");
        }
        if(i == 11) {
            p += check_prefix(p, "; rv:11.0) like Gecko");
        }else if (i >= 9) {
            p += check_prefix(p, ")");
        }else {
#ifdef __x86_64__
            todo_wine
#endif
            /* This assumes that either p points at some property before
             * '; .NET', or that there is more than one such occurrence.
             */
            ok(strstr(p, "; .NET") != NULL, "no '; .NET' in %s\n", wine_dbgstr_a(p));
            p = strchr(p, ')');
            p += check_prefix(p, ")");
        }

        ok(!*p, "unexpected suffix %s for version %u\n", wine_dbgstr_a(p), i);
    }

    size = saved+10;
    hres = pObtainUserAgentString(0, str2, &size);
    ok(hres == S_OK, "ObtainUserAgentString failed: %08lx\n", hres);
    ok(size == saved, "size=%ld, expected %ld\n", size, saved);

    size = 0;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_USERAGENT, NULL, 0, &size, 0);
    ok(hres == E_OUTOFMEMORY, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(size, "size == 0\n");

    size = 0xdeadbeef;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_USERAGENT, NULL, 1000, &size, 0);
    ok(hres == E_INVALIDARG, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(size, "size == 0\n");

    saved = size;
    size = 0;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_USERAGENT, str2, saved+10, &size, 0);
    ok(hres == E_OUTOFMEMORY, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(size == saved, "size = %ld, expected %ld\n", size, saved);
    ok(sizeof(expected) <= strlen(str2) && !memcmp(expected, str2, sizeof(expected)-1),
       "user agent was \"%s\", expected to start with \"%s\"\n",
       str2, expected);

    size = 0;
    str2[0] = 0;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_USERAGENT, str2, saved, &size, 0);
    ok(hres == E_OUTOFMEMORY, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(size == saved, "size = %ld, expected %ld\n", size, saved);
    ok(sizeof(expected) <= strlen(str2) && !memcmp(expected, str2, sizeof(expected)-1),
       "user agent was \"%s\", expected to start with \"%s\"\n",
       str2, expected);

    size = saved;
    str2[0] = 0;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_USERAGENT, str2, saved-1, &size, 0);
    ok(hres == E_OUTOFMEMORY, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(size == saved, "size = %ld, expected %ld\n", size, saved);
    ok(!str2[0], "buf changed\n");

    size = saved;
    str2[0] = 0;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_USERAGENT, str2, saved, NULL, 0);
    ok(hres == E_INVALIDARG, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(!str2[0], "buf changed\n");

    hres = UrlMkSetSessionOption(URLMON_OPTION_USERAGENT, test_str, sizeof(test_str), 0);
    ok(hres == S_OK, "UrlMkSetSessionOption failed: %08lx\n", hres);

    size = 0;
    str2[0] = 0;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_USERAGENT, str2, saved, &size, 0);
    ok(hres == E_OUTOFMEMORY, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(size == sizeof(test_str) && !memcmp(str2, test_str, sizeof(test_str)), "wrong user agent\n");

    for(i = 0; i < 12; i++) {
        size = sizeof(ua);
        hres = pObtainUserAgentString(i, ua, &size);
        ok(hres == S_OK, "[%u] ObtainUserAgentString failed: %08lx\n", i, hres);
        ok(size == strlen(ua) + 1, "[%u] unexpected size %lu, expected %Iu\n", i, size, strlen(ua) + 1);
        if(i >= 7)
            ok(strcmp(ua, test_str), "[%u] unexpected UA, did not expect \"test\"\n", i);
        else
            ok(!strcmp(ua, test_str), "[%u] unexpected UA %s, expected \"test\"\n", i, wine_dbgstr_a(ua));

        size = sizeof(ua);
        hres = pObtainUserAgentString(i | UAS_EXACTLEGACY, ua, &size);
        ok(hres == S_OK, "[%u] ObtainUserAgentString failed: %08lx\n", i, hres);
        ok(size == strlen(ua) + 1, "[%u] unexpected size %lu, expected %Iu\n", i, size, strlen(ua) + 1);
        if(i == 7)
            ok(strcmp(ua, test_str), "[%u] unexpected UA, did not expect \"test\"\n", i);
        else
            ok(!strcmp(ua, test_str), "[%u] unexpected UA %s, expected \"test\"\n", i, wine_dbgstr_a(ua));
    }

    hres = UrlMkSetSessionOption(URLMON_OPTION_USERAGENT, test2_str, sizeof(test2_str), 0);
    ok(hres == S_OK, "UrlMkSetSessionOption failed: %08lx\n", hres);

    size = 0;
    str2[0] = 0;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_USERAGENT, str2, saved, &size, 0);
    ok(hres == E_OUTOFMEMORY, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(size == sizeof(test_str) && !memcmp(str2, test_str, sizeof(test_str)), "wrong user agent\n");

    hres = UrlMkSetSessionOption(URLMON_OPTION_USERAGENT, test_str, 2, 0);
    ok(hres == S_OK, "UrlMkSetSessionOption failed: %08lx\n", hres);

    size = 0;
    str2[0] = 0;
    hres = pUrlMkGetSessionOption(URLMON_OPTION_USERAGENT, str2, saved, &size, 0);
    ok(hres == E_OUTOFMEMORY, "UrlMkGetSessionOption failed: %08lx\n", hres);
    ok(size == 3 && !strcmp(str2, "te"), "wrong user agent\n");

    hres = UrlMkSetSessionOption(URLMON_OPTION_USERAGENT, test_str, 0, 0);
    ok(hres == E_INVALIDARG, "UrlMkSetSessionOption failed: %08lx\n", hres);

    hres = UrlMkSetSessionOption(URLMON_OPTION_USERAGENT, NULL, sizeof(test_str), 0);
    ok(hres == E_INVALIDARG, "UrlMkSetSessionOption failed: %08lx\n", hres);

    hres = UrlMkSetSessionOption(URLMON_OPTION_USERAGENT, NULL, 0, 0);
    ok(hres == E_INVALIDARG, "UrlMkSetSessionOption failed: %08lx\n", hres);

    free(str2);
}

static void test_MkParseDisplayNameEx(void)
{
    IMoniker *mon = NULL;
    LPWSTR name;
    DWORD issys;
    ULONG eaten = 0;
    IBindCtx *bctx;
    HRESULT hres;

    static const WCHAR clsid_nameW[] = {'c','l','s','i','d',':',
            '2','0','D','0','4','F','E','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','8',
            '-','0','8','0','0','2','B','3','0','3','0','9','D',':',0};

    const struct
    {
        LPBC *ppbc;
        LPCWSTR szDisplayName;
        ULONG *pchEaten;
        LPMONIKER *ppmk;
    } invalid_parameters[] =
    {
        {NULL,  NULL,     NULL,   NULL},
        {NULL,  NULL,     NULL,   &mon},
        {NULL,  NULL,     &eaten, NULL},
        {NULL,  NULL,     &eaten, &mon},
        {NULL,  wszEmpty, NULL,   NULL},
        {NULL,  wszEmpty, NULL,   &mon},
        {NULL,  wszEmpty, &eaten, NULL},
        {NULL,  wszEmpty, &eaten, &mon},
        {&bctx, NULL,     NULL,   NULL},
        {&bctx, NULL,     NULL,   &mon},
        {&bctx, NULL,     &eaten, NULL},
        {&bctx, NULL,     &eaten, &mon},
        {&bctx, wszEmpty, NULL,   NULL},
        {&bctx, wszEmpty, NULL,   &mon},
        {&bctx, wszEmpty, &eaten, NULL},
        {&bctx, wszEmpty, &eaten, &mon},
    };

    int i;

    CreateBindCtx(0, &bctx);

    for (i = 0; i < ARRAY_SIZE(invalid_parameters); i++)
    {
        eaten = 0xdeadbeef;
        mon = (IMoniker *)0xdeadbeef;
        hres = MkParseDisplayNameEx(invalid_parameters[i].ppbc ? *invalid_parameters[i].ppbc : NULL,
                                    invalid_parameters[i].szDisplayName,
                                    invalid_parameters[i].pchEaten,
                                    invalid_parameters[i].ppmk);
        ok(hres == E_INVALIDARG,
            "[%d] Expected MkParseDisplayNameEx to return E_INVALIDARG, got %08lx\n", i, hres);
        ok(eaten == 0xdeadbeef, "[%d] Expected eaten to be 0xdeadbeef, got %lu\n", i, eaten);
        ok(mon == (IMoniker *)0xdeadbeef, "[%d] Expected mon to be 0xdeadbeef, got %p\n", i, mon);
    }

    hres = MkParseDisplayNameEx(bctx, url9, &eaten, &mon);
    ok(hres == S_OK, "MkParseDisplayNameEx failed: %08lx\n", hres);
    ok(eaten == ARRAY_SIZE(url9)-1, "eaten=%ld\n", eaten);
    ok(mon != NULL, "mon == NULL\n");

    hres = IMoniker_GetDisplayName(mon, NULL, 0, &name);
    ok(hres == S_OK, "GetDisplayName failed: %08lx\n", hres);
    ok(!lstrcmpW(name, url9), "wrong display name %s\n", wine_dbgstr_w(name));
    CoTaskMemFree(name);

    hres = IMoniker_IsSystemMoniker(mon, &issys);
    ok(hres == S_OK, "IsSystemMoniker failed: %08lx\n", hres);
    ok(issys == MKSYS_URLMONIKER, "issys=%lx\n", issys);

    IMoniker_Release(mon);

    hres = MkParseDisplayNameEx(bctx, clsid_nameW, &eaten, &mon);
    ok(hres == S_OK, "MkParseDisplayNameEx failed: %08lx\n", hres);
    ok(eaten == ARRAY_SIZE(clsid_nameW)-1, "eaten=%ld\n", eaten);
    ok(mon != NULL, "mon == NULL\n");

    hres = IMoniker_IsSystemMoniker(mon, &issys);
    ok(hres == S_OK, "IsSystemMoniker failed: %08lx\n", hres);
    ok(issys == MKSYS_CLASSMONIKER, "issys=%lx\n", issys);

    IMoniker_Release(mon);

    hres = MkParseDisplayNameEx(bctx, url8, &eaten, &mon);
    ok(FAILED(hres), "MkParseDisplayNameEx succeeded: %08lx\n", hres);

    IBindCtx_Release(bctx);
}

static void test_IsValidURL(void)
{
    HRESULT hr;
    IBindCtx *bctx = NULL;

    hr = IsValidURL(NULL, 0, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    hr = IsValidURL(NULL, wszHttpWineHQ, 0);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    CreateBindCtx(0, &bctx);

    hr = IsValidURL(bctx, wszHttpWineHQ, 0);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    IBindCtx_Release(bctx);
}

static const struct {
    INTERNETFEATURELIST feature;
    DWORD               get_flags;
    HRESULT             expected;
    BOOL                todo;
} default_feature_tests[] = {
    {FEATURE_OBJECT_CACHING,GET_FEATURE_FROM_PROCESS,S_OK},
    {FEATURE_ZONE_ELEVATION,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_MIME_HANDLING,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_MIME_SNIFFING,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_WINDOW_RESTRICTIONS,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_WEBOC_POPUPMANAGEMENT,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_BEHAVIORS,GET_FEATURE_FROM_PROCESS,S_OK},
    {FEATURE_DISABLE_MK_PROTOCOL,GET_FEATURE_FROM_PROCESS,S_OK},
    {FEATURE_LOCALMACHINE_LOCKDOWN,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_SECURITYBAND,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_RESTRICT_ACTIVEXINSTALL,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_VALIDATE_NAVIGATE_URL,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_RESTRICT_FILEDOWNLOAD,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_ADDON_MANAGEMENT,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_PROTOCOL_LOCKDOWN,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_HTTP_USERNAME_PASSWORD_DISABLE,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_SAFE_BINDTOOBJECT,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_UNC_SAVEDFILECHECK,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_GET_URL_DOM_FILEPATH_UNENCODED,GET_FEATURE_FROM_PROCESS,S_OK},
    {FEATURE_TABBED_BROWSING,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_SSLUX,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_DISABLE_NAVIGATION_SOUNDS,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_DISABLE_LEGACY_COMPRESSION,GET_FEATURE_FROM_PROCESS,S_OK},
    {FEATURE_FORCE_ADDR_AND_STATUS,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_XMLHTTP,GET_FEATURE_FROM_PROCESS,S_OK},
    {FEATURE_DISABLE_TELNET_PROTOCOL,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_FEEDS,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_BLOCK_INPUT_PROMPTS,GET_FEATURE_FROM_PROCESS,S_FALSE}
};

static void test_internet_feature_defaults(void) {
    HRESULT hres;
    DWORD i;

    for(i = 0; i < ARRAY_SIZE(default_feature_tests); ++i) {
        hres = pCoInternetIsFeatureEnabled(default_feature_tests[i].feature, default_feature_tests[i].get_flags);
        todo_wine_if (default_feature_tests[i].todo)
            ok(hres == default_feature_tests[i].expected, "CoInternetIsFeatureEnabled returned %08lx, expected %08lx on test %ld\n",
                hres, default_feature_tests[i].expected, i);
    }
}

/* With older versions of IE (IE 7 and earlier), urlmon caches
 * the FeatureControl values from the registry when it's loaded
 * into memory. Newer versions of IE conditionally cache the
 * the FeatureControl registry values (i.e. When a call to
 * CoInternetIsFeatureEnabled and a corresponding CoInternetSetFeatureEnabled
 * call hasn't already been made for the specified Feature). Because of
 * this we skip these tests on IE 7 and earlier.
 */
static const char* szFeatureControlKey = "Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl";

static void test_internet_features_registry(void) {
    HRESULT hres;
    DWORD res;
    char module[MAX_PATH];
    char *name;
    HKEY feature_control;
    HKEY feature;
    DWORD value;
    BOOL skip_zone;
    BOOL delete_feature_key = TRUE;

    static const char* szFeatureBehaviorsKey = "FEATURE_BEHAVIORS";
    static const char* szFeatureZoneElevationKey = "FEATURE_ZONE_ELEVATION";

    if(!pIEInstallScope) {
        win_skip("Skipping internet feature registry tests, IE is too old...\n");
        return;
    }

    res = GetModuleFileNameA(NULL, module, sizeof(module));
    ok(res, "GetModuleFileName failed: %ld\n", GetLastError());

    name = strrchr(module, '\\')+1;

    /* Some Windows machines don't have a FeatureControl key in HKCU. */
    res = RegOpenKeyA(HKEY_CURRENT_USER, szFeatureControlKey, &feature_control);
    ok(res == ERROR_SUCCESS, "RegCreateKey failed: %ld\n", res);

    res = RegOpenKeyA(feature_control, szFeatureBehaviorsKey, &feature);
    if(res == ERROR_SUCCESS) {
        /* FEATURE_BEHAVIORS already existed, so don't delete it when we're done. */
        delete_feature_key = FALSE;
    }else {
        res = RegCreateKeyA(feature_control, szFeatureBehaviorsKey, &feature);
        ok(res == ERROR_SUCCESS, "RegCreateKey failed: %ld\n", res);
    }

    value = 0;
    res = RegSetValueExA(feature, name, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "RegSetValueEx failed: %ld\n", res);

    hres = pCoInternetIsFeatureEnabled(FEATURE_BEHAVIORS, GET_FEATURE_FROM_PROCESS);
    ok(hres == S_FALSE, "CoInternetIsFeatureEnabled returned %08lx, expected S_FALSE\n", hres);

    if(delete_feature_key) {
        RegCloseKey(feature);
        RegDeleteKeyA(feature_control, szFeatureBehaviorsKey);
    } else {
        RegDeleteValueA(feature, name);
        RegCloseKey(feature);
    }

    /* IE's feature control cached the value it got from the registry earlier. */
    hres = pCoInternetIsFeatureEnabled(FEATURE_BEHAVIORS, GET_FEATURE_FROM_PROCESS);
    ok(hres == S_FALSE, "CoInternetIsFeatureEnabled returned %08lx, expected S_FALSE\n", hres);

    /* Restore this feature back to its default value. */
    hres = pCoInternetSetFeatureEnabled(FEATURE_BEHAVIORS, SET_FEATURE_ON_PROCESS, TRUE);
    ok(hres == S_OK, "CoInternetSetFeatureEnabled failed: %08lx\n", hres);

    RegCloseKey(feature_control);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, szFeatureControlKey, &feature_control);
    ok(res == ERROR_SUCCESS, "RegOpenKey failed: %ld\n", res);

    res = RegOpenKeyA(feature_control, szFeatureZoneElevationKey, &feature);
    ok(res == ERROR_SUCCESS, "RegOpenKey failed: %ld\n", res);

    value = 1;
    res = RegSetValueExA(feature, "*", 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not allowed to modify zone elevation\n");
        skip_zone = TRUE;
    }
    else
    {
        skip_zone = FALSE;
        ok(res == ERROR_SUCCESS, "RegSetValueEx failed: %ld\n", res);

        hres = pCoInternetIsFeatureEnabled(FEATURE_ZONE_ELEVATION, GET_FEATURE_FROM_PROCESS);
        ok(hres == S_OK, "CoInternetIsFeatureEnabled returned %08lx, expected S_OK\n", hres);
    }
    RegDeleteValueA(feature, "*");
    RegCloseKey(feature);
    RegCloseKey(feature_control);

    /* Value is still cached from last time. */
    if (!skip_zone)
    {
        hres = pCoInternetIsFeatureEnabled(FEATURE_ZONE_ELEVATION, GET_FEATURE_FROM_PROCESS);
        ok(hres == S_OK, "CoInternetIsFeatureEnabled returned %08lx, expected S_OK\n", hres);

        hres = pCoInternetSetFeatureEnabled(FEATURE_ZONE_ELEVATION, SET_FEATURE_ON_PROCESS, FALSE);
        ok(hres == S_OK, "CoInternetSetFeatureEnabled failed: %08lx\n", hres);
    }

    test_internet_feature_defaults();
}

static void test_CoInternetIsFeatureEnabled(void) {
    HRESULT hres;

    hres = pCoInternetIsFeatureEnabled(FEATURE_ENTRY_COUNT, GET_FEATURE_FROM_PROCESS);
    ok(hres == E_FAIL, "CoInternetIsFeatureEnabled returned %08lx, expected E_FAIL\n", hres);
}

static const struct {
    INTERNETFEATURELIST feature;
    DWORD               set_flags;
    BOOL                enable;
    HRESULT             set_expected;
    BOOL                set_todo;
    DWORD               get_flags;
    HRESULT             get_expected;
    BOOL                get_todo;
} internet_feature_tests[] = {
    {FEATURE_OBJECT_CACHING,SET_FEATURE_ON_PROCESS,FALSE,S_OK,FALSE,GET_FEATURE_FROM_PROCESS,S_FALSE},
    {FEATURE_WEBOC_POPUPMANAGEMENT,SET_FEATURE_ON_PROCESS,TRUE,S_OK,FALSE,GET_FEATURE_FROM_PROCESS,S_OK},
    {FEATURE_LOCALMACHINE_LOCKDOWN,SET_FEATURE_ON_PROCESS,TRUE,S_OK,FALSE,GET_FEATURE_FROM_PROCESS,S_OK}
};

static void test_CoInternetSetFeatureEnabled(void) {
    HRESULT hres;
    DWORD i;

    hres = pCoInternetSetFeatureEnabled(FEATURE_ENTRY_COUNT,SET_FEATURE_ON_PROCESS,TRUE);
    ok(hres == E_FAIL, "CoInternetSetFeatureEnabled returned %08lx, expected E_FAIL\n", hres);

    for(i = 0; i < ARRAY_SIZE(internet_feature_tests); ++i) {
        hres = pCoInternetSetFeatureEnabled(internet_feature_tests[i].feature, internet_feature_tests[i].set_flags,
                                            internet_feature_tests[i].enable);
        todo_wine_if (internet_feature_tests[i].set_todo)
            ok(hres == internet_feature_tests[i].set_expected, "CoInternetSetFeatureEnabled returned %08lx, expected %08lx on test %ld\n",
                hres, internet_feature_tests[i].set_expected, i);

        hres = pCoInternetIsFeatureEnabled(internet_feature_tests[i].feature, internet_feature_tests[i].set_flags);
        todo_wine_if (internet_feature_tests[i].get_todo)
            ok(hres == internet_feature_tests[i].get_expected, "CoInternetIsFeatureEnabled returned %08lx, expected %08lx on test %ld\n",
                hres, internet_feature_tests[i].get_expected, i);

    }
}

static void test_internet_features(void) {
    HKEY key;
    DWORD res;

    if(!pCoInternetIsFeatureEnabled || !pCoInternetSetFeatureEnabled) {
        win_skip("Skipping internet feature tests, IE is too old\n");
        return;
    }

    /* IE10 takes FeatureControl key into account only if it's available upon process start. */
    res = RegOpenKeyA(HKEY_CURRENT_USER, szFeatureControlKey, &key);
    if(res != ERROR_SUCCESS) {
        PROCESS_INFORMATION pi;
        STARTUPINFOA si = { 0 };
        char cmdline[MAX_PATH];
        char **argv;
        BOOL ret;

        res = RegCreateKeyA(HKEY_CURRENT_USER, szFeatureControlKey, &key);
        ok(res == ERROR_SUCCESS, "RegCreateKey failed: %ld\n", res);

        trace("Running features tests in a separated process.\n");

        winetest_get_mainargs( &argv );
        sprintf(cmdline, "\"%s\" %s internet_features", argv[0], argv[1]);
        ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        ok(ret, "Could not create process: %lu\n", GetLastError());
        wait_child_process( pi.hProcess );
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        RegDeleteKeyA(HKEY_CURRENT_USER, szFeatureControlKey);
        return;
    }

    test_internet_features_registry();
    test_CoInternetIsFeatureEnabled();
    test_CoInternetSetFeatureEnabled();
}

static BINDINFO rem_bindinfo = { sizeof(rem_bindinfo) }, in_bindinfo;
static DWORD rem_bindf;

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallbackEx *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IBindStatusCallbackEx, riid)
       || IsEqualGUID(&IID_IBindStatusCallback, riid)
       || IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallbackEx *iface)
{
    return 2;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallbackEx *iface)
{
    return 1;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallbackEx *iface,
        DWORD dwReserved, IBinding *pib)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallbackEx *iface, LONG *pnPriority)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallbackEx *iface, DWORD reserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallbackEx *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallbackEx *iface, HRESULT hresult, LPCWSTR szError)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallbackEx *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    in_bindinfo = *pbindinfo;
    *grfBINDF = rem_bindf;
    *pbindinfo = rem_bindinfo;
    return S_OK;
}

static STGMEDIUM in_stgmed, rem_stgmed;

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallbackEx *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    in_stgmed = *pstgmed;
    *pstgmed = rem_stgmed;
    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallbackEx *iface, REFIID riid, IUnknown *punk)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static HRESULT WINAPI BindStatusCallbackEx_GetBindInfoEx(IBindStatusCallbackEx *iface, DWORD *grfBINDF,
        BINDINFO *pbindinfo, DWORD *grfBINDF2, DWORD *pdwReserved)
{
    in_bindinfo = *pbindinfo;
    *grfBINDF = rem_bindf;
    *pbindinfo = rem_bindinfo;
    *grfBINDF2 = 11;
    *pdwReserved = 12;
    return S_OK;
}

static const IBindStatusCallbackExVtbl BindStatusCallbackExVtbl = {
    BindStatusCallback_QueryInterface,
    BindStatusCallback_AddRef,
    BindStatusCallback_Release,
    BindStatusCallback_OnStartBinding,
    BindStatusCallback_GetPriority,
    BindStatusCallback_OnLowResource,
    BindStatusCallback_OnProgress,
    BindStatusCallback_OnStopBinding,
    BindStatusCallback_GetBindInfo,
    BindStatusCallback_OnDataAvailable,
    BindStatusCallback_OnObjectAvailable,
    BindStatusCallbackEx_GetBindInfoEx
};

static IBindStatusCallbackEx BindStatusCallback = { &BindStatusCallbackExVtbl };

typedef struct {
    IUnknown IUnknown_iface;
    LONG ref;
} RefUnk;

static inline RefUnk *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, RefUnk, IUnknown_iface);
}

static HRESULT WINAPI RefUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if(!IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef(iface);
    *ppv = iface;
    return S_OK;
}

static ULONG WINAPI RefUnk_AddRef(IUnknown *iface)
{
    RefUnk *This = impl_from_IUnknown(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI RefUnk_Release(IUnknown *iface)
{
    RefUnk *This = impl_from_IUnknown(iface);
    return InterlockedDecrement(&This->ref);
}

static const IUnknownVtbl RefUnkVtbl = {
    RefUnk_QueryInterface,
    RefUnk_AddRef,
    RefUnk_Release
};

static RefUnk unk_in = {{&RefUnkVtbl}}, unk_out = {{&RefUnkVtbl}};

static HANDLE thread_ready;

static DWORD WINAPI bsc_thread(void *arg)
{
    IStream *stream = arg;
    LARGE_INTEGER zero;
    MSG msg;
    HRESULT hres;

    CoInitialize(NULL);

    hres = CoMarshalInterface(stream, &IID_IBindStatusCallback, (IUnknown*)&BindStatusCallback,
                              MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hres == S_OK, "CoMarshalInterface failed: %08lx\n", hres);

    zero.QuadPart = 0;
    hres = IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);
    ok(hres == S_OK, "Seek failed: 0x%08lx\n", hres);

    SetEvent(thread_ready);

    while(GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return 0;
}

static void test_bsc_marshaling(void)
{
    FORMATETC formatetc = {0, NULL, 1, -1, TYMED_ISTREAM};
    IBindStatusCallbackEx *callbackex;
    IBindStatusCallback *bsc;
    BINDINFO bindinfo;
    IStream *stream, *binding_stream;
    HANDLE thread;
    WCHAR *extra_info_out;
    WCHAR *verb_out;
    LARGE_INTEGER zero;
    STGMEDIUM stgmed;
    void *buf;
    DWORD bindf;
    HRESULT hres;

    hres = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hres == S_OK, "CreateStreamOnHGlobal returned: %08lx\n", hres);

    thread_ready = CreateEventW(NULL, TRUE, FALSE, NULL);
    thread = CreateThread(NULL, 0, bsc_thread, stream, 0, NULL);
    WaitForSingleObject(thread_ready, INFINITE);

    hres = CoUnmarshalInterface(stream, &IID_IBindStatusCallback, (void**)&bsc);
    ok(hres == S_OK, "CoUnmarshalInterface failed: %08lx\n", hres);

    hres = CreateStreamOnHGlobal(NULL, TRUE, &binding_stream);
    ok(hres == S_OK, "CreateStreamOnHGlobal returned: %08lx\n", hres);
    hres = IStream_Write(binding_stream, "xxx", 3, NULL);
    ok(hres == S_OK, "Write failed: %08lx\n", hres);

    rem_bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    bindf = 0xdeadbeef;

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);
    bindinfo.grfBindInfoF = 12;
    bindinfo.dwBindVerb = 13;
    bindinfo.cbstgmedData = 19;
    bindinfo.dwOptions = 14;
    bindinfo.dwOptionsFlags = 15;
    bindinfo.dwCodePage = 16;
    bindinfo.securityAttributes.nLength = 30;
    bindinfo.securityAttributes.lpSecurityDescriptor = (void*)0xdead0001;
    bindinfo.securityAttributes.bInheritHandle = 31;
    bindinfo.iid.Data1 = 17;
    bindinfo.pUnk = (IUnknown*)0xdeadbeef;
    bindinfo.dwReserved = 18;
    bindinfo.stgmedData.pUnkForRelease = &unk_in.IUnknown_iface;
    unk_in.ref = 1;

    memset(&rem_bindinfo, 0, sizeof(bindinfo));
    rem_bindinfo.cbSize = sizeof(rem_bindinfo);
    rem_bindinfo.szExtraInfo = extra_info_out = a2co("extra info out");
    rem_bindinfo.grfBindInfoF = 22;
    rem_bindinfo.dwBindVerb = 23;
    rem_bindinfo.szCustomVerb = verb_out = a2co("custom verb out");
    rem_bindinfo.cbstgmedData = 29;
    rem_bindinfo.dwOptions = 24;
    rem_bindinfo.dwOptionsFlags = 25;
    rem_bindinfo.dwCodePage = 16;
    rem_bindinfo.securityAttributes.nLength = 40;
    rem_bindinfo.securityAttributes.lpSecurityDescriptor = (void*)0xdead0002;
    rem_bindinfo.securityAttributes.bInheritHandle = 41;
    rem_bindinfo.iid.Data1 = 27;
    rem_bindinfo.pUnk = (IUnknown*)0xdeadbeef;
    rem_bindinfo.dwReserved = 18;
    rem_bindinfo.stgmedData.pUnkForRelease = &unk_out.IUnknown_iface;
    unk_out.ref = 1;

    hres = IBindStatusCallback_GetBindInfo(bsc, &bindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
    ok(bindf == rem_bindf, "bindf = %lx, expected %lx\n", bindf, rem_bindf);

    ok(in_bindinfo.cbSize == sizeof(in_bindinfo), "cbSize = %lu\n", in_bindinfo.cbSize);
    ok(!in_bindinfo.szExtraInfo, "szExtraInfo = %s\n", wine_dbgstr_w(in_bindinfo.szExtraInfo));
    ok(in_bindinfo.grfBindInfoF == 12, "cbSize = %lu\n", in_bindinfo.grfBindInfoF);
    ok(in_bindinfo.dwBindVerb == 13, "dwBindVerb = %lu\n", in_bindinfo.dwBindVerb);
    ok(!in_bindinfo.szCustomVerb, "szCustomVerb = %s\n", wine_dbgstr_w(in_bindinfo.szCustomVerb));
    ok(in_bindinfo.cbstgmedData == 19, "cbstgmedData = %lu\n", in_bindinfo.cbstgmedData);
    ok(!in_bindinfo.dwOptions, "dwOptions = %lu\n", in_bindinfo.dwOptions);
    ok(!in_bindinfo.dwOptionsFlags, "dwOptionsFlags = %lu\n", in_bindinfo.dwOptionsFlags);
    ok(!in_bindinfo.dwCodePage, "dwCodePage = %lu\n", in_bindinfo.dwCodePage);
    ok(!in_bindinfo.iid.Data1, "iid = %s\n", debugstr_guid(&in_bindinfo.iid));
    ok(!in_bindinfo.pUnk, "pUnk = %p\n", in_bindinfo.pUnk);
    ok(!in_bindinfo.dwReserved, "dwReserved = %lu\n", in_bindinfo.dwReserved);
    ok(!in_bindinfo.securityAttributes.nLength, "securityAttributes.nLength = %lu\n",
       in_bindinfo.securityAttributes.nLength);
    ok(!in_bindinfo.securityAttributes.lpSecurityDescriptor,
       "securityAttributes.lpSecurityDescriptor = %p\n",
       in_bindinfo.securityAttributes.lpSecurityDescriptor);
    ok(!in_bindinfo.securityAttributes.bInheritHandle, "securityAttributes.bInheritHandle = %u\n",
       in_bindinfo.securityAttributes.bInheritHandle);
    ok(!in_bindinfo.stgmedData.pUnkForRelease, "pUnkForRelease = %p\n",
       in_bindinfo.stgmedData.pUnkForRelease);

    ok(bindinfo.cbSize == sizeof(rem_bindinfo), "cbSize = %lu\n", rem_bindinfo.cbSize);
    ok(!lstrcmpW(bindinfo.szExtraInfo, L"extra info out"),
       "szExtraInfo = %s\n", wine_dbgstr_w(bindinfo.szExtraInfo));
    ok(bindinfo.grfBindInfoF == 22, "grfBindInfoF = %lu\n", rem_bindinfo.grfBindInfoF);
    ok(bindinfo.dwBindVerb == 23, "dwBindVerb = %lu\n", bindinfo.dwBindVerb);
    ok(bindinfo.szCustomVerb != verb_out, "szCustomVerb == inbuf\n");
    ok(!lstrcmpW(bindinfo.szCustomVerb, L"custom verb out"), "szCustomVerb = %s\n",
       wine_dbgstr_w(bindinfo.szCustomVerb));
    ok(bindinfo.cbstgmedData == 29, "cbstgmedData = %lu\n", bindinfo.cbstgmedData);
    ok(bindinfo.dwOptions == 24, "dwOptions = %lu\n", bindinfo.dwOptions);
    ok(bindinfo.dwOptionsFlags == 25, "dwOptionsFlags = %lu\n", bindinfo.dwOptionsFlags);
    ok(bindinfo.dwCodePage, "dwCodePage = %lu\n", bindinfo.dwCodePage);
    ok(!bindinfo.iid.Data1, "iid = %s\n", debugstr_guid(&bindinfo.iid));
    ok(!bindinfo.pUnk, "pUnk = %p\n", bindinfo.pUnk);
    ok(bindinfo.dwReserved == 18, "dwReserved = %lu\n", bindinfo.dwReserved);
    ok(bindinfo.securityAttributes.nLength == 30, "securityAttributes.nLength = %lu\n",
       bindinfo.securityAttributes.nLength);
    ok(bindinfo.securityAttributes.lpSecurityDescriptor == (void*)0xdead0001,
       "securityAttributes.lpSecurityDescriptor = %p\n",
       bindinfo.securityAttributes.lpSecurityDescriptor);
    ok(bindinfo.securityAttributes.bInheritHandle == 31, "securityAttributes.bInheritHandle = %u\n",
       bindinfo.securityAttributes.bInheritHandle);
    ok(bindinfo.stgmedData.pUnkForRelease == &unk_in.IUnknown_iface, "pUnkForRelease = %p\n",
       bindinfo.stgmedData.pUnkForRelease);
    ok(unk_out.ref == 1, "unk_out.ref = %lu\n", unk_out.ref);

    bindinfo.stgmedData.pUnkForRelease = NULL;
    ReleaseBindInfo(&bindinfo);

    zero.QuadPart = 0;
    hres = IStream_Seek(binding_stream, zero, STREAM_SEEK_SET, NULL);
    ok(hres == S_OK, "Seek failed: 0x%08lx\n", hres);

    /* Return IStream stgmed from GetBindInfo, it's not marshaled back */
    rem_bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    bindf = 0xdeadbeef;

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);

    memset(&rem_bindinfo, 0, sizeof(rem_bindinfo));
    rem_bindinfo.cbSize = sizeof(rem_bindinfo);

    rem_bindinfo.stgmedData.tymed = TYMED_ISTREAM;
    rem_bindinfo.stgmedData.pstm = binding_stream;
    rem_bindinfo.cbstgmedData = 3;
    IStream_AddRef(binding_stream);

    hres = IBindStatusCallback_GetBindInfo(bsc, &bindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
    ok(bindf == rem_bindf, "bindf = %lx, expected %lx\n", bindf, rem_bindf);

    ok(in_bindinfo.cbSize == sizeof(in_bindinfo), "cbSize = %lu\n", in_bindinfo.cbSize);
    ok(!in_bindinfo.pUnk, "pUnk = %p\n", in_bindinfo.pUnk);
    ok(in_bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
       in_bindinfo.stgmedData.tymed);

    ok(bindinfo.cbSize == sizeof(bindinfo), "cbSize = %lu\n", bindinfo.cbSize);
    ok(!bindinfo.pUnk, "pUnk = %p\n", bindinfo.pUnk);
    ok(bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
       bindinfo.stgmedData.tymed);
    ok(!bindinfo.stgmedData.pstm, "stm = %p\n",
       bindinfo.stgmedData.pstm);
    ok(!bindinfo.stgmedData.pUnkForRelease, "pUnkForRelease = %p\n",
       bindinfo.stgmedData.pUnkForRelease);
    ok(bindinfo.cbstgmedData == 3, "cbstgmedData = %lu\n", bindinfo.cbstgmedData);

    ReleaseBindInfo(&bindinfo);

    /* Same, but with pUnkForRelease, it's not marshaled back */
    rem_bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    bindf = 0xdeadbeef;

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);

    memset(&rem_bindinfo, 0, sizeof(rem_bindinfo));
    rem_bindinfo.cbSize = sizeof(rem_bindinfo);

    rem_bindinfo.stgmedData.tymed = TYMED_ISTREAM;
    rem_bindinfo.stgmedData.pstm = binding_stream;
    rem_bindinfo.stgmedData.pUnkForRelease = &unk_out.IUnknown_iface;
    unk_out.ref = 1;
    rem_bindinfo.cbstgmedData = 3;
    IStream_AddRef(binding_stream);

    hres = IBindStatusCallback_GetBindInfo(bsc, &bindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
    ok(bindf == rem_bindf, "bindf = %lx, expected %lx\n", bindf, rem_bindf);

    ok(in_bindinfo.cbSize == sizeof(in_bindinfo), "cbSize = %lu\n", in_bindinfo.cbSize);
    ok(!in_bindinfo.pUnk, "pUnk = %p\n", in_bindinfo.pUnk);
    ok(in_bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
       in_bindinfo.stgmedData.tymed);

    ok(bindinfo.cbSize == sizeof(bindinfo), "cbSize = %lu\n", bindinfo.cbSize);
    ok(!bindinfo.pUnk, "pUnk = %p\n", bindinfo.pUnk);
    ok(bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
       bindinfo.stgmedData.tymed);
    ok(!bindinfo.stgmedData.pstm, "stm = %p\n",
       bindinfo.stgmedData.pstm);
    ok(!bindinfo.stgmedData.pUnkForRelease, "pUnkForRelease = %p\n",
       bindinfo.stgmedData.pUnkForRelease);
    ok(bindinfo.cbstgmedData == 3, "cbstgmedData = %lu\n", bindinfo.cbstgmedData);

    ReleaseBindInfo(&bindinfo);

    /* Return HGLOBAL stgmed from GetBindInfo, it's not marshaled back */
    rem_bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    bindf = 0xdeadbeef;

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);

    memset(&rem_bindinfo, 0, sizeof(rem_bindinfo));
    rem_bindinfo.cbSize = sizeof(rem_bindinfo);

    rem_bindinfo.stgmedData.tymed = TYMED_HGLOBAL;

    buf = GlobalAlloc(0, 5);
    strcpy(buf, "test");
    rem_bindinfo.stgmedData.hGlobal = buf;
    rem_bindinfo.cbstgmedData = 5;

    hres = IBindStatusCallback_GetBindInfo(bsc, &bindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
    ok(bindf == rem_bindf, "bindf = %lx, expected %lx\n", bindf, rem_bindf);

    ok(in_bindinfo.cbSize == sizeof(in_bindinfo), "cbSize = %lu\n", in_bindinfo.cbSize);
    ok(!in_bindinfo.pUnk, "pUnk = %p\n", in_bindinfo.pUnk);
    ok(in_bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
       in_bindinfo.stgmedData.tymed);

    ok(bindinfo.cbSize == sizeof(bindinfo), "cbSize = %lu\n", bindinfo.cbSize);
    ok(!bindinfo.pUnk, "pUnk = %p\n", bindinfo.pUnk);
    ok(bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
       bindinfo.stgmedData.tymed);
    ok(!bindinfo.stgmedData.pstm, "stm = %p\n",
       bindinfo.stgmedData.pstm);
    ok(!bindinfo.stgmedData.pUnkForRelease, "pUnkForRelease = %p\n",
       bindinfo.stgmedData.pUnkForRelease);
    ok(bindinfo.cbstgmedData == 5, "cbstgmedData = %lu\n", bindinfo.cbstgmedData);

    ReleaseBindInfo(&bindinfo);

    /* Same with GetBindInfoEx */
    hres = IBindStatusCallback_QueryInterface(bsc, &IID_IBindStatusCallbackEx, (void**)&callbackex);
    if(SUCCEEDED(hres)) {
        DWORD bindf2, reserved;

        rem_bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
        bindf = bindf2 = reserved = 0xdeadbeef;

        memset(&bindinfo, 0, sizeof(bindinfo));
        bindinfo.cbSize = sizeof(bindinfo);
        bindinfo.grfBindInfoF = 12;
        bindinfo.dwBindVerb = 13;
        bindinfo.cbstgmedData = 19;
        bindinfo.dwOptions = 14;
        bindinfo.dwOptionsFlags = 15;
        bindinfo.dwCodePage = 16;
        bindinfo.securityAttributes.nLength = 30;
        bindinfo.securityAttributes.lpSecurityDescriptor = (void*)0xdead0001;
        bindinfo.securityAttributes.bInheritHandle = 31;
        bindinfo.iid.Data1 = 17;
        bindinfo.pUnk = (IUnknown*)0xdeadbeef;
        bindinfo.dwReserved = 18;
        bindinfo.stgmedData.pUnkForRelease = &unk_in.IUnknown_iface;
        unk_in.ref = 1;

        memset(&rem_bindinfo, 0, sizeof(bindinfo));
        rem_bindinfo.cbSize = sizeof(rem_bindinfo);
        rem_bindinfo.szExtraInfo = extra_info_out = a2co("extra info out");
        rem_bindinfo.grfBindInfoF = 22;
        rem_bindinfo.dwBindVerb = 23;
        rem_bindinfo.szCustomVerb = verb_out = a2co("custom verb out");
        rem_bindinfo.cbstgmedData = 29;
        rem_bindinfo.dwOptions = 24;
        rem_bindinfo.dwOptionsFlags = 25;
        rem_bindinfo.dwCodePage = 16;
        rem_bindinfo.securityAttributes.nLength = 40;
        rem_bindinfo.securityAttributes.lpSecurityDescriptor = (void*)0xdead0002;
        rem_bindinfo.securityAttributes.bInheritHandle = 41;
        rem_bindinfo.iid.Data1 = 27;
        rem_bindinfo.pUnk = (IUnknown*)0xdeadbeef;
        rem_bindinfo.dwReserved = 18;
        rem_bindinfo.stgmedData.pUnkForRelease = &unk_out.IUnknown_iface;
        unk_out.ref = 1;

        hres = IBindStatusCallbackEx_GetBindInfoEx(callbackex, &bindf, &bindinfo, &bindf2, &reserved);
        ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
        ok(bindf == rem_bindf, "bindf = %lx, expected %lx\n", bindf, rem_bindf);
        ok(bindf2 == 11, "bindf2 = %lx\n", bindf);
        ok(reserved == 12, "reserved = %lx\n", reserved);

        ok(in_bindinfo.cbSize == sizeof(in_bindinfo), "cbSize = %lu\n", in_bindinfo.cbSize);
        ok(!in_bindinfo.szExtraInfo, "szExtraInfo = %s\n", wine_dbgstr_w(in_bindinfo.szExtraInfo));
        ok(in_bindinfo.grfBindInfoF == 12, "cbSize = %lu\n", in_bindinfo.grfBindInfoF);
        ok(in_bindinfo.dwBindVerb == 13, "dwBindVerb = %lu\n", in_bindinfo.dwBindVerb);
        ok(!in_bindinfo.szCustomVerb, "szCustomVerb = %s\n", wine_dbgstr_w(in_bindinfo.szCustomVerb));
        ok(in_bindinfo.cbstgmedData == 19, "cbstgmedData = %lu\n", in_bindinfo.cbstgmedData);
        ok(!in_bindinfo.dwOptions, "dwOptions = %lu\n", in_bindinfo.dwOptions);
        ok(!in_bindinfo.dwOptionsFlags, "dwOptionsFlags = %lu\n", in_bindinfo.dwOptionsFlags);
        ok(!in_bindinfo.dwCodePage, "dwCodePage = %lu\n", in_bindinfo.dwCodePage);
        ok(!in_bindinfo.iid.Data1, "iid = %s\n", debugstr_guid(&in_bindinfo.iid));
        ok(!in_bindinfo.pUnk, "pUnk = %p\n", in_bindinfo.pUnk);
        ok(!in_bindinfo.dwReserved, "dwReserved = %lu\n", in_bindinfo.dwReserved);
        ok(!in_bindinfo.securityAttributes.nLength, "securityAttributes.nLength = %lu\n",
           in_bindinfo.securityAttributes.nLength);
        ok(!in_bindinfo.securityAttributes.lpSecurityDescriptor,
           "securityAttributes.lpSecurityDescriptor = %p\n",
           in_bindinfo.securityAttributes.lpSecurityDescriptor);
        ok(!in_bindinfo.securityAttributes.bInheritHandle, "securityAttributes.bInheritHandle = %u\n",
           in_bindinfo.securityAttributes.bInheritHandle);
        ok(!in_bindinfo.stgmedData.pUnkForRelease, "pUnkForRelease = %p\n",
           in_bindinfo.stgmedData.pUnkForRelease);

        ok(bindinfo.cbSize == sizeof(rem_bindinfo), "cbSize = %lu\n", rem_bindinfo.cbSize);
        ok(!lstrcmpW(bindinfo.szExtraInfo, L"extra info out"),
           "szExtraInfo = %s\n", wine_dbgstr_w(bindinfo.szExtraInfo));
        ok(bindinfo.grfBindInfoF == 22, "grfBindInfoF = %lu\n", rem_bindinfo.grfBindInfoF);
        ok(bindinfo.dwBindVerb == 23, "dwBindVerb = %lu\n", bindinfo.dwBindVerb);
        ok(!lstrcmpW(bindinfo.szCustomVerb, L"custom verb out"), "szCustomVerb = %s\n",
           wine_dbgstr_w(bindinfo.szCustomVerb));
        ok(bindinfo.cbstgmedData == 29, "cbstgmedData = %lu\n", bindinfo.cbstgmedData);
        ok(bindinfo.dwOptions == 24, "dwOptions = %lu\n", bindinfo.dwOptions);
        ok(bindinfo.dwOptionsFlags == 25, "dwOptionsFlags = %lu\n", bindinfo.dwOptionsFlags);
        ok(bindinfo.dwCodePage, "dwCodePage = %lu\n", bindinfo.dwCodePage);
        ok(!bindinfo.iid.Data1, "iid = %s\n", debugstr_guid(&bindinfo.iid));
        ok(!bindinfo.pUnk, "pUnk = %p\n", bindinfo.pUnk);
        ok(bindinfo.dwReserved == 18, "dwReserved = %lu\n", bindinfo.dwReserved);
        ok(bindinfo.securityAttributes.nLength == 30, "securityAttributes.nLength = %lu\n",
           bindinfo.securityAttributes.nLength);
        ok(bindinfo.securityAttributes.lpSecurityDescriptor == (void*)0xdead0001,
           "securityAttributes.lpSecurityDescriptor = %p\n",
           bindinfo.securityAttributes.lpSecurityDescriptor);
        ok(bindinfo.securityAttributes.bInheritHandle == 31, "securityAttributes.bInheritHandle = %u\n",
           bindinfo.securityAttributes.bInheritHandle);
        ok(bindinfo.stgmedData.pUnkForRelease == &unk_in.IUnknown_iface, "pUnkForRelease = %p\n",
           bindinfo.stgmedData.pUnkForRelease);
        ok(unk_out.ref == 1, "unk_out.ref = %lu\n", unk_out.ref);

        bindinfo.stgmedData.pUnkForRelease = NULL;
        ReleaseBindInfo(&bindinfo);

        zero.QuadPart = 0;
        hres = IStream_Seek(binding_stream, zero, STREAM_SEEK_SET, NULL);
        ok(hres == S_OK, "Seek failed: 0x%08lx\n", hres);

        /* Return IStream stgmed from GetBindInfoEx, it's not marshaled back */
        rem_bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
        bindf = bindf2 = reserved = 0xdeadbeef;

        memset(&bindinfo, 0, sizeof(bindinfo));
        bindinfo.cbSize = sizeof(bindinfo);

        memset(&rem_bindinfo, 0, sizeof(rem_bindinfo));
        rem_bindinfo.cbSize = sizeof(rem_bindinfo);

        rem_bindinfo.stgmedData.tymed = TYMED_ISTREAM;
        rem_bindinfo.stgmedData.pstm = binding_stream;
        rem_bindinfo.cbstgmedData = 3;
        IStream_AddRef(binding_stream);

        hres = IBindStatusCallbackEx_GetBindInfoEx(callbackex, &bindf, &bindinfo, &bindf2, &reserved);
        ok(hres == S_OK, "GetBindInfoEx failed: %08lx\n", hres);
        ok(bindf == rem_bindf, "bindf = %lx, expected %lx\n", bindf, rem_bindf);

        ok(in_bindinfo.cbSize == sizeof(in_bindinfo), "cbSize = %lu\n", in_bindinfo.cbSize);
        ok(!in_bindinfo.pUnk, "pUnk = %p\n", in_bindinfo.pUnk);
        ok(in_bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
           in_bindinfo.stgmedData.tymed);

        ok(bindinfo.cbSize == sizeof(bindinfo), "cbSize = %lu\n", bindinfo.cbSize);
        ok(!bindinfo.pUnk, "pUnk = %p\n", bindinfo.pUnk);
        ok(bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
           bindinfo.stgmedData.tymed);
        ok(!bindinfo.stgmedData.pstm, "stm = %p\n",
           bindinfo.stgmedData.pstm);
        ok(!bindinfo.stgmedData.pUnkForRelease, "pUnkForRelease = %p\n",
           bindinfo.stgmedData.pUnkForRelease);
        ok(bindinfo.cbstgmedData == 3, "cbstgmedData = %lu\n", bindinfo.cbstgmedData);

        ReleaseBindInfo(&bindinfo);

        /* Same, but with pUnkForRelease, it's not marshaled back */
        rem_bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
        bindf = bindf2 = reserved = 0xdeadbeef;

        memset(&bindinfo, 0, sizeof(bindinfo));
        bindinfo.cbSize = sizeof(bindinfo);

        memset(&rem_bindinfo, 0, sizeof(rem_bindinfo));
        rem_bindinfo.cbSize = sizeof(rem_bindinfo);

        rem_bindinfo.stgmedData.tymed = TYMED_ISTREAM;
        rem_bindinfo.stgmedData.pstm = binding_stream;
        rem_bindinfo.stgmedData.pUnkForRelease = &unk_out.IUnknown_iface;
        unk_out.ref = 1;
        rem_bindinfo.cbstgmedData = 3;
        IStream_AddRef(binding_stream);

        hres = IBindStatusCallbackEx_GetBindInfoEx(callbackex, &bindf, &bindinfo, &bindf2, &reserved);
        ok(hres == S_OK, "GetBindInfoEx failed: %08lx\n", hres);
        ok(bindf == rem_bindf, "bindf = %lx, expected %lx\n", bindf, rem_bindf);

        ok(in_bindinfo.cbSize == sizeof(in_bindinfo), "cbSize = %lu\n", in_bindinfo.cbSize);
        ok(!in_bindinfo.pUnk, "pUnk = %p\n", in_bindinfo.pUnk);
        ok(in_bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
           in_bindinfo.stgmedData.tymed);

        ok(bindinfo.cbSize == sizeof(bindinfo), "cbSize = %lu\n", bindinfo.cbSize);
        ok(!bindinfo.pUnk, "pUnk = %p\n", bindinfo.pUnk);
        ok(bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
           bindinfo.stgmedData.tymed);
        ok(!bindinfo.stgmedData.pstm, "stm = %p\n",
           bindinfo.stgmedData.pstm);
        ok(!bindinfo.stgmedData.pUnkForRelease, "pUnkForRelease = %p\n",
           bindinfo.stgmedData.pUnkForRelease);
        ok(bindinfo.cbstgmedData == 3, "cbstgmedData = %lu\n", bindinfo.cbstgmedData);

        ReleaseBindInfo(&bindinfo);

        /* Return HGLOBAL stgmed from GetBindInfoEx, it's not marshaled back */
        rem_bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
        bindf = bindf2 = reserved = 0xdeadbeef;

        memset(&bindinfo, 0, sizeof(bindinfo));
        bindinfo.cbSize = sizeof(bindinfo);

        memset(&rem_bindinfo, 0, sizeof(rem_bindinfo));
        rem_bindinfo.cbSize = sizeof(rem_bindinfo);

        rem_bindinfo.stgmedData.tymed = TYMED_HGLOBAL;

        buf = GlobalAlloc(0, 5);
        strcpy(buf, "test");
        rem_bindinfo.stgmedData.hGlobal = buf;
        rem_bindinfo.cbstgmedData = 5;

        hres = IBindStatusCallbackEx_GetBindInfoEx(callbackex, &bindf, &bindinfo, &bindf2, &reserved);
        ok(hres == S_OK, "GetBindInfoEx failed: %08lx\n", hres);
        ok(bindf == rem_bindf, "bindf = %lx, expected %lx\n", bindf, rem_bindf);

        ok(in_bindinfo.cbSize == sizeof(in_bindinfo), "cbSize = %lu\n", in_bindinfo.cbSize);
        ok(!in_bindinfo.pUnk, "pUnk = %p\n", in_bindinfo.pUnk);
        ok(in_bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
           in_bindinfo.stgmedData.tymed);

        ok(bindinfo.cbSize == sizeof(bindinfo), "cbSize = %lu\n", bindinfo.cbSize);
        ok(!bindinfo.pUnk, "pUnk = %p\n", bindinfo.pUnk);
        ok(bindinfo.stgmedData.tymed == TYMED_NULL, "tymed = %lu\n",
           bindinfo.stgmedData.tymed);
        ok(!bindinfo.stgmedData.pstm, "stm = %p\n",
           bindinfo.stgmedData.pstm);
        ok(!bindinfo.stgmedData.pUnkForRelease, "pUnkForRelease = %p\n",
           bindinfo.stgmedData.pUnkForRelease);
        ok(bindinfo.cbstgmedData == 5, "cbstgmedData = %lu\n", bindinfo.cbstgmedData);

        ReleaseBindInfo(&bindinfo);

        IBindStatusCallbackEx_Release(callbackex);
    }else {
        win_skip("IBindStatusCallbackEx not supported\n");
    }

    /* Test marshaling stgmed from OnDataAvailable */
    memset(&in_stgmed, 0xcc, sizeof(in_stgmed));
    stgmed.tymed = TYMED_ISTREAM;
    stgmed.pstm = binding_stream;
    stgmed.pUnkForRelease = NULL;

    hres = IBindStatusCallback_OnDataAvailable(bsc, 1, 10, &formatetc, &stgmed);
    ok(hres == S_OK, "OnDataAvailable failed: %08lx\n", hres);

    ok(in_stgmed.tymed == TYMED_ISTREAM, "tymed = %lu\n", in_stgmed.tymed);
    ok(in_stgmed.pstm != NULL, "pstm = NULL\n");
    ok(!in_stgmed.pUnkForRelease, "pUnkForRelease = %p\n", in_stgmed.pUnkForRelease);

    /* OnDataAvailable with both IStream and pUnkForRelease */
    memset(&in_stgmed, 0xcc, sizeof(in_stgmed));
    stgmed.tymed = TYMED_ISTREAM;
    stgmed.pstm = binding_stream;
    stgmed.pUnkForRelease = &unk_in.IUnknown_iface;
    unk_in.ref = 1;

    hres = IBindStatusCallback_OnDataAvailable(bsc, 1, 10, &formatetc, &stgmed);
    ok(hres == S_OK, "OnDataAvailable failed: %08lx\n", hres);

    ok(in_stgmed.tymed == TYMED_ISTREAM, "tymed = %lu\n", in_stgmed.tymed);
    ok(in_stgmed.pstm != NULL, "pstm = NULL\n");
    ok(in_stgmed.pUnkForRelease != NULL, "pUnkForRelease = %p\n", in_stgmed.pUnkForRelease);
    ok(unk_in.ref > 1, "ref = %lu\n", unk_in.ref);

    /* OnDataAvailable with TYMED_ISTREAM, but NULL stream */
    memset(&in_stgmed, 0xcc, sizeof(in_stgmed));
    stgmed.tymed = TYMED_ISTREAM;
    stgmed.pstm = binding_stream;
    stgmed.pUnkForRelease = NULL;

    hres = IBindStatusCallback_OnDataAvailable(bsc, 1, 10, &formatetc, &stgmed);
    ok(hres == S_OK, "OnDataAvailable failed: %08lx\n", hres);

    ok(in_stgmed.tymed == TYMED_ISTREAM, "tymed = %lu\n", in_stgmed.tymed);
    ok(in_stgmed.pstm != NULL, "pstm = NULL\n");
    ok(!in_stgmed.pUnkForRelease, "pUnkForRelease = %p\n", in_stgmed.pUnkForRelease);

    /* OnDataAvailable with TYMED_NULL and pUnkForRelease */
    memset(&in_stgmed, 0xcc, sizeof(in_stgmed));
    stgmed.tymed = TYMED_NULL;
    stgmed.pstm = binding_stream;
    stgmed.pUnkForRelease = &unk_in.IUnknown_iface;
    unk_in.ref = 1;

    hres = IBindStatusCallback_OnDataAvailable(bsc, 1, 10, &formatetc, &stgmed);
    ok(hres == S_OK, "OnDataAvailable failed: %08lx\n", hres);

    ok(in_stgmed.tymed == TYMED_NULL, "tymed = %lu\n", in_stgmed.tymed);
    ok(!in_stgmed.pstm, "pstm != NULL\n");
    ok(in_stgmed.pUnkForRelease != NULL, "pUnkForRelease = %p\n", in_stgmed.pUnkForRelease);
    ok(unk_in.ref == 1, "ref = %lu\n", unk_in.ref);

    IStream_Release(binding_stream);
    IBindStatusCallback_Release(bsc);

    TerminateThread(thread, 0);
}

static void test_MapBrowserEmulationModeToUserAgent(BOOL custom_ua)
{
    /* Undocumented structure of unknown size, crashes if it's too small (with arbitrary values from stack) */
    struct
    {
        DWORD version;
        char unknown[252];
    } arg;
    static char test_str[] = "test";
    const char *custom_ua_msg = "";
    HRESULT hres;
    unsigned i;
    WCHAR *ua;

    if(!pMapBrowserEmulationModeToUserAgent) {
        win_skip("MapBrowserEmulationModeToUserAgent not available\n");
        return;
    }
    memset(&arg, 0, sizeof(arg));

    if(custom_ua) {
        hres = UrlMkSetSessionOption(URLMON_OPTION_USERAGENT, test_str, sizeof(test_str), 0);
        ok(hres == S_OK, "UrlMkSetSessionOption failed: %08lx\n", hres);
        custom_ua_msg = " (with custom ua)";
    }

    for(i = 0; i < 12; i++) {
        arg.version = i;
        ua = (WCHAR*)0xdeadbeef;
        hres = pMapBrowserEmulationModeToUserAgent(&arg, &ua);
        ok(hres == (i == 5 || i >= 7 || custom_ua ? S_OK : E_FAIL),
           "[%u] MapBrowserEmulationModeToUserAgent%s returned %08lx\n", i, custom_ua_msg, hres);
        if(hres != S_OK)
            ok(ua == NULL, "[%u] ua%s = %p\n", i, custom_ua_msg, ua);
        else {
            char buf[1024];
            DWORD size = sizeof(buf);
            WCHAR *ua2;

            if(custom_ua)
                ua2 = a2co(test_str);
            else {
                hres = pObtainUserAgentString(i, buf, &size);
                ok(hres == S_OK, "[%u] ObtainUserAgentString%s failed: %08lx\n", i, custom_ua_msg, hres);
                ua2 = a2co(buf);
            }
            ok(!lstrcmpW(ua, ua2), "[%u] ua%s = %s, expected %s\n", i, custom_ua_msg, wine_dbgstr_w(ua), wine_dbgstr_w(ua2));
            CoTaskMemFree(ua2);
            CoTaskMemFree(ua);
        }
    }
}

START_TEST(misc)
{
    HMODULE hurlmon;
    int argc;
    char **argv;

    argc = winetest_get_mainargs(&argv);

    hurlmon = GetModuleHandleA("urlmon.dll");
    pCoInternetCompareUrl = (void *) GetProcAddress(hurlmon, "CoInternetCompareUrl");
    pCoInternetGetSecurityUrl = (void*) GetProcAddress(hurlmon, "CoInternetGetSecurityUrl");
    pCoInternetGetSession = (void*) GetProcAddress(hurlmon, "CoInternetGetSession");
    pCoInternetParseUrl = (void*) GetProcAddress(hurlmon, "CoInternetParseUrl");
    pCoInternetQueryInfo = (void*) GetProcAddress(hurlmon, "CoInternetQueryInfo");
    pCopyStgMedium = (void*) GetProcAddress(hurlmon, "CopyStgMedium");
    pCopyBindInfo = (void*) GetProcAddress(hurlmon, "CopyBindInfo");
    pFindMimeFromData = (void*) GetProcAddress(hurlmon, "FindMimeFromData");
    pObtainUserAgentString = (void*) GetProcAddress(hurlmon, "ObtainUserAgentString");
    pReleaseBindInfo = (void*) GetProcAddress(hurlmon, "ReleaseBindInfo");
    pUrlMkGetSessionOption = (void*) GetProcAddress(hurlmon, "UrlMkGetSessionOption");
    pCompareSecurityIds = (void*) GetProcAddress(hurlmon, "CompareSecurityIds");
    pCoInternetIsFeatureEnabled = (void*) GetProcAddress(hurlmon, "CoInternetIsFeatureEnabled");
    pCoInternetSetFeatureEnabled = (void*) GetProcAddress(hurlmon, "CoInternetSetFeatureEnabled");
    pIEInstallScope = (void*) GetProcAddress(hurlmon, "IEInstallScope");
    pMapBrowserEmulationModeToUserAgent = (void*) GetProcAddress(hurlmon, (const char*)445);

    if (!pCoInternetCompareUrl || !pCoInternetGetSecurityUrl ||
        !pCoInternetGetSession || !pCoInternetParseUrl || !pCompareSecurityIds) {
        win_skip("Various needed functions not present, too old IE\n");
        return;
    }

    OleInitialize(NULL);

    if(argc <= 2 || strcmp(argv[2], "internet_features")) {
        register_protocols();

        test_MapBrowserEmulationModeToUserAgent(FALSE);
        test_CreateFormatEnum();
        test_RegisterFormatEnumerator();
        test_CoInternetParseUrl();
        test_CoInternetCompareUrl();
        test_CoInternetQueryInfo();
        test_FindMimeFromData();
        test_NameSpace();
        test_MimeFilter();
        test_ReleaseBindInfo();
        test_CopyStgMedium();
        test_CopyBindInfo();
        test_UrlMkGetSessionOption();
        test_user_agent();
        test_MkParseDisplayNameEx();
        test_IsValidURL();
        test_bsc_marshaling();
        test_MapBrowserEmulationModeToUserAgent(TRUE);
    }

    test_internet_features();

    OleUninitialize();
}
