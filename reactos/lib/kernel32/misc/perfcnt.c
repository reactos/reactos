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
/* $Id: perfcnt.c,v 1.2 2003/07/10 18:50:51 chorns Exp $ */
/*
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/perfcnt.c
 * PURPOSE:         Performance counter
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
WINBOOL STDCALL
QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
{
  LARGE_INTEGER Frequency;
  NTSTATUS Status;

  Status = NtQueryPerformanceCounter(lpPerformanceCount,
				     &Frequency);
  if (!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return(FALSE);
  }

  if (Frequency.QuadPart == 0ULL)
  {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
  }

  return(TRUE);
}


/*
 * @implemented
 */
WINBOOL STDCALL
QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency)
{
  LARGE_INTEGER Count;
  NTSTATUS Status;

  Status = NtQueryPerformanceCounter(&Count,
				     lpFrequency);
  if (!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return(FALSE);
  }

  if (lpFrequency->QuadPart == 0ULL)
  {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
  }

  return(TRUE);
}

/* EOF */
