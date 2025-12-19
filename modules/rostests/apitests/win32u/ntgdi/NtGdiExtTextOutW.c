/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiExtTextOutW
 * PROGRAMMERS:
 */

#include "../win32nt.h"

/*
BOOL
APIENTRY
NtGdiExtTextOutW(
    IN HDC hDC,
    IN INT XStart,
    IN INT YStart,
    IN UINT fuOptions,
    IN OPTIONAL LPRECT UnsafeRect,
    IN LPWSTR UnsafeString,
    IN INT Count,
    IN OPTIONAL LPINT UnsafeDx,
    IN DWORD dwCodePage)
*/

START_TEST(NtGdiExtTextOutW)
{
    static const LPWSTR lpstr = L"Hallo";
    ULONG len = lstrlenW(lpstr);
    RECT rect = { 0, 0, 100, 100 };
    INT Dx[10] = {10, -5, 10, 5, 10, -10, 10, 5, 10, 5};

    HDC hdc = CreateCompatibleDC(NULL);
    HBITMAP hbm = CreateCompatibleBitmap(hdc, 10, 10);
    SelectObject(hdc, hbm);

    /* Test valid call */
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0, &rect, lpstr, len, Dx, 0), 1);

    /* Test NULL hdc */
    SetLastError(0xDEADFACE);
    ok_int(NtGdiExtTextOutW(NULL, 0, 0, 0, &rect, lpstr, len, Dx, 0), 0);
    ok_err(0xDEADFACE);

    /* Test NULL rect */
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0, NULL, lpstr, len, Dx, 0), 1);

    /* Test with invalid rect */
    RECT invalid_rect = { MAXLONG, MINLONG, MINLONG, MAXLONG };
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0, &invalid_rect, lpstr, len, Dx, 0), 1);

    /* Test NULL lpstr with zero len */
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0, &rect, NULL, 0, Dx, 0), 1);

    /* Test NULL lpstr with non-zero len */
    SetLastError(0xDEADFACE);
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0, &rect, NULL, len, Dx, 0), 0);
    ok_err(0xDEADFACE);

    /* Test NULL pdx and NULL lpstr with zero len */
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0, &rect, NULL, 0, NULL, 0), 1);

    /* Test NULL pdx and valid lpstr with zero len */
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0, &rect, lpstr, 0, NULL, 0), 1);

    /* Test NULL pdx and valid lpstr with non-zero len */
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0, &rect, lpstr, len, NULL, 0), 1);

    /* Test NULL pdx and ETO_PDY */
    if (GetNTVersion() <= _WIN32_WINNT_VISTA)
    {
        ok_int(NtGdiExtTextOutW(hdc, 0, 0, ETO_PDY, &rect, lpstr, len, NULL, 0), 1);
    }
    else
    {
        SetLastError(0xDEADFACE);
        ok_int(NtGdiExtTextOutW(hdc, 0, 0, ETO_PDY, &rect, lpstr, len, NULL, 0), 0);
        ok_err(ERROR_INVALID_PARAMETER);
    }

    /* Test different flOpts */
    SetLastError(0xDEADFACE);
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000001, &rect, lpstr, len, Dx, 0), 0); // ETO_GRAYED
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000002, &rect, lpstr, len, Dx, 0), 1); // ETO_OPAQUE
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000005, &rect, lpstr, len, Dx, 0), 0); // ETO_OPAQUE | ETO_GRAYED
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000004, &rect, lpstr, len, Dx, 0), 1); // ETO_CLIPPED
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000008, &rect, lpstr, len, Dx, 0), 0);
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000010, &rect, lpstr, len, Dx, 0), 1); // ETO_GLYPH_INDEX
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000020, &rect, lpstr, len, Dx, 0), 0);
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000040, &rect, lpstr, len, Dx, 0), 0);
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000080, &rect, lpstr, len, Dx, 0), 1); // ETO_RTLREADING
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000100, &rect, lpstr, len, Dx, 0), 0);
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000200, &rect, lpstr, len, Dx, 0), 0);
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000400, &rect, lpstr, len, Dx, 0), 1); // ETO_NUMERICSLOCAL
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00000800, &rect, lpstr, len, Dx, 0), 1); // ETO_NUMERICSLATIN
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00001000, &rect, lpstr, len, Dx, 0), 1); // ETO_IGNORELANGUAGE
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00002000, &rect, lpstr, len, Dx, 0), 1); // ETO_PDY
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00004000, &rect, lpstr, len, Dx, 0), 0);
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00008000, &rect, lpstr, len, Dx, 0), 0);
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00010000, &rect, lpstr, len, Dx, 0), 0); // ETO_REVERSE_INDEX_MAP
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0x00020000, &rect, lpstr, len, Dx, 0), 0);
    ok_err(0xDEADFACE);

    /* Test invalid lpDx */
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0, 0, lpstr, len, (INT*)((ULONG_PTR)-1), 0), 0);

    /* Test alignment requirement for lpDx */
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0, 0, lpstr, len, (INT*)((ULONG_PTR)Dx + 1), 0),
              (GetNTVersion() >= _WIN32_WINNT_WIN10) ? 1 : 0);

    /* Test invalid codepage */
    ok_int(NtGdiExtTextOutW(hdc, 0, 0, 0, &rect, lpstr, len, Dx, 0xDEADCAFE), 1);

    DeleteDC(hdc);
    DeleteObject(hbm);
}
