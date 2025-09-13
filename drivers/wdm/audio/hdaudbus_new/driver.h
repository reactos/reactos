/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/hdaudbus/driver.h
 * PURPOSE:         HDAUDBUS Driver
 * PROGRAMMER:      Coolstar TODO
                    Johannes Anderwald
 */

#if !defined(_SKLHDAUDBUS_H_)
#define _SKLHDAUDBUS_H_

#define POOL_ZERO_DOWN_LEVEL_SUPPORT

#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <ntddk.h>
#include <ks.h>
#include <initguid.h>
#include <wdm.h>
#include <wdmguid.h>
#include <ntintsafe.h>
#include <ntstrsafe.h>
#include <hdaudio.h>
#include <portcls.h>
#define NDEBUG
#include <debug.h>

typedef struct _CODEC_IDS {
    UINT32 CodecAddress;
    UINT8 FunctionGroupStartNode;

    UINT16 CtlrDevId;
    UINT16 CtlrVenId;

    BOOL IsDSP;

    UINT16 FuncId;
    UINT16 VenId;
    UINT16 DevId;
    UINT32 SubsysId;
    UINT16 RevId;
} CODEC_IDS, * PCODEC_IDS;


#include "hda_registers.h"
#include "fdo.h"
#include "buspdo.h"
#include "hdac_controller.h"
#include "hdac_stream.h"
#include "hda_verbs.h"

#define DRIVERNAME "sklhdaudbus.sys: "
#define SKLHDAUDBUS_POOL_TAG 'SADH'

#define VEN_INTEL 0x8086
#define VEN_VMWARE 0x15AD

#include "regfuncs.h"

#define MAXUINT64 ((UINT64)~ ((UINT64)0)) // FIXME
#define MAXULONG64 ((ULONG64)~ ((ULONG64)0)) // FIXME
#define MAXULONG32 ((ULONG32) ~((ULONG32)0)) // FIXME


VOID HDA_BusInterface(PVOID Context, PHDAUDIO_BUS_INTERFACE Interface);
VOID HDA_BusInterfaceV2(PVOID Context, PHDAUDIO_BUS_INTERFACE_V2 Interface);
VOID HDA_BusInterfaceBDL(PVOID Context, PHDAUDIO_BUS_INTERFACE_BDL InterfaceHDA);
VOID HDA_BusInterfaceStandard(PVOID Context, PBUS_INTERFACE_STANDARD InterfaceStandard);
VOID HDA_BusInterfaceReference(PVOID Context, PBUS_INTERFACE_REFERENCE InterfaceStandard);

HDAUDIO_BUS_INTERFACE_V3 HDA_BusInterfaceV3(PVOID Context);

#define IS_BXT(ven, dev) (ven == VEN_INTEL && dev == 0x5a98)

static inline void mdelay(LONG msec) {
	LARGE_INTEGER Interval;
	Interval.QuadPart = -10 * 1000 * (LONGLONG)msec;
	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

static inline void udelay(LONG usec) {
	LARGE_INTEGER Interval;
	Interval.QuadPart = -10 * (LONGLONG)usec;
	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

PVOID
AllocateItem(
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes);

VOID
FreeItem(
    __drv_freesMem(Mem) PVOID Item);

/* fdo.cpp */
KSERVICE_ROUTINE HDA_InterruptService;

NTSTATUS
NTAPI
HDA_FDOStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
NTAPI
HDA_FDORemoveDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp);

NTSTATUS
NTAPI
HDA_FDOQueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);


/* pdo.cpp*/

NTSTATUS
HDA_PDORemoveDevice(
    _In_ PDEVICE_OBJECT DeviceObject);

NTSTATUS
HDA_PDOQueryBusInformation(
    IN PIRP Irp);

NTSTATUS
NTAPI
HDA_PDOQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
HDA_PDOHandleQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
HDA_PDOQueryBusDeviceCapabilities(
    IN PIRP Irp);

NTSTATUS
HDA_PDOQueryBusDevicePnpState(
    IN PIRP Irp);

/* businterface.cpp */

NTSTATUS
HDA_PDOHandleQueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);


//
// Helper macros
//

#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_INFO    2
#define DEBUG_LEVEL_VERBOSE 3

#define DBG_INIT  1
#define DBG_PNP   2
#define DBG_IOCTL 4

static ULONG SklHdAudBusDebugLevel = 100;
static ULONG SklHdAudBusDebugCatagories = DBG_INIT | DBG_PNP | DBG_IOCTL;

#if 0
#define SklHdAudBusPrint(dbglevel, dbgcatagory, fmt, ...) {             \
    if (SklHdAudBusDebugLevel >= dbglevel &&                            \
        (SklHdAudBusDebugCatagories & dbgcatagory))                     \
		    {                                                           \
        DbgPrint(DRIVERNAME);                                           \
        DbgPrint(fmt, ##__VA_ARGS__);                                     \
		    }                                                           \
}
#else
#define SklHdAudBusPrint(dbglevel, fmt, ...) {                       \
}
#endif
#endif
