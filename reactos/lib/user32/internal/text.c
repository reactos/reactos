/*
 * text functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 *
 */

//#include <stdlib.h>
#include <windows.h>
#include <user32/text.h>




int tabstop;
int tabwidth;
int spacewidth;
int prefix_offset;
/*
typedef struct { 
    UINT cbSize; 
    int  iTabLength; 
    int  iLeftMargin; 
    int  iRightMargin; 
    UINT uiLengthDrawn; 
} DRAWTEXTPARAMS, FAR *LPDRAWTEXTPARAMS; 
*/

int TEXT_DrawTextEx(HDC hDC,void *strPtr,int nCount,LPRECT lpRect,UINT uFormat,LPDRAWTEXTPARAMS dtp,WINBOOL Unicode )
{
    SIZE size;
    int line[1024];
    int len, lh, count=nCount;
    int prefix_x = 0;
    int prefix_end = 0;
    TEXTMETRIC tm;
    int x = lpRect->left, y = lpRect->top;
    int width = lpRect->right - lpRect->left;
    int max_width = 0;

    //TRACE(text,"%s, %d , [(%d,%d),(%d,%d)]\n",
	//  debugstr_an (lpString, count), count,
	//  lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
    
    if (count == -1) { 
	if ( Unicode )
		count = lstrlenW(strPtr);
	else
		count = lstrlenA(strPtr);
    }
    if ( Unicode )
    	GetTextMetricsW(hDC, &tm);
    else
    	GetTextMetricsA(hDC, &tm);

    if (uFormat & DT_EXTERNALLEADING)
	lh = tm.tmHeight + tm.tmExternalLeading;
    else
	lh = tm.tmHeight;

    if (uFormat & DT_TABSTOP)
	tabstop = uFormat >> 8;

   
    if (uFormat & DT_EXPANDTABS)
    {
		GetTextExtentPointA(hDC, " ", 1, &size);
		spacewidth = size.cx;
		if ( dtp->iTabLength == 0 ) {
			GetTextExtentPointA(hDC, "o", 1, &size);
			tabwidth = size.cx * tabstop;
		}
		else
			tabwidth = tabstop * dtp->iTabLength;
    	
    }

    if (uFormat & DT_CALCRECT) uFormat |= DT_NOCLIP;

    do
    {
	prefix_offset = -1;
	if ( Unicode )
		strPtr = TEXT_NextLineW(hDC, strPtr, &count, (LPWSTR)line, &len, width, uFormat);
	else
		strPtr = TEXT_NextLineA(hDC, strPtr, &count, (LPSTR)line, &len, width, uFormat);

	if ( Unicode ) {
		if (prefix_offset != -1)
		{
	    		GetTextExtentPointW(hDC, (LPWSTR)line, prefix_offset, &size);
	    		prefix_x = size.cx;
	    		GetTextExtentPointW(hDC, (LPWSTR)line, prefix_offset + 1, &size);
	    		prefix_end = size.cx - 1;
		}
		if (!GetTextExtentPointW(hDC, (LPWSTR)line, len, &size)) 
			return 0;
	}
	else {
		if (prefix_offset != -1)
		{
	    		GetTextExtentPointA(hDC, (LPSTR)line, prefix_offset, &size);
	    		prefix_x = size.cx;
	    		GetTextExtentPointA(hDC, (LPSTR)line, prefix_offset + 1, &size);
	    		prefix_end = size.cx - 1;
		}
		if (!GetTextExtentPointA(hDC, (LPSTR)line, len, &size)) 
			return 0;
	}	

	

	if (uFormat & DT_CENTER) 
		x = (lpRect->left + lpRect->right - size.cx) / 2;
	else if (uFormat & DT_RIGHT) 
		x = lpRect->right - size.cx;

	if (uFormat & DT_SINGLELINE)
	{
	    if (uFormat & DT_VCENTER) y = lpRect->top + 
	    	(lpRect->bottom - lpRect->top) / 2 - size.cy / 2;
	    else if (uFormat & DT_BOTTOM) y = lpRect->bottom - size.cy;
	}
	if (!(uFormat & DT_CALCRECT))
	{
		if ( Unicode ) {
	    		if (!ExtTextOutW(hDC, x, y, (uFormat & DT_NOCLIP) ? 0 : ETO_CLIPPED,lpRect, (LPWSTR)line, len, NULL )) 
				return 0;
		}
		else {
			if (!ExtTextOutA(hDC, x, y, (uFormat & DT_NOCLIP) ? 0 : ETO_CLIPPED,lpRect, (LPSTR)line, len, NULL )) 
				return 0;
		}
            if (prefix_offset != -1)
            {
                HPEN hpen = CreatePen( PS_SOLID, 1, GetTextColor(hDC) );
                HPEN oldPen = SelectObject( hDC, hpen );
                MoveToEx(hDC, x + prefix_x, y + tm.tmAscent + 1,NULL );
                LineTo(hDC, x + prefix_end + 1, y + tm.tmAscent + 1 );
                SelectObject( hDC, oldPen );
                DeleteObject( hpen );
            }
	}
	else if (size.cx > max_width)
	    max_width = size.cx;

	y += lh;
	if (strPtr)
	{
	    if (!(uFormat & DT_NOCLIP))
	    {
		if (y > lpRect->bottom - lh)
		    break;
	    }
	}
    }
    while (strPtr);
    if (uFormat & DT_CALCRECT)
    {
	lpRect->right = lpRect->left + max_width;
	lpRect->bottom = y;
    }
    return y - lpRect->top;
}


LPCSTR TEXT_NextLineA( HDC hdc, LPCSTR str, INT *count,
                                  LPSTR dest, INT *len, INT width, WORD format)
{
    /* Return next line of text from a string.
     * 
     * hdc - handle to DC.
     * str - string to parse into lines.
     * count - length of str.
     * dest - destination in which to return line.
     * len - length of resultant line in dest in chars.
     * width - maximum width of line in pixels.
     * format - format type passed to DrawText.
     *
     * Returns pointer to next char in str after end of the line
     * or NULL if end of str reached.
     */

    int i = 0, j = 0, k;
    int plen = 0;
    int numspaces;
    SIZE size;
    int lasttab = 0;
    int wb_i = 0, wb_j = 0, wb_count = 0;

    while (*count)
    {
	switch (str[i])
	{
	case CR:
	case LF:
	    if (!(format & DT_SINGLELINE))
	    {
		if ((*count > 1) && (str[i] == CR) && (str[i+1] == LF))
                {
		    (*count)--;
                    i++;
                }
		i++;
		*len = j;
		(*count)--;
		return (&str[i]);
	    }
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		(format & DT_WORDBREAK))
	    {
		if (!GetTextExtentPointA(hdc, &dest[j-1], 1, &size))
		    return NULL;
		plen += size.cx;
	    }
	    break;
	    
	case PREFIX:
	    if (!(format & DT_NOPREFIX) && *count > 1)
                {
                if (str[++i] == PREFIX)
		    (*count)--;
		else {
                    prefix_offset = j;
                    break;
                }
	    }
            dest[j++] = str[i++];
            if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
                (format & DT_WORDBREAK))
            {
                if (!GetTextExtentPointA(hdc, &dest[j-1], 1, &size))
                    return NULL;
                plen += size.cx;
            }
	    break;
	    
	case TAB:
	    if (format & DT_EXPANDTABS)
	    {
		wb_i = ++i;
		wb_j = j;
		wb_count = *count;

		if (!GetTextExtentPointA(hdc, &dest[lasttab], j - lasttab,
					                         &size))
		    return NULL;

		numspaces = (tabwidth - size.cx) / spacewidth;
		for (k = 0; k < numspaces; k++)
		    dest[j++] = SPACE;
		plen += tabwidth - size.cx;
		lasttab = wb_j + numspaces;
	    }
	    else
	    {
		dest[j++] = str[i++];
		if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		    (format & DT_WORDBREAK))
		{
		    if (!GetTextExtentPointA(hdc, &dest[j-1], 1, &size))
			return NULL;
		    plen += size.cx;
		}
	    }
	    break;

	case SPACE:
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		(format & DT_WORDBREAK))
	    {
		wb_i = i;
		wb_j = j - 1;
		wb_count = *count;
		if (!GetTextExtentPointA(hdc, &dest[j-1], 1, &size))
		    return NULL;
		plen += size.cx;
	    }
	    break;

	default:
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		(format & DT_WORDBREAK))
	    {
		if (!GetTextExtentPointA(hdc, &dest[j-1], 1, &size))
		    return NULL;
		plen += size.cx;
	    }
	}

	(*count)--;
	if (!(format & DT_NOCLIP) || (format & DT_WORDBREAK))
	{
	    if (plen > width)
	    {
		if (format & DT_WORDBREAK)
		{
		    if (wb_j)
		    {
			*len = wb_j;
			*count = wb_count - 1;
			return (&str[wb_i]);
		    }
		}
		else
		{
		    *len = j;
		    return (&str[i]);
		}
	    }
	}
    }
    
    *len = j;
    return NULL;
}

LPCWSTR TEXT_NextLineW( HDC hdc, LPCWSTR str, INT *count,
                                  LPWSTR dest, INT *len, INT width, WORD format)
{
    /* Return next line of text from a string.
     * 
     * hdc - handle to DC.
     * str - string to parse into lines.
     * count - length of str.
     * dest - destination in which to return line.
     * len - length of resultant line in dest in chars.
     * width - maximum width of line in pixels.
     * format - format type passed to DrawText.
     *
     * Returns pointer to next char in str after end of the line
     * or NULL if end of str reached.
     */

    int i = 0, j = 0, k;
    int plen = 0;
    int numspaces;
    SIZE size;
    int lasttab = 0;
    int wb_i = 0, wb_j = 0, wb_count = 0;

    while (*count)
    {
	switch (str[i])
	{
	case CR:
	case LF:
	    if (!(format & DT_SINGLELINE))
	    {
		if ((*count > 1) && (str[i] == CR) && (str[i+1] == LF))
                {
		    (*count)--;
                    i++;
                }
		i++;
		*len = j;
		(*count)--;
		return (&str[i]);
	    }
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		(format & DT_WORDBREAK))
	    {
		if (!GetTextExtentPointW(hdc, &dest[j-1], 1, &size))
		    return NULL;
		plen += size.cx;
	    }
	    break;
	    
	case PREFIX:
	    if (!(format & DT_NOPREFIX) && *count > 1)
                {
                if (str[++i] == PREFIX)
		    (*count)--;
		else {
                    prefix_offset = j;
                    break;
                }
	    }
            dest[j++] = str[i++];
            if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
                (format & DT_WORDBREAK))
            {
                if (!GetTextExtentPointW(hdc, &dest[j-1], 1, &size))
                    return NULL;
                plen += size.cx;
            }
	    break;
	    
	case TAB:
	    if (format & DT_EXPANDTABS)
	    {
		wb_i = ++i;
		wb_j = j;
		wb_count = *count;

		if (!GetTextExtentPointW(hdc, &dest[lasttab], j - lasttab,
					                         &size))
		    return NULL;

		numspaces = (tabwidth - size.cx) / spacewidth;
		for (k = 0; k < numspaces; k++)
		    dest[j++] = SPACE;
		plen += tabwidth - size.cx;
		lasttab = wb_j + numspaces;
	    }
	    else
	    {
		dest[j++] = str[i++];
		if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		    (format & DT_WORDBREAK))
		{
		    if (!GetTextExtentPointW(hdc, &dest[j-1], 1, &size))
			return NULL;
		    plen += size.cx;
		}
	    }
	    break;

	case SPACE:
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		(format & DT_WORDBREAK))
	    {
		wb_i = i;
		wb_j = j - 1;
		wb_count = *count;
		if (!GetTextExtentPointW(hdc, &dest[j-1], 1, &size))
		    return NULL;
		plen += size.cx;
	    }
	    break;

	default:
	    dest[j++] = str[i++];
	    if (!(format & DT_NOCLIP) || !(format & DT_NOPREFIX) ||
		(format & DT_WORDBREAK))
	    {
		if (!GetTextExtentPointW(hdc, &dest[j-1], 1, &size))
		    return NULL;
		plen += size.cx;
	    }
	}

	(*count)--;
	if (!(format & DT_NOCLIP) || (format & DT_WORDBREAK))
	{
	    if (plen > width)
	    {
		if (format & DT_WORDBREAK)
		{
		    if (wb_j)
		    {
			*len = wb_j;
			*count = wb_count - 1;
			return (&str[wb_i]);
		    }
		}
		else
		{
		    *len = j;
		    return (&str[i]);
		}
	    }
	}
    }
    
    *len = j;
    return NULL;
}

#if 0


WINBOOL TEXT_GrayString(HDC hdc, HBRUSH hb, 
                              GRAYSTRINGPROC fn, LPARAM lp, INT len,
                              INT x, INT y, INT cx, INT cy, WINBOOL Unicode)
{
    HBITMAP hbm, hbmsave;
    HBRUSH hbsave;
    HFONT hfsave;
    HDC memdc = CreateCompatibleDC(hdc);
    int slen = len;
    WINBOOL retval = TRUE;
    RECT r;
    COLORREF fg, bg;

    if(!hdc) return FALSE;
    
    if(len == 0)
    {
        if ( Unicode )
    		slen = lstrlenW((LPCWSTR)lp);
	else
		slen = lstrlenA((LPCSTR)lp);
    
    }

    if((cx == 0 || cy == 0) && slen != -1)
    {
        SIZE s;
        if ( Unicode )
        	GetTextExtentPointW(hdc, (LPCWSTR)lp, slen, &s);
	else
		GetTextExtentPointA(hdc, (LPCSTR)lp, slen, &s);
     
        if(cx == 0) cx = s.cx;
        if(cy == 0) cy = s.cy;
    }

    r.left = r.top = 0;
    r.right = cx;
    r.bottom = cy;

    hbm = CreateBitmap(cx, cy, 1, 1, NULL);
    hbmsave = (HBITMAP)SelectObject(memdc, hbm);
    FillRect(memdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SetTextColor(memdc, RGB(255, 255, 255));
    SetBkColor(memdc, RGB(0, 0, 0));
    hfsave = (HFONT)SelectObject(memdc, GetCurrentObject(hdc, OBJ_FONT));
            
    if(fn)
            retval = fn(memdc, lp, slen);
    else { 
	if (Unicode )
            TextOutW(memdc, 0, 0, (LPCWSTR)lp, slen);
	else
	    TextOutA(memdc, 0, 0, (LPCSTR)lp, slen);
    }
  

    SelectObject(memdc, hfsave);

/*
 * Windows doc says that the bitmap isn't grayed when len == -1 and
 * the callback function returns FALSE. However, testing this on
 * win95 showed otherwise...
*/
#ifdef GRAYSTRING_USING_DOCUMENTED_BEHAVIOUR
    if(retval || len != -1)
#endif
    {
        hbsave = (HBRUSH)SelectObject(memdc, CACHE_GetPattern55AABrush());
        PatBlt(memdc, 0, 0, cx, cy, 0x000A0329);
        SelectObject(memdc, hbsave);
    }

    if(hb) hbsave = (HBRUSH)SelectObject(hdc, hb);
    fg = SetTextColor(hdc, RGB(0, 0, 0));
    bg = SetBkColor(hdc, RGB(255, 255, 255));
    BitBlt(hdc, x, y, cx, cy, memdc, 0, 0, 0x00E20746);
    SetTextColor(hdc, fg);
    SetBkColor(hdc, bg);
    if(hb) SelectObject(hdc, hbsave);

    SelectObject(memdc, hbmsave);
    DeleteObject(hbm);
    DeleteDC(memdc);
    return retval;
}


#endif

/***********************************************************************
 *           TEXT_TabbedTextOut
 *
 * Helper function for TabbedTextOut() and GetTabbedTextExtent().
 * Note: this doesn't work too well for text-alignment modes other
 *       than TA_LEFT|TA_TOP. But we want bug-for-bug compatibility :-)
 */
LONG TEXT_TabbedTextOutA( HDC hdc, INT x, INT y, LPCSTR lpstr,
                         INT count, INT cTabStops, const INT *lpTabPos,
                         INT nTabOrg, WINBOOL fDisplayText )
{
    INT defWidth;
    DWORD extent = 0;
    int i, tabPos = x;
    int start = x;
    SIZE szSize;

    if (cTabStops == 1)
    {
        defWidth = lpTabPos ? *lpTabPos : 0;
        cTabStops = 0;
    }
    else
    {
        TEXTMETRIC tm;
        GetTextMetricsA( hdc, &tm );
        defWidth = 8 * tm.tmAveCharWidth;
    }
    
    while (count > 0)
    {
        for (i = 0; i < count; i++)
            if (lpstr[i] == '\t') break;
        GetTextExtentPoint32A( hdc, lpstr, i, &szSize );
        extent = szSize.cx;
        if (lpTabPos)
        {
            while ((cTabStops > 0) &&
                   (nTabOrg + *lpTabPos <= x + LOWORD(extent)))
            {
                lpTabPos++;
                cTabStops--;
            }
        }
        else
        {
            while ((cTabStops > 0) &&
                   (nTabOrg + *lpTabPos <= x + LOWORD(extent)))
            {
                lpTabPos++;
                cTabStops--;
            }
        }
        if (i == count)
            tabPos = x + LOWORD(extent);
        else if (cTabStops > 0)
            tabPos = nTabOrg + (lpTabPos ? *lpTabPos : 0);
        else
            tabPos = nTabOrg + ((x + LOWORD(extent) - nTabOrg) / defWidth + 1) * defWidth;
        if (fDisplayText)
        {
            RECT r;
            SetRect( &r, x, y, tabPos, y+HIWORD(extent) );
            ExtTextOutA( hdc, x, y,
                           GetBkMode(hdc) == OPAQUE ? ETO_OPAQUE : 0,
                           &r, lpstr, i, NULL );
        }
        x = tabPos;
        count -= i+1;
        lpstr += i+1;
    }
    return MAKELONG(tabPos - start, HIWORD(extent));
}

/***********************************************************************
 *           TEXT_TabbedTextOut
 *
 * Helper function for TabbedTextOut() and GetTabbedTextExtent().
 * Note: this doesn't work too well for text-alignment modes other
 *       than TA_LEFT|TA_TOP. But we want bug-for-bug compatibility :-)
 */
LONG TEXT_TabbedTextOutW( HDC hdc, INT x, INT y, LPCWSTR lpstr,
                         INT count, INT cTabStops, const INT *lpTabPos,
                         INT nTabOrg, WINBOOL fDisplayText )
{
    INT defWidth;
    DWORD extent = 0;
    int i, tabPos = x;
    int start = x;
    SIZE szSize;

    if (cTabStops == 1)
    {
        defWidth = lpTabPos ? *lpTabPos : 0;
        cTabStops = 0;
    }
    else
    {
        TEXTMETRIC tm;
        GetTextMetricsW( hdc, &tm );
        defWidth = 8 * tm.tmAveCharWidth;
    }
    
    while (count > 0)
    {
        for (i = 0; i < count; i++)
            if (lpstr[i] == '\t') break;
        GetTextExtentPoint32W( hdc, lpstr, i, &szSize );
        extent = szSize.cx;
        if (lpTabPos)
        {
            while ((cTabStops > 0) &&
                   (nTabOrg + *lpTabPos <= x + LOWORD(extent)))
            {
                lpTabPos++;
                cTabStops--;
            }
        }
        else
        {
            while ((cTabStops > 0) &&
                   (nTabOrg + *lpTabPos <= x + LOWORD(extent)))
            {
                lpTabPos++;
                cTabStops--;
            }
        }
        if (i == count)
            tabPos = x + LOWORD(extent);
        else if (cTabStops > 0)
            tabPos = nTabOrg + (lpTabPos ? *lpTabPos : 0);
        else
            tabPos = nTabOrg + ((x + LOWORD(extent) - nTabOrg) / defWidth + 1) * defWidth;
        if (fDisplayText)
        {
            RECT r;
            SetRect( &r, x, y, tabPos, y+HIWORD(extent) );
            ExtTextOutW( hdc, x, y,
                           GetBkMode(hdc) == OPAQUE ? ETO_OPAQUE : 0,
                           &r, lpstr, i, NULL );
        }
        x = tabPos;
        count -= i+1;
        lpstr += i+1;
    }
    return MAKELONG(tabPos - start, HIWORD(extent));
}
#if 0
int _alloca(int x)
{
	return malloc(x);
}
#endif