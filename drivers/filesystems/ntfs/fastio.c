/*
 *  ReactOS kernel
 *  Copyright (C) 2008 ReactOS Team
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/fastio.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Pierre Schweitzer       
 */

/* INCLUDES *****************************************************************/

#include <ntddk.h>

#define NDEBUG
#include <debug.h>

#include "ntfs.h"


/* FUNCTIONS ****************************************************************/

BOOLEAN NTAPI
NtfsAcqLazyWrite(PVOID Context,
                 BOOLEAN Wait)
{
  UNIMPLEMENTED;
  return FALSE;
}

VOID NTAPI
NtfsRelLazyWrite(PVOID Context)
{
  UNIMPLEMENTED;
}

BOOLEAN NTAPI
NtfsAcqReadAhead(PVOID Context,
                 BOOLEAN Wait)
{
  UNIMPLEMENTED;
  return FALSE;
}

VOID NTAPI
NtfsRelReadAhead(PVOID Context)
{
  UNIMPLEMENTED;
}
