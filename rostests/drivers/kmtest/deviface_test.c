/*
 * PnP Test
 * Device Interface functions test
 *
 * Copyright 2004 Filip Navara <xnavara@volny.cz>
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
 */

/* INCLUDES *******************************************************************/

#include <ddk/ntddk.h>
#include <ndk/iotypes.h>
#include "kmtest.h"

//#define NDEBUG
#include "debug.h"

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS STDCALL
(*IoGetDeviceInterfaces_Func)(
   IN CONST GUID *InterfaceClassGuid,
   IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
   IN ULONG Flags,
   OUT PWSTR *SymbolicLinkList);

NTSTATUS STDCALL
ReactOS_IoGetDeviceInterfaces(
   IN CONST GUID *InterfaceClassGuid,
   IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
   IN ULONG Flags,
   OUT PWSTR *SymbolicLinkList);

VOID FASTCALL DeviceInterfaceTest_Func()
{
   NTSTATUS Status;
   PWSTR SymbolicLinkList;
   PWSTR SymbolicLinkListPtr;
   GUID Guid = {0x378de44c, 0x56ef, 0x11d1, {0xbc, 0x8c, 0x00, 0xa0, 0xc9, 0x14, 0x05, 0xdd}};

   Status = IoGetDeviceInterfaces_Func(
      &Guid,
      NULL,
      0,
      &SymbolicLinkList);

   if (!NT_SUCCESS(Status))
   {
      DPRINT(
         "[PnP Test] IoGetDeviceInterfaces failed with status 0x%X\n",
         Status);
      return;
   }

   DPRINT("[PnP Test] IoGetDeviceInterfaces results:\n");
   for (SymbolicLinkListPtr = SymbolicLinkList;
        SymbolicLinkListPtr[0] != 0 && SymbolicLinkListPtr[1] != 0;
        SymbolicLinkListPtr += wcslen(SymbolicLinkListPtr) + 1)
   {
      DPRINT("[PnP Test] %S\n", SymbolicLinkListPtr);
   }

#if 0
   DPRINT("[PnP Test] Trying to get aliases\n");

   for (SymbolicLinkListPtr = SymbolicLinkList;
        SymbolicLinkListPtr[0] != 0 && SymbolicLinkListPtr[1] != 0;
        SymbolicLinkListPtr += wcslen(SymbolicLinkListPtr) + 1)
   {
      UNICODE_STRING SymbolicLink;
      UNICODE_STRING AliasSymbolicLink;

      SymbolicLink.Buffer = SymbolicLinkListPtr;
      SymbolicLink.Length = SymbolicLink.MaximumLength = wcslen(SymbolicLinkListPtr);
      RtlInitUnicodeString(&AliasSymbolicLink, NULL);
      IoGetDeviceInterfaceAlias(
         &SymbolicLink,
         &AliasGuid,
         &AliasSymbolicLink);
      if (AliasSymbolicLink.Buffer != NULL)
      {
         DPRINT("[PnP Test] Original: %S\n", SymbolicLinkListPtr);
         DPRINT("[PnP Test] Alias: %S\n", AliasSymbolicLink.Buffer);
      }
   }
#endif

   ExFreePool(SymbolicLinkList);
}

VOID RegisterDI_Test()
{
    GUID Guid = {0x378de44c, 0x56ef, 0x11d1, {0xbc, 0x8c, 0x00, 0xa0, 0xc9, 0x14, 0x05, 0xdd}};
    DEVICE_OBJECT DeviceObject;
    EXTENDED_DEVOBJ_EXTENSION DeviceObjectExtension;
    DEVICE_NODE DeviceNode;
    UNICODE_STRING SymbolicLinkName;
    NTSTATUS Status;

    RtlInitUnicodeString(&SymbolicLinkName, L"");

    // Prepare our surrogate of a Device Object
    DeviceObject.DeviceObjectExtension = (PDEVOBJ_EXTENSION)&DeviceObjectExtension;

    // 1. DeviceNode = NULL
    DeviceObjectExtension.DeviceNode = NULL;
    Status = IoRegisterDeviceInterface(&DeviceObject, &Guid, NULL,
        &SymbolicLinkName);

    ok(Status == STATUS_INVALID_DEVICE_REQUEST,
        "IoRegisterDeviceInterface returned 0x%08lX\n", Status);

    // 2. DeviceNode->InstancePath is of a null length
    DeviceObjectExtension.DeviceNode = &DeviceNode;
    DeviceNode.InstancePath.Length = 0;
    Status = IoRegisterDeviceInterface(&DeviceObject, &Guid, NULL,
        &SymbolicLinkName);

    ok(Status == STATUS_INVALID_DEVICE_REQUEST,
        "IoRegisterDeviceInterface returned 0x%08lX\n", Status);
}

VOID FASTCALL NtoskrnlIoDeviceInterface()
{
    StartTest();

    // Test IoRegisterDeviceInterface() failures now
    RegisterDI_Test();

/*
    DPRINT("Calling DeviceInterfaceTest_Func with native functions\n");
    IoGetDeviceInterfaces_Func = IoGetDeviceInterfaces;
    DeviceInterfaceTest_Func();
    DPRINT("Calling DeviceInterfaceTest_Func with ReactOS functions\n");
    IoGetDeviceInterfaces_Func = ReactOS_IoGetDeviceInterfaces;
    DeviceInterfaceTest_Func();
*/

    FinishTest("NTOSKRNL Io Device Interface Test");
}
