#pragma once

/* KD ROUTINES ***************************************************************/

struct _KD_DISPATCH_TABLE;

typedef
NTSTATUS
(NTAPI *PKDP_INIT_ROUTINE)(
    _In_ struct _KD_DISPATCH_TABLE *DispatchTable,
    _In_ ULONG BootPhase);

typedef
VOID
(NTAPI *PKDP_PRINT_ROUTINE)(
    _In_ PCCH String,
    _In_ ULONG Length);

VOID
KdIoPuts(
    _In_ PCSTR String);

VOID
__cdecl
KdIoPrintf(
    _In_ PCSTR Format,
    ...);

SIZE_T
KdIoReadLine(
    _Out_ PCHAR Buffer,
    _In_ SIZE_T Size);


/* INIT ROUTINES *************************************************************/

KIRQL
NTAPI
KdbpAcquireLock(
    _In_ PKSPIN_LOCK SpinLock);

VOID
NTAPI
KdbpReleaseLock(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ KIRQL OldIrql);

VOID
KdpScreenAcquire(VOID);

VOID
KdpScreenRelease(VOID);

NTSTATUS
NTAPI
KdpScreenInit(
    _In_ struct _KD_DISPATCH_TABLE *DispatchTable,
    _In_ ULONG BootPhase);

NTSTATUS
NTAPI
KdpSerialInit(
    _In_ struct _KD_DISPATCH_TABLE *DispatchTable,
    _In_ ULONG BootPhase);

NTSTATUS
NTAPI
KdpDebugLogInit(
    _In_ struct _KD_DISPATCH_TABLE *DispatchTable,
    _In_ ULONG BootPhase);

#ifdef KDBG
#define KdpKdbgInit KdbInitialize
#endif


/* KD GLOBALS ****************************************************************/

/* Serial debug connection */
#define DEFAULT_DEBUG_PORT      2 /* COM2 */
#define DEFAULT_DEBUG_COM1_IRQ  4 /* COM1 IRQ */
#define DEFAULT_DEBUG_COM2_IRQ  3 /* COM2 IRQ */
#define DEFAULT_DEBUG_BAUD_RATE 115200 /* 115200 Baud */

/* KD Native Modes */
#define KdScreen    0
#define KdSerial    1
#define KdFile      2
#define KdKdbg      3
#define KdMax       4

/* KD Private Debug Modes */
typedef struct _KDP_DEBUG_MODE
{
    union
    {
        struct
        {
            /* Native Modes */
            UCHAR Screen :1;
            UCHAR Serial :1;
            UCHAR File   :1;
        };

        /* Generic Value */
        ULONG Value;
    };
} KDP_DEBUG_MODE;

/* Dispatch Table for Wrapper Functions */
typedef struct _KD_DISPATCH_TABLE
{
    LIST_ENTRY KdProvidersList;
    PKDP_INIT_ROUTINE KdpInitRoutine;
    PKDP_PRINT_ROUTINE KdpPrintRoutine;
    NTSTATUS InitStatus;
} KD_DISPATCH_TABLE, *PKD_DISPATCH_TABLE;

/* The current Debugging Mode */
extern KDP_DEBUG_MODE KdpDebugMode;

/* Port Information for the Serial Native Mode */
extern ULONG  SerialPortNumber;
extern CPPORT SerialPortInfo;

/* Logging file path */
extern ANSI_STRING KdpLogFileName;

/* Init Functions for Native Providers */
extern PKDP_INIT_ROUTINE InitRoutines[KdMax];

/* Dispatch Tables for Native Providers */
extern KD_DISPATCH_TABLE DispatchTable[KdMax];

/* The KD Native Provider List */
extern LIST_ENTRY KdProviders;
