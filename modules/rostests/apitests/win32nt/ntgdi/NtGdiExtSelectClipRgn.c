/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiCombineRgn
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiExtSelectClipRgn)
{
    HRGN hRgnDest, hRgn1, hRgn2;
    HDC hdc;
// test what params are accepted for what operations
// 0? invalid? are params maybe ignored in some cases?
// LastError

    /* Preparation */
    hRgnDest = CreateRectRgn(100, 100, 100, 100);
    hRgn1 = CreateRectRgn(1,1,4,4);
    hRgn2 = CreateRectRgn(2,2,6,3);

    hdc = GetDC(NULL);

    /* RGN_AND = 1, RGN_OR = 2, RGN_XOR = 3, RGN_DIFF = 4, RGN_COPY = 5 */

    SetLastError(0xDEADFACE);
    ok_int(NtGdiExtSelectClipRgn(NULL, NULL, RGN_AND-1), ERROR);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);
    SetLastError(0xDEADFACE);
    ok_int(NtGdiExtSelectClipRgn(NULL, NULL, RGN_COPY+1), ERROR);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);
    SetLastError(0xDEADFACE);
    ok_int(NtGdiExtSelectClipRgn(NULL, NULL, RGN_COPY), ERROR);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);
    ok_int(NtGdiExtSelectClipRgn(hdc, hRgnDest, RGN_COPY), NULLREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, NULL, RGN_COPY), SIMPLEREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, hRgnDest, RGN_DIFF), SIMPLEREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, hRgn1, RGN_COPY), SIMPLEREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, hRgn2, RGN_DIFF), COMPLEXREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, NULL, RGN_COPY), SIMPLEREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, NULL, RGN_COPY), SIMPLEREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, hRgnDest, RGN_COPY), NULLREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, hRgn1, RGN_AND), NULLREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, hRgn2, RGN_AND), NULLREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, NULL, RGN_COPY), SIMPLEREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, hRgn1, RGN_OR), SIMPLEREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, hRgn2, RGN_OR), SIMPLEREGION);
    ok_int(NtGdiExtSelectClipRgn(hdc, hRgn1, RGN_XOR), COMPLEXREGION);
    SetLastError(0xDEADFACE);
    ok_int(NtGdiExtSelectClipRgn(hdc, NULL, RGN_DIFF), ERROR);
    ok_int(NtGdiExtSelectClipRgn(hdc, NULL, RGN_OR), ERROR);
    ok_int(NtGdiExtSelectClipRgn(hdc, NULL, RGN_XOR), ERROR);
    ok_int(NtGdiExtSelectClipRgn(hdc, NULL, RGN_AND), ERROR);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(NtGdiExtSelectClipRgn(hdc, NULL, RGN_COPY), SIMPLEREGION);
}
