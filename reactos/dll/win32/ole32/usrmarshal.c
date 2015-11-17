/*
 * Miscellaneous Marshaling Routines
 *
 * Copyright 2005 Robert Shearman
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align))&~(_Align))
#define ALIGNED_POINTER(_Ptr, _Align) ((LPVOID)ALIGNED_LENGTH((ULONG_PTR)(_Ptr), _Align))
#define ALIGN_LENGTH(_Len, _Align) _Len = ALIGNED_LENGTH(_Len, _Align)
#define ALIGN_POINTER(_Ptr, _Align) _Ptr = ALIGNED_POINTER(_Ptr, _Align)

#define USER_MARSHAL_PTR_PREFIX \
  ( (DWORD)'U'         | ( (DWORD)'s' << 8 ) | \
  ( (DWORD)'e' << 16 ) | ( (DWORD)'r' << 24 ) )

static const char* debugstr_user_flags(ULONG *pFlags)
{
    char buf[12];
    const char* loword;
    switch (LOWORD(*pFlags))
    {
    case MSHCTX_LOCAL:
        loword="MSHCTX_LOCAL";
        break;
    case MSHCTX_NOSHAREDMEM:
        loword="MSHCTX_NOSHAREDMEM";
        break;
    case MSHCTX_DIFFERENTMACHINE:
        loword="MSHCTX_DIFFERENTMACHINE";
        break;
    case MSHCTX_INPROC:
        loword="MSHCTX_INPROC";
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

/******************************************************************************
 *           CLIPFORMAT_UserSize [OLE32.@]
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
ULONG __RPC_USER CLIPFORMAT_UserSize(ULONG *pFlags, ULONG StartingSize, CLIPFORMAT *pCF)
{
    ULONG size = StartingSize;

    TRACE("(%s, %d, %p\n", debugstr_user_flags(pFlags), StartingSize, pCF);

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
        ret = GetClipboardFormatNameW(*pCF, format, sizeof(format)/sizeof(format[0])-1);
        if (!ret)
            RaiseException(DV_E_CLIPFORMAT, 0, 0, NULL);
        size += (ret + 1) * sizeof(WCHAR);
    }
    return size;
}

/******************************************************************************
 *           CLIPFORMAT_UserMarshal [OLE32.@]
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
    TRACE("(%s, %p, &0x%04x\n", debugstr_user_flags(pFlags), pBuffer, *pCF);

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

        len = GetClipboardFormatNameW(*pCF, format, sizeof(format)/sizeof(format[0])-1);
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
 *           CLIPFORMAT_UserUnmarshal [OLE32.@]
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

    TRACE("(%s, %p, %p\n", debugstr_user_flags(pFlags), pBuffer, pCF);

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
 *           CLIPFORMAT_UserFree [OLE32.@]
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

static ULONG handle_UserSize(ULONG *pFlags, ULONG StartingSize, HANDLE *handle)
{
    if (LOWORD(*pFlags) == MSHCTX_DIFFERENTMACHINE)
    {
        ERR("can't remote a local handle\n");
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);
        return StartingSize;
    }
    return StartingSize + sizeof(RemotableHandle);
}

static unsigned char * handle_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, HANDLE *handle)
{
    RemotableHandle *remhandle = (RemotableHandle *)pBuffer;
    if (LOWORD(*pFlags) == MSHCTX_DIFFERENTMACHINE)
    {
        ERR("can't remote a local handle\n");
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);
        return pBuffer;
    }
    remhandle->fContext = WDT_INPROC_CALL;
    remhandle->u.hInproc = (LONG_PTR)*handle;
    return pBuffer + sizeof(RemotableHandle);
}

static unsigned char * handle_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, HANDLE *handle)
{
    RemotableHandle *remhandle = (RemotableHandle *)pBuffer;
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
        TRACE("(%s, %d, %p\n", debugstr_user_flags(pFlags), StartingSize, handle); \
        return handle_UserSize(pFlags, StartingSize, (HANDLE *)handle); \
    } \
    \
    unsigned char * __RPC_USER type##_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, type *handle) \
    { \
        TRACE("(%s, %p, &%p\n", debugstr_user_flags(pFlags), pBuffer, *handle); \
        return handle_UserMarshal(pFlags, pBuffer, (HANDLE *)handle); \
    } \
    \
    unsigned char * __RPC_USER type##_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, type *handle) \
    { \
        TRACE("(%s, %p, %p\n", debugstr_user_flags(pFlags), pBuffer, handle); \
        return handle_UserUnmarshal(pFlags, pBuffer, (HANDLE *)handle); \
    } \
    \
    void __RPC_USER type##_UserFree(ULONG *pFlags, type *handle) \
    { \
        TRACE("(%s, &%p\n", debugstr_user_flags(pFlags), *handle); \
        handle_UserFree(pFlags, (HANDLE *)handle); \
    }

IMPL_WIREM_HANDLE(HACCEL)
IMPL_WIREM_HANDLE(HMENU)
IMPL_WIREM_HANDLE(HWND)
IMPL_WIREM_HANDLE(HDC)
IMPL_WIREM_HANDLE(HICON)
IMPL_WIREM_HANDLE(HBRUSH)

/******************************************************************************
 *           HGLOBAL_UserSize [OLE32.@]
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

    TRACE("(%s, %d, %p\n", debugstr_user_flags(pFlags), StartingSize, phGlobal);

    ALIGN_LENGTH(size, 3);

    size += sizeof(ULONG);

    if (LOWORD(*pFlags == MSHCTX_INPROC))
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
 *           HGLOBAL_UserMarshal [OLE32.@]
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
    TRACE("(%s, %p, &%p\n", debugstr_user_flags(pFlags), pBuffer, *phGlobal);

    ALIGN_POINTER(pBuffer, 3);

    if (LOWORD(*pFlags == MSHCTX_INPROC))
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
 *           HGLOBAL_UserUnmarshal [OLE32.@]
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

    TRACE("(%s, %p, &%p\n", debugstr_user_flags(pFlags), pBuffer, *phGlobal);

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
 *           HGLOBAL_UserFree [OLE32.@]
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
    TRACE("(%s, &%p\n", debugstr_user_flags(pFlags), *phGlobal);

    if (LOWORD(*pFlags != MSHCTX_INPROC) && *phGlobal)
        GlobalFree(*phGlobal);
}

/******************************************************************************
 *           HBITMAP_UserSize [OLE32.@]
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
ULONG __RPC_USER HBITMAP_UserSize(ULONG *pFlags, ULONG StartingSize, HBITMAP *phBmp)
{
    FIXME(":stub\n");
    return StartingSize;
}

/******************************************************************************
*           HBITMAP_UserMarshal [OLE32.@]
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
unsigned char * __RPC_USER HBITMAP_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, HBITMAP *phBmp)
{
    FIXME(":stub\n");
    return pBuffer;
}

/******************************************************************************
 *           HBITMAP_UserUnmarshal [OLE32.@]
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
unsigned char * __RPC_USER HBITMAP_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, HBITMAP *phBmp)
{
    FIXME(":stub\n");
    return pBuffer;
}

/******************************************************************************
 *           HBITMAP_UserFree [OLE32.@]
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
void __RPC_USER HBITMAP_UserFree(ULONG *pFlags, HBITMAP *phBmp)
{
    FIXME(":stub\n");
}

/******************************************************************************
 *           HPALETTE_UserSize [OLE32.@]
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
 *           HPALETTE_UserMarshal [OLE32.@]
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
 *           HPALETTE_UserUnmarshal [OLE32.@]
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
 *           HPALETTE_UserFree [OLE32.@]
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
 *           HMETAFILE_UserSize [OLE32.@]
 *
 * Calculates the buffer size required to marshal a metafile.
 *
 * PARAMS
 *  pFlags       [I] Flags. See notes.
 *  StartingSize [I] Starting size of the buffer. This value is added on to
 *                   the buffer size required for the clip format.
 *  phmf         [I] Metafile to size.
 *
 * RETURNS
 *  The buffer size required to marshal a metafile plus the starting size.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is a ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
ULONG __RPC_USER HMETAFILE_UserSize(ULONG *pFlags, ULONG StartingSize, HMETAFILE *phmf)
{
    ULONG size = StartingSize;

    TRACE("(%s, %d, &%p\n", debugstr_user_flags(pFlags), StartingSize, *phmf);

    ALIGN_LENGTH(size, 3);

    size += sizeof(ULONG);
    if (LOWORD(*pFlags) == MSHCTX_INPROC)
        size += sizeof(ULONG_PTR);
    else
    {
        size += sizeof(ULONG);

        if (*phmf)
        {
            UINT mfsize;

            size += 2 * sizeof(ULONG);
            mfsize = GetMetaFileBitsEx(*phmf, 0, NULL);
            size += mfsize;
        }
    }

    return size;
}

/******************************************************************************
 *           HMETAFILE_UserMarshal [OLE32.@]
 *
 * Marshals a metafile into a buffer.
 *
 * PARAMS
 *  pFlags  [I] Flags. See notes.
 *  pBuffer [I] Buffer to marshal the clip format into.
 *  phEmf   [I] Metafile to marshal.
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
unsigned char * __RPC_USER HMETAFILE_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, HMETAFILE *phmf)
{
    TRACE("(%s, %p, &%p\n", debugstr_user_flags(pFlags), pBuffer, *phmf);

    ALIGN_POINTER(pBuffer, 3);

    if (LOWORD(*pFlags) == MSHCTX_INPROC)
    {
        if (sizeof(*phmf) == 8)
            *(ULONG *)pBuffer = WDT_INPROC64_CALL;
        else
            *(ULONG *)pBuffer = WDT_INPROC_CALL;
        pBuffer += sizeof(ULONG);
        *(HMETAFILE *)pBuffer = *phmf;
        pBuffer += sizeof(HMETAFILE);
    }
    else
    {
        *(ULONG *)pBuffer = WDT_REMOTE_CALL;
        pBuffer += sizeof(ULONG);
        *(ULONG *)pBuffer = (ULONG)(ULONG_PTR)*phmf;
        pBuffer += sizeof(ULONG);

        if (*phmf)
        {
            UINT mfsize = GetMetaFileBitsEx(*phmf, 0, NULL);

            *(ULONG *)pBuffer = mfsize;
            pBuffer += sizeof(ULONG);
            *(ULONG *)pBuffer = mfsize;
            pBuffer += sizeof(ULONG);
            GetMetaFileBitsEx(*phmf, mfsize, pBuffer);
            pBuffer += mfsize;
        }
    }

    return pBuffer;
}

/******************************************************************************
 *           HMETAFILE_UserUnmarshal [OLE32.@]
 *
 * Unmarshals a metafile from a buffer.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  pBuffer  [I] Buffer to marshal the clip format from.
 *  phmf     [O] Address that receive the unmarshaled metafile.
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
unsigned char * __RPC_USER HMETAFILE_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, HMETAFILE *phmf)
{
    ULONG fContext;

    TRACE("(%s, %p, %p\n", debugstr_user_flags(pFlags), pBuffer, phmf);

    ALIGN_POINTER(pBuffer, 3);

    fContext = *(ULONG *)pBuffer;
    pBuffer += sizeof(ULONG);

    if (((fContext == WDT_INPROC_CALL) && (sizeof(*phmf) < 8)) ||
        ((fContext == WDT_INPROC64_CALL) && (sizeof(*phmf) == 8)))
    {
        *phmf = *(HMETAFILE *)pBuffer;
        pBuffer += sizeof(*phmf);
    }
    else if (fContext == WDT_REMOTE_CALL)
    {
        ULONG handle;

        handle = *(ULONG *)pBuffer;
        pBuffer += sizeof(ULONG);

        if (handle)
        {
            ULONG size;
            size = *(ULONG *)pBuffer;
            pBuffer += sizeof(ULONG);
            if (size != *(ULONG *)pBuffer)
            {
                RaiseException(RPC_X_BAD_STUB_DATA, 0, 0, NULL);
                return pBuffer;
            }
            pBuffer += sizeof(ULONG);
            *phmf = SetMetaFileBitsEx(size, pBuffer);
            pBuffer += size;
        }
        else
            *phmf = NULL;
    }
    else
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);

    return pBuffer;
}

/******************************************************************************
 *           HMETAFILE_UserFree [OLE32.@]
 *
 * Frees an unmarshaled metafile.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  phmf     [I] Metafile to free.
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
void __RPC_USER HMETAFILE_UserFree(ULONG *pFlags, HMETAFILE *phmf)
{
    TRACE("(%s, &%p\n", debugstr_user_flags(pFlags), *phmf);

    if (LOWORD(*pFlags) != MSHCTX_INPROC)
        DeleteMetaFile(*phmf);
}

/******************************************************************************
*           HENHMETAFILE_UserSize [OLE32.@]
*
* Calculates the buffer size required to marshal an enhanced metafile.
*
* PARAMS
*  pFlags       [I] Flags. See notes.
*  StartingSize [I] Starting size of the buffer. This value is added on to
*                   the buffer size required for the clip format.
*  phEmf        [I] Enhanced metafile to size.
*
* RETURNS
*  The buffer size required to marshal an enhanced metafile plus the starting size.
*
* NOTES
*  Even though the function is documented to take a pointer to a ULONG in
*  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
*  the first parameter is a ULONG.
*  This function is only intended to be called by the RPC runtime.
*/
ULONG __RPC_USER HENHMETAFILE_UserSize(ULONG *pFlags, ULONG StartingSize, HENHMETAFILE *phEmf)
{
    ULONG size = StartingSize;

    TRACE("(%s, %d, %p\n", debugstr_user_flags(pFlags), StartingSize, *phEmf);

    size += sizeof(ULONG);
    if (LOWORD(*pFlags) == MSHCTX_INPROC)
        size += sizeof(ULONG_PTR);
    else
    {
        size += sizeof(ULONG);

        if (*phEmf)
        {
            UINT emfsize;
    
            size += 2 * sizeof(ULONG);
            emfsize = GetEnhMetaFileBits(*phEmf, 0, NULL);
            size += emfsize;
        }
    }

    return size;
}

/******************************************************************************
 *           HENHMETAFILE_UserMarshal [OLE32.@]
 *
 * Marshals an enhance metafile into a buffer.
 *
 * PARAMS
 *  pFlags  [I] Flags. See notes.
 *  pBuffer [I] Buffer to marshal the clip format into.
 *  phEmf   [I] Enhanced metafile to marshal.
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
unsigned char * __RPC_USER HENHMETAFILE_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, HENHMETAFILE *phEmf)
{
    TRACE("(%s, %p, &%p\n", debugstr_user_flags(pFlags), pBuffer, *phEmf);

    if (LOWORD(*pFlags) == MSHCTX_INPROC)
    {
        if (sizeof(*phEmf) == 8)
            *(ULONG *)pBuffer = WDT_INPROC64_CALL;
        else
            *(ULONG *)pBuffer = WDT_INPROC_CALL;
        pBuffer += sizeof(ULONG);
        *(HENHMETAFILE *)pBuffer = *phEmf;
        pBuffer += sizeof(HENHMETAFILE);
    }
    else
    {
        *(ULONG *)pBuffer = WDT_REMOTE_CALL;
        pBuffer += sizeof(ULONG);
        *(ULONG *)pBuffer = (ULONG)(ULONG_PTR)*phEmf;
        pBuffer += sizeof(ULONG);
    
        if (*phEmf)
        {
            UINT emfsize = GetEnhMetaFileBits(*phEmf, 0, NULL);
    
            *(ULONG *)pBuffer = emfsize;
            pBuffer += sizeof(ULONG);
            *(ULONG *)pBuffer = emfsize;
            pBuffer += sizeof(ULONG);
            GetEnhMetaFileBits(*phEmf, emfsize, pBuffer);
            pBuffer += emfsize;
        }
    }

    return pBuffer;
}

/******************************************************************************
 *           HENHMETAFILE_UserUnmarshal [OLE32.@]
 *
 * Unmarshals an enhanced metafile from a buffer.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  pBuffer  [I] Buffer to marshal the clip format from.
 *  phEmf    [O] Address that receive the unmarshaled enhanced metafile.
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
unsigned char * __RPC_USER HENHMETAFILE_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, HENHMETAFILE *phEmf)
{
    ULONG fContext;

    TRACE("(%s, %p, %p\n", debugstr_user_flags(pFlags), pBuffer, phEmf);

    fContext = *(ULONG *)pBuffer;
    pBuffer += sizeof(ULONG);

    if (((fContext == WDT_INPROC_CALL) && (sizeof(*phEmf) < 8)) ||
        ((fContext == WDT_INPROC64_CALL) && (sizeof(*phEmf) == 8)))
    {
        *phEmf = *(HENHMETAFILE *)pBuffer;
        pBuffer += sizeof(*phEmf);
    }
    else if (fContext == WDT_REMOTE_CALL)
    {
        ULONG handle;

        handle = *(ULONG *)pBuffer;
        pBuffer += sizeof(ULONG);

        if (handle)
        {
            ULONG size;
            size = *(ULONG *)pBuffer;
            pBuffer += sizeof(ULONG);
            if (size != *(ULONG *)pBuffer)
            {
                RaiseException(RPC_X_BAD_STUB_DATA, 0, 0, NULL);
                return pBuffer;
            }
            pBuffer += sizeof(ULONG);
            *phEmf = SetEnhMetaFileBits(size, pBuffer);
            pBuffer += size;
        }
        else 
            *phEmf = NULL;
    }
    else
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);

    return pBuffer;
}

/******************************************************************************
 *           HENHMETAFILE_UserFree [OLE32.@]
 *
 * Frees an unmarshaled enhanced metafile.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  phEmf    [I] Enhanced metafile to free.
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
void __RPC_USER HENHMETAFILE_UserFree(ULONG *pFlags, HENHMETAFILE *phEmf)
{
    TRACE("(%s, &%p\n", debugstr_user_flags(pFlags), *phEmf);

    if (LOWORD(*pFlags) != MSHCTX_INPROC)
        DeleteEnhMetaFile(*phEmf);
}

/******************************************************************************
 *           HMETAFILEPICT_UserSize [OLE32.@]
 *
 * Calculates the buffer size required to marshal an metafile pict.
 *
 * PARAMS
 *  pFlags       [I] Flags. See notes.
 *  StartingSize [I] Starting size of the buffer. This value is added on to
 *                   the buffer size required for the clip format.
 *  phMfp        [I] Metafile pict to size.
 *
 * RETURNS
 *  The buffer size required to marshal a metafile pict plus the starting size.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is a ULONG.
 *  This function is only intended to be called by the RPC runtime.
 */
ULONG __RPC_USER HMETAFILEPICT_UserSize(ULONG *pFlags, ULONG StartingSize, HMETAFILEPICT *phMfp)
{
    ULONG size = StartingSize;

    TRACE("(%s, %d, &%p)\n", debugstr_user_flags(pFlags), StartingSize, *phMfp);

    size += sizeof(ULONG);

    if(LOWORD(*pFlags) == MSHCTX_INPROC)
        size += sizeof(HMETAFILEPICT);
    else
    {
        size += sizeof(ULONG);

        if (*phMfp)
        {
            METAFILEPICT *mfpict = GlobalLock(*phMfp);

            /* FIXME: raise an exception if mfpict is NULL? */
            size += 3 * sizeof(ULONG);
            size += sizeof(ULONG);

            size = HMETAFILE_UserSize(pFlags, size, &mfpict->hMF);

            GlobalUnlock(*phMfp);
        }
    }

    return size;
}

/******************************************************************************
 *           HMETAFILEPICT_UserMarshal [OLE32.@]
 *
 * Marshals a metafile pict into a buffer.
 *
 * PARAMS
 *  pFlags  [I] Flags. See notes.
 *  pBuffer [I] Buffer to marshal the clip format into.
 *  phMfp   [I] Metafile pict to marshal.
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
unsigned char * __RPC_USER HMETAFILEPICT_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, HMETAFILEPICT *phMfp)
{
    TRACE("(%s, %p, &%p)\n", debugstr_user_flags(pFlags), pBuffer, *phMfp);

    if (LOWORD(*pFlags) == MSHCTX_INPROC)
    {
        if (sizeof(HMETAFILEPICT) == 8)
            *(ULONG *)pBuffer = WDT_INPROC64_CALL;
        else
            *(ULONG *)pBuffer = WDT_INPROC_CALL;
        pBuffer += sizeof(ULONG);
        *(HMETAFILEPICT *)pBuffer = *phMfp;
        pBuffer += sizeof(HMETAFILEPICT);
    }
    else
    {
        *(ULONG *)pBuffer = WDT_REMOTE_CALL;
        pBuffer += sizeof(ULONG);
        *(ULONG *)pBuffer = (ULONG)(ULONG_PTR)*phMfp;
        pBuffer += sizeof(ULONG);

        if (*phMfp)
        {
            METAFILEPICT *mfpict = GlobalLock(*phMfp);
            remoteMETAFILEPICT * remmfpict = (remoteMETAFILEPICT *)pBuffer;

            /* FIXME: raise an exception if mfpict is NULL? */
            remmfpict->mm = mfpict->mm;
            remmfpict->xExt = mfpict->xExt;
            remmfpict->yExt = mfpict->yExt;
            pBuffer += 3 * sizeof(ULONG);
            *(ULONG *)pBuffer = USER_MARSHAL_PTR_PREFIX;
            pBuffer += sizeof(ULONG);

            pBuffer = HMETAFILE_UserMarshal(pFlags, pBuffer, &mfpict->hMF);

            GlobalUnlock(*phMfp);
        }
    }
    return pBuffer;
}

/******************************************************************************
 *           HMETAFILEPICT_UserUnmarshal [OLE32.@]
 *
 * Unmarshals an metafile pict from a buffer.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  pBuffer  [I] Buffer to marshal the clip format from.
 *  phMfp    [O] Address that receive the unmarshaled metafile pict.
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
unsigned char * __RPC_USER HMETAFILEPICT_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, HMETAFILEPICT *phMfp)
{
    ULONG fContext;

    TRACE("(%s, %p, %p)\n", debugstr_user_flags(pFlags), pBuffer, phMfp);

    fContext = *(ULONG *)pBuffer;
    pBuffer += sizeof(ULONG);

    if ((fContext == WDT_INPROC_CALL) || fContext == WDT_INPROC64_CALL)
    {
        *phMfp = *(HMETAFILEPICT *)pBuffer;
        pBuffer += sizeof(HMETAFILEPICT);
    }
    else
    {
        ULONG handle = *(ULONG *)pBuffer;
        pBuffer += sizeof(ULONG);
        *phMfp = NULL;

        if(handle)
        {
            METAFILEPICT *mfpict;
            const remoteMETAFILEPICT *remmfpict;
            ULONG user_marshal_prefix;

            remmfpict = (const remoteMETAFILEPICT *)pBuffer;

            *phMfp = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAFILEPICT));
            if (!*phMfp)
                RpcRaiseException(E_OUTOFMEMORY);

            mfpict = GlobalLock(*phMfp);
            mfpict->mm = remmfpict->mm;
            mfpict->xExt = remmfpict->xExt;
            mfpict->yExt = remmfpict->yExt;
            pBuffer += 3 * sizeof(ULONG);
            user_marshal_prefix = *(ULONG *)pBuffer;
            pBuffer += sizeof(ULONG);

            if (user_marshal_prefix != USER_MARSHAL_PTR_PREFIX)
                RpcRaiseException(RPC_X_INVALID_TAG);

            pBuffer = HMETAFILE_UserUnmarshal(pFlags, pBuffer, &mfpict->hMF);

            GlobalUnlock(*phMfp);
        }
    }
    return pBuffer;
}

/******************************************************************************
 *           HMETAFILEPICT_UserFree [OLE32.@]
 *
 * Frees an unmarshaled metafile pict.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  phMfp    [I] Metafile pict to free.
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
void __RPC_USER HMETAFILEPICT_UserFree(ULONG *pFlags, HMETAFILEPICT *phMfp)
{
    TRACE("(%s, &%p)\n", debugstr_user_flags(pFlags), *phMfp);

    if ((LOWORD(*pFlags) != MSHCTX_INPROC) && *phMfp)
    {
        METAFILEPICT *mfpict;

        mfpict = GlobalLock(*phMfp);
        /* FIXME: raise an exception if mfpict is NULL? */
        HMETAFILE_UserFree(pFlags, &mfpict->hMF);
        GlobalUnlock(*phMfp);

        GlobalFree(*phMfp);
    }
}

/******************************************************************************
 *           WdtpInterfacePointer_UserSize [OLE32.@]
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

    TRACE("(%s, 0%x, %d, %p, %s)\n", debugstr_user_flags(pFlags), RealFlags, StartingSize, punk, debugstr_guid(riid));

    hr = CoGetMarshalSizeMax(&marshal_size, riid, punk, LOWORD(RealFlags), NULL, MSHLFLAGS_NORMAL);
    if(FAILED(hr)) return StartingSize;

    ALIGN_LENGTH(StartingSize, 3);
    StartingSize += 2 * sizeof(DWORD);
    return StartingSize + marshal_size;
}

/******************************************************************************
 *           WdtpInterfacePointer_UserMarshal [OLE32.@]
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

    TRACE("(%s, 0x%x, %p, &%p, %s)\n", debugstr_user_flags(pFlags), RealFlags, pBuffer, punk, debugstr_guid(riid));

    if(!h) return NULL;
    if(CreateStreamOnHGlobal(h, TRUE, &stm) != S_OK)
    {
        GlobalFree(h);
        return NULL;
    }

    if(CoMarshalInterface(stm, riid, punk, LOWORD(RealFlags), NULL, MSHLFLAGS_NORMAL) != S_OK)
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
 *           WdtpInterfacePointer_UserUnmarshal [OLE32.@]
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

    TRACE("(%s, %p, %p, %s)\n", debugstr_user_flags(pFlags), pBuffer, ppunk, debugstr_guid(riid));

    ALIGN_POINTER(pBuffer, 3);

    size = *(DWORD *)pBuffer;
    pBuffer += sizeof(DWORD);
    if(size != *(DWORD *)pBuffer)
        RaiseException(RPC_X_BAD_STUB_DATA, 0, 0, NULL);

    pBuffer += sizeof(DWORD);

    /* FIXME: sanity check on size */

    h = GlobalAlloc(GMEM_MOVEABLE, size);
    if(!h) RaiseException(RPC_X_NO_MEMORY, 0, 0, NULL);

    if(CreateStreamOnHGlobal(h, TRUE, &stm) != S_OK)
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

    if(hr != S_OK) RaiseException(hr, 0, 0, NULL);

    if(orig) IUnknown_Release(orig);

    return pBuffer + size;
}

/******************************************************************************
 *           WdtpInterfacePointer_UserFree [OLE32.@]
 *
 * Releases an unmarshaled interface pointer.
 *
 * PARAMS
 *  punk    [I] Interface pointer to release.
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI WdtpInterfacePointer_UserFree(IUnknown *punk)
{
    TRACE("(%p)\n", punk);
    if(punk) IUnknown_Release(punk);
}

/******************************************************************************
*           STGMEDIUM_UserSize [OLE32.@]
*
* Calculates the buffer size required to marshal an STGMEDIUM.
*
* PARAMS
*  pFlags       [I] Flags. See notes.
*  StartingSize [I] Starting size of the buffer. This value is added on to
*                   the buffer size required for the clip format.
*  pStgMedium   [I] STGMEDIUM to size.
*
* RETURNS
*  The buffer size required to marshal an STGMEDIUM plus the starting size.
*
* NOTES
*  Even though the function is documented to take a pointer to a ULONG in
*  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
*  the first parameter is a ULONG.
*  This function is only intended to be called by the RPC runtime.
*/
ULONG __RPC_USER STGMEDIUM_UserSize(ULONG *pFlags, ULONG StartingSize, STGMEDIUM *pStgMedium)
{
    ULONG size = StartingSize;

    TRACE("(%s, %d, %p\n", debugstr_user_flags(pFlags), StartingSize, pStgMedium);

    ALIGN_LENGTH(size, 3);

    size += 2 * sizeof(DWORD);
    if (pStgMedium->tymed != TYMED_NULL)
        size += sizeof(DWORD);

    switch (pStgMedium->tymed)
    {
    case TYMED_NULL:
        TRACE("TYMED_NULL\n");
        break;
    case TYMED_HGLOBAL:
        TRACE("TYMED_HGLOBAL\n");
        if (pStgMedium->u.hGlobal)
            size = HGLOBAL_UserSize(pFlags, size, &pStgMedium->u.hGlobal);
        break;
    case TYMED_FILE:
        TRACE("TYMED_FILE\n");
        if (pStgMedium->u.lpszFileName)
        {
            TRACE("file name is %s\n", debugstr_w(pStgMedium->u.lpszFileName));
            size += 3 * sizeof(DWORD) +
                (strlenW(pStgMedium->u.lpszFileName) + 1) * sizeof(WCHAR);
        }
        break;
    case TYMED_ISTREAM:
        TRACE("TYMED_ISTREAM\n");
        if (pStgMedium->u.pstm)
        {
            IUnknown *unk;
            IStream_QueryInterface(pStgMedium->u.pstm, &IID_IUnknown, (void**)&unk);
            size = WdtpInterfacePointer_UserSize(pFlags, LOWORD(*pFlags), size, unk, &IID_IStream);
            IUnknown_Release(unk);
        }
        break;
    case TYMED_ISTORAGE:
        TRACE("TYMED_ISTORAGE\n");
        if (pStgMedium->u.pstg)
        {
            IUnknown *unk;
            IStorage_QueryInterface(pStgMedium->u.pstg, &IID_IUnknown, (void**)&unk);
            size = WdtpInterfacePointer_UserSize(pFlags, LOWORD(*pFlags), size, unk, &IID_IStorage);
            IUnknown_Release(unk);
        }
        break;
    case TYMED_GDI:
        TRACE("TYMED_GDI\n");
        if (pStgMedium->u.hBitmap)
        {
            FIXME("not implemented for GDI object %p\n", pStgMedium->u.hBitmap);
        }
        break;
    case TYMED_MFPICT:
        TRACE("TYMED_MFPICT\n");
        if (pStgMedium->u.hMetaFilePict)
            size = HMETAFILEPICT_UserSize(pFlags, size, &pStgMedium->u.hMetaFilePict);
        break;
    case TYMED_ENHMF:
        TRACE("TYMED_ENHMF\n");
        if (pStgMedium->u.hEnhMetaFile)
            size = HENHMETAFILE_UserSize(pFlags, size, &pStgMedium->u.hEnhMetaFile);
        break;
    default:
        RaiseException(DV_E_TYMED, 0, 0, NULL);
    }

    if (pStgMedium->pUnkForRelease)
        size = WdtpInterfacePointer_UserSize(pFlags, LOWORD(*pFlags), size, pStgMedium->pUnkForRelease, &IID_IUnknown);

    return size;
}

/******************************************************************************
 *           STGMEDIUM_UserMarshal [OLE32.@]
 *
 * Marshals a STGMEDIUM into a buffer.
 *
 * PARAMS
 *  pFlags  [I] Flags. See notes.
 *  pBuffer [I] Buffer to marshal the clip format into.
 *  pCF     [I] STGMEDIUM to marshal.
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
unsigned char * __RPC_USER STGMEDIUM_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, STGMEDIUM *pStgMedium)
{
    TRACE("(%s, %p, %p\n", debugstr_user_flags(pFlags), pBuffer, pStgMedium);

    ALIGN_POINTER(pBuffer, 3);

    *(DWORD *)pBuffer = pStgMedium->tymed;
    pBuffer += sizeof(DWORD);
    if (pStgMedium->tymed != TYMED_NULL)
    {
        *(DWORD *)pBuffer = (DWORD)(DWORD_PTR)pStgMedium->u.pstg;
        pBuffer += sizeof(DWORD);
    }
    *(DWORD *)pBuffer = (DWORD)(DWORD_PTR)pStgMedium->pUnkForRelease;
    pBuffer += sizeof(DWORD);

    switch (pStgMedium->tymed)
    {
    case TYMED_NULL:
        TRACE("TYMED_NULL\n");
        break;
    case TYMED_HGLOBAL:
        TRACE("TYMED_HGLOBAL\n");
        if (pStgMedium->u.hGlobal)
            pBuffer = HGLOBAL_UserMarshal(pFlags, pBuffer, &pStgMedium->u.hGlobal);
        break;
    case TYMED_FILE:
        TRACE("TYMED_FILE\n");
        if (pStgMedium->u.lpszFileName)
        {
            DWORD len;
            len = strlenW(pStgMedium->u.lpszFileName);
            /* conformance */
            *(DWORD *)pBuffer = len + 1;
            pBuffer += sizeof(DWORD);
            /* offset */
            *(DWORD *)pBuffer = 0;
            pBuffer += sizeof(DWORD);
            /* variance */
            *(DWORD *)pBuffer = len + 1;
            pBuffer += sizeof(DWORD);

            TRACE("file name is %s\n", debugstr_w(pStgMedium->u.lpszFileName));
            memcpy(pBuffer, pStgMedium->u.lpszFileName, (len + 1) * sizeof(WCHAR));
        }
        break;
    case TYMED_ISTREAM:
        TRACE("TYMED_ISTREAM\n");
        if (pStgMedium->u.pstm)
        {
            IUnknown *unk;
            IStream_QueryInterface(pStgMedium->u.pstm, &IID_IUnknown, (void**)&unk);
            pBuffer = WdtpInterfacePointer_UserMarshal(pFlags, LOWORD(*pFlags), pBuffer, unk, &IID_IStream);
            IUnknown_Release(unk);
        }
        break;
    case TYMED_ISTORAGE:
        TRACE("TYMED_ISTORAGE\n");
        if (pStgMedium->u.pstg)
        {
            IUnknown *unk;
            IStorage_QueryInterface(pStgMedium->u.pstg, &IID_IUnknown, (void**)&unk);
            pBuffer = WdtpInterfacePointer_UserMarshal(pFlags, LOWORD(*pFlags), pBuffer, unk, &IID_IStorage);
            IUnknown_Release(unk);
        }
        break;
    case TYMED_GDI:
        TRACE("TYMED_GDI\n");
        if (pStgMedium->u.hBitmap)
        {
            FIXME("not implemented for GDI object %p\n", pStgMedium->u.hBitmap);
        }
        break;
    case TYMED_MFPICT:
        TRACE("TYMED_MFPICT\n");
        if (pStgMedium->u.hMetaFilePict)
            pBuffer = HMETAFILEPICT_UserMarshal(pFlags, pBuffer, &pStgMedium->u.hMetaFilePict);
        break;
    case TYMED_ENHMF:
        TRACE("TYMED_ENHMF\n");
        if (pStgMedium->u.hEnhMetaFile)
            pBuffer = HENHMETAFILE_UserMarshal(pFlags, pBuffer, &pStgMedium->u.hEnhMetaFile);
        break;
    default:
        RaiseException(DV_E_TYMED, 0, 0, NULL);
    }

    if (pStgMedium->pUnkForRelease)
        pBuffer = WdtpInterfacePointer_UserMarshal(pFlags, LOWORD(*pFlags), pBuffer, pStgMedium->pUnkForRelease, &IID_IUnknown);

    return pBuffer;
}

/******************************************************************************
 *           STGMEDIUM_UserUnmarshal [OLE32.@]
 *
 * Unmarshals a STGMEDIUM from a buffer.
 *
 * PARAMS
 *  pFlags     [I] Flags. See notes.
 *  pBuffer    [I] Buffer to marshal the clip format from.
 *  pStgMedium [O] Address that receive the unmarshaled STGMEDIUM.
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
unsigned char * __RPC_USER STGMEDIUM_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, STGMEDIUM *pStgMedium)
{
    DWORD content = 0;
    DWORD releaseunk;

    ALIGN_POINTER(pBuffer, 3);

    TRACE("(%s, %p, %p\n", debugstr_user_flags(pFlags), pBuffer, pStgMedium);

    pStgMedium->tymed = *(DWORD *)pBuffer;
    pBuffer += sizeof(DWORD);
    if (pStgMedium->tymed != TYMED_NULL)
    {
        content = *(DWORD *)pBuffer;
        pBuffer += sizeof(DWORD);
    }
    releaseunk = *(DWORD *)pBuffer;
    pBuffer += sizeof(DWORD);

    switch (pStgMedium->tymed)
    {
    case TYMED_NULL:
        TRACE("TYMED_NULL\n");
        break;
    case TYMED_HGLOBAL:
        TRACE("TYMED_HGLOBAL\n");
        if (content)
            pBuffer = HGLOBAL_UserUnmarshal(pFlags, pBuffer, &pStgMedium->u.hGlobal);
        break;
    case TYMED_FILE:
        TRACE("TYMED_FILE\n");
        if (content)
        {
            DWORD conformance;
            DWORD variance;
            conformance = *(DWORD *)pBuffer;
            pBuffer += sizeof(DWORD);
            if (*(DWORD *)pBuffer != 0)
            {
                ERR("invalid offset %d\n", *(DWORD *)pBuffer);
                RpcRaiseException(RPC_S_INVALID_BOUND);
                return NULL;
            }
            pBuffer += sizeof(DWORD);
            variance = *(DWORD *)pBuffer;
            pBuffer += sizeof(DWORD);
            if (conformance != variance)
            {
                ERR("conformance (%d) and variance (%d) should be equal\n",
                    conformance, variance);
                RpcRaiseException(RPC_S_INVALID_BOUND);
                return NULL;
            }
            if (conformance > 0x7fffffff)
            {
                ERR("conformance 0x%x too large\n", conformance);
                RpcRaiseException(RPC_S_INVALID_BOUND);
                return NULL;
            }
            pStgMedium->u.lpszFileName = CoTaskMemAlloc(conformance * sizeof(WCHAR));
            if (!pStgMedium->u.lpszFileName) RpcRaiseException(ERROR_OUTOFMEMORY);
            TRACE("unmarshalled file name is %s\n", debugstr_wn((const WCHAR *)pBuffer, variance));
            memcpy(pStgMedium->u.lpszFileName, pBuffer, variance * sizeof(WCHAR));
            pBuffer += variance * sizeof(WCHAR);
        }
        else
            pStgMedium->u.lpszFileName = NULL;
        break;
    case TYMED_ISTREAM:
        TRACE("TYMED_ISTREAM\n");
        if (content)
        {
            pBuffer = WdtpInterfacePointer_UserUnmarshal(pFlags, pBuffer, (IUnknown**)&pStgMedium->u.pstm, &IID_IStream);
        }
        else
        {
            if (pStgMedium->u.pstm) IStream_Release( pStgMedium->u.pstm );
            pStgMedium->u.pstm = NULL;
        }
        break;
    case TYMED_ISTORAGE:
        TRACE("TYMED_ISTORAGE\n");
        if (content)
        {
            pBuffer = WdtpInterfacePointer_UserUnmarshal(pFlags, pBuffer, (IUnknown**)&pStgMedium->u.pstg, &IID_IStorage);
        }
        else
        {
            if (pStgMedium->u.pstg) IStorage_Release( pStgMedium->u.pstg );
            pStgMedium->u.pstg = NULL;
        }
        break;
    case TYMED_GDI:
        TRACE("TYMED_GDI\n");
        if (content)
        {
            FIXME("not implemented for GDI object\n");
        }
        else
            pStgMedium->u.hBitmap = NULL;
        break;
    case TYMED_MFPICT:
        TRACE("TYMED_MFPICT\n");
        if (content)
            pBuffer = HMETAFILEPICT_UserUnmarshal(pFlags, pBuffer, &pStgMedium->u.hMetaFilePict);
        else
            pStgMedium->u.hMetaFilePict = NULL;
        break;
    case TYMED_ENHMF:
        TRACE("TYMED_ENHMF\n");
        if (content)
            pBuffer = HENHMETAFILE_UserUnmarshal(pFlags, pBuffer, &pStgMedium->u.hEnhMetaFile);
        else
            pStgMedium->u.hEnhMetaFile = NULL;
        break;
    default:
        RaiseException(DV_E_TYMED, 0, 0, NULL);
    }

    if (releaseunk)
        pBuffer = WdtpInterfacePointer_UserUnmarshal(pFlags, pBuffer, &pStgMedium->pUnkForRelease, &IID_IUnknown);
    /* Unlike the IStream / IStorage ifaces, the existing pUnkForRelease
       is left intact if a NULL ptr is unmarshalled - see the tests. */

    return pBuffer;
}

/******************************************************************************
 *           STGMEDIUM_UserFree [OLE32.@]
 *
 * Frees an unmarshaled STGMEDIUM.
 *
 * PARAMS
 *  pFlags     [I] Flags. See notes.
 *  pStgmedium [I] STGMEDIUM to free.
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
void __RPC_USER STGMEDIUM_UserFree(ULONG *pFlags, STGMEDIUM *pStgMedium)
{
    TRACE("(%s, %p\n", debugstr_user_flags(pFlags), pStgMedium);

    ReleaseStgMedium(pStgMedium);
}

ULONG __RPC_USER ASYNC_STGMEDIUM_UserSize(ULONG *pFlags, ULONG StartingSize, ASYNC_STGMEDIUM *pStgMedium)
{
    TRACE("\n");
    return STGMEDIUM_UserSize(pFlags, StartingSize, pStgMedium);
}

unsigned char * __RPC_USER ASYNC_STGMEDIUM_UserMarshal(  ULONG *pFlags, unsigned char *pBuffer, ASYNC_STGMEDIUM *pStgMedium)
{
    TRACE("\n");
    return STGMEDIUM_UserMarshal(pFlags, pBuffer, pStgMedium);
}

unsigned char * __RPC_USER ASYNC_STGMEDIUM_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, ASYNC_STGMEDIUM *pStgMedium)
{
    TRACE("\n");
    return STGMEDIUM_UserUnmarshal(pFlags, pBuffer, pStgMedium);
}

void __RPC_USER ASYNC_STGMEDIUM_UserFree(ULONG *pFlags, ASYNC_STGMEDIUM *pStgMedium)
{
    TRACE("\n");
    STGMEDIUM_UserFree(pFlags, pStgMedium);
}

ULONG __RPC_USER FLAG_STGMEDIUM_UserSize(ULONG *pFlags, ULONG StartingSize, FLAG_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
    return StartingSize;
}

unsigned char * __RPC_USER FLAG_STGMEDIUM_UserMarshal(  ULONG *pFlags, unsigned char *pBuffer, FLAG_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
    return pBuffer;
}

unsigned char * __RPC_USER FLAG_STGMEDIUM_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, FLAG_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
    return pBuffer;
}

void __RPC_USER FLAG_STGMEDIUM_UserFree(ULONG *pFlags, FLAG_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
}

ULONG __RPC_USER SNB_UserSize(ULONG *pFlags, ULONG StartingSize, SNB *pSnb)
{
    ULONG size = StartingSize;

    TRACE("(%s, %d, %p\n", debugstr_user_flags(pFlags), StartingSize, pSnb);

    ALIGN_LENGTH(size, 3);

    /* two counters from RemSNB header, plus one more ULONG */
    size += 3*sizeof(ULONG);

    /* now actual data length */
    if (*pSnb)
    {
        WCHAR **ptrW = *pSnb;

        while (*ptrW)
        {
            size += (strlenW(*ptrW) + 1)*sizeof(WCHAR);
            ptrW++;
        }
    }

    return size;
}

struct SNB_wire {
    ULONG charcnt;
    ULONG strcnt;
    ULONG datalen;
    WCHAR data[1];
};

unsigned char * __RPC_USER SNB_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, SNB *pSnb)
{
    struct SNB_wire *wire;
    ULONG size;

    TRACE("(%s, %p, %p)\n", debugstr_user_flags(pFlags), pBuffer, pSnb);

    ALIGN_POINTER(pBuffer, 3);

    wire = (struct SNB_wire*)pBuffer;
    wire->charcnt = wire->strcnt = 0;
    size = 3*sizeof(ULONG);

    if (*pSnb)
    {
        WCHAR **ptrW = *pSnb;
        WCHAR *dataW = wire->data;

        while (*ptrW)
        {
            ULONG len = strlenW(*ptrW) + 1;

            wire->strcnt++;
            wire->charcnt += len;
            memcpy(dataW, *ptrW, len*sizeof(WCHAR));
            dataW += len;

            size += len*sizeof(WCHAR);
            ptrW++;
        }
    }

    wire->datalen = wire->charcnt;
    return pBuffer + size;
}

unsigned char * __RPC_USER SNB_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, SNB *pSnb)
{
    USER_MARSHAL_CB *umcb = (USER_MARSHAL_CB*)pFlags;
    struct SNB_wire *wire;

    TRACE("(%s, %p, %p)\n", debugstr_user_flags(pFlags), pBuffer, pSnb);

    wire = (struct SNB_wire*)pBuffer;

    if (*pSnb)
        umcb->pStubMsg->pfnFree(*pSnb);

    if (wire->datalen == 0)
        *pSnb = NULL;
    else
    {
        WCHAR *src = wire->data, *dest;
        WCHAR **ptrW;
        ULONG i;

        ptrW = *pSnb = umcb->pStubMsg->pfnAllocate((wire->strcnt+1)*sizeof(WCHAR*) + wire->datalen*sizeof(WCHAR));
        dest = (WCHAR*)(*pSnb + wire->strcnt + 1);

        for (i = 0; i < wire->strcnt; i++)
        {
            ULONG len = strlenW(src);
            memcpy(dest, src, (len + 1)*sizeof(WCHAR));
            *ptrW = dest;
            src += len + 1;
            dest += len + 1;
            ptrW++;
        }
        *ptrW = NULL;
    }

    return pBuffer + 3*sizeof(ULONG) + wire->datalen*sizeof(WCHAR);
}

void __RPC_USER SNB_UserFree(ULONG *pFlags, SNB *pSnb)
{
    USER_MARSHAL_CB *umcb = (USER_MARSHAL_CB*)pFlags;
    TRACE("(%p)\n", pSnb);
    if (*pSnb)
        umcb->pStubMsg->pfnFree(*pSnb);
}

/* call_as/local stubs for unknwn.idl */

HRESULT CALLBACK IClassFactory_CreateInstance_Proxy(
    IClassFactory* This,
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObject)
{
    TRACE("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppvObject);
    *ppvObject = NULL;
    if (pUnkOuter)
    {
        ERR("aggregation is not allowed on remote objects\n");
        return CLASS_E_NOAGGREGATION;
    }
    return IClassFactory_RemoteCreateInstance_Proxy(This, riid,
                                                    (IUnknown **) ppvObject);
}

HRESULT __RPC_STUB IClassFactory_CreateInstance_Stub(
    IClassFactory* This,
    REFIID riid,
    IUnknown **ppvObject)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppvObject);
    return IClassFactory_CreateInstance(This, NULL, riid, (void **) ppvObject);
}

HRESULT CALLBACK IClassFactory_LockServer_Proxy(
    IClassFactory* This,
    BOOL fLock)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IClassFactory_LockServer_Stub(
    IClassFactory* This,
    BOOL fLock)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

/* call_as/local stubs for objidl.idl */

HRESULT CALLBACK IEnumUnknown_Next_Proxy(
    IEnumUnknown* This,
    ULONG celt,
    IUnknown **rgelt,
    ULONG *pceltFetched)
{
    ULONG fetched;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    if (!pceltFetched) pceltFetched = &fetched;
    return IEnumUnknown_RemoteNext_Proxy(This, celt, rgelt, pceltFetched);
}

HRESULT __RPC_STUB IEnumUnknown_Next_Stub(
    IEnumUnknown* This,
    ULONG celt,
    IUnknown **rgelt,
    ULONG *pceltFetched)
{
    HRESULT hr;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    *pceltFetched = 0;
    hr = IEnumUnknown_Next(This, celt, rgelt, pceltFetched);
    if (hr == S_OK) *pceltFetched = celt;
    return hr;
}

HRESULT CALLBACK IBindCtx_SetBindOptions_Proxy(
    IBindCtx* This,
    BIND_OPTS *pbindopts)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBindCtx_SetBindOptions_Stub(
    IBindCtx* This,
    BIND_OPTS2 *pbindopts)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IBindCtx_GetBindOptions_Proxy(
    IBindCtx* This,
    BIND_OPTS *pbindopts)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBindCtx_GetBindOptions_Stub(
    IBindCtx* This,
    BIND_OPTS2 *pbindopts)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IEnumMoniker_Next_Proxy(
    IEnumMoniker* This,
    ULONG celt,
    IMoniker **rgelt,
    ULONG *pceltFetched)
{
    ULONG fetched;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    if (!pceltFetched) pceltFetched = &fetched;
    return IEnumMoniker_RemoteNext_Proxy(This, celt, rgelt, pceltFetched);
}

HRESULT __RPC_STUB IEnumMoniker_Next_Stub(
    IEnumMoniker* This,
    ULONG celt,
    IMoniker **rgelt,
    ULONG *pceltFetched)
{
    HRESULT hr;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    *pceltFetched = 0;
    hr = IEnumMoniker_Next(This, celt, rgelt, pceltFetched);
    if (hr == S_OK) *pceltFetched = celt;
    return hr;
}

BOOL CALLBACK IRunnableObject_IsRunning_Proxy(
    IRunnableObject* This)
{
    BOOL rv;
    FIXME(":stub\n");
    memset(&rv, 0, sizeof rv);
    return rv;
}

HRESULT __RPC_STUB IRunnableObject_IsRunning_Stub(
    IRunnableObject* This)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IMoniker_BindToObject_Proxy(
    IMoniker* This,
    IBindCtx *pbc,
    IMoniker *pmkToLeft,
    REFIID riidResult,
    void **ppvResult)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IMoniker_BindToObject_Stub(
    IMoniker* This,
    IBindCtx *pbc,
    IMoniker *pmkToLeft,
    REFIID riidResult,
    IUnknown **ppvResult)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IMoniker_BindToStorage_Proxy(
    IMoniker* This,
    IBindCtx *pbc,
    IMoniker *pmkToLeft,
    REFIID riid,
    void **ppvObj)
{
    TRACE("(%p)->(%p %p %s %p)\n", This, pbc, pmkToLeft, debugstr_guid(riid), ppvObj);
    return IMoniker_RemoteBindToStorage_Proxy(This, pbc, pmkToLeft, riid, (IUnknown**)ppvObj);
}

HRESULT __RPC_STUB IMoniker_BindToStorage_Stub(
    IMoniker* This,
    IBindCtx *pbc,
    IMoniker *pmkToLeft,
    REFIID riid,
    IUnknown **ppvObj)
{
    TRACE("(%p)->(%p %p %s %p)\n", This, pbc, pmkToLeft, debugstr_guid(riid), ppvObj);
    return IMoniker_BindToStorage(This, pbc, pmkToLeft, riid, (void**)ppvObj);
}

HRESULT CALLBACK IEnumString_Next_Proxy(
    IEnumString* This,
    ULONG celt,
    LPOLESTR *rgelt,
    ULONG *pceltFetched)
{
    ULONG fetched;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    if (!pceltFetched) pceltFetched = &fetched;
    return IEnumString_RemoteNext_Proxy(This, celt, rgelt, pceltFetched);
}

HRESULT __RPC_STUB IEnumString_Next_Stub(
    IEnumString* This,
    ULONG celt,
    LPOLESTR *rgelt,
    ULONG *pceltFetched)
{
    HRESULT hr;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    *pceltFetched = 0;
    hr = IEnumString_Next(This, celt, rgelt, pceltFetched);
    if (hr == S_OK) *pceltFetched = celt;
    return hr;
}

HRESULT CALLBACK ISequentialStream_Read_Proxy(
    ISequentialStream* This,
    void *pv,
    ULONG cb,
    ULONG *pcbRead)
{
    ULONG read;
    HRESULT hr;

    TRACE("(%p)->(%p, %d, %p)\n", This, pv, cb, pcbRead);

    hr = ISequentialStream_RemoteRead_Proxy(This, pv, cb, &read);
    if(pcbRead) *pcbRead = read;

    return hr;
}

HRESULT __RPC_STUB ISequentialStream_Read_Stub(
    ISequentialStream* This,
    byte *pv,
    ULONG cb,
    ULONG *pcbRead)
{
    TRACE("(%p)->(%p, %d, %p)\n", This, pv, cb, pcbRead);
    return ISequentialStream_Read(This, pv, cb, pcbRead);
}

HRESULT CALLBACK ISequentialStream_Write_Proxy(
    ISequentialStream* This,
    const void *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
    ULONG written;
    HRESULT hr;

    TRACE("(%p)->(%p, %d, %p)\n", This, pv, cb, pcbWritten);

    hr = ISequentialStream_RemoteWrite_Proxy(This, pv, cb, &written);
    if(pcbWritten) *pcbWritten = written;

    return hr;
}

HRESULT __RPC_STUB ISequentialStream_Write_Stub(
    ISequentialStream* This,
    const byte *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
    TRACE("(%p)->(%p, %d, %p)\n", This, pv, cb, pcbWritten);
    return ISequentialStream_Write(This, pv, cb, pcbWritten);
}

HRESULT CALLBACK IStream_Seek_Proxy(
    IStream* This,
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER *plibNewPosition)
{
    ULARGE_INTEGER newpos;
    HRESULT hr;

    TRACE("(%p)->(%s, %d, %p)\n", This, wine_dbgstr_longlong(dlibMove.QuadPart), dwOrigin, plibNewPosition);

    hr = IStream_RemoteSeek_Proxy(This, dlibMove, dwOrigin, &newpos);
    if(plibNewPosition) *plibNewPosition = newpos;

    return hr;
}

HRESULT __RPC_STUB IStream_Seek_Stub(
    IStream* This,
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER *plibNewPosition)
{
    TRACE("(%p)->(%s, %d, %p)\n", This, wine_dbgstr_longlong(dlibMove.QuadPart), dwOrigin, plibNewPosition);
    return IStream_Seek(This, dlibMove, dwOrigin, plibNewPosition);
}

HRESULT CALLBACK IStream_CopyTo_Proxy(
    IStream* This,
    IStream *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten)
{
    ULARGE_INTEGER read, written;
    HRESULT hr;

    TRACE("(%p)->(%p, %s, %p, %p)\n", This, pstm, wine_dbgstr_longlong(cb.QuadPart), pcbRead, pcbWritten);

    hr = IStream_RemoteCopyTo_Proxy(This, pstm, cb, &read, &written);
    if(pcbRead) *pcbRead = read;
    if(pcbWritten) *pcbWritten = written;

    return hr;
}

HRESULT __RPC_STUB IStream_CopyTo_Stub(
    IStream* This,
    IStream *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten)
{
    TRACE("(%p)->(%p, %s, %p, %p)\n", This, pstm, wine_dbgstr_longlong(cb.QuadPart), pcbRead, pcbWritten);

    return IStream_CopyTo(This, pstm, cb, pcbRead, pcbWritten);
}

HRESULT CALLBACK IEnumSTATSTG_Next_Proxy(
    IEnumSTATSTG* This,
    ULONG celt,
    STATSTG *rgelt,
    ULONG *pceltFetched)
{
    ULONG fetched;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    if (!pceltFetched) pceltFetched = &fetched;
    return IEnumSTATSTG_RemoteNext_Proxy(This, celt, rgelt, pceltFetched);
}

HRESULT __RPC_STUB IEnumSTATSTG_Next_Stub(
    IEnumSTATSTG* This,
    ULONG celt,
    STATSTG *rgelt,
    ULONG *pceltFetched)
{
    HRESULT hr;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    *pceltFetched = 0;
    hr = IEnumSTATSTG_Next(This, celt, rgelt, pceltFetched);
    if (hr == S_OK) *pceltFetched = celt;
    return hr;
}

HRESULT CALLBACK IStorage_OpenStream_Proxy(
    IStorage* This,
    LPCOLESTR pwcsName,
    void *reserved1,
    DWORD grfMode,
    DWORD reserved2,
    IStream **ppstm)
{
    TRACE("(%p)->(%s, %p, %08x, %d %p)\n", This, debugstr_w(pwcsName), reserved1, grfMode, reserved2, ppstm);
    if(reserved1) WARN("reserved1 %p\n", reserved1);

    return IStorage_RemoteOpenStream_Proxy(This, pwcsName, 0, NULL, grfMode, reserved2, ppstm);
}

HRESULT __RPC_STUB IStorage_OpenStream_Stub(
    IStorage* This,
    LPCOLESTR pwcsName,
    ULONG cbReserved1,
    byte *reserved1,
    DWORD grfMode,
    DWORD reserved2,
    IStream **ppstm)
{
    TRACE("(%p)->(%s, %d, %p, %08x, %d %p)\n", This, debugstr_w(pwcsName), cbReserved1, reserved1, grfMode, reserved2, ppstm);
    if(cbReserved1 || reserved1) WARN("cbReserved1 %d reserved1 %p\n", cbReserved1, reserved1);

    return IStorage_OpenStream(This, pwcsName, NULL, grfMode, reserved2, ppstm);
}

HRESULT CALLBACK IStorage_EnumElements_Proxy(
    IStorage* This,
    DWORD reserved1,
    void *reserved2,
    DWORD reserved3,
    IEnumSTATSTG **ppenum)
{
    TRACE("(%p)->(%d, %p, %d, %p)\n", This, reserved1, reserved2, reserved3, ppenum);
    if(reserved2) WARN("reserved2 %p\n", reserved2);

    return IStorage_RemoteEnumElements_Proxy(This, reserved1, 0, NULL, reserved3, ppenum);
}

HRESULT __RPC_STUB IStorage_EnumElements_Stub(
    IStorage* This,
    DWORD reserved1,
    ULONG cbReserved2,
    byte *reserved2,
    DWORD reserved3,
    IEnumSTATSTG **ppenum)
{
    TRACE("(%p)->(%d, %d, %p, %d, %p)\n", This, reserved1, cbReserved2, reserved2, reserved3, ppenum);
    if(cbReserved2 || reserved2) WARN("cbReserved2 %d reserved2 %p\n", cbReserved2, reserved2);

    return IStorage_EnumElements(This, reserved1, NULL, reserved3, ppenum);
}

HRESULT CALLBACK ILockBytes_ReadAt_Proxy(
    ILockBytes* This,
    ULARGE_INTEGER ulOffset,
    void *pv,
    ULONG cb,
    ULONG *pcbRead)
{
    ULONG read;
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %d, %p)\n", This, wine_dbgstr_longlong(ulOffset.QuadPart), pv, cb, pcbRead);

    hr = ILockBytes_RemoteReadAt_Proxy(This, ulOffset, pv, cb, &read);
    if(pcbRead) *pcbRead = read;

    return hr;
}

HRESULT __RPC_STUB ILockBytes_ReadAt_Stub(
    ILockBytes* This,
    ULARGE_INTEGER ulOffset,
    byte *pv,
    ULONG cb,
    ULONG *pcbRead)
{
    TRACE("(%p)->(%s, %p, %d, %p)\n", This, wine_dbgstr_longlong(ulOffset.QuadPart), pv, cb, pcbRead);
    return ILockBytes_ReadAt(This, ulOffset, pv, cb, pcbRead);
}

HRESULT CALLBACK ILockBytes_WriteAt_Proxy(
    ILockBytes* This,
    ULARGE_INTEGER ulOffset,
    const void *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
    ULONG written;
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %d, %p)\n", This, wine_dbgstr_longlong(ulOffset.QuadPart), pv, cb, pcbWritten);

    hr = ILockBytes_RemoteWriteAt_Proxy(This, ulOffset, pv, cb, &written);
    if(pcbWritten) *pcbWritten = written;

    return hr;
}

HRESULT __RPC_STUB ILockBytes_WriteAt_Stub(
    ILockBytes* This,
    ULARGE_INTEGER ulOffset,
    const byte *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
    TRACE("(%p)->(%s, %p, %d, %p)\n", This, wine_dbgstr_longlong(ulOffset.QuadPart), pv, cb, pcbWritten);
    return ILockBytes_WriteAt(This, ulOffset, pv, cb, pcbWritten);
}

HRESULT CALLBACK IFillLockBytes_FillAppend_Proxy(
    IFillLockBytes* This,
    const void *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
    ULONG written;
    HRESULT hr;

    TRACE("(%p)->(%p, %d, %p)\n", This, pv, cb, pcbWritten);

    hr = IFillLockBytes_RemoteFillAppend_Proxy(This, pv, cb, &written);
    if(pcbWritten) *pcbWritten = written;

    return hr;
}

HRESULT __RPC_STUB IFillLockBytes_FillAppend_Stub(
    IFillLockBytes* This,
    const byte *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
    TRACE("(%p)->(%p, %d, %p)\n", This, pv, cb, pcbWritten);
    return IFillLockBytes_FillAppend(This, pv, cb, pcbWritten);
}

HRESULT CALLBACK IFillLockBytes_FillAt_Proxy(
    IFillLockBytes* This,
    ULARGE_INTEGER ulOffset,
    const void *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
    ULONG written;
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %d, %p)\n", This, wine_dbgstr_longlong(ulOffset.QuadPart), pv, cb, pcbWritten);

    hr = IFillLockBytes_RemoteFillAt_Proxy(This, ulOffset, pv, cb, &written);
    if(pcbWritten) *pcbWritten = written;

    return hr;
}

HRESULT __RPC_STUB IFillLockBytes_FillAt_Stub(
    IFillLockBytes* This,
    ULARGE_INTEGER ulOffset,
    const byte *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
    TRACE("(%p)->(%s, %p, %d, %p)\n", This, wine_dbgstr_longlong(ulOffset.QuadPart), pv, cb, pcbWritten);
    return IFillLockBytes_FillAt(This, ulOffset, pv, cb, pcbWritten);
}

HRESULT CALLBACK IEnumFORMATETC_Next_Proxy(
    IEnumFORMATETC* This,
    ULONG celt,
    FORMATETC *rgelt,
    ULONG *pceltFetched)
{
    ULONG fetched;
    if (!pceltFetched) pceltFetched = &fetched;
    return IEnumFORMATETC_RemoteNext_Proxy(This, celt, rgelt, pceltFetched);
}

HRESULT __RPC_STUB IEnumFORMATETC_Next_Stub(
    IEnumFORMATETC* This,
    ULONG celt,
    FORMATETC *rgelt,
    ULONG *pceltFetched)
{
    HRESULT hr;
    *pceltFetched = 0;
    hr = IEnumFORMATETC_Next(This, celt, rgelt, pceltFetched);
    if (hr == S_OK) *pceltFetched = celt;
    return hr;
}

HRESULT CALLBACK IEnumSTATDATA_Next_Proxy(
    IEnumSTATDATA* This,
    ULONG celt,
    STATDATA *rgelt,
    ULONG *pceltFetched)
{
    ULONG fetched;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    if (!pceltFetched) pceltFetched = &fetched;
    return IEnumSTATDATA_RemoteNext_Proxy(This, celt, rgelt, pceltFetched);
}

HRESULT __RPC_STUB IEnumSTATDATA_Next_Stub(
    IEnumSTATDATA* This,
    ULONG celt,
    STATDATA *rgelt,
    ULONG *pceltFetched)
{
    HRESULT hr;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    *pceltFetched = 0;
    hr = IEnumSTATDATA_Next(This, celt, rgelt, pceltFetched);
    if (hr == S_OK) *pceltFetched = celt;
    return hr;
}

void CALLBACK IAdviseSink_OnDataChange_Proxy(
    IAdviseSink* This,
    FORMATETC *pFormatetc,
    STGMEDIUM *pStgmed)
{
    TRACE("(%p)->(%p, %p)\n", This, pFormatetc, pStgmed);
    IAdviseSink_RemoteOnDataChange_Proxy(This, pFormatetc, pStgmed);
}

HRESULT __RPC_STUB IAdviseSink_OnDataChange_Stub(
    IAdviseSink* This,
    FORMATETC *pFormatetc,
    ASYNC_STGMEDIUM *pStgmed)
{
    TRACE("(%p)->(%p, %p)\n", This, pFormatetc, pStgmed);
    IAdviseSink_OnDataChange(This, pFormatetc, pStgmed);
    return S_OK;
}

void CALLBACK IAdviseSink_OnViewChange_Proxy(
    IAdviseSink* This,
    DWORD dwAspect,
    LONG lindex)
{
    TRACE("(%p)->(%d, %d)\n", This, dwAspect, lindex);
    IAdviseSink_RemoteOnViewChange_Proxy(This, dwAspect, lindex);
}

HRESULT __RPC_STUB IAdviseSink_OnViewChange_Stub(
    IAdviseSink* This,
    DWORD dwAspect,
    LONG lindex)
{
    TRACE("(%p)->(%d, %d)\n", This, dwAspect, lindex);
    IAdviseSink_OnViewChange(This, dwAspect, lindex);
    return S_OK;
}

void CALLBACK IAdviseSink_OnRename_Proxy(
    IAdviseSink* This,
    IMoniker *pmk)
{
    TRACE("(%p)->(%p)\n", This, pmk);
    IAdviseSink_RemoteOnRename_Proxy(This, pmk);
}

HRESULT __RPC_STUB IAdviseSink_OnRename_Stub(
    IAdviseSink* This,
    IMoniker *pmk)
{
    TRACE("(%p)->(%p)\n", This, pmk);
    IAdviseSink_OnRename(This, pmk);
    return S_OK;
}

void CALLBACK IAdviseSink_OnSave_Proxy(
    IAdviseSink* This)
{
    TRACE("(%p)\n", This);
    IAdviseSink_RemoteOnSave_Proxy(This);
}

HRESULT __RPC_STUB IAdviseSink_OnSave_Stub(
    IAdviseSink* This)
{
    TRACE("(%p)\n", This);
    IAdviseSink_OnSave(This);
    return S_OK;
}

void CALLBACK IAdviseSink_OnClose_Proxy(
    IAdviseSink* This)
{
    TRACE("(%p)\n", This);
    IAdviseSink_RemoteOnClose_Proxy(This);
}

HRESULT __RPC_STUB IAdviseSink_OnClose_Stub(
    IAdviseSink* This)
{
    TRACE("(%p)\n", This);
    IAdviseSink_OnClose(This);
    return S_OK;
}

void CALLBACK IAdviseSink2_OnLinkSrcChange_Proxy(
    IAdviseSink2* This,
    IMoniker *pmk)
{
    TRACE("(%p)->(%p)\n", This, pmk);
    IAdviseSink2_RemoteOnLinkSrcChange_Proxy(This, pmk);
}

HRESULT __RPC_STUB IAdviseSink2_OnLinkSrcChange_Stub(
    IAdviseSink2* This,
    IMoniker *pmk)
{
    TRACE("(%p)->(%p)\n", This, pmk);
    IAdviseSink2_OnLinkSrcChange(This, pmk);
    return S_OK;
}

HRESULT CALLBACK IDataObject_GetData_Proxy(
    IDataObject* This,
    FORMATETC *pformatetcIn,
    STGMEDIUM *pmedium)
{
    TRACE("(%p)->(%p, %p)\n", This, pformatetcIn, pmedium);
    return IDataObject_RemoteGetData_Proxy(This, pformatetcIn, pmedium);
}

HRESULT __RPC_STUB IDataObject_GetData_Stub(
    IDataObject* This,
    FORMATETC *pformatetcIn,
    STGMEDIUM *pRemoteMedium)
{
    TRACE("(%p)->(%p, %p)\n", This, pformatetcIn, pRemoteMedium);
    return IDataObject_GetData(This, pformatetcIn, pRemoteMedium);
}

HRESULT CALLBACK IDataObject_GetDataHere_Proxy(IDataObject *iface, FORMATETC *fmt, STGMEDIUM *med)
{
    IUnknown *release;
    IStorage *stg = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", iface, fmt, med);

    if ((med->tymed & (TYMED_HGLOBAL | TYMED_FILE | TYMED_ISTREAM | TYMED_ISTORAGE)) == 0)
        return DV_E_TYMED;
    if (med->tymed != fmt->tymed)
        return DV_E_TYMED;

    release = med->pUnkForRelease;
    med->pUnkForRelease = NULL;

    if (med->tymed == TYMED_ISTREAM || med->tymed == TYMED_ISTORAGE)
    {
        stg = med->u.pstg; /* This may actually be a stream, but that's ok */
        if (stg) IStorage_AddRef( stg );
    }

    hr = IDataObject_RemoteGetDataHere_Proxy(iface, fmt, med);

    med->pUnkForRelease = release;
    if (stg)
    {
        if (med->u.pstg)
            IStorage_Release( med->u.pstg );
        med->u.pstg = stg;
    }

    return hr;
}

HRESULT __RPC_STUB IDataObject_GetDataHere_Stub(
    IDataObject* This,
    FORMATETC *pformatetc,
    STGMEDIUM *pRemoteMedium)
{
    TRACE("(%p)->(%p, %p)\n", This, pformatetc, pRemoteMedium);
    return IDataObject_GetDataHere(This, pformatetc, pRemoteMedium);
}

HRESULT CALLBACK IDataObject_SetData_Proxy(
    IDataObject* This,
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium,
    BOOL fRelease)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDataObject_SetData_Stub(
    IDataObject* This,
    FORMATETC *pformatetc,
    FLAG_STGMEDIUM *pmedium,
    BOOL fRelease)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

/* call_as/local stubs for oleidl.idl */

HRESULT CALLBACK IOleInPlaceActiveObject_TranslateAccelerator_Proxy(
    IOleInPlaceActiveObject* This,
    LPMSG lpmsg)
{
    TRACE("(%p %p)\n", This, lpmsg);
    return IOleInPlaceActiveObject_RemoteTranslateAccelerator_Proxy(This);
}

HRESULT __RPC_STUB IOleInPlaceActiveObject_TranslateAccelerator_Stub(
    IOleInPlaceActiveObject* This)
{
    TRACE("(%p)\n", This);
    return S_FALSE;
}

HRESULT CALLBACK IOleInPlaceActiveObject_ResizeBorder_Proxy(
    IOleInPlaceActiveObject* This,
    LPCRECT prcBorder,
    IOleInPlaceUIWindow *pUIWindow,
    BOOL fFrameWindow)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IOleInPlaceActiveObject_ResizeBorder_Stub(
    IOleInPlaceActiveObject* This,
    LPCRECT prcBorder,
    REFIID riid,
    IOleInPlaceUIWindow *pUIWindow,
    BOOL fFrameWindow)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IOleCache2_UpdateCache_Proxy(
    IOleCache2* This,
    LPDATAOBJECT pDataObject,
    DWORD grfUpdf,
    LPVOID pReserved)
{
    TRACE("(%p, %p, 0x%08x, %p)\n", This, pDataObject, grfUpdf, pReserved);
    return IOleCache2_RemoteUpdateCache_Proxy(This, pDataObject, grfUpdf, (LONG_PTR)pReserved);
}

HRESULT __RPC_STUB IOleCache2_UpdateCache_Stub(
    IOleCache2* This,
    LPDATAOBJECT pDataObject,
    DWORD grfUpdf,
    LONG_PTR pReserved)
{
    TRACE("(%p, %p, 0x%08x, %li)\n", This, pDataObject, grfUpdf, pReserved);
    return IOleCache2_UpdateCache(This, pDataObject, grfUpdf, (void*)pReserved);
}

HRESULT CALLBACK IEnumOLEVERB_Next_Proxy(
    IEnumOLEVERB* This,
    ULONG celt,
    LPOLEVERB rgelt,
    ULONG *pceltFetched)
{
    ULONG fetched;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    if (!pceltFetched) pceltFetched = &fetched;
    return IEnumOLEVERB_RemoteNext_Proxy(This, celt, rgelt, pceltFetched);
}

HRESULT __RPC_STUB IEnumOLEVERB_Next_Stub(
    IEnumOLEVERB* This,
    ULONG celt,
    LPOLEVERB rgelt,
    ULONG *pceltFetched)
{
    HRESULT hr;
    TRACE("(%p)->(%d, %p, %p)\n", This, celt, rgelt, pceltFetched);
    *pceltFetched = 0;
    hr = IEnumOLEVERB_Next(This, celt, rgelt, pceltFetched);
    if (hr == S_OK) *pceltFetched = celt;
    return hr;
}

HRESULT CALLBACK IViewObject_Draw_Proxy(
    IViewObject* This,
    DWORD dwDrawAspect,
    LONG lindex,
    void *pvAspect,
    DVTARGETDEVICE *ptd,
    HDC hdcTargetDev,
    HDC hdcDraw,
    LPCRECTL lprcBounds,
    LPCRECTL lprcWBounds,
    BOOL (STDMETHODCALLTYPE *pfnContinue)(ULONG_PTR dwContinue),
    ULONG_PTR dwContinue)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IViewObject_Draw_Stub(
    IViewObject* This,
    DWORD dwDrawAspect,
    LONG lindex,
    ULONG_PTR pvAspect,
    DVTARGETDEVICE *ptd,
    ULONG_PTR hdcTargetDev,
    ULONG_PTR hdcDraw,
    LPCRECTL lprcBounds,
    LPCRECTL lprcWBounds,
    IContinue *pContinue)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IViewObject_GetColorSet_Proxy(
    IViewObject* This,
    DWORD dwDrawAspect,
    LONG lindex,
    void *pvAspect,
    DVTARGETDEVICE *ptd,
    HDC hicTargetDev,
    LOGPALETTE **ppColorSet)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IViewObject_GetColorSet_Stub(
    IViewObject* This,
    DWORD dwDrawAspect,
    LONG lindex,
    ULONG_PTR pvAspect,
    DVTARGETDEVICE *ptd,
    ULONG_PTR hicTargetDev,
    LOGPALETTE **ppColorSet)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IViewObject_Freeze_Proxy(
    IViewObject* This,
    DWORD dwDrawAspect,
    LONG lindex,
    void *pvAspect,
    DWORD *pdwFreeze)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IViewObject_Freeze_Stub(
    IViewObject* This,
    DWORD dwDrawAspect,
    LONG lindex,
    ULONG_PTR pvAspect,
    DWORD *pdwFreeze)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IViewObject_GetAdvise_Proxy(
    IViewObject* This,
    DWORD *pAspects,
    DWORD *pAdvf,
    IAdviseSink **ppAdvSink)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IViewObject_GetAdvise_Stub(
    IViewObject* This,
    DWORD *pAspects,
    DWORD *pAdvf,
    IAdviseSink **ppAdvSink)
{
    FIXME(":stub\n");
    return E_NOTIMPL;
}
