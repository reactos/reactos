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
//#include <net/miniport.h>
#include <ddk/ntddk.h>
//#include "<ntiologc.h>"
#include <net/ndis.h>
#endif

#include "ntddpack.h"
#include "debug.h"
#include "packet.h"


#if DBG
//
// Declare the global debug flag for this driver.
//

ULONG PacketDebugFlag = PACKET_DEBUG_LOUD;

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
// Strings
//

NDIS_STRING PacketName = NDIS_STRING_CONST("Packet_");
NDIS_STRING devicePrefix = NDIS_STRING_CONST("\\Device\\");
NDIS_STRING symbolicLinkPrefix = NDIS_STRING_CONST("\\DosDevices\\");
NDIS_STRING tcpLinkageKeyName = NDIS_STRING_CONST("\\Registry\\Machine\\System"
								L"\\CurrentControlSet\\Services\\Tcpip\\Linkage");
NDIS_STRING AdapterListKey = NDIS_STRING_CONST("\\Registry\\Machine\\System"
								L"\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");
NDIS_STRING bindValueName = NDIS_STRING_CONST("Bind");

//
// Global variable that points to the names of the bound adapters
//
WCHAR* bindP = NULL;

//
//  Packet Driver's entry routine.
//

//NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT DriverObject,
//			     PUNICODE_STRING RegistryPath)

NTSTATUS
STDCALL
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NDIS_STATUS NdisStatus = STATUS_SUCCESS;
    NDIS_PROTOCOL_CHARACTERISTICS  ProtocolChar;
    UNICODE_STRING MacDriverName;
    UNICODE_STRING UnicodeDeviceName;
    PDEVICE_OBJECT DeviceObject = NULL;
    PDEVICE_EXTENSION DeviceExtension = NULL;
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
	
	//This driver at the moment works only on single processor machines
	if(NdisSystemProcessorCount() != 1){
		return STATUS_IMAGE_MP_UP_MISMATCH;
	}
    IF_LOUD(DbgPrint("\n\nPacket: DriverEntry\n");)
	RtlZeroMemory(&ProtocolChar,sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
#ifdef NDIS50
    ProtocolChar.MajorNdisVersion            = 5;
#else
    ProtocolChar.MajorNdisVersion            = 3;
#endif
    ProtocolChar.MinorNdisVersion            = 0;
//    ProtocolChar.Reserved                    = 0;
    ProtocolChar.OpenAdapterCompleteHandler  = PacketOpenAdapterComplete;
    ProtocolChar.CloseAdapterCompleteHandler = PacketCloseAdapterComplete;
    ProtocolChar.u2.SendCompleteHandler         = PacketSendComplete;
    ProtocolChar.u3.TransferDataCompleteHandler = PacketTransferDataComplete;
    ProtocolChar.ResetCompleteHandler        = PacketResetComplete;
    ProtocolChar.RequestCompleteHandler      = PacketRequestComplete;
    ProtocolChar.u4.ReceiveHandler              = Packet_tap;
    ProtocolChar.ReceiveCompleteHandler      = PacketReceiveComplete;
    ProtocolChar.StatusHandler               = PacketStatus;
    ProtocolChar.StatusCompleteHandler       = PacketStatusComplete;
#ifdef NDIS50
    ProtocolChar.BindAdapterHandler          = PacketBindAdapter;
    ProtocolChar.UnbindAdapterHandler        = PacketUnbindAdapter;
    ProtocolChar.ReceivePacketHandler        = NULL;
#endif
    ProtocolChar.Name                        = ProtoName;
/*
NdisRegisterProtocol(
    OUT PNDIS_STATUS                    Status,
    OUT PNDIS_HANDLE                    NdisProtocolHandle,
    IN  PNDIS_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
    IN  UINT                            CharacteristicsLength);
 */
    NdisRegisterProtocol(
        &NdisStatus,
        &NdisProtocolHandle,
        &ProtocolChar,
        sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        IF_LOUD(DbgPrint("Packet: Failed to register protocol with NDIS\n");)
        return NdisStatus;
    }
    //
    // Set up the device driver entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] = PacketOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = PacketClose;
    DriverObject->MajorFunction[IRP_MJ_READ]   = PacketRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]  = PacketWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = PacketIoControl;
    DriverObject->DriverUnload = PacketUnload;
    //
    //  Get the name of the Packet driver and the name of the MAC driver
    //  to bind to from the registry
    //
    Status=PacketReadRegistry(
               &BindString,
               &ExportString,
               RegistryPath
               );
    if (Status != STATUS_SUCCESS) {
		IF_LOUD(DbgPrint("Trying dynamic binding\n");)	
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
            BindString
            );
        RtlInitUnicodeString(
            &UnicodeDeviceName,
            ExportString
            );
        //
        //  Advance to the next string of the MULTI_SZ string
        //
        BindString   += (MacDriverName.Length+sizeof(UNICODE_NULL))/sizeof(WCHAR);
        ExportString += (UnicodeDeviceName.Length+sizeof(UNICODE_NULL))/sizeof(WCHAR);
        IF_LOUD(DbgPrint("Packet: DeviceName=%ws  MacName=%ws\n",UnicodeDeviceName.Buffer,MacDriverName.Buffer);)
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
                    &DeviceObject
                    );
        if (Status != STATUS_SUCCESS) {
            IF_LOUD(DbgPrint("Packet: IoCreateDevice() failed:\n");)
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
        //  Managed to create at least on device.
        //
        return STATUS_SUCCESS;
    }
    ExFreePool(BindStringSave);
    ExFreePool(ExportStringSave);
RegistryError:
    NdisDeregisterProtocol(&NdisStatus, NdisProtocolHandle);
    Status=STATUS_UNSUCCESSFUL;
    return(Status);
}

//
// Return the list of the MACs from SYSTEM\CurrentControlSet\Control\Class\{4D36E972-E325-11CE-BFC1-08002BE10318}
//

PWCHAR getAdaptersList(void)
{
	PWCHAR DeviceNames = (PWCHAR) ExAllocatePool(PagedPool, 4096);
	PKEY_VALUE_PARTIAL_INFORMATION result = NULL;
	OBJECT_ATTRIBUTES objAttrs;
	NTSTATUS status;
	HANDLE keyHandle;
	UINT BufPos=0;
	
	if (DeviceNames == NULL) {
		IF_LOUD(DbgPrint("Unable the allocate the buffer for the list of the network adapters\n");)
			return NULL;
	}
	InitializeObjectAttributes(&objAttrs, &AdapterListKey,
		OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwOpenKey(&keyHandle, KEY_READ, &objAttrs);
	if (!NT_SUCCESS(status)) {
		IF_LOUD(DbgPrint("\n\nStatus of %x opening %ws\n", status, tcpLinkageKeyName.Buffer);)
    } else { //OK
		ULONG resultLength;
		KEY_VALUE_PARTIAL_INFORMATION valueInfo;
		CHAR AdapInfo[1024];
		UINT i=0;
		IF_LOUD(DbgPrint("getAdaptersList: scanning the list of the adapters in the registry, DeviceNames=%x\n",DeviceNames);)
			// Scan the list of the devices
			while((status=ZwEnumerateKey(keyHandle,i,KeyBasicInformation,AdapInfo,sizeof(AdapInfo),&resultLength))==STATUS_SUCCESS) {
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
					PKEY_VALUE_PARTIAL_INFORMATION valueInfoP =	(PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePool(PagedPool, valueInfoLength);
					if (valueInfoP != NULL) {
						status = ZwQueryValueKey(ExportKeyHandle, &FinalExportKey,
							KeyValuePartialInformation,
							valueInfoP,
							valueInfoLength, &resultLength);
						if (!NT_SUCCESS(status)) {
							IF_LOUD(DbgPrint("Status of %x querying key value\n", status);)
                        } else{
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

//
// Return the MACs that bind to TCP/IP.
//

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
      ULONG valueInfoLength = valueInfo.DataLength +
        FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]);
      PKEY_VALUE_PARTIAL_INFORMATION valueInfoP =
        (PKEY_VALUE_PARTIAL_INFORMATION)
        ExAllocatePool(PagedPool, valueInfoLength);
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

//
// create a device associated with a given MAC.
//

BOOLEAN createDevice(IN OUT PDRIVER_OBJECT adriverObjectP,
                     IN PUNICODE_STRING amacNameP, NDIS_HANDLE aProtoHandle)
{
  BOOLEAN result = FALSE;
  NTSTATUS status;
  PDEVICE_OBJECT devObjP;
  UNICODE_STRING deviceName;

  IF_LOUD(DbgPrint("\n\ncreateDevice for MAC %ws\n", amacNameP->Buffer);)
  if (RtlCompareMemory(amacNameP->Buffer, devicePrefix.Buffer,
                       devicePrefix.Length) < devicePrefix.Length) {
    return result;
  }

  deviceName.Length = 0;
  deviceName.MaximumLength = (USHORT)(amacNameP->Length + PacketName.Length + sizeof(UNICODE_NULL));
  deviceName.Buffer = ExAllocatePool(PagedPool, deviceName.MaximumLength);
  if (deviceName.Buffer != NULL) {
    RtlAppendUnicodeStringToString(&deviceName, &devicePrefix);
    RtlAppendUnicodeStringToString(&deviceName, &PacketName);
    RtlAppendUnicodeToString(&deviceName, amacNameP->Buffer +
                             devicePrefix.Length / sizeof(WCHAR));
	IF_LOUD(DbgPrint("\n\nDevice name: %ws\n", deviceName.Buffer);)
	status = IoCreateDevice(adriverObjectP, sizeof(PDEVICE_EXTENSION),
                            &deviceName, FILE_DEVICE_TRANSPORT, 0, FALSE,
                            &devObjP);
    if (NT_SUCCESS(status)) {
      PDEVICE_EXTENSION devExtP = (PDEVICE_EXTENSION)devObjP->DeviceExtension;
	  IF_LOUD(DbgPrint("\n\nDevice created succesfully\n");)
      devObjP->Flags |= DO_DIRECT_IO;
	  devExtP->DeviceObject = devObjP;
      RtlInitUnicodeString(&devExtP->AdapterName,amacNameP->Buffer);   
	  devExtP->BindString = NULL;
      devExtP->ExportString = NULL;
	  devExtP->NdisProtocolHandle=aProtoHandle;
    }
    else IF_LOUD(DbgPrint("\n\nIoCreateDevice status = %x\n", status););
  }
  if (deviceName.Length > 0) 
	  ExFreePool(deviceName.Buffer);
  return result;
}



VOID STDCALL
PacketUnload(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_OBJECT     DeviceObject;
    PDEVICE_OBJECT     OldDeviceObject;
    PDEVICE_EXTENSION  DeviceExtension;
    NDIS_HANDLE        NdisProtocolHandle;
    NDIS_STATUS        Status;
	
    IF_LOUD(DbgPrint("Packet: Unload\n");)
		
	DeviceObject    = DriverObject->DeviceObject;
    while (DeviceObject != NULL) {
        DeviceExtension = DeviceObject->DeviceExtension;
        NdisProtocolHandle=DeviceExtension->NdisProtocolHandle;
        OldDeviceObject=DeviceObject;
        DeviceObject=DeviceObject->NextDevice;
        IoDeleteDevice(OldDeviceObject);
    }
    NdisDeregisterProtocol(
        &Status,
        NdisProtocolHandle
        );
	// Free the adapters names
	ExFreePool( bindP ); 
}


NTSTATUS STDCALL
PacketIoControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
    NDIS_STATUS	        Status;
    POPEN_INSTANCE      Open;
    PIO_STACK_LOCATION  IrpSp;
    PLIST_ENTRY         RequestListEntry;
    PINTERNAL_REQUEST   pRequest;
    ULONG               FunctionCode;
    PLIST_ENTRY         PacketListEntry;
	UINT				i;
	PUCHAR				tpointer;
	ULONG				dim,timeout;
	PUCHAR				prog;
	PPACKET_OID_DATA    OidData;
	int					*StatsBuf;
    PNDIS_PACKET        pPacket;
    PPACKET_RESERVED    Reserved;
	ULONG				mode;
	PUCHAR				TmpBPFProgram;
	
    IF_LOUD(DbgPrint("Packet: IoControl\n");)
		
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
    FunctionCode=IrpSp->Parameters.DeviceIoControl.IoControlCode;
    Open=IrpSp->FileObject->FsContext;
    RequestListEntry=ExInterlockedRemoveHeadList(&Open->RequestList,&Open->RequestSpinLock);
    if (RequestListEntry == NULL) {
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_UNSUCCESSFUL;
    }
    pRequest=CONTAINING_RECORD(RequestListEntry,INTERNAL_REQUEST,ListElement);
    pRequest->Irp=Irp;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IF_LOUD(DbgPrint("Packet: Function code is %08lx  buff size=%08lx  %08lx\n",FunctionCode,IrpSp->Parameters.DeviceIoControl.InputBufferLength,IrpSp->Parameters.DeviceIoControl.OutputBufferLength);)
	switch (FunctionCode){
	case BIOCGSTATS: //function to get the capture stats
		StatsBuf=Irp->UserBuffer;
		StatsBuf[0]=Open->Received;
		StatsBuf[1]=Open->Dropped;
		Irp->IoStatus.Information=8;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		Status=STATUS_SUCCESS;
		break;
	case BIOCGEVNAME: //function to get the name of the event associated with the current instance
		if(IrpSp->Parameters.DeviceIoControl.OutputBufferLength<26){			
			Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			Irp->IoStatus.Information=0;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		    ExInterlockedInsertTailList(&Open->RequestList, &pRequest->ListElement, &Open->RequestSpinLock);
			return STATUS_UNSUCCESSFUL;
		}
		RtlCopyMemory(Irp->UserBuffer,(Open->ReadEventName.Buffer)+18,26);
		Irp->IoStatus.Information=26;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		Status=STATUS_SUCCESS;
		break;
	case BIOCSETF:  //fuction to set a new bpf filter
		//free the previous buffer if it was present
		if(Open->bpfprogram!=NULL){
			TmpBPFProgram=Open->bpfprogram;
			Open->bpfprogram=NULL;
			ExFreePool(TmpBPFProgram);
		}
		//get the pointer to the new program
		prog=(PUCHAR)Irp->AssociatedIrp.SystemBuffer;
		if(prog==NULL){
			Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			Irp->IoStatus.Information=0;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		    ExInterlockedInsertTailList(&Open->RequestList, &pRequest->ListElement, &Open->RequestSpinLock);
			return STATUS_UNSUCCESSFUL;
		}
		//before accepting the program we must check that it's valid
		//Otherwise, a bogus program could easily crash the system
		if(bpf_validate((struct bpf_insn*)prog,(IrpSp->Parameters.DeviceIoControl.InputBufferLength)/sizeof(struct bpf_insn))==0) {
			Irp->IoStatus.Information=0;
			Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		    ExInterlockedInsertTailList(&Open->RequestList, &pRequest->ListElement, &Open->RequestSpinLock);
			return STATUS_UNSUCCESSFUL;
		}
		//allocate the memory to contain the new filter program
		TmpBPFProgram=(PUCHAR)ExAllocatePool(NonPagedPool, IrpSp->Parameters.DeviceIoControl.InputBufferLength);
		if (TmpBPFProgram==NULL){
			// no memory
			Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			Irp->IoStatus.Information=0;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		    ExInterlockedInsertTailList(&Open->RequestList, &pRequest->ListElement, &Open->RequestSpinLock);
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		//copy the program in the new buffer
		RtlCopyMemory(TmpBPFProgram,prog,IrpSp->Parameters.DeviceIoControl.InputBufferLength);
		Open->bpfprogram=TmpBPFProgram;
		//return
		Irp->IoStatus.Information=IrpSp->Parameters.DeviceIoControl.InputBufferLength;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Open->Bhead=0;
		Open->Btail=0;
		Open->BLastByte=0;
		Open->Received=0;		
		Open->Dropped=0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		Status=STATUS_SUCCESS;
		break;
	case BIOCSMODE:  //set the capture mode
		mode=((PULONG)Irp->AssociatedIrp.SystemBuffer)[0];
		if(mode==MODE_STAT){
			Open->mode=MODE_STAT;
			Open->Nbytes.QuadPart=0;
			Open->Npackets.QuadPart=0;
			if(Open->TimeOut.QuadPart==0)Open->TimeOut.QuadPart=-10000000;
			Irp->IoStatus.Information=1;
			Irp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			ExInterlockedInsertTailList(&Open->RequestList, &pRequest->ListElement, &Open->RequestSpinLock);
			return NDIS_STATUS_SUCCESS;
		}
		else if(mode==MODE_CAPT){
			Open->mode=MODE_CAPT;
			Irp->IoStatus.Information=0;
			Irp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		    ExInterlockedInsertTailList(&Open->RequestList, &pRequest->ListElement, &Open->RequestSpinLock);
			return NDIS_STATUS_SUCCESS;
		}
		else{
			Irp->IoStatus.Information=0;
			Irp->IoStatus.Status = NDIS_STATUS_FAILURE;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		    ExInterlockedInsertTailList(&Open->RequestList, &pRequest->ListElement, &Open->RequestSpinLock);
			return NDIS_STATUS_FAILURE;
		}
		break;
	case BIOCSETBUFFERSIZE:	//function to set the dimension of the buffer for the packets
		//get the number of buffers to allocate
		dim=((PULONG)Irp->AssociatedIrp.SystemBuffer)[0];
		//free the old buffer
		if(Open->Buffer!=NULL) ExFreePool(Open->Buffer);
		//allocate the new buffer
		if(dim!=0){
			tpointer=ExAllocatePool(NonPagedPool,dim);
			if (tpointer==NULL) {
				// no memory
				Open->BufSize=0;
				Open->Buffer=NULL;
				Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				Irp->IoStatus.Information = 0;
				IoCompleteRequest(Irp, IO_NO_INCREMENT);
				ExInterlockedInsertTailList(&Open->RequestList, &pRequest->ListElement, &Open->RequestSpinLock);
				return STATUS_INSUFFICIENT_RESOURCES;
			}
		}
		else
			tpointer=NULL;
		Open->Buffer=tpointer;
		Open->Bhead=0;
		Open->Btail=0;
		Open->BLastByte=0;
		Irp->IoStatus.Information=dim;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		Open->BufSize=(UINT)dim;
		Status=STATUS_SUCCESS;
		break;
	case BIOCSRTIMEOUT: //set the timeout on the read calls
		timeout=((PULONG)Irp->AssociatedIrp.SystemBuffer)[0];
		if((int)timeout==-1)
			Open->TimeOut.QuadPart=(LONGLONG)IMMEDIATE;
		else {
			Open->TimeOut.QuadPart=(LONGLONG)timeout;
			Open->TimeOut.QuadPart*=10000;
			Open->TimeOut.QuadPart=-Open->TimeOut.QuadPart;
		}
		IF_LOUD(DbgPrint("Packet: read timeout set to %d:%d\n",Open->TimeOut.HighPart,Open->TimeOut.LowPart);)
		Irp->IoStatus.Information=timeout;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		Status=STATUS_SUCCESS;
		break;
	case BIOCSWRITEREP: //set the writes repetition number
		Open->Nwrites=((PULONG)Irp->AssociatedIrp.SystemBuffer)[0];
		Irp->IoStatus.Information=Open->Nwrites;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		Status=STATUS_SUCCESS;
		break;
	case BIOCSMINTOCOPY: //set the minimum buffer's size to copy to the application
		Open->MinToCopy=((PULONG)Irp->AssociatedIrp.SystemBuffer)[0];
		Irp->IoStatus.Information=Open->MinToCopy;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		Status=STATUS_SUCCESS;
		break;
	case IOCTL_PROTOCOL_RESET:
        IF_LOUD(DbgPrint("Packet: IoControl - Reset request\n");)
		IoMarkIrpPending(Irp);
		Irp->IoStatus.Status = STATUS_SUCCESS;
		ExInterlockedInsertTailList(&Open->ResetIrpList,&Irp->Tail.Overlay.ListEntry,&Open->RequestSpinLock);
        NdisReset(&Status,Open->AdapterHandle);
        if (Status != NDIS_STATUS_PENDING) {
            IF_LOUD(DbgPrint("Packet: IoControl - ResetComplete being called\n");)
				PacketResetComplete(Open,Status);
        }
		break;
	case BIOCSETOID:
	case BIOCQUERYOID:
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
			
            IF_LOUD(DbgPrint("Packet: IoControl: Request: Oid=%08lx, Length=%08lx\n",OidData->Oid,OidData->Length);)
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
            IF_LOUD(DbgPrint("Packet: Calling RequestCompleteHandler\n");)
			PacketRequestComplete(Open, &pRequest->Request, Status);
            return Status;
        }
		NdisWaitEvent(&Open->IOEvent, 5000);
		return(Open->IOStatus);
		break;
	default:
		return(NDIS_STATUS_FAILURE);
		
	}
	ExInterlockedInsertTailList(&Open->RequestList, &pRequest->ListElement, &Open->RequestSpinLock);
	return Status;
}

VOID
PacketRequestComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS   Status
    )
{
    POPEN_INSTANCE      Open;
    PIO_STACK_LOCATION  IrpSp;
    PIRP                Irp;
    PINTERNAL_REQUEST   pRequest;
    ULONG               FunctionCode;

    PPACKET_OID_DATA    OidData;

    IF_LOUD(DbgPrint("Packet: RequestComplete\n");)

    Open= (POPEN_INSTANCE)ProtocolBindingContext;
    pRequest=CONTAINING_RECORD(NdisRequest,INTERNAL_REQUEST,Request);
    Irp=pRequest->Irp;
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
	// Unlock the IOCTL call
	NdisSetEvent(&Open->IOEvent);
    return;
}

VOID
PacketStatus(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN NDIS_STATUS   Status,
    IN PVOID         StatusBuffer,
    IN UINT          StatusBufferSize
    )
{
    IF_LOUD(DbgPrint("Packet: Status Indication\n");)
    return;
}

VOID
PacketStatusComplete(
    IN NDIS_HANDLE  ProtocolBindingContext
    )
{
    IF_LOUD(DbgPrint("Packet: StatusIndicationComplete\n");)
    return;
}

#if 0

NTSTATUS
PacketCreateSymbolicLink(
    IN  PUNICODE_STRING  DeviceName,
    IN  BOOLEAN          Create
    )
{
    UNICODE_STRING UnicodeDosDeviceName;
    NTSTATUS       Status;

    if (DeviceName->Length < sizeof(L"\\Device\\")) {
        return STATUS_UNSUCCESSFUL;
    }
    RtlInitUnicodeString(&UnicodeDosDeviceName,NULL);
    UnicodeDosDeviceName.MaximumLength=DeviceName->Length+sizeof(L"\\DosDevices")+sizeof(UNICODE_NULL);
    UnicodeDosDeviceName.Buffer=ExAllocatePool(
                                    NonPagedPool,
                                    UnicodeDosDeviceName.MaximumLength
                                    );
    if (UnicodeDosDeviceName.Buffer != NULL) {
        RtlZeroMemory(
            UnicodeDosDeviceName.Buffer,
            UnicodeDosDeviceName.MaximumLength
            );
        RtlAppendUnicodeToString(
            &UnicodeDosDeviceName,
            L"\\DosDevices\\"
            );
        RtlAppendUnicodeToString(
            &UnicodeDosDeviceName,
            (DeviceName->Buffer+(sizeof("\\Device")))
            );

        IF_LOUD(DbgPrint("Packet: DosDeviceName is %ws\n",UnicodeDosDeviceName.Buffer);)

        if (Create) {
            Status=IoCreateSymbolicLink(&UnicodeDosDeviceName,DeviceName);
        } else {
            Status=IoDeleteSymbolicLink(&UnicodeDosDeviceName);
        }
        ExFreePool(UnicodeDosDeviceName.Buffer);

    }
    return Status;
}

#endif

NTSTATUS
PacketReadRegistry(
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

    Path=ExAllocatePool(
             PagedPool,
             RegistryPath->Length+sizeof(WCHAR)
             );
    if (Path == NULL) {
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

    IF_LOUD(DbgPrint("Packet: Reg path is %ws\n",RegistryPath->Buffer);)

    RtlZeroMemory(
        ParamTable,
        sizeof(ParamTable)
        );
    //
    //  change to the linkage key
    //
    ParamTable[0].QueryRoutine = NULL;
    ParamTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    ParamTable[0].Name = Linkage;
    //
    //  Get the name of the mac driver we should bind to
    //
    ParamTable[1].QueryRoutine = PacketQueryRegistryRoutine;
    ParamTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                          RTL_QUERY_REGISTRY_NOEXPAND;
    ParamTable[1].Name = Bind;
    ParamTable[1].EntryContext = (PVOID)MacDriverName;
    ParamTable[1].DefaultType = REG_MULTI_SZ;
    //
    //  Get the name that we should use for the driver object
    //
    ParamTable[2].QueryRoutine = PacketQueryRegistryRoutine;
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
    ExFreePool(Path);
    return Status;
}

NTSTATUS
STDCALL
PacketQueryRegistryRoutine(
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
    Buffer=ExAllocatePool(NonPagedPool,ValueLength);
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
