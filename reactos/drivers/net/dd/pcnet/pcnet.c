/*
 *  ReactOS AMD PCNet Driver
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 *
 * PROJECT:         ReactOS AMD PCNet Driver
 * FILE:            drivers/net/dd/pcnet/pcnet.c
 * PURPOSE:         PCNet Device Driver
 * PROGRAMMER:      Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *                  9-Sept-2003 vizzini - Created
 * NOTES:
 *     - this is hard-coded to NDIS4
 *     - this assumes a little-endian machine
 *     - this assumes a 32-bit machine
 *     - this doesn't handle multiple PCNET NICs yet
 *     - this driver includes both NdisRaw and NdisImmediate calls
 *       for NDIS testing purposes.  Pick your poison below.
 */
#include <ndis.h>
#include "pci.h"
#include "pcnethw.h"
#include "pcnet.h"


VOID
STDCALL
MiniportHandleInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext)
/*
 * FUNCTION: Handle an interrupt if told to by MiniportISR
 * ARGUMENTS: 
 *     MiniportAdapterContext: context specified to NdisMSetAttributes
 * NOTES:
 *     - Called by NDIS at DISPATCH_LEVEL
 */
{
  PADAPTER Adapter = (PADAPTER)MiniportAdapterContext;
  USHORT Data;
  BOOLEAN ErrorHandled = FALSE;
  BOOLEAN ReceiveHandled = FALSE;
  BOOLEAN IdonHandled = FALSE;
  USHORT Temp;

  PCNET_DbgPrint(("Called\n"));

  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);

  PCNET_DbgPrint(("CSR0 is 0x%x\n", Data));

  while(Data & CSR0_INTR)
    {
      if(Data & CSR0_ERR)
        {
#if DBG
          if(ErrorHandled)
            {
              PCNET_DbgPrint(("ERROR HANDLED TWO TIMES\n"));
              ASSERT(0);
            }

          ErrorHandled = TRUE;
#endif

          PCNET_DbgPrint(("clearing an error\n"));
          NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_MERR|CSR0_BABL|CSR0_CERR|CSR0_MISS);
          NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Temp);
          PCNET_DbgPrint(("CSR0 is now 0x%x\n", Temp));
        }
      else if(Data & CSR0_IDON)
        {
#if DBG
          if(IdonHandled)
            {
              PCNET_DbgPrint(("IDON HANDLED TWO TIMES\n"));
              ASSERT(0);
            }

          IdonHandled = TRUE;
#endif

          PCNET_DbgPrint(("clearing IDON\n"));
          NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_IDON);
          NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Temp);
          PCNET_DbgPrint(("CSR0 is now 0x%x\n", Temp));
        }
      else if(Data & CSR0_RINT)
        {
#if DBG
          if(ReceiveHandled)
            {
              PCNET_DbgPrint(("RECEIVE HANDLED TWO TIMES\n"));
              ASSERT(0);
            }

          ReceiveHandled = TRUE;
#endif

          PCNET_DbgPrint(("receive interrupt - clearing\n"));
          NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_RINT);
          NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Temp);
          PCNET_DbgPrint(("CSR0 is now 0x%x\n", Temp));

          while(1)
            {
              PRECEIVE_DESCRIPTOR Descriptor = Adapter->ReceiveDescriptorRingVirt + Adapter->CurrentReceiveDescriptorIndex;
              PCHAR Buffer;
              ULONG ByteCount;

              if(Descriptor->FLAGS & RD_OWN)
                {
                  PCNET_DbgPrint(("no more receive descriptors to process\n"));
                  break;
                }

              if(Descriptor->FLAGS & RD_ERR)
                {
                  PCNET_DbgPrint(("receive descriptor error: 0x%x\n", Descriptor->FLAGS));
                  break;
                }

              if(!((Descriptor->FLAGS & RD_STP) && (Descriptor->FLAGS & RD_ENP)))
                {
                  PCNET_DbgPrint(("receive descriptor not start&end: 0x%x\n", Descriptor->FLAGS));
                  break;
                }

              Buffer = Adapter->ReceiveBufferPtrVirt + Adapter->CurrentReceiveDescriptorIndex * BUFFER_SIZE;
              ByteCount = Descriptor->MCNT & 0xfff;

              PCNET_DbgPrint(("Indicating a %d-byte packet (index %d)\n", ByteCount, Adapter->CurrentReceiveDescriptorIndex));

              NdisMEthIndicateReceive(Adapter->MiniportAdapterHandle, 0, Buffer, 14, Buffer+14, ByteCount-14, ByteCount-14);

              memset(Descriptor, 0, sizeof(RECEIVE_DESCRIPTOR));
              Descriptor->RBADR = 
                  (ULONG)(Adapter->ReceiveBufferPtrPhys + Adapter->CurrentReceiveDescriptorIndex * BUFFER_SIZE);
              Descriptor->BCNT = (-BUFFER_SIZE) | 0xf000;
              Descriptor->FLAGS &= RD_OWN;

              Adapter->CurrentReceiveDescriptorIndex++;

              if(Adapter->CurrentReceiveDescriptorIndex == NUMBER_OF_BUFFERS)
                Adapter->CurrentReceiveDescriptorIndex = 0;
            }
        }
      else
        {
          PCNET_DbgPrint(("UNHANDLED INTERRUPT\n"));
          ASSERT(FALSE);
        }

      NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
    }

  /* re-enable interrupts */
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_IENA);

  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  PCNET_DbgPrint(("CSR0 is now 0x%x\n", Data));
}


NDIS_STATUS 
MiQueryCard(
    IN PADAPTER Adapter)
/*
 * FUNCTION: Detect the PCNET NIC in the configured slot and query its I/O address and interrupt vector
 * ARGUMENTS:
 *     MiniportAdapterContext: context supplied to NdisMSetAttributes
 * RETURNS:
 *     NDIS_STATUS_FAILURE on a general error
 *     NDIS_STATUS_ADAPTER_NOT_FOUND on not finding the adapter
 *     NDIS_STATUS_SUCCESS on succes
 */
{
  ULONG  buf32 = 0;
  UCHAR  buf8  = 0;
  NDIS_STATUS Status;

  /* Detect the card in the configured slot */
  Status = NdisReadPciSlotInformation(Adapter->MiniportAdapterHandle, Adapter->SlotNumber, PCI_PCIID, &buf32, 4);
  if(Status != 4)
    {
      Status =  NDIS_STATUS_FAILURE;
      PCNET_DbgPrint(("NdisReadPciSlotInformation failed\n"));
      BREAKPOINT;
      return Status;
    }

  if(buf32 != PCI_ID)
    {
      Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
      PCNET_DbgPrint(("card in slot 0x%x isn't us: 0x%x\n", Adapter->SlotNumber, buf32));
      BREAKPOINT;
      return Status;
    }

  Status = NdisReadPciSlotInformation(Adapter->MiniportAdapterHandle, Adapter->SlotNumber,
      PCI_COMMAND, &buf32, 4);
  if(Status != 4)
    {
      PCNET_DbgPrint(("NdisReadPciSlotInformation failed\n"));
      BREAKPOINT;
      return NDIS_STATUS_FAILURE;
    }

  PCNET_DbgPrint(("config/status register: 0x%x\n", buf32));

  if(buf32 & 0x1)
    {
      PCNET_DbgPrint(("io space access is enabled.\n"));
    }
  else
    {
      PCNET_DbgPrint(("io space is NOT enabled!\n"));
      BREAKPOINT;
      return NDIS_STATUS_FAILURE;
    }

  /* get IO base physical address */
  buf32 = 0;
  Status = NdisReadPciSlotInformation(Adapter->MiniportAdapterHandle, Adapter->SlotNumber, PCI_IOBAR, &buf32, 4);
  if(Status != 4)
    {
      Status = NDIS_STATUS_FAILURE;
      PCNET_DbgPrint(("NdisReadPciSlotInformation failed\n"));
      BREAKPOINT;
      return Status;
    }

  if(!buf32)
    {
      PCNET_DbgPrint(("No base i/o address set\n"));
      return NDIS_STATUS_FAILURE;
    }

  buf32 &= ~1;  /* even up address - comes out odd for some reason */

  PCNET_DbgPrint(("detected io address 0x%x\n", buf32));
  Adapter->IoBaseAddress = buf32;

  /* get interrupt vector */
  Status = NdisReadPciSlotInformation(Adapter->MiniportAdapterHandle, Adapter->SlotNumber, PCI_ILR, &buf8, 1);
  if(Status != 1)
    {
      Status = NDIS_STATUS_FAILURE;
      PCNET_DbgPrint(("NdisReadPciSlotInformation failed\n"));
      BREAKPOINT;
      return Status;
    }

  PCNET_DbgPrint(("interrupt: 0x%x\n", buf8));
  Adapter->InterruptVector = buf8;

  return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
MiGetConfig(
    PADAPTER Adapter, 
    NDIS_HANDLE WrapperConfigurationContext)
/*
 * FUNCTION: Get configuration parameters from the registry
 * ARGUMENTS:
 *     Adapter: pointer to the Adapter struct for this NIC
 *     WrapperConfigurationContext: Context passed into MiniportInitialize
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on success
 *     NDIS_STATUS_{something} on failure (return val from other Ndis calls)
 */
{
  PNDIS_CONFIGURATION_PARAMETER Parameter; 
  NDIS_HANDLE ConfigurationHandle = 0;
  UNICODE_STRING Keyword;
  NDIS_STATUS Status;

  NdisOpenConfiguration(&Status, &ConfigurationHandle, WrapperConfigurationContext);
  if(Status != NDIS_STATUS_SUCCESS)
    {
      PCNET_DbgPrint(("Unable to open configuration: 0x%x\n", Status));
      BREAKPOINT;
      return Status;
    }

  RtlInitUnicodeString(&Keyword, L"SlotNumber");
  NdisReadConfiguration(&Status, &Parameter, ConfigurationHandle, &Keyword, NdisParameterInteger);
  if(Status != NDIS_STATUS_SUCCESS)
    {
      PCNET_DbgPrint(("Unable to read slot number: 0x%x\n", Status));
      BREAKPOINT;
    }
  else 
    Adapter->SlotNumber = Parameter->ParameterData.IntegerData;

  NdisCloseConfiguration(ConfigurationHandle);

  return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
MiAllocateSharedMemory(
    PADAPTER Adapter)
/*
 * FUNCTION: Allocate all shared memory used by the miniport
 * ARGUMENTS:
 *     Adapter: Pointer to the miniport's adapter object
 * RETURNS:
 *     NDIS_STATUS_RESOURCES on insufficient memory
 *     NDIS_STATUS_SUCCESS on success
 */
{
  PTRANSMIT_DESCRIPTOR TransmitDescriptor;
  PRECEIVE_DESCRIPTOR  ReceiveDescriptor;
  NDIS_PHYSICAL_ADDRESS PhysicalAddress;
  ULONG i;

  /* allocate the initialization block */
  Adapter->InitializationBlockLength = sizeof(INITIALIZATION_BLOCK);
  NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle, Adapter->InitializationBlockLength, 
      FALSE, (PVOID *)&Adapter->InitializationBlockVirt, &PhysicalAddress);
  if(!Adapter->InitializationBlockVirt)
    {
      PCNET_DbgPrint(("insufficient resources\n"));
      BREAKPOINT;
      return NDIS_STATUS_RESOURCES;
    }

  if(((ULONG)Adapter->InitializationBlockVirt & 0x00000003) != 0)
    {
      PCNET_DbgPrint(("address 0x%x not dword-aligned\n", Adapter->InitializationBlockVirt));
      BREAKPOINT;
      return NDIS_STATUS_RESOURCES;
    }

  Adapter->InitializationBlockPhys = (PINITIALIZATION_BLOCK)NdisGetPhysicalAddressLow(PhysicalAddress);
  memset(Adapter->InitializationBlockVirt, 0, sizeof(INITIALIZATION_BLOCK));

  /* allocate the transport descriptor ring */
  Adapter->TransmitDescriptorRingLength = sizeof(TRANSMIT_DESCRIPTOR) * NUMBER_OF_BUFFERS;
  NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle, Adapter->TransmitDescriptorRingLength,
      FALSE, (PVOID *)&Adapter->TransmitDescriptorRingVirt, &PhysicalAddress);
  if(!Adapter->TransmitDescriptorRingVirt)
    {
      PCNET_DbgPrint(("insufficient resources\n"));
      BREAKPOINT;
      return NDIS_STATUS_RESOURCES;
    }

  if(((ULONG)Adapter->TransmitDescriptorRingVirt & 0x00000003) != 0)
    {
      PCNET_DbgPrint(("address 0x%x not dword-aligned\n", Adapter->TransmitDescriptorRingVirt));
      BREAKPOINT;
      return NDIS_STATUS_RESOURCES;
    }

  Adapter->TransmitDescriptorRingPhys = (PTRANSMIT_DESCRIPTOR)NdisGetPhysicalAddressLow(PhysicalAddress);
  memset(Adapter->TransmitDescriptorRingVirt, 0, sizeof(TRANSMIT_DESCRIPTOR) * NUMBER_OF_BUFFERS);

  /* allocate the receive descriptor ring */
  Adapter->ReceiveDescriptorRingLength = sizeof(RECEIVE_DESCRIPTOR) * NUMBER_OF_BUFFERS;
  NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle, Adapter->ReceiveDescriptorRingLength,
      FALSE, (PVOID *)&Adapter->ReceiveDescriptorRingVirt, &PhysicalAddress);
  if(!Adapter->ReceiveDescriptorRingVirt)
    {
      PCNET_DbgPrint(("insufficient resources\n"));
      BREAKPOINT;
      return NDIS_STATUS_RESOURCES;
    }

  if(((ULONG)Adapter->ReceiveDescriptorRingVirt & 0x00000003) != 0)
    {
      PCNET_DbgPrint(("address 0x%x not dword-aligned\n", Adapter->ReceiveDescriptorRingVirt));
      BREAKPOINT;
      return NDIS_STATUS_RESOURCES;
    }

  Adapter->ReceiveDescriptorRingPhys = (PRECEIVE_DESCRIPTOR)NdisGetPhysicalAddressLow(PhysicalAddress);
  memset(Adapter->ReceiveDescriptorRingVirt, 0, sizeof(RECEIVE_DESCRIPTOR) * NUMBER_OF_BUFFERS);

  /* allocate transmit buffers */
  Adapter->TransmitBufferLength = BUFFER_SIZE * NUMBER_OF_BUFFERS;
  NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle, Adapter->TransmitBufferLength, 
      FALSE, (PVOID *)&Adapter->TransmitBufferPtrVirt, &PhysicalAddress);
  if(!Adapter->TransmitBufferPtrVirt)
    {
      PCNET_DbgPrint(("insufficient resources\n"));
      BREAKPOINT;
      return NDIS_STATUS_RESOURCES;
    }

  if(((ULONG)Adapter->TransmitBufferPtrVirt & 0x00000003) != 0)
    {
      PCNET_DbgPrint(("address 0x%x not dword-aligned\n", Adapter->TransmitBufferPtrVirt));
      BREAKPOINT;
      return NDIS_STATUS_RESOURCES;
    }

  Adapter->TransmitBufferPtrPhys = (PCHAR)NdisGetPhysicalAddressLow(PhysicalAddress);
  memset(Adapter->TransmitBufferPtrVirt, 0, BUFFER_SIZE * NUMBER_OF_BUFFERS);

  /* allocate receive buffers */
  Adapter->ReceiveBufferLength = BUFFER_SIZE * NUMBER_OF_BUFFERS;
  NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle, Adapter->ReceiveBufferLength, 
      FALSE, (PVOID *)&Adapter->ReceiveBufferPtrVirt, &PhysicalAddress);
  if(!Adapter->ReceiveBufferPtrVirt)
    {
      PCNET_DbgPrint(("insufficient resources\n"));
      BREAKPOINT;
      return NDIS_STATUS_RESOURCES;
    }

  if(((ULONG)Adapter->ReceiveBufferPtrVirt & 0x00000003) != 0)
    {
      PCNET_DbgPrint(("address 0x%x not dword-aligned\n", Adapter->ReceiveBufferPtrVirt));
      BREAKPOINT;
      return NDIS_STATUS_RESOURCES;
    }

  Adapter->ReceiveBufferPtrPhys = (PCHAR)NdisGetPhysicalAddressLow(PhysicalAddress);
  memset(Adapter->ReceiveBufferPtrVirt, 0, BUFFER_SIZE * NUMBER_OF_BUFFERS);

  /* initialize tx descriptors */
  TransmitDescriptor = Adapter->TransmitDescriptorRingVirt;
  for(i = 0; i < NUMBER_OF_BUFFERS; i++)
    {
      (TransmitDescriptor+i)->TBADR = (ULONG)Adapter->TransmitBufferPtrPhys + i * BUFFER_SIZE;
      (TransmitDescriptor+i)->BCNT = 0xf000 | -BUFFER_SIZE; /* 2's compliment  + set top 4 bits */
      (TransmitDescriptor+i)->FLAGS = TD1_STP | TD1_ENP;
    }

  PCNET_DbgPrint(("transmit ring initialized\n"));

  /* initialize rx */
  ReceiveDescriptor = Adapter->ReceiveDescriptorRingVirt;
  for(i = 0; i < NUMBER_OF_BUFFERS; i++)
    {
      (ReceiveDescriptor+i)->RBADR = (ULONG)Adapter->ReceiveBufferPtrPhys + i * BUFFER_SIZE;
      (ReceiveDescriptor+i)->BCNT = 0xf0000 | -BUFFER_SIZE; /* 2's compliment  + set top 4 bits */
      (ReceiveDescriptor+i)->FLAGS |= RD_OWN;
    }

  PCNET_DbgPrint(("receive ring initialized\n"));

  return NDIS_STATUS_SUCCESS;
}


VOID
MiPrepareInitializationBlock(
    PADAPTER Adapter)
/*
 * FUNCTION: Initialize the initialization block
 * ARGUMENTS:
 *     Adapter: pointer to the miniport's adapter object
 */
{
  ULONG i = 0;

  /* read burned-in address from card */
  for(i = 0; i < 6; i++)
    NdisRawReadPortUchar(Adapter->PortOffset + i, Adapter->InitializationBlockVirt->PADR + i);

  /* set up receive ring */
  PCNET_DbgPrint(("Receive ring physical address: 0x%x\n", Adapter->ReceiveDescriptorRingPhys));
  Adapter->InitializationBlockVirt->RDRA = (ULONG)Adapter->ReceiveDescriptorRingPhys;
  Adapter->InitializationBlockVirt->RLEN = (LOG_NUMBER_OF_BUFFERS << 4) & 0xf0;

  /* set up transmit ring */
  PCNET_DbgPrint(("Transmit ring physical address: 0x%x\n", Adapter->TransmitDescriptorRingPhys));
  Adapter->InitializationBlockVirt->TDRA = (ULONG)Adapter->TransmitDescriptorRingPhys;
  Adapter->InitializationBlockVirt->TLEN = (LOG_NUMBER_OF_BUFFERS << 4) & 0xf0;
}


VOID
MiFreeSharedMemory(
    PADAPTER Adapter)
/*
 * FUNCTION: Free all allocated shared memory
 * ARGUMENTS:
 *     Adapter: pointer to the miniport's adapter struct
 */
{
  NDIS_PHYSICAL_ADDRESS PhysicalAddress;

  PhysicalAddress.u.HighPart = 0;

  if(Adapter->InitializationBlockVirt)
    {
      PhysicalAddress.u.LowPart = (ULONG)Adapter->InitializationBlockPhys;
      NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle, Adapter->InitializationBlockLength, 
          FALSE, (PVOID *)&Adapter->InitializationBlockVirt, PhysicalAddress);
    }

  if(Adapter->TransmitDescriptorRingVirt)
    {
      PhysicalAddress.u.LowPart = (ULONG)Adapter->TransmitDescriptorRingPhys;
      NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle, Adapter->TransmitDescriptorRingLength,
        FALSE, (PVOID *)&Adapter->TransmitDescriptorRingVirt, PhysicalAddress);
    }

  if(Adapter->ReceiveDescriptorRingVirt)
    {
      PhysicalAddress.u.LowPart = (ULONG)Adapter->ReceiveDescriptorRingPhys;
      NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle, Adapter->ReceiveDescriptorRingLength,
          FALSE, (PVOID *)&Adapter->ReceiveDescriptorRingVirt, PhysicalAddress);
    }

  if(Adapter->TransmitBufferPtrVirt)
    {
      PhysicalAddress.u.LowPart = (ULONG)Adapter->TransmitBufferPtrPhys;
      NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle, Adapter->TransmitBufferLength, 
          FALSE, (PVOID *)&Adapter->TransmitBufferPtrVirt, PhysicalAddress);
    }

  if(Adapter->ReceiveBufferPtrVirt)
    {
      PhysicalAddress.u.LowPart = (ULONG)Adapter->ReceiveBufferPtrPhys;
      NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle, Adapter->ReceiveBufferLength, 
          FALSE, (PVOID *)&Adapter->ReceiveBufferPtrVirt, PhysicalAddress);
    }
}


VOID
STDCALL
MiniportHalt(
    IN NDIS_HANDLE MiniportAdapterContext)
/*
 * FUNCTION: Stop the adapter and release any per-adapter resources
 * ARGUMENTS:
 *     MiniportAdapterContext: context specified to NdisMSetAttributes
 * NOTES:
 *     - Called by NDIS at PASSIVE_LEVEL
 */
{
  PADAPTER Adapter = (PADAPTER)Adapter;

  PCNET_DbgPrint(("Called\n"));
  ASSERT(Adapter);

  /* stop the chip */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_STOP);

  /* deregister the interrupt */
  NdisMDeregisterInterrupt(&Adapter->InterruptObject);

  /* deregister i/o port range */
  NdisMDeregisterIoPortRange(Adapter->MiniportAdapterHandle, Adapter->IoBaseAddress, NUMBER_OF_PORTS, Adapter->PortOffset);

  /* free shared memory */
  MiFreeSharedMemory(Adapter);

  /* free map registers */
  NdisMFreeMapRegisters(Adapter->MiniportAdapterHandle);

  /* free the adapter */
  NdisFreeMemory(Adapter, 0, 0);
}


VOID
MiInitChip(
    PADAPTER Adapter)
/*
 * FUNCTION: Initialize and start the PCNET chip
 * ARGUMENTS:
 *     Adapter: pointer to the miniport's adapter struct
 * NOTES:
 *     - should be coded to detect failure and return an error
 *     - the vmware virtual lance chip doesn't support 32-bit i/o so don't do that.
 */
{
  USHORT Data = 0;

  PCNET_DbgPrint(("Called\n"));

  /*
   * first reset the chip - 32-bit reset followed by 16-bit reset.  if it's in 32-bit mode, it'll reset
   * twice.  if it's in 16-bit mode, the first read will be nonsense and the second will be a reset.  the
   * card is reset by reading from the reset register.  on reset it's in 16-bit i/o mode.
   */
  NdisRawReadPortUshort(Adapter->PortOffset + RESET32, &Data);
  NdisRawReadPortUshort(Adapter->PortOffset + RESET16, &Data);

  /* stop the chip */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_STOP);

  /* pause for 1ms so the chip will have time to reset */
  NdisStallExecution(1);

  PCNET_DbgPrint(("chip stopped\n"));

  /* set the software style to 2 (32 bits) */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR58);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);

  Data |= SW_STYLE_2;

  NdisRawWritePortUshort(Adapter->PortOffset + RDP, Data);

  /* set up csr4: auto transmit pad, disable polling, disable transmit interrupt, dmaplus */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR4);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);

  Data |= CSR4_APAD_XMT | /* CSR4_DPOLL |*/ CSR4_TXSTRTM | CSR4_DMAPLUS;
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, Data);

  /* set up bcr18: burst read/write enable */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, BCR18);
  NdisRawReadPortUshort(Adapter->PortOffset + BDP, &Data);

  Data |= BCR18_BREADE | BCR18_BWRITE ;
  NdisRawWritePortUshort(Adapter->PortOffset + BDP, Data);

  /* set up csr1 and csr2 with init block */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR1);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, (USHORT)((ULONG)Adapter->InitializationBlockPhys & 0xffff));
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR2);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, (USHORT)((ULONG)Adapter->InitializationBlockPhys >> 16) & 0xffff);

  PCNET_DbgPrint(("programmed with init block\n"));

  /* Set mode to 0 */
  Data = 0;
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR15);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, Data);

  /* load init block and start the card */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_STRT|CSR0_INIT|CSR0_IENA);

  PCNET_DbgPrint(("card started\n"));

  Adapter->Flags &= ~RESET_IN_PROGRESS;
}


BOOLEAN
MiTestCard(
    PADAPTER Adapter)
/*
 * FUNCTION: Test the NIC
 * ARGUMENTS:
 *     Adapter: pointer to the miniport's adapter struct
 * RETURNS:
 *     TRUE if the test succeeds
 *     FALSE otherwise
 * NOTES:
 *     - this is where to add diagnostics.  This is called
 *       at the very end of initialization.
 */
{
  int i = 0;
  UCHAR address[6];
  USHORT Data = 0;

  /* see if we can read/write now */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  PCNET_DbgPrint(("Port 0x%x RAP 0x%x CSR0 0x%x RDP 0x%x, Interupt status register is 0x%x\n", Adapter->PortOffset, RAP, CSR0, RDP, Data));

  /* read the BIA */
  for(i = 0; i < 6; i++)
      NdisRawReadPortUchar(Adapter->PortOffset + i, &address[i]);

  PCNET_DbgPrint(("burned-in address: %x:%x:%x:%x:%x:%x\n", address[0], address[1], address[2], address[3], address[4], address[5]));
  /* Read status flags from CSR0 */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  PCNET_DbgPrint(("CSR0: 0x%x\n", Data));

  /* Read status flags from CSR3 */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR3);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);

  PCNET_DbgPrint(("CSR3: 0x%x\n", Data));
  /* Read status flags from CSR4 */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR4);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  PCNET_DbgPrint(("CSR4: 0x%x\n", Data));

  /* Read status flags from CSR5 */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR5);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  PCNET_DbgPrint(("CSR5: 0x%x\n", Data));

  /* Read status flags from CSR6 */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR6);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  PCNET_DbgPrint(("CSR6: 0x%x\n", Data));

  return TRUE;
}


NDIS_STATUS
STDCALL
MiniportInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext)
/*
 * FUNCTION:  Initialize a new miniport
 * ARGUMENTS:
 *     OpenErrorStatus:  pointer to a var to return status info in
 *     SelectedMediumIndex: index of the selected medium (will be NdisMedium802_3)
 *     MediumArray: array of media that we can pick from
 *     MediumArraySize: size of MediumArray
 *     MiniportAdapterHandle: NDIS-assigned handle for this miniport instance
 *     WrapperConfigurationContext: temporary NDIS-assigned handle for passing
 *                                  to configuration APIs
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on success
 *     NDIS_STATUS_FAILURE on general failure
 *     NDIS_STATUS_UNSUPPORTED_MEDIA on not finding 802_3 in the MediaArray
 *     NDIS_STATUS_RESOURCES on insufficient system resources
 *     NDIS_STATUS_ADAPTER_NOT_FOUND on not finding the adapter
 * NOTES:
 *     - Called by NDIS at PASSIVE_LEVEL, once per detected card
 *     - Will int 3 on failure of MiTestCard if DBG=1
 */
{
  UINT i = 0;
  PADAPTER Adapter = 0;
  NDIS_STATUS Status = NDIS_STATUS_FAILURE;
  BOOLEAN InterruptRegistered = FALSE;

  /* Pick a medium */
  for(i = 0; i < MediumArraySize; i++)
    if(MediumArray[i] == NdisMedium802_3)
      break;

  if(i == MediumArraySize)
    {
      Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
      PCNET_DbgPrint(("unsupported media\n"));
      BREAKPOINT;
      *OpenErrorStatus = Status;
      return Status;
    }

  *SelectedMediumIndex = i;

  /* allocate our adapter struct */
  Status = NdisAllocateMemoryWithTag((PVOID *)&Adapter, sizeof(ADAPTER), PCNET_TAG);
  if(Status != NDIS_STATUS_SUCCESS)
    {
      Status =  NDIS_STATUS_RESOURCES;
      PCNET_DbgPrint(("Insufficient resources\n"));
      BREAKPOINT;
      *OpenErrorStatus = Status;
      return Status;
    }

  memset(Adapter,0,sizeof(ADAPTER));

  Adapter->MiniportAdapterHandle = MiniportAdapterHandle;

  /* register our adapter structwith ndis */
  NdisMSetAttributesEx(Adapter->MiniportAdapterHandle, Adapter, 0, NDIS_ATTRIBUTE_BUS_MASTER, NdisInterfacePci);

  do
    {
      /* get registry config */
      Status = MiGetConfig(Adapter, WrapperConfigurationContext);
      if(Status != NDIS_STATUS_SUCCESS)
        {
          PCNET_DbgPrint(("MiGetConfig failed\n"));
          Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
          BREAKPOINT;
          break;
        }

      /* Card-specific detection and setup */
      Status = MiQueryCard(Adapter);
      if(Status != NDIS_STATUS_SUCCESS)
        {
          PCNET_DbgPrint(("MiQueryCard failed\n"));
          Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
          BREAKPOINT;
          break;
        }

      /* register an IO port range */
      Status = NdisMRegisterIoPortRange(&Adapter->PortOffset, Adapter->MiniportAdapterHandle, 
          Adapter->IoBaseAddress, NUMBER_OF_PORTS);
      if(Status != NDIS_STATUS_SUCCESS)
        {
          PCNET_DbgPrint(("NdisMRegisterIoPortRange failed: 0x%x\n", Status));
          BREAKPOINT
          break;
        }

      /* Allocate map registers */
      Status = NdisMAllocateMapRegisters(Adapter->MiniportAdapterHandle, 0, 
          NDIS_DMA_32BITS, 8, BUFFER_SIZE);
      if(Status != NDIS_STATUS_SUCCESS)
        {
          PCNET_DbgPrint(("NdisMAllocateMapRegisters failed: 0x%x\n", Status));
          BREAKPOINT
          break;
        }

      /* set up the interrupt */
      memset(&Adapter->InterruptObject, 0, sizeof(NDIS_MINIPORT_INTERRUPT));
      Status = NdisMRegisterInterrupt(&Adapter->InterruptObject, Adapter->MiniportAdapterHandle, Adapter->InterruptVector,
          Adapter->InterruptVector, FALSE, TRUE, NdisInterruptLevelSensitive);
      if(Status != NDIS_STATUS_SUCCESS)
        {
          PCNET_DbgPrint(("NdisMRegisterInterrupt failed: 0x%x\n", Status));
          BREAKPOINT
          break;
        }

      InterruptRegistered = TRUE;

      /* Allocate and initialize shared data structures */
      Status = MiAllocateSharedMemory(Adapter);
      if(Status != NDIS_STATUS_SUCCESS)
        {
          Status = NDIS_STATUS_RESOURCES;
          PCNET_DbgPrint(("MiAllocateSharedMemory failed", Status));
          BREAKPOINT
          break;
        }

      /* set up the initialization block */
      MiPrepareInitializationBlock(Adapter);

      PCNET_DbgPrint(("Interrupt registered successfully\n"));

      /* Initialize and start the chip */
      MiInitChip(Adapter);

      Status = NDIS_STATUS_SUCCESS;
    }
  while(0);

  if(Status != NDIS_STATUS_SUCCESS && Adapter)
    {
      PCNET_DbgPrint(("Error; freeing stuff\n"));

      NdisMFreeMapRegisters(Adapter->MiniportAdapterHandle); /* doesn't hurt to free if we never alloc'd? */

      if(Adapter->PortOffset)
        NdisMDeregisterIoPortRange(Adapter->MiniportAdapterHandle, Adapter->IoBaseAddress, NUMBER_OF_PORTS, Adapter->PortOffset);

      if(InterruptRegistered)
        NdisMDeregisterInterrupt(&Adapter->InterruptObject);

      MiFreeSharedMemory(Adapter);

      NdisFreeMemory(Adapter, 0, 0);
    }

#if DBG
  if(!MiTestCard(Adapter))
    ASSERT(0);
#endif

  PCNET_DbgPrint(("returning 0x%x\n", Status));
  *OpenErrorStatus = Status;
  return Status;
}


VOID
STDCALL
MiniportISR(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueMiniportHandleInterrupt,
    IN NDIS_HANDLE MiniportAdapterContext)
/*
 * FUNCTION: Miniport interrupt service routine
 * ARGUMENTS:
 *     InterruptRecognized: the interrupt was ours
 *     QueueMiniportHandleInterrupt: whether to queue a DPC to handle this interrupt
 *     MiniportAdapterContext: the context originally passed to NdisMSetAttributes
 * NOTES:
 *     - called by NDIS at DIRQL
 *     - by setting QueueMiniportHandleInterrupt to TRUE, MiniportHandleInterrupt
 *       will be called
 */
{
  USHORT Data;
  USHORT Rap;
  PADAPTER Adapter = (PADAPTER)MiniportAdapterContext;

  PCNET_DbgPrint(("Called\n"));

  /* save the old RAP value */
  NdisRawReadPortUshort(Adapter->PortOffset + RAP, &Rap);

  /* is this ours? */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);

  if(!(Data & CSR0_INTR))
    {
      PCNET_DbgPrint(("not our interrupt.\n"));
      *InterruptRecognized = FALSE;
      *QueueMiniportHandleInterrupt = FALSE;
    }
  else
    {
      PCNET_DbgPrint(("detected our interrupt\n"));

      /* disable interrupts */
      NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
      NdisRawWritePortUshort(Adapter->PortOffset + RDP, 0);

      *InterruptRecognized = TRUE;
      *QueueMiniportHandleInterrupt = TRUE;
    }

  /* restore the rap */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, Rap);
}


NDIS_STATUS
STDCALL
MiniportReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext)
/*
 * FUNCTION: Reset the miniport
 * ARGUMENTS:
 *     AddressingReset: Whether or not we want NDIS to subsequently call MiniportSetInformation
 *                      to reset our addresses and filters
 *     MiniportAdapterContext: context originally passed to NdisMSetAttributes
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on all requests
 * Notes:
 *     - Called by NDIS at PASSIVE_LEVEL when it thinks we need a reset
 */
{
  PCNET_DbgPrint(("Called\n"));

  /* MiniportReset doesn't do anything at the moment... perhaps this should be fixed. */

  *AddressingReset = FALSE;
  return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
STDCALL
MiniportSend(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags)
/*
 * FUNCTION: Called by NDIS when it has a packet for the NIC to send out
 * ARGUMENTS:
 *     MiniportAdapterContext: context originally input to NdisMSetAttributes
 *     Packet: The NDIS_PACKET to be sent
 *     Flags: Flags associated with Packet
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on all requests
 * NOTES:
 *     - Called by NDIS at DISPATCH_LEVEL
 */
{
  PCNET_DbgPrint(("Called\n"));
  return NDIS_STATUS_SUCCESS;
}


NTSTATUS
STDCALL
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Start this driver
 * ARGUMENTS:
 *     DriverObject: Pointer to the system-allocated driver object
 *     RegistryPath: Pointer to our SCM database entry
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on success
 *     NDIS_STATUS_FAILURE on failure
 * NOTES:
 *     - Called by the I/O manager when the driver starts at PASSIVE_LEVEL
 *     - TODO: convert this to NTSTATUS return values
 */
{
  NDIS_HANDLE WrapperHandle;
  NDIS_MINIPORT_CHARACTERISTICS Characteristics;
  NDIS_STATUS Status;

  memset(&Characteristics, 0, sizeof(Characteristics));
  Characteristics.MajorNdisVersion = 4;
  Characteristics.HaltHandler = MiniportHalt;
  Characteristics.HandleInterruptHandler = MiniportHandleInterrupt;
  Characteristics.InitializeHandler = MiniportInitialize;
  Characteristics.ISRHandler = MiniportISR;
  Characteristics.QueryInformationHandler = MiniportQueryInformation;
  Characteristics.ResetHandler = MiniportReset;
  Characteristics.SetInformationHandler = MiniportSetInformation;
  Characteristics.u1.SendHandler = MiniportSend;

  NdisMInitializeWrapper(&WrapperHandle, DriverObject, RegistryPath, 0);

  Status = NdisMRegisterMiniport(WrapperHandle, &Characteristics, sizeof(Characteristics));
  if(Status != NDIS_STATUS_SUCCESS)
    {
      NdisTerminateWrapper(WrapperHandle, 0);
      return NDIS_STATUS_FAILURE;
    }

  return NDIS_STATUS_SUCCESS;
}

