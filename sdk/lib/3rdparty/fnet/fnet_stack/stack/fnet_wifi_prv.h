/**************************************************************************
*
* Copyright 2011-2017 by Andrey Butok. FNET Community.
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
*  Private. Wi-Fi platform independent API functions.
*
***************************************************************************/

#ifndef _FNET_WIFI_PRV_H_

#define _FNET_WIFI_PRV_H_

#include "fnet.h"
#include "fnet_netif_prv.h"

/************************************************************************
*     Definitions
*************************************************************************/
/**************************************************************************/ /*!
 * @internal
 * @brief    Wi-Fi interface API structure.
 ******************************************************************************/
typedef struct fnet_wifi_api
{
    fnet_return_t       (*wifi_connect)(fnet_netif_t *netif, fnet_wifi_connect_params_t *params);                  /* Starts Wi-Fi connection in station mode.*/
    fnet_return_t       (*wifi_access_point)(fnet_netif_t *netif_desc, fnet_wifi_access_point_params_t *params);   /* Initializes a Wi-Fi access point.*/
    fnet_return_t       (*wifi_disconnect)(fnet_netif_t *netif);                                                   /* Disconnects from a Wi-Fi network.*/
    fnet_wifi_op_mode_t (*wifi_get_op_mode)(fnet_netif_t *netif);                                                  /* Retrieves current operation mode of the Wi-Fi interface.*/
    fnet_return_t       (*wifi_set_country_code)(fnet_netif_t *netif, const fnet_char_t *country_code);            /* Set two-letter country code (ISO3166) to indicate the country-specific regulatory domain in with the W-Fi device is operating.
                                                                                                                      Null-terminated string. For example, "US" - United State, "JP" - Japan.*/
    fnet_return_t       (*wifi_get_country_code)(fnet_netif_t *netif, fnet_char_t *country_code);                  /* Get two letter country code (ISO3166) indicating the country-specific regulatory domain in with the W-Fi device is operating.
                                                                                                                      Null-terminated string. For example, "US" - United State, "JP" - Japan.*/

    fnet_uint32_t       (*wifi_fw_get_version)(fnet_netif_t *netif);                                               /* Gets Wi-Fi interface firmware version number (optional).*/
    fnet_return_t       (*wifi_fw_update)(fnet_netif_t *netif, const fnet_uint8_t *fw_buffer, fnet_size_t fw_buffer_size); /* Updates Wi-Fi interface firmware (optional).*/
} fnet_wifi_api_t;

/************************************************************************
*     Global Data Structures
*************************************************************************/

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* _FNET_WIFI_PRV_H_ */
