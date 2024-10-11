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
#include "msg.h"

static PVOID (WINAPI * pAlloc)(LONG);
static PVOID (WINAPI * pReAlloc)(PVOID, LONG);
static BOOL (WINAPI * pFree)(PVOID);
static LONG (WINAPI * pGetSize)(PVOID);

static INT (WINAPI * pStr_GetPtrA)(LPCSTR, LPSTR, INT);
static BOOL (WINAPI * pStr_SetPtrA)(LPSTR, LPCSTR);
static INT (WINAPI * pStr_GetPtrW)(LPCWSTR, LPWSTR, INT);
static BOOL (WINAPI * pStr_SetPtrW)(LPWSTR, LPCWSTR);

static BOOL (WINAPI *pSetWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);
static BOOL (WINAPI *pRemoveWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR);
static LRESULT (WINAPI *pDefSubclassProc)(HWND, UINT, WPARAM, LPARAM);

static HMODULE hComctl32;

/* For message tests */
enum seq_index
{
    CHILD_SEQ_INDEX,
    PARENT_SEQ_INDEX,
    NUM_MSG_SEQUENCES
};

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

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

static BOOL init_functions_v6(void)
{
    hComctl32 = LoadLibraryA("comctl32.dll");
    if (!hComctl32)
    {
        trace("Could not load comctl32.dll version 6\n");
        return FALSE;
    }

    COMCTL32_GET_PROC(410, SetWindowSubclass)
    COMCTL32_GET_PROC(412, RemoveWindowSubclass)
    COMCTL32_GET_PROC(413, DefSubclassProc)

    return TRUE;
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, min_timeout, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
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
       "Expected 1, got %ld\n", size);

    /* reallocate the block */
    p = pReAlloc(p, 2);
    ok(p != NULL, "Expected non-NULL ptr\n");

    /* get the new size */
    size = pGetSize(p);
    ok(size == 2 ||
       broken(size == min), /* win9x */
       "Expected 2, got %ld\n", size);

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
    GetTempFileNameW(tmp_path, L"ICO", 0, icon_path);
    handle = CreateFileW(icon_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFileW failed with error %lu\n", GetLastError());
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
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %lx\n", hr);
    ok(icon == NULL, "Expected NULL, got %p\n", icon);

    icon = (HICON)0x1234;
    hr = pLoadIconMetric(NULL, NULL, LIM_LARGE, &icon);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %lx\n", hr);
    ok(icon == NULL, "Expected NULL, got %p\n", icon);

    icon = (HICON)0x1234;
    hr = pLoadIconWithScaleDown(NULL, NULL, 32, 32, &icon);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %lx\n", hr);
    ok(icon == NULL, "Expected NULL, got %p\n", icon);

    /* non-existing filename */
    hr = pLoadIconMetric(NULL, L"nonexisting.ico", LIM_LARGE, &icon);
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* Win7 */,
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %lx\n", hr);

    hr = pLoadIconWithScaleDown(NULL, L"nonexisting.ico", 32, 32, &icon);
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %lx\n", hr);

    /* non-existing resource name */
    hr = pLoadIconMetric(hinst, L"Nonexisting", LIM_LARGE, &icon);
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %lx\n", hr);

    hr = pLoadIconWithScaleDown(hinst, L"Noneexisting", 32, 32, &icon);
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %lx\n", hr);

    /* load icon using predefined identifier */
    hr = pLoadIconMetric(NULL, (LPWSTR)IDI_APPLICATION, LIM_SMALL, &icon);
    ok(hr == S_OK, "Expected S_OK, got %lx\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %lu\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == GetSystemMetrics(SM_CXSMICON), "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == GetSystemMetrics(SM_CYSMICON), "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    hr = pLoadIconMetric(NULL, (LPWSTR)IDI_APPLICATION, LIM_LARGE, &icon);
    ok(hr == S_OK, "Expected S_OK, got %lx\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %lu\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == GetSystemMetrics(SM_CXICON), "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == GetSystemMetrics(SM_CYICON), "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    hr = pLoadIconWithScaleDown(NULL, (LPWSTR)IDI_APPLICATION, 42, 42, &icon);
    ok(hr == S_OK, "Expected S_OK, got %lx\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %lu\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == 42, "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == 42, "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    /* load icon from file */
    hr = pLoadIconMetric(NULL, icon_path, LIM_SMALL, &icon);
    ok(hr == S_OK, "Expected S_OK, got %lx\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %lu\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == GetSystemMetrics(SM_CXSMICON), "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == GetSystemMetrics(SM_CYSMICON), "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    hr = pLoadIconWithScaleDown(NULL, icon_path, 42, 42, &icon);
    ok(hr == S_OK, "Expected S_OK, got %lx\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %lu\n", GetLastError());
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

struct wm_themechanged_test
{
    const char *class;
    const struct message *expected_msg;
    int ignored_msg_count;
    DWORD ignored_msgs[16];
    BOOL todo;
};

static BOOL ignore_message(UINT msg)
{
    /* these are always ignored */
    return (msg >= 0xc000 ||
            msg == WM_GETICON ||
            msg == WM_GETOBJECT ||
            msg == WM_TIMECHANGE ||
            msg == WM_DISPLAYCHANGE ||
            msg == WM_DEVICECHANGE ||
            msg == WM_DWMNCRENDERINGCHANGED ||
            msg == WM_WININICHANGE ||
            msg == WM_CHILDACTIVATE);
}

static LRESULT CALLBACK test_wm_themechanged_proc(HWND hwnd, UINT message, WPARAM wParam,
                                                  LPARAM lParam, UINT_PTR id, DWORD_PTR ref_data)
{
    const struct wm_themechanged_test *test = (const struct wm_themechanged_test *)ref_data;
    static int defwndproc_counter = 0;
    struct message msg = {0};
    LRESULT ret;
    int i;

    if (ignore_message(message))
        return pDefSubclassProc(hwnd, message, wParam, lParam);

    /* Extra messages to be ignored for a test case */
    for (i = 0; i < test->ignored_msg_count; ++i)
    {
        if (message == test->ignored_msgs[i])
            return pDefSubclassProc(hwnd, message, wParam, lParam);
    }

    msg.message = message;
    msg.flags = sent | wparam | lparam;
    if (defwndproc_counter)
        msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, CHILD_SEQ_INDEX, &msg);

    if (message == WM_NCDESTROY)
        pRemoveWindowSubclass(hwnd, test_wm_themechanged_proc, 0);

    ++defwndproc_counter;
    ret = pDefSubclassProc(hwnd, message, wParam, lParam);
    --defwndproc_counter;

    return ret;
}

static HWND create_control(const char *class, DWORD style, HWND parent, DWORD_PTR data)
{
    HWND hwnd;

    if (parent)
        style |= WS_CHILD;
    hwnd = CreateWindowExA(0, class, "test", style, 0, 0, 50, 20, parent, 0, 0, NULL);
    ok(!!hwnd, "Failed to create %s style %#lx parent %p\n", class, style, parent);
    pSetWindowSubclass(hwnd, test_wm_themechanged_proc, 0, data);
    return hwnd;
}

static const struct message wm_themechanged_paint_erase_seq[] =
{
    {WM_THEMECHANGED, sent | wparam | lparam},
    {WM_PAINT, sent | wparam | lparam},
    /* TestBot w7u_2qxl VM occasionally doesn't send WM_ERASEBKGND, hence the 'optional' */
    {WM_ERASEBKGND, sent | defwinproc | optional},
    {0},
};

static const struct message wm_themechanged_paint_seq[] =
{
    {WM_THEMECHANGED, sent | wparam | lparam},
    {WM_PAINT, sent | wparam | lparam},
    {0},
};

static const struct message wm_themechanged_no_paint_seq[] =
{
    {WM_THEMECHANGED, sent | wparam | lparam},
    {0},
};

static void test_WM_THEMECHANGED(void)
{
    HWND parent, child;
    char buffer[64];
    int i;

    static const struct wm_themechanged_test tests[] =
    {
        {ANIMATE_CLASSA, wm_themechanged_no_paint_seq},
        {WC_BUTTONA, wm_themechanged_paint_erase_seq, 2, {WM_GETTEXT, WM_GETTEXTLENGTH}},
        {WC_COMBOBOXA, wm_themechanged_paint_erase_seq, 1, {WM_CTLCOLOREDIT}},
        {WC_COMBOBOXEXA, wm_themechanged_no_paint_seq},
        {DATETIMEPICK_CLASSA, wm_themechanged_paint_erase_seq},
        {WC_EDITA, wm_themechanged_paint_erase_seq, 7, {WM_GETTEXTLENGTH, WM_GETFONT, EM_GETSEL, EM_GETRECT, EM_CHARFROMPOS, EM_LINEFROMCHAR, EM_POSFROMCHAR}},
        {WC_HEADERA, wm_themechanged_paint_erase_seq},
        {HOTKEY_CLASSA, wm_themechanged_no_paint_seq},
        {WC_IPADDRESSA, wm_themechanged_paint_erase_seq, 1, {WM_CTLCOLOREDIT}},
        {WC_LISTBOXA, wm_themechanged_paint_erase_seq},
        {WC_LISTVIEWA, wm_themechanged_paint_erase_seq},
        {MONTHCAL_CLASSA, wm_themechanged_paint_erase_seq},
        {WC_NATIVEFONTCTLA, wm_themechanged_no_paint_seq},
        {WC_PAGESCROLLERA, wm_themechanged_paint_erase_seq},
        {PROGRESS_CLASSA, wm_themechanged_paint_erase_seq, 3, {WM_STYLECHANGING, WM_STYLECHANGED, WM_NCPAINT}},
        {REBARCLASSNAMEA, wm_themechanged_no_paint_seq, 1, {WM_WINDOWPOSCHANGING}},
        {WC_STATICA, wm_themechanged_paint_erase_seq, 2, {WM_GETTEXT, WM_GETTEXTLENGTH}},
        {STATUSCLASSNAMEA, wm_themechanged_paint_erase_seq},
        {"SysLink", wm_themechanged_no_paint_seq},
        {WC_TABCONTROLA, wm_themechanged_paint_erase_seq},
        {TOOLBARCLASSNAMEA, wm_themechanged_paint_erase_seq, 1, {WM_WINDOWPOSCHANGING}},
        {TOOLTIPS_CLASSA, wm_themechanged_no_paint_seq},
        {TRACKBAR_CLASSA, wm_themechanged_paint_seq},
        {WC_TREEVIEWA, wm_themechanged_paint_erase_seq, 1, {0x1128}},
        {UPDOWN_CLASSA, wm_themechanged_paint_erase_seq, 1, {WM_NCPAINT}},
        {WC_SCROLLBARA, wm_themechanged_paint_erase_seq, 1, {SBM_GETSCROLLINFO}},
    };

    parent = CreateWindowExA(0, WC_STATICA, "parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100,
                             200, 200, 0, 0, 0, NULL);
    ok(!!parent, "Failed to create parent window\n");

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        child = create_control(tests[i].class, WS_VISIBLE, parent, (DWORD_PTR)&tests[i]);
        flush_events();
        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        SendMessageW(child, WM_THEMECHANGED, 0, 0);
        flush_events();

        sprintf(buffer, "Test %d class %s WM_THEMECHANGED", i, tests[i].class);
        ok_sequence(sequences, CHILD_SEQ_INDEX, tests[i].expected_msg, buffer, tests[i].todo);
        DestroyWindow(child);
    }

    DestroyWindow(parent);
}

static const struct message wm_syscolorchange_seq[] =
{
    {WM_SYSCOLORCHANGE, sent | wparam | lparam},
    {0},
};

static INT_PTR CALLBACK wm_syscolorchange_dlg_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct message msg = {0};

    msg.message = message;
    msg.flags = sent | wparam | lparam;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, CHILD_SEQ_INDEX, &msg);
    return FALSE;
}

static void test_WM_SYSCOLORCHANGE(void)
{
    HWND parent, dialog;
    struct
    {
        DLGTEMPLATE tmplate;
        WORD menu;
        WORD class;
        WORD title;
    } temp = {{0}};

    parent = CreateWindowExA(0, WC_STATICA, "parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100,
                             200, 200, 0, 0, 0, NULL);
    ok(!!parent, "CreateWindowExA failed, error %ld\n", GetLastError());

    temp.tmplate.style = WS_CHILD | WS_VISIBLE;
    temp.tmplate.cx = 50;
    temp.tmplate.cy = 50;
    dialog = CreateDialogIndirectParamA(NULL, &temp.tmplate, parent, wm_syscolorchange_dlg_proc, 0);
    ok(!!dialog, "CreateDialogIndirectParamA failed, error %ld\n", GetLastError());
    flush_events();
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    SendMessageW(dialog, WM_SYSCOLORCHANGE, 0, 0);
    ok_sequence(sequences, CHILD_SEQ_INDEX, wm_syscolorchange_seq, "test dialog WM_SYSCOLORCHANGE", FALSE);

    EndDialog(dialog, 0);
    DestroyWindow(parent);
}

static const struct message empty_seq[] =
{
    {0}
};

static const struct message wm_erasebkgnd_seq[] =
{
    {WM_ERASEBKGND, sent},
    {0}
};

static const struct message wm_ctlcolorstatic_seq[] =
{
    {WM_CTLCOLORSTATIC, sent},
    {0}
};

static const struct message drawthemeparentbackground_seq[] =
{
    {WM_ERASEBKGND, sent},
    {WM_PRINTCLIENT, sent},
    {0}
};

static const struct message drawthemeparentbackground_optional_seq[] =
{
    {WM_ERASEBKGND, sent | optional},
    {WM_PRINTCLIENT, sent | optional},
    {0}
};

static const struct message pushbutton_seq[] =
{
    {WM_ERASEBKGND, sent},
    {WM_PRINTCLIENT, sent},
    {WM_CTLCOLORBTN, sent},
    {0}
};

static const struct message defpushbutton_seq[] =
{
    {WM_ERASEBKGND, sent},
    {WM_PRINTCLIENT, sent},
    {WM_CTLCOLORBTN, sent},
    {WM_ERASEBKGND, sent | optional},
    {WM_PRINTCLIENT, sent | optional},
    {WM_CTLCOLORBTN, sent | optional},
    {0}
};

static const struct message checkbox_seq[] =
{
    {WM_ERASEBKGND, sent | optional},
    {WM_PRINTCLIENT, sent | optional},
    {WM_CTLCOLORSTATIC, sent},
    {0}
};

static const struct message radiobutton_seq[] =
{
    {WM_ERASEBKGND, sent},
    {WM_PRINTCLIENT, sent},
    {WM_CTLCOLORSTATIC, sent},
    {0}
};

static const struct message groupbox_seq[] =
{
    {WM_CTLCOLORSTATIC, sent},
    {WM_ERASEBKGND, sent},
    {WM_PRINTCLIENT, sent},
    {0}
};

static const struct message ownerdrawbutton_seq[] =
{
    {WM_CTLCOLORBTN, sent},
    {WM_CTLCOLORBTN, sent},
    {0}
};

static const struct message splitbutton_seq[] =
{
    {WM_ERASEBKGND, sent},
    {WM_PRINTCLIENT, sent},
    /* Either WM_CTLCOLORSTATIC or WM_CTLCOLORBTN */
    {WM_CTLCOLORSTATIC, sent | optional},
    {WM_CTLCOLORBTN, sent | optional},
    /* BS_DEFSPLITBUTTON or BS_DEFCOMMANDLINK */
    {WM_ERASEBKGND, sent | optional},
    {WM_PRINTCLIENT, sent | optional},
    {WM_CTLCOLORSTATIC, sent | optional},
    {WM_CTLCOLORBTN, sent | optional},
    {0}
};

static const struct message combobox_seq[] =
{
    {WM_ERASEBKGND, sent | optional},
    {WM_PRINTCLIENT, sent | optional},
    {WM_CTLCOLOREDIT, sent},
    {WM_CTLCOLORLISTBOX, sent},
    {WM_CTLCOLORLISTBOX, sent | optional},
    {WM_CTLCOLOREDIT, sent | optional},
    {WM_CTLCOLOREDIT, sent | optional},
    {0}
};

static const struct message edit_seq[] =
{
    {WM_CTLCOLOREDIT, sent},
    {WM_CTLCOLOREDIT, sent | optional},
    {0}
};

static const struct message listbox_seq[] =
{
    {WM_CTLCOLORLISTBOX, sent},
    {WM_CTLCOLORLISTBOX, sent},
    {0}
};

static const struct message treeview_seq[] =
{
    {WM_CTLCOLOREDIT, sent | optional},
    {WM_CTLCOLOREDIT, sent | optional},
    {0}
};

static const struct message scrollbar_seq[] =
{
    {WM_CTLCOLORSCROLLBAR, sent},
    {WM_CTLCOLORSCROLLBAR, sent | optional},
    {0}
};

static LRESULT WINAPI test_themed_background_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    struct message msg = {0};
    HBRUSH brush;
    RECT rect;

    if (message == WM_ERASEBKGND || message == WM_PRINTCLIENT || (message >= WM_CTLCOLORMSGBOX
        && message <= WM_CTLCOLORSTATIC))
    {
        msg.message = message;
        msg.flags = sent;
        add_message(sequences, PARENT_SEQ_INDEX, &msg);
    }

    if (message == WM_ERASEBKGND)
    {
        brush = CreateSolidBrush(RGB(255, 0, 0));
        GetClientRect(hwnd, &rect);
        FillRect((HDC)wp, &rect, brush);
        DeleteObject(brush);
        return 1;
    }
    else if (message >= WM_CTLCOLORMSGBOX && message <= WM_CTLCOLORSTATIC)
    {
        return (LRESULT)GetStockObject(GRAY_BRUSH);
    }

    return DefWindowProcA(hwnd, message, wp, lp);
}

static void test_themed_background(void)
{
    DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
    DPI_AWARENESS_CONTEXT old_context = NULL;
    BOOL (WINAPI *pIsThemeActive)(void);
    HWND child, parent;
    HMODULE uxtheme;
    COLORREF color;
    WNDCLASSA cls;
    HDC hdc;
    int i;

    static const struct test
    {
        const CHAR *class_name;
        DWORD style;
        const struct message *seq;
        BOOL todo;
    }
    tests[] =
    {
        {ANIMATE_CLASSA, 0, empty_seq, TRUE},
        {WC_BUTTONA, BS_PUSHBUTTON, pushbutton_seq},
        {WC_BUTTONA, BS_DEFPUSHBUTTON, defpushbutton_seq},
        {WC_BUTTONA, BS_CHECKBOX, checkbox_seq},
        {WC_BUTTONA, BS_AUTOCHECKBOX, checkbox_seq},
        {WC_BUTTONA, BS_RADIOBUTTON, radiobutton_seq},
        {WC_BUTTONA, BS_3STATE, checkbox_seq},
        {WC_BUTTONA, BS_AUTO3STATE, checkbox_seq},
        {WC_BUTTONA, BS_GROUPBOX, groupbox_seq},
        {WC_BUTTONA, BS_USERBUTTON, pushbutton_seq},
        {WC_BUTTONA, BS_AUTORADIOBUTTON, radiobutton_seq},
        {WC_BUTTONA, BS_PUSHBOX, radiobutton_seq, TRUE},
        {WC_BUTTONA, BS_OWNERDRAW, ownerdrawbutton_seq},
        {WC_BUTTONA, BS_SPLITBUTTON, splitbutton_seq},
        {WC_BUTTONA, BS_DEFSPLITBUTTON, splitbutton_seq},
        {WC_BUTTONA, BS_COMMANDLINK, splitbutton_seq},
        {WC_BUTTONA, BS_DEFCOMMANDLINK, splitbutton_seq},
        {WC_COMBOBOXA, 0, combobox_seq, TRUE},
        {WC_COMBOBOXEXA, 0, drawthemeparentbackground_optional_seq},
        {DATETIMEPICK_CLASSA, 0, drawthemeparentbackground_optional_seq, TRUE},
        {WC_EDITA, 0, edit_seq},
        {WC_HEADERA, 0, empty_seq},
        {HOTKEY_CLASSA, 0, empty_seq, TRUE},
        {WC_IPADDRESSA, 0, empty_seq},
        {WC_LISTBOXA, 0, listbox_seq, TRUE},
        {WC_LISTVIEWA, 0, empty_seq},
        {MONTHCAL_CLASSA, 0, empty_seq},
        {WC_NATIVEFONTCTLA, 0, empty_seq},
        {WC_PAGESCROLLERA, 0, wm_erasebkgnd_seq},
        {PROGRESS_CLASSA, 0, drawthemeparentbackground_optional_seq},
        {REBARCLASSNAMEA, 0, empty_seq},
        {WC_STATICA, SS_LEFT, wm_ctlcolorstatic_seq},
        {WC_STATICA, SS_ICON, wm_ctlcolorstatic_seq},
        {WC_STATICA, SS_BLACKRECT, wm_ctlcolorstatic_seq},
        {WC_STATICA, SS_OWNERDRAW, wm_ctlcolorstatic_seq},
        {WC_STATICA, SS_BITMAP, wm_ctlcolorstatic_seq},
        {WC_STATICA, SS_ENHMETAFILE, wm_ctlcolorstatic_seq},
        {WC_STATICA, SS_ETCHEDHORZ, wm_ctlcolorstatic_seq},
        {STATUSCLASSNAMEA, 0, empty_seq},
        {"SysLink", 0, wm_ctlcolorstatic_seq},
        {WC_TABCONTROLA, 0, drawthemeparentbackground_seq},
        {TOOLBARCLASSNAMEA, 0, empty_seq, TRUE},
        {TOOLTIPS_CLASSA, 0, empty_seq},
        {TRACKBAR_CLASSA, 0, wm_ctlcolorstatic_seq},
        {WC_TREEVIEWA, 0, treeview_seq},
        {UPDOWN_CLASSA, 0, empty_seq},
        {WC_SCROLLBARA, 0, scrollbar_seq},
        {WC_SCROLLBARA, SBS_SIZEBOX, empty_seq},
        {WC_SCROLLBARA, SBS_SIZEGRIP, empty_seq},
        /* Scrollbars in non-client area */
        {"ChildClass", WS_HSCROLL, empty_seq},
        {"ChildClass", WS_VSCROLL, empty_seq},
    };

    uxtheme = LoadLibraryA("uxtheme.dll");
    pIsThemeActive = (void *)GetProcAddress(uxtheme, "IsThemeActive");
    if (!pIsThemeActive())
    {
        skip("Theming is inactive.\n");
        FreeLibrary(uxtheme);
        return;
    }
    FreeLibrary(uxtheme);

    pSetThreadDpiAwarenessContext = (void *)GetProcAddress(GetModuleHandleA("user32.dll"),
                                                           "SetThreadDpiAwarenessContext");
    if (pSetThreadDpiAwarenessContext)
        old_context = pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    memset(&cls, 0, sizeof(cls));
    cls.hInstance = GetModuleHandleA(0);
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpfnWndProc = test_themed_background_proc;
    cls.lpszClassName = "ParentClass";
    RegisterClassA(&cls);

    cls.lpfnWndProc = DefWindowProcA;
    cls.lpszClassName = "ChildClass";
    RegisterClassA(&cls);

    parent = CreateWindowA("ParentClass", "parent", WS_POPUP | WS_VISIBLE, 100, 100, 100, 100,
                           0, 0, 0, 0);
    ok(parent != NULL, "CreateWindowA failed, error %lu.\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("%s %#lx", tests[i].class_name, tests[i].style);

        child = CreateWindowA(tests[i].class_name, "    ", WS_CHILD | WS_VISIBLE | tests[i].style,
                              0, 0, 50, 100, parent, 0, 0, 0);
        ok(child != NULL, "CreateWindowA failed, error %lu.\n", GetLastError());
        flush_events();
        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        RedrawWindow(child, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ERASENOW | RDW_FRAME);
        ok_sequence(sequences, PARENT_SEQ_INDEX, tests[i].seq, "paint background", tests[i].todo);

        /* For message sequences that contain both DrawThemeParentBackground() messages and
         * WM_CTLCOLOR*, do a color test to check which is really in effect for controls that can be
         * tested automatically. For WM_ERASEBKGND from DrawThemeParentBackground(), a red brush is
         * used. For WM_CTLCOLOR*, a gray brush is returned. If there are only WM_CTLCOLOR* messages
         * in the message sequence, then surely DrawThemeParentBackground() is not used.
         *
         * For tests that use pushbutton_seq and defpushbutton_seq, manual tests on XP show that
         * a brush from WM_CTLCOLORBTN is used to fill background even after a successful
         * DrawThemeParentBackground(). This behavior can be verified by returning a gray or hollow
         * brush in test_themed_background_proc() when handling WM_CTLCOLORBTN. It can't be tested
         * automatically here because stock Windows themes don't use transparent button bitmaps */
        if (tests[i].seq == radiobutton_seq || tests[i].seq == groupbox_seq)
        {
            hdc = GetDC(child);

            if (tests[i].seq == radiobutton_seq)
            {
                /* WM_CTLCOLORSTATIC is used to fill background */
                color = GetPixel(hdc, 40, 40);
                /* BS_PUSHBOX is unimplemented on Wine */
                todo_wine_if(i == 11)
                ok(color == 0x808080, "Expected color %#x, got %#lx.\n", 0x808080, color);
            }
            else if (tests[i].seq == groupbox_seq)
            {
                /* DrawThemeParentBackground() is used to fill content background */
                color = GetPixel(hdc, 40, 40);
                ok(color == 0xff, "Expected color %#x, got %#lx.\n", 0xff, color);

                /* WM_CTLCOLORSTATIC is used to fill text background */
                color = GetPixel(hdc, 10, 10);
                ok(color == 0x808080, "Expected color %#x, got %#lx.\n", 0x808080, color);
            }
            else if (tests[i].seq == scrollbar_seq)
            {
                /* WM_CTLCOLORSCROLLBAR is used to fill tracks only */
                color = GetPixel(hdc, 10, 10);
                ok(color != RGB(255, 0, 0), "Got unexpected color %#08lx.\n", color);

                color = GetPixel(hdc, 10, 60);
                ok(color == RGB(255, 0, 0) || broken(color == CLR_INVALID), /* Win7 on TestBots */
                   "Got unexpected color %#08lx.\n", color);
            }

            ReleaseDC(child, hdc);
        }

        DestroyWindow(child);
        winetest_pop_context();
    }

    DestroyWindow(parent);
    UnregisterClassA("ChildClass", GetModuleHandleA(0));
    UnregisterClassA("ParentClass", GetModuleHandleA(0));
    if (pSetThreadDpiAwarenessContext)
        pSetThreadDpiAwarenessContext(old_context);
}

static WNDPROC old_proc;

static const struct message wm_stylechanged_seq[] =
{
    {WM_STYLECHANGED, sent},
    {0}
};

static const struct message wm_stylechanged_repaint_seq[] =
{
    {WM_STYLECHANGED, sent},
    {WM_PAINT, sent},
    {WM_ERASEBKGND, sent | defwinproc},
    {0}
};

static const struct message wm_stylechanged_combox_seq[] =
{
    {WM_STYLECHANGED, sent},
    {WM_ERASEBKGND, sent | defwinproc},
    {WM_PAINT, sent},
    {0}
};

static const struct message wm_stylechanged_listview_report_seq[] =
{
    {WM_STYLECHANGED, sent},
    {WM_ERASEBKGND, sent | defwinproc},
    {WM_PAINT, sent},
    {WM_ERASEBKGND, sent | defwinproc},
    {0}
};

static const struct message wm_stylechanged_pager_seq[] =
{
    {WM_STYLECHANGED, sent},
    {WM_PAINT, sent},
    {WM_NCPAINT, sent | defwinproc},
    {WM_ERASEBKGND, sent | defwinproc},
    {0}
};

static const struct message wm_stylechanged_progress_seq[] =
{
    {WM_STYLECHANGED, sent},
    {WM_PAINT, sent | optional}, /* WM_PAINT and WM_ERASEBKGND are missing with comctl32 v5 */
    {WM_ERASEBKGND, sent | defwinproc | optional},
    {0}
};

static const struct message wm_stylechanged_trackbar_seq[] =
{
    {WM_STYLECHANGED, sent},
    {WM_PAINT, sent | defwinproc},
    {WM_PAINT, sent},
    {0}
};

static LRESULT WINAPI test_wm_stylechanged_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    static int defwndproc_counter = 0;
    struct message msg = {0};
    LRESULT ret;

    if (message == WM_STYLECHANGED
        || message == WM_PAINT
        || message == WM_ERASEBKGND
        || message == WM_NCPAINT)
    {
        msg.message = message;
        msg.flags = sent | wparam | lparam;
        if (defwndproc_counter)
            msg.flags |= defwinproc;
        msg.wParam = wp;
        msg.lParam = lp;
        add_message(sequences, CHILD_SEQ_INDEX, &msg);
    }

    ++defwndproc_counter;
    ret = CallWindowProcA(old_proc, hwnd, message, wp, lp);
    --defwndproc_counter;
    return ret;
}

static void test_WM_STYLECHANGED(void)
{
    HWND parent, hwnd;
    STYLESTRUCT style;
    unsigned int i;

    static const struct test
    {
        const CHAR *class_name;
        DWORD add_style;
        const struct message *seq;
        BOOL todo;
    }
    tests[] =
    {
        {ANIMATE_CLASSA, WS_TABSTOP, wm_stylechanged_seq},
        {WC_BUTTONA, WS_TABSTOP, wm_stylechanged_seq},
        {WC_COMBOBOXA, WS_TABSTOP, wm_stylechanged_combox_seq, TRUE},
        {WC_COMBOBOXEXA, WS_TABSTOP, wm_stylechanged_seq},
        {DATETIMEPICK_CLASSA, WS_TABSTOP, wm_stylechanged_seq},
        {WC_EDITA, WS_TABSTOP, wm_stylechanged_seq},
        {WC_HEADERA, WS_TABSTOP, wm_stylechanged_repaint_seq, TRUE},
        {HOTKEY_CLASSA, WS_TABSTOP, wm_stylechanged_seq},
        {WC_IPADDRESSA, WS_TABSTOP, wm_stylechanged_seq},
        {WC_LISTBOXA, WS_TABSTOP, wm_stylechanged_repaint_seq, TRUE},
        {WC_LISTVIEWA, WS_TABSTOP, wm_stylechanged_seq},
        {WC_LISTVIEWA, LVS_REPORT, wm_stylechanged_listview_report_seq, TRUE},
        {MONTHCAL_CLASSA, WS_TABSTOP, wm_stylechanged_repaint_seq, TRUE},
        {WC_NATIVEFONTCTLA, WS_TABSTOP, wm_stylechanged_seq},
        {WC_PAGESCROLLERA, WS_TABSTOP, wm_stylechanged_pager_seq, TRUE},
        {PROGRESS_CLASSA, WS_TABSTOP, wm_stylechanged_progress_seq},
        {REBARCLASSNAMEA, WS_TABSTOP, wm_stylechanged_seq},
        {WC_STATICA, WS_TABSTOP, wm_stylechanged_seq},
        {STATUSCLASSNAMEA, WS_TABSTOP, wm_stylechanged_seq},
        {"SysLink", WS_TABSTOP, wm_stylechanged_seq},
        {WC_TABCONTROLA, WS_TABSTOP, wm_stylechanged_seq},
        {TOOLBARCLASSNAMEA, WS_TABSTOP, wm_stylechanged_seq},
        {TOOLTIPS_CLASSA, WS_TABSTOP, wm_stylechanged_seq},
        {TRACKBAR_CLASSA, WS_TABSTOP, wm_stylechanged_trackbar_seq, TRUE},
        {WC_TREEVIEWA, WS_TABSTOP, wm_stylechanged_seq},
        {UPDOWN_CLASSA, WS_TABSTOP, wm_stylechanged_seq},
        {WC_SCROLLBARA, WS_TABSTOP, wm_stylechanged_seq},
    };

    parent = CreateWindowA(WC_STATICA, "parent", WS_POPUP | WS_VISIBLE, 100, 100, 100, 100,
                           0, 0, 0, 0);
    ok(parent != NULL, "CreateWindowA failed, error %lu.\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("%s", tests[i].class_name);

        hwnd = CreateWindowA(tests[i].class_name, "test", WS_CHILD | WS_VISIBLE, 0, 0, 50, 50,
                             parent, 0, 0, 0);
        /* SysLink is unavailable in comctl32 v5 */
        if (!hwnd && !lstrcmpA(tests[i].class_name, "SysLink"))
        {
            winetest_pop_context();
            continue;
        }
        ok(hwnd != NULL, "CreateWindowA failed, error %lu.\n", GetLastError());
        old_proc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)test_wm_stylechanged_proc);
        ok(old_proc != NULL, "SetWindowLongPtrA failed, error %lu.\n", GetLastError());
        flush_events();
        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        style.styleOld = GetWindowLongA(hwnd, GWL_STYLE);
        style.styleNew = style.styleOld | tests[i].add_style;
        SendMessageA(hwnd, WM_STYLECHANGED, GWL_STYLE, (LPARAM)&style);
        flush_events();
        ok_sequence(sequences, CHILD_SEQ_INDEX, tests[i].seq, "WM_STYLECHANGED", tests[i].todo);

        DestroyWindow(hwnd);
        winetest_pop_context();
    }

    DestroyWindow(parent);
}

START_TEST(misc)
{
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);
    if(!InitFunctionPtrs())
        return;

    test_GetPtrAW();
    test_Alloc();
    test_comctl32_classes(FALSE);
    test_WM_STYLECHANGED();

    FreeLibrary(hComctl32);

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;
    if(!init_functions_v6())
        return;

    test_comctl32_classes(TRUE);
    test_builtin_classes();
    test_LoadIconWithScaleDown();
    test_themed_background();
    test_WM_THEMECHANGED();
    test_WM_SYSCOLORCHANGE();
    test_WM_STYLECHANGED();

    unload_v6_module(ctx_cookie, hCtx);
    FreeLibrary(hComctl32);
}
