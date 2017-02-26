/* Unit test suite for uxtheme API functions
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
 *
 */

#include <stdarg.h>

#include "windows.h"
#include "vfwmsgs.h"
#include "uxtheme.h"

#include "wine/test.h"

static HTHEME  (WINAPI * pOpenThemeDataEx)(HWND, LPCWSTR, DWORD);
static HPAINTBUFFER (WINAPI *pBeginBufferedPaint)(HDC, const RECT *, BP_BUFFERFORMAT, BP_PAINTPARAMS *, HDC *);
static HRESULT (WINAPI *pBufferedPaintClear)(HPAINTBUFFER, const RECT *);
static HRESULT (WINAPI *pEndBufferedPaint)(HPAINTBUFFER, BOOL);
static HRESULT (WINAPI *pGetBufferedPaintBits)(HPAINTBUFFER, RGBQUAD **, int *);
static HDC (WINAPI *pGetBufferedPaintDC)(HPAINTBUFFER);
static HDC (WINAPI *pGetBufferedPaintTargetDC)(HPAINTBUFFER);
static HRESULT (WINAPI *pGetBufferedPaintTargetRect)(HPAINTBUFFER, RECT *);

static void init_funcs(void)
{
    HMODULE hUxtheme = GetModuleHandleA("uxtheme.dll");

#define UXTHEME_GET_PROC(func) p ## func = (void*)GetProcAddress(hUxtheme, #func)
    UXTHEME_GET_PROC(BeginBufferedPaint);
    UXTHEME_GET_PROC(BufferedPaintClear);
    UXTHEME_GET_PROC(EndBufferedPaint);
    UXTHEME_GET_PROC(GetBufferedPaintBits);
    UXTHEME_GET_PROC(GetBufferedPaintDC);
    UXTHEME_GET_PROC(GetBufferedPaintTargetDC);
    UXTHEME_GET_PROC(GetBufferedPaintTargetRect);
    UXTHEME_GET_PROC(BufferedPaintClear);

    UXTHEME_GET_PROC(OpenThemeDataEx);
#undef UXTHEME_GET_PROC
}

static void test_IsThemed(void)
{
    BOOL bThemeActive;
    BOOL bAppThemed;
    BOOL bTPDefined;

    bThemeActive = IsThemeActive();
    trace("Theming is %s\n", (bThemeActive) ? "active" : "inactive");

    bAppThemed = IsAppThemed();
    trace("Test executable is %s\n", (bAppThemed) ? "themed" : "not themed");

    SetLastError(0xdeadbeef);
    bTPDefined = IsThemePartDefined(NULL, 0 , 0);
    ok( bTPDefined == FALSE, "Expected FALSE\n");
    ok( GetLastError() == E_HANDLE,
        "Expected E_HANDLE, got 0x%08x\n",
        GetLastError());
}

static void test_GetWindowTheme(void)
{
    HTHEME    hTheme;
    HWND      hWnd;
    BOOL    bDestroyed;

    SetLastError(0xdeadbeef);
    hTheme = GetWindowTheme(NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_HANDLE,
            "Expected E_HANDLE, got 0x%08x\n",
            GetLastError());

    /* Only do the bare minimum to get a valid hwnd */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    SetLastError(0xdeadbeef);
    hTheme = GetWindowTheme(hWnd);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08x\n",
        GetLastError());

    bDestroyed = DestroyWindow(hWnd);
    if (!bDestroyed)
        trace("Window %p couldn't be destroyed : 0x%08x\n",
            hWnd, GetLastError());
}

static void test_SetWindowTheme(void)
{
    HRESULT hRes;
    HWND    hWnd;
    BOOL    bDestroyed;

    hRes = SetWindowTheme(NULL, NULL, NULL);
todo_wine
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08x\n", hRes);

    /* Only do the bare minimum to get a valid hwnd */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    hRes = SetWindowTheme(hWnd, NULL, NULL);
    ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);

    bDestroyed = DestroyWindow(hWnd);
    if (!bDestroyed)
        trace("Window %p couldn't be destroyed : 0x%08x\n",
            hWnd, GetLastError());
}

static void test_OpenThemeData(void)
{
    HTHEME    hTheme, hTheme2;
    HWND      hWnd;
    BOOL      bThemeActive;
    HRESULT   hRes;
    BOOL      bDestroyed;
    BOOL      bTPDefined;

    WCHAR szInvalidClassList[] = {'D','E','A','D','B','E','E','F', 0 };
    WCHAR szButtonClassList[]  = {'B','u','t','t','o','n', 0 };
    WCHAR szButtonClassList2[]  = {'b','U','t','T','o','N', 0 };
    WCHAR szClassList[]        = {'B','u','t','t','o','n',';','L','i','s','t','B','o','x', 0 };

    bThemeActive = IsThemeActive();

    /* All NULL */
    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(NULL, NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08x\n",
            GetLastError());

    /* A NULL hWnd and an invalid classlist */
    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(NULL, szInvalidClassList);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
            "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(NULL, szClassList);
    if (bThemeActive)
    {
        ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
        todo_wine
            ok( GetLastError() == ERROR_SUCCESS,
                "Expected ERROR_SUCCESS, got 0x%08x\n",
                GetLastError());
    }
    else
    {
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        todo_wine
            ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
                "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08x\n",
                GetLastError());
    }

    /* Only do the bare minimum to get a valid hdc */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szInvalidClassList);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
            "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08x\n",
            GetLastError());

    /* Close invalid handle */
    hRes = CloseThemeData((HTHEME)0xdeadbeef);
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08x\n", hRes);

    if (!bThemeActive)
    {
        SetLastError(0xdeadbeef);
        hTheme = OpenThemeData(hWnd, szButtonClassList);
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        todo_wine
            ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
                "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08x\n",
                GetLastError());
        skip("No active theme, skipping rest of OpenThemeData tests\n");
        return;
    }

    /* Only do the next checks if we have an active theme */

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szButtonClassList);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    /* Test with bUtToN instead of Button */
    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szButtonClassList2);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szClassList);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    /* GetWindowTheme should return the last handle opened by OpenThemeData */
    SetLastError(0xdeadbeef);
    hTheme2 = GetWindowTheme(hWnd);
    ok( hTheme == hTheme2, "Expected the same HTHEME handle (%p<->%p)\n",
        hTheme, hTheme2);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08x\n",
        GetLastError());

    hRes = CloseThemeData(hTheme);
    ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);

    /* Close a second time */
    hRes = CloseThemeData(hTheme);
    ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);

    /* See if closing makes a difference for GetWindowTheme */
    SetLastError(0xdeadbeef);
    hTheme2 = NULL;
    hTheme2 = GetWindowTheme(hWnd);
    ok( hTheme == hTheme2, "Expected the same HTHEME handle (%p<->%p)\n",
        hTheme, hTheme2);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08x\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    bTPDefined = IsThemePartDefined(hTheme, 0 , 0);
    todo_wine
    {
        ok( bTPDefined == FALSE, "Expected FALSE\n");
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());
    }

    bDestroyed = DestroyWindow(hWnd);
    if (!bDestroyed)
        trace("Window %p couldn't be destroyed : 0x%08x\n",
            hWnd, GetLastError());
}

static void test_OpenThemeDataEx(void)
{
    HTHEME    hTheme;
    HWND      hWnd;
    BOOL      bThemeActive;
    BOOL      bDestroyed;

    WCHAR szInvalidClassList[] = {'D','E','A','D','B','E','E','F', 0 };
    WCHAR szButtonClassList[]  = {'B','u','t','t','o','n', 0 };
    WCHAR szButtonClassList2[]  = {'b','U','t','T','o','N', 0 };
    WCHAR szClassList[]        = {'B','u','t','t','o','n',';','L','i','s','t','B','o','x', 0 };

    if (!pOpenThemeDataEx)
    {
        win_skip("OpenThemeDataEx not available\n");
        return;
    }

    bThemeActive = IsThemeActive();

    /* All NULL */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(NULL, NULL, 0);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08x\n",
            GetLastError());

    /* A NULL hWnd and an invalid classlist without flags */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(NULL, szInvalidClassList, 0);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
            "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(NULL, szClassList, 0);
    if (bThemeActive)
    {
        ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
        todo_wine
            ok( GetLastError() == ERROR_SUCCESS,
                "Expected ERROR_SUCCESS, got 0x%08x\n",
                GetLastError());
    }
    else
    {
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        todo_wine
            ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
                "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08x\n",
                GetLastError());
    }

    /* Only do the bare minimum to get a valid hdc */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, NULL, 0);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szInvalidClassList, 0);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
            "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08x\n",
            GetLastError());

    if (!bThemeActive)
    {
        SetLastError(0xdeadbeef);
        hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, 0);
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        todo_wine
            ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
                "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08x\n",
                GetLastError());
        skip("No active theme, skipping rest of OpenThemeDataEx tests\n");
        return;
    }

    /* Only do the next checks if we have an active theme */

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, 0);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, OTD_FORCE_RECT_SIZING);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, OTD_NONCLIENT);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, 0x3);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    /* Test with bUtToN instead of Button */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList2, 0);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szClassList, 0);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    bDestroyed = DestroyWindow(hWnd);
    if (!bDestroyed)
        trace("Window %p couldn't be destroyed : 0x%08x\n",
            hWnd, GetLastError());
}

static void test_GetCurrentThemeName(void)
{
    BOOL    bThemeActive;
    HRESULT hRes;
    WCHAR currentTheme[MAX_PATH];
    WCHAR currentColor[MAX_PATH];
    WCHAR currentSize[MAX_PATH];

    bThemeActive = IsThemeActive();

    /* All NULLs */
    hRes = GetCurrentThemeName(NULL, 0, NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* Number of characters given is 0 */
    hRes = GetCurrentThemeName(currentTheme, 0, NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == S_OK || broken(hRes == E_FAIL /* WinXP SP1 */), "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    hRes = GetCurrentThemeName(currentTheme, 2, NULL, 0, NULL, 0);
    if (bThemeActive)
        todo_wine
            ok(hRes == E_NOT_SUFFICIENT_BUFFER ||
               broken(hRes == E_FAIL /* WinXP SP1 */),
               "Expected E_NOT_SUFFICIENT_BUFFER, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* The same is true if the number of characters is too small for Color and/or Size */
    hRes = GetCurrentThemeName(currentTheme, sizeof(currentTheme) / sizeof(WCHAR),
                                currentColor, 2,
                                currentSize,  sizeof(currentSize)  / sizeof(WCHAR));
    if (bThemeActive)
        todo_wine
            ok(hRes == E_NOT_SUFFICIENT_BUFFER ||
               broken(hRes == E_FAIL /* WinXP SP1 */),
               "Expected E_NOT_SUFFICIENT_BUFFER, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* Given number of characters is correct */
    hRes = GetCurrentThemeName(currentTheme, sizeof(currentTheme) / sizeof(WCHAR), NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* Given number of characters for the theme name is too large */
    hRes = GetCurrentThemeName(currentTheme, sizeof(currentTheme), NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == E_POINTER || hRes == S_OK, "Expected E_POINTER or S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED ||
            hRes == E_POINTER, /* win2k3 */
            "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);
 
    /* The too large case is only for the theme name, not for color name or size name */
    hRes = GetCurrentThemeName(currentTheme, sizeof(currentTheme) / sizeof(WCHAR),
                                currentColor, sizeof(currentTheme),
                                currentSize,  sizeof(currentSize)  / sizeof(WCHAR));
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    hRes = GetCurrentThemeName(currentTheme, sizeof(currentTheme) / sizeof(WCHAR),
                                currentColor, sizeof(currentTheme) / sizeof(WCHAR),
                                currentSize,  sizeof(currentSize));
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* Correct call */
    hRes = GetCurrentThemeName(currentTheme, sizeof(currentTheme) / sizeof(WCHAR),
                                currentColor, sizeof(currentColor) / sizeof(WCHAR),
                                currentSize,  sizeof(currentSize)  / sizeof(WCHAR));
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);
}

static void test_CloseThemeData(void)
{
    HRESULT hRes;

    hRes = CloseThemeData(NULL);
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08x\n", hRes);
    hRes = CloseThemeData(INVALID_HANDLE_VALUE);
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08x\n", hRes);
}

static void test_buffered_paint(void)
{
    BP_PAINTPARAMS params = { 0 };
    BP_BUFFERFORMAT format;
    HDC target, src, hdc;
    HPAINTBUFFER buffer;
    RECT rect, rect2;
    RGBQUAD *bits;
    HRESULT hr;
    int row;

    if (!pBeginBufferedPaint)
    {
        win_skip("Buffered painting API is not supported.\n");
        return;
    }

    buffer = pBeginBufferedPaint(NULL, NULL, BPBF_COMPATIBLEBITMAP,
            NULL, NULL);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);

    target = CreateCompatibleDC(0);
    buffer = pBeginBufferedPaint(target, NULL, BPBF_COMPATIBLEBITMAP,
            NULL, NULL);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);

    params.cbSize = sizeof(params);
    buffer = pBeginBufferedPaint(target, NULL, BPBF_COMPATIBLEBITMAP,
            &params, NULL);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);

    src = (void *)0xdeadbeef;
    buffer = pBeginBufferedPaint(target, NULL, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);
    ok(src == NULL, "Unexpected buffered dc %p\n", src);

    /* target rect is mandatory */
    SetRectEmpty(&rect);
    src = (void *)0xdeadbeef;
    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);
    ok(src == NULL, "Unexpected buffered dc %p\n", src);

    /* inverted rectangle */
    SetRect(&rect, 10, 0, 5, 5);
    src = (void *)0xdeadbeef;
    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);
    ok(src == NULL, "Unexpected buffered dc %p\n", src);

    SetRect(&rect, 0, 10, 5, 0);
    src = (void *)0xdeadbeef;
    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);
    ok(src == NULL, "Unexpected buffered dc %p\n", src);

    /* valid rectangle, no target dc */
    SetRect(&rect, 0, 0, 5, 5);
    src = (void *)0xdeadbeef;
    buffer = pBeginBufferedPaint(NULL, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);
    ok(src == NULL, "Unexpected buffered dc %p\n", src);

    SetRect(&rect, 0, 0, 5, 5);
    src = NULL;
    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer != NULL, "Unexpected buffer %p\n", buffer);
    ok(src != NULL, "Expected buffered dc\n");
    hr = pEndBufferedPaint(buffer, FALSE);
    ok(hr == S_OK, "Unexpected return code %#x\n", hr);

    SetRect(&rect, 0, 0, 5, 5);
    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer != NULL, "Unexpected buffer %p\n", buffer);

    /* clearing */
    hr = pBufferedPaintClear(NULL, NULL);
todo_wine
    ok(hr == E_FAIL, "Unexpected return code %#x\n", hr);

    hr = pBufferedPaintClear(buffer, NULL);
todo_wine
    ok(hr == S_OK, "Unexpected return code %#x\n", hr);

    /* access buffer attributes */
    hdc = pGetBufferedPaintDC(buffer);
    ok(hdc == src, "Unexpected hdc, %p, buffered dc %p\n", hdc, src);

    hdc = pGetBufferedPaintTargetDC(buffer);
    ok(hdc == target, "Unexpected target hdc %p, original %p\n", hdc, target);

    hr = pGetBufferedPaintTargetRect(NULL, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

    hr = pGetBufferedPaintTargetRect(buffer, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

    hr = pGetBufferedPaintTargetRect(NULL, &rect2);
    ok(hr == E_FAIL, "Unexpected return code %#x\n", hr);

    SetRectEmpty(&rect2);
    hr = pGetBufferedPaintTargetRect(buffer, &rect2);
    ok(hr == S_OK, "Unexpected return code %#x\n", hr);
    ok(EqualRect(&rect, &rect2), "Wrong target rect\n");

    hr = pEndBufferedPaint(buffer, FALSE);
    ok(hr == S_OK, "Unexpected return code %#x\n", hr);

    /* invalid buffer handle */
    hr = pEndBufferedPaint(NULL, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected return code %#x\n", hr);

    hdc = pGetBufferedPaintDC(NULL);
    ok(hdc == NULL, "Unexpected hdc %p\n", hdc);

    hdc = pGetBufferedPaintTargetDC(NULL);
    ok(hdc == NULL, "Unexpected target hdc %p\n", hdc);

    hr = pGetBufferedPaintTargetRect(NULL, &rect2);
    ok(hr == E_FAIL, "Unexpected return code %#x\n", hr);

    hr = pGetBufferedPaintTargetRect(NULL, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

    bits = (void *)0xdeadbeef;
    row = 10;
    hr = pGetBufferedPaintBits(NULL, &bits, &row);
    ok(hr == E_FAIL, "Unexpected return code %#x\n", hr);
    ok(row == 10, "Unexpected row count %d\n", row);
    ok(bits == (void *)0xdeadbeef, "Unepexpected data pointer %p\n", bits);

    hr = pGetBufferedPaintBits(NULL, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

    hr = pGetBufferedPaintBits(NULL, &bits, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

    hr = pGetBufferedPaintBits(NULL, NULL, &row);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

    /* access buffer bits */
    for (format = BPBF_COMPATIBLEBITMAP; format <= BPBF_TOPDOWNMONODIB; format++)
    {
        buffer = pBeginBufferedPaint(target, &rect, format, &params, &src);

        /* only works for DIB buffers */
        bits = NULL;
        row = 0;
        hr = pGetBufferedPaintBits(buffer, &bits, &row);
        if (format == BPBF_COMPATIBLEBITMAP)
            ok(hr == E_FAIL, "Unexpected return code %#x\n", hr);
        else
        {
            ok(hr == S_OK, "Unexpected return code %#x\n", hr);
            ok(bits != NULL, "Bitmap bits %p\n", bits);
            ok(row >= (rect.right - rect.left), "format %d: bitmap width %d\n", format, row);
        }

        hr = pEndBufferedPaint(buffer, FALSE);
        ok(hr == S_OK, "Unexpected return code %#x\n", hr);
    }

    DeleteDC(target);
}

START_TEST(system)
{
    init_funcs();

    /* No real functional tests will be done (yet). The current tests
     * only show input/return behaviour
     */

    /* IsThemeActive, IsAppThemed and IsThemePartDefined*/
    trace("Starting test_IsThemed()\n");
    test_IsThemed();

    /* GetWindowTheme */
    trace("Starting test_GetWindowTheme()\n");
    test_GetWindowTheme();

    /* SetWindowTheme */
    trace("Starting test_SetWindowTheme()\n");
    test_SetWindowTheme();

    /* OpenThemeData, a bit more functional now */
    trace("Starting test_OpenThemeData()\n");
    test_OpenThemeData();

    /* OpenThemeDataEx */
    trace("Starting test_OpenThemeDataEx()\n");
    test_OpenThemeDataEx();

    /* GetCurrentThemeName */
    trace("Starting test_GetCurrentThemeName()\n");
    test_GetCurrentThemeName();

    /* CloseThemeData */
    trace("Starting test_CloseThemeData()\n");
    test_CloseThemeData();

    test_buffered_paint();
}
