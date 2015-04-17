/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ps2.h
 * PURPOSE:         PS/2 controller emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _PS2_H_
#define _PS2_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

#define PS2_DATA_PORT       0x60
#define PS2_CONTROL_PORT    0x64

typedef VOID (WINAPI *PS2_DEVICE_CMDPROC)(LPVOID Param, BYTE Command);

/* FUNCTIONS ******************************************************************/

VOID PS2SetDeviceCmdProc(BYTE PS2Port, LPVOID Param, PS2_DEVICE_CMDPROC DeviceCommand);

BOOLEAN PS2QueuePush(BYTE PS2Port, BYTE Data);
BOOLEAN PS2PortQueueRead(BYTE PS2Port);

BOOLEAN PS2Initialize(VOID);
VOID PS2Cleanup(VOID);

#endif // _PS2_H_

/* EOF */
