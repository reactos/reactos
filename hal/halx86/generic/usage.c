/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         HAL Resource Report Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <hal.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

BOOLEAN HalpGetInfoFromACPI;
BOOLEAN HalpNMIDumpFlag;
PUCHAR KdComPortInUse;
PADDRESS_USAGE HalpAddressUsageList;
IDTUsageFlags HalpIDTUsageFlags[MAXIMUM_IDTVECTOR+1];
IDTUsage HalpIDTUsage[MAXIMUM_IDTVECTOR+1];

USHORT HalpComPortIrqMapping[5][2] =
{
    {0x3F8, 4},
    {0x2F8, 3},
    {0x3E8, 4},
    {0x2E8, 3},
    {0, 0}
};

ADDRESS_USAGE HalpComIoSpace =
{
    NULL, CmResourceTypePort, IDT_INTERNAL,
    {
        {0x2F8,   0x8},     /* COM 1 */
        {0,0},
    }
};

ADDRESS_USAGE HalpDefaultIoSpace =
{
    NULL, CmResourceTypePort, IDT_INTERNAL,
    {
#if defined(SARCH_PC98)
        /* PIC 1 */
        {0x00,  1},
        {0x02,  1},
        /* PIC 2 */
        {0x08,  1},
        {0x0A,  1},
        /* DMA */
        {0x01,  1},
        {0x03,  1},
        {0x05,  1},
        {0x07,  1},
        {0x09,  1},
        {0x0B,  1},
        {0x0D,  1},
        {0x0F,  1},
        {0x11,  1},
        {0x13,  1},
        {0x15,  1},
        {0x17,  1},
        {0x19,  1},
        {0x1B,  1},
        {0x1D,  1},
        {0x1F,  1},
        {0x21,  1},
        {0x23,  1},
        {0x25,  1},
        {0x27,  1},
        {0x29,  1},
        {0x2B,  1},
        {0x2D,  1},
        {0xE05, 1},
        {0xE07, 1},
        {0xE09, 1},
        {0xE0B, 1},
        /* RTC */
        {0x20,  1},
        {0x22,  1},
        {0x128, 1},
        /* System Control */
        {0x33,  1},
        {0x37,  1},
        /* PIT */
        {0x71,  1},
        {0x73,  1},
        {0x75,  1},
        {0x77,  1},
        {0x3FD9,1},
        {0x3FDB,1},
        {0x3FDD,1},
        {0x3FDF,1},
        /* x87 Coprocessor */
        {0xF8,  8},
#else
        {0x00,  0x20}, /* DMA 1 */
        {0xC0,  0x20}, /* DMA 2 */
        {0x80,  0x10}, /* DMA EPAR */
        {0x20,  0x2},  /* PIC 1 */
        {0xA0,  0x2},  /* PIC 2 */
        {0x40,  0x4},  /* PIT 1 */
        {0x48,  0x4},  /* PIT 2 */
        {0x92,  0x1},  /* System Control Port A */
        {0x70,  0x2},  /* CMOS  */
        {0xF0,  0x10}, /* x87 Coprocessor */
#endif
        {0xCF8, 0x8},  /* PCI 0 */
        {0,0},
    }
};

/* FUNCTIONS ******************************************************************/

#ifndef _MINIHAL_
CODE_SEG("INIT")
VOID
NTAPI
HalpGetResourceSortValue(IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
                         OUT PULONG Scale,
                         OUT PLARGE_INTEGER Value)
{
    /* Sorting depends on resource type */
    switch (Descriptor->Type)
    {
        case CmResourceTypeInterrupt:

            /* Interrupt goes by level */
            *Scale = 0;
            *Value = RtlConvertUlongToLargeInteger(Descriptor->u.Interrupt.Level);
            break;

        case CmResourceTypePort:

            /* Port goes by port address */
            *Scale = 1;
            *Value = Descriptor->u.Port.Start;
            break;

        case CmResourceTypeMemory:

            /* Memory goes by base address */
            *Scale = 2;
            *Value = Descriptor->u.Memory.Start;
            break;

        default:

            /* Anything else */
            *Scale = 4;
            *Value = RtlConvertUlongToLargeInteger(0);
            break;
    }
}

CODE_SEG("INIT")
VOID
NTAPI
HalpBuildPartialFromIdt(IN ULONG Entry,
                        IN PCM_PARTIAL_RESOURCE_DESCRIPTOR RawDescriptor,
                        IN PCM_PARTIAL_RESOURCE_DESCRIPTOR TranslatedDescriptor)
{
    /* Exclusive interrupt entry */
    RawDescriptor->Type = CmResourceTypeInterrupt;
    RawDescriptor->ShareDisposition = CmResourceShareDriverExclusive;

    /* Check the interrupt type */
    if (HalpIDTUsageFlags[Entry].Flags & IDT_LATCHED)
    {
        /* Latched */
        RawDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    }
    else
    {
        /* Level */
        RawDescriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
    }

    /* Get vector and level from IDT usage */
    RawDescriptor->u.Interrupt.Vector = HalpIDTUsage[Entry].BusReleativeVector;
    RawDescriptor->u.Interrupt.Level = HalpIDTUsage[Entry].BusReleativeVector;

    /* Affinity is all the CPUs */
    RawDescriptor->u.Interrupt.Affinity = HalpActiveProcessors;

    /* The translated copy is identical */
    RtlCopyMemory(TranslatedDescriptor, RawDescriptor, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    /* But the vector and IRQL must be set correctly */
    TranslatedDescriptor->u.Interrupt.Vector = Entry;
    TranslatedDescriptor->u.Interrupt.Level = HalpIDTUsage[Entry].Irql;
}

CODE_SEG("INIT")
VOID
NTAPI
HalpBuildPartialFromAddress(IN INTERFACE_TYPE Interface,
                            IN PADDRESS_USAGE CurrentAddress,
                            IN ULONG Element,
                            IN PCM_PARTIAL_RESOURCE_DESCRIPTOR RawDescriptor,
                            IN PCM_PARTIAL_RESOURCE_DESCRIPTOR TranslatedDescriptor)
{
    ULONG AddressSpace;

    /* Set the type and make it exclusive */
    RawDescriptor->Type = CurrentAddress->Type;
    RawDescriptor->ShareDisposition = CmResourceShareDriverExclusive;

    /* Check what this is */
    if (RawDescriptor->Type == CmResourceTypePort)
    {
        /* Write out port data */
        AddressSpace = 1;
        RawDescriptor->Flags = CM_RESOURCE_PORT_IO;
        RawDescriptor->u.Port.Start.HighPart = 0;
        RawDescriptor->u.Port.Start.LowPart = CurrentAddress->Element[Element].Start;
        RawDescriptor->u.Port.Length = CurrentAddress->Element[Element].Length;

        /* Determine if 16-bit port addresses are allowed */
        RawDescriptor->Flags |= HalpIs16BitPortDecodeSupported();
    }
    else
    {
        /* Write out memory data */
        AddressSpace = 0;
        RawDescriptor->Flags = (CurrentAddress->Flags & IDT_READ_ONLY) ?
                                CM_RESOURCE_MEMORY_READ_ONLY :
                                CM_RESOURCE_MEMORY_READ_WRITE;
        RawDescriptor->u.Memory.Start.HighPart = 0;
        RawDescriptor->u.Memory.Start.LowPart = CurrentAddress->Element[Element].Start;
        RawDescriptor->u.Memory.Length = CurrentAddress->Element[Element].Length;
    }

    /* Make an identical copy to begin with */
    RtlCopyMemory(TranslatedDescriptor, RawDescriptor, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    /* Check what this is */
    if (RawDescriptor->Type == CmResourceTypePort)
    {
        /* Translate the port */
        HalTranslateBusAddress(Interface,
                               0,
                               RawDescriptor->u.Port.Start,
                               &AddressSpace,
                               &TranslatedDescriptor->u.Port.Start);

        /* If it turns out this is memory once translated, flag it */
        if (AddressSpace == 0) TranslatedDescriptor->Flags = CM_RESOURCE_PORT_MEMORY;

    }
    else
    {
        /* Translate the memory */
        HalTranslateBusAddress(Interface,
                               0,
                               RawDescriptor->u.Memory.Start,
                               &AddressSpace,
                               &TranslatedDescriptor->u.Memory.Start);
    }
}

CODE_SEG("INIT")
VOID
NTAPI
HalpReportResourceUsage(IN PUNICODE_STRING HalName,
                        IN INTERFACE_TYPE InterfaceType)
{
    PCM_RESOURCE_LIST RawList, TranslatedList;
    PCM_FULL_RESOURCE_DESCRIPTOR RawFull, TranslatedFull;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CurrentRaw, CurrentTranslated, SortedRaw, SortedTranslated;
    CM_PARTIAL_RESOURCE_DESCRIPTOR RawPartial, TranslatedPartial;
    PCM_PARTIAL_RESOURCE_LIST RawPartialList = NULL, TranslatedPartialList = NULL;
    INTERFACE_TYPE Interface;
    ULONG i, j, k, ListSize, Count, Port, Element, CurrentScale, SortScale, ReportType, FlagMatch;
    ADDRESS_USAGE *CurrentAddress;
    LARGE_INTEGER CurrentSortValue, SortValue;
    DbgPrint("%wZ Detected\n", HalName);

    /* Check if KD is using a COM port */
    if (KdComPortInUse)
    {
        /* Enter it into the I/O space */
        HalpComIoSpace.Element[0].Start = PtrToUlong(KdComPortInUse);
        HalpComIoSpace.Next = HalpAddressUsageList;
        HalpAddressUsageList = &HalpComIoSpace;

#if defined(SARCH_XBOX)
        /*
         * Do not claim interrupt resources for the KD COM port.
         * The actual COM port lacks SERIRQ, IRQ 4 is hardwired to the NIC.
         */
        UNREFERENCED_PARAMETER(Port);
#else
        /* Use the debug port table if we have one */
        HalpGetInfoFromACPI = HalpGetDebugPortTable();

        /* Check if we're using ACPI */
        if (!HalpGetInfoFromACPI)
        {
            /* No, so use our local table */
            for (i = 0, Port = HalpComPortIrqMapping[i][0];
                 Port;
                 i++, Port = HalpComPortIrqMapping[i][0])
            {
                /* Is this the port we want? */
                if (Port == (ULONG_PTR)KdComPortInUse)
                {
                    /* Register it */
                    HalpRegisterVector(IDT_DEVICE | IDT_LATCHED,
                                       HalpComPortIrqMapping[i][1],
                                       HalpComPortIrqMapping[i][1] +
                                       PRIMARY_VECTOR_BASE,
                                       HIGH_LEVEL);
                }
            }
        }
#endif
    }

    /* On non-ACPI systems, we need to build an address map */
    HalpBuildAddressMap();

    /* Allocate the master raw and translated lists */
    RawList = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE * 2, TAG_HAL);
    TranslatedList = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE * 2, TAG_HAL);
    if (!(RawList) || !(TranslatedList))
    {
        /* Bugcheck the system */
        KeBugCheckEx(HAL_MEMORY_ALLOCATION,
                     4 * PAGE_SIZE,
                     1,
                     (ULONG_PTR)__FILE__,
                     __LINE__);
    }

    /* Zero out the lists */
    RtlZeroMemory(RawList, PAGE_SIZE * 2);
    RtlZeroMemory(TranslatedList, PAGE_SIZE * 2);

    /* Set the interface type to begin with */
    RawList->List[0].InterfaceType = InterfaceTypeUndefined;

    /* Loop all IDT entries that are not IRQs */
    for (i = 0; i < PRIMARY_VECTOR_BASE; i++)
    {
        /* Check if the IDT isn't owned */
        if (!(HalpIDTUsageFlags[i].Flags & IDT_REGISTERED))
        {
            /* Then register it for internal usage */
            HalpIDTUsageFlags[i].Flags = IDT_INTERNAL;
            HalpIDTUsage[i].BusReleativeVector = (UCHAR)i;
        }
    }

    /* Our full raw descriptors start here */
    RawFull = RawList->List;

    /* Keep track of the current partial raw and translated descriptors */
    CurrentRaw = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)RawList->List;
    CurrentTranslated = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)TranslatedList->List;

    /* Do two passes */
    for (ReportType = 0; ReportType < 2; ReportType++)
    {
        /* Pass 0 is for device usage */
        if (ReportType == 0)
        {
            FlagMatch = IDT_DEVICE & ~IDT_REGISTERED;
            Interface = InterfaceType;
        }
        else
        {
            /* Past 1 is for internal HAL usage */
            FlagMatch = IDT_INTERNAL & ~IDT_REGISTERED;
            Interface = Internal;
        }

        /* Reset loop variables */
        i = Element = 0;

        /* Start looping our address uage list and interrupts */
        CurrentAddress = HalpAddressUsageList;
        while (TRUE)
        {
            /* Check for valid vector number */
            if (i <= MAXIMUM_IDTVECTOR)
            {
                /* Check if this entry should be parsed */
                if ((HalpIDTUsageFlags[i].Flags & FlagMatch))
                {
                    /* Parse it */
                    HalpBuildPartialFromIdt(i, &RawPartial, &TranslatedPartial);
                    i++;
                }
                else
                {
                    /* Skip this entry */
                    i++;
                    continue;
                }
            }
            else
            {
                /* This is an address instead */
                if (!CurrentAddress) break;

                /* Check if the address should be reported */
                if (!(CurrentAddress->Flags & FlagMatch) ||
                    !(CurrentAddress->Element[Element].Length))
                {
                    /* Nope, skip it */
                    Element = 0;
                    CurrentAddress = CurrentAddress->Next;
                    continue;
                }

                /* Otherwise, parse the entry */
                HalpBuildPartialFromAddress(Interface,
                                            CurrentAddress,
                                            Element,
                                            &RawPartial,
                                            &TranslatedPartial);
                Element++;
            }

            /* Check for interface change */
            if (RawFull->InterfaceType != Interface)
            {
                /* We need to add another full descriptor */
                RawList->Count++;
                TranslatedList->Count++;

                /* The full descriptor follows wherever we were */
                RawFull = (PCM_FULL_RESOURCE_DESCRIPTOR)CurrentRaw;
                TranslatedFull = (PCM_FULL_RESOURCE_DESCRIPTOR)CurrentTranslated;

                /* And it is of this new interface type */
                RawFull->InterfaceType = Interface;
                TranslatedFull->InterfaceType = Interface;

                /* And its partial descriptors begin here */
                RawPartialList = &RawFull->PartialResourceList;
                TranslatedPartialList = &TranslatedFull->PartialResourceList;

                /* And our next full descriptor should follow here */
                CurrentRaw = RawFull->PartialResourceList.PartialDescriptors;
                CurrentTranslated = TranslatedFull->PartialResourceList.PartialDescriptors;
            }

            /* We have written a new partial descriptor */
            RawPartialList->Count++;
            TranslatedPartialList->Count++;

            /* Copy our local descriptors into the actual list */
            RtlCopyMemory(CurrentRaw, &RawPartial, sizeof(RawPartial));
            RtlCopyMemory(CurrentTranslated, &TranslatedPartial, sizeof(TranslatedPartial));

            /* Move to the next partial descriptor */
            CurrentRaw++;
            CurrentTranslated++;
        }
    }

    /* Get the final list of the size for the kernel call later */
    ListSize = (ULONG)((ULONG_PTR)CurrentRaw - (ULONG_PTR)RawList);

    /* Now reset back to the first full descriptor */
    RawFull = RawList->List;
    TranslatedFull = TranslatedList->List;

    /* And loop all the full descriptors */
    for (i = 0; i < RawList->Count; i++)
    {
        /* Get the first partial descriptor in this list */
        CurrentRaw = RawFull->PartialResourceList.PartialDescriptors;
        CurrentTranslated = TranslatedFull->PartialResourceList.PartialDescriptors;

        /* Get the count of partials in this list */
        Count = RawFull->PartialResourceList.Count;

        /* Loop all the partials in this list */
        for (j = 0; j < Count; j++)
        {
            /* Get the sort value at this point */
            HalpGetResourceSortValue(CurrentRaw, &CurrentScale, &CurrentSortValue);

            /* Save the current sort pointer */
            SortedRaw = CurrentRaw;
            SortedTranslated = CurrentTranslated;

            /* Loop all descriptors starting from this one */
            for (k = j; k < Count; k++)
            {
                /* Get the sort value at the sort point */
                HalpGetResourceSortValue(SortedRaw, &SortScale, &SortValue);

                /* Check if a swap needs to occur */
                if ((SortScale < CurrentScale) ||
                    ((SortScale == CurrentScale) &&
                     (SortValue.QuadPart <= CurrentSortValue.QuadPart)))
                {
                    /* Swap raw partial with the sort location partial */
                    RtlCopyMemory(&RawPartial, CurrentRaw, sizeof(RawPartial));
                    RtlCopyMemory(CurrentRaw, SortedRaw, sizeof(RawPartial));
                    RtlCopyMemory(SortedRaw, &RawPartial, sizeof(RawPartial));

                    /* Swap translated partial in the same way */
                    RtlCopyMemory(&TranslatedPartial, CurrentTranslated, sizeof(TranslatedPartial));
                    RtlCopyMemory(CurrentTranslated, SortedTranslated, sizeof(TranslatedPartial));
                    RtlCopyMemory(SortedTranslated, &TranslatedPartial, sizeof(TranslatedPartial));

                    /* Update the sort value at this point */
                    HalpGetResourceSortValue(CurrentRaw, &CurrentScale, &CurrentSortValue);
                }

                /* The sort location has been updated */
                SortedRaw++;
                SortedTranslated++;
            }

            /* Move to the next partial */
            CurrentRaw++;
            CurrentTranslated++;
        }

        /* Move to the next full descriptor */
        RawFull = (PCM_FULL_RESOURCE_DESCRIPTOR)CurrentRaw;
        TranslatedFull = (PCM_FULL_RESOURCE_DESCRIPTOR)CurrentTranslated;
    }

    /* Mark this is an ACPI system, if it is */
    HalpMarkAcpiHal();

    /* Tell the kernel about all this */
    IoReportHalResourceUsage(HalName,
                             RawList,
                             TranslatedList,
                             ListSize);

    /* Free our lists */
    ExFreePool(RawList);
    ExFreePool(TranslatedList);

    /* Get the machine's serial number */
    HalpReportSerialNumber();
}
#endif /* !_MINIHAL_ */

CODE_SEG("INIT")
VOID
NTAPI
HalpRegisterVector(IN UCHAR Flags,
                   IN ULONG BusVector,
                   IN ULONG SystemVector,
                   IN KIRQL Irql)
{
    /* Save the vector flags */
    HalpIDTUsageFlags[SystemVector].Flags = Flags;

    /* Save the vector data */
    HalpIDTUsage[SystemVector].Irql  = Irql;
    HalpIDTUsage[SystemVector].BusReleativeVector = (UCHAR)BusVector;
}

#ifndef _MINIHAL_
CODE_SEG("INIT")
VOID
NTAPI
HalpEnableInterruptHandler(IN UCHAR Flags,
                           IN ULONG BusVector,
                           IN ULONG SystemVector,
                           IN KIRQL Irql,
                           IN PVOID Handler,
                           IN KINTERRUPT_MODE Mode)
{
    /* Set the IDT_LATCHED flag for latched interrupts */
    if (Mode == Latched) Flags |= IDT_LATCHED;

    /* Register the vector */
    HalpRegisterVector(Flags, BusVector, SystemVector, Irql);

    /* Connect the interrupt */
    KeRegisterInterruptHandler(SystemVector, Handler);

    /* Enable the interrupt */
    HalEnableSystemInterrupt(SystemVector, Irql, Mode);
}

CODE_SEG("INIT")
VOID
NTAPI
HalpGetNMICrashFlag(VOID)
{
    UNICODE_STRING ValueName;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\CrashControl");
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG ResultLength;
    HANDLE Handle;
    NTSTATUS Status;
    KEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;

    /* Set default */
    HalpNMIDumpFlag = 0;

    /* Initialize attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open crash key */
    Status = ZwOpenKey(&Handle, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Query key value */
        RtlInitUnicodeString(&ValueName, L"NMICrashDump");
        Status = ZwQueryValueKey(Handle,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 &KeyValueInformation,
                                 sizeof(KeyValueInformation),
                                 &ResultLength);
        if (NT_SUCCESS(Status))
        {
            /* Check for valid data */
            if (ResultLength == sizeof(KEY_VALUE_PARTIAL_INFORMATION))
            {
                /* Read the flag */
                HalpNMIDumpFlag = KeyValueInformation.Data[0];
            }
        }

        /* We're done */
        ZwClose(Handle);
    }
}
#endif  /* !_MINIHAL_ */

/* EOF */
