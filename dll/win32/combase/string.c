/*
 * Copyright 2014 Martin Storsjo
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

#include <string.h>
#include <wchar.h>

#include "windows.h"
#include "winerror.h"
#include "hstring.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winstring);

#define HSTRING_REFERENCE_FLAG 1

struct hstring_header
{
    UINT32 flags;
    UINT32 length;
    UINT32 padding1;
    UINT32 padding2;
    const WCHAR *str;
};

struct hstring_private
{
    struct hstring_header header;
    LONG refcount;
    WCHAR buffer[1];
};

static const WCHAR empty[1];

C_ASSERT(sizeof(struct hstring_header) <= sizeof(HSTRING_HEADER));

static inline struct hstring_private *impl_from_HSTRING(HSTRING string)
{
    return (struct hstring_private *)string;
}

static inline struct hstring_private *impl_from_HSTRING_HEADER(HSTRING_HEADER *header)
{
    return CONTAINING_RECORD(header, struct hstring_private, header);
}

static inline struct hstring_private *impl_from_HSTRING_BUFFER(HSTRING_BUFFER buffer)
{
    return CONTAINING_RECORD(buffer, struct hstring_private, buffer);
}

static BOOL alloc_string(UINT32 len, HSTRING *out)
{
    struct hstring_private *priv;
    priv = malloc(offsetof(struct hstring_private, buffer[len+1]));
    if (!priv)
        return FALSE;

    priv->header.flags = 0;
    priv->header.length = len;
    priv->header.str = priv->buffer;

    priv->refcount = 1;
    priv->buffer[len] = '\0';

    *out = (HSTRING)priv;
    return TRUE;
}

/***********************************************************************
 *      WindowsCreateString (combase.@)
 */
HRESULT WINAPI WindowsCreateString(LPCWSTR ptr, UINT32 len,
                                   HSTRING *out)
{
    struct hstring_private *priv;

    TRACE("(%s, %u, %p)\n", debugstr_wn(ptr, len), len, out);

    if (out == NULL)
        return E_INVALIDARG;
    if (len == 0)
    {
        *out = NULL;
        return S_OK;
    }
    if (ptr == NULL)
        return E_POINTER;
    if (!alloc_string(len, out))
        return E_OUTOFMEMORY;
    priv = impl_from_HSTRING(*out);
    memcpy(priv->buffer, ptr, len * sizeof(*priv->buffer));
    return S_OK;
}

/***********************************************************************
 *      WindowsCreateStringReference (combase.@)
 */
HRESULT WINAPI WindowsCreateStringReference(LPCWSTR ptr, UINT32 len,
                                            HSTRING_HEADER *header, HSTRING *out)
{
    struct hstring_private *priv = impl_from_HSTRING_HEADER(header);

    TRACE("(%s, %u, %p, %p)\n", debugstr_wn(ptr, len), len, header, out);

    if (out == NULL || header == NULL)
        return E_INVALIDARG;
    if (ptr != NULL && ptr[len] != '\0')
        return E_INVALIDARG;
    if (len == 0)
    {
        *out = NULL;
        return S_OK;
    }
    if (ptr == NULL)
        return E_POINTER;

    priv->header.str = ptr;
    priv->header.length = len;
    priv->header.flags = HSTRING_REFERENCE_FLAG;

    *out = (HSTRING)priv;
    return S_OK;
}

/***********************************************************************
 *      WindowsDeleteString (combase.@)
 */
HRESULT WINAPI WindowsDeleteString(HSTRING str)
{
    struct hstring_private *priv = impl_from_HSTRING(str);

    TRACE("(%p)\n", str);

    if (str == NULL)
        return S_OK;
    if (priv->header.flags & HSTRING_REFERENCE_FLAG)
        return S_OK;
    if (InterlockedDecrement(&priv->refcount) == 0)
        free(priv);
    return S_OK;
}

/***********************************************************************
 *      WindowsDuplicateString (combase.@)
 */
HRESULT WINAPI WindowsDuplicateString(HSTRING str, HSTRING *out)
{
    struct hstring_private *priv = impl_from_HSTRING(str);

    TRACE("(%p, %p)\n", str, out);

    if (out == NULL)
        return E_INVALIDARG;
    if (str == NULL)
    {
        *out = NULL;
        return S_OK;
    }
    if (priv->header.flags & HSTRING_REFERENCE_FLAG)
        return WindowsCreateString(priv->header.str, priv->header.length, out);
    InterlockedIncrement(&priv->refcount);
    *out = str;
    return S_OK;
}

/***********************************************************************
 *      WindowsPreallocateStringBuffer (combase.@)
 */
HRESULT WINAPI WindowsPreallocateStringBuffer(UINT32 len, WCHAR **outptr,
                                              HSTRING_BUFFER *out)
{
    struct hstring_private *priv;
    HSTRING str;

    TRACE("(%u, %p, %p)\n", len, outptr, out);

    if (outptr == NULL || out == NULL)
        return E_POINTER;
    if (len == 0)
    {
        *outptr = (LPWSTR)empty;
        *out = NULL;
        return S_OK;
    }

    if (!alloc_string(len, &str))
        return E_OUTOFMEMORY;
    priv = impl_from_HSTRING(str);
    *outptr = priv->buffer;
    *out = (HSTRING_BUFFER)&priv->buffer;
    return S_OK;
}

/***********************************************************************
 *      WindowsDeleteStringBuffer (combase.@)
 */
HRESULT WINAPI WindowsDeleteStringBuffer(HSTRING_BUFFER buf)
{
    struct hstring_private *priv = NULL;

    TRACE("(%p)\n", buf);

    if(buf)
        priv = impl_from_HSTRING_BUFFER(buf);

    return WindowsDeleteString((HSTRING)priv);
}

/***********************************************************************
 *      WindowsPromoteStringBuffer (combase.@)
 */
HRESULT WINAPI WindowsPromoteStringBuffer(HSTRING_BUFFER buf, HSTRING *out)
{
    struct hstring_private *priv = impl_from_HSTRING_BUFFER(buf);

    TRACE("(%p, %p)\n", buf, out);

    if (out == NULL)
        return E_POINTER;
    if (buf == NULL)
    {
        *out = NULL;
        return S_OK;
    }
    if (priv->buffer[priv->header.length] != 0 || priv->header.flags & HSTRING_REFERENCE_FLAG || priv->refcount != 1)
        return E_INVALIDARG;
    *out = (HSTRING)priv;
    return S_OK;
}

/***********************************************************************
 *      WindowsGetStringLen (combase.@)
 */
UINT32 WINAPI WindowsGetStringLen(HSTRING str)
{
    struct hstring_private *priv = impl_from_HSTRING(str);

    TRACE("(%p)\n", str);

    if (str == NULL)
        return 0;
    return priv->header.length;
}

/***********************************************************************
 *      WindowsGetStringRawBuffer (combase.@)
 */
LPCWSTR WINAPI WindowsGetStringRawBuffer(HSTRING str, UINT32 *len)
{
    struct hstring_private *priv = impl_from_HSTRING(str);

    TRACE("(%p, %p)\n", str, len);

    if (str == NULL)
    {
        if (len)
            *len = 0;
        return empty;
    }
    if (len)
        *len = priv->header.length;
    return priv->header.str;
}

/***********************************************************************
 *      WindowsStringHasEmbeddedNull (combase.@)
 */
HRESULT WINAPI WindowsStringHasEmbeddedNull(HSTRING str, BOOL *out)
{
    UINT32 i;
    struct hstring_private *priv = impl_from_HSTRING(str);

    TRACE("(%p, %p)\n", str, out);

    if (out == NULL)
        return E_INVALIDARG;
    if (str == NULL)
    {
        *out = FALSE;
        return S_OK;
    }
    for (i = 0; i < priv->header.length; i++)
    {
        if (priv->header.str[i] == '\0')
        {
            *out = TRUE;
            return S_OK;
        }
    }
    *out = FALSE;
    return S_OK;
}

/***********************************************************************
 *      WindowsSubstring (combase.@)
 */
HRESULT WINAPI WindowsSubstring(HSTRING str, UINT32 start, HSTRING *out)
{
    struct hstring_private *priv = impl_from_HSTRING(str);
    UINT32 len = WindowsGetStringLen(str);

    TRACE("(%p, %u, %p)\n", str, start, out);

    if (out == NULL)
        return E_INVALIDARG;
    if (start > len)
        return E_BOUNDS;
    if (start == len)
    {
        *out = NULL;
        return S_OK;
    }
    return WindowsCreateString(&priv->header.str[start], len - start, out);
}

/***********************************************************************
 *      WindowsSubstringWithSpecifiedLength (combase.@)
 */
HRESULT WINAPI WindowsSubstringWithSpecifiedLength(HSTRING str, UINT32 start, UINT32 len, HSTRING *out)
{
    struct hstring_private *priv = impl_from_HSTRING(str);

    TRACE("(%p, %u, %u, %p)\n", str, start, len, out);

    if (out == NULL)
        return E_INVALIDARG;
    if (start + len < start ||
        start + len > WindowsGetStringLen(str))
        return E_BOUNDS;
    if (len == 0)
    {
        *out = NULL;
        return S_OK;
    }
    return WindowsCreateString(&priv->header.str[start], len, out);
}

/***********************************************************************
 *      WindowsConcatString (combase.@)
 */
HRESULT WINAPI WindowsConcatString(HSTRING str1, HSTRING str2, HSTRING *out)
{
    struct hstring_private *priv1 = impl_from_HSTRING(str1);
    struct hstring_private *priv2 = impl_from_HSTRING(str2);
    struct hstring_private *priv;

    TRACE("(%p, %p, %p)\n", str1, str2, out);

    if (out == NULL)
        return E_INVALIDARG;
    if (str1 == NULL)
        return WindowsDuplicateString(str2, out);
    if (str2 == NULL)
        return WindowsDuplicateString(str1, out);
    if (!priv1->header.length && !priv2->header.length)
    {
        *out = NULL;
        return S_OK;
    }
    if (!alloc_string(priv1->header.length + priv2->header.length, out))
        return E_OUTOFMEMORY;
    priv = impl_from_HSTRING(*out);
    memcpy(priv->buffer, priv1->header.str, priv1->header.length * sizeof(*priv1->buffer));
    memcpy(priv->buffer + priv1->header.length, priv2->header.str, priv2->header.length * sizeof(*priv2->buffer));
    return S_OK;
}

/***********************************************************************
 *      WindowsIsStringEmpty (combase.@)
 */
BOOL WINAPI WindowsIsStringEmpty(HSTRING str)
{
    struct hstring_private *priv = impl_from_HSTRING(str);

    TRACE("(%p)\n", str);

    if (str == NULL)
        return TRUE;
    return priv->header.length == 0;
}

/***********************************************************************
 *      WindowsCompareStringOrdinal (combase.@)
 */
HRESULT WINAPI WindowsCompareStringOrdinal(HSTRING str1, HSTRING str2, INT32 *res)
{
    struct hstring_private *priv1 = impl_from_HSTRING(str1);
    struct hstring_private *priv2 = impl_from_HSTRING(str2);
    const WCHAR *buf1 = empty, *buf2 = empty;
    UINT32 len1 = 0, len2 = 0;

    TRACE("(%p, %p, %p)\n", str1, str2, res);

    if (res == NULL)
        return E_INVALIDARG;
    if (str1 == str2)
    {
        *res = 0;
        return S_OK;
    }
    if (str1)
    {
        buf1 = priv1->header.str;
        len1 = priv1->header.length;
    }
    if (str2)
    {
        buf2 = priv2->header.str;
        len2 = priv2->header.length;
    }
    *res = CompareStringOrdinal(buf1, len1, buf2, len2, FALSE) - CSTR_EQUAL;
    return S_OK;
}

/***********************************************************************
 *      WindowsTrimStringStart (combase.@)
 */
HRESULT WINAPI WindowsTrimStringStart(HSTRING str1, HSTRING str2, HSTRING *out)
{
    struct hstring_private *priv1 = impl_from_HSTRING(str1);
    struct hstring_private *priv2 = impl_from_HSTRING(str2);
    UINT32 start;

    TRACE("(%p, %p, %p)\n", str1, str2, out);

    if (!out || !str2 || !priv2->header.length)
        return E_INVALIDARG;
    if (!str1)
    {
        *out = NULL;
        return S_OK;
    }
    for (start = 0; start < priv1->header.length; start++)
    {
        if (!wmemchr(priv2->header.str, priv1->header.str[start], priv2->header.length))
            break;
    }
    return start ? WindowsCreateString(&priv1->header.str[start], priv1->header.length - start, out) :
                   WindowsDuplicateString(str1, out);
}

/***********************************************************************
 *      WindowsTrimStringEnd (combase.@)
 */
HRESULT WINAPI WindowsTrimStringEnd(HSTRING str1, HSTRING str2, HSTRING *out)
{
    struct hstring_private *priv1 = impl_from_HSTRING(str1);
    struct hstring_private *priv2 = impl_from_HSTRING(str2);
    UINT32 len;

    TRACE("(%p, %p, %p)\n", str1, str2, out);

    if (!out || !str2 || !priv2->header.length)
        return E_INVALIDARG;
    if (!str1)
    {
        *out = NULL;
        return S_OK;
    }
    for (len = priv1->header.length; len > 0; len--)
    {
        if (!wmemchr(priv2->header.str, priv1->header.str[len - 1], priv2->header.length))
            break;
    }
    return (len < priv1->header.length) ? WindowsCreateString(priv1->header.str, len, out) :
                                   WindowsDuplicateString(str1, out);
}
