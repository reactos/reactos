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

#include "wined3d_private.h"

#include <winreg.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

struct wined3d_wndproc
{
    HWND window;
    BOOL unicode;
    WNDPROC proc;
    struct wined3d_device *device;
};

struct wined3d_wndproc_table
{
    struct wined3d_wndproc *entries;
    unsigned int count;
    unsigned int size;
};

static struct wined3d_wndproc_table wndproc_table;

static CRITICAL_SECTION wined3d_cs;
static CRITICAL_SECTION_DEBUG wined3d_cs_debug =
{
    0, 0, &wined3d_cs,
    {&wined3d_cs_debug.ProcessLocksList,
    &wined3d_cs_debug.ProcessLocksList},
    0, 0, {(DWORD_PTR)(__FILE__ ": wined3d_cs")}
};
static CRITICAL_SECTION wined3d_cs = {&wined3d_cs_debug, -1, 0, 0, 0, 0};

static CRITICAL_SECTION wined3d_wndproc_cs;
static CRITICAL_SECTION_DEBUG wined3d_wndproc_cs_debug =
{
    0, 0, &wined3d_wndproc_cs,
    {&wined3d_wndproc_cs_debug.ProcessLocksList,
    &wined3d_wndproc_cs_debug.ProcessLocksList},
    0, 0, {(DWORD_PTR)(__FILE__ ": wined3d_wndproc_cs")}
};
static CRITICAL_SECTION wined3d_wndproc_cs = {&wined3d_wndproc_cs_debug, -1, 0, 0, 0, 0};

/* When updating default value here, make sure to update winecfg as well,
 * where appropriate. */
struct wined3d_settings wined3d_settings =
{
    MAKEDWORD_VERSION(1, 0), /* Default to legacy OpenGL */
    TRUE,           /* Use of GLSL enabled by default */
    ORM_FBO,        /* Use FBOs to do offscreen rendering */
    PCI_VENDOR_NONE,/* PCI Vendor ID */
    PCI_DEVICE_NONE,/* PCI Device ID */
    0,              /* Multisampling AA Quality Levels */
    0,              /* The default of memory is set in init_driver_info */
    NULL,           /* No wine logo by default */
    TRUE,           /* Multisampling enabled by default. */
    FALSE,          /* No strict draw ordering. */
    TRUE,           /* Don't try to render onscreen by default. */
    ~0U,            /* No VS shader model limit by default. */
    ~0U,            /* No GS shader model limit by default. */
    ~0U,            /* No PS shader model limit by default. */
    FALSE,          /* 3D support enabled by default. */
#if defined(STAGING_CSMT)
    TRUE,           /* Multithreaded CS by default. */
    FALSE,          /* Do not ignore render target maps. */
#endif /* STAGING_CSMT */
};

struct wined3d * CDECL wined3d_create(DWORD flags)
{
    struct wined3d *object;
    HRESULT hr;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET(struct wined3d, adapters[1]));
    if (!object)
    {
        ERR("Failed to allocate wined3d object memory.\n");
        return NULL;
    }

    if (wined3d_settings.no_3d)
        flags |= WINED3D_NO3D;

    hr = wined3d_init(object, flags);
    if (FAILED(hr))
    {
        WARN("Failed to initialize wined3d object, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return NULL;
    }

    TRACE("Created wined3d object %p.\n", object);

    return object;
}

static DWORD get_config_key(HKEY defkey, HKEY appkey, const char *name, char *buffer, DWORD size)
{
    if (appkey && !RegQueryValueExA(appkey, name, 0, NULL, (BYTE *)buffer, &size)) return 0;
    if (defkey && !RegQueryValueExA(defkey, name, 0, NULL, (BYTE *)buffer, &size)) return 0;
    return ERROR_FILE_NOT_FOUND;
}

static DWORD get_config_key_dword(HKEY defkey, HKEY appkey, const char *name, DWORD *data)
{
    DWORD type;
    DWORD size = sizeof(DWORD);
    if (appkey && !RegQueryValueExA(appkey, name, 0, &type, (BYTE *)data, &size) && (type == REG_DWORD)) return 0;
    if (defkey && !RegQueryValueExA(defkey, name, 0, &type, (BYTE *)data, &size) && (type == REG_DWORD)) return 0;
    return ERROR_FILE_NOT_FOUND;
}

static BOOL wined3d_dll_init(HINSTANCE hInstDLL)
{
    DWORD wined3d_context_tls_idx;
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
    wc.hIcon                = LoadIconA(NULL, (const char *)IDI_WINLOGO);
    wc.hCursor              = LoadCursorA(NULL, (const char *)IDC_ARROW);
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

    if (hkey || appkey)
    {
        if (!get_config_key_dword(hkey, appkey, "MaxVersionGL", &tmpvalue))
        {
            if (tmpvalue != wined3d_settings.max_gl_version)
            {
                ERR_(winediag)("Setting maximum allowed wined3d GL version to %u.%u.\n",
                        tmpvalue >> 16, tmpvalue & 0xffff);
                wined3d_settings.max_gl_version = tmpvalue;
            }
        }
        if ( !get_config_key( hkey, appkey, "UseGLSL", buffer, size) )
        {
            if (!strcmp(buffer,"disabled"))
            {
                ERR_(winediag)("The GLSL shader backend has been disabled. You get to keep all the pieces if it breaks.\n");
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
        if (!get_config_key( hkey, appkey, "MultisamplingAAQualityLevels", buffer, size ))
        {
            int quality_levels = atoi(buffer);
            if(quality_levels > 0)
            {
                wined3d_settings.msaa_quality_levels = quality_levels;
                TRACE("Setting MultisamplingAAQualityLevels to %i\n", quality_levels);
            }
            else
                ERR("MultisamplingAAQualityLevels is %i but must be >0\n", quality_levels);
        }
        if ( !get_config_key( hkey, appkey, "VideoMemorySize", buffer, size) )
        {
            int TmpVideoMemorySize = atoi(buffer);
            if(TmpVideoMemorySize > 0)
            {
                wined3d_settings.emulated_textureram = (UINT64)TmpVideoMemorySize *1024*1024;
                TRACE("Use %iMiB = 0x%s bytes for emulated_textureram\n",
                        TmpVideoMemorySize,
                        wine_dbgstr_longlong(wined3d_settings.emulated_textureram));
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
            if (!strcmp(buffer, "disabled"))
            {
                TRACE("Multisampling disabled.\n");
                wined3d_settings.allow_multisampling = FALSE;
            }
        }
        if (!get_config_key(hkey, appkey, "StrictDrawOrdering", buffer, size)
                && !strcmp(buffer,"enabled"))
        {
            TRACE("Enforcing strict draw ordering.\n");
            wined3d_settings.strict_draw_ordering = TRUE;
        }
        if (!get_config_key(hkey, appkey, "AlwaysOffscreen", buffer, size)
                && !strcmp(buffer,"disabled"))
        {
            TRACE("Not always rendering backbuffers offscreen.\n");
            wined3d_settings.always_offscreen = FALSE;
        }
        if (!get_config_key_dword(hkey, appkey, "MaxShaderModelVS", &wined3d_settings.max_sm_vs))
            TRACE("Limiting VS shader model to %u.\n", wined3d_settings.max_sm_vs);
        if (!get_config_key_dword(hkey, appkey, "MaxShaderModelGS", &wined3d_settings.max_sm_gs))
            TRACE("Limiting GS shader model to %u.\n", wined3d_settings.max_sm_gs);
        if (!get_config_key_dword(hkey, appkey, "MaxShaderModelPS", &wined3d_settings.max_sm_ps))
            TRACE("Limiting PS shader model to %u.\n", wined3d_settings.max_sm_ps);
        if (!get_config_key(hkey, appkey, "DirectDrawRenderer", buffer, size)
                && !strcmp(buffer, "gdi"))
        {
            TRACE("Disabling 3D support.\n");
            wined3d_settings.no_3d = TRUE;
        }
#if defined(STAGING_CSMT)
        if (!get_config_key(hkey, appkey, "CSMT", buffer, size)
                && !strcmp(buffer,"disabled"))
        {
            TRACE("Disabling multithreaded command stream.\n");
            wined3d_settings.cs_multithreaded = FALSE;
        }
        if (!get_config_key(hkey, appkey, "ignore_rt_map", buffer, size)
                && !strcmp(buffer,"enabled"))
        {
            TRACE("Ignoring render target maps.\n");
            wined3d_settings.ignore_rt_map = TRUE;
        }
    }

    FIXME_(winediag)("Experimental wined3d CSMT feature is currently %s.\n",
        wined3d_settings.cs_multithreaded ? "enabled" : "disabled");
#else  /* STAGING_CSMT */
    }
#endif /* STAGING_CSMT */

    if (appkey) RegCloseKey( appkey );
    if (hkey) RegCloseKey( hkey );

    wined3d_dxtn_init();

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
        /* Trying to unregister these would be futile. These entries can only
         * exist if either we skipped them in wined3d_unregister_window() due
         * to the application replacing the wndproc after the entry was
         * registered, or if the application still has an active wined3d
         * device. In the latter case the application has bigger problems than
         * these entries. */
        WARN("Leftover wndproc table entry %p.\n", &wndproc_table.entries[i]);
    }
    HeapFree(GetProcessHeap(), 0, wndproc_table.entries);

    HeapFree(GetProcessHeap(), 0, wined3d_settings.logo);
    UnregisterClassA(WINED3D_OPENGL_WINDOW_CLASS_NAME, hInstDLL);

    DeleteCriticalSection(&wined3d_wndproc_cs);
    DeleteCriticalSection(&wined3d_cs);

    wined3d_dxtn_free();

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

static void wined3d_wndproc_mutex_lock(void)
{
    EnterCriticalSection(&wined3d_wndproc_cs);
}

static void wined3d_wndproc_mutex_unlock(void)
{
    LeaveCriticalSection(&wined3d_wndproc_cs);
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
    struct wined3d_device *device;
    BOOL unicode;
    WNDPROC proc;

    wined3d_wndproc_mutex_lock();
    entry = wined3d_find_wndproc(window);

    if (!entry)
    {
        wined3d_wndproc_mutex_unlock();
        ERR("Window %p is not registered with wined3d.\n", window);
        return DefWindowProcW(window, message, wparam, lparam);
    }

    device = entry->device;
    unicode = entry->unicode;
    proc = entry->proc;
    wined3d_wndproc_mutex_unlock();

    if (device)
        return device_process_message(device, window, unicode, message, wparam, lparam, proc);
    if (unicode)
        return CallWindowProcW(proc, window, message, wparam, lparam);
    return CallWindowProcA(proc, window, message, wparam, lparam);
}

BOOL wined3d_register_window(HWND window, struct wined3d_device *device)
{
    struct wined3d_wndproc *entry;

    wined3d_wndproc_mutex_lock();

    if (wined3d_find_wndproc(window))
    {
        wined3d_wndproc_mutex_unlock();
        WARN("Window %p is already registered with wined3d.\n", window);
        return TRUE;
    }

    if (wndproc_table.size == wndproc_table.count)
    {
        unsigned int new_size = max(1, wndproc_table.size * 2);
        struct wined3d_wndproc *new_entries;

        if (!wndproc_table.entries) new_entries = HeapAlloc(GetProcessHeap(), 0, new_size * sizeof(*new_entries));
        else new_entries = HeapReAlloc(GetProcessHeap(), 0, wndproc_table.entries, new_size * sizeof(*new_entries));

        if (!new_entries)
        {
            wined3d_wndproc_mutex_unlock();
            ERR("Failed to grow table.\n");
            return FALSE;
        }

        wndproc_table.entries = new_entries;
        wndproc_table.size = new_size;
    }

    entry = &wndproc_table.entries[wndproc_table.count++];
    entry->window = window;
    entry->unicode = IsWindowUnicode(window);
    /* Set a window proc that matches the window. Some applications (e.g. NoX)
     * replace the window proc after we've set ours, and expect to be able to
     * call the previous one (ours) directly, without using CallWindowProc(). */
    if (entry->unicode)
        entry->proc = (WNDPROC)SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR)wined3d_wndproc);
    else
        entry->proc = (WNDPROC)SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)wined3d_wndproc);
    entry->device = device;

    wined3d_wndproc_mutex_unlock();

    return TRUE;
}

void wined3d_unregister_window(HWND window)
{
    struct wined3d_wndproc *entry, *last;
    LONG_PTR proc;

    wined3d_wndproc_mutex_lock();

    if (!(entry = wined3d_find_wndproc(window)))
    {
        wined3d_wndproc_mutex_unlock();
        ERR("Window %p is not registered with wined3d.\n", window);
        return;
    }

    if (entry->unicode)
    {
        proc = GetWindowLongPtrW(window, GWLP_WNDPROC);
        if (proc != (LONG_PTR)wined3d_wndproc)
        {
            entry->device = NULL;
            wined3d_wndproc_mutex_unlock();
            WARN("Not unregistering window %p, window proc %#lx doesn't match wined3d window proc %p.\n",
                    window, proc, wined3d_wndproc);
            return;
        }

        SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR)entry->proc);
    }
    else
    {
        proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
        if (proc != (LONG_PTR)wined3d_wndproc)
        {
            entry->device = NULL;
            wined3d_wndproc_mutex_unlock();
            WARN("Not unregistering window %p, window proc %#lx doesn't match wined3d window proc %p.\n",
                    window, proc, wined3d_wndproc);
            return;
        }

        SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)entry->proc);
    }

    last = &wndproc_table.entries[--wndproc_table.count];
    if (entry != last) *entry = *last;

    wined3d_wndproc_mutex_unlock();
}

void wined3d_strictdrawing_set(int value)
{
    wined3d_settings.strict_draw_ordering = value;
}

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            return wined3d_dll_init(inst);

        case DLL_PROCESS_DETACH:
            if (!reserved)
                return wined3d_dll_destroy(inst);
            break;

        case DLL_THREAD_DETACH:
            if (!context_set_current(NULL))
            {
                ERR("Failed to clear current context.\n");
            }
            return TRUE;
    }
    return TRUE;
}
