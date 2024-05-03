/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Unit Tests for the ISA PnP bus driver (device discovery and resource tests)
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#include "../../../../drivers/bus/isapnp/isapnp.c"
#include "../../../../drivers/bus/isapnp/hardware.c"

/* GLOBALS ********************************************************************/

static const ULONG DrvpIsaBusPorts[] = { 0xA79, 0x279 };
static const ULONG DrvpIsaBusReadDataPorts[] = { 0x274, 0x3E4, 0x204, 0x2E4, 0x354, 0x2F4 };

extern PISAPNP_CARD IsapCard;

#define TEST_RDP_IO_BASE  ((PUCHAR)(0x2F4 | 3))

/* FUNCTIONS ******************************************************************/

static
VOID
DrvFlushDeviceConfig(
    _In_ PISAPNP_CARD_LOGICAL_DEVICE LogDev)
{
    UCHAR MemControl[8];

    /*
     * Save the memory control registers
     * since we would need the correct values for the configuration process.
     */
    MemControl[0] = LogDev->Registers[0x42];
    MemControl[1] = LogDev->Registers[0x4A];
    MemControl[2] = LogDev->Registers[0x52];
    MemControl[3] = LogDev->Registers[0x5A];
    MemControl[4] = LogDev->Registers[0x7A];
    MemControl[5] = LogDev->Registers[0x84];
    MemControl[6] = LogDev->Registers[0x94];
    MemControl[7] = LogDev->Registers[0xA4];

    /* Fill the whole configuration area with 0xCC for testing purposes */
    RtlFillMemory(&LogDev->Registers[0x40], sizeof(LogDev->Registers) - 0x40, 0xCC);

    /* Restore saved registers */
    LogDev->Registers[0x42] = MemControl[0];
    LogDev->Registers[0x4A] = MemControl[1];
    LogDev->Registers[0x52] = MemControl[2];
    LogDev->Registers[0x5A] = MemControl[3];
    LogDev->Registers[0x7A] = MemControl[4];
    LogDev->Registers[0x84] = MemControl[5];
    LogDev->Registers[0x94] = MemControl[6];
    LogDev->Registers[0xA4] = MemControl[7];
}

static
BOOLEAN
DrvCreateCards(VOID)
{
    PISAPNP_CARD Card;

    /* Create 2 cards */
    IsapCard = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*IsapCard) * 2);
    if (!IsapCard)
        return FALSE;

    Card = IsapCard;
    DrvCreateCard1(Card++);
    DrvCreateCard2(Card++);

    return TRUE;
}

static
BOOLEAN
DrvTestIsolation(VOID)
{
    UCHAR Cards;

    /* Run the isolation protocol on an empty bus */
    Cards = IsaHwTryReadDataPort(TEST_RDP_IO_BASE);
    ok_eq_int(Cards, 0);
    IsaHwWaitForKey();

    if (!DrvCreateCards())
    {
        skip("No memory\n");
        return FALSE;
    }

    /* Another bus that contains 2 cards */
    Cards = IsaHwTryReadDataPort(TEST_RDP_IO_BASE);
    ok_eq_int(Cards, 2);

    return TRUE;
}

static
VOID
DrvTestResources(VOID)
{
    ISAPNP_FDO_EXTENSION FdoExt = { 0 };
    PISAPNP_CARD_LOGICAL_DEVICE LogDev;
    PLIST_ENTRY Entry;
    ULONG i;

    /* Our cards were isolated via DrvTestIsolation() */
    FdoExt.Cards = 2;
    FdoExt.ReadDataPort = TEST_RDP_IO_BASE;
    InitializeListHead(&FdoExt.DeviceListHead);

    /* Enumerate all logical devices on the bus */
    IsaHwFillDeviceList(&FdoExt);
    IsaHwWaitForKey();

    for (Entry = FdoExt.DeviceListHead.Flink, i = 0;
         Entry != &FdoExt.DeviceListHead;
         Entry = Entry->Flink)
    {
        ISAPNP_PDO_EXTENSION PdoExt = { 0 };
        PCM_RESOURCE_LIST ResourceList;
        PIO_RESOURCE_REQUIREMENTS_LIST ReqList;

        PdoExt.IsaPnpDevice = CONTAINING_RECORD(Entry, ISAPNP_LOGICAL_DEVICE, DeviceLink);

        /* Create the resource lists */
        IsaPnpCreateLogicalDeviceRequirements(&PdoExt);
        IsaPnpCreateLogicalDeviceResources(&PdoExt);

        ReqList = PdoExt.RequirementsList;
        ResourceList = PdoExt.ResourceList;

        /* Process each discovered logical device */
        switch (i++)
        {
            case 0:
            {
                DrvTestCard1Dev1Resources(ResourceList, ReqList);

                LogDev = &IsapCard[0].LogDev[0];
                ok_eq_int(LogDev->Registers[0x30], 0x00);
                break;
            }
            case 1:
            {
                DrvTestCard1Dev2Resources(ResourceList, ReqList);

                LogDev = &IsapCard[0].LogDev[1];
                ok_eq_int(LogDev->Registers[0x30], 0x00);
                break;
            }
            case 2:
            {
                DrvTestCard1Dev3Resources(ResourceList, ReqList);

                LogDev = &IsapCard[0].LogDev[2];
                ok_eq_int(LogDev->Registers[0x30], 0x00);
                break;
            }
            case 3:
            {
                DrvTestCard1Dev4Resources(ResourceList, ReqList);

                LogDev = &IsapCard[0].LogDev[3];
                ok_eq_int(LogDev->Registers[0x30], 0x00);
                break;
            }
            case 4:
            {
                DrvTestCard1Dev5Resources(ResourceList, ReqList);

                LogDev = &IsapCard[0].LogDev[4];
                ok_eq_int(LogDev->Registers[0x30], 0x00);
                break;
            }
            case 5:
            {
                DrvTestCard1Dev6Resources(ResourceList, ReqList);

                /* Card 1, logical device 6 */
                LogDev = &IsapCard[0].LogDev[5];

                /* Should be activated only after configuration */
                ok_eq_int(LogDev->Registers[0x30], 0x00);

                /* I/O configuration test */
                {
                    NTSTATUS Status;

                    DrvFlushDeviceConfig(LogDev);

                    /* Assume that this device comes up with I/O range check logic enabled */
                    LogDev->Registers[0x31] = 0x02;

                    /* Create new resources */
                    ResourceList = DrvTestCard1Dev6CreateConfigurationResources();
                    if (ResourceList == NULL)
                    {
                        skip("No ResourceList\n");
                        break;
                    }

                    /* Assign resources to the device */
                    {
                        IsaHwWakeDevice(PdoExt.IsaPnpDevice);

                        Status = IsaHwConfigureDevice(&FdoExt, PdoExt.IsaPnpDevice, ResourceList);
                        ok_eq_hex(Status, STATUS_SUCCESS);

                        IsaHwActivateDevice(&FdoExt, PdoExt.IsaPnpDevice);
                        IsaHwWaitForKey();
                    }

                    DrvTestCard1Dev6ConfigurationResult(LogDev);

                    /* I/O range check must be disabled */
                    ok_eq_int(LogDev->Registers[0x31], 0x00);

                    /* Verify device activation */
                    ok_eq_int(LogDev->Registers[0x30], 0x01);
                }
                break;
            }
            case 6:
            {
                DrvTestCard1Dev7Resources(ResourceList, ReqList);

                LogDev = &IsapCard[0].LogDev[6];
                ok_eq_int(LogDev->Registers[0x30], 0x00);
                break;
            }

            default:
                break;
        }
    }

    ok(i == 7, "Some devices not tested\n");
}

/*
 * FullList Count 1
 * List #0 Iface 0 Bus #0 Ver.0 Rev.3000 Count 2
 * [1:10] IO:  Start 0:A79, Len 1
 * [1:10] IO:  Start 0:279, Len 1
 */
static
VOID
DrvTestReadDataPortQueryResources(VOID)
{
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG i;

    ResourceList = IsaPnpCreateReadPortDOResources();

    ok(ResourceList != NULL, "ResourceList is NULL\n");
    if (ResourceList == NULL)
    {
        skip("No ResourceList\n");
        return;
    }
    expect_resource_list_header(ResourceList, Internal, 2UL);

    Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];

    for (i = 0; i < RTL_NUMBER_OF(DrvpIsaBusPorts); ++i)
    {
        expect_port_res(Descriptor,
                        CM_RESOURCE_PORT_16_BIT_DECODE,
                        CmResourceShareDeviceExclusive,
                        1ul,
                        (ULONG64)DrvpIsaBusPorts[i]);
        Descriptor++;
    }

    /*********************************************************/

    ok_eq_size(GetPoolAllocSize(ResourceList), (ULONG_PTR)Descriptor - (ULONG_PTR)ResourceList);
}

/*
 * Interface 0 Bus 0 Slot 0 AlternativeLists 1
 *
 * AltList, AltList->Count 10 Ver.1 Rev.1
 * [0:1:10] IO: Min 0:A79, Max 0:A79, Align 1 Len 1
 * [8:1:10] IO: Min 0:0, Max 0:0, Align 1 Len 0
 * [0:1:10] IO: Min 0:279, Max 0:279, Align 1 Len 1
 * [8:1:10] IO: Min 0:0, Max 0:0, Align 1 Len 0
 * [0:1:10] IO: Min 0:274, Max 0:277, Align 1 Len 4
 * [8:1:10] IO: Min 0:0, Max 0:0, Align 1 Len 0
 * [0:1:10] IO: Min 0:3E4, Max 0:3E7, Align 1 Len 4
 * [8:1:10] IO: Min 0:0, Max 0:0, Align 1 Len 0
 * [0:1:10] IO: Min 0:204, Max 0:207, Align 1 Len 4
 * [8:1:10] IO: Min 0:0, Max 0:0, Align 1 Len 0
 * [0:1:10] IO: Min 0:2E4, Max 0:2E7, Align 1 Len 4
 * [8:1:10] IO: Min 0:0, Max 0:0, Align 1 Len 0
 * [0:1:10] IO: Min 0:354, Max 0:357, Align 1 Len 4
 * [8:1:10] IO: Min 0:0, Max 0:0, Align 1 Len 0
 * [0:1:10] IO: Min 0:2F4, Max 0:2F7, Align 1 Len 4
 * [8:1:10] IO: Min 0:0, Max 0:0, Align 1 Len 0
*/
static
VOID
DrvTestReadDataPortQueryResourcesRequirementsForEnum(VOID)
{
    PIO_RESOURCE_REQUIREMENTS_LIST ReqList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    ULONG i;

    ReqList = IsaPnpCreateReadPortDORequirements(0);

    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    expect_requirements_list_header(ReqList, Internal, 1UL);
    expect_alt_list_header(&ReqList->List[0], 16UL);

    Descriptor = &ReqList->List[0].Descriptors[0];

    for (i = 0; i < RTL_NUMBER_OF(DrvpIsaBusPorts) * 2; ++i)
    {
        if ((i % 2) == 0)
        {
            expect_port_req(Descriptor,
                            0,
                            CM_RESOURCE_PORT_16_BIT_DECODE,
                            CmResourceShareDeviceExclusive,
                            1ul,
                            1ul,
                            (ULONG64)DrvpIsaBusPorts[i / 2],
                            (ULONG64)DrvpIsaBusPorts[i / 2]);
        }
        else
        {
            expect_port_req(Descriptor,
                            IO_RESOURCE_ALTERNATIVE,
                            CM_RESOURCE_PORT_16_BIT_DECODE,
                            CmResourceShareDeviceExclusive,
                            0ul,
                            1ul,
                            0ull,
                            0ull);
        }

        Descriptor++;
    }

    for (i = 0; i < RTL_NUMBER_OF(DrvpIsaBusReadDataPorts) * 2; ++i)
    {
        if ((i % 2) == 0)
        {
            expect_port_req(Descriptor,
                            0,
                            CM_RESOURCE_PORT_16_BIT_DECODE,
                            CmResourceShareDeviceExclusive,
                            4ul,
                            1ul,
                            (ULONG64)DrvpIsaBusReadDataPorts[i / 2],
                            (ULONG64)(DrvpIsaBusReadDataPorts[i / 2]) + 4 - 1);
        }
        else
        {
            expect_port_req(Descriptor,
                            IO_RESOURCE_ALTERNATIVE,
                            CM_RESOURCE_PORT_16_BIT_DECODE,
                            CmResourceShareDeviceExclusive,
                            0ul,
                            1ul,
                            0ull,
                            0ull);
        }

        Descriptor++;
    }

    /*********************************************************/

    ok_int(ReqList->ListSize, GetPoolAllocSize(ReqList));
    ok_int(ReqList->ListSize, (ULONG_PTR)Descriptor - (ULONG_PTR)ReqList);
}

/*
 * Interface 0 Bus 0 Slot 0 AlternativeLists 1
 *
 * AltList, AltList->Count A Ver.1 Rev.1
 * [0:1:10] IO: Min 0:A79, Max 0:A79, Align 1 Len 1
 * [8:1:10] IO: Min 0:0, Max 0:0, Align 1 Len 0
 * [0:1:10] IO: Min 0:279, Max 0:279, Align 1 Len 1
 * [8:1:10] IO: Min 0:0, Max 0:0, Align 1 Len 0
 * [8:1:10] IO: Min 0:274, Max 0:277, Align 1 Len 4
 * [8:1:10] IO: Min 0:3E4, Max 0:3E7, Align 1 Len 4
 * [8:1:10] IO: Min 0:204, Max 0:207, Align 1 Len 4
 * [8:1:10] IO: Min 0:2E4, Max 0:2E7, Align 1 Len 4
 * [0:1:10] IO: Min 0:354, Max 0:357, Align 1 Len 4 <-- selected (4th range)
 * [8:1:10] IO: Min 0:2F4, Max 0:2F7, Align 1 Len 4
 */
static
VOID
DrvTestReadDataPortQueryResourcesRequirementsForRebalance(VOID)
{
    PIO_RESOURCE_REQUIREMENTS_LIST ReqList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    ULONG i;

    /* Select the 4th I/O range in the list */
#define RDP_INDEX 4
    ReqList = IsaPnpCreateReadPortDORequirements(DrvpIsaBusReadDataPorts[RDP_INDEX]);

    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    expect_requirements_list_header(ReqList, Internal, 1UL);
    expect_alt_list_header(&ReqList->List[0], 10UL);

    Descriptor = &ReqList->List[0].Descriptors[0];

    for (i = 0; i < RTL_NUMBER_OF(DrvpIsaBusPorts) * 2; ++i)
    {
        if ((i % 2) == 0)
        {
            expect_port_req(Descriptor,
                            0,
                            CM_RESOURCE_PORT_16_BIT_DECODE,
                            CmResourceShareDeviceExclusive,
                            1ul,
                            1ul,
                            (ULONG64)DrvpIsaBusPorts[i / 2],
                            (ULONG64)DrvpIsaBusPorts[i / 2]);
        }
        else
        {
            expect_port_req(Descriptor,
                            IO_RESOURCE_ALTERNATIVE,
                            CM_RESOURCE_PORT_16_BIT_DECODE,
                            CmResourceShareDeviceExclusive,
                            0ul,
                            1ul,
                            0ull,
                            0ull);
        }

        Descriptor++;
    }

    for (i = 0; i < RTL_NUMBER_OF(DrvpIsaBusReadDataPorts); ++i)
    {
        expect_port_req(Descriptor,
                        (i == RDP_INDEX) ? 0 : IO_RESOURCE_ALTERNATIVE,
                        CM_RESOURCE_PORT_16_BIT_DECODE,
                        CmResourceShareDeviceExclusive,
                        4ul,
                        1ul,
                        (ULONG64)DrvpIsaBusReadDataPorts[i],
                        (ULONG64)(DrvpIsaBusReadDataPorts[i]) + 4 - 1);

        Descriptor++;
    }

    /*********************************************************/

    ok_int(ReqList->ListSize, GetPoolAllocSize(ReqList));
    ok_int(ReqList->ListSize, (ULONG_PTR)Descriptor - (ULONG_PTR)ReqList);
}

START_TEST(Resources)
{
    DrvTestReadDataPortQueryResources();
    DrvTestReadDataPortQueryResourcesRequirementsForEnum();
    DrvTestReadDataPortQueryResourcesRequirementsForRebalance();

    if (DrvTestIsolation())
    {
        DrvTestResources();
    }
}
