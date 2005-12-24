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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#include "ole2.h"
#include "oleauto.h"
#include "rpcproxy.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align))&~(_Align))
#define ALIGNED_POINTER(_Ptr, _Align) ((LPVOID)ALIGNED_LENGTH((ULONG_PTR)(_Ptr), _Align))
#define ALIGN_LENGTH(_Len, _Align) _Len = ALIGNED_LENGTH(_Len, _Align)
#define ALIGN_POINTER(_Ptr, _Align) _Ptr = ALIGNED_POINTER(_Ptr, _Align)

static void dump_user_flags(unsigned long *pFlags)
{
    if (HIWORD(*pFlags) == NDR_LOCAL_DATA_REPRESENTATION)
        TRACE("MAKELONG(NDR_LOCAL_REPRESENTATION, ");
    else
        TRACE("MAKELONG(0x%04x, ", HIWORD(*pFlags));
    switch (LOWORD(*pFlags))
    {
    case MSHCTX_LOCAL: TRACE("MSHCTX_LOCAL)"); break;
    case MSHCTX_NOSHAREDMEM: TRACE("MSHCTX_NOSHAREDMEM)"); break;
    case MSHCTX_DIFFERENTMACHINE: TRACE("MSHCTX_DIFFERENTMACHINE)"); break;
    case MSHCTX_INPROC: TRACE("MSHCTX_INPROC)"); break;
    default: TRACE("%d)", LOWORD(*pFlags));
    }
}

unsigned long __RPC_USER CLIPFORMAT_UserSize(unsigned long *pFlags, unsigned long StartingSize, CLIPFORMAT *pCF)
{
    unsigned long size = StartingSize;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %ld, %p\n", StartingSize, pCF);

    size += sizeof(userCLIPFORMAT);

    /* only need to marshal the name if it is not a pre-defined type and
     * we are going remote */
    if ((*pCF >= 0xc000) && (LOWORD(*pFlags) == MSHCTX_DIFFERENTMACHINE))
    {
        WCHAR format[255];
        INT ret;
        size += 3 * sizeof(INT);
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

unsigned char * __RPC_USER CLIPFORMAT_UserMarshal(unsigned long *pFlags, unsigned char *pBuffer, CLIPFORMAT *pCF)
{
    wireCLIPFORMAT wirecf = (wireCLIPFORMAT)pBuffer;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %p, &0x%04x\n", pBuffer, *pCF);

    wirecf->u.dwValue = *pCF;
    pBuffer += sizeof(*wirecf);

    /* only need to marshal the name if it is not a pre-defined type and
     * we are going remote */
    if ((*pCF >= 0xc000) && (LOWORD(*pFlags) == MSHCTX_DIFFERENTMACHINE))
    {
        WCHAR format[255];
        INT len;
        wirecf->fContext = WDT_REMOTE_CALL;
        len = GetClipboardFormatNameW(*pCF, format, sizeof(format)/sizeof(format[0])-1);
        if (!len)
            RaiseException(DV_E_CLIPFORMAT, 0, 0, NULL);
        len += 1;
        *(INT *)pBuffer = len;
        pBuffer += sizeof(INT);
        *(INT *)pBuffer = 0;
        pBuffer += sizeof(INT);
        *(INT *)pBuffer = len;
        pBuffer += sizeof(INT);
        TRACE("marshaling format name %s\n", debugstr_wn(format, len-1));
        lstrcpynW((LPWSTR)pBuffer, format, len);
        pBuffer += len * sizeof(WCHAR);
        *(WCHAR *)pBuffer = '\0';
        pBuffer += sizeof(WCHAR);
    }
    else
        wirecf->fContext = WDT_INPROC_CALL;

    return pBuffer;
}

unsigned char * __RPC_USER CLIPFORMAT_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, CLIPFORMAT *pCF)
{
    wireCLIPFORMAT wirecf = (wireCLIPFORMAT)pBuffer;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %p, %p\n", pBuffer, pCF);

    pBuffer += sizeof(*wirecf);
    if (wirecf->fContext == WDT_INPROC_CALL)
        *pCF = (CLIPFORMAT)wirecf->u.dwValue;
    else if (wirecf->fContext == WDT_REMOTE_CALL)
    {
        CLIPFORMAT cf;
        INT len = *(INT *)pBuffer;
        pBuffer += sizeof(INT);
        if (*(INT *)pBuffer != 0)
            RaiseException(RPC_S_INVALID_BOUND, 0, 0, NULL);
        pBuffer += sizeof(INT);
        if (*(INT *)pBuffer != len)
            RaiseException(RPC_S_INVALID_BOUND, 0, 0, NULL);
        pBuffer += sizeof(INT);
        if (((WCHAR *)pBuffer)[len] != '\0')
            RaiseException(RPC_S_INVALID_BOUND, 0, 0, NULL);
        TRACE("unmarshaling clip format %s\n", debugstr_w((LPCWSTR)pBuffer));
        cf = RegisterClipboardFormatW((LPCWSTR)pBuffer);
        pBuffer += (len + 1) * sizeof(WCHAR);
        if (!cf)
            RaiseException(DV_E_CLIPFORMAT, 0, 0, NULL);
        *pCF = cf;
    }
    else
        /* code not really appropriate, but nearest I can find */
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);
    return pBuffer;
}

void __RPC_USER CLIPFORMAT_UserFree(unsigned long *pFlags, CLIPFORMAT *pCF)
{
    /* there is no inverse of the RegisterClipboardFormat function,
     * so nothing to do */
}

static unsigned long __RPC_USER handle_UserSize(unsigned long *pFlags, unsigned long StartingSize, HANDLE *handle)
{
    if (LOWORD(*pFlags) == MSHCTX_DIFFERENTMACHINE)
    {
        ERR("can't remote a local handle\n");
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);
        return StartingSize;
    }
    return StartingSize + sizeof(RemotableHandle);
}

static unsigned char * __RPC_USER handle_UserMarshal(unsigned long *pFlags, unsigned char *pBuffer, HANDLE *handle)
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

static unsigned char * __RPC_USER handle_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, HANDLE *handle)
{
    RemotableHandle *remhandle = (RemotableHandle *)pBuffer;
    if (remhandle->fContext != WDT_INPROC_CALL)
        RaiseException(RPC_X_BAD_STUB_DATA, 0, 0, NULL);
    *handle = (HANDLE)remhandle->u.hInproc;
    return pBuffer + sizeof(RemotableHandle);
}

static void __RPC_USER handle_UserFree(unsigned long *pFlags, HANDLE *phMenu)
{
    /* nothing to do */
}

#define IMPL_WIREM_HANDLE(type) \
    unsigned long __RPC_USER type##_UserSize(unsigned long *pFlags, unsigned long StartingSize, type *handle) \
    { \
        TRACE("("); dump_user_flags(pFlags); TRACE(", %ld, %p\n", StartingSize, handle); \
        return handle_UserSize(pFlags, StartingSize, (HANDLE *)handle); \
    } \
    \
    unsigned char * __RPC_USER type##_UserMarshal(unsigned long *pFlags, unsigned char *pBuffer, type *handle) \
    { \
        TRACE("("); dump_user_flags(pFlags); TRACE(", %p, &%p\n", pBuffer, *handle); \
        return handle_UserMarshal(pFlags, pBuffer, (HANDLE *)handle); \
    } \
    \
    unsigned char * __RPC_USER type##_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, type *handle) \
    { \
        TRACE("("); dump_user_flags(pFlags); TRACE(", %p, %p\n", pBuffer, handle); \
        return handle_UserUnmarshal(pFlags, pBuffer, (HANDLE *)handle); \
    } \
    \
    void __RPC_USER type##_UserFree(unsigned long *pFlags, type *handle) \
    { \
        TRACE("("); dump_user_flags(pFlags); TRACE(", &%p\n", *handle); \
        return handle_UserFree(pFlags, (HANDLE *)handle); \
    }

IMPL_WIREM_HANDLE(HACCEL)
IMPL_WIREM_HANDLE(HMENU)
IMPL_WIREM_HANDLE(HWND)

unsigned long __RPC_USER HGLOBAL_UserSize(unsigned long *pFlags, unsigned long StartingSize, HGLOBAL *phGlobal)
{
    unsigned long size = StartingSize;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %ld, %p\n", StartingSize, phGlobal);

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
            size += (unsigned long)ret;
        }
    }
    
    return size;
}

unsigned char * __RPC_USER HGLOBAL_UserMarshal(unsigned long *pFlags, unsigned char *pBuffer, HGLOBAL *phGlobal)
{
    TRACE("("); dump_user_flags(pFlags); TRACE(", %p, &%p\n", pBuffer, *phGlobal);

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
        *(ULONG *)pBuffer = (ULONG)*phGlobal;
        pBuffer += sizeof(ULONG);
        if (*phGlobal)
        {
            const unsigned char *memory;
            SIZE_T size = GlobalSize(*phGlobal);
            *(ULONG *)pBuffer = (ULONG)size;
            pBuffer += sizeof(ULONG);
            *(ULONG *)pBuffer = (ULONG)*phGlobal;
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

unsigned char * __RPC_USER HGLOBAL_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, HGLOBAL *phGlobal)
{
    ULONG fContext;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %p, &%p\n", pBuffer, *phGlobal);

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

void __RPC_USER HGLOBAL_UserFree(unsigned long *pFlags, HGLOBAL *phGlobal)
{
    TRACE("("); dump_user_flags(pFlags); TRACE(", &%p\n", *phGlobal);

    if (LOWORD(*pFlags != MSHCTX_INPROC) && *phGlobal)
        GlobalFree(*phGlobal);
}

unsigned long __RPC_USER HBITMAP_UserSize(unsigned long *pFlags, unsigned long StartingSize, HBITMAP *phBmp)
{
    FIXME(":stub\n");
    return StartingSize;
}

unsigned char * __RPC_USER HBITMAP_UserMarshal(unsigned long *pFlags, unsigned char *pBuffer, HBITMAP *phBmp)
{
    FIXME(":stub\n");
    return pBuffer;
}

unsigned char * __RPC_USER HBITMAP_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, HBITMAP *phBmp)
{
    FIXME(":stub\n");
    return pBuffer;
}

void __RPC_USER HBITMAP_UserFree(unsigned long *pFlags, HBITMAP *phBmp)
{
    FIXME(":stub\n");
}

unsigned long __RPC_USER HDC_UserSize(unsigned long *pFlags, unsigned long StartingSize, HDC *phdc)
{
    FIXME(":stub\n");
    return StartingSize;
}

unsigned char * __RPC_USER HDC_UserMarshal(unsigned long *pFlags, unsigned char *pBuffer, HDC *phdc)
{
    FIXME(":stub\n");
    return pBuffer;
}

unsigned char * __RPC_USER HDC_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, HDC *phdc)
{
    FIXME(":stub\n");
    return pBuffer;
}

void __RPC_USER HDC_UserFree(unsigned long *pFlags, HDC *phdc)
{
    FIXME(":stub\n");
}

unsigned long __RPC_USER HPALETTE_UserSize(unsigned long *pFlags, unsigned long StartingSize, HPALETTE *phPal)
{
    FIXME(":stub\n");
    return StartingSize;
}

unsigned char * __RPC_USER HPALETTE_UserMarshal(unsigned long *pFlags, unsigned char *pBuffer, HPALETTE *phPal)
{
    FIXME(":stub\n");
    return pBuffer;
}

unsigned char * __RPC_USER HPALETTE_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, HPALETTE *phPal)
{
    FIXME(":stub\n");
    return pBuffer;
}

void __RPC_USER HPALETTE_UserFree(unsigned long *pFlags, HPALETTE *phPal)
{
    FIXME(":stub\n");
}


unsigned long __RPC_USER HENHMETAFILE_UserSize(unsigned long *pFlags, unsigned long StartingSize, HENHMETAFILE *phEmf)
{
    unsigned long size = StartingSize;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %ld, %p\n", StartingSize, *phEmf);

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

unsigned char * __RPC_USER HENHMETAFILE_UserMarshal(unsigned long *pFlags, unsigned char *pBuffer, HENHMETAFILE *phEmf)
{
    TRACE("("); dump_user_flags(pFlags); TRACE(", %p, &%p\n", pBuffer, *phEmf);

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

unsigned char * __RPC_USER HENHMETAFILE_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, HENHMETAFILE *phEmf)
{
    ULONG fContext;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %p, %p\n", pBuffer, phEmf);

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

void __RPC_USER HENHMETAFILE_UserFree(unsigned long *pFlags, HENHMETAFILE *phEmf)
{
    TRACE("("); dump_user_flags(pFlags); TRACE(", &%p\n", *phEmf);

    if (LOWORD(*pFlags) != MSHCTX_INPROC)
        DeleteEnhMetaFile(*phEmf);
}

unsigned long __RPC_USER STGMEDIUM_UserSize(unsigned long *pFlags, unsigned long StartingSize, STGMEDIUM *pStgMedium)
{
    unsigned long size = StartingSize;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %ld, %p\n", StartingSize, pStgMedium);

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
        size = HGLOBAL_UserSize(pFlags, size, &pStgMedium->u.hGlobal);
        break;
    case TYMED_FILE:
        FIXME("TYMED_FILE\n");
        break;
    case TYMED_ISTREAM:
        FIXME("TYMED_ISTREAM\n");
        break;
    case TYMED_ISTORAGE:
        FIXME("TYMED_ISTORAGE\n");
        break;
    case TYMED_GDI:
        FIXME("TYMED_GDI\n");
        break;
    case TYMED_MFPICT:
        FIXME("TYMED_MFPICT\n");
        break;
    case TYMED_ENHMF:
        TRACE("TYMED_ENHMF\n");
        size = HENHMETAFILE_UserSize(pFlags, size, &pStgMedium->u.hEnhMetaFile);
        break;
    default:
        RaiseException(DV_E_TYMED, 0, 0, NULL);
    }

    if (pStgMedium->pUnkForRelease)
        FIXME("buffer size pUnkForRelease\n");

    return size;
}

unsigned char * __RPC_USER STGMEDIUM_UserMarshal(unsigned long *pFlags, unsigned char *pBuffer, STGMEDIUM *pStgMedium)
{
    TRACE("("); dump_user_flags(pFlags); TRACE(", %p, %p\n", pBuffer, pStgMedium);

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
        pBuffer = HGLOBAL_UserMarshal(pFlags, pBuffer, &pStgMedium->u.hGlobal);
        break;
    case TYMED_FILE:
        FIXME("TYMED_FILE\n");
        break;
    case TYMED_ISTREAM:
        FIXME("TYMED_ISTREAM\n");
        break;
    case TYMED_ISTORAGE:
        FIXME("TYMED_ISTORAGE\n");
        break;
    case TYMED_GDI:
        FIXME("TYMED_GDI\n");
        break;
    case TYMED_MFPICT:
        FIXME("TYMED_MFPICT\n");
        break;
    case TYMED_ENHMF:
        TRACE("TYMED_ENHMF\n");
        pBuffer = HENHMETAFILE_UserMarshal(pFlags, pBuffer, &pStgMedium->u.hEnhMetaFile);
        break;
    default:
        RaiseException(DV_E_TYMED, 0, 0, NULL);
    }

    if (pStgMedium->pUnkForRelease)
        FIXME("marshal pUnkForRelease\n");

    return pBuffer;
}

unsigned char * __RPC_USER STGMEDIUM_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, STGMEDIUM *pStgMedium)
{
    DWORD content;
    DWORD releaseunk;

    ALIGN_POINTER(pBuffer, 3);

    TRACE("("); dump_user_flags(pFlags); TRACE(", %p, %p\n", pBuffer, pStgMedium);

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
        pBuffer = HGLOBAL_UserUnmarshal(pFlags, pBuffer, &pStgMedium->u.hGlobal);
        break;
    case TYMED_FILE:
        FIXME("TYMED_FILE\n");
        break;
    case TYMED_ISTREAM:
        FIXME("TYMED_ISTREAM\n");
        break;
    case TYMED_ISTORAGE:
        FIXME("TYMED_ISTORAGE\n");
        break;
    case TYMED_GDI:
        FIXME("TYMED_GDI\n");
        break;
    case TYMED_MFPICT:
        FIXME("TYMED_MFPICT\n");
        break;
    case TYMED_ENHMF:
        TRACE("TYMED_ENHMF\n");
        pBuffer = HENHMETAFILE_UserUnmarshal(pFlags, pBuffer, &pStgMedium->u.hEnhMetaFile);
        break;
    default:
        RaiseException(DV_E_TYMED, 0, 0, NULL);
    }

    pStgMedium->pUnkForRelease = NULL;
    if (releaseunk)
        FIXME("unmarshal pUnkForRelease\n");

    return pBuffer;
}

void __RPC_USER STGMEDIUM_UserFree(unsigned long *pFlags, STGMEDIUM *pStgMedium)
{
    TRACE("("); dump_user_flags(pFlags); TRACE(", %p\n", pStgMedium);

    ReleaseStgMedium(pStgMedium);
}

unsigned long __RPC_USER ASYNC_STGMEDIUM_UserSize(unsigned long *pFlags, unsigned long StartingSize, ASYNC_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
    return StartingSize;
}

unsigned char * __RPC_USER ASYNC_STGMEDIUM_UserMarshal(  unsigned long *pFlags, unsigned char *pBuffer, ASYNC_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
    return pBuffer;
}

unsigned char * __RPC_USER ASYNC_STGMEDIUM_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, ASYNC_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
    return pBuffer;
}

void __RPC_USER ASYNC_STGMEDIUM_UserFree(unsigned long *pFlags, ASYNC_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
}

unsigned long __RPC_USER FLAG_STGMEDIUM_UserSize(unsigned long *pFlags, unsigned long StartingSize, FLAG_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
    return StartingSize;
}

unsigned char * __RPC_USER FLAG_STGMEDIUM_UserMarshal(  unsigned long *pFlags, unsigned char *pBuffer, FLAG_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
    return pBuffer;
}

unsigned char * __RPC_USER FLAG_STGMEDIUM_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, FLAG_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
    return pBuffer;
}

void __RPC_USER FLAG_STGMEDIUM_UserFree(unsigned long *pFlags, FLAG_STGMEDIUM *pStgMedium)
{
    FIXME(":stub\n");
}

unsigned long __RPC_USER SNB_UserSize(unsigned long *pFlags, unsigned long StartingSize, SNB *pSnb)
{
    FIXME(":stub\n");
    return StartingSize;
}

unsigned char * __RPC_USER SNB_UserMarshal(  unsigned long *pFlags, unsigned char *pBuffer, SNB *pSnb)
{
    FIXME(":stub\n");
    return pBuffer;
}

unsigned char * __RPC_USER SNB_UserUnmarshal(unsigned long *pFlags, unsigned char *pBuffer, SNB *pSnb)
{
    FIXME(":stub\n");
    return pBuffer;
}

void __RPC_USER SNB_UserFree(unsigned long *pFlags, SNB *pSnb)
{
    FIXME(":stub\n");
}
