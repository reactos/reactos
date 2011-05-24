/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/delete.c
 * PURPOSE:         Free rossym info
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#define NTOSAPI
#include <ntddk.h>
#include <reactos/rossym.h>
#include <ntimage.h>

#define NDEBUG
#include <debug.h>

#include "rossympriv.h"
#include "pe.h"
#include "dwarf.h"

VOID
RosSymDelete(PROSSYM_INFO RosSymInfo)
{
	dwarfclose(RosSymInfo);
}

/* EOF */
