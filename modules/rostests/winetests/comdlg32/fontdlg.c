/*
 * Unit test suite for comdlg32 API functions: font dialogs
 *
 * Copyright 2009 Vincent Povirk for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winspool.h"
#include "winuser.h"
#include "objbase.h"

#include "commdlg.h"

#include "wine/test.h"

static int get_dpiy(void)
{
    HDC hdc;
    int result;

    hdc = GetDC(0);
    result = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);

    return result;
}

static HDC get_printer_ic(void)
{
    PRINTER_INFO_2A *info;
    DWORD info_size, num_printers=0;
    BOOL ret;
    HDC result=NULL;

    EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &info_size, &num_printers);

    if (info_size == 0)
        return NULL;

    info = malloc(info_size);

    ret = EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 2, (LPBYTE)info, info_size, &info_size, &num_printers);

    if (ret)
        result = CreateICA(info->pDriverName, info->pPrinterName, NULL, NULL);

    free(info);

    return result;
}

static UINT_PTR CALLBACK CFHookProcOK(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        PostMessageA(hdlg, WM_COMMAND, IDOK, FALSE);
        return 0;
    default:
        return 0;
    }
}

static void test_ChooseFontA(void)
{
    LOGFONTA lfa;
    CHOOSEFONTA cfa;
    BOOL ret;
    int dpiy = get_dpiy();
    int expected_pointsize, expected_lfheight;
    HDC printer_ic;

    memset(&lfa, 0, sizeof(LOGFONTA));
    lfa.lfHeight = -16;
    lfa.lfWeight = FW_NORMAL;
    strcpy(lfa.lfFaceName, "Symbol");

    memset(&cfa, 0, sizeof(CHOOSEFONTA));
    cfa.lStructSize = sizeof(cfa);
    cfa.lpLogFont = &lfa;
    cfa.Flags = CF_ENABLEHOOK|CF_INITTOLOGFONTSTRUCT|CF_SCREENFONTS;
    cfa.lpfnHook = CFHookProcOK;

    ret = ChooseFontA(&cfa);

    expected_pointsize = MulDiv(16, 72, dpiy) * 10;
    expected_lfheight = -MulDiv(expected_pointsize, dpiy, 720);

    ok(ret == TRUE, "ChooseFontA returned FALSE\n");
    ok(cfa.iPointSize == expected_pointsize, "Expected %i, got %i\n", expected_pointsize, cfa.iPointSize);
    ok(lfa.lfHeight == expected_lfheight, "Expected %i, got %li\n", expected_lfheight, lfa.lfHeight);
    ok(lfa.lfWeight == FW_NORMAL, "Expected FW_NORMAL, got %li\n", lfa.lfWeight);
    ok(lfa.lfCharSet == SYMBOL_CHARSET, "Expected SYMBOL_CHARSET, got %i\n", lfa.lfCharSet);
    ok(strcmp(lfa.lfFaceName, "Symbol") == 0, "Expected Symbol, got %s\n", lfa.lfFaceName);

    cfa.Flags = CF_ENABLEHOOK|CF_INITTOLOGFONTSTRUCT|CF_SCREENFONTS|CF_NOSCRIPTSEL;
    ret = ChooseFontA(&cfa);
    ok(ret == TRUE, "ChooseFontA returned FALSE\n");
    ok(lfa.lfCharSet == DEFAULT_CHARSET, "Expected DEFAULT_CHARSET, got %i\n", lfa.lfCharSet);

    printer_ic = get_printer_ic();
    if (!printer_ic)
        skip("can't get a DC for a local printer\n");
    else
    {
        memset(&lfa, 0, sizeof(LOGFONTA));
        lfa.lfHeight = -16;
        lfa.lfWeight = FW_NORMAL;
        strcpy(lfa.lfFaceName, "Symbol");

        memset(&cfa, 0, sizeof(CHOOSEFONTA));
        cfa.lStructSize = sizeof(cfa);
        cfa.lpLogFont = &lfa;
        cfa.Flags = CF_ENABLEHOOK|CF_INITTOLOGFONTSTRUCT|CF_PRINTERFONTS;
        cfa.hDC = printer_ic;
        cfa.lpfnHook = CFHookProcOK;

        ret = ChooseFontA(&cfa);

        expected_pointsize = MulDiv(16, 72, dpiy) * 10;
        expected_lfheight = -MulDiv(expected_pointsize, dpiy, 720);

        ok(ret == TRUE, "ChooseFontA returned FALSE\n");
        ok(cfa.iPointSize == expected_pointsize, "Expected %i, got %i\n", expected_pointsize, cfa.iPointSize);
        ok(lfa.lfHeight == expected_lfheight, "Expected %i, got %li\n", expected_lfheight, lfa.lfHeight);
        ok(lfa.lfWeight == FW_NORMAL, "Expected FW_NORMAL, got %li\n", lfa.lfWeight);
        ok((strcmp(lfa.lfFaceName, "Symbol") == 0) ||
            broken(*lfa.lfFaceName == 0), "Expected Symbol, got %s\n", lfa.lfFaceName);

        DeleteDC(printer_ic);
    }
}

START_TEST(fontdlg)
{
    test_ChooseFontA();
}
