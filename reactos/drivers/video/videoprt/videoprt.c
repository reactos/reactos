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
 * $Id: videoprt.c,v 1.21.2.2 2004/03/15 17:02:14 navaraf Exp $
 */

#include "videoprt.h"
#include "internal/ps.h"

/* GLOBAL VARIABLES ***********************************************************/

ULONG CsrssInitialized = FALSE;
PEPROCESS Csrss = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS STDCALL
DriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath)
{
   return STATUS_SUCCESS;
}

PVOID STDCALL
VideoPortImageDirectoryEntryToData(
   PVOID BaseAddress,
   ULONG Directory)
{
   PIMAGE_NT_HEADERS NtHeader;
   ULONG Va;
  
   NtHeader = RtlImageNtHeader(BaseAddress);
   if (NtHeader == NULL)
      return NULL;
  
   if (Directory >= NtHeader->OptionalHeader.NumberOfRvaAndSizes)
      return NULL;
  
   Va = NtHeader->OptionalHeader.DataDirectory[Directory].VirtualAddress;
   if (Va == 0)
      return NULL;
  
   return (PVOID)(BaseAddress + Va);
}

PVOID STDCALL
VideoPortGetProcAddress(
   IN PVOID HwDeviceExtension,
   IN PUCHAR FunctionName)
{
   SYSTEM_LOAD_IMAGE GdiDriverInfo;
   PVOID BaseAddress;
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PUSHORT OrdinalPtr;
   PULONG NamePtr;
   PULONG AddressPtr;
   ULONG i = 0;
   NTSTATUS Status;

   DPRINT("VideoPortGetProcAddress(%s)\n", FunctionName);

   RtlInitUnicodeString(&GdiDriverInfo.ModuleName, L"videoprt");
   Status = ZwSetSystemInformation(
      SystemLoadImage,
      &GdiDriverInfo, 
      sizeof(SYSTEM_LOAD_IMAGE));
   if (!NT_SUCCESS(Status))
   {
      DPRINT("Couldn't get our own module handle?\n");
      return NULL;
   }

   BaseAddress = GdiDriverInfo.ModuleBase;

   /* Get the pointer to the export directory */
   ExportDir = (PIMAGE_EXPORT_DIRECTORY)VideoPortImageDirectoryEntryToData(
      BaseAddress,
      IMAGE_DIRECTORY_ENTRY_EXPORT);

   /* Search by name */
   AddressPtr = (PULONG)
      ((ULONG_PTR)BaseAddress + (ULONG_PTR)ExportDir->AddressOfFunctions);
   OrdinalPtr = (PUSHORT)
      ((ULONG_PTR)BaseAddress + (ULONG_PTR)ExportDir->AddressOfNameOrdinals);
   NamePtr = (PULONG)
      ((ULONG_PTR)BaseAddress + (ULONG_PTR)ExportDir->AddressOfNames);
   for (i = 0; i < ExportDir->NumberOfNames; i++, NamePtr++, OrdinalPtr++)
   {
      if (!_strnicmp(FunctionName, (char*)(BaseAddress + *NamePtr),
                     strlen(FunctionName)))
      {
         return (PVOID)((ULONG_PTR)BaseAddress + 
                        (ULONG_PTR)AddressPtr[*OrdinalPtr]);	  
      }
   }

   DPRINT("VideoPortGetProcAddress: Can't resolve symbol %s\n", FunctionName);

   return NULL;
}

VOID FASTCALL 
IntAttachToCSRSS(PEPROCESS *CallingProcess, PEPROCESS *PrevAttachedProcess) 
{ 
   *CallingProcess = PsGetCurrentProcess(); 
   if (*CallingProcess != Csrss) 
   { 
      if (PsGetCurrentThread()->OldProcess != NULL)
      { 
         *PrevAttachedProcess = *CallingProcess; 
         KeDetachProcess(); 
      } 
      else 
      { 
         *PrevAttachedProcess = NULL; 
      } 
      KeAttachProcess(Csrss); 
   } 
} 
 
VOID FASTCALL 
IntDetachFromCSRSS(PEPROCESS *CallingProcess, PEPROCESS *PrevAttachedProcess) 
{ 
   if (*CallingProcess != Csrss) 
   { 
      KeDetachProcess(); 
      if (NULL != *PrevAttachedProcess) 
      { 
         KeAttachProcess(*PrevAttachedProcess); 
      } 
   } 
} 

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

ULONG STDCALL
VideoPortInitialize(
   IN PVOID Context1,
   IN PVOID Context2,
   IN PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
   IN PVOID HwContext)
{
   PDRIVER_OBJECT DriverObject = Context1;
   PUNICODE_STRING RegistryPath = Context2;
   NTSTATUS Status;
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

   DPRINT("VideoPortInitialize\n");

   DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
   if (DriverExtension != NULL)
   {
      DPRINT1(
         "Oops, we were called twice to initialize the driver. This propably\n"
         "means that we were loaded by legacy driver, so we have to fallback\n"
         "to the old driver detection paradigm. Unfortunetly this case isn't\n"
         "implemented yet.");

      return STATUS_UNSUCCESSFUL;
   }

   Status = IoAllocateDriverObjectExtension(
      DriverObject,
      DriverObject,
      sizeof(VIDEO_PORT_DRIVER_EXTENSION),
      (PVOID *)&DriverExtension);

   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   RtlCopyMemory(
      &DriverExtension->InitializationData,
      HwInitializationData,
      sizeof(VIDEO_HW_INITIALIZATION_DATA));
   DriverExtension->HwContext = HwContext;

   RtlCopyMemory(&DriverExtension->RegistryPath, RegistryPath, sizeof(UNICODE_STRING));

#ifndef NDEBUG
   switch (HwInitializationData->HwInitDataSize)
   {
      case SIZE_OF_NT4_VIDEO_HW_INITIALIZATION_DATA:
         DPRINT("We were loaded by a Windows NT miniport driver.\n"); break;
      case SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA:
         DPRINT("We were loaded by a Windows 2000 miniport driver.\n"); break;
      case sizeof(VIDEO_HW_INITIALIZATION_DATA):
         DPRINT("We were loaded by a Windows XP or later miniport driver.\n"); break;
      default:
         DPRINT("Invalid HwInitializationData size.\n"); break;
   }
#endif

   DriverObject->DriverExtension->AddDevice = VideoPortAddDevice;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = VideoPortDispatchOpen;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = VideoPortDispatchClose;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VideoPortDispatchDeviceControl;
   DriverObject->MajorFunction[IRP_MJ_PNP] = VideoPortDispatchPnp;
   DriverObject->MajorFunction[IRP_MJ_POWER] = VideoPortDispatchPower;
   DriverObject->DriverUnload = VideoPortUnload;

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */

VOID
VideoPortDebugPrint(
   IN VIDEO_DEBUG_LEVEL DebugPrintLevel,
   IN PCHAR DebugMessage, ...)
{
   char Buffer[256];
   va_list ap;

   va_start(ap, DebugMessage);
   vsprintf(Buffer, DebugMessage, ap);
   va_end(ap);

   DbgPrint(Buffer);
}

/*
 * @unimplemented
 */

VOID STDCALL
VideoPortLogError(
   IN PVOID HwDeviceExtension,
   IN PVIDEO_REQUEST_PACKET Vrp OPTIONAL,
   IN VP_STATUS ErrorCode,
   IN ULONG UniqueId)
{
   DPRINT1("VideoPortLogError ErrorCode %d (0x%x) UniqueId %lu (0x%lx)\n",
           ErrorCode, ErrorCode, UniqueId, UniqueId);
   if (NULL != Vrp)
   {
      DPRINT1("Vrp->IoControlCode %lu (0x%lx)\n", Vrp->IoControlCode, Vrp->IoControlCode);
   }
}

/*
 * @implemented
 */

ULONG STDCALL
VideoPortGetBusData(
   IN PVOID HwDeviceExtension,
   IN BUS_DATA_TYPE BusDataType,
   IN ULONG SlotNumber,
   OUT PVOID Buffer,
   IN ULONG Offset,
   IN ULONG Length)
{
   DPRINT("VideoPortGetBusData\n");

   return HalGetBusDataByOffset(
      BusDataType, 
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension)->SystemIoBusNumber,
      BusDataType == Cmos ? SlotNumber :
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension)->SystemIoSlotNumber,
      Buffer, 
      Offset, 
      Length);
}

/*
 * @implemented
 */

ULONG STDCALL
VideoPortSetBusData(
   IN PVOID HwDeviceExtension,
   IN BUS_DATA_TYPE BusDataType,
   IN ULONG SlotNumber,
   IN PVOID Buffer,
   IN ULONG Offset,
   IN ULONG Length)
{
   DPRINT("VideoPortSetBusData\n");
   return HalSetBusDataByOffset(
      BusDataType,
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension)->SystemIoBusNumber,
      BusDataType == Cmos ? SlotNumber :
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension)->SystemIoSlotNumber,
      Buffer,
      Offset,
      Length);
}

/*
 * @implemented
 */

UCHAR STDCALL
VideoPortGetCurrentIrql(VOID)
{
   return KeGetCurrentIrql();
}

typedef struct QueryRegistryCallbackContext
{
  PVOID HwDeviceExtension;
  PVOID HwContext;
  PMINIPORT_GET_REGISTRY_ROUTINE HwGetRegistryRoutine;
} QUERY_REGISTRY_CALLBACK_CONTEXT, *PQUERY_REGISTRY_CALLBACK_CONTEXT;

static NTSTATUS STDCALL
QueryRegistryCallback(
   IN PWSTR ValueName,
   IN ULONG ValueType,
   IN PVOID ValueData,
   IN ULONG ValueLength,
   IN PVOID Context,
   IN PVOID EntryContext)
{
   PQUERY_REGISTRY_CALLBACK_CONTEXT CallbackContext = (PQUERY_REGISTRY_CALLBACK_CONTEXT) Context;

   DPRINT("Found registry value for name %S: type %d, length %d\n",
          ValueName, ValueType, ValueLength);
   return (*(CallbackContext->HwGetRegistryRoutine))(
      CallbackContext->HwDeviceExtension,
      CallbackContext->HwContext,
      ValueName,
      ValueData,
      ValueLength);
}

/*
 * @unimplemented
 */

VP_STATUS STDCALL
VideoPortGetRegistryParameters(
   IN PVOID HwDeviceExtension,
   IN PWSTR ParameterName,
   IN UCHAR IsParameterFileName,
   IN PMINIPORT_GET_REGISTRY_ROUTINE GetRegistryRoutine,
   IN PVOID HwContext)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];
   QUERY_REGISTRY_CALLBACK_CONTEXT Context;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   DPRINT("VideoPortGetRegistryParameters ParameterName %S\n", ParameterName);

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

   if (IsParameterFileName)
   {
      UNIMPLEMENTED;
   }

   Context.HwDeviceExtension = HwDeviceExtension;
   Context.HwContext = HwContext;
   Context.HwGetRegistryRoutine = GetRegistryRoutine;

   QueryTable[0].QueryRoutine = QueryRegistryCallback;
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
   QueryTable[0].Name = ParameterName;
   QueryTable[0].EntryContext = NULL;
   QueryTable[0].DefaultType = REG_NONE;
   QueryTable[0].DefaultData = NULL;
   QueryTable[0].DefaultLength = 0;

   QueryTable[1].QueryRoutine = NULL;
   QueryTable[1].Name = NULL;

   return NT_SUCCESS(RtlQueryRegistryValues(
      RTL_REGISTRY_ABSOLUTE,
      DeviceExtension->RegistryPath.Buffer,
      QueryTable,
      &Context,
      NULL)) ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortSetRegistryParameters(
   IN PVOID HwDeviceExtension,
   IN PWSTR ValueName,
   IN PVOID ValueData,
   IN ULONG ValueLength)
{
   DPRINT("VideoSetRegistryParameters\n");
   assert_irql(PASSIVE_LEVEL);
   return RtlWriteRegistryValue(
      RTL_REGISTRY_ABSOLUTE,
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension)->RegistryPath.Buffer,
      ValueName,
      REG_BINARY,
      ValueData,
      ValueLength);
}

/*
 * @implemented
 */ 

VP_STATUS STDCALL
VideoPortGetVgaStatus(
   IN PVOID HwDeviceExtension,
   OUT PULONG VgaStatus)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   DPRINT("VideoPortGetVgaStatus\n");

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   if (KeGetCurrentIrql() == PASSIVE_LEVEL)
   {
      if (DeviceExtension->AdapterInterfaceType == PCIBus)
      {
         /* VgaStatus: 0 == VGA not enabled, 1 == VGA enabled. */
         /* Assumed for now */
         *VgaStatus = 1;
         return NO_ERROR;
      }
   }

   return ERROR_INVALID_FUNCTION;    
}

/*
 * @implemented
 */

PVOID STDCALL
VideoPortGetRomImage(
   IN PVOID HwDeviceExtension,
   IN PVOID Unused1,
   IN ULONG Unused2,
   IN ULONG Length)
{
   static PVOID RomImageBuffer = NULL;
   PEPROCESS CallingProcess; 
   PEPROCESS PrevAttachedProcess; 

   DPRINT("VideoPortGetRomImage(HwDeviceExtension 0x%X Length 0x%X)\n",
          HwDeviceExtension, Length);

   /* If the length is zero then free the existing buffer. */
   if (Length == 0)
   {
      if (RomImageBuffer != NULL)
      {
         ExFreePool(RomImageBuffer);
         RomImageBuffer = NULL;
      }
      return NULL;
   }
   else
   {
      /*
       * The DDK says we shouldn't use the legacy C0000 method but get the  
       * rom base address from the corresponding pci or acpi register but  
       * lets ignore that and use C0000 anyway. We have already mapped the
       * bios area into memory so we'll copy from there.                   
       */

      /* Copy the bios. */
      Length = min(Length, 0x10000);
      if (RomImageBuffer != NULL)
      {
         ExFreePool(RomImageBuffer);
      }

      RomImageBuffer = ExAllocatePool(PagedPool, Length);
      if (RomImageBuffer == NULL)
      {
         return NULL;
      }

      IntAttachToCSRSS(&CallingProcess, &PrevAttachedProcess);
      RtlCopyMemory(RomImageBuffer, (PUCHAR)0xC0000, Length);
      IntDetachFromCSRSS(&CallingProcess, &PrevAttachedProcess);

      return RomImageBuffer;
   }
}

/*
 * @implemented
 */

BOOLEAN STDCALL
VideoPortScanRom(
   IN PVOID HwDeviceExtension, 
   IN PUCHAR RomBase,
   IN ULONG RomLength,
   IN PUCHAR String)
{
   ULONG StringLength;
   BOOLEAN Found;
   PUCHAR SearchLocation;

   DPRINT("VideoPortScanRom RomBase %p RomLength 0x%x String %s\n", RomBase, RomLength, String);

   StringLength = strlen(String);
   Found = FALSE;
   SearchLocation = RomBase;
   for (SearchLocation = RomBase;
        !Found && SearchLocation < RomBase + RomLength - StringLength;
        SearchLocation++)
   {
      Found = (RtlCompareMemory(SearchLocation, String, StringLength) == StringLength);
      if (Found)
      {
         DPRINT("Match found at %p\n", SearchLocation);
      }
   }

   return Found;
}

/*
 * @implemented
 */

BOOLEAN STDCALL
VideoPortSynchronizeExecution(
   IN PVOID HwDeviceExtension,
   IN VIDEO_SYNCHRONIZE_PRIORITY Priority,
   IN PMINIPORT_SYNCHRONIZE_ROUTINE SynchronizeRoutine,
   OUT PVOID Context)
{
   BOOLEAN Ret;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   KIRQL OldIrql;

   switch (Priority)
   {
      case VpLowPriority:
         Ret = (*SynchronizeRoutine)(Context);
         break;
   
      case VpMediumPriority:
         DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
         if (DeviceExtension->InterruptObject == NULL)
            Ret = (*SynchronizeRoutine)(Context);
         else
            Ret = KeSynchronizeExecution(
               DeviceExtension->InterruptObject,
               SynchronizeRoutine,
               Context);
         break;

      case VpHighPriority:
         OldIrql = KeGetCurrentIrql();
         if (OldIrql < SYNCH_LEVEL)
            OldIrql = KfRaiseIrql(SYNCH_LEVEL);

         Ret = (*SynchronizeRoutine)(Context);

         if (OldIrql < SYNCH_LEVEL)
            KfLowerIrql(OldIrql);
         break;

      default:
         Ret = FALSE;
   }

   return Ret;
}

/*
 * @unimplemented
 */

BOOLEAN STDCALL
VideoPortDDCMonitorHelper(
   PVOID HwDeviceExtension,
   /*PI2C_FNC_TABLE*/PVOID I2CFunctions,
   PUCHAR pEdidBuffer,
   ULONG EdidBufferSize
   )
{
   DPRINT1("VideoPortDDCMonitorHelper() - Unimplemented.\n");
   return FALSE;
}

/*
 * @unimplemented
 */

VP_STATUS STDCALL
VideoPortEnumerateChildren(
   IN PVOID HwDeviceExtension,
   IN PVOID Reserved)
{
   DPRINT1("VideoPortEnumerateChildren(): Unimplemented.\n");
   return NO_ERROR;
}

/*
 * @unimplemented
 */

VP_STATUS STDCALL
VideoPortCreateSecondaryDisplay(
   IN PVOID HwDeviceExtension,
   IN OUT PVOID *SecondaryDeviceExtension,
   IN ULONG Flag)
{
   DPRINT1("VideoPortCreateSecondaryDisplay: Unimplemented.\n");
   return NO_ERROR;
}

/*
 * @implemented
 */

BOOLEAN STDCALL
VideoPortQueueDpc(
   IN PVOID HwDeviceExtension,
   IN PMINIPORT_DPC_ROUTINE CallbackRoutine,
   IN PVOID Context)
{
   return KeInsertQueueDpc(
      &VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension)->DpcObject, 
      (PVOID)CallbackRoutine,
      (PVOID)Context);
}

/*
 * @unimplemented
 */

PVOID STDCALL
VideoPortGetAssociatedDeviceExtension(IN PVOID DeviceObject)
{
   DPRINT1("VideoPortGetAssociatedDeviceExtension: Unimplemented.\n");
   return NULL;
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortGetVersion(
   IN PVOID HwDeviceExtension,
   IN OUT PVPOSVERSIONINFO VpOsVersionInfo)
{
   RTL_OSVERSIONINFOEXW Version;

   Version.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
   if (VpOsVersionInfo->Size >= sizeof(VPOSVERSIONINFO))
   {
#if 1
      if (NT_SUCCESS(RtlGetVersion((PRTL_OSVERSIONINFOW)&Version)))
      {
         VpOsVersionInfo->MajorVersion = Version.dwMajorVersion;
         VpOsVersionInfo->MinorVersion = Version.dwMinorVersion;
         VpOsVersionInfo->BuildNumber = Version.dwBuildNumber;
         VpOsVersionInfo->ServicePackMajor = Version.wServicePackMajor;
         VpOsVersionInfo->ServicePackMinor = Version.wServicePackMinor;
         return NO_ERROR;
      }
      return ERROR_INVALID_PARAMETER;
#else
      VpOsVersionInfo->MajorVersion = 5;
      VpOsVersionInfo->MinorVersion = 0;
      VpOsVersionInfo->BuildNumber = 2195;
      VpOsVersionInfo->ServicePackMajor = 4;
      VpOsVersionInfo->ServicePackMinor = 0;
      return NO_ERROR;
#endif
   }

   return ERROR_INVALID_PARAMETER;
}

/*
 * @unimplemented
 */

BOOLEAN STDCALL
VideoPortCheckForDeviceExistence(
   IN PVOID HwDeviceExtension,
   IN USHORT VendorId,
   IN USHORT DeviceId,
   IN UCHAR RevisionId,
   IN USHORT SubVendorId,
   IN USHORT SubSystemId,
   IN ULONG Flags)
{
   DPRINT1("VideoPortCheckForDeviceExistence: Unimplemented.\n");
   return TRUE;
}

/*
 * @unimplemented
 */

VP_STATUS STDCALL
VideoPortRegisterBugcheckCallback(
   IN PVOID HwDeviceExtension,
   IN ULONG BugcheckCode,
   IN PVOID Callback,
   IN ULONG BugcheckDataSize)
{
   DPRINT1("VideoPortRegisterBugcheckCallback(): Unimplemented.\n");
   return NO_ERROR;
}

/*
 * @implemented
 */

LONGLONG STDCALL
VideoPortQueryPerformanceCounter(
   IN PVOID HwDeviceExtension,
   OUT PLONGLONG PerformanceFrequency OPTIONAL)
{
   LARGE_INTEGER Result;

   DPRINT("VideoPortQueryPerformanceCounter\n");
   Result = KeQueryPerformanceCounter((PLARGE_INTEGER)PerformanceFrequency);
   return Result.QuadPart;
}
