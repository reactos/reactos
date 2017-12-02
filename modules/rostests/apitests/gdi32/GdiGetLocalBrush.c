/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GdiGetLocalBrush
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

HBRUSH WINAPI GdiGetLocalBrush(HBRUSH hbr);

void Test_GdiGetLocalBrush()
{
    ok(GdiGetLocalBrush((HBRUSH)-1) == (HBRUSH)-1, "\n");
    ok(GdiGetLocalBrush((HBRUSH)0) == (HBRUSH)0, "\n");
    ok(GdiGetLocalBrush((HBRUSH)1) == (HBRUSH)1, "\n");
    ok(GdiGetLocalBrush((HBRUSH)2) == (HBRUSH)2, "\n");
}

START_TEST(GdiGetLocalBrush)
{
    Test_GdiGetLocalBrush();
}

