/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: win32.c,v 1.1 2002/01/04 13:09:11 ekohl Exp $
 *
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/win32.c
 * PURPOSE:                win32k support
 * PROGRAMMER:             Eric Kohl (ekohl@rz-online.de)
 * REVISION HISTORY: 
 *               04/01/2002: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

static ULONG PspWin32ProcessSize = 0;


/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL
PsCreateWin32Process(PEPROCESS Process)
{
  if (Process->Win32Process != NULL)
    return(STATUS_SUCCESS);

  Process->Win32Process = ExAllocatePool(NonPagedPool,
					 PspWin32ProcessSize);
  if (Process->Win32Process == NULL)
    return(STATUS_NO_MEMORY);

  RtlZeroMemory(Process->Win32Process,
		PspWin32ProcessSize);

  return(STATUS_SUCCESS);
}


VOID STDCALL
PsEstablishWin32Callouts(PVOID Param1,
			 PVOID Param2,
			 PVOID Param3,
			 PVOID Param4,
			 PVOID Param5,
			 ULONG W32ProcessSize)
{
  PspWin32ProcessSize = W32ProcessSize;
}

/* EOF */
