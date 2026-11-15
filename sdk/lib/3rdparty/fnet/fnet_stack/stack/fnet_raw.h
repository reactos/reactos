/**************************************************************************
*
* Copyright 2011-2018 by Andrey Butok. FNET Community.
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
*  RAW socket definitions.
*
***************************************************************************/

#ifndef _FNET_RAW_H_

#define _FNET_RAW_H_

#if FNET_CFG_RAW

#include "fnet.h"
#include "fnet_socket.h"
#include "fnet_socket_prv.h"
#include "fnet_prot.h"

/************************************************************************
*     RAW definitions
*************************************************************************/
#define FNET_RAW_TTL            (64u)                             /* Default TTL for RAW socket.*/
#define FNET_RAW_TX_BUF_MAX     (FNET_CFG_SOCKET_RAW_TX_BUF_SIZE) /* Default maximum size for send socket buffer.*/
#define FNET_RAW_RX_BUF_MAX     (FNET_CFG_SOCKET_RAW_RX_BUF_SIZE) /* Default maximum size for receive socket buffer.*/

/************************************************************************
*     Global Data Structures
*************************************************************************/
extern fnet_prot_if_t fnet_raw_prot_if;

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif
void _fnet_raw_input(fnet_netif_t *netif, struct fnet_sockaddr *foreign_addr,  struct fnet_sockaddr *local_addr, fnet_netbuf_t *nb, fnet_netbuf_t *ip_nb);
#if defined(__cplusplus)
}
#endif

#endif  /* FNET_CFG_RAW */

#endif /* _FNET_RAW_H_ */
