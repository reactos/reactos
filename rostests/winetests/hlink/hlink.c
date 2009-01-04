/*
 * Implementation of hyperlinking (hlink.dll)
 *
 * Copyright 2006 Mike McCormack
 * Copyright 2007-2008 Jacek Caban for CodeWeavers
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

#include <stdio.h>

#include <initguid.h>
#include <hlink.h>
#include <hlguids.h>

#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(IsSystemMoniker);
DEFINE_EXPECT(BindToStorage);
DEFINE_EXPECT(GetDisplayName);

static const char *debugstr_w(LPCWSTR str)
{
    static char buf[1024];
    if(!str)
        return "(null)";
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
    return buf;
}

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static void test_HlinkIsShortcut(void)
{
    int i;
    HRESULT hres;

    static const WCHAR file0[] = {'f','i','l','e',0};
    static const WCHAR file1[] = {'f','i','l','e','.','u','r','l',0};
    static const WCHAR file2[] = {'f','i','l','e','.','l','n','k',0};
    static const WCHAR file3[] = {'f','i','l','e','.','u','R','l',0};
    static const WCHAR file4[] = {'f','i','l','e','u','r','l',0};
    static const WCHAR file5[] = {'c',':','\\','f','i','l','e','.','u','r','l',0};
    static const WCHAR file6[] = {'c',':','\\','f','i','l','e','.','l','n','k',0};
    static const WCHAR file7[] = {'.','u','r','l',0};

    static struct {
        LPCWSTR file;
        HRESULT hres;
    } shortcut_test[] = {
        {file0, S_FALSE},
        {file1, S_OK},
        {file2, S_FALSE},
        {file3, S_OK},
        {file4, S_FALSE},
        {file5, S_OK},
        {file6, S_FALSE},
        {file7, S_OK},
        {NULL,  E_INVALIDARG}
    };

    for(i=0; i<sizeof(shortcut_test)/sizeof(shortcut_test[0]); i++) {
        hres = HlinkIsShortcut(shortcut_test[i].file);
        ok(hres == shortcut_test[i].hres, "[%d] HlinkIsShortcut returned %08x, expected %08x\n",
           i, hres, shortcut_test[i].hres);
    }
}

static void test_reference(void)
{
    HRESULT r;
    IHlink *lnk = NULL;
    IMoniker *mk = NULL;
    const WCHAR url[] = { 'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g',0 };
    const WCHAR url2[] = { 'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g','/',0 };
    LPWSTR str = NULL;

    r = HlinkCreateFromString(url, NULL, NULL, NULL,
                              0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(r == S_OK, "failed to create link\n");
    if (FAILED(r))
        return;

    r = IHlink_GetMonikerReference(lnk, HLINKGETREF_DEFAULT, NULL, NULL);
    ok(r == S_OK, "failed\n");

    r = IHlink_GetMonikerReference(lnk, HLINKGETREF_DEFAULT, &mk, &str);
    ok(r == S_OK, "failed\n");
    ok(mk != NULL, "no moniker\n");
    ok(str == NULL, "string should be null\n");

    r = IMoniker_Release(mk);
    ok( r == 1, "moniker refcount wrong\n");

    r = IHlink_GetStringReference(lnk, -1, &str, NULL);
    ok(r == S_OK, "failed\n");
    CoTaskMemFree(str);

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, &str, NULL);
    ok(r == S_OK, "failed\n");
    todo_wine {
    ok(!lstrcmpW(str, url2), "url wrong\n");
    }
    CoTaskMemFree(str);

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, NULL, NULL);
    ok(r == S_OK, "failed\n");

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, NULL, &str);
    ok(r == S_OK, "failed\n");
    ok(str == NULL, "string should be null\n");

    /* Unimplented functions checks */
    r = IHlink_GetAdditionalParams(lnk, NULL);
    ok(r == E_NOTIMPL, "failed\n");

    r = IHlink_SetAdditionalParams(lnk, NULL);
    ok(r == E_NOTIMPL, "failed\n");

    IHlink_Release(lnk);
}

/* url only */
static const unsigned char expected_hlink_data[] =
{
    0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0xe0,0xc9,0xea,0x79,0xf9,0xba,0xce,0x11,
    0x8c,0x82,0x00,0xaa,0x00,0x4b,0xa9,0x0b,
    0x26,0x00,0x00,0x00,0x68,0x00,0x74,0x00,
    0x74,0x00,0x70,0x00,0x3a,0x00,0x2f,0x00,
    0x2f,0x00,0x77,0x00,0x69,0x00,0x6e,0x00,
    0x65,0x00,0x68,0x00,0x71,0x00,0x2e,0x00,
    0x6f,0x00,0x72,0x00,0x67,0x00,0x2f,0x00,
    0x00,0x00,
};

/* url only (IE7) */
static const unsigned char expected_hlink_data_ie7[] =
{
    0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0xe0,0xc9,0xea,0x79,0xf9,0xba,0xce,0x11,
    0x8c,0x82,0x00,0xaa,0x00,0x4b,0xa9,0x0b,
    0x3e,0x00,0x00,0x00,0x68,0x00,0x74,0x00,
    0x74,0x00,0x70,0x00,0x3a,0x00,0x2f,0x00,
    0x2f,0x00,0x77,0x00,0x69,0x00,0x6e,0x00,
    0x65,0x00,0x68,0x00,0x71,0x00,0x2e,0x00,
    0x6f,0x00,0x72,0x00,0x67,0x00,0x2f,0x00,
    0x00,0x00,0x79,0x58,0x81,0xf4,0x3b,0x1d,
    0x7f,0x48,0xaf,0x2c,0x82,0x5d,0xc4,0x85,
    0x27,0x63,0x00,0x00,0x00,0x00,0xa5,0xab,
    0x00,0x00,
};

/* url + friendly name */
static const unsigned char expected_hlink_data2[] =
{
    0x02,0x00,0x00,0x00,0x17,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x57,0x00,0x69,0x00,
    0x6e,0x00,0x65,0x00,0x20,0x00,0x48,0x00,
    0x51,0x00,0x00,0x00,0xe0,0xc9,0xea,0x79,
    0xf9,0xba,0xce,0x11,0x8c,0x82,0x00,0xaa,
    0x00,0x4b,0xa9,0x0b,0x26,0x00,0x00,0x00,
    0x68,0x00,0x74,0x00,0x74,0x00,0x70,0x00,
    0x3a,0x00,0x2f,0x00,0x2f,0x00,0x77,0x00,
    0x69,0x00,0x6e,0x00,0x65,0x00,0x68,0x00,
    0x71,0x00,0x2e,0x00,0x6f,0x00,0x72,0x00,
    0x67,0x00,0x2f,0x00,0x00,0x00,
};

/* url + friendly name (IE7) */
static const unsigned char expected_hlink_data2_ie7[] =
{
    0x02,0x00,0x00,0x00,0x17,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x57,0x00,0x69,0x00,
    0x6e,0x00,0x65,0x00,0x20,0x00,0x48,0x00,
    0x51,0x00,0x00,0x00,0xe0,0xc9,0xea,0x79,
    0xf9,0xba,0xce,0x11,0x8c,0x82,0x00,0xaa,
    0x00,0x4b,0xa9,0x0b,0x3e,0x00,0x00,0x00,
    0x68,0x00,0x74,0x00,0x74,0x00,0x70,0x00,
    0x3a,0x00,0x2f,0x00,0x2f,0x00,0x77,0x00,
    0x69,0x00,0x6e,0x00,0x65,0x00,0x68,0x00,
    0x71,0x00,0x2e,0x00,0x6f,0x00,0x72,0x00,
    0x67,0x00,0x2f,0x00,0x00,0x00,0x79,0x58,
    0x81,0xf4,0x3b,0x1d,0x7f,0x48,0xaf,0x2c,
    0x82,0x5d,0xc4,0x85,0x27,0x63,0x00,0x00,
    0x00,0x00,0xa5,0xab,0x00,0x00,
};

/* url + friendly name + location */
static const unsigned char expected_hlink_data3[] =
{
    0x02,0x00,0x00,0x00,0x1f,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x57,0x00,0x69,0x00,
    0x6e,0x00,0x65,0x00,0x20,0x00,0x48,0x00,
    0x51,0x00,0x00,0x00,0xe0,0xc9,0xea,0x79,
    0xf9,0xba,0xce,0x11,0x8c,0x82,0x00,0xaa,
    0x00,0x4b,0xa9,0x0b,0x26,0x00,0x00,0x00,
    0x68,0x00,0x74,0x00,0x74,0x00,0x70,0x00,
    0x3a,0x00,0x2f,0x00,0x2f,0x00,0x77,0x00,
    0x69,0x00,0x6e,0x00,0x65,0x00,0x68,0x00,
    0x71,0x00,0x2e,0x00,0x6f,0x00,0x72,0x00,
    0x67,0x00,0x2f,0x00,0x00,0x00,0x07,0x00,
    0x00,0x00,0x5f,0x00,0x62,0x00,0x6c,0x00,
    0x61,0x00,0x6e,0x00,0x6b,0x00,0x00,0x00,
};

/* url + friendly name + location (IE7) */
static const unsigned char expected_hlink_data3_ie7[] =
{
    0x02,0x00,0x00,0x00,0x1f,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x57,0x00,0x69,0x00,
    0x6e,0x00,0x65,0x00,0x20,0x00,0x48,0x00,
    0x51,0x00,0x00,0x00,0xe0,0xc9,0xea,0x79,
    0xf9,0xba,0xce,0x11,0x8c,0x82,0x00,0xaa,
    0x00,0x4b,0xa9,0x0b,0x3e,0x00,0x00,0x00,
    0x68,0x00,0x74,0x00,0x74,0x00,0x70,0x00,
    0x3a,0x00,0x2f,0x00,0x2f,0x00,0x77,0x00,
    0x69,0x00,0x6e,0x00,0x65,0x00,0x68,0x00,
    0x71,0x00,0x2e,0x00,0x6f,0x00,0x72,0x00,
    0x67,0x00,0x2f,0x00,0x00,0x00,0x79,0x58,
    0x81,0xf4,0x3b,0x1d,0x7f,0x48,0xaf,0x2c,
    0x82,0x5d,0xc4,0x85,0x27,0x63,0x00,0x00,
    0x00,0x00,0xa5,0xab,0x00,0x00,0x07,0x00,
    0x00,0x00,0x5f,0x00,0x62,0x00,0x6c,0x00,
    0x61,0x00,0x6e,0x00,0x6b,0x00,0x00,0x00,
};

/* relative url */
static const unsigned char expected_hlink_data4[] =
{
    0x02,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x0b,0x00,0x00,0x00,0x69,0x6e,
    0x64,0x65,0x78,0x2e,0x68,0x74,0x6d,0x6c,
    0x00,0xff,0xff,0xad,0xde,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,
};

/* url + target frame name */
static const unsigned char expected_hlink_data5[] =
{
    0x02,0x00,0x00,0x00,0x83,0x00,0x00,0x00,
    0x07,0x00,0x00,0x00,0x74,0x00,0x67,0x00,
    0x74,0x00,0x66,0x00,0x72,0x00,0x6d,0x00,
    0x00,0x00,0xe0,0xc9,0xea,0x79,0xf9,0xba,
    0xce,0x11,0x8c,0x82,0x00,0xaa,0x00,0x4b,
    0xa9,0x0b,0x26,0x00,0x00,0x00,0x68,0x00,
    0x74,0x00,0x74,0x00,0x70,0x00,0x3a,0x00,
    0x2f,0x00,0x2f,0x00,0x77,0x00,0x69,0x00,
    0x6e,0x00,0x65,0x00,0x68,0x00,0x71,0x00,
    0x2e,0x00,0x6f,0x00,0x72,0x00,0x67,0x00,
    0x2f,0x00,0x00,0x00,
};

/* url + target frame name (IE7) */
static const unsigned char expected_hlink_data5_ie7[] =
{
    0x02,0x00,0x00,0x00,0x83,0x00,0x00,0x00,
    0x07,0x00,0x00,0x00,0x74,0x00,0x67,0x00,
    0x74,0x00,0x66,0x00,0x72,0x00,0x6d,0x00,
    0x00,0x00,0xe0,0xc9,0xea,0x79,0xf9,0xba,
    0xce,0x11,0x8c,0x82,0x00,0xaa,0x00,0x4b,
    0xa9,0x0b,0x3e,0x00,0x00,0x00,0x68,0x00,
    0x74,0x00,0x74,0x00,0x70,0x00,0x3a,0x00,
    0x2f,0x00,0x2f,0x00,0x77,0x00,0x69,0x00,
    0x6e,0x00,0x65,0x00,0x68,0x00,0x71,0x00,
    0x2e,0x00,0x6f,0x00,0x72,0x00,0x67,0x00,
    0x2f,0x00,0x00,0x00,0x79,0x58,0x81,0xf4,
    0x3b,0x1d,0x7f,0x48,0xaf,0x2c,0x82,0x5d,
    0xc4,0x85,0x27,0x63,0x00,0x00,0x00,0x00,
    0xa5,0xab,0x00,0x00,
};

/* filename */
static const unsigned char expected_hlink_data6[] =
{
     0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
     0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,0x04,0x00,0x00,0x00,0x63,0x3a,
     0x5c,0x00,0xff,0xff,0xad,0xde,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x0c,0x00,0x00,0x00,0x06,0x00,
     0x00,0x00,0x03,0x00,0x63,0x00,0x3a,0x00,
     0x5c,0x00,
};

static void test_persist_save_data(const char *testname, IHlink *lnk,
                                   const unsigned char *expected_data,
                                   unsigned int expected_data_size,
                                   const unsigned char *expected_data_alt,
                                   unsigned int expected_data_alt_size)
{
    HRESULT hr;
    IStream *stream;
    IPersistStream *ps;
    HGLOBAL hglobal;
    DWORD data_size;
    const unsigned char *data;
    DWORD i;
    BOOL same;
    unsigned int expected_data_win9x_size = 0;

    hr = IHlink_QueryInterface(lnk, &IID_IPersistStream, (void **)&ps);
    ok(hr == S_OK, "IHlink_QueryInterface failed with error 0x%08x\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed with error 0x%08x\n", hr);

    hr = IPersistStream_Save(ps, stream, TRUE);
    ok(hr == S_OK, "IPersistStream_Save failed with error 0x%08x\n", hr);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "GetHGlobalFromStream failed with error 0x%08x\n", hr);

    data_size = GlobalSize(hglobal);

    data = GlobalLock(hglobal);

    if (expected_data_size % 4)
        expected_data_win9x_size =  4 * ((expected_data_size / 4) + 1);

    /* first check we have the right amount of data */
    ok((data_size == expected_data_size) ||
       (data_size == expected_data_alt_size) ||
       broken(data_size == expected_data_win9x_size), /* Win9x and WinMe */
       "%s: Size of saved data differs (expected %d or %d, actual %d)\n",
       testname, expected_data_size, expected_data_alt_size, data_size);

    same = TRUE;
    /* then do a byte-by-byte comparison */
    for (i = 0; i < min(data_size, expected_data_size); i++)
    {
        if ((expected_data[i] != data[i]) &&
            (((expected_data != expected_hlink_data2) &&
              (expected_data != expected_hlink_data3)) ||
             ((i < 52 || i >= 56) && (i < 80 || i >= 84))))
        {
            same = FALSE;
            break;
        }
    }

    if (!same && (expected_data_alt != expected_data))
    {
        /* then try the alternate data */
        same = TRUE;
        for (i = 0; i < min(data_size, expected_data_alt_size); i++)
        {
            if ((expected_data_alt[i] != data[i]) &&
                (((expected_data_alt != expected_hlink_data2) &&
                  (expected_data_alt != expected_hlink_data3)) ||
                 ((i < 52 || i >= 56) && (i < 80 || i >= 84))))
            {
                same = FALSE;
                break;
            }
        }
    }

    ok(same, "%s: Saved data differs\n", testname);
    if (!same)
    {
        for (i = 0; i < data_size; i++)
        {
            if (i % 8 == 0) printf("    ");
            printf("0x%02x,", data[i]);
            if (i % 8 == 7) printf("\n");
        }
        printf("\n");
    }

    GlobalUnlock(hglobal);

    IStream_Release(stream);
    IPersistStream_Release(ps);
}

static void test_persist(void)
{
    static const WCHAR url[] = { 'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g',0 };
    static const WCHAR rel_url[] = { 'i','n','d','e','x','.','h','t','m','l',0 };
    static const WCHAR filename[] = { 'c',':','\\',0 };
    static const WCHAR friendly_name[] = { 'W','i','n','e',' ','H','Q',0 };
    static const WCHAR location[] = { '_','b','l','a','n','k',0 };
    static const WCHAR target_frame_name[] = { 't','g','t','f','r','m',0 };
    HRESULT hr;
    IHlink *lnk;

    hr = HlinkCreateFromString(url, NULL, NULL, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinkCreateFromString failed with error 0x%08x\n", hr);
    if (!lnk) {
        skip("Can't create lnk, skipping test_persist.  Was wineprefixcreate run properly?\n");
        return;
    }
    test_persist_save_data("url only", lnk,
        expected_hlink_data, sizeof(expected_hlink_data),
        expected_hlink_data_ie7, sizeof(expected_hlink_data_ie7));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(url, NULL, friendly_name, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinCreateFromString failed with error 0x%08x\n", hr);
    test_persist_save_data("url + friendly name", lnk,
        expected_hlink_data2, sizeof(expected_hlink_data2),
        expected_hlink_data2_ie7, sizeof(expected_hlink_data2_ie7));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(url, location, friendly_name, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinCreateFromString failed with error 0x%08x\n", hr);
    test_persist_save_data("url + friendly_name + location", lnk,
        expected_hlink_data3, sizeof(expected_hlink_data3),
        expected_hlink_data3_ie7, sizeof(expected_hlink_data3_ie7));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(rel_url, NULL, NULL, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinCreateFromString failed with error 0x%08x\n", hr);
    test_persist_save_data("relative url", lnk,
        expected_hlink_data4, sizeof(expected_hlink_data4),
        expected_hlink_data4, sizeof(expected_hlink_data4));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(url, NULL, NULL, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinCreateFromString failed with error 0x%08x\n", hr);
    hr = IHlink_SetTargetFrameName(lnk, target_frame_name);
    ok(hr == S_OK, "IHlink_SetTargetFrameName failed with error 0x%08x\n", hr);
    test_persist_save_data("url + target frame name", lnk,
        expected_hlink_data5, sizeof(expected_hlink_data5),
        expected_hlink_data5_ie7, sizeof(expected_hlink_data5_ie7));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(filename, NULL, NULL, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinCreateFromString failed with error 0x%08x\n", hr);
    test_persist_save_data("filename", lnk,
        expected_hlink_data6, sizeof(expected_hlink_data6),
        expected_hlink_data6, sizeof(expected_hlink_data6));
    IHlink_Release(lnk);
}

static void test_special_reference(void)
{
    LPWSTR ref;
    HRESULT hres;

    hres = HlinkGetSpecialReference(HLSR_HOME, &ref);
    ok(hres == S_OK, "HlinkGetSpecialReference(HLSR_HOME) failed: %08x\n", hres);
    ok(ref != NULL, "ref == NULL\n");
    CoTaskMemFree(ref);

    hres = HlinkGetSpecialReference(HLSR_SEARCHPAGE, &ref);
    ok(hres == S_OK, "HlinkGetSpecialReference(HLSR_SEARCHPAGE) failed: %08x\n", hres);
    ok(ref != NULL, "ref == NULL\n");
    CoTaskMemFree(ref);

    ref = (void*)0xdeadbeef;
    hres = HlinkGetSpecialReference(HLSR_HISTORYFOLDER, &ref);
    ok(hres == E_NOTIMPL, "HlinkGetSpecialReference(HLSR_HISTORYFOLDER) failed: %08x\n", hres);
    ok(ref == NULL, "ref=%p\n", ref);

    ref = (void*)0xdeadbeef;
    hres = HlinkGetSpecialReference(4, &ref);
    ok(hres == E_INVALIDARG, "HlinkGetSpecialReference(HLSR_HISTORYFOLDER) failed: %08x\n", hres);
    ok(ref == NULL, "ref=%p\n", ref);
}

static void test_HlinkCreateExtensionServices(void)
{
    IAuthenticate *authenticate;
    IHttpNegotiate *http_negotiate;
    LPWSTR password, username, headers;
    HWND hwnd;
    HRESULT hres;

    static const WCHAR usernameW[] = {'u','s','e','r',0};
    static const WCHAR passwordW[] = {'p','a','s','s',0};
    static const WCHAR headersW[] = {'h','e','a','d','e','r','s',0};
    static const WCHAR headersexW[] = {'h','e','a','d','e','r','s','\r','\n',0};

    hres = HlinkCreateExtensionServices(NULL, NULL, NULL, NULL,
                                        NULL, &IID_IAuthenticate, (void**)&authenticate);
    ok(hres == S_OK, "HlinkCreateExtensionServices failed: %08x\n", hres);
    ok(authenticate != NULL, "HlinkCreateExtensionServices returned NULL\n");

    password = username = (void*)0xdeadbeef;
    hwnd = (void*)0xdeadbeef;
    hres = IAuthenticate_Authenticate(authenticate, &hwnd, &username, &password);
    ok(hres == S_OK, "Authenticate failed: %08x\n", hres);
    ok(!hwnd, "hwnd != NULL\n");
    ok(!username, "username != NULL\n");
    ok(!password, "password != NULL\n");

    hres = IAuthenticate_QueryInterface(authenticate, &IID_IHttpNegotiate, (void**)&http_negotiate);
    ok(hres == S_OK, "Could not get IHttpNegotiate interface: %08x\n", hres);

    headers = (void*)0xdeadbeef;
    hres = IHttpNegotiate_BeginningTransaction(http_negotiate, (void*)0xdeadbeef, (void*)0xdeadbeef,
                                               0, &headers);
    ok(hres == S_OK, "BeginningTransaction failed: %08x\n", hres);
    ok(headers == NULL, "headers != NULL\n");

    hres = IHttpNegotiate_BeginningTransaction(http_negotiate, (void*)0xdeadbeef, (void*)0xdeadbeef,
                                               0, NULL);
    ok(hres == E_INVALIDARG, "BeginningTransaction failed: %08x, expected E_INVALIDARG\n", hres);

    headers = (void*)0xdeadbeef;
    hres = IHttpNegotiate_OnResponse(http_negotiate, 200, (void*)0xdeadbeef, (void*)0xdeadbeef, &headers);
    ok(hres == S_OK, "OnResponse failed: %08x\n", hres);
    ok(headers == NULL, "headers != NULL\n");

    IHttpNegotiate_Release(http_negotiate);
    IAuthenticate_Release(authenticate);


    hres = HlinkCreateExtensionServices(headersW, (HWND)0xfefefefe, usernameW, passwordW,
                                        NULL, &IID_IAuthenticate, (void**)&authenticate);
    ok(hres == S_OK, "HlinkCreateExtensionServices failed: %08x\n", hres);
    ok(authenticate != NULL, "HlinkCreateExtensionServices returned NULL\n");

    password = username = NULL;
    hwnd = NULL;
    hres = IAuthenticate_Authenticate(authenticate, &hwnd, &username, &password);
    ok(hres == S_OK, "Authenticate failed: %08x\n", hres);
    ok(hwnd == (HWND)0xfefefefe, "hwnd=%p\n", hwnd);
    ok(!lstrcmpW(username, usernameW), "unexpected username\n");
    ok(!lstrcmpW(password, passwordW), "unexpected password\n");
    CoTaskMemFree(username);
    CoTaskMemFree(password);

    password = username = (void*)0xdeadbeef;
    hwnd = (void*)0xdeadbeef;
    hres = IAuthenticate_Authenticate(authenticate, &hwnd, NULL, &password);
    ok(hres == E_INVALIDARG, "Authenticate failed: %08x\n", hres);
    ok(password == (void*)0xdeadbeef, "password = %p\n", password);
    ok(hwnd == (void*)0xdeadbeef, "hwnd = %p\n", hwnd);

    hres = IAuthenticate_QueryInterface(authenticate, &IID_IHttpNegotiate, (void**)&http_negotiate);
    ok(hres == S_OK, "Could not get IHttpNegotiate interface: %08x\n", hres);

    headers = (void*)0xdeadbeef;
    hres = IHttpNegotiate_BeginningTransaction(http_negotiate, (void*)0xdeadbeef, (void*)0xdeadbeef,
                                               0, &headers);
    ok(hres == S_OK, "BeginningTransaction failed: %08x\n", hres);
    ok(!lstrcmpW(headers, headersexW), "unexpected headers \"%s\"\n", debugstr_w(headers));
    CoTaskMemFree(headers);

    headers = (void*)0xdeadbeef;
    hres = IHttpNegotiate_OnResponse(http_negotiate, 200, (void*)0xdeadbeef, (void*)0xdeadbeef, &headers);
    ok(hres == S_OK, "OnResponse failed: %08x\n", hres);
    ok(headers == NULL, "unexpected headers \"%s\"\n", debugstr_w(headers));

    IHttpNegotiate_Release(http_negotiate);
    IAuthenticate_Release(authenticate);
}

static void test_HlinkParseDisplayName(void)
{
    IMoniker *mon = NULL;
    LPWSTR name;
    DWORD issys;
    ULONG eaten = 0;
    IBindCtx *bctx;
    HRESULT hres;

    static const WCHAR winehq_urlW[] =
            {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g',
	     '/','s','i','t','e','/','a','b','o','u','t',0};
    static const WCHAR invalid_urlW[] = {'t','e','s','t',':','1','2','3','a','b','c',0};
    static const WCHAR clsid_nameW[] = {'c','l','s','i','d',':',
            '2','0','D','0','4','F','E','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','8',
            '-','0','8','0','0','2','B','3','0','3','0','9','D',':',0};

    CreateBindCtx(0, &bctx);

    hres = HlinkParseDisplayName(bctx, winehq_urlW, FALSE, &eaten, &mon);
    ok(hres == S_OK, "HlinkParseDisplayName failed: %08x\n", hres);
    ok(eaten == sizeof(winehq_urlW)/sizeof(WCHAR)-1, "eaten=%d\n", eaten);
    ok(mon != NULL, "mon == NULL\n");

    hres = IMoniker_GetDisplayName(mon, bctx, 0, &name);
    ok(hres == S_OK, "GetDiasplayName failed: %08x\n", hres);
    ok(!lstrcmpW(name, winehq_urlW), "wrong display name %s\n", debugstr_w(name));
    CoTaskMemFree(name);

    hres = IMoniker_IsSystemMoniker(mon, &issys);
    ok(hres == S_OK, "IsSystemMoniker failed: %08x\n", hres);
    ok(issys == MKSYS_URLMONIKER, "issys=%x\n", issys);

    IMoniker_Release(mon);

    hres = HlinkParseDisplayName(bctx, clsid_nameW, FALSE, &eaten, &mon);
    ok(hres == S_OK, "HlinkParseDisplayName failed: %08x\n", hres);
    ok(eaten == sizeof(clsid_nameW)/sizeof(WCHAR)-1, "eaten=%d\n", eaten);
    ok(mon != NULL, "mon == NULL\n");

    hres = IMoniker_IsSystemMoniker(mon, &issys);
    ok(hres == S_OK, "IsSystemMoniker failed: %08x\n", hres);
    ok(issys == MKSYS_CLASSMONIKER, "issys=%x\n", issys);

    IMoniker_Release(mon);

    hres = HlinkParseDisplayName(bctx, invalid_urlW, FALSE, &eaten, &mon);
     ok(hres == S_OK, "HlinkParseDisplayName failed: %08x\n", hres);
    ok(eaten == sizeof(invalid_urlW)/sizeof(WCHAR)-1, "eaten=%d\n", eaten);
    ok(mon != NULL, "mon == NULL\n");

    hres = IMoniker_GetDisplayName(mon, bctx, 0, &name);
    ok(hres == S_OK, "GetDiasplayName failed: %08x\n", hres);
    ok(!lstrcmpW(name, invalid_urlW), "wrong display name %s\n", debugstr_w(name));
    CoTaskMemFree(name);

    hres = IMoniker_IsSystemMoniker(mon, &issys);
    ok(hres == S_OK, "IsSystemMoniker failed: %08x\n", hres);
    ok(issys == MKSYS_FILEMONIKER, "issys=%x\n", issys);

    IBindCtx_Release(bctx);
}

static IBindCtx *_bctx;

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    ok(0, "unexpected service %s\n", debugstr_guid(guidService));
    return E_NOINTERFACE;
}

static IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider ServiceProvider = { &ServiceProviderVtbl };

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallback *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IServiceProvider)) {
        *ppv = &ServiceProvider;
	return S_OK;
    }

    ok(0, "unexpected interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    return 2;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    return 1;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface, DWORD dwReserved,
        IBinding *pib)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface, REFIID riid, IUnknown *punk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IBindStatusCallbackVtbl BindStatusCallbackVtbl = {
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
    BindStatusCallback_OnObjectAvailable
};

static IBindStatusCallback BindStatusCallback = { &BindStatusCallbackVtbl };

static HRESULT WINAPI Moniker_QueryInterface(IMoniker *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    ok(0, "unexpected riid: %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Moniker_AddRef(IMoniker *iface)
{
    return 2;
}

static ULONG WINAPI Moniker_Release(IMoniker *iface)
{
    return 1;
}

static HRESULT WINAPI Moniker_GetClassID(IMoniker *iface, CLSID *pClassID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_IsDirty(IMoniker *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Load(IMoniker *iface, IStream *pStm)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Save(IMoniker *iface, IStream *pStm, BOOL fClearDirty)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_GetSizeMax(IMoniker *iface, ULARGE_INTEGER *pcbSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_BindToObject(IMoniker *iface, IBindCtx *pcb, IMoniker *pmkToLeft,
        REFIID riidResult, void **ppvResult)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_BindToStorage(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        REFIID riid, void **ppv)
{
    IUnknown *unk;
    HRESULT hres;

    static OLECHAR BSCBHolder[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };

    CHECK_EXPECT(BindToStorage);

    ok(pbc == _bctx, "pbc != _bctx\n");
    ok(pmkToLeft == NULL, "pmkToLeft=%p\n", pmkToLeft);
    ok(IsEqualGUID(&IID_IUnknown, riid), "unexpected riid %s\n", debugstr_guid(riid));
    ok(ppv != NULL, "ppv == NULL\n");
    ok(*ppv == NULL, "*ppv=%p\n", *ppv);

    hres = IBindCtx_GetObjectParam(pbc, BSCBHolder, &unk);
    ok(hres == S_OK, "GetObjectParam failed: %08x\n", hres);
    ok(unk != NULL, "unk == NULL\n");

    IUnknown_Release(unk);

    return S_OK;
}

static HRESULT WINAPI Moniker_Reduce(IMoniker *iface, IBindCtx *pbc, DWORD dwReduceHowFar,
        IMoniker **ppmkToLeft, IMoniker **ppmkReduced)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_ComposeWith(IMoniker *iface, IMoniker *pmkRight,
        BOOL fOnlyIfNotGeneric, IMoniker **ppnkComposite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Enum(IMoniker *iface, BOOL fForwrd, IEnumMoniker **ppenumMoniker)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_IsEqual(IMoniker *iface, IMoniker *pmkOtherMoniker)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Hash(IMoniker *iface, DWORD *pdwHash)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_IsRunning(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        IMoniker *pmkNewlyRunning)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_GetTimeOfLastChange(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, FILETIME *pFileTime)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Inverse(IMoniker *iface, IMoniker **ppmk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_CommonPrefixWith(IMoniker *iface, IMoniker *pmkOther,
        IMoniker **ppmkPrefix)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_RelativePathTo(IMoniker *iface, IMoniker *pmkOther,
        IMoniker **pmkRelPath)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_GetDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR *ppszDisplayName)
{
    static const WCHAR winehq_urlW[] =
            {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g',
	     '/','s','i','t','e','/','a','b','o','u','t',0};

    CHECK_EXPECT(GetDisplayName);

    ok(pbc != NULL, "pbc == NULL\n");
    ok(pbc != _bctx, "pbc == _bctx\n");
    ok(pmkToLeft == NULL, "pmkToLeft=%p\n", pmkToLeft);

    *ppszDisplayName = CoTaskMemAlloc(sizeof(winehq_urlW));
    memcpy(*ppszDisplayName, winehq_urlW, sizeof(winehq_urlW));
    return S_OK;
}

static HRESULT WINAPI Moniker_ParseDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_IsSystemMoniker(IMoniker *iface, DWORD *pdwMksys)
{
    CHECK_EXPECT2(IsSystemMoniker);

    *pdwMksys = MKSYS_URLMONIKER;
    return S_OK;
}

static IMonikerVtbl MonikerVtbl = {
    Moniker_QueryInterface,
    Moniker_AddRef,
    Moniker_Release,
    Moniker_GetClassID,
    Moniker_IsDirty,
    Moniker_Load,
    Moniker_Save,
    Moniker_GetSizeMax,
    Moniker_BindToObject,
    Moniker_BindToStorage,
    Moniker_Reduce,
    Moniker_ComposeWith,
    Moniker_Enum,
    Moniker_IsEqual,
    Moniker_Hash,
    Moniker_IsRunning,
    Moniker_GetTimeOfLastChange,
    Moniker_Inverse,
    Moniker_CommonPrefixWith,
    Moniker_RelativePathTo,
    Moniker_GetDisplayName,
    Moniker_ParseDisplayName,
    Moniker_IsSystemMoniker
};

static IMoniker Moniker = { &MonikerVtbl };

static void test_HlinkResolveMonikerForData(void)
{
    IBindCtx *bctx;
    HRESULT hres;

    CreateBindCtx(0, &bctx);
    _bctx = bctx;

    SET_EXPECT(IsSystemMoniker);
    SET_EXPECT(GetDisplayName);
    SET_EXPECT(BindToStorage);

    hres = HlinkResolveMonikerForData(&Moniker, 0, bctx, 0, NULL, &BindStatusCallback, NULL);
    ok(hres == S_OK, "HlinkResolveMonikerForData failed: %08x\n", hres);

    CHECK_CALLED(IsSystemMoniker);
    CHECK_CALLED(GetDisplayName);
    CHECK_CALLED(BindToStorage);

    IBindCtx_Release(bctx);
}

START_TEST(hlink)
{
    CoInitialize(NULL);

    test_HlinkIsShortcut();
    test_reference();
    test_persist();
    test_special_reference();
    test_HlinkCreateExtensionServices();
    test_HlinkParseDisplayName();
    test_HlinkResolveMonikerForData();

    CoUninitialize();
}
