/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 drivers/dd/sndblst/portio.c (see also sndblst.h)
 * PURPOSE:              Sound Blaster port I/O helper
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Sept 28, 2003: Created
 */

#include <ntddk.h>
#include "sndblst.h"

BOOLEAN WaitToSend(ULONG BasePort)
{
    int TimeOut;

    DPRINT("WaitToSend ");

    // Check if it's OK to send
    for (TimeOut = SB_TIMEOUT;
         ! SB_READY_TO_SEND(BasePort) && TimeOut > 0;
         TimeOut --);

    // If a time-out occurs, we report failure
    if (! TimeOut)
    {
        DPRINT("FAILED\n");
        return FALSE;
    }

    DPRINT("SUCCEEDED\n");

    return TRUE;
}

BOOLEAN WaitToReceive(ULONG BasePort)
{
    int TimeOut;

    DPRINT("WaitToReceive ");

    // Check if it's OK to receive
    for (TimeOut = SB_TIMEOUT;
         ! SB_READY_TO_RECEIVE(BasePort) && TimeOut > 0;
         TimeOut --);

    // If a time-out occurs, we report failure
    if (! TimeOut)
    {
        DPRINT("FAILED\n");
        return FALSE;
    }

    DPRINT("SUCCEEDED\n");

    return TRUE;
}


USHORT InitSoundCard(ULONG BasePort)
{
    ULONG TimeOut;
    BOOLEAN Status;
    UCHAR DSP_Major, DSP_Minor;

    DPRINT("InitSoundCard() called\n");

    DPRINT("Resetting sound card\n");
//    if (!WaitToSend(BasePort))
//        return FALSE;

    SB_WRITE_RESET(BasePort, 0x01);
    for (TimeOut = 0; TimeOut < 30000; TimeOut ++); // Wait a while
    SB_WRITE_RESET(BasePort, 0x00);

    // Check if it's OK to receive (some cards will ignore the above reset
    // command and so will not issue an ACK, so time out is NOT an error)
    DPRINT("Waiting for an ACK\n");
    if (WaitToReceive(BasePort))
    {
        // Check to make sure the reset was acknowledged:
        for (TimeOut = SB_TIMEOUT;
             (Status = (SB_READ_DATA(BasePort) != SB_DSP_READY) && (TimeOut > 0));
             TimeOut --);
    }

    DPRINT("Querying DSP version\n");
    if (! WaitToSend(BasePort))
        return FALSE;

    SB_WRITE_DATA(BasePort, SB_GET_DSP_VERSION);

    if (! WaitToReceive(BasePort))
        return FALSE;

    DSP_Major = SB_READ_DATA(BasePort);
    DSP_Minor = SB_READ_DATA(BasePort);

    DPRINT("DSP v%d.%d\n", DSP_Major, DSP_Minor);

    // if audio is disabled,
    // version tests return 0xFF everywhere
    if (DSP_Major == 0xFF && DSP_Minor == 0xFF)
        return FALSE;

    DPRINT("Sound card initialized!\n");

    return (DSP_Major * 256) + DSP_Minor;
}
