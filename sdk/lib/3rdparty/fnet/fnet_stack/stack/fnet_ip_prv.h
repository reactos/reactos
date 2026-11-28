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
*  Private. IP protocol API.
*
***************************************************************************/

#ifndef _FNET_IP_PRV_H_

#define _FNET_IP_PRV_H_

#include "fnet.h"
#include "fnet_netif_prv.h"
#include "fnet_socket_prv.h"

/************************************************************************
*    Definitions.
*************************************************************************/
/* Maximum size of IP input queue.*/
#define FNET_IP_QUEUE_COUNT_MAX    (FNET_CFG_IP_MAX_PACKET/2U)

typedef struct                              /* IP input queue.*/
{
    fnet_netbuf_t   *head;                  /* Pointer to the queue head.*/
    fnet_size_t     count;                  /* Number of data in buffer.*/
} fnet_ip_queue_t;

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

fnet_return_t _fnet_ip_queue_append( fnet_ip_queue_t *queue, fnet_netif_t *netif, fnet_netbuf_t *nb );
fnet_netbuf_t *_fnet_ip_queue_read( fnet_ip_queue_t *queue, fnet_netif_t **netif );
fnet_return_t _fnet_ip_setsockopt( fnet_socket_if_t *sock, fnet_protocol_t level, fnet_socket_options_t optname, const void *optval, fnet_size_t optlen );
fnet_return_t _fnet_ip_getsockopt( fnet_socket_if_t *sock, fnet_protocol_t level, fnet_socket_options_t optname, void *optval, fnet_size_t *optlen );

#if defined(__cplusplus)
}
#endif

#endif /* _FNET_IP_PRV_H_ */
