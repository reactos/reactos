#ifndef __INCLUDE_DDK_KDFUNCS_H
#define __INCLUDE_DDK_KDFUNCS_H
/* $Id: kdfuncs.h,v 1.6 2002/05/08 17:05:32 chorns Exp $ */

/* --- NTOSKRNL.EXE --- */
#if defined(__NTOSKRNL__)
extern BOOLEAN KdDebuggerEnabled __declspec(dllexport);
extern BOOLEAN KdDebuggerNotPresent __declspec(dllexport);
#else
extern BOOLEAN KdDebuggerEnabled __declspec(dllimport);
extern BOOLEAN KdDebuggerNotPresent __declspec(dllimport);
#endif

BYTE
STDCALL
KdPollBreakIn (
	VOID
	);

BOOLEAN
STDCALL
KdPortInitialize (
	PKD_PORT_INFORMATION	PortInformation,
	DWORD	Unknown1,
	DWORD	Unknown2
	);

BOOLEAN
STDCALL
KdPortInitializeEx (
	PKD_PORT_INFORMATION	PortInformation,
	DWORD	Unknown1,
	DWORD	Unknown2
	);

BOOLEAN
STDCALL
KdPortGetByte (
	PUCHAR	ByteRecieved
	);

BOOLEAN
STDCALL
KdPortGetByteEx (
	PKD_PORT_INFORMATION	PortInformation,
	PUCHAR	ByteRecieved
	);

BOOLEAN
STDCALL
KdPortPollByte (
	PUCHAR	ByteRecieved
	);

BOOLEAN
STDCALL
KdPortPollByteEx (
	PKD_PORT_INFORMATION	PortInformation,
	PUCHAR	ByteRecieved
	);

VOID
STDCALL
KdPortPutByte (
	UCHAR ByteToSend
	);

VOID
STDCALL
KdPortPutByteEx (
	PKD_PORT_INFORMATION	PortInformation,
	UCHAR ByteToSend
	);

VOID
STDCALL
KdPortRestore (
	VOID
	);

VOID
STDCALL
KdPortSave (
	VOID
	);

BOOLEAN
STDCALL
KdPortDisableInterrupts(
  VOID
  );

BOOLEAN
STDCALL
KdPortEnableInterrupts(
  VOID
  );

#endif /* __INCLUDE_DDK_KDFUNCS_H */
