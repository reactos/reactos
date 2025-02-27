/*
 * Copyright 2004 Ivan Leo Puoti
 * Copyright 2010 Christian Costa
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

#include "initguid.h"
#include "d3drm_private.h"

void d3drm_object_init(struct d3drm_object *object, const char *classname)
{
    object->ref = 1;
    object->appdata = 0;
    list_init(&object->destroy_callbacks);
    object->classname = classname;
    object->name = NULL;
}

struct destroy_callback
{
    struct list entry;
    D3DRMOBJECTCALLBACK cb;
    void *ctx;
};

HRESULT d3drm_object_add_destroy_callback(struct d3drm_object *object, D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct destroy_callback *callback;

    if (!cb)
        return D3DRMERR_BADVALUE;

    if (!(callback = malloc(sizeof(*callback))))
        return E_OUTOFMEMORY;

    callback->cb = cb;
    callback->ctx = ctx;

    list_add_head(&object->destroy_callbacks, &callback->entry);
    return D3DRM_OK;
}

HRESULT d3drm_object_delete_destroy_callback(struct d3drm_object *object, D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct destroy_callback *callback;

    if (!cb)
        return D3DRMERR_BADVALUE;

    LIST_FOR_EACH_ENTRY(callback, &object->destroy_callbacks, struct destroy_callback, entry)
    {
        if (callback->cb == cb && callback->ctx == ctx)
        {
            list_remove(&callback->entry);
            free(callback);
            break;
        }
    }

    return D3DRM_OK;
}

HRESULT d3drm_object_get_class_name(struct d3drm_object *object, DWORD *size, char *name)
{
    DWORD req_size;

    if (!size)
        return E_INVALIDARG;

    req_size = strlen(object->classname) + 1;
    if (name && *size < req_size)
        return E_INVALIDARG;

    *size = req_size;

    if (name)
        memcpy(name, object->classname, req_size);

    return D3DRM_OK;
}

HRESULT d3drm_object_get_name(struct d3drm_object *object, DWORD *size, char *name)
{
    DWORD req_size;

    if (!size)
        return E_INVALIDARG;

    req_size = object->name ? strlen(object->name) + 1 : 0;
    if (name && *size < req_size)
        return E_INVALIDARG;

    if (name)
    {
        if (object->name)
            memcpy(name, object->name, req_size);
        else if (*size)
            *name = 0;
    }

    *size = req_size;

    return D3DRM_OK;
}

HRESULT d3drm_object_set_name(struct d3drm_object *object, const char *name)
{
    DWORD req_size;

    free(object->name);
    object->name = NULL;

    if (name)
    {
        req_size = strlen(name) + 1;
        if (!(object->name = malloc(req_size)))
            return E_OUTOFMEMORY;
        memcpy(object->name, name, req_size);
    }

    return D3DRM_OK;
}

void d3drm_object_cleanup(IDirect3DRMObject *iface, struct d3drm_object *object)
{
    struct destroy_callback *callback, *callback2;

    LIST_FOR_EACH_ENTRY_SAFE(callback, callback2, &object->destroy_callbacks, struct destroy_callback, entry)
    {
        callback->cb(iface, callback->ctx);
        list_remove(&callback->entry);
        free(callback);
    }

    free(object->name);
    object->name = NULL;
}
