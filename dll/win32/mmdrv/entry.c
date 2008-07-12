/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/mmdrv/entry.c
 *
 * PURPOSE:     MME generic low-level audio device support library DriverProc
 *              entry-point.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmddk.h>

#include <mmebuddy.h>

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
            TRACE_("DRV_LOAD\n");
            return 1L;

        case DRV_FREE :
            TRACE_("DRV_FREE\n");
            return 1L;

        default :
            return DefaultDriverProc(driver_id,
                                     driver_handle,
                                     message,
                                     parameter1,
                                     parameter2);
    }
}
