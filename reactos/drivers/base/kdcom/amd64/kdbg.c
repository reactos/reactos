/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kdcom/kdbg.c
 * PURPOSE:         Serial i/o functions for the kernel debugger.
 * PROGRAMMER:      Alex Ionescu
 *                  Hervé Poussineau
 */

/* INCLUDES *****************************************************************/

#define NOEXTAPI
#include <ntddk.h>
#define NDEBUG
#include <halfuncs.h>
#include <stdio.h>
#include <debug.h>
#include "arc/arc.h"
#include "windbgkd.h"
#include <kddll.h>
#include <ioaccess.h> /* port intrinsics */

typedef struct _KD_PORT_INFORMATION
{
    ULONG ComPort;
    ULONG BaudRate;
    ULONG BaseAddress;
} KD_PORT_INFORMATION, *PKD_PORT_INFORMATION;

BOOLEAN
NTAPI
KdPortInitializeEx(
    IN PKD_PORT_INFORMATION PortInformation,
    IN ULONG Unknown1,
    IN ULONG Unknown2);

BOOLEAN
NTAPI
KdPortGetByteEx(
    IN PKD_PORT_INFORMATION PortInformation,
    OUT PUCHAR ByteReceived);

BOOLEAN
NTAPI
KdPortPollByteEx(
    IN PKD_PORT_INFORMATION PortInformation,
    OUT PUCHAR ByteReceived);

VOID
NTAPI
KdPortPutByteEx(
    IN PKD_PORT_INFORMATION PortInformation,
    IN UCHAR ByteToSend);

#define DEFAULT_BAUD_RATE    19200

#ifdef _M_IX86
const ULONG BaseArray[5] = {0, 0x3F8, 0x2F8, 0x3E8, 0x2E8};
#elif defined(_M_PPC)
const ULONG BaseArray[2] = {0, 0x800003f8};
#elif defined(_M_MIPS)
const ULONG BaseArray[3] = {0, 0x80006000, 0x80007000};
#elif defined(_M_ARM)
const ULONG BaseArray[2] = {0, 0xF1012000};
#elif defined(_M_AMD64)
const ULONG BaseArray[5] = {0, 0x3F8, 0x2F8, 0x3E8, 0x2E8};
#else
#error Unknown architecture
#endif

/* MACROS *******************************************************************/

#define   SER_RBR(x)   ((PUCHAR)(x)+0)
#define   SER_THR(x)   ((PUCHAR)(x)+0)
#define   SER_DLL(x)   ((PUCHAR)(x)+0)
#define   SER_IER(x)   ((PUCHAR)(x)+1)
#define     SR_IER_ERDA   0x01
#define     SR_IER_ETHRE  0x02
#define     SR_IER_ERLSI  0x04
#define     SR_IER_EMS    0x08
#define     SR_IER_ALL    0x0F
#define   SER_DLM(x)   ((PUCHAR)(x)+1)
#define   SER_IIR(x)   ((PUCHAR)(x)+2)
#define   SER_FCR(x)   ((PUCHAR)(x)+2)
#define     SR_FCR_ENABLE_FIFO 0x01
#define     SR_FCR_CLEAR_RCVR  0x02
#define     SR_FCR_CLEAR_XMIT  0x04
#define   SER_LCR(x)   ((PUCHAR)(x)+3)
#define     SR_LCR_CS5 0x00
#define     SR_LCR_CS6 0x01
#define     SR_LCR_CS7 0x02
#define     SR_LCR_CS8 0x03
#define     SR_LCR_ST1 0x00
#define     SR_LCR_ST2 0x04
#define     SR_LCR_PNO 0x00
#define     SR_LCR_POD 0x08
#define     SR_LCR_PEV 0x18
#define     SR_LCR_PMK 0x28
#define     SR_LCR_PSP 0x38
#define     SR_LCR_BRK 0x40
#define     SR_LCR_DLAB 0x80
#define   SER_MCR(x)   ((PUCHAR)(x)+4)
#define     SR_MCR_DTR 0x01
#define     SR_MCR_RTS 0x02
#define     SR_MCR_OUT1 0x04
#define     SR_MCR_OUT2 0x08
#define     SR_MCR_LOOP 0x10
#define   SER_LSR(x)   ((PUCHAR)(x)+5)
#define     SR_LSR_DR  0x01
#define     SR_LSR_TBE 0x20
#define   SER_MSR(x)   ((PUCHAR)(x)+6)
#define     SR_MSR_CTS 0x10
#define     SR_MSR_DSR 0x20
#define   SER_SCR(x)   ((PUCHAR)(x)+7)


/* GLOBAL VARIABLES *********************************************************/

/* STATIC VARIABLES *********************************************************/

//static KD_PORT_INFORMATION DefaultPort = { 0, 0, 0 };

/* The com port must only be initialized once! */
//static BOOLEAN PortInitialized = FALSE;


/* STATIC FUNCTIONS *********************************************************/

/*
static BOOLEAN
KdpDoesComPortExist(
    IN ULONG BaseAddress)
{
    UNIMPLEMENTED;
    return FALSE;
}
*/

/* FUNCTIONS ****************************************************************/

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING  RegistryPath)
{
    return STATUS_SUCCESS;
}

/* HAL.KdPortInitialize */
BOOLEAN
NTAPI
KdPortInitialize(
    IN PKD_PORT_INFORMATION PortInformation,
    IN ULONG Unknown1,
    IN ULONG Unknown2)
{
    UNIMPLEMENTED;
    return FALSE;
}


/* HAL.KdPortInitializeEx */
BOOLEAN
NTAPI
KdPortInitializeEx(
    IN PKD_PORT_INFORMATION PortInformation,
    IN ULONG Unknown1,
    IN ULONG Unknown2)
{
    UNIMPLEMENTED;
    return FALSE;
}


/* HAL.KdPortGetByte */
BOOLEAN
NTAPI
KdPortGetByte(
    OUT PUCHAR ByteReceived)
{
    UNIMPLEMENTED;
    return FALSE;
}


/* HAL.KdPortGetByteEx */
BOOLEAN
NTAPI
KdPortGetByteEx(
    IN PKD_PORT_INFORMATION PortInformation,
    OUT PUCHAR ByteReceived)
{
    UNIMPLEMENTED;
    return FALSE;
}


/* HAL.KdPortPollByte */
BOOLEAN
NTAPI
KdPortPollByte(
    OUT PUCHAR ByteReceived)
{
    UNIMPLEMENTED;
    return FALSE;
}


/* HAL.KdPortPollByteEx */
BOOLEAN
NTAPI
KdPortPollByteEx(
    IN PKD_PORT_INFORMATION PortInformation,
    OUT PUCHAR ByteReceived)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* HAL.KdPortPutByte */
VOID
NTAPI
KdPortPutByte(
    IN UCHAR ByteToSend)
{
    UNIMPLEMENTED;
    return;
}

/* HAL.KdPortPutByteEx */
VOID
NTAPI
KdPortPutByteEx(
    IN PKD_PORT_INFORMATION PortInformation,
    IN UCHAR ByteToSend)
{
    UNIMPLEMENTED;
    return;
}


/* HAL.KdPortRestore */
VOID
NTAPI
KdPortRestore(VOID)
{
    UNIMPLEMENTED;
}


/* HAL.KdPortSave */
VOID
NTAPI
KdPortSave(VOID)
{
    UNIMPLEMENTED;
}


/* HAL.KdPortDisableInterrupts */
BOOLEAN
NTAPI
KdPortDisableInterrupts(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}



/* HAL.KdPortEnableInterrupts */
BOOLEAN
NTAPI
KdPortEnableInterrupts(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KdDebuggerInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KdDebuggerInitialize1(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdSave(
    IN BOOLEAN SleepTransition)
{
    /* Nothing to do on COM ports */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdRestore(
    IN BOOLEAN SleepTransition)
{
    /* Nothing to do on COM ports */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
VOID
NTAPI
KdSendPacket(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData,
    IN OUT PKD_CONTEXT Context)
{
    UNIMPLEMENTED;
    return;
}

/*
 * @unimplemented
 */
KDSTATUS
NTAPI
KdReceivePacket(
    IN ULONG PacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength,
    IN OUT PKD_CONTEXT Context)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
