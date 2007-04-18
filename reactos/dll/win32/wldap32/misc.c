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

#include "wine/port.h"
#include "wine/debug.h"

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#ifdef HAVE_LDAP_H
#include <ldap.h>
#else
#define LDAP_SUCCESS        0x00
#define LDAP_NOT_SUPPORTED  0x5c
#endif

#include "winldap_private.h"
#include "wldap32.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_abandon     (WLDAP32.@)
 *
 * Cancel an asynchronous operation.
 *
 * PARAMS
 *  ld    [I] Pointer to an LDAP context.
 *  msgid [I] ID of the operation to cancel.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL WLDAP32_ldap_abandon( WLDAP32_LDAP *ld, ULONG msgid )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p, 0x%08x)\n", ld, msgid );

    if (!ld) return ~0UL;
    ret = ldap_abandon_ext( ld, msgid, NULL, NULL );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_check_filterA     (WLDAP32.@)
 *
 * See ldap_check_filterW.
 */
ULONG CDECL ldap_check_filterA( WLDAP32_LDAP *ld, PCHAR filter )
{
    ULONG ret;
    WCHAR *filterW = NULL;

    TRACE( "(%p, %s)\n", ld, debugstr_a(filter) );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (filter) {
        filterW = strAtoW( filter );
        if (!filterW) return WLDAP32_LDAP_NO_MEMORY;
    }

    ret = ldap_check_filterW( ld, filterW );

    strfreeW( filterW );
    return ret;
}

/***********************************************************************
 *      ldap_check_filterW     (WLDAP32.@)
 *
 * Check filter syntax.
 *
 * PARAMS
 *  ld     [I] Pointer to an LDAP context.
 *  filter [I] Filter string.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_check_filterW( WLDAP32_LDAP *ld, PWCHAR filter )
{
    TRACE( "(%p, %s)\n", ld, debugstr_w(filter) );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    return LDAP_SUCCESS; /* FIXME: do some checks */
}

/***********************************************************************
 *      ldap_cleanup     (WLDAP32.@)
 */
ULONG CDECL ldap_cleanup( HANDLE instance )
{
    TRACE( "(%p)\n", instance );
    return LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_conn_from_msg     (WLDAP32.@)
 *
 * Get the LDAP context for a given message.
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *  res [I] LDAP message.
 *
 * RETURNS
 *  Success: Pointer to an LDAP context.
 *  Failure: NULL
 */
WLDAP32_LDAP * CDECL ldap_conn_from_msg( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
    TRACE( "(%p, %p)\n", ld, res );

    if (!ld || !res) return NULL;
    return ld; /* FIXME: not always correct */
}

/***********************************************************************
 *      ldap_count_entries     (WLDAP32.@)
 *
 * Count the number of entries returned from a search.
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *  res [I] LDAP message.
 *
 * RETURNS
 *  Success: The number of entries.
 *  Failure: ~0UL
 */
ULONG CDECL WLDAP32_ldap_count_entries( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p, %p)\n", ld, res );

    if (!ld) return ~0UL;
    ret = ldap_count_entries( ld, res );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_count_references     (WLDAP32.@)
 *
 * Count the number of references returned from a search.
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *  res [I] LDAP message.
 *
 * RETURNS
 *  Success: The number of references.
 *  Failure: ~0UL
 */
ULONG CDECL WLDAP32_ldap_count_references( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP_COUNT_REFERENCES

    TRACE( "(%p, %p)\n", ld, res );

    if (!ld) return 0;
    ret = ldap_count_references( ld, res );

#endif
    return ret;
}

static ULONG get_escape_size( PCHAR src, ULONG srclen )
{
    ULONG i, size = 0;

    if (src)
    {
        for (i = 0; i < srclen; i++)
        {
            if ((src[i] >= '0' && src[i] <= '9') ||
                (src[i] >= 'A' && src[i] <= 'Z') ||
                (src[i] >= 'a' && src[i] <= 'z'))
                size++;
            else
                size += 3;
        }
    }
    return size + 1;
}

static void escape_filter_element( PCHAR src, ULONG srclen, PCHAR dst )
{
    ULONG i;
    static const char fmt[] = "\\%02X";
    char *d = dst;

    for (i = 0; i < srclen; i++)
    {
        if ((src[i] >= '0' && src[i] <= '9') ||
            (src[i] >= 'A' && src[i] <= 'Z') ||
            (src[i] >= 'a' && src[i] <= 'z'))
            *d++ = src[i];
        else
        {
            sprintf( d, fmt, (unsigned char)src[i] );
            d += 3;
        }
    }
    *++d = 0;
}

/***********************************************************************
 *      ldap_escape_filter_elementA     (WLDAP32.@)
 *
 * See ldap_escape_filter_elementW.
 */
ULONG CDECL ldap_escape_filter_elementA( PCHAR src, ULONG srclen, PCHAR dst, ULONG dstlen )
{
    ULONG len;

    TRACE( "(%p, 0x%08x, %p, 0x%08x)\n", src, srclen, dst, dstlen );

    len = get_escape_size( src, srclen );
    if (!dst) return len;

    if (!src || dstlen < len)
        return WLDAP32_LDAP_PARAM_ERROR;
    else
    {
        escape_filter_element( src, srclen, dst );
        return LDAP_SUCCESS;
    }
}

/***********************************************************************
 *      ldap_escape_filter_elementW     (WLDAP32.@)
 *
 * Escape binary data for safe passing in filters.
 *
 * PARAMS
 *  src    [I] Filter element to be escaped.
 *  srclen [I] Length in bytes of the filter element.
 *  dst    [O] Destination buffer for the escaped filter element.
 *  dstlen [I] Length in bytes of the destination buffer.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_escape_filter_elementW( PCHAR src, ULONG srclen, PWCHAR dst, ULONG dstlen )
{
    ULONG len;

    TRACE( "(%p, 0x%08x, %p, 0x%08x)\n", src, srclen, dst, dstlen );

    len = get_escape_size( src, srclen );
    if (!dst) return len;

    /* no matter what you throw at it, this is what native returns */
    return WLDAP32_LDAP_PARAM_ERROR;
}

/***********************************************************************
 *      ldap_first_attributeA     (WLDAP32.@)
 *
 * See ldap_first_attributeW.
 */
PCHAR CDECL ldap_first_attributeA( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry,
    WLDAP32_BerElement** ptr )
{
    PCHAR ret = NULL;
#ifdef HAVE_LDAP
    WCHAR *retW;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry) return NULL;
    retW = ldap_first_attributeW( ld, entry, ptr );

    ret = strWtoA( retW );
    ldap_memfreeW( retW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_first_attributeW     (WLDAP32.@)
 *
 * Get the first attribute for a given entry. 
 *
 * PARAMS
 *  ld    [I] Pointer to an LDAP context.
 *  entry [I] Entry to retrieve attribute for.
 *  ptr   [O] Position pointer.
 *
 * RETURNS
 *  Success: Name of the first attribute.
 *  Failure: NULL
 *
 * NOTES
 *  Use ldap_memfree to free the returned string.
 */
PWCHAR CDECL ldap_first_attributeW( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry,
    WLDAP32_BerElement** ptr )
{
    PWCHAR ret = NULL;
#ifdef HAVE_LDAP
    char *retU;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry) return NULL;
    retU = ldap_first_attribute( ld, entry, ptr );

    ret = strUtoW( retU );
    ldap_memfree( retU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_first_entry     (WLDAP32.@)
 *
 * Get the first entry from a result message.
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *  res [I] Search result message.
 *
 * RETURNS
 *  Success: The first entry.
 *  Failure: NULL
 *
 * NOTES
 *  The returned entry will be freed when the message is freed. 
 */
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_first_entry( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
#ifdef HAVE_LDAP

    TRACE( "(%p, %p)\n", ld, res );

    if (!ld || !res) return NULL;
    return ldap_first_entry( ld, res );

#endif
    return NULL;
}

/***********************************************************************
 *      ldap_first_reference     (WLDAP32.@)
 *
 * Get the first reference from a result message.
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *  res [I] Search result message.
 *
 * RETURNS
 *  Success: The first reference.
 *  Failure: NULL
 */
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_first_reference( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
#ifdef HAVE_LDAP_FIRST_REFERENCE

    TRACE( "(%p, %p)\n", ld, res );

    if (!ld) return NULL;
    return ldap_first_reference( ld, res );

#endif
    return NULL;
}

/***********************************************************************
 *      ldap_memfreeA     (WLDAP32.@)
 *
 * See ldap_memfreeW.
 */
void CDECL ldap_memfreeA( PCHAR block )
{
    TRACE( "(%p)\n", block );
    strfreeA( block );
}

/***********************************************************************
 *      ldap_memfreeW     (WLDAP32.@)
 *
 * Free a block of memory.
 *
 * PARAMS
 *  block [I] Pointer to memory block to be freed.
 */
void CDECL ldap_memfreeW( PWCHAR block )
{
    TRACE( "(%p)\n", block );
    strfreeW( block );
}

/***********************************************************************
 *      ldap_msgfree     (WLDAP32.@)
 *
 * Free a message.
 *
 * PARAMS
 *  res [I] Message to be freed.
 */
ULONG CDECL WLDAP32_ldap_msgfree( WLDAP32_LDAPMessage *res )
{
    ULONG ret = LDAP_SUCCESS;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", res );
    ldap_msgfree( res );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_next_attributeA     (WLDAP32.@)
 *
 * See ldap_next_attributeW.
 */
PCHAR CDECL ldap_next_attributeA( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry,
    WLDAP32_BerElement *ptr )
{
    PCHAR ret = NULL;
#ifdef HAVE_LDAP
    WCHAR *retW;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry || !ptr) return NULL;
    retW = ldap_next_attributeW( ld, entry, ptr );

    ret = strWtoA( retW );
    ldap_memfreeW( retW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_next_attributeW     (WLDAP32.@)
 *
 * Get the next attribute for a given entry.
 *
 * PARAMS
 *  ld    [I]   Pointer to an LDAP context.
 *  entry [I]   Entry to retrieve attribute for.
 *  ptr   [I/O] Position pointer.
 *
 * RETURNS
 *  Success: The name of the next attribute.
 *  Failure: NULL
 *
 * NOTES
 *  Free the returned string after each iteration with ldap_memfree.
 *  When done iterating and when ptr != NULL, call ber_free( ptr, 0 ).
 */
PWCHAR CDECL ldap_next_attributeW( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry,
    WLDAP32_BerElement *ptr )
{
    PWCHAR ret = NULL;
#ifdef HAVE_LDAP
    char *retU;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry || !ptr) return NULL;
    retU = ldap_next_attribute( ld, entry, ptr );

    ret = strUtoW( retU );
    ldap_memfree( retU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_next_entry     (WLDAP32.@)
 *
 * Get the next entry from a result message.
 *
 * PARAMS
 *  ld    [I] Pointer to an LDAP context.
 *  entry [I] Entry returned by a previous call.
 *
 * RETURNS
 *  Success: The next entry.
 *  Failure: NULL
 *
 * NOTES
 *  The returned entry will be freed when the message is freed.
 */
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_next_entry( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry )
{
#ifdef HAVE_LDAP

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;
    return ldap_next_entry( ld, entry );

#endif
    return NULL;
}

/***********************************************************************
 *      ldap_next_reference     (WLDAP32.@)
 *
 * Get the next reference from a result message.
 *
 * PARAMS
 *  ld    [I] Pointer to an LDAP context.
 *  entry [I] Entry returned by a previous call.
 *
 * RETURNS
 *  Success: The next reference.
 *  Failure: NULL
 *
 * NOTES
 *  The returned entry will be freed when the message is freed.
 */
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_next_reference( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry )
{
#ifdef HAVE_LDAP_NEXT_REFERENCE

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;
    return ldap_next_reference( ld, entry );

#endif
    return NULL;
}

/***********************************************************************
 *      ldap_result     (WLDAP32.@)
 *
 * Get the result of an asynchronous operation.
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  msgid   [I] Message ID of the operation.
 *  all     [I] How many results should be returned?
 *  timeout [I] How long to wait for the results?
 *  res     [O] Result message for the operation.
 *
 * RETURNS
 *  Success: One of the following values:
 *
 *   LDAP_RES_ADD
 *   LDAP_RES_BIND
 *   LDAP_RES_COMPARE
 *   LDAP_RES_DELETE
 *   LDAP_RES_EXTENDED
 *   LDAP_RES_MODIFY
 *   LDAP_RES_MODRDN
 *   LDAP_RES_REFERRAL
 *   LDAP_RES_SEARCH_ENTRY
 *   LDAP_RES_SEARCH_RESULT
 *
 *  Failure: ~0UL
 *
 *  This function returns 0 when the timeout has expired.
 *
 * NOTES
 *  A NULL timeout pointer causes the function to block waiting
 *  for results to arrive. A timeout value of 0 causes the function
 *  to immediately return any available results. Free returned results
 *  with ldap_msgfree.
 */
ULONG CDECL WLDAP32_ldap_result( WLDAP32_LDAP *ld, ULONG msgid, ULONG all,
    struct l_timeval *timeout, WLDAP32_LDAPMessage **res )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p, 0x%08x, 0x%08x, %p, %p)\n", ld, msgid, all, timeout, res );

    if (!ld || !res || msgid == ~0UL) return ~0UL;
    ret = ldap_result( ld, msgid, all, (struct timeval *)timeout, res );

#endif
    return ret;
}

/***********************************************************************
 *      LdapUnicodeToUTF8     (WLDAP32.@)
 *
 * Convert a wide character string to a UTF8 string.
 *
 * PARAMS
 *  src    [I] Wide character string to convert.
 *  srclen [I] Size of string to convert, in characters.
 *  dst    [O] Pointer to a buffer that receives the converted string.
 *  dstlen [I] Size of the destination buffer in characters. 
 *
 * RETURNS
 *  The number of characters written into the destination buffer.
 *
 * NOTES
 *  Set dstlen to zero to ask for the required buffer size.
 */
int CDECL LdapUnicodeToUTF8( LPCWSTR src, int srclen, LPSTR dst, int dstlen )
{
    return WideCharToMultiByte( CP_UTF8, 0, src, srclen, dst, dstlen, NULL, NULL );
}

/***********************************************************************
 *      LdapUTF8ToUnicode     (WLDAP32.@)
 *
 * Convert a UTF8 string to a wide character string.
 *
 * PARAMS
 *  src    [I] UTF8 string to convert.
 *  srclen [I] Size of string to convert, in characters.
 *  dst    [O] Pointer to a buffer that receives the converted string.
 *  dstlen [I] Size of the destination buffer in characters. 
 *
 * RETURNS
 *  The number of characters written into the destination buffer.
 *
 * NOTES
 *  Set dstlen to zero to ask for the required buffer size.
 */
int CDECL LdapUTF8ToUnicode( LPCSTR src, int srclen, LPWSTR dst, int dstlen )
{
    return MultiByteToWideChar( CP_UTF8, 0, src, srclen, dst, dstlen );
}
