/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GdiDeleteLocalDC
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

BOOL WINAPI GdiDeleteLocalDC(HDC);

void Test_GdiDeleteLocalDC()
{
    ok(GdiDeleteLocalDC((HDC)-1) == TRUE, "\n");
    ok(GdiDeleteLocalDC((HDC)0) == TRUE, "\n");
    ok(GdiDeleteLocalDC((HDC)1) == TRUE, "\n");
    ok(GdiDeleteLocalDC((HDC)2) == TRUE, "\n");
}

START_TEST(GdiDeleteLocalDC)
{
    Test_GdiDeleteLocalDC();
}

