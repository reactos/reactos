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

#include <ddk/ntddk.h>
#include <string.h>
#include <ntos/keyboard.h>
#include <ntos/minmax.h>
#include <rosrtl/string.h>

#include <ddk/ntddkbd.h>
#include <ddk/ntdd8042.h>

#define NDEBUG
#include <debug.h>

#include "i8042prt.h"

/* FUNCTIONS *****************************************************************/

/*
 * Read the registry keys associated with this device. The RegistryPath
 * var is a hack. This should be more like what microsoft does, but I
 * don't know exactly what they do except that it's a hack too...
 */
VOID STDCALL I8042ReadRegistry(PDRIVER_OBJECT DriverObject,
                               PDEVICE_EXTENSION DevExt)
                               
{
	RTL_QUERY_REGISTRY_TABLE Parameters[18];
	UNICODE_STRING ParametersPath;

	PWSTR RegistryPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\i8042Prt\\Parameters";

	NTSTATUS Status;

	DWORD DefaultHeadless = 0;
	DWORD DefaultCrashScroll = 0;
	DWORD DefaultCrashSysRq = 0;
	DWORD DefaultReportResetErrors = 0;
	DWORD DefaultPollStatusIterations = 1;
	DWORD DefaultResendIterations = 3;
	DWORD DefaultPollingIterations = 12000;
	DWORD DefaultPollingIterationsMaximum = 12000;
	DWORD DefaultKeyboardDataQueueSize = 100;
	DWORD DefaultOverrideKeyboardType = 0;
	DWORD DefaultOverrideKeyboardSubtype = 0;
	DWORD DefaultMouseDataQueueSize = 100;
	DWORD DefaultMouseResendStallTime = 1000;
	DWORD DefaultMouseSynchIn100ns = 20000000;
	DWORD DefaultMouseResolution = 3;
	DWORD DefaultSampleRate = 60;
	DWORD DefaultNumberOfButtons = 2;
	DWORD DefaultEnableWheelDetection = 1;

	RtlInitUnicodeString(&ParametersPath, NULL);
	ParametersPath.MaximumLength = (wcslen(RegistryPath) *
				       		sizeof(WCHAR)) +
			               sizeof(UNICODE_NULL);

	ParametersPath.Buffer = ExAllocatePoolWithTag(PagedPool,
	                                      ParametersPath.MaximumLength,
	                                      TAG_I8042);

	if (!ParametersPath.Buffer) {
		DPRINT1("No buffer space for reading registry\n");
		return;
	}

	RtlZeroMemory(ParametersPath.Buffer, ParametersPath.MaximumLength);
	RtlAppendUnicodeToString(&ParametersPath, RegistryPath);

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

	Parameters[14].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[14].Name = L"SampleRate";
	Parameters[14].EntryContext = &DevExt->MouseAttributes.SampleRate;
	Parameters[14].DefaultType = REG_DWORD;
	Parameters[14].DefaultData = &DefaultSampleRate;
	Parameters[14].DefaultLength = sizeof(ULONG);

	Parameters[15].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[15].Name = L"NumberOfButtons";
	Parameters[15].EntryContext = &DevExt->Settings.NumberOfButtons;
	Parameters[15].DefaultType = REG_DWORD;
	Parameters[15].DefaultData = &DefaultNumberOfButtons;
	Parameters[15].DefaultLength = sizeof(ULONG);

	Parameters[16].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[16].Name = L"EnableWheelDetection";
	Parameters[16].EntryContext = &DevExt->Settings.EnableWheelDetection;
	Parameters[16].DefaultType = REG_DWORD;
	Parameters[16].DefaultData = &DefaultEnableWheelDetection;
	Parameters[16].DefaultLength = sizeof(ULONG);

	Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE |
			                        RTL_REGISTRY_OPTIONAL,
	                                ParametersPath.Buffer,
	                                Parameters,
	                                NULL,
	                                NULL);

	if (Status != STATUS_SUCCESS) {
		DPRINT1 ("Can't read registry: %x\n", Status);
		/* Actually, the defaults are not set when the function
		 * fails, as would happen during setup, so you have to
		 * set them manually anyway...
		 */
		RTL_QUERY_REGISTRY_TABLE *Current = Parameters;
		while (Current->Name) {
			*((DWORD *)Current->EntryContext) =
			                       *((DWORD *)Current->DefaultData);
			Current++;
		}
		DPRINT1 ("Manually set defaults\n");

	}
	ExFreePoolWithTag(ParametersPath.Buffer, TAG_I8042);
	DPRINT("Done reading registry\n");
}

