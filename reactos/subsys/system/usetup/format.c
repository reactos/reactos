/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: format.c,v 1.1 2003/04/28 19:44:13 chorns Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/format.c
 * PURPOSE:         Filesystem format support functions
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#define NDEBUG
#include <debug.h>
#include <fslib/vfatlib.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS
FormatPartition(PUNICODE_STRING DriveRoot)
{
  NTSTATUS Status;

  VfatInitialize();

  Status = VfatFormat(DriveRoot,
  	0,    // MediaFlag
  	NULL, // Label
  	TRUE, // QuickFormat
  	0,    // ClusterSize
  	NULL); // Callback
  DPRINT("VfatFormat() status 0x%.08x\n", Status);

  VfatCleanup();

  return Status;
}

/* EOF */
