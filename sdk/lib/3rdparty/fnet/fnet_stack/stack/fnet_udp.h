/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community.
*
***************************************************************************
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may
*  not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
*  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
***************************************************************************
*
*  Private. UDP protocol definitions.
*
***************************************************************************/

#ifndef _FNET_UDP_H_

#define _FNET_UDP_H_

#if FNET_CFG_UDP

#include "fnet.h"
#include "fnet_socket.h"
#include "fnet_socket_prv.h"
#include "fnet_prot.h"

/************************************************************************
*     UDP definitions
*************************************************************************/
#define FNET_UDP_TTL            (64u)                       /* Default TTL.*/
#define FNET_UDP_TTL_MULTICAST  (1u)                        /* Default TTL for Multicast datagrams.
                                                             * RFC112 6.1: If the upper-layer protocol
                                                             * chooses not to specify a time-to-live, it should
                                                             * default to 1 for all multicast IP datagrams, so that an explicit
                                                             * choice is required to multicast beyond a single network.
                                                             */
#define FNET_UDP_DF             (FNET_FALSE)                       /* DF flag.*/
#define FNET_UDP_TX_BUF_MAX     (FNET_CFG_SOCKET_UDP_TX_BUF_SIZE) /* Default maximum size for send socket buffer.*/
#define FNET_UDP_RX_BUF_MAX     (FNET_CFG_SOCKET_UDP_RX_BUF_SIZE) /* Default maximum size for receive socket buffer.*/

/************************************************************************
*     Global Data Structures
*************************************************************************/
extern fnet_prot_if_t fnet_udp_prot_if;

/* Structure of UDP header.*/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint16_t source_port FNET_COMP_PACKED;      /* Source port number.*/
    fnet_uint16_t destination_port FNET_COMP_PACKED; /* Destination port number.*/
    fnet_uint16_t length FNET_COMP_PACKED;           /* Length.*/
    fnet_uint16_t checksum FNET_COMP_PACKED;         /* Checksum.*/
} fnet_udp_header_t;
FNET_COMP_PACKED_END


#endif  /* FNET_CFG_UDP */

#endif /* _FNET_UDP_H_ */
