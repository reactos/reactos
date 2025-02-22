/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GdiConvertPalette
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

HPALETTE WINAPI GdiConvertPalette(HPALETTE);

void Test_GdiConvertPalette()
{
    ok(GdiConvertPalette((HPALETTE)-1) == (HPALETTE)-1, "\n");
    ok(GdiConvertPalette((HPALETTE)0) == (HPALETTE)0, "\n");
    ok(GdiConvertPalette((HPALETTE)1) == (HPALETTE)1, "\n");
    ok(GdiConvertPalette((HPALETTE)2) == (HPALETTE)2, "\n");
}

START_TEST(GdiConvertPalette)
{
    Test_GdiConvertPalette();
}

