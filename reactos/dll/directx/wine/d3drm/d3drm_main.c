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

#include "d3drm_private.h"

/***********************************************************************
 *		DllMain  (D3DRM.@)
 */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( inst );
        break;
    }
    return TRUE;
}

void d3drm_object_init(struct d3drm_object *object)
{
    object->ref = 1;
    object->appdata = 0;
    list_init(&object->destroy_callbacks);
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

    callback = HeapAlloc(GetProcessHeap(), 0, sizeof(*callback));
    if (!callback)
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
            HeapFree(GetProcessHeap(), 0, callback);
            break;
        }
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
        HeapFree(GetProcessHeap(), 0, callback);
    }
}
