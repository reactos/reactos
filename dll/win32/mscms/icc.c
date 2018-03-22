/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004, 2005 Hans Leidekker
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winternl.h"
#include "icm.h"

#include "mscms_priv.h"

#ifdef HAVE_LCMS2

static inline void adjust_endianness32( ULONG *ptr )
{
#ifndef WORDS_BIGENDIAN
    *ptr = RtlUlongByteSwap(*ptr);
#endif
}

void get_profile_header( const struct profile *profile, PROFILEHEADER *header )
{
    unsigned int i;

    memcpy( header, profile->data, sizeof(PROFILEHEADER) );

    /* ICC format is big-endian, swap bytes if necessary */
    for (i = 0; i < sizeof(PROFILEHEADER) / sizeof(ULONG); i++)
        adjust_endianness32( (ULONG *)header + i );
}

void set_profile_header( const struct profile *profile, const PROFILEHEADER *header )
{
    unsigned int i;

    memcpy( profile->data, header, sizeof(PROFILEHEADER) );

    /* ICC format is big-endian, swap bytes if necessary */
    for (i = 0; i < sizeof(PROFILEHEADER) / sizeof(ULONG); i++)
        adjust_endianness32( (ULONG *)profile->data + i );
}

static BOOL get_adjusted_tag( const struct profile *profile, TAGTYPE type, cmsTagEntry *tag )
{
    DWORD i, num_tags = *(DWORD *)(profile->data + sizeof(cmsICCHeader));
    cmsTagEntry *entry;
    ULONG sig;

    adjust_endianness32( &num_tags );
    for (i = 0; i < num_tags; i++)
    {
        entry = (cmsTagEntry *)(profile->data + sizeof(cmsICCHeader) + sizeof(DWORD) + i * sizeof(*tag));
        sig = entry->sig;
        adjust_endianness32( &sig );
        if (sig == type)
        {
            tag->sig    = sig;
            tag->offset = entry->offset;
            tag->size   = entry->size;
            adjust_endianness32( &tag->offset );
            adjust_endianness32( &tag->size );
            return TRUE;
        }
    }
    return FALSE;
}

BOOL get_tag_data( const struct profile *profile, TAGTYPE type, DWORD offset, void *buffer, DWORD *len )
{
    cmsTagEntry tag;

    if (!get_adjusted_tag( profile, type, &tag )) return FALSE;

    if (!buffer) offset = 0;
    if (offset > tag.size) return FALSE;
    if (*len < tag.size - offset || !buffer)
    {
        *len = tag.size - offset;
        return FALSE;
    }
    memcpy( buffer, profile->data + tag.offset + offset, tag.size - offset );
    *len = tag.size - offset;
    return TRUE;
}

BOOL set_tag_data( const struct profile *profile, TAGTYPE type, DWORD offset, const void *buffer, DWORD *len )
{
    cmsTagEntry tag;

    if (!get_adjusted_tag( profile, type, &tag )) return FALSE;

    if (offset > tag.size) return FALSE;
    *len = min( tag.size - offset, *len );
    memcpy( profile->data + tag.offset + offset, buffer, *len );
    return TRUE;
}

#endif /* HAVE_LCMS2 */
