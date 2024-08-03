/*
 * Misc tests
 *
 * Copyright 2006 Paul Vriens
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

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"
#include "v6util.h"

static PVOID (WINAPI * pAlloc)(LONG);
static PVOID (WINAPI * pReAlloc)(PVOID, LONG);
static BOOL (WINAPI * pFree)(PVOID);
static LONG (WINAPI * pGetSize)(PVOID);

static INT (WINAPI * pStr_GetPtrA)(LPCSTR, LPSTR, INT);
static BOOL (WINAPI * pStr_SetPtrA)(LPSTR, LPCSTR);
static INT (WINAPI * pStr_GetPtrW)(LPCWSTR, LPWSTR, INT);
static BOOL (WINAPI * pStr_SetPtrW)(LPWSTR, LPCWSTR);

static HMODULE hComctl32 = 0;

static char testicon_data[] =
{
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x00, 0x00, 0x01, 0x00,
    0x20, 0x00, 0x40, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x12, 0x0b,
    0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xde, 0xde, 0xde, 0xff, 0xde, 0xde, 0xde, 0xff, 0xde, 0xde,
    0xde, 0xff, 0xde, 0xde, 0xde, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};

#define COMCTL32_GET_PROC(ordinal, func) \
    p ## func = (void*)GetProcAddress(hComctl32, (LPSTR)ordinal); \
    if(!p ## func) { \
      trace("GetProcAddress(%d)(%s) failed\n", ordinal, #func); \
      FreeLibrary(hComctl32); \
    }

static BOOL InitFunctionPtrs(void)
{
    hComctl32 = LoadLibraryA("comctl32.dll");

    if(!hComctl32)
    {
        trace("Could not load comctl32.dll\n");
        return FALSE;
    }

    COMCTL32_GET_PROC(71, Alloc);
    COMCTL32_GET_PROC(72, ReAlloc);
    COMCTL32_GET_PROC(73, Free);
    COMCTL32_GET_PROC(74, GetSize);

    COMCTL32_GET_PROC(233, Str_GetPtrA)
    COMCTL32_GET_PROC(234, Str_SetPtrA)
    COMCTL32_GET_PROC(235, Str_GetPtrW)
    COMCTL32_GET_PROC(236, Str_SetPtrW)

    return TRUE;
}

static void test_GetPtrAW(void)
{
    if (pStr_GetPtrA)
    {
        static const char source[] = "Just a source string";
        static const char desttest[] = "Just a destination string";
        static char dest[MAX_PATH];
        int sourcelen;
        int destsize = MAX_PATH;
        int count;

        sourcelen = strlen(source) + 1;

        count = pStr_GetPtrA(NULL, NULL, 0);
        ok (count == 0, "Expected count to be 0, it was %d\n", count);

        if (0)
        {
            /* Crashes on W98, NT4, W2K, XP, W2K3
             * Our implementation also crashes and we should probably leave
             * it like that.
             */
            count = pStr_GetPtrA(NULL, NULL, destsize);
            trace("count : %d\n", count);
        }

        count = pStr_GetPtrA(source, NULL, 0);
        ok (count == sourcelen ||
            broken(count == sourcelen - 1), /* win9x */
            "Expected count to be %d, it was %d\n", sourcelen, count);

        strcpy(dest, desttest);
        count = pStr_GetPtrA(source, dest, 0);
        ok (count == sourcelen ||
            broken(count == 0), /* win9x */
            "Expected count to be %d, it was %d\n", sourcelen, count);
        ok (!lstrcmpA(dest, desttest) ||
            broken(!lstrcmpA(dest, "")), /* Win7 */
            "Expected destination to not have changed\n");

        count = pStr_GetPtrA(source, NULL, destsize);
        ok (count == sourcelen ||
            broken(count == sourcelen - 1), /* win9x */
            "Expected count to be %d, it was %d\n", sourcelen, count);

        count = pStr_GetPtrA(source, dest, destsize);
        ok (count == sourcelen ||
            broken(count == sourcelen - 1), /* win9x */
            "Expected count to be %d, it was %d\n", sourcelen, count);
        ok (!lstrcmpA(source, dest), "Expected source and destination to be the same\n");

        strcpy(dest, desttest);
        count = pStr_GetPtrA(NULL, dest, destsize);
        ok (count == 0, "Expected count to be 0, it was %d\n", count);
        ok (dest[0] == '\0', "Expected destination to be cut-off and 0 terminated\n");

        destsize = 15;
        count = pStr_GetPtrA(source, dest, destsize);
        ok (count == 15 ||
            broken(count == 14), /* win9x */
            "Expected count to be 15, it was %d\n", count);
        ok (!memcmp(source, dest, 14), "Expected first part of source and destination to be the same\n");
        ok (dest[14] == '\0', "Expected destination to be cut-off and 0 terminated\n");
    }
}

static void test_Alloc(void)
{
    PCHAR p;
    BOOL res;
    DWORD size, min;

    /* allocate size 0 */
    p = pAlloc(0);
    ok(p != NULL, "Expected non-NULL ptr\n");

    /* get the minimum size */
    min = pGetSize(p);

    /* free the block */
    res = pFree(p);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);

    /* allocate size 1 */
    p = pAlloc(1);
    ok(p != NULL, "Expected non-NULL ptr\n");

    /* get the allocated size */
    size = pGetSize(p);
    ok(size == 1 ||
       broken(size == min), /* win9x */
       "Expected 1, got %d\n", size);

    /* reallocate the block */
    p = pReAlloc(p, 2);
    ok(p != NULL, "Expected non-NULL ptr\n");

    /* get the new size */
    size = pGetSize(p);
    ok(size == 2 ||
       broken(size == min), /* win9x */
       "Expected 2, got %d\n", size);

    /* free the block */
    res = pFree(p);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);

    /* free a NULL ptr */
    res = pFree(NULL);
    ok(res == TRUE ||
       broken(res == FALSE), /* win9x */
       "Expected TRUE, got %d\n", res);

    /* reallocate a NULL ptr */
    p = pReAlloc(NULL, 2);
    ok(p != NULL, "Expected non-NULL ptr\n");

    res = pFree(p);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
}

static void test_LoadIconWithScaleDown(void)
{
    static const WCHAR nonexisting_fileW[] = {'n','o','n','e','x','i','s','t','i','n','g','.','i','c','o',0};
    static const WCHAR nonexisting_resourceW[] = {'N','o','n','e','x','i','s','t','i','n','g',0};
    static const WCHAR prefixW[] = {'I','C','O',0};
    HRESULT (WINAPI *pLoadIconMetric)(HINSTANCE, const WCHAR *, int, HICON *);
    HRESULT (WINAPI *pLoadIconWithScaleDown)(HINSTANCE, const WCHAR *, int, int, HICON *);
    WCHAR tmp_path[MAX_PATH], icon_path[MAX_PATH];
    ICONINFO info;
    HMODULE hinst;
    HANDLE handle;
    DWORD written;
    HRESULT hr;
    BITMAP bmp;
    HICON icon;
    void *ptr;
    int bytes;
    BOOL res;

    hinst = LoadLibraryA("comctl32.dll");
    pLoadIconMetric        = (void *)GetProcAddress(hinst, "LoadIconMetric");
    pLoadIconWithScaleDown = (void *)GetProcAddress(hinst, "LoadIconWithScaleDown");
    if (!pLoadIconMetric || !pLoadIconWithScaleDown)
    {
        win_skip("LoadIconMetric or pLoadIconWithScaleDown not exported by name\n");
        FreeLibrary(hinst);
        return;
    }

    GetTempPathW(MAX_PATH, tmp_path);
    GetTempFileNameW(tmp_path, prefixW, 0, icon_path);
    handle = CreateFileW(icon_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFileW failed with error %u\n", GetLastError());
    res = WriteFile(handle, testicon_data, sizeof(testicon_data), &written, NULL);
    ok(res && written == sizeof(testicon_data), "Failed to write icon file\n");
    CloseHandle(handle);

    /* test ordinals */
    ptr = GetProcAddress(hinst, (const char *)380);
    ok(ptr == pLoadIconMetric,
       "got wrong pointer for ordinal 380, %p expected %p\n", ptr, pLoadIconMetric);

    ptr = GetProcAddress(hinst, (const char *)381);
    ok(ptr == pLoadIconWithScaleDown,
       "got wrong pointer for ordinal 381, %p expected %p\n", ptr, pLoadIconWithScaleDown);

    /* invalid arguments */
    icon = (HICON)0x1234;
    hr = pLoadIconMetric(NULL, (LPWSTR)IDI_APPLICATION, 0x100, &icon);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %x\n", hr);
    ok(icon == NULL, "Expected NULL, got %p\n", icon);

    icon = (HICON)0x1234;
    hr = pLoadIconMetric(NULL, NULL, LIM_LARGE, &icon);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %x\n", hr);
    ok(icon == NULL, "Expected NULL, got %p\n", icon);

    icon = (HICON)0x1234;
    hr = pLoadIconWithScaleDown(NULL, NULL, 32, 32, &icon);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %x\n", hr);
    ok(icon == NULL, "Expected NULL, got %p\n", icon);

    /* non-existing filename */
    hr = pLoadIconMetric(NULL, nonexisting_fileW, LIM_LARGE, &icon);
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* Win7 */,
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %x\n", hr);

    hr = pLoadIconWithScaleDown(NULL, nonexisting_fileW, 32, 32, &icon);
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %x\n", hr);

    /* non-existing resource name */
    hr = pLoadIconMetric(hinst, nonexisting_resourceW, LIM_LARGE, &icon);
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %x\n", hr);

    hr = pLoadIconWithScaleDown(hinst, nonexisting_resourceW, 32, 32, &icon);
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %x\n", hr);

    /* load icon using predefined identifier */
    hr = pLoadIconMetric(NULL, (LPWSTR)IDI_APPLICATION, LIM_SMALL, &icon);
    ok(hr == S_OK, "Expected S_OK, got %x\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %u\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == GetSystemMetrics(SM_CXSMICON), "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == GetSystemMetrics(SM_CYSMICON), "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    hr = pLoadIconMetric(NULL, (LPWSTR)IDI_APPLICATION, LIM_LARGE, &icon);
    ok(hr == S_OK, "Expected S_OK, got %x\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %u\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == GetSystemMetrics(SM_CXICON), "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == GetSystemMetrics(SM_CYICON), "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    hr = pLoadIconWithScaleDown(NULL, (LPWSTR)IDI_APPLICATION, 42, 42, &icon);
    ok(hr == S_OK, "Expected S_OK, got %x\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %u\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == 42, "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == 42, "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    /* load icon from file */
    hr = pLoadIconMetric(NULL, icon_path, LIM_SMALL, &icon);
    ok(hr == S_OK, "Expected S_OK, got %x\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %u\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == GetSystemMetrics(SM_CXSMICON), "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == GetSystemMetrics(SM_CYSMICON), "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    hr = pLoadIconWithScaleDown(NULL, icon_path, 42, 42, &icon);
    ok(hr == S_OK, "Expected S_OK, got %x\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %u\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == 42, "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == 42, "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    DeleteFileW(icon_path);
    FreeLibrary(hinst);
}

static void check_class( const char *name, int must_exist, UINT style, UINT ignore, BOOL v6 )
{
    WNDCLASSA wc;

    if (GetClassInfoA( 0, name, &wc ))
    {
        char buff[64];
        HWND hwnd;

todo_wine_if(!strcmp(name, "SysLink") && !must_exist && !v6)
        ok( must_exist, "System class %s should %sexist\n", name, must_exist ? "" : "NOT " );
        if (!must_exist) return;

todo_wine_if(!strcmp(name, "ScrollBar") || (!strcmp(name, "tooltips_class32") && v6))
        ok( !(~wc.style & style & ~ignore), "System class %s is missing bits %x (%08x/%08x)\n",
            name, ~wc.style & style, wc.style, style );
todo_wine_if((!strcmp(name, "tooltips_class32") && v6) || !strcmp(name, "SysLink"))
        ok( !(wc.style & ~style), "System class %s has extra bits %x (%08x/%08x)\n",
            name, wc.style & ~style, wc.style, style );
        ok( !wc.hInstance, "System class %s has hInstance %p\n", name, wc.hInstance );

        hwnd = CreateWindowA(name, 0, 0, 0, 0, 0, 0, 0, NULL, GetModuleHandleA(NULL), 0);
        ok( hwnd != NULL, "Failed to create window for class %s.\n", name );
        GetClassNameA(hwnd, buff, ARRAY_SIZE(buff));
        ok( !strcmp(name, buff), "Unexpected class name %s, expected %s.\n", buff, name );
        DestroyWindow(hwnd);
    }
    else
        ok( !must_exist, "System class %s does not exist\n", name );
}

/* test styles of system classes */
static void test_builtin_classes(void)
{
    /* check style bits */
    check_class( "Button",     1, CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE );
    check_class( "ComboBox",   1, CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE );
    check_class( "Edit",       1, CS_PARENTDC | CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE );
    check_class( "ListBox",    1, CS_PARENTDC | CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE );
    check_class( "ScrollBar",  1, CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE );
    check_class( "Static",     1, CS_PARENTDC | CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE );
    check_class( "ComboLBox",  1, CS_SAVEBITS | CS_DBLCLKS | CS_DROPSHADOW | CS_GLOBALCLASS, CS_DROPSHADOW, FALSE );
}

static void test_comctl32_classes(BOOL v6)
{
    check_class(ANIMATE_CLASSA,      1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_COMBOBOXEXA,      1, CS_GLOBALCLASS, 0, FALSE);
    check_class(DATETIMEPICK_CLASSA, 1, CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_HEADERA,          1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    check_class(HOTKEY_CLASSA,       1, CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_IPADDRESSA,       1, CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_LISTVIEWA,        1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    check_class(MONTHCAL_CLASSA,     1, CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_NATIVEFONTCTLA,   1, CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_PAGESCROLLERA,    1, CS_GLOBALCLASS, 0, FALSE);
    check_class(PROGRESS_CLASSA,     1, CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE);
    check_class(REBARCLASSNAMEA,     1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    check_class(STATUSCLASSNAMEA,    1, CS_DBLCLKS | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_TABCONTROLA,      1, CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE);
    check_class(TOOLBARCLASSNAMEA,   1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    if (v6)
        check_class(TOOLTIPS_CLASSA, 1, CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS | CS_DROPSHADOW, CS_SAVEBITS | CS_HREDRAW | CS_VREDRAW /* XP */, TRUE);
    else
        check_class(TOOLTIPS_CLASSA, 1, CS_DBLCLKS | CS_GLOBALCLASS | CS_SAVEBITS, CS_HREDRAW | CS_VREDRAW /* XP */, FALSE);
    check_class(TRACKBAR_CLASSA,     1, CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_TREEVIEWA,        1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    check_class(UPDOWN_CLASSA,       1, CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE);
    check_class("SysLink", v6, CS_GLOBALCLASS, 0, FALSE);
}

START_TEST(misc)
{
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    if(!InitFunctionPtrs())
        return;

    test_GetPtrAW();
    test_Alloc();

    test_comctl32_classes(FALSE);

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    test_comctl32_classes(TRUE);
    test_builtin_classes();
    test_LoadIconWithScaleDown();

    unload_v6_module(ctx_cookie, hCtx);
}
