/*
 *  ReactOS kernel
 *  Copyright (C) 2002,2003 ReactOS Team
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
/* $Id: fs_rec.h,v 1.3 2003/01/16 11:58:15 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/fs_rec/fs_rec.h
 * PURPOSE:          Filesystem recognizer driver
 * PROGRAMMER:       Eric Kohl
 */


/* Filesystem types (add new filesystems here)*/

#define FS_TYPE_UNUSED		0
#define FS_TYPE_VFAT		1
#define FS_TYPE_NTFS		2
#define FS_TYPE_CDFS		3
#define FS_TYPE_UDFS		4


typedef struct _DEVICE_EXTENSION
{
  ULONG FsType;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/* blockdev.c */

NTSTATUS
FsRecReadSectors(IN PDEVICE_OBJECT DeviceObject,
		 IN ULONG DiskSector,
		 IN ULONG SectorCount,
		 IN ULONG SectorSize,
		 IN OUT PUCHAR Buffer);

NTSTATUS
FsRecDeviceIoControl(IN PDEVICE_OBJECT DeviceObject,
		     IN ULONG ControlCode,
		     IN PVOID InputBuffer,
		     IN ULONG InputBufferSize,
		     IN OUT PVOID OutputBuffer,
		     IN OUT PULONG OutputBufferSize);


/* cdfs.c */

NTSTATUS
FsRecCdfsFsControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);


/* fat.c */

NTSTATUS
FsRecVfatFsControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);


/* ntfs.c */

NTSTATUS
FsRecNtfsFsControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);


/* udfs.c */

NTSTATUS
FsRecUdfsFsControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);

/* EOF */
