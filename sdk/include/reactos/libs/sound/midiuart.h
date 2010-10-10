/*
    ReactOS Sound System
    MIDI UART support

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        26 May 2008 - Created

    Notes:
        MIDI UART is fairly simple. There are two ports - one is a data
        port and is read/write, the other is a command/status port where
        you can write commands, and read status.

        We use a subset of the functionality offered by the original MPU-401
        hardware, which is pretty much the only part implemented in sound
        cards these days, known as "MIDI UART" mode.
*/

#ifndef ROS_MIDIUART
#define ROS_MIDIUART

/* Port read/write abstraction (no wait) */
#define WRITE_MIDIUART_DATA(bp, x)      WRITE_PORT_UCHAR((PUCHAR) bp, x)
#define READ_MIDIUART_DATA(bp)          READ_PORT_UCHAR((PUCHAR) bp)
#define WRITE_MIDIUART_COMMAND(bp, x)   WRITE_PORT_UCHAR((PUCHAR) bp+1, x)
#define READ_MIDIUART_STATUS(bp)        READ_PORT_UCHAR((PUCHAR) bp+1)

/* Status flags */
#define MIDIUART_STATUS_DTR             0x40
#define MIDIUART_STATUS_CTS             0x80


/*
    WaitForMidiUartStatus

    A universal routine for waiting for one or more bits to be set on the
    MIDI UART command/status port. (Not a particularly efficient wait as
    this polls the port until it's ready!)

    If the timeout is reached, the function returns FALSE. Otherwise, when
    the specified flag(s) become set, the function returns TRUE.
*/

BOOLEAN
WaitForMidiUartStatus(
    IN  PUCHAR UartBasePort,
    IN  UCHAR StatusFlags,
    IN  ULONG Timeout);

/* Waits for the CTS status bit to be set */
#define WaitForMidiUartCTS(UartBasePort, Timeout) \
    WaitForMidiUartStatus(UartBasePort, MIDIUART_STATUS_CTS, Timeout)

/* Waits for the DTR status bit to be set */
#define WaitForMidiUartDTR(UartBasePort, Timeout) \
    WaitForMidiUartStatus(UartBasePort, MIDIUART_STATUS_DTR, Timeout)

/*
    WriteMidiUartByte

    Wait for the CTS bit to be set on the command/status port, before
    writing to the data port. If CTS does not get set within the timeout
    period, returns FALSE. Otherwise, returns TRUE.
*/

BOOLEAN
WriteMidiUartByte(
    IN  PUCHAR UartBasePort,
    IN  UCHAR Data,
    IN  ULONG Timeout);


/*
    WriteMidiUartMulti

    Write multiple bytes to the MIDI UART data port. The timeout applies on a
    per-byte basis. If it is reached for any byte, the function will return
    FALSE.

    All data is written "as-is" - there are no checks made as to the validity
    of the data.
*/

BOOLEAN
WriteMidiUartMulti(
    IN  PUCHAR UartBasePort,
    IN  PUCHAR Data,
    IN  ULONG DataLength,
    IN  ULONG Timeout);


/*
    ReadMidiUartByte

    Wait for the DTR bit to be set on the command/status port, before
    reading from the data port. If DTR does not get set within the
    timeout period, returns FALSE. Otherwise, returns TRUE.

    On success, the read data is stored in the location specified by
    the Data parameter.
*/

BOOLEAN
ReadMidiUartByte(
    IN  PUCHAR UartBasePort,
    OUT UCHAR* Data,
    IN  ULONG Timeout);

#endif
