/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel debugger PCI configurator
 * COPYRIGHT:   Copyright 2022 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * FIXME: We don't use a PCI resource allocator and rely on firmware to
 * have configured PCI devices properly. The KD PCI configurator should
 * allocate and assign PCI resources for all PCI buses
 * before the debugging device can be enabled.
 */

/* INCLUDES *******************************************************************/

#include <hal.h>

/* GLOBALS ********************************************************************/

#if defined(EARLY_DEBUG)
ULONG (*DPRINT0)(_In_ _Printf_format_string_ PCSTR Format, ...);
#else
#if defined(_MSC_VER)
#define DPRINT0   __noop
#else
#define DPRINT0
#endif
#endif

PCI_TYPE1_CFG_CYCLE_BITS HalpPciDebuggingDevice[2] = {0};

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("INIT")
ULONG
HalpPciBarLength(
    _In_ ULONG CurrentBar,
    _In_ ULONG NextBar)
{
    ULONG64 Bar;
    ULONG Length;

    Bar = CurrentBar;

    if (CurrentBar & PCI_ADDRESS_IO_SPACE)
    {
        Length = 1 << 2;
    }
    else
    {
        if ((CurrentBar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)
        {
            Bar = ((ULONG64)NextBar << 32) | CurrentBar;
        }

        Length = 1 << 4;
    }

    while (!(Bar & Length) && Length)
    {
        Length <<= 1;
    }

    return Length;
}

static
CODE_SEG("INIT")
BOOLEAN
HalpConfigureDebuggingDevice(
    _In_ PDEBUG_DEVICE_DESCRIPTOR PciDevice,
    _In_ ULONG PciBus,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _Inout_ PPCI_COMMON_HEADER PciConfig)
{
    ULONG i, Register;

    Register = PciConfig->Command & ~(PCI_ENABLE_MEMORY_SPACE |
                                     PCI_ENABLE_IO_SPACE);
    HalpPhase0SetPciDataByOffset(PciBus,
                                 PciSlot,
                                 &Register,
                                 FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                                 sizeof(USHORT));

    /* Fill out the device descriptor */
    for (i = 0; i < MAXIMUM_DEBUG_BARS; ++i)
    {
        ULONG Length, NextBar;
        PDEBUG_DEVICE_ADDRESS DeviceAddress;

        DeviceAddress = &PciDevice->BaseAddress[i];
        DeviceAddress->Valid = FALSE;

        Register = 0xFFFFFFFF;
        HalpPhase0SetPciDataByOffset(PciBus,
                                     PciSlot,
                                     &Register,
                                     FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.BaseAddresses[i]),
                                     sizeof(ULONG));
        HalpPhase0GetPciDataByOffset(PciBus,
                                     PciSlot,
                                     &Register,
                                     FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.BaseAddresses[i]),
                                     sizeof(ULONG));
        HalpPhase0SetPciDataByOffset(PciBus,
                                     PciSlot,
                                     &PciConfig->u.type0.BaseAddresses[i],
                                     FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.BaseAddresses[i]),
                                     sizeof(ULONG));

        if (i < MAXIMUM_DEBUG_BARS - 1)
            NextBar = PciConfig->u.type0.BaseAddresses[i + 1];
        else
            NextBar = 0;

        Length = HalpPciBarLength(Register, NextBar);
        if (Register == 0 || Length == 0)
            continue;

        /* I/O space */
        if (Register & PCI_ADDRESS_IO_SPACE)
        {
            DeviceAddress->Type = CmResourceTypePort;
            DeviceAddress->Length = Length;
            DeviceAddress->Valid = TRUE;
            DeviceAddress->TranslatedAddress =
                UlongToPtr(PciConfig->u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_ADDRESS_MASK);

            DPRINT0("BAR[%u] IO  %lx, length 0x%lx, 0x%lx\n",
                    i,
                    DeviceAddress->TranslatedAddress,
                    Length,
                    Register);
        }
        else
        {
            PHYSICAL_ADDRESS PhysicalAddress;
            BOOLEAN SkipBar = FALSE;

            DeviceAddress->Type = CmResourceTypeMemory;
            DeviceAddress->Length = Length;
            DeviceAddress->Valid = TRUE;

            /* 32-bit memory space */
            PhysicalAddress.HighPart = 0;
            PhysicalAddress.LowPart =
                PciConfig->u.type0.BaseAddresses[i] & PCI_ADDRESS_MEMORY_ADDRESS_MASK;

            /* 64-bit memory space */
            if (((Register & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT))
            {
                PhysicalAddress.HighPart = NextBar;
                SkipBar = TRUE;
            }

            DPRINT0("BAR[%u] MEM %I64x, length 0x%lx, 0x%lx\n",
                    i,
                    PhysicalAddress.QuadPart,
                    Length,
                    Register);

            if (SkipBar)
            {
                ++i;
            }

            DeviceAddress->TranslatedAddress =
                HalpMapPhysicalMemory64(PhysicalAddress, BYTES_TO_PAGES(Length));
        }
    }
    PciDevice->Bus = PciBus;
    PciDevice->Slot = PciSlot.u.AsULONG;
    PciDevice->VendorID = PciConfig->VendorID;
    PciDevice->DeviceID = PciConfig->DeviceID;
    PciDevice->BaseClass = PciConfig->BaseClass;
    PciDevice->SubClass = PciConfig->SubClass;
    PciDevice->ProgIf = PciConfig->ProgIf;

    /* Enable decodes */
    PciConfig->Command |= (PCI_ENABLE_MEMORY_SPACE |
                           PCI_ENABLE_IO_SPACE |
                           PCI_ENABLE_BUS_MASTER);
    HalpPhase0SetPciDataByOffset(PciBus,
                                 PciSlot,
                                 &PciConfig->Command,
                                 FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                                 sizeof(USHORT));

    return TRUE;
}

static
CODE_SEG("INIT")
BOOLEAN
HalpMatchDebuggingDevice(
    _In_ PDEBUG_DEVICE_DESCRIPTOR PciDevice,
    _In_ ULONG PciBus,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _In_ PPCI_COMMON_HEADER PciConfig)
{
    /* Check if we weren't given a specific device location */
    if (PciDevice->Bus == 0xFFFFFFFF && PciDevice->Slot == 0xFFFFFFFF)
    {
        if (PciDevice->DeviceID == 0xFFFF && PciDevice->VendorID == 0xFFFF)
        {
            if (PciDevice->BaseClass == PciConfig->BaseClass &&
                PciDevice->SubClass == PciConfig->SubClass)
            {
                if (PciDevice->ProgIf == 0xFF ||
                    PciDevice->ProgIf == PciConfig->ProgIf)
                {
                    return TRUE;
                }
            }
        }
        else if (PciDevice->DeviceID == PciConfig->DeviceID &&
                 PciDevice->VendorID == PciConfig->VendorID)
        {
            return TRUE;
        }
    }
    else if (PciDevice->Bus == PciBus &&
             PciDevice->Slot == PciSlot.u.AsULONG)
    {
         return TRUE;
    }

    return FALSE;
}

static
CODE_SEG("INIT")
BOOLEAN
HalpFindMatchingDebuggingDevice(
    _In_ PDEBUG_DEVICE_DESCRIPTOR PciDevice)
{
    ULONG BusNumber, DeviceNumber, FunctionNumber;

    for (BusNumber = 0; BusNumber < 0xFF; ++BusNumber)
    {
        for (DeviceNumber = 0; DeviceNumber < PCI_MAX_DEVICES; ++DeviceNumber)
        {
            for (FunctionNumber = 0; FunctionNumber < PCI_MAX_FUNCTION; ++FunctionNumber)
            {
                ULONG Bytes;
                PCI_SLOT_NUMBER PciSlot;
                PCI_COMMON_HEADER PciConfig;

                PciSlot.u.bits.DeviceNumber = DeviceNumber;
                PciSlot.u.bits.FunctionNumber = FunctionNumber;
                PciSlot.u.bits.Reserved = 0;
                Bytes = HalpPhase0GetPciDataByOffset(BusNumber,
                                                     PciSlot,
                                                     &PciConfig,
                                                     0,
                                                     PCI_COMMON_HDR_LENGTH);
                if (Bytes != PCI_COMMON_HDR_LENGTH ||
                    PciConfig.VendorID == PCI_INVALID_VENDORID ||
                    PciConfig.VendorID == 0)
                {
                    if (FunctionNumber == 0)
                    {
                        /* This slot has no single- or a multi-function device */
                        break;
                    }
                    else
                    {
                        /* Continue scanning the functions */
                        continue;
                    }
                }

                DPRINT0("Check %02x:%02x.%x [%04x:%04x]\n",
                        BusNumber, DeviceNumber, FunctionNumber,
                        PciConfig.VendorID, PciConfig.DeviceID);

                switch (PCI_CONFIGURATION_TYPE(&PciConfig))
                {
                    case PCI_DEVICE_TYPE:
                    {
                        if (HalpMatchDebuggingDevice(PciDevice, BusNumber, PciSlot, &PciConfig))
                        {
                            DPRINT0("Found device\n");

                            if (HalpConfigureDebuggingDevice(PciDevice,
                                                             BusNumber,
                                                             PciSlot,
                                                             &PciConfig))
                            {
                                DPRINT0("Device is ready\n");
                                return TRUE;
                            }
                        }
                        break;
                    }

                    case PCI_BRIDGE_TYPE:
                    {
                        /* FIXME: Implement PCI resource allocator */
                        break;
                    }

                    case PCI_CARDBUS_BRIDGE_TYPE:
                    {
                        /* FIXME: Implement PCI resource allocator */
                        break;
                    }

                    default:
                        break;
                }

                if (!PCI_MULTIFUNCTION_DEVICE(&PciConfig))
                {
                    /* The device is a single function device */
                    break;
                }
            }
        }
    }

    return FALSE;
}

CODE_SEG("INIT")
VOID
NTAPI
HalpRegisterPciDebuggingDeviceInfo(VOID)
{
    ULONG i;
    NTSTATUS Status;
    WCHAR StringBuffer[16];
    BOOLEAN HasDebuggingDevice = FALSE;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\"
                                                 L"CurrentControlSet\\Services\\PCI\\Debug");
    HANDLE Handle, KeyHandle;

    PAGED_CODE();

    for (i = 0; i < RTL_NUMBER_OF(HalpPciDebuggingDevice); ++i)
    {
        if (HalpPciDebuggingDevice[i].InUse)
        {
            HasDebuggingDevice = TRUE;
            break;
        }
    }
    if (!HasDebuggingDevice)
    {
        /* Nothing to register */
        return;
    }

    Status = HalpOpenRegistryKey(&Handle, 0, &KeyName, KEY_ALL_ACCESS, TRUE);
    if (!NT_SUCCESS(Status))
        return;

    for (i = 0; i < RTL_NUMBER_OF(HalpPciDebuggingDevice); ++i)
    {
        ULONG Value;
        PCI_SLOT_NUMBER PciSlot;

        if (!HalpPciDebuggingDevice[i].InUse)
            continue;

        RtlInitEmptyUnicodeString(&KeyName, StringBuffer, sizeof(StringBuffer));
        RtlIntegerToUnicodeString(i, 10, &KeyName);
        Status = HalpOpenRegistryKey(&KeyHandle,
                                     Handle,
                                     &KeyName,
                                     KEY_ALL_ACCESS,
                                     TRUE);
        if (!NT_SUCCESS(Status))
            continue;

        Value = HalpPciDebuggingDevice[i].BusNumber;
        RtlInitUnicodeString(&KeyName, L"Bus");
        ZwSetValueKey(KeyHandle,
                      &KeyName,
                      0,
                      REG_DWORD,
                      &Value,
                      sizeof(Value));

        PciSlot.u.AsULONG = 0;
        PciSlot.u.bits.DeviceNumber = HalpPciDebuggingDevice[i].DeviceNumber;
        PciSlot.u.bits.FunctionNumber = HalpPciDebuggingDevice[i].FunctionNumber;
        Value = PciSlot.u.AsULONG;
        RtlInitUnicodeString(&KeyName, L"Slot");
        ZwSetValueKey(KeyHandle,
                      &KeyName,
                      0,
                      REG_DWORD,
                      &Value,
                      sizeof(Value));

        ZwClose(KeyHandle);
    }

    ZwClose(Handle);
}

/**
 * @brief
 * Releases the PCI device MMIO mappings
 * previously allocated with HalpSetupPciDeviceForDebugging().
 *
 * This is used to release resources when a device specific initialization fails.
 *
 * @param[in,out]   PciDevice
 * Pointer to the debug device descriptor, whose mappings are to be released.
 *
 * @return STATUS_SUCCESS.
 */
CODE_SEG("INIT")
NTSTATUS
NTAPI
HalpReleasePciDeviceForDebugging(
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR PciDevice)
{
    ULONG i;

    DPRINT0("%s(%p) called\n", __FUNCTION__, PciDevice);

    for (i = 0; i < MAXIMUM_DEBUG_BARS; ++i)
    {
        PDEBUG_DEVICE_ADDRESS DeviceAddress = &PciDevice->BaseAddress[i];

        if (DeviceAddress->Type == CmResourceTypeMemory && DeviceAddress->Valid)
        {
            HalpUnmapVirtualAddress(DeviceAddress->TranslatedAddress,
                                    BYTES_TO_PAGES(DeviceAddress->Length));

            DeviceAddress->Valid = FALSE;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Finds and fully initializes the PCI device
 * associated with the supplied debug device descriptor.
 *
 * @param[in]       LoaderBlock
 * Pointer to the Loader parameter block. Can be NULL.
 *
 * @param[in,out]   PciDevice
 * Pointer to the debug device descriptor.
 *
 * @return Status.
 *
 * This routine is used to match devices to debug device descriptors during
 * boot phase of the system. This function will search the first device that
 * matches the criteria given by the fields of the debug device descriptor.
 * A value of all 1's for the field will indicate that the function
 * should ignore that field in the search criteria.
 * The @c Length field of the debug memory requirements optionally specifies
 * library-determined number of bytes to be allocated for the device context.
 *
 * Example:
 * @code
 * RtlZeroMemory(&PciDevice, sizeof(DEBUG_DEVICE_DESCRIPTOR));
 * PciDevice.VendorID = 0xFFFF;
 * PciDevice.DeviceID = 0xFFFF;
 * PciDevice.Bus = 0xFFFFFFFF;
 * PciDevice.Slot = 0xFFFFFFFF;
 * PciDevice.BaseClass = PCI_CLASS_SERIAL_BUS_CTLR;
 * PciDevice.SubClass = PCI_SUBCLASS_SB_USB;
 * PciDevice.ProgIf = 0x30;
 * PciDevice.Memory.Length = sizeof(HW_EXTENSION);
 * @endcode
 *
 * @sa HalpReleasePciDeviceForDebugging
 */
CODE_SEG("INIT")
NTSTATUS
NTAPI
HalpSetupPciDeviceForDebugging(
    _In_opt_ PVOID LoaderBlock,
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR PciDevice)
{
    ULONG i;
    ULONG64 MaxAddress;
    PFN_NUMBER PageCount;
    PCI_SLOT_NUMBER PciSlot;
    PHYSICAL_ADDRESS PhysicalAddress;
    PPCI_TYPE1_CFG_CYCLE_BITS DebuggingDevice;

#if defined(EARLY_DEBUG)
    if (LoaderBlock)
    {
        /* Define your own function or use the trick with FreeLoader */
        DPRINT0 = ((PLOADER_PARAMETER_BLOCK)LoaderBlock)->u.I386.CommonDataArea;
    }
#endif

    DPRINT0("%s(%p, %p) called\n", __FUNCTION__, LoaderBlock, PciDevice);

    if (!HalpFindMatchingDebuggingDevice(PciDevice))
    {
        DPRINT0("No device found matching given device descriptor!\n");
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    if (PciDevice->Initialized)
        return STATUS_SUCCESS;

    PciSlot.u.AsULONG = PciDevice->Slot;

    /* Check if the device is already present */
    for (i = 0; i < RTL_NUMBER_OF(HalpPciDebuggingDevice); ++i)
    {
        DebuggingDevice = &HalpPciDebuggingDevice[i];

        if (DebuggingDevice->InUse &&
            DebuggingDevice->DeviceNumber == PciSlot.u.bits.DeviceNumber &&
            DebuggingDevice->FunctionNumber == PciSlot.u.bits.FunctionNumber &&
            DebuggingDevice->BusNumber == PciDevice->Bus)
        {
            DPRINT0("Device %p(0x%lx) is already in use!\n", PciDevice, PciDevice->Slot);
            return STATUS_UNSUCCESSFUL;
        }
    }

    /* Save the device location */
    for (i = 0; i < RTL_NUMBER_OF(HalpPciDebuggingDevice); ++i)
    {
        DebuggingDevice = &HalpPciDebuggingDevice[i];

        if (!DebuggingDevice->InUse)
        {
            DebuggingDevice->DeviceNumber = PciSlot.u.bits.DeviceNumber;
            DebuggingDevice->FunctionNumber = PciSlot.u.bits.FunctionNumber;
            DebuggingDevice->BusNumber = PciDevice->Bus;
            DebuggingDevice->InUse = TRUE;

            PciDevice->Initialized = TRUE;
            break;
        }
    }
    if (i == RTL_NUMBER_OF(HalpPciDebuggingDevice))
    {
        DPRINT0("Maximum device count reached!\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (!PciDevice->Memory.Length)
        return STATUS_SUCCESS;

    if (!LoaderBlock)
        return STATUS_INVALID_PARAMETER_1;

    if (!PciDevice->Memory.MaxEnd.QuadPart)
    {
        PciDevice->Memory.MaxEnd.QuadPart = (ULONG64)-1;
    }
    MaxAddress = min(PciDevice->Memory.MaxEnd.QuadPart, 0xFFFFFFFF);
    PageCount = BYTES_TO_PAGES(PciDevice->Memory.Length);

    /* Allocate the device context */
    PhysicalAddress.QuadPart = HalpAllocPhysicalMemory(LoaderBlock,
                                                       MaxAddress,
                                                       PageCount,
                                                       FALSE);
    PciDevice->Memory.Start = PhysicalAddress;
    if (!PhysicalAddress.QuadPart)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    PciDevice->Memory.VirtualAddress = HalpMapPhysicalMemory64(PhysicalAddress, PageCount);

    return STATUS_SUCCESS;
}
