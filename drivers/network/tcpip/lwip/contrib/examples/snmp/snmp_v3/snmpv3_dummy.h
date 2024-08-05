/**
 * @file
 * Dummy SNMPv3 functions.
 */

/*
 * Copyright (c) 2017 Dirk Ziegelmeier.
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Dirk Ziegelmeier <dziegel@gmx.de>
 */

#ifndef LWIP_HDR_APPS_SNMP_V3_DUMMY_H
#define LWIP_HDR_APPS_SNMP_V3_DUMMY_H

#include "lwip/apps/snmp_opts.h"
#include "lwip/err.h"
#include "lwip/apps/snmpv3.h"

#if LWIP_SNMP && LWIP_SNMP_V3

err_t snmpv3_set_user_auth_algo(const char *username, snmpv3_auth_algo_t algo);
err_t snmpv3_set_user_priv_algo(const char *username, snmpv3_priv_algo_t algo);
err_t snmpv3_set_user_auth_key(const char *username, const char *password);
err_t snmpv3_set_user_priv_key(const char *username, const char *password);

void snmpv3_dummy_init(void);

#endif /* LWIP_SNMP && LWIP_SNMP_V3 */

#endif /* LWIP_HDR_APPS_SNMP_V3_DUMMY_H */
