/*
 * Copyright 2018 Nikolay Sivov
 * Copyright 2018 Zhiyi Zhang
 * Copyright 2021-2023 Zebediah Figura
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

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <wchar.h>

#include "windef.h"
#include "winbase.h"
#include "pathcch.h"
#include "strsafe.h"
#include "shlwapi.h"
#include "wininet.h"
#include "intshcut.h"
#include "winternl.h"

#include "kernelbase.h"
#include "wine/exception.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(path);

static const char hexDigits[] = "0123456789ABCDEF";

static const unsigned char hashdata_lookup[256] =
{
    0x01, 0x0e, 0x6e, 0x19, 0x61, 0xae, 0x84, 0x77, 0x8a, 0xaa, 0x7d, 0x76, 0x1b, 0xe9, 0x8c, 0x33,
    0x57, 0xc5, 0xb1, 0x6b, 0xea, 0xa9, 0x38, 0x44, 0x1e, 0x07, 0xad, 0x49, 0xbc, 0x28, 0x24, 0x41,
    0x31, 0xd5, 0x68, 0xbe, 0x39, 0xd3, 0x94, 0xdf, 0x30, 0x73, 0x0f, 0x02, 0x43, 0xba, 0xd2, 0x1c,
    0x0c, 0xb5, 0x67, 0x46, 0x16, 0x3a, 0x4b, 0x4e, 0xb7, 0xa7, 0xee, 0x9d, 0x7c, 0x93, 0xac, 0x90,
    0xb0, 0xa1, 0x8d, 0x56, 0x3c, 0x42, 0x80, 0x53, 0x9c, 0xf1, 0x4f, 0x2e, 0xa8, 0xc6, 0x29, 0xfe,
    0xb2, 0x55, 0xfd, 0xed, 0xfa, 0x9a, 0x85, 0x58, 0x23, 0xce, 0x5f, 0x74, 0xfc, 0xc0, 0x36, 0xdd,
    0x66, 0xda, 0xff, 0xf0, 0x52, 0x6a, 0x9e, 0xc9, 0x3d, 0x03, 0x59, 0x09, 0x2a, 0x9b, 0x9f, 0x5d,
    0xa6, 0x50, 0x32, 0x22, 0xaf, 0xc3, 0x64, 0x63, 0x1a, 0x96, 0x10, 0x91, 0x04, 0x21, 0x08, 0xbd,
    0x79, 0x40, 0x4d, 0x48, 0xd0, 0xf5, 0x82, 0x7a, 0x8f, 0x37, 0x69, 0x86, 0x1d, 0xa4, 0xb9, 0xc2,
    0xc1, 0xef, 0x65, 0xf2, 0x05, 0xab, 0x7e, 0x0b, 0x4a, 0x3b, 0x89, 0xe4, 0x6c, 0xbf, 0xe8, 0x8b,
    0x06, 0x18, 0x51, 0x14, 0x7f, 0x11, 0x5b, 0x5c, 0xfb, 0x97, 0xe1, 0xcf, 0x15, 0x62, 0x71, 0x70,
    0x54, 0xe2, 0x12, 0xd6, 0xc7, 0xbb, 0x0d, 0x20, 0x5e, 0xdc, 0xe0, 0xd4, 0xf7, 0xcc, 0xc4, 0x2b,
    0xf9, 0xec, 0x2d, 0xf4, 0x6f, 0xb6, 0x99, 0x88, 0x81, 0x5a, 0xd9, 0xca, 0x13, 0xa5, 0xe7, 0x47,
    0xe6, 0x8e, 0x60, 0xe3, 0x3e, 0xb3, 0xf6, 0x72, 0xa2, 0x35, 0xa0, 0xd7, 0xcd, 0xb4, 0x2f, 0x6d,
    0x2c, 0x26, 0x1f, 0x95, 0x87, 0x00, 0xd8, 0x34, 0x3f, 0x17, 0x25, 0x45, 0x27, 0x75, 0x92, 0xb8,
    0xa3, 0xc8, 0xde, 0xeb, 0xf8, 0xf3, 0xdb, 0x0a, 0x98, 0x83, 0x7b, 0xe5, 0xcb, 0x4c, 0x78, 0xd1,
};

struct parsed_url
{
    const WCHAR *scheme;   /* [out] start of scheme                     */
    DWORD scheme_len;      /* [out] size of scheme (until colon)        */
    const WCHAR *username; /* [out] start of Username                   */
    DWORD username_len;    /* [out] size of Username (until ":" or "@") */
    const WCHAR *password; /* [out] start of Password                   */
    DWORD password_len;    /* [out] size of Password (until "@")        */
    const WCHAR *hostname; /* [out] start of Hostname                   */
    DWORD hostname_len;    /* [out] size of Hostname (until ":" or "/") */
    const WCHAR *port;     /* [out] start of Port                       */
    DWORD port_len;        /* [out] size of Port (until "/" or eos)     */
    const WCHAR *query;    /* [out] start of Query                      */
    DWORD query_len;       /* [out] size of Query (until eos)           */
    DWORD scheme_number;
};

static WCHAR *heap_strdupAtoW(const char *str)
{
    WCHAR *ret = NULL;

    if (str)
    {
        DWORD len;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = heap_alloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

static bool array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    unsigned int new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return true;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return false;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = heap_realloc( *elements, new_capacity * size )))
        return false;

    *elements = new_elements;
    *capacity = new_capacity;

    return true;
}

static bool is_slash( char c )
{
    return c == '/' || c == '\\';
}

static BOOL is_drive_specA( const char *str )
{
    return isalpha( str[0] ) && str[1] == ':';
}

static BOOL is_drive_spec( const WCHAR *str )
{
    return isalpha( str[0] ) && str[1] == ':';
}

static BOOL is_escaped_drive_spec( const WCHAR *str )
{
    return isalpha( str[0] ) && (str[1] == ':' || str[1] == '|');
}

static BOOL is_prefixed_unc(const WCHAR *string)
{
    return !wcsnicmp(string, L"\\\\?\\UNC\\", 8 );
}

static BOOL is_prefixed_disk(const WCHAR *string)
{
    return !wcsncmp(string, L"\\\\?\\", 4) && is_drive_spec( string + 4 );
}

static BOOL is_prefixed_volume(const WCHAR *string)
{
    const WCHAR *guid;
    INT i = 0;

    if (wcsnicmp( string, L"\\\\?\\Volume", 10 )) return FALSE;

    guid = string + 10;

    while (i <= 37)
    {
        switch (i)
        {
        case 0:
            if (guid[i] != '{') return FALSE;
            break;
        case 9:
        case 14:
        case 19:
        case 24:
            if (guid[i] != '-') return FALSE;
            break;
        case 37:
            if (guid[i] != '}') return FALSE;
            break;
        default:
            if (!isxdigit(guid[i])) return FALSE;
            break;
        }
        i++;
    }

    return TRUE;
}

/* Get the next character beyond end of the segment.
   Return TRUE if the last segment ends with a backslash */
static BOOL get_next_segment(const WCHAR *next, const WCHAR **next_segment)
{
    while (*next && *next != '\\') next++;
    if (*next == '\\')
    {
        *next_segment = next + 1;
        return TRUE;
    }
    else
    {
        *next_segment = next;
        return FALSE;
    }
}

/* Find the last character of the root in a path, if there is one, without any segments */
static const WCHAR *get_root_end(const WCHAR *path)
{
    /* Find path root */
    if (is_prefixed_volume(path))
        return path[48] == '\\' ? path + 48 : path + 47;
    else if (is_prefixed_unc(path))
        return path + 7;
    else if (is_prefixed_disk(path))
        return path[6] == '\\' ? path + 6 : path + 5;
    /* \\ */
    else if (path[0] == '\\' && path[1] == '\\')
        return path + 1;
    /* \ */
    else if (path[0] == '\\')
        return path;
    /* X:\ */
    else if (is_drive_spec( path ))
        return path[2] == '\\' ? path + 2 : path + 1;
    else
        return NULL;
}

HRESULT WINAPI PathAllocCanonicalize(const WCHAR *path_in, DWORD flags, WCHAR **path_out)
{
    WCHAR *buffer, *dst;
    const WCHAR *src;
    const WCHAR *root_end;
    SIZE_T buffer_size, length;

    TRACE("%s %#lx %p\n", debugstr_w(path_in), flags, path_out);

    if (!path_in || !path_out
        || ((flags & PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS) && (flags & PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS))
        || (flags & (PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS | PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS)
            && !(flags & PATHCCH_ALLOW_LONG_PATHS))
        || ((flags & PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH) && (flags & PATHCCH_ALLOW_LONG_PATHS)))
    {
        if (path_out) *path_out = NULL;
        return E_INVALIDARG;
    }

    length = lstrlenW(path_in);
    if ((length + 1 > MAX_PATH && !(flags & (PATHCCH_ALLOW_LONG_PATHS | PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH)))
        || (length + 1 > PATHCCH_MAX_CCH))
    {
        *path_out = NULL;
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }

    /* PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH implies PATHCCH_DO_NOT_NORMALIZE_SEGMENTS */
    if (flags & PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH) flags |= PATHCCH_DO_NOT_NORMALIZE_SEGMENTS;

    /* path length + possible \\?\ addition + possible \ addition + NUL */
    buffer_size = (length + 6) * sizeof(WCHAR);
    buffer = LocalAlloc(LMEM_ZEROINIT, buffer_size);
    if (!buffer)
    {
        *path_out = NULL;
        return E_OUTOFMEMORY;
    }

    src = path_in;
    dst = buffer;

    root_end = get_root_end(path_in);
    if (root_end) root_end = buffer + (root_end - path_in);

    /* Copy path root */
    if (root_end)
    {
        memcpy(dst, src, (root_end - buffer + 1) * sizeof(WCHAR));
        src += root_end - buffer + 1;
        if(PathCchStripPrefix(dst, length + 6) == S_OK)
        {
            /* Fill in \ in X:\ if the \ is missing */
            if (is_drive_spec( dst ) && dst[2]!= '\\')
            {
                dst[2] = '\\';
                dst[3] = 0;
            }
            dst = buffer + lstrlenW(buffer);
            root_end = dst;
        }
        else
            dst += root_end - buffer + 1;
    }

    while (*src)
    {
        if (src[0] == '.')
        {
            if (src[1] == '.')
            {
                /* Keep one . after * */
                if (dst > buffer && dst[-1] == '*')
                {
                    *dst++ = *src++;
                    continue;
                }

                /* Keep the .. if not surrounded by \ */
                if ((src[2] != '\\' && src[2]) || (dst > buffer && dst[-1] != '\\'))
                {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    continue;
                }

                /* Remove the \ before .. if the \ is not part of root */
                if (dst > buffer && dst[-1] == '\\' && (!root_end || dst - 1 > root_end))
                {
                    *--dst = '\0';
                    /* Remove characters until a \ is encountered */
                    while (dst > buffer)
                    {
                        if (dst[-1] == '\\')
                        {
                            *--dst = 0;
                            break;
                        }
                        else
                            *--dst = 0;
                    }
                }
                /* Remove the extra \ after .. if the \ before .. wasn't deleted */
                else if (src[2] == '\\')
                    src++;

                src += 2;
            }
            else
            {
                /* Keep the . if not surrounded by \ */
                if ((src[1] != '\\' && src[1]) || (dst > buffer && dst[-1] != '\\'))
                {
                    *dst++ = *src++;
                    continue;
                }

                /* Remove the \ before . if the \ is not part of root */
                if (dst > buffer && dst[-1] == '\\' && (!root_end || dst - 1 > root_end)) dst--;
                /* Remove the extra \ after . if the \ before . wasn't deleted */
                else if (src[1] == '\\')
                    src++;

                src++;
            }

            /* If X:\ is not complete, then complete it */
            if (is_drive_spec( buffer ) && buffer[2] != '\\')
            {
                root_end = buffer + 2;
                dst = buffer + 3;
                buffer[2] = '\\';
                /* If next character is \, use the \ to fill in */
                if (src[0] == '\\') src++;
            }
        }
        /* Copy over */
        else
            *dst++ = *src++;
    }
    /* End the path */
    *dst = 0;

    /* Strip multiple trailing . */
    if (!(flags & PATHCCH_DO_NOT_NORMALIZE_SEGMENTS))
    {
        while (dst > buffer && dst[-1] == '.')
        {
            /* Keep a . after * */
            if (dst - 1 > buffer && dst[-2] == '*')
                break;
            /* If . follow a : at the second character, remove the . and add a \ */
            else if (dst - 1 > buffer && dst[-2] == ':' && dst - 2 == buffer + 1)
                *--dst = '\\';
            else
                *--dst = 0;
        }
    }

    /* If result path is empty, fill in \ */
    if (!*buffer)
    {
        buffer[0] = '\\';
        buffer[1] = 0;
    }

    /* Extend the path if needed */
    length = lstrlenW(buffer);
    if (((length + 1 > MAX_PATH && is_drive_spec( buffer ))
         || (is_drive_spec( buffer ) && flags & PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH))
        && !(flags & PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS))
    {
        memmove(buffer + 4, buffer, (length + 1) * sizeof(WCHAR));
        buffer[0] = '\\';
        buffer[1] = '\\';
        buffer[2] = '?';
        buffer[3] = '\\';
    }

    /* Add a trailing backslash to the path if needed */
    if (flags & PATHCCH_ENSURE_TRAILING_SLASH)
        PathCchAddBackslash(buffer, buffer_size);

    *path_out = buffer;
    return S_OK;
}

HRESULT WINAPI PathAllocCombine(const WCHAR *path1, const WCHAR *path2, DWORD flags, WCHAR **out)
{
    SIZE_T combined_length, length2;
    WCHAR *combined_path;
    BOOL add_backslash = FALSE;
    HRESULT hr;

    TRACE("%s %s %#lx %p\n", wine_dbgstr_w(path1), wine_dbgstr_w(path2), flags, out);

    if ((!path1 && !path2) || !out)
    {
        if (out) *out = NULL;
        return E_INVALIDARG;
    }

    if (!path1 || !path2) return PathAllocCanonicalize(path1 ? path1 : path2, flags, out);

    /* If path2 is fully qualified, use path2 only */
    if (is_drive_spec( path2 ) || (path2[0] == '\\' && path2[1] == '\\'))
    {
        path1 = path2;
        path2 = NULL;
        add_backslash = (is_drive_spec(path1) && !path1[2])
                        || (is_prefixed_disk(path1) && !path1[6]);
    }

    length2 = path2 ? lstrlenW(path2) : 0;
    /* path1 length + path2 length + possible backslash + NULL */
    combined_length = lstrlenW(path1) + length2 + 2;

    combined_path = HeapAlloc(GetProcessHeap(), 0, combined_length * sizeof(WCHAR));
    if (!combined_path)
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    lstrcpyW(combined_path, path1);
    PathCchStripPrefix(combined_path, combined_length);
    if (add_backslash) PathCchAddBackslashEx(combined_path, combined_length, NULL, NULL);

    if (path2 && path2[0])
    {
        if (path2[0] == '\\' && path2[1] != '\\')
        {
            PathCchStripToRoot(combined_path, combined_length);
            path2++;
        }

        PathCchAddBackslashEx(combined_path, combined_length, NULL, NULL);
        lstrcatW(combined_path, path2);
    }

    hr = PathAllocCanonicalize(combined_path, flags, out);
    HeapFree(GetProcessHeap(), 0, combined_path);
    return hr;
}

HRESULT WINAPI PathCchAddBackslash(WCHAR *path, SIZE_T size)
{
    return PathCchAddBackslashEx(path, size, NULL, NULL);
}

HRESULT WINAPI PathCchAddBackslashEx(WCHAR *path, SIZE_T size, WCHAR **endptr, SIZE_T *remaining)
{
    BOOL needs_termination;
    SIZE_T length;

    TRACE("%s, %Iu, %p, %p\n", debugstr_w(path), size, endptr, remaining);

    length = lstrlenW(path);
    needs_termination = size && length && path[length - 1] != '\\';

    if (length >= (needs_termination ? size - 1 : size))
    {
        if (endptr) *endptr = NULL;
        if (remaining) *remaining = 0;
        return STRSAFE_E_INSUFFICIENT_BUFFER;
    }

    if (!needs_termination)
    {
        if (endptr) *endptr = path + length;
        if (remaining) *remaining = size - length;
        return S_FALSE;
    }

    path[length++] = '\\';
    path[length] = 0;

    if (endptr) *endptr = path + length;
    if (remaining) *remaining = size - length;

    return S_OK;
}

HRESULT WINAPI PathCchAddExtension(WCHAR *path, SIZE_T size, const WCHAR *extension)
{
    const WCHAR *existing_extension, *next;
    SIZE_T path_length, extension_length, dot_length;
    BOOL has_dot;
    HRESULT hr;

    TRACE("%s %Iu %s\n", wine_dbgstr_w(path), size, wine_dbgstr_w(extension));

    if (!path || !size || size > PATHCCH_MAX_CCH || !extension) return E_INVALIDARG;

    next = extension;
    while (*next)
    {
        if ((*next == '.' && next > extension) || *next == ' ' || *next == '\\') return E_INVALIDARG;
        next++;
    }

    has_dot = extension[0] == '.';

    hr = PathCchFindExtension(path, size, &existing_extension);
    if (FAILED(hr)) return hr;
    if (*existing_extension) return S_FALSE;

    path_length = wcsnlen(path, size);
    dot_length = has_dot ? 0 : 1;
    extension_length = lstrlenW(extension);

    if (path_length + dot_length + extension_length + 1 > size) return STRSAFE_E_INSUFFICIENT_BUFFER;

    /* If extension is empty or only dot, return S_OK with path unchanged */
    if (!extension[0] || (extension[0] == '.' && !extension[1])) return S_OK;

    if (!has_dot)
    {
        path[path_length] = '.';
        path_length++;
    }

    lstrcpyW(path + path_length, extension);
    return S_OK;
}

HRESULT WINAPI PathCchAppend(WCHAR *path1, SIZE_T size, const WCHAR *path2)
{
    TRACE("%s %Iu %s\n", wine_dbgstr_w(path1), size, wine_dbgstr_w(path2));

    return PathCchAppendEx(path1, size, path2, PATHCCH_NONE);
}

HRESULT WINAPI PathCchAppendEx(WCHAR *path1, SIZE_T size, const WCHAR *path2, DWORD flags)
{
    HRESULT hr;
    WCHAR *result;

    TRACE("%s %Iu %s %#lx\n", wine_dbgstr_w(path1), size, wine_dbgstr_w(path2), flags);

    if (!path1 || !size) return E_INVALIDARG;

    /* Create a temporary buffer for result because we need to keep path1 unchanged if error occurs.
     * And PathCchCombineEx writes empty result if there is error so we can't just use path1 as output
     * buffer for PathCchCombineEx */
    result = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
    if (!result) return E_OUTOFMEMORY;

    /* Avoid the single backslash behavior with PathCchCombineEx when appending */
    if (path2 && path2[0] == '\\' && path2[1] != '\\') path2++;

    hr = PathCchCombineEx(result, size, path1, path2, flags);
    if (SUCCEEDED(hr)) memcpy(path1, result, size * sizeof(WCHAR));

    HeapFree(GetProcessHeap(), 0, result);
    return hr;
}

HRESULT WINAPI PathCchCanonicalize(WCHAR *out, SIZE_T size, const WCHAR *in)
{
    TRACE("%p %Iu %s\n", out, size, wine_dbgstr_w(in));

    /* Not X:\ and path > MAX_PATH - 4, return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE) */
    if (lstrlenW(in) > MAX_PATH - 4 && !(is_drive_spec( in ) && in[2] == '\\'))
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);

    return PathCchCanonicalizeEx(out, size, in, PATHCCH_NONE);
}

HRESULT WINAPI PathCchCanonicalizeEx(WCHAR *out, SIZE_T size, const WCHAR *in, DWORD flags)
{
    WCHAR *buffer;
    SIZE_T length;
    HRESULT hr;

    TRACE("%p %Iu %s %#lx\n", out, size, wine_dbgstr_w(in), flags);

    if (!size) return E_INVALIDARG;

    hr = PathAllocCanonicalize(in, flags, &buffer);
    if (FAILED(hr)) return hr;

    length = lstrlenW(buffer);
    if (size < length + 1)
    {
        /* No root and path > MAX_PATH - 4, return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE) */
        if (length > MAX_PATH - 4 && !(in[0] == '\\' || (is_drive_spec( in ) && in[2] == '\\')))
            hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
        else
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
    }

    if (SUCCEEDED(hr))
    {
        memcpy(out, buffer, (length + 1) * sizeof(WCHAR));

        /* Fill a backslash at the end of X: */
        if (is_drive_spec( out ) && !out[2] && size > 3)
        {
            out[2] = '\\';
            out[3] = 0;
        }
    }

    LocalFree(buffer);
    return hr;
}

HRESULT WINAPI PathCchCombine(WCHAR *out, SIZE_T size, const WCHAR *path1, const WCHAR *path2)
{
    TRACE("%p %s %s\n", out, wine_dbgstr_w(path1), wine_dbgstr_w(path2));

    return PathCchCombineEx(out, size, path1, path2, PATHCCH_NONE);
}

HRESULT WINAPI PathCchCombineEx(WCHAR *out, SIZE_T size, const WCHAR *path1, const WCHAR *path2, DWORD flags)
{
    HRESULT hr;
    WCHAR *buffer;
    SIZE_T length;

    TRACE("%p %s %s %#lx\n", out, wine_dbgstr_w(path1), wine_dbgstr_w(path2), flags);

    if (!out || !size || size > PATHCCH_MAX_CCH) return E_INVALIDARG;

    hr = PathAllocCombine(path1, path2, flags, &buffer);
    if (FAILED(hr))
    {
        out[0] = 0;
        return hr;
    }

    length = lstrlenW(buffer);
    if (length + 1 > size)
    {
        out[0] = 0;
        LocalFree(buffer);
        return STRSAFE_E_INSUFFICIENT_BUFFER;
    }
    else
    {
        memcpy(out, buffer, (length + 1) * sizeof(WCHAR));
        LocalFree(buffer);
        return S_OK;
    }
}

HRESULT WINAPI PathCchFindExtension(const WCHAR *path, SIZE_T size, const WCHAR **extension)
{
    const WCHAR *lastpoint = NULL;
    SIZE_T counter = 0;

    TRACE("%s %Iu %p\n", wine_dbgstr_w(path), size, extension);

    if (!path || !size || size > PATHCCH_MAX_CCH)
    {
        *extension = NULL;
        return E_INVALIDARG;
    }

    while (*path)
    {
        if (*path == '\\' || *path == ' ')
            lastpoint = NULL;
        else if (*path == '.')
            lastpoint = path;

        path++;
        counter++;
        if (counter == size || counter == PATHCCH_MAX_CCH)
        {
            *extension = NULL;
            return E_INVALIDARG;
        }
    }

    *extension = lastpoint ? lastpoint : path;
    return S_OK;
}

BOOL WINAPI PathCchIsRoot(const WCHAR *path)
{
    const WCHAR *root_end;
    const WCHAR *next;
    BOOL is_unc;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || !*path) return FALSE;

    root_end = get_root_end(path);
    if (!root_end) return FALSE;

    if ((is_unc = is_prefixed_unc(path)) || (path[0] == '\\' && path[1] == '\\' && path[2] != '?'))
    {
        next = root_end + 1;
        /* No extra segments */
        if ((is_unc && !*next) || (!is_unc && !*next)) return TRUE;

        /* Has first segment with an ending backslash but no remaining characters */
        if (get_next_segment(next, &next) && !*next) return FALSE;
        /* Has first segment with no ending backslash */
        else if (!*next)
            return TRUE;
        /* Has first segment with an ending backslash and has remaining characters*/
        else
        {
            /* Second segment must have no backslash and no remaining characters */
            return !get_next_segment(next, &next) && !*next;
        }
    }
    else if (*root_end == '\\' && !root_end[1])
        return TRUE;
    else
        return FALSE;
}

HRESULT WINAPI PathCchRemoveBackslash(WCHAR *path, SIZE_T path_size)
{
    WCHAR *path_end;
    SIZE_T free_size;

    TRACE("%s %Iu\n", debugstr_w(path), path_size);

    return PathCchRemoveBackslashEx(path, path_size, &path_end, &free_size);
}

HRESULT WINAPI PathCchRemoveBackslashEx(WCHAR *path, SIZE_T path_size, WCHAR **path_end, SIZE_T *free_size)
{
    const WCHAR *root_end;
    SIZE_T path_length;

    TRACE("%s %Iu %p %p\n", debugstr_w(path), path_size, path_end, free_size);

    if (!path_size || !path_end || !free_size)
    {
        if (path_end) *path_end = NULL;
        if (free_size) *free_size = 0;
        return E_INVALIDARG;
    }

    path_length = wcsnlen(path, path_size);
    if (path_length == path_size && !path[path_length]) return E_INVALIDARG;

    root_end = get_root_end(path);
    if (path_length > 0 && path[path_length - 1] == '\\')
    {
        *path_end = path + path_length - 1;
        *free_size = path_size - path_length + 1;
        /* If the last character is beyond end of root */
        if (!root_end || path + path_length - 1 > root_end)
        {
            path[path_length - 1] = 0;
            return S_OK;
        }
        else
            return S_FALSE;
    }
    else
    {
        *path_end = path + path_length;
        *free_size = path_size - path_length;
        return S_FALSE;
    }
}

HRESULT WINAPI PathCchRemoveExtension(WCHAR *path, SIZE_T size)
{
    const WCHAR *extension;
    WCHAR *next;
    HRESULT hr;

    TRACE("%s %Iu\n", wine_dbgstr_w(path), size);

    if (!path || !size || size > PATHCCH_MAX_CCH) return E_INVALIDARG;

    hr = PathCchFindExtension(path, size, &extension);
    if (FAILED(hr)) return hr;

    next = path + (extension - path);
    while (next - path < size && *next) *next++ = 0;

    return next == extension ? S_FALSE : S_OK;
}

HRESULT WINAPI PathCchRemoveFileSpec(WCHAR *path, SIZE_T size)
{
    WCHAR *last, *root_end;

    TRACE("%s %Iu\n", wine_dbgstr_w(path), size);

    if (!path || !size || size > PATHCCH_MAX_CCH) return E_INVALIDARG;

    if (FAILED(PathCchSkipRoot(path, (const WCHAR **)&root_end)))
        root_end = path;

    /* The backslash at the end of UNC and \\* are not considered part of root in this case */
    if (root_end > path && root_end[-1] == '\\' && ((is_prefixed_unc(path) && path[8])
        || (path[0] == '\\' && path[1] == '\\' && path[2] && path[2] != '?')))
        root_end--;

    if (!(last = StrRChrW(root_end, NULL, '\\'))) last = root_end;
    if (last > root_end && last[-1] == '\\' && last[1] != '?') --last;
    if (last - path >= size) return E_INVALIDARG;
    if (!*last) return S_FALSE;
    *last = 0;
    return S_OK;
}

HRESULT WINAPI PathCchRenameExtension(WCHAR *path, SIZE_T size, const WCHAR *extension)
{
    HRESULT hr;

    TRACE("%s %Iu %s\n", wine_dbgstr_w(path), size, wine_dbgstr_w(extension));

    hr = PathCchRemoveExtension(path, size);
    if (FAILED(hr)) return hr;

    hr = PathCchAddExtension(path, size, extension);
    return FAILED(hr) ? hr : S_OK;
}

HRESULT WINAPI PathCchSkipRoot(const WCHAR *path, const WCHAR **root_end)
{
    TRACE("%s %p\n", debugstr_w(path), root_end);

    if (!path || !path[0] || !root_end
        || (!wcsnicmp(path, L"\\\\?", 3) && !is_prefixed_volume(path) && !is_prefixed_unc(path)
            && !is_prefixed_disk(path)))
        return E_INVALIDARG;

    *root_end = get_root_end(path);
    if (*root_end)
    {
        (*root_end)++;
        if (is_prefixed_unc(path))
        {
            get_next_segment(*root_end, root_end);
            get_next_segment(*root_end, root_end);
        }
        else if (path[0] == '\\' && path[1] == '\\' && path[2] != '?')
        {
            /* Skip share server */
            get_next_segment(*root_end, root_end);
            /* If mount point is empty, don't skip over mount point */
            if (**root_end != '\\') get_next_segment(*root_end, root_end);
        }
    }

    return *root_end ? S_OK : E_INVALIDARG;
}

HRESULT WINAPI PathCchStripPrefix(WCHAR *path, SIZE_T size)
{
    TRACE("%s %Iu\n", wine_dbgstr_w(path), size);

    if (!path || !size || size > PATHCCH_MAX_CCH) return E_INVALIDARG;

    if (is_prefixed_unc(path))
    {
        /* \\?\UNC\a -> \\a */
        if (size < lstrlenW(path + 8) + 3) return E_INVALIDARG;
        lstrcpyW(path + 2, path + 8);
        return S_OK;
    }
    else if (is_prefixed_disk(path))
    {
        /* \\?\C:\ -> C:\ */
        if (size < lstrlenW(path + 4) + 1) return E_INVALIDARG;
        lstrcpyW(path, path + 4);
        return S_OK;
    }
    else
        return S_FALSE;
}

HRESULT WINAPI PathCchStripToRoot(WCHAR *path, SIZE_T size)
{
    const WCHAR *root_end;
    WCHAR *segment_end;

    TRACE("%s %Iu\n", wine_dbgstr_w(path), size);

    if (!path || !*path || !size || size > PATHCCH_MAX_CCH) return E_INVALIDARG;

    if (PathCchSkipRoot(path, &root_end) == S_OK)
    {
        if (root_end && root_end > path && root_end[-1] == '\\'
            && ((is_prefixed_unc(path) && path[8]) || (path[0] == '\\' && path[1] == '\\' && path[2] && path[2] != '?')))
            root_end--;
        if (root_end - path >= size) return E_INVALIDARG;

        segment_end = path + (root_end - path);
        if (!*segment_end) return S_FALSE;

        *segment_end = 0;
        return S_OK;
    }
    else
    {
        *path = 0;
        return E_INVALIDARG;
    }
}

BOOL WINAPI PathIsUNCEx(const WCHAR *path, const WCHAR **server)
{
    const WCHAR *result = NULL;

    TRACE("%s %p\n", wine_dbgstr_w(path), server);

    if (is_prefixed_unc(path))
        result = path + 8;
    else if (path[0] == '\\' && path[1] == '\\' && path[2] != '?')
        result = path + 2;

    if (server) *server = result;
    return !!result;
}

BOOL WINAPI PathIsUNCA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    return path && (path[0] == '\\') && (path[1] == '\\');
}

BOOL WINAPI PathIsUNCW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    return path && (path[0] == '\\') && (path[1] == '\\');
}

BOOL WINAPI PathIsRelativeA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || !*path || IsDBCSLeadByte(*path))
        return TRUE;

    return !(*path == '\\' || (*path && path[1] == ':'));
}

BOOL WINAPI PathIsRelativeW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || !*path)
        return TRUE;

    return !(*path == '\\' || (*path && path[1] == ':'));
}

BOOL WINAPI PathIsUNCServerShareA(const char *path)
{
    BOOL seen_slash = FALSE;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (path && *path++ == '\\' && *path++ == '\\')
    {
        while (*path)
        {
            if (*path == '\\')
            {
                if (seen_slash)
                    return FALSE;
                seen_slash = TRUE;
            }

            path = CharNextA(path);
        }
    }

    return seen_slash;
}

BOOL WINAPI PathIsUNCServerShareW(const WCHAR *path)
{
    BOOL seen_slash = FALSE;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (path && *path++ == '\\' && *path++ == '\\')
    {
        while (*path)
        {
            if (*path == '\\')
            {
                if (seen_slash)
                    return FALSE;
                seen_slash = TRUE;
            }

            path++;
        }
    }

    return seen_slash;
}

BOOL WINAPI PathIsRootA(const char *path)
{
    WCHAR pathW[MAX_PATH];

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, MAX_PATH))
        return FALSE;
    if (is_prefixed_unc(pathW) || is_prefixed_disk(pathW) || is_prefixed_volume(pathW)) return FALSE;

    return PathIsRootW(pathW);
}

BOOL WINAPI PathIsRootW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    return PathCchIsRoot(path);
}

BOOL WINAPI PathRemoveFileSpecA(char *path)
{
    char *root_end = NULL, *ptr;

    TRACE("%s\n", debugstr_a(path));

    if (!path || !*path)
        return FALSE;

    if (is_drive_specA(path))
    {
        root_end = path + 2;
        if (*root_end == '\\') ++root_end;
    }
    else
    {
        root_end = path;
        if (*root_end == '\\') ++root_end;
        if (root_end[1] != '?')
        {
            if (*root_end == '\\') ++root_end;
            if (root_end - path > 1 && is_drive_specA(root_end)) root_end += 2;
            if (*root_end == '\\' && root_end[1] && root_end[1] != '\\') ++root_end;
        }
    }
    ptr = StrRChrA(root_end, NULL, '\\');
    if (ptr && ptr != root_end)
    {
        if (ptr[-1] == '\\') --ptr;
        *ptr = 0;
        return TRUE;
    }
    if (!*root_end) return FALSE;
    *root_end = 0;
    return TRUE;
}

BOOL WINAPI PathRemoveFileSpecW(WCHAR *path)
{
    WCHAR *root_end = NULL, *ptr;

    TRACE("%s\n", debugstr_w(path));

    if (!path || !*path)
        return FALSE;

    if (is_prefixed_volume(path))    root_end = path + 48;
    else if (is_prefixed_disk(path)) root_end = path + 6;
    else if (is_drive_spec(path))    root_end = path + 2;
    if (!root_end)
    {
        root_end = path;
        if (*root_end == '\\') ++root_end;
        if (root_end[1] != '?')
        {
            if (*root_end == '\\') ++root_end;
            if (root_end - path > 1 && is_drive_spec(root_end)) root_end += 2;
            if (*root_end == '\\' && root_end[1] && root_end[1] != '\\') ++root_end;
        }
    }
    else if (*root_end == '\\') ++root_end;
    ptr = StrRChrW(root_end, NULL, '\\');
    if (ptr && ptr != root_end)
    {
        if (ptr[-1] == '\\') --ptr;
        *ptr = 0;
        return TRUE;
    }
    if (!*root_end) return FALSE;
    *root_end = 0;
    return TRUE;
}

BOOL WINAPI PathStripToRootA(char *path)
{
    WCHAR pathW[MAX_PATH];

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, MAX_PATH)) return FALSE;

    *path = 0;
    if (is_prefixed_unc(pathW) || is_prefixed_disk(pathW) || is_prefixed_volume(pathW)) return FALSE;
    if (!PathStripToRootW(pathW)) return FALSE;
    return !!WideCharToMultiByte(CP_ACP, 0, pathW, -1, path, MAX_PATH, 0, 0);
}

BOOL WINAPI PathStripToRootW(WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    return SUCCEEDED(PathCchStripToRoot(path, PATHCCH_MAX_CCH));
}

LPSTR WINAPI PathAddBackslashA(char *path)
{
    unsigned int len;
    char *prev = path;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || (len = strlen(path)) >= MAX_PATH)
        return NULL;

    if (len)
    {
        do
        {
            path = CharNextA(prev);
            if (*path)
            prev = path;
        } while (*path);

        if (*prev != '\\')
        {
            *path++ = '\\';
            *path = '\0';
        }
    }

    return path;
}

LPWSTR WINAPI PathAddBackslashW(WCHAR *path)
{
    unsigned int len;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || (len = lstrlenW(path)) >= MAX_PATH)
        return NULL;

    if (len)
    {
        path += len;
        if (path[-1] != '\\')
        {
            *path++ = '\\';
            *path = '\0';
        }
    }

    return path;
}

LPSTR WINAPI PathFindExtensionA(const char *path)
{
    const char *lastpoint = NULL;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (path)
    {
        while (*path)
        {
            if (*path == '\\' || *path == ' ')
                lastpoint = NULL;
            else if (*path == '.')
                lastpoint = path;
            path = CharNextA(path);
        }
    }

    return (LPSTR)(lastpoint ? lastpoint : path);
}

LPWSTR WINAPI PathFindExtensionW(const WCHAR *path)
{
    const WCHAR *lastpoint = NULL;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (path)
    {
        while (*path)
        {
            if (*path == '\\' || *path == ' ')
                lastpoint = NULL;
            else if (*path == '.')
                lastpoint = path;
            path++;
        }
    }

    return (LPWSTR)(lastpoint ? lastpoint : path);
}

BOOL WINAPI PathAddExtensionA(char *path, const char *ext)
{
    unsigned int len;

    TRACE("%s, %s\n", wine_dbgstr_a(path), wine_dbgstr_a(ext));

    if (!path || !ext || *(PathFindExtensionA(path)))
        return FALSE;

    len = strlen(path);
    if (len + strlen(ext) >= MAX_PATH)
        return FALSE;

    strcpy(path + len, ext);
    return TRUE;
}

BOOL WINAPI PathAddExtensionW(WCHAR *path, const WCHAR *ext)
{
    unsigned int len;

    TRACE("%s, %s\n", wine_dbgstr_w(path), wine_dbgstr_w(ext));

    if (!path || !ext || *(PathFindExtensionW(path)))
        return FALSE;

    len = lstrlenW(path);
    if (len + lstrlenW(ext) >= MAX_PATH)
        return FALSE;

    lstrcpyW(path + len, ext);
    return TRUE;
}

BOOL WINAPI PathCanonicalizeW(WCHAR *buffer, const WCHAR *path)
{
    const WCHAR *src = path;
    WCHAR *dst = buffer;

    TRACE("%p, %s\n", buffer, wine_dbgstr_w(path));

    if (dst)
        *dst = '\0';

    if (!dst || !path)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!*path)
    {
        *buffer++ = '\\';
        *buffer = '\0';
        return TRUE;
    }

    /* Copy path root */
    if (*src == '\\')
    {
        *dst++ = *src++;
    }
    else if (*src && src[1] == ':')
    {
        /* X:\ */
        *dst++ = *src++;
        *dst++ = *src++;
        if (*src == '\\')
            *dst++ = *src++;
    }

    /* Canonicalize the rest of the path */
    while (*src)
    {
        if (*src == '.')
        {
            if (src[1] == '\\' && (src == path || src[-1] == '\\' || src[-1] == ':'))
            {
                src += 2; /* Skip .\ */
            }
            else if (src[1] == '.' && dst != buffer && dst[-1] == '\\')
            {
                /* \.. backs up a directory, over the root if it has no \ following X:.
                 * .. is ignored if it would remove a UNC server name or initial \\
                 */
                if (dst != buffer)
                {
                    *dst = '\0'; /* Allow PathIsUNCServerShareA test on lpszBuf */
                    if (dst > buffer + 1 && dst[-1] == '\\' && (dst[-2] != '\\' || dst > buffer + 2))
                    {
                        if (dst[-2] == ':' && (dst > buffer + 3 || dst[-3] == ':'))
                        {
                            dst -= 2;
                            while (dst > buffer && *dst != '\\')
                                dst--;
                            if (*dst == '\\')
                                dst++; /* Reset to last '\' */
                            else
                                dst = buffer; /* Start path again from new root */
                        }
                        else if (dst[-2] != ':' && !PathIsUNCServerShareW(buffer))
                            dst -= 2;
                    }
                    while (dst > buffer && *dst != '\\')
                        dst--;
                    if (dst == buffer)
                    {
                        *dst++ = '\\';
                        src++;
                    }
                }
                src += 2; /* Skip .. in src path */
            }
            else
                *dst++ = *src++;
        }
        else
            *dst++ = *src++;
    }

    /* Append \ to naked drive specs */
    if (dst - buffer == 2 && dst[-1] == ':')
        *dst++ = '\\';
    *dst++ = '\0';
    return TRUE;
}

BOOL WINAPI PathCanonicalizeA(char *buffer, const char *path)
{
    WCHAR pathW[MAX_PATH], bufferW[MAX_PATH];
    BOOL ret;
    int len;

    TRACE("%p, %s\n", buffer, wine_dbgstr_a(path));

    if (buffer)
        *buffer = '\0';

    if (!buffer || !path)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, ARRAY_SIZE(pathW));
    if (!len)
        return FALSE;

    ret = PathCanonicalizeW(bufferW, pathW);
    WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, MAX_PATH, 0, 0);

    return ret;
}

WCHAR * WINAPI PathCombineW(WCHAR *dst, const WCHAR *dir, const WCHAR *file)
{
    BOOL use_both = FALSE, strip = FALSE;
    WCHAR tmp[MAX_PATH];

    TRACE("%p, %s, %s\n", dst, wine_dbgstr_w(dir), wine_dbgstr_w(file));

    /* Invalid parameters */
    if (!dst)
        return NULL;

    if (!dir && !file)
    {
        dst[0] = 0;
        return NULL;
    }

    if ((!file || !*file) && dir)
    {
        /* Use dir only */
        lstrcpynW(tmp, dir, ARRAY_SIZE(tmp));
    }
    else if (!dir || !*dir || !PathIsRelativeW(file))
    {
        if (!dir || !*dir || *file != '\\' || PathIsUNCW(file))
        {
            /* Use file only */
            lstrcpynW(tmp, file, ARRAY_SIZE(tmp));
        }
        else
        {
            use_both = TRUE;
            strip = TRUE;
        }
    }
    else
        use_both = TRUE;

    if (use_both)
    {
        lstrcpynW(tmp, dir, ARRAY_SIZE(tmp));
        if (strip)
        {
            PathStripToRootW(tmp);
            file++; /* Skip '\' */
        }

        if (!PathAddBackslashW(tmp) || lstrlenW(tmp) + lstrlenW(file) >= MAX_PATH)
        {
            dst[0] = 0;
            return NULL;
        }

        lstrcatW(tmp, file);
    }

    PathCanonicalizeW(dst, tmp);
    return dst;
}

LPSTR WINAPI PathCombineA(char *dst, const char *dir, const char *file)
{
    WCHAR dstW[MAX_PATH], dirW[MAX_PATH], fileW[MAX_PATH];

    TRACE("%p, %s, %s\n", dst, wine_dbgstr_a(dir), wine_dbgstr_a(file));

    /* Invalid parameters */
    if (!dst)
        return NULL;

    if (!dir && !file)
        goto fail;

    if (dir && !MultiByteToWideChar(CP_ACP, 0, dir, -1, dirW, ARRAY_SIZE(dirW)))
        goto fail;

    if (file && !MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, ARRAY_SIZE(fileW)))
        goto fail;

    if (PathCombineW(dstW, dir ? dirW : NULL, file ? fileW : NULL))
        if (WideCharToMultiByte(CP_ACP, 0, dstW, -1, dst, MAX_PATH, 0, 0))
            return dst;
fail:
    dst[0] = 0;
    return NULL;
}

BOOL WINAPI PathAppendA(char *path, const char *append)
{
    TRACE("%s, %s\n", wine_dbgstr_a(path), wine_dbgstr_a(append));

    if (path && append)
    {
        if (!PathIsUNCA(append))
            while (*append == '\\')
                append++;

        if (PathCombineA(path, path, append))
            return TRUE;
    }

    return FALSE;
}

BOOL WINAPI PathAppendW(WCHAR *path, const WCHAR *append)
{
    TRACE("%s, %s\n", wine_dbgstr_w(path), wine_dbgstr_w(append));

    if (path && append)
    {
        if (!PathIsUNCW(append))
            while (*append == '\\')
                append++;

        if (PathCombineW(path, path, append))
            return TRUE;
    }

    return FALSE;
}

int WINAPI PathCommonPrefixA(const char *file1, const char *file2, char *path)
{
    const char *iter1 = file1;
    const char *iter2 = file2;
    unsigned int len = 0;

    TRACE("%s, %s, %p.\n", wine_dbgstr_a(file1), wine_dbgstr_a(file2), path);

    if (path)
        *path = '\0';

    if (!file1 || !file2)
        return 0;

    /* Handle roots first */
    if (PathIsUNCA(file1))
    {
        if (!PathIsUNCA(file2))
            return 0;
        iter1 += 2;
        iter2 += 2;
    }
    else if (PathIsUNCA(file2))
        return 0;

    for (;;)
    {
        /* Update len */
        if ((!*iter1 || *iter1 == '\\') && (!*iter2 || *iter2 == '\\'))
            len = iter1 - file1; /* Common to this point */

        if (!*iter1 || (tolower(*iter1) != tolower(*iter2)))
            break; /* Strings differ at this point */

        iter1++;
        iter2++;
    }

    if (len == 2)
        len++; /* Feature/Bug compatible with Win32 */

    if (len && path)
    {
        memcpy(path, file1, len);
        path[len] = '\0';
    }

    return len;
}

int WINAPI PathCommonPrefixW(const WCHAR *file1, const WCHAR *file2, WCHAR *path)
{
    const WCHAR *iter1 = file1;
    const WCHAR *iter2 = file2;
    unsigned int len = 0;

    TRACE("%s, %s, %p\n", wine_dbgstr_w(file1), wine_dbgstr_w(file2), path);

    if (path)
        *path = '\0';

    if (!file1 || !file2)
        return 0;

    /* Handle roots first */
    if (PathIsUNCW(file1))
    {
        if (!PathIsUNCW(file2))
            return 0;
        iter1 += 2;
        iter2 += 2;
    }
    else if (PathIsUNCW(file2))
      return 0;

    for (;;)
    {
        /* Update len */
        if ((!*iter1 || *iter1 == '\\') && (!*iter2 || *iter2 == '\\'))
            len = iter1 - file1; /* Common to this point */

        if (!*iter1 || (towupper(*iter1) != towupper(*iter2)))
            break; /* Strings differ at this point */

        iter1++;
        iter2++;
    }

    if (len == 2)
        len++; /* Feature/Bug compatible with Win32 */

    if (len && path)
    {
        memcpy(path, file1, len * sizeof(WCHAR));
        path[len] = '\0';
    }

    return len;
}

BOOL WINAPI PathIsPrefixA(const char *prefix, const char *path)
{
    TRACE("%s, %s\n", wine_dbgstr_a(prefix), wine_dbgstr_a(path));

    return prefix && path && PathCommonPrefixA(path, prefix, NULL) == (int)strlen(prefix);
}

BOOL WINAPI PathIsPrefixW(const WCHAR *prefix, const WCHAR *path)
{
    TRACE("%s, %s\n", wine_dbgstr_w(prefix), wine_dbgstr_w(path));

    return prefix && path && PathCommonPrefixW(path, prefix, NULL) == (int)lstrlenW(prefix);
}

char * WINAPI PathFindFileNameA(const char *path)
{
    const char *last_slash = path;

    TRACE("%s\n", wine_dbgstr_a(path));

    while (path && *path)
    {
        if ((*path == '\\' || *path == '/' || *path == ':') &&
                path[1] && path[1] != '\\' && path[1] != '/')
            last_slash = path + 1;
        path = CharNextA(path);
    }

    return (char *)last_slash;
}

WCHAR * WINAPI PathFindFileNameW(const WCHAR *path)
{
    const WCHAR *last_slash = path;

    TRACE("%s\n", wine_dbgstr_w(path));

    while (path && *path)
    {
        if ((*path == '\\' || *path == '/' || *path == ':') &&
                path[1] && path[1] != '\\' && path[1] != '/')
            last_slash = path + 1;
        path++;
    }

    return (WCHAR *)last_slash;
}

char * WINAPI PathGetArgsA(const char *path)
{
    BOOL seen_quote = FALSE;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return NULL;

    while (*path)
    {
        if (*path == ' ' && !seen_quote)
            return (char *)path + 1;

        if (*path == '"')
            seen_quote = !seen_quote;
        path = CharNextA(path);
    }

    return (char *)path;
}

WCHAR * WINAPI PathGetArgsW(const WCHAR *path)
{
    BOOL seen_quote = FALSE;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return NULL;

    while (*path)
    {
        if (*path == ' ' && !seen_quote)
            return (WCHAR *)path + 1;

        if (*path == '"')
            seen_quote = !seen_quote;
        path++;
    }

    return (WCHAR *)path;
}

UINT WINAPI PathGetCharTypeW(WCHAR ch)
{
    UINT flags = 0;

    TRACE("%#x\n", ch);

    if (!ch || ch < ' ' || ch == '<' || ch == '>' || ch == '"' || ch == '|' || ch == '/')
        flags = GCT_INVALID; /* Invalid */
    else if (ch == '*' || ch == '?')
        flags = GCT_WILD; /* Wildchars */
    else if (ch == '\\' || ch == ':')
        return GCT_SEPARATOR; /* Path separators */
    else
    {
        if (ch < 126)
        {
            if (((ch & 0x1) && ch != ';') || !ch || iswalnum(ch) || ch == '$' || ch == '&' || ch == '(' ||
                    ch == '.' || ch == '@' || ch == '^' || ch == '\'' || ch == '`')
            {
                flags |= GCT_SHORTCHAR; /* All these are valid for DOS */
            }
        }
        else
            flags |= GCT_SHORTCHAR; /* Bug compatible with win32 */

        flags |= GCT_LFNCHAR; /* Valid for long file names */
    }

    return flags;
}

UINT WINAPI PathGetCharTypeA(UCHAR ch)
{
    return PathGetCharTypeW(ch);
}

int WINAPI PathGetDriveNumberA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (path && *path && path[1] == ':')
    {
        if (*path >= 'a' && *path <= 'z') return *path - 'a';
        if (*path >= 'A' && *path <= 'Z') return *path - 'A';
    }
    return -1;
}

int WINAPI PathGetDriveNumberW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return -1;

    if (!wcsncmp(path, L"\\\\?\\", 4)) path += 4;

    if (!path[0] || path[1] != ':') return -1;
    if (path[0] >= 'A' && path[0] <= 'Z') return path[0] - 'A';
    if (path[0] >= 'a' && path[0] <= 'z') return path[0] - 'a';
    return -1;
}

BOOL WINAPI PathIsFileSpecA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return FALSE;

    while (*path)
    {
        if (*path == '\\' || *path == ':')
            return FALSE;
        path = CharNextA(path);
    }

    return TRUE;
}

BOOL WINAPI PathIsFileSpecW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return FALSE;

    while (*path)
    {
        if (*path == '\\' || *path == ':')
            return FALSE;
        path++;
    }

    return TRUE;
}

BOOL WINAPI PathIsUNCServerA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (!(path && path[0] == '\\' && path[1] == '\\'))
        return FALSE;

    while (*path)
    {
        if (*path == '\\')
            return FALSE;
        path = CharNextA(path);
    }

    return TRUE;
}

BOOL WINAPI PathIsUNCServerW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!(path && path[0] == '\\' && path[1] == '\\'))
        return FALSE;

    return !wcschr(path + 2, '\\');
}

void WINAPI PathRemoveBlanksA(char *path)
{
    char *start, *first;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || !*path)
        return;

    start = first = path;

    while (*path == ' ')
        path = CharNextA(path);

    while (*path)
        *start++ = *path++;

    if (start != first)
        while (start[-1] == ' ')
            start--;

    *start = '\0';
}

void WINAPI PathRemoveBlanksW(WCHAR *path)
{
    WCHAR *start, *first;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || !*path)
        return;

    start = first = path;

    while (*path == ' ')
        path++;

    while (*path)
        *start++ = *path++;

    if (start != first)
        while (start[-1] == ' ')
            start--;

    *start = '\0';
}

void WINAPI PathRemoveExtensionA(char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return;

    path = PathFindExtensionA(path);
    if (path && *path)
        *path = '\0';
}

void WINAPI PathRemoveExtensionW(WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return;

    path = PathFindExtensionW(path);
    if (path && *path)
        *path = '\0';
}

BOOL WINAPI PathRenameExtensionA(char *path, const char *ext)
{
    char *extension;

    TRACE("%s, %s\n", wine_dbgstr_a(path), wine_dbgstr_a(ext));

    extension = PathFindExtensionA(path);

    if (!extension || (extension - path + strlen(ext) >= MAX_PATH))
        return FALSE;

    strcpy(extension, ext);
    return TRUE;
}

BOOL WINAPI PathRenameExtensionW(WCHAR *path, const WCHAR *ext)
{
    WCHAR *extension;

    TRACE("%s, %s\n", wine_dbgstr_w(path), wine_dbgstr_w(ext));

    extension = PathFindExtensionW(path);

    if (!extension || (extension - path + lstrlenW(ext) >= MAX_PATH))
        return FALSE;

    lstrcpyW(extension, ext);
    return TRUE;
}

void WINAPI PathUnquoteSpacesA(char *path)
{
    unsigned int len;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || *path != '"')
        return;

    len = strlen(path) - 1;
    if (path[len] == '"')
    {
        path[len] = '\0';
        for (; *path; path++)
            *path = path[1];
    }
}

void WINAPI PathUnquoteSpacesW(WCHAR *path)
{
    unsigned int len;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || *path != '"')
        return;

    len = lstrlenW(path) - 1;
    if (path[len] == '"')
    {
        path[len] = '\0';
        for (; *path; path++)
            *path = path[1];
    }
}

char * WINAPI PathRemoveBackslashA(char *path)
{
    char *ptr;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return NULL;

    ptr = CharPrevA(path, path + strlen(path));
    if (!PathIsRootA(path) && *ptr == '\\')
        *ptr = '\0';

    return ptr;
}

WCHAR * WINAPI PathRemoveBackslashW(WCHAR *path)
{
    WCHAR *ptr;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return NULL;

    ptr = path + lstrlenW(path);
    if (ptr > path) ptr--;
    if (!PathIsRootW(path) && *ptr == '\\')
      *ptr = '\0';

    return ptr;
}

BOOL WINAPI PathIsLFNFileSpecA(const char *path)
{
    unsigned int name_len = 0, ext_len = 0;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return FALSE;

    while (*path)
    {
        if (*path == ' ')
            return TRUE; /* DOS names cannot have spaces */
        if (*path == '.')
        {
            if (ext_len)
                return TRUE; /* DOS names have only one dot */
            ext_len = 1;
        }
        else if (ext_len)
        {
            ext_len++;
            if (ext_len > 4)
                return TRUE; /* DOS extensions are <= 3 chars*/
        }
        else
        {
            name_len++;
            if (name_len > 8)
                return TRUE; /* DOS names are <= 8 chars */
        }
        path = CharNextA(path);
    }

    return FALSE; /* Valid DOS path */
}

BOOL WINAPI PathIsLFNFileSpecW(const WCHAR *path)
{
    unsigned int name_len = 0, ext_len = 0;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return FALSE;

    while (*path)
    {
        if (*path == ' ')
            return TRUE; /* DOS names cannot have spaces */
        if (*path == '.')
        {
            if (ext_len)
                return TRUE; /* DOS names have only one dot */
            ext_len = 1;
        }
        else if (ext_len)
        {
            ext_len++;
            if (ext_len > 4)
                return TRUE; /* DOS extensions are <= 3 chars*/
        }
        else
        {
            name_len++;
            if (name_len > 8)
                return TRUE; /* DOS names are <= 8 chars */
        }
        path++;
    }

    return FALSE; /* Valid DOS path */
}

#define PATH_CHAR_CLASS_LETTER      0x00000001
#define PATH_CHAR_CLASS_ASTERIX     0x00000002
#define PATH_CHAR_CLASS_DOT         0x00000004
#define PATH_CHAR_CLASS_BACKSLASH   0x00000008
#define PATH_CHAR_CLASS_COLON       0x00000010
#define PATH_CHAR_CLASS_SEMICOLON   0x00000020
#define PATH_CHAR_CLASS_COMMA       0x00000040
#define PATH_CHAR_CLASS_SPACE       0x00000080
#define PATH_CHAR_CLASS_OTHER_VALID 0x00000100
#define PATH_CHAR_CLASS_DOUBLEQUOTE 0x00000200

#define PATH_CHAR_CLASS_INVALID     0x00000000
#define PATH_CHAR_CLASS_ANY         0xffffffff

static const DWORD path_charclass[] =
{
    /* 0x00 */  PATH_CHAR_CLASS_INVALID,      /* 0x01 */  PATH_CHAR_CLASS_INVALID,
    /* 0x02 */  PATH_CHAR_CLASS_INVALID,      /* 0x03 */  PATH_CHAR_CLASS_INVALID,
    /* 0x04 */  PATH_CHAR_CLASS_INVALID,      /* 0x05 */  PATH_CHAR_CLASS_INVALID,
    /* 0x06 */  PATH_CHAR_CLASS_INVALID,      /* 0x07 */  PATH_CHAR_CLASS_INVALID,
    /* 0x08 */  PATH_CHAR_CLASS_INVALID,      /* 0x09 */  PATH_CHAR_CLASS_INVALID,
    /* 0x0a */  PATH_CHAR_CLASS_INVALID,      /* 0x0b */  PATH_CHAR_CLASS_INVALID,
    /* 0x0c */  PATH_CHAR_CLASS_INVALID,      /* 0x0d */  PATH_CHAR_CLASS_INVALID,
    /* 0x0e */  PATH_CHAR_CLASS_INVALID,      /* 0x0f */  PATH_CHAR_CLASS_INVALID,
    /* 0x10 */  PATH_CHAR_CLASS_INVALID,      /* 0x11 */  PATH_CHAR_CLASS_INVALID,
    /* 0x12 */  PATH_CHAR_CLASS_INVALID,      /* 0x13 */  PATH_CHAR_CLASS_INVALID,
    /* 0x14 */  PATH_CHAR_CLASS_INVALID,      /* 0x15 */  PATH_CHAR_CLASS_INVALID,
    /* 0x16 */  PATH_CHAR_CLASS_INVALID,      /* 0x17 */  PATH_CHAR_CLASS_INVALID,
    /* 0x18 */  PATH_CHAR_CLASS_INVALID,      /* 0x19 */  PATH_CHAR_CLASS_INVALID,
    /* 0x1a */  PATH_CHAR_CLASS_INVALID,      /* 0x1b */  PATH_CHAR_CLASS_INVALID,
    /* 0x1c */  PATH_CHAR_CLASS_INVALID,      /* 0x1d */  PATH_CHAR_CLASS_INVALID,
    /* 0x1e */  PATH_CHAR_CLASS_INVALID,      /* 0x1f */  PATH_CHAR_CLASS_INVALID,
    /* ' '  */  PATH_CHAR_CLASS_SPACE,        /* '!'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '"'  */  PATH_CHAR_CLASS_DOUBLEQUOTE,  /* '#'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '$'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '%'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '&'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '\'' */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '('  */  PATH_CHAR_CLASS_OTHER_VALID,  /* ')'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '*'  */  PATH_CHAR_CLASS_ASTERIX,      /* '+'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* ','  */  PATH_CHAR_CLASS_COMMA,        /* '-'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '.'  */  PATH_CHAR_CLASS_DOT,          /* '/'  */  PATH_CHAR_CLASS_INVALID,
    /* '0'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '1'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '2'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '3'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '4'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '5'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '6'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '7'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '8'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '9'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* ':'  */  PATH_CHAR_CLASS_COLON,        /* ';'  */  PATH_CHAR_CLASS_SEMICOLON,
    /* '<'  */  PATH_CHAR_CLASS_INVALID,      /* '='  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '>'  */  PATH_CHAR_CLASS_INVALID,      /* '?'  */  PATH_CHAR_CLASS_LETTER,
    /* '@'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* 'A'  */  PATH_CHAR_CLASS_ANY,
    /* 'B'  */  PATH_CHAR_CLASS_ANY,          /* 'C'  */  PATH_CHAR_CLASS_ANY,
    /* 'D'  */  PATH_CHAR_CLASS_ANY,          /* 'E'  */  PATH_CHAR_CLASS_ANY,
    /* 'F'  */  PATH_CHAR_CLASS_ANY,          /* 'G'  */  PATH_CHAR_CLASS_ANY,
    /* 'H'  */  PATH_CHAR_CLASS_ANY,          /* 'I'  */  PATH_CHAR_CLASS_ANY,
    /* 'J'  */  PATH_CHAR_CLASS_ANY,          /* 'K'  */  PATH_CHAR_CLASS_ANY,
    /* 'L'  */  PATH_CHAR_CLASS_ANY,          /* 'M'  */  PATH_CHAR_CLASS_ANY,
    /* 'N'  */  PATH_CHAR_CLASS_ANY,          /* 'O'  */  PATH_CHAR_CLASS_ANY,
    /* 'P'  */  PATH_CHAR_CLASS_ANY,          /* 'Q'  */  PATH_CHAR_CLASS_ANY,
    /* 'R'  */  PATH_CHAR_CLASS_ANY,          /* 'S'  */  PATH_CHAR_CLASS_ANY,
    /* 'T'  */  PATH_CHAR_CLASS_ANY,          /* 'U'  */  PATH_CHAR_CLASS_ANY,
    /* 'V'  */  PATH_CHAR_CLASS_ANY,          /* 'W'  */  PATH_CHAR_CLASS_ANY,
    /* 'X'  */  PATH_CHAR_CLASS_ANY,          /* 'Y'  */  PATH_CHAR_CLASS_ANY,
    /* 'Z'  */  PATH_CHAR_CLASS_ANY,          /* '['  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '\\' */  PATH_CHAR_CLASS_BACKSLASH,    /* ']'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '^'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* '_'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '`'  */  PATH_CHAR_CLASS_OTHER_VALID,  /* 'a'  */  PATH_CHAR_CLASS_ANY,
    /* 'b'  */  PATH_CHAR_CLASS_ANY,          /* 'c'  */  PATH_CHAR_CLASS_ANY,
    /* 'd'  */  PATH_CHAR_CLASS_ANY,          /* 'e'  */  PATH_CHAR_CLASS_ANY,
    /* 'f'  */  PATH_CHAR_CLASS_ANY,          /* 'g'  */  PATH_CHAR_CLASS_ANY,
    /* 'h'  */  PATH_CHAR_CLASS_ANY,          /* 'i'  */  PATH_CHAR_CLASS_ANY,
    /* 'j'  */  PATH_CHAR_CLASS_ANY,          /* 'k'  */  PATH_CHAR_CLASS_ANY,
    /* 'l'  */  PATH_CHAR_CLASS_ANY,          /* 'm'  */  PATH_CHAR_CLASS_ANY,
    /* 'n'  */  PATH_CHAR_CLASS_ANY,          /* 'o'  */  PATH_CHAR_CLASS_ANY,
    /* 'p'  */  PATH_CHAR_CLASS_ANY,          /* 'q'  */  PATH_CHAR_CLASS_ANY,
    /* 'r'  */  PATH_CHAR_CLASS_ANY,          /* 's'  */  PATH_CHAR_CLASS_ANY,
    /* 't'  */  PATH_CHAR_CLASS_ANY,          /* 'u'  */  PATH_CHAR_CLASS_ANY,
    /* 'v'  */  PATH_CHAR_CLASS_ANY,          /* 'w'  */  PATH_CHAR_CLASS_ANY,
    /* 'x'  */  PATH_CHAR_CLASS_ANY,          /* 'y'  */  PATH_CHAR_CLASS_ANY,
    /* 'z'  */  PATH_CHAR_CLASS_ANY,          /* '{'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '|'  */  PATH_CHAR_CLASS_INVALID,      /* '}'  */  PATH_CHAR_CLASS_OTHER_VALID,
    /* '~'  */  PATH_CHAR_CLASS_OTHER_VALID
};

BOOL WINAPI PathIsValidCharA(char c, DWORD class)
{
    if ((unsigned)c > 0x7e)
        return class & PATH_CHAR_CLASS_OTHER_VALID;

    return class & path_charclass[(unsigned)c];
}

BOOL WINAPI PathIsValidCharW(WCHAR c, DWORD class)
{
    if (c > 0x7e)
        return class & PATH_CHAR_CLASS_OTHER_VALID;

    return class & path_charclass[c];
}

char * WINAPI PathFindNextComponentA(const char *path)
{
    char *slash;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || !*path)
        return NULL;

    if ((slash = StrChrA(path, '\\')))
    {
        if (slash[1] == '\\')
            slash++;
        return slash + 1;
    }

    return (char *)path + strlen(path);
}

WCHAR * WINAPI PathFindNextComponentW(const WCHAR *path)
{
    WCHAR *slash;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || !*path)
        return NULL;

    if ((slash = StrChrW(path, '\\')))
    {
        if (slash[1] == '\\')
            slash++;
        return slash + 1;
    }

    return (WCHAR *)path + lstrlenW(path);
}

char * WINAPI PathSkipRootA(const char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || !*path)
        return NULL;

    if (*path == '\\' && path[1] == '\\')
    {
        /* Network share: skip share server and mount point */
        path += 2;
        if ((path = StrChrA(path, '\\')) && (path = StrChrA(path + 1, '\\')))
            path++;
        return (char *)path;
    }

    if (IsDBCSLeadByte(*path))
        return NULL;

    /* Check x:\ */
    if (path[0] && path[1] == ':' && path[2] == '\\')
        return (char *)path + 3;

    return NULL;
}

WCHAR * WINAPI PathSkipRootW(const WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || !*path)
        return NULL;

    if (*path == '\\' && path[1] == '\\')
    {
        /* Network share: skip share server and mount point */
        path += 2;
        if ((path = StrChrW(path, '\\')) && (path = StrChrW(path + 1, '\\')))
            path++;
        return (WCHAR *)path;
    }

    /* Check x:\ */
    if (path[0] && path[1] == ':' && path[2] == '\\')
        return (WCHAR *)path + 3;

    return NULL;
}

void WINAPI PathStripPathA(char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (path)
    {
        char *filename = PathFindFileNameA(path);
        if (filename != path)
            RtlMoveMemory(path, filename, strlen(filename) + 1);
    }
}

void WINAPI PathStripPathW(WCHAR *path)
{
    WCHAR *filename;

    TRACE("%s\n", wine_dbgstr_w(path));
    filename = PathFindFileNameW(path);
    if (filename != path)
        RtlMoveMemory(path, filename, (lstrlenW(filename) + 1) * sizeof(WCHAR));
}

BOOL WINAPI PathSearchAndQualifyA(const char *path, char *buffer, UINT length)
{
    TRACE("%s, %p, %u\n", wine_dbgstr_a(path), buffer, length);

    if (SearchPathA(NULL, path, NULL, length, buffer, NULL))
        return TRUE;

    return !!GetFullPathNameA(path, length, buffer, NULL);
}

BOOL WINAPI PathSearchAndQualifyW(const WCHAR *path, WCHAR *buffer, UINT length)
{
    TRACE("%s, %p, %u\n", wine_dbgstr_w(path), buffer, length);

    if (SearchPathW(NULL, path, NULL, length, buffer, NULL))
        return TRUE;
    return !!GetFullPathNameW(path, length, buffer, NULL);
}

BOOL WINAPI PathRelativePathToA(char *path, const char *from, DWORD attributes_from, const char *to,
        DWORD attributes_to)
{
    WCHAR pathW[MAX_PATH], fromW[MAX_PATH], toW[MAX_PATH];
    BOOL ret;

    TRACE("%p, %s, %#lx, %s, %#lx\n", path, wine_dbgstr_a(from), attributes_from, wine_dbgstr_a(to), attributes_to);

    if (!path || !from || !to)
        return FALSE;

    MultiByteToWideChar(CP_ACP, 0, from, -1, fromW, ARRAY_SIZE(fromW));
    MultiByteToWideChar(CP_ACP, 0, to, -1, toW, ARRAY_SIZE(toW));
    ret = PathRelativePathToW(pathW, fromW, attributes_from, toW, attributes_to);
    WideCharToMultiByte(CP_ACP, 0, pathW, -1, path, MAX_PATH, 0, 0);

    return ret;
}

BOOL WINAPI PathRelativePathToW(WCHAR *path, const WCHAR *from, DWORD attributes_from, const WCHAR *to,
        DWORD attributes_to)
{
    WCHAR fromW[MAX_PATH], toW[MAX_PATH];
    DWORD len;

    TRACE("%p, %s, %#lx, %s, %#lx\n", path, wine_dbgstr_w(from), attributes_from, wine_dbgstr_w(to), attributes_to);

    if (!path || !from || !to)
        return FALSE;

    *path = '\0';
    lstrcpynW(fromW, from, ARRAY_SIZE(fromW));
    lstrcpynW(toW, to, ARRAY_SIZE(toW));

    if (!(attributes_from & FILE_ATTRIBUTE_DIRECTORY))
        PathRemoveFileSpecW(fromW);
    if (!(attributes_to & FILE_ATTRIBUTE_DIRECTORY))
        PathRemoveFileSpecW(toW);

    /* Paths can only be relative if they have a common root */
    if (!(len = PathCommonPrefixW(fromW, toW, 0)))
        return FALSE;

    /* Strip off 'from' components to the root, by adding "..\" */
    from = fromW + len;
    if (!*from)
    {
        path[0] = '.';
        path[1] = '\0';
    }
    if (*from == '\\')
        from++;

    while (*from)
    {
        from = PathFindNextComponentW(from);
        lstrcatW(path, *from ? L"..\\" : L"..");
    }

    /* From the root add the components of 'to' */
    to += len;
    /* We check to[-1] to avoid skipping end of string. See the notes for this function. */
    if (*to && to[-1])
    {
        if (*to != '\\')
            to--;
        len = lstrlenW(path);
        if (len + lstrlenW(to) >= MAX_PATH)
        {
            *path = '\0';
            return FALSE;
        }
        lstrcpyW(path + len, to);
    }

    return TRUE;
}

HRESULT WINAPI PathMatchSpecExA(const char *path, const char *mask, DWORD flags)
{
    WCHAR *pathW, *maskW;
    HRESULT ret;

    TRACE("%s, %s\n", wine_dbgstr_a(path), wine_dbgstr_a(mask));

    if (flags)
        FIXME("Ignoring flags %#lx.\n", flags);

    if (!lstrcmpA(mask, "*.*"))
        return S_OK; /* Matches every path */

    pathW = heap_strdupAtoW( path );
    maskW = heap_strdupAtoW( mask );
    ret = PathMatchSpecExW( pathW, maskW, flags );
    heap_free( pathW );
    heap_free( maskW );
    return ret;
}

BOOL WINAPI PathMatchSpecA(const char *path, const char *mask)
{
    return PathMatchSpecExA(path, mask, 0) == S_OK;
}

static BOOL path_match_maskW(const WCHAR *name, const WCHAR *mask)
{
    while (*name && *mask && *mask != ';')
    {
        if (*mask == '*')
        {
            do
            {
                if (path_match_maskW(name, mask + 1))
                    return TRUE;  /* try substrings */
            } while (*name++);
            return FALSE;
        }

        if (towupper(*mask) != towupper(*name) && *mask != '?')
            return FALSE;

        name++;
        mask++;
    }

    if (!*name)
    {
        while (*mask == '*')
            mask++;
        if (!*mask || *mask == ';')
            return TRUE;
    }

    return FALSE;
}

HRESULT WINAPI PathMatchSpecExW(const WCHAR *path, const WCHAR *mask, DWORD flags)
{
    TRACE("%s, %s\n", wine_dbgstr_w(path), wine_dbgstr_w(mask));

    if (flags)
        FIXME("Ignoring flags %#lx.\n", flags);

    if (!lstrcmpW(mask, L"*.*"))
        return S_OK; /* Matches every path */

    while (*mask)
    {
        while (*mask == ' ')
            mask++; /* Eat leading spaces */

        if (path_match_maskW(path, mask))
            return S_OK; /* Matches the current path */

        while (*mask && *mask != ';')
            mask++; /* masks separated by ';' */

        if (*mask == ';')
            mask++;
    }

    return S_FALSE;
}

BOOL WINAPI PathMatchSpecW(const WCHAR *path, const WCHAR *mask)
{
    return PathMatchSpecExW(path, mask, 0) == S_OK;
}

void WINAPI PathQuoteSpacesA(char *path)
{
    TRACE("%s\n", wine_dbgstr_a(path));

    if (path && StrChrA(path, ' '))
    {
        size_t len = strlen(path) + 1;

        if (len + 2 < MAX_PATH)
        {
            memmove(path + 1, path, len);
            path[0] = '"';
            path[len] = '"';
            path[len + 1] = '\0';
        }
    }
}

void WINAPI PathQuoteSpacesW(WCHAR *path)
{
    TRACE("%s\n", wine_dbgstr_w(path));

    if (path && StrChrW(path, ' '))
    {
        int len = lstrlenW(path) + 1;

        if (len + 2 < MAX_PATH)
        {
            memmove(path + 1, path, len * sizeof(WCHAR));
            path[0] = '"';
            path[len] = '"';
            path[len + 1] = '\0';
        }
    }
}

BOOL WINAPI PathIsSameRootA(const char *path1, const char *path2)
{
    const char *start;
    int len;

    TRACE("%s, %s\n", wine_dbgstr_a(path1), wine_dbgstr_a(path2));

    if (!path1 || !path2 || !(start = PathSkipRootA(path1)))
        return FALSE;

    len = PathCommonPrefixA(path1, path2, NULL) + 1;
    return start - path1 <= len;
}

BOOL WINAPI PathIsSameRootW(const WCHAR *path1, const WCHAR *path2)
{
    const WCHAR *start;
    int len;

    TRACE("%s, %s\n", wine_dbgstr_w(path1), wine_dbgstr_w(path2));

    if (!path1 || !path2 || !(start = PathSkipRootW(path1)))
        return FALSE;

    len = PathCommonPrefixW(path1, path2, NULL) + 1;
    return start - path1 <= len;
}

BOOL WINAPI PathFileExistsA(const char *path)
{
    UINT prev_mode;
    DWORD attrs;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path)
        return FALSE;

    /* Prevent a dialog box if path is on a disk that has been ejected. */
    prev_mode = SetErrorMode(SEM_FAILCRITICALERRORS);
    attrs = GetFileAttributesA(path);
    SetErrorMode(prev_mode);
    return attrs != INVALID_FILE_ATTRIBUTES;
}

BOOL WINAPI PathFileExistsW(const WCHAR *path)
{
    UINT prev_mode;
    DWORD attrs;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path)
        return FALSE;

    prev_mode = SetErrorMode(SEM_FAILCRITICALERRORS);
    attrs = GetFileAttributesW(path);
    SetErrorMode(prev_mode);
    return attrs != INVALID_FILE_ATTRIBUTES;
}

int WINAPI PathParseIconLocationA(char *path)
{
    int ret = 0;
    char *comma;

    TRACE("%s\n", debugstr_a(path));

    if (!path)
        return 0;

    if ((comma = strchr(path, ',')))
    {
        *comma++ = '\0';
        ret = StrToIntA(comma);
    }
    PathUnquoteSpacesA(path);
    PathRemoveBlanksA(path);

    return ret;
}

int WINAPI PathParseIconLocationW(WCHAR *path)
{
    WCHAR *comma;
    int ret = 0;

    TRACE("%s\n", debugstr_w(path));

    if (!path)
        return 0;

    if ((comma = StrChrW(path, ',')))
    {
        *comma++ = '\0';
        ret = StrToIntW(comma);
    }
    PathUnquoteSpacesW(path);
    PathRemoveBlanksW(path);

    return ret;
}

BOOL WINAPI PathUnExpandEnvStringsA(const char *path, char *buffer, UINT buf_len)
{
    WCHAR bufferW[MAX_PATH], *pathW;
    DWORD len;
    BOOL ret;

    TRACE("%s, %p, %d\n", debugstr_a(path), buffer, buf_len);

    pathW = heap_strdupAtoW(path);
    if (!pathW) return FALSE;

    ret = PathUnExpandEnvStringsW(pathW, bufferW, MAX_PATH);
    HeapFree(GetProcessHeap(), 0, pathW);
    if (!ret) return FALSE;

    len = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL);
    if (buf_len < len + 1) return FALSE;

    WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, buf_len, NULL, NULL);
    return TRUE;
}

struct envvars_map
{
    const WCHAR *var;
    WCHAR path[MAX_PATH];
    DWORD len;
};

static void init_envvars_map(struct envvars_map *map)
{
    while (map->var)
    {
        map->len = ExpandEnvironmentStringsW(map->var, map->path, ARRAY_SIZE(map->path));
        /* exclude null from length */
        if (map->len) map->len--;
        map++;
    }
}

BOOL WINAPI PathUnExpandEnvStringsW(const WCHAR *path, WCHAR *buffer, UINT buf_len)
{
    static struct envvars_map null_var = {L"", {0}, 0};
    struct envvars_map *match = &null_var, *cur;
    struct envvars_map envvars[] =
    {
        { L"%ALLUSERSPROFILE%" },
        { L"%APPDATA%" },
        { L"%ProgramFiles%" },
        { L"%SystemRoot%" },
        { L"%SystemDrive%" },
        { L"%USERPROFILE%" },
        { NULL }
    };
    DWORD pathlen;
    UINT needed;

    TRACE("%s, %p, %d\n", debugstr_w(path), buffer, buf_len);

    pathlen = lstrlenW(path);
    init_envvars_map(envvars);
    cur = envvars;
    while (cur->var)
    {
        /* path can't contain expanded value or value wasn't retrieved */
        if (cur->len == 0 || cur->len > pathlen ||
            CompareStringOrdinal( cur->path, cur->len, path, cur->len, TRUE ) != CSTR_EQUAL)
        {
            cur++;
            continue;
        }

        if (cur->len > match->len)
            match = cur;
        cur++;
    }

    needed = lstrlenW(match->var) + 1 + pathlen - match->len;
    if (match->len == 0 || needed > buf_len) return FALSE;

    lstrcpyW(buffer, match->var);
    lstrcatW(buffer, &path[match->len]);
    TRACE("ret %s\n", debugstr_w(buffer));

    return TRUE;
}

static const struct
{
    URL_SCHEME scheme_number;
    const WCHAR *scheme_name;
}
url_schemes[] =
{
    { URL_SCHEME_FTP,        L"ftp"},
    { URL_SCHEME_HTTP,       L"http"},
    { URL_SCHEME_GOPHER,     L"gopher"},
    { URL_SCHEME_MAILTO,     L"mailto"},
    { URL_SCHEME_NEWS,       L"news"},
    { URL_SCHEME_NNTP,       L"nntp"},
    { URL_SCHEME_TELNET,     L"telnet"},
    { URL_SCHEME_WAIS,       L"wais"},
    { URL_SCHEME_FILE,       L"file"},
    { URL_SCHEME_MK,         L"mk"},
    { URL_SCHEME_HTTPS,      L"https"},
    { URL_SCHEME_SHELL,      L"shell"},
    { URL_SCHEME_SNEWS,      L"snews"},
    { URL_SCHEME_LOCAL,      L"local"},
    { URL_SCHEME_JAVASCRIPT, L"javascript"},
    { URL_SCHEME_VBSCRIPT,   L"vbscript"},
    { URL_SCHEME_ABOUT,      L"about"},
    { URL_SCHEME_RES,        L"res"},
};

static const WCHAR *parse_scheme( const WCHAR *p )
{
    while (*p <= 0x7f && (iswalnum( *p ) || *p == '+' || *p == '-' || *p == '.'))
        ++p;
    return p;
}

static DWORD get_scheme_code(const WCHAR *scheme, DWORD scheme_len)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(url_schemes); ++i)
    {
        if (scheme_len == lstrlenW(url_schemes[i].scheme_name)
                && !wcsnicmp(scheme, url_schemes[i].scheme_name, scheme_len))
            return url_schemes[i].scheme_number;
    }

    return URL_SCHEME_UNKNOWN;
}

HRESULT WINAPI ParseURLA(const char *url, PARSEDURLA *result)
{
    WCHAR scheme[INTERNET_MAX_SCHEME_LENGTH];
    const char *ptr = url;
    int len;

    TRACE("%s, %p\n", wine_dbgstr_a(url), result);

    if (result->cbSize != sizeof(*result))
        return E_INVALIDARG;

    while (*ptr && (isalnum( *ptr ) || *ptr == '-' || *ptr == '+' || *ptr == '.'))
        ptr++;

    if (*ptr != ':' || ptr <= url + 1)
    {
        result->pszProtocol = NULL;
        return URL_E_INVALID_SYNTAX;
    }

    result->pszProtocol = url;
    result->cchProtocol = ptr - url;
    result->pszSuffix = ptr + 1;
    result->cchSuffix = strlen(result->pszSuffix);

    len = MultiByteToWideChar(CP_ACP, 0, url, ptr - url, scheme, ARRAY_SIZE(scheme));
    result->nScheme = get_scheme_code(scheme, len);

    return S_OK;
}

HRESULT WINAPI ParseURLW(const WCHAR *url, PARSEDURLW *result)
{
    const WCHAR *ptr = url;

    TRACE("%s, %p\n", wine_dbgstr_w(url), result);

    if (result->cbSize != sizeof(*result))
        return E_INVALIDARG;

    while (*ptr && (iswalnum(*ptr) || *ptr == '-' || *ptr == '+' || *ptr == '.'))
        ptr++;

    if (*ptr != ':' || ptr <= url + 1)
    {
        result->pszProtocol = NULL;
        return URL_E_INVALID_SYNTAX;
    }

    result->pszProtocol = url;
    result->cchProtocol = ptr - url;
    result->pszSuffix = ptr + 1;
    result->cchSuffix = lstrlenW(result->pszSuffix);
    result->nScheme = get_scheme_code(url, ptr - url);

    return S_OK;
}

HRESULT WINAPI UrlUnescapeA(char *url, char *unescaped, DWORD *unescaped_len, DWORD flags)
{
    BOOL stop_unescaping = FALSE;
    const char *src;
    char *dst, next;
    DWORD needed;
    HRESULT hr;

    TRACE("%s, %p, %p, %#lx\n", wine_dbgstr_a(url), unescaped, unescaped_len, flags);

    if (!url)
        return E_INVALIDARG;

    if (flags & URL_UNESCAPE_INPLACE)
        dst = url;
    else
    {
        if (!unescaped || !unescaped_len) return E_INVALIDARG;
        dst = unescaped;
    }

    for (src = url, needed = 0; *src; src++, needed++)
    {
        if (flags & URL_DONT_UNESCAPE_EXTRA_INFO && (*src == '#' || *src == '?'))
        {
            stop_unescaping = TRUE;
            next = *src;
        }
        else if (*src == '%' && isxdigit(*(src + 1)) && isxdigit(*(src + 2)) && !stop_unescaping)
        {
            INT ih;
            char buf[3];
            memcpy(buf, src + 1, 2);
            buf[2] = '\0';
            ih = strtol(buf, NULL, 16);
            next = (CHAR) ih;
            src += 2; /* Advance to end of escape */
        }
        else
            next = *src;

        if (flags & URL_UNESCAPE_INPLACE || needed < *unescaped_len)
            *dst++ = next;
    }

    if (flags & URL_UNESCAPE_INPLACE || needed < *unescaped_len)
    {
        *dst = '\0';
        hr = S_OK;
    }
    else
    {
        needed++; /* add one for the '\0' */
        hr = E_POINTER;
    }

    if (!(flags & URL_UNESCAPE_INPLACE))
        *unescaped_len = needed;

    if (hr == S_OK)
        TRACE("result %s\n", flags & URL_UNESCAPE_INPLACE ? wine_dbgstr_a(url) : wine_dbgstr_a(unescaped));

    return hr;
}

static int get_utf8_len(unsigned char code)
{
    if (code < 0x80)
        return 1;
    else if ((code & 0xe0) == 0xc0)
        return 2;
    else if ((code & 0xf0) == 0xe0)
        return 3;
    else if ((code & 0xf8) == 0xf0)
        return 4;
    return 0;
}

HRESULT WINAPI UrlUnescapeW(WCHAR *url, WCHAR *unescaped, DWORD *unescaped_len, DWORD flags)
{
    WCHAR *dst, next, utf16_buf[4];
    BOOL stop_unescaping = FALSE;
    int utf8_len, utf16_len, i;
    const WCHAR *src;
    char utf8_buf[4];
    DWORD needed;
    HRESULT hr;

    TRACE("%s, %p, %p, %#lx\n", wine_dbgstr_w(url), unescaped, unescaped_len, flags);

    if (!url)
        return E_INVALIDARG;

    if (flags & URL_UNESCAPE_INPLACE)
        dst = url;
    else
    {
        if (!unescaped || !unescaped_len) return E_INVALIDARG;
        dst = unescaped;
    }

    for (src = url, needed = 0; *src; src++, needed++)
    {
        utf16_len = 0;
        if (flags & URL_DONT_UNESCAPE_EXTRA_INFO && (*src == '#' || *src == '?'))
        {
            stop_unescaping = TRUE;
            next = *src;
        }
        else if (*src == '%' && isxdigit(*(src + 1)) && isxdigit(*(src + 2)) && !stop_unescaping)
        {
            INT ih;
            WCHAR buf[5] = L"0x";

            memcpy(buf + 2, src + 1, 2*sizeof(WCHAR));
            buf[4] = 0;
            StrToIntExW(buf, STIF_SUPPORT_HEX, &ih);
            src += 2; /* Advance to end of escape */

            if (flags & URL_UNESCAPE_AS_UTF8)
            {
                utf8_buf[0] = ih;
                utf8_len = get_utf8_len(ih);
                for (i = 1; i < utf8_len && *(src + 1) == '%' && *(src + 2) &&  *(src + 3); i++)
                {
                    memcpy(buf + 2, src + 2, 2 * sizeof(WCHAR));
                    StrToIntExW(buf, STIF_SUPPORT_HEX, &ih);
                    /* Check if it is a valid continuation byte. */
                    if ((ih & 0xc0) == 0x80)
                    {
                        utf8_buf[i] = ih;
                        src += 3;
                    }
                    else
                        break;
                }

                utf16_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                        utf8_buf, i, utf16_buf, ARRAYSIZE(utf16_buf));
                if (utf16_len)
                    needed += utf16_len - 1;
                else
                    next = 0xfffd;
            }
            else
                next = (WCHAR) ih;
        }
        else
            next = *src;

        if (flags & URL_UNESCAPE_INPLACE || needed < *unescaped_len)
        {
            if (utf16_len)
            {
                memcpy(dst, utf16_buf, utf16_len * sizeof(*utf16_buf));
                dst += utf16_len;
            }
            else
                *dst++ = next;
        }
    }

    if (flags & URL_UNESCAPE_INPLACE || needed < *unescaped_len)
    {
        *dst = '\0';
        hr = S_OK;
    }
    else
    {
        needed++; /* add one for the '\0' */
        hr = E_POINTER;
    }

    if (!(flags & URL_UNESCAPE_INPLACE))
        *unescaped_len = needed;

    if (hr == S_OK)
        TRACE("result %s\n", flags & URL_UNESCAPE_INPLACE ? wine_dbgstr_w(url) : wine_dbgstr_w(unescaped));

    return hr;
}

HRESULT WINAPI PathCreateFromUrlA(const char *pszUrl, char *pszPath, DWORD *pcchPath, DWORD dwReserved)
{
    WCHAR bufW[MAX_PATH];
    WCHAR *pathW = bufW;
    UNICODE_STRING urlW;
    HRESULT ret;
    DWORD lenW = ARRAY_SIZE(bufW), lenA;

    if (!pszUrl || !pszPath || !pcchPath || !*pcchPath)
        return E_INVALIDARG;

    if(!RtlCreateUnicodeStringFromAsciiz(&urlW, pszUrl))
        return E_INVALIDARG;
    if((ret = PathCreateFromUrlW(urlW.Buffer, pathW, &lenW, dwReserved)) == E_POINTER) {
        pathW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
        ret = PathCreateFromUrlW(urlW.Buffer, pathW, &lenW, dwReserved);
    }
    if(ret == S_OK) {
        RtlUnicodeToMultiByteSize(&lenA, pathW, lenW * sizeof(WCHAR));
        if(*pcchPath > lenA) {
            RtlUnicodeToMultiByteN(pszPath, *pcchPath - 1, &lenA, pathW, lenW * sizeof(WCHAR));
            pszPath[lenA] = 0;
            *pcchPath = lenA;
        } else {
            *pcchPath = lenA + 1;
            ret = E_POINTER;
        }
    }
    if(pathW != bufW) HeapFree(GetProcessHeap(), 0, pathW);
    RtlFreeUnicodeString(&urlW);
    return ret;
}

HRESULT WINAPI PathCreateFromUrlW(const WCHAR *url, WCHAR *path, DWORD *pcchPath, DWORD dwReserved)
{
    DWORD nslashes, unescape, len;
    const WCHAR *src;
    WCHAR *tpath, *dst;
    HRESULT hr = S_OK;

    TRACE("%s, %p, %p, %#lx\n", wine_dbgstr_w(url), path, pcchPath, dwReserved);

    if (!url || !path || !pcchPath || !*pcchPath)
        return E_INVALIDARG;

    if (wcsnicmp( url, L"file:", 5))
        return E_INVALIDARG;

    url += 5;

    src = url;
    nslashes = 0;
    while (*src == '/' || *src == '\\')
    {
        nslashes++;
        src++;
    }

    /* We need a temporary buffer so we can compute what size to ask for.
     * We know that the final string won't be longer than the current pszUrl
     * plus at most two backslashes. All the other transformations make it
     * shorter.
     */
    len = 2 + lstrlenW(url) + 1;
    if (*pcchPath < len)
        tpath = heap_alloc(len * sizeof(WCHAR));
    else
        tpath = path;

    len = 0;
    dst = tpath;
    unescape = 1;
    switch (nslashes)
    {
    case 0:
        /* 'file:' + escaped DOS path */
        break;
    case 1:
        /* 'file:/' + escaped DOS path */
        /* fall through */
    case 3:
        /* 'file:///' (implied localhost) + escaped DOS path */
        if (!is_escaped_drive_spec( src ))
            src -= 1;
        break;
    case 2:
        if (lstrlenW(src) >= 10 && !wcsnicmp( src, L"localhost", 9) && (src[9] == '/' || src[9] == '\\'))
        {
            /* 'file://localhost/' + escaped DOS path */
            src += 10;
        }
        else if (is_escaped_drive_spec( src ))
        {
            /* 'file://' + unescaped DOS path */
            unescape = 0;
        }
        else
        {
            /*    'file://hostname:port/path' (where path is escaped)
             * or 'file:' + escaped UNC path (\\server\share\path)
             * The second form is clearly specific to Windows and it might
             * even be doing a network lookup to try to figure it out.
             */
            while (*src && *src != '/' && *src != '\\')
                src++;
            len = src - url;
            StrCpyNW(dst, url, len + 1);
            dst += len;
            if (*src && is_escaped_drive_spec( src + 1 ))
            {
                /* 'Forget' to add a trailing '/', just like Windows */
                src++;
            }
        }
        break;
    case 4:
        /* 'file://' + unescaped UNC path (\\server\share\path) */
        unescape = 0;
        if (is_escaped_drive_spec( src ))
            break;
        /* fall through */
    default:
        /* 'file:/...' + escaped UNC path (\\server\share\path) */
        src -= 2;
    }

    /* Copy the remainder of the path */
    len += lstrlenW(src);
    lstrcpyW(dst, src);

     /* First do the Windows-specific path conversions */
    for (dst = tpath; *dst; dst++)
        if (*dst == '/') *dst = '\\';
    if (is_escaped_drive_spec( tpath ))
        tpath[1] = ':'; /* c| -> c: */

    /* And only then unescape the path (i.e. escaped slashes are left as is) */
    if (unescape)
    {
        hr = UrlUnescapeW(tpath, NULL, &len, URL_UNESCAPE_INPLACE);
        if (hr == S_OK)
        {
            /* When working in-place UrlUnescapeW() does not set len */
            len = lstrlenW(tpath);
        }
    }

    if (*pcchPath < len + 1)
    {
        hr = E_POINTER;
        *pcchPath = len + 1;
    }
    else
    {
        *pcchPath = len;
        if (tpath != path)
            lstrcpyW(path, tpath);
    }
    if (tpath != path)
        heap_free(tpath);

    TRACE("Returning (%lu) %s\n", *pcchPath, wine_dbgstr_w(path));
    return hr;
}

HRESULT WINAPI PathCreateFromUrlAlloc(const WCHAR *url, WCHAR **path, DWORD reserved)
{
    WCHAR pathW[MAX_PATH];
    DWORD size;
    HRESULT hr;

    size = MAX_PATH;
    hr = PathCreateFromUrlW(url, pathW, &size, reserved);
    if (SUCCEEDED(hr))
    {
        /* Yes, this is supposed to crash if 'path' is NULL */
        *path = StrDupW(pathW);
    }

    return hr;
}

BOOL WINAPI PathIsURLA(const char *path)
{
    PARSEDURLA base;
    HRESULT hr;

    TRACE("%s\n", wine_dbgstr_a(path));

    if (!path || !*path)
        return FALSE;

    /* get protocol */
    base.cbSize = sizeof(base);
    hr = ParseURLA(path, &base);
    return hr == S_OK && (base.nScheme != URL_SCHEME_INVALID);
}

BOOL WINAPI PathIsURLW(const WCHAR *path)
{
    PARSEDURLW base;
    HRESULT hr;

    TRACE("%s\n", wine_dbgstr_w(path));

    if (!path || !*path)
        return FALSE;

    /* get protocol */
    base.cbSize = sizeof(base);
    hr = ParseURLW(path, &base);
    return hr == S_OK && (base.nScheme != URL_SCHEME_INVALID);
}

#define WINE_URL_BASH_AS_SLASH    0x01
#define WINE_URL_COLLAPSE_SLASHES 0x02
#define WINE_URL_ESCAPE_SLASH     0x04
#define WINE_URL_ESCAPE_HASH      0x08
#define WINE_URL_ESCAPE_QUESTION  0x10
#define WINE_URL_STOP_ON_HASH     0x20
#define WINE_URL_STOP_ON_QUESTION 0x40

static BOOL url_needs_escape(WCHAR ch, DWORD flags, DWORD int_flags)
{
    if (flags & URL_ESCAPE_SPACES_ONLY)
        return ch == ' ';

    if ((flags & URL_ESCAPE_PERCENT) && (ch == '%'))
        return TRUE;

    if ((flags & URL_ESCAPE_AS_UTF8) && (ch >= 0x80))
        return TRUE;

    if (ch <= 31 || (ch >= 127 && ch <= 255) )
        return TRUE;

    if (iswalnum(ch))
        return FALSE;

    switch (ch) {
    case ' ':
    case '<':
    case '>':
    case '\"':
    case '{':
    case '}':
    case '|':
    case '\\':
    case '^':
    case ']':
    case '[':
    case '`':
    case '&':
        return TRUE;
    case '/':
        return !!(int_flags & WINE_URL_ESCAPE_SLASH);
    case '?':
        return !!(int_flags & WINE_URL_ESCAPE_QUESTION);
    case '#':
        return !!(int_flags & WINE_URL_ESCAPE_HASH);
    default:
        return FALSE;
    }
}

HRESULT WINAPI UrlEscapeA(const char *url, char *escaped, DWORD *escaped_len, DWORD flags)
{
    WCHAR bufW[INTERNET_MAX_URL_LENGTH];
    WCHAR *escapedW = bufW;
    UNICODE_STRING urlW;
    HRESULT hr;
    DWORD lenW = ARRAY_SIZE(bufW), lenA;

    if (!escaped || !escaped_len || !*escaped_len)
        return E_INVALIDARG;

    if (!RtlCreateUnicodeStringFromAsciiz(&urlW, url))
        return E_INVALIDARG;

    if (flags & URL_ESCAPE_AS_UTF8)
    {
        RtlFreeUnicodeString(&urlW);
        return E_NOTIMPL;
    }

    if ((hr = UrlEscapeW(urlW.Buffer, escapedW, &lenW, flags)) == E_POINTER)
    {
        escapedW = heap_alloc(lenW * sizeof(WCHAR));
        hr = UrlEscapeW(urlW.Buffer, escapedW, &lenW, flags);
    }

    if (hr == S_OK)
    {
        RtlUnicodeToMultiByteSize(&lenA, escapedW, lenW * sizeof(WCHAR));
        if (*escaped_len > lenA)
        {
            RtlUnicodeToMultiByteN(escaped, *escaped_len - 1, &lenA, escapedW, lenW * sizeof(WCHAR));
            escaped[lenA] = 0;
            *escaped_len = lenA;
        }
        else
        {
            *escaped_len = lenA + 1;
            hr = E_POINTER;
        }
    }
    if (escapedW != bufW)
        heap_free(escapedW);
    RtlFreeUnicodeString(&urlW);
    return hr;
}

HRESULT WINAPI UrlEscapeW(const WCHAR *url, WCHAR *escaped, DWORD *escaped_len, DWORD flags)
{
    DWORD needed = 0, slashes = 0, int_flags;
    WCHAR next[12], *dst, *dst_ptr;
    BOOL stop_escaping = FALSE;
    PARSEDURLW parsed_url;
    const WCHAR *src;
    INT i, len;
    HRESULT hr;

    TRACE("%p, %s, %p, %p, %#lx\n", url, wine_dbgstr_w(url), escaped, escaped_len, flags);

    if (!url || !escaped_len || !escaped || *escaped_len == 0)
        return E_INVALIDARG;

    if (flags & ~(URL_ESCAPE_SPACES_ONLY | URL_ESCAPE_SEGMENT_ONLY | URL_DONT_ESCAPE_EXTRA_INFO |
            URL_ESCAPE_PERCENT | URL_ESCAPE_AS_UTF8))
    {
        FIXME("Unimplemented flags: %08lx\n", flags);
    }

    dst_ptr = dst = heap_alloc(*escaped_len * sizeof(WCHAR));
    if (!dst_ptr)
        return E_OUTOFMEMORY;

    /* fix up flags */
    if (flags & URL_ESCAPE_SPACES_ONLY)
        /* if SPACES_ONLY specified, reset the other controls */
        flags &= ~(URL_DONT_ESCAPE_EXTRA_INFO | URL_ESCAPE_PERCENT | URL_ESCAPE_SEGMENT_ONLY);
    else
        /* if SPACES_ONLY *not* specified the assume DONT_ESCAPE_EXTRA_INFO */
        flags |= URL_DONT_ESCAPE_EXTRA_INFO;

    int_flags = 0;
    if (flags & URL_ESCAPE_SEGMENT_ONLY)
        int_flags = WINE_URL_ESCAPE_QUESTION | WINE_URL_ESCAPE_HASH | WINE_URL_ESCAPE_SLASH;
    else
    {
        parsed_url.cbSize = sizeof(parsed_url);
        if (ParseURLW(url, &parsed_url) != S_OK)
            parsed_url.nScheme = URL_SCHEME_INVALID;

        TRACE("scheme = %d (%s)\n", parsed_url.nScheme, debugstr_wn(parsed_url.pszProtocol, parsed_url.cchProtocol));

        if (flags & URL_DONT_ESCAPE_EXTRA_INFO)
            int_flags = WINE_URL_STOP_ON_HASH | WINE_URL_STOP_ON_QUESTION;

        switch(parsed_url.nScheme) {
        case URL_SCHEME_FILE:
            int_flags |= WINE_URL_BASH_AS_SLASH | WINE_URL_COLLAPSE_SLASHES | WINE_URL_ESCAPE_HASH;
            int_flags &= ~WINE_URL_STOP_ON_HASH;
            break;

        case URL_SCHEME_HTTP:
        case URL_SCHEME_HTTPS:
            int_flags |= WINE_URL_BASH_AS_SLASH;
            if(parsed_url.pszSuffix[0] != '/' && parsed_url.pszSuffix[0] != '\\')
                int_flags |= WINE_URL_ESCAPE_SLASH;
            break;

        case URL_SCHEME_MAILTO:
            int_flags |= WINE_URL_ESCAPE_SLASH | WINE_URL_ESCAPE_QUESTION | WINE_URL_ESCAPE_HASH;
            int_flags &= ~(WINE_URL_STOP_ON_QUESTION | WINE_URL_STOP_ON_HASH);
            break;

        case URL_SCHEME_INVALID:
            break;

        case URL_SCHEME_FTP:
        default:
            if(parsed_url.pszSuffix[0] != '/')
                int_flags |= WINE_URL_ESCAPE_SLASH;
            break;
        }
    }

    for (src = url; *src; )
    {
        WCHAR cur = *src;
        len = 0;

        if ((int_flags & WINE_URL_COLLAPSE_SLASHES) && src == url + parsed_url.cchProtocol + 1)
        {
            while (cur == '/' || cur == '\\')
            {
                slashes++;
                cur = *++src;
            }
            if (slashes == 2 && !wcsnicmp(src, L"localhost", 9)) { /* file://localhost/ -> file:/// */
                if(src[9] == '/' || src[9] == '\\') src += 10;
                slashes = 3;
            }

            switch (slashes)
            {
            case 1:
            case 3:
                next[0] = next[1] = next[2] = '/';
                len = 3;
                break;
            case 0:
                len = 0;
                break;
            default:
                next[0] = next[1] = '/';
                len = 2;
                break;
            }
        }
        if (len == 0)
        {
            if (cur == '#' && (int_flags & WINE_URL_STOP_ON_HASH))
                stop_escaping = TRUE;

            if (cur == '?' && (int_flags & WINE_URL_STOP_ON_QUESTION))
                stop_escaping = TRUE;

            if (cur == '\\' && (int_flags & WINE_URL_BASH_AS_SLASH) && !stop_escaping) cur = '/';

            if (url_needs_escape(cur, flags, int_flags) && !stop_escaping)
            {
                if (flags & URL_ESCAPE_AS_UTF8)
                {
                    char utf[16];

                    if ((cur >= 0xd800 && cur <= 0xdfff) && (src[1] >= 0xdc00 && src[1] <= 0xdfff))
                    {
                        len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, src, 2, utf, sizeof(utf), NULL, NULL);
                        src++;
                    }
                    else
                        len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &cur, 1, utf, sizeof(utf), NULL, NULL);

                    if (!len)
                    {
                        utf[0] = 0xef;
                        utf[1] = 0xbf;
                        utf[2] = 0xbd;
                        len = 3;
                    }

                    for (i = 0; i < len; ++i)
                    {
                        next[i*3+0] = '%';
                        next[i*3+1] = hexDigits[(utf[i] >> 4) & 0xf];
                        next[i*3+2] = hexDigits[utf[i] & 0xf];
                    }
                    len *= 3;
                }
                else
                {
                    next[0] = '%';
                    next[1] = hexDigits[(cur >> 4) & 0xf];
                    next[2] = hexDigits[cur & 0xf];
                    len = 3;
                }
            }
            else
            {
                next[0] = cur;
                len = 1;
            }
            src++;
        }

        if (needed + len <= *escaped_len)
        {
            memcpy(dst, next, len*sizeof(WCHAR));
            dst += len;
        }
        needed += len;
    }

    if (needed < *escaped_len)
    {
        *dst = '\0';
        memcpy(escaped, dst_ptr, (needed+1)*sizeof(WCHAR));
        hr = S_OK;
    }
    else
    {
        needed++; /* add one for the '\0' */
        hr = E_POINTER;
    }
    *escaped_len = needed;

    heap_free(dst_ptr);
    return hr;
}

HRESULT WINAPI UrlCanonicalizeA(const char *src_url, char *canonicalized, DWORD *canonicalized_len, DWORD flags)
{
    LPWSTR url, canonical;
    HRESULT hr;

    TRACE("%s, %p, %p, %#lx\n", wine_dbgstr_a(src_url), canonicalized, canonicalized_len, flags);

    if (!src_url || !canonicalized || !canonicalized_len || !*canonicalized_len)
        return E_INVALIDARG;

    url = heap_strdupAtoW(src_url);
    canonical = heap_alloc(*canonicalized_len * sizeof(WCHAR));
    if (!url || !canonical)
    {
        heap_free(url);
        heap_free(canonical);
        return E_OUTOFMEMORY;
    }

    hr = UrlCanonicalizeW(url, canonical, canonicalized_len, flags);
    if (hr == S_OK)
        WideCharToMultiByte(CP_ACP, 0, canonical, -1, canonicalized, *canonicalized_len + 1, NULL, NULL);

    heap_free(url);
    heap_free(canonical);
    return hr;
}

static bool scheme_is_opaque( URL_SCHEME scheme )
{
    switch (scheme)
    {
        case URL_SCHEME_ABOUT:
        case URL_SCHEME_JAVASCRIPT:
        case URL_SCHEME_MAILTO:
        case URL_SCHEME_SHELL:
        case URL_SCHEME_VBSCRIPT:
            return true;

        default:
            return false;
    }
}

static bool scheme_preserves_backslashes( URL_SCHEME scheme )
{
    switch (scheme)
    {
        case URL_SCHEME_FTP:
        case URL_SCHEME_INVALID:
        case URL_SCHEME_LOCAL:
        case URL_SCHEME_MK:
        case URL_SCHEME_RES:
        case URL_SCHEME_UNKNOWN:
        case URL_SCHEME_WAIS:
            return true;

        default:
            return false;
    }
}

static bool scheme_uses_hostname( URL_SCHEME scheme )
{
    switch (scheme)
    {
        case URL_SCHEME_ABOUT:
        case URL_SCHEME_JAVASCRIPT:
        case URL_SCHEME_MAILTO:
        case URL_SCHEME_MK:
        case URL_SCHEME_SHELL:
        case URL_SCHEME_VBSCRIPT:
            return false;

        default:
            return true;
    }
}

static bool scheme_char_is_separator( URL_SCHEME scheme, WCHAR c )
{
    if (c == '/')
        return true;
    if (c == '\\' && scheme != URL_SCHEME_INVALID && scheme != URL_SCHEME_UNKNOWN)
        return true;
    return false;
}

static bool scheme_char_is_hostname_separator( URL_SCHEME scheme, DWORD flags, WCHAR c )
{
    switch (c)
    {
        case 0:
        case '/':
            return true;
        case '\\':
            return !scheme_preserves_backslashes( scheme );
        case '?':
            return scheme != URL_SCHEME_FILE || (flags & (URL_WININET_COMPATIBILITY | URL_FILE_USE_PATHURL));
        case '#':
            return scheme != URL_SCHEME_FILE;
        default:
            return false;
    }
}

static bool scheme_char_is_dot_separator( URL_SCHEME scheme, DWORD flags, WCHAR c )
{
    switch (c)
    {
        case 0:
        case '/':
        case '?':
            return true;
        case '#':
            return (scheme != URL_SCHEME_FILE || !(flags & (URL_WININET_COMPATIBILITY | URL_FILE_USE_PATHURL)));
        case '\\':
            return (scheme != URL_SCHEME_INVALID && scheme != URL_SCHEME_UNKNOWN && scheme != URL_SCHEME_MK);
        default:
            return false;
    }
}

/* There are essentially two types of behaviour concerning dot simplification,
 * not counting opaque schemes:
 *
 * 1) Simplify dots if and only if the first element is not a single or double
 *    dot. If a double dot would rewind past the root, ignore it. For example:
 *
 *        http://hostname/a/../../b/. -> http://hostname/b/
 *        http://hostname/./../../b/. -> http://hostname/./../../b/.
 *
 * 2) Effectively treat all paths as relative. Always simplify, except if a
 *    double dot would rewind past the root, in which case emit it verbatim.
 *    For example:
 *
 *        wine://hostname/a/../../b/. -> wine://hostname/../b/
 *        wine://hostname/./../../b/. -> wine://hostname/../b/
 *
 * For unclear reasons, this behaviour also correlates with whether a final
 * slash is always emitted after a single or double dot (e.g. if
 * URL_DONT_SIMPLIFY is specified). The former type does not emit a slash; the
 * latter does.
 */
static bool scheme_is_always_relative( URL_SCHEME scheme, DWORD flags )
{
    switch (scheme)
    {
        case URL_SCHEME_INVALID:
        case URL_SCHEME_UNKNOWN:
            return true;

        case URL_SCHEME_FILE:
            return flags & (URL_WININET_COMPATIBILITY | URL_FILE_USE_PATHURL);

        default:
            return false;
    }
}

struct string_buffer
{
    WCHAR *string;
    size_t len, capacity;
};

static void append_string( struct string_buffer *buffer, const WCHAR *str, size_t len )
{
    array_reserve( (void **)&buffer->string, &buffer->capacity, buffer->len + len, sizeof(WCHAR) );
    memcpy( buffer->string + buffer->len, str, len * sizeof(WCHAR) );
    buffer->len += len;
}

static void append_char( struct string_buffer *buffer, WCHAR c )
{
    append_string( buffer, &c, 1 );
}

static char get_slash_dir( URL_SCHEME scheme, DWORD flags, char src, const struct string_buffer *dst )
{
    if (src && scheme_preserves_backslashes( scheme ))
        return src;

    if (scheme == URL_SCHEME_FILE && (flags & (URL_FILE_USE_PATHURL | URL_WININET_COMPATIBILITY))
            && !wmemchr( dst->string, '#', dst->len ))
        return '\\';

    return '/';
}

static void rewrite_url( struct string_buffer *dst, const WCHAR *url, DWORD *flags_ptr )
{
    DWORD flags = *flags_ptr;
    bool pathurl = (flags & (URL_FILE_USE_PATHURL | URL_WININET_COMPATIBILITY));
    bool is_relative = false, has_hostname = false, has_initial_slash = false;
    const WCHAR *query = NULL, *hash = NULL;
    URL_SCHEME scheme = URL_SCHEME_INVALID;
    size_t query_len = 0, hash_len = 0;
    const WCHAR *scheme_end, *src_end;
    const WCHAR *hostname = NULL;
    size_t hostname_len = 0;
    const WCHAR *src = url;
    size_t root_offset;

    /* Determine the scheme. */

    scheme_end = parse_scheme( url );

    if (*scheme_end == ':' && scheme_end >= url + 2)
    {
        size_t scheme_len = scheme_end + 1 - url;

        scheme = get_scheme_code( url, scheme_len - 1 );

        for (size_t i = 0; i < scheme_len; ++i)
            append_char( dst, tolower( *src++ ));
    }
    else if (url[0] == '\\' && url[1] == '\\')
    {
        append_string( dst, L"file:", 5 );
        if (!pathurl && !(flags & URL_UNESCAPE))
            flags |= URL_ESCAPE_UNSAFE | URL_ESCAPE_PERCENT;
        scheme = URL_SCHEME_FILE;

        has_hostname = true;
    }

    if (is_escaped_drive_spec( url ))
    {
        append_string( dst, L"file://", 7 );
        if (!pathurl && !(flags & URL_UNESCAPE))
            flags |= URL_ESCAPE_UNSAFE | URL_ESCAPE_PERCENT;
        scheme = URL_SCHEME_FILE;

        hostname_len = 0;
        has_hostname = true;
    }
    else if (scheme == URL_SCHEME_MK)
    {
        if (src[0] == '@')
        {
            while (*src && *src != '/')
                append_char( dst, *src++ );
            if (*src == '/')
                append_char( dst, *src++ );
            else
                append_char( dst, '/' );

            if ((src[0] == '.' && scheme_char_is_dot_separator( scheme, flags, src[1] )) ||
                (src[0] == '.' && src[1] == '.' && scheme_char_is_dot_separator( scheme, flags, src[2] )))
                is_relative = true;
        }
    }
    else if (scheme_uses_hostname( scheme ) && scheme_char_is_separator( scheme, src[0] )
            && scheme_char_is_separator( scheme, src[1] ))
    {
        append_char( dst, scheme_preserves_backslashes( scheme ) ? src[0] : '/' );
        append_char( dst, scheme_preserves_backslashes( scheme ) ? src[1] : '/' );
        src += 2;
        if (scheme == URL_SCHEME_FILE && is_slash( src[0] ) && is_slash( src[1] ))
        {
            while (is_slash( *src ))
                ++src;
        }

        hostname = src;

        while (!scheme_char_is_hostname_separator( scheme, flags, *src ))
            ++src;
        hostname_len = src - hostname;
        has_hostname = true;
        has_initial_slash = true;
    }
    else if (scheme_char_is_separator( scheme, src[0] ))
    {
        has_initial_slash = true;

        if (scheme == URL_SCHEME_UNKNOWN || scheme == URL_SCHEME_INVALID)
        {
            /* Special case: an unknown scheme starting with a single slash
             * considers the "root" to be the single slash.
             * Most other schemes treat it as an empty path segment instead. */
            append_char( dst, *src++ );

            if (*src == '\\')
                ++src;
        }
        else if (scheme == URL_SCHEME_FILE)
        {
            src++;

            append_string( dst, L"//", 2 );

            hostname_len = 0;
            has_hostname = true;
        }
    }
    else
    {
        if (scheme == URL_SCHEME_FILE)
        {
            if (is_escaped_drive_spec( src ))
            {
                append_string( dst, L"//", 2 );
                hostname_len = 0;
                has_hostname = true;
            }
            else
            {
                if (flags & URL_FILE_USE_PATHURL)
                    append_string( dst, L"//", 2 );
            }
        }
    }

    if (scheme == URL_SCHEME_FILE && (flags & URL_FILE_USE_PATHURL))
        flags |= URL_UNESCAPE;

    *flags_ptr = flags;

    if (has_hostname)
    {
        if (scheme == URL_SCHEME_FILE)
        {
            bool is_drive = false;

            if (is_slash( *src ))
                ++src;

            if (hostname_len >= 2 && is_escaped_drive_spec( hostname ))
            {
                hostname_len = 0;
                src = hostname;
                is_drive = true;
            }
            else if (is_escaped_drive_spec( src ))
            {
                is_drive = true;
            }

            if (pathurl)
            {
                if (hostname_len == 9 && !wcsnicmp( hostname, L"localhost", 9 ))
                {
                    hostname_len = 0;
                    if (is_slash( *src ))
                        ++src;
                    if (is_escaped_drive_spec( src ))
                        is_drive = true;
                }

                if (!is_drive)
                {
                    if (hostname_len)
                    {
                        append_string( dst, L"\\\\", 2 );
                        append_string( dst, hostname, hostname_len );
                    }

                    if ((*src && *src != '?') || (flags & URL_WININET_COMPATIBILITY))
                        append_char( dst, get_slash_dir( scheme, flags, 0, dst ));
                }
            }
            else
            {
                if (hostname_len)
                    append_string( dst, hostname, hostname_len );
                append_char( dst, '/' );
            }

            if (is_drive)
            {
                /* Root starts after the first slash when file flags are in use,
                 * but directly after the drive specification if not. */
                if (pathurl)
                {
                    while (!scheme_char_is_hostname_separator( scheme, flags, *src ))
                        append_char( dst, *src++ );
                    if (is_slash( *src ))
                    {
                        append_char( dst, '\\' );
                        src++;
                    }
                }
                else
                {
                    append_char( dst, *src++ );
                    append_char( dst, *src++ );
                    if (is_slash( *src ))
                    {
                        append_char( dst, '/' );
                        src++;
                    }
                }
            }
        }
        else
        {
            for (size_t i = 0; i < hostname_len; ++i)
            {
                if (scheme == URL_SCHEME_UNKNOWN || scheme == URL_SCHEME_INVALID)
                    append_char( dst, hostname[i] );
                else
                    append_char( dst, tolower( hostname[i] ));
            }

            if (*src == '/' || *src == '\\')
            {
                append_char( dst, scheme_preserves_backslashes( scheme ) ? *src : '/' );
                src++;
            }
            else
            {
                append_char( dst, '/' );
            }
        }

        if ((src[0] == '.' && scheme_char_is_dot_separator( scheme, flags, src[1] )) ||
            (src[0] == '.' && src[1] == '.' && scheme_char_is_dot_separator( scheme, flags, src[2] )))
        {
            if (!scheme_is_always_relative( scheme, flags ))
                is_relative = true;
        }
    }

    /* root_offset now points to the point past which we will not rewind.
     * If there is a hostname, it points to the character after the closing
     * slash. */

    root_offset = dst->len;

    /* Break up the rest of the URL into the body, query, and hash parts. */

    src_end = src + wcslen( src );

    if (scheme_is_opaque( scheme ))
    {
        /* +1 for null terminator */
        append_string( dst, src, src_end + 1 - src );
        return;
    }

    if (scheme == URL_SCHEME_FILE)
    {
        if (!pathurl)
        {
            if (src[0] == '#')
                hash = src;
            else if (is_slash( src[0] ) && src[1] == '#')
                hash = src + 1;

            if (src[0] == '?')
                query = src;
            else if (is_slash( src[0] ) && src[1] == '?')
                query = src + 1;
        }
        else
        {
            query = wcschr( src, '?' );
        }

        if (!hash)
        {
            for (const WCHAR *p = src; p < src_end; ++p)
            {
                if (!wcsnicmp( p, L".htm#" , 5))
                    hash = p + 4;
                else if (!wcsnicmp( p, L".html#", 6 ))
                    hash = p + 5;
            }
        }
    }
    else
    {
        query = wcschr( src, '?' );
        hash = wcschr( src, '#' );
    }

    if (query)
        query_len = ((hash && hash > query) ? hash : src_end) - query;
    if (hash)
        hash_len = ((query && query > hash) ? query : src_end) - hash;

    if (query)
        src_end = query;
    if (hash && hash < src_end)
        src_end = hash;

    if (scheme == URL_SCHEME_UNKNOWN && !has_initial_slash)
    {
        if (!(flags & URL_DONT_SIMPLIFY) && src[0] == '.' && src_end == src + 1)
            src++;
        flags |= URL_DONT_SIMPLIFY;
    }

    while (src < src_end)
    {
        bool is_dots = false;
        size_t len;

        for (len = 0; src + len < src_end && !scheme_char_is_separator( scheme, src[len] ); ++len)
            ;

        if (src[0] == '.' && scheme_char_is_dot_separator( scheme, flags, src[1] ))
        {
            if (!is_relative)
            {
                if (flags & URL_DONT_SIMPLIFY)
                {
                    is_dots = true;
                }
                else
                {
                    ++src;
                    if (*src == '/' || *src == '\\')
                        ++src;
                    continue;
                }
            }
        }
        else if (src[0] == '.' && src[1] == '.' && scheme_char_is_dot_separator( scheme, flags, src[2] ))
        {
            if (!is_relative)
            {
                if (flags & URL_DONT_SIMPLIFY)
                {
                    is_dots = true;
                }
                else if (dst->len == root_offset && scheme_is_always_relative( scheme, flags ))
                {
                    /* We could also use is_dots here, except that we need to
                     * update root afterwards. */

                    append_char( dst, *src++ );
                    append_char( dst, *src++ );
                    if (*src == '/' || *src == '\\')
                        append_char( dst, get_slash_dir( scheme, flags, *src++, dst ));
                    else
                        append_char( dst, get_slash_dir( scheme, flags, 0, dst ));
                    root_offset = dst->len;
                    continue;
                }
                else
                {
                    if (dst->len > root_offset)
                        --dst->len; /* rewind past the last slash */

                    while (dst->len > root_offset && !scheme_char_is_separator( scheme, dst->string[dst->len - 1] ))
                        --dst->len;

                    src += 2;
                    if (*src == '/' || *src == '\\')
                        ++src;
                    continue;
                }
            }
        }

        if (len)
        {
            append_string( dst, src, len );
            src += len;
        }

        if (*src == '?' || *src == '#' || !*src)
        {
            if (scheme == URL_SCHEME_UNKNOWN && !has_initial_slash)
                is_dots = false;

            if (is_dots && scheme_is_always_relative( scheme, flags ))
                append_char( dst, get_slash_dir( scheme, flags, 0, dst ));
        }
        else /* slash */
        {
            append_char( dst, get_slash_dir( scheme, flags, *src++, dst ));
        }
    }

    /* If the source was non-empty but collapsed to an empty string, output a
     * single slash. */
    if (!dst->len && src_end != url)
        append_char( dst, '/' );

    /* UNKNOWN and FILE schemes usually reorder the ? before the #, but others
     * emit them in the original order. */
    if (query && hash && scheme != URL_SCHEME_FILE && scheme != URL_SCHEME_INVALID && scheme != URL_SCHEME_UNKNOWN)
    {
        if (query < hash)
        {
            append_string( dst, query, query_len );
            append_string( dst, hash, hash_len );
        }
        else
        {
            append_string( dst, hash, hash_len );
            append_string( dst, query, query_len );
        }
    }
    else if (!(scheme == URL_SCHEME_FILE && (flags & URL_FILE_USE_PATHURL)))
    {
        if (query)
            append_string( dst, query, query_len );

        if (hash)
            append_string( dst, hash, hash_len );
    }

    append_char( dst, 0 );
}

HRESULT WINAPI UrlCanonicalizeW(const WCHAR *src_url, WCHAR *canonicalized, DWORD *canonicalized_len, DWORD flags)
{
    struct string_buffer rewritten = {0};
    DWORD escape_flags;
    HRESULT hr = S_OK;
    const WCHAR *src;
    WCHAR *url, *dst;
    DWORD len;

    TRACE("%s, %p, %p, %#lx\n", wine_dbgstr_w(src_url), canonicalized, canonicalized_len, flags);

    if (!src_url || !canonicalized || !canonicalized_len || !*canonicalized_len)
        return E_INVALIDARG;

    if (!*src_url)
    {
        *canonicalized = 0;
        return S_OK;
    }

    /* PATHURL takes precedence. */
    if (flags & URL_FILE_USE_PATHURL)
        flags &= ~URL_WININET_COMPATIBILITY;

    /* strip initial and final C0 control characters and space */
    src = src_url;
    while (*src > 0 && *src <= 0x20)
        ++src;
    len = wcslen( src );
    while (len && src[len - 1] > 0 && src[len - 1] <= 0x20)
        --len;

    if (!(url = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
        return E_OUTOFMEMORY;

    dst = url;
    for (size_t i = 0; i < len; ++i)
    {
        if (src[i] != '\t' && src[i] != '\n' && src[i] != '\r')
            *dst++ = src[i];
    }
    *dst++ = 0;

    rewrite_url( &rewritten, url, &flags );

    if (flags & URL_UNESCAPE)
    {
        len = rewritten.len;
        UrlUnescapeW( rewritten.string, NULL, &len, URL_UNESCAPE_INPLACE);
        rewritten.len = wcslen( rewritten.string ) + 1;
    }

    /* URL_ESCAPE_SEGMENT_ONLY seems to be ignored. */
    escape_flags = flags & (URL_ESCAPE_UNSAFE | URL_ESCAPE_SPACES_ONLY | URL_ESCAPE_PERCENT |
            URL_DONT_ESCAPE_EXTRA_INFO);

    if (escape_flags)
    {
        escape_flags &= ~URL_ESCAPE_UNSAFE;
        hr = UrlEscapeW( rewritten.string, canonicalized, canonicalized_len, escape_flags );
    }
    else
    {
        /* No escaping needed, just copy the string */
        if (rewritten.len <= *canonicalized_len)
        {
            memcpy( canonicalized, rewritten.string, rewritten.len * sizeof(WCHAR) );
            *canonicalized_len = rewritten.len - 1;
        }
        else
        {
            hr = E_POINTER;
            *canonicalized_len = rewritten.len;
        }
    }

    heap_free( rewritten.string );
    heap_free( url );

    if (hr == S_OK)
        TRACE("result %s\n", wine_dbgstr_w(canonicalized));

    return hr;
}

HRESULT WINAPI UrlApplySchemeA(const char *url, char *out, DWORD *out_len, DWORD flags)
{
    LPWSTR inW, outW;
    HRESULT hr;
    DWORD len;

    TRACE("%s, %p, %p:out size %ld, %#lx\n", wine_dbgstr_a(url), out, out_len, out_len ? *out_len : 0, flags);

    if (!url || !out || !out_len)
        return E_INVALIDARG;

    inW = heap_alloc(2 * INTERNET_MAX_URL_LENGTH * sizeof(WCHAR));
    outW = inW + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(CP_ACP, 0, url, -1, inW, INTERNET_MAX_URL_LENGTH);
    len = INTERNET_MAX_URL_LENGTH;

    hr = UrlApplySchemeW(inW, outW, &len, flags);
    if (hr != S_OK)
    {
        heap_free(inW);
        return hr;
    }

    len = WideCharToMultiByte(CP_ACP, 0, outW, -1, NULL, 0, NULL, NULL);
    if (len > *out_len)
    {
        hr = E_POINTER;
        goto cleanup;
    }

    WideCharToMultiByte(CP_ACP, 0, outW, -1, out, *out_len, NULL, NULL);
    len--;

cleanup:
    *out_len = len;
    heap_free(inW);
    return hr;
}

static HRESULT url_guess_scheme(const WCHAR *url, WCHAR *out, DWORD *out_len)
{
    WCHAR reg_path[MAX_PATH], value[MAX_PATH], data[MAX_PATH];
    DWORD value_len, data_len, dwType, i;
    WCHAR Wxx, Wyy;
    HKEY newkey;
    INT index;
    BOOL j;

    MultiByteToWideChar(CP_ACP, 0,
            "Software\\Microsoft\\Windows\\CurrentVersion\\URL\\Prefixes", -1, reg_path, MAX_PATH);
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, reg_path, 0, 1, &newkey);
    index = 0;
    while (value_len = data_len = MAX_PATH,
            RegEnumValueW(newkey, index, value, &value_len, 0, &dwType, (LPVOID)data, &data_len) == 0)
    {
        TRACE("guess %d %s is %s\n", index, wine_dbgstr_w(value), wine_dbgstr_w(data));

        j = FALSE;
        for (i = 0; i < value_len; ++i)
        {
            Wxx = url[i];
            Wyy = value[i];
            /* remember that TRUE is not-equal */
            j = ChrCmpIW(Wxx, Wyy);
            if (j) break;
        }
        if ((i == value_len) && !j)
        {
            if (lstrlenW(data) + lstrlenW(url) + 1 > *out_len)
            {
                *out_len = lstrlenW(data) + lstrlenW(url) + 1;
                RegCloseKey(newkey);
                return E_POINTER;
            }
            lstrcpyW(out, data);
            lstrcatW(out, url);
            *out_len = lstrlenW(out);
            TRACE("matched and set to %s\n", wine_dbgstr_w(out));
            RegCloseKey(newkey);
            return S_OK;
        }
        index++;
    }
    RegCloseKey(newkey);
    return E_FAIL;
}

static HRESULT url_create_from_path(const WCHAR *path, WCHAR *url, DWORD *url_len)
{
    PARSEDURLW parsed_url;
    WCHAR *new_url;
    DWORD needed;
    HRESULT hr;

    parsed_url.cbSize = sizeof(parsed_url);
    if (ParseURLW(path, &parsed_url) == S_OK)
    {
        if (parsed_url.nScheme != URL_SCHEME_INVALID && parsed_url.cchProtocol > 1)
        {
            needed = lstrlenW(path);
            if (needed >= *url_len)
            {
                *url_len = needed + 1;
                return E_POINTER;
            }
            else
            {
                *url_len = needed;
                return S_FALSE;
            }
        }
    }

    new_url = heap_alloc((lstrlenW(path) + 9) * sizeof(WCHAR)); /* "file:///" + path length + 1 */
    lstrcpyW(new_url, L"file:");
    if (is_drive_spec( path )) lstrcatW(new_url, L"///");
    lstrcatW(new_url, path);
    hr = UrlEscapeW(new_url, url, url_len, URL_ESCAPE_PERCENT);
    heap_free(new_url);
    return hr;
}

static HRESULT url_apply_default_scheme(const WCHAR *url, WCHAR *out, DWORD *length)
{
    DWORD data_len, dwType;
    WCHAR data[MAX_PATH];
    HKEY newkey;

    /* get and prepend default */
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\URL\\DefaultPrefix",
                  0, 1, &newkey);
    data_len = sizeof(data);
    RegQueryValueExW(newkey, NULL, 0, &dwType, (BYTE *)data, &data_len);
    RegCloseKey(newkey);
    if (lstrlenW(data) + lstrlenW(url) + 1 > *length)
    {
        *length = lstrlenW(data) + lstrlenW(url) + 1;
        return E_POINTER;
    }
    lstrcpyW(out, data);
    lstrcatW(out, url);
    *length = lstrlenW(out);
    TRACE("used default %s\n", wine_dbgstr_w(out));
    return S_OK;
}

HRESULT WINAPI UrlApplySchemeW(const WCHAR *url, WCHAR *out, DWORD *length, DWORD flags)
{
    PARSEDURLW in_scheme;
    DWORD res1;
    HRESULT hr;

    TRACE("%s, %p, %p:out size %ld, %#lx\n", wine_dbgstr_w(url), out, length, length ? *length : 0, flags);

    if (!url || !out || !length)
        return E_INVALIDARG;

    if (flags & URL_APPLY_GUESSFILE)
    {
        if ((*length > 1 && ':' == url[1]) || PathIsUNCW(url))
        {
            res1 = *length;
            hr = url_create_from_path(url, out, &res1);
            if (hr == S_OK || hr == E_POINTER)
            {
                *length = res1;
                return hr;
            }
            else if (hr == S_FALSE)
            {
                return hr;
            }
        }
    }

    in_scheme.cbSize = sizeof(in_scheme);
    /* See if the base has a scheme */
    res1 = ParseURLW(url, &in_scheme);
    if (res1)
    {
        /* no scheme in input, need to see if we need to guess */
        if (flags & URL_APPLY_GUESSSCHEME)
        {
            if ((hr = url_guess_scheme(url, out, length)) != E_FAIL)
                return hr;
        }
    }

    /* If we are here, then either invalid scheme,
     * or no scheme and can't/failed guess.
     */
    if ((((res1 == 0) && (flags & URL_APPLY_FORCEAPPLY)) || ((res1 != 0)) ) && (flags & URL_APPLY_DEFAULT))
        return url_apply_default_scheme(url, out, length);

    return S_FALSE;
}

INT WINAPI UrlCompareA(const char *url1, const char *url2, BOOL ignore_slash)
{
    INT ret, len, len1, len2;

    if (!ignore_slash)
        return strcmp(url1, url2);
    len1 = strlen(url1);
    if (url1[len1-1] == '/') len1--;
    len2 = strlen(url2);
    if (url2[len2-1] == '/') len2--;
    if (len1 == len2)
        return strncmp(url1, url2, len1);
    len = min(len1, len2);
    ret = strncmp(url1, url2, len);
    if (ret) return ret;
    if (len1 > len2) return 1;
    return -1;
}

INT WINAPI UrlCompareW(const WCHAR *url1, const WCHAR *url2, BOOL ignore_slash)
{
    size_t len, len1, len2;
    INT ret;

    if (!ignore_slash)
        return lstrcmpW(url1, url2);
    len1 = lstrlenW(url1);
    if (url1[len1-1] == '/') len1--;
    len2 = lstrlenW(url2);
    if (url2[len2-1] == '/') len2--;
    if (len1 == len2)
        return wcsncmp(url1, url2, len1);
    len = min(len1, len2);
    ret = wcsncmp(url1, url2, len);
    if (ret) return ret;
    if (len1 > len2) return 1;
    return -1;
}

HRESULT WINAPI UrlFixupW(const WCHAR *url, WCHAR *translatedUrl, DWORD maxChars)
{
    DWORD srcLen;

    FIXME("%s, %p, %ld stub\n", wine_dbgstr_w(url), translatedUrl, maxChars);

    if (!url)
        return E_FAIL;

    srcLen = lstrlenW(url) + 1;

    /* For now just copy the URL directly */
    lstrcpynW(translatedUrl, url, (maxChars < srcLen) ? maxChars : srcLen);

    return S_OK;
}

const char * WINAPI UrlGetLocationA(const char *url)
{
    PARSEDURLA base;

    base.cbSize = sizeof(base);
    if (ParseURLA(url, &base) != S_OK) return NULL;  /* invalid scheme */

    /* if scheme is file: then never return pointer */
    if (!strncmp(base.pszProtocol, "file", min(4, base.cchProtocol)))
        return NULL;

    /* Look for '#' and return its addr */
    return strchr(base.pszSuffix, '#');
}

const WCHAR * WINAPI UrlGetLocationW(const WCHAR *url)
{
    PARSEDURLW base;

    base.cbSize = sizeof(base);
    if (ParseURLW(url, &base) != S_OK) return NULL;  /* invalid scheme */

    /* if scheme is file: then never return pointer */
    if (!wcsncmp(base.pszProtocol, L"file", min(4, base.cchProtocol)))
        return NULL;

    /* Look for '#' and return its addr */
    return wcschr(base.pszSuffix, '#');
}

HRESULT WINAPI UrlGetPartA(const char *url, char *out, DWORD *out_len, DWORD part, DWORD flags)
{
    LPWSTR inW, outW;
    DWORD len, len2;
    HRESULT hr;

    if (!url || !out || !out_len || !*out_len)
        return E_INVALIDARG;

    inW = heap_alloc(2 * INTERNET_MAX_URL_LENGTH * sizeof(WCHAR));
    outW = inW + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(CP_ACP, 0, url, -1, inW, INTERNET_MAX_URL_LENGTH);

    len = INTERNET_MAX_URL_LENGTH;
    hr = UrlGetPartW(inW, outW, &len, part, flags);
    if (FAILED(hr))
    {
        heap_free(inW);
        return hr;
    }

    len2 = WideCharToMultiByte(CP_ACP, 0, outW, len + 1, NULL, 0, NULL, NULL);
    if (len2 > *out_len)
    {
        *out_len = len2;
        heap_free(inW);
        return E_POINTER;
    }
    len2 = WideCharToMultiByte(CP_ACP, 0, outW, len + 1, out, *out_len, NULL, NULL);
    *out_len = len2 - 1;
    heap_free(inW);
    if (hr == S_OK && !*out_len) hr = S_FALSE;
    return hr;
}

static const WCHAR *parse_url_element( const WCHAR *url, const WCHAR *separators )
{
    const WCHAR *p;

    if ((p = wcspbrk( url, separators )))
        return p;
    return url + wcslen( url );
}

static void parse_url( const WCHAR *url, struct parsed_url *pl )
{
    const WCHAR *work;

    memset(pl, 0, sizeof(*pl));
    pl->scheme = url;
    work = parse_scheme( pl->scheme );
    if (work < url + 2 || *work != ':') return;
    pl->scheme_len = work - pl->scheme;
    work++;
    pl->scheme_number = get_scheme_code(pl->scheme, pl->scheme_len);
    if (!is_slash( work[0] ) || !is_slash( work[1] ))
    {
        if (pl->scheme_number != URL_SCHEME_FILE)
            pl->scheme_number = URL_SCHEME_UNKNOWN;
        return;
    }
    work += 2;

    if (pl->scheme_number != URL_SCHEME_FILE)
    {
        pl->username = work;
        work = parse_url_element( pl->username, L":@/\\?#" );
        pl->username_len = work - pl->username;
        if (*work == ':')
        {
            pl->password = work + 1;
            work = parse_url_element( pl->password, L"@/\\?#" );
            pl->password_len = work - pl->password;
            if (*work == '@')
            {
                work++;
            }
            else
            {
                /* what we just parsed must be the hostname and port
                 * so reset pointers and clear then let it parse */
                pl->username_len = pl->password_len = 0;
                work = pl->username;
                pl->username = pl->password = 0;
            }
        }
        else if (*work == '@')
        {
            /* no password */
            pl->password_len = 0;
            pl->password = 0;
            work++;
        }
        else
        {
            /* what was parsed was hostname, so reset pointers and let it parse */
            pl->username_len = pl->password_len = 0;
            work = pl->username;
            pl->username = pl->password = 0;
        }
    }

    pl->hostname = work;
    if (pl->scheme_number == URL_SCHEME_FILE)
    {
        work = parse_url_element( pl->hostname, L"/\\?#" );
        pl->hostname_len = work - pl->hostname;
        if (pl->hostname_len >= 2 && pl->hostname[1] == ':')
            pl->hostname_len = 0;
    }
    else
    {
        work = parse_url_element( pl->hostname, L":/\\?#" );
        pl->hostname_len = work - pl->hostname;

        if (*work == ':')
        {
            pl->port = work + 1;
            work = parse_url_element( pl->port, L"/\\?#" );
            pl->port_len = work - pl->port;
        }
    }

    if ((pl->query = wcschr( work, '?' )))
    {
        ++pl->query;
        pl->query_len = lstrlenW(pl->query);
    }
}

HRESULT WINAPI UrlGetPartW(const WCHAR *url, WCHAR *out, DWORD *out_len, DWORD part, DWORD flags)
{
    LPCWSTR addr, schaddr;
    struct parsed_url pl;
    DWORD size, schsize;

    TRACE("%s, %p, %p(%ld), %#lx, %#lx\n", wine_dbgstr_w(url), out, out_len, *out_len, part, flags);

    if (!url || !out || !out_len || !*out_len)
        return E_INVALIDARG;

    parse_url(url, &pl);

    switch (pl.scheme_number)
    {
        case URL_SCHEME_FTP:
        case URL_SCHEME_GOPHER:
        case URL_SCHEME_HTTP:
        case URL_SCHEME_HTTPS:
        case URL_SCHEME_TELNET:
        case URL_SCHEME_NEWS:
        case URL_SCHEME_NNTP:
        case URL_SCHEME_SNEWS:
            break;

        case URL_SCHEME_FILE:
            if (part != URL_PART_SCHEME && part != URL_PART_QUERY && part != URL_PART_HOSTNAME)
                return E_FAIL;
            break;

        default:
            if (part != URL_PART_SCHEME && part != URL_PART_QUERY)
                return E_FAIL;
    }

    switch (part)
    {
    case URL_PART_SCHEME:
        flags &= ~URL_PARTFLAG_KEEPSCHEME;
        addr = pl.scheme;
        size = pl.scheme_len;
        break;

    case URL_PART_HOSTNAME:
        addr = pl.hostname;
        size = pl.hostname_len;
        break;

    case URL_PART_USERNAME:
        if (!pl.username)
            return E_INVALIDARG;
        addr = pl.username;
        size = pl.username_len;
        break;

    case URL_PART_PASSWORD:
        if (!pl.password)
            return E_INVALIDARG;
        addr = pl.password;
        size = pl.password_len;
        break;

    case URL_PART_PORT:
        if (!pl.port)
            return E_INVALIDARG;
        addr = pl.port;
        size = pl.port_len;
        break;

    case URL_PART_QUERY:
        flags &= ~URL_PARTFLAG_KEEPSCHEME;
        addr = pl.query;
        size = pl.query_len;
        break;

    default:
        return E_INVALIDARG;
    }

    if (flags == URL_PARTFLAG_KEEPSCHEME && pl.scheme_number != URL_SCHEME_FILE)
    {
        if (!pl.scheme || !pl.scheme_len)
            return E_FAIL;
        schaddr = pl.scheme;
        schsize = pl.scheme_len;
        if (*out_len < schsize + size + 2)
        {
            *out_len = schsize + size + 2;
            return E_POINTER;
        }
        memcpy(out, schaddr, schsize*sizeof(WCHAR));
        out[schsize] = ':';
        memcpy(out + schsize+1, addr, size*sizeof(WCHAR));
        out[schsize+1+size] = 0;
        *out_len = schsize + 1 + size;
    }
    else
    {
        if (*out_len < size + 1)
        {
            *out_len = size + 1;
            return E_POINTER;
        }

        if (part == URL_PART_SCHEME)
        {
            unsigned int i;

            for (i = 0; i < size; ++i)
                out[i] = tolower( addr[i] );
        }
        else
        {
            memcpy( out, addr, size * sizeof(WCHAR) );
        }
        out[size] = 0;
        *out_len = size;
    }
    TRACE("len=%ld %s\n", *out_len, wine_dbgstr_w(out));

    return S_OK;
}

BOOL WINAPI UrlIsA(const char *url, URLIS Urlis)
{
    const char *last;
    PARSEDURLA base;

    TRACE("%s, %d\n", debugstr_a(url), Urlis);

    if (!url)
        return FALSE;

    switch (Urlis) {

    case URLIS_OPAQUE:
        base.cbSize = sizeof(base);
        if (ParseURLA(url, &base) != S_OK) return FALSE;  /* invalid scheme */
        return scheme_is_opaque( base.nScheme );

    case URLIS_FILEURL:
        return (CompareStringA(LOCALE_INVARIANT, NORM_IGNORECASE, url, 5, "file:", 5) == CSTR_EQUAL);

    case URLIS_DIRECTORY:
        last = url + strlen(url) - 1;
        return (last >= url && (*last == '/' || *last == '\\' ));

    case URLIS_URL:
        return PathIsURLA(url);

    case URLIS_NOHISTORY:
    case URLIS_APPLIABLE:
    case URLIS_HASQUERY:
    default:
        FIXME("(%s %d): stub\n", debugstr_a(url), Urlis);
    }

    return FALSE;
}

BOOL WINAPI UrlIsW(const WCHAR *url, URLIS Urlis)
{
    const WCHAR *last;
    PARSEDURLW base;

    TRACE("%s, %d\n", debugstr_w(url), Urlis);

    if (!url)
        return FALSE;

    switch (Urlis)
    {
    case URLIS_OPAQUE:
        base.cbSize = sizeof(base);
        if (ParseURLW(url, &base) != S_OK) return FALSE;  /* invalid scheme */
        switch (base.nScheme)
        {
        case URL_SCHEME_MAILTO:
        case URL_SCHEME_SHELL:
        case URL_SCHEME_JAVASCRIPT:
        case URL_SCHEME_VBSCRIPT:
        case URL_SCHEME_ABOUT:
            return TRUE;
        }
        return FALSE;

    case URLIS_FILEURL:
        return !wcsnicmp( url, L"file:", 5 );

    case URLIS_DIRECTORY:
        last = url + lstrlenW(url) - 1;
        return (last >= url && (*last == '/' || *last == '\\'));

    case URLIS_URL:
        return PathIsURLW(url);

    case URLIS_NOHISTORY:
    case URLIS_APPLIABLE:
    case URLIS_HASQUERY:
    default:
        FIXME("(%s %d): stub\n", debugstr_w(url), Urlis);
    }

    return FALSE;
}

BOOL WINAPI UrlIsOpaqueA(const char *url)
{
    return UrlIsA(url, URLIS_OPAQUE);
}

BOOL WINAPI UrlIsOpaqueW(const WCHAR *url)
{
    return UrlIsW(url, URLIS_OPAQUE);
}

BOOL WINAPI UrlIsNoHistoryA(const char *url)
{
    return UrlIsA(url, URLIS_NOHISTORY);
}

BOOL WINAPI UrlIsNoHistoryW(const WCHAR *url)
{
    return UrlIsW(url, URLIS_NOHISTORY);
}

HRESULT WINAPI UrlCreateFromPathA(const char *path, char *url, DWORD *url_len, DWORD reserved)
{
    WCHAR bufW[INTERNET_MAX_URL_LENGTH];
    DWORD lenW = ARRAY_SIZE(bufW), lenA;
    UNICODE_STRING pathW;
    WCHAR *urlW = bufW;
    HRESULT hr;

    if (!RtlCreateUnicodeStringFromAsciiz(&pathW, path))
        return E_INVALIDARG;

    if ((hr = UrlCreateFromPathW(pathW.Buffer, urlW, &lenW, reserved)) == E_POINTER)
    {
        urlW = heap_alloc(lenW * sizeof(WCHAR));
        hr = UrlCreateFromPathW(pathW.Buffer, urlW, &lenW, reserved);
    }

    if (SUCCEEDED(hr))
    {
        RtlUnicodeToMultiByteSize(&lenA, urlW, lenW * sizeof(WCHAR));
        if (*url_len > lenA)
        {
            RtlUnicodeToMultiByteN(url, *url_len - 1, &lenA, urlW, lenW * sizeof(WCHAR));
            url[lenA] = 0;
            *url_len = lenA;
        }
        else
        {
            *url_len = lenA + 1;
            hr = E_POINTER;
        }
    }
    if (urlW != bufW)
        heap_free(urlW);
    RtlFreeUnicodeString(&pathW);
    return hr;
}

HRESULT WINAPI UrlCreateFromPathW(const WCHAR *path, WCHAR *url, DWORD *url_len, DWORD reserved)
{
    HRESULT hr;

    TRACE("%s, %p, %p, %#lx\n", debugstr_w(path), url, url_len, reserved);

    if (reserved || !url || !url_len)
        return E_INVALIDARG;

    hr = url_create_from_path(path, url, url_len);
    if (hr == S_FALSE)
        lstrcpyW(url, path);

    return hr;
}

HRESULT WINAPI UrlCombineA(const char *base, const char *relative, char *combined, DWORD *combined_len, DWORD flags)
{
    WCHAR *baseW, *relativeW, *combinedW;
    DWORD len, len2;
    HRESULT hr;

    TRACE("%s, %s, %ld, %#lx\n", debugstr_a(base), debugstr_a(relative), combined_len ? *combined_len : 0, flags);

    if (!base || !relative || !combined_len)
        return E_INVALIDARG;

    baseW = heap_alloc(3 * INTERNET_MAX_URL_LENGTH * sizeof(WCHAR));
    relativeW = baseW + INTERNET_MAX_URL_LENGTH;
    combinedW = relativeW + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(CP_ACP, 0, base, -1, baseW, INTERNET_MAX_URL_LENGTH);
    MultiByteToWideChar(CP_ACP, 0, relative, -1, relativeW, INTERNET_MAX_URL_LENGTH);
    len = *combined_len;

    hr = UrlCombineW(baseW, relativeW, combined ? combinedW : NULL, &len, flags);
    if (hr != S_OK)
    {
        *combined_len = len;
        heap_free(baseW);
        return hr;
    }

    len2 = WideCharToMultiByte(CP_ACP, 0, combinedW, len, NULL, 0, NULL, NULL);
    if (len2 > *combined_len)
    {
        *combined_len = len2;
        heap_free(baseW);
        return E_POINTER;
    }
    WideCharToMultiByte(CP_ACP, 0, combinedW, len+1, combined, *combined_len + 1, NULL, NULL);
    *combined_len = len2;
    heap_free(baseW);
    return S_OK;
}

HRESULT WINAPI UrlCombineW(const WCHAR *baseW, const WCHAR *relativeW, WCHAR *combined, DWORD *combined_len, DWORD flags)
{
    DWORD i, len, process_case = 0, myflags, sizeloc = 0;
    LPWSTR work, preliminary, mbase, canonicalized;
    PARSEDURLW base, relative;
    HRESULT hr;

    TRACE("%s, %s, %ld, %#lx\n", debugstr_w(baseW), debugstr_w(relativeW), combined_len ? *combined_len : 0, flags);

    if (!baseW || !relativeW || !combined_len)
        return E_INVALIDARG;

    base.cbSize = sizeof(base);
    relative.cbSize = sizeof(relative);

    /* Get space for duplicates of the input and the output */
    preliminary = heap_alloc(3 * INTERNET_MAX_URL_LENGTH * sizeof(WCHAR));
    mbase = preliminary + INTERNET_MAX_URL_LENGTH;
    canonicalized = mbase + INTERNET_MAX_URL_LENGTH;
    *preliminary = '\0';

    /* Canonicalize the base input prior to looking for the scheme */
    myflags = flags & (URL_DONT_SIMPLIFY | URL_UNESCAPE);
    len = INTERNET_MAX_URL_LENGTH;
    UrlCanonicalizeW(baseW, mbase, &len, myflags);

    /* See if the base has a scheme */
    if (ParseURLW(mbase, &base) != S_OK)
    {
        /* If base has no scheme return relative. */
        TRACE("no scheme detected in Base\n");
        process_case = 1;
    }
    else do
    {
        BOOL manual_search = FALSE;

        work = (LPWSTR)base.pszProtocol;
        for (i = 0; i < base.cchProtocol; ++i)
            work[i] = RtlDowncaseUnicodeChar(work[i]);

        /* mk is a special case */
        if (base.nScheme == URL_SCHEME_MK)
        {
            WCHAR *ptr = wcsstr(base.pszSuffix, L"::");
            if (ptr)
            {
                int delta;

                ptr += 2;
                delta = ptr-base.pszSuffix;
                base.cchProtocol += delta;
                base.pszSuffix += delta;
                base.cchSuffix -= delta;
            }
        }
        else
        {
            /* get size of location field (if it exists) */
            work = (LPWSTR)base.pszSuffix;
            sizeloc = 0;
            if (*work++ == '/')
            {
                if (*work++ == '/')
                {
                    /* At this point have start of location and
                     * it ends at next '/' or end of string.
                     */
                    while (*work && (*work != '/')) work++;
                    sizeloc = (DWORD)(work - base.pszSuffix);
                }
            }
        }

        /* If there is a '?', then the remaining part can only contain a
         * query string or fragment, so start looking for the last leaf
         * from the '?'. Otherwise, if there is a '#' and the characters
         * immediately preceding it are ".htm[l]", then begin looking for
         * the last leaf starting from the '#'. Otherwise the '#' is not
         * meaningful and just start looking from the end. */
        if ((work = wcspbrk(base.pszSuffix + sizeloc, L"#?")))
        {
            if (*work == '?' || base.nScheme == URL_SCHEME_HTTP || base.nScheme == URL_SCHEME_HTTPS)
                manual_search = TRUE;
            else if (work - base.pszSuffix > 4)
            {
                if (!wcsnicmp(work - 4, L".htm", 4)) manual_search = TRUE;
            }

            if (!manual_search && work - base.pszSuffix > 5)
            {
                if (!wcsnicmp(work - 5, L".html", 5)) manual_search = TRUE;
            }
        }

        if (manual_search)
        {
            /* search backwards starting from the current position */
            while (*work != '/' && work > base.pszSuffix + sizeloc)
                --work;
            base.cchSuffix = work - base.pszSuffix + 1;
        }
        else
        {
            /* search backwards starting from the end of the string */
            work = wcsrchr((base.pszSuffix+sizeloc), '/');
            if (work)
            {
                len = (DWORD)(work - base.pszSuffix + 1);
                base.cchSuffix = len;
            }
            else
                base.cchSuffix = sizeloc;
        }

        /*
         * At this point:
         *    .pszSuffix   points to location (starting with '//')
         *    .cchSuffix   length of location (above) and rest less the last
         *                 leaf (if any)
         *    sizeloc   length of location (above) up to but not including
         *              the last '/'
         */

        if (ParseURLW(relativeW, &relative) != S_OK)
        {
            /* No scheme in relative */
            TRACE("no scheme detected in Relative\n");
            relative.pszSuffix = relativeW;  /* case 3,4,5 depends on this */
            relative.cchSuffix = lstrlenW( relativeW );
            if (*relativeW == ':')
            {
                /* Case that is either left alone or uses base. */
                if (flags & URL_PLUGGABLE_PROTOCOL)
                {
                    process_case = 5;
                    break;
                }
                process_case = 1;
                break;
            }
            if (is_drive_spec( relativeW ))
            {
                /* case that becomes "file:///" */
                lstrcpyW(preliminary, L"file:///");
                process_case = 1;
                break;
            }
            if ((relativeW[0] == '/' || relativeW[0] == '\\') &&
                    (relativeW[1] == '/' || relativeW[1] == '\\'))
            {
                /* Relative has location and the rest. */
                process_case = 3;
                break;
            }
            if (*relativeW == '/' || *relativeW == '\\')
            {
                /* Relative is root to location. */
                process_case = 4;
                break;
            }
            if (*relativeW == '#')
            {
                if (!(work = wcschr(base.pszSuffix+base.cchSuffix, '#')))
                    work = (LPWSTR)base.pszSuffix + lstrlenW(base.pszSuffix);

                memcpy(preliminary, base.pszProtocol, (work-base.pszProtocol)*sizeof(WCHAR));
                preliminary[work-base.pszProtocol] = '\0';
                process_case = 1;
                break;
            }
            process_case = (*base.pszSuffix == '/' || base.nScheme == URL_SCHEME_MK) ? 5 : 3;
            break;
        }
        else
        {
            work = (LPWSTR)relative.pszProtocol;
            for (i = 0; i < relative.cchProtocol; ++i)
                work[i] = RtlDowncaseUnicodeChar(work[i]);
        }

        /* Handle cases where relative has scheme. */
        if ((base.cchProtocol == relative.cchProtocol) && !wcsncmp(base.pszProtocol, relative.pszProtocol, base.cchProtocol))
        {
            /* since the schemes are the same */
            if (*relative.pszSuffix == '/' && *(relative.pszSuffix+1) == '/')
            {
                /* Relative replaces location and what follows. */
                process_case = 3;
                break;
            }
            if (*relative.pszSuffix == '/')
            {
                /* Relative is root to location */
                process_case = 4;
                break;
            }
            /* replace either just location if base's location starts with a
             * slash or otherwise everything */
            process_case = (*base.pszSuffix == '/') ? 5 : 1;
            break;
        }

        if (*relative.pszSuffix == '/' && *(relative.pszSuffix+1) == '/')
        {
            /* Relative replaces scheme, location, and following and handles PLUGGABLE */
            process_case = 2;
            break;
        }
        process_case = 1;
        break;
    } while (FALSE); /* a little trick to allow easy exit from nested if's */

    hr = S_OK;
    switch (process_case)
    {
    case 1:
        /* Return relative appended to whatever is in combined (which may the string "file:///" */
        lstrcatW(preliminary, relativeW);
        break;

    case 2:
        /* Relative replaces scheme and location */
        lstrcpyW(preliminary, relativeW);
        break;

    case 3:
        /* Return the base scheme with relative. Basically keeps the scheme and replaces the domain and following. */
        memcpy(preliminary, base.pszProtocol, (base.cchProtocol + 1)*sizeof(WCHAR));
        work = preliminary + base.cchProtocol + 1;
        lstrcpyW(work, relative.pszSuffix);
        break;

    case 4:
        /* Return the base scheme and location but everything after the location is relative. (Replace document from root on.) */
        memcpy(preliminary, base.pszProtocol, (base.cchProtocol+1+sizeloc)*sizeof(WCHAR));
        work = preliminary + base.cchProtocol + 1 + sizeloc;
        if (flags & URL_PLUGGABLE_PROTOCOL)
            *(work++) = '/';
        lstrcpyW(work, relative.pszSuffix);
        break;

    case 5:
        /* Return the base without its document (if any) and append relative after its scheme. */
        memcpy(preliminary, base.pszProtocol, (base.cchProtocol + 1 + base.cchSuffix)*sizeof(WCHAR));
        work = preliminary + base.cchProtocol + 1 + base.cchSuffix - 1;
        if (*work++ != '/')
            *(work++) = '/';
        lstrcpyW(work, relative.pszSuffix);
        break;

    default:
        FIXME("Unexpected case %ld.\n", process_case);
        hr = E_INVALIDARG;
    }

    if (hr == S_OK)
    {
        if (*combined_len == 0)
            *combined_len = 1;
        hr = UrlCanonicalizeW(preliminary, canonicalized, combined_len, flags & ~URL_FILE_USE_PATHURL);
        if (SUCCEEDED(hr) && combined)
            lstrcpyW( combined, canonicalized );

        TRACE("return-%ld len=%ld, %s\n", process_case, *combined_len, debugstr_w(combined));
    }

    heap_free(preliminary);
    return hr;
}

HRESULT WINAPI HashData(const unsigned char *src, DWORD src_len, unsigned char *dest, DWORD dest_len)
{
    INT src_count = src_len - 1, dest_count = dest_len - 1;

    if (!src || !dest)
        return E_INVALIDARG;

    while (dest_count >= 0)
    {
        dest[dest_count] = (dest_count & 0xff);
        dest_count--;
    }

    while (src_count >= 0)
    {
        dest_count = dest_len - 1;
        while (dest_count >= 0)
        {
            dest[dest_count] = hashdata_lookup[src[src_count] ^ dest[dest_count]];
            dest_count--;
        }
        src_count--;
    }

    return S_OK;
}

HRESULT WINAPI UrlHashA(const char *url, unsigned char *dest, DWORD dest_len)
{
    __TRY
    {
        HashData((const BYTE *)url, (int)strlen(url), dest, dest_len);
    }
    __EXCEPT_PAGE_FAULT
    {
        return E_INVALIDARG;
    }
    __ENDTRY
    return S_OK;
}

HRESULT WINAPI UrlHashW(const WCHAR *url, unsigned char *dest, DWORD dest_len)
{
    char urlA[MAX_PATH];

    TRACE("%s, %p, %ld\n", debugstr_w(url), dest, dest_len);

    __TRY
    {
        WideCharToMultiByte(CP_ACP, 0, url, -1, urlA, MAX_PATH, NULL, NULL);
        HashData((const BYTE *)urlA, (int)strlen(urlA), dest, dest_len);
    }
    __EXCEPT_PAGE_FAULT
    {
        return E_INVALIDARG;
    }
    __ENDTRY
    return S_OK;
}

BOOL WINAPI IsInternetESCEnabled(void)
{
    FIXME(": stub\n");
    return FALSE;
}
