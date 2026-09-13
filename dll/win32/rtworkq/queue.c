/*
 * Copyright 2019-2020 Nikolay Sivov for CodeWeavers
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

#include <assert.h>

#define COBJMACROS
#include "initguid.h"
#include "rtworkq.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

#define FIRST_USER_QUEUE_HANDLE 5
#define MAX_USER_QUEUE_HANDLES 124

#define WAIT_ITEM_KEY_MASK      (0x82000000)
#define SCHEDULED_ITEM_KEY_MASK (0x80000000)

static LONG next_item_key;

static RTWQWORKITEM_KEY get_item_key(DWORD mask, DWORD key)
{
    return ((RTWQWORKITEM_KEY)mask << 32) | key;
}

static RTWQWORKITEM_KEY generate_item_key(DWORD mask)
{
    return get_item_key(mask, InterlockedIncrement(&next_item_key));
}

struct queue_handle
{
    void *obj;
    LONG refcount;
    WORD generation;
};

static struct queue_handle user_queues[MAX_USER_QUEUE_HANDLES];
static struct queue_handle *next_free_user_queue;
static struct queue_handle *next_unused_user_queue = user_queues;
static WORD queue_generation;
static DWORD shared_mt_queue;

static CRITICAL_SECTION queues_section;
static CRITICAL_SECTION_DEBUG queues_critsect_debug =
{
    0, 0, &queues_section,
    { &queues_critsect_debug.ProcessLocksList, &queues_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": queues_section") }
};
static CRITICAL_SECTION queues_section = { &queues_critsect_debug, -1, 0, 0, 0, 0 };

static LONG platform_lock;
static CO_MTA_USAGE_COOKIE mta_cookie;

static struct queue_handle *get_queue_obj(DWORD handle)
{
    unsigned int idx = HIWORD(handle) - FIRST_USER_QUEUE_HANDLE;

    if (idx < MAX_USER_QUEUE_HANDLES && user_queues[idx].refcount)
    {
        if (LOWORD(handle) == user_queues[idx].generation)
            return &user_queues[idx];
    }

    return NULL;
}

/* Should be kept in sync with corresponding MFASYNC_CALLBACK_ constants. */
enum rtwq_callback_queue_id
{
    RTWQ_CALLBACK_QUEUE_UNDEFINED     = 0x00000000,
    RTWQ_CALLBACK_QUEUE_STANDARD      = 0x00000001,
    RTWQ_CALLBACK_QUEUE_RT            = 0x00000002,
    RTWQ_CALLBACK_QUEUE_IO            = 0x00000003,
    RTWQ_CALLBACK_QUEUE_TIMER         = 0x00000004,
    RTWQ_CALLBACK_QUEUE_MULTITHREADED = 0x00000005,
    RTWQ_CALLBACK_QUEUE_LONG_FUNCTION = 0x00000007,
    RTWQ_CALLBACK_QUEUE_PRIVATE_MASK  = 0xffff0000,
    RTWQ_CALLBACK_QUEUE_ALL           = 0xffffffff,
};

/* Should be kept in sync with corresponding MFASYNC_ constants. */
enum rtwq_callback_flags
{
    RTWQ_FAST_IO_PROCESSING_CALLBACK = 0x00000001,
    RTWQ_SIGNAL_CALLBACK             = 0x00000002,
    RTWQ_BLOCKING_CALLBACK           = 0x00000004,
    RTWQ_REPLY_CALLBACK              = 0x00000008,
    RTWQ_LOCALIZE_REMOTE_CALLBACK    = 0x00000010,
};

enum system_queue_index
{
    SYS_QUEUE_STANDARD = 0,
    SYS_QUEUE_RT,
    SYS_QUEUE_IO,
    SYS_QUEUE_TIMER,
    SYS_QUEUE_MULTITHREADED,
    SYS_QUEUE_DO_NOT_USE,
    SYS_QUEUE_LONG_FUNCTION,
    SYS_QUEUE_COUNT,
};

enum work_item_type
{
    WORK_ITEM_WORK,
    WORK_ITEM_TIMER,
    WORK_ITEM_WAIT,
};

struct work_item
{
    IUnknown IUnknown_iface;
    LONG refcount;
    struct list entry;
    IRtwqAsyncResult *result;
    IRtwqAsyncResult *reply_result;
    struct queue *queue;
    RTWQWORKITEM_KEY key;
    LONG priority;
    DWORD flags;
    PTP_SIMPLE_CALLBACK finalization_callback;
    enum work_item_type type;
    union
    {
        TP_WORK *work_object;
        TP_WAIT *wait_object;
        TP_TIMER *timer_object;
    } u;
};

static struct work_item *work_item_impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct work_item, IUnknown_iface);
}

static const TP_CALLBACK_PRIORITY priorities[] =
{
    TP_CALLBACK_PRIORITY_HIGH,
    TP_CALLBACK_PRIORITY_NORMAL,
    TP_CALLBACK_PRIORITY_LOW,
};

struct queue;
struct queue_desc;

struct queue_ops
{
    HRESULT (*init)(const struct queue_desc *desc, struct queue *queue);
    BOOL (*shutdown)(struct queue *queue);
    void (*submit)(struct queue *queue, struct work_item *item);
};

struct queue_desc
{
    RTWQ_WORKQUEUE_TYPE queue_type;
    const struct queue_ops *ops;
    DWORD target_queue;
};

struct queue
{
    IRtwqAsyncCallback IRtwqAsyncCallback_iface;
    const struct queue_ops *ops;
    TP_POOL *pool;
    TP_CALLBACK_ENVIRON_V3 envs[ARRAY_SIZE(priorities)];
    CRITICAL_SECTION cs;
    struct list pending_items;
    DWORD id;
    /* Data used for serial queues only. */
    PTP_SIMPLE_CALLBACK finalization_callback;
    DWORD target_queue;
};

static void shutdown_queue(struct queue *queue);

static HRESULT lock_user_queue(DWORD queue)
{
    HRESULT hr = RTWQ_E_INVALID_WORKQUEUE;
    struct queue_handle *entry;

    if (!(queue & RTWQ_CALLBACK_QUEUE_PRIVATE_MASK))
        return S_OK;

    EnterCriticalSection(&queues_section);
    entry = get_queue_obj(queue);
    if (entry && entry->refcount)
    {
        entry->refcount++;
        hr = S_OK;
    }
    LeaveCriticalSection(&queues_section);
    return hr;
}

static HRESULT unlock_user_queue(DWORD queue)
{
    HRESULT hr = RTWQ_E_INVALID_WORKQUEUE;
    struct queue_handle *entry;

    if (!(queue & RTWQ_CALLBACK_QUEUE_PRIVATE_MASK))
        return S_OK;

    EnterCriticalSection(&queues_section);
    entry = get_queue_obj(queue);
    if (entry && entry->refcount)
    {
        if (--entry->refcount == 0)
        {
            if (shared_mt_queue == queue) shared_mt_queue = 0;
            shutdown_queue((struct queue *)entry->obj);
            free(entry->obj);
            entry->obj = next_free_user_queue;
            next_free_user_queue = entry;
        }
        hr = S_OK;
    }
    LeaveCriticalSection(&queues_section);
    return hr;
}

static struct queue *queue_impl_from_IRtwqAsyncCallback(IRtwqAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct queue, IRtwqAsyncCallback_iface);
}

static HRESULT WINAPI queue_serial_callback_QueryInterface(IRtwqAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IRtwqAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IRtwqAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI queue_serial_callback_AddRef(IRtwqAsyncCallback *iface)
{
    return 2;
}

static ULONG WINAPI queue_serial_callback_Release(IRtwqAsyncCallback *iface)
{
    return 1;
}

static HRESULT WINAPI queue_serial_callback_GetParameters(IRtwqAsyncCallback *iface, DWORD *flags, DWORD *queue_id)
{
    struct queue *queue = queue_impl_from_IRtwqAsyncCallback(iface);

    *flags = 0;
    *queue_id = queue->id;

    return S_OK;
}

static HRESULT WINAPI queue_serial_callback_Invoke(IRtwqAsyncCallback *iface, IRtwqAsyncResult *result)
{
    /* Reply callback won't be called in a regular way, pending items and chained queues will make it
       unnecessary complicated to reach actual work queue that's able to execute this item. Instead
       serial queues are cleaned up right away on submit(). */
    return S_OK;
}

static const IRtwqAsyncCallbackVtbl queue_serial_callback_vtbl =
{
    queue_serial_callback_QueryInterface,
    queue_serial_callback_AddRef,
    queue_serial_callback_Release,
    queue_serial_callback_GetParameters,
    queue_serial_callback_Invoke,
};

static struct queue system_queues[SYS_QUEUE_COUNT];

static struct queue *get_system_queue(DWORD queue_id)
{
    switch (queue_id)
    {
        case RTWQ_CALLBACK_QUEUE_STANDARD:
        case RTWQ_CALLBACK_QUEUE_RT:
        case RTWQ_CALLBACK_QUEUE_IO:
        case RTWQ_CALLBACK_QUEUE_TIMER:
        case RTWQ_CALLBACK_QUEUE_MULTITHREADED:
        case RTWQ_CALLBACK_QUEUE_LONG_FUNCTION:
            return &system_queues[queue_id - 1];
        default:
            return NULL;
    }
}

static HRESULT grab_queue(DWORD queue_id, struct queue **ret);

static void CALLBACK standard_queue_cleanup_callback(void *object_data, void *group_data)
{
}

static HRESULT pool_queue_init(const struct queue_desc *desc, struct queue *queue)
{
    TP_CALLBACK_ENVIRON_V3 env;
    unsigned int max_thread, i;

    queue->pool = CreateThreadpool(NULL);

    memset(&env, 0, sizeof(env));
    env.Version = 3;
    env.Size = sizeof(env);
    env.Pool = queue->pool;
    env.CleanupGroup = CreateThreadpoolCleanupGroup();
    env.CleanupGroupCancelCallback = standard_queue_cleanup_callback;
    env.CallbackPriority = TP_CALLBACK_PRIORITY_NORMAL;
    for (i = 0; i < ARRAY_SIZE(queue->envs); ++i)
    {
        queue->envs[i] = env;
        queue->envs[i].CallbackPriority = priorities[i];
    }
    list_init(&queue->pending_items);
    InitializeCriticalSection(&queue->cs);

    max_thread = (desc->queue_type == RTWQ_STANDARD_WORKQUEUE || desc->queue_type == RTWQ_WINDOW_WORKQUEUE) ? 1 : 4;

    SetThreadpoolThreadMinimum(queue->pool, 1);
    SetThreadpoolThreadMaximum(queue->pool, max_thread);

    if (desc->queue_type == RTWQ_WINDOW_WORKQUEUE)
        FIXME("RTWQ_WINDOW_WORKQUEUE is not supported.\n");

    return S_OK;
}

static BOOL pool_queue_shutdown(struct queue *queue)
{
    if (!queue->pool)
        return FALSE;

    CloseThreadpoolCleanupGroupMembers(queue->envs[0].CleanupGroup, TRUE, NULL);
    CloseThreadpool(queue->pool);
    queue->pool = NULL;

    return TRUE;
}

static void CALLBACK standard_queue_worker(TP_CALLBACK_INSTANCE *instance, void *context, TP_WORK *work)
{
    struct work_item *item = context;
    RTWQASYNCRESULT *result = (RTWQASYNCRESULT *)item->result;

    TRACE("result object %p.\n", result);

    /* Submitting from serial queue in reply mode, use different result object acting as receipt token.
       It's submitted to user callback still, but when invoked, special serial queue callback will be used
       to ensure correct destination queue. */

    IRtwqAsyncCallback_Invoke(result->pCallback, item->reply_result ? item->reply_result : item->result);

    IUnknown_Release(&item->IUnknown_iface);
}

static void pool_queue_submit(struct queue *queue, struct work_item *item)
{
    TP_CALLBACK_PRIORITY callback_priority;
    TP_CALLBACK_ENVIRON_V3 env;

    if (item->priority == 0)
        callback_priority = TP_CALLBACK_PRIORITY_NORMAL;
    else if (item->priority < 0)
        callback_priority = TP_CALLBACK_PRIORITY_LOW;
    else
        callback_priority = TP_CALLBACK_PRIORITY_HIGH;

    env = queue->envs[callback_priority];
    env.FinalizationCallback = item->finalization_callback;
    /* Worker pool callback will release one reference. Grab one more to keep object alive when
       we need finalization callback. */
    if (item->finalization_callback)
        IUnknown_AddRef(&item->IUnknown_iface);
    item->u.work_object = CreateThreadpoolWork(standard_queue_worker, item, (TP_CALLBACK_ENVIRON *)&env);
    item->type = WORK_ITEM_WORK;
    SubmitThreadpoolWork(item->u.work_object);

    TRACE("dispatched %p.\n", item->result);
}

static const struct queue_ops pool_queue_ops =
{
    pool_queue_init,
    pool_queue_shutdown,
    pool_queue_submit,
};

static struct work_item * serial_queue_get_next(struct queue *queue, struct work_item *item)
{
    struct work_item *next_item = NULL;

    list_remove(&item->entry);
    if (!list_empty(&item->queue->pending_items))
        next_item = LIST_ENTRY(list_head(&item->queue->pending_items), struct work_item, entry);

    return next_item;
}

static void CALLBACK serial_queue_finalization_callback(PTP_CALLBACK_INSTANCE instance, void *user_data)
{
    struct work_item *item = (struct work_item *)user_data, *next_item;
    struct queue *target_queue, *queue = item->queue;
    HRESULT hr;

    EnterCriticalSection(&queue->cs);

    if ((next_item = serial_queue_get_next(queue, item)))
    {
        if (SUCCEEDED(hr = grab_queue(queue->target_queue, &target_queue)))
            target_queue->ops->submit(target_queue, next_item);
        else
            WARN("Failed to grab queue for id %#lx, hr %#lx.\n", queue->target_queue, hr);
    }

    LeaveCriticalSection(&queue->cs);

    IUnknown_Release(&item->IUnknown_iface);
}

static HRESULT serial_queue_init(const struct queue_desc *desc, struct queue *queue)
{
    queue->IRtwqAsyncCallback_iface.lpVtbl = &queue_serial_callback_vtbl;
    queue->target_queue = desc->target_queue;
    lock_user_queue(queue->target_queue);
    queue->finalization_callback = serial_queue_finalization_callback;

    return S_OK;
}

static BOOL serial_queue_shutdown(struct queue *queue)
{
    unlock_user_queue(queue->target_queue);

    return TRUE;
}

static struct work_item * serial_queue_is_ack_token(struct queue *queue, struct work_item *item)
{
    RTWQASYNCRESULT *async_result = (RTWQASYNCRESULT *)item->result;
    struct work_item *head;

    if (list_empty(&queue->pending_items))
        return NULL;

    head = LIST_ENTRY(list_head(&queue->pending_items), struct work_item, entry);
    if (head->reply_result == item->result && async_result->pCallback == &queue->IRtwqAsyncCallback_iface)
        return head;

    return NULL;
}

static void serial_queue_submit(struct queue *queue, struct work_item *item)
{
    struct work_item *head, *next_item = NULL;
    struct queue *target_queue;
    HRESULT hr;

    /* In reply mode queue will advance when 'reply_result' is invoked, in regular mode it will advance automatically,
       via finalization callback. */

    if (item->flags & RTWQ_REPLY_CALLBACK)
    {
        if (FAILED(hr = RtwqCreateAsyncResult(NULL, &queue->IRtwqAsyncCallback_iface, NULL, &item->reply_result)))
            WARN("Failed to create reply object, hr %#lx.\n", hr);
    }
    else
        item->finalization_callback = queue->finalization_callback;

    /* Serial queues could be chained together, detach from current queue before transitioning item to this one.
       Items are not detached when submitted to pool queues, because pool queues won't forward them further. */
    EnterCriticalSection(&item->queue->cs);
    list_remove(&item->entry);
    LeaveCriticalSection(&item->queue->cs);

    EnterCriticalSection(&queue->cs);

    item->queue = queue;

    if ((head = serial_queue_is_ack_token(queue, item)))
    {
        /* Ack receipt token - pop waiting item, advance. */
        next_item = serial_queue_get_next(queue, head);
        IUnknown_Release(&head->IUnknown_iface);
    }
    else
    {
        if (list_empty(&queue->pending_items))
            next_item = item;
        list_add_tail(&queue->pending_items, &item->entry);
        IUnknown_AddRef(&item->IUnknown_iface);
    }

    if (next_item)
    {
        if (SUCCEEDED(hr = grab_queue(queue->target_queue, &target_queue)))
            target_queue->ops->submit(target_queue, next_item);
        else
            WARN("Failed to grab queue for id %#lx, hr %#lx.\n", queue->target_queue, hr);
    }

    LeaveCriticalSection(&queue->cs);
}

static const struct queue_ops serial_queue_ops =
{
    serial_queue_init,
    serial_queue_shutdown,
    serial_queue_submit,
};

static HRESULT WINAPI work_item_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI work_item_AddRef(IUnknown *iface)
{
    struct work_item *item = work_item_impl_from_IUnknown(iface);
    return InterlockedIncrement(&item->refcount);
}

static ULONG WINAPI work_item_Release(IUnknown *iface)
{
    struct work_item *item = work_item_impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&item->refcount);

    if (!refcount)
    {
        switch (item->type)
        {
            case WORK_ITEM_WORK:
                if (item->u.work_object) CloseThreadpoolWork(item->u.work_object);
                break;
            case WORK_ITEM_WAIT:
                if (item->u.wait_object) CloseThreadpoolWait(item->u.wait_object);
                break;
            case WORK_ITEM_TIMER:
                if (item->u.timer_object) CloseThreadpoolTimer(item->u.timer_object);
                break;
        }
        if (item->reply_result)
            IRtwqAsyncResult_Release(item->reply_result);
        IRtwqAsyncResult_Release(item->result);
        free(item);
    }

    return refcount;
}

static const IUnknownVtbl work_item_vtbl =
{
    work_item_QueryInterface,
    work_item_AddRef,
    work_item_Release,
};

static struct work_item * alloc_work_item(struct queue *queue, LONG priority, IRtwqAsyncResult *result)
{
    RTWQASYNCRESULT *async_result = (RTWQASYNCRESULT *)result;
    DWORD flags = 0, queue_id = 0;
    struct work_item *item;

    item = calloc(1, sizeof(*item));

    item->IUnknown_iface.lpVtbl = &work_item_vtbl;
    item->result = result;
    IRtwqAsyncResult_AddRef(item->result);
    item->refcount = 1;
    item->queue = queue;
    list_init(&item->entry);
    item->priority = priority;

    if (SUCCEEDED(IRtwqAsyncCallback_GetParameters(async_result->pCallback, &flags, &queue_id)))
        item->flags = flags;

    return item;
}

static void init_work_queue(const struct queue_desc *desc, struct queue *queue)
{
    assert(desc->ops != NULL);

    queue->ops = desc->ops;
    if (SUCCEEDED(queue->ops->init(desc, queue)))
    {
        list_init(&queue->pending_items);
        InitializeCriticalSection(&queue->cs);
    }
}

static HRESULT grab_queue(DWORD queue_id, struct queue **ret)
{
    struct queue *queue = get_system_queue(queue_id);
    RTWQ_WORKQUEUE_TYPE queue_type;
    struct queue_handle *entry;

    *ret = NULL;

    if (!system_queues[SYS_QUEUE_STANDARD].pool)
        return RTWQ_E_SHUTDOWN;

    if (queue && queue->pool)
    {
        *ret = queue;
        return S_OK;
    }
    else if (queue)
    {
        struct queue_desc desc;

        EnterCriticalSection(&queues_section);
        switch (queue_id)
        {
            case RTWQ_CALLBACK_QUEUE_IO:
            case RTWQ_CALLBACK_QUEUE_MULTITHREADED:
            case RTWQ_CALLBACK_QUEUE_LONG_FUNCTION:
                queue_type = RTWQ_MULTITHREADED_WORKQUEUE;
                break;
            default:
                queue_type = RTWQ_STANDARD_WORKQUEUE;
        }

        desc.queue_type = queue_type;
        desc.ops = &pool_queue_ops;
        desc.target_queue = 0;
        init_work_queue(&desc, queue);
        LeaveCriticalSection(&queues_section);
        *ret = queue;
        return S_OK;
    }

    /* Handles user queues. */
    if ((entry = get_queue_obj(queue_id)))
        *ret = entry->obj;

    return *ret ? S_OK : RTWQ_E_INVALID_WORKQUEUE;
}

static void shutdown_queue(struct queue *queue)
{
    struct work_item *item, *item2;

    if (!queue->ops || !queue->ops->shutdown(queue))
        return;

    EnterCriticalSection(&queue->cs);
    LIST_FOR_EACH_ENTRY_SAFE(item, item2, &queue->pending_items, struct work_item, entry)
    {
        list_remove(&item->entry);
        IUnknown_Release(&item->IUnknown_iface);
    }
    LeaveCriticalSection(&queue->cs);

    DeleteCriticalSection(&queue->cs);

    memset(queue, 0, sizeof(*queue));
}

static HRESULT queue_submit_item(struct queue *queue, LONG priority, IRtwqAsyncResult *result)
{
    struct work_item *item;

    if (!(item = alloc_work_item(queue, priority, result)))
        return E_OUTOFMEMORY;

    queue->ops->submit(queue, item);

    return S_OK;
}

static HRESULT queue_put_work_item(DWORD queue_id, LONG priority, IRtwqAsyncResult *result)
{
    struct queue *queue;
    HRESULT hr;

    if (FAILED(hr = grab_queue(queue_id, &queue)))
        return hr;

    return queue_submit_item(queue, priority, result);
}

static HRESULT invoke_async_callback(IRtwqAsyncResult *result)
{
    RTWQASYNCRESULT *result_data = (RTWQASYNCRESULT *)result;
    DWORD queue = RTWQ_CALLBACK_QUEUE_STANDARD, flags;
    HRESULT hr;

    if (FAILED(IRtwqAsyncCallback_GetParameters(result_data->pCallback, &flags, &queue)))
        queue = RTWQ_CALLBACK_QUEUE_STANDARD;

    if (FAILED(lock_user_queue(queue)))
        queue = RTWQ_CALLBACK_QUEUE_STANDARD;

    hr = queue_put_work_item(queue, 0, result);

    unlock_user_queue(queue);

    return hr;
}

/* Return TRUE when the item is actually released by this function. The item could have been already
 * removed from pending items when it got canceled. */
static BOOL queue_release_pending_item(struct work_item *item)
{
    struct queue *queue = item->queue;
    BOOL ret = FALSE;

    EnterCriticalSection(&queue->cs);
    if (item->key)
    {
        list_remove(&item->entry);
        ret = TRUE;
        item->key = 0;
        IUnknown_Release(&item->IUnknown_iface);
    }
    LeaveCriticalSection(&queue->cs);
    return ret;
}

static void CALLBACK waiting_item_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_WAIT *wait,
        TP_WAIT_RESULT wait_result)
{
    struct work_item *item = context;

    TRACE("result object %p.\n", item->result);

    invoke_async_callback(item->result);

    IUnknown_Release(&item->IUnknown_iface);
}

static void CALLBACK waiting_item_cancelable_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_WAIT *wait,
        TP_WAIT_RESULT wait_result)
{
    struct work_item *item = context;

    TRACE("result object %p.\n", item->result);

    if (queue_release_pending_item(item))
        invoke_async_callback(item->result);

    IUnknown_Release(&item->IUnknown_iface);
}

static void CALLBACK scheduled_item_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_TIMER *timer)
{
    struct work_item *item = context;

    TRACE("result object %p.\n", item->result);

    invoke_async_callback(item->result);

    IUnknown_Release(&item->IUnknown_iface);
}

static void CALLBACK scheduled_item_cancelable_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_TIMER *timer)
{
    struct work_item *item = context;

    TRACE("result object %p.\n", item->result);

    if (queue_release_pending_item(item))
        invoke_async_callback(item->result);

    IUnknown_Release(&item->IUnknown_iface);
}

static void CALLBACK periodic_item_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_TIMER *timer)
{
    struct work_item *item = context;

    IUnknown_AddRef(&item->IUnknown_iface);

    invoke_async_callback(item->result);

    IUnknown_Release(&item->IUnknown_iface);
}

static void queue_mark_item_pending(DWORD mask, struct work_item *item, RTWQWORKITEM_KEY *key)
{
    *key = generate_item_key(mask);
    item->key = *key;

    EnterCriticalSection(&item->queue->cs);
    list_add_tail(&item->queue->pending_items, &item->entry);
    IUnknown_AddRef(&item->IUnknown_iface);
    LeaveCriticalSection(&item->queue->cs);
}

static HRESULT queue_submit_wait(struct queue *queue, HANDLE event, LONG priority, IRtwqAsyncResult *result,
        RTWQWORKITEM_KEY *key)
{
    PTP_WAIT_CALLBACK callback;
    struct work_item *item;

    if (!(item = alloc_work_item(queue, priority, result)))
        return E_OUTOFMEMORY;

    if (key)
    {
        queue_mark_item_pending(WAIT_ITEM_KEY_MASK, item, key);
        callback = waiting_item_cancelable_callback;
    }
    else
        callback = waiting_item_callback;

    item->u.wait_object = CreateThreadpoolWait(callback, item,
            (TP_CALLBACK_ENVIRON *)&queue->envs[TP_CALLBACK_PRIORITY_NORMAL]);
    item->type = WORK_ITEM_WAIT;
    SetThreadpoolWait(item->u.wait_object, event, NULL);

    TRACE("dispatched %p.\n", result);

    return S_OK;
}

static HRESULT queue_submit_timer(struct queue *queue, IRtwqAsyncResult *result, INT64 timeout, DWORD period,
        RTWQWORKITEM_KEY *key)
{
    PTP_TIMER_CALLBACK callback;
    struct work_item *item;
    FILETIME filetime;
    LARGE_INTEGER t;

    if (!(item = alloc_work_item(queue, 0, result)))
        return E_OUTOFMEMORY;

    if (key)
    {
        queue_mark_item_pending(SCHEDULED_ITEM_KEY_MASK, item, key);
    }

    if (period)
        callback = periodic_item_callback;
    else
        callback = key ? scheduled_item_cancelable_callback : scheduled_item_callback;

    t.QuadPart = timeout * 1000 * 10;
    filetime.dwLowDateTime = t.u.LowPart;
    filetime.dwHighDateTime = t.u.HighPart;

    item->u.timer_object = CreateThreadpoolTimer(callback, item,
            (TP_CALLBACK_ENVIRON *)&queue->envs[TP_CALLBACK_PRIORITY_NORMAL]);
    item->type = WORK_ITEM_TIMER;
    SetThreadpoolTimer(item->u.timer_object, &filetime, period, 0);

    TRACE("dispatched %p.\n", result);

    return S_OK;
}

static HRESULT queue_cancel_item(struct queue *queue, RTWQWORKITEM_KEY key)
{
    TP_WAIT *wait_object;
    TP_TIMER *timer_object;
    struct work_item *item;

    EnterCriticalSection(&queue->cs);
    LIST_FOR_EACH_ENTRY(item, &queue->pending_items, struct work_item, entry)
    {
        if (item->key == key)
        {
            /* We can't immediately release the item here, because the callback could already be
             * running somewhere else. And if we release it here, the callback will access freed memory.
             * So instead we have to make sure the callback is really stopped, or has really finished
             * running before we do that. And we can't do that in this critical section, which would be a
             * deadlock. So we first keep an extra reference to it, then leave the critical section to
             * wait for the thread-pool objects, finally we re-enter critical section to release it. */
            key >>= 32;
            IUnknown_AddRef(&item->IUnknown_iface);
            if ((key & WAIT_ITEM_KEY_MASK) == WAIT_ITEM_KEY_MASK)
            {
                wait_object = item->u.wait_object;
                item->u.wait_object = NULL;
                LeaveCriticalSection(&queue->cs);

                SetThreadpoolWait(wait_object, NULL, NULL);
                WaitForThreadpoolWaitCallbacks(wait_object, TRUE);
                CloseThreadpoolWait(wait_object);
            }
            else if ((key & SCHEDULED_ITEM_KEY_MASK) == SCHEDULED_ITEM_KEY_MASK)
            {
                timer_object = item->u.timer_object;
                item->u.timer_object = NULL;
                LeaveCriticalSection(&queue->cs);

                SetThreadpoolTimer(timer_object, NULL, 0, 0);
                WaitForThreadpoolTimerCallbacks(timer_object, TRUE);
                CloseThreadpoolTimer(timer_object);
            }
            else
            {
                WARN("Unknown item key mask %#I64x.\n", key);
                LeaveCriticalSection(&queue->cs);
            }

            if (queue_release_pending_item(item))
            {
                /* This means the callback wasn't run during our wait, so we can invoke the
                 * callback with a canceled status, and release the work item. */
                if ((key & WAIT_ITEM_KEY_MASK) == WAIT_ITEM_KEY_MASK)
                {
                    IRtwqAsyncResult_SetStatus(item->result, RTWQ_E_OPERATION_CANCELLED);
                    invoke_async_callback(item->result);
                }
                IUnknown_Release(&item->IUnknown_iface);
            }
            IUnknown_Release(&item->IUnknown_iface);
            return S_OK;
        }
    }
    LeaveCriticalSection(&queue->cs);

    return RTWQ_E_NOT_FOUND;
}

static HRESULT alloc_user_queue(const struct queue_desc *desc, DWORD *queue_id)
{
    struct queue_handle *entry;
    struct queue *queue;
    unsigned int idx;

    *queue_id = RTWQ_CALLBACK_QUEUE_UNDEFINED;

    if (platform_lock <= 0)
        return RTWQ_E_SHUTDOWN;

    if (!(queue = calloc(1, sizeof(*queue))))
        return E_OUTOFMEMORY;

    init_work_queue(desc, queue);

    EnterCriticalSection(&queues_section);

    entry = next_free_user_queue;
    if (entry)
        next_free_user_queue = entry->obj;
    else if (next_unused_user_queue < user_queues + MAX_USER_QUEUE_HANDLES)
        entry = next_unused_user_queue++;
    else
    {
        LeaveCriticalSection(&queues_section);
        free(queue);
        WARN("Out of user queue handles.\n");
        return E_OUTOFMEMORY;
    }

    entry->refcount = 1;
    entry->obj = queue;
    if (++queue_generation == 0xffff) queue_generation = 1;
    entry->generation = queue_generation;
    idx = entry - user_queues + FIRST_USER_QUEUE_HANDLE;
    *queue_id = (idx << 16) | entry->generation;

    LeaveCriticalSection(&queues_section);

    return S_OK;
}

struct async_result
{
    RTWQASYNCRESULT result;
    LONG refcount;
    IUnknown *object;
    IUnknown *state;
};

static struct async_result *impl_from_IRtwqAsyncResult(IRtwqAsyncResult *iface)
{
    return CONTAINING_RECORD(iface, struct async_result, result.AsyncResult);
}

static HRESULT WINAPI async_result_QueryInterface(IRtwqAsyncResult *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IRtwqAsyncResult) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IRtwqAsyncResult_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI async_result_AddRef(IRtwqAsyncResult *iface)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);
    ULONG refcount = InterlockedIncrement(&result->refcount);

    TRACE("%p, %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI async_result_Release(IRtwqAsyncResult *iface)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);
    ULONG refcount = InterlockedDecrement(&result->refcount);

    TRACE("%p, %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (result->result.pCallback)
            IRtwqAsyncCallback_Release(result->result.pCallback);
        if (result->object)
            IUnknown_Release(result->object);
        if (result->state)
            IUnknown_Release(result->state);
        if (result->result.hEvent)
            CloseHandle(result->result.hEvent);
        free(result);

        RtwqUnlockPlatform();
    }

    return refcount;
}

static HRESULT WINAPI async_result_GetState(IRtwqAsyncResult *iface, IUnknown **state)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);

    TRACE("%p, %p.\n", iface, state);

    if (!result->state)
        return E_POINTER;

    *state = result->state;
    IUnknown_AddRef(*state);

    return S_OK;
}

static HRESULT WINAPI async_result_GetStatus(IRtwqAsyncResult *iface)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);

    TRACE("%p.\n", iface);

    return result->result.hrStatusResult;
}

static HRESULT WINAPI async_result_SetStatus(IRtwqAsyncResult *iface, HRESULT status)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);

    TRACE("%p, %#lx.\n", iface, status);

    result->result.hrStatusResult = status;

    return S_OK;
}

static HRESULT WINAPI async_result_GetObject(IRtwqAsyncResult *iface, IUnknown **object)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);

    TRACE("%p, %p.\n", iface, object);

    if (!result->object)
        return E_POINTER;

    *object = result->object;
    IUnknown_AddRef(*object);

    return S_OK;
}

static IUnknown * WINAPI async_result_GetStateNoAddRef(IRtwqAsyncResult *iface)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);

    TRACE("%p.\n", iface);

    return result->state;
}

static const IRtwqAsyncResultVtbl async_result_vtbl =
{
    async_result_QueryInterface,
    async_result_AddRef,
    async_result_Release,
    async_result_GetState,
    async_result_GetStatus,
    async_result_SetStatus,
    async_result_GetObject,
    async_result_GetStateNoAddRef,
};

static HRESULT create_async_result(IUnknown *object, IRtwqAsyncCallback *callback, IUnknown *state, IRtwqAsyncResult **out)
{
    struct async_result *result;

    if (!out)
        return E_INVALIDARG;

    if (!(result = calloc(1, sizeof(*result))))
        return E_OUTOFMEMORY;

    RtwqLockPlatform();

    result->result.AsyncResult.lpVtbl = &async_result_vtbl;
    result->refcount = 1;
    result->object = object;
    if (result->object)
        IUnknown_AddRef(result->object);
    result->result.pCallback = callback;
    if (result->result.pCallback)
        IRtwqAsyncCallback_AddRef(result->result.pCallback);
    result->state = state;
    if (result->state)
        IUnknown_AddRef(result->state);

    *out = &result->result.AsyncResult;

    TRACE("Created async result object %p.\n", *out);

    return S_OK;
}

HRESULT WINAPI RtwqCreateAsyncResult(IUnknown *object, IRtwqAsyncCallback *callback, IUnknown *state,
        IRtwqAsyncResult **out)
{
    TRACE("%p, %p, %p, %p.\n", object, callback, state, out);

    return create_async_result(object, callback, state, out);
}

HRESULT WINAPI RtwqLockPlatform(void)
{
    InterlockedIncrement(&platform_lock);

    return S_OK;
}

HRESULT WINAPI RtwqUnlockPlatform(void)
{
    InterlockedDecrement(&platform_lock);

    return S_OK;
}

static void init_system_queues(void)
{
    struct queue_desc desc;
    HRESULT hr;

    /* Always initialize standard queue, keep the rest lazy. */

    EnterCriticalSection(&queues_section);

    if (system_queues[SYS_QUEUE_STANDARD].pool)
    {
        LeaveCriticalSection(&queues_section);
        return;
    }

    if (FAILED(hr = CoIncrementMTAUsage(&mta_cookie)))
        WARN("Failed to initialize MTA, hr %#lx.\n", hr);

    desc.queue_type = RTWQ_STANDARD_WORKQUEUE;
    desc.ops = &pool_queue_ops;
    desc.target_queue = 0;
    init_work_queue(&desc, &system_queues[SYS_QUEUE_STANDARD]);

    LeaveCriticalSection(&queues_section);
}

HRESULT WINAPI RtwqStartup(void)
{
    if (InterlockedIncrement(&platform_lock) == 1)
    {
        init_system_queues();
    }

    return S_OK;
}

static void shutdown_system_queues(void)
{
    unsigned int i;
    HRESULT hr;

    EnterCriticalSection(&queues_section);

    for (i = 0; i < ARRAY_SIZE(system_queues); ++i)
    {
        shutdown_queue(&system_queues[i]);
    }

    if (FAILED(hr = CoDecrementMTAUsage(mta_cookie)))
        WARN("Failed to uninitialize MTA, hr %#lx.\n", hr);

    LeaveCriticalSection(&queues_section);
}

HRESULT WINAPI RtwqShutdown(void)
{
    if (platform_lock <= 0)
        return S_OK;

    if (InterlockedExchangeAdd(&platform_lock, -1) == 1)
    {
        shutdown_system_queues();
    }

    return S_OK;
}

HRESULT WINAPI RtwqPutWaitingWorkItem(HANDLE event, LONG priority, IRtwqAsyncResult *result, RTWQWORKITEM_KEY *key)
{
    struct queue *queue;
    HRESULT hr;

    TRACE("%p, %ld, %p, %p.\n", event, priority, result, key);

    if (FAILED(hr = grab_queue(RTWQ_CALLBACK_QUEUE_TIMER, &queue)))
        return hr;

    hr = queue_submit_wait(queue, event, priority, result, key);

    return hr;
}

static HRESULT schedule_work_item(IRtwqAsyncResult *result, INT64 timeout, RTWQWORKITEM_KEY *key)
{
    struct queue *queue;
    HRESULT hr;

    if (FAILED(hr = grab_queue(RTWQ_CALLBACK_QUEUE_TIMER, &queue)))
        return hr;

    TRACE("%p, %s, %p.\n", result, wine_dbgstr_longlong(timeout), key);

    return queue_submit_timer(queue, result, timeout, 0, key);
}

HRESULT WINAPI RtwqScheduleWorkItem(IRtwqAsyncResult *result, INT64 timeout, RTWQWORKITEM_KEY *key)
{
    TRACE("%p, %s, %p.\n", result, wine_dbgstr_longlong(timeout), key);

    return schedule_work_item(result, timeout, key);
}

struct periodic_callback
{
    IRtwqAsyncCallback IRtwqAsyncCallback_iface;
    LONG refcount;
    RTWQPERIODICCALLBACK callback;
};

static struct periodic_callback *impl_from_IRtwqAsyncCallback(IRtwqAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct periodic_callback, IRtwqAsyncCallback_iface);
}

static HRESULT WINAPI periodic_callback_QueryInterface(IRtwqAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IRtwqAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IRtwqAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI periodic_callback_AddRef(IRtwqAsyncCallback *iface)
{
    struct periodic_callback *callback = impl_from_IRtwqAsyncCallback(iface);
    ULONG refcount = InterlockedIncrement(&callback->refcount);

    TRACE("%p, %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI periodic_callback_Release(IRtwqAsyncCallback *iface)
{
    struct periodic_callback *callback = impl_from_IRtwqAsyncCallback(iface);
    ULONG refcount = InterlockedDecrement(&callback->refcount);

    TRACE("%p, %lu.\n", iface, refcount);

    if (!refcount)
        free(callback);

    return refcount;
}

static HRESULT WINAPI periodic_callback_GetParameters(IRtwqAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI periodic_callback_Invoke(IRtwqAsyncCallback *iface, IRtwqAsyncResult *result)
{
    struct periodic_callback *callback = impl_from_IRtwqAsyncCallback(iface);
    IUnknown *context = NULL;

    if (FAILED(IRtwqAsyncResult_GetObject(result, &context)))
        WARN("Expected object to be set for result object.\n");

    callback->callback(context);

    if (context)
        IUnknown_Release(context);

    return S_OK;
}

static const IRtwqAsyncCallbackVtbl periodic_callback_vtbl =
{
    periodic_callback_QueryInterface,
    periodic_callback_AddRef,
    periodic_callback_Release,
    periodic_callback_GetParameters,
    periodic_callback_Invoke,
};

static HRESULT create_periodic_callback_obj(RTWQPERIODICCALLBACK callback, IRtwqAsyncCallback **out)
{
    struct periodic_callback *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IRtwqAsyncCallback_iface.lpVtbl = &periodic_callback_vtbl;
    object->refcount = 1;
    object->callback = callback;

    *out = &object->IRtwqAsyncCallback_iface;

    return S_OK;
}

HRESULT WINAPI RtwqAddPeriodicCallback(RTWQPERIODICCALLBACK callback, IUnknown *context, DWORD *key)
{
    IRtwqAsyncCallback *periodic_callback;
    RTWQWORKITEM_KEY workitem_key;
    IRtwqAsyncResult *result;
    struct queue *queue;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", callback, context, key);

    if (FAILED(hr = grab_queue(RTWQ_CALLBACK_QUEUE_TIMER, &queue)))
        return hr;

    if (FAILED(hr = create_periodic_callback_obj(callback, &periodic_callback)))
        return hr;

    hr = create_async_result(context, periodic_callback, NULL, &result);
    IRtwqAsyncCallback_Release(periodic_callback);
    if (FAILED(hr))
        return hr;

    /* Same period MFGetTimerPeriodicity() returns. */
    hr = queue_submit_timer(queue, result, 0, 10, key ? &workitem_key : NULL);

    IRtwqAsyncResult_Release(result);

    if (key)
        *key = workitem_key;

    return S_OK;
}

HRESULT WINAPI RtwqRemovePeriodicCallback(DWORD key)
{
    struct queue *queue;
    HRESULT hr;

    TRACE("%#lx.\n", key);

    if (FAILED(hr = grab_queue(RTWQ_CALLBACK_QUEUE_TIMER, &queue)))
        return hr;

    return queue_cancel_item(queue, get_item_key(SCHEDULED_ITEM_KEY_MASK, key));
}

HRESULT WINAPI RtwqCancelWorkItem(RTWQWORKITEM_KEY key)
{
    struct queue *queue;
    HRESULT hr;

    TRACE("%s.\n", wine_dbgstr_longlong(key));

    if (FAILED(hr = grab_queue(RTWQ_CALLBACK_QUEUE_TIMER, &queue)))
        return hr;

    return queue_cancel_item(queue, key);
}

HRESULT WINAPI RtwqInvokeCallback(IRtwqAsyncResult *result)
{
    TRACE("%p.\n", result);

    return invoke_async_callback(result);
}

HRESULT WINAPI RtwqPutWorkItem(DWORD queue, LONG priority, IRtwqAsyncResult *result)
{
    TRACE("%#lx, %ld, %p.\n", queue, priority, result);

    return queue_put_work_item(queue, priority, result);
}

HRESULT WINAPI RtwqAllocateWorkQueue(RTWQ_WORKQUEUE_TYPE queue_type, DWORD *queue)
{
    struct queue_desc desc;

    TRACE("%d, %p.\n", queue_type, queue);

    desc.queue_type = queue_type;
    desc.ops = &pool_queue_ops;
    desc.target_queue = 0;
    return alloc_user_queue(&desc, queue);
}

HRESULT WINAPI RtwqLockWorkQueue(DWORD queue)
{
    TRACE("%#lx.\n", queue);

    return lock_user_queue(queue);
}

HRESULT WINAPI RtwqUnlockWorkQueue(DWORD queue)
{
    TRACE("%#lx.\n", queue);

    return unlock_user_queue(queue);
}

HRESULT WINAPI RtwqSetLongRunning(DWORD queue_id, BOOL enable)
{
    struct queue *queue;
    HRESULT hr;
    int i;

    TRACE("%#lx, %d.\n", queue_id, enable);

    lock_user_queue(queue_id);

    if (SUCCEEDED(hr = grab_queue(queue_id, &queue)))
    {
        for (i = 0; i < ARRAY_SIZE(queue->envs); ++i)
            queue->envs[i].u.s.LongFunction = !!enable;
    }

    unlock_user_queue(queue_id);

    return hr;
}

HRESULT WINAPI RtwqLockSharedWorkQueue(const WCHAR *usageclass, LONG priority, DWORD *taskid, DWORD *queue)
{
    struct queue_desc desc;
    HRESULT hr;

    TRACE("%s, %ld, %p, %p.\n", debugstr_w(usageclass), priority, taskid, queue);

    if (!usageclass)
        return E_POINTER;

    if (!*usageclass && taskid)
        return E_INVALIDARG;

    if (*usageclass)
        FIXME("Class name is ignored.\n");

    EnterCriticalSection(&queues_section);

    if (shared_mt_queue)
        hr = lock_user_queue(shared_mt_queue);
    else
    {
        desc.queue_type = RTWQ_MULTITHREADED_WORKQUEUE;
        desc.ops = &pool_queue_ops;
        desc.target_queue = 0;
        hr = alloc_user_queue(&desc, &shared_mt_queue);
    }

    *queue = shared_mt_queue;

    LeaveCriticalSection(&queues_section);

    return hr;
}

HRESULT WINAPI RtwqSetDeadline(DWORD queue_id, LONGLONG deadline, HANDLE *request)
{
    FIXME("%#lx, %s, %p.\n", queue_id, wine_dbgstr_longlong(deadline), request);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqSetDeadline2(DWORD queue_id, LONGLONG deadline, LONGLONG predeadline, HANDLE *request)
{
    FIXME("%#lx, %s, %s, %p.\n", queue_id, wine_dbgstr_longlong(deadline), wine_dbgstr_longlong(predeadline), request);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqCancelDeadline(HANDLE request)
{
    FIXME("%p.\n", request);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqAllocateSerialWorkQueue(DWORD target_queue, DWORD *queue)
{
    struct queue_desc desc;

    TRACE("%#lx, %p.\n", target_queue, queue);

    desc.queue_type = RTWQ_STANDARD_WORKQUEUE;
    desc.ops = &serial_queue_ops;
    desc.target_queue = target_queue;
    return alloc_user_queue(&desc, queue);
}

HRESULT WINAPI RtwqJoinWorkQueue(DWORD queue, HANDLE hFile, HANDLE *cookie)
{
    FIXME("%#lx, %p, %p.\n", queue, hFile, cookie);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqUnjoinWorkQueue(DWORD queue, HANDLE cookie)
{
    FIXME("%#lx, %p.\n", queue, cookie);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqGetWorkQueueMMCSSClass(DWORD queue, WCHAR *class, DWORD *length)
{
    FIXME("%#lx, %p, %p.\n", queue, class, length);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqGetWorkQueueMMCSSTaskId(DWORD queue, DWORD *taskid)
{
    FIXME("%#lx, %p.\n", queue, taskid);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqGetWorkQueueMMCSSPriority(DWORD queue, LONG *priority)
{
    FIXME("%#lx, %p.\n", queue, priority);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqRegisterPlatformWithMMCSS(const WCHAR *class, DWORD *taskid, LONG priority)
{
    FIXME("%s, %p, %ld.\n", debugstr_w(class), taskid, priority);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqUnregisterPlatformFromMMCSS(void)
{
    FIXME("\n");

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqBeginRegisterWorkQueueWithMMCSS(DWORD queue, const WCHAR *class, DWORD taskid, LONG priority,
        IRtwqAsyncCallback *callback, IUnknown *state)
{
    FIXME("%#lx, %s, %lu, %ld, %p, %p.\n", queue, debugstr_w(class), taskid, priority, callback, state);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqEndRegisterWorkQueueWithMMCSS(IRtwqAsyncResult *result, DWORD *taskid)
{
    FIXME("%p, %p.\n", result, taskid);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqBeginUnregisterWorkQueueWithMMCSS(DWORD queue, IRtwqAsyncCallback *callback, IUnknown *state)
{
    FIXME("%#lx, %p, %p.\n", queue, callback, state);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqEndUnregisterWorkQueueWithMMCSS(IRtwqAsyncResult *result)
{
    FIXME("%p.\n", result);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqRegisterPlatformEvents(IRtwqPlatformEvents *events)
{
    FIXME("%p.\n", events);

    return E_NOTIMPL;
}

HRESULT WINAPI RtwqUnregisterPlatformEvents(IRtwqPlatformEvents *events)
{
    FIXME("%p.\n", events);

    return E_NOTIMPL;
}
