/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/legacy/bus/pcibus.c
 * PURPOSE:         PCI Bus Support (Configuration Space, Resource Allocation)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern BOOLEAN HalpPciLockSettings;
ULONG HalpBusType;

BOOLEAN HalpPCIConfigInitialized;
ULONG HalpMinPciBus, HalpMaxPciBus;
KSPIN_LOCK HalpPCIConfigLock;
PCI_CONFIG_HANDLER PCIConfigHandler;

/* PCI Operation Matrix */
UCHAR PCIDeref[4][4] =
{
    {0, 1, 2, 2},   // ULONG-aligned offset
    {1, 1, 1, 1},   // UCHAR-aligned offset
    {2, 1, 2, 2},   // USHORT-aligned offset
    {1, 1, 1, 1}    // UCHAR-aligned offset
};

/* Type 1 PCI Bus */
PCI_CONFIG_HANDLER PCIConfigHandlerType1 =
{
    /* Synchronization */
    (FncSync)HalpPCISynchronizeType1,
    (FncReleaseSync)HalpPCIReleaseSynchronzationType1,

    /* Read */
    {
        (FncConfigIO)HalpPCIReadUlongType1,
        (FncConfigIO)HalpPCIReadUcharType1,
        (FncConfigIO)HalpPCIReadUshortType1
    },

    /* Write */
    {
        (FncConfigIO)HalpPCIWriteUlongType1,
        (FncConfigIO)HalpPCIWriteUcharType1,
        (FncConfigIO)HalpPCIWriteUshortType1
    }
};

/* Type 2 PCI Bus */
PCI_CONFIG_HANDLER PCIConfigHandlerType2 =
{
    /* Synchronization */
    (FncSync)HalpPCISynchronizeType2,
    (FncReleaseSync)HalpPCIReleaseSynchronizationType2,

    /* Read */
    {
        (FncConfigIO)HalpPCIReadUlongType2,
        (FncConfigIO)HalpPCIReadUcharType2,
        (FncConfigIO)HalpPCIReadUshortType2
    },

    /* Write */
    {
        (FncConfigIO)HalpPCIWriteUlongType2,
        (FncConfigIO)HalpPCIWriteUcharType2,
        (FncConfigIO)HalpPCIWriteUshortType2
    }
};

PCIPBUSDATA HalpFakePciBusData =
{
    {
        PCI_DATA_TAG,
        PCI_DATA_VERSION,
        HalpReadPCIConfig,
        HalpWritePCIConfig,
        NULL,
        NULL,
        {{{0, 0, 0}}},
        {0, 0, 0, 0}
    },
    {{0, 0}},
    32,
};

BUS_HANDLER HalpFakePciBusHandler =
{
    1,
    PCIBus,
    PCIConfiguration,
    0,
    NULL,
    NULL,
    &HalpFakePciBusData,
    0,
    NULL,
    {0, 0, 0, 0},
    (PGETSETBUSDATA)HalpGetPCIData,
    (PGETSETBUSDATA)HalpSetPCIData,
    NULL,
    HalpAssignPCISlotResources,
    NULL,
    NULL
};

/* TYPE 1 FUNCTIONS **********************************************************/

VOID
NTAPI
HalpPCISynchronizeType1(IN PBUS_HANDLER BusHandler,
                        IN PCI_SLOT_NUMBER Slot,
                        OUT PKIRQL OldIrql,
                        OUT PPCI_TYPE1_CFG_BITS PciCfg1)
{
    /* Setup the PCI Configuration Register */
    PciCfg1->u.AsULONG = 0;
    PciCfg1->u.bits.BusNumber = BusHandler->BusNumber;
    PciCfg1->u.bits.DeviceNumber = Slot.u.bits.DeviceNumber;
    PciCfg1->u.bits.FunctionNumber = Slot.u.bits.FunctionNumber;
    PciCfg1->u.bits.Enable = TRUE;

    /* Acquire the lock */
    KeRaiseIrql(HIGH_LEVEL, OldIrql);
    KeAcquireSpinLockAtDpcLevel(&HalpPCIConfigLock);
}

VOID
NTAPI
HalpPCIReleaseSynchronzationType1(IN PBUS_HANDLER BusHandler,
                                  IN KIRQL OldIrql)
{
    PCI_TYPE1_CFG_BITS PciCfg1;

    /* Clear the PCI Configuration Register */
    PciCfg1.u.AsULONG = 0;
    WRITE_PORT_ULONG(((PPCIPBUSDATA)BusHandler->BusData)->Config.Type1.Address,
                     PciCfg1.u.AsULONG);

    /* Release the lock */
    KeReleaseSpinLock(&HalpPCIConfigLock, OldIrql);
}

TYPE1_READ(HalpPCIReadUcharType1, UCHAR)
TYPE1_READ(HalpPCIReadUshortType1, USHORT)
TYPE1_READ(HalpPCIReadUlongType1, ULONG)
TYPE1_WRITE(HalpPCIWriteUcharType1, UCHAR)
TYPE1_WRITE(HalpPCIWriteUshortType1, USHORT)
TYPE1_WRITE(HalpPCIWriteUlongType1, ULONG)

/* TYPE 2 FUNCTIONS **********************************************************/

VOID
NTAPI
HalpPCISynchronizeType2(IN PBUS_HANDLER BusHandler,
                        IN PCI_SLOT_NUMBER Slot,
                        OUT PKIRQL OldIrql,
                        OUT PPCI_TYPE2_ADDRESS_BITS PciCfg)
{
    PCI_TYPE2_CSE_BITS PciCfg2Cse;
    PPCIPBUSDATA BusData = (PPCIPBUSDATA)BusHandler->BusData;

    /* Setup the configuration register */
    PciCfg->u.AsUSHORT = 0;
    PciCfg->u.bits.Agent = (USHORT)Slot.u.bits.DeviceNumber;
    PciCfg->u.bits.AddressBase = (USHORT)BusData->Config.Type2.Base;

    /* Acquire the lock */
    KeRaiseIrql(HIGH_LEVEL, OldIrql);
    KeAcquireSpinLockAtDpcLevel(&HalpPCIConfigLock);

    /* Setup the CSE Register */
    PciCfg2Cse.u.AsUCHAR = 0;
    PciCfg2Cse.u.bits.Enable = TRUE;
    PciCfg2Cse.u.bits.FunctionNumber = (UCHAR)Slot.u.bits.FunctionNumber;
    PciCfg2Cse.u.bits.Key = -1;

    /* Write the bus number and CSE */
    WRITE_PORT_UCHAR(BusData->Config.Type2.Forward,
                     (UCHAR)BusHandler->BusNumber);
    WRITE_PORT_UCHAR(BusData->Config.Type2.CSE, PciCfg2Cse.u.AsUCHAR);
}

VOID
NTAPI
HalpPCIReleaseSynchronizationType2(IN PBUS_HANDLER BusHandler,
                                   IN KIRQL OldIrql)
{
    PCI_TYPE2_CSE_BITS PciCfg2Cse;
    PPCIPBUSDATA BusData = (PPCIPBUSDATA)BusHandler->BusData;

    /* Clear CSE and bus number */
    PciCfg2Cse.u.AsUCHAR = 0;
    WRITE_PORT_UCHAR(BusData->Config.Type2.CSE, PciCfg2Cse.u.AsUCHAR);
    WRITE_PORT_UCHAR(BusData->Config.Type2.Forward, 0);

    /* Release the lock */
    KeReleaseSpinLock(&HalpPCIConfigLock, OldIrql);
}

TYPE2_READ(HalpPCIReadUcharType2, UCHAR)
TYPE2_READ(HalpPCIReadUshortType2, USHORT)
TYPE2_READ(HalpPCIReadUlongType2, ULONG)
TYPE2_WRITE(HalpPCIWriteUcharType2, UCHAR)
TYPE2_WRITE(HalpPCIWriteUshortType2, USHORT)
TYPE2_WRITE(HalpPCIWriteUlongType2, ULONG)

/* PCI CONFIGURATION SPACE ***************************************************/

VOID
NTAPI
HalpPCIConfig(IN PBUS_HANDLER BusHandler,
              IN PCI_SLOT_NUMBER Slot,
              IN PUCHAR Buffer,
              IN ULONG Offset,
              IN ULONG Length,
              IN FncConfigIO *ConfigIO)
{
    KIRQL OldIrql;
    ULONG i;
    UCHAR State[20];

    /* Synchronize the operation */
    PCIConfigHandler.Synchronize(BusHandler, Slot, &OldIrql, State);

    /* Loop every increment */
    while (Length)
    {
        /* Find out the type of read/write we need to do */
        i = PCIDeref[Offset % sizeof(ULONG)][Length % sizeof(ULONG)];

        /* Do the read/write and return the number of bytes */
        i = ConfigIO[i]((PPCIPBUSDATA)BusHandler->BusData,
                        State,
                        Buffer,
                        Offset);

        /* Increment the buffer position and offset, and decrease the length */
        Offset += i;
        Buffer += i;
        Length -= i;
    }

    /* Release the lock and PCI bus */
    PCIConfigHandler.ReleaseSynchronzation(BusHandler, OldIrql);
}

VOID
NTAPI
HalpReadPCIConfig(IN PBUS_HANDLER BusHandler,
                  IN PCI_SLOT_NUMBER Slot,
                  IN PVOID Buffer,
                  IN ULONG Offset,
                  IN ULONG Length)
{
    /* Validate the PCI Slot */
    if (!HalpValidPCISlot(BusHandler, Slot))
    {
        /* Fill the buffer with invalid data */
        RtlFillMemory(Buffer, Length, -1);
    }
    else
    {
        /* Send the request */
        HalpPCIConfig(BusHandler,
                      Slot,
                      Buffer,
                      Offset,
                      Length,
                      PCIConfigHandler.ConfigRead);
    }
}

VOID
NTAPI
HalpWritePCIConfig(IN PBUS_HANDLER BusHandler,
                   IN PCI_SLOT_NUMBER Slot,
                   IN PVOID Buffer,
                   IN ULONG Offset,
                   IN ULONG Length)
{
    /* Validate the PCI Slot */
    if (HalpValidPCISlot(BusHandler, Slot))
    {
        /* Send the request */
        HalpPCIConfig(BusHandler,
                      Slot,
                      Buffer,
                      Offset,
                      Length,
                      PCIConfigHandler.ConfigWrite);
    }
}

#ifdef SARCH_XBOX
static
BOOLEAN
HalpXboxBlacklistedPCISlot(
    _In_ ULONG BusNumber,
    _In_ PCI_SLOT_NUMBER Slot)
{
    /* Trying to get PCI config data from devices 0:0:1 and 0:0:2 will completely
     * hang the Xbox. Also, the device number doesn't seem to be decoded for the
     * video card, so it appears to be present on 1:0:0 - 1:31:0.
     * We hack around these problems by indicating "device not present" for devices
     * 0:0:1, 0:0:2, 1:1:0, 1:2:0, 1:3:0, ...., 1:31:0 */
    if ((BusNumber == 0 && Slot.u.bits.DeviceNumber == 0 &&
        (Slot.u.bits.FunctionNumber == 1 || Slot.u.bits.FunctionNumber == 2)) ||
        (BusNumber == 1 && Slot.u.bits.DeviceNumber != 0))
    {
        DPRINT("Blacklisted PCI slot (%d:%d:%d)\n",
               BusNumber, Slot.u.bits.DeviceNumber, Slot.u.bits.FunctionNumber);
        return TRUE;
    }

    return FALSE;
}
#endif

BOOLEAN
NTAPI
HalpValidPCISlot(IN PBUS_HANDLER BusHandler,
                 IN PCI_SLOT_NUMBER Slot)
{
    PCI_SLOT_NUMBER MultiSlot;
    PPCIPBUSDATA BusData = (PPCIPBUSDATA)BusHandler->BusData;
    UCHAR HeaderType;
    //ULONG Device;

    /* Simple validation */
    if (Slot.u.bits.Reserved) return FALSE;
    if (Slot.u.bits.DeviceNumber >= BusData->MaxDevice) return FALSE;

#ifdef SARCH_XBOX
    if (HalpXboxBlacklistedPCISlot(BusHandler->BusNumber, Slot))
        return FALSE;
#endif

    /* Function 0 doesn't need checking */
    if (!Slot.u.bits.FunctionNumber) return TRUE;

    /* Functions 0+ need Multi-Function support, so check the slot */
    //Device = Slot.u.bits.DeviceNumber;
    MultiSlot = Slot;
    MultiSlot.u.bits.FunctionNumber = 0;

    /* Send function 0 request to get the header back */
    HalpReadPCIConfig(BusHandler,
                      MultiSlot,
                      &HeaderType,
                      FIELD_OFFSET(PCI_COMMON_CONFIG, HeaderType),
                      sizeof(UCHAR));

    /* Now make sure the header is multi-function */
    if (!(HeaderType & PCI_MULTIFUNCTION) || (HeaderType == 0xFF)) return FALSE;
    return TRUE;
}

CODE_SEG("INIT")
ULONG
HalpPhase0GetPciDataByOffset(
    _In_ ULONG Bus,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _Out_writes_bytes_all_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    ULONG BytesLeft = Length;
    PUCHAR BufferPtr = Buffer;
    PCI_TYPE1_CFG_BITS PciCfg;

#ifdef SARCH_XBOX
    if (HalpXboxBlacklistedPCISlot(Bus, PciSlot))
    {
        RtlFillMemory(Buffer, Length, 0xFF);
        return Length;
    }
#endif

    PciCfg.u.AsULONG = 0;
    PciCfg.u.bits.BusNumber = Bus;
    PciCfg.u.bits.DeviceNumber = PciSlot.u.bits.DeviceNumber;
    PciCfg.u.bits.FunctionNumber = PciSlot.u.bits.FunctionNumber;
    PciCfg.u.bits.Enable = TRUE;

    while (BytesLeft)
    {
        ULONG i;

        PciCfg.u.bits.RegisterNumber = Offset / sizeof(ULONG);
        WRITE_PORT_ULONG((PULONG)PCI_TYPE1_ADDRESS_PORT, PciCfg.u.AsULONG);

        i = PCIDeref[Offset % sizeof(ULONG)][BytesLeft % sizeof(ULONG)];
        switch (i)
        {
            case 0:
            {
                *(PULONG)BufferPtr = READ_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT);

                /* Number of bytes read */
                i = sizeof(ULONG);
                break;
            }
            case 1:
            {
                *BufferPtr = READ_PORT_UCHAR((PUCHAR)(PCI_TYPE1_DATA_PORT +
                                             Offset % sizeof(ULONG)));
                break;
            }
            case 2:
            {
                *(PUSHORT)BufferPtr = READ_PORT_USHORT((PUSHORT)(PCI_TYPE1_DATA_PORT +
                                                                 Offset % sizeof(ULONG)));
                break;
            }

            DEFAULT_UNREACHABLE;
        }

        Offset += i;
        BufferPtr += i;
        BytesLeft -= i;
    }

    return Length;
}

CODE_SEG("INIT")
ULONG
HalpPhase0SetPciDataByOffset(
    _In_ ULONG Bus,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    ULONG BytesLeft = Length;
    PUCHAR BufferPtr = Buffer;
    PCI_TYPE1_CFG_BITS PciCfg;

#ifdef SARCH_XBOX
    if (HalpXboxBlacklistedPCISlot(Bus, PciSlot))
    {
        return 0;
    }
#endif

    PciCfg.u.AsULONG = 0;
    PciCfg.u.bits.BusNumber = Bus;
    PciCfg.u.bits.DeviceNumber = PciSlot.u.bits.DeviceNumber;
    PciCfg.u.bits.FunctionNumber = PciSlot.u.bits.FunctionNumber;
    PciCfg.u.bits.Enable = TRUE;

    while (BytesLeft)
    {
        ULONG i;

        PciCfg.u.bits.RegisterNumber = Offset / sizeof(ULONG);
        WRITE_PORT_ULONG((PULONG)PCI_TYPE1_ADDRESS_PORT, PciCfg.u.AsULONG);

        i = PCIDeref[Offset % sizeof(ULONG)][BytesLeft % sizeof(ULONG)];
        switch (i)
        {
            case 0:
            {
                WRITE_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT, *(PULONG)BufferPtr);

                /* Number of bytes written */
                i = sizeof(ULONG);
                break;
            }
            case 1:
            {
                WRITE_PORT_UCHAR((PUCHAR)(PCI_TYPE1_DATA_PORT + Offset % sizeof(ULONG)),
                                 *BufferPtr);
                break;
            }
            case 2:
            {
                WRITE_PORT_USHORT((PUSHORT)(PCI_TYPE1_DATA_PORT + Offset % sizeof(ULONG)),
                                  *(PUSHORT)BufferPtr);
                break;
            }

            DEFAULT_UNREACHABLE;
        }

        Offset += i;
        BufferPtr += i;
        BytesLeft -= i;
    }

    return Length;
}

/* HAL PCI CALLBACKS *********************************************************/

ULONG
NTAPI
HalpGetPCIData(IN PBUS_HANDLER BusHandler,
               IN PBUS_HANDLER RootHandler,
               IN ULONG SlotNumber,
               IN PVOID Buffer,
               IN ULONG Offset,
               IN ULONG Length)
{
    PCI_SLOT_NUMBER Slot;
    UCHAR PciBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)PciBuffer;
    ULONG Len = 0;

    Slot.u.AsULONG = SlotNumber;
#ifdef SARCH_XBOX
    if (HalpXboxBlacklistedPCISlot(BusHandler->BusNumber, Slot))
    {
        RtlFillMemory(Buffer, Length, 0xFF);
        return Length;
    }
#endif

    /* Normalize the length */
    if (Length > sizeof(PCI_COMMON_CONFIG)) Length = sizeof(PCI_COMMON_CONFIG);

    /* Check if this is a vendor-specific read */
    if (Offset >= PCI_COMMON_HDR_LENGTH)
    {
        /* Read the header */
        HalpReadPCIConfig(BusHandler, Slot, PciConfig, 0, sizeof(ULONG));

        /* Make sure the vendor is valid */
        if (PciConfig->VendorID == PCI_INVALID_VENDORID) return 0;
    }
    else
    {
        /* Read the entire header */
        Len = PCI_COMMON_HDR_LENGTH;
        HalpReadPCIConfig(BusHandler, Slot, PciConfig, 0, Len);

        /* Validate the vendor ID */
        if (PciConfig->VendorID == PCI_INVALID_VENDORID)
        {
            /* It's invalid, but we want to return this much */
            Len = sizeof(USHORT);
        }

        /* Now check if there's space left */
        if (Len < Offset) return 0;

        /* There is, so return what's after the offset and normalize */
        Len -= Offset;
        if (Len > Length) Len = Length;

        /* Copy the data into the caller's buffer */
        RtlMoveMemory(Buffer, PciBuffer + Offset, Len);

        /* Update buffer and offset, decrement total length */
        Offset += Len;
        Buffer = (PVOID)((ULONG_PTR)Buffer + Len);
        Length -= Len;
    }

    /* Now we still have something to copy */
    if (Length)
    {
        /* Check if it's vendor-specific data */
        if (Offset >= PCI_COMMON_HDR_LENGTH)
        {
            /* Read it now */
            HalpReadPCIConfig(BusHandler, Slot, Buffer, Offset, Length);
            Len += Length;
        }
    }

    /* Update the total length read */
    return Len;
}

ULONG
NTAPI
HalpSetPCIData(IN PBUS_HANDLER BusHandler,
               IN PBUS_HANDLER RootHandler,
               IN ULONG SlotNumber,
               IN PVOID Buffer,
               IN ULONG Offset,
               IN ULONG Length)
{
    PCI_SLOT_NUMBER Slot;
    UCHAR PciBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)PciBuffer;
    ULONG Len = 0;

    Slot.u.AsULONG = SlotNumber;
#ifdef SARCH_XBOX
    if (HalpXboxBlacklistedPCISlot(BusHandler->BusNumber, Slot))
        return 0;
#endif

    /* Normalize the length */
    if (Length > sizeof(PCI_COMMON_CONFIG)) Length = sizeof(PCI_COMMON_CONFIG);

    /* Check if this is a vendor-specific read */
    if (Offset >= PCI_COMMON_HDR_LENGTH)
    {
        /* Read the header */
        HalpReadPCIConfig(BusHandler, Slot, PciConfig, 0, sizeof(ULONG));

        /* Make sure the vendor is valid */
        if (PciConfig->VendorID == PCI_INVALID_VENDORID) return 0;
    }
    else
    {
        /* Read the entire header and validate the vendor ID */
        Len = PCI_COMMON_HDR_LENGTH;
        HalpReadPCIConfig(BusHandler, Slot, PciConfig, 0, Len);
        if (PciConfig->VendorID == PCI_INVALID_VENDORID) return 0;

        /* Return what's after the offset and normalize */
        Len -= Offset;
        if (Len > Length) Len = Length;

        /* Copy the specific caller data */
        RtlMoveMemory(PciBuffer + Offset, Buffer, Len);

        /* Write the actual configuration data */
        HalpWritePCIConfig(BusHandler, Slot, PciBuffer + Offset, Offset, Len);

        /* Update buffer and offset, decrement total length */
        Offset += Len;
        Buffer = (PVOID)((ULONG_PTR)Buffer + Len);
        Length -= Len;
    }

    /* Now we still have something to copy */
    if (Length)
    {
        /* Check if it's vendor-specific data */
        if (Offset >= PCI_COMMON_HDR_LENGTH)
        {
            /* Read it now */
            HalpWritePCIConfig(BusHandler, Slot, Buffer, Offset, Length);
            Len += Length;
        }
    }

    /* Update the total length read */
    return Len;
}
#ifndef _MINIHAL_
ULONG
NTAPI
HalpGetPCIIntOnISABus(IN PBUS_HANDLER BusHandler,
                      IN PBUS_HANDLER RootHandler,
                      IN ULONG BusInterruptLevel,
                      IN ULONG BusInterruptVector,
                      OUT PKIRQL Irql,
                      OUT PKAFFINITY Affinity)
{
    /* Validate the level first */
    if (BusInterruptLevel < 1) return 0;

    /* PCI has its IRQs on top of ISA IRQs, so pass it on to the ISA handler */
    return HalGetInterruptVector(Isa,
                                 0,
                                 BusInterruptLevel,
                                 0,
                                 Irql,
                                 Affinity);
}
#endif // _MINIHAL_

VOID
NTAPI
HalpPCIPin2ISALine(IN PBUS_HANDLER BusHandler,
                   IN PBUS_HANDLER RootHandler,
                   IN PCI_SLOT_NUMBER SlotNumber,
                   IN PPCI_COMMON_CONFIG PciData)
{
    UNIMPLEMENTED_DBGBREAK();
}

VOID
NTAPI
HalpPCIISALine2Pin(IN PBUS_HANDLER BusHandler,
                   IN PBUS_HANDLER RootHandler,
                   IN PCI_SLOT_NUMBER SlotNumber,
                   IN PPCI_COMMON_CONFIG PciNewData,
                   IN PPCI_COMMON_CONFIG PciOldData)
{
    UNIMPLEMENTED_DBGBREAK();
}

#ifndef _MINIHAL_
NTSTATUS
NTAPI
HalpGetISAFixedPCIIrq(IN PBUS_HANDLER BusHandler,
                      IN PBUS_HANDLER RootHandler,
                      IN PCI_SLOT_NUMBER PciSlot,
                      OUT PSUPPORTED_RANGE *Range)
{
    PCI_COMMON_HEADER PciData;

    /* Read PCI configuration data */
    HalGetBusData(PCIConfiguration,
                  BusHandler->BusNumber,
                  PciSlot.u.AsULONG,
                  &PciData,
                  PCI_COMMON_HDR_LENGTH);

    /* Make sure it's a real device */
    if (PciData.VendorID == PCI_INVALID_VENDORID) return STATUS_UNSUCCESSFUL;

    /* Allocate the supported range structure */
    *Range = ExAllocatePoolWithTag(PagedPool, sizeof(SUPPORTED_RANGE), TAG_HAL);
    if (!*Range) return STATUS_INSUFFICIENT_RESOURCES;

    /* Set it up */
    RtlZeroMemory(*Range, sizeof(SUPPORTED_RANGE));
    (*Range)->Base = 1;

    /* If the PCI device has no IRQ, nothing to do */
    if (!PciData.u.type0.InterruptPin) return STATUS_SUCCESS;

    /* FIXME: The PCI IRQ Routing Miniport should be called */

    /* Also if the INT# seems bogus, nothing to do either */
    if ((PciData.u.type0.InterruptLine == 0) ||
        (PciData.u.type0.InterruptLine == 255))
    {
        /* Fake success */
        return STATUS_SUCCESS;
    }

    /* Otherwise, the INT# should be valid, return it to the caller */
    (*Range)->Base = PciData.u.type0.InterruptLine;
    (*Range)->Limit = PciData.u.type0.InterruptLine;
    return STATUS_SUCCESS;
}
#endif // _MINIHAL_

static ULONG NTAPI
PciSize(ULONG Base, ULONG Mask)
{
    ULONG Size = Mask & Base; /* Find the significant bits */
    Size = Size & ~(Size - 1); /* Get the lowest of them to find the decode size */
    return Size;
}

NTSTATUS
NTAPI
HalpAdjustPCIResourceList(IN PBUS_HANDLER BusHandler,
                          IN PBUS_HANDLER RootHandler,
                          IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *pResourceList)
{
    PPCIPBUSDATA BusData;
    PCI_SLOT_NUMBER SlotNumber;
    PSUPPORTED_RANGE Interrupt;
    NTSTATUS Status;

    /* Get PCI bus data */
    BusData = BusHandler->BusData;
    SlotNumber.u.AsULONG = (*pResourceList)->SlotNumber;

    /* Get the IRQ supported range */
    Status = BusData->GetIrqRange(BusHandler, RootHandler, SlotNumber, &Interrupt);
    if (!NT_SUCCESS(Status)) return Status;
#ifndef _MINIHAL_
    /* Handle the /PCILOCK feature */
    if (HalpPciLockSettings)
    {
        /* /PCILOCK is not yet supported */
        UNIMPLEMENTED_DBGBREAK("/PCILOCK boot switch is not yet supported.");
    }
#endif
    /* Now create the correct resource list based on the supported bus ranges */
#if 0
    Status = HaliAdjustResourceListRange(BusHandler->BusAddresses,
                                         Interrupt,
                                         pResourceList);
#else
    DPRINT1("HAL: No PCI Resource Adjustment done! Hardware may malfunction\n");
    Status = STATUS_SUCCESS;
#endif

    /* Return to caller */
    ExFreePool(Interrupt);
    return Status;
}

NTSTATUS
NTAPI
HalpAssignPCISlotResources(IN PBUS_HANDLER BusHandler,
                           IN PBUS_HANDLER RootHandler,
                           IN PUNICODE_STRING RegistryPath,
                           IN PUNICODE_STRING DriverClassName OPTIONAL,
                           IN PDRIVER_OBJECT DriverObject,
                           IN PDEVICE_OBJECT DeviceObject OPTIONAL,
                           IN ULONG Slot,
                           IN OUT PCM_RESOURCE_LIST *AllocatedResources)
{
    PCI_COMMON_CONFIG PciConfig;
    SIZE_T Address;
    ULONG ResourceCount;
    ULONG Size[PCI_TYPE0_ADDRESSES];
    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR Offset;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    PCI_SLOT_NUMBER SlotNumber;
    ULONG WriteBuffer;
    DPRINT1("WARNING: PCI Slot Resource Assignment is FOOBAR\n");

    /* FIXME: Should handle 64-bit addresses */

    /* Read configuration data */
    SlotNumber.u.AsULONG = Slot;
    HalpReadPCIConfig(BusHandler, SlotNumber, &PciConfig, 0, PCI_COMMON_HDR_LENGTH);

    /* Check if we read it correctly */
    if (PciConfig.VendorID == PCI_INVALID_VENDORID)
        return STATUS_NO_SUCH_DEVICE;

    /* Read the PCI configuration space for the device and store base address and
    size information in temporary storage. Count the number of valid base addresses */
    ResourceCount = 0;
    for (Address = 0; Address < PCI_TYPE0_ADDRESSES; Address++)
    {
        if (0xffffffff == PciConfig.u.type0.BaseAddresses[Address])
            PciConfig.u.type0.BaseAddresses[Address] = 0;

        /* Memory resource */
        if (0 != PciConfig.u.type0.BaseAddresses[Address])
        {
            ResourceCount++;

            Offset = (UCHAR)FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.BaseAddresses[Address]);

            /* Write 0xFFFFFFFF there */
            WriteBuffer = 0xffffffff;
            HalpWritePCIConfig(BusHandler, SlotNumber, &WriteBuffer, Offset, sizeof(ULONG));

            /* Read that figure back from the config space */
            HalpReadPCIConfig(BusHandler, SlotNumber, &Size[Address], Offset, sizeof(ULONG));

            /* Write back initial value */
            HalpWritePCIConfig(BusHandler, SlotNumber, &PciConfig.u.type0.BaseAddresses[Address], Offset, sizeof(ULONG));
        }
    }

    /* Interrupt resource */
    if (0 != PciConfig.u.type0.InterruptPin &&
        0 != PciConfig.u.type0.InterruptLine &&
        0xFF != PciConfig.u.type0.InterruptLine)
        ResourceCount++;

    /* Allocate output buffer and initialize */
    *AllocatedResources = ExAllocatePoolWithTag(
        PagedPool,
        sizeof(CM_RESOURCE_LIST) +
        (ResourceCount - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR),
        TAG_HAL);

    if (NULL == *AllocatedResources)
        return STATUS_NO_MEMORY;

    (*AllocatedResources)->Count = 1;
    (*AllocatedResources)->List[0].InterfaceType = PCIBus;
    (*AllocatedResources)->List[0].BusNumber = BusHandler->BusNumber;
    (*AllocatedResources)->List[0].PartialResourceList.Version = 1;
    (*AllocatedResources)->List[0].PartialResourceList.Revision = 1;
    (*AllocatedResources)->List[0].PartialResourceList.Count = ResourceCount;
    Descriptor = (*AllocatedResources)->List[0].PartialResourceList.PartialDescriptors;

    /* Store configuration information */
    for (Address = 0; Address < PCI_TYPE0_ADDRESSES; Address++)
    {
        if (0 != PciConfig.u.type0.BaseAddresses[Address])
        {
            if (PCI_ADDRESS_MEMORY_SPACE ==
                (PciConfig.u.type0.BaseAddresses[Address] & 0x1))
            {
                Descriptor->Type = CmResourceTypeMemory;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive; /* FIXME I have no idea... */
                Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;             /* FIXME Just a guess */
                Descriptor->u.Memory.Start.QuadPart = (PciConfig.u.type0.BaseAddresses[Address] & PCI_ADDRESS_MEMORY_ADDRESS_MASK);
                Descriptor->u.Memory.Length = PciSize(Size[Address], PCI_ADDRESS_MEMORY_ADDRESS_MASK);
            }
            else if (PCI_ADDRESS_IO_SPACE ==
                (PciConfig.u.type0.BaseAddresses[Address] & 0x1))
            {
                Descriptor->Type = CmResourceTypePort;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive; /* FIXME I have no idea... */
                Descriptor->Flags = CM_RESOURCE_PORT_IO;                       /* FIXME Just a guess */
                Descriptor->u.Port.Start.QuadPart = PciConfig.u.type0.BaseAddresses[Address] &= PCI_ADDRESS_IO_ADDRESS_MASK;
                Descriptor->u.Port.Length = PciSize(Size[Address], PCI_ADDRESS_IO_ADDRESS_MASK & 0xffff);
            }
            else
            {
                ASSERT(FALSE);
                return STATUS_UNSUCCESSFUL;
            }
            Descriptor++;
        }
    }

    if (0 != PciConfig.u.type0.InterruptPin &&
        0 != PciConfig.u.type0.InterruptLine &&
        0xFF != PciConfig.u.type0.InterruptLine)
    {
        Descriptor->Type = CmResourceTypeInterrupt;
        Descriptor->ShareDisposition = CmResourceShareShared;          /* FIXME Just a guess */
        Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;     /* FIXME Just a guess */
        Descriptor->u.Interrupt.Level = PciConfig.u.type0.InterruptLine;
        Descriptor->u.Interrupt.Vector = PciConfig.u.type0.InterruptLine;
        Descriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

        Descriptor++;
    }

    ASSERT(Descriptor == (*AllocatedResources)->List[0].PartialResourceList.PartialDescriptors + ResourceCount);

    /* FIXME: Should store the resources in the registry resource map */

    return Status;
}

ULONG
NTAPI
HaliPciInterfaceReadConfig(IN PBUS_HANDLER RootBusHandler,
                           IN ULONG BusNumber,
                           IN PCI_SLOT_NUMBER SlotNumber,
                           IN PVOID Buffer,
                           IN ULONG Offset,
                           IN ULONG Length)
{
    BUS_HANDLER BusHandler;

    /* Setup fake PCI Bus handler */
    RtlCopyMemory(&BusHandler, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
    BusHandler.BusNumber = BusNumber;

    /* Read configuration data */
    HalpReadPCIConfig(&BusHandler, SlotNumber, Buffer, Offset, Length);

    /* Return length */
    return Length;
}

CODE_SEG("INIT")
PPCI_REGISTRY_INFO_INTERNAL
NTAPI
HalpQueryPciRegistryInfo(VOID)
{
#ifndef _MINIHAL_
    WCHAR NameBuffer[8];
    OBJECT_ATTRIBUTES  ObjectAttributes;
    UNICODE_STRING KeyName, ConfigName, IdentName;
    HANDLE KeyHandle, BusKeyHandle, CardListHandle;
    NTSTATUS Status;
    UCHAR KeyBuffer[sizeof(CM_FULL_RESOURCE_DESCRIPTOR) + 100];
    PKEY_VALUE_FULL_INFORMATION ValueInfo = (PVOID)KeyBuffer;
    UCHAR PartialKeyBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                           sizeof(PCI_CARD_DESCRIPTOR)];
    PKEY_VALUE_PARTIAL_INFORMATION PartialValueInfo = (PVOID)PartialKeyBuffer;
    KEY_FULL_INFORMATION KeyInformation;
    ULONG ResultLength;
    PWSTR Tag;
    ULONG i, ElementCount;
    PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PPCI_REGISTRY_INFO PciRegInfo;
    PPCI_REGISTRY_INFO_INTERNAL PciRegistryInfo;
    PPCI_CARD_DESCRIPTOR CardDescriptor;

    /* Setup the object attributes for the key */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\Hardware\\Description\\"
                         L"System\\MultiFunctionAdapter");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the key */
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return NULL;

    /* Setup the receiving string */
    KeyName.Buffer = NameBuffer;
    KeyName.MaximumLength = sizeof(NameBuffer);

    /* Setup the configuration and identifier key names */
    RtlInitUnicodeString(&ConfigName, L"Configuration Data");
    RtlInitUnicodeString(&IdentName, L"Identifier");

    /* Keep looping for each ID */
    for (i = 0; TRUE; i++)
    {
        /* Setup the key name */
        RtlIntegerToUnicodeString(i, 10, &KeyName);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   KeyHandle,
                                   NULL);

        /* Open it */
        Status = ZwOpenKey(&BusKeyHandle, KEY_READ, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            /* None left, fail */
            ZwClose(KeyHandle);
            return NULL;
        }

        /* Read the registry data */
        Status = ZwQueryValueKey(BusKeyHandle,
                                 &IdentName,
                                 KeyValueFullInformation,
                                 ValueInfo,
                                 sizeof(KeyBuffer),
                                 &ResultLength);
        if (!NT_SUCCESS(Status))
        {
            /* Failed, try the next one */
            ZwClose(BusKeyHandle);
            continue;
        }

        /* Get the PCI Tag and validate it */
        Tag = (PWSTR)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);
        if ((Tag[0] != L'P') ||
            (Tag[1] != L'C') ||
            (Tag[2] != L'I') ||
            (Tag[3]))
        {
            /* Not a valid PCI entry, skip it */
            ZwClose(BusKeyHandle);
            continue;
        }

        /* Now read our PCI structure */
        Status = ZwQueryValueKey(BusKeyHandle,
                                 &ConfigName,
                                 KeyValueFullInformation,
                                 ValueInfo,
                                 sizeof(KeyBuffer),
                                 &ResultLength);
        ZwClose(BusKeyHandle);
        if (!NT_SUCCESS(Status)) continue;

        /* We read it OK! Get the actual resource descriptors */
        FullDescriptor  = (PCM_FULL_RESOURCE_DESCRIPTOR)
                          ((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);
        PartialDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
                            ((ULONG_PTR)FullDescriptor->
                                        PartialResourceList.PartialDescriptors);

        /* Check if this is our PCI Registry Information */
        if (PartialDescriptor->Type == CmResourceTypeDeviceSpecific)
        {
            /* It is, stop searching */
            break;
        }
    }

    /* Close the key */
    ZwClose(KeyHandle);

    /* Save the PCI information for later */
    PciRegInfo = (PPCI_REGISTRY_INFO)(PartialDescriptor + 1);

    /* Assume no Card List entries */
    ElementCount = 0;

    /* Set up for checking the PCI Card List key */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\PnP\\PCI\\CardList");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Attempt to open it */
    Status = ZwOpenKey(&CardListHandle, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* It exists, so let's query it */
        Status = ZwQueryKey(CardListHandle,
                            KeyFullInformation,
                            &KeyInformation,
                            sizeof(KEY_FULL_INFORMATION),
                            &ResultLength);
        if (!NT_SUCCESS(Status))
        {
            /* Failed to query, so no info */
            PciRegistryInfo = NULL;
        }
        else
        {
            /* Allocate the full structure */
            PciRegistryInfo =
                ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(PCI_REGISTRY_INFO_INTERNAL) +
                                      (KeyInformation.Values *
                                       sizeof(PCI_CARD_DESCRIPTOR)),
                                       TAG_HAL);
            if (PciRegistryInfo)
            {
                /* Get the first card descriptor entry */
                CardDescriptor = (PPCI_CARD_DESCRIPTOR)(PciRegistryInfo + 1);

                /* Loop all the values */
                for (i = 0; i < KeyInformation.Values; i++)
                {
                    /* Attempt to get the value */
                    Status = ZwEnumerateValueKey(CardListHandle,
                                                 i,
                                                 KeyValuePartialInformation,
                                                 PartialValueInfo,
                                                 sizeof(PartialKeyBuffer),
                                                 &ResultLength);
                    if (!NT_SUCCESS(Status))
                    {
                        /* Something went wrong, stop the search */
                        break;
                    }

                    /* Make sure it is correctly sized */
                    if (PartialValueInfo->DataLength == sizeof(PCI_CARD_DESCRIPTOR))
                    {
                        /* Sure is, copy it over */
                        *CardDescriptor = *(PPCI_CARD_DESCRIPTOR)
                                           PartialValueInfo->Data;

                        /* One more Card List entry */
                        ElementCount++;

                        /* Move to the next descriptor */
                        CardDescriptor = (CardDescriptor + 1);
                    }
                }
            }
        }

        /* Close the Card List key */
        ZwClose(CardListHandle);
    }
    else
    {
       /* No key, no Card List */
       PciRegistryInfo = NULL;
    }

    /* Check if we failed to get the full structure */
    if (!PciRegistryInfo)
    {
        /* Just allocate the basic structure then */
        PciRegistryInfo = ExAllocatePoolWithTag(NonPagedPool,
                                                sizeof(PCI_REGISTRY_INFO_INTERNAL),
                                                TAG_HAL);
        if (!PciRegistryInfo) return NULL;
    }

    /* Save the info we got */
    PciRegistryInfo->MajorRevision = PciRegInfo->MajorRevision;
    PciRegistryInfo->MinorRevision = PciRegInfo->MinorRevision;
    PciRegistryInfo->NoBuses = PciRegInfo->NoBuses;
    PciRegistryInfo->HardwareMechanism = PciRegInfo->HardwareMechanism;
    PciRegistryInfo->ElementCount = ElementCount;

    /* Return it */
    return PciRegistryInfo;
#else
    return NULL;
#endif
}

CODE_SEG("INIT")
VOID
NTAPI
HalpInitializePciStubs(VOID)
{
    PPCI_REGISTRY_INFO_INTERNAL PciRegistryInfo;
    UCHAR PciType;
    PPCIPBUSDATA BusData = (PPCIPBUSDATA)HalpFakePciBusHandler.BusData;
    ULONG i;
    PCI_SLOT_NUMBER j;
    ULONG VendorId = 0;
    ULONG MaxPciBusNumber;

    /* Query registry information */
    PciRegistryInfo = HalpQueryPciRegistryInfo();
    if (!PciRegistryInfo)
    {
        /* Assume type 1 */
        PciType = 1;

        /* Force a manual bus scan later */
        MaxPciBusNumber = MAXULONG;
    }
    else
    {
        /* Get the PCI type */
        PciType = PciRegistryInfo->HardwareMechanism & 0xF;

        /* Get MaxPciBusNumber and make it 0-based */
        MaxPciBusNumber = PciRegistryInfo->NoBuses - 1;

        /* Free the info structure */
        ExFreePoolWithTag(PciRegistryInfo, TAG_HAL);
    }

    /* Initialize the PCI lock */
    KeInitializeSpinLock(&HalpPCIConfigLock);

    /* Check the type of PCI bus */
    switch (PciType)
    {
        /* Type 1 PCI Bus */
        case 1:

            /* Copy the Type 1 handler data */
            RtlCopyMemory(&PCIConfigHandler,
                          &PCIConfigHandlerType1,
                          sizeof(PCIConfigHandler));

            /* Set correct I/O Ports */
            BusData->Config.Type1.Address = PCI_TYPE1_ADDRESS_PORT;
            BusData->Config.Type1.Data = PCI_TYPE1_DATA_PORT;
            break;

        /* Type 2 PCI Bus */
        case 2:

            /* Copy the Type 2 handler data */
            RtlCopyMemory(&PCIConfigHandler,
                          &PCIConfigHandlerType2,
                          sizeof (PCIConfigHandler));

            /* Set correct I/O Ports */
            BusData->Config.Type2.CSE = PCI_TYPE2_CSE_PORT;
            BusData->Config.Type2.Forward = PCI_TYPE2_FORWARD_PORT;
            BusData->Config.Type2.Base = PCI_TYPE2_ADDRESS_BASE;

            /* Only 16 devices supported, not 32 */
            BusData->MaxDevice = 16;
            break;

        default:

            /* Invalid type */
            DbgPrint("HAL: Unknown PCI type\n");
    }

    /* Run a forced bus scan if needed */
    if (MaxPciBusNumber == MAXULONG)
    {
        /* Initialize the max bus number to 0xFF */
        HalpMaxPciBus = 0xFF;

        /* Initialize the counter */
        MaxPciBusNumber = 0;

        /* Loop all possible buses */
        for (i = 0; i < HalpMaxPciBus; i++)
        {
            /* Loop all devices */
            for (j.u.AsULONG = 0; j.u.AsULONG < BusData->MaxDevice; j.u.AsULONG++)
            {
                /* Query the interface */
                if (HaliPciInterfaceReadConfig(NULL,
                                               i,
                                               j,
                                               &VendorId,
                                               0,
                                               sizeof(ULONG)))
                {
                    /* Validate the vendor ID */
                    if ((VendorId & 0xFFFF) != PCI_INVALID_VENDORID)
                    {
                        /* Set this as the maximum ID */
                        MaxPciBusNumber = i;
                        break;
                    }
                }
            }
        }
    }

    /* Set the real max bus number */
    HalpMaxPciBus = MaxPciBusNumber;

    /* We're done */
    HalpPCIConfigInitialized = TRUE;
}

/* EOF */

