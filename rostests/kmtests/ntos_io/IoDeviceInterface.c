/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Device Interface functions test
 * PROGRAMMER:      Filip Navara <xnavara@volny.cz>
 */

/* TODO: what's with the prototypes at the top, what's with the if-ed out part? Doesn't process most results */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#if 0
NTSTATUS
(NTAPI *IoGetDeviceInterfaces_Func)(
   IN CONST GUID *InterfaceClassGuid,
   IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
   IN ULONG Flags,
   OUT PWSTR *SymbolicLinkList);

NTSTATUS NTAPI
ReactOS_IoGetDeviceInterfaces(
   IN CONST GUID *InterfaceClassGuid,
   IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
   IN ULONG Flags,
   OUT PWSTR *SymbolicLinkList);
#endif /* 0 */

static VOID DeviceInterfaceTest_Func()
{
   NTSTATUS Status;
   PWSTR SymbolicLinkList;
   PWSTR SymbolicLinkListPtr;
   GUID Guid = {0x378de44c, 0x56ef, 0x11d1, {0xbc, 0x8c, 0x00, 0xa0, 0xc9, 0x14, 0x05, 0xdd}};

   Status = IoGetDeviceInterfaces(
      &Guid,
      NULL,
      0,
      &SymbolicLinkList);

   ok(NT_SUCCESS(Status),
         "IoGetDeviceInterfaces failed with status 0x%X\n",
         (unsigned int)Status);
   if (!NT_SUCCESS(Status))
   {
      return;
   }

   DPRINT("IoGetDeviceInterfaces results:\n");
   for (SymbolicLinkListPtr = SymbolicLinkList;
        SymbolicLinkListPtr[0] != 0 && SymbolicLinkListPtr[1] != 0;
        SymbolicLinkListPtr += wcslen(SymbolicLinkListPtr) + 1)
   {
      DPRINT1("Symbolic Link: %S\n", SymbolicLinkListPtr);
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

START_TEST(IoDeviceInterface)
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

    DeviceInterfaceTest_Func();
}
