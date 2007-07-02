/*        DirectDrawEx
 *
 * Copyright 2006 Ulrich Czekalla
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

#define COBJMACROS

#ifndef WINE_NATIVEWIN32
# include "winbase.h"
# include "wingdi.h"
#else
# include <windows.h>
#endif

#include "ddraw.h"

#include "initguid.h"
#include "ddrawex_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);


#ifdef WINE_NATIVEWIN32
static HMODULE hDDrawEx;

HRESULT (WINAPI *pDllCanUnloadNow)(void);
HRESULT (WINAPI *pDllGetClassObject)(
    REFCLSID rclsid, REFIID riid, LPVOID *ppv);

BOOL
IsPassthrough()
{
    if (!hDDrawEx)
        return FALSE;
    if (hDDrawEx != (HMODULE) -1)
        return TRUE;
    hDDrawEx = LoadLibraryA("ddrawex.sav");
    if (hDDrawEx) {
#define getproc(x) *(FARPROC *)&p##x = GetProcAddress(hDDrawEx, #x)
        getproc(DllCanUnloadNow);
        getproc(DllGetClassObject);
#undef getproc
    }
    return hDDrawEx != NULL;
}
#endif

/*******************************************************************************
 * IDirectDrawClassFactory::QueryInterface
 *
 *******************************************************************************/
static HRESULT WINAPI
IDirectDrawClassFactoryImpl_QueryInterface(IClassFactory *iface,
                    REFIID riid,
                    void **obj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl*) iface;

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
 *******************************************************************************/
static ULONG WINAPI
IDirectDrawClassFactoryImpl_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl*) iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %d.\n", This, ref - 1);

    return ref;
}

/*******************************************************************************
 * IDirectDrawClassFactory::Release
 *
 *******************************************************************************/
static ULONG WINAPI
IDirectDrawClassFactoryImpl_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl*) iface;
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->() decrementing from %d.\n", This, ref+1);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}


/*******************************************************************************
 * IDirectDrawClassFactory::CreateInstance
 *
 *******************************************************************************/
static HRESULT WINAPI
IDirectDrawClassFactoryImpl_CreateInstance(IClassFactory *iface,
                                           IUnknown *UnkOuter,
                                           REFIID riid,
                                           void **obj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl*) iface;

    TRACE("(%p)->(%p,%s,%p)\n",This,UnkOuter,debugstr_guid(riid),obj);

    return This->pfnCreateInstance(UnkOuter, riid, obj);
}

/*******************************************************************************
 * IDirectDrawClassFactory::LockServer
 *
 *******************************************************************************/
static HRESULT WINAPI
IDirectDrawClassFactoryImpl_LockServer(IClassFactory *iface,BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl*) iface;
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
 * IDirectDrawFactory::QueryInterface
 *
 *******************************************************************************/
static HRESULT WINAPI
IDirectDrawFactoryImpl_QueryInterface(IDirectDrawFactory *iface,
                    REFIID riid,
                    void **obj)
{
    IDirectDrawFactoryImpl *This = (IDirectDrawFactoryImpl*) iface;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirectDrawFactory))
    {
        IDirectDrawFactory_AddRef(iface);
        *obj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),obj);
    return E_NOINTERFACE;
}

/*******************************************************************************
 * IDirectDrawFactory::AddRef
 *
 *******************************************************************************/
static ULONG WINAPI
IDirectDrawFactoryImpl_AddRef(IDirectDrawFactory *iface)
{
    IDirectDrawFactoryImpl *This = (IDirectDrawFactoryImpl*) iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %d.\n", This, ref - 1);

    return ref;
}

/*******************************************************************************
 * IDirectDrawFactory::Release
 *
 *******************************************************************************/
static ULONG WINAPI
IDirectDrawFactoryImpl_Release(IDirectDrawFactory *iface)
{
    IDirectDrawFactoryImpl *This = (IDirectDrawFactoryImpl*) iface;
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->() decrementing from %d.\n", This, ref+1);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}


/*******************************************************************************
 * IDirectDrawFactoryImpl_CreateDirectDraw
 *******************************************************************************/
static HRESULT WINAPI
IDirectDrawFactoryImpl_CreateDirectDraw(IDirectDrawFactory* iface,
                                        GUID * pGUID,
                                        HWND hWnd,
                                        DWORD dwCoopLevelFlags,
                                        DWORD dwReserved,
                                        IUnknown *pUnkOuter,
                                        IDirectDraw **ppDirectDraw)
{
    HRESULT hr;

    TRACE("\n");

#ifndef WINE_NATIVEWIN32
    hr = DirectDrawCreateEx(pGUID, (void**)ppDirectDraw, &IID_IDirectDraw3, pUnkOuter);
#else
    hr = DirectDrawCreateEx(pGUID, (void**)ppDirectDraw, &IID_IDirectDraw7, pUnkOuter);
#endif

    if (FAILED(hr))
        return hr;

    hr = IDirectDraw_SetCooperativeLevel(*ppDirectDraw, hWnd, dwCoopLevelFlags);
    if (FAILED(hr))
        IDirectDraw_Release(*ppDirectDraw);

    return hr;
}


/*******************************************************************************
 * IDirectDrawFactoryImpl_DirectDrawEnumerate
 *******************************************************************************/
static HRESULT WINAPI
IDirectDrawFactoryImpl_DirectDrawEnumerate(IDirectDrawFactory* iface,
                                           LPDDENUMCALLBACKW lpCallback,
                                           LPVOID lpContext)
{
    FIXME("Stub!\n");
    return E_FAIL;
}


/*******************************************************************************
 * Direct Draw Factory VTable
 *******************************************************************************/
static const IDirectDrawFactoryVtbl IDirectDrawFactory_Vtbl =
{
    IDirectDrawFactoryImpl_QueryInterface,
    IDirectDrawFactoryImpl_AddRef,
    IDirectDrawFactoryImpl_Release,
    IDirectDrawFactoryImpl_CreateDirectDraw,
    IDirectDrawFactoryImpl_DirectDrawEnumerate,
};

/***********************************************************************
 * CreateDirectDrawFactory
 *
 ***********************************************************************/
static HRESULT
CreateDirectDrawFactory(IUnknown* UnkOuter, REFIID iid, void **obj)
{
    HRESULT hr;
    IDirectDrawFactoryImpl *This = NULL;

    TRACE("(%p,%s,%p)\n", UnkOuter, debugstr_guid(iid), obj);

    if (UnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawFactoryImpl));

    if(!This)
    {
        ERR("Out of memory when creating DirectDraw\n");
        return E_OUTOFMEMORY;
    }

    This->lpVtbl = &IDirectDrawFactory_Vtbl;

    hr = IDirectDrawFactory_QueryInterface((IDirectDrawFactory *)This, iid,  obj);

    if (FAILED(hr))
        HeapFree(GetProcessHeap(), 0, This);

    return hr;
}


/*******************************************************************************
 * DllCanUnloadNow [DDRAWEX.@]  Determines whether the DLL is in use.
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        return pDllCanUnloadNow();
#endif

    FIXME("(void): stub\n");
    return S_FALSE;
}


/*******************************************************************************
 * DllGetClassObject [DDRAWEX.@]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    IClassFactoryImpl *factory;

#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        return pDllGetClassObject(rclsid, riid, ppv);
#endif
    TRACE("ddrawex (%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (!IsEqualGUID( &IID_IClassFactory, riid)
        && !IsEqualGUID( &IID_IUnknown, riid))
        return E_NOINTERFACE;

    if (!IsEqualGUID(&CLSID_DirectDrawFactory, rclsid))
    {
        FIXME("%s: no class found.\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    factory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    factory->lpVtbl = &IClassFactory_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = CreateDirectDrawFactory;

    *ppv = (IClassFactory*) factory;

    return S_OK;
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
 * DllMain (DDRAWEX.0)
 *
 ***********************************************************************/
BOOL WINAPI
DllMain(HINSTANCE hInstDLL,
        DWORD Reason,
        void *lpv)
{
    TRACE("(%p,%x,%p)\n", hInstDLL, Reason, lpv);
#ifdef WINE_NATIVEWIN32
    if (Reason == DLL_PROCESS_ATTACH)
    {
        char buffer[MAX_PATH+64];
        DWORD size = sizeof(buffer);
        HKEY hkey = 0;
        HKEY appkey = 0;
        DWORD len;

       /* @@ Wine registry key: HKCU\Software\Wine\Direct3D */
       if ( RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Direct3D", &hkey )
           && RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Direct3D", &hkey ) )
           hkey = 0;

       len = GetModuleFileNameA( 0, buffer, MAX_PATH );
       if (len && len < MAX_PATH)
       {
            char *p, *appname = buffer;
            if ((p = strrchr( appname, '/' ))) appname = p + 1;
            if ((p = strrchr( appname, '\\' ))) appname = p + 1;
            TRACE("appname = [%s]\n", appname);
            memmove(
                buffer + strlen( "Software\\Wine\\AppDefaults\\" ),
                appname, strlen( appname ) + 1);
            memcpy(
                buffer, "Software\\Wine\\AppDefaults\\",
                strlen( "Software\\Wine\\AppDefaults\\" ));
            strcat( buffer, "\\Direct3D" );

            /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\Direct3D */
            if (RegOpenKeyA( HKEY_CURRENT_USER, buffer, &appkey )
                && RegOpenKeyA( HKEY_LOCAL_MACHINE, buffer, &appkey ))
                appkey = 0;
       }

       if ( 0 != hkey || 0 != appkey )
       {
            if ( !get_config_key( hkey, appkey, "Passthrough", buffer, size) )
            {
                if (!strcmp(buffer,"true") || !strcmp(buffer,"yes"))
                {
                    TRACE("Passthrough mode\n");
                    hDDrawEx = (HMODULE) -1;
                }
            }
        }
    }
#endif
    return TRUE;
}
