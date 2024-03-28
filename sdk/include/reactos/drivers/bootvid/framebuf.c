/*
 * PROJECT:     ReactOS Boot Video Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Boot-time (POST) display discovery helper functions.
 * COPYRIGHT:   Copyright 2023-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
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


#ifdef GET_DISPLAY_BY_LDR_BLOCK
#include <ndk/kefuncs.h>
#endif

//#include <drivers/bootvid/framebuf.h> // FIXME: Include?
#include <drivers/videoprt/vpcfgcnv.h>

NTSTATUS
GetFramebufferVideoData(
    _Out_ PPHYSICAL_ADDRESS BaseAddress,    // Start
    _Out_ PULONG BufferSize,                // Length
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _In_ PVOID ConfigurationData,
    _In_ ULONG ConfigurationDataLength)
{
    if (!ConfigurationData ||
        (ConfigurationDataLength < sizeof(CM_PARTIAL_RESOURCE_LIST)))
    {
        /* Invalid entry?! */
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    /* Initialize the display adapter parameters, converting
     * them to the new format if needed */
    if (IsLegacyConfigData(ConfigurationData, ConfigurationDataLength,
                           sizeof(VIDEO_CONFIGURATION_DATA)))
    {
        /* Legacy configuration data, convert it into new format */
        CM_PARTIAL_RESOURCE_DESCRIPTOR FrameBuffer;

        DPRINT1("    GenFbVmp: Legacy Config data found\n");

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
        *BaseAddress = FrameBuffer.u.Memory.Start;
        *BufferSize  = FrameBuffer.u.Memory.Length;

        DPRINT1("**  GenFbVmp: Legacy framebuffer address (32-bit only): 0x%x\n",
                *BaseAddress);

        /* The legacy video controller configuration data does not
         * contain any information regarding framebuffer format, etc.
         * We will later calculate default values. */
        return STATUS_SUCCESS;
    }
    else if (IsNewConfigData(ConfigurationData, ConfigurationDataLength))
    {
        /* New configuration data */
        PCM_PARTIAL_RESOURCE_DESCRIPTOR FrameBuffer, Descriptor;

        DPRINT1("    GenFbVmp: New Config data found\n");

        GetVideoData((PCM_PARTIAL_RESOURCE_LIST)ConfigurationData,
                     NULL, // Interrupt
                     NULL, // ControlPort
                     NULL, // CursorPort
                     &FrameBuffer,
                     &Descriptor);

        if (FrameBuffer)
        {
            /* Save the framebuffer base and size */
            *BaseAddress = FrameBuffer->u.Memory.Start;
            *BufferSize  = FrameBuffer->u.Memory.Length;

            DPRINT1("**  GenFbVmp: Got framebuffer address: 0x%p\n",
                    *BaseAddress);
        }
        else
        {
            /* No framebuffer base?! Zero it out */
            DPRINT1("**  GenFbVmp: No framebuffer address?!\n");
            BaseAddress->QuadPart = 0;
            *BufferSize = 0;
        }

        if (Descriptor &&
            (Descriptor->u.DeviceSpecificData.DataSize >= sizeof(CM_FRAMEBUF_DEVICE_DATA)))
        {
            /* NOTE: This descriptor *MUST* be the last one.
             * The actual device data follows the descriptor. */
            PCM_FRAMEBUF_DEVICE_DATA VideoData = (PCM_FRAMEBUF_DEVICE_DATA)(Descriptor + 1);

            /* Just copy the data */
            *VideoConfigData = *VideoData;

            DPRINT1("**  GenFbVmp: Framebuffer information data found\n");
        }
        else
        {
            /* The configuration data does not contain any
             * information regarding framebuffer format, etc.
             * We will later calculate default values. */
            DPRINT1("**  GenFbVmp: Framebuffer information data NOT FOUND\n");
        }

        return STATUS_SUCCESS;
    }
    else
    {
        /* Unknown configuration, invalid entry?! */
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

#if 0
    /* Fail if no framebuffer was provided */
    if ((BaseAddress->QuadPart == 0) || (*BufferSize == 0))
    {
        DPRINT1("No framebuffer found!\n");
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
        /* Invalid entry?! */
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
            /* The configuration data does not contain any
             * information regarding the monitor.
             * We will later calculate default values. */
        }

        return STATUS_SUCCESS;
    }

    /* Unknown configuration, invalid entry?! */
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
    _Out_ PPHYSICAL_ADDRESS BaseAddress,    // Start // FrameBuffer
    _Out_ PULONG BufferSize,                // Length
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_ PINTERFACE_TYPE Interface, // FIXME: Make it opt?
    _Out_ PULONG BusNumber)
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
    *BaseAddress = BootGraphicsInfo.PhysicalFrameBuffer; // FrameBuffer;
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
    *BufferSize = VideoConfigData->PixelsPerScanLine *
                  VideoConfigData->ScreenHeight *
                  (VideoConfigData->BitsPerPixel / 8);

    // FIXME: What to do about BootGraphicsInfo.Flags ?

    VideoConfigData->VideoClock = 0; // FIXME: Use EDID

    /* No bus interface used there */
    *Interface = InterfaceTypeUndefined;
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
    _Out_ PPHYSICAL_ADDRESS BaseAddress,    // Start // FrameBuffer
    _Out_ PULONG BufferSize,                // Length
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_ PINTERFACE_TYPE Interface, // FIXME: Make it opt?
    _Out_ PULONG BusNumber)
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
    *Interface = InterfaceTypeUndefined;
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

static
NTSTATUS
FindBootDisplayFromLoaderARCTree(
    _Out_ PPHYSICAL_ADDRESS BaseAddress,    // Start // FrameBuffer
    _Out_ PULONG BufferSize,                // Length
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_ PINTERFACE_TYPE Interface, // FIXME: Make it opt?
    _Out_ PULONG BusNumber)
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
            break;

        if (Entry->ComponentEntry.Flags & (Output | ConsoleOut))
        {
            /* Found it */
            break;
        }
    }
#endif

    if (!Entry)
        return STATUS_DEVICE_DOES_NOT_EXIST;

    // Entry->ComponentEntry.IdentifierLength;
    DPRINT1("Display: '%s'\n", Entry->ComponentEntry.Identifier);

    Status = GetFramebufferVideoData(BaseAddress,
                                     BufferSize,
                                     VideoConfigData,
                                     Entry->ConfigurationData,
                                     Entry->ComponentEntry.ConfigurationDataLength);
    if (!NT_SUCCESS(Status) ||
        (BaseAddress->QuadPart == 0) ||
        (*BufferSize == 0))
    {
        /* Fail if no framebuffer was provided */
        DPRINT1("No framebuffer found!\n");
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // TODO: Determine an INTERFACE_TYPE and BusNumber for this entry?
    // For the time being, set it to undefined.
    //
    *Interface = InterfaceTypeUndefined;
    *BusNumber = -1;

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
    DPRINT1("Monitor: '%s'\n", Entry->ComponentEntry.Identifier);

    if ((Entry->ComponentEntry.Class != PeripheralClass)   ||
        (Entry->ComponentEntry.Type  != MonitorPeripheral) ||
       !(Entry->ComponentEntry.Flags & (Output | ConsoleOut)))
    {
        /* Ignore */
        return STATUS_SUCCESS;
    }

    /* Retrieve monitor configuration data. We use our local variable
     * since we may need to do some adjustments in the video data
     * using monitor data, even if the caller does not require the
     * monitor data itself to be returned. */
    Status = GetFramebufferMonitorData(MonitorConfigData,
                                       Entry->ConfigurationData,
                                       Entry->ComponentEntry.ConfigurationDataLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid monitor data, ignoring.\n");
    }

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
    _Out_ PPHYSICAL_ADDRESS BaseAddress,    // Start // FrameBuffer
    _Out_ PULONG BufferSize,                // Length
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_ PINTERFACE_TYPE Interface, // FIXME: Make it opt?
    _Out_ PULONG BusNumber)
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

    *BaseAddress = DisplayContext.BaseAddress;
    *BufferSize  = DisplayContext.BufferSize;

    *Interface = DisplayContext.Interface;
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
    _Out_ PPHYSICAL_ADDRESS BaseAddress,    // Start // FrameBuffer
    _Out_ PULONG BufferSize,                // Length
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_ PINTERFACE_TYPE Interface, // FIXME: Make it opt?
    _Out_ PULONG BusNumber);

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
 * Retrieves configuration data for the boot-time (i.e. the current
 * power-on self-test (POST)) display controller and monitor peripheral.
 *
 * This data is initialized by the firmware and the NT bootloader,
 * and can be retrieved from different sources:
 * - on Windows 8+: from API, or via the BgContext structure in the
 *   loader block parameter extension,
 * - on ReactOS, and Windows <= 2003-compatible: passed in loader block
 *   configuration ARC tree, or its cached version in the registry tree
 *   \Registry\Machine\Hardware\Description.
 *
 * (Note that data from the loader block is available only for boot
 * drivers and gets freed later on during NT loading.)
 **/
NTSTATUS
FindBootDisplay(
    _Out_ PPHYSICAL_ADDRESS BaseAddress,    // Start
    _Out_ PULONG BufferSize,                // Length
    _Out_ PCM_FRAMEBUF_DEVICE_DATA VideoConfigData,
    _Out_opt_ PCM_MONITOR_DEVICE_DATA MonitorConfigData,
    _Out_ PINTERFACE_TYPE Interface, // FIXME: Make it opt?
    _Out_ PULONG BusNumber)          // FIXME: Make it opt?
{
    NTSTATUS Status;
    ULONG i;
    CM_MONITOR_DEVICE_DATA LocalMonitorConfigData = {0};

    PAGED_CODE();

    for (i = 0; i < RTL_NUMBER_OF(BootDisplayFinders); ++i)
    {
        /*
         * Retrieve video and monitor configuration data.
         * For monitor data, we use our local variable since we may need
         * to do some adjustments in the video data using monitor data,
         * even if the caller does not require the monitor data itself
         * to be returned.
         */
        Status = BootDisplayFinders[i].FinderProc(BaseAddress,
                                                  BufferSize,
                                                  VideoConfigData,
                                                  &LocalMonitorConfigData,
                                                  Interface,
                                                  BusNumber);
        if (NT_SUCCESS(Status))
        {
            DPRINT1("Boot Display found by %s!\n", BootDisplayFinders[i].MethodName);

            /* If the individual framebuffer screen sizes are not
             * already initialized by now, use monitor data */
            //
            // TODO: Investigate: Do we want to do this here, or in the caller?
            //
            if ((VideoConfigData->ScreenWidth == 0) || (VideoConfigData->ScreenHeight == 0))
            {
                VideoConfigData->ScreenWidth  = LocalMonitorConfigData.HorizontalResolution;
                VideoConfigData->ScreenHeight = LocalMonitorConfigData.VerticalResolution;
            }

            /* Return the monitor data to the caller if required */
            if (MonitorConfigData)
                *MonitorConfigData = LocalMonitorConfigData;

#if 0
            if ((VideoConfigData->ScreenWidth <= 1) || (VideoConfigData->ScreenHeight <= 1))
            {
                DPRINT1("Cannot obtain current screen resolution!\n");
                // return FALSE;
            }
#endif

            return Status;
        }
    }

    DPRINT1("Boot Display not found!\n");
    return STATUS_NO_SUCH_DEVICE;      // <-- "A device that does not exist was specified."
    // STATUS_DEVICE_DOES_NOT_EXIST;   // <-- "This device does not exist." (next-to-better)
    // STATUS_SYSTEM_DEVICE_NOT_FOUND; // <-- "The requested system device cannot be found." (better, but Vista+)
}


/**
 * @brief
 * Wrapper around HalTranslateBusAddress() and HALPRIVATEDISPATCH->HalFindBusAddressTranslation().
 *
 * @note
 * Context parameter is supplemental from standard HalTranslateBusAddress.
 * It allows context passing for the underlying HalFindBusAddressTranslation
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
        /* Return bus data */
        TranslatedAddress->QuadPart = BusAddress.QuadPart;
        return TRUE;
    }

    /* If InterfaceType is greater than the maximum supported type,
     * this means we don't now yet the interface and bus number,
     * and we need to find a bus translation. */
    if (InterfaceType >= MaximumInterfaceType)
    {
        /* Make sure that we have a bus translation function */
        if (!HalFindBusAddressTranslation)
            return FALSE;

        /* Loop trying to find possible VGA base addresses */
        /* Get the VGA Register address */
        return HalFindBusAddressTranslation(BusAddress,
                                            AddressSpace,
                                            TranslatedAddress,
                                            &Context,
                                            TRUE);
    }

    /* Otherwise, InterfaceType is valid, call the original HAL function */
    return HalTranslateBusAddress(InterfaceType,
                                  BusNumber,
                                  BusAddress,
                                  AddressSpace,
                                  TranslatedAddress);
}

/* EOF */
