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
*  Private. IPv4 protocol API.
*
***************************************************************************/

#ifndef _FNET_IP4_PRV_H_

#define _FNET_IP4_PRV_H_

#include "fnet.h"
#include "fnet_ip4.h"
#include "fnet_netif.h"
#include "fnet_netif_prv.h"

/************************************************************************
*    Definitions.
*************************************************************************/
#define FNET_IP4_MAX_PACKET     (FNET_CFG_IP_MAX_PACKET)

#define FNET_IP4_MAX_OPTIONS    (40U) /* Maximum option field length */

/************************************************************************
*    IP implementation parameters.
*************************************************************************/
#define FNET_IP4_VERSION        (4U)   /* IP version */
#define FNET_IP4_TTL_MAX        (255U) /* Maximum time to live */
#define FNET_IP4_TTL_DEFAULT    (64U)  /* Default ttl, from RFC 1340 */

#define FNET_IP4_DF             (0x4000U)    /* Dont fragment flag */
#define FNET_IP4_MF             (0x2000U)    /* More fragments flag */
#define FNET_IP4_FLAG_MASK      (0xE000U)    /* Mask for fragmenting bits */
#define FNET_IP4_OFFSET_MASK    (0x1fffU)    /* Mask for fragmenting bits */

#define FNET_IP4_TIMER_PERIOD   (500U)
#define FNET_IP4_FRAG_TTL       (10000U/FNET_IP4_TIMER_PERIOD) /* TTL for fragments to complete a datagram (10sec)*/

/**************************************************************************/ /*!
 * @internal
 * @brief    IPv4 layer socket options.
 ******************************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint8_t       ttl       FNET_COMP_PACKED;       /**< TTL.*/
    fnet_uint8_t       tos       FNET_COMP_PACKED;       /**< TOS.*/
#if FNET_CFG_MULTICAST
    fnet_uint8_t       ttl_multicast FNET_COMP_PACKED;   /**< TTL.*/
#endif
} fnet_ip4_sockopt_t;
FNET_COMP_PACKED_END

/**************************************************************************/ /*!
 * @internal
 * @brief    Structure of IPv4 header.
 ******************************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint8_t version__header_length    FNET_COMP_PACKED;   /**< version =4 & header length (x4) (>=5)*/
    fnet_uint8_t tos                       FNET_COMP_PACKED;   /**< type of service */
    fnet_uint16_t  total_length             FNET_COMP_PACKED;   /**< total length */
    fnet_uint16_t  id                       FNET_COMP_PACKED;   /**< identification */
    fnet_uint16_t  flags_fragment_offset    FNET_COMP_PACKED;   /**< flags & fragment offset field (measured in 8-byte order).*/
    fnet_uint8_t  ttl                       FNET_COMP_PACKED;   /**< time to live */
    fnet_uint8_t  protocol                  FNET_COMP_PACKED;   /**< protocol */
    fnet_uint16_t  checksum                 FNET_COMP_PACKED;   /**< checksum */
    fnet_ip4_addr_t source_addr             FNET_COMP_PACKED;   /**< source address */
    fnet_ip4_addr_t desination_addr         FNET_COMP_PACKED;   /**< destination address */
} fnet_ip4_header_t;
FNET_COMP_PACKED_END

/**************************************************************************/ /*!
 * @internal
 * @brief    Structure of IP fragment header.
 ******************************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_ip_frag_header
{
    fnet_uint8_t version__header_length    FNET_COMP_PACKED;   /**< version =4 & header length (x4) (>=5)*/
    fnet_uint8_t mf                        FNET_COMP_PACKED;
    fnet_uint16_t total_length             FNET_COMP_PACKED;   /**< data-payload total length (Host endian)*/
    fnet_uint16_t id                       FNET_COMP_PACKED;   /**< identification*/
    fnet_uint16_t offset                   FNET_COMP_PACKED;   /**< offset field (measured in 8-byte order). (Host endian)*/
    fnet_netbuf_t *nb                       FNET_COMP_PACKED;
    struct fnet_ip_frag_header *next        FNET_COMP_PACKED;   /**< Pointer to the next fragment.*/
    struct fnet_ip_frag_header *prev        FNET_COMP_PACKED;   /**< Pointer to the previous fragment.*/
} fnet_ip4_frag_header_t;
FNET_COMP_PACKED_END

#define FNET_IP_HEADER_GET_FLAG(x)                      ((x)->flags_fragment_offset & FNET_HTONS(FNET_IP4_FLAG_MASK))
#define FNET_IP_HEADER_GET_OFFSET(x)                    ((x)->flags_fragment_offset & FNET_HTONS(FNET_IP4_OFFSET_MASK))
#define FNET_IP_HEADER_GET_VERSION(x)                   (((x)->version__header_length & 0xF0u)>>4)
#define FNET_IP_HEADER_SET_VERSION(x, version)          ((x)->version__header_length = (fnet_uint8_t)(((x)->version__header_length & 0x0Fu)|(((version)&0x0Fu)<<4)))
#define FNET_IP_HEADER_GET_HEADER_LENGTH(x)             ((x)->version__header_length & 0x0Fu)
#define FNET_IP_HEADER_SET_HEADER_LENGTH(x, length)     ((x)->version__header_length = (fnet_uint8_t)(((x)->version__header_length & 0xF0u)|((length)&0x0Fu)))

/**************************************************************************/ /*!
 * @internal
 * @brief    Structure of the head of each reassembly list.
 ******************************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_ip4_frag_list
{
    struct fnet_ip4_frag_list   *next           FNET_COMP_PACKED;   /**< Pointer to the next reassembly list.*/
    struct fnet_ip4_frag_list   *prev           FNET_COMP_PACKED;   /**< Pointer to the previous reassembly list.*/
    fnet_uint8_t                ttl             FNET_COMP_PACKED;   /**< TTL for reassembly.*/
    fnet_uint8_t                protocol        FNET_COMP_PACKED;   /**< protocol.*/
    fnet_uint16_t               id              FNET_COMP_PACKED;   /**< identification.*/
    fnet_ip4_addr_t             source_addr     FNET_COMP_PACKED;   /**< source address.*/
    fnet_ip4_addr_t             desination_addr FNET_COMP_PACKED;   /**< destination address.*/
    fnet_ip4_frag_header_t      *frag_ptr       FNET_COMP_PACKED;   /**< Pointer to the first fragment of the list.*/
} fnet_ip4_frag_list_t;
FNET_COMP_PACKED_END

/******************************************************************************
 * Multicast related structures.
 ******************************************************************************/
#if FNET_CFG_MULTICAST
/* Entry of the multicast group list.*/
typedef struct
{
    fnet_netif_t    *netif;         /* Interface to join on. */
    fnet_ip4_addr_t group_addr;     /* IP address of joined multicast group. */
    fnet_index_t    user_counter;   /* User counter. Keeps a reference count of the number
                                        * of requests to join a particular host group. */
} fnet_ip4_multicast_list_entry_t;

/* Global multicast list.*/
extern fnet_ip4_multicast_list_entry_t fnet_ip4_multicast_list[FNET_CFG_MULTICAST_MAX];

#endif /* FNET_CFG_MULTICAST */

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

fnet_return_t _fnet_ip4_init( void );
void _fnet_ip4_release( void );
fnet_bool_t _fnet_ip4_addr_is_broadcast( fnet_ip4_addr_t addr, fnet_netif_t *netif );
fnet_error_t _fnet_ip4_output( fnet_netif_t *netif,    fnet_ip4_addr_t src_ip, fnet_ip4_addr_t dest_ip,
                               fnet_uint8_t protocol, fnet_uint8_t tos,     fnet_uint8_t ttl,
                               fnet_netbuf_t *nb, fnet_bool_t DF,  fnet_bool_t do_not_route,
                               FNET_COMP_PACKED_VAR fnet_uint16_t *checksum );
void _fnet_ip4_input( fnet_netif_t *netif, fnet_netbuf_t *nb );
fnet_netif_t *_fnet_ip4_route( fnet_ip4_addr_t dest_ip );
fnet_size_t _fnet_ip4_maximum_packet( fnet_ip4_addr_t dest_ip );
void _fnet_ip4_drain( void );
fnet_bool_t _fnet_ip4_will_fragment( fnet_netif_t *netif, fnet_size_t protocol_message_size);
void _fnet_ip4_set_socket_addr(fnet_netif_t *netif, fnet_ip4_header_t *ip_hdr, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr );
struct _fnet_socket_if_t; /* Forward declaration.*/
fnet_error_t _fnet_ip4_getsockopt(struct _fnet_socket_if_t *sock, fnet_socket_options_t optname, void *optval, fnet_size_t *optlen );
fnet_error_t _fnet_ip4_setsockopt(struct _fnet_socket_if_t  *sock, fnet_socket_options_t optname, const void *optval, fnet_size_t optlen );

#if FNET_CFG_MULTICAST
fnet_ip4_multicast_list_entry_t *_fnet_ip4_multicast_join( fnet_netif_t *netif, fnet_ip4_addr_t group_addr );
void _fnet_ip4_multicast_leave_entry( fnet_ip4_multicast_list_entry_t *multicastentry );
#endif /* FNET_CFG_MULTICAST */

#if defined(__cplusplus)
}
#endif

#endif /* _FNET_IP4_PRV_H_ */
