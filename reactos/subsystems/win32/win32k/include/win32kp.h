/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32K
 * FILE:            subsystems/win32/win32k/include/win32kp.h
 * PURPOSE:         Internal Win32K Header
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 */

#ifndef _INCLUDE_INTERNAL_WIN32K_H
#define _INCLUDE_INTERNAL_WIN32K_H

/* INCLUDES ******************************************************************/

/* Prototypes */
W32KAPI UINT APIENTRY wine_server_call(void *req_ptr);

/* Internal  Win32K Headers */
#include <error.h>
#include <wine/server_protocol.h>
#include <win32.h>
#include <heap.h>

#include "winesup.h"

/* client communication functions (from server.h) */
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

#endif
