/* Copyright (c) 2003 Juan Lang
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
#include "config.h"
#include "wine/debug.h"
#include "nbcmdqueue.h"

WINE_DEFAULT_DEBUG_CHANNEL(netbios);

struct NBCmdQueue
{
    HANDLE           heap;
    CRITICAL_SECTION cs;
    PNCB             head;
};

#define CANCEL_EVENT_PTR(ncb) (PHANDLE)((ncb)->ncb_reserve)
#define NEXT_PTR(ncb) (PNCB *)((ncb)->ncb_reserve + sizeof(HANDLE))

/* The reserved area of an ncb will be used for the following data:
 * - a cancelled flag (BOOL, 4 bytes??)
 * - a handle to an event that's set by a cancelled command on completion
 *   (HANDLE, 4 bytes)
 * These members are used in the following way
 * - on cancel, set the event member of the reserved field (with create event)
 * - NBCmdComplete will delete the ncb from the queue of there's no event;
 *   otherwise it will set the event and not delete the ncb
 * - cancel must lock the queue before finding the ncb in it, and can unlock it
 *   once it's set the event (and the cancelled flag)
 * - NBCmdComplete must lock the queue before attempting to remove the ncb or
 *   check the event
 * - NBCmdQueueCancelAll will lock the queue, and cancel all ncb's in the queue.
 *   It'll then unlock the queue, and wait on the event in the head of the queue
 *   until there's no more ncb's in the queue.
 * Space optimization: use the handle as a boolean.  NULL == 0 => not cancelled.
 * Non-NULL == valid handle => cancelled.  This allows storing a next pointer
 * in the ncb's reserved field as well, avoiding a memory alloc for a new
 * command (cool).
 */

struct NBCmdQueue *NBCmdQueueCreate(HANDLE heap)
{
    struct NBCmdQueue *queue;

    if (heap == NULL)
        heap = GetProcessHeap();
    queue = HeapAlloc(heap, 0, sizeof(struct NBCmdQueue));
    if (queue)
    {
        queue->heap = heap;
        InitializeCriticalSection(&queue->cs);
        queue->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": NBCmdQueue.cs");
        queue->head = NULL;
    }
    return queue;
}

UCHAR NBCmdQueueAdd(struct NBCmdQueue *queue, PNCB ncb)
{
    UCHAR ret;

    TRACE(": queue %p, ncb %p\n", queue, ncb);

    if (!queue)
        return NRC_BADDR;
    if (!ncb)
        return NRC_INVADDRESS;

    *CANCEL_EVENT_PTR(ncb) = NULL;
    EnterCriticalSection(&queue->cs);
    *NEXT_PTR(ncb) = queue->head;
    queue->head = ncb;
    ret = NRC_GOODRET;
    LeaveCriticalSection(&queue->cs);
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static PNCB *NBCmdQueueFindNBC(struct NBCmdQueue *queue, PNCB ncb)
{
    PNCB *ret;

    if (!queue || !ncb)
        ret = NULL;
    else
    {
        ret = &queue->head;
        while (ret && *ret != ncb)
            ret = NEXT_PTR(*ret);
    }
    return ret;
}

UCHAR NBCmdQueueCancel(struct NBCmdQueue *queue, PNCB ncb)
{
    UCHAR ret;
    PNCB *spot;

    TRACE(": queue %p, ncb %p\n", queue, ncb);

    if (!queue)
        return NRC_BADDR;
    if (!ncb)
        return NRC_INVADDRESS;

    EnterCriticalSection(&queue->cs);
    spot = NBCmdQueueFindNBC(queue, ncb);
    if (spot)
    {
        *CANCEL_EVENT_PTR(*spot) = CreateEventW(NULL, FALSE, FALSE, NULL);
        WaitForSingleObject(*CANCEL_EVENT_PTR(*spot), INFINITE);
        CloseHandle(*CANCEL_EVENT_PTR(*spot));
        *spot = *NEXT_PTR(*spot);
        if (ncb->ncb_retcode == NRC_CMDCAN)
            ret = NRC_CMDCAN;
        else
            ret = NRC_CANOCCR;
    }
    else
        ret = NRC_INVADDRESS;
    LeaveCriticalSection(&queue->cs);
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

UCHAR NBCmdQueueComplete(struct NBCmdQueue *queue, PNCB ncb, UCHAR retcode)
{
    UCHAR ret;
    PNCB *spot;

    TRACE(": queue %p, ncb %p\n", queue, ncb);

    if (!queue)
        return NRC_BADDR;
    if (!ncb)
        return NRC_INVADDRESS;

    EnterCriticalSection(&queue->cs);
    spot = NBCmdQueueFindNBC(queue, ncb);
    if (spot)
    {
        if (*CANCEL_EVENT_PTR(*spot))
            SetEvent(*CANCEL_EVENT_PTR(*spot));
        else
            *spot = *NEXT_PTR(*spot);
        ret = NRC_GOODRET;
    }
    else
        ret = NRC_INVADDRESS;
    LeaveCriticalSection(&queue->cs);
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

UCHAR NBCmdQueueCancelAll(struct NBCmdQueue *queue)
{
    UCHAR ret;

    TRACE(": queue %p\n", queue);

    if (!queue)
        return NRC_BADDR;

    EnterCriticalSection(&queue->cs);
    while (queue->head)
    {
        TRACE(": waiting for ncb %p (command 0x%02x)\n", queue->head,
         queue->head->ncb_command);
        NBCmdQueueCancel(queue, queue->head);
    }
    LeaveCriticalSection(&queue->cs);
    ret = NRC_GOODRET;
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

void NBCmdQueueDestroy(struct NBCmdQueue *queue)
{
    TRACE(": queue %p\n", queue);

    if (queue)
    {
        NBCmdQueueCancelAll(queue);
        queue->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&queue->cs);
        HeapFree(queue->heap, 0, queue);
    }
}
