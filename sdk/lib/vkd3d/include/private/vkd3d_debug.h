/*
 * Copyright 2016 Józef Kucia for CodeWeavers
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

#ifndef __VKD3D_DEBUG_H
#define __VKD3D_DEBUG_H

#include "vkd3d_common.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef VKD3D_NO_TRACE_MESSAGES
#define TRACE(args...) do { } while (0)
#define TRACE_ON() (false)
#endif

#ifdef VKD3D_NO_DEBUG_MESSAGES
#define WARN(args...) do { } while (0)
#define FIXME(args...) do { } while (0)
#endif

enum vkd3d_dbg_level
{
    VKD3D_DBG_LEVEL_NONE,
    VKD3D_DBG_LEVEL_ERR,
    VKD3D_DBG_LEVEL_FIXME,
    VKD3D_DBG_LEVEL_WARN,
    VKD3D_DBG_LEVEL_TRACE,
};

enum vkd3d_dbg_level vkd3d_dbg_get_level(void);

void vkd3d_dbg_printf(enum vkd3d_dbg_level level, const char *function, const char *fmt, ...) VKD3D_PRINTF_FUNC(3, 4);
void vkd3d_dbg_set_log_callback(PFN_vkd3d_log callback);

const char *vkd3d_dbg_sprintf(const char *fmt, ...) VKD3D_PRINTF_FUNC(1, 2);
const char *vkd3d_dbg_vsprintf(const char *fmt, va_list args);
const char *debugstr_a(const char *str);
const char *debugstr_an(const char *str, size_t n);
const char *debugstr_w(const WCHAR *wstr, size_t wchar_size);

#define VKD3D_DBG_LOG(level) \
        do { \
        const enum vkd3d_dbg_level vkd3d_dbg_level = VKD3D_DBG_LEVEL_##level; \
        VKD3D_DBG_PRINTF

#define VKD3D_DBG_LOG_ONCE(first_time_level, level) \
        do { \
        static bool vkd3d_dbg_next_time; \
        const enum vkd3d_dbg_level vkd3d_dbg_level = vkd3d_dbg_next_time \
        ? VKD3D_DBG_LEVEL_##level : VKD3D_DBG_LEVEL_##first_time_level; \
        vkd3d_dbg_next_time = true; \
        VKD3D_DBG_PRINTF

#define VKD3D_DBG_PRINTF(...) \
        vkd3d_dbg_printf(vkd3d_dbg_level, __FUNCTION__, __VA_ARGS__); } while (0)

#ifndef TRACE
#define TRACE VKD3D_DBG_LOG(TRACE)
#endif

#ifndef WARN
#define WARN  VKD3D_DBG_LOG(WARN)
#endif

#ifndef FIXME
#define FIXME VKD3D_DBG_LOG(FIXME)
#endif

#define ERR   VKD3D_DBG_LOG(ERR)

#ifndef TRACE_ON
#define TRACE_ON() (vkd3d_dbg_get_level() == VKD3D_DBG_LEVEL_TRACE)
#endif

#ifndef WARN_ON
#define WARN_ON() (vkd3d_dbg_get_level() >= VKD3D_DBG_LEVEL_WARN)
#endif

#define FIXME_ONCE VKD3D_DBG_LOG_ONCE(FIXME, WARN)

#define VKD3D_DEBUG_ENV_NAME(name) const char *const vkd3d_dbg_env_name = name

static inline const char *debugstr_guid(const GUID *guid)
{
    if (!guid)
        return "(null)";

    return vkd3d_dbg_sprintf("{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            (unsigned long)guid->Data1, guid->Data2, guid->Data3, guid->Data4[0],
            guid->Data4[1], guid->Data4[2], guid->Data4[3], guid->Data4[4],
            guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

static inline const char *debugstr_hresult(HRESULT hr)
{
    switch (hr)
    {
#define TO_STR(u) case u: return #u;
        TO_STR(S_OK)
        TO_STR(S_FALSE)
        TO_STR(E_NOTIMPL)
        TO_STR(E_NOINTERFACE)
        TO_STR(E_POINTER)
        TO_STR(E_ABORT)
        TO_STR(E_FAIL)
        TO_STR(E_OUTOFMEMORY)
        TO_STR(E_INVALIDARG)
        TO_STR(DXGI_ERROR_NOT_FOUND)
        TO_STR(DXGI_ERROR_MORE_DATA)
        TO_STR(DXGI_ERROR_UNSUPPORTED)
#undef TO_STR
        default:
            return vkd3d_dbg_sprintf("%#x", (int)hr);
    }
}

unsigned int vkd3d_env_var_as_uint(const char *name, unsigned int default_value);

struct vkd3d_debug_option
{
    const char *name;
    uint64_t flag;
};

bool vkd3d_debug_list_has_member(const char *string, const char *member);
uint64_t vkd3d_parse_debug_options(const char *string,
        const struct vkd3d_debug_option *options, unsigned int option_count);
void vkd3d_set_thread_name(const char *name);

#endif  /* __VKD3D_DEBUG_H */
