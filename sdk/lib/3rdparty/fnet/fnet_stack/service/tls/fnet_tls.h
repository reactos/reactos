/**************************************************************************
*
* Copyright 2015-2018 by Andrey Butok. FNET Community.
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
*  TLS Library API.
*
***************************************************************************/
#ifndef _FNET_TLS_H_

#define _FNET_TLS_H_

#if FNET_CFG_TLS || defined(__DOXYGEN__)

/*! @addtogroup fnet_tls
*
* The Transport Layer Security (TLS) protocol provides privacy and data integrity between communicating applications.@n
* FNET uses the mbedTLS library (https://tls.mbed.org/). Its source code is placed in the fnet/third_party/mbedtls-x.x.x folder.
* @n
* After the TLS context is initialized by calling the @ref fnet_tls_init() function,
* the user application may create TLS sockets by calling fnet_tls_socket() and to receive/send data using fnet_tls_socket_recv() and fnet_tls_socket_send().
* @n
* For the TLS server example, refer to the HTTPS server in the FNET Shell mbedTLS demo example.
*
* Configuration parameters:
* - @ref FNET_CFG_TLS
* - @ref FNET_CFG_TLS_SOCKET_MAX
*
*/

/*! @{ */

/**************************************************************************/ /*!
 * @brief TLS context descriptor.
 * @see fnet_tls_init()
 ******************************************************************************/
typedef void *fnet_tls_desc_t;

/**************************************************************************/ /*!
 * @brief TLS socket descriptor.
 * @see fnet_tls_socket()
 ******************************************************************************/
typedef void *fnet_tls_socket_t;

/**************************************************************************/ /*!
 * @brief    TLS roles
 ******************************************************************************/
typedef enum
{
    FNET_TLS_ROLE_SERVER            = 1,     /**< @brief TLS Server */
    FNET_TLS_ROLE_CLIENT            = 2      /**< @brief TLS Client */
} fnet_tls_role_t;

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initialize the TLS context.
 *
 * @param role     TLS role. Client or server.
 *
 * @return This function returns:
 *   - TLS Context descriptor if no error occurs.
 *   - @c FNET_NULL if an error occurs.
 *
 * @see fnet_tls_release(), fnet_tls_socket(), fnet_tls_set_own_certificate()
 *
 ******************************************************************************
 *
 * This function initializes the TLS context.@n
 * It allocates all  resources and set parameters required for TLS connection.
 * After a successful initialization, the application may create TLS sockets using
 * fnet_tls_socket(). The context may be shared between several sockets.
 *
 ******************************************************************************/
fnet_tls_desc_t fnet_tls_init(fnet_tls_role_t role);

/***************************************************************************/ /*!
 *
 * @brief    Set own certificate chain and private key
 *
 * @param tls_desc                  TLS context descriptor
 * @param own_certificate           Buffer holding own public certificate chain, in PEM or DER format.
 * @param own_certificate_size      Size of the public certificate chain buffer (including the terminating null byte for PEM data).
 * @param private_key               Buffer holding the own private key, in PEM or DER format.
 * @param private_key_size          Size of the private key buffer (including the terminating null byte for PEM data).
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_tls_init()
 *
 ******************************************************************************
 *
 * This function assign own public certificate chain and own private key to the TLS context.@n
 * Used mainly by TLS servers.
 *
 ******************************************************************************/
fnet_return_t fnet_tls_set_own_certificate(fnet_tls_desc_t tls_desc, const fnet_uint8_t *own_certificate, fnet_size_t own_certificate_size, const fnet_uint8_t *private_key, fnet_size_t private_key_size);

/***************************************************************************/ /*!
 *
 * @brief    Assign Certificate Authority certificate
 *
 * @param tls_desc              TLS context descriptor
 * @param ca_certificate        Buffer holding CA (Certificate Authority), or trusted root, certificate data, in PEM or DER format.
 *                              It is used to verify certificates received from peers during the TLS handshake.
 * @param ca_certificate_size   Size of the CA certificate buffer (including the terminating null byte for PEM data).
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_tls_init()
 *
 ******************************************************************************
 *
 * This function assign CA (Certificate Authority), or trusted root, certificate to the TLS context.@n
 * It is used to verify certificates received from peers during the TLS handshake.@n
 * Used mainly by TLS clients.
 *
 ******************************************************************************/
fnet_return_t fnet_tls_set_ca_certificate(fnet_tls_desc_t tls_desc, const fnet_uint8_t *ca_certificate, fnet_size_t ca_certificate_size);

/***************************************************************************/ /*!
 *
 * @brief    Release the TLS context.
 *
 * @param tls_desc     TLS context descriptor to be released.
 *
 * @see fnet_tls_init()
 *
 ******************************************************************************
 *
 * This function releases all resources allocated  during TLS context initialization.
 *
 ******************************************************************************/
void fnet_tls_release(fnet_tls_desc_t tls_desc);

/***************************************************************************/ /*!
 *
 * @brief    Create the TLS socket.
 *
 * @param tls_desc   TLS context descriptor.
 *
 * @param sock       FNET socket descriptor.
 *
 * @return This function returns:
 *   - TLS socket descriptor, if no error occurs.
 *   - @ref FNET_NULL if an error occurs.
 *
 * @see fnet_tls_socket_close(), fnet_tls_init()
 *
 ******************************************************************************
 *
 * This function creates a TLS socket and returns its descriptor.@n
 * The data can be transferred using the @ref fnet_tls_socket_send() and  the @ref
 * fnet_tls_socket_recv() calls. When a session has been completed, the @ref fnet_tls_socket_close()
 * must be performed.
 *
 ******************************************************************************/
fnet_tls_socket_t fnet_tls_socket(fnet_tls_desc_t tls_desc, fnet_socket_t sock);

/***************************************************************************/ /*!
 *
 * @brief    Close the TLS socket.
 *
 * @param tls_sock      TLS socket descriptor to be closed.
 *
 * @see fnet_tls_socket()
 *
 ******************************************************************************
 *
 * This function releases the TLS socket descriptor.@n
 * An application should always have a matching call to the @ref fnet_tls_socket_close() for
 * each successful call to the @ref fnet_tls_socket() to return any TLS socket resources to
 * the system.
 *
 ******************************************************************************/
void fnet_tls_socket_close(fnet_tls_socket_t tls_sock);

/***************************************************************************/ /*!
 *
 * @brief    Perform the TLS handshake
 *
 * @param tls_sock      TLS socket descriptor.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_tls_socket()
 *
 ******************************************************************************
 *
 * The function performs TLS handshake on already connected socket.@n
 * If the function returns FNET_ERR the TLS handshake is failed
 * and the connection must be closed.@n
 * It is optional, as the handshake may be done inside fnet_tls_socket_recv()
 * or fnet_tls_socket_send() call if needed.
 *
 ******************************************************************************/
fnet_return_t fnet_tls_socket_connect(fnet_tls_socket_t tls_sock);

/***************************************************************************/ /*!
 *
 * @brief    Set the host name to check against the received server certificate.
 *
 * @param tls_sock      TLS socket descriptor.
 *
 * @param hostname      Server hostname, may be FNET_NULL to clear hostname
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_tls_socket(), fnet_tls_socket_connect()
 *
 ******************************************************************************
 *
 * The function sets or reset the host name to check against the received
 * server certificate.@n
 * It is used mainly by TLS client and should be set before TLS handshake.
 *
 ******************************************************************************/
fnet_return_t fnet_tls_socket_set_hostname(fnet_tls_socket_t tls_sock, const fnet_char_t *hostname);

/***************************************************************************/ /*!
 *
 * @brief    Receive data from a TLS socket.
 *
 * @param tls_sock      TLS socket descriptor.
 *
 * @param buf           Buffer for the incoming data.
 *
 * @param len           Length of the @c buf.
 *
 * @return This function returns:
 *   - The number of bytes received, if no error occurs.
 *     The return value is set to zero, if there
 *     is no input data.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_tls_socket()
 *
 ******************************************************************************
 *
 * The function returns as much data as is currently available up to the size
 * of the buffer supplied.@n
 * If the function returns FNET_ERR the current TLS connection must be closed.
 *
 ******************************************************************************/
fnet_ssize_t fnet_tls_socket_recv(fnet_tls_socket_t tls_sock, fnet_uint8_t *buf, fnet_size_t len);

/***************************************************************************/ /*!
 *
 * @brief    Send data on a TLS socket.
 *
 * @param tls_sock  TLS socket descriptor.
 *
 * @param buf       Buffer containing data to be transmitted.
 *
 * @param len       Length of the data in @c buf.
 *
 * @return This function returns:
 *   - The total number of bytes sent, if no error occurs.
 *     It can be less than the number indicated by @c len.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_tls_socket()
 *
 ******************************************************************************
 *
 * This function sends data over TLS layer. @n
 * The number of actually sent bytes can be between @c 0 and the requested length, depending on
 * buffer availability on both client and server machines.@n
 * If the function returns FNET_ERR the current TLS connection must be closed.
 *
 ******************************************************************************/
fnet_ssize_t fnet_tls_socket_send(fnet_tls_socket_t tls_sock, const fnet_uint8_t *buf, fnet_size_t len);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_TLS */

#endif /* _FNET_TLS_H_ */
