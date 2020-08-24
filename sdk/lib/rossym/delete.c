/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * PURPOSE:         Free rossym info
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#define NTOSAPI
#include <ntdef.h>
#include <reactos/rossym.h>
#include "rossympriv.h"

VOID
RosSymDelete(PROSSYM_INFO RosSymInfo)
{
  RosSymFreeMem(RosSymInfo);
}
