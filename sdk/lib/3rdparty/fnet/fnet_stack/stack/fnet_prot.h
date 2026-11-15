/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community
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
*  Private. Transport protocol interface definitions.
*
***************************************************************************/

#ifndef _FNET_PROT_H_

#define _FNET_PROT_H_

#include "fnet_socket_prv.h"

/************************************************************************
*    Protocol numbers.
*************************************************************************/
#define FNET_PROT_ICMP4  (1U)
#define FNET_PROT_IGMP   (2U)
#define FNET_PROT_UDP    (17U)
#define FNET_PROT_TCP    (6U)
#define FNET_PROT_ICMP6  (58U)

/************************************************************************
*    Protocol notify commands.
*************************************************************************/
typedef enum
{
    FNET_PROT_NOTIFY_QUENCH,           /* Some one said to slow down.*/
    FNET_PROT_NOTIFY_MSGSIZE,          /* Message size forced drop.*/
    FNET_PROT_NOTIFY_UNREACH_HOST,     /* No route to host.*/
    FNET_PROT_NOTIFY_UNREACH_PROTOCOL, /* Dst says bad protocol.*/
    FNET_PROT_NOTIFY_UNREACH_PORT,     /* Bad port #.*/
    FNET_PROT_NOTIFY_UNREACH_SRCFAIL,  /* Source route failed.*/
    FNET_PROT_NOTIFY_UNREACH_NET,      /* No route to network.*/
    FNET_PROT_NOTIFY_TIMXCEED_INTRANS, /* Packet time-to-live expired in transit.*/
    FNET_PROT_NOTIFY_TIMXCEED_REASS,   /* Reassembly time-to-leave expired.*/
    FNET_PROT_NOTIFY_PARAMPROB         /* Header incorrect.*/
} fnet_prot_notify_t;

struct fnet_netif; /* Forward declaration.*/

/************************************************************************
*    Transport Layer Protocol interface control structure.
*************************************************************************/
typedef struct fnet_prot_if
{
    fnet_socket_if_t        *head;                      /* Pointer to the head of the protocol's socket list.*/
    fnet_address_family_t   family;                     /* Address domain family.*/
    fnet_socket_type_t      type;                       /* Socket type used for.*/
    fnet_uint32_t           protocol;                   /* Protocol number.*/
    fnet_return_t           (*prot_init)( void );       /* (Optional) Protocol initialization function.*/
    void                    (*prot_release)( void );    /* (Optional) Protocol release function.*/
    void                    (*prot_input)(fnet_netif_t *netif, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb, fnet_netbuf_t *ip_nb); /* Protocol input function.*/
    void                    (*prot_control_input)(fnet_prot_notify_t command, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb);  /* (Optional) Protocol input control function.*/
    void                    (*prot_drain)( void );      /* (Optional) Protocol drain function. */
    const fnet_socket_prot_if_t *socket_api;            /* (Optional) Pointer to Socket API structure.*/
} fnet_prot_if_t;

/************************************************************************
*     Function Prototypes.
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

fnet_return_t _fnet_prot_init( void );
void _fnet_prot_release( void );
fnet_prot_if_t *_fnet_prot_find( fnet_address_family_t family, fnet_socket_type_t type, fnet_uint32_t protocol );
void _fnet_prot_drain( void );

#if defined(__cplusplus)
}
#endif

#endif
