/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for RealizePalette
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

#include <gditools.h>

START_TEST(RealizePalette)
{
    ok(GdiToolsInit(), "GdiToolsInit failed\n");

    ok_int(RealizePalette(NULL), GDI_ERROR);
    ok_int(RealizePalette((HDC)UlongToHandle(0xdeadc0de)), GDI_ERROR);
    ok_int(RealizePalette((HDC)UlongToHandle(0x00010001)), 0);
    ok_int(RealizePalette(ghdcDIB32), 0);
}
