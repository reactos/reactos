/*
 *  ReactOS Floppy Driver
 *  Copyright (C) 2004, Vizzini (vizzini@plasmic.com)
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
 *
 * PROJECT:         ReactOS Floppy Driver
 * FILE:            floppy.h
 * PURPOSE:         Header for Main floppy driver routines
 * PROGRAMMER:      Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *                  15-Feb-2004 vizzini - Created
 */

#define MAX_DEVICE_NAME 255
#define MAX_ARC_PATH_LEN 255
#define MAX_DRIVES_PER_CONTROLLER 4
#define MAX_CONTROLLERS 4

#ifdef _MSC_VER
/* MS doesn't prototype this but the w2k kernel exports it */
int _cdecl swprintf(const WCHAR *, ...);

/* need ioctls in ddk build mode */
#include <ntdddisk.h>
#endif

/* missing from ros headers */
/* TODO: fix this right */
#ifndef KdPrint
#define KdPrint(x) DbgPrint x
#endif

#ifndef ASSERT
#define ASSERT(x) { if(!(x)) __asm__("int $3\n"); }
#endif

#ifndef assert
#define assert(x) ASSERT(x)
#endif

#ifndef PAGED_CODE
#define PAGED_CODE() {ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);}
#endif

struct _CONTROLLER_INFO;

typedef struct _DRIVE_INFO
{
  struct _CONTROLLER_INFO  *ControllerInfo;
  UCHAR                    UnitNumber; /* 0,1,2,3 */
  LARGE_INTEGER            MotorStartTime;
  PDEVICE_OBJECT           DeviceObject;
  CM_FLOPPY_DEVICE_DATA    FloppyDeviceData;
  DISK_GEOMETRY            DiskGeometry;
  UCHAR                    BytesPerSectorCode;
  WCHAR                    SymLinkBuffer[MAX_DEVICE_NAME];
  ULONG                    DiskChangeCount;
} DRIVE_INFO, *PDRIVE_INFO;

typedef struct _CONTROLLER_INFO
{
  BOOLEAN          Populated;
  ULONG            ControllerNumber;
  INTERFACE_TYPE   InterfaceType;
  ULONG            BusNumber;
  ULONG            Level;
  KIRQL            MappedLevel;
  ULONG            Vector;
  ULONG            MappedVector;
  KINTERRUPT_MODE  InterruptMode;
  PUCHAR           BaseAddress;
  ULONG            Dma;
  ULONG            MapRegisters;
  PVOID            MapRegisterBase;
  BOOLEAN          Master;
  KEVENT           SynchEvent;
  KDPC             Dpc;
  PKINTERRUPT      InterruptObject;
  PADAPTER_OBJECT  AdapterObject;
  UCHAR            NumberOfDrives;
  BOOLEAN          ImpliedSeeks;
  DRIVE_INFO       DriveInfo[MAX_DRIVES_PER_CONTROLLER];
  PDRIVE_INFO      CurrentDrive;
  BOOLEAN          Model30;
  KEVENT           MotorStoppedEvent;
  KTIMER           MotorTimer;
  KDPC             MotorStopDpc;
  BOOLEAN          StopDpcQueued;
} CONTROLLER_INFO, *PCONTROLLER_INFO;

NTSTATUS NTAPI ReadWrite(PDEVICE_OBJECT DeviceObject,
                                PIRP Irp);

VOID NTAPI QueueThread(PVOID Context);

extern KEVENT QueueThreadTerminate;

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject,
                           PUNICODE_STRING RegistryPath);

VOID NTAPI SignalMediaChanged(PDEVICE_OBJECT DeviceObject,
                              PIRP Irp);

/*
 * MEDIA TYPES
 *
 * This table was found at http://www.nondot.org/sabre/os/files/Disk/FloppyMediaIDs.txt.  
 * Thanks to raster@indirect.com for this information.
 *
 * Format   Size   Cyls   Heads  Sec/Trk   FATs   Sec/FAT   Sec/Root   Media
 * 160K     5 1/4   40      1       8       2        ?         ?        FE
 * 180K     5 1/4   40      1       9       2        ?         4        FC
 * 320K     5 1/4   40      2       8       2        ?         ?        FF
 * 360K     5 1/4   40      2       9       2        4         7        FD
 * 1.2M     5 1/4   80      2      15       2       14        14        F9
 * 720K     3 1/2   80      2       9       2        6         7        F9
 * 1.44M    3 1/2   80      2      18       2       18        14        F0
 */

#define GEOMETRY_144_MEDIATYPE F3_1Pt44_512
#define GEOMETRY_144_CYLINDERS 80
#define GEOMETRY_144_TRACKSPERCYLINDER 2
#define GEOMETRY_144_SECTORSPERTRACK 18
#define GEOMETRY_144_BYTESPERSECTOR 512

VOID NTAPI WaitForControllerInterrupt(PCONTROLLER_INFO ControllerInfo);

NTSTATUS NTAPI ResetChangeFlag(PDRIVE_INFO DriveInfo);

VOID NTAPI StartMotor(PDRIVE_INFO DriveInfo);
VOID NTAPI StopMotor(PCONTROLLER_INFO ControllerInfo);

