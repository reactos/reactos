/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            himem.c
 * PURPOSE:         DOS XMS Driver
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "cpu/bop.h"

#include "dos.h"
#include "dos/dem.h"
#include "device.h"

#define XMS_DEVICE_NAME "XMSXXXX0"
#define XMS_BOP 0x52

/* PRIVATE VARIABLES **********************************************************/

static const BYTE EntryProcedure[] = {
    0xEB, // jmp short +0x03
    0x03,
    0x90, // nop
    0x90, // nop
    0x90, // nop
    LOBYTE(EMULATOR_BOP),
    HIBYTE(EMULATOR_BOP),
    XMS_BOP,
    0xCB // retf
};

static PDOS_DEVICE_NODE Node = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID WINAPI XmsBopProcedure(LPWORD Stack)
{
    switch (getAH())
    {
        /* Get XMS Version */
        case 0x00:
        {
            setAX(0x0300); /* XMS version 3.0 */
            setDX(0x0001); /* HMA present */

            break;
        }

        default:
        {
            DPRINT1("XMS command AH = 0x%02X NOT IMPLEMENTED\n", getAH());
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN XmsGetDriverEntry(PDWORD Pointer)
{
    if (Node == NULL) return FALSE;
    *Pointer = DEVICE_PRIVATE_AREA(Node->Driver);
    return TRUE;
}

VOID XmsInitialize(VOID)
{
    Node = DosCreateDeviceEx(DOS_DEVATTR_IOCTL | DOS_DEVATTR_CHARACTER,
                             XMS_DEVICE_NAME,
                             sizeof(EntryProcedure));

    RegisterBop(XMS_BOP, XmsBopProcedure);

    /* Copy the entry routine to the device private area */
    RtlMoveMemory(FAR_POINTER(DEVICE_PRIVATE_AREA(Node->Driver)),
                  EntryProcedure,
                  sizeof(EntryProcedure));
}

VOID XmsCleanup(VOID)
{
    RegisterBop(XMS_BOP, NULL);
    DosDeleteDevice(Node);
}
