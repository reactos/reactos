#pragma once

INIT_FUNCTION
VOID
NTAPI
InbvUpdateProgressBar(
    IN ULONG Progress
);

INIT_FUNCTION
VOID
NTAPI
InbvRotBarInit(
    VOID
);

INIT_FUNCTION
BOOLEAN
NTAPI
InbvDriverInitialize(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Count
);

INIT_FUNCTION
VOID
NTAPI
InbvEnableBootDriver(
    IN BOOLEAN Enable
);

INIT_FUNCTION
VOID
NTAPI
DisplayBootBitmap(
    IN BOOLEAN TextMode
);

INIT_FUNCTION
VOID
NTAPI
DisplayFilter(
    IN PCHAR *String
);

INIT_FUNCTION
VOID
NTAPI
FinalizeBootLogo(
    VOID
);

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

INIT_FUNCTION
VOID
NTAPI
InbvIndicateProgress(
    VOID
);

VOID
NTAPI
InbvPortEnableFifo(
    IN ULONG PortId,
    IN BOOLEAN Enable
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

BOOLEAN
NTAPI
InbvPortPollOnly(
    IN ULONG PortId
);

extern BOOLEAN InbvBootDriverInstalled;
