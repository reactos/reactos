/*
 * Copyright (c) 1999, 2000
 *	Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifdef _MSC_VER
#include "stdarg.h"
#include "ntddk.h"
#include "ntiologc.h"
#include "ndis.h"
#else
#include <ddk/ntddk.h>
#include <net/ndis.h>
#endif

#include "ntddpack.h"

#include "debug.h"
#include "packet.h"
#include "win_bpf.h"
#include "win_bpf_filter_init.h"

#include "tme.h"

#if DBG
// Declare the global debug flag for this driver.
//ULONG PacketDebugFlag = PACKET_DEBUG_LOUD;
ULONG PacketDebugFlag = PACKET_DEBUG_LOUD + PACKET_DEBUG_VERY_LOUD + PACKET_DEBUG_INIT;

#endif

PDEVICE_EXTENSION GlobalDeviceExtension;

////////////////////////////////////////////////////////////////////////////////
#define ROBERTS_PATCH
#ifdef ROBERTS_PATCH

#define NDIS_STRING_CONST(x)	{sizeof(L##x)-2, sizeof(L##x), L##x}

//void __moddi3(void) {}
//void __divdi3(void) {}

#endif // ROBERTS_PATCH
////////////////////////////////////////////////////////////////////////////////
//
// Global strings
//
NDIS_STRING NPF_Prefix = NDIS_STRING_CONST("NPF_");
NDIS_STRING devicePrefix = NDIS_STRING_CONST("\\Device\\");
NDIS_STRING symbolicLinkPrefix = NDIS_STRING_CONST("\\DosDevices\\");
NDIS_STRING tcpLinkageKeyName = NDIS_STRING_CONST("\\Registry\\Machine\\System"
								L"\\CurrentControlSet\\Services\\Tcpip\\Linkage");
NDIS_STRING AdapterListKey = NDIS_STRING_CONST("\\Registry\\Machine\\System"
								L"\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");
NDIS_STRING bindValueName = NDIS_STRING_CONST("Bind");


/// Global variable that points to the names of the bound adapters
WCHAR* bindP = NULL;

extern struct time_conv G_Start_Time; // from openclos.c

extern NDIS_SPIN_LOCK Opened_Instances_Lock;

//
//  Packet Driver's entry routine.
//
NTSTATUS
#ifdef __GNUC__
STDCALL
#endif
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{

    NDIS_PROTOCOL_CHARACTERISTICS  ProtocolChar;
    UNICODE_STRING MacDriverName;
    UNICODE_STRING UnicodeDeviceName;
    PDEVICE_OBJECT DeviceObject = NULL;
    PDEVICE_EXTENSION DeviceExtension = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS ErrorCode = STATUS_SUCCESS;
    NDIS_STRING ProtoName = NDIS_STRING_CONST("PacketDriver");
    ULONG          DevicesCreated=0;
    PWSTR          BindString;
    PWSTR          ExportString;
    PWSTR          BindStringSave;
    PWSTR          ExportStringSave;
    NDIS_HANDLE    NdisProtocolHandle;
	WCHAR* bindT;
	PKEY_VALUE_PARTIAL_INFORMATION tcpBindingsP;
	UNICODE_STRING macName;
	
	// This driver at the moment works only on single processor machines
	if(NdisSystemProcessorCount() != 1){
		return STATUS_IMAGE_MP_UP_MISMATCH;
	}

    IF_LOUD(DbgPrint("Packet: DriverEntry\n");)

	RtlZeroMemory(&ProtocolChar,sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

#ifdef NDIS50
    ProtocolChar.MajorNdisVersion            = 5;
#else
    ProtocolChar.MajorNdisVersion            = 3;
#endif
    ProtocolChar.MinorNdisVersion            = 0;
#ifndef __GNUC__
    ProtocolChar.Reserved                    = 0;
#else
    ProtocolChar.u1.Reserved                 = 0;
#endif
    ProtocolChar.OpenAdapterCompleteHandler  = NPF_OpenAdapterComplete;
    ProtocolChar.CloseAdapterCompleteHandler = NPF_CloseAdapterComplete;
#ifndef __GNUC__
    ProtocolChar.SendCompleteHandler         = NPF_SendComplete;
    ProtocolChar.TransferDataCompleteHandler = NPF_TransferDataComplete;
#else
    ProtocolChar.u2.SendCompleteHandler         = NPF_SendComplete;
    ProtocolChar.u3.TransferDataCompleteHandler = NPF_TransferDataComplete;
#endif
    ProtocolChar.ResetCompleteHandler        = NPF_ResetComplete;
    ProtocolChar.RequestCompleteHandler      = NPF_RequestComplete;
#ifndef __GNUC__
    ProtocolChar.ReceiveHandler              = NPF_tap;
#else
    ProtocolChar.u4.ReceiveHandler           = NPF_tap;
#endif
    ProtocolChar.ReceiveCompleteHandler      = NPF_ReceiveComplete;
    ProtocolChar.StatusHandler               = NPF_Status;
    ProtocolChar.StatusCompleteHandler       = NPF_StatusComplete;
#ifdef NDIS50
    ProtocolChar.BindAdapterHandler          = NPF_BindAdapter;
    ProtocolChar.UnbindAdapterHandler        = NPF_UnbindAdapter;
    ProtocolChar.PnPEventHandler             = NPF_PowerChange;
    ProtocolChar.ReceivePacketHandler        = NULL;
#endif
    ProtocolChar.Name                        = ProtoName;

    NdisRegisterProtocol(
        &Status,
        &NdisProtocolHandle,
        &ProtocolChar,
        sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

    if (Status != NDIS_STATUS_SUCCESS) {

        IF_LOUD(DbgPrint("NPF: Failed to register protocol with NDIS\n");)

        return Status;

    }
	
    NdisAllocateSpinLock(&Opened_Instances_Lock);

    // Set up the device driver entry points.
    DriverObject->MajorFunction[IRP_MJ_CREATE] = NPF_Open;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = NPF_Close;
    DriverObject->MajorFunction[IRP_MJ_READ]   = NPF_Read;
    DriverObject->MajorFunction[IRP_MJ_WRITE]  = NPF_Write;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = NPF_IoControl;
    DriverObject->DriverUnload = NPF_Unload;

/*
    //  Get the name of the Packet driver and the name of the NIC driver
    //  to bind to from the registry
    Status=NPF_ReadRegistry(
               &BindString,
               &ExportString,
               RegistryPath
               );

    if (Status != STATUS_SUCCESS) {

		IF_LOUD(DbgPrint("Trying dynamic binding\n");)	
 */
		bindP = getAdaptersList();

		if (bindP == NULL) {
			IF_LOUD(DbgPrint("Adapters not found in the registry, try to copy the bindings of TCP-IP.\n");)

			tcpBindingsP = getTcpBindings();
			
			if (tcpBindingsP == NULL){
				IF_LOUD(DbgPrint("TCP-IP not found, quitting.\n");)
				goto RegistryError;
			}
			
			bindP = (WCHAR*)tcpBindingsP;
			bindT = (WCHAR*)(tcpBindingsP->Data);
        } else {
			bindT = bindP;
		}

		for (; *bindT != UNICODE_NULL; bindT += (macName.Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR)) {
			RtlInitUnicodeString(&macName, bindT);
			createDevice(DriverObject, &macName, NdisProtocolHandle);
		}

		return STATUS_SUCCESS;
/*
    }
    BindStringSave   = BindString;
    ExportStringSave = ExportString;
    //
    //  create a device object for each entry
    //
    while (*BindString!= UNICODE_NULL && *ExportString!= UNICODE_NULL) {
        //
        //  Create a counted unicode string for both null terminated strings
        //
        RtlInitUnicodeString(
            &MacDriverName,
            BindString);
        RtlInitUnicodeString(
            &UnicodeDeviceName,
            ExportString);
        //
        //  Advance to the next string of the MULTI_SZ string
        //
        BindString   += (MacDriverName.Length+sizeof(UNICODE_NULL))/sizeof(WCHAR);
        ExportString += (UnicodeDeviceName.Length+sizeof(UNICODE_NULL))/sizeof(WCHAR);
        IF_LOUD(DbgPrint("NPF: DeviceName=%ws  MacName=%ws\n",UnicodeDeviceName.Buffer,MacDriverName.Buffer);)
        //
        //  Create the device object
        //
        Status = IoCreateDevice(
                    DriverObject,
                    sizeof(DEVICE_EXTENSION),
                    &UnicodeDeviceName,
                    FILE_DEVICE_PROTOCOL,
                    0,
                    FALSE,
                    &DeviceObject);
        if (Status != STATUS_SUCCESS) {
            IF_LOUD(DbgPrint("NPF: IoCreateDevice() failed:\n");)
            break;
        }
        DevicesCreated++;
        DeviceObject->Flags |= DO_DIRECT_IO;
        DeviceExtension  =  (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
        DeviceExtension->DeviceObject = DeviceObject;
        //
        //  Save the the name of the MAC driver to open in the Device Extension
        //
        DeviceExtension->AdapterName=MacDriverName;
        if (DevicesCreated == 1) {
            DeviceExtension->BindString   = NULL;
            DeviceExtension->ExportString = NULL;
        }
        DeviceExtension->NdisProtocolHandle=NdisProtocolHandle;
    }
    if (DevicesCreated > 0) {
        //
        //  Managed to create at least one device.
        //
        IF_LOUD(DbgPrint("NPF: Managed to create at least one device.\n");)
        return STATUS_SUCCESS;
    }
    ExFreePool(BindStringSave);
    ExFreePool(ExportStringSave);
 */

RegistryError:
    IF_LOUD(DbgPrint("NPF: RegistryError: calling NdisDeregisterProtocol()\n");)
    NdisDeregisterProtocol(
        &Status,
        NdisProtocolHandle
        );

    Status=STATUS_UNSUCCESSFUL;

    return(Status);

}

//-------------------------------------------------------------------

PWCHAR getAdaptersList(void)
{
	PKEY_VALUE_PARTIAL_INFORMATION result = NULL;
	OBJECT_ATTRIBUTES objAttrs;
	NTSTATUS status;
	HANDLE keyHandle;
	UINT BufPos=0;
	
#define NPF_TAG_DEVICENAME  TAG('0', 'P', 'W', 'A')
	PWCHAR DeviceNames = (PWCHAR) ExAllocatePoolWithTag(PagedPool, 4096, NPF_TAG_DEVICENAME);
	
	if (DeviceNames == NULL) {
		IF_LOUD(DbgPrint("Unable the allocate the buffer for the list of the network adapters\n");)
			return NULL;
	}
	
	InitializeObjectAttributes(&objAttrs, &AdapterListKey,
		OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwOpenKey(&keyHandle, KEY_READ, &objAttrs);
	if (!NT_SUCCESS(status)) {

        //IF_LOUD(DbgPrint("Status of %x opening %ws\n", status, tcpLinkageKeyName.Buffer);)
        IF_LOUD(DbgPrint("Status of %x opening %ws\n", status, AdapterListKey.Buffer);)

    } else { //OK
		ULONG resultLength;
		KEY_VALUE_PARTIAL_INFORMATION valueInfo;
		CHAR AdapInfo[1024];
		UINT i=0;
		
		IF_LOUD(DbgPrint("getAdaptersList: scanning the list of the adapters in the registry, DeviceNames=%x\n",DeviceNames);)
			
			// Scan the list of the devices
			while((status=ZwEnumerateKey(keyHandle,i,KeyBasicInformation,AdapInfo,sizeof(AdapInfo),&resultLength))==STATUS_SUCCESS)	{
				WCHAR ExportKeyName [512];
				PWCHAR ExportKeyPrefix = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\";
				UINT ExportKeyPrefixSize = sizeof(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");
				PWCHAR LinkageKeyPrefix = L"\\Linkage";
				UINT LinkageKeyPrefixSize = sizeof(L"\\Linkage");
				NDIS_STRING FinalExportKey = NDIS_STRING_CONST("Export");
				PKEY_BASIC_INFORMATION tInfo= (PKEY_BASIC_INFORMATION)AdapInfo;
				UNICODE_STRING AdapterKeyName;
				HANDLE ExportKeyHandle;
				KEY_VALUE_PARTIAL_INFORMATION valueInfo;
				ULONG resultLength;
				
				RtlCopyMemory(ExportKeyName,
					ExportKeyPrefix,
					ExportKeyPrefixSize);
				
				RtlCopyMemory((PCHAR)ExportKeyName+ExportKeyPrefixSize,
					tInfo->Name,
					tInfo->NameLength+2);
				
				RtlCopyMemory((PCHAR)ExportKeyName+ExportKeyPrefixSize+tInfo->NameLength,
					LinkageKeyPrefix,
					LinkageKeyPrefixSize);
				
				IF_LOUD(DbgPrint("Key name=%ws\n", ExportKeyName);)
										
				RtlInitUnicodeString(&AdapterKeyName, ExportKeyName);
				
				InitializeObjectAttributes(&objAttrs, &AdapterKeyName,
					OBJ_CASE_INSENSITIVE, NULL, NULL);
				
				status=ZwOpenKey(&ExportKeyHandle,KEY_READ,&objAttrs);
				
				if (!NT_SUCCESS(status)) {
					DbgPrint("OpenKey Failed, %d!\n",status);
					i++;
					continue;
				}
				
				status = ZwQueryValueKey(ExportKeyHandle, &FinalExportKey,
					KeyValuePartialInformation, &valueInfo,
					sizeof(valueInfo), &resultLength);
				
				if (!NT_SUCCESS(status) && (status != STATUS_BUFFER_OVERFLOW)) {
					IF_LOUD(DbgPrint("\n\nStatus of %x querying key value for size\n", status);)
                } else { // We know how big it needs to be.
					ULONG valueInfoLength = valueInfo.DataLength + FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]);
#define NPF_TAG_KEYVALUE  TAG('1', 'P', 'W', 'A')
					PKEY_VALUE_PARTIAL_INFORMATION valueInfoP =	(PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePoolWithTag(PagedPool, valueInfoLength, NPF_TAG_KEYVALUE);
					if (valueInfoP != NULL) {
						status = ZwQueryValueKey(ExportKeyHandle, &FinalExportKey,
							KeyValuePartialInformation,
							valueInfoP,
							valueInfoLength, &resultLength);
						if (!NT_SUCCESS(status)) {
							IF_LOUD(DbgPrint("Status of %x querying key value\n", status);)
                        } else {
							IF_LOUD(DbgPrint("Device %d = %ws\n", i, valueInfoP->Data);)
								RtlCopyMemory((PCHAR)DeviceNames+BufPos,
								valueInfoP->Data,
								valueInfoP->DataLength);
							BufPos+=valueInfoP->DataLength-2;
						}
						
						ExFreePool(valueInfoP);
                    } else {
						IF_LOUD(DbgPrint("Error Allocating the buffer for the device name\n");)
					}
					
				}
				
				// terminate the buffer
				DeviceNames[BufPos/2]=0;
				DeviceNames[BufPos/2+1]=0;
				
				ZwClose (ExportKeyHandle);
				i++;
				
			}
			
			ZwClose (keyHandle);
			
	}
	if(BufPos==0){
		ExFreePool(DeviceNames);
		return NULL;
	}
	return DeviceNames;
}

//-------------------------------------------------------------------

PKEY_VALUE_PARTIAL_INFORMATION getTcpBindings(void)
{
  PKEY_VALUE_PARTIAL_INFORMATION result = NULL;
  OBJECT_ATTRIBUTES objAttrs;
  NTSTATUS status;
  HANDLE keyHandle;

  InitializeObjectAttributes(&objAttrs, &tcpLinkageKeyName,
                             OBJ_CASE_INSENSITIVE, NULL, NULL);
  status = ZwOpenKey(&keyHandle, KEY_READ, &objAttrs);
  if (!NT_SUCCESS(status)) {
    IF_LOUD(DbgPrint("\n\nStatus of %x opening %ws\n", status, tcpLinkageKeyName.Buffer);)
  } else {
    ULONG resultLength;
    KEY_VALUE_PARTIAL_INFORMATION valueInfo;

    IF_LOUD(DbgPrint("\n\nOpened %ws\n", tcpLinkageKeyName.Buffer);)

    status = ZwQueryValueKey(keyHandle, &bindValueName,
                             KeyValuePartialInformation, &valueInfo,
                             sizeof(valueInfo), &resultLength);
    if (!NT_SUCCESS(status) && (status != STATUS_BUFFER_OVERFLOW)) {
      IF_LOUD(DbgPrint("\n\nStatus of %x querying key value for size\n", status);)
    } else {                      // We know how big it needs to be.
      ULONG valueInfoLength = valueInfo.DataLength + FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]);
#define NPF_TAG_KEYVALUE2  TAG('2', 'P', 'W', 'A')
      PKEY_VALUE_PARTIAL_INFORMATION valueInfoP =
        (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(PagedPool, valueInfoLength, NPF_TAG_KEYVALUE2);
      
	  if (valueInfoP != NULL) {
        status = ZwQueryValueKey(keyHandle, &bindValueName,
                                 KeyValuePartialInformation,
                                 valueInfoP,
                                 valueInfoLength, &resultLength);
      
		if (!NT_SUCCESS(status)) {
          IF_LOUD(DbgPrint("\n\nStatus of %x querying key value\n", status);)
        }
        else if (valueInfoLength != resultLength) {
          IF_LOUD(DbgPrint("\n\nQuerying key value result len = %u "
                     "but previous len = %u\n",
                     resultLength, valueInfoLength);)
        }
        else if (valueInfoP->Type != REG_MULTI_SZ) {
          IF_LOUD(DbgPrint("\n\nTcpip bind value not REG_MULTI_SZ but %u\n",
                     valueInfoP->Type);)
        }
        else {                  // It's OK
#if DBG
          ULONG i;
          WCHAR* dataP = (WCHAR*)(&valueInfoP->Data[0]);
          IF_LOUD(DbgPrint("\n\nBind value:\n");)
          for (i = 0; *dataP != UNICODE_NULL; i++) {
            UNICODE_STRING macName;
            RtlInitUnicodeString(&macName, dataP);
            IF_LOUD(DbgPrint("\n\nMac %u = %ws\n", i, macName.Buffer);)
            dataP +=
              (macName.Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR);
          }
#endif // DBG
          result = valueInfoP;
        }
      }
    }
    ZwClose(keyHandle);
  }
  return result;
}

//-------------------------------------------------------------------

BOOLEAN createDevice(IN OUT PDRIVER_OBJECT adriverObjectP,
                     IN PUNICODE_STRING amacNameP, NDIS_HANDLE aProtoHandle)
{
    NTSTATUS status;
    PDEVICE_OBJECT devObjP;
    UNICODE_STRING deviceName;
	UNICODE_STRING deviceSymLink;

	IF_LOUD(DbgPrint("\n\ncreateDevice for MAC %ws\n", amacNameP->Buffer););
    if (RtlCompareMemory(amacNameP->Buffer, devicePrefix.Buffer,
                       devicePrefix.Length) < devicePrefix.Length) {
		return FALSE;
    }

    deviceName.Length = 0;
    deviceName.MaximumLength = (USHORT)(amacNameP->Length + NPF_Prefix.Length + sizeof(UNICODE_NULL));
#define NPF_TAG_DEVICENAMEBUF  TAG('3', 'P', 'W', 'A')
    deviceName.Buffer = ExAllocatePoolWithTag(PagedPool, deviceName.MaximumLength, NPF_TAG_DEVICENAMEBUF);
 
	if (deviceName.Buffer == NULL)
		return FALSE;

	deviceSymLink.Length = 0;
	deviceSymLink.MaximumLength =(USHORT)(amacNameP->Length-devicePrefix.Length 
		+ symbolicLinkPrefix.Length 
		+ NPF_Prefix.Length 
		+ sizeof(UNICODE_NULL));

#define NPF_TAG_SYMLINKBUF  TAG('3', 'P', 'W', 'A')
	deviceSymLink.Buffer = ExAllocatePoolWithTag(NonPagedPool, deviceSymLink.MaximumLength, NPF_TAG_SYMLINKBUF);

	if (deviceSymLink.Buffer  == NULL)
	{
		ExFreePool(deviceName.Buffer);
		return FALSE;
	}

    RtlAppendUnicodeStringToString(&deviceName, &devicePrefix);
    RtlAppendUnicodeStringToString(&deviceName, &NPF_Prefix);
    RtlAppendUnicodeToString(&deviceName, amacNameP->Buffer +
                             devicePrefix.Length / sizeof(WCHAR));
    
	RtlAppendUnicodeStringToString(&deviceSymLink, &symbolicLinkPrefix);
	RtlAppendUnicodeStringToString(&deviceSymLink, &NPF_Prefix);
	RtlAppendUnicodeToString(&deviceSymLink, amacNameP->Buffer +
		devicePrefix.Length / sizeof(WCHAR));

	IF_LOUD(DbgPrint("Creating device name: %ws\n", deviceName.Buffer);)
    
	status = IoCreateDevice(adriverObjectP, 
        sizeof(PDEVICE_EXTENSION),
        &deviceName, 
        FILE_DEVICE_TRANSPORT,
        0,
        FALSE,
        &devObjP);

    if (NT_SUCCESS(status)) {
        PDEVICE_EXTENSION devExtP = (PDEVICE_EXTENSION)devObjP->DeviceExtension;

		IF_LOUD(DbgPrint("Device created successfully\n"););

        devObjP->Flags |= DO_DIRECT_IO;
        RtlInitUnicodeString(&devExtP->AdapterName,amacNameP->Buffer);
		devExtP->NdisProtocolHandle=aProtoHandle;

		IF_LOUD(DbgPrint("Trying to create SymLink %ws\n",deviceSymLink.Buffer););

		if (IoCreateSymbolicLink(&deviceSymLink,&deviceName) != STATUS_SUCCESS)	{
			IF_LOUD(DbgPrint("\n\nError creating SymLink %ws\nn", deviceSymLink.Buffer););

			ExFreePool(deviceName.Buffer);
			ExFreePool(deviceSymLink.Buffer);

			devExtP->ExportString = NULL;

			return FALSE;
    }

		IF_LOUD(DbgPrint("SymLink %ws successfully created.\n\n", deviceSymLink.Buffer););

		devExtP->ExportString = deviceSymLink.Buffer;

        ExFreePool(deviceName.Buffer);

		return TRUE;
    }
  
	else 
	{
		IF_LOUD(DbgPrint("\n\nIoCreateDevice status = %x\n", status););

		ExFreePool(deviceName.Buffer);
		ExFreePool(deviceSymLink.Buffer);
		
		return FALSE;
	}
}

//-------------------------------------------------------------------

VOID STDCALL
NPF_Unload(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_OBJECT     DeviceObject;
    PDEVICE_OBJECT     OldDeviceObject;
    PDEVICE_EXTENSION  DeviceExtension;
	
    NDIS_HANDLE        NdisProtocolHandle;
    NDIS_STATUS        Status;
	
	NDIS_STRING		   SymLink;

    IF_LOUD(DbgPrint("NPF: Unload\n"););

	DeviceObject    = DriverObject->DeviceObject;

    while (DeviceObject != NULL) {
        OldDeviceObject=DeviceObject;
		
        DeviceObject=DeviceObject->NextDevice;

		DeviceExtension = OldDeviceObject->DeviceExtension;

        NdisProtocolHandle=DeviceExtension->NdisProtocolHandle;

		IF_LOUD(DbgPrint("Deleting Adapter %ws, Protocol Handle=%x, Device Obj=%x (%x)\n",
			DeviceExtension->AdapterName.Buffer,
			NdisProtocolHandle,
			DeviceObject,
			OldDeviceObject););

		if (DeviceExtension->ExportString)
		{
			RtlInitUnicodeString(&SymLink , DeviceExtension->ExportString);
		
			IF_LOUD(DbgPrint("Deleting SymLink at %p\n", SymLink.Buffer););

			IoDeleteSymbolicLink(&SymLink);
			ExFreePool(DeviceExtension->ExportString);
		}

        IoDeleteDevice(OldDeviceObject);
    }

	NdisDeregisterProtocol(
        &Status,
        NdisProtocolHandle
        );

	// Free the adapters names
	ExFreePool( bindP );
}

//-------------------------------------------------------------------

NTSTATUS STDCALL
NPF_IoControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
    POPEN_INSTANCE      Open;
    PIO_STACK_LOCATION  IrpSp;
    PLIST_ENTRY         RequestListEntry;
    PINTERNAL_REQUEST   pRequest;
    ULONG               FunctionCode;
    NDIS_STATUS	        Status;
    PLIST_ENTRY         PacketListEntry;
	UINT				i;
	PUCHAR				tpointer;
	ULONG				dim,timeout;
	PUCHAR				prog;
	PPACKET_OID_DATA    OidData;
	int					*StatsBuf;
    PNDIS_PACKET        pPacket;
	ULONG				mode;
	PWSTR				DumpNameBuff;
	PUCHAR				TmpBPFProgram;
	INT					WriteRes;
	BOOLEAN				SyncWrite = FALSE;
	struct bpf_insn		*initprogram;
	ULONG				insns;
	ULONG				cnt;
	BOOLEAN				IsExtendedFilter=FALSE;

    IF_LOUD(DbgPrint("NPF: IoControl\n");)
		
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
    FunctionCode=IrpSp->Parameters.DeviceIoControl.IoControlCode;
    Open=IrpSp->FileObject->FsContext;

    Irp->IoStatus.Status = STATUS_SUCCESS;

    IF_LOUD(DbgPrint("NPF: Function code is %08lx  buff size=%08lx  %08lx\n",FunctionCode,IrpSp->Parameters.DeviceIoControl.InputBufferLength,IrpSp->Parameters.DeviceIoControl.OutputBufferLength);)
		

	switch (FunctionCode){
		
	case BIOCGSTATS: //function to get the capture stats
		
		if(IrpSp->Parameters.DeviceIoControl.OutputBufferLength < 4*sizeof(INT)){			
			EXIT_FAILURE(0);
		}

		*(((PUINT)Irp->UserBuffer)+3) = Open->Accepted;
		*(((PUINT)Irp->UserBuffer)) = Open->Received;
		*(((PUINT)Irp->UserBuffer)+1) = Open->Dropped;
		*(((PUINT)Irp->UserBuffer)+2) = 0;		// Not yet supported
		
		EXIT_SUCCESS(4*sizeof(INT));
		
		break;
		
	case BIOCGEVNAME: //function to get the name of the event associated with the current instance

		if(IrpSp->Parameters.DeviceIoControl.OutputBufferLength<26){			
			EXIT_FAILURE(0);
		}

		RtlCopyMemory(Irp->UserBuffer,(Open->ReadEventName.Buffer)+18,26);

		EXIT_SUCCESS(26);

		break;

	case BIOCSENDPACKETSSYNC:

		SyncWrite = TRUE;

	case BIOCSENDPACKETSNOSYNC:

		WriteRes = NPF_BufferedWrite(Irp,
			(PUCHAR)Irp->AssociatedIrp.SystemBuffer,
			IrpSp->Parameters.DeviceIoControl.InputBufferLength,
			SyncWrite);

		if( WriteRes != -1)
		{
			EXIT_SUCCESS(WriteRes);
		}
		
		EXIT_FAILURE(WriteRes);

		break;

	case BIOCSETF:  
		
		// Free the previous buffer if it was present
		if(Open->bpfprogram!=NULL){
			TmpBPFProgram=Open->bpfprogram;
			Open->bpfprogram = NULL;
			ExFreePool(TmpBPFProgram);
		}
		
		if (Open->Filter!=NULL)
		{
			JIT_BPF_Filter *OldFilter=Open->Filter;
			Open->Filter=NULL;
			BPF_Destroy_JIT_Filter(OldFilter);
		}
        
		// Get the pointer to the new program
		prog=(PUCHAR)Irp->AssociatedIrp.SystemBuffer;
		
		if(prog==NULL){
			IF_LOUD(DbgPrint("0001");)
			
			EXIT_FAILURE(0);
		}
		
		insns=(IrpSp->Parameters.DeviceIoControl.InputBufferLength)/sizeof(struct bpf_insn);
		
		//count the number of operative instructions
		for (cnt=0;(cnt<insns) &&(((struct bpf_insn*)prog)[cnt].code!=BPF_SEPARATION); cnt++);
		
		IF_LOUD(DbgPrint("Operative instructions=%u\n",cnt);)

		if (((struct bpf_insn*)prog)[cnt].code==BPF_SEPARATION && (insns-cnt-1)!=0) 
		{
			IF_LOUD(DbgPrint("Initialization instructions=%u\n",insns-cnt-1);)
	
			IsExtendedFilter=TRUE;

			initprogram=&((struct bpf_insn*)prog)[cnt+1];
			
			if(bpf_filter_init(initprogram,&(Open->mem_ex),&(Open->tme), &G_Start_Time)!=INIT_OK)
			{
			
				IF_LOUD(DbgPrint("Error initializing NPF machine (bpf_filter_init)\n");)
				
				EXIT_FAILURE(0);
			}
		}

		//the NPF processor has been initialized, we have to validate the operative instructions
		insns=cnt;
		
		if(bpf_validate((struct bpf_insn*)prog,cnt,Open->mem_ex.size)==0)
		{
			IF_LOUD(DbgPrint("Error validating program");)
			//FIXME: the machine has been initialized(?), but the operative code is wrong. 
			//we have to reset the machine!
			//something like: reallocate the mem_ex, and reset the tme_core
			EXIT_FAILURE(0);
		}
		
		// Allocate the memory to contain the new filter program
		// We could need the original BPF binary if we are forced to use bpf_filter_with_2_buffers()
#define NPF_TAG_BPFPROG  TAG('4', 'P', 'W', 'A')
		TmpBPFProgram=(PUCHAR)ExAllocatePoolWithTag(NonPagedPool, cnt*sizeof(struct bpf_insn), NPF_TAG_BPFPROG);
		if (TmpBPFProgram==NULL){
			IF_LOUD(DbgPrint("Error - No memory for filter");)
			// no memory
			EXIT_FAILURE(0);
		}
		
		//copy the program in the new buffer
		RtlCopyMemory(TmpBPFProgram,prog,cnt*sizeof(struct bpf_insn));
		Open->bpfprogram=TmpBPFProgram;
		
		// Create the new JIT filter function
		if(!IsExtendedFilter)
			if((Open->Filter=BPF_jitter((struct bpf_insn*)Open->bpfprogram,cnt)) == NULL) {
			IF_LOUD(DbgPrint("Error jittering filter");)
			EXIT_FAILURE(0);
		}

		//return
		Open->Bhead = 0;
		Open->Btail = 0;
		(INT)Open->BLastByte = -1;
		Open->Received = 0;		
		Open->Dropped = 0;
		Open->Accepted = 0;

		EXIT_SUCCESS(IrpSp->Parameters.DeviceIoControl.InputBufferLength);
		
		break;		
		
	case BIOCSMODE:  //set the capture mode
		
		mode=*((PULONG)Irp->AssociatedIrp.SystemBuffer);
		
		if(mode == MODE_CAPT){
			Open->mode=MODE_CAPT;
			
			EXIT_SUCCESS(0);
		}
 		else if (mode==MODE_MON){
 			Open->mode=MODE_MON;

			EXIT_SUCCESS(0);
 		}	
		else{
			if(mode & MODE_STAT){
				Open->mode = MODE_STAT;
				Open->Nbytes.QuadPart=0;
				Open->Npackets.QuadPart=0;
				
				if(Open->TimeOut.QuadPart==0)Open->TimeOut.QuadPart=-10000000;
				
			}
			
			if(mode & MODE_DUMP){
				
				Open->mode |= MODE_DUMP;
				Open->MinToCopy=(Open->BufSize<2000000)?Open->BufSize/2:1000000;
				
			}	
			EXIT_SUCCESS(0);
		}
		
		EXIT_FAILURE(0);
		
		break;

	case BIOCSETDUMPFILENAME:

		if(Open->mode & MODE_DUMP)
		{
			
			// Close current dump file
			if(Open->DumpFileHandle != NULL){
				NPF_CloseDumpFile(Open);
				Open->DumpFileHandle = NULL;
			}
			
			if(IrpSp->Parameters.DeviceIoControl.InputBufferLength == 0){
				EXIT_FAILURE(0);
			}
			
			// Allocate the buffer that will contain the string
#define NPF_TAG_DUMPNAMEBUF  TAG('5', 'P', 'W', 'A')
			DumpNameBuff=ExAllocatePoolWithTag(NonPagedPool, IrpSp->Parameters.DeviceIoControl.InputBufferLength, NPF_TAG_DUMPNAMEBUF);
			if(DumpNameBuff==NULL || Open->DumpFileName.Buffer!=NULL){
				IF_LOUD(DbgPrint("NPF: unable to allocate the dump filename: not enough memory or name already set\n");)
					EXIT_FAILURE(0);
			}
			
			// Copy the buffer
			RtlCopyBytes((PVOID)DumpNameBuff, 
				Irp->AssociatedIrp.SystemBuffer, 
				IrpSp->Parameters.DeviceIoControl.InputBufferLength);
			
			// Force a \0 at the end of the filename to avoid that malformed strings cause RtlInitUnicodeString to crash the system 
			((SHORT*)DumpNameBuff)[IrpSp->Parameters.DeviceIoControl.InputBufferLength/2-1]=0;
			
			// Create the unicode string
			RtlInitUnicodeString(&Open->DumpFileName, DumpNameBuff);
			
			IF_LOUD(DbgPrint("NPF: dump file name set to %ws, len=%d\n",
				Open->DumpFileName.Buffer,
				IrpSp->Parameters.DeviceIoControl.InputBufferLength);)
				
			// Try to create the file
			if ( NT_SUCCESS( NPF_OpenDumpFile(Open,&Open->DumpFileName,FALSE)) &&
				NT_SUCCESS( NPF_StartDump(Open))){
				
				EXIT_SUCCESS(0);
			}
		}
		
		EXIT_FAILURE(0);
		
		break;
				
	case BIOCSETDUMPLIMITS:

		Open->MaxDumpBytes = *(PULONG)Irp->AssociatedIrp.SystemBuffer;
		Open->MaxDumpPacks = *((PULONG)Irp->AssociatedIrp.SystemBuffer + 1);

		IF_LOUD(DbgPrint("NPF: Set dump limits to %u bytes, %u packs\n", Open->MaxDumpBytes, Open->MaxDumpPacks);)

		EXIT_SUCCESS(0);

		break;

	case BIOCISDUMPENDED:
		if(IrpSp->Parameters.DeviceIoControl.OutputBufferLength < 4){			
			EXIT_FAILURE(0);
		}

		*((UINT*)Irp->UserBuffer) = (Open->DumpLimitReached)?1:0;

		EXIT_SUCCESS(4);

		break;

	case BIOCSETBUFFERSIZE:
		
		// Get the number of bytes to allocate
		dim = *((PULONG)Irp->AssociatedIrp.SystemBuffer);

		// Free the old buffer
		tpointer = Open->Buffer;
		if(tpointer != NULL){
			Open->BufSize = 0;
			Open->Buffer = NULL;
			ExFreePool(tpointer);
		}
		// Allocate the new buffer
		if(dim!=0){
#define NPF_TAG_TPOINTER  TAG('6', 'P', 'W', 'A')
			tpointer = ExAllocatePoolWithTag(NonPagedPool, dim, NPF_TAG_TPOINTER);
			if (tpointer==NULL)	{
				// no memory
				Open->BufSize = 0;
				Open->Buffer = NULL;
				EXIT_FAILURE(0);
			}
		}
		else
			tpointer = NULL;

		Open->Buffer = tpointer;
		Open->Bhead = 0;
		Open->Btail = 0;
		(INT)Open->BLastByte = -1;
		
		Open->BufSize = (UINT)dim;
		EXIT_SUCCESS(dim);
		
		break;
		
	case BIOCSRTIMEOUT: //set the timeout on the read calls
		
		timeout = *((PULONG)Irp->AssociatedIrp.SystemBuffer);
		if((int)timeout==-1)
			Open->TimeOut.QuadPart=(LONGLONG)IMMEDIATE;
		else {
			Open->TimeOut.QuadPart=(LONGLONG)timeout;
			Open->TimeOut.QuadPart*=10000;
			Open->TimeOut.QuadPart=-Open->TimeOut.QuadPart;
		}

		//IF_LOUD(DbgPrint("NPF: read timeout set to %d:%d\n",Open->TimeOut.HighPart,Open->TimeOut.LowPart);)
		EXIT_SUCCESS(timeout);
		
		break;
		
	case BIOCSWRITEREP: //set the writes repetition number
		
		Open->Nwrites = *((PULONG)Irp->AssociatedIrp.SystemBuffer);
		
		EXIT_SUCCESS(Open->Nwrites);
		
		break;

	case BIOCSMINTOCOPY: //set the minimum buffer's size to copy to the application

		Open->MinToCopy = *((PULONG)Irp->AssociatedIrp.SystemBuffer);
		
		EXIT_SUCCESS(Open->MinToCopy);
		
		break;
		
	case IOCTL_PROTOCOL_RESET:
		
        IF_LOUD(DbgPrint("NPF: IoControl - Reset request\n");)

		IoMarkIrpPending(Irp);
		Irp->IoStatus.Status = STATUS_SUCCESS;

		ExInterlockedInsertTailList(&Open->ResetIrpList,&Irp->Tail.Overlay.ListEntry,&Open->RequestSpinLock);
        NdisReset(&Status,Open->AdapterHandle);
        if (Status != NDIS_STATUS_PENDING) {
            IF_LOUD(DbgPrint("NPF: IoControl - ResetComplete being called\n");)
				NPF_ResetComplete(Open,Status);
        }
		
		break;
		
		
	case BIOCSETOID:
	case BIOCQUERYOID:
		
		// Extract a request from the list of free ones
		RequestListEntry=ExInterlockedRemoveHeadList(&Open->RequestList,&Open->RequestSpinLock);
		if (RequestListEntry == NULL)
		{
			EXIT_FAILURE(0);
		}

		pRequest=CONTAINING_RECORD(RequestListEntry,INTERNAL_REQUEST,ListElement);
		pRequest->Irp=Irp;
		pRequest->Internal = FALSE;

        
		//
        //  See if it is an Ndis request
        //
        OidData=Irp->AssociatedIrp.SystemBuffer;
		
        if (((FunctionCode == BIOCSETOID) || (FunctionCode == BIOCQUERYOID))
            &&
            (IrpSp->Parameters.DeviceIoControl.InputBufferLength == IrpSp->Parameters.DeviceIoControl.OutputBufferLength)
            &&
            (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(PACKET_OID_DATA))
            &&
            (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(PACKET_OID_DATA)-1+OidData->Length)) {
			
            IF_LOUD(DbgPrint("NPF: IoControl: Request: Oid=%08lx, Length=%08lx\n",OidData->Oid,OidData->Length);)
				
				//
				//  The buffer is valid
				//
				if (FunctionCode == BIOCSETOID){
					
					pRequest->Request.RequestType=NdisRequestSetInformation;
					pRequest->Request.DATA.SET_INFORMATION.Oid=OidData->Oid;
					
					pRequest->Request.DATA.SET_INFORMATION.InformationBuffer=OidData->Data;
					pRequest->Request.DATA.SET_INFORMATION.InformationBufferLength=OidData->Length;
				} else{
					pRequest->Request.RequestType=NdisRequestQueryInformation;
					pRequest->Request.DATA.QUERY_INFORMATION.Oid=OidData->Oid;
					
					pRequest->Request.DATA.QUERY_INFORMATION.InformationBuffer=OidData->Data;
					pRequest->Request.DATA.QUERY_INFORMATION.InformationBufferLength=OidData->Length;
					
				}

				NdisResetEvent(&Open->IOEvent);
				//
				//  submit the request
				//
				NdisRequest(
					&Status,
					Open->AdapterHandle,
					&pRequest->Request
					);
				
        } else {
            //
            //  buffer too small
            //
            Status=NDIS_STATUS_FAILURE;
            pRequest->Request.DATA.SET_INFORMATION.BytesRead=0;
            pRequest->Request.DATA.QUERY_INFORMATION.BytesWritten=0;
			
        }
		
        if (Status != NDIS_STATUS_PENDING) {
            IF_LOUD(DbgPrint("NPF: Calling RequestCompleteHandler\n");)
				
			NPF_RequestComplete(Open, &pRequest->Request, Status);
            return Status;
			
        }

		NdisWaitEvent(&Open->IOEvent, 5000);

		return(Open->IOStatus);
		
		break;
		
		
	default:
		
		EXIT_FAILURE(0);
	}
	
	return Status;
}

//-------------------------------------------------------------------

VOID
NPF_RequestComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS   Status
    )

{
    POPEN_INSTANCE      Open;
    PIO_STACK_LOCATION  IrpSp;
    PIRP                Irp;
    PINTERNAL_REQUEST   pRequest;
    UINT                FunctionCode;
	KIRQL				OldIrq;

    PPACKET_OID_DATA    OidData;

    IF_LOUD(DbgPrint("NPF: RequestComplete\n");)

    Open= (POPEN_INSTANCE)ProtocolBindingContext;

    pRequest=CONTAINING_RECORD(NdisRequest,INTERNAL_REQUEST,Request);
    Irp=pRequest->Irp;

	if(pRequest->Internal == TRUE){

		// Put the request in the list of the free ones
		ExInterlockedInsertTailList(&Open->RequestList, &pRequest->ListElement, &Open->RequestSpinLock);
		
	    if(Status != NDIS_STATUS_SUCCESS)
			Open->MaxFrameSize = 1514;	// Assume Ethernet

		// We always return success, because the adapter has been already opened
		Irp->IoStatus.Status = NDIS_STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return;
	}

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    FunctionCode=IrpSp->Parameters.DeviceIoControl.IoControlCode;

    OidData=Irp->AssociatedIrp.SystemBuffer;

    if (FunctionCode == BIOCSETOID) {

        OidData->Length=pRequest->Request.DATA.SET_INFORMATION.BytesRead;

    } else {

        if (FunctionCode == BIOCQUERYOID) {

            OidData->Length=pRequest->Request.DATA.QUERY_INFORMATION.BytesWritten;

		    IF_LOUD(DbgPrint("RequestComplete: BytesWritten=%d\n",pRequest->Request.DATA.QUERY_INFORMATION.BytesWritten);)
        }

    }

    Irp->IoStatus.Information=IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    IF_LOUD(DbgPrint("RequestComplete: BytesReturned=%d\n",IrpSp->Parameters.DeviceIoControl.InputBufferLength);)

    ExInterlockedInsertTailList(
        &Open->RequestList,
        &pRequest->ListElement,
        &Open->RequestSpinLock);

    Irp->IoStatus.Status = Status;

	Open->IOStatus = Status;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	// Unlock the caller
	NdisSetEvent(&Open->IOEvent);

    return;


}

//-------------------------------------------------------------------

VOID
NPF_Status(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN NDIS_STATUS   Status,
    IN PVOID         StatusBuffer,
    IN UINT          StatusBufferSize
    )

{

    IF_LOUD(DbgPrint("NPF: Status Indication\n");)

    return;

}

//-------------------------------------------------------------------

VOID
NPF_StatusComplete(
    IN NDIS_HANDLE  ProtocolBindingContext
    )

{

    IF_LOUD(DbgPrint("NPF: StatusIndicationComplete\n");)

    return;

}

//-------------------------------------------------------------------

NTSTATUS
NPF_ReadRegistry(
    IN  PWSTR              *MacDriverName,
    IN  PWSTR              *PacketDriverName,
    IN  PUNICODE_STRING     RegistryPath
    )

{
    NTSTATUS   Status;

    RTL_QUERY_REGISTRY_TABLE ParamTable[4];

    PWSTR      Bind       = L"Bind";
    PWSTR      Export     = L"Export";
    PWSTR      Parameters = L"Parameters";
    PWSTR      Linkage    = L"Linkage";

    PWCHAR     Path;

#define NPF_TAG_PATH  TAG('7', 'P', 'W', 'A')
    Path=ExAllocatePoolWithTag(PagedPool, RegistryPath->Length+sizeof(WCHAR), NPF_TAG_PATH);

    if (Path == NULL) {
		IF_LOUD(DbgPrint("\nPacketReadRegistry: returing STATUS_INSUFFICIENT_RESOURCES\n");)
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
        Path,
        RegistryPath->Length+sizeof(WCHAR)
        );

    RtlCopyMemory(
        Path,
        RegistryPath->Buffer,
        RegistryPath->Length
        );

    IF_LOUD(DbgPrint("NPF: Reg path is %ws\n",RegistryPath->Buffer);)

    RtlZeroMemory(
        ParamTable,
        sizeof(ParamTable)
        );



    //
    //  change to the linkage key
    //
    //ParamTable[0].QueryRoutine = NULL;
    ParamTable[0].QueryRoutine = NPF_QueryRegistryRoutine;
    ParamTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    ParamTable[0].Name = Linkage;


    //
    //  Get the name of the mac driver we should bind to
    //

    ParamTable[1].QueryRoutine = NPF_QueryRegistryRoutine;
    ParamTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                          RTL_QUERY_REGISTRY_NOEXPAND;

    ParamTable[1].Name = Bind;
    ParamTable[1].EntryContext = (PVOID)MacDriverName;
    ParamTable[1].DefaultType = REG_MULTI_SZ;

    //
    //  Get the name that we should use for the driver object
    //

    ParamTable[2].QueryRoutine = NPF_QueryRegistryRoutine;
    ParamTable[2].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                          RTL_QUERY_REGISTRY_NOEXPAND;

    ParamTable[2].Name = Export;
    ParamTable[2].EntryContext = (PVOID)PacketDriverName;
    ParamTable[2].DefaultType = REG_MULTI_SZ;


    Status=RtlQueryRegistryValues(
               RTL_REGISTRY_ABSOLUTE,
               Path,
               ParamTable,
               NULL,
               NULL
               );

    if (Status != STATUS_SUCCESS) {
		// insert hard coded parameters here while registry on ROS is not working...
		IF_LOUD(DbgPrint("PacketReadRegistry() RtlQueryRegistryValues failed - returing fixed parameters\n");)

        *MacDriverName = ExAllocatePool(PagedPool, 50 * sizeof(WCHAR));
		//memcpy(*MacDriverName,    L"\\Device\\ne2000", 15 * sizeof(WCHAR));
		memcpy(*MacDriverName,    L"\\Device\\ne2000", 15 * sizeof(WCHAR));
		
        *PacketDriverName = ExAllocatePool(PagedPool, 50 * sizeof(WCHAR));
		memcpy(*PacketDriverName, L"\\Device\\NPF_ne2000", 19 * sizeof(WCHAR));
		Status = STATUS_SUCCESS;

	}

    ExFreePool(Path);
    return Status;
}

//-------------------------------------------------------------------

NTSTATUS STDCALL
NPF_QueryRegistryRoutine(
    IN PWSTR     ValueName,
    IN ULONG     ValueType,
    IN PVOID     ValueData,
    IN ULONG     ValueLength,
    IN PVOID     Context,
    IN PVOID     EntryContext
    )

{

    PUCHAR       Buffer;

    IF_LOUD(DbgPrint("Perf: QueryRegistryRoutine\n");)

    if (ValueType != REG_MULTI_SZ) {

        return STATUS_OBJECT_NAME_NOT_FOUND;

    }

#define NPF_TAG_REGBUF  TAG('8', 'P', 'W', 'A')
    Buffer=ExAllocatePoolWithTag(NonPagedPool, ValueLength, NPF_TAG_REGBUF);

    if (Buffer==NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    RtlCopyMemory(
        Buffer,
        ValueData,
        ValueLength
        );

    *((PUCHAR *)EntryContext)=Buffer;

    return STATUS_SUCCESS;

}
