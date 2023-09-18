/*
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2010 Henri Verbeet for CodeWeavers.
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
#include "wine/port.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

static void wined3d_query_buffer_invalidate(struct wined3d_query *query)
{
    /* map[0] != map[1]: exact values do not have any significance. */
    query->map_ptr[0] = 0;
    query->map_ptr[1] = ~(UINT64)0;
}

static BOOL wined3d_query_buffer_is_valid(struct wined3d_query *query)
{
    return query->map_ptr[0] == query->map_ptr[1];
}

static void wined3d_query_create_buffer_object(struct wined3d_context_gl *context_gl, struct wined3d_query *query)
{
    const GLuint map_flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLuint buffer_object;

    GL_EXTCALL(glGenBuffers(1, &buffer_object));
    GL_EXTCALL(glBindBuffer(GL_QUERY_BUFFER, buffer_object));
    GL_EXTCALL(glBufferStorage(GL_QUERY_BUFFER, sizeof(query->map_ptr[0]) * 2, NULL, map_flags));
    query->map_ptr = GL_EXTCALL(glMapBufferRange(GL_QUERY_BUFFER, 0, sizeof(query->map_ptr[0]) * 2, map_flags));
    GL_EXTCALL(glBindBuffer(GL_QUERY_BUFFER, 0));
    checkGLcall("query buffer object creation");

    wined3d_query_buffer_invalidate(query);
    query->buffer_object = buffer_object;
}

void wined3d_query_gl_destroy_buffer_object(struct wined3d_context_gl *context_gl, struct wined3d_query *query)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    GL_EXTCALL(glDeleteBuffers(1, &query->buffer_object));
    checkGLcall("query buffer object destruction");

    query->buffer_object = 0;
    query->map_ptr = NULL;
}

/* From ARB_occlusion_query: "Querying the state for a given occlusion query
 * forces that occlusion query to complete within a finite amount of time."
 * In practice, that means drivers flush when retrieving
 * GL_QUERY_RESULT_AVAILABLE, which can be undesirable when applications use a
 * significant number of queries. Using a persistently mapped query buffer
 * object allows us to avoid these implicit flushes. An additional benefit is
 * that it allows us to poll the query status from the application-thread
 * instead of from the csmt-thread. */
static BOOL wined3d_query_buffer_queue_result(struct wined3d_context_gl *context_gl,
        struct wined3d_query *query, GLuint id)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLsync tmp_sync;

    if (!gl_info->supported[ARB_QUERY_BUFFER_OBJECT] || !gl_info->supported[ARB_BUFFER_STORAGE])
        return FALSE;
    /* Don't use query buffers without CSMT, mainly for simplicity. */
    if (!context_gl->c.device->cs->thread)
        return FALSE;

    if (query->buffer_object)
    {
        /* If there's still a query result in-flight for the existing buffer
         * object (i.e., the query was restarted before we received its
         * result), we can't reuse the existing buffer object. */
        if (wined3d_query_buffer_is_valid(query))
            wined3d_query_buffer_invalidate(query);
        else
            wined3d_query_gl_destroy_buffer_object(context_gl, query);
    }

    if (!query->buffer_object)
        wined3d_query_create_buffer_object(context_gl, query);

    GL_EXTCALL(glBindBuffer(GL_QUERY_BUFFER, query->buffer_object));
    /* Read the same value twice. We know we have the result if map_ptr[0] == map_ptr[1]. */
    GL_EXTCALL(glGetQueryObjectui64v(id, GL_QUERY_RESULT, (void *)0));
    GL_EXTCALL(glGetQueryObjectui64v(id, GL_QUERY_RESULT, (void *)sizeof(query->map_ptr[0])));
    GL_EXTCALL(glBindBuffer(GL_QUERY_BUFFER, 0));
    checkGLcall("queue query result");

    /* ARB_buffer_storage requires the client to call FenceSync with
     * SYNC_GPU_COMMANDS_COMPLETE after the server does a write. This behavior
     * is not enforced by Mesa.
     */
    tmp_sync = GL_EXTCALL(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
    GL_EXTCALL(glDeleteSync(tmp_sync));
    checkGLcall("query buffer sync");

    return TRUE;
}

static UINT64 get_query_result64(GLuint id, const struct wined3d_gl_info *gl_info)
{
    if (gl_info->supported[ARB_TIMER_QUERY])
    {
        GLuint64 result;
        GL_EXTCALL(glGetQueryObjectui64v(id, GL_QUERY_RESULT, &result));
        return result;
    }
    else
    {
        GLuint result;
        GL_EXTCALL(glGetQueryObjectuiv(id, GL_QUERY_RESULT, &result));
        return result;
    }
}

static void wined3d_query_init(struct wined3d_query *query, struct wined3d_device *device,
        enum wined3d_query_type type, const void *data, DWORD data_size,
        const struct wined3d_query_ops *query_ops, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    query->ref = 1;
    query->parent = parent;
    query->parent_ops = parent_ops;
    query->device = device;
    query->state = QUERY_CREATED;
    query->type = type;
    query->data = data;
    query->data_size = data_size;
    query->query_ops = query_ops;
    list_init(&query->poll_list_entry);
}

static struct wined3d_event_query *wined3d_event_query_from_query(struct wined3d_query *query)
{
    return CONTAINING_RECORD(query, struct wined3d_event_query, query);
}

static struct wined3d_occlusion_query *wined3d_occlusion_query_from_query(struct wined3d_query *query)
{
    return CONTAINING_RECORD(query, struct wined3d_occlusion_query, query);
}

static struct wined3d_timestamp_query *wined3d_timestamp_query_from_query(struct wined3d_query *query)
{
    return CONTAINING_RECORD(query, struct wined3d_timestamp_query, query);
}

static struct wined3d_so_statistics_query *wined3d_so_statistics_query_from_query(struct wined3d_query *query)
{
    return CONTAINING_RECORD(query, struct wined3d_so_statistics_query, query);
}

static struct wined3d_pipeline_statistics_query *wined3d_pipeline_statistics_query_from_query(
        struct wined3d_query *query)
{
    return CONTAINING_RECORD(query, struct wined3d_pipeline_statistics_query, query);
}

static BOOL wined3d_fence_supported(const struct wined3d_gl_info *gl_info)
{
    return gl_info->supported[ARB_SYNC] || gl_info->supported[NV_FENCE] || gl_info->supported[APPLE_FENCE];
}

static enum wined3d_fence_result wined3d_fence_test(const struct wined3d_fence *fence,
        struct wined3d_device *device, DWORD flags)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    enum wined3d_fence_result ret;
    BOOL fence_result;

    TRACE("fence %p, device %p, flags %#x.\n", fence, device, flags);

    if (!fence->context_gl)
    {
        TRACE("Fence not issued.\n");
        return WINED3D_FENCE_NOT_STARTED;
    }

    if (!(context_gl = wined3d_context_gl_reacquire(fence->context_gl)))
    {
        if (!fence->context_gl->gl_info->supported[ARB_SYNC])
        {
            WARN("Fence tested from wrong thread.\n");
            return WINED3D_FENCE_WRONG_THREAD;
        }
        context_gl = wined3d_context_gl(context_acquire(device, NULL, 0));
    }
    gl_info = context_gl->gl_info;

    if (gl_info->supported[ARB_SYNC])
    {
        GLenum gl_ret = GL_EXTCALL(glClientWaitSync(fence->object.sync,
                (flags & WINED3DGETDATA_FLUSH) ? GL_SYNC_FLUSH_COMMANDS_BIT : 0, 0));
        checkGLcall("glClientWaitSync");

        switch (gl_ret)
        {
            case GL_ALREADY_SIGNALED:
            case GL_CONDITION_SATISFIED:
                ret = WINED3D_FENCE_OK;
                break;

            case GL_TIMEOUT_EXPIRED:
                ret = WINED3D_FENCE_WAITING;
                break;

            case GL_WAIT_FAILED:
            default:
                ERR("glClientWaitSync returned %#x.\n", gl_ret);
                ret = WINED3D_FENCE_ERROR;
        }
    }
    else if (gl_info->supported[APPLE_FENCE])
    {
        fence_result = GL_EXTCALL(glTestFenceAPPLE(fence->object.id));
        checkGLcall("glTestFenceAPPLE");
        if (fence_result)
            ret = WINED3D_FENCE_OK;
        else
            ret = WINED3D_FENCE_WAITING;
    }
    else if (gl_info->supported[NV_FENCE])
    {
        fence_result = GL_EXTCALL(glTestFenceNV(fence->object.id));
        checkGLcall("glTestFenceNV");
        if (fence_result)
            ret = WINED3D_FENCE_OK;
        else
            ret = WINED3D_FENCE_WAITING;
    }
    else
    {
        ERR("Fence created despite lack of GL support.\n");
        ret = WINED3D_FENCE_ERROR;
    }

    context_release(&context_gl->c);
    return ret;
}

enum wined3d_fence_result wined3d_fence_wait(const struct wined3d_fence *fence,
        struct wined3d_device *device)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    enum wined3d_fence_result ret;

    TRACE("fence %p, device %p.\n", fence, device);

    if (!fence->context_gl)
    {
        TRACE("Fence not issued.\n");
        return WINED3D_FENCE_NOT_STARTED;
    }
    gl_info = fence->context_gl->gl_info;

    if (!(context_gl = wined3d_context_gl_reacquire(fence->context_gl)))
    {
        /* A glFinish does not reliably wait for draws in other contexts. The caller has
         * to find its own way to cope with the thread switch
         */
        if (!gl_info->supported[ARB_SYNC])
        {
            WARN("Fence finished from wrong thread.\n");
            return WINED3D_FENCE_WRONG_THREAD;
        }
        context_gl = wined3d_context_gl(context_acquire(device, NULL, 0));
    }
    gl_info = context_gl->gl_info;

    if (gl_info->supported[ARB_SYNC])
    {
        /* Timeouts near 0xffffffffffffffff may immediately return GL_TIMEOUT_EXPIRED,
         * possibly because macOS internally adds some slop to the timer. To avoid this,
         * we use a large number that isn't near the point of overflow (macOS 10.12.5).
         */
        GLenum gl_ret = GL_EXTCALL(glClientWaitSync(fence->object.sync,
                GL_SYNC_FLUSH_COMMANDS_BIT, ~(GLuint64)0 >> 1));
        checkGLcall("glClientWaitSync");

        switch (gl_ret)
        {
            case GL_ALREADY_SIGNALED:
            case GL_CONDITION_SATISFIED:
                ret = WINED3D_FENCE_OK;
                break;

                /* We don't expect a timeout for a ~292 year wait */
            default:
                ERR("glClientWaitSync returned %#x.\n", gl_ret);
                ret = WINED3D_FENCE_ERROR;
        }
    }
    else if (gl_info->supported[APPLE_FENCE])
    {
        GL_EXTCALL(glFinishFenceAPPLE(fence->object.id));
        checkGLcall("glFinishFenceAPPLE");
        ret = WINED3D_FENCE_OK;
    }
    else if (gl_info->supported[NV_FENCE])
    {
        GL_EXTCALL(glFinishFenceNV(fence->object.id));
        checkGLcall("glFinishFenceNV");
        ret = WINED3D_FENCE_OK;
    }
    else
    {
        ERR("Fence created without GL support.\n");
        ret = WINED3D_FENCE_ERROR;
    }

    context_release(&context_gl->c);
    return ret;
}

void wined3d_fence_issue(struct wined3d_fence *fence, struct wined3d_device *device)
{
    struct wined3d_context_gl *context_gl = NULL;
    const struct wined3d_gl_info *gl_info;

    if (fence->context_gl && !(context_gl = wined3d_context_gl_reacquire(fence->context_gl))
            && !fence->context_gl->gl_info->supported[ARB_SYNC])
        wined3d_context_gl_free_fence(fence);
    if (!context_gl)
        context_gl = wined3d_context_gl(context_acquire(device, NULL, 0));
    gl_info = context_gl->gl_info;
    if (!fence->context_gl)
        wined3d_context_gl_alloc_fence(context_gl, fence);

    if (gl_info->supported[ARB_SYNC])
    {
        if (fence->object.sync)
            GL_EXTCALL(glDeleteSync(fence->object.sync));
        checkGLcall("glDeleteSync");
        fence->object.sync = GL_EXTCALL(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
        checkGLcall("glFenceSync");
    }
    else if (gl_info->supported[APPLE_FENCE])
    {
        GL_EXTCALL(glSetFenceAPPLE(fence->object.id));
        checkGLcall("glSetFenceAPPLE");
    }
    else if (gl_info->supported[NV_FENCE])
    {
        GL_EXTCALL(glSetFenceNV(fence->object.id, GL_ALL_COMPLETED_NV));
        checkGLcall("glSetFenceNV");
    }

    context_release(&context_gl->c);
}

static void wined3d_fence_free(struct wined3d_fence *fence)
{
    if (fence->context_gl)
        wined3d_context_gl_free_fence(fence);
}

void wined3d_fence_destroy(struct wined3d_fence *fence)
{
    wined3d_fence_free(fence);
    heap_free(fence);
}

static HRESULT wined3d_fence_init(struct wined3d_fence *fence, const struct wined3d_gl_info *gl_info)
{
    if (!wined3d_fence_supported(gl_info))
    {
        WARN("Fences not supported.\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    return WINED3D_OK;
}

HRESULT wined3d_fence_create(struct wined3d_device *device, struct wined3d_fence **fence)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_fence *object;
    HRESULT hr;

    TRACE("device %p, fence %p.\n", device, fence);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_fence_init(object, gl_info)))
    {
        heap_free(object);
        return hr;
    }

    TRACE("Created fence %p.\n", object);
    *fence = object;

    return WINED3D_OK;
}

ULONG CDECL wined3d_query_incref(struct wined3d_query *query)
{
    ULONG refcount = InterlockedIncrement(&query->ref);

    TRACE("%p increasing refcount to %u.\n", query, refcount);

    return refcount;
}

static void wined3d_query_destroy_object(void *object)
{
    struct wined3d_query *query = object;

    if (!list_empty(&query->poll_list_entry))
        list_remove(&query->poll_list_entry);
}

ULONG CDECL wined3d_query_decref(struct wined3d_query *query)
{
    ULONG refcount = InterlockedDecrement(&query->ref);

    TRACE("%p decreasing refcount to %u.\n", query, refcount);

    if (!refcount)
    {
        struct wined3d_device *device = query->device;

        query->parent_ops->wined3d_object_destroyed(query->parent);
        wined3d_cs_destroy_object(device->cs, wined3d_query_destroy_object, query);
        device->adapter->adapter_ops->adapter_destroy_query(query);
    }

    return refcount;
}

HRESULT CDECL wined3d_query_get_data(struct wined3d_query *query,
        void *data, UINT data_size, DWORD flags)
{
    TRACE("query %p, data %p, data_size %u, flags %#x.\n",
            query, data, data_size, flags);

    if (flags)
        WARN("Ignoring flags %#x.\n", flags);

    if (query->state == QUERY_BUILDING)
    {
        WARN("Query is building, returning S_FALSE.\n");
        return S_FALSE;
    }

    if (query->state == QUERY_CREATED)
    {
        WARN("Query wasn't started yet.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (query->device->cs->thread)
    {
        if (query->counter_main != query->counter_retrieved
                || (query->buffer_object && !wined3d_query_buffer_is_valid(query)))
        {
            if (flags & WINED3DGETDATA_FLUSH && !query->device->cs->queries_flushed)
                wined3d_cs_emit_flush(query->device->cs);
            return S_FALSE;
        }
        if (query->buffer_object)
            query->data = query->map_ptr;
    }
    else if (!query->query_ops->query_poll(query, flags))
    {
        return S_FALSE;
    }

    if (data)
        memcpy(data, query->data, min(data_size, query->data_size));

    return S_OK;
}

UINT CDECL wined3d_query_get_data_size(const struct wined3d_query *query)
{
    TRACE("query %p.\n", query);

    return query->data_size;
}

HRESULT CDECL wined3d_query_issue(struct wined3d_query *query, DWORD flags)
{
    TRACE("query %p, flags %#x.\n", query, flags);

    if (flags & WINED3DISSUE_END)
        ++query->counter_main;

    wined3d_cs_emit_query_issue(query->device->cs, query, flags);

    if (flags & WINED3DISSUE_BEGIN)
        query->state = QUERY_BUILDING;
    else
        query->state = QUERY_SIGNALLED;

    return WINED3D_OK;
}

static BOOL wined3d_occlusion_query_ops_poll(struct wined3d_query *query, DWORD flags)
{
    struct wined3d_occlusion_query *oq = wined3d_occlusion_query_from_query(query);
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    GLuint available;

    TRACE("query %p, flags %#x.\n", query, flags);

    if (!(context_gl = wined3d_context_gl_reacquire(oq->context_gl)))
    {
        FIXME("%p Wrong thread, returning 1.\n", query);
        oq->samples = 1;
        return TRUE;
    }
    gl_info = context_gl->gl_info;

    GL_EXTCALL(glGetQueryObjectuiv(oq->id, GL_QUERY_RESULT_AVAILABLE, &available));
    TRACE("Available %#x.\n", available);

    if (available)
    {
        oq->samples = get_query_result64(oq->id, gl_info);
        TRACE("Returning 0x%s samples.\n", wine_dbgstr_longlong(oq->samples));
    }

    checkGLcall("poll occlusion query");
    context_release(&context_gl->c);

    return available;
}

static BOOL wined3d_event_query_ops_poll(struct wined3d_query *query, DWORD flags)
{
    struct wined3d_event_query *event_query = wined3d_event_query_from_query(query);
    enum wined3d_fence_result ret;

    TRACE("query %p, flags %#x.\n", query, flags);

    ret = wined3d_fence_test(&event_query->fence, query->device, flags);
    switch (ret)
    {
        case WINED3D_FENCE_OK:
        case WINED3D_FENCE_NOT_STARTED:
            return event_query->signalled = TRUE;

        case WINED3D_FENCE_WAITING:
            return event_query->signalled = FALSE;

        case WINED3D_FENCE_WRONG_THREAD:
            FIXME("(%p) Wrong thread, reporting GPU idle.\n", query);
            return event_query->signalled = TRUE;

        case WINED3D_FENCE_ERROR:
            ERR("The GL event query failed.\n");
            return event_query->signalled = TRUE;

        default:
            ERR("Unexpected wined3d_event_query_test result %#x.\n", ret);
            return event_query->signalled = TRUE;
    }
}

void * CDECL wined3d_query_get_parent(const struct wined3d_query *query)
{
    TRACE("query %p.\n", query);

    return query->parent;
}

enum wined3d_query_type CDECL wined3d_query_get_type(const struct wined3d_query *query)
{
    TRACE("query %p.\n", query);

    return query->type;
}

static BOOL wined3d_event_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    TRACE("query %p, flags %#x.\n", query, flags);

    if (flags & WINED3DISSUE_END)
    {
        struct wined3d_event_query *event_query = wined3d_event_query_from_query(query);

        wined3d_fence_issue(&event_query->fence, query->device);
        return TRUE;
    }
    else if (flags & WINED3DISSUE_BEGIN)
    {
        /* Started implicitly at query creation. */
        ERR("Event query issued with START flag - what to do?\n");
    }

    return FALSE;
}

static BOOL wined3d_occlusion_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    struct wined3d_occlusion_query *oq = wined3d_occlusion_query_from_query(query);
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    BOOL poll = FALSE;

    TRACE("query %p, flags %#x.\n", query, flags);

    /* This is allowed according to MSDN and our tests. Reset the query and
     * restart. */
    if (flags & WINED3DISSUE_BEGIN)
    {
        if (oq->started)
        {
            if ((context_gl = wined3d_context_gl_reacquire(oq->context_gl)))
            {
                gl_info = context_gl->gl_info;
                GL_EXTCALL(glEndQuery(GL_SAMPLES_PASSED));
                checkGLcall("glEndQuery()");
            }
            else
            {
                FIXME("Wrong thread, can't restart query.\n");
                wined3d_context_gl_free_occlusion_query(oq);
                context_gl = wined3d_context_gl(context_acquire(device, NULL, 0));
                wined3d_context_gl_alloc_occlusion_query(context_gl, oq);
            }
        }
        else
        {
            if (oq->context_gl)
                wined3d_context_gl_free_occlusion_query(oq);
            context_gl = wined3d_context_gl(context_acquire(device, NULL, 0));
            wined3d_context_gl_alloc_occlusion_query(context_gl, oq);
        }
        gl_info = context_gl->gl_info;

        GL_EXTCALL(glBeginQuery(GL_SAMPLES_PASSED, oq->id));
        checkGLcall("glBeginQuery()");

        context_release(&context_gl->c);
        oq->started = TRUE;
    }
    if (flags & WINED3DISSUE_END)
    {
        /* MSDN says END on a non-building occlusion query returns an error,
         * but our tests show that it returns OK. But OpenGL doesn't like it,
         * so avoid generating an error. */
        if (oq->started)
        {
            if ((context_gl = wined3d_context_gl_reacquire(oq->context_gl)))
            {
                gl_info = context_gl->gl_info;
                GL_EXTCALL(glEndQuery(GL_SAMPLES_PASSED));
                checkGLcall("glEndQuery()");
                wined3d_query_buffer_queue_result(context_gl, query, oq->id);

                context_release(&context_gl->c);
                poll = TRUE;
            }
            else
            {
                FIXME("Wrong thread, can't end query.\n");
            }
        }
        oq->started = FALSE;
    }

    return poll;
}

static BOOL wined3d_timestamp_query_ops_poll(struct wined3d_query *query, DWORD flags)
{
    struct wined3d_timestamp_query *tq = wined3d_timestamp_query_from_query(query);
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    GLuint64 timestamp;
    GLuint available;

    TRACE("query %p, flags %#x.\n", query, flags);

    if (!(context_gl = wined3d_context_gl_reacquire(tq->context_gl)))
    {
        FIXME("%p Wrong thread, returning 1.\n", query);
        tq->timestamp = 1;
        return TRUE;
    }
    gl_info = context_gl->gl_info;

    GL_EXTCALL(glGetQueryObjectuiv(tq->id, GL_QUERY_RESULT_AVAILABLE, &available));
    checkGLcall("glGetQueryObjectuiv(GL_QUERY_RESULT_AVAILABLE)");
    TRACE("available %#x.\n", available);

    if (available)
    {
        GL_EXTCALL(glGetQueryObjectui64v(tq->id, GL_QUERY_RESULT, &timestamp));
        checkGLcall("glGetQueryObjectui64v(GL_QUERY_RESULT)");
        TRACE("Returning timestamp %s.\n", wine_dbgstr_longlong(timestamp));
        tq->timestamp = timestamp;
    }

    context_release(&context_gl->c);

    return available;
}

static BOOL wined3d_timestamp_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    struct wined3d_timestamp_query *tq = wined3d_timestamp_query_from_query(query);
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;

    TRACE("query %p, flags %#x.\n", query, flags);

    if (flags & WINED3DISSUE_BEGIN)
    {
        WARN("Ignoring WINED3DISSUE_BEGIN with a TIMESTAMP query.\n");
    }
    if (flags & WINED3DISSUE_END)
    {
        if (tq->context_gl)
            wined3d_context_gl_free_timestamp_query(tq);
        context_gl = wined3d_context_gl(context_acquire(query->device, NULL, 0));
        gl_info = context_gl->gl_info;
        wined3d_context_gl_alloc_timestamp_query(context_gl, tq);
        GL_EXTCALL(glQueryCounter(tq->id, GL_TIMESTAMP));
        checkGLcall("glQueryCounter()");
        context_release(&context_gl->c);

        return TRUE;
    }

    return FALSE;
}

static BOOL wined3d_timestamp_disjoint_query_ops_poll(struct wined3d_query *query, DWORD flags)
{
    TRACE("query %p, flags %#x.\n", query, flags);

    return TRUE;
}

static BOOL wined3d_timestamp_disjoint_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    TRACE("query %p, flags %#x.\n", query, flags);

    return FALSE;
}

static BOOL wined3d_so_statistics_query_ops_poll(struct wined3d_query *query, DWORD flags)
{
    struct wined3d_so_statistics_query *pq = wined3d_so_statistics_query_from_query(query);
    GLuint written_available, generated_available;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;

    TRACE("query %p, flags %#x.\n", query, flags);

    if (!(context_gl = wined3d_context_gl_reacquire(pq->context_gl)))
    {
        FIXME("%p Wrong thread, returning 0 primitives.\n", query);
        memset(&pq->statistics, 0, sizeof(pq->statistics));
        return TRUE;
    }
    gl_info = context_gl->gl_info;

    GL_EXTCALL(glGetQueryObjectuiv(pq->u.query.written,
            GL_QUERY_RESULT_AVAILABLE, &written_available));
    GL_EXTCALL(glGetQueryObjectuiv(pq->u.query.generated,
            GL_QUERY_RESULT_AVAILABLE, &generated_available));
    TRACE("Available %#x, %#x.\n", written_available, generated_available);

    if (written_available && generated_available)
    {
        pq->statistics.primitives_written = get_query_result64(pq->u.query.written, gl_info);
        pq->statistics.primitives_generated = get_query_result64(pq->u.query.generated, gl_info);
        TRACE("Returning %s, %s primitives.\n",
                wine_dbgstr_longlong(pq->statistics.primitives_written),
                wine_dbgstr_longlong(pq->statistics.primitives_generated));
    }

    checkGLcall("poll SO statistics query");
    context_release(&context_gl->c);

    return written_available && generated_available;
}

static void wined3d_so_statistics_query_end(struct wined3d_so_statistics_query *query,
        struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (gl_info->supported[ARB_TRANSFORM_FEEDBACK3])
    {
        GL_EXTCALL(glEndQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query->stream_idx));
        GL_EXTCALL(glEndQueryIndexed(GL_PRIMITIVES_GENERATED, query->stream_idx));
    }
    else
    {
        GL_EXTCALL(glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN));
        GL_EXTCALL(glEndQuery(GL_PRIMITIVES_GENERATED));
    }
    checkGLcall("end query");
}

static BOOL wined3d_so_statistics_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    struct wined3d_so_statistics_query *pq = wined3d_so_statistics_query_from_query(query);
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    BOOL poll = FALSE;

    TRACE("query %p, flags %#x.\n", query, flags);

    if (flags & WINED3DISSUE_BEGIN)
    {
        if (pq->started)
        {
            if ((context_gl = wined3d_context_gl_reacquire(pq->context_gl)))
            {
                wined3d_so_statistics_query_end(pq, context_gl);
            }
            else
            {
                FIXME("Wrong thread, can't restart query.\n");
                wined3d_context_gl_free_so_statistics_query(pq);
                context_gl = wined3d_context_gl(context_acquire(device, NULL, 0));
                wined3d_context_gl_alloc_so_statistics_query(context_gl, pq);
            }
        }
        else
        {
            if (pq->context_gl)
                wined3d_context_gl_free_so_statistics_query(pq);
            context_gl = wined3d_context_gl(context_acquire(device, NULL, 0));
            wined3d_context_gl_alloc_so_statistics_query(context_gl, pq);
        }
        gl_info = context_gl->gl_info;

        if (gl_info->supported[ARB_TRANSFORM_FEEDBACK3])
        {
            GL_EXTCALL(glBeginQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,
                    pq->stream_idx, pq->u.query.written));
            GL_EXTCALL(glBeginQueryIndexed(GL_PRIMITIVES_GENERATED,
                    pq->stream_idx, pq->u.query.generated));
        }
        else
        {
            GL_EXTCALL(glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,
                    pq->u.query.written));
            GL_EXTCALL(glBeginQuery(GL_PRIMITIVES_GENERATED,
                    pq->u.query.generated));
        }
        checkGLcall("begin query");

        context_release(&context_gl->c);
        pq->started = TRUE;
    }
    if (flags & WINED3DISSUE_END)
    {
        if (pq->started)
        {
            if ((context_gl = wined3d_context_gl_reacquire(pq->context_gl)))
            {
                wined3d_so_statistics_query_end(pq, context_gl);

                context_release(&context_gl->c);
                poll = TRUE;
            }
            else
            {
                FIXME("Wrong thread, can't end query.\n");
            }
        }
        pq->started = FALSE;
    }

    return poll;
}

static BOOL wined3d_pipeline_query_ops_poll(struct wined3d_query *query, DWORD flags)
{
    struct wined3d_pipeline_statistics_query *pq = wined3d_pipeline_statistics_query_from_query(query);
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    GLuint available;
    int i;

    TRACE("query %p, flags %#x.\n", query, flags);

    if (!(context_gl = wined3d_context_gl_reacquire(pq->context_gl)))
    {
        FIXME("%p Wrong thread.\n", query);
        memset(&pq->statistics, 0, sizeof(pq->statistics));
        return TRUE;
    }
    gl_info = context_gl->gl_info;

    for (i = 0; i < ARRAY_SIZE(pq->u.id); ++i)
    {
        GL_EXTCALL(glGetQueryObjectuiv(pq->u.id[i], GL_QUERY_RESULT_AVAILABLE, &available));
        if (!available)
            break;
    }

    if (available)
    {
        pq->statistics.vertices_submitted = get_query_result64(pq->u.query.vertices, gl_info);
        pq->statistics.primitives_submitted = get_query_result64(pq->u.query.primitives, gl_info);
        pq->statistics.vs_invocations = get_query_result64(pq->u.query.vertex_shader, gl_info);
        pq->statistics.hs_invocations = get_query_result64(pq->u.query.tess_control_shader, gl_info);
        pq->statistics.ds_invocations = get_query_result64(pq->u.query.tess_eval_shader, gl_info);
        pq->statistics.gs_invocations = get_query_result64(pq->u.query.geometry_shader, gl_info);
        pq->statistics.gs_primitives = get_query_result64(pq->u.query.geometry_primitives, gl_info);
        pq->statistics.ps_invocations = get_query_result64(pq->u.query.fragment_shader, gl_info);
        pq->statistics.cs_invocations = get_query_result64(pq->u.query.compute_shader, gl_info);
        pq->statistics.clipping_input_primitives = get_query_result64(pq->u.query.clipping_input, gl_info);
        pq->statistics.clipping_output_primitives = get_query_result64(pq->u.query.clipping_output, gl_info);
    }

    checkGLcall("poll pipeline statistics query");
    context_release(&context_gl->c);
    return available;
}

static void wined3d_pipeline_statistics_query_end(struct wined3d_pipeline_statistics_query *query,
        struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    GL_EXTCALL(glEndQuery(GL_VERTICES_SUBMITTED_ARB));
    GL_EXTCALL(glEndQuery(GL_PRIMITIVES_SUBMITTED_ARB));
    GL_EXTCALL(glEndQuery(GL_VERTEX_SHADER_INVOCATIONS_ARB));
    GL_EXTCALL(glEndQuery(GL_TESS_CONTROL_SHADER_PATCHES_ARB));
    GL_EXTCALL(glEndQuery(GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB));
    GL_EXTCALL(glEndQuery(GL_GEOMETRY_SHADER_INVOCATIONS));
    GL_EXTCALL(glEndQuery(GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB));
    GL_EXTCALL(glEndQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB));
    GL_EXTCALL(glEndQuery(GL_COMPUTE_SHADER_INVOCATIONS_ARB));
    GL_EXTCALL(glEndQuery(GL_CLIPPING_INPUT_PRIMITIVES_ARB));
    GL_EXTCALL(glEndQuery(GL_CLIPPING_OUTPUT_PRIMITIVES_ARB));
    checkGLcall("end query");
}

static BOOL wined3d_pipeline_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    struct wined3d_pipeline_statistics_query *pq = wined3d_pipeline_statistics_query_from_query(query);
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    BOOL poll = FALSE;

    TRACE("query %p, flags %#x.\n", query, flags);

    if (flags & WINED3DISSUE_BEGIN)
    {
        if (pq->started)
        {
            if ((context_gl = wined3d_context_gl_reacquire(pq->context_gl)))
            {
                wined3d_pipeline_statistics_query_end(pq, context_gl);
            }
            else
            {
                FIXME("Wrong thread, can't restart query.\n");
                wined3d_context_gl_free_pipeline_statistics_query(pq);
                context_gl = wined3d_context_gl(context_acquire(device, NULL, 0));
                wined3d_context_gl_alloc_pipeline_statistics_query(context_gl, pq);
            }
        }
        else
        {
            if (pq->context_gl)
                wined3d_context_gl_free_pipeline_statistics_query(pq);
            context_gl = wined3d_context_gl(context_acquire(device, NULL, 0));
            wined3d_context_gl_alloc_pipeline_statistics_query(context_gl, pq);
        }
        gl_info = context_gl->gl_info;

        GL_EXTCALL(glBeginQuery(GL_VERTICES_SUBMITTED_ARB, pq->u.query.vertices));
        GL_EXTCALL(glBeginQuery(GL_PRIMITIVES_SUBMITTED_ARB, pq->u.query.primitives));
        GL_EXTCALL(glBeginQuery(GL_VERTEX_SHADER_INVOCATIONS_ARB, pq->u.query.vertex_shader));
        GL_EXTCALL(glBeginQuery(GL_TESS_CONTROL_SHADER_PATCHES_ARB, pq->u.query.tess_control_shader));
        GL_EXTCALL(glBeginQuery(GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB, pq->u.query.tess_eval_shader));
        GL_EXTCALL(glBeginQuery(GL_GEOMETRY_SHADER_INVOCATIONS, pq->u.query.geometry_shader));
        GL_EXTCALL(glBeginQuery(GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB, pq->u.query.geometry_primitives));
        GL_EXTCALL(glBeginQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB, pq->u.query.fragment_shader));
        GL_EXTCALL(glBeginQuery(GL_COMPUTE_SHADER_INVOCATIONS_ARB, pq->u.query.compute_shader));
        GL_EXTCALL(glBeginQuery(GL_CLIPPING_INPUT_PRIMITIVES_ARB, pq->u.query.clipping_input));
        GL_EXTCALL(glBeginQuery(GL_CLIPPING_OUTPUT_PRIMITIVES_ARB, pq->u.query.clipping_output));
        checkGLcall("begin query");

        context_release(&context_gl->c);
        pq->started = TRUE;
    }
    if (flags & WINED3DISSUE_END)
    {
        if (pq->started)
        {
            if ((context_gl = wined3d_context_gl_reacquire(pq->context_gl)))
            {
                wined3d_pipeline_statistics_query_end(pq, context_gl);
                context_release(&context_gl->c);
                poll = TRUE;
            }
            else
            {
                FIXME("Wrong thread, can't end query.\n");
            }
        }
        pq->started = FALSE;
    }

    return poll;
}

static BOOL wined3d_statistics_query_ops_poll(struct wined3d_query *query, DWORD flags)
{
    TRACE("query %p, flags %#x.\n", query, flags);

    return TRUE;
}

static BOOL wined3d_statistics_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    FIXME("query %p, flags %#x.\n", query, flags);

    return FALSE;
}

static BOOL wined3d_overflow_query_ops_poll(struct wined3d_query *query, DWORD flags)
{
    TRACE("query %p, flags %#x.\n", query, flags);

    return TRUE;
}

static BOOL wined3d_overflow_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    FIXME("query %p, flags %#x.\n", query, flags);

    return FALSE;
}

static void wined3d_event_query_ops_destroy(struct wined3d_query *query)
{
    struct wined3d_event_query *event_query = wined3d_event_query_from_query(query);

    wined3d_fence_free(&event_query->fence);
    heap_free(event_query);
}

static const struct wined3d_query_ops event_query_ops =
{
    wined3d_event_query_ops_poll,
    wined3d_event_query_ops_issue,
    wined3d_event_query_ops_destroy,
};

static HRESULT wined3d_event_query_create(struct wined3d_device *device,
        enum wined3d_query_type type, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_query **query)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_event_query *object;
    HRESULT hr;

    TRACE("device %p, type %#x, parent %p, parent_ops %p, query %p.\n",
            device, type, parent, parent_ops, query);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_fence_init(&object->fence, gl_info)))
    {
        WARN("Event queries not supported.\n");
        heap_free(object);
        return hr;
    }

    wined3d_query_init(&object->query, device, type, &object->signalled,
            sizeof(object->signalled), &event_query_ops, parent, parent_ops);

    TRACE("Created query %p.\n", object);
    *query = &object->query;

    return WINED3D_OK;
}

static void wined3d_occlusion_query_ops_destroy(struct wined3d_query *query)
{
    struct wined3d_occlusion_query *oq = wined3d_occlusion_query_from_query(query);

    if (oq->context_gl)
        wined3d_context_gl_free_occlusion_query(oq);
    heap_free(oq);
}

static const struct wined3d_query_ops occlusion_query_ops =
{
    wined3d_occlusion_query_ops_poll,
    wined3d_occlusion_query_ops_issue,
    wined3d_occlusion_query_ops_destroy,
};

static HRESULT wined3d_occlusion_query_create(struct wined3d_device *device,
        enum wined3d_query_type type, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_query **query)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_occlusion_query *object;

    TRACE("device %p, type %#x, parent %p, parent_ops %p, query %p.\n",
            device, type, parent, parent_ops, query);

    if (!gl_info->supported[ARB_OCCLUSION_QUERY])
    {
        WARN("Unsupported in local OpenGL implementation: ARB_OCCLUSION_QUERY.\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    wined3d_query_init(&object->query, device, type, &object->samples,
            sizeof(object->samples), &occlusion_query_ops, parent, parent_ops);

    TRACE("Created query %p.\n", object);
    *query = &object->query;

    return WINED3D_OK;
}

static void wined3d_timestamp_query_ops_destroy(struct wined3d_query *query)
{
    struct wined3d_timestamp_query *tq = wined3d_timestamp_query_from_query(query);

    if (tq->context_gl)
        wined3d_context_gl_free_timestamp_query(tq);
    heap_free(tq);
}

static const struct wined3d_query_ops timestamp_query_ops =
{
    wined3d_timestamp_query_ops_poll,
    wined3d_timestamp_query_ops_issue,
    wined3d_timestamp_query_ops_destroy,
};

static HRESULT wined3d_timestamp_query_create(struct wined3d_device *device,
        enum wined3d_query_type type, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_query **query)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_timestamp_query *object;

    TRACE("device %p, type %#x, parent %p, parent_ops %p, query %p.\n",
            device, type, parent, parent_ops, query);

    if (!gl_info->supported[ARB_TIMER_QUERY])
    {
        WARN("Unsupported in local OpenGL implementation: ARB_TIMER_QUERY.\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    wined3d_query_init(&object->query, device, type, &object->timestamp,
            sizeof(object->timestamp), &timestamp_query_ops, parent, parent_ops);

    TRACE("Created query %p.\n", object);
    *query = &object->query;

    return WINED3D_OK;
}

static void wined3d_timestamp_disjoint_query_ops_destroy(struct wined3d_query *query)
{
    heap_free(query);
}

static const struct wined3d_query_ops timestamp_disjoint_query_ops =
{
    wined3d_timestamp_disjoint_query_ops_poll,
    wined3d_timestamp_disjoint_query_ops_issue,
    wined3d_timestamp_disjoint_query_ops_destroy,
};

static HRESULT wined3d_timestamp_disjoint_query_create(struct wined3d_device *device,
        enum wined3d_query_type type, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_query **query)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_query *object;

    TRACE("device %p, type %#x, parent %p, parent_ops %p, query %p.\n",
            device, type, parent, parent_ops, query);

    if (!gl_info->supported[ARB_TIMER_QUERY])
    {
        WARN("Unsupported in local OpenGL implementation: ARB_TIMER_QUERY.\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (type == WINED3D_QUERY_TYPE_TIMESTAMP_DISJOINT)
    {
        static const struct wined3d_query_data_timestamp_disjoint disjoint_data = {1000 * 1000 * 1000, FALSE};

        wined3d_query_init(object, device, type, &disjoint_data,
                sizeof(disjoint_data), &timestamp_disjoint_query_ops, parent, parent_ops);
    }
    else
    {
        static const UINT64 freq = 1000 * 1000 * 1000;

        wined3d_query_init(object, device, type, &freq,
                sizeof(freq), &timestamp_disjoint_query_ops, parent, parent_ops);
    }

    TRACE("Created query %p.\n", object);
    *query = object;

    return WINED3D_OK;
}

static void wined3d_so_statistics_query_ops_destroy(struct wined3d_query *query)
{
    struct wined3d_so_statistics_query *pq = wined3d_so_statistics_query_from_query(query);

    if (pq->context_gl)
        wined3d_context_gl_free_so_statistics_query(pq);
    heap_free(pq);
}

static const struct wined3d_query_ops so_statistics_query_ops =
{
    wined3d_so_statistics_query_ops_poll,
    wined3d_so_statistics_query_ops_issue,
    wined3d_so_statistics_query_ops_destroy,
};

static HRESULT wined3d_so_statistics_query_create(struct wined3d_device *device,
        enum wined3d_query_type type, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_query **query)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_so_statistics_query *object;
    unsigned int stream_idx;

    if (WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM0 <= type && type <= WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM3)
        stream_idx = type - WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM0;
    else if (type == WINED3D_QUERY_TYPE_SO_STATISTICS)
        stream_idx = 0;
    else
        return WINED3DERR_NOTAVAILABLE;

    TRACE("device %p, type %#x, parent %p, parent_ops %p, query %p.\n",
            device, type, parent, parent_ops, query);

    if (!gl_info->supported[WINED3D_GL_PRIMITIVE_QUERY])
    {
        WARN("OpenGL implementation does not support primitive queries.\n");
        return WINED3DERR_NOTAVAILABLE;
    }
    if (stream_idx && !gl_info->supported[ARB_TRANSFORM_FEEDBACK3])
    {
        WARN("OpenGL implementation does not support indexed queries.\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    wined3d_query_init(&object->query, device, type, &object->statistics,
            sizeof(object->statistics), &so_statistics_query_ops, parent, parent_ops);
    object->stream_idx = stream_idx;

    TRACE("Created query %p.\n", object);
    *query = &object->query;

    return WINED3D_OK;
}

static void wined3d_pipeline_query_ops_destroy(struct wined3d_query *query)
{
    struct wined3d_pipeline_statistics_query *pq = wined3d_pipeline_statistics_query_from_query(query);
    if (pq->context_gl)
        wined3d_context_gl_free_pipeline_statistics_query(pq);
    heap_free(pq);
}

static const struct wined3d_query_ops pipeline_query_ops =
{
    wined3d_pipeline_query_ops_poll,
    wined3d_pipeline_query_ops_issue,
    wined3d_pipeline_query_ops_destroy,
};

static HRESULT wined3d_pipeline_query_create(struct wined3d_device *device,
        enum wined3d_query_type type, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_query **query)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_pipeline_statistics_query *object;

    TRACE("device %p, type %#x, parent %p, parent_ops %p, query %p.\n",
            device, type, parent, parent_ops, query);

    if (!gl_info->supported[ARB_PIPELINE_STATISTICS_QUERY])
    {
        WARN("OpenGL implementation does not support pipeline statistics queries.\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    wined3d_query_init(&object->query, device, type, &object->statistics,
            sizeof(object->statistics), &pipeline_query_ops, parent, parent_ops);

    TRACE("Created query %p.\n", object);
    *query = &object->query;

    return WINED3D_OK;
}

static void wined3d_statistics_query_ops_destroy(struct wined3d_query *query)
{
    HeapFree(GetProcessHeap(), 0, query);
}

static const struct wined3d_query_ops statistics_query_ops =
{
    wined3d_statistics_query_ops_poll,
    wined3d_statistics_query_ops_issue,
    wined3d_statistics_query_ops_destroy,
};

#ifdef __REACTOS__
#else
static HRESULT wined3d_statistics_query_create(struct wined3d_device *device,
        enum wined3d_query_type type, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_query **query)
{
    static const struct wined3d_query_data_so_statistics statistics = { 1, 1 };
    struct wined3d_query *object;

    FIXME("device %p, type %#x, parent %p, query %p.\n", device, type, parent, query);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    wined3d_query_init(object, device, type, &statistics,
            sizeof(statistics), &statistics_query_ops, parent, parent_ops);

    TRACE("Created query %p.\n", object);
    *query = object;

    return WINED3D_OK;
}
#endif

static void wined3d_overflow_query_ops_destroy(struct wined3d_query *query)
{
    HeapFree(GetProcessHeap(), 0, query);
}

static const struct wined3d_query_ops overflow_query_ops =
{
    wined3d_overflow_query_ops_poll,
    wined3d_overflow_query_ops_issue,
    wined3d_overflow_query_ops_destroy,
};

static HRESULT wined3d_overflow_query_create(struct wined3d_device *device,
        enum wined3d_query_type type, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_query **query)
{
    static const BOOL overflow = FALSE;
    struct wined3d_query *object;

    FIXME("device %p, type %#x, parent %p, query %p.\n", device, type, parent, query);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    wined3d_query_init(object, device, type, &overflow,
            sizeof(overflow), &overflow_query_ops, parent, parent_ops);

    TRACE("Created query %p.\n", object);
    *query = object;

    return WINED3D_OK;
}

HRESULT wined3d_query_gl_create(struct wined3d_device *device, enum wined3d_query_type type,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_query **query)
{
    TRACE("device %p, type %#x, parent %p, parent_ops %p, query %p.\n",
            device, type, parent, parent_ops, query);

    switch (type)
    {
        case WINED3D_QUERY_TYPE_EVENT:
            return wined3d_event_query_create(device, type, parent, parent_ops, query);

        case WINED3D_QUERY_TYPE_OCCLUSION:
            return wined3d_occlusion_query_create(device, type, parent, parent_ops, query);

        case WINED3D_QUERY_TYPE_TIMESTAMP:
            return wined3d_timestamp_query_create(device, type, parent, parent_ops, query);

        case WINED3D_QUERY_TYPE_TIMESTAMP_DISJOINT:
        case WINED3D_QUERY_TYPE_TIMESTAMP_FREQ:
            return wined3d_timestamp_disjoint_query_create(device, type, parent, parent_ops, query);

        case WINED3D_QUERY_TYPE_SO_STATISTICS:
        case WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM0:
        case WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM1:
        case WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM2:
        case WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM3:
            return wined3d_so_statistics_query_create(device, type, parent, parent_ops, query);

        case WINED3D_QUERY_TYPE_PIPELINE_STATISTICS:
            return wined3d_pipeline_query_create(device, type, parent, parent_ops, query);\

        case WINED3D_QUERY_TYPE_SO_OVERFLOW:
            return wined3d_overflow_query_create(device, type, parent, parent_ops, query);

        default:
            FIXME("Unhandled query type %#x.\n", type);
            return WINED3DERR_NOTAVAILABLE;
    }
}

HRESULT CDECL wined3d_query_create(struct wined3d_device *device, enum wined3d_query_type type,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_query **query)
{
    TRACE("device %p, type %#x, parent %p, parent_ops %p, query %p.\n",
            device, type, parent, parent_ops, query);

    return device->adapter->adapter_ops->adapter_create_query(device, type, parent, parent_ops, query);
}
