/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video support for linear framebuffers
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

BOOLEAN
VidFbInitializeVideo(
    _In_ ULONG_PTR BaseAddress,
    _In_ ULONG BufferSize,
    _In_ UINT32 ScreenWidth,
    _In_ UINT32 ScreenHeight,
    _In_ UINT32 PixelsPerScanLine,
    _In_ UINT32 BitsPerPixel);

VOID
VidFbClearScreenColor(
    _In_ UINT32 Color,
    _In_ BOOLEAN FullScreen);

VOID
VidFbClearScreen(
    _In_ UCHAR Attr);

VOID
VidFbOutputChar(
    _In_ UCHAR Char,
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ UINT32 FgColor,
    _In_ UINT32 BgColor);

VOID
VidFbPutChar(
    _In_ UCHAR Char,
    _In_ UCHAR Attr,
    _In_ ULONG X,
    _In_ ULONG Y);

VOID
VidFbGetDisplaySize(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ PULONG Depth);

ULONG
VidFbGetBufferSize(VOID);

VOID
VidFbCopyOffScreenBufferToVRAM(
    _In_ PVOID Buffer);

VOID
VidFbScrollUp(
    _In_ UCHAR Attr);

#if 0
VOID
VidFbSetTextCursorPosition(UCHAR X, UCHAR Y);

VOID
VidFbHideShowTextCursor(BOOLEAN Show);

BOOLEAN
VidFbIsPaletteFixed(VOID);

VOID
VidFbSetPaletteColor(
    _In_ UCHAR Color,
    _In_ UCHAR Red, _In_ UCHAR Green, _In_ UCHAR Blue);

VOID
VidFbGetPaletteColor(
    _In_ UCHAR Color,
    _Out_ PUCHAR Red, _Out_ PUCHAR Green, _Out_ PUCHAR Blue);
#endif
