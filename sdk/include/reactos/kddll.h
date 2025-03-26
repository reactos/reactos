#ifndef _KDDLL_
#define _KDDLL_

typedef ULONG KDSTATUS;
#define KdPacketReceived     0
#define KdPacketTimedOut     1
#define KdPacketNeedsResend  2

NTSTATUS
NTAPI
KdDebuggerInitialize0(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock
);

NTSTATUS
NTAPI
KdDebuggerInitialize1(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock
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
