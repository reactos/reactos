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
*  FNET Buffer and memory management implementation.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_prot.h"
#include "fnet_mempool.h"
#include "fnet_stack_prv.h"


#define FNET_HEAP_SPLIT     (0) /* If 1 the main heap will be splitted to two parts.
                                 * (have issue so to be tested later).*/

static fnet_mempool_desc_t fnet_mempool_main = 0; /* Main memory pool. */

#if FNET_HEAP_SPLIT
    #define FNET_NETBUF_MEMPOOL_SIZE(size)      (((size)*4)/5)
    static fnet_mempool_desc_t fnet_mempool_netbuf = 0; /* Netbuf memory pool. */
#else
    #define fnet_mempool_netbuf fnet_mempool_main
#endif

fnet_netbuf_t *dm_nb;

/************************************************************************
* DESCRIPTION: Creates a new net_buf and allocates memory
*              for a new data buffer.
*************************************************************************/
fnet_netbuf_t *_fnet_netbuf_new( fnet_size_t len, fnet_bool_t drain )
{
    fnet_netbuf_t   *nb;
    void            *nb_d;

    nb = (fnet_netbuf_t *)_fnet_malloc_netbuf(sizeof(fnet_netbuf_t));

    if((nb == 0) && drain)
    {
        _fnet_prot_drain();
        nb = (fnet_netbuf_t *)_fnet_malloc_netbuf(sizeof(fnet_netbuf_t));
    }

    if(nb == 0) /* If FNET_NETBUF_MALLOC_NOWAIT and no free memory for a new net_buf*/
    {
        return (fnet_netbuf_t *)0;
    }

    nb_d = _fnet_malloc_netbuf(len + sizeof(fnet_uint32_t)/* For reference_counter */);

    if((nb_d == 0) && drain )
    {
        _fnet_prot_drain();
        nb_d = _fnet_malloc_netbuf(len + sizeof(fnet_uint32_t)/* For reference_counter */);
    }

    if(nb_d == 0) /* If FNET_NETBUF_MALLOC_NOWAIT and no free memory for data.*/
    {
        _fnet_free_netbuf(nb);
        return (fnet_netbuf_t *)0;
    }

    nb->next = (fnet_netbuf_t *)0;
    nb->next_chain = (fnet_netbuf_t *)0;

    /* In memory net_buf's data is after the descriptor "data".*/

    ((fnet_uint32_t *)nb_d)[0] = 1u; /* First element is used by the reference_counter.*/
    nb->data = &((fnet_uint32_t *)nb_d)[0];
    nb->data_ptr = &((fnet_uint32_t *)nb_d)[1];
    nb->length = len;
    nb->total_length = len;
    nb->flags = 0u;

    return (nb);
}

/************************************************************************
* DESCRIPTION: Creates a new net_buf using data buffer,
*              which was created before for another net_buf.
*************************************************************************/
fnet_netbuf_t *_fnet_netbuf_copy( fnet_netbuf_t *nb, fnet_size_t offset, fnet_size_t len, fnet_bool_t drain )
{
    fnet_netbuf_t   *loc_nb, *loc_nb_head, *tmp_nb;
    fnet_int32_t    tot_len = 0;
    fnet_size_t     tot_offset = 0u;

    tmp_nb = nb;

    do /* Calculate the total length of the buf for current net_buf chain.*/
    {
        tot_len += (fnet_int32_t)tmp_nb->length;
        tmp_nb = tmp_nb->next;
    }
    while (tmp_nb);

    if(len == FNET_NETBUF_COPYALL)
    {
        tot_len -= (fnet_int32_t)offset;
        len = (fnet_size_t)tot_len;
    }
    else
    {
        if(((len + offset) > (fnet_size_t)tot_len ) || ((fnet_size_t)tot_len < offset))
        {
            return (fnet_netbuf_t *)0;
        }
    }
    /* In tot_len finally - the size of required net_buf data*/

    tmp_nb = nb;


    loc_nb = (fnet_netbuf_t *)_fnet_malloc_netbuf(sizeof(fnet_netbuf_t));

    if((loc_nb == 0) && drain)
    {
        _fnet_prot_drain();
        loc_nb = (fnet_netbuf_t *)_fnet_malloc_netbuf(sizeof(fnet_netbuf_t));
    }

    if(loc_nb == 0)
    {
        return (fnet_netbuf_t *)0;
    }

    loc_nb_head = loc_nb; /* Save the head of net_buf chain.*/
    loc_nb->next_chain = (fnet_netbuf_t *)0;
    loc_nb->total_length = (fnet_size_t)len;
    loc_nb->flags = nb->flags;

    if(tmp_nb->length > offset) /* If offset less than size of 1st net_buf.*/
    {
        tot_offset = offset;
    }
    else /* Find corresponding net_buf and calculate the offset in it.*/
    {
        while((tot_offset += tmp_nb->length) <= offset)
        {
            tmp_nb = tmp_nb->next;
        }

        tot_offset = (tmp_nb->length + offset - tot_offset);
    }

    loc_nb->data = tmp_nb->data;

    loc_nb->data_ptr = (fnet_uint8_t *)tmp_nb->data_ptr + tot_offset;

    ((fnet_uint32_t *)loc_nb->data)[0] = ((fnet_uint32_t *)loc_nb->data)[0] + 1u;    /* Increment the the reference_counter.*/

    tot_len = (fnet_int32_t)(len - (tmp_nb->length - tot_offset));

    if(tot_len <= 0) /* If only one net_buf required.*/
    {
        loc_nb->length = (fnet_size_t)len;
    }
    else
    {
        loc_nb->length = tmp_nb->length - tot_offset;

        do
        {
            loc_nb->next = (fnet_netbuf_t *)_fnet_malloc_netbuf(sizeof(fnet_netbuf_t));

            if((loc_nb->next == 0) && drain )
            {
                _fnet_prot_drain();
                loc_nb->next = (fnet_netbuf_t *)_fnet_malloc_netbuf(sizeof(fnet_netbuf_t));
            }

            if(loc_nb->next == 0) /* there is a need to erase all buffers,*/
            {
                /* which were created earlier.*/
                loc_nb_head = _fnet_netbuf_free(loc_nb_head);

                while(loc_nb_head != loc_nb->next)
                {
                    tmp_nb = loc_nb_head->next;
                    _fnet_free_netbuf(loc_nb_head);
                    loc_nb_head = tmp_nb;
                }

                return (fnet_netbuf_t *)0;
            }

            loc_nb = loc_nb->next;
            loc_nb->next_chain = (fnet_netbuf_t *)0;

            tmp_nb = tmp_nb->next;

            loc_nb->data = tmp_nb->data;
            loc_nb->flags = tmp_nb->flags;

            ((fnet_uint32_t *)loc_nb->data)[0] = ((fnet_uint32_t *)loc_nb->data)[0] + 1u; /* Increment the the reference_counter.*/

            loc_nb->data_ptr = tmp_nb->data_ptr;

            tot_len -= (fnet_int32_t)tmp_nb->length;

            if(tot_len < 0) /* for correct calculation of length */
            {
                loc_nb->length = (fnet_size_t)(tot_len + (fnet_int32_t)tmp_nb->length);
            }
            else
            {
                loc_nb->length = tmp_nb->length;
            }
        }
        while (tot_len > 0);
    }

    loc_nb->next = (fnet_netbuf_t *)0;

    return (loc_nb_head);
}

/************************************************************************
* DESCRIPTION: Creates a new net_buf and fills it by a content of
*              the external data buffer.
*************************************************************************/
fnet_netbuf_t *_fnet_netbuf_from_buf( const void *data_ptr, fnet_size_t len, fnet_bool_t drain )
{
    fnet_netbuf_t *nb;

    nb = _fnet_netbuf_new(len, drain);

    if(nb)
    {
        fnet_memcpy(nb->data_ptr, data_ptr, len);
    }

    return (nb);
}

/************************************************************************
* DESCRIPTION: Creates a new continuous buffer, which contains
*              info from all net buffers of the current chain.
*************************************************************************/
void _fnet_netbuf_to_buf( fnet_netbuf_t *nb, fnet_size_t offset, fnet_size_t len, void *data_ptr )
{
    fnet_uint8_t    *u_buf;
    fnet_netbuf_t   *tmp_nb;
    fnet_size_t     tot_len = 0u;
    fnet_size_t     tot_offset = 0u;
    fnet_size_t     cur_len;

    u_buf = (fnet_uint8_t *)data_ptr;

    if(len == 0u)
    {
        return; /* if input params aren't correct. */
    }


    tmp_nb = nb;
    /* This part is similar to the corresponding part in _fnet_netbuf_copy */
    do
    {
        tot_len += tmp_nb->length;
        tmp_nb = tmp_nb->next;
    }
    while (tmp_nb);

    if(len == FNET_NETBUF_COPYALL) /* If buffer should contain info from all net buffers */
    {
        tot_len -= offset;
        len = (fnet_size_t)tot_len;
    }
    else
    {
        if(((len + offset) > tot_len) || (tot_len < offset))
        {

            return; /* if input params aren't correct. */
        }
    }

    tmp_nb = nb;

    if(tmp_nb->length > offset)
    {
        tot_offset = offset;
    }
    else
    {
        while((tot_offset += tmp_nb->length) <= offset)
        {
            tmp_nb = tmp_nb->next;
        }

        tot_offset = (tmp_nb->length + offset - tot_offset);
    }

    tot_len = len;

    do
    {
        /* Calculate the quantity of bytes we copy from the current net_buf*/
        cur_len = ((tmp_nb->length - tot_offset > tot_len) ? tot_len : tmp_nb->length - tot_offset);
        /* and substract from the total quantity of bytes we copy */
        tot_len -= cur_len;

        fnet_memcpy(u_buf, (fnet_uint8_t *)tmp_nb->data_ptr + tot_offset, (fnet_size_t)cur_len);
        tot_offset = 0u;          /* offset is only for the first peace of data*/

        u_buf = u_buf + cur_len; /* move the pointer for the next copy */
        tmp_nb = tmp_nb->next;   /* go to the next net_buf in the chain */
    }
    while (tot_len);
}

/************************************************************************
* DESCRIPTION: Frees the memory, which was allocated by net_buf 'nb' and
*              returns pointer to the next net_buf.
*************************************************************************/
fnet_netbuf_t *_fnet_netbuf_free( fnet_netbuf_t *nb )
{
    fnet_netbuf_t *tmp_nb;

    if(nb != 0)
    {
        if(((fnet_uint32_t *)nb->data)[0] == 1u)   /* If nobody uses this data buffer. */
        {
            _fnet_free_netbuf(nb->data);
        }
        else                           /* else decrement reference counter */
        {
            ((fnet_uint32_t *)nb->data)[0] = ((fnet_uint32_t *)nb->data)[0] - 1u;
        }

        tmp_nb = nb->next;

        _fnet_free_netbuf(nb);

        return (tmp_nb);
    }
    else
    {
        return (0);
    }
}

/************************************************************************
* DESCRIPTION: Frees the memory, which was allocated by all net_bufs
*              in the chain beginning from the buffer 'nb'
*************************************************************************/
void _fnet_netbuf_free_chain( fnet_netbuf_t *nb )
{
    fnet_netbuf_t *tmp_nb;

    while(nb != 0)
    {
        tmp_nb = nb->next;

        if(((fnet_uint32_t *)nb->data)[0] == 1u)   /* If nobody uses this data buffer. */
        {
            _fnet_free_netbuf(nb->data);
        }
        else                            /* Else decrement reference counter */
        {
            ((fnet_uint32_t *)nb->data)[0] = ((fnet_uint32_t *)nb->data)[0] - 1u;
        }

        _fnet_free_netbuf(nb);

        nb = tmp_nb;
    }
}

/************************************************************************
* DESCRIPTION: Create a data buffer for the first net_buf with the
*              length len
*************************************************************************/
fnet_return_t _fnet_netbuf_pullup( fnet_netbuf_t **nb_ptr, fnet_size_t len)
{
    fnet_netbuf_t   *nb = *nb_ptr;
    fnet_size_t     tot_len = 0u;
    fnet_size_t     offset;
    fnet_netbuf_t   *tmp_nb;
    fnet_netbuf_t   *nb_run;
    void            *new_buf;

    /* Check length*/
    if((nb == 0) || (len == 0u) || (nb->length >= len) || (nb->total_length < len))
    {
        /* if function shouldn't do anything*/
        return FNET_OK;
    }

    tmp_nb = nb;
    tot_len = nb->length;

    /* Search of the last buffer, from which the data have to be copied.
     * Stop at the last partial segment (not one past it) to avoid the
     * unsigned (len - offset) underflow below. */
    while((tot_len < len) && (tmp_nb->next != 0))
    {
        tmp_nb = tmp_nb->next;
        tot_len += tmp_nb->length;
    }

    new_buf = (struct net_buf_data *)_fnet_malloc_netbuf((fnet_size_t)len + sizeof(fnet_uint32_t)/* For reference_counter */);

    if(new_buf == 0)
    {
        return FNET_ERR;
    }

    ((fnet_uint32_t *)new_buf)[0] = 1u; /* First element is used by the reference_counter.*/

    /* Copy into it the contents of first data buffer. Skip the reference counter (placed in the first bytes). */
    fnet_memcpy(&((fnet_uint32_t *)new_buf)[1], nb->data_ptr, nb->length);
    offset = nb->length;

    /* Free old data buffer (for the first net_buf) */
    if(((fnet_uint32_t *)nb->data)[0] == 1u)   /* If nobody uses this data buffer. */
    {
        _fnet_free_netbuf(nb->data);
    }
    else                            /* Else decrement reference counter */
    {
        ((fnet_uint32_t *)nb->data)[0] = ((fnet_uint32_t *)nb->data)[0] - 1u;
    }

    /* Currently data buffer contains the contents of the first buffer */
    nb->data = &((fnet_uint32_t *)new_buf)[0];
    nb->data_ptr = &((fnet_uint32_t *)new_buf)[1];

    nb_run = nb->next;      /* Let's start from the next buffer */

    while(nb_run != tmp_nb) /* Copy full data buffers */
    {
        fnet_memcpy((fnet_uint8_t *)nb->data_ptr + offset, nb_run->data_ptr, nb_run->length);

        if(nb_run != tmp_nb)
        {
            offset += nb_run->length;
            nb_run = _fnet_netbuf_free(nb_run);
        }
    }

    tot_len = len - offset;

    /* Copy the remaining part and change data pointer and length of the
     * last net_buf, which is the source for the first net_buf */

    fnet_memcpy((fnet_uint8_t *)nb->data_ptr + offset, nb_run->data_ptr, tot_len);

    nb_run->length -= tot_len;

    if(nb_run->length == 0u)
    {
        nb_run = _fnet_netbuf_free(nb_run);
    }
    else
    {
        nb_run->data_ptr = (fnet_uint8_t *)nb_run->data_ptr + tot_len;
    }

    /* Setting up the params of the first net_buf.*/
    nb->next = nb_run;

    nb->length = (fnet_size_t)len;

    *nb_ptr = nb;

    return FNET_OK;
}


/************************************************************************
* DESCRIPTION: Trims len bytes from the begin of the net_buf data if len
*              is positive. Otherwise len bytes should be trimmed from the
*              end of net_buf buffer. If len=0 - do nothing
*************************************************************************/
void _fnet_netbuf_trim( fnet_netbuf_t **nb_ptr, fnet_int32_t len )
{
    fnet_netbuf_t   *head_nb;
    fnet_netbuf_t   *nb;
    fnet_size_t     tot_len;
    fnet_size_t     total_rem;
    fnet_flag_t     tmp_flags;

    if(len == 0)
    {
        return;
    }

    nb = (fnet_netbuf_t *) *nb_ptr;
    head_nb = nb;

    /* If the quantity of trimmed bytes is greater than net_buf size - do nothing.*/
    if((nb == 0) || (nb->total_length < (fnet_size_t)(len > 0 ? len : -len)))
    {
        return;
    }

    tot_len = nb->length;
    total_rem = nb->total_length;

    if(len > 0) /* Trim len bytes from the begin of the buffer.*/
    {
        /* If we delete the head_nb, we want to carry forward the flags */
        tmp_flags = nb->flags;

        while((nb != 0) && ((fnet_size_t)len >= tot_len))
        {
            *nb_ptr = nb->next;

            nb = _fnet_netbuf_free(nb); /* In some cases we delete some net_bufs.*/
            if(nb != 0)
            {
                tot_len += nb->length;
            }
        }

        if(nb != 0)
        {
            nb->data_ptr = (fnet_uint8_t *)nb->data_ptr + /* Or change pointer. */
                           nb->length - (tot_len - (fnet_size_t)len);
            nb->length = tot_len - (fnet_size_t)len;
            nb->flags = tmp_flags;
        }
    }
    else /* Trim len bytes from the end of the buffer. */
    {
        while((nb != 0) && ((fnet_int32_t)total_rem + len > (fnet_int32_t)tot_len))
        {
            nb = nb->next;         /* Run up to the first net_buf, which points */
            /* to the data, which should be erased.*/
            if(nb != 0)
            {
                tot_len += nb->length;
            }
        }

        /* Cut the part of the first net_buf, which should be modified.*/
        if(nb != 0)
        {
            nb->length += ((fnet_size_t)len + total_rem - tot_len);

            while(nb->next != 0) /* Cut the redundant net_bufs. */
            {
                nb->next = _fnet_netbuf_free(nb->next);
            }

            if(nb->length == 0u)  /* if |len| == total_length */
            {
                head_nb = _fnet_netbuf_free(nb);
            }

            nb = head_nb;
        }
    }

    if(nb != 0)
    {
        nb->total_length = (total_rem - (fnet_size_t)(len > 0 ? len : -len));
    }

    *nb_ptr = nb;

}

/************************************************************************
* DESCRIPTION: Cuts len bytes in net_buf queue starting from offset "offset"
*
*************************************************************************/
fnet_netbuf_t *_fnet_netbuf_cut_center( fnet_netbuf_t **nb_ptr, fnet_size_t offset, fnet_size_t len )
{
    fnet_netbuf_t   *head_nb, *tmp_nb, *nb;
    fnet_size_t     tot_len;

    if(len == 0u)
    {
        return (0);
    }

    nb = (fnet_netbuf_t *) *nb_ptr;

    if((nb == 0) || (nb->total_length < (len + offset)) )
    {
        return (0);
    }

    if(offset == 0u) /* The first case - when we cut from the begin of buffer.*/
    {
        _fnet_netbuf_trim(&nb, (fnet_int32_t)len);
        *nb_ptr = nb;
        return (nb);
    }

    head_nb = nb;

    tmp_nb = nb;

    tot_len = nb->length;

    while((nb != 0) && (offset >= tot_len))
    {
        nb = nb->next;                             /* Run up th the first net_buf, which points */
        if(nb)
        {
            tot_len += nb->length;                     /* to the data, which should be erased.*/

            if(offset >= tot_len)
            {
                tmp_nb = nb;                          /* To store previous pointer. */
            }
        }
    }

    if(nb == 0)
    {
        return 0; /* Should never happen - assert? */
    }

    if(tot_len - nb->length == offset)            /* If we start cut from the begin of buffer. */
    {
        nb->total_length = head_nb->total_length; /* For correct _fnet_netbuf_trim execution.*/
        _fnet_netbuf_trim(&tmp_nb->next, (fnet_int32_t)len);
        head_nb->total_length -= len;

        return ((fnet_netbuf_t *) *nb_ptr);
    }

    /* If only the middle of one net_buf should be cut.*/
    if(tot_len - offset > len)
    {
        head_nb->total_length -= len;

        /* Split one net_uf into two.*/
        if(((fnet_uint32_t *)nb->data)[0] == 1u) /* If we can simply erase them (reference_counter == 1).*/
        {
            fnet_memcpy((fnet_uint8_t *)nb->data_ptr + nb->length - tot_len + offset,
                        (fnet_uint8_t *)nb->data_ptr + nb->length - tot_len + offset + len,
                        (fnet_size_t)(tot_len - offset - len));
            nb->length -= len;
        }
        else
        {
            head_nb = _fnet_netbuf_new(tot_len - offset - len, FNET_FALSE);

            if(head_nb == 0) /* If no free memory.*/
            {
                nb = (fnet_netbuf_t *) *nb_ptr;
                nb->total_length += len;
                return (0);
            }

            fnet_memcpy(head_nb->data_ptr,
                        (fnet_uint8_t *)nb->data_ptr + nb->length - tot_len + offset + len,
                        (fnet_size_t)(tot_len - offset - len));

            head_nb->next = nb->next;

            nb->next = head_nb;

            nb->length -= tot_len - offset;
        }

        return ((fnet_netbuf_t *) *nb_ptr);
    }

    if(tot_len - offset == len) /* If we cut from the middle of buffer to the end only.*/
    {
        nb->length -= len;

        head_nb->total_length -= len;

        return ((fnet_netbuf_t *) *nb_ptr);
    }
    else                                /* Cut from the middle of net_buf to the end and trim remaining info*/
    {
        /* (tot_len-offset < len)*/
        nb->length -= tot_len - offset;

        nb->next->total_length = head_nb->total_length; /* For correct _fnet_netbuf_trim execution. */
        _fnet_netbuf_trim(&nb->next, (fnet_int32_t)(len - (tot_len - offset)));

        head_nb->total_length -= len;

        return ((fnet_netbuf_t *) *nb_ptr);
    }
}

/************************************************************************
* DESCRIPTION: Makes one net_buf from two. If the first or second pointer
*              is 0, the pointer to the second or first buffer
*              correspondingly is returned
*************************************************************************/
fnet_netbuf_t *_fnet_netbuf_concat( fnet_netbuf_t *nb1, fnet_netbuf_t *nb2 )
{
    fnet_netbuf_t *head_nb;

    if(nb1 == 0)
    {
        return nb2;
    }

    if(nb2 == 0)
    {
        return nb1;
    }

    head_nb = nb1;

    while(nb1->next != 0)
    {
        nb1 = nb1->next;
    }

    nb1->next = nb2;

    head_nb->flags |= nb2->flags;
    head_nb->total_length += nb2->total_length;

    return head_nb;
}

/************************************************************************
* DESCRIPTION: Adds chain nb_chain into the queue of chains pointed by nb_ptr
*************************************************************************/
void _fnet_netbuf_queue_add( fnet_netbuf_t **nb_ptr, fnet_netbuf_t *nb_chain )
{
    fnet_netbuf_t *nb;

    nb = (fnet_netbuf_t *) *nb_ptr;

    if(nb == 0)
    {
        *nb_ptr = nb_chain;
    }
    else
    {
        while(nb->next_chain)
        {
            nb = nb->next_chain;
        }

        nb->next_chain = nb_chain;
    }

}

/************************************************************************
* DESCRIPTION: Deletes chain nb_chain, which is in the queue pointed by nb_ptr
*************************************************************************/
void _fnet_netbuf_queue_del( fnet_netbuf_t **nb_ptr, fnet_netbuf_t *nb_chain )
{
    fnet_netbuf_t *nb_current, *nb;

    if(!((nb_ptr == 0) || ((fnet_netbuf_t *) *nb_ptr == 0)))
    {
        nb_current = (fnet_netbuf_t *) *nb_ptr;

        if(nb_current == nb_chain)
        {
            nb = nb_current->next_chain;
            _fnet_netbuf_free_chain(nb_current);
            *nb_ptr = nb;
        }
        else
        {
            while(nb_current && (nb_current->next_chain != nb_chain))
            {
                nb_current = nb_current->next_chain;
            }

            if(nb_current)
            {
                nb = nb_current->next_chain->next_chain;

                _fnet_netbuf_free_chain(nb_current->next_chain);

                nb_current->next_chain = nb;
            }
            /* else not found -> assert?*/
        }
    }
}

/************************************************************************
* DESCRIPTION: Heap init
*************************************************************************/
fnet_return_t _fnet_heap_init( void *heap_ptr, fnet_size_t heap_size )
{
    fnet_return_t result;

    /* Init memory pools. */
#if FNET_HEAP_SPLIT
    if(((fnet_mempool_main = _fnet_mempool_init( heap_ptr, heap_size, FNET_MEMPOOL_ALIGN_8 )) != 0) &&
       ((fnet_mempool_netbuf = _fnet_mempool_init( (void *)((fnet_uint32_t)_fnet_malloc( FNET_NETBUF_MEMPOOL_SIZE(heap_size))),
                               FNET_NETBUF_MEMPOOL_SIZE(heap_size), FNET_MEMPOOL_ALIGN_8 )) != 0) )

#else
    if((fnet_mempool_main = _fnet_mempool_init( heap_ptr, heap_size, FNET_MEMPOOL_ALIGN_8 )) != 0)
#endif
    {
        result = FNET_OK;
    }
    else
    {
        result = FNET_ERR;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Frees memory in heap for TCP/IP
*************************************************************************/
void _fnet_free_netbuf( void *ap )
{
    _fnet_mempool_free(fnet_mempool_netbuf, ap);
}

/************************************************************************
* DESCRIPTION: Allocates memory in heap for TCP/IP
*************************************************************************/
void *_fnet_malloc_netbuf( fnet_size_t nbytes )
{
    return _fnet_mempool_malloc( fnet_mempool_netbuf, nbytes );
}

/************************************************************************
* DESCRIPTION: Returns a quantity of free memory (for debug needs)
*************************************************************************/
fnet_size_t _fnet_free_mem_status_netbuf( void )
{
    return _fnet_mempool_free_mem_status( fnet_mempool_netbuf );
}

/************************************************************************
* DESCRIPTION: Returns a maximum size of posible allocated memory chunk.
*************************************************************************/
fnet_size_t _fnet_malloc_max_netbuf( void )
{
    return _fnet_mempool_malloc_max( fnet_mempool_netbuf  );
}

/************************************************************************
* DESCRIPTION: Free all Net Memory
*************************************************************************/
void _fnet_mem_release_netbuf( void )
{
    _fnet_mempool_release(fnet_mempool_netbuf);
}

/************************************************************************
* DESCRIPTION: Frees memory in heap for TCP/IP
*************************************************************************/
void _fnet_free( void *ap )
{
    _fnet_mempool_free(fnet_mempool_main, ap);
}

/************************************************************************
* DESCRIPTION: Allocates memory in heap for TCP/IP
*************************************************************************/
void *_fnet_malloc( fnet_size_t nbytes )
{
    return _fnet_mempool_malloc( fnet_mempool_main, nbytes );
}

/************************************************************************
* DESCRIPTION: Allocates memory in heap for TCP/IP
*************************************************************************/
void *_fnet_malloc_zero( fnet_size_t nbytes )
{
    void *result;

    result = _fnet_malloc(nbytes);
    if(result)
    {
        fnet_memset_zero(result, nbytes);
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Returns a quantity of free memory (for debug needs)
*************************************************************************/
fnet_size_t fnet_free_mem_status( void )
{
    fnet_size_t     result;

    _fnet_stack_mutex_lock();
    result = _fnet_free_mem_status();
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_size_t _fnet_free_mem_status( void )
{
    return  _fnet_mempool_free_mem_status( fnet_mempool_main );
}

/************************************************************************
* DESCRIPTION: Returns a maximum size of posible allocated memory chunk.
*************************************************************************/
fnet_size_t fnet_malloc_max( void )
{
    fnet_size_t     result;

    _fnet_stack_mutex_lock();
    result = _fnet_malloc_max();
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_size_t _fnet_malloc_max( void )
{
    return _fnet_mempool_malloc_max( fnet_mempool_main  );
}

/************************************************************************
* DESCRIPTION: Free all Net Memory
*************************************************************************/
void _fnet_mem_release( void )
{
    _fnet_mempool_release(fnet_mempool_main);
}

#if 0 /* For Debug needs.*/
fnet_return_t _fnet_netbuf_mempool_check( void )
{
    return _fnet_mempool_check(fnet_mempool_netbuf);
}

void _FNET_DEBUG_NETBUF_print_chain( fnet_netbuf_t *nb, fnet_uint8_t *str, fnet_index_t max)
{
    fnet_index_t i = 0u;
    fnet_println("== %s nb = 0x%08X ==", str, (fnet_uint32_t)nb);

    while(nb->next && i < max)
    {
        nb = nb->next;
        i++;
        fnet_println("\t(%d) next = 0x%08X", i, (fnet_uint32_t)nb);
    }
}
#endif




