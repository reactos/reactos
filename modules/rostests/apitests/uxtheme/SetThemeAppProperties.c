/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetThemeAppProperties
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <apitest.h>
#include <stdio.h>
#include <windows.h>
#include <uxtheme.h>
#include <vfwmsgs.h>

START_TEST(SetThemeAppProperties)
{
    BOOL bThemeActive;
    HTHEME hTheme;
    HWND hWnd;

    bThemeActive = IsThemeActive();
    if (!bThemeActive)
    {
        skip("No active theme, skipping SetWindowTheme tests\n");
        return;
    }

    SetLastError(0xdeadbeef);

    bThemeActive = IsAppThemed();
    ok (bThemeActive == FALSE, "\n");
    ok( GetLastError() == 0, "Expected 0 last error, got 0x%lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(NULL, L"BUTTON");
    ok (hTheme == NULL, "\n");
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED last error, got 0x%lx\n", GetLastError());

    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    ok (hWnd != NULL, "\n");

    SetLastError(0xdeadbeef);
    bThemeActive = IsAppThemed();
    ok (bThemeActive == TRUE, "\n");
    ok( GetLastError() == 0, "Expected 0 last error, got 0x%lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(NULL, L"BUTTON");
    ok (hTheme != NULL, "\n");
    ok( GetLastError() == 0, "Expected 0 last error, got 0x%lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    SetThemeAppProperties(0);
    ok( GetLastError() == 0, "Expected 0 last error, got 0x%lx\n", GetLastError());

    bThemeActive = IsThemeActive();
    ok (bThemeActive == TRUE, "\n");

    bThemeActive = IsAppThemed();
    ok (bThemeActive == TRUE, "\n");

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(NULL, L"BUTTON");
    ok (hTheme == NULL, "\n");
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED last error, got 0x%lx\n", GetLastError());

    SetThemeAppProperties(STAP_ALLOW_NONCLIENT);

    hTheme = OpenThemeDataEx (NULL, L"BUTTON", OTD_NONCLIENT);
    ok (hTheme != NULL, "\n");
    SetLastError(0xdeadbeef);
    hTheme = OpenThemeDataEx (NULL, L"BUTTON", 0);
    ok (hTheme == NULL, "\n");
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED last error, got 0x%lx\n", GetLastError());

    SetThemeAppProperties(STAP_ALLOW_CONTROLS);

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeDataEx (NULL, L"BUTTON", OTD_NONCLIENT);
    ok (hTheme == NULL, "\n");
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED last error, got 0x%lx\n", GetLastError());
    hTheme = OpenThemeDataEx (NULL, L"BUTTON", 0);
    ok (hTheme != NULL, "\n");

}
