#ifndef _FMIFS_H
#define _FMIFS_H
/* $Id: fmifs.h,v 1.1 1999/05/16 07:27:35 ea Exp $
 *
 * fmifs.h
 *
 * Copyright (c) 1998 Mark Russinovich
 * Systems Internals
 * http://www.sysinternals.com
 *
 * Typedefs and definitions for using chkdsk and formatex
 * functions exported by the fmifs.dll library.
 *
 * ---
 *
 * 1999-02-18 (Emanuele Aliberti)
 * 	Normalized function names.
 *
 */
#ifndef _INC_WINDOWS_
#include <windows.h>
#endif

/* Output command */
typedef
struct
{
	DWORD Lines;
	PCHAR Output;
	
} TEXTOUTPUT, *PTEXTOUTPUT;


/* Callback command types */
typedef
enum
{
	PROGRESS,
	DONEWITHSTRUCTURE,
	UNKNOWN2,
	UNKNOWN3,
	UNKNOWN4,
	UNKNOWN5,
	INSUFFICIENTRIGHTS,
	UNKNOWN7,
	UNKNOWN8,
	UNKNOWN9,
	UNKNOWNA,
	DONE,
	UNKNOWNC,
	UNKNOWND,
	OUTPUT,
	STRUCTUREPROGRESS

} CALLBACKCOMMAND;


/* FMIFS callback definition */
typedef
BOOL
(STDCALL * PFMIFSCALLBACK) (
	CALLBACKCOMMAND	Command,
	DWORD		SubAction,
	PVOID		ActionInfo
	);

/* Chkdsk command in FMIFS */
VOID
STDCALL
ChkDsk(
	PWCHAR		DriveRoot, 
	PWCHAR		Format,
	BOOL		CorrectErrors, 
	BOOL		Verbose, 
	BOOL		CheckOnlyIfDirty,
	BOOL		ScanDrive, 
	PVOID		Unused2, 
	PVOID		Unused3,
	PFMIFSCALLBACK	Callback
	);

/* ChkdskEx command in FMIFS (not in the original) */
VOID
STDCALL
ChkDskEx(
	PWCHAR		DriveRoot, 
	PWCHAR		Format,
	BOOL		CorrectErrors, 
	BOOL		Verbose, 
	BOOL		CheckOnlyIfDirty,
	BOOL		ScanDrive, 
	PVOID		Unused2, 
	PVOID		Unused3,
	PFMIFSCALLBACK	Callback
	);

/* DiskCopy command in FMIFS */

VOID
STDCALL
DiskCopy(VOID);

/* Enable/Disable volume compression */
BOOL
STDCALL
EnableVolumeCompression(
	PWCHAR	DriveRoot,
	BOOL	Enable
	);

/* Format command in FMIFS */

/* media flags */
#define FMIFS_HARDDISK 0xC
#define FMIFS_FLOPPY   0x8

VOID
STDCALL
FormatEx(
	PWCHAR		DriveRoot,
	DWORD		MediaFlag,
	PWCHAR		Format,
	PWCHAR		Label,
	BOOL		QuickFormat,
	DWORD		ClusterSize,
	PFMIFSCALLBACK	Callback
	);

#endif /* ndef _FMIFS_H */
