/*
** Toolbar.c
**
** This is it, the incredibly famous toolbar control.  Most of
** the customization stuff is in another file.
*/

#include "ctlspriv.h"
#include "toolbar.h"
#include "image.h"

// these values are defined by the UI gods...
#define DEFAULTBITMAPX 16
#define DEFAULTBITMAPY 15

#define SMALL_DXYBITMAP	16	// new dx dy for sdt images
#define LARGE_DXYBITMAP	24

#define DEFAULTBUTTONX 24
#define DEFAULTBUTTONY 22
// horizontal/vertical space taken up by button chisel, sides,
// and a 1 pixel margin.  used in GrowToolbar.
#define XSLOP 7
#define YSLOP 6

extern HBRUSH g_hbrColorDither;

const int g_dxButtonSep = 8;
const int s_xFirstButton = 0;	// was 8 in 3.1
const int s_dxOverlap = 0;	// was 1 in 3.1

int g_cRefToolbar = 0;

// Globals - since all of these globals are used durring a paint we have to
// take a criticial section around all toolbar paints.  this sucks.
//
HDC g_hdcGlyphs = NULL;		// globals for fast drawing
HDC g_hdcMono = NULL;
HBITMAP g_hbmMono = NULL;

HBITMAP g_hbmDefault = NULL;

HDC     g_hdcButton = NULL;		// contains g_hbmFace (when it exists)
HBITMAP g_hbmFace   = NULL;

int g_dxFace, g_dyFace;	// current dimensions of g_hbmFace (2*g_dxFace)

HDC g_hdcFaceCache = NULL;	// used for button cache



const UINT wStateMasks[] = {
    TBSTATE_ENABLED,
    TBSTATE_CHECKED,
    TBSTATE_PRESSED,
    TBSTATE_HIDDEN,
    TBSTATE_INDETERMINATE
};

LRESULT CALLBACK ToolbarWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
void NEAR PASCAL TBOnButtonStructSize(PTBSTATE ptb, UINT uStructSize);
BOOL NEAR PASCAL SetBitmapSize(PTBSTATE ptb, int width, int height);
int  NEAR PASCAL AddBitmap(PTBSTATE ptb, int nButtons, HINSTANCE hBMInst, UINT wBMID);
BOOL NEAR PASCAL GrowToolbar(PTBSTATE ptb, int newButWidth, int newButHeight, BOOL bInside);

#define HeightWithString(ptb, h) (h + ptb->dyIconFont + 1)

#ifdef IEWIN31_25
#define SEND_WM_COMMAND(hwnd, id, hwndCtl, codeNotify) \
    SendMessage((hwnd), WM_COMMAND, (WPARAM)(int)(id), MAKELPARAM((UINT)(hwndCtl), (codeNotify)))

BOOL FAR PASCAL SendItemNotify(PTBSTATE ptb, int iItem, int code)
{
    // for compatibility with old users of we do this bogus stuff with WM_COMMAND
    // stuffing in an item index in a field that should be an hwnd
    return SEND_WM_COMMAND(ptb->hwndCommand, GetWindowID(ptb->hwnd), (HWND)iItem, code);
}
#endif

BOOL NEAR PASCAL InitGlobalObjects(void)
{
    g_cRefToolbar++;

    if (g_cRefToolbar != 1)
        return TRUE;

    g_hdcGlyphs = CreateCompatibleDC(NULL);
    if (!g_hdcGlyphs)
        return FALSE;
    SetObjectOwner(g_hdcGlyphs, HINST_THISDLL);

    g_hdcMono = CreateCompatibleDC(NULL);
    if (!g_hdcMono)
        return FALSE;
    SetObjectOwner(g_hdcMono, HINST_THISDLL);

    g_hbmMono = CreateMonoBitmap(DEFAULTBUTTONX, DEFAULTBUTTONY);
    if (!g_hbmMono)
        return FALSE;
    SetObjectOwner(g_hbmMono, HINST_THISDLL);

    g_hbmDefault = SelectObject(g_hdcMono, g_hbmMono);

    g_hdcButton = CreateCompatibleDC(NULL);
    if (!g_hdcButton)
        return FALSE;
    SetObjectOwner(g_hdcButton, HINST_THISDLL);

    g_hdcFaceCache = CreateCompatibleDC(NULL);
    if (!g_hdcFaceCache)
        return FALSE;
    SetObjectOwner(g_hdcFaceCache, HINST_THISDLL);

    return TRUE;
}


void NEAR PASCAL TBChangeFont(PTBSTATE ptb, WPARAM wParam)
{

    HFONT hOldFont;
    LOGFONT lf;
    TEXTMETRIC tm;
    int i;
    int newWidth;

    if ((wParam != 0) && (wParam != SPI_SETICONTITLELOGFONT))
        return;

    ENTERCRITICAL;

    // Several of these windows may get this at the same time
    // so we need to process this under the critical section...

    if (!SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0))
        goto Abort;
    hOldFont = ptb->hfontIcon;

    ptb->hfontIcon = CreateFontIndirect(&lf);
    if (!ptb->hfontIcon)
    {
        ptb->hfontIcon = hOldFont;
        goto Abort;
    }

    if (hOldFont)
        DeleteObject(hOldFont);

    hOldFont = SelectObject(g_hdcMono, ptb->hfontIcon);
    GetTextMetrics(g_hdcMono, &tm);
    ptb->dyIconFont = tm.tmHeight;

    if (ptb->nStrings > 0)
    {
        SIZE size;
        PSTR pstr;

        // Now we need to walk through each of the strings to see
        // if we need to resize the toolbar.
        newWidth = ptb->iDxBitmap;
        for (i=ptb->nStrings-1; i>= 0; i--)
        {
            pstr = ptb->pStrings[i];
            GetTextExtentPoint(g_hdcMono, pstr, lstrlen(pstr), &size);
            if (size.cx > newWidth)
                newWidth = size.cx;
        }
        GrowToolbar(ptb, newWidth, HeightWithString(ptb, ptb->iDyBitmap), TRUE);
    }

    if (hOldFont)
	SelectObject(g_hdcMono, hOldFont);

Abort:
    LEAVECRITICAL;
}


BOOL NEAR PASCAL FreeGlobalObjects(void)
{
    g_cRefToolbar--;

    if (g_cRefToolbar != 0)
        return TRUE;

    if (g_hdcMono) {
	if (g_hbmDefault)
	    SelectObject(g_hdcMono, g_hbmDefault);
	DeleteDC(g_hdcMono);		// toast the DCs
	g_hdcMono = NULL;
    }

    if (g_hdcGlyphs) {
	DeleteDC(g_hdcGlyphs);
	g_hdcGlyphs = NULL;
    }

    if (g_hdcFaceCache) {
	DeleteDC(g_hdcFaceCache);
	g_hdcFaceCache = NULL;
    }

    if (g_hdcButton) {
	if (g_hbmDefault)
	    SelectObject(g_hdcButton, g_hbmDefault);
	DeleteDC(g_hdcButton);
	g_hdcButton = NULL;
    }

    if (g_hbmFace) {
	DeleteObject(g_hbmFace);
	g_hbmFace = NULL;
    }

    if (g_hbmMono) {
	DeleteObject(g_hbmMono);
	g_hbmMono = NULL;
    }

}

HWND WINAPI CreateToolbarEx(HWND hwnd, DWORD ws, UINT wID, int nBitmaps,
			HINSTANCE hBMInst, UINT wBMID, LPCTBBUTTON lpButtons,
			int iNumButtons, int dxButton, int dyButton,
			int dxBitmap, int dyBitmap, UINT uStructSize)
{

    HWND hwndToolbar = CreateWindow(c_szToolbarClass, NULL, WS_CHILD | ws,
	      0, 0, 100, 30, hwnd, (HMENU)wID, HINST_THISDLL, NULL);
    if (hwndToolbar)
    {
        PTBSTATE ptb = (PTBSTATE)GetWindowInt(hwndToolbar, 0);
        TBOnButtonStructSize(ptb, uStructSize);

        if ((dxBitmap && dyBitmap && !SetBitmapSize(ptb, dxBitmap, dyBitmap)) ||
            (dxButton && dyButton && !SetBitmapSize(ptb,dxButton, dyButton)))
	{
	    //!!!! do we actually need to deal with this?
	    DestroyWindow(hwndToolbar);
	    hwndToolbar = NULL;
	    goto Error;
	}

        AddBitmap(ptb, nBitmaps, hBMInst, wBMID);
        InsertButtons(ptb, (UINT)-1, iNumButtons, (LPTBBUTTON)lpButtons);

        // ptb may be bogus now after above button insert
    }
Error:
    return hwndToolbar;
}

/* This is no longer declared in COMMCTRL.H.  It only exists for compatibility
** with existing apps; new apps must use CreateToolbarEx.
*/
HWND WINAPI CreateToolbar(HWND hwnd, DWORD ws, UINT wID, int nBitmaps, HINSTANCE hBMInst, UINT wBMID, LPCTBBUTTON lpButtons, int iNumButtons)
{
    // old-style toolbar, so no divider.
    return CreateToolbarEx(hwnd, ws | CCS_NODIVIDER, wID, nBitmaps, hBMInst, wBMID,
    		lpButtons, iNumButtons, 0, 0, 0, 0, sizeof(OLDTBBUTTON));
}

#pragma code_seg(CODESEG_INIT)

BOOL FAR PASCAL InitToolbarClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, c_szToolbarClass, &wc)) {
#ifndef WIN32
#ifndef IEWIN31
	extern LRESULT CALLBACK _ToolbarWndProc(HWND, UINT, WPARAM, LPARAM);
	wc.lpfnWndProc	 = _ToolbarWndProc;
#else
	wc.lpfnWndProc	 = (WNDPROC)ToolbarWndProc;
#endif
#else
	wc.lpfnWndProc	 = (WNDPROC)ToolbarWndProc;
#endif

	wc.lpszClassName = c_szToolbarClass;
	wc.style	 = CS_DBLCLKS | CS_GLOBALCLASS;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = sizeof(PTBSTATE);
	wc.hInstance	 = hInstance;	// use DLL instance if in DLL
	wc.hIcon	 = NULL;
	wc.hCursor	 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	wc.lpszMenuName	 = NULL;

	if (!RegisterClass(&wc))
	    return FALSE;
    }

    return TRUE;
}
#pragma code_seg()

#define BEVEL   2
#define FRAME   1

void NEAR PASCAL PatB(HDC hdc,int x,int y,int dx,int dy, DWORD rgb)
{
    RECT    rc;

    SetBkColor(hdc,rgb);
    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}

void NEAR PASCAL DrawString(PTBSTATE ptb, HDC hdc, int x, int y, int dx, PSTR pszString)
{
    int oldMode;
    DWORD oldTextColor;
    HFONT oldhFont;
    int len;
    SIZE size;

    oldMode = SetBkMode(hdc, TRANSPARENT);
    oldTextColor = SetTextColor(hdc, 0L);
    oldhFont = SelectObject(hdc, ptb->hfontIcon);

    len = lstrlen(pszString);

    GetTextExtentPoint(hdc, pszString, len, &size);
    // center the string horizontally
    x += (dx-size.cx-1)/2;

    TextOut(hdc, x, y, (LPSTR)pszString, len);

    if (oldhFont)
	SelectObject(hdc, oldhFont);
    SetTextColor(hdc, oldTextColor);
    SetBkMode(hdc, oldMode);
}

// create a mono bitmap mask:
//   1's where color == COLOR_BTNFACE || COLOR_3DHILIGHT
//   0's everywhere else

void NEAR PASCAL CreateMask(PTBSTATE ptb, LPTBBUTTON pTBButton, int xoffset, int yoffset, int dx, int dy)
{
    // initalize whole area with 1's
    PatBlt(g_hdcMono, 0, 0, dx, dy, WHITENESS);

    // create mask based on color bitmap
    // convert this to 1's
    SetBkColor(g_hdcGlyphs, g_clrBtnFace);
    BitBlt(g_hdcMono, xoffset, yoffset, ptb->iDxBitmap, ptb->iDyBitmap,
    	g_hdcGlyphs, pTBButton->iBitmap * ptb->iDxBitmap, 0, SRCCOPY);
    // convert this to 1's
    SetBkColor(g_hdcGlyphs, g_clrBtnHighlight);
    // OR in the new 1's
    BitBlt(g_hdcMono, xoffset, yoffset, ptb->iDxBitmap, ptb->iDyBitmap,
    	g_hdcGlyphs, pTBButton->iBitmap * ptb->iDxBitmap, 0, SRCPAINT);

    if (pTBButton->iString != -1 && (pTBButton->iString < ptb->nStrings))
    {
	DrawString(ptb, g_hdcMono, 1, yoffset + ptb->iDyBitmap + 1, dx, ptb->pStrings[pTBButton->iString]);
    }
}


/* Given a button number, the corresponding bitmap is loaded and selected in,
 * and the Window origin set.
 * Returns NULL on Error, 1 if the necessary bitmap is already selected,
 * or the old bitmap otherwise.
 */
HBITMAP FAR PASCAL SelectBM(HDC hDC, PTBSTATE ptb, int nButton)
{
  PTBBMINFO pTemp;
  HBITMAP hRet;
  int nBitmap, nTot;

  for (pTemp=ptb->pBitmaps, nBitmap=0, nTot=0; ; ++pTemp, ++nBitmap)
    {
      if (nBitmap >= ptb->nBitmaps)
	  return(NULL);

      if (nButton < nTot+pTemp->nButtons)
	  break;

      nTot += pTemp->nButtons;
    }

  /* Special case when the required bitmap is already selected
   */
  if (nBitmap == ptb->nSelectedBM)
      return((HBITMAP)1);

  if (!pTemp->hbm || (hRet=SelectObject(hDC, pTemp->hbm))==NULL)
    {
      if (pTemp->hbm)
	  DeleteObject(pTemp->hbm);

      if (pTemp->hInst)
      {
	  pTemp->hbm = CreateMappedBitmap(pTemp->hInst, pTemp->wID, 0, NULL, 0);
      }
      else
	  pTemp->hbm = (HBITMAP)pTemp->wID;

      if (!pTemp->hbm || (hRet=SelectObject(hDC, pTemp->hbm))==NULL)
	  return(NULL);
    }

  ptb->nSelectedBM = nBitmap;
#ifdef WIN32
  SetWindowOrgEx(hDC, nTot * ptb->iDxBitmap, 0, NULL);
#else // WIN32
  SetWindowOrg(hDC, nTot * ptb->iDxBitmap, 0);
#endif

  return(hRet);
}

void FAR PASCAL DrawBlankButton(HDC hdc, int x, int y, int dx, int dy, UINT state)
{
    RECT r1;

    // face color
    PatB(hdc, x, y, dx, dy, g_clrBtnFace);

    r1.left = x;
    r1.top = y;
    r1.right = x + dx;
    r1.bottom = y + dy;

    DrawFrameControl(hdc, &r1, DFC_BUTTON, ((state & TBSTATE_PRESSED) ? (DFCS_BUTTONPUSH | DFCS_PUSHED) : DFCS_BUTTONPUSH));
}

#define DSPDxax	 0x00E20746
#define PSDPxax  0x00B8074A

#define FillBkColor(hdc, prc) ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL)

void NEAR PASCAL DrawFace(PTBSTATE ptb, LPTBBUTTON ptButton, HDC hdc, int x, int y,
			int offx, int offy, int dx)
{
    BitBlt(hdc, x + offx, y + offy, ptb->iDxBitmap, ptb->iDyBitmap,
	    g_hdcGlyphs, ptButton->iBitmap * ptb->iDxBitmap, 0, SRCCOPY);

    if (ptButton->iString != -1 && (ptButton->iString < ptb->nStrings))
    {
	DrawString(ptb, hdc, x + 1, y + offy + ptb->iDyBitmap + 1, dx, ptb->pStrings[ptButton->iString]);
    }
}

void FAR PASCAL DrawButton(HDC hdc, int x, int y, int dx, int dy, PTBSTATE ptb, LPTBBUTTON ptButton, BOOL bFaceCache)
{
    int yOffset;
    HBRUSH hbrOld;
    BOOL bMaskCreated = FALSE;
    BYTE state;
    int xButton = 0;		// assume button is down
    int g_dxFace, g_dyFace;
    int xCenterOffset;

    g_dxFace = dx - 4;
    g_dyFace = dy - 4;

    // make local copy of state and do proper overriding
    state = ptButton->fsState;
    if (state & TBSTATE_INDETERMINATE) {
	if (state & TBSTATE_PRESSED)
	    state &= ~TBSTATE_INDETERMINATE;
	else if (state & TBSTATE_ENABLED)
	    state = TBSTATE_INDETERMINATE;
	else
	    state &= ~TBSTATE_INDETERMINATE;
    }

    // get the proper button look-- up or down.
    if (!(state & (TBSTATE_PRESSED | TBSTATE_CHECKED))) {
	xButton = dx;	// use 'up' version of button
    }
    if (bFaceCache)
	BitBlt(hdc, x, y, dx, dy, g_hdcButton, xButton, 0, SRCCOPY);
    else
	DrawBlankButton(hdc, x, y, dx, dy, state);


    // move coordinates inside border and away from upper left highlight.
    // the extents change accordingly.
    x += 2;
    y += 2;

    if (!SelectBM(g_hdcGlyphs, ptb, ptButton->iBitmap))
	return;

    // calculate offset of face from (x,y).  y is always from the top,
    // so the offset is easy.  x needs to be centered in face.
    yOffset = 1;
    xCenterOffset = (g_dxFace - ptb->iDxBitmap)/2;
    if (state & (TBSTATE_PRESSED | TBSTATE_CHECKED))
    {
	// pressed state moves down and to the right
	xCenterOffset++;
        yOffset++;
    }

    // now put on the face
    if (state & TBSTATE_ENABLED) {

        // regular version
	DrawFace(ptb, ptButton, hdc, x, y, xCenterOffset, yOffset, g_dxFace);
    } else {
        // disabled version (or indeterminate)
	bMaskCreated = TRUE;
	CreateMask(ptb, ptButton, xCenterOffset, yOffset, g_dxFace, g_dyFace);

	SetTextColor(hdc, 0L);	 // 0's in mono -> 0 (for ROP)
	SetBkColor(hdc, 0x00FFFFFF); // 1's in mono -> 1

	// draw glyph's white understrike
	if (!(state & TBSTATE_INDETERMINATE)) {
	    hbrOld = SelectObject(hdc, g_hbrBtnHighlight);
	    if (hbrOld) {
	        // draw hilight color where we have 0's in the mask
                BitBlt(hdc, x + 1, y + 1, g_dxFace, g_dyFace, g_hdcMono, 0, 0, PSDPxax);
	        SelectObject(hdc, hbrOld);
	    }
	}

	// gray out glyph
	hbrOld = SelectObject(hdc, g_hbrBtnShadow);
	if (hbrOld) {
	    // draw the shadow color where we have 0's in the mask
            BitBlt(hdc, x, y, g_dxFace, g_dyFace, g_hdcMono, 0, 0, PSDPxax);
	    SelectObject(hdc, hbrOld);
	}

	if (state & TBSTATE_CHECKED) {
	    BitBlt(g_hdcMono, 1, 1, g_dxFace - 1, g_dyFace - 1, g_hdcMono, 0, 0, SRCAND);
	}
    }

    if (state & (TBSTATE_CHECKED | TBSTATE_INDETERMINATE)) {
        LOGBRUSH lb;

        // BUGBUG, after the problem with craating the dither brush is cleared
        // up, move this init to image InitDitherBrush()
#ifdef WIN32
        HBRUSH hbrGray = (HBRUSH)DefWindowProc(ptb->hwnd, WM_CTLCOLORSCROLLBAR, (WPARAM)hdc, (LPARAM)ptb->hwnd);
#else
        HBRUSH hbrGray = (HBRUSH)DefWindowProc(ptb->hwnd, WM_CTLCOLOR, (WPARAM)hdc,
                MAKELPARAM(ptb->hwnd, CTLCOLOR_SCROLLBAR));
#endif

        GetObject(hbrGray, sizeof(lb), &lb);

        if (lb.lbStyle != BS_SOLID) {
            hbrGray = g_hbrColorDither;
        }

        hbrOld = SelectObject(hdc, hbrGray);
	if (hbrOld) {

	    if (!bMaskCreated)
	        CreateMask(ptb, ptButton, xCenterOffset, yOffset, g_dxFace, g_dyFace);

	    SetTextColor(hdc, 0L);		// 0 -> 0
	    SetBkColor(hdc, 0x00FFFFFF);	// 1 -> 1

	    // only draw the dither brush where the mask is 1's
            BitBlt(hdc, x, y, g_dxFace, g_dyFace, g_hdcMono, 0, 0, DSPDxax);
	
	    SelectObject(hdc, hbrOld);
	}
    }
}

void FAR PASCAL FlushButtonCache(PTBSTATE ptb)
{
    if (ptb->hbmCache) {
	DeleteObject(ptb->hbmCache);
	ptb->hbmCache = 0;
    }
}

// make sure that g_hbmMono is big enough to do masks for this
// size of button.  if not, fail.
BOOL NEAR PASCAL CheckMonoMask(int width, int height)
{
    BITMAP bm;
    HBITMAP hbmTemp;

    GetObject(g_hbmMono, sizeof(BITMAP), &bm);
    if (width > bm.bmWidth || height > bm.bmHeight) {
	hbmTemp = CreateMonoBitmap(width, height);
	if (!hbmTemp)
	    return FALSE;
	SelectObject(g_hdcMono, hbmTemp);
	DeleteObject(g_hbmMono);
	g_hbmMono = hbmTemp;
    }
    return TRUE;
}

/*
** GrowToolbar
**
** Attempt to grow the button size.
**
** The calling function can either specify a new internal measurement
** or a new external measurement.
*/
BOOL NEAR PASCAL GrowToolbar(PTBSTATE ptb, int newButWidth, int newButHeight, BOOL bInside)
{
    if (!newButWidth)
	newButWidth = DEFAULTBUTTONX;
    if (!newButHeight)
	newButHeight = DEFAULTBUTTONY;

    // if growing based on inside measurement, get full size
    if (bInside) {
	newButHeight += YSLOP;
	newButWidth += XSLOP;

	// if toolbar already has strings, don't shrink width it because it
	// might clip room for the string
	if ((newButWidth < ptb->iButWidth) && ptb->nStrings)
	    newButWidth = ptb->iButWidth;
    }
    else {
    	if (newButHeight == -1)
	    newButHeight = ptb->iButHeight;
    	if (newButWidth == -1)
	    newButWidth = ptb->iButWidth;

    	if (newButHeight < ptb->iDyBitmap + YSLOP)
	    newButHeight = ptb->iDyBitmap + YSLOP;
    	if (newButWidth < ptb->iDxBitmap + XSLOP)
	    newButWidth = ptb->iDxBitmap + XSLOP;
    }

    // if the size of the toolbar is actually growing, see if shadow
    // bitmaps can be made sufficiently large.
    if ((newButWidth > ptb->iButWidth) || (newButHeight > ptb->iButHeight)) {
	if (!CheckMonoMask(newButWidth, newButHeight))
	    return(FALSE);
    }

    if (!bInside && (ptb->iButWidth != newButWidth) || (ptb->iButHeight != newButHeight))
        InvalidateRect(ptb->hwnd, NULL, TRUE);

    ptb->iButWidth = newButWidth;
    ptb->iButHeight = newButHeight;

    // bar height has 2 pixels above, 2 below
    ptb->iYPos = 2;

    FlushButtonCache(ptb);
    FlushToolTipsMgr(ptb);

    return TRUE;
}

BOOL NEAR PASCAL SetBitmapSize(PTBSTATE ptb, int width, int height)
{
    int realh;

    if (width == -1)
        width = ptb->iDxBitmap;

    if (height == -1)
        height = ptb->iDyBitmap;

    realh = height;

    if ((ptb->iDxBitmap == width) && (ptb->iDyBitmap == height))
	return TRUE;

    if (ptb->nStrings)
	realh = HeightWithString(ptb, height);

    if (GrowToolbar(ptb, width, realh, TRUE)) {
	ptb->iDxBitmap = width;
	ptb->iDyBitmap = height;
	return TRUE;
    }
    return FALSE;
}

void NEAR PASCAL TB_OnSysColorChange(PTBSTATE ptb)
{
    int i;
    PTBBMINFO pBitmap;

    ReInitGlobalColors();
    ColorDitherBrush_OnSysColorChange();
    //  Reset all of the bitmaps
    for (i = ptb->nBitmaps - 1, pBitmap = ptb->pBitmaps; i >= 0; --i, ++pBitmap)
    {
    	if (pBitmap->hInst && pBitmap->hbm)
    	{
    	    DeleteObject(pBitmap->hbm);
    	    pBitmap->hbm = NULL;
    	}
    }

    FlushButtonCache(ptb);
}

#define CACHE 0x01
#define BUILD 0x02

BOOL NEAR PASCAL ToolbarPaint(PTBSTATE ptb, HDC hdcIn)
{
    RECT rc;
    HDC hdc;
    PAINTSTRUCT ps;
    int iButton, xButton, yButton;
    PTBBUTTON pAllButtons = ptb->Buttons;
    HBITMAP hbmOldGlyphs;
    int xCache = 0;
    UINT wFlags = 0;
    int iCacheWidth = 0;
    HBITMAP hbmTemp;
    BOOL bFaceCache = TRUE;		// assume face cache exists
    BOOL bFlushToolTipsMgr = FALSE;


    GetClientRect(ptb->hwnd, &rc);

    if (hdcIn)
    {
        hdc = hdcIn;
    }
    else
        hdc = BeginPaint(ptb->hwnd, &ps);

    if (!rc.right)
    	goto Error1;

    // setup global stuff for fast painting

    // We need to kick-start the bitmap selection process.
    ptb->nSelectedBM = -1;
    hbmOldGlyphs = SelectBM(g_hdcGlyphs, ptb, 0);
    if (!hbmOldGlyphs)
    	goto Error1;

    yButton   = ptb->iYPos;
    rc.top    = ptb->iYPos;
    rc.bottom = ptb->iYPos + ptb->iButHeight;

    if (!ptb->hbmCache) {
    	// calculate the width of the cache.
    	for (iButton = 0; iButton < ptb->iNumButtons; iButton++)
    	{
    	    if (!(pAllButtons[iButton].fsState & TBSTATE_HIDDEN) &&
    	    	!(pAllButtons[iButton].fsStyle & TBSTYLE_SEP))
    	    	iCacheWidth += ptb->iButWidth;
    	}
    	ptb->hbmCache = CreateCompatibleBitmap(g_hdcGlyphs, iCacheWidth, ptb->iButHeight);
    	wFlags |= BUILD;

    	// if needed, create or enlarge bitmap for pre-building button states
    	if (!(g_hbmFace &&
    		  (ptb->iButWidth <= g_dxFace) &&
    		  (ptb->iButHeight <= g_dyFace)))
    	{
    	    hbmTemp = CreateCompatibleBitmap(g_hdcGlyphs, 2 * ptb->iButWidth, ptb->iButHeight);
    	    if (hbmTemp)
    	    {
    	    	SelectObject(g_hdcButton, hbmTemp);
    	    	if (g_hbmFace)
    	    	    DeleteObject(g_hbmFace);
    	    	g_hbmFace = hbmTemp;
    	    	SetObjectOwner(g_hbmFace, HINST_THISDLL);
    	    	g_dxFace = ptb->iButWidth;
    	    	g_dyFace = ptb->iButHeight;
    	    }
    	    else
    	    	bFaceCache = FALSE;
    	}

	bFlushToolTipsMgr = TRUE;
    }

    if (ptb->hbmCache)
    {
        SelectObject(g_hdcFaceCache, ptb->hbmCache);
	wFlags |= CACHE;
    }
    else
        wFlags = 0;

    if (bFaceCache)
    {
	DrawBlankButton(g_hdcButton, 0, 0,
			ptb->iButWidth, ptb->iButHeight, TBSTATE_PRESSED);
	DrawBlankButton(g_hdcButton, ptb->iButWidth, 0,
			ptb->iButWidth, ptb->iButHeight, 0);
    }

    for (iButton = 0, xButton = s_xFirstButton;
	iButton < ptb->iNumButtons; iButton++)
    {
	PTBBUTTON pButton = &pAllButtons[iButton];

	if (!(pButton->fsState & TBSTATE_HIDDEN))
	{
	    if (pButton->fsStyle & TBSTYLE_SEP)
	    {
	    	xButton += pButton->iBitmap;
	    }
	    else
	    {
	    	if (wFlags & BUILD) {
                    BYTE bStateOld = pButton->fsState;
                    pButton->fsState &= ~TBSTATE_PRESSED;
    		    DrawButton(g_hdcFaceCache, xCache, 0, ptb->iButWidth, ptb->iButHeight, ptb, pButton, bFaceCache);
                    pButton->fsState = bStateOld;
                }

		rc.left = xButton;
                rc.right = xButton + ptb->iButWidth;
		if (RectVisible(hdc, &rc))
		{
		    if ((wFlags & CACHE) && !(pButton->fsState & TBSTATE_PRESSED))
			BitBlt(hdc, xButton, yButton, ptb->iButWidth, ptb->iButHeight, g_hdcFaceCache, xCache, 0, SRCCOPY);
		    else
		    	DrawButton(hdc, xButton, yButton, ptb->iButWidth, ptb->iButHeight, ptb, pButton, bFaceCache);
		}

		// advance the "pointer" in the cache
		xCache += ptb->iButWidth;
		xButton += (ptb->iButWidth - s_dxOverlap);
	    }

	    if (pButton->fsState & TBSTATE_WRAP)
	    {
	    	int dy;

	    	if (pButton->fsStyle & TBSTYLE_SEP)
	    	    dy = ptb->iButHeight + pButton->iBitmap * 2 / 3;
	    	else
	    	    dy = ptb->iButHeight;
	    	xButton = s_xFirstButton;
	    	yButton   += dy;
	    	rc.top    += dy;
	    	rc.bottom += dy;
	    }
        }
    }

    if (wFlags & CACHE)
    	SelectObject(g_hdcFaceCache, g_hbmDefault);
    SelectObject(g_hdcGlyphs, hbmOldGlyphs);

Error1:
    if (hdcIn == NULL)
        EndPaint(ptb->hwnd, &ps);

    return bFlushToolTipsMgr;
}


BOOL NEAR PASCAL GetItemRect(PTBSTATE ptb, UINT uButton, LPRECT lpRect)
{
    UINT iButton, xPos, yPos;
    PTBBUTTON pButton;

    if (uButton >= (UINT)ptb->iNumButtons
    	|| (ptb->Buttons[uButton].fsState & TBSTATE_HIDDEN))
    {
    	return FALSE;
    }

    xPos = s_xFirstButton;
    yPos = ptb->iYPos;

    for (iButton = 0, pButton = ptb->Buttons; iButton < uButton; iButton++, pButton++)
    {
	if (!(pButton->fsState & TBSTATE_HIDDEN))
	{
	    if (pButton->fsStyle & TBSTYLE_SEP)
	    	xPos += pButton->iBitmap;
	    else
	    	xPos += (ptb->iButWidth - s_dxOverlap);

	    if (pButton->fsState & TBSTATE_WRAP)
	    {
	    	if (pButton->fsStyle & TBSTYLE_SEP)
	    	    yPos += ptb->iButHeight + pButton->iBitmap * 2 / 3;
	    	else
	    	    yPos += ptb->iButHeight;
	    	xPos = s_xFirstButton;
	    }
	}
    }

    // pButton should now point at the required button, and xPos should be
    // its left edge.  Note that we already checked if the button was hidden above

    lpRect->left   = xPos;
    lpRect->right  = xPos + (pButton->fsStyle & TBSTYLE_SEP ? pButton->iBitmap : ptb->iButWidth);
    lpRect->top    = yPos;
    lpRect->bottom = yPos + ptb->iButHeight;

    return TRUE;
}


void NEAR PASCAL InvalidateButton(PTBSTATE ptb, PTBBUTTON pButtonToPaint)
{
    RECT rc;

    if (GetItemRect(ptb, pButtonToPaint-ptb->Buttons, &rc))
    {
    	InvalidateRect(ptb->hwnd, &rc, FALSE);
    }
}

// do hit testing by sliding the origin of the supplied point
//
// returns:
//	>= 0	index of non sperator item hit
//	< 0	index of seperator or nearest non seperator item (area just below and to the left)
//
// +--------------------------------------
// |	  -1	-1    -1    -1
// |	  btn   sep   btn
// |    +-----+     +-----+
// |    |     |     |     |
// | -1 |  0  | -1  |  2  | -3
// |    |     |     |     |
// |    +-----+     +-----+
// |
// | -1	  -1	-1    -2    -3
//

int FAR PASCAL TBHitTest(PTBSTATE ptb, int xPos, int yPos)
{
	int prev = 0;
	int last = 0;
	int i;
	RECT rc;

        if (ptb->iNumButtons == 0)
            return(-1);

	for (i=0; i<ptb->iNumButtons; i++)
	{
		if (GetItemRect(ptb, i, &rc))
		{
			if (yPos >= rc.top && yPos <= rc.bottom)
			{
				if (xPos >= rc.left && xPos <= rc.right)
				{
					if (ptb->Buttons[i].fsStyle & TBSTYLE_SEP)
						return - i - 1;
					else
						return i;
				}
				else
				{
					prev = i + 1;
				}
			}
			else
			{
				last = i;
			}
		}
	}

	if (prev)
		return -1 - prev;
	return last + 1;
	
#if 0
    PTBBUTTON pBtnT, pBtnLast;
    int iButton, xOriginal;
    PTBBUTTON pButton;

    xPos -= s_xFirstButton;
    if (xPos < 0)
        return -1;

    yPos -= ptb->iYPos;
    if (yPos < 0)
        return -1;

    xOriginal = xPos;
    for (iButton = 0, pButton = ptb->Buttons; iButton < ptb->iNumButtons; iButton++, pButton++)
    {
        if (!(pButton->fsState & TBSTATE_HIDDEN))
	{
            if (pButton->fsStyle & TBSTYLE_SEP)
	        xPos -= pButton->iBitmap;
            else
	        xPos -= (ptb->iButWidth - s_dxOverlap);

    	    if (pButton->fsState & TBSTATE_WRAP)
	    {
	        xPos = xOriginal;
	        yPos -= ptb->iYPos + ptb->iButHeight;
	    }

            if (xPos < 0)
	    {
	        if (yPos < (ptb->iYPos + ptb->iButHeight))
	        {
	            if (pButton->fsStyle & TBSTYLE_SEP)
	                break;

	            if (yPos < ptb->iButHeight)
	                return iButton;
		    else
		        break;
	        }
            }
	}
    }

    return -1 - iButton;
#endif
}

int NEAR PASCAL CountRows(PTBSTATE ptb)
{
    PTBBUTTON pButton, pBtnLast;
    int rows = 1;

    pBtnLast = &(ptb->Buttons[ptb->iNumButtons]);
    for (pButton = ptb->Buttons; pButton<pBtnLast; pButton++) {
        if (pButton->fsState & TBSTATE_WRAP) {
            rows++;
            if (pButton->fsStyle & TBSTYLE_SEP)
                rows++;
        }
    }
    return rows;
}

/**** WrapToolbar:
 * The buttons in the toolbar is layed out from left to right,
 * top to bottom. If adding another button to the current row,
 * while computing the layout, would cause that button to extend
 * beyond the right edge or the client area, then locate a break-
 * point (marked with the TBSTATE_WRAP flag). A break-point is:
 *
 * a) The right-most separator on the current row.
 *
 * b) The right-most button if there is no separator on the current row.
 *
 * A new row is also started at the end of any button group (sequence
 * of buttons that are dlimited by separators) that are taller than
 * or equal to two rows.
 */

void NEAR PASCAL WrapToolbar(PTBSTATE ptb, int dx, LPRECT lpRect, int FAR *pRows)
{
	PTBBUTTON pButton, pBtnT, pBtnLast;
	int xPos, yPos, xMax;
	BOOL bFoundIt;
	BOOL bNextBreak = FALSE;

	xMax = ptb->iButWidth;
	xPos = s_xFirstButton;
	yPos = ptb->iYPos;
	pBtnLast = &(ptb->Buttons[ptb->iNumButtons]);


	if (pRows)
		(*pRows)=1;

	for (pButton = ptb->Buttons; pButton<pBtnLast; pButton++)
	{
		pButton->fsState &= ~TBSTATE_WRAP;

		if (!(pButton->fsState & TBSTATE_HIDDEN))
		{
			if (pButton->fsStyle & TBSTYLE_SEP)
				xPos += pButton->iBitmap;
			else
				xPos += (ptb->iButWidth - s_dxOverlap);

			// The current row exceeds the right edge. Wrap it.
			if (!(pButton->fsStyle&TBSTYLE_SEP) && (xPos > dx)) {

				for (pBtnT=pButton, bFoundIt = FALSE;
				pBtnT>ptb->Buttons && !(pBtnT->fsState & TBSTATE_WRAP);
				pBtnT--)
				{
					if ((pBtnT->fsStyle & TBSTYLE_SEP) &&
						 !(pBtnT->fsState & TBSTATE_HIDDEN))
					{
						pBtnT->fsState |= TBSTATE_WRAP;
						xPos = s_xFirstButton;
						yPos += pBtnT->iBitmap * 2 / 3 + ptb->iButHeight;
						bFoundIt = TRUE;
						pButton = pBtnT;
						bNextBreak = FALSE;
						if (pRows)
							(*pRows)++;
						break;
					}
				}

				// Did we find a separator? Force a wrap anyway!
				if (bFoundIt==FALSE)
				{
					pBtnT = pButton;
					if (pButton!=ptb->Buttons) {
						/* Back-up to first non-hidden button. */
						do {
							pBtnT--;
						} while ((pBtnT>ptb->Buttons) &&
									(pBtnT->fsState & TBSTATE_HIDDEN));

						/* Is it already wrapped? */
						if (pBtnT->fsState & TBSTATE_WRAP)
							pBtnT = pButton;
					}
						

					pBtnT->fsState |= TBSTATE_WRAP;
					xPos = s_xFirstButton;
					yPos += ptb->iButHeight;
					pButton = pBtnT;
					bNextBreak = TRUE;
				}

				// Count another row.
				if (pRows)
					(*pRows)++;
			}
			else
			{
				pButton->fsState &= ~TBSTATE_WRAP;


				if ((pButton->fsStyle&TBSTYLE_SEP) && (bNextBreak))
				{
                                        bNextBreak = FALSE;
					pButton->fsState |= TBSTATE_WRAP;
					xPos = s_xFirstButton;
					yPos += ptb->iButHeight + pButton->iBitmap * 2 / 3 ;
					if (pRows)
						(*pRows)+=2;
				}
			}

			if (!(pButton->fsStyle&TBSTYLE_SEP))
				xMax = max(xPos, xMax);
		}
	}

	if (lpRect)
	{
		lpRect->left = 0;
		lpRect->right = xMax;
		lpRect->top = 0;
		lpRect->bottom = yPos + ptb->iYPos + ptb->iButHeight;
	}
}



BOOL NEAR PASCAL BoxIt(PTBSTATE ptb, int height, BOOL fLarger, LPRECT lpRect)
{
	int dx, bwidth;
	int rows, prevRows, prevWidth;
	RECT rcCur;
	
	if (height<1)
		height = 1;

	rows = CountRows(ptb);
	if (height==rows || ptb->iNumButtons==0)
	{
	    GetClientRect(ptb->hwnd, lpRect);
	    return FALSE;
	}

 	bwidth = ptb->iButWidth-s_dxOverlap;
	prevRows = ptb->iNumButtons+1;
	prevWidth = bwidth;
	for (rows=height+1, dx = bwidth; rows>height;dx+=bwidth/4)
	{
		WrapToolbar(ptb, dx, &rcCur, &rows);
		if (rows<prevRows && rows>height)
		{
			prevWidth = dx;
			prevRows = rows;
		}
	}

	if (rows<height && fLarger)
	{
		WrapToolbar(ptb, prevWidth, &rcCur, NULL);
	}

	if (lpRect)
		*lpRect = rcCur;

	return TRUE;
}


int FAR PASCAL PositionFromID(PTBSTATE ptb, int id)
{
    int i;

    // Handle case where this is sent at the wrong time..
    if (ptb == NULL)
        return -1;

    // note, we don't skip seperators, so you better not have conflicting
    // cmd ids and seperator ids.
    for (i = 0; i < ptb->iNumButtons; i++)
        if (ptb->Buttons[i].idCommand == id)
	    return i;		// position found

    return -1;		// ID not found!
}

// check a radio button by button index.
// the button matching idCommand was just pressed down.  this forces
// up all other buttons in the group.
// this does not work with buttons that are forced up with

void NEAR PASCAL MakeGroupConsistant(PTBSTATE ptb, int idCommand)
{
    int i, iFirst, iLast, iButton;
    int cButtons = ptb->iNumButtons;
    PTBBUTTON pAllButtons = ptb->Buttons;

    iButton = PositionFromID(ptb, idCommand);

    if (iButton < 0)
        return;

    // assertion

//    if (!(pAllButtons[iButton].fsStyle & TBSTYLE_CHECK))
//	return;

    // did the pressed button just go down?
    if (!(pAllButtons[iButton].fsState & TBSTATE_CHECKED))
        return;         // no, can't do anything

    // find the limits of this radio group

    for (iFirst = iButton; (iFirst > 0) && (pAllButtons[iFirst].fsStyle & TBSTYLE_GROUP); iFirst--)
        if (!(pAllButtons[iFirst].fsStyle & TBSTYLE_GROUP))
            iFirst++;

    cButtons--;
    for (iLast = iButton; (iLast < cButtons) && (pAllButtons[iLast].fsStyle & TBSTYLE_GROUP); iLast++);

    if (!(pAllButtons[iLast].fsStyle & TBSTYLE_GROUP))
        iLast--;

    // search for the currently down button and pop it up
    for (i = iFirst; i <= iLast; i++) {
        if (i != iButton) {
            // is this button down?
            if (pAllButtons[i].fsState & TBSTATE_CHECKED) {
	        pAllButtons[i].fsState &= ~TBSTATE_CHECKED;     // pop it up
                InvalidateButton(ptb, &pAllButtons[i]);
                break;          // only one button is down right?
            }
        }
    }
}

void NEAR PASCAL DestroyStrings(PTBSTATE ptb)
{
    PSTR *p;
    PSTR end = 0, start = 0;
    int i;

    p = ptb->pStrings;
    for (i = 0; i < ptb->nStrings; i++) {
	if (!(*p < end) && (*p > start)) {
	    start = (*p);
	    end = start + LocalSize((HANDLE)*p);
	    LocalFree((HANDLE)*p);
	}
	p++;
    }

    LocalFree((HANDLE)ptb->pStrings);
}

#define MAXSTRINGSIZE 1024
int NEAR PASCAL AddStrings(PTBSTATE ptb, WPARAM wParam, LPARAM lParam)
{
    int i;
    HFONT hOldFont;
    LPSTR lpsz;
    PSTR  pString, psz;
    int numstr;
    PSTR *pFoo;
    PSTR *pOffset;
    char cSeparator;
    int len;
    int newWidth;

    // read the string as a resource
    if (wParam != 0) {
	pString = (PSTR)LocalAlloc(LPTR, MAXSTRINGSIZE);
	if (!pString)
	    return -1;
	i = LoadString((HINSTANCE)wParam, LOWORD(lParam), (LPSTR)pString, MAXSTRINGSIZE);
	if (!i) {
	    LocalFree(pString);
	    return -1;
	}
	// realloc string buffer to actual needed size
	LocalReAlloc(pString, (i + 1) * sizeof(TCHAR), LMEM_MOVEABLE);

	// convert separators to '\0' and count number of strings
	cSeparator = *pString;
#ifdef DBCS
	for (numstr = 0, psz = pString + 1, i--; i; i--, psz = AnsiNext(psz)) {
	    {
	      // extra i-- if DBCS
	      if (AnsiPrev(pString, psz)==(psz-2))
	        	  i--;
                if (*psz == cSeparator) {
                    if (i != 1) // We don't want to count the second terminator as another string
	        	numstr++;
		  *psz = 0;	// terminate with 0
	      }
	    }
#else
	for (numstr = 0, psz = pString + 1, i--; i; i--, psz++) {
	    if (*psz == cSeparator) {
                if (i != 1) // We don't want to count the second terminator as another string
		numstr++;
		*psz = 0;	// terminate with 0
	    }
#endif
	    // shift string to the left to overwrite separator identifier
	    *(psz - 1) = *psz;
	}
    }
    // read explicit string.  copy it into local memory, too.
    else {
	// find total length and number of strings
	for (i = 0, numstr = 0, lpsz = (LPSTR)lParam;;) {
	    i++;
	    if (*lpsz == 0) {
		numstr++;
		if (*(lpsz + 1) == 0)
		    break;
	    }
	    lpsz++;
	}
	pString = (PSTR)LocalAlloc(LPTR, i);
	if (!pString)
	    return -1;
	hmemcpy(pString, (void FAR *)lParam, i);
    }

    // make room for increased string pointer table
    if (ptb->pStrings)
	pFoo = (PSTR *)LocalReAlloc(ptb->pStrings,
		(ptb->nStrings + numstr) * sizeof(PSTR), LMEM_MOVEABLE);
    else
	pFoo = (PSTR *)LocalAlloc(LPTR, numstr * sizeof(PSTR));
    if (!pFoo) {
	LocalFree(pString);
	return -1;
    }

    ptb->pStrings = pFoo;
    // pointer to next open slot in string index table.
    pOffset = ptb->pStrings + ptb->nStrings;

    hOldFont = SelectObject(g_hdcMono, ptb->hfontIcon);
    // fix up string pointer table to deal with the new strings.
    // check if any string is big enough to necessitate a wider button.
    newWidth = ptb->iDxBitmap;
    for (i = 0; i < numstr; i++, pOffset++)
    {
        SIZE size;
	*pOffset = pString;

	len = lstrlen(pString);
        GetTextExtentPoint(g_hdcMono, pString, len, &size);

	if (size.cx > newWidth)
	    newWidth = size.cx;
	pString += len + 1;
    }
    if (hOldFont)
	SelectObject(g_hdcMono, hOldFont);

    // is the world big enough to handle the larger buttons?
    if (!GrowToolbar(ptb, newWidth, HeightWithString(ptb, ptb->iDyBitmap), TRUE))
    {
	// back out changes.
	if (ptb->nStrings == 0) {
	    LocalFree(ptb->pStrings);
	    ptb->pStrings = 0;
	}
	else
	    ptb->pStrings = (PSTR *)LocalReAlloc(ptb->pStrings,
	    		ptb->nStrings * sizeof(PSTR), LMEM_MOVEABLE);
	LocalFree(pString);
	return -1;
    }

    i = ptb->nStrings;
    ptb->nStrings += numstr;
    return i;				// index of first added string
}

#ifdef WIN32
void NEAR PASCAL MapToStandardBitmaps(HINSTANCE FAR *phinst, UINT FAR * pidBM, int FAR *pnButtons)
{
    if (*phinst == HINST_COMMCTRL) {
        *phinst = g_hinst;

        // low 2 bits are coded M(mono == ~color) L(large == ~small)
        //  0 0	-> color small
        //  0 1	-> color large
        //  ...
        //  1 1	-> mono  large

        switch (*pidBM)
        {
        case IDB_STD_SMALL_COLOR:
        case IDB_STD_LARGE_COLOR:
        case IDB_STD_SMALL_MONO:
        case IDB_STD_LARGE_MONO:
            *pidBM = IDB_STDTB_SMALL_COLOR + (*pidBM & 1);
            *pnButtons = STD_PRINT + 1;
            break;

        case IDB_VIEW_SMALL_COLOR:
        case IDB_VIEW_LARGE_COLOR:
        case IDB_VIEW_SMALL_MONO:
        case IDB_VIEW_LARGE_MONO:
            *pidBM = IDB_VIEWTB_SMALL_COLOR + (*pidBM & 1);
            *pnButtons = VIEW_NEWFOLDER + 1;
            break;
        }
    }
}
#endif

/* Adds a new bitmap to the list of BMs available for this toolbar.
 * Returns the index of the first button in the bitmap or -1 if there
 * was an error.
 */
int NEAR PASCAL AddBitmap(PTBSTATE ptb, int nButtons, HINSTANCE hBMInst, UINT idBM)
{
  PTBBMINFO pTemp;
  int nBM, nIndex;

  // map things to the standard toolbar images
#ifdef WIN32
  if (hBMInst == HINST_COMMCTRL)	// -1
  {
      // set the proper dimensions...
      if (idBM & 1)
          SetBitmapSize(ptb, LARGE_DXYBITMAP, LARGE_DXYBITMAP);
      else
          SetBitmapSize(ptb, SMALL_DXYBITMAP, SMALL_DXYBITMAP);

      MapToStandardBitmaps(&hBMInst, &idBM, &nButtons);
  }
#endif

  if (ptb->pBitmaps)
    {
      /* Check if the bitmap has already been added
       */
      for (nBM=ptb->nBitmaps, pTemp=ptb->pBitmaps, nIndex=0;
	    nBM>0; --nBM, ++pTemp)
	{
	  if (pTemp->hInst==hBMInst && pTemp->wID==idBM)
	    {
	      /* We already have this bitmap, but have we "registered" all
	       * the buttons in it?
	       */
	      if (pTemp->nButtons >= nButtons)
		  return(nIndex);
	      if (nBM == 1)
		{
		  /* If this is the last bitmap, we can easily increase the
		   * number of buttons without messing anything up.
		   */
		  pTemp->nButtons = nButtons;
		  return(nIndex);
		}
	    }

	  nIndex += pTemp->nButtons;
	}

      pTemp = (PTBBMINFO)LocalReAlloc(ptb->pBitmaps,
	    (ptb->nBitmaps + 1)*sizeof(TBBMINFO), LMEM_MOVEABLE);
      if (!pTemp)
	  return(-1);
      ptb->pBitmaps = pTemp;
    }
  else
    {
      ptb->pBitmaps = (PTBBMINFO)LocalAlloc(LPTR, sizeof(TBBMINFO));
      if (!ptb->pBitmaps)
	  return(-1);
    }

  pTemp = ptb->pBitmaps + ptb->nBitmaps;

  pTemp->hInst = hBMInst;
  pTemp->wID = idBM;
  pTemp->nButtons = nButtons;
  pTemp->hbm = NULL;

  ++ptb->nBitmaps;

  for (nButtons=0, --pTemp; pTemp>=ptb->pBitmaps; --pTemp)
      nButtons += pTemp->nButtons;

  return(nButtons);
}

BOOL NEAR PASCAL ReplaceBitmap(PTBSTATE ptb, LPTBREPLACEBITMAP lprb)
{
    int nBM;
    PTBBMINFO pTemp;

#ifdef WIN32
    int iTemp;

    MapToStandardBitmaps(&lprb->hInstOld, &lprb->nIDOld, &iTemp);
    MapToStandardBitmaps(&lprb->hInstNew, &lprb->nIDNew, &lprb->nButtons);
#endif

    for (nBM=ptb->nBitmaps, pTemp=ptb->pBitmaps;
         nBM>0; --nBM, ++pTemp)
    {
        if (pTemp->hInst==lprb->hInstOld && pTemp->wID==lprb->nIDOld)
        {
            // number of buttons must match
            if (pTemp->hInst && pTemp->hbm)
                DeleteObject(pTemp->hbm);
            pTemp->hbm = NULL;
            pTemp->hInst = lprb->hInstNew;
            pTemp->wID = lprb->nIDNew;
            pTemp->nButtons = lprb->nButtons;
            FlushButtonCache(ptb);
            return TRUE;
        }
    }
    return FALSE;
}


void FAR PASCAL FlushToolTipsMgr(PTBSTATE ptb) {

    // change all the rects for the tool tips mgr.  this is
    // cheap, and we don't do it often, so go ahead
    // and do them all.
    if(ptb->hwndToolTips) {
	UINT i;
	TOOLINFO ti;
	PTBBUTTON pButton;
	
        ti.cbSize = sizeof(ti);
	ti.hwnd = ptb->hwnd;
        ti.lpszText = LPSTR_TEXTCALLBACK;
	for ( i = 0, pButton = ptb->Buttons;
	     i < (UINT)ptb->iNumButtons;
	     i++, pButton++) {

            if (!(pButton->fsStyle & TBSTYLE_SEP)) {
                ti.uId = pButton->idCommand;
                if (!GetItemRect(ptb, i, &ti.rect))
                    ti.rect.left = ti.rect.right = ti.rect.top = ti.rect.bottom = 0;

                SendMessage(ptb->hwndToolTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
            }
	}
    }
}

BOOL FAR PASCAL InsertButtons(PTBSTATE ptb, UINT uWhere, UINT uButtons, LPTBBUTTON lpButtons)
{
    PTBBUTTON pIn, pOut;
    PTBSTATE ptbNew;

    // comments by chee (not the original author) so they not be
    // exactly right... be warned.

    if (!ptb || !ptb->uStructSize)
	return FALSE;

    // enlarge the main structure
    ptbNew = (PTBSTATE)LocalReAlloc(ptb, sizeof(TBSTATE) - sizeof(TBBUTTON)
				      + (ptb->iNumButtons + uButtons) * sizeof(TBBUTTON),
									LMEM_MOVEABLE);
    if (!ptbNew)
	return FALSE;

#ifdef DEBUG
    if (ptbNew != ptb)
	DebugMsg(DM_TRACE, "InsertButtons caused the ptb to change!");
#endif
    ptb = ptbNew;

    SetWindowInt(ptb->hwnd, 0, (int)ptb);

    // if where points beyond the end, set it at the end
    if (uWhere > (UINT)ptb->iNumButtons)
	uWhere = ptb->iNumButtons;

    // move buttons above uWhere up uButton spaces
    // the uWhere gets inverted and counts to zero..
    for (pIn=ptb->Buttons+ptb->iNumButtons-1, pOut=pIn+uButtons,
	 uWhere=(UINT)ptb->iNumButtons-uWhere; uWhere>0;
	 --pIn, --pOut, --uWhere)
	*pOut = *pIn;

    // now do the copy.
    for (lpButtons=(LPTBBUTTON)((LPSTR)lpButtons+ptb->uStructSize*(uButtons-1)),
	 ptb->iNumButtons+=(int)uButtons;  // init
	 uButtons>0; //test
	 --pOut, lpButtons=(LPTBBUTTON)((LPSTR)lpButtons-ptb->uStructSize), --uButtons)
    {
	TBInputStruct(ptb, pOut, lpButtons);
	
	if(ptb->hwndToolTips && !(lpButtons->fsStyle & TBSTYLE_SEP)) {
	    TOOLINFO ti;
	    // don't bother setting the rect because we'll do it below
	    // in FlushToolTipsMgr;
            ti.cbSize = sizeof(ti);
	    ti.uFlags = 0;
	    ti.hwnd = ptb->hwnd;
	    ti.uId = lpButtons->idCommand;
            ti.lpszText = LPSTR_TEXTCALLBACK;
	    SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0,
			(LPARAM)(LPTOOLINFO)&ti);
	}

	if ((pOut->fsStyle & TBSTYLE_SEP) && pOut->iBitmap <=0)
	    pOut->iBitmap = g_dxButtonSep;
    }

    // flush the cache
    FlushButtonCache(ptb);

	// Re-compute layout if toolbar is wrapable.
	if (ptb->style & TBSTYLE_WRAPABLE) {
		SendMessage(ptb->hwnd, TB_AUTOSIZE, 0, 0);
	}

    // We need to completely redraw the toolbar at this point.
    InvalidateRect(ptb->hwnd, NULL, TRUE);

    return(TRUE);
}


/* Notice that the state structure is not realloc'ed smaller at this
 * point.  This is a time optimization, and the fact that the structure
 * will not move is used in other places.
 */
BOOL FAR PASCAL DeleteButton(PTBSTATE ptb, UINT uIndex)
{
    PTBBUTTON pIn, pOut;

    if (uIndex >= (UINT)ptb->iNumButtons)
	return FALSE;

    if (ptb->hwndToolTips) {
	TOOLINFO ti;

        ti.cbSize = sizeof(ti);
	ti.hwnd = ptb->hwnd;
	ti.uId = ptb->Buttons[uIndex].idCommand;
	SendMessage(ptb->hwndToolTips, TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
    }

    --ptb->iNumButtons;
    for (pOut=ptb->Buttons+uIndex, pIn=pOut+1;
	 uIndex<(UINT)ptb->iNumButtons; ++uIndex, ++pIn, ++pOut)
	*pOut = *pIn;

    // flush the cache
    FlushButtonCache(ptb);

    // We need to completely redraw the toolbar at this point.
    InvalidateRect(ptb->hwnd, NULL, TRUE);

    return TRUE;
}


// deal with old TBBUTON structs for compatibility

void FAR PASCAL TBInputStruct(PTBSTATE ptb, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt)
{
    if (ptb->uStructSize >= sizeof(TBBUTTON))
    {
    	*pButtonInt = *pButtonExt;
    }
    else
    /* It is assumed the only other possibility is the OLDBUTTON struct */
    {
    	*(LPOLDTBBUTTON)pButtonInt = *(LPOLDTBBUTTON)pButtonExt;
    	/* We don't care about dwData */
    	pButtonInt->dwData = 0;
    	pButtonInt->iString = -1;
    }
}


void NEAR PASCAL TBOutputStruct(PTBSTATE ptb, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt)
{
    if (ptb->uStructSize >= sizeof(TBBUTTON))
    {
    	LPSTR pOut;
    	int i;

    	/* Fill the part we know about and fill the rest with 0's
    	*/
    	*pButtonExt = *pButtonInt;
    	for (i = ptb->uStructSize - sizeof(TBBUTTON), pOut = (LPSTR)(pButtonExt + 1);
    		i > 0; --i, ++pOut)
    	{
    	    *pOut = 0;
    	}
    }
    else
    /* It is assumed the only other possibility is the OLDBUTTON struct */
    {
    	*(LPOLDTBBUTTON)pButtonExt = *(LPOLDTBBUTTON)pButtonInt;
    }
}

void NEAR PASCAL TBOnButtonStructSize(PTBSTATE ptb, UINT uStructSize)
{
	/* You are not allowed to change this after adding buttons.
	*/
	if (ptb && !ptb->iNumButtons)
	{
            ptb->uStructSize = uStructSize;
	}
}


LRESULT CALLBACK ToolbarWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fSameButton;
    PTBBUTTON ptbButton;
    int iPos;
    BYTE fsState;
    DWORD dw;
    PTBSTATE ptb = (PTBSTATE)GetWindowInt(hWnd, 0);

    switch (wMsg) {
    case WM_NCCREATE:

	#define lpcs ((LPCREATESTRUCT)lParam)

        InitDitherBrush();
        InitGlobalMetrics(0);
        InitGlobalColors();
	if (!InitGlobalObjects())
	{
            FreeGlobalObjects();
	    return 0;	// WM_NCCREATE failure is 0
        }


	/* create the state data for this toolbar */

	ptb = (PTBSTATE)LocalAlloc(LPTR, sizeof(TBSTATE) - sizeof(TBBUTTON));
	if (!ptb)
	    return 0;	// WM_NCCREATE failure is 0

	// note, zero init memory from above
        ptb->hwnd = hWnd;
	ptb->hwndCommand = lpcs->hwndParent;
	ptb->style = lpcs->style;
        ptb->hfontIcon = NULL;  // initialize to null.
	ptb->uStructSize = 0;

        // Now Initialize the hfont we will use.
        TBChangeFont(ptb, 0);

	// grow the button size to the appropriate girth
	if (!SetBitmapSize(ptb, DEFAULTBITMAPX, DEFAULTBITMAPX))
	{
	    LocalFree((HLOCAL)ptb);
	    return 0;	// WM_NCCREATE failure is 0
	}

	SetWindowInt(hWnd, 0, (int)ptb);

	if (!(ptb->style & (CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM)))
	{
	    ptb->style |= CCS_TOP;
	    SetWindowLong(hWnd, GWL_STYLE, ptb->style);
	}


	if (ptb->style & TBSTYLE_TOOLTIPS)
	{
	    TOOLINFO ti;
	    // don't bother setting the rect because we'll do it below
	    // in FlushToolTipsMgr;
            ti.cbSize = sizeof(ti);
	    ti.uFlags = TTF_IDISHWND;
	    ti.hwnd = hWnd;
	    ti.uId = (UINT)hWnd;
            ti.lpszText = 0;
	    ptb->hwndToolTips = CreateWindow(c_szSToolTipsClass, NULL,	
		WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hWnd, NULL, lpcs->hInstance, NULL);

	    SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0,
			(LPARAM)(LPTOOLINFO)&ti);
        }
	return TRUE;

    case WM_DESTROY:
	if (ptb)
	  {
	    PTBBMINFO pTemp;
	    int i;

	    /* Free all the bitmaps before exiting
	     */
	    for (pTemp = ptb->pBitmaps, i = ptb->nBitmaps - 1; i >= 0; ++pTemp, --i)
	      {
		if (pTemp->hInst && pTemp->hbm)
		    DeleteObject(pTemp->hbm);
	      }
	    FlushButtonCache(ptb);
	    if (ptb->nStrings > 0)
		DestroyStrings(ptb);

            if (ptb->hfontIcon)
                DeleteObject(ptb->hfontIcon);

              if (ptb->pBitmaps)
                  LocalFree(ptb->pBitmaps);

	    LocalFree((HLOCAL)ptb);
	    SetWindowInt(hWnd, 0, 0);
	  }
	FreeGlobalObjects();
        TerminateDitherBrush();
	break;

    case WM_NCCALCSIZE:

        // let defwindowproc handle the standard borders etc...
	dw = DefWindowProc(hWnd, wMsg, wParam, lParam);

	// add the extra edge at the top of the toolbar to seperate from the menu bar
	if (!(ptb->style & CCS_NODIVIDER))
	{
	    ((NCCALCSIZE_PARAMS FAR *)lParam)->rgrc[0].top += g_cyEdge;
	}
	return dw;

    case WM_NCHITTEST:
        return HTCLIENT;

    case WM_NCACTIVATE:	// BUGBUG: really needed? actiave and inactive draw the same.
    case WM_NCPAINT:
	// old-style toolbars are forced to be without dividers above
	if (!(ptb->style & CCS_NODIVIDER))
	{
	    RECT rc;
	    HDC hdc = GetWindowDC(hWnd);
	    GetWindowRect(hWnd, &rc);
	    MapWindowRect(NULL, hWnd, &rc);	// screen -> client

	    rc.bottom = -rc.top;		// bottom of NC area
	    rc.top = rc.bottom - g_cyEdge;

	    DrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_TOP | BF_BOTTOM);
	    ReleaseDC(hWnd, hdc);
	}
	else
	    goto DoDefault;
	break;

    case WM_PRINTCLIENT:
    case WM_PAINT:
	{
	    BOOL bFlushToolTipsMgr;

	    ENTERCRITICAL;
	    // BUGUG: out bogus paint code uses lots of globals so this
	    // must be atomic.  bogus hu?
	    bFlushToolTipsMgr = ToolbarPaint(ptb, (HDC)wParam);
	    LEAVECRITICAL;

	    if (bFlushToolTipsMgr) {
		FlushToolTipsMgr(ptb);
	    }
	}
	break;

    case WM_SYSCOLORCHANGE:
	TB_OnSysColorChange(ptb);
	break;

    case TB_GETROWS:
	return CountRows(ptb);
	
    case TB_SETROWS:
      {
	RECT rc;

	if (BoxIt(ptb, LOWORD(wParam), HIWORD(wParam), &rc))
	{
            FlushToolTipsMgr(ptb);
            SetWindowPos(hWnd, NULL, 0, 0, rc.right, rc.bottom,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
            InvalidateRect(hWnd, NULL, TRUE);
	}
	if (lParam)
	    *((RECT FAR *)lParam) = rc;
      }	
      break;

    case TB_AUTOSIZE:
    case WM_SIZE:
    {
	HWND hwndParent;
	RECT rc;


	hwndParent = GetParent(hWnd);
	if (!hwndParent)
	    break;


        if (ptb->style & TBSTYLE_WRAPABLE)
	{
	    RECT rcNew;
            if ((ptb->style & CCS_NORESIZE) || (ptb->style & CCS_NOPARENTALIGN)) {
                GetWindowRect(hWnd, &rc);
            } else {
                GetWindowRect(hwndParent, &rc);
            }

	    WrapToolbar(ptb, rc.right - rc.left, &rcNew, NULL);
	    FlushToolTipsMgr(ptb);
        }

        GetWindowRect(hWnd, &rc);
        MapWindowPoints(HWND_DESKTOP, hwndParent, (LPPOINT)&rc, 2);
        NewSize(hWnd, ptb->iButHeight * CountRows(ptb) + g_cxEdge * 2, ptb->style,
                rc.left, rc.top, rc.right, rc.bottom);
	break;
    }

    case WM_COMMAND:
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
	SendMessage(ptb->hwndCommand, wMsg, wParam, lParam);
	break;

    case WM_RBUTTONDBLCLK:
        if (!SendNotify(ptb->hwndCommand, hWnd, NM_RDBLCLK, NULL))
            goto DoDefault;
        break;

    case WM_RBUTTONUP:
        if (!SendNotify(ptb->hwndCommand, hWnd, NM_RCLICK, NULL))
            goto DoDefault;
        break;

    case WM_LBUTTONDBLCLK:
#ifndef IEWIN31_25
        iPos = TBHitTest(ptb, LOWORD(lParam), HIWORD(lParam));
	if (iPos < 0 && (ptb->style & CCS_ADJUSTABLE))
	  {
	    iPos = -1 - iPos;
	    CustomizeTB(ptb, iPos);
        } else {
            goto HandleLButtonDown;
        }
#else
    goto HandleLButtonDown;
#endif
	break;

    case WM_LBUTTONDOWN:
        RelayToToolTips(ptb->hwndToolTips, hWnd, wMsg, wParam, lParam);

        iPos = TBHitTest(ptb, LOWORD(lParam), HIWORD(lParam));

#ifndef IEWIN31_25
	if ((ptb->style & CCS_ADJUSTABLE) &&
		 (((wParam & MK_SHIFT) && !(ptb->style & TBSTYLE_ALTDRAG)) ||
		  ((GetKeyState(VK_MENU) & ~1) && (ptb->style & TBSTYLE_ALTDRAG))))
	{
	    MoveButton(ptb, iPos);
	}
	else 
#endif
	{

HandleLButtonDown:
            if (iPos >= 0 && iPos < ptb->iNumButtons)
            {
                // should this check for the size of the button struct?
                ptbButton = ptb->Buttons + iPos;

                ptb->pCaptureButton = ptbButton;
                SetCapture(hWnd);

                if (ptbButton->fsState & TBSTATE_ENABLED)
                {
                    ptbButton->fsState |= TBSTATE_PRESSED;
                    InvalidateButton(ptb, ptbButton);
                    UpdateWindow(hWnd);         // imedeate feedback
                }

                SendItemNotify(ptb, ptb->pCaptureButton->idCommand, TBN_BEGINDRAG);
            }
        }
	break;

    case WM_MOUSEMOVE:
        RelayToToolTips(ptb->hwndToolTips, hWnd, wMsg, wParam, lParam);

	// if the toolbar has lost the capture for some reason, stop
	if (ptb->pCaptureButton == NULL) {
            //DebugMsg(DM_TRACE, "Bail because pCaptureButton == NULL");
	    break;
        }

	if (hWnd != GetCapture())
	{
            //DebugMsg(DM_TRACE, "cature isn't us");
	    SendItemNotify(ptb, ptb->pCaptureButton->idCommand, TBN_ENDDRAG);

	    // if the button is still pressed, unpress it.
	    if (ptb->pCaptureButton->fsState & TBSTATE_PRESSED)
	    	SendMessage(hWnd, TB_PRESSBUTTON, ptb->pCaptureButton->idCommand, 0L);
	    ptb->pCaptureButton = NULL;
	}
	else if (ptb->pCaptureButton->fsState & TBSTATE_ENABLED)
	{
            //DebugMsg(DM_TRACE, "cature IS us, and state is enabled");
	    iPos = TBHitTest(ptb, LOWORD(lParam), HIWORD(lParam));
	    fSameButton = (iPos >= 0 && ptb->pCaptureButton == ptb->Buttons + iPos);
	    if (fSameButton == !(ptb->pCaptureButton->fsState & TBSTATE_PRESSED))
	    {
                //DebugMsg(DM_TRACE, "cature IS us, and Button is different");
	    	ptb->pCaptureButton->fsState ^= TBSTATE_PRESSED;
	    	InvalidateButton(ptb, ptb->pCaptureButton);
	    }
        }
	break;

    case WM_LBUTTONUP:
        RelayToToolTips(ptb->hwndToolTips, hWnd, wMsg, wParam, lParam);

	if (ptb->pCaptureButton != NULL) {

	    int idCommand = ptb->pCaptureButton->idCommand;

	    ReleaseCapture();

	    SendItemNotify(ptb, idCommand, TBN_ENDDRAG);

	    iPos = TBHitTest(ptb, LOWORD(lParam), HIWORD(lParam));
	    if ((ptb->pCaptureButton->fsState & TBSTATE_ENABLED) && iPos >=0
	    	&& (ptb->pCaptureButton == ptb->Buttons+iPos)) {
	    	ptb->pCaptureButton->fsState &= ~TBSTATE_PRESSED;

	    	if (ptb->pCaptureButton->fsStyle & TBSTYLE_CHECK) {
	    	    if (ptb->pCaptureButton->fsStyle & TBSTYLE_GROUP) {

	    	    	// group buttons already checked can't be force
	    	    	// up by the user.

	    	    	if (ptb->pCaptureButton->fsState & TBSTATE_CHECKED) {
	    	    		ptb->pCaptureButton = NULL;
	    	    		break;	// bail!
	    	    	}

	    	    	ptb->pCaptureButton->fsState |= TBSTATE_CHECKED;
	    	    	MakeGroupConsistant(ptb, idCommand);
	    	    } else {
	    	    	ptb->pCaptureButton->fsState ^= TBSTATE_CHECKED; // toggle
	    	    }
	    	    // if we change a button's state, we need to flush the
	    	    // cache
	    	    FlushButtonCache(ptb);
	    	}
	    	InvalidateButton(ptb, ptb->pCaptureButton);
	    	ptb->pCaptureButton = NULL;

	    	FORWARD_WM_COMMAND(ptb->hwndCommand, idCommand, hWnd, BN_CLICKED, SendMessage);
	    }
	    else {
	    	ptb->pCaptureButton = NULL;
	    }
	}
	break;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        TBChangeFont(ptb, wParam);
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
        case TTN_SHOW:
        case TTN_POP:
        case TTN_NEEDTEXT:
            SendMessage(ptb->hwndCommand, WM_NOTIFY, wParam, lParam);
            break;
        }
        break;

    case WM_STYLECHANGED:
	if (wParam == GWL_STYLE)
	{
	    ptb->style = ((LPSTYLESTRUCT)lParam)->styleNew;
	    DebugMsg(DM_TRACE, "toolbar window style changed %x", ptb->style);
	}
        return 0;

    case TB_SETSTATE:
	iPos = PositionFromID(ptb, (int)wParam);
	if (iPos < 0)
	    return FALSE;
	ptbButton = ptb->Buttons + iPos;

	fsState = (BYTE)(LOWORD(lParam) ^ ptbButton->fsState);
        ptbButton->fsState = (BYTE)LOWORD(lParam);

	if (fsState)
	    // flush the button cache
	    //!!!! this could be much more intelligent
	    FlushButtonCache(ptb);

	if (fsState & TBSTATE_HIDDEN)
	    InvalidateRect(hWnd, NULL, TRUE);
	else if (fsState)
	    InvalidateButton(ptb, ptbButton);
        return TRUE;

    // set the cmd ID of a button based on its position
    case TB_SETCMDID:
	if (wParam >= (UINT)ptb->iNumButtons)
	    return FALSE;

	ptb->Buttons[wParam].idCommand = (UINT)lParam;
	return TRUE;

    case TB_GETSTATE:
	iPos = PositionFromID(ptb, (int)wParam);
	if (iPos < 0)
	    return -1L;
        return ptb->Buttons[iPos].fsState;

    case TB_ENABLEBUTTON:
    case TB_CHECKBUTTON:
    case TB_PRESSBUTTON:
    case TB_HIDEBUTTON:
    case TB_INDETERMINATE:

        iPos = PositionFromID(ptb, (int)wParam);
	if (iPos < 0)
	    return FALSE;
        ptbButton = &ptb->Buttons[iPos];
        fsState = ptbButton->fsState;

        if (LOWORD(lParam))
            ptbButton->fsState |= wStateMasks[wMsg - TB_ENABLEBUTTON];
	else
            ptbButton->fsState &= ~wStateMasks[wMsg - TB_ENABLEBUTTON];

        // did this actually change the state?
        if (fsState != ptbButton->fsState) {
            // is this button a member of a group?
	    if ((wMsg == TB_CHECKBUTTON) && (ptbButton->fsStyle & TBSTYLE_GROUP))
	        MakeGroupConsistant(ptb, (int)wParam);

	    // flush the button cache
	    //!!!! this could be much more intelligent
	    FlushButtonCache(ptb);

	    if (wMsg == TB_HIDEBUTTON)
		InvalidateRect(hWnd, NULL, TRUE);
	    else
		InvalidateButton(ptb, ptbButton);
        }
        return(TRUE);

    case TB_ISBUTTONENABLED:
    case TB_ISBUTTONCHECKED:
    case TB_ISBUTTONPRESSED:
    case TB_ISBUTTONHIDDEN:
    case TB_ISBUTTONINDETERMINATE:
        iPos = PositionFromID(ptb, (int)wParam);
	if (iPos < 0)
	    return(-1L);
        return (LRESULT)ptb->Buttons[iPos].fsState & wStateMasks[wMsg - TB_ISBUTTONENABLED];

    case TB_ADDBITMAP:
#ifdef WIN32
    case TB_ADDBITMAP32:	// only for compatibility with mail
        #define pab ((LPTBADDBITMAP)lParam)
	return AddBitmap(ptb, wParam, pab->hInst, pab->nID);
	#undef pab
#else
	return AddBitmap(ptb, wParam, (HINSTANCE)LOWORD(lParam), HIWORD(lParam));
#endif	

    case TB_REPLACEBITMAP:
        return ReplaceBitmap(ptb, (LPTBREPLACEBITMAP)lParam);

    case TB_ADDSTRING:
	return AddStrings(ptb, wParam, lParam);

    case TB_ADDBUTTONS:
	return InsertButtons(ptb, (UINT)-1, wParam, (LPTBBUTTON)lParam);

    case TB_INSERTBUTTON:
	return InsertButtons(ptb, wParam, 1, (LPTBBUTTON)lParam);

    case TB_DELETEBUTTON:
	return DeleteButton(ptb, wParam);

    case TB_GETBUTTON:
	if (wParam >= (UINT)ptb->iNumButtons)
	    return(FALSE);

	TBOutputStruct(ptb, ptb->Buttons + wParam, (LPTBBUTTON)lParam);
	return TRUE;

    case TB_BUTTONCOUNT:
	return ptb->iNumButtons;

    case TB_COMMANDTOINDEX:
        return PositionFromID(ptb, (int)wParam);

#ifndef IEWIN31_25
    case TB_SAVERESTORE:
#ifdef WIN32
	#define psr ((TBSAVEPARAMS *)lParam)
	return SaveRestoreFromReg(ptb, wParam, psr->hkr, psr->pszSubKey, psr->pszValueName);
	#undef psr
#else
	return SaveRestore(ptb, wParam, (LPSTR FAR *)lParam);
#endif
    case TB_CUSTOMIZE:
	CustomizeTB(ptb, ptb->iNumButtons);
	break;
#endif

    case TB_GETITEMRECT:
	return GetItemRect(ptb, wParam, (LPRECT)lParam);

    case TB_BUTTONSTRUCTSIZE:
        TBOnButtonStructSize(ptb, wParam);
        break;

    case TB_SETBUTTONSIZE:
	return GrowToolbar(ptb, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), FALSE);

    case TB_SETBITMAPSIZE:
	return SetBitmapSize(ptb, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));

    case WM_GETFONT:
	return (LRESULT)(UINT)(ptb? ptb->hfontIcon : 0);

    case TB_GETTOOLTIPS:
        return (LRESULT)(UINT)ptb->hwndToolTips;
	
    case TB_SETTOOLTIPS:
	ptb->hwndToolTips = (HWND)wParam;
	break;

    case TB_SETPARENT:
	{
	HWND hwndOld = ptb->hwndCommand;
	ptb->hwndCommand = (HWND)wParam;
	return (LRESULT)(UINT)hwndOld;
	}
	
    case TB_CHANGEBITMAP:
    {
        PTBBMINFO pTemp;
        int nBitmap;
        UINT nTot;

        iPos = PositionFromID(ptb, (int)wParam);
        if (iPos < 0)
            return(FALSE);

        //
        // Check to see if the new bitmap ID is
        // valid.
        //

        pTemp = ptb->pBitmaps;
        nTot = 0;

        for (nBitmap=0; nBitmap < ptb->nBitmaps; nBitmap++) {
            nTot += pTemp->nButtons;
            pTemp++;
        }

        if (LOWORD(lParam) >= nTot)
            return FALSE;

        ptbButton = &ptb->Buttons[iPos];
        ptbButton->iBitmap = LOWORD(lParam);
        FlushButtonCache(ptb);
        InvalidateButton(ptb, ptbButton);
        return TRUE;
    }

    case TB_GETBITMAP:
        iPos = PositionFromID(ptb, (int)wParam);
        if (iPos < 0)
            return(FALSE);
        ptbButton = &ptb->Buttons[iPos];
        return ptbButton->iBitmap;

    case TB_GETBUTTONTEXT:
        iPos = PositionFromID(ptb, (int)wParam);
        if (iPos >= 0) {
            ptbButton = &ptb->Buttons[iPos];
            if ((ptbButton->iString > -1) && (ptbButton->iString < ptb->nStrings)) {
                if (lParam) {
                    lstrcpy((LPSTR)lParam, ptb->pStrings[ptbButton->iString]);
                }
                return lstrlen(ptb->pStrings[ptbButton->iString]);
            }
        }
        return -1;

#ifdef WIN32
    case TB_GETBITMAPFLAGS:
        {
	DWORD fFlags = 0;
	HDC hdc = GetDC(NULL);

        if (GetDeviceCaps(hdc, LOGPIXELSY) >= 120)
            fFlags |= TBBF_LARGE;

        ReleaseDC(NULL, hdc);

        return fFlags;
	}
#endif
	
    default:
DoDefault:
	return DefWindowProc(hWnd, wMsg, wParam, lParam);
    }

    return 0L;
}
