/*
 * PROJECT:     ReactOS Boot Video Driver for Original Xbox
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Arch-specific header file
 * COPYRIGHT:   Copyright 2004 Gé van Geldorp (gvg@reactos.org)
 *              Copyright 2005 Filip Navara (navaraf@reactos.org)
 *              Copyright 2020 Stanislav Motylkov (x86corez@gmail.com)
 */

#ifndef _BOOTVID_XBOX_H_
#define _BOOTVID_XBOX_H_

#pragma once

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

#endif /* _BOOTVID_XBOX_H_ */
