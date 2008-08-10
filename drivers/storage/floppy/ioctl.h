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
 * FILE:            ioctl.h
 * PURPOSE:         Header for IOCTL routines
 * PROGRAMMER:      Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *                  15-Feb-2004 vizzini - Created
 */

DRIVER_DISPATCH DeviceIoctl;
NTSTATUS NTAPI DeviceIoctl(PDEVICE_OBJECT DeviceObject,
                           PIRP Irp);

VOID NTAPI DeviceIoctlPassive(PDRIVE_INFO DriveInfo,
                              PIRP Irp);

