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
 * $Id: services.c,v 1.1 2004/01/19 15:56:53 navaraf Exp $
 */

#include "videoprt.h"

VOID STDCALL
IntInterfaceReference(PVOID Context)
{
   /* Do nothing */
}

VOID STDCALL
IntInterfaceDereference(PVOID Context)
{
   /* Do nothing */
}

VP_STATUS
STDCALL
VideoPortQueryServices(
  IN PVOID HwDeviceExtension,
  IN VIDEO_PORT_SERVICES ServicesType,
  IN OUT PINTERFACE Interface)
{
   switch (ServicesType)
   {
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
            return STATUS_SUCCESS;
         }
         break;

      case VideoPortServicesAGP:
      case VideoPortServicesI2C:
      case VideoPortServicesHeadless:
         return STATUS_NOT_IMPLEMENTED;

      default:
         break;
   }

   return STATUS_UNSUCCESSFUL;
}
