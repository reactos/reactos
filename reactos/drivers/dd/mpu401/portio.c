/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 services/dd/mpu401/portio.c (see also mpu401.h)
 * PURPOSE:              MPU-401 MIDI port I/O helper
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Sept 26, 2003: Created
 */

#include <windows.h>
#include <ddk/ntddk.h>
#include "mpu401.h"


BOOLEAN WaitToSend(UINT BasePort)
{
    int TimeOut;
    
    DbgPrint("WaitToSend ");

    // Check if it's OK to send
    for (TimeOut = MPU401_TIMEOUT;
         ! MPU401_READY_TO_SEND(BasePort) && TimeOut > 0;
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

    DbgPrint("WaitToSend ");

    // Check if it's OK to receive
    for (TimeOut = MPU401_TIMEOUT;
         ! MPU401_READY_TO_RECEIVE(BasePort) && TimeOut > 0;
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


BOOLEAN InitUARTMode(UINT BasePort)
{
    UINT TimeOut;
    UCHAR Status = 0;

    DbgPrint("InitUARTMode() called\n");

    // Check if it's OK to send
    if (! WaitToSend(BasePort))
        return FALSE;

    DbgPrint("Resetting MPU401\n");

    // Send an MPU reset:
    MPU401_WRITE_COMMAND(BasePort, 0xff);

    // Check if it's OK to receive (some cards will ignore the above reset
    // command and so will not issue an ACK, so time out is NOT an error)
    DbgPrint("Waiting for an ACK\n");
    if (WaitToReceive(BasePort))
    {
        // Check to make sure the reset was acknowledged:
        for (TimeOut = MPU401_TIMEOUT;
             Status = (MPU401_READ_DATA(BasePort) & 0xfe) && TimeOut > 0;
             TimeOut --);
    }

    DbgPrint("Entering UART mode\n");
    // Now we kick the MPU401 into UART ("dumb") mode
    MPU401_WRITE_COMMAND(BasePort, 0x3f);

    return TRUE;
}
