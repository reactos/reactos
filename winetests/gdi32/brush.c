/*
 * Unit test suite for brushes
 *
 * Copyright 2004 Kevin Koltzau
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
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "wine/test.h"

typedef struct _STOCK_BRUSH {
    COLORREF color;
    int stockobj;
    const char *name;
} STOCK_BRUSH;

static void test_solidbrush(void)
{
    static const STOCK_BRUSH stock[] = {
        {RGB(255,255,255), WHITE_BRUSH, "white"},
        {RGB(192,192,192), LTGRAY_BRUSH, "ltgray"},
        {RGB(128,128,128), GRAY_BRUSH, "gray"},
        {RGB(0,0,0), BLACK_BRUSH, "black"},
        {RGB(0,0,255), -1, "blue"}
    };
    HBRUSH solidBrush;
    HBRUSH stockBrush;
    LOGBRUSH br;
    size_t i;
    INT ret;

    for(i=0; i<sizeof(stock)/sizeof(stock[0]); i++) {
        solidBrush = CreateSolidBrush(stock[i].color);
        
        if(stock[i].stockobj != -1) {
            stockBrush = GetStockObject(stock[i].stockobj);
            ok(stockBrush!=solidBrush ||
               broken(stockBrush==solidBrush), /* win9x does return stock object */
               "Stock %s brush equals solid %s brush\n", stock[i].name, stock[i].name);
        }
        else
            stockBrush = NULL;
        memset(&br, 0, sizeof(br));
        ret = GetObject(solidBrush, sizeof(br), &br);
        ok( ret !=0, "GetObject on solid %s brush failed, error=%d\n", stock[i].name, GetLastError());
        ok(br.lbStyle==BS_SOLID, "%s brush has wrong style, got %d expected %d\n", stock[i].name, br.lbStyle, BS_SOLID);
        ok(br.lbColor==stock[i].color, "%s brush has wrong color, got 0x%08x expected 0x%08x\n", stock[i].name, br.lbColor, stock[i].color);
        
        if(stockBrush) {
            /* Sanity check, make sure the colors being compared do in fact have a stock brush */
            ret = GetObject(stockBrush, sizeof(br), &br);
            ok( ret !=0, "GetObject on stock %s brush failed, error=%d\n", stock[i].name, GetLastError());
            ok(br.lbColor==stock[i].color, "stock %s brush unexpected color, got 0x%08x expected 0x%08x\n", stock[i].name, br.lbColor, stock[i].color);
        }

        DeleteObject(solidBrush);
        ret = GetObject(solidBrush, sizeof(br), &br);
        ok(ret==0 ||
           broken(ret!=0), /* win9x */
           "GetObject succeeded on a deleted %s brush\n", stock[i].name);
    }
}
 
START_TEST(brush)
{
    test_solidbrush();
}
