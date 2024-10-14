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
    0x12, 0x34,             // Product ID 0x1234
    0xFF, 0xFF, 0xFF, 0xFF, // Serial Number
    0xFF,                   // Checksum (dummy)

    0x0A, 0x10, 0x10, // PnP version 1.0, vendor version 1.0

    0x82, 6, 0x00, // ANSI identifier 'Test 1'
    'T', 'e', 's', 't', ' ', '1',

    /* ********************* DEVICE 1 ********************* */

    0x15,       // Logical device ID
    0xC9, 0xF3, // Vendor ID 0xF3C9 'ROS'
    0x12, 0x34, // Product ID 0x1234
    0x00,

    0x82, 12, 0x00, // ANSI identifier 'Test 1 Dev 1'
    'T', 'e', 's', 't', ' ', '1', ' ',
    'D', 'e', 'v', ' ', '1',

    // (A) Fixed
    // (B) Dependent
    // (C) Fixed
    // (D) End

    0x47, 0x01, 0x30, 0x03, 0x40, 0x03, 0x04, 0x02, // I/O Base 16-bit 0x330-0x340, len 2, align 4

    0x30, // Start dependent Function

    0x47, 0x00, 0x00, 0x06, 0x80, 0x07, 0x01, 0x08, // I/O Base 10-bit 0x600-0x780, len 8, align 1
    0x4B, 0x16, 0xFC, 0x0C, // Fixed I/O 0x16, length 12 (NOTE: We fill byte 2 with garbage)

    0x22, 0x20, 0x00,       // IRQ 5 positive edge triggered
    0x23, 0x1C, 0x00, 0x08, // IRQ 2, 3, 4 active low, level-sensitive

    0x31, 0x02, // Start dependent Function

    0x81, 0x09, 0x00, // Memory Range
    0x1B, // Writeable, read-cacheable, write-through, supports range length, 32-bit memory only
    0x80, 0x0C, // Range minimum 0xC8000
    0xC0, 0x0D, // Range maximum 0xDC000
    0x40, 0xFF, // Base alignment 0xFF40
    0x40, 0x00, // Range length 0x4000

    0x81, 0x09, 0x00, // Memory Range
    0x66, // Non-writeable, read-cacheable, write-through,
          // supports high address, 8-bit, shadowable, expansion ROM
    0x40, 0x0D, // Range minimum 0xD4000
    0x80, 0x0D, // Range maximum 0xD8000
    0x00, 0x00, // Base alignment 0x10000 (NOTE: Special case)
    0x40, 0x00, // Range length 0x4000

    0x2A, 0x03, 0x04, // DMA 0 or 1. 8-bit only, ISA Master, not use count by byte/word, ISA compat

    0x38, // End dependent Function

    0x2A, 0x04, 0x41, // DMA 3. 8- and 16-bit, not use count by byte/word, Type B

    /* ********************* DEVICE 2 ********************* */

    0x15,       // Logical device ID
    0xC9, 0xF3, // Vendor ID 0xF3C9 'ROS'
    0x12, 0x35, // Product ID 0x1235
    0x00,

    // (A) Fixed
    // (B) Dependent
    // (C) End

    0x47, 0x01, 0x00, 0x05, 0x06, 0x05, 0x01, 0x01, // I/O Base 16-bit 0x500-0x506, len 1, align 1

    0x30, // Start dependent Function

    0x47, 0x01, 0x00, 0x06, 0x07, 0x06, 0x01, 0x01, // I/O Base 16-bit 0x600-0x607, len 1, align 1

    0x30, // Start dependent Function

    0x47, 0x01, 0x00, 0x07, 0x08, 0x07, 0x01, 0x01, // I/O Base 16-bit 0x700-0x708, len 1, align 1

    0x38, // End dependent Function

    /* ********************* DEVICE 3 ********************* */

    0x15,       // Logical device ID
    0xC9, 0xF3, // Vendor ID 0xF3C9 'ROS'
    0x12, 0x36, // Product ID 0x1236
    0x00,

    // (A) Dependent
    // (B) Fixed
    // (C) End

    0x30, // Start dependent Function

    0x47, 0x01, 0x00, 0x06, 0x07, 0x06, 0x01, 0x01, // I/O Base 16-bit 0x600-0x607, len 1, align 1

    0x30, // Start dependent Function

    0x47, 0x01, 0x00, 0x07, 0x08, 0x07, 0x01, 0x01, // I/O Base 16-bit 0x700-0x708, len 1, align 1

    0x38, // End dependent Function

    0x47, 0x01, 0x00, 0x05, 0x06, 0x05, 0x01, 0x01, // I/O Base 16-bit 0x500-0x506, len 1, align 1

    /* ********************* DEVICE 4 ********************* */

    0x15,       // Logical device ID
    0xC9, 0xF3, // Vendor ID 0xF3C9 'ROS'
    0x12, 0x36, // Product ID 0x1236
    0x00,

    // (A) Dependent
    // (B) End

    0x30, // Start dependent Function

    0x47, 0x01, 0x00, 0x06, 0x07, 0x06, 0x01, 0x01, // I/O Base 16-bit 0x600-0x607, len 1, align 1

    0x30, // Start dependent Function

    0x47, 0x01, 0x00, 0x07, 0x08, 0x07, 0x01, 0x01, // I/O Base 16-bit 0x700-0x708, len 1, align 1

    0x38, // End dependent Function

    /* ********************* DEVICE 5 ********************* */

    // We cannot mix 24- and 32-bit memory descriptors, so create a separate logical device

    0x15,       // Logical device ID
    0xC9, 0xF3, // Vendor ID 0xF3C9 'ROS'
    0xAB, 0xCD, // Product ID 0xABCD
    0x00,

    0x1C,       // Compatible device ID
    0xAD, 0x34, // Vendor ID 0x34AD 'MEM'
    0x56, 0x78, // Product ID 0x5678

    0x82, 12, 0x00, // ANSI identifier 'Test 1 Dev 2'
    'T', 'e', 's', 't', ' ', '1', ' ',
    'D', 'e', 'v', ' ', '2',

    // (A) Fixed
    // (B) End

    0x85, 0x11, 0x00, // 32-bit Memory Range
    0x66, // Non-writeable, read-cacheable, write-through,
          // supports high address, 8-bit, shadowable, expansion ROM
    0x00, 0x00, 0x0D, 0x00, // Range minimum 0xD0000
    0x00, 0x00, 0x0E, 0x00, // Range maximum 0xE0000
    0x00, 0x01, 0x00, 0x00, // Base alignment 0x100
    0x00, 0x80, 0x00, 0x00, // Range length 0x8000

    0x86, 0x09, 0x00, // 32-bit Fixed Memory Range
    0x66, // Non-writeable, read-cacheable, write-through,
          // supports high address, 8-bit, shadowable, expansion ROM
    0x00, 0x80, 0x0C, 0x00, // Range base address 0xC8000
    0x00, 0x80, 0x00, 0x00, // Length 0x008000

    /* ********************* DEVICE 6 ********************* */

    0x15,       // Logical device ID
    0xC9, 0xF3, // Vendor ID 0xF3C9 'ROS'
    0xA0, 0x01, // Product ID 0xA001
    0x00,

    // NOTE: We don't supply any ANSI identifiers here

    0x81, 0x09, 0x00, // Memory Range
    0x6E, // Non-writeable, read-cacheable, write-through,
          // supports high address, 16-bit, shadowable, expansion ROM
    0x00, 0x0A, // Range minimum 0xA0000
    0x40, 0x0A, // Range maximum 0xA4000
    0x04, 0x00, // Base alignment 4
    0x10, 0x00, // Range length 0x1000

    0x81, 0x09, 0x00, // Memory Range
    0x66, // Non-writeable, read-cacheable, write-through,
          // supports high address, 8-bit, shadowable, expansion ROM
    0x00, 0x08, // Range minimum 0x8000
    0x00, 0x09, // Range maximum 0x9000
    0x04, 0x00, // Base alignment 4
    0x01, 0x00, // Range length 0x100

    0x2A, 0x40, 0x04, // DMA 6

    0x22, 0x20, 0x00, // IRQ 5 positive edge triggered

    0x4B, 0x80, 0x00, 0x08, // Fixed I/O 0x80, length 8

    /* ********************* DEVICE 7 ********************* */

    0x15,       // Logical device ID
    0xB0, 0x15, // Vendor ID 0x15B0 'EMP'
    0x20, 0x00, // Product ID 0x2000
    0x00,

    // No resource requirements for this device

    0x73, '1', '2', '3', // Vendor defined valid tag

    /* **************************************************** */

    0x79, // END
    0xFF, // Checksum (dummy)
};

/* FUNCTIONS ******************************************************************/

VOID
DrvCreateCard1(
    _In_ PISAPNP_CARD Card)
{
    PISAPNP_CARD_LOGICAL_DEVICE LogDev;

    IsaBusCreateCard(Card, DrvpTestPnpRom, sizeof(DrvpTestPnpRom), 7);

    /* NOTE: Boot resources of the devices should be made from the requirements */

    /* ********************* DEVICE 1 ********************* */
    LogDev = &Card->LogDev[0];

    /*
     * Assign some I/O base but don't enable decodes.
     * The driver will ignore such I/O configuration.
     */
    LogDev->Registers[0x60] = 0x03;
    LogDev->Registers[0x61] = 0x90;

    /* ******************* DEVICE 2, 3, 4 ***************** */
    LogDev++;
    LogDev++;
    LogDev++;

    /* ********************* DEVICE 5 ********************* */
    LogDev++;

    /* Enable decodes */
    LogDev->Registers[0x30] = 0x01;

    /* No DMA is active */
    LogDev->Registers[0x74] = 0x04;
    LogDev->Registers[0x75] = 0x04;

    /* Memory 32 Base #0 0xD6000 */
    LogDev->Registers[0x76] = 0x00;
    LogDev->Registers[0x77] = 0x0D;
    LogDev->Registers[0x78] = 0x60;
    LogDev->Registers[0x79] = 0x00;
    /* Memory 32 Control #0 - enable range length, 8-bit memory */
    LogDev->Registers[0x7A] = 0x00;
    /* Memory 32 Range length #0 0xFFFF8000 (32kB) */
    LogDev->Registers[0x7B] = 0xFF;
    LogDev->Registers[0x7C] = 0xFF;
    LogDev->Registers[0x7D] = 0x80;
    LogDev->Registers[0x7E] = 0x00;

    /* Memory 32 Base #1 0xC8000 */
    LogDev->Registers[0x80] = 0x00;
    LogDev->Registers[0x81] = 0x0C;
    LogDev->Registers[0x82] = 0x80;
    LogDev->Registers[0x83] = 0x00;
    /* Memory 32 Control #1 - enable upper limit, 8-bit memory */
    LogDev->Registers[0x84] = 0x01;
    /* Memory 32 Limit #1 0xD0000 (0xC8000 + 0x8000 = 0xD0000) */
    LogDev->Registers[0x85] = 0x00;
    LogDev->Registers[0x86] = 0x0D;
    LogDev->Registers[0x87] = 0x00;
    LogDev->Registers[0x88] = 0x00;

    /* ********************* DEVICE 6 ********************* */
    LogDev++;

    /* Enable decodes */
    LogDev->Registers[0x30] = 0x01;

    /* Memory Base #0 0xA0000 */
    LogDev->Registers[0x40] = 0x0A;
    LogDev->Registers[0x41] = 0x00;
    /*
     * Memory Control #0 - enable upper limit, 8-bit memory.
     * The resource descriptor is 16-bit,
     * so we can test the configuration code that touches this register.
     */
    LogDev->Registers[0x42] = 0x01;
    /* Memory Limit #0 0xA4000 (0xA0000 + 0x4000 = 0xA4000) */
    LogDev->Registers[0x43] = 0x0A;
    LogDev->Registers[0x44] = 0x40;

    /* Memory Control #1 - enable range length, 8-bit memory */
    LogDev->Registers[0x4A] = 0x00;
    /* Memory Base #1 is disabled */

    /* I/O Base 80 */
    LogDev->Registers[0x60] = 0x00;
    LogDev->Registers[0x61] = 0x80;

    /* IRQ 5 low-to-high transition */
    LogDev->Registers[0x70] = 0x05 | 0xF0; // We add some garbage, must be ignored by the driver
    LogDev->Registers[0x71] = 0x02;

    /* DMA 6 */
    LogDev->Registers[0x74] = 0x06 | 0xF8; // Ditto

    /* No DMA is active */
    LogDev->Registers[0x75] = 0x04;

    /* ********************* DEVICE 7 ********************* */

    /* No resources on purpose */
}

/* No boot resources */
static
VOID
DrvTestCard1Dev1QueryResources(
    _In_ PCM_RESOURCE_LIST ResourceList)
{
    ok_eq_pointer(ResourceList, NULL);
}

/*
 * Interface 1 Bus 0 Slot 0 AlternativeLists 2
 *
 * AltList 0, AltList->Count 8 Ver.1 Rev.30
 * [0:1:11] IO: Min 0:330, Max 0:341, Align 4 Len 2
 * [0:1:5]  IO: Min 0:600, Max 0:787, Align 1 Len 8
 * [0:1:5]  IO: Min 0:16, Max 0:21, Align 1 Len C
 * [0:1:1]  INT: Min 5 Max 5
 * [0:1:1]  INT: Min 2 Max 2
 * [8:1:1]  INT: Min 3 Max 3
 * [8:1:1]  INT: Min 4 Max 4
 * [0:0:0]  DMA: Min 2 Max 2
 *
 * AltList 1, AltList->Count 6 Ver.1 Rev.31
 * [0:1:11] IO: Min 0:330, Max 0:341, Align 4 Len 2
 * [0:1:10] MEM: Min 0:C8000, Max 0:DFFFF, Align FF40 Len 4000
 * [0:1:10] MEM: Min 0:D4000, Max 0:DBFFF, Align 10000 Len 4000
 * [0:0:0]  DMA: Min 0 Max 0
 * [8:0:0]  DMA: Min 1 Max 1
 * [0:0:0]  DMA: Min 2 Max 2
 */
static
VOID
DrvTestCard1Dev1QueryResourceRequirements(
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    PIO_RESOURCE_LIST AltList;

    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    expect_requirements_list_header(ReqList, Isa, 2UL);

    /************************* LIST 0 ************************/

    AltList = &ReqList->List[0];
    Descriptor = &AltList->Descriptors[0];

    expect_alt_list_header(AltList, 8UL);

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    2ul,
                    4ul,
                    0x330ull,
                    0x341ull);
    Descriptor++;

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_10_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    8ul,
                    1ul,
                    0x600ull,
                    0x787ull);
    Descriptor++;

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_10_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    12ul,
                    1ul,
                    0x16ull,
                    0x21ull);
    Descriptor++;

    expect_irq_req(Descriptor,
                   0,
                   CM_RESOURCE_INTERRUPT_LATCHED,
                   CmResourceShareDeviceExclusive,
                   5ul,
                   5ul);
    Descriptor++;

    // NOTE: The native driver returns CM_RESOURCE_INTERRUPT_LATCHED
    // and CmResourceShareDeviceExclusive for some reason
#if 0
    expect_irq_req(Descriptor,
                   0,
                   CM_RESOURCE_INTERRUPT_LATCHED,
                   CmResourceShareDeviceExclusive,
                   2ul,
                   2ul);
#else
    expect_irq_req(Descriptor,
                   0,
                   CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE,
                   CmResourceShareShared,
                   2ul,
                   2ul);
#endif
    Descriptor++;

#if 0
    expect_irq_req(Descriptor,
                   IO_RESOURCE_ALTERNATIVE,
                   CM_RESOURCE_INTERRUPT_LATCHED,
                   CmResourceShareDeviceExclusive,
                   3ul,
                   3ul);
#else
    expect_irq_req(Descriptor,
                   IO_RESOURCE_ALTERNATIVE,
                   CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE,
                   CmResourceShareShared,
                   3ul,
                   3ul);
#endif
    Descriptor++;

#if 0
    expect_irq_req(Descriptor,
                   IO_RESOURCE_ALTERNATIVE,
                   CM_RESOURCE_INTERRUPT_LATCHED,
                   CmResourceShareDeviceExclusive,
                   4ul,
                   4ul);
#else
    expect_irq_req(Descriptor,
                   IO_RESOURCE_ALTERNATIVE,
                   CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE,
                   CmResourceShareShared,
                   4ul,
                   4ul);
#endif
    Descriptor++;

    expect_dma_req(Descriptor,
                   0,
                   CM_RESOURCE_DMA_8,
                   CmResourceShareUndetermined,
                   2ul,
                   2ul);
    Descriptor++;

    /************************* LIST 1 ************************/

    AltList = (PIO_RESOURCE_LIST)(AltList->Descriptors + AltList->Count);
    Descriptor = &AltList->Descriptors[0];

    expect_alt_list_header(AltList, 6UL);

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    2ul,
                    4ul,
                    0x330ull,
                    0x341ull);
    Descriptor++;

    expect_mem_req(Descriptor,
                   0,
                   CM_RESOURCE_MEMORY_24,
                   CmResourceShareDeviceExclusive,
                   0x4000ul,
                   0xFF40ul,
                   0xC8000ull,
                   0xDFFFFull);
    Descriptor++;

    // NOTE: The native driver returns CM_RESOURCE_MEMORY_24 only for some reason
#if 0
    expect_mem_req(Descriptor,
                   0,
                   CM_RESOURCE_MEMORY_24,
                   CmResourceShareDeviceExclusive,
                   0x4000ul,
                   0x10000ul,
                   0xD4000ull,
                   0xDBFFFull);
#else
    expect_mem_req(Descriptor,
                   0,
                   CM_RESOURCE_MEMORY_24 | CM_RESOURCE_MEMORY_READ_ONLY,
                   CmResourceShareDeviceExclusive,
                   0x4000ul,
                   0x10000ul,
                   0xD4000ull,
                   0xDBFFFull);
#endif
    Descriptor++;

    expect_dma_req(Descriptor,
                   0,
                   CM_RESOURCE_DMA_8,
                   CmResourceShareUndetermined,
                   0ul,
                   0ul);
    Descriptor++;

    expect_dma_req(Descriptor,
                   IO_RESOURCE_ALTERNATIVE,
                   CM_RESOURCE_DMA_8,
                   CmResourceShareUndetermined,
                   1ul,
                   1ul);
    Descriptor++;

    expect_dma_req(Descriptor,
                   0,
                   CM_RESOURCE_DMA_8,
                   CmResourceShareUndetermined,
                   2ul,
                   2ul);
    Descriptor++;

    /*********************************************************/

    ok_int(ReqList->ListSize, GetPoolAllocSize(ReqList));
    ok_int(ReqList->ListSize, (ULONG_PTR)Descriptor - (ULONG_PTR)ReqList);
}

VOID
DrvTestCard1Dev1Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    DrvTestCard1Dev1QueryResources(ResourceList);
    DrvTestCard1Dev1QueryResourceRequirements(ReqList);
}

/* No boot resources */
static
VOID
DrvTestCard1Dev2QueryResources(
    _In_ PCM_RESOURCE_LIST ResourceList)
{
    ok_eq_pointer(ResourceList, NULL);
}

/*
 * Interface 1 Bus 0 Slot 0 AlternativeLists 2
 *
 * AltList 0, AltList->Count 2 Ver.1 Rev.30
 * [0:1:11] IO: Min 0:500, Max 0:506, Align 1 Len 1
 * [0:1:11] IO: Min 0:600, Max 0:607, Align 1 Len 1
 *
 * AltList 1, AltList->Count 2 Ver.1 Rev.31
 * [0:1:11] IO: Min 0:500, Max 0:506, Align 1 Len 1
 * [0:1:11] IO: Min 0:700, Max 0:708, Align 1 Len 1
 */
static
VOID
DrvTestCard1Dev2QueryResourceRequirements(
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    PIO_RESOURCE_LIST AltList;

    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    expect_requirements_list_header(ReqList, Isa, 2UL);

    /************************* LIST 0 ************************/

    AltList = &ReqList->List[0];
    Descriptor = &AltList->Descriptors[0];

    expect_alt_list_header(AltList, 2UL);

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    1ul,
                    1ul,
                    0x500ull,
                    0x506ull);
    Descriptor++;

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    1ul,
                    1ul,
                    0x600ull,
                    0x607ull);
    Descriptor++;

    /************************* LIST 1 ************************/

    AltList = (PIO_RESOURCE_LIST)(AltList->Descriptors + AltList->Count);
    Descriptor = &AltList->Descriptors[0];

    expect_alt_list_header(AltList, 2UL);

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    1ul,
                    1ul,
                    0x500ull,
                    0x506ull);
    Descriptor++;

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    1ul,
                    1ul,
                    0x700ull,
                    0x708ull);
    Descriptor++;
}

VOID
DrvTestCard1Dev2Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    DrvTestCard1Dev2QueryResources(ResourceList);
    DrvTestCard1Dev2QueryResourceRequirements(ReqList);
}

/* No boot resources */
static
VOID
DrvTestCard1Dev3QueryResources(
    _In_ PCM_RESOURCE_LIST ResourceList)
{
    ok_eq_pointer(ResourceList, NULL);
}

/*
 * Interface 1 Bus 0 Slot 0 AlternativeLists 2
 *
 * AltList 0, AltList->Count 2 Ver.1 Rev.30
 * [0:1:11] IO: Min 0:600, Max 0:607, Align 1 Len 1
 * [0:1:11] IO: Min 0:500, Max 0:506, Align 1 Len 1
 *
 * AltList 1, AltList->Count 2 Ver.1 Rev.31
 * [0:1:11] IO: Min 0:700, Max 0:708, Align 1 Len 1
 * [0:1:11] IO: Min 0:500, Max 0:506, Align 1 Len 1
 */
static
VOID
DrvTestCard1Dev3QueryResourceRequirements(
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    PIO_RESOURCE_LIST AltList;

    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    expect_requirements_list_header(ReqList, Isa, 2UL);

    /************************* LIST 0 ************************/

    AltList = &ReqList->List[0];
    Descriptor = &AltList->Descriptors[0];

    expect_alt_list_header(AltList, 2UL);

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    1ul,
                    1ul,
                    0x600ull,
                    0x607ull);
    Descriptor++;

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    1ul,
                    1ul,
                    0x500ull,
                    0x506ull);
    Descriptor++;

    /************************* LIST 1 ************************/

    AltList = (PIO_RESOURCE_LIST)(AltList->Descriptors + AltList->Count);
    Descriptor = &AltList->Descriptors[0];

    expect_alt_list_header(AltList, 2UL);

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    1ul,
                    1ul,
                    0x700ull,
                    0x708ull);
    Descriptor++;

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    1ul,
                    1ul,
                    0x500ull,
                    0x506ull);
    Descriptor++;
}

VOID
DrvTestCard1Dev3Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    DrvTestCard1Dev3QueryResources(ResourceList);
    DrvTestCard1Dev3QueryResourceRequirements(ReqList);
}

/* No boot resources */
static
VOID
DrvTestCard1Dev4QueryResources(
    _In_ PCM_RESOURCE_LIST ResourceList)
{
    ok_eq_pointer(ResourceList, NULL);
}

/*
 * Interface 1 Bus 0 Slot 0 AlternativeLists 2
 *
 * AltList 0, AltList->Count 1 Ver.1 Rev.30
 * [0:1:11] IO: Min 0:600, Max 0:607, Align 1 Len 1
 *
 * AltList 1, AltList->Count 1 Ver.1 Rev.31
 * [0:1:11] IO: Min 0:700, Max 0:708, Align 1 Len 1
 */
static
VOID
DrvTestCard1Dev4QueryResourceRequirements(
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    PIO_RESOURCE_LIST AltList;

    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    expect_requirements_list_header(ReqList, Isa, 2UL);

    /************************* LIST 0 ************************/

    AltList = &ReqList->List[0];
    Descriptor = &AltList->Descriptors[0];

    expect_alt_list_header(AltList, 1UL);

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    1ul,
                    1ul,
                    0x600ull,
                    0x607ull);
    Descriptor++;

    /************************* LIST 1 ************************/

    AltList = (PIO_RESOURCE_LIST)(AltList->Descriptors + AltList->Count);
    Descriptor = &AltList->Descriptors[0];

    expect_alt_list_header(AltList, 1UL);

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    1ul,
                    1ul,
                    0x700ull,
                    0x708ull);
    Descriptor++;
}

VOID
DrvTestCard1Dev4Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    DrvTestCard1Dev4QueryResources(ResourceList);
    DrvTestCard1Dev4QueryResourceRequirements(ReqList);
}

/*
 * FullList Count 1
 * List #0 Iface 1 Bus #0 Ver.0 Rev.3000 Count 2
 * [1:11] MEM: 0:D6000 Len 8000
 * [1:11] MEM: 0:C8000 Len 8000
 */
static
VOID
DrvTestCard1Dev5QueryResources(
    _In_ PCM_RESOURCE_LIST ResourceList)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    ok(ResourceList != NULL, "ResourceList is NULL\n");
    if (ResourceList == NULL)
    {
        skip("No ResourceList\n");
        return;
    }
    expect_resource_list_header(ResourceList, Isa, 2UL);

    Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];

    expect_mem_res(Descriptor,
                   CM_RESOURCE_MEMORY_24 | CM_RESOURCE_MEMORY_READ_ONLY,
                   CmResourceShareDeviceExclusive,
                   0x8000ul,
                   0xD6000ull);
    Descriptor++;

    expect_mem_res(Descriptor,
                   CM_RESOURCE_MEMORY_24 | CM_RESOURCE_MEMORY_READ_ONLY,
                   CmResourceShareDeviceExclusive,
                   0x8000ul,
                   0xC8000ull);
    Descriptor++;

    /*********************************************************/

    ok_eq_size(GetPoolAllocSize(ResourceList), (ULONG_PTR)Descriptor - (ULONG_PTR)ResourceList);
}

/*
 * Interface 1 Bus 0 Slot 0 AlternativeLists 1
 *
 * AltList 0, AltList->Count 3 Ver.1 Rev.30
 * [1:3:0]  CFG: Priority 2000 Res1 720075 Res2 650072
 * [1:1:11] MEM: Min 0:D6000, Max 0:DDFFF, Align 1 Len 8000
 * [1:1:11] MEM: Min 0:C8000, Max 0:CFFFF, Align 1 Len 8000
 *
 * OR (decodes disabled)
 *
 * AltList 0, AltList->Count 2 Ver.1 Rev.30
 * [0:1:10] MEM: Min 0:D0000, Max 0:E7FFF, Align 100 Len 8000
 * [0:1:10] MEM: Min 0:C8000, Max 0:CFFFF, Align 1 Len 8000
 */
static
VOID
DrvTestCard1Dev5QueryResourceRequirements(
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    PIO_RESOURCE_LIST AltList;
    ULONG Count;

    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    expect_requirements_list_header(ReqList, Isa, 1UL);

    /************************* LIST 0 ************************/

    AltList = &ReqList->List[0];
    Descriptor = &AltList->Descriptors[0];

    if (Descriptor->Type == CmResourceTypeConfigData)
        Count = 3;
    else
        Count = 2;
    expect_alt_list_header(AltList, Count);

    /* TODO: Should we support this? */
    if (Descriptor->Type == CmResourceTypeConfigData)
    {
        expect_cfg_req(Descriptor,
                       IO_RESOURCE_PREFERRED,
                       0,
                       CmResourceShareShared,
                       0x2000ul,
                       0ul,
                       0ul);
        Descriptor++;
    }

    expect_mem_req(Descriptor,
                   0,
                   CM_RESOURCE_MEMORY_24 | CM_RESOURCE_MEMORY_READ_ONLY,
                   CmResourceShareDeviceExclusive,
                   0x8000ul,
                   0x100ul,
                   0xD0000ull,
                   0xE7FFFull);
    Descriptor++;

    expect_mem_req(Descriptor,
                   0,
                   CM_RESOURCE_MEMORY_24 | CM_RESOURCE_MEMORY_READ_ONLY,
                   CmResourceShareDeviceExclusive,
                   0x8000ul,
                   0x1ul,
                   0xC8000ull,
                   0xCFFFFull);
    Descriptor++;

    /*********************************************************/

    ok_int(ReqList->ListSize, GetPoolAllocSize(ReqList));
    ok_int(ReqList->ListSize, (ULONG_PTR)Descriptor - (ULONG_PTR)ReqList);
}

VOID
DrvTestCard1Dev5Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    DrvTestCard1Dev5QueryResources(ResourceList);
    DrvTestCard1Dev5QueryResourceRequirements(ReqList);
}

/*
 * FullList Count 1
 * List #0 Iface 1 Bus #0 Ver.0 Rev.3000 Count 4
 * [1:11] MEM: 0:A0000 Len 4000
 * [1:5]  IO:  Start 0:80, Len 8
 * [1:0]  DMA: Channel 6 Port 0 Res 0
 * [1:1]  INT: Lev 5 Vec 5 Aff FFFFFFFF
 */
static
VOID
DrvTestCard1Dev6QueryResources(
    _In_ PCM_RESOURCE_LIST ResourceList)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    ok(ResourceList != NULL, "ResourceList is NULL\n");
    if (ResourceList == NULL)
    {
        skip("No ResourceList\n");
        return;
    }
    expect_resource_list_header(ResourceList, Isa, 4UL);

    Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];

    expect_port_res(Descriptor,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_10_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    8ul,
                    0x80ull);
    Descriptor++;

    expect_irq_res(Descriptor,
                   CM_RESOURCE_INTERRUPT_LATCHED,
                   CmResourceShareDeviceExclusive,
                   5ul,
                   5ul,
                   (KAFFINITY)-1);
    Descriptor++;

    expect_dma_res(Descriptor,
                   0,
                   CmResourceShareDeviceExclusive,
                   6ul);
    Descriptor++;

    expect_mem_res(Descriptor,
                   CM_RESOURCE_MEMORY_24 | CM_RESOURCE_MEMORY_READ_ONLY,
                   CmResourceShareDeviceExclusive,
                   0x4000ul,
                   0xA0000ull);
    Descriptor++;

    /*********************************************************/

    ok_eq_size(GetPoolAllocSize(ResourceList), (ULONG_PTR)Descriptor - (ULONG_PTR)ResourceList);
}

/*
 * Interface 1 Bus 0 Slot 0 AlternativeLists 1
 *
 * AltList 0, AltList->Count 6 Ver.1 Rev.30
 * [1:3:0]  CFG: Priority 2000 Res1 7 Res2 0
 * [1:1:11] MEM: Min 0:A0000, Max 0:A3FFF, Align 1 Len 4000
 * [0:1:10] MEM: Min 0:80000, Max 0:900FF, Align 4 Len 100
 * [1:0:0]  DMA: Min 6 Max 6
 * [1:1:1]  INT: Min 5 Max 5
 * [1:1:5]  IO: Min 0:80, Max 0:87, Align 1 Len 8
 *
 * OR (decodes disabled)
 *
 * AltList 0, AltList->Count 5 Ver.1 Rev.30
 * [0:1:10] MEM: Min 0:A0000, Max 0:A4FFF, Align 4 Len 1000
 * [0:1:10] MEM: Min 0:80000, Max 0:900FF, Align 4 Len 100
 * [0:0:0]  DMA: Min 6 Max 6
 * [0:1:1]  INT: Min 5 Max 5
 * [0:1:5]  IO: Min 0:80, Max 0:87, Align 1 Len 8
 */
static
VOID
DrvTestCard1Dev6QueryResourceRequirements(
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    PIO_RESOURCE_LIST AltList;
    ULONG Count;

    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    expect_requirements_list_header(ReqList, Isa, 1UL);

    /************************* LIST 0 ************************/

    AltList = &ReqList->List[0];
    Descriptor = &AltList->Descriptors[0];

    if (Descriptor->Type == CmResourceTypeConfigData)
        Count = 6;
    else
        Count = 5;
    expect_alt_list_header(AltList, Count);

    /* TODO: Should we support this? */
    if (Descriptor->Type == CmResourceTypeConfigData)
    {
        expect_cfg_req(Descriptor,
                       IO_RESOURCE_PREFERRED,
                       0,
                       CmResourceShareShared,
                       0x2000ul,
                       0ul,
                       0ul);
        Descriptor++;
    }

    expect_mem_req(Descriptor,
                   0,
                   CM_RESOURCE_MEMORY_24 | CM_RESOURCE_MEMORY_READ_ONLY,
                   CmResourceShareDeviceExclusive,
                   0x1000ul,
                   0x4ul,
                   0xA0000ull,
                   0xA4FFFull);
    Descriptor++;

    // NOTE: The native driver returns CM_RESOURCE_MEMORY_24 only for some reason
#if 0
    expect_mem_req(Descriptor,
                   0,
                   CM_RESOURCE_MEMORY_24,
                   CmResourceShareDeviceExclusive,
                   0x100ul,
                   0x4ul,
                   0x80000ull,
                   0x900FFull);
#else
    expect_mem_req(Descriptor,
                   0,
                   CM_RESOURCE_MEMORY_24 | CM_RESOURCE_MEMORY_READ_ONLY,
                   CmResourceShareDeviceExclusive,
                   0x100ul,
                   0x4ul,
                   0x80000ull,
                   0x900FFull);
#endif
    Descriptor++;

    expect_dma_req(Descriptor,
                   0,
                   CM_RESOURCE_DMA_8,
                   CmResourceShareUndetermined,
                   6ul,
                   6ul);
    Descriptor++;

    expect_irq_req(Descriptor,
                   0,
                   CM_RESOURCE_INTERRUPT_LATCHED,
                   CmResourceShareDeviceExclusive,
                   5ul,
                   5ul);
    Descriptor++;

    expect_port_req(Descriptor,
                    0,
                    CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_10_BIT_DECODE,
                    CmResourceShareDeviceExclusive,
                    8ul,
                    1ul,
                    0x80ull,
                    0x87ull);
    Descriptor++;

    /*********************************************************/

    ok_int(ReqList->ListSize, GetPoolAllocSize(ReqList));
    ok_int(ReqList->ListSize, (ULONG_PTR)Descriptor - (ULONG_PTR)ReqList);
}

VOID
DrvTestCard1Dev6Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    DrvTestCard1Dev6QueryResources(ResourceList);
    DrvTestCard1Dev6QueryResourceRequirements(ReqList);
}

VOID
DrvTestCard1Dev6ConfigurationResult(
    _In_ PISAPNP_CARD_LOGICAL_DEVICE LogDev)
{
    ULONG i, Offset;

    /* Memory Base #0 = 0xA2000 */
    ok_eq_int(LogDev->Registers[0x40], 0x0A);
    ok_eq_int(LogDev->Registers[0x41], 0x20);
    /* Memory Control #0 = upper limit enabled, 16-bit memory */
    ok_eq_int(LogDev->Registers[0x42], 0x03);
    /* Memory Upper limit #0 = 0xA3000 (0xA2000 + 0x1000) */
    ok_eq_int(LogDev->Registers[0x43], 0x0A);
    ok_eq_int(LogDev->Registers[0x44], 0x30);

    /* Memory Base #1 = 0x89000 */
    ok_eq_int(LogDev->Registers[0x48], 0x08);
    ok_eq_int(LogDev->Registers[0x49], 0x90);
    /* Memory Control #1 = range length enabled, 8-bit memory */
    ok_eq_int(LogDev->Registers[0x4A], 0x00);
    /* Memory Upper limit #1 = 0xFFFF00 (0x100) */
    ok_eq_int(LogDev->Registers[0x4B], 0xFF);
    ok_eq_int(LogDev->Registers[0x4C], 0xFF);

    /* Memory #2-3 should be disabled */
    for (i = 2; i < 4; ++i)
    {
        Offset = 0x40 + i * 8;

        /* Memory Base */
        ok_eq_int(LogDev->Registers[Offset    ], 0x00);
        ok_eq_int(LogDev->Registers[Offset + 1], 0x00);
        /* Memory Control */
        ok_eq_int(LogDev->Registers[Offset + 2], 0x00);
        /* Memory Upper limit or range length */
        ok_eq_int(LogDev->Registers[Offset + 3], 0x00);
        ok_eq_int(LogDev->Registers[Offset + 4], 0x00);
    }

    /* Memory 32 #0-3 should be disabled */
    for (i = 0; i < 4; ++i)
    {
        if (i == 0)
            Offset = 0x76;
        else
            Offset = 0x70 + i * 16;

        /* Memory 32 Base */
        ok_eq_int(LogDev->Registers[Offset    ], 0x00);
        ok_eq_int(LogDev->Registers[Offset + 1], 0x00);
        ok_eq_int(LogDev->Registers[Offset + 2], 0x00);
        ok_eq_int(LogDev->Registers[Offset + 3], 0x00);
        /* Memory 32 Control */
        ok_eq_int(LogDev->Registers[Offset + 4], 0x00);
        /* Memory 32 Upper limit or range length */
        ok_eq_int(LogDev->Registers[Offset + 5], 0x00);
        ok_eq_int(LogDev->Registers[Offset + 6], 0x00);
        ok_eq_int(LogDev->Registers[Offset + 7], 0x00);
        ok_eq_int(LogDev->Registers[Offset + 8], 0x00);
    }

    /* I/O Base #0 = 0x80 */
    ok_eq_int(LogDev->Registers[0x60], 0x00);
    ok_eq_int(LogDev->Registers[0x61], 0x80);

    /* I/O Base #1-6 should be disabled */
    for (i = 1; i < 6; ++i)
    {
        Offset = 0x60 + i * 2;

        ok_eq_int(LogDev->Registers[Offset    ], 0x00);
        ok_eq_int(LogDev->Registers[Offset + 1], 0x00);
    }

    /* IRQ select #0 = IRQ 5 low-to-high transition */
    ok_eq_int(LogDev->Registers[0x70], 0x05);
    ok_eq_int(LogDev->Registers[0x71], 0x02);

    /* IRQ select #1 should be disabled */
    ok_eq_int(LogDev->Registers[0x72], 0x00);
    ok_eq_int(LogDev->Registers[0x73], 0x00);

    /* DMA select #0 = DMA 6 */
    ok_eq_int(LogDev->Registers[0x74], 0x06);

    /* DMA select #1 = No DMA is active */
    ok_eq_int(LogDev->Registers[0x75], 0x04);
}

PCM_RESOURCE_LIST
DrvTestCard1Dev6CreateConfigurationResources(VOID)
{
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG ListSize;

#define RESOURCE_COUNT 5
    /*
     * Make the following resources from the requirements:
     *
     * FullList Count 1
     * List #0 Iface 1 Bus #0 Ver.1 Rev.1 Count 5
     * [1:11] MEM: 0:A2000 Len 1000
     * [1:11] MEM: 0:89000 Len 100
     * [0:0]  DMA: Channel 6 Port 0 Res 0
     * [1:1]  INT: Lev 5 Vec 3F Aff FFFFFFFF
     * [1:5]  IO:  Start 0:80, Len 8
     */
    ListSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors) +
               sizeof(*Descriptor) * RESOURCE_COUNT;

    ResourceList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ListSize);
    if (ResourceList == NULL)
        return NULL;
    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = Isa;
    ResourceList->List[0].BusNumber = 0;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = RESOURCE_COUNT;

    Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];

    Descriptor->Type = CmResourceTypeMemory;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_MEMORY_24 | CM_RESOURCE_MEMORY_READ_ONLY;
    Descriptor->u.Memory.Start.LowPart = 0xA2000;
    Descriptor->u.Memory.Length = 0x1000;
    ++Descriptor;

    Descriptor->Type = CmResourceTypeMemory;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_MEMORY_24 | CM_RESOURCE_MEMORY_READ_ONLY;
    Descriptor->u.Memory.Start.LowPart = 0x89000;
    Descriptor->u.Memory.Length = 0x100;
    ++Descriptor;

    Descriptor->Type = CmResourceTypeDma;
    Descriptor->ShareDisposition = CmResourceShareUndetermined;
    Descriptor->Flags = CM_RESOURCE_DMA_8;
    Descriptor->u.Dma.Channel = 6;
    ++Descriptor;

    Descriptor->Type = CmResourceTypeInterrupt;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    Descriptor->u.Interrupt.Level = 5;
    Descriptor->u.Interrupt.Vector = 0x3F;
    Descriptor->u.Interrupt.Affinity = (KAFFINITY)-1;
    ++Descriptor;

    Descriptor->Type = CmResourceTypePort;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_10_BIT_DECODE;
    Descriptor->u.Memory.Start.LowPart = 0x80;
    Descriptor->u.Memory.Length = 8;

    return ResourceList;
}

VOID
DrvTestCard1Dev7Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList)
{
    /* No resources */
    ok_eq_pointer(ResourceList, NULL);
    ok_eq_pointer(ReqList, NULL);
}
