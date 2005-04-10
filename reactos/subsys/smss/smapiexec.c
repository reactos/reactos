/* $Id$
 *
 * smapiexec.c - SM_API_EXECUTE_PROGRAM
 *
 * Reactos Session Manager
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include "smss.h"

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
                   
   RtlDestroyProcessParameters (ProcessParameters);
   
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("SM: %s: Running \"%S\" failed (Status=0x%08lx)\n",
			__FUNCTION__, ImagePathString.Buffer, Status);
		return Status;
	}

   ZwResumeThread(pProcessInfo->ThreadHandle, NULL);


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
 * NAME
 *	SmLookupSubsystem/5
 *
 * DESCRIPTION
 * 	Read from the registry key
 * 	\Registry\SYSTEM\CurrentControlSet\Control\Session Manager\Subsystems
 * 	the value which name is Name.
 *
 * ARGUMENTS
 * 	Name: name of the program to run, that is a value's name in
 * 	      the SM registry key Subsystems;
 * 	Data: what the registry gave back for Name;
 * 	DataLength: how much Data the registry returns;
 * 	DataType: what is Data?
 * 	Expand: set it TRUE if you want this function to use the env
 * 	      to possibly expand Data before giving it back.
 */
NTSTATUS STDCALL
SmLookupSubsystem (IN     PWSTR   Name,
		   IN OUT PWSTR   Data,
		   IN OUT PULONG  DataLength,
		   IN OUT PULONG  DataType,
		   IN     BOOLEAN Expand)
{
	NTSTATUS           Status = STATUS_SUCCESS;
	UNICODE_STRING     usKeyName = {0};
	OBJECT_ATTRIBUTES  Oa = {0};
	HANDLE             hKey = (HANDLE) 0;

	DPRINT("SM: %s(Name='%S') called\n", __FUNCTION__, Name);
	/*
	 * Prepare the key name to scan and
	 * related object attributes.
	 */
	RtlInitUnicodeString (& usKeyName,
		L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\SubSystems");

	InitializeObjectAttributes (& Oa,
				    & usKeyName,
				    OBJ_CASE_INSENSITIVE,
				    NULL,
				    NULL);
	/*
	 * Open the key. This MUST NOT fail, if the
	 * request is for a legitimate subsystem.
	 */
	Status = NtOpenKey (& hKey,
			      MAXIMUM_ALLOWED,
			      & Oa);
	if(NT_SUCCESS(Status))
	{
		UNICODE_STRING usValueName = {0};
		WCHAR          KeyValueInformation [1024] = {L'\0'};
		ULONG          ResultLength = 0L;
		PKEY_VALUE_PARTIAL_INFORMATION
			       kvpi = (PKEY_VALUE_PARTIAL_INFORMATION) KeyValueInformation;

		
		RtlInitUnicodeString (& usValueName, Name);
		Status = NtQueryValueKey (hKey,
					  & usValueName,
					  KeyValuePartialInformation,
					  KeyValueInformation,
					  sizeof KeyValueInformation,
					  & ResultLength);
		if(NT_SUCCESS(Status))
		{
			DPRINT("nkvpi.TitleIndex = %ld\n", kvpi->TitleIndex);
			DPRINT("kvpi.Type        = %ld\n", kvpi->Type);
			DPRINT("kvpi.DataLength  = %ld\n", kvpi->DataLength);

			if((NULL != Data) && (NULL != DataLength) && (NULL != DataType))
			{
				*DataType = kvpi->Type;
				if((Expand) && (REG_EXPAND_SZ == *DataType))
				{
					UNICODE_STRING Source;
					WCHAR          DestinationBuffer [2048] = {0};
					UNICODE_STRING Destination;
					ULONG          Length = 0;

					DPRINT("SM: %s: value will be expanded\n", __FUNCTION__);

					Source.Length        = kvpi->DataLength;
					Source.MaximumLength = kvpi->DataLength;
					Source.Buffer        = (PWCHAR) & kvpi->Data;

					Destination.Length        = 0;
					Destination.MaximumLength = sizeof DestinationBuffer;
					Destination.Buffer        = DestinationBuffer;

					Status = RtlExpandEnvironmentStrings_U (SmSystemEnvironment,
										& Source,
										& Destination,
										& Length);
					if(NT_SUCCESS(Status))
					{
						*DataLength = min(*DataLength, Destination.Length);
						RtlCopyMemory (Data, Destination.Buffer, *DataLength);				
					}
							
				}else{
					DPRINT("SM: %s: value won't be expanded\n", __FUNCTION__);
					*DataLength = min(*DataLength, kvpi->DataLength);
					RtlCopyMemory (Data, & kvpi->Data, *DataLength);
				}
				*DataType = kvpi->Type;
			}else{
				DPRINT1("SM: %s: Data or DataLength or DataType is NULL!\n", __FUNCTION__);
				Status = STATUS_INVALID_PARAMETER;
			}
		}else{
			DPRINT1("%s: NtQueryValueKey failed (Status=0x%08lx)\n", __FUNCTION__, Status);
		}
		NtClose (hKey);
	}else{
		DPRINT1("%s: NtOpenKey failed (Status=0x%08lx)\n", __FUNCTION__, Status);
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
	DPRINT("SM: %s called from CID(%lx|%lx)\n",
		__FUNCTION__, Request->Header.ClientId.UniqueProcess,
		Request->Header.ClientId.UniqueThread);
	ExecPgm = & Request->Request.ExecPgm;
	/* Check if the name lenght is valid */
	if((ExecPgm->NameLength > 0) &&
	   (ExecPgm->NameLength <= SM_EXEXPGM_MAX_LENGTH) &&
	   TRUE /* TODO: check LPC payload size */)
	{
		
		RtlZeroMemory (Name, sizeof Name);
		RtlCopyMemory (Name,
			       ExecPgm->Name,
			       (sizeof ExecPgm->Name[0] * ExecPgm->NameLength));
		DPRINT("SM: %s: Name='%S'\n", __FUNCTION__, Name);
		/*
		 * Check if program name is internal
		 * (Is this correct? Debug is in the registry too)
		 */
		if(0 == _wcsicmp(L"DEBUG", Name))
		{
			/*
			 * Initialize DBGSS.
			 * NOTE: probably in early prototypes it was an
			 * independent process; now it is embedded in the
			 * SM for performance or security.
			 */
			Request->SmHeader.Status = SmInitializeDbgSs();
		}
		else
		{
			WCHAR Data [MAX_PATH + 1] = {0};
			ULONG DataLength = sizeof Data;
			ULONG DataType = REG_EXPAND_SZ;

			/* Lookup Name in the registry */
			Status = SmLookupSubsystem (Name,
						    Data,
						    & DataLength,
						    & DataType,
						    TRUE); /* expand */
			if(NT_SUCCESS(Status))
			{
				WCHAR ImagePath [MAX_PATH + 1] = {0};

				wcscpy (ImagePath, L"\\??\\");
				wcscat (ImagePath, Data);
			
				/* Create native process */
				Request->SmHeader.Status = SmCreateUserProcess(ImagePath,
								      L"", /* FIXME */
								      FALSE, /* wait */
				      				      NULL,
			      					      FALSE, /* terminate */
			      					      NULL);
			}else{
				Request->SmHeader.Status = Status;
			}
		}
	}
	else
	{
		Request->SmHeader.Status = Status = STATUS_INVALID_PARAMETER;
	}
	return Status;
}

/* EOF */
