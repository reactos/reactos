#ifndef __INCLUDE_DDK_KDFUNCS_H
#define __INCLUDE_DDK_KDFUNCS_H
/* $Id: kdfuncs.h,v 1.1 2000/02/26 22:41:34 ea Exp $ */

/* --- NTOSKRNL.EXE --- */
extern BOOLEAN KdDebuggerEnabled;
extern BOOLEAN KdDebuggerNotPresent;

BYTE
STDCALL
KdPollBreakIn (
	VOID
	);

/* --- HAL.DLL --- */
extern ULONG KdComPortInUse;

BOOLEAN
STDCALL
KdPortInitialize (
	PKD_PORT_INFORMATION	PortInformation,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
BYTE
STDCALL
KdPortGetByte (
	VOID
	);
BYTE
STDCALL
KdPortPollByte (
	VOID
	);
VOID
STDCALL
KdPortPutByte (
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

#endif /* __INCLUDE_DDK_KDFUNCS_H */
