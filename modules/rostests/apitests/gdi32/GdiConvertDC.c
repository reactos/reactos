/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GdiConvertDC
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

HDC WINAPI GdiConvertDC(HDC hdc);

void Test_GdiConvertDC()
{
    ok(GdiConvertDC((HDC)-1) == (HDC)-1, "\n");
    ok(GdiConvertDC((HDC)0) == (HDC)0, "\n");
    ok(GdiConvertDC((HDC)1) == (HDC)1, "\n");
    ok(GdiConvertDC((HDC)2) == (HDC)2, "\n");
}

START_TEST(GdiConvertDC)
{
    Test_GdiConvertDC();
}

