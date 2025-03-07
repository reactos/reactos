/*
 * PROJECT:     ReactOS i8042 (ps/2 keyboard-mouse controller) driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/i8042prt/i8042prt.c
 * PURPOSE:     Reading the registry
 * PROGRAMMERS: Copyright Victor Kirhenshtein (sauros@iname.com)
                Copyright Jason Filby (jasonfilby@yahoo.com)
                Copyright Martijn Vernooij (o112w8r02@sneakemail.com)
                Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "i8042prt.h"

#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
ReadRegistryEntries(
	IN PUNICODE_STRING RegistryPath,
	OUT PI8042_SETTINGS Settings)
{
	RTL_QUERY_REGISTRY_TABLE Parameters[17];
	NTSTATUS Status;

	ULONG DefaultKeyboardDataQueueSize = 0x64;
	PCWSTR DefaultKeyboardDeviceBaseName = L"KeyboardPort";
	ULONG DefaultMouseDataQueueSize = 0x64;
	ULONG DefaultMouseResolution = 3;
	ULONG DefaultMouseSynchIn100ns = 20000000;
	ULONG DefaultNumberOfButtons = 2;
	PCWSTR DefaultPointerDeviceBaseName = L"PointerPort";
	ULONG DefaultPollStatusIterations = 1;
	ULONG DefaultOverrideKeyboardType = 4;
	ULONG DefaultOverrideKeyboardSubtype = 0;
	ULONG DefaultPollingIterations = 12000;
	ULONG DefaultPollingIterationsMaximum = 12000;
	ULONG DefaultResendIterations = 0x3;
	ULONG DefaultSampleRate = 60;
	ULONG DefaultCrashOnCtrlScroll;

	/* Default value for CrashOnCtrlScroll depends if we're
	 * running a debug build or a normal build.
	 */
#if DBG
	DefaultCrashOnCtrlScroll = 1;
#else
	DefaultCrashOnCtrlScroll = 0;
#endif

	RtlZeroMemory(Parameters, sizeof(Parameters));

	Parameters[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
	Parameters[0].Name = L"Parameters";

	Parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[1].Name = L"KeyboardDataQueueSize";
	Parameters[1].EntryContext = &Settings->KeyboardDataQueueSize;
	Parameters[1].DefaultType = REG_DWORD;
	Parameters[1].DefaultData = &DefaultKeyboardDataQueueSize;
	Parameters[1].DefaultLength = sizeof(ULONG);

	Parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[2].Name = L"KeyboardDeviceBaseName";
	Parameters[2].EntryContext = &Settings->KeyboardDeviceBaseName;
	Parameters[2].DefaultType = REG_SZ;
	Parameters[2].DefaultData = (PVOID)DefaultKeyboardDeviceBaseName;
	Parameters[2].DefaultLength = 0;

	Parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[3].Name = L"MouseDataQueueSize";
	Parameters[3].EntryContext = &Settings->MouseDataQueueSize;
	Parameters[3].DefaultType = REG_DWORD;
	Parameters[3].DefaultData = &DefaultMouseDataQueueSize;
	Parameters[3].DefaultLength = sizeof(ULONG);

	Parameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[4].Name = L"MouseResolution";
	Parameters[4].EntryContext = &Settings->MouseResolution;
	Parameters[4].DefaultType = REG_DWORD;
	Parameters[4].DefaultData = &DefaultMouseResolution;
	Parameters[4].DefaultLength = sizeof(ULONG);

	Parameters[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[5].Name = L"MouseSynchIn100ns";
	Parameters[5].EntryContext = &Settings->MouseSynchIn100ns;
	Parameters[5].DefaultType = REG_DWORD;
	Parameters[5].DefaultData = &DefaultMouseSynchIn100ns;
	Parameters[5].DefaultLength = sizeof(ULONG);

	Parameters[6].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[6].Name = L"NumberOfButtons";
	Parameters[6].EntryContext = &Settings->NumberOfButtons;
	Parameters[6].DefaultType = REG_DWORD;
	Parameters[6].DefaultData = &DefaultNumberOfButtons;
	Parameters[6].DefaultLength = sizeof(ULONG);

	Parameters[7].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[7].Name = L"PointerDeviceBaseName";
	Parameters[7].EntryContext = &Settings->PointerDeviceBaseName;
	Parameters[7].DefaultType = REG_SZ;
	Parameters[7].DefaultData = (PVOID)DefaultPointerDeviceBaseName;
	Parameters[7].DefaultLength = 0;

	Parameters[8].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[8].Name = L"PollStatusIterations";
	Parameters[8].EntryContext = &Settings->PollStatusIterations;
	Parameters[8].DefaultType = REG_DWORD;
	Parameters[8].DefaultData = &DefaultPollStatusIterations;
	Parameters[8].DefaultLength = sizeof(ULONG);

	Parameters[9].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[9].Name = L"OverrideKeyboardType";
	Parameters[9].EntryContext = &Settings->OverrideKeyboardType;
	Parameters[9].DefaultType = REG_DWORD;
	Parameters[9].DefaultData = &DefaultOverrideKeyboardType;
	Parameters[9].DefaultLength = sizeof(ULONG);

	Parameters[10].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[10].Name = L"OverrideKeyboardSubtype";
	Parameters[10].EntryContext = &Settings->OverrideKeyboardSubtype;
	Parameters[10].DefaultType = REG_DWORD;
	Parameters[10].DefaultData = &DefaultOverrideKeyboardSubtype;
	Parameters[10].DefaultLength = sizeof(ULONG);

	Parameters[11].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[11].Name = L"PollingIterations";
	Parameters[11].EntryContext = &Settings->PollingIterations;
	Parameters[11].DefaultType = REG_DWORD;
	Parameters[11].DefaultData = &DefaultPollingIterations;
	Parameters[11].DefaultLength = sizeof(ULONG);

	Parameters[12].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[12].Name = L"PollingIterationsMaximum";
	Parameters[12].EntryContext = &Settings->PollingIterationsMaximum;
	Parameters[12].DefaultType = REG_DWORD;
	Parameters[12].DefaultData = &DefaultPollingIterationsMaximum;
	Parameters[12].DefaultLength = sizeof(ULONG);

	Parameters[13].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[13].Name = L"ResendIterations";
	Parameters[13].EntryContext = &Settings->ResendIterations;
	Parameters[13].DefaultType = REG_DWORD;
	Parameters[13].DefaultData = &DefaultResendIterations;
	Parameters[13].DefaultLength = sizeof(ULONG);

	Parameters[14].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[14].Name = L"SampleRate";
	Parameters[14].EntryContext = &Settings->SampleRate;
	Parameters[14].DefaultType = REG_DWORD;
	Parameters[14].DefaultData = &DefaultSampleRate;
	Parameters[14].DefaultLength = sizeof(ULONG);

	Parameters[15].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[15].Name = L"CrashOnCtrlScroll";
	Parameters[15].EntryContext = &Settings->CrashOnCtrlScroll;
	Parameters[15].DefaultType = REG_DWORD;
	Parameters[15].DefaultData = &DefaultCrashOnCtrlScroll;
	Parameters[15].DefaultLength = sizeof(ULONG);

	Status = RtlQueryRegistryValues(
		RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
		RegistryPath->Buffer,
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
		if (!RtlCreateUnicodeString(&Settings->KeyboardDeviceBaseName, DefaultKeyboardDeviceBaseName)
		 || !RtlCreateUnicodeString(&Settings->PointerDeviceBaseName, DefaultPointerDeviceBaseName))
		{
			WARN_(I8042PRT, "RtlCreateUnicodeString() failed\n");
			Status = STATUS_NO_MEMORY;
		}
		else
		{
			Status = STATUS_SUCCESS;
		}
	}

	if (NT_SUCCESS(Status))
	{
		INFO_(I8042PRT, "KeyboardDataQueueSize : 0x%lx\n", Settings->KeyboardDataQueueSize);
		INFO_(I8042PRT, "KeyboardDeviceBaseName : %wZ\n", &Settings->KeyboardDeviceBaseName);
		INFO_(I8042PRT, "MouseDataQueueSize : 0x%lx\n", Settings->MouseDataQueueSize);
		INFO_(I8042PRT, "MouseResolution : 0x%lx\n", Settings->MouseResolution);
		INFO_(I8042PRT, "MouseSynchIn100ns : %lu\n", Settings->MouseSynchIn100ns);
		INFO_(I8042PRT, "NumberOfButtons : 0x%lx\n", Settings->NumberOfButtons);
		INFO_(I8042PRT, "PointerDeviceBaseName : %wZ\n", &Settings->PointerDeviceBaseName);
		INFO_(I8042PRT, "PollStatusIterations : 0x%lx\n", Settings->PollStatusIterations);
		INFO_(I8042PRT, "OverrideKeyboardType : 0x%lx\n", Settings->OverrideKeyboardType);
		INFO_(I8042PRT, "OverrideKeyboardSubtype : 0x%lx\n", Settings->OverrideKeyboardSubtype);
		INFO_(I8042PRT, "PollingIterations : 0x%lx\n", Settings->PollingIterations);
		INFO_(I8042PRT, "PollingIterationsMaximum : %lu\n", Settings->PollingIterationsMaximum);
		INFO_(I8042PRT, "ResendIterations : 0x%lx\n", Settings->ResendIterations);
		INFO_(I8042PRT, "SampleRate : %lu\n", Settings->SampleRate);
	}

	return Status;
}
