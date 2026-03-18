/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Serial Port Boot Driver for Headless Terminal Support
 * COPYRIGHT:   Copyright 2010 ReactOS Portable Systems Group
 *              Copyright 2022-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <cportlib/cportlib.h>
#include <cportlib/uartinfo.h>

/* GLOBALS *******************************************************************/

CPPORT Port[4] =
{
    {NULL, 0, TRUE},
    {NULL, 0, TRUE},
    {NULL, 0, TRUE},
    {NULL, 0, TRUE}
};

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
InbvPortPollOnly(
    _In_ ULONG PortId)
{
    UCHAR Dummy;

    /* Poll a byte from the port */
    return CpGetByte(&Port[PortId], &Dummy, FALSE, TRUE) == CP_GET_SUCCESS;
}

BOOLEAN
NTAPI
InbvPortGetByte(
    _In_ ULONG PortId,
    _Out_ PUCHAR Byte)
{
    /* Read a byte from the port */
    return CpGetByte(&Port[PortId], Byte, TRUE, FALSE) == CP_GET_SUCCESS;
}

VOID
NTAPI
InbvPortPutByte(
    _In_ ULONG PortId,
    _In_ UCHAR Byte)
{
    /* Send the byte */
    CpPutByte(&Port[PortId], Byte);
}

VOID
NTAPI
InbvPortEnableFifo(
    _In_ ULONG PortId,
    _In_ BOOLEAN Enable)
{
    /* Set FIFO as requested */
    CpEnableFifo(Port[PortId].Address, Enable);
}

VOID
NTAPI
InbvPortTerminate(
    _In_ ULONG PortId)
{
    /* The port is now available */
    Port[PortId].Address = NULL;
}

BOOLEAN
NTAPI
InbvPortInitialize(
    _In_ ULONG BaudRate,
    _In_ ULONG PortNumber,
    _In_ PUCHAR PortAddress,
    _Out_ PULONG PortId,
    _In_ BOOLEAN IsMMIODevice)
{
    /* Not yet supported */
    ASSERT(IsMMIODevice == FALSE);

    /* Set the default baud rate */
    if (BaudRate == 0)
        BaudRate = DEFAULT_BAUD_RATE;

    /* Check if the port or address is given */
    if (PortNumber)
    {
        /* Pick correct address for port */
        if (!PortAddress)
        {
            if (PortNumber < 1 || PortNumber > MAX_COM_PORTS)
                PortNumber = MAX_COM_PORTS;
            PortAddress = UlongToPtr(BaseArray[PortNumber]);
        }
    }
    else
    {
        /* Pick correct port for address */
#if defined(SARCH_PC98)
        static const ULONG TestPorts[] = {1, 2};
#else
        static const ULONG TestPorts[] = {2, 1};
#endif
        PortAddress = UlongToPtr(BaseArray[TestPorts[0]]);
        if (CpDoesPortExist(PortAddress))
        {
            PortNumber = TestPorts[0];
        }
        else
        {
            PortAddress = UlongToPtr(BaseArray[TestPorts[1]]);
            if (!CpDoesPortExist(PortAddress))
                return FALSE;
            PortNumber = TestPorts[1];
        }
    }

    /* Initialize the port unless it's already up and return it */
    if (Port[PortNumber - 1].Address)
        return FALSE;

    CpInitialize(&Port[PortNumber - 1], PortAddress, BaudRate);
    *PortId = PortNumber - 1;
    return TRUE;
}

/* EOF */
