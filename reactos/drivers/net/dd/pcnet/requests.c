/*
 * ReactOS AMD PCNet Driver
 *
 * Copyright (C) 2000 Casper Hornstroup <chorns@users.sourceforge.net>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * PROGRAMMERS:
 *     Vizzini (vizzini@plasmic.com), 
 *     borrowed very heavily from the ReactOS ne2000 driver by 
 *     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *     14-Sept-2003 vizzini - Created
 */

#include <ndis.h>
#include "pcnethw.h"
#include "pcnet.h"

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
  OID_802_3_PERMANENT_ADDRESS,
  OID_802_3_CURRENT_ADDRESS,
  OID_802_3_MULTICAST_LIST,
  OID_802_3_MAXIMUM_LIST_SIZE,
  OID_802_3_MAC_OPTIONS
};


NDIS_STATUS
STDCALL
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
 *     - Called by NDIS at PASSIVE_LEVEL
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

  PCNET_DbgPrint(("Called. OID 0x%x\n", Oid));

  ASSERT(Adapter);

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
          /* TODO: implement this... */
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
          /* XXX I'm not sure this is correct */
          GenericULONG = 1514;
          break;
        }

    case OID_GEN_LINK_SPEED:
        {     
          GenericULONG = 100000;  /* 10Mbps */
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
          GenericULONG = 0x1022;
          break;
        }

    case OID_GEN_VENDOR_DESCRIPTION:
        {
          /* XXX implement me */
          CopyFrom = 0;
          CopySize = 0;
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
          GenericULONG = 1;
          break;
        }

    case OID_GEN_MAXIMUM_TOTAL_SIZE:
        {
          GenericULONG = 1514;
          break;
        }

    case OID_GEN_PROTOCOL_OPTIONS:
        {
          PCNET_DbgPrint(("OID_GEN_PROTOCOL_OPTIONS.\n"));
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
          GenericULONG = (ULONG)NdisMediaStateConnected;
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

    case OID_802_3_MULTICAST_LIST:
        {
          /* XXX Implement me */
          PCNET_DbgPrint(("OID_802_3_MULTICAST_LIST.\n"));
          Status = NDIS_STATUS_NOT_SUPPORTED;
          break;
        }

    case OID_802_3_MAXIMUM_LIST_SIZE:
        {
          /* XXX Implement me */
          GenericULONG = 0;
          break;
        }

    case OID_802_3_MAC_OPTIONS:
        {
          /* XXX Implement me */
          PCNET_DbgPrint(("OID_802_3_MAC_OPTIONS.\n"));
          Status = NDIS_STATUS_NOT_SUPPORTED;
          break;
        }
      
    default:
        {
          PCNET_DbgPrint(("Unknown OID\n"));
          Status = NDIS_STATUS_INVALID_OID;
          break;
        }
    }

  if (Status == NDIS_STATUS_SUCCESS) 
    {
      if (CopySize > InformationBufferLength) 
        {
          *BytesNeeded  = (CopySize - InformationBufferLength);
          *BytesWritten = 0;
          Status        = NDIS_STATUS_BUFFER_TOO_SHORT;
        } 
      else 
        {
          NdisMoveMemory(InformationBuffer, CopyFrom, CopySize);
          *BytesWritten = CopySize;
          *BytesNeeded  = 0;
         }
    }

  PCNET_DbgPrint(("Leaving. Status is 0x%x\n", Status));

  return Status;
}

NDIS_STATUS
STDCALL
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
 *     - Called by NDIS at PASSIVE_LEVEL
 *     - verify buffer space as mentioned in previous function notes
 */
{
  ULONG GenericULONG;
  NDIS_STATUS Status   = NDIS_STATUS_SUCCESS;
  PADAPTER Adapter = (PADAPTER)MiniportAdapterContext;

  ASSERT(Adapter);

  PCNET_DbgPrint(("Called, OID 0x%x\n", Oid));

  switch (Oid) 
    {
    case OID_GEN_CURRENT_PACKET_FILTER:
      {
        /* Verify length */
        if (InformationBufferLength < sizeof(ULONG)) 
          {
            *BytesRead   = 0;
            *BytesNeeded = sizeof(ULONG) - InformationBufferLength;
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
            *BytesRead   = 4;
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
            *BytesNeeded = sizeof(ULONG) - InformationBufferLength;
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
          }

        NdisMoveMemory(&GenericULONG, InformationBuffer, sizeof(ULONG));

        if (GenericULONG > 1500)
          Status = NDIS_STATUS_INVALID_LENGTH;
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
            *BytesNeeded = 0;
            Status       = NDIS_STATUS_INVALID_LENGTH;
            break;
          }

        /* Set new multicast address list */
        //NdisMoveMemory(Adapter->Addresses, InformationBuffer, InformationBufferLength);

        /* FIXME: Update hardware */

        break;
      }

    default:
      {
        PCNET_DbgPrint(("Invalid object ID (0x%X).\n", Oid));
        *BytesRead   = 0;
        *BytesNeeded = 0;
        Status       = NDIS_STATUS_INVALID_OID;
        break;
      }
    }

  if (Status == NDIS_STATUS_SUCCESS) 
    {
      *BytesRead   = InformationBufferLength;
      *BytesNeeded = 0;
    }

  PCNET_DbgPrint(("Leaving. Status (0x%X).\n", Status));

  return Status;
}

