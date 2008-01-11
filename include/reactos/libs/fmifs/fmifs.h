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
	FMIFS_REMOVABLE,
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
	FSNOTSUPPORTED,
	VOLUMEINUSE,
	UNKNOWN9,
	UNKNOWNA,
	DONE,
	UNKNOWNC,
	UNKNOWND,
	OUTPUT,
	STRUCTUREPROGRESS,
	CLUSTERSIZETOOSMALL,
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
	IN FMIFS_MEDIA_FLAG MediaFlag,
	IN PWCHAR Format,
	IN PWCHAR Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback);

BOOLEAN NTAPI
QueryAvailableFileSystemFormat(
	IN DWORD Index,
	IN OUT PWCHAR FileSystem, /* FIXME: Probably one minimal size is mandatory, but which one? */
	OUT UCHAR* Major,
	OUT UCHAR* Minor,
	OUT BOOLEAN* LastestVersion);

BOOL NTAPI
QueryDeviceInformation(
	IN PWCHAR DriveRoot,
	OUT ULONG* Buffer, /* That is probably some 4-bytes structure */
	IN ULONG BufferSize); /* 4 */

BOOL NTAPI
QueryFileSystemName(
	IN PWCHAR DriveRoot,
	OUT PWCHAR FileSystem OPTIONAL, /* FIXME: Probably one minimal size is mandatory, but which one? */
	OUT UCHAR* Unknown2 OPTIONAL, /* Always 0? */
	OUT UCHAR* Unknown3 OPTIONAL, /* Always 0? */
	OUT ULONG* Unknown4 OPTIONAL); /* Always 0? */

BOOLEAN NTAPI
QueryLatestFileSystemVersion(
	IN PWCHAR FileSystem,
	OUT UCHAR* Major OPTIONAL,
	OUT UCHAR* Minor OPTIONAL);

BOOL NTAPI
QuerySupportedMedia(
	IN PWCHAR DriveRoot,
	OUT FMIFS_MEDIA_FLAG *CurrentMedia OPTIONAL,
	IN ULONG Unknown3,
	OUT PULONG Unknown4); /* Always 1? */

BOOL NTAPI
SetLabel(
	IN PWCHAR DriveRoot,
	IN PWCHAR Label);

/* Functions provided by u*.dll */

typedef NTSTATUS
(NTAPI *FORMATEX)(
	IN PUNICODE_STRING DriveRoot,
	IN FMIFS_MEDIA_FLAG MediaFlag,
	IN PUNICODE_STRING Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback);

typedef NTSTATUS
(NTAPI *CHKDSKEX)(
	IN PUNICODE_STRING DriveRoot,
	IN BOOLEAN FixErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PFMIFSCALLBACK Callback);

#endif /* ndef _FMIFS_H */
