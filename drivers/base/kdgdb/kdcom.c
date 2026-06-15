/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kdgdb/kdcom.c
 * PURPOSE:         COM port functions for the kernel debugger.
 */

#include "kdgdb.h"

#include <arc/arc.h>
#include <stdlib.h>
#include <ndk/halfuncs.h>

#include <cportlib/cportlib.h>
#include <cportlib/uartinfo.h>

/* GLOBALS ********************************************************************/

CPPORT KdComPort;
#ifdef KDDEBUG
CPPORT KdDebugComPort;
#endif

/* DEBUGGING ******************************************************************/

#ifdef KDDEBUG
ULONG KdpDbgPrint(const char *Format, ...)
{
    va_list ap;
    int Length;
    char* ptr;
    CHAR Buffer[512];

    va_start(ap, Format);
    Length = _vsnprintf(Buffer, sizeof(Buffer), Format, ap);
    va_end(ap);

    /* Check if we went past the buffer */
    if (Length == -1)
    {
        /* Terminate it if we went over-board */
        Buffer[sizeof(Buffer) - 1] = '\n';

        /* Put maximum */
        Length = sizeof(Buffer);
    }

    ptr = Buffer;
    while (Length--)
    {
        if (*ptr == '\n')
            CpPutByte(&KdDebugComPort, '\r');

        CpPutByte(&KdDebugComPort, *ptr++);
    }

    return 0;
}
#endif

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
KdD0Transition(VOID)
{
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdD3Transition(VOID)
{
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdSave(IN BOOLEAN SleepTransition)
{
    /* Nothing to do on COM ports */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdRestore(IN BOOLEAN SleepTransition)
{
    /* Nothing to do on COM ports */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdpPortInitialize(
    _In_ PUCHAR PortAddress,
    _In_ ULONG BaudRate)
{
    NTSTATUS Status;

    Status = CpInitialize(&KdComPort, PortAddress, BaudRate);
    if (!NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER;

    KdComPortInUse = KdComPort.Address;
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
KdDebuggerInitialize0(IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
#define CONST_STR_LEN(x) (sizeof(x)/sizeof(x[0]) - 1)

    ULONG ComPortNumber   = DEFAULT_DEBUG_PORT;
    ULONG ComPortBaudRate = DEFAULT_DEBUG_BAUD_RATE;
    PUCHAR ComPortAddress = NULL;

    PSTR CommandLine, PortString, BaudString;
    ULONG Value;

    /* Check if we have a LoaderBlock */
    if (LoaderBlock)
    {
        /* Get the Command Line */
        CommandLine = LoaderBlock->LoadOptions;

        /* Upcase it */
        _strupr(CommandLine);

        /* Get the port and baud rate */
        PortString = strstr(CommandLine, "DEBUGPORT");
        BaudString = strstr(CommandLine, "BAUDRATE");

        /* Check if we got the DEBUGPORT parameter */
        if (PortString)
        {
            /* Move past the actual string and any spaces */
            PortString += CONST_STR_LEN("DEBUGPORT");
            while (*PortString == ' ') ++PortString;
            /* Skip the equals sign */
            if (*PortString) ++PortString;

            /* Do we have a serial port? Recognize both
             * 'DEBUGPORT=GDB' and 'DEBUGPORT=COM' syntaxes. */
            if (_strnicmp(PortString, "GDB", CONST_STR_LEN("GDB")) != 0 &&
                _strnicmp(PortString, "COM", CONST_STR_LEN("COM")) != 0)
            {
                return STATUS_INVALID_PARAMETER;
            }

            /* Check for a valid serial port */
            PortString += CONST_STR_LEN("COM"); // Same as len("GDB")
            if (*PortString != ':')
            {
                Value = (ULONG)atol(PortString);
                if (Value > MAX_COM_PORTS)
                    return STATUS_INVALID_PARAMETER;
                // if (Value > 0 && Value <= MAX_COM_PORTS)
                /* Set the port to use */
                ComPortNumber = Value;
            }
            else
            {
                /* Retrieve and set its address */
                Value = strtoul(PortString + 1, NULL, 0);
                if (Value)
                {
                    ComPortNumber = 0;
                    ComPortAddress = UlongToPtr(Value);
                }
            }
        }

        /* Check if we got a baud rate */
        if (BaudString)
        {
            /* Move past the actual string and any spaces */
            BaudString += CONST_STR_LEN("BAUDRATE");
            while (*BaudString == ' ') ++BaudString;

            /* Make sure we have a rate */
            if (*BaudString)
            {
                /* Read and set it */
                Value = (ULONG)atol(BaudString + 1);
                if (Value) ComPortBaudRate = Value;
            }
        }
    }

    if (!ComPortAddress)
        ComPortAddress = UlongToPtr(BaseArray[ComPortNumber]);

#ifdef KDDEBUG
    /*
     * Try to find a free COM port and use it as the KD debugging port.
     * NOTE: Inspired by freeldr/comm/rs232.c, Rs232PortInitialize(...)
     */
    {
    /*
     * Enumerate COM ports from the last to the first one, and stop
     * when we find a valid port. If we reach the first list element
     * (the undefined COM port), no valid port was found.
     */
    PUCHAR Address = NULL;
    ULONG ComPort;
    for (ComPort = MAX_COM_PORTS; ComPort > 0; ComPort--)
    {
        /* Check if the port exist; skip the KD port */
        Address = UlongToPtr(BaseArray[ComPort]);
        if ((Address != ComPortAddress) && CpDoesPortExist(Address))
            break;
    }
    if (ComPort != 0 && Address != NULL)
        CpInitialize(&KdDebugComPort, Address, DEFAULT_BAUD_RATE);
    }
#endif

    /* Initialize the port */
    return KdpPortInitialize(ComPortAddress, ComPortBaudRate);
}

/******************************************************************************
 * \name KdDebuggerInitialize1
 * \brief Phase 1 initialization.
 * \param [opt] LoaderBlock Pointer to the Loader parameter block. Can be NULL.
 * \return Status
 */
NTSTATUS
NTAPI
KdDebuggerInitialize1(IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    return STATUS_SUCCESS;
}


VOID
NTAPI
KdpSendByte(IN UCHAR Byte)
{
    /* Send the byte */
    CpPutByte(&KdComPort, Byte);
}

KDSTATUS
NTAPI
KdpPollByte(OUT PUCHAR OutByte)
{
    /* Poll the byte */
    if (CpGetByte(&KdComPort, OutByte, FALSE, FALSE) == CP_GET_SUCCESS)
    {
        return KdPacketReceived;
    }
    else
    {
        return KdPacketTimedOut;
    }
}

KDSTATUS
NTAPI
KdpReceiveByte(OUT PUCHAR OutByte)
{
    USHORT CpStatus;

    do
    {
        CpStatus = CpGetByte(&KdComPort, OutByte, TRUE, FALSE);
    } while (CpStatus == CP_GET_NODATA);

    /* Get the byte */
    if (CpStatus == CP_GET_SUCCESS)
    {
        return KdPacketReceived;
    }

    return KdPacketTimedOut;
}

KDSTATUS
NTAPI
KdpPollBreakIn(VOID)
{
    KDSTATUS KdStatus;
    UCHAR Byte;

    KdStatus = KdpPollByte(&Byte);
    if (KdStatus == KdPacketReceived)
    {
        if (Byte == 0x03)
        {
            KDDBGPRINT("BreakIn Polled.\n");
            return KdPacketReceived;
        }
        else if (Byte == '$')
        {
            /* GDB tried to send a new packet. N-ack it. */
            KdpSendByte('-');
        }
    }
    return KdPacketTimedOut;
}

/* EOF */
