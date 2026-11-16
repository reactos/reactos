/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        boot/freeldr/freeldr/include/arch/pc/hardware.h
 * PURPOSE:     PC hardware detection prototypes
 * COPYRIGHT:   Copyright 2003 Eric Kohl
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Eric Kohl
 *              Ryan Wilson <ryan@verosnt.com>
 */

#pragma once

#define TAG_HW_RESOURCE_LIST    'lRwH'
#define TAG_HW_DISK_CONTEXT     'cDwH'

/*
 * These aren't defined in the ioaccess.h header.
 * Because of that we manually define the symbols we need to make use of I/O ports.
 */
#define READ_PORT_BUFFER_UCHAR(port, buffer, count)   __inbytestring(H2I(port), buffer, count)
#define READ_PORT_BUFFER_USHORT(port, buffer, count)  __inwordstring(H2I(port), buffer, count)
#define READ_PORT_BUFFER_ULONG(port, buffer, count)   __indwordstring(H2I(port), buffer, count)
#define WRITE_PORT_BUFFER_UCHAR(port, buffer, count)  __outbytestring(H2I(port), buffer, count)
#define WRITE_PORT_BUFFER_USHORT(port, buffer, count) __outwordstring(H2I(port), buffer, count)
#define WRITE_PORT_BUFFER_ULONG(port, buffer, count)  __outdwordstring(H2I(port), buffer, count)

/* PROTOTYPES ***************************************************************/

/* hardware.c */
VOID StallExecutionProcessor(ULONG Microseconds);
VOID HalpCalibrateStallExecution(VOID);

/* PCI Type 1 Ports */
#define PCI_TYPE1_ADDRESS_PORT      (PULONG)0xCF8
#define PCI_TYPE1_DATA_PORT         0xCFC

/* PCI Type 1 Configuration Register */
typedef struct _PCI_TYPE1_CFG_BITS
{
    union
    {
        struct
        {
            ULONG RegisterNumber:8;
            ULONG FunctionNumber:3;
            ULONG DeviceNumber:5;
            ULONG BusNumber:8;
            ULONG Reserved:7;
            ULONG Enable:1;
        } bits;

        ULONG AsULONG;
    } u;
} PCI_TYPE1_CFG_BITS, *PPCI_TYPE1_CFG_BITS;

typedef
PCM_PARTIAL_RESOURCE_LIST
(*GET_HARDDISK_CONFIG_DATA)(UCHAR DriveNumber, ULONG* pSize);

extern GET_HARDDISK_CONFIG_DATA GetHarddiskConfigurationData;

typedef
ULONG
(*GET_SERIAL_PORT)(ULONG Index, PULONG Irq);

VOID
DetectBiosDisks(PCONFIGURATION_COMPONENT_DATA SystemKey,
                PCONFIGURATION_COMPONENT_DATA BusKey);

/* hwacpi.c */
VOID DetectAcpiBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber);
VOID PcStopFirmwareWatchdog(VOID);
VOID PcUnmapReservedBootRanges(VOID);

/* hwapm.c */
VOID DetectApmBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber);

/* hwpci.c */
typedef
BOOLEAN
(*DETECT_PCI_BUS)(
    _In_ PCONFIGURATION_COMPONENT_DATA SystemKey,
    _Inout_ PULONG BusNumber,
    _Out_ PPCI_REGISTRY_INFO BusData);

VOID
DetectPciBus(
    _In_ PCONFIGURATION_COMPONENT_DATA SystemKey,
    _Inout_ PULONG BusNumber,
    _In_ DETECT_PCI_BUS MachDetectPciBus);

/* i386pnp.S */
ULONG_PTR __cdecl PnpBiosSupported(VOID);
ULONG __cdecl PnpBiosGetDeviceNodeCount(ULONG *NodeSize,
                  ULONG *NodeCount);
ULONG __cdecl PnpBiosGetDeviceNode(UCHAR *NodeId,
             UCHAR *NodeBuffer);
ULONG __cdecl PnpBiosGetDockStationInformation(UCHAR *DockingStationInfo);

/* i386pxe.S */
USHORT __cdecl PxeCallApi(USHORT Segment, USHORT Offset, USHORT Service, VOID* Parameter);

/* EOF */
