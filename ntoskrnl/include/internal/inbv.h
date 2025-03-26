/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Boot Video Driver support header
 * COPYRIGHT:   Copyright 2007 Alex Ionescu (alex.ionescu@reactos.org)
 *              Copyright 2019-2022 Hermès Bélusca-Maïto
 */

#pragma once

/* Native definitions from BOOTVID (Boot Video Driver) */
#include "bootvid/bootvid.h"

//
// Driver Initialization
//
CODE_SEG("INIT")
BOOLEAN
NTAPI
InbvDriverInitialize(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ ULONG Count
);

extern BOOLEAN InbvBootDriverInstalled;

INBV_DISPLAY_STATE
NTAPI
InbvGetDisplayState(VOID);

VOID
NTAPI
InbvAcquireLock(VOID);

VOID
NTAPI
InbvReleaseLock(VOID);

PUCHAR
NTAPI
InbvGetResourceAddress(
    _In_ ULONG ResourceNumber
);

//
// Display Functions
//
VOID
NTAPI
InbvBitBlt(
    _In_ PUCHAR Buffer,
    _In_ ULONG X,
    _In_ ULONG Y
);

VOID
NTAPI
InbvBufferToScreenBlt(
    _In_ PUCHAR Buffer,
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta
);

VOID
NTAPI
InbvScreenToBufferBlt(
    _Out_ PUCHAR Buffer,
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta
);

//
// Progress-Bar Functions
//
VOID
NTAPI
InbvSetProgressBarCoordinates(
    _In_ ULONG Left,
    _In_ ULONG Top
);

CODE_SEG("INIT")
VOID
NTAPI
InbvIndicateProgress(VOID);

VOID
NTAPI
InbvSetProgressBarSubset(
    _In_ ULONG Floor,
    _In_ ULONG Ceiling
);

VOID
NTAPI
InbvUpdateProgressBar(
    _In_ ULONG Percentage
);

//
// Headless Terminal Support Functions
//
VOID
NTAPI
InbvPortEnableFifo(
    IN ULONG PortId,
    IN BOOLEAN Enable
);

BOOLEAN
NTAPI
InbvPortPollOnly(
    IN ULONG PortId
);

BOOLEAN
NTAPI
InbvPortGetByte(
    IN ULONG PortId,
    OUT PUCHAR Byte
);

VOID
NTAPI
InbvPortPutByte(
    IN ULONG PortId,
    IN UCHAR Byte
);

VOID
NTAPI
InbvPortTerminate(
    IN ULONG PortId
);

BOOLEAN
NTAPI
InbvPortInitialize(
    IN ULONG BaudRate,
    IN ULONG PortNumber,
    IN PUCHAR PortAddress,
    OUT PULONG PortId,
    IN BOOLEAN IsMMIODevice
);
