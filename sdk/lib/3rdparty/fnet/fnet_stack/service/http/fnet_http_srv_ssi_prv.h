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
*  Private. FNET HTTP Server SSI API.
*
***************************************************************************/

#ifndef _FNET_HTTP_SRV_SSI_PRV_H_

#define _FNET_HTTP_SRV_SSI_PRV_H_

#include "fnet.h"

#if FNET_CFG_HTTP_SRV && FNET_CFG_HTTP_SRV_SSI

/* SSI statemachine state. */
typedef enum
{
    FNET_HTTP_SRV_SSI_WAIT_HEAD = 0,
    FNET_HTTP_SRV_SSI_WAIT_TAIL,
    FNET_HTTP_SRV_SSI_INCLUDING
}
fnet_http_srv_ssi_state_t;

/* SSI private control structure. */
struct fnet_http_srv_ssi_if
{
    const fnet_http_srv_ssi_t  *ssi_table; /* Pointer to the SSI table.*/
    fnet_http_srv_ssi_send_t   send;    /* Pointer to the respond callback.*/
    fnet_http_srv_ssi_state_t  state;        /* State. */
};

extern const struct fnet_http_srv_file_handler fnet_http_srv_ssi_handler; /* SSI file handler. */

#endif

#endif
