/*
    ReactOS Sound System
    MIDI UART support

    Author:
        Andrew Greenwood (silverblade@reactos.org)

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
