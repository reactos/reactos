/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/input/i8042prt/registry.c
 * PURPOSE:          i8042 (ps/2 keyboard-mouse controller) driver
 *                   Reading the registry
 * PROGRAMMER:       Victor Kirhenshtein (sauros@iname.com)
 *                   Jason Filby (jasonfilby@yahoo.com)
 *                   Tinus
 */

/* INCLUDES ****************************************************************/

#include "i8042prt.h"

#ifndef NDEBUG
#define NDEBUG
#endif
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * Read the registry keys associated with this device. The RegistryPath
 * var is a hack. This should be more like what microsoft does, but I
 * don't know exactly what they do except that it's a hack too...
 */
VOID STDCALL I8042ReadRegistry(PDRIVER_OBJECT DriverObject,
                               PDEVICE_EXTENSION DevExt)

{
	RTL_QUERY_REGISTRY_TABLE Parameters[19];

	NTSTATUS Status;

	ULONG DefaultHeadless = 0;
	ULONG DefaultCrashScroll = 0;
	ULONG DefaultCrashSysRq = 0;
	ULONG DefaultReportResetErrors = 0;
	ULONG DefaultPollStatusIterations = 1;
	ULONG DefaultResendIterations = 3;
	ULONG DefaultPollingIterations = 12000;
	ULONG DefaultPollingIterationsMaximum = 12000;
	ULONG DefaultKeyboardDataQueueSize = 100;
	ULONG DefaultOverrideKeyboardType = 0;
	ULONG DefaultOverrideKeyboardSubtype = 0;
	ULONG DefaultMouseDataQueueSize = 100;
	ULONG DefaultMouseResendStallTime = 1000;
	ULONG DefaultMouseSynchIn100ns = 20000000;
	ULONG DefaultMouseResolution = 3;
	ULONG DefaultSampleRate = 60;
	ULONG DefaultNumberOfButtons = 2;
	ULONG DefaultEnableWheelDetection = 1;

	RtlZeroMemory(Parameters, sizeof(Parameters));

	Parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[0].Name = L"Headless";
	Parameters[0].EntryContext = &DevExt->Settings.Headless;
	Parameters[0].DefaultType = REG_DWORD;
	Parameters[0].DefaultData = &DefaultHeadless;
	Parameters[0].DefaultLength = sizeof(ULONG);

	Parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[1].Name = L"CrashOnCtrlScroll";
	Parameters[1].EntryContext = &DevExt->Settings.CrashScroll;
	Parameters[1].DefaultType = REG_DWORD;
	Parameters[1].DefaultData = &DefaultCrashScroll;
	Parameters[1].DefaultLength = sizeof(ULONG);

	Parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[2].Name = L"BreakOnSysRq";
	Parameters[2].EntryContext = &DevExt->Settings.CrashSysRq;
	Parameters[2].DefaultType = REG_DWORD;
	Parameters[2].DefaultData = &DefaultCrashSysRq;
	Parameters[2].DefaultLength = sizeof(ULONG);

	Parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[3].Name = L"ReportResetErrors";
	Parameters[3].EntryContext = &DevExt->Settings.ReportResetErrors;
	Parameters[3].DefaultType = REG_DWORD;
	Parameters[3].DefaultData = &DefaultReportResetErrors;
	Parameters[3].DefaultLength = sizeof(ULONG);

	Parameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[4].Name = L"PollStatusIterations";
	Parameters[4].EntryContext = &DevExt->Settings.PollStatusIterations;
	Parameters[4].DefaultType = REG_DWORD;
	Parameters[4].DefaultData = &DefaultPollStatusIterations;
	Parameters[4].DefaultLength = sizeof(ULONG);

	Parameters[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[5].Name = L"ResendIterations";
	Parameters[5].EntryContext = &DevExt->Settings.ResendIterations;
	Parameters[5].DefaultType = REG_DWORD;
	Parameters[5].DefaultData = &DefaultResendIterations;
	Parameters[5].DefaultLength = sizeof(ULONG);

	Parameters[6].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[6].Name = L"PollingIterations";
	Parameters[6].EntryContext = &DevExt->Settings.PollingIterations;
	Parameters[6].DefaultType = REG_DWORD;
	Parameters[6].DefaultData = &DefaultPollingIterations;
	Parameters[6].DefaultLength = sizeof(ULONG);

	Parameters[7].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[7].Name = L"PollingIterationsMaximum";
	Parameters[7].EntryContext = &DevExt->Settings.PollingIterationsMaximum;
	Parameters[7].DefaultType = REG_DWORD;
	Parameters[7].DefaultData = &DefaultPollingIterationsMaximum;
	Parameters[7].DefaultLength = sizeof(ULONG);

	Parameters[8].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[8].Name = L"KeyboardDataQueueSize";
	Parameters[8].EntryContext =
	                       &DevExt->KeyboardAttributes.InputDataQueueLength;
	Parameters[8].DefaultType = REG_DWORD;
	Parameters[8].DefaultData = &DefaultKeyboardDataQueueSize;
	Parameters[8].DefaultLength = sizeof(ULONG);

	Parameters[9].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[9].Name = L"OverrideKeyboardType";
	Parameters[9].EntryContext = &DevExt->Settings.OverrideKeyboardType;
	Parameters[9].DefaultType = REG_DWORD;
	Parameters[9].DefaultData = &DefaultOverrideKeyboardType;
	Parameters[9].DefaultLength = sizeof(ULONG);

	Parameters[10].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[10].Name = L"OverrideKeyboardSubtype";
	Parameters[10].EntryContext = &DevExt->Settings.OverrideKeyboardSubtype;
	Parameters[10].DefaultType = REG_DWORD;
	Parameters[10].DefaultData = &DefaultOverrideKeyboardSubtype;
	Parameters[10].DefaultLength = sizeof(ULONG);

	Parameters[11].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[11].Name = L"MouseDataQueueSize";
	Parameters[11].EntryContext =
	                      &DevExt->MouseAttributes.InputDataQueueLength;
	Parameters[11].DefaultType = REG_DWORD;
	Parameters[11].DefaultData = &DefaultMouseDataQueueSize;
	Parameters[11].DefaultLength = sizeof(ULONG);

	Parameters[12].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[12].Name = L"MouseResendStallTime";
	Parameters[12].EntryContext = &DevExt->Settings.MouseResendStallTime;
	Parameters[12].DefaultType = REG_DWORD;
	Parameters[12].DefaultData = &DefaultMouseResendStallTime;
	Parameters[12].DefaultLength = sizeof(ULONG);

	Parameters[13].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[13].Name = L"MouseSynchIn100ns";
	Parameters[13].EntryContext = &DevExt->Settings.MouseSynchIn100ns;
	Parameters[13].DefaultType = REG_DWORD;
	Parameters[13].DefaultData = &DefaultMouseSynchIn100ns;
	Parameters[13].DefaultLength = sizeof(ULONG);

	Parameters[14].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[14].Name = L"MouseResolution";
	Parameters[14].EntryContext = &DevExt->Settings.MouseResolution;
	Parameters[14].DefaultType = REG_DWORD;
	Parameters[14].DefaultData = &DefaultMouseResolution;
	Parameters[14].DefaultLength = sizeof(ULONG);

	Parameters[15].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[15].Name = L"SampleRate";
	Parameters[15].EntryContext = &DevExt->MouseAttributes.SampleRate;
	Parameters[15].DefaultType = REG_DWORD;
	Parameters[15].DefaultData = &DefaultSampleRate;
	Parameters[15].DefaultLength = sizeof(ULONG);

	Parameters[16].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[16].Name = L"NumberOfButtons";
	Parameters[16].EntryContext = &DevExt->Settings.NumberOfButtons;
	Parameters[16].DefaultType = REG_DWORD;
	Parameters[16].DefaultData = &DefaultNumberOfButtons;
	Parameters[16].DefaultLength = sizeof(ULONG);

	Parameters[17].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[17].Name = L"EnableWheelDetection";
	Parameters[17].EntryContext = &DevExt->Settings.EnableWheelDetection;
	Parameters[17].DefaultType = REG_DWORD;
	Parameters[17].DefaultData = &DefaultEnableWheelDetection;
	Parameters[17].DefaultLength = sizeof(ULONG);

	Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
	                                I8042RegistryPath.Buffer,
	                                Parameters,
	                                NULL,
	                                NULL);

	if (!NT_SUCCESS(Status)) {
		/* Actually, the defaults are not set when the function
		 * fails, as would happen during setup, so you have to
		 * set them manually anyway...
		 */
		RTL_QUERY_REGISTRY_TABLE *Current = Parameters;
		DPRINT ("Can't read registry: %x\n", Status);
		while (Current->Name) {
			*((PULONG)Current->EntryContext) =
			                       *((PULONG)Current->DefaultData);
			Current++;
		}
		DPRINT ("Manually set defaults\n");

	}

	if (DevExt->Settings.MouseResolution > 3)
		DevExt->Settings.MouseResolution = 3;

	DPRINT("Done reading registry\n");
}
