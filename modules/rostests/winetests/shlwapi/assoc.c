/* Unit test suite for SHLWAPI IQueryAssociations functions
 *
 * Copyright 2008 Google (Lei Zhang)
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

#include "wine/test.h"
#include "shlwapi.h"
#include "shlguid.h"

#define expect(expected, got) ok( (expected) == (got), "Expected %d, got %d\n", (expected), (got))
#define expect_hr(expected, got) ok( (expected) == (got), "Expected %08x, got %08x\n", (expected), (got))

static HRESULT (WINAPI *pAssocQueryStringA)(ASSOCF,ASSOCSTR,LPCSTR,LPCSTR,LPSTR,LPDWORD) = NULL;
static HRESULT (WINAPI *pAssocQueryStringW)(ASSOCF,ASSOCSTR,LPCWSTR,LPCWSTR,LPWSTR,LPDWORD) = NULL;
static HRESULT (WINAPI *pAssocCreate)(CLSID, REFIID, void **) = NULL;
static HRESULT (WINAPI *pAssocGetPerceivedType)(LPCWSTR, PERCEIVED *, INT *, LPWSTR *) = NULL;

/* Every version of Windows with IE should have this association? */
static const WCHAR dotHtml[] = { '.','h','t','m','l',0 };
static const WCHAR badBad[] = { 'b','a','d','b','a','d',0 };
static const WCHAR dotBad[] = { '.','b','a','d',0 };
static const WCHAR open[] = { 'o','p','e','n',0 };
static const WCHAR invalid[] = { 'i','n','v','a','l','i','d',0 };

static void test_getstring_bad(void)
{
    static const WCHAR openwith[] = {'O','p','e','n','W','i','t','h','.','e','x','e',0};
    WCHAR buf[MAX_PATH];
    HRESULT hr;
    DWORD len;

    if (!pAssocQueryStringW)
    {
        win_skip("AssocQueryStringW() is missing\n");
        return;
    }

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, NULL, open, NULL, &len);
    expect_hr(E_INVALIDARG, hr);
    ok(len == 0xdeadbeef, "got %u\n", len);

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, badBad, open, NULL, &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);

    len = ARRAY_SIZE(buf);
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotBad, open, buf, &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */ ||
       hr == S_OK /* Win8 */,
       "Unexpected result : %08x\n", hr);
    if (hr == S_OK)
    {
        ok(len < ARRAY_SIZE(buf), "got %u\n", len);
        ok(!lstrcmpiW(buf + len - ARRAY_SIZE(openwith), openwith), "wrong data\n");
    }

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, invalid, NULL, &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);

    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, open, NULL, NULL);
    ok(hr == E_UNEXPECTED ||
       hr == E_INVALIDARG, /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, NULL, open, NULL, &len);
    expect_hr(E_INVALIDARG, hr);
    ok(len == 0xdeadbeef, "got %u\n", len);

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, badBad, open, NULL, &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotBad, open, NULL, &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */ ||
       hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND) /* Win8 */ ||
       hr == S_FALSE, /* Win10 */
       "Unexpected result : %08x\n", hr);
    ok((hr == S_FALSE && len < ARRAY_SIZE(buf)) || len == 0xdeadbeef,
       "got hr=%08x and len=%u\n", hr, len);

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, invalid, NULL, &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) || /* W2K/Vista/W2K8 */
       hr == E_FAIL, /* Win9x/WinMe/NT4 */
       "Unexpected result : %08x\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);

    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, open, NULL, NULL);
    ok(hr == E_UNEXPECTED ||
       hr == E_INVALIDARG, /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);
}

static void test_getstring_basic(void)
{
    HRESULT hr;
    WCHAR * friendlyName;
    WCHAR * executableName;
    DWORD len, len2, slen;

    if (!pAssocQueryStringW)
    {
        win_skip("AssocQueryStringW() is missing\n");
        return;
    }

    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, open, NULL, &len);
    expect_hr(S_FALSE, hr);
    if (hr != S_FALSE)
    {
        skip("failed to get initial len\n");
        return;
    }

    executableName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                               len * sizeof(WCHAR));
    if (!executableName)
    {
        skip("failed to allocate memory\n");
        return;
    }

    len2 = len;
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, open,
                           executableName, &len2);
    expect_hr(S_OK, hr);
    slen = lstrlenW(executableName) + 1;
    expect(len, len2);
    expect(len, slen);

    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, open, NULL,
                           &len);
    ok(hr == S_FALSE ||
       hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* Win9x/NT4 */ ||
       hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), /* Win8 */
       "Unexpected result : %08x\n", hr);
    if (hr != S_FALSE)
    {
        HeapFree(GetProcessHeap(), 0, executableName);
        skip("failed to get initial len\n");
        return;
    }

    friendlyName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                               len * sizeof(WCHAR));
    if (!friendlyName)
    {
        HeapFree(GetProcessHeap(), 0, executableName);
        skip("failed to allocate memory\n");
        return;
    }

    len2 = len;
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, open,
                           friendlyName, &len2);
    expect_hr(S_OK, hr);
    slen = lstrlenW(friendlyName) + 1;
    expect(len, len2);
    expect(len, slen);

    HeapFree(GetProcessHeap(), 0, executableName);
    HeapFree(GetProcessHeap(), 0, friendlyName);
}

static void test_getstring_no_extra(void)
{
    LONG ret;
    HKEY hkey;
    HRESULT hr;
    static const CHAR dotWinetest[] = {
        '.','w','i','n','e','t','e','s','t',0
    };
    static const CHAR winetestfile[] = {
        'w','i','n','e','t','e','s','t', 'f','i','l','e',0
    };
    static const CHAR winetestfileAction[] = {
        'w','i','n','e','t','e','s','t','f','i','l','e',
        '\\','s','h','e','l','l',
        '\\','f','o','o',
        '\\','c','o','m','m','a','n','d',0
    };
    static const CHAR action[] = {
        'n','o','t','e','p','a','d','.','e','x','e',0
    };
    CHAR buf[MAX_PATH];
    DWORD len = MAX_PATH;

    if (!pAssocQueryStringA)
    {
        win_skip("AssocQueryStringA() is missing\n");
        return;
    }

    buf[0] = '\0';
    ret = RegCreateKeyA(HKEY_CLASSES_ROOT, dotWinetest, &hkey);
    if (ret != ERROR_SUCCESS) {
        skip("failed to create dotWinetest key\n");
        return;
    }

    ret = RegSetValueA(hkey, NULL, REG_SZ, winetestfile, lstrlenA(winetestfile));
    RegCloseKey(hkey);
    if (ret != ERROR_SUCCESS)
    {
        skip("failed to set dotWinetest key\n");
        goto cleanup;
    }

    ret = RegCreateKeyA(HKEY_CLASSES_ROOT, winetestfileAction, &hkey);
    if (ret != ERROR_SUCCESS)
    {
        skip("failed to create winetestfileAction key\n");
        goto cleanup;
    }

    ret = RegSetValueA(hkey, NULL, REG_SZ, action, lstrlenA(action));
    RegCloseKey(hkey);
    if (ret != ERROR_SUCCESS)
    {
        skip("failed to set winetestfileAction key\n");
        goto cleanup;
    }

    hr = pAssocQueryStringA(0, ASSOCSTR_EXECUTABLE, dotWinetest, NULL, buf, &len);
    ok(hr == S_OK ||
       hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), /* XP and W2K3 */
       "Unexpected result : %08x\n", hr);
    hr = pAssocQueryStringA(0, ASSOCSTR_EXECUTABLE, dotWinetest, "foo", buf, &len);
    expect_hr(S_OK, hr);
    ok(strstr(buf, action) != NULL,
        "got '%s' (Expected result to include 'notepad.exe')\n", buf);

cleanup:
    SHDeleteKeyA(HKEY_CLASSES_ROOT, dotWinetest);
    SHDeleteKeyA(HKEY_CLASSES_ROOT, winetestfile);

}

static void test_assoc_create(void)
{
    HRESULT hr;
    IQueryAssociations *pqa;

    if (!pAssocCreate)
    {
        win_skip("AssocCreate() is missing\n");
        return;
    }

    hr = pAssocCreate(IID_NULL, &IID_NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected result : %08x\n", hr);

    hr = pAssocCreate(CLSID_QueryAssociations, &IID_NULL, (LPVOID*)&pqa);
    ok(hr == CLASS_E_CLASSNOTAVAILABLE || hr == E_NOTIMPL || hr == E_NOINTERFACE
        , "Unexpected result : %08x\n", hr);

    hr = pAssocCreate(IID_NULL, &IID_IQueryAssociations, (LPVOID*)&pqa);
    ok(hr == CLASS_E_CLASSNOTAVAILABLE || hr == E_NOTIMPL || hr == E_INVALIDARG
        , "Unexpected result : %08x\n", hr);

    hr = pAssocCreate(CLSID_QueryAssociations, &IID_IQueryAssociations, (LPVOID*)&pqa);
    ok(hr == S_OK  || hr == E_NOTIMPL /* win98 */
        , "Unexpected result : %08x\n", hr);
    if(hr == S_OK)
    {
        IQueryAssociations_Release(pqa);
    }

    hr = pAssocCreate(CLSID_QueryAssociations, &IID_IUnknown, (LPVOID*)&pqa);
    ok(hr == S_OK  || hr == E_NOTIMPL /* win98 */
        , "Unexpected result : %08x\n", hr);
    if(hr == S_OK)
    {
        IQueryAssociations_Release(pqa);
    }
}

/* Based on http://www.geoffchappell.com/studies/windows/shell/shlwapi/api/assocapi/getperceivedtype.htm */
struct assoc_test_struct
{
    PCSTR     extension;
    PERCEIVED perceived;
    INT       flags;
    PCSTR     type;
    DWORD     minversion;
    HRESULT   hr;
};

#define HARDCODED_NATIVE_WMSDK      (PERCEIVEDFLAG_HARDCODED | PERCEIVEDFLAG_NATIVESUPPORT | PERCEIVEDFLAG_WMSDK)
#define HARDCODED_NATIVE_GDIPLUS    (PERCEIVEDFLAG_HARDCODED | PERCEIVEDFLAG_NATIVESUPPORT | PERCEIVEDFLAG_GDIPLUS)
#define HARDCODED_NATIVE_ZIPFLDR    (PERCEIVEDFLAG_HARDCODED | PERCEIVEDFLAG_NATIVESUPPORT | PERCEIVEDFLAG_ZIPFOLDER)
#define SOFTCODED_NATIVESUPPORT     (PERCEIVEDFLAG_SOFTCODED | PERCEIVEDFLAG_NATIVESUPPORT)

static const struct assoc_test_struct assoc_perceived_types[] =
{
    /* builtins */
    { ".aif",           PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".aifc",          PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".aiff",          PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".asf",           PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".asx",           PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".au",            PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".avi",           PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".bas",           PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,    "application" },
    { ".bat",           PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,    "application" },
    { ".bmp",           PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".cmd",           PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,    "application" },
    { ".com",           PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,    "application" },
    { ".cpl",           PERCEIVED_TYPE_SYSTEM,      PERCEIVEDFLAG_HARDCODED,    "system",       0x600 },
    { ".dib",           PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".dvr-ms",        PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".emf",           PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".exe",           PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,    "application" },
    { ".gif",           PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".hta",           PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,    "application" },
    /* htm & html are PERCEIVED_TYPE_TEXT, PERCEIVEDFLAG_NATIVESUPPORT | PERCEIVEDFLAG_SOFTCODED in w2k3 */
    { ".htm",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_HARDCODED,    "document",     0x600 },
    { ".html",          PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_HARDCODED,    "document",     0x600 },
    { ".ico",           PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".IVF",           PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".jfif",          PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".jpe",           PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".jpeg",          PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".jpg",           PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".lnk",           PERCEIVED_TYPE_UNSPECIFIED, PERCEIVEDFLAG_HARDCODED,    NULL,           0x600, E_FAIL },
    { ".m1v",           PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".m3u",           PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".mht",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_HARDCODED,    "document",     0x600 },
    { ".mid",           PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".midi",          PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".msi",           PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,    "application" },
    /* below win8 this is defined to be video */
    { ".mp2",           PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio",        0x602 },
    { ".mp2v",          PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".mp3",           PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".mpa",           PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".mpe",           PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".mpeg",          PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".mpg",           PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".mpv2",          PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".pif",           PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,    "application" },
    { ".png",           PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".reg",           PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,    "application" },
    { ".rle",           PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".rmi",           PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".scr",           PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,    "application" },
    { ".search-ms",     PERCEIVED_TYPE_UNSPECIFIED, PERCEIVEDFLAG_HARDCODED,    NULL,           0x600, E_FAIL },
    { ".snd",           PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".tif",           PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".tiff",          PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".vb",            PERCEIVED_TYPE_APPLICATION, PERCEIVEDFLAG_HARDCODED,    "application" },
    { ".wav",           PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".wax",           PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".wm",            PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".wma",           PERCEIVED_TYPE_AUDIO,       HARDCODED_NATIVE_WMSDK,     "audio" },
    { ".wmf",           PERCEIVED_TYPE_IMAGE,       HARDCODED_NATIVE_GDIPLUS,   "image" },
    { ".wmv",           PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".wmx",           PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".wvx",           PERCEIVED_TYPE_VIDEO,       HARDCODED_NATIVE_WMSDK,     "video" },
    { ".zip",           PERCEIVED_TYPE_COMPRESSED,  HARDCODED_NATIVE_ZIPFLDR,   "compressed" },
    /* found in the registry under HKEY_CLASSES_ROOT on a new Win7 VM */
    { ".386",           PERCEIVED_TYPE_SYSTEM,      PERCEIVEDFLAG_SOFTCODED,    "system" },
    { ".3g2",           PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".3gp",           PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".3gp2",          PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".3gpp",          PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".AAC",           PERCEIVED_TYPE_AUDIO,       PERCEIVEDFLAG_SOFTCODED,    "audio",        0x601 },
    { ".ADT",           PERCEIVED_TYPE_AUDIO,       PERCEIVEDFLAG_SOFTCODED,    "audio",        0x601 },
    { ".ADTS",          PERCEIVED_TYPE_AUDIO,       PERCEIVEDFLAG_SOFTCODED,    "audio",        0x601 },
    { ".asm",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".asmx",          PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".aspx",          PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".c",             PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".cab",           PERCEIVED_TYPE_COMPRESSED,  PERCEIVEDFLAG_SOFTCODED,    "compressed",   0x600 },
    { ".chk",           PERCEIVED_TYPE_SYSTEM,      PERCEIVEDFLAG_SOFTCODED,    "system" },
    { ".cpp",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".css",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".cxx",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".def",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".diz",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".docx",          PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x601 },
    { ".drv",           PERCEIVED_TYPE_SYSTEM,      PERCEIVEDFLAG_SOFTCODED,    "system",       0x600 },
    { ".gz",            PERCEIVED_TYPE_COMPRESSED,  PERCEIVEDFLAG_SOFTCODED,    "compressed" },
    { ".h",             PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".hpp",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".hxx",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".inc",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".ini",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text",         0x600 },
    { ".java",          PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".local",         PERCEIVED_TYPE_SYSTEM,      PERCEIVEDFLAG_SOFTCODED,    "system" },
    { ".M2T",           PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".M2TS",          PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".M2V",           PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".m4a",           PERCEIVED_TYPE_AUDIO,       PERCEIVEDFLAG_SOFTCODED,    "audio",        0x601 },
    { ".m4b",           PERCEIVED_TYPE_AUDIO,       PERCEIVEDFLAG_SOFTCODED,    "audio",        0x601 },
    { ".m4p",           PERCEIVED_TYPE_AUDIO,       PERCEIVEDFLAG_SOFTCODED,    "audio",        0x601 },
    { ".m4v",           PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".manifest",      PERCEIVED_TYPE_SYSTEM,      PERCEIVEDFLAG_SOFTCODED,    "system" },
    { ".MOD",           PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".mov",           PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".mp4",           PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".mp4v",          PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".MTS",           PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".nvr",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".ocx",           PERCEIVED_TYPE_SYSTEM,      PERCEIVEDFLAG_SOFTCODED,    "system" },
    { ".odt",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x601 },
    { ".php3",          PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".pl",            PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".plg",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".ps1xml",        PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "Text" },
    { ".rtf",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x600 },
    { ".sed",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".shtml",         PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".sql",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".sys",           PERCEIVED_TYPE_SYSTEM,      PERCEIVEDFLAG_SOFTCODED,    "system",       0x600 },
    { ".tar",           PERCEIVED_TYPE_COMPRESSED,  PERCEIVEDFLAG_SOFTCODED,    "compressed" },
    { ".text",          PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".tgz",           PERCEIVED_TYPE_COMPRESSED,  PERCEIVEDFLAG_SOFTCODED,    "compressed" },
    { ".TS",            PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".tsv",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".TTS",           PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".txt",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".vob",           PERCEIVED_TYPE_VIDEO,       PERCEIVEDFLAG_SOFTCODED,    "video",        0x601 },
    { ".vxd",           PERCEIVED_TYPE_SYSTEM,      PERCEIVEDFLAG_SOFTCODED,    "system" },
    { ".wdp",           PERCEIVED_TYPE_IMAGE,       PERCEIVEDFLAG_SOFTCODED,    "image" },
    { ".wmz",           PERCEIVED_TYPE_COMPRESSED,  PERCEIVEDFLAG_SOFTCODED,    "compressed" },
    { ".wpl",           PERCEIVED_TYPE_AUDIO,       PERCEIVEDFLAG_SOFTCODED,    "audio",        0x601 },
    { ".wsz",           PERCEIVED_TYPE_COMPRESSED,  PERCEIVEDFLAG_SOFTCODED,    "compressed" },
    { ".x",             PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text" },
    { ".xml",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text",         0x601 },
    { ".xsl",           PERCEIVED_TYPE_TEXT,        SOFTCODED_NATIVESUPPORT,    "text",         0x601 },
    { ".z",             PERCEIVED_TYPE_COMPRESSED,  PERCEIVEDFLAG_SOFTCODED,    "compressed" },
    /* found in the registry under HKEY_CLASSES_ROOT\PerceivedType */
    { ".doc",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x600 },
    { ".dot",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x600 },
    { ".mhtml",         PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x600 },
    { ".pot",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x600 },
    { ".ppt",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x600 },
    { ".rtf",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x600 },
    { ".wri",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x600 },
    { ".xls",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x600 },
    { ".xlt",           PERCEIVED_TYPE_DOCUMENT,    PERCEIVEDFLAG_SOFTCODED,    "document",     0x600 },

};

static void test_assoc_one(const struct assoc_test_struct* test)
{
    LPWSTR extension, type_expected, type_returned;
    PERCEIVED perceived;
    HRESULT hr;
    INT flags;

    /* if SHStrDupA receives a nullptr as input, it will null the output */
    SHStrDupA(test->extension, &extension);
    SHStrDupA(test->type, &type_expected);

    perceived = 0xdeadbeef;
    flags = 0xdeadbeef;

    hr = pAssocGetPerceivedType(extension, &perceived, &flags, NULL);
    expect_hr(type_expected ? S_OK : test->hr, hr);
    ok(perceived == test->perceived, "%s: got perceived 0x%x, expected 0x%x\n",
       test->extension, perceived, test->perceived);
    ok(flags == test->flags, "%s: got flags 0x%x, expected 0x%x\n",
       test->extension, flags, test->flags);

    type_returned = (void *)0xdeadbeef;
    perceived = 0xdeadbeef;
    flags = 0xdeadbeef;

    hr = pAssocGetPerceivedType(extension, &perceived, &flags, &type_returned);
    expect_hr(type_expected ? S_OK : test->hr, hr);
    ok(perceived == test->perceived, "%s: got perceived 0x%x, expected 0x%x\n",
       test->extension, perceived, test->perceived);
    ok(flags == test->flags, "%s: got flags 0x%x, expected 0x%x\n",
       test->extension, flags, test->flags);

    if (!type_expected)
    {
        ok(type_returned == (void *)0xdeadbeef || broken(type_returned == NULL) /* Win 8 */,
           "%s: got type %p, expected 0xdeadbeef\n", test->extension, type_returned);
    }
    else if (type_returned == (void *)0xdeadbeef)
    {
        ok(type_returned != (void *)0xdeadbeef, "%s: got type %p, expected '%s'\n",
           test->extension, type_returned, test->type);
    }
    else
    {
        ok(StrCmpIW(type_expected, type_returned) == 0, "%s: got type %s, expected '%s'\n",
           test->extension, wine_dbgstr_w(type_returned), test->type);
    }

    CoTaskMemFree(type_returned);
    CoTaskMemFree(extension);
    CoTaskMemFree(type_expected);
}

static void test_assoc_perceived(void)
{
    static const struct assoc_test_struct should_not_exist =
        { ".should_not_exist", PERCEIVED_TYPE_UNSPECIFIED, PERCEIVEDFLAG_UNDEFINED, NULL, 0, 0x80070002 };
    static const struct assoc_test_struct htm[] =
    {
        { ".htm",  PERCEIVED_TYPE_TEXT, SOFTCODED_NATIVESUPPORT, "text", 0x600 },
        { ".html", PERCEIVED_TYPE_TEXT, SOFTCODED_NATIVESUPPORT, "text", 0x600 },
    };
    static const struct assoc_test_struct mp2 =
        { ".mp2", PERCEIVED_TYPE_VIDEO, HARDCODED_NATIVE_WMSDK, "video" };

    OSVERSIONINFOEXW osvi;
    DWORD version;
    size_t i;

    if (!pAssocGetPerceivedType)
    {
        win_skip("AssocGetPerceivedType() is missing\n");
        return;
    }

    memset(&osvi, 0, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExW((LPOSVERSIONINFOW)&osvi);
    version = (osvi.dwMajorVersion << 8) | osvi.dwMinorVersion;

    /* invalid entry results in HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) */
    test_assoc_one(&should_not_exist);

    for (i = 0; i < sizeof(assoc_perceived_types) / sizeof(assoc_perceived_types[0]); ++i)
    {
        if (assoc_perceived_types[i].minversion && assoc_perceived_types[i].minversion > version)
            continue;
        if (!(assoc_perceived_types[i].flags & PERCEIVEDFLAG_HARDCODED))
            todo_wine test_assoc_one(&assoc_perceived_types[i]);
        else
            test_assoc_one(&assoc_perceived_types[i]);
    }

    /* below Vista */
    if (version < 0x600)
    {
        todo_wine
        test_assoc_one(&htm[0]);
        todo_wine
        test_assoc_one(&htm[1]);
    }

    /* below Win8 */
    if (version < 0x602)
    {
        test_assoc_one(&mp2);
    }
}

START_TEST(assoc)
{
    HMODULE hshlwapi;
    hshlwapi = GetModuleHandleA("shlwapi.dll");
    pAssocQueryStringA = (void*)GetProcAddress(hshlwapi, "AssocQueryStringA");
    pAssocQueryStringW = (void*)GetProcAddress(hshlwapi, "AssocQueryStringW");
    pAssocCreate       = (void*)GetProcAddress(hshlwapi, "AssocCreate");
    pAssocGetPerceivedType = (void*)GetProcAddress(hshlwapi, "AssocGetPerceivedType");

    test_getstring_bad();
    test_getstring_basic();
    test_getstring_no_extra();
    test_assoc_create();
    test_assoc_perceived();
}
