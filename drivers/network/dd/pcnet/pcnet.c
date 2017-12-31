/*
 * ReactOS AMD PCNet Driver
 *
 * Copyright (C) 2003 Vizzini <vizzini@plasmic.com>
 * Copyright (C) 2004 Filip Navara <navaraf@reactos.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * REVISIONS:
 *     09-Sep-2003 vizzini - Created
 *     10-Oct-2004 navaraf - Fix receive to work on VMware adapters (
 *                           need to set busmaster bit on PCI).
 *                         - Indicate receive completion.
 *                         - Implement packet transmitting.
 *                         - Don't read slot number from registry and
 *                           report itself as NDIS 5.0 miniport.
 *     11-Oct-2004 navaraf - Fix nasty bugs in halt code path.
 *     17-Oct-2004 navaraf - Add multicast support.
 *                         - Add media state detection support.
 *                         - Protect the adapter context with spinlock
 *                           and move code talking to card to inside
 *                           NdisMSynchronizeWithInterrupt calls where
 *                           necessary.
 *
 * NOTES:
 *     - this assumes a 32-bit machine
 */

#include "pcnet.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath);

static VOID
NTAPI
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
  UINT i = 0;

  DPRINT("Called\n");

  ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

  NdisDprAcquireSpinLock(&Adapter->Lock);

  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);

  DPRINT("CSR0 is 0x%x\n", Data);

  while((Data & CSR0_INTR) && i++ < INTERRUPT_LIMIT)
    {
      /* Clear interrupt flags early to avoid race conditions. */
      NdisRawWritePortUshort(Adapter->PortOffset + RDP, Data);

      if(Data & CSR0_ERR)
        {
          DPRINT("error: %x\n", Data & (CSR0_MERR|CSR0_BABL|CSR0_CERR|CSR0_MISS));
          if (Data & CSR0_CERR)
            Adapter->Statistics.XmtCollisions++;
        }
      if(Data & CSR0_IDON)
        {
          DPRINT("IDON\n");
        }
      if(Data & CSR0_RINT)
        {
          BOOLEAN IndicatedData = FALSE;

          DPRINT("receive interrupt\n");

          while(1)
            {
              PRECEIVE_DESCRIPTOR Descriptor = Adapter->ReceiveDescriptorRingVirt + Adapter->CurrentReceiveDescriptorIndex;
              PCHAR Buffer;
              ULONG ByteCount;

              if(Descriptor->FLAGS & RD_OWN)
                {
                  DPRINT("no more receive descriptors to process\n");
                  break;
                }

              if(Descriptor->FLAGS & RD_ERR)
                {
                  DPRINT("receive descriptor error: 0x%x\n", Descriptor->FLAGS);
                  if (Descriptor->FLAGS & RD_BUFF)
                    Adapter->Statistics.RcvBufferErrors++;
                  if (Descriptor->FLAGS & RD_CRC)
                    Adapter->Statistics.RcvCrcErrors++;
                  if (Descriptor->FLAGS & RD_OFLO)
                    Adapter->Statistics.RcvOverflowErrors++;
                  if (Descriptor->FLAGS & RD_FRAM)
                    Adapter->Statistics.RcvFramingErrors++;
                  break;
                }

              if(!((Descriptor->FLAGS & RD_STP) && (Descriptor->FLAGS & RD_ENP)))
                {
                  DPRINT("receive descriptor not start&end: 0x%x\n", Descriptor->FLAGS);
                  break;
                }

              Buffer = Adapter->ReceiveBufferPtrVirt + Adapter->CurrentReceiveDescriptorIndex * BUFFER_SIZE;
              ByteCount = Descriptor->MCNT & 0xfff;

              DPRINT("Indicating a %d-byte packet (index %d)\n", ByteCount, Adapter->CurrentReceiveDescriptorIndex);

              NdisMEthIndicateReceive(Adapter->MiniportAdapterHandle, 0, Buffer, 14, Buffer+14, ByteCount-14, ByteCount-14);

              IndicatedData = TRUE;

              RtlZeroMemory(Descriptor, sizeof(RECEIVE_DESCRIPTOR));
              Descriptor->RBADR = Adapter->ReceiveBufferPtrPhys.QuadPart +
                                  (Adapter->CurrentReceiveDescriptorIndex * BUFFER_SIZE);
              Descriptor->BCNT = (-BUFFER_SIZE) | 0xf000;
              Descriptor->FLAGS |= RD_OWN;

              Adapter->CurrentReceiveDescriptorIndex++;
              Adapter->CurrentReceiveDescriptorIndex %= Adapter->BufferCount;

              Adapter->Statistics.RcvGoodFrames++;
            }

            if (IndicatedData)
                NdisMEthIndicateReceiveComplete(Adapter->MiniportAdapterHandle);
        }
      if(Data & CSR0_TINT)
        {
          PTRANSMIT_DESCRIPTOR Descriptor;

          DPRINT("transmit interrupt\n");

          while (Adapter->CurrentTransmitStartIndex !=
                 Adapter->CurrentTransmitEndIndex)
            {
              Descriptor = Adapter->TransmitDescriptorRingVirt + Adapter->CurrentTransmitStartIndex;

              DPRINT("buffer %d flags %x flags2 %x\n",
                     Adapter->CurrentTransmitStartIndex,
                     Descriptor->FLAGS, Descriptor->FLAGS2);

              if (Descriptor->FLAGS & TD1_OWN)
                {
                  DPRINT("non-TXed buffer\n");
                  break;
                }

              if (Descriptor->FLAGS & TD1_STP)
                {
                  if (Descriptor->FLAGS & TD1_ONE)
                    Adapter->Statistics.XmtOneRetry++;
                  else if (Descriptor->FLAGS & TD1_MORE)
                    Adapter->Statistics.XmtMoreThanOneRetry++;
                }

              if (Descriptor->FLAGS & TD1_ERR)
                {
                  DPRINT("major error: %x\n", Descriptor->FLAGS2);
                  if (Descriptor->FLAGS2 & TD2_RTRY)
                    Adapter->Statistics.XmtRetryErrors++;
                  if (Descriptor->FLAGS2 & TD2_LCAR)
                    Adapter->Statistics.XmtLossesOfCarrier++;
                  if (Descriptor->FLAGS2 & TD2_LCOL)
                    Adapter->Statistics.XmtLateCollisions++;
                  if (Descriptor->FLAGS2 & TD2_EXDEF)
                    Adapter->Statistics.XmtExcessiveDeferrals++;
                  if (Descriptor->FLAGS2 & TD2_UFLO)
                    Adapter->Statistics.XmtBufferUnderflows++;
                  if (Descriptor->FLAGS2 & TD2_BUFF)
                    Adapter->Statistics.XmtBufferErrors++;
                  break;
                }

              Adapter->CurrentTransmitStartIndex++;
              Adapter->CurrentTransmitStartIndex %= Adapter->BufferCount;

              Adapter->Statistics.XmtGoodFrames++;
            }
          NdisMSendResourcesAvailable(Adapter->MiniportAdapterHandle);
        }
      if(Data & ~(CSR0_ERR | CSR0_IDON | CSR0_RINT | CSR0_TINT))
        {
          DPRINT("UNHANDLED INTERRUPT CSR0 0x%x\n", Data);
        }

      NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
    }

  /* re-enable interrupts */
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_IENA);

  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  DPRINT("CSR0 is now 0x%x\n", Data);

  NdisDprReleaseSpinLock(&Adapter->Lock);
}

static NDIS_STATUS
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
  Status = NdisReadPciSlotInformation(Adapter->MiniportAdapterHandle, 0, PCI_PCIID, &buf32, 4);
  if(Status != 4)
    {
      Status =  NDIS_STATUS_FAILURE;
      DPRINT1("NdisReadPciSlotInformation failed\n");
      return Status;
    }

  if(buf32 != PCI_ID)
    {
      Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
      DPRINT1("card in slot isn't our: 0x%x\n", 0, buf32);
      return Status;
    }

  /* set busmaster and io space enable bits */
  buf32 = PCI_BMEN | PCI_IOEN;
  NdisWritePciSlotInformation(Adapter->MiniportAdapterHandle, 0, PCI_COMMAND, &buf32, 4);

  /* get IO base physical address */
  buf32 = 0;
  Status = NdisReadPciSlotInformation(Adapter->MiniportAdapterHandle, 0, PCI_IOBAR, &buf32, 4);
  if(Status != 4)
    {
      Status = NDIS_STATUS_FAILURE;
      DPRINT1("NdisReadPciSlotInformation failed\n");
      return Status;
    }

  if(!buf32)
    {
      DPRINT1("No base i/o address set\n");
      return NDIS_STATUS_FAILURE;
    }

  buf32 &= ~1;  /* even up address - comes out odd for some reason */

  DPRINT("detected io address 0x%x\n", buf32);
  Adapter->IoBaseAddress = buf32;

  /* get interrupt vector */
  Status = NdisReadPciSlotInformation(Adapter->MiniportAdapterHandle, 0, PCI_ILR, &buf8, 1);
  if(Status != 1)
    {
      Status = NDIS_STATUS_FAILURE;
      DPRINT1("NdisReadPciSlotInformation failed\n");
      return Status;
    }

  DPRINT("interrupt: 0x%x\n", buf8);
  Adapter->InterruptVector = buf8;

  return NDIS_STATUS_SUCCESS;
}

static VOID
MiFreeSharedMemory(
    PADAPTER Adapter)
/*
 * FUNCTION: Free all allocated shared memory
 * ARGUMENTS:
 *     Adapter: pointer to the miniport's adapter struct
 */
{
  NDIS_PHYSICAL_ADDRESS PhysicalAddress;

  if(Adapter->InitializationBlockVirt)
    {
      PhysicalAddress = Adapter->InitializationBlockPhys;
      NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle, Adapter->InitializationBlockLength,
          FALSE, Adapter->InitializationBlockVirt, PhysicalAddress);
      Adapter->InitializationBlockVirt = NULL;
    }

  if(Adapter->TransmitDescriptorRingVirt)
    {
      PhysicalAddress = Adapter->TransmitDescriptorRingPhys;
      NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle, Adapter->TransmitDescriptorRingLength,
        FALSE, Adapter->TransmitDescriptorRingVirt, PhysicalAddress);
      Adapter->TransmitDescriptorRingVirt = NULL;
    }

  if(Adapter->ReceiveDescriptorRingVirt)
    {
      PhysicalAddress = Adapter->ReceiveDescriptorRingPhys;
      NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle, Adapter->ReceiveDescriptorRingLength,
          FALSE, Adapter->ReceiveDescriptorRingVirt, PhysicalAddress);
      Adapter->ReceiveDescriptorRingVirt = NULL;
    }

  if(Adapter->TransmitBufferPtrVirt)
    {
      PhysicalAddress = Adapter->TransmitBufferPtrPhys;
      NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle, Adapter->TransmitBufferLength,
          TRUE, Adapter->TransmitBufferPtrVirt, PhysicalAddress);
      Adapter->TransmitBufferPtrVirt = NULL;
    }

  if(Adapter->ReceiveBufferPtrVirt)
    {
      PhysicalAddress = Adapter->ReceiveBufferPtrPhys;
      NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle, Adapter->ReceiveBufferLength,
          TRUE, Adapter->ReceiveBufferPtrVirt, PhysicalAddress);
      Adapter->ReceiveBufferPtrVirt = NULL;
    }
}

static NDIS_STATUS
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
  ULONG BufferCount = NUMBER_OF_BUFFERS;
  ULONG LogBufferCount = LOG_NUMBER_OF_BUFFERS;

  while (BufferCount != 0)
  {
      /* allocate the initialization block (we have this in the loop so we can use MiFreeSharedMemory) */
      Adapter->InitializationBlockLength = sizeof(INITIALIZATION_BLOCK);
      NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle, Adapter->InitializationBlockLength,
          FALSE, (PVOID *)&Adapter->InitializationBlockVirt, &PhysicalAddress);
      if(!Adapter->InitializationBlockVirt)
      {
         /* Buffer backoff won't help us here */
         DPRINT1("insufficient resources\n");
         return NDIS_STATUS_RESOURCES;
      }

      if (((ULONG_PTR)Adapter->InitializationBlockVirt & 0x00000003) != 0)
      {
         DPRINT1("address 0x%x not dword-aligned\n", Adapter->InitializationBlockVirt);
         return NDIS_STATUS_RESOURCES;
      }

      Adapter->InitializationBlockPhys = PhysicalAddress;

      /* allocate the transport descriptor ring */
      Adapter->TransmitDescriptorRingLength = sizeof(TRANSMIT_DESCRIPTOR) * BufferCount;
      NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle, Adapter->TransmitDescriptorRingLength,
          FALSE, (PVOID *)&Adapter->TransmitDescriptorRingVirt, &PhysicalAddress);
      if (!Adapter->TransmitDescriptorRingVirt)
      {
          DPRINT1("Backing off buffer count by %d buffers due to allocation failure\n", (BufferCount >> 1));
          BufferCount = BufferCount >> 1;
          LogBufferCount--;
          MiFreeSharedMemory(Adapter);
          continue;
      }

      if (((ULONG_PTR)Adapter->TransmitDescriptorRingVirt & 0x00000003) != 0)
      {
         DPRINT1("address 0x%x not dword-aligned\n", Adapter->TransmitDescriptorRingVirt);
         return NDIS_STATUS_RESOURCES;
      }

      Adapter->TransmitDescriptorRingPhys = PhysicalAddress;
      RtlZeroMemory(Adapter->TransmitDescriptorRingVirt, sizeof(TRANSMIT_DESCRIPTOR) * BufferCount);

      /* allocate the receive descriptor ring */
      Adapter->ReceiveDescriptorRingLength = sizeof(RECEIVE_DESCRIPTOR) * BufferCount;
      NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle, Adapter->ReceiveDescriptorRingLength,
          FALSE, (PVOID *)&Adapter->ReceiveDescriptorRingVirt, &PhysicalAddress);
      if (!Adapter->ReceiveDescriptorRingVirt)
      {
          DPRINT1("Backing off buffer count by %d buffers due to allocation failure\n", (BufferCount >> 1));
          BufferCount = BufferCount >> 1;
          LogBufferCount--;
          MiFreeSharedMemory(Adapter);
          continue;
      }

      if (((ULONG_PTR)Adapter->ReceiveDescriptorRingVirt & 0x00000003) != 0)
      {
          DPRINT1("address 0x%x not dword-aligned\n", Adapter->ReceiveDescriptorRingVirt);
          return NDIS_STATUS_RESOURCES;
      }

      Adapter->ReceiveDescriptorRingPhys = PhysicalAddress;
      RtlZeroMemory(Adapter->ReceiveDescriptorRingVirt, sizeof(RECEIVE_DESCRIPTOR) * BufferCount);

      /* allocate transmit buffers */
      Adapter->TransmitBufferLength = BUFFER_SIZE * BufferCount;
      NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle, Adapter->TransmitBufferLength,
         TRUE, (PVOID *)&Adapter->TransmitBufferPtrVirt, &PhysicalAddress);
      if(!Adapter->TransmitBufferPtrVirt)
      {
          DPRINT1("Backing off buffer count by %d buffers due to allocation failure\n", (BufferCount >> 1));
          BufferCount = BufferCount >> 1;
          LogBufferCount--;
          MiFreeSharedMemory(Adapter);
          continue;
      }

      if(((ULONG_PTR)Adapter->TransmitBufferPtrVirt & 0x00000003) != 0)
      {
          DPRINT1("address 0x%x not dword-aligned\n", Adapter->TransmitBufferPtrVirt);
          return NDIS_STATUS_RESOURCES;
      }

      Adapter->TransmitBufferPtrPhys = PhysicalAddress;
      RtlZeroMemory(Adapter->TransmitBufferPtrVirt, BUFFER_SIZE * BufferCount);

      /* allocate receive buffers */
      Adapter->ReceiveBufferLength = BUFFER_SIZE * BufferCount;
      NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle, Adapter->ReceiveBufferLength,
         TRUE, (PVOID *)&Adapter->ReceiveBufferPtrVirt, &PhysicalAddress);
      if(!Adapter->ReceiveBufferPtrVirt)
      {
          DPRINT1("Backing off buffer count by %d buffers due to allocation failure\n", (BufferCount >> 1));
          BufferCount = BufferCount >> 1;
          LogBufferCount--;
          MiFreeSharedMemory(Adapter);
          continue;
      }

      if (((ULONG_PTR)Adapter->ReceiveBufferPtrVirt & 0x00000003) != 0)
      {
          DPRINT1("address 0x%x not dword-aligned\n", Adapter->ReceiveBufferPtrVirt);
          return NDIS_STATUS_RESOURCES;
      }

      Adapter->ReceiveBufferPtrPhys = PhysicalAddress;
      RtlZeroMemory(Adapter->ReceiveBufferPtrVirt, BUFFER_SIZE * BufferCount);

      break;
  }

  if (!BufferCount)
  {
      DPRINT1("Failed to allocate adapter buffers\n");
      return NDIS_STATUS_RESOURCES;
  }

  Adapter->BufferCount = BufferCount;
  Adapter->LogBufferCount = LogBufferCount;

  /* initialize tx descriptors */
  TransmitDescriptor = Adapter->TransmitDescriptorRingVirt;
  for(i = 0; i < BufferCount; i++)
    {
      (TransmitDescriptor+i)->TBADR = Adapter->TransmitBufferPtrPhys.QuadPart + i * BUFFER_SIZE;
      (TransmitDescriptor+i)->BCNT = 0xf000 | -BUFFER_SIZE; /* 2's compliment  + set top 4 bits */
      (TransmitDescriptor+i)->FLAGS = TD1_STP | TD1_ENP;
    }

  DPRINT("transmit ring initialized\n");

  /* initialize rx */
  ReceiveDescriptor = Adapter->ReceiveDescriptorRingVirt;
  for(i = 0; i < BufferCount; i++)
    {
      (ReceiveDescriptor+i)->RBADR = Adapter->ReceiveBufferPtrPhys.QuadPart + i * BUFFER_SIZE;
      (ReceiveDescriptor+i)->BCNT = 0xf000 | -BUFFER_SIZE; /* 2's compliment  + set top 4 bits */
      (ReceiveDescriptor+i)->FLAGS = RD_OWN;
    }

  DPRINT("receive ring initialized\n");

  return NDIS_STATUS_SUCCESS;
}

static VOID
MiPrepareInitializationBlock(
    PADAPTER Adapter)
/*
 * FUNCTION: Initialize the initialization block
 * ARGUMENTS:
 *     Adapter: pointer to the miniport's adapter object
 */
{
  ULONG i = 0;

  RtlZeroMemory(Adapter->InitializationBlockVirt, sizeof(INITIALIZATION_BLOCK));

  /* read burned-in address from card */
  for(i = 0; i < 6; i++)
    NdisRawReadPortUchar(Adapter->PortOffset + i, Adapter->InitializationBlockVirt->PADR + i);
  DPRINT("MAC address: %02x-%02x-%02x-%02x-%02x-%02x\n",
         Adapter->InitializationBlockVirt->PADR[0],
         Adapter->InitializationBlockVirt->PADR[1],
         Adapter->InitializationBlockVirt->PADR[2],
         Adapter->InitializationBlockVirt->PADR[3],
         Adapter->InitializationBlockVirt->PADR[4],
         Adapter->InitializationBlockVirt->PADR[5]);

  /* set up receive ring */
  DPRINT("Receive ring physical address: 0x%x\n", Adapter->ReceiveDescriptorRingPhys);
  Adapter->InitializationBlockVirt->RDRA = Adapter->ReceiveDescriptorRingPhys.QuadPart;
  Adapter->InitializationBlockVirt->RLEN = (Adapter->LogBufferCount << 4) & 0xf0;

  /* set up transmit ring */
  DPRINT("Transmit ring physical address: 0x%x\n", Adapter->TransmitDescriptorRingPhys);
  Adapter->InitializationBlockVirt->TDRA = Adapter->TransmitDescriptorRingPhys.QuadPart;
  Adapter->InitializationBlockVirt->TLEN = (Adapter->LogBufferCount << 4) & 0xf0;
}

static BOOLEAN
NTAPI
MiSyncStop(
    IN PVOID SynchronizeContext)
/*
 * FUNCTION: Stop the adapter
 * ARGUMENTS:
 *     SynchronizeContext: Adapter context
 */
{
  PADAPTER Adapter = (PADAPTER)SynchronizeContext;
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_STOP);
  return TRUE;
}

static VOID
NTAPI
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
  PADAPTER Adapter = (PADAPTER)MiniportAdapterContext;
  BOOLEAN TimerCancelled;

  DPRINT("Called\n");
  ASSERT(Adapter);

  /* stop the media detection timer */
  NdisMCancelTimer(&Adapter->MediaDetectionTimer, &TimerCancelled);

  /* stop the chip */
  NdisMSynchronizeWithInterrupt(&Adapter->InterruptObject, MiSyncStop, Adapter);

  /* deregister the interrupt */
  NdisMDeregisterInterrupt(&Adapter->InterruptObject);

  /* deregister i/o port range */
  NdisMDeregisterIoPortRange(Adapter->MiniportAdapterHandle, Adapter->IoBaseAddress, NUMBER_OF_PORTS, (PVOID)Adapter->PortOffset);

  /* deregister the shutdown routine */
  NdisMDeregisterAdapterShutdownHandler(Adapter->MiniportAdapterHandle);

  /* free shared memory */
  MiFreeSharedMemory(Adapter);

  /* free map registers */
  NdisMFreeMapRegisters(Adapter->MiniportAdapterHandle);

  /* free the lock */
  NdisFreeSpinLock(&Adapter->Lock);

  /* free the adapter */
  NdisFreeMemory(Adapter, 0, 0);
}

static BOOLEAN
NTAPI
MiSyncMediaDetection(
    IN PVOID SynchronizeContext)
/*
 * FUNCTION: Stop the adapter
 * ARGUMENTS:
 *     SynchronizeContext: Adapter context
 */
{
  PADAPTER Adapter = (PADAPTER)SynchronizeContext;
  NDIS_MEDIA_STATE MediaState = MiGetMediaState(Adapter);
  UINT MediaSpeed = MiGetMediaSpeed(Adapter);
  BOOLEAN FullDuplex = MiGetMediaDuplex(Adapter);

  DPRINT("Called\n");
  DPRINT("MediaState: %d\n", MediaState);
  if (MediaState != Adapter->MediaState ||
      MediaSpeed != Adapter->MediaSpeed ||
      FullDuplex != Adapter->FullDuplex)
    {
      Adapter->MediaState = MediaState;
      Adapter->MediaSpeed = MediaSpeed;
      Adapter->FullDuplex = FullDuplex;
      return TRUE;
    }
  return FALSE;
}

static VOID
NTAPI
MiniportMediaDetectionTimer(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3)
/*
 * FUNCTION: Periodically query media state
 * ARGUMENTS:
 *     FunctionContext: Adapter context
 * NOTES:
 *     - Called by NDIS at DISPATCH_LEVEL
 */
{
  PADAPTER Adapter = (PADAPTER)FunctionContext;

  ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

  if (NdisMSynchronizeWithInterrupt(&Adapter->InterruptObject,
                                    MiSyncMediaDetection,
                                    FunctionContext))
    {
      NdisMIndicateStatus(Adapter->MiniportAdapterHandle,
        Adapter->MediaState == NdisMediaStateConnected ?
        NDIS_STATUS_MEDIA_CONNECT : NDIS_STATUS_MEDIA_DISCONNECT,
        (PVOID)0, 0);
      NdisMIndicateStatusComplete(Adapter->MiniportAdapterHandle);
    }
}

static VOID
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

  DPRINT("Called\n");

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

  DPRINT("chip stopped\n");

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
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, (USHORT)(Adapter->InitializationBlockPhys.LowPart & 0xffff));
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR2);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, (USHORT)(Adapter->InitializationBlockPhys.LowPart >> 16) & 0xffff);

  DPRINT("programmed with init block\n");

  /* Set mode to 0 */
  Data = 0;
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR15);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, Data);

  /* load init block and start the card */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_STRT|CSR0_INIT|CSR0_IENA);

  /* Allow LED programming */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, BCR2);
  NdisRawWritePortUshort(Adapter->PortOffset + BDP, BCR2_LEDPE);

  /* LED0 is configured for link status (on = up, off = down) */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, BCR4);
  NdisRawWritePortUshort(Adapter->PortOffset + BDP, BCR4_LNKSTE | BCR4_PSE);

  /* LED1 is configured for link duplex (on = full, off = half) */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, BCR5);
  NdisRawWritePortUshort(Adapter->PortOffset + BDP, BCR5_FDLSE | BCR5_PSE);

  /* LED2 is configured for link speed (on = 100M, off = 10M) */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, BCR6);
  NdisRawWritePortUshort(Adapter->PortOffset + BDP, BCR6_E100 | BCR6_PSE);

  /* LED3 is configured for trasmit/receive activity */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, BCR7);
  NdisRawWritePortUshort(Adapter->PortOffset + BDP, BCR7_XMTE | BCR7_RCVE | BCR7_PSE);

  Adapter->MediaState = MiGetMediaState(Adapter);
  Adapter->FullDuplex = MiGetMediaDuplex(Adapter);
  Adapter->MediaSpeed = MiGetMediaSpeed(Adapter);

  DPRINT("card started\n");

  Adapter->Flags &= ~RESET_IN_PROGRESS;
}

#if DBG
static BOOLEAN
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
  DPRINT("Port 0x%x RAP 0x%x CSR0 0x%x RDP 0x%x, Interrupt status register is 0x%x\n", Adapter->PortOffset, RAP, CSR0, RDP, Data);

  /* read the BIA */
  for(i = 0; i < 6; i++)
      NdisRawReadPortUchar(Adapter->PortOffset + i, &address[i]);

  DPRINT("burned-in address: %02x:%02x:%02x:%02x:%02x:%02x\n", address[0], address[1], address[2], address[3], address[4], address[5]);
  /* Read status flags from CSR0 */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  DPRINT("CSR0: 0x%x\n", Data);

  /* Read status flags from CSR3 */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR3);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  DPRINT("CSR3: 0x%x\n", Data);
  
  /* Read status flags from CSR4 */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR4);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  DPRINT("CSR4: 0x%x\n", Data);

  /* Read status flags from CSR5 */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR5);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  DPRINT("CSR5: 0x%x\n", Data);

  /* Read status flags from CSR6 */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR6);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);
  DPRINT("CSR6: 0x%x\n", Data);
  
  /* Read status flags from BCR4 */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, BCR4);
  NdisRawReadPortUshort(Adapter->PortOffset + BDP, &Data);
  DPRINT("BCR4: 0x%x\n", Data);  

  return TRUE;
}
#endif

VOID
NTAPI
MiniportShutdown( PVOID Context )
{
  PADAPTER Adapter = Context;

  DPRINT("Stopping the chip\n");

  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_STOP);
}

static NDIS_STATUS
NTAPI
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
  BOOLEAN InterruptRegistered = FALSE, MapRegistersAllocated = FALSE;
  NDIS_HANDLE ConfigurationHandle;
  UINT *RegNetworkAddress = 0;
  UINT RegNetworkAddressLength = 0;

  ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

  /* Pick a medium */
  for(i = 0; i < MediumArraySize; i++)
    if(MediumArray[i] == NdisMedium802_3)
      break;

  if(i == MediumArraySize)
    {
      Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
      DPRINT1("unsupported media\n");
      *OpenErrorStatus = Status;
      return Status;
    }

  *SelectedMediumIndex = i;

  /* allocate our adapter struct */
  Status = NdisAllocateMemoryWithTag((PVOID *)&Adapter, sizeof(ADAPTER), PCNET_TAG);
  if(Status != NDIS_STATUS_SUCCESS)
    {
      Status =  NDIS_STATUS_RESOURCES;
      DPRINT1("Insufficient resources\n");
      *OpenErrorStatus = Status;
      return Status;
    }

  RtlZeroMemory(Adapter, sizeof(ADAPTER));

  Adapter->MiniportAdapterHandle = MiniportAdapterHandle;

  /* register our adapter structwith ndis */
  NdisMSetAttributesEx(Adapter->MiniportAdapterHandle, Adapter, 0, NDIS_ATTRIBUTE_BUS_MASTER, NdisInterfacePci);

  do
    {
      /* Card-specific detection and setup */
      Status = MiQueryCard(Adapter);
      if(Status != NDIS_STATUS_SUCCESS)
        {
          DPRINT1("MiQueryCard failed\n");
          Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
          break;
        }

      /* register an IO port range */
      Status = NdisMRegisterIoPortRange((PVOID*)&Adapter->PortOffset, Adapter->MiniportAdapterHandle,
          (UINT)Adapter->IoBaseAddress, NUMBER_OF_PORTS);
      if(Status != NDIS_STATUS_SUCCESS)
        {
          DPRINT1("NdisMRegisterIoPortRange failed: 0x%x\n", Status);
          break;
        }

      /* Allocate map registers */
      Status = NdisMAllocateMapRegisters(Adapter->MiniportAdapterHandle, 0,
          NDIS_DMA_32BITS, 8, BUFFER_SIZE);
      if(Status != NDIS_STATUS_SUCCESS)
        {
          DPRINT1("NdisMAllocateMapRegisters failed: 0x%x\n", Status);
          break;
        }

      MapRegistersAllocated = TRUE;

      /* set up the interrupt */
      Status = NdisMRegisterInterrupt(&Adapter->InterruptObject, Adapter->MiniportAdapterHandle, Adapter->InterruptVector,
          Adapter->InterruptVector, TRUE, TRUE, NdisInterruptLevelSensitive);
      if(Status != NDIS_STATUS_SUCCESS)
        {
          DPRINT1("NdisMRegisterInterrupt failed: 0x%x\n", Status);
          break;
        }

      InterruptRegistered = TRUE;

      /* Allocate and initialize shared data structures */
      Status = MiAllocateSharedMemory(Adapter);
      if(Status != NDIS_STATUS_SUCCESS)
        {
          Status = NDIS_STATUS_RESOURCES;
          DPRINT1("MiAllocateSharedMemory failed\n", Status);
          break;
        }

      /* set up the initialization block */
      MiPrepareInitializationBlock(Adapter);

      /* see if someone set a network address manually */
      NdisOpenConfiguration(&Status, &ConfigurationHandle, WrapperConfigurationContext);
      if (Status == NDIS_STATUS_SUCCESS)
      {
         NdisReadNetworkAddress(&Status, (PVOID *)&RegNetworkAddress, &RegNetworkAddressLength, ConfigurationHandle);
         if(Status == NDIS_STATUS_SUCCESS && RegNetworkAddressLength == 6)
         {
             int i;
             DPRINT("NdisReadNetworkAddress returned successfully, address %x:%x:%x:%x:%x:%x\n",
                     RegNetworkAddress[0], RegNetworkAddress[1], RegNetworkAddress[2], RegNetworkAddress[3],
                     RegNetworkAddress[4], RegNetworkAddress[5]);

             for(i = 0; i < 6; i++)
                 Adapter->InitializationBlockVirt->PADR[i] = RegNetworkAddress[i];
         }

         NdisCloseConfiguration(ConfigurationHandle);
      }

      DPRINT("Interrupt registered successfully\n");

      /* Initialize and start the chip */
      MiInitChip(Adapter);

      NdisAllocateSpinLock(&Adapter->Lock);

      Status = NDIS_STATUS_SUCCESS;
    }
  while(0);

  if(Status != NDIS_STATUS_SUCCESS && Adapter)
    {
      DPRINT("Error; freeing stuff\n");

      MiFreeSharedMemory(Adapter);

      if(MapRegistersAllocated)
        NdisMFreeMapRegisters(Adapter->MiniportAdapterHandle);

      if(Adapter->PortOffset)
        NdisMDeregisterIoPortRange(Adapter->MiniportAdapterHandle, Adapter->IoBaseAddress, NUMBER_OF_PORTS, (PVOID)Adapter->PortOffset);

      if(InterruptRegistered)
        NdisMDeregisterInterrupt(&Adapter->InterruptObject);

      NdisFreeMemory(Adapter, 0, 0);
    }

  if(Status == NDIS_STATUS_SUCCESS)
    {
      NdisMInitializeTimer(&Adapter->MediaDetectionTimer,
                           Adapter->MiniportAdapterHandle,
                           MiniportMediaDetectionTimer,
                           Adapter);
      NdisMSetPeriodicTimer(&Adapter->MediaDetectionTimer,
                            MEDIA_DETECTION_INTERVAL);
      NdisMRegisterAdapterShutdownHandler(Adapter->MiniportAdapterHandle,
                                          Adapter,
                                          MiniportShutdown);
    }

#if DBG
  if(!MiTestCard(Adapter))
    ASSERT(0);
#endif

  DPRINT("returning 0x%x\n", Status);
  *OpenErrorStatus = Status;
  return Status;
}

static VOID
NTAPI
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

  DPRINT("Called\n");

  /* save the old RAP value */
  NdisRawReadPortUshort(Adapter->PortOffset + RAP, &Rap);

  /* is this ours? */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawReadPortUshort(Adapter->PortOffset + RDP, &Data);

  if(!(Data & CSR0_INTR))
    {
      DPRINT("not our interrupt.\n");
      *InterruptRecognized = FALSE;
      *QueueMiniportHandleInterrupt = FALSE;
    }
  else
    {
      DPRINT("detected our interrupt\n");

      /* disable interrupts */
      NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
      NdisRawWritePortUshort(Adapter->PortOffset + RDP, 0);

      *InterruptRecognized = TRUE;
      *QueueMiniportHandleInterrupt = TRUE;
    }

  /* restore the rap */
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, Rap);
}

static NDIS_STATUS
NTAPI
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
  DPRINT("Called\n");

  /* MiniportReset doesn't do anything at the moment... perhaps this should be fixed. */

  *AddressingReset = FALSE;
  return NDIS_STATUS_SUCCESS;
}

static BOOLEAN
NTAPI
MiSyncStartTransmit(
    IN PVOID SynchronizeContext)
/*
 * FUNCTION: Stop the adapter
 * ARGUMENTS:
 *     SynchronizeContext: Adapter context
 */
{
  PADAPTER Adapter = (PADAPTER)SynchronizeContext;
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, CSR0);
  NdisRawWritePortUshort(Adapter->PortOffset + RDP, CSR0_IENA | CSR0_TDMD);
  return TRUE;
}

static NDIS_STATUS
NTAPI
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
 *     NDIS_STATUS_SUCCESS on processed requests
 *     NDIS_STATUS_RESOURCES if there's no place in buffer ring
 * NOTES:
 *     - Called by NDIS at DISPATCH_LEVEL
 */
{
  PADAPTER Adapter = (PADAPTER)MiniportAdapterContext;
  PTRANSMIT_DESCRIPTOR Desc;
  PNDIS_BUFFER NdisBuffer;
  PVOID SourceBuffer;
  UINT TotalPacketLength, SourceLength, Position = 0;

  DPRINT("Called\n");

  ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

  NdisDprAcquireSpinLock(&Adapter->Lock);

  /* Check if we have free entry in our circular buffer. */
  if ((Adapter->CurrentTransmitEndIndex + 1 ==
       Adapter->CurrentTransmitStartIndex) ||
      (Adapter->CurrentTransmitEndIndex == Adapter->BufferCount - 1 &&
       Adapter->CurrentTransmitStartIndex == 0))
    {
      DPRINT1("No free space in circular buffer\n");
      NdisDprReleaseSpinLock(&Adapter->Lock);
      return NDIS_STATUS_RESOURCES;
    }

  Desc = Adapter->TransmitDescriptorRingVirt + Adapter->CurrentTransmitEndIndex;

  NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, &TotalPacketLength);
  ASSERT(TotalPacketLength <= BUFFER_SIZE);

  DPRINT("TotalPacketLength: %x\n", TotalPacketLength);

  while (NdisBuffer)
    {
      NdisQueryBuffer(NdisBuffer, &SourceBuffer, &SourceLength);

      DPRINT("Buffer: %x Length: %x\n", SourceBuffer, SourceLength);

      RtlCopyMemory(Adapter->TransmitBufferPtrVirt +
                    Adapter->CurrentTransmitEndIndex * BUFFER_SIZE + Position,
                    SourceBuffer, SourceLength);

      Position += SourceLength;

      NdisGetNextBuffer(NdisBuffer, &NdisBuffer);
    }

#if DBG && 0
  {
    PUCHAR Ptr = Adapter->TransmitBufferPtrVirt +
                 Adapter->CurrentTransmitEndIndex * BUFFER_SIZE;
    for (Position = 0; Position < TotalPacketLength; Position++)
      {
        if (Position % 16 == 0)
          DbgPrint("\n");
        DbgPrint("%x ", *Ptr++);
      }
  }
  DbgPrint("\n");
#endif

  Adapter->CurrentTransmitEndIndex++;
  Adapter->CurrentTransmitEndIndex %= Adapter->BufferCount;

  Desc->FLAGS = TD1_OWN | TD1_STP | TD1_ENP;
  Desc->BCNT = 0xf000 | -(INT)TotalPacketLength;

  NdisMSynchronizeWithInterrupt(&Adapter->InterruptObject, MiSyncStartTransmit, Adapter);

  NdisDprReleaseSpinLock(&Adapter->Lock);

  return NDIS_STATUS_SUCCESS;
}

static ULONG
NTAPI
MiEthernetCrc(UCHAR *Address)
/*
 * FUNCTION: Calculate Ethernet CRC32
 * ARGUMENTS:
 *     Address: 6-byte ethernet address
 * RETURNS:
 *     The calculated CRC32 value.
 */
{
  UINT Counter, Length;
  ULONG Value = ~0;

  for (Length = 0; Length < 6; Length++)
    {
      Value ^= *Address++;
      for (Counter = 0; Counter < 8; Counter++)
        {
          Value >>= 1;
          Value ^= (Value & 1) * 0xedb88320;
        }
    }

  return Value;
}

NDIS_STATUS
NTAPI
MiSetMulticast(
    PADAPTER Adapter,
    UCHAR *Addresses,
    UINT AddressCount)
{
  UINT Index;
  ULONG CrcIndex;

  NdisZeroMemory(Adapter->InitializationBlockVirt->LADR, 8);
  for (Index = 0; Index < AddressCount; Index++)
    {
      CrcIndex = MiEthernetCrc(Addresses) >> 26;
      Adapter->InitializationBlockVirt->LADR[CrcIndex >> 3] |= 1 << (CrcIndex & 15);
      Addresses += 6;
    }

  /* FIXME: The specification mentions we need to reload the init block here. */

  return NDIS_STATUS_SUCCESS;
}

BOOLEAN
NTAPI
MiGetMediaDuplex(PADAPTER Adapter)
{
  ULONG Data;

  NdisRawWritePortUshort(Adapter->PortOffset + RAP, BCR5);
  NdisRawReadPortUshort(Adapter->PortOffset + BDP, &Data);

  return (Data & BCR5_LEDOUT) != 0;
}

UINT
NTAPI
MiGetMediaSpeed(PADAPTER Adapter)
{
  ULONG Data;

  NdisRawWritePortUshort(Adapter->PortOffset + RAP, BCR6);
  NdisRawReadPortUshort(Adapter->PortOffset + BDP, &Data);

  return Data & BCR6_LEDOUT ? 100 : 10;
}

NDIS_MEDIA_STATE
NTAPI
MiGetMediaState(PADAPTER Adapter)
/*
 * FUNCTION: Determine the link state
 * ARGUMENTS:
 *     Adapter: Adapter context
 * RETURNS:
 *     NdisMediaStateConnected if the cable is connected
 *     NdisMediaStateDisconnected if the cable is disconnected
 */
{
  ULONG Data;
  NdisRawWritePortUshort(Adapter->PortOffset + RAP, BCR4);
  NdisRawReadPortUshort(Adapter->PortOffset + BDP, &Data);
  return Data & BCR4_LEDOUT ? NdisMediaStateConnected : NdisMediaStateDisconnected;
}

NTSTATUS
NTAPI
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

  RtlZeroMemory(&Characteristics, sizeof(Characteristics));
  Characteristics.MajorNdisVersion = NDIS_MINIPORT_MAJOR_VERSION;
  Characteristics.MinorNdisVersion = NDIS_MINIPORT_MINOR_VERSION;
  Characteristics.HaltHandler = MiniportHalt;
  Characteristics.HandleInterruptHandler = MiniportHandleInterrupt;
  Characteristics.InitializeHandler = MiniportInitialize;
  Characteristics.ISRHandler = MiniportISR;
  Characteristics.QueryInformationHandler = MiniportQueryInformation;
  Characteristics.ResetHandler = MiniportReset;
  Characteristics.SetInformationHandler = MiniportSetInformation;
  Characteristics.SendHandler = MiniportSend;

  NdisMInitializeWrapper(&WrapperHandle, DriverObject, RegistryPath, 0);
  if (!WrapperHandle) return NDIS_STATUS_FAILURE;

  Status = NdisMRegisterMiniport(WrapperHandle, &Characteristics, sizeof(Characteristics));
  if(Status != NDIS_STATUS_SUCCESS)
    {
      NdisTerminateWrapper(WrapperHandle, 0);
      return NDIS_STATUS_FAILURE;
    }

  return NDIS_STATUS_SUCCESS;
}
