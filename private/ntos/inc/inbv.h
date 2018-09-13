/*++

Copyright(c) 1998  Microsoft Corporation

Module Name:

    inbv.h

Abstract:

    This module contains the public header information (function prototypes,
    data and type declarations) for the Initialization Boot Video component.

Author:

    Erick Smith (ericks) 23-Mar-1998

Revision History:

--*/

#ifndef _INBV_
#define _INBV_

typedef
BOOLEAN
(*INBV_RESET_DISPLAY_PARAMETERS)(
    ULONG Cols,
    ULONG Rows
    );

typedef
VOID
(*INBV_DISPLAY_STRING_FILTER)(
    PUCHAR *Str
    );

VOID
InbvNotifyDisplayOwnershipLost(
    INBV_RESET_DISPLAY_PARAMETERS ResetDisplayParameters
    );

VOID
InbvInstallDisplayStringFilter(
    INBV_DISPLAY_STRING_FILTER DisplayStringFilter
    );

VOID
InbvAcquireDisplayOwnership(
    VOID
    );

BOOLEAN
InbvDriverInitialize(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Count
    );

BOOLEAN
InbvResetDisplay(
    );

VOID
InbvBitBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y
    );

VOID
InbvSolidColorFill(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2,
    ULONG color
    );

BOOLEAN
InbvDisplayString(
    PUCHAR Str
    );

VOID
InbvUpdateProgressBar(
    ULONG Percentage
    );

VOID
InbvSetProgressBarSubset(
    ULONG   Floor,
    ULONG   Ceiling
    );

VOID
InbvSetBootDriverBehavior(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
InbvIndicateProgress(
    VOID
    );

VOID
InbvSaveProgressIndicatorCount(
    VOID
    );

VOID
InbvSetProgressBarCoordinates(
    ULONG x,
    ULONG y
    );

VOID
InbvEnableBootDriver(
    BOOLEAN bEnable
    );

BOOLEAN
InbvEnableDisplayString(
    BOOLEAN bEnable
    );

BOOLEAN
InbvIsBootDriverInstalled(
    VOID
    );

PUCHAR
InbvGetResourceAddress(
    IN ULONG ResourceNumber
    );

VOID
InbvBufferToScreenBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    );

VOID
InbvScreenToBufferBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    );

BOOLEAN
InbvTestLock(
    VOID
    );

VOID
InbvAcquireLock(
    VOID
    );

VOID
InbvReleaseLock(
    VOID
    );

BOOLEAN
InbvCheckDisplayOwnership(
    VOID
    );

VOID
InbvSetScrollRegion(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2
    );

ULONG
InbvSetTextColor(
    ULONG Color
    );

VOID
InbvSetDisplayOwnership(
    BOOLEAN DisplayOwned
    );

#endif
