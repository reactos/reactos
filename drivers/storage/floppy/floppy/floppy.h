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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * PROJECT:         ReactOS Floppy Driver
 * FILE:            floppy.h
 * PURPOSE:         Header for Main floppy driver routines
 * PROGRAMMER:      Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *                  15-Feb-2004 vizzini - Created
 */

#pragma once

#define MAX_DEVICE_NAME 255
#define MAX_ARC_PATH_LEN 255
#define MAX_DRIVES_PER_CONTROLLER 4
#define MAX_CONTROLLERS 4

/* MS doesn't prototype this but the w2k kernel exports it */
int _cdecl swprintf(const WCHAR *, ...);

/* need ioctls in ddk build mode */
#include <ntdddisk.h>

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
    WCHAR                    ArcPathBuffer[MAX_ARC_PATH_LEN];
    WCHAR                    DeviceNameBuffer[MAX_DEVICE_NAME];
    ULONG                    DiskChangeCount;
    BOOLEAN                  Initialized;
} DRIVE_INFO, *PDRIVE_INFO;

typedef struct _CONTROLLER_INFO
{
    BOOLEAN          Populated;
    BOOLEAN          Initialized;
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

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

VOID NTAPI
SignalMediaChanged(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI
WaitForControllerInterrupt(PCONTROLLER_INFO ControllerInfo, PLARGE_INTEGER Timeout);

NTSTATUS NTAPI
ResetChangeFlag(PDRIVE_INFO DriveInfo);

VOID NTAPI
StartMotor(PDRIVE_INFO DriveInfo);

VOID NTAPI
StopMotor(PCONTROLLER_INFO ControllerInfo);

/*
 * MEDIA TYPES
 *
 * This table was found at https://web.archive.org/web/20021207232702/http://www.nondot.org/sabre/os/files/Disk/FloppyMediaIDs.txt .
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
