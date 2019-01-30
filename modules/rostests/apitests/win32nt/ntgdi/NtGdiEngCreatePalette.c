/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiEngCreatePalette
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiEngCreatePalette)
{
    HPALETTE hPal;
    ULONG Colors[3] = {1,2,3};
    PENTRY pEntry;

    hPal = NtGdiEngCreatePalette(PAL_RGB, 3, Colors, 0xff000000, 0x00ff0000, 0x0000ff00);

    ok(hPal != NULL, "hPal was NULL.\n");
    ok_int((int)GDI_HANDLE_GET_TYPE(hPal), (int)GDI_OBJECT_TYPE_PALETTE);
    pEntry = &GdiHandleTable[GDI_HANDLE_GET_INDEX(hPal)];
    ok(pEntry->einfo.pobj != NULL, "pEntry->einfo.pobj was NULL.\n");
    ok_long(pEntry->ObjectOwner.ulObj, GetCurrentProcessId());
    ok_ptr(pEntry->pUser, NULL);
    //TEST(pEntry->Type == (((UINT)hPal >> 16) | GDI_OBJECT_TYPE_PALETTE));
}
