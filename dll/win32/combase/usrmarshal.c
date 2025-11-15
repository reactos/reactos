/*
 * Marshaling Routines
 *
 * Copyright 2005 Robert Shearman
 * Copyright 2009 Huw Davies
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

#define COBJMACROS
#include "ole2.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align))&~(_Align))
#define ALIGNED_POINTER(_Ptr, _Align) ((LPVOID)ALIGNED_LENGTH((ULONG_PTR)(_Ptr), _Align))
#define ALIGN_LENGTH(_Len, _Align) _Len = ALIGNED_LENGTH(_Len, _Align)
#define ALIGN_POINTER(_Ptr, _Align) _Ptr = ALIGNED_POINTER(_Ptr, _Align)

static const char* debugstr_user_flags(ULONG *pFlags)
{
    char buf[12];
    const char* loword;
    switch (LOWORD(*pFlags))
    {
    case MSHCTX_LOCAL:
        loword = "MSHCTX_LOCAL";
        break;
    case MSHCTX_NOSHAREDMEM:
        loword = "MSHCTX_NOSHAREDMEM";
        break;
    case MSHCTX_DIFFERENTMACHINE:
        loword = "MSHCTX_DIFFERENTMACHINE";
        break;
    case MSHCTX_INPROC:
        loword = "MSHCTX_INPROC";
        break;
    default:
        sprintf(buf, "%d", LOWORD(*pFlags));
        loword=buf;
    }

    if (HIWORD(*pFlags) == NDR_LOCAL_DATA_REPRESENTATION)
        return wine_dbg_sprintf("MAKELONG(%s, NDR_LOCAL_DATA_REPRESENTATION)", loword);
    else
        return wine_dbg_sprintf("MAKELONG(%s, 0x%04x)", loword, HIWORD(*pFlags));
}

static ULONG handle_UserSize(ULONG *pFlags, ULONG StartingSize, HANDLE *handle)
{
    if (LOWORD(*pFlags) == MSHCTX_DIFFERENTMACHINE)
    {
        ERR("can't remote a local handle\n");
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);
        return StartingSize;
    }

    ALIGN_LENGTH(StartingSize, 3);
    return StartingSize + sizeof(RemotableHandle);
}

static unsigned char * handle_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, HANDLE *handle)
{
    RemotableHandle *remhandle;
    if (LOWORD(*pFlags) == MSHCTX_DIFFERENTMACHINE)
    {
        ERR("can't remote a local handle\n");
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);
        return pBuffer;
    }

    ALIGN_POINTER(pBuffer, 3);
    remhandle = (RemotableHandle *)pBuffer;
    remhandle->fContext = WDT_INPROC_CALL;
    remhandle->u.hInproc = (LONG_PTR)*handle;
    return pBuffer + sizeof(RemotableHandle);
}

static unsigned char * handle_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, HANDLE *handle)
{
    RemotableHandle *remhandle;

    ALIGN_POINTER(pBuffer, 3);
    remhandle = (RemotableHandle *)pBuffer;
    if (remhandle->fContext != WDT_INPROC_CALL)
        RaiseException(RPC_X_BAD_STUB_DATA, 0, 0, NULL);
    *handle = (HANDLE)(LONG_PTR)remhandle->u.hInproc;
    return pBuffer + sizeof(RemotableHandle);
}

static void handle_UserFree(ULONG *pFlags, HANDLE *handle)
{
    /* nothing to do */
}

#define IMPL_WIREM_HANDLE(type) \
    ULONG __RPC_USER type##_UserSize(ULONG *pFlags, ULONG StartingSize, type *handle) \
    { \
        TRACE("%s, %lu, %p.\n", debugstr_user_flags(pFlags), StartingSize, handle); \
        return handle_UserSize(pFlags, StartingSize, (HANDLE *)handle); \
    } \
    \
    unsigned char * __RPC_USER type##_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, type *handle) \
    { \
        TRACE("%s, %p, &%p.\n", debugstr_user_flags(pFlags), pBuffer, *handle); \
        return handle_UserMarshal(pFlags, pBuffer, (HANDLE *)handle); \
    } \
    \
    unsigned char * __RPC_USER type##_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, type *handle) \
    { \
        TRACE("%s, %p, %p.\n", debugstr_user_flags(pFlags), pBuffer, handle); \
        return handle_UserUnmarshal(pFlags, pBuffer, (HANDLE *)handle); \
    } \
    \
    void __RPC_USER type##_UserFree(ULONG *pFlags, type *handle) \
    { \
        TRACE("%s, &%p.\n", debugstr_user_flags(pFlags), *handle); \
        handle_UserFree(pFlags, (HANDLE *)handle); \
    }

IMPL_WIREM_HANDLE(HACCEL)
IMPL_WIREM_HANDLE(HBRUSH)
IMPL_WIREM_HANDLE(HDC)
IMPL_WIREM_HANDLE(HICON)
IMPL_WIREM_HANDLE(HMENU)
IMPL_WIREM_HANDLE(HWND)

/******************************************************************************
 *           CLIPFORMAT_UserSize (combase.@)
 *
 * Calculates the buffer size required to marshal a clip format.
 *
 * PARAMS
 *  pFlags       [I] Flags. See notes.
 *  StartingSize [I] Starting size of the buffer. This value is added on to
 *                   the buffer size required for the clip format.
 *  pCF          [I] Clip format to size.
 *
 * RETURNS
 *  The buffer size required to marshal a clip format plus the starting size.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to an unsigned
 *  long in pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is an unsigned long.
 *  This function is only intended to be called by the RPC runtime.
 */
ULONG __RPC_USER CLIPFORMAT_UserSize(ULONG *pFlags, ULONG size, CLIPFORMAT *pCF)
{
    TRACE("%s, %lu, %p.\n", debugstr_user_flags(pFlags), size, pCF);

    ALIGN_LENGTH(size, 3);

    size += 8;

    /* only need to marshal the name if it is not a pre-defined type and
     * we are going remote */
    if ((*pCF >= 0xc000) && (LOWORD(*pFlags) == MSHCTX_DIFFERENTMACHINE))
    {
        WCHAR format[255];
        INT ret;
        size += 3 * sizeof(UINT);
        /* urg! this function is badly designed because it won't tell us how
         * much space is needed without doing a dummy run of storing the
         * name into a buffer */
        ret = GetClipboardFormatNameW(*pCF, format, ARRAY_SIZE(format)-1);
        if (!ret)
            RaiseException(DV_E_CLIPFORMAT, 0, 0, NULL);
        size += (ret + 1) * sizeof(WCHAR);
    }
    return size;
}

/******************************************************************************
 *           CLIPFORMAT_UserMarshal (combase.@)
 *
 * Marshals a clip format into a buffer.
 *
 * PARAMS
 *  pFlags  [I] Flags. See notes.
 *  pBuffer [I] Buffer to marshal the clip format into.
 *  pCF     [I] Clip format to marshal.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to an unsigned
 *  long in pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is an unsigned long.
 *  This function is only intended to be called by the RPC runtime.
 */
unsigned char * __RPC_USER CLIPFORMAT_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, CLIPFORMAT *pCF)
{
    TRACE("%s, %p, &0x%04x.\n", debugstr_user_flags(pFlags), pBuffer, *pCF);

    ALIGN_POINTER(pBuffer, 3);

    /* only need to marshal the name if it is not a pre-defined type and
     * we are going remote */
    if ((*pCF >= 0xc000) && (LOWORD(*pFlags) == MSHCTX_DIFFERENTMACHINE))
    {
        WCHAR format[255];
        UINT len;

        *(DWORD *)pBuffer = WDT_REMOTE_CALL;
        pBuffer += 4;
        *(DWORD *)pBuffer = *pCF;
        pBuffer += 4;

        len = GetClipboardFormatNameW(*pCF, format, ARRAY_SIZE(format)-1);
        if (!len)
            RaiseException(DV_E_CLIPFORMAT, 0, 0, NULL);
        len += 1;
        *(UINT *)pBuffer = len;
        pBuffer += sizeof(UINT);
        *(UINT *)pBuffer = 0;
        pBuffer += sizeof(UINT);
        *(UINT *)pBuffer = len;
        pBuffer += sizeof(UINT);
        TRACE("marshaling format name %s\n", debugstr_w(format));
        memcpy(pBuffer, format, len * sizeof(WCHAR));
        pBuffer += len * sizeof(WCHAR);
    }
    else
    {
        *(DWORD *)pBuffer = WDT_INPROC_CALL;
        pBuffer += 4;
        *(DWORD *)pBuffer = *pCF;
        pBuffer += 4;
    }

    return pBuffer;
}

/******************************************************************************
 *           CLIPFORMAT_UserUnmarshal (combase.@)
 *
 * Unmarshals a clip format from a buffer.
 *
 * PARAMS
 *  pFlags  [I] Flags. See notes.
 *  pBuffer [I] Buffer to marshal the clip format from.
 *  pCF     [O] Address that receive the unmarshaled clip format.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to an unsigned
 *  long in pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is an unsigned long.
 *  This function is only intended to be called by the RPC runtime.
 */
unsigned char * __RPC_USER CLIPFORMAT_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, CLIPFORMAT *pCF)
{
    LONG fContext;

    TRACE("%s, %p, %p.\n", debugstr_user_flags(pFlags), pBuffer, pCF);

    ALIGN_POINTER(pBuffer, 3);

    fContext = *(DWORD *)pBuffer;
    pBuffer += 4;

    if (fContext == WDT_INPROC_CALL)
    {
        *pCF = *(CLIPFORMAT *)pBuffer;
        pBuffer += 4;
    }
    else if (fContext == WDT_REMOTE_CALL)
    {
        CLIPFORMAT cf;
        UINT len;

        /* pointer ID for registered clip format string */
        if (*(DWORD *)pBuffer == 0)
            RaiseException(RPC_S_INVALID_BOUND, 0, 0, NULL);
        pBuffer += 4;

        len = *(UINT *)pBuffer;
        pBuffer += sizeof(UINT);
        if (*(UINT *)pBuffer != 0)
            RaiseException(RPC_S_INVALID_BOUND, 0, 0, NULL);
        pBuffer += sizeof(UINT);
        if (*(UINT *)pBuffer != len)
            RaiseException(RPC_S_INVALID_BOUND, 0, 0, NULL);
        pBuffer += sizeof(UINT);
        if (((WCHAR *)pBuffer)[len - 1] != '\0')
            RaiseException(RPC_S_INVALID_BOUND, 0, 0, NULL);
        TRACE("unmarshaling clip format %s\n", debugstr_w((LPCWSTR)pBuffer));
        cf = RegisterClipboardFormatW((LPCWSTR)pBuffer);
        pBuffer += len * sizeof(WCHAR);
        if (!cf)
            RaiseException(DV_E_CLIPFORMAT, 0, 0, NULL);
        *pCF = cf;
    }
    else
        /* code not really appropriate, but nearest I can find */
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);
    return pBuffer;
}

/******************************************************************************
 *           CLIPFORMAT_UserFree (combase.@)
 *
 * Frees an unmarshaled clip format.
 *
 * PARAMS
 *  pFlags  [I] Flags. See notes.
 *  pCF     [I] Clip format to free.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to an unsigned
 *  long in pFlags, it actually takes a pointer to a USER_MARSHAL_CB
 *  structure, of which the first parameter is an unsigned long.
 *  This function is only intended to be called by the RPC runtime.
 */
void __RPC_USER CLIPFORMAT_UserFree(ULONG *pFlags, CLIPFORMAT *pCF)
{
    /* there is no inverse of the RegisterClipboardFormat function,
     * so nothing to do */
}

/******************************************************************************
 *           HBITMAP_UserSize (combase.@)
 *
 * Calculates the buffer size required to marshal a bitmap.
 *
 * PARAMS
 *  pFlags       [I] Flags. See notes.
 *  StartingSize [I] Starting size of the buffer. This value is added on to
 *                   the buffer size required for the clip format.
 *  phBmp        [I] Bitmap to size.
 *
 * RETURNS
 *  The buffer size required to marshal an bitmap plus the starting size.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is a ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
ULONG __RPC_USER HBITMAP_UserSize(ULONG *flags, ULONG size, HBITMAP *bmp)
{
    TRACE("%s, %lu, %p.\n", debugstr_user_flags(flags), size, *bmp);

    ALIGN_LENGTH(size, 3);

    size += sizeof(ULONG);
    if (LOWORD(*flags) == MSHCTX_INPROC)
        size += sizeof(ULONG);
    else
    {
        size += sizeof(ULONG);

        if (*bmp)
        {
            size += sizeof(ULONG);
            size += FIELD_OFFSET(userBITMAP, cbSize);
            size += GetBitmapBits(*bmp, 0, NULL);
        }
    }

    return size;
}

/******************************************************************************
*           HBITMAP_UserMarshal (combase.@)
*
* Marshals a bitmap into a buffer.
*
* PARAMS
*  pFlags  [I] Flags. See notes.
*  pBuffer [I] Buffer to marshal the clip format into.
*  phBmp   [I] Bitmap to marshal.
*
* RETURNS
*  The end of the marshaled data in the buffer.
*
* NOTES
*  Even though the function is documented to take a pointer to a ULONG in
*  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
*  the first parameter is a ULONG.
*  This function is only intended to be called by the RPC runtime.
*/
unsigned char * __RPC_USER HBITMAP_UserMarshal(ULONG *flags, unsigned char *buffer, HBITMAP *bmp)
{
    TRACE("(%s, %p, %p)\n", debugstr_user_flags(flags), buffer, *bmp);

    ALIGN_POINTER(buffer, 3);

    if (LOWORD(*flags) == MSHCTX_INPROC)
    {
        *(ULONG *)buffer = WDT_INPROC_CALL;
        buffer += sizeof(ULONG);
        *(ULONG *)buffer = (ULONG)(ULONG_PTR)*bmp;
        buffer += sizeof(ULONG);
    }
    else
    {
        *(ULONG *)buffer = WDT_REMOTE_CALL;
        buffer += sizeof(ULONG);
        *(ULONG *)buffer = (ULONG)(ULONG_PTR)*bmp;
        buffer += sizeof(ULONG);

        if (*bmp)
        {
            static const ULONG header_size = FIELD_OFFSET(userBITMAP, cbSize);
            BITMAP bitmap;
            ULONG bitmap_size;

            bitmap_size = GetBitmapBits(*bmp, 0, NULL);
            *(ULONG *)buffer = bitmap_size;
            buffer += sizeof(ULONG);

            GetObjectW(*bmp, sizeof(BITMAP), &bitmap);
            memcpy(buffer, &bitmap, header_size);
            buffer += header_size;

            GetBitmapBits(*bmp, bitmap_size, buffer);
            buffer += bitmap_size;
        }
    }
    return buffer;
}

/******************************************************************************
 *           HBITMAP_UserUnmarshal (combase.@)
 *
 * Unmarshals a bitmap from a buffer.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  pBuffer  [I] Buffer to marshal the clip format from.
 *  phBmp    [O] Address that receive the unmarshaled bitmap.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to an ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is an ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
unsigned char * __RPC_USER HBITMAP_UserUnmarshal(ULONG *flags, unsigned char *buffer, HBITMAP *bmp)
{
    ULONG context;

    TRACE("(%s, %p, %p)\n", debugstr_user_flags(flags), buffer, bmp);

    ALIGN_POINTER(buffer, 3);

    context = *(ULONG *)buffer;
    buffer += sizeof(ULONG);

    if (context == WDT_INPROC_CALL)
    {
        *bmp = *(HBITMAP *)buffer;
        buffer += sizeof(*bmp);
    }
    else if (context == WDT_REMOTE_CALL)
    {
        ULONG handle = *(ULONG *)buffer;
        buffer += sizeof(ULONG);

        if (handle)
        {
            static const ULONG header_size = FIELD_OFFSET(userBITMAP, cbSize);
            BITMAP bitmap;
            ULONG bitmap_size;
            unsigned char *bits;

            bitmap_size = *(ULONG *)buffer;
            buffer += sizeof(ULONG);
            bits = malloc(bitmap_size);

            memcpy(&bitmap, buffer, header_size);
            buffer += header_size;

            memcpy(bits, buffer, bitmap_size);
            buffer += bitmap_size;

            bitmap.bmBits = bits;
            *bmp = CreateBitmapIndirect(&bitmap);

            free(bits);
        }
        else
            *bmp = NULL;
    }
    else
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);

    return buffer;
}

/******************************************************************************
 *           HBITMAP_UserFree (combase.@)
 *
 * Frees an unmarshaled bitmap.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  phBmp    [I] Bitmap to free.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of
 *  which the first parameter is a ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
void __RPC_USER HBITMAP_UserFree(ULONG *flags, HBITMAP *bmp)
{
    TRACE("(%s, %p)\n", debugstr_user_flags(flags), *bmp);

    if (LOWORD(*flags) != MSHCTX_INPROC)
        DeleteObject(*bmp);
}

/******************************************************************************
 *           HPALETTE_UserSize (combase.@)
 *
 * Calculates the buffer size required to marshal a palette.
 *
 * PARAMS
 *  pFlags       [I] Flags. See notes.
 *  StartingSize [I] Starting size of the buffer. This value is added on to
 *                   the buffer size required for the clip format.
 *  phPal        [I] Palette to size.
 *
 * RETURNS
 *  The buffer size required to marshal a palette plus the starting size.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is a ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
ULONG __RPC_USER HPALETTE_UserSize(ULONG *pFlags, ULONG StartingSize, HPALETTE *phPal)
{
    FIXME(":stub\n");
    return StartingSize;
}

/******************************************************************************
 *           HPALETTE_UserMarshal (combase.@)
 *
 * Marshals a palette into a buffer.
 *
 * PARAMS
 *  pFlags  [I] Flags. See notes.
 *  pBuffer [I] Buffer to marshal the clip format into.
 *  phPal   [I] Palette to marshal.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is a ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
unsigned char * __RPC_USER HPALETTE_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, HPALETTE *phPal)
{
    FIXME(":stub\n");
    return pBuffer;
}

/******************************************************************************
 *           HPALETTE_UserUnmarshal (combase.@)
 *
 * Unmarshals a palette from a buffer.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  pBuffer  [I] Buffer to marshal the clip format from.
 *  phPal    [O] Address that receive the unmarshaled palette.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to an ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is an ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
unsigned char * __RPC_USER HPALETTE_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, HPALETTE *phPal)
{
    FIXME(":stub\n");
    return pBuffer;
}

/******************************************************************************
 *           HGLOBAL_UserSize    (combase.@)
 *
 * Calculates the buffer size required to marshal an HGLOBAL.
 *
 * PARAMS
 *  pFlags       [I] Flags. See notes.
 *  StartingSize [I] Starting size of the buffer. This value is added on to
 *                   the buffer size required for the clip format.
 *  phGlobal     [I] HGLOBAL to size.
 *
 * RETURNS
 *  The buffer size required to marshal an HGLOBAL plus the starting size.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is a ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
ULONG __RPC_USER HGLOBAL_UserSize(ULONG *pFlags, ULONG StartingSize, HGLOBAL *phGlobal)
{
    ULONG size = StartingSize;

    TRACE("%s, %lu, %p.\n", debugstr_user_flags(pFlags), StartingSize, phGlobal);

    ALIGN_LENGTH(size, 3);

    size += sizeof(ULONG);

    if (LOWORD(*pFlags) == MSHCTX_INPROC)
        size += sizeof(HGLOBAL);
    else
    {
        size += sizeof(ULONG);
        if (*phGlobal)
        {
            SIZE_T ret;
            size += 3 * sizeof(ULONG);
            ret = GlobalSize(*phGlobal);
            size += (ULONG)ret;
        }
    }

    return size;
}

/******************************************************************************
 *           HGLOBAL_UserMarshal        (combase.@)
 *
 * Marshals an HGLOBAL into a buffer.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  pBuffer  [I] Buffer to marshal the clip format into.
 *  phGlobal [I] HGLOBAL to marshal.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is a ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
unsigned char * __RPC_USER HGLOBAL_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, HGLOBAL *phGlobal)
{
    TRACE("%s, %p, &%p.\n", debugstr_user_flags(pFlags), pBuffer, *phGlobal);

    ALIGN_POINTER(pBuffer, 3);

    if (LOWORD(*pFlags) == MSHCTX_INPROC)
    {
        if (sizeof(*phGlobal) == 8)
            *(ULONG *)pBuffer = WDT_INPROC64_CALL;
        else
            *(ULONG *)pBuffer = WDT_INPROC_CALL;
        pBuffer += sizeof(ULONG);
        *(HGLOBAL *)pBuffer = *phGlobal;
        pBuffer += sizeof(HGLOBAL);
    }
    else
    {
        *(ULONG *)pBuffer = WDT_REMOTE_CALL;
        pBuffer += sizeof(ULONG);
        *(ULONG *)pBuffer = HandleToULong(*phGlobal);
        pBuffer += sizeof(ULONG);
        if (*phGlobal)
        {
            const unsigned char *memory;
            SIZE_T size = GlobalSize(*phGlobal);
            *(ULONG *)pBuffer = (ULONG)size;
            pBuffer += sizeof(ULONG);
            *(ULONG *)pBuffer = HandleToULong(*phGlobal);
            pBuffer += sizeof(ULONG);
            *(ULONG *)pBuffer = (ULONG)size;
            pBuffer += sizeof(ULONG);

            memory = GlobalLock(*phGlobal);
            memcpy(pBuffer, memory, size);
            pBuffer += size;
            GlobalUnlock(*phGlobal);
        }
    }

    return pBuffer;
}

/******************************************************************************
 *           HGLOBAL_UserUnmarshal        (combase.@)
 *
 * Unmarshals an HGLOBAL from a buffer.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  pBuffer  [I] Buffer to marshal the clip format from.
 *  phGlobal [O] Address that receive the unmarshaled HGLOBAL.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to an ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is an ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
unsigned char * __RPC_USER HGLOBAL_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, HGLOBAL *phGlobal)
{
    ULONG fContext;

    TRACE("%s, %p, &%p.\n", debugstr_user_flags(pFlags), pBuffer, *phGlobal);

    ALIGN_POINTER(pBuffer, 3);

    fContext = *(ULONG *)pBuffer;
    pBuffer += sizeof(ULONG);

    if (((fContext == WDT_INPROC_CALL) && (sizeof(*phGlobal) < 8)) ||
        ((fContext == WDT_INPROC64_CALL) && (sizeof(*phGlobal) == 8)))
    {
        *phGlobal = *(HGLOBAL *)pBuffer;
        pBuffer += sizeof(*phGlobal);
    }
    else if (fContext == WDT_REMOTE_CALL)
    {
        ULONG handle;

        handle = *(ULONG *)pBuffer;
        pBuffer += sizeof(ULONG);

        if (handle)
        {
            ULONG size;
            void *memory;

            size = *(ULONG *)pBuffer;
            pBuffer += sizeof(ULONG);
            /* redundancy is bad - it means you have to check consistency like
             * this: */
            if (*(ULONG *)pBuffer != handle)
            {
                RaiseException(RPC_X_BAD_STUB_DATA, 0, 0, NULL);
                return pBuffer;
            }
            pBuffer += sizeof(ULONG);
            /* redundancy is bad - it means you have to check consistency like
             * this: */
            if (*(ULONG *)pBuffer != size)
            {
                RaiseException(RPC_X_BAD_STUB_DATA, 0, 0, NULL);
                return pBuffer;
            }
            pBuffer += sizeof(ULONG);

            /* FIXME: check size is not too big */

            *phGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
            memory = GlobalLock(*phGlobal);
            memcpy(memory, pBuffer, size);
            pBuffer += size;
            GlobalUnlock(*phGlobal);
        }
        else
            *phGlobal = NULL;
    }
    else
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);

    return pBuffer;
}

/******************************************************************************
 *           HGLOBAL_UserFree        (combase.@)
 *
 * Frees an unmarshaled HGLOBAL.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  phGlobal [I] HGLOBAL to free.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of
 *  which the first parameter is a ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
void __RPC_USER HGLOBAL_UserFree(ULONG *pFlags, HGLOBAL *phGlobal)
{
    TRACE("%s, &%p.\n", debugstr_user_flags(pFlags), *phGlobal);

    if (LOWORD(*pFlags) != MSHCTX_INPROC && *phGlobal)
        GlobalFree(*phGlobal);
}

/******************************************************************************
 *           HPALETTE_UserFree (combase.@)
 *
 * Frees an unmarshaled palette.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  phPal    [I] Palette to free.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of
 *  which the first parameter is a ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
void __RPC_USER HPALETTE_UserFree(ULONG *pFlags, HPALETTE *phPal)
{
    FIXME(":stub\n");
}

/******************************************************************************
 *           WdtpInterfacePointer_UserSize (combase.@)
 *
 * Calculates the buffer size required to marshal an interface pointer.
 *
 * PARAMS
 *  pFlags       [I] Flags. See notes.
 *  RealFlags    [I] The MSHCTX to use when marshaling the interface.
 *  punk         [I] Interface pointer to size.
 *  StartingSize [I] Starting size of the buffer. This value is added on to
 *                   the buffer size required for the clip format.
 *  riid         [I] ID of interface to size.
 *
 * RETURNS
 *  The buffer size required to marshal an interface pointer plus the starting size.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is a ULONG.
 */
ULONG __RPC_USER WdtpInterfacePointer_UserSize(ULONG *pFlags, ULONG RealFlags, ULONG StartingSize, IUnknown *punk, REFIID riid)
{
    DWORD marshal_size = 0;
    HRESULT hr;

    TRACE("%s, %#lx, %lu, %p, %s.\n", debugstr_user_flags(pFlags), RealFlags, StartingSize, punk, debugstr_guid(riid));

    hr = CoGetMarshalSizeMax(&marshal_size, riid, punk, LOWORD(RealFlags), NULL, MSHLFLAGS_NORMAL);
    if (FAILED(hr)) return StartingSize;

    ALIGN_LENGTH(StartingSize, 3);
    StartingSize += 2 * sizeof(DWORD);
    return StartingSize + marshal_size;
}

/******************************************************************************
 *           WdtpInterfacePointer_UserMarshal (combase.@)
 *
 * Marshals an interface pointer into a buffer.
 *
 * PARAMS
 *  pFlags    [I] Flags. See notes.
 *  RealFlags [I] The MSHCTX to use when marshaling the interface.
 *  pBuffer   [I] Buffer to marshal the clip format into.
 *  punk      [I] Interface pointer to marshal.
 *  riid      [I] ID of interface to marshal.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is a ULONG.
 */
unsigned char * WINAPI WdtpInterfacePointer_UserMarshal(ULONG *pFlags, ULONG RealFlags, unsigned char *pBuffer, IUnknown *punk, REFIID riid)
{
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, 0);
    IStream *stm;
    DWORD size;
    void *ptr;

    TRACE("%s, %#lx, %p, &%p, %s.\n", debugstr_user_flags(pFlags), RealFlags, pBuffer, punk, debugstr_guid(riid));

    if (!h) return NULL;
    if (CreateStreamOnHGlobal(h, TRUE, &stm) != S_OK)
    {
        GlobalFree(h);
        return NULL;
    }

    if (CoMarshalInterface(stm, riid, punk, LOWORD(RealFlags), NULL, MSHLFLAGS_NORMAL) != S_OK)
    {
        IStream_Release(stm);
        return pBuffer;
    }

    ALIGN_POINTER(pBuffer, 3);
    size = GlobalSize(h);

    *(DWORD *)pBuffer = size;
    pBuffer += sizeof(DWORD);
    *(DWORD *)pBuffer = size;
    pBuffer += sizeof(DWORD);

    ptr = GlobalLock(h);
    memcpy(pBuffer, ptr, size);
    GlobalUnlock(h);

    IStream_Release(stm);
    return pBuffer + size;
}

/******************************************************************************
 *           WdtpInterfacePointer_UserUnmarshal (combase.@)
 *
 * Unmarshals an interface pointer from a buffer.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  pBuffer  [I] Buffer to marshal the clip format from.
 *  ppunk    [I/O] Address that receives the unmarshaled interface pointer.
 *  riid     [I] ID of interface to unmarshal.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to an ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is an ULONG.
 */
unsigned char * WINAPI WdtpInterfacePointer_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, IUnknown **ppunk, REFIID riid)
{
    HRESULT hr;
    HGLOBAL h;
    IStream *stm;
    DWORD size;
    void *ptr;
    IUnknown *orig;

    TRACE("%s, %p, %p, %s.\n", debugstr_user_flags(pFlags), pBuffer, ppunk, debugstr_guid(riid));

    ALIGN_POINTER(pBuffer, 3);

    size = *(DWORD *)pBuffer;
    pBuffer += sizeof(DWORD);
    if (size != *(DWORD *)pBuffer)
        RaiseException(RPC_X_BAD_STUB_DATA, 0, 0, NULL);

    pBuffer += sizeof(DWORD);

    /* FIXME: sanity check on size */

    h = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!h) RaiseException(RPC_X_NO_MEMORY, 0, 0, NULL);

    if (CreateStreamOnHGlobal(h, TRUE, &stm) != S_OK)
    {
        GlobalFree(h);
        RaiseException(RPC_X_NO_MEMORY, 0, 0, NULL);
    }

    ptr = GlobalLock(h);
    memcpy(ptr, pBuffer, size);
    GlobalUnlock(h);

    orig = *ppunk;
    hr = CoUnmarshalInterface(stm, riid, (void**)ppunk);
    IStream_Release(stm);

    if (hr != S_OK) RaiseException(hr, 0, 0, NULL);

    if (orig) IUnknown_Release(orig);

    return pBuffer + size;
}

/******************************************************************************
 *           WdtpInterfacePointer_UserFree (combase.@)
 */
void WINAPI WdtpInterfacePointer_UserFree(IUnknown *punk)
{
    TRACE("%p.\n", punk);
    if (punk) IUnknown_Release(punk);
}
