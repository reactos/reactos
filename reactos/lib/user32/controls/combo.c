/*
 * Combo controls
 * 
 * Copyright 1997 Alex Korobka
 * 
 * FIXME: roll up in Netscape 3.01.
 */

#include <string.h>

#include <windows.h>
#include <user32/sysmetr.h>
#include <user32/win.h>

#include <user32/heapdup.h>
#include <user32/combo.h>
#include <user32/debug.h>

  /* bits in the dwKeyData */
#define KEYDATA_ALT             0x2000
#define KEYDATA_PREVSTATE       0x4000

/*
 * Additional combo box definitions
 */

#define CB_GETPTR( wnd )      (*(LPHEADCOMBO*)((wnd)->wExtra))
#define CB_NOTIFY( lphc, code ) \
	(SendMessageA( (lphc)->owner, WM_COMMAND, \
			 MAKEWPARAM((lphc)->self->wIDmenu, (code)), (LPARAM)(lphc)->self->hwndSelf))
#define CB_GETEDITTEXTLENGTH( lphc ) \
	(SendMessageA( (lphc)->hWndEdit, WM_GETTEXTLENGTH, 0, 0 ))

static HBITMAP 	hComboBmp = 0;
static UINT		CBitHeight, CBitWidth;
static UINT		CBitOffset = 8;

/***********************************************************************
 *           COMBO_Init
 *
 * Load combo button bitmap.
 */
WINBOOL COMBO_Init(void)
{
  HDC		hDC;
  
  if( hComboBmp ) return TRUE;
  if( (hDC = CreateCompatibleDC(0)) )
  {
    WINBOOL	bRet = FALSE;
    if( (hComboBmp = LoadBitmap(0, MAKEINTRESOURCE(OBM_COMBO))) )
    {
      BITMAP      bm;
      HBITMAP     hPrevB;
      RECT        r;

      GetObjectA( hComboBmp, sizeof(bm), &bm );
      CBitHeight = bm.bmHeight;
      CBitWidth  = bm.bmWidth;

      DPRINT( "combo bitmap [%i,%i]\n", CBitWidth, CBitHeight );

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
static LRESULT COMBO_NCCreate(WND* wnd, LPARAM lParam)
{
   LPHEADCOMBO 		lphc;

   if ( wnd && COMBO_Init() &&
      (lphc = HeapAlloc(GetProcessHeap(), 0, sizeof(HEADCOMBO))) )
   {
	LPCREATESTRUCTA     lpcs = (CREATESTRUCTA*)lParam;
       
	memset( lphc, 0, sizeof(HEADCOMBO) );
       *(LPHEADCOMBO*)wnd->wExtra = lphc;

       /* some braindead apps do try to use scrollbar/border flags */

	lphc->dwStyle = (lpcs->style & ~(WS_BORDER | WS_HSCROLL | WS_VSCROLL));
	wnd->dwStyle &= ~(WS_BORDER | WS_HSCROLL | WS_VSCROLL);

	if( !(lpcs->style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) )
              lphc->dwStyle |= CBS_HASSTRINGS;
	if( !(wnd->dwExStyle & WS_EX_NOPARENTNOTIFY) )
	      lphc->wState |= CBF_NOTIFY;

	DPRINT( "[0x%08x], style = %08x\n", 
		     (UINT)lphc, lphc->dwStyle );

	return (LRESULT)(UINT)wnd->hwndSelf; 
    }
    return (LRESULT)FALSE;
}

/***********************************************************************
 *           COMBO_NCDestroy
 */
static LRESULT COMBO_NCDestroy( LPHEADCOMBO lphc )
{

   if( lphc )
   {
       WND*		wnd = lphc->self;

       DPRINT("[%04x]: freeing storage\n", CB_HWND(lphc));

       if( (CB_GETTYPE(lphc) != CBS_SIMPLE) && lphc->hWndLBox ) 
   	   DestroyWindow( lphc->hWndLBox );

       HeapFree( GetProcessHeap(), 0, lphc );
       wnd->wExtra[0] = 0;
   }
   return 0;
}

/***********************************************************************
 *           CBGetDefaultTextHeight
 */
static void CBGetDefaultTextHeight( LPHEADCOMBO lphc, LPSIZE lpSize )
{
   if( lphc->editHeight ) /* explicitly set height */
       lpSize->cy = lphc->editHeight;
   else
   {
       HDC    hDC = GetDC( lphc->self->hwndSelf );
       HFONT  hPrevFont = 0;

       if( lphc->hFont ) hPrevFont = SelectObject( hDC, lphc->hFont );

       GetTextExtentPointA( hDC, "0", 1, lpSize);

       lpSize->cy += lpSize->cy / 4 + 4 * SYSMETRICS_CYBORDER;

       if( hPrevFont ) SelectObject( hDC, hPrevFont );
       ReleaseDC( lphc->self->hwndSelf, hDC );
   }
   lpSize->cx = lphc->RectCombo.right - lphc->RectCombo.left;
}


/***********************************************************************
 *           CBCalcPlacement
 *
 * Set up component coordinates given valid lphc->RectCombo.
 */
static void CBCalcPlacement( LPHEADCOMBO lphc, LPRECT lprEdit, 
			     LPRECT lprButton, LPRECT lprLB )
{
   RECT	rect = lphc->RectCombo;
   SIZE	size;

   /* get combo height and width */

   if( CB_OWNERDRAWN(lphc) )
   {
       UINT	u = lphc->RectEdit.bottom - lphc->RectEdit.top;

       if( lphc->wState & CBF_MEASUREITEM ) /* first initialization */
       {
	   MEASUREITEMSTRUCT        mi;

	   /* calculate defaults before sending WM_MEASUREITEM */

	   CBGetDefaultTextHeight( lphc, &size );

	   lphc->wState &= ~CBF_MEASUREITEM;

	   mi.CtlType = ODT_COMBOBOX;
	   mi.CtlID   = lphc->self->wIDmenu;
	   mi.itemID  = -1;
	   mi.itemWidth  = size.cx;
	   mi.itemHeight = size.cy - 6; /* ownerdrawn cb is taller */
	   mi.itemData   = 0;
	   SendMessageA(lphc->owner, WM_MEASUREITEM, 
				(WPARAM)mi.CtlID, (LPARAM)&mi);
	   u = 6 + (UINT)mi.itemHeight;
       }
       else
	   size.cx = rect.right - rect.left;
       size.cy = u;
   }
   else 
       CBGetDefaultTextHeight( lphc, &size );

   /* calculate text and button placement */

   lprEdit->left = lprEdit->top = lprButton->top = 0;
   if( CB_GETTYPE(lphc) == CBS_SIMPLE ) 	/* no button */
       lprButton->left = lprButton->right = lprButton->bottom = 0;
   else
   {
       INT	i = size.cx - CBitWidth - 10;	/* seems ok */

       lprButton->right = size.cx;
       lprButton->left = (INT)i;
       lprButton->bottom = lprButton->top + size.cy;

       if( i < 0 ) size.cx = 0;
       else size.cx = i;
   }

   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
   {
       size.cx -= CBitOffset;
       if( size.cx < 0 ) size.cx = 0;
   }

   lprEdit->right = size.cx; lprEdit->bottom = size.cy;

   /* listbox placement */

   lprLB->left = ( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST ) ? 0 : CBitOffset;
   lprLB->top = lprEdit->bottom - SYSMETRICS_CYBORDER;
   lprLB->right = rect.right - rect.left;
   lprLB->bottom = rect.bottom - rect.top;

   if( lphc->droppedWidth > (lprLB->right - lprLB->left) )
       lprLB->right = lprLB->left + lphc->droppedWidth;

   DPRINT("[%04x]: (%i,%i-%i,%i) placement\n",
		CB_HWND(lphc), lphc->RectCombo.left, lphc->RectCombo.top, 
		lphc->RectCombo.right, lphc->RectCombo.bottom);

   DPRINT("\ttext\t= (%i,%i-%i,%i)\n",
		lprEdit->left, lprEdit->top, lprEdit->right, lprEdit->bottom);

   DPRINT("\tbutton\t= (%i,%i-%i,%i)\n",
		lprButton->left, lprButton->top, lprButton->right, lprButton->bottom);

   DPRINT("\tlbox\t= (%i,%i-%i,%i)\n", 
		lprLB->left, lprLB->top, lprLB->right, lprLB->bottom );
}

/***********************************************************************
 *           CBGetDroppedControlRect
 */
static void CBGetDroppedControlRect( LPHEADCOMBO lphc, LPRECT lpRect)
{
   lpRect->left = lphc->RectCombo.left +
                      (lphc->wState & CBF_EDIT) ? CBitOffset : 0;
   lpRect->top = lphc->RectCombo.top + lphc->RectEdit.bottom -
                                             SYSMETRICS_CYBORDER;
   lpRect->right = lphc->RectCombo.right;
   lpRect->bottom = lphc->RectCombo.bottom - SYSMETRICS_CYBORDER;
}

/***********************************************************************
 *           COMBO_Create
 */
LRESULT COMBO_Create( LPHEADCOMBO lphc, WND* wnd, LPARAM lParam)
{
  static char clbName[] = "ComboLBox";
  static char editName[] = "Edit";

  LPCREATESTRUCT  lpcs = (CREATESTRUCT*)lParam;
  
  if( !CB_GETTYPE(lphc) ) lphc->dwStyle |= CBS_SIMPLE;
  else if( CB_GETTYPE(lphc) != CBS_DROPDOWNLIST ) lphc->wState |= CBF_EDIT;

  lphc->self  = wnd;
  lphc->owner = lpcs->hwndParent;

  /* M$ IE 3.01 actually creates (and rapidly destroys) an ownerless combobox */

  if( lphc->owner || !(lpcs->style & WS_VISIBLE) )
  {
      UINT	lbeStyle;
      RECT	editRect, btnRect, lbRect;

      GetWindowRect( wnd->hwndSelf, &lphc->RectCombo );

      lphc->wState |= CBF_MEASUREITEM;
      CBCalcPlacement( lphc, &editRect, &btnRect, &lbRect );
      lphc->RectButton = btnRect;
      lphc->droppedWidth = lphc->editHeight = 0;

      /* create listbox popup */

      lbeStyle = (LBS_NOTIFY | WS_BORDER | WS_CLIPSIBLINGS) | 
                 (lpcs->style & (WS_VSCROLL | CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE));

      if( lphc->dwStyle & CBS_SORT )
	lbeStyle |= LBS_SORT;
      if( lphc->dwStyle & CBS_HASSTRINGS )
	lbeStyle |= LBS_HASSTRINGS;
      if( lphc->dwStyle & CBS_NOINTEGRALHEIGHT )
	lbeStyle |= LBS_NOINTEGRALHEIGHT;
      if( lphc->dwStyle & CBS_DISABLENOSCROLL )
	lbeStyle |= LBS_DISABLENOSCROLL;
  
      if( CB_GETTYPE(lphc) == CBS_SIMPLE ) 	/* child listbox */
	lbeStyle |= WS_CHILD | WS_VISIBLE;
      else					/* popup listbox */
      {
	lbeStyle |= WS_POPUP;
	OffsetRect( &lbRect, lphc->RectCombo.left, lphc->RectCombo.top );
      }

     /* Dropdown ComboLBox is not a child window and we cannot pass 
      * ID_CB_LISTBOX directly because it will be treated as a menu handle.
      */

      lphc->hWndLBox = CreateWindowExA( 0, clbName, NULL, lbeStyle, 
		        lbRect.left + SYSMETRICS_CXBORDER, 
		        lbRect.top + SYSMETRICS_CYBORDER, 
			lbRect.right - lbRect.left - 2 * SYSMETRICS_CXBORDER, 
			lbRect.bottom - lbRect.top - 2 * SYSMETRICS_CYBORDER, 
			lphc->self->hwndSelf, 
		       (lphc->dwStyle & CBS_DROPDOWN)? (HMENU)0 : (HMENU)ID_CB_LISTBOX,
			lphc->self->hInstance, (LPVOID)lphc );
      if( lphc->hWndLBox )
      {
	  WINBOOL	bEdit = TRUE;
	  lbeStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NOHIDESEL | ES_LEFT;
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
	      lphc->hWndEdit = CreateWindowExA( 0, editName, NULL, lbeStyle,
		  	editRect.left, editRect.top, editRect.right - editRect.left,
			editRect.bottom - editRect.top, lphc->self->hwndSelf, 
			(HMENU)ID_CB_EDIT, lphc->self->hInstance, NULL );
	      if( !lphc->hWndEdit ) bEdit = FALSE;
	  } 

          if( bEdit )
	  {
	      lphc->RectEdit = editRect;
	      if( CB_GETTYPE(lphc) != CBS_SIMPLE )
	      {
		lphc->wState |= CBF_NORESIZE;
		SetWindowPos( wnd->hwndSelf, 0, 0, 0, 
				lphc->RectCombo.right - lphc->RectCombo.left,
				lphc->RectEdit.bottom - lphc->RectEdit.top,
				SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
		lphc->wState &= ~CBF_NORESIZE;
	      }
	      DPRINT("init done\n");
	      return (LRESULT)wnd->hwndSelf;
	  }
	  DPRINT("edit control failure.\n");
      } else DPRINT("listbox failure.\n");
  } else DPRINT("no owner for visible combo.\n");

  /* CreateWindow() will send WM_NCDESTROY to cleanup */

  return -1;
}

/***********************************************************************
 *           CBPaintButton
 *
 * Paint combo button (normal, pressed, and disabled states).
 */
static void CBPaintButton(LPHEADCOMBO lphc, HDC hdc)
{
    RECT 	r;
    UINT 	x, y;
    WINBOOL 	bWINBOOL;
    HDC       hMemDC;
    HBRUSH    hPrevBrush;
    COLORREF    oldTextColor, oldBkColor;

    if( lphc->wState & CBF_NOREDRAW ) return;

    hPrevBrush = SelectObject(hdc, GetSysColorBrush(COLOR_BTNFACE));
    r = lphc->RectButton;

    Rectangle(hdc, r.left, r.top, r.right, r.bottom );
    if( (bWINBOOL = lphc->wState & CBF_BUTTONDOWN) )
    {
	DrawEdge( hdc, &r, EDGE_SUNKEN, BF_RECT );
	OffsetRect( &r, 1, 1 );
    } else {
        r.top++, r.left++;
	DrawEdge( hdc, &r, EDGE_RAISED, BF_RECT );
	r.top--, r.left--;
    }

    InflateRect( &r, -1, -1 );	

    x = (r.left + r.right - CBitWidth) >> 1;
    y = (r.top + r.bottom - CBitHeight) >> 1;

    InflateRect( &r, -3, -3 );

    hMemDC = CreateCompatibleDC( hdc );
    SelectObject( hMemDC, hComboBmp );
    oldTextColor = SetTextColor( hdc, GetSysColor(COLOR_BTNFACE) );
    oldBkColor = SetBkColor( hdc, CB_DISABLED(lphc) ? RGB(128,128,128) :
			       RGB(0,0,0) );
    BitBlt( hdc, x, y, 8, 8, hMemDC, 0, 0, SRCCOPY );
    SetBkColor( hdc, oldBkColor );
    SetTextColor( hdc, oldTextColor );
    DeleteDC( hMemDC );
    SelectObject( hdc, hPrevBrush );
}

/***********************************************************************
 *           CBPaintText
 *
 * Paint CBS_DROPDOWNLIST text field / update edit control contents.
 */
static void CBPaintText(LPHEADCOMBO lphc, HDC hdc)
{
   INT	id, size = 0;
   LPSTR	pText = NULL;

   if( lphc->wState & CBF_NOREDRAW ) return;

   /* follow Windows combobox that sends a bunch of text 
    * inquiries to its listbox while processing WM_PAINT. */

   if( (id = SendMessageA(lphc->hWndLBox, LB_GETCURSEL, 0, 0) ) != LB_ERR )
   {
        size = SendMessageA( lphc->hWndLBox, LB_GETTEXTLEN, id, 0);
        if( (pText = HeapAlloc( GetProcessHeap(), 0, size + 1)) )
	{
	    SendMessageA( lphc->hWndLBox, LB_GETTEXT, (WPARAM)id, (LPARAM)pText );
	    pText[size] = '\0';	/* just in case */
	} else return;
   }

   if( lphc->wState & CBF_EDIT )
   {
	if( CB_HASSTRINGS(lphc) ) SetWindowTextA( lphc->hWndEdit, pText ? pText : "" );
	if( lphc->wState & CBF_FOCUSED ) 
	    SendMessageA( lphc->hWndEdit, EM_SETSEL, 0, (LPARAM)(-1));
   }
   else /* paint text field ourselves */
   {
        HBRUSH hPrevBrush = 0;
	HDC	 hDC = hdc;

	if( !hDC ) 
	{
	    if ((hDC = GetDC(lphc->self->hwndSelf)))
            {
                HBRUSH hBrush = (HBRUSH)SendMessageA( lphc->owner,
                                                  WM_CTLCOLORLISTBOX, 
                                                  (WPARAM)hDC, (LPARAM)lphc->self->hwndSelf );
                hPrevBrush = SelectObject( hDC, 
                           (hBrush) ? hBrush : GetStockObject(WHITE_BRUSH) );
            }
	}
	if( hDC )
	{
	    RECT	rect;
	    UINT	itemState;
	    HFONT	hPrevFont = (lphc->hFont) ? SelectObject(hDC, lphc->hFont) : 0;

	    PatBlt( hDC, (rect.left = lphc->RectEdit.left + SYSMETRICS_CXBORDER),
			   (rect.top = lphc->RectEdit.top + SYSMETRICS_CYBORDER),
			   (rect.right = lphc->RectEdit.right - SYSMETRICS_CXBORDER),
			   (rect.bottom = lphc->RectEdit.bottom - SYSMETRICS_CYBORDER) - 1, PATCOPY );
	    InflateRect( &rect, -1, -1 );

	    if( lphc->wState & CBF_FOCUSED && 
	        !(lphc->wState & CBF_DROPPED) )
	    {
		/* highlight */

		FillRect( hDC, &rect, GetSysColorBrush(COLOR_HIGHLIGHT) );
                SetBkColor( hDC, GetSysColor( COLOR_HIGHLIGHT ) );
                SetTextColor( hDC, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
		itemState = ODS_SELECTED | ODS_FOCUS;
	    } else itemState = 0;

	    if( CB_OWNERDRAWN(lphc) )
	    {
		DRAWITEMSTRUCT dis;

		if( lphc->self->dwStyle & WS_DISABLED ) itemState |= ODS_DISABLED;

		dis.CtlType	= ODT_COMBOBOX;
		dis.CtlID	= lphc->self->wIDmenu;
		dis.hwndItem	= lphc->self->hwndSelf;
		dis.itemAction	= ODA_DRAWENTIRE;
		dis.itemID	= id;
		dis.itemState	= itemState;
		dis.hDC		= hDC;
		dis.rcItem	= rect;
		dis.itemData	= SendMessageA( lphc->hWndLBox, LB_GETITEMDATA, 
						  		  (WPARAM)id, 0 );
		SendMessageA( lphc->owner, WM_DRAWITEM, 
				lphc->self->wIDmenu, (LPARAM)&dis );
	    }
	    else
	    {
		ExtTextOutA( hDC, rect.left + 1, rect.top + 1,
                               ETO_OPAQUE | ETO_CLIPPED, &rect,
                               pText ? pText : "" , size, NULL );
		if(lphc->wState & CBF_FOCUSED && !(lphc->wState & CBF_DROPPED))
		    DrawFocusRect( hDC, &rect );
	    }

	    if( hPrevFont ) SelectObject(hDC, hPrevFont );
	    if( !hdc ) 
	    {
		if( hPrevBrush ) SelectObject( hDC, hPrevBrush );
		ReleaseDC( lphc->self->hwndSelf, hDC );
	    }
	}
   }
   if (pText)
	HeapFree( GetProcessHeap(), 0, pText );
}

/***********************************************************************
 *           COMBO_Paint
 */
static LRESULT COMBO_Paint(LPHEADCOMBO lphc, HDC hParamDC)
{
  PAINTSTRUCT ps;
  HDC 	hDC;
  
  hDC = (hParamDC) ? hParamDC
		   : BeginPaint( lphc->self->hwndSelf, &ps);
  if( hDC && !(lphc->wState & CBF_NOREDRAW) )
  {
      HBRUSH	hPrevBrush, hBkgBrush;

      hBkgBrush = (HBRUSH)SendMessageA( lphc->owner, WM_CTLCOLORLISTBOX,
				  (WPARAM)hDC, (LPARAM)lphc->self->hwndSelf );
      if( !hBkgBrush ) hBkgBrush = GetStockObject(WHITE_BRUSH);

      hPrevBrush = SelectObject( hDC, hBkgBrush );
      if( !IsRectEmpty(&lphc->RectButton) )
      {
	  /* paint everything to the right of the text field */

	  PatBlt( hDC, lphc->RectEdit.right, lphc->RectEdit.top,
			 lphc->RectButton.right - lphc->RectEdit.right,
			 lphc->RectEdit.bottom - lphc->RectEdit.top, PATCOPY );
          CBPaintButton( lphc, hDC );
      }

      if( !(lphc->wState & CBF_EDIT) )
      {
	  /* paint text field */
	  
	  HPEN hPrevPen = SelectObject( hDC, GetSysColorPen(
							  COLOR_WINDOWFRAME) );

	  Rectangle( hDC, lphc->RectEdit.left, lphc->RectEdit.top,
		       lphc->RectEdit.right, lphc->RectButton.bottom );
	  SelectObject( hDC, hPrevPen );
	  CBPaintText( lphc, hDC );
      }
      if( hPrevBrush ) SelectObject( hDC, hPrevBrush );
  }
  if( !hParamDC ) EndPaint(lphc->self->hwndSelf, &ps);
  return 0;
}

/***********************************************************************
 *           CBUpdateLBox
 *
 * Select listbox entry according to the contents of the edit control.
 */
static INT CBUpdateLBox( LPHEADCOMBO lphc )
{
   INT	length, idx, ret;
   LPSTR	pText = NULL;
   
   idx = ret = LB_ERR;
   length = CB_GETEDITTEXTLENGTH( lphc );
 
   if( length > 0 ) 
       pText = (LPSTR) HeapAlloc( GetProcessHeap(), 0, length + 1);

   DPRINT("\t edit text length %i\n", length );

   if( pText )
   {
       if( length ) GetWindowTextA( lphc->hWndEdit, pText, length + 1);
       else pText[0] = '\0';
       idx = SendMessageA( lphc->hWndLBox, LB_FINDSTRING, 
			     (WPARAM)(-1), (LPARAM)pText );
       if( idx == LB_ERR ) idx = 0;	/* select first item */
       else ret = idx;
       HeapFree( GetProcessHeap(), 0, pText );
   }

   /* select entry */

   SendMessageA( lphc->hWndLBox, LB_SETCURSEL, (WPARAM)idx, 0 );
   
   if( idx >= 0 )
   {
       SendMessageA( lphc->hWndLBox, LB_SETTOPINDEX, (WPARAM)idx, 0 );
       /* probably superfluous but Windows sends this too */
       SendMessageA( lphc->hWndLBox, LB_SETCARETINDEX, (WPARAM)idx, 0 );
   }
   return ret;
}

/***********************************************************************
 *           CBUpdateEdit
 *
 * Copy a listbox entry to the edit control.
 */
static void CBUpdateEdit( LPHEADCOMBO lphc , INT index )
{
   INT	length;
   LPSTR	pText = NULL;

   DPRINT("\t %i\n", index );

   if( index == -1 )
   {
       length = CB_GETEDITTEXTLENGTH( lphc );
       if( length )
       {
           if( (pText = (LPSTR) HeapAlloc( GetProcessHeap(), 0, length + 1)) )
           {
 	   	GetWindowTextA( lphc->hWndEdit, pText, length + 1 );
	   	index = SendMessageA( lphc->hWndLBox, LB_FINDSTRING,
				        (WPARAM)(-1), (LPARAM)pText );
	   	HeapFree( GetProcessHeap(), 0, pText );
           }
       }
   }

   if( index >= 0 ) /* got an entry */
   {
       length = SendMessageA( lphc->hWndLBox, LB_GETTEXTLEN, (WPARAM)index, 0);
       if( length )
       {
	   if( (pText = (LPSTR) HeapAlloc( GetProcessHeap(), 0, length + 1)) )
	   {
		SendMessageA( lphc->hWndLBox, LB_GETTEXT, 
				(WPARAM)index, (LPARAM)pText );
		SendMessageA( lphc->hWndEdit, WM_SETTEXT, 0, (LPARAM)pText );
		SendMessageA( lphc->hWndEdit, EM_SETSEL, 0, (LPARAM)(-1) );
		HeapFree( GetProcessHeap(), 0, pText );
	   }
       }
   }
}

/***********************************************************************
 *           CBDropDown
 * 
 * Show listbox popup.
 */
static void CBDropDown( LPHEADCOMBO lphc )
{
   INT	index;
   RECT	rect;
   LPRECT	pRect = NULL;

   DPRINT("[%04x]: drop down\n", CB_HWND(lphc));

   CB_NOTIFY( lphc, CBN_DROPDOWN );

   /* set selection */

   lphc->wState |= CBF_DROPPED;
   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
   {
       index = CBUpdateLBox( lphc );
       if( !(lphc->wState & CBF_CAPTURE) ) CBUpdateEdit( lphc, index );
   }
   else
   {
       index = SendMessageA( lphc->hWndLBox, LB_GETCURSEL, 0, 0 );
       if( index == LB_ERR ) index = 0;
       SendMessageA( lphc->hWndLBox, LB_SETTOPINDEX, (WPARAM)index, 0 );
       SendMessageA( lphc->hWndLBox, LB_CARETON, 0, 0 );
       pRect = &lphc->RectEdit;
   }

   /* now set popup position */

   GetWindowRect( lphc->self->hwndSelf, &rect );
   
   rect.top += lphc->RectEdit.bottom - lphc->RectEdit.top - SYSMETRICS_CYBORDER;
   rect.bottom = rect.top + lphc->RectCombo.bottom - 
			    lphc->RectCombo.top - SYSMETRICS_CYBORDER;
   rect.right = rect.left + lphc->RectCombo.right - lphc->RectCombo.left;
   rect.left += ( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST ) ? 0 : CBitOffset;

   SetWindowPos( lphc->hWndLBox, HWND_TOP, rect.left, rect.top, 
		 rect.right - rect.left, rect.bottom - rect.top, 
		 SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW);

   if( !(lphc->wState & CBF_NOREDRAW) )
       if( pRect )
           RedrawWindow( lphc->self->hwndSelf, pRect, 0, RDW_INVALIDATE | 
			   RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN );
   ShowWindow( lphc->hWndLBox, SW_SHOWNA );
}

/***********************************************************************
 *           CBRollUp
 *
 * Hide listbox popup.
 */
static void CBRollUp( LPHEADCOMBO lphc, WINBOOL ok, WINBOOL bButton )
{
   HWND	hWnd = lphc->self->hwndSelf;

   CB_NOTIFY( lphc, (ok) ? CBN_SELENDOK : CBN_SELENDCANCEL );

   if( IsWindow( hWnd ) && CB_GETTYPE(lphc) != CBS_SIMPLE )
   {

       DPRINT("[%04x]: roll up [%i]\n", CB_HWND(lphc), (INT)ok );

       /* always send WM_LBUTTONUP? */
       SendMessageA( lphc->hWndLBox, WM_LBUTTONUP, 0, (LPARAM)(-1) );

       if( lphc->wState & CBF_DROPPED ) 
       {
	   RECT	rect;

	   lphc->wState &= ~CBF_DROPPED;
	   ShowWindow( lphc->hWndLBox, SW_HIDE );

	   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
	   {
	       INT index = SendMessageA( lphc->hWndLBox, LB_GETCURSEL, 0, 0 );
	       CBUpdateEdit( lphc, index );
	       rect = lphc->RectButton;
	   }
	   else 
           {
	       if( bButton )
	           UnionRect( &rect, &lphc->RectButton,
				       &lphc->RectEdit );
	       else
		   rect = lphc->RectEdit;
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
WINBOOL COMBO_FlipListbox( LPHEADCOMBO lphc, WINBOOL bRedrawButton )
{
   if( lphc->wState & CBF_DROPPED )
   {
       CBRollUp( lphc, TRUE, bRedrawButton );
       return FALSE;
   }

   CBDropDown( lphc );
   return TRUE;
}

/***********************************************************************
 *           COMBO_GetLBWindow
 *
 * Edit control helper.
 */
HWND COMBO_GetLBWindow( WND* pWnd )
{
  LPHEADCOMBO       lphc = CB_GETPTR(pWnd);
  if( lphc ) return lphc->hWndLBox;
  return 0;
}


/***********************************************************************
 *           CBRepaintButton
 */
static void CBRepaintButton( LPHEADCOMBO lphc )
{
   HDC        hDC = GetDC( lphc->self->hwndSelf );

   if( hDC )
   {
       CBPaintButton( lphc, hDC );
       ReleaseDC( lphc->self->hwndSelf, hDC );
   }
}

/***********************************************************************
 *           COMBO_SetFocus
 */
static void COMBO_SetFocus( LPHEADCOMBO lphc )
{
   if( !(lphc->wState & CBF_FOCUSED) )
   {
       if( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST )
           SendMessageA( lphc->hWndLBox, LB_CARETON, 0, 0 );

       if( lphc->wState & CBF_EDIT )
           SendMessageA( lphc->hWndEdit, EM_SETSEL, 0, (LPARAM)(-1) );
       lphc->wState |= CBF_FOCUSED;
       if( !(lphc->wState & CBF_EDIT) ) CBPaintText( lphc, 0 );

       CB_NOTIFY( lphc, CBN_SETFOCUS );
   }
}

/***********************************************************************
 *           COMBO_KillFocus
 */
static void COMBO_KillFocus( LPHEADCOMBO lphc )
{
   HWND	hWnd = lphc->self->hwndSelf;

   if( lphc->wState & CBF_FOCUSED )
   {
       SendMessageA( hWnd, WM_LBUTTONUP, 0, (LPARAM)(-1) );

       CBRollUp( lphc, FALSE, TRUE );
       if( IsWindow( hWnd ) )
       {
           if( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST )
               SendMessageA( lphc->hWndLBox, LB_CARETOFF, 0, 0 );

 	   lphc->wState &= ~CBF_FOCUSED;

           /* redraw text */
           if( lphc->wState & CBF_EDIT )
               SendMessageA( lphc->hWndEdit, EM_SETSEL, (WPARAM)(-1), 0 );
           else CBPaintText( lphc, 0 );

           CB_NOTIFY( lphc, CBN_KILLFOCUS );
       }
   }
}

/***********************************************************************
 *           COMBO_Command
 */
static LRESULT COMBO_Command( LPHEADCOMBO lphc, WPARAM wParam, HWND hWnd )
{
   if( lphc->wState & CBF_EDIT && lphc->hWndEdit == hWnd )
   {
       /* ">> 8" makes gcc generate jump-table instead of cmp ladder */

       switch( HIWORD(wParam) >> 8 )
       {   
	   case (EN_SETFOCUS >> 8):

		DPRINT("[%04x]: edit [%04x] got focus\n", 
			     CB_HWND(lphc), lphc->hWndEdit );

		if( !(lphc->wState & CBF_FOCUSED) ) COMBO_SetFocus( lphc );
	        break;

	   case (EN_KILLFOCUS >> 8):

		DPRINT("[%04x]: edit [%04x] lost focus\n",
			     CB_HWND(lphc), lphc->hWndEdit );

		/* NOTE: it seems that Windows' edit control sends an
		 * undocumented message WM_USER + 0x1B instead of this
		 * notification (only when it happens to be a part of 
		 * the combo). ?? - AK.
		 */

		COMBO_KillFocus( lphc );
		break;


	   case (EN_CHANGE >> 8):
		CB_NOTIFY( lphc, CBN_EDITCHANGE );
		CBUpdateLBox( lphc );
		break;

	   case (EN_UPDATE >> 8):
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

		DPRINT("[%04x]: lbox selection change [%04x]\n", 
			     CB_HWND(lphc), lphc->wState );

		/* do not roll up if selection is being tracked 
		 * by arrowkeys in the dropdown listbox */

		if( (lphc->wState & CBF_DROPPED) && !(lphc->wState & CBF_NOROLLUP) )
		     CBRollUp( lphc, (HIWORD(wParam) == LBN_SELCHANGE), TRUE );
		else lphc->wState &= ~CBF_NOROLLUP;

		CB_NOTIFY( lphc, CBN_SELCHANGE );
		CBPaintText( lphc, 0 );
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
static LRESULT COMBO_ItemOp( LPHEADCOMBO lphc, UINT msg, 
			       WPARAM wParam, LPARAM lParam ) 
{
   HWND	hWnd = lphc->self->hwndSelf;

   DPRINT("[%04x]: ownerdraw op %04x\n", CB_HWND(lphc), msg );

#define lpIS    ((LPDELETEITEMSTRUCT)lParam)

   /* two first items are the same in all 4 structs */
   lpIS->CtlType = ODT_COMBOBOX;
   lpIS->CtlID   = lphc->self->wIDmenu;

   switch( msg )	/* patch window handle */
   {
	case WM_DELETEITEM: 
	     lpIS->hwndItem = hWnd; 
#undef  lpIS
	     break;
	case WM_DRAWITEM: 
#define lpIS    ((LPDRAWITEMSTRUCT)lParam)
	     lpIS->hwndItem = hWnd; 
#undef  lpIS
	     break;
	case WM_COMPAREITEM: 
#define lpIS    ((LPCOMPAREITEMSTRUCT)lParam)
	     lpIS->hwndItem = hWnd; 
#undef  lpIS
	     break;
   }

   return SendMessageA( lphc->owner, msg, lphc->self->wIDmenu, lParam );
}

/***********************************************************************
 *           COMBO_GetText
 */
static LRESULT COMBO_GetText( LPHEADCOMBO lphc, UINT N, LPSTR lpText)
{
   if( lphc->wState & CBF_EDIT )
       return SendMessageA( lphc->hWndEdit, WM_GETTEXT, 
			     (WPARAM)N, (LPARAM)lpText );     

   /* get it from the listbox */

   if( lphc->hWndLBox )
   {
       INT idx = SendMessageA( lphc->hWndLBox, LB_GETCURSEL, 0, 0 );
       if( idx != LB_ERR )
       {
           LPSTR	lpBuffer;
           INT	length = SendMessageA( lphc->hWndLBox, LB_GETTEXTLEN,
					        (WPARAM)idx, 0 );

           /* 'length' is without the terminating character */
           if( length >= N )
	       lpBuffer = (LPSTR) HeapAlloc( GetProcessHeap(), 0, length + 1 );
           else 
	       lpBuffer = lpText;

           if( lpBuffer )
           {
	       INT    n = SendMessageA( lphc->hWndLBox, LB_GETTEXT, 
					   (WPARAM)idx, (LPARAM)lpBuffer );

	       /* truncate if buffer is too short */

	       if( length >= N )
	       {
	       	   if (N && lpText) {
	           if( n != LB_ERR ) memcpy( lpText, lpBuffer, (N>n) ? n+1 : N-1 );
	           lpText[N - 1] = '\0';
                   }
	           HeapFree( GetProcessHeap(), 0, lpBuffer );
	       }
	       return (LRESULT)n;
	   }
       }
   }
   return 0;
}


/***********************************************************************
 *           CBResetPos
 *
 * This function sets window positions according to the updated 
 * component placement struct.
 */
static void CBResetPos( LPHEADCOMBO lphc, LPRECT lbRect, WINBOOL bRedraw )
{
   WINBOOL	bDrop = (CB_GETTYPE(lphc) != CBS_SIMPLE);

   /* NOTE: logs sometimes have WM_LBUTTONUP before a cascade of
    * sizing messages */

   if( lphc->wState & CBF_EDIT )
       SetWindowPos( lphc->hWndEdit, 0, lphc->RectEdit.left, lphc->RectEdit.top,
                       lphc->RectEdit.right - lphc->RectEdit.left,
                       lphc->RectEdit.bottom - lphc->RectEdit.top,
                       SWP_NOZORDER | SWP_NOACTIVATE | ((bDrop) ? SWP_NOREDRAW : 0) );

   if( bDrop )
       OffsetRect( lbRect, lphc->RectCombo.left, lphc->RectCombo.top );

   lbRect->right -= lbRect->left;	/* convert to width */
   lbRect->bottom -= lbRect->top;
   SetWindowPos( lphc->hWndLBox, 0, lbRect->left, lbRect->top,
                   lbRect->right, lbRect->bottom, 
		   SWP_NOACTIVATE | SWP_NOZORDER | ((bDrop) ? SWP_NOREDRAW : 0) );

   if( bDrop )
   {
       if( lphc->wState & CBF_DROPPED )
       {
           lphc->wState &= ~CBF_DROPPED;
           ShowWindow( lphc->hWndLBox, SW_HIDE );
       }

       lphc->wState |= CBF_NORESIZE;
       SetWindowPos( lphc->self->hwndSelf, 0, 0, 0,
                       lphc->RectCombo.right - lphc->RectCombo.left,
                       lphc->RectEdit.bottom - lphc->RectEdit.top,
                       SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW );
       lphc->wState &= ~CBF_NORESIZE;

       if( bRedraw && !(lphc->wState & CBF_NOREDRAW) )
           RedrawWindow( lphc->self->hwndSelf, NULL, 0,
                           RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW );
   }
}


/***********************************************************************
 *           COMBO_Size
 */
static void COMBO_Size( LPHEADCOMBO lphc )
{
  RECT	rect;
  INT		w, h;

  GetWindowRect( lphc->self->hwndSelf, &rect );
  w = rect.right - rect.left; h = rect.bottom - rect.top;

  DPRINT("w = %i, h = %i\n", w, h );

  /* CreateWindow() may send a bogus WM_SIZE, ignore it */

  if( w == (lphc->RectCombo.right - lphc->RectCombo.left) )
  {
      if( (CB_GETTYPE(lphc) == CBS_SIMPLE) &&
	  (h == (lphc->RectCombo.bottom - lphc->RectCombo.top)) )
          return;
      else if( (lphc->dwStyle & CBS_DROPDOWN) &&
	       (h == (lphc->RectEdit.bottom - lphc->RectEdit.top))  )
	       return;
  }
  lphc->RectCombo = rect;
  CBCalcPlacement( lphc, &lphc->RectEdit, &lphc->RectButton, &rect );
  CBResetPos( lphc, &rect, TRUE );
}


/***********************************************************************
 *           COMBO_Font
 */
static void COMBO_Font( LPHEADCOMBO lphc, HFONT hFont, WINBOOL bRedraw )
{
  RECT        rect;

  lphc->hFont = hFont;

  if( lphc->wState & CBF_EDIT )
      SendMessageA( lphc->hWndEdit, WM_SETFONT, (WPARAM)hFont, bRedraw );
  SendMessageA( lphc->hWndLBox, WM_SETFONT, (WPARAM)hFont, bRedraw );

  GetWindowRect( lphc->self->hwndSelf, &rect );
  OffsetRect( &lphc->RectCombo, rect.left - lphc->RectCombo.left,
                                  rect.top - lphc->RectCombo.top );
  CBCalcPlacement( lphc, &lphc->RectEdit,
                         &lphc->RectButton, &rect );
  CBResetPos( lphc, &rect, bRedraw );
}


/***********************************************************************
 *           COMBO_SetItemHeight
 */
static LRESULT COMBO_SetItemHeight( LPHEADCOMBO lphc, INT index, INT height )
{
   LRESULT	lRet = CB_ERR;

   if( index == -1 ) /* set text field height */
   {
       if( height < 768 )
       {
           RECT	rect;

           lphc->editHeight = height;
           GetWindowRect( lphc->self->hwndSelf, &rect );
           OffsetRect( &lphc->RectCombo, rect.left - lphc->RectCombo.left,
				           rect.top - lphc->RectCombo.top );
           CBCalcPlacement( lphc, &lphc->RectEdit,
			          &lphc->RectButton, &rect );
           CBResetPos( lphc, &rect, TRUE );
	   lRet = height;
       }
   } 
   else if ( CB_OWNERDRAWN(lphc) )	/* set listbox item height */
	lRet = SendMessageA( lphc->hWndLBox, LB_SETITEMHEIGHT, 
			      (WPARAM)index, (LPARAM)height );
   return lRet;
}

/***********************************************************************
 *           COMBO_SelectString
 */
static LRESULT COMBO_SelectString( LPHEADCOMBO lphc, INT start, LPCSTR pText )
{
   INT index = SendMessageA( lphc->hWndLBox, LB_SELECTSTRING, 
				 (WPARAM)start, (LPARAM)pText );
   if( index >= 0 )
   {
        if( lphc->wState & CBF_EDIT )
	    CBUpdateEdit( lphc, index );
	else
	    CBPaintText( lphc, 0 );
   }
   return (LRESULT)index;
}

/***********************************************************************
 *           COMBO_LButtonDown
 */
static void COMBO_LButtonDown( LPHEADCOMBO lphc, LPARAM lParam )
{
   POINT     pt = { LOWORD(lParam), HIWORD(lParam) };
   WINBOOL      bButton = PtInRect(&lphc->RectButton, pt);
   HWND      hWnd = lphc->self->hwndSelf;

   if( (CB_GETTYPE(lphc) == CBS_DROPDOWNLIST) ||
       (bButton && (CB_GETTYPE(lphc) == CBS_DROPDOWN)) )
   {
       lphc->wState |= CBF_BUTTONDOWN;
       if( lphc->wState & CBF_DROPPED )
       {
	   /* got a click to cancel selection */

           CBRollUp( lphc, TRUE, FALSE );
	   if( !IsWindow( hWnd ) ) return;

           if( lphc->wState & CBF_CAPTURE )
           {
               lphc->wState &= ~CBF_CAPTURE;
               ReleaseCapture();
           }
           lphc->wState &= ~CBF_BUTTONDOWN;
       }
       else
       {
	   /* drop down the listbox and start tracking */

           lphc->wState |= CBF_CAPTURE;
           CBDropDown( lphc );
           SetCapture( hWnd );
       }
       if( bButton ) CBRepaintButton( lphc );
   }
}

/***********************************************************************
 *           COMBO_LButtonUp
 *
 * Release capture and stop tracking if needed.
 */
static void COMBO_LButtonUp( LPHEADCOMBO lphc, LPARAM lParam )
{
   if( lphc->wState & CBF_CAPTURE )
   {
       lphc->wState &= ~CBF_CAPTURE;
       if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
       {
	   INT index = CBUpdateLBox( lphc );
	   CBUpdateEdit( lphc, index );
       }
       ReleaseCapture();
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
   POINT  pt = { LOWORD(lParam), HIWORD(lParam) };
   RECT   lbRect;

   if( lphc->wState & CBF_BUTTONDOWN )
   {
       WINBOOL	bButton = PtInRect(&lphc->RectButton, pt);

       if( !bButton )
       {
	   lphc->wState &= ~CBF_BUTTONDOWN;
	   CBRepaintButton( lphc );
       }
   }

   GetClientRect( lphc->hWndLBox, &lbRect );
   MapWindowPoints( lphc->self->hwndSelf, lphc->hWndLBox, &pt, 1 );
   if( PtInRect(&lbRect, pt) )
   {
       lphc->wState &= ~CBF_CAPTURE;
       ReleaseCapture();
       if( CB_GETTYPE(lphc) == CBS_DROPDOWN ) CBUpdateLBox( lphc );

       /* hand over pointer tracking */
       SendMessageA( lphc->hWndLBox, WM_LBUTTONDOWN, wParam, lParam );
   }
}


/***********************************************************************
 *           ComboWndProc
 *
 * http://www.microsoft.com/msdn/sdk/platforms/doc/sdk/win/ctrl/src/combobox_15.htm
 */
LRESULT WINAPI ComboWndProc( HWND hwnd, UINT message,
                             WPARAM wParam, LPARAM lParam )
{
    WND*	pWnd = WIN_FindWndPtr(hwnd);
   
    if( pWnd )
    {
      LPHEADCOMBO	lphc = CB_GETPTR(pWnd);

      DPRINT( "[%04x]: msg %s wp %08x lp %08lx\n",
		   pWnd->hwndSelf, SPY_GetMsgName(message), wParam, lParam );

      if( lphc || message == WM_NCCREATE )
      switch(message) 
      {	

	/* System messages */

     	case WM_NCCREATE: 
		return COMBO_NCCreate(pWnd, lParam);

     	case WM_NCDESTROY: 
		COMBO_NCDestroy(lphc);
		break;

     	case WM_CREATE: 
		return COMBO_Create(lphc, pWnd, lParam);

     	case WM_PAINT:
		/* wParam may contain a valid HDC! */
		return COMBO_Paint(lphc, wParam);

	case WM_ERASEBKGND:
		return TRUE;

     	case WM_GETDLGCODE: 
		return (LRESULT)(DLGC_WANTARROWS | DLGC_WANTCHARS);

	case WM_SIZE:
	        if( lphc->hWndLBox && 
		  !(lphc->wState & CBF_NORESIZE) ) COMBO_Size( lphc );
		return TRUE;

	case WM_SETFONT:
		COMBO_Font( lphc, (HFONT)wParam, (WINBOOL)lParam );
		return TRUE;

	case WM_GETFONT:
		return (LRESULT)lphc->hFont;

	case WM_SETFOCUS:
		if( lphc->wState & CBF_EDIT )
		    SetFocus( lphc->hWndEdit );
		else
		    COMBO_SetFocus( lphc );
		return TRUE;

	case WM_KILLFOCUS:
#define hwndFocus ((HWND)wParam)
		if( !hwndFocus ||
		    (hwndFocus != lphc->hWndEdit && hwndFocus != lphc->hWndLBox ))
		    COMBO_KillFocus( lphc );
#undef hwndFocus
		return TRUE;

	case WM_COMMAND:
		return COMBO_Command( lphc, wParam, (HWND)lParam );

	case WM_GETTEXT:
		return COMBO_GetText( lphc, (UINT)wParam, (LPSTR)lParam );

	case WM_SETTEXT:
	case WM_GETTEXTLENGTH:
	case WM_CLEAR:
	case WM_CUT:
        case WM_PASTE:
	case WM_COPY:
		if( lphc->wState & CBF_EDIT )
		    return SendMessageA( lphc->hWndEdit, message, wParam, lParam );
		return CB_ERR;

	case WM_DRAWITEM:
	case WM_DELETEITEM:
	case WM_COMPAREITEM:
	case WM_MEASUREITEM:
		return COMBO_ItemOp( lphc, message, wParam, lParam );

	case WM_ENABLE:
		if( lphc->wState & CBF_EDIT )
		    EnableWindow( lphc->hWndEdit, (WINBOOL)wParam ); 
		EnableWindow( lphc->hWndLBox, (WINBOOL)wParam );
		return TRUE;

	case WM_SETREDRAW:
		if( wParam )
		    lphc->wState &= ~CBF_NOREDRAW;
		else
		    lphc->wState |= CBF_NOREDRAW;

		if( lphc->wState & CBF_EDIT )
		    SendMessageA( lphc->hWndEdit, message, wParam, lParam );
		SendMessageA( lphc->hWndLBox, message, wParam, lParam );
		return 0;
		
	case WM_SYSKEYDOWN:
		if( KEYDATA_ALT & HIWORD(lParam) )
		    if( wParam == VK_UP || wParam == VK_DOWN )
			COMBO_FlipListbox( lphc, TRUE );
		break;

	case WM_CHAR:
	case WM_KEYDOWN:
		if( lphc->wState & CBF_EDIT )
		    return SendMessageA( lphc->hWndEdit, message, wParam, lParam );
		else
		    return SendMessageA( lphc->hWndLBox, message, wParam, lParam );

	case WM_LBUTTONDOWN: 
		if( !(lphc->wState & CBF_FOCUSED) ) SetFocus( lphc->self->hwndSelf );
		if( lphc->wState & CBF_FOCUSED ) COMBO_LButtonDown( lphc, lParam );
		return TRUE;

	case WM_LBUTTONUP:
		COMBO_LButtonUp( lphc, lParam );
		return TRUE;

	case WM_MOUSEMOVE: 
		if( lphc->wState & CBF_CAPTURE ) 
		    COMBO_MouseMove( lphc, wParam, lParam );
		return TRUE;

	/* Combo messages */

	case CB_ADDSTRING:
		return SendMessageA( lphc->hWndLBox, LB_ADDSTRING, 0, lParam);

	case CB_INSERTSTRING:
		return SendMessageA( lphc->hWndLBox, LB_INSERTSTRING, wParam, lParam);

	case CB_DELETESTRING:
		return SendMessageA( lphc->hWndLBox, LB_DELETESTRING, wParam, 0);

	case CB_SELECTSTRING:
		return COMBO_SelectString( lphc, (INT)wParam, (LPSTR)lParam );

	case CB_FINDSTRING:
		return SendMessageA( lphc->hWndLBox, LB_FINDSTRING, wParam, lParam);

	case CB_FINDSTRINGEXACT:
		return SendMessageA( lphc->hWndLBox, LB_FINDSTRINGEXACT, 
						       wParam, lParam );

	case CB_SETITEMHEIGHT:
		return COMBO_SetItemHeight( lphc, (INT)wParam, (INT)lParam);

	case CB_GETITEMHEIGHT:
		if( (INT)wParam >= 0 )	/* listbox item */
		    return SendMessageA( lphc->hWndLBox, LB_GETITEMHEIGHT, wParam, 0);
		return (lphc->RectEdit.bottom - lphc->RectEdit.top);

	case CB_RESETCONTENT:
		SendMessageA( lphc->hWndLBox, LB_RESETCONTENT, 0, 0 );
		CBPaintText( lphc, 0 );
		return TRUE;

	case CB_INITSTORAGE:
		return SendMessageA( lphc->hWndLBox, LB_INITSTORAGE, wParam, lParam);

	case CB_GETHORIZONTALEXTENT:
		return SendMessageA( lphc->hWndLBox, LB_GETHORIZONTALEXTENT, 0, 0);

	case CB_SETHORIZONTALEXTENT:
		return SendMessageA( lphc->hWndLBox, LB_SETHORIZONTALEXTENT, wParam, 0);

	case CB_GETTOPINDEX:
		return SendMessageA( lphc->hWndLBox, LB_GETTOPINDEX, 0, 0);

	case CB_GETLOCALE:
		return SendMessageA( lphc->hWndLBox, LB_GETLOCALE, 0, 0);

	case CB_SETLOCALE:
		return SendMessageA( lphc->hWndLBox, LB_SETLOCALE, wParam, 0);

	case CB_GETDROPPEDWIDTH:
		if( lphc->droppedWidth )
		    return lphc->droppedWidth;
		return lphc->RectCombo.right - lphc->RectCombo.left - 
		           (lphc->wState & CBF_EDIT) ? CBitOffset : 0;

	case CB_SETDROPPEDWIDTH:
		if( (CB_GETTYPE(lphc) != CBS_SIMPLE) &&
		    (INT)wParam < 768 ) lphc->droppedWidth = (INT)wParam;
		return CB_ERR;

	case CB_GETDROPPEDCONTROLRECT:
		if( lParam ) CBGetDroppedControlRect(lphc, (LPRECT)lParam );
		return CB_OKAY;

	case CB_GETDROPPEDSTATE:
		return (lphc->wState & CBF_DROPPED) ? TRUE : FALSE;


	case CB_DIR:
		return COMBO_Directory( lphc, (UINT)wParam, 
				       (LPSTR)lParam, (message == CB_DIR));
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
		return TRUE;

	case CB_GETCOUNT:
		return SendMessageA( lphc->hWndLBox, LB_GETCOUNT, 0, 0);

	case CB_GETCURSEL:
		return SendMessageA( lphc->hWndLBox, LB_GETCURSEL, 0, 0);

	case CB_SETCURSEL:
		lParam = SendMessageA( lphc->hWndLBox, LB_SETCURSEL, wParam, 0);
		if( lphc->wState & CBF_SELCHANGE )
		{
		    /* no LBN_SELCHANGE in this case, update manually */
		   
		    CBPaintText( lphc, 0 );
		    lphc->wState &= ~CBF_SELCHANGE;
		}
	        return lParam;


	case CB_GETLBTEXT:
		return SendMessageA( lphc->hWndLBox, LB_GETTEXT, wParam, lParam);

	case CB_GETLBTEXTLEN:
		return SendMessageA( lphc->hWndLBox, LB_GETTEXTLEN, wParam, 0);

	case CB_GETITEMDATA:
		return SendMessageA( lphc->hWndLBox, LB_GETITEMDATA, wParam, 0);

	case CB_SETITEMDATA:
		return SendMessageA( lphc->hWndLBox, LB_SETITEMDATA, wParam, lParam);

	case CB_GETEDITSEL:
		if( lphc->wState & CBF_EDIT )
		{
		    INT	a, b;

		    return SendMessageA( lphc->hWndEdit, EM_GETSEL,
					   (wParam) ? wParam : (WPARAM)&a,
					   (lParam) ? lParam : (LPARAM)&b );
		}
		return CB_ERR;


	case CB_SETEDITSEL:
		if( lphc->wState & CBF_EDIT ) 
		    return SendMessageA( lphc->hWndEdit, EM_SETSEL, 
			  (INT)(INT)LOWORD(lParam), (INT)(INT)HIWORD(lParam) );
		return CB_ERR;


	case CB_SETEXTENDEDUI:
		if( CB_GETTYPE(lphc) == CBS_SIMPLE ) return CB_ERR;

		if( wParam )
		    lphc->wState |= CBF_EUI;
		else lphc->wState &= ~CBF_EUI;
		return CB_OKAY;


	case CB_GETEXTENDEDUI:
		return (lphc->wState & CBF_EUI) ? TRUE : FALSE;

	case (WM_USER + 0x1B):
	        DPRINT( "[%04x]: undocumented msg!\n", hwnd );
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
  }
  return CB_ERR;
}

