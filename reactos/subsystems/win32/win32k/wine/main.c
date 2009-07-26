/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/init.c
 * PURPOSE:         Driver Initialization
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#undef LIST_FOR_EACH
#undef LIST_FOR_EACH_SAFE
#include "object.h"

#define WANT_REQUEST_HANDLERS
#include "request.h"

#define NDEBUG
#include <debug.h>

ERESOURCE UserLock;

PVOID RequestData;
ULONG ReplySize;
PVOID ReplyData;

/* PRIVATE FUNCTIONS *********************************************************/

VOID UserEnterExclusive(VOID)
{
    /* Acquire user resource exclusively */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&UserLock, TRUE);
}

VOID UserLeave(VOID)
{
    /* Release user resource */
    ExReleaseResourceLite(&UserLock);
    KeLeaveCriticalRegion();
}

#if 0
VOID UserCleanup(VOID)
{
   ExDeleteResourceLite(&UserLock);
}
#endif

VOID UserInitialize(VOID)
{
    NTSTATUS Status;

    /* Initialize user access resource */
    Status = ExInitializeResourceLite(&UserLock);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failure initializing USER resource!\n");
    }
}

UINT
APIENTRY
wine_server_call(void *req_ptr)
{
    struct __server_request_info *reqinfo = req_ptr;
    union generic_reply reply;
    enum request req = reqinfo->u.req.request_header.req;
    UCHAR i;
    ULONG DataWritten=0;
    BOOLEAN FreeReqData = FALSE;

    DPRINT("WineServer call of type 0x%x\n", req);

    /* Zero reply's memory area */
    memset( &reply, 0, sizeof(reply) );

    /* Acquire lock */
    UserEnterExclusive();

    /* Clear error status */
    set_error(0);

    if (reqinfo->data_count > 1)
    {
        /* We need to make a contiguous data block from those */
        DataWritten = 0;
        for (i=0; i<reqinfo->data_count; i++)
        {
            DataWritten += reqinfo->data[i].size;
        }

        /* Allocate memory for it */
        RequestData = ExAllocatePool(PagedPool, DataWritten);

        /* Place them */
        DataWritten = 0;
        for (i=0; i<reqinfo->data_count; i++)
        {
            RtlCopyMemory(((UCHAR *)RequestData + DataWritten),
                          reqinfo->data[i].ptr,
                          reqinfo->data[i].size);

            /* Advance to the next data block */
            DataWritten += reqinfo->data[i].size;
        }

        /* Set the allocation flag so we free this memory later */
        FreeReqData = TRUE;
    }
    else
    {
        RequestData = (PVOID)reqinfo->data[0].ptr;
    }

    ReplySize = 0;
    ReplyData = NULL;

    /* Perform the request */
    if (req < REQ_NB_REQUESTS)
    {
        /* Call request handler */
        req_handlers[req]( req_ptr, &reply );
    }
    else
    {
        DPRINT1("WineServer call of type 0x%x is not implemented!\n", req);
    }

    /* Free the request data area if needed */
    if (FreeReqData) ExFreePool(RequestData);

    /* Copy back the reply data if any */
    if (ReplySize && reqinfo->reply_data)
    {
        /* Copy it */
        RtlCopyMemory(reqinfo->reply_data, ReplyData, ReplySize);

        /* Free temp storage */
        ExFreePool(ReplyData);
    }
    else if (ReplySize)
    {
        DPRINT1("Caller didn't provide any storage for output data, not transferring anything.\n");
        ReplySize = 0;
    }

    /* Set reply's error flag and size */
    reply.reply_header.error = get_error();
    reply.reply_header.reply_size = ReplySize;

    /* Copy reply back */
    memcpy (&reqinfo->u.reply, &reply, sizeof(reply));

    /* Release lock */
    UserLeave();

    /* Perform any pending notifies without holding the lock */
    if (req == REQ_create_desktop)
    {
        if (reply.reply_header.error != STATUS_OBJECT_NAME_EXISTS)
            CsrNotifyCreateDesktop((HDESK)((struct create_desktop_reply *)&reply)->handle);
    }

    return reply.reply_header.error;
}

/* allocate the reply data */
void *set_reply_data_size( void *req, data_size_t size )
{
    ASSERT( size <= get_reply_max_size(req) );

    /* Allocate storage for reply data */
    if (size && !(ReplyData = ExAllocatePool( PagedPool, size ))) size = 0;

    /* Set reply size */
    ReplySize = size;

    /* Return a pointer to allocated storage */
    return ReplyData;
}

/* set the reply data pointer directly (will be freed by request code) */
void set_reply_data_ptr( void *req, void *data, data_size_t size )
{
    ASSERT( size < get_reply_max_size(req));

    /* Just save them */
    ReplySize = size;
    ReplyData = data;
}

