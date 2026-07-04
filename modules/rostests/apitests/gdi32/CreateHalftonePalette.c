/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for CreateHalftonePalette
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include <apitest.h>
#include <wingdi.h>

START_TEST(CreateHalftonePalette)
{
    HPALETTE hPal;
    UINT entries;
    BOOL ret;
    PALETTEENTRY pe[256];
    PALETTEENTRY sysPal[20];

    hPal = CreateHalftonePalette(NULL);
    ok(hPal != NULL, "CreateHalftonePalette(NULL) failed, LastError=%lu\n", GetLastError());
    if (!hPal)
    {
        skip("CreateHalftonePalette(NULL) failed, skipping test\n");
        return;
    }

    /* Validate the palette */
    entries = GetPaletteEntries(hPal, 0, _countof(pe), pe);
    ok(entries == _countof(pe), "Unexpected number of palette entries: %u\n", entries);
    /* Get system default palette */
    entries = GetPaletteEntries(GetStockObject(DEFAULT_PALETTE), 0, 20, sysPal);
    ok(entries == _countof(sysPal), "Unexpected number of system palette entries: %u\n", entries);
    /* First and last ten entries are default ones */
    for (int i = 0; i < 10; i++)
    {
        ok(pe[i].peRed == sysPal[i].peRed && 
           pe[i].peGreen == sysPal[i].peGreen &&
           pe[i].peBlue == sysPal[i].peBlue, "Unexpected color at index %d\n", i);

        ok(pe[246 + i].peRed == sysPal[10 + i].peRed && 
           pe[246 + i].peGreen == sysPal[10 + i].peGreen &&
           pe[246 + i].peBlue == sysPal[10 + i].peBlue, "Unexpected color at index %d\n", 246 + i);
    }

    ret = DeleteObject(hPal);
    ok(ret != FALSE, "DeleteObject(hPal) failed\n");
}
