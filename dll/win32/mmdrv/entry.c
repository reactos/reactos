/*
    ReactOS Sound System
    Default MME Driver

    Purpose:
        MME driver entry-point

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmddk.h>
#include <mmebuddy.h>
#include <debug.h>

APIENTRY LONG
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

        default :
            return DefaultDriverProc(driver_id,
                                     driver_handle,
                                     message,
                                     parameter1,
                                     parameter2);
    }
}
