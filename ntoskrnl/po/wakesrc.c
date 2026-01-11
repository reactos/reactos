/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager system wake source management
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

LIST_ENTRY PopWakeSourceDevicesList;
ULONG PopSystemFullWake;
#if 0
KSEMAPHORE PopWakeSourceResetSemaphore;
KEVENT PopWakeSourceResetComplete;
#endif

/* PUBLIC FUNCTIONS ***********************************************************/

/* EOF */
