/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 dll/win32/mmdrv/entry.c
 * PURPOSE:              Multimedia User Mode Driver (DriverProc)
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Jan 14, 2007: Created
 */

#include <mmdrv.h>


/*
    Nothing particularly special happens here.

    Back in the days of Windows 3.1, we would do something more useful here,
    as this is effectively the old-style equivalent of NT's "DriverEntry",
    though far more primitive.

    In summary, we just implement to satisfy the MME API (winmm) requirements.
*/

LONG
DriverProc(
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
