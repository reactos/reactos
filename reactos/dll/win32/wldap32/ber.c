/*
 * WLDAP32 - LDAP support for Wine
 *
 * Copyright 2005 Hans Leidekker
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
#include "winldap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

#ifndef LBER_ERROR
# define LBER_ERROR (~0U)
#endif

/***********************************************************************
 *      ber_alloc_t     (WLDAP32.@)
 *
 * Allocate a berelement structure.
 *
 * PARAMS
 *  options [I] Must be LBER_USE_DER.
 *
 * RETURNS
 *  Success: Pointer to an allocated berelement structure.
 *  Failure: NULL
 *
 * NOTES
 *  Free the berelement structure with ber_free.
 */
BerElement * CDECL WLDAP32_ber_alloc_t( INT options )
{
#ifdef HAVE_LDAP
    return ber_alloc_t( options );
#else
    return NULL;
#endif
}


/***********************************************************************
 *      ber_bvdup     (WLDAP32.@)
 *
 * Copy a berval structure.
 *
 * PARAMS
 *  berval [I] Pointer to the berval structure to be copied.
 *
 * RETURNS
 *  Success: Pointer to a copy of the berval structure.
 *  Failure: NULL
 *
 * NOTES
 *  Free the copy with ber_bvfree.
 */
BERVAL * CDECL WLDAP32_ber_bvdup( BERVAL *berval )
{
#ifdef HAVE_LDAP
    return ber_bvdup( berval );
#else
    return NULL;
#endif
}


/***********************************************************************
 *      ber_bvecfree     (WLDAP32.@)
 *
 * Free an array of berval structures.
 *
 * PARAMS
 *  berval [I] Pointer to an array of berval structures.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Use this function only to free an array of berval structures
 *  returned by a call to ber_scanf with a 'V' in the format string.
 */
void CDECL WLDAP32_ber_bvecfree( PBERVAL *berval )
{
#ifdef HAVE_LDAP
    ber_bvecfree( berval );
#endif
}


/***********************************************************************
 *      ber_bvfree     (WLDAP32.@)
 *
 * Free a berval structure.
 *
 * PARAMS
 *  berval [I] Pointer to a berval structure.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Use this function only to free berval structures allocated by
 *  an LDAP API.
 */
void CDECL WLDAP32_ber_bvfree( BERVAL *berval )
{
#ifdef HAVE_LDAP
    ber_bvfree( berval );
#endif
}


/***********************************************************************
 *      ber_first_element     (WLDAP32.@)
 *
 * Return the tag of the first element in a set or sequence.
 *
 * PARAMS
 *  berelement [I] Pointer to a berelement structure.
 *  len        [O] Receives the length of the first element.
 *  opaque     [O] Receives a pointer to a cookie.
 *
 * RETURNS
 *  Success: Tag of the first element.
 *  Failure: LBER_DEFAULT (no more data).
 *
 * NOTES
 *  len and cookie should be passed to ber_next_element.
 */
ULONG CDECL WLDAP32_ber_first_element( BerElement *berelement, ULONG *len, CHAR **opaque )
{
#ifdef HAVE_LDAP
    return ber_first_element( berelement, len, opaque );
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_flatten     (WLDAP32.@)
 *
 * Flatten a berelement structure into a berval structure.
 *
 * PARAMS
 *  berlement [I] Pointer to a berelement structure.
 *  berval    [O] Pointer to a berval structure.
 *
 * RETURNS
 *  Success: 0
 *  Failure: LBER_ERROR
 *
 * NOTES
 *  Free the berval structure with ber_bvfree.
 */
INT CDECL WLDAP32_ber_flatten( BerElement *berelement, PBERVAL *berval )
{
#ifdef HAVE_LDAP
    return ber_flatten( berelement, berval );
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_free     (WLDAP32.@)
 *
 * Free a berelement structure.
 *
 * PARAMS
 *  berlement [I] Pointer to the berelement structure to be freed.
 *  buf       [I] Flag.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Set buf to 0 if the berelement was allocated with ldap_first_attribute
 *  or ldap_next_attribute, otherwise set it to 1.
 */
void CDECL WLDAP32_ber_free( BerElement *berelement, INT buf )
{
#ifdef HAVE_LDAP
    ber_free( berelement, buf );
#endif
}


/***********************************************************************
 *      ber_init     (WLDAP32.@)
 *
 * Initialise a berelement structure from a berval structure.
 *
 * PARAMS
 *  berval [I] Pointer to a berval structure.
 *
 * RETURNS
 *  Success: Pointer to a berelement structure.
 *  Failure: NULL
 *
 * NOTES
 *  Call ber_free to free the returned berelement structure.
 */
BerElement * CDECL WLDAP32_ber_init( BERVAL *berval )
{
#ifdef HAVE_LDAP
    return ber_init( berval );
#else
    return NULL;
#endif
}


/***********************************************************************
 *      ber_next_element     (WLDAP32.@)
 *
 * Return the tag of the next element in a set or sequence.
 *
 * PARAMS
 *  berelement [I]   Pointer to a berelement structure.
 *  len        [I/O] Receives the length of the next element.
 *  opaque     [I/O] Pointer to a cookie.
 *
 * RETURNS
 *  Success: Tag of the next element.
 *  Failure: LBER_DEFAULT (no more data).
 *
 * NOTES
 *  len and cookie are intitialised by ber_first_element and should
 *  be passed on in subsequent calls to ber_next_element.
 */
ULONG CDECL WLDAP32_ber_next_element( BerElement *berelement, ULONG *len, CHAR *opaque )
{
#ifdef HAVE_LDAP
    return ber_next_element( berelement, len, opaque );
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_peek_tag     (WLDAP32.@)
 *
 * Return the tag of the next element.
 *
 * PARAMS
 *  berelement [I] Pointer to a berelement structure.
 *  len        [O] Receives the length of the next element.
 *
 * RETURNS
 *  Success: Tag of the next element.
 *  Failure: LBER_DEFAULT (no more data).
 */
ULONG CDECL WLDAP32_ber_peek_tag( BerElement *berelement, ULONG *len )
{
#ifdef HAVE_LDAP
    return ber_peek_tag( berelement, len );
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_skip_tag     (WLDAP32.@)
 *
 * Skip the current tag and return the tag of the next element.
 *
 * PARAMS
 *  berelement [I] Pointer to a berelement structure.
 *  len        [O] Receives the length of the skipped element.
 *
 * RETURNS
 *  Success: Tag of the next element.
 *  Failure: LBER_DEFAULT (no more data).
 */
ULONG CDECL WLDAP32_ber_skip_tag( BerElement *berelement, ULONG *len )
{
#ifdef HAVE_LDAP
    return ber_skip_tag( berelement, len );
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_printf     (WLDAP32.@)
 *
 * Encode a berelement structure.
 *
 * PARAMS
 *  berelement [I/O] Pointer to a berelement structure.
 *  fmt        [I]   Format string.
 *  ...        [I]   Values to encode.
 *
 * RETURNS
 *  Success: Non-negative number. 
 *  Failure: LBER_ERROR
 *
 * NOTES
 *  berelement must have been allocated with ber_alloc_t. This function
 *  can be called multiple times to append data.
 */
INT CDECL WLDAP32_ber_printf( BerElement *berelement, PCHAR fmt, ... )
{
#ifdef HAVE_LDAP
    va_list list;
    int ret = 0;
    char new_fmt[2];

    new_fmt[1] = 0;
    va_start( list, fmt );
    while (*fmt)
    {
        new_fmt[0] = *fmt++;
        switch(new_fmt[0])
        {
        case 'b':
        case 'e':
        case 'i':
            {
                int i = va_arg( list, int );
                ret = ber_printf( berelement, new_fmt, i );
                break;
            }
        case 'o':
        case 's':
            {
                char *str = va_arg( list, char * );
                ret = ber_printf( berelement, new_fmt, str );
                break;
            }
        case 't':
            {
                unsigned int tag = va_arg( list, unsigned int );
                ret = ber_printf( berelement, new_fmt, tag );
                break;
            }
        case 'v':
            {
                char **array = va_arg( list, char ** );
                ret = ber_printf( berelement, new_fmt, array );
                break;
            }
        case 'V':
            {
                struct berval **array = va_arg( list, struct berval ** );
                ret = ber_printf( berelement, new_fmt, array );
                break;
            }
        case 'X':
            {
                char *str = va_arg( list, char * );
                int len = va_arg( list, int );
                new_fmt[0] = 'B';  /* 'X' is deprecated */
                ret = ber_printf( berelement, new_fmt, str, len );
                break;
            }
        case 'n':
        case '{':
        case '}':
        case '[':
        case ']':
            ret = ber_printf( berelement, new_fmt );
            break;
        default:
            FIXME( "Unknown format '%c'\n", new_fmt[0] );
            ret = -1;
            break;
        }
        if (ret == -1) break;
    }
    va_end( list );
    return ret;
#else
    return LBER_ERROR;
#endif
}


/***********************************************************************
 *      ber_scanf     (WLDAP32.@)
 *
 * Decode a berelement structure.
 *
 * PARAMS
 *  berelement [I/O] Pointer to a berelement structure.
 *  fmt        [I]   Format string.
 *  ...        [I]   Pointers to values to be decoded.
 *
 * RETURNS
 *  Success: Non-negative number. 
 *  Failure: LBER_ERROR
 *
 * NOTES
 *  berelement must have been allocated with ber_init. This function
 *  can be called multiple times to decode data.
 */
INT CDECL WLDAP32_ber_scanf( BerElement *berelement, PCHAR fmt, ... )
{
#ifdef HAVE_LDAP
    va_list list;
    int ret = 0;
    char new_fmt[2];

    new_fmt[1] = 0;
    va_start( list, fmt );
    while (*fmt)
    {
        new_fmt[0] = *fmt++;
        switch(new_fmt[0])
        {
        case 'a':
            {
                char **ptr = va_arg( list, char ** );
                ret = ber_scanf( berelement, new_fmt, ptr );
                break;
            }
        case 'b':
        case 'e':
        case 'i':
            {
                int *i = va_arg( list, int * );
                ret = ber_scanf( berelement, new_fmt, i );
                break;
            }
        case 't':
            {
                unsigned int *tag = va_arg( list, unsigned int * );
                ret = ber_scanf( berelement, new_fmt, tag );
                break;
            }
        case 'v':
            {
                char ***array = va_arg( list, char *** );
                ret = ber_scanf( berelement, new_fmt, array );
                break;
            }
        case 'B':
            {
                char **str = va_arg( list, char ** );
                int *len = va_arg( list, int * );
                ret = ber_scanf( berelement, new_fmt, str, len );
                break;
            }
        case 'O':
            {
                struct berval **ptr = va_arg( list, struct berval ** );
                ret = ber_scanf( berelement, new_fmt, ptr );
                break;
            }
        case 'V':
            {
                struct berval ***array = va_arg( list, struct berval *** );
                ret = ber_scanf( berelement, new_fmt, array );
                break;
            }
        case 'n':
        case 'x':
        case '{':
        case '}':
        case '[':
        case ']':
            ret = ber_scanf( berelement, new_fmt );
            break;
        default:
            FIXME( "Unknown format '%c'\n", new_fmt[0] );
            ret = -1;
            break;
        }
        if (ret == -1) break;
    }
    va_end( list );
    return ret;
#else
    return LBER_ERROR;
#endif
}
