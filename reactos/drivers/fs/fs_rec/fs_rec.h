/*
    This is a File System Recognizer for RomFs.
    Copyright (C) 2001 Bo Brantén.
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


//
// Registry path for the FSD and this driver
//

#define FSD_REGISTRY_PATH \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\RomFs"

#define FSR_REGISTRY_PATH \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\RomFsRec"


/* Filesystem types (add new filesystems here)*/
#define FS_TYPE_UNUSED		0
#define FS_TYPE_VFAT		1
#define FS_TYPE_NTFS		2
#define FS_TYPE_CDFS		3



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


/* cdfs.c */

NTSTATUS
FsRecCdfsFsControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);


/* ntfs.c */

NTSTATUS
FsRecNtfsFsControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);


/* vfat.c */

NTSTATUS
FsRecVfatFsControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp);

/* EOF */
