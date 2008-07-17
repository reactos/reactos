/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/mme/DriverProc.c
 *
 * PURPOSE:     Provides a default/boilerplate implementation of the DriverProc
 *              export function. (NOTE: This file may be fairly redundant
 *              because the MME API can provide default functionality anyway.)
 *              This file will be removed from future revisions.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmddk.h>
#include <ntddsnd.h>

#include <mmebuddy.h>

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
            TRACE_("DRV_LOAD\n");
            return 1L;

        case DRV_FREE :
            TRACE_("DRV_FREE\n");
            return 1L;

        case DRV_OPEN :
            TRACE_("DRV_OPEN\n");
            return 1L;

        case DRV_CLOSE :
            TRACE_("DRV_CLOSE\n");
            return 1L;

        case DRV_ENABLE :
            TRACE_("DRV_ENABLE\n");
            return 1L;

        case DRV_DISABLE :
            TRACE_("DRV_DISABLE\n");
            return 1L;

        /*
            We don't provide configuration capabilities. This used to be
            for things like I/O port, IRQ, DMA settings, etc.
        */

        case DRV_QUERYCONFIGURE :
            TRACE_("DRV_QUERYCONFIGURE\n");
            return 0L;

        case DRV_CONFIGURE :
            TRACE_("DRV_CONFIGURE\n");
            return 0L;

        case DRV_INSTALL :
            TRACE_("DRV_INSTALL\n");
            return DRVCNF_RESTART;
    };

    return DefDriverProc(driver_id,
                         driver_handle,
                         message,
                         parameter1,
                         parameter2);
}
