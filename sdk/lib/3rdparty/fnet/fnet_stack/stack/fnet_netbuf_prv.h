/**************************************************************************
*
* Copyright 2018 by Andrey Butok. FNET Community.
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
*  Private. FNET Buffer and memory management definitions.
*
***************************************************************************/

#ifndef _FNET_NETBUF_PRV_H_

#define _FNET_NETBUF_PRV_H_

#include "fnet_mempool.h"

/**************************************************************************/ /*!
 * @internal
 * @brief    netbuf flags.
 ******************************************************************************/
typedef enum
{
    FNET_NETBUF_FLAG_NONE                   = 0x00u,     /* None. */
    FNET_NETBUF_FLAG_BROADCAST              = 0x02u,     /* Send/received as link-level broadcast. */
    FNET_NETBUF_FLAG_MULTICAST              = 0x04u,     /* Send/received as link-level multicast. */
    FNET_NETBUF_FLAG_HW_IP_CHECKSUM         = 0x10u,     /* IPv4 header checksum is calculated/checked by HW.*/
    FNET_NETBUF_FLAG_HW_PROTOCOL_CHECKSUM   = 0x20u      /* Protocol (UDP, TCP, ICMP) checksum is calculated/checked by HW.*/
} fnet_netbuf_flag_t;

/**************************************************************************/ /*!
 * @internal
 * @brief    netbuf types.
 ******************************************************************************/
typedef enum
{
    FNET_NETBUF_TYPE_DATA    = 0,       /**< dynamic (data) allocation.*/
    FNET_NETBUF_TYPE_HEADER  = 1,
    FNET_NETBUF_TYPE_ADDRESS = 2,       /**< address of the message.*/
    FNET_NETBUF_TYPE_OPTION  = 3        /**< options.*/
} fnet_netbuf_type_t;

/**************************************************************************/ /*!
 * @internal
 * @brief    Header at beginning of each net_buf.
 ******************************************************************************/
typedef struct fnet_netbuf
{
    struct fnet_netbuf  *next;          /**< next buffer in chain */
    struct fnet_netbuf  *next_chain;    /**< next chain in queue/record */
    void                *data;          /**< pointer to the beginning of the data buffer */
    void                *data_ptr;      /**< pointer to actual data */
    fnet_size_t         length;         /**< amount of actual data in this net_buf */
    fnet_size_t         total_length;   /**< length of buffer + additionally chained buffers (only for first netbuf)*/
    fnet_flag_t         flags;
} fnet_netbuf_t;

#define FNET_NETBUF_COPYALL   ((fnet_size_t)(-1))

#if defined(__cplusplus)
extern "C" {
#endif

/* Memory management functions */
fnet_return_t _fnet_heap_init( void *heap_ptr, fnet_size_t heap_size );
void _fnet_free( void *ap );
void *_fnet_malloc( fnet_size_t nbytes );
void *_fnet_malloc_zero( fnet_size_t nbytes );
fnet_size_t _fnet_free_mem_status( void );
fnet_size_t _fnet_malloc_max( void );
void _fnet_mem_release( void );

void _fnet_free_netbuf( void *ap );
void *_fnet_malloc_netbuf( fnet_size_t nbytes );
fnet_size_t _fnet_free_mem_status_netbuf( void );
fnet_size_t _fnet_malloc_max_netbuf( void );
void _fnet_mem_release_netbuf( void );

/* Netbuf service routines */
fnet_netbuf_t *_fnet_netbuf_new( fnet_size_t len, fnet_bool_t drain );
fnet_netbuf_t *_fnet_netbuf_free( fnet_netbuf_t *nb );
fnet_netbuf_t *_fnet_netbuf_copy( fnet_netbuf_t *nb, fnet_size_t offset, fnet_size_t len, fnet_bool_t drain );
fnet_netbuf_t *_fnet_netbuf_from_buf( const void *data_ptr, fnet_size_t len, fnet_bool_t drain );
fnet_netbuf_t *_fnet_netbuf_concat( fnet_netbuf_t *nb1, fnet_netbuf_t *nb2 );
void _fnet_netbuf_to_buf( fnet_netbuf_t *nb, fnet_size_t offset, fnet_size_t len, void *data_ptr );
fnet_return_t _fnet_netbuf_pullup( fnet_netbuf_t **nb_ptr, fnet_size_t len);
void _fnet_netbuf_trim( fnet_netbuf_t **nb_ptr, fnet_int32_t len );
fnet_netbuf_t *_fnet_netbuf_cut_center( fnet_netbuf_t **nb_ptr, fnet_size_t offset, fnet_size_t len);
void _fnet_netbuf_queue_add( fnet_netbuf_t **nb_ptr, fnet_netbuf_t *nb_chain );
void _fnet_netbuf_queue_del( fnet_netbuf_t **nb_ptr, fnet_netbuf_t *nb_chain );
void _fnet_netbuf_free_chain( fnet_netbuf_t *nb );

#if 0 /* For Debug needs.*/
fnet_return_t _fnet_netbuf_mempool_check( void );
void _FNET_DEBUG_NETBUF_print_chain( fnet_netbuf_t *nb, fnet_uint8_t *str, fnrt_index_t max);
#endif


#if defined(__cplusplus)
}
#endif

#endif /* _FNET_NETBUF_PRV_H_ */
