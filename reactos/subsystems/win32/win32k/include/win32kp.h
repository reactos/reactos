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

/* Internal Win32K Headers */
#include <error.h>

/* Wine protocol */
#include <wine/server_protocol.h>

/* RosGdi syscalls */
#include <ntrosgdi.h>
#include <win32k/rosuser.h>

/* CSR interaction */
#include <csr.h>

#include <win32.h>
#include <heap.h>
#include <tags.h>

#include <dib.h>

/* Eng and GRE stuff */
#include <driver.h>
#include <engevent.h>
#include <devobj.h>
#include <gdiobj.h>
#include <surfobj.h>
#include <brushobj.h>
#include <dc.h>
#include <palobj.h>
#include <clipobj.h>
#include <floatobj.h>
#include <xformobj.h>
#include <xlateobj.h>
#include <cursor.h>
#include <gre.h>

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

/* TODO: Move to eng.h */
ULONGLONG
APIENTRY
EngGetTickCount(VOID);

/* TODO: Move to user.h */
VOID UserEnterExclusive(VOID);
VOID UserLeave(VOID);
VOID UserCleanup(VOID);
VOID UserInitialize(VOID);

#endif
