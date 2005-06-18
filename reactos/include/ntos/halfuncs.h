#ifndef __INCLUDE_NTOS_HALFUNCS_H
#define __INCLUDE_NTOS_HALFUNCS_H

#include <ntos/haltypes.h>

NTSTATUS STDCALL
HalAdjustResourceList(PCM_RESOURCE_LIST	Resources);

BOOLEAN STDCALL
HalAllProcessorsStarted(VOID);

VOID
STDCALL
HalDisplayString (
    IN PCHAR String
);

BOOLEAN STDCALL
HalBeginSystemInterrupt(ULONG Vector,
  KIRQL Irql,
  PKIRQL OldIrql);

VOID STDCALL
HalCalibratePerformanceCounter(ULONG Count);

BOOLEAN STDCALL
HalDisableSystemInterrupt(ULONG Vector,
  KIRQL Irql);

VOID STDCALL
HalDisplayString(IN PCH String);

BOOLEAN STDCALL
HalEnableSystemInterrupt(ULONG Vector,
  KIRQL Irql,
  KINTERRUPT_MODE InterruptMode);

VOID STDCALL
HalEndSystemInterrupt(KIRQL Irql,
  ULONG Unknown2);

BOOLEAN STDCALL
HalFlushCommonBuffer(ULONG Unknown1,
		     ULONG Unknown2,
		     ULONG Unknown3,
		     ULONG Unknown4,
		     ULONG Unknown5,
		     ULONG Unknown6,
		     ULONG Unknown7,
		     ULONG Unknown8);

BOOLEAN STDCALL
HalGetEnvironmentVariable(IN PCH Name,
			  OUT PCH Value,
			  IN USHORT ValueLength);

VOID STDCALL
HalInitializeProcessor(ULONG ProcessorNumber,
  PVOID ProcessorStack);

BOOLEAN STDCALL
HalInitSystem(ULONG BootPhase,
  PLOADER_PARAMETER_BLOCK LoaderBlock);

VOID STDCALL
HalQueryDisplayParameters(PULONG DispSizeX,
			  PULONG DispSizeY,
			  PULONG CursorPosX,
			  PULONG CursorPosY);

VOID STDCALL
HalQueryRealTimeClock(PTIME_FIELDS Time);

VOID STDCALL
HalReportResourceUsage(VOID);

VOID STDCALL
HalRequestIpi(ULONG Unknown);

VOID STDCALL
HalSetDisplayParameters(ULONG CursorPosX,
			ULONG CursorPosY);

BOOLEAN STDCALL
HalSetEnvironmentVariable(IN PCH Name,
			  IN PCH Value);

ULONG FASTCALL
HalSystemVectorDispatchEntry(ULONG Unknown1,
			     ULONG Unknown2,
			     ULONG Unknown3);

VOID
STDCALL
IoAssignDriveLetters(IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
  IN  PSTRING NtDeviceName,
  OUT PUCHAR NtSystemPath,
  OUT PSTRING NtSystemPathString);

KIRQL
STDCALL
KeRaiseIrqlToSynchLevel(VOID);

VOID STDCALL
HalReturnToFirmware(ULONG Action);

VOID FASTCALL
HalRequestSoftwareInterrupt(KIRQL SoftwareInterruptRequested);

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

/* Non-standard functions */
VOID STDCALL
HalReleaseDisplayOwnership();

BOOLEAN STDCALL
HalQueryDisplayOwnership();

#endif /* __INCLUDE_NTOS_HALDDK_H */

/* EOF */
