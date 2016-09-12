/*
 * metafile driver text functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wine/wingdi16.h"
#include "mfdrv/metafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(metafile);


/******************************************************************
 *         MFDRV_MetaExtTextOut
 */
static BOOL MFDRV_MetaExtTextOut( PHYSDEV dev, short x, short y, UINT16 flags,
				 const RECT16 *rect, LPCSTR str, short count,
				 const INT16 *lpDx)
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;
    BOOL isrect = flags & (ETO_CLIPPED | ETO_OPAQUE);

    len = sizeof(METARECORD) + (((count + 1) >> 1) * 2) + 2 * sizeof(short)
	    + sizeof(UINT16);
    if (isrect)
        len += sizeof(RECT16);
    if (lpDx)
     len+=count*sizeof(INT16);
    if (!(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len)))
	return FALSE;

    mr->rdSize = len / 2;
    mr->rdFunction = META_EXTTEXTOUT;
    *(mr->rdParm) = y;
    *(mr->rdParm + 1) = x;
    *(mr->rdParm + 2) = count;
    *(mr->rdParm + 3) = flags;
    if (isrect) memcpy(mr->rdParm + 4, rect, sizeof(RECT16));
    memcpy(mr->rdParm + (isrect ? 8 : 4), str, count);
    if (lpDx)
     memcpy(mr->rdParm + (isrect ? 8 : 4) + ((count + 1) >> 1),lpDx,
      count*sizeof(INT16));
    ret = MFDRV_WriteRecord( dev, mr, mr->rdSize * 2);
    HeapFree( GetProcessHeap(), 0, mr);
    return ret;
}



/***********************************************************************
 *           MFDRV_ExtTextOut
 */
BOOL MFDRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags,
                       const RECT *lprect, LPCWSTR str, UINT count, const INT *lpDx )
{
    RECT16	rect16;
    LPINT16	lpdx16 = NULL;
    BOOL	ret;
    unsigned int i, j;
    LPSTR       ascii;
    DWORD len;
    CHARSETINFO csi;
    int charset = GetTextCharset( dev->hdc );
    UINT cp = CP_ACP;

    if(TranslateCharsetInfo(ULongToPtr(charset), &csi, TCI_SRCCHARSET))
        cp = csi.ciACP;
    else {
        switch(charset) {
	case OEM_CHARSET:
	    cp = GetOEMCP();
	    break;
	case DEFAULT_CHARSET:
	    cp = GetACP();
	    break;

	case VISCII_CHARSET:
	case TCVN_CHARSET:
	case KOI8_CHARSET:
	case ISO3_CHARSET:
	case ISO4_CHARSET:
	case ISO10_CHARSET:
	case CELTIC_CHARSET:
	  /* FIXME: These have no place here, but because x11drv
	     enumerates fonts with these (made up) charsets some apps
	     might use them and then the FIXME below would become
	     annoying.  Now we could pick the intended codepage for
	     each of these, but since it's broken anyway we'll just
	     use CP_ACP and hope it'll go away...
	  */
	    cp = CP_ACP;
	    break;


	default:
	    FIXME("Can't find codepage for charset %d\n", charset);
	    break;
	}
    }


    TRACE("cp == %d\n", cp);
    len = WideCharToMultiByte(cp, 0, str, count, NULL, 0, NULL, NULL);
    ascii = HeapAlloc(GetProcessHeap(), 0, len);
    WideCharToMultiByte(cp, 0, str, count, ascii, len, NULL, NULL);
    TRACE("mapped %s -> %s\n", debugstr_wn(str, count), debugstr_an(ascii, len));


    if (lprect)
    {
        rect16.left   = lprect->left;
        rect16.top    = lprect->top;
        rect16.right  = lprect->right;
        rect16.bottom = lprect->bottom;
    }

    if(lpDx) {
        lpdx16 = HeapAlloc( GetProcessHeap(), 0, sizeof(INT16)*len );
	for(i = j = 0; i < len; )
	    if(IsDBCSLeadByteEx(cp, ascii[i])) {
	        lpdx16[i++] = lpDx[j++];
		lpdx16[i++] = 0;
	    } else
	        lpdx16[i++] = lpDx[j++];
    }

    ret = MFDRV_MetaExtTextOut(dev,x,y,flags,lprect?&rect16:NULL,ascii,len,lpdx16);
    HeapFree( GetProcessHeap(), 0, ascii );
    HeapFree( GetProcessHeap(), 0, lpdx16 );
    return ret;
}
