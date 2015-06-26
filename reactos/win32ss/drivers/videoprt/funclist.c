/*
 * VideoPort driver
 *
 * Copyright (C) 2007 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "videoprt.h"

#define NDEBUG
#include <debug.h>

typedef struct _VIDEO_PORT_FUNCTION_TABLE {
    PVOID Address;
    PCSZ Name;
} *PVIDEO_PORT_FUNCTION_TABLE, VIDEO_PORT_FUNCTION_TABLE;

/* GLOBAL VARIABLES ***********************************************************/

#define VP_EXPORTED_FUNCS (sizeof(VideoPortExports) / sizeof(*VideoPortExports))

/* Create an array of entries with pfn, psz, for IntVideoPortGetProcAddress */
#define MAKE_ENTRY(FUNCTIONNAME) { FUNCTIONNAME, #FUNCTIONNAME }
const VIDEO_PORT_FUNCTION_TABLE VideoPortExports[] = {
    MAKE_ENTRY(VideoPortQueueDpc),
    MAKE_ENTRY(VideoPortAllocatePool),
    MAKE_ENTRY(VideoPortFreePool),
    MAKE_ENTRY(VideoPortReleaseCommonBuffer),
    MAKE_ENTRY(VideoPortAllocateCommonBuffer),
    MAKE_ENTRY(VideoPortCreateSecondaryDisplay),
    MAKE_ENTRY(VideoPortGetDmaAdapter),
    MAKE_ENTRY(VideoPortGetVersion),
    MAKE_ENTRY(VideoPortLockBuffer),
    MAKE_ENTRY(VideoPortUnlockBuffer),
    MAKE_ENTRY(VideoPortSetEvent),
    MAKE_ENTRY(VideoPortClearEvent),
    MAKE_ENTRY(VideoPortReadStateEvent),
    MAKE_ENTRY(VideoPortRegisterBugcheckCallback),
    MAKE_ENTRY(VideoPortCreateEvent),
    MAKE_ENTRY(VideoPortDeleteEvent),
    MAKE_ENTRY(VideoPortWaitForSingleObject),
    MAKE_ENTRY(VideoPortCheckForDeviceExistence),
    MAKE_ENTRY(VideoPortFlushRegistry),
    MAKE_ENTRY(VideoPortQueryPerformanceCounter),
};
#undef MAKE_ENTRY

PVOID NTAPI
IntVideoPortGetProcAddress(
   IN PVOID HwDeviceExtension,
   IN PUCHAR FunctionName)
{
    ULONG i = 0;

    TRACE_(VIDEOPRT, "VideoPortGetProcAddress(%s)\n", FunctionName);

   /* Search by name */

    for (i = 0; i < VP_EXPORTED_FUNCS; i++)
    {
        if (!strcmp((PCHAR)FunctionName, VideoPortExports[i].Name))
        {
            return (PVOID)VideoPortExports[i].Address;
        }
    }

   ERR_(VIDEOPRT, "VideoPortGetProcAddress: Can't resolve symbol %s\n", FunctionName);

   return NULL;
}
