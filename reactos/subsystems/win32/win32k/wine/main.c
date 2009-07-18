/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/init.c
 * PURPOSE:         Driver Initialization
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define WANT_REQUEST_HANDLERS
#include "request.h"

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

UINT
APIENTRY
wine_server_call(void *req_ptr)
{
    struct __server_request_info *reqinfo = req_ptr;
    //struct type##_request * const req = &__req.u.req.type##_request;
    //const struct type##_reply * const reply = &__req.u.reply.type##_reply;

    union generic_reply reply;
    enum request req = reqinfo->u.req.request_header.req;

    DPRINT("WineServer call of type 0x%x\n", req);

    memset( &reply, 0, sizeof(reply) );

    //if (debug_level) trace_request();

    if (req < REQ_NB_REQUESTS)
        req_handlers[req]( req_ptr, &reply );
    else
        DPRINT1("WineServer call of type 0x%x is not implemented!\n", req);

    //reply.reply_header.error = current->error;
    //reply.reply_header.reply_size = current->reply_size;
    //if (debug_level) trace_reply( req, &reply );

    return 0;
}
