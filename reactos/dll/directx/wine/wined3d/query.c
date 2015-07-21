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


#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

BOOL wined3d_event_query_supported(const struct wined3d_gl_info *gl_info)
{
    return gl_info->supported[ARB_SYNC] || gl_info->supported[NV_FENCE] || gl_info->supported[APPLE_FENCE];
}

void wined3d_event_query_destroy(struct wined3d_event_query *query)
{
    if (query->context) context_free_event_query(query);
    HeapFree(GetProcessHeap(), 0, query);
}

static enum wined3d_event_query_result wined3d_event_query_test(const struct wined3d_event_query *query,
        const struct wined3d_device *device)
{
    struct wined3d_context *context;
    const struct wined3d_gl_info *gl_info;
    enum wined3d_event_query_result ret;
    BOOL fence_result;

    TRACE("(%p) : device %p\n", query, device);

    if (!query->context)
    {
        TRACE("Query not started\n");
        return WINED3D_EVENT_QUERY_NOT_STARTED;
    }

    if (!query->context->gl_info->supported[ARB_SYNC] && query->context->tid != GetCurrentThreadId())
    {
        WARN("Event query tested from wrong thread\n");
        return WINED3D_EVENT_QUERY_WRONG_THREAD;
    }

    context = context_acquire(device, query->context->current_rt);
    gl_info = context->gl_info;

    if (gl_info->supported[ARB_SYNC])
    {
        GLenum gl_ret = GL_EXTCALL(glClientWaitSync(query->object.sync, 0, 0));
        checkGLcall("glClientWaitSync");

        switch (gl_ret)
        {
            case GL_ALREADY_SIGNALED:
            case GL_CONDITION_SATISFIED:
                ret = WINED3D_EVENT_QUERY_OK;
                break;

            case GL_TIMEOUT_EXPIRED:
                ret = WINED3D_EVENT_QUERY_WAITING;
                break;

            case GL_WAIT_FAILED:
            default:
                ERR("glClientWaitSync returned %#x.\n", gl_ret);
                ret = WINED3D_EVENT_QUERY_ERROR;
        }
    }
    else if (gl_info->supported[APPLE_FENCE])
    {
        fence_result = GL_EXTCALL(glTestFenceAPPLE(query->object.id));
        checkGLcall("glTestFenceAPPLE");
        if (fence_result) ret = WINED3D_EVENT_QUERY_OK;
        else ret = WINED3D_EVENT_QUERY_WAITING;
    }
    else if (gl_info->supported[NV_FENCE])
    {
        fence_result = GL_EXTCALL(glTestFenceNV(query->object.id));
        checkGLcall("glTestFenceNV");
        if (fence_result) ret = WINED3D_EVENT_QUERY_OK;
        else ret = WINED3D_EVENT_QUERY_WAITING;
    }
    else
    {
        ERR("Event query created despite lack of GL support\n");
        ret = WINED3D_EVENT_QUERY_ERROR;
    }

    context_release(context);
    return ret;
}

enum wined3d_event_query_result wined3d_event_query_finish(const struct wined3d_event_query *query,
        const struct wined3d_device *device)
{
    struct wined3d_context *context;
    const struct wined3d_gl_info *gl_info;
    enum wined3d_event_query_result ret;

    TRACE("(%p)\n", query);

    if (!query->context)
    {
        TRACE("Query not started\n");
        return WINED3D_EVENT_QUERY_NOT_STARTED;
    }
    gl_info = query->context->gl_info;

    if (query->context->tid != GetCurrentThreadId() && !gl_info->supported[ARB_SYNC])
    {
        /* A glFinish does not reliably wait for draws in other contexts. The caller has
         * to find its own way to cope with the thread switch
         */
        WARN("Event query finished from wrong thread\n");
        return WINED3D_EVENT_QUERY_WRONG_THREAD;
    }

    context = context_acquire(device, query->context->current_rt);

    if (gl_info->supported[ARB_SYNC])
    {
        /* Apple seems to be into arbitrary limits, and timeouts larger than
         * 0xfffffffffffffbff immediately return GL_TIMEOUT_EXPIRED. We don't
         * really care and can live with waiting a few μs less. (OS X 10.7.4). */
        GLenum gl_ret = GL_EXTCALL(glClientWaitSync(query->object.sync, GL_SYNC_FLUSH_COMMANDS_BIT, ~(GLuint64)0xffff));
        checkGLcall("glClientWaitSync");

        switch (gl_ret)
        {
            case GL_ALREADY_SIGNALED:
            case GL_CONDITION_SATISFIED:
                ret = WINED3D_EVENT_QUERY_OK;
                break;

                /* We don't expect a timeout for a ~584 year wait */
            default:
                ERR("glClientWaitSync returned %#x.\n", gl_ret);
                ret = WINED3D_EVENT_QUERY_ERROR;
        }
    }
    else if (context->gl_info->supported[APPLE_FENCE])
    {
        GL_EXTCALL(glFinishFenceAPPLE(query->object.id));
        checkGLcall("glFinishFenceAPPLE");
        ret = WINED3D_EVENT_QUERY_OK;
    }
    else if (context->gl_info->supported[NV_FENCE])
    {
        GL_EXTCALL(glFinishFenceNV(query->object.id));
        checkGLcall("glFinishFenceNV");
        ret = WINED3D_EVENT_QUERY_OK;
    }
    else
    {
        ERR("Event query created without GL support\n");
        ret = WINED3D_EVENT_QUERY_ERROR;
    }

    context_release(context);
    return ret;
}

void wined3d_event_query_issue(struct wined3d_event_query *query, const struct wined3d_device *device)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    if (query->context)
    {
        if (!query->context->gl_info->supported[ARB_SYNC] && query->context->tid != GetCurrentThreadId())
        {
            context_free_event_query(query);
            context = context_acquire(device, NULL);
            context_alloc_event_query(context, query);
        }
        else
        {
            context = context_acquire(device, query->context->current_rt);
        }
    }
    else
    {
        context = context_acquire(device, NULL);
        context_alloc_event_query(context, query);
    }

    gl_info = context->gl_info;

    if (gl_info->supported[ARB_SYNC])
    {
        if (query->object.sync) GL_EXTCALL(glDeleteSync(query->object.sync));
        checkGLcall("glDeleteSync");
        query->object.sync = GL_EXTCALL(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
        checkGLcall("glFenceSync");
    }
    else if (gl_info->supported[APPLE_FENCE])
    {
        GL_EXTCALL(glSetFenceAPPLE(query->object.id));
        checkGLcall("glSetFenceAPPLE");
    }
    else if (gl_info->supported[NV_FENCE])
    {
        GL_EXTCALL(glSetFenceNV(query->object.id, GL_ALL_COMPLETED_NV));
        checkGLcall("glSetFenceNV");
    }

    context_release(context);
}

ULONG CDECL wined3d_query_incref(struct wined3d_query *query)
{
    ULONG refcount = InterlockedIncrement(&query->ref);

    TRACE("%p increasing refcount to %u.\n", query, refcount);

    return refcount;
}

#if defined(STAGING_CSMT)
void wined3d_query_destroy(struct wined3d_query *query)
{
    /* Queries are specific to the GL context that created them. Not
     * deleting the query will obviously leak it, but that's still better
     * than potentially deleting a different query with the same id in this
     * context, and (still) leaking the actual query. */
    if (query->type == WINED3D_QUERY_TYPE_EVENT)
    {
        struct wined3d_event_query *event_query = query->extendedData;
        if (event_query) wined3d_event_query_destroy(event_query);
    }
    else if (query->type == WINED3D_QUERY_TYPE_OCCLUSION)
    {
        struct wined3d_occlusion_query *oq = query->extendedData;

        if (oq->context) context_free_occlusion_query(oq);
        HeapFree(GetProcessHeap(), 0, query->extendedData);
    }
    else if (query->type == WINED3D_QUERY_TYPE_TIMESTAMP)
    {
        struct wined3d_timestamp_query *tq = query->extendedData;

        if (tq->context)
            context_free_timestamp_query(tq);
        HeapFree(GetProcessHeap(), 0, query->extendedData);
    }

    HeapFree(GetProcessHeap(), 0, query);
}

#endif /* STAGING_CSMT */
ULONG CDECL wined3d_query_decref(struct wined3d_query *query)
{
    ULONG refcount = InterlockedDecrement(&query->ref);

    TRACE("%p decreasing refcount to %u.\n", query, refcount);

    if (!refcount)
#if defined(STAGING_CSMT)
        wined3d_cs_emit_query_destroy(query->device->cs, query);
#else  /* STAGING_CSMT */
    {
        /* Queries are specific to the GL context that created them. Not
         * deleting the query will obviously leak it, but that's still better
         * than potentially deleting a different query with the same id in this
         * context, and (still) leaking the actual query. */
        if (query->type == WINED3D_QUERY_TYPE_EVENT)
        {
            struct wined3d_event_query *event_query = query->extendedData;
            if (event_query) wined3d_event_query_destroy(event_query);
        }
        else if (query->type == WINED3D_QUERY_TYPE_OCCLUSION)
        {
            struct wined3d_occlusion_query *oq = query->extendedData;

            if (oq->context) context_free_occlusion_query(oq);
            HeapFree(GetProcessHeap(), 0, query->extendedData);
        }
        else if (query->type == WINED3D_QUERY_TYPE_TIMESTAMP)
        {
            struct wined3d_timestamp_query *tq = query->extendedData;

            if (tq->context)
                context_free_timestamp_query(tq);
            HeapFree(GetProcessHeap(), 0, query->extendedData);
        }

        HeapFree(GetProcessHeap(), 0, query);
    }
#endif /* STAGING_CSMT */

    return refcount;
}

HRESULT CDECL wined3d_query_get_data(struct wined3d_query *query,
        void *data, UINT data_size, DWORD flags)
{
    TRACE("query %p, data %p, data_size %u, flags %#x.\n",
            query, data, data_size, flags);

    return query->query_ops->query_get_data(query, data, data_size, flags);
}

UINT CDECL wined3d_query_get_data_size(const struct wined3d_query *query)
{
    TRACE("query %p.\n", query);

    return query->data_size;
}

HRESULT CDECL wined3d_query_issue(struct wined3d_query *query, DWORD flags)
{
    TRACE("query %p, flags %#x.\n", query, flags);

#if defined(STAGING_CSMT)
    if (flags & WINED3DISSUE_END)
        query->counter_main++;

    wined3d_cs_emit_query_issue(query->device->cs, query, flags);

    if (flags & WINED3DISSUE_BEGIN)
        query->state = QUERY_BUILDING;
    else
        query->state = QUERY_SIGNALLED;

    return WINED3D_OK;
#else  /* STAGING_CSMT */
    return query->query_ops->query_issue(query, flags);
#endif /* STAGING_CSMT */
}

static void fill_query_data(void *out, unsigned int out_size, const void *result, unsigned int result_size)
{
    memcpy(out, result, min(out_size, result_size));
}

static HRESULT wined3d_occlusion_query_ops_get_data(struct wined3d_query *query,
        void *data, DWORD size, DWORD flags)
{
#if defined(STAGING_CSMT)
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_occlusion_query *oq = query->extendedData;
    GLuint samples;
#else  /* STAGING_CSMT */
    struct wined3d_occlusion_query *oq = query->extendedData;
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_context *context;
    GLuint available;
    GLuint samples;
    HRESULT res;

    TRACE("query %p, data %p, size %#x, flags %#x.\n", query, data, size, flags);

    if (!oq->context)
        query->state = QUERY_CREATED;
#endif /* STAGING_CSMT */

    if (query->state == QUERY_CREATED)
    {
        /* D3D allows GetData on a new query, OpenGL doesn't. So just invent the data ourselves */
        TRACE("Query wasn't yet started, returning S_OK\n");
        samples = 0;
        fill_query_data(data, size, &samples, sizeof(samples));
        return S_OK;
    }

#if defined(STAGING_CSMT)
    TRACE("(%p) : type D3DQUERY_OCCLUSION, data %p, size %#x, flags %#x.\n", query, data, size, flags);

#endif /* STAGING_CSMT */
    if (query->state == QUERY_BUILDING)
    {
        /* Msdn says this returns an error, but our tests show that S_FALSE is returned */
        TRACE("Query is building, returning S_FALSE\n");
        return S_FALSE;
    }

    if (!gl_info->supported[ARB_OCCLUSION_QUERY])
    {
        WARN("%p Occlusion queries not supported. Returning 1.\n", query);
        samples = 1;
        fill_query_data(data, size, &samples, sizeof(samples));
        return S_OK;
    }

#if defined(STAGING_CSMT)
    if (!wined3d_settings.cs_multithreaded)
    {
        if (!query->query_ops->query_poll(query))
            return S_FALSE;
    }
    else if (query->counter_main != query->counter_retrieved)
    {
        return S_FALSE;
    }

    if (data)
        fill_query_data(data, size, &oq->samples, sizeof(oq->samples));

    return S_OK;
}

static BOOL wined3d_occlusion_query_ops_poll(struct wined3d_query *query)
{
    struct wined3d_occlusion_query *oq = query->extendedData;
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_context *context;
    GLuint available;
    GLuint samples;
    BOOL ret;

    if (oq->context->tid != GetCurrentThreadId())
    {
        FIXME("%p Wrong thread, returning 1.\n", query);
        oq->samples = 1;
        return TRUE;
#else  /* STAGING_CSMT */
    if (oq->context->tid != GetCurrentThreadId())
    {
        FIXME("%p Wrong thread, returning 1.\n", query);
        samples = 1;
        fill_query_data(data, size, &samples, sizeof(samples));
        return S_OK;
#endif /* STAGING_CSMT */
    }

    context = context_acquire(query->device, oq->context->current_rt);

    GL_EXTCALL(glGetQueryObjectuiv(oq->id, GL_QUERY_RESULT_AVAILABLE, &available));
    checkGLcall("glGetQueryObjectuiv(GL_QUERY_RESULT_AVAILABLE)");
    TRACE("available %#x.\n", available);

    if (available)
    {
#if defined(STAGING_CSMT)
        GL_EXTCALL(glGetQueryObjectuiv(oq->id, GL_QUERY_RESULT, &samples));
        checkGLcall("glGetQueryObjectuiv(GL_QUERY_RESULT)");
        TRACE("Returning %d samples.\n", samples);
        oq->samples = samples;
        ret = TRUE;
    }
    else
    {
        ret = FALSE;
    }

    context_release(context);

    return ret;
}

static BOOL wined3d_event_query_ops_poll(struct wined3d_query *query)
{
    struct wined3d_event_query *event_query = query->extendedData;
    enum wined3d_event_query_result ret;

    ret = wined3d_event_query_test(event_query, query->device);
    switch(ret)
    {
        case WINED3D_EVENT_QUERY_OK:
        case WINED3D_EVENT_QUERY_NOT_STARTED:
            return TRUE;

        case WINED3D_EVENT_QUERY_WAITING:
            return FALSE;

        case WINED3D_EVENT_QUERY_WRONG_THREAD:
            FIXME("(%p) Wrong thread, reporting GPU idle.\n", query);
            return TRUE;

        case WINED3D_EVENT_QUERY_ERROR:
            ERR("The GL event query failed, returning D3DERR_INVALIDCALL\n");
            return TRUE;

        default:
            ERR("Unexpected wined3d_event_query_test result %u\n", ret);
            return TRUE;
    }
}

static HRESULT wined3d_event_query_ops_get_data(struct wined3d_query *query,
        void *pData, DWORD dwSize, DWORD flags)
{
    struct wined3d_event_query *event_query = query->extendedData;
    BOOL *data = pData;
    enum wined3d_event_query_result ret;

    TRACE("query %p, pData %p, dwSize %#x, flags %#x.\n", query, pData, dwSize, flags);

    if (!pData || !dwSize) return S_OK;
    if (!event_query)
    {
        WARN("Event query not supported by GL, reporting GPU idle.\n");
        *data = TRUE;
        return S_OK;
    }

    if (!wined3d_settings.cs_multithreaded)
        ret = query->query_ops->query_poll(query);
    else if (query->counter_main != query->counter_retrieved)
        ret = FALSE;
    else
        ret = TRUE;

    if (data)
        fill_query_data(data, dwSize, &ret, sizeof(ret));
#else  /* STAGING_CSMT */
        if (size)
        {
            GL_EXTCALL(glGetQueryObjectuiv(oq->id, GL_QUERY_RESULT, &samples));
            checkGLcall("glGetQueryObjectuiv(GL_QUERY_RESULT)");
            TRACE("Returning %d samples.\n", samples);
            fill_query_data(data, size, &samples, sizeof(samples));
        }
        res = S_OK;
    }
    else
    {
        res = S_FALSE;
    }

    context_release(context);

    return res;
}

static HRESULT wined3d_event_query_ops_get_data(struct wined3d_query *query,
        void *data, DWORD size, DWORD flags)
{
    struct wined3d_event_query *event_query = query->extendedData;
    BOOL signaled;
    enum wined3d_event_query_result ret;

    TRACE("query %p, data %p, size %#x, flags %#x.\n", query, data, size, flags);

    if (!data || !size) return S_OK;
    if (!event_query)
    {
        WARN("Event query not supported by GL, reporting GPU idle.\n");
        signaled = TRUE;
        fill_query_data(data, size, &signaled, sizeof(signaled));
        return S_OK;
    }

    ret = wined3d_event_query_test(event_query, query->device);
    switch(ret)
    {
        case WINED3D_EVENT_QUERY_OK:
        case WINED3D_EVENT_QUERY_NOT_STARTED:
            signaled = TRUE;
            fill_query_data(data, size, &signaled, sizeof(signaled));
            break;

        case WINED3D_EVENT_QUERY_WAITING:
            signaled = FALSE;
            fill_query_data(data, size, &signaled, sizeof(signaled));
            break;

        case WINED3D_EVENT_QUERY_WRONG_THREAD:
            FIXME("(%p) Wrong thread, reporting GPU idle.\n", query);
            signaled = TRUE;
            fill_query_data(data, size, &signaled, sizeof(signaled));
            break;

        case WINED3D_EVENT_QUERY_ERROR:
            ERR("The GL event query failed, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
    }
#endif /* STAGING_CSMT */

    return S_OK;
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

#if defined(STAGING_CSMT)
static BOOL wined3d_event_query_ops_issue(struct wined3d_query *query, DWORD flags)
#else  /* STAGING_CSMT */
static HRESULT wined3d_event_query_ops_issue(struct wined3d_query *query, DWORD flags)
#endif /* STAGING_CSMT */
{
    TRACE("query %p, flags %#x.\n", query, flags);

    TRACE("(%p) : flags %#x, type D3DQUERY_EVENT\n", query, flags);
    if (flags & WINED3DISSUE_END)
    {
        struct wined3d_event_query *event_query = query->extendedData;

        /* Faked event query support */
#if defined(STAGING_CSMT)
        if (!event_query) return FALSE;

        wined3d_event_query_issue(event_query, query->device);
        return TRUE;
    }
    else if (flags & WINED3DISSUE_BEGIN)
    {
        /* Started implicitly at device creation */
        ERR("Event query issued with START flag - what to do?\n");
    }
    return FALSE;
}

static BOOL wined3d_occlusion_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    BOOL poll = FALSE;
#else  /* STAGING_CSMT */
        if (!event_query) return WINED3D_OK;

        wined3d_event_query_issue(event_query, query->device);
    }
    else if (flags & WINED3DISSUE_BEGIN)
    {
        /* Started implicitly at device creation */
        ERR("Event query issued with START flag - what to do?\n");
    }

    if (flags & WINED3DISSUE_BEGIN)
        query->state = QUERY_BUILDING;
    else
        query->state = QUERY_SIGNALLED;

    return WINED3D_OK;
}

static HRESULT wined3d_occlusion_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
#endif /* STAGING_CSMT */

    TRACE("query %p, flags %#x.\n", query, flags);

    if (gl_info->supported[ARB_OCCLUSION_QUERY])
    {
        struct wined3d_occlusion_query *oq = query->extendedData;
        struct wined3d_context *context;

        /* This is allowed according to msdn and our tests. Reset the query and restart */
        if (flags & WINED3DISSUE_BEGIN)
        {
#if defined(STAGING_CSMT)
            if (oq->started)
#else  /* STAGING_CSMT */
            if (query->state == QUERY_BUILDING)
#endif /* STAGING_CSMT */
            {
                if (oq->context->tid != GetCurrentThreadId())
                {
                    FIXME("Wrong thread, can't restart query.\n");

                    context_free_occlusion_query(oq);
                    context = context_acquire(query->device, NULL);
                    context_alloc_occlusion_query(context, oq);
                }
                else
                {
                    context = context_acquire(query->device, oq->context->current_rt);

                    GL_EXTCALL(glEndQuery(GL_SAMPLES_PASSED));
                    checkGLcall("glEndQuery()");
                }
            }
            else
            {
                if (oq->context) context_free_occlusion_query(oq);
                context = context_acquire(query->device, NULL);
                context_alloc_occlusion_query(context, oq);
            }

            GL_EXTCALL(glBeginQuery(GL_SAMPLES_PASSED, oq->id));
            checkGLcall("glBeginQuery()");

            context_release(context);
#if defined(STAGING_CSMT)
            oq->started = TRUE;
#endif /* STAGING_CSMT */
        }
        if (flags & WINED3DISSUE_END)
        {
            /* Msdn says _END on a non-building occlusion query returns an error, but
             * our tests show that it returns OK. But OpenGL doesn't like it, so avoid
             * generating an error
             */
#if defined(STAGING_CSMT)
            if (oq->started)
#else  /* STAGING_CSMT */
            if (query->state == QUERY_BUILDING)
#endif /* STAGING_CSMT */
            {
                if (oq->context->tid != GetCurrentThreadId())
                {
                    FIXME("Wrong thread, can't end query.\n");
                }
                else
                {
                    context = context_acquire(query->device, oq->context->current_rt);

                    GL_EXTCALL(glEndQuery(GL_SAMPLES_PASSED));
                    checkGLcall("glEndQuery()");

                    context_release(context);
#if defined(STAGING_CSMT)
                    poll = TRUE;
                }
            }
            oq->started = FALSE;
#else  /* STAGING_CSMT */
                }
            }
#endif /* STAGING_CSMT */
        }
    }
    else
    {
        FIXME("%p Occlusion queries not supported.\n", query);
    }

#if defined(STAGING_CSMT)
    return poll;
}

static HRESULT wined3d_timestamp_query_ops_get_data(struct wined3d_query *query,
        void *data, DWORD size, DWORD flags)
{
    struct wined3d_timestamp_query *tq = query->extendedData;

    TRACE("(%p) : type D3DQUERY_TIMESTAMP, data %p, size %#x, flags %#x.\n", query, data, size, flags);

    if (query->state == QUERY_CREATED)
    {
        UINT64 zero = 0;
        /* D3D allows GetData on a new query, OpenGL doesn't. So just invent the data ourselves */
        TRACE("Query wasn't yet started, returning S_OK.\n");
        if (data)
            fill_query_data(data, size, &zero, sizeof(zero));
        return S_OK;
    }

    if (!wined3d_settings.cs_multithreaded)
    {
        if (!query->query_ops->query_poll(query))
            return S_FALSE;
    }
    else if (query->counter_main != query->counter_retrieved)
    {
        return S_FALSE;
    }

    if (data)
        fill_query_data(data, size, &tq->timestamp, sizeof(tq->timestamp));

    return S_OK;
}

static BOOL wined3d_timestamp_query_ops_poll(struct wined3d_query *query)
{
    struct wined3d_timestamp_query *tq = query->extendedData;
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_context *context;
    GLuint available;
    GLuint64 timestamp;
    BOOL ret;

    if (!gl_info->supported[ARB_TIMER_QUERY])
    {
        TRACE("Faking timestamp.\n");
        QueryPerformanceCounter((LARGE_INTEGER *)&tq->timestamp);
        return TRUE;
    }

    if (tq->context->tid != GetCurrentThreadId())
    {
        FIXME("%p Wrong thread, returning 1.\n", query);
        tq->timestamp = 1;
        return TRUE;
#else  /* STAGING_CSMT */
    if (flags & WINED3DISSUE_BEGIN)
        query->state = QUERY_BUILDING;
    else
        query->state = QUERY_SIGNALLED;

    return WINED3D_OK; /* can be WINED3DERR_INVALIDCALL.    */
}

static HRESULT wined3d_timestamp_query_ops_get_data(struct wined3d_query *query,
        void *data, DWORD size, DWORD flags)
{
    struct wined3d_timestamp_query *tq = query->extendedData;
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_context *context;
    GLuint available;
    GLuint64 timestamp;
    HRESULT res;

    TRACE("query %p, data %p, size %#x, flags %#x.\n", query, data, size, flags);

    if (!tq->context)
        query->state = QUERY_CREATED;

    if (query->state == QUERY_CREATED)
    {
        /* D3D allows GetData on a new query, OpenGL doesn't. So just invent the data ourselves. */
        TRACE("Query wasn't yet started, returning S_OK.\n");
        timestamp = 0;
        fill_query_data(data, size, &timestamp, sizeof(timestamp));
        return S_OK;
    }

    if (tq->context->tid != GetCurrentThreadId())
    {
        FIXME("%p Wrong thread, returning 1.\n", query);
        timestamp = 1;
        fill_query_data(data, size, &timestamp, sizeof(timestamp));
        return S_OK;
#endif /* STAGING_CSMT */
    }

    context = context_acquire(query->device, tq->context->current_rt);

    GL_EXTCALL(glGetQueryObjectuiv(tq->id, GL_QUERY_RESULT_AVAILABLE, &available));
    checkGLcall("glGetQueryObjectuiv(GL_QUERY_RESULT_AVAILABLE)");
    TRACE("available %#x.\n", available);

    if (available)
    {
#if defined(STAGING_CSMT)
        GL_EXTCALL(glGetQueryObjectui64v(tq->id, GL_QUERY_RESULT, &timestamp));
        checkGLcall("glGetQueryObjectui64v(GL_QUERY_RESULT)");
        TRACE("Returning timestamp %s.\n", wine_dbgstr_longlong(timestamp));
        tq->timestamp = timestamp;
        ret = TRUE;
    }
    else
    {
        ret = FALSE;
    }

    context_release(context);

    return ret;
}

static BOOL wined3d_timestamp_query_ops_issue(struct wined3d_query *query, DWORD flags)
#else  /* STAGING_CSMT */
        if (size)
        {
            GL_EXTCALL(glGetQueryObjectui64v(tq->id, GL_QUERY_RESULT, &timestamp));
            checkGLcall("glGetQueryObjectui64v(GL_QUERY_RESULT)");
            TRACE("Returning timestamp %s.\n", wine_dbgstr_longlong(timestamp));
            fill_query_data(data, size, &timestamp, sizeof(timestamp));
        }
        res = S_OK;
    }
    else
    {
        res = S_FALSE;
    }

    context_release(context);

    return res;
}

static HRESULT wined3d_timestamp_query_ops_issue(struct wined3d_query *query, DWORD flags)
#endif /* STAGING_CSMT */
{
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

    TRACE("query %p, flags %#x.\n", query, flags);

    if (gl_info->supported[ARB_TIMER_QUERY])
    {
        struct wined3d_timestamp_query *tq = query->extendedData;
        struct wined3d_context *context;

        if (flags & WINED3DISSUE_BEGIN)
        {
            WARN("Ignoring WINED3DISSUE_BEGIN with a TIMESTAMP query.\n");
        }
        if (flags & WINED3DISSUE_END)
        {
            if (tq->context)
                context_free_timestamp_query(tq);
            context = context_acquire(query->device, NULL);
            context_alloc_timestamp_query(context, tq);
            GL_EXTCALL(glQueryCounter(tq->id, GL_TIMESTAMP));
            checkGLcall("glQueryCounter()");
            context_release(context);
        }
    }
    else
    {
        ERR("Timestamp queries not supported.\n");
    }

    if (flags & WINED3DISSUE_END)
#if defined(STAGING_CSMT)
        return TRUE;
    return FALSE;
}

static HRESULT wined3d_timestamp_disjoint_query_ops_get_data(struct wined3d_query *query,
        void *data, DWORD size, DWORD flags)
{
    TRACE("query %p, data %p, size %#x, flags %#x.\n", query, data, size, flags);
#else  /* STAGING_CSMT */
        query->state = QUERY_SIGNALLED;

    return WINED3D_OK;
}

static HRESULT wined3d_timestamp_disjoint_query_ops_get_data(struct wined3d_query *query,
        void *data, DWORD size, DWORD flags)
{
    TRACE("query %p, data %p, size %#x, flags %#x.\n", query, data, size, flags);

#endif /* STAGING_CSMT */
    if (query->type == WINED3D_QUERY_TYPE_TIMESTAMP_DISJOINT)
    {
        static const struct wined3d_query_data_timestamp_disjoint disjoint_data = {1000 * 1000 * 1000, FALSE};

        if (query->state == QUERY_BUILDING)
        {
            TRACE("Query is building, returning S_FALSE.\n");
            return S_FALSE;
        }

        fill_query_data(data, size, &disjoint_data, sizeof(disjoint_data));
    }
    else
    {
        static const UINT64 freq = 1000 * 1000 * 1000;

        fill_query_data(data, size, &freq, sizeof(freq));
    }
    return S_OK;
}

#if defined(STAGING_CSMT)
static BOOL wined3d_timestamp_disjoint_query_ops_poll(struct wined3d_query *query)
{
    return TRUE;
}

static BOOL wined3d_timestamp_disjoint_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    TRACE("query %p, flags %#x.\n", query, flags);
    return FALSE;
}

static const struct wined3d_query_ops event_query_ops =
{
    wined3d_event_query_ops_get_data,
    wined3d_event_query_ops_poll,
    wined3d_event_query_ops_issue,
};

static const struct wined3d_query_ops occlusion_query_ops =
{
    wined3d_occlusion_query_ops_get_data,
    wined3d_occlusion_query_ops_poll,
    wined3d_occlusion_query_ops_issue,
};

static const struct wined3d_query_ops timestamp_query_ops =
{
    wined3d_timestamp_query_ops_get_data,
    wined3d_timestamp_query_ops_poll,
    wined3d_timestamp_query_ops_issue,
};

static const struct wined3d_query_ops timestamp_disjoint_query_ops =
{
    wined3d_timestamp_disjoint_query_ops_get_data,
    wined3d_timestamp_disjoint_query_ops_poll,
#else  /* STAGING_CSMT */
static HRESULT wined3d_timestamp_disjoint_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    TRACE("query %p, flags %#x.\n", query, flags);

    if (flags & WINED3DISSUE_BEGIN)
        query->state = QUERY_BUILDING;
    if (flags & WINED3DISSUE_END)
        query->state = QUERY_SIGNALLED;

    return WINED3D_OK;
}

static const struct wined3d_query_ops event_query_ops =
{
    wined3d_event_query_ops_get_data,
    wined3d_event_query_ops_issue,
};

static const struct wined3d_query_ops occlusion_query_ops =
{
    wined3d_occlusion_query_ops_get_data,
    wined3d_occlusion_query_ops_issue,
};

static const struct wined3d_query_ops timestamp_query_ops =
{
    wined3d_timestamp_query_ops_get_data,
    wined3d_timestamp_query_ops_issue,
};

static const struct wined3d_query_ops timestamp_disjoint_query_ops =
{
    wined3d_timestamp_disjoint_query_ops_get_data,
#endif /* STAGING_CSMT */
    wined3d_timestamp_disjoint_query_ops_issue,
};

static HRESULT query_init(struct wined3d_query *query, struct wined3d_device *device,
        enum wined3d_query_type type, void *parent)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

    query->parent = parent;

    switch (type)
    {
        case WINED3D_QUERY_TYPE_OCCLUSION:
            TRACE("Occlusion query.\n");
            if (!gl_info->supported[ARB_OCCLUSION_QUERY])
            {
                WARN("Unsupported in local OpenGL implementation: ARB_OCCLUSION_QUERY.\n");
                return WINED3DERR_NOTAVAILABLE;
            }
            query->query_ops = &occlusion_query_ops;
            query->data_size = sizeof(DWORD);
#if defined(STAGING_CSMT)
            query->extendedData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                    sizeof(struct wined3d_occlusion_query));
            if (!query->extendedData)
            {
                ERR("Failed to allocate occlusion query extended data.\n");
                return E_OUTOFMEMORY;
            }
#else  /* STAGING_CSMT */
            query->extendedData = HeapAlloc(GetProcessHeap(), 0, sizeof(struct wined3d_occlusion_query));
            if (!query->extendedData)
            {
                ERR("Failed to allocate occlusion query extended data.\n");
                return E_OUTOFMEMORY;
            }
            ((struct wined3d_occlusion_query *)query->extendedData)->context = NULL;
#endif /* STAGING_CSMT */
            break;

        case WINED3D_QUERY_TYPE_EVENT:
            TRACE("Event query.\n");
            if (!wined3d_event_query_supported(gl_info))
            {
                /* Half-Life 2 needs this query. It does not render the main
                 * menu correctly otherwise. Pretend to support it, faking
                 * this query does not do much harm except potentially
                 * lowering performance. */
                FIXME("Event query: Unimplemented, but pretending to be supported.\n");
            }
            query->query_ops = &event_query_ops;
            query->data_size = sizeof(BOOL);
            query->extendedData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct wined3d_event_query));
            if (!query->extendedData)
            {
                ERR("Failed to allocate event query memory.\n");
                return E_OUTOFMEMORY;
            }
            break;

        case WINED3D_QUERY_TYPE_TIMESTAMP:
            TRACE("Timestamp query.\n");
            if (!gl_info->supported[ARB_TIMER_QUERY])
            {
                WARN("Unsupported in local OpenGL implementation: ARB_TIMER_QUERY.\n");
                return WINED3DERR_NOTAVAILABLE;
            }
            query->query_ops = &timestamp_query_ops;
            query->data_size = sizeof(UINT64);
            query->extendedData = HeapAlloc(GetProcessHeap(), 0, sizeof(struct wined3d_timestamp_query));
            if (!query->extendedData)
            {
                ERR("Failed to allocate timestamp query extended data.\n");
                return E_OUTOFMEMORY;
            }
            ((struct wined3d_timestamp_query *)query->extendedData)->context = NULL;
            break;

        case WINED3D_QUERY_TYPE_TIMESTAMP_DISJOINT:
        case WINED3D_QUERY_TYPE_TIMESTAMP_FREQ:
            TRACE("TIMESTAMP_DISJOINT query.\n");
            if (!gl_info->supported[ARB_TIMER_QUERY])
            {
                WARN("Unsupported in local OpenGL implementation: ARB_TIMER_QUERY.\n");
                return WINED3DERR_NOTAVAILABLE;
            }
            query->query_ops = &timestamp_disjoint_query_ops;
            query->data_size = type == WINED3D_QUERY_TYPE_TIMESTAMP_DISJOINT
                    ? sizeof(struct wined3d_query_data_timestamp_disjoint) : sizeof(UINT64);
            query->extendedData = NULL;
            break;

        case WINED3D_QUERY_TYPE_VCACHE:
        case WINED3D_QUERY_TYPE_RESOURCE_MANAGER:
        case WINED3D_QUERY_TYPE_VERTEX_STATS:
        case WINED3D_QUERY_TYPE_PIPELINE_TIMINGS:
        case WINED3D_QUERY_TYPE_INTERFACE_TIMINGS:
        case WINED3D_QUERY_TYPE_VERTEX_TIMINGS:
        case WINED3D_QUERY_TYPE_PIXEL_TIMINGS:
        case WINED3D_QUERY_TYPE_BANDWIDTH_TIMINGS:
        case WINED3D_QUERY_TYPE_CACHE_UTILIZATION:
        default:
            FIXME("Unhandled query type %#x.\n", type);
            return WINED3DERR_NOTAVAILABLE;
    }

    query->type = type;
    query->state = QUERY_CREATED;
    query->device = device;
    query->ref = 1;
#if defined(STAGING_CSMT)
    list_init(&query->poll_list_entry);
#endif /* STAGING_CSMT */

    return WINED3D_OK;
}

HRESULT CDECL wined3d_query_create(struct wined3d_device *device,
        enum wined3d_query_type type, void *parent, struct wined3d_query **query)
{
    struct wined3d_query *object;
    HRESULT hr;

    TRACE("device %p, type %#x, query %p.\n", device, type, query);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = query_init(object, device, type, parent)))
    {
        WARN("Failed to initialize query, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created query %p.\n", object);
    *query = object;

    return WINED3D_OK;
}
