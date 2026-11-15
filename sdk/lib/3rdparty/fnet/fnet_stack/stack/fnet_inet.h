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
*  Address-conversion functions API.
*
***************************************************************************/

#ifndef _FNET_INET_H_

#define _FNET_INET_H_

/*! @addtogroup fnet_socket
*/
/*! @{ */

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Converts an IPv4 address into a string in Internet
 *           standard dotted-decimal format.
 *
 *
 * @param addr       Structure that represents an Internet address.
 *
 * @param res_str    Pointer to a character buffer will contain the resulting
 *                   text address in standard "." notation.@n
 *                   The @c res_str buffer must be at least 16 bytes long
 *                   (@ref FNET_IP4_ADDR_STR_SIZE).
 *
 *
 *
 * @return This function always returns the @c res_str.
 *
 * @see fnet_inet_aton(), fnet_inet_ntop(), fnet_inet_pton()
 *
 ******************************************************************************
 *
 * This function takes an Internet address structure, specified by the @c addr
 * parameter, and returns a null-terminated ASCII string, representing the
 * address in "." (dot) notation as in "a.b.c.d" into buffer pointed to by the
 * @c res_str.
 *
 * @note
 * fnet_inet_ntop() extends the fnet_inet_ntoa() function to support multiple
 * address families. @n
 * fnet_inet_ntoa() is now considered to be deprecated.
 *
 ******************************************************************************/
fnet_char_t *fnet_inet_ntoa( struct fnet_in_addr addr, fnet_char_t *res_str );

/***************************************************************************/ /*!
 *
 * @brief    Converts the string in the standard dotted-decimal notation
 *           to an integer value, suitable for use as an IPv4 address.
 *
 *
 * @param cp         Null-terminated character string representing a number
 *                   expressed in the Internet standard "." (dotted) notation.
 *
 * @param addr       Pointer to an integer will contain a suitable
 *                   binary representation of the Internet address @c cp.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if the string in the @c cp parameter does not contain
 *     a legitimate Internet address.
 *
 * @see fnet_inet_aton(), fnet_inet_ntop(), fnet_inet_pton()
 *
 ******************************************************************************
 *
 * This function interprets the character string specified by the @c cp
 * parameter. This string represents a numeric Internet address expressed
 * in the Internet standard "." notation. The value returned, pointed to by the @c addr,
 * is a number suitable for use as an Internet address.@n
 * @note
 * fnet_inet_pton() extends the fnet_inet_aton() function to support multiple
 * address families. @n
 * fnet_inet_aton() is now considered to be deprecated.
 *
 ******************************************************************************/
fnet_return_t fnet_inet_aton( fnet_char_t *cp, struct fnet_in_addr *addr );

/***************************************************************************/ /*!
 *
 * @brief   Converts IPv4 or IPv6 address from binary to text form.
 *
 *
 * @param family   The address family (@ref AF_INET or @ref AF_INET6).
 *
 * @param addr  Pointer to the IP address in network-byte order.
 *
 * @param str   Pointer to a buffer in which to store the NULL-terminated
 *              string representation of the IP address.@n
 *              For an IPv4 address, the @c str buffer must be at least 16 bytes long
 *                   (@ref FNET_IP4_ADDR_STR_SIZE).@n
 *              For an IPv6 address, the @c str buffer must be at least 46 bytes long
 *                   (@ref FNET_IP6_ADDR_STR_SIZE).@n
 *
 * @param str_len  Length of the @c str buffer.
 *
 *
 * @return This function returns:
 *   - pointer to a buffer containing the string representation of IP
 *     address (the @c str), if no error occurs,
 *   - @ref FNET_NULL if an error occurs.
 *
 * @see fnet_inet_pton()
 *
 ******************************************************************************
 *
 * This function converts the network address structure, specified by the @c addr
 * parameter, in the @c addr_family address family into a character string.
 * The resulting string is copied to the buffer pointed to by @c str.
 *
 * @note
 * fnet_inet_ntop() extends the fnet_inet_ntoa() function to support multiple
 * address families. @n
 * fnet_inet_ntoa() is now considered to be deprecated.
 *
 ******************************************************************************/
fnet_char_t *fnet_inet_ntop(fnet_address_family_t family, const void *addr, fnet_char_t *str, fnet_size_t str_len);

/***************************************************************************/ /*!
 *
 * @brief    Converts IPv4 and IPv6 addresses from text to binary form.
 *
 *
 * @param family     The address family (@ref AF_INET or @ref AF_INET6).
 *
 * @param str        Null-terminated character string that contains the text
 *                   representation of the IP address to convert to numeric
 *                   binary form.
 *
 * @param addr       Pointer to a buffer in which to store the numeric binary
 *                   representation of the IP address @c str.
 *
 * @param addr_len   Length of the @c addr buffer.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if the string in the @c str parameter does not contain
 *     a legitimate Internet address.
 *
 * @see fnet_inet_ntop()
 *
 ******************************************************************************
 *
 * This function converts the character string @c src into a network address
 * structure in the @c addr_family address family, then copies the network
 * address structure to the @c addr buffer.
 *
 * @note
 * fnet_inet_pton() extends the fnet_inet_aton() function to support multiple
 * address families. @n
 * fnet_inet_aton() is now considered to be deprecated.
 *
 ******************************************************************************/
fnet_return_t fnet_inet_pton (fnet_address_family_t family, const fnet_char_t *str, void *addr, fnet_size_t addr_len);


/***************************************************************************/ /*!
 *
 * @brief    Converts IPv4 and IPv6 addresses from text to socket-address structure.
 *
 * @param str        Null-terminated character string that contains the text
 *                   representation of the IP address to convert to socket-address
 *                   structure. The @c sa_port is set to zero.
 *
 * @param addr       Pointer to a socket-address structure to be filled.

 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if the string in the @c str parameter does not contain
 *     a legitimate Internet address.
 *
 * @see fnet_inet_pton()
 *
 ******************************************************************************
 *
 * This function converts the character string @c src into a socket-address
 * structure.
 *
 ******************************************************************************/
fnet_return_t fnet_inet_ptos (const fnet_char_t *str, struct fnet_sockaddr *addr);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _FNET_INET_H_ */
