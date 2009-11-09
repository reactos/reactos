/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
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
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

HRESULT copy_pixels(UINT bpp, const BYTE *srcbuffer,
    UINT srcwidth, UINT srcheight, INT srcstride,
    const WICRect *rc, UINT dststride, UINT dstbuffersize, BYTE *dstbuffer)
{
    UINT bytesperrow;
    UINT row_offset; /* number of bits into the source rows where the data starts */

    if (rc->X < 0 || rc->Y < 0 || rc->X+rc->Width > srcwidth || rc->Y+rc->Height > srcheight)
        return E_INVALIDARG;

    bytesperrow = ((bpp * rc->Width)+7)/8;

    if (dststride < bytesperrow)
        return E_INVALIDARG;

    if ((dststride * rc->Height) > dstbuffersize)
        return E_INVALIDARG;

    row_offset = rc->X * bpp;

    if (row_offset % 8 == 0)
    {
        /* everything lines up on a byte boundary */
        UINT row;
        const BYTE *src;
        BYTE *dst;

        src = srcbuffer + (row_offset / 8) + srcstride * rc->Y;
        dst = dstbuffer;
        for (row=0; row < rc->Height; row++)
        {
            memcpy(dst, src, bytesperrow);
            src += srcstride;
            dst += dststride;
        }
        return S_OK;
    }
    else
    {
        /* we have to do a weird bitwise copy. eww. */
        FIXME("cannot reliably copy bitmap data if bpp < 8\n");
        return E_FAIL;
    }
}
