/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/smlib/lookupss.c
 */
#include "precomp.h"

#define NDEBUG
#include <debug.h>

/**********************************************************************
 * NAME							EXPORTED
 *	SmLookupSubsystem/6
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
 * 	Environment: set it if you want this function to use it
 * 	      to possibly expand Data before giving it back; if set
 * 	      to NULL, no expansion will be performed.
 */
NTSTATUS WINAPI
SmLookupSubsystem (IN     PWSTR   Name,
		   IN OUT PWSTR   Data,
		   IN OUT PULONG  DataLength,
		   IN OUT PULONG  DataType,
		   IN     PVOID   Environment OPTIONAL)
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
		PWCHAR         KeyValueInformation = NULL;
		ULONG          KeyValueInformationLength = 1024;
		ULONG          ResultLength = 0L;
		PKEY_VALUE_PARTIAL_INFORMATION kvpi = NULL;

		KeyValueInformation = RtlAllocateHeap (RtlGetProcessHeap(),
						       0,
						       KeyValueInformationLength);
		if (NULL == KeyValueInformation)
		{
			return STATUS_NO_MEMORY;
		}
		kvpi = (PKEY_VALUE_PARTIAL_INFORMATION) KeyValueInformation;
		RtlInitUnicodeString (& usValueName, Name);
		Status = NtQueryValueKey (hKey,
					  & usValueName,
					  KeyValuePartialInformation,
					  KeyValueInformation,
					  KeyValueInformationLength,
					  & ResultLength);
		if(NT_SUCCESS(Status))
		{
			DPRINT("nkvpi.TitleIndex = %ld\n", kvpi->TitleIndex);
			DPRINT("kvpi.Type        = %ld\n", kvpi->Type);
			DPRINT("kvpi.DataLength  = %ld\n", kvpi->DataLength);

			if((NULL != Data) && (NULL != DataLength) && (NULL != DataType))
			{
				*DataType = kvpi->Type;
				if((NULL != Environment) && (REG_EXPAND_SZ == *DataType))
				{
					UNICODE_STRING Source;
					PWCHAR         DestinationBuffer = NULL;
					UNICODE_STRING Destination;
					ULONG          Length = 0;

					DPRINT("SM: %s: value will be expanded\n", __FUNCTION__);

					DestinationBuffer = RtlAllocateHeap (RtlGetProcessHeap(),
									     0,
									     (2 * KeyValueInformationLength));
					if (NULL == DestinationBuffer)
					{
						Status = STATUS_NO_MEMORY;
					}
					else
					{
						Source.Length        = kvpi->DataLength;
						Source.MaximumLength = kvpi->DataLength;
						Source.Buffer        = (PWCHAR) & kvpi->Data;

						Destination.Length        = 0;
						Destination.MaximumLength = (2 * KeyValueInformationLength);
						Destination.Buffer        = DestinationBuffer;

						Status = RtlExpandEnvironmentStrings_U (Environment,
											& Source,
											& Destination,
											& Length);
						if(NT_SUCCESS(Status))
						{
							*DataLength = min(*DataLength, Destination.Length);
							RtlCopyMemory (Data, Destination.Buffer, *DataLength);
						}
						RtlFreeHeap (RtlGetProcessHeap(), 0, DestinationBuffer);
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
		RtlFreeHeap (RtlGetProcessHeap(), 0, KeyValueInformation);
		NtClose (hKey);
	}else{
		DPRINT1("%s: NtOpenKey failed (Status=0x%08lx)\n", __FUNCTION__, Status);
	}
	return Status;
}

/* EOF */
