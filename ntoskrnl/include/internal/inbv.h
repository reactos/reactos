#ifndef NTOSKRNL_INBV_H
#define NTOSKRNL_INBV_H

typedef struct _InbvProgressState
{
    ULONG Floor;
    ULONG Ceiling;
    ULONG Bias;
} INBV_PROGRESS_STATE;

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

extern BOOLEAN InbvBootDriverInstalled;

#endif /* NTOSKRNL_INBV_H */


