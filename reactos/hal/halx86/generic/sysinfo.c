/*
 * PROJECT:         ReactOS HA:
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/sysinfo.c
 * PURPOSE:         HAL Information Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

PUCHAR KdComPortInUse;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
HaliQuerySystemInformation(IN     HAL_QUERY_INFORMATION_CLASS InformationClass,
                           IN     ULONG  BufferSize,
                           IN OUT PVOID  Buffer,
                              OUT PULONG ReturnedLength)
{
#define REPORT_THIS_CASE(X) case X: DPRINT1("Unhandled case: %s\n", #X); break
	switch (InformationClass)
	{
		REPORT_THIS_CASE(HalInstalledBusInformation);
		REPORT_THIS_CASE(HalProfileSourceInformation);
		REPORT_THIS_CASE(HalInformationClassUnused1);
		REPORT_THIS_CASE(HalPowerInformation);
		REPORT_THIS_CASE(HalProcessorSpeedInformation);
		REPORT_THIS_CASE(HalCallbackInformation);
		REPORT_THIS_CASE(HalMapRegisterInformation);
		REPORT_THIS_CASE(HalMcaLogInformation);
		case HalFrameBufferCachingInformation:
		if (BufferSize >= 1)
		{
			// The only caller that has been seen calling this function told
			// us it expected a single byte back. We therefore guess it expects
			// a BOOLEAN, and we dream up the value TRUE to (we think) tell it
			// "Sure, the framebuffer is cached".
			BOOLEAN ToReturn = TRUE;
			DPRINT("%s: caller expects %u bytes (should be 1)\n", "HalFrameBufferCachingInformation", BufferSize);
			ASSERT(sizeof(BOOLEAN) == 1);
			*ReturnedLength = sizeof(BOOLEAN);
			RtlCopyMemory(Buffer, &ToReturn, sizeof(BOOLEAN));
			return STATUS_SUCCESS;
		}
		break;
		REPORT_THIS_CASE(HalDisplayBiosInformation);
		REPORT_THIS_CASE(HalProcessorFeatureInformation);
		REPORT_THIS_CASE(HalNumaTopologyInterface);
		REPORT_THIS_CASE(HalErrorInformation);
		REPORT_THIS_CASE(HalCmcLogInformation);
		REPORT_THIS_CASE(HalCpeLogInformation);
		REPORT_THIS_CASE(HalQueryMcaInterface);
		REPORT_THIS_CASE(HalQueryAMLIIllegalIOPortAddresses);
		REPORT_THIS_CASE(HalQueryMaxHotPlugMemoryAddress);
		REPORT_THIS_CASE(HalPartitionIpiInterface);
		REPORT_THIS_CASE(HalPlatformInformation);
		REPORT_THIS_CASE(HalQueryProfileSourceList);
	}
#undef REPORT_THIS_CASE

	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HaliSetSystemInformation(IN HAL_SET_INFORMATION_CLASS InformationClass,
                         IN ULONG BufferSize,
                         IN OUT PVOID Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
