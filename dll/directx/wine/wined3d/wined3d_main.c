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
#include "wine/port.h"

#include "initguid.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

struct wined3d_wndproc
{
    struct wined3d *wined3d;
    HWND window;
    BOOL unicode;
    BOOL filter;
    WNDPROC proc;
    struct wined3d_device *device;
    uint32_t flags;
};

struct wined3d_wndproc_table
{
    struct wined3d_wndproc *entries;
    SIZE_T count;
    SIZE_T size;
};

struct wined3d_window_hook
{
    HHOOK hook;
    DWORD thread_id;
    unsigned int count;
};

struct wined3d_hooked_swapchain
{
    struct wined3d_swapchain *swapchain;
    DWORD thread_id;
};

struct wined3d_hook_table
{
    struct wined3d_window_hook *hooks;
    SIZE_T hooks_size;
    SIZE_T hook_count;

    struct wined3d_hooked_swapchain *swapchains;
    SIZE_T swapchains_size;
    SIZE_T swapchain_count;
};

static struct wined3d_wndproc_table wndproc_table;
static struct wined3d_hook_table hook_table;

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
    TRUE,           /* Multithreaded CS by default. */
    MAKEDWORD_VERSION(4, 4), /* Default to OpenGL 4.4 */
    ORM_FBO,        /* Use FBOs to do offscreen rendering */
    PCI_VENDOR_NONE,/* PCI Vendor ID */
    PCI_DEVICE_NONE,/* PCI Device ID */
    0,              /* The default of memory is set in init_driver_info */
    NULL,           /* No wine logo by default */
    TRUE,           /* Prefer multisample textures to multisample renderbuffers. */
    ~0u,            /* Don't force a specific sample count by default. */
    FALSE,          /* Don't range check relative addressing indices in float constants. */
    FALSE,          /* No strict shader math by default. */
    0,              /* IEEE 0 * inf result. */
    ~0U,            /* No VS shader model limit by default. */
    ~0U,            /* No HS shader model limit by default. */
    ~0U,            /* No DS shader model limit by default. */
    ~0U,            /* No GS shader model limit by default. */
    ~0U,            /* No PS shader model limit by default. */
    ~0u,            /* No CS shader model limit by default. */
    WINED3D_RENDERER_AUTO,
    WINED3D_SHADER_BACKEND_AUTO,
};

struct wined3d * CDECL wined3d_create(DWORD flags)
{
    struct wined3d *object;
    HRESULT hr;

    if (!(object = heap_alloc_zero(FIELD_OFFSET(struct wined3d, adapters[1]))))
    {
        ERR("Failed to allocate wined3d object memory.\n");
        return NULL;
    }

    if (wined3d_settings.renderer == WINED3D_RENDERER_NO3D)
        flags |= WINED3D_NO3D;

    if (FAILED(hr = wined3d_init(object, flags)))
    {
        WARN("Failed to initialize wined3d object, hr %#x.\n", hr);
        heap_free(object);
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

static DWORD get_config_key_dword(HKEY defkey, HKEY appkey, const char *name, DWORD *value)
{
    DWORD type, data, size;

    size = sizeof(data);
    if (appkey && !RegQueryValueExA(appkey, name, 0, &type, (BYTE *)&data, &size) && type == REG_DWORD) goto success;
    size = sizeof(data);
    if (defkey && !RegQueryValueExA(defkey, name, 0, &type, (BYTE *)&data, &size) && type == REG_DWORD) goto success;

    return ERROR_FILE_NOT_FOUND;

success:
    *value = data;
    return 0;
}

BOOL wined3d_get_app_name(char *app_name, unsigned int app_name_size)
{
    char buffer[MAX_PATH];
    unsigned int len;
    char *p, *name;

    len = GetModuleFileNameA(0, buffer, ARRAY_SIZE(buffer));
    if (!(len && len < MAX_PATH))
        return FALSE;

    name = buffer;
    if ((p = strrchr(name, '/' )))
        name = p + 1;
    if ((p = strrchr(name, '\\')))
        name = p + 1;

    len = strlen(name) + 1;
    if (app_name_size < len)
        return FALSE;

    memcpy(app_name, name, len);
    return TRUE;
}

static BOOL wined3d_dll_init(HINSTANCE hInstDLL)
{
    DWORD wined3d_context_tls_idx;
    char buffer[MAX_PATH+10];
    DWORD size = sizeof(buffer);
    HKEY hkey = 0;
    HKEY appkey = 0;
    DWORD tmpvalue;
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

    if (wined3d_get_app_name(buffer, ARRAY_SIZE(buffer)))
    {
        HKEY tmpkey;
        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\Direct3D */
        if (!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey))
        {
            strcat(buffer, "\\Direct3D");
            TRACE("Application name %s.\n", buffer);
            if (RegOpenKeyA(tmpkey, buffer, &appkey)) appkey = 0;
            RegCloseKey(tmpkey);
        }
    }

    if (hkey || appkey)
    {
        if (!get_config_key_dword(hkey, appkey, "csmt", &wined3d_settings.cs_multithreaded))
            ERR_(winediag)("Setting multithreaded command stream to %#x.\n", wined3d_settings.cs_multithreaded);
        if (!get_config_key_dword(hkey, appkey, "MaxVersionGL", &tmpvalue))
        {
            ERR_(winediag)("Setting maximum allowed wined3d GL version to %u.%u.\n",
                    tmpvalue >> 16, tmpvalue & 0xffff);
            wined3d_settings.max_gl_version = tmpvalue;
        }
        if (!get_config_key(hkey, appkey, "shader_backend", buffer, size))
        {
            if (!_strnicmp(buffer, "glsl", -1))
            {
                ERR_(winediag)("Using the GLSL shader backend.\n");
                wined3d_settings.shader_backend = WINED3D_SHADER_BACKEND_GLSL;
            }
            else if (!_strnicmp(buffer, "arb", -1))
            {
                ERR_(winediag)("Using the ARB shader backend.\n");
                wined3d_settings.shader_backend = WINED3D_SHADER_BACKEND_ARB;
            }
            else if (!_strnicmp(buffer, "none", -1))
            {
                ERR_(winediag)("Disabling shader backends.\n");
                wined3d_settings.shader_backend = WINED3D_SHADER_BACKEND_NONE;
            }
        }
        else if (!get_config_key(hkey, appkey, "UseGLSL", buffer, size) && !strcmp(buffer, "disabled"))
        {
            wined3d_settings.shader_backend = WINED3D_SHADER_BACKEND_ARB;
        }
        if (wined3d_settings.shader_backend == WINED3D_SHADER_BACKEND_ARB
                || wined3d_settings.shader_backend == WINED3D_SHADER_BACKEND_NONE)
        {
            ERR_(winediag)("The GLSL shader backend has been disabled. You get to keep all the pieces if it breaks.\n");
            TRACE("Use of GL Shading Language disabled.\n");
        }
        if (!get_config_key(hkey, appkey, "OffscreenRenderingMode", buffer, size)
                && !strcmp(buffer,"backbuffer"))
            wined3d_settings.offscreen_rendering_mode = ORM_BACKBUFFER;
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

            if (!(wined3d_settings.logo = heap_alloc(len)))
                ERR("Failed to allocate logo path memory.\n");
            else
                memcpy(wined3d_settings.logo, buffer, len);
        }
        if (!get_config_key_dword(hkey, appkey, "MultisampleTextures", &wined3d_settings.multisample_textures))
            ERR_(winediag)("Setting multisample textures to %#x.\n", wined3d_settings.multisample_textures);
        if (!get_config_key_dword(hkey, appkey, "SampleCount", &wined3d_settings.sample_count))
            ERR_(winediag)("Forcing sample count to %u. This may not be compatible with all applications.\n",
                    wined3d_settings.sample_count);
        if (!get_config_key(hkey, appkey, "CheckFloatConstants", buffer, size)
                && !strcmp(buffer, "enabled"))
        {
            TRACE("Checking relative addressing indices in float constants.\n");
            wined3d_settings.check_float_constants = TRUE;
        }
        if (!get_config_key_dword(hkey, appkey, "strict_shader_math", &wined3d_settings.strict_shader_math))
            ERR_(winediag)("Setting strict shader math to %#x.\n", wined3d_settings.strict_shader_math);
        if (!get_config_key_dword(hkey, appkey, "multiply_special", &wined3d_settings.multiply_special))
            ERR_(winediag)("Setting multiply special to %#x.\n", wined3d_settings.multiply_special);
        if (!get_config_key_dword(hkey, appkey, "MaxShaderModelVS", &wined3d_settings.max_sm_vs))
            TRACE("Limiting VS shader model to %u.\n", wined3d_settings.max_sm_vs);
        if (!get_config_key_dword(hkey, appkey, "MaxShaderModelHS", &wined3d_settings.max_sm_hs))
            TRACE("Limiting HS shader model to %u.\n", wined3d_settings.max_sm_hs);
        if (!get_config_key_dword(hkey, appkey, "MaxShaderModelDS", &wined3d_settings.max_sm_ds))
            TRACE("Limiting DS shader model to %u.\n", wined3d_settings.max_sm_ds);
        if (!get_config_key_dword(hkey, appkey, "MaxShaderModelGS", &wined3d_settings.max_sm_gs))
            TRACE("Limiting GS shader model to %u.\n", wined3d_settings.max_sm_gs);
        if (!get_config_key_dword(hkey, appkey, "MaxShaderModelPS", &wined3d_settings.max_sm_ps))
            TRACE("Limiting PS shader model to %u.\n", wined3d_settings.max_sm_ps);
        if (!get_config_key_dword(hkey, appkey, "MaxShaderModelCS", &wined3d_settings.max_sm_cs))
            TRACE("Limiting CS shader model to %u.\n", wined3d_settings.max_sm_cs);
        if (!get_config_key(hkey, appkey, "renderer", buffer, size)
                || !get_config_key(hkey, appkey, "DirectDrawRenderer", buffer, size))
        {
            if (!strcmp(buffer, "vulkan"))
            {
                ERR_(winediag)("Using the Vulkan renderer.\n");
                wined3d_settings.renderer = WINED3D_RENDERER_VULKAN;
            }
            else if (!strcmp(buffer, "gl"))
            {
                ERR_(winediag)("Using the OpenGL renderer.\n");
                wined3d_settings.renderer = WINED3D_RENDERER_OPENGL;
            }
            else if (!strcmp(buffer, "gdi") || !strcmp(buffer, "no3d"))
            {
                ERR_(winediag)("Disabling 3D support.\n");
                wined3d_settings.renderer = WINED3D_RENDERER_NO3D;
            }
        }
    }

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
    heap_free(wndproc_table.entries);

    heap_free(hook_table.swapchains);
    for (i = 0; i < hook_table.hook_count; ++i)
    {
        WARN("Leftover hook table entry %p.\n", &hook_table.hooks[i]);
        UnhookWindowsHookEx(hook_table.hooks[i].hook);
    }
    heap_free(hook_table.hooks);

    heap_free(wined3d_settings.logo);
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

static struct wined3d_wndproc *wined3d_find_wndproc(HWND window, struct wined3d *wined3d)
{
    unsigned int i;

    for (i = 0; i < wndproc_table.count; ++i)
    {
        struct wined3d_wndproc *entry = &wndproc_table.entries[i];

        if (entry->window == window && entry->wined3d == wined3d)
            return entry;
    }

    return NULL;
}

BOOL wined3d_filter_messages(HWND window, BOOL filter)
{
    struct wined3d_wndproc *entry;
    BOOL ret;

    wined3d_wndproc_mutex_lock();

    if (!(entry = wined3d_find_wndproc(window, NULL)))
    {
        wined3d_wndproc_mutex_unlock();
        return FALSE;
    }

    ret = entry->filter;
    entry->filter = filter;

    wined3d_wndproc_mutex_unlock();

    return ret;
}

static LRESULT CALLBACK wined3d_wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    struct wined3d_wndproc *entry;
    struct wined3d_device *device;
    BOOL unicode, filter;
    WNDPROC proc;

    wined3d_wndproc_mutex_lock();

    if (!(entry = wined3d_find_wndproc(window, NULL)))
    {
        wined3d_wndproc_mutex_unlock();
        ERR("Window %p is not registered with wined3d.\n", window);
        return DefWindowProcW(window, message, wparam, lparam);
    }

    device = entry->device;
    unicode = entry->unicode;
    filter = entry->filter;
    proc = entry->proc;
    wined3d_wndproc_mutex_unlock();

    if (device)
    {
        if (filter && message != WM_DISPLAYCHANGE)
        {
            TRACE("Filtering message: window %p, message %#x, wparam %#lx, lparam %#lx.\n",
                    window, message, wparam, lparam);

            if (unicode)
                return DefWindowProcW(window, message, wparam, lparam);
            return DefWindowProcA(window, message, wparam, lparam);
        }

        return device_process_message(device, window, unicode, message, wparam, lparam, proc);
    }
    if (unicode)
        return CallWindowProcW(proc, window, message, wparam, lparam);
    return CallWindowProcA(proc, window, message, wparam, lparam);
}

static LRESULT CALLBACK wined3d_hook_proc(int code, WPARAM wparam, LPARAM lparam)
{
    struct wined3d_swapchain_desc swapchain_desc;
    struct wined3d_swapchain *swapchain;
    struct wined3d_wndproc *entry;
    MSG *msg = (MSG *)lparam;
    unsigned int i;

    /* Handle Alt+Enter. */
    if (code == HC_ACTION && msg->message == WM_SYSKEYDOWN
            && msg->wParam == VK_RETURN && (msg->lParam & (KF_ALTDOWN << 16)))
    {
        wined3d_wndproc_mutex_lock();

        for (i = 0; i < hook_table.swapchain_count; ++i)
        {
            swapchain = hook_table.swapchains[i].swapchain;

            if (swapchain->state.device_window != msg->hwnd)
                continue;

            if ((entry = wined3d_find_wndproc(msg->hwnd, swapchain->device->wined3d))
                    && (entry->flags & (WINED3D_REGISTER_WINDOW_NO_WINDOW_CHANGES
                    | WINED3D_REGISTER_WINDOW_NO_ALT_ENTER)))
                continue;

            wined3d_swapchain_get_desc(swapchain, &swapchain_desc);
            swapchain_desc.windowed = !swapchain_desc.windowed;
            wined3d_swapchain_state_set_fullscreen(&swapchain->state, &swapchain_desc,
                    swapchain->device->wined3d, swapchain->device->adapter->ordinal, NULL);

            wined3d_wndproc_mutex_unlock();

            return 1;
        }

        wined3d_wndproc_mutex_unlock();
    }

    return CallNextHookEx(0, code, wparam, lparam);
}

BOOL CDECL wined3d_register_window(struct wined3d *wined3d, HWND window,
        struct wined3d_device *device, unsigned int flags)
{
    struct wined3d_wndproc *entry;

    TRACE("wined3d %p, window %p, device %p, flags %#x.\n", wined3d, window, device, flags);

    wined3d_wndproc_mutex_lock();

    if ((entry = wined3d_find_wndproc(window, wined3d)))
    {
        if (!wined3d)
            WARN("Window %p is already registered with wined3d.\n", window);
        entry->flags = flags;
        wined3d_wndproc_mutex_unlock();
        return TRUE;
    }

    if (!wined3d_array_reserve((void **)&wndproc_table.entries, &wndproc_table.size,
            wndproc_table.count + 1, sizeof(*entry)))
    {
        wined3d_wndproc_mutex_unlock();
        ERR("Failed to grow table.\n");
        return FALSE;
    }

    entry = &wndproc_table.entries[wndproc_table.count++];
    entry->window = window;
    entry->unicode = IsWindowUnicode(window);
    if (!wined3d)
    {
        /* Set a window proc that matches the window. Some applications (e.g.
         * NoX) replace the window proc after we've set ours, and expect to be
         * able to call the previous one (ours) directly, without using
         * CallWindowProc(). */
        if (entry->unicode)
            entry->proc = (WNDPROC)SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR)wined3d_wndproc);
        else
            entry->proc = (WNDPROC)SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)wined3d_wndproc);
    }
    else
    {
        entry->proc = NULL;
    }
    entry->device = device;
    entry->wined3d = wined3d;
    entry->flags = flags;

    wined3d_wndproc_mutex_unlock();

    return TRUE;
}

static BOOL restore_wndproc(struct wined3d_wndproc *entry)
{
    LONG_PTR proc;

    if (entry->unicode)
    {
        proc = GetWindowLongPtrW(entry->window, GWLP_WNDPROC);
        if (proc != (LONG_PTR)wined3d_wndproc)
            return FALSE;
        SetWindowLongPtrW(entry->window, GWLP_WNDPROC, (LONG_PTR)entry->proc);
    }
    else
    {
        proc = GetWindowLongPtrA(entry->window, GWLP_WNDPROC);
        if (proc != (LONG_PTR)wined3d_wndproc)
            return FALSE;
        SetWindowLongPtrA(entry->window, GWLP_WNDPROC, (LONG_PTR)entry->proc);
    }

    return TRUE;
}

void wined3d_unregister_window(HWND window)
{
    struct wined3d_wndproc *entry, *last;

    wined3d_wndproc_mutex_lock();

    if (!(entry = wined3d_find_wndproc(window, NULL)))
    {
        wined3d_wndproc_mutex_unlock();
        ERR("Window %p is not registered with wined3d.\n", window);
        return;
    }

    if (entry->proc && !restore_wndproc(entry))
    {
        entry->device = NULL;
        WARN("Not unregistering window %p, current window proc doesn't match wined3d window proc.\n", window);
        wined3d_wndproc_mutex_unlock();
        return;
    }

    last = &wndproc_table.entries[--wndproc_table.count];
    if (entry != last) *entry = *last;

    wined3d_wndproc_mutex_unlock();
}

void CDECL wined3d_unregister_windows(struct wined3d *wined3d)
{
    struct wined3d_wndproc *entry, *last;
    unsigned int i = 0;

    TRACE("wined3d %p.\n", wined3d);

    wined3d_wndproc_mutex_lock();

    while (i < wndproc_table.count)
    {
        entry = &wndproc_table.entries[i];

        if (entry->wined3d != wined3d)
        {
            ++i;
            continue;
        }

        if (entry->proc && !restore_wndproc(entry))
        {
            entry->device = NULL;
            WARN("Not unregistering window %p, current window proc doesn't match wined3d window proc.\n",
                    entry->window);
            ++i;
            continue;
        }

        last = &wndproc_table.entries[--wndproc_table.count];
        if (entry != last)
            *entry = *last;
        else
            ++i;
    }

    wined3d_wndproc_mutex_unlock();
}

static struct wined3d_window_hook *wined3d_find_hook(DWORD thread_id)
{
    unsigned int i;

    for (i = 0; i < hook_table.hook_count; ++i)
    {
        if (hook_table.hooks[i].thread_id == thread_id)
            return &hook_table.hooks[i];
    }

    return NULL;
}

void wined3d_hook_swapchain(struct wined3d_swapchain *swapchain)
{
    struct wined3d_hooked_swapchain *swapchain_entry;
    struct wined3d_window_hook *hook;

    wined3d_wndproc_mutex_lock();

    if (!wined3d_array_reserve((void **)&hook_table.swapchains, &hook_table.swapchains_size,
            hook_table.swapchain_count + 1, sizeof(*swapchain_entry)))
    {
        wined3d_wndproc_mutex_unlock();
        return;
    }

    swapchain_entry = &hook_table.swapchains[hook_table.swapchain_count++];
    swapchain_entry->swapchain = swapchain;
    swapchain_entry->thread_id = GetWindowThreadProcessId(swapchain->state.device_window, NULL);

    if ((hook = wined3d_find_hook(swapchain_entry->thread_id)))
    {
        ++hook->count;
        wined3d_wndproc_mutex_unlock();
        return;
    }

    if (!wined3d_array_reserve((void **)&hook_table.hooks, &hook_table.hooks_size,
            hook_table.hook_count + 1, sizeof(*hook)))
    {
        --hook_table.swapchain_count;
        wined3d_wndproc_mutex_unlock();
        return;
    }

    hook = &hook_table.hooks[hook_table.hook_count++];
    hook->thread_id = swapchain_entry->thread_id;
    hook->hook = SetWindowsHookExW(WH_GETMESSAGE, wined3d_hook_proc, 0, hook->thread_id);
    hook->count = 1;

    wined3d_wndproc_mutex_unlock();
}

void wined3d_unhook_swapchain(struct wined3d_swapchain *swapchain)
{
    struct wined3d_hooked_swapchain *swapchain_entry, *last_swapchain_entry;
    struct wined3d_window_hook *hook, *last_hook;
    unsigned int i;

    wined3d_wndproc_mutex_lock();

    for (i = 0; i < hook_table.swapchain_count; ++i)
    {
        swapchain_entry = &hook_table.swapchains[i];

        if (swapchain_entry->swapchain != swapchain)
            continue;

        if ((hook = wined3d_find_hook(swapchain_entry->thread_id)) && !--hook->count)
        {
            UnhookWindowsHookEx(hook->hook);
            last_hook = &hook_table.hooks[--hook_table.hook_count];
            if (hook != last_hook)
                *hook = *last_hook;
        }

        last_swapchain_entry = &hook_table.swapchains[--hook_table.swapchain_count];
        if (swapchain_entry != last_swapchain_entry)
            *swapchain_entry = *last_swapchain_entry;

        break;
    }

    wined3d_wndproc_mutex_unlock();
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
            if (!wined3d_context_gl_set_current(NULL))
            {
                ERR("Failed to clear current context.\n");
            }
            return TRUE;
    }
    return TRUE;
}
