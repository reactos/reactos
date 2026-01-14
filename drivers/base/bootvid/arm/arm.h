/*
 * PROJECT:     ReactOS Boot Video Driver for ARM devices
 * LICENSE:     BSD - See COPYING.ARM in root directory
 * PURPOSE:     PrimeCell Color LCD Controller (PL110) definitions
 * COPYRIGHT:   Copyright 2008 ReactOS Portable Systems Group <ros.arm@reactos.org>
 */

#pragma once

#define READ_REGISTER_ULONG(r) (*(volatile ULONG * const)(r))
#define WRITE_REGISTER_ULONG(r, v) (*(volatile ULONG *)(r) = (v))

#define READ_REGISTER_USHORT(r) (*(volatile USHORT * const)(r))
#define WRITE_REGISTER_USHORT(r, v) (*(volatile USHORT *)(r) = (v))

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
