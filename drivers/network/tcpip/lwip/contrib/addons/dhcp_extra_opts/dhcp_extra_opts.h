/*
 * Copyright (c) Espressif Systems (Shanghai) CO LTD
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * To use these additional DHCP options, make sure this file is included in LWIP_HOOK_FILENAME
 * and define these hooks:
 *
 * #define LWIP_HOOK_DHCP_PARSE_OPTION(netif, dhcp, state, msg, msg_type, option, len, pbuf, offset)   \
 *         do {    LWIP_UNUSED_ARG(msg);                                           \
 *                 dhcp_parse_extra_opts(dhcp, state, option, len, pbuf, offset);  \
 *             } while(0)
 *
 * #define LWIP_HOOK_DHCP_APPEND_OPTIONS(netif, dhcp, state, msg, msg_type, options_len_ptr) \
 *         dhcp_append_extra_opts(netif, state, msg, options_len_ptr);
 *
 * To enable (disable) these option, please set one or both of the below macros to 1 (0)
 * #define LWIP_DHCP_ENABLE_MTU_UPDATE   1
 * #define LWIP_DHCP_ENABLE_CLIENT_ID    1
 */

#ifndef LWIP_HDR_CONTRIB_ADDONS_DHCP_OPTS_H
#define LWIP_HDR_CONTRIB_ADDONS_DHCP_OPTS_H

/* Add standard integers so the header could be included before lwip */
#include <stdint.h>

/* Forward declare lwip structs */
struct dhcp;
struct pbuf;
struct dhcp;
struct netif;
struct dhcp_msg;

/* Internal hook functions */
void dhcp_parse_extra_opts(struct dhcp *dhcp, uint8_t state, uint8_t option, uint8_t len, struct pbuf* p, uint16_t offset);
void dhcp_append_extra_opts(struct netif *netif, uint8_t state, struct dhcp_msg *msg_out, uint16_t *options_out_len);

#endif /* LWIP_HDR_CONTRIB_ADDONS_DHCP_OPTS_H */
