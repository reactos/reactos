/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: event.c,v 1.2 2004/03/19 20:58:32 navaraf Exp $
 */

#include "videoprt.h"

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortCreateEvent(
   IN PVOID HwDeviceExtension,
   IN ULONG EventFlag,
   IN PVOID Unused,
   OUT PEVENT *Event)
{
   EVENT_TYPE Type;
 
   (*Event) = ExAllocatePoolWithTag(
      NonPagedPool,
      sizeof(KEVENT), 
      TAG_VIDEO_PORT);

   if ((*Event) == NULL)
      return ERROR_NOT_ENOUGH_MEMORY;

   if (EventFlag & NOTIFICATION_EVENT)
      Type = NotificationEvent;
   else
      Type = SynchronizationEvent;

   KeInitializeEvent((PKEVENT)*Event, Type, EventFlag & INITIAL_EVENT_SIGNALED);
   return NO_ERROR;
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortDeleteEvent(
   IN PVOID HwDeviceExtension,
   IN PEVENT Event)
{
   ExFreePool(Event);
   return NO_ERROR;
}

/*
 * @implemented
 */

LONG STDCALL
VideoPortSetEvent(
   IN PVOID HwDeviceExtension,
   IN PEVENT Event)
{
   return KeSetEvent((PKEVENT)Event, IO_NO_INCREMENT, FALSE);
}

/*
 * @implemented
 */

VOID STDCALL
VideoPortClearEvent(
   IN PVOID HwDeviceExtension,
   IN PEVENT Event)
{
   KeClearEvent((PKEVENT)Event);
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortWaitForSingleObject(
   IN PVOID HwDeviceExtension,
   IN PVOID Object,
   IN PLARGE_INTEGER Timeout OPTIONAL)
{
   return KeWaitForSingleObject(
      Object,
      Executive,
      KernelMode,
      FALSE,
      Timeout);
}
