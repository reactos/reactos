/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/smapiquery.c
 * PURPOSE:         SM_API_QUERY_INFORMATION.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>


/**********************************************************************
 * SmQryInfo/1							API
 */
SMAPI(SmQryInfo)
{
	NTSTATUS Status = STATUS_SUCCESS;

	DPRINT("SM: %s called\n", __FUNCTION__);

	switch (Request->Request.QryInfo.SmInformationClass)
	{
	case SmBasicInformation:
		if(Request->Request.QryInfo.DataLength != sizeof (SM_BASIC_INFORMATION))
		{
			Request->Reply.QryInfo.DataLength = sizeof (SM_BASIC_INFORMATION);
			Request->SmHeader.Status = STATUS_INFO_LENGTH_MISMATCH;
		}else{
			Request->SmHeader.Status =
				SmGetClientBasicInformation (& Request->Reply.QryInfo.BasicInformation);
		}
		break;
	case SmSubSystemInformation:
		if(Request->Request.QryInfo.DataLength != sizeof (SM_SUBSYSTEM_INFORMATION))
		{
			Request->Reply.QryInfo.DataLength = sizeof (SM_SUBSYSTEM_INFORMATION);
			Request->SmHeader.Status = STATUS_INFO_LENGTH_MISMATCH;
		}else{
			Request->SmHeader.Status =
				SmGetSubSystemInformation (& Request->Reply.QryInfo.SubSystemInformation);
		}
		break;
	default:
		Request->SmHeader.Status = STATUS_NOT_IMPLEMENTED;
		break;
	}
	return Status;
}


/* EOF */
