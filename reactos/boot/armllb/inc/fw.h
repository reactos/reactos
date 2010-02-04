/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/inc/fw.h
 * PURPOSE:         LLB Firmware Functions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

VOID
LlbFwPutChar(
    INT Ch
);

BOOLEAN
LlbFwKbHit(
    VOID
);

INT
LlbFwGetCh(
    VOID
);

ULONG
LlbFwVideoSetDisplayMode(
    IN PCHAR DisplayModeName,
    IN BOOLEAN Init
);

VOID
LlbFwVideoGetDisplaySize(
    OUT PULONG Width,
    OUT PULONG Height,
    OUT PULONG Depth
);

ULONG
LlbFwVideoGetBufferSize(
    VOID
);

VOID
LlbFwVideoSetTextCursorPosition(
    IN ULONG X,
    IN ULONG Y
);

VOID
LlbFwVideoHideShowTextCursor(
    IN BOOLEAN Show
);

VOID
LlbFwVideoCopyOffScreenBufferToVRAM(
    IN PVOID Buffer
);

VOID
LlbFwVideoClearScreen(
    IN UCHAR Attr
);
    
VOID
LlbFwVideoPutChar(
    IN INT c,
    IN UCHAR Attr,
    IN ULONG X,
    IN ULONG Y
);

BOOLEAN
LlbFwVideoIsPaletteFixed(
    VOID
);

VOID
LlbFwVideoSetPaletteColor(
    IN UCHAR Color,
    IN UCHAR Red,
    IN UCHAR Green,
    IN UCHAR Blue
);

VOID
LlbFwVideoGetPaletteColor(
    IN UCHAR Color,
    OUT PUCHAR Red,
    OUT PUCHAR Green,
    OUT PUCHAR Blue
);

VOID
LlbFwVideoSync(
    VOID
);

VOID
LlbFwGetTime(
    VOID
);

/* EOF */
