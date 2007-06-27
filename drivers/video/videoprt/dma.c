/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#include "videoprt.h"

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

PVOID NTAPI
VideoPortAllocateCommonBuffer(
   IN PVOID HwDeviceExtension,
   IN PVP_DMA_ADAPTER VpDmaAdapter,
   IN ULONG DesiredLength,
   OUT PPHYSICAL_ADDRESS LogicalAddress,
   IN BOOLEAN CacheEnabled,
   PVOID Reserved)
{
   return HalAllocateCommonBuffer(
      (PADAPTER_OBJECT)VpDmaAdapter,
      DesiredLength,
      LogicalAddress,
      CacheEnabled);
}

/*
 * @implemented
 */

VOID NTAPI
VideoPortReleaseCommonBuffer(
   IN PVOID HwDeviceExtension,
   IN PVP_DMA_ADAPTER VpDmaAdapter,
   IN ULONG Length,
   IN PHYSICAL_ADDRESS LogicalAddress,
   IN PVOID VirtualAddress,
   IN BOOLEAN CacheEnabled)
{
   HalFreeCommonBuffer(
      (PADAPTER_OBJECT)VpDmaAdapter,
      Length,
      LogicalAddress,
      VirtualAddress,
      CacheEnabled);
}

/*
 * @unimplemented
 */

VOID NTAPI
VideoPortPutDmaAdapter(
   IN PVOID HwDeviceExtension,
   IN PVP_DMA_ADAPTER VpDmaAdapter)
{
   DPRINT1("unimplemented VideoPortPutDmaAdapter\n");
}

/*
 * @unimplemented
 */

PVP_DMA_ADAPTER NTAPI
VideoPortGetDmaAdapter(
   IN PVOID HwDeviceExtension,
   IN PVP_DEVICE_DESCRIPTION VpDeviceExtension)
{
   DEVICE_DESCRIPTION DeviceDescription;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   ULONG NumberOfMapRegisters;
   PVP_DMA_ADAPTER Adapter;

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

   DPRINT("VideoPortGetDmaAdapter\n");

   DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
   DeviceDescription.Master = TRUE /* ?? */;
   DeviceDescription.ScatterGather = VpDeviceExtension->ScatterGather;
   DeviceDescription.DemandMode = FALSE /* ?? */;
   DeviceDescription.AutoInitialize = FALSE /* ?? */;
   DeviceDescription.Dma32BitAddresses = VpDeviceExtension->Dma32BitAddresses;
   DeviceDescription.IgnoreCount = FALSE /* ?? */;
   DeviceDescription.Reserved1 = FALSE;
   DeviceDescription.BusNumber = DeviceExtension->SystemIoBusNumber;
   DeviceDescription.DmaChannel = 0 /* ?? */;
   DeviceDescription.InterfaceType = DeviceExtension->AdapterInterfaceType;
   DeviceDescription.DmaWidth = Width8Bits;
   DeviceDescription.DmaSpeed = Compatible;
   DeviceDescription.MaximumLength = VpDeviceExtension->MaximumLength;
   DeviceDescription.DmaPort = 0;

   Adapter =
      (PVP_DMA_ADAPTER)HalGetAdapter(&DeviceDescription, &NumberOfMapRegisters);
   DPRINT("Adapter %X\n", Adapter);
   return(Adapter);
}

/*
 * @implemented
 */
VOID NTAPI
VideoPortFreeCommonBuffer( IN PVOID HwDeviceExtension,
                                 IN ULONG  Length,
                                 IN PVOID  VirtualAddress,
                                 IN PHYSICAL_ADDRESS  LogicalAddress,
                                 IN BOOLEAN  CacheEnabled)
{
   DEVICE_DESCRIPTION DeviceDescription;
   PVP_DMA_ADAPTER VpDmaAdapter;

   VpDmaAdapter = VideoPortGetDmaAdapter(
                    HwDeviceExtension, 
                    (PVP_DEVICE_DESCRIPTION)&DeviceDescription);

   HalFreeCommonBuffer(
      (PADAPTER_OBJECT)VpDmaAdapter,
      Length,
      LogicalAddress,
      VirtualAddress,
      CacheEnabled);
}
