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

#include <windows.h>
#include <ddk/ntddk.h>
#include "sndblst.h"


BOOLEAN WaitToSend(UINT BasePort)
{
    int TimeOut;
    
    DbgPrint("WaitToSend ");

    // Check if it's OK to send
    for (TimeOut = SB_TIMEOUT;
         ! SB_READY_TO_SEND(BasePort) && TimeOut > 0;
         TimeOut --);

    // If a time-out occurs, we report failure
    if (! TimeOut)
    {
        DbgPrint("FAILED\n");
        return FALSE;
    }
    
    DbgPrint("SUCCEEDED\n");

    return TRUE;
}

BOOLEAN WaitToReceive(UINT BasePort)
{
    int TimeOut;

    DbgPrint("WaitToReceive ");

    // Check if it's OK to receive
    for (TimeOut = SB_TIMEOUT;
         ! SB_READY_TO_RECEIVE(BasePort) && TimeOut > 0;
         TimeOut --);

    // If a time-out occurs, we report failure
    if (! TimeOut)
    {
        DbgPrint("FAILED\n");
        return FALSE;
    }
    
    DbgPrint("SUCCEEDED\n");

    return TRUE;
}


WORD InitSoundCard(UINT BasePort)
{
    UINT TimeOut;
    BOOLEAN Status;
    UCHAR DSP_Major, DSP_Minor;

    DbgPrint("InitSoundCard() called\n");

    DbgPrint("Resetting sound card\n");
//    if (!WaitToSend(BasePort))
//        return FALSE;

    SB_WRITE_RESET(BasePort, 0x01);
    for (TimeOut = 0; TimeOut < 30000; TimeOut ++); // Wait a while
    SB_WRITE_RESET(BasePort, 0x00);

    // Check if it's OK to receive (some cards will ignore the above reset
    // command and so will not issue an ACK, so time out is NOT an error)
    DbgPrint("Waiting for an ACK\n");
    if (WaitToReceive(BasePort))
    {
        // Check to make sure the reset was acknowledged:
        for (TimeOut = SB_TIMEOUT;
             Status = (SB_READ_DATA(BasePort) != SB_DSP_READY) && TimeOut > 0;
             TimeOut --);
    }

    DbgPrint("Querying DSP version\n");
    if (! WaitToSend(BasePort))
        return FALSE;

    SB_WRITE_DATA(BasePort, SB_GET_DSP_VERSION);

    if (! WaitToReceive(BasePort))
        return FALSE;

    DSP_Major = SB_READ_DATA(BasePort);
    DSP_Minor = SB_READ_DATA(BasePort);

    DbgPrint("DSP v%d.%d\n", DSP_Major, DSP_Minor);

    DbgPrint("Sound card initialized!\n");
    
    return (DSP_Major * 256) + DSP_Minor;
}
