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

static HRESULT (WINAPI * pCloseThemeData)(HTHEME);
static HRESULT (WINAPI * pGetCurrentThemeName)(LPWSTR, int, LPWSTR, int, LPWSTR, int);
static HTHEME  (WINAPI * pGetWindowTheme)(HWND);
static BOOL    (WINAPI * pIsAppThemed)(VOID);
static BOOL    (WINAPI * pIsThemeActive)(VOID);
static BOOL    (WINAPI * pIsThemePartDefined)(HTHEME, int, int);
static HTHEME  (WINAPI * pOpenThemeData)(HWND, LPCWSTR);
static HTHEME  (WINAPI * pOpenThemeDataEx)(HWND, LPCWSTR, DWORD);
static HRESULT (WINAPI * pSetWindowTheme)(HWND, LPCWSTR, LPCWSTR);

static HMODULE hUxtheme = 0;

#define UXTHEME_GET_PROC(func) p ## func = (void*)GetProcAddress(hUxtheme, #func);

static BOOL InitFunctionPtrs(void)
{
    hUxtheme = LoadLibraryA("uxtheme.dll");
    if(!hUxtheme) {
      trace("Could not load uxtheme.dll\n");
      return FALSE;
    }
    if (hUxtheme)
    {
      UXTHEME_GET_PROC(CloseThemeData)
      UXTHEME_GET_PROC(GetCurrentThemeName)
      UXTHEME_GET_PROC(GetWindowTheme)
      UXTHEME_GET_PROC(IsAppThemed)
      UXTHEME_GET_PROC(IsThemeActive)
      UXTHEME_GET_PROC(IsThemePartDefined)
      UXTHEME_GET_PROC(OpenThemeData)
      UXTHEME_GET_PROC(OpenThemeDataEx)
      UXTHEME_GET_PROC(SetWindowTheme)
    }
    /* The following functions should be available, if not return FALSE. The Vista functions will
     * be checked (at some point in time) within the single tests if needed. All used functions for
     * now are present on WinXP, W2K3 and Wine.
     */
    if (!pCloseThemeData || !pGetCurrentThemeName ||
        !pGetWindowTheme || !pIsAppThemed ||
        !pIsThemeActive || !pIsThemePartDefined ||
        !pOpenThemeData || !pSetWindowTheme)
    {
        FreeLibrary(hUxtheme);
        return FALSE;
    }

    return TRUE;
}

static void test_IsThemed(void)
{
    BOOL bThemeActive;
    BOOL bAppThemed;
    BOOL bTPDefined;

    bThemeActive = pIsThemeActive();
    trace("Theming is %s\n", (bThemeActive) ? "active" : "inactive");

    bAppThemed = pIsAppThemed();
    trace("Test executable is %s\n", (bAppThemed) ? "themed" : "not themed");

    SetLastError(0xdeadbeef);
    bTPDefined = pIsThemePartDefined(NULL, 0 , 0);
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
    hTheme = pGetWindowTheme(NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_HANDLE,
            "Expected E_HANDLE, got 0x%08x\n",
            GetLastError());

    /* Only do the bare minimum to get a valid hwnd */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    SetLastError(0xdeadbeef);
    hTheme = pGetWindowTheme(hWnd);
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

    hRes = pSetWindowTheme(NULL, NULL, NULL);
todo_wine
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08x\n", hRes);

    /* Only do the bare minimum to get a valid hwnd */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    hRes = pSetWindowTheme(hWnd, NULL, NULL);
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

    bThemeActive = pIsThemeActive();

    /* All NULL */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(NULL, NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08x\n",
            GetLastError());

    /* A NULL hWnd and an invalid classlist */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(NULL, szInvalidClassList);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
            "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(NULL, szClassList);
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
    hTheme = pOpenThemeData(hWnd, NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(hWnd, szInvalidClassList);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    todo_wine
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED,
            "Expected GLE() to be E_PROP_ID_UNSUPPORTED, got 0x%08x\n",
            GetLastError());

    if (!bThemeActive)
    {
        SetLastError(0xdeadbeef);
        hTheme = pOpenThemeData(hWnd, szButtonClassList);
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
    hTheme = pOpenThemeData(hWnd, szButtonClassList);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    /* Test with bUtToN instead of Button */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(hWnd, szButtonClassList2);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeData(hWnd, szClassList);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    todo_wine
        ok( GetLastError() == ERROR_SUCCESS,
            "Expected ERROR_SUCCESS, got 0x%08x\n",
            GetLastError());

    /* GetWindowTheme should return the last handle opened by OpenThemeData */
    SetLastError(0xdeadbeef);
    hTheme2 = pGetWindowTheme(hWnd);
    ok( hTheme == hTheme2, "Expected the same HTHEME handle (%p<->%p)\n",
        hTheme, hTheme2);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08x\n",
        GetLastError());

    hRes = pCloseThemeData(hTheme);
    ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);

    /* Close a second time */
    hRes = pCloseThemeData(hTheme);
    ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);

    /* See if closing makes a difference for GetWindowTheme */
    SetLastError(0xdeadbeef);
    hTheme2 = NULL;
    hTheme2 = pGetWindowTheme(hWnd);
    ok( hTheme == hTheme2, "Expected the same HTHEME handle (%p<->%p)\n",
        hTheme, hTheme2);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08x\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    bTPDefined = pIsThemePartDefined(hTheme, 0 , 0);
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

    bThemeActive = pIsThemeActive();

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

    bThemeActive = pIsThemeActive();

    /* All NULLs */
    hRes = pGetCurrentThemeName(NULL, 0, NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* Number of characters given is 0 */
    hRes = pGetCurrentThemeName(currentTheme, 0, NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == S_OK || broken(hRes == E_FAIL /* WinXP SP1 */), "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    hRes = pGetCurrentThemeName(currentTheme, 2, NULL, 0, NULL, 0);
    if (bThemeActive)
        todo_wine
            ok(hRes == E_NOT_SUFFICIENT_BUFFER ||
               broken(hRes == E_FAIL /* WinXP SP1 */),
               "Expected E_NOT_SUFFICIENT_BUFFER, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* The same is true if the number of characters is too small for Color and/or Size */
    hRes = pGetCurrentThemeName(currentTheme, sizeof(currentTheme) / sizeof(WCHAR), 
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
    hRes = pGetCurrentThemeName(currentTheme, sizeof(currentTheme) / sizeof(WCHAR), NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* Given number of characters for the theme name is too large */
    hRes = pGetCurrentThemeName(currentTheme, sizeof(currentTheme), NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == E_POINTER || hRes == S_OK, "Expected E_POINTER or S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED ||
            hRes == E_POINTER, /* win2k3 */
            "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);
 
    /* The too large case is only for the theme name, not for color name or size name */
    hRes = pGetCurrentThemeName(currentTheme, sizeof(currentTheme) / sizeof(WCHAR), 
                                currentColor, sizeof(currentTheme),
                                currentSize,  sizeof(currentSize)  / sizeof(WCHAR));
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    hRes = pGetCurrentThemeName(currentTheme, sizeof(currentTheme) / sizeof(WCHAR), 
                                currentColor, sizeof(currentTheme) / sizeof(WCHAR),
                                currentSize,  sizeof(currentSize));
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* Correct call */
    hRes = pGetCurrentThemeName(currentTheme, sizeof(currentTheme) / sizeof(WCHAR), 
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

    hRes = pCloseThemeData(NULL);
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08x\n", hRes);
}

START_TEST(system)
{
    if(!InitFunctionPtrs())
        return;

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

    FreeLibrary(hUxtheme);
}
