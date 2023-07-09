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
--*/

#include "iop.h"
#pragma hdrstop

// INIT data segment

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

PVOID IopPnpScratchBuffer1 = NULL;
PVOID IopPnpScratchBuffer2 = NULL;
PCM_RESOURCE_LIST IopInitHalResources;
PDEVICE_NODE IopInitHalDeviceNode;
PIOP_RESERVED_RESOURCES_RECORD IopInitReservedResourceList;

// Regular data segment

#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg()
#endif

PDEVICE_NODE IopRootDeviceNode;// IopRootDeviceNode - the head of the PnP manager's device node tree.
PDRIVER_OBJECT IoPnpDriverObject;// IoPnPDriverObject - the madeup driver object for pnp manager
KSPIN_LOCK IopPnPSpinLock;// IopPnPSpinLock - spinlock for Pnp code.
ERESOURCE IopDeviceTreeLock;// IopDeviceTreeLock - performs syncronization around the whole device node tree.
KEVENT PiEventQueueEmpty;// PiEventQueueEmpty - Manual reset event which is set when the queue is empty
KEVENT PiEnumerationLock;// PiEnumerationLock - to synchronize boot phase device enumeration
LONG IopEnumerationCount;// iopEnumerationCount - indicates how many devices are being enumerated.
ULONG IopNumberDeviceNodes;// IopNumberDeviceNodes - Number of outstanding device nodes in the system.
LIST_ENTRY IopPnpEnumerationRequestList;// IopPnpEnumerationRequestList - a link list of device enumeration requests to worker thread.
BOOLEAN PnPInitialized;// PnPInitComplete - A flag to indicate if PnP initialization is completed.
BOOLEAN PnPDetectionEnabled;// PnPDetectionEnabled - A flag to indicate if detection code can be executed
BOOLEAN PnPBootDriversInitialized;// PnPBootDriverInitialied
BOOLEAN PnPBootDriversLoaded;// PnPBootDriverLoaded
BOOLEAN IopBootConfigsReserved;// IopBootConfigsReserved - Indicates whether we have reserved BOOT configs or not.

// IopResourcesReleased - a flag to indicate if a device is removed and its resources are freed.
//     This is for reallocating resources for DNF_INSUFFICIENT_RESOURCES devices.
BOOLEAN IopResourcesReleased;

PIO_RESERVE_RESOURCES_ROUTINE IopReserveResourcesRoutine;// Variable to hold resources reservation routine.
ULONG IoDeviceNodeTreeSequence;// Device node tree sequence.  Is bumped every time the tree is modified or a warm eject is queued.
PBUS_TYPE_GUID_LIST IopBusTypeGuidList;// List of queried bus type guids
INTERFACE_TYPE PnpDefaultInterfaceType;// PnpDefaultInterfaceTYpe - Use this if the interface type of resource list is unknown.
BOOLEAN PnpAsyncOk;// PnpStartAsynOk - control how start irp should be handled. Synchronously or Asynchronously?
ULONG IopMaxDeviceNodeLevel;// IopMaxDeviceNodeLevel - Level number of the DeviceNode deepest in the tree
LIST_ENTRY  IopPendingEjects;// IopPendingEjects - List of pending eject requests
LIST_ENTRY  IopPendingSurpriseRemovals;// IopPendingSurpriseRemovals - List of pending surprise removal requests
KEVENT IopWarmEjectLock;// Warm eject lock - only one warm eject is allowed to occur at a time
PDEVICE_OBJECT IopWarmEjectPdo;// This field contains a devobj if a warm eject is in progress.

// Arbiter data

ARBITER_INSTANCE IopRootPortArbiter;
ARBITER_INSTANCE IopRootMemArbiter;
ARBITER_INSTANCE IopRootDmaArbiter;
ARBITER_INSTANCE IopRootIrqArbiter;
ARBITER_INSTANCE IopRootBusNumberArbiter;