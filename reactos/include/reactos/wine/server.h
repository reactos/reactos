/*
 * Definitions for the client side of the Wine server communication
 *
 * Copyright (C) 1998 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINE_SERVER_H
#define __WINE_WINE_SERVER_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <wine/server_protocol.h>

/* client communication functions */

struct __server_iovec
{
    const void  *ptr;
    data_size_t  size;
};

#define __SERVER_MAX_DATA 5

struct __server_request_info
{
    union
    {
        union generic_request req;    /* request structure */
        union generic_reply   reply;  /* reply structure */
    } u;
    unsigned int          data_count; /* count of request data pointers */
    void                 *reply_data; /* reply data pointer */
    struct __server_iovec data[__SERVER_MAX_DATA];  /* request variable size data */
};

extern unsigned int wine_server_call( void *req_ptr );
extern void CDECL wine_server_send_fd( int fd );
extern int CDECL wine_server_fd_to_handle( int fd, unsigned int access, unsigned int attributes, HANDLE *handle );
extern int CDECL wine_server_handle_to_fd( HANDLE handle, unsigned int access, int *unix_fd, unsigned int *options );
extern void CDECL wine_server_release_fd( HANDLE handle, int unix_fd );

/* do a server call and set the last error code */
static inline unsigned int wine_server_call_err( void *req_ptr )
{
    unsigned int res = wine_server_call( req_ptr );
    if (res) SetLastError( RtlNtStatusToDosError(res) );
    return res;
}

/* get the size of the variable part of the returned reply */
static inline data_size_t wine_server_reply_size( const void *reply )
{
    return ((const struct reply_header *)reply)->reply_size;
}

/* add some data to be sent along with the request */
static inline void wine_server_add_data( void *req_ptr, const void *ptr, data_size_t size )
{
    struct __server_request_info * const req = req_ptr;
    if (size)
    {
        req->data[req->data_count].ptr = ptr;
        req->data[req->data_count++].size = size;
        req->u.req.request_header.request_size += size;
    }
}

/* set the pointer and max size for the reply var data */
static inline void wine_server_set_reply( void *req_ptr, void *ptr, data_size_t max_size )
{
    struct __server_request_info * const req = req_ptr;
    req->reply_data = ptr;
    req->u.req.request_header.reply_size = max_size;
}

/* convert an object handle to a server handle */
static inline obj_handle_t wine_server_obj_handle( HANDLE handle )
{
    if ((int)(INT_PTR)handle != (INT_PTR)handle) return 0xfffffff0;  /* some invalid handle */
    return (INT_PTR)handle;
}

/* convert a user handle to a server handle */
static inline user_handle_t wine_server_user_handle( HANDLE handle )
{
    return (UINT_PTR)handle;
}

/* convert a server handle to a generic handle */
static inline HANDLE wine_server_ptr_handle( obj_handle_t handle )
{
    return (HANDLE)(INT_PTR)(int)handle;
}

/* convert a client pointer to a server client_ptr_t */
static inline client_ptr_t wine_server_client_ptr( const void *ptr )
{
    return (client_ptr_t)(ULONG_PTR)ptr;
}

/* convert a server client_ptr_t to a real pointer */
static inline void *wine_server_get_ptr( client_ptr_t ptr )
{
    return (void *)(ULONG_PTR)ptr;
}


/* macros for server requests */

#define SERVER_START_REQ(type) \
    do { \
        struct __server_request_info __req; \
        struct type##_request * const req = &__req.u.req.type##_request; \
        const struct type##_reply * const reply = &__req.u.reply.type##_reply; \
        memset( &__req.u.req, 0, sizeof(__req.u.req) ); \
        __req.u.req.request_header.req = REQ_##type; \
        __req.data_count = 0; \
        (void)reply; \
        do

#define SERVER_END_REQ \
        while(0); \
    } while(0)


#endif  /* __WINE_WINE_SERVER_H */
