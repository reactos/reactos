/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for RealizePalette
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

#include "init.h"

START_TEST(RealizePalette)
{
    InitStuff();
    ok_int(RealizePalette(NULL), GDI_ERROR);
    ok_int(RealizePalette((HDC)0xdeadc0de), GDI_ERROR);
    ok_int(RealizePalette((HDC)0x00010001), 0);
    ok_int(RealizePalette(ghdcDIB32), 0);

}
