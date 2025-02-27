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

#ifdef _WIN32
# define _WIN32_WINNT 0x0600    /* For InitOnceExecuteOnce(). */
#endif

#include "vkd3d_common.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "vkd3d_memory.h"

#define VKD3D_DEBUG_BUFFER_COUNT 64
#define VKD3D_DEBUG_BUFFER_SIZE 512

extern const char *const vkd3d_dbg_env_name;

static const char *const debug_level_names[] =
{
    [VKD3D_DBG_LEVEL_NONE ] =   "none",
    [VKD3D_DBG_LEVEL_MESSAGE] = "message",
    [VKD3D_DBG_LEVEL_ERR  ] =   "err",
    [VKD3D_DBG_LEVEL_FIXME] =   "fixme",
    [VKD3D_DBG_LEVEL_WARN ] =   "warn",
    [VKD3D_DBG_LEVEL_TRACE] =   "trace",
};

enum vkd3d_dbg_level vkd3d_dbg_get_level(void)
{
    static unsigned int level = ~0u;
    const char *vkd3d_debug;
    unsigned int i;

    if (level != ~0u)
        return level;

    if (!(vkd3d_debug = getenv(vkd3d_dbg_env_name)))
        vkd3d_debug = "";

    for (i = 0; i < ARRAY_SIZE(debug_level_names); ++i)
    {
        if (!strcmp(debug_level_names[i], vkd3d_debug))
        {
            level = i;
            return level;
        }
    }

    /* Default debug level. */
    level = VKD3D_DBG_LEVEL_FIXME;
    return level;
}

static PFN_vkd3d_log log_callback;

static void vkd3d_dbg_voutput(const char *fmt, va_list args)
{
    if (log_callback)
        log_callback(fmt, args);
    else
        vfprintf(stderr, fmt, args);
}

static void vkd3d_dbg_output(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vkd3d_dbg_voutput(fmt, args);
    va_end(args);
}

void vkd3d_dbg_printf(enum vkd3d_dbg_level level, const char *function, const char *fmt, ...)
{
    va_list args;

    if (vkd3d_dbg_get_level() < level)
        return;

#ifdef _WIN32
    vkd3d_dbg_output("vkd3d:%04lx:%s:%s ", GetCurrentThreadId(), debug_level_names[level], function);
#elif HAVE_GETTID
    vkd3d_dbg_output("vkd3d:%u:%s:%s ", gettid(), debug_level_names[level], function);
#else
    vkd3d_dbg_output("vkd3d:%s:%s ", debug_level_names[level], function);
#endif
    va_start(args, fmt);
    vkd3d_dbg_voutput(fmt, args);
    va_end(args);
}

void vkd3d_dbg_set_log_callback(PFN_vkd3d_log callback)
{
    log_callback = callback;
}

static char *get_buffer(void)
{
    static char buffers[VKD3D_DEBUG_BUFFER_COUNT][VKD3D_DEBUG_BUFFER_SIZE];
    static unsigned int buffer_index;
    unsigned int current_index;

    current_index = vkd3d_atomic_increment_u32(&buffer_index) % ARRAY_SIZE(buffers);
    return buffers[current_index];
}

const char *vkd3d_dbg_vsprintf(const char *fmt, va_list args)
{
    char *buffer;

    buffer = get_buffer();
    vsnprintf(buffer, VKD3D_DEBUG_BUFFER_SIZE, fmt, args);
    buffer[VKD3D_DEBUG_BUFFER_SIZE - 1] = '\0';
    return buffer;
}

const char *vkd3d_dbg_sprintf(const char *fmt, ...)
{
    const char *buffer;
    va_list args;

    va_start(args, fmt);
    buffer = vkd3d_dbg_vsprintf(fmt, args);
    va_end(args);
    return buffer;
}

static int get_escape_char(int c)
{
    switch (c)
    {
        case '"':
        case '\\':
            return c;
        case '\t':
            return 't';
        case '\n':
            return 'n';
        case '\r':
            return 'r';
        default:
            return 0;
    }
}

const char *debugstr_an(const char *str, size_t n)
{
    char *buffer, *ptr;
    int escape_char;
    char c;

    if (!str)
        return "(null)";
    if (n == SIZE_MAX)
        n = strlen(str);

    ptr = buffer = get_buffer();

    *ptr++ = '"';
    while (n-- && ptr <= buffer + VKD3D_DEBUG_BUFFER_SIZE - 8)
    {
        c = *str++;

        if ((escape_char = get_escape_char(c)))
        {
            *ptr++ = '\\';
            *ptr++ = escape_char;
            continue;
        }

        if (isprint(c))
        {
            *ptr++ = c;
        }
        else
        {
            *ptr++ = '\\';
            sprintf(ptr, "%02x", c);
            ptr += 2;
        }
    }
    *ptr++ = '"';

    if (++n)
    {
        *ptr++ = '.';
        *ptr++ = '.';
        *ptr++ = '.';
    }
    *ptr = '\0';

    return buffer;
}

const char *debugstr_a(const char *str)
{
    return debugstr_an(str, SIZE_MAX);
}

static const char *debugstr_w16(const uint16_t *wstr)
{
    char *buffer, *ptr;
    uint16_t c;

    if (!wstr)
        return "(null)";

    ptr = buffer = get_buffer();

    *ptr++ = '"';
    while ((c = *wstr++) && ptr <= buffer + VKD3D_DEBUG_BUFFER_SIZE - 10)
    {
        int escape_char = get_escape_char(c);

        if (escape_char)
        {
            *ptr++ = '\\';
            *ptr++ = escape_char;
            continue;
        }

        if (isprint(c))
        {
            *ptr++ = c;
        }
        else
        {
            *ptr++ = '\\';
            sprintf(ptr, "%04x", c);
            ptr += 4;
        }
    }
    *ptr++ = '"';

    if (c)
    {
        *ptr++ = '.';
        *ptr++ = '.';
        *ptr++ = '.';
    }
    *ptr = '\0';

    return buffer;
}

static const char *debugstr_w32(const uint32_t *wstr)
{
    char *buffer, *ptr;
    uint32_t c;

    if (!wstr)
        return "(null)";

    ptr = buffer = get_buffer();

    *ptr++ = '"';
    while ((c = *wstr++) && ptr <= buffer + VKD3D_DEBUG_BUFFER_SIZE - 10)
    {
        int escape_char = get_escape_char(c);

        if (escape_char)
        {
            *ptr++ = '\\';
            *ptr++ = escape_char;
            continue;
        }

        if (isprint(c))
        {
            *ptr++ = c;
        }
        else
        {
            *ptr++ = '\\';
            sprintf(ptr, "%04x", c);
            ptr += 4;
        }
    }
    *ptr++ = '"';

    if (c)
    {
        *ptr++ = '.';
        *ptr++ = '.';
        *ptr++ = '.';
    }
    *ptr = '\0';

    return buffer;
}

const char *debugstr_w(const WCHAR *wstr, size_t wchar_size)
{
    if (wchar_size == 2)
        return debugstr_w16((const uint16_t *)wstr);
    return debugstr_w32((const uint32_t *)wstr);
}

unsigned int vkd3d_env_var_as_uint(const char *name, unsigned int default_value)
{
    const char *value = getenv(name);
    unsigned long r;
    char *end_ptr;

    if (value)
    {
        errno = 0;
        r = strtoul(value, &end_ptr, 0);
        if (!errno && end_ptr != value)
            return min(r, UINT_MAX);
    }

    return default_value;
}

static bool is_option_separator(char c)
{
    return c == ',' || c == ';' || c == '\0';
}

bool vkd3d_debug_list_has_member(const char *string, const char *member)
{
    char prev_char, next_char;
    const char *p;

    p = string;
    while (p)
    {
        if ((p = strstr(p, member)))
        {
            prev_char = p > string ? p[-1] : 0;
            p += strlen(member);
            next_char = *p;

            if (is_option_separator(prev_char) && is_option_separator(next_char))
                return true;
        }
    }

    return false;
}

uint64_t vkd3d_parse_debug_options(const char *string,
        const struct vkd3d_debug_option *options, unsigned int option_count)
{
    uint64_t flags = 0;
    unsigned int i;

    for (i = 0; i < option_count; ++i)
    {
        const struct vkd3d_debug_option *opt = &options[i];

        if (vkd3d_debug_list_has_member(string, opt->name))
            flags |= opt->flag;
    }

    return flags;
}

#ifdef _WIN32

static HRESULT (WINAPI *pfn_SetThreadDescription)(HANDLE, const WCHAR *);
static BOOL WINAPI resolve_SetThreadDescription(INIT_ONCE *once, void *param, void **context)
{
    HMODULE kernelbase;

    if (!(kernelbase = LoadLibraryA("kernelbase.dll")))
        return TRUE;

    if (!(pfn_SetThreadDescription = (void *)GetProcAddress(kernelbase, "SetThreadDescription")))
        FreeLibrary(kernelbase);

    return TRUE;
}

#endif

void vkd3d_set_thread_name(const char *name)
{
#ifdef _WIN32
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;
    WCHAR *wname;
    int ret;

    InitOnceExecuteOnce(&init_once, resolve_SetThreadDescription, NULL, NULL);
    if (!pfn_SetThreadDescription)
        return;

    if ((ret = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0)) <= 0)
        return;

    if (!(wname = vkd3d_malloc(ret * sizeof(*wname))))
        return;

    if ((ret = MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, ret)) > 0)
        pfn_SetThreadDescription(GetCurrentThread(), wname);

    vkd3d_free(wname);
#elif defined(HAVE_PTHREAD_SETNAME_NP_2)
    pthread_setname_np(pthread_self(), name);
#elif defined(HAVE_PTHREAD_SETNAME_NP_1)
    pthread_setname_np(name);
#endif
}
