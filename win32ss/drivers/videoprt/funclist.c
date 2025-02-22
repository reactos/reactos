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

/* Create an array of entries with pfn, psz, for IntVideoPortGetProcAddress */
#define MAKE_ENTRY(FUNCTIONNAME) { FUNCTIONNAME, #FUNCTIONNAME }
const VIDEO_PORT_FUNCTION_TABLE VideoPortExports[] = {
    MAKE_ENTRY(VideoPortDDCMonitorHelper),
    MAKE_ENTRY(VideoPortDoDma),
    MAKE_ENTRY(VideoPortGetCommonBuffer),
    MAKE_ENTRY(VideoPortGetMdl),
    MAKE_ENTRY(VideoPortLockPages),
    MAKE_ENTRY(VideoPortSignalDmaComplete),
    MAKE_ENTRY(VideoPortUnlockPages),
    MAKE_ENTRY(VideoPortAssociateEventsWithDmaHandle),
    MAKE_ENTRY(VideoPortGetBytesUsed),
    MAKE_ENTRY(VideoPortSetBytesUsed),
    MAKE_ENTRY(VideoPortGetDmaContext),
    MAKE_ENTRY(VideoPortSetDmaContext),
    MAKE_ENTRY(VideoPortMapDmaMemory),
    MAKE_ENTRY(VideoPortUnmapDmaMemory),
    MAKE_ENTRY(VideoPortGetAgpServices),
    MAKE_ENTRY(VideoPortAllocateContiguousMemory),
    MAKE_ENTRY(VideoPortGetRomImage),
    MAKE_ENTRY(VideoPortGetAssociatedDeviceExtension),
    MAKE_ENTRY(VideoPortGetAssociatedDeviceID),
    MAKE_ENTRY(VideoPortAcquireDeviceLock),
    MAKE_ENTRY(VideoPortReleaseDeviceLock),
    MAKE_ENTRY(VideoPortAllocateBuffer),
    MAKE_ENTRY(VideoPortFreeCommonBuffer),
    MAKE_ENTRY(VideoPortReleaseBuffer),
    MAKE_ENTRY(VideoPortInterlockedIncrement),
    MAKE_ENTRY(VideoPortInterlockedDecrement),
    MAKE_ENTRY(VideoPortInterlockedExchange),
    MAKE_ENTRY(VideoPortGetVgaStatus),
    MAKE_ENTRY(VideoPortQueueDpc),
    MAKE_ENTRY(VideoPortEnumerateChildren),
    MAKE_ENTRY(VideoPortQueryServices),
    MAKE_ENTRY(VideoPortGetDmaAdapter),
    MAKE_ENTRY(VideoPortPutDmaAdapter),
    MAKE_ENTRY(VideoPortAllocateCommonBuffer),
    MAKE_ENTRY(VideoPortReleaseCommonBuffer),
    MAKE_ENTRY(VideoPortLockBuffer),
    MAKE_ENTRY(VideoPortUnlockBuffer),
    MAKE_ENTRY(VideoPortStartDma),
    MAKE_ENTRY(VideoPortCompleteDma),
    MAKE_ENTRY(VideoPortCreateEvent),
    MAKE_ENTRY(VideoPortDeleteEvent),
    MAKE_ENTRY(VideoPortSetEvent),
    MAKE_ENTRY(VideoPortClearEvent),
    MAKE_ENTRY(VideoPortReadStateEvent),
    MAKE_ENTRY(VideoPortWaitForSingleObject),
    MAKE_ENTRY(VideoPortAllocatePool),
    MAKE_ENTRY(VideoPortFreePool),
    MAKE_ENTRY(VideoPortCreateSpinLock),
    MAKE_ENTRY(VideoPortDeleteSpinLock),
    MAKE_ENTRY(VideoPortAcquireSpinLock),
    MAKE_ENTRY(VideoPortAcquireSpinLockAtDpcLevel),
    MAKE_ENTRY(VideoPortReleaseSpinLock),
    MAKE_ENTRY(VideoPortReleaseSpinLockFromDpcLevel),
    MAKE_ENTRY(VideoPortCheckForDeviceExistence),
    MAKE_ENTRY(VideoPortCreateSecondaryDisplay),
    MAKE_ENTRY(VideoPortFlushRegistry),
    MAKE_ENTRY(VideoPortQueryPerformanceCounter),
    MAKE_ENTRY(VideoPortGetVersion),
    MAKE_ENTRY(VideoPortRegisterBugcheckCallback),
};
#undef MAKE_ENTRY

PVOID NTAPI
IntVideoPortGetProcAddress(
    IN PVOID HwDeviceExtension,
    IN PUCHAR FunctionName)
{
    ULONG i;

    TRACE_(VIDEOPRT, "VideoPortGetProcAddress(%s)\n", FunctionName);

   /* Search by name */
    for (i = 0; i < ARRAYSIZE(VideoPortExports); i++)
    {
        if (!strcmp((PCHAR)FunctionName, VideoPortExports[i].Name))
        {
            return (PVOID)VideoPortExports[i].Address;
        }
    }

    ERR_(VIDEOPRT, "VideoPortGetProcAddress: Can't resolve symbol %s\n", FunctionName);

    return NULL;
}
