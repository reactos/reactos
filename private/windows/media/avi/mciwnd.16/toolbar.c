/*
**
** Toolbar.c
**
** This is it, the incredibly famous toolbar control.  Most of 
** the customization stuff is in another file.
**
*/
#include "ctlspriv.h"

#define Reference(x) ((x)=(x))

#define SZCODE static char _based(_segname("_CODE"))

char aszToolbarClassName[] = TOOLBARCLASSNAME;

SZCODE szUSER[] = "USER.EXE";
SZCODE szDrawFrameControl[] = "DrawFrameControl";
SZCODE szKernel[] = "KERNEL.EXE";
SZCODE szWriteProfileStruct[] = "WritePrivateProfileStruct";

// these values are defined by the UI gods...
#define DEFAULTBITMAPX 16
#define DEFAULTBITMAPY 15

#define DEFAULTBUTTONX 24
#define DEFAULTBUTTONY 22

// horizontal/vertical space taken up by button chisel, sides, 
// and a 1 pixel margin.  used in GrowToolbar.
#define XSLOP 7
#define YSLOP 6

#define SLOPTOP 1
#define SLOPBOT 1
#define SLOPLFT 8

static int dxButtonSep = 8;
static int xFirstButton = SLOPLFT;  //!!! was 8

static int iInitCount = 0;

static int nSelectedBM = -1;
static HDC hdcGlyphs = NULL;           // globals for fast drawing
static HDC hdcMono = NULL;
static HBITMAP hbmMono = NULL;
static HBITMAP hbmDefault = NULL;

static HDC hdcButton = NULL;           // contains hbmFace (when it exists)
static HBITMAP hbmFace = NULL;
static int dxFace, dyFace;             // current dimensions of hbmFace (2*dxFace)

static HDC hdcFaceCache = NULL;        // used for button cache

static HFONT hIconFont = NULL;         // font used for strings in buttons
static int yIconFont;                  // height of the font

static BOOL g_bUseDFC = FALSE;         // use DrawFrameControl, if available
static BOOL g_bProfStruct = FALSE;     // use PrivateProfileStruct routines
static WORD g_dxOverlap = 1;           // overlap between buttons

static WORD wStateMasks[] = {
    TBSTATE_ENABLED,
    TBSTATE_CHECKED,
    TBSTATE_PRESSED,
    TBSTATE_HIDDEN,
    TBSTATE_INDETERMINATE
};

LRESULT CALLBACK _loadds ToolbarWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

#define HeightWithString(h) (h + yIconFont + 1)

static BOOL NEAR PASCAL InitGlobalObjects(void)
{
    LOGFONT lf;
    TEXTMETRIC tm;
    HFONT hOldFont;

    iInitCount++;

    if (iInitCount != 1)
        return TRUE;

    hdcGlyphs = CreateCompatibleDC(NULL);
    if (!hdcGlyphs)
        return FALSE;
    hdcMono = CreateCompatibleDC(NULL);
    if (!hdcMono)
        return FALSE;

    hbmMono = CreateBitmap(DEFAULTBUTTONX, DEFAULTBUTTONY, 1, 1, NULL);
    if (!hbmMono)
        return FALSE;

    hbmDefault = SelectObject(hdcMono, hbmMono);

    hdcButton = CreateCompatibleDC(NULL);
    if (!hdcButton)
        return FALSE;
    hdcFaceCache = CreateCompatibleDC(NULL);
    if (!hdcFaceCache)
        return FALSE;

    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0);
    hIconFont = CreateFontIndirect(&lf);
    if (!hIconFont)
	return FALSE;

    hOldFont = SelectObject(hdcMono, hIconFont);
    GetTextMetrics(hdcMono, &tm);
    yIconFont = tm.tmHeight;
    if (hOldFont)
	SelectObject(hdcMono, hOldFont);

#if WINVER >= 0x0400
    // set a global flag to see if USER will draw for us
    if (GetProcAddress(LoadLibrary(szUSER), szDrawFrameControl))
    {
	g_bUseDFC = TRUE;
	g_dxOverlap = 0;	// buttons do NOT overlap with new look
    }
    // set a global flag to see if KERNEL does profile structs
    if (GetProcAddress(LoadLibrary(szKernel), szWriteProfileStruct))
        g_bProfStruct = TRUE;
#endif

    return TRUE;
}


static BOOL NEAR PASCAL FreeGlobalObjects(void)
{
    iInitCount--;

    if (iInitCount != 0)
        return TRUE;

    if (hdcMono) {
	if (hbmDefault)
	    SelectObject(hdcMono, hbmDefault);
	DeleteDC(hdcMono);		// toast the DCs
    }
    hdcMono = NULL;

    if (hdcGlyphs)
	DeleteDC(hdcGlyphs);
    hdcGlyphs = NULL;
    if (hdcFaceCache)
	DeleteDC(hdcFaceCache);
    hdcFaceCache = NULL;

    if (hdcButton) {
	if (hbmDefault)
	    SelectObject(hdcButton, hbmDefault);
	DeleteDC(hdcButton);
    }
    hdcButton = NULL;

    if (hbmFace)
	DeleteObject(hbmFace);
    hbmFace = NULL;

    if (hbmMono)
	DeleteObject(hbmMono);
    hbmMono = NULL;

    if (hIconFont)
	DeleteObject(hIconFont);
    hIconFont = NULL;
}

HWND WINAPI CreateToolbarEx(HWND hwnd, DWORD ws, WORD wID, int nBitmaps,
			HINSTANCE hBMInst, WORD wBMID, LPCTBBUTTON lpButtons, 
			int iNumButtons, int dxButton, int dyButton, 
			int dxBitmap, int dyBitmap, UINT uStructSize)
{

    HWND hwndToolbar;

    hwndToolbar = CreateWindow(aszToolbarClassName, NULL, WS_CHILD | ws,
	      0, 0, 100, 30, hwnd, (HMENU)wID,
	      (HINSTANCE)GetWindowWord(hwnd, GWW_HINSTANCE), NULL);
    if (!hwndToolbar)
	goto Error1;

    SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, uStructSize, 0L);

    if (dxBitmap && dyBitmap)
	if (!SendMessage(hwndToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(dxBitmap, dyBitmap)))
	{
	    //!!!! do we actually need to deal with this?
	    DestroyWindow(hwndToolbar);
	    hwndToolbar = NULL;
	    goto Error1;
	}

    if (dxButton && dyButton)
	if (!SendMessage(hwndToolbar, TB_SETBUTTONSIZE, 0, MAKELONG(dxButton, dyButton)))
	{
	    //!!!! do we actually need to deal with this?
	    DestroyWindow(hwndToolbar);
	    hwndToolbar = NULL;
	    goto Error1;
	}

    SendMessage(hwndToolbar, TB_ADDBITMAP, nBitmaps, MAKELONG(hBMInst, wBMID));
    SendMessage(hwndToolbar, TB_ADDBUTTONS, iNumButtons, (LPARAM)lpButtons);

Error1:
    return hwndToolbar;
}


#if 0
/* This is no longer declared in COMMCTRL.H.  It only exists for compatibility
** with existing apps; new apps must use CreateToolbarEx.
*/
HWND WINAPI CreateToolbar(HWND hwnd, DWORD ws, WORD wID, int nBitmaps, HINSTANCE hBMInst, WORD wBMID, LPCTBBUTTON lpButtons, int iNumButtons)
{
    // old-style toolbar, so no divider.
    ws |= CCS_NODIVIDER;
    return (CreateToolbarEx(hwnd, ws, wID, nBitmaps, hBMInst, wBMID, 
    		lpButtons, iNumButtons, 0, 0, 0, 0, sizeof(OLDTBBUTTON)));
}
#endif

BOOL FAR PASCAL InitToolbarClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, aszToolbarClassName, &wc)) {

	wc.lpszClassName = aszToolbarClassName;
	wc.style	 = CS_GLOBALCLASS | CS_DBLCLKS;
	wc.lpfnWndProc	 = (WNDPROC)ToolbarWndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = sizeof(PTBSTATE);
	wc.hInstance	 = hInstance;
	wc.hIcon	 = NULL;
	wc.hCursor	 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	wc.lpszMenuName	 = NULL;

	if (!RegisterClass(&wc))
	    return FALSE;
    }

    return TRUE;
}



#define BEVEL   2
#define FRAME   1

static void NEAR PASCAL PatB(HDC hdc,int x,int y,int dx,int dy, DWORD rgb)
{
    RECT    rc;

    SetBkColor(hdc,rgb);
    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}

static void NEAR PASCAL DrawString(HDC hdc, int x, int y, int dx, PSTR pszString)
{
    int oldMode;
    DWORD oldTextColor;
    HFONT oldhFont;
    DWORD dwExt;
    int len;

    oldMode = SetBkMode(hdc, TRANSPARENT);
    oldTextColor = SetTextColor(hdc, 0L);
    oldhFont = SelectObject(hdc, hIconFont);

    len = lstrlen(pszString);
    dwExt = GetTextExtent(hdc, (LPSTR)pszString, len);
    // center the string horizontally
    x += (dx - LOWORD(dwExt) - 1)/2;

    TextOut(hdc, x, y, (LPSTR)pszString, len);

    if (oldhFont)
	SelectObject(hdc, oldhFont);
    SetTextColor(hdc, oldTextColor);
    SetBkMode(hdc, oldMode);
}

// create a mono bitmap mask:
//   1's where color == COLOR_BTNFACE || COLOR_HILIGHT
//   0's everywhere else

static void NEAR PASCAL CreateMask(PTBSTATE pTBState, PTBBUTTON pTBButton, int xoffset, int yoffset, int dx, int dy)
{
    PSTR pFoo;

    // initalize whole area with 1's
    PatBlt(hdcMono, 0, 0, dx, dy, WHITENESS);

    // create mask based on color bitmap
    // convert this to 1's
    SetBkColor(hdcGlyphs, rgbFace);
    BitBlt(hdcMono, xoffset, yoffset, pTBState->iDxBitmap, pTBState->iDyBitmap, 
    	hdcGlyphs, pTBButton->iBitmap * pTBState->iDxBitmap, 0, SRCCOPY);
    // convert this to 1's
    SetBkColor(hdcGlyphs, rgbHilight);
    // OR in the new 1's
    BitBlt(hdcMono, xoffset, yoffset, pTBState->iDxBitmap, pTBState->iDyBitmap, 
    	hdcGlyphs, pTBButton->iBitmap * pTBState->iDxBitmap, 0, SRCPAINT);

    if (pTBButton->iString != -1 && (pTBButton->iString < pTBState->nStrings))
    {
	pFoo = pTBState->pStrings[pTBButton->iString];
	DrawString(hdcMono, 1, yoffset + pTBState->iDyBitmap + 1, dx, pFoo);
    }
}


/* Given a button number, the corresponding bitmap is loaded and selected in,
 * and the Window origin set.
 * Returns NULL on Error, 1 if the necessary bitmap is already selected,
 * or the old bitmap otherwise.
 */
static HBITMAP FAR PASCAL SelectBM(HDC hDC, PTBSTATE pTBState, int nButton)
{
  PTBBMINFO pTemp;
  HBITMAP hRet;
  int nBitmap, nTot;

  for (pTemp=pTBState->pBitmaps, nBitmap=0, nTot=0; ; ++pTemp, ++nBitmap)
    {
      if (nBitmap >= pTBState->nBitmaps)
	  return(NULL);

      if (nButton < nTot+pTemp->nButtons)
	  break;

      nTot += pTemp->nButtons;
    }

  /* Special case when the required bitmap is already selected
   */
  if (nBitmap == nSelectedBM)
      return((HBITMAP)1);

  if (!pTemp->hbm || (hRet=SelectObject(hDC, pTemp->hbm))==NULL)
    {
      if (pTemp->hbm)
	  DeleteObject(pTemp->hbm);

      if (pTemp->hInst)
	  pTemp->hbm = CreateMappedBitmap(pTemp->hInst, pTemp->wID,
		TRUE, NULL, 0);
      else
	  pTemp->hbm = (HBITMAP)pTemp->wID;

      if (!pTemp->hbm || (hRet=SelectObject(hDC, pTemp->hbm))==NULL)
	  return(NULL);
    }

  nSelectedBM = nBitmap;
#ifdef WIN32
  SetWindowOrgEx(hDC, nTot * pTBState->iDxBitmap, 0, NULL);
#else // WIN32
  SetWindowOrg(hDC, nTot * pTBState->iDxBitmap, 0);
#endif

  return(hRet);
}

static void FAR PASCAL DrawBlankButton(HDC hdc, int x, int y, int dx, int dy, WORD state, WORD wButtType)
{
#if WINVER >= 0x0400
    RECT r1;
#endif

    // face color
    PatB(hdc, x, y, dx, dy, rgbFace);

#if WINVER >= 0x0400
    if (g_bUseDFC)
    {
	r1.left = x;
	r1.top = y;
	r1.right = x + dx;
	r1.bottom = y + dy;

	DrawFrameControl(hdc, &r1, wButtType, 
		(state & TBSTATE_PRESSED) ? DFCS_PUSHED : 0);
    }
    else
#endif
    {
	if (state & TBSTATE_PRESSED) {
	    PatB(hdc, x + 1, y, dx - 2, 1, rgbFrame);
	    PatB(hdc, x + 1, y + dy - 1, dx - 2, 1, rgbFrame);
	    PatB(hdc, x, y + 1, 1, dy - 2, rgbFrame);
	    PatB(hdc, x + dx - 1, y +1, 1, dy - 2, rgbFrame);
	    PatB(hdc, x + 1, y + 1, 1, dy-2, rgbShadow);
	    PatB(hdc, x + 1, y + 1, dx-2, 1, rgbShadow);
	}
	else {
	    PatB(hdc, x + 1, y, dx - 2, 1, rgbFrame);
	    PatB(hdc, x + 1, y + dy - 1, dx - 2, 1, rgbFrame);
	    PatB(hdc, x, y + 1, 1, dy - 2, rgbFrame);
	    PatB(hdc, x + dx - 1, y + 1, 1, dy - 2, rgbFrame);
	    dx -= 2;
	    dy -= 2;
	    PatB(hdc, x + 1, y + 1, 1, dy - 1, rgbHilight);
	    PatB(hdc, x + 1, y + 1, dx - 1, 1, rgbHilight);
	    PatB(hdc, x + dx, y + 1, 1, dy, rgbShadow);
	    PatB(hdc, x + 1, y + dy, dx, 1,   rgbShadow);
	    PatB(hdc, x + dx - 1, y + 2, 1, dy - 2, rgbShadow);
	    PatB(hdc, x + 2, y + dy - 1, dx - 2, 1,   rgbShadow);
	}
    }
}

#define DSPDxax	 0x00E20746
#define PSDPxax  0x00B8074A

#define FillBkColor(hdc, prc) ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL)

static void NEAR PASCAL DrawFace(PTBSTATE pTBState, PTBBUTTON ptButton, HDC hdc, int x, int y,
			int offx, int offy, int dx)
{
    PSTR pFoo;

    BitBlt(hdc, x + offx, y + offy, pTBState->iDxBitmap, pTBState->iDyBitmap, 
	    hdcGlyphs, ptButton->iBitmap * pTBState->iDxBitmap, 0, SRCCOPY);

    if (ptButton->iString != -1 && (ptButton->iString < pTBState->nStrings))
    {
	pFoo = pTBState->pStrings[ptButton->iString];
	DrawString(hdc, x + 1, y + offy + pTBState->iDyBitmap + 1, dx, pFoo);
    }
}

static void FAR PASCAL DrawButton(HDC hdc, int x, int y, int dx, int dy, PTBSTATE pTBState, PTBBUTTON ptButton, BOOL bFaceCache)
{
    int yOffset;
    HBRUSH hbrOld, hbr;
    BOOL bMaskCreated = FALSE;
    BYTE state;
    int xButton = 0;		// assume button is down
    int dxFace, dyFace;
    int xCenterOffset;

    dxFace = dx - 4;
    dyFace = dy - 4;

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
	BitBlt(hdc, x, y, dx, dy, hdcButton, xButton, 0, SRCCOPY);
    else
	DrawBlankButton(hdc, x, y, dx, dy, state, pTBState->wButtonType);


    // move coordinates inside border and away from upper left highlight.
    // the extents change accordingly.
    x += 2;
    y += 2;

    if (!SelectBM(hdcGlyphs, pTBState, ptButton->iBitmap))
	return;

    // calculate offset of face from (x,y).  y is always from the top,
    // so the offset is easy.  x needs to be centered in face.
    yOffset = 1;
    xCenterOffset = (dxFace - pTBState->iDxBitmap)/2;
    if (state & (TBSTATE_PRESSED | TBSTATE_CHECKED))
    {
	// pressed state moves down and to the right
	xCenterOffset++;
        yOffset++;
    }

    // now put on the face
    if (state & TBSTATE_ENABLED) {
        // regular version
	DrawFace(pTBState, ptButton, hdc, x, y, xCenterOffset, yOffset, dxFace);
    } else {
        // disabled version (or indeterminate)
	bMaskCreated = TRUE;
	CreateMask(pTBState, ptButton, xCenterOffset, yOffset, dxFace, dyFace);

	SetTextColor(hdc, 0L);	 // 0's in mono -> 0 (for ROP)
	SetBkColor(hdc, 0x00FFFFFF); // 1's in mono -> 1

	// draw glyph's white understrike
	if (!(state & TBSTATE_INDETERMINATE)) {
	    hbr = CreateSolidBrush(rgbHilight);
	    if (hbr) {
	        hbrOld = SelectObject(hdc, hbr);
	        if (hbrOld) {
	            // draw hilight color where we have 0's in the mask
                    BitBlt(hdc, x + 1, y + 1, dxFace, dyFace, hdcMono, 0, 0, PSDPxax);
	            SelectObject(hdc, hbrOld);
	        }
	        DeleteObject(hbr);
	    }
	}

	// gray out glyph
	hbr = CreateSolidBrush(rgbShadow);
	if (hbr) {
	    hbrOld = SelectObject(hdc, hbr);
	    if (hbrOld) {
	        // draw the shadow color where we have 0's in the mask
                BitBlt(hdc, x, y, dxFace, dyFace, hdcMono, 0, 0, PSDPxax);
	        SelectObject(hdc, hbrOld);
	    }
	    DeleteObject(hbr);
	}

	if (state & TBSTATE_CHECKED) {
	    BitBlt(hdcMono, 1, 1, dxFace - 1, dyFace - 1, hdcMono, 0, 0, SRCAND);
	}
    }

    if (state & (TBSTATE_CHECKED | TBSTATE_INDETERMINATE)) {

        hbrOld = SelectObject(hdc, hbrDither);
	if (hbrOld) {

	    if (!bMaskCreated)
	        CreateMask(pTBState, ptButton, xCenterOffset, yOffset, dxFace, dyFace);

	    SetTextColor(hdc, 0L);		// 0 -> 0
	    SetBkColor(hdc, 0x00FFFFFF);	// 1 -> 1

	    // only draw the dither brush where the mask is 1's
            BitBlt(hdc, x, y, dxFace, dyFace, hdcMono, 0, 0, DSPDxax);
	    
	    SelectObject(hdc, hbrOld);
	}
    }
}

static void NEAR PASCAL FlushButtonCache(PTBSTATE pTBState)
{
    if (pTBState->hbmCache) {
	DeleteObject(pTBState->hbmCache);
	pTBState->hbmCache = 0;
    }
}

// make sure that hbmMono is big enough to do masks for this
// size of button.  if not, fail.
static BOOL NEAR PASCAL CheckMonoMask(int width, int height)
{
    BITMAP bm;
    HBITMAP hbmTemp;

    GetObject(hbmMono, sizeof(BITMAP), &bm);
    if (width > bm.bmWidth || height > bm.bmHeight) {
	hbmTemp = CreateBitmap(width, height, 1, 1, NULL);
	if (!hbmTemp)
	    return FALSE;
	SelectObject(hdcMono, hbmTemp);
	DeleteObject(hbmMono);
	hbmMono = hbmTemp;
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
static BOOL NEAR PASCAL GrowToolbar(PTBSTATE pTBState, int newButWidth, int newButHeight, BOOL bInside)
{
    // if growing based on inside measurement, get full size
    if (bInside) {
	newButHeight += YSLOP;
	newButWidth += XSLOP;
	
	// if toolbar already has strings, don't shrink width it because it
	// might clip room for the string
	if ((newButWidth < pTBState->iButWidth) && pTBState->nStrings)
	    newButWidth = pTBState->iButWidth;
    }
    else {
    	if (newButHeight < pTBState->iButHeight)
	    newButHeight = pTBState->iButHeight;
    	if (newButWidth < pTBState->iButWidth)
	    newButWidth = pTBState->iButWidth;
    }

    // if the size of the toolbar is actually growing, see if shadow
    // bitmaps can be made sufficiently large.
    if ((newButWidth > pTBState->iButWidth) || (newButHeight > pTBState->iButHeight)) {
	if (!CheckMonoMask(newButWidth, newButHeight))
	    return(FALSE);
    }

    pTBState->iButWidth = newButWidth;
    pTBState->iButHeight = newButHeight;

//!!!ACK ACK ACK ACK
#if 0
    // bar height has 2 pixels above, 3 below
    pTBState->iBarHeight = pTBState->iButHeight + 5;
    pTBState->iYPos = 2;
#else
    pTBState->iBarHeight = pTBState->iButHeight + SLOPTOP+SLOPBOT;
    pTBState->iYPos = SLOPTOP;
#endif

    return TRUE;
}

static BOOL NEAR PASCAL SetBitmapSize(PTBSTATE pTBState, int width, int height)
{
    int realh = height;

    if (pTBState->nStrings)
	realh = HeightWithString(height);

    if (GrowToolbar(pTBState, width, realh, TRUE)) {
	pTBState->iDxBitmap = width;
	pTBState->iDyBitmap = height;
	return TRUE;
    }
    return FALSE;
}

static void NEAR PASCAL UpdateTBState(PTBSTATE pTBState)
{
	int i;
	PTBBMINFO pBitmap;

	if (pTBState->nSysColorChanges!=nSysColorChanges)
	{
		/* Reset all of the bitmaps if the sys colors have changed
		 * since the last time the bitmaps were created.
		 */
		for (i=pTBState->nBitmaps-1, pBitmap=pTBState->pBitmaps; i>=0;
			--i, ++pBitmap)
		{
			if (pBitmap->hInst && pBitmap->hbm)
			{
				DeleteObject(pBitmap->hbm);
				pBitmap->hbm = NULL;
			}
		}

		FlushButtonCache(pTBState);

		// now we're updated to latest color scheme
		pTBState->nSysColorChanges = nSysColorChanges;
	}
}

#define CACHE 0x01
#define BUILD 0x02

static void NEAR PASCAL ToolbarPaint(HWND hWnd, PTBSTATE pTBState)
{
    RECT rc;
    HDC hdc;
    PAINTSTRUCT ps;
    int iButton, xButton, yButton;
    int cButtons = pTBState->iNumButtons;
    PTBBUTTON pAllButtons = pTBState->Buttons;
    HBITMAP hbmOldGlyphs;
    int xCache = 0;
    WORD wFlags = 0;
    int iCacheWidth = 0;
    HBITMAP hbmTemp;
    BOOL bFaceCache = TRUE;		// assume face cache exists
    int dx,dy;

    CheckSysColors();
    UpdateTBState(pTBState);

    hdc = BeginPaint(hWnd, &ps);

    GetClientRect(hWnd, &rc);
    if (!rc.right)
	goto Error1;

    dx = pTBState->iButWidth;
    dy = pTBState->iButHeight;

    // setup global stuff for fast painting

    /* We need to kick-start the bitmap selection process.
     */
    nSelectedBM = -1;
    hbmOldGlyphs = SelectBM(hdcGlyphs, pTBState, 0);
    if (!hbmOldGlyphs)
	goto Error1;

    yButton = pTBState->iYPos;
    rc.top = yButton;
    rc.bottom = yButton + dy;

    if (!(pTBState->hbmCache)) {
	// calculate the width of the cache.
	for (iButton = 0; iButton < cButtons; iButton++) {
	    if (!(pAllButtons[iButton].fsState & TBSTATE_HIDDEN) &&
			!(pAllButtons[iButton].fsStyle & TBSTYLE_SEP))
		iCacheWidth += pTBState->iButWidth;
	}
	pTBState->hbmCache = CreateCompatibleBitmap(hdcGlyphs, iCacheWidth, dy);
	wFlags |= BUILD;

	// if needed, create or enlarge bitmap for pre-building button states
	if (!(hbmFace && (dx <= dxFace) && (dy <= dyFace))) {
	    hbmTemp = CreateCompatibleBitmap(hdcGlyphs, 2*dx, dy);
	    if (hbmTemp) {
		SelectObject(hdcButton, hbmTemp);
		if (hbmFace)
		    DeleteObject(hbmFace);
		hbmFace = hbmTemp;
		dxFace = dx;
		dyFace = dy;
	    }
	    else 
		bFaceCache = FALSE;
	}
    }
    if (pTBState->hbmCache) {
        SelectObject(hdcFaceCache,pTBState->hbmCache);
	wFlags |= CACHE;
    }
    else
        wFlags = 0;

    if (bFaceCache) {
	DrawBlankButton(hdcButton, 0, 0, dx, dy, TBSTATE_PRESSED, pTBState->wButtonType);
	DrawBlankButton(hdcButton, dx, 0, dx, dy, 0, pTBState->wButtonType);
    }

    for (iButton = 0, xButton = xFirstButton;
	iButton < cButtons;
	iButton++) {

        PTBBUTTON ptbButton = &pAllButtons[iButton];

	if (ptbButton->fsState & TBSTATE_HIDDEN) {
	    /* Do nothing */ ;
        } else if (ptbButton->fsStyle & TBSTYLE_SEP) {
	    xButton += ptbButton->iBitmap;
        } else {
	    if (wFlags & BUILD)
	        DrawButton(hdcFaceCache, xCache, 0, dx, dy, pTBState, ptbButton, bFaceCache);

            rc.left = xButton;
            rc.right = xButton + dx;
	    if (RectVisible(hdc, &rc)) {
		if ((wFlags & CACHE) && !(ptbButton->fsState & TBSTATE_PRESSED))
		    BitBlt(hdc, xButton, yButton, dx, dy, 
				hdcFaceCache, xCache, 0, SRCCOPY);
		else
		    DrawButton(hdc, xButton, yButton, dx, dy, pTBState, ptbButton, bFaceCache);
	    }
	    // advance the "pointer" in the cache
	    xCache += dx;

	    xButton += (dx - g_dxOverlap);
        }
    }

    if (wFlags & CACHE)
	SelectObject(hdcFaceCache, hbmDefault);
    SelectObject(hdcGlyphs, hbmOldGlyphs);

Error1:
    EndPaint(hWnd, &ps);
}


static BOOL NEAR PASCAL GetItemRect(PTBSTATE pTBState, UINT uButton, LPRECT lpRect)
{
	UINT iButton, xPos;
	PTBBUTTON pButton;

	if (uButton>=(UINT)pTBState->iNumButtons
		|| (pTBState->Buttons[uButton].fsState&TBSTATE_HIDDEN))
	{
		return(FALSE);
	}

	xPos = xFirstButton;

	for (iButton=0, pButton=pTBState->Buttons; iButton<uButton;
		++iButton, ++pButton)
	{
		if (pButton->fsState & TBSTATE_HIDDEN)
		{
			/* Do nothing */ ;
		}
		else if (pButton->fsStyle & TBSTYLE_SEP)
		{
			xPos += pButton->iBitmap;
		}
		else
		{
			xPos += (pTBState->iButWidth - g_dxOverlap);
		}
	}

	/* pButton should now point at the required button, and xPos should be
	 * its left edge.  Note that we already checked if the button was
	 * hidden above.
	 */
	lpRect->left   = xPos;
	lpRect->right  = xPos + (pButton->fsStyle&TBSTYLE_SEP
		? pButton->iBitmap : pTBState->iButWidth);
	lpRect->top    = pTBState->iYPos;
	lpRect->bottom = lpRect->top + pTBState->iButHeight;

	return(TRUE);
}


static void NEAR PASCAL InvalidateButton(HWND hwnd, PTBSTATE pTBState, PTBBUTTON pButtonToPaint)
{
	RECT rc;

	if (GetItemRect(pTBState, pButtonToPaint-pTBState->Buttons, &rc))
	{
		InvalidateRect(hwnd, &rc, FALSE);
	}
}


static int FAR PASCAL TBHitTest(PTBSTATE pTBState, int xPos, int yPos)
{
  int iButton;
  int cButtons = pTBState->iNumButtons;
  PTBBUTTON pButton;

  xPos -= xFirstButton;
  if (xPos < 0)
      return(-1);
  yPos -= pTBState->iYPos;

  for (iButton=0, pButton=pTBState->Buttons; iButton<cButtons;
	++iButton, ++pButton)
    {
      if (pButton->fsState & TBSTATE_HIDDEN)
	  /* Do nothing */ ;
      else if (pButton->fsStyle & TBSTYLE_SEP)
	  xPos -= pButton->iBitmap;
      else
	  xPos -= (pTBState->iButWidth - g_dxOverlap);

      if (xPos < 0)
	{
	  if (pButton->fsStyle&TBSTYLE_SEP
		|| (UINT)yPos>=(UINT)pTBState->iButHeight)
	      break;

	  return(iButton);
	}
    }

  return(-1 - iButton);
}


static int FAR PASCAL PositionFromID(PTBSTATE pTBState, int id)
{
    int i;
    int cButtons = pTBState->iNumButtons;
    PTBBUTTON pAllButtons = pTBState->Buttons;

    for (i = 0; i < cButtons; i++)
        if (pAllButtons[i].idCommand == id)
	    return i;		// position found

    return -1;		// ID not found!
}

// check a radio button by button index.
// the button matching idCommand was just pressed down.  this forces
// up all other buttons in the group.
// this does not work with buttons that are forced up with

static void NEAR PASCAL MakeGroupConsistant(HWND hWnd, PTBSTATE pTBState, int idCommand)
{
    int i, iFirst, iLast, iButton;
    int cButtons = pTBState->iNumButtons;
    PTBBUTTON pAllButtons = pTBState->Buttons;

    iButton = PositionFromID(pTBState, idCommand);

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
                InvalidateButton(hWnd, pTBState, &pAllButtons[i]);
                break;          // only one button is down right?
            }
        }
    }
}

static void NEAR PASCAL DestroyStrings(PTBSTATE pTBState)
{
    PSTR *p;
    PSTR end = 0, start = 0;
    int i;

    p = pTBState->pStrings;
    for (i = 0; i < pTBState->nStrings; i++) {
	if (!(*p < end) && (*p > start)) {
	    start = (*p);
	    end = start + LocalSize((HANDLE)*p);
	    LocalFree((HANDLE)*p);
	}
	p++;
	i++;
    }

    LocalFree((HANDLE)pTBState->pStrings);
}

// not needed for MCIWnd
#if 0
#define MAXSTRINGSIZE 1024
static int NEAR PASCAL AddStrings(PTBSTATE pTBState, WPARAM wParam, LPARAM lParam)
{
    int i;
    DWORD dwExt;
    HFONT hOldFont;
    LPSTR lpsz;
    PSTR  pString, psz, ptmp;
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
	LocalReAlloc(pString, i, LMEM_MOVEABLE);

	// convert separators to '\0' and count number of strings
	cSeparator = *pString;
	ptmp = pString;
	pString += 1;
	for (numstr = 0, psz = pString; i >= psz-pString; *psz ? psz = (PSTR)AnsiNext((LPSTR)psz) : psz++) {
	    if (*psz == cSeparator) {
		numstr++;
		*psz = 0;	// terminate with 0
	    }

	}
    }
    // read explicit string.  copy it into local memory, too.
    else {
	// find total length and number of strings
	for (i = 0, numstr = 0, lpsz = (LPSTR)lParam;;) {
	    i++;
	    if (*lpsz == 0) {
		numstr++;
		if (*(lpsz+1) == 0)
		    break;
	    }
	    lpsz++;
	}
	pString = (PSTR)LocalAlloc(LPTR, i);
	ptmp = pString;
	if (!pString)
	    return -1;
	hmemcpy(pString, (void FAR *)lParam, i);
    }

    // make room for increased string pointer table
    if (pTBState->pStrings)
	pFoo = (PSTR *)LocalReAlloc(pTBState->pStrings, 
		(pTBState->nStrings + numstr) * sizeof(PSTR), LMEM_MOVEABLE);
    else
	pFoo = (PSTR *)LocalAlloc(LPTR, numstr * sizeof(PSTR));
    if (!pFoo) {
	LocalFree(ptmp);
	return -1;
    }

    pTBState->pStrings = pFoo;
    // pointer to next open slot in string index table.
    pOffset = pTBState->pStrings + pTBState->nStrings;

    hOldFont = SelectObject(hdcMono, hIconFont);
    // fix up string pointer table to deal with the new strings.
    // check if any string is big enough to necessitate a wider button.
    newWidth = pTBState->iDxBitmap;
    for (i = 0; i < numstr; i++, pOffset++) {
	*pOffset = pString;

	len = lstrlen(pString);
	dwExt = GetTextExtent(hdcMono, pString, len);
	if ((int)(LOWORD(dwExt)) > newWidth)
	    newWidth = LOWORD(dwExt);
	pString += len + 1;
    }
    if (hOldFont)
	SelectObject(hdcMono, hOldFont);

    // is the world big enough to handle the larger buttons?
    if (!GrowToolbar(pTBState, newWidth, HeightWithString(pTBState->iDyBitmap), TRUE))
    {
	// back out changes.
	if (pTBState->nStrings == 0) {
	    LocalFree(pTBState->pStrings);
	    pTBState->pStrings = 0;
	}
	else
	    pTBState->pStrings = (PSTR *)LocalReAlloc(pTBState->pStrings, 
	    		pTBState->nStrings * sizeof(PSTR), LMEM_MOVEABLE);
	LocalFree(ptmp);
	return -1;
    }

    i = pTBState->nStrings;
    pTBState->nStrings += numstr;
    return i;				// index of first added string
}
#endif

/* Adds a new bitmap to the list of BMs available for this toolbar.
 * Returns the index of the first button in the bitmap or -1 if there
 * was an error.
 */
static int NEAR PASCAL AddBitmap(PTBSTATE pTBState, int nButtons,
      HINSTANCE hBMInst, WORD wBMID)
{
  PTBBMINFO pTemp;
  int nBM, nIndex;

  if (pTBState->pBitmaps)
    {
      /* Check if the bitmap has already been added
       */
      for (nBM=pTBState->nBitmaps, pTemp=pTBState->pBitmaps, nIndex=0;
	    nBM>0; --nBM, ++pTemp)
	{
	  if (pTemp->hInst==hBMInst && pTemp->wID==wBMID)
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

      pTemp = (PTBBMINFO)LocalReAlloc(pTBState->pBitmaps,
	    (pTBState->nBitmaps+1)*sizeof(TBBMINFO), LMEM_MOVEABLE);
      if (!pTemp)
	  return(-1);
      pTBState->pBitmaps = pTemp;
    }
  else
    {
      pTBState->pBitmaps = (PTBBMINFO)LocalAlloc(LPTR, sizeof(TBBMINFO));
      if (!pTBState->pBitmaps)
	  return(-1);
    }

  pTemp = pTBState->pBitmaps + pTBState->nBitmaps;

  pTemp->hInst = hBMInst;
  pTemp->wID = wBMID;
  pTemp->nButtons = nButtons;
  pTemp->hbm = NULL;

  ++pTBState->nBitmaps;

  for (nButtons=0, --pTemp; pTemp>=pTBState->pBitmaps; --pTemp)
      nButtons += pTemp->nButtons;

  return(nButtons);
}


static BOOL NEAR PASCAL InsertButtons(HWND hWnd, PTBSTATE pTBState,
      UINT uWhere, UINT uButtons, LPTBBUTTON lpButtons)
{
  PTBBUTTON pIn, pOut;

  if (!pTBState || !pTBState->uStructSize)
      return(FALSE);

  pTBState = (PTBSTATE)LocalReAlloc(pTBState, sizeof(TBSTATE)-sizeof(TBBUTTON)
	+ (pTBState->iNumButtons+uButtons)*sizeof(TBBUTTON), LMEM_MOVEABLE);
  if (!pTBState)
      return(FALSE);

  SETWINDOWPOINTER(hWnd, PTBSTATE, pTBState);

  if (uWhere > (UINT)pTBState->iNumButtons)
      uWhere = pTBState->iNumButtons;

  for (pIn=pTBState->Buttons+pTBState->iNumButtons-1, pOut=pIn+uButtons,
	uWhere=(UINT)pTBState->iNumButtons-uWhere; uWhere>0;
	--pIn, --pOut, --uWhere)
      *pOut = *pIn;

  for (lpButtons=(LPTBBUTTON)((LPSTR)lpButtons+pTBState->uStructSize*(uButtons-1)), pTBState->iNumButtons+=(int)uButtons; uButtons>0;
	--pOut, lpButtons=(LPTBBUTTON)((LPSTR)lpButtons-pTBState->uStructSize), --uButtons)
    {
      TBInputStruct(pTBState, pOut, lpButtons);

      if ((pOut->fsStyle&TBSTYLE_SEP) && pOut->iBitmap<=0)
	  pOut->iBitmap = dxButtonSep;
    }
	      
  // flush the cache
  FlushButtonCache(pTBState);

  /* We need to completely redraw the toolbar at this point.
   */
  InvalidateRect(hWnd, NULL, TRUE);

  return(TRUE);
}


/* Notice that the state structure is not realloc'ed smaller at this
 * point.  This is a time optimization, and the fact that the structure
 * will not move is used in other places.
 */
static BOOL NEAR PASCAL DeleteButton(HWND hWnd, PTBSTATE pTBState, UINT uIndex)
{
  PTBBUTTON pIn, pOut;

  if (uIndex >= (UINT)pTBState->iNumButtons)
      return(FALSE);

  --pTBState->iNumButtons;
  for (pOut=pTBState->Buttons+uIndex, pIn=pOut+1;
	uIndex<(UINT)pTBState->iNumButtons; ++uIndex, ++pIn, ++pOut)
      *pOut = *pIn;

  // flush the cache
  FlushButtonCache(pTBState);

  /* We need to completely redraw the toolbar at this point.
   */
  InvalidateRect(hWnd, NULL, TRUE);

  return(TRUE);
}


static void FAR PASCAL TBInputStruct(PTBSTATE pTBState, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt)
{
	if (pTBState->uStructSize >= sizeof(TBBUTTON))
	{
		*pButtonInt = *pButtonExt;
	}
	else
	/* It is assumed the only other possibility is the OLDBUTTON struct */
	{
		*(LPOLDTBBUTTON)pButtonInt = *(LPOLDTBBUTTON)pButtonExt;
		/* We don't care about dwData */
		pButtonInt->iString = -1;
	}
}


static void FAR PASCAL TBOutputStruct(PTBSTATE pTBState, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt)
{
	if (pTBState->uStructSize >= sizeof(TBBUTTON))
	{
		LPSTR pOut;
		int i;

		/* Fill the part we know about and fill the rest with 0's
		*/
		*pButtonExt = *pButtonInt;
		for (i=pTBState->uStructSize-sizeof(TBBUTTON), pOut=(LPSTR)(pButtonExt+1);
			i>0; --i, ++pOut)
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


LRESULT CALLBACK _loadds ToolbarWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fSameButton;
    PTBBUTTON ptbButton;
    PTBSTATE pTBState;
    int iPos;
    BYTE fsState;
#if WINVER >= 0x0400
    DWORD dw;
#endif

    pTBState = GETWINDOWPOINTER(hWnd, PTBSTATE);

    switch (wMsg) {
    case WM_CREATE:

	#define lpcs ((LPCREATESTRUCT)lParam)

        if (!CreateDitherBrush(FALSE))
            return -1;

	if (!InitGlobalObjects()) {
            FreeGlobalObjects();
	    return -1;
        }

	/* create the state data for this toolbar */

	pTBState = ALLOCWINDOWPOINTER(PTBSTATE, sizeof(TBSTATE)-sizeof(TBBUTTON));
	if (!pTBState)
	    return -1;

	/* The struct is initialized to all NULL when created.
	 */
	pTBState->hwndCommand = lpcs->hwndParent;

	pTBState->uStructSize = 0;

	// grow the button size to the appropriate girth
	if (!SetBitmapSize(pTBState, DEFAULTBITMAPX, DEFAULTBITMAPX))
	    return -1;

	SETWINDOWPOINTER(hWnd, PTBSTATE, pTBState);

	if (!(lpcs->style&(CCS_TOP|CCS_NOMOVEY|CCS_BOTTOM)))
	  {
	    lpcs->style |= CCS_TOP;
	    SetWindowLong(hWnd, GWL_STYLE, lpcs->style);
	  }
	break;

    case WM_DESTROY:
	if (pTBState)
	  {
	    PTBBMINFO pTemp;
	    int i;

	    /* Free all the bitmaps before exiting
	     */
	    for (pTemp=pTBState->pBitmaps, i=pTBState->nBitmaps-1; i>=0;
		  ++pTemp, --i)
	      {
		if (pTemp->hInst && pTemp->hbm)
		    DeleteObject(pTemp->hbm);
	      }
	    FlushButtonCache(pTBState);
	    if (pTBState->nStrings > 0)
		DestroyStrings(pTBState);

	    FREEWINDOWPOINTER(pTBState);
	    SETWINDOWPOINTER(hWnd, PTBSTATE, 0);
	  }
	FreeGlobalObjects();
        FreeDitherBrush();
	break;

    case WM_NCCALCSIZE:
#if WINVER >= 0x0400
         /* 
          * This is sent when the window manager wants to find out
          * how big our client area is to be.  If we have a mini-caption
          * then we trap this message and calculate the cleint area rect,
          * which is the client area rect calculated by DefWindowProc()
          * minus the width/height of the mini-caption bar
          */
         // let defwindowproc handle the standard borders etc...

	dw = DefWindowProc(hWnd, wMsg, wParam, lParam ) ;

	if (!(GetWindowLong(hWnd, GWL_STYLE) & CCS_NODIVIDER))
	{
	    NCCALCSIZE_PARAMS FAR *lpNCP;
	    lpNCP = (NCCALCSIZE_PARAMS FAR *)lParam;
	    lpNCP->rgrc[0].top += 2;
	}

        return dw;
#endif
	break;

    case WM_NCACTIVATE:
    case WM_NCPAINT:

#if WINVER >= 0x0400
	// old-style toolbars are forced to be without dividers above
	if (!(GetWindowLong(hWnd, GWL_STYLE) & CCS_NODIVIDER))
	{
	    HDC hdc;
	    RECT rc;

	    hdc = GetWindowDC(hWnd);
	    GetWindowRect(hWnd, &rc);
	    ScreenToClient(hWnd, (LPPOINT)&(rc.left));
	    ScreenToClient(hWnd, (LPPOINT)&(rc.right));
	    rc.bottom = (-rc.top);	// bottom of NC area
	    rc.top = rc.bottom - (2 * GetSystemMetrics(SM_CYBORDER));

	    DrawBorder(hdc, &rc, BDR_SUNKENOUTER, BF_TOP | BF_BOTTOM);
	    ReleaseDC(hWnd, hdc);
	}
	else
            goto DoDefault;
#endif
	break;

    case WM_PAINT:
	ToolbarPaint(hWnd, pTBState);
	break;

#if 0
    case TB_AUTOSIZE:
    case WM_SIZE:
      {
	RECT rc;
	HWND hwndParent;

	GetWindowRect(hWnd, &rc);
	rc.right -= rc.left;
	rc.bottom -= rc.top;

	/* If there is no parent, then this is a top level window
	 */
	hwndParent = GetParent(hWnd);
	if (hwndParent)
	    ScreenToClient(hwndParent, (LPPOINT)&rc);

	NewSize(hWnd, pTBState->iBarHeight, GetWindowLong(hWnd, GWL_STYLE),
              rc.left, rc.top, rc.right, rc.bottom);
	break;
      }
#endif

    case WM_HSCROLL:  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    case WM_COMMAND:
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
	SendMessage(pTBState->hwndCommand, wMsg, wParam, lParam);
        break;

    case WM_CTLCOLOR:
        //!!!!! ack use COLOR_BTNFACE
        return (LRESULT)(UINT)GetStockObject(LTGRAY_BRUSH);

#if 0
    case WM_LBUTTONDBLCLK:
        iPos = TBHitTest(pTBState, LOWORD(lParam), HIWORD(lParam));
	if (iPos<0 && (GetWindowLong(hWnd, GWL_STYLE)&CCS_ADJUSTABLE))
	  {
	    iPos = -1 - iPos;
	    CustomizeTB(hWnd, pTBState, iPos);
	  }
	break;
#endif

    case WM_LBUTTONDOWN:

        iPos = TBHitTest(pTBState, LOWORD(lParam), HIWORD(lParam));
#if 0
	if ((wParam&MK_SHIFT) &&(GetWindowLong(hWnd, GWL_STYLE)&CCS_ADJUSTABLE))
	  {
	    MoveButton(hWnd, pTBState, iPos);
	  } else
#endif
	if (iPos >= 0)
	  {
	    ptbButton = pTBState->Buttons + iPos;

	    pTBState->pCaptureButton = ptbButton;
	    SetCapture(hWnd);

	    if (ptbButton->fsState & TBSTATE_ENABLED)
	      {
		ptbButton->fsState |= TBSTATE_PRESSED;
		InvalidateButton(hWnd, pTBState, ptbButton);
		UpdateWindow(hWnd);         // imedeate feedback
	      }

	    SendMessage(pTBState->hwndCommand, WM_COMMAND, GETWINDOWID(hWnd), MAKELONG(pTBState->pCaptureButton->idCommand, TBN_BEGINDRAG));
	  }
	break;

    case WM_MOUSEMOVE:
	// if the toolbar has lost the capture for some reason, stop
	if (hWnd != GetCapture()) {
	    SendMessage(pTBState->hwndCommand, WM_COMMAND, GETWINDOWID(hWnd), 
	    		MAKELONG(pTBState->pCaptureButton->idCommand, TBN_ENDDRAG));
	    // if the button is still pressed, unpress it.
	    if (pTBState->pCaptureButton->fsState & TBSTATE_PRESSED)
	        SendMessage(hWnd, TB_PRESSBUTTON, pTBState->pCaptureButton->idCommand, 0L);
	    pTBState->pCaptureButton = NULL;
	}
	else if (pTBState->pCaptureButton!=NULL
	      && (pTBState->pCaptureButton->fsState & TBSTATE_ENABLED)) {

	    iPos = TBHitTest(pTBState, LOWORD(lParam), HIWORD(lParam));
	    fSameButton = (iPos>=0
		  && pTBState->pCaptureButton==pTBState->Buttons+iPos);
	    if (fSameButton == !(pTBState->pCaptureButton->fsState & TBSTATE_PRESSED)) {
		pTBState->pCaptureButton->fsState ^= TBSTATE_PRESSED;
		InvalidateButton(hWnd, pTBState, pTBState->pCaptureButton);
	    }
	}
	break;

    case WM_LBUTTONUP:
	if (pTBState->pCaptureButton != NULL) {

	    int idCommand;

	    idCommand = pTBState->pCaptureButton->idCommand;

	    ReleaseCapture();

	    SendMessage(pTBState->hwndCommand, WM_COMMAND, GETWINDOWID(hWnd), MAKELONG(idCommand, TBN_ENDDRAG));

	    iPos = TBHitTest(pTBState, LOWORD(lParam), HIWORD(lParam));
	    if ((pTBState->pCaptureButton->fsState&TBSTATE_ENABLED) && iPos>=0
		  && (pTBState->pCaptureButton==pTBState->Buttons+iPos)) {
		pTBState->pCaptureButton->fsState &= ~TBSTATE_PRESSED;

		if (pTBState->pCaptureButton->fsStyle & TBSTYLE_CHECK) {
		    if (pTBState->pCaptureButton->fsStyle & TBSTYLE_GROUP) {

			// group buttons already checked can't be force
			// up by the user.

		        if (pTBState->pCaptureButton->fsState & TBSTATE_CHECKED) {
			    pTBState->pCaptureButton = NULL;
			    break;	// bail!
			}

			pTBState->pCaptureButton->fsState |= TBSTATE_CHECKED;
		        MakeGroupConsistant(hWnd, pTBState, idCommand);
		    } else {
			pTBState->pCaptureButton->fsState ^= TBSTATE_CHECKED; // toggle
		    }
		    // if we change a button's state, we need to flush the
		    // cache
		    FlushButtonCache(pTBState);
		}
		InvalidateButton(hWnd, pTBState, pTBState->pCaptureButton);
		pTBState->pCaptureButton = NULL;
		SendMessage(pTBState->hwndCommand, WM_COMMAND, idCommand, 0L);
	    }
	    else {
		pTBState->pCaptureButton = NULL;
	    }
	}
	break;

    case TB_SETSTATE:
	iPos = PositionFromID(pTBState, (int)wParam);
	if (iPos < 0)
	    return(FALSE);
	ptbButton = pTBState->Buttons + iPos;

	fsState = (BYTE)(LOWORD(lParam) ^ ptbButton->fsState);
        ptbButton->fsState = (BYTE)LOWORD(lParam);

	if (fsState)
	    // flush the button cache
	    //!!!! this could be much more intelligent
	    FlushButtonCache(pTBState);

	if (fsState & TBSTATE_HIDDEN)
	    InvalidateRect(hWnd, NULL, TRUE);
	else if (fsState)
	    InvalidateButton(hWnd, pTBState, ptbButton);
        return(TRUE);

    case TB_GETSTATE:
	iPos = PositionFromID(pTBState, (int)wParam);
	if (iPos < 0)
	    return(-1L);
        return(pTBState->Buttons[iPos].fsState);

    case TB_ENABLEBUTTON:
    case TB_CHECKBUTTON:
    case TB_PRESSBUTTON:
    case TB_HIDEBUTTON:
    case TB_INDETERMINATE:

        iPos = PositionFromID(pTBState, (int)wParam);
	if (iPos < 0)
	    return(FALSE);
        ptbButton = &pTBState->Buttons[iPos];
        fsState = ptbButton->fsState;

        if (LOWORD(lParam))
            ptbButton->fsState |= wStateMasks[wMsg - TB_ENABLEBUTTON];
	else
            ptbButton->fsState &= ~wStateMasks[wMsg - TB_ENABLEBUTTON];

        // did this actually change the state?
        if (fsState != ptbButton->fsState) {
            // is this button a member of a group?
	    if ((wMsg == TB_CHECKBUTTON) && (ptbButton->fsStyle & TBSTYLE_GROUP))
	        MakeGroupConsistant(hWnd, pTBState, (int)wParam);

	    // flush the button cache
	    //!!!! this could be much more intelligent
	    FlushButtonCache(pTBState);

	    if (wMsg == TB_HIDEBUTTON)
		InvalidateRect(hWnd, NULL, TRUE);
	    else
		InvalidateButton(hWnd, pTBState, ptbButton);
        }
        return(TRUE);

    case TB_ISBUTTONENABLED:
    case TB_ISBUTTONCHECKED:
    case TB_ISBUTTONPRESSED:
    case TB_ISBUTTONHIDDEN:
    case TB_ISBUTTONINDETERMINATE:
        iPos = PositionFromID(pTBState, (int)wParam);
	if (iPos < 0)
	    return(-1L);
        return (LRESULT)pTBState->Buttons[iPos].fsState
	      & wStateMasks[wMsg - TB_ISBUTTONENABLED];

    case TB_ADDBITMAP:
	return(AddBitmap(pTBState, wParam,
	      (HINSTANCE)LOWORD(lParam), HIWORD(lParam)));

#if 0	// not needed for MCIWnd
    case TB_ADDSTRING:
	return(AddStrings(pTBState, wParam, lParam));
#endif

    case TB_ADDBUTTONS:
	return(InsertButtons(hWnd, pTBState, (UINT)-1, wParam,
	      (LPTBBUTTON)lParam));

    case TB_INSERTBUTTON:
	return(InsertButtons(hWnd, pTBState, wParam, 1, (LPTBBUTTON)lParam));

    case TB_DELETEBUTTON:
	return(DeleteButton(hWnd, pTBState, wParam));

    case TB_GETBUTTON:
	if (wParam >= (UINT)pTBState->iNumButtons)
	    return(FALSE);

	TBOutputStruct(pTBState, pTBState->Buttons+wParam, (LPTBBUTTON)lParam);
	return(TRUE);

    case TB_BUTTONCOUNT:
	return(pTBState->iNumButtons);

    case TB_COMMANDTOINDEX:
        return(PositionFromID(pTBState, (int)wParam));

#if 0
    case TB_SAVERESTORE:
	return(SaveRestore(hWnd, pTBState, wParam, (LPSTR FAR *)lParam));

    case TB_CUSTOMIZE:
	CustomizeTB(hWnd, pTBState, pTBState->iNumButtons);
	break;
#endif

    case TB_GETITEMRECT:
	return(MAKELRESULT(GetItemRect(pTBState, wParam, (LPRECT)lParam), 0));
	break;

    case TB_BUTTONSTRUCTSIZE:
	/* You are not allowed to change this after adding buttons.
	*/
	if (!pTBState || pTBState->iNumButtons)
	{
		break;
	}
	pTBState->uStructSize = wParam;
	break;

    case TB_SETBUTTONSIZE:
	if (!LOWORD(lParam))
	    lParam = MAKELONG(DEFAULTBUTTONX, HIWORD(lParam));
	if (!HIWORD(lParam))
	    lParam = MAKELONG(LOWORD(lParam), DEFAULTBUTTONY);
	return(GrowToolbar(pTBState, LOWORD(lParam), HIWORD(lParam), FALSE));

    case TB_SETBITMAPSIZE:
	return(SetBitmapSize(pTBState, LOWORD(lParam), HIWORD(lParam)));

    case TB_SETBUTTONTYPE:
	pTBState->wButtonType = wParam;
	break;

    default:
#if WINVER >= 0x0400
DoDefault:
#endif
	return DefWindowProc(hWnd, wMsg, wParam, lParam);
    }

    return 0L;
}
