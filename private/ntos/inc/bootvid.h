/*++

Copyright(c) 1998  Microsoft Corporation

Module Name:

    bootvid.h

Abstract:

    This module contains the public header information (function prototypes,
    data and type declarations) for the boot video driver.

Author:

    Erick Smith (ericks) 23-Mar-1998

Revision History:

--*/

VOID
VidSolidColorFill(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2,
    ULONG color
    );

VOID
VidDisplayString(
    PUCHAR str
    );

VOID
VidResetDisplay(
    VOID
    );

BOOLEAN
VidInitialize(
    BOOLEAN SetMode
    );

VOID
VidBitBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y
    );

VOID
VidBufferToScreenBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    );

VOID
VidScreenToBufferBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    );

VOID
VidSetScrollRegion(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2
    );

VOID
VidCleanUp(
    VOID
    );

VOID
VidDisplayStringXY(
    PUCHAR s,
    ULONG x,
    ULONG y,
    BOOLEAN Transparent
    );

ULONG
VidSetTextColor(
    ULONG Color
    );
