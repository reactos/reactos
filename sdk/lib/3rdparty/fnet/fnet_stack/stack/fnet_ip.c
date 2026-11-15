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
*  IP general functions.
*
***************************************************************************/

#include "fnet_ip_prv.h"
#include "fnet_prot.h"
#include "fnet_socket_prv.h"

/************************************************************************
* DESCRIPTION: Appends IP input queue.
*************************************************************************/
fnet_return_t _fnet_ip_queue_append( fnet_ip_queue_t *queue, fnet_netif_t *netif, fnet_netbuf_t *nb )
{
    fnet_netbuf_t *nb_netif;

    fnet_isr_lock();

    if((nb->total_length + queue->count) > FNET_IP_QUEUE_COUNT_MAX)
    {
        goto ERROR;
    }

    if((nb_netif = _fnet_netbuf_new(sizeof(fnet_netif_t *), FNET_FALSE)) == 0)
    {
        goto ERROR;
    }

    nb_netif->data_ptr = (void *)netif;

    queue->count += nb->total_length;

    nb = _fnet_netbuf_concat(nb_netif, nb);
    _fnet_netbuf_queue_add(&queue->head, nb);

    fnet_isr_unlock();
    return FNET_OK;

ERROR:
    fnet_isr_unlock();
    return FNET_ERR;
}

/************************************************************************
* DESCRIPTION: Reads a IP datagram from IP input queue.
*************************************************************************/
fnet_netbuf_t *_fnet_ip_queue_read( fnet_ip_queue_t *queue, fnet_netif_t **netif )
{
    fnet_netbuf_t *nb;
    fnet_netbuf_t *nb_netif;

    if((nb_netif = queue->head) != 0)
    {
        nb = nb_netif->next;

        *netif = ((fnet_netif_t *)(nb_netif->data_ptr));

        fnet_isr_lock();

        if(nb)
        {
            queue->count -= nb->total_length;
        }

        nb_netif->next = 0;
        _fnet_netbuf_queue_del(&queue->head, nb_netif);

        fnet_isr_unlock();
    }
    else
    {
        nb = 0;
    }

    return nb;
}

/************************************************************************
* DESCRIPTION: This function sets the value of IP socket option.
*************************************************************************/
fnet_return_t _fnet_ip_setsockopt( fnet_socket_if_t *sock, fnet_protocol_t level, fnet_socket_options_t optname, const void *optval, fnet_size_t optlen )
{
    fnet_error_t error;

    if((optval) && (optlen))
    {
#if FNET_CFG_IP4
        if((level == IPPROTO_IP) && ((sock->protocol_interface->family & AF_INET) != 0u))
        {
            error = _fnet_ip4_setsockopt(sock, optname, optval, optlen);
            if(error != FNET_ERR_OK)
            {
                goto ERROR_SOCK;
            }
        }
        else
#endif
#if FNET_CFG_IP6
            if((level == IPPROTO_IPV6) && ((sock->protocol_interface->family & AF_INET6) != 0u))
            {
                error = _fnet_ip6_setsockopt(sock, optname, optval, optlen);
                if(error != FNET_ERR_OK)
                {
                    goto ERROR_SOCK;
                }
            }
            else

#endif
            {
                error = FNET_ERR_INVAL; /* Invalid argument.*/
                goto ERROR_SOCK;
            }
    }
    else
    {
        error = FNET_ERR_INVAL; /* Invalid argument.*/
        goto ERROR_SOCK;
    }

    return (FNET_OK);

ERROR_SOCK:
    sock->options.error = error;

    fnet_error_set(error);
    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: This function retrieves the current value
*              of IP-layer socket option.
*************************************************************************/
fnet_return_t _fnet_ip_getsockopt( fnet_socket_if_t *sock, fnet_protocol_t level, fnet_socket_options_t optname, void *optval, fnet_size_t *optlen )
{
    fnet_error_t error;

    if((optval) && (optlen) && (*optlen))
    {

#if FNET_CFG_IP4
        if((level == IPPROTO_IP) && ((sock->protocol_interface->family & AF_INET) != 0u))
        {
            error = _fnet_ip4_getsockopt(sock, optname, optval, optlen);
            if(error != FNET_ERR_OK)
            {
                goto ERROR_SOCK;
            }
        }
        else
#endif
#if FNET_CFG_IP6
            if((level == IPPROTO_IPV6) && ((sock->protocol_interface->family & AF_INET6) != 0u))
            {
                error = _fnet_ip6_getsockopt(sock, optname, optval, optlen);
                if(error != FNET_ERR_OK)
                {
                    goto ERROR_SOCK;
                }
            }
            else
#endif
            {
                error = FNET_ERR_INVAL; /* Invalid argument.*/
                goto ERROR_SOCK;
            }
    }
    else
    {
        error = FNET_ERR_INVAL; /* Invalid argument.*/
        goto ERROR_SOCK;
    }

    return (FNET_OK);

ERROR_SOCK:
    sock->options.error = error;

    fnet_error_set(error);
    return (FNET_ERR);
}

