/*
    ReactOS Sound System
    MME Interface

    Purpose:
        Default DriverProc implementation

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmddk.h>
#include <ntddsnd.h>
#include <debug.h>

LONG
DefaultDriverProc(
    DWORD driver_id,
    HANDLE driver_handle,
    UINT message,
    LONG parameter1,
    LONG parameter2)
{
    switch ( message )
    {
        case DRV_LOAD :
            DPRINT("DRV_LOAD\n");
            return 1L;

        case DRV_FREE :
            DPRINT("DRV_FREE\n");
            return 1L;

        case DRV_OPEN :
            DPRINT("DRV_OPEN\n");
            return 1L;

        case DRV_CLOSE :
            DPRINT("DRV_CLOSE\n");
            return 1L;

        case DRV_ENABLE :
            DPRINT("DRV_ENABLE\n");
            return 1L;

        case DRV_DISABLE :
            DPRINT("DRV_DISABLE\n");
            return 1L;

        /*
            We don't provide configuration capabilities. This used to be
            for things like I/O port, IRQ, DMA settings, etc.
        */

        case DRV_QUERYCONFIGURE :
            DPRINT("DRV_QUERYCONFIGURE\n");
            return 0L;

        case DRV_CONFIGURE :
            DPRINT("DRV_CONFIGURE\n");
            return 0L;

        case DRV_INSTALL :
            DPRINT("DRV_INSTALL\n");
            return DRVCNF_RESTART;
    };

    return DefDriverProc(driver_id,
                         driver_handle,
                         message,
                         parameter1,
                         parameter2);
}
