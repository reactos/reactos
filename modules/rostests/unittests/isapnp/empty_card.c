/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Dummy card resource tests for the ISA PnP bus driver
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

/* GLOBALS ********************************************************************/

static UCHAR DrvpTestPnpRom[] =
{
    0x49, 0xF3,             // Vendor ID 0xF349 'ROS'
    0x55, 0x66,             // Product ID 0x5566
    0xFF, 0xFF, 0xFF, 0xFF, // Serial Number
    0xFF,                   // Checksum (dummy)

    0x0A, 0x10, 0x10, // PnP version 1.0, vendor version 1.0

    0x82, 6, 0x00, // ANSI identifier 'Test 2'
    'T', 'e', 's', 't', ' ', '2',

    /* ********************* DEVICE 1 ********************* */

    0x15,       // Logical device ID
    0x24, 0x08, // Vendor ID 0x0824 'BAD'
    0x30, 0x00, // Product ID 0x3000
    0x00,

    0x82, 0xCC, 0xCC, // Long ANSI identifier to verify resource data bounds checking

    /* **************************************************** */

    0x79, // END
    0xFF, // Checksum (dummy)
};

/* FUNCTIONS ******************************************************************/

VOID
DrvCreateCard2(
    _In_ PISAPNP_CARD Card)
{
    PISAPNP_CARD_LOGICAL_DEVICE LogDev;

    IsaBusCreateCard(Card, DrvpTestPnpRom, sizeof(DrvpTestPnpRom), 1);

    /* ********************* DEVICE 1 ********************* */
    LogDev = &Card->LogDev[0];

    /* Enable decodes */
    LogDev->Registers[0x30] = 0x01;

    /* No DMA is active */
    LogDev->Registers[0x74] = 0x04;
    LogDev->Registers[0x75] = 0x04;
}
