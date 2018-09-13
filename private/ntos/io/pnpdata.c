/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pnpsubs.c

Abstract:

    This module contains the plug-and-play data

Author:

    Shie-Lin Tzong (shielint) 30-Jan-1995

Environment:

    Kernel mode


Revision History:


--*/

#include "iop.h"
#pragma hdrstop

//
// INIT data segment
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

PVOID IopPnpScratchBuffer1 = NULL;
PVOID IopPnpScratchBuffer2 = NULL;
PCM_RESOURCE_LIST IopInitHalResources;
PDEVICE_NODE IopInitHalDeviceNode;
PIOP_RESERVED_RESOURCES_RECORD IopInitReservedResourceList;

//
// Regular data segment
//

#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg()
#endif

//
// IopRootDeviceNode - the head of the PnP manager's device node tree.
//

PDEVICE_NODE IopRootDeviceNode;

//
// IoPnPDriverObject - the madeup driver object for pnp manager
//

PDRIVER_OBJECT IoPnpDriverObject;

//
// IopPnPSpinLock - spinlock for Pnp code.
//

KSPIN_LOCK IopPnPSpinLock;

//
// IopDeviceTreeLock - performs syncronization around the whole device node tree.
//

ERESOURCE IopDeviceTreeLock;

//
// PiEventQueueEmpty - Manual reset event which is set when the queue is empty
//

KEVENT PiEventQueueEmpty;

//
// PiEnumerationLock - to synchronize boot phase device enumeration
//

KEVENT PiEnumerationLock;

//
// iopEnumerationCount - indicates how many devices are being enumerated.
//

LONG IopEnumerationCount;

//
// IopNumberDeviceNodes - Number of outstanding device nodes in the system.
//

ULONG IopNumberDeviceNodes;

//
// IopPnpEnumerationRequestList - a link list of device enumeration requests to worker thread.
//

LIST_ENTRY IopPnpEnumerationRequestList;

//
// PnPInitComplete - A flag to indicate if PnP initialization is completed.
//

BOOLEAN PnPInitialized;

//
// PnPDetectionEnabled - A flag to indicate if detection code can be executed
//

BOOLEAN PnPDetectionEnabled;

//
// PnPBootDriverInitialied
//

BOOLEAN PnPBootDriversInitialized;

//
// PnPBootDriverLoaded
//

BOOLEAN PnPBootDriversLoaded;

//
// IopBootConfigsReserved - Indicates whether we have reserved BOOT configs or not.
//

BOOLEAN IopBootConfigsReserved;

//
// IopResourcesReleased - a flag to indicate if a device is removed and its resources
//     are freed.  This is for reallocating resources for DNF_INSUFFICIENT_RESOURCES
//     devices.
//

BOOLEAN IopResourcesReleased;

//
// Variable to hold resources reservation routine.
//

PIO_RESERVE_RESOURCES_ROUTINE IopReserveResourcesRoutine;

//
// Device node tree sequence.  Is bumped every time the tree is modified or a warm
// eject is queued.
//

ULONG IoDeviceNodeTreeSequence;

//
// List of queried bus type guids
//

PBUS_TYPE_GUID_LIST IopBusTypeGuidList;

//
// PnpDefaultInterfaceTYpe - Use this if the interface type of resource list is unknown.
//

INTERFACE_TYPE PnpDefaultInterfaceType;

//
// PnpStartAsynOk - control how start irp should be handled. Synchronously or Asynchronously?
//

BOOLEAN PnpAsyncOk;

//
// IopMaxDeviceNodeLevel - Level number of the DeviceNode deepest in the tree
//
ULONG IopMaxDeviceNodeLevel;

//
// IopPendingEjects - List of pending eject requests
//
LIST_ENTRY  IopPendingEjects;

//
// IopPendingSurpriseRemovals - List of pending surprise removal requests
//
LIST_ENTRY  IopPendingSurpriseRemovals;

//
// Warm eject lock - only one warm eject is allowed to occur at a time
//
KEVENT IopWarmEjectLock;

//
// This field contains a devobj if a warm eject is in progress.
//
PDEVICE_OBJECT IopWarmEjectPdo;

//
// Arbiter data
//

ARBITER_INSTANCE IopRootPortArbiter;
ARBITER_INSTANCE IopRootMemArbiter;
ARBITER_INSTANCE IopRootDmaArbiter;
ARBITER_INSTANCE IopRootIrqArbiter;
ARBITER_INSTANCE IopRootBusNumberArbiter;
