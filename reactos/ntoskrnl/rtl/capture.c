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
/* $Id: capture.c,v 1.4 2002/09/08 10:23:41 chorns Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/capture.c
 * PURPOSE:         Helper routines for system calls.
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                02/09/01: Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/safe.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
RtlCaptureUnicodeString(PUNICODE_STRING Dest,
			PUNICODE_STRING UnsafeSrc)
{
  PUNICODE_STRING Src;
  NTSTATUS Status;

  /*
   * Copy the source string structure to kernel space.
   */
  Status = MmCopyFromCaller(&Src, UnsafeSrc, sizeof(UNICODE_STRING));
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  /*
   * Initialize the destination string.
   */
  Dest->Length = Src->Length;
  Dest->MaximumLength = Src->MaximumLength;
  Dest->Buffer = ExAllocatePool(NonPagedPool, Dest->MaximumLength);
  if (Dest->Buffer == NULL)
    {
      return(STATUS_NO_MEMORY);
    }

  /*
   * Copy the source string to kernel space.
   */
  Status = MmCopyFromCaller(Dest->Buffer, Src->Buffer, Dest->Length);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(Dest->Buffer);
      return(Status);
    }

  return(STATUS_SUCCESS);
}

NTSTATUS
RtlCaptureAnsiString(PANSI_STRING Dest,
		     PANSI_STRING UnsafeSrc)
{
  PANSI_STRING Src; 
  NTSTATUS Status;
  
  /*
   * Copy the source string structure to kernel space.
   */
  Status = MmCopyFromCaller(&Src, UnsafeSrc, sizeof(ANSI_STRING));
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  /*
   * Initialize the destination string.
   */
  Dest->Length = Src->Length;
  Dest->MaximumLength = Src->MaximumLength;
  Dest->Buffer = ExAllocatePool(NonPagedPool, Dest->MaximumLength);
  if (Dest->Buffer == NULL)
    {
      return(Status);
    }

  /*
   * Copy the source string to kernel space.
   */
  Status = MmCopyFromCaller(Dest->Buffer, Src->Buffer, Dest->Length);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(Dest->Buffer);
      return(Status);
    }

  return(STATUS_SUCCESS);
}

/* EOF */

