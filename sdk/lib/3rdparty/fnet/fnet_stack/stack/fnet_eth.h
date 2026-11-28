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
* Ethernet platform independent API definitions.
*
***************************************************************************/

#ifndef _FNET_ETH_H_

#define _FNET_ETH_H_

/**************************************************************************
*     Definitions
***************************************************************************/

/*! @addtogroup fnet_netif
*/
/*! @{ */

/**************************************************************************/ /*!
 * @def     FNET_MAC_ADDR_STR_SIZE
 * @brief   Size of the string buffer that will contain
 *          null-terminated ASCII string of MAC address, represented
 *          by six groups of two hexadecimal digits, separated by colons (:).
 * @see fnet_mac_to_str
 * @showinitializer
 ******************************************************************************/
#define FNET_MAC_ADDR_STR_SIZE       (18)

/**************************************************************************/ /*!
 * @def     FNET_ETH_MTU
 * @brief   The largest allowed MTU by Ethernet at the network layer.
 * @showinitializer
 ******************************************************************************/
#define FNET_ETH_MTU                (1500u)

/**************************************************************************/ /*!
 * @brief Media Access Control (MAC) address  type.
 ******************************************************************************/
typedef fnet_uint8_t fnet_mac_addr_t[6]; /* MAC address type.*/

/**************************************************************************/ /*!
 * @def FNET_MAC_ADDR_INIT
 * @param a First octet of an MAC address.
 * @param b Second octet of an MAC address.
 * @param c Third octet of an MAC address.
 * @param d Fourth octet of an MAC address.
 * @param e Fifth  octet of an MAC address.
 * @param f Sixth octet of an MAC address.
 * @hideinitializer
 * @brief Converts the standard notation @c a:b:c:d:e:f to fnet_mac_addr_t value.
 ******************************************************************************/
#define FNET_MAC_ADDR_INIT(a, b, c, d, e, f)   { (a), (b), (c), (d), (e), (f) }

/* MAC address is multicast. */
#define FNET_MAC_ADDR_IS_MULTICAST(a)	((((a)[0]) == 0x01U)?FNET_TRUE:FNET_FALSE)

/* MAC address is broadcast. */
#define FNET_MAC_ADDR_IS_BROADCAST(a)	(((((a)[0]) == 0xFFU) && \
        (((a)[1]) == 0xFFU)&& \
        (((a)[1]) == 0xFFU)&& \
        (((a)[1]) == 0xFFU)&& \
        (((a)[1]) == 0xFFU)&& \
        (((a)[1]) == 0xFFU))? FNET_TRUE:FNET_FALSE)
/* Copying address. */
#define FNET_MAC_ADDR_COPY(from_addr, to_addr)  \
    (fnet_memcpy(&to_addr[0], &from_addr[0], sizeof(fnet_mac_addr_t)))

/* Useful mac addresses */
extern const fnet_mac_addr_t fnet_eth_null_addr;
extern const fnet_mac_addr_t fnet_eth_broadcast;

/************************************************************************
*  IEEE802.3 PHY MII management register set
*************************************************************************/
#define FNET_ETH_MII_REG_CR             (0x0000U)   /* Control Register */
#define FNET_ETH_MII_REG_SR             (0x0001U)   /* Status Register */
#define FNET_ETH_MII_REG_IDR1           (0x0002U)   /* Identification Register #1 */
#define FNET_ETH_MII_REG_IDR2           (0x0003U)   /* Identification Register #2 */
#define FNET_ETH_MII_REG_ANAR           (0x0004U)   /* Auto-Negotiation Advertisement Register */
#define FNET_ETH_MII_REG_ANLPAR         (0x0005U)   /* Auto-Negotiation Link Partner Ability Register */
#define FNET_ETH_MII_REG_ANER           (0x0006U)   /* Auto-Negotiation Expansion Register */
#define FNET_ETH_MII_REG_ANNPTR         (0x0007U)   /* Auto-Negotiation Next Page TX Register */

/* Status Register flags*/
#define FNET_ETH_MII_REG_SR_LINK_STATUS (0x0004U)
#define FNET_ETH_MII_REG_SR_AN_ABILITY  (0x0008U)
#define FNET_ETH_MII_REG_SR_AN_COMPLETE (0x0020U)

/* Control Register flags*/
#define FNET_ETH_MII_REG_CR_RESET       (0x8000U)    /* Resetting a port is accomplished by setting this bit to 1.*/
#define FNET_ETH_MII_REG_CR_LOOPBACK    (0x4000U)    /* Determines Digital Loopback Mode. */
#define FNET_ETH_MII_REG_CR_DATARATE    (0x2000U)    /* Speed Selection bit.*/
#define FNET_ETH_MII_REG_CR_ANE         (0x1000U)    /* Auto-Negotiation Enable bit. */
#define FNET_ETH_MII_REG_CR_PDWN        (0x0800U)    /* Power Down bit. */
#define FNET_ETH_MII_REG_CR_ISOL        (0x0400U)    /* Isolate bit.*/
#define FNET_ETH_MII_REG_CR_ANE_RESTART (0x0200U)    /* Restart Auto-Negotiation bit.*/
#define FNET_ETH_MII_REG_CR_DPLX        (0x0100U)    /* Duplex Mode bit.*/

/*  Auto-Negotiation Advertisement Register flags*/
#define FNET_ETH_MII_REG_ANAR_100_FULLDUPLEX    (0x0100U)   /* 100 Mbps full-duplex capable.*/
#define FNET_ETH_MII_REG_ANAR_100_HALFDUPLEX    (0x0080U)   /* 100 Mbps half-duplex capable.*/
#define FNET_ETH_MII_REG_ANAR_10_FULLDUPLEX     (0x0040U)   /* 10 Mbps full-duplex capable.*/
#define FNET_ETH_MII_REG_ANAR_10_HALFDUPLEX     (0x0020U)   /* 10 Mbps half-duplex capable.*/
#define FNET_ETH_MII_REG_ANAR_IEEE8023          (0x0001U)   /* IEEE 802.3 */


/******************************************************************************
*     Function Prototypes
*******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Converts a 6 byte MAC address into a null terminated string.
 *
 *
 * @param[in] addr       MAC address.
 *
 * @param[out] str_mac    Pointer to a character buffer will contain the resulting
 *                   text address in standard ":" notation. @n
 *                   The @c str_mac buffer must be at least 18 bytes long
 *                   (@ref FNET_MAC_ADDR_STR_SIZE).
 *
 * @return This function always returns the @c str_mac.
 *
 * @see fnet_str_to_mac()
 ******************************************************************************
 * This function takes an MAC-48 address, specified by the @c addr
 * parameter, and returns a null-terminated ASCII string, represented
 * by six groups of two hexadecimal digits, separated by colons (:),
 * in transmission order (e.g. 01:23:45:67:89:ab ), into buffer pointed to by the
 * @c str_mac.
 ******************************************************************************/
fnet_char_t *fnet_mac_to_str( const fnet_mac_addr_t addr, fnet_char_t *str_mac );

/***************************************************************************/ /*!
 *
 * @brief    Converts a null terminated string to a 6 byte MAC address
 *
 * @param[in] str_mac    Null-terminated MAC address string as pairs of
 *                      hexadecimal numbers separated by colons.
 *
 * @param[out] addr      Buffer will contain a suitable
 *                      binary representation of the @c str_mac MAC address .
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if the string in the @c str_mac parameter does not contain
 *     a legitimate MAC address.
 *
 * @see fnet_mac_to_str()
 ******************************************************************************
 * This function interprets the character string specified by the @c str_mac
 * parameter. This string represents a numeric MAC address expressed
 * in six groups of two hexadecimal digits, separated by colons (:),
 * in transmission order. The value returned, pointed to by the @c addr,
 * is a number suitable for use as an MAC address.
 ******************************************************************************/
fnet_return_t fnet_str_to_mac( const fnet_char_t *str_mac, fnet_mac_addr_t addr );

/*! @} */


/*! @addtogroup fnet_netif_eth
*/
/*! @{ */

/***************************************************************************/ /*!
 *
 * @brief    Read a value from an Ethernet PHY's MII register.
 *
 * @param netif_desc       Network interface descriptor.
 *
 * @param reg_addr         PHY register address.
 *
 * @param reg_data         Pointer where result is written to (16-bits)
 *
 * @return This function returns:
 *   - @ref FNET_OK on success.
 *   - @ref FNET_ERR on failure.
 *
 ******************************************************************************
 *
 * This function reads a value of the Ethernet PHY's MII register.
 *
 ******************************************************************************/
fnet_return_t  fnet_eth_phy_read(fnet_netif_desc_t netif_desc, fnet_uint32_t reg_addr, fnet_uint16_t *reg_data);

/***************************************************************************/ /*!
 *
 * @brief    Write Ethernet PHY's MII register.
 *
 * @param netif_desc       Network interface descriptor.
 *
 * @param reg_addr         PHY register address.
 *
 * @param reg_data         Data to write (16-bits)
 *
 * @return This function returns:
 *   - @ref FNET_OK on success.
 *   - @ref FNET_ERR on failure.
 *
 ******************************************************************************
 *
 * This function writes to an Ethernet PHY's MII register.
 *
 ******************************************************************************/
fnet_return_t  fnet_eth_phy_write(fnet_netif_desc_t netif_desc, fnet_uint32_t reg_addr, fnet_uint16_t reg_data);

/***************************************************************************/ /*!
 *
 * @brief    Get a PHY address number of the specified Ethernet interface.
 *
 * @param netif_desc     Ethernet network interface descriptor.
 *
 * @return This function returns PHY address number.
 *
 * @see fnet_eth_phy_set_addr(), FNET_CFG_CPU_ETH0_PHY_ADDR
 *
 ******************************************************************************
 *
 * This function reads the PHY address number of the Ethernet interface.@n
 *
 ******************************************************************************/
fnet_uint8_t fnet_eth_phy_get_addr(fnet_netif_desc_t netif_desc);

/***************************************************************************/ /*!
 *
 * @brief    Set the PHY address number of the specified Ethernet interface.
 *
 * @param netif_desc     Ethernet network interface descriptor.
 *
 * @param phy_addr       PHY address number.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_eth_phy_get_addr(), FNET_CFG_CPU_ETH0_PHY_ADDR
 *
 ******************************************************************************
 *
 * This function changes the PHY address of the @c netif interface
 * to the @c phy_addr value.@n
 * This function is optional, default PHY address is defined by @ref FNET_CFG_CPU_ETH0_PHY_ADDR
 *
 ******************************************************************************/
fnet_return_t  fnet_eth_phy_set_addr(fnet_netif_desc_t netif_desc, fnet_uint8_t phy_addr );

/*! @} */

#if defined(__cplusplus)
}
#endif

#endif /* _FNET_ETH_H_ */
