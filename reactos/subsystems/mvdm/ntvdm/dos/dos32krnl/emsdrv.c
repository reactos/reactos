/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            emsdrv.c
 * PURPOSE:         DOS EMS Driver
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"

#include "dos.h"
#include "dos/dem.h"
#include "device.h"

#define EMS_DEVICE_NAME "EMMXXXX0"

/* PRIVATE VARIABLES **********************************************************/

static PDOS_DEVICE_NODE Node;

/* PRIVATE FUNCTIONS **********************************************************/

WORD NTAPI EmsDrvDispatchIoctlRead(PDOS_DEVICE_NODE Device, DWORD Buffer, PWORD Length)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return DOS_DEVSTAT_DONE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID EmsDrvInitialize(VOID)
{
    /* Create the device */
    Node = DosCreateDevice(DOS_DEVATTR_IOCTL
                           | DOS_DEVATTR_CHARACTER,
                           EMS_DEVICE_NAME);
    Node->IoctlReadRoutine = EmsDrvDispatchIoctlRead;
}

VOID EmsDrvCleanup(VOID)
{
    /* Delete the device */
    DosDeleteDevice(Node);
}
