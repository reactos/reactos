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
#include "winternl.h"
#include "icm.h"

#include "mscms_priv.h"

#ifdef HAVE_LCMS

static inline void MSCMS_adjust_endianess32( ULONG *ptr )
{
#ifndef WORDS_BIGENDIAN
    *ptr = RtlUlongByteSwap(*ptr);
#endif
}

void MSCMS_get_profile_header( const icProfile *iccprofile, PROFILEHEADER *header )
{
    unsigned int i;

    memcpy( header, iccprofile, sizeof(PROFILEHEADER) );

    /* ICC format is big-endian, swap bytes if necessary */
    for (i = 0; i < sizeof(PROFILEHEADER) / sizeof(ULONG); i++)
        MSCMS_adjust_endianess32( (ULONG *)header + i );
}

void MSCMS_set_profile_header( icProfile *iccprofile, const PROFILEHEADER *header )
{
    unsigned int i;
    icHeader *iccheader = (icHeader *)iccprofile;

    memcpy( iccheader, header, sizeof(icHeader) );

    /* ICC format is big-endian, swap bytes if necessary */
    for (i = 0; i < sizeof(icHeader) / sizeof(ULONG); i++)
        MSCMS_adjust_endianess32( (ULONG *)iccheader + i );
}

DWORD MSCMS_get_tag_count( const icProfile *iccprofile )
{
    ULONG count = iccprofile->count;

    MSCMS_adjust_endianess32( &count );
    return count;
}

void MSCMS_get_tag_by_index( icProfile *iccprofile, DWORD index, icTag *tag )
{
    icTag *tmp = (icTag *)((char *)iccprofile->data + index * sizeof(icTag));

    tag->sig = tmp->sig;
    tag->offset = tmp->offset;
    tag->size = tmp->size;

    MSCMS_adjust_endianess32( &tag->sig );
    MSCMS_adjust_endianess32( &tag->offset );
    MSCMS_adjust_endianess32( &tag->size );
}

void MSCMS_get_tag_data( const icProfile *iccprofile, const icTag *tag, DWORD offset, void *buffer )
{
    memcpy( buffer, (const char *)iccprofile + tag->offset + offset, tag->size - offset );
}

void MSCMS_set_tag_data( icProfile *iccprofile, const icTag *tag, DWORD offset, const void *buffer )
{
    memcpy( (char *)iccprofile + tag->offset + offset, buffer, tag->size - offset );
}

DWORD MSCMS_get_profile_size( const icProfile *iccprofile )
{
    DWORD size = ((const icHeader *)iccprofile)->size;

    MSCMS_adjust_endianess32( &size );
    return size;
}

#endif /* HAVE_LCMS */
