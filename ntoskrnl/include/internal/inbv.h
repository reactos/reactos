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

//
// InbvBitBltAligned() alignments
//
typedef enum _BBLT_VERT_ALIGNMENT
{
    AL_VERTICAL_TOP = 0,
    AL_VERTICAL_CENTER,
    AL_VERTICAL_BOTTOM
} BBLT_VERT_ALIGNMENT;

typedef enum _BBLT_HORZ_ALIGNMENT
{
    AL_HORIZONTAL_LEFT = 0,
    AL_HORIZONTAL_CENTER,
    AL_HORIZONTAL_RIGHT
} BBLT_HORZ_ALIGNMENT;

//
// Bitmap Display Functions
//
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

VOID
NTAPI
InbvBitBltAligned(
    IN PVOID Image,
    IN BOOLEAN NoPalette,
    IN BBLT_HORZ_ALIGNMENT HorizontalAlignment,
    IN BBLT_VERT_ALIGNMENT VerticalAlignment,
    IN ULONG MarginLeft,
    IN ULONG MarginTop,
    IN ULONG MarginRight,
    IN ULONG MarginBottom
);

VOID
NTAPI
InbvBitBltPalette(
    IN PVOID Image,
    IN BOOLEAN NoPalette,
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
