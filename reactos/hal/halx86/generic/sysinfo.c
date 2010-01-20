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
PADDRESS_USAGE HalpAddressUsageList;
IDTUsageFlags HalpIDTUsageFlags[MAXIMUM_IDTVECTOR];
IDTUsage HalpIDTUsage[MAXIMUM_IDTVECTOR];

ADDRESS_USAGE HalpDefaultIoSpace =
{
    NULL, CmResourceTypePort, IDT_INTERNAL,
    {
        {0x2000,  0xC000}, /* PIC?? */
        {0xC000,  0x1000}, /* DMA 2 */
        {0x8000,  0x1000}, /* DMA 1 */
        {0x2000,  0x200},  /* PIC 1 */
        {0xA000,  0x200},  /* PIC 2 */
        {0x4000,  0x400},  /* PIT 1 */
        {0x4800,  0x400},  /* PIT 2 */
        {0x9200,  0x100},  /* ????? */
        {0x7000,  0x200},  /* CMOS  */
        {0xF000,  0x1000}, /* ????? */
        {0xCF800, 0x800},  /* PCI 0 */
        {0,0},
    }
};

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
HaliQuerySystemInformation(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
                           IN ULONG BufferSize,
                           IN OUT PVOID Buffer,
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
		{
            /* FIXME: TODO */
            return STATUS_NOT_IMPLEMENTED;
		}
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

VOID
NTAPI
HalpRegisterVector(IN UCHAR Flags,
                   IN ULONG BusVector,
                   IN ULONG SystemVector,
                   IN KIRQL Irql)
{
    /* Save the vector flags */
    HalpIDTUsageFlags[SystemVector].Flags = Flags;

    /* Save the vector data */
    HalpIDTUsage[SystemVector].Irql  = Irql;
    HalpIDTUsage[SystemVector].BusReleativeVector = BusVector;
}

VOID
NTAPI
HalpEnableInterruptHandler(IN UCHAR Flags,
                           IN ULONG BusVector,
                           IN ULONG SystemVector,
                           IN KIRQL Irql,
                           IN PVOID Handler,
                           IN KINTERRUPT_MODE Mode)
{
    UCHAR Entry;

    /* Convert the vector into the IDT entry */
    Entry = HalVectorToIDTEntry(SystemVector);

    /* Register the vector */
    HalpRegisterVector(Flags, BusVector, SystemVector, Irql);

    /* Connect the interrupt */
    ((PKIPCR)KeGetPcr())->IDT[Entry].ExtendedOffset = (USHORT)(((ULONG_PTR)Handler >> 16) & 0xFFFF);
    ((PKIPCR)KeGetPcr())->IDT[Entry].Offset = (USHORT)((ULONG_PTR)Handler);

    /* Enable the interrupt */
    HalEnableSystemInterrupt(SystemVector, Irql, Mode);
}

/* EOF */
