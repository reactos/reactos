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
*  General service-header file.
*
***************************************************************************/

#ifndef _FNET_SERVICE_H_

#define _FNET_SERVICE_H_

#include "serial/fnet_serial.h"
#include "shell/fnet_shell.h"
#include "sntp/fnet_sntp.h"
#include "telnet/fnet_telnet.h"
#include "tftp/fnet_tftp_cln.h"
#include "tftp/fnet_tftp_srv.h"
#include "dhcp/fnet_dhcp_cln.h"
#include "dhcp/fnet_dhcp_srv.h"
#include "flash/fnet_flash.h"
#include "fs/fnet_fs.h"
#include "fs/fnet_fs_rom.h"
#include "tls/fnet_tls.h"
#include "http/fnet_http_srv.h"
#include "http/fnet_http_cln.h"
#include "dns/fnet_dns.h"
#include "ping/fnet_ping.h"
#include "llmnr/fnet_llmnr.h"
#include "mdns/fnet_mdns.h"
#include "autoip/fnet_autoip.h"
#include "link/fnet_link.h"
#include "tls/fnet_tls.h"
#include "bench/fnet_bench_srv.h"
#include "bench/fnet_bench_cln.h"
#include "azure/fnet_azure.h"

/*! @addtogroup fnet_service
* The polling mechanism enables the execution of registered services
* (DHCP client, TFTP client, Shell, Telnet server or HTTP server) in
* "background" - during the application
* idle time. Typically, the polling callback is registered during the service
* initialization (for example the @ref fnet_http_srv_init() or other service initialization
* function).
* In order to make the polling mechanism work, the user application should
* call the @ref fnet_service_poll() API function periodically, during the idle time.@n
* @n
* Configuration parameters:
* - @ref FNET_CFG_SERVICE_MAX
*/
/*! @{ */

/**************************************************************************/ /*!
 * @brief Descriptor of a registered service.
 ******************************************************************************/
typedef void *fnet_service_desc_t;

/**************************************************************************/ /*!
 * @brief Service callback function prototype.
 *
 * @param service_cookie   This parameter is assigned during
 *                        a service registration by the @ref fnet_service_register().
 *
 ******************************************************************************/
typedef void (* fnet_service_poll_t)(void *service_cookie);

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Service polling function.
 *
 * @see fnet_service_register()
 *
 ******************************************************************************
 *
 * This function calls all registered service routines.@n
 * The user application should call this function periodically, after any service
 * initialization.
 *
 ******************************************************************************/
void fnet_service_poll(void);

/***************************************************************************/ /*!
 *
 * @brief    Registers the service routine in the polling list.
 *
 * @param service        Pointer to the service-polling routine.
 *
 * @param service_cookie  Service-polling-routine-specific parameter.
 *
 * @return This function returns:
 *   - Service descriptor, if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_service_unregister()
 *
 ******************************************************************************
 *
 * This function adds the service routine into the polling list.@n
 * This function is usually called during a service initialization.
 * User application should not call this function directly.
 *
 ******************************************************************************/
fnet_service_desc_t fnet_service_register( fnet_service_poll_t service, void *service_cookie );

/***************************************************************************/ /*!
 *
 * @brief    Unregisters the service routine.
 *
 * @param desc       Service descriptor to be unregistered.
 *
 * @see fnet_service_register()
 *
 ******************************************************************************
 *
 * This function unregisters the service routine assigned to the @c desc
 * descriptor.@n
 * This function is usually called during a service release.
 * User application should not call this function directly.
 *
 ******************************************************************************/
void fnet_service_unregister( fnet_service_desc_t desc );

#if defined(__cplusplus)
}
#endif

/*! @} */

#if FNET_CFG_MULTITHREADING
    void fnet_service_mutex_lock(void);
    void fnet_service_mutex_unlock(void);
#else
    #define fnet_service_mutex_lock()        do{}while(0)
    #define fnet_service_mutex_unlock()      do{}while(0)
#endif

#endif /* _FNET_SERVICE_H_ */

