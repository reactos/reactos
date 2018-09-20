/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiDdDeleteDirectDrawObject
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiDdDeleteDirectDrawObject)
{
    HANDLE hDirectDraw;
    HDC hdc = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
    ok(hdc != NULL, "CreateDCW() failed\n");

    ok(NtGdiDdDeleteDirectDrawObject(NULL) == FALSE,
       "NtGdiDdDeleteDirectDrawObject() succeeded on NULL object\n");

    if (hdc == NULL)
    {
        skip("No DC\n");
        return;
    }

    hDirectDraw = NtGdiDdCreateDirectDrawObject(hdc);
    ok(hDirectDraw != NULL, "NtGdiDdCreateDirectDrawObject() failed\n");

    if (hDirectDraw == NULL)
    {
        skip("No DirectDrawObject\n");
        ok(DeleteDC(hdc) != 0, "DeleteDC() failed\n");
        return;
    }

    ok(NtGdiDdDeleteDirectDrawObject(hDirectDraw) == TRUE,
       "NtGdiDdDeleteDirectDrawObject() failed on existing object\n");
    ok(NtGdiDdDeleteDirectDrawObject(hDirectDraw) == FALSE,
       "NtGdiDdDeleteDirectDrawObject() succeeded on deleted object\n");

    ok(DeleteDC(hdc) != 0, "DeleteDC() failed\n");
}
