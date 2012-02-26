/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/kdcom.c
 * PURPOSE:         COM port functions for the kernel debugger.
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@ewactos.org)
 */

#include "kddll.h"
#include "kdcom.h"

/* serial debug connection */
#define DEFAULT_DEBUG_PORT      2 /* COM2 */
#define DEFAULT_DEBUG_COM1_IRQ  4 /* COM1 IRQ */
#define DEFAULT_DEBUG_COM2_IRQ  3 /* COM2 IRQ */
#define DEFAULT_DEBUG_BAUD_RATE 115200 /* 115200 Baud */

#define DEFAULT_BAUD_RATE    19200


#if defined(_M_IX86) || defined(_M_AMD64)
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

/* GLOBALS ********************************************************************/

PUCHAR ComPortBase;
ULONG ComPortNumber = DEFAULT_DEBUG_PORT;
ULONG ComPortBaudRate = DEFAULT_DEBUG_BAUD_RATE;
ULONG ComPortIrq = 0;


NTSTATUS
NTAPI
KdpPortInitialize()
{
    ULONG Mode;

    KDDBGPRINT("KdpPortInitialize, Port = COM%ld\n", ComPortNumber);

    /* Enable loop mode (set Bit 4 of the MCR) */
    WRITE_PORT_UCHAR(ComPortBase + COM_MCR, MCR_LOOP);

    /* Clear all modem output bits */
    WRITE_PORT_UCHAR(ComPortBase + COM_MCR, MCR_LOOP);

    /* The upper nibble of the MSR (modem output bits) must be
     * equal to the lower nibble of the MCR (modem input bits) */
    if ((READ_PORT_UCHAR(ComPortBase + COM_MSR) & 0xF0) != 0x00)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Set all modem output bits */
    WRITE_PORT_UCHAR(ComPortBase + COM_MCR, MCR_ALL);

    /* The upper nibble of the MSR (modem output bits) must be
     * equal to the lower nibble of the MCR (modem input bits) */
    if ((READ_PORT_UCHAR(ComPortBase + COM_MSR) & 0xF0) != 0xF0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Enable FIFO */
    WRITE_PORT_UCHAR(ComPortBase + COM_FCR,
                     FCR_ENABLE_FIFO | FCR_CLEAR_RCVR | FCR_CLEAR_XMIT);

    /* Disable interrupts */
    WRITE_PORT_UCHAR(ComPortBase + COM_LCR, 0);
    WRITE_PORT_UCHAR(ComPortBase + COM_IEN, 0);

    /* Enable on DTR and RTS  */
    WRITE_PORT_UCHAR(ComPortBase + COM_MCR, MCR_DTR | MCR_RTS);

    /* Set DLAB */
    WRITE_PORT_UCHAR(ComPortBase + COM_LCR, LCR_DLAB);

    /* Set baud rate */
    Mode = 115200 / ComPortBaudRate;
    WRITE_PORT_UCHAR(ComPortBase + COM_DLL, (UCHAR)(Mode & 0xff));
    WRITE_PORT_UCHAR(ComPortBase + COM_DLM, (UCHAR)((Mode >> 8) & 0xff));

    /* Reset DLAB and set 8 data bits, 1 stop bit, no parity, no break */
    WRITE_PORT_UCHAR(ComPortBase + COM_LCR, LCR_CS8 | LCR_ST1 | LCR_PNO);

    /* Check for 16450/16550 scratch register */
    WRITE_PORT_UCHAR(ComPortBase + COM_SCR, 0xff);
    if (READ_PORT_UCHAR(ComPortBase + COM_SCR) != 0xff)
    {
        return STATUS_INVALID_PARAMETER;
    }
    WRITE_PORT_UCHAR(ComPortBase + COM_SCR, 0x00);
    if (READ_PORT_UCHAR(ComPortBase + COM_SCR) != 0x00)
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

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
    PCHAR CommandLine, PortString, BaudString, IrqString;
    ULONG Value;

    /* Check if e have a LoaderBlock */
    if (LoaderBlock)
    {
        /* HACK */
        KdpDbgPrint = LoaderBlock->u.I386.CommonDataArea;
        KDDBGPRINT("KdDebuggerInitialize0\n");

        /* Get the Command Line */
        CommandLine = LoaderBlock->LoadOptions;

        /* Upcase it */
        _strupr(CommandLine);

        /* Get the port and baud rate */
        PortString = strstr(CommandLine, "DEBUGPORT");
        BaudString = strstr(CommandLine, "BAUDRATE");
        IrqString = strstr(CommandLine, "IRQ");

        /* Check if we got the /DEBUGPORT parameter */
        if (PortString)
        {
            /* Move past the actual string, to reach the port*/
            PortString += strlen("DEBUGPORT");

            /* Now get past any spaces and skip the equal sign */
            while (*PortString == ' ') PortString++;
            PortString++;

            /* Do we have a serial port? */
            if (strncmp(PortString, "COM", 3) != 0)
            {
                return STATUS_INVALID_PARAMETER;
            }

            /* Gheck for a valid Serial Port */
            PortString += 3;
            Value = atol(PortString);
            if (Value > 4)
            {
                return STATUS_INVALID_PARAMETER;
            }

            /* Set the port to use */
            ComPortNumber = Value;
       }

        /* Check if we got a baud rate */
        if (BaudString)
        {
            /* Move past the actual string, to reach the rate */
            BaudString += strlen("BAUDRATE");

            /* Now get past any spaces */
            while (*BaudString == ' ') BaudString++;

            /* And make sure we have a rate */
            if (*BaudString)
            {
                /* Read and set it */
                Value = atol(BaudString + 1);
                if (Value) ComPortBaudRate = Value;
            }
        }

        /* Check Serial Port Settings [IRQ] */
        if (IrqString)
        {
            /* Move past the actual string, to reach the rate */
            IrqString += strlen("IRQ");

            /* Now get past any spaces */
            while (*IrqString == ' ') IrqString++;

            /* And make sure we have an IRQ */
            if (*IrqString)
            {
                /* Read and set it */
                Value = atol(IrqString + 1);
                if (Value) ComPortIrq = Value;
            }
        }
    }

    /* Get base address */
    ComPortBase = UlongToPtr(BaseArray[ComPortNumber]);
    KdComPortInUse = ComPortBase;

    /* Initialize the port */
    return KdpPortInitialize();
}

VOID
NTAPI
KdpSendByte(IN BYTE Byte)
{
    /* Wait for the port to be ready */
    while ((READ_PORT_UCHAR(ComPortBase + COM_LSR) & LSR_TBE) == 0);

    /* This is needed due to subtle timing issues */
    READ_PORT_UCHAR(ComPortBase + COM_MSR);
    while ((READ_PORT_UCHAR(ComPortBase + COM_LSR) & LSR_TBE) == 0);
    READ_PORT_UCHAR(ComPortBase + COM_MSR);

    /* Send the byte */
    WRITE_PORT_UCHAR(ComPortBase + COM_DAT, Byte);
}

KDP_STATUS
NTAPI
KdpPollByte(OUT PBYTE OutByte)
{
    READ_PORT_UCHAR(ComPortBase + COM_MSR); // Timing

    /* Check if data is available */
    if ((READ_PORT_UCHAR(ComPortBase + COM_LSR) & LSR_DR))
    {
        /* Yes, return the byte */
        *OutByte = READ_PORT_UCHAR(ComPortBase + COM_DAT);
        return KDP_PACKET_RECEIVED;
    }

    /* Timed out */
    return KDP_PACKET_TIMEOUT;
}

KDP_STATUS
NTAPI
KdpReceiveByte(OUT PBYTE OutByte)
{
    ULONG Repeats = KdpStallScaleFactor * 100;

    while (Repeats--)
    {
        /* Check if data is available */
        if (KdpPollByte(OutByte) == KDP_PACKET_RECEIVED)
        {
            /* We successfully got a byte */
            return KDP_PACKET_RECEIVED;
        }
    }

    /* Timed out */
    return KDP_PACKET_TIMEOUT;
}

KDP_STATUS
NTAPI
KdpPollBreakIn()
{
    UCHAR Byte;
    if (KdpPollByte(&Byte) == KDP_PACKET_RECEIVED)
    {
        if (Byte == BREAKIN_PACKET_BYTE)
        {
            return KDP_PACKET_RECEIVED;
        }
    }
    return KDP_PACKET_TIMEOUT;
}

NTSTATUS
NTAPI
KdSave(
    IN BOOLEAN SleepTransition)
{
    /* Nothing to do on COM ports */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdRestore(
    IN BOOLEAN SleepTransition)
{
    /* Nothing to do on COM ports */
    return STATUS_SUCCESS;
}

