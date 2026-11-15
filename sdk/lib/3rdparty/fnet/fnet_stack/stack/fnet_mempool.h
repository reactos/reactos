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
*  FNET memory pools API.
*
***************************************************************************/

#ifndef _FNET_MEMPOOL_H_

#define _FNET_MEMPOOL_H_

/**************************************************************************/ /*!
 * @internal
 * @brief Memory pool descriptor.
 * @see _fnet_mempool_init()
 ******************************************************************************/
typedef fnet_int32_t fnet_mempool_desc_t;

/* Memory pool unit header.*/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_mempool_unit_header
{
    fnet_size_t                         size FNET_COMP_PACKED;                     /* Unit size. */
    struct fnet_mempool_unit_header     *ptr FNET_COMP_PACKED;   /* Pointer to the next free block. */
}
fnet_mempool_unit_header_t;
FNET_COMP_PACKED_END

typedef enum
{
    FNET_MEMPOOL_ALIGN_8 = (8u),       /* Evenly divisible by 8.*/
    FNET_MEMPOOL_ALIGN_16 = (16u),     /* Evenly divisible by 16.*/
    FNET_MEMPOOL_ALIGN_32 = (32u),     /* Evenly divisible by 32.*/
    FNET_MEMPOOL_ALIGN_64 = (64u)      /* Evenly divisible by 64.*/
}
fnet_mempool_align_t;

#if defined(__cplusplus)
extern "C" {
#endif

fnet_mempool_desc_t _fnet_mempool_init( void *pool_ptr, fnet_size_t pool_size, fnet_mempool_align_t alignment );
void _fnet_mempool_release( fnet_mempool_desc_t mpool );
void _fnet_mempool_free( fnet_mempool_desc_t mpool, void *ap );
void *_fnet_mempool_malloc(fnet_mempool_desc_t mpool, fnet_size_t nbytes );
fnet_size_t _fnet_mempool_free_mem_status( fnet_mempool_desc_t mpool);
fnet_size_t _fnet_mempool_malloc_max( fnet_mempool_desc_t mpool );

#if 0 /* For Debug needs.*/
fnet_return_t _fnet_mempool_check( fnet_mempool_desc_t mpool );
#endif

#if defined(__cplusplus)
}
#endif

#endif
