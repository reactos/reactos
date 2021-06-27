#pragma once

// Native definitions from BOOTVID (Boot Video Driver).
#include "bootvid/bootvid.h"

//
// Driver Initialization
//
CODE_SEG("INIT")
BOOLEAN
NTAPI
InbvDriverInitialize(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Count
);

extern BOOLEAN InbvBootDriverInstalled;

PUCHAR
NTAPI
InbvGetResourceAddress(
    IN ULONG ResourceNumber
);

VOID
NTAPI
InbvBitBlt(
    IN PUCHAR Buffer,
    IN ULONG X,
    IN ULONG Y
);

//
// Progress-Bar Functions
//
CODE_SEG("INIT")
VOID
NTAPI
InbvIndicateProgress(
    VOID
);

CODE_SEG("INIT")
VOID
NTAPI
InbvSetProgressBarSubset(
    _In_ ULONG Floor,
    _In_ ULONG Ceiling
);

CODE_SEG("INIT")
VOID
NTAPI
InbvUpdateProgressBar(
    IN ULONG Progress
);

//
// Boot Splash-Screen Functions
//
CODE_SEG("INIT")
VOID
NTAPI
InbvRotBarInit(
    VOID
);

CODE_SEG("INIT")
VOID
NTAPI
DisplayBootBitmap(
    IN BOOLEAN TextMode
);

CODE_SEG("INIT")
VOID
NTAPI
DisplayFilter(
    IN PCHAR *String
);

CODE_SEG("INIT")
VOID
NTAPI
FinalizeBootLogo(
    VOID
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
