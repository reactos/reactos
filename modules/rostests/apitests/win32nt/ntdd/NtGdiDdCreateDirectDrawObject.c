/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiDdCreateDirectDrawObject
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiDdCreateDirectDrawObject)
{
    HANDLE hDirectDraw;
    HDC hdc = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
    ok(hdc != NULL, "CreateDCW() failed\n");

    hDirectDraw = NtGdiDdCreateDirectDrawObject(NULL);
    ok(hDirectDraw == NULL,
       "NtGdiDdCreateDirectDrawObject() succeeded on NULL device context\n");
    if (hDirectDraw != NULL)
    {
        ok(NtGdiDdDeleteDirectDrawObject(hDirectDraw) == TRUE,
           "NtGdiDdDeleteDirectDrawObject() failed on unwanted object\n");
    }

    if (hdc == NULL)
    {
        skip("No DC\n");
        return;
    }

    hDirectDraw = NtGdiDdCreateDirectDrawObject(hdc);
    ok(hDirectDraw != NULL, "NtGdiDdCreateDirectDrawObject() failed\n");
    if (hDirectDraw != NULL)
    {
        ok(NtGdiDdDeleteDirectDrawObject(hDirectDraw) == TRUE,
           "NtGdiDdDeleteDirectDrawObject() failed\n");
    }

    ok(DeleteDC(hdc) != 0, "DeleteDC() failed\n");
}
