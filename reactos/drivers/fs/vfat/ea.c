/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: ea.c,v 1.1 2002/08/14 20:58:31 dwelch Exp $
 *
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/ea.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)

 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>
#include <limits.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS 
VfatSetExtendedAttributes(PFILE_OBJECT FileObject, 
			  PVOID Ea,
			  ULONG EaLength)
{
  return(STATUS_EAS_NOT_SUPPORTED);
}
