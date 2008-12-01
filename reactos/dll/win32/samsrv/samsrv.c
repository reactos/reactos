/*
 *  SAM Server DLL
 *  Copyright (C) 2005 Eric Kohl
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

/* INCLUDES *****************************************************************/

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

//#include <samsrv.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

NTSTATUS WINAPI
SamIInitialize(VOID)
{
  DPRINT1("SamIInitialize() called\n");

  return STATUS_SUCCESS;
}


NTSTATUS WINAPI
SampInitializeRegistry(VOID)
{
  DPRINT1("SampInitializeRegistry() called\n");

  return STATUS_SUCCESS;
}

/* EOF */
