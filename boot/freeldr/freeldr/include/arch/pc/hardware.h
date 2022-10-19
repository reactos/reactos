/*
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#define TAG_HW_RESOURCE_LIST    'lRwH'
#define TAG_HW_DISK_CONTEXT     'cDwH'

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
BOOLEAN
(*FIND_PCI_BIOS)(PPCI_REGISTRY_INFO BusData);

extern FIND_PCI_BIOS FindPciBios;

typedef
ULONG
(*GET_SERIAL_PORT)(ULONG Index, PULONG Irq);

VOID
DetectBiosDisks(PCONFIGURATION_COMPONENT_DATA SystemKey,
                PCONFIGURATION_COMPONENT_DATA BusKey);

/* hwacpi.c */
VOID DetectAcpiBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber);

/* hwapm.c */
VOID DetectApmBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber);

/* hwpci.c */
VOID DetectPciBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber);

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
