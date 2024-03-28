/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager volumes and Device Object Power Extension (DOPE) support routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KGUARDED_MUTEX PopVolumeLock;
KSPIN_LOCK PopDopeGlobalLock;
LIST_ENTRY PopVolumeDevices;

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
PoInitializeDeviceObject(
    _Inout_ PDEVOBJ_EXTENSION DeviceObjectExtension)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

VOID
NTAPI
PoRemoveVolumeDevice(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

VOID
NTAPI
PoVolumeDevice(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

/* EOF */
