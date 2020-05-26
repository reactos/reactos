/*
 * PROJECT:     ReactOS Boot Video Driver for Original Xbox
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Arch-specific header file
 * COPYRIGHT:   Copyright 2004 Gé van Geldorp (gvg@reactos.org)
 *              Copyright 2005 Filip Navara (navaraf@reactos.org)
 *              Copyright 2020 Stanislav Motylkov (x86corez@gmail.com)
 */

#ifndef _BOOTVID_NV2A_H_
#define _BOOTVID_NV2A_H_

#pragma once

/* FIXME: obtain fb size from firmware somehow (Cromwell reserves high 4 MB of RAM) */
#define NV2A_VIDEO_MEMORY_SIZE    (4 * 1024 * 1024)

#define NV2A_CONTROL_FRAMEBUFFER_ADDRESS_OFFSET 0x600800
#define NV2A_CRTC_REGISTER_INDEX                0x6013D4
#define NV2A_CRTC_REGISTER_VALUE                0x6013D5
#define NV2A_RAMDAC_FP_HVALID_END               0x680838
#define NV2A_RAMDAC_FP_VVALID_END               0x680818

#define BB_OFFSET(x, y)    ((y) * SCREEN_WIDTH + (x))
#define FB_OFFSET(x, y)    (((PanV + (y)) * FrameBufferWidth + PanH + (x)) * BytesPerPixel)

VOID
NTAPI
InitPaletteWithTable(
    _In_ PULONG Table,
    _In_ ULONG Count);

VOID
PrepareForSetPixel(VOID);

VOID
SetPixel(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ UCHAR Color);

VOID
NTAPI
PreserveRow(
    _In_ ULONG CurrentTop,
    _In_ ULONG TopDelta,
    _In_ BOOLEAN Restore);

VOID
NTAPI
DoScroll(
    _In_ ULONG Scroll);

VOID
NTAPI
DisplayCharacter(
    _In_ CHAR Character,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG TextColor,
    _In_ ULONG BackColor);

#endif /* _BOOTVID_NV2A_H_ */
