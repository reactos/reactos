#ifndef _KDDLL_
#define _KDDLL_

typedef enum _KDSTATUS
{
    KdPacketReceived = 0,
    KdPacketTimedOut,
    KdPacketNeedsResend
} KDSTATUS;

NTSTATUS
NTAPI
KdDebuggerInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

NTSTATUS
NTAPI
KdDebuggerInitialize1(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

KDSTATUS
NTAPI
KdReceivePacket(
    IN ULONG PacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength,
    IN OUT PKD_CONTEXT Context
);

NTSTATUS
NTAPI
KdRestore(
    IN BOOLEAN SleepTransition
);

NTSTATUS
NTAPI
KdSave(
    IN BOOLEAN SleepTransition
);

VOID
NTAPI
KdSendPacket(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData,
    IN OUT PKD_CONTEXT Context
);

NTSTATUS
NTAPI
KdD0Transition(
    VOID
);

NTSTATUS
NTAPI
KdD3Transition(
    VOID
);

#endif
