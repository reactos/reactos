/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GdiSetAttrs
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

BOOL WINAPI GdiSetAttrs(HDC);

void Test_GdiSetAttrs()
{
    ok(GdiSetAttrs((HDC)-1) == TRUE, "\n");
    ok(GdiSetAttrs((HDC)0) == TRUE, "\n");
    ok(GdiSetAttrs((HDC)1) == TRUE, "\n");
    ok(GdiSetAttrs((HDC)2) == TRUE, "\n");
}

START_TEST(GdiSetAttrs)
{
    Test_GdiSetAttrs();
}

