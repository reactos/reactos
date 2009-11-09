/*        DirectDraw Base Functions
 *
 * Copyright 1997-1999 Marcus Meissner
 * Copyright 1998 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2006 Stefan Dösinger
 * Copyright 2008 Denver Gingerich
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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
#include "wine/debug.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "wine/exception.h"
#include "winreg.h"

#include "ddraw.h"
#include "d3d.h"

#define DDRAW_INIT_GUID
#include "ddraw_private.h"

static typeof(WineDirect3DCreate) *pWineDirect3DCreate;

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/* The configured default surface */
WINED3DSURFTYPE DefaultSurfaceType = SURFACE_UNKNOWN;

/* DDraw list and critical section */
static struct list global_ddraw_list = LIST_INIT(global_ddraw_list);

static CRITICAL_SECTION_DEBUG ddraw_cs_debug =
{
    0, 0, &ddraw_cs,
    { &ddraw_cs_debug.ProcessLocksList,
    &ddraw_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": ddraw_cs") }
};
CRITICAL_SECTION ddraw_cs = { &ddraw_cs_debug, -1, 0, 0, 0, 0 };

/* value of ForceRefreshRate */
DWORD force_refresh_rate = 0;

/*
 * Helper Function for DDRAW_Create and DirectDrawCreateClipper for
 * lazy loading of the Wine D3D driver.
 *
 * Returns
 *  TRUE on success
 *  FALSE on failure.
 */

BOOL LoadWineD3D(void)
{
    static HMODULE hWineD3D = (HMODULE) -1;
    if (hWineD3D == (HMODULE) -1)
    {
        hWineD3D = LoadLibraryA("wined3d");
        if (hWineD3D)
        {
            pWineDirect3DCreate = (typeof(WineDirect3DCreate) *)GetProcAddress(hWineD3D, "WineDirect3DCreate");
            pWineDirect3DCreateClipper = (typeof(WineDirect3DCreateClipper) *) GetProcAddress(hWineD3D, "WineDirect3DCreateClipper");
            return TRUE;
        }
    }
    return hWineD3D != NULL;
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
static HRESULT
DDRAW_Create(const GUID *guid,
             void **DD,
             IUnknown *UnkOuter,
             REFIID iid)
{
    IDirectDrawImpl *This = NULL;
    HRESULT hr;
    IWineD3D *wineD3D = NULL;
    IWineD3DDevice *wineD3DDevice = NULL;
    HDC hDC;
    WINED3DDEVTYPE devicetype;

    TRACE("(%s,%p,%p)\n", debugstr_guid(guid), DD, UnkOuter);

    *DD = NULL;

    /* We don't care about this guids. Well, there's no special guid anyway
     * OK, we could
     */
    if (guid == (GUID *) DDCREATE_EMULATIONONLY)
    {
        /* Use the reference device id. This doesn't actually change anything,
         * WineD3D always uses OpenGL for D3D rendering. One could make it request
         * indirect rendering
         */
        devicetype = WINED3DDEVTYPE_REF;
    }
    else if(guid == (GUID *) DDCREATE_HARDWAREONLY)
    {
        devicetype = WINED3DDEVTYPE_HAL;
    }
    else
    {
        devicetype = 0;
    }

    /* DDraw doesn't support aggregation, according to msdn */
    if (UnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    /* DirectDraw creation comes here */
    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawImpl));
    if(!This)
    {
        ERR("Out of memory when creating DirectDraw\n");
        return E_OUTOFMEMORY;
    }

    /* The interfaces:
     * IDirectDraw and IDirect3D are the same object,
     * QueryInterface is used to get other interfaces.
     */
    This->lpVtbl = &IDirectDraw7_Vtbl;
    This->IDirectDraw_vtbl = &IDirectDraw1_Vtbl;
    This->IDirectDraw2_vtbl = &IDirectDraw2_Vtbl;
    This->IDirectDraw4_vtbl = &IDirectDraw4_Vtbl;
    This->IDirect3D_vtbl = &IDirect3D1_Vtbl;
    This->IDirect3D2_vtbl = &IDirect3D2_Vtbl;
    This->IDirect3D3_vtbl = &IDirect3D3_Vtbl;
    This->IDirect3D7_vtbl = &IDirect3D7_Vtbl;
    This->device_parent_vtbl = &ddraw_wined3d_device_parent_vtbl;

    /* See comments in IDirectDrawImpl_CreateNewSurface for a description
     * of this member.
     * Read from a registry key, should add a winecfg option later
     */
    This->ImplType = DefaultSurfaceType;

    /* Get the current screen settings */
    hDC = GetDC(0);
    This->orig_bpp = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
    ReleaseDC(0, hDC);
    This->orig_width = GetSystemMetrics(SM_CXSCREEN);
    This->orig_height = GetSystemMetrics(SM_CYSCREEN);

    if (!LoadWineD3D())
    {
        ERR("Couldn't load WineD3D - OpenGL libs not present?\n");
        hr = DDERR_NODIRECTDRAWSUPPORT;
        goto err_out;
    }

    /* Initialize WineD3D
     *
     * All Rendering (2D and 3D) is relayed to WineD3D,
     * but DirectDraw specific management, like DDSURFACEDESC and DDPIXELFORMAT
     * structure handling is handled in this lib.
     */
    wineD3D = pWineDirect3DCreate(7 /* DXVersion */, (IUnknown *) This /* Parent */);
    if(!wineD3D)
    {
        ERR("Failed to initialise WineD3D\n");
        hr = E_OUTOFMEMORY;
        goto err_out;
    }
    This->wineD3D = wineD3D;
    TRACE("WineD3D created at %p\n", wineD3D);

    /* Initialized member...
     *
     * It is set to false at creation time, and set to true in
     * IDirectDraw7::Initialize. Its sole purpose is to return DD_OK on
     * initialize only once
     */
    This->initialized = FALSE;

    /* Initialize WineD3DDevice
     *
     * It is used for screen setup, surface and palette creation
     * When a Direct3DDevice7 is created, the D3D capabilities of WineD3D are
     * initialized
     */
    hr = IWineD3D_CreateDevice(wineD3D, 0 /* D3D_ADAPTER_DEFAULT */, devicetype, NULL /* FocusWindow, don't know yet */,
            0 /* BehaviorFlags */, (IUnknown *)This, (IWineD3DDeviceParent *)&This->device_parent_vtbl, &wineD3DDevice);
    if(FAILED(hr))
    {
        ERR("Failed to create a wineD3DDevice, result = %x\n", hr);
        goto err_out;
    }
    This->wineD3DDevice = wineD3DDevice;
    TRACE("wineD3DDevice created at %p\n", This->wineD3DDevice);

    /* Register the window class
     *
     * It is used to create a hidden window for D3D
     * rendering, if the application didn't pass one.
     * It can also be used for Creating a device window
     * from SetCooperativeLevel
     *
     * The name: DDRAW_<address>. The classname is
     * 32 bit long, so a 64 bit address will fit nicely
     * (Will this be compiled for 64 bit anyway?)
     *
     */
    sprintf(This->classname, "DDRAW_%p", This);

    memset(&This->wnd_class, 0, sizeof(This->wnd_class));
    This->wnd_class.style = CS_HREDRAW | CS_VREDRAW;
    This->wnd_class.lpfnWndProc = DefWindowProcA;
    This->wnd_class.cbClsExtra = 0;
    This->wnd_class.cbWndExtra = 0;
    This->wnd_class.hInstance = GetModuleHandleA(0);
    This->wnd_class.hIcon = 0;
    This->wnd_class.hCursor = 0;
    This->wnd_class.hbrBackground = GetStockObject(BLACK_BRUSH);
    This->wnd_class.lpszMenuName = NULL;
    This->wnd_class.lpszClassName = This->classname;
    if(!RegisterClassA(&This->wnd_class))
    {
        ERR("RegisterClassA failed!\n");
        goto err_out;
    }

    /* Get the amount of video memory */
    This->total_vidmem = IWineD3DDevice_GetAvailableTextureMem(This->wineD3DDevice);

    list_init(&This->surface_list);
    list_add_head(&global_ddraw_list, &This->ddraw_list_entry);

    /* Call QueryInterface to get the pointer to the requested interface. This also initializes
     * The required refcount
     */
    hr = IDirectDraw7_QueryInterface((IDirectDraw7 *)This, iid, DD);
    if(SUCCEEDED(hr)) return DD_OK;

err_out:
    /* Let's hope we never need this ;) */
    if(wineD3DDevice) IWineD3DDevice_Release(wineD3DDevice);
    if(wineD3D) IWineD3D_Release(wineD3D);
    if(This) HeapFree(GetProcessHeap(), 0, This->decls);
    HeapFree(GetProcessHeap(), 0, This);
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
HRESULT WINAPI
DirectDrawCreate(GUID *GUID,
                 LPDIRECTDRAW *DD,
                 IUnknown *UnkOuter)
{
    HRESULT hr;
    TRACE("(%s,%p,%p)\n", debugstr_guid(GUID), DD, UnkOuter);

    EnterCriticalSection(&ddraw_cs);
    hr = DDRAW_Create(GUID, (void **) DD, UnkOuter, &IID_IDirectDraw);
    LeaveCriticalSection(&ddraw_cs);
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
HRESULT WINAPI
DirectDrawCreateEx(GUID *GUID,
                   LPVOID *DD,
                   REFIID iid,
                   IUnknown *UnkOuter)
{
    HRESULT hr;
    TRACE("(%s,%p,%s,%p)\n", debugstr_guid(GUID), DD, debugstr_guid(iid), UnkOuter);

    if (!IsEqualGUID(iid, &IID_IDirectDraw7))
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    hr = DDRAW_Create(GUID, DD, UnkOuter, iid);
    LeaveCriticalSection(&ddraw_cs);
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
HRESULT WINAPI
DirectDrawEnumerateA(LPDDENUMCALLBACKA Callback,
                     LPVOID Context)
{
    BOOL stop = FALSE;

    TRACE(" Enumerating default DirectDraw HAL interface\n");
    /* We only have one driver */
    __TRY
    {
        static CHAR driver_desc[] = "DirectDraw HAL",
        driver_name[] = "display";

        stop = !Callback(NULL, driver_desc, driver_name, Context);
    }
    __EXCEPT_PAGE_FAULT
    {
        return E_INVALIDARG;
    }
    __ENDTRY

    TRACE(" End of enumeration\n");
    return DD_OK;
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
HRESULT WINAPI
DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA Callback,
                       LPVOID Context,
                       DWORD Flags)
{
    BOOL stop = FALSE;
    TRACE("Enumerating default DirectDraw HAL interface\n");

    /* We only have one driver by now */
    __TRY
    {
        static CHAR driver_desc[] = "DirectDraw HAL",
        driver_name[] = "display";

        /* QuickTime expects the description "DirectDraw HAL" */
        stop = !Callback(NULL, driver_desc, driver_name, Context, 0);
    }
    __EXCEPT_PAGE_FAULT
    {
        return E_INVALIDARG;
    }
    __ENDTRY;

    TRACE("End of enumeration\n");
    return DD_OK;
}

/***********************************************************************
 * DirectDrawEnumerateW (DDRAW.@)
 *
 * Enumerates legacy drivers, unicode version. See
 * the comments above DirectDrawEnumerateA for more details.
 *
 * The Flag member is not supported right now.
 *
 ***********************************************************************/

/***********************************************************************
 * DirectDrawEnumerateExW (DDRAW.@)
 *
 * Enumerates DirectDraw7 drivers, unicode version. See
 * the comments above DirectDrawEnumerateA for more details.
 *
 * The Flag member is not supported right now.
 *
 ***********************************************************************/

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

    TRACE("(%p,%s,%p)\n", UnkOuter, debugstr_guid(iid), obj);

    EnterCriticalSection(&ddraw_cs);
    hr = DDRAW_Create(NULL, obj, UnkOuter, iid);
    LeaveCriticalSection(&ddraw_cs);
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

    EnterCriticalSection(&ddraw_cs);
    hr = DirectDrawCreateClipper(0, &Clip, UnkOuter);
    if (hr != DD_OK)
    {
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    hr = IDirectDrawClipper_QueryInterface(Clip, riid, obj);
    IDirectDrawClipper_Release(Clip);

    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static const struct object_creation_info object_creation[] =
{
    { &CLSID_DirectDraw,        CF_CreateDirectDraw },
    { &CLSID_DirectDraw7,       CF_CreateDirectDraw },
    { &CLSID_DirectDrawClipper, CF_CreateDirectDrawClipper }
};

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
static HRESULT WINAPI
IDirectDrawClassFactoryImpl_QueryInterface(IClassFactory *iface,
                    REFIID riid,
                    void **obj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *obj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),obj);
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
static ULONG WINAPI
IDirectDrawClassFactoryImpl_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %d.\n", This, ref - 1);

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
static ULONG WINAPI
IDirectDrawClassFactoryImpl_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->() decrementing from %d.\n", This, ref+1);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

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
static HRESULT WINAPI
IDirectDrawClassFactoryImpl_CreateInstance(IClassFactory *iface,
                                           IUnknown *UnkOuter,
                                           REFIID riid,
                                           void **obj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    TRACE("(%p)->(%p,%s,%p)\n",This,UnkOuter,debugstr_guid(riid),obj);

    return This->pfnCreateInstance(UnkOuter, riid, obj);
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
static HRESULT WINAPI
IDirectDrawClassFactoryImpl_LockServer(IClassFactory *iface,BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

/*******************************************************************************
 * The class factory VTable
 *******************************************************************************/
static const IClassFactoryVtbl IClassFactory_Vtbl =
{
    IDirectDrawClassFactoryImpl_QueryInterface,
    IDirectDrawClassFactoryImpl_AddRef,
    IDirectDrawClassFactoryImpl_Release,
    IDirectDrawClassFactoryImpl_CreateInstance,
    IDirectDrawClassFactoryImpl_LockServer
};

/*******************************************************************************
 * DllGetClassObject [DDRAW.@]
 * Retrieves class object from a DLL object
 *
 * NOTES
 *    Docs say returns STDAPI
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    unsigned int i;
    IClassFactoryImpl *factory;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if ( !IsEqualGUID( &IID_IClassFactory, riid )
	 && ! IsEqualGUID( &IID_IUnknown, riid) )
	return E_NOINTERFACE;

    for (i=0; i < sizeof(object_creation)/sizeof(object_creation[0]); i++)
    {
	if (IsEqualGUID(object_creation[i].clsid, rclsid))
	    break;
    }

    if (i == sizeof(object_creation)/sizeof(object_creation[0]))
    {
	FIXME("%s: no class found.\n", debugstr_guid(rclsid));
	return CLASS_E_CLASSNOTAVAILABLE;
    }

    factory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    factory->lpVtbl = &IClassFactory_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = factory;
    return S_OK;
}


/*******************************************************************************
 * DllCanUnloadNow [DDRAW.@]  Determines whether the DLL is in use.
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: S_FALSE
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    HRESULT hr;
    FIXME("(void): stub\n");

    EnterCriticalSection(&ddraw_cs);
    hr = S_FALSE;
    LeaveCriticalSection(&ddraw_cs);

    return hr;
}

/*******************************************************************************
 * DestroyCallback
 *
 * Callback function for the EnumSurfaces call in DllMain.
 * Dumps some surface info and releases the surface
 *
 * Params:
 *  surf: The enumerated surface
 *  desc: it's description
 *  context: Pointer to the ddraw impl
 *
 * Returns:
 *  DDENUMRET_OK;
 *******************************************************************************/
static HRESULT WINAPI
DestroyCallback(IDirectDrawSurface7 *surf,
                DDSURFACEDESC2 *desc,
                void *context)
{
    IDirectDrawSurfaceImpl *Impl = (IDirectDrawSurfaceImpl *)surf;
    IDirectDrawImpl *ddraw = context;
    ULONG ref;

    ref = IDirectDrawSurface7_Release(surf);  /* For the EnumSurfaces */
    WARN("Surface %p has an reference count of %d\n", Impl, ref);

    /* Skip surfaces which are attached somewhere or which are
     * part of a complex compound. They will get released when destroying
     * the root
     */
    if( (!Impl->is_complex_root) || (Impl->first_attached != Impl) )
        return DDENUMRET_OK;
    /* Skip our depth stencil surface, it will be released with the render target */
    if( Impl == ddraw->DepthStencilBuffer)
        return DDENUMRET_OK;

    /* Destroy the surface */
    while(ref) ref = IDirectDrawSurface7_Release(surf);

    return DDENUMRET_OK;
}

/***********************************************************************
 * get_config_key
 *
 * Reads a config key from the registry. Taken from WineD3D
 *
 ***********************************************************************/
static inline DWORD get_config_key(HKEY defkey, HKEY appkey, const char* name, char* buffer, DWORD size)
{
    if (0 != appkey && !RegQueryValueExA( appkey, name, 0, NULL, (LPBYTE) buffer, &size )) return 0;
    if (0 != defkey && !RegQueryValueExA( defkey, name, 0, NULL, (LPBYTE) buffer, &size )) return 0;
    return ERROR_FILE_NOT_FOUND;
}

/***********************************************************************
 * DllMain (DDRAW.0)
 *
 * Could be used to register DirectDraw drivers, if we have more than
 * one. Also used to destroy any objects left at unload if the
 * app didn't release them properly(Gothic 2, Diablo 2, Moto racer, ...)
 *
 ***********************************************************************/
BOOL WINAPI
DllMain(HINSTANCE hInstDLL,
        DWORD Reason,
        LPVOID lpv)
{
    TRACE("(%p,%x,%p)\n", hInstDLL, Reason, lpv);
    if (Reason == DLL_PROCESS_ATTACH)
    {
        char buffer[MAX_PATH+10];
        DWORD size = sizeof(buffer);
        HKEY hkey = 0;
        HKEY appkey = 0;
        DWORD len;

       /* @@ Wine registry key: HKCU\Software\Wine\Direct3D */
       if ( RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Direct3D", &hkey ) ) hkey = 0;

       len = GetModuleFileNameA( 0, buffer, MAX_PATH );
       if (len && len < MAX_PATH)
       {
            HKEY tmpkey;
            /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\Direct3D */
            if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey ))
            {
                char *p, *appname = buffer;
                if ((p = strrchr( appname, '/' ))) appname = p + 1;
                if ((p = strrchr( appname, '\\' ))) appname = p + 1;
                strcat( appname, "\\Direct3D" );
                TRACE("appname = [%s]\n", appname);
                if (RegOpenKeyA( tmpkey, appname, &appkey )) appkey = 0;
                RegCloseKey( tmpkey );
            }
       }

       if ( 0 != hkey || 0 != appkey )
       {
            if ( !get_config_key( hkey, appkey, "DirectDrawRenderer", buffer, size) )
            {
                if (!strcmp(buffer,"gdi"))
                {
                    TRACE("Defaulting to GDI surfaces\n");
                    DefaultSurfaceType = SURFACE_GDI;
                }
                else if (!strcmp(buffer,"opengl"))
                {
                    TRACE("Defaulting to opengl surfaces\n");
                    DefaultSurfaceType = SURFACE_OPENGL;
                }
                else
                {
                    ERR("Unknown default surface type. Supported are:\n gdi, opengl\n");
                }
            }
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
            DWORD type, data;
            size = sizeof(data);
            if (!RegQueryValueExA( hkey, "ForceRefreshRate", NULL, &type, (LPBYTE)&data, &size ) && type == REG_DWORD)
            {
                TRACE("ForceRefreshRate set; overriding refresh rate to %d Hz\n", data);
                force_refresh_rate = data;
            }
            RegCloseKey( hkey );
        }

        DisableThreadLibraryCalls(hInstDLL);
    }
    else if (Reason == DLL_PROCESS_DETACH)
    {
        if(!list_empty(&global_ddraw_list))
        {
            struct list *entry, *entry2;
            WARN("There are still existing DirectDraw interfaces. Wine bug or buggy application?\n");

            /* We remove elements from this loop */
            LIST_FOR_EACH_SAFE(entry, entry2, &global_ddraw_list)
            {
                HRESULT hr;
                DDSURFACEDESC2 desc;
                int i;
                IDirectDrawImpl *ddraw = LIST_ENTRY(entry, IDirectDrawImpl, ddraw_list_entry);

                WARN("DDraw %p has a refcount of %d\n", ddraw, ddraw->ref7 + ddraw->ref4 + ddraw->ref3 + ddraw->ref2 + ddraw->ref1);

                /* Add references to each interface to avoid freeing them unexpectedly */
                IDirectDraw_AddRef((IDirectDraw *)&ddraw->IDirectDraw_vtbl);
                IDirectDraw2_AddRef((IDirectDraw2 *)&ddraw->IDirectDraw2_vtbl);
                IDirectDraw4_AddRef((IDirectDraw4 *)&ddraw->IDirectDraw4_vtbl);
                IDirectDraw7_AddRef((IDirectDraw7 *)ddraw);

                /* Does a D3D device exist? Destroy it
                    * TODO: Destroy all Vertex buffers, Lights, Materials
                    * and execute buffers too
                    */
                if(ddraw->d3ddevice)
                {
                    WARN("DDraw %p has d3ddevice %p attached\n", ddraw, ddraw->d3ddevice);
                    while(IDirect3DDevice7_Release((IDirect3DDevice7 *)ddraw->d3ddevice));
                }

                /* Try to release the objects
                    * Do an EnumSurfaces to find any hanging surfaces
                    */
                memset(&desc, 0, sizeof(desc));
                desc.dwSize = sizeof(desc);
                for(i = 0; i <= 1; i++)
                {
                    hr = IDirectDraw7_EnumSurfaces((IDirectDraw7 *)ddraw,
                            DDENUMSURFACES_ALL, &desc, ddraw, DestroyCallback);
                    if(hr != D3D_OK)
                        ERR("(%p) EnumSurfaces failed, prepare for trouble\n", ddraw);
                }

                /* Check the surface count */
                if(ddraw->surfaces > 0)
                    ERR("DDraw %p still has %d surfaces attached\n", ddraw, ddraw->surfaces);

                /* Release all hanging references to destroy the objects. This
                    * restores the screen mode too
                    */
                while(IDirectDraw_Release((IDirectDraw *)&ddraw->IDirectDraw_vtbl));
                while(IDirectDraw2_Release((IDirectDraw2 *)&ddraw->IDirectDraw2_vtbl));
                while(IDirectDraw4_Release((IDirectDraw4 *)&ddraw->IDirectDraw4_vtbl));
                while(IDirectDraw7_Release((IDirectDraw7 *)ddraw));
            }
        }
    }

    return TRUE;
}
