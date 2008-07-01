/*
    ReactOS Sound System
    MIDI UART support

    Author:
        Andrew Greenwood (andrew.greenwood@silverblade.co.uk)

    History:
        26 May 2008 - Created

    Notes:
        Functions documented in midiuart.h
*/

#include <ntddk.h>
#include "midiuart.h"

BOOLEAN
WaitForMidiUartStatus(
    IN  PUCHAR UartBasePort,
    IN  UCHAR StatusFlags,
    IN  ULONG Timeout)
{
    ULONG RemainingTime = Timeout;

    while ( RemainingTime -- )
    {
        if ( READ_MIDIUART_STATUS(UartBasePort) & StatusFlags )
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
WriteMidiUartByte(
    IN  PUCHAR UartBasePort,
    IN  UCHAR Data,
    IN  ULONG Timeout)
{
    if ( ! WaitForMidiUartCTS(UartBasePort, Timeout) )
    {
        return FALSE;
    }

    WRITE_MIDIUART_DATA(UartBasePort, Data);

    return TRUE;
}

BOOLEAN
WriteMidiUartMulti(
    IN  PUCHAR UartBasePort,
    IN  PUCHAR Data,
    IN  ULONG DataLength,
    IN  ULONG Timeout)
{
    ULONG DataIndex;

    for ( DataIndex = 0; DataIndex < DataLength; ++ DataIndex )
    {
        if ( ! WriteMidiUartByte(UartBasePort, Data[DataIndex], Timeout) )
        {
            /* We failed - don't try writing any more */
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
ReadMidiUartByte(
    IN  PUCHAR UartBasePort,
    OUT UCHAR* Data,
    IN  ULONG Timeout)
{
    if ( ! Data )
    {
        return FALSE;
    }

    if ( ! WaitForMidiUartDTR(UartBasePort, Timeout) )
    {
        return FALSE;
    }

    *Data = READ_MIDIUART_DATA(UartBasePort);

    return TRUE;
}


/* Experimental OO-style stuff */
/*
typedef struct _MIDIUART
{
    PUCHAR BasePort;
    ULONG Timeout;
} MIDIUART, *PMIDIUART;

NTSTATUS
MidiUart_Create(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout,
    OUT PMIDIUART* MidiUart)
{
    PMIDIUART NewMidiUart;

    if ( ! MidiUart )
        return STATUS_INVALID_PARAMETER;

    NewMidiUart = ExAllocatePoolWithTag(sizeof(MIDIUART), PAGED_POOL, 'MIDU');

    if ( ! NewMidiUart )
        return STATUS_INSUFFICIENT_RESOURCES;

    NewMidiUart->BasePort = BasePort;
    NewMidiUart->Timeout = Timeout;

    *MidiUart = NewMidiUart;

    return STATUS_SUCCESS;
}

BOOLEAN
MidiUart_WaitForStatus(
    IN  PMIDIUART MidiUart,
    IN  UCHAR StatusFlags)
{
    if ( ! MidiUart)
        return FALSE;

    return WaitForMidiUartStatus(MidiUart->BasePort,
                                 StatusFlags,
                                 MidiUart->Timeout);
}

#define MidiUart_WaitForCTS(inst) \
    MidiUart_WaitForStatus(inst, MIDIUART_STATUS_CTS)

#define MidiUart_WaitForDTR(inst) \
    MidiUart_WaitForStatus(inst, MIDIUART_STATUS_DTR)

BOOLEAN
MidiUart_WriteByte(
    IN  PMIDIUART MidiUart,
    IN  UCHAR Data)
{
    if ( ! MidiUart )
        return FALSE;

    return WriteMidiUartByte(MidiUart->BasePort,
                             Data,
                             MidiUart->Timeout);
}

BOOLEAN
MidiUart_ReadByte(
    IN  PMIDIUART MidiUart,
    OUT PUCHAR Data)
{
    if ( ! MidiUart )
        return FALSE;

    return ReadMidiUartByte(MidiUart->BasePort,
                            Data,
                            MidiUart->Timeout);
}
*/
