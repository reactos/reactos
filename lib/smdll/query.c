/* $Id: compses.c 13731 2005-02-23 23:37:06Z ea $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/smdll/query.c
 * PURPOSE:         Call SM API SM_API_QUERY (not in NT)
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/api.h>
#include <sm/helper.h>

#define NDEBUG
#include <debug.h>

/**********************************************************************
 * NAME							EXPORTED
 *	SmQuery/4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
NTSTATUS STDCALL
SmQuery (IN      HANDLE                SmApiPort,
	 IN      SM_INFORMATION_CLASS  SmInformationClass,
	 IN OUT  PVOID                 Data,
	 IN OUT  PULONG                DataLength)
{
	/* TODO */
	if(NULL != DataLength)
	{
		*DataLength = 0;
	}
	return STATUS_SUCCESS;	
}
/* EOF */
