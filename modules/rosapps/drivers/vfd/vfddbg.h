/*
	vfddbg.h

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver: debug functions header

	Copyright (C) 2003-2005 Ken Kato
*/

#ifndef _VFDDBG_H_
#define _VFDDBG_H_

#if DBG

#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus

PCSTR
GetStatusName(
	NTSTATUS			status);

PCSTR
GetMajorFuncName(
	UCHAR				major_code);

PCSTR
GetIoControlName(
	ULONG				ctrl_code);

#ifdef VFD_PNP

PCSTR
GetPnpIrpName(
	ULONG				minor_code);

PCSTR
GetPowerIrpName(
	ULONG				minor_code);

PCSTR
GetSystemIrpName(
	ULONG				minor_code);

#endif	// VFD_PNP

//
//	Debug Trace Level Flags
//
#define VFDERR			0x00000000
#define VFDWARN			0x00000001
#define VFDINFO			0x00000003

#define VFDDEV			0x00000004
#define VFDDRV			0x00000008
#define VFDRDWR			0x00000010
#define VFDIMG			0x00000020
#define VFDLINK			0x00000040
#define VFDFMT			0x00000080
#define VFDCTL			0x00000100
#define VFDMNT			0x00000200
#define VFDPNP			0x00000400

#define VFDTRACE(LEVEL,STRING)					\
	if ((TraceFlags & (LEVEL)) == (LEVEL)) {	\
		DbgPrint STRING;						\
	}

extern ULONG TraceFlags;

#else	// DBG
#define VFDTRACE(LEVEL,STRING)
#endif	// DBG

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif	// _VFDDBG_H_
