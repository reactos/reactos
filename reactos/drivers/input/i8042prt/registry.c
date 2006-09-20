/*
 * PROJECT:     ReactOS i8042 (ps/2 keyboard-mouse controller) driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/i8042prt/i8042prt.c
 * PURPOSE:     Reading the registry
 * PROGRAMMERS: Copyright Victor Kirhenshtein (sauros@iname.com)
                Copyright Jason Filby (jasonfilby@yahoo.com)
                Copyright Martijn Vernooij (o112w8r02@sneakemail.com)
                Copyright 2006 Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "i8042prt.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
ReadRegistryEntries(
	IN PUNICODE_STRING RegistryPath,
	OUT PI8042_SETTINGS Settings)
{
	UNICODE_STRING ParametersRegistryKey;
	RTL_QUERY_REGISTRY_TABLE Parameters[16];
	NTSTATUS Status;

	ULONG DefaultKeyboardDataQueueSize = 0x64;
	UNICODE_STRING DefaultKeyboardDeviceBaseName = RTL_CONSTANT_STRING(L"KeyboardPort");
	ULONG DefaultMouseDataQueueSize = 0x64;
	ULONG DefaultMouseResolution = 3;
	ULONG DefaultMouseSynchIn100ns = 20000000;
	ULONG DefaultNumberOfButtons = 2;
	UNICODE_STRING DefaultPointerDeviceBaseName = RTL_CONSTANT_STRING(L"PointerPort");
	ULONG DefaultPollStatusIterations = 1;
	ULONG DefaultOverrideKeyboardType = 4;
	ULONG DefaultOverrideKeyboardSubtype = 0;
	ULONG DefaultPollingIterations = 0x400;
	ULONG DefaultPollingIterationsMaximum = 12000;
	ULONG DefaultResendIterations = 0x3;
	ULONG DefaultSampleRate = 60;
	ULONG DefaultCrashOnCtrlScroll;

	/* Default value for CrashOnCtrlScroll depends if we're
	 * running a debug build or a normal build.
	 */
#ifdef DBG
	DefaultCrashOnCtrlScroll = 1;
#else
	DefaultCrashOnCtrlScroll = 0;
#endif

	ParametersRegistryKey.Length = 0;
	ParametersRegistryKey.MaximumLength = RegistryPath->Length + sizeof(L"\\Parameters") + sizeof(UNICODE_NULL);
	ParametersRegistryKey.Buffer = ExAllocatePool(PagedPool, ParametersRegistryKey.MaximumLength);
	if (!ParametersRegistryKey.Buffer)
	{
		DPRINT("ExAllocatePool() failed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlCopyUnicodeString(&ParametersRegistryKey, RegistryPath);
	RtlAppendUnicodeToString(&ParametersRegistryKey, L"\\Parameters");
	ParametersRegistryKey.Buffer[ParametersRegistryKey.Length / sizeof(WCHAR)] = UNICODE_NULL;

	RtlZeroMemory(Parameters, sizeof(Parameters));

	Parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[0].Name = L"KeyboardDataQueueSize";
	Parameters[0].EntryContext = &Settings->KeyboardDataQueueSize;
	Parameters[0].DefaultType = REG_DWORD;
	Parameters[0].DefaultData = &DefaultKeyboardDataQueueSize;
	Parameters[0].DefaultLength = sizeof(ULONG);

	Parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[1].Name = L"KeyboardDeviceBaseName";
	Parameters[1].EntryContext = &Settings->KeyboardDeviceBaseName;
	Parameters[1].DefaultType = REG_SZ;
	Parameters[1].DefaultData = &DefaultKeyboardDeviceBaseName;
	Parameters[1].DefaultLength = 0;

	Parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[2].Name = L"MouseDataQueueSize";
	Parameters[2].EntryContext = &Settings->MouseDataQueueSize;
	Parameters[2].DefaultType = REG_DWORD;
	Parameters[2].DefaultData = &DefaultMouseDataQueueSize;
	Parameters[2].DefaultLength = sizeof(ULONG);

	Parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[3].Name = L"MouseResolution";
	Parameters[3].EntryContext = &Settings->MouseResolution;
	Parameters[3].DefaultType = REG_DWORD;
	Parameters[3].DefaultData = &DefaultMouseResolution;
	Parameters[3].DefaultLength = sizeof(ULONG);

	Parameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[4].Name = L"MouseSynchIn100ns";
	Parameters[4].EntryContext = &Settings->MouseSynchIn100ns;
	Parameters[4].DefaultType = REG_DWORD;
	Parameters[4].DefaultData = &DefaultMouseSynchIn100ns;
	Parameters[4].DefaultLength = sizeof(ULONG);

	Parameters[5].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[5].Name = L"NumberOfButtons";
	Parameters[5].EntryContext = &Settings->NumberOfButtons;
	Parameters[5].DefaultType = REG_DWORD;
	Parameters[5].DefaultData = &DefaultNumberOfButtons;
	Parameters[5].DefaultLength = sizeof(ULONG);

	Parameters[6].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[6].Name = L"PointerDeviceBaseName";
	Parameters[6].EntryContext = &Settings->PointerDeviceBaseName;
	Parameters[6].DefaultType = REG_SZ;
	Parameters[6].DefaultData = &DefaultPointerDeviceBaseName;
	Parameters[6].DefaultLength = 0;

	Parameters[7].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[7].Name = L"PollStatusIterations";
	Parameters[7].EntryContext = &Settings->PollStatusIterations;
	Parameters[7].DefaultType = REG_DWORD;
	Parameters[7].DefaultData = &DefaultPollStatusIterations;
	Parameters[7].DefaultLength = sizeof(ULONG);

	Parameters[8].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[8].Name = L"OverrideKeyboardType";
	Parameters[8].EntryContext = &Settings->OverrideKeyboardType;
	Parameters[8].DefaultType = REG_DWORD;
	Parameters[8].DefaultData = &DefaultOverrideKeyboardType;
	Parameters[8].DefaultLength = sizeof(ULONG);

	Parameters[9].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[9].Name = L"OverrideKeyboardSubtype";
	Parameters[9].EntryContext = &Settings->OverrideKeyboardSubtype;
	Parameters[9].DefaultType = REG_DWORD;
	Parameters[9].DefaultData = &DefaultOverrideKeyboardSubtype;
	Parameters[9].DefaultLength = sizeof(ULONG);

	Parameters[10].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[10].Name = L"PollingIterations";
	Parameters[10].EntryContext = &Settings->PollingIterations;
	Parameters[10].DefaultType = REG_DWORD;
	Parameters[10].DefaultData = &DefaultPollingIterations;
	Parameters[10].DefaultLength = sizeof(ULONG);

	Parameters[11].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[11].Name = L"PollingIterationsMaximum";
	Parameters[11].EntryContext = &Settings->PollingIterationsMaximum;
	Parameters[11].DefaultType = REG_DWORD;
	Parameters[11].DefaultData = &DefaultPollingIterationsMaximum;
	Parameters[11].DefaultLength = sizeof(ULONG);

	Parameters[12].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[12].Name = L"ResendIterations";
	Parameters[12].EntryContext = &Settings->ResendIterations;
	Parameters[12].DefaultType = REG_DWORD;
	Parameters[12].DefaultData = &DefaultResendIterations;
	Parameters[12].DefaultLength = sizeof(ULONG);

	Parameters[13].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[13].Name = L"SampleRate";
	Parameters[13].EntryContext = &Settings->SampleRate;
	Parameters[13].DefaultType = REG_DWORD;
	Parameters[13].DefaultData = &DefaultSampleRate;
	Parameters[13].DefaultLength = sizeof(ULONG);

	Parameters[14].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[14].Name = L"CrashOnCtrlScroll";
	Parameters[14].EntryContext = &Settings->CrashOnCtrlScroll;
	Parameters[14].DefaultType = REG_DWORD;
	Parameters[14].DefaultData = &DefaultCrashOnCtrlScroll;
	Parameters[14].DefaultLength = sizeof(ULONG);

	Status = RtlQueryRegistryValues(
		RTL_REGISTRY_ABSOLUTE,
		ParametersRegistryKey.Buffer,
		Parameters,
		NULL,
		NULL);

	if (NT_SUCCESS(Status))
	{
		/* Check values */
		if (Settings->KeyboardDataQueueSize < 1)
			Settings->KeyboardDataQueueSize = DefaultKeyboardDataQueueSize;
		if (Settings->MouseDataQueueSize < 1)
			Settings->MouseDataQueueSize = DefaultMouseDataQueueSize;
		if (Settings->NumberOfButtons < 1)
			Settings->NumberOfButtons = DefaultNumberOfButtons;
		if (Settings->PollingIterations < 0x400)
			Settings->PollingIterations = DefaultPollingIterations;
		if (Settings->PollingIterationsMaximum < 0x400)
			Settings->PollingIterationsMaximum = DefaultPollingIterationsMaximum;
		if (Settings->ResendIterations < 1)
			Settings->ResendIterations = DefaultResendIterations;
	}
	else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
	{
		/* Registry path doesn't exist. Set defaults */
		Settings->KeyboardDataQueueSize = DefaultKeyboardDataQueueSize;
		Settings->MouseDataQueueSize = DefaultMouseDataQueueSize;
		Settings->MouseResolution = DefaultMouseResolution;
		Settings->MouseSynchIn100ns = DefaultMouseSynchIn100ns;
		Settings->NumberOfButtons = DefaultNumberOfButtons;
		Settings->PollStatusIterations = DefaultPollStatusIterations;
		Settings->OverrideKeyboardType = DefaultOverrideKeyboardType;
		Settings->OverrideKeyboardSubtype = DefaultOverrideKeyboardSubtype;
		Settings->PollingIterations = DefaultPollingIterations;
		Settings->PollingIterationsMaximum = DefaultPollingIterationsMaximum;
		Settings->ResendIterations = DefaultResendIterations;
		Settings->SampleRate = DefaultSampleRate;
		Settings->CrashOnCtrlScroll = DefaultCrashOnCtrlScroll;
		Status = RtlDuplicateUnicodeString(
			RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
			&DefaultKeyboardDeviceBaseName,
			&Settings->KeyboardDeviceBaseName);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("RtlDuplicateUnicodeString() failed with status 0x%08lx\n", Status);
		}
		else
		{
			Status = RtlDuplicateUnicodeString(
				RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
				&DefaultPointerDeviceBaseName,
				&Settings->PointerDeviceBaseName);
			if (!NT_SUCCESS(Status))
				DPRINT("RtlDuplicateUnicodeString() failed with status 0x%08lx\n", Status);
		}
	}

	if (NT_SUCCESS(Status))
	{
		DPRINT("KeyboardDataQueueSize : 0x%lx\n", Settings->KeyboardDataQueueSize);
		DPRINT("KeyboardDeviceBaseName : %wZ\n", &Settings->KeyboardDeviceBaseName);
		DPRINT("MouseDataQueueSize : 0x%lx\n", Settings->MouseDataQueueSize);
		DPRINT("MouseResolution : 0x%lx\n", Settings->MouseResolution);
		DPRINT("MouseSynchIn100ns : %lu\n", Settings->MouseSynchIn100ns);
		DPRINT("NumberOfButtons : 0x%lx\n", Settings->NumberOfButtons);
		DPRINT("PointerDeviceBaseName : %wZ\n", &Settings->PointerDeviceBaseName);
		DPRINT("PollStatusIterations : 0x%lx\n", Settings->PollStatusIterations);
		DPRINT("OverrideKeyboardType : 0x%lx\n", Settings->OverrideKeyboardType);
		DPRINT("OverrideKeyboardSubtype : 0x%lx\n", Settings->OverrideKeyboardSubtype);
		DPRINT("PollingIterations : 0x%lx\n", Settings->PollingIterations);
		DPRINT("PollingIterationsMaximum : %lu\n", Settings->PollingIterationsMaximum);
		DPRINT("ResendIterations : 0x%lx\n", Settings->ResendIterations);
		DPRINT("SampleRate : %lu\n", Settings->SampleRate);
	}

	return Status;
}
