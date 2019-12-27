/*
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#ifndef __MEMORY_H
#include "mm.h"
#endif

#define MAX_XBOX_COM_PORTS    2

#define LPC_IO_BASE             0x2E
#define LPC_ENTER_CONFIG_KEY    0x55
#define LPC_EXIT_CONFIG_KEY     0xAA

#define LPC_DEVICE_SERIAL_PORT_1    0x4
#define LPC_DEVICE_SERIAL_PORT_2    0x5

#define LPC_CONFIG_DEVICE_NUMBER                0x07
#define LPC_CONFIG_DEVICE_ACTIVATE              0x30
#define LPC_CONFIG_DEVICE_BASE_ADDRESS_HIGH     0x60
#define LPC_CONFIG_DEVICE_BASE_ADDRESS_LOW      0x61
#define LPC_CONFIG_DEVICE_INTERRUPT             0x70

#define NV2A_CONTROL_OFFSET            0xFD000000
#define   NV2A_CRTC_OFFSET              (0x600000 + NV2A_CONTROL_OFFSET)
#define     NV2A_CRTC_FRAMEBUFFER_START    (0x800 + NV2A_CRTC_OFFSET)
#define     NV2A_CRTC_REGISTER_INDEX      (0x13D4 + NV2A_CRTC_OFFSET)
#define     NV2A_CRTC_REGISTER_VALUE      (0x13D5 + NV2A_CRTC_OFFSET)
#define   NV2A_RAMDAC_OFFSET            (0x680000 + NV2A_CONTROL_OFFSET)
#define     NV2A_RAMDAC_FP_HVALID_END      (0x838 + NV2A_RAMDAC_OFFSET)
#define     NV2A_RAMDAC_FP_VVALID_END      (0x818 + NV2A_RAMDAC_OFFSET)

extern UCHAR XboxFont8x16[256 * 16];

VOID XboxConsPutChar(int Ch);
BOOLEAN XboxConsKbHit(VOID);
int XboxConsGetCh(VOID);

VOID XboxVideoInit(VOID);
VOID XboxVideoClearScreen(UCHAR Attr);
VIDEODISPLAYMODE XboxVideoSetDisplayMode(char *DisplayModem, BOOLEAN Init);
VOID XboxVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);
ULONG XboxVideoGetBufferSize(VOID);
VOID XboxVideoGetFontsFromFirmware(PULONG RomFontPointers);
VOID XboxVideoSetTextCursorPosition(UCHAR X, UCHAR Y);
VOID XboxVideoHideShowTextCursor(BOOLEAN Show);
VOID XboxVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y);
VOID XboxVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOLEAN XboxVideoIsPaletteFixed(VOID);
VOID XboxVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID XboxVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
VOID XboxVideoSync(VOID);
VOID XboxVideoPrepareForReactOS(VOID);
VOID XboxPrepareForReactOS(VOID);

VOID XboxMemInit(VOID);
PFREELDR_MEMORY_DESCRIPTOR XboxMemGetMemoryMap(ULONG *MemoryMapSize);

VOID XboxDiskInit(BOOLEAN Init);
BOOLEAN XboxDiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOLEAN XboxDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY DriveGeometry);
ULONG XboxDiskGetCacheableBlockCount(UCHAR DriveNumber);

TIMEINFO* XboxGetTime(VOID);

PCONFIGURATION_COMPONENT_DATA XboxHwDetect(VOID);
VOID XboxHwIdle(VOID);

VOID XboxSetLED(PCSTR Pattern);

/* EOF */
