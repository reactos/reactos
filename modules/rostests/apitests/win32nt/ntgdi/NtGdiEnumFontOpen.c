/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiEnumFontOpen
 * PROGRAMMERS:
 */

#include "../win32nt.h"

START_TEST(NtGdiEnumFontOpen)
{
    HDC hDC;
    ULONG_PTR idEnum;
    ULONG ulCount;
    PENTRY pEntry;

    DWORD dwOsVer = NtCurrentPeb()->OSMajorVersion << 8 | NtCurrentPeb()->OSMinorVersion;
    if (dwOsVer >= _WIN32_WINNT_WIN7)
    {
        skip("NtGdiEnumFontOpen is not supported on Windows 7 or later\n");
        return;
    }

    hDC = CreateDCW(L"DISPLAY",NULL,NULL,NULL);

    // FIXME: We should load the font first

    idEnum = NtGdiEnumFontOpen(hDC, 2, 0, 32, L"Courier", ANSI_CHARSET, &ulCount);
    ok(idEnum != 0, "idEnum was 0.\n");
    if (idEnum == 0)
    {
        skip("idEnum == 0\n");
        return;
    }

    /* we should have a gdi handle here */
    ok_int((int)GDI_HANDLE_GET_TYPE(idEnum), (int)GDI_OBJECT_TYPE_ENUMFONT);
    pEntry = &GdiHandleTable[GDI_HANDLE_GET_INDEX(idEnum)];
    ok(pEntry->einfo.pobj != NULL, "pEntry->einfo.pobj was NULL.\n");
    ok_long(pEntry->ObjectOwner.ulObj, GetCurrentProcessId());
    ok_ptr(pEntry->pUser, NULL);
    ok_int(pEntry->FullUnique, (idEnum >> 16) & 0xFFFF);
    ok_int(pEntry->Objt, GDI_OBJECT_TYPE_ENUMFONT >> 16);
    ok_int(pEntry->Flags, 0);

    /* We should not be able to use DeleteObject() on the handle */
    ok_int(DeleteObject((HGDIOBJ)idEnum), FALSE);

    NtGdiEnumFontClose(idEnum);

    // Test no logfont (NULL): should word
    // Test empty lfFaceName string: should not work
}
