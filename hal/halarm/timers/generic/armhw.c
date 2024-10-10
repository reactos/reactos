/*
 * PROJECT:     ARM Generic Timer Implementation
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Does cool things like Memory Management
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include "timer.h"
#include "generictimer.h"
#define NDEBUG
#include <debug.h>

UINT32
ArmGenericTimerGetTimerFreq (
  VOID
  )
{
  return ArmReadCntFrq ();
}

UINT64
ArmGenericTimerGetSystemCount(VOID)
{
  return ArmReadCntPct ();
}
