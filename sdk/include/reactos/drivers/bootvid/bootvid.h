/*
 * PROJECT:     ReactOS Boot Video Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main BOOTVID header.
 * COPYRIGHT:   Copyright 2007-2020 Alex Ionescu (alex.ionescu@reactos.org)
 */

#ifndef _BOOTVID_
#define _BOOTVID_

#pragma once

#include "display.h"

BOOLEAN
NTAPI
VidInitialize(
    _In_ BOOLEAN SetMode);

VOID
NTAPI
VidResetDisplay(
    _In_ BOOLEAN HalReset);

ULONG
NTAPI
VidSetTextColor(
    _In_ ULONG Color);

VOID
NTAPI
VidDisplayStringXY(
    _In_ PUCHAR String,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ BOOLEAN Transparent);

VOID
NTAPI
VidSetScrollRegion(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom);

VOID
NTAPI
VidCleanUp(VOID);

VOID
NTAPI
VidBufferToScreenBlt(
    _In_ PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta);

VOID
NTAPI
VidDisplayString(
    _In_ PUCHAR String);

VOID
NTAPI
VidBitBlt(
    _In_ PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top);

VOID
NTAPI
VidScreenToBufferBlt(
    _Out_ PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta);

VOID
NTAPI
VidSolidColorFill(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ UCHAR Color);

#endif // _BOOTVID_
