/* $Id: $
 *
 * smapiexec.c - SM_API_EXECUTE_PROGRAM
 *
 * Reactos Session Manager
 *
 */

#include "smss.h"
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

/**********************************************************************
 * SmCreateUserProcess/5
 *
 */
NTSTATUS STDCALL
SmCreateUserProcess (LPWSTR ImagePath,
		     LPWSTR CommandLine,
		     BOOLEAN WaitForIt,
		     PLARGE_INTEGER Timeout OPTIONAL,
		     BOOLEAN TerminateIt,
		     PRTL_PROCESS_INFO UserProcessInfo OPTIONAL
		     )
{
	UNICODE_STRING			ImagePathString = {0};
	UNICODE_STRING			CommandLineString = {0};
	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters = NULL;
	RTL_PROCESS_INFO		ProcessInfo = {0};
	PRTL_PROCESS_INFO		pProcessInfo = & ProcessInfo;
	NTSTATUS			Status = STATUS_SUCCESS;


	DPRINT("SM: %s called\n",__FUNCTION__);

	RtlInitUnicodeString (& ImagePathString, ImagePath);
	RtlInitUnicodeString (& CommandLineString, CommandLine);

	RtlCreateProcessParameters(& ProcessParameters,
				   & ImagePathString,
				   NULL,
				   NULL,
				   & CommandLineString,
				   SmSystemEnvironment,
				   NULL,
				   NULL,
				   NULL,
				   NULL);

	if(NULL != UserProcessInfo)
	{
		/* Use caller provided storage */
		pProcessInfo = UserProcessInfo;
	}

	Status = RtlCreateUserProcess (& ImagePathString,
				       OBJ_CASE_INSENSITIVE,
				       ProcessParameters,
				       NULL,
				       NULL,
				       NULL,
				       FALSE,
				       NULL,
				       NULL,
				       pProcessInfo);
	if (!NT_SUCCESS(Status))
	{
		CHAR AnsiBuffer [MAX_PATH];
		INT i = 0;
		for(i=0;ImagePathString.Buffer[i];i++)
		{
			/* raw U -> A */
			AnsiBuffer [i] = (CHAR) (ImagePathString.Buffer[i] & 0xff);
		}

		DPRINT1("SM: %s: Running \"%s\" failed (Status=0x%08lx)\n",
			AnsiBuffer, __FUNCTION__, Status);
		return Status;
	}

	RtlDestroyProcessParameters (ProcessParameters);

	/* Wait for process termination */
	if(WaitForIt)
	{
		NtWaitForSingleObject (pProcessInfo->ProcessHandle,
				       FALSE,
				       Timeout);
	}

	/* Terminate process */
	if(TerminateIt)
	{
		NtClose(pProcessInfo->ThreadHandle);
		NtClose(pProcessInfo->ProcessHandle);
	}
	return STATUS_SUCCESS;
}

/**********************************************************************
 * SmExecPgm/1							API
 */
SMAPI(SmExecPgm)
{
	DPRINT("SM: %s called\n",__FUNCTION__);
	Request->Status = STATUS_NOT_IMPLEMENTED;
	return STATUS_SUCCESS;
}


/* EOF */
