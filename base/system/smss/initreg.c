/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/initreg.c
 * PURPOSE:         Hive loading.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>

#if 0
NTSTATUS
SmInitializeRegistry(VOID)
{
  DPRINT("SM: %s: initializing registry\n", __FUNCTION__);

  /* Load remaining registry hives */
  return NtInitializeRegistry(CM_BOOT_FLAG_SMSS);
}
#endif

/* EOF */
