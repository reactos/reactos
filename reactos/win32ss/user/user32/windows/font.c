/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/windows/font.c
 * PURPOSE:         Draw Text
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(text);

DWORD WINAPI GdiGetCodePage(HDC hdc);

INT WINAPI DrawTextExWorker( HDC hdc, LPWSTR str, INT i_count,
                        LPRECT rect, UINT flags, LPDRAWTEXTPARAMS dtp );


/* FUNCTIONS *****************************************************************/


/***********************************************************************
 *           TEXT_TabbedTextOut
 *
 * Helper function for TabbedTextOut() and GetTabbedTextExtent().
 * Note: this doesn't work too well for text-alignment modes other
 *       than TA_LEFT|TA_TOP. But we want bug-for-bug compatibility :-)
 */
/* WINE synced 22-May-2006 */
LONG TEXT_TabbedTextOut( HDC hdc,
                         INT x,
                         INT y,
                         LPCWSTR lpstr,
                         INT count,
                         INT cTabStops,
                         const INT *lpTabPos,
                         INT nTabOrg,
                         BOOL fDisplayText )
{
    INT defWidth;
    SIZE extent;
    int i, j;
    int start = x;
    TEXTMETRICW tm;

    if (!lpTabPos)
        cTabStops=0;

    GetTextMetricsW( hdc, &tm );

    if (cTabStops == 1)
    {
        defWidth = *lpTabPos;
        cTabStops = 0;
    }
    else
    {
        defWidth = 8 * tm.tmAveCharWidth;
    }

    while (count > 0)
    {
        RECT r;
        INT x0;
        x0 = x;
        r.left = x0;
        /* chop the string into substrings of 0 or more <tabs>
         * possibly followed by 1 or more normal characters */
        for (i = 0; i < count; i++)
            if (lpstr[i] != '\t') break;
        for (j = i; j < count; j++)
            if (lpstr[j] == '\t') break;
        /* get the extent of the normal character part */
        GetTextExtentPointW( hdc, lpstr + i, j - i , &extent );
        /* and if there is a <tab>, calculate its position */
        if( i) {
            /* get x coordinate for the drawing of this string */
            for (; cTabStops > i; lpTabPos++, cTabStops--)
            {
                if( nTabOrg + abs( *lpTabPos) > x) {
                    if( lpTabPos[ i - 1] >= 0) {
                        /* a left aligned tab */
                        x = nTabOrg + lpTabPos[ i-1] + extent.cx;
                        break;
                    }
                    else
                    {
                        /* if tab pos is negative then text is right-aligned
                         * to tab stop meaning that the string extends to the
                         * left, so we must subtract the width of the string */
                        if (nTabOrg - lpTabPos[ i - 1] - extent.cx > x)
                        {
                            x = nTabOrg - lpTabPos[ i - 1];
                            x0 = x - extent.cx;
                            break;
                        }
                    }
                }
            }
            /* if we have run out of tab stops and we have a valid default tab
             * stop width then round x up to that width */
            if ((cTabStops <= i) && (defWidth > 0)) {
                x0 = nTabOrg + ((x - nTabOrg) / defWidth + i) * defWidth;
                x = x0 + extent.cx;
            } else if ((cTabStops <= i) && (defWidth < 0)) {
                x = nTabOrg + ((x - nTabOrg + extent.cx) / -defWidth + i)
                    * -defWidth;
                x0 = x - extent.cx;
            }
        } else
            x += extent.cx;

        if (fDisplayText)
        {
            r.top    = y;
            r.right  = x;
            r.bottom = y + extent.cy;

            ExtTextOutW( hdc, x0, y, GetBkMode(hdc) == OPAQUE ? ETO_OPAQUE : 0,
                         &r, lpstr + i, j - i, NULL );
        }
        count -= j;
        lpstr += j;
    }

    if(!extent.cy)
        extent.cy = tm.tmHeight;

    return MAKELONG(x - start, extent.cy);
}

/*
 * @implemented
 */
LONG
WINAPI
TabbedTextOutA(
  HDC hDC,
  int X,
  int Y,
  LPCSTR lpString,
  int nCount,
  int nTabPositions,
  CONST INT *lpnTabStopPositions,
  int nTabOrigin)
{
  LONG ret;
  DWORD len;
  LPWSTR strW;
  UINT cp = GdiGetCodePage( hDC ); // CP_ACP

  len = MultiByteToWideChar(cp, 0, lpString, nCount, NULL, 0);
  if (!len) return 0;
  strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
  if (!strW) return 0;
  MultiByteToWideChar(cp, 0, lpString, nCount, strW, len);
  ret = TabbedTextOutW(hDC, X, Y, strW, len, nTabPositions, lpnTabStopPositions, nTabOrigin);
  HeapFree(GetProcessHeap(), 0, strW);
  return ret;
}

/*
 * @implemented
 */
LONG
WINAPI
TabbedTextOutW(
  HDC hDC,
  int X,
  int Y,
  LPCWSTR lpString,
  int nCount,
  int nTabPositions,
  CONST INT *lpnTabStopPositions,
  int nTabOrigin)
{
  return TEXT_TabbedTextOut(hDC, X, Y, lpString, nCount, nTabPositions, lpnTabStopPositions, nTabOrigin, TRUE);
}

/* WINE synced 22-May-2006 */
/*
 * @implemented
 */
DWORD
WINAPI
GetTabbedTextExtentA(
  HDC hDC,
  LPCSTR lpString,
  int nCount,
  int nTabPositions,
  CONST INT *lpnTabStopPositions)
{
    LONG ret;
    UINT cp = GdiGetCodePage( hDC ); // CP_ACP
    DWORD len;
    LPWSTR strW;

    len = MultiByteToWideChar(cp, 0, lpString, nCount, NULL, 0);
    if (!len) return 0;
    strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!strW) return 0;
    MultiByteToWideChar(cp, 0, lpString, nCount, strW, len);
    ret = GetTabbedTextExtentW(hDC, strW, len, nTabPositions, lpnTabStopPositions);
    HeapFree(GetProcessHeap(), 0, strW);
    return ret;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetTabbedTextExtentW(
  HDC hDC,
  LPCWSTR lpString,
  int nCount,
  int nTabPositions,
  CONST INT *lpnTabStopPositions)
{
   return TEXT_TabbedTextOut(hDC, 0, 0, lpString, nCount, nTabPositions, lpnTabStopPositions, 0, FALSE);
}


/***********************************************************************
 *           DrawTextExW    (USER32.@)
 *
 * The documentation on the extra space required for DT_MODIFYSTRING at MSDN
 * is not quite complete, especially with regard to \0.  We will assume that
 * the returned string could have a length of up to i_count+3 and also have
 * a trailing \0 (which would be 4 more than a not-null-terminated string but
 * 3 more than a null-terminated string).  If this is not so then increase
 * the allowance in DrawTextExA.
 */
#define MAX_BUFFER 1024
/* 
 * @implemented
 *
 * Synced with Wine Staging 1.7.37
 */
INT WINAPI DrawTextExW( HDC hdc, LPWSTR str, INT i_count,
                        LPRECT rect, UINT flags, LPDRAWTEXTPARAMS dtp )
{
   return DrawTextExWorker( hdc, str, i_count, rect, flags, dtp );
}

/***********************************************************************
 *           DrawTextExA    (USER32.@)
 *
 * If DT_MODIFYSTRING is specified then there must be room for up to
 * 4 extra characters.  We take great care about just how much modified
 * string we return.
 *
 * @implemented
 *
 * Synced with Wine Staging 1.7.37
 */
INT WINAPI DrawTextExA( HDC hdc, LPSTR str, INT count,
                        LPRECT rect, UINT flags, LPDRAWTEXTPARAMS dtp )
{
   WCHAR *wstr;
   WCHAR *p;
   INT ret = 0;
   int i;
   DWORD wcount;
   DWORD wmax;
   DWORD amax;
   UINT cp;

   if (!count) return 0;
   if (!str && count > 0) return 0;
   if( !str || ((count == -1) && !(count = strlen(str))))
   {
        int lh;
        TEXTMETRICA tm;

        if (dtp && dtp->cbSize != sizeof(DRAWTEXTPARAMS))
            return 0;

        GetTextMetricsA(hdc, &tm);
        if (flags & DT_EXTERNALLEADING)
            lh = tm.tmHeight + tm.tmExternalLeading;
        else
            lh = tm.tmHeight;

        if( flags & DT_CALCRECT)
        {
            rect->right = rect->left;
            if( flags & DT_SINGLELINE)
                rect->bottom = rect->top + lh;
            else
                rect->bottom = rect->top;
        }
        return lh;
   }
   cp = GdiGetCodePage( hdc );
   wcount = MultiByteToWideChar( cp, 0, str, count, NULL, 0 );
   wmax = wcount;
   amax = count;
   if (flags & DT_MODIFYSTRING)
   {
        wmax += 4;
        amax += 4;
   }
   wstr = HeapAlloc(GetProcessHeap(), 0, wmax * sizeof(WCHAR));
   if (wstr)
   {
       MultiByteToWideChar( cp, 0, str, count, wstr, wcount );
       if (flags & DT_MODIFYSTRING)
           for (i=4, p=wstr+wcount; i--; p++) *p=0xFFFE;
           /* Initialise the extra characters so that we can see which ones
            * change.  U+FFFE is guaranteed to be not a unicode character and
            * so will not be generated by DrawTextEx itself.
            */
       ret = DrawTextExW( hdc, wstr, wcount, rect, flags, dtp );
       if (flags & DT_MODIFYSTRING)
       {
            /* Unfortunately the returned string may contain multiple \0s
             * and so we need to measure it ourselves.
             */
            for (i=4, p=wstr+wcount; i-- && *p != 0xFFFE; p++) wcount++;
            WideCharToMultiByte( cp, 0, wstr, wcount, str, amax, NULL, NULL );
       }
       HeapFree(GetProcessHeap(), 0, wstr);
   }
   return ret;
}

/***********************************************************************
 *           DrawTextW    (USER32.@)
 *
 * @implemented
 * Synced with Wine Staging 1.7.37
 */
INT WINAPI DrawTextW( HDC hdc, LPCWSTR str, INT count, LPRECT rect, UINT flags )
{
    DRAWTEXTPARAMS dtp;

    memset (&dtp, 0, sizeof(dtp));
    dtp.cbSize = sizeof(dtp);
    if (flags & DT_TABSTOP)
    {
        dtp.iTabLength = (flags >> 8) & 0xff;
        flags &= 0xffff00ff;
    }
    return DrawTextExW(hdc, (LPWSTR)str, count, rect, flags, &dtp);
}

/***********************************************************************
 *           DrawTextA    (USER32.@)
 *
 * @implemented
 * Synced with Wine Staging 1.7.37
 */
INT WINAPI DrawTextA( HDC hdc, LPCSTR str, INT count, LPRECT rect, UINT flags )
{
    DRAWTEXTPARAMS dtp;

    memset (&dtp, 0, sizeof(dtp));
    dtp.cbSize = sizeof(dtp);
    if (flags & DT_TABSTOP)
    {
        dtp.iTabLength = (flags >> 8) & 0xff;
        flags &= 0xffff00ff;
    }
    return DrawTextExA( hdc, (LPSTR)str, count, rect, flags, &dtp );
}
