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
/* $Id: debug.c,v 1.4 2003/05/18 17:16:17 ea Exp $
 * 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Brush Functions
 * FILE:              subsys/win32k/eng/debug.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 11/7/1999: Created
 */

#include <ddk/ntddk.h>

VOID STDCALL
EngDebugPrint(PCHAR StandardPrefix,
	      PCHAR DebugMessage,
	      va_list ap)
{
  DbgPrint(StandardPrefix);
  DbgPrint(DebugMessage, ap);
  DbgPrint("\n");
}
/* EOF */
