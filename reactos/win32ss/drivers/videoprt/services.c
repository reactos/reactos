/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
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

VOID NTAPI
IntInterfaceReference(PVOID Context)
{
   /* Do nothing */
}

VOID NTAPI
IntInterfaceDereference(PVOID Context)
{
   /* Do nothing */
}

VP_STATUS NTAPI
VideoPortQueryServices(
  IN PVOID HwDeviceExtension,
  IN VIDEO_PORT_SERVICES ServicesType,
  IN OUT PINTERFACE Interface)
{
   TRACE_(VIDEOPRT, "VideoPortQueryServices - ServicesType: 0x%x\n", ServicesType);

   switch (ServicesType)
   {
#if defined(_M_IX86)
      case VideoPortServicesInt10:
         if (Interface->Version >= VIDEO_PORT_INT10_INTERFACE_VERSION_1 ||
             Interface->Size >= sizeof(VIDEO_PORT_INT10_INTERFACE))
         {
            VIDEO_PORT_INT10_INTERFACE *Int10Interface =
               (VIDEO_PORT_INT10_INTERFACE *)Interface;

            Interface->InterfaceReference = IntInterfaceReference;
            Interface->InterfaceDereference = IntInterfaceDereference;
            Int10Interface->Int10AllocateBuffer = IntInt10AllocateBuffer;
            Int10Interface->Int10FreeBuffer = IntInt10FreeBuffer;
            Int10Interface->Int10ReadMemory = IntInt10ReadMemory;
            Int10Interface->Int10WriteMemory = IntInt10WriteMemory;
            Int10Interface->Int10CallBios = IntInt10CallBios;
            return NO_ERROR;
         }
         break;
#endif
      case VideoPortServicesAGP:
         if ((Interface->Version == VIDEO_PORT_AGP_INTERFACE_VERSION_2 &&
              Interface->Size >= sizeof(VIDEO_PORT_AGP_INTERFACE_2)) ||
             (Interface->Version == VIDEO_PORT_AGP_INTERFACE_VERSION_1 &&
              Interface->Size >= sizeof(VIDEO_PORT_AGP_INTERFACE)))
         {
            if (NT_SUCCESS(IntAgpGetInterface(HwDeviceExtension, Interface)))
            {
               return NO_ERROR;
            }
         }
         break;

      case VideoPortServicesI2C:
          UNIMPLEMENTED;
          return ERROR_INVALID_FUNCTION;

      case VideoPortServicesHeadless:
         UNIMPLEMENTED;
         return ERROR_INVALID_FUNCTION;

      default:
         break;
   }

   return ERROR_INVALID_FUNCTION;
}

BOOLEAN NTAPI
VideoPortGetAgpServices(
   IN PVOID HwDeviceExtension,
   OUT PVIDEO_PORT_AGP_SERVICES AgpServices)
{
   VIDEO_PORT_AGP_INTERFACE Interface;
   VP_STATUS Status;

   TRACE_(VIDEOPRT, "VideoPortGetAgpServices\n");

   Interface.Size = sizeof(Interface);
   Interface.Version = VIDEO_PORT_AGP_INTERFACE_VERSION_1;

   Status = VideoPortQueryServices(HwDeviceExtension, VideoPortServicesAGP,
                                   (PINTERFACE)&Interface);
   if (Status != NO_ERROR)
   {
      WARN_(VIDEOPRT, "VideoPortQueryServices() failed!\n");
      return FALSE;
   }

   RtlCopyMemory(AgpServices, &Interface.AgpReservePhysical, sizeof(VIDEO_PORT_AGP_SERVICES));
   return TRUE;
}
