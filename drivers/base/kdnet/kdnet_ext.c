/*
 * PROJECT:     ReactOS Networking Debugging Module
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Kdnet extensibility initialization
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "kdnet.h"

PKDNET_EXTENSIBILITY_EXPORTS KdNetExtensibilityExports = NULL;

#include <ndk/haltypes.h>
#include <ndk/halfuncs.h>

static NTSTATUS g_KdNetErrorStatus = STATUS_SUCCESS;
static PWCHAR   g_KdNetErrorString = NULL;
static ULONG    g_KdNetHardwareId  = 0;

static UCHAR  NTAPI KdNetRrUchar(PUCHAR r)            { return READ_REGISTER_UCHAR(r); }
static USHORT NTAPI KdNetRrUshort(PUSHORT r)          { return READ_REGISTER_USHORT(r); }
static ULONG  NTAPI KdNetRrUlong(PULONG r)            { return READ_REGISTER_ULONG(r); }
static VOID   NTAPI KdNetWrUchar(PUCHAR r, UCHAR v)   { WRITE_REGISTER_UCHAR(r, v); }
static VOID   NTAPI KdNetWrUshort(PUSHORT r, USHORT v){ WRITE_REGISTER_USHORT(r, v); }
static VOID   NTAPI KdNetWrUlong(PULONG r, ULONG v)   { WRITE_REGISTER_ULONG(r, v); }
static UCHAR  NTAPI KdNetRpUchar(PUCHAR p)            { return READ_PORT_UCHAR(p); }
static USHORT NTAPI KdNetRpUshort(PUSHORT p)          { return READ_PORT_USHORT(p); }
static ULONG  NTAPI KdNetRpUlong(PULONG p)            { return READ_PORT_ULONG(p); }
static VOID   NTAPI KdNetWpUchar(PUCHAR p, UCHAR v)   { WRITE_PORT_UCHAR(p, v); }
static VOID   NTAPI KdNetWpUshort(PUSHORT p, USHORT v){ WRITE_PORT_USHORT(p, v); }
static VOID   NTAPI KdNetWpUlong(PULONG p, ULONG v)   { WRITE_PORT_ULONG(p, v); }
static VOID   NTAPI KdNetStall(ULONG us)              { KeStallExecutionProcessor(us); }


NTSTATUS
KdNetInitializeExtensibility(
    _In_opt_ PCHAR LoaderOptions,
    _Inout_ struct _DEBUG_DEVICE_DESCRIPTOR *Device,
    _In_opt_ PKDNET_INITIALIZE_LIBRARY KdInitializeLibrary,
    _Out_ PKDNET_EXTENSIBILITY_EXPORTS ExtensibilityExports,
    _Out_opt_ void *SerialExtensibility)
{
    NTSTATUS Status;
    PKDNET_INITIALIZE_LIBRARY InitLib;
    static KDNET_EXTENSIBILITY_IMPORTS Imports;

    if (!ExtensibilityExports)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(ExtensibilityExports, sizeof(*ExtensibilityExports));
    ExtensibilityExports->FunctionCount = KDNET_EXT_EXPORTS;

    RtlZeroMemory(&Imports, sizeof(Imports));
    Imports.FunctionCount = KDNET_EXT_IMPORTS;
    Imports.Exports = ExtensibilityExports;
    Imports.GetPciDataByOffset = (KDNET_GET_PCI_DATA_BY_OFFSET)KdGetPciDataByOffset;
    Imports.SetPciDataByOffset = (KDNET_SET_PCI_DATA_BY_OFFSET)KdSetPciDataByOffset;
    Imports.MapPhysicalMemory64 = (KDNET_MAP_PHYSICAL_MEMORY_64)KdMapPhysicalMemory64;
    Imports.UnmapVirtualAddress = (KDNET_UNMAP_VIRTUAL_ADDRESS)KdUnmapVirtualAddress;

    Imports.GetPhysicalAddress = (KDNET_GET_PHYSICAL_ADDRESS)MmGetPhysicalAddress;
    Imports.StallExecutionProcessor = KdNetStall;

    Imports.ReadRegisterUChar = KdNetRrUchar;
    Imports.ReadRegisterUShort = KdNetRrUshort;
    Imports.ReadRegisterULong = KdNetRrUlong;
    Imports.WriteRegisterUChar = KdNetWrUchar;
    Imports.WriteRegisterUShort = KdNetWrUshort;
    Imports.WriteRegisterULong = KdNetWrUlong;

    Imports.ReadPortUChar = KdNetRpUchar;
    Imports.ReadPortUShort = KdNetRpUshort;
    Imports.ReadPortULong = KdNetRpUlong;
    Imports.WritePortUChar = KdNetWpUchar;
    Imports.WritePortUShort = KdNetWpUshort;
    Imports.WritePortULong = KdNetWpUlong;

    Imports.SetHiberRange = (KDNET_SET_HIBER_RANGE)PoSetHiberRange;
    Imports.BugCheckEx = (KDNET_BUGCHECK_EX)KeBugCheckEx;

    /* TODO: */
    Imports.ReadRegisterULong64 = NULL;
    Imports.WriteRegisterULong64 = NULL;
    Imports.ReadPortULong64 = NULL;
    Imports.WritePortULong64 = NULL;
    Imports.ReadCycleCounter = NULL;
    Imports.KdNetDbgPrintf = (KDNET_DBGPRINT)FrLdrDbgPrint; //HACK-ish: 
    Imports.VmbusInitialize = NULL;
    Imports.GetHypervisorVendorId = NULL;
    Imports.ExecutionEnvironment = 0;

    Imports.KdNetErrorStatus = &g_KdNetErrorStatus;
    Imports.KdNetErrorString = &g_KdNetErrorString;
    Imports.KdNetHardwareID = &g_KdNetHardwareId;

    InitLib = KdInitializeLibrary;
    if (!InitLib)
        return STATUS_PROCEDURE_NOT_FOUND;

    Status = InitLib(&Imports, LoaderOptions, Device);
    if (FrLdrDbgPrint)
        FrLdrDbgPrint("kdnet: Ext KdInitializeLibrary=%p -> 0x%08lx\n", InitLib, Status);
    if (!NT_SUCCESS(Status))
        return Status;

    KdNetExtensibilityExports = ExtensibilityExports;
    return STATUS_SUCCESS;
}
