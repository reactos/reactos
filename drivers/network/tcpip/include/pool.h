/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * PURPOSE:     Prototypes for memory pooling
 */

#pragma once

NDIS_STATUS PrependPacket( PNDIS_PACKET Packet, PCHAR Data, UINT Len,
			   BOOLEAN Copy );

/* EOF */
