/*
 * Theming - Combo box control
 *
 * Copyright (c) 2005 by Frank Richter
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
 *
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "uxtheme.h"
#include "tmschema.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(themingcombo);

/* Subclass-private state flags */
#define STATE_NOREDRAW         1
#define STATE_HOT              2

/* some constants for metrics, same as in user32 */
#define COMBO_XBORDERSIZE      2
#define COMBO_YBORDERSIZE      2
#define COMBO_EDITBUTTONSPACE  0
#define EDIT_CONTROL_PADDING   1

/* paint text of combobox, needed for read-only drop downs. */
static void paint_text (HWND hwnd, HDC hdc, DWORD dwStyle, const COMBOBOXINFO *cbi)
{
    INT  id, size = 0;
    LPWSTR pText = NULL;
    UINT itemState = ODS_COMBOBOXEDIT;
    HFONT font = (HFONT)SendMessageW (hwnd, WM_GETFONT, 0, 0);
    HFONT hPrevFont = (font) ? SelectObject(hdc, font) : 0;
    RECT rectEdit;
    BOOL focused = GetFocus () == hwnd;
    BOOL dropped = cbi->stateButton == STATE_SYSTEM_PRESSED;

    TRACE("\n");

    /* follow Windows combobox that sends a bunch of text
     * inquiries to its listbox while processing WM_PAINT. */

    if( (id = SendMessageW (cbi->hwndList, LB_GETCURSEL, 0, 0) ) != LB_ERR )
    {
        size = SendMessageW (cbi->hwndList, LB_GETTEXTLEN, id, 0);
        if (size == LB_ERR)
          FIXME("LB_ERR probably not handled yet\n");
        if( (pText = HeapAlloc( GetProcessHeap(), 0, (size + 1) * sizeof(WCHAR))) )
        {
            /* size from LB_GETTEXTLEN may be too large, from LB_GETTEXT is accurate */
            size=SendMessageW (cbi->hwndList, LB_GETTEXT, (WPARAM)id, (LPARAM)pText);
            pText[size] = '\0'; /* just in case */
        } else return;
    }
    else
       if( !(dwStyle & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) )
           return;

    /*
     * Give ourselves some space.
     */
    CopyRect (&rectEdit, &cbi->rcItem);
    InflateRect( &rectEdit, -1, -1 );
     
    if(dwStyle & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))
    {
       DRAWITEMSTRUCT dis;
       HRGN           clipRegion;
       UINT ctlid = (UINT)GetWindowLongPtrW( hwnd, GWLP_ID );

       /* setup state for DRAWITEM message. Owner will highlight */
       if ( focused && !dropped )
           itemState |= ODS_SELECTED | ODS_FOCUS;

       /*
        * Save the current clip region.
        * To retrieve the clip region, we need to create one "dummy"
        * clip region.
        */
       clipRegion = CreateRectRgnIndirect(&rectEdit);

       if (GetClipRgn(hdc, clipRegion)!=1)
       {
         DeleteObject(clipRegion);
         clipRegion=NULL;
       }

       if (!IsWindowEnabled(hwnd)) itemState |= ODS_DISABLED;

       dis.CtlType      = ODT_COMBOBOX;
       dis.CtlID        = ctlid;
       dis.hwndItem     = hwnd;
       dis.itemAction   = ODA_DRAWENTIRE;
       dis.itemID       = id;
       dis.itemState    = itemState;
       dis.hDC          = hdc;
       dis.rcItem       = rectEdit;
       dis.itemData     = SendMessageW(cbi->hwndList, LB_GETITEMDATA,
                                        (WPARAM)id, 0 );

       /*
        * Clip the DC and have the parent draw the item.
        */
       IntersectClipRect(hdc,
                         rectEdit.left,  rectEdit.top,
                         rectEdit.right, rectEdit.bottom);

       SendMessageW(GetParent (hwnd), WM_DRAWITEM, ctlid, (LPARAM)&dis );

       /*
        * Reset the clipping region.
        */
       SelectClipRgn(hdc, clipRegion);
    }
    else
    {
       static const WCHAR empty_stringW[] = { 0 };

       if ( focused && !dropped ) {

           /* highlight */
           FillRect( hdc, &rectEdit, GetSysColorBrush(COLOR_HIGHLIGHT) );
           SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
           SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
      }

      ExtTextOutW( hdc,
                   rectEdit.left + 1,
                   rectEdit.top + 1,
                   ETO_OPAQUE | ETO_CLIPPED,
                   &rectEdit,
                   pText ? pText : empty_stringW , size, NULL );

      if ( focused && !dropped )
        DrawFocusRect( hdc, &rectEdit );
    }

    if( hPrevFont )
      SelectObject(hdc, hPrevFont );
   
    HeapFree( GetProcessHeap(), 0, pText );
}

/* paint the combobox */
static LRESULT paint (HTHEME theme, HWND hwnd, HDC hParamDC, ULONG state)
{
  PAINTSTRUCT ps;
  HDC   hDC;
  COMBOBOXINFO cbi;
  DWORD dwStyle = GetWindowLongW (hwnd, GWL_STYLE);

  hDC = (hParamDC) ? hParamDC
                   : BeginPaint( hwnd, &ps);

  TRACE("hdc=%p\n", hDC);

  if( hDC && !(state & STATE_NOREDRAW) )
  {
      RECT frameRect;
      int buttonState;
  
      cbi.cbSize = sizeof (cbi);
      SendMessageW (hwnd, CB_GETCOMBOBOXINFO, 0, (LPARAM)&cbi);
      
      /* paint border */
      if ((dwStyle & CBS_DROPDOWNLIST) != CBS_SIMPLE)
          GetClientRect (hwnd, &frameRect);
      else
      {
          CopyRect (&frameRect, &cbi.rcItem);

          InflateRect(&frameRect, 
              EDIT_CONTROL_PADDING + COMBO_XBORDERSIZE, 
              EDIT_CONTROL_PADDING + COMBO_YBORDERSIZE);
      }
      
      DrawThemeBackground (theme, hDC, 0, 
          IsWindowEnabled (hwnd) ? CBXS_NORMAL : CBXS_DISABLED, &frameRect, NULL);
      
      /* paint button */
      if (cbi.stateButton != STATE_SYSTEM_INVISIBLE)
      {
          if (!IsWindowEnabled (hwnd))
              buttonState = CBXS_DISABLED;
          else if (cbi.stateButton == STATE_SYSTEM_PRESSED)
              buttonState = CBXS_PRESSED;
          else if (state & STATE_HOT)
              buttonState = CBXS_HOT;
          else
              buttonState = CBXS_NORMAL;
          DrawThemeBackground (theme, hDC, CP_DROPDOWNBUTTON, buttonState, 
              &cbi.rcButton, NULL);
      }

      /* paint text, if we need to */
      if ((dwStyle & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST)
          paint_text (hwnd, hDC, dwStyle, &cbi);
  }

  if( !hParamDC )
    EndPaint(hwnd, &ps);

  return 0;
}


/**********************************************************************
 * The combo control subclass window proc.
 */
LRESULT CALLBACK THEMING_ComboSubclassProc (HWND hwnd, UINT msg, 
                                            WPARAM wParam, LPARAM lParam, 
                                            ULONG_PTR dwRefData)
{
    const WCHAR* themeClass = WC_COMBOBOXW;
    HTHEME theme;
    LRESULT result;
      
    switch (msg)
    {
    case WM_CREATE:
        result = THEMING_CallOriginalClass  (hwnd, msg, wParam, lParam);
        OpenThemeData( hwnd, themeClass );
        return result;
    
    case WM_DESTROY:
        theme = GetWindowTheme( hwnd );
        CloseThemeData ( theme );
        return THEMING_CallOriginalClass (hwnd, msg, wParam, lParam);

    case WM_THEMECHANGED:
        theme = GetWindowTheme( hwnd );
        CloseThemeData ( theme );
        OpenThemeData( hwnd, themeClass );
        break;
        
    case WM_SYSCOLORCHANGE:
        theme = GetWindowTheme( hwnd );
	if (!theme) return THEMING_CallOriginalClass (hwnd, msg, wParam, lParam);
        /* Do nothing. When themed, a WM_THEMECHANGED will be received, too,
   	 * which will do the repaint. */
        break;
        
    case WM_PAINT:
        theme = GetWindowTheme( hwnd );
        if (!theme) return THEMING_CallOriginalClass (hwnd, msg, wParam, lParam);
        return paint (theme, hwnd, (HDC)wParam, dwRefData);

    case WM_SETREDRAW:
        /* Since there doesn't seem to be WM_GETREDRAW, do redraw tracking in 
         * the subclass as well. */
        if( wParam )
            dwRefData &= ~STATE_NOREDRAW;
        else
            dwRefData |= STATE_NOREDRAW;
        THEMING_SetSubclassData (hwnd, dwRefData);
        return THEMING_CallOriginalClass (hwnd, msg, wParam, lParam);
        
    case WM_MOUSEMOVE:
        {
            /* Dropdown button hot-tracking */
            COMBOBOXINFO cbi;
            POINT pt;

            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            cbi.cbSize = sizeof (cbi);
            SendMessageW (hwnd, CB_GETCOMBOBOXINFO, 0, (LPARAM)&cbi);
            
            if (cbi.stateButton != STATE_SYSTEM_INVISIBLE)
            {
                if (PtInRect (&cbi.rcButton, pt))
                {
                    if (!(dwRefData & STATE_HOT))
                    {
                        dwRefData |= STATE_HOT;
                        THEMING_SetSubclassData (hwnd, dwRefData);
                        RedrawWindow (hwnd, &cbi.rcButton, 0, 
                            RDW_INVALIDATE | RDW_UPDATENOW);
                    }
                }
                else
                {
                    if (dwRefData & STATE_HOT)
                    {
                        dwRefData &= ~STATE_HOT;
                        THEMING_SetSubclassData (hwnd, dwRefData);
                        RedrawWindow (hwnd, &cbi.rcButton, 0, 
                            RDW_INVALIDATE | RDW_UPDATENOW);
                    }
                }
            }
        }
        return THEMING_CallOriginalClass (hwnd, msg, wParam, lParam);
    
    default: 
	/* Call old proc */
	return THEMING_CallOriginalClass (hwnd, msg, wParam, lParam);
    }
    return 0;
}
