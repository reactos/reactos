#ifndef _FMIFS_H
#define _FMIFS_H
/*
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
 * ---
 *
 * 2006-09-04 (Hervé Poussineau)
 * 	Add some prototypes
 *
 */

/* Output command */
typedef struct
{
	ULONG Lines;
	PCHAR Output;
} TEXTOUTPUT, *PTEXTOUTPUT;

/* media flags */
typedef enum
{
	FMIFS_UNKNOWN0,
	FMIFS_UNKNOWN1,
	FMIFS_UNKNOWN2,
	FMIFS_UNKNOWN3,
	FMIFS_UNKNOWN4,
	FMIFS_UNKNOWN5,
	FMIFS_UNKNOWN6,
	FMIFS_UNKNOWN7,
	FMIFS_FLOPPY,
	FMIFS_UNKNOWN9,
	FMIFS_UNKNOWN10,
	FMIFS_UNKNOWN11,
	FMIFS_HARDDISK,
	FMIFS_UNKNOWN13,
	FMIFS_UNKNOWN14,
	FMIFS_UNKNOWN15,
	FMIFS_UNKNOWN16,
	FMIFS_UNKNOWN17,
	FMIFS_UNKNOWN18,
	FMIFS_UNKNOWN19,
	FMIFS_UNKNOWN20,
	FMIFS_UNKNOWN21,
	FMIFS_UNKNOWN22,
	FMIFS_UNKNOWN23,
} FMIFS_MEDIA_FLAG;

/* Callback command types */
typedef enum
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
typedef BOOLEAN
(NTAPI* PFMIFSCALLBACK)(
	IN CALLBACKCOMMAND Command,
	IN ULONG SubAction,
	IN PVOID ActionInfo);

/* Chkdsk command in FMIFS */
VOID NTAPI
Chkdsk(
	IN PWCHAR DriveRoot,
	IN PWCHAR Format,
	IN BOOLEAN CorrectErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PVOID Unused2,
	IN PVOID Unused3,
	IN PFMIFSCALLBACK Callback);

/* ChkdskEx command in FMIFS (not in the original) */
VOID NTAPI
ChkdskEx(
	IN PWCHAR DriveRoot,
	IN PWCHAR Format,
	IN BOOLEAN CorrectErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PVOID Unused2,
	IN PVOID Unused3,
	IN PFMIFSCALLBACK Callback);

FMIFS_MEDIA_FLAG NTAPI
ComputeFmMediaType(
	IN ULONG MediaType);

/* DiskCopy command in FMIFS */
VOID NTAPI
DiskCopy(VOID);

/* Enable/Disable volume compression */
BOOLEAN NTAPI
EnableVolumeCompression(
	IN PWCHAR DriveRoot,
	IN USHORT Compression);

/* Format command in FMIFS */
VOID NTAPI
FormatEx(
	IN PWCHAR DriveRoot,
	IN ULONG MediaFlag,
	IN PWCHAR Format,
	IN PWCHAR Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback);

BOOLEAN NTAPI
QueryAvailableFileSystemFormat(
	IN DWORD Index,
	IN OUT PWCHAR FileSystem OPTIONAL, /* FIXME: Probably one minimal size is mandatory, but which one? */
	OUT UCHAR* Major OPTIONAL,
	OUT UCHAR* Minor OPTIONAL,
	OUT BOOLEAN* LastestVersion OPTIONAL);

BOOL NTAPI
QueryDeviceInformation(
	IN PWCHAR DriveRoot,
	OUT ULONG* Buffer, /* That is probably some 4-bytes structure */
	IN ULONG BufferSize); /* 4 */

BOOLEAN NTAPI
QueryLatestFileSystemVersion(
	IN PWCHAR FileSystem,
	OUT UCHAR* Major OPTIONAL,
	OUT UCHAR* Minor OPTIONAL);

/*ULONG NTAPI
QuerySupportedMedia(
	PVOID Unknown1,
	ULONG Unknown2,
	ULONG Unknown3,
	ULONG Unknown4);*/

#endif /* ndef _FMIFS_H */
