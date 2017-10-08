/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/inbv/inbvport.c
 * PURPOSE:         Serial Port Boot Driver for Headless Terminal Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <debug.h>

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
InbvPortPollOnly(IN ULONG PortId)
{
    UCHAR Dummy;

    /* Poll a byte from the port */
    return CpGetByte(&Port[PortId], &Dummy, FALSE, TRUE) == CP_GET_SUCCESS;
}

BOOLEAN
NTAPI
InbvPortGetByte(IN  ULONG  PortId,
                OUT PUCHAR Byte)
{
    /* Read a byte from the port */
    return CpGetByte(&Port[PortId], Byte, TRUE, FALSE) == CP_GET_SUCCESS;
}

VOID
NTAPI
InbvPortPutByte(IN ULONG PortId,
                IN UCHAR Byte)
{
    /* Send the byte */
    CpPutByte(&Port[PortId], Byte);
}

VOID
NTAPI
InbvPortEnableFifo(IN ULONG   PortId,
                   IN BOOLEAN Enable)
{
    /* Set FIFO as requested */
    CpEnableFifo(Port[PortId].Address, Enable);
}

VOID
NTAPI
InbvPortTerminate(IN ULONG PortId)
{
    /* The port is now available */
    Port[PortId].Address = NULL;
}

BOOLEAN
NTAPI
InbvPortInitialize(IN  ULONG   BaudRate,
                   IN  ULONG   PortNumber,
                   IN  PUCHAR  PortAddress,
                   OUT PULONG  PortId,
                   IN  BOOLEAN IsMMIODevice)
{
    /* Not yet supported */
    ASSERT(IsMMIODevice == FALSE);

    /* Set default baud rate */
    if (BaudRate == 0) BaudRate = 19200;

    /* Check if port or address given */
    if (PortNumber)
    {
        /* Pick correct address for port */
        if (!PortAddress)
        {
            switch (PortNumber)
            {
                case 1:
                    PortAddress = (PUCHAR)0x3F8;
                    break;

                case 2:
                    PortAddress = (PUCHAR)0x2F8;
                    break;

                case 3:
                    PortAddress = (PUCHAR)0x3E8;
                    break;

                default:
                    PortNumber = 4;
                    PortAddress = (PUCHAR)0x2E8;
            }
        }
    }
    else
    {
        /* Pick correct port for address */
        PortAddress = (PUCHAR)0x2F8;
        if (CpDoesPortExist(PortAddress))
        {
            PortNumber = 2;
        }
        else
        {
            PortAddress = (PUCHAR)0x3F8;
            if (!CpDoesPortExist(PortAddress)) return FALSE;
            PortNumber = 1;
        }
    }

    /* Initialize the port unless it's already up, and then return it */
    if (Port[PortNumber - 1].Address) return FALSE;

    CpInitialize(&Port[PortNumber - 1], PortAddress, BaudRate);
    *PortId = PortNumber - 1;

    return TRUE;
}

/* EOF */
