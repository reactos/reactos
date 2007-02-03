#ifndef NTOSKRNL_INBV_H
#define NTOSKRNL_INBV_H

typedef struct _InbvProgressState
{
    ULONG Floor;
    ULONG Ceiling;
    ULONG Bias;
} INBV_PROGRESS_STATE;

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

#endif /* NTOSKRNL_INBV_H */


