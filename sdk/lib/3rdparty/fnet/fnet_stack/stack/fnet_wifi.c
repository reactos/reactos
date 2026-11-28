/**************************************************************************
*
* Copyright 2017 by Andrey Butok. FNET Community.
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
*  Wi-Fi platform independent API functions.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_netif_prv.h"
#include "fnet_wifi_prv.h"
#include "fnet_stack_prv.h"

/************************************************************************
* DESCRIPTION: Connects to a Wi-Fi accesss point.
*************************************************************************/
fnet_return_t fnet_wifi_connect(fnet_netif_desc_t netif_desc, fnet_wifi_connect_params_t *params)
{
    fnet_return_t   result = FNET_ERR;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif && params && (netif->netif_api->netif_type == FNET_NETIF_TYPE_WIFI))
    {
        if(netif->netif_api->wifi_api && netif->netif_api->wifi_api->wifi_connect)
        {
            _fnet_stack_mutex_lock();
            result = netif->netif_api->wifi_api->wifi_connect(netif, params);
            _fnet_stack_mutex_unlock();
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Disconnects Wi-Fi interface. It means to turn off the Wi-Fi interface radio
*************************************************************************/
fnet_return_t   fnet_wifi_disconnect(fnet_netif_desc_t netif_desc)
{
    fnet_return_t   result = FNET_ERR;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif && (netif->netif_api->netif_type == FNET_NETIF_TYPE_WIFI))
    {
        if(netif->netif_api->wifi_api && netif->netif_api->wifi_api->wifi_disconnect)
        {
            _fnet_stack_mutex_lock();
            result = netif->netif_api->wifi_api->wifi_disconnect(netif);
            _fnet_stack_mutex_unlock();
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Initializes a Wi-Fi access point.
*************************************************************************/
fnet_return_t   fnet_wifi_access_point(fnet_netif_desc_t netif_desc, fnet_wifi_access_point_params_t *params)
{
    fnet_return_t   result = FNET_ERR;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif && (netif->netif_api->netif_type == FNET_NETIF_TYPE_WIFI))
    {
        if(netif->netif_api->wifi_api && netif->netif_api->wifi_api->wifi_access_point)
        {
            _fnet_stack_mutex_lock();
            result = netif->netif_api->wifi_api->wifi_access_point(netif, params);
            _fnet_stack_mutex_unlock();
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Retrieves current operation mode of the Wi-Fi interface.
*************************************************************************/
fnet_wifi_op_mode_t fnet_wifi_get_op_mode(fnet_netif_desc_t netif_desc)
{
    fnet_wifi_op_mode_t result = FNET_WIFI_OP_MODE_NONE;
    fnet_netif_t        *netif = (fnet_netif_t *)netif_desc;

    if(netif && (netif->netif_api->netif_type == FNET_NETIF_TYPE_WIFI))
    {
        if(netif->netif_api->wifi_api && netif->netif_api->wifi_api->wifi_get_op_mode)
        {
            _fnet_stack_mutex_lock();
            result = netif->netif_api->wifi_api->wifi_get_op_mode(netif);
            _fnet_stack_mutex_unlock();
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Retrieves firmware version number of the Wi-Fi interface module.
*************************************************************************/
fnet_uint32_t fnet_wifi_fw_get_version(fnet_netif_desc_t netif_desc)
{
    fnet_uint32_t   result = 0;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif && (netif->netif_api->netif_type == FNET_NETIF_TYPE_WIFI))
    {
        if(netif->netif_api->wifi_api && netif->netif_api->wifi_api->wifi_fw_get_version)
        {
            result = netif->netif_api->wifi_api->wifi_fw_get_version(netif);
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Set country code of the Wi-Fi interface.
*************************************************************************/
fnet_return_t fnet_wifi_set_country_code(fnet_netif_desc_t netif_desc, const fnet_char_t *country_code)
{
    fnet_return_t   result = FNET_ERR;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif && (netif->netif_api->netif_type == FNET_NETIF_TYPE_WIFI))
    {
        if(netif->netif_api->wifi_api && netif->netif_api->wifi_api->wifi_set_country_code)
        {
            _fnet_stack_mutex_lock();
            result = netif->netif_api->wifi_api->wifi_set_country_code(netif, country_code);
            _fnet_stack_mutex_unlock();
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Retrieve country code used by the Wi-Fi interface.
*************************************************************************/
fnet_return_t fnet_wifi_get_country_code(fnet_netif_desc_t netif_desc, fnet_char_t *country_code)
{
    fnet_return_t   result = FNET_ERR;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif && (netif->netif_api->netif_type == FNET_NETIF_TYPE_WIFI))
    {
        if(netif->netif_api->wifi_api && netif->netif_api->wifi_api->wifi_get_country_code)
        {
            _fnet_stack_mutex_lock();
            result = netif->netif_api->wifi_api->wifi_get_country_code(netif, country_code);
            _fnet_stack_mutex_unlock();
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Updates firmware of the Wi-Fi interface.
*************************************************************************/
fnet_return_t fnet_wifi_fw_update(fnet_netif_desc_t netif_desc, const fnet_uint8_t *fw_buffer, fnet_size_t fw_buffer_size)
{
    fnet_return_t   result = FNET_ERR;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif && (netif->netif_api->netif_type == FNET_NETIF_TYPE_WIFI))
    {
        if(netif->netif_api->wifi_api && netif->netif_api->wifi_api->wifi_fw_update)
        {
            _fnet_stack_mutex_lock();
            result = netif->netif_api->wifi_api->wifi_fw_update(netif, fw_buffer, fw_buffer_size);
            _fnet_stack_mutex_unlock();
        }
    }

    return result;
}
