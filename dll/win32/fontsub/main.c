/*
 * Copyright 2014 Nikolay Sivov for CodeWeavers
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
#include "windef.h"
#include "winbase.h"
#include "fontsub.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(fontsub);

ULONG __cdecl CreateFontPackage(const unsigned char *src, const ULONG src_len, unsigned char **dest,
    ULONG *dest_len, ULONG *written, const unsigned short flags, const unsigned short face_index,
    const unsigned short format, const unsigned short lang, const unsigned short platform, const unsigned short encoding,
    const unsigned short *keep_list, const unsigned short keep_len, CFP_ALLOCPROC allocproc,
    CFP_REALLOCPROC reallocproc, CFP_FREEPROC freeproc, void *reserved)
{
    FIXME("(%p %lu %p %p %p %#x %u %u %u %u %u %p %u %p %p %p %p): stub\n", src, src_len, dest, dest_len,
        written, flags, face_index, format, lang, platform, encoding, keep_list, keep_len, allocproc,
        reallocproc, freeproc, reserved);

    if (format != TTFCFP_SUBSET)
        return ERR_GENERIC;

    *dest = allocproc(src_len);
    if (!*dest)
        return ERR_MEM;

    memcpy(*dest, src, src_len);
    *dest_len = src_len;
    *written = src_len;

    return NO_ERROR;
}
