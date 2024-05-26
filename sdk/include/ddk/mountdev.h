/*
 * mountmgr.h
 *
 * Mount Manager mounted devices interface
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

#ifndef _MOUNTDEV_
#define _MOUNTDEV_

#pragma once

#include <mountmgr.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Windows 2003 fixed the required access for some IOCTLs */
#if (NTDDI_VERSION >= NTDDI_WS03)
    #define EXPECTED_ACCESS (FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#else
    #define EXPECTED_ACCESS FILE_ANY_ACCESS
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

#define IOCTL_MOUNTDEV_QUERY_UNIQUE_ID            CTL_CODE(MOUNTDEVCONTROLTYPE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME  CTL_CODE(MOUNTDEVCONTROLTYPE, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTDEV_LINK_CREATED               CTL_CODE(MOUNTDEVCONTROLTYPE, 4, METHOD_BUFFERED, EXPECTED_ACCESS)
#define IOCTL_MOUNTDEV_LINK_DELETED               CTL_CODE(MOUNTDEVCONTROLTYPE, 5, METHOD_BUFFERED, EXPECTED_ACCESS)

typedef struct _MOUNTDEV_UNIQUE_ID {
  USHORT UniqueIdLength;
  UCHAR UniqueId[1];
} MOUNTDEV_UNIQUE_ID, *PMOUNTDEV_UNIQUE_ID;

typedef struct _MOUNTDEV_SUGGESTED_LINK_NAME {
  BOOLEAN UseOnlyIfThereAreNoOtherLinks;
  USHORT NameLength;
  WCHAR Name[1];
} MOUNTDEV_SUGGESTED_LINK_NAME, *PMOUNTDEV_SUGGESTED_LINK_NAME;

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

#define IOCTL_MOUNTDEV_QUERY_STABLE_GUID          CTL_CODE(MOUNTDEVCONTROLTYPE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _MOUNTDEV_STABLE_GUID {
  GUID StableGuid;
} MOUNTDEV_STABLE_GUID, *PMOUNTDEV_STABLE_GUID;

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

/* NOTE: Support for IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY
 * seems to have been removed from official WDK in Vista+ */
#define IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY    CTL_CODE(MOUNTDEVCONTROLTYPE, 1, METHOD_BUFFERED, EXPECTED_ACCESS)
typedef struct _MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT {
  ULONG Size;
  USHORT OldUniqueIdOffset;
  USHORT OldUniqueIdLength;
  USHORT NewUniqueIdOffset;
  USHORT NewUniqueIdLength;
} MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT, *PMOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _MOUNTDEV_ */
