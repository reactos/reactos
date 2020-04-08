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
VidInitialize(IN BOOLEAN SetMode);

VOID
NTAPI
VidResetDisplay(IN BOOLEAN HalReset);

ULONG
NTAPI
VidSetTextColor(IN ULONG Color);

VOID
NTAPI
VidDisplayStringXY(IN PUCHAR String,
                   IN ULONG Left,
                   IN ULONG Top,
                   IN BOOLEAN Transparent);

VOID
NTAPI
VidSetScrollRegion(IN ULONG Left,
                   IN ULONG Top,
                   IN ULONG Right,
                   IN ULONG Bottom);

VOID
NTAPI
VidCleanUp(VOID);

VOID
NTAPI
VidBufferToScreenBlt(IN PUCHAR Buffer,
                     IN ULONG Left,
                     IN ULONG Top,
                     IN ULONG Width,
                     IN ULONG Height,
                     IN ULONG Delta);

VOID
NTAPI
VidDisplayString(IN PUCHAR String);

VOID
NTAPI
VidBitBlt(IN PUCHAR Buffer,
          IN ULONG Left,
          IN ULONG Top);

VOID
NTAPI
VidScreenToBufferBlt(OUT PUCHAR Buffer,
                     IN ULONG Left,
                     IN ULONG Top,
                     IN ULONG Width,
                     IN ULONG Height,
                     IN ULONG Delta);

VOID
NTAPI
VidSolidColorFill(IN ULONG Left,
                  IN ULONG Top,
                  IN ULONG Right,
                  IN ULONG Bottom,
                  IN UCHAR Color);

#endif // _BOOTVID_
