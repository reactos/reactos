/*
 * PROJECT:     ReactOS Boot Video Driver for Original Xbox
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Arch-specific header file
 * COPYRIGHT:   Copyright 2004 Gé van Geldorp <gvg@reactos.org>
 *              Copyright 2005 Filip Navara <navaraf@reactos.org>
 *              Copyright 2020 Stanislav Motylkov <x86corez@gmail.com>
 */

#pragma once

VOID
InitPaletteWithTable(
    _In_reads_(Count) const ULONG* Table,
    _In_ ULONG Count);

#define PrepareForSetPixel()

VOID
SetPixel(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ UCHAR Color);
