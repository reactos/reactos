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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __I386_HARDWARE_H_
#define __I386_HARDWARE_H_

typedef enum
{
  InterfaceTypeUndefined = -1,
  Internal,
  Isa,
  Eisa,
  MicroChannel,
  TurboChannel,
  PCIBus,
  VMEBus,
  NuBus,
  PCMCIABus,
  CBus,
  MPIBus,
  MPSABus,
  ProcessorInternal,
  InternalPowerBus,
  PNPISABus,
  MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;


typedef enum _CM_RESOURCE_TYPE
{
  CmResourceTypeNull = 0,
  CmResourceTypePort,
  CmResourceTypeInterrupt,
  CmResourceTypeMemory,
  CmResourceTypeDma,
  CmResourceTypeDeviceSpecific,
  CmResourceTypeMaximum
} CM_RESOURCE_TYPE;


typedef enum _CM_SHARE_DISPOSITION
{
  CmResourceShareUndetermined = 0,
  CmResourceShareDeviceExclusive,
  CmResourceShareDriverExclusive,
  CmResourceShareShared
} CM_SHARE_DISPOSITION;


typedef U64 PHYSICAL_ADDRESS;

typedef struct
{
  U8 Type;
  U8 ShareDisposition;
  U16 Flags;
  union
    {
      struct
	{
	  PHYSICAL_ADDRESS Start;
	  U32 Length;
	} __attribute__((packed)) Port;
      struct
	{
	  U32 Level;
	  U32 Vector;
	  U32 Affinity;
	} __attribute__((packed)) Interrupt;
      struct
	{
	  PHYSICAL_ADDRESS Start;
	  U32 Length;
	} __attribute__((packed)) Memory;
      struct
	{
	  U32 Channel;
	  U32 Port;
	  U32 Reserved1;
	} __attribute__((packed)) Dma;
      struct
	{
	  U32 DataSize;
	  U32 Reserved1;
	  U32 Reserved2;
	} __attribute__((packed)) DeviceSpecificData;
    } __attribute__((packed)) u;
} __attribute__((packed)) CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;


typedef struct
{
  U16 Version;
  U16 Revision;
  U32 Count;
  CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} __attribute__((packed))CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;


typedef struct
{
  INTERFACE_TYPE InterfaceType;
  U32 BusNumber;
  CM_PARTIAL_RESOURCE_LIST PartialResourceList;
} __attribute__((packed)) CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;


typedef struct _CM_COMPONENT_INFORMATION
{
  U32 Flags;
  U32 Version;
  U32 Key;
  U32 Affinity;
} __attribute__((packed)) CM_COMPONENT_INFORMATION, *PCM_COMPONENT_INFORMATION;


/* CM_COMPONENT_INFORMATION.Flags */
#define Failed      0x00000001
#define ReadOnly    0x00000002
#define Removable   0x00000004
#define ConsoleIn   0x00000008
#define ConsoleOut  0x00000010
#define Input       0x00000020
#define Output      0x00000040


/* PROTOTYPES ***************************************************************/

/* hardware.c */
VOID SetComponentInformation(HKEY ComponentKey,
			     U32 Flags,
			     U32 Key,
			     U32 Affinity);

/* hwcpu.c */

/* i386cpu.S */
U32 CpuidSupported(VOID);
VOID GetCpuid(U32 Level,
	      U32 *eax,
	      U32 *ebx,
	      U32 *ecx,
	      U32 *edx);

U32 MpsSupported(VOID);
U32 MpsGetDefaultConfiguration(VOID);
U32 MpsGetConfigurationTable(PVOID ConfigTable);

/* i386pnp.S */
//U32 PnpBiosSupported(VOID);
U32 PnpBiosSupported(PVOID InstallationCheck);
U32 PnpBiosGetDeviceNodeCount(U32 *NodeSize,
			      U32 *NodeCount);
U32 PnpBiosGetDeviceNode(U8 *NodeId,
			 U8 *NodeBuffer);

#endif /* __I386_HARDWARE_H_ */

/* EOF */
