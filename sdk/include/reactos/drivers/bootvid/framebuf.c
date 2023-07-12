/*
 * PROJECT:     ReactOS Boot Video Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Boot-time (POST) display discovery helper functions.
 * COPYRIGHT:   Copyright 2023-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <arc/arc.h>   // For CONFIGURATION_COMPONENT*
#include <ndk/kefuncs.h>
#include <drivers/bootvid/framebuf.h>

#if DBG
#define DPRINT_TRACE DPRINT1
#else
#define DPRINT_TRACE(...)
#endif

static BOOLEAN
IsNewConfigData(
    _In_ PVOID ConfigurationData,
    _In_ ULONG ConfigurationDataLength)
{
    /* Cast to PCM_PARTIAL_RESOURCE_LIST to access
     * the common Version and Revision fields */
    PCM_PARTIAL_RESOURCE_LIST ResourceList =
        (PCM_PARTIAL_RESOURCE_LIST)ConfigurationData;

    if (!ConfigurationData)
    {
        DPRINT_TRACE("IsNewConfigData:FALSE - ConfigurationData == NULL\n");
        return FALSE;
    }

    if (ConfigurationDataLength < FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors))
    {
        DPRINT_TRACE("IsNewConfigData:FALSE - ConfigurationDataLength == %lu\n",
                ConfigurationDataLength);
        return FALSE;
    }

    /* If Version/Revision is strictly lower than 1.2, this cannot be
     * a new configuration data (even if the length appears to match
     * a CM_FULL_RESOURCE_DESCRIPTOR with zero or more descriptors) */
    if ( (ResourceList->Version <  1) ||
        ((ResourceList->Version == 1) && (ResourceList->Revision <= 1)) )
    {
        DPRINT_TRACE("IsNewConfigData:FALSE - Version %lu, Revision %lu\n",
                ResourceList->Version, ResourceList->Revision);
        return FALSE;
    }

    /* This should be a new configuration data */
    DPRINT_TRACE("IsNewConfigData:TRUE\n");
    return TRUE;
}

/**
 * @brief
 * Given a CM_PARTIAL_RESOURCE_LIST, obtain pointers to resource descriptors
 * for legacy video configuration: interrupt, control and cursor I/O ports,
 * and video RAM memory descriptors. In addition, retrieve any device-specific
 * resource present.
 **/
static VOID
GetVideoData(
    _In_ PCM_PARTIAL_RESOURCE_LIST ResourceList,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* Interrupt,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* ControlPort,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* CursorPort,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* VideoRam,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* DeviceSpecific)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG PortCount = 0, IntCount = 0, MemCount = 0;
    ULONG i;

    /* Initialize the return values */
    if (Interrupt)   *Interrupt   = NULL;
    if (ControlPort) *ControlPort = NULL;
    if (CursorPort)  *CursorPort  = NULL;
    *VideoRam = NULL;
    if (DeviceSpecific) *DeviceSpecific = NULL;

    DPRINT_TRACE("GetVideoData\n");

    for (i = 0; i < ResourceList->Count; ++i)
    {
        Descriptor = &ResourceList->PartialDescriptors[i];

        switch (Descriptor->Type)
        {
            case CmResourceTypePort:
            {
                DPRINT_TRACE("    CmResourceTypePort\n");
                /* We only check for memory I/O ports */
                // if (!(Descriptor->Flags & CM_RESOURCE_PORT_MEMORY))
                if (Descriptor->Flags & CM_RESOURCE_PORT_IO)
                    break;

                /* If more than two memory I/O ports
                 * have been encountered, ignore them */
                if (PortCount > 2)
                    break;
                ++PortCount;

                /* First port is Control; second port is Cursor */
                if (PortCount == 1)
                {
                    if (ControlPort)
                        *ControlPort = Descriptor;
                }
                else // if (PortCount == 2)
                {
                    if (CursorPort)
                        *CursorPort = Descriptor;
                }
                break;
            }

            case CmResourceTypeInterrupt:
            {
                DPRINT_TRACE("    CmResourceTypeInterrupt\n");
                /* If more than one interrupt resource
                 * has been encountered, ignore them */
                if (IntCount > 1)
                    break;
                ++IntCount;

                if (Interrupt)
                    *Interrupt = Descriptor;
                break;
            }

            case CmResourceTypeMemory:
            {
                DPRINT_TRACE("    CmResourceTypeMemory\n");
                /* If more than one memory resource
                 * has been encountered, ignore them */
                if (MemCount > 1)
                    break;
                ++MemCount;

                /* Video RAM should be writable (but may or may not be readable) */
                if ((Descriptor->Flags & CM_RESOURCE_MEMORY_WRITEABILITY_MASK)
                        == CM_RESOURCE_MEMORY_READ_ONLY)
                {
                    break; /* Cannot use this memory */
                }
                *VideoRam = Descriptor;
                break;
            }

            case CmResourceTypeDeviceSpecific:
            {
                DPRINT_TRACE("    CmResourceTypeDeviceSpecific\n");
                /* NOTE: This descriptor *MUST* be the last one.
                 * The actual device data follows the descriptor. */
                ASSERT(i == ResourceList->Count - 1);
                i = ResourceList->Count; // To force-break the for-loop.

                if (DeviceSpecific)
                    *DeviceSpecific = Descriptor;
                break;
            }
        }
    }
}

/**
 * @brief
 * Given a CM_PARTIAL_RESOURCE_LIST, obtain a pointer to resource descriptor
 * for monitor configuration data, listed as a device-specific resource.
 **/
static VOID
GetMonitorData(
    _In_ PCM_PARTIAL_RESOURCE_LIST ResourceList,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* DeviceSpecific)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG i;

    /* Initialize the return values */
    *DeviceSpecific = NULL;

    /* Find the CmResourceTypeDeviceSpecific CM_MONITOR_DEVICE_DATA */
    for (i = 0; i < ResourceList->Count; ++i)
    {
        Descriptor = &ResourceList->PartialDescriptors[i];
        if (Descriptor->Type == CmResourceTypeDeviceSpecific)
        {
            /* NOTE: This descriptor *MUST* be the last one.
             * The actual device data follows the descriptor. */
            ASSERT(i == ResourceList->Count - 1);

            if (DeviceSpecific)
                *DeviceSpecific = Descriptor;
            break;
        }
    }
}

NTSTATUS
GetFramebufferVideoData(
    _Out_ PPHYSICAL_ADDRESS VideoRamAddress,
    _Out_ PULONG VideoRamSize,
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _In_ PVOID ConfigurationData,
    _In_ ULONG ConfigurationDataLength)
{
    if (!ConfigurationData ||
        (ConfigurationDataLength < sizeof(CM_PARTIAL_RESOURCE_LIST)))
    {
        /* Invalid entry */
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    /* Initialize the display adapter parameters, converting
     * them to the new format if needed */
    // TODO: Handle legacy VIDEO_HARDWARE_CONFIGURATION_DATA based data
    if (IsNewConfigData(ConfigurationData, ConfigurationDataLength))
    {
        /* New configuration data */
        PCM_PARTIAL_RESOURCE_DESCRIPTOR VideoRam, Descriptor;

        DPRINT_TRACE("**  FbBVid: New Config data found\n");
        GetVideoData((PCM_PARTIAL_RESOURCE_LIST)ConfigurationData,
                     NULL, // Interrupt
                     NULL, // ControlPort
                     NULL, // CursorPort
                     &VideoRam,
                     &Descriptor);

        if (VideoRam)
        {
            /* Save the video RAM base and size */
            *VideoRamAddress = VideoRam->u.Memory.Start;
            *VideoRamSize = VideoRam->u.Memory.Length;
            DPRINT_TRACE("**  FbBVid: Got video RAM address: 0x%I64X - Size: %lu\n",
                VideoRamAddress->QuadPart, *VideoRamSize);
        }
        else
        {
            /* No video RAM base: zero it out */
            DPRINT1("**  FbBVid: No video RAM available\n");
            VideoRamAddress->QuadPart = 0;
            *VideoRamSize = 0;
        }

        if (Descriptor &&
            (Descriptor->u.DeviceSpecificData.DataSize >= sizeof(CM_FRAMEBUF_DEVICE_DATA)))
        {
            /* NOTE: This descriptor *MUST* be the last one.
             * The actual device data follows the descriptor. */
            PCM_FRAMEBUF_DEVICE_DATA VideoData = (PCM_FRAMEBUF_DEVICE_DATA)(Descriptor + 1);

            /* Just copy the data */
            *VideoConfigData = *VideoData;

            DPRINT_TRACE("**  FbBVid: Framebuffer at: 0x%I64X\n",
                VideoRamAddress->QuadPart + VideoData->FrameBufferOffset);
        }
        else
        {
            /* The configuration data does not contain any
             * framebuffer information (format, etc.)
             * We will calculate default values later. */
            DPRINT1("**  FbBVid: Framebuffer data NOT FOUND\n");
        }

        return STATUS_SUCCESS;
    }
    else
    {
        /* Unknown configuration, invalid entry? */
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

#if 0
    /* Fail if no video is available */
    if ((VideoRamAddress->QuadPart == 0) || (*VideoRamSize == 0))
    {
        DPRINT1("No video available!\n");
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }
#endif

    return STATUS_SUCCESS;
}

NTSTATUS
GetFramebufferMonitorData(
    _Out_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _In_ PVOID ConfigurationData,
    _In_ ULONG ConfigurationDataLength)
{
    if (!ConfigurationData ||
        (ConfigurationDataLength < sizeof(CM_PARTIAL_RESOURCE_LIST)))
    {
        /* Invalid entry */
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    /* Initialize the monitor parameters, converting
     * them to the new format if needed */
    // TODO: Handle legacy MONITOR_CONFIGURATION_DATA
    if (IsNewConfigData(ConfigurationData, ConfigurationDataLength))
    {
        /* New configuration data */
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

        GetMonitorData((PCM_PARTIAL_RESOURCE_LIST)ConfigurationData,
                       &Descriptor);

        if (Descriptor &&
            (Descriptor->u.DeviceSpecificData.DataSize >= sizeof(CM_MONITOR_DEVICE_DATA)))
        {
            /* NOTE: This descriptor *MUST* be the last one.
             * The actual device data follows the descriptor. */
            PCM_MONITOR_DEVICE_DATA MonitorData = (PCM_MONITOR_DEVICE_DATA)(Descriptor + 1);

            /* Just copy the data */
            *MonitorConfigData = *MonitorData;
        }
        else
        {
            /* The configuration data does not contain any monitor information.
             * We will calculate default values later. */
        }

        return STATUS_SUCCESS;
    }

    /* Unknown configuration, invalid entry? */
    return STATUS_DEVICE_CONFIGURATION_ERROR;
}


/**
 * @brief
 * Retrieve the NT interface type and bus number of a given ARC configuration
 * component, that can be used in a call to HalTranslateBusAddress() or to
 * BootTranslateBusAddress().
 **/
static INTERFACE_TYPE
GetArcComponentInterface(
    _In_ PCONFIGURATION_COMPONENT Component,
    _Out_opt_ PULONG BusNumber)
{
    static const struct
    {
        PCSTR Identifier;
        INTERFACE_TYPE InterfaceType;
        // USHORT Count;
    } CmpMultifunctionTypes[] =
    {
        {"ISA", Isa},
        {"MCA", MicroChannel},
        {"PCI", PCIBus},
        {"VME", VMEBus},
        {"PCMCIA", PCMCIABus},
        {"CBUS", CBus},
        {"MPIPI", MPIBus},
        {"MPSA", MPSABus},
        {NULL, Internal}
    };

    INTERFACE_TYPE Interface = InterfaceTypeUndefined;

    /* Retrieve the top-level parent adapter component */
    PCONFIGURATION_COMPONENT_DATA ComponentData =
        CONTAINING_RECORD(Component, CONFIGURATION_COMPONENT_DATA, ComponentEntry);
    for (; ComponentData->Parent; ComponentData = ComponentData->Parent)
    {
        if (ComponentData->Parent->ComponentEntry.Class == SystemClass)
            break;
    }
    Component = &ComponentData->ComponentEntry;

    /* Check if this is an adapter */
    if (!ComponentData->Parent ||
        (ComponentData->Parent->ComponentEntry.Class != SystemClass) ||
        (Component->Class != AdapterClass))
    {
        return Interface; /* It's not, return early */
    }

    /* Check what kind of adapter it is */
    switch (Component->Type)
    {
        /* EISA */
        case EisaAdapter:
        {
            Interface = Eisa;
            // Bus = CmpTypeCount[EisaAdapter]++;
            break;
        }

        /* Turbo-channel */
        case TcAdapter:
        {
            Interface = TurboChannel;
            // Bus = CmpTypeCount[TurboChannel]++;
            break;
        }

        /* ISA, PCI, etc buses */
        case MultiFunctionAdapter:
        {
            /* Check if we have an identifier */
            if (Component->Identifier)
            {
                /* Loop each multi-function adapter type */
                ULONG i;
                for (i = 0; CmpMultifunctionTypes[i].Identifier; i++)
                {
                    /* Check for a name match */
                    if (!_stricmp(CmpMultifunctionTypes[i].Identifier,
                                  Component->Identifier))
                    {
                        /* Match found */
                        break;
                    }
                }

                Interface = CmpMultifunctionTypes[i].InterfaceType;
                // Bus = CmpMultifunctionTypes[i].Count++;
            }
            break;
        }

        /* SCSI Bus */
        case ScsiAdapter:
        {
            Interface = Internal;
            // Bus = CmpTypeCount[ScsiAdapter]++;
            break;
        }

        /* Unknown */
        default:
        {
            Interface = InterfaceTypeUndefined;
            // Bus = CmpUnknownBusCount++;
            break;
        }
    }

    if (BusNumber)
        *BusNumber = Component->Key;
    return Interface;
}

static NTSTATUS
FindBootDisplayFromLoaderARCTree(
    _Out_ PPHYSICAL_ADDRESS VideoRamAddress,
    _Out_ PULONG VideoRamSize,
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_opt_ PINTERFACE_TYPE Interface,
    _Out_opt_ PULONG BusNumber)
{
    PCONFIGURATION_COMPONENT_DATA ConfigurationRoot;
    PCONFIGURATION_COMPONENT_DATA Entry = NULL;
    // ULONG ComponentKey = 0; // First controller.
    NTSTATUS Status;

    ConfigurationRoot = (KeLoaderBlock ? KeLoaderBlock->ConfigurationRoot : NULL);
    if (!ConfigurationRoot)
        return STATUS_DEVICE_DOES_NOT_EXIST;

    /* Enumerate and find the boot-time console display controller */
#if 0
    Entry = KeFindConfigurationEntry(ConfigurationRoot,
                                     ControllerClass,
                                     DisplayController,
                                     &ComponentKey);
#else
    while (TRUE)
    {
        Entry = KeFindConfigurationNextEntry(ConfigurationRoot,
                                             ControllerClass,
                                             DisplayController,
                                             NULL, // &ComponentKey
                                             &Entry);
        if (!Entry)
            break; /* Not found */
        if (Entry->ComponentEntry.Flags & (Output | ConsoleOut))
            break; /* Found it */
    }
#endif

    if (!Entry)
        return STATUS_DEVICE_DOES_NOT_EXIST;

    // Entry->ComponentEntry.IdentifierLength;
    DPRINT_TRACE("Display: '%s'\n", Entry->ComponentEntry.Identifier);

    Status = GetFramebufferVideoData(VideoRamAddress,
                                     VideoRamSize,
                                     VideoConfigData,
                                     Entry->ConfigurationData,
                                     Entry->ComponentEntry.ConfigurationDataLength);
    if (!NT_SUCCESS(Status) ||
        (VideoRamAddress->QuadPart == 0) || (*VideoRamSize == 0))
    {
        /* Fail if no framebuffer was provided */
        DPRINT1("No framebuffer found!\n");
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    if (Interface)
        *Interface = GetArcComponentInterface(&Entry->ComponentEntry, BusNumber);
    //if (BusNumber)
    //    *BusNumber = -1;

    /* If no monitor data to retrieve, just return success */
    if (!MonitorConfigData)
        return STATUS_SUCCESS;

    /* Now find the MonitorPeripheral to obtain more information.
     * It should be a child of the display controller. */
    Entry = Entry->Child;
    /* Ignore if no monitor data is given */
    if (!Entry)
        return STATUS_SUCCESS;

    // Entry->ComponentEntry.IdentifierLength;
    DPRINT_TRACE("Monitor: '%s'\n", Entry->ComponentEntry.Identifier);

    if ((Entry->ComponentEntry.Class != PeripheralClass)   ||
        (Entry->ComponentEntry.Type  != MonitorPeripheral) ||
       !(Entry->ComponentEntry.Flags & (Output | ConsoleOut)))
    {
        /* Ignore */
        return STATUS_SUCCESS;
    }

    /* Retrieve monitor configuration data. Use the local variable
     * since we may need to do some adjustments in the video data
     * using monitor data, even if the caller does not require the
     * monitor data itself to be returned. */
    Status = GetFramebufferMonitorData(MonitorConfigData,
                                       Entry->ConfigurationData,
                                       Entry->ComponentEntry.ConfigurationDataLength);
    if (!NT_SUCCESS(Status))
        DPRINT_TRACE("Invalid monitor data, ignoring.\n");

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Retrieves configuration data for the boot-time (POST) display controller
 * and monitor peripheral.
 *
 * This data is initialized by the firmware and the NT bootloader,
 * and can be retrieved from the loader block configuration ARC tree.
 *
 * @note
 * Data from the loader block is available only for boot
 * drivers and gets freed later on during NT loading.
 **/
NTSTATUS
FindBootDisplay(
    _Out_ PPHYSICAL_ADDRESS VideoRamAddress,
    _Out_ PULONG VideoRamSize,
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_opt_ PINTERFACE_TYPE Interface,
    _Out_opt_ PULONG BusNumber)
{
    CM_MONITOR_DEVICE_DATA LocalMonitorConfigData = {0};
    INTERFACE_TYPE LocalInterface;
    ULONG LocalBusNumber;
    NTSTATUS Status;

    PAGED_CODE();

    /*
     * Retrieve video and monitor configuration data.
     * For monitor data, use the local variable since we may need
     * to do some adjustments in the video data using monitor data,
     * even if the caller does not require the monitor data itself
     * to be returned.
     */
    Status = FindBootDisplayFromLoaderARCTree(VideoRamAddress,
                                              VideoRamSize,
                                              VideoConfigData,
                                              &LocalMonitorConfigData,
                                              &LocalInterface,
                                              &LocalBusNumber);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Boot Display not found\n");
        return STATUS_DEVICE_DOES_NOT_EXIST; // STATUS_SYSTEM_DEVICE_NOT_FOUND; (Vista+)
    }

#if DBG
    DPRINT_TRACE("\n");
    DbgPrint("Boot Display found on Interface %lu, Bus %lu\n",
             LocalInterface, LocalBusNumber);
    DbgPrint("    VideoRamAddress   : 0x%I64X\n", VideoRamAddress->QuadPart);
    DbgPrint("    VideoRamSize      : %lu\n", *VideoRamSize);
    DbgPrint("    Version           : %u.%u\n", VideoConfigData->Version, VideoConfigData->Revision);
    DbgPrint("    VideoClock        : %lu\n", VideoConfigData->VideoClock);
    DbgPrint("Framebuffer format:\n");
    /* Absolute offset from the start of the video RAM of the framebuffer
     * to be displayed on the monitor. The framebuffer size is obtained by:
     * FrameBufferSize = ScreenHeight * PixelsPerScanLine * BytesPerPixel */
    DbgPrint("    BaseAddress       : 0x%I64X\n", VideoRamAddress->QuadPart + VideoConfigData->FrameBufferOffset);
    // DbgPrint("    BufferSize        : %lu\n", framebufInfo.BufferSize);
    DbgPrint("    ScreenWidth       : %lu\n", VideoConfigData->ScreenWidth);
    DbgPrint("    ScreenHeight      : %lu\n", VideoConfigData->ScreenHeight);
    DbgPrint("    PixelsPerScanLine : %lu\n", VideoConfigData->PixelsPerScanLine);
    DbgPrint("    BitsPerPixel      : %lu\n", VideoConfigData->BitsPerPixel);
    DbgPrint("    ARGB masks:       : %08x/%08x/%08x/%08x\n",
             VideoConfigData->PixelMasks.ReservedMask,
             VideoConfigData->PixelMasks.RedMask,
             VideoConfigData->PixelMasks.GreenMask,
             VideoConfigData->PixelMasks.BlueMask);
#endif

    /* If the screen sizes are not already initialized by now, use monitor data */
    // TODO: Investigate: Do we want to do this here, or in the caller?
    if ((VideoConfigData->ScreenWidth == 0) || (VideoConfigData->ScreenHeight == 0))
    {
        VideoConfigData->ScreenWidth  = LocalMonitorConfigData.HorizontalResolution;
        VideoConfigData->ScreenHeight = LocalMonitorConfigData.VerticalResolution;
#if DBG
        DbgPrint("Video screen dimensions not defined; use monitor resolution\n");
        DbgPrint("    ScreenWidth       : %lu\n", VideoConfigData->ScreenWidth);
        DbgPrint("    ScreenHeight      : %lu\n", VideoConfigData->ScreenHeight);
#endif
    }

    /* Return any optional data to the caller if required */
    if (MonitorConfigData)
        *MonitorConfigData = LocalMonitorConfigData;
    if (Interface)
        *Interface = LocalInterface;
    if (BusNumber)
        *BusNumber = LocalBusNumber;

    if ((VideoConfigData->ScreenWidth <= 1) || (VideoConfigData->ScreenHeight <= 1))
    {
        DPRINT1("Cannot obtain current screen resolution\n");
        /* Don't fail, but the caller will have to cope with it */
    }

    return STATUS_SUCCESS;
}


/**
 * @brief
 * Wrapper around HalTranslateBusAddress() and HALPRIVATEDISPATCH->HalFindBusAddressTranslation().
 *
 * @note
 * Context parameter is supplemental from standard HalTranslateBusAddress().
 * It allows context passing for the underlying HalFindBusAddressTranslation()
 * between different calls of the function.
 **/
#if 0
typedef BOOLEAN
(NTAPI *pHalFindBusAddressTranslation)(
    _In_ PHYSICAL_ADDRESS BusAddress,
    _Inout_ PULONG AddressSpace,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress,
    _Inout_ PULONG_PTR Context,
    _In_ BOOLEAN NextBus);
#else
#include <ndk/halfuncs.h>
#endif

BOOLEAN
NTAPI
BootTranslateBusAddress(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ PHYSICAL_ADDRESS BusAddress,
    _Inout_ PULONG AddressSpace,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress/*,
    _Inout_ PULONG_PTR Context*/)
{
    ULONG_PTR Context = 0;

    /* If InterfaceType is negative, don't do any translation */
    if (InterfaceType <= InterfaceTypeUndefined)
    {
        /* Return the bus address */
        TranslatedAddress->QuadPart = BusAddress.QuadPart;
        return TRUE;
    }

    /* If InterfaceType is valid, attempt to call the original HAL function */
    if (InterfaceType < MaximumInterfaceType)
    {
        if (HalTranslateBusAddress(InterfaceType,
                                   BusNumber,
                                   BusAddress,
                                   AddressSpace,
                                   TranslatedAddress))
        {
            return TRUE;
        }
        DPRINT1("HalTranslateBusAddress(%lu, %lu, 0x%I64X) failed, fall back to finding a bus translation\n",
                InterfaceType, BusNumber, BusAddress.QuadPart);
    }

    /* Otherwise, in case of failure, or if InterfaceType is greater than
     * the maximum supported type (meaning we don't know yet the interface
     * and bus number), we need to find a bus translation. */

    /* Ensure we have a bus translation function then do the call */
    if (!HalFindBusAddressTranslation)
        return FALSE;
    return HalFindBusAddressTranslation(BusAddress,
                                        AddressSpace,
                                        TranslatedAddress,
                                        &Context,
                                        TRUE);
}

/* EOF */
