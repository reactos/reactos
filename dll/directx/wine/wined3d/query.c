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


#include <config.h>
#include <wine/port.h>
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

ULONG CDECL wined3d_query_decref(struct wined3d_query *query)
{
    ULONG refcount = InterlockedDecrement(&query->ref);

    TRACE("%p decreasing refcount to %u.\n", query, refcount);

    if (!refcount)
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

        HeapFree(GetProcessHeap(), 0, query);
    }

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

    return query->query_ops->query_issue(query, flags);
}

static HRESULT wined3d_occlusion_query_ops_get_data(struct wined3d_query *query,
        void *pData, DWORD dwSize, DWORD flags)
{
    struct wined3d_occlusion_query *oq = query->extendedData;
    struct wined3d_device *device = query->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_context *context;
    DWORD* data = pData;
    GLuint available;
    GLuint samples;
    HRESULT res;

    TRACE("(%p) : type D3DQUERY_OCCLUSION, pData %p, dwSize %#x, flags %#x.\n", query, pData, dwSize, flags);

    if (!oq->context)
        query->state = QUERY_CREATED;

    if (query->state == QUERY_CREATED)
    {
        /* D3D allows GetData on a new query, OpenGL doesn't. So just invent the data ourselves */
        TRACE("Query wasn't yet started, returning S_OK\n");
        if(data) *data = 0;
        return S_OK;
    }

    if (query->state == QUERY_BUILDING)
    {
        /* Msdn says this returns an error, but our tests show that S_FALSE is returned */
        TRACE("Query is building, returning S_FALSE\n");
        return S_FALSE;
    }

    if (!gl_info->supported[ARB_OCCLUSION_QUERY])
    {
        WARN("%p Occlusion queries not supported. Returning 1.\n", query);
        *data = 1;
        return S_OK;
    }

    if (oq->context->tid != GetCurrentThreadId())
    {
        FIXME("%p Wrong thread, returning 1.\n", query);
        *data = 1;
        return S_OK;
    }

    context = context_acquire(query->device, oq->context->current_rt);

    GL_EXTCALL(glGetQueryObjectuivARB(oq->id, GL_QUERY_RESULT_AVAILABLE_ARB, &available));
    checkGLcall("glGetQueryObjectuivARB(GL_QUERY_RESULT_AVAILABLE)");
    TRACE("available %#x.\n", available);

    if (available)
    {
        if (data)
        {
            GL_EXTCALL(glGetQueryObjectuivARB(oq->id, GL_QUERY_RESULT_ARB, &samples));
            checkGLcall("glGetQueryObjectuivARB(GL_QUERY_RESULT)");
            TRACE("Returning %d samples.\n", samples);
            *data = samples;
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

    ret = wined3d_event_query_test(event_query, query->device);
    switch(ret)
    {
        case WINED3D_EVENT_QUERY_OK:
        case WINED3D_EVENT_QUERY_NOT_STARTED:
            *data = TRUE;
            break;

        case WINED3D_EVENT_QUERY_WAITING:
            *data = FALSE;
            break;

        case WINED3D_EVENT_QUERY_WRONG_THREAD:
            FIXME("(%p) Wrong thread, reporting GPU idle.\n", query);
            *data = TRUE;
            break;

        case WINED3D_EVENT_QUERY_ERROR:
            ERR("The GL event query failed, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
    }

    return S_OK;
}

enum wined3d_query_type CDECL wined3d_query_get_type(const struct wined3d_query *query)
{
    TRACE("query %p.\n", query);

    return query->type;
}

static HRESULT wined3d_event_query_ops_issue(struct wined3d_query *query, DWORD flags)
{
    TRACE("query %p, flags %#x.\n", query, flags);

    TRACE("(%p) : flags %#x, type D3DQUERY_EVENT\n", query, flags);
    if (flags & WINED3DISSUE_END)
    {
        struct wined3d_event_query *event_query = query->extendedData;

        /* Faked event query support */
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

    TRACE("query %p, flags %#x.\n", query, flags);

    if (gl_info->supported[ARB_OCCLUSION_QUERY])
    {
        struct wined3d_occlusion_query *oq = query->extendedData;
        struct wined3d_context *context;

        /* This is allowed according to msdn and our tests. Reset the query and restart */
        if (flags & WINED3DISSUE_BEGIN)
        {
            if (query->state == QUERY_BUILDING)
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

                    GL_EXTCALL(glEndQueryARB(GL_SAMPLES_PASSED_ARB));
                    checkGLcall("glEndQuery()");
                }
            }
            else
            {
                if (oq->context) context_free_occlusion_query(oq);
                context = context_acquire(query->device, NULL);
                context_alloc_occlusion_query(context, oq);
            }

            GL_EXTCALL(glBeginQueryARB(GL_SAMPLES_PASSED_ARB, oq->id));
            checkGLcall("glBeginQuery()");

            context_release(context);
        }
        if (flags & WINED3DISSUE_END)
        {
            /* Msdn says _END on a non-building occlusion query returns an error, but
             * our tests show that it returns OK. But OpenGL doesn't like it, so avoid
             * generating an error
             */
            if (query->state == QUERY_BUILDING)
            {
                if (oq->context->tid != GetCurrentThreadId())
                {
                    FIXME("Wrong thread, can't end query.\n");
                }
                else
                {
                    context = context_acquire(query->device, oq->context->current_rt);

                    GL_EXTCALL(glEndQueryARB(GL_SAMPLES_PASSED_ARB));
                    checkGLcall("glEndQuery()");

                    context_release(context);
                }
            }
        }
    }
    else
    {
        FIXME("%p Occlusion queries not supported.\n", query);
    }

    if (flags & WINED3DISSUE_BEGIN)
        query->state = QUERY_BUILDING;
    else
        query->state = QUERY_SIGNALLED;

    return WINED3D_OK; /* can be WINED3DERR_INVALIDCALL.    */
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

static HRESULT query_init(struct wined3d_query *query, struct wined3d_device *device, enum wined3d_query_type type)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

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
            query->extendedData = HeapAlloc(GetProcessHeap(), 0, sizeof(struct wined3d_occlusion_query));
            if (!query->extendedData)
            {
                ERR("Failed to allocate occlusion query extended data.\n");
                return E_OUTOFMEMORY;
            }
            ((struct wined3d_occlusion_query *)query->extendedData)->context = NULL;
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

        case WINED3D_QUERY_TYPE_VCACHE:
        case WINED3D_QUERY_TYPE_RESOURCE_MANAGER:
        case WINED3D_QUERY_TYPE_VERTEX_STATS:
        case WINED3D_QUERY_TYPE_TIMESTAMP:
        case WINED3D_QUERY_TYPE_TIMESTAMP_DISJOINT:
        case WINED3D_QUERY_TYPE_TIMESTAMP_FREQ:
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

    return WINED3D_OK;
}

HRESULT CDECL wined3d_query_create(struct wined3d_device *device,
        enum wined3d_query_type type, struct wined3d_query **query)
{
    struct wined3d_query *object;
    HRESULT hr;

    TRACE("device %p, type %#x, query %p.\n", device, type, query);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = query_init(object, device, type);
    if (FAILED(hr))
    {
        WARN("Failed to initialize query, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created query %p.\n", object);
    *query = object;

    return WINED3D_OK;
}
