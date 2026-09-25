/*
 * PROJECT:     ReactOS Boot Video Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Boot-time (POST) display discovery helper functions.
 * COPYRIGHT:   Copyright 2023-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/**
 * Use the following defines to include or exclude support
 * for discovery by API or via loader block information.
 *
 * #define GET_DISPLAY_BY_API
 * #define GET_DISPLAY_BY_LDR_BLOCK
 **/
// If none have been defined by the user, define them all.
#if !defined(GET_DISPLAY_BY_API) && !defined(GET_DISPLAY_BY_LDR_BLOCK)
#define GET_DISPLAY_BY_API
#define GET_DISPLAY_BY_LDR_BLOCK
#endif


#include <arc/arc.h>   // For CONFIGURATION_COMPONENT*

#ifdef GET_DISPLAY_BY_LDR_BLOCK
#include <ndk/kefuncs.h>
#endif

//#include <drivers/bootvid/framebuf.h> // FIXME: Include?
#include <drivers/videoprt/vpcfgcnv.h>


#if DBG
#define DPRINT_TRACE DPRINT1
#else
#define DPRINT_TRACE(...)
#endif

#if 0 // Moved to vpcfgcnv.h
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
#endif // Moved to vpcfgcnv.h

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
    if (IsLegacyConfigData(ConfigurationData, ConfigurationDataLength,
                           sizeof(VIDEO_CONFIGURATION_DATA)))
    {
        /* Legacy configuration data, convert it into new format */
        CM_PARTIAL_RESOURCE_DESCRIPTOR FrameBuffer;

        DPRINT_TRACE("**  FbBVid: Legacy Config data found\n");
        ConvertLegacyVideoConfigDataToDeviceData(
            ConfigurationData,
        #if 0
            &(VideoConfigData->Version),
            &(VideoConfigData->Revision),
        #endif
            NULL, // Interrupt
            NULL, // ControlPort
            NULL, // CursorPort
            &FrameBuffer);

        /* Save the framebuffer base and size */
        *VideoRamAddress = FrameBuffer.u.Memory.Start;
        *VideoRamSize = FrameBuffer.u.Memory.Length;

        DPRINT_TRACE("**  FbBVid: Legacy video RAM address (32-bit only): 0x%X - Size: %lu\n",
            VideoRamAddress->LowPart, *VideoRamSize);

        /* The legacy video controller configuration data does not
         * contain any framebuffer information (format, etc.)
         * We will calculate default values later. */
        return STATUS_SUCCESS;
    }
    else if (IsNewConfigData(ConfigurationData, ConfigurationDataLength))
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
    if (IsLegacyConfigData(ConfigurationData, ConfigurationDataLength,
                           sizeof(MONITOR_CONFIGURATION_DATA)))
    {
        /* Legacy configuration data, convert it into new format */
        ConvertLegacyMonitorConfigDataToDeviceData(
            ConfigurationData, MonitorConfigData);

        return STATUS_SUCCESS;
    }
    else if (IsNewConfigData(ConfigurationData, ConfigurationDataLength))
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


#if (NTDDI_VERSION >= NTDDI_WIN8)

#ifdef GET_DISPLAY_BY_API

/*
 * Some information about boot-time (POST) display controller data retrieval
 * on Windows 8+:
 *
 * ZwQuerySystemInformation(90, &data, sizeof(data) \* 32 *\);
 *
 * See https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/ex/sysinfo/boot_graphics.htm
 *
 * SystemBootGraphicsInformation = 0x7e, // 126
 *
 * BgkQueryBootGraphicsInformation(0, ...) == SystemBootGraphicsInformation
 * BgkQueryBootGraphicsInformation(1 and 2, ...) == SystemBootLogoInformation
 * BgkQueryBootGraphicsInformation(3, ...) == SystemEdidInformation
 *
 */

typedef enum _SYSTEM_PIXEL_FORMAT
{
    SystemPixelFormatUnknown,
    SystemPixelFormatR8G8B8,
    SystemPixelFormatR8G8B8X8,
    SystemPixelFormatB8G8R8,
    SystemPixelFormatB8G8R8X8
} SYSTEM_PIXEL_FORMAT;

typedef struct _SYSTEM_BOOT_GRAPHICS_INFORMATION
{
    PHYSICAL_ADDRESS PhysicalFrameBuffer; // Geoff Chappell uses LARGE_INTEGER FrameBuffer, but it's not accurate.
    ULONG Width;
    ULONG Height;
    ULONG PixelStride;
    ULONG Flags;
    SYSTEM_PIXEL_FORMAT Format;
    ULONG DisplayRotation;
} SYSTEM_BOOT_GRAPHICS_INFORMATION, *PSYSTEM_BOOT_GRAPHICS_INFORMATION;

// The only bit that is defined for the Flags is 0x00000001.
// It is set while boot graphics are enabled.

static
NTSTATUS
FindBootDisplayFromAPI(
    _Out_ PPHYSICAL_ADDRESS VideoRamAddress,
    _Out_ PULONG VideoRamSize,
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_opt_ PINTERFACE_TYPE Interface,
    _Out_opt_ PULONG BusNumber)
{
    NTSTATUS Status;
    SYSTEM_BOOT_GRAPHICS_INFORMATION BootGraphicsInfo;

    PAGED_CODE();

    /* Retrieve the boot graphics information currently in use by the kernel */
    Status = ZwQuerySystemInformation(SystemBootGraphicsInformation,
                                      &BootGraphicsInfo,
                                      sizeof(BootGraphicsInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Not found, bail out */
        return Status;
    }

    /* Found it! */
    *VideoRamAddress = BootGraphicsInfo.PhysicalFrameBuffer; // FrameBuffer;
    // Buffer size determined below after PixelsPerScanLine and BitsPerPixel set.

    /* Horizontal and Vertical resolution in pixels */
    VideoConfigData->ScreenWidth  = BootGraphicsInfo.Width;
    VideoConfigData->ScreenHeight = BootGraphicsInfo.Height;

    /* Number of pixel elements per video memory line */
    VideoConfigData->PixelsPerScanLine = BootGraphicsInfo.PixelStride;

    //
    // TODO: Investigate display rotation!
    //
    // See OpenCorePkg OcConsoleLib/ConsoleGop.c
    // if ((BootGraphicsInfo.DisplayRotation == 90) ||
    //     (BootGraphicsInfo.DisplayRotation == 270))
    if (VideoConfigData->ScreenWidth < VideoConfigData->ScreenHeight)
    {
        #define SWAP(x, y) { (x) ^= (y); (y) ^= (x); (x) ^= (y); }
        SWAP(VideoConfigData->ScreenWidth, VideoConfigData->ScreenHeight);
        VideoConfigData->PixelsPerScanLine = VideoConfigData->ScreenWidth;
        #undef SWAP
    }
    // FIXME: Should we somehow return BootGraphicsInfo.DisplayRotation to the caller?

    /* Physical format of the pixel */
    switch (BootGraphicsInfo.Format)
    {
        case SystemPixelFormatR8G8B8X8: // PixelRedGreenBlueReserved8BitPerColor:
        case SystemPixelFormatB8G8R8X8: // PixelBlueGreenRedReserved8BitPerColor:
        {
            VideoConfigData->BitsPerPixel = (8 * sizeof(ULONG));
            VideoConfigData->PixelInformation = EfiPixelMasks[BootGraphicsInfo.Format/2-1];
            break;
        }

        case SystemPixelFormatR8G8B8:
        case SystemPixelFormatB8G8R8:
        {
            VideoConfigData->BitsPerPixel = (8 * (sizeof(ULONG) - 1));
            VideoConfigData->PixelInformation = EfiPixelMasks[BootGraphicsInfo.Format/2];
            VideoConfigData->PixelInformation.ReservedMask = 0; // No Reserved channel here.
            break;
        }

        case SystemPixelFormatUnknown:
        default:
        {
            ERR("Unsupported BGFX format %lu\n", BootGraphicsInfo.Format);
            VideoConfigData->BitsPerPixel = 0;
            RtlZeroMemory(&VideoConfigData->PixelInformation,
                          sizeof(VideoConfigData->PixelInformation));
            break;
        }
    }

    /* Now set the framebuffer size */
    *VideoRamSize = VideoConfigData->PixelsPerScanLine *
                    VideoConfigData->ScreenHeight *
                    (VideoConfigData->BitsPerPixel / 8);

    // FIXME: What to do about BootGraphicsInfo.Flags ?

    VideoConfigData->VideoClock = 0; // FIXME: Use EDID

    /* No bus interface used there */
    if (Interface)
        *Interface = InterfaceTypeUndefined;
    if (BusNumber)
        *BusNumber = -1; // or 0 ?

    /* If no monitor data to retrieve, just return success */
    if (!MonitorConfigData)
        return STATUS_SUCCESS;

    // FIXME: No monitor data to retrieve?
    UNREFERENCED_PARAMETER(MonitorConfigData);
#if 0
    /* Retrieve the EDID of the boot console monitor */
    Status = ZwQuerySystemInformation(SystemEdidInformation,
                                      &BootEdidInfo,
                                      sizeof(BootEdidInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Not found, just ignore */
    }
#endif

    return STATUS_SUCCESS;
}

#endif // GET_DISPLAY_BY_API

#ifdef GET_DISPLAY_BY_LDR_BLOCK

typedef struct _BOOT_GRAPHICS_CONTEXT
{
    ULONG ResidentSize;     //< Size of the whole BOOT_GRAPHICS_CONTEXT structure.
    UCHAR Unknown[0x50];    // FIXME: TODO Find what this is and fix size!
} BOOT_GRAPHICS_CONTEXT, *PBOOT_GRAPHICS_CONTEXT;

static
NTSTATUS
FindBootDisplayFromLoaderBGFX(
    _Out_ PPHYSICAL_ADDRESS VideoRamAddress,
    _Out_ PULONG VideoRamSize,
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_opt_ PINTERFACE_TYPE Interface,
    _Out_opt_ PULONG BusNumber)
{
    PLOADER_PARAMETER_EXTENSION Extension;

    if (!KeLoaderBlock)
        return STATUS_NO_SUCH_DEVICE;

    /* Query the boot graphics memory pointer used by the kernel */
    Extension = KeLoaderBlock->Extension;
    if (Extension && Extension->Size >= sizeof())
    {
        (PBOOT_GRAPHICS_CONTEXT)Extension->BgContext;
    }

    /* No bus interface used there */
    if (Interface)
        *Interface = InterfaceTypeUndefined;
    if (BusNumber)
        *BusNumber = -1; // or 0 ?

    /* If no monitor data to retrieve, just return success */
    if (!MonitorConfigData)
        return STATUS_SUCCESS;

    // FIXME: No monitor data to retrieve?
    // TODO: In some versions of the structure, one may
    // find the EDID of the corresponding monitor...
    UNREFERENCED_PARAMETER(MonitorConfigData);

    return STATUS_SUCCESS;
}

#endif // GET_DISPLAY_BY_LDR_BLOCK

#endif // (NTDDI_VERSION >= NTDDI_WIN8)


#ifdef GET_DISPLAY_BY_LDR_BLOCK

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

#endif // GET_DISPLAY_BY_LDR_BLOCK


#ifdef GET_DISPLAY_BY_API

typedef struct _DISPLAY_CONTEXT
{
    /* Input parameters */
    CONFIGURATION_TYPE DeviceType; // Which device type to search for.

    /* Output data */
    PHYSICAL_ADDRESS BaseAddress;
    ULONG BufferSize;
    INTERFACE_TYPE Interface;
    ULONG BusNumber;

    /* In input, contain pointers to the structures.
     * These are initialized on output. */
    PCM_FRAMEBUF_DEVICE_DATA VideoConfigData;
    PCM_MONITOR_DEVICE_DATA MonitorConfigData;
} DISPLAY_CONTEXT, *PDISPLAY_CONTEXT;

/**
 * @brief
 * A PIO_QUERY_DEVICE_ROUTINE callback for IoQueryDeviceDescription()
 * to return success when an enumerated display controller has been found.
 **/
static NTSTATUS
NTAPI
pEnumDisplayControllerCallback(
    _In_ PVOID Context,
    _In_ PUNICODE_STRING PathName,
    _In_ INTERFACE_TYPE BusType,
    _In_ ULONG BusNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION* BusInformation,
    _In_ CONFIGURATION_TYPE ControllerType,
    _In_ ULONG ControllerNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION* ControllerInformation,
    _In_ CONFIGURATION_TYPE PeripheralType,
    _In_ ULONG PeripheralNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION* PeripheralInformation)
{
#define GetDeviceInfoData(info) \
    ((info) ? (PVOID)((ULONG_PTR)(info) + (info)->DataOffset) : NULL)

#define GetDeviceInfoLength(info) \
    ((info) ? (info)->DataLength : 0)

    PDISPLAY_CONTEXT DisplayContext = Context;
    PKEY_VALUE_FULL_INFORMATION* DeviceInformation;
    PWCHAR Identifier;
    ULONG IdentifierLength;
    PVOID ConfigurationData;
    ULONG ConfigurationDataLength;
    PCM_COMPONENT_INFORMATION CompInfo;
    ULONG CompInfoLength;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(PathName);
    UNREFERENCED_PARAMETER(BusInformation);
    UNREFERENCED_PARAMETER(ControllerType);
    UNREFERENCED_PARAMETER(ControllerNumber);
    UNREFERENCED_PARAMETER(PeripheralType);
    UNREFERENCED_PARAMETER(PeripheralNumber);

    /* The display controller has been found */
    DPRINT1("pEnumDisplayControllerCallback():\n"
            "    PathName:   '%wZ'\n"
            "    BusType:    %lu\n"
            "    BusNumber:  %lu\n"
            "    CtrlType:   %lu\n"
            "    CtrlNumber: %lu\n"
            "    PeriType:   %lu\n"
            "    PeriNumber: %lu\n"
            "\n",
            PathName, BusType, BusNumber,
            ControllerType, ControllerNumber,
            PeripheralType, PeripheralNumber);

    /* Capture information and return it via Context */

    switch (DisplayContext->DeviceType)
    {
        case DisplayController:
        {
            ASSERT(ControllerInformation);

            /* Retrieve the pointers */
            DeviceInformation = ControllerInformation;
            ConfigurationData =
                GetDeviceInfoData(DeviceInformation[IoQueryDeviceConfigurationData]);
            ConfigurationDataLength =
                GetDeviceInfoLength(DeviceInformation[IoQueryDeviceConfigurationData]);

            CompInfo =
                GetDeviceInfoData(DeviceInformation[IoQueryDeviceComponentInformation]);
            CompInfoLength =
                GetDeviceInfoLength(DeviceInformation[IoQueryDeviceComponentInformation]);

            Identifier =
                GetDeviceInfoData(DeviceInformation[IoQueryDeviceIdentifier]);
            IdentifierLength =
                GetDeviceInfoLength(DeviceInformation[IoQueryDeviceIdentifier]);

            DPRINT1("Display: '%.*ws'\n",
                    IdentifierLength/sizeof(WCHAR), Identifier);

            if (CompInfo && (CompInfoLength == sizeof(CM_COMPONENT_INFORMATION)) &&
                !(CompInfo->Flags.Output && CompInfo->Flags.ConsoleOut))
            {
                DPRINT1("Weird: this DisplayController has flags %lu\n", CompInfo->Flags);

                /* Ignore */
                return STATUS_SUCCESS;
            }

            ASSERT(DisplayContext->VideoConfigData);

            Status = GetFramebufferVideoData(&DisplayContext->BaseAddress,
                                             &DisplayContext->BufferSize,
                                             DisplayContext->VideoConfigData,
                                             GET_LEGACY_DATA(ConfigurationData),
                                             GET_LEGACY_DATA_LEN(ConfigurationDataLength));
            if (!NT_SUCCESS(Status) ||
                (DisplayContext->BaseAddress.QuadPart == 0) ||
                (DisplayContext->BufferSize == 0))
            {
                /* Fail if no framebuffer was provided */
                DPRINT1("No framebuffer found!\n");
                return STATUS_DEVICE_DOES_NOT_EXIST;
            }

            DisplayContext->Interface = BusType;
            DisplayContext->BusNumber = BusNumber;
            break;
        }

        case MonitorPeripheral:
        {
            ASSERT(ControllerInformation);
            ASSERT(PeripheralInformation);

            /* Retrieve the pointers */
            DeviceInformation = PeripheralInformation;
            ConfigurationData =
                GetDeviceInfoData(DeviceInformation[IoQueryDeviceConfigurationData]);
            ConfigurationDataLength =
                GetDeviceInfoLength(DeviceInformation[IoQueryDeviceConfigurationData]);

            CompInfo =
                GetDeviceInfoData(DeviceInformation[IoQueryDeviceComponentInformation]);
            CompInfoLength =
                GetDeviceInfoLength(DeviceInformation[IoQueryDeviceComponentInformation]);

            Identifier =
                GetDeviceInfoData(DeviceInformation[IoQueryDeviceIdentifier]);
            IdentifierLength =
                GetDeviceInfoLength(DeviceInformation[IoQueryDeviceIdentifier]);

            DPRINT1("Monitor: '%.*ws'\n",
                    IdentifierLength/sizeof(WCHAR), Identifier);

            if (CompInfo && (CompInfoLength == sizeof(CM_COMPONENT_INFORMATION)) &&
                !(CompInfo->Flags.Output && CompInfo->Flags.ConsoleOut))
            {
                DPRINT1("Weird: this MonitorPeripheral has flags %lu\n", CompInfo->Flags);

                /* Ignore */
                return STATUS_SUCCESS;
            }

            ASSERT(DisplayContext->MonitorConfigData);

            Status = GetFramebufferMonitorData(DisplayContext->MonitorConfigData,
                                               GET_LEGACY_DATA(ConfigurationData),
                                               GET_LEGACY_DATA_LEN(ConfigurationDataLength));
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Invalid monitor data, ignoring.\n");
                Status = STATUS_SUCCESS;
            }

            break;
        }

        default:
            ASSERT(FALSE); // We should never be called there.
            return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;

#undef GetDeviceInfoLength
#undef GetDeviceInfoData
}

static
NTSTATUS
FindBootDisplayFromCachedConfigTree(
    _Out_ PPHYSICAL_ADDRESS VideoRamAddress,
    _Out_ PULONG VideoRamSize,
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_opt_ PINTERFACE_TYPE Interface,
    _Out_opt_ PULONG BusNumber)
{
    NTSTATUS Status;
    INTERFACE_TYPE InterfaceType;
    CONFIGURATION_TYPE ControllerType = DisplayController;
    CONFIGURATION_TYPE PeripheralType = MonitorPeripheral;
    ULONG ControllerNumber = 0;

    DISPLAY_CONTEXT DisplayContext = {0};

    /* Find the first DisplayController available on any bus in the system */
    DisplayContext.DeviceType = ControllerType;
    DisplayContext.VideoConfigData = VideoConfigData;
    for (InterfaceType = 0; InterfaceType < MaximumInterfaceType; ++InterfaceType)
    {
        Status = IoQueryDeviceDescription(&InterfaceType,
                                          NULL,
                                          &ControllerType,
                                          &ControllerNumber, // Should we specify that thing?
                                          NULL,
                                          NULL,
                                          pEnumDisplayControllerCallback,
                                          &DisplayContext);

        /* If found, break the loop right now */
        if (NT_SUCCESS(Status))
            break;
    }

    if (!NT_SUCCESS(Status)) // || (InterfaceType >= MaximumInterfaceType)
    {
        DPRINT1("Boot console not found\n");
        return Status;
    }

    *VideoRamAddress = DisplayContext.BaseAddress;
    *VideoRamSize = DisplayContext.BufferSize;

    if (Interface)
        *Interface = DisplayContext.Interface;
    if (BusNumber)
        *BusNumber = DisplayContext.BusNumber;

    // ASSERT(InterfaceType == DisplayContext.Interface);

    /* If no monitor data to retrieve, just return success */
    if (!MonitorConfigData)
        return STATUS_SUCCESS;

    /* Now find the optional MonitorPeripheral on this controller */
    DisplayContext.DeviceType = PeripheralType;
    DisplayContext.MonitorConfigData = MonitorConfigData;
    Status = IoQueryDeviceDescription(&InterfaceType,
                                      &DisplayContext.BusNumber,
                                      &ControllerType,
                                      &ControllerNumber,
                                      &PeripheralType,
                                      NULL, // In principle we should retrieve the 1st monitor.
                                      pEnumDisplayControllerCallback,
                                      &DisplayContext);
    if (!NT_SUCCESS(Status))
    {
        /* The optional monitor was not found, just ignore */
    }

    return STATUS_SUCCESS;
}

#endif // GET_DISPLAY_BY_API


typedef
NTSTATUS
(*PFIND_BOOT_DISPLAY)(
    _Out_ PPHYSICAL_ADDRESS VideoRamAddress,
    _Out_ PULONG VideoRamSize,
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_opt_ PINTERFACE_TYPE Interface,
    _Out_opt_ PULONG BusNumber);

static const struct
{
    PCSTR MethodName;
    PFIND_BOOT_DISPLAY FinderProc;
} BootDisplayFinders[] =
{
#if (NTDDI_VERSION >= NTDDI_WIN8)
#ifdef GET_DISPLAY_BY_API
    /* Find by API */
    {"API"               , FindBootDisplayFromAPI       },
#endif
#ifdef GET_DISPLAY_BY_LDR_BLOCK
    /* Otherwise, try looking in the loader block itself */
    {"Loader BGFX Block" , FindBootDisplayFromLoaderBGFX},
#endif
#endif
#ifdef GET_DISPLAY_BY_LDR_BLOCK
    /* If not found, try looking in the loader block ARC tree */
    {"Loader ARC Tree"   , FindBootDisplayFromLoaderARCTree   },
#endif
#ifdef GET_DISPLAY_BY_API
    /* Otherwise, try looking in the cached hardware tree */
    {"Configuration Tree", FindBootDisplayFromCachedConfigTree},
#endif
};

/**
 * @brief
 * Retrieves configuration data for the boot-time (POST) display controller
 * and monitor peripheral.
 *
 * This data is initialized by the firmware and the NT bootloader,
 * and can be retrieved from different sources:
 * - on Windows 8+: from API, or via the BgContext structure in the
 *   loader block parameter extension,
 * - on ReactOS, and Windows <= 2003-compatible: passed in loader block
 *   configuration ARC tree, or its cached version in the registry tree
 *   \Registry\Machine\Hardware\Description.
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
    ULONG i;
    NTSTATUS Status;

    PAGED_CODE();

    for (i = 0; i < RTL_NUMBER_OF(BootDisplayFinders); ++i)
    {
        /*
         * Retrieve video and monitor configuration data.
         * For monitor data, use the local variable since we may need
         * to do some adjustments in the video data using monitor data,
         * even if the caller does not require the monitor data itself
         * to be returned.
         */
        Status = BootDisplayFinders[i].FinderProc(VideoRamAddress,
                                                  VideoRamSize,
                                                  VideoConfigData,
                                                  &LocalMonitorConfigData,
                                                  &LocalInterface,
                                                  &LocalBusNumber);
        if (NT_SUCCESS(Status))
            break;
    }
    if (!NT_SUCCESS(Status) /*|| i >= RTL_NUMBER_OF(BootDisplayFinders)*/)
    {
        DPRINT1("Boot Display not found\n");
        return STATUS_DEVICE_DOES_NOT_EXIST; // STATUS_SYSTEM_DEVICE_NOT_FOUND; (Vista+)
    }

#if DBG
    DPRINT_TRACE("\n");
    DbgPrint("Boot Display found by %s\n", BootDisplayFinders[i].MethodName);
    DbgPrint("on Interface %lu, Bus %lu\n", LocalInterface, LocalBusNumber);
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
