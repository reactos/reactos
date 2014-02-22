/*
 * VideoPort driver
 *
 * Copyright (C) 2007 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "videoprt.h"

#define NDEBUG
#include <debug.h>

typedef struct _VIDEO_PORT_FUNCTION_TABLE {
    PVOID Address;
    PUCHAR Name;
} *PVIDEO_PORT_FUNCTION_TABLE, VIDEO_PORT_FUNCTION_TABLE;

/* GLOBAL VARIABLES ***********************************************************/

#define VP_EXPORTED_FUNCS 6

UCHAR FN_VideoPortClearEvent[] = "VideoPortClearEvent";
UCHAR FN_VideoPortCreateEvent[] = "VideoPortCreateEvent";
UCHAR FN_VideoPortCreateSecondaryDisplay[] = "VideoPortCreateSecondaryDisplay";
UCHAR FN_VideoPortDeleteEvent[] = "VideoPortDeleteEvent";
UCHAR FN_VideoPortQueueDpc[] = "VideoPortQueueDpc";
UCHAR FN_VideoPortSetEvent[] = "VideoPortSetEvent";

VIDEO_PORT_FUNCTION_TABLE VideoPortExports[] = {
    {VideoPortClearEvent, FN_VideoPortClearEvent},
    {VideoPortCreateEvent, FN_VideoPortCreateEvent},
    {VideoPortCreateSecondaryDisplay, FN_VideoPortCreateSecondaryDisplay},
    {VideoPortDeleteEvent, FN_VideoPortDeleteEvent},
    {VideoPortQueueDpc, FN_VideoPortQueueDpc},
    {VideoPortSetEvent, FN_VideoPortSetEvent}
};

PVOID NTAPI
IntVideoPortGetProcAddress(
   IN PVOID HwDeviceExtension,
   IN PUCHAR FunctionName)
{
   ULONG i = 0;

   TRACE_(VIDEOPRT, "VideoPortGetProcAddress(%s)\n", FunctionName);

   /* Search by name */
   for (i = 0; i < VP_EXPORTED_FUNCS; i++)
   {
      if (!_strnicmp((PCHAR)FunctionName, (PCHAR)VideoPortExports[i].Name,
                     strlen((PCHAR)FunctionName)))
      {
         return (PVOID)VideoPortExports[i].Address;
      }
   }

   WARN_(VIDEOPRT, "VideoPortGetProcAddress: Can't resolve symbol %s\n", FunctionName);

   return NULL;
}
