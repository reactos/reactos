/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/smapiexec.c
 * PURPOSE:         SM_API_EXECUTE_PROGRAM.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>

static const WCHAR szSystemDirectory[] = L"\\System32";

/**********************************************************************
 * SmCreateUserProcess/5
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	ImagePath: absolute path of the image to run;
 *	CommandLine: arguments and options for ImagePath;
 *	Flags: Wait flag: Set for boot time processes and unset for
 *			subsystems bootstrapping;
 *		1Mb reserve flag: Set for subsystems, unset for everything
*			else
 *	Timeout: optional: used if WaitForIt==TRUE;
 *	ProcessHandle: optional: a duplicated handle for
 		the child process (storage provided by the caller).
 *
 * RETURN VALUE
 * 	NTSTATUS:
 *
 */
NTSTATUS STDCALL
SmCreateUserProcess (LPWSTR ImagePath,
		     LPWSTR CommandLine,
		     ULONG Flags,
		     PLARGE_INTEGER Timeout OPTIONAL,
		     PRTL_USER_PROCESS_INFORMATION UserProcessInfo OPTIONAL)
{
	UNICODE_STRING			ImagePathString = {0};
	UNICODE_STRING			CommandLineString = {0};
        UNICODE_STRING			SystemDirectory = {0};
	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters = NULL;
	RTL_USER_PROCESS_INFORMATION		ProcessInfo = {0};
	PRTL_USER_PROCESS_INFORMATION		pProcessInfo = & ProcessInfo;
	NTSTATUS			Status = STATUS_SUCCESS;

	DPRINT("SM: %s called\n", __FUNCTION__);

	if (NULL != UserProcessInfo)
	{
		pProcessInfo = UserProcessInfo;
	}

	RtlInitUnicodeString (& ImagePathString, ImagePath);
	RtlInitUnicodeString (& CommandLineString, CommandLine);

	SystemDirectory.MaximumLength = (wcslen(SharedUserData->NtSystemRoot) * sizeof(WCHAR)) + sizeof(szSystemDirectory);
	SystemDirectory.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
						 0,
						 SystemDirectory.MaximumLength);
	if (SystemDirectory.Buffer == NULL)
	{
		Status = STATUS_NO_MEMORY;
		DPRINT1("SM: %s: Allocating system directory string failed (Status=0x%08lx)\n",
			__FUNCTION__, Status);
		return Status;
	}

	Status = RtlAppendUnicodeToString(& SystemDirectory,
					  SharedUserData->NtSystemRoot);
	if (!NT_SUCCESS(Status))
	{
		goto FailProcParams;
	}

	Status = RtlAppendUnicodeToString(& SystemDirectory,
					  szSystemDirectory);
	if (!NT_SUCCESS(Status))
	{
		goto FailProcParams;
	}


	Status = RtlCreateProcessParameters(& ProcessParameters,
					    & ImagePathString,
					    NULL,
					    & SystemDirectory,
					    & CommandLineString,
					    SmSystemEnvironment,
					    NULL,
					    NULL,
					    NULL,
					    NULL);

	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    SystemDirectory.Buffer);

	if (!NT_SUCCESS(Status))
	{
FailProcParams:
		DPRINT1("SM: %s: Creating process parameters failed (Status=0x%08lx)\n",
			__FUNCTION__, Status);
		return Status;
	}
#ifdef ROS_DOESNT_SUCK
	/* Reserve lower 1Mb, if requested */
	if (Flags & SM_CREATE_FLAG_RESERVE_1MB)
		ProcessParameters->Flags |= RTL_USER_PROCESS_PARAMETERS_RESERVE_1MB;
#endif
	/* Create the user process */
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

	RtlDestroyProcessParameters (ProcessParameters);

	if (!NT_SUCCESS(Status))
	{
		DPRINT1("SM: %s: Running \"%S\" failed (Status=0x%08lx)\n",
			__FUNCTION__, ImagePathString.Buffer, Status);
		return Status;
	}
	/*
	 * It the caller is *not* interested in the child info,
	 * resume it immediately.
	 */
	if (NULL == UserProcessInfo)
	{
		Status = NtResumeThread (ProcessInfo.ThreadHandle, NULL);
		if(!NT_SUCCESS(Status))
		{
			DPRINT1("SM: %s: NtResumeThread failed (Status=0x%08lx)\n",
				__FUNCTION__, Status);
		}
	}

	/* Wait for process termination */
	if (Flags & SM_CREATE_FLAG_WAIT)
	{
		Status = NtWaitForSingleObject (pProcessInfo->ProcessHandle,
						FALSE,
						Timeout);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("SM: %s: NtWaitForSingleObject failed with Status=0x%08lx\n",
				__FUNCTION__, Status);
		}
	}

    if (NULL == UserProcessInfo)
    {
        NtClose(pProcessInfo->ProcessHandle);
        NtClose(pProcessInfo->ThreadHandle);
    }
	return Status;
}


/**********************************************************************
 * SmExecPgm/1							API
 */
SMAPI(SmExecPgm)
{
	PSM_PORT_MESSAGE_EXECPGM ExecPgm = NULL;
	WCHAR                    Name [SM_EXEXPGM_MAX_LENGTH + 1];
	NTSTATUS                 Status = STATUS_SUCCESS;

	DPRINT("SM: %s called\n",__FUNCTION__);

	if(NULL == Request)
	{
		DPRINT1("SM: %s: Request == NULL!\n", __FUNCTION__);
		return STATUS_INVALID_PARAMETER;
	}
	DPRINT("SM: %s called from CID(%p|%p)\n",
		__FUNCTION__, Request->Header.ClientId.UniqueProcess,
		Request->Header.ClientId.UniqueThread);
	ExecPgm = & Request->Request.ExecPgm;
	/* Check if the name lenght is valid */
	if((ExecPgm->NameLength > 0) &&
	   (ExecPgm->NameLength <= SM_EXEXPGM_MAX_LENGTH) &&
	   TRUE /* TODO: check LPC payload size */)
	{
		WCHAR Data [MAX_PATH + 1] = {0};
		ULONG DataLength = sizeof Data;
		ULONG DataType = REG_EXPAND_SZ;


		RtlZeroMemory (Name, sizeof Name);
		RtlCopyMemory (Name,
			       ExecPgm->Name,
			       (sizeof ExecPgm->Name[0] * ExecPgm->NameLength));
		DPRINT("SM: %s: Name='%S'\n", __FUNCTION__, Name);
		/* Lookup Name in the registry */
		Status = SmLookupSubsystem (Name,
					    Data,
					    & DataLength,
					    & DataType,
					    SmSystemEnvironment /* expand */);
		if(NT_SUCCESS(Status))
		{
			/* Is the subsystem definition non-empty? */
			if (DataLength > sizeof Data[0])
			{
				WCHAR ImagePath [MAX_PATH + 1] = {0};
				PWCHAR CommandLine = ImagePath;
				RTL_USER_PROCESS_INFORMATION ProcessInfo = {0};

				wcscpy (ImagePath, L"\\??\\");
				wcscat (ImagePath, Data);
				/*
				 * Look for the beginning of the command line.
				 */
				for (;	(*CommandLine != L'\0') && (*CommandLine != L' ');
					CommandLine ++);
				for (; *CommandLine == L' '; CommandLine ++)
				{
					*CommandLine = L'\0';
				}
				/*
				 * Create a native process (suspended).
				 */
				ProcessInfo.Size = sizeof ProcessInfo;
				Request->SmHeader.Status =
					SmCreateUserProcess(ImagePath,
							      CommandLine,
							      SM_CREATE_FLAG_RESERVE_1MB,
							      NULL, /* timeout */
							      & ProcessInfo);
				if (NT_SUCCESS(Request->SmHeader.Status))
				{
					Status = SmCreateClient (& ProcessInfo, Name);
					if (NT_SUCCESS(Status))
					{
						Status = NtResumeThread (ProcessInfo.ThreadHandle, NULL);
						if (!NT_SUCCESS(Status))
						{
							DPRINT1("SM: %s: NtResumeThread failed (Status=0x%08lx)\n",
								__FUNCTION__, Status);
							//Status = SmDestroyClient TODO
						}
					} else {
						DPRINT1("SM: %s: SmCreateClient failed (Status=0x%08lx)\n",
							__FUNCTION__, Status);
					}
					NtClose(ProcessInfo.ThreadHandle);
					NtClose(ProcessInfo.ProcessHandle);
				}
			}
			else
			{
				/*
				 * OK, the definition is empty, but check
				 * if it is the name of an embedded subsystem.
				 */
				if(0 == _wcsicmp(L"DEBUG", Name))
				{
					/*
					 * Initialize the embedded DBGSS.
					 */
					Request->SmHeader.Status = SmInitializeDbgSs();
				}
				else
				{
					/*
					 * Badly defined subsystem. Check the registry!
					 */
					Request->SmHeader.Status = STATUS_NOT_FOUND;
				}
			}
		} else {
			/* It couldn't lookup the Name! */
			Request->SmHeader.Status = Status;
		}
	}
	return Status;
}

/* EOF */
