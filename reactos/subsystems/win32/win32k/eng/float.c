/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Engine floating point functions
 * FILE:              subsys/win32k/eng/float.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

BOOL
STDCALL
EngRestoreFloatingPointState ( IN VOID *Buffer )
{
  NTSTATUS Status;
  Status = KeRestoreFloatingPointState((PKFLOATING_SAVE)Buffer);
  if (Status != STATUS_SUCCESS)
    {
      return FALSE;
    }
  return TRUE;
}

ULONG
STDCALL
EngSaveFloatingPointState(OUT VOID  *Buffer,
			  IN ULONG  BufferSize)
{
  KFLOATING_SAVE TempBuffer;
  NTSTATUS Status;
  if (Buffer == NULL || BufferSize == 0)
    {
      /* Check for floating point support. */
      Status = KeSaveFloatingPointState(&TempBuffer);
      if (Status != STATUS_SUCCESS)
	{
	  return(0);
	}
      KeRestoreFloatingPointState(&TempBuffer);
      return(sizeof(KFLOATING_SAVE));
    }
  if (BufferSize < sizeof(KFLOATING_SAVE))
    {
      return(0);
    }
  Status = KeSaveFloatingPointState((PKFLOATING_SAVE)Buffer);
  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }
  return TRUE;
}

VOID
STDCALL
FLOATOBJ_Add (
	IN OUT PFLOATOBJ  pf,
	IN PFLOATOBJ      pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_2i3r.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_AddFloat(
	IN OUT PFLOATOBJ  pf,
	IN FLOATL  f
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0ip3.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_AddLong(
	IN OUT PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_12jr.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_Div(
	IN OUT PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_3ndz.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_DivFloat(
	IN OUT PFLOATOBJ  pf,
	IN FLOATL  f
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0gfb.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_DivLong(
	IN OUT PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_6jdz.htm
  UNIMPLEMENTED;
}

BOOL
STDCALL
FLOATOBJ_Equal(
	IN PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_6ysn.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_EqualLong(
	IN PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_1pgn.htm
  UNIMPLEMENTED;
  return FALSE;
}

LONG
STDCALL
FLOATOBJ_GetFloat ( IN PFLOATOBJ pf )
{
  // www.osr.com/ddk/graphics/gdifncs_4d5z.htm
  UNIMPLEMENTED;
  return 0;
}

LONG
STDCALL
FLOATOBJ_GetLong ( IN PFLOATOBJ pf )
{
  // www.osr.com/ddk/graphics/gdifncs_0tgn.htm
  UNIMPLEMENTED;
  return 0;
}

BOOL
STDCALL
FLOATOBJ_GreaterThan(
	IN PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8n53.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_GreaterThanLong(
	IN PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_6gx3.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_LessThan(
	IN PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_1ynb.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
FLOATOBJ_LessThanLong(
	IN PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_9nzb.htm
  UNIMPLEMENTED;
  return FALSE;
}

VOID
STDCALL
FLOATOBJ_Mul(
	IN OUT PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8ppj.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_MulFloat(
	IN OUT PFLOATOBJ  pf,
	IN FLOATL  f
	)
{
  // www.osr.com/ddk/graphics/gdifncs_3puv.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_MulLong(
	IN OUT PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_56lj.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_Neg ( IN OUT PFLOATOBJ pf )
{
  // www.osr.com/ddk/graphics/gdifncs_14pz.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_SetFloat(
	OUT PFLOATOBJ  pf,
	IN FLOATL  f
	)
{
  // www.osr.com/ddk/graphics/gdifncs_1prb.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_SetLong(
	OUT PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0gpz.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_Sub(
	IN OUT PFLOATOBJ  pf,
	IN PFLOATOBJ  pf1
	)
{
  // www.osr.com/ddk/graphics/gdifncs_6lyf.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_SubFloat(
	IN OUT PFLOATOBJ  pf,
	IN FLOATL  f
	)
{
  // www.osr.com/ddk/graphics/gdifncs_2zvr.htm
  UNIMPLEMENTED;
}

VOID
STDCALL
FLOATOBJ_SubLong(
	IN OUT PFLOATOBJ  pf,
	IN LONG  l
	)
{
  // www.osr.com/ddk/graphics/gdifncs_852f.htm
  UNIMPLEMENTED;
}
