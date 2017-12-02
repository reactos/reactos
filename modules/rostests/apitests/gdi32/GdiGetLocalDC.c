/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GdiGetLocalDC
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

HDC WINAPI GdiGetLocalDC(HDC);

void Test_GdiGetLocalDC()
{
    ok(GdiGetLocalDC((HDC)-1) == (HDC)-1, "\n");
    ok(GdiGetLocalDC((HDC)0) == (HDC)0, "\n");
    ok(GdiGetLocalDC((HDC)1) == (HDC)1, "\n");
    ok(GdiGetLocalDC((HDC)2) == (HDC)2, "\n");
}

START_TEST(GdiGetLocalDC)
{
    Test_GdiGetLocalDC();
}

