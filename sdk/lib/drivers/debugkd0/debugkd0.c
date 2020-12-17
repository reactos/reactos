/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            lib/drivers/debugkd0/debugkd0.c
 * PURPOSE:         Kernel phase 0 debug library header
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES *******************************************************************/

#include <ntifs.h>
#include "debugkd0.h"

/* DATA **********************************************************************/

#if DBG_KD0

static CPPORT DbgComPort[4] =
{
    {NULL, 0, TRUE},
    {NULL, 0, TRUE},
    {NULL, 0, TRUE},
    {NULL, 0, TRUE}
};

extern ULONG DbgComPortId;

#endif

/* FUNCTIONS ******************************************************************/

#if DBG_KD0

ULONG
__cdecl
DbgKdPrint0(_In_ PCHAR Format, ...)
{
    USHORT Length;
    va_list ap;
    CHAR PrintBuffer[255];
    PCHAR pChar;
    ULONG Size;

    /* Format the string */
    va_start(ap, Format);
    Length = (USHORT)_vsnprintf(PrintBuffer, sizeof(PrintBuffer), Format, ap);
    va_end(ap);

    /* Send it directly */
    pChar = PrintBuffer;

    for (Size = Length; Size > 0; Size--)
    {
        /* Send the byte */
        CpPutByte(&DbgComPort[DbgComPortId], *pChar);
        pChar++;
    }

    return 0;
}

/*
    if no PortNumber, it set COM2, if no COM2 then COM1
    if no PortAddress, it set to PortNumber
    if no BaudRate, it set  DEFAULT_COM_BAUD_RATE == 19200
*/
BOOLEAN
NTAPI
DbgKdPortInitialize(_In_ ULONG PortNumber OPTIONAL,
                    _In_ PUCHAR PortAddress OPTIONAL,
                    _In_ ULONG BaudRate OPTIONAL,
                    _Out_ PULONG OutPortId)
{
    ULONG PortId;

    /* Check if port given */
    if (PortNumber)
    {
        /* Pick correct address for port */
        if (!PortAddress)
        {
            switch (PortNumber)
            {
                case COM1:
                    PortAddress = COM1_ADDR;
                    break;

                case COM2:
                    PortAddress = COM2_ADDR;
                    break;

                case COM3:
                    PortAddress = COM3_ADDR;
                    break;

                case COM4:
                    PortAddress = COM4_ADDR;
                    break;

                default:
                    /* ??? */
                    PortAddress = COM2_ADDR;
                    PortNumber = COM2;
                    break;
            }
        }

        /* Test port */
        if (!CpDoesPortExist(PortAddress)) {
            return FALSE; // No this port
        }
    }
    else
    {
        /* Default port is COM2 */
        PortAddress = COM2_ADDR;

        /* Test port */
        if (CpDoesPortExist(PortAddress))
        {
            /* Test ok */
            PortNumber = COM2;
        }
        else
        {
            /* No COM2 port. Probe COM1 port */
            PortAddress = COM1_ADDR;

            /* Test port */
            if (!CpDoesPortExist(PortAddress)) {
                return FALSE; // No COM2 or COM1 ports
            }

            /* Test ok */
            PortNumber = COM1;
        }
    }

    PortId = (PortNumber - 1);

    /* Initialize the port unless it's already up, and then return it */
    if (DbgComPort[PortId].Address) {
        return FALSE;
    }

    /* Set default baud rate */
    if (!BaudRate) {
        BaudRate = DEFAULT_COM_BAUD_RATE;
    }

    /* Initialize COM port */
    CpInitialize(&DbgComPort[PortId], PortAddress, BaudRate);

    if (OutPortId) {
        *OutPortId = PortId;
    }

    return TRUE;
}
#else
BOOLEAN
NTAPI
DbgKdPortInitialize(_In_ ULONG PortNumber,
                    _In_ PUCHAR PortAddress,
                    _In_ ULONG BaudRate,
                    _Out_ PULONG OutPortId)
{
    return FALSE;
}

#endif

/* EOF */
