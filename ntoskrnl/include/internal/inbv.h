#pragma once

typedef struct _InbvProgressState
{
    ULONG Floor;
    ULONG Ceiling;
    ULONG Bias;
} INBV_PROGRESS_STATE;

typedef struct _BT_PROGRESS_INDICATOR
{
    ULONG Count;
    ULONG Expected;
    ULONG Percentage;
} BT_PROGRESS_INDICATOR, *PBT_PROGRESS_INDICATOR;

typedef enum _ROT_BAR_TYPE
{
    RB_UNSPECIFIED,
    RB_SQUARE_CELLS
} ROT_BAR_TYPE;

VOID
NTAPI
InbvUpdateProgressBar(
    IN ULONG Progress
);

BOOLEAN
NTAPI
InbvDriverInitialize(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Count
);

VOID
NTAPI
InbvEnableBootDriver(
    IN BOOLEAN Enable
);

VOID
NTAPI
DisplayBootBitmap(
    IN BOOLEAN SosMode
);

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

VOID
NTAPI
InbvIndicateProgress(
    VOID
);
           
extern BOOLEAN InbvBootDriverInstalled;
