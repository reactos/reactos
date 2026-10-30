/*
 * PROJECT:     ReactOS Generic Framebuffer Video Miniport Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main file.
 * COPYRIGHT:   Copyright 2022-2023 Justin Miller <justinmiller100@gmail.com>
 *              Copyright 2023-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *              Copyright 2024 Sylas Hollander <distrohopper39b.business@gmail.com>
 *
 * REFERENCES:
 * https://github.com/open-watcom/open-watcom-v2/tree/master/bld/src/win32/videomp
 */

#include <ntifs.h>
#include <arc/arc.h>

#include <dderror.h>
#define __BROKEN__
#include <miniport.h>
#include <video.h>
#include <devioctl.h>

#include <stdio.h> // <ntstrsafe.h>

#include <section_attribs.h>

#include <debug.h>
// #define DPRINT(fmt, ...)    VideoDebugPrint((Info, fmt, ##__VA_ARGS__))
// #define DPRINT1(fmt, ...)   VideoDebugPrint((Error, fmt, ##__VA_ARGS__))

#include "../../../../boot/freeldr/freeldr/arch/twidbits.h" // FIXME: Path

/* Include the Boot-time (POST) display discovery helper functions */
//#include <drivers/bootvid/framebuf.h>
#include <drivers/bootvid/framebuf.c>

// Define if the driver must behave as a legacy driver.
#define DRIVER_LEGACY   1
// Define if the driver must behave as a PnP driver.
#define DRIVER_PNP      2
#define DRIVER_MODE     DRIVER_PNP // (DRIVER_LEGACY | DRIVER_PNP)

/********************************** Globals ***********************************/

/**
 * @brief
 * Registry Identification strings used in GenFbVmpFindAdapter()
 * for "HardwareInformation.AdapterString", "HardwareInformation.BiosString",
 * "HardwareInformation.ChipType", and "HardwareInformation.DacType".
 **/
static const WCHAR AdapterString[]   = L"Generic Framebuffer";
static const WCHAR AdapterChipType[] = L"Framebuffer";
static const WCHAR AdapterDacType[]  = L"Internal";

#if 0
typedef struct
{
    PUCHAR Mapped;
    PHYSICAL_ADDRESS RangeStart;
    ULONG RangeLength;
    UCHAR RangeInIoSpace;
} GENFB_ADDRESS_RANGE;
#endif

typedef struct _GENFB_DISPLAY_INFO
{
    /* Where the display controller is */
    INTERFACE_TYPE Interface;
    ULONG BusNumber;

    PHYSICAL_ADDRESS VramAddress; // == PhysicalVideoMemoryBase;
    ULONG VramSize; // == PhysicalVideoMemoryLength;
    PVOID VideoRamAddress; // Mapped VRAM address.

    // GENFB_ADDRESS_RANGE FrameBuffer;
    // PHYSICAL_ADDRESS FrameBuffer; // PhysicalFrameOffset;
    ULONG FrameBufferSize; // == PhysicalFrameLength;
    PVOID FrameAddress; // Mapped framebuffer virtual address.

    /* Configuration data from hardware tree */
    CM_FRAMEBUF_DEVICE_DATA VideoConfigData;
    CM_MONITOR_DEVICE_DATA MonitorConfigData;

} GENFB_DISPLAY_INFO, *PGENFB_DISPLAY_INFO;

typedef struct _GENFB_DEVICE_EXTENSION
{
    GENFB_DISPLAY_INFO DisplayInfo;
    ULONG BytesPerScanLine;
    UCHAR BytesPerPixel;

    /* The one and only video mode we support */
    VIDEO_MODE_INFORMATION CurrentVideoMode;
} GENFB_DEVICE_EXTENSION, *PGENFB_DEVICE_EXTENSION;

/* The unique boot-time display available */
GENFB_DISPLAY_INFO gBootDisplay = {0};
BOOLEAN gbBootDisplayFound = FALSE;
/**/BOOLEAN gbBootDisplayReported = FALSE;/**/


/********************************** Private ***********************************/

/**
 * @brief
 * Maps the video RAM and framebuffer to the requested preferred address.
 **/
static VP_STATUS
GenFbVmpMapVideoMemory(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ HANDLE ProcessHandle,
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _Inout_ PULONG Length,
    _Inout_ PVOID* VirtualAddress)
{
    PVOID RequestedVirtualAddress;
    ULONG CapturedLength;
    ULONG InIoSpace;
    VP_STATUS Status;

    CapturedLength = *Length;
    InIoSpace = VIDEO_MEMORY_SPACE_MEMORY | VIDEO_MEMORY_SPACE_P6CACHE;
    if (ProcessHandle)
    {
        /* *VirtualAddress as input for a requested virtual address is *UNUSED* in this case */
        RequestedVirtualAddress = (PVOID)ProcessHandle;
        InIoSpace |= VIDEO_MEMORY_SPACE_USER_MODE;
    }
    else
    {
        /* Map the video RAM (set up by the firmware) to the
         * preferred address the user requests, if possible. */
        RequestedVirtualAddress = *VirtualAddress;
    }
    Status = VideoPortMapMemory(HwDeviceExtension,
                                PhysicalAddress,
                                &CapturedLength,
                                &InIoSpace,
                                &RequestedVirtualAddress);
    if (Status != NO_ERROR)
    {
        /* The underlying call to MmMapIoSpace failed, this may be because
         * MmWriteCombined isn't supported, so try again with MmNonCached */
        CapturedLength = *Length;
        InIoSpace = VIDEO_MEMORY_SPACE_MEMORY;
        if (ProcessHandle)
        {
            RequestedVirtualAddress = (PVOID)ProcessHandle;
            InIoSpace |= VIDEO_MEMORY_SPACE_USER_MODE;
        }
        else
        {
            RequestedVirtualAddress = *VirtualAddress;
        }
        Status = VideoPortMapMemory(HwDeviceExtension,
                                    PhysicalAddress,
                                    &CapturedLength,
                                    &InIoSpace,
                                    &RequestedVirtualAddress);
    }
    if (Status != NO_ERROR)
    {
        DPRINT1("Failed to map video RAM 0x%I64X (%lu bytes)\n",
                PhysicalAddress.QuadPart, *Length);
        return Status;
    }

    *Length = CapturedLength;
    *VirtualAddress = RequestedVirtualAddress;
    return Status;
}

/**
 * @brief
 * Releases the mapping between the virtual address space
 * and the adapter's framebuffer and video RAM.
 **/
static VP_STATUS
GenFbVmpUnmapVideoMemory(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID VirtualAddress,
    _In_opt_ HANDLE ProcessHandle)
{
    return VideoPortUnmapMemory(HwDeviceExtension, VirtualAddress, ProcessHandle);
}

static BOOLEAN
GenFbVmpSetupCurrentMode(
    _In_ PGENFB_DEVICE_EXTENSION DeviceExtension)
{
    PVIDEO_MODE_INFORMATION VideoMode = &DeviceExtension->CurrentVideoMode;
    PGENFB_DISPLAY_INFO DisplayInfo   = &DeviceExtension->DisplayInfo;
    PCM_FRAMEBUF_DEVICE_DATA VideoData  = &DisplayInfo->VideoConfigData;
    PCM_MONITOR_DEVICE_DATA MonitorData = &DisplayInfo->MonitorConfigData;
    ULONG RegValue;
    // ULONG BytesPerScanLine;
    // UCHAR BytesPerPixel;

    VideoMode->Length = sizeof(*VideoMode);
    VideoMode->ModeIndex = 0; /* Only one single mode is supported */

    ASSERT(VideoData->ScreenWidth >= 1);
    ASSERT(VideoData->ScreenHeight >= 1);

    /* Retrieve the framebuffer visible screen dimensions, and its attributes */
    VideoMode->VisScreenWidth  = VideoData->ScreenWidth;
    VideoMode->VisScreenHeight = VideoData->ScreenHeight;

    // BytesPerPixel = (VideoData->BitsPerPixel + 7) / 8;
    // ASSERT(DeviceExtension->BytesPerPixel == BytesPerPixel);

    // BytesPerScanLine = VideoData->PixelsPerScanLine * BytesPerPixel;
    // ASSERT(DeviceExtension->BytesPerScanLine == BytesPerScanLine);
    // ASSERT(DisplayInfo->FrameBufferSize == VideoMode->VisScreenHeight * BytesPerScanLine);

    VideoMode->ScreenStride = DeviceExtension->BytesPerScanLine;

    VideoMode->NumberOfPlanes = 1;
    VideoMode->BitsPerPlane = VideoData->BitsPerPixel / VideoMode->NumberOfPlanes;

    /* Video frequency in Hertz */
    VideoMode->Frequency = VideoData->VideoClock;
    if (VideoMode->Frequency == 0) // or 1 ?
        VideoMode->Frequency = 60; // Default value.

    /* Use metrics from the monitor, if any */
    VideoMode->XMillimeter = MonitorData->HorizontalScreenSize;
    VideoMode->YMillimeter = MonitorData->VerticalScreenSize;
    if ((VideoMode->XMillimeter == 0) || (VideoMode->YMillimeter == 0))
    {
        /* Assume 96 DPI and 25.4 millimeters per inch, round to nearest */
        static const ULONG dpi = 96;
        // VideoMode->XMillimeter = VideoData->ScreenWidth  * 254 / 960;
        // VideoMode->YMillimeter = VideoData->ScreenHeight * 254 / 960;
        VideoMode->XMillimeter = ((ULONGLONG)VideoData->ScreenWidth  * 254 + (dpi * 5)) / (dpi * 10);
        VideoMode->YMillimeter = ((ULONGLONG)VideoData->ScreenHeight * 254 + (dpi * 5)) / (dpi * 10);
    }

    if (VideoData->BitsPerPixel > 8)
    {
        if (VideoData->PixelMasks.RedMask   == 0 &&
            VideoData->PixelMasks.GreenMask == 0 &&
            VideoData->PixelMasks.BlueMask  == 0)
        {
            /* Determine pixel mask given color depth and color channel */
            switch (VideoData->BitsPerPixel)
            {
                case 32:
                case 24: /* 8:8:8 */
                    // ulMask = 0x00FF0000 >> (Channel * 8);
                    VideoMode->RedMask   = 0x00FF0000;
                    VideoMode->GreenMask = 0x0000FF00;
                    VideoMode->BlueMask  = 0x000000FF;
                    break;
                case 16: /* 5:6:5 */
                    VideoMode->RedMask   = 0xF800;
                    VideoMode->GreenMask = 0x07E0;
                    VideoMode->BlueMask  = 0x001F;
                    break;
                case 15: /* 5:5:5 */
                    // ulMask = 0x00007C00 >> (Channel * 5);
                    VideoMode->RedMask   = 0x7C00;
                    VideoMode->GreenMask = 0x03E0;
                    VideoMode->BlueMask  = 0x001F;
                    break;
                default:
                    /* Unsupported BPP */
                    UNIMPLEMENTED;
                    DPRINT1("BitsPerPixel %d not implemented\n", VideoData->BitsPerPixel);
                    VideoMode->RedMask   = 0;
                    VideoMode->GreenMask = 0;
                    VideoMode->BlueMask  = 0;
            }
        }
        else
        {
            /* Copy the pixel masks */
            VideoMode->RedMask   = VideoData->PixelMasks.RedMask;
            VideoMode->GreenMask = VideoData->PixelMasks.GreenMask;
            VideoMode->BlueMask  = VideoData->PixelMasks.BlueMask;
        }

        VideoMode->NumberRedBits   = CountNumberOfBits(VideoMode->RedMask);
            // VideoData->PixelMasks.NumberRedBits;
        VideoMode->NumberGreenBits = CountNumberOfBits(VideoMode->GreenMask);
            // VideoData->PixelMasks.NumberGreenBits;
        VideoMode->NumberBlueBits  = CountNumberOfBits(VideoMode->BlueMask);
            // VideoData->PixelMasks.NumberBlueBits;
    }
    else
    {
        /* Palettized modes don't have a mask */
        VideoMode->RedMask   = 0;
        VideoMode->GreenMask = 0;
        VideoMode->BlueMask  = 0;
    }

    /* Video RAM dimensions in *pixels*, not in bytes */
    VideoMode->VideoMemoryBitmapWidth  = VideoData->PixelsPerScanLine;
    VideoMode->VideoMemoryBitmapHeight = DisplayInfo->VramSize / VideoMode->ScreenStride;
    // VideoRamLength == VramSize == VideoMemoryBitmapHeight * ScreenStride == PhysicalVideoMemoryLength;

    VideoMode->AttributeFlags =
        VIDEO_MODE_GRAPHICS | VIDEO_MODE_COLOR | VIDEO_MODE_NO_OFF_SCREEN;
    if (VideoData->BitsPerPixel <= 8)
        VideoMode->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN;

    VideoMode->DriverSpecificAttributeFlags = 0;

    /*
     * At startup, win32k.sys tries to select the best video mode by reading
     * the "DefaultSettings...." values in the video device hardware registry
     * key, then attempts to match it with all the modes supported by all
     * the different active video drivers (including the fallback VGA one on
     * supported machines) for the current adapter, trying to find the exact
     * or the closest suitable mode. If needed, another video driver, that
     * supports some other video mode than the one we want, can be activated
     * instead.
     * Since we can handle the only one single boot video mode, and want to
     * be activated instead, we *MUST* override the video mode cached in the
     * registry, so that win32k.sys will instead try to match it against ours.
     *
     * See also:
     * https://www.betaarchive.com/wiki/index.php/Microsoft_KB_Archive/102992#Video_Device_Driver_Entries
     */

    RegValue = VideoMode->VisScreenWidth;
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"DefaultSettings.XResolution",
                                   &RegValue, sizeof(RegValue));

    RegValue = VideoMode->VisScreenHeight;
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"DefaultSettings.YResolution",
                                   &RegValue, sizeof(RegValue));

    RegValue = VideoMode->BitsPerPlane /* * VideoMode->NumberOfPlanes */; // VideoData->BitsPerPixel;
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"DefaultSettings.BitsPerPel",
                                   &RegValue, sizeof(RegValue));

    // TODO: hbelusca - Add these two also during setup.
    RegValue = VideoMode->Frequency;
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"DefaultSettings.VRefresh",
                                   &RegValue, sizeof(RegValue));

    RegValue = !!(VideoMode->AttributeFlags & VIDEO_MODE_INTERLACED);
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"DefaultSettings.Interlaced",
                                   &RegValue, sizeof(RegValue));

#if 0
    // L"Attach.RelativeX", pdm->dmPosition.x;
    // L"Attach.RelativeY", pdm->dmPosition.y;
    // L"Attach.PrimaryDevice"
    // L"Attach.ToDesktop"
    // L"DefaultSettings.BitsPerPel", pdm->dmBitsPerPel;
    // L"DefaultSettings.XResolution", pdm->dmPelsWidth;
    // L"DefaultSettings.YResolution", pdm->dmPelsHeight;
    // L"DefaultSettings.Flags", pdm->dmDisplayFlags;
    // L"DefaultSettings.VRefresh", pdm->dmDisplayFrequency;
    // L"DefaultSettings.XPanning", pdm->dmPanningWidth;
    // L"DefaultSettings.YPanning", pdm->dmPanningHeight;
    // L"DefaultSettings.Orientation", pdm->dmDisplayOrientation;
    // L"DefaultSettings.FixedOutput", pdm->dmDisplayFixedOutput;
#endif

    return TRUE;
}


/*********************************** Public ***********************************/

/*
 * From a (VIDEO|MONITOR)_HARDWARE_CONFIGURATION_DATA structure, retrieve
 * where the actual legacy data starts from, based from the fact that the
 * first two fields, InterfaceType and BusNumber, are common with the
 * CM_FULL_RESOURCE_DESCRIPTOR header, and the actual data starts where
 * the CM_FULL_RESOURCE_DESCRIPTOR::PartialResourceList member would start.
 */
#define GET_LEGACY_DATA(fullConfigData) \
    ((PVOID)&(((PCM_FULL_RESOURCE_DESCRIPTOR)fullConfigData)->PartialResourceList))

#define GET_LEGACY_DATA_LEN(fullConfigDataLen) \
    ((fullConfigDataLen >= FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList)) \
   ? (fullConfigDataLen  - FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList)) \
   : 0)

/**
 * @brief
 * Callback routine for the VideoPortGetDeviceData function.
 *
 * @return
 * - NO_ERROR if the function completed properly.
 * - ERROR_DEV_NOT_EXIST if we did not find the device.
 * - ERROR_INVALID_PARAMETER otherwise.
 **/
static VP_STATUS
NTAPI
GenFbGetDeviceDataCallback(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID Context,
    _In_ VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    _In_ PVOID Identifier,
    _In_ ULONG IdentifierLength,
    _In_ PVOID ConfigurationData,
    _In_ ULONG ConfigurationDataLength,
    _Inout_ PVOID ComponentInformation,
    _In_ ULONG ComponentInformationLength)
{
    PGENFB_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
    PGENFB_DISPLAY_INFO DisplayInfo = &DeviceExtension->DisplayInfo;
    PCM_COMPONENT_INFORMATION CompInfo = ComponentInformation;

    switch (DeviceDataType)
    {
        case VpControllerData:
        {
            NTSTATUS Status;

            DPRINT1("Getting controller information: Display: '%.*ws'\n",
                    IdentifierLength/sizeof(WCHAR), (PCWCH)Identifier);

            if (!ConfigurationData ||
                (ConfigurationDataLength < sizeof(VIDEO_HARDWARE_CONFIGURATION_DATA)))
            {
                DPRINT1("Invalid display configuration data %p %lu\n",
                        ConfigurationData, ConfigurationDataLength);
                return ERROR_DEV_NOT_EXIST;
            }

            if (CompInfo && (ComponentInformationLength == sizeof(CM_COMPONENT_INFORMATION)) &&
                !(CompInfo->Flags.Output && CompInfo->Flags.ConsoleOut))
            {
                DPRINT1("Weird: this DisplayController has flags %lu\n", CompInfo->Flags);
            }

            /* Pre-initialize VideoConfigData */
            VideoPortZeroMemory(&DisplayInfo->VideoConfigData,
                                sizeof(DisplayInfo->VideoConfigData));

            Status = GetFramebufferVideoData(&DisplayInfo->VramAddress,
                                             &DisplayInfo->VramSize,
                                             &DisplayInfo->VideoConfigData,
                                             GET_LEGACY_DATA(ConfigurationData),
                                             GET_LEGACY_DATA_LEN(ConfigurationDataLength));
            if (!NT_SUCCESS(Status) ||
                (DisplayInfo->VramAddress.QuadPart == 0) ||
                (DisplayInfo->VramSize == 0))
            {
                /* Fail if no framebuffer was provided */
                DPRINT1("No framebuffer found!\n");
                return ERROR_DEV_NOT_EXIST;
            }

            return NO_ERROR;
        }

#if 0 // TODO: Reactivate once the structures are in place.
        case VpMonitorData:
        {
            DPRINT1("Getting monitor information: Monitor: '%.*ws'\n",
                    IdentifierLength/sizeof(WCHAR), (PCWCH)Identifier);

            if (!ConfigurationData ||
                (ConfigurationDataLength < sizeof(MONITOR_HARDWARE_CONFIGURATION_DATA)))
            {
                DPRINT1("Invalid monitor configuration data %p %lu\n",
                        ConfigurationData, ConfigurationDataLength);
                return ERROR_DEV_NOT_EXIST;
            }

            if (CompInfo && (ComponentInformationLength == sizeof(CM_COMPONENT_INFORMATION)) &&
                !(CompInfo->Flags.Output && CompInfo->Flags.ConsoleOut))
            {
                DPRINT1("Weird: this MonitorPeripheral has flags %lu\n", CompInfo->Flags);
            }

            /* Pre-initialize MonitorConfigData */
            VideoPortZeroMemory(&DisplayInfo->MonitorConfigData,
                                sizeof(DisplayInfo->MonitorConfigData));

            /* Retrieve optional monitor configuration data;
             * ignore any error if it does not exist. */
            GetFramebufferMonitorData(&DisplayInfo->MonitorConfigData,
                                      GET_LEGACY_DATA(ConfigurationData),
                                      GET_LEGACY_DATA_LEN(ConfigurationDataLength));

            return NO_ERROR;
        }
#endif

        default:
        {
            DPRINT1("Unknown device type %lu\n", DeviceDataType);
            return ERROR_INVALID_PARAMETER;
        }
    }
}

static VP_STATUS
GenFbAcquireResources(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG NumAccessRanges,
    _In_ PVIDEO_ACCESS_RANGE AccessRanges)
{
    VP_STATUS status;
    ULONG i;

    /*
     * Claim the video resources if we are loaded as fall-back device.
     * Otherwise, another video driver is handling the video display
     * and claimed the resources, so we can fail loading.
     */

    /*
     * Try to acquire video resources exclusively.
     */
    for (i = 0; i < NumAccessRanges; ++i)
    {
        AccessRanges[i].RangeShareable = FALSE;
    }
    status = VideoPortVerifyAccessRanges(HwDeviceExtension,
                                         NumAccessRanges,
                                         AccessRanges);
    if (status != NO_ERROR)
    {
        /*
         * We could not obtain the resources exclusively: this means
         * another driver is handling this display. Fail to load!
         */
        return status;
    }

    /*
     * The resources have been obtained exclusively: we are the default
     * fall-back driver. Re-claim those resources in shared mode, so that
     * a PnP driver for this display can take over later, claiming those
     * resources and load successfully.
     */
    for (i = 0; i < NumAccessRanges; ++i)
    {
        AccessRanges[i].RangeShareable = TRUE;
    }
    return VideoPortVerifyAccessRanges(HwDeviceExtension,
                                       NumAccessRanges,
                                       AccessRanges);
}

static VP_STATUS
GenFbVmpConfigureAdapter(
    _In_ PVOID HwDeviceExtension,
    _Inout_ PVIDEO_PORT_CONFIG_INFO ConfigInfo)
{
    PGENFB_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
    PGENFB_DISPLAY_INFO DisplayInfo = &DeviceExtension->DisplayInfo;
    PCM_FRAMEBUF_DEVICE_DATA VideoData  = &DisplayInfo->VideoConfigData;
    PCM_MONITOR_DEVICE_DATA MonitorData = &DisplayInfo->MonitorConfigData;
    VIDEO_ACCESS_RANGE accessRanges[1];
    ULONGLONG VramSizeULL;
    ULONG VramSizeUL;
    VP_STATUS status;

#if 1 // TODO: Keep it here?

    PHYSICAL_ADDRESS FrameBuffer;

    /* The VRAM address must be page-aligned */
    if (DisplayInfo->VramAddress.QuadPart % PAGE_SIZE != 0) // DPRINTed for diagnostics on some systems
        DPRINT1("** VramAddress 0x%I64X isn't PAGE_SIZE aligned\n", DisplayInfo->VramAddress.QuadPart);
    ASSERT(DisplayInfo->VramAddress.QuadPart % PAGE_SIZE == 0);
    if (DisplayInfo->VramSize % PAGE_SIZE != 0)
        DPRINT1("** VramSize %lu (0x%lx) isn't multiple of PAGE_SIZE\n", DisplayInfo->VramSize, DisplayInfo->VramSize);
    // ASSERT(DisplayInfo->VramSize % PAGE_SIZE == 0); // This assert may fail, e.g. 800x600@32bpp UEFI GOP display

    /* If the screen sizes are not already initialized by now, use monitor data */
    if ((VideoData->ScreenWidth == 0) || (VideoData->ScreenHeight == 0))
    {
        VideoData->ScreenWidth  = MonitorData->HorizontalResolution;
        VideoData->ScreenHeight = MonitorData->VerticalResolution;
    }
    if ((VideoData->ScreenWidth < 1) || (VideoData->ScreenHeight < 1))
    {
        DPRINT1("Cannot obtain current screen resolution\n");
        return ERROR_INVALID_PARAMETER;
    }

    /* Retrieve the framebuffer address, its visible screen dimensions, and its attributes */
    FrameBuffer.QuadPart = DisplayInfo->VramAddress.QuadPart + VideoData->FrameBufferOffset;

    DeviceExtension->BytesPerPixel = (VideoData->BitsPerPixel + 7) / 8; // Round up to nearest byte.
    ASSERT(DeviceExtension->BytesPerPixel >= 1);

    /*
     * Number of bytes per scan-line ("Pitch", or "ScreenStride").
     * It may be greater than ScreenWidth in case scan lines are padded to
     * an amount of memory alignment (e.g. for performance reasons, or due
     * to hardware restrictions). These padding pixels are outside of the
     * visible area.
     */
    ASSERT(VideoData->ScreenWidth <= VideoData->PixelsPerScanLine);
    DeviceExtension->BytesPerScanLine = VideoData->PixelsPerScanLine * DeviceExtension->BytesPerPixel;
    if (DeviceExtension->BytesPerScanLine < 1)
    {
        DPRINT1("Invalid BytesPerScanLine = %lu\n", DeviceExtension->BytesPerScanLine);
        return ERROR_INVALID_PARAMETER;
    }

    /* Compute the visible framebuffer size */
    DisplayInfo->FrameBufferSize = VideoData->ScreenHeight * DeviceExtension->BytesPerScanLine;

    /* Verify that the framebuffer actually fits inside the video RAM */
    if (FrameBuffer.QuadPart + DisplayInfo->FrameBufferSize >
        DisplayInfo->VramAddress.QuadPart + DisplayInfo->VramSize)
    {
        DPRINT1("The framebuffer exceeds video memory bounds!\n");
        return ERROR_INVALID_PARAMETER;
    }

#endif

    /*
     * Fill up the device extension and the configuration
     * information with the appropriate data.
     */

    // ConfigInfo->BusInterruptLevel  = ConfigData->Irql;
    // ConfigInfo->BusInterruptVector = ConfigData->Vector;
    // HalGetInterruptVector(...)

    /* Setup the access ranges */
    // QUESTION: Do we need to also set up the
    // ConfigData->ControlBase/Size and ConfigData->CursorBase/Size
    // memory ports, or any other resource we get,
    // besides setting up the framebuffer?
    accessRanges[0].RangeStart = DisplayInfo->VramAddress;
    accessRanges[0].RangeLength = DisplayInfo->VramSize;
    accessRanges[0].RangeInIoSpace = VIDEO_MEMORY_SPACE_MEMORY;
    accessRanges[0].RangeVisible = FALSE;
    accessRanges[0].RangeShareable = FALSE;
    accessRanges[0].RangePassive = 0;

    /* Try to acquire those hardware resources */
    status = GenFbAcquireResources(HwDeviceExtension,
                                   RTL_NUMBER_OF(accessRanges),
                                   accessRanges);
    if (status != NO_ERROR)
        return status;

    /* Save framebuffer information */
    // DeviceExtension->PhysicalFrameAddress = ConfigData->FrameBase;
    // DeviceExtension->FrameLength = ConfigData->FrameSize;

    // QUESTION: If set up above, should we also map the
    // video controller and cursor register?
    // DeviceExtension->VideoAddress

    /* Map the video memory into the system virtual
     * address space so we can clear it out. */
    accessRanges[0].RangeStart = FrameBuffer;
    accessRanges[0].RangeLength = DisplayInfo->FrameBufferSize;
    DeviceExtension->DisplayInfo.FrameAddress =
        VideoPortGetDeviceBase(HwDeviceExtension,
                               accessRanges[0].RangeStart,
                               accessRanges[0].RangeLength,
                               accessRanges[0].RangeInIoSpace);
    if (!DeviceExtension->DisplayInfo.FrameAddress)
    {
        /* We failed, release the acquired resources and bail out */
        VideoPortVerifyAccessRanges(HwDeviceExtension, 0, NULL);
        return ERROR_INVALID_PARAMETER;
    }

    DPRINT1("GenFbVmpConfigureAdapter: Mapped framebuffer 0x%I64X to 0x%p - size %lu\n",
            accessRanges[0].RangeStart,
            DeviceExtension->DisplayInfo.FrameAddress,
            accessRanges[0].RangeLength);

//
// TODO: Or setup the current mode only in HwInitialize
//
    /* From the captured video framebuffer and monitor data,
     * synthesize our single video mode information structure. */
    if (!GenFbVmpSetupCurrentMode(HwDeviceExtension))
    {
        /* We failed, release the acquired resources and bail out */
        // VideoPortFreeDeviceBase(HwDeviceExtension, ...);
        VideoPortVerifyAccessRanges(HwDeviceExtension, 0, NULL);
        return ERROR_DEV_NOT_EXIST;
    }

    /* Zero out the emulator entries since we do not support them (not VGA) */
    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.QuadPart = 0;
    ConfigInfo->VdmPhysicalVideoMemoryLength = 0;
    ConfigInfo->HardwareStateSize = 0;

    /*
     * For a list of possible parameters, consult
     * https://learn.microsoft.com/windows-hardware/drivers/display/setting-hardware-information-in-the-registry
     */

    /* Chipset information */
    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.AdapterString",
                                   (PVOID)AdapterString, sizeof(AdapterString));
    // L"HardwareInformation.BiosString"
    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.ChipType",
                                   (PVOID)AdapterChipType, sizeof(AdapterChipType));
    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.DacType",
                                   (PVOID)AdapterDacType, sizeof(AdapterDacType));

    /* Video Memory - Contrary to what MSDN says, this value is in *bytes* and not in megabytes */
    VramSizeUL = DisplayInfo->VramSize;
    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &VramSizeUL, sizeof(VramSizeUL));

    /* New Windows 8+ value, checked by 3rd-party tools:
     * report the memory size in a 64-bit value for graphics cards
     * that report more than 4GB of available VRAM. */
    VramSizeULL = (ULONGLONG)DisplayInfo->VramSize;
    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.qwMemorySize",
                                   &VramSizeULL, sizeof(VramSizeULL));

    /* Note: Don't use VideoMode->Frequency since that value is
     * the regularized one, and not the original reported one. */
    if (VideoData->VideoClock != 0)
    {
        ULONG VideoClock = VideoData->VideoClock;
        VideoPortSetRegistryParameters(HwDeviceExtension,
                                       L"HardwareInformation.CurrentChipClockSpeed",
                                       &VideoClock, sizeof(VideoClock));

        // Other undocumented speed indicators:
        // L"HardwareInformation.CurrentMemClockSpeed"
        // L"HardwareInformation.CurrentPixelClockSpeed"
    }

    return NO_ERROR;
}

#if (DRIVER_MODE & DRIVER_PNP)
CODE_SEG("PAGE")
VP_STATUS NTAPI
GenFbVmpFindAdapter(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID HwContext,
    _In_ PWSTR ArgumentString,
    _Inout_ PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    _In_ PUCHAR Again)
{
    PGENFB_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    PAGED_CODE();

    DPRINT1("GenFbVmpFindAdapter(0x%p, 0x%p, %s, 0x%p, 0x%p)\n",
        HwDeviceExtension, HwContext, ArgumentString, ConfigInfo, Again);

    *Again = FALSE;

    if (ConfigInfo->Length < sizeof(*ConfigInfo))
        return ERROR_INVALID_PARAMETER;

__debugbreak();

    //
    // TODO: Detect whether this is the detection for the boot-time console
    // that has been searched for during initialization. If we are in PnP mode
    // just skip the detection.
    //

    /// TEMP HACK: Do the check differently
    if (gbBootDisplayFound &&
        (gBootDisplay.Interface == ConfigInfo->AdapterInterfaceType) &&
        (gBootDisplay.BusNumber == ConfigInfo->SystemIoBusNumber))
    {
        if (gbBootDisplayReported)
        {
            DPRINT1("GenFbVmpFindAdapter (PnP): Boot display was already reported on Interface %lu, Bus %lu\n",
                    gBootDisplay.Interface, gBootDisplay.BusNumber);
            return ERROR_DEV_NOT_EXIST;
        }
        else
        {
            DPRINT1("GenFbVmpFindAdapter (PnP): Boot display found on Interface %lu, Bus %lu\n",
                    gBootDisplay.Interface, gBootDisplay.BusNumber);
            /* We do the reporting below */
            gbBootDisplayReported = TRUE;
        }
    }
    else
    {
        /* The boot display controller is not on this bus */
        return ERROR_DEV_NOT_EXIST;
    }

#if 0 // TODO: Reconsider this.
    /*
     * Retrieve any configuration data for the display controller and monitor.
     */

    VideoPortZeroMemory(DisplayInfo, sizeof(*DisplayInfo));

    /* Enumerate and find the boot-time console display controller */
    // ControllerClass, DisplayController
    if (VideoPortGetDeviceData(HwDeviceExtension,
                               VpControllerData,
                               GenFbGetDeviceDataCallback,
                               NULL /*&DeviceExtension->DisplayInfo*/) != NO_ERROR)
    {
        DPRINT1("GenFbVmp: Getting controller info failed\n");
        return ERROR_DEV_NOT_EXIST;
    }

    /* Now find the MonitorPeripheral to obtain more information.
     * It should be child of the display controller. */
    // PeripheralClass, MonitorPeripheral
    if (VideoPortGetDeviceData(HwDeviceExtension,
                               VpMonitorData,
                               GenFbGetDeviceDataCallback,
                                   NULL /*&DeviceExtension->DisplayInfo*/) != NO_ERROR)
    {
        /* Ignore if no monitor data is given */
        DPRINT1("GenFbVmp: Optional monitor info not found\n");
    }
#endif

    /* Copy the boot display data for the device extension,
     * and configure the video adapter */
    VideoPortMoveMemory(&DeviceExtension->DisplayInfo,
                        &gBootDisplay, sizeof(gBootDisplay));
    return GenFbVmpConfigureAdapter(HwDeviceExtension, ConfigInfo);
}
#endif // DRIVER_PNP

#if (DRIVER_MODE & DRIVER_LEGACY)
/**
 * @brief
 * Find the video adapter corresponding to the detected boot display.
 **/
CODE_SEG("PAGE")
VP_STATUS NTAPI
GenFbVmpFindBootDisplayAdapter(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID HwContext,
    _In_ PWSTR ArgumentString,
    _Inout_ PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    _In_ PUCHAR Again)
{
    PGENFB_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
    VP_STATUS status;

    PAGED_CODE();

    DPRINT1("GenFbVmpFindBootDisplayAdapter(0x%p, 0x%p, %s, 0x%p, 0x%p)\n",
        HwDeviceExtension, HwContext, ArgumentString, ConfigInfo, Again);

    *Again = FALSE;

    if (ConfigInfo->Length < sizeof(*ConfigInfo))
        return ERROR_INVALID_PARAMETER;

    //
    // TODO: Detect whether this is the detection for the boot-time console
    // that has been searched for during initialization. If we are in PnP mode
    // just skip the detection.
    //

DPRINT1("ConfigInfo->AdapterInterfaceType: %lu; ConfigInfo->SystemIoBusNumber: %lu\n",
    ConfigInfo->AdapterInterfaceType, ConfigInfo->SystemIoBusNumber);

    if ((gBootDisplay.Interface != InterfaceTypeUndefined) &&
        (gBootDisplay.Interface != ConfigInfo->AdapterInterfaceType) /* &&
        (gBootDisplay.BusNumber != ConfigInfo->SystemIoBusNumber) */)
    {
        /* The boot display controller is not on this bus */
        return ERROR_DEV_NOT_EXIST;
    }

    // Now, either (gBootDisplay.Interface == InterfaceTypeUndefined) ,
    // or (gBootDisplay.Interface == ConfigInfo->AdapterInterfaceType) ...

    DBG_UNREFERENCED_LOCAL_VARIABLE(status);

    /*
     * Retrieve any configuration data for the display controller
     */

    if (gBootDisplay.Interface == InterfaceTypeUndefined)
    {
        /* Check whether the boot display controller is on this bus */

        /* Enumerate and find the boot-time console display controller
         * (ControllerClass, DisplayController) */
        if (VideoPortGetDeviceData(HwDeviceExtension,
                                   VpControllerData,
                                   GenFbGetDeviceDataCallback,
                                   NULL /*&DeviceExtension->DisplayInfo*/) != NO_ERROR)
        {
            DPRINT1("GenFbVmp: Getting controller info failed\n");
            return ERROR_DEV_NOT_EXIST;
        }

        /* Check whether the reported video data matches the boot display info */
        if (DeviceExtension->DisplayInfo.VramAddress.QuadPart != gBootDisplay.VramAddress.QuadPart ||
            DeviceExtension->DisplayInfo.VramSize != gBootDisplay.VramSize)
        {
            DPRINT1("GenFbVmp: Display controller info doesn't match boot display info\n");
            return ERROR_DEV_NOT_EXIST;
        }

        /* Save the actual interface and bus number */
        gBootDisplay.Interface = ConfigInfo->AdapterInterfaceType;
        gBootDisplay.BusNumber = ConfigInfo->SystemIoBusNumber;
    }
    else
    {
        /* The boot display controller interface is known and is the one
         * currently being enumerated. Its configuration data is already
         * known... */
    }

#if 0
    /* Extra checks when on PCI bus... */
    if (gBootDisplay.Interface == PCIBus)
    {
        PCI_COMMON_CONFIG Config;
        ULONG ReturnedLength;

        // status = VideoPortGetAccessRanges(HwDeviceExtension,
                                          // 0,
                                          // NULL,
                                          // NUM_S3_PCI_ACCESS_RANGES,
                                          // &accessRange[LINEAR_FRAME_BUF],
                                          // NULL,
                                          // NULL,
                                          // &Slot);

        /* NOTE: This function calls internally HalGetBusDataByOffset().
         * The bus and slot number are internally given via the data stored
         * in the video device extension. */
        ReturnedLength = VideoPortGetBusData(HwDeviceExtension,
                                             PCIConfiguration,
                                             0,
                                             &Config,
                                             0, // Zero offset
                                             sizeof(Config));

        PCI_COMMON_HDR_LENGTH

        if (ReturnedLength != sizeof(Config))
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }


        if (((Config.BaseClass == PCI_CLASS_PRE_20) &&
             (Config.SubClass  == PCI_SUBCLASS_PRE_20_VGA)) ||
            ((Config.BaseClass == PCI_CLASS_DISPLAY_CTLR) &&
             (Config.SubClass  == PCI_SUBCLASS_VID_VGA_CTLR)))
        {
        }

    }
#endif

    gbBootDisplayReported = TRUE;

    /* Copy the boot display data for the device extension,
     * and configure the video adapter */
    VideoPortMoveMemory(&DeviceExtension->DisplayInfo,
                        &gBootDisplay, sizeof(gBootDisplay));
    return GenFbVmpConfigureAdapter(HwDeviceExtension, ConfigInfo);
}
#endif // DRIVER_LEGACY

CODE_SEG("PAGE")
BOOLEAN NTAPI
GenFbVmpInitialize(
    _In_ PVOID HwDeviceExtension)
{
    PGENFB_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    PAGED_CODE();

    /* Zero the frame buffer */
    //VideoPortZeroDeviceMemory(DeviceExtension->DisplayInfo.FrameAddress,
    //                          DeviceExtension->DisplayInfo.FrameBufferSize);
    RtlFillMemoryUlong(DeviceExtension->DisplayInfo.FrameAddress, // FIXME: Testing purposes!
                       DeviceExtension->DisplayInfo.FrameBufferSize, 0xFF0000);

    return TRUE;
}

#if DBG
CODE_SEG("PAGE")
static PCSTR
GenFbVmpIOControlName(
    _In_ ULONG IoControlCode)
{
    static CHAR UnknownIOCTL[sizeof("Unknown IOCTL 0xFFFFFFFF")];

    PAGED_CODE();

#define DBG_IOCTL(ioctl) \
    case ioctl: return #ioctl

    switch (IoControlCode)
    {
    DBG_IOCTL(IOCTL_VIDEO_ENABLE_VDM);
    DBG_IOCTL(IOCTL_VIDEO_DISABLE_VDM);
    DBG_IOCTL(IOCTL_VIDEO_REGISTER_VDM);
    DBG_IOCTL(IOCTL_VIDEO_SET_OUTPUT_DEVICE_POWER_STATE);
    DBG_IOCTL(IOCTL_VIDEO_GET_OUTPUT_DEVICE_POWER_STATE);
    DBG_IOCTL(IOCTL_VIDEO_MONITOR_DEVICE);
    DBG_IOCTL(IOCTL_VIDEO_ENUM_MONITOR_PDO);
    DBG_IOCTL(IOCTL_VIDEO_INIT_WIN32K_CALLBACKS);
    DBG_IOCTL(IOCTL_VIDEO_HANDLE_VIDEOPARAMETERS);
    DBG_IOCTL(IOCTL_VIDEO_IS_VGA_DEVICE);
    DBG_IOCTL(IOCTL_VIDEO_USE_DEVICE_IN_SESSION);
    DBG_IOCTL(IOCTL_VIDEO_PREPARE_FOR_EARECOVERY);
    DBG_IOCTL(IOCTL_VIDEO_DISABLE_CURSOR);
    DBG_IOCTL(IOCTL_VIDEO_DISABLE_POINTER);
    DBG_IOCTL(IOCTL_VIDEO_ENABLE_CURSOR);
    DBG_IOCTL(IOCTL_VIDEO_ENABLE_POINTER);
    DBG_IOCTL(IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES);
    DBG_IOCTL(IOCTL_VIDEO_GET_BANK_SELECT_CODE);
    DBG_IOCTL(IOCTL_VIDEO_GET_CHILD_STATE);
    DBG_IOCTL(IOCTL_VIDEO_GET_POWER_MANAGEMENT);
    DBG_IOCTL(IOCTL_VIDEO_LOAD_AND_SET_FONT);
    DBG_IOCTL(IOCTL_VIDEO_MAP_VIDEO_MEMORY);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_AVAIL_MODES);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_CURRENT_MODE);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_CURSOR_ATTR);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_CURSOR_POSITION);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_POINTER_ATTR);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_POINTER_POSITION);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES);
    DBG_IOCTL(IOCTL_VIDEO_RESET_DEVICE);
    DBG_IOCTL(IOCTL_VIDEO_RESTORE_HARDWARE_STATE);
    DBG_IOCTL(IOCTL_VIDEO_SAVE_HARDWARE_STATE);
    DBG_IOCTL(IOCTL_VIDEO_SET_CHILD_STATE_CONFIGURATION);
    DBG_IOCTL(IOCTL_VIDEO_SET_COLOR_REGISTERS);
    DBG_IOCTL(IOCTL_VIDEO_SET_CURRENT_MODE);
    DBG_IOCTL(IOCTL_VIDEO_SET_CURSOR_ATTR);
    DBG_IOCTL(IOCTL_VIDEO_SET_CURSOR_POSITION);
    DBG_IOCTL(IOCTL_VIDEO_SET_PALETTE_REGISTERS);
    DBG_IOCTL(IOCTL_VIDEO_SET_POINTER_ATTR);
    DBG_IOCTL(IOCTL_VIDEO_SET_POINTER_POSITION);
    DBG_IOCTL(IOCTL_VIDEO_SET_POWER_MANAGEMENT);
    DBG_IOCTL(IOCTL_VIDEO_SHARE_VIDEO_MEMORY);
    DBG_IOCTL(IOCTL_VIDEO_SWITCH_DUALVIEW);
    DBG_IOCTL(IOCTL_VIDEO_UNMAP_VIDEO_MEMORY);
    DBG_IOCTL(IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY);
    DBG_IOCTL(IOCTL_VIDEO_SET_COLOR_LUT_DATA);
    DBG_IOCTL(IOCTL_VIDEO_VALIDATE_CHILD_STATE_CONFIGURATION);
    DBG_IOCTL(IOCTL_VIDEO_SET_BANK_POSITION);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS);
    DBG_IOCTL(IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS);
    DBG_IOCTL(IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS);

    default:
        sprintf/*RtlStringCbPrintfA*/(UnknownIOCTL/*, sizeof(UnknownIOCTL)*/, "Unknown IOCTL 0x%lx", IoControlCode);
        return UnknownIOCTL;
    }
#undef DBG_IOCTL
}
#endif

CODE_SEG("PAGE")
VP_STATUS NTAPI
GenFbVmpSetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _In_ PVIDEO_POWER_MANAGEMENT VideoPowerControl);

CODE_SEG("PAGE")
VP_STATUS NTAPI
GenFbVmpGetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _Out_ PVIDEO_POWER_MANAGEMENT VideoPowerControl);

CODE_SEG("PAGE")
BOOLEAN NTAPI
GenFbVmpStartIO(
    _In_ PVOID HwDeviceExtension,
    _Inout_ PVIDEO_REQUEST_PACKET RequestPacket)
{
    PGENFB_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
    PGENFB_DISPLAY_INFO DisplayInfo = &DeviceExtension->DisplayInfo;
    PSTATUS_BLOCK StatusBlock = RequestPacket->StatusBlock;
    VP_STATUS Status = ERROR_INVALID_PARAMETER;

    PAGED_CODE();

#if DBG
    DPRINT1("GenFbVmpStartIO: %s\n", GenFbVmpIOControlName(RequestPacket->IoControlCode));
#endif
    switch (RequestPacket->IoControlCode)
    {
// IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES   Non-Modal   Optional
// IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES    Non-Modal   Optional

        case IOCTL_VIDEO_SET_CURRENT_MODE:
        {
            PVIDEO_MODE RequestedMode = (PVIDEO_MODE)RequestPacket->InputBuffer;
            ULONG RequestedModeNum;

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            RequestedModeNum = RequestedMode->RequestedMode &
                ~(VIDEO_MODE_NO_ZERO_MEMORY | VIDEO_MODE_MAP_MEM_LINEAR);

            /* Nothing to do. We only support one single mode
             * and we are already in that mode. */
            if (RequestedModeNum != 0)
            {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
            /* We only support linear framebuffer memory format */
            if (RequestedMode->RequestedMode & ~VIDEO_MODE_MAP_MEM_LINEAR)
            {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }

            // TODO: Introduce and invoke a hardware callback to reset the adapter state.

            /* Zero the frame buffer if requested */
            if (!(RequestedMode->RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY))
            {
                VideoPortZeroDeviceMemory(DeviceExtension->DisplayInfo.FrameAddress,
                                          DeviceExtension->DisplayInfo.FrameBufferSize);
            }

            Status = NO_ERROR;
            break;
        }

        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
        {
            PVIDEO_MEMORY RequestedAddress = (PVIDEO_MEMORY)RequestPacket->InputBuffer;
            PVIDEO_MEMORY_INFORMATION MapInformation = (PVIDEO_MEMORY_INFORMATION)RequestPacket->OutputBuffer;

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY) ||
                RequestPacket->OutputBufferLength < sizeof(VIDEO_MEMORY_INFORMATION))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            /* Map the video RAM (set up by the firmware) to the
             * preferred address the user requests, if possible. */
            MapInformation->VideoRamBase = RequestedAddress->RequestedVirtualAddress;
            MapInformation->VideoRamLength = DisplayInfo->VramSize;

            Status = GenFbVmpMapVideoMemory(DeviceExtension,
                                            NULL,
                                            DisplayInfo->VramAddress,
                                            &MapInformation->VideoRamLength,
                                            &MapInformation->VideoRamBase);
            if (Status != NO_ERROR)
            {
                DPRINT1("Failed to map video RAM 0x%I64X (%lu bytes)\n",
                        DisplayInfo->VramAddress.QuadPart, DisplayInfo->VramSize);
                StatusBlock->Information = 0;
                break;
            }

            /* The framebuffer starts at a given offset from the VideoRamBase */
            MapInformation->FrameBufferBase =
                (PVOID)((ULONG_PTR)MapInformation->VideoRamBase + DisplayInfo->VideoConfigData.FrameBufferOffset);
            MapInformation->FrameBufferLength = DisplayInfo->FrameBufferSize;

            DPRINT1("Video RAM 0x%I64X mapped to 0x%p (%lu bytes) - FrameBuffer at 0x%p (%lu bytes)\n",
                    DisplayInfo->VramAddress.QuadPart,
                    MapInformation->VideoRamBase, MapInformation->VideoRamLength,
                    MapInformation->FrameBufferBase, MapInformation->FrameBufferLength);

            StatusBlock->Information = sizeof(*MapInformation);
            break;
        }

        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
        {
            PVIDEO_MEMORY VideoMemory = (PVIDEO_MEMORY)RequestPacket->InputBuffer;

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            Status = GenFbVmpUnmapVideoMemory(
                        HwDeviceExtension,
                        VideoMemory->RequestedVirtualAddress, NULL);
            break;
        }

        case IOCTL_VIDEO_SHARE_VIDEO_MEMORY:
        {
            PVIDEO_SHARE_MEMORY VideoShareMemory = (PVIDEO_SHARE_MEMORY)RequestPacket->InputBuffer;
            PVIDEO_SHARE_MEMORY_INFORMATION VideoShareMemoryInfo = (PVIDEO_SHARE_MEMORY_INFORMATION)RequestPacket->OutputBuffer;

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_SHARE_MEMORY) ||
                RequestPacket->OutputBufferLength < sizeof(VIDEO_SHARE_MEMORY_INFORMATION))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            PHYSICAL_ADDRESS FrameBuffer;

            FrameBuffer.QuadPart = DisplayInfo->VramAddress.QuadPart + DisplayInfo->VideoConfigData.FrameBufferOffset; // PhysicalFrameBase;

            /* Verify that the framebuffer actually fits inside the video RAM */
            // if (FrameBuffer.QuadPart + FrameBufferSize > VramAddress.QuadPart + VramSize)
            if ((VideoShareMemory->ViewOffset > DisplayInfo->VramSize) ||
                ((VideoShareMemory->ViewOffset + VideoShareMemory->ViewSize) > DisplayInfo->VramSize))
            {
                DPRINT1("The framebuffer exceeds video memory bounds!\n");
                Status = ERROR_INVALID_PARAMETER;
                break;
            }

            /*
             * Remarks:
             * VideoShareMemory->ViewOffset is *NOT* used as input for the mapping.
             * VideoShareMemory->RequestedVirtualAddress *CANNOT BE USED* by this IOCTL.
             */

            // TODO: Sample codes mention "the input buffer and the output buffer
            // are the same buffer", which isn't completely accurate, and mean
            // that callers *CAN* specify the same buffer. We should need to handle
            // this case and cache the input data.

            PVOID VirtualAddress;
            VideoShareMemoryInfo->SharedViewSize = VideoShareMemory->ViewSize;
            VirtualAddress = NULL; // VideoShareMemory->RequestedVirtualAddress;
            Status = GenFbVmpMapVideoMemory(DeviceExtension,
                                            VideoShareMemory->ProcessHandle,
                                            FrameBuffer,
                                            &VideoShareMemoryInfo->SharedViewSize,
                                            &VirtualAddress);
            if (Status != NO_ERROR)
            {
                DPRINT1("Failed to map video RAM 0x%I64X (%lu bytes)\n",
                        DisplayInfo->VramAddress.QuadPart, DisplayInfo->VramSize);
                StatusBlock->Information = 0;
                break;
            }

            /*
             * See
             * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddvdeo/ns-ntddvdeo-_video_share_memory_information#remarks
             */
            VideoShareMemoryInfo->SharedViewOffset = VideoShareMemory->ViewOffset;
            // VideoShareMemoryInfo->SharedViewSize; // Retrieved above.
            VideoShareMemoryInfo->VirtualAddress = VirtualAddress;

            StatusBlock->Information = sizeof(*VideoShareMemoryInfo);
            break;
        }

        case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:
        {
            PVIDEO_SHARE_MEMORY VideoShareMemory = (PVIDEO_SHARE_MEMORY)RequestPacket->InputBuffer;

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_SHARE_MEMORY))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            /* ViewOffset, ViewSize, are unused in this IOCTL */
            Status = GenFbVmpUnmapVideoMemory(
                        HwDeviceExtension,
                        VideoShareMemory->RequestedVirtualAddress,
                        VideoShareMemory->ProcessHandle);
            break;
        }

        case IOCTL_VIDEO_RESET_DEVICE:
        {
            /* Zero the frame buffer */
__debugbreak();
            VideoPortZeroDeviceMemory(DeviceExtension->DisplayInfo.FrameAddress,
                                      DeviceExtension->DisplayInfo.FrameBufferSize);
            /* Nothing more to be done here */
            Status = NO_ERROR;
            break;
        }

        case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
        {
            PVIDEO_NUM_MODES Modes = (PVIDEO_NUM_MODES)RequestPacket->OutputBuffer;

            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_NUM_MODES))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            /* We only support one single mode set at boot time */
            Modes->NumModes = 1;
            Modes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);
            StatusBlock->Information = sizeof(*Modes);
            Status = NO_ERROR;
            break;
        }

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:
            /* Since we support only one single mode, return
             * only that mode that is also the active one. */
        case IOCTL_VIDEO_QUERY_CURRENT_MODE:
        {
            PVIDEO_MODE_INFORMATION VideoMode = (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer;

            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            /* Copy back our existing current mode */
            VideoPortMoveMemory(VideoMode,
                                &DeviceExtension->CurrentVideoMode,
                                sizeof(*VideoMode));
            StatusBlock->Information = sizeof(*VideoMode);
            Status = NO_ERROR;
            break;
        }

#if (DRIVER_MODE & DRIVER_PNP)
        case IOCTL_VIDEO_GET_CHILD_STATE:
        {
            // PULONG pChildIndex, pChildState;

            if (RequestPacket->InputBufferLength < sizeof(ULONG) ||
                RequestPacket->OutputBufferLength < sizeof(ULONG))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            // pChildIndex = RequestPacket->InputBuffer;
            // pChildState = RequestPacket->OutputBuffer;

            /* Always say the child is active */
            *((ULONG *)RequestPacket->OutputBuffer) = VIDEO_CHILD_ACTIVE;
            StatusBlock->Information = sizeof(ULONG);
            Status = NO_ERROR;
            break;
        }
#endif // DRIVER_PNP

        /* Obsolete in Win2000+; just call the HwGetPowerState handler */
        case IOCTL_VIDEO_GET_POWER_MANAGEMENT:
        {
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_POWER_MANAGEMENT))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            /*
             * 2nd parameter "HwId":
             * see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/video/nc-video-pvideo_hw_power_get#parameters
             */
            Status = GenFbVmpGetPowerState(HwDeviceExtension,
                                           DISPLAY_ADAPTER_HW_ID,
                                           (PVIDEO_POWER_MANAGEMENT)RequestPacket->OutputBuffer);
            StatusBlock->Information = (Status == NO_ERROR ? sizeof(VIDEO_POWER_MANAGEMENT) : 0);
            break;
        }

        /* Obsolete in Win2000+; just call the HwSetPowerState handler */
        case IOCTL_VIDEO_SET_POWER_MANAGEMENT:
        {
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_POWER_MANAGEMENT))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            /*
             * 2nd parameter "HwId":
             * see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/video/nc-video-pvideo_hw_power_set#parameters
             */
            Status = GenFbVmpSetPowerState(HwDeviceExtension,
                                           DISPLAY_ADAPTER_HW_ID,
                                           (PVIDEO_POWER_MANAGEMENT)RequestPacket->InputBuffer);
            break;
        }

        default:
        {
            // DPRINT1("GenFbVmpStartIO: Unhandled IOCTL 0x%x\n", RequestPacket->IoControlCode);
#if DBG
            DPRINT1("GenFbVmpStartIO: %s\n", GenFbVmpIOControlName(RequestPacket->IoControlCode));
#endif
            StatusBlock->Information = 0;
            Status = ERROR_INVALID_FUNCTION;
            break;
        }
    }

    StatusBlock->Status = Status;
    return TRUE;
}

/**
 * @brief   Set the device power state.
 **/
CODE_SEG("PAGE")
VP_STATUS NTAPI
GenFbVmpSetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _In_ PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    PAGED_CODE();

    /* Unused */
    DPRINT1("GenFbVmpSetPowerState(%p)\n", HwDeviceExtension);
    __debugbreak();
    return NO_ERROR;
}

/**
 * @brief   Validate support for a requested power state.
 **/
CODE_SEG("PAGE")
VP_STATUS NTAPI
GenFbVmpGetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _Out_ PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    PAGED_CODE();

    /* Unused */
    DPRINT1("GenFbVmpGetPowerState(%p)\n", HwDeviceExtension);
    __debugbreak();
    return ERROR_DEVICE_REINITIALIZATION_NEEDED; // NO_ERROR;
}

#if (DRIVER_MODE & DRIVER_PNP)
/**
 * @brief
 * Return child device descriptors. In this case just a single monitor.
 **/
CODE_SEG("PAGE")
VP_STATUS NTAPI
GenFbVmpGetVideoChildDescriptor(
    _In_ PVOID HwDeviceExtension,
    _In_ PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    _Out_ PVIDEO_CHILD_TYPE VideoChildType,
    _Out_ PUCHAR pChildDescriptor,
    _Out_ PULONG UId,
    _Out_ PULONG pUnused)
{
    PAGED_CODE();

    /* Unused */
    DPRINT1("GenFbVmpGetVideoChildDescriptor(%p)\n", HwDeviceExtension);
    __debugbreak();

    if (ChildEnumInfo->Size < sizeof(*ChildEnumInfo))
        return ERROR_INVALID_PARAMETER; // VIDEO_ENUM_NO_MORE_DEVICES; // VIDEO_ENUM_INVALID_DEVICE;

    *pUnused = 0;

// https://git.reactos.org/?p=reactos.git;a=blob;f=win32ss/drivers/miniport/bochs/bochsmp.c;hb=b03c40f440b1d6b4a0dc7025230d4e5798ca323e#l700
// https://git.reactos.org/?p=reactos.git;a=blob;f=win32ss/drivers/miniport/vbe/edid.c;hb=b03c40f440b1d6b4a0dc7025230d4e5798ca323e#l229

// If ChildIndex is set to zero, the driver should use the value specified in ACPIHwId as the ID of the device being enumerated.

    /*
     * ChildIndex set to 0 is used to enumerate devices found by the ACPI firmware,
     * which we do not currently support. Treat ChildIndex >= 1 as the monitor(s).
     */
    if (ChildEnumInfo->ChildIndex == DISPLAY_ADAPTER_HW_ID)
    {
        *VideoChildType = VideoChip;
        return VIDEO_ENUM_MORE_DEVICES;
    }
    else if (ChildEnumInfo->ChildIndex > 0)
    {
        /* Only support one attached monitor */
        if (ChildEnumInfo->ChildIndex <= /*pExt->NumMonitors*/ 1)
        {
            *VideoChildType = Monitor;
            *UId = ChildEnumInfo->ChildIndex; // Use the index as the ID.
            return VIDEO_ENUM_MORE_DEVICES;
        }
    }

    return VIDEO_ENUM_NO_MORE_DEVICES;
}
#endif // DRIVER_PNP

CODE_SEG("INIT")
ULONG NTAPI
DriverEntry(
    _In_ PVOID Context1,
    _In_ PVOID Context2)
{
    VIDEO_HW_INITIALIZATION_DATA VideoInitData;
    ULONG OsMajorVersion = 0, OsMinorVersion = 0, OsBuildNumber = 0;
    NTSTATUS Status;

    __debugbreak();

    /* Get operating system version.
     * NOTE: The PsGetVersion function was not available in NT 3.x. */
    PsGetVersion(&OsMajorVersion, &OsMinorVersion, &OsBuildNumber, NULL);
    VideoDebugPrint((Info, "GenFbVmp: Running on NT %lu.%lu build %lu\n",
                     OsMajorVersion, OsMinorVersion, OsBuildNumber));

    /* Find boot-time framebuffer display information from the LoaderBlock */
    VideoPortZeroMemory(&gBootDisplay, sizeof(gBootDisplay));
    Status = FindBootDisplay(&gBootDisplay.VramAddress,
                             &gBootDisplay.VramSize,
                             &gBootDisplay.VideoConfigData,
                             &gBootDisplay.MonitorConfigData,
                             &gBootDisplay.Interface,
                             &gBootDisplay.BusNumber);
    gbBootDisplayFound = NT_SUCCESS(Status);

    if (!gbBootDisplayFound)
        DPRINT1("No boot framebuffer detected.\n");
    else
        DPRINT1("Boot framebuffer detected.\n");

#if !(DRIVER_MODE & DRIVER_PNP)
    /* Legacy-only driver: if we haven't found the boot display, just bail out right now */
    if (!gbBootDisplayFound)
        return (ULONG)Status;
#endif

    /* Set up the initialization data structure; support older NT versions */
    VideoPortZeroMemory(&VideoInitData, sizeof(VideoInitData));

    if ((OsMajorVersion < 3) || // Windows NT 3.1
        (OsMajorVersion == 3) && (OsMinorVersion < 5))
    {
        // RTL_SIZEOF_THROUGH_FIELD(VIDEO_HW_INITIALIZATION_DATA, StartingDeviceNumber);
        VideoInitData.HwInitDataSize = FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwResetHw);
    }
    else if ((OsMajorVersion == 3) && (OsMinorVersion == 5)) // Windows NT 3.5
    {
        VideoInitData.HwInitDataSize = RTL_SIZEOF_THROUGH_FIELD(VIDEO_HW_INITIALIZATION_DATA, HwTimer);
    }
    else if ((OsMajorVersion == 3) && (OsMinorVersion > 5) || // Windows NT 3.51
             (OsMajorVersion == 4)) // Windows NT 4.x
    {
        VideoInitData.HwInitDataSize = SIZE_OF_NT4_VIDEO_HW_INITIALIZATION_DATA;
    }
    else if (OsMajorVersion == 5)
    {
        if (OsMinorVersion >= 1) // Windows NT 5.1+ (XP/2003)
            VideoInitData.HwInitDataSize = SIZE_OF_WXP_VIDEO_HW_INITIALIZATION_DATA;
        else // Windows NT 5.0 (2000)
            VideoInitData.HwInitDataSize = SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA;
    }
    else // if (OsMajorVersion >= 6) // Vista+
    {
        VideoInitData.HwInitDataSize = sizeof(VideoInitData);
    }

    VideoInitData.HwInitialize = GenFbVmpInitialize;
    // VideoInitData.HwInterrupt = NULL;
    VideoInitData.HwStartIO = GenFbVmpStartIO;
    VideoInitData.HwDeviceExtensionSize = sizeof(GENFB_DEVICE_EXTENSION);
    // VideoInitData.HwResetHw = NULL; // TODO
    // VideoInitData.HwTimer = NULL;
    // VideoInitData.HwStartDma = NULL;

    /* Start with parameters for Device0 */
    VideoInitData.StartingDeviceNumber = 0;

#if (DRIVER_MODE & DRIVER_LEGACY)
    /* Register, if any, the single boot-time framebuffer display controller
     * available on the system. Don't register ourselves as PnP for this call. */
    if (gbBootDisplayFound)
    {
        DPRINT1("Registering Boot Framebuffer.\n");
        VideoInitData.HwFindAdapter = GenFbVmpFindBootDisplayAdapter;

        /* If the interface on which the boot-time display controller
         * is unknown, loop on all the supported buses to initialize.
         * Otherwise if the interface is known, just call VideoPort on
         * this bus. */
        if (gBootDisplay.Interface == InterfaceTypeUndefined)
        {
            INTERFACE_TYPE Interface;
            for (Interface = 0; Interface < MaximumInterfaceType; ++Interface)
            {
                VideoInitData.AdapterInterfaceType = Interface;
                Status = VideoPortInitialize(Context1, Context2, &VideoInitData, NULL);
                if (Status == STATUS_SUCCESS)
                    break;
            }
        }
        else
        {
            VideoInitData.AdapterInterfaceType = gBootDisplay.Interface;
            Status = VideoPortInitialize(Context1, Context2, &VideoInitData, NULL);
        }
    }

#if (DRIVER_MODE & DRIVER_PNP)
    /* NT 3.x doesn't support PnP, so stop here */
    if (OsMajorVersion < 4)
#endif
        return (ULONG)Status;
#endif // DRIVER_LEGACY

#if (DRIVER_MODE & DRIVER_PNP)
    /* Now register as a PnP driver for handling any other framebuffer devices
     * that may be present on the system or could appear in the future. */
    VideoInitData.HwFindAdapter = GenFbVmpFindAdapter;
    VideoInitData.HwSetPowerState = GenFbVmpSetPowerState;
    VideoInitData.HwGetPowerState = GenFbVmpGetPowerState;
    VideoInitData.HwGetVideoChildDescriptor = GenFbVmpGetVideoChildDescriptor;

    // VideoInitData.HwQueryInterface = NULL;
    // VideoInitData.HwChildDeviceExtensionSize = 0;
    // VideoInitData.HwLegacyResourceList  = NULL;
    // VideoInitData.HwLegacyResourceCount = 0;
    // VideoInitData.HwGetLegacyResources  = NULL
    // VideoInitData.AllowEarlyEnumeration = FALSE;

    VideoInitData.AdapterInterfaceType = 0;
    Status = VideoPortInitialize(Context1, Context2, &VideoInitData, NULL);
    return (ULONG)Status;
#endif // DRIVER_PNP
}

/* EOF */
