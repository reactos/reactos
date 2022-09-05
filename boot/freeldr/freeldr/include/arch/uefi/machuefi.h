/*
 * PROJECT:     Freeldr UEFI Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     UEFI "mach" header
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <machine.h>

EFI_STATUS
UefiMachInit(_In_ EFI_HANDLE ImageHandle,
             _In_ EFI_SYSTEM_TABLE *SystemTable);

VOID
UefiConsPutChar(int Ch);

BOOLEAN
UefiConsKbHit(VOID);

VOID
UefiConsSetCursor(UINT32 Col, UINT32 Row);

int
UefiConsGetCh(void);

EFI_STATUS
UefiInitalizeVideo(VOID);

VOID
UefiVideoClearScreen(UCHAR Attr);

VIDEODISPLAYMODE
UefiVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init);

VOID
UefiVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);

ULONG
UefiVideoGetBufferSize(VOID);

VOID
UefiVideoGetFontsFromFirmware(PULONG RomFontPointers);

VOID
UefiVideoSetTextCursorPosition(UCHAR X, UCHAR Y);

VOID
UefiVideoHideShowTextCursor(BOOLEAN Show);

VOID
UefiVideoOutputChar(UCHAR Char, unsigned X,
                    unsigned Y, ULONG FgColor, ULONG BgColor);

VOID
UefiVideoPutChar(int Ch, UCHAR Attr,
                 unsigned X, unsigned Y);


VOID
UefiVideoCopyOffScreenBufferToVRAM(PVOID Buffer);

BOOLEAN
UefiVideoIsPaletteFixed(VOID);

VOID
UefiVideoSetPaletteColor(UCHAR Color, UCHAR Red,
                         UCHAR Green, UCHAR Blue);

VOID
UefiVideoGetPaletteColor(UCHAR Color, UCHAR* Red,
                         UCHAR* Green, UCHAR* Blue);

VOID
UefiVideoSync(VOID);

VOID
UefiPcBeep(VOID);

PFREELDR_MEMORY_DESCRIPTOR
UefiMemGetMemoryMap(ULONG *MemoryMapSize);

VOID
UefiGetExtendedBIOSData(PULONG ExtendedBIOSDataArea,
                        PULONG ExtendedBIOSDataSize);

UCHAR
UefiGetFloppyCount(VOID);

BOOLEAN
UefiDiskReadLogicalSectors(IN UCHAR DriveNumber,
                           IN ULONGLONG SectorNumber,
                           IN ULONG SectorCount,
                           OUT PVOID Buffer);

BOOLEAN
UefiDiskGetDriveGeometry(UCHAR DriveNumber,
                         PGEOMETRY Geometry);

ULONG
UefiDiskGetCacheableBlockCount(UCHAR DriveNumber);

TIMEINFO*
UefiGetTime(VOID);

BOOLEAN
UefiInitializeBootDevices(VOID);

PCONFIGURATION_COMPONENT_DATA
UefiHwDetect(VOID);

VOID
UefiPrepareForReactOS(VOID);

VOID
UefiHwIdle(VOID);

VOID
UefiInitializeFileSystemSupport(_In_ EFI_HANDLE ImageHandle,
                                _In_ EFI_SYSTEM_TABLE *SystemTable);

VOID
UefiVideoScrollUp(VOID);
