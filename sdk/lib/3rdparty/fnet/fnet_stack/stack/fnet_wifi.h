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
* Wi-Fi platform independent API definitions.
*
***************************************************************************/

#ifndef _FNET_WIFI_H_

#define _FNET_WIFI_H_

/**************************************************************************
*     Definitions
***************************************************************************/
/*! @addtogroup fnet_netif_wifi
*/
/*! @{ */

/**************************************************************************/ /*!
 * @def     FNET_WIFI_SSID_SIZE
 * @brief   Wi-Fi SSID (Subscription Service Identifier) can be up to 32 alphanumeric character unique identifier (zero-terminated string).
 * @showinitializer
 ******************************************************************************/
#define FNET_WIFI_SSID_SIZE             sizeof("123456789_123456789_123456789_12")

/**************************************************************************/ /*!
 * @def     FNET_WIFI_PASSPHRASE_SIZE_MAX
 * @brief   Alphanumeric pass-phrase can contain maximum 64 hexadecimal digits (zero-terminated).
 * @showinitializer
 ******************************************************************************/
#define FNET_WIFI_PASSPHRASE_SIZE_MAX   sizeof("123456789_123456789_123456789_123456789_123456789_123456789_1234")

/**************************************************************************/ /*!
 * @def     FNET_WIFI_PASSPHRASE_SIZE_MIN
 * @brief   Alphanumeric pass-phrase can contain minimum 8 characters (zero-terminated).
 * @showinitializer
 ******************************************************************************/
#define FNET_WIFI_PASSPHRASE_SIZE_MIN   sizeof("12345678")

/**************************************************************************/ /*!
 * @brief Wi-Fi operation mode.
 * @see fnet_wifi_get_op_mode()
 ***********************************s*******************************************/
typedef enum
{
    FNET_WIFI_OP_MODE_NONE,             /**< @brief Disconnected. Wi-Fi radio is off @see fnet_wifi_disconnect()*/
    FNET_WIFI_OP_MODE_STATION,          /**< @brief Station mode. @see fnet_wifi_connect()*/
    FNET_WIFI_OP_MODE_ACCESS_POINT      /**< @brief Access point mode. @see fnet_wifi_access_point()*/
} fnet_wifi_op_mode_t;

/**************************************************************************/ /*!
 * @brief WPA-personal security mode.
 ***********************************s*******************************************/
typedef enum
{
    FNET_WIFI_WPA_NONE,         /**< @brief None. The Wi-Fi network is open.*/
    FNET_WIFI_WPA,              /**< @brief WPA. This mode uses TKIP cipher. It is used mainly for legacy devices.*/
    FNET_WIFI_WPA2              /**< @brief WPA2. This mode uses AES(CCMP) cipher. @n
                                  It is used for best security and better performance.*/
} fnet_wifi_wpa_mode_t;

/**************************************************************************/ /*!
 * @brief Wi-Fi connection parameters for @ref fnet_wifi_connect().
 ******************************************************************************/
typedef struct
{
    fnet_char_t     *ssid ;             /**< @brief Wi-Fi SSID (Subscription Service Identifier), also called Wireless Network Name. @n
                                        It can be up to 32 alphanumeric character unique identifier (zero-terminated string).*/
    fnet_char_t     *wpa_passphrase;   /**< @brief WPA alphanumeric pass-phrase. Also, called Pre-Shared Key (PSK). @n
                                        It can be between 8 and 63 characters or 64 hexadecimal digits (zero-terminated).*/
} fnet_wifi_connect_params_t;

/**************************************************************************/ /*!
 * @brief Wi-Fi access point parameters for @ref fnet_wifi_access_point().
 ******************************************************************************/
typedef struct
{
    fnet_char_t             *ssid ;                 /**< @brief Wi-Fi SSID (Subscription Service Identifier), also called Wireless Network Name. @n
                                                    It can be up to 32 alphanumeric character unique identifier (zero-terminated string).*/
    fnet_wifi_wpa_mode_t    wpa_mode;               /**< @brief WPA security mode. */
    fnet_char_t             *wpa_passphrase;        /**< @brief WPA alphanumeric pass-phrase. Also, called Pre-Shared Key (PSK). @n
                                                    It can be between 8 and 63 characters or 64 hexadecimal digits (zero-terminated).@n
                                                    The parameter is ignored if @c wpa_mode is FNET_WIFI_WPA_NONE.*/

    fnet_uint8_t            *ie;                    /**< @brief Information Element to be added to Probe-Response and Beacon frames.@n It is optional.*/
    fnet_size_t             ie_size;                /**< @brief Information Element size. @note QCA4002 supports maximum 64 bytes.*/
} fnet_wifi_access_point_params_t;

/******************************************************************************
*     Function Prototypes
*******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Start Wi-Fi connection in station mode.
 *
 * @param netif_desc    Network interface descriptor.
 *
 * @param params        Connection parameters.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_wifi_disconnect(), fnet_netif_is_connected()
 *
 ******************************************************************************
 *
 * This function starts Wi-Fi connection in the station mode.
 * Connection parameters are defined in the @c params structure. @n
 * To close connection call fnet_wifi_disconnect().
 * The connection state may be determined by fnet_netif_is_connected().
 *
 ******************************************************************************/
fnet_return_t fnet_wifi_connect(fnet_netif_desc_t netif_desc, fnet_wifi_connect_params_t *params);

/***************************************************************************/ /*!
 *
 * @brief    Disconnect from a Wi-Fi network.
 *
 * @param netif_desc    Network interface descriptor.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_wifi_connect(), fnet_netif_is_connected()
 *
 ******************************************************************************
 *
 * This function disconnects from a Wi-Fi network.
 * It means to turn off the Wi-Fi interface radio.
 *
 ******************************************************************************/
fnet_return_t fnet_wifi_disconnect(fnet_netif_desc_t netif_desc);

/***************************************************************************/ /*!
 *
 * @brief    Initialize a Wi-Fi access point.
 *
 * @param netif_desc    Network interface descriptor.
 *
 * @param params        Access point parameters.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_wifi_disconnect(), fnet_netif_is_connected()
 *
 ******************************************************************************
 *
 * This function initializes a Wi-Fi access point.
 * Access point parameters are defined in the @c params structure. @n
 * To disable the access point mode call fnet_wifi_disconnect().@n
 * The connection state may be determined by fnet_netif_is_connected().
 *
 ******************************************************************************/
fnet_return_t fnet_wifi_access_point(fnet_netif_desc_t netif_desc, fnet_wifi_access_point_params_t *params);

/***************************************************************************/ /*!
 *
 * @brief    Retrieve current operation mode of the Wi-Fi interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return This function returns current operation mode.
 *
 * @see fnet_wifi_connect(), fnet_wifi_access_point(), fnet_wifi_disconnect()
 *
 ******************************************************************************
 *
 * This function retrieves the operation mode of the @c netif Wi-Fi interface.
 *
 ******************************************************************************/
fnet_wifi_op_mode_t fnet_wifi_get_op_mode(fnet_netif_desc_t netif_desc);

/***************************************************************************/ /*!
 *
 * @brief    Retrieve firmware version number of the Wi-Fi interface module.
 *
 * @param netif_desc       Network interface descriptor.
 *
 * @return This function returns:
 *   - firmware version number if no error occurs.
 *   - @c 0 if an error occurs or Wi-Fi driver is not able to provide a version number.
 *
 ******************************************************************************
 *
 * This function gets a version number of a firmware running on the Wi-Fi module.
 *
 ******************************************************************************/
fnet_uint32_t fnet_wifi_fw_get_version(fnet_netif_desc_t netif_desc);

/***************************************************************************/ /*!
 *
 * @brief    Set country code of the Wi-Fi interface.
 *
 * @param[in] netif_desc  Network interface descriptor.
 *
 * @param[in] country_code  A two-letter country code (3 bytes - null-terminated string).
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_wifi_get_country_code()
 *
 ******************************************************************************
 *
 * This function sets the country code (ISO3166) to indicate
 * the country-specific regulatory domain in with the W-Fi device is operating. @n
 * For example, "US" - United State, "JP" - Japan.
 * @note QCA4002/4 WiFi module supports only up to three set operations.
 *
 ******************************************************************************/
fnet_return_t fnet_wifi_set_country_code(fnet_netif_desc_t netif_desc, const fnet_char_t *country_code);

/***************************************************************************/ /*!
 *
 * @brief    Retrieve country code used by the Wi-Fi interface.
 *
 * @param[in] netif_desc  Network interface descriptor.
 *
 * @param[out] country_code        String buffer that receives a two-letter country code (3 bytes - null-terminated string).
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_wifi_set_country_code()
 *
 ******************************************************************************
 *
 * This function retrieves the active country code (ISO3166) indicating
 * the country-specific regulatory domain in with the W-Fi device is operating. @n
 * For example, "US" - United State, "JP" - Japan.
 *
 ******************************************************************************/
fnet_return_t fnet_wifi_get_country_code(fnet_netif_desc_t netif_desc, fnet_char_t *country_code);

/***************************************************************************/ /*!
 *
 * @brief    Update firmware of the Wi-Fi interface.
 *
 * @param netif_desc       Network interface descriptor.
 *
 * @param fw_buffer        Firmware buffer.
 *
 * @param fw_buffer_size   Firmware buffer size.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 ******************************************************************************
 *
 * This function does firmware update of the Wi-Fi module. @n
 * It is valid only if FNET_CFG_CPU_WIFI_FW_UPDATE is set.
 * @note QCA firmware buffer can be found in @verbatim \fnet\third_party\qca-x.x.x\flashotp_x_x_x.h @endverbatim
 *
 ******************************************************************************/
fnet_return_t fnet_wifi_fw_update(fnet_netif_desc_t netif_desc, const fnet_uint8_t *fw_buffer, fnet_size_t fw_buffer_size);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif
