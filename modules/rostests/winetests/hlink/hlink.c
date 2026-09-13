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
DEFINE_EXPECT(BindToObject);
DEFINE_EXPECT(GetDisplayName);

DEFINE_EXPECT(ComposeWith);
DEFINE_EXPECT(OnNavigationComplete);
DEFINE_EXPECT(Enum);
DEFINE_EXPECT(Reduce);

DEFINE_EXPECT(GetClassID);
DEFINE_EXPECT(Save);

DEFINE_EXPECT(HBC_QueryInterface_IHlinkHistory);
DEFINE_EXPECT(HBC_QueryInterface_IMarshal);
DEFINE_EXPECT(HBC_QueryInterface_IdentityUnmarshal);
DEFINE_EXPECT(HBC_QueryInterface_IUnknown);
DEFINE_EXPECT(HBC_GetObject);
DEFINE_EXPECT(HBC_UpdateHlink);

DEFINE_EXPECT(HT_QueryInterface_IHlinkTarget);
DEFINE_EXPECT(HT_SetBrowseContext);
DEFINE_EXPECT(HT_GetBrowseContext);
DEFINE_EXPECT(HT_Navigate);
DEFINE_EXPECT(HT_GetFriendlyName);

DEFINE_EXPECT(HLF_UpdateHlink);

DEFINE_EXPECT(BindStatusCallback_GetBindInfo);
DEFINE_EXPECT(BindStatusCallback_OnObjectAvailable);
DEFINE_EXPECT(BindStatusCallback_OnStartBinding);
DEFINE_EXPECT(BindStatusCallback_OnStopBinding);

DEFINE_GUID(CLSID_IdentityUnmarshal,0x0000001b,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IHlinkHistory,0x79eac9c8,0xbaf9,0x11ce,0x8c,0x82,0x00,0xaa,0x00,0x4b,0xa9,0x0b);

static IHlinkTarget HlinkTarget;

static const WCHAR winehq_urlW[] =
        {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',
         '/','t','e','s','t','s','/','h','e','l','l','o','.','h','t','m','l',0};
static const WCHAR winehq_404W[] =
        {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',
         '/','t','e','s','t','s','/','f','a','k','e','u','r','l',0};

static void test_HlinkIsShortcut(void)
{
    UINT i;
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

    for (i=0; i < ARRAY_SIZE(shortcut_test); i++) {
        hres = HlinkIsShortcut(shortcut_test[i].file);
        ok(hres == shortcut_test[i].hres, "[%d] HlinkIsShortcut returned %08lx, expected %08lx\n",
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

    r = IHlink_GetStringReference(lnk, -1, NULL, NULL);
    ok(r == S_OK, "failed, r=%08lx\n", r);

    r = IHlink_GetStringReference(lnk, -1, NULL, &str);
    ok(r == S_OK, "failed, r=%08lx\n", r);
    ok(str == NULL, "string should be null\n");

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, &str, NULL);
    ok(r == S_OK, "failed\n");
    ok(!lstrcmpW(str, url2), "url wrong\n");
    CoTaskMemFree(str);

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, NULL, NULL);
    ok(r == S_OK, "failed\n");

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, NULL, &str);
    ok(r == S_OK, "failed\n");
    ok(str == NULL, "string should be null\n");

    /* Unimplemented functions checks */
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

    hr = IHlink_QueryInterface(lnk, &IID_IPersistStream, (void **)&ps);
    ok(hr == S_OK, "IHlink_QueryInterface failed with error 0x%08lx\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed with error 0x%08lx\n", hr);

    hr = IPersistStream_Save(ps, stream, TRUE);
    ok(hr == S_OK, "IPersistStream_Save failed with error 0x%08lx\n", hr);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "GetHGlobalFromStream failed with error 0x%08lx\n", hr);

    data_size = GlobalSize(hglobal);

    data = GlobalLock(hglobal);

    /* first check we have the right amount of data */
    ok((data_size == expected_data_size) ||
       (data_size == expected_data_alt_size),
       "%s: Size of saved data differs (expected %d or %d, actual %ld)\n",
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
            if ((expected_data_alt == expected_hlink_data_ie7  && i == 89)  /* Win10 */ ||
                (expected_data_alt == expected_hlink_data2_ie7 && i == 109) /* Win10 */ ||
                (expected_data_alt == expected_hlink_data3_ie7 && i == 109) /* Win10 */ ||
                (expected_data_alt == expected_hlink_data5_ie7 && i == 107) /* Win10 */)
            {
                ok(data[i] == 0 || broken(data[i] == 1) || broken(data[i] == 3),
                   "Expected 0 or 1, got %d\n", data[i]);
                continue;
            }
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
    ok(hr == S_OK, "HlinkCreateFromString failed with error 0x%08lx\n", hr);
    if (!lnk) {
        skip("Can't create lnk, skipping test_persist.\n");
        return;
    }
    test_persist_save_data("url only", lnk,
        expected_hlink_data, sizeof(expected_hlink_data),
        expected_hlink_data_ie7, sizeof(expected_hlink_data_ie7));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(url, NULL, friendly_name, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "HlinkCreateFromString failed with error 0x%08lx\n", hr);
    test_persist_save_data("url + friendly name", lnk,
        expected_hlink_data2, sizeof(expected_hlink_data2),
        expected_hlink_data2_ie7, sizeof(expected_hlink_data2_ie7));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(url, location, friendly_name, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "HlinkCreateFromString failed with error 0x%08lx\n", hr);
    test_persist_save_data("url + friendly_name + location", lnk,
        expected_hlink_data3, sizeof(expected_hlink_data3),
        expected_hlink_data3_ie7, sizeof(expected_hlink_data3_ie7));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(rel_url, NULL, NULL, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "HlinkCreateFromString failed with error 0x%08lx\n", hr);
    test_persist_save_data("relative url", lnk,
        expected_hlink_data4, sizeof(expected_hlink_data4),
        expected_hlink_data4, sizeof(expected_hlink_data4));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(url, NULL, NULL, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "HlinkCreateFromString failed with error 0x%08lx\n", hr);
    hr = IHlink_SetTargetFrameName(lnk, target_frame_name);
    ok(hr == S_OK, "IHlink_SetTargetFrameName failed with error 0x%08lx\n", hr);
    test_persist_save_data("url + target frame name", lnk,
        expected_hlink_data5, sizeof(expected_hlink_data5),
        expected_hlink_data5_ie7, sizeof(expected_hlink_data5_ie7));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(filename, NULL, NULL, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "HlinkCreateFromString failed with error 0x%08lx\n", hr);
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
    ok(hres == S_OK, "HlinkGetSpecialReference(HLSR_HOME) failed: %08lx\n", hres);
    ok(ref != NULL, "ref == NULL\n");
    CoTaskMemFree(ref);

    hres = HlinkGetSpecialReference(HLSR_SEARCHPAGE, &ref);
    ok(hres == S_OK, "HlinkGetSpecialReference(HLSR_SEARCHPAGE) failed: %08lx\n", hres);
    ok(ref != NULL, "ref == NULL\n");
    CoTaskMemFree(ref);

    ref = (void*)0xdeadbeef;
    hres = HlinkGetSpecialReference(HLSR_HISTORYFOLDER, &ref);
    ok(hres == E_NOTIMPL, "HlinkGetSpecialReference(HLSR_HISTORYFOLDER) failed: %08lx\n", hres);
    ok(ref == NULL, "ref=%p\n", ref);

    ref = (void*)0xdeadbeef;
    hres = HlinkGetSpecialReference(4, &ref);
    ok(hres == E_INVALIDARG, "HlinkGetSpecialReference(HLSR_HISTORYFOLDER) failed: %08lx\n", hres);
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
    ok(hres == S_OK, "HlinkCreateExtensionServices failed: %08lx\n", hres);
    ok(authenticate != NULL, "HlinkCreateExtensionServices returned NULL\n");

    password = username = (void*)0xdeadbeef;
    hwnd = (void*)0xdeadbeef;
    hres = IAuthenticate_Authenticate(authenticate, &hwnd, &username, &password);
    ok(hres == S_OK, "Authenticate failed: %08lx\n", hres);
    ok(!hwnd, "hwnd != NULL\n");
    ok(!username, "username != NULL\n");
    ok(!password, "password != NULL\n");

    hres = IAuthenticate_QueryInterface(authenticate, &IID_IHttpNegotiate, (void**)&http_negotiate);
    ok(hres == S_OK, "Could not get IHttpNegotiate interface: %08lx\n", hres);

    headers = (void*)0xdeadbeef;
    hres = IHttpNegotiate_BeginningTransaction(http_negotiate, (void*)0xdeadbeef, (void*)0xdeadbeef,
                                               0, &headers);
    ok(hres == S_OK, "BeginningTransaction failed: %08lx\n", hres);
    ok(headers == NULL, "headers != NULL\n");

    hres = IHttpNegotiate_BeginningTransaction(http_negotiate, (void*)0xdeadbeef, (void*)0xdeadbeef,
                                               0, NULL);
    ok(hres == E_INVALIDARG, "BeginningTransaction failed: %08lx, expected E_INVALIDARG\n", hres);

    headers = (void*)0xdeadbeef;
    hres = IHttpNegotiate_OnResponse(http_negotiate, 200, (void*)0xdeadbeef, (void*)0xdeadbeef, &headers);
    ok(hres == S_OK, "OnResponse failed: %08lx\n", hres);
    ok(headers == NULL, "headers != NULL\n");

    IHttpNegotiate_Release(http_negotiate);
    IAuthenticate_Release(authenticate);


    hres = HlinkCreateExtensionServices(headersW, (HWND)0xfefefefe, usernameW, passwordW,
                                        NULL, &IID_IAuthenticate, (void**)&authenticate);
    ok(hres == S_OK, "HlinkCreateExtensionServices failed: %08lx\n", hres);
    ok(authenticate != NULL, "HlinkCreateExtensionServices returned NULL\n");

    password = username = NULL;
    hwnd = NULL;
    hres = IAuthenticate_Authenticate(authenticate, &hwnd, &username, &password);
    ok(hres == S_OK, "Authenticate failed: %08lx\n", hres);
    ok(hwnd == (HWND)0xfefefefe, "hwnd=%p\n", hwnd);
    ok(!lstrcmpW(username, usernameW), "unexpected username\n");
    ok(!lstrcmpW(password, passwordW), "unexpected password\n");
    CoTaskMemFree(username);
    CoTaskMemFree(password);

    password = username = (void*)0xdeadbeef;
    hwnd = (void*)0xdeadbeef;
    hres = IAuthenticate_Authenticate(authenticate, &hwnd, NULL, &password);
    ok(hres == E_INVALIDARG, "Authenticate failed: %08lx\n", hres);
    ok(password == (void*)0xdeadbeef, "password = %p\n", password);
    ok(hwnd == (void*)0xdeadbeef, "hwnd = %p\n", hwnd);

    hres = IAuthenticate_QueryInterface(authenticate, &IID_IHttpNegotiate, (void**)&http_negotiate);
    ok(hres == S_OK, "Could not get IHttpNegotiate interface: %08lx\n", hres);

    headers = (void*)0xdeadbeef;
    hres = IHttpNegotiate_BeginningTransaction(http_negotiate, (void*)0xdeadbeef, (void*)0xdeadbeef,
                                               0, &headers);
    ok(hres == S_OK, "BeginningTransaction failed: %08lx\n", hres);
    ok(!lstrcmpW(headers, headersexW), "unexpected headers %s\n", wine_dbgstr_w(headers));
    CoTaskMemFree(headers);

    headers = (void*)0xdeadbeef;
    hres = IHttpNegotiate_OnResponse(http_negotiate, 200, (void*)0xdeadbeef, (void*)0xdeadbeef, &headers);
    ok(hres == S_OK, "OnResponse failed: %08lx\n", hres);
    ok(headers == NULL, "unexpected headers %s\n", wine_dbgstr_w(headers));

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

    static const WCHAR invalid_urlW[] = {'t','e','s','t',':','1','2','3','a','b','c',0};
    static const WCHAR clsid_nameW[] = {'c','l','s','i','d',':',
            '2','0','D','0','4','F','E','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','8',
            '-','0','8','0','0','2','B','3','0','3','0','9','D',':',0};
    static const WCHAR file_urlW[] =
            {'f','i','l','e',':','/','/','/','c',':','\\','f','i','l','e','.','t','x','t',0};

    CreateBindCtx(0, &bctx);

    hres = HlinkParseDisplayName(bctx, winehq_urlW, FALSE, &eaten, &mon);
    ok(hres == S_OK, "HlinkParseDisplayName failed: %08lx\n", hres);
    ok(eaten == ARRAY_SIZE(winehq_urlW)-1, "eaten=%ld\n", eaten);
    ok(mon != NULL, "mon == NULL\n");

    hres = IMoniker_GetDisplayName(mon, bctx, 0, &name);
    ok(hres == S_OK, "GetDisplayName failed: %08lx\n", hres);
    ok(!lstrcmpW(name, winehq_urlW), "wrong display name %s\n", wine_dbgstr_w(name));
    CoTaskMemFree(name);

    hres = IMoniker_IsSystemMoniker(mon, &issys);
    ok(hres == S_OK, "IsSystemMoniker failed: %08lx\n", hres);
    ok(issys == MKSYS_URLMONIKER, "issys=%lx\n", issys);

    IMoniker_Release(mon);

    hres = HlinkParseDisplayName(bctx, clsid_nameW, FALSE, &eaten, &mon);
    ok(hres == S_OK, "HlinkParseDisplayName failed: %08lx\n", hres);
    ok(eaten == ARRAY_SIZE(clsid_nameW)-1, "eaten=%ld\n", eaten);
    ok(mon != NULL, "mon == NULL\n");

    hres = IMoniker_IsSystemMoniker(mon, &issys);
    ok(hres == S_OK, "IsSystemMoniker failed: %08lx\n", hres);
    ok(issys == MKSYS_CLASSMONIKER, "issys=%lx\n", issys);

    IMoniker_Release(mon);

    hres = HlinkParseDisplayName(bctx, invalid_urlW, FALSE, &eaten, &mon);
    ok(hres == S_OK, "HlinkParseDisplayName failed: %08lx\n", hres);
    ok(eaten == ARRAY_SIZE(invalid_urlW)-1, "eaten=%ld\n", eaten);
    ok(mon != NULL, "mon == NULL\n");

    hres = IMoniker_GetDisplayName(mon, bctx, 0, &name);
    ok(hres == S_OK, "GetDisplayName failed: %08lx\n", hres);
    ok(!lstrcmpW(name, invalid_urlW), "wrong display name %s\n", wine_dbgstr_w(name));
    CoTaskMemFree(name);

    hres = IMoniker_IsSystemMoniker(mon, &issys);
    ok(hres == S_OK, "IsSystemMoniker failed: %08lx\n", hres);
    ok(issys == MKSYS_FILEMONIKER, "issys=%lx\n", issys);

    IMoniker_Release(mon);

    hres = HlinkParseDisplayName(bctx, file_urlW, FALSE, &eaten, &mon);
    ok(hres == S_OK, "HlinkParseDisplayName failed: %08lx\n", hres);
    ok(eaten == ARRAY_SIZE(file_urlW)-1, "eaten=%ld\n", eaten);
    ok(mon != NULL, "mon == NULL\n");

    hres = IMoniker_GetDisplayName(mon, bctx, 0, &name);
    ok(hres == S_OK, "GetDisplayName failed: %08lx\n", hres);
    ok(!lstrcmpW(name, file_urlW+8), "wrong display name %s\n", wine_dbgstr_w(name));
    CoTaskMemFree(name);

    hres = IMoniker_IsSystemMoniker(mon, &issys);
    ok(hres == S_OK, "IsSystemMoniker failed: %08lx\n", hres);
    ok(issys == MKSYS_FILEMONIKER, "issys=%lx\n", issys);

    IMoniker_Release(mon);
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
    ok(0, "unexpected service %s\n", wine_dbgstr_guid(guidService));
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

    ok(0, "unexpected interface %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static LONG bind_callback_refs = 1;

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    return ++bind_callback_refs;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    return --bind_callback_refs;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD reserved, IBinding *binding)
{
    CHECK_EXPECT(BindStatusCallback_OnStartBinding);

    ok(!binding, "binding = %p\n", binding);
    return S_OK;
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

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface, HRESULT hr, const WCHAR *error)
{
    CHECK_EXPECT(BindStatusCallback_OnStopBinding);

    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(!error, "got error %s\n", wine_dbgstr_w(error));

    return 0xdeadbeef;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface, DWORD *bind_flags, BINDINFO *bind_info)
{
    CHECK_EXPECT(BindStatusCallback_GetBindInfo);

    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface, REFIID iid, IUnknown *out)
{
    CHECK_EXPECT(BindStatusCallback_OnObjectAvailable);

    ok(IsEqualGUID(iid, &IID_IUnknown), "iid = %s\n", wine_dbgstr_guid(iid));
    ok(out == (IUnknown *)&HlinkTarget, "out = %p\n", out);

    return 0xdeadbeef;
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

static HRESULT WINAPI HlinkBrowseContext_QueryInterface(
        IHlinkBrowseContext *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualGUID(&IID_IHlinkHistory, riid))
        CHECK_EXPECT(HBC_QueryInterface_IHlinkHistory);
    else if (IsEqualGUID(&IID_IMarshal, riid))
        CHECK_EXPECT2(HBC_QueryInterface_IMarshal);
    else if (IsEqualGUID(&CLSID_IdentityUnmarshal, riid))
        CHECK_EXPECT(HBC_QueryInterface_IdentityUnmarshal);
    else if (IsEqualGUID(&IID_IUnknown, riid))
        CHECK_EXPECT(HBC_QueryInterface_IUnknown);
    else
        ok(0, "unexpected interface: %s\n", wine_dbgstr_guid(riid));

    return E_NOINTERFACE;
}

static LONG browse_ctx_refs = 1;

static ULONG WINAPI HlinkBrowseContext_AddRef(IHlinkBrowseContext *iface)
{
    return ++browse_ctx_refs;
}

static ULONG WINAPI HlinkBrowseContext_Release(IHlinkBrowseContext *iface)
{
    return --browse_ctx_refs;
}

static HRESULT WINAPI HlinkBrowseContext_Register(IHlinkBrowseContext *iface,
        DWORD reserved, IUnknown *piunk, IMoniker *pimk, DWORD *pdwRegister)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IUnknown *HBC_object;

static IMoniker Moniker;
static HRESULT WINAPI HlinkBrowseContext_GetObject(IHlinkBrowseContext *iface,
        IMoniker *pimk, BOOL fBindIfRootRegistered, IUnknown **ppiunk)
{
    IBindCtx *bctx;
    WCHAR *name;
    HRESULT hr;

    CHECK_EXPECT(HBC_GetObject);

    CreateBindCtx(0, &bctx);
    hr = IMoniker_GetDisplayName(pimk, bctx, NULL, &name);
    ok(hr == S_OK, "Failed to get display name, hr %#lx.\n", hr);
    ok(!lstrcmpW(winehq_urlW, name) || !lstrcmpW(winehq_404W, name), "got unexpected url\n");
    CoTaskMemFree(name);
    IBindCtx_Release(bctx);

    ok(fBindIfRootRegistered == 1, "fBindIfRootRegistered = %x\n", fBindIfRootRegistered);

    *ppiunk = HBC_object;
    return HBC_object ? S_OK : 0xdeadbeef;
}

static HRESULT WINAPI HlinkBrowseContext_Revoke(IHlinkBrowseContext *iface, DWORD dwRegister)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkBrowseContext_SetBrowseWindowInfo(
        IHlinkBrowseContext *iface, HLBWINFO *phlbwi)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkBrowseContext_GetBrowseWindowInfo(
        IHlinkBrowseContext *iface, HLBWINFO *phlbwi)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkBrowseContext_SetInitialHlink(IHlinkBrowseContext *iface,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkBrowseContext_OnNavigateHlink(IHlinkBrowseContext *iface, DWORD grfHLNF,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, ULONG *puHLID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkBrowseContext_UpdateHlink(IHlinkBrowseContext *iface, ULONG uHLID,
        IMoniker *pimkTarget, LPCWSTR location, LPCWSTR friendly_name)
{
    CHECK_EXPECT(HBC_UpdateHlink);
    return S_OK;
}

static HRESULT WINAPI HlinkBrowseContext_EnumNavigationStack(IHlinkBrowseContext *iface,
        DWORD dwReserved, DWORD grfHLFNAMEF, IEnumHLITEM **ppienumhlitem)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkBrowseContext_QueryHlink(IHlinkBrowseContext *iface,
        DWORD grfHLQF, ULONG uHLID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkBrowseContext_GetHlink(IHlinkBrowseContext *iface,
        ULONG uHLID, IHlink **ppihl)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkBrowseContext_SetCurrentHlink(
        IHlinkBrowseContext *iface, ULONG uHLID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkBrowseContext_Clone(IHlinkBrowseContext *iface,
        IUnknown *piunkOuter, REFIID riid, IUnknown **ppiunkObj)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkBrowseContext_Close(IHlinkBrowseContext *iface, DWORD reserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IHlinkBrowseContextVtbl HlinkBrowseContextVtbl = {
    HlinkBrowseContext_QueryInterface,
    HlinkBrowseContext_AddRef,
    HlinkBrowseContext_Release,
    HlinkBrowseContext_Register,
    HlinkBrowseContext_GetObject,
    HlinkBrowseContext_Revoke,
    HlinkBrowseContext_SetBrowseWindowInfo,
    HlinkBrowseContext_GetBrowseWindowInfo,
    HlinkBrowseContext_SetInitialHlink,
    HlinkBrowseContext_OnNavigateHlink,
    HlinkBrowseContext_UpdateHlink,
    HlinkBrowseContext_EnumNavigationStack,
    HlinkBrowseContext_QueryHlink,
    HlinkBrowseContext_GetHlink,
    HlinkBrowseContext_SetCurrentHlink,
    HlinkBrowseContext_Clone,
    HlinkBrowseContext_Close
};

static IHlinkBrowseContext HlinkBrowseContext = { &HlinkBrowseContextVtbl };

static HRESULT WINAPI HlinkTarget_QueryInterface(IHlinkTarget *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IHlinkTarget, riid)) {
        CHECK_EXPECT(HT_QueryInterface_IHlinkTarget);
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected interface: %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI HlinkTarget_AddRef(IHlinkTarget *iface)
{
    return 2;
}

static ULONG WINAPI HlinkTarget_Release(IHlinkTarget *iface)
{
    return 1;
}

static HRESULT WINAPI HlinkTarget_SetBrowseContext(IHlinkTarget *iface,
        IHlinkBrowseContext *pihlbc)
{
    CHECK_EXPECT(HT_SetBrowseContext);

    ok(pihlbc == &HlinkBrowseContext, "pihlbc != &HlinkBrowseContext (%p)\n", pihlbc);
    return S_OK;
}

static HRESULT WINAPI HlinkTarget_GetBrowseContext(IHlinkTarget *iface,
        IHlinkBrowseContext **ppihlbc)
{
    CHECK_EXPECT(HT_GetBrowseContext);

    *ppihlbc = NULL;
    return S_OK;
}

static HRESULT WINAPI HlinkTarget_Navigate(IHlinkTarget *iface,
        DWORD grfHLNF, LPCWSTR pwzJumpLocation)
{
    CHECK_EXPECT(HT_Navigate);

    ok(grfHLNF == 0, "grfHLNF = %lx\n", grfHLNF);
    ok(pwzJumpLocation == NULL, "pwzJumpLocation = %s\n", wine_dbgstr_w(pwzJumpLocation));
    return S_OK;
}

static HRESULT WINAPI HlinkTarget_GetMoniker(IHlinkTarget *iface,
        LPCWSTR pwzLocation, DWORD dwAssign, IMoniker **ppimkLocation)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkTarget_GetFriendlyName(IHlinkTarget *iface,
        LPCWSTR pwzLocation, LPWSTR *ppwzFriendlyName)
{
    CHECK_EXPECT(HT_GetFriendlyName);
    return E_NOTIMPL;
}

static IHlinkTargetVtbl HlinkTargetVtbl = {
    HlinkTarget_QueryInterface,
    HlinkTarget_AddRef,
    HlinkTarget_Release,
    HlinkTarget_SetBrowseContext,
    HlinkTarget_GetBrowseContext,
    HlinkTarget_Navigate,
    HlinkTarget_GetMoniker,
    HlinkTarget_GetFriendlyName
};

static IHlinkTarget HlinkTarget = { &HlinkTargetVtbl };

static HRESULT WINAPI Moniker_QueryInterface(IMoniker *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    ok(0, "unexpected riid: %s\n", wine_dbgstr_guid(riid));
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
    CHECK_EXPECT(GetClassID);
    *pClassID = IID_IUnknown; /* not a valid CLSID */
    return S_OK;
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
    CHECK_EXPECT(Save);
    return S_OK;
}

static HRESULT WINAPI Moniker_GetSizeMax(IMoniker *iface, ULARGE_INTEGER *pcbSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static BOOL async_bind;
static IBindStatusCallback *async_bind_callback;

static HRESULT WINAPI Moniker_BindToObject(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        REFIID riid, void **ppv)
{
    static WCHAR bscb_holderW[] = {'_','B','S','C','B','_','H','o','l','d','e','r','_',0};
    IUnknown *bind_callback_holder;
    HRESULT hr;

    CHECK_EXPECT(BindToObject);

    ok(pbc != _bctx, "pbc != _bctx\n");
    ok(pmkToLeft == NULL, "pmkToLeft = %p\n", pmkToLeft);
    ok(IsEqualGUID(&IID_IUnknown, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    ok(ppv != NULL, "ppv == NULL\n");
    ok(*ppv == NULL, "*ppv = %p\n", *ppv);

    if (async_bind)
    {
        hr = IBindCtx_GetObjectParam(pbc, bscb_holderW, &bind_callback_holder);
        ok(hr == S_OK, "Failed to get IBindStatusCallback holder, hr %#lx.\n", hr);
        hr = IUnknown_QueryInterface(bind_callback_holder, &IID_IBindStatusCallback,
                (void **)&async_bind_callback);
        ok(hr == S_OK, "Failed to get IBindStatusCallback interface, hr %#lx.\n", hr);
        IUnknown_Release(bind_callback_holder);
        return MK_S_ASYNCHRONOUS;
    }

    *ppv = &HlinkTarget;
    return S_OK;
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
    ok(IsEqualGUID(&IID_IUnknown, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    ok(ppv != NULL, "ppv == NULL\n");
    ok(*ppv == NULL, "*ppv=%p\n", *ppv);

    hres = IBindCtx_GetObjectParam(pbc, BSCBHolder, &unk);
    ok(hres == S_OK, "GetObjectParam failed: %08lx\n", hres);
    ok(unk != NULL, "unk == NULL\n");

    IUnknown_Release(unk);

    return S_OK;
}

static HRESULT WINAPI Moniker_Reduce(IMoniker *iface, IBindCtx *pbc, DWORD dwReduceHowFar,
        IMoniker **ppmkToLeft, IMoniker **ppmkReduced)
{
    CHECK_EXPECT(Reduce);
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
    CHECK_EXPECT(Enum);
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
    CHECK_EXPECT2(GetDisplayName);

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
    ok(hres == S_OK, "HlinkResolveMonikerForData failed: %08lx\n", hres);

    CHECK_CALLED(IsSystemMoniker);
    CHECK_CALLED(GetDisplayName);
    CHECK_CALLED(BindToStorage);

    IBindCtx_Release(bctx);
    _bctx = NULL;
}

static void test_HlinkGetSetMonikerReference(void)
{
    IMoniker *found_trgt, *dummy, *dummy2;
    IHlink *hlink;
    HRESULT hres;
    const WCHAR one[] = {'1',0};
    const WCHAR two[] = {'2',0};
    const WCHAR name[] = {'a',0};
    WCHAR *found_loc;

    /* create two dummy monikers to use as targets */
    hres = CreateItemMoniker(one, one, &dummy);
    ok(hres == S_OK, "CreateItemMoniker failed: 0x%08lx\n", hres);

    hres = CreateItemMoniker(two, two, &dummy2);
    ok(hres == S_OK, "CreateItemMoniker failed: 0x%08lx\n", hres);

    /* create a new hlink: target => dummy, location => one */
    hres = HlinkCreateFromMoniker(dummy, one, name, NULL, 0, NULL, &IID_IHlink, (void**)&hlink);
    ok(hres == S_OK, "HlinkCreateFromMoniker failed: 0x%08lx\n", hres);

    /* validate the target and location */
    hres = IHlink_GetMonikerReference(hlink, HLINKGETREF_DEFAULT, &found_trgt, &found_loc);
    ok(hres == S_OK, "IHlink_GetMonikerReference failed: 0x%08lx\n", hres);
    ok(found_trgt == dummy, "Found target should've been %p, was: %p\n", dummy, found_trgt);
    ok(lstrcmpW(found_loc, one) == 0, "Found location should've been %s, was: %s\n", wine_dbgstr_w(one), wine_dbgstr_w(found_loc));
    IMoniker_Release(found_trgt);
    CoTaskMemFree(found_loc);

    /* set location => two */
    hres = IHlink_SetMonikerReference(hlink, HLINKSETF_LOCATION, dummy2, two);
    ok(hres == S_OK, "IHlink_SetMonikerReference failed: 0x%08lx\n", hres);

    hres = IHlink_GetMonikerReference(hlink, HLINKGETREF_DEFAULT, &found_trgt, &found_loc);
    ok(hres == S_OK, "IHlink_GetMonikerReference failed: 0x%08lx\n", hres);
    ok(found_trgt == dummy, "Found target should've been %p, was: %p\n", dummy, found_trgt);
    ok(lstrcmpW(found_loc, two) == 0, "Found location should've been %s, was: %s\n", wine_dbgstr_w(two), wine_dbgstr_w(found_loc));
    IMoniker_Release(found_trgt);
    CoTaskMemFree(found_loc);

    /* set target => dummy2 */
    hres = IHlink_SetMonikerReference(hlink, HLINKSETF_TARGET, dummy2, one);
    ok(hres == S_OK, "IHlink_SetMonikerReference failed: 0x%08lx\n", hres);

    hres = IHlink_GetMonikerReference(hlink, HLINKGETREF_DEFAULT, &found_trgt, &found_loc);
    ok(hres == S_OK, "IHlink_GetMonikerReference failed: 0x%08lx\n", hres);
    ok(found_trgt == dummy2, "Found target should've been %p, was: %p\n", dummy2, found_trgt);
    ok(lstrcmpW(found_loc, two) == 0, "Found location should've been %s, was: %s\n", wine_dbgstr_w(two), wine_dbgstr_w(found_loc));
    IMoniker_Release(found_trgt);
    CoTaskMemFree(found_loc);

    /* set target => dummy, location => one */
    hres = IHlink_SetMonikerReference(hlink, HLINKSETF_TARGET | HLINKSETF_LOCATION, dummy, one);
    ok(hres == S_OK, "IHlink_SetMonikerReference failed: 0x%08lx\n", hres);

    hres = IHlink_GetMonikerReference(hlink, HLINKGETREF_DEFAULT, &found_trgt, &found_loc);
    ok(hres == S_OK, "IHlink_GetMonikerReference failed: 0x%08lx\n", hres);
    ok(found_trgt == dummy, "Found target should've been %p, was: %p\n", dummy, found_trgt);
    ok(lstrcmpW(found_loc, one) == 0, "Found location should've been %s, was: %s\n", wine_dbgstr_w(one), wine_dbgstr_w(found_loc));
    IMoniker_Release(found_trgt);
    CoTaskMemFree(found_loc);

    /* no HLINKSETF flags */
    hres = IHlink_SetMonikerReference(hlink, 0, dummy2, two);
    ok(hres == E_INVALIDARG, "IHlink_SetMonikerReference should've failed with E_INVALIDARG (0x%08lx), failed with 0x%08lx\n", E_INVALIDARG, hres);

    hres = IHlink_GetMonikerReference(hlink, HLINKGETREF_DEFAULT, &found_trgt, &found_loc);
    ok(hres == S_OK, "IHlink_GetMonikerReference failed: 0x%08lx\n", hres);
    ok(found_trgt == dummy, "Found target should've been %p, was: %p\n", dummy, found_trgt);
    ok(lstrcmpW(found_loc, one) == 0, "Found location should've been %s, was: %s\n", wine_dbgstr_w(one), wine_dbgstr_w(found_loc));
    IMoniker_Release(found_trgt);
    CoTaskMemFree(found_loc);

    /* invalid HLINKSETF flags */
    /* Windows returns garbage; on 32-bit it returns the flags probably because the compiler happened to store them in %eax at some point */
    if (0) 
        IHlink_SetMonikerReference(hlink, 12, dummy2, two);

    hres = IHlink_GetMonikerReference(hlink, HLINKGETREF_DEFAULT, &found_trgt, &found_loc);
    ok(hres == S_OK, "IHlink_GetMonikerReference failed: 0x%08lx\n", hres);
    ok(found_trgt == dummy, "Found target should've been %p, was: %p\n", dummy, found_trgt);
    ok(lstrcmpW(found_loc, one) == 0, "Found location should've been %s, was: %s\n", wine_dbgstr_w(one), wine_dbgstr_w(found_loc));
    IMoniker_Release(found_trgt);
    CoTaskMemFree(found_loc);

    /* valid & invalid HLINKSETF flags */
    hres = IHlink_SetMonikerReference(hlink, 12 | HLINKSETF_TARGET, dummy2, two);
    ok(hres == S_OK, "IHlink_SetMonikerReference failed: 0x%08lx\n", hres);

    hres = IHlink_GetMonikerReference(hlink, HLINKGETREF_DEFAULT, &found_trgt, &found_loc);
    ok(hres == S_OK, "IHlink_GetMonikerReference failed: 0x%08lx\n", hres);
    ok(found_trgt == dummy2, "Found target should've been %p, was: %p\n", dummy2, found_trgt);
    ok(lstrcmpW(found_loc, one) == 0, "Found location should've been %s, was: %s\n", wine_dbgstr_w(one), wine_dbgstr_w(found_loc));
    IMoniker_Release(found_trgt);
    CoTaskMemFree(found_loc);

    /* NULL args */
    hres = IHlink_SetMonikerReference(hlink, HLINKSETF_TARGET | HLINKSETF_LOCATION, NULL, NULL);
    ok(hres == S_OK, "IHlink_SetMonikerReference failed: 0x%08lx\n", hres);

    hres = IHlink_GetMonikerReference(hlink, HLINKGETREF_DEFAULT, &found_trgt, &found_loc);
    ok(hres == S_OK, "IHlink_GetMonikerReference failed: 0x%08lx\n", hres);
    ok(found_trgt == NULL, "Found target should've been %p, was: %p\n", NULL, found_trgt);
    ok(found_loc == NULL, "Found location should've been %s, was: %s\n", wine_dbgstr_w(NULL), wine_dbgstr_w(found_loc));
    if(found_trgt)
        IMoniker_Release(found_trgt);

    IHlink_Release(hlink);
    IMoniker_Release(dummy2);
    IMoniker_Release(dummy);

    SET_EXPECT(Reduce);
    SET_EXPECT(Enum);
    SET_EXPECT(IsSystemMoniker);
    SET_EXPECT(GetDisplayName);
    hres = HlinkCreateFromMoniker(&Moniker, NULL, NULL, NULL, 0, NULL,
            &IID_IHlink, (void **)&hlink);
    ok(hres == S_OK, "CreateFromMoniker failed: %08lx\n", hres);

    hres = IHlink_GetMonikerReference(hlink, HLINKGETREF_ABSOLUTE,
            &found_trgt, &found_loc);
    ok(hres == S_OK, "CreateFromMoniker failed: %08lx\n", hres);
    ok(found_trgt == &Moniker, "Got unexpected moniker: %p\n", found_trgt);
    ok(found_loc == NULL, "Got unexpected location: %p\n", found_loc);
    todo_wine CHECK_CALLED(Reduce);
    todo_wine CHECK_CALLED(Enum);
    CHECK_CALLED(IsSystemMoniker);
    CHECK_CALLED(GetDisplayName);

    IHlink_Release(hlink);
}

static void test_HlinkGetSetStringReference(void)
{
    IHlink *link;
    static const WCHAR one[] = {'1',0};
    static const WCHAR two[] = {'2',0};
    static const WCHAR three[] = {'3',0};
    static const WCHAR empty[] = {0};
    WCHAR *fnd_tgt, *fnd_loc;
    HRESULT hres;

    /* create a new hlink: target => NULL, location => one */
    hres = HlinkCreateFromMoniker(NULL, one, empty, NULL, 0, NULL, &IID_IHlink, (void**)&link);
    ok(hres == S_OK, "HlinkCreateFromMoniker failed: 0x%08lx\n", hres);

    /* test setting/getting location */
    hres = IHlink_GetStringReference(link, HLINKGETREF_DEFAULT, &fnd_tgt, &fnd_loc);
    ok(hres == S_OK, "IHlink_GetStringReference failed: 0x%08lx\n", hres);
    ok(fnd_tgt == NULL, "Found target should have been NULL, was: %s\n", wine_dbgstr_w(fnd_tgt));
    ok(!lstrcmpW(fnd_loc, one), "Found location should have been %s, was: %s\n", wine_dbgstr_w(one), wine_dbgstr_w(fnd_loc));
    CoTaskMemFree(fnd_tgt);
    CoTaskMemFree(fnd_loc);

    hres = IHlink_SetStringReference(link, HLINKSETF_LOCATION, one, two);
    ok(hres == S_OK, "IHlink_SetStringReference failed: 0x%08lx\n", hres);

    hres = IHlink_GetStringReference(link, HLINKGETREF_DEFAULT, &fnd_tgt, &fnd_loc);
    ok(hres == S_OK, "IHlink_GetStringReference failed: 0x%08lx\n", hres);
    ok(fnd_tgt == NULL, "Found target should have been NULL, was: %s\n", wine_dbgstr_w(fnd_tgt));
    ok(!lstrcmpW(fnd_loc, two), "Found location should have been %s, was: %s\n", wine_dbgstr_w(two), wine_dbgstr_w(fnd_loc));
    CoTaskMemFree(fnd_tgt);
    CoTaskMemFree(fnd_loc);

    hres = IHlink_SetStringReference(link, -HLINKSETF_LOCATION, two, one);
    ok(hres == S_OK, "IHlink_SetStringReference failed: 0x%08lx\n", hres);

    hres = IHlink_GetStringReference(link, HLINKGETREF_DEFAULT, &fnd_tgt, &fnd_loc);
    ok(hres == S_OK, "IHlink_GetStringReference failed: 0x%08lx\n", hres);
    ok(fnd_tgt == NULL, "Found target should have been NULL, was: %s\n", wine_dbgstr_w(fnd_tgt));
    ok(!lstrcmpW(fnd_loc, one), "Found location should have been %s, was: %s\n", wine_dbgstr_w(one), wine_dbgstr_w(fnd_loc));
    CoTaskMemFree(fnd_tgt);
    CoTaskMemFree(fnd_loc);

    /* test setting/getting target */
    hres = IHlink_SetStringReference(link, HLINKSETF_TARGET, two, three);
    ok(hres == S_OK, "IHlink_SetStringReference failed: 0x%08lx\n", hres);

    hres = IHlink_GetStringReference(link, HLINKGETREF_DEFAULT, &fnd_tgt, &fnd_loc);
    ok(hres == S_OK, "IHlink_GetStringReference failed: 0x%08lx\n", hres);
    ok(!lstrcmpW(fnd_tgt, two), "Found target should have been %s, was: %s\n", wine_dbgstr_w(two), wine_dbgstr_w(fnd_tgt));
    ok(!lstrcmpW(fnd_loc, one), "Found location should have been %s, was: %s\n", wine_dbgstr_w(one), wine_dbgstr_w(fnd_loc));
    CoTaskMemFree(fnd_tgt);
    CoTaskMemFree(fnd_loc);

    hres = IHlink_SetStringReference(link, -HLINKSETF_TARGET, three, two);
    ok(hres == S_OK, "IHlink_SetStringReference failed: 0x%08lx\n", hres);

    hres = IHlink_GetStringReference(link, HLINKGETREF_DEFAULT, &fnd_tgt, &fnd_loc);
    ok(hres == S_OK, "IHlink_GetStringReference failed: 0x%08lx\n", hres);
    ok(!lstrcmpW(fnd_tgt, three), "Found target should have been %s, was: %s\n", wine_dbgstr_w(three), wine_dbgstr_w(fnd_tgt));
    ok(!lstrcmpW(fnd_loc, two), "Found location should have been %s, was: %s\n", wine_dbgstr_w(two), wine_dbgstr_w(fnd_loc));
    CoTaskMemFree(fnd_tgt);
    CoTaskMemFree(fnd_loc);

    /* test setting/getting both */
    hres = IHlink_SetStringReference(link, HLINKSETF_TARGET | HLINKSETF_LOCATION, one, two);
    ok(hres == S_OK, "IHlink_SetStringReference failed: 0x%08lx\n", hres);

    hres = IHlink_GetStringReference(link, HLINKGETREF_DEFAULT, &fnd_tgt, &fnd_loc);
    ok(hres == S_OK, "IHlink_GetStringReference failed: 0x%08lx\n", hres);
    ok(!lstrcmpW(fnd_tgt, one), "Found target should have been %s, was: %s\n", wine_dbgstr_w(one), wine_dbgstr_w(fnd_tgt));
    ok(!lstrcmpW(fnd_loc, two), "Found location should have been %s, was: %s\n", wine_dbgstr_w(two), wine_dbgstr_w(fnd_loc));
    CoTaskMemFree(fnd_tgt);
    CoTaskMemFree(fnd_loc);

    hres = IHlink_SetStringReference(link, -(HLINKSETF_TARGET | HLINKSETF_LOCATION), three, one);
    ok(hres == S_OK, "IHlink_SetStringReference failed: 0x%08lx\n", hres);

    hres = IHlink_GetStringReference(link, HLINKGETREF_DEFAULT, &fnd_tgt, &fnd_loc);
    ok(hres == S_OK, "IHlink_GetStringReference failed: 0x%08lx\n", hres);
    ok(!lstrcmpW(fnd_tgt, three), "Found target should have been %s, was: %s\n", wine_dbgstr_w(three), wine_dbgstr_w(fnd_tgt));
    ok(!lstrcmpW(fnd_loc, two), "Found location should have been %s, was: %s\n", wine_dbgstr_w(two), wine_dbgstr_w(fnd_loc));
    CoTaskMemFree(fnd_tgt);
    CoTaskMemFree(fnd_loc);

    /* test invalid flags/params */
    hres = IHlink_GetStringReference(link, 4, &fnd_tgt, &fnd_loc);
    ok(hres == E_INVALIDARG, "IHlink_GetStringReference should have failed "
           "with E_INVALIDARG (0x%08lx), instead: 0x%08lx\n", E_INVALIDARG, hres);
    ok(fnd_tgt == NULL, "Found target should have been NULL, was: %s\n", wine_dbgstr_w(fnd_tgt));
    ok(fnd_loc == NULL, "Found location should have been NULL, was: %s\n", wine_dbgstr_w(fnd_loc));
    CoTaskMemFree(fnd_tgt);
    CoTaskMemFree(fnd_loc);

    hres = IHlink_GetStringReference(link, -1, &fnd_tgt, NULL);
    todo_wine ok(hres == E_FAIL, "IHlink_GetStringReference should have failed "
           "with E_FAIL (0x%08lx), instead: 0x%08lx\n", E_FAIL, hres);
    CoTaskMemFree(fnd_tgt);

    hres = IHlink_GetStringReference(link, -1, NULL, NULL);
    ok(hres == S_OK, "failed, hres=%08lx\n", hres);

    hres = IHlink_GetStringReference(link, -1, NULL, &fnd_loc);
    ok(hres == S_OK, "failed, hres=%08lx\n", hres);
    CoTaskMemFree(fnd_loc);

    hres = IHlink_GetStringReference(link, -1, &fnd_tgt, &fnd_loc);
    todo_wine ok(hres == E_FAIL, "IHlink_GetStringReference should have failed "
           "with E_FAIL (0x%08lx), instead: 0x%08lx\n", E_FAIL, hres);
    CoTaskMemFree(fnd_tgt);
    CoTaskMemFree(fnd_loc);

    hres = IHlink_GetStringReference(link, -2, &fnd_tgt, &fnd_loc);
    ok(hres == E_INVALIDARG, "IHlink_GetStringReference should have failed "
           "with E_INVALIDARG (0x%08lx), instead: 0x%08lx\n", E_INVALIDARG, hres);
    CoTaskMemFree(fnd_tgt);
    CoTaskMemFree(fnd_loc);

    if (0)
    {
        /* Windows returns garbage; on 32-bit it returns the flags probably because the compiler happened to store them in %eax at some point */
        IHlink_SetStringReference(link, 4, NULL, NULL);
        IHlink_SetStringReference(link, -4, NULL, NULL);
    }

    IHlink_Release(link);
}

#define setStringRef(h,f,t,l) r_setStringRef(__LINE__,h,f,t,l)
static void r_setStringRef(unsigned line, IHlink *hlink, DWORD flags, const WCHAR *tgt, const WCHAR *loc)
{
    HRESULT hres;
    hres = IHlink_SetStringReference(hlink, flags, tgt, loc);
    ok_(__FILE__,line) (hres == S_OK, "IHlink_SetStringReference failed: 0x%08lx\n", hres);
}

#define getStringRef(h,t,l) r_getStringRef(__LINE__,h,t,l)
static void r_getStringRef(unsigned line, IHlink *hlink, const WCHAR *exp_tgt, const WCHAR *exp_loc)
{
    HRESULT hres;
    WCHAR *fnd_tgt, *fnd_loc;

    hres = IHlink_GetStringReference(hlink, HLINKGETREF_DEFAULT, &fnd_tgt, &fnd_loc);
    ok_(__FILE__,line) (hres == S_OK, "IHlink_GetStringReference failed: 0x%08lx\n", hres);

    if(exp_tgt)
        ok_(__FILE__,line) (!lstrcmpW(fnd_tgt, exp_tgt), "Found string target should have been %s, was: %s\n", wine_dbgstr_w(exp_tgt), wine_dbgstr_w(fnd_tgt));
    else
        ok_(__FILE__,line) (fnd_tgt == NULL, "Found string target should have been NULL, was: %s\n", wine_dbgstr_w(fnd_tgt));

    if(exp_loc)
        ok_(__FILE__,line) (!lstrcmpW(fnd_loc, exp_loc), "Found string location should have been %s, was: %s\n", wine_dbgstr_w(exp_loc), wine_dbgstr_w(fnd_loc));
    else
        ok_(__FILE__,line) (fnd_loc == NULL, "Found string location should have been NULL, was: %s\n", wine_dbgstr_w(fnd_loc));

    CoTaskMemFree(fnd_tgt);
    CoTaskMemFree(fnd_loc);
}

#define setMonikerRef(h,f,t,l) r_setMonikerRef(__LINE__,h,f,t,l)
static void r_setMonikerRef(unsigned line, IHlink *hlink, DWORD flags, IMoniker *tgt, const WCHAR *loc)
{
    HRESULT hres;
    hres = IHlink_SetMonikerReference(hlink, flags, tgt, loc);
    ok_(__FILE__,line) (hres == S_OK, "IHlink_SetMonikerReference failed: 0x%08lx\n", hres);
}

/* passing 0xFFFFFFFF as exp_tgt will return the retrieved target & not test it */
#define getMonikerRef(h,t,l,r) r_getMonikerRef(__LINE__,h,t,l,r)
static IMoniker *r_getMonikerRef(unsigned line, IHlink *hlink, IMoniker *exp_tgt, const WCHAR *exp_loc, DWORD ref)
{
    HRESULT hres;
    IMoniker *fnd_tgt;
    WCHAR *fnd_loc;

    hres = IHlink_GetMonikerReference(hlink, ref, &fnd_tgt, &fnd_loc);
    ok_(__FILE__,line) (hres == S_OK, "IHlink_GetMonikerReference failed: 0x%08lx\n", hres);

    if(exp_loc)
        ok_(__FILE__,line) (!lstrcmpW(fnd_loc, exp_loc), "Found string location should have been %s, was: %s\n", wine_dbgstr_w(exp_loc), wine_dbgstr_w(fnd_loc));
    else
        ok_(__FILE__,line) (fnd_loc == NULL, "Found string location should have been NULL, was: %s\n", wine_dbgstr_w(fnd_loc));

    CoTaskMemFree(fnd_loc);

    if(exp_tgt == (IMoniker*)0xFFFFFFFF)
        return fnd_tgt;

    ok_(__FILE__,line) (fnd_tgt == exp_tgt, "Found moniker target should have been %p, was: %p\n", exp_tgt, fnd_tgt);

    if(fnd_tgt)
        IMoniker_Release(fnd_tgt);

    return NULL;
}

static void test_HlinkMoniker(void)
{
    IHlink *hlink;
    IMoniker *aMon, *file_mon;
    static const WCHAR emptyW[] = {0};
    static const WCHAR wordsW[] = {'w','o','r','d','s',0};
    static const WCHAR aW[] = {'a',0};
    static const WCHAR bW[] = {'b',0};
    HRESULT hres;

    hres = HlinkCreateFromString(NULL, NULL, NULL, NULL, 0, NULL, &IID_IHlink, (void**)&hlink);
    ok(hres == S_OK, "HlinkCreateFromString failed: 0x%08lx\n", hres);
    getStringRef(hlink, NULL, NULL);
    getMonikerRef(hlink, NULL, NULL, HLINKGETREF_RELATIVE);

    /* setting a string target creates a moniker reference */
    setStringRef(hlink, HLINKSETF_TARGET | HLINKSETF_LOCATION, aW, wordsW);
    getStringRef(hlink, aW, wordsW);
    aMon = getMonikerRef(hlink, (IMoniker*)0xFFFFFFFF, wordsW, HLINKGETREF_RELATIVE);
    ok(aMon != NULL, "Moniker from %s target should not be NULL\n", wine_dbgstr_w(aW));
    if(aMon)
        IMoniker_Release(aMon);

    /* setting target & location to the empty string deletes the moniker
     * reference */
    setStringRef(hlink, HLINKSETF_TARGET | HLINKSETF_LOCATION, emptyW, emptyW);
    getStringRef(hlink, NULL, NULL);
    getMonikerRef(hlink, NULL, NULL, HLINKGETREF_RELATIVE);

    /* setting a moniker target also sets the target string to that moniker's
     * display name */
    hres = CreateFileMoniker(bW, &file_mon);
    ok(hres == S_OK, "CreateFileMoniker failed: 0x%08lx\n", hres);

    setMonikerRef(hlink, HLINKSETF_TARGET | HLINKSETF_LOCATION, file_mon, wordsW);
    getStringRef(hlink, bW, wordsW);
    getMonikerRef(hlink, file_mon, wordsW, HLINKGETREF_RELATIVE);

    IMoniker_Release(file_mon);

    IHlink_Release(hlink);
}

static void test_HashLink(void)
{
    IHlink *hlink;
    IMoniker *pmk;
    const WCHAR hash_targetW[] = {'a','f','i','l','e','#','a','n','a','n','c','h','o','r',0};
    const WCHAR two_hash_targetW[] = {'a','f','i','l','e','#','a','n','a','n','c','h','o','r','#','a','n','o','t','h','e','r',0};
    const WCHAR hash_no_tgtW[] = {'#','a','n','a','n','c','h','o','r',0};
    const WCHAR tgt_partW[] = {'a','f','i','l','e',0};
    const WCHAR loc_partW[] = {'a','n','a','n','c','h','o','r',0};
    const WCHAR two_hash_loc_partW[] = {'a','n','a','n','c','h','o','r','#','a','n','o','t','h','e','r',0};
    const WCHAR test_locW[] = {'t','e','s','t','l','o','c',0};
    HRESULT hres;

    /* simple single hash test */
    hres = HlinkCreateFromString(hash_targetW, NULL, NULL, NULL, 0, NULL, &IID_IHlink, (void*)&hlink);
    ok(hres == S_OK, "HlinkCreateFromString failed: 0x%08lx\n", hres);
    ok(hlink != NULL, "Didn't get an hlink\n");

    if(hlink){
        getStringRef(hlink, tgt_partW, loc_partW);
        pmk = getMonikerRef(hlink, (IMoniker*)0xFFFFFFFF, loc_partW, HLINKGETREF_RELATIVE);
        ok(pmk != NULL, "Found moniker should not be NULL\n");
        if(pmk)
            IMoniker_Release(pmk);

        setStringRef(hlink, HLINKSETF_TARGET, hash_targetW, NULL);
        getStringRef(hlink, hash_targetW, loc_partW);

        IHlink_Release(hlink);
    }

    /* two hashes in the target */
    hres = HlinkCreateFromString(two_hash_targetW, NULL, NULL, NULL, 0, NULL, &IID_IHlink, (void*)&hlink);
    ok(hres == S_OK, "HlinkCreateFromString failed: 0x%08lx\n", hres);
    ok(hlink != NULL, "Didn't get an hlink\n");

    if(hlink){
        getStringRef(hlink, tgt_partW, two_hash_loc_partW);
        pmk = getMonikerRef(hlink, (IMoniker*)0xFFFFFFFF, two_hash_loc_partW, HLINKGETREF_RELATIVE);
        ok(pmk != NULL, "Found moniker should not be NULL\n");
        if(pmk)
            IMoniker_Release(pmk);

        IHlink_Release(hlink);
    }

    /* target with hash plus a location string */
    hres = HlinkCreateFromString(hash_targetW, test_locW, NULL, NULL, 0, NULL, &IID_IHlink, (void*)&hlink);
    ok(hres == S_OK, "HlinkCreateFromString failed: 0x%08lx\n", hres);
    ok(hlink != NULL, "Didn't get an hlink\n");

    if(hlink){
        getStringRef(hlink, tgt_partW, test_locW);
        pmk = getMonikerRef(hlink, (IMoniker*)0xFFFFFFFF, test_locW, HLINKGETREF_RELATIVE);
        ok(pmk != NULL, "Found moniker should not be NULL\n");
        if(pmk)
            IMoniker_Release(pmk);

        IHlink_Release(hlink);
    }

    /* target with hash containing no "target part" */
    hres = HlinkCreateFromString(hash_no_tgtW, NULL, NULL, NULL, 0, NULL, &IID_IHlink, (void*)&hlink);
    ok(hres == S_OK, "HlinkCreateFromString failed: 0x%08lx\n", hres);
    ok(hlink != NULL, "Didn't get an hlink\n");

    if(hlink){
        getStringRef(hlink, NULL, loc_partW);
        pmk = getMonikerRef(hlink, (IMoniker*)0xFFFFFFFF, loc_partW, HLINKGETREF_RELATIVE);
        ok(pmk == NULL, "Found moniker should be NULL\n");
        if(pmk)
            IMoniker_Release(pmk);

        IHlink_Release(hlink);
    }
}

static const WCHAR site_monikerW[] = {'S','I','T','E','_','M','O','N','I','K','E','R',0};
static const WCHAR ref_monikerW[] = {'R','E','F','_','M','O','N','I','K','E','R',0};

static HRESULT WINAPI hls_test_Moniker_BindToStorage(IMoniker *iface,
        IBindCtx *pbc, IMoniker *toLeft, REFIID riid, void **obj)
{
    ok(0, "BTS: %p %p %p %s %p\n", iface, pbc, toLeft, wine_dbgstr_guid(riid), obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI hls_site_Moniker_ComposeWith(IMoniker *iface,
        IMoniker *right, BOOL onlyIfNotGeneric, IMoniker **composite)
{
    LPOLESTR rightName;
    HRESULT hres;

    ok(onlyIfNotGeneric == 0, "Expected onlyIfNotGeneric to be FALSE\n");

    CHECK_EXPECT(ComposeWith);

    hres = IMoniker_GetDisplayName(right, NULL, NULL, &rightName);
    ok(hres == S_OK, "GetDisplayName failed: %08lx\n", hres);
    ok(!lstrcmpW(rightName, ref_monikerW),
            "Expected to get moniker set via SetMonikerReference, instead got: %s\n",
            wine_dbgstr_w(rightName));
    CoTaskMemFree(rightName);

    *composite = NULL;

    /* unlikely error code to verify this return result is used */
    return E_OUTOFMEMORY;
}

static HRESULT WINAPI hls_site_Moniker_GetDisplayName(IMoniker *iface,
        IBindCtx *pbc, IMoniker *toLeft, LPOLESTR *displayName)
{
    *displayName = CoTaskMemAlloc(sizeof(site_monikerW));
    memcpy(*displayName, site_monikerW, sizeof(site_monikerW));
    return S_OK;
}

static HRESULT WINAPI hls_ref_Moniker_GetDisplayName(IMoniker *iface,
        IBindCtx *pbc, IMoniker *toLeft, LPOLESTR *displayName)
{
    *displayName = CoTaskMemAlloc(sizeof(ref_monikerW));
    memcpy(*displayName, ref_monikerW, sizeof(ref_monikerW));
    return S_OK;
}

static HRESULT WINAPI hls_test_Moniker_IsSystemMoniker(IMoniker *iface,
        DWORD *mksys)
{
    return S_FALSE;
}

static IMonikerVtbl hls_site_MonikerVtbl = {
    Moniker_QueryInterface,
    Moniker_AddRef,
    Moniker_Release,
    Moniker_GetClassID,
    Moniker_IsDirty,
    Moniker_Load,
    Moniker_Save,
    Moniker_GetSizeMax,
    Moniker_BindToObject,
    hls_test_Moniker_BindToStorage,
    Moniker_Reduce,
    hls_site_Moniker_ComposeWith,
    Moniker_Enum,
    Moniker_IsEqual,
    Moniker_Hash,
    Moniker_IsRunning,
    Moniker_GetTimeOfLastChange,
    Moniker_Inverse,
    Moniker_CommonPrefixWith,
    Moniker_RelativePathTo,
    hls_site_Moniker_GetDisplayName,
    Moniker_ParseDisplayName,
    hls_test_Moniker_IsSystemMoniker
};

static IMonikerVtbl hls_ref_MonikerVtbl = {
    Moniker_QueryInterface,
    Moniker_AddRef,
    Moniker_Release,
    Moniker_GetClassID,
    Moniker_IsDirty,
    Moniker_Load,
    Moniker_Save,
    Moniker_GetSizeMax,
    Moniker_BindToObject,
    hls_test_Moniker_BindToStorage,
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
    hls_ref_Moniker_GetDisplayName,
    Moniker_ParseDisplayName,
    hls_test_Moniker_IsSystemMoniker
};

static IMoniker hls_site_Moniker = { &hls_site_MonikerVtbl };
static IMoniker hls_ref_Moniker = { &hls_ref_MonikerVtbl };

static HRESULT WINAPI hls_QueryInterface(IHlinkSite *iface, REFGUID iid,
        void **obj)
{
    ok(0, "QI: %p %s %p\n", iface, wine_dbgstr_guid(iid), obj);
    return E_NOTIMPL;
}

static ULONG WINAPI hls_AddRef(IHlinkSite *iface)
{
    return 2;
}

static ULONG WINAPI hls_Release(IHlinkSite *iface)
{
    return 1;
}

static HRESULT WINAPI hls_QueryService(IHlinkSite *iface, DWORD siteData,
        REFGUID service, REFIID riid, IUnknown **punk)
{
    ok(0, "QS: %p %lx %s %s %p\n", iface, siteData, wine_dbgstr_guid(service),
            wine_dbgstr_guid(riid), punk);
    return E_NOTIMPL;
}

#define SITEDATA_SUCCESS 1
#define SITEDATA_NOTIMPL 2

static HRESULT WINAPI hls_GetMoniker(IHlinkSite *iface, DWORD siteData,
        DWORD assign, DWORD which, IMoniker **pmk)
{
    ok(siteData == SITEDATA_NOTIMPL ||
            siteData == SITEDATA_SUCCESS, "Unexpected site data: %lu\n", siteData);

    if(siteData == SITEDATA_SUCCESS){
        *pmk = &hls_site_Moniker;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI hls_ReadyToNavigate(IHlinkSite *iface, DWORD siteData,
        DWORD reserved)
{
    ok(0, "RTN: %p %lx %lx\n", iface, siteData, reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI hls_OnNavigationComplete(IHlinkSite *iface,
        DWORD siteData, DWORD reserved, HRESULT error, LPCWSTR errorStr)
{
    CHECK_EXPECT(OnNavigationComplete);
    ok(siteData == SITEDATA_SUCCESS, "Unexpected site data: %lu\n", siteData);
    ok(error == E_OUTOFMEMORY, "Expected E_OUTOFMEMORY, got: %08lx\n", error);
    return E_NOTIMPL;
}

static IHlinkSiteVtbl HlinkSiteVtbl = {
    hls_QueryInterface,
    hls_AddRef,
    hls_Release,
    hls_QueryService,
    hls_GetMoniker,
    hls_ReadyToNavigate,
    hls_OnNavigationComplete
};

static IHlinkSite HlinkSite = { &HlinkSiteVtbl };

static void test_HlinkSite(void)
{
    IHlink *hl;
    IMoniker *mon_ref;
    IBindCtx *pbc;
    HRESULT hres;

    hres = HlinkCreateFromString(NULL, NULL, NULL, NULL, 0, NULL,
            &IID_IHlink, (LPVOID*)&hl);
    ok(hres == S_OK, "HlinkCreateFromString failed: %08lx\n", hres);
    getMonikerRef(hl, NULL, NULL, HLINKGETREF_RELATIVE);

    hres = IHlink_SetHlinkSite(hl, &HlinkSite, SITEDATA_SUCCESS);
    ok(hres == S_OK, "SetHlinkSite failed: %08lx\n", hres);
    getMonikerRef(hl, NULL, NULL, HLINKGETREF_RELATIVE);
    getStringRef(hl, NULL, NULL);

    hres = IHlink_GetMonikerReference(hl, HLINKGETREF_RELATIVE, &mon_ref, NULL);
    ok(hres == S_OK, "GetMonikerReference failed: %08lx\n", hres);
    ok(mon_ref == NULL, "Didn't get expected moniker, instead: %p\n", mon_ref);

    hres = IHlink_GetMonikerReference(hl, HLINKGETREF_ABSOLUTE, &mon_ref, NULL);
    ok(hres == S_OK, "GetMonikerReference failed: %08lx\n", hres);
    ok(mon_ref == &hls_site_Moniker, "Didn't get expected moniker, instead: %p\n", mon_ref);

    SET_EXPECT(Reduce);
    SET_EXPECT(Enum);
    hres = IHlink_SetMonikerReference(hl, HLINKSETF_TARGET, &hls_ref_Moniker, NULL);
    ok(hres == S_OK, "SetMonikerReference failed: %08lx\n", hres);
    todo_wine CHECK_CALLED(Reduce);
    todo_wine CHECK_CALLED(Enum);

    getMonikerRef(hl, &hls_ref_Moniker, NULL, HLINKGETREF_RELATIVE);

    SET_EXPECT(Enum);
    getStringRef(hl, ref_monikerW, NULL);
    todo_wine CHECK_CALLED(Enum);

    hres = IHlink_GetMonikerReference(hl, HLINKGETREF_RELATIVE, &mon_ref, NULL);
    ok(hres == S_OK, "GetMonikerReference failed: %08lx\n", hres);
    ok(mon_ref == &hls_ref_Moniker, "Didn't get expected moniker, instead: %p\n", mon_ref);
    IMoniker_Release(mon_ref);

    SET_EXPECT(ComposeWith);
    hres = IHlink_GetMonikerReference(hl, HLINKGETREF_ABSOLUTE, &mon_ref, NULL);
    ok(hres == E_OUTOFMEMORY, "Expected E_OUTOFMEMORY, got: %08lx\n", hres);
    ok(mon_ref == NULL, "Shouldn't have got a Moniker, got: %p\n", mon_ref);
    CHECK_CALLED(ComposeWith);

    hres = CreateBindCtx(0, &pbc);
    ok(hres == S_OK, "CreateBindCtx failed: %08lx\n", hres);

    SET_EXPECT(ComposeWith);
    SET_EXPECT(OnNavigationComplete);
    hres = IHlink_Navigate(hl, 0, pbc, NULL, NULL);
    ok(hres == E_OUTOFMEMORY, "Navigate should've failed: %08lx\n", hres);
    CHECK_CALLED(ComposeWith);
    CHECK_CALLED(OnNavigationComplete);

    IBindCtx_Release(pbc);
    IHlink_Release(hl);

    SET_EXPECT(Reduce);
    SET_EXPECT(Enum);
    hres = HlinkCreateFromMoniker(&hls_ref_Moniker, NULL, NULL, &HlinkSite, SITEDATA_SUCCESS,
            NULL, &IID_IHlink, (LPVOID*)&hl);
    ok(hres == S_OK, "HlinkCreateFromMoniker failed: %08lx\n", hres);
    todo_wine CHECK_CALLED(Reduce);
    todo_wine CHECK_CALLED(Enum);
    getMonikerRef(hl, &hls_ref_Moniker, NULL, HLINKGETREF_RELATIVE);
    IHlink_Release(hl);

    hres = HlinkCreateFromMoniker(NULL, NULL, NULL, &HlinkSite, SITEDATA_SUCCESS,
            NULL, &IID_IHlink, (LPVOID*)&hl);
    ok(hres == S_OK, "HlinkCreateFromMoniker failed: %08lx\n", hres);
    getMonikerRef(hl, NULL, NULL, HLINKGETREF_RELATIVE);
    IHlink_Release(hl);

    SET_EXPECT(Reduce);
    SET_EXPECT(Enum);
    SET_EXPECT(IsSystemMoniker);
    SET_EXPECT(GetDisplayName);
    hres = HlinkCreateFromMoniker(&Moniker, NULL, NULL, &HlinkSite, SITEDATA_NOTIMPL,
            NULL, &IID_IHlink, (LPVOID*)&hl);
    ok(hres == S_OK, "HlinkCreateFromMoniker failed: %08lx\n", hres);
    getMonikerRef(hl, &Moniker, NULL, HLINKGETREF_ABSOLUTE);
    IHlink_Release(hl);
    todo_wine CHECK_CALLED(Reduce);
    todo_wine CHECK_CALLED(Enum);
    CHECK_CALLED(IsSystemMoniker);
    CHECK_CALLED(GetDisplayName);
}

static void test_HlinkClone(void)
{
    HRESULT hres;
    IHlink *hl, *cloned = NULL;
    IMoniker *dummy, *fnd_mk;
    IHlinkSite *fnd_site;
    WCHAR *fnd_name;
    DWORD fnd_data;
    const WCHAR one[] = {'1',0};
    const WCHAR two[] = {'2',0};
    const WCHAR name[] = {'a',0};

    hres = HlinkClone(NULL, NULL, NULL, 0, NULL);
    ok(hres == E_INVALIDARG, "Got wrong failure code: %08lx\n", hres);

    hres = HlinkCreateFromString(NULL, NULL, NULL, NULL, 0, NULL,
            &IID_IHlink, (void**)&hl);
    ok(hres == S_OK, "HlinkCreateFromString failed: %08lx\n", hres);

    hres = HlinkClone(hl, &IID_IHlink, NULL, 0, NULL);
    ok(hres == E_INVALIDARG, "Got wrong failure code: %08lx\n", hres);

    if (0)
    { 
        /* crash on Windows XP */
        HlinkClone(hl, NULL, NULL, 0, NULL);

        HlinkClone(hl, NULL, NULL, 0, (void**)&cloned);
    }

    hres = HlinkClone(hl, &IID_IHlink, NULL, 0, (void**)&cloned);
    ok(hres == S_OK, "HlinkClone failed: %08lx\n", hres);
    ok(cloned != NULL, "Didn't get a clone\n");
    getMonikerRef(cloned, NULL, NULL, HLINKGETREF_RELATIVE);
    IHlink_Release(cloned);

    IHlink_Release(hl);

    SET_EXPECT(Reduce);
    SET_EXPECT(Enum);
    hres = HlinkCreateFromMoniker(&hls_ref_Moniker, two, NULL, NULL, 0, NULL, &IID_IHlink, (void**)&hl);
    todo_wine CHECK_CALLED(Reduce);
    todo_wine CHECK_CALLED(Enum);
    ok(hres == S_OK, "HlinkCreateFromMoniker failed: 0x%08lx\n", hres);
    getMonikerRef(hl, &hls_ref_Moniker, two, HLINKGETREF_RELATIVE);

    SET_EXPECT(Save);
    SET_EXPECT(GetClassID);
    cloned = (IHlink*)0xdeadbeef;
    hres = HlinkClone(hl, &IID_IHlink, NULL, 0, (void**)&cloned);
    /* fails because of invalid CLSID given by Moniker_GetClassID */
    ok(hres == REGDB_E_CLASSNOTREG, "Wrong error code: %08lx\n", hres);
    ok(cloned == NULL, "Shouldn't have gotten a clone\n");
    CHECK_CALLED(Save);
    CHECK_CALLED(GetClassID);

    IHlink_Release(hl);

    hres = CreateItemMoniker(one, one, &dummy);
    ok(hres == S_OK, "CreateItemMoniker failed: 0x%08lx\n", hres);

    hres = HlinkCreateFromMoniker(dummy, two, name, &HlinkSite, SITEDATA_SUCCESS, NULL, &IID_IHlink, (void**)&hl);
    ok(hres == S_OK, "HlinkCreateFromMoniker failed: 0x%08lx\n", hres);
    getMonikerRef(hl, dummy, two, HLINKGETREF_RELATIVE);

    cloned = NULL;
    hres = HlinkClone(hl, &IID_IHlink, NULL, 0, (void**)&cloned);
    ok(hres == S_OK, "HlinkClone failed: %08lx\n", hres);
    ok(cloned != NULL, "Should have gotten a clone\n");

    fnd_mk = getMonikerRef(cloned, (IMoniker*)0xFFFFFFFF, two, HLINKGETREF_RELATIVE);
    ok(fnd_mk != NULL, "Expected non-null Moniker\n");
    ok(fnd_mk != dummy, "Expected a new Moniker to be created\n");

    fnd_name = NULL;
    hres = IHlink_GetFriendlyName(cloned, HLFNAMEF_DEFAULT, &fnd_name);
    ok(hres == S_OK, "GetFriendlyName failed: %08lx\n", hres);
    ok(fnd_name != NULL, "Expected friendly name to be non-NULL\n");
    ok(lstrcmpW(fnd_name, name) == 0, "Expected friendly name to be %s, was %s\n",
            wine_dbgstr_w(name), wine_dbgstr_w(fnd_name));
    CoTaskMemFree(fnd_name);

    fnd_site = (IHlinkSite*)0xdeadbeef;
    fnd_data = 4;
    hres = IHlink_GetHlinkSite(cloned, &fnd_site, &fnd_data);
    ok(hres == S_OK, "GetHlinkSite failed: %08lx\n", hres);
    ok(fnd_site == NULL, "Expected NULL site\n");
    ok(fnd_data == 4, "Expected site data to be 4, was: %ld\n", fnd_data);

    IHlink_Release(cloned);
    IHlink_Release(hl);

    hres = HlinkCreateFromMoniker(dummy, NULL, NULL, NULL, 0, NULL, &IID_IHlink, (void**)&hl);
    ok(hres == S_OK, "HlinkCreateFromMoniker failed: 0x%08lx\n", hres);
    getMonikerRef(hl, dummy, NULL, HLINKGETREF_RELATIVE);

    cloned = NULL;
    hres = HlinkClone(hl, &IID_IHlink, &HlinkSite, SITEDATA_SUCCESS, (void**)&cloned);
    ok(hres == S_OK, "HlinkClone failed: %08lx\n", hres);
    ok(cloned != NULL, "Should have gotten a clone\n");

    fnd_mk = getMonikerRef(cloned, (IMoniker*)0xFFFFFFFF, NULL, HLINKGETREF_RELATIVE);
    ok(fnd_mk != NULL, "Expected non-null Moniker\n");
    ok(fnd_mk != dummy, "Expected a new Moniker to be created\n");

    fnd_site = (IHlinkSite*)0xdeadbeef;
    fnd_data = 4;
    hres = IHlink_GetHlinkSite(cloned, &fnd_site, &fnd_data);
    ok(hres == S_OK, "GetHlinkSite failed: %08lx\n", hres);
    ok(fnd_site == &HlinkSite, "Expected found site to be HlinkSite, was: %p\n", fnd_site);
    ok(fnd_data == SITEDATA_SUCCESS, "Unexpected site data: %lu\n", fnd_data);

    IHlink_Release(cloned);
    IHlink_Release(hl);

    IMoniker_Release(dummy);
}

static void test_StdHlink(void)
{
    IHlink *hlink;
    WCHAR *str;
    HRESULT hres;

    static const WCHAR testW[] = {'t','e','s','t',0};

    hres = CoCreateInstance(&CLSID_StdHlink, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHlink, (void**)&hlink);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);

    str = (void*)0xdeadbeef;
    hres = IHlink_GetTargetFrameName(hlink, &str);
    ok(hres == S_FALSE, "GetTargetFrameName failed: %08lx\n", hres);
    ok(!str, "str = %s\n", wine_dbgstr_w(str));

    hres = IHlink_SetTargetFrameName(hlink, testW);
    ok(hres == S_OK, "SetTargetFrameName failed: %08lx\n", hres);

    str = (void*)0xdeadbeef;
    hres = IHlink_GetTargetFrameName(hlink, &str);
    ok(hres == S_OK, "GetTargetFrameName failed: %08lx\n", hres);
    ok(!lstrcmpW(str, testW), "str = %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    hres = IHlink_SetTargetFrameName(hlink, NULL);
    ok(hres == S_OK, "SetTargetFrameName failed: %08lx\n", hres);

    str = (void*)0xdeadbeef;
    hres = IHlink_GetTargetFrameName(hlink, &str);
    ok(hres == S_FALSE, "GetTargetFrameName failed: %08lx\n", hres);
    ok(!str, "str = %s\n", wine_dbgstr_w(str));

    IHlink_Release(hlink);
}

static void test_Hlink_Navigate(void)
{
    BINDINFO bind_info = {sizeof(BINDINFO)};
    DWORD bind_flags;
    IHlink *hlink;
    IBindCtx *pbc;
    HRESULT hres;

    hres = CreateBindCtx(0, &pbc);
    ok(hres == S_OK, "CreateBindCtx failed: %08lx\n", hres);
    _bctx = pbc;

    HBC_object = NULL;

    SET_EXPECT(Reduce);
    SET_EXPECT(Enum);
    SET_EXPECT(IsSystemMoniker);
    SET_EXPECT(GetDisplayName);
    hres = HlinkCreateFromMoniker(&Moniker, NULL, NULL, NULL,
            0, NULL, &IID_IHlink, (void**)&hlink);
    ok(hres == S_OK, "HlinkCreateFromMoniker failed: %08lx\n", hres);
    todo_wine CHECK_CALLED(Reduce);
    todo_wine CHECK_CALLED(Enum);
    todo_wine CHECK_CALLED(IsSystemMoniker);
    CHECK_CALLED(GetDisplayName);

    SET_EXPECT(IsSystemMoniker);
    SET_EXPECT(GetDisplayName);
    SET_EXPECT(HBC_GetObject);
    SET_EXPECT(Reduce);
    SET_EXPECT(BindToObject);
    SET_EXPECT(HT_QueryInterface_IHlinkTarget);
    SET_EXPECT(HT_GetBrowseContext);
    SET_EXPECT(HT_SetBrowseContext);
    SET_EXPECT(HBC_QueryInterface_IHlinkHistory);
    SET_EXPECT(HT_Navigate);
    SET_EXPECT(HT_GetFriendlyName);
    hres = IHlink_Navigate(hlink, 0, pbc, NULL, &HlinkBrowseContext);
    ok(hres == S_OK, "Navigate failed: %08lx\n", hres);
    CHECK_CALLED(IsSystemMoniker);
    CHECK_CALLED(GetDisplayName);
    CHECK_CALLED(HBC_GetObject);
    todo_wine CHECK_CALLED(Reduce);
    CHECK_CALLED(BindToObject);
    CHECK_CALLED(HT_QueryInterface_IHlinkTarget);
    todo_wine CHECK_CALLED(HT_GetBrowseContext);
    CHECK_CALLED(HT_SetBrowseContext);
    todo_wine CHECK_CALLED(HBC_QueryInterface_IHlinkHistory);
    CHECK_CALLED(HT_Navigate);
    todo_wine CHECK_CALLED(HT_GetFriendlyName);

    /* Test with valid return from HlinkBrowseContext::GetObject */
    HBC_object = (IUnknown *)&HlinkTarget;

    SET_EXPECT(IsSystemMoniker);
    SET_EXPECT(GetDisplayName);
    SET_EXPECT(HBC_GetObject);
    SET_EXPECT(HT_QueryInterface_IHlinkTarget);
    SET_EXPECT(HT_Navigate);
    SET_EXPECT(HT_GetFriendlyName);
    hres = IHlink_Navigate(hlink, 0, pbc, NULL, &HlinkBrowseContext);
    ok(hres == S_OK, "Navigate failed: %08lx\n", hres);
    CHECK_CALLED(IsSystemMoniker);
    CHECK_CALLED(GetDisplayName);
    CHECK_CALLED(HBC_GetObject);
    CHECK_CALLED(HT_QueryInterface_IHlinkTarget);
    CHECK_CALLED(HT_Navigate);
    todo_wine CHECK_CALLED(HT_GetFriendlyName);

    HBC_object = NULL;

if (0) {    /* these currently open a browser window on wine */
    /* Test from string */
    SET_EXPECT(HBC_GetObject);
    hres = HlinkNavigateToStringReference(winehq_404W, NULL, NULL, 0, NULL, 0, pbc, NULL, &HlinkBrowseContext);
    todo_wine ok(hres == INET_E_OBJECT_NOT_FOUND, "Expected INET_E_OBJECT_NOT_FOUND, got %08lx\n", hres);
    CHECK_CALLED(HBC_GetObject);

    /* MSDN claims browse context and bind context can't be null, but they can */
    SET_EXPECT(HBC_GetObject);
    hres = HlinkNavigateToStringReference(winehq_404W, NULL, NULL, 0, NULL, 0, NULL, NULL, &HlinkBrowseContext);
    todo_wine ok(hres == INET_E_OBJECT_NOT_FOUND, "Expected INET_E_OBJECT_NOT_FOUND, got %08lx\n", hres);
    CHECK_CALLED(HBC_GetObject);
}

    /* these open a browser window, so mark them interactive only */
    if (winetest_interactive)
    {
        /* both parameters null */
        SET_EXPECT(IsSystemMoniker);
        SET_EXPECT(GetDisplayName);
        hres = IHlink_Navigate(hlink, 0, NULL, NULL, NULL);
        ok(hres == DRAGDROP_S_DROP, "Expected DRAGDROP_S_DROP, got %08lx\n", hres);
        CHECK_CALLED(IsSystemMoniker);
        CHECK_CALLED(GetDisplayName);

        /* same, from string */
        hres = HlinkNavigateToStringReference(winehq_404W, NULL, NULL, 0, NULL, 0, NULL, NULL, NULL);
        ok(hres == DRAGDROP_S_DROP, "Expected DRAGDROP_S_DROP, got %08lx\n", hres);

        /* try basic test with valid URL */
        SET_EXPECT(HBC_GetObject);
        SET_EXPECT(HBC_QueryInterface_IHlinkHistory);
        SET_EXPECT(HBC_QueryInterface_IMarshal);
        SET_EXPECT(HBC_QueryInterface_IdentityUnmarshal);
        SET_EXPECT(HBC_QueryInterface_IUnknown);
        hres = HlinkNavigateToStringReference(winehq_urlW, NULL, NULL, 0, NULL, 0, pbc, NULL, &HlinkBrowseContext);
        ok(hres == S_OK, "Expected S_OK, got %08lx\n", hres);
        CHECK_CALLED(HBC_GetObject);
        todo_wine CHECK_CALLED(HBC_QueryInterface_IHlinkHistory);
        todo_wine CHECK_CALLED(HBC_QueryInterface_IMarshal);
        todo_wine CHECK_CALLED(HBC_QueryInterface_IdentityUnmarshal);
        todo_wine CHECK_CALLED(HBC_QueryInterface_IUnknown);
    }
    else
        skip("interactive IHlink_Navigate tests\n");

    /* test binding callback */
    SET_EXPECT(IsSystemMoniker);
    SET_EXPECT(GetDisplayName);
    SET_EXPECT(BindStatusCallback_GetBindInfo);
    SET_EXPECT(HBC_GetObject);
    SET_EXPECT(Reduce);
    SET_EXPECT(BindToObject);
    SET_EXPECT(BindStatusCallback_OnStartBinding);
    SET_EXPECT(BindStatusCallback_OnObjectAvailable);
    SET_EXPECT(HT_QueryInterface_IHlinkTarget);
    SET_EXPECT(HT_GetBrowseContext);
    SET_EXPECT(HT_SetBrowseContext);
    SET_EXPECT(HBC_QueryInterface_IHlinkHistory);
    SET_EXPECT(HT_Navigate);
    SET_EXPECT(HT_GetFriendlyName);
    SET_EXPECT(BindStatusCallback_OnStopBinding);
    hres = IHlink_Navigate(hlink, 0, pbc, &BindStatusCallback, &HlinkBrowseContext);
    ok(hres == S_OK, "Navigate failed: %#lx\n", hres);
    CHECK_CALLED(IsSystemMoniker);
    CHECK_CALLED(GetDisplayName);
    CHECK_CALLED(HBC_GetObject);
    todo_wine
    CHECK_CALLED(BindStatusCallback_GetBindInfo);
    todo_wine
    CHECK_CALLED(Reduce);
    CHECK_CALLED(BindToObject);
todo_wine {
    CHECK_CALLED(BindStatusCallback_OnStartBinding);
    CHECK_CALLED(BindStatusCallback_OnObjectAvailable);
}
    CHECK_CALLED(HT_QueryInterface_IHlinkTarget);
    todo_wine
    CHECK_CALLED(HT_GetBrowseContext);
    CHECK_CALLED(HT_SetBrowseContext);
    todo_wine
    CHECK_CALLED(HBC_QueryInterface_IHlinkHistory);
    CHECK_CALLED(HT_Navigate);
    todo_wine
    CHECK_CALLED(HT_GetFriendlyName);
    todo_wine
    CHECK_CALLED(BindStatusCallback_OnStopBinding);

    ok(bind_callback_refs == 1, "Got unexpected refcount %ld.\n", bind_callback_refs);
    ok(browse_ctx_refs == 1, "Got unexpected refcount %ld.\n", browse_ctx_refs);

    /* test asynchronous binding */
    async_bind = TRUE;
    SET_EXPECT(IsSystemMoniker);
    SET_EXPECT(GetDisplayName);
    SET_EXPECT(HBC_GetObject);
    SET_EXPECT(Reduce);
    SET_EXPECT(BindToObject);
    hres = IHlink_Navigate(hlink, 0, pbc, NULL, &HlinkBrowseContext);
    ok(hres == MK_S_ASYNCHRONOUS, "Navigate failed: %#lx\n", hres);
    CHECK_CALLED(IsSystemMoniker);
    CHECK_CALLED(GetDisplayName);
    CHECK_CALLED(HBC_GetObject);
    todo_wine
    CHECK_CALLED(Reduce);
    CHECK_CALLED(BindToObject);

    ok(browse_ctx_refs > 1, "Got unexpected refcount %ld.\n", browse_ctx_refs);

    hres = IHlink_Navigate(hlink, 0, pbc, NULL, &HlinkBrowseContext);
    ok(hres == E_UNEXPECTED, "Got hr %#lx.\n", hres);

    hres = IBindStatusCallback_GetBindInfo(async_bind_callback, &bind_flags, &bind_info);
    ok(hres == S_OK, "Got hr %#lx.\n", hres);

    hres = IBindStatusCallback_OnStartBinding(async_bind_callback, 0, NULL);
    ok(hres == S_OK, "Got hr %#lx.\n", hres);

    SET_EXPECT(HT_QueryInterface_IHlinkTarget);
    SET_EXPECT(HT_GetBrowseContext);
    SET_EXPECT(HT_SetBrowseContext);
    SET_EXPECT(HBC_QueryInterface_IHlinkHistory);
    SET_EXPECT(HT_Navigate);
    SET_EXPECT(HT_GetFriendlyName);
    hres = IBindStatusCallback_OnObjectAvailable(async_bind_callback, &IID_IUnknown,
            (IUnknown *)&HlinkTarget);
    ok(hres == S_OK, "Got hr %#lx.\n", hres);
    CHECK_CALLED(HT_QueryInterface_IHlinkTarget);
    todo_wine
    CHECK_CALLED(HT_GetBrowseContext);
    CHECK_CALLED(HT_SetBrowseContext);
    todo_wine
    CHECK_CALLED(HBC_QueryInterface_IHlinkHistory);
    CHECK_CALLED(HT_Navigate);
    todo_wine
    CHECK_CALLED(HT_GetFriendlyName);

    hres = IHlink_Navigate(hlink, 0, pbc, NULL, &HlinkBrowseContext);
    ok(hres == E_UNEXPECTED, "Got hr %#lx.\n", hres);

    ok(browse_ctx_refs > 1, "Got unexpected refcount %ld.\n", browse_ctx_refs);

    hres = IBindStatusCallback_OnStopBinding(async_bind_callback, S_OK, NULL);
    ok(hres == S_OK, "Got hr %#lx.\n", hres);

    ok(browse_ctx_refs == 1, "Got unexpected refcount %ld.\n", browse_ctx_refs);

    IBindStatusCallback_Release(async_bind_callback);

    SET_EXPECT(IsSystemMoniker);
    SET_EXPECT(GetDisplayName);
    SET_EXPECT(BindStatusCallback_GetBindInfo);
    SET_EXPECT(HBC_GetObject);
    SET_EXPECT(Reduce);
    SET_EXPECT(BindToObject);
    hres = IHlink_Navigate(hlink, 0, pbc, &BindStatusCallback, &HlinkBrowseContext);
    ok(hres == MK_S_ASYNCHRONOUS, "Navigate failed: %#lx\n", hres);
    CHECK_CALLED(IsSystemMoniker);
    CHECK_CALLED(GetDisplayName);
    todo_wine
    CHECK_CALLED(BindStatusCallback_GetBindInfo);
    CHECK_CALLED(HBC_GetObject);
    todo_wine
    CHECK_CALLED(Reduce);
    CHECK_CALLED(BindToObject);

    ok(bind_callback_refs > 1, "Got unexpected refcount %ld.\n", bind_callback_refs);
    ok(browse_ctx_refs > 1, "Got unexpected refcount %ld.\n", browse_ctx_refs);

    hres = IHlink_Navigate(hlink, 0, pbc, NULL, &HlinkBrowseContext);
    ok(hres == E_UNEXPECTED, "Got hr %#lx.\n", hres);

    SET_EXPECT(BindStatusCallback_GetBindInfo);
    hres = IBindStatusCallback_GetBindInfo(async_bind_callback, &bind_flags, &bind_info);
    ok(hres == E_NOTIMPL, "Got hr %#lx.\n", hres);
    CHECK_CALLED(BindStatusCallback_GetBindInfo);

    SET_EXPECT(BindStatusCallback_OnStartBinding);
    hres = IBindStatusCallback_OnStartBinding(async_bind_callback, 0, NULL);
    ok(hres == S_OK, "Got hr %#lx.\n", hres);
    CHECK_CALLED(BindStatusCallback_OnStartBinding);

    SET_EXPECT(BindStatusCallback_OnObjectAvailable);
    SET_EXPECT(HT_QueryInterface_IHlinkTarget);
    SET_EXPECT(HT_GetBrowseContext);
    SET_EXPECT(HT_SetBrowseContext);
    SET_EXPECT(HBC_QueryInterface_IHlinkHistory);
    SET_EXPECT(HT_Navigate);
    SET_EXPECT(HT_GetFriendlyName);
    hres = IBindStatusCallback_OnObjectAvailable(async_bind_callback, &IID_IUnknown,
            (IUnknown *)&HlinkTarget);
    ok(hres == S_OK, "Got hr %#lx.\n", hres);
    CHECK_CALLED(BindStatusCallback_OnObjectAvailable);
    CHECK_CALLED(HT_QueryInterface_IHlinkTarget);
    todo_wine
    CHECK_CALLED(HT_GetBrowseContext);
    CHECK_CALLED(HT_SetBrowseContext);
    todo_wine
    CHECK_CALLED(HBC_QueryInterface_IHlinkHistory);
    CHECK_CALLED(HT_Navigate);
    todo_wine
    CHECK_CALLED(HT_GetFriendlyName);

    hres = IHlink_Navigate(hlink, 0, pbc, NULL, &HlinkBrowseContext);
    ok(hres == E_UNEXPECTED, "Got hr %#lx.\n", hres);

    ok(bind_callback_refs > 1, "Got unexpected refcount %ld.\n", bind_callback_refs);
    ok(browse_ctx_refs > 1, "Got unexpected refcount %ld.\n", browse_ctx_refs);

    SET_EXPECT(BindStatusCallback_OnStopBinding);
    hres = IBindStatusCallback_OnStopBinding(async_bind_callback, S_OK, NULL);
    ok(hres == S_OK, "Got hr %#lx.\n", hres);
    CHECK_CALLED(BindStatusCallback_OnStopBinding);

    ok(bind_callback_refs == 1, "Got unexpected refcount %ld.\n", bind_callback_refs);
    ok(browse_ctx_refs == 1, "Got unexpected refcount %ld.\n", browse_ctx_refs);

    IBindStatusCallback_Release(async_bind_callback);

    IHlink_Release(hlink);
    IBindCtx_Release(pbc);
    _bctx = NULL;
}

static HRESULT WINAPI hlinkframe_QueryInterface(IHlinkFrame *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IHlinkFrame))
    {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI hlinkframe_AddRef(IHlinkFrame *iface)
{
    return 2;
}

static ULONG WINAPI hlinkframe_Release(IHlinkFrame *iface)
{
    return 1;
}

static HRESULT WINAPI hlinkframe_SetBrowseContext(IHlinkFrame *iface, IHlinkBrowseContext *bc)
{
    ok(0, "unexpected %p\n", bc);
    return E_NOTIMPL;
}

static HRESULT WINAPI hlinkframe_GetBrowseContext(IHlinkFrame *iface, IHlinkBrowseContext **bc)
{
    *bc = NULL;
    ok(0, "unexpected %p\n", bc);
    return E_NOTIMPL;
}

static HRESULT WINAPI hlinkframe_Navigate(IHlinkFrame *iface, DWORD grfHLNF, LPBC pbc, IBindStatusCallback *bsc, IHlink *navigate)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI hlinkframe_OnNavigate(IHlinkFrame *iface, DWORD grfHLNF, IMoniker *target, LPCWSTR location, LPCWSTR friendly_name,
    DWORD reserved)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI hlinkframe_UpdateHlink(IHlinkFrame *iface, ULONG uHLID, IMoniker *target, LPCWSTR location, LPCWSTR friendly_name)
{
    CHECK_EXPECT(HLF_UpdateHlink);
    return S_OK;
}

static IHlinkFrameVtbl hlinkframevtbl = {
    hlinkframe_QueryInterface,
    hlinkframe_AddRef,
    hlinkframe_Release,
    hlinkframe_SetBrowseContext,
    hlinkframe_GetBrowseContext,
    hlinkframe_Navigate,
    hlinkframe_OnNavigate,
    hlinkframe_UpdateHlink
};

static IHlinkFrame testframe = { &hlinkframevtbl };

static void test_HlinkUpdateStackItem(void)
{
    static const WCHAR location[] = {'l','o','c','a','t','i','o','n',0};
    HRESULT hr;

    hr = HlinkUpdateStackItem(NULL, NULL, HLID_CURRENT, &Moniker, location, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    SET_EXPECT(HBC_UpdateHlink);
    hr = HlinkUpdateStackItem(NULL, &HlinkBrowseContext, HLID_CURRENT, &Moniker, location, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    CHECK_CALLED(HBC_UpdateHlink);

    SET_EXPECT(HLF_UpdateHlink);
    hr = HlinkUpdateStackItem(&testframe, &HlinkBrowseContext, HLID_CURRENT, &Moniker, location, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    CHECK_CALLED(HLF_UpdateHlink);
}

static void test_HlinkCreateFromMoniker(void)
{
    IPersistStream *stream;
    IMoniker *moniker;
    HRESULT hr;

    hr = CreateItemMoniker(L"1", L"1", &moniker);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = HlinkCreateFromMoniker(moniker, L"1", L"a", NULL, 0, NULL, &IID_IPersistStream, (void **)&stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IPersistStream_Release(stream);

    IMoniker_Release(moniker);
}

static void test_HlinkCreateFromString(void)
{
    IPersistStream *stream;
    HRESULT hr;

    hr = HlinkCreateFromString(L"http://winehq.org", NULL, NULL, NULL, 0, NULL, &IID_IPersistStream, (void **)&stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IPersistStream_Release(stream);
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
    test_HlinkGetSetMonikerReference();
    test_HlinkGetSetStringReference();
    test_HlinkMoniker();
    test_HashLink();
    test_HlinkSite();
    test_HlinkClone();
    test_StdHlink();
    test_Hlink_Navigate();
    test_HlinkUpdateStackItem();
    test_HlinkCreateFromMoniker();
    test_HlinkCreateFromString();

    CoUninitialize();
}
