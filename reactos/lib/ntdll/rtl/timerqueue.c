/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Timer Queue implementation
 * FILE:              lib/ntdll/rtl/timerqueue.c
 */

/* INCLUDES ****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

typedef VOID (CALLBACK *WAITORTIMERCALLBACKFUNC) (PVOID, BOOLEAN );

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlCreateTimer(HANDLE TimerQueue,
               PHANDLE phNewTimer,
	       WAITORTIMERCALLBACKFUNC Callback,
	       PVOID Parameter,
	       DWORD DueTime,
	       DWORD Period,
	       ULONG Flags)
{
  DPRINT1("RtlCreateTimer: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlCreateTimerQueue(PHANDLE TimerQueue)
{
  DPRINT1("RtlCreateTimerQueue: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeleteTimer(HANDLE TimerQueue,
               HANDLE Timer,
	       HANDLE CompletionEvent)
{
  DPRINT1("RtlDeleteTimer: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeleteTimerQueue(HANDLE TimerQueue)
{
  DPRINT1("RtlDeleteTimerQueue: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeleteTimerQueueEx(HANDLE TimerQueue,
                      HANDLE CompletionEvent)
{
  DPRINT1("RtlDeleteTimerQueueEx: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlUpdateTimer(HANDLE TimerQueue,
               HANDLE Timer,
	       ULONG DueTime,
	       ULONG Period)
{
  DPRINT1("RtlUpdateTimer: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
