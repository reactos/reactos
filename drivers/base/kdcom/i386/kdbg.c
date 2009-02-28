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

static KD_PORT_INFORMATION DefaultPort = { 0, 0, 0 };

/* The com port must only be initialized once! */
static BOOLEAN PortInitialized = FALSE;


/* STATIC FUNCTIONS *********************************************************/

static BOOLEAN
KdpDoesComPortExist(
    IN ULONG BaseAddress)
{
    BOOLEAN found;
    UCHAR mcr;
    UCHAR msr;

    found = FALSE;

    /* save Modem Control Register (MCR) */
    mcr = READ_PORT_UCHAR(SER_MCR(BaseAddress));

    /* enable loop mode (set Bit 4 of the MCR) */
    WRITE_PORT_UCHAR(SER_MCR(BaseAddress), SR_MCR_LOOP);

    /* clear all modem output bits */
    WRITE_PORT_UCHAR(SER_MCR(BaseAddress), SR_MCR_LOOP);

    /* read the Modem Status Register */
    msr = READ_PORT_UCHAR(SER_MSR(BaseAddress));

    /*
     * the upper nibble of the MSR (modem output bits) must be
     * equal to the lower nibble of the MCR (modem input bits)
     */
    if ((msr & 0xF0) == 0x00)
    {
        /* set all modem output bits */
        WRITE_PORT_UCHAR(SER_MCR(BaseAddress), SR_MCR_DTR | SR_MCR_RTS | SR_MCR_OUT1 | SR_MCR_OUT2 | SR_MCR_LOOP);

        /* read the Modem Status Register */
        msr = READ_PORT_UCHAR(SER_MSR(BaseAddress));

        /*
         * the upper nibble of the MSR (modem output bits) must be
         * equal to the lower nibble of the MCR (modem input bits)
         */
        if ((msr & 0xF0) == 0xF0)
        {
            /*
             * setup a resonable state for the port:
             * enable fifo and clear recieve/transmit buffers
             */
            WRITE_PORT_UCHAR(SER_FCR(BaseAddress),
                            (SR_FCR_ENABLE_FIFO | SR_FCR_CLEAR_RCVR | SR_FCR_CLEAR_XMIT));
            WRITE_PORT_UCHAR(SER_FCR(BaseAddress), 0);
            READ_PORT_UCHAR(SER_RBR(BaseAddress));
            WRITE_PORT_UCHAR(SER_IER(BaseAddress), 0);
            found = TRUE;
        }
    }

    /* restore MCR */
    WRITE_PORT_UCHAR(SER_MCR(BaseAddress), mcr);

    return found;
}


/* FUNCTIONS ****************************************************************/

/* HAL.KdPortInitialize */
BOOLEAN
NTAPI
KdPortInitialize(
    IN PKD_PORT_INFORMATION PortInformation,
    IN ULONG Unknown1,
    IN ULONG Unknown2)
{
    SIZE_T i;
    CHAR buffer[80];

    if (!PortInitialized)
    {
        DefaultPort.BaudRate = PortInformation->BaudRate;

        if (PortInformation->ComPort == 0)
        {
            for (i = sizeof(BaseArray) / sizeof(BaseArray[0]) - 1; i > 0; i--)
            {
                if (KdpDoesComPortExist(BaseArray[i]))
                {
                    DefaultPort.BaseAddress = BaseArray[i];
                    DefaultPort.ComPort = i;
                    PortInformation->BaseAddress = DefaultPort.BaseAddress;
                    PortInformation->ComPort = DefaultPort.ComPort;
                    break;
                }
            }
            if (i == 0)
            {
                sprintf(buffer,
                        "\nKernel Debugger: No COM port found!\n\n");
                HalDisplayString(buffer);
                return FALSE;
            }
        }

        PortInitialized = TRUE;
    }

    /* initialize port */
    if (!KdPortInitializeEx(&DefaultPort, Unknown1, Unknown2))
        return FALSE;

    /* set global info */
    KdComPortInUse = (PUCHAR)DefaultPort.BaseAddress;

    return TRUE;
}


/* HAL.KdPortInitializeEx */
BOOLEAN
NTAPI
KdPortInitializeEx(
    IN PKD_PORT_INFORMATION PortInformation,
    IN ULONG Unknown1,
    IN ULONG Unknown2)
{
    ULONG ComPortBase;
    CHAR buffer[80];
    ULONG divisor;
    UCHAR lcr;

#ifdef _ARM_
    UNIMPLEMENTED;
    return FALSE;
#endif

    if (PortInformation->BaudRate == 0)
        PortInformation->BaudRate = DEFAULT_BAUD_RATE;

    if (PortInformation->ComPort == 0)
        return FALSE;

    if (!KdpDoesComPortExist(BaseArray[PortInformation->ComPort]))
    {
        sprintf(buffer,
                "\nKernel Debugger: Serial port not found!\n\n");
        HalDisplayString(buffer);
        return FALSE;
    }

    ComPortBase = BaseArray[PortInformation->ComPort];
    PortInformation->BaseAddress = ComPortBase;
#ifndef NDEBUG
    sprintf(buffer,
            "\nSerial port COM%ld found at 0x%lx\n",
            PortInformation->ComPort,
            ComPortBase);
    HalDisplayString(buffer);
#endif /* NDEBUG */

    /* set baud rate and data format (8N1) */

    /*  turn on DTR and RTS  */
    WRITE_PORT_UCHAR(SER_MCR(ComPortBase), SR_MCR_DTR | SR_MCR_RTS);

    /* set DLAB */
    lcr = READ_PORT_UCHAR(SER_LCR(ComPortBase)) | SR_LCR_DLAB;
    WRITE_PORT_UCHAR(SER_LCR(ComPortBase), lcr);

    /* set baud rate */
    divisor = 115200 / PortInformation->BaudRate;
    WRITE_PORT_UCHAR(SER_DLL(ComPortBase), (UCHAR)(divisor & 0xff));
    WRITE_PORT_UCHAR(SER_DLM(ComPortBase), (UCHAR)((divisor >> 8) & 0xff));

    /* reset DLAB and set 8N1 format */
    WRITE_PORT_UCHAR(SER_LCR(ComPortBase),
                     SR_LCR_CS8 | SR_LCR_ST1 | SR_LCR_PNO);

    /* read junk out of the RBR */
    lcr = READ_PORT_UCHAR(SER_RBR(ComPortBase));

#ifndef NDEBUG
    /* print message to blue screen */
    sprintf(buffer,
            "\nKernel Debugger: COM%ld (Port 0x%lx) BaudRate %ld\n\n",
            PortInformation->ComPort,
            ComPortBase,
            PortInformation->BaudRate);

    HalDisplayString(buffer);
#endif /* NDEBUG */

    return TRUE;
}


/* HAL.KdPortGetByte */
BOOLEAN
NTAPI
KdPortGetByte(
    OUT PUCHAR ByteReceived)
{
    if (!PortInitialized)
        return FALSE;
    return KdPortGetByteEx(&DefaultPort, ByteReceived);
}


/* HAL.KdPortGetByteEx */
BOOLEAN
NTAPI
KdPortGetByteEx(
    IN PKD_PORT_INFORMATION PortInformation,
    OUT PUCHAR ByteReceived)
{
    PUCHAR ComPortBase = (PUCHAR)PortInformation->BaseAddress;

    if ((READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_DR))
    {
        *ByteReceived = READ_PORT_UCHAR(SER_RBR(ComPortBase));
        return TRUE;
    }

    return FALSE;
}


/* HAL.KdPortPollByte */
BOOLEAN
NTAPI
KdPortPollByte(
    OUT PUCHAR ByteReceived)
{
    if (!PortInitialized)
        return FALSE;
    return KdPortPollByteEx(&DefaultPort, ByteReceived);
}


/* HAL.KdPortPollByteEx */
BOOLEAN
NTAPI
KdPortPollByteEx(
    IN PKD_PORT_INFORMATION PortInformation,
    OUT PUCHAR ByteReceived)
{
    PUCHAR ComPortBase = (PUCHAR)PortInformation->BaseAddress;

    while ((READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_DR) == 0)
        ;

    *ByteReceived = READ_PORT_UCHAR(SER_RBR(ComPortBase));

    return TRUE;
}


/* HAL.KdPortPutByte */
VOID
NTAPI
KdPortPutByte(
    IN UCHAR ByteToSend)
{
    if (!PortInitialized)
        return;
    KdPortPutByteEx(&DefaultPort, ByteToSend);
}

/* HAL.KdPortPutByteEx */
VOID
NTAPI
KdPortPutByteEx(
    IN PKD_PORT_INFORMATION PortInformation,
    IN UCHAR ByteToSend)
{
    PUCHAR ComPortBase = (PUCHAR)PortInformation->BaseAddress;

    while ((READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_TBE) == 0)
        ;

    WRITE_PORT_UCHAR(SER_THR(ComPortBase), ByteToSend);
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
    UCHAR ch;

    if (!PortInitialized)
        return FALSE;

    ch = READ_PORT_UCHAR(SER_MCR(DefaultPort.BaseAddress));
    ch &= (~(SR_MCR_OUT1 | SR_MCR_OUT2));
    WRITE_PORT_UCHAR(SER_MCR(DefaultPort.BaseAddress), ch);

    ch = READ_PORT_UCHAR(SER_IER(DefaultPort.BaseAddress));
    ch &= (~SR_IER_ALL);
    WRITE_PORT_UCHAR(SER_IER(DefaultPort.BaseAddress), ch);

    return TRUE;
}


/* HAL.KdPortEnableInterrupts */
BOOLEAN
NTAPI
KdPortEnableInterrupts(VOID)
{
    UCHAR ch;

    if (PortInitialized == FALSE)
        return FALSE;

    ch = READ_PORT_UCHAR(SER_IER(DefaultPort.BaseAddress));
    ch &= (~SR_IER_ALL);
    ch |= SR_IER_ERDA;
    WRITE_PORT_UCHAR(SER_IER(DefaultPort.BaseAddress), ch);

    ch = READ_PORT_UCHAR(SER_MCR(DefaultPort.BaseAddress));
    ch &= (~SR_MCR_LOOP);
    ch |= (SR_MCR_OUT1 | SR_MCR_OUT2);
    WRITE_PORT_UCHAR(SER_MCR(DefaultPort.BaseAddress), ch);

    return TRUE;
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
