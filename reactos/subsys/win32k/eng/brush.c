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
/* $Id: brush.c,v 1.8 2003/07/11 15:59:37 royce Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Brush Functions
 * FILE:              subsys/win32k/eng/brush.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <ddk/winddi.h>

/*
 * @implemented
 */
PVOID STDCALL
BRUSHOBJ_pvAllocRbrush(IN PBRUSHOBJ  BrushObj,
		       IN ULONG  ObjSize)
{
  BrushObj->pvRbrush=EngAllocMem(0, ObjSize, 0);
  return(BrushObj->pvRbrush);
}

/*
 * @implemented
 */
PVOID STDCALL
BRUSHOBJ_pvGetRbrush(IN PBRUSHOBJ  BrushObj)
{
  return(BrushObj->pvRbrush);
}
/* EOF */
