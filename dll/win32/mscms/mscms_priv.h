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

#ifdef HAVE_LCMS2
#include <lcms2.h>

/*  A simple structure to tie together a pointer to an icc profile, an lcms
 *  color profile handle and a Windows file handle. If the profile is memory
 *  based the file handle field is set to INVALID_HANDLE_VALUE. The 'access'
 *  field records the access parameter supplied to an OpenColorProfile()
 *  call, i.e. PROFILE_READ or PROFILE_READWRITE.
 */

struct profile
{
    HANDLE      file;
    DWORD       access;
    char       *data;
    DWORD       size;
    cmsHPROFILE cmsprofile;
};

struct transform
{
    cmsHTRANSFORM cmstransform;
};

extern HPROFILE create_profile( struct profile * ) DECLSPEC_HIDDEN;
extern BOOL close_profile( HPROFILE ) DECLSPEC_HIDDEN;

extern HTRANSFORM create_transform( struct transform * ) DECLSPEC_HIDDEN;
extern BOOL close_transform( HTRANSFORM ) DECLSPEC_HIDDEN;

struct profile *grab_profile( HPROFILE ) DECLSPEC_HIDDEN;
struct transform *grab_transform( HTRANSFORM ) DECLSPEC_HIDDEN;

void release_profile( struct profile * ) DECLSPEC_HIDDEN;
void release_transform( struct transform * ) DECLSPEC_HIDDEN;

extern void free_handle_tables( void ) DECLSPEC_HIDDEN;

extern BOOL get_tag_data( const struct profile *, TAGTYPE, DWORD, void *, DWORD * ) DECLSPEC_HIDDEN;
extern BOOL set_tag_data( const struct profile *, TAGTYPE, DWORD, const void *, DWORD * ) DECLSPEC_HIDDEN;
extern void get_profile_header( const struct profile *, PROFILEHEADER * ) DECLSPEC_HIDDEN;
extern void set_profile_header( const struct profile *, const PROFILEHEADER * ) DECLSPEC_HIDDEN;

#endif /* HAVE_LCMS2 */

extern const char *dbgstr_tag(DWORD) DECLSPEC_HIDDEN;
