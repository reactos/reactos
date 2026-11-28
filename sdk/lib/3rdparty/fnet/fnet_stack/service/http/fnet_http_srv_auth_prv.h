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
*  Private. FNET HTTP Server Authentication API.
*
***************************************************************************/

#ifndef _FNET_HTTP_SRV_AUTH_PRV_H_

#define _FNET_HTTP_SRV_AUTH_PRV_H_

#include "fnet.h"

#if FNET_CFG_HTTP_SRV && FNET_CFG_HTTP_SRV_AUTHENTICATION_BASIC && FNET_CFG_HTTP_SRV_VERSION_MAJOR

#include "fnet_http_srv_auth.h"

typedef fnet_return_t(*fnet_http_srv_auth_scheme_validate_t)(const fnet_http_srv_auth_t *auth_entry, fnet_char_t *auth_param);
typedef fnet_size_t(*fnet_http_srv_auth_scheme_generate_t)(struct fnet_http_srv_if *http, fnet_uint8_t *buffer, fnet_size_t buffer_size);

struct fnet_http_srv_auth_scheme_if
{
    fnet_http_srv_auth_scheme_t             id;
    fnet_char_t                             *name;
    fnet_http_srv_auth_scheme_validate_t    validate; /* Validate credentials params.*/
    fnet_http_srv_auth_scheme_generate_t    generate; /* Generate challenge params.*/
};

#if defined(__cplusplus)
extern "C" {
#endif

void _fnet_http_srv_auth_validate_uri(struct fnet_http_srv_if *http);
fnet_return_t _fnet_http_srv_auth_validate_credentials(struct fnet_http_srv_if *http, fnet_char_t *credentials);
fnet_size_t _fnet_http_srv_auth_generate_challenge(struct fnet_http_srv_if *http, fnet_uint8_t *buffer, fnet_size_t buffer_size);

#if defined(__cplusplus)
}
#endif

#endif

#endif
