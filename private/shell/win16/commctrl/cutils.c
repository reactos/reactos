/*
**  CUTILS.C
**
**  Common utilities for common controls
**
*/

#include "ctlspriv.h"

DWORD API GetProcessDword(DWORD idProcess, int iIndex); // <krnlcmn.h>

#ifdef WIN31
#ifdef SM_CXEDGE
#undef SM_CXEDGE
#undef SM_CYEDGE
#undef SM_CXMINSPACING
#undef SM_CYMINSPACING
#undef SM_CXSMICON
#undef SM_CYSMICON
#undef SM_CYSMCAPTION
#undef SM_CXSMSIZE
#undef SM_CYSMSIZE
#undef SM_CXMENUSIZE
#undef SM_CYMENUSIZE
#undef SM_ARRANGE
#undef SM_USERTYPE
#undef SM_XWORKAREA
#undef SM_YWORKAREA
#undef SM_CXWORKAREA
#undef SM_CYWORKAREA
#undef SM_CYCAPTIONICON
#undef SM_CYSMCAPTIONICON
#undef SM_CXMINIMIZED
#undef SM_CYMINIMIZED
#undef SM_CXMAXTRACK
#undef SM_CYMAXTRACK
#undef SM_CXMAXIMIZED
#undef SM_CYMAXIMIZED
#undef SM_SHOWSOUNDS
#undef SM_KEYBOARDPREF
#undef SM_HIGHCONTRAST
#undef SM_SCREENREADER
#undef SM_CURSORSIZE
#undef SM_CLEANBOOT
#undef SM_CXDRAG
#undef SM_CYDRAG
#undef SM_NETWORK
#undef SM_CXMENUCHECK
#undef SM_CYMENUCHECK
#endif
#endif

//
// Globals - REVIEW_32
//

int g_cxEdge;
int g_cyEdge;
int g_cxBorder;
int g_cyBorder;
int g_cxScreen;
int g_cyScreen;
int g_cxFrame;
int g_cyFrame;
int g_cxVScroll;
int g_cyHScroll;
int g_cxIcon, g_cyIcon;
int g_cxIconSpacing, g_cyIconSpacing;
int g_cxIconMargin, g_cyIconMargin;
int g_cyLabelSpace;
int g_cxLabelMargin;
int g_cxDoubleClk;
int g_cyDoubleClk;
int g_cxScrollbar;
int g_cyScrollbar;

COLORREF g_clrWindow;
COLORREF g_clrWindowText;
COLORREF g_clrWindowFrame;
COLORREF g_clrGrayText;
COLORREF g_clrBtnText;
COLORREF g_clrBtnFace;
COLORREF g_clrBtnShadow;
COLORREF g_clrBtnHighlight;
COLORREF g_clrHighlight;
COLORREF g_clrHighlightText;
COLORREF g_clrInfoText;
COLORREF g_clrInfoBk;

HBRUSH g_hbrGrayText;
HBRUSH g_hbrWindow;
HBRUSH g_hbrWindowText;
HBRUSH g_hbrWindowFrame;
HBRUSH g_hbrBtnFace;
HBRUSH g_hbrBtnHighlight;
HBRUSH g_hbrBtnShadow;
HBRUSH g_hbrHighlight;


#ifdef WIN31

HBRUSH g_hbr3DDkShadow;
HBRUSH g_hbr3DFace;
HBRUSH g_hbr3DHilight;
HBRUSH g_hbr3DLight;
HBRUSH g_hbr3DShadow;
HBRUSH g_hbrBtnText;
HBRUSH g_hbrWhite;
HBRUSH g_hbrGray;
HBRUSH g_hbrBlack;

int g_oemInfo_Planes;
int g_oemInfo_BitsPixel;
int g_oemInfo_BitCount;

#endif

HFONT g_hfontSystem;

#define CCS_ALIGN (CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM)

/* Note that the default alignment is CCS_BOTTOM
 */
void FAR PASCAL NewSize(HWND hWnd, int nHeight, LONG style, int left, int top, int width, int height)
{
  RECT rc, rcWindow, rcBorder;

  /* Resize the window unless the user said not to
   */
  if (!(style & CCS_NORESIZE))
    {
      /* Calculate the borders around the client area of the status bar
       */
      GetWindowRect(hWnd, &rcWindow);
      rcWindow.right -= rcWindow.left;  // -> dx
      rcWindow.bottom -= rcWindow.top;  // -> dy

      GetClientRect(hWnd, &rc);
      ClientToScreen(hWnd, (LPPOINT)&rc);

      rcBorder.left = rc.left - rcWindow.left;
      rcBorder.top  = rc.top  - rcWindow.top ;
      rcBorder.right  = rcWindow.right  - rc.right  - rcBorder.left;
      rcBorder.bottom = rcWindow.bottom - rc.bottom - rcBorder.top ;

      nHeight += rcBorder.top + rcBorder.bottom;

      /* Check whether to align to the parent window
       */
      if (style & CCS_NOPARENTALIGN)
    {
      /* Check out whether this bar is top aligned or bottom aligned
       */
      switch (style & CCS_ALIGN)
        {
          case CCS_TOP:
          case CCS_NOMOVEY:
        break;

          default:
        top = top + height - nHeight;
        }
    }
      else
    {
      /* It is assumed there is a parent by default
       */
      GetClientRect(GetParent(hWnd), &rc);

      /* Don't forget to account for the borders
       */
      left = -rcBorder.left;
      width = rc.right + rcBorder.left + rcBorder.right;

      if ((style & CCS_ALIGN) == CCS_TOP)
          top = -rcBorder.top;
      else if ((style & CCS_ALIGN) != CCS_NOMOVEY)
          top = rc.bottom - nHeight + rcBorder.bottom;
    }

      if (!(style & CCS_NOMOVEY) && !(style & CCS_NODIVIDER))
        {
      top += g_cyEdge;  // double pixel edge thing
    }

      SetWindowPos(hWnd, NULL, left, top, width, nHeight, SWP_NOZORDER);
    }
}

BOOL FAR PASCAL MGetTextExtent(HDC hdc, LPCSTR lpstr, int cnt, int FAR * pcx, int FAR * pcy)
{
    BOOL fSuccess;
    SIZE size = {0,0};
    fSuccess=GetTextExtentPoint(hdc, lpstr, cnt, &size);
    if (pcx)
        *pcx=size.cx;
    if (pcy)
        *pcy=size.cy;

    return fSuccess;
}


// these are the default colors used to map the dib colors
// to the current system colors

#define RGB_BUTTONTEXT      (RGB(000,000,000))  // black
#define RGB_BUTTONSHADOW    (RGB(128,128,128))  // dark grey
#define RGB_BUTTONFACE      (RGB(192,192,192))  // bright grey
#define RGB_BUTTONHILIGHT   (RGB(255,255,255))  // white
#define RGB_BACKGROUNDSEL   (RGB(000,000,255))  // blue
#define RGB_BACKGROUND      (RGB(255,000,255))  // magenta
#define FlipColor(rgb)      (RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb)))

#define MAX_COLOR_MAPS      16

// BUGBUG: can we just nuke this function and use LoadImage(..., LR_MAP3DCOLORS)???????

HBITMAP WINAPI CreateMappedBitmap(HINSTANCE hInstance, int idBitmap,
      UINT wFlags, LPCOLORMAP lpColorMap, int iNumMaps)
{
  HDC           hdc, hdcMem = NULL;
  HANDLE        h;
  DWORD FAR     *p;
  DWORD FAR     *lpTable;
  LPSTR         lpBits;
  HANDLE        hRes;
  LPBITMAPINFOHEADER    lpBitmapInfo;
  HBITMAP       hbm = NULL, hbmOld;
  int numcolors, i;
  int wid, hgt;
  HANDLE        hMunge;
  LPBITMAPINFOHEADER    lpMungeInfo;
  int           offBits;
  DWORD         rgbMaskTable[16];
  DWORD         rgbBackground;
  static const COLORMAP SysColorMap[] = {
    {RGB_BUTTONTEXT,    COLOR_BTNTEXT},     // black
    {RGB_BUTTONSHADOW,  COLOR_BTNSHADOW},   // dark grey
    {RGB_BUTTONFACE,    COLOR_BTNFACE},     // bright grey
    {RGB_BUTTONHILIGHT, COLOR_BTNHIGHLIGHT},// white
    {RGB_BACKGROUNDSEL, COLOR_HIGHLIGHT},   // blue
    {RGB_BACKGROUND,    COLOR_WINDOW}       // magenta
  };
  #define NUM_DEFAULT_MAPS (sizeof(SysColorMap)/sizeof(COLORMAP))
  COLORMAP DefaultColorMap[NUM_DEFAULT_MAPS];
  COLORMAP DIBColorMap[MAX_COLOR_MAPS];

  h = FindResource(hInstance, MAKEINTRESOURCE(idBitmap), RT_BITMAP);
  if (!h)
      return NULL;

  hRes = LoadResource(hInstance, h);

  /* Lock the bitmap and get a pointer to the color table. */
  lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);
  if (!lpBitmapInfo)
    return NULL;

  // munge on a copy of the color table instead of the original
  // (prevent possibility of "reload" with messed table
  offBits = (int)lpBitmapInfo->biSize + ((1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD));
  hMunge = GlobalAlloc(GMEM_MOVEABLE, offBits);
  if (!hMunge)
    goto Exit1;
  lpMungeInfo = GlobalLock(hMunge);
  hmemcpy(lpMungeInfo, lpBitmapInfo, offBits);

  /* Get system colors for the default color map */
  if (!lpColorMap) {
    lpColorMap = DefaultColorMap;
    iNumMaps = NUM_DEFAULT_MAPS;
    for (i=0; i < iNumMaps; i++) {
      lpColorMap[i].from = SysColorMap[i].from;
      lpColorMap[i].to = GetSysColor((int)SysColorMap[i].to);
    }
  }

  /* Transform RGB color map to a BGR DIB format color map */
  if (iNumMaps > MAX_COLOR_MAPS)
    iNumMaps = MAX_COLOR_MAPS;
  for (i=0; i < iNumMaps; i++) {
    DIBColorMap[i].to = FlipColor(lpColorMap[i].to);
    DIBColorMap[i].from = FlipColor(lpColorMap[i].from);
  }

  // use the table in the munging buffer
  lpTable = p = (DWORD FAR *)(((LPSTR)lpMungeInfo) + lpMungeInfo->biSize);

  /* Replace button-face and button-shadow colors with the current values
   */
  numcolors = 16;

  // if we are creating a mask, build a color table with white
  // marking the transparent section (where it used to be background)
  // and black marking the opaque section (everything else).  this
  // table is used below to build the mask using the original DIB bits.
  if (wFlags & CMB_MASKED) {
      rgbBackground = FlipColor(RGB_BACKGROUND);
      for (i = 0; i < 16; i++) {
          if (p[i] == rgbBackground)
              rgbMaskTable[i] = 0xFFFFFF;   // transparent section
          else
              rgbMaskTable[i] = 0x000000;   // opaque section
      }
  }

  while (numcolors-- > 0) {
      for (i = 0; i < iNumMaps; i++) {
          if ((*p & 0x00FFFFFF) == DIBColorMap[i].from) {
             *p = DIBColorMap[i].to;
             break;
          }
      }
      p++;
  }

  /* First skip over the header structure */
  lpBits = (LPSTR)(lpBitmapInfo) + offBits;

  /* Create a color bitmap compatible with the display device */
  i = wid = (int)lpBitmapInfo->biWidth;
  hgt = (int)lpBitmapInfo->biHeight;
  hdc = GetDC(NULL);
  hdcMem = CreateCompatibleDC(hdc);
  if (!hdcMem)
      goto cleanup;

  // if creating a mask, the bitmap needs to be twice as wide.
  if (wFlags & CMB_MASKED)
      i = wid*2;

// discardable bitmaps aren't much use anymore...
//
//  if (wFlags & CMB_DISCARDABLE)
//      hbm = CreateDiscardableBitmap(hdc, i, hgt);
//  else

      hbm = CreateCompatibleBitmap(hdc, i, hgt);
  if (hbm) {
      hbmOld = SelectObject(hdcMem, hbm);

      // set the main image
      StretchDIBits(hdcMem, 0, 0, wid, hgt, 0, 0, wid, hgt, lpBits,
                 (LPBITMAPINFO)lpMungeInfo, DIB_RGB_COLORS, SRCCOPY);

      // if building a mask, replace the DIB's color table with the
      // mask's black/white table and set the bits.  in order to
      // complete the masked effect, the actual image needs to be
      // modified so that it has the color black in all sections
      // that are to be transparent.
      if (wFlags & CMB_MASKED) {
          hmemcpy(lpTable, (DWORD FAR *)rgbMaskTable, 16 * sizeof(RGBQUAD));
          StretchDIBits(hdcMem, wid, 0, wid, hgt, 0, 0, wid, hgt, lpBits,
                 (LPBITMAPINFO)lpMungeInfo, DIB_RGB_COLORS, SRCCOPY);
          BitBlt(hdcMem, 0, 0, wid, hgt, hdcMem, wid, 0, 0x00220326);   // DSna
      }
      SelectObject(hdcMem, hbmOld);
  }

cleanup:
  if (hdcMem)
      DeleteObject(hdcMem);
  ReleaseDC(NULL, hdc);

  GlobalUnlock(hMunge);
  GlobalFree(hMunge);

Exit1:
  UnlockResource(hRes);
  FreeResource(hRes);

  return hbm;
}

// moved from shelldll\dragdrop.c

// should caller pass in message that indicates termination
// (WM_LBUTTONUP, WM_RBUTTONUP)?
//
// in:
//      hwnd    to do check on
//      x, y    in client coordinates
//
// returns:
//      TRUE    the user began to drag (moved mouse outside double click rect)
//      FALSE   mouse came up inside click rect
//
// BUGBUG, should support VK_ESCAPE to cancel

BOOL FAR PASCAL CheckForDragBegin(HWND hwnd, int x, int y)
{
    RECT    rc;
    MSG32   msg32;
    int dxClickRect, dyClickRect;

    dxClickRect = g_cxDoubleClk; // / 2;
    dyClickRect = g_cyDoubleClk; // / 2;

    // See if the user moves a certain number of pixels in any direction

    SetRect(&rc, x - dxClickRect, y - dyClickRect,
           x + dxClickRect, y + dyClickRect);

    MapWindowPoints(hwnd, NULL, (LPPOINT)&rc, 2);

    SetCapture(hwnd);
    do {
        if (PeekMessage32(&msg32, NULL, 0, 0, PM_REMOVE, TRUE))
        {
            // See if the application wants to process the message...
            if (CallMsgFilter32(&msg32, MSGF_COMMCTRL_BEGINDRAG, TRUE) != 0)
                continue;

            switch (msg32.message) {
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                ReleaseCapture();
                return FALSE;

            case WM_MOUSEMOVE:
                if (!PtInRect(&rc, msg32.pt)) {
                    ReleaseCapture();
                    return TRUE;
                }
                break;

            default:
                TranslateMessage32(&msg32, TRUE);
                DispatchMessage32(&msg32, TRUE);
                break;
            }
        }

        // WM_CANCELMODE messages will unset the capture, in that
        // case I want to exit this loop
    } while (GetCapture() == hwnd);

    return FALSE;
}

/* Regular StrToInt; stops at first non-digit. */

int WINAPI StrToInt(LPCSTR lpSrc)   // atoi()
{

#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')

    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == '-') {
        bNeg = TRUE;
    lpSrc++;
    }

    while (ISDIGIT(*lpSrc)) {
    n *= 10;
    n += *lpSrc - '0';
    lpSrc++;
    }
    return bNeg ? -n : n;
}

#undef StrToLong

#ifdef WIN32

LONG WINAPI StrToLong(LPCSTR lpSrc) // atoi()
{
    return StrToInt(lpSrc);
}

#else

/* Regular StrToLong; stops at first non-digit. */

LONG WINAPI StrToLong(LPCSTR lpSrc) // atoi()
{

#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')

    LONG n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == '-') {
        bNeg = TRUE;
    lpSrc++;
    }

    while (ISDIGIT(*lpSrc)) {
    n *= 10;
    n += *lpSrc - '0';
    lpSrc++;
    }
    return bNeg ? -n : n;

}
#endif

#pragma code_seg(CODESEG_INIT)

// wParam is from WM_WININICHANGE (new for chicago)

void FAR PASCAL InitGlobalMetrics(WPARAM wParam)
{
    // bug fix HACK: these are NOT members of USER's NONCLIENTMETRICS struct
    g_cxIcon = GetSystemMetrics(SM_CXICON);
    g_cyIcon = GetSystemMetrics(SM_CYICON);

    g_cxIconSpacing = GetSystemMetrics(SM_CXICONSPACING);
    g_cyIconSpacing = GetSystemMetrics(SM_CYICONSPACING);

    // BUGBUG: some of these are also not members of NONCLIENTMETRICS
    if ((wParam == 0) || (wParam == SPI_SETNONCLIENTMETRICS))
    {

        // REVIEW, make sure all these vars are used somewhere.
#ifndef WIN31
        NONCLIENTMETRICS ncm;

        g_cxEdge = GetSystemMetrics(SM_CXEDGE);
        g_cyEdge = GetSystemMetrics(SM_CYEDGE);
#else
        g_cxEdge = 2;
        g_cyEdge = 2;
#endif
        g_cxBorder = GetSystemMetrics(SM_CXBORDER);
        g_cyBorder = GetSystemMetrics(SM_CYBORDER);
        g_cxScreen = GetSystemMetrics(SM_CXSCREEN);
        g_cyScreen = GetSystemMetrics(SM_CYSCREEN);
        g_cxFrame  = GetSystemMetrics(SM_CXFRAME);
        g_cyFrame  = GetSystemMetrics(SM_CYFRAME);

#ifndef WIN31
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

        g_cxVScroll = g_cxScrollbar = (int)ncm.iScrollWidth;
        g_cyHScroll = g_cyScrollbar = (int)ncm.iScrollHeight;
#else
    g_cxVScroll = g_cxScrollbar = GetSystemMetrics(SM_CXVSCROLL);
    g_cyHScroll = g_cyScrollbar = GetSystemMetrics(SM_CYHSCROLL);
#endif

    // this is true for 4.0 modules only
    // for 3.x modules user lies and adds one to these values
    // Assert(g_cxVScroll == GetSystemMetrics(SM_CXVSCROLL));
    // Assert(g_cyHScroll == GetSystemMetrics(SM_CYHSCROLL));

        g_cxIconMargin = g_cxBorder * 8;
        g_cyIconMargin = g_cyBorder * 2;
        g_cyLabelSpace = g_cyIconMargin + (g_cyBorder * 2);
        g_cxLabelMargin = g_cxBorder * 2;

        g_cxDoubleClk = GetSystemMetrics(SM_CXDOUBLECLK);
        g_cyDoubleClk = GetSystemMetrics(SM_CYDOUBLECLK);
    }
}

void FAR PASCAL InitGlobalColors()
{
    g_clrWindow = GetSysColor(COLOR_WINDOW);
    g_clrWindowText = GetSysColor(COLOR_WINDOWTEXT);
    g_clrWindowFrame = GetSysColor(COLOR_WINDOWFRAME);
    g_clrGrayText = GetSysColor(COLOR_GRAYTEXT);
    g_clrBtnText = GetSysColor(COLOR_BTNTEXT);
    g_clrBtnFace = GetSysColor(COLOR_BTNFACE);
    g_clrBtnShadow = GetSysColor(COLOR_BTNSHADOW);
    g_clrBtnHighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
    g_clrHighlight = GetSysColor(COLOR_HIGHLIGHT);
    g_clrHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
#ifndef WIN31
    g_clrInfoText = GetSysColor(COLOR_INFOTEXT);
    g_clrInfoBk = GetSysColor(COLOR_INFOBK);

    g_hbrGrayText = GetSysColorBrush(COLOR_GRAYTEXT);
    g_hbrWindow = GetSysColorBrush(COLOR_WINDOW);
    g_hbrWindowText = GetSysColorBrush(COLOR_WINDOWTEXT);
    g_hbrWindowFrame = GetSysColorBrush(COLOR_WINDOWFRAME);
    g_hbrBtnFace = GetSysColorBrush(COLOR_BTNFACE);
    g_hbrBtnHighlight = GetSysColorBrush(COLOR_BTNHIGHLIGHT);
    g_hbrBtnShadow = GetSysColorBrush(COLOR_BTNSHADOW);
    g_hbrHighlight = GetSysColorBrush(COLOR_HIGHLIGHT);

#else   // WIN31

    if (!g_hbrGrayText)
    {
        g_clrInfoText = RGB(0,0,0);
        g_clrInfoBk = RGB(255,255,255);

        // Init these brushes if they haven't already been init'ed.
        g_hbrGrayText = CreateSolidBrush(g_clrGrayText);
        g_hbrWindow = CreateSolidBrush(g_clrWindow);
        g_hbrWindowText = CreateSolidBrush(g_clrWindowText);
        g_hbrWindowFrame = CreateSolidBrush(g_clrWindowFrame);
        g_hbrBtnFace = CreateSolidBrush(g_clrBtnFace);
        g_hbrBtnHighlight = CreateSolidBrush(g_clrBtnHighlight);
        g_hbrBtnShadow = CreateSolidBrush(g_clrBtnShadow);
        g_hbrHighlight = CreateSolidBrush(g_clrHighlight);
        g_hbrBtnText = CreateSolidBrush(g_clrBtnText);
        g_hbrWhite = CreateSolidBrush(RGB(255,255,255));
        g_hbrGray = CreateSolidBrush(RGB(127,127,127));
        g_hbrBlack = CreateSolidBrush(RGB(0,0,0));

        // these system colors don't exist for win31...
        g_hbr3DFace = CreateSolidBrush(g_clrBtnFace);
        g_hbr3DShadow = CreateSolidBrush(g_clrBtnShadow);
        g_hbr3DHilight = CreateSolidBrush(RGB_3DHILIGHT);
        g_hbr3DLight = CreateSolidBrush(RGB_3DLIGHT);
        g_hbr3DDkShadow = CreateSolidBrush(RGB_3DDKSHADOW);
    }
    // oem info for drawing routines
    {
        // Get the (Planes * BitCount) for the current device
        HDC hdcScreen = GetDC(NULL);
        g_oemInfo_Planes      = GetDeviceCaps(hdcScreen, PLANES);
        g_oemInfo_BitsPixel   = GetDeviceCaps(hdcScreen, BITSPIXEL);
        g_oemInfo_BitCount    = g_oemInfo_Planes * g_oemInfo_BitsPixel;
        ReleaseDC(NULL,hdcScreen);
    }

#endif  //WIN31

    g_hfontSystem = GetStockObject(SYSTEM_FONT);
}

void FAR PASCAL ReInitGlobalColors()
{
    DestroyGlobalColors();
    InitGlobalColors();
}

void FAR PASCAL DestroyGlobalColors()
{
#ifdef WIN31
    if (g_hbrGrayText)
        DeleteObject(g_hbrGrayText);
    if (g_hbrWindow)
        DeleteObject(g_hbrWindow);
    if (g_hbrWindowText)
        DeleteObject(g_hbrWindowText);
    if (g_hbrWindowFrame)
        DeleteObject(g_hbrWindowFrame);
    if (g_hbrBtnFace)
        DeleteObject(g_hbrBtnFace);
    if (g_hbrBtnHighlight)
        DeleteObject(g_hbrBtnHighlight);
    if (g_hbrBtnShadow)
        DeleteObject(g_hbrBtnShadow);
    if (g_hbrHighlight)
        DeleteObject(g_hbrHighlight);
    if (g_hbrBtnText)
        DeleteObject(g_hbrBtnText);
    if (g_hbrWhite)
        DeleteObject(g_hbrWhite);
    if (g_hbrGray)
        DeleteObject(g_hbrGray);
    if (g_hbrBlack)
        DeleteObject(g_hbrBlack);
    if (g_hbr3DFace)
        DeleteObject(g_hbr3DFace);
    if (g_hbr3DShadow)
        DeleteObject(g_hbr3DShadow);
    if (g_hbr3DHilight)
        DeleteObject(g_hbr3DHilight);
    if (g_hbr3DLight)
        DeleteObject(g_hbr3DLight);
    if (g_hbr3DDkShadow)
        DeleteObject(g_hbr3DDkShadow);

    g_hbrGrayText = NULL;
    g_hbrWindow = NULL;
    g_hbrWindowText = NULL;
    g_hbrWindowFrame = NULL;
    g_hbrBtnFace = NULL;
    g_hbrBtnHighlight = NULL;
    g_hbrBtnShadow = NULL;
    g_hbrHighlight = NULL;
    g_hbrBtnText = NULL;
    g_hbrWhite = NULL;
    g_hbrGray = NULL;
    g_hbrBlack = NULL;

    g_hbr3DFace = NULL;
    g_hbr3DShadow = NULL;
    g_hbr3DHilight = NULL;
    g_hbr3DLight = NULL;
    g_hbr3DDkShadow = NULL;
#endif  //WIN31

}

#pragma code_seg()

void FAR PASCAL RelayToToolTips(HWND hwndToolTips, HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    if(hwndToolTips) {
        MSG msg;
        msg.lParam = lParam;
        msg.wParam = wParam;
        msg.message = wMsg;
        msg.hwnd = hWnd;
        SendMessage(hwndToolTips, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msg);
    }
}

#define DT_SEARCHTIMEOUT    1000L   // 1 seconds
int g_iIncrSearchFailed = 0;

static LPSTR s_pszCharBuf = NULL;
static int s_ichCharBuf = 0;
static DWORD s_timeLast = 0L;
#ifdef  FE_IME
static BOOL s_fReplaceCompChar = FALSE;
#endif

int FAR PASCAL GetIncrementSearchString(LPSTR lpsz)
{
    if (GetMessageTime() - s_timeLast > DT_SEARCHTIMEOUT)
    {
        g_iIncrSearchFailed = 0;
        s_ichCharBuf = 0;
    }

    if (s_ichCharBuf && lpsz) {
        lstrcpyn(lpsz, s_pszCharBuf, s_ichCharBuf + 1);
        lpsz[s_ichCharBuf] = 0;
    }
    return s_ichCharBuf;
}

#ifdef  FE_IME
// Now only Korean version is interested in incremental search with composition string.
BOOL FAR PASCAL IncrementSearchImeCompStr(BOOL fCompStr, LPSTR lpszCompStr, LPSTR FAR *lplpstr)
{
    static int cbCharBuf = 0;
    BOOL fRestart = FALSE;

    if (!s_fReplaceCompChar && GetMessageTime() - s_timeLast > DT_SEARCHTIMEOUT)
    {
        g_iIncrSearchFailed = 0;
        s_ichCharBuf = 0;
    }

    if (s_ichCharBuf == 0)
    {
        fRestart = TRUE;
        s_fReplaceCompChar = FALSE;
    }
    s_timeLast = GetMessageTime();

    // Is there room for new character plus zero terminator?
    //
    if (!s_fReplaceCompChar && s_ichCharBuf + 2 + 1 > cbCharBuf)
    {
        LPSTR psz = ReAlloc(s_pszCharBuf, cbCharBuf + 16);
        if (!psz)
            return fRestart;

        cbCharBuf += 16;
        s_pszCharBuf = psz;
    }

    if (s_fReplaceCompChar)
    {
        if (lpszCompStr[0])
        {
            s_pszCharBuf[s_ichCharBuf-2] = lpszCompStr[0];
            s_pszCharBuf[s_ichCharBuf-1] = lpszCompStr[1];
            s_pszCharBuf[s_ichCharBuf] = 0;
        }
        else
        {
            s_ichCharBuf -= 2;
            s_pszCharBuf[s_ichCharBuf] = 0;
        }
    }
    else
    {
        s_pszCharBuf[s_ichCharBuf++] = lpszCompStr[0];
        s_pszCharBuf[s_ichCharBuf++] = lpszCompStr[1];
        s_pszCharBuf[s_ichCharBuf] = 0;
    }

    s_fReplaceCompChar = (fCompStr && lpszCompStr[0]);

    if (s_ichCharBuf == 2 && s_fReplaceCompChar)
        fRestart = TRUE;

    *lplpstr = s_pszCharBuf;

    return fRestart;

}
#endif

BOOL FAR PASCAL IncrementSearchString(UINT ch, LPSTR FAR *lplpstr)
{
    // BUGBUG:: review the use of all these statics.  Not a major problem
    // as basically we will not use them if we time out between characters
    // (1/4 second)
    static int cbCharBuf = 0;
    BOOL fRestart = FALSE;

    if (!ch) {
        s_ichCharBuf =0;
        g_iIncrSearchFailed = 0;
        return FALSE;
    }

    if (GetMessageTime() - s_timeLast > DT_SEARCHTIMEOUT)
    {
        g_iIncrSearchFailed = 0;
        s_ichCharBuf = 0;
    }

    if (s_ichCharBuf == 0)
        fRestart = TRUE;

    s_timeLast = GetMessageTime();

    // Is there room for new character plus zero terminator?
    //
    if (s_ichCharBuf + 1 + 1 > cbCharBuf)
    {
        LPSTR psz = ReAlloc(s_pszCharBuf, cbCharBuf + 16);
        if (!psz)
            return fRestart;

        cbCharBuf += 16;
        s_pszCharBuf = psz;
    }

    s_pszCharBuf[s_ichCharBuf++] = ch;
    s_pszCharBuf[s_ichCharBuf] = 0;

    *lplpstr = s_pszCharBuf;

    return fRestart;
}


// strips out the accelerators.  they CAN be the same buffers.
void PASCAL StripAccelerators(LPSTR lpszFrom, LPSTR lpszTo)
{

    BOOL fRet = FALSE;

    while ( *lpszTo = *lpszFrom ) {
#ifdef DBCS
        if (IsDBCSLeadByte(*lpszFrom)) {
            (*((WORD FAR*)lpszTo)) = (*((WORD FAR *)lpszFrom));
            lpszTo += 2;
            lpszFrom += 2;
            continue;
        }
        if ((*lpszFrom == '(') && (*(lpszFrom+1)==CH_PREFIX)){
            int i;
            for(i=0; i<4 && *lpszFrom;i++, lpszFrom++)
                ;


            if (*lpszFrom == '\0') {
                *lpszTo = 0;
                break;
            }
            continue;
        }
#endif

        if (*lpszFrom == '\t') {
            *lpszTo = 0;
            break;
        }

        if ( (*lpszFrom++ != CH_PREFIX) || (*lpszFrom == CH_PREFIX) ) {
            lpszTo++;
        }
    }
}

#ifdef IEWIN31_25
// common control info helpers
void FAR PASCAL CIInitialize(LPCONTROLINFO lpci, HWND hwnd, LPCREATESTRUCT lpcs)
{
    InitGlobalMetrics(0);
    lpci->hwnd = hwnd;
    lpci->hwndParent = lpcs->hwndParent;
    lpci->style = lpcs->style;
#if TODO
    lpci->uiCodePage = CP_ACP;
#endif

    lpci->bUnicode = (SendMessage (lpci->hwndParent, WM_NOTIFYFORMAT,
                                 (WPARAM)lpci->hwnd, NF_QUERY) == NFR_UNICODE);

}

LRESULT FAR PASCAL CIHandleNotifyFormat(LPCONTROLINFO lpci, LPARAM lParam)
{
    if (lParam == NF_QUERY) {
#ifdef UNICODE
        return NFR_UNICODE;
#else
        return NFR_ANSI;
#endif
    } else if (lParam == NF_REQUERY) {
        UINT uiResult;

        uiResult = SendMessage (lpci->hwndParent, WM_NOTIFYFORMAT,
                                (WPARAM)lpci->hwnd, NF_QUERY);

        lpci->bUnicode = (uiResult == NFR_UNICODE);

        return uiResult;
    }
    return 0;
}

BOOL CCForwardEraseBackground(HWND hwnd, HDC hdc)
{
    HWND hwndParent = GetParent(hwnd);
    LRESULT lres = 0;

    if (hwndParent)
    {
        // Adjust the origin so the parent paints in the right place
        POINT pt = {0,0};

        MapWindowPoints(hwnd, hwndParent, &pt, 1);
        OffsetWindowOrgEx(hdc, pt.x, pt.y, &pt);

        lres = SendMessage(hwndParent, WM_ERASEBKGND, (WPARAM) hdc, 0L);

        SetWindowOrgEx(hdc, pt.x, pt.y, NULL);
    }
    return(lres != 0);
}
#endif  //IEWIN31_25

