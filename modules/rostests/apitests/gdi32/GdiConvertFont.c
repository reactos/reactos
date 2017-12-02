/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GdiConvertFont
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

HFONT WINAPI GdiConvertFont(HFONT);

void Test_GdiConvertFont()
{
    ok(GdiConvertFont((HFONT)-1) == (HFONT)-1, "\n");
    ok(GdiConvertFont((HFONT)0) == (HFONT)0, "\n");
    ok(GdiConvertFont((HFONT)1) == (HFONT)1, "\n");
    ok(GdiConvertFont((HFONT)2) == (HFONT)2, "\n");
}

START_TEST(GdiConvertFont)
{
    Test_GdiConvertFont();
}

