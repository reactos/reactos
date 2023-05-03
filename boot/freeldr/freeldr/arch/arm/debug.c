/*
 * PROJECT:     Freeldr ARM32
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Arch specific debug
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <freeldr.h>
#include <debug.h>

#define QEMUUART 0x09000000
volatile unsigned int * UART0DR = (unsigned int *) QEMUUART;

BOOLEAN
Rs232PortInitialize(IN ULONG ComPort,
                    IN ULONG BaudRate)
{
    return TRUE;
}

VOID
Rs232PortPutByte(UCHAR ByteToSend)
{
    *UART0DR = ByteToSend;
}

VOID
FrLdrBugCheckWithMessage(
    ULONG BugCode,
    PCHAR File,
    ULONG Line,
    PSTR Format,
    ...)
{
}
