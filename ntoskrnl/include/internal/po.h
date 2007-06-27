/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/po.h
* PURPOSE:         Internal header for the Power Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

#include <poclass.h>

//
// Define this if you want debugging support
//
#define _PO_DEBUG_                                      0x00

//
// These define the Debug Masks Supported
//
#define PO_STATE_DEBUG                                  0x01

//
// Debug/Tracing support
//
#if _PO_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define POTRACE DbgPrintEx
#else
#define POTRACE(x, ...)                                 \
    if (x & PopTraceLevel) DbgPrint(__VA_ARGS__)
#endif
#else
#define POTRACE(x, ...) DPRINT(__VA_ARGS__)
#endif

//
// Initialization routines
//
BOOLEAN
NTAPI
PoInitSystem(
    IN ULONG BootPhase,
    IN BOOLEAN HaveAcpiTable
);

VOID
NTAPI
PoInitializePrcb(
    IN PKPRCB Prcb
);

//
// Power State routines
//
NTSTATUS
NTAPI
PopSetSystemPowerState(
    SYSTEM_POWER_STATE PowerState
);

VOID
NTAPI
PopCleanupPowerState(
    IN PPOWER_STATE PowerState
);

NTSTATUS
NTAPI
PopAddRemoveSysCapsCallback(
    IN PVOID NotificationStructure,
    IN PVOID Context
);

//
// Notifications
//
VOID
NTAPI
PoNotifySystemTimeSet(
    VOID
);

//
// Global data inside the Power Manager
//
extern PDEVICE_NODE PopSystemPowerDeviceNode;
