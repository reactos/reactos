/* $Id: bus.c,v 1.5 2000/04/09 15:58:13 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/bus.c
 * PURPOSE:         Bus functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *
 *
 * TODO:
 *   - Add bus handler functions for all busses
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/hal.h>

#define NDEBUG
#include <internal/debug.h>


/* TYPE DEFINITIONS *********************************************************/


struct _BUS_HANDLER;

typedef NTSTATUS (STDCALL *pAdjustResourceList) (
	IN struct _BUS_HANDLER *BusHandler,
	IN ULONG BusNumber,
	IN OUT PCM_RESOURCE_LIST Resources
	);

typedef NTSTATUS (STDCALL *pAssignSlotResources) (
	IN struct _BUS_HANDLER *BusHandler,
	IN ULONG BusNumber,
	IN PUNICODE_STRING RegistryPath,
	IN PUNICODE_STRING DriverClassName,
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT DeviceObject,
	IN ULONG SlotNumber,
	IN OUT PCM_RESOURCE_LIST *AllocatedResources
	);

typedef ULONG (STDCALL *pGetSetBusData) (
	IN struct _BUS_HANDLER *BusHandler,
	IN ULONG BusNumber,
	IN ULONG SlotNumber,
	OUT PVOID Buffer,
	IN ULONG Offset,
	IN ULONG Length
	);

typedef ULONG (STDCALL *pGetInterruptVector) (
	IN struct _BUS_HANDLER *BusHandler,
	IN ULONG BusNumber,
	IN ULONG BusInterruptLevel,
	IN ULONG BusInterruptVector,
	OUT PKIRQL Irql,
	OUT PKAFFINITY Affinity
	);

typedef ULONG (STDCALL *pTranslateBusAddress) (
	IN struct _BUS_HANDLER *BusHandler,
	IN ULONG BusNumber,
	IN PHYSICAL_ADDRESS BusAddress,
	IN OUT PULONG AddressSpace,
	OUT PPHYSICAL_ADDRESS TranslatedAddress
	);

typedef struct _BUS_HANDLER
{
	LIST_ENTRY Entry;
	INTERFACE_TYPE InterfaceType;
	BUS_DATA_TYPE BusDataType;
	ULONG BusNumber;
	ULONG RefCount;

	pGetSetBusData		GetBusData;
	pGetSetBusData		SetBusData;
	pAdjustResourceList	AdjustResourceList;
	pAssignSlotResources	AssignSlotResources;
	pGetInterruptVector	GetInterruptVector;
	pTranslateBusAddress	TranslateBusAddress;

} BUS_HANDLER, *PBUS_HANDLER;


/* sysbus.h */
ULONG
STDCALL
HalpGetSystemInterruptVector (
	PVOID BusHandler,
	ULONG BusNumber,
	ULONG BusInterruptLevel,
	ULONG BusInterruptVector,
	PKIRQL Irql,
	PKAFFINITY Affinity
	);

BOOLEAN
STDCALL
HalpTranslateSystemBusAddress (
	PBUS_HANDLER		BusHandler,
	ULONG			BusNumber,
	PHYSICAL_ADDRESS	BusAddress,
	PULONG			AddressSpace,
	PPHYSICAL_ADDRESS	TranslatedAddress
	);

/* isa.c */
BOOLEAN
STDCALL
HalpTranslateIsaBusAddress (
	PBUS_HANDLER		BusHandler,
	ULONG			BusNumber,
	PHYSICAL_ADDRESS	BusAddress,
	PULONG			AddressSpace,
	PPHYSICAL_ADDRESS	TranslatedAddress
	);


/* GLOBAL VARIABLES **********************************************************/

KSPIN_LOCK HalpBusHandlerSpinLock = {0,};
LIST_ENTRY HalpBusHandlerList;


/* FUNCTIONS *****************************************************************/

static
NTSTATUS
STDCALL
HalpNoAdjustResourceList (
	PBUS_HANDLER		BusHandler,
	ULONG			BusNumber,
	PCM_RESOURCE_LIST	Resources
	)
{
	return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
STDCALL
HalpNoAssignSlotResources (
	PBUS_HANDLER		BusHandler,
	ULONG			BusNumber,
	PUNICODE_STRING		RegistryPath,
	PUNICODE_STRING		DriverClassName,
	PDRIVER_OBJECT		DriverObject,
	PDEVICE_OBJECT		DeviceObject,
	ULONG			SlotNumber,
	PCM_RESOURCE_LIST	*AllocatedResources
	)
{
	return STATUS_NOT_SUPPORTED;
}

static
ULONG
STDCALL
HalpNoBusData (
	PBUS_HANDLER	BusHandler,
	ULONG		BusNumber,
	ULONG		SlotNumber,
	PVOID		Buffer,
	ULONG		Offset,
	ULONG		Length
	)
{
	return 0;
}

static
ULONG
STDCALL
HalpNoGetInterruptVector (
	PBUS_HANDLER	BusHandler,
	ULONG		BusNumber,
	ULONG		BusInterruptLevel,
	ULONG		BusInterruptVector,
	PKIRQL		Irql,
	PKAFFINITY	Affinity
	)
{
	return 0;
}

static
ULONG
STDCALL
HalpNoTranslateBusAddress (
	PBUS_HANDLER		BusHandler,
	ULONG			BusNumber,
	PHYSICAL_ADDRESS	BusAddress,
	PULONG			AddressSpace,
	PPHYSICAL_ADDRESS	TranslatedAddress
	)
{
	return 0;
}


PBUS_HANDLER
HalpAllocateBusHandler (
	INTERFACE_TYPE	InterfaceType,
	BUS_DATA_TYPE	BusDataType,
	ULONG		BusNumber
	)
{
	PBUS_HANDLER BusHandler = NULL;

	DPRINT("HalpAllocateBusHandler()\n");

	BusHandler = ExAllocatePool (NonPagedPool, sizeof(BUS_HANDLER));
	if (BusHandler == NULL)
		return NULL;

	RtlZeroMemory (BusHandler, sizeof(BUS_HANDLER));

	InsertTailList (&HalpBusHandlerList,
	                &BusHandler->Entry);

	BusHandler->InterfaceType = InterfaceType;
	BusHandler->BusDataType = BusDataType;
	BusHandler->BusNumber = BusNumber;

	/* initialize default bus handler functions */
	BusHandler->GetBusData = HalpNoBusData;
	BusHandler->SetBusData = HalpNoBusData;
	BusHandler->AdjustResourceList = HalpNoAdjustResourceList;
	BusHandler->AssignSlotResources = HalpNoAssignSlotResources;
	BusHandler->GetInterruptVector = HalpNoGetInterruptVector;
	BusHandler->TranslateBusAddress = HalpNoTranslateBusAddress;

	/* any more ?? */

	DPRINT("HalpAllocateBusHandler() done\n");

	return BusHandler;
}


VOID
HalpInitBusHandlers (VOID)
{
	PBUS_HANDLER BusHandler;

	/* general preparations */
	KeInitializeSpinLock (&HalpBusHandlerSpinLock);
	InitializeListHead (&HalpBusHandlerList);

	/* initialize hal dispatch tables */
#if 0


	HalDispatchTable->HalQueryBusSlots = HaliQueryBusSlots;
#endif

	/*
	 * add bus handlers
	 */

	/* system bus handler */
	BusHandler = HalpAllocateBusHandler (Internal,
	                                     ConfigurationSpaceUndefined,
	                                     0);
	BusHandler->GetInterruptVector =
		(pGetInterruptVector)HalpGetSystemInterruptVector;
	BusHandler->TranslateBusAddress =
		(pTranslateBusAddress)HalpTranslateSystemBusAddress;

	/* cmos bus handler */
	BusHandler = HalpAllocateBusHandler (InterfaceTypeUndefined,
	                                     Cmos,
	                                     0);
//	BusHandler->GetBusData =
//		(pGetSetBusData)HalpGetCmosData;
//	BusHandler->SetBusData =
//		(pGetSetBusData)HalpSetCmosData;

	/* isa bus handler */
	BusHandler = HalpAllocateBusHandler (Isa,
	                                     ConfigurationSpaceUndefined,
	                                     0);
	BusHandler->TranslateBusAddress =
		(pTranslateBusAddress)HalpTranslateIsaBusAddress;

#if 0
	/* pci bus handler */
	BusHandler = HalpAllocateBusHandler (PCIBus,
	                                     PCIConfiguration,
	                                     0);
//	BusHandler->GetBusData =
//		(pGetSetBusData)HalpGetPciData;
//	BusHandler->SetBusData =
//		(pGetSetBusData)HalpSetPciData;
//	BusHandler->GetInterruptVector =
//		(pGetInterruptVector)HalpGetPciInterruptVector;
//	BusHandler->AdjustResourceList =
//		(pGetSetBusData)HalpAdjustPciResourceList;
//	BusHandler->AssignSlotResources =
//		(pGetSetBusData)HalpAssignPciSlotResources;
#endif
}


PBUS_HANDLER
FASTCALL
HaliHandlerForBus (
	INTERFACE_TYPE	InterfaceType,
	ULONG		BusNumber
	)
{
	PBUS_HANDLER BusHandler;
	PLIST_ENTRY CurrentEntry;
	KIRQL OldIrql;

	KeAcquireSpinLock (&HalpBusHandlerSpinLock, &OldIrql);

	CurrentEntry = HalpBusHandlerList.Flink;
	while (CurrentEntry != &HalpBusHandlerList)
	{
		BusHandler = (PBUS_HANDLER)CurrentEntry;
		if (BusHandler->InterfaceType == InterfaceType &&
		    BusHandler->BusNumber == BusNumber)
		{
			KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);
			return BusHandler;
		}
		CurrentEntry = CurrentEntry->Flink;
	}
	KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);

	return NULL;
}


PBUS_HANDLER
FASTCALL
HaliHandlerForConfigSpace (
	BUS_DATA_TYPE	BusDataType,
	ULONG		BusNumber
	)
{
	PBUS_HANDLER BusHandler;
	PLIST_ENTRY CurrentEntry;
	KIRQL OldIrql;

	KeAcquireSpinLock (&HalpBusHandlerSpinLock, &OldIrql);

	CurrentEntry = HalpBusHandlerList.Flink;
	while (CurrentEntry != &HalpBusHandlerList)
	{
		BusHandler = (PBUS_HANDLER)CurrentEntry;
		if (BusHandler->BusDataType == BusDataType &&
		    BusHandler->BusNumber == BusNumber)
		{
			KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);
			return BusHandler;
		}
		CurrentEntry = CurrentEntry->Flink;
	}
	KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);

	return NULL;
}


PBUS_HANDLER
FASTCALL
HaliReferenceHandlerForBus (
	INTERFACE_TYPE	InterfaceType,
	ULONG		BusNumber
	)
{
	PBUS_HANDLER BusHandler;
	PLIST_ENTRY CurrentEntry;
	KIRQL OldIrql;

	KeAcquireSpinLock (&HalpBusHandlerSpinLock, &OldIrql);

	CurrentEntry = HalpBusHandlerList.Flink;
	while (CurrentEntry != &HalpBusHandlerList)
	{
		BusHandler = (PBUS_HANDLER)CurrentEntry;
		if (BusHandler->InterfaceType == InterfaceType &&
		    BusHandler->BusNumber == BusNumber)
		{
			BusHandler->RefCount++;
			KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);
			return BusHandler;
		}
		CurrentEntry = CurrentEntry->Flink;
	}
	KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);

	return NULL;
}


PBUS_HANDLER
FASTCALL
HaliReferenceHandlerForConfigSpace (
	BUS_DATA_TYPE	BusDataType,
	ULONG		BusNumber
	)
{
	PBUS_HANDLER BusHandler;
	PLIST_ENTRY CurrentEntry;
	KIRQL OldIrql;

	KeAcquireSpinLock (&HalpBusHandlerSpinLock, &OldIrql);

	CurrentEntry = HalpBusHandlerList.Flink;
	while (CurrentEntry != &HalpBusHandlerList)
	{
		BusHandler = (PBUS_HANDLER)CurrentEntry;
		if (BusHandler->BusDataType == BusDataType &&
		    BusHandler->BusNumber == BusNumber)
		{
			BusHandler->RefCount++;
			KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);
			return BusHandler;
		}
		CurrentEntry = CurrentEntry->Flink;
	}
	KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);

	return NULL;
}


VOID
FASTCALL
HaliDereferenceBusHandler (
	PBUS_HANDLER	BusHandler
	)
{
	KIRQL OldIrql;

	KeAcquireSpinLock (&HalpBusHandlerSpinLock, &OldIrql);

	BusHandler->RefCount--;

	KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);
}


NTSTATUS
STDCALL
HalAdjustResourceList (
	PCM_RESOURCE_LIST	Resources
	)
{
	PBUS_HANDLER BusHandler;
	NTSTATUS Status;

	BusHandler = HaliReferenceHandlerForBus (Resources->List[0].InterfaceType,
	                                         Resources->List[0].BusNumber);
	if (BusHandler == NULL)
		return STATUS_SUCCESS;

	Status = BusHandler->AdjustResourceList (BusHandler,
	                                         Resources->List[0].BusNumber,
	                                         Resources);

	HaliDereferenceBusHandler (BusHandler);

	return Status;
}


NTSTATUS
STDCALL
HalAssignSlotResources (
	PUNICODE_STRING		RegistryPath,
	PUNICODE_STRING		DriverClassName,
	PDRIVER_OBJECT		DriverObject,
	PDEVICE_OBJECT		DeviceObject,
	INTERFACE_TYPE		BusType,
	ULONG			BusNumber,
	ULONG			SlotNumber,
	PCM_RESOURCE_LIST	*AllocatedResources
	)
{
	PBUS_HANDLER BusHandler;
	NTSTATUS Status;

	BusHandler = HaliReferenceHandlerForBus (BusType,
	                                         BusNumber);
	if (BusHandler == NULL)
		return STATUS_NOT_FOUND;

	Status = BusHandler->AssignSlotResources (BusHandler,
	                                          BusNumber,
	                                          RegistryPath,
	                                          DriverClassName,
	                                          DriverObject,
	                                          DeviceObject,
	                                          SlotNumber,
	                                          AllocatedResources);

	HaliDereferenceBusHandler (BusHandler);

	return Status;
}


ULONG
STDCALL
HalGetBusData (
	BUS_DATA_TYPE	BusDataType,
	ULONG		BusNumber,
	ULONG		SlotNumber,
	PVOID		Buffer,
	ULONG		Length
	)
{
	return (HalGetBusDataByOffset (BusDataType,
	                               BusNumber,
	                               SlotNumber,
	                               Buffer,
	                               0,
	                               Length));
}


ULONG
STDCALL
HalGetBusDataByOffset (
	BUS_DATA_TYPE	BusDataType,
	ULONG		BusNumber,
	ULONG		SlotNumber,
	PVOID		Buffer,
	ULONG		Offset,
	ULONG		Length
	)
{
	PBUS_HANDLER BusHandler;
	ULONG Result;

	BusHandler = HaliReferenceHandlerForConfigSpace (BusDataType,
	                                                 BusNumber);
	if (BusHandler == NULL)
		return 0;

	Result = BusHandler->GetBusData (BusHandler,
	                                 BusNumber,
	                                 SlotNumber,
	                                 Buffer,
	                                 Offset,
	                                 Length);

	HaliDereferenceBusHandler (BusHandler);

	return Result;
}


ULONG
STDCALL
HalGetInterruptVector (
	INTERFACE_TYPE	InterfaceType,
	ULONG		BusNumber,
	ULONG		BusInterruptLevel,
	ULONG		BusInterruptVector,
	PKIRQL		Irql,
	PKAFFINITY	Affinity
	)
{
	PBUS_HANDLER BusHandler;
	ULONG Result;

	BusHandler = HaliReferenceHandlerForBus (InterfaceType,
	                                         BusNumber);
	if (BusHandler == NULL)
		return 0;

	Result = BusHandler->GetInterruptVector (BusHandler,
	                                         BusNumber,
	                                         BusInterruptLevel,
	                                         BusInterruptVector,
	                                         Irql,
	                                         Affinity);

	HaliDereferenceBusHandler (BusHandler);

	return Result;
}


ULONG
STDCALL
HalSetBusData (
	BUS_DATA_TYPE	BusDataType,
	ULONG		BusNumber,
	ULONG		SlotNumber,
	PVOID		Buffer,
	ULONG		Length
	)
{
	return (HalSetBusDataByOffset (BusDataType,
	                               BusNumber,
	                               SlotNumber,
	                               Buffer,
	                               0,
	                               Length));
}


ULONG
STDCALL
HalSetBusDataByOffset (
	BUS_DATA_TYPE	BusDataType,
	ULONG		BusNumber,
	ULONG		SlotNumber,
	PVOID		Buffer,
	ULONG		Offset,
	ULONG		Length
	)
{
	PBUS_HANDLER BusHandler;
	ULONG Result;

	BusHandler = HaliReferenceHandlerForConfigSpace (BusDataType,
	                                                 BusNumber);
	if (BusHandler == NULL)
		return 0;

	Result = BusHandler->SetBusData (BusHandler,
	                                 BusNumber,
	                                 SlotNumber,
	                                 Buffer,
	                                 Offset,
	                                 Length);

	HaliDereferenceBusHandler (BusHandler);

	return Result;
}


BOOLEAN
STDCALL
HalTranslateBusAddress (
	INTERFACE_TYPE		InterfaceType,
	ULONG			BusNumber,
	PHYSICAL_ADDRESS	BusAddress,
	PULONG			AddressSpace,
	PPHYSICAL_ADDRESS	TranslatedAddress
	)
{
	PBUS_HANDLER BusHandler;
	BOOLEAN Result;

	BusHandler = HaliReferenceHandlerForBus (InterfaceType,
	                                         BusNumber);
	if (BusHandler == NULL)
		return FALSE;

	Result = BusHandler->TranslateBusAddress (BusHandler,
	                                          BusNumber,
	                                          BusAddress,
	                                          AddressSpace,
	                                          TranslatedAddress);

	HaliDereferenceBusHandler (BusHandler);

	return Result;
}

/* EOF */
