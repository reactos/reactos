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
*  Socket interface implementation.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_stack_prv.h"
#include "fnet_socket_prv.h"
#include "fnet_timer_prv.h"
#include "fnet_prot.h"

/************************************************************************
*     Global Data Structures
*************************************************************************/
static fnet_uint16_t fnet_port_last = FNET_SOCKET_PORT_EPHEMERAL_BEGIN;

/* Array of sockets descriptors. */
static fnet_socket_if_t *fnet_socket_desc[FNET_CFG_SOCKET_MAX];

#if FNET_CFG_SOCKET_CALLBACK_ON_RX
    /* Socket activity event handler.*/
    static void (*fnet_socket_callback_on_rx)(void) ;
    fnet_event_desc_t fnet_socket_event_rx;
#endif

/************************************************************************
*     Function Prototypes
*************************************************************************/
static fnet_socket_t _fnet_socket_desc_alloc(void);
static void _fnet_socket_desc_set(fnet_socket_t desc, fnet_socket_if_t *sock);
static void _fnet_socket_desc_free(fnet_socket_t desc);
static fnet_socket_if_t *_fnet_socket_desc_find(fnet_socket_t desc);
static fnet_error_t _fnet_socket_addr_check_len(const struct fnet_sockaddr *addr, fnet_size_t addr_len);
#if FNET_CFG_SOCKET_CALLBACK_ON_RX
    static void _fnet_socket_is_activity(void *cookie);
#endif

/************************************************************************
* DESCRIPTION: Initialization of the socket layer.
*************************************************************************/
fnet_return_t _fnet_socket_init( void )
{
    fnet_memset_zero(fnet_socket_desc, sizeof(fnet_socket_desc));

#if FNET_CFG_SOCKET_CALLBACK_ON_RX
    /* Install event Handler. */
    fnet_socket_event_rx = fnet_event_init(_fnet_socket_is_activity, 0);
    if (fnet_socket_event_rx == 0)
    {
        return FNET_ERR;
    }
#endif
    return FNET_OK;
}

/************************************************************************
* DESCRIPTION: This function is called on the socket activity event.
*************************************************************************/
#if FNET_CFG_SOCKET_CALLBACK_ON_RX
static void _fnet_socket_is_activity(void *cookie)
{
    FNET_COMP_UNUSED_ARG(cookie);

    if(fnet_socket_callback_on_rx)
    {
        fnet_socket_callback_on_rx();
    }
}

/************************************************************************
* DESCRIPTION: Registers the "socket layer activity" event handler.
*************************************************************************/
void fnet_socket_set_callback_on_rx( void(*callback)(void))
{
    fnet_socket_callback_on_rx = callback;
}

#endif /* FNET_CFG_SOCKET_CALLBACK_ON_RX */

/************************************************************************
* DESCRIPTION: This function sets socket error.
*************************************************************************/
void _fnet_socket_set_error( fnet_socket_if_t *sock, fnet_error_t error )
{
    if(sock->options.local_error != FNET_ERR_OK)
    {
        error = sock->options.local_error;
        sock->options.local_error = FNET_ERR_OK;
    }

    sock->options.error = error;

    fnet_error_set(error);
}

/************************************************************************
* DESCRIPTION: This function adds socket into the queue.
*************************************************************************/
void _fnet_socket_list_add( fnet_socket_if_t **head, fnet_socket_if_t *s )
{
    fnet_isr_lock();
    s->next = *head;

    if(s->next != 0)
    {
        s->next->prev = s;
    }

    s->prev = 0;
    *head = s;
    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: This function removes socket from the queue
*************************************************************************/
void _fnet_socket_list_del( fnet_socket_if_t **head, fnet_socket_if_t *s )
{
    fnet_isr_lock();

    if(s->prev == 0)
    {
        *head = s->next;
    }
    else
    {
        s->prev->next = s->next;
    }

    if(s->next != 0)
    {
        s->next->prev = s->prev;
    }

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: This function reserves socket descriptor.
*************************************************************************/
static fnet_socket_t _fnet_socket_desc_alloc( void )
{
    fnet_index_t    i;
    fnet_socket_t   res = FNET_NULL;

    fnet_isr_lock();

    for (i = 0u; i < FNET_CFG_SOCKET_MAX; i++) /* Find the empty descriptor.*/
    {
        if(fnet_socket_desc[i] == 0)
        {
            fnet_socket_desc[i] = (fnet_socket_if_t *)FNET_SOCKET_DESC_RESERVED;
            res = &fnet_socket_desc[i];
            break;
        }
    }

    fnet_isr_unlock();

    return (res);
}

/************************************************************************
* DESCRIPTION: This function assigns the socket descriptor to the socket.
*************************************************************************/
static void _fnet_socket_desc_set( fnet_socket_t desc, fnet_socket_if_t *sock )
{
    *(fnet_socket_if_t **)desc = sock;
    sock->descriptor = desc;
}

/************************************************************************
* DESCRIPTION: This function frees the socket descriptor.
*************************************************************************/
static void _fnet_socket_desc_free( fnet_socket_t desc )
{
    *(fnet_socket_if_t **)desc = FNET_NULL;
}

/************************************************************************
* DESCRIPTION: This function looking for socket structure
*              associated with the socket descriptor.
*************************************************************************/
static fnet_socket_if_t *_fnet_socket_desc_find( fnet_socket_t desc )
{
    fnet_socket_if_t *s = FNET_NULL;

    if(_fnet_is_enabled && desc)
    {
        s = *(fnet_socket_if_t **)desc;
    }

    return (s);
}

/************************************************************************
* DESCRIPTION: This function release all resources allocated for the socket.
*************************************************************************/
void _fnet_socket_release( fnet_socket_if_t **head, fnet_socket_if_t *sock )
{
    fnet_isr_lock();
    _fnet_socket_list_del(head, sock);
    _fnet_socket_buffer_release(&sock->receive_buffer);
    _fnet_socket_buffer_release(&sock->send_buffer);
    _fnet_free(sock);
    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: Return FNET_TRUE if there's a socket whose addresses 'confict'
*              with the supplied addresses.
*************************************************************************/
fnet_bool_t _fnet_socket_conflict( fnet_socket_if_t *head,  const struct fnet_sockaddr *local_addr,
                                   const struct fnet_sockaddr *foreign_addr /*optional*/, fnet_bool_t wildcard )
{
    fnet_socket_if_t *sock = head;

    while(sock != 0)
    {
        if((sock->local_addr.sa_port == local_addr->sa_port)
           && (((fnet_socket_addr_is_unspecified(local_addr)) && (wildcard)) || (fnet_socket_addr_are_equal(&sock->local_addr, local_addr)) )
           && (((foreign_addr == 0) && (wildcard)) || (fnet_socket_addr_are_equal(&sock->foreign_addr, foreign_addr)))
           && (((foreign_addr == 0) && (wildcard)) || (foreign_addr && (sock->foreign_addr.sa_port == foreign_addr->sa_port))) )
        {
            return (FNET_TRUE);
        }

        sock = sock->next;
    }

    return (FNET_FALSE);
}

/************************************************************************
* DESCRIPTION: This function looks for a socket with the best match
*              to the local and foreign address parameters.
*************************************************************************/
fnet_socket_if_t *_fnet_socket_lookup( fnet_socket_if_t *head,  struct fnet_sockaddr *local_addr, struct fnet_sockaddr *foreign_addr, fnet_uint32_t protocol_number)
{
    fnet_socket_if_t   *sock;
    fnet_socket_if_t   *match_sock = 0;
    fnet_index_t    match_wildcard = 3u;
    fnet_index_t    wildcard;

    for (sock = head; sock != 0; sock = sock->next)
    {
        /* Compare protocol number.*/
        if(sock->protocol_number != protocol_number)
        {
            continue; /* Ignore. */
        }

        /* Compare local port number.*/
        if(sock->local_addr.sa_port != local_addr->sa_port)
        {
            continue; /* Ignore. */
        }

        /* Compare scope_id, if set.*/
        if(sock->local_addr.sa_scope_id)
        {
            if(sock->local_addr.sa_scope_id != local_addr->sa_scope_id)
            {
                continue; /* Ignore. */
            }
        }

        wildcard = 0u;

        /* Compare local address.*/
        if(!fnet_socket_addr_is_unspecified(&sock->local_addr))
        {
            if(fnet_socket_addr_is_unspecified(local_addr))
            {
                wildcard++;
            }
            else if(!fnet_socket_addr_are_equal(&sock->local_addr, local_addr))
            {
                continue;
            }
            else
            {}
        }
        else
        {
            if(!fnet_socket_addr_is_unspecified(local_addr))
            {
                wildcard++;
            }
        }

        /* Compare foreign address and port number.*/
        if(!fnet_socket_addr_is_unspecified(&sock->foreign_addr))
        {
            if(fnet_socket_addr_is_unspecified(foreign_addr))
            {
                wildcard++;
            }
            else if((!fnet_socket_addr_are_equal(&sock->foreign_addr, foreign_addr)) || (sock->foreign_addr.sa_port != foreign_addr->sa_port))
            {
                continue;
            }
            else
            {}
        }
        else
        {
            if(!fnet_socket_addr_is_unspecified(foreign_addr))
            {
                wildcard++;
            }
        }

        if(wildcard < match_wildcard)
        {
            match_sock = sock;

            if((match_wildcard = wildcard) == 0u)
            {
                break; /* Exact match is found.*/
            }
        }
    }

    return (match_sock);
}

/************************************************************************
* DESCRIPTION: Choose a unique (non-conflicting) local port for the socket
*              list starting at 'head'. The port will always be
*	           FNET_SOCKET_PORT_EPHEMERAL_BEGIN <= local_port <= FNET_SOCKET_PORT_EPHEMERAL_END (ephemeral port).
*              In network byte order.
*************************************************************************/
fnet_uint16_t _fnet_socket_get_uniqueport( fnet_socket_if_t *head, struct fnet_sockaddr *local_addr )
{
    fnet_uint16_t           local_port = fnet_port_last;
    struct fnet_sockaddr    local_addr_tmp;

    fnet_memcpy(&local_addr_tmp, local_addr, sizeof(local_addr_tmp));

    fnet_isr_lock();

    do
    {
        local_port++;
        if((local_port < FNET_SOCKET_PORT_EPHEMERAL_BEGIN) || (local_port >= FNET_SOCKET_PORT_EPHEMERAL_END))
        {
            local_port = (fnet_uint16_t)FNET_SOCKET_PORT_EPHEMERAL_BEGIN;
        }
        local_addr_tmp.sa_port = fnet_htons(local_port);
    }
    while (_fnet_socket_conflict(head, &local_addr_tmp, FNET_NULL, FNET_TRUE));

    fnet_port_last = local_port;

    fnet_isr_unlock();

    return local_addr_tmp.sa_port;
}

/************************************************************************
* DESCRIPTION: This function creates new socket structure and fills
*              its proper fields by values from existing socket
*************************************************************************/
fnet_socket_if_t *_fnet_socket_copy( fnet_socket_if_t *sock )
{
    fnet_socket_if_t *sock_cp;

    if((sock_cp = (fnet_socket_if_t *)_fnet_malloc(sizeof(fnet_socket_if_t))) != 0)
    {
        fnet_memcpy(sock_cp, sock, sizeof(fnet_socket_if_t));

        sock_cp->next = 0;
        sock_cp->prev = 0;
        sock_cp->descriptor = FNET_NULL;
        sock_cp->state = SS_CLOSED;
        sock_cp->protocol_control = 0;
        sock_cp->head_con = 0;
        sock_cp->partial_con = 0;
        sock_cp->incoming_con = 0;
        sock_cp->receive_buffer.count = 0u;
        sock_cp->receive_buffer.net_buf_chain = 0;
        sock_cp->send_buffer.count = 0u;
        sock_cp->send_buffer.net_buf_chain = 0;
        sock_cp->options.error = FNET_ERR_OK;
        sock_cp->options.local_error = FNET_ERR_OK;
        return (sock_cp);
    }
    else
    {
        return (0);
    }
}
/************************************************************************
* DESCRIPTION: This function creates a socket and returns
*              the descriptor to the application.
*************************************************************************/
fnet_socket_t fnet_socket( fnet_address_family_t family, fnet_socket_type_t type, fnet_uint32_t protocol )
{
    fnet_prot_if_t      *prot;
    fnet_socket_if_t    *sock;
    fnet_socket_t       res;
    fnet_error_t        error = FNET_ERR_OK;

    _fnet_stack_mutex_lock();

    if(_fnet_is_enabled == FNET_FALSE) /* Stack is disabled */
    {
        error = FNET_ERR_SYSNOTREADY;
        goto ERROR_1;
    }

    res = _fnet_socket_desc_alloc();

    if(res == FNET_NULL)
    {
        error = FNET_ERR_NO_DESC; /* No more socket descriptors are available.*/
        goto ERROR_1;
    }

    if((prot = _fnet_prot_find(family, type, (fnet_uint32_t) protocol)) == 0)
    {
        error = FNET_ERR_PROTONOSUPPORT; /* Protocol not supported.*/
        goto ERROR_2;
    }

    if((sock = (fnet_socket_if_t *)_fnet_malloc_zero(sizeof(fnet_socket_if_t))) == 0)
    {
        error = FNET_ERR_NOMEM; /* Cannot allocate memory.*/
        goto ERROR_2;
    }

    _fnet_socket_desc_set(res, sock);
    sock->protocol_interface = prot;
    sock->local_addr.sa_family = family;
    sock->state = SS_CLOSED;

    /* Save protocol number.*/
    sock->protocol_number = protocol;
    if(!sock->protocol_number)
    {
        sock->protocol_number = prot->protocol;
    }

    sock->foreign_addr.sa_family = family;

    _fnet_socket_list_add(&prot->head, sock);

    if((prot->socket_api->prot_attach) && (prot->socket_api->prot_attach(sock) == FNET_ERR))
    {
        _fnet_socket_release(&sock->protocol_interface->head, sock);
        error = fnet_error_get();
        goto ERROR_2;
    }

    _fnet_stack_mutex_unlock();

    return (res);

ERROR_2:
    _fnet_socket_desc_free(res);

ERROR_1:
    fnet_error_set(error);

    _fnet_stack_mutex_unlock();

    return (FNET_NULL);
}

/************************************************************************
* DESCRIPTION: This function establishes a connection to
*              a specified socket.
*************************************************************************/
fnet_return_t fnet_socket_connect( fnet_socket_t s, struct fnet_sockaddr *name, fnet_size_t namelen )
{
    fnet_socket_if_t        *sock;
    fnet_error_t            error = FNET_ERR_OK;
    struct fnet_sockaddr    foreign_addr;
    struct fnet_sockaddr    local_addr_tmp;
    fnet_return_t           result;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {
        if(sock->state == SS_LISTENING) /* The socket is marked to accept connections (listen).*/
        {
            error = FNET_ERR_OPNOTSUPP; /*  Operation not supported.*/
            goto ERROR_SOCK;
        }

        /* The protocol is connection oriented.*/
        if(sock->protocol_interface->socket_api->con_req)
        {
            /* A connection has already been initiated.*/
            if(sock->state == SS_CONNECTED)
            {
                error = FNET_ERR_ISCONN; /* Socket is already connected.*/
                goto ERROR_SOCK;
            }

            if(sock->state == SS_CONNECTING)
            {
                error = FNET_ERR_INPROGRESS; /* The action is in progress. */
                goto ERROR_SOCK;
            }
        }

        /* Check name, namelen values */
        if((error = _fnet_socket_addr_check_len(name, namelen)) != FNET_ERR_OK)
        {
            goto ERROR_SOCK;
        }

        foreign_addr = *name;

        if (fnet_socket_addr_is_unspecified(&foreign_addr))
        {
            error = FNET_ERR_DESTADDRREQ; /* Destination address required.*/
            goto ERROR_SOCK;
        }

        if((foreign_addr.sa_port == 0U) && (sock->protocol_interface->type != SOCK_RAW))
        {
            error = FNET_ERR_ADDRNOTAVAIL; /* Can't assign requested port.*/
            goto ERROR_SOCK;
        }

        fnet_memcpy(&local_addr_tmp, &sock->local_addr, sizeof(local_addr_tmp));
        local_addr_tmp.sa_family &= foreign_addr.sa_family; /* In case sa_family  is AF_SUPPORTED */

        if (fnet_socket_addr_is_unspecified(&local_addr_tmp))
        {
            fnet_netif_t *netif;

            netif = _fnet_netif_get_by_scope_id(foreign_addr.sa_scope_id);

            switch(local_addr_tmp.sa_family)
            {
#if FNET_CFG_IP4
                case AF_INET:
                {
                    if(netif == FNET_NULL)
                    {
                        if((netif = _fnet_ip4_route(((struct fnet_sockaddr_in *)(&foreign_addr))->sin_addr.s_addr)) == 0)
                        {
                            error = FNET_ERR_NETUNREACH; /* No route. */
                            goto ERROR_SOCK;
                        }
                    }

                    ((struct fnet_sockaddr_in *)(&local_addr_tmp))->sin_addr.s_addr = netif->ip4_addr.address;
                }
                break;
#endif /* FNET_CFG_IP4 */
#if FNET_CFG_IP6
                case AF_INET6:
                {
                    const fnet_ip6_addr_t *local_ip6_addr;
                    /* Check if can find a route to the destination.*/
                    if((local_ip6_addr = _fnet_ip6_select_src_addr(netif, &((struct fnet_sockaddr_in6 *)(&foreign_addr))->sin6_addr.s6_addr)) == FNET_NULL)
                    {
                        error = FNET_ERR_NETUNREACH; /* No route. */
                        goto ERROR_SOCK;
                    }

                    /* Init local address.*/
                    FNET_IP6_ADDR_COPY(local_ip6_addr, &((struct fnet_sockaddr_in6 *)(&local_addr_tmp))->sin6_addr.s6_addr);
                    ((struct fnet_sockaddr_in6 *)(&local_addr_tmp))->sin6_scope_id = ((struct fnet_sockaddr_in6 *)(&foreign_addr))->sin6_scope_id;
                }
                break;
#endif /* FNET_CFG_IP6 */
                default: /* Wrang family. */
                    error = FNET_ERR_AFNOSUPPORT;
                    goto ERROR_SOCK;
            }
        }

        if(local_addr_tmp.sa_port == 0u)
        {
            local_addr_tmp.sa_port = _fnet_socket_get_uniqueport(sock->protocol_interface->head,
                                     &local_addr_tmp); /* Get ephemeral port.*/
        }

        if(_fnet_socket_conflict(sock->protocol_interface->head, &local_addr_tmp, &foreign_addr, FNET_TRUE))
        {
            error = FNET_ERR_ADDRINUSE; /* Address already in use. */
            goto ERROR_SOCK;
        }

        fnet_memcpy(&sock->local_addr, &local_addr_tmp, sizeof(sock->local_addr));

        /* Start the appropriate protocol connection.*/
        if(sock->protocol_interface->socket_api->prot_connect)
        {
            result = sock->protocol_interface->socket_api->prot_connect(sock, &foreign_addr);
        }
        else
        {
            result = FNET_OK;
        }
    }
    else
    {
        error = FNET_ERR_BAD_DESC; /* Bad descriptor.*/
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (result);

ERROR_SOCK:
    _fnet_socket_set_error(sock, error);

ERROR:
    fnet_error_set(error);

    _fnet_stack_mutex_unlock();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: This function associates a local address with a socket.
*************************************************************************/
fnet_return_t fnet_socket_bind( fnet_socket_t s, const struct fnet_sockaddr *name, fnet_size_t namelen )
{
    fnet_socket_if_t   *sock;
    fnet_error_t    error = FNET_ERR_OK;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {
        if((error = _fnet_socket_addr_check_len(name, namelen)) != FNET_ERR_OK)
        {
            goto ERROR_SOCK;
        }

        if((sock->local_addr.sa_port == 0u) && (fnet_socket_addr_is_unspecified(&sock->local_addr)))
        {
            if(!fnet_socket_addr_is_multicast(name)) /* Is not multicast.*/
            {
                if((!fnet_socket_addr_is_unspecified(name)) && (!_fnet_socket_addr_is_broadcast(&sock->local_addr, FNET_NULL)) && (_fnet_netif_get_by_sockaddr(name) == FNET_NULL))
                {
                    /* The specified address is not a valid address for this system.*/
                    error = FNET_ERR_ADDRNOTAVAIL;
                    goto ERROR_SOCK;
                }

                if((name->sa_port != 0u)
                   && (_fnet_socket_conflict(sock->protocol_interface->head, name, FNET_NULL, FNET_FALSE)))
                {
                    error = FNET_ERR_ADDRINUSE; /* Address already in use. */
                    goto ERROR_SOCK;
                }
            }

            _fnet_socket_ip_addr_copy(name, &sock->local_addr);
            sock->local_addr.sa_scope_id = name->sa_scope_id;
            if((name->sa_port == 0u) && (sock->protocol_interface->type != SOCK_RAW))
            {
                sock->local_addr.sa_port = _fnet_socket_get_uniqueport(sock->protocol_interface->head, &sock->local_addr); /* Get ephemeral port.*/
            }
            else
            {
                sock->local_addr.sa_port = name->sa_port;
            }

            _fnet_socket_buffer_release(&sock->receive_buffer);
            _fnet_socket_buffer_release(&sock->send_buffer);
        }
        else
        {
            error = FNET_ERR_INVAL; /* The socket is already bound to an address.*/
            goto ERROR_SOCK;
        }
    }
    else
    {
        /* Bad descriptor.*/
        fnet_error_set(FNET_ERR_BAD_DESC);
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (FNET_OK);

ERROR_SOCK:
    _fnet_socket_set_error(sock, error);

ERROR:
    _fnet_stack_mutex_unlock();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: This function closes an existing socket.
*************************************************************************/
fnet_return_t fnet_socket_close( fnet_socket_t s )
{
    fnet_socket_if_t   *sock;
    fnet_return_t   result = FNET_OK;
    fnet_error_t    error;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {

#if FNET_CFG_MULTICAST
        /* Leave all IPv4 & IPv6 multicast groups.*/
        {
            fnet_index_t i;

            for(i = 0u; i < FNET_CFG_MULTICAST_SOCKET_MAX; i++)
            {
#if FNET_CFG_IP4
                if (sock->ip4_multicast_entry[i] != FNET_NULL)
                {
                    _fnet_ip4_multicast_leave_entry(sock->ip4_multicast_entry[i]);
                }
#endif
#if FNET_CFG_IP6
                if (sock->ip6_multicast_entry[i] != FNET_NULL)
                {
                    _fnet_ip6_multicast_leave_entry(sock->ip6_multicast_entry[i]);
                }
#endif
            }
        }
#endif

        if(sock->protocol_interface->socket_api->prot_detach)
        {
            result = sock->protocol_interface->socket_api->prot_detach(sock);
        }

        if(result == FNET_OK)
        {
            _fnet_socket_desc_free(s);
        }
    }
    else
    {
        error = FNET_ERR_BAD_DESC; /* Bad descriptor.*/
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (result);
ERROR:
    fnet_error_set(error);

    _fnet_stack_mutex_unlock();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: This function to disable reception, transmission, or both.
*************************************************************************/
fnet_return_t fnet_socket_shutdown( fnet_socket_t s, fnet_sd_flags_t how )
{
    fnet_socket_if_t   *sock;
    fnet_return_t   result = FNET_OK;
    fnet_error_t    error;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {
        if((sock->protocol_interface) && (sock->protocol_interface->socket_api->prot_shutdown))
        {
            result = sock->protocol_interface->socket_api->prot_shutdown(sock, how);
        }
    }
    else
    {
        error = FNET_ERR_BAD_DESC; /* Bad descriptor.*/
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (result);
ERROR:
    fnet_error_set(error);

    _fnet_stack_mutex_unlock();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: This function places the socket into the state where
*              it is listening for an incoming connection.
*************************************************************************/
fnet_return_t fnet_socket_listen( fnet_socket_t s, fnet_size_t backlog )
{
    fnet_socket_if_t   *sock;
    fnet_error_t    error;
    fnet_return_t   result = FNET_OK;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {
        if((sock->state == SS_CONNECTING) || (sock->state == SS_CONNECTED))
        {
            error = FNET_ERR_ISCONN; /* Socket is already connected.*/
            goto ERROR_SOCK;
        }

        if(sock->local_addr.sa_port == 0u)
        {
            error = FNET_ERR_BOUNDREQ; /* The socket has not been bound.*/
            goto ERROR_SOCK;
        }

        if((sock->protocol_interface) && (sock->protocol_interface->socket_api->prot_listen))
        {
            result = sock->protocol_interface->socket_api->prot_listen(sock, backlog);
        }
        else
        {
            error = FNET_ERR_OPNOTSUPP; /* Operation not supported.*/
            goto ERROR_SOCK;
        }
    }
    else
    {
        fnet_error_set(FNET_ERR_BAD_DESC);/* Bad descriptor.*/
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (result);

ERROR_SOCK:
    _fnet_socket_set_error(sock, error);

ERROR:
    _fnet_stack_mutex_unlock();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: This function accepts a connection on a specified socket.
*************************************************************************/
fnet_socket_t fnet_socket_accept( fnet_socket_t s, struct fnet_sockaddr *addr, fnet_size_t *addrlen )
{
    fnet_socket_if_t   *sock;
    fnet_socket_if_t   *sock_new;
    fnet_socket_t       desc;
    fnet_error_t        error;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {
        if((sock->protocol_interface) && (sock->protocol_interface->socket_api->prot_accept))
        {
            if(sock->state != SS_LISTENING)
            {
                error = FNET_ERR_INVAL; /* Invalid argument.*/
                goto ERROR_SOCK;
            }

            if(addr && addrlen)
            {
                if((error = _fnet_socket_addr_check_len(&sock->local_addr, (*addrlen) )) != FNET_ERR_OK )
                {
                    goto ERROR_SOCK;
                }
            }

            desc = _fnet_socket_desc_alloc();
            if(desc)
            {
                fnet_isr_lock();

                if((sock_new = sock->protocol_interface->socket_api->prot_accept(sock)) == 0)
                {
                    _fnet_socket_desc_free(desc);
                    fnet_isr_unlock();
                    error = FNET_ERR_AGAIN;
                    goto ERROR_SOCK;
                }

                _fnet_socket_desc_set(desc, sock_new);
                _fnet_socket_list_add(&sock->protocol_interface->head, sock_new);

                fnet_isr_unlock();

                if(addr && addrlen)
                {
                    _fnet_socket_addr_copy(&sock_new->foreign_addr, addr);
                }
            }
            else
            {
                error = FNET_ERR_NO_DESC; /* No more socket descriptors are available.*/
                goto ERROR_SOCK;
            }
        }
        else
        {
            error = FNET_ERR_OPNOTSUPP; /*  Operation not supported.*/
            goto ERROR_SOCK;
        }
    }
    else
    {
        fnet_error_set(FNET_ERR_BAD_DESC);/* Bad descriptor.*/
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (desc);

ERROR_SOCK:
    _fnet_socket_set_error(sock, error);

ERROR:
    _fnet_stack_mutex_unlock();

    return (FNET_NULL);
}

/************************************************************************
* DESCRIPTION: This function sends data to a specific destination.
*************************************************************************/
fnet_ssize_t fnet_socket_sendto( fnet_socket_t s, const void *buf, fnet_size_t len, fnet_flag_t flags, const struct fnet_sockaddr *to, fnet_size_t tolen )
{
    fnet_socket_if_t    *sock;
    fnet_error_t        error;
    fnet_ssize_t        result = 0;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {
        if((to == FNET_NULL) || (tolen == 0u))
        {
            if(fnet_socket_addr_is_unspecified(&sock->foreign_addr))
            {
                error = FNET_ERR_NOTCONN; /* Socket is not connected.*/
                goto ERROR_SOCK;
            }

            to = FNET_NULL;
        }
        else
        {
            if((error = _fnet_socket_addr_check_len(to, tolen)) != FNET_ERR_OK)
            {
                goto ERROR_SOCK;
            }

            if(fnet_socket_addr_is_unspecified(to))
            {
                error = FNET_ERR_DESTADDRREQ; /* Destination address required.*/
                goto ERROR_SOCK;
            }
        }

        if(buf)
        {
            /* If the socket is shutdowned, return.*/
            if(sock->send_buffer.is_shutdown)
            {
                error = FNET_ERR_SHUTDOWN;
                goto ERROR_SOCK;
            }

            if(sock->protocol_interface->socket_api->prot_snd)
            {
                result = sock->protocol_interface->socket_api->prot_snd(sock, (const fnet_uint8_t *)buf, len, flags, to);
            }
            else
            {
                error = FNET_ERR_OPNOTSUPP; /* Operation not supported.*/
                goto ERROR_SOCK;
            }
        }
        else
        {
            error = FNET_ERR_INVAL; /* Invalid argument.*/
            goto ERROR_SOCK;
        }
    }
    else
    {
        fnet_error_set(FNET_ERR_BAD_DESC);/* Bad descriptor.*/
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (result);

ERROR_SOCK:
    _fnet_socket_set_error(sock, error);

ERROR:
    _fnet_stack_mutex_unlock();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: This function sends data on a connected socket.
*************************************************************************/
fnet_ssize_t fnet_socket_send( fnet_socket_t s, const void *buf, fnet_size_t len, fnet_flag_t flags )
{
    return fnet_socket_sendto(s, buf, len, flags, FNET_NULL, 0u);
}

/************************************************************************
* DESCRIPTION: This function reads incoming data of socket and captures
*              the address from which the data was sent.
*************************************************************************/
fnet_ssize_t fnet_socket_recvfrom( fnet_socket_t s, void *buf, fnet_size_t len, fnet_flag_t flags, struct fnet_sockaddr *from, fnet_size_t *fromlen )
{
    fnet_socket_if_t   *sock;
    fnet_error_t    error;
    fnet_int32_t    result = 0;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {
        if(buf)
        {

            /* The sockets must be bound before calling recv.*/
            if((sock->local_addr.sa_port == 0u) && (sock->protocol_interface->type != SOCK_RAW))
            {
                error = FNET_ERR_BOUNDREQ; /* The socket has not been bound with fnet_socket_bind().*/
                goto ERROR_SOCK;
            }

            if(from && fromlen)
            {
                if((error = _fnet_socket_addr_check_len(&sock->local_addr, (*fromlen) )) != FNET_ERR_OK )
                {
                    goto ERROR_SOCK;
                }
            }

            /* If the socket is shutdowned, return.*/
            if(sock->receive_buffer.is_shutdown)
            {
                error = FNET_ERR_SHUTDOWN;
                goto ERROR_SOCK;
            }

            if(sock->protocol_interface->socket_api->prot_rcv)
            {
                result = sock->protocol_interface->socket_api->prot_rcv(sock, buf, len, flags, (from && fromlen) ? from : FNET_NULL);
            }
            else
            {
                error = FNET_ERR_OPNOTSUPP; /* Operation not supported.*/
                goto ERROR_SOCK;
            }
        }
        else
        {
            error = FNET_ERR_INVAL; /* Invalid argument.*/
            goto ERROR_SOCK;
        }
    }
    else
    {
        fnet_error_set(FNET_ERR_BAD_DESC);/* Bad descriptor.*/
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (result);

ERROR_SOCK:
    _fnet_socket_set_error(sock, error);

ERROR:
    _fnet_stack_mutex_unlock();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: This function receives data from a connected socket.
*************************************************************************/
fnet_ssize_t fnet_socket_recv( fnet_socket_t s, void *buf, fnet_size_t len, fnet_flag_t flags )
{
    return fnet_socket_recvfrom(s, buf, len, flags, FNET_NULL, FNET_NULL);
}

/************************************************************************
* DESCRIPTION: This function retrieves the current name
*              for the specified socket.
*************************************************************************/
fnet_return_t fnet_socket_getname( fnet_socket_t s, struct fnet_sockaddr *name, fnet_size_t *namelen )
{
    fnet_socket_if_t   *sock;
    fnet_error_t    error;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {
        if((name == 0) || (namelen == 0))
        {
            error = FNET_ERR_INVAL;
            goto ERROR_SOCK;
        }

        if((error = _fnet_socket_addr_check_len(&sock->local_addr, (*namelen) )) != FNET_ERR_OK )
        {
            goto ERROR_SOCK;
        }

        if((sock->local_addr.sa_port == 0U) && (sock->protocol_interface->type != SOCK_RAW))
        {
            error = FNET_ERR_BOUNDREQ; /* The socket has not been bound with fnet_socket_bind().*/
            goto ERROR_SOCK;
        }

        _fnet_socket_addr_copy(&sock->local_addr, name);
    }
    else
    {
        fnet_error_set(FNET_ERR_BAD_DESC);/* Bad descriptor.*/
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (FNET_OK);

ERROR_SOCK:
    _fnet_socket_set_error(sock, error);

ERROR:
    _fnet_stack_mutex_unlock();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: This function retrieves the name of the peer
*              connected to the socket
*************************************************************************/
fnet_return_t fnet_socket_getpeername( fnet_socket_t s, struct fnet_sockaddr *name, fnet_size_t *namelen )
{
    fnet_socket_if_t   *sock;
    fnet_error_t    error;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {
        if((name == 0) || (namelen == 0) )
        {
            error = FNET_ERR_INVAL;
            goto ERROR_SOCK;
        }

        if((error = _fnet_socket_addr_check_len(&sock->local_addr, (*namelen) )) != FNET_ERR_OK )
        {
            goto ERROR_SOCK;
        }

        if(fnet_socket_addr_is_unspecified(&sock->foreign_addr))
        {
            error = FNET_ERR_NOTCONN; /* Socket is not connected.*/
            goto ERROR_SOCK;
        }

        _fnet_socket_addr_copy(&sock->foreign_addr, name);
    }
    else
    {
        fnet_error_set(FNET_ERR_BAD_DESC);/* Bad descriptor.*/
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (FNET_OK);

ERROR_SOCK:
    _fnet_socket_set_error(sock, error);

ERROR:
    _fnet_stack_mutex_unlock();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: This function sets the current value for a socket option
*              associated with a socket
*************************************************************************/
fnet_return_t fnet_socket_setopt( fnet_socket_t s, fnet_protocol_t level, fnet_socket_options_t optname, const void *optval, fnet_size_t optvallen )
{
    fnet_socket_if_t   *sock;
    fnet_error_t    error;
    fnet_return_t   result = FNET_OK;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {
        if(optval && optvallen)
        {
            if(level != SOL_SOCKET)
            {
                if((sock->protocol_interface) && (sock->protocol_interface->socket_api->prot_setsockopt))
                {
                    result = sock->protocol_interface->socket_api->prot_setsockopt(sock, level, optname, optval, optvallen);
                }
                else
                {
                    error = FNET_ERR_INVAL; /* Invalid argument.*/
                    goto ERROR_SOCK;
                }
            }
            else
            {
                switch(optname)     /* Socket options processing.*/
                {
                    case SO_LINGER: /* Linger on close if data present.*/
                        if(optvallen < sizeof(struct fnet_linger))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        sock->options.linger_ms = (fnet_time_t)((const struct fnet_linger *)optval)->l_linger * 1000u;
                        sock->options.so_linger = (((const struct fnet_linger *)optval)->l_onoff) ? FNET_TRUE : FNET_FALSE;
                        break;
                    case SO_KEEPALIVE: /* Keep connections alive.*/
                        if(optvallen < sizeof(fnet_uint32_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }
                        sock->options.so_keepalive = (*((const fnet_uint32_t *)optval)) ? FNET_TRUE : FNET_FALSE;
                        break;
                    case SO_DONTROUTE: /* Just use interface addresses.*/
                        if(optvallen < sizeof(fnet_uint32_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }
                        sock->options.so_dontroute = (*((const fnet_uint32_t *)optval)) ? FNET_TRUE : FNET_FALSE;
                        break;
#if FNET_CFG_TCP_URGENT
                    case SO_OOBINLINE: /* Leave received OOB data in line.*/
                        if(optvallen < sizeof(fnet_uint32_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }
                        sock->options.so_oobinline = (*((const fnet_uint32_t *)optval)) ? FNET_TERUE : FNET_FALSE;
                        break;
#endif
                    case SO_SNDBUF: /* Send buffer size.*/
                    case SO_RCVBUF: /* Receive buffer size.*/
                        if((optvallen < sizeof(fnet_uint32_t)))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        if(optname == SO_SNDBUF)
                        {
                            sock->send_buffer.count_max = *((const fnet_uint32_t *)optval);
                        }
                        else
                        {
                            sock->receive_buffer.count_max = *((const fnet_uint32_t *)optval);
                        }

                        break;
                    default:
                        error = FNET_ERR_NOPROTOOPT; /* The option is unknown or unsupported. */
                        goto ERROR_SOCK;
                }
            }
        }
        else
        {
            error = FNET_ERR_INVAL; /* Invalid argument.*/
            goto ERROR_SOCK;
        }
    }
    else
    {
        fnet_error_set(FNET_ERR_BAD_DESC);/* Bad descriptor.*/
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (result);

ERROR_SOCK:
    _fnet_socket_set_error(sock, error);

ERROR:
    _fnet_stack_mutex_unlock();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: This function retrieves the current value for
*              a socket option associated with a socket
*************************************************************************/
fnet_return_t fnet_socket_getopt( fnet_socket_t s, fnet_protocol_t level, fnet_socket_options_t optname, void *optval, fnet_size_t *optvallen )
{
    fnet_socket_if_t   *sock;
    fnet_error_t    error;
    fnet_return_t   result = FNET_OK;

    _fnet_stack_mutex_lock();

    if((sock = _fnet_socket_desc_find(s)) != 0)
    {
        if(optval && optvallen)
        {
            if(level != SOL_SOCKET)
            {
                if((sock->protocol_interface) && (sock->protocol_interface->socket_api->prot_getsockopt))
                {
                    result = sock->protocol_interface->socket_api->prot_getsockopt(sock, level, optname, optval, optvallen);
                }
                else
                {
                    error = FNET_ERR_INVAL; /* Invalid argument.*/
                    goto ERROR_SOCK;
                }
            }
            else
            {
                switch(optname)     /* Socket options processing.*/
                {
                    case SO_LINGER: /* Linger on close if data present.*/
                        if(*optvallen < sizeof(struct fnet_linger))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        *optvallen = sizeof(struct fnet_linger);
                        ((struct fnet_linger *)optval)->l_onoff = sock->options.so_linger;
                        ((struct fnet_linger *)optval)->l_linger = (fnet_uint16_t)(sock->options.linger_ms / 1000u);
                        break;
                    case SO_KEEPALIVE: /* Keep connections alive.*/
                        if(*optvallen < sizeof(fnet_uint32_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }
                        *optvallen = sizeof(fnet_uint32_t);
                        *((fnet_uint32_t *)optval) = (fnet_uint32_t)(sock->options.so_keepalive);
                        break;
                    case SO_DONTROUTE: /* Just use interface addresses.*/
                        if(*optvallen < sizeof(fnet_uint32_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }
                        *optvallen = sizeof(fnet_uint32_t);
                        *((fnet_uint32_t *)optval) = (fnet_uint32_t)(sock->options.so_dontroute);
                        break;
#if FNET_CFG_TCP_URGENT
                    case SO_OOBINLINE: /* Leave received OOB data in line.*/
                        if(*optvallen < sizeof(fnet_uint32_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        *optvallen = sizeof(fnet_uint32_t);
                        *((fnet_uint32_t *)optval) = (fnet_uint32_t)(sock->options.so_oobinline);
                        break;
#endif
                    case SO_ACCEPTCONN: /* Socket is listening. */
                        if(*optvallen < sizeof(fnet_uint32_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        *optvallen = sizeof(fnet_uint32_t);
                        *((fnet_uint32_t *)optval) = (fnet_uint32_t)(sock->state == SS_LISTENING);
                        break;
                    case SO_SNDBUF: /* Send buffer size.*/
                    case SO_RCVBUF: /* Receive buffer size.*/
                        if(*optvallen < sizeof(fnet_uint32_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        *optvallen = sizeof(fnet_uint32_t);

                        if(optname == SO_SNDBUF)
                        {
                            *((fnet_uint32_t *)optval) = sock->send_buffer.count_max;
                        }
                        else
                        {
                            *((fnet_uint32_t *)optval) = sock->receive_buffer.count_max;
                        }
                        break;
                    case SO_STATE: /* State of the socket.*/
                        if(*optvallen < sizeof(fnet_socket_state_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        *optvallen = sizeof(fnet_socket_state_t);
                        *((fnet_socket_state_t *)optval) = sock->state;
                        break;

                    case SO_RCVNUM:  /* Use to determine the amount of data pending in the network's input buffer that can be read from socket.*/
                    case SO_SNDNUM: /* Use to determine the amount of data in the network's output buffer.*/
                        if(*optvallen < sizeof(fnet_uint32_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        *optvallen = sizeof(fnet_uint32_t);

                        if(optname == SO_RCVNUM)
                        {
                            *((fnet_uint32_t *)optval) = sock->receive_buffer.count;
                        }
                        else
                        {
                            *((fnet_uint32_t *)optval) = sock->send_buffer.count;
                        }
                        break;
                    case SO_ERROR: /* Socket error.*/
                        if(*optvallen < sizeof(fnet_error_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        *optvallen = sizeof(fnet_error_t);
                        *((fnet_error_t *)optval) = sock->options.error;
                        sock->options.error = FNET_ERR_OK; /* Reset error.*/
                        break;
                    case SO_TYPE:
                        if(*optvallen < sizeof(fnet_socket_type_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        *optvallen = sizeof(fnet_socket_type_t);
                        *((fnet_socket_type_t *)optval) = (sock->protocol_interface ? sock->protocol_interface->type : SOCK_UNSPEC);
                        break;
                    case SO_LISTENQLIMIT:
                        if(*optvallen < sizeof(fnet_uint32_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        *optvallen = sizeof(fnet_uint32_t);
                        *((fnet_uint32_t *)optval) = sock->con_limit;
                        break;
                    case SO_LISTENQLEN:
                        if(*optvallen < sizeof(fnet_uint32_t))
                        {
                            error = FNET_ERR_INVAL;
                            goto ERROR_SOCK;
                        }

                        *optvallen = sizeof(fnet_uint32_t);
                        *((fnet_uint32_t *)optval) = sock->incoming_con_len;
                        break;
                    default:
                        error = FNET_ERR_NOPROTOOPT; /* The option is unknown or unsupported. */
                        goto ERROR_SOCK;
                }/* case*/
            }/* else */
        }
        else
        {
            error = FNET_ERR_INVAL; /* Invalid argument.*/
            goto ERROR_SOCK;
        }
    }
    else
    {
        fnet_error_set(FNET_ERR_BAD_DESC);/* Bad descriptor.*/
        goto ERROR;
    }

    _fnet_stack_mutex_unlock();

    return (result);

ERROR_SOCK:
    _fnet_socket_set_error(sock, error);

ERROR:
    _fnet_stack_mutex_unlock();

    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION: Discards any buffers in the socket buffer
*************************************************************************/
void _fnet_socket_buffer_release( fnet_socket_buffer_t *sb )
{
    fnet_netbuf_t   *nb_ptr;
    fnet_netbuf_t   *tmp_nb_ptr;

    fnet_isr_lock();

    if(sb && (sb->net_buf_chain))
    {
        nb_ptr = sb->net_buf_chain;

        while(nb_ptr != 0)
        {
            tmp_nb_ptr = nb_ptr->next_chain;
            _fnet_netbuf_free_chain(nb_ptr);
            nb_ptr = tmp_nb_ptr;
        }

        sb->net_buf_chain = 0;
        sb->count = 0u;
    }

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: Append the record to the end of the socket buffer.
*************************************************************************/
fnet_return_t _fnet_socket_buffer_append_record( fnet_socket_buffer_t *sb, fnet_netbuf_t *nb )
{
    fnet_isr_lock();

    if((nb->total_length + sb->count) > sb->count_max)
    {
        fnet_isr_unlock();
        return FNET_ERR;
    }

    sb->net_buf_chain = _fnet_netbuf_concat(sb->net_buf_chain, nb);

    sb->count += nb->total_length;
    fnet_isr_unlock();

    return FNET_OK;
}

/************************************************************************
* DESCRIPTION: Constract net_buf chain  and add it to the queue.
*              The chain contains the address of the message
*              and the message data.
*************************************************************************/
fnet_return_t _fnet_socket_buffer_append_address( fnet_socket_buffer_t *sb, fnet_netbuf_t *nb, struct fnet_sockaddr *addr)
{
    fnet_socket_buffer_addr_t   *sb_address;
    fnet_netbuf_t               *nb_addr;

    fnet_isr_lock();

    if((nb->total_length + sb->count) > sb->count_max)
    {
        goto ERROR;
    }

    if((nb_addr = _fnet_netbuf_new(sizeof(fnet_socket_buffer_addr_t), FNET_FALSE)) == 0)
    {
        goto ERROR;
    }

    sb_address = (fnet_socket_buffer_addr_t *)nb_addr->data_ptr;

    fnet_memcpy(&sb_address->addr_s, addr, sizeof(sb_address->addr_s));

    sb->count += nb->total_length;

    nb = _fnet_netbuf_concat(nb_addr, nb);
    _fnet_netbuf_queue_add(&sb->net_buf_chain, nb);
    fnet_isr_unlock();

#if FNET_CFG_SOCKET_CALLBACK_ON_RX
    /* Wake-up user application.*/
    fnet_event_raise(fnet_socket_event_rx);
#endif

    return FNET_OK;

ERROR:
    fnet_isr_unlock();
    return FNET_ERR;
}

/************************************************************************
* DESCRIPTION: This function reads data from socket buffer and
*              put this data into application buffer.
*************************************************************************/
fnet_size_t _fnet_socket_buffer_read_record( fnet_socket_buffer_t *sb, fnet_uint8_t *buf, fnet_size_t len, fnet_bool_t remove )
{
    if(sb->net_buf_chain)
    {
        if(len > sb->net_buf_chain->total_length)
        {
            len = sb->net_buf_chain->total_length;
        }

        _fnet_netbuf_to_buf(sb->net_buf_chain, 0u, len, buf);

        if(remove == FNET_TRUE)
        {
            fnet_isr_lock();
            _fnet_netbuf_trim(&sb->net_buf_chain, (fnet_int32_t)len);
            sb->count -= len;
            fnet_isr_unlock();
        }
    }
    else
    {
        len = 0u;
    }

    return len;
}

/************************************************************************
* DESCRIPTION:This function reads data from socket buffer and
*             put this data into application buffer.
*             And captures the address information from which the data was sent.
*************************************************************************/
fnet_int32_t _fnet_socket_buffer_read_address( fnet_socket_buffer_t *sb, fnet_uint8_t *buf, fnet_size_t len, struct fnet_sockaddr *foreign_addr, fnet_bool_t remove )
{
    fnet_netbuf_t   *nb;
    fnet_netbuf_t   *nb_addr;

    if(((nb_addr = sb->net_buf_chain) != 0) )
    {
        if((nb = nb_addr->next) != 0)
        {
            if(len > nb->total_length)
            {
                len = nb->total_length;
            }

            _fnet_netbuf_to_buf(nb, 0u, len, buf);
        }
        else
        {
            len = 0u;
        }

        fnet_memcpy(foreign_addr, &((fnet_socket_buffer_addr_t *)(nb_addr->data_ptr))->addr_s, sizeof(*foreign_addr));

        if(len < (nb_addr->total_length - sizeof(fnet_socket_buffer_addr_t)))
        {
            len = (fnet_size_t)FNET_ERR;
        }

        if(remove)
        {
            fnet_isr_lock();

            if(nb)
            {
                sb->count -= nb->total_length;
            }
            _fnet_netbuf_queue_del(&sb->net_buf_chain, nb_addr);

            fnet_isr_unlock();
        }
    }
    else
    {
        len = 0u;
    }

    return (fnet_int32_t)len;
}

/************************************************************************
* DESCRIPTION: This function check fnet_sockaddr structure and its size.
*************************************************************************/
static fnet_error_t _fnet_socket_addr_check_len(const struct fnet_sockaddr *addr, fnet_size_t addr_len )
{
    fnet_error_t result = FNET_ERR_OK;

    if(addr && addr_len)
    {
#if FNET_CFG_IP6
        if((addr->sa_family & AF_INET6) != 0u)
        {
            if(addr_len < sizeof(struct fnet_sockaddr_in6))
            {
                result = FNET_ERR_INVAL;    /* Invalid argument.*/
            }
        }
        else
#endif /* FNET_CFG_IP4 */

#if FNET_CFG_IP4
            if((addr->sa_family & AF_INET) != 0u )
            {
                if(addr_len < sizeof(struct fnet_sockaddr_in))
                {
                    result = FNET_ERR_INVAL;    /* Invalid argument.*/
                }
            }
            else
#endif /* FNET_CFG_IP4 */
            {
                result = FNET_ERR_AFNOSUPPORT;
            }
    }
    else
    {
        result = FNET_ERR_INVAL;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Returns FNET_FALSE if the address is not multicast.
*************************************************************************/
fnet_bool_t fnet_socket_addr_is_multicast(const struct fnet_sockaddr *addr)
{
    fnet_bool_t     result = FNET_FALSE;

    if(addr)
    {
#if FNET_CFG_IP6
        if((addr->sa_family & AF_INET6) != 0u)
        {
            result = FNET_IP6_ADDR_IS_MULTICAST( &((const struct fnet_sockaddr_in6 *)addr)->sin6_addr.s6_addr);
        }
        else
#endif /* FNET_CFG_IP4 */

#if FNET_CFG_IP4
            if((addr->sa_family & AF_INET) != 0u)
            {
                result = FNET_IP4_ADDR_IS_MULTICAST( ((const struct fnet_sockaddr_in *)addr)->sin_addr.s_addr);
            }
            else
#endif /* FNET_CFG_IP4 */
            {}
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Returns FNET_FALSE if the address is not broadcast.
*************************************************************************/
fnet_bool_t _fnet_socket_addr_is_broadcast(const struct fnet_sockaddr *addr, fnet_netif_t *netif)
{
    fnet_bool_t result = FNET_FALSE;

#if FNET_CFG_IP4
    if(addr)
    {
        if((addr->sa_family & AF_INET) != 0u)
        {
            result = _fnet_ip4_addr_is_broadcast( ((const struct fnet_sockaddr_in *)addr)->sin_addr.s_addr, netif );
        }
    }
#else
    FNET_COMP_UNUSED_ARG(netif);
    FNET_COMP_UNUSED_ARG(addr);
#endif /* FNET_CFG_IP4 */

    return result;
}

/************************************************************************
* DESCRIPTION: Returns FNET_FALSE if the address is not specified.
*************************************************************************/
fnet_bool_t fnet_socket_addr_is_unspecified(const struct fnet_sockaddr *addr)
{
    fnet_bool_t result = FNET_TRUE;

    if(addr)
    {
#if FNET_CFG_IP6
        if((addr->sa_family & AF_INET6) != 0u)
        {
            result = FNET_IP6_ADDR_IS_UNSPECIFIED( &((const struct fnet_sockaddr_in6 *)addr)->sin6_addr.s6_addr);
        }
        else
#endif
#if FNET_CFG_IP4
            if((addr->sa_family & AF_INET) != 0u)
            {
                result = FNET_IP4_ADDR_IS_UNSPECIFIED( ((const struct fnet_sockaddr_in *)addr)->sin_addr.s_addr);
            }
            else
#endif
            {}
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Returns FNET_FALSE if the addresses are not equal.
*************************************************************************/
fnet_bool_t fnet_socket_addr_are_equal(const struct fnet_sockaddr *addr1, const struct fnet_sockaddr *addr2 )
{
    fnet_bool_t result = FNET_FALSE;

    if(addr1 && addr2 && (addr1->sa_family == addr2->sa_family))
    {
#if FNET_CFG_IP6
        if((addr1->sa_family & AF_INET6) != 0u)
        {
            result =  FNET_IP6_ADDR_EQUAL( &((const struct fnet_sockaddr_in6 *)addr1)->sin6_addr.s6_addr, &((const struct fnet_sockaddr_in6 *)addr2)->sin6_addr.s6_addr);
        }
        else
#endif
#if FNET_CFG_IP4
            if((addr1->sa_family & AF_INET) != 0u)
            {
                result = (((const struct fnet_sockaddr_in *)addr1)->sin_addr.s_addr == (((const struct fnet_sockaddr_in *)addr2)->sin_addr.s_addr)) ? FNET_TRUE : FNET_FALSE;
            }
            else
#endif
            {}
    }

    return result;
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
void _fnet_socket_ip_addr_copy(const struct fnet_sockaddr *from_addr, struct fnet_sockaddr *to_addr)
{

    if(from_addr && to_addr && (to_addr->sa_family == from_addr->sa_family))
    {
#if FNET_CFG_IP6
        if((from_addr->sa_family & AF_INET6) != 0u)
        {
            FNET_IP6_ADDR_COPY(&((const struct fnet_sockaddr_in6 *)from_addr)->sin6_addr.s6_addr, &((struct fnet_sockaddr_in6 *)to_addr)->sin6_addr.s6_addr);
        }
        else
#endif
#if FNET_CFG_IP4
            if((from_addr->sa_family & AF_INET) != 0u)
            {
                ((struct fnet_sockaddr_in *)to_addr)->sin_addr.s_addr = ((const struct fnet_sockaddr_in *)from_addr)->sin_addr.s_addr;
            }
            else
#endif
            {}
    }
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
void _fnet_socket_addr_copy(const struct fnet_sockaddr *from_addr, struct fnet_sockaddr *to_addr)
{
    if(from_addr && to_addr)
    {
#if FNET_CFG_IP6
        if((from_addr->sa_family & AF_INET6) != 0u)
        {
            fnet_memcpy(to_addr, from_addr, sizeof(struct fnet_sockaddr_in6));
        }
        else
#endif
#if FNET_CFG_IP4
            if((from_addr->sa_family & AF_INET) != 0u)
            {
                fnet_memcpy(to_addr, from_addr, sizeof(struct fnet_sockaddr_in));
            }
            else
#endif
            {}
    }
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
fnet_netif_t *_fnet_socket_addr_route(const struct fnet_sockaddr *dest_addr)
{
    fnet_netif_t *result = FNET_NULL;

    if(dest_addr)
    {
        switch(dest_addr->sa_family)
        {
#if FNET_CFG_IP4
            case AF_INET:
                /* Check Scope ID.*/
                if((result = _fnet_netif_get_by_scope_id(dest_addr->sa_scope_id)) == FNET_NULL)
                {
                    result = _fnet_ip4_route(((const struct fnet_sockaddr_in *)dest_addr)->sin_addr.s_addr);
                }
                break;
#endif
#if FNET_CFG_IP6
            case AF_INET6:

                /* Check Scope ID.*/
                if((result = _fnet_netif_get_by_scope_id(dest_addr->sa_scope_id)) == FNET_NULL)
                {
                    const fnet_ip6_addr_t   *src_ip;

                    src_ip = _fnet_ip6_select_src_addr(FNET_NULL, &((const struct fnet_sockaddr_in6 *)dest_addr)->sin6_addr.s6_addr);

                    if(src_ip)
                    {
                        result = (fnet_netif_t *)_fnet_netif_get_by_ip6_addr(src_ip);
                    }
                }
                break;
#endif
            default:
                break;
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Polls current socket event state.
*************************************************************************/
fnet_size_t fnet_socket_poll(fnet_socket_poll_t *socket_poll, fnet_size_t socket_poll_size)
{
    fnet_size_t     result = 0;

    if(socket_poll)
    {
        while(socket_poll_size)
        {
            if(socket_poll->s && socket_poll->events) /* If socket valid and any requested event */
            {
                fnet_socket_if_t   *sock;

                _fnet_stack_mutex_lock();

                sock = _fnet_socket_desc_find(socket_poll->s);

                if(sock)
                {
                    socket_poll->events_occurred = FNET_SOCKET_EVENT_NONE;

                    fnet_isr_lock();

                    if(socket_poll->events &  FNET_SOCKET_EVENT_IN) /* There is data for reading or a new pending connection for accepting. */
                    {
                        if(sock->state == SS_LISTENING) /*If socket is listening */
                        {
                            if(sock->incoming_con_len)
                            {
                                socket_poll->events_occurred |= FNET_SOCKET_EVENT_IN; /* IN event */
                            }
                        }
                        else
                        {
                            if(sock->receive_buffer.count) /* There is data for reading */
                            {
                                socket_poll->events_occurred |= FNET_SOCKET_EVENT_IN; /* IN event */
                            }
                            else /* No data */
                            {
                                /* The protocol is connection oriented (TCP).*/
                                if((sock->protocol_interface->socket_api->con_req) &&
                                   ((sock->state == SS_CLOSED) || (sock->state == SS_CLOSING))) /* If the socket is disconnected*/
                                {
                                    _fnet_socket_set_error(sock, FNET_ERR_NOTCONN); /* Will be catched later if FNET_SOCKET_EVENT_ERR is set. */
                                }
                            }
                        }
                    }

                    if(socket_poll->events &  FNET_SOCKET_EVENT_OUT) /* There is a free buffer space and the socket may accept data for writing. */
                    {
                        if((sock->send_buffer.count_max - sock->send_buffer.count) > 0 ) /* If there is a free buffer space */
                        {
                            /* The protocol is connection oriented (TCP).*/
                            if(sock->protocol_interface->socket_api->con_req)
                            {
                                if(sock->state == SS_CONNECTED) /* Socket is connected.*/
                                {
                                    socket_poll->events_occurred |= FNET_SOCKET_EVENT_OUT;
                                }
                                else if((sock->state == SS_CLOSED) || (sock->state == SS_CLOSING)) /* If the socket is disconnected*/
                                {
                                    _fnet_socket_set_error(sock, FNET_ERR_NOTCONN); /* Will be catched later if FNET_SOCKET_EVENT_ERR is set. */
                                }
                            }
                            else /* Non-connection socket is always ready to send */
                            {
                                socket_poll->events_occurred |= FNET_SOCKET_EVENT_OUT;
                            }
                        }
                    }

                    if(socket_poll->events &  FNET_SOCKET_EVENT_ERR) /* There is a socket error. */
                    {
                        if(sock->options.error != FNET_ERR_OK) /* Thre is socket error */
                        {
                            socket_poll->events_occurred |= FNET_SOCKET_EVENT_ERR;
                        }
                    }

                    fnet_isr_unlock();

                    if(socket_poll->events_occurred)
                    {
                        result++;
                    }
                }

                _fnet_stack_mutex_unlock();
            }
            /* Next socket event */
            socket_poll++;
            socket_poll_size--;
        }
    }

    return result;
}
