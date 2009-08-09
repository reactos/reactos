/*
Copyright (c) 2006-2007 dogbert <dogber1@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define PUT_GUIDS_HERE
#include <initguid.h>
#include "adapter.hpp"

//#pragma code_seg("PAGE")


NTSTATUS InstallSubdevice(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PWCHAR Name,
    REFGUID PortClassId,
    REFGUID MiniportClassId,
    PFNCREATEINSTANCE MiniportCreate,
    PUNKNOWN UnknownAdapter,
    PRESOURCELIST ResourceList,
    REFGUID PortInterfaceId,
    PUNKNOWN* OutPortUnknown)
{
	NTSTATUS	ntStatus;
	PPORT	   	Port;
	PMINIPORT   MiniPort;

    ////PAGED_CODE();
    DBGPRINT(("InstallSubdevice()"));

	ntStatus = PcNewPort(&Port, PortClassId);
	if (NT_SUCCESS(ntStatus)) {
		if (MiniportCreate) {
			ntStatus = MiniportCreate((PUNKNOWN*)&MiniPort, MiniportClassId, NULL, NonPagedPool);
		} else {
			ntStatus = PcNewMiniport(&MiniPort, MiniportClassId);
		}
	}

	if (!NT_SUCCESS(ntStatus)) {
		Port->Release();
		return ntStatus;
	}

	ntStatus = Port->Init(DeviceObject, Irp, MiniPort, UnknownAdapter, ResourceList);
	if (NT_SUCCESS(ntStatus)) {
		ntStatus = PcRegisterSubdevice(DeviceObject, Name, Port);

		if (OutPortUnknown && NT_SUCCESS (ntStatus)) {
			ntStatus = Port->QueryInterface(IID_IUnknown, (PVOID *)OutPortUnknown);
		}
	}

	if (MiniPort) {
		MiniPort->Release();
	}

	if (Port) {
		Port->Release();
	}

	return ntStatus;
}


NTSTATUS
ProcessResources(
    PRESOURCELIST ResourceList,
    PRESOURCELIST* UartResourceList)
{
	NTSTATUS ntStatus;

	////PAGED_CODE();
	////ASSERT(ResourceList);
	////ASSERT(UartResourceList);
	//DBGPRINT(("ProcessResources()"));
	//DBGPRINT(("NumberOfPorts: %d, NumberOfInterrupts: %d, NumberOfDmas: %d", ResourceList->NumberOfPorts(), ResourceList->NumberOfInterrupts(), ResourceList->NumberOfDmas()));

#ifdef UART
	(*UartResourceList) = NULL;
#endif


	if ((ResourceList->NumberOfPorts() == 0) || (ResourceList->NumberOfPorts() > 2) || (ResourceList->NumberOfInterrupts() != 1) || (ResourceList->NumberOfDmas() != 0)) {
		DBGPRINT(("Unexpected configuration"));
		return STATUS_DEVICE_CONFIGURATION_ERROR;
	}

#ifdef UART
	ntStatus = PcNewResourceSublist(UartResourceList, NULL, PagedPool, ResourceList, 2);
	if (NT_SUCCESS(ntStatus)) {
		(*UartResourceList)->AddPortFromParent(ResourceList, 1);
		(*UartResourceList)->AddInterruptFromParent(ResourceList, 0);
	}
#endif

	return STATUS_SUCCESS;
}


NTSTATUS StartDevice(PDEVICE_OBJECT DeviceObject, PIRP Irp, PRESOURCELIST ResourceList)
{
	NTSTATUS ntStatus;
	PPORT    pPort = 0;
	ULONG*   MPUBase;
#if 0
	//PAGED_CODE();
	//ASSERT(DeviceObject);
	//ASSERT(Irp);
	//ASSERT(ResourceList);
	DBGPRINT(("StartDevice()"));
#endif

	ntStatus = PcNewPort(&pPort,CLSID_PortWaveCyclic);
	if (NT_SUCCESS(ntStatus)) {
		// not supported in the first edition of win98
		PPORTEVENTS pPortEvents = 0;
		ntStatus = pPort->QueryInterface(IID_IPortEvents, (PVOID *)&pPortEvents);
		if (!NT_SUCCESS(ntStatus)) {
			DBGPRINT(("ERROR: This driver doesn't work under Win98!"));
			ntStatus = STATUS_UNSUCCESSFUL;
		}
		else
		{
			pPortEvents->Release();
		}
		pPort->Release ();
	} else {
		return ntStatus;
	}

	// resource validation
	PRESOURCELIST UartResourceList = NULL;
	ntStatus = ProcessResources(ResourceList, &UartResourceList);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("ProcessResources() failed"));
		return ntStatus;
	}

	PCMIADAPTER	pCMIAdapter	= NULL;
	PUNKNOWN	pUnknownCommon = NULL;

	// create the CMIAdapter object
	ntStatus = NewCMIAdapter(&pUnknownCommon, IID_ICMIAdapter, NULL, NonPagedPool);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("NewCMIAdapter() failed"));
		return ntStatus;
	}

	ntStatus = pUnknownCommon->QueryInterface(IID_ICMIAdapter, (PVOID *)&pCMIAdapter);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("QueryInterface() for ICMIAdapter failed"));
		return ntStatus;
	}
	ntStatus = pCMIAdapter->init(ResourceList, DeviceObject);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("CMIAdapter->init() failed"));
		return ntStatus;
	}

	ntStatus = PcRegisterAdapterPowerManagement((PUNKNOWN)pCMIAdapter, DeviceObject);

	pUnknownCommon->Release();

	PUNKNOWN unknownWave = NULL;
	PUNKNOWN unknownTopology = NULL;

	// install the topology miniport.
	ntStatus = InstallSubdevice(DeviceObject, Irp, L"Topology", CLSID_PortTopology, CLSID_PortTopology, CreateMiniportTopologyCMI, pCMIAdapter, NULL, GUID_NULL, &unknownTopology);
	if (!NT_SUCCESS (ntStatus)) {
		DBGPRINT(("Topology miniport installation failed"));
		return ntStatus;
	}

#ifdef UART
	// install the UART miniport - execution order important
	ntStatus = STATUS_UNSUCCESSFUL;
	MPUBase = 0;
	for (int i=0;i<ResourceList->NumberOfPorts();i++) {
		if (ResourceList->FindTranslatedPort(i)->u.Port.Length == 2) {
			MPUBase = (UInt32*)ResourceList->FindTranslatedPort(i)->u.Port.Start.QuadPart;
		}
	}
	if (MPUBase != 0) {
		ntStatus = pCMIAdapter->activateMPU(MPUBase);
		if (NT_SUCCESS(ntStatus)) {
			ntStatus = InstallSubdevice(DeviceObject, Irp, L"Uart", CLSID_PortDMus, CLSID_MiniportDriverDMusUART, NULL, pCMIAdapter->getInterruptSync(), UartResourceList, IID_IPortDMus, NULL);
		}
	}
	if (!NT_SUCCESS(ntStatus)) {
		MPUBase = 0;
		pCMIAdapter->activateMPU(0);
		DBGPRINT(("UART miniport installation failed"));
	}
	if (UartResourceList) {
		UartResourceList->Release();
	}
#endif

	// install the wave miniport - the order matters here
#ifdef WAVERT
	ntStatus = InstallSubdevice(DeviceObject, Irp, L"Wave", CLSID_PortWaveRT, CLSID_PortWaveRT, CreateMiniportWaveCMI, pCMIAdapter, ResourceList, IID_IPortWaveRT, &unknownWave);
#else
	ntStatus = InstallSubdevice(DeviceObject, Irp, L"Wave", CLSID_PortWaveCyclic, CLSID_PortWaveCyclic, CreateMiniportWaveCMI, pCMIAdapter, ResourceList, IID_IPortWaveCyclic, &unknownWave);
#endif
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("Wave miniport installation failed"));
		return ntStatus;
	}

	// connect wave and topology pins
	ntStatus = PcRegisterPhysicalConnection(DeviceObject, unknownWave, PIN_WAVE_RENDER_SOURCE, unknownTopology, PIN_WAVEOUT_SOURCE);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("Cannot connect topology and wave miniport (render)!"));
		return ntStatus;
	}
	ntStatus = PcRegisterPhysicalConnection(DeviceObject, unknownTopology, PIN_WAVEIN_DEST, unknownWave, PIN_WAVE_CAPTURE_SOURCE);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("Cannot connect topology and wave miniport (capture)!"));
		return ntStatus;
	}
	if (!IoIsWdmVersionAvailable(6,0)) {
		// this shit fixes the fucking XP mixer and breaks the vista mixer, so we have to check for vista here
		ntStatus = PcRegisterPhysicalConnection(DeviceObject, unknownWave, PIN_WAVE_AC3_RENDER_SOURCE, unknownTopology, PIN_SPDIF_AC3_SOURCE);
		if (!NT_SUCCESS(ntStatus)) {
			DBGPRINT(("Cannot connect topology and wave miniport (ac3)!"));
		}
	}

	// clean up
	if (pCMIAdapter) {
		pCMIAdapter->Release();
	}
	if (unknownTopology) {
		unknownTopology->Release();
	}
	if (unknownWave) {
		unknownWave->Release();
	}

	return ntStatus;
}

extern 
"C"
NTSTATUS
NTAPI
AddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject)
{
#if 0
    //PAGED_CODE();
    DBGPRINT(("AddDevice()"));
#endif

    return PcAddAdapterDevice(DriverObject, PhysicalDeviceObject, (PCPFNSTARTDEVICE)StartDevice, MAX_MINIPORTS, 0);
}

bool CopyResourceDescriptor(PIO_RESOURCE_DESCRIPTOR pInResDescriptor, PIO_RESOURCE_DESCRIPTOR pOutResDescriptor)
{
#if 0
	//PAGED_CODE();
	//ASSERT(pInResDescriptor);
	//ASSERT(pOutResDescriptor);
	DBGPRINT(("CopyResourceDescriptor()"));
	RtlCopyMemory(pOutResDescriptor, pInResDescriptor, sizeof(IO_RESOURCE_DESCRIPTOR));
#else
	pOutResDescriptor->Type             = pInResDescriptor->Type;
	pOutResDescriptor->ShareDisposition = pInResDescriptor->ShareDisposition;
	pOutResDescriptor->Flags            = pInResDescriptor->Flags;
	pOutResDescriptor->Option           = pInResDescriptor->Option;

	switch (pInResDescriptor->Type) {
		case CmResourceTypePort:
		case CmResourceTypePort | CmResourceTypeNonArbitrated:  // huh?
/*			// filter crap
			if ((pInResDescriptor->u.Port.Length == 0) ||
			    ( (pInResDescriptor->u.Port.MinimumAddress.HighPart == pInResDescriptor->u.Port.MaximumAddress.HighPart) && (pInResDescriptor->u.Port.MinimumAddress.LowPart == pInResDescriptor->u.Port.MaximumAddress.LowPart) ) ) {
				return FALSE;
			}
*/			pOutResDescriptor->u.Port.MinimumAddress = pInResDescriptor->u.Port.MinimumAddress;
			pOutResDescriptor->u.Port.MaximumAddress = pInResDescriptor->u.Port.MaximumAddress;
			pOutResDescriptor->u.Port.Length         = pInResDescriptor->u.Port.Length;
			pOutResDescriptor->u.Port.Alignment	     = pInResDescriptor->u.Port.Alignment;
#if 0
			DBGPRINT((" Port: min %08x.%08x max %08x.%08x, Length: %x, Option: %x", pOutResDescriptor->u.Port.MinimumAddress.HighPart, pOutResDescriptor->u.Port.MinimumAddress.LowPart,
			                                                            pOutResDescriptor->u.Port.MaximumAddress.HighPart, pOutResDescriptor->u.Port.MaximumAddress.LowPart,
			                                                            pOutResDescriptor->u.Port.Length, pOutResDescriptor->Option));
#endif
			break;
		case CmResourceTypeInterrupt:
			pOutResDescriptor->u.Interrupt.MinimumVector = pInResDescriptor->u.Interrupt.MinimumVector;
			pOutResDescriptor->u.Interrupt.MaximumVector = pInResDescriptor->u.Interrupt.MaximumVector;
#if 0
			DBGPRINT((" IRQ:  min %x max %x, Option: %d", pOutResDescriptor->u.Interrupt.MinimumVector, pOutResDescriptor->u.Interrupt.MaximumVector, pOutResDescriptor->Option));
#endif
			break;
		default:
			return FALSE;
	}
	return TRUE;
#endif
}

extern
"C"
NTSTATUS
NTAPI
AdapterDispatchPnp(
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp)
{
	NTSTATUS                       ntStatus = STATUS_SUCCESS;
	ULONG                          resourceListSize;
	PIO_RESOURCE_REQUIREMENTS_LIST resourceList, list;
	PIO_RESOURCE_DESCRIPTOR        descriptor;
	PIO_STACK_LOCATION             pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

	////PAGED_CODE();
	////ASSERT(pDeviceObject);
	////ASSERT(pIrp);
	DBGPRINT(("AdapterDispatchPnp()"));

	if (pIrpStack->MinorFunction == IRP_MN_FILTER_RESOURCE_REQUIREMENTS) {
		DBGPRINT(("[AdapterDispatchPnp] - IRP_MN_FILTER_RESOURCE_REQUIREMENTS"));

		list = (PIO_RESOURCE_REQUIREMENTS_LIST)pIrp->IoStatus.Information;

		// IO_RESOURCE_REQUIREMENTS_LIST has 1 IO_RESOURCE_LIST, IO_RESOURCE_LIST has 1 IO_RESOURCE_DESCRIPTOR and we want 2 more
		resourceListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) + sizeof(IO_RESOURCE_DESCRIPTOR)*(list->List[0].Count+2) ;
		resourceList = (PIO_RESOURCE_REQUIREMENTS_LIST)ExAllocatePoolWithTag(PagedPool, resourceListSize, 'LRDV');

		if (!resourceList) {
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			return ntStatus;
		}


		RtlZeroMemory(resourceList, resourceListSize);

		// initialize the list header
		resourceList->AlternativeLists = 1; // number of IO_RESOURCE_LISTs
		resourceList->ListSize = resourceListSize;

		resourceList->List[0].Version  = 1;
		resourceList->List[0].Revision = 1;
		resourceList->List[0].Count    = 0;

		// copy the resources which have already been assigned
		for (int i=0;i<list->List[0].Count;i++) {
			if (CopyResourceDescriptor(&list->List[0].Descriptors[i], &resourceList->List[0].Descriptors[resourceList->List[0].Count])) {
				resourceList->List[0].Count++;
			}
		}
		ExFreePool(list);

		// an additional port for mpu401
		resourceList->List[0].Count++;
		descriptor = &resourceList->List[0].Descriptors[resourceList->List[0].Count-1];
		descriptor->Option                = IO_RESOURCE_PREFERRED;
		descriptor->Type                  = CmResourceTypePort;
		descriptor->ShareDisposition      = CmResourceShareDeviceExclusive;
		descriptor->Flags                 = CM_RESOURCE_PORT_IO;
		descriptor->u.Port.MinimumAddress.LowPart  = 0x300;
		descriptor->u.Port.MinimumAddress.HighPart = 0;
		descriptor->u.Port.MaximumAddress.LowPart  = 0x330;
		descriptor->u.Port.MaximumAddress.HighPart = 0;
		descriptor->u.Port.Length         = 2;
		descriptor->u.Port.Alignment      = 0x10;

		// mpu401 port should be optional. yes, this is severely braindamaged.
		resourceList->List[0].Count++;
		descriptor = &resourceList->List[0].Descriptors[resourceList->List[0].Count-1];
		descriptor->Option                = IO_RESOURCE_ALTERNATIVE;
		descriptor->Type                  = CmResourceTypePort;
		descriptor->ShareDisposition      = CmResourceShareDeviceExclusive;
		descriptor->Flags                 = CM_RESOURCE_PORT_IO;
		descriptor->u.Port.MinimumAddress.LowPart  = 0x0;
		descriptor->u.Port.MinimumAddress.HighPart = 0;
		descriptor->u.Port.MaximumAddress.LowPart  = 0xFFFF;
		descriptor->u.Port.MaximumAddress.HighPart = 0;
		descriptor->u.Port.Length         = 1;
		descriptor->u.Port.Alignment      = 0x10;

//		DBGPRINT(("number of resource list descriptors: %d", resourceList->List[0].Count));

		pIrp->IoStatus.Information = (ULONG_PTR)resourceList;

		// set the return status
		pIrp->IoStatus.Status = ntStatus;
	}

	// Pass the IRPs on to PortCls
	ntStatus = PcDispatchIrp(pDeviceObject, pIrp);

	return ntStatus;
}

extern 
"C"
NTSTATUS
NTAPI
DriverEntry(
    PDRIVER_OBJECT DriverObject,
     PUNICODE_STRING RegistryPathName)
{
    NTSTATUS ntStatus;

    DBGPRINT(("DriverEntry()"));


    //bind the adapter driver to the portclass driver
    ntStatus = PcInitializeAdapterDriver(DriverObject, RegistryPathName, AddDevice);


#ifdef UART
    if(NT_SUCCESS(ntStatus)) {
    DriverObject->MajorFunction[IRP_MJ_PNP] = AdapterDispatchPnp;
    }
#endif
#ifdef WAVERT
    if (!IoIsWdmVersionAvailable(6,0)) {
    ntStatus = STATUS_UNSUCCESSFUL;
    }
#endif

    return ntStatus;
}

#pragma code_seg()
int __cdecl _purecall (void)
{
	return 0;
}
