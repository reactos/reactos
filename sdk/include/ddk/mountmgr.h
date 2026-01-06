/*
 * mountmgr.h
 *
 * Mount Manager driver interface
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Magnus Olsen <greatlord@reactos.org>
 *   Amine Khaldi <amine.khaldi@reactos.org>
 *   Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _MOUNTMGR_
#define _MOUNTMGR_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

#define MOUNTMGR_DEVICE_NAME        L"\\Device\\MountPointManager"
#define MOUNTMGR_DOS_DEVICE_NAME    L"\\\\.\\MountPointManager"
#define MOUNTMGRCONTROLTYPE         ((ULONG)'m') /* Mount Manager */
#define MOUNTDEVCONTROLTYPE         ((ULONG)'M') /* Mount Device */

#ifdef DEFINE_GUID
DEFINE_GUID(MOUNTDEV_MOUNTED_DEVICE_GUID, 0x53F5630D, 0xB6BF, 0x11D0, 0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B);
#endif

/* IOCTLs */
#define IOCTL_MOUNTMGR_CREATE_POINT \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 0, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_DELETE_POINTS \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 1, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_QUERY_POINTS \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 3, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 4, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 5, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 6, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 7, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_CHANGE_NOTIFY \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 8, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 9, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 10, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 11, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_MOUNTDEV_QUERY_DEVICE_NAME \
  CTL_CODE(MOUNTDEVCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* ----- safer helper macros ----- */

/* Check for \DosDevices\X: prefix (drive letter) */
#define MOUNTMGR_IS_DRIVE_LETTER(s) \
  ((s) && (s)->Length >= 14 * sizeof(WCHAR) && \
   (s)->Buffer[0] == L'\\' && \
   (s)->Buffer[1] == L'D' && \
   (s)->Buffer[2] == L'o' && \
   (s)->Buffer[3] == L's' && \
   (s)->Buffer[4] == L'D' && \
   (s)->Buffer[5] == L'e' && \
   (s)->Buffer[6] == L'v' && \
   (s)->Buffer[7] == L'i' && \
   (s)->Buffer[8] == L'c' && \
   (s)->Buffer[9] == L'e' && \
   (s)->Buffer[10] == L's' && \
   (s)->Buffer[11] == L'\\' && \
   (s)->Buffer[12] >= L'A' && (s)->Buffer[12] <= L'Z' && \
   (s)->Buffer[13] == L':')

/* Check for \??\Volume{GUID} prefix */
#define MOUNTMGR_IS_VOLUME_NAME(s) \
  ((s)->Length >= 48 * sizeof(WCHAR) && \
   (s)->Buffer[0] == L'\\' && \
   ((s)->Buffer[1] == L'?' || (s)->Buffer[1] == L'\\') && \
   (s)->Buffer[2] == L'?' && \
   (s)->Buffer[3] == L'\\' && \
   (s)->Buffer[4] == L'V' && \
   (s)->Buffer[5] == L'o' && \
   (s)->Buffer[6] == L'l' && \
   (s)->Buffer[7] == L'u' && \
   (s)->Buffer[8] == L'm' && \
   (s)->Buffer[9] == L'e' && \
   (s)->Buffer[10] == L'{' && \
   (s)->Buffer[19] == L'-' && \
   (s)->Buffer[24] == L'-' && \
   (s)->Buffer[29] == L'-' && \
   (s)->Buffer[34] == L'-' && \
   (s)->Buffer[47] == L'}')

/* DOS vs NT volume name helpers */
#define MOUNTMGR_IS_DOS_VOLUME_NAME(s) ((s) && MOUNTMGR_IS_VOLUME_NAME(s) && (s)->Buffer[1] == L'\\')
#define MOUNTMGR_IS_NT_VOLUME_NAME(s) \
    (MOUNTMGR_IS_VOLUME_NAME(s) && (s)->Buffer[1] == L'?')

/* WB volume name helpers */
#define MOUNTMGR_IS_DOS_VOLUME_NAME_WB(s) \
    (MOUNTMGR_IS_VOLUME_NAME(s) && (s)->Length == 98 && (s)->Buffer[1] == L'\\')

#define MOUNTMGR_IS_NT_VOLUME_NAME_WB(s) \
    (MOUNTMGR_IS_VOLUME_NAME(s) && (s)->Length == 98 && (s)->Buffer[1] == L'?')

/* ----- structures ----- */

typedef struct _MOUNTMGR_CREATE_POINT_INPUT {
  USHORT SymbolicLinkNameOffset;
  USHORT SymbolicLinkNameLength;
  USHORT DeviceNameOffset;
  USHORT DeviceNameLength;
} MOUNTMGR_CREATE_POINT_INPUT, *PMOUNTMGR_CREATE_POINT_INPUT;

#pragma pack(push, 1)
typedef struct _MOUNTMGR_MOUNT_POINT {
  ULONG SymbolicLinkNameOffset;
  USHORT SymbolicLinkNameLength;
  USHORT Reserved0;
  ULONG UniqueIdOffset;
  USHORT UniqueIdLength;
  USHORT Reserved1;
  ULONG DeviceNameOffset;
  USHORT DeviceNameLength;
  USHORT Reserved2;
} MOUNTMGR_MOUNT_POINT, *PMOUNTMGR_MOUNT_POINT;

typedef struct _MOUNTMGR_MOUNT_POINTS {
  ULONG Size;
  ULONG NumberOfMountPoints;
  MOUNTMGR_MOUNT_POINT MountPoints[1]; /* flexible array */
} MOUNTMGR_MOUNT_POINTS, *PMOUNTMGR_MOUNT_POINTS;
#pragma pack(pop)

typedef struct _MOUNTMGR_DRIVE_LETTER_TARGET {
  USHORT DeviceNameLength;
  _Field_size_bytes_(DeviceNameLength) WCHAR DeviceName[1]; /* variable-length */
} MOUNTMGR_DRIVE_LETTER_TARGET, *PMOUNTMGR_DRIVE_LETTER_TARGET;

typedef struct _MOUNTMGR_DRIVE_LETTER_INFORMATION {
  BOOLEAN DriveLetterWasAssigned;
  UCHAR CurrentDriveLetter;
} MOUNTMGR_DRIVE_LETTER_INFORMATION, *PMOUNTMGR_DRIVE_LETTER_INFORMATION;

typedef struct _MOUNTMGR_VOLUME_MOUNT_POINT {
  USHORT SourceVolumeNameOffset;
  USHORT SourceVolumeNameLength;
  USHORT TargetVolumeNameOffset;
  USHORT TargetVolumeNameLength;
} MOUNTMGR_VOLUME_MOUNT_POINT, *PMOUNTMGR_VOLUME_MOUNT_POINT;

typedef struct _MOUNTMGR_CHANGE_NOTIFY_INFO {
  ULONG EpicNumber;
} MOUNTMGR_CHANGE_NOTIFY_INFO, *PMOUNTMGR_CHANGE_NOTIFY_INFO;

typedef struct _MOUNTMGR_TARGET_NAME {
  USHORT DeviceNameLength;
  _Field_size_bytes_(DeviceNameLength) WCHAR DeviceName[1]; /* variable-length */
} MOUNTMGR_TARGET_NAME, *PMOUNTMGR_TARGET_NAME;

typedef struct _MOUNTDEV_NAME {
  USHORT NameLength;
  _Field_size_bytes_(NameLength) WCHAR Name[1]; /* variable-length */
} MOUNTDEV_NAME, *PMOUNTDEV_NAME;

#endif /* NTDDI_WIN2K */

#if (NTDDI_VERSION >= NTDDI_WINXP)

#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _MOUNTMGR_VOLUME_PATHS {
  ULONG MultiSzLength;
  WCHAR MultiSz[1]; /* variable-length multi-sz */
} MOUNTMGR_VOLUME_PATHS, *PMOUNTMGR_VOLUME_PATHS;

#endif /* NTDDI_WINXP */

#if (NTDDI_VERSION >= NTDDI_WS03)

#define IOCTL_MOUNTMGR_SCRUB_REGISTRY \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 14, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_QUERY_AUTO_MOUNT \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTMGR_SET_AUTO_MOUNT \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 16, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

typedef enum _MOUNTMGR_AUTO_MOUNT_STATE {
  Disabled = 0,
  Enabled
} MOUNTMGR_AUTO_MOUNT_STATE;

typedef struct _MOUNTMGR_QUERY_AUTO_MOUNT {
  MOUNTMGR_AUTO_MOUNT_STATE CurrentState;
} MOUNTMGR_QUERY_AUTO_MOUNT, *PMOUNTMGR_QUERY_AUTO_MOUNT;

typedef struct _MOUNTMGR_SET_AUTO_MOUNT {
  MOUNTMGR_AUTO_MOUNT_STATE NewState;
} MOUNTMGR_SET_AUTO_MOUNT, *PMOUNTMGR_SET_AUTO_MOUNT;

#endif /* NTDDI_WS03 */

#if (NTDDI_VERSION >= NTDDI_WIN7)

#define IOCTL_MOUNTMGR_BOOT_DL_ASSIGNMENT \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 17, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_TRACELOG_CACHE \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 18, METHOD_BUFFERED, FILE_READ_ACCESS)

#endif /* NTDDI_WIN7 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _MOUNTMGR_ */
