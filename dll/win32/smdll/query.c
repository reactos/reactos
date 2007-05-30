/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/smdll/query.c
 * PURPOSE:         Call SM API SM_API_QUERY_INFORMATION (not in NT)
 */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <sm/helper.h>

#define NDEBUG
#include <debug.h>


/**********************************************************************
 * NAME							EXPORTED
 *	SmQueryInformation/5
 *
 * DESCRIPTION
 *	Ask the SM to collect some data from its internal data
 *	structures and send it back.
 *
 * ARGUMENTS
 *	hSmApiPort: handle returned by SmConnectApiPort;
 *	SmInformationClass: an SM information class ID:
 *		SM_BASIC_INFORMATION: the number of registered subsystems
 *	Data: pointer to storage for the information to request;
 *	DataLength: length in bytes of the Data buffer; it must be
 *		set and must match the SmInformationClass info size;
 *	ReturnedDataLength: optional pointer to storage to receive
 *		the size of the returnede data.
 *	
 * RETURN VALUE
 * 	STATUS_SUCCESS: OK you get what you asked for;
 * 	STATUS_INFO_LENGTH_MISMATCH: you set DataLength to 0 or to a
 * 		value that does not match whet the SmInformationClass
 * 		requires;
 * 	STATUS_INVALID_PARAMETER_2: bad information class;
 * 	A port error.
 * 	
 */
NTSTATUS STDCALL
SmQueryInformation (IN      HANDLE                hSmApiPort,
		    IN      SM_INFORMATION_CLASS  SmInformationClass,
		    IN OUT  PVOID                 Data,
		    IN      ULONG                 DataLength,
		    IN OUT  PULONG                ReturnedDataLength OPTIONAL)
{
	NTSTATUS         Status = STATUS_SUCCESS;
	SM_PORT_MESSAGE  SmReqMsg;


	if(0 == DataLength)
	{
		return STATUS_INFO_LENGTH_MISMATCH;
	}
	/* Marshal data in the port message */
	switch (SmInformationClass)
	{
		case SmBasicInformation:
			if(DataLength != sizeof (SM_BASIC_INFORMATION))
			{
				return STATUS_INFO_LENGTH_MISMATCH;
			}
			SmReqMsg.Request.QryInfo.SmInformationClass = SmBasicInformation;
			SmReqMsg.Request.QryInfo.DataLength = DataLength;
			SmReqMsg.Request.QryInfo.BasicInformation.SubSystemCount = 0;
			break;
		case SmSubSystemInformation:
			if(DataLength != sizeof (SM_SUBSYSTEM_INFORMATION))
			{
				return STATUS_INFO_LENGTH_MISMATCH;
			}
			SmReqMsg.Request.QryInfo.SmInformationClass = SmSubSystemInformation;
			SmReqMsg.Request.QryInfo.DataLength = DataLength;
			SmReqMsg.Request.QryInfo.SubSystemInformation.SubSystemId =
				((PSM_SUBSYSTEM_INFORMATION)Data)->SubSystemId;
			break;
		default:
			return STATUS_INVALID_PARAMETER_2;
	}
	/* SM API to invoke */
	SmReqMsg.SmHeader.ApiIndex = SM_API_QUERY_INFORMATION;

	/* Prepare the port request message */
	SmReqMsg.Header.u2.s2.Type = LPC_NEW_MESSAGE;
	SmReqMsg.Header.u1.s1.DataLength    = SM_PORT_DATA_SIZE(SmReqMsg.Request);
	SmReqMsg.Header.u1.s1.TotalLength = SM_PORT_MESSAGE_SIZE;
	Status = NtRequestWaitReplyPort (hSmApiPort, (PPORT_MESSAGE) & SmReqMsg, (PPORT_MESSAGE) & SmReqMsg);
	if (NT_SUCCESS(Status))
	{
		/* Unmarshal data */
		RtlCopyMemory (Data, & SmReqMsg.Reply.QryInfo.BasicInformation, SmReqMsg.Reply.QryInfo.DataLength);
		/* Use caller provided storage to store data size */
		if(NULL != ReturnedDataLength)
		{
			*ReturnedDataLength = SmReqMsg.Reply.QryInfo.DataLength;
		}
		return SmReqMsg.SmHeader.Status;
	}
	DPRINT("SMLIB: %s failed (Status=0x%08lx)\n", __FUNCTION__, Status);
	return Status;
}
/* EOF */
