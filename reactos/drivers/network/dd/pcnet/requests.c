/*
 * ReactOS AMD PCNet Driver
 *
 * Copyright (C) 2000 Casper Hornstrup <chorns@users.sourceforge.net>
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
 * PROGRAMMERS:
 *     Vizzini (vizzini@plasmic.com),
 *     borrowed very heavily from the ReactOS ne2000 driver by
 *     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *     14-Sep-2003 vizzini - Created
 *     17-Oct-2004 navaraf - Add multicast support.
 *                         - Add media state detection support.
 *                         - Protect the adapter context with spinlock
 *                           and move code talking to card to inside
 *                           NdisMSynchronizeWithInterrupt calls where
 *                           necessary.
 */

#include "pcnet.h"

#define NDEBUG
#include <debug.h>

/* List of supported OIDs */
static ULONG MiniportOIDList[] =
{
  OID_GEN_SUPPORTED_LIST,
  OID_GEN_HARDWARE_STATUS,
  OID_GEN_MEDIA_SUPPORTED,
  OID_GEN_MEDIA_IN_USE,
  OID_GEN_MAXIMUM_LOOKAHEAD,
  OID_GEN_MAXIMUM_FRAME_SIZE,
  OID_GEN_LINK_SPEED,
  OID_GEN_TRANSMIT_BUFFER_SPACE,
  OID_GEN_RECEIVE_BUFFER_SPACE,
  OID_GEN_TRANSMIT_BLOCK_SIZE,
  OID_GEN_RECEIVE_BLOCK_SIZE,
  OID_GEN_VENDOR_ID,
  OID_GEN_VENDOR_DESCRIPTION,
  OID_GEN_VENDOR_DRIVER_VERSION,
  OID_GEN_CURRENT_PACKET_FILTER,
  OID_GEN_CURRENT_LOOKAHEAD,
  OID_GEN_DRIVER_VERSION,
  OID_GEN_MAXIMUM_TOTAL_SIZE,
  OID_GEN_PROTOCOL_OPTIONS,
  OID_GEN_MAC_OPTIONS,
  OID_GEN_MEDIA_CONNECT_STATUS,
  OID_GEN_MAXIMUM_SEND_PACKETS,
  OID_GEN_XMIT_OK,
  OID_GEN_RCV_OK,
  OID_GEN_XMIT_ERROR,
  OID_GEN_RCV_ERROR,
  OID_GEN_RCV_NO_BUFFER,
  OID_GEN_RCV_CRC_ERROR,
  OID_802_3_PERMANENT_ADDRESS,
  OID_802_3_CURRENT_ADDRESS,
  OID_802_3_MULTICAST_LIST,
  OID_802_3_MAXIMUM_LIST_SIZE,
  OID_802_3_MAC_OPTIONS,
  OID_802_3_RCV_ERROR_ALIGNMENT,
  OID_802_3_XMIT_ONE_COLLISION,
  OID_802_3_XMIT_MORE_COLLISIONS
};


NDIS_STATUS
NTAPI
MiniportQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded)
/*
 * FUNCTION: Query an OID from the driver
 * ARGUMENTS:
 *     MiniportAdapterContext: context originally passed to NdisMSetAttributes
 *     Oid: OID NDIS is querying
 *     InformationBuffer: pointer to buffer into which to write the results of the query
 *     InformationBufferLength: size in bytes of InformationBuffer
 *     BytesWritten: number of bytes written into InformationBuffer in response to the query
 *     BytesNeeded: number of bytes needed to answer the query
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on all queries
 * NOTES:
 *     - Called by NDIS at DISPATCH_LEVEL
 *     - If InformationBufferLength is insufficient to store the results, return the amount
 *       needed in BytesNeeded and return NDIS_STATUS_INVALID_LENGTH
 * TODO:
 *     - Update to verify input buffer & size and return insufficient buffer codes
 */
{
  NDIS_STATUS Status;
  PVOID CopyFrom;
  UINT CopySize;
  ULONG GenericULONG;
  PADAPTER Adapter = (PADAPTER)MiniportAdapterContext;

  DPRINT("Called. OID 0x%x\n", Oid);

  ASSERT(Adapter);

  NdisAcquireSpinLock(&Adapter->Lock);

  Status   = NDIS_STATUS_SUCCESS;
  CopyFrom = (PVOID)&GenericULONG;
  CopySize = sizeof(ULONG);

  switch (Oid)
    {
    case OID_GEN_SUPPORTED_LIST:
        {
          CopyFrom = (PVOID)&MiniportOIDList;
          CopySize = sizeof(MiniportOIDList);
          break;
        }

    case OID_GEN_HARDWARE_STATUS:
        {
          GenericULONG = (ULONG)NdisHardwareStatusReady;
          break;
        }

    case OID_GEN_MEDIA_SUPPORTED:
    case OID_GEN_MEDIA_IN_USE:
        {
          static const NDIS_MEDIUM Medium = NdisMedium802_3;
          CopyFrom = (PVOID)&Medium;
          CopySize = sizeof(NDIS_MEDIUM);
          break;
        }

    case OID_GEN_CURRENT_LOOKAHEAD:
    case OID_GEN_MAXIMUM_LOOKAHEAD:
        {
          GenericULONG = 1500;
          break;
        }

    case OID_GEN_MAXIMUM_FRAME_SIZE:
        {
          /*
           * The value returned by this OID must be equal to
           * OID_GEN_MAXIMUM_TOTAL_SIZE - sizeof(ETHERNET_HEADER)
           * where sizeof(ETHERNET_HEADER) is 14.
           */
          GenericULONG = 1500;
          break;
        }

    case OID_GEN_LINK_SPEED:
        {
          GenericULONG = Adapter->MediaSpeed * 10000;
          break;
        }

    case OID_GEN_TRANSMIT_BUFFER_SPACE:
        {
          /* XXX fix me */
          GenericULONG = BUFFER_SIZE;
          break;
        }

    case OID_GEN_RECEIVE_BUFFER_SPACE:
        {
          /* XXX fix me */
          GenericULONG = BUFFER_SIZE;
          break;
        }

    case OID_GEN_TRANSMIT_BLOCK_SIZE:
        {
          GenericULONG = BUFFER_SIZE;
          break;
        }

    case OID_GEN_RECEIVE_BLOCK_SIZE:
        {
          GenericULONG = BUFFER_SIZE;
          break;
        }

    case OID_GEN_VENDOR_ID:
        {
          UCHAR *CharPtr = (UCHAR *)&GenericULONG;
          GenericULONG = 0;
          /* Read the first three bytes of the permanent MAC address */
          NdisRawReadPortUchar(Adapter->PortOffset, CharPtr);
          NdisRawReadPortUchar(Adapter->PortOffset + 1, CharPtr + 1);
          NdisRawReadPortUchar(Adapter->PortOffset + 2, CharPtr + 2);
          break;
        }

    case OID_GEN_VENDOR_DESCRIPTION:
        {
          static UCHAR VendorDesc[] = "ReactOS Team";
          CopyFrom = VendorDesc;
          CopySize = sizeof(VendorDesc);
          break;
        }

    case OID_GEN_VENDOR_DRIVER_VERSION:
        {
          /* XXX implement me */
          GenericULONG = 1;
          break;
        }

    case OID_GEN_CURRENT_PACKET_FILTER:
        {
          GenericULONG = Adapter->CurrentPacketFilter;
          break;
        }

    case OID_GEN_DRIVER_VERSION:
        {
          /* NDIS version used by the driver. */
          static const USHORT DriverVersion =
            (NDIS_MINIPORT_MAJOR_VERSION << 8) + NDIS_MINIPORT_MINOR_VERSION;
          CopyFrom = (PVOID)&DriverVersion;
          CopySize = sizeof(DriverVersion);
          break;
        }

    case OID_GEN_MAXIMUM_TOTAL_SIZE:
        {
          /* See comment in OID_GEN_MAXIMUM_FRAME_SIZE. */
          GenericULONG = 1514;
          break;
        }

    case OID_GEN_PROTOCOL_OPTIONS:
        {
          DPRINT("OID_GEN_PROTOCOL_OPTIONS.\n");
          Status = NDIS_STATUS_NOT_SUPPORTED;
          break;
        }

    case OID_GEN_MAC_OPTIONS:
        {
          GenericULONG = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
                         NDIS_MAC_OPTION_RECEIVE_SERIALIZED  |
                         NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  |
                         NDIS_MAC_OPTION_NO_LOOPBACK;
          break;
        }

    case OID_GEN_MEDIA_CONNECT_STATUS:
        {
          GenericULONG = (ULONG)NdisMediaStateConnected; /* Adapter->MediaState */
          break;
        }

    case OID_GEN_MAXIMUM_SEND_PACKETS:
        {
          GenericULONG = 1;
          break;
        }

    case OID_802_3_CURRENT_ADDRESS:
    case OID_802_3_PERMANENT_ADDRESS:
        {
          CopyFrom = (PVOID)&Adapter->InitializationBlockVirt->PADR;
          CopySize = 6;
          break;
        }

    case OID_802_3_MAXIMUM_LIST_SIZE:
        {
          GenericULONG = MAX_MULTICAST_ADDRESSES;
          break;
        }

    case OID_GEN_XMIT_OK:
        GenericULONG = Adapter->Statistics.XmtGoodFrames;
        break;

    case OID_GEN_RCV_OK:
        GenericULONG = Adapter->Statistics.RcvGoodFrames;
        break;

    case OID_GEN_XMIT_ERROR:
        GenericULONG = Adapter->Statistics.XmtRetryErrors +
                       Adapter->Statistics.XmtLossesOfCarrier +
                       Adapter->Statistics.XmtCollisions +
                       Adapter->Statistics.XmtLateCollisions +
                       Adapter->Statistics.XmtExcessiveDeferrals +
                       Adapter->Statistics.XmtBufferUnderflows +
                       Adapter->Statistics.XmtBufferErrors;
        break;

    case OID_GEN_RCV_ERROR:
        GenericULONG = Adapter->Statistics.RcvBufferErrors +
                       Adapter->Statistics.RcvCrcErrors +
                       Adapter->Statistics.RcvOverflowErrors +
                       Adapter->Statistics.RcvFramingErrors;
        break;

    case OID_GEN_RCV_NO_BUFFER:
        GenericULONG = Adapter->Statistics.RcvBufferErrors +
                       Adapter->Statistics.RcvOverflowErrors;
        break;

    case OID_GEN_RCV_CRC_ERROR:
        GenericULONG = Adapter->Statistics.RcvCrcErrors;
        break;

    case OID_802_3_RCV_ERROR_ALIGNMENT:
        GenericULONG = Adapter->Statistics.RcvFramingErrors;
        break;

    case OID_802_3_XMIT_ONE_COLLISION:
        GenericULONG = Adapter->Statistics.XmtOneRetry;
        break;

    case OID_802_3_XMIT_MORE_COLLISIONS:
        GenericULONG = Adapter->Statistics.XmtMoreThanOneRetry;
        break;

    default:
        {
          DPRINT1("Unknown OID\n");
          Status = NDIS_STATUS_NOT_SUPPORTED;
          break;
        }
    }

  if (Status == NDIS_STATUS_SUCCESS)
    {
      if (CopySize > InformationBufferLength)
        {
          *BytesNeeded = CopySize;
          *BytesWritten = 0;
          Status        = NDIS_STATUS_INVALID_LENGTH;
        }
      else
        {
          NdisMoveMemory(InformationBuffer, CopyFrom, CopySize);
          *BytesWritten = CopySize;
          *BytesNeeded  = CopySize;
         }
    }
   else
    {
       *BytesWritten = 0;
       *BytesNeeded = 0;
    }

  NdisReleaseSpinLock(&Adapter->Lock);

  DPRINT("Leaving. Status is 0x%x\n", Status);

  return Status;
}

NDIS_STATUS
NTAPI
MiniportSetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded)
/*
 * FUNCTION: Set a miniport variable (OID)
 * ARGUMENTS:
 *     MiniportAdapterContext: context originally passed into NdisMSetAttributes
 *     Oid: the variable being set
 *     InformationBuffer: the data to set the variable to
 *     InformationBufferLength: number of bytes in InformationBuffer
 *     BytesRead: number of bytes read by us out of the buffer
 *     BytesNeeded: number of bytes required to satisfy the request if InformationBufferLength
 *                  is insufficient
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on all requests
 * NOTES:
 *     - Called by NDIS at DISPATCH_LEVEL
 *     - verify buffer space as mentioned in previous function notes
 */
{
  ULONG GenericULONG;
  NDIS_STATUS Status   = NDIS_STATUS_SUCCESS;
  PADAPTER Adapter = (PADAPTER)MiniportAdapterContext;

  ASSERT(Adapter);

  DPRINT("Called, OID 0x%x\n", Oid);

  NdisAcquireSpinLock(&Adapter->Lock);

  switch (Oid)
    {
    case OID_GEN_CURRENT_PACKET_FILTER:
      {
        /* Verify length */
        if (InformationBufferLength < sizeof(ULONG))
          {
            *BytesRead   = 0;
            *BytesNeeded = sizeof(ULONG);
            Status       = NDIS_STATUS_INVALID_LENGTH;
            break;
          }

        NdisMoveMemory(&GenericULONG, InformationBuffer, sizeof(ULONG));

        /* Check for properties the driver don't support */
        if (GenericULONG &
           (NDIS_PACKET_TYPE_ALL_FUNCTIONAL |
            NDIS_PACKET_TYPE_FUNCTIONAL     |
            NDIS_PACKET_TYPE_GROUP          |
            NDIS_PACKET_TYPE_MAC_FRAME      |
            NDIS_PACKET_TYPE_SMT            |
            NDIS_PACKET_TYPE_SOURCE_ROUTING)
           )
          {
            *BytesRead   = sizeof(ULONG);
            *BytesNeeded = 0;
            Status       = NDIS_STATUS_NOT_SUPPORTED;
            break;
          }

        Adapter->CurrentPacketFilter = GenericULONG;

        /* FIXME: Set filter on hardware */

        break;
      }

    case OID_GEN_CURRENT_LOOKAHEAD:
      {
        /* Verify length */
        if (InformationBufferLength < sizeof(ULONG))
          {
            *BytesRead   = 0;
            *BytesNeeded = sizeof(ULONG);
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
          }

        NdisMoveMemory(&GenericULONG, InformationBuffer, sizeof(ULONG));

        if (GenericULONG > 1500)
          Status = NDIS_STATUS_INVALID_DATA;
        else
          Adapter->CurrentLookaheadSize = GenericULONG;

        break;
      }

    case OID_802_3_MULTICAST_LIST:
      {
        /* Verify length. Must be multiple of hardware address length */
        if ((InformationBufferLength % 6) != 0)
          {
            *BytesRead   = 0;
            *BytesNeeded = InformationBufferLength + (InformationBufferLength % 6);
            Status       = NDIS_STATUS_INVALID_LENGTH;
            break;
          }

        ASSERT((InformationBufferLength / 6) <= MAX_MULTICAST_ADDRESSES);

        /* Set new multicast address list */
        //NdisMoveMemory(Adapter->Addresses, InformationBuffer, InformationBufferLength);

        /* Update hardware */
        Status = MiSetMulticast(Adapter, InformationBuffer, InformationBufferLength / 6);

        break;
      }

    default:
      {
        DPRINT1("Invalid object ID (0x%X).\n", Oid);
        *BytesRead   = 0;
        *BytesNeeded = 0;
        Status       = NDIS_STATUS_NOT_SUPPORTED;
        break;
      }
    }

  if (Status == NDIS_STATUS_SUCCESS)
    {
      *BytesRead   = InformationBufferLength;
      *BytesNeeded = 0;
    }

  NdisReleaseSpinLock(&Adapter->Lock);

  DPRINT("Leaving. Status (0x%X).\n", Status);

  return Status;
}
