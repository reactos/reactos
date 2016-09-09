/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/legacy/bus/bushndlr.c
 * PURPOSE:         Generic HAL Bus Handler Support
 * PROGRAMMERS:     Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KSPIN_LOCK HalpBusDatabaseSpinLock;
KEVENT HalpBusDatabaseEvent;
LIST_ENTRY HalpAllBusHandlers;
PARRAY HalpBusTable;
PARRAY HalpConfigTable;

/* PRIVATE FUNCTIONS **********************************************************/

PARRAY
NTAPI
HalpAllocateArray(IN ULONG ArraySize)
{
    PARRAY Array;
    ULONG Size;
    
    /* Compute array size */
    if (ArraySize == MAXULONG) ArraySize = 0;
    Size = ArraySize * sizeof(PARRAY) + sizeof(ARRAY);
    
    /* Allocate the array */
    Array = ExAllocatePoolWithTag(NonPagedPool,
                                  Size,
                                  TAG_BUS_HANDLER);
    if (!Array) KeBugCheckEx(HAL_MEMORY_ALLOCATION, Size, 0, (ULONG_PTR)__FILE__, __LINE__);
    
    /* Initialize it */
    Array->ArraySize = ArraySize;
    RtlZeroMemory(Array->Element, sizeof(PVOID) * (ArraySize + 1));
    return Array;
}

VOID
NTAPI
HalpGrowArray(IN PARRAY *CurrentArray,
              IN PARRAY *NewArray)
{
    PVOID Tmp;
    
    /* Check if the current array doesn't exist yet, or if it's smaller than the new one */
    if (!(*CurrentArray) || ((*NewArray)->ArraySize > (*CurrentArray)->ArraySize))
    {
        /* Does it exist (and can it fit?) */
        if (*CurrentArray)
        {
            /* Copy the current array into the new one */
            RtlCopyMemory(&(*NewArray)->Element,
                          &(*CurrentArray)->Element,
                          sizeof(PVOID) * ((*CurrentArray)->ArraySize + 1));
        }
        
        /* Swap the pointers (XOR swap would be more l33t) */
        Tmp = *CurrentArray;
        *CurrentArray = *NewArray;
        *NewArray = Tmp;
    }
}

PBUS_HANDLER
FASTCALL
HalpLookupHandler(IN PARRAY Array,
                  IN ULONG Type,
                  IN ULONG Number,
                  IN BOOLEAN AddReference)
{
    PHAL_BUS_HANDLER Bus;
    PBUS_HANDLER Handler = NULL;
    
    /* Make sure the entry exists */
    if (Array->ArraySize >= Type)
    {
        /* Retrieve it */
        Array = Array->Element[Type];
        
        /* Make sure the entry array exists */
        if ((Array) && (Array->ArraySize >= Number))
        {
            /* Retrieve the bus and its handler */
            Bus = Array->Element[Number];
            Handler = &Bus->Handler;
            
            /* Reference the handler if needed */
            if (AddReference) Bus->ReferenceCount++;
        }
    }
    
    /* Return the handler */
    return Handler;
}

ULONG
NTAPI
HalpNoBusData(IN PBUS_HANDLER BusHandler,
              IN PBUS_HANDLER RootHandler,
              IN ULONG SlotNumber,
              IN PVOID Buffer,
              IN ULONG Offset,
              IN ULONG Length)
{
    /* Not implemented */
    DPRINT1("STUB GetSetBusData\n");
    return 0;
}
              
NTSTATUS
NTAPI
HalpNoAdjustResourceList(IN PBUS_HANDLER BusHandler,
                         IN PBUS_HANDLER RootHandler,
                         IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *pResourceList)
{
    DPRINT1("STUB Adjustment\n");
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
HalpNoAssignSlotResources(IN PBUS_HANDLER BusHandler,
                          IN PBUS_HANDLER RootHandler,
                          IN PUNICODE_STRING RegistryPath,
                          IN PUNICODE_STRING DriverClassName OPTIONAL,
                          IN PDRIVER_OBJECT DriverObject,
                          IN PDEVICE_OBJECT DeviceObject OPTIONAL,
                          IN ULONG SlotNumber,
                          IN OUT PCM_RESOURCE_LIST *AllocatedResources)
{
    DPRINT1("STUB Assignment\n");
    return STATUS_NOT_SUPPORTED;
}

VOID
FASTCALL
HaliReferenceBusHandler(IN PBUS_HANDLER Handler)
{
    PHAL_BUS_HANDLER Bus;
    
    /* Find and reference the bus handler */
    Bus = CONTAINING_RECORD(Handler, HAL_BUS_HANDLER, Handler);
    Bus->ReferenceCount++;
}

VOID
FASTCALL
HaliDereferenceBusHandler(IN PBUS_HANDLER Handler)
{
    PHAL_BUS_HANDLER Bus;
    
    /* Find and dereference the bus handler */
    Bus = CONTAINING_RECORD(Handler, HAL_BUS_HANDLER, Handler);
    Bus->ReferenceCount--;
    ASSERT(Bus->ReferenceCount != 0);
}

PBUS_HANDLER
FASTCALL
HaliHandlerForBus(IN INTERFACE_TYPE InterfaceType,
                  IN ULONG BusNumber)
{
    /* Lookup the interface in the bus table */
    return HalpLookupHandler(HalpBusTable, InterfaceType, BusNumber, FALSE);
}

PBUS_HANDLER
FASTCALL
HaliHandlerForConfigSpace(IN BUS_DATA_TYPE ConfigType,
                          IN ULONG BusNumber)
{
    /* Lookup the configuration in the configuration table */
    return HalpLookupHandler(HalpConfigTable, ConfigType, BusNumber, FALSE);
}

PBUS_HANDLER
FASTCALL
HaliReferenceHandlerForBus(IN INTERFACE_TYPE InterfaceType,
                           IN ULONG BusNumber)
{
    /* Lookup the interface in the bus table, and reference the handler */
    return HalpLookupHandler(HalpBusTable, InterfaceType, BusNumber, TRUE);
}

PBUS_HANDLER
FASTCALL
HaliReferenceHandlerForConfigSpace(IN BUS_DATA_TYPE ConfigType,
                                   IN ULONG BusNumber)
{
    /* Lookup the configuration in the configuration table and add a reference */
    return HalpLookupHandler(HalpConfigTable, ConfigType, BusNumber, TRUE);
}

PBUS_HANDLER
NTAPI
HalpContextToBusHandler(IN ULONG_PTR ContextValue)
{
    PLIST_ENTRY NextEntry;
    PHAL_BUS_HANDLER BusHandler, ThisHandler;
    
    /* Start lookup */
    NextEntry = HalpAllBusHandlers.Flink;
    ThisHandler = CONTAINING_RECORD(NextEntry, HAL_BUS_HANDLER, AllHandlers);
    if (ContextValue)
    {
        /* If the list is empty, quit */
        if (IsListEmpty(&HalpAllBusHandlers)) return NULL;

        /* Otherwise, scan the list */
        BusHandler = CONTAINING_RECORD(ContextValue, HAL_BUS_HANDLER, Handler);
        do
        {
            /* Check if we've reached the right one */
            ThisHandler = CONTAINING_RECORD(NextEntry, HAL_BUS_HANDLER, AllHandlers);
            if (ThisHandler == BusHandler) break;
            
            /* Try the next one */
            NextEntry = NextEntry->Flink;
        } while (NextEntry != &HalpAllBusHandlers);
    }
    
    /* If we looped back to the end, we didn't find anything */
    if (NextEntry == &HalpAllBusHandlers) return NULL;
    
    /* Otherwise return the handler */
    return &ThisHandler->Handler;
}

#ifndef _MINIHAL_
NTSTATUS
NTAPI
HaliRegisterBusHandler(IN INTERFACE_TYPE InterfaceType,
                       IN BUS_DATA_TYPE ConfigType,
                       IN ULONG BusNumber,
                       IN INTERFACE_TYPE ParentBusType,
                       IN ULONG ParentBusNumber,
                       IN ULONG ExtraData,
                       IN PINSTALL_BUS_HANDLER InstallCallback,
                       OUT PBUS_HANDLER *ReturnedBusHandler)
{
    PHAL_BUS_HANDLER Bus, OldHandler = NULL;
    PHAL_BUS_HANDLER* BusEntry;
    //PVOID CodeHandle;
    PARRAY InterfaceArray, InterfaceBusNumberArray, ConfigArray, ConfigBusNumberArray;
    PBUS_HANDLER ParentHandler;
    KIRQL OldIrql;
    NTSTATUS Status;
    
    /* Make sure we have a valid handler */
    ASSERT((InterfaceType != InterfaceTypeUndefined) ||
           (ConfigType != ConfigurationSpaceUndefined));
    
    /* Allocate the bus handler */
    Bus = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(HAL_BUS_HANDLER) + ExtraData,
                                TAG_BUS_HANDLER);
    if (!Bus) return STATUS_INSUFFICIENT_RESOURCES;
    
    /* Return the handler */
    *ReturnedBusHandler = &Bus->Handler;
    
    /* FIXME: Fix the kernel first. Don't page us out */
    //CodeHandle = MmLockPagableDataSection(&HaliRegisterBusHandler);

    /* Synchronize with anyone else */
    KeWaitForSingleObject(&HalpBusDatabaseEvent,
                          WrExecutive,
                          KernelMode,
                          FALSE,
                          NULL);
    
    /* Check for unknown/root bus */
    if (BusNumber == -1)
    {
        /* We must have an interface */
        ASSERT(InterfaceType != InterfaceTypeUndefined);
        
        /* Find the right bus */
        BusNumber = 0;
        while (HaliHandlerForBus(InterfaceType, BusNumber)) BusNumber++;
    }
    
    /* Allocate arrays for the handler */
    InterfaceArray = HalpAllocateArray(InterfaceType);
    InterfaceBusNumberArray = HalpAllocateArray(BusNumber);
    ConfigArray = HalpAllocateArray(ConfigType);
    ConfigBusNumberArray = HalpAllocateArray(BusNumber);
    
    /* Only proceed if all allocations succeeded */
    if ((InterfaceArray) && (InterfaceBusNumberArray) && (ConfigArray) && (ConfigBusNumberArray))
    {
        /* Find the parent handler if any */
        ParentHandler = HaliReferenceHandlerForBus(ParentBusType, ParentBusNumber);
        
        /* Initialize the handler */
        RtlZeroMemory(Bus, sizeof(HAL_BUS_HANDLER) + ExtraData);
        Bus->ReferenceCount = 1;
        
        /* Fill out bus data */
        Bus->Handler.BusNumber = BusNumber;
        Bus->Handler.InterfaceType = InterfaceType;
        Bus->Handler.ConfigurationType = ConfigType;
        Bus->Handler.ParentHandler = ParentHandler;

        /* Fill out dummy handlers */
        Bus->Handler.GetBusData = HalpNoBusData;
        Bus->Handler.SetBusData = HalpNoBusData;
        Bus->Handler.AdjustResourceList = HalpNoAdjustResourceList;
        Bus->Handler.AssignSlotResources = HalpNoAssignSlotResources;
        
        /* Make space for extra data */
        if (ExtraData) Bus->Handler.BusData = Bus + 1;

        /* Check for a parent handler */
        if (ParentHandler)
        {
            /* Inherit the parent routines */
            Bus->Handler.GetBusData = ParentHandler->GetBusData;
            Bus->Handler.SetBusData = ParentHandler->SetBusData;
            Bus->Handler.AdjustResourceList = ParentHandler->AdjustResourceList;
            Bus->Handler.AssignSlotResources = ParentHandler->AssignSlotResources;
            Bus->Handler.TranslateBusAddress = ParentHandler->TranslateBusAddress;
            Bus->Handler.GetInterruptVector = ParentHandler->GetInterruptVector;
        }
        
        /* We don't support this yet */
        ASSERT(!InstallCallback);
        
        /* Lock the buses */
        KeAcquireSpinLock(&HalpBusDatabaseSpinLock, &OldIrql);

        /* Make space for the interface */
        HalpGrowArray(&HalpBusTable, &InterfaceArray);
        
        /* Check if we really have an interface */
        if (InterfaceType != InterfaceTypeUndefined)
        {
            /* Make space for the association */
            HalpGrowArray((PARRAY*)&HalpBusTable->Element[InterfaceType],
                          &InterfaceBusNumberArray);
            
            /* Get the bus handler pointer */
            BusEntry = (PHAL_BUS_HANDLER*)&((PARRAY)HalpBusTable->Element[InterfaceType])->Element[BusNumber];
            
            /* Check if there was already a handler there, and set the new one */
            if (*BusEntry) OldHandler = *BusEntry;
            *BusEntry = Bus;
        }
        
        /* Now add a space for the configuration space */
        HalpGrowArray(&HalpConfigTable, &ConfigArray);
        
        /* Check if we really have one */
        if (ConfigType != ConfigurationSpaceUndefined)
        {
            /* Make space for this association */
            HalpGrowArray((PARRAY*)&HalpConfigTable->Element[ConfigType],
                          &ConfigBusNumberArray);
            
            /* Get the bus handler pointer */
            BusEntry = (PHAL_BUS_HANDLER*)&((PARRAY)HalpConfigTable->Element[ConfigType])->Element[BusNumber];
            if (*BusEntry)
            {
                /* Get the old entry, but make sure it's the same we had before */
                ASSERT((OldHandler == NULL) || (OldHandler == *BusEntry));
                OldHandler = *BusEntry;
            }
            
            /* Set the new entry */
            *BusEntry = Bus;
        }

        /* Link the adapter */
        InsertTailList(&HalpAllBusHandlers, &Bus->AllHandlers);
        
        /* Remove the old linkage */
        Bus = OldHandler;
        if (Bus) RemoveEntryList(&Bus->AllHandlers);

        /* Release the lock */
        KeReleaseSpinLock(&HalpBusDatabaseSpinLock, OldIrql);
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Signal the event */
    KeSetEvent(&HalpBusDatabaseEvent, 0, FALSE);
    
    /* FIXME: Fix the kernel first. Re-page the function */
    //MmUnlockPagableImageSection(CodeHandle);

    /* Free all allocations */
    if (Bus) ExFreePoolWithTag(Bus, TAG_BUS_HANDLER);
    if (InterfaceArray) ExFreePoolWithTag(InterfaceArray, TAG_BUS_HANDLER);
    if (InterfaceBusNumberArray) ExFreePoolWithTag(InterfaceBusNumberArray, TAG_BUS_HANDLER);
    if (ConfigArray) ExFreePoolWithTag(ConfigArray, TAG_BUS_HANDLER);
    if (ConfigBusNumberArray) ExFreePoolWithTag(ConfigBusNumberArray, TAG_BUS_HANDLER);

    /* And we're done */
    return Status;
}
#endif

VOID
NTAPI
HalpInitBusHandler(VOID)
{
    /* Setup the bus lock */
    KeInitializeSpinLock(&HalpBusDatabaseSpinLock);
    
    /* Setup the bus event */
    KeInitializeEvent(&HalpBusDatabaseEvent, SynchronizationEvent, TRUE);
    
    /* Setup the bus configuration and bus table */
    HalpBusTable = HalpAllocateArray(0);
    HalpConfigTable = HalpAllocateArray(0);
    
    /* Setup the bus list */
    InitializeListHead(&HalpAllBusHandlers);

    /* Setup the HAL Dispatch routines */
#ifndef _MINIHAL_
    HalRegisterBusHandler = HaliRegisterBusHandler;
    HalHandlerForBus = HaliHandlerForBus;
    HalHandlerForConfigSpace = HaliHandlerForConfigSpace;
    HalReferenceHandlerForBus = HaliReferenceHandlerForBus;
    HalReferenceBusHandler = HaliReferenceBusHandler;
    HalDereferenceBusHandler = HaliDereferenceBusHandler;
#endif
    HalPciAssignSlotResources = HalpAssignSlotResources;
    HalPciTranslateBusAddress = HaliTranslateBusAddress; /* PCI Driver can override */
    if (!HalFindBusAddressTranslation) HalFindBusAddressTranslation = HaliFindBusAddressTranslation;
}

/* EOF */
