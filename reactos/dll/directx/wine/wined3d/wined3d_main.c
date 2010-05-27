/*
 * Direct3D wine internal interface main
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004      Jason Edmeades
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include "initguid.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

struct wined3d_wndproc
{
    HWND window;
    WNDPROC proc;
    IWineD3DDeviceImpl *device;
};

struct wined3d_wndproc_table
{
    struct wined3d_wndproc *entries;
    unsigned int count;
    unsigned int size;
};

static struct wined3d_wndproc_table wndproc_table;

int num_lock = 0;
void (*CDECL wine_tsx11_lock_ptr)(void) = NULL;
void (*CDECL wine_tsx11_unlock_ptr)(void) = NULL;

static CRITICAL_SECTION wined3d_cs;
static CRITICAL_SECTION_DEBUG wined3d_cs_debug =
{
    0, 0, &wined3d_cs,
    {&wined3d_cs_debug.ProcessLocksList,
    &wined3d_cs_debug.ProcessLocksList},
    0, 0, {(DWORD_PTR)(__FILE__ ": wined3d_cs")}
};
static CRITICAL_SECTION wined3d_cs = {&wined3d_cs_debug, -1, 0, 0, 0, 0};

/* When updating default value here, make sure to update winecfg as well,
 * where appropriate. */
wined3d_settings_t wined3d_settings =
{
    VS_HW,          /* Hardware by default */
    PS_HW,          /* Hardware by default */
    TRUE,           /* Use of GLSL enabled by default */
    ORM_FBO,        /* Use FBOs to do offscreen rendering */
    RTL_READTEX,    /* Default render target locking method */
    PCI_VENDOR_NONE,/* PCI Vendor ID */
    PCI_DEVICE_NONE,/* PCI Device ID */
    0,              /* The default of memory is set in FillGLCaps */
    NULL,           /* No wine logo by default */
    FALSE,          /* Disable multisampling for now due to Nvidia driver bugs which happens for some users */
    FALSE,          /* No strict draw ordering. */
};

IWineD3D * WINAPI WineDirect3DCreate(UINT version, IUnknown *parent)
{
    IWineD3DImpl *object;
    HRESULT hr;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate wined3d object memory.\n");
        return NULL;
    }

    hr = wined3d_init(object, version, parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize wined3d object, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return NULL;
    }

    TRACE("Created wined3d object %p for d3d%d support.\n", object, version);

    return (IWineD3D *)object;
}

static inline DWORD get_config_key(HKEY defkey, HKEY appkey, const char* name, char* buffer, DWORD size)
{
    if (0 != appkey && !RegQueryValueExA( appkey, name, 0, NULL, (LPBYTE) buffer, &size )) return 0;
    if (0 != defkey && !RegQueryValueExA( defkey, name, 0, NULL, (LPBYTE) buffer, &size )) return 0;
    return ERROR_FILE_NOT_FOUND;
}

static inline DWORD get_config_key_dword(HKEY defkey, HKEY appkey, const char* name, DWORD *data)
{
    DWORD type;
    DWORD size = sizeof(DWORD);
    if (0 != appkey && !RegQueryValueExA( appkey, name, 0, &type, (LPBYTE) data, &size ) && (type == REG_DWORD)) return 0;
    if (0 != defkey && !RegQueryValueExA( defkey, name, 0, &type, (LPBYTE) data, &size ) && (type == REG_DWORD)) return 0;
    return ERROR_FILE_NOT_FOUND;
}

static void CDECL wined3d_do_nothing(void)
{
}

static BOOL wined3d_dll_init(HINSTANCE hInstDLL)
{
    DWORD wined3d_context_tls_idx;
    HMODULE mod;
    char buffer[MAX_PATH+10];
    DWORD size = sizeof(buffer);
    HKEY hkey = 0;
    HKEY appkey = 0;
    DWORD len, tmpvalue;
    WNDCLASSA wc;

    wined3d_context_tls_idx = TlsAlloc();
    if (wined3d_context_tls_idx == TLS_OUT_OF_INDEXES)
    {
        DWORD err = GetLastError();
        ERR("Failed to allocate context TLS index, err %#x.\n", err);
        return FALSE;
    }
    context_set_tls_idx(wined3d_context_tls_idx);

    /* We need our own window class for a fake window which we use to retrieve GL capabilities */
    /* We might need CS_OWNDC in the future if we notice strange things on Windows.
     * Various articles/posts about OpenGL problems on Windows recommend this. */
    wc.style                = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc          = DefWindowProcA;
    wc.cbClsExtra           = 0;
    wc.cbWndExtra           = 0;
    wc.hInstance            = hInstDLL;
    wc.hIcon                = LoadIconA(NULL, (LPCSTR)IDI_WINLOGO);
    wc.hCursor              = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground        = NULL;
    wc.lpszMenuName         = NULL;
    wc.lpszClassName        = WINED3D_OPENGL_WINDOW_CLASS_NAME;

    if (!RegisterClassA(&wc))
    {
        ERR("Failed to register window class 'WineD3D_OpenGL'!\n");
        if (!TlsFree(wined3d_context_tls_idx))
        {
            DWORD err = GetLastError();
            ERR("Failed to free context TLS index, err %#x.\n", err);
        }
        return FALSE;
    }

    DisableThreadLibraryCalls(hInstDLL);

    mod = GetModuleHandleA( "winex11.drv" );
    if (mod)
    {
        wine_tsx11_lock_ptr   = (void *)GetProcAddress( mod, "wine_tsx11_lock" );
        wine_tsx11_unlock_ptr = (void *)GetProcAddress( mod, "wine_tsx11_unlock" );
    }
    else /* We are most likely on Windows */
    {
        wine_tsx11_lock_ptr   = wined3d_do_nothing;
        wine_tsx11_unlock_ptr = wined3d_do_nothing;
    }
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
        if ( !get_config_key( hkey, appkey, "VertexShaderMode", buffer, size) )
        {
            if (!strcmp(buffer,"none"))
            {
                TRACE("Disable vertex shaders\n");
                wined3d_settings.vs_mode = VS_NONE;
            }
        }
        if ( !get_config_key( hkey, appkey, "PixelShaderMode", buffer, size) )
        {
            if (!strcmp(buffer,"enabled"))
            {
                TRACE("Allow pixel shaders\n");
                wined3d_settings.ps_mode = PS_HW;
            }
            if (!strcmp(buffer,"disabled"))
            {
                TRACE("Disable pixel shaders\n");
                wined3d_settings.ps_mode = PS_NONE;
            }
        }
        if ( !get_config_key( hkey, appkey, "UseGLSL", buffer, size) )
        {
            if (!strcmp(buffer,"disabled"))
            {
                TRACE("Use of GL Shading Language disabled\n");
                wined3d_settings.glslRequested = FALSE;
            }
        }
        if ( !get_config_key( hkey, appkey, "OffscreenRenderingMode", buffer, size) )
        {
            if (!strcmp(buffer,"backbuffer"))
            {
                TRACE("Using the backbuffer for offscreen rendering\n");
                wined3d_settings.offscreen_rendering_mode = ORM_BACKBUFFER;
            }
            else if (!strcmp(buffer,"fbo"))
            {
                TRACE("Using FBOs for offscreen rendering\n");
                wined3d_settings.offscreen_rendering_mode = ORM_FBO;
            }
        }
        if ( !get_config_key( hkey, appkey, "RenderTargetLockMode", buffer, size) )
        {
            if (!strcmp(buffer,"disabled"))
            {
                TRACE("Disabling render target locking\n");
                wined3d_settings.rendertargetlock_mode = RTL_DISABLE;
            }
            else if (!strcmp(buffer,"readdraw"))
            {
                TRACE("Using glReadPixels for render target reading and glDrawPixels for writing\n");
                wined3d_settings.rendertargetlock_mode = RTL_READDRAW;
            }
            else if (!strcmp(buffer,"readtex"))
            {
                TRACE("Using glReadPixels for render target reading and textures for writing\n");
                wined3d_settings.rendertargetlock_mode = RTL_READTEX;
            }
        }
        if ( !get_config_key_dword( hkey, appkey, "VideoPciDeviceID", &tmpvalue) )
        {
            int pci_device_id = tmpvalue;

            /* A pci device id is 16-bit */
            if(pci_device_id > 0xffff)
            {
                ERR("Invalid value for VideoPciDeviceID. The value should be smaller or equal to 65535 or 0xffff\n");
            }
            else
            {
                TRACE("Using PCI Device ID %04x\n", pci_device_id);
                wined3d_settings.pci_device_id = pci_device_id;
            }
        }
        if ( !get_config_key_dword( hkey, appkey, "VideoPciVendorID", &tmpvalue) )
        {
            int pci_vendor_id = tmpvalue;

            /* A pci device id is 16-bit */
            if(pci_vendor_id > 0xffff)
            {
                ERR("Invalid value for VideoPciVendorID. The value should be smaller or equal to 65535 or 0xffff\n");
            }
            else
            {
                TRACE("Using PCI Vendor ID %04x\n", pci_vendor_id);
                wined3d_settings.pci_vendor_id = pci_vendor_id;
            }
        }
        if ( !get_config_key( hkey, appkey, "VideoMemorySize", buffer, size) )
        {
            int TmpVideoMemorySize = atoi(buffer);
            if(TmpVideoMemorySize > 0)
            {
                wined3d_settings.emulated_textureram = TmpVideoMemorySize *1024*1024;
                TRACE("Use %iMB = %d byte for emulated_textureram\n",
                        TmpVideoMemorySize,
                        wined3d_settings.emulated_textureram);
            }
            else
                ERR("VideoMemorySize is %i but must be >0\n", TmpVideoMemorySize);
        }
        if ( !get_config_key( hkey, appkey, "WineLogo", buffer, size) )
        {
            size_t len = strlen(buffer) + 1;

            wined3d_settings.logo = HeapAlloc(GetProcessHeap(), 0, len);
            if (!wined3d_settings.logo) ERR("Failed to allocate logo path memory.\n");
            else memcpy(wined3d_settings.logo, buffer, len);
        }
        if ( !get_config_key( hkey, appkey, "Multisampling", buffer, size) )
        {
            if (!strcmp(buffer,"enabled"))
            {
                TRACE("Allow multisampling\n");
                wined3d_settings.allow_multisampling = TRUE;
            }
        }
        if (!get_config_key(hkey, appkey, "StrictDrawOrdering", buffer, size)
                && !strcmp(buffer,"enabled"))
        {
            TRACE("Enforcing strict draw ordering.\n");
            wined3d_settings.strict_draw_ordering = TRUE;
        }
    }
    if (wined3d_settings.vs_mode == VS_HW)
        TRACE("Allow HW vertex shaders\n");
    if (wined3d_settings.ps_mode == PS_NONE)
        TRACE("Disable pixel shaders\n");
    if (wined3d_settings.glslRequested)
        TRACE("If supported by your system, GL Shading Language will be used\n");

    if (appkey) RegCloseKey( appkey );
    if (hkey) RegCloseKey( hkey );

    return TRUE;
}

static BOOL wined3d_dll_destroy(HINSTANCE hInstDLL)
{
    DWORD wined3d_context_tls_idx = context_get_tls_idx();
    unsigned int i;

    if (!TlsFree(wined3d_context_tls_idx))
    {
        DWORD err = GetLastError();
        ERR("Failed to free context TLS index, err %#x.\n", err);
    }

    for (i = 0; i < wndproc_table.count; ++i)
    {
        struct wined3d_wndproc *entry = &wndproc_table.entries[i];
        SetWindowLongPtrW(entry->window, GWLP_WNDPROC, (LONG_PTR)entry->proc);
    }
    HeapFree(GetProcessHeap(), 0, wndproc_table.entries);

    HeapFree(GetProcessHeap(), 0, wined3d_settings.logo);
    UnregisterClassA(WINED3D_OPENGL_WINDOW_CLASS_NAME, hInstDLL);

    return TRUE;
}

void WINAPI wined3d_mutex_lock(void)
{
    EnterCriticalSection(&wined3d_cs);
}

void WINAPI wined3d_mutex_unlock(void)
{
    LeaveCriticalSection(&wined3d_cs);
}

static struct wined3d_wndproc *wined3d_find_wndproc(HWND window)
{
    unsigned int i;

    for (i = 0; i < wndproc_table.count; ++i)
    {
        if (wndproc_table.entries[i].window == window)
        {
            return &wndproc_table.entries[i];
        }
    }

    return NULL;
}

static LRESULT CALLBACK wined3d_wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    struct wined3d_wndproc *entry;
    IWineD3DDeviceImpl *device;
    WNDPROC proc;

    wined3d_mutex_lock();
    entry = wined3d_find_wndproc(window);

    if (!entry)
    {
        wined3d_mutex_unlock();
        ERR("Window %p is not registered with wined3d.\n", window);
        return DefWindowProcW(window, message, wparam, lparam);
    }

    device = entry->device;
    proc = entry->proc;
    wined3d_mutex_unlock();

    return device_process_message(device, window, message, wparam, lparam, proc);
}

BOOL wined3d_register_window(HWND window, IWineD3DDeviceImpl *device)
{
    struct wined3d_wndproc *entry;

    wined3d_mutex_lock();

    if (wndproc_table.size == wndproc_table.count)
    {
        unsigned int new_size = max(1, wndproc_table.size * 2);
        struct wined3d_wndproc *new_entries;

        if (!wndproc_table.entries) new_entries = HeapAlloc(GetProcessHeap(), 0, new_size * sizeof(*new_entries));
        else new_entries = HeapReAlloc(GetProcessHeap(), 0, wndproc_table.entries, new_size * sizeof(*new_entries));

        if (!new_entries)
        {
            wined3d_mutex_unlock();
            ERR("Failed to grow table.\n");
            return FALSE;
        }

        wndproc_table.entries = new_entries;
        wndproc_table.size = new_size;
    }

    entry = &wndproc_table.entries[wndproc_table.count++];
    entry->window = window;
    entry->proc = (WNDPROC)SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR)wined3d_wndproc);
    entry->device = device;

    wined3d_mutex_unlock();

    return TRUE;
}

void wined3d_unregister_window(HWND window)
{
    unsigned int i;

    wined3d_mutex_lock();
    for (i = 0; i < wndproc_table.count; ++i)
    {
        if (wndproc_table.entries[i].window == window)
        {
            struct wined3d_wndproc *entry = &wndproc_table.entries[i];
            struct wined3d_wndproc *last = &wndproc_table.entries[--wndproc_table.count];

            if (GetWindowLongPtrW(window, GWLP_WNDPROC) == (LONG_PTR)wined3d_wndproc)
                SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR)entry->proc);
            if (entry != last) *entry = *last;
            wined3d_mutex_unlock();

            return;
        }
    }
    wined3d_mutex_unlock();

    ERR("Window %p is not registered with wined3d.\n", window);
}

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("WineD3D DLLMain Reason=%u\n", fdwReason);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            return wined3d_dll_init(hInstDLL);

        case DLL_PROCESS_DETACH:
            return wined3d_dll_destroy(hInstDLL);

        case DLL_THREAD_DETACH:
        {
            if (!context_set_current(NULL))
            {
                ERR("Failed to clear current context.\n");
            }
            return TRUE;
        }

        default:
            return TRUE;
    }
}
