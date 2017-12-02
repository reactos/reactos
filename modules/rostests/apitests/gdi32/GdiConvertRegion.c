/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GdiConvertRegion
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

HRGN WINAPI GdiConvertRegion(HRGN);

void Test_GdiConvertRegion()
{
    ok(GdiConvertRegion((HRGN)-1) == (HRGN)-1, "\n");
    ok(GdiConvertRegion((HRGN)0) == (HRGN)0, "\n");
    ok(GdiConvertRegion((HRGN)1) == (HRGN)1, "\n");
    ok(GdiConvertRegion((HRGN)2) == (HRGN)2, "\n");
}

START_TEST(GdiConvertRegion)
{
    Test_GdiConvertRegion();
}

