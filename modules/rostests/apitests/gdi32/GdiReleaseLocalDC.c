/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GdiReleaseLocalDC
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

BOOL WINAPI GdiReleaseLocalDC(HDC);

void Test_GdiReleaseLocalDC()
{
    ok(GdiReleaseLocalDC((HDC)-1) == TRUE, "\n");
    ok(GdiReleaseLocalDC((HDC)0) == TRUE, "\n");
    ok(GdiReleaseLocalDC((HDC)1) == TRUE, "\n");
    ok(GdiReleaseLocalDC((HDC)2) == TRUE, "\n");
}

START_TEST(GdiReleaseLocalDC)
{
    Test_GdiReleaseLocalDC();
}

