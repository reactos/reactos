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
/* $Id: perfcnt.c,v 1.2 2003/06/20 10:37:23 gvg Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Performance Counter Functions
 * FILE:              subsys/win32k/eng/perfcnt.c
 * PROGRAMER:         Ge van Geldorp
 */

#include <ddk/ntddk.h>

#define NDEBUG
#include <win32k/debug1.h>
#include <debug.h>

VOID STDCALL
EngQueryPerformanceFrequency(LONGLONG *Frequency)
{
  LARGE_INTEGER Freq;

  KeQueryPerformanceCounter(&Freq);
  *Frequency = Freq.QuadPart;
}

VOID STDCALL
EngQueryPerformanceCounter(LONGLONG *Count)
{
  LARGE_INTEGER PerfCount;

  PerfCount = KeQueryPerformanceCounter(NULL);
  *Count = PerfCount.QuadPart;
}
