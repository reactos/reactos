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
VidFbOutputChar(
    _In_ UCHAR Char,
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ UINT32 FgColor,
    _In_ UINT32 BgColor);

VOID
VidFbGetDisplaySize(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ PULONG Depth);

ULONG
VidFbGetBufferSize(VOID);

VOID
VidFbScrollUp(
    _In_ UINT32 Color,
    _In_ ULONG Scroll);

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



/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Linear framebuffer based console support
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

VOID
FbConsClearScreen(
    _In_ UCHAR Attr);

VOID
FbConsOutputChar(
    _In_ UCHAR Char,
    _In_ ULONG Column,
    _In_ ULONG Row,
    _In_ UINT32 FgColor,
    _In_ UINT32 BgColor);

VOID
FbConsPutChar(
    _In_ UCHAR Char,
    _In_ UCHAR Attr,
    _In_ ULONG Column,
    _In_ ULONG Row);

VOID
FbConsGetDisplaySize(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ PULONG Depth);

ULONG
FbConsGetBufferSize(VOID);

VOID
FbConsCopyOffScreenBufferToVRAM(
    _In_ PVOID Buffer);

VOID
FbConsScrollUp(
    _In_ UCHAR Attr);
