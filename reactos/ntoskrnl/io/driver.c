/* $Id: driver.c,v 1.1 2002/06/10 08:47:21 ekohl Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/driver.c
 * PURPOSE:        Manage devices
 * PROGRAMMER:     David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                 15/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/po.h>
#include <internal/ldr.h>
#include <internal/id.h>
#include <internal/pool.h>
#include <internal/registry.h>

#include <roscfg.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED IoDriverObjectType = NULL;

#define TAG_DRIVER             TAG('D', 'R', 'V', 'R')
#define TAG_DRIVER_EXTENSION   TAG('D', 'R', 'V', 'E')

#define DRIVER_REGISTRY_KEY_BASENAME  L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"


/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL
IopCreateDriver(PVOID ObjectBody,
		PVOID Parent,
		PWSTR RemainingPath,
		POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT("LdrCreateModule(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody,
	 Parent,
	 RemainingPath);
  if (RemainingPath != NULL && wcschr(RemainingPath + 1, '\\') != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  return(STATUS_SUCCESS);
}


VOID
IopInitDriverImplementation(VOID)
{
  /*  Register the process object type  */
  IoDriverObjectType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  IoDriverObjectType->Tag = TAG('D', 'R', 'V', 'T');
  IoDriverObjectType->TotalObjects = 0;
  IoDriverObjectType->TotalHandles = 0;
  IoDriverObjectType->MaxObjects = ULONG_MAX;
  IoDriverObjectType->MaxHandles = ULONG_MAX;
  IoDriverObjectType->PagedPoolCharge = 0;
  IoDriverObjectType->NonpagedPoolCharge = sizeof(MODULE);
  IoDriverObjectType->Dump = NULL;
  IoDriverObjectType->Open = NULL;
  IoDriverObjectType->Close = NULL;
  IoDriverObjectType->Delete = NULL;
  IoDriverObjectType->Parse = NULL;
  IoDriverObjectType->Security = NULL;
  IoDriverObjectType->QueryName = NULL;
  IoDriverObjectType->OkayToClose = NULL;
  IoDriverObjectType->Create = IopCreateDriver;
  IoDriverObjectType->DuplicationNotify = NULL;
  RtlInitUnicodeString(&IoDriverObjectType->TypeName, L"Driver");
}

/**********************************************************************
 * NAME							EXPORTED
 *	NtLoadDriver
 *
 * DESCRIPTION
 * 	Loads a device driver.
 * 	
 * ARGUMENTS
 *	DriverServiceName
 *		Name of the service to load (registry key).
 *		
 * RETURN VALUE
 * 	Status.
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtLoadDriver(IN PUNICODE_STRING DriverServiceName)
{
  PDEVICE_NODE DeviceNode;
  NTSTATUS Status;

  PMODULE_OBJECT ModuleObject;
  WCHAR Buffer[MAX_PATH];
  ULONG Length;
  LPWSTR Start;
  LPWSTR Ext;

  /* FIXME: this should lookup the filename from the registry */

  /* Use IopRootDeviceNode for now */
  Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Status = LdrLoadModule(DriverServiceName, &ModuleObject);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("LdrLoadModule() failed (Status %lx)\n", Status);
      IopFreeDeviceNode(DeviceNode);
      return(Status);
    }

  /* Set a service name for the device node */

  /* Get the service name from the module name */
  Start = wcsrchr(ModuleObject->BaseName.Buffer, L'\\');
  if (Start == NULL)
    Start = ModuleObject->BaseName.Buffer;
  else
    Start++;

  Ext = wcsrchr(ModuleObject->BaseName.Buffer, L'.');
  if (Ext != NULL)
    Length = Ext - Start;
  else
    Length = wcslen(Start);

  wcsncpy(Buffer, Start, Length);
  RtlInitUnicodeString(&DeviceNode->ServiceName, Buffer);


  Status = IopInitializeDriver(ModuleObject->EntryPoint, DeviceNode);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("IopInitializeDriver() failed (Status %lx)\n", Status);
      ObDereferenceObject(ModuleObject);
      IopFreeDeviceNode(DeviceNode);
    }

  return(Status);
}


NTSTATUS STDCALL
NtUnloadDriver(IN PUNICODE_STRING DriverServiceName)
{
  UNIMPLEMENTED;
}

/* EOF */
