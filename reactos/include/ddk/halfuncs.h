#ifndef __INCLUDE_DDK_HALFUNCS_H
#define __INCLUDE_DDK_HALFUNCS_H
/* $Id$ */

#include <ntos/haltypes.h>
#include <ddk/iotypes.h>

VOID STDCALL
HalAcquireDisplayOwnership(IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters);

NTSTATUS STDCALL
HalAllocateAdapterChannel(IN PADAPTER_OBJECT AdapterObject,
			  IN PWAIT_CONTEXT_BLOCK WaitContextBlock,
			  IN ULONG NumberOfMapRegisters,
			  IN PDRIVER_CONTROL ExecutionRoutine);

PVOID STDCALL
HalAllocateCommonBuffer(PADAPTER_OBJECT AdapterObject,
			ULONG Length,
			PPHYSICAL_ADDRESS LogicalAddress,
			BOOLEAN CacheEnabled);

PVOID STDCALL
HalAllocateCrashDumpRegisters(IN PADAPTER_OBJECT AdapterObject,
			      IN OUT PULONG NumberOfMapRegisters);

NTSTATUS STDCALL
HalAssignSlotResources(
	PUNICODE_STRING		RegistryPath,
	PUNICODE_STRING		DriverClassName,
	PDRIVER_OBJECT		DriverObject,
	PDEVICE_OBJECT		DeviceObject,
	INTERFACE_TYPE		BusType,
	ULONG			BusNumber,
	ULONG			SlotNumber,
	PCM_RESOURCE_LIST	*AllocatedResources
	);

/*
FASTCALL
HalClearSoftwareInterrupt
*/

/*
 * HalExamineMBR() is not exported explicitly.
 * It is exported by the HalDispatchTable.
 *
 * VOID
 * HalExamineMBR(PDEVICE_OBJECT DeviceObject,
 *               ULONG SectorSize,
 *               ULONG MBRTypeIdentifier,
 *               PVOID Buffer);
 */

VOID STDCALL
HalFreeCommonBuffer(PADAPTER_OBJECT AdapterObject,
		    ULONG Length,
		    PHYSICAL_ADDRESS LogicalAddress,
		    PVOID VirtualAddress,
		    BOOLEAN CacheEnabled);

PADAPTER_OBJECT STDCALL
HalGetAdapter(PDEVICE_DESCRIPTION DeviceDescription,
	      PULONG NumberOfMapRegisters);

ULONG STDCALL
HalGetBusData(BUS_DATA_TYPE BusDataType,
	      ULONG BusNumber,
	      ULONG SlotNumber,
	      PVOID Buffer,
	      ULONG Length);

ULONG STDCALL
HalGetBusDataByOffset(BUS_DATA_TYPE BusDataType,
		      ULONG BusNumber,
		      ULONG SlotNumber,
		      PVOID Buffer,
		      ULONG Offset,
		      ULONG Length);

ULONG STDCALL
HalGetDmaAlignmentRequirement( 
  VOID);

			   
ULONG STDCALL
HalGetInterruptVector(INTERFACE_TYPE InterfaceType,
		      ULONG BusNumber,
		      ULONG BusInterruptLevel,
		      ULONG BusInterruptVector,
		      PKIRQL Irql,
		      PKAFFINITY Affinity);

BOOLEAN STDCALL
HalMakeBeep(ULONG Frequency);

/*
 * HalQuerySystemInformation() is not exported explicitly.
 * It is exported by the HalDispatchTable.
 *
 * VOID
 * HalQuerySystemInformation(VOID);
 */

ULONG STDCALL
HalReadDmaCounter(PADAPTER_OBJECT AdapterObject);

/*
FASTCALL
HalRequestSoftwareInterrupt
*/

ULONG STDCALL
HalSetBusData(BUS_DATA_TYPE BusDataType,
	      ULONG BusNumber,
	      ULONG SlotNumber,
	      PVOID Buffer,
	      ULONG Length);

ULONG STDCALL
HalSetBusDataByOffset(BUS_DATA_TYPE BusDataType,
		      ULONG BusNumber,
		      ULONG SlotNumber,
		      PVOID Buffer,
		      ULONG Offset,
		      ULONG Length);

/*
HalSetProfileInterval
*/

VOID STDCALL
HalSetRealTimeClock(PTIME_FIELDS Time);

/*
HalSetTimeIncrement
*/

/*
HalStartProfileInterrupt
*/

/*
HalStopProfileInterrupt
*/


BOOLEAN STDCALL
HalTranslateBusAddress(INTERFACE_TYPE InterfaceType,
		       ULONG BusNumber,
		       PHYSICAL_ADDRESS BusAddress,
		       PULONG AddressSpace,
		       PPHYSICAL_ADDRESS TranslatedAddress);


/*
 * Port I/O functions
 */

VOID STDCALL
READ_PORT_BUFFER_UCHAR(PUCHAR Port,
		       PUCHAR Value,
		       ULONG Count);

VOID STDCALL
READ_PORT_BUFFER_ULONG(PULONG Port,
		       PULONG Value,
		       ULONG Count);

VOID STDCALL
READ_PORT_BUFFER_USHORT(PUSHORT Port,
			PUSHORT Value,
			ULONG Count);

UCHAR STDCALL
READ_PORT_UCHAR(PUCHAR Port);

ULONG STDCALL
READ_PORT_ULONG(PULONG Port);

USHORT STDCALL
READ_PORT_USHORT(PUSHORT Port);

VOID STDCALL
WRITE_PORT_BUFFER_UCHAR(PUCHAR Port,
			PUCHAR Value,
			ULONG Count);

VOID STDCALL
WRITE_PORT_BUFFER_ULONG(PULONG Port,
			PULONG Value,
			ULONG Count);

VOID STDCALL
WRITE_PORT_BUFFER_USHORT(PUSHORT Port,
			 PUSHORT Value,
			 ULONG Count);

VOID STDCALL
WRITE_PORT_UCHAR(PUCHAR Port,
		 UCHAR Value);

VOID STDCALL
WRITE_PORT_ULONG(PULONG Port,
		 ULONG Value);

VOID STDCALL
WRITE_PORT_USHORT(PUSHORT Port,
		  USHORT Value);

#endif /* __INCLUDE_DDK_HALDDK_H */

/* EOF */
