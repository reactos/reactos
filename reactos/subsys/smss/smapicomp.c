/* $Id: $
 *
 * smapicomp.c - SM_API_COMPLETE_SESSION
 *
 * Reactos Session Manager
 *
 */

#include "smss.h"
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>


/**********************************************************************
 * SmCompSes/1							API
 */
SMAPI(SmCompSes)
{
	DPRINT("SM: %s called\n",__FUNCTION__);
	Request->Status = STATUS_NOT_IMPLEMENTED;
	return STATUS_SUCCESS;
}


/* EOF */
