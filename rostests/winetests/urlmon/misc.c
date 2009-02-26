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
#define NONAMELESSUNION

#include <wine/test.h>
#include <stdarg.h>
#include <stddef.h>

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
DEFINE_EXPECT(QI_IInternetProtocolInfo);
DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(unk_Release);

static const char *debugstr_w(LPCWSTR str)
{
    static char buf[1024];
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
    return buf;
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
    ok(hres == E_FAIL, "CreateFormatEnumerator failed: %08x, expected E_FAIL\n", hres);
    hres = CreateFormatEnumerator(0, formatetc, NULL);
    ok(hres == E_INVALIDARG, "CreateFormatEnumerator failed: %08x, expected E_INVALIDARG\n", hres);
    hres = CreateFormatEnumerator(5, formatetc, NULL);
    ok(hres == E_INVALIDARG, "CreateFormatEnumerator failed: %08x, expected E_INVALIDARG\n", hres);


    hres = CreateFormatEnumerator(5, formatetc, &fenum);
    ok(hres == S_OK, "CreateFormatEnumerator failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IEnumFORMATETC_Next(fenum, 2, NULL, &ul);
    ok(hres == E_INVALIDARG, "Next failed: %08x, expected E_INVALIDARG\n", hres);
    ul = 100;
    hres = IEnumFORMATETC_Next(fenum, 0, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08x\n", hres);
    ok(ul == 0, "ul=%d, expected 0\n", ul);

    hres = IEnumFORMATETC_Next(fenum, 2, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08x\n", hres);
    ok(fetc[0].lindex == 0, "fetc[0].lindex=%d, expected 0\n", fetc[0].lindex);
    ok(fetc[1].lindex == 1, "fetc[1].lindex=%d, expected 1\n", fetc[1].lindex);
    ok(fetc[0].ptd == &dev, "fetc[0].ptd=%p, expected %p\n", fetc[0].ptd, &dev);
    ok(ul == 2, "ul=%d, expected 2\n", ul);

    hres = IEnumFORMATETC_Skip(fenum, 1);
    ok(hres == S_OK, "Skip failed: %08x\n", hres);

    hres = IEnumFORMATETC_Next(fenum, 4, fetc, &ul);
    ok(hres == S_FALSE, "Next failed: %08x, expected S_FALSE\n", hres);
    ok(fetc[0].lindex == 3, "fetc[0].lindex=%d, expected 3\n", fetc[0].lindex);
    ok(fetc[1].lindex == 4, "fetc[1].lindex=%d, expected 4\n", fetc[1].lindex);
    ok(fetc[0].ptd == NULL, "fetc[0].ptd=%p, expected NULL\n", fetc[0].ptd);
    ok(ul == 2, "ul=%d, expected 2\n", ul);

    hres = IEnumFORMATETC_Next(fenum, 4, fetc, &ul);
    ok(hres == S_FALSE, "Next failed: %08x, expected S_FALSE\n", hres);
    ok(ul == 0, "ul=%d, expected 0\n", ul);
    ul = 100;
    hres = IEnumFORMATETC_Next(fenum, 0, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08x\n", hres);
    ok(ul == 0, "ul=%d, expected 0\n", ul);

    hres = IEnumFORMATETC_Skip(fenum, 3);
    ok(hres == S_FALSE, "Skip failed: %08x, expected S_FALSE\n", hres);

    hres = IEnumFORMATETC_Reset(fenum);
    ok(hres == S_OK, "Reset failed: %08x\n", hres);

    hres = IEnumFORMATETC_Next(fenum, 5, fetc, NULL);
    ok(hres == S_OK, "Next failed: %08x\n", hres);
    ok(fetc[0].lindex == 0, "fetc[0].lindex=%d, expected 0\n", fetc[0].lindex);

    hres = IEnumFORMATETC_Reset(fenum);
    ok(hres == S_OK, "Reset failed: %08x\n", hres);

    hres = IEnumFORMATETC_Skip(fenum, 2);
    ok(hres == S_OK, "Skip failed: %08x\n", hres);

    hres = IEnumFORMATETC_Clone(fenum, NULL);
    ok(hres == E_INVALIDARG, "Clone failed: %08x, expected E_INVALIDARG\n", hres);

    hres = IEnumFORMATETC_Clone(fenum, &fenum2);
    ok(hres == S_OK, "Clone failed: %08x\n", hres);

    if(SUCCEEDED(hres)) {
        ok(fenum != fenum2, "fenum == fenum2\n");

        hres = IEnumFORMATETC_Next(fenum2, 2, fetc, &ul);
        ok(hres == S_OK, "Next failed: %08x\n", hres);
        ok(fetc[0].lindex == 2, "fetc[0].lindex=%d, expected 2\n", fetc[0].lindex);

        IEnumFORMATETC_Release(fenum2);
    }

    hres = IEnumFORMATETC_Next(fenum, 2, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08x\n", hres);
    ok(fetc[0].lindex == 2, "fetc[0].lindex=%d, expected 2\n", fetc[0].lindex);

    hres = IEnumFORMATETC_Skip(fenum, 1);
    ok(hres == S_OK, "Skip failed: %08x\n", hres);
    
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
    ok(hres == S_OK, "CreateFormatEnumerator failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = RegisterFormatEnumerator(NULL, format, 0);
    ok(hres == E_INVALIDARG,
            "RegisterFormatEnumerator failed: %08x, expected E_INVALIDARG\n", hres);
    hres = RegisterFormatEnumerator(bctx, NULL, 0);
    ok(hres == E_INVALIDARG,
            "RegisterFormatEnumerator failed: %08x, expected E_INVALIDARG\n", hres);

    hres = RegisterFormatEnumerator(bctx, format, 0);
    ok(hres == S_OK, "RegisterFormatEnumerator failed: %08x\n", hres);

    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == S_OK, "GetObjectParam failed: %08x\n", hres);
    ok(unk == (IUnknown*)format, "unk != format\n");

    hres = RevokeFormatEnumerator(NULL, format);
    ok(hres == E_INVALIDARG,
            "RevokeFormatEnumerator failed: %08x, expected E_INVALIDARG\n", hres);

    hres = RevokeFormatEnumerator(bctx, format);
    ok(hres == S_OK, "RevokeFormatEnumerator failed: %08x\n", hres);

    hres = RevokeFormatEnumerator(bctx, format);
    ok(hres == E_FAIL, "RevokeFormatEnumerator failed: %08x, expected E_FAIL\n", hres);

    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == E_FAIL, "GetObjectParam failed: %08x, expected E_FAIL\n", hres);

    hres = RegisterFormatEnumerator(bctx, format, 0);
    ok(hres == S_OK, "RegisterFormatEnumerator failed: %08x\n", hres);

    hres = CreateFormatEnumerator(1, &formatetc, &format2);
    ok(hres == S_OK, "CreateFormatEnumerator failed: %08x\n", hres);

    if(SUCCEEDED(hres)) {
        hres = RevokeFormatEnumerator(bctx, format);
        ok(hres == S_OK, "RevokeFormatEnumerator failed: %08x\n", hres);

        IEnumFORMATETC_Release(format2);
    }

    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == E_FAIL, "GetObjectParam failed: %08x, expected E_FAIL\n", hres);

    IEnumFORMATETC_Release(format);

    hres = RegisterFormatEnumerator(bctx, format, 0);
    ok(hres == S_OK, "RegisterFormatEnumerator failed: %08x\n", hres);
    hres = RevokeFormatEnumerator(bctx, NULL);
    ok(hres == S_OK, "RevokeFormatEnumerator failed: %08x\n", hres);
    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == E_FAIL, "GetObjectParam failed: %08x, expected E_FAIL\n", hres);

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
static const WCHAR url10[] = {'f','i','l','e',':','/','/','s','o','m','e','%','2','0','f','i','l','e',
        '.','j','p','g',0};

static const WCHAR url4e[] = {'f','i','l','e',':','s','o','m','e',' ','f','i','l','e',
        '.','j','p','g',0};

static const WCHAR path3[] = {'c',':','\\','I','n','d','e','x','.','h','t','m',0};
static const WCHAR path4[] = {'s','o','m','e',' ','f','i','l','e','.','j','p','g',0};

static const WCHAR wszRes[] = {'r','e','s',0};
static const WCHAR wszFile[] = {'f','i','l','e',0};
static const WCHAR wszHttp[] = {'h','t','t','p',0};
static const WCHAR wszAbout[] = {'a','b','o','u','t',0};
static const WCHAR wszEmpty[] = {0};

struct parse_test {
    LPCWSTR url;
    HRESULT secur_hres;
    LPCWSTR encoded_url;
    HRESULT path_hres;
    LPCWSTR path;
    LPCWSTR schema;
};

static const struct parse_test parse_tests[] = {
    {url1, S_OK,   url1,  E_INVALIDARG, NULL, wszRes},
    {url2, E_FAIL, url2,  E_INVALIDARG, NULL, wszEmpty},
    {url3, E_FAIL, url3,  S_OK, path3,        wszFile},
    {url4, E_FAIL, url4e, S_OK, path4,        wszFile},
    {url5, E_FAIL, url5,  E_INVALIDARG, NULL, wszHttp},
    {url6, S_OK,   url6,  E_INVALIDARG, NULL, wszAbout}
};

static void test_CoInternetParseUrl(void)
{
    HRESULT hres;
    DWORD size;
    int i;

    static WCHAR buf[4096];

    memset(buf, 0xf0, sizeof(buf));
    hres = CoInternetParseUrl(parse_tests[0].url, PARSE_SCHEMA, 0, buf,
            3, &size, 0);
    ok(hres == E_POINTER, "schema failed: %08x, expected E_POINTER\n", hres);

    for(i=0; i < sizeof(parse_tests)/sizeof(parse_tests[0]); i++) {
        memset(buf, 0xf0, sizeof(buf));
        hres = CoInternetParseUrl(parse_tests[i].url, PARSE_SECURITY_URL, 0, buf,
                sizeof(buf)/sizeof(WCHAR), &size, 0);
        ok(hres == parse_tests[i].secur_hres, "[%d] security url failed: %08x, expected %08x\n",
                i, hres, parse_tests[i].secur_hres);

        memset(buf, 0xf0, sizeof(buf));
        hres = CoInternetParseUrl(parse_tests[i].url, PARSE_ENCODE, 0, buf,
                sizeof(buf)/sizeof(WCHAR), &size, 0);
        ok(hres == S_OK, "[%d] encoding failed: %08x\n", i, hres);
        ok(size == lstrlenW(parse_tests[i].encoded_url), "[%d] wrong size\n", i);
        ok(!lstrcmpW(parse_tests[i].encoded_url, buf), "[%d] wrong encoded url\n", i);

        memset(buf, 0xf0, sizeof(buf));
        hres = CoInternetParseUrl(parse_tests[i].url, PARSE_PATH_FROM_URL, 0, buf,
                sizeof(buf)/sizeof(WCHAR), &size, 0);
        ok(hres == parse_tests[i].path_hres, "[%d] path failed: %08x, expected %08x\n",
                i, hres, parse_tests[i].path_hres);
        if(parse_tests[i].path) {
            ok(size == lstrlenW(parse_tests[i].path), "[%d] wrong size\n", i);
            ok(!lstrcmpW(parse_tests[i].path, buf), "[%d] wrong path\n", i);
        }

        memset(buf, 0xf0, sizeof(buf));
        hres = CoInternetParseUrl(parse_tests[i].url, PARSE_SCHEMA, 0, buf,
                sizeof(buf)/sizeof(WCHAR), &size, 0);
        ok(hres == S_OK, "[%d] schema failed: %08x\n", i, hres);
        ok(size == lstrlenW(parse_tests[i].schema), "[%d] wrong size\n", i);
        ok(!lstrcmpW(parse_tests[i].schema, buf), "[%d] wrong schema\n", i);
    }
}

static void test_CoInternetCompareUrl(void)
{
    HRESULT hres;

    hres = CoInternetCompareUrl(url1, url1, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08x\n", hres);

    hres = CoInternetCompareUrl(url1, url3, 0);
    ok(hres == S_FALSE, "CoInternetParseUrl failed: %08x\n", hres);

    hres = CoInternetCompareUrl(url3, url1, 0);
    ok(hres == S_FALSE, "CoInternetParseUrl failed: %08x\n", hres);
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

    for(i=0; i < sizeof(query_info_tests)/sizeof(query_info_tests[0]); i++) {
        cb = 0xdeadbeef;
        memset(buf, '?', sizeof(buf));
        hres = CoInternetQueryInfo(query_info_tests[0].url, QUERY_USES_NETWORK, 0, buf, sizeof(buf), &cb, 0);
        ok(hres == S_OK, "[%d] CoInternetQueryInfo failed: %08x\n", i, hres);
        ok(cb == sizeof(DWORD), "[%d] cb = %d\n", i, cb);
        ok(*(DWORD*)buf == query_info_tests[i].uses_net, "[%d] ret %x, expected %x\n",
           i, *(DWORD*)buf, query_info_tests[i].uses_net);

        hres = CoInternetQueryInfo(query_info_tests[0].url, QUERY_USES_NETWORK, 0, buf, 3, &cb, 0);
        ok(hres == E_FAIL, "[%d] CoInternetQueryInfo failed: %08x, expected E_FAIL\n", i, hres);
        hres = CoInternetQueryInfo(query_info_tests[0].url, QUERY_USES_NETWORK, 0, NULL, sizeof(buf), &cb, 0);
        ok(hres == E_FAIL, "[%d] CoInternetQueryInfo failed: %08x, expected E_FAIL\n", i, hres);

        memset(buf, '?', sizeof(buf));
        hres = CoInternetQueryInfo(query_info_tests[0].url, QUERY_USES_NETWORK, 0, buf, sizeof(buf), NULL, 0);
        ok(hres == S_OK, "[%d] CoInternetQueryInfo failed: %08x\n", i, hres);
        ok(*(DWORD*)buf == query_info_tests[i].uses_net, "[%d] ret %x, expected %x\n",
           i, *(DWORD*)buf, query_info_tests[i].uses_net);
    }
}

static const WCHAR mimeTextHtml[] = {'t','e','x','t','/','h','t','m','l',0};
static const WCHAR mimeTextPlain[] = {'t','e','x','t','/','p','l','a','i','n',0};
static const WCHAR mimeTextRichtext[] = {'t','e','x','t','/','r','i','c','h','t','e','x','t',0};
static const WCHAR mimeAppOctetStream[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
    'o','c','t','e','t','-','s','t','r','e','a','m',0};
static const WCHAR mimeImagePjpeg[] = {'i','m','a','g','e','/','p','j','p','e','g',0};
static const WCHAR mimeImageGif[] = {'i','m','a','g','e','/','g','i','f',0};
static const WCHAR mimeImageBmp[] = {'i','m','a','g','e','/','b','m','p',0};
static const WCHAR mimeImageXPng[] = {'i','m','a','g','e','/','x','-','p','n','g',0};
static const WCHAR mimeImageTiff[] = {'i','m','a','g','e','/','t','i','f','f',0};
static const WCHAR mimeVideoAvi[] = {'v','i','d','e','o','/','a','v','i',0};
static const WCHAR mimeVideoMpeg[] = {'v','i','d','e','o','/','m','p','e','g',0};
static const WCHAR mimeAppPostscript[] =
    {'a','p','p','l','i','c','a','t','i','o','n','/','p','o','s','t','s','c','r','i','p','t',0};
static const WCHAR mimeAppXCompressed[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
                                    'x','-','c','o','m','p','r','e','s','s','e','d',0};
static const WCHAR mimeAppXZip[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
                                    'x','-','z','i','p','-','c','o','m','p','r','e','s','s','e','d',0};
static const WCHAR mimeAppXGzip[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
                                    'x','-','g','z','i','p','-','c','o','m','p','r','e','s','s','e','d',0};
static const WCHAR mimeAppJava[] = {'a','p','p','l','i','c','a','t','i','o','n','/','j','a','v','a',0};
static const WCHAR mimeAppPdf[] = {'a','p','p','l','i','c','a','t','i','o','n','/','p','d','f',0};
static const WCHAR mimeAppXMSDownload[] =
    {'a','p','p','l','i','c','a','t','i','o','n','/','x','-','m','s','d','o','w','n','l','o','a','d',0};
static const WCHAR mimeAudioWav[] = {'a','u','d','i','o','/','w','a','v',0};
static const WCHAR mimeAudioBasic[] = {'a','u','d','i','o','/','b','a','s','i','c',0};

static const struct {
    LPCWSTR url;
    LPCWSTR mime;
    HRESULT hres;
} mime_tests[] = {
    {url1, mimeTextHtml, S_OK},
    {url2, mimeTextHtml, S_OK},
    {url3, mimeTextHtml, S_OK},
    {url4, NULL, E_FAIL},
    {url5, NULL, __HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)},
    {url6, NULL, E_FAIL},
    {url7, NULL, __HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)}
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
static BYTE data39[] = {0x4d,0x4d,0x00,0x2a};
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

static const struct {
    BYTE *data;
    DWORD size;
    LPCWSTR mime;
} mime_tests2[] = {
    {data1, sizeof(data1), mimeTextPlain},
    {data2, sizeof(data2), mimeAppOctetStream},
    {data3, sizeof(data3), mimeAppOctetStream},
    {data4, sizeof(data4), mimeAppOctetStream},
    {data5, sizeof(data5), mimeTextPlain},
    {data6, sizeof(data6), mimeTextPlain},
    {data7, sizeof(data7), mimeTextHtml},
    {data8, sizeof(data8), mimeTextHtml},
    {data9, sizeof(data9), mimeTextHtml},
    {data10, sizeof(data10), mimeTextHtml},
    {data11, sizeof(data11), mimeTextHtml},
    {data12, sizeof(data12), mimeTextHtml},
    {data13, sizeof(data13), mimeTextPlain},
    {data14, sizeof(data14), mimeTextPlain},
    {data15, sizeof(data15), mimeTextPlain},
    {data16, sizeof(data16), mimeImagePjpeg},
    {data17, sizeof(data17), mimeAppOctetStream},
    {data18, sizeof(data18), mimeTextHtml},
    {data19, sizeof(data19), mimeImageGif},
    {data20, sizeof(data20), mimeImageGif},
    {data21, sizeof(data21), mimeTextPlain},
    {data22, sizeof(data22), mimeImageGif},
    {data23, sizeof(data23), mimeTextPlain},
    {data24, sizeof(data24), mimeImageGif},
    {data25, sizeof(data25), mimeImageGif},
    {data26, sizeof(data26), mimeTextHtml},
    {data27, sizeof(data27), mimeTextPlain},
    {data28, sizeof(data28), mimeImageBmp},
    {data29, sizeof(data29), mimeImageBmp},
    {data30, sizeof(data30), mimeAppOctetStream},
    {data31, sizeof(data31), mimeTextHtml},
    {data32, sizeof(data32), mimeAppOctetStream},
    {data33, sizeof(data33), mimeAppOctetStream},
    {data34, sizeof(data34), mimeImageXPng},
    {data35, sizeof(data35), mimeImageXPng},
    {data36, sizeof(data36), mimeAppOctetStream},
    {data37, sizeof(data37), mimeTextHtml},
    {data38, sizeof(data38), mimeAppOctetStream},
    {data39, sizeof(data39), mimeImageTiff},
    {data40, sizeof(data40), mimeTextHtml},
    {data41, sizeof(data41), mimeImageTiff},
    {data42, sizeof(data42), mimeTextPlain},
    {data43, sizeof(data43), mimeAppOctetStream},
    {data44, sizeof(data44), mimeVideoAvi},
    {data45, sizeof(data45), mimeTextPlain},
    {data46, sizeof(data46), mimeTextPlain},
    {data47, sizeof(data47), mimeTextPlain},
    {data48, sizeof(data48), mimeTextHtml},
    {data49, sizeof(data49), mimeVideoAvi},
    {data50, sizeof(data50), mimeVideoMpeg},
    {data51, sizeof(data51), mimeVideoMpeg},
    {data52, sizeof(data52), mimeAppOctetStream},
    {data53, sizeof(data53), mimeAppOctetStream},
    {data54, sizeof(data54), mimeTextHtml},
    {data55, sizeof(data55), mimeAppXGzip},
    {data56, sizeof(data56), mimeTextPlain},
    {data57, sizeof(data57), mimeTextHtml},
    {data58, sizeof(data58), mimeAppOctetStream},
    {data59, sizeof(data59), mimeAppXZip},
    {data60, sizeof(data60), mimeTextPlain},
    {data61, sizeof(data61), mimeTextHtml},
    {data62, sizeof(data62), mimeAppJava},
    {data63, sizeof(data63), mimeTextPlain},
    {data64, sizeof(data64), mimeTextHtml},
    {data65, sizeof(data65), mimeAppPdf},
    {data66, sizeof(data66), mimeTextPlain},
    {data67, sizeof(data67), mimeTextHtml},
    {data68, sizeof(data68), mimeAppXMSDownload},
    {data69, sizeof(data69), mimeTextPlain},
    {data70, sizeof(data70), mimeTextHtml},
    {data71, sizeof(data71), mimeTextRichtext},
    {data72, sizeof(data72), mimeTextPlain},
    {data73, sizeof(data73), mimeTextPlain},
    {data74, sizeof(data74), mimeTextHtml},
    {data75, sizeof(data75), mimeAudioWav},
    {data76, sizeof(data76), mimeTextPlain},
    {data77, sizeof(data77), mimeTextPlain},
    {data78, sizeof(data78), mimeTextHtml},
    {data79, sizeof(data79), mimeAppPostscript},
    {data80, sizeof(data80), mimeTextPlain},
    {data81, sizeof(data81), mimeTextHtml},
    {data82, sizeof(data82), mimeAudioBasic},
    {data83, sizeof(data83), mimeTextPlain},
    {data84, sizeof(data84), mimeTextHtml},
    {data85, sizeof(data85), mimeTextPlain}
};

static void test_FindMimeFromData(void)
{
    HRESULT hres;
    LPWSTR mime;
    int i;

    for(i=0; i<sizeof(mime_tests)/sizeof(mime_tests[0]); i++) {
        mime = (LPWSTR)0xf0f0f0f0;
        hres = FindMimeFromData(NULL, mime_tests[i].url, NULL, 0, NULL, 0, &mime, 0);
        if(mime_tests[i].mime) {
            ok(hres == S_OK, "[%d] FindMimeFromData failed: %08x\n", i, hres);
            ok(!lstrcmpW(mime, mime_tests[i].mime), "[%d] wrong mime\n", i);
            CoTaskMemFree(mime);
        }else {
            ok(hres == E_FAIL || hres == mime_tests[i].hres,
               "[%d] FindMimeFromData failed: %08x, expected %08x\n",
               i, hres, mime_tests[i].hres);
            ok(mime == (LPWSTR)0xf0f0f0f0, "[%d] mime != 0xf0f0f0f0\n", i);
        }

        mime = (LPWSTR)0xf0f0f0f0;
        hres = FindMimeFromData(NULL, mime_tests[i].url, NULL, 0, mimeTextPlain, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08x\n", i, hres);
        ok(!lstrcmpW(mime, mimeTextPlain), "[%d] wrong mime\n", i);
        CoTaskMemFree(mime);

        mime = (LPWSTR)0xf0f0f0f0;
        hres = FindMimeFromData(NULL, mime_tests[i].url, NULL, 0, mimeAppOctetStream, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08x\n", i, hres);
        ok(!lstrcmpW(mime, mimeAppOctetStream), "[%d] wrong mime\n", i);
        CoTaskMemFree(mime);
    }

    for(i=0; i < sizeof(mime_tests2)/sizeof(mime_tests2[0]); i++) {
        hres = FindMimeFromData(NULL, NULL, mime_tests2[i].data, mime_tests2[i].size,
                NULL, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08x\n", i, hres);
        ok(!lstrcmpW(mime, mime_tests2[i].mime), "[%d] wrong mime: %s\n", i, debugstr_w(mime));
        CoTaskMemFree(mime);

        hres = FindMimeFromData(NULL, NULL, mime_tests2[i].data, mime_tests2[i].size,
                mimeTextHtml, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08x\n", i, hres);
        if(!lstrcmpW(mimeAppOctetStream, mime_tests2[i].mime)
           || !lstrcmpW(mimeTextPlain, mime_tests2[i].mime))
            ok(!lstrcmpW(mime, mimeTextHtml), "[%d] wrong mime\n", i);
        else
            ok(!lstrcmpW(mime, mime_tests2[i].mime), "[%d] wrong mime\n", i);
        CoTaskMemFree(mime);

        hres = FindMimeFromData(NULL, NULL, mime_tests2[i].data, mime_tests2[i].size,
                mimeImagePjpeg, 0, &mime, 0);
        ok(hres == S_OK, "[%d] FindMimeFromData failed: %08x\n", i, hres);
        if(!lstrcmpW(mimeAppOctetStream, mime_tests2[i].mime) || i == 17)
            ok(!lstrcmpW(mime, mimeImagePjpeg), "[%d] wrong mime\n", i);
        else
            ok(!lstrcmpW(mime, mime_tests2[i].mime), "[%d] wrong mime\n", i);

        CoTaskMemFree(mime);
    }

    hres = FindMimeFromData(NULL, url1, data1, sizeof(data1), NULL, 0, &mime, 0);
    ok(hres == S_OK, "FindMimeFromData failed: %08x\n", hres);
    ok(!lstrcmpW(mime, mimeTextPlain), "wrong mime\n");
    CoTaskMemFree(mime);

    hres = FindMimeFromData(NULL, url1, data1, sizeof(data1), mimeAppOctetStream, 0, &mime, 0);
    ok(hres == S_OK, "FindMimeFromData failed: %08x\n", hres);
    ok(!lstrcmpW(mime, mimeTextPlain), "wrong mime\n");
    CoTaskMemFree(mime);

    hres = FindMimeFromData(NULL, url4, data1, sizeof(data1), mimeAppOctetStream, 0, &mime, 0);
    ok(hres == S_OK, "FindMimeFromData failed: %08x\n", hres);
    ok(!lstrcmpW(mime, mimeTextPlain), "wrong mime\n");
    CoTaskMemFree(mime);

    hres = FindMimeFromData(NULL, NULL, NULL, 0, NULL, 0, &mime, 0);
    ok(hres == E_INVALIDARG, "FindMimeFromData failed: %08x, excepted E_INVALIDARG\n", hres);

    hres = FindMimeFromData(NULL, NULL, NULL, 0, mimeTextPlain, 0, &mime, 0);
    ok(hres == E_INVALIDARG, "FindMimeFromData failed: %08x, expected E_INVALIDARG\n", hres);

    hres = FindMimeFromData(NULL, NULL, data1, 0, NULL, 0, &mime, 0);
    ok(hres == E_FAIL, "FindMimeFromData failed: %08x, expected E_FAIL\n", hres);

    hres = FindMimeFromData(NULL, url1, data1, 0, NULL, 0, &mime, 0);
    ok(hres == E_FAIL, "FindMimeFromData failed: %08x, expected E_FAIL\n", hres);

    hres = FindMimeFromData(NULL, NULL, data1, 0, mimeTextPlain, 0, &mime, 0);
    ok(hres == S_OK, "FindMimeFromData failed: %08x\n", hres);
    ok(!lstrcmpW(mime, mimeTextPlain), "wrong mime\n");
    CoTaskMemFree(mime);

    hres = FindMimeFromData(NULL, NULL, data1, 0, mimeTextPlain, 0, NULL, 0);
    ok(hres == E_INVALIDARG, "FindMimeFromData failed: %08x, expected E_INVALIDARG\n", hres);
}

static const BYTE secid1[] = {'f','i','l','e',':',0,0,0,0};
static const BYTE secid5[] = {'h','t','t','p',':','w','w','w','.','w','i','n','e','h','q',
    '.','o','r','g',3,0,0,0};
static const BYTE secid6[] = {'a','b','o','u','t',':','b','l','a','n','k',3,0,0,0};
static const BYTE secid7[] = {'f','t','p',':','w','i','n','e','h','q','.','o','r','g',
                              3,0,0,0};
static const BYTE secid10[] =
    {'f','i','l','e',':','s','o','m','e','%','2','0','f','i','l','e','.','j','p','g',3,0,0,0};
static const BYTE secid10_2[] =
    {'f','i','l','e',':','s','o','m','e',' ','f','i','l','e','.','j','p','g',3,0,0,0};

static struct secmgr_test {
    LPCWSTR url;
    DWORD zone;
    HRESULT zone_hres;
    DWORD secid_size;
    const BYTE *secid;
    HRESULT secid_hres;
} secmgr_tests[] = {
    {url1, 0,   S_OK, sizeof(secid1), secid1, S_OK},
    {url2, 100, 0x80041001, 0, NULL, E_INVALIDARG},
    {url3, 0,   S_OK, sizeof(secid1), secid1, S_OK},
    {url5, 3,   S_OK, sizeof(secid5), secid5, S_OK},
    {url6, 3,   S_OK, sizeof(secid6), secid6, S_OK},
    {url7, 3,   S_OK, sizeof(secid7), secid7, S_OK}
};

static void test_SecurityManager(void)
{
    int i;
    IInternetSecurityManager *secmgr = NULL;
    BYTE buf[512];
    DWORD zone, size, policy;
    HRESULT hres;

    hres = CoInternetCreateSecurityManager(NULL, &secmgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    for(i=0; i < sizeof(secmgr_tests)/sizeof(secmgr_tests[0]); i++) {
        zone = 100;
        hres = IInternetSecurityManager_MapUrlToZone(secmgr, secmgr_tests[i].url,
                                                     &zone, 0);
        ok(hres == secmgr_tests[i].zone_hres /* IE <=6 */
           || (FAILED(secmgr_tests[i].zone_hres) && hres == E_INVALIDARG), /* IE7 */
           "[%d] MapUrlToZone failed: %08x, expected %08x\n",
                i, hres, secmgr_tests[i].zone_hres);
        if(SUCCEEDED(hres))
            ok(zone == secmgr_tests[i].zone, "[%d] zone=%d, expected %d\n", i, zone,
               secmgr_tests[i].zone);
        else
            ok(zone == secmgr_tests[i].zone || zone == -1, "[%d] zone=%d\n", i, zone);

        size = sizeof(buf);
        memset(buf, 0xf0, sizeof(buf));
        hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[i].url,
                buf, &size, 0);
        ok(hres == secmgr_tests[i].secid_hres,
           "[%d] GetSecurityId failed: %08x, expected %08x\n",
           i, hres, secmgr_tests[i].secid_hres);
        if(secmgr_tests[i].secid) {
            ok(size == secmgr_tests[i].secid_size, "[%d] size=%d, expected %d\n",
                    i, size, secmgr_tests[i].secid_size);
            ok(!memcmp(buf, secmgr_tests[i].secid, size), "[%d] wrong secid\n", i);
        }
    }

    zone = 100;
    hres = IInternetSecurityManager_MapUrlToZone(secmgr, url10, &zone, 0);
    ok(hres == S_OK, "MapUrlToZone failed: %08x, expected S_OK\n", hres);
    ok(zone == 3, "zone=%d, expected 3\n", zone);

    /* win2k3 translates %20 into a space */
    size = sizeof(buf);
    memset(buf, 0xf0, sizeof(buf));
    hres = IInternetSecurityManager_GetSecurityId(secmgr, url10, buf, &size, 0);
    ok(hres == S_OK, "GetSecurityId failed: %08x, expected S_OK\n", hres);
    ok(size == sizeof(secid10) ||
       size == sizeof(secid10_2), /* win2k3 */
       "size=%d\n", size);
    ok(!memcmp(buf, secid10, size) ||
       !memcmp(buf, secid10_2, size), /* win2k3 */
       "wrong secid\n");

    zone = 100;
    hres = IInternetSecurityManager_MapUrlToZone(secmgr, NULL, &zone, 0);
    ok(hres == E_INVALIDARG, "MapUrlToZone failed: %08x, expected E_INVALIDARG\n", hres);
    ok(zone == 100 || zone == -1, "zone=%d\n", zone);

    size = sizeof(buf);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, NULL, buf, &size, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08x, expected E_INVALIDARG\n", hres);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[1].url,
                                                  NULL, &size, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08x, expected E_INVALIDARG\n", hres);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[1].url,
                                                  buf, NULL, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08x, expected E_INVALIDARG\n", hres);

    hres = IInternetSecurityManager_ProcessUrlAction(secmgr, NULL, URLACTION_SCRIPT_RUN, (BYTE*)&policy,
            sizeof(WCHAR), NULL, 0, 0, 0);
    ok(hres == E_INVALIDARG, "ProcessUrlAction failed: %08x, expected E_INVALIDARG\n", hres);

    IInternetSecurityManager_Release(secmgr);
}

/* Check if Internet Explorer is configured to run in "Enhanced Security Configuration" (aka hardened mode) */
/* Note: this code is duplicated in dlls/mshtml/tests/dom.c, dlls/mshtml/tests/script.c and dlls/urlmon/tests/misc.c */
static BOOL is_ie_hardened(void)
{
    HKEY zone_map;
    DWORD ie_harden, type, size;

    ie_harden = 0;
    if(RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap",
                    0, KEY_QUERY_VALUE, &zone_map) == ERROR_SUCCESS) {
        size = sizeof(DWORD);
        if (RegQueryValueEx(zone_map, "IEHarden", NULL, &type, (LPBYTE) &ie_harden, &size) != ERROR_SUCCESS ||
            type != REG_DWORD) {
            ie_harden = 0;
        }
    RegCloseKey(zone_map);
    }

    return ie_harden != 0;
}

static void test_url_action(IInternetSecurityManager *secmgr, IInternetZoneManager *zonemgr, DWORD action)
{
    DWORD res, size, policy, reg_policy;
    char buf[10];
    HKEY hkey;
    HRESULT hres;

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\3", &hkey);
    if(res != ERROR_SUCCESS) {
        ok(0, "Could not open zone key\n");
        return;
    }

    wsprintf(buf, "%X", action);
    size = sizeof(DWORD);
    res = RegQueryValueExA(hkey, buf, NULL, NULL, (BYTE*)&reg_policy, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || size != sizeof(DWORD)) {
        policy = 0xdeadbeef;
        hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                sizeof(WCHAR), NULL, 0, 0, 0);
        ok(hres == E_FAIL, "ProcessUrlAction(%x) failed: %08x, expected E_FAIL\n", action, hres);
        ok(policy == 0xdeadbeef, "(%x) policy=%x\n", action, policy);

        policy = 0xdeadbeef;
        hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, (BYTE*)&policy,
                sizeof(DWORD), URLZONEREG_DEFAULT);
        ok(hres == E_FAIL, "GetZoneActionPolicy failed: %08x, expected E_FAIL\n", hres);
        ok(policy == 0xdeadbeef, "(%x) policy=%x\n", action, policy);
        return;
    }

    policy = 0xdeadbeef;
    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, (BYTE*)&policy,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08x\n", hres);
    ok(policy == reg_policy, "(%x) policy=%x, expected %x\n", action, policy, reg_policy);

    if(policy != URLPOLICY_QUERY) {
        if(winetest_interactive || ! is_ie_hardened()) {
            policy = 0xdeadbeef;
            hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                    sizeof(WCHAR), NULL, 0, 0, 0);
            if(reg_policy == URLPOLICY_DISALLOW)
                ok(hres == S_FALSE, "ProcessUrlAction(%x) failed: %08x, expected S_FALSE\n", action, hres);
            else
                ok(hres == S_OK, "ProcessUrlAction(%x) failed: %08x\n", action, hres);
            ok(policy == 0xdeadbeef, "(%x) policy=%x\n", action, policy);
        }else {
            skip("IE running in Enhanced Security Configuration\n");
        }
	}
}

static void test_special_url_action(IInternetSecurityManager *secmgr, IInternetZoneManager *zonemgr, DWORD action)
{
    DWORD policy;
    HRESULT hres;

    policy = 0xdeadbeef;
    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, (BYTE*)&policy,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08x\n", hres);
    ok(policy == URLPOLICY_DISALLOW, "(%x) policy=%x, expected URLPOLIVY_DISALLOW\n", action, policy);

    policy = 0xdeadbeef;
    hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url1, action, (BYTE*)&policy,
            sizeof(WCHAR), NULL, 0, 0, 0);
    ok(hres == S_FALSE, "ProcessUrlAction(%x) failed: %08x, expected S_FALSE\n", action, hres);
}

static void test_polices(void)
{
    IInternetZoneManager *zonemgr = NULL;
    IInternetSecurityManager *secmgr = NULL;
    HRESULT hres;

    hres = CoInternetCreateSecurityManager(NULL, &secmgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08x\n", hres);
    hres = CoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hres == S_OK, "CoInternetCreateZoneManager failed: %08x\n", hres);

    test_url_action(secmgr, zonemgr, URLACTION_SCRIPT_RUN);
    test_url_action(secmgr, zonemgr, URLACTION_ACTIVEX_OVERRIDE_OBJECT_SAFETY);
    test_url_action(secmgr, zonemgr, URLACTION_CHANNEL_SOFTDIST_PERMISSIONS);
    test_url_action(secmgr, zonemgr, 0xdeadbeef);

    test_special_url_action(secmgr, zonemgr, URLACTION_SCRIPT_OVERRIDE_SAFETY);

    IInternetSecurityManager_Release(secmgr);
    IInternetZoneManager_Release(zonemgr);
}

static void test_ZoneManager(void)
{
    IInternetZoneManager *zonemgr = NULL;
    BYTE buf[32];
    HRESULT hres;

    hres = CoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hres == S_OK, "CoInternetCreateZoneManager failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, 0x1a10, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08x\n", hres);
    ok(*(DWORD*)buf == 1, "policy=%d, expected 1\n", *(DWORD*)buf);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, 0x1a10, NULL,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08x, expected E_INVALIDARG\n", hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, 0x1a10, buf,
            2, URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08x, expected E_INVALIDARG\n", hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, 0x1fff, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_FAIL, "GetZoneActionPolicy failed: %08x, expected E_FAIL\n", hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 13, 0x1a10, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08x, expected E_INVALIDARG\n", hres);

    IInternetZoneManager_Release(zonemgr);
}

static void register_protocols(void)
{
    IInternetSession *session;
    IClassFactory *factory;
    HRESULT hres;

    static const WCHAR wszAbout[] = {'a','b','o','u','t',0};

    hres = CoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = CoGetClassObject(&CLSID_AboutProtocol, CLSCTX_INPROC_SERVER, NULL,
            &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Coud not get AboutProtocol factory: %08x\n", hres);
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
    CHECK_EXPECT(ParseUrl);
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
        CHECK_EXPECT(QI_IInternetProtocolInfo);
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
    DWORD size;
    HRESULT hres;

    static const WCHAR wszTest[] = {'t','e','s','t',0};

    hres = CoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetSession_RegisterNameSpace(session, NULL, &IID_NULL,
                                              wszTest, 0, NULL, 0);
    ok(hres == E_INVALIDARG, "RegisterNameSpace failed: %08x\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &test_protocol_cf, &IID_NULL,
                                              NULL, 0, NULL, 0);
    ok(hres == E_INVALIDARG, "RegisterNameSpace failed: %08x\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &test_protocol_cf, &IID_NULL,
                                              wszTest, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08x\n", hres);

    qiret = E_NOINTERFACE;
    expect_cf = &test_protocol_cf;
    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(ParseUrl);

    hres = CoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, sizeof(buf)/sizeof(WCHAR),
                              &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08x\n", hres);

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(ParseUrl);

    qiret = S_OK;
    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(ParseUrl);

    hres = CoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, sizeof(buf)/sizeof(WCHAR),
                              &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08x\n", hres);

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(ParseUrl);

    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08x\n", hres);

    hres = CoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, sizeof(buf)/sizeof(WCHAR),
                              &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08x\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &test_protocol_cf2, &IID_NULL,
                                              wszTest, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08x\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &test_protocol_cf, &IID_NULL,
                                              wszTest, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08x\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &test_protocol_cf, &IID_NULL,
                                              wszTest, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08x\n", hres);

    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(ParseUrl);

    hres = CoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, sizeof(buf)/sizeof(WCHAR),
                              &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08x\n", hres);

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(ParseUrl);

    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08x\n", hres);

    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(ParseUrl);

    hres = CoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, sizeof(buf)/sizeof(WCHAR),
                              &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08x\n", hres);

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(ParseUrl);

    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08x\n", hres);

    expect_cf = &test_protocol_cf2;
    SET_EXPECT(QI_IInternetProtocolInfo);
    SET_EXPECT(ParseUrl);

    hres = CoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, sizeof(buf)/sizeof(WCHAR),
                              &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08x\n", hres);

    CHECK_CALLED(QI_IInternetProtocolInfo);
    CHECK_CALLED(ParseUrl);

    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08x\n", hres);
    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08x\n", hres);
    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf, NULL);
    ok(hres == E_INVALIDARG, "UnregisterNameSpace failed: %08x\n", hres);
    hres = IInternetSession_UnregisterNameSpace(session, NULL, wszTest);
    ok(hres == E_INVALIDARG, "UnregisterNameSpace failed: %08x\n", hres);

    hres = IInternetSession_UnregisterNameSpace(session, &test_protocol_cf2, wszTest);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08x\n", hres);

    hres = CoInternetParseUrl(url8, PARSE_ENCODE, 0, buf, sizeof(buf)/sizeof(WCHAR),
                              &size, 0);
    ok(hres == S_OK, "CoInternetParseUrl failed: %08x\n", hres);

    IInternetSession_Release(session);
}

static void test_MimeFilter(void)
{
    IInternetSession *session;
    HRESULT hres;

    static const WCHAR mimeW[] = {'t','e','s','t','/','m','i','m','e',0};

    hres = CoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetSession_RegisterMimeFilter(session, &test_cf, &IID_NULL, mimeW);
    ok(hres == S_OK, "RegisterMimeFilter failed: %08x\n", hres);

    hres = IInternetSession_UnregisterMimeFilter(session, &test_cf, mimeW);
    ok(hres == S_OK, "UnregisterMimeFilter failed: %08x\n", hres);

    hres = IInternetSession_UnregisterMimeFilter(session, &test_cf, mimeW);
    ok(hres == S_OK, "UnregisterMimeFilter failed: %08x\n", hres);

    hres = IInternetSession_UnregisterMimeFilter(session, (void*)0xdeadbeef, mimeW);
    ok(hres == S_OK, "UnregisterMimeFilter failed: %08x\n", hres);

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

    ReleaseBindInfo(NULL); /* shouldn't crash */

    memset(&bi, 0, sizeof(bi));
    bi.cbSize = sizeof(BINDINFO);
    bi.pUnk = &unk;
    SET_EXPECT(unk_Release);
    ReleaseBindInfo(&bi);
    ok(bi.cbSize == sizeof(BINDINFO), "bi.cbSize=%d\n", bi.cbSize);
    ok(bi.pUnk == NULL, "bi.pUnk=%p, expected NULL\n", bi.pUnk);
    CHECK_CALLED(unk_Release);

    memset(&bi, 0, sizeof(bi));
    bi.cbSize = offsetof(BINDINFO, pUnk);
    bi.pUnk = &unk;
    ReleaseBindInfo(&bi);
    ok(bi.cbSize == offsetof(BINDINFO, pUnk), "bi.cbSize=%d\n", bi.cbSize);
    ok(bi.pUnk == &unk, "bi.pUnk=%p, expected %p\n", bi.pUnk, &unk);

    memset(&bi, 0, sizeof(bi));
    bi.pUnk = &unk;
    ReleaseBindInfo(&bi);
    ok(!bi.cbSize, "bi.cbSize=%d, expected 0\n", bi.cbSize);
    ok(bi.pUnk == &unk, "bi.pUnk=%p, expected %p\n", bi.pUnk, &unk);
}

static void test_CopyStgMedium(void)
{
    STGMEDIUM src, dst;
    HGLOBAL empty;
    HRESULT hres;

    static WCHAR fileW[] = {'f','i','l','e',0};

    memset(&src, 0xf0, sizeof(src));
    memset(&dst, 0xe0, sizeof(dst));
    memset(&empty, 0xf0, sizeof(empty));
    src.tymed = TYMED_NULL;
    src.pUnkForRelease = NULL;
    hres = CopyStgMedium(&src, &dst);
    ok(hres == S_OK, "CopyStgMedium failed: %08x\n", hres);
    ok(dst.tymed == TYMED_NULL, "tymed=%d\n", dst.tymed);
    ok(dst.u.hGlobal == empty, "u=%p\n", dst.u.hGlobal);
    ok(!dst.pUnkForRelease, "pUnkForRelease=%p, expected NULL\n", dst.pUnkForRelease);

    memset(&dst, 0xe0, sizeof(dst));
    src.tymed = TYMED_ISTREAM;
    src.u.pstm = NULL;
    src.pUnkForRelease = NULL;
    hres = CopyStgMedium(&src, &dst);
    ok(hres == S_OK, "CopyStgMedium failed: %08x\n", hres);
    ok(dst.tymed == TYMED_ISTREAM, "tymed=%d\n", dst.tymed);
    ok(!dst.u.pstm, "pstm=%p\n", dst.u.pstm);
    ok(!dst.pUnkForRelease, "pUnkForRelease=%p, expected NULL\n", dst.pUnkForRelease);

    memset(&dst, 0xe0, sizeof(dst));
    src.tymed = TYMED_FILE;
    src.u.lpszFileName = fileW;
    src.pUnkForRelease = NULL;
    hres = CopyStgMedium(&src, &dst);
    ok(hres == S_OK, "CopyStgMedium failed: %08x\n", hres);
    ok(dst.tymed == TYMED_FILE, "tymed=%d\n", dst.tymed);
    ok(dst.u.lpszFileName && dst.u.lpszFileName != fileW, "lpszFileName=%p\n", dst.u.lpszFileName);
    ok(!lstrcmpW(dst.u.lpszFileName, fileW), "wrong file name\n");
    ok(!dst.pUnkForRelease, "pUnkForRelease=%p, expected NULL\n", dst.pUnkForRelease);

    hres = CopyStgMedium(&src, NULL);
    ok(hres == E_POINTER, "CopyStgMedium failed: %08x, expected E_POINTER\n", hres);
    hres = CopyStgMedium(NULL, &dst);
    ok(hres == E_POINTER, "CopyStgMedium failed: %08x, expected E_POINTER\n", hres);
}

static void test_UrlMkGetSessionOption(void)
{
    DWORD encoding, size;
    HRESULT hres;

    size = encoding = 0xdeadbeef;
    hres = UrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, &encoding,
                                 sizeof(encoding), &size, 0);
    ok(hres == S_OK, "UrlMkGetSessionOption failed: %08x\n", hres);
    ok(encoding != 0xdeadbeef, "encoding not changed\n");
    ok(size == sizeof(encoding), "size=%d\n", size);

    size = encoding = 0xdeadbeef;
    hres = UrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, &encoding,
                                 sizeof(encoding)+1, &size, 0);
    ok(hres == S_OK, "UrlMkGetSessionOption failed: %08x\n", hres);
    ok(encoding != 0xdeadbeef, "encoding not changed\n");
    ok(size == sizeof(encoding), "size=%d\n", size);

    size = encoding = 0xdeadbeef;
    hres = UrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, &encoding,
                                 sizeof(encoding)-1, &size, 0);
    ok(hres == E_INVALIDARG, "UrlMkGetSessionOption failed: %08x\n", hres);
    ok(encoding == 0xdeadbeef, "encoding = %08x, exepcted 0xdeadbeef\n", encoding);
    ok(size == 0xdeadbeef, "size=%d\n", size);

    size = encoding = 0xdeadbeef;
    hres = UrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, NULL,
                                 sizeof(encoding)-1, &size, 0);
    ok(hres == E_INVALIDARG, "UrlMkGetSessionOption failed: %08x\n", hres);
    ok(encoding == 0xdeadbeef, "encoding = %08x, exepcted 0xdeadbeef\n", encoding);
    ok(size == 0xdeadbeef, "size=%d\n", size);

    encoding = 0xdeadbeef;
    hres = UrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, &encoding,
                                 sizeof(encoding)-1, NULL, 0);
    ok(hres == E_INVALIDARG, "UrlMkGetSessionOption failed: %08x\n", hres);
    ok(encoding == 0xdeadbeef, "encoding = %08x, exepcted 0xdeadbeef\n", encoding);
}

static void test_ObtainUserAgentString(void)
{
    static const CHAR expected[] = "Mozilla/4.0 (compatible; MSIE ";
    static CHAR str[3];
    LPSTR str2 = NULL;
    HRESULT hres;
    DWORD size, saved;

    hres = ObtainUserAgentString(0, NULL, NULL);
    ok(hres == E_INVALIDARG, "ObtainUserAgentString failed: %08x\n", hres);

    size = 100;
    hres = ObtainUserAgentString(0, NULL, &size);
    ok(hres == E_INVALIDARG, "ObtainUserAgentString failed: %08x\n", hres);
    ok(size == 100, "size=%d, expected %d\n", size, 100);

    size = 0;
    hres = ObtainUserAgentString(0, str, &size);
    ok(hres == E_OUTOFMEMORY, "ObtainUserAgentString failed: %08x\n", hres);
    ok(size > 0, "size=%d, expected non-zero\n", size);

    size = 2;
    str[0] = 'a';
    hres = ObtainUserAgentString(0, str, &size);
    ok(hres == E_OUTOFMEMORY, "ObtainUserAgentString failed: %08x\n", hres);
    ok(size > 0, "size=%d, expected non-zero\n", size);
    ok(str[0] == 'a', "str[0]=%c, expected 'a'\n", str[0]);

    size = 0;
    hres = ObtainUserAgentString(1, str, &size);
    ok(hres == E_OUTOFMEMORY, "ObtainUserAgentString failed: %08x\n", hres);
    ok(size > 0, "size=%d, expected non-zero\n", size);

    str2 = HeapAlloc(GetProcessHeap(), 0, (size+20)*sizeof(CHAR));
    if (!str2)
    {
        skip("skipping rest of ObtainUserAgent tests, out of memory\n");
    }
    else
    {
        saved = size;
        hres = ObtainUserAgentString(0, str2, &size);
        ok(hres == S_OK, "ObtainUserAgentString failed: %08x\n", hres);
        ok(size == saved, "size=%d, expected %d\n", size, saved);
        ok(strlen(expected) <= strlen(str2) &&
           !memcmp(expected, str2, strlen(expected)*sizeof(CHAR)),
           "user agent was \"%s\", expected to start with \"%s\"\n",
           str2, expected);

        size = saved+10;
        hres = ObtainUserAgentString(0, str2, &size);
        ok(hres == S_OK, "ObtainUserAgentString failed: %08x\n", hres);
        ok(size == saved, "size=%d, expected %d\n", size, saved);
    }
    HeapFree(GetProcessHeap(), 0, str2);
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

    CreateBindCtx(0, &bctx);

    hres = MkParseDisplayNameEx(bctx, url9, &eaten, &mon);
    ok(hres == S_OK, "MkParseDisplayNameEx failed: %08x\n", hres);
    ok(eaten == sizeof(url9)/sizeof(WCHAR)-1, "eaten=%d\n", eaten);
    ok(mon != NULL, "mon == NULL\n");

    hres = IMoniker_GetDisplayName(mon, NULL, 0, &name);
    ok(hres == S_OK, "GetDiasplayName failed: %08x\n", hres);
    ok(!lstrcmpW(name, url9), "wrong display name %s\n", debugstr_w(name));
    CoTaskMemFree(name);

    hres = IMoniker_IsSystemMoniker(mon, &issys);
    ok(hres == S_OK, "IsSystemMoniker failed: %08x\n", hres);
    ok(issys == MKSYS_URLMONIKER, "issys=%x\n", issys);

    IMoniker_Release(mon);

    hres = MkParseDisplayNameEx(bctx, clsid_nameW, &eaten, &mon);
    ok(hres == S_OK, "MkParseDisplayNameEx failed: %08x\n", hres);
    ok(eaten == sizeof(clsid_nameW)/sizeof(WCHAR)-1, "eaten=%d\n", eaten);
    ok(mon != NULL, "mon == NULL\n");

    hres = IMoniker_IsSystemMoniker(mon, &issys);
    ok(hres == S_OK, "IsSystemMoniker failed: %08x\n", hres);
    ok(issys == MKSYS_CLASSMONIKER, "issys=%x\n", issys);

    IMoniker_Release(mon);

    hres = MkParseDisplayNameEx(bctx, url8, &eaten, &mon);
    ok(FAILED(hres), "MkParseDisplayNameEx succeeded: %08x\n", hres);

    IBindCtx_Release(bctx);
}

START_TEST(misc)
{
    OleInitialize(NULL);

    register_protocols();

    test_CreateFormatEnum();
    test_RegisterFormatEnumerator();
    test_CoInternetParseUrl();
    test_CoInternetCompareUrl();
    test_CoInternetQueryInfo();
    test_FindMimeFromData();
    test_SecurityManager();
    test_polices();
    test_ZoneManager();
    test_NameSpace();
    test_MimeFilter();
    test_ReleaseBindInfo();
    test_CopyStgMedium();
    test_UrlMkGetSessionOption();
    test_ObtainUserAgentString();
    test_MkParseDisplayNameEx();

    OleUninitialize();
}
