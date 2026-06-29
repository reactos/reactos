/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for NtGdiLineTo
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "../win32nt.h"
#include <gditools.h>

#define DIBINDEX(n) MAKELONG((n),0x10FF)

static HDC ghdcDDB1;

static
COLORREF
Do_LineTo(
    COLORREF crPenColor,
    HDC hdc,
    COLORREF crBack,
    COLORREF crText)
{
    SetDCPenColor(hdc, crPenColor);
    SetBkColor(hdc, crBack);
    SetTextColor(hdc, crText);
    NtGdiMoveTo(hdc, 0, 0, NULL);
    NtGdiLineTo(hdc, 4, 0);
    COLORREF cr = NtGdiGetPixel(hdc, 0, 0);
    return cr;
}

void Test_NtGdiLineTo_1BPP_BW(HDC hdc)
{
    HPEN hOldPen = SelectObject(hdc, GetStockObject(DC_PEN));

    // White bg color
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0xFFFFFF, 0x000000), 0x000000); // white bg, black fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0xFFFFFF, 0x000000), 0x000000); // white bg, black fg: dark => black
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0xFFFFFF, 0x000000), 0x000000); // white bg, black fg: bright => black
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0xFFFFFF, 0x000000), 0xFFFFFF); // white bg, black fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0xFFFFFF, 0x010101), 0x000000); // white bg, dark fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0xFFFFFF, 0x010101), 0x000000); // white bg, dark fg: fg => black
    ok_eq_hex(Do_LineTo(0x020202, hdc, 0xFFFFFF, 0x010101), 0x000000); // white bg, dark fg: dark => black
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0xFFFFFF, 0x010101), 0x000000); // white bg, dark fg: bright => black
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0xFFFFFF, 0x010101), 0xFFFFFF); // white bg, dark fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0xFFFFFF, 0xFEFEFE), 0x000000); // white bg, bright fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0xFFFFFF, 0xFEFEFE), 0x000000); // white bg, bright fg: dark => black
    ok_eq_hex(Do_LineTo(0xFDFDFD, hdc, 0xFFFFFF, 0xFEFEFE), 0x000000); // white bg, bright fg: bright => black
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0xFFFFFF, 0xFEFEFE), 0x000000); // white bg, bright fg: fg => black
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0xFFFFFF, 0xFEFEFE), 0xFFFFFF); // white bg, bright fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0xFFFFFF, 0xFFFFFF), 0x000000); // white bg, white fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0xFFFFFF, 0xFFFFFF), 0x000000); // white bg, white fg: dark => black
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0xFFFFFF, 0xFFFFFF), 0x000000); // white bg, white fg: bright => black
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0xFFFFFF, 0xFFFFFF), 0xFFFFFF); // white bg, white fg: white => white

    // bright bg color
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0xFEFEFE, 0x000000), 0x000000); // bright bg, black fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0xFEFEFE, 0x000000), 0x000000); // bright bg, black fg: dark => black
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0xFEFEFE, 0x000000), 0xFFFFFF); // bright bg, black fg: bg => white
    ok_eq_hex(Do_LineTo(0xFDFDFD, hdc, 0xFEFEFE, 0x000000), 0x000000); // bright bg, black fg: bright => black
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0xFEFEFE, 0x000000), 0xFFFFFF); // bright bg, black fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0xFEFEFE, 0x010101), 0x000000); // bright bg, dark fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0xFEFEFE, 0x010101), 0x000000); // bright bg, dark fg: fg => black
    ok_eq_hex(Do_LineTo(0x020202, hdc, 0xFEFEFE, 0x010101), 0x000000); // bright bg, dark fg: dark => black
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0xFEFEFE, 0x010101), 0xFFFFFF); // bright bg, dark fg: bg => white
    ok_eq_hex(Do_LineTo(0xFDFDFD, hdc, 0xFEFEFE, 0x010101), 0x000000); // bright bg, dark fg: bright => black
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0xFEFEFE, 0x010101), 0xFFFFFF); // bright bg, dark fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0xFEFEFE, 0xFDFDFD), 0x000000); // bright bg, bright fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0xFEFEFE, 0xFDFDFD), 0x000000); // bright bg, bright fg: dark => black
    ok_eq_hex(Do_LineTo(0xFDFDFD, hdc, 0xFEFEFE, 0xFDFDFD), 0x000000); // bright bg, bright fg: fg => black
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0xFEFEFE, 0xFDFDFD), 0xFFFFFF); // bright bg, bright fg: bg => white
    ok_eq_hex(Do_LineTo(0xFDFDFD, hdc, 0xFEFEFE, 0xFDFDFD), 0x000000); // bright bg, bright fg: dark => black
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0xFEFEFE, 0xFDFDFD), 0xFFFFFF); // bright bg, bright fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0xFEFEFE, 0xFFFFFF), 0x000000); // bright bg, white fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0xFEFEFE, 0xFFFFFF), 0x000000); // bright bg, white fg: dark => black
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0xFEFEFE, 0xFFFFFF), 0xFFFFFF); // bright bg, white fg: bg => white
    ok_eq_hex(Do_LineTo(0xFDFDFD, hdc, 0xFEFEFE, 0xFFFFFF), 0x000000); // bright bg, white fg: bright => black
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0xFEFEFE, 0xFFFFFF), 0xFFFFFF); // bright bg, white fg: white => white

    // dark bg color
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0x010101, 0x000000), 0x000000); // dark bg, black fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0x010101, 0x000000), 0x000000); // dark bg, black fg: bg => black
    ok_eq_hex(Do_LineTo(0x020202, hdc, 0x010101, 0x000000), 0xFFFFFF); // dark bg, black fg: dark => white
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0x010101, 0x000000), 0xFFFFFF); // dark bg, black fg: bright => white
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0x010101, 0x000000), 0xFFFFFF); // dark bg, black fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0x010101, 0x020202), 0x000000); // dark bg, dark fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0x010101, 0x020202), 0x000000); // dark bg, dark fg: bg => black
    ok_eq_hex(Do_LineTo(0x020202, hdc, 0x010101, 0x020202), 0xFFFFFF); // dark bg, dark fg: fg => white
    ok_eq_hex(Do_LineTo(0x030303, hdc, 0x010101, 0x020202), 0xFFFFFF); // dark bg, dark fg: dark => white
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0x010101, 0x020202), 0xFFFFFF); // dark bg, dark fg: bright => white
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0x010101, 0x020202), 0xFFFFFF); // dark bg, dark fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0x010101, 0xFEFEFE), 0x000000); // dark bg, bright fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0x010101, 0xFEFEFE), 0x000000); // dark bg, bright fg: bg => black
    ok_eq_hex(Do_LineTo(0x030303, hdc, 0x010101, 0xFEFEFE), 0xFFFFFF); // dark bg, bright fg: dark => white
    ok_eq_hex(Do_LineTo(0xFDFDFD, hdc, 0x010101, 0xFEFEFE), 0xFFFFFF); // dark bg, bright fg: bright => white
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0x010101, 0xFEFEFE), 0xFFFFFF); // dark bg, bright fg: fg => white
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0x010101, 0xFEFEFE), 0xFFFFFF); // dark bg, bright fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0x010101, 0xFFFFFF), 0x000000); // dark bg, white fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0x010101, 0xFFFFFF), 0x000000); // dark bg, white fg: bg => black
    ok_eq_hex(Do_LineTo(0x020202, hdc, 0x010101, 0xFFFFFF), 0xFFFFFF); // dark bg, white fg: dark => white
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0x010101, 0xFFFFFF), 0xFFFFFF); // dark bg, white fg: dark => white
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0x010101, 0xFFFFFF), 0xFFFFFF); // dark bg, white fg: white => white

    // black bg color
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0x000000, 0x000000), 0x000000); // black bg, black fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0x000000, 0x000000), 0xFFFFFF); // black bg, black fg: dark => white
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0x000000, 0x000000), 0xFFFFFF); // black bg, black fg: bright => white
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0x000000, 0x000000), 0xFFFFFF); // black bg, black fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0x000000, 0x020202), 0x000000); // black bg, dark fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0x000000, 0x020202), 0xFFFFFF); // black bg, dark fg: dark => white
    ok_eq_hex(Do_LineTo(0x020202, hdc, 0x000000, 0x020202), 0xFFFFFF); // black bg, dark fg: fg => white
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0x000000, 0x020202), 0xFFFFFF); // black bg, dark fg: bright => white
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0x000000, 0x020202), 0xFFFFFF); // black bg, dark fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0x000000, 0xFEFEFE), 0x000000); // black bg, bright fg: black => black
    ok_eq_hex(Do_LineTo(0x030303, hdc, 0x000000, 0xFEFEFE), 0xFFFFFF); // black bg, bright fg: dark => white
    ok_eq_hex(Do_LineTo(0xFDFDFD, hdc, 0x000000, 0xFEFEFE), 0xFFFFFF); // black bg, bright fg: bright => white
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0x000000, 0xFEFEFE), 0xFFFFFF); // black bg, bright fg: fg => white
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0x000000, 0xFEFEFE), 0xFFFFFF); // black bg, bright fg: white => white
    ok_eq_hex(Do_LineTo(0x000000, hdc, 0x000000, 0xFFFFFF), 0x000000); // black bg, white fg: black => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0x000000, 0xFFFFFF), 0xFFFFFF); // black bg, white fg: dark => white
    ok_eq_hex(Do_LineTo(0xFEFEFE, hdc, 0x000000, 0xFFFFFF), 0xFFFFFF); // black bg, white fg: bright => white
    ok_eq_hex(Do_LineTo(0xFFFFFF, hdc, 0x000000, 0xFFFFFF), 0xFFFFFF); // black bg, white fg: white => white

    // Test the bg brightness threshold behavior
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0x80807E, 0x000000), 0xFFFFFF); // just below threshold bg, black fg: grey => white
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0x80807F, 0x000000), 0x000000); // just above threshold bg, black fg: grey => black
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0xFF007F, 0x000000), 0xFFFFFF); // just below threshold bg, black fg: grey => white
    ok_eq_hex(Do_LineTo(0x010101, hdc, 0xFF0080, 0x000000), 0x000000); // just above threshold bg, black fg: grey => black

    SelectObject(hdc, hOldPen);
}

void Test_NtGdiLineTo_1BPP_RB(void)
{
    HPEN hOldPen = SelectObject(ghdcDIB1_RB, GetStockObject(DC_PEN));

    ok_eq_hex(Do_LineTo(0x000000, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0x0000FF); // white bg, black fg: black => red
    ok_eq_hex(Do_LineTo(0x0000FF, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0x0000FF); // white bg, black fg: red => red
    ok_eq_hex(Do_LineTo(0x5050FF, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0x0000FF); // white bg, black fg: gray-red => red
    ok_eq_hex(Do_LineTo(0xFF0000, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0xFF0000); // white bg, black fg: blue => blue
    ok_eq_hex(Do_LineTo(0xFE0000, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0x0000FF); // white bg, black fg: almost blue => red
    ok_eq_hex(Do_LineTo(0xFF5050, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0x0000FF); // white bg, black fg: gray-blue => red
    ok_eq_hex(Do_LineTo(0xFFFFFF, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0xFF0000); // white bg, black fg: white => blue

    ok_eq_hex(Do_LineTo(0x000000, ghdcDIB1_RB, 0x000000, 0xFFFFFF), 0xFF0000); // black bg, white fg: black => blue
    ok_eq_hex(Do_LineTo(0x0000FF, ghdcDIB1_RB, 0x000000, 0xFFFFFF), 0x0000FF); // black bg, white fg: red => red
    ok_eq_hex(Do_LineTo(0x5050FF, ghdcDIB1_RB, 0x000000, 0xFFFFFF), 0x0000FF); // black bg, white fg: gray-red => red
    ok_eq_hex(Do_LineTo(0xFF0000, ghdcDIB1_RB, 0x000000, 0xFFFFFF), 0xFF0000); // black bg, white fg: blue => blue
    ok_eq_hex(Do_LineTo(0xFE0000, ghdcDIB1_RB, 0x000000, 0xFFFFFF), 0x0000FF); // black bg, white fg: blue => red
    ok_eq_hex(Do_LineTo(0xFF5050, ghdcDIB1_RB, 0x000000, 0xFFFFFF), 0x0000FF); // black bg, white fg: gray-blue => red
    ok_eq_hex(Do_LineTo(0xFFFFFF, ghdcDIB1_RB, 0x000000, 0xFFFFFF), 0x0000FF); // black bg, white fg: white => red

    ok_eq_hex(Do_LineTo(0x000000, ghdcDIB1_RB, 0x2020FE, 0xFFFFFF), 0xFF0000); // reddish bg, white fg: black => blue
    ok_eq_hex(Do_LineTo(0x0000FF, ghdcDIB1_RB, 0x2020FE, 0xFFFFFF), 0x0000FF); // reddish bg, white fg: red => red
    ok_eq_hex(Do_LineTo(0x5050FF, ghdcDIB1_RB, 0x2020FE, 0xFFFFFF), 0xFF0000); // reddish bg, white fg: gray-red => blue
    ok_eq_hex(Do_LineTo(0xFF0000, ghdcDIB1_RB, 0x2020FE, 0xFFFFFF), 0xFF0000); // reddish bg, white fg: blue => blue
    ok_eq_hex(Do_LineTo(0xFE0000, ghdcDIB1_RB, 0x2020FE, 0xFFFFFF), 0xFF0000); // reddish bg, white fg: blue => blue
    ok_eq_hex(Do_LineTo(0xFF5050, ghdcDIB1_RB, 0x2020FE, 0xFFFFFF), 0xFF0000); // reddish bg, white fg: gray-blue => blue
    ok_eq_hex(Do_LineTo(0xFFFFFF, ghdcDIB1_RB, 0x2020FE, 0xFFFFFF), 0xFF0000); // reddish bg, white fg: white => blue
    ok_eq_hex(Do_LineTo(0x2020FE, ghdcDIB1_RB, 0x2020FE, 0xFFFFFF), 0x0000FF); // reddish bg, white fg: bg => red

    ok_eq_hex(Do_LineTo(0x000000, ghdcDIB1_RB, 0xFE2010, 0xFFFFFF), 0x0000FF); // blueish bg, white fg: black => red
    ok_eq_hex(Do_LineTo(0x0000FF, ghdcDIB1_RB, 0xFE2010, 0xFFFFFF), 0x0000FF); // blueish bg, white fg: red => red
    ok_eq_hex(Do_LineTo(0x5050FF, ghdcDIB1_RB, 0xFE2010, 0xFFFFFF), 0x0000FF); // blueish bg, white fg: gray-red => red
    ok_eq_hex(Do_LineTo(0xFF0000, ghdcDIB1_RB, 0xFE2010, 0xFFFFFF), 0xFF0000); // blueish bg, white fg: blue => blue
    ok_eq_hex(Do_LineTo(0xFE0000, ghdcDIB1_RB, 0xFE2010, 0xFFFFFF), 0x0000FF); // blueish bg, white fg: blue => red
    ok_eq_hex(Do_LineTo(0xFF5050, ghdcDIB1_RB, 0xFE2010, 0xFFFFFF), 0x0000FF); // blueish bg, white fg: gray-blue => red
    ok_eq_hex(Do_LineTo(0xFFFFFF, ghdcDIB1_RB, 0xFE2010, 0xFFFFFF), 0x0000FF); // blueish bg, white fg: white => red
    ok_eq_hex(Do_LineTo(0xFE2010, ghdcDIB1_RB, 0xFE2010, 0xFFFFFF), 0xFF0000); // blueish bg, white fg: bg => blue

    // Test the bg color threshold behavior
    ok_eq_hex(Do_LineTo(0x010101, ghdcDIB1_RB, 0x002001, 0x000000), 0xFF0000); // closer to red: grey => blue
    ok_eq_hex(Do_LineTo(0x010101, ghdcDIB1_RB, 0x808081, 0x000000), 0xFF0000); // closer to red: grey => blue
    ok_eq_hex(Do_LineTo(0x010101, ghdcDIB1_RB, 0xFE80FF, 0x000000), 0xFF0000); // closer to red: grey => blue
    ok_eq_hex(Do_LineTo(0x010101, ghdcDIB1_RB, 0x000000, 0x000000), 0x0000FF); // middle: grey => red
    ok_eq_hex(Do_LineTo(0x010101, ghdcDIB1_RB, 0x808080, 0x000000), 0x0000FF); // middle: grey => red
    ok_eq_hex(Do_LineTo(0x010101, ghdcDIB1_RB, 0xFFFFFF, 0x000000), 0x0000FF); // middle: grey => red
    ok_eq_hex(Do_LineTo(0x010101, ghdcDIB1_RB, 0x013000, 0x000000), 0x0000FF); // closer to blue: grey => red
    ok_eq_hex(Do_LineTo(0x010101, ghdcDIB1_RB, 0x81A080, 0x000000), 0x0000FF); // closer to blue: grey => red
    ok_eq_hex(Do_LineTo(0x010101, ghdcDIB1_RB, 0xFFFFFE, 0x000000), 0x0000FF); // closer to blue: grey => red

    SelectObject(ghdcDIB1_RB, hOldPen);
}

START_TEST(NtGdiLineTo)
{
    ok(GdiToolsInit(), "GdiToolsInit failed\n");

    DEVMODEW dmOld;
    ChangeScreenBpp(32, &dmOld);

    ghdcDDB1 = CreateCompatibleDC(NULL);
    ok(SelectObject(ghdcDDB1, ghbmp1) != NULL, "SelectObject failed\n");

    Test_NtGdiLineTo_1BPP_BW(ghdcDDB1);
    Test_NtGdiLineTo_1BPP_BW(ghdcDIB1);
    Test_NtGdiLineTo_1BPP_BW(ghdcDIB1_InvCol);
    Test_NtGdiLineTo_1BPP_RB();

    ChangeDisplaySettingsW(&dmOld, 0);
}
