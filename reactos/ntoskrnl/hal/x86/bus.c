/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/bus.c
 * PURPOSE:         Bus functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *
 *
 * NOTE: The bus handler code is under contruction!
 *       It does NOT work yet!
 *
 * TODO:
 *   - Add missing default bus handler functions
 *   - Change ntoskrnl's initialization sequence
 *     (non-paged pool and spin locks must be available before HalInitSystem()
 *      is called in system initialization phase 0)
 *   - Add bus handler functions for all busses
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/hal.h>

#define NDEBUG
#include <internal/debug.h>


/* TYPE DEFINITIONS *********************************************************/

struct _BUS_HANDLER;

typedef ULONG (STDCALL *pGetInterruptVector) (
	IN struct _BUS_HANDLER *BusHandler,
	IN ULONG BusInterruptLevel,
	IN ULONG BusInterruptVector,
	OUT PKIRQL Irql,
	OUT PKAFFINITY Affinity
	);

typedef ULONG (STDCALL *pTranslateBusAddress) (
	IN struct _BUS_HANDLER *BusHandler,
	IN PHYSICAL_ADDRESS BusAddress,
	IN ULONG Length,
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

	PVOID GetBusData;
	PVOID SetBusData;
	PVOID AssignSlotResources;
	pGetInterruptVector	GetInterruptVector;
	pTranslateBusAddress	TranslateBusAddress;

} BUS_HANDLER, *PBUS_HANDLER;


/* GLOBAL VARIABLES **********************************************************/

//KSPIN_LOCK HalpBusHandlerSpinLock = {0,};
LIST_ENTRY HalpBusHandlerList;


/* FUNCTIONS *****************************************************************/

static
ULONG
HalpNoBusData (
	PBUS_HANDLER	BusHandler,
	ULONG		SlotNumber,
	PVOID		Buffer,
	ULONG		Offset,
	ULONG		Length
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

//	BusHandler = ExAllocatePool (NonPagedPool, sizeof(BUS_HANDLER));
	if (BusHandler == NULL)
		return NULL;

	RtlZeroMemory (BusHandler, sizeof(BUS_HANDLER));

	InsertTailList (&HalpBusHandlerList,
	                &BusHandler->Entry);

	BusHandler->InterfaceType = InterfaceType;
	BusHandler->BusDataType = BusDataType;
	BusHandler->BusNumber = BusNumber;

	/* initialize handler function table */
	BusHandler->GetBusData = HalpNoBusData;
	BusHandler->SetBusData = HalpNoBusData;
//	BusHandler->AdjustResourceList = HalpNoAdjustResourceList;
//	BusHandler->AssignSlotResources = HalpNoAssignSlotResources;
//	BusHandler->GetInterruptVector = HalpNoGetInterruptVector;
//	BusHandler->TranslateBusAddress = HalpNoTranslateBusAddress;


	return BusHandler;
}



VOID
HalpInitBusHandlers (VOID)
{
	PBUS_HANDLER BusHandler;

	/* general preparations */
//	KeInitializeSpinLock (&HalpBusHandlerSpinLock);
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
//	BusHandler->GetInterruptVector =
//		(pGetInterruptVector)HalpGetSystemInterruptVector;
//	BusHandler->TranslateBusAddress =
//		(pTranslateBusAddress)HalpTranslateSystemBusAddress;

	/* cmos bus handler */
	BusHandler = HalpAllocateBusHandler (InterfaceTypeUndefined,
	                                     Cmos,
	                                     0);

	/* pci bus handler */
	BusHandler = HalpAllocateBusHandler (PCIBus,
	                                     PCIConfiguration,
	                                     0);

	/* isa bus handler */
	BusHandler = HalpAllocateBusHandler (Isa,
	                                     ConfigurationSpaceUndefined,
	                                     0);

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
//	KIRQL OldIrql;

//	KeAcquireSpinLock (&HalpBusHandlerSpinLock, &OldIrql);

	CurrentEntry = HalpBusHandlerList.Flink;
	while (CurrentEntry != &HalpBusHandlerList)
	{
		BusHandler = (PBUS_HANDLER)CurrentEntry;
		if (BusHandler->InterfaceType == InterfaceType &&
		    BusHandler->BusNumber == BusNumber)
		{
//			KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);
			return BusHandler;
		}
		CurrentEntry = CurrentEntry->Flink;
	}
//	KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);

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
//	KIRQL OldIrql;

//	KeAcquireSpinLock (&HalpBusHandlerSpinLock, &OldIrql);

	CurrentEntry = HalpBusHandlerList.Flink;
	while (CurrentEntry != &HalpBusHandlerList)
	{
		BusHandler = (PBUS_HANDLER)CurrentEntry;
		if (BusHandler->BusDataType == BusDataType &&
		    BusHandler->BusNumber == BusNumber)
		{
//			KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);
			return BusHandler;
		}
		CurrentEntry = CurrentEntry->Flink;
	}
//	KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);

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
//	KIRQL OldIrql;

//	KeAcquireSpinLock (&HalpBusHandlerSpinLock, &OldIrql);

	CurrentEntry = HalpBusHandlerList.Flink;
	while (CurrentEntry != &HalpBusHandlerList)
	{
		BusHandler = (PBUS_HANDLER)CurrentEntry;
		if (BusHandler->InterfaceType == InterfaceType &&
		    BusHandler->BusNumber == BusNumber)
		{
			BusHandler->RefCount++;
//			KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);
			return BusHandler;
		}
		CurrentEntry = CurrentEntry->Flink;
	}
//	KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);

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
//	KIRQL OldIrql;

//	KeAcquireSpinLock (&HalpBusHandlerSpinLock, &OldIrql);

	CurrentEntry = HalpBusHandlerList.Flink;
	while (CurrentEntry != &HalpBusHandlerList)
	{
		BusHandler = (PBUS_HANDLER)CurrentEntry;
		if (BusHandler->BusDataType == BusDataType &&
		    BusHandler->BusNumber == BusNumber)
		{
			BusHandler->RefCount++;
//			KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);
			return BusHandler;
		}
		CurrentEntry = CurrentEntry->Flink;
	}
//	KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);

	return NULL;
}


VOID
FASTCALL
HaliDereferenceBusHandler (
	PBUS_HANDLER	BusHandler
	)
{
//	KIRQL OldIrql;

//	KeAcquireSpinLock (&HalpBusHandlerSpinLock, &OldIrql);

	BusHandler->RefCount--;

//	KeReleaseSpinLock (&HalpBusHandlerSpinLock, OldIrql);
}


NTSTATUS
STDCALL
HalAdjustResourceList (
	PCM_RESOURCE_LIST	Resources
	)
{
#if 0
	PBUS_HANDLER BusHandler;
	NTSTATUS Status;

	BusHandler = HaliReferenceHandlerForBus (Resources->List[0].InterfaceType,
	                                         Resources->List[0].BusNumber);
	if (BusHandler == NULL)
		return STATUS_SUCCESS;

	Status = BusHandler->AdjustResourceList (BusHandler,
	                                         BusHandler,
	                                         Resources);

	HaliDereferenceBusHandler (BusHandler);

	return Status;
#endif
   UNIMPLEMENTED;
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
#if 0
	PBUS_HANDLER BusHandler;
	NTSTATUS Status;

	BusHandler = HaliReferenceHandlerForBus (InterfaceType,
	                                         BusNumber);
	if (BusHandler == NULL)
		return STATUS_NOT_FOUND;

	Status = BusHandler->AssignSlotResources (BusHandler,
	                                          BusHandler,
	                                          RegistryPath,
	                                          DriverClassName,
	                                          DriverObject,
	                                          DeviceObject,
	                                          SlotNumber,
	                                          AllocatedResources);

	HaliDereferenceBusHandler (BusHandler);

	return Status;
#endif
   UNIMPLEMENTED;
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
#if 0
	PBUS_HANDLER BusHandler;
	ULONG Result;

	BusHandler = HaliReferenceHandlerForConfigSpace (BusDataType,
	                                                 BusNumber);
	if (BusHandler == NULL)
		return 0;

	Result = BusHandler->GetBusData (BusHandler,
	                                 SlotNumber,
	                                 Buffer,
	                                 Offset,
	                                 Length);

	HaliDereferenceBusHandler (BusHandler);

	return Result;
#endif
   UNIMPLEMENTED;
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
#if 0
	PBUS_HANDLER BusHandler;
	ULONG Result;

	BusHandler = HaliReferenceHandlerForBus (InterfaceType,
	                                         BusNumber);
	if (BusHandler == NULL)
		return 0;

	Result = BusHandler->GetInterruptVector (BusHandler,
	                                         BusInterruptLevel,
	                                         BusInterruptVector,
	                                         Irql,
	                                         Affinity);

	HaliDereferenceBusHandler (BusHandler);

	return Result;
#endif
	return (HalpGetSystemInterruptVector (NULL,
	                                      BusInterruptLevel,
	                                      BusInterruptVector,
	                                      Irql,
	                                      Affinity));
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
#if 0
	PBUS_HANDLER BusHandler;
	ULONG Result;

	BusHandler = HaliReferenceHandlerForConfigSpace (BusDataType,
	                                                 BusNumber);
	if (BusHandler == NULL)
		return 0;

	Result = BusHandler->SetBusData (BusHandler,
	                                 BusHandler,
	                                 SlotNumber,
	                                 Buffer,
	                                 Offset,
	                                 Length);

	HaliDereferenceBusHandler (BusHandler);

	return Result;
#endif
   UNIMPLEMENTED;
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
#if 0
	PBUS_HANDLER BusHandler;
	BOOLEAN Result;

	BusHandler = HaliReferenceHandlerForBus (InterfaceType,
	                                         BusNumber);
	if (BusHandler == NULL)
		return FALSE;

	Result = BusHandler->TranslateBusAddress (BusHandler,
	                                          BusHandler,
	                                          BusAddress,
	                                          AddressSpace,
	                                          TranslatedAddress);

	HaliDereferenceBusHandler (BusHandler);

	return Result;
#endif
   UNIMPLEMENTED;
}

/* EOF */
