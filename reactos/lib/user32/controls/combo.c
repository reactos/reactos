/*
 * Combo controls
 *
 * Copyright 1997 Alex Korobka
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
 *
 * FIXME: roll up in Netscape 3.01.
 */

#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <user32.h>
#include <debug.h>

#include "user32/regcontrol.h"
#include "controls.h"

//WINE_DEFAULT_DEBUG_CHANNEL(combo);

  /* bits in the dwKeyData */
#define KEYDATA_ALT             0x2000
#define KEYDATA_PREVSTATE       0x4000

/*
 * Additional combo box definitions
 */

#define CB_NOTIFY( lphc, code ) \
    (SendMessageW((lphc)->owner, WM_COMMAND, \
                  MAKEWPARAM(GetWindowLongA((lphc)->self,GWL_ID), (code)), (LPARAM)(lphc)->self))

#define CB_DISABLED( lphc )   (!IsWindowEnabled((lphc)->self))
#define CB_OWNERDRAWN( lphc ) ((lphc)->dwStyle & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))
#define CB_HASSTRINGS( lphc ) ((lphc)->dwStyle & CBS_HASSTRINGS)
#define CB_HWND( lphc )       ((lphc)->self)

#define ISWIN31 (LOWORD(GetVersion()) == 0x0a03)

/*
 * Drawing globals
 */
static HBITMAP 	hComboBmp = 0;
static UINT	CBitHeight, CBitWidth;

/*
 * Look and feel dependant "constants"
 */

#define COMBO_YBORDERGAP         5
#define COMBO_XBORDERSIZE()      ( 2 )
#define COMBO_YBORDERSIZE()      ( 2 )
#define COMBO_EDITBUTTONSPACE()  ( 0 )
#define EDIT_CONTROL_PADDING()   ( 1 )

static LRESULT WINAPI ComboWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI ComboWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

/*********************************************************************
 * combo class descriptor
 */
const struct builtin_class_descr COMBO_builtin_class =
{
    "ComboBox",           /* name */
    CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS, /* style  */
    (WNDPROC) ComboWndProcA,        /* procA */
    (WNDPROC) ComboWndProcW,        /* procW */
    sizeof(HEADCOMBO *),  /* extra */
    (LPCSTR) IDC_ARROW,           /* cursor */
    0                     /* brush */
};


/***********************************************************************
 *           COMBO_Init
 *
 * Load combo button bitmap.
 */
static BOOL COMBO_Init()
{
  HDC		hDC;

  if( hComboBmp ) return TRUE;
  if( (hDC = CreateCompatibleDC(0)) )
  {
    BOOL	bRet = FALSE;
    if( (hComboBmp = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_COMBO))) )
    {
      BITMAP      bm;
      HBITMAP     hPrevB;
      RECT        r;

      GetObjectW( hComboBmp, sizeof(bm), &bm );
      CBitHeight = bm.bmHeight;
      CBitWidth  = bm.bmWidth;

      DbgPrint("combo bitmap [%i,%i]\n", CBitWidth, CBitHeight );

      hPrevB = SelectObject( hDC, hComboBmp);
      SetRect( &r, 0, 0, CBitWidth, CBitHeight );
      InvertRect( hDC, &r );
      SelectObject( hDC, hPrevB );
      bRet = TRUE;
    }
    DeleteDC( hDC );
    return bRet;
  }
  return FALSE;
}

/***********************************************************************
 *           COMBO_NCCreate
 */
static LRESULT COMBO_NCCreate(HWND hwnd, LONG style)
{
    LPHEADCOMBO lphc;

    if (COMBO_Init() && (lphc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HEADCOMBO))) )
    {
        lphc->self = hwnd;
        SetWindowLongA( hwnd, 0, (LONG)lphc );

       /* some braindead apps do try to use scrollbar/border flags */

	lphc->dwStyle = style & ~(WS_BORDER | WS_HSCROLL | WS_VSCROLL);
        SetWindowLongA( hwnd, GWL_STYLE, style & ~(WS_BORDER | WS_HSCROLL | WS_VSCROLL) );

	/*
	 * We also have to remove the client edge style to make sure
	 * we don't end-up with a non client area.
	 */
        SetWindowLongA( hwnd, GWL_EXSTYLE,
                        GetWindowLongA( hwnd, GWL_EXSTYLE ) & ~WS_EX_CLIENTEDGE );

	if( !(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) )
              lphc->dwStyle |= CBS_HASSTRINGS;
	if( !(GetWindowLongA( hwnd, GWL_EXSTYLE ) & WS_EX_NOPARENTNOTIFY) )
	      lphc->wState |= CBF_NOTIFY;

        DbgPrint("[%p], style = %08x\n", lphc, lphc->dwStyle );
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           COMBO_NCDestroy
 */
static LRESULT COMBO_NCDestroy( LPHEADCOMBO lphc )
{

   if( lphc )
   {
       DbgPrint("[%p]: freeing storage\n", lphc->self);

       if( (CB_GETTYPE(lphc) != CBS_SIMPLE) && lphc->hWndLBox )
   	   DestroyWindow( lphc->hWndLBox );

       SetWindowLongA( lphc->self, 0, 0 );
       HeapFree( GetProcessHeap(), 0, lphc );
   }
   return 0;
}

/***********************************************************************
 *           CBGetTextAreaHeight
 *
 * This method will calculate the height of the text area of the
 * combobox.
 * The height of the text area is set in two ways.
 * It can be set explicitly through a combobox message or through a
 * WM_MEASUREITEM callback.
 * If this is not the case, the height is set to 13 dialog units.
 * This height was determined through experimentation.
 */
static INT CBGetTextAreaHeight(
  HWND        hwnd,
  LPHEADCOMBO lphc)
{
  INT iTextItemHeight;

  if( lphc->editHeight ) /* explicitly set height */
  {
    iTextItemHeight = lphc->editHeight;
  }
  else
  {
    TEXTMETRICW tm;
    HDC         hDC       = GetDC(hwnd);
    HFONT       hPrevFont = 0;
    INT         baseUnitY;

    if (lphc->hFont)
      hPrevFont = SelectObject( hDC, lphc->hFont );

    GetTextMetricsW(hDC, &tm);

    baseUnitY = tm.tmHeight;

    if( hPrevFont )
      SelectObject( hDC, hPrevFont );

    ReleaseDC(hwnd, hDC);

    iTextItemHeight = ((13 * baseUnitY) / 8);

    /*
     * This "formula" calculates the height of the complete control.
     * To calculate the height of the text area, we have to remove the
     * borders.
     */
    iTextItemHeight -= 2*COMBO_YBORDERSIZE();
  }

  /*
   * Check the ownerdraw case if we haven't asked the parent the size
   * of the item yet.
   */
  if ( CB_OWNERDRAWN(lphc) &&
       (lphc->wState & CBF_MEASUREITEM) )
  {
    MEASUREITEMSTRUCT measureItem;
    RECT              clientRect;
    INT               originalItemHeight = iTextItemHeight;
    UINT id = GetWindowLongA( lphc->self, GWL_ID );

    /*
     * We use the client rect for the width of the item.
     */
    GetClientRect(hwnd, &clientRect);

    lphc->wState &= ~CBF_MEASUREITEM;

    /*
     * Send a first one to measure the size of the text area
     */
    measureItem.CtlType    = ODT_COMBOBOX;
    measureItem.CtlID      = id;
    measureItem.itemID     = -1;
    measureItem.itemWidth  = clientRect.right;
    measureItem.itemHeight = iTextItemHeight - 6; /* ownerdrawn cb is taller */
    measureItem.itemData   = 0;
    SendMessageW(lphc->owner, WM_MEASUREITEM, id, (LPARAM)&measureItem);
    iTextItemHeight = 6 + measureItem.itemHeight;

    /*
     * Send a second one in the case of a fixed ownerdraw list to calculate the
     * size of the list items. (we basically do this on behalf of the listbox)
     */
    if (lphc->dwStyle & CBS_OWNERDRAWFIXED)
    {
      measureItem.CtlType    = ODT_COMBOBOX;
      measureItem.CtlID      = id;
      measureItem.itemID     = 0;
      measureItem.itemWidth  = clientRect.right;
      measureItem.itemHeight = originalItemHeight;
      measureItem.itemData   = 0;
      SendMessageW(lphc->owner, WM_MEASUREITEM, id, (LPARAM)&measureItem);
      lphc->fixedOwnerDrawHeight = measureItem.itemHeight;
    }

    /*
     * Keep the size for the next time
     */
    lphc->editHeight = iTextItemHeight;
  }

  return iTextItemHeight;
}

/***********************************************************************
 *           CBForceDummyResize
 *
 * The dummy resize is used for listboxes that have a popup to trigger
 * a re-arranging of the contents of the combobox and the recalculation
 * of the size of the "real" control window.
 */
static void CBForceDummyResize(
  LPHEADCOMBO lphc)
{
  RECT windowRect;
  int newComboHeight;

  newComboHeight = CBGetTextAreaHeight(lphc->self,lphc) + 2*COMBO_YBORDERSIZE();

  GetWindowRect(lphc->self, &windowRect);

  /*
   * We have to be careful, resizing a combobox also has the meaning that the
   * dropped rect will be resized. In this case, we want to trigger a resize
   * to recalculate layout but we don't want to change the dropped rectangle
   * So, we pass the height of text area of control as the height.
   * this will cancel-out in the processing of the WM_WINDOWPOSCHANGING
   * message.
   */
  SetWindowPos( lphc->self,
		NULL,
		0, 0,
		windowRect.right  - windowRect.left,
		newComboHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
}

/***********************************************************************
 *           CBCalcPlacement
 *
 * Set up component coordinates given valid lphc->RectCombo.
 */
static void CBCalcPlacement(
  HWND        hwnd,
  LPHEADCOMBO lphc,
  LPRECT      lprEdit,
  LPRECT      lprButton,
  LPRECT      lprLB)
{
  /*
   * Again, start with the client rectangle.
   */
  GetClientRect(hwnd, lprEdit);

  /*
   * Remove the borders
   */
  InflateRect(lprEdit, -COMBO_XBORDERSIZE(), -COMBO_YBORDERSIZE());

  /*
   * Chop off the bottom part to fit with the height of the text area.
   */
  lprEdit->bottom = lprEdit->top + CBGetTextAreaHeight(hwnd, lphc);

  /*
   * The button starts the same vertical position as the text area.
   */
  CopyRect(lprButton, lprEdit);

  /*
   * If the combobox is "simple" there is no button.
   */
  if( CB_GETTYPE(lphc) == CBS_SIMPLE )
    lprButton->left = lprButton->right = lprButton->bottom = 0;
  else
  {
    /*
     * Let's assume the combobox button is the same width as the
     * scrollbar button.
     * size the button horizontally and cut-off the text area.
     */
    lprButton->left = lprButton->right - GetSystemMetrics(SM_CXVSCROLL);
    lprEdit->right  = lprButton->left;
  }

  /*
   * In the case of a dropdown, there is an additional spacing between the
   * text area and the button.
   */
  if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
  {
    lprEdit->right -= COMBO_EDITBUTTONSPACE();
  }

  /*
   * If we have an edit control, we space it away from the borders slightly.
   */
  if (CB_GETTYPE(lphc) != CBS_DROPDOWNLIST)
  {
    InflateRect(lprEdit, -EDIT_CONTROL_PADDING(), -EDIT_CONTROL_PADDING());
  }

  /*
   * Adjust the size of the listbox popup.
   */
  if( CB_GETTYPE(lphc) == CBS_SIMPLE )
  {
    /*
     * Use the client rectangle to initialize the listbox rectangle
     */
    GetClientRect(hwnd, lprLB);

    /*
     * Then, chop-off the top part.
     */
    lprLB->top = lprEdit->bottom + COMBO_YBORDERSIZE();
  }
  else
  {
    /*
     * Make sure the dropped width is as large as the combobox itself.
     */
    if (lphc->droppedWidth < (lprButton->right + COMBO_XBORDERSIZE()))
    {
      lprLB->right  = lprLB->left + (lprButton->right + COMBO_XBORDERSIZE());

      /*
       * In the case of a dropdown, the popup listbox is offset to the right.
       * so, we want to make sure it's flush with the right side of the
       * combobox
       */
      if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
	lprLB->right -= COMBO_EDITBUTTONSPACE();
    }
    else
       lprLB->right = lprLB->left + lphc->droppedWidth;
  }

  DbgPrint("\ttext\t= (%ld,%ld-%ld,%ld)\n",
	lprEdit->left, lprEdit->top, lprEdit->right, lprEdit->bottom);

  DbgPrint("\tbutton\t= (%ld,%ld-%ld,%ld)\n",
	lprButton->left, lprButton->top, lprButton->right, lprButton->bottom);

  DbgPrint("\tlbox\t= (%ld,%ld-%ld,%ld)\n",
	lprLB->left, lprLB->top, lprLB->right, lprLB->bottom );
}

/***********************************************************************
 *           CBGetDroppedControlRect
 */
static void CBGetDroppedControlRect( LPHEADCOMBO lphc, LPRECT lpRect)
{
    /* In windows, CB_GETDROPPEDCONTROLRECT returns the upper left corner
     of the combo box and the lower right corner of the listbox */

    GetWindowRect(lphc->self, lpRect);

    lpRect->right =  lpRect->left + lphc->droppedRect.right - lphc->droppedRect.left;
    lpRect->bottom = lpRect->top + lphc->droppedRect.bottom - lphc->droppedRect.top;

}

/***********************************************************************
 *           COMBO_WindowPosChanging
 */
static LRESULT COMBO_WindowPosChanging(
  HWND        hwnd,
  LPHEADCOMBO lphc,
  WINDOWPOS*  posChanging)
{
  /*
   * We need to override the WM_WINDOWPOSCHANGING method to handle all
   * the non-simple comboboxes. The problem is that those controls are
   * always the same height. We have to make sure they are not resized
   * to another value.
   */
  if ( ( CB_GETTYPE(lphc) != CBS_SIMPLE ) &&
       ((posChanging->flags & SWP_NOSIZE) == 0) )
  {
    int newComboHeight;

    newComboHeight = CBGetTextAreaHeight(hwnd,lphc) +
                      2*COMBO_YBORDERSIZE();

    /*
     * Resizing a combobox has another side effect, it resizes the dropped
     * rectangle as well. However, it does it only if the new height for the
     * combobox is different from the height it should have. In other words,
     * if the application resizing the combobox only had the intention to resize
     * the actual control, for example, to do the layout of a dialog that is
     * resized, the height of the dropdown is not changed.
     */
    if (posChanging->cy != newComboHeight)
    {
	DbgPrint("posChanging->cy=%d, newComboHeight=%d, oldbot=%ld, oldtop=%ld\n",
	      posChanging->cy, newComboHeight, lphc->droppedRect.bottom,
	      lphc->droppedRect.top);
      lphc->droppedRect.bottom = lphc->droppedRect.top + posChanging->cy - newComboHeight;

      posChanging->cy = newComboHeight;
    }
  }

  return 0;
}

/***********************************************************************
 *           COMBO_Create
 */
static LRESULT COMBO_Create( HWND hwnd, LPHEADCOMBO lphc, HWND hwndParent, LONG style,
                             BOOL unicode )
{
  static const WCHAR clbName[] = {'C','o','m','b','o','L','B','o','x',0};
  static const WCHAR editName[] = {'E','d','i','t',0};

  if( !CB_GETTYPE(lphc) ) lphc->dwStyle |= CBS_SIMPLE;
  if( CB_GETTYPE(lphc) != CBS_DROPDOWNLIST ) lphc->wState |= CBF_EDIT;

  lphc->owner = hwndParent;

  /*
   * The item height and dropped width are not set when the control
   * is created.
   */
  lphc->droppedWidth = lphc->editHeight = 0;

  /*
   * The first time we go through, we want to measure the ownerdraw item
   */
  lphc->wState |= CBF_MEASUREITEM;

  /* M$ IE 3.01 actually creates (and rapidly destroys) an ownerless combobox */

  if( lphc->owner || !(style & WS_VISIBLE) )
  {
      UINT lbeStyle   = 0;
      UINT lbeExStyle = 0;

      /*
       * Initialize the dropped rect to the size of the client area of the
       * control and then, force all the areas of the combobox to be
       * recalculated.
       */
      GetClientRect( hwnd, &lphc->droppedRect );
      CBCalcPlacement(hwnd, lphc, &lphc->textRect, &lphc->buttonRect, &lphc->droppedRect );

      /*
       * Adjust the position of the popup listbox if it's necessary
       */
      if ( CB_GETTYPE(lphc) != CBS_SIMPLE )
      {
	lphc->droppedRect.top   = lphc->textRect.bottom + COMBO_YBORDERSIZE();

	/*
	 * If it's a dropdown, the listbox is offset
	 */
	if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
	  lphc->droppedRect.left += COMBO_EDITBUTTONSPACE();

	ClientToScreen(hwnd, (LPPOINT)&lphc->droppedRect);
	ClientToScreen(hwnd, (LPPOINT)&lphc->droppedRect.right);
      }

      /* create listbox popup */

      lbeStyle = (LBS_NOTIFY | WS_BORDER | WS_CLIPSIBLINGS | WS_CHILD) |
                 (style & (WS_VSCROLL | CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE));

      if( lphc->dwStyle & CBS_SORT )
	lbeStyle |= LBS_SORT;
      if( lphc->dwStyle & CBS_HASSTRINGS )
	lbeStyle |= LBS_HASSTRINGS;
      if( lphc->dwStyle & CBS_NOINTEGRALHEIGHT )
	lbeStyle |= LBS_NOINTEGRALHEIGHT;
      if( lphc->dwStyle & CBS_DISABLENOSCROLL )
	lbeStyle |= LBS_DISABLENOSCROLL;

      if( CB_GETTYPE(lphc) == CBS_SIMPLE ) 	/* child listbox */
      {
	lbeStyle |= WS_VISIBLE;

	/*
	 * In win 95 look n feel, the listbox in the simple combobox has
	 * the WS_EXCLIENTEDGE style instead of the WS_BORDER style.
	 */
	//if (TWEAK_WineLook > WIN31_LOOK)
	//{
	  lbeStyle   &= ~WS_BORDER;
	  lbeExStyle |= WS_EX_CLIENTEDGE;
	//}
      }

      if (unicode)
          lphc->hWndLBox = CreateWindowExW(lbeExStyle, clbName, NULL, lbeStyle,
                                           lphc->droppedRect.left,
                                           lphc->droppedRect.top,
                                           lphc->droppedRect.right - lphc->droppedRect.left,
                                           lphc->droppedRect.bottom - lphc->droppedRect.top,
                                           hwnd, (HMENU)ID_CB_LISTBOX,
                                           (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE ), lphc );
      else
          lphc->hWndLBox = CreateWindowExA(lbeExStyle, "ComboLBox", NULL, lbeStyle,
                                           lphc->droppedRect.left,
                                           lphc->droppedRect.top,
                                           lphc->droppedRect.right - lphc->droppedRect.left,
                                           lphc->droppedRect.bottom - lphc->droppedRect.top,
                                           hwnd, (HMENU)ID_CB_LISTBOX,
                                           (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE ), lphc );

      if( lphc->hWndLBox )
      {
	  BOOL	bEdit = TRUE;
	  lbeStyle = WS_CHILD | WS_VISIBLE | ES_NOHIDESEL | ES_LEFT | ES_COMBO;

	  /*
	   * In Win95 look, the border fo the edit control is
	   * provided by the combobox
	   */
	  //if (TWEAK_WineLook == WIN31_LOOK)
	  //  lbeStyle |= WS_BORDER;

	  if( lphc->wState & CBF_EDIT )
	  {
	      if( lphc->dwStyle & CBS_OEMCONVERT )
		  lbeStyle |= ES_OEMCONVERT;
	      if( lphc->dwStyle & CBS_AUTOHSCROLL )
		  lbeStyle |= ES_AUTOHSCROLL;
	      if( lphc->dwStyle & CBS_LOWERCASE )
		  lbeStyle |= ES_LOWERCASE;
	      else if( lphc->dwStyle & CBS_UPPERCASE )
		  lbeStyle |= ES_UPPERCASE;

              if (!IsWindowEnabled(hwnd)) lbeStyle |= WS_DISABLED;

              if (unicode)
                  lphc->hWndEdit = CreateWindowExW(0, editName, NULL, lbeStyle,
                                                   lphc->textRect.left, lphc->textRect.top,
                                                   lphc->textRect.right - lphc->textRect.left,
                                                   lphc->textRect.bottom - lphc->textRect.top,
                                                   hwnd, (HMENU)ID_CB_EDIT,
                                                   (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE ), NULL );
              else
                  lphc->hWndEdit = CreateWindowExA(0, "Edit", NULL, lbeStyle,
                                                   lphc->textRect.left, lphc->textRect.top,
                                                   lphc->textRect.right - lphc->textRect.left,
                                                   lphc->textRect.bottom - lphc->textRect.top,
                                                   hwnd, (HMENU)ID_CB_EDIT,
                                                   (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE ), NULL );

	      if( !lphc->hWndEdit )
		bEdit = FALSE;
	  }

          if( bEdit )
	  {
	    if( CB_GETTYPE(lphc) != CBS_SIMPLE )
	    {
              /* Now do the trick with parent */
	      SetParent(lphc->hWndLBox, HWND_DESKTOP);
              /*
               * If the combo is a dropdown, we must resize the control
	       * to fit only the text area and button. To do this,
	       * we send a dummy resize and the WM_WINDOWPOSCHANGING message
	       * will take care of setting the height for us.
               */
	      CBForceDummyResize(lphc);
	    }

	    OutputDebugStringA("init done\n");
	    return 0;
	  }
	  OutputDebugStringA("edit control failure.\n");
      } else OutputDebugStringA("listbox failure.\n");
  } else OutputDebugStringA("no owner for visible combo.\n");

  /* CreateWindow() will send WM_NCDESTROY to cleanup */

  return -1;
}

/***********************************************************************
 *           CBPaintButton
 *
 * Paint combo button (normal, pressed, and disabled states).
 */
static void CBPaintButton(
  LPHEADCOMBO lphc,
  HDC         hdc,
  RECT        rectButton)
{
    if( lphc->wState & CBF_NOREDRAW )
      return;

        UINT buttonState = DFCS_SCROLLCOMBOBOX;

	if (lphc->wState & CBF_BUTTONDOWN)
	{
	    buttonState |= DFCS_PUSHED;
	}

	if (CB_DISABLED(lphc))
	{
	  buttonState |= DFCS_INACTIVE;
	}

	DrawFrameControl(hdc,
			 &rectButton,
			 DFC_SCROLL,
			 buttonState);
}

/***********************************************************************
 *           CBPaintText
 *
 * Paint CBS_DROPDOWNLIST text field / update edit control contents.
 */
static void CBPaintText(
  LPHEADCOMBO lphc,
  HDC         hdc,
  RECT        rectEdit)
{
   INT	id, size = 0;
   LPWSTR pText = NULL;

   if( lphc->wState & CBF_NOREDRAW ) return;

   OutputDebugStringA("\n");

   /* follow Windows combobox that sends a bunch of text
    * inquiries to its listbox while processing WM_PAINT. */

   if( (id = SendMessageW(lphc->hWndLBox, LB_GETCURSEL, 0, 0) ) != LB_ERR )
   {
        size = SendMessageW(lphc->hWndLBox, LB_GETTEXTLEN, id, 0);
	if (size == LB_ERR)
	  FIXME("LB_ERR probably not handled yet\n");
        if( (pText = HeapAlloc( GetProcessHeap(), 0, (size + 1) * sizeof(WCHAR))) )
	{
            /* size from LB_GETTEXTLEN may be too large, from LB_GETTEXT is accurate */
	    size=SendMessageW(lphc->hWndLBox, LB_GETTEXT, (WPARAM)id, (LPARAM)pText);
	    pText[size] = '\0';	/* just in case */
	} else return;
   }
   else
       if( !CB_OWNERDRAWN(lphc) )
	   return;

   if( lphc->wState & CBF_EDIT )
   {
        static const WCHAR empty_stringW[] = { 0 };
	if( CB_HASSTRINGS(lphc) ) SetWindowTextW( lphc->hWndEdit, pText ? pText : empty_stringW );
	if( lphc->wState & CBF_FOCUSED )
	    SendMessageW(lphc->hWndEdit, EM_SETSEL, 0, (LPARAM)(-1));
   }
   else /* paint text field ourselves */
   {
     UINT	itemState = ODS_COMBOBOXEDIT;
     HFONT	hPrevFont = (lphc->hFont) ? SelectObject(hdc, lphc->hFont) : 0;

     /*
      * Give ourselves some space.
      */
     InflateRect( &rectEdit, -1, -1 );

     if( CB_OWNERDRAWN(lphc) )
     {
       DRAWITEMSTRUCT dis;
       HRGN           clipRegion;
       UINT ctlid = GetWindowLongA( lphc->self, GWL_ID );

       /* setup state for DRAWITEM message. Owner will highlight */
       if ( (lphc->wState & CBF_FOCUSED) &&
	    !(lphc->wState & CBF_DROPPED) )
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

       if (!IsWindowEnabled(lphc->self) & WS_DISABLED) itemState |= ODS_DISABLED;

       dis.CtlType	= ODT_COMBOBOX;
       dis.CtlID	= ctlid;
       dis.hwndItem	= lphc->self;
       dis.itemAction	= ODA_DRAWENTIRE;
       dis.itemID	= id;
       dis.itemState	= itemState;
       dis.hDC		= hdc;
       dis.rcItem	= rectEdit;
       dis.itemData	= SendMessageW(lphc->hWndLBox, LB_GETITEMDATA,
					(WPARAM)id, 0 );

       /*
	* Clip the DC and have the parent draw the item.
	*/
       IntersectClipRect(hdc,
			 rectEdit.left,  rectEdit.top,
			 rectEdit.right, rectEdit.bottom);

       SendMessageW(lphc->owner, WM_DRAWITEM, ctlid, (LPARAM)&dis );

       /*
	* Reset the clipping region.
	*/
       SelectClipRgn(hdc, clipRegion);
     }
     else
     {
       static const WCHAR empty_stringW[] = { 0 };

       if ( (lphc->wState & CBF_FOCUSED) &&
	    !(lphc->wState & CBF_DROPPED) ) {

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

       if(lphc->wState & CBF_FOCUSED && !(lphc->wState & CBF_DROPPED))
	 DrawFocusRect( hdc, &rectEdit );
     }

     if( hPrevFont )
       SelectObject(hdc, hPrevFont );
   }
   if (pText)
	HeapFree( GetProcessHeap(), 0, pText );
}

/***********************************************************************
 *           CBPaintBorder
 */
static void CBPaintBorder(
  HWND        hwnd,
  LPHEADCOMBO lphc,
  HDC         hdc)
{
  RECT clientRect;

  if (CB_GETTYPE(lphc) != CBS_SIMPLE)
  {
    GetClientRect(hwnd, &clientRect);
  }
  else
  {
    CopyRect(&clientRect, &lphc->textRect);

    InflateRect(&clientRect, EDIT_CONTROL_PADDING(), EDIT_CONTROL_PADDING());
    InflateRect(&clientRect, COMBO_XBORDERSIZE(), COMBO_YBORDERSIZE());
  }

  DrawEdge(hdc, &clientRect, EDGE_SUNKEN, BF_RECT);
}

/***********************************************************************
 *           COMBO_PrepareColors
 *
 * This method will sent the appropriate WM_CTLCOLOR message to
 * prepare and setup the colors for the combo's DC.
 *
 * It also returns the brush to use for the background.
 */
static HBRUSH COMBO_PrepareColors(
  LPHEADCOMBO lphc,
  HDC         hDC)
{
  HBRUSH  hBkgBrush;

  /*
   * Get the background brush for this control.
   */
  if (CB_DISABLED(lphc))
  {
    hBkgBrush = (HBRUSH)SendMessageW(lphc->owner, WM_CTLCOLORSTATIC,
				     (WPARAM)hDC, (LPARAM)lphc->self );

    /*
     * We have to change the text color since WM_CTLCOLORSTATIC will
     * set it to the "enabled" color. This is the same behavior as the
     * edit control
     */
    SetTextColor(hDC, GetSysColor(COLOR_GRAYTEXT));
  }
  else
  {
    if (lphc->wState & CBF_EDIT)
    {
      hBkgBrush = (HBRUSH)SendMessageW(lphc->owner, WM_CTLCOLOREDIT,
				       (WPARAM)hDC, (LPARAM)lphc->self );
    }
    else
    {
      hBkgBrush = (HBRUSH)SendMessageW(lphc->owner, WM_CTLCOLORLISTBOX,
				       (WPARAM)hDC, (LPARAM)lphc->self );
    }
  }

  /*
   * Catch errors.
   */
  if( !hBkgBrush )
    hBkgBrush = GetSysColorBrush(COLOR_WINDOW);

  return hBkgBrush;
}

/***********************************************************************
 *           COMBO_EraseBackground
 */
static LRESULT COMBO_EraseBackground(
  HWND        hwnd,
  LPHEADCOMBO lphc,
  HDC         hParamDC)
{
  HBRUSH  hBkgBrush;
  HDC 	  hDC;

  if(lphc->wState & CBF_EDIT)
      return TRUE;

  hDC = (hParamDC) ? hParamDC
		   : GetDC(hwnd);
  /*
   * Retrieve the background brush
   */
  hBkgBrush = COMBO_PrepareColors(lphc, hDC);

  FillRect(hDC, &lphc->textRect, hBkgBrush);

  if (!hParamDC)
    ReleaseDC(hwnd, hDC);

  return TRUE;
}

/***********************************************************************
 *           COMBO_Paint
 */
static LRESULT COMBO_Paint(LPHEADCOMBO lphc, HDC hParamDC)
{
  PAINTSTRUCT ps;
  HDC 	hDC;

  hDC = (hParamDC) ? hParamDC
		   : BeginPaint( lphc->self, &ps);

  DbgPrint("hdc=%p\n", hDC);

  if( hDC && !(lphc->wState & CBF_NOREDRAW) )
  {
      HBRUSH	hPrevBrush, hBkgBrush;

      /*
       * Retrieve the background brush and select it in the
       * DC.
       */
      hBkgBrush = COMBO_PrepareColors(lphc, hDC);

      hPrevBrush = SelectObject( hDC, hBkgBrush );

      /*
       * In non 3.1 look, there is a sunken border on the combobox
       */
      //if (TWEAK_WineLook != WIN31_LOOK)
      //{
	CBPaintBorder(lphc->self, lphc, hDC);
      //}

      if( !IsRectEmpty(&lphc->buttonRect) )
      {
	CBPaintButton(lphc, hDC, lphc->buttonRect);
      }

      /* paint the edit control padding area */
      if (CB_GETTYPE(lphc) != CBS_DROPDOWNLIST)
      {
          RECT rPadEdit = lphc->textRect;

          InflateRect(&rPadEdit, EDIT_CONTROL_PADDING(), EDIT_CONTROL_PADDING());

          FrameRect( hDC, &rPadEdit, GetSysColorBrush(COLOR_WINDOW) );
      }

      if( !(lphc->wState & CBF_EDIT) )
      {
	CBPaintText( lphc, hDC, lphc->textRect);
      }

      if( hPrevBrush )
	SelectObject( hDC, hPrevBrush );
  }

  if( !hParamDC )
    EndPaint(lphc->self, &ps);

  return 0;
}

/***********************************************************************
 *           CBUpdateLBox
 *
 * Select listbox entry according to the contents of the edit control.
 */
static INT CBUpdateLBox( LPHEADCOMBO lphc, BOOL bSelect )
{
   INT	length, idx;
   LPWSTR pText = NULL;

   idx = LB_ERR;
   length = SendMessageW( lphc->hWndEdit, WM_GETTEXTLENGTH, 0, 0 );

   if( length > 0 )
       pText = HeapAlloc( GetProcessHeap(), 0, (length + 1) * sizeof(WCHAR));

   DbgPrint("\t edit text length %i\n", length );

   if( pText )
   {
       if( length ) GetWindowTextW( lphc->hWndEdit, pText, length + 1);
       else pText[0] = '\0';
       idx = SendMessageW(lphc->hWndLBox, LB_FINDSTRING,
			     (WPARAM)(-1), (LPARAM)pText );
       HeapFree( GetProcessHeap(), 0, pText );
   }

   SendMessageW(lphc->hWndLBox, LB_SETCURSEL, (WPARAM)(bSelect ? idx : -1), 0);

   /* probably superfluous but Windows sends this too */
   SendMessageW(lphc->hWndLBox, LB_SETCARETINDEX, (WPARAM)(idx < 0 ? 0 : idx), 0);
   SendMessageW(lphc->hWndLBox, LB_SETTOPINDEX, (WPARAM)(idx < 0 ? 0 : idx), 0);

   return idx;
}

/***********************************************************************
 *           CBUpdateEdit
 *
 * Copy a listbox entry to the edit control.
 */
static void CBUpdateEdit( LPHEADCOMBO lphc , INT index )
{
   INT	length;
   LPWSTR pText = NULL;
   static const WCHAR empty_stringW[] = { 0 };

   DbgPrint("\t %i\n", index );

   if( index >= 0 ) /* got an entry */
   {
       length = SendMessageW(lphc->hWndLBox, LB_GETTEXTLEN, (WPARAM)index, 0);
       if( length != LB_ERR)
       {
	   if( (pText = HeapAlloc( GetProcessHeap(), 0, (length + 1) * sizeof(WCHAR))) )
	   {
		SendMessageW(lphc->hWndLBox, LB_GETTEXT,
				(WPARAM)index, (LPARAM)pText );
	   }
       }
   }

   lphc->wState |= (CBF_NOEDITNOTIFY | CBF_NOLBSELECT);
   SendMessageW(lphc->hWndEdit, WM_SETTEXT, 0, pText ? (LPARAM)pText : (LPARAM)empty_stringW);
   lphc->wState &= ~(CBF_NOEDITNOTIFY | CBF_NOLBSELECT);

   if( lphc->wState & CBF_FOCUSED )
      SendMessageW(lphc->hWndEdit, EM_SETSEL, 0, (LPARAM)(-1));

   if( pText )
       HeapFree( GetProcessHeap(), 0, pText );
}

/***********************************************************************
 *           CBDropDown
 *
 * Show listbox popup.
 */
static void CBDropDown( LPHEADCOMBO lphc )
{
   RECT rect,r;
   int nItems = 0;
   int nDroppedHeight;

   DbgPrint("[%p]: drop down\n", lphc->self);

   CB_NOTIFY( lphc, CBN_DROPDOWN );

   /* set selection */

   lphc->wState |= CBF_DROPPED;
   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
   {
       lphc->droppedIndex = CBUpdateLBox( lphc, TRUE );

       /* Update edit only if item is in the list */
       if( !(lphc->wState & CBF_CAPTURE) && lphc->droppedIndex >= 0)
	 CBUpdateEdit( lphc, lphc->droppedIndex );
   }
   else
   {
       lphc->droppedIndex = SendMessageW(lphc->hWndLBox, LB_GETCURSEL, 0, 0);

       SendMessageW(lphc->hWndLBox, LB_SETTOPINDEX,
                     (WPARAM)(lphc->droppedIndex == LB_ERR ? 0 : lphc->droppedIndex), 0 );
       SendMessageW(lphc->hWndLBox, LB_CARETON, 0, 0);
   }

   /* now set popup position */
   GetWindowRect( lphc->self, &rect );

   /*
    * If it's a dropdown, the listbox is offset
    */
   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
     rect.left += COMBO_EDITBUTTONSPACE();

  /* if the dropped height is greater than the total height of the dropped
     items list, then force the drop down list height to be the total height
     of the items in the dropped list */

  /* And Remove any extra space (Best Fit) */
   nDroppedHeight = lphc->droppedRect.bottom - lphc->droppedRect.top;
  /* if listbox length has been set directly by its handle */
   GetWindowRect(lphc->hWndLBox, &r);
   if (nDroppedHeight < r.bottom - r.top)
       nDroppedHeight = r.bottom - r.top;
   nItems = (int)SendMessageW(lphc->hWndLBox, LB_GETCOUNT, 0, 0);

   if (nItems > 0)
   {
      int nHeight;
      int nIHeight;

      nIHeight = (int)SendMessageW(lphc->hWndLBox, LB_GETITEMHEIGHT, 0, 0);

      nHeight = nIHeight*nItems;

      if (nHeight < nDroppedHeight - COMBO_YBORDERSIZE())
         nDroppedHeight = nHeight + COMBO_YBORDERSIZE();

      if (nDroppedHeight < nIHeight)
      {
            if (nItems < 5)
                nDroppedHeight = (nItems+1)*nIHeight;
            else
                nDroppedHeight = 6*nIHeight;
      }
   }

   /*If height of dropped rectangle gets beyond a screen size it should go up, otherwise down.*/
   if( (rect.bottom + nDroppedHeight) >= GetSystemMetrics( SM_CYSCREEN ) )
      rect.bottom = rect.top - nDroppedHeight;

   SetWindowPos( lphc->hWndLBox, HWND_TOP, rect.left, rect.bottom,
		 lphc->droppedRect.right - lphc->droppedRect.left,
		 nDroppedHeight,
		 SWP_NOACTIVATE | SWP_SHOWWINDOW);


   if( !(lphc->wState & CBF_NOREDRAW) )
     RedrawWindow( lphc->self, NULL, 0, RDW_INVALIDATE |
			   RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN );

   EnableWindow( lphc->hWndLBox, TRUE );
   if (GetCapture() != lphc->self)
      SetCapture(lphc->hWndLBox);
}

/***********************************************************************
 *           CBRollUp
 *
 * Hide listbox popup.
 */
static void CBRollUp( LPHEADCOMBO lphc, BOOL ok, BOOL bButton )
{
   HWND	hWnd = lphc->self;

   DbgPrint("[%p]: sel ok? [%i] dropped? [%i]\n",
	 lphc->self, (INT)ok, (INT)(lphc->wState & CBF_DROPPED));

   CB_NOTIFY( lphc, (ok) ? CBN_SELENDOK : CBN_SELENDCANCEL );

   if( IsWindow( hWnd ) && CB_GETTYPE(lphc) != CBS_SIMPLE )
   {

       if( lphc->wState & CBF_DROPPED )
       {
	   RECT	rect;

	   lphc->wState &= ~CBF_DROPPED;
	   ShowWindow( lphc->hWndLBox, SW_HIDE );

           if(GetCapture() == lphc->hWndLBox)
           {
               ReleaseCapture();
           }

	   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
	   {
	       rect = lphc->buttonRect;
	   }
	   else
           {
	       if( bButton )
	       {
		 UnionRect( &rect,
			    &lphc->buttonRect,
			    &lphc->textRect);
	       }
	       else
		 rect = lphc->textRect;

	       bButton = TRUE;
	   }

	   if( bButton && !(lphc->wState & CBF_NOREDRAW) )
	       RedrawWindow( hWnd, &rect, 0, RDW_INVALIDATE |
			       RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN );
	   CB_NOTIFY( lphc, CBN_CLOSEUP );
       }
   }
}

/***********************************************************************
 *           COMBO_FlipListbox
 *
 * Used by the ComboLBox to show/hide itself in response to VK_F4, etc...
 */
BOOL COMBO_FlipListbox( LPHEADCOMBO lphc, BOOL ok, BOOL bRedrawButton )
{
   if( lphc->wState & CBF_DROPPED )
   {
       CBRollUp( lphc, ok, bRedrawButton );
       return FALSE;
   }

   CBDropDown( lphc );
   return TRUE;
}

/***********************************************************************
 *           CBRepaintButton
 */
static void CBRepaintButton( LPHEADCOMBO lphc )
   {
  InvalidateRect(lphc->self, &lphc->buttonRect, TRUE);
  UpdateWindow(lphc->self);
}

/***********************************************************************
 *           COMBO_SetFocus
 */
static void COMBO_SetFocus( LPHEADCOMBO lphc )
{
   if( !(lphc->wState & CBF_FOCUSED) )
   {
       if( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST )
           SendMessageW(lphc->hWndLBox, LB_CARETON, 0, 0);

       /* This is wrong. Message sequences seem to indicate that this
          is set *after* the notify. */
       /* lphc->wState |= CBF_FOCUSED;  */

       if( !(lphc->wState & CBF_EDIT) )
	 InvalidateRect(lphc->self, &lphc->textRect, TRUE);

       CB_NOTIFY( lphc, CBN_SETFOCUS );
       lphc->wState |= CBF_FOCUSED;
   }
}

/***********************************************************************
 *           COMBO_KillFocus
 */
static void COMBO_KillFocus( LPHEADCOMBO lphc )
{
   HWND	hWnd = lphc->self;

   if( lphc->wState & CBF_FOCUSED )
   {
       CBRollUp( lphc, FALSE, TRUE );
       if( IsWindow( hWnd ) )
       {
           if( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST )
               SendMessageW(lphc->hWndLBox, LB_CARETOFF, 0, 0);

 	   lphc->wState &= ~CBF_FOCUSED;

           /* redraw text */
	   if( !(lphc->wState & CBF_EDIT) )
	     InvalidateRect(lphc->self, &lphc->textRect, TRUE);

           CB_NOTIFY( lphc, CBN_KILLFOCUS );
       }
   }
}

/***********************************************************************
 *           COMBO_Command
 */
static LRESULT COMBO_Command( LPHEADCOMBO lphc, WPARAM wParam, HWND hWnd )
{
   if ( lphc->wState & CBF_EDIT && lphc->hWndEdit == hWnd )
   {
       /* ">> 8" makes gcc generate jump-table instead of cmp ladder */

       switch( HIWORD(wParam) >> 8 )
       {
	   case (EN_SETFOCUS >> 8):

               DbgPrint("[%p]: edit [%p] got focus\n", lphc->self, lphc->hWndEdit );

		COMBO_SetFocus( lphc );
	        break;

	   case (EN_KILLFOCUS >> 8):

               DbgPrint("[%p]: edit [%p] lost focus\n", lphc->self, lphc->hWndEdit );

		/* NOTE: it seems that Windows' edit control sends an
		 * undocumented message WM_USER + 0x1B instead of this
		 * notification (only when it happens to be a part of
		 * the combo). ?? - AK.
		 */

		COMBO_KillFocus( lphc );
		break;


	   case (EN_CHANGE >> 8):
	       /*
	        * In some circumstances (when the selection of the combobox
		* is changed for example) we don't wans the EN_CHANGE notification
		* to be forwarded to the parent of the combobox. This code
		* checks a flag that is set in these occasions and ignores the
		* notification.
	        */
		if (lphc->wState & CBF_NOLBSELECT)
		{
		  lphc->wState &= ~CBF_NOLBSELECT;
		}
		else
		{
		  CBUpdateLBox( lphc, lphc->wState & CBF_DROPPED );
		}

	        if (!(lphc->wState & CBF_NOEDITNOTIFY))
		  CB_NOTIFY( lphc, CBN_EDITCHANGE );
		break;

	   case (EN_UPDATE >> 8):
	        if (!(lphc->wState & CBF_NOEDITNOTIFY))
		  CB_NOTIFY( lphc, CBN_EDITUPDATE );
		break;

	   case (EN_ERRSPACE >> 8):
		CB_NOTIFY( lphc, CBN_ERRSPACE );
       }
   }
   else if( lphc->hWndLBox == hWnd )
   {
       switch( HIWORD(wParam) )
       {
	   case LBN_ERRSPACE:
		CB_NOTIFY( lphc, CBN_ERRSPACE );
		break;

	   case LBN_DBLCLK:
		CB_NOTIFY( lphc, CBN_DBLCLK );
		break;

	   case LBN_SELCHANGE:
	   case LBN_SELCANCEL:

               DbgPrint("[%p]: lbox selection change [%x]\n", lphc->self, lphc->wState );

		if( HIWORD(wParam) == LBN_SELCHANGE)
		{
		   if( lphc->wState & CBF_EDIT )
		   {
		       INT index = SendMessageW(lphc->hWndLBox, LB_GETCURSEL, 0, 0);
		       lphc->wState |= CBF_NOLBSELECT;
		       CBUpdateEdit( lphc, index );
		       /* select text in edit, as Windows does */
		       SendMessageW(lphc->hWndEdit, EM_SETSEL, 0, (LPARAM)(-1));
		   }
		   else
		       InvalidateRect(lphc->self, &lphc->textRect, TRUE);
		}

		/* do not roll up if selection is being tracked
		 * by arrowkeys in the dropdown listbox */
                if( ((lphc->wState & CBF_DROPPED) && !(lphc->wState & CBF_NOROLLUP)) )
                {
                   CBRollUp( lphc, (HIWORD(wParam) == LBN_SELCHANGE), TRUE );
                }
		else lphc->wState &= ~CBF_NOROLLUP;

		CB_NOTIFY( lphc, CBN_SELCHANGE );

		/* fall through */

	   case LBN_SETFOCUS:
	   case LBN_KILLFOCUS:
		/* nothing to do here since ComboLBox always resets the focus to its
		 * combo/edit counterpart */
		 break;
       }
   }
   return 0;
}

/***********************************************************************
 *           COMBO_ItemOp
 *
 * Fixup an ownerdrawn item operation and pass it up to the combobox owner.
 */
static LRESULT COMBO_ItemOp( LPHEADCOMBO lphc, UINT msg, LPARAM lParam )
{
   HWND hWnd = lphc->self;
   UINT id = GetWindowLongA( hWnd, GWL_ID );

   DbgPrint("[%p]: ownerdraw op %04x\n", lphc->self, msg );

   switch( msg )
   {
   case WM_DELETEITEM:
       {
           DELETEITEMSTRUCT *lpIS = (DELETEITEMSTRUCT *)lParam;
           lpIS->CtlType  = ODT_COMBOBOX;
           lpIS->CtlID    = id;
           lpIS->hwndItem = hWnd;
           break;
       }
   case WM_DRAWITEM:
       {
           DRAWITEMSTRUCT *lpIS = (DRAWITEMSTRUCT *)lParam;
           lpIS->CtlType  = ODT_COMBOBOX;
           lpIS->CtlID    = id;
           lpIS->hwndItem = hWnd;
           break;
       }
   case WM_COMPAREITEM:
       {
           COMPAREITEMSTRUCT *lpIS = (COMPAREITEMSTRUCT *)lParam;
           lpIS->CtlType  = ODT_COMBOBOX;
           lpIS->CtlID    = id;
           lpIS->hwndItem = hWnd;
           break;
       }
   case WM_MEASUREITEM:
       {
           MEASUREITEMSTRUCT *lpIS = (MEASUREITEMSTRUCT *)lParam;
           lpIS->CtlType  = ODT_COMBOBOX;
           lpIS->CtlID    = id;
           break;
       }
   }
   return SendMessageW(lphc->owner, msg, id, lParam);
}


/***********************************************************************
 *           COMBO_GetTextW
 */
static LRESULT COMBO_GetTextW( LPHEADCOMBO lphc, INT count, LPWSTR buf )
{
    INT length;

    if( lphc->wState & CBF_EDIT )
        return SendMessageW( lphc->hWndEdit, WM_GETTEXT, count, (LPARAM)buf );

    /* get it from the listbox */

    if (!count || !buf) return 0;
    if( lphc->hWndLBox )
    {
        INT idx = SendMessageW(lphc->hWndLBox, LB_GETCURSEL, 0, 0);
        if (idx == LB_ERR) goto error;
        length = SendMessageW(lphc->hWndLBox, LB_GETTEXTLEN, idx, 0 );
        if (length == LB_ERR) goto error;

        /* 'length' is without the terminating character */
        if (length >= count)
        {
            LPWSTR lpBuffer = HeapAlloc(GetProcessHeap(), 0, (length + 1) * sizeof(WCHAR));
            if (!lpBuffer) goto error;
            length = SendMessageW(lphc->hWndLBox, LB_GETTEXT, idx, (LPARAM)lpBuffer);

            /* truncate if buffer is too short */
            if (length != LB_ERR)
            {
                lstrcpynW( buf, lpBuffer, count );
                length = count;
            }
            HeapFree( GetProcessHeap(), 0, lpBuffer );
        }
        else length = SendMessageW(lphc->hWndLBox, LB_GETTEXT, idx, (LPARAM)buf);

        if (length == LB_ERR) return 0;
        return length;
    }

 error:  /* error - truncate string, return zero */
    buf[0] = 0;
    return 0;
}


/***********************************************************************
 *           COMBO_GetTextA
 *
 * NOTE! LB_GETTEXT does not count terminating \0, WM_GETTEXT does.
 *       also LB_GETTEXT might return values < 0, WM_GETTEXT doesn't.
 */
static LRESULT COMBO_GetTextA( LPHEADCOMBO lphc, INT count, LPSTR buf )
{
    INT length;

    if( lphc->wState & CBF_EDIT )
        return SendMessageA( lphc->hWndEdit, WM_GETTEXT, count, (LPARAM)buf );

    /* get it from the listbox */

    if (!count || !buf) return 0;
    if( lphc->hWndLBox )
    {
        INT idx = SendMessageA(lphc->hWndLBox, LB_GETCURSEL, 0, 0);
        if (idx == LB_ERR) goto error;
        length = SendMessageA(lphc->hWndLBox, LB_GETTEXTLEN, idx, 0 );
        if (length == LB_ERR) goto error;

        /* 'length' is without the terminating character */
        if (length >= count)
        {
            LPSTR lpBuffer = HeapAlloc(GetProcessHeap(), 0, (length + 1) );
            if (!lpBuffer) goto error;
            length = SendMessageA(lphc->hWndLBox, LB_GETTEXT, idx, (LPARAM)lpBuffer);

            /* truncate if buffer is too short */
            if (length != LB_ERR)
            {
                lstrcpynA( buf, lpBuffer, count );
                length = count;
            }
            HeapFree( GetProcessHeap(), 0, lpBuffer );
        }
        else length = SendMessageA(lphc->hWndLBox, LB_GETTEXT, idx, (LPARAM)buf);

        if (length == LB_ERR) return 0;
        return length;
    }

 error:  /* error - truncate string, return zero */
    buf[0] = 0;
    return 0;
}


/***********************************************************************
 *           CBResetPos
 *
 * This function sets window positions according to the updated
 * component placement struct.
 */
static void CBResetPos(
  LPHEADCOMBO lphc,
  LPRECT      rectEdit,
  LPRECT      rectLB,
  BOOL        bRedraw)
{
   BOOL	bDrop = (CB_GETTYPE(lphc) != CBS_SIMPLE);

   /* NOTE: logs sometimes have WM_LBUTTONUP before a cascade of
    * sizing messages */

   if( lphc->wState & CBF_EDIT )
     SetWindowPos( lphc->hWndEdit, 0,
		   rectEdit->left, rectEdit->top,
		   rectEdit->right - rectEdit->left,
		   rectEdit->bottom - rectEdit->top,
                       SWP_NOZORDER | SWP_NOACTIVATE | ((bDrop) ? SWP_NOREDRAW : 0) );

   SetWindowPos( lphc->hWndLBox, 0,
		 rectLB->left, rectLB->top,
                 rectLB->right - rectLB->left,
		 rectLB->bottom - rectLB->top,
		   SWP_NOACTIVATE | SWP_NOZORDER | ((bDrop) ? SWP_NOREDRAW : 0) );

   if( bDrop )
   {
       if( lphc->wState & CBF_DROPPED )
       {
           lphc->wState &= ~CBF_DROPPED;
           ShowWindow( lphc->hWndLBox, SW_HIDE );
       }

       if( bRedraw && !(lphc->wState & CBF_NOREDRAW) )
           RedrawWindow( lphc->self, NULL, 0,
                           RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW );
   }
}


/***********************************************************************
 *           COMBO_Size
 */
static void COMBO_Size( LPHEADCOMBO lphc )
  {
  CBCalcPlacement(lphc->self,
		  lphc,
		  &lphc->textRect,
		  &lphc->buttonRect,
		  &lphc->droppedRect);

  CBResetPos( lphc, &lphc->textRect, &lphc->droppedRect, TRUE );
}


/***********************************************************************
 *           COMBO_Font
 */
static void COMBO_Font( LPHEADCOMBO lphc, HFONT hFont, BOOL bRedraw )
{
  /*
   * Set the font
   */
  lphc->hFont = hFont;

  /*
   * Propagate to owned windows.
   */
  if( lphc->wState & CBF_EDIT )
      SendMessageW(lphc->hWndEdit, WM_SETFONT, (WPARAM)hFont, bRedraw);
  SendMessageW(lphc->hWndLBox, WM_SETFONT, (WPARAM)hFont, bRedraw);

  /*
   * Redo the layout of the control.
   */
  if ( CB_GETTYPE(lphc) == CBS_SIMPLE)
  {
    CBCalcPlacement(lphc->self,
		    lphc,
		    &lphc->textRect,
		    &lphc->buttonRect,
		    &lphc->droppedRect);

    CBResetPos( lphc, &lphc->textRect, &lphc->droppedRect, TRUE );
  }
  else
  {
    CBForceDummyResize(lphc);
  }
}


/***********************************************************************
 *           COMBO_SetItemHeight
 */
static LRESULT COMBO_SetItemHeight( LPHEADCOMBO lphc, INT index, INT height )
{
   LRESULT	lRet = CB_ERR;

   if( index == -1 ) /* set text field height */
   {
       if( height < 32768 )
       {
           lphc->editHeight = height;

	 /*
	  * Redo the layout of the control.
	  */
	 if ( CB_GETTYPE(lphc) == CBS_SIMPLE)
	 {
	   CBCalcPlacement(lphc->self,
			   lphc,
			   &lphc->textRect,
			   &lphc->buttonRect,
			   &lphc->droppedRect);

	   CBResetPos( lphc, &lphc->textRect, &lphc->droppedRect, TRUE );
	 }
	 else
	 {
	   CBForceDummyResize(lphc);
	 }

	   lRet = height;
       }
   }
   else if ( CB_OWNERDRAWN(lphc) )	/* set listbox item height */
	lRet = SendMessageW(lphc->hWndLBox, LB_SETITEMHEIGHT,
			      (WPARAM)index, (LPARAM)height );
   return lRet;
}

/***********************************************************************
 *           COMBO_SelectString
 */
static LRESULT COMBO_SelectString( LPHEADCOMBO lphc, INT start, LPARAM pText, BOOL unicode )
{
   INT index = unicode ? SendMessageW(lphc->hWndLBox, LB_SELECTSTRING, (WPARAM)start, pText) :
                         SendMessageA(lphc->hWndLBox, LB_SELECTSTRING, (WPARAM)start, pText);
   if( index >= 0 )
   {
     if( lphc->wState & CBF_EDIT )
       CBUpdateEdit( lphc, index );
     else
     {
       InvalidateRect(lphc->self, &lphc->textRect, TRUE);
     }
   }
   return (LRESULT)index;
}

/***********************************************************************
 *           COMBO_LButtonDown
 */
static void COMBO_LButtonDown( LPHEADCOMBO lphc, LPARAM lParam )
{
   POINT     pt;
   BOOL      bButton;
   HWND      hWnd = lphc->self;

   pt.x = LOWORD(lParam);
   pt.y = HIWORD(lParam);
   bButton = PtInRect(&lphc->buttonRect, pt);

   if( (CB_GETTYPE(lphc) == CBS_DROPDOWNLIST) ||
       (bButton && (CB_GETTYPE(lphc) == CBS_DROPDOWN)) )
   {
       lphc->wState |= CBF_BUTTONDOWN;
       if( lphc->wState & CBF_DROPPED )
       {
	   /* got a click to cancel selection */

           lphc->wState &= ~CBF_BUTTONDOWN;
           CBRollUp( lphc, TRUE, FALSE );
	   if( !IsWindow( hWnd ) ) return;

           if( lphc->wState & CBF_CAPTURE )
           {
               lphc->wState &= ~CBF_CAPTURE;
               ReleaseCapture();
           }
       }
       else
       {
	   /* drop down the listbox and start tracking */

           lphc->wState |= CBF_CAPTURE;
           SetCapture( hWnd );
           CBDropDown( lphc );
       }
       if( bButton ) CBRepaintButton( lphc );
   }
}

/***********************************************************************
 *           COMBO_LButtonUp
 *
 * Release capture and stop tracking if needed.
 */
static void COMBO_LButtonUp( LPHEADCOMBO lphc )
{
   if( lphc->wState & CBF_CAPTURE )
   {
       lphc->wState &= ~CBF_CAPTURE;
       if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
       {
	   INT index = CBUpdateLBox( lphc, TRUE );
	   /* Update edit only if item is in the list */
	   if(index >= 0)
	   {
	       lphc->wState |= CBF_NOLBSELECT;
	       CBUpdateEdit( lphc, index );
	       lphc->wState &= ~CBF_NOLBSELECT;
	   }
       }
       ReleaseCapture();
       SetCapture(lphc->hWndLBox);
   }

   if( lphc->wState & CBF_BUTTONDOWN )
   {
       lphc->wState &= ~CBF_BUTTONDOWN;
       CBRepaintButton( lphc );
   }
}

/***********************************************************************
 *           COMBO_MouseMove
 *
 * Two things to do - track combo button and release capture when
 * pointer goes into the listbox.
 */
static void COMBO_MouseMove( LPHEADCOMBO lphc, WPARAM wParam, LPARAM lParam )
{
   POINT  pt;
   RECT   lbRect;

   pt.x = LOWORD(lParam);
   pt.y = HIWORD(lParam);

   if( lphc->wState & CBF_BUTTONDOWN )
   {
     BOOL bButton;

     bButton = PtInRect(&lphc->buttonRect, pt);

     if( !bButton )
     {
       lphc->wState &= ~CBF_BUTTONDOWN;
       CBRepaintButton( lphc );
     }
   }

   GetClientRect( lphc->hWndLBox, &lbRect );
   MapWindowPoints( lphc->self, lphc->hWndLBox, &pt, 1 );
   if( PtInRect(&lbRect, pt) )
   {
       lphc->wState &= ~CBF_CAPTURE;
       ReleaseCapture();
       if( CB_GETTYPE(lphc) == CBS_DROPDOWN ) CBUpdateLBox( lphc, TRUE );

       /* hand over pointer tracking */
       SendMessageW(lphc->hWndLBox, WM_LBUTTONDOWN, wParam, lParam);
   }
}


/***********************************************************************
 *           ComboWndProc_common
 *
 * http://www.microsoft.com/msdn/sdk/platforms/doc/sdk/win32/ctrl/src/combobox_15.htm
 */
static LRESULT ComboWndProc_common( HWND hwnd, UINT message,
                                    WPARAM wParam, LPARAM lParam, BOOL unicode )
{
      LPHEADCOMBO lphc = (LPHEADCOMBO)GetWindowLongA( hwnd, 0 );

      //TRACE("[%p]: msg %s wp %08x lp %08lx\n",
      //      hwnd, SPY_GetMsgName(message, hwnd), wParam, lParam );

      if( lphc || message == WM_NCCREATE )
      switch(message)
      {

	/* System messages */

     	case WM_NCCREATE:
	{
		LONG style = unicode ? ((LPCREATESTRUCTW)lParam)->style :
				       ((LPCREATESTRUCTA)lParam)->style;
                return COMBO_NCCreate(hwnd, style);
	}
     	case WM_NCDESTROY:
		COMBO_NCDestroy(lphc);
		break;/* -> DefWindowProc */

     	case WM_CREATE:
	{
		HWND hwndParent;
		LONG style;
		if(unicode)
		{
		    hwndParent = ((LPCREATESTRUCTW)lParam)->hwndParent;
		    style = ((LPCREATESTRUCTW)lParam)->style;
		}
		else
		{
		    hwndParent = ((LPCREATESTRUCTA)lParam)->hwndParent;
		    style = ((LPCREATESTRUCTA)lParam)->style;
		}
                return COMBO_Create(hwnd, lphc, hwndParent, style, unicode);
	}

        case WM_PRINTCLIENT:
	        if (lParam & PRF_ERASEBKGND)
		  COMBO_EraseBackground(hwnd, lphc, (HDC)wParam);

		/* Fallthrough */
     	case WM_PAINT:
		/* wParam may contain a valid HDC! */
		return  COMBO_Paint(lphc, (HDC)wParam);
	case WM_ERASEBKGND:
		return  COMBO_EraseBackground(hwnd, lphc, (HDC)wParam);
	case WM_GETDLGCODE:
	{
		LRESULT result = DLGC_WANTARROWS | DLGC_WANTCHARS;
		if (lParam && (((LPMSG)lParam)->message == WM_KEYDOWN))
		{
		   int vk = (int)((LPMSG)lParam)->wParam;

		   if ((vk == VK_RETURN || vk == VK_ESCAPE) && (lphc->wState & CBF_DROPPED))
		       result |= DLGC_WANTMESSAGE;
		}
		return  result;
	}
	case WM_WINDOWPOSCHANGING:
	        return  COMBO_WindowPosChanging(hwnd, lphc, (LPWINDOWPOS)lParam);
    case WM_WINDOWPOSCHANGED:
        /* SetWindowPos can be called on a Combobox to resize its Listbox.
         * In that case, the Combobox itself will not be resized, so we won't
         * get a WM_SIZE. Since we still want to update the Listbox, we have to
         * do it here.
         */
        /* fall through */
	case WM_SIZE:
	        if( lphc->hWndLBox &&
		  !(lphc->wState & CBF_NORESIZE) ) COMBO_Size( lphc );
		return  TRUE;
	case WM_SETFONT:
		COMBO_Font( lphc, (HFONT)wParam, (BOOL)lParam );
		return  TRUE;
	case WM_GETFONT:
		return  (LRESULT)lphc->hFont;
	case WM_SETFOCUS:
		if( lphc->wState & CBF_EDIT )
		    SetFocus( lphc->hWndEdit );
		else
		    COMBO_SetFocus( lphc );
		return  TRUE;
	case WM_KILLFOCUS:
            {
                //HWND hwndFocus = WIN_GetFullHandle( (HWND)wParam );
		//if( !hwndFocus ||
		//    (hwndFocus != lphc->hWndEdit && hwndFocus != lphc->hWndLBox ))
		//    COMBO_KillFocus( lphc );
		return  TRUE;
            }
	case WM_COMMAND:
		//return  COMBO_Command( lphc, wParam, WIN_GetFullHandle( (HWND)lParam ) );
		return COMBO_Command( lphc, wParam, (HWND)lParam );

	case WM_GETTEXT:
            return unicode ? COMBO_GetTextW( lphc, wParam, (LPWSTR)lParam )
                           : COMBO_GetTextA( lphc, wParam, (LPSTR)lParam );
	case WM_SETTEXT:
	case WM_GETTEXTLENGTH:
	case WM_CLEAR:
                if ((message == WM_GETTEXTLENGTH) && !ISWIN31 && !(lphc->wState & CBF_EDIT))
                {
                    int j = SendMessageW(lphc->hWndLBox, LB_GETCURSEL, 0, 0);
                    if (j == -1) return 0;
                    return unicode ? SendMessageW(lphc->hWndLBox, LB_GETTEXTLEN, j, 0) :
                                     SendMessageA(lphc->hWndLBox, LB_GETTEXTLEN, j, 0);
                }
		else if( lphc->wState & CBF_EDIT )
		{
		    LRESULT ret;
		    lphc->wState |= CBF_NOEDITNOTIFY;
		    ret = unicode ? SendMessageW(lphc->hWndEdit, message, wParam, lParam) :
				    SendMessageA(lphc->hWndEdit, message, wParam, lParam);
		    lphc->wState &= ~CBF_NOEDITNOTIFY;
		    return ret;
		}
		else return CB_ERR;
	case WM_CUT:
        case WM_PASTE:
	case WM_COPY:
		if( lphc->wState & CBF_EDIT )
		{
		    return unicode ? SendMessageW(lphc->hWndEdit, message, wParam, lParam) :
				     SendMessageA(lphc->hWndEdit, message, wParam, lParam);
		}
		else return  CB_ERR;

	case WM_DRAWITEM:
	case WM_DELETEITEM:
	case WM_COMPAREITEM:
	case WM_MEASUREITEM:
		return COMBO_ItemOp(lphc, message, lParam);
	case WM_ENABLE:
		if( lphc->wState & CBF_EDIT )
		    EnableWindow( lphc->hWndEdit, (BOOL)wParam );
		EnableWindow( lphc->hWndLBox, (BOOL)wParam );

		/* Force the control to repaint when the enabled state changes. */
		InvalidateRect(lphc->self, NULL, TRUE);
		return  TRUE;
	case WM_SETREDRAW:
		if( wParam )
		    lphc->wState &= ~CBF_NOREDRAW;
		else
		    lphc->wState |= CBF_NOREDRAW;

		if( lphc->wState & CBF_EDIT )
		    SendMessageW(lphc->hWndEdit, message, wParam, lParam);
		SendMessageW(lphc->hWndLBox, message, wParam, lParam);
		return  0;
	case WM_SYSKEYDOWN:
		if( KEYDATA_ALT & HIWORD(lParam) )
		    if( wParam == VK_UP || wParam == VK_DOWN )
			COMBO_FlipListbox( lphc, FALSE, FALSE );
                return  0;

	case WM_CHAR:
	case WM_IME_CHAR:
	case WM_KEYDOWN:
	{
		HWND hwndTarget;

		if ((wParam == VK_RETURN || wParam == VK_ESCAPE) &&
		     (lphc->wState & CBF_DROPPED))
		{
		   CBRollUp( lphc, wParam == VK_RETURN, FALSE );
		   return TRUE;
		}

		if( lphc->wState & CBF_EDIT )
		    hwndTarget = lphc->hWndEdit;
		else
		    hwndTarget = lphc->hWndLBox;

		return unicode ? SendMessageW(hwndTarget, message, wParam, lParam) :
				 SendMessageA(hwndTarget, message, wParam, lParam);
	}
	case WM_LBUTTONDOWN:
		if( !(lphc->wState & CBF_FOCUSED) ) SetFocus( lphc->self );
		if( lphc->wState & CBF_FOCUSED ) COMBO_LButtonDown( lphc, lParam );
		return  TRUE;
	case WM_LBUTTONUP:
		COMBO_LButtonUp( lphc );
		return  TRUE;
	case WM_MOUSEMOVE:
		if( lphc->wState & CBF_CAPTURE )
		    COMBO_MouseMove( lphc, wParam, lParam );
		return  TRUE;

        case WM_MOUSEWHEEL:
                if (wParam & (MK_SHIFT | MK_CONTROL))
                    return unicode ? DefWindowProcW(hwnd, message, wParam, lParam) :
				     DefWindowProcA(hwnd, message, wParam, lParam);
                if (SHIWORD(wParam) > 0) return SendMessageW(hwnd, WM_KEYDOWN, VK_UP, 0);
                if (SHIWORD(wParam) < 0) return SendMessageW(hwnd, WM_KEYDOWN, VK_DOWN, 0);
                return TRUE;

	/* Combo messages */

	case CB_ADDSTRING:
		return unicode ? SendMessageW(lphc->hWndLBox, LB_ADDSTRING, 0, lParam) :
				 SendMessageA(lphc->hWndLBox, LB_ADDSTRING, 0, lParam);
	case CB_INSERTSTRING:
		return unicode ? SendMessageW(lphc->hWndLBox, LB_INSERTSTRING, wParam, lParam) :
				 SendMessageA(lphc->hWndLBox, LB_INSERTSTRING, wParam, lParam);
	case CB_DELETESTRING:
		return unicode ? SendMessageW(lphc->hWndLBox, LB_DELETESTRING, wParam, 0) :
				 SendMessageA(lphc->hWndLBox, LB_DELETESTRING, wParam, 0);
	case CB_SELECTSTRING:
		return COMBO_SelectString(lphc, (INT)wParam, lParam, unicode);
	case CB_FINDSTRING:
		return unicode ? SendMessageW(lphc->hWndLBox, LB_FINDSTRING, wParam, lParam) :
				 SendMessageA(lphc->hWndLBox, LB_FINDSTRING, wParam, lParam);
	case CB_FINDSTRINGEXACT:
		return unicode ? SendMessageW(lphc->hWndLBox, LB_FINDSTRINGEXACT, wParam, lParam) :
				 SendMessageA(lphc->hWndLBox, LB_FINDSTRINGEXACT, wParam, lParam);
	case CB_SETITEMHEIGHT:
		return  COMBO_SetItemHeight( lphc, (INT)wParam, (INT)lParam);
	case CB_GETITEMHEIGHT:
		if( (INT)wParam >= 0 )	/* listbox item */
                    return SendMessageW(lphc->hWndLBox, LB_GETITEMHEIGHT, wParam, 0);
                return  CBGetTextAreaHeight(hwnd, lphc);
	case CB_RESETCONTENT:
		SendMessageW(lphc->hWndLBox, LB_RESETCONTENT, 0, 0);
                if( (lphc->wState & CBF_EDIT) && CB_HASSTRINGS(lphc) )
		{
		    static const WCHAR empty_stringW[] = { 0 };
                    SendMessageW(lphc->hWndEdit, WM_SETTEXT, 0, (LPARAM)empty_stringW);
		}
                else
                    InvalidateRect(lphc->self, NULL, TRUE);
		return  TRUE;
	case CB_INITSTORAGE:
		return SendMessageW(lphc->hWndLBox, LB_INITSTORAGE, wParam, lParam);
	case CB_GETHORIZONTALEXTENT:
		return SendMessageW(lphc->hWndLBox, LB_GETHORIZONTALEXTENT, 0, 0);
	case CB_SETHORIZONTALEXTENT:
		return SendMessageW(lphc->hWndLBox, LB_SETHORIZONTALEXTENT, wParam, 0);
	case CB_GETTOPINDEX:
		return SendMessageW(lphc->hWndLBox, LB_GETTOPINDEX, 0, 0);
	case CB_GETLOCALE:
		return SendMessageW(lphc->hWndLBox, LB_GETLOCALE, 0, 0);
	case CB_SETLOCALE:
		return SendMessageW(lphc->hWndLBox, LB_SETLOCALE, wParam, 0);
	case CB_GETDROPPEDWIDTH:
		if( lphc->droppedWidth )
                    return  lphc->droppedWidth;
		return  lphc->droppedRect.right - lphc->droppedRect.left;
	case CB_SETDROPPEDWIDTH:
		if( (CB_GETTYPE(lphc) != CBS_SIMPLE) &&
		    (INT)wParam < 32768 ) lphc->droppedWidth = (INT)wParam;
		return  CB_ERR;
	case CB_GETDROPPEDCONTROLRECT:
		if( lParam ) CBGetDroppedControlRect(lphc, (LPRECT)lParam );
		return  CB_OKAY;
	case CB_GETDROPPEDSTATE:
		return  (lphc->wState & CBF_DROPPED) ? TRUE : FALSE;
	case CB_DIR:
		if(message == CB_DIR) message = LB_DIR;
		return unicode ? SendMessageW(lphc->hWndLBox, message, wParam, lParam) :
				 SendMessageA(lphc->hWndLBox, message, wParam, lParam);

	case CB_SHOWDROPDOWN:
		if( CB_GETTYPE(lphc) != CBS_SIMPLE )
		{
		    if( wParam )
		    {
			if( !(lphc->wState & CBF_DROPPED) )
			    CBDropDown( lphc );
		    }
		    else
			if( lphc->wState & CBF_DROPPED )
		            CBRollUp( lphc, FALSE, TRUE );
		}
		return  TRUE;
	case CB_GETCOUNT:
		return SendMessageW(lphc->hWndLBox, LB_GETCOUNT, 0, 0);
	case CB_GETCURSEL:
		return SendMessageW(lphc->hWndLBox, LB_GETCURSEL, 0, 0);
	case CB_SETCURSEL:
		lParam = SendMessageW(lphc->hWndLBox, LB_SETCURSEL, wParam, 0);
	        if( lParam >= 0 )
	            SendMessageW(lphc->hWndLBox, LB_SETTOPINDEX, wParam, 0);

		/* no LBN_SELCHANGE in this case, update manually */
		if( lphc->wState & CBF_EDIT )
		    CBUpdateEdit( lphc, (INT)wParam );
		else
		    InvalidateRect(lphc->self, &lphc->textRect, TRUE);
		lphc->wState &= ~CBF_SELCHANGE;
	        return  lParam;
	case CB_GETLBTEXT:
		return unicode ? SendMessageW(lphc->hWndLBox, LB_GETTEXT, wParam, lParam) :
				 SendMessageA(lphc->hWndLBox, LB_GETTEXT, wParam, lParam);
	case CB_GETLBTEXTLEN:
                return unicode ? SendMessageW(lphc->hWndLBox, LB_GETTEXTLEN, wParam, 0) :
                                 SendMessageA(lphc->hWndLBox, LB_GETTEXTLEN, wParam, 0);
	case CB_GETITEMDATA:
		return SendMessageW(lphc->hWndLBox, LB_GETITEMDATA, wParam, 0);
	case CB_SETITEMDATA:
		return SendMessageW(lphc->hWndLBox, LB_SETITEMDATA, wParam, lParam);
	case CB_GETEDITSEL:
		/* Edit checks passed parameters itself */
		if( lphc->wState & CBF_EDIT )
		    return SendMessageW(lphc->hWndEdit, EM_GETSEL, wParam, lParam);
		return  CB_ERR;
	case CB_SETEDITSEL:
		if( lphc->wState & CBF_EDIT )
                    return SendMessageW(lphc->hWndEdit, EM_SETSEL,
			  (INT)(INT16)LOWORD(lParam), (INT)(INT16)HIWORD(lParam) );
		return  CB_ERR;
	case CB_SETEXTENDEDUI:
                if( CB_GETTYPE(lphc) == CBS_SIMPLE )
                    return  CB_ERR;
		if( wParam )
		    lphc->wState |= CBF_EUI;
		else lphc->wState &= ~CBF_EUI;
		return  CB_OKAY;
	case CB_GETEXTENDEDUI:
		return  (lphc->wState & CBF_EUI) ? TRUE : FALSE;

	default:
		if (message >= WM_USER)
		    DbgPrint("unknown msg WM_USER+%04x wp=%04x lp=%08lx\n",
			message - WM_USER, wParam, lParam );
		break;
      }
      return unicode ? DefWindowProcW(hwnd, message, wParam, lParam) :
                       DefWindowProcA(hwnd, message, wParam, lParam);
}

/***********************************************************************
 *           ComboWndProcA
 *
 * This is just a wrapper for the real ComboWndProc which locks/unlocks
 * window structs.
 */
static LRESULT WINAPI ComboWndProcA( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if (!IsWindow(hwnd)) return 0;
    return ComboWndProc_common( hwnd, message, wParam, lParam, FALSE );
}

/***********************************************************************
 *           ComboWndProcW
 */
static LRESULT WINAPI ComboWndProcW( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if (!IsWindow(hwnd)) return 0;
    return ComboWndProc_common( hwnd, message, wParam, lParam, TRUE );
}

/*************************************************************************
 *           GetComboBoxInfo   (USER32.@)
 */
BOOL WINAPI GetComboBoxInfo(HWND hwndCombo,      /* [in] handle to combo box */
			    PCOMBOBOXINFO pcbi   /* [in/out] combo box information */)
{
    FIXME("\n");
    return FALSE;

}
