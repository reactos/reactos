/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kdcom/kdcom.c
 * PURPOSE:         COM port functions for the kernel debugger.
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "kddll.h"

#include <arc/arc.h>
#include <stdlib.h>
#include <ndk/halfuncs.h>

#include <cportlib/cportlib.h>
#include <cportlib/uartinfo.h>
#ifdef _M_ARM64
#include <drivers/acpi/acpi.h>
#endif

/* GLOBALS ********************************************************************/

CPPORT KdComPort;
#ifdef KDDEBUG
CPPORT KdDebugComPort;
#endif

#ifdef _M_ARM64
/* PL011 registers, the SBSA UART is the subset without the control register */
#define KDCOM_PL011_DATA            0x00
#define KDCOM_PL011_FLAGS           0x18
#define KDCOM_PL011_CONTROL         0x30

#define KDCOM_PL011_DATA_ERRORS     0xF00
#define KDCOM_PL011_FLAGS_RX_EMPTY  0x10
#define KDCOM_PL011_FLAGS_TX_FULL   0x20
#define KDCOM_PL011_CONTROL_RUN     0x301

/* Same spin count cportlib uses for a waiting read */
#define KDCOM_PL011_WAIT_COUNT      (1024 * 200)

/* The port is a PL011 or SBSA UART, not a 16550 */
static BOOLEAN KdComPl011;
#endif

/* DEBUGGING ******************************************************************/

#ifdef KDDEBUG
#include <stdio.h>
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

#ifdef _M_ARM64
/**
 * @brief Finds the first DBG2 serial port this driver can run.
 *
 * A 16550 is only taken with byte wide registers, which is what cportlib
 * expects.
 */
static
BOOLEAN
KdpFindDbg2Port(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _Out_ PPHYSICAL_ADDRESS PhysicalAddress,
    _Out_ PUSHORT Subtype)
{
    PDBG2_TABLE Table;
    PDBG2_DEVICE_INFO Entry;
    PGEN_ADDR BaseRegister;
    ULONG Offset, Remaining;
    BOOLEAN Usable, Found = FALSE;

    Table = KdGetAcpiTablePhase0(LoaderBlock, DBG2_SIGNATURE);
    if (!Table)
        return FALSE;

    Offset = Table->DeviceInfoOffset;
    for (Remaining = Table->DeviceInfoCount; Remaining != 0; Remaining--)
    {
        if ((Table->Header.Length < sizeof(*Table)) ||
            (Offset > Table->Header.Length - sizeof(*Entry)))
        {
            break;
        }

        Entry = (PDBG2_DEVICE_INFO)((PUCHAR)Table + Offset);
        if ((Entry->Length < sizeof(*Entry)) ||
            (Entry->Length > Table->Header.Length - Offset))
        {
            break;
        }
        Offset += Entry->Length;

        if ((Entry->PortType != DBG2_PORT_TYPE_SERIAL) ||
            (Entry->RegisterCount == 0) ||
            (Entry->BaseAddressOffset > Entry->Length - sizeof(*BaseRegister)))
        {
            continue;
        }

        /* Only memory mapped ports */
        BaseRegister = (PGEN_ADDR)((PUCHAR)Entry + Entry->BaseAddressOffset);
        if ((BaseRegister->AddressSpaceID != 0) || (BaseRegister->Address.QuadPart == 0))
            continue;

        switch (Entry->PortSubtype)
        {
            case DBG2_SERIAL_16550:
            case DBG2_SERIAL_16550_SUBSET:
                Usable = (BaseRegister->BitWidth == 8);
                break;

            case DBG2_SERIAL_PL011:
            case DBG2_SERIAL_SBSA_32BIT:
            case DBG2_SERIAL_SBSA:
                Usable = TRUE;
                break;

            default:
                Usable = FALSE;
                break;
        }

        if (Usable)
        {
            *PhysicalAddress = BaseRegister->Address;
            *Subtype = Entry->PortSubtype;
            Found = TRUE;
            break;
        }
    }

    KdUnmapVirtualAddress(Table, BYTES_TO_PAGES(Table->Header.Length));
    return Found;
}

/**
 * @brief Maps and starts the debug port.
 *
 * There are no fixed ports on ARM64. Without an address on the command line
 * or a usable DBG2 entry, no device memory is touched.
 */
static
NTSTATUS
KdpFirmwarePortInitialize(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_opt_ PUCHAR PortAddress,
    _In_ ULONG BaudRate)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    USHORT Subtype = DBG2_SERIAL_16550;
    PUCHAR Base;
    NTSTATUS Status;

    /* A port given on the command line is a 16550 */
    if (PortAddress)
        PhysicalAddress.QuadPart = (ULONG_PTR)PortAddress;
    else if (!KdpFindDbg2Port(LoaderBlock, &PhysicalAddress, &Subtype))
        return STATUS_NOT_FOUND;

    Base = KdMapPhysicalMemory64(PhysicalAddress, 1);
    if (!Base)
        return STATUS_INSUFFICIENT_RESOURCES;

    if ((Subtype == DBG2_SERIAL_16550) || (Subtype == DBG2_SERIAL_16550_SUBSET))
    {
        Status = KdpPortInitialize(Base, BaudRate);
        if (!NT_SUCCESS(Status))
            KdUnmapVirtualAddress(Base, 1);
        return Status;
    }

    /* The firmware already set the line up, only make sure it runs */
    if (Subtype == DBG2_SERIAL_PL011)
    {
        WRITE_REGISTER_ULONG((PULONG)(Base + KDCOM_PL011_CONTROL),
                             READ_REGISTER_ULONG((PULONG)(Base + KDCOM_PL011_CONTROL)) |
                             KDCOM_PL011_CONTROL_RUN);
    }

    KdComPort.Address = Base;
    KdComPort.BaudRate = BaudRate;
    KdComPort.Flags = 0;
    KdComPl011 = TRUE;
    KdComPortInUse = Base;
    return STATUS_SUCCESS;
}

/**
 * @brief Reads a byte from the PL011, same results as CpGetByte.
 */
static
USHORT
KdpPl011GetByte(
    _Out_ PUCHAR OutByte,
    _In_ BOOLEAN Wait)
{
    ULONG Tries = Wait ? KDCOM_PL011_WAIT_COUNT : 1;
    ULONG Data;

    while (Tries--)
    {
        if (READ_REGISTER_ULONG((PULONG)(KdComPort.Address + KDCOM_PL011_FLAGS)) &
            KDCOM_PL011_FLAGS_RX_EMPTY)
        {
            continue;
        }

        Data = READ_REGISTER_ULONG((PULONG)(KdComPort.Address + KDCOM_PL011_DATA));
        if (Data & KDCOM_PL011_DATA_ERRORS)
        {
            *OutByte = 0;
            return CP_GET_ERROR;
        }

        *OutByte = (UCHAR)Data;
        return CP_GET_SUCCESS;
    }

    return CP_GET_NODATA;
}
#endif

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

            /* Do we have a serial port? */
            if (_strnicmp(PortString, "COM", CONST_STR_LEN("COM")) != 0)
                return STATUS_INVALID_PARAMETER;

            /* Check for a valid serial port */
            PortString += CONST_STR_LEN("COM");
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

#ifdef _M_ARM64
    UNREFERENCED_PARAMETER(ComPortNumber);
    return KdpFirmwarePortInitialize(LoaderBlock, ComPortAddress, ComPortBaudRate);
#else
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
#endif
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
#ifdef _M_ARM64
    if (KdComPl011)
    {
        while (READ_REGISTER_ULONG((PULONG)(KdComPort.Address + KDCOM_PL011_FLAGS)) &
               KDCOM_PL011_FLAGS_TX_FULL)
        {
            YieldProcessor();
        }

        WRITE_REGISTER_ULONG((PULONG)(KdComPort.Address + KDCOM_PL011_DATA), Byte);
        return;
    }
#endif

    /* Send the byte */
    CpPutByte(&KdComPort, Byte);
}

KDP_STATUS
NTAPI
KdpPollByte(OUT PUCHAR OutByte)
{
    USHORT Status;

    /* Poll the byte */
#ifdef _M_ARM64
    if (KdComPl011)
        Status = KdpPl011GetByte(OutByte, FALSE);
    else
#endif
    Status = CpGetByte(&KdComPort, OutByte, FALSE, FALSE);
    switch (Status)
    {
        case CP_GET_SUCCESS:
            return KDP_PACKET_RECEIVED;

        case CP_GET_NODATA:
            return KDP_PACKET_TIMEOUT;

        case CP_GET_ERROR:
        default:
            return KDP_PACKET_RESEND;
    }
}

KDP_STATUS
NTAPI
KdpReceiveByte(OUT PUCHAR OutByte)
{
    USHORT Status;

    /* Get the byte */
#ifdef _M_ARM64
    if (KdComPl011)
        Status = KdpPl011GetByte(OutByte, TRUE);
    else
#endif
    Status = CpGetByte(&KdComPort, OutByte, TRUE, FALSE);
    switch (Status)
    {
        case CP_GET_SUCCESS:
            return KDP_PACKET_RECEIVED;

        case CP_GET_NODATA:
            return KDP_PACKET_TIMEOUT;

        case CP_GET_ERROR:
        default:
            return KDP_PACKET_RESEND;
    }
}

KDP_STATUS
NTAPI
KdpPollBreakIn(VOID)
{
    KDP_STATUS KdStatus;
    UCHAR Byte;

    KdStatus = KdpPollByte(&Byte);
    if ((KdStatus == KDP_PACKET_RECEIVED) && (Byte == BREAKIN_PACKET_BYTE))
    {
        return KDP_PACKET_RECEIVED;
    }
    return KDP_PACKET_TIMEOUT;
}

/* EOF */
