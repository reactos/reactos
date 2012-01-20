/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/smapicomp.c
 * PURPOSE:         SM_API_COMPLETE_SESSION.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>


/**********************************************************************
 * SmCompSes/1							API
 */
SMAPI(SmCompSes)
{
	NTSTATUS                  Status = STATUS_SUCCESS;

	DPRINT("SM: %s called\n", __FUNCTION__);

	DPRINT("SM: %s: ClientId.UniqueProcess=%p\n",
		__FUNCTION__, Request->Header.ClientId.UniqueProcess);
	Status = SmCompleteClientInitialization (PtrToUlong(Request->Header.ClientId.UniqueProcess));
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("SM: %s: SmCompleteClientInitialization failed (Status=0x%08lx)\n",
			__FUNCTION__, Status);
	}
	Request->SmHeader.Status = Status;
	return STATUS_SUCCESS;
}


/* EOF */
