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
/* $Id: resource.c,v 1.15 2004/05/30 18:30:03 navaraf Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/resource.c
 * PURPOSE:         Hardware resource managment
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 *                  Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <rosrtl/string.h>
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static CONFIGURATION_INFORMATION
SystemConfigurationInformation = {0, 0, 0, 0, 0, 0, 0, FALSE, FALSE};

/* API Parameters to Pass in IopQueryBusDescription */
typedef struct IO_QUERY {
    PINTERFACE_TYPE  BusType;
    PULONG  BusNumber;
    PCONFIGURATION_TYPE  ControllerType;
    PULONG  ControllerNumber;
    PCONFIGURATION_TYPE  PeripheralType;
    PULONG  PeripheralNumber;
    PIO_QUERY_DEVICE_ROUTINE  CalloutRoutine;
    PVOID  Context;
} IO_QUERY, *PIO_QUERY;

PWSTR ArcTypes[42] = {
    L"System",
    L"CentralProcessor",
    L"FloatingPointProcessor",
    L"PrimaryICache",
    L"PrimaryDCache",
    L"SecondaryICache",
    L"SecondaryDCache",
    L"SecondaryCache",
    L"EisaAdapter",
    L"TcAdapter",
    L"ScsiAdapter",
    L"DtiAdapter",
    L"MultifunctionAdapter",
    L"DiskController",
    L"TapeController",
    L"CdRomController",
    L"WormController",
    L"SerialController",
    L"NetworkController",
    L"DisplayController",
    L"ParallelController",
    L"PointerController",
    L"KeyboardController",
    L"AudioController",
    L"OtherController",
    L"DiskPeripheral",
    L"FloppyDiskPeripheral",
    L"TapePeripheral",
    L"ModemPeripheral",
    L"MonitorPeripheral",
    L"PrinterPeripheral",
    L"PointerPeripheral",
    L"KeyboardPeripheral",
    L"TerminalPeripheral",
    L"OtherPeripheral",
    L"LinePeripheral",
    L"NetworkPeripheral",
    L"SystemMemory",
    L"DockingInformation",
    L"RealModeIrqRoutingTable",
    L"RealModePCIEnumeration",
    L"Undefined"
};

#define TAG_IO_RESOURCE    TAG('R', 'S', 'R', 'C')

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * IopQueryDeviceDescription
 *
 * FUNCTION:
 *     Reads and returns Hardware information from the appropriate hardware
 *     registry key. Helper sub of IopQueryBusDescription. 
 *
 * ARGUMENTS:
 *     Query          - What the parent function wants.
 *     RootKey        - Which key to look in
 *     RootKeyHandle  - Handle to the key
 *     Bus            - Bus Number.
 *     BusInformation - The Configuration Information Sent
 *
 * RETURNS:
 *      Status
 */

NTSTATUS STDCALL
IopQueryDeviceDescription(
   PIO_QUERY Query,
   UNICODE_STRING RootKey,
   HANDLE RootKeyHandle,
   ULONG Bus,
   PKEY_VALUE_FULL_INFORMATION *BusInformation) 
{
   NTSTATUS Status;

   /* Controller Stuff */
   UNICODE_STRING ControllerString;
   UNICODE_STRING ControllerRootRegName = RootKey;
   UNICODE_STRING ControllerRegName;
   HANDLE ControllerKeyHandle;
   PKEY_FULL_INFORMATION ControllerFullInformation;
   PKEY_VALUE_FULL_INFORMATION ControllerInformation[3] = {NULL, NULL, NULL};
   ULONG ControllerNumber;
   ULONG ControllerLoop;
   ULONG MaximumControllerNumber;

   /* Peripheral Stuff */
   UNICODE_STRING PeripheralString;
   HANDLE PeripheralKeyHandle;
   PKEY_FULL_INFORMATION PeripheralFullInformation;
   PKEY_VALUE_FULL_INFORMATION PeripheralInformation[3] = {NULL, NULL, NULL};
   ULONG PeripheralNumber;
   ULONG PeripheralLoop;
   ULONG MaximumPeripheralNumber;

   /* Global Registry Stuff */
   OBJECT_ATTRIBUTES ObjectAttributes;
   ULONG LenFullInformation;
   ULONG LenKeyFullInformation;
   UNICODE_STRING TempString;
   WCHAR TempBuffer[14];
   PWSTR Strings[3] = {
      L"Identifier",
      L"Configuration Data",
      L"Component Information"
   };

   /* Temporary String */
   TempString.MaximumLength = sizeof(TempBuffer);
   TempString.Length = 0;
   TempString.Buffer = TempBuffer;

   /* Add Controller Name to String */
   RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
   RtlAppendUnicodeToString(&ControllerRootRegName, ArcTypes[*Query->ControllerType]);

   /* Set the Controller Number if specified */
   if (Query->ControllerNumber && *(Query->ControllerNumber))
   {
      ControllerNumber = *Query->ControllerNumber;
      MaximumControllerNumber = ControllerNumber + 1;
   } else {
      /* Find out how many Controller Numbers there are */
      InitializeObjectAttributes(
         &ObjectAttributes,
         &ControllerRootRegName,
         OBJ_CASE_INSENSITIVE,
         NULL,
         NULL);

      Status = ZwOpenKey(&ControllerKeyHandle, KEY_READ, &ObjectAttributes);
      if (NT_SUCCESS(Status))
      {
         /* How much buffer space */
         ZwQueryKey(ControllerKeyHandle, KeyFullInformation, NULL, 0, &LenFullInformation);

         /* Allocate it */
         ControllerFullInformation = ExAllocatePoolWithTag(PagedPool, LenFullInformation, TAG_IO_RESOURCE);

         /* Get the Information */
         Status = ZwQueryKey(ControllerKeyHandle, KeyFullInformation, ControllerFullInformation, LenFullInformation, &LenFullInformation);
         ZwClose(ControllerKeyHandle);
         ControllerKeyHandle = NULL;
      }

      /* No controller was found, go back to function. */
      if (!NT_SUCCESS(Status))
      {
         if (ControllerFullInformation != NULL)
            ExFreePool(ControllerFullInformation);
         return Status;
      }
		
      /* Find out Controller Numbers */
      ControllerNumber = 0;
      MaximumControllerNumber = ControllerFullInformation->SubKeys;

      /* Free Memory */
      ExFreePool(ControllerFullInformation);
      ControllerFullInformation = NULL;
   }

   /* Save String */
   ControllerRegName = ControllerRootRegName;

   /* Loop through controllers */
   for (; ControllerNumber < MaximumControllerNumber; ControllerNumber++)
   {
      /* Load String */
      ControllerRootRegName = ControllerRegName;

      /* Controller Number to Registry String */
      Status = RtlIntegerToUnicodeString(ControllerNumber, 10, &TempString);

      /* Create String */
      Status |= RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
      Status |= RtlAppendUnicodeStringToString(&ControllerRootRegName, &TempString);
		
      /* Something messed up */
      if (!NT_SUCCESS(Status)) break;

      /* Open the Registry Key */
      InitializeObjectAttributes(
         &ObjectAttributes,
         &ControllerRootRegName,
         OBJ_CASE_INSENSITIVE,
         NULL,
         NULL);

      Status = ZwOpenKey(&ControllerKeyHandle, KEY_READ, &ObjectAttributes);

      /* Read the Configuration Data... */
      if (NT_SUCCESS(Status))
      {
         for (ControllerLoop = 0; ControllerLoop < 3; ControllerLoop++)
         {
            /* Identifier String First */
            RtlInitUnicodeString(&ControllerString, Strings[ControllerLoop]);

            /* How much buffer space */
            ZwQueryValueKey(ControllerKeyHandle, &ControllerString, KeyValueFullInformation, NULL, 0, &LenKeyFullInformation);

            /* Allocate it */
            ControllerInformation[ControllerLoop] = ExAllocatePoolWithTag(PagedPool, LenKeyFullInformation, TAG_IO_RESOURCE);

            /* Get the Information */
            Status = ZwQueryValueKey(ControllerKeyHandle, &ControllerString, KeyValueFullInformation, ControllerInformation[ControllerLoop], LenKeyFullInformation, &LenKeyFullInformation);
         }

         /* Clean Up */
         ZwClose(ControllerKeyHandle);
         ControllerKeyHandle = NULL;
      }

      /* Something messed up */
      if (!NT_SUCCESS(Status))
         goto EndLoop;

      /* We now have Bus *AND* Controller Information.. is it enough? */
      if (!(*Query->PeripheralType))
      {
         Status = Query->CalloutRoutine(
            Query->Context,
            &ControllerRootRegName,
            *Query->BusType,
            Bus,
            BusInformation,
            *Query->ControllerType,
            ControllerNumber,
            ControllerInformation,
            0,
            0,
            NULL);
         goto EndLoop;
      }

      /* Not enough...caller also wants peripheral name */
      Status = RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
      Status |= RtlAppendUnicodeToString(&ControllerRootRegName, ArcTypes[*Query->PeripheralType]);

      /* Something messed up */
      if (!NT_SUCCESS(Status)) goto EndLoop;	

      /* Set the Peripheral Number if specified */
      if (Query->PeripheralNumber && *Query->PeripheralNumber)
      {
         PeripheralNumber = *Query->PeripheralNumber;
         MaximumPeripheralNumber = PeripheralNumber + 1;
      } else {
         /* Find out how many Peripheral Numbers there are */
         InitializeObjectAttributes(
            &ObjectAttributes,
            &ControllerRootRegName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

         Status = ZwOpenKey(&PeripheralKeyHandle, KEY_READ, &ObjectAttributes);

         if (NT_SUCCESS(Status))
         {
            /* How much buffer space */
            ZwQueryKey(PeripheralKeyHandle, KeyFullInformation, NULL, 0, &LenFullInformation);

            /* Allocate it */
            PeripheralFullInformation = ExAllocatePoolWithTag(PagedPool, LenFullInformation, TAG_IO_RESOURCE);

            /* Get the Information */
            Status = ZwQueryKey(PeripheralKeyHandle, KeyFullInformation, PeripheralFullInformation, LenFullInformation, &LenFullInformation);
            ZwClose(PeripheralKeyHandle);
            PeripheralKeyHandle = NULL;
         }

         /* No controller was found, go back to function but clean up first */
         if (!NT_SUCCESS(Status))
         {
            Status = STATUS_SUCCESS;
            goto EndLoop;
         }

         /* Find out Peripheral Number */
         PeripheralNumber = 0;
         MaximumPeripheralNumber = PeripheralFullInformation->SubKeys;

         /* Free Memory */
         ExFreePool(PeripheralFullInformation);
         PeripheralFullInformation = NULL;
      }

      /* Save Name */
      ControllerRegName = ControllerRootRegName;

      /* Loop through Peripherals */
      for (; PeripheralNumber < MaximumPeripheralNumber; PeripheralNumber++)
      {
         /* Restore Name */
         ControllerRootRegName = ControllerRegName;

         /* Peripheral Number to Registry String */
         Status = RtlIntegerToUnicodeString(PeripheralNumber, 10, &TempString);

         /* Create String */
         Status |= RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
	 Status |= RtlAppendUnicodeStringToString(&ControllerRootRegName, &TempString);
		
         /* Something messed up */
         if (!NT_SUCCESS(Status)) break;

         /* Open the Registry Key */
         InitializeObjectAttributes(
            &ObjectAttributes,
            &ControllerRootRegName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

         Status = ZwOpenKey(&PeripheralKeyHandle, KEY_READ, &ObjectAttributes);

         if (NT_SUCCESS(Status))
         {
            for (PeripheralLoop = 0; PeripheralLoop < 3; PeripheralLoop++)
            {
               /* Identifier String First */
               RtlInitUnicodeString(&PeripheralString, Strings[PeripheralLoop]);

               /* How much buffer space */
               ZwQueryValueKey(PeripheralKeyHandle, &PeripheralString, KeyValueFullInformation, NULL, 0, &LenKeyFullInformation);

               /* Allocate it */
               PeripheralInformation[PeripheralLoop] = ExAllocatePoolWithTag(PagedPool, LenKeyFullInformation, TAG_IO_RESOURCE);

               /* Get the Information */
               Status = ZwQueryValueKey(PeripheralKeyHandle, &PeripheralString, KeyValueFullInformation, PeripheralInformation[PeripheralLoop], LenKeyFullInformation, &LenKeyFullInformation);
            }

            /* Clean Up */
            ZwClose(PeripheralKeyHandle);
            PeripheralKeyHandle = NULL;

            /* We now have everything the caller could possibly want */
            if (NT_SUCCESS(Status))
            {
#if 0
               Status = Query->CalloutRoutine(
                  Query->Context,
                  &ControllerRootRegName,
                  *Query->BusType,
                  Bus,
                  BusInformation,
                  *Query->ControllerType,
                  ControllerNumber,
                  ControllerInformation,
                  *Query->PeripheralType,
                  PeripheralNumber,
                  PeripheralInformation);
#else
               Status = STATUS_SUCCESS;
#endif
            }

            /* Free the allocated memory */
            for (PeripheralLoop = 0; PeripheralLoop < 3; PeripheralLoop++)
            {
               if (PeripheralInformation[PeripheralLoop])
               {
                  ExFreePool(PeripheralInformation[PeripheralLoop]);
                  PeripheralInformation[PeripheralLoop] = NULL;
               }
            }

            /* Something Messed up */
            if (!NT_SUCCESS(Status)) break;
         }
      }

EndLoop:
      /* Free the allocated memory */
      for (ControllerLoop = 0; ControllerLoop < 3; ControllerLoop++)
      {
         if (ControllerInformation[ControllerLoop])
         {
            ExFreePool(ControllerInformation[ControllerLoop]);
            ControllerInformation[ControllerLoop] = NULL;
         }
      }

      /* Something Messed up */
      if (!NT_SUCCESS(Status)) break;
   }

   return Status;
}

/*
 * IopQueryBusDescription
 *
 * FUNCTION:
 *      Reads and returns Hardware information from the appropriate hardware
 *      registry key. Helper sub of IoQueryDeviceDescription. Has two modes
 *      of operation, either looking for Root Bus Types or for sub-Bus
 *      information.
 *
 * ARGUMENTS:
 *      Query         - What the parent function wants.
 *      RootKey	      - Which key to look in
 *      RootKeyHandle - Handle to the key
 *      Bus           - Bus Number.
 *      KeyIsRoot     - Whether we are looking for Root Bus Types or
 *                      information under them.
 *
 * RETURNS:
 *      Status
 */

NTSTATUS STDCALL
IopQueryBusDescription(
   PIO_QUERY Query,
   UNICODE_STRING RootKey,
   HANDLE RootKeyHandle,
   PULONG Bus,
   BOOLEAN KeyIsRoot) 
{
   NTSTATUS Status;
   ULONG BusLoop;
   UNICODE_STRING SubRootRegName;
   UNICODE_STRING BusString;
   UNICODE_STRING SubBusString;
   ULONG LenBasicInformation;
   ULONG LenFullInformation;
   ULONG LenKeyFullInformation;
   ULONG LenKey;
   HANDLE SubRootKeyHandle;
   PKEY_FULL_INFORMATION FullInformation;
   PKEY_BASIC_INFORMATION BasicInformation;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PKEY_VALUE_FULL_INFORMATION BusInformation[3] = {NULL, NULL, NULL};

   /* How much buffer space */
   Status = ZwQueryKey(RootKeyHandle, KeyFullInformation, NULL, 0, &LenFullInformation);

   if (!NT_SUCCESS(Status))
      return Status;

   /* Allocate it */
   FullInformation = ExAllocatePoolWithTag(PagedPool, LenFullInformation, TAG_IO_RESOURCE);

   /* Get the Information */
   Status = ZwQueryKey(RootKeyHandle, KeyFullInformation, FullInformation, LenFullInformation, &LenFullInformation);

   /* Everything was fine */
   if (NT_SUCCESS(Status))
   {
      /* Buffer needed for all the keys under this one */
      LenBasicInformation = FullInformation->MaxNameLen + sizeof(KEY_BASIC_INFORMATION);

      /* Allocate it */
      BasicInformation = ExAllocatePoolWithTag(PagedPool, LenBasicInformation, TAG_IO_RESOURCE);
   }

   /* Deallocate the old Buffer */
   ExFreePool(FullInformation);

   /* Try to find a Bus */
   for (BusLoop = 0; NT_SUCCESS(Status); BusLoop++)
   {
      /* Bus parameter was passed and number was matched */
      if ((Query->BusNumber) && (*(Query->BusNumber)) == *Bus) break;

      /* Enumerate the Key */
      Status = ZwEnumerateKey(
         RootKeyHandle,
         BusLoop,
         KeyBasicInformation,
         BasicInformation,
         LenBasicInformation,
         &LenKey);

      /* Everything enumerated */
      if (!NT_SUCCESS(Status)) break;

      /* What Bus are we going to go down? (only check if this is a Root Key) */
      if (KeyIsRoot)
      {
         if (wcsncmp(BasicInformation->Name, L"MultifunctionAdapter", BasicInformation->NameLength / 2) &&
             wcsncmp(BasicInformation->Name, L"EisaAdapter", BasicInformation->NameLength / 2) &&
             wcsncmp(BasicInformation->Name, L"TcAdapter", BasicInformation->NameLength / 2))
         {
            /* Nothing found, check next */
            continue;
         }
      }

      /* Enumerate the Bus. */
      BusString.Buffer = BasicInformation->Name;
      BusString.Length = BasicInformation->NameLength;
      BusString.MaximumLength = BasicInformation->NameLength;

      /* Open a handle to the Root Registry Key */
      InitializeObjectAttributes(
         &ObjectAttributes,
         &BusString,
         OBJ_CASE_INSENSITIVE,
         RootKeyHandle,
         NULL);

      Status = ZwOpenKey(&SubRootKeyHandle, KEY_READ, &ObjectAttributes);

      /* Go on if we can't */
      if (!NT_SUCCESS(Status)) continue;

      /* Key opened. Create the path */
      SubRootRegName = RootKey;
      RtlAppendUnicodeToString(&SubRootRegName, L"\\");
      RtlAppendUnicodeStringToString(&SubRootRegName, &BusString);

      if (!KeyIsRoot)
      {
         /* Parsing a SubBus-key */
         int SubBusLoop;
         PWSTR Strings[3] = {
            L"Identifier",
            L"Configuration Data",
            L"Component Information"};

         for (SubBusLoop = 0; SubBusLoop < 3; SubBusLoop++)
         {
            /* Identifier String First */
            RtlInitUnicodeString(&SubBusString, Strings[SubBusLoop]);

            /* How much buffer space */
            ZwQueryValueKey(SubRootKeyHandle, &SubBusString, KeyValueFullInformation, NULL, 0, &LenKeyFullInformation);

            /* Allocate it */
            BusInformation[SubBusLoop] = ExAllocatePoolWithTag(PagedPool, LenKeyFullInformation, TAG_IO_RESOURCE);

            /* Get the Information */
            Status = ZwQueryValueKey(SubRootKeyHandle, &SubBusString, KeyValueFullInformation, BusInformation[SubBusLoop], LenKeyFullInformation, &LenKeyFullInformation);
         }

         if (NT_SUCCESS(Status))
         {
            /* Do we have something */
            if (BusInformation[1] != NULL &&
                BusInformation[1]->DataLength != 0 &&
                /* Does it match what we want? */
                (((PCM_FULL_RESOURCE_DESCRIPTOR)((ULONG_PTR)BusInformation[1] + BusInformation[1]->DataOffset))->InterfaceType == *(Query->BusType)))
            {
               /* Found a bus */
               (*Bus)++;

               /* Is it the bus we wanted */
               if (Query->BusNumber == NULL || *(Query->BusNumber) == *Bus)
               {
                  /* If we don't want Controller Information, we're done... call the callback */
                  if (Query->ControllerType == NULL)
                  {
                     Status = Query->CalloutRoutine(
                        Query->Context,
                        &SubRootRegName,
                        *(Query->BusType),
                        *Bus,
                        BusInformation,
                        0,
                        0,
                        NULL,
                        0,
                        0,
                        NULL);
                  } else {
                     /* We want Controller Info...get it */
                     Status = IopQueryDeviceDescription(Query, SubRootRegName, RootKeyHandle, *Bus, (PKEY_VALUE_FULL_INFORMATION*)BusInformation);
                  }
               }
            }
         }

         /* Free the allocated memory */
         for (SubBusLoop = 0; SubBusLoop < 3; SubBusLoop++)
         {
            if (BusInformation[SubBusLoop])
            {
               ExFreePool(BusInformation[SubBusLoop]);
               BusInformation[SubBusLoop] = NULL;
            }
         }

         /* Exit the Loop if we found the bus */
         if (Query->BusNumber != NULL && *(Query->BusNumber) == *Bus)
         {
            ZwClose(SubRootKeyHandle);
            SubRootKeyHandle = NULL;
            continue;
         }
      }

      /* Enumerate the buses below us recursively if we haven't found the bus yet */
      Status = IopQueryBusDescription(Query, SubRootRegName, SubRootKeyHandle, Bus, !KeyIsRoot);

      /* Everything enumerated */
      if (Status == STATUS_NO_MORE_ENTRIES) Status = STATUS_SUCCESS;

      ZwClose(SubRootKeyHandle);
      SubRootKeyHandle = NULL;
   }
	
   /* Free the last remaining Allocated Memory */
   if (BasicInformation)
      ExFreePool(BasicInformation);

   return Status;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PCONFIGURATION_INFORMATION STDCALL
IoGetConfigurationInformation(VOID)
{
  return(&SystemConfigurationInformation);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
IoReportResourceUsage(PUNICODE_STRING DriverClassName,
		      PDRIVER_OBJECT DriverObject,
		      PCM_RESOURCE_LIST DriverList,
		      ULONG DriverListSize,
		      PDEVICE_OBJECT DeviceObject,
		      PCM_RESOURCE_LIST DeviceList,
		      ULONG DeviceListSize,
		      BOOLEAN OverrideConflict,
		      PBOOLEAN ConflictDetected)
     /*
      * FUNCTION: Reports hardware resources in the 
      * \Registry\Machine\Hardware\ResourceMap tree, so that a subsequently
      * loaded driver cannot attempt to use the same resources.
      * ARGUMENTS:
      *       DriverClassName - The class of driver under which the resource
      *       information should be stored.
      *       DriverObject - The driver object that was input to the 
      *       DriverEntry.
      *       DriverList - Resources that claimed for the driver rather than
      *       per-device.
      *       DriverListSize - Size in bytes of the DriverList.
      *       DeviceObject - The device object for which resources should be
      *       claimed.
      *       DeviceList - List of resources which should be claimed for the
      *       device.
      *       DeviceListSize - Size of the per-device resource list in bytes.
      *       OverrideConflict - True if the resources should be cliamed
      *       even if a conflict is found.
      *       ConflictDetected - Points to a variable that receives TRUE if
      *       a conflict is detected with another driver.
      */
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
IoAssignResources(PUNICODE_STRING RegistryPath,
		  PUNICODE_STRING DriverClassName,
		  PDRIVER_OBJECT DriverObject,
		  PDEVICE_OBJECT DeviceObject,
		  PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources,
		  PCM_RESOURCE_LIST* AllocatedResources)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
}

/*
 * FUNCTION:
 *     Reads and returns Hardware information from the appropriate hardware registry key.
 *
 * ARGUMENTS:
 *     BusType          - MCA, ISA, EISA...specifies the Bus Type
 *     BusNumber	- Which bus of above should be queried
 *     ControllerType	- Specifices the Controller Type
 *     ControllerNumber	- Which of the controllers to query.
 *     CalloutRoutine	- Which function to call for each valid query.
 *     Context          - Value to pass to the callback.
 *
 * RETURNS:
 *     Status
 *
 * STATUS:
 *     @implemented
 */

NTSTATUS NTAPI
IoQueryDeviceDescription(PINTERFACE_TYPE BusType,
			 PULONG BusNumber,
			 PCONFIGURATION_TYPE ControllerType,
			 PULONG ControllerNumber,
			 PCONFIGURATION_TYPE PeripheralType,
			 PULONG PeripheralNumber,
			 PIO_QUERY_DEVICE_ROUTINE CalloutRoutine,
			 PVOID Context)
{
   NTSTATUS Status;
   ULONG BusLoopNumber = -1; /* Root Bus */
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING RootRegKey;
   HANDLE RootRegHandle;
   WCHAR RootRegString[] = L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\SYSTEM\\";
   IO_QUERY Query;

   /* Set up the String */
   RootRegKey.Length = 0;
   RootRegKey.MaximumLength = 2048;
   RootRegKey.Buffer = ExAllocatePoolWithTag(PagedPool, RootRegKey.MaximumLength, TAG_IO_RESOURCE);
   RtlAppendUnicodeToString(&RootRegKey, RootRegString);
	
   /* Open a handle to the Root Registry Key */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &RootRegKey,
      OBJ_CASE_INSENSITIVE,
      NULL,
      NULL);

   Status = ZwOpenKey(&RootRegHandle, KEY_READ, &ObjectAttributes);

   if (NT_SUCCESS(Status))
   {
      /* Use a helper function to loop though this key and get the info */
      Query.BusType = BusType;
      Query.BusNumber = BusNumber;
      Query.ControllerType = ControllerType;
      Query.ControllerNumber = ControllerNumber;
      Query.PeripheralType = PeripheralType;
      Query.PeripheralNumber = PeripheralNumber;
      Query.CalloutRoutine = CalloutRoutine;
      Query.Context = Context;
      Status = IopQueryBusDescription(&Query, RootRegKey, RootRegHandle, &BusLoopNumber, TRUE);

      /* Close registry */
      ZwClose(RootRegHandle);
   }

   /* Free Memory */
   ExFreePool(RootRegKey.Buffer);

   return Status;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
IoReportHalResourceUsage(PUNICODE_STRING HalDescription,
			 PCM_RESOURCE_LIST RawList,
			 PCM_RESOURCE_LIST TranslatedList,
			 ULONG ListSize)
/*
 * FUNCTION:
 *      Reports hardware resources of the HAL in the
 *      \Registry\Machine\Hardware\ResourceMap tree.
 * ARGUMENTS:
 *      HalDescription: Descriptive name of the HAL.
 *      RawList: List of raw (bus specific) resources which should be
 *               claimed for the HAL.
 *      TranslatedList: List of translated (system wide) resources which
 *                      should be claimed for the HAL.
 *      ListSize: Size in bytes of the raw and translated resource lists.
 *                Both lists have the same size.
 * RETURNS:
 *      Status.
 */
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING Name;
  ULONG Disposition;
  NTSTATUS Status;
  HANDLE ResourcemapKey;
  HANDLE HalKey;
  HANDLE DescriptionKey;

  /* Open/Create 'RESOURCEMAP' key. */
  RtlRosInitUnicodeStringFromLiteral(&Name,
		       L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     0,
			     NULL);
  Status = NtCreateKey(&ResourcemapKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       &Disposition);
  if (!NT_SUCCESS(Status))
    return(Status);

  /* Open/Create 'Hardware Abstraction Layer' key */
  RtlRosInitUnicodeStringFromLiteral(&Name,
		       L"Hardware Abstraction Layer");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     ResourcemapKey,
			     NULL);
  Status = NtCreateKey(&HalKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       &Disposition);
  NtClose(ResourcemapKey);
  if (!NT_SUCCESS(Status))
      return(Status);

  /* Create 'HalDescription' key */
  InitializeObjectAttributes(&ObjectAttributes,
			     HalDescription,
			     OBJ_CASE_INSENSITIVE,
			     HalKey,
			     NULL);
  Status = NtCreateKey(&DescriptionKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       &Disposition);
  NtClose(HalKey);
  if (!NT_SUCCESS(Status))
    return(Status);

  /* Add '.Raw' value. */
  RtlRosInitUnicodeStringFromLiteral(&Name,
		       L".Raw");
  Status = NtSetValueKey(DescriptionKey,
			 &Name,
			 0,
			 REG_RESOURCE_LIST,
			 RawList,
			 ListSize);
  if (!NT_SUCCESS(Status))
    {
      NtClose(DescriptionKey);
      return(Status);
    }

  /* Add '.Translated' value. */
  RtlRosInitUnicodeStringFromLiteral(&Name,
		       L".Translated");
  Status = NtSetValueKey(DescriptionKey,
			 &Name,
			 0,
			 REG_RESOURCE_LIST,
			 TranslatedList,
			 ListSize);
  NtClose(DescriptionKey);

  return(Status);
}

/* EOF */
