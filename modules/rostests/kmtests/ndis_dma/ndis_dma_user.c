/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Load the NDIS DMA test driver
 * COPYRIGHT:   Copyright 2026 ReactOS Lab
 */

#include <kmt_test.h>

START_TEST(NdisDma)
{
    DWORD Error;

    Error = KmtLoadDriver(L"ndis_dma", FALSE);
    ok_eq_int(Error, ERROR_SUCCESS);
    if (Error)
        return;

    KmtUnloadDriver();
}
