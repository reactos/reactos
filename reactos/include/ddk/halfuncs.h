#ifndef __INCLUDE_DDK_HALFUNCS_H
#define __INCLUDE_DDK_HALFUNCS_H
/* $Id: halfuncs.h,v 1.6 2003/06/07 10:14:39 chorns Exp $ */

#include <ntos/haltypes.h>

VOID STDCALL
HalAcquireDisplayOwnership(IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters);

NTSTATUS STDCALL
HalAdjustResourceList(PCM_RESOURCE_LIST	Resources);

NTSTATUS STDCALL
HalAllocateAdapterChannel(IN PADAPTER_OBJECT AdapterObject,
			  IN PDEVICE_OBJECT DeviceObject,
			  IN ULONG NumberOfMapRegisters,
			  IN PDRIVER_CONTROL ExecutionRoutine,
			  IN PVOID Context);

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

VOID STDCALL
HalCalibratePerformanceCounter(ULONG Count);

/*
FASTCALL
HalClearSoftwareInterrupt
*/

VOID STDCALL
HalDisplayString(IN PCH String);

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

BOOLEAN STDCALL
HalFlushCommonBuffer(ULONG Unknown1,
		     ULONG Unknown2,
		     ULONG Unknown3,
		     ULONG Unknown4,
		     ULONG Unknown5,
		     ULONG Unknown6,
		     ULONG Unknown7,
		     ULONG Unknown8);

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

/* Is this function really exported ?? */
ULONG
HalGetDmaAlignmentRequirement(VOID);

BOOLEAN STDCALL
HalGetEnvironmentVariable(IN PCH Name,
			  OUT PCH Value,
			  IN USHORT ValueLength);

ULONG STDCALL
HalGetInterruptVector(INTERFACE_TYPE InterfaceType,
		      ULONG BusNumber,
		      ULONG BusInterruptLevel,
		      ULONG BusInterruptVector,
		      PKIRQL Irql,
		      PKAFFINITY Affinity);

BOOLEAN STDCALL
HalMakeBeep(ULONG Frequency);

VOID STDCALL
HalQueryDisplayParameters(PULONG DispSizeX,
			  PULONG DispSizeY,
			  PULONG CursorPosX,
			  PULONG CursorPosY);

VOID STDCALL
HalQueryRealTimeClock(PTIME_FIELDS Time);

/*
 * HalQuerySystemInformation() is not exported explicitly.
 * It is exported by the HalDispatchTable.
 *
 * VOID
 * HalQuerySystemInformation(VOID);
 */

ULONG STDCALL
HalReadDmaCounter(PADAPTER_OBJECT AdapterObject);

VOID STDCALL
HalRequestIpi(ULONG Unknown);

/*
FASTCALL
HalRequestSoftwareInterrupt
*/

VOID STDCALL
HalReturnToFirmware(ULONG Action);

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

VOID STDCALL
HalSetDisplayParameters(ULONG CursorPosX,
			ULONG CursorPosY);

BOOLEAN STDCALL
HalSetEnvironmentVariable(IN PCH Name,
			  IN PCH Value);

/*
HalSetProfileInterval
*/

VOID STDCALL
HalSetRealTimeClock(PTIME_FIELDS Time);

/*
HalSetTimeIncrement
*/

BOOLEAN STDCALL
HalStartNextProcessor(ULONG Unknown1,
		      ULONG Unknown2);

/*
HalStartProfileInterrupt
*/

/*
HalStopProfileInterrupt
*/

ULONG FASTCALL
HalSystemVectorDispatchEntry(ULONG Unknown1,
			     ULONG Unknown2,
			     ULONG Unknown3);

BOOLEAN STDCALL
HalTranslateBusAddress(INTERFACE_TYPE InterfaceType,
		       ULONG BusNumber,
		       PHYSICAL_ADDRESS BusAddress,
		       PULONG AddressSpace,
		       PPHYSICAL_ADDRESS TranslatedAddress);


/*
 * Kernel debugger support functions
 */

BOOLEAN STDCALL
KdPortInitialize(PKD_PORT_INFORMATION PortInformation,
		 DWORD Unknown1,
		 DWORD Unknown2);

BOOLEAN STDCALL
KdPortGetByte(PUCHAR ByteRecieved);

BOOLEAN STDCALL
KdPortPollByte(PUCHAR ByteRecieved);

VOID STDCALL
KdPortPutByte(UCHAR ByteToSend);


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
