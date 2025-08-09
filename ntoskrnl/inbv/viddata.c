/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later
 * PURPOSE:     Shared video framebuffer data between inbv and bootvid
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <ntifs.h>

/* Global framebuffer data exported for bootvid driver */
PHYSICAL_ADDRESS VidpFrameBufferBase = {0};
ULONG VidpFrameBufferSize = 0;
ULONG VidpScreenWidth = 0;
ULONG VidpScreenHeight = 0;
ULONG VidpPixelsPerScanLine = 0;