/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/config.c
 * PURPOSE:     NDIS Configuration Services
 * PROGRAMMERS: Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *     Vizzini 08-20-2003 Created
 * NOTES:
 *     - Currently this only supports enumeration of Root and PCI devices
 *     - This whole thing is really just a band-aid until we have real PnP
 *     - Strictly speaking, I'm not even sure it's my job to call 
 *       HalAssignSlotResources(), but the vmware nic driver likes it
 *     - Please send me feedback if there is a better way :)
 * TODO:
 *     - Break these functions up a bit; i hate 200-line functions
 */

#include <ndissys.h>
#include <miniport.h>

/* Registry path to the enumeration database */
#define ENUM_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum"

/* Registry path to the services database */
#define SERVICES_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services"

/*
 * This has to be big enough to hold enumerated registry paths until
 * registry accesses are properly re-coded
 */
#define KEY_INFORMATION_SIZE 512

/* same sort of deal as above */
#define VALUE_INFORMATION_SIZE 100

/*
 * NET class GUID, as defined by Microsoft, used to tell if a
 * device belongs to NDIS or not.
 */
#define NET_GUID L"{4D36E972-E325-11CE-BFC1-08002BE10318}"

#define RZ() do { DbgPrint("%s:%i Checking RedZone\n", __FILE__, __LINE__ ); ExAllocatePool(PagedPool,0); } while (0);

/* see miniport.c */
extern LIST_ENTRY OrphanAdapterListHead;
extern KSPIN_LOCK OrphanAdapterListLock;


BOOLEAN NdisFindDevicePci(UINT VendorID, UINT DeviceID, PUINT BusNumber, PUINT SlotNumber)
/*
 * FUNCTION: Find a PCI device given its Vendor and Device IDs
 * ARGUMENTS:
 *     VendorID: The card's PCI Vendor ID
 *     DeviceID: The card's PCI Device ID
 *     BusNumber: The card's bus number on success
 *     SlotNumber: The card's slot number on success
 * RETURNS:
 *     TRUE if the card is fouund
 *     FALSE Otherwise
 * NOTES:
 *     - This only finds the first card of a type
 *     - This doesn't handle function enumeration correctly
 *     - Based loosely on Dekker & Newcomer examples
 */
{
  PCI_COMMON_CONFIG PciData;
  int i, j, k;

  ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

  /* dekker says there are 256 possible PCI buses */
  for(i = 0; i < 256; i++)
    {
      for(j = 0; j < PCI_MAX_DEVICES; j++)
        {
          for(k = 0; k < PCI_MAX_FUNCTION; k++)
            {
              /* TODO: figure out what to do with k */

              if(HalGetBusData(PCIConfiguration, i, j, &PciData, sizeof(PciData)))
                {
                  if(PciData.VendorID == 0xffff) /* Is this handled right? */
                    continue;

                  if(PciData.VendorID == VendorID && PciData.DeviceID == DeviceID)
                    {
                      if(BusNumber) 
                        *BusNumber = i;
                      if(SlotNumber) 
                        *SlotNumber = j;

                      return TRUE;
                    }
                }
            }
        }
    }

  NDIS_DbgPrint(MAX_TRACE, ("Didn't find device 0x%x:0x%x\n", VendorID, DeviceID));

  return FALSE;
}


VOID NdisStartDriver(PUNICODE_STRING uBusName, PUNICODE_STRING uDeviceId, PUNICODE_STRING uServiceName)
/*
 * FUNCTION: Starts an NDIS driver
 * ARGUMENTS:
 *     uBusName: the card's bus type, by name, as extracted from the 
 *               enumeration database in the registry
 *     uDeviceId: the card's device ID
 *     uServiceName: the driver (scm db entry) associated with the card
 * NOTES:
 *     - This doesn't prooperly handle multiple instances of the same card or driver
 *     - for PCI cards, this finds the card and creates an "orphan" adapter object for
 *       it.  This object is tagged with the registry key to the driver and includes
 *       the slot number and bus number of the card.  NOTE that some stupid nic drivers
 *       still depend on a registry key for slot number, so this isn't always enough.
 *       Whatever the case, all of the card's resources are enumerated and assigned
 *       via HalAssignSlotResources() and the orphan adapter is put onto the list. 
 *       When the miniport calls NdisMRegisterMiniport(), ndis loops through the list
 *       of orphans and associates any orphans that belong to that miniport with
 *       its corresponding LOGICAL_ADAPTER objects.  
 */
{
  ULONG            VendorId;
  ULONG            DeviceId;
  UINT             SlotNumber;
  UINT             BusNumber;
  UNICODE_STRING   Temp;
  UNICODE_STRING   ServiceKey;
  PWCHAR           ServiceKeyStr;
  NTSTATUS         NtStatus;
  PORPHAN_ADAPTER  OrphanAdapter;

  NDIS_DbgPrint(MAX_TRACE, ("Called; Starting %wZ\n", uServiceName));

  ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

  /* prepare a services key */
  ServiceKeyStr = ExAllocatePool(PagedPool, sizeof(SERVICES_KEY) + sizeof(WCHAR) + uServiceName->Length);
  if(!ServiceKeyStr)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient Resources.\n"));
      return;
    }

  wcscpy(ServiceKeyStr, SERVICES_KEY);
  wcscat(ServiceKeyStr, L"\\");
  wcsncat(ServiceKeyStr, uServiceName->Buffer, uServiceName->Length/sizeof(WCHAR));
  ServiceKeyStr[wcslen(SERVICES_KEY)+1+uServiceName->Length/sizeof(WCHAR)] = 0;

  RtlInitUnicodeString(&ServiceKey, ServiceKeyStr);

  if(!wcsncmp(uBusName->Buffer, L"PCI", uBusName->Length))
  {
    /*
     * first see if a card with the requested id exists. 
     * PCI IDs are formatted VEN_ABCD&DEV_ABCD[&...] 
     */

    if(wcsncmp(uDeviceId->Buffer, L"VEN_", 4))
      {
        NDIS_DbgPrint(MIN_TRACE, ("Bogus uDeviceId parsing VEN\n"));
        ExFreePool(ServiceKeyStr);
        return;
      }

    Temp.Buffer = &(uDeviceId->Buffer[4]);                      /* offset of vendor id */
    Temp.Length = Temp.MaximumLength = 4 * sizeof(WCHAR);       /* 4-digit id */

    NtStatus = RtlUnicodeStringToInteger(&Temp, 16, &VendorId);
    if(!NT_SUCCESS(NtStatus))
      {
        NDIS_DbgPrint(MIN_TRACE, ("RtlUnicodeStringToInteger returned 0x%x\n", NtStatus));
        ExFreePool(ServiceKeyStr);
        return;
      }

    if(wcsncmp(&(uDeviceId->Buffer[9]), L"DEV_", 4))    
      {
        NDIS_DbgPrint(MIN_TRACE, ("Bogus uDeviceId parsing DEV\n"));
        ExFreePool(ServiceKeyStr);
        return;
      }

    Temp.Buffer = &(uDeviceId->Buffer[13]);     /* offset of device id */
    Temp.Length = 4 * sizeof(WCHAR);            /* 4-dight id */

    NtStatus = RtlUnicodeStringToInteger(&Temp, 16, &DeviceId);
    if(!NT_SUCCESS(NtStatus))
      {
        NDIS_DbgPrint(MIN_TRACE, ("RtlUnicodeStringToInteger returned 0x%x\n", NtStatus));
        ExFreePool(ServiceKeyStr);
        return;
      }

    if(!NdisFindDevicePci(VendorId, DeviceId, &BusNumber, &SlotNumber))
      {
        NDIS_DbgPrint(MIN_TRACE, ("Didn't find a configured card 0x%x 0x%x\n", VendorId, DeviceId));
        ExFreePool(ServiceKeyStr);
        return;
      }

    NDIS_DbgPrint(MAX_TRACE, ("Found a card 0x%x 0x%x at bus 0x%x slot 0x%x\n", VendorId, DeviceId, BusNumber, SlotNumber));

    OrphanAdapter = ExAllocatePool(PagedPool, sizeof(ORPHAN_ADAPTER));
    if(!OrphanAdapter)
      {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient Resources.\n"));
        ExFreePool(ServiceKeyStr);
        return;
      }

    OrphanAdapter->RegistryPath.Buffer = ExAllocatePool(PagedPool, Temp.MaximumLength);
    if(!OrphanAdapter->RegistryPath.Buffer)
      {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient Resources.\n"));
        ExFreePool(ServiceKeyStr);
        return;
      }

    OrphanAdapter->RegistryPath.Length = Temp.Length;
    OrphanAdapter->RegistryPath.MaximumLength = Temp.MaximumLength;
    memcpy(OrphanAdapter->RegistryPath.Buffer, Temp.Buffer, Temp.Length);

    OrphanAdapter->BusType    = PCIBus; 
    OrphanAdapter->BusNumber  = BusNumber;
    OrphanAdapter->SlotNumber = SlotNumber;

    ExInterlockedInsertTailList(&OrphanAdapterListHead, &OrphanAdapter->ListEntry, &OrphanAdapterListLock);
  }

  /*
   * found a card; start its driver. this should be done from a 
   * system thread, after NDIS.SYS's DriverEntry returns, but I 
   * really don't know how to block on DriverEntry returning, so 
   * what the hell.
   */

  NDIS_DbgPrint(MID_TRACE, ("Loading driver %wZ\n", &ServiceKey));
  ZwLoadDriver(&ServiceKey);

  ExFreePool(ServiceKeyStr);
}


VOID NdisStartDevices()
/*
 * FUNCTION: Find and start all NDIS Net class devices
 * NOTES:
 *     - Not sure if this handles multiple instances of the same
 *       device or driver correctly yet
 */
{
  HANDLE EnumHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  NTSTATUS NtStatus;
  ULONG EnumIndex = 0;
  ULONG ResultLength;
  PKEY_BASIC_INFORMATION KeyInformation;
  ULONG KeyInformationLength;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

  RtlInitUnicodeString(&KeyName, ENUM_KEY);
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, 0, 0);
  NtStatus = ZwOpenKey(&EnumHandle, KEY_ALL_ACCESS, &ObjectAttributes);
  if(!NT_SUCCESS(NtStatus))
    {
      NDIS_DbgPrint(MIN_TRACE, ("Unable to open the Enum key\n"));
      return;
    }

  NDIS_DbgPrint(MAX_TRACE, ("Opened the enum key\n"));

  KeyInformation = ExAllocatePool(PagedPool, KEY_INFORMATION_SIZE);    // should be enough for most key names?
  if(!KeyInformation)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      return;
    }

  KeyInformationLength = KEY_INFORMATION_SIZE;

  while(NT_SUCCESS(ZwEnumerateKey(EnumHandle, EnumIndex, 
        KeyBasicInformation, KeyInformation, KeyInformationLength, &ResultLength)))
    {
      /* iterate through each enumerator (PCI, Root, ISAPNP, etc) */

      HANDLE EnumeratorHandle;
      WCHAR *EnumeratorStr;
      UINT EnumeratorIndex = 0;

      NDIS_DbgPrint(MAX_TRACE, ("Enum iteration 0x%x\n", EnumIndex));

      EnumIndex++;

      EnumeratorStr = ExAllocatePool(PagedPool, sizeof(WCHAR) + KeyInformation->NameLength);
      if(!EnumeratorStr)
        {
          NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
          return;
        }

      wcsncpy(EnumeratorStr, KeyInformation->Name, KeyInformation->NameLength/sizeof(WCHAR));
      EnumeratorStr[KeyInformation->NameLength/sizeof(WCHAR)] = 0;

      RtlInitUnicodeString(&KeyName, EnumeratorStr);
      InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, EnumHandle, 0);
      if(!NT_SUCCESS(ZwOpenKey(&EnumeratorHandle, KEY_ALL_ACCESS, &ObjectAttributes)))
        {
          NDIS_DbgPrint(MIN_TRACE, ("Failed to open key %wZ\n", &KeyName));
          return;
        }
        
      while(NT_SUCCESS(ZwEnumerateKey(EnumeratorHandle, EnumeratorIndex, KeyBasicInformation,
          KeyInformation, KeyInformationLength, &ResultLength)))
        {
          /* iterate through each device id */

          HANDLE DeviceHandle;
          WCHAR *DeviceStr;
          UINT DeviceIndex = 0;
          UNICODE_STRING BusName;

          BusName.Buffer = KeyName.Buffer;
          BusName.Length = KeyName.Length;
          BusName.MaximumLength = KeyName.MaximumLength;

          EnumeratorIndex++;

          DeviceStr = ExAllocatePool(PagedPool, KeyInformation->NameLength + sizeof(WCHAR));
          if(!DeviceStr)
            {
              NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
              return;
            }

          wcsncpy(DeviceStr, KeyInformation->Name, KeyInformation->NameLength/sizeof(WCHAR));
          DeviceStr[KeyInformation->NameLength/sizeof(WCHAR)] = 0;

          RtlInitUnicodeString(&KeyName, DeviceStr);
          InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, EnumeratorHandle, 0);
          if(!NT_SUCCESS(ZwOpenKey(&DeviceHandle, KEY_ALL_ACCESS, &ObjectAttributes)))
            {
              NDIS_DbgPrint(MIN_TRACE, ("Failed to open key %wZ\n", &KeyName));
              return;
            }

          while(NT_SUCCESS(ZwEnumerateKey(DeviceHandle, DeviceIndex, KeyBasicInformation, 
                KeyInformation, KeyInformationLength, &ResultLength)))
            {
              /* iterate through each instance id, starting drivers in the process */
              HANDLE InstanceHandle;
              WCHAR *InstanceStr;
              UNICODE_STRING ValueName;
              UNICODE_STRING ServiceName;
              PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
              ULONG KeyValueInformationLength;
              UNICODE_STRING DeviceId;

              DeviceId.Buffer = KeyName.Buffer;
              DeviceId.Length = KeyName.Length;
              DeviceId.MaximumLength = KeyName.MaximumLength;

              DeviceIndex++;

              InstanceStr = ExAllocatePool(PagedPool, KeyInformation->NameLength + sizeof(WCHAR));
              if(!InstanceStr)
                {
                  NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
                  return;
                }

              wcsncpy(InstanceStr, KeyInformation->Name, KeyInformation->NameLength/sizeof(WCHAR));
              InstanceStr[KeyInformation->NameLength/sizeof(WCHAR)] = 0;

              NDIS_DbgPrint(MAX_TRACE, ("NameLength = 0x%x and InstanceStr: %ws\n", KeyInformation->NameLength, InstanceStr));

              RtlInitUnicodeString(&KeyName, InstanceStr);
              InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, DeviceHandle, 0);
              if(!NT_SUCCESS(ZwOpenKey(&InstanceHandle, KEY_ALL_ACCESS, &ObjectAttributes)))
                {
                  NDIS_DbgPrint(MIN_TRACE, ("Failed to open key %wZ\n", &KeyName));
                  return;
                }

              /* read class, looking for net guid */
              RtlInitUnicodeString(&ValueName, L"ClassGUID");

              KeyValueInformation = ExAllocatePool(PagedPool, VALUE_INFORMATION_SIZE);
              if(!KeyValueInformation)
                {
                  NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
                  return;
                }

              KeyValueInformationLength = VALUE_INFORMATION_SIZE;

              NtStatus = ZwQueryValueKey(InstanceHandle, &ValueName, KeyValuePartialInformation, 
                  KeyValueInformation, KeyValueInformationLength, &ResultLength);
              if(!NT_SUCCESS(NtStatus))
                {
                  /* this isn't fatal, it just means that this device isn't ours */
                  NDIS_DbgPrint(MID_TRACE, ("Failed to query value %wZ from key %wZ\n", &ValueName, &KeyName));
                  continue;
                }

              if(!wcsncmp(NET_GUID, (PWCHAR)KeyValueInformation->Data, KeyValueInformation->DataLength/sizeof(WCHAR)))
                {
                  RtlInitUnicodeString(&ValueName, L"Service");

                  NtStatus = ZwQueryValueKey(InstanceHandle, &ValueName, KeyValuePartialInformation, 
                      KeyValueInformation, KeyValueInformationLength, &ResultLength);
                  if(!NT_SUCCESS(NtStatus))
                    {
                      /* non-fatal also */
                      NDIS_DbgPrint(MID_TRACE, ("Failed to query value %wZ from key %wZ\n", &ValueName, &KeyName));
                      continue;
                    }

                  /* this is a net driver; start it */
                  ServiceName.Length = ServiceName.MaximumLength = KeyValueInformation->DataLength;
                  ServiceName.Buffer = (PWCHAR)KeyValueInformation->Data;
                  NdisStartDriver(&BusName, &DeviceId, &ServiceName);
                }
              else
                NDIS_DbgPrint(MAX_TRACE, ("...this device is not ours\n"));

              ExFreePool(KeyValueInformation);
              ExFreePool(InstanceStr);
              ZwClose(InstanceHandle);
            }

          ExFreePool(DeviceStr);
          ZwClose(DeviceHandle);
        }

      ExFreePool(EnumeratorStr);
      ZwClose(EnumeratorHandle);
    }

  ZwClose(EnumHandle);
  ExFreePool(KeyInformation);
}

