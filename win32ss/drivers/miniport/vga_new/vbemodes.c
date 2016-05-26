/*
 * PROJECT:         ReactOS VGA Miniport Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            win32ss/drivers/miniport/vga_new/vbemodes.c
 * PURPOSE:         Mode Initialization and Mode Set for VBE-compatible cards
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "vga.h"

/* FUNCTIONS ******************************************************************/

ULONG
NTAPI
RaiseToPower2Ulong(IN ULONG Value)
{
    ULONG SquaredResult = Value;
    if ((Value - 1) & Value) for (SquaredResult = 1; (SquaredResult < Value) && (SquaredResult); SquaredResult *= 2);
    return SquaredResult;
}

ULONG
NTAPI
RaiseToPower2(IN USHORT Value)
{
    ULONG SquaredResult = Value;
    if ((Value - 1) & Value) for (SquaredResult = 1; (SquaredResult < Value) && (SquaredResult); SquaredResult *= 2);
    return SquaredResult;
}

ULONG
NTAPI
VbeGetVideoMemoryBaseAddress(IN PHW_DEVICE_EXTENSION VgaExtension,
                             IN PVIDEOMODE VgaMode)
{
    ULONG Length = 4 * 1024;
    USHORT TrampolineMemorySegment, TrampolineMemoryOffset;
    PVOID Context;
    INT10_BIOS_ARGUMENTS BiosArguments;
    PVBE_MODE_INFO VbeModeInfo;
    ULONG BaseAddress;
    VP_STATUS Status;

    /* Need linear and INT10 interface */
    if (!(VgaMode->fbType & VIDEO_MODE_BANKED)) return 0;
    if (VgaExtension->Int10Interface.Size) return 0;

    /* Allocate scratch area and context */
    VbeModeInfo = VideoPortAllocatePool(VgaExtension, 1, sizeof(VBE_MODE_INFO), ' agV');
    if (!VbeModeInfo) return 0;
    Context = VgaExtension->Int10Interface.Context;
    Status = VgaExtension->Int10Interface.Int10AllocateBuffer(Context,
                                                              &TrampolineMemorySegment,
                                                              &TrampolineMemoryOffset,
                                                              &Length);
    if (Status != NO_ERROR) return 0;

    /* Ask VBE BIOS for mode info */
    VideoPortZeroMemory(&BiosArguments, sizeof(BiosArguments));
    BiosArguments.Ecx = HIWORD(VgaMode->Mode);
    BiosArguments.Edi = TrampolineMemorySegment;
    BiosArguments.SegEs = TrampolineMemoryOffset;
    BiosArguments.Eax = VBE_GET_MODE_INFORMATION;
    Status = VgaExtension->Int10Interface.Int10CallBios(Context, &BiosArguments);
    if (Status != NO_ERROR) return 0;
    if (VBE_GETRETURNCODE(BiosArguments.Eax) != VBE_SUCCESS)
        return 0;
    Status = VgaExtension->Int10Interface.Int10ReadMemory(Context,
                                                          TrampolineMemorySegment,
                                                          TrampolineMemoryOffset,
                                                          VbeModeInfo,
                                                          sizeof(VBE_MODE_INFO));
    if (Status != NO_ERROR) return 0;

    /* Return phys address and cleanup */
    BaseAddress = VbeModeInfo->PhysBasePtr;
    VgaExtension->Int10Interface.Int10FreeBuffer(Context,
                                                 TrampolineMemorySegment,
                                                 TrampolineMemoryOffset);
    VideoPortFreePool(VgaExtension, VbeModeInfo);
    return BaseAddress;
}

VP_STATUS
NTAPI
VbeSetMode(IN PHW_DEVICE_EXTENSION VgaDeviceExtension,
           IN PVIDEOMODE VgaMode,
           OUT PULONG PhysPtrChange)
{
    VP_STATUS Status;
    VIDEO_X86_BIOS_ARGUMENTS BiosArguments;
    ULONG ModeIndex;
    ULONG BaseAddress;

    VideoPortZeroMemory(&BiosArguments, sizeof(BiosArguments));
    ModeIndex = VgaMode->Mode;
    BiosArguments.Eax = VBE_SET_VBE_MODE;
    BiosArguments.Ebx = HIWORD(ModeIndex);
    VideoDebugPrint((0, "Switching to %lx %lx\n", BiosArguments.Eax, BiosArguments.Ebx));
    Status = VideoPortInt10(VgaDeviceExtension, &BiosArguments);
    if (Status != NO_ERROR) return Status;
    if(VBE_GETRETURNCODE(BiosArguments.Eax) != VBE_SUCCESS)
    {
        VideoDebugPrint((0, "Changing VBE mode failed, Eax %lx", BiosArguments.Eax));
        return ERROR_INVALID_PARAMETER;
    }

    /* Check for VESA mode */
    if (ModeIndex >> 16)
    {
        /* Mode set fail */
        if (VBE_GETRETURNCODE(BiosArguments.Eax) != VBE_SUCCESS)
            return ERROR_INVALID_PARAMETER;

        /* Check current mode is desired mode */
        VideoPortZeroMemory(&BiosArguments, sizeof(BiosArguments));
        BiosArguments.Eax = VBE_GET_CURRENT_VBE_MODE;
        Status = VideoPortInt10(VgaDeviceExtension, &BiosArguments);
        if ((Status == NO_ERROR) &&
            (VBE_GETRETURNCODE(BiosArguments.Eax) == VBE_SUCCESS) &&
            ((BiosArguments.Ebx ^ (ModeIndex >> 16)) & VBE_MODE_BITS))
        {
            return ERROR_INVALID_PARAMETER;
        }

        /* Set logical scanline width if different from physical */
        if (VgaMode->LogicalWidth != VgaMode->hres)
        {
            /* Check setting works after being set */
            VideoPortZeroMemory(&BiosArguments, sizeof(BiosArguments));
            BiosArguments.Eax = VBE_SET_GET_LOGICAL_SCAN_LINE_LENGTH;
            BiosArguments.Ecx = VgaMode->LogicalWidth;
            BiosArguments.Ebx = 0;
            Status = VideoPortInt10(VgaDeviceExtension, &BiosArguments);
            if ((Status != NO_ERROR) ||
                (VBE_GETRETURNCODE(BiosArguments.Eax) != VBE_SUCCESS) ||
                (BiosArguments.Ecx != VgaMode->LogicalWidth))
            {
                return ERROR_INVALID_PARAMETER;
            }
        }
    }

    /* Get VRAM address to update changes */
    BaseAddress = VbeGetVideoMemoryBaseAddress(VgaDeviceExtension, VgaMode);
    if ((BaseAddress) && (VgaMode->PhysBase != BaseAddress))
    {
        *PhysPtrChange = TRUE;
        VgaMode->PhysBase = BaseAddress;
    }

    return NO_ERROR;
}

VOID
NTAPI
InitializeModeTable(IN PHW_DEVICE_EXTENSION VgaExtension)
{
    ULONG ModeCount = 0;
    ULONG Length = 4 * 1024;
    ULONG TotalMemory;
    VP_STATUS Status;
    INT10_BIOS_ARGUMENTS BiosArguments;
    PVBE_INFO VbeInfo;
    PVBE_MODE_INFO VbeModeInfo;
    PVOID Context;
    USHORT TrampolineMemorySegment;
    USHORT TrampolineMemoryOffset;
    ULONG VbeVersion;
    ULONG NewModes = 0;
    BOOLEAN FourBppModeFound = FALSE;
    USHORT ModeResult;
    USHORT Mode;
    PUSHORT ThisMode;
    BOOLEAN LinearAddressing;
    ULONG Size, ScreenSize;
    PVIDEOMODE VgaMode;
    PVOID BaseAddress;
    ULONG ScreenStride;
    PHYSICAL_ADDRESS PhysicalAddress;

    /* Enable only default vga modes if no vesa */
    VgaModeList = ModesVGA;
    if (VideoPortIsNoVesa())
    {
        VgaExtension->Int10Interface.Size = 0;
        VgaExtension->Int10Interface.Version = 0;
        return;
    }

    /* Query INT10 interface */
    VgaExtension->Int10Interface.Version = VIDEO_PORT_INT10_INTERFACE_VERSION_1;
    VgaExtension->Int10Interface.Size = sizeof(VIDEO_PORT_INT10_INTERFACE);
    if (VideoPortQueryServices(VgaExtension,
                               VideoPortServicesInt10,
                               (PINTERFACE)&VgaExtension->Int10Interface))
    {
        VgaExtension->Int10Interface.Size = 0;
        VgaExtension->Int10Interface.Version = 0;
    }

    /* Add ref */
    VideoDebugPrint((0, "have int10 iface\n"));
    VgaExtension->Int10Interface.InterfaceReference(VgaExtension->Int10Interface.Context);
    Context = VgaExtension->Int10Interface.Context;

    /* Allocate scratch area and context */
    Status = VgaExtension->Int10Interface.Int10AllocateBuffer(Context,
                                                              &TrampolineMemorySegment,
                                                              &TrampolineMemoryOffset,
                                                              &Length);
    if (Status != NO_ERROR) return;
    VbeInfo = VideoPortAllocatePool(VgaExtension, 1, sizeof(VBE_INFO), ' agV');
    if (!VbeInfo) return;

    VbeModeInfo = &VbeInfo->Modes;

    /* Init VBE data and write to card buffer */
    VideoDebugPrint((0, "have int10 data\n"));
    VbeInfo->ModeArray[128] = 0xFFFF;
    VbeInfo->Info.Signature = VBE2_MAGIC;
    Status = VgaExtension->Int10Interface.Int10WriteMemory(Context,
                                                           TrampolineMemorySegment,
                                                           TrampolineMemoryOffset,
                                                           &VbeInfo->Info.Signature,
                                                           4);
    if (Status != NO_ERROR) return;

    /* Get controller info */
    VideoPortZeroMemory(&BiosArguments, sizeof(BiosArguments));
    BiosArguments.Edi = TrampolineMemoryOffset;
    BiosArguments.SegEs = TrampolineMemorySegment;
    BiosArguments.Eax = VBE_GET_CONTROLLER_INFORMATION;
    Status = VgaExtension->Int10Interface.Int10CallBios(Context, &BiosArguments);
    if (Status != NO_ERROR) return;
    if(VBE_GETRETURNCODE(BiosArguments.Eax) != VBE_SUCCESS)
    {
        VideoDebugPrint((0, "BiosArguments.Eax %lx\n", BiosArguments.Eax));
        return;
    }
    Status = VgaExtension->Int10Interface.Int10ReadMemory(Context,
                                                          TrampolineMemorySegment,
                                                          TrampolineMemoryOffset,
                                                          VbeInfo,
                                                          512);
    if (Status != NO_ERROR) return;

    /* Check correct VBE BIOS */
    VideoDebugPrint((0, "have vbe data\n"));
    TotalMemory = VbeInfo->Info.TotalMemory << 16;
    VbeVersion = VbeInfo->Info.Version;
    VideoDebugPrint((0, "vbe version %lx memory %lx\n", VbeVersion, TotalMemory));
    if (!ValidateVbeInfo(VgaExtension, VbeInfo)) return;

    /* Read modes */
    VideoDebugPrint((0, "read modes from %p\n", VbeInfo->Info.VideoModePtr));
    Status = VgaExtension->Int10Interface.Int10ReadMemory(Context,
                                                          HIWORD(VbeInfo->Info.VideoModePtr),
                                                          LOWORD(VbeInfo->Info.VideoModePtr),
                                                          VbeInfo->ModeArray,
                                                          128 * sizeof(USHORT));
    if (Status != NO_ERROR) return;
    VideoDebugPrint((0, "Read modes at: %p\n", VbeInfo->ModeArray));

    /* Count modes, check for new 4bpp SVGA modes */
    ThisMode = VbeInfo->ModeArray;
    ModeResult = VbeInfo->ModeArray[0];
    while (ModeResult != 0xFFFF)
    {
        Mode = ModeResult & 0x1FF;
        VideoDebugPrint((0, "Mode found: %lx\n", Mode));
        if ((Mode == 0x102) || (Mode == 0x6A)) FourBppModeFound = TRUE;
        ModeResult = *++ThisMode;
        NewModes++;
    }

    /* Remove the built-in mode if not supported by card and check max modes */
    if (!FourBppModeFound) --NumVideoModes;
    if ((NewModes >= 128) && (NumVideoModes > 8)) goto Cleanup;

    /* Switch to new SVGA mode list, copy VGA modes */
    VgaModeList = VideoPortAllocatePool(VgaExtension, 1, (NewModes + NumVideoModes) * sizeof(VIDEOMODE), ' agV');
    if (!VgaModeList) goto Cleanup;
    VideoPortMoveMemory(VgaModeList, ModesVGA, NumVideoModes * sizeof(VIDEOMODE));

    /* Apply fixup for Intel Brookdale */
    if (g_bIntelBrookdaleBIOS)
    {
        VideoDebugPrint((0, "Intel Brookdale-G Video BIOS Not Support!\n"));
        while (TRUE);
    }

    /* Scan SVGA modes */
    VideoDebugPrint((0, "Static modes: %d\n", NumVideoModes));
    VgaMode = &VgaModeList[NumVideoModes];
    ThisMode = VbeInfo->ModeArray;
    VideoDebugPrint((0, "new modes: %d\n", NewModes));
    while (NewModes--)
    {
        /* Get info on mode */
        VideoDebugPrint((0, "Getting info of mode %lx.\n", *ThisMode));
        VideoPortZeroMemory(&BiosArguments, sizeof(BiosArguments));
        BiosArguments.Eax = VBE_GET_MODE_INFORMATION;
        BiosArguments.Ecx = *ThisMode;
        BiosArguments.Edi = TrampolineMemoryOffset;
        BiosArguments.SegEs = TrampolineMemorySegment;
        Status = VgaExtension->Int10Interface.Int10CallBios(Context, &BiosArguments);
        if (Status != NO_ERROR) goto Next;
        if (VBE_GETRETURNCODE(BiosArguments.Eax) != VBE_SUCCESS) goto Next;
        Status = VgaExtension->Int10Interface.Int10ReadMemory(Context,
                                                              TrampolineMemorySegment,
                                                              TrampolineMemoryOffset,
                                                              VbeModeInfo,
                                                              256);
        if (Status != NO_ERROR) goto Next;

        /* Parse graphics modes only if linear framebuffer support */
        VideoDebugPrint((0, "attr: %lx\n", VbeModeInfo->ModeAttributes));
        if (!(VbeModeInfo->ModeAttributes & (VBE_MODEATTR_VALID |
                                             VBE_MODEATTR_GRAPHICS))) goto Next;
        LinearAddressing = ((VbeVersion >= 0x200) &&
                            (VbeModeInfo->PhysBasePtr) &&
                            (VbeModeInfo->ModeAttributes & VBE_MODEATTR_LINEAR)) ?
                            TRUE : FALSE;

        /* Check SVGA modes if 8bpp or higher */
        VideoDebugPrint((0, "PhysBase: %lx\n", VbeModeInfo->PhysBasePtr));
        if ((VbeModeInfo->XResolution >= 640) &&
            (VbeModeInfo->YResolution >= 480) &&
            (VbeModeInfo->NumberOfPlanes >= 1) &&
            (VbeModeInfo->BitsPerPixel >= 8))
        {
            /* Copy VGA mode info */
            VideoPortZeroMemory(VgaMode, sizeof(VIDEOMODE));
            VgaMode->numPlanes = VbeModeInfo->NumberOfPlanes;
            VgaMode->hres = VbeModeInfo->XResolution;
            VgaMode->vres = VbeModeInfo->YResolution;
            VgaMode->Frequency = 1;
            VgaMode->Mode = (*ThisMode << 16) | VBE_SET_VBE_MODE;
            VgaMode->Granularity = VbeModeInfo->WinGranularity << 10;
            VideoDebugPrint((0, "Mode %lx (Granularity %d)\n", VgaMode->Mode, VgaMode->Granularity));

            /* Set flags */
            if (VbeModeInfo->ModeAttributes & VBE_MODEATTR_COLOR) VgaMode->fbType |= VIDEO_MODE_COLOR;
            if (VbeModeInfo->ModeAttributes & VBE_MODEATTR_GRAPHICS) VgaMode->fbType |= VIDEO_MODE_GRAPHICS;
            if (VbeModeInfo->ModeAttributes & VBE_MODEATTR_NON_VGA) VgaMode->NonVgaMode = TRUE;

            /* If no char data, say 80x25 */
            VgaMode->col = VbeModeInfo->XCharSize ? VbeModeInfo->XResolution / VbeModeInfo->XCharSize : 80;
            VgaMode->row = VbeModeInfo->YCharSize ? VbeModeInfo->YResolution / VbeModeInfo->YCharSize : 25;
            VideoDebugPrint((0, "%d by %d rows\n", VgaMode->col, VgaMode->row));

            /* Check RGB555 (15bpp only) */
            VgaMode->bitsPerPlane = VbeModeInfo->BitsPerPixel / VbeModeInfo->NumberOfPlanes;
            if ((VgaMode->bitsPerPlane == 16) && (VbeModeInfo->GreenMaskSize == 5)) VgaMode->bitsPerPlane = 15;
            VideoDebugPrint((0, "BPP: %d\n", VgaMode->bitsPerPlane));

            /* Do linear or banked frame buffers */
            VgaMode->FrameBufferBase = 0;
            if (!LinearAddressing)
            {
                /* Read the screen stride (scanline size) */
                ScreenStride = RaiseToPower2(VbeModeInfo->BytesPerScanLine);
                //ASSERT(ScreenStride <= MAX_USHORT);
                VgaMode->wbytes = (USHORT)ScreenStride;
                VideoDebugPrint((0, "ScanLines: %lx Stride: %lx\n", VbeModeInfo->BytesPerScanLine, VgaMode->wbytes));

                /* Size of frame buffer is Height X ScanLine, align to bank/page size */
                ScreenSize = VgaMode->hres * ScreenStride;
                VideoDebugPrint((0, "Size: %lx\n", ScreenSize));
                Size = (ScreenSize + ((64 * 1024) - 1)) & ((64 * 1024) - 1);
                VideoDebugPrint((0, "Size: %lx\n", ScreenSize));
                if (Size > TotalMemory) Size = (Size + ((4 * 1024) - 1)) & ((4 * 1024) - 1);
                VideoDebugPrint((0, "Size: %lx\n", ScreenSize));

                /* Banked VGA at 0xA0000 (64K) */
                VideoDebugPrint((0, "Final size: %lx\n", Size));
                VgaMode->fbType |= VIDEO_MODE_BANKED;
                VgaMode->sbytes = Size;
                VgaMode->PhysSize = 64 * 1024;
                VgaMode->FrameBufferSize = 64 * 1024;
                VgaMode->NoBankSwitch = TRUE;
                VgaMode->PhysBase = 0xA0000;
                VgaMode->LogicalWidth = RaiseToPower2(VgaMode->hres);
            }
            else
            {
                /* VBE 3.00+ has specific field, read legacy field if not */
                VideoDebugPrint((0, "LINEAR MODE!!!\n"));
                ScreenStride = (VbeVersion >= 0x300) ? VbeModeInfo->LinBytesPerScanLine : 0;
                if (!ScreenStride) ScreenStride = VbeModeInfo->BytesPerScanLine;
                //ASSERT(ScreenStride <= MAX_USHORT);
                VgaMode->wbytes = (USHORT)ScreenStride;
                VideoDebugPrint((0, "ScanLines: %lx Stride: %lx\n", VbeModeInfo->BytesPerScanLine, VgaMode->wbytes));

                /* Size of frame buffer is Height X ScanLine, align to page size */
                ScreenSize = VgaMode->hres * LOWORD(VgaMode->wbytes);
                VideoDebugPrint((0, "Size: %lx\n", ScreenSize));
                Size = RaiseToPower2Ulong(ScreenSize);
                VideoDebugPrint((0, "Size: %lx\n", ScreenSize));
                if (Size > TotalMemory) Size = (Size + ((4 * 1024) - 1)) & ((4 * 1024) - 1);
                VideoDebugPrint((0, "Size: %lx\n", ScreenSize));

                /* Linear VGA must read settings from VBE */
                VgaMode->fbType |= VIDEO_MODE_LINEAR;
                VgaMode->sbytes = Size;
                VgaMode->PhysSize = Size;
                VgaMode->FrameBufferSize = Size;
                VgaMode->NoBankSwitch = FALSE;
                VgaMode->PhysBase = VbeModeInfo->PhysBasePtr;
                VgaMode->LogicalWidth = VgaMode->hres;

                /* Make VBE_SET_VBE_MODE command use Linear Framebuffer Select */
                VgaMode->Mode |= (VBE_MODE_LINEAR_FRAMEBUFFER << 16);
            }

            /* Override bank switch if not support by card */
            if (VbeModeInfo->ModeAttributes & VBE_MODEATTR_NO_BANK_SWITCH) VgaMode->NoBankSwitch = TRUE;

            /* Next */
            if (ScreenSize <= TotalMemory)
            {
                VgaMode++;
                ModeCount++;
            }
        }
Next:
        /* Next */
        ThisMode++;
    }

    /* Check if last mode was color to do test */
    VideoDebugPrint((0, "mode scan complete. Total modes: %d\n", ModeCount));
    if (--VgaMode->fbType & VIDEO_MODE_COLOR)
    {
        /* Try map physical buffer and free if worked */
        PhysicalAddress.QuadPart = VgaMode->PhysBase;
        BaseAddress = VideoPortGetDeviceBase(VgaExtension, PhysicalAddress, 4 * 1024, FALSE);
        if (BaseAddress)
        {
            VideoPortFreeDeviceBase(VgaExtension, BaseAddress);
        }
        else
        {
            /* Not work, so throw out VBE data */
            ModeCount = 0;
        }
    }

    /* Cleanup sucess path */
    VideoPortFreePool(VgaExtension, VbeInfo);
    VgaExtension->Int10Interface.Int10FreeBuffer(Context,
                                                 TrampolineMemorySegment,
                                                 TrampolineMemoryOffset);
    NumVideoModes += ModeCount;
    return;

Cleanup:
    /* Cleanup failure path, reset standard VGA and free memory */
    VgaModeList = ModesVGA;
    VideoPortFreePool(VgaExtension, VbeInfo);
    VgaExtension->Int10Interface.Int10FreeBuffer(Context,
                                                 TrampolineMemorySegment,
                                                 TrampolineMemoryOffset);
}

/* EOF */
