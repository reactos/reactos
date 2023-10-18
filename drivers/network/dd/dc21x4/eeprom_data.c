/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     EEPROM data for boards without the standard SROM Format
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* Adapted from the Linux tulip driver written by Donald Becker */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

/* GLOBALS ********************************************************************/

/* Asante */
static DC_PG_DATA UCHAR SRompLeafAsante[] =
{
    0x00, 0x00, 0x94,

    0x00, 0x08, // Default Autoselect
    0x00,       // GPIO direction
    0x01,       // 1 block

    0x80 + 12,  // Extended block, 12 bytes
    0x01,       // MII
    0x00,       // PHY #0
    0x00,       // GPIO stream length
    0x00,       // Reset stream length
    0x00, 0x78, // Capabilities
    0xE0, 0x01, // Advertisement
    0x00, 0x50, // FDX
    0x00, 0x18, // TTM
};

/* SMC 9332DST */
static DC_PG_DATA UCHAR SRompLeaf9332[] =
{
    0x00, 0x00, 0xC0,

    0x00, 0x08, // Default Autoselect
    0x1F,       // GPIO direction
    0x04,       // 4 blocks

    0x00,       // GPR 0
    0x00,       // GPIO data
    0x9E, 0x00, // Command 0x009E

    0x04,       // GPR 4
    0x00,       // GPIO data
    0x9E, 0x00, // Command 0x009E
    0x03,       // GPR 3
    0x09,       // GPIO data
    0x6D, 0x00, // Command 0x006D

    0x05,       // GPR 5
    0x09,       // GPIO data
    0x6D, 0x00, // Command 0x006D
};

/* Cogent EM100 */
static DC_PG_DATA UCHAR SRompLeafEm100[] =
{
    0x00, 0x00, 0x92,

    0x00, 0x08, // Default Autoselect
    0x3F,       // GPIO direction
    0x06,       // 6 blocks

    0x07,       // GPR 7
    0x01,       // GPIO data
    0x21, 0x80, // Command 0x8021

    0x08,       // GPR 8
    0x01,       // GPIO data
    0x21, 0x80, // Command 0x8021

    0x00,       // GPR 0
    0x01,       // GPIO data
    0x9E, 0x00, // Command 0x009E

    0x04,       // GPR 4
    0x01,       // GPIO data
    0x9E, 0x00, // Command 0x009E

    0x03,       // GPR 3
    0x01,       // GPIO data
    0x6D, 0x00, // Command 0x006D

    0x05,       // GPR 5
    0x01,       // GPIO data
    0x6D, 0x00, // Command 0x006D
};

/* Maxtech NX-110 */
static DC_PG_DATA UCHAR SRompLeafNx110[] =
{
    0x00, 0x00, 0xE8,

    0x00, 0x08, // Default Autoselect
    0x13,       // GPIO direction
    0x05,       // 5 blocks

    0x01,       // GPR 1
    0x10,       // GPIO data
    0x9E, 0x00, // Command 0x009E

    0x00,       // GPR 0
    0x00,       // GPIO data
    0x9E, 0x00, // Command 0x009E

    0x04,       // GPR 4
    0x00,       // GPIO data
    0x9E, 0x00, // Command 0x009E

    0x03,       // GPR 3
    0x03,       // GPIO data
    0x6D, 0x00, // Command 0x006D

    0x05,       // GPR 5
    0x03,       // GPIO data
    0x6D, 0x00, // Command 0x006D
};

/* Accton EN1207 */
static DC_PG_DATA UCHAR SRompLeafEn1207[] =
{
    0x00, 0x00, 0xE8,

    0x00, 0x08, // Default Autoselect
    0x1F,       // GPIO direction
    0x05,       // 5 blocks

    0x01,       // GPR 1
    0x1B,       // GPIO data
    0x00, 0x00, // Command 0x0000

    0x00,       // GPR 0
    0x0B,       // GPIO data
    0x9E, 0x00, // Command 0x009E

    0x04,       // GPR 4
    0x0B,       // GPIO data
    0x9E, 0x00, // Command 0x009E

    0x03,       // GPR 3
    0x1B,       // GPIO data
    0x6D, 0x00, // Command 0x006D

    0x05,       // GPR 5
    0x1B,       // GPIO data
    0x6D, 0x00, // Command 0x006D
};

/* NetWinder */
static DC_PG_DATA UCHAR SRompLeafNetWinder[] =
{
    0x00, 0x10, 0x57,

    0x00, 0x08, // Default Autoselect
    0x01,       // 1 block

    0x80 + 21,  // Extended block, 21 bytes
    0x03,       // MII
    0x01,       // PHY #1
    0x00,       // GPIO stream length
    0x03,       // Reset stream length
    0x21, 0x08,
    0x00, 0x00,
    0x01, 0x00,
    0x00, 0x00, // Capabilities
    0xE1, 0x01, // Advertisement
    0x00, 0x00, // FDX
    0x00, 0x00, // TTM
    0x00,       // PHY cannot be unplugged
};

/* Cobalt Microserver */
static DC_PG_DATA UCHAR SRompLeafCobaltMicroserver[] =
{
    0x00, 0x10, 0xE0,

    0x00, 0x08, // Default Autoselect
    0x01,       // 1 block

    0x80 + 21,  // Extended block, 21 bytes
    0x03,       // MII
    0x00,       // PHY #0
    0x00,       // GPIO stream length
    0x04,       // Reset stream length
    0x01, 0x08, // Set control mode, GP0 output
    0x00, 0x00, // Drive GP0 Low (RST is active low)
    0x00, 0x08, // Control mode, GP0 input (undriven)
    0x00, 0x00, // Clear control mode
    0x00, 0x78, // Capabilities: 100TX FDX + HDX, 10bT FDX + HDX
    0xE0, 0x01, // Advertise all above
    0x00, 0x50, // FDX all above
    0x00, 0x18, // Set fast TTM in 100bt modes
    0x00,       // PHY cannot be unplugged
};

#if DBG
#define DEFINE_BOARD(Leaf, Name) { Name, Leaf, sizeof(Leaf) - 3 /* OUI (3 bytes) */}
#else
#define DEFINE_BOARD(Leaf, Name) { Leaf, sizeof(Leaf) - 3 }
#endif

DC_PG_DATA
DC_SROM_REPAIR_ENTRY SRompRepairData[] =
{
    DEFINE_BOARD(SRompLeafAsante, "Asante"),
    DEFINE_BOARD(SRompLeaf9332, "SMC 9332DST"),
    DEFINE_BOARD(SRompLeafEm100, "Cogent EM100"),
    DEFINE_BOARD(SRompLeafNx110, "Maxtech NX-110"),
    DEFINE_BOARD(SRompLeafEn1207, "Accton EN1207"), // Must be defined after the NX-110
    DEFINE_BOARD(SRompLeafNetWinder, "NetWinder"),
    DEFINE_BOARD(SRompLeafCobaltMicroserver, "Cobalt Microserver"),
    DEFINE_BOARD(NULL, NULL),
};
