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
	int i;
	for (i = 0; i < RosSymInfo->pe->nsections; i++) {
		RtlFreeAnsiString(ANSI_NAME_STRING(&RosSymInfo->pe->sect[i]));
	}
	RosSymFreeMem(RosSymInfo->pe->sect);
	dwarfclose(RosSymInfo);
}

/* EOF */
