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
* Service default configuration.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_SERVICE_CONFIG_H_

#define _FNET_SERVICE_CONFIG_H_

/*! @addtogroup fnet_service_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def     FNET_CFG_SERVICE_MAX
 * @brief   Maximum number of registered services in the service-polling list.@n
 *          Default value is @b @c 11.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SERVICE_MAX
    #define FNET_CFG_SERVICE_MAX   (12u)
#endif

#include "service/dhcp/fnet_dhcp_config.h"
#include "service/autoip/fnet_autoip_config.h"
#include "service/tls/fnet_tls_config.h"
#include "service/http/fnet_http_config.h"
#include "service/fs/fnet_fs_config.h"
#include "service/tftp/fnet_tftp_config.h"
#include "service/telnet/fnet_telnet_config.h"
#include "service/dns/fnet_dns_config.h"
#include "service/ping/fnet_ping_config.h"
#include "service/serial/fnet_serial_config.h"
#include "service/shell/fnet_shell_config.h"
#include "service/sntp/fnet_sntp_config.h"
#include "service/llmnr/fnet_llmnr_config.h"
#include "service/mdns/fnet_mdns_config.h"
#include "service/flash/fnet_flash_config.h"
#include "service/link/fnet_link_config.h"
#include "service/bench/fnet_bench_config.h"
#include "service/azure/fnet_azure_config.h"

/*! @} */

#endif /* _FNET_SERVICE_CONFIG_H_ */
