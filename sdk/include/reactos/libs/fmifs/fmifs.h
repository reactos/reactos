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
 *     Normalized function names.
 *
 * ---
 *
 * 2006-09-04 (Hervé Poussineau)
 *     Add some prototypes
 *
 */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* Output command */
typedef struct
{
    ULONG Lines;
    PCHAR Output;
} TEXTOUTPUT, *PTEXTOUTPUT;

/* Device information */
typedef struct _DEVICE_INFORMATION
{
    ULONG DeviceFlags;
    ULONG SectorSize;
    LARGE_INTEGER SectorCount;
} DEVICE_INFORMATION, *PDEVICE_INFORMATION;

/* Device information flags */
#define MEMORYSTICK_FORMAT_CAPABLE 0x10
#define MEMORYSTICK_SUPPORTS_PROGRESS_BAR 0x20
#define DEVICE_HOTPLUG 0x40
#define DEVICE_MEMORYSTICK 0x41

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

/* ChkdskEx command in FMIFS */
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
Format(
    IN PWCHAR DriveRoot,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PWCHAR Format,
    IN PWCHAR Label,
    IN BOOLEAN QuickFormat,
    IN PFMIFSCALLBACK Callback);

/* FormatEx command in FMIFS */
VOID NTAPI
FormatEx(
    IN PWCHAR DriveRoot,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PWCHAR Format,
    IN PWCHAR Label,
    IN BOOLEAN QuickFormat,
    IN ULONG ClusterSize,
    IN PFMIFSCALLBACK Callback);

/* FormatEx2 command in FMIFS */
// FIXME!

BOOLEAN NTAPI
QueryAvailableFileSystemFormat(
    IN DWORD Index,
    IN OUT PWCHAR FileSystem, /* FIXME: Probably one minimal size is mandatory, but which one? */
    OUT UCHAR* Major,
    OUT UCHAR* Minor,
    OUT BOOLEAN* LatestVersion);

BOOL
NTAPI
QueryDeviceInformation(
    _In_ PWCHAR DriveRoot,
    _Out_ PVOID DeviceInformation,
    _In_ ULONG BufferSize);

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

typedef BOOLEAN
(NTAPI *PULIB_CHKDSK)(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PVOID pUnknown1,
    IN PVOID pUnknown2,
    IN PVOID pUnknown3,
    IN PVOID pUnknown4,
    IN PULONG ExitStatus);

// FIXME: PULIB_CHKDSKEX of u*.dll works with ChkdskEx() of FMIFS.DLL

typedef BOOLEAN
(NTAPI *PULIB_FORMAT)(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize);

// FIXME: PULIB_FORMATEX of u*.dll works with FormatEx2() of FMIFS.DLL

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* ndef _FMIFS_H */
