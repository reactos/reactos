/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/bus/bushndlr.c
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
    if (ArraySize == -1) ArraySize = 0;
    Size = ArraySize * sizeof(PARRAY) + sizeof(ARRAY);
    
    /* Allocate the array */
    Array = ExAllocatePoolWithTag(NonPagedPool,
                                  Size,
                                  'BusH');
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

    /* These should be written by the PCI driver later, but we give defaults */
    HalPciTranslateBusAddress = HalpTranslateBusAddress;
    HalPciAssignSlotResources = HalpAssignSlotResources;
    HalFindBusAddressTranslation = HalpFindBusAddressTranslation;
}
