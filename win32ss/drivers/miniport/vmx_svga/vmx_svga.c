/*
 * PROJECT:         ReactOS
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            win32ss/drivers/miniport/vmx_svga/vmx_svga.c
 * PURPOSE:         VMware SVGA-II video miniport
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

static WCHAR AdapterString[] = L"VMware SVGA II";

typedef struct _VMX_SIZE
{
    USHORT Width;
    USHORT Height;
} VMX_SIZE, *PVMX_SIZE;

static const VMX_SIZE VmxAvailableResolutions[] =
{
    { 640, 480 },
    { 800, 600 },
    { 1024, 600 },
    { 1024, 768 },
    { 1152, 864 },
    { 1280, 720 },
    { 1280, 768 },
    { 1280, 960 },
    { 1280, 1024 },
    { 1368, 768 },
    { 1400, 1050 },
    { 1440, 900 },
    { 1600, 900 },
    { 1600, 1200 },
    { 1680, 1050 },
    { 1920, 1080 },
    { 2048, 1536 },
    { 2560, 1440 },
    { 2560, 1600 },
    { 2560, 2048 },
    { 2800, 2100 },
    { 3200, 2400 },
    { 3840, 2160 },
};

static ULONG
VmxCountBits(ULONG Mask)
{
    ULONG Count = 0;

    while (Mask)
    {
        Count += Mask & 1;
        Mask >>= 1;
    }

    return Count;
}

ULONG
NTAPI
VmxReadUlong(IN PHW_DEVICE_EXTENSION DeviceExtension,
             IN ULONG Index)
{
    VideoPortWritePortUlong(DeviceExtension->IndexPort, Index);
    return VideoPortReadPortUlong(DeviceExtension->ValuePort);
}

VOID
NTAPI
VmxWriteUlong(IN PHW_DEVICE_EXTENSION DeviceExtension,
              IN ULONG Index,
              IN ULONG Value)
{
    VideoPortWritePortUlong(DeviceExtension->IndexPort, Index);
    VideoPortWritePortUlong(DeviceExtension->ValuePort, Value);
}

static VOID
VmxFreeAdapterMappings(IN PHW_DEVICE_EXTENSION DeviceExtension)
{
    if (DeviceExtension->FifoRange.Mapped)
    {
        VideoPortFreeDeviceBase(DeviceExtension, DeviceExtension->FifoRange.Mapped);
        DeviceExtension->FifoRange.Mapped = NULL;
        DeviceExtension->Fifo = NULL;
    }

    if (DeviceExtension->IoPorts.Mapped)
    {
        VideoPortFreeDeviceBase(DeviceExtension, DeviceExtension->IoPorts.Mapped);
        DeviceExtension->IoPorts.Mapped = NULL;
        DeviceExtension->IndexPort = NULL;
        DeviceExtension->ValuePort = NULL;
        DeviceExtension->InterruptPort = NULL;
    }
}

static VP_STATUS
VmxGetAdapterResources(IN PHW_DEVICE_EXTENSION DeviceExtension)
{
    VIDEO_ACCESS_RANGE AccessRanges[3];
    ULONG Index;
    ULONG MemoryRange = 0;
    VP_STATUS Status;

    VideoPortZeroMemory(AccessRanges, sizeof(AccessRanges));
    Status = VideoPortGetAccessRanges(DeviceExtension,
                                      0,
                                      NULL,
                                      sizeof(AccessRanges) / sizeof(AccessRanges[0]),
                                      AccessRanges,
                                      NULL,
                                      NULL,
                                      NULL);
    if (Status != NO_ERROR)
    {
        DPRINT1("VMX: VideoPortGetAccessRanges failed: 0x%lx\n", Status);
        return Status;
    }

    for (Index = 0; Index < sizeof(AccessRanges) / sizeof(AccessRanges[0]); Index++)
    {
        if (AccessRanges[Index].RangeLength == 0)
            continue;

        if (AccessRanges[Index].RangeInIoSpace)
        {
            if (DeviceExtension->IoPorts.RangeLength != 0)
            {
                DPRINT1("VMX: more than one I/O range was assigned\n");
                return ERROR_INVALID_PARAMETER;
            }

            DeviceExtension->IoPorts.RangeStart = AccessRanges[Index].RangeStart;
            DeviceExtension->IoPorts.RangeLength = AccessRanges[Index].RangeLength;
            DeviceExtension->IoPorts.RangeInIoSpace = AccessRanges[Index].RangeInIoSpace;
        }
        else if (MemoryRange++ == 0)
        {
            DeviceExtension->FrameBuffer.RangeStart = AccessRanges[Index].RangeStart;
            DeviceExtension->FrameBuffer.RangeLength = AccessRanges[Index].RangeLength;
            DeviceExtension->FrameBuffer.RangeInIoSpace = AccessRanges[Index].RangeInIoSpace;
        }
        else
        {
            DeviceExtension->FifoRange.RangeStart = AccessRanges[Index].RangeStart;
            DeviceExtension->FifoRange.RangeLength = AccessRanges[Index].RangeLength;
            DeviceExtension->FifoRange.RangeInIoSpace = AccessRanges[Index].RangeInIoSpace;
        }
    }

    if (DeviceExtension->IoPorts.RangeLength < (SVGA_VALUE_PORT + sizeof(ULONG)) ||
        DeviceExtension->FrameBuffer.RangeLength == 0 ||
        DeviceExtension->FifoRange.RangeLength == 0)
    {
        DPRINT1("VMX: incomplete PCI resources (io=%lu fb=%lu fifo=%lu)\n",
                DeviceExtension->IoPorts.RangeLength,
                DeviceExtension->FrameBuffer.RangeLength,
                DeviceExtension->FifoRange.RangeLength);
        return ERROR_DEV_NOT_EXIST;
    }

    DeviceExtension->IoPorts.Mapped = VideoPortGetDeviceBase(
        DeviceExtension,
        DeviceExtension->IoPorts.RangeStart,
        DeviceExtension->IoPorts.RangeLength,
        VIDEO_MEMORY_SPACE_IO);
    if (!DeviceExtension->IoPorts.Mapped)
    {
        DPRINT1("VMX: failed to map BAR0 I/O ports\n");
        return ERROR_DEV_NOT_EXIST;
    }

    DeviceExtension->IndexPort = (PULONG)(DeviceExtension->IoPorts.Mapped + SVGA_INDEX_PORT);
    DeviceExtension->ValuePort = (PULONG)(DeviceExtension->IoPorts.Mapped + SVGA_VALUE_PORT);

    if (DeviceExtension->IoPorts.RangeLength >= (SVGA_IRQSTATUS_PORT + sizeof(ULONG)))
        DeviceExtension->InterruptPort = (PULONG)(DeviceExtension->IoPorts.Mapped + SVGA_IRQSTATUS_PORT);

    return NO_ERROR;
}

static VOID
VmxReconcileMemoryRanges(IN PHW_DEVICE_EXTENSION DeviceExtension,
                         IN ULONG RegisterFrameBuffer,
                         IN ULONG RegisterFifo)
{
    VMX_ADDRESS_RANGE Temporary;

    if (RegisterFrameBuffer == DeviceExtension->FifoRange.RangeStart.LowPart &&
        RegisterFifo == DeviceExtension->FrameBuffer.RangeStart.LowPart)
    {
        Temporary = DeviceExtension->FrameBuffer;
        DeviceExtension->FrameBuffer = DeviceExtension->FifoRange;
        DeviceExtension->FifoRange = Temporary;
    }
}

VP_STATUS
NTAPI
VmxInitDevice(IN PHW_DEVICE_EXTENSION DeviceExtension)
{
    LONG Version;
    ULONG Id;
    ULONG FrameBufferStart;
    ULONG FifoStart;
    ULONG RegisterVramSize;
    ULONG RegisterFrameBufferSize;
    ULONG RegisterMemSize;

    DeviceExtension->Version = SVGA_ID_INVALID;

    for (Version = SVGA_VERSION_2; Version >= SVGA_VERSION_0; Version--)
    {
        Id = SVGA_MAKE_ID((ULONG)Version);
        VmxWriteUlong(DeviceExtension, SVGA_REG_ID, Id);
        if (VmxReadUlong(DeviceExtension, SVGA_REG_ID) == Id)
        {
            DeviceExtension->Version = (ULONG)Version;
            break;
        }
    }

    if (DeviceExtension->Version == SVGA_ID_INVALID)
    {
        DPRINT1("VMX: SVGA ID negotiation failed\n");
        return ERROR_DEV_NOT_EXIST;
    }

    /* ID 0 predates the command FIFO required by this driver. */
    if (DeviceExtension->Version < SVGA_VERSION_1)
    {
        DPRINT1("VMX: SVGA version %lu is too old\n", DeviceExtension->Version);
        return ERROR_INVALID_FUNCTION;
    }

    FrameBufferStart = VmxReadUlong(DeviceExtension, SVGA_REG_FB_START);
    FifoStart = VmxReadUlong(DeviceExtension, SVGA_REG_MEM_START);
    VmxReconcileMemoryRanges(DeviceExtension, FrameBufferStart, FifoStart);

    RegisterVramSize = VmxReadUlong(DeviceExtension, SVGA_REG_VRAM_SIZE);
    RegisterFrameBufferSize = VmxReadUlong(DeviceExtension, SVGA_REG_FB_SIZE);
    RegisterMemSize = VmxReadUlong(DeviceExtension, SVGA_REG_MEM_SIZE);

    DeviceExtension->VramSize = RegisterVramSize;
    if (DeviceExtension->VramSize == 0 ||
        DeviceExtension->VramSize > DeviceExtension->FrameBuffer.RangeLength)
    {
        DeviceExtension->VramSize = DeviceExtension->FrameBuffer.RangeLength;
    }

    DeviceExtension->FrameBufferSize = RegisterFrameBufferSize;
    if (DeviceExtension->FrameBufferSize == 0 ||
        DeviceExtension->FrameBufferSize > DeviceExtension->VramSize)
    {
        DeviceExtension->FrameBufferSize = DeviceExtension->VramSize;
    }

    DeviceExtension->MemSize = RegisterMemSize;
    if (DeviceExtension->MemSize == 0 ||
        DeviceExtension->MemSize > DeviceExtension->FifoRange.RangeLength)
    {
        DeviceExtension->MemSize = DeviceExtension->FifoRange.RangeLength;
    }

    if (DeviceExtension->MemSize < (SVGA_FIFO_CORE_REGS + 1) * sizeof(ULONG))
    {
        DPRINT1("VMX: FIFO BAR is too small (%lu bytes)\n", DeviceExtension->MemSize);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DeviceExtension->FifoRange.Mapped = VideoPortGetDeviceBase(
        DeviceExtension,
        DeviceExtension->FifoRange.RangeStart,
        DeviceExtension->MemSize,
        VIDEO_MEMORY_SPACE_MEMORY);
    if (!DeviceExtension->FifoRange.Mapped)
    {
        DPRINT1("VMX: failed to map FIFO BAR\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    DeviceExtension->Fifo = (PULONG)DeviceExtension->FifoRange.Mapped;

    DeviceExtension->Capabilities = VmxReadUlong(DeviceExtension, SVGA_REG_CAPABILITIES);
    DeviceExtension->MaxWidth = VmxReadUlong(DeviceExtension, SVGA_REG_MAX_WIDTH);
    DeviceExtension->MaxHeight = VmxReadUlong(DeviceExtension, SVGA_REG_MAX_HEIGHT);

    if (DeviceExtension->Capabilities & SVGA_CAP_8BIT_EMULATION)
        DeviceExtension->BitsPerPixel = VmxReadUlong(DeviceExtension, SVGA_REG_HOST_BITS_PER_PIXEL);
    else
        DeviceExtension->BitsPerPixel = VmxReadUlong(DeviceExtension, SVGA_REG_BITS_PER_PIXEL);

    DeviceExtension->Depth = VmxReadUlong(DeviceExtension, SVGA_REG_DEPTH);
    DeviceExtension->RedMask = VmxReadUlong(DeviceExtension, SVGA_REG_RED_MASK);
    DeviceExtension->GreenMask = VmxReadUlong(DeviceExtension, SVGA_REG_GREEN_MASK);
    DeviceExtension->BlueMask = VmxReadUlong(DeviceExtension, SVGA_REG_BLUE_MASK);

    if (DeviceExtension->MaxWidth < 640 ||
        DeviceExtension->MaxHeight < 480 ||
        DeviceExtension->BitsPerPixel < 15 ||
        DeviceExtension->BitsPerPixel > SVGA_MAX_BITS_PER_PIXEL)
    {
        DPRINT1("VMX: unsupported capabilities %lux%lu %lu-bpp\n",
                DeviceExtension->MaxWidth,
                DeviceExtension->MaxHeight,
                DeviceExtension->BitsPerPixel);
        return ERROR_INVALID_PARAMETER;
    }

    VmxWriteUlong(DeviceExtension, SVGA_REG_GUEST_ID, SVGA_GUEST_ID_REACTOS);

    DPRINT("VMX: SVGA%lu %lux%lu max, %lu-bpp, VRAM %lu, FIFO %lu\n",
           DeviceExtension->Version,
           DeviceExtension->MaxWidth,
           DeviceExtension->MaxHeight,
           DeviceExtension->BitsPerPixel,
           DeviceExtension->VramSize,
           DeviceExtension->MemSize);

    return NO_ERROR;
}

static BOOLEAN
VmxInitializeFifo(IN PHW_DEVICE_EXTENSION DeviceExtension)
{
    ULONG RegisterCount;
    ULONG Minimum;

    if (!DeviceExtension->Fifo)
        return FALSE;

    RegisterCount = VmxReadUlong(DeviceExtension, SVGA_REG_MEM_REGS);
    if (RegisterCount < SVGA_FIFO_CORE_REGS)
        RegisterCount = SVGA_FIFO_CORE_REGS;

    if (RegisterCount > (DeviceExtension->MemSize / sizeof(ULONG)) - 1)
    {
        DPRINT1("VMX: invalid FIFO register count %lu for %lu-byte BAR\n",
                RegisterCount,
                DeviceExtension->MemSize);
        return FALSE;
    }

    Minimum = RegisterCount * sizeof(ULONG);
    if (DeviceExtension->MemSize - Minimum < SVGA_FIFO_MIN_COMMAND_BYTES)
    {
        DPRINT1("VMX: FIFO command area is too small (%lu bytes)\n",
                DeviceExtension->MemSize - Minimum);
        return FALSE;
    }

    VmxWriteUlong(DeviceExtension, SVGA_REG_CONFIG_DONE, 0);
    VideoPortWriteRegisterUlong(DeviceExtension->Fifo + SVGA_FIFO_MIN, Minimum);
    VideoPortWriteRegisterUlong(DeviceExtension->Fifo + SVGA_FIFO_MAX, DeviceExtension->MemSize);
    VideoPortWriteRegisterUlong(DeviceExtension->Fifo + SVGA_FIFO_NEXT_CMD, Minimum);
    VideoPortWriteRegisterUlong(DeviceExtension->Fifo + SVGA_FIFO_STOP, Minimum);
    VmxWriteUlong(DeviceExtension, SVGA_REG_CONFIG_DONE, 1);

    DeviceExtension->FifoReady = TRUE;
    return TRUE;
}

static VOID
VmxFillModeInfo(IN PHW_DEVICE_EXTENSION DeviceExtension,
                IN ULONG Width,
                IN ULONG Height,
                IN ULONG ModeIndex,
                OUT PVIDEO_MODE_INFORMATION ModeInfo)
{
    ULONG BytesPerPixel = (DeviceExtension->BitsPerPixel + 7) / 8;

    VideoPortZeroMemory(ModeInfo, sizeof(*ModeInfo));
    ModeInfo->Length = sizeof(*ModeInfo);
    ModeInfo->ModeIndex = ModeIndex;
    ModeInfo->VisScreenWidth = Width;
    ModeInfo->VisScreenHeight = Height;
    ModeInfo->ScreenStride = Width * BytesPerPixel;
    ModeInfo->NumberOfPlanes = 1;
    ModeInfo->BitsPerPlane = DeviceExtension->BitsPerPixel;
    ModeInfo->Frequency = 60;
    ModeInfo->XMillimeter = Width * 254 / 960;
    ModeInfo->YMillimeter = Height * 254 / 960;
    ModeInfo->NumberRedBits = VmxCountBits(DeviceExtension->RedMask);
    ModeInfo->NumberGreenBits = VmxCountBits(DeviceExtension->GreenMask);
    ModeInfo->NumberBlueBits = VmxCountBits(DeviceExtension->BlueMask);
    ModeInfo->RedMask = DeviceExtension->RedMask;
    ModeInfo->GreenMask = DeviceExtension->GreenMask;
    ModeInfo->BlueMask = DeviceExtension->BlueMask;
    ModeInfo->AttributeFlags = VIDEO_MODE_GRAPHICS | VIDEO_MODE_COLOR | VIDEO_MODE_NO_OFF_SCREEN;
    ModeInfo->VideoMemoryBitmapWidth = Width;
    ModeInfo->VideoMemoryBitmapHeight = Height;
}

ULONG
NTAPI
VmxInitModes(IN PHW_DEVICE_EXTENSION DeviceExtension)
{
    ULONG Index;
    ULONG ModeCount = 0;
    ULONG BytesPerPixel = (DeviceExtension->BitsPerPixel + 7) / 8;

    for (Index = 0;
         Index < sizeof(VmxAvailableResolutions) / sizeof(VmxAvailableResolutions[0]) &&
         ModeCount < VMX_MAX_MODES;
         Index++)
    {
        ULONGLONG RequiredBytes;
        ULONG Width = VmxAvailableResolutions[Index].Width;
        ULONG Height = VmxAvailableResolutions[Index].Height;

        if (Width > DeviceExtension->MaxWidth || Height > DeviceExtension->MaxHeight)
            continue;

        RequiredBytes = (ULONGLONG)Width * Height * BytesPerPixel;
        if (RequiredBytes > DeviceExtension->FrameBufferSize)
            continue;

        VmxFillModeInfo(DeviceExtension,
                        Width,
                        Height,
                        ModeCount,
                        &DeviceExtension->Modes[ModeCount]);
        ModeCount++;
    }

    DeviceExtension->VideoModeCount = ModeCount;
    DeviceExtension->CurrentModeIndex = 0;
    return ModeCount;
}

static BOOLEAN
VmxFifoSubmitUpdate(IN PHW_DEVICE_EXTENSION DeviceExtension,
                    IN ULONG X,
                    IN ULONG Y,
                    IN ULONG Width,
                    IN ULONG Height)
{
    ULONG Minimum;
    ULONG Maximum;
    ULONG Next;
    ULONG Stop;
    ULONG FreeBytes;
    ULONG Values[5];
    ULONG Index;

    if (!DeviceExtension->FifoReady)
        return FALSE;

    Minimum = VideoPortReadRegisterUlong(DeviceExtension->Fifo + SVGA_FIFO_MIN);
    Maximum = VideoPortReadRegisterUlong(DeviceExtension->Fifo + SVGA_FIFO_MAX);
    Next = VideoPortReadRegisterUlong(DeviceExtension->Fifo + SVGA_FIFO_NEXT_CMD);
    Stop = VideoPortReadRegisterUlong(DeviceExtension->Fifo + SVGA_FIFO_STOP);

    if (Minimum < SVGA_FIFO_CORE_REGS * sizeof(ULONG) ||
        Maximum > DeviceExtension->MemSize ||
        Minimum >= Maximum ||
        Next < Minimum || Next >= Maximum ||
        Stop < Minimum || Stop >= Maximum)
    {
        return FALSE;
    }

    if (Next >= Stop)
        FreeBytes = (Maximum - Next) + (Stop - Minimum);
    else
        FreeBytes = Stop - Next;

    /* Keep one DWORD unused to distinguish a full FIFO from an empty FIFO. */
    if (FreeBytes < sizeof(Values) + sizeof(ULONG))
        return FALSE;

    Values[0] = SVGA_CMD_UPDATE;
    Values[1] = X;
    Values[2] = Y;
    Values[3] = Width;
    Values[4] = Height;

    for (Index = 0; Index < sizeof(Values) / sizeof(Values[0]); Index++)
    {
        VideoPortWriteRegisterUlong((PULONG)(DeviceExtension->FifoRange.Mapped + Next), Values[Index]);
        Next += sizeof(ULONG);
        if (Next == Maximum)
            Next = Minimum;
    }

    VideoPortWriteRegisterUlong(DeviceExtension->Fifo + SVGA_FIFO_NEXT_CMD, Next);
    return TRUE;
}

BOOLEAN
NTAPI
VmxIsMultiMon(IN PHW_DEVICE_EXTENSION DeviceExtension)
{
    if ((DeviceExtension->Capabilities & SVGA_CAP_MULTIMON) &&
        (DeviceExtension->Capabilities & SVGA_CAP_PITCHLOCK) &&
        VmxReadUlong(DeviceExtension, SVGA_REG_NUM_DISPLAYS) > 1)
    {
        return TRUE;
    }

    return FALSE;
}

static BOOLEAN
VmxMapVideoMemory(IN PHW_DEVICE_EXTENSION DeviceExtension,
                  IN PVIDEO_MEMORY RequestedAddress,
                  OUT PVIDEO_MEMORY_INFORMATION MapInformation,
                  OUT PSTATUS_BLOCK StatusBlock)
{
    VP_STATUS Status;
    ULONG MemorySpace = VIDEO_MEMORY_SPACE_MEMORY;
    ULONG FrameBufferLength;
    PVIDEO_MODE_INFORMATION ModeInfo;

    MapInformation->VideoRamBase = RequestedAddress->RequestedVirtualAddress;
    MapInformation->VideoRamLength = DeviceExtension->VramSize;

    Status = VideoPortMapMemory(DeviceExtension,
                                DeviceExtension->FrameBuffer.RangeStart,
                                &MapInformation->VideoRamLength,
                                &MemorySpace,
                                &MapInformation->VideoRamBase);
    if (Status != NO_ERROR)
    {
        StatusBlock->Status = Status;
        StatusBlock->Information = 0;
        return FALSE;
    }

    if (DeviceExtension->CurrentModeIndex >= DeviceExtension->VideoModeCount)
        DeviceExtension->CurrentModeIndex = 0;

    ModeInfo = &DeviceExtension->Modes[DeviceExtension->CurrentModeIndex];
    FrameBufferLength = ModeInfo->ScreenStride * ModeInfo->VisScreenHeight;

    if (DeviceExtension->FrameBufferOffset > MapInformation->VideoRamLength ||
        FrameBufferLength > MapInformation->VideoRamLength - DeviceExtension->FrameBufferOffset)
    {
        VideoPortUnmapMemory(DeviceExtension, MapInformation->VideoRamBase, NULL);
        StatusBlock->Status = ERROR_INVALID_PARAMETER;
        StatusBlock->Information = 0;
        return FALSE;
    }

    MapInformation->FrameBufferBase = (PUCHAR)MapInformation->VideoRamBase + DeviceExtension->FrameBufferOffset;
    MapInformation->FrameBufferLength = FrameBufferLength;
    StatusBlock->Status = NO_ERROR;
    StatusBlock->Information = sizeof(*MapInformation);
    return TRUE;
}

static BOOLEAN
VmxUnmapVideoMemory(IN PHW_DEVICE_EXTENSION DeviceExtension,
                    IN PVIDEO_MEMORY VideoMemory,
                    OUT PSTATUS_BLOCK StatusBlock)
{
    VP_STATUS Status;

    Status = VideoPortUnmapMemory(DeviceExtension,
                                  VideoMemory->RequestedVirtualAddress,
                                  NULL);
    StatusBlock->Status = Status;
    StatusBlock->Information = 0;
    return Status == NO_ERROR;
}

static BOOLEAN
VmxQueryNumAvailableModes(IN PHW_DEVICE_EXTENSION DeviceExtension,
                          OUT PVIDEO_NUM_MODES NumModes,
                          OUT PSTATUS_BLOCK StatusBlock)
{
    NumModes->NumModes = DeviceExtension->VideoModeCount;
    NumModes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);
    StatusBlock->Status = NO_ERROR;
    StatusBlock->Information = sizeof(*NumModes);
    return TRUE;
}

static BOOLEAN
VmxQueryAvailableModes(IN PHW_DEVICE_EXTENSION DeviceExtension,
                       OUT PVIDEO_MODE_INFORMATION ReturnedModes,
                       OUT PSTATUS_BLOCK StatusBlock)
{
    ULONG Index;

    for (Index = 0; Index < DeviceExtension->VideoModeCount; Index++)
        ReturnedModes[Index] = DeviceExtension->Modes[Index];

    StatusBlock->Status = NO_ERROR;
    StatusBlock->Information = DeviceExtension->VideoModeCount * sizeof(VIDEO_MODE_INFORMATION);
    return TRUE;
}

static BOOLEAN
VmxSetCurrentMode(IN PHW_DEVICE_EXTENSION DeviceExtension,
                  IN PVIDEO_MODE RequestedMode,
                  OUT PSTATUS_BLOCK StatusBlock)
{
    ULONG ModeIndex = RequestedMode->RequestedMode & 0x3FFFFFFF;
    PVIDEO_MODE_INFORMATION ModeInfo;
    ULONG Width;
    ULONG Height;
    ULONG BitsPerPixel;
    ULONG FrameBufferLength;

    if (ModeIndex >= DeviceExtension->VideoModeCount)
    {
        StatusBlock->Status = ERROR_INVALID_PARAMETER;
        StatusBlock->Information = 0;
        return FALSE;
    }

    ModeInfo = &DeviceExtension->Modes[ModeIndex];

    VmxWriteUlong(DeviceExtension, SVGA_REG_WIDTH, ModeInfo->VisScreenWidth);
    VmxWriteUlong(DeviceExtension, SVGA_REG_HEIGHT, ModeInfo->VisScreenHeight);
    if (DeviceExtension->Capabilities & SVGA_CAP_8BIT_EMULATION)
        VmxWriteUlong(DeviceExtension, SVGA_REG_BITS_PER_PIXEL, ModeInfo->BitsPerPlane);
    VmxWriteUlong(DeviceExtension, SVGA_REG_ENABLE, SVGA_REG_ENABLE_ENABLE);

    Width = VmxReadUlong(DeviceExtension, SVGA_REG_WIDTH);
    Height = VmxReadUlong(DeviceExtension, SVGA_REG_HEIGHT);
    BitsPerPixel = VmxReadUlong(DeviceExtension, SVGA_REG_BITS_PER_PIXEL);

    if (Width != ModeInfo->VisScreenWidth ||
        Height != ModeInfo->VisScreenHeight ||
        BitsPerPixel != ModeInfo->BitsPerPlane)
    {
        DPRINT1("VMX: mode %lu rejected (%lux%lu %lu-bpp reported)\n",
                ModeIndex,
                Width,
                Height,
                BitsPerPixel);
        StatusBlock->Status = ERROR_INVALID_PARAMETER;
        StatusBlock->Information = 0;
        return FALSE;
    }

    ModeInfo->ScreenStride = VmxReadUlong(DeviceExtension, SVGA_REG_BYTES_PER_LINE);
    if (ModeInfo->ScreenStride == 0)
        ModeInfo->ScreenStride = Width * ((BitsPerPixel + 7) / 8);

    ModeInfo->RedMask = VmxReadUlong(DeviceExtension, SVGA_REG_RED_MASK);
    ModeInfo->GreenMask = VmxReadUlong(DeviceExtension, SVGA_REG_GREEN_MASK);
    ModeInfo->BlueMask = VmxReadUlong(DeviceExtension, SVGA_REG_BLUE_MASK);
    ModeInfo->NumberRedBits = VmxCountBits(ModeInfo->RedMask);
    ModeInfo->NumberGreenBits = VmxCountBits(ModeInfo->GreenMask);
    ModeInfo->NumberBlueBits = VmxCountBits(ModeInfo->BlueMask);

    DeviceExtension->FrameBufferOffset = VmxReadUlong(DeviceExtension, SVGA_REG_FB_OFFSET);
    FrameBufferLength = ModeInfo->ScreenStride * ModeInfo->VisScreenHeight;
    if (DeviceExtension->FrameBufferOffset > DeviceExtension->FrameBufferSize ||
        FrameBufferLength > DeviceExtension->FrameBufferSize - DeviceExtension->FrameBufferOffset)
    {
        DPRINT1("VMX: mode %lu framebuffer exceeds VRAM\n", ModeIndex);
        VmxWriteUlong(DeviceExtension, SVGA_REG_ENABLE, SVGA_REG_ENABLE_DISABLE);
        StatusBlock->Status = ERROR_NOT_ENOUGH_MEMORY;
        StatusBlock->Information = 0;
        return FALSE;
    }

    DeviceExtension->CurrentModeIndex = ModeIndex;
    StatusBlock->Status = NO_ERROR;
    StatusBlock->Information = 0;

    /* A first damage notification makes the newly selected scanout visible. */
    VmxFifoSubmitUpdate(DeviceExtension, 0, 0, Width, Height);
    return TRUE;
}

static BOOLEAN
VmxQueryCurrentMode(IN PHW_DEVICE_EXTENSION DeviceExtension,
                    OUT PVIDEO_MODE_INFORMATION VideoModeInfo,
                    OUT PSTATUS_BLOCK StatusBlock)
{
    if (DeviceExtension->CurrentModeIndex >= DeviceExtension->VideoModeCount)
    {
        StatusBlock->Status = ERROR_INVALID_PARAMETER;
        StatusBlock->Information = 0;
        return FALSE;
    }

    *VideoModeInfo = DeviceExtension->Modes[DeviceExtension->CurrentModeIndex];
    StatusBlock->Status = NO_ERROR;
    StatusBlock->Information = sizeof(*VideoModeInfo);
    return TRUE;
}

static BOOLEAN
VmxResetDevice(IN PHW_DEVICE_EXTENSION DeviceExtension,
               OUT PSTATUS_BLOCK StatusBlock)
{
    UNREFERENCED_PARAMETER(DeviceExtension);

    StatusBlock->Status = NO_ERROR;
    StatusBlock->Information = 0;
    return TRUE;
}

static BOOLEAN
VmxGetChildState(OUT PULONG ChildState,
                 OUT PSTATUS_BLOCK StatusBlock)
{
    *ChildState = VIDEO_CHILD_ACTIVE;
    StatusBlock->Status = NO_ERROR;
    StatusBlock->Information = sizeof(*ChildState);
    return TRUE;
}

VP_STATUS
NTAPI
VmxFindAdapter(IN PVOID HwDeviceExtension,
               IN PVOID HwContext,
               IN PWSTR ArgumentString,
               IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
               OUT PUCHAR Again)
{
    VP_STATUS Status;
    PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    UNREFERENCED_PARAMETER(HwContext);
    UNREFERENCED_PARAMETER(ArgumentString);

    VideoPortZeroMemory(DeviceExtension, sizeof(*DeviceExtension));
    *Again = FALSE;

    if (ConfigInfo->Length < sizeof(*ConfigInfo))
        return ERROR_INVALID_PARAMETER;

    Status = VmxGetAdapterResources(DeviceExtension);
    if (Status != NO_ERROR)
        return Status;

    Status = VmxInitDevice(DeviceExtension);
    if (Status != NO_ERROR)
    {
        VmxFreeAdapterMappings(DeviceExtension);
        return Status;
    }

    if (VmxIsMultiMon(DeviceExtension))
        DPRINT1("VMX: multiple displays detected; baseline driver exposes the primary display only\n");

    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.ChipType",
                                   AdapterString,
                                   sizeof(AdapterString));
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.DacType",
                                   AdapterString,
                                   sizeof(AdapterString));
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &DeviceExtension->VramSize,
                                   sizeof(DeviceExtension->VramSize));
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.AdapterString",
                                   AdapterString,
                                   sizeof(AdapterString));
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.BiosString",
                                   AdapterString,
                                   sizeof(AdapterString));

    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;
    ConfigInfo->HardwareStateSize = 0;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.QuadPart = 0;
    ConfigInfo->VdmPhysicalVideoMemoryLength = 0;

    return NO_ERROR;
}

BOOLEAN
NTAPI
VmxInitialize(IN PVOID HwDeviceExtension)
{
    PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    if (!VmxInitializeFifo(DeviceExtension))
    {
        DPRINT1("VMX: FIFO initialization failed\n");
        VmxFreeAdapterMappings(DeviceExtension);
        return FALSE;
    }

    if (VmxInitModes(DeviceExtension) == 0)
    {
        DPRINT1("VMX: no usable display modes\n");
        VmxFreeAdapterMappings(DeviceExtension);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTAPI
VmxStartIO(IN PVOID HwDeviceExtension,
           IN PVIDEO_REQUEST_PACKET RequestPacket)
{
    PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    RequestPacket->StatusBlock->Status = ERROR_INVALID_FUNCTION;
    RequestPacket->StatusBlock->Information = 0;

    switch (RequestPacket->IoControlCode)
    {
        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY) ||
                RequestPacket->OutputBufferLength < sizeof(VIDEO_MEMORY_INFORMATION))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
            VmxMapVideoMemory(DeviceExtension,
                              (PVIDEO_MEMORY)RequestPacket->InputBuffer,
                              (PVIDEO_MEMORY_INFORMATION)RequestPacket->OutputBuffer,
                              RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
            VmxUnmapVideoMemory(DeviceExtension,
                                (PVIDEO_MEMORY)RequestPacket->InputBuffer,
                                RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_NUM_MODES))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
            VmxQueryNumAvailableModes(DeviceExtension,
                                      (PVIDEO_NUM_MODES)RequestPacket->OutputBuffer,
                                      RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:
            if (RequestPacket->OutputBufferLength <
                DeviceExtension->VideoModeCount * sizeof(VIDEO_MODE_INFORMATION))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
            VmxQueryAvailableModes(DeviceExtension,
                                   (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
                                   RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_SET_CURRENT_MODE:
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
            VmxSetCurrentMode(DeviceExtension,
                              (PVIDEO_MODE)RequestPacket->InputBuffer,
                              RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_QUERY_CURRENT_MODE:
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
            VmxQueryCurrentMode(DeviceExtension,
                                (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
                                RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_RESET_DEVICE:
            VmxResetDevice(DeviceExtension, RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_GET_CHILD_STATE:
            if (RequestPacket->OutputBufferLength < sizeof(ULONG))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
            VmxGetChildState((PULONG)RequestPacket->OutputBuffer,
                             RequestPacket->StatusBlock);
            break;

        default:
            DPRINT("VMX: unsupported IOCTL 0x%lx\n", RequestPacket->IoControlCode);
            break;
    }

    /* The request was synchronously completed; StatusBlock carries success/error. */
    return TRUE;
}

BOOLEAN
NTAPI
VmxResetHw(IN PVOID HwDeviceExtension,
           IN ULONG Columns,
           IN ULONG Rows)
{
    PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    UNREFERENCED_PARAMETER(Columns);
    UNREFERENCED_PARAMETER(Rows);

    if (DeviceExtension->IndexPort && DeviceExtension->ValuePort)
    {
        VmxWriteUlong(DeviceExtension, SVGA_REG_CONFIG_DONE, 0);
        VmxWriteUlong(DeviceExtension, SVGA_REG_ENABLE, SVGA_REG_ENABLE_DISABLE);
        DeviceExtension->FifoReady = FALSE;
    }

    return TRUE;
}

VP_STATUS
NTAPI
VmxGetPowerState(IN PVOID HwDeviceExtension,
                 IN ULONG HwId,
                 IN PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(HwId);
    UNREFERENCED_PARAMETER(VideoPowerControl);

    return ERROR_DEVICE_REINITIALIZATION_NEEDED;
}

VP_STATUS
NTAPI
VmxSetPowerState(IN PVOID HwDeviceExtension,
                 IN ULONG HwId,
                 IN PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(HwId);
    UNREFERENCED_PARAMETER(VideoPowerControl);

    return NO_ERROR;
}

BOOLEAN
NTAPI
VmxInterrupt(IN PVOID HwDeviceExtension)
{
    PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
    ULONG InterruptState;

    if (!(DeviceExtension->Capabilities & SVGA_CAP_IRQMASK) ||
        !DeviceExtension->InterruptPort)
    {
        return FALSE;
    }

    InterruptState = VideoPortReadPortUlong(DeviceExtension->InterruptPort);
    if (InterruptState == 0)
        return FALSE;

    DeviceExtension->InterruptState |= InterruptState;
    return TRUE;
}

VP_STATUS
NTAPI
VmxGetVideoChildDescriptor(IN PVOID HwDeviceExtension,
                           IN PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
                           OUT PVIDEO_CHILD_TYPE VideoChildType,
                           OUT PUCHAR pChildDescriptor,
                           OUT PULONG UId,
                           OUT PULONG pUnused)
{
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(pChildDescriptor);

    if (ChildEnumInfo->Size < sizeof(*VideoChildType))
        return VIDEO_ENUM_NO_MORE_DEVICES;

    if (ChildEnumInfo->ChildIndex == 0)
        return VIDEO_ENUM_INVALID_DEVICE;

    *pUnused = 0;

    if (ChildEnumInfo->ChildIndex == DISPLAY_ADAPTER_HW_ID)
    {
        *VideoChildType = VideoChip;
        return VIDEO_ENUM_MORE_DEVICES;
    }

    if (ChildEnumInfo->ChildIndex != 1)
        return VIDEO_ENUM_NO_MORE_DEVICES;

    *UId = 0;
    *VideoChildType = Monitor;
    return VIDEO_ENUM_MORE_DEVICES;
}

ULONG
NTAPI
DriverEntry(IN PVOID Context1,
            IN PVOID Context2)
{
    VIDEO_HW_INITIALIZATION_DATA InitData;

    VideoPortZeroMemory(&InitData, sizeof(InitData));

    InitData.HwInitDataSize = sizeof(InitData);
    InitData.HwFindAdapter = VmxFindAdapter;
    InitData.HwInitialize = VmxInitialize;
    InitData.HwInterrupt = VmxInterrupt;
    InitData.HwStartIO = VmxStartIO;
    InitData.HwResetHw = VmxResetHw;
    InitData.HwGetPowerState = VmxGetPowerState;
    InitData.HwSetPowerState = VmxSetPowerState;
    InitData.HwGetVideoChildDescriptor = VmxGetVideoChildDescriptor;
    InitData.AdapterInterfaceType = PCIBus;
    InitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    return VideoPortInitialize(Context1, Context2, &InitData, NULL);
}
