/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: main.c,v 1.110 2001/12/27 23:56:42 dwelch Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/main.c
 * PURPOSE:         Initalizes the kernel
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <reactos/resource.h>
#include <internal/mm.h>
#include <internal/module.h>
#include <internal/ldr.h>
#include <internal/ex.h>
#include <internal/ps.h>
#include <internal/ke.h>
#include <internal/io.h>
#include <internal/po.h>
#include <internal/cc.h>
//#include <internal/se.h>
#include <napi/shared_data.h>
#include <internal/v86m.h>
#include <internal/kd.h>
#include <internal/trap.h>
#include "../dbg/kdb.h"
#include <internal/registry.h>

#ifdef HALDBG
#include <internal/ntosdbg.h>
#else
#define ps(args...)
#endif

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

ULONG EXPORTED NtBuildNumber = KERNEL_VERSION_BUILD;
ULONG EXPORTED NtGlobalFlag = 0;
CHAR  EXPORTED KeNumberProcessors;
LOADER_PARAMETER_BLOCK EXPORTED KeLoaderBlock;
static LOADER_MODULE KeLoaderModules[64];
static UCHAR KeLoaderModuleStrings[64][256];
static UCHAR KeLoaderCommandLine[256];
static ADDRESS_RANGE KeMemoryMap[64];
static ULONG KeMemoryMapRangeCount;
static ULONG FirstKrnlPhysAddr;
static ULONG LastKrnlPhysAddr;
static ULONG LastKernelAddress;
volatile BOOLEAN Initialized = FALSE;

extern PVOID Ki386InitialStackArray[MAXIMUM_PROCESSORS];

typedef struct
{
  LPWSTR ServiceName;
  LPWSTR DeviceDesc;
  LPWSTR Group;
  DWORD Start;
  DWORD Type;
} SERVICE, *PSERVICE;

SERVICE Services[] = {
  {L"pci", L"PCI Bus Driver", L"Boot Bus Extender", 0, 1},
  {L"keyboard", L"Standard Keyboard Driver", L"Base", 0, 1},
  {L"blue", L"Bluescreen Driver", L"Base", 0, 1},
/*  {L"vidport", L"Video Port Driver", L"Base", 0, 1},
  {L"vgamp", L"VGA Miniport", L"Base", 0, 1},
  {L"minixfs", L"Minix File System", L"File system", 0, 1},
  {L"msfs", L"Mail Slot File System", L"File system", 0, 1},
  {L"npfs", L"Named Pipe File System", L"File system", 0, 1},
  {L"psaux", L"PS/2 Auxillary Port Driver", L"", 0, 1},
  {L"mouclass", L"Mouse Class Driver", L"Pointer Class", 0, 1},
  {L"ndis", L"NDIS System Driver", L"NDIS Wrapper", 0, 1},
  {L"ne2000", L"Novell Eagle 2000 Driver", L"NDIS", 0, 1},
  {L"afd", L"AFD Networking Support Environment", L"TDI", 0, 1},*/
  {NULL,}
};

/* FUNCTIONS ****************************************************************/

//#define FULLREG

VOID CreateDefaultRegistryForLegacyDriver(
  PSERVICE Service)
{
#ifdef FULLREG
  WCHAR LegacyDriver[] = L"LegacyDriver";
#endif
  WCHAR InstancePath[MAX_PATH];
  WCHAR KeyNameBuffer[MAX_PATH];
  WCHAR Name[MAX_PATH];
  UNICODE_STRING KeyName;
  HANDLE KeyHandle;
#ifdef FULLREG
  DWORD DwordData;
#endif
  ULONG Length;
  NTSTATUS Status;
  WCHAR ImagePath[MAX_PATH];
 
  /* Enum section */
  wcscpy(Name, Service->ServiceName);
  _wcsupr(Name);
  wcscpy(InstancePath, L"Root\\LEGACY_");
  wcscat(InstancePath, Name);
  wcscat(InstancePath, L"\\0000");

  wcscpy(KeyNameBuffer, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
  wcscat(KeyNameBuffer, InstancePath);

  RtlInitUnicodeString(&KeyName, KeyNameBuffer);

  DPRINT("Key name is %S\n", KeyName.Buffer);

  Status = RtlpCreateRegistryKeyPath(KeyName.Buffer);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlpCreateRegistryKeyPath() failed with status %x\n", Status);
  return;
    }

  Status = RtlpGetRegistryHandle(
    RTL_REGISTRY_ENUM,
	  InstancePath,
		TRUE,
		&KeyHandle);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlpGetRegistryHandle() failed (Status %x)\n", Status);
  return;
    }
#ifdef FULLREG
  DwordData = 0;
  Length = sizeof(DWORD);
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_HANDLE,
    (PWSTR)KeyHandle,
		L"Capabilities",
		REG_DWORD,
		(LPWSTR)&DwordData,
		Length);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlWriteRegistryValue() failed (Status %x)\n", Status);
	NtClose(KeyHandle);
	return;
    }

  Length = (wcslen(LegacyDriver) + 1) * sizeof(WCHAR);
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_HANDLE,
    (PWSTR)KeyHandle,
		L"Class",
		REG_SZ,
		LegacyDriver,
		Length);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlWriteRegistryValue() failed (Status %x)\n", Status);
	NtClose(KeyHandle);
	return;
    }
#endif
  Length = (wcslen(Service->DeviceDesc) + 1) * sizeof(WCHAR);
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_HANDLE,
		(PWSTR)KeyHandle,
		L"DeviceDesc",
		REG_SZ,
		Service->DeviceDesc,
		Length);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlWriteRegistryValue() failed (Status %x)\n", Status);
	NtClose(KeyHandle);
	return;
    }
#ifdef FULLREG
  DwordData = 0;
  Length = Length = sizeof(DWORD);
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_HANDLE,
    (PWSTR)KeyHandle,
		L"Legacy",
		REG_DWORD,
		(LPWSTR)&DwordData,
		sizeof(DWORD));
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlWriteRegistryValue() failed (Status %x)\n", Status);
	NtClose(KeyHandle);
	return;
    }
#endif
  Length = (wcslen(Service->ServiceName) + 1) * sizeof(WCHAR);
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_HANDLE,
		(PWSTR)KeyHandle,
		L"Service",
		REG_SZ,
		Service->ServiceName,
		Length);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlWriteRegistryValue() failed (Status %x)\n", Status);
	NtClose(KeyHandle);
	return;
    }

  NtClose(KeyHandle);


  /* Services section */

  Status = RtlpGetRegistryHandle(
    RTL_REGISTRY_SERVICES,
	  Service->ServiceName,
		TRUE,
		&KeyHandle);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlpGetRegistryHandle() failed (Status %x)\n", Status);
  return;
    }
#ifdef FULLREG
  Length = (wcslen(Service->DeviceDesc) + 1) * sizeof(WCHAR);
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_HANDLE,
		(PWSTR)KeyHandle,
		L"DisplayName",
		REG_SZ,
		Service->DeviceDesc,
		Length);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlWriteRegistryValue() failed (Status %x)\n", Status);
	NtClose(KeyHandle);
	return;
    }

  DwordData = 1;
  Length = sizeof(DWORD);
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_HANDLE,
    (PWSTR)KeyHandle,
		L"ErrorControl",
		REG_DWORD,
		(LPWSTR)&DwordData,
		Length);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlWriteRegistryValue() failed (Status %x)\n", Status);
	NtClose(KeyHandle);
	return;
    }

  Length = (wcslen(Service->Group) + 1) * sizeof(WCHAR);
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_HANDLE,
		(PWSTR)KeyHandle,
		L"Group",
		REG_SZ,
		Service->Group,
		Length);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlWriteRegistryValue() failed (Status %x)\n", Status);
	NtClose(KeyHandle);
	return;
    }
#endif
  wcscpy(ImagePath, L"\\SystemRoot\\System32\\drivers\\");
  wcscat(ImagePath, Service->ServiceName);
  wcscat(ImagePath, L".sys");

  Length = (wcslen(ImagePath) + 1) * sizeof(WCHAR);
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_HANDLE,
		(PWSTR)KeyHandle,
		L"ImagePath",
		REG_SZ,
		ImagePath,
		Length);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlWriteRegistryValue() failed (Status %x)\n", Status);
	NtClose(KeyHandle);
	return;
    }
#if FULLREG
  DwordData = Service->Start;
  Length = sizeof(DWORD);
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_HANDLE,
    (PWSTR)KeyHandle,
		L"Start",
		REG_DWORD,
		(LPWSTR)&DwordData,
		Length);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlWriteRegistryValue() failed (Status %x)\n", Status);
	NtClose(KeyHandle);
	return;
    }

  DwordData = Service->Type;
  Length = sizeof(DWORD);
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_HANDLE,
    (PWSTR)KeyHandle,
		L"Type",
		REG_DWORD,
		(LPWSTR)&DwordData,
		Length);
  if (!NT_SUCCESS(Status))
    {
  DPRINT1("RtlWriteRegistryValue() failed (Status %x)\n", Status);
	NtClose(KeyHandle);
	return;
    }
#endif
  NtClose(KeyHandle);
}

VOID CreateDefaultRegistry()
{
  NTSTATUS Status;
  ULONG i;

  Status = RtlpCreateRegistryKeyPath(L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
  if (!NT_SUCCESS(Status))
  {
    CPRINT("RtlpCreateRegistryKeyPath() (Status %x)\n", Status);
    return;
  }

  Status = RtlpCreateRegistryKeyPath(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
  if (!NT_SUCCESS(Status))
  {
    CPRINT("RtlpCreateRegistryKeyPath() (Status %x)\n", Status);
    return;
  }

  for (i = 0; Services[i].ServiceName != NULL; i++)
  {
    CreateDefaultRegistryForLegacyDriver(&Services[i]);
  }
}


static BOOLEAN
RtlpCheckFileNameExtension(PCHAR FileName,
			   PCHAR Extension)
{
   PCHAR Ext;

   Ext = strrchr(FileName, '.');
   if ((Extension == NULL) || (*Extension == 0))
     {
	if (Ext == NULL)
	  return TRUE;
	else
	  return FALSE;
     }
   if (*Extension != '.')
     Ext++;
   
   if (_stricmp(Ext, Extension) == 0)
     return TRUE;
   else
     return FALSE;
}

static VOID
CreateSystemRootLink (PCSZ ParameterLine)
{
	UNICODE_STRING LinkName;
	UNICODE_STRING DeviceName;
	UNICODE_STRING ArcName;
	UNICODE_STRING BootPath;
	PCHAR ParamBuffer;
	PWCHAR ArcNameBuffer;
	PCHAR p;
	NTSTATUS Status;
	ULONG Length;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE Handle;

	/* create local parameter line copy */
	ParamBuffer = ExAllocatePool (PagedPool, 256);
	strcpy (ParamBuffer, (char *)ParameterLine);

	DPRINT("%s\n", ParamBuffer);
	/* Format: <arc_name>\<path> [options...] */

	/* cut options off */
	p = strchr (ParamBuffer, ' ');
	if (p)
		*p = 0;
	DPRINT("%s\n", ParamBuffer);

	/* extract path */
	p = strchr (ParamBuffer, '\\');
	if (p)
	{
		DPRINT("Boot path: %s\n", p);
		RtlCreateUnicodeStringFromAsciiz (&BootPath, p);
		*p = 0;
	}
	else
	{
		DPRINT("Boot path: %s\n", "\\");
		RtlCreateUnicodeStringFromAsciiz (&BootPath, "\\");
	}
	DPRINT("Arc name: %s\n", ParamBuffer);
	
	/* Only arc name left - build full arc name */
	ArcNameBuffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));
	swprintf (ArcNameBuffer,
	          L"\\ArcName\\%S", ParamBuffer);
	RtlInitUnicodeString (&ArcName, ArcNameBuffer);
	DPRINT1("Arc name: %wZ\n", &ArcName);

	/* free ParamBuffer */
	ExFreePool (ParamBuffer);

	/* allocate device name string */
	DeviceName.Length = 0;
	DeviceName.MaximumLength = 256 * sizeof(WCHAR);
	DeviceName.Buffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));

	InitializeObjectAttributes (&ObjectAttributes,
	                            &ArcName,
	                            0,
	                            NULL,
	                            NULL);

	Status = NtOpenSymbolicLinkObject (&Handle,
	                                   SYMBOLIC_LINK_ALL_ACCESS,
	                                   &ObjectAttributes);
	if (!NT_SUCCESS(Status))
	{
	  RtlFreeUnicodeString (&BootPath);
	  RtlFreeUnicodeString (&DeviceName);
	  DPRINT1("NtOpenSymbolicLinkObject() '%wZ' failed (Status %x)\n",
		         &ArcName,
		         Status);
	  RtlFreeUnicodeString (&ArcName);

	  KeBugCheck (0x0);
	}
	RtlFreeUnicodeString (&ArcName);

	Status = NtQuerySymbolicLinkObject (Handle,
	                                    &DeviceName,
	                                    &Length);
	NtClose (Handle);
	if (!NT_SUCCESS(Status))
	{
		RtlFreeUnicodeString (&BootPath);
		RtlFreeUnicodeString (&DeviceName);
		DbgPrint("NtQuerySymbolicObject() failed (Status %x)\n",
		         Status);

		KeBugCheck (0x0);
	}
	DPRINT("Length: %lu DeviceName: %wZ\n", Length, &DeviceName);

	RtlAppendUnicodeStringToString (&DeviceName,
					&BootPath);

	RtlFreeUnicodeString (&BootPath);
	DPRINT("DeviceName: %wZ\n", &DeviceName);

	/* create the '\SystemRoot' link */
	RtlInitUnicodeString (&LinkName,
			      L"\\SystemRoot");

	Status = IoCreateSymbolicLink (&LinkName,
				       &DeviceName);
	RtlFreeUnicodeString (&DeviceName);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("IoCreateSymbolicLink() failed (Status %x)\n",
		         Status);

		KeBugCheck (0x0);
	}

	/* Check if '\SystemRoot'(LinkName) can be opened, otherwise crash it! */
	InitializeObjectAttributes (&ObjectAttributes,
	                            &LinkName,
	                            0,
	                            NULL,
	                            NULL);

	Status = NtOpenSymbolicLinkObject (&Handle,
	                                   SYMBOLIC_LINK_ALL_ACCESS,
	                                   &ObjectAttributes);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("NtOpenSymbolicLinkObject() failed to open '\\SystemRoot' (Status %x)\n",
		         Status);
		KeBugCheck (0x0);
	}
	NtClose(Handle);
}


static VOID
InitSystemSharedUserPage (PCSZ ParameterLine)
{
   PKUSER_SHARED_DATA SharedPage;

   UNICODE_STRING ArcDeviceName;
   UNICODE_STRING ArcName;
   UNICODE_STRING BootPath;
   UNICODE_STRING DriveDeviceName;
   UNICODE_STRING DriveName;
   WCHAR DriveNameBuffer[20];
   PCHAR ParamBuffer;
   PWCHAR ArcNameBuffer;
   PCHAR p;
   NTSTATUS Status;
   ULONG Length;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE Handle;
   ULONG i;
   BOOLEAN BootDriveFound;

   SharedPage = (PKUSER_SHARED_DATA)KERNEL_SHARED_DATA_BASE;
   SharedPage->DosDeviceMap = 0;
   SharedPage->NtProductType = NtProductWinNt;
   for (i = 0; i < 32; i++)
     {
	SharedPage->DosDeviceDriveType[i] = 0;
     }

   BootDriveFound = FALSE;

   /*
    * Retrieve the current dos system path
    * (e.g.: C:\reactos) from the given arc path
    * (e.g.: multi(0)disk(0)rdisk(0)partititon(1)\reactos)
    * Format: "<arc_name>\<path> [options...]"
    */

   /* create local parameter line copy */
   ParamBuffer = ExAllocatePool (PagedPool, 256);
   strcpy (ParamBuffer, (char *)ParameterLine);
   DPRINT("%s\n", ParamBuffer);

   /* cut options off */
   p = strchr (ParamBuffer, ' ');
   if (p)
     {
	*p = 0;
     }
   DPRINT("%s\n", ParamBuffer);

   /* extract path */
   p = strchr (ParamBuffer, '\\');
   if (p)
     {
	DPRINT("Boot path: %s\n", p);
	RtlCreateUnicodeStringFromAsciiz (&BootPath, p);
	*p = 0;
     }
   else
     {
	DPRINT("Boot path: %s\n", "\\");
	RtlCreateUnicodeStringFromAsciiz (&BootPath, "\\");
     }
   DPRINT("Arc name: %s\n", ParamBuffer);

   /* Only arc name left - build full arc name */
   ArcNameBuffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));
   swprintf (ArcNameBuffer, L"\\ArcName\\%S", ParamBuffer);
   RtlInitUnicodeString (&ArcName, ArcNameBuffer);
   DPRINT("Arc name: %wZ\n", &ArcName);

   /* free ParamBuffer */
   ExFreePool (ParamBuffer);

   /* allocate arc device name string */
   ArcDeviceName.Length = 0;
   ArcDeviceName.MaximumLength = 256 * sizeof(WCHAR);
   ArcDeviceName.Buffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));

   InitializeObjectAttributes (&ObjectAttributes,
			       &ArcName,
			       0,
			       NULL,
			       NULL);

   Status = NtOpenSymbolicLinkObject (&Handle,
				      SYMBOLIC_LINK_ALL_ACCESS,
				      &ObjectAttributes);
   RtlFreeUnicodeString (&ArcName);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeUnicodeString (&BootPath);
	RtlFreeUnicodeString (&ArcDeviceName);
	DbgPrint("NtOpenSymbolicLinkObject() failed (Status %x)\n",
	         Status);

	KeBugCheck (0x0);
     }

   Status = NtQuerySymbolicLinkObject (Handle,
				       &ArcDeviceName,
				       &Length);
   NtClose (Handle);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeUnicodeString (&BootPath);
	RtlFreeUnicodeString (&ArcDeviceName);
	DbgPrint("NtQuerySymbolicObject() failed (Status %x)\n",
		 Status);

	KeBugCheck (0x0);
     }
   DPRINT("Length: %lu ArcDeviceName: %wZ\n", Length, &ArcDeviceName);


   /* allocate device name string */
   DriveDeviceName.Length = 0;
   DriveDeviceName.MaximumLength = 256 * sizeof(WCHAR);
   DriveDeviceName.Buffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));

   for (i = 0; i < 26; i++)
     {
	swprintf (DriveNameBuffer, L"\\??\\%C:", 'A' + i);
	RtlInitUnicodeString (&DriveName,
			      DriveNameBuffer);

	InitializeObjectAttributes (&ObjectAttributes,
				    &DriveName,
				    0,
				    NULL,
				    NULL);

	Status = NtOpenSymbolicLinkObject (&Handle,
					   SYMBOLIC_LINK_ALL_ACCESS,
					   &ObjectAttributes);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT("Failed to open link %wZ\n",
		    &DriveName);
	     continue;
	  }

	Status = NtQuerySymbolicLinkObject (Handle,
					    &DriveDeviceName,
					    &Length);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT("Failed query open link %wZ\n",
		    &DriveName);
	     continue;
	  }
	DPRINT("Opened link: %wZ ==> %wZ\n",
	       &DriveName, &DriveDeviceName);

	if (!RtlCompareUnicodeString (&ArcDeviceName, &DriveDeviceName, FALSE))
	  {
	     DPRINT("DOS Boot path: %c:%wZ\n", 'A' + i, &BootPath);
	     swprintf (SharedPage->NtSystemRoot,
		       L"%C:%wZ", 'A' + i, &BootPath);

		BootDriveFound = TRUE;
	  }

	NtClose (Handle);

	/* set bit in dos drives bitmap (drive available) */
	SharedPage->DosDeviceMap |= (1<<i);
     }

   RtlFreeUnicodeString (&BootPath);
   RtlFreeUnicodeString (&DriveDeviceName);
   RtlFreeUnicodeString (&ArcDeviceName);

   DPRINT("DosDeviceMap: 0x%x\n", SharedPage->DosDeviceMap);

   if (BootDriveFound == FALSE)
     {
	DbgPrint("No system drive found!\n");
	KeBugCheck (0x0);
     }
}

#ifndef NDEBUG

VOID DumpBIOSMemoryMap(VOID)
{
  ULONG i;

  DbgPrint("Dumping BIOS memory map:\n");
  DbgPrint("Memory map base: %d\n", KeLoaderBlock.MmapAddr);
  DbgPrint("Memory map size: %d\n", KeLoaderBlock.MmapLength);
  DbgPrint("Address range count: %d\n", KeMemoryMapRangeCount);
  for (i = 0; i < KeMemoryMapRangeCount; i++)
    {
      DbgPrint("Range: Base (%08X)  Length (%08X)  Type (%02X)\n",
        KeMemoryMap[i].BaseAddrLow,
        KeMemoryMap[i].LengthLow,
        KeMemoryMap[i].Type);
    }
  for (;;);
}

#endif /* !NDEBUG */

#if 1
// SEH Test

static ULONG Scratch;

EXCEPTION_DISPOSITION
ExpUnhandledException1(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION ExceptionRegistration,
  PCONTEXT Context,
  PVOID DispatcherContext)
{
  DbgPrint("ExpUnhandledException1() called\n");
  DbgPrint("ExceptionRecord 0x%X\n", ExceptionRecord);
  DbgPrint("  Flags 0x%X\n", ExceptionRecord->ExceptionFlags);
  DbgPrint("ExceptionRegistration 0x%X\n", ExceptionRegistration);
  DbgPrint("Context 0x%X\n", Context);
  DbgPrint("DispatcherContext 0x%X\n", DispatcherContext);

  Context->Eax = (ULONG)&Scratch;

  return ExceptionContinueExecution;
}


EXCEPTION_DISPOSITION
ExpUnhandledException2(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION ExceptionRegistration,
  PCONTEXT Context,
  PVOID DispatcherContext)
{
  DbgPrint("ExpUnhandledException2() called\n");
  DbgPrint("ExceptionRecord 0x%X\n", ExceptionRecord);
  DbgPrint("  Flags 0x%X\n", ExceptionRecord->ExceptionFlags);
  DbgPrint("ExceptionRegistration 0x%X\n", ExceptionRegistration);
  DbgPrint("Context 0x%X\n", Context);
  DbgPrint("DispatcherContext 0x%X\n", DispatcherContext);

#if 1
  Context->Eax = (ULONG)&Scratch;

  return ExceptionContinueExecution;

#else

  return ExceptionContinueSearch;

#endif
}


#if 1
// Put in mingw headers
extern VOID
CDECL
_local_unwind2(
  PEXCEPTION_REGISTRATION RegistrationFrame,
  DWORD TryLevel);

extern VOID
CDECL
_global_unwind2(
  PVOID RegistrationFrame);

extern EXCEPTION_DISPOSITION
CDECL
_except_handler2(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION RegistrationFrame,
  PCONTEXT Context,
  PVOID DispatcherContext);

extern EXCEPTION_DISPOSITION
CDECL
_except_handler3(
  PEXCEPTION_RECORD ExceptionRecord,
  PEXCEPTION_REGISTRATION RegistrationFrame,
  PCONTEXT Context,
  PVOID DispatcherContext);

#endif

PRTL_EXCEPTION_REGISTRATION
CurrentRER(VOID)
{
   ULONG Value;
   
   __asm__("movl %%ebp, %0\n\t" : "=a" (Value));

   return((PRTL_EXCEPTION_REGISTRATION)Value) - 1;
}

PULONG x;
PRTL_EXCEPTION_REGISTRATION TestER;
SCOPETABLE_ENTRY ScopeTable;
PEXCEPTION_REGISTRATION OSPtr;


DWORD CDECL SEHFilterRoutine(VOID)
{
  DbgPrint("Within filter routine.\n");
  return EXCEPTION_EXECUTE_HANDLER;
  //return EXCEPTION_CONTINUE_EXECUTION;
}

VOID CDECL SEHHandlerRoutine(VOID)
{
  DbgPrint("Within exception handler.\n");
  DbgPrint("System halted.\n");
  for (;;);
}


VOID SEHTest()
{
  RTL_EXCEPTION_REGISTRATION ER;
  LPEXCEPTION_POINTERS ExceptionPointers;
  PVOID StandardESPInFrame;

  __asm__ ("movl %%esp,%%eax;" : "=a" (StandardESPInFrame));
  DbgPrint("StandardESPInFrame: 0x%X\n", StandardESPInFrame);

  ExceptionPointers = NULL;

  ER.OS.handler = _except_handler3;
  __asm__ ("movl %%fs:0,%%eax;" : "=a" (ER.OS.prev));
  DbgPrint("ER.OS.prev: 0x%X\n", ER.OS.prev);

  ER.ScopeTable = &ScopeTable;
  DbgPrint("ER.ScopeTable: 0x%X\n", ER.ScopeTable);
  ER.TryLevel = -1;
  __asm__ ("movl %%ebp,%%eax;" : "=a" (ER.Ebp));
  DbgPrint("ER.Ebp: 0x%X\n", ER.Ebp);

  ScopeTable.PreviousTryLevel = -1;
  ScopeTable.FilterRoutine = SEHFilterRoutine;
  DbgPrint("ScopeTable.FilterRoutine: 0x%X\n", ScopeTable.FilterRoutine);
  ScopeTable.HandlerRoutine = SEHHandlerRoutine;
  DbgPrint("ScopeTable.HandlerRoutine: 0x%X\n", ScopeTable.HandlerRoutine);


  OSPtr = &ER.OS;
  DbgPrint("OSPtr: 0x%X\n", OSPtr);

  __asm__ ("movl %0,%%eax;movl %%eax,%%fs:0;" : : "m" (OSPtr));

  /*__try1(__except_handler3)*/ if(1) {
    ER.TryLevel = 0; // Entered first try... block

    DbgPrint("Within guarded section.\n");
    x = (PULONG)0xf2000000; *x = 0;
    DbgPrint("After exception.\n");
  } /* __except1 */ if(0) {
  }

  DbgPrint("After exception2.\n");

  __asm__ ("movl %0,%%eax;movl %%eax,%%fs:0;" : : "m" (ER.OS.prev));
  //KeGetCurrentKPCR()->ExceptionList = ER.OS.prev;

  DbgPrint("Exiting.\n");
}

#endif

VOID
ExpInitializeExecutive(VOID)
{
  ULONG i;
  ULONG start;
  ULONG length;
  PCHAR name;
  CHAR str[50];

  /*
   * Fail at runtime if someone has changed various structures without
   * updating the offsets used for the assembler code.
   */
  assert(FIELD_OFFSET(KTHREAD, InitialStack) == KTHREAD_INITIAL_STACK);
  assert(FIELD_OFFSET(KTHREAD, Teb) == KTHREAD_TEB);
  assert(FIELD_OFFSET(KTHREAD, KernelStack) == KTHREAD_KERNEL_STACK);
  assert(FIELD_OFFSET(KTHREAD, PreviousMode) == KTHREAD_PREVIOUS_MODE);
  assert(FIELD_OFFSET(KTHREAD, TrapFrame) == KTHREAD_TRAP_FRAME);
  assert(FIELD_OFFSET(ETHREAD, ThreadsProcess) == ETHREAD_THREADS_PROCESS);
  assert(FIELD_OFFSET(KPROCESS, DirectoryTableBase) == 
	 KPROCESS_DIRECTORY_TABLE_BASE);
  assert(FIELD_OFFSET(KTRAP_FRAME, Reserved9) == KTRAP_FRAME_RESERVED9);
  assert(FIELD_OFFSET(KV86M_TRAP_FRAME, regs) == TF_REGS);
  assert(FIELD_OFFSET(KV86M_TRAP_FRAME, orig_ebp) == TF_ORIG_EBP);
  
  assert(FIELD_OFFSET(KPCR, ExceptionList) == KPCR_EXCEPTION_LIST);
  assert(FIELD_OFFSET(KPCR, Self) == KPCR_SELF);
  assert(FIELD_OFFSET(KPCR, CurrentThread) == KPCR_CURRENT_THREAD);

  LdrInit1();

  KeLowerIrql(DISPATCH_LEVEL);
  
  NtEarlyInitVdm();
  
  MmInit1(FirstKrnlPhysAddr,
    LastKrnlPhysAddr,
    LastKernelAddress,
    (PADDRESS_RANGE)&KeMemoryMap,
    KeMemoryMapRangeCount);
  
  /* create default nls tables */
  RtlpInitNlsTables();
  
  /*
   * Initialize the kernel debugger
   */
  KdInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
  if (KdPollBreakIn ())
    {
      DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);
    }
  
  MmInit2();
  KeInit2();
  
  KeLowerIrql(PASSIVE_LEVEL);

  ObInit();
  PiInitProcessManager();
  
  /*
   * Display version number and copyright/warranty message
   */
  HalDisplayString("Starting ReactOS "KERNEL_VERSION_STR" (Build "
		   KERNEL_VERSION_BUILD_STR")\n");
  HalDisplayString(RES_STR_LEGAL_COPYRIGHT);
  HalDisplayString("\n\nReactOS is free software, covered by the GNU General "
		   "Public License, and you\n");
  HalDisplayString("are welcome to change it and/or distribute copies of it "
		   "under certain\n"); 
  HalDisplayString("conditions. There is absolutely no warranty for "
		   "ReactOS.\n\n");

  /* Initialize all processors */
  KeNumberProcessors = 0;

  while (!HalAllProcessorsStarted())
    {
      PVOID ProcessorStack;

      if (KeNumberProcessors != 0)
	{
	  KePrepareForApplicationProcessorInit(KeNumberProcessors);
	  PsPrepareForApplicationProcessorInit(KeNumberProcessors);
	}
      /* Allocate a stack for use when booting the processor */
      /* FIXME: The nonpaged memory for the stack is not released after use */
      ProcessorStack = 
	ExAllocatePool(NonPagedPool, MM_STACK_SIZE) + MM_STACK_SIZE;
      Ki386InitialStackArray[((int)KeNumberProcessors)] = 
	(PVOID)(ProcessorStack - MM_STACK_SIZE);
      HalInitializeProcessor(KeNumberProcessors, ProcessorStack);
      KeNumberProcessors++;
    }

  if (KeNumberProcessors > 1)
    {
      sprintf(str,
	      "Found %d system processors. [%lu MB Memory]\n",
	      KeNumberProcessors,
	      (KeLoaderBlock.MemHigher + 1088)/ 1024);
    }
  else
    {
      sprintf(str,
	      "Found 1 system processor. [%lu MB Memory]\n",
	      (KeLoaderBlock.MemHigher + 1088)/ 1024);
    }
  HalDisplayString(str);

  /*
   * Initialize various critical subsystems
   */
  HalInitSystem(1, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  ExInit();
  IoInit();
  PoInit();
  LdrInitModuleManagement();
  CmInitializeRegistry();
  NtInit();
  MmInit3();
  CcInit();
  
  /* Report all resources used by hal */
  HalReportResourceUsage();
  
  /*
   * Initalize services loaded at boot time
   */
  DPRINT1("%d files loaded\n",KeLoaderBlock.ModsCount);
  for (i=0; i < KeLoaderBlock.ModsCount; i++)
    {
      CPRINT("Module: '%s' at %08lx, length 0x%08lx\n",
       KeLoaderModules[i].String,
       KeLoaderModules[i].ModStart,
       KeLoaderModules[i].ModEnd - KeLoaderModules[i].ModStart);
    }

  /*  Pass 1: load nls files  */
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {
      name = (PCHAR)KeLoaderModules[i].String;
      if (RtlpCheckFileNameExtension(name, ".nls"))
	{
	  ULONG Mod2Start = 0;
	  ULONG Mod2End = 0;
	  ULONG Mod3Start = 0;
	  ULONG Mod3End = 0;

	  name = (PCHAR)KeLoaderModules[i+1].String;
	  if (RtlpCheckFileNameExtension(name, ".nls"))
	    {
	      Mod2Start = (ULONG)KeLoaderModules[i+1].ModStart;
	      Mod2End = (ULONG)KeLoaderModules[i+1].ModEnd;

	      name = (PCHAR)KeLoaderModules[i+2].String;
	      if (RtlpCheckFileNameExtension(name, ".nls"))
	        {
		  Mod3Start = (ULONG)KeLoaderModules[i+2].ModStart;
		  Mod3End = (ULONG)KeLoaderModules[i+2].ModEnd;
	        }
	    }

	  /* Initialize nls sections */
	  RtlpInitNlsSections((ULONG)KeLoaderModules[i].ModStart,
			      (ULONG)KeLoaderModules[i].ModEnd,
			      Mod2Start,
			      Mod2End,
			      Mod3Start,
			      Mod3End);
	  break;
	}
    }

  /*  Pass 2: load registry chunks passed in  */
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {
      start = KeLoaderModules[i].ModStart;
      length = KeLoaderModules[i].ModEnd - start;
      name = (PCHAR)KeLoaderModules[i].String;
      if (RtlpCheckFileNameExtension(name, "") ||
	  RtlpCheckFileNameExtension(name, ".hiv"))
	{
	  CPRINT("Process registry chunk at %08lx\n", start);
	  CmImportHive((PCHAR)start, length);
	}
    }

  /*
   * Enter the kernel debugger before starting up the boot drivers
   */
#ifdef KDBG
  KdbEnter();
#endif /* KDBG */

  /*  Pass 3: process boot loaded drivers  */
  for (i=1; i < KeLoaderBlock.ModsCount; i++)
    {
      start = KeLoaderModules[i].ModStart;
      length = KeLoaderModules[i].ModEnd - start;
      name = (PCHAR)KeLoaderModules[i].String;
      if (RtlpCheckFileNameExtension(name, ".sys") ||
	  RtlpCheckFileNameExtension(name, ".sym"))
	{
	  CPRINT("Processing module '%s' at %08lx, length 0x%08lx\n",
	         name, start, length);
	  LdrProcessDriver((PVOID)start, name, length);
	}
    }
  
  /* Create the SystemRoot symbolic link */
  CPRINT("CommandLine: %s\n", (PUCHAR)KeLoaderBlock.CommandLine);

  CreateSystemRootLink ((PUCHAR)KeLoaderBlock.CommandLine);

#ifdef DBGPRINT_FILE_LOG
  /* On the assumption that we can now access disks start up the debug 
     logger thread */
  DebugLogInit2();
#endif /* DBGPRINT_FILE_LOG */
  

  CmInitializeRegistry2();

#if 0
  CreateDefaultRegistry();
#endif

  PiInitDefaultLocale();

  /*
   * Start the motherboard enumerator (the HAL)
   */
  HalInitSystem(2, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
#if 0
  /*
   * Load boot start drivers
   */
  IopLoadBootStartDrivers();
#else
  /*
   * Load Auto configured drivers
   */
  LdrLoadAutoConfigDrivers();
#endif
  /*
   * Assign drive letters
   */
  IoAssignDriveLetters ((PLOADER_PARAMETER_BLOCK)&KeLoaderBlock,
			NULL,
			NULL,
			NULL);

  /*
   * Initialize shared user page:
   *  - set dos system path, dos device map, etc.
   */
  InitSystemSharedUserPage ((PUCHAR)KeLoaderBlock.CommandLine);

  /*
   *  Launch initial process
   */
  LdrLoadInitialProcess();

  PsTerminateSystemThread(STATUS_SUCCESS);
}


VOID
KiSystemStartup(BOOLEAN BootProcessor)
{
  HalInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  if (BootProcessor)
    {
      /* Never returns */
      ExpInitializeExecutive();
      KeBugCheck(0);
    }
  /* Do application processor initialization */
  KeApplicationProcessorInit();
  PsApplicationProcessorInit();
  KeLowerIrql(PASSIVE_LEVEL);
  PsIdleThreadMain(NULL);
  KeBugCheck(0);
  for(;;);
}

VOID
_main (ULONG MultiBootMagic, PLOADER_PARAMETER_BLOCK _LoaderBlock)
/*
 * FUNCTION: Called by the boot loader to start the kernel
 * ARGUMENTS:
 *          LoaderBlock = Pointer to boot parameters initialized by the boot 
 *                        loader
 * NOTE: The boot parameters are stored in low memory which will become
 * invalid after the memory managment is initialized so we make a local copy.
 */
{
  ULONG i;
  ULONG size;
  ULONG last_kernel_address;
  extern ULONG _bss_end__;
  ULONG HalBase;
  ULONG DriverBase;
  ULONG DriverSize;

  /* Low level architecture specific initialization */
  KeInit1();

  /*
   * Copy the parameters to a local buffer because lowmem will go away
   */
  memcpy (&KeLoaderBlock, _LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));
  memcpy (&KeLoaderModules[1], (PVOID)KeLoaderBlock.ModsAddr,
	  sizeof(LOADER_MODULE) * KeLoaderBlock.ModsCount);
  KeLoaderBlock.ModsCount++;
  KeLoaderBlock.ModsAddr = (ULONG)&KeLoaderModules;
  
  /*
   * FIXME: Preliminary hack!!!! Add boot device to beginning of command line.
   * This should be done by the boot loader.
   */
  strcpy (KeLoaderCommandLine,
	  "multi(0)disk(0)rdisk(0)partition(1)\\reactos /DEBUGPORT=SCREEN");
  strcat (KeLoaderCommandLine, (PUCHAR)KeLoaderBlock.CommandLine);
  
  KeLoaderBlock.CommandLine = (ULONG)KeLoaderCommandLine;
  strcpy(KeLoaderModuleStrings[0], "ntoskrnl.exe");
  KeLoaderModules[0].String = (ULONG)KeLoaderModuleStrings[0];
  KeLoaderModules[0].ModStart = 0xC0000000;
  KeLoaderModules[0].ModEnd = PAGE_ROUND_UP((ULONG)&_bss_end__);
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {
      strcpy(KeLoaderModuleStrings[i], (PUCHAR)KeLoaderModules[i].String);
      KeLoaderModules[i].ModStart -= 0x200000;
      KeLoaderModules[i].ModStart += 0xc0000000;
      KeLoaderModules[i].ModEnd -= 0x200000;
      KeLoaderModules[i].ModEnd += 0xc0000000;
      KeLoaderModules[i].String = (ULONG)KeLoaderModuleStrings[i];
    }

#ifdef HAL_DBG
  HalnInitializeDisplay((PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
#endif

  HalBase = KeLoaderModules[1].ModStart;
  DriverBase = KeLoaderModules[KeLoaderBlock.ModsCount - 1].ModEnd;

  /*
   * Process hal.dll
   */
  LdrSafePEProcessModule((PVOID)HalBase, (PVOID)DriverBase, (PVOID)0xC0000000, &DriverSize);

  LdrHalBase = (ULONG_PTR)DriverBase;
  last_kernel_address = DriverBase + DriverSize;

  /*
   * Process ntoskrnl.exe
   */
  LdrSafePEProcessModule((PVOID)0xC0000000, (PVOID)0xC0000000, (PVOID)DriverBase, &DriverSize);

  FirstKrnlPhysAddr = KeLoaderModules[0].ModStart - 0xc0000000 + 0x200000;
  LastKrnlPhysAddr = last_kernel_address - 0xc0000000 + 0x200000;
  LastKernelAddress = last_kernel_address;

#ifndef ACPI
  /* FIXME: VMware does not like it when ReactOS is using the BIOS memory map */
  KeLoaderBlock.Flags &= ~MB_FLAGS_MMAP_INFO;
#endif

  KeMemoryMapRangeCount = 0;
  if (KeLoaderBlock.Flags & MB_FLAGS_MMAP_INFO)
    {
      /* We have a memory map from the nice BIOS */
      size = *((PULONG)(KeLoaderBlock.MmapAddr - sizeof(ULONG)));
      i = 0;
      while (i < KeLoaderBlock.MmapLength)
        {
          memcpy (&KeMemoryMap[KeMemoryMapRangeCount],
            (PVOID)(KeLoaderBlock.MmapAddr + i),
	          sizeof(ADDRESS_RANGE));
          KeMemoryMapRangeCount++;
          i += size;
        }
    }
  
  KiSystemStartup(1);
}

/* EOF */

