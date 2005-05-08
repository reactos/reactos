/* $Id: setypes.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/obtypes.h
 * PURPOSE:         Defintions for Object Manager Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _OBTYPES_H
#define _OBTYPES_H

#ifndef _NTIFS_H
typedef struct _DEVICE_MAP *PDEVICE_MAP;
#endif

extern POBJECT_TYPE ObDirectoryType;
extern PDEVICE_MAP ObSystemDeviceMap;

/* Values for DosDeviceDriveType */
#define DOSDEVICE_DRIVE_UNKNOWN		0
#define DOSDEVICE_DRIVE_CALCULATE	1
#define DOSDEVICE_DRIVE_REMOVABLE	2
#define DOSDEVICE_DRIVE_FIXED		3
#define DOSDEVICE_DRIVE_REMOTE		4
#define DOSDEVICE_DRIVE_CDROM		5
#define DOSDEVICE_DRIVE_RAMDISK		6

typedef struct _OBJECT_BASIC_INFORMATION {
  ULONG Attributes;
  ACCESS_MASK GrantedAccess;
  ULONG HandleCount;
  ULONG PointerCount;
  ULONG PagedPoolUsage;
  ULONG NonPagedPoolUsage;
  ULONG Reserved[3];
  ULONG NameInformationLength;
  ULONG TypeInformationLength;
  ULONG SecurityDescriptorLength;
  LARGE_INTEGER CreateTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

/* FIXME: Use Windows Type!! */
typedef struct _HANDLE_TABLE {
    LIST_ENTRY ListHead;
    KSPIN_LOCK ListLock;
} HANDLE_TABLE;

/* FIXME: Use Windows Type!! */
typedef struct _OBJECT_HEADER {
/*
 * PURPOSE: Header for every object managed by the object manager
 */
    UNICODE_STRING Name;
    LIST_ENTRY Entry;
    LONG RefCount;
    LONG HandleCount;
    BOOLEAN CloseInProcess;
    BOOLEAN Permanent;
    BOOLEAN Inherit;
    struct _DIRECTORY_OBJECT* Parent;
    POBJECT_TYPE ObjectType;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
   
   /*
    * PURPOSE: Object type
    * NOTE: This overlaps the first member of the object body
    */
    CSHORT Type;
   
   /*
    * PURPOSE: Object size
    * NOTE: This overlaps the second member of the object body
    */
    CSHORT Size;
} OBJECT_HEADER, *POBJECT_HEADER;

#endif
