/*        DirectDraw Base Functions
 *
 * Copyright 1997-1999 Marcus Meissner
 * Copyright 1998 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2006 Stefan Dösinger
 * Copyright 2008 Denver Gingerich
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

#define DDRAW_INIT_GUID
#include "ddraw_private.h"
#include "rpcproxy.h"

#include "wine/exception.h"
#include "winreg.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static struct list global_ddraw_list = LIST_INIT(global_ddraw_list);

static HINSTANCE instance;

/* value of ForceRefreshRate */
DWORD force_refresh_rate = 0;

struct ddraw_handle_table global_handle_table;

/* Structure for converting DirectDrawEnumerateA to DirectDrawEnumerateExA */
struct callback_info
{
    LPDDENUMCALLBACKA callback;
    void *context;
};

/* Enumeration callback for converting DirectDrawEnumerateA to DirectDrawEnumerateExA */
static BOOL CALLBACK enum_callback(GUID *guid, char *description, char *driver_name,
                                      void *context, HMONITOR monitor)
{
    const struct callback_info *info = context;

    return info->callback(guid, description, driver_name, info->context);
}

static void ddraw_enumerate_secondary_devices(struct wined3d *wined3d, LPDDENUMCALLBACKEXA callback,
                                              void *context)
{
    struct wined3d_adapter_identifier adapter_id;
    struct wined3d_adapter *wined3d_adapter;
    struct wined3d_output_desc output_desc;
    struct wined3d_output *wined3d_output;
    unsigned int interface_count = 0;
    unsigned int adapter_idx = 0;
    unsigned int output_idx;
    BOOL cont_enum = TRUE;
    HRESULT hr;

    while (cont_enum && (wined3d_adapter = wined3d_get_adapter(wined3d, adapter_idx)))
    {
        char device_name[512] = "", description[512] = "";

        /* The Battle.net System Checker expects the GetAdapterIdentifier DeviceName to match the
         * Driver Name, so obtain the DeviceName and GUID from D3D. */
        memset(&adapter_id, 0x0, sizeof(adapter_id));
        adapter_id.description = description;
        adapter_id.description_size = sizeof(description);

        wined3d_mutex_lock();
        if (FAILED(hr = wined3d_adapter_get_identifier(wined3d_adapter, 0x0, &adapter_id)))
        {
            WARN("Failed to get adapter identifier, hr %#lx.\n", hr);
            wined3d_mutex_unlock();
            break;
        }
        wined3d_mutex_unlock();

        for (output_idx = 0; cont_enum && (wined3d_output = wined3d_adapter_get_output(
                wined3d_adapter, output_idx)); ++output_idx)
        {
            wined3d_mutex_lock();
            if (FAILED(hr = wined3d_output_get_desc(wined3d_output, &output_desc)))
            {
                WARN("Failed to get output description, hr %#lx.\n", hr);
                wined3d_mutex_unlock();
                break;
            }
            wined3d_mutex_unlock();

            TRACE("Interface %u: %s\n", interface_count++,
                    wine_dbgstr_guid(&adapter_id.device_identifier));
            WideCharToMultiByte(CP_ACP, 0, output_desc.device_name, -1, device_name,
                    sizeof(device_name), NULL, NULL);
            cont_enum = callback(&adapter_id.device_identifier, adapter_id.description,
                    device_name, context, output_desc.monitor);
        }

        if (FAILED(hr))
            break;

        ++adapter_idx;
    }
}

/* Handle table functions */
BOOL ddraw_handle_table_init(struct ddraw_handle_table *t, UINT initial_size)
{
    if (!(t->entries = calloc(initial_size, sizeof(*t->entries))))
    {
        ERR("Failed to allocate handle table memory.\n");
        return FALSE;
    }
    t->free_entries = NULL;
    t->table_size = initial_size;
    t->entry_count = 0;

    return TRUE;
}

void ddraw_handle_table_destroy(struct ddraw_handle_table *t)
{
    free(t->entries);
    memset(t, 0, sizeof(*t));
}

DWORD ddraw_allocate_handle(struct ddraw_handle_table *t, void *object, enum ddraw_handle_type type)
{
    struct ddraw_handle_entry *entry;

    if (!t)
        t = &global_handle_table;

    if (t->free_entries)
    {
        DWORD idx = t->free_entries - t->entries;
        /* Use a free handle */
        entry = t->free_entries;
        if (entry->type != DDRAW_HANDLE_FREE)
        {
            ERR("Handle %#lx (%p) is in the free list, but has type %#x.\n", idx, entry->object, entry->type);
            return DDRAW_INVALID_HANDLE;
        }
        t->free_entries = entry->object;
        entry->object = object;
        entry->type = type;

        return idx;
    }

    if (!(t->entry_count < t->table_size))
    {
        /* Grow the table */
        UINT new_size = t->table_size + (t->table_size >> 1);
        struct ddraw_handle_entry *new_entries;

        if (!(new_entries = realloc(t->entries, new_size * sizeof(*t->entries))))
        {
            ERR("Failed to grow the handle table.\n");
            return DDRAW_INVALID_HANDLE;
        }
        t->entries = new_entries;
        t->table_size = new_size;
    }

    entry = &t->entries[t->entry_count];
    entry->object = object;
    entry->type = type;

    return t->entry_count++;
}

void *ddraw_free_handle(struct ddraw_handle_table *t, DWORD handle, enum ddraw_handle_type type)
{
    struct ddraw_handle_entry *entry;
    void *object;

    if (!t)
        t = &global_handle_table;

    if (handle == DDRAW_INVALID_HANDLE || handle >= t->entry_count)
    {
        WARN("Invalid handle %#lx passed.\n", handle);
        return NULL;
    }

    entry = &t->entries[handle];
    if (entry->type != type)
    {
        WARN("Handle %#lx (%p) is not of type %#x.\n", handle, entry->object, type);
        return NULL;
    }

    object = entry->object;
    entry->object = t->free_entries;
    entry->type = DDRAW_HANDLE_FREE;
    t->free_entries = entry;

    return object;
}

void *ddraw_get_object(struct ddraw_handle_table *t, DWORD handle, enum ddraw_handle_type type)
{
    struct ddraw_handle_entry *entry;

    if (!t)
        t = &global_handle_table;

    if (handle == DDRAW_INVALID_HANDLE || handle >= t->entry_count)
    {
        WARN("Invalid handle %#lx passed.\n", handle);
        return NULL;
    }

    entry = &t->entries[handle];
    if (entry->type != type)
    {
        WARN("Handle %#lx (%p) is not of type %#x.\n", handle, entry->object, type);
        return NULL;
    }

    return entry->object;
}

HRESULT WINAPI GetSurfaceFromDC(HDC dc, IDirectDrawSurface4 **surface, HDC *device_dc)
{
    struct ddraw *ddraw;

    TRACE("dc %p, surface %p, device_dc %p.\n", dc, surface, device_dc);

    if (!surface)
        return E_INVALIDARG;

    if (!device_dc)
    {
        *surface = NULL;

        return E_INVALIDARG;
    }

    wined3d_mutex_lock();
    LIST_FOR_EACH_ENTRY(ddraw, &global_ddraw_list, struct ddraw, ddraw_list_entry)
    {
        if (FAILED(IDirectDraw4_GetSurfaceFromDC(&ddraw->IDirectDraw4_iface, dc, surface)))
            continue;

        *device_dc = NULL; /* FIXME */
        wined3d_mutex_unlock();
        return DD_OK;
    }
    wined3d_mutex_unlock();

    *surface = NULL;
    *device_dc = NULL;

    return DDERR_NOTFOUND;
}

/***********************************************************************
 *
 * Helper function for DirectDrawCreate and friends
 * Creates a new DDraw interface with the given REFIID
 *
 * Interfaces that can be created:
 *  IDirectDraw, IDirectDraw2, IDirectDraw4, IDirectDraw7
 *  IDirect3D, IDirect3D2, IDirect3D3, IDirect3D7. (Does Windows return
 *  IDirect3D interfaces?)
 *
 * Arguments:
 *  guid: ID of the requested driver, NULL for the default driver.
 *        The GUID can be queried with DirectDrawEnumerate(Ex)A/W
 *  DD: Used to return the pointer to the created object
 *  UnkOuter: For aggregation, which is unsupported. Must be NULL
 *  iid: requested version ID.
 *
 * Returns:
 *  DD_OK if the Interface was created successfully
 *  CLASS_E_NOAGGREGATION if UnkOuter is not NULL
 *  E_OUTOFMEMORY if some allocation failed
 *
 ***********************************************************************/
static HRESULT DDRAW_Create(const GUID *guid, void **out, IUnknown *outer_unknown, REFIID iid)
{
    enum wined3d_device_type device_type;
    struct ddraw *ddraw;
    DWORD flags = 0;
    HRESULT hr;

    TRACE("driver_guid %s, ddraw %p, outer_unknown %p, interface_iid %s.\n",
            debugstr_guid(guid), out, outer_unknown, debugstr_guid(iid));

    *out = NULL;

    if (guid == (GUID *) DDCREATE_EMULATIONONLY)
    {
        device_type = WINED3D_DEVICE_TYPE_REF;
    }
    else if (guid == (GUID *) DDCREATE_HARDWAREONLY)
    {
        device_type = WINED3D_DEVICE_TYPE_HAL;
    }
    else
    {
        device_type = WINED3D_DEVICE_TYPE_HAL;
    }

    /* DDraw doesn't support aggregation, according to msdn */
    if (outer_unknown != NULL)
        return CLASS_E_NOAGGREGATION;

    if (!IsEqualGUID(iid, &IID_IDirectDraw7))
        flags = WINED3D_LEGACY_FFP_LIGHTING;

    if (!(ddraw = calloc(1, sizeof(*ddraw))))
    {
        ERR("Out of memory when creating DirectDraw.\n");
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = ddraw_init(ddraw, flags, device_type)))
    {
        WARN("Failed to initialize ddraw object, hr %#lx.\n", hr);
        free(ddraw);
        return hr;
    }

    hr = IDirectDraw7_QueryInterface(&ddraw->IDirectDraw7_iface, iid, out);
    IDirectDraw7_Release(&ddraw->IDirectDraw7_iface);
    if (SUCCEEDED(hr))
        list_add_head(&global_ddraw_list, &ddraw->ddraw_list_entry);
    else
        WARN("Failed to query interface %s from ddraw object %p.\n", debugstr_guid(iid), ddraw);

    return hr;
}

/***********************************************************************
 * DirectDrawCreate (DDRAW.@)
 *
 * Creates legacy DirectDraw Interfaces. Can't create IDirectDraw7
 * interfaces in theory
 *
 * Arguments, return values: See DDRAW_Create
 *
 ***********************************************************************/
HRESULT WINAPI DECLSPEC_HOTPATCH DirectDrawCreate(GUID *driver_guid, IDirectDraw **ddraw, IUnknown *outer)
{
    HRESULT hr;

    TRACE("driver_guid %s, ddraw %p, outer %p.\n",
            debugstr_guid(driver_guid), ddraw, outer);

    wined3d_mutex_lock();
    hr = DDRAW_Create(driver_guid, (void **)ddraw, outer, &IID_IDirectDraw);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = IDirectDraw_Initialize(*ddraw, driver_guid)))
            IDirectDraw_Release(*ddraw);
    }

    return hr;
}

/***********************************************************************
 * DirectDrawCreateEx (DDRAW.@)
 *
 * Only creates new IDirectDraw7 interfaces, supposed to fail if legacy
 * interfaces are requested.
 *
 * Arguments, return values: See DDRAW_Create
 *
 ***********************************************************************/
HRESULT WINAPI DECLSPEC_HOTPATCH DirectDrawCreateEx(GUID *driver_guid,
        void **ddraw, REFIID interface_iid, IUnknown *outer)
{
    HRESULT hr;

    TRACE("driver_guid %s, ddraw %p, interface_iid %s, outer %p.\n",
            debugstr_guid(driver_guid), ddraw, debugstr_guid(interface_iid), outer);

    if (!IsEqualGUID(interface_iid, &IID_IDirectDraw7))
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    hr = DDRAW_Create(driver_guid, ddraw, outer, interface_iid);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        IDirectDraw7 *ddraw7 = *(IDirectDraw7 **)ddraw;
        hr = IDirectDraw7_Initialize(ddraw7, driver_guid);
        if (FAILED(hr))
            IDirectDraw7_Release(ddraw7);
    }

    return hr;
}

/***********************************************************************
 * DirectDrawEnumerateA (DDRAW.@)
 *
 * Enumerates legacy ddraw drivers, ascii version. We only have one
 * driver, which relays to WineD3D. If we were sufficiently cool,
 * we could offer various interfaces, which use a different default surface
 * implementation, but I think it's better to offer this choice in
 * winecfg, because some apps use the default driver, so we would need
 * a winecfg option anyway, and there shouldn't be 2 ways to set one setting
 *
 * Arguments:
 *  Callback: Callback function from the app
 *  Context: Argument to the call back.
 *
 * Returns:
 *  DD_OK on success
 *  E_INVALIDARG if the Callback caused a page fault
 *
 *
 ***********************************************************************/
HRESULT WINAPI DirectDrawEnumerateA(LPDDENUMCALLBACKA callback, void *context)
{
    struct callback_info info;

    TRACE("callback %p, context %p.\n", callback, context);

    info.callback = callback;
    info.context = context;
    return DirectDrawEnumerateExA(enum_callback, &info, 0x0);
}

/***********************************************************************
 * DirectDrawEnumerateExA (DDRAW.@)
 *
 * Enumerates DirectDraw7 drivers, ascii version. See
 * the comments above DirectDrawEnumerateA for more details.
 *
 * The Flag member is not supported right now.
 *
 ***********************************************************************/
HRESULT WINAPI DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA callback, void *context, DWORD flags)
{
    struct wined3d *wined3d;

    TRACE("callback %p, context %p, flags %#lx.\n", callback, context, flags);

    if (flags & ~(DDENUM_ATTACHEDSECONDARYDEVICES |
                  DDENUM_DETACHEDSECONDARYDEVICES |
                  DDENUM_NONDISPLAYDEVICES))
        return DDERR_INVALIDPARAMS;

    if (flags & ~DDENUM_ATTACHEDSECONDARYDEVICES)
        FIXME("flags %#lx not handled\n", flags & ~DDENUM_ATTACHEDSECONDARYDEVICES);

    TRACE("Enumerating ddraw interfaces\n");
    if (!(wined3d = wined3d_create(DDRAW_WINED3D_FLAGS)))
    {
        if (!(wined3d = wined3d_create(DDRAW_WINED3D_FLAGS | WINED3D_NO3D)))
        {
            WARN("Failed to create a wined3d object.\n");
            return E_FAIL;
        }

        WARN("Created a wined3d object without 3D support.\n");
    }

    __TRY
    {
        /* QuickTime expects the description "DirectDraw HAL" */
        static CHAR driver_desc[] = "DirectDraw HAL",
        driver_name[] = "display";
        BOOL cont_enum;

        TRACE("Default interface: DirectDraw HAL\n");
        cont_enum = callback(NULL, driver_desc, driver_name, context, 0);

        /* The Battle.net System Checker expects both a NULL device and a GUID-based device */
        if (cont_enum && (flags & DDENUM_ATTACHEDSECONDARYDEVICES))
            ddraw_enumerate_secondary_devices(wined3d, callback, context);
    }
    __EXCEPT_PAGE_FAULT
    {
        wined3d_decref(wined3d);
        return DDERR_INVALIDPARAMS;
    }
    __ENDTRY;

    wined3d_decref(wined3d);
    TRACE("End of enumeration\n");
    return DD_OK;
}

/***********************************************************************
 * DirectDrawEnumerateW (DDRAW.@)
 *
 * Enumerates legacy drivers, unicode version.
 * This function is not implemented on Windows.
 *
 ***********************************************************************/
HRESULT WINAPI DirectDrawEnumerateW(LPDDENUMCALLBACKW callback, void *context)
{
    TRACE("callback %p, context %p.\n", callback, context);

    if (!callback)
        return DDERR_INVALIDPARAMS;
    else
        return DDERR_UNSUPPORTED;
}

/***********************************************************************
 * DirectDrawEnumerateExW (DDRAW.@)
 *
 * Enumerates DirectDraw7 drivers, unicode version.
 * This function is not implemented on Windows.
 *
 ***********************************************************************/
HRESULT WINAPI DirectDrawEnumerateExW(LPDDENUMCALLBACKEXW callback, void *context, DWORD flags)
{
    TRACE("callback %p, context %p, flags %#lx.\n", callback, context, flags);

    return DDERR_UNSUPPORTED;
}

/***********************************************************************
 * Classfactory implementation.
 ***********************************************************************/

/***********************************************************************
 * CF_CreateDirectDraw
 *
 * DDraw creation function for the class factory
 *
 * Params:
 *  UnkOuter: Set to NULL
 *  iid: ID of the wanted interface
 *  obj: Address to pass the interface pointer back
 *
 * Returns
 *  DD_OK / DDERR*, see DDRAW_Create
 *
 ***********************************************************************/
static HRESULT
CF_CreateDirectDraw(IUnknown* UnkOuter, REFIID iid,
                    void **obj)
{
    HRESULT hr;

    TRACE("outer_unknown %p, riid %s, object %p.\n", UnkOuter, debugstr_guid(iid), obj);

    wined3d_mutex_lock();
    hr = DDRAW_Create(NULL, obj, UnkOuter, iid);
    wined3d_mutex_unlock();

    return hr;
}

/***********************************************************************
 * CF_CreateDirectDraw
 *
 * Clipper creation function for the class factory
 *
 * Params:
 *  UnkOuter: Set to NULL
 *  iid: ID of the wanted interface
 *  obj: Address to pass the interface pointer back
 *
 * Returns
 *  DD_OK / DDERR*, see DDRAW_Create
 *
 ***********************************************************************/
static HRESULT
CF_CreateDirectDrawClipper(IUnknown* UnkOuter, REFIID riid,
                              void **obj)
{
    HRESULT hr;
    IDirectDrawClipper *Clip;

    TRACE("outer_unknown %p, riid %s, object %p.\n", UnkOuter, debugstr_guid(riid), obj);

    wined3d_mutex_lock();
    hr = DirectDrawCreateClipper(0, &Clip, UnkOuter);
    if (hr != DD_OK)
    {
        wined3d_mutex_unlock();
        return hr;
    }

    hr = IDirectDrawClipper_QueryInterface(Clip, riid, obj);
    IDirectDrawClipper_Release(Clip);

    wined3d_mutex_unlock();

    return hr;
}

static const struct object_creation_info object_creation[] =
{
    { &CLSID_DirectDraw,        CF_CreateDirectDraw },
    { &CLSID_DirectDraw7,       CF_CreateDirectDraw },
    { &CLSID_DirectDrawClipper, CF_CreateDirectDrawClipper }
};

struct ddraw_class_factory
{
    IClassFactory IClassFactory_iface;

    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *outer, REFIID iid, void **out);
};

static inline struct ddraw_class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_class_factory, IClassFactory_iface);
}

/*******************************************************************************
 * IDirectDrawClassFactory::QueryInterface
 *
 * QueryInterface for the class factory
 *
 * PARAMS
 *    riid   Reference to identifier of queried interface
 *    ppv    Address to return the interface pointer at
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: E_NOINTERFACE
 *
 *******************************************************************************/
static HRESULT WINAPI ddraw_class_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

/*******************************************************************************
 * IDirectDrawClassFactory::AddRef
 *
 * AddRef for the class factory
 *
 * RETURNS
 *  The new refcount
 *
 *******************************************************************************/
static ULONG WINAPI ddraw_class_factory_AddRef(IClassFactory *iface)
{
    struct ddraw_class_factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&factory->ref);

    TRACE("%p increasing refcount to %lu.\n", factory, ref);

    return ref;
}

/*******************************************************************************
 * IDirectDrawClassFactory::Release
 *
 * Release for the class factory. If the refcount falls to 0, the object
 * is destroyed
 *
 * RETURNS
 *  The new refcount
 *
 *******************************************************************************/
static ULONG WINAPI ddraw_class_factory_Release(IClassFactory *iface)
{
    struct ddraw_class_factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&factory->ref);

    TRACE("%p decreasing refcount to %lu.\n", factory, ref);

    if (!ref)
        free(factory);

    return ref;
}


/*******************************************************************************
 * IDirectDrawClassFactory::CreateInstance
 *
 * What is this? Seems to create DirectDraw objects...
 *
 * Params
 *  The usual things???
 *
 * RETURNS
 *  ???
 *
 *******************************************************************************/
static HRESULT WINAPI ddraw_class_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer_unknown, REFIID riid, void **out)
{
    struct ddraw_class_factory *factory = impl_from_IClassFactory(iface);

    TRACE("iface %p, outer_unknown %p, riid %s, out %p.\n",
            iface, outer_unknown, debugstr_guid(riid), out);

    return factory->pfnCreateInstance(outer_unknown, riid, out);
}

/*******************************************************************************
 * IDirectDrawClassFactory::LockServer
 *
 * What is this?
 *
 * Params
 *  ???
 *
 * RETURNS
 *  S_OK, because it's a stub
 *
 *******************************************************************************/
static HRESULT WINAPI ddraw_class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("iface %p, dolock %#x stub!\n", iface, dolock);

    return S_OK;
}

/*******************************************************************************
 * The class factory VTable
 *******************************************************************************/
static const IClassFactoryVtbl IClassFactory_Vtbl =
{
    ddraw_class_factory_QueryInterface,
    ddraw_class_factory_AddRef,
    ddraw_class_factory_Release,
    ddraw_class_factory_CreateInstance,
    ddraw_class_factory_LockServer
};

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **out)
{
    struct ddraw_class_factory *factory;
    unsigned int i;

    TRACE("rclsid %s, riid %s, out %p.\n",
            debugstr_guid(rclsid), debugstr_guid(riid), out);

    if (!IsEqualGUID(&IID_IClassFactory, riid)
            && !IsEqualGUID(&IID_IUnknown, riid))
        return E_NOINTERFACE;

    for (i=0; i < ARRAY_SIZE(object_creation); i++)
    {
        if (IsEqualGUID(object_creation[i].clsid, rclsid))
            break;
    }

    if (i == ARRAY_SIZE(object_creation))
    {
        FIXME("%s: no class found.\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if (!(factory = calloc(1, sizeof(*factory))))
        return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &IClassFactory_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *out = factory;
    return S_OK;
}


/***********************************************************************
 * DllMain (DDRAW.0)
 *
 * Could be used to register DirectDraw drivers, if we have more than
 * one. Also used to destroy any objects left at unload if the
 * app didn't release them properly(Gothic 2, Diablo 2, Moto racer, ...)
 *
 ***********************************************************************/
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        static HMODULE ddraw_self;
        HKEY hkey = 0;
        WNDCLASSA wc;

        /* Register the window class. This is used to create a hidden window
         * for D3D rendering, if the application didn't pass one. It can also
         * be used for creating a device window from SetCooperativeLevel(). */
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = DefWindowProcA;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = inst;
        wc.hIcon = 0;
        wc.hCursor = 0;
        wc.hbrBackground = GetStockObject(BLACK_BRUSH);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = DDRAW_WINDOW_CLASS_NAME;
        if (!RegisterClassA(&wc))
        {
            ERR("Failed to register ddraw window class, last error %#lx.\n", GetLastError());
            return FALSE;
        }

        if (!ddraw_handle_table_init(&global_handle_table, 64))
        {
            UnregisterClassA(DDRAW_WINDOW_CLASS_NAME, inst);
            return FALSE;
        }

        /* On Windows one can force the refresh rate that DirectDraw uses by
         * setting an override value in dxdiag.  This is documented in KB315614
         * (main article), KB230002, and KB217348.  By comparing registry dumps
         * before and after setting the override, we see that the override value
         * is stored in HKLM\Software\Microsoft\DirectDraw\ForceRefreshRate as a
         * DWORD that represents the refresh rate to force.  We use this
         * registry entry to modify the behavior of SetDisplayMode so that Wine
         * users can override the refresh rate in a Windows-compatible way.
         *
         * dxdiag will not accept a refresh rate lower than 40 or higher than
         * 120 so this value should be within that range.  It is, of course,
         * possible for a user to set the registry entry value directly so that
         * assumption might not hold.
         *
         * There is no current mechanism for setting this value through the Wine
         * GUI.  It would be most appropriate to set this value through a dxdiag
         * clone, but it may be sufficient to use winecfg.
         *
         * TODO: Create a mechanism for setting this value through the Wine GUI.
         */
        if ( !RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectDraw", &hkey ) )
        {
            DWORD type, data, size;

            size = sizeof(data);
            if (!RegQueryValueExA(hkey, "ForceRefreshRate", NULL, &type, (BYTE *)&data, &size) && type == REG_DWORD)
            {
                TRACE("ForceRefreshRate set; overriding refresh rate to %ld Hz\n", data);
                force_refresh_rate = data;
            }
            RegCloseKey( hkey );
        }

        /* Prevent the ddraw module from being unloaded. When switching to
         * exclusive mode, we replace the window proc of the ddraw window. If
         * an application would unload ddraw from the WM_DESTROY handler for
         * that window, it would return to unmapped memory and die. Apparently
         * this is supposed to work on Windows. */
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                (const WCHAR *)&ddraw_self, &ddraw_self))
            ERR("Failed to get own module handle.\n");

        instance = inst;
        DisableThreadLibraryCalls(inst);
        break;
    }

    case DLL_PROCESS_DETACH:
        if (WARN_ON(ddraw))
        {
            struct ddraw *ddraw;
            unsigned int i;

            LIST_FOR_EACH_ENTRY(ddraw, &global_ddraw_list, struct ddraw, ddraw_list_entry)
            {
                struct ddraw_surface *surface;

                WARN("DirectDraw object %p has reference counts {%lu, %lu, %lu, %lu, %lu}.\n",
                        ddraw, ddraw->ref7, ddraw->ref4, ddraw->ref3, ddraw->ref2, ddraw->ref1);

                if (!list_empty(&ddraw->d3ddevice_list))
                    WARN("DirectDraw object %p has Direct3D device(s) attached.\n", ddraw);

                LIST_FOR_EACH_ENTRY(surface, &ddraw->surface_list, struct ddraw_surface, surface_list_entry)
                {
                    WARN("Surface %p has reference counts {%lu, %lu, %lu, %lu, %lu, %lu}.\n",
                            surface, surface->ref7, surface->ref4, surface->ref3,
                            surface->ref2, surface->ref1, surface->gamma_count);
                }
            }

            for (i = 0; i < global_handle_table.entry_count; ++i)
            {
                struct ddraw_handle_entry *entry = &global_handle_table.entries[i];

                switch (entry->type)
                {
                    case DDRAW_HANDLE_FREE:
                        break;

                    case DDRAW_HANDLE_MATERIAL:
                        WARN("Material handle %#x (%p) not unset properly.\n", i + 1, entry->object);
                        break;

                    case DDRAW_HANDLE_SURFACE:
                        WARN("Texture handle %#x (%p) not unset properly.\n", i + 1, entry->object);
                        break;

                    case DDRAW_HANDLE_MATRIX:
                        WARN("Leftover matrix handle %#x (%p), deleting.\n", i + 1, entry->object);
                        break;

                    default:
                        WARN("Handle %#x (%p) has unknown type %#x.\n", i + 1, entry->object, entry->type);
                        break;
                }
            }
            ddraw_handle_table_destroy(&global_handle_table);
        }

        if (reserved) break;
        UnregisterClassA(DDRAW_WINDOW_CLASS_NAME, inst);
    }

    return TRUE;
}
