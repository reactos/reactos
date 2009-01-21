/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kdcom/kdbg.c
 * PURPOSE:         Serial i/o functions for the kernel debugger.
 * PROGRAMMER:      Alex Ionescu
 *                  Hervé Poussineau
 *                  Timo Kreuzer
 */

/* INCLUDES *****************************************************************/

#define NOEXTAPI
#include <ntddk.h>
#define NDEBUG
#include <halfuncs.h>
#include <stdio.h>
#include <stdlib.h>
#include <debug.h>
#include "arc/arc.h"
#include "windbgkd.h"
#include <kddll.h>
#include <ioaccess.h> /* port intrinsics */

typedef struct _KD_PORT_INFORMATION
{
    ULONG ComPort;
    ULONG BaudRate;
    ULONG_PTR BaseAddress;
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

/* serial debug connection */
#define DEFAULT_DEBUG_PORT      2 /* COM2 */
#define DEFAULT_DEBUG_COM1_IRQ  4 /* COM1 IRQ */
#define DEFAULT_DEBUG_COM2_IRQ  3 /* COM2 IRQ */
#define DEFAULT_DEBUG_BAUD_RATE 115200 /* 115200 Baud */

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

ULONG CurrentPacketId = INITIAL_PACKET_ID;

/* STATIC VARIABLES *********************************************************/

static KD_PORT_INFORMATION DefaultPort = { 0, 0, 0 };

/* The com port must only be initialized once! */
static BOOLEAN PortInitialized = FALSE;

ULONG KdpPort;
ULONG KdpPortIrq;

// HACK!!!
typedef ULONG (*DBGRNT)(const char *Format, ...);
DBGRNT FrLdrDbgPrint = 0;

/* STATIC FUNCTIONS *********************************************************/

static BOOLEAN
KdpDoesComPortExist(
    IN ULONG_PTR BaseAddress)
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
    *KdComPortInUse = (PUCHAR)DefaultPort.BaseAddress;

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
    ULONG_PTR ComPortBase;
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

/* NEW INTERNAL FUNCTIONS ****************************************************/

/******************************************************************************
 * \name KdpCalculateChecksum
 * \brief Calculates the checksum for the packet data.
 * \param Buffer Pointer to the packet data.
 * \param Length Length of data in bytes.
 * \return The calculated checksum.
 * \sa http://www.vista-xp.co.uk/forums/technical-reference-library/2540-basics-debugging.html
 */
ULONG
NTAPI
KdpCalculateChecksum(
    IN PVOID Buffer,
    IN ULONG Length)
{
    ULONG i, Checksum = 0;

    for (i = 0; i < Length; i++)
    {
        Checksum += ((PUCHAR)Buffer)[i];
    }
    return Checksum;
}

/******************************************************************************
 * \name KdpSendBuffer
 * \brief Sends a buffer of data to the KD port.
 * \param Buffer Pointer to the data.
 * \param Size Size of data in bytes.
 */
VOID
NTAPI
KdpSendBuffer(
    IN PVOID Buffer,
    IN ULONG Size)
{
    INT i;
    for (i = 0; i < Size; i++)
    {
        KdPortPutByteEx(&DefaultPort, ((PUCHAR)Buffer)[i]);
    }
}


/******************************************************************************
 * \name KdpReceiveBuffer
 * \brief Recieves data from the KD port and fills a buffer.
 * \param Buffer Pointer to a buffer that receives the data.
 * \param Size Size of data to receive in bytes.
 * \return KdPacketReceived if successful. 
 *         KdPacketTimedOut if the receice timed out (10 seconds).
 * \todo Handle timeout.
 */
KDSTATUS
NTAPI
KdpReceiveBuffer(
    OUT PVOID Buffer,
    IN  ULONG Size)
{
    ULONG i;
    PUCHAR ByteBuffer = Buffer;
    BOOLEAN Ret, TimeOut;

    for (i = 0; i < Size; i++)
    {
        do
        {
            Ret = KdPortGetByteEx(&DefaultPort, &ByteBuffer[i]);
            TimeOut = FALSE; // FIXME timeout after 10 Sec
        }
        while (!Ret || TimeOut);

        if (TimeOut)
        {
            return KdPacketTimedOut;
        }
//        FrLdrDbgPrint("Received byte: %x\n", ByteBuffer[i]);
    }

    return KdPacketReceived;
}

KDSTATUS
NTAPI
KdpReceivePacketLeader(
    OUT PULONG PacketLeader)
{
    UCHAR Byte, PrevByte;
    ULONG i, Temp;
    KDSTATUS RcvCode;

    Temp = 0;
    PrevByte = 0;
    for (i = 0; i < 4; i++)
    {
        RcvCode = KdpReceiveBuffer(&Byte, sizeof(UCHAR));
        Temp = (Temp << 8) | Byte;
        if ( (RcvCode != KdPacketReceived) ||
             ((Byte != PACKET_LEADER_BYTE) &&
              (Byte != CONTROL_PACKET_LEADER_BYTE)) ||
             (PrevByte != 0 && Byte != PrevByte) )
        {
            return KdPacketNeedsResend;
        }
        PrevByte = Byte;
    }
    
    *PacketLeader = Temp;

    return KdPacketReceived;
}


VOID
NTAPI
KdpSendControlPacket(
    IN USHORT PacketType,
    IN ULONG PacketId OPTIONAL)
{
    KD_PACKET Packet;

    Packet.PacketLeader = CONTROL_PACKET_LEADER;
    Packet.PacketId = PacketId;
    Packet.ByteCount = 0;
    Packet.Checksum = 0;
    Packet.PacketType = PacketType;

    KdpSendBuffer(&Packet, sizeof(KD_PACKET));
}


/* NEW PUBLIC FUNCTIONS ******************************************************/

/******************************************************************************
 * \name KdDebuggerInitialize0
 * \brief Phase 0 initialization.
 * \param [opt] LoaderBlock Pointer to the Loader parameter block. Can be NULL.
 * \return Status
 */
NTSTATUS
NTAPI
KdDebuggerInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    ULONG Value;
    PCHAR CommandLine, Port, BaudRate, Irq;

    /* Apply default values */
    KdpPortIrq = 0;
    DefaultPort.ComPort = DEFAULT_DEBUG_PORT;
    DefaultPort.BaudRate = DEFAULT_DEBUG_BAUD_RATE;

    /* Check if e have a LoaderBlock */
    if (LoaderBlock)
    {
        /* Get the Command Line */
        CommandLine = LoaderBlock->LoadOptions;

        /* Upcase it */
        _strupr(CommandLine);

        /* Get the port and baud rate */
        Port = strstr(CommandLine, "DEBUGPORT");
        BaudRate = strstr(CommandLine, "BAUDRATE");
        Irq = strstr(CommandLine, "IRQ");

        /* Check if we got the /DEBUGPORT parameter */
        if (Port)
        {
            /* Move past the actual string, to reach the port*/
            Port += strlen("DEBUGPORT");

            /* Now get past any spaces and skip the equal sign */
            while (*Port == ' ') Port++;
            Port++;

            /* Do we have a serial port? */
            if (strncmp(Port, "COM", 3) != 0)
            {
                return STATUS_INVALID_PARAMETER;
            }

            /* Gheck for a valid Serial Port */
            Port += 3;
            Value = atol(Port);
            if (Value > 4)
            {
                return STATUS_INVALID_PARAMETER;
            }

            /* Set the port to use */
            DefaultPort.ComPort = Value;
       }

        /* Check if we got a baud rate */
        if (BaudRate)
        {
            /* Move past the actual string, to reach the rate */
            BaudRate += strlen("BAUDRATE");

            /* Now get past any spaces */
            while (*BaudRate == ' ') BaudRate++;

            /* And make sure we have a rate */
            if (*BaudRate)
            {
                /* Read and set it */
                Value = atol(BaudRate + 1);
                if (Value) DefaultPort.BaudRate = Value;
            }
        }

        /* Check Serial Port Settings [IRQ] */
        if (Irq)
        {
            /* Move past the actual string, to reach the rate */
            Irq += strlen("IRQ");

            /* Now get past any spaces */
            while (*Irq == ' ') Irq++;

            /* And make sure we have an IRQ */
            if (*Irq)
            {
                /* Read and set it */
                Value = atol(Irq + 1);
                if (Value) KdpPortIrq = Value;
            }
        }
    }

    // HACK use com1 for FrLdrDbg, com2 for WinDbg
    DefaultPort.ComPort = 2;

    /* Get base address */
    DefaultPort.BaseAddress = BaseArray[DefaultPort.ComPort];

    /* Check if the COM port does exist */
    if (!KdpDoesComPortExist(DefaultPort.BaseAddress))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize the port */
    KdPortInitializeEx(&DefaultPort, 0, 0);
    PortInitialized = TRUE;

    return STATUS_SUCCESS;
}

/******************************************************************************
 * \name KdDebuggerInitialize1
 * \brief Phase 1 initialization.
 * \param [opt] LoaderBlock Pointer to the Loader parameter block. Can be NULL.
 * \return Status
 */
NTSTATUS
NTAPI
KdDebuggerInitialize1(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    // HACK: misuse this function to get a pointer to FrLdrDbgPrint
    FrLdrDbgPrint = (PVOID)LoaderBlock;
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
    KD_PACKET Packet;
    KDSTATUS RcvCode;

    for (;;)
    {
        /* Initialize a KD_PACKET */
        Packet.PacketLeader = PACKET_LEADER;
        Packet.PacketType = PacketType;
        Packet.ByteCount = MessageHeader->Length;
        Packet.Checksum = KdpCalculateChecksum(MessageHeader->Buffer,
                                               MessageHeader->Length);

        /* If we have message data, add it to the packet */
        if (MessageData)
        {
            Packet.ByteCount += MessageData->Length;
            Packet.Checksum += KdpCalculateChecksum(MessageData->Buffer,
                                                    MessageData->Length);
        }

        /* Set the packet id */
        Packet.PacketId = CurrentPacketId;

        /* Send the packet header to the KD port */
        KdpSendBuffer(&Packet, sizeof(KD_PACKET));

        /* Send the message header */
        KdpSendBuffer(MessageHeader->Buffer, MessageHeader->Length);

        /* If we have meesage data, also send it */
        if (MessageData)
        {
            KdpSendBuffer(MessageData->Buffer, MessageData->Length);
        }

        /* Finalize with a trailing byte */
        KdPortPutByte(PACKET_TRAILING_BYTE);

        /* Wait for acknowledge */
        RcvCode = KdReceivePacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                                  NULL,
                                  NULL,
                                  0,
                                  NULL);

        /* Did we succeed? */
        if (RcvCode == KdPacketReceived)
        {
            break;
        }

        /* PACKET_TYPE_KD_DEBUG_IO is allowed to instantly timeout */
        if (PacketType == PACKET_TYPE_KD_DEBUG_IO)
        {
            /* No response, silently fail. */
            return;
        }

        /* Packet timed out, send it again */
    }

    return;
}


/******************************************************************************
 * \name KdReceivePacket
 * \brief Receive a packet from the KD port.
 * \param [in] PacketType Describes the type of the packet to receive.
 *        This can be one of the PACKET_TYPE_ constants.
 * \param [out] MessageHeader Pointer to a STRING structure for the header.
 * \param [out] MessageData Pointer to a STRING structure for the data.
 * \return KdPacketReceived if successful, KdPacketTimedOut if the receive
 *         timed out, KdPacketNeedsResend to signal that the last packet needs
 *         to be sent again.
 * \note If PacketType is PACKET_TYPE_KD_POLL_BREAKIN, the function doesn't
 *       wait for any data, but returns KdPacketTimedOut instantly if no breakin
 *       packet byte is received.
 * \sa http://www.nynaeve.net/?p=169
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
    UCHAR Byte = 0;
    KDSTATUS RcvCode;
    KD_PACKET Packet;
    ULONG Checksum;

    /* Special handling for breakin packet */
    if(PacketType == PACKET_TYPE_KD_POLL_BREAKIN)
    {
        if (KdPortGetByteEx(&DefaultPort, &Byte))
        {
            if (Byte == BREAKIN_PACKET_BYTE)
            {
                return KdPacketReceived;
            }
        }
        return KdPacketTimedOut;
    }

    for (;;)
    {
        FrLdrDbgPrint("KdReceivePacket 0\n");

        /* Step 1 - Read PacketLeader */
        RcvCode = KdpReceivePacketLeader(&Packet.PacketLeader);
        if (RcvCode != KdPacketReceived)
        {
            /* Couldn't read a correct packet leader. Start over. */
            continue;
        }

        FrLdrDbgPrint("KdReceivePacket 1, PacketLeader == 0x%x\n", Packet.PacketLeader);

        /* Step 2 - Read PacketType */
        RcvCode = KdpReceiveBuffer(&Packet.PacketType, sizeof(USHORT));
        if (RcvCode != KdPacketReceived)
        {
            /* Didn't receive a PacketType or PacketType is bad. Start over. */
            continue;
        }

        FrLdrDbgPrint("KdReceivePacket 2, PacketType == 0x%x\n", Packet.PacketType);

        /* Step 3 - Read ByteCount */
        RcvCode = KdpReceiveBuffer(&Packet.ByteCount, sizeof(USHORT));
        if (RcvCode != KdPacketReceived || Packet.ByteCount > PACKET_MAX_SIZE)
        {
            /* Didn't receive ByteCount or it's too big. Start over. */
            continue;
        }

        FrLdrDbgPrint("KdReceivePacket 3, ByteCount == 0x%x\n", Packet.ByteCount);

        /* Step 4 - Read PacketId */
        RcvCode = KdpReceiveBuffer(&Packet.PacketId, sizeof(ULONG));
        if (RcvCode != KdPacketReceived)
        {
            /* Didn't receive PacketId. Start over. */
            continue;
        }

        FrLdrDbgPrint("KdReceivePacket 4, PacketId == 0x%x\n", Packet.PacketId);
/*
        if (Packet.PacketId != ExpectedPacketId)
        {
            // Ask for a resend!
            continue;
        }
*/

        /* Step 5 - Read Checksum */
        RcvCode = KdpReceiveBuffer(&Packet.Checksum, sizeof(ULONG));
        if (RcvCode != KdPacketReceived)
        {
            /* Didn't receive Checksum. Start over. */
            continue;
        }

        FrLdrDbgPrint("KdReceivePacket 5, Checksum == 0x%x\n", Packet.Checksum);

        /* Step 6 - Handle control packets */
        if (Packet.PacketLeader == CONTROL_PACKET_LEADER)
        {
            switch (Packet.PacketType)
            {
                case PACKET_TYPE_KD_ACKNOWLEDGE:
                    if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
                    {
                        /* Remote acknowledges the last packet */
                        return KdPacketReceived;
                    }
                    continue;

                case PACKET_TYPE_KD_RESEND:
                    /* Remote wants us to resend the last packet */
                    return KdPacketNeedsResend;

                case PACKET_TYPE_KD_RESET:
                    FrLdrDbgPrint("KdReceivePacket - got a reset packet\n");
                    KdpSendControlPacket(PACKET_TYPE_KD_RESET, 0);
                    CurrentPacketId = INITIAL_PACKET_ID;
                    return KdPacketNeedsResend;

                default:
                    FrLdrDbgPrint("KdReceivePacket - got unknown control packet\n");
                    return KdPacketNeedsResend;
            }
        }

        /* Did we wait for an ack packet? */
        if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
        {
            /* We received something different, start over */
            continue;
        }

        /* Did we get the right packet type? */
        if (PacketType != Packet.PacketType)
        {
            /* We received something different, start over */
            continue;
        }

        /* Get size of the message header */
        switch (Packet.PacketType)
        {
            case PACKET_TYPE_KD_STATE_CHANGE64:
                MessageHeader->Length = sizeof(DBGKD_WAIT_STATE_CHANGE64);
                break;

            case PACKET_TYPE_KD_STATE_MANIPULATE:
                MessageHeader->Length = sizeof(DBGKD_MANIPULATE_STATE64);
                break;

            case PACKET_TYPE_KD_DEBUG_IO:
                MessageHeader->Length = sizeof(DBGKD_DEBUG_IO);
                break;

            default:
                FrLdrDbgPrint("KdReceivePacket - unknown PacketType\n");
                return KdPacketNeedsResend;
        }

FrLdrDbgPrint("KdReceivePacket - got normal PacketType\n");

        /* Packet smaller than expected? */
        if (MessageHeader->Length > Packet.ByteCount)
        {
            FrLdrDbgPrint("KdReceivePacket - too few data (%d) for type %d\n",
                          Packet.ByteCount, MessageHeader->Length);
            MessageHeader->Length = Packet.ByteCount;
        }

FrLdrDbgPrint("KdReceivePacket - got normal PacketType, Buffer = %p\n", MessageHeader->Buffer);

        /* Receive the message header data */
        RcvCode = KdpReceiveBuffer(MessageHeader->Buffer,
                                   MessageHeader->Length);
        if (RcvCode != KdPacketReceived)
        {
            /* Didn't receive data. Start over. */
            FrLdrDbgPrint("KdReceivePacket - Didn't receive message header data. Start over\n");
            continue;
        }

FrLdrDbgPrint("KdReceivePacket - got normal PacketType 3\n");

        /* Calculate checksum for the header data */
        Checksum = KdpCalculateChecksum(MessageHeader->Buffer,
                                        MessageHeader->Length);

        /* Shall we receive messsage data? */
        if (MessageData && Packet.ByteCount > MessageHeader->Length)
        {
            FrLdrDbgPrint("KdReceivePacket - got normal PacketType 2b\n");
            /* Calculate the length of the message data */
            MessageData->Length = Packet.ByteCount - MessageHeader->Length;

            /* Receive the message data */
            RcvCode = KdpReceiveBuffer(MessageData->Buffer,
                                       MessageData->Length);
            if (RcvCode != KdPacketReceived)
            {
                /* Didn't receive data. Start over. */
                FrLdrDbgPrint("KdReceivePacket - Didn't receive message data. Start over\n");
                continue;
            }

            /* Add cheksum for message data */
            Checksum += KdpCalculateChecksum(MessageData->Buffer,
                                             MessageData->Length);
        }

        /* Compare checksum */
        if (Packet.Checksum != Checksum)
        {
            // Send PACKET_TYPE_KD_RESEND
            FrLdrDbgPrint("KdReceivePacket - wrong cheksum, got %x, calculated %x\n",
                          Packet.Checksum, Checksum);
            continue;
        }

        /* We must receive a PACKET_TRAILING_BYTE now */
        RcvCode = KdpReceiveBuffer(&Byte, sizeof(UCHAR));

        /* Acknowledge the received packet */
        KdpSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE, CurrentPacketId);
        CurrentPacketId ^= 1;

FrLdrDbgPrint("KdReceivePacket - all ok\n");

        return KdPacketReceived;



    }

    return KdPacketReceived;
}

/* EOF */
