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
 */

#include "videoprt.h"

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

VP_STATUS NTAPI
VideoPortCreateEvent(
   IN PVOID HwDeviceExtension,
   IN ULONG EventFlag,
   IN PVOID Unused,
   OUT PEVENT *Event)
{
   PVIDEO_PORT_EVENT VpEvent;
   EVENT_TYPE Type = SynchronizationEvent;

   /* Allocate storage for the event structure */
   VpEvent = ExAllocatePoolWithTag(
      NonPagedPool,
      sizeof(VIDEO_PORT_EVENT),
      TAG_VIDEO_PORT);

   /* Fail if not enough memory */
   if (!VpEvent) return ERROR_NOT_ENOUGH_MEMORY;

   /* Initialize the event structure */
   RtlZeroMemory(VpEvent, sizeof(VIDEO_PORT_EVENT));
   VpEvent->pKEvent = &VpEvent->Event;

   /* Determine the event type */
   if (EventFlag & NOTIFICATION_EVENT)
      Type = NotificationEvent;

   /* Initialize kernel event */
   KeInitializeEvent(VpEvent->pKEvent, Type, EventFlag & INITIAL_EVENT_SIGNALED);

   /* Indicate success */
   return NO_ERROR;
}

/*
 * @implemented
 */

VP_STATUS NTAPI
VideoPortDeleteEvent(
   IN PVOID HwDeviceExtension,
   IN PEVENT Event)
{
   /* Free storage */
   ExFreePool(Event);

   /* Indicate success */
   return NO_ERROR;
}

/*
 * @implemented
 */

LONG NTAPI
VideoPortSetEvent(
   IN PVOID HwDeviceExtension,
   IN PEVENT Event)
{
   return KeSetEvent(Event->pKEvent, IO_NO_INCREMENT, FALSE);
}

/*
 * @implemented
 */

VOID NTAPI
VideoPortClearEvent(
   IN PVOID HwDeviceExtension,
   IN PEVENT Event)
{
   KeClearEvent(Event->pKEvent);
}

/*
 * @implemented
 */

VP_STATUS NTAPI
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
