/*
**  CUTILS.C
**
**  Common utilities for common controls
**
*/

#include "ctlspriv.h"
#include "advpub.h"             // For REGINSTALL
#include <ntverp.h>
#include "ccver.h"              // App compat version hacks

#ifndef SSW_EX_IGNORESETTINGS
#define SSW_EX_IGNORESETTINGS   0x00040000  // ignore system settings to turn on/off smooth scroll
#endif


//
// Globals - REVIEW_32
//

BOOL g_fAnimate;
BOOL g_fSmoothScroll;

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
int g_cxSmIcon, g_cySmIcon;
int g_cxIconSpacing, g_cyIconSpacing;
int g_cxIconMargin, g_cyIconMargin;
int g_cyLabelSpace;
int g_cxLabelMargin;
int g_cxDoubleClk;
int g_cyDoubleClk;
int g_cxScrollbar;
int g_cyScrollbar;
int g_fDragFullWindows;


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
COLORREF g_clr3DDkShadow;
COLORREF g_clr3DLight;

HBRUSH g_hbrGrayText;
HBRUSH g_hbrWindow;
HBRUSH g_hbrWindowText;
HBRUSH g_hbrWindowFrame;
HBRUSH g_hbrBtnFace;
HBRUSH g_hbrBtnHighlight;
HBRUSH g_hbrBtnShadow;
HBRUSH g_hbrHighlight;


DWORD  g_dwHoverSelectTimeout;

HFONT g_hfontSystem;

#define CCS_ALIGN (CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM)

int TrueMapWindowPoints(HWND hwndFrom, HWND hwndTo, LPPOINT lppt, UINT cPoints);

// Note that the default alignment is CCS_BOTTOM
//
void FAR PASCAL NewSize(HWND hWnd, int nThickness, LONG style, int left, int top, int width, int height)
{
    // Resize the window unless the user said not to
    //
    if (!(style & CCS_NORESIZE))
    {
        RECT rc, rcWindow, rcBorder;

        // Remember size that was passed in and don't bother calling SetWindowPos if we're not
        // actually going to change the window size
        int leftSave = left;
        int topSave = top;
        int widthSave = width;
        int heightSave = height;

        // Calculate the borders around the client area of the status bar
        GetWindowRect(hWnd, &rcWindow);
        rcWindow.right -= rcWindow.left;  // -> dx
        rcWindow.bottom -= rcWindow.top;  // -> dy

        GetClientRect(hWnd, &rc);

        //
        // If the window is mirrored, mirror the anchor point
        // since it will be passed to SWP which accepts screen
        // ccordinates. This mainly fixes the display of status bar
        // and others. [samera]
        //
        if (IS_WINDOW_RTL_MIRRORED(hWnd))
        {
            TrueMapWindowPoints(hWnd, NULL, (LPPOINT)&rc.left, 1);
        }
        else
        {
            ClientToScreen(hWnd, (LPPOINT)&rc);
        }

        rcBorder.left = rc.left - rcWindow.left;
        rcBorder.top  = rc.top  - rcWindow.top ;
        rcBorder.right  = rcWindow.right  - rc.right  - rcBorder.left;
        rcBorder.bottom = rcWindow.bottom - rc.bottom - rcBorder.top ;

        if (style & CCS_VERT)
            nThickness += rcBorder.left + rcBorder.right;
        else
            nThickness += rcBorder.top + rcBorder.bottom;

        // Check whether to align to the parent window
        //
        if (style & CCS_NOPARENTALIGN)
        {
            // Check out whether this bar is top aligned or bottom aligned
            //
            switch (style & CCS_ALIGN)
            {
            case CCS_TOP:
            case CCS_NOMOVEY:
                break;

            default: // CCS_BOTTOM
                if(style & CCS_VERT)
                    left = left + width - nThickness;
                else
                    top = top + height - nThickness;
            }
        }
        else
        {
            // It is assumed there is a parent by default
            //
            GetClientRect(GetParent(hWnd), &rc);

            // Don't forget to account for the borders
            //
            if(style & CCS_VERT)
            {
                top = -rcBorder.right;
                height = rc.bottom + rcBorder.top + rcBorder.bottom;
            }
            else
            {
                left = -rcBorder.left;
                width = rc.right + rcBorder.left + rcBorder.right;
            }

            if ((style & CCS_ALIGN) == CCS_TOP)
            {
                if(style & CCS_VERT)
                    left = -rcBorder.left;
                else
                    top = -rcBorder.top;
            }
            else if ((style & CCS_ALIGN) != CCS_NOMOVEY)
            {
                if (style & CCS_VERT)
                    left = rc.right - nThickness + rcBorder.right;
                else
                    top = rc.bottom - nThickness + rcBorder.bottom;
            }
        }
        if (!(style & CCS_NOMOVEY) && !(style & CCS_NODIVIDER))
        {
            if (style & CCS_VERT)
                left += g_cxEdge;
            else
                top += g_cyEdge;      // double pixel edge thing
        }

        if(style & CCS_VERT)
            width = nThickness;
        else
            height = nThickness;

        SetWindowPos(hWnd, NULL, left, top, width, height, SWP_NOZORDER);
    }
}


BOOL FAR PASCAL MGetTextExtent(HDC hdc, LPCTSTR lpstr, int cnt, int FAR * pcx, int FAR * pcy)
{
    BOOL fSuccess;
    SIZE size = {0,0};
    
    if (cnt == -1)
        cnt = lstrlen(lpstr);
    
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

#ifdef UNIX
RGBQUAD CLR_TO_RGBQUAD( COLORREF clr)
{
    /* main modif for unix: keep the extra byte in the rgbReserved field
       This is used for our motif colors that are expressed in term of
       CMAPINDEX rather than real RGBs, this function is also portable
       and immune to endianness*/
    RGBQUAD rgbqResult;
    rgbqResult.rgbRed=GetRValue(clr);
    rgbqResult.rgbGreen=GetGValue(clr);
    rgbqResult.rgbBlue=GetBValue(clr);
    rgbqResult.rgbReserved=(BYTE)((clr>>24)&0xff);
    return rgbqResult;
}

COLORREF RGBQUAD_TO_CLR( RGBQUAD rgbQ )
{
    return ( ((DWORD)rgbQ.rgbRed) |  ((DWORD)(rgbQ.rgbGreen << 8)) | 
            ((DWORD)(rgbQ.rgbBlue << 16)) | ((DWORD)(rgbQ.rgbReserved << 24)) );
}

/* This is just plain wrong
   1) they definition of COLORMAP is based on COLORREFs but a
      DIB color map is RGBQUAD
   2) FlipColor as per previous definition does not flip at all
      since it goes from COLORREF to COLORREF
   so we are better doing nothing, so we dont loose our CMAP flag
   (Jose)
   */
#define FlipColor(rgb)      (rgb)
#else
#define FlipColor(rgb)      (RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb)))
#endif /* UNIX */


#define MAX_COLOR_MAPS      16

// This is almost the same as LoadImage(..., LR_MAP3DCOLORS) except that
//
//  -   The app can specify a custom color map,
//  -   The default color map maps colors beyond the 3D colors,
//  -   strange UNIX stuff happens that I'm afraid to mess with.
//
HBITMAP WINAPI CreateMappedBitmap(HINSTANCE hInstance, INT_PTR idBitmap,
      UINT wFlags, LPCOLORMAP lpColorMap, int iNumMaps)
{
  HDC                   hdc, hdcMem = NULL;
  HANDLE                h;
  COLOR_STRUCT FAR      *p;
  COLOR_STRUCT FAR      *lpTable;
  LPBYTE                lpBits;
  HANDLE                hRes;
  LPBITMAPINFOHEADER    lpBitmapInfo;
  HBITMAP               hbm = NULL, hbmOld;
  int numcolors, i;
  int wid, hgt;
  LPBITMAPINFOHEADER    lpMungeInfo;
  int                   offBits;
  COLOR_STRUCT          rgbMaskTable[16];
  COLOR_STRUCT          rgbBackground;

#ifdef UNIX
  const curColorRes = GetScreenDepth();
#endif /* UNIX */

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

#ifndef UNIX
  COLORMAP DIBColorMap[MAX_COLOR_MAPS];
#else
  COLORREF DIBColorRefs[MAX_COLOR_MAPS];
#endif

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
  lpMungeInfo = GlobalAlloc(GPTR, offBits);
  if (!lpMungeInfo)
        goto Exit1;
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


#ifndef UNIX
  /* IEUNIX
   1) their definition of COLORMAP is based on COLORREFs but a
      DIB color map is RGBQUAD
   2) FlipColor as per definition above does not flip at all
      since it goes from COLORREF to COLORREF
   so we are better doing nothing, this the(Jose)
   */
  for (i=0; i < iNumMaps; i++) {
    DIBColorMap[i].to = FlipColor(lpColorMap[i].to);
    DIBColorMap[i].from = FlipColor(lpColorMap[i].from);
  }
#endif /* !UNIX */

  // use the table in the munging buffer
  lpTable = p = (COLOR_STRUCT FAR *)(((LPBYTE)lpMungeInfo) + lpMungeInfo->biSize);

  /* Replace button-face and button-shadow colors with the current values
   */
  numcolors = 16;

#ifdef UNIX 
    for (i=0; i<numcolors; i++) {
        if (p[i].rgbReserved & 0x04)  // if colormap index
	   DIBColorRefs[i] = MwGetTrueRGBValue(RGBQUAD_TO_CLR(p[i]));
        else
	   DIBColorRefs[i] = RGB(p[i].rgbRed, p[i].rgbGreen, p[i].rgbBlue);
    }
#endif

  // if we are creating a mask, build a color table with white
  // marking the transparent section (where it used to be background)
  // and black marking the opaque section (everything else).  this
  // table is used below to build the mask using the original DIB bits.
#ifndef UNIX
  if (wFlags & CMB_MASKED) {
      rgbBackground = FlipColor(RGB_BACKGROUND);
      for (i = 0; i < 16; i++) {
          if (p[i] == rgbBackground)
              rgbMaskTable[i] = 0xFFFFFF;       // transparent section
          else
              rgbMaskTable[i] = 0x000000;       // opaque section
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
#else
  if (wFlags & CMB_MASKED) {

      /* IEUNIX - Varmac: later on change the below to use MwXPixel */
      for (i = 0; i < 16; i++) {
	  if (DIBColorRefs[i] == RGB_BACKGROUND) {
              rgbMaskTable[i].rgbRed = 0xFF;   // transparent section
              rgbMaskTable[i].rgbGreen = 0xFF;
              rgbMaskTable[i].rgbBlue = 0xFF;
	  } else {
	      rgbMaskTable[i].rgbRed = 0x00;   // opaque section
              rgbMaskTable[i].rgbGreen = 0x00;
              rgbMaskTable[i].rgbBlue = 0x00;
	  }
          rgbMaskTable[i].rgbReserved = 0;
      }
  }
  p += (numcolors-1);

  while (numcolors-- > 0) {
      for (i = 0; i < iNumMaps; i++) {
	  if (DIBColorRefs[numcolors] == lpColorMap[i].from) {
//	      if (GetScreenDepth() < 2)  {
//	          if  (lpColorMap[i].from != RGB_BUTTONTEXT) 
//			*p = CLR_TO_RGBQUAD(RGB(255,255,255));
//	      } else	
	        *p = CLR_TO_RGBQUAD(lpColorMap[i].to);
              break;
          }
      }
      p--;
  }
#endif /* !UNIX */

  /* First skip over the header structure */
  lpBits = (LPBYTE)(lpBitmapInfo) + offBits;

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
  if (wFlags & CMB_DIBSECTION)
  {
    // Have to edit the header slightly, since CreateDIBSection supports
    // only BI_RGB and BI_BITFIELDS.  This is the same whackery that USER
    // does in LoadImage.
    LPVOID pvDummy;
    DWORD dwCompression = lpMungeInfo->biCompression;
    if (dwCompression != BI_BITFIELDS)
      lpMungeInfo->biCompression = BI_RGB;
    hbm = CreateDIBSection(hdc, (LPBITMAPINFO)lpMungeInfo, DIB_RGB_COLORS,
                           &pvDummy, NULL, 0);
    lpMungeInfo->biCompression = dwCompression;
  }

  // If CMB_DIBSECTION failed, then create a DDB instead.  Not perfect,
  // but better than creating nothing.  We also get here if the caller
  // didn't ask for a DIB section.
  if (hbm == NULL)
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
#ifdef UNIX
          if (MwGetTrueRGBValue(GetSysColor(COLOR_WINDOW)) != 0xffffff)
#endif
          BitBlt(hdcMem, 0, 0, wid, hgt, hdcMem, wid, 0, 0x00220326);   // DSna
      }
      SelectObject(hdcMem, hbmOld);
  }

cleanup:
  if (hdcMem)
      DeleteObject(hdcMem);
  ReleaseDC(NULL, hdc);

  GlobalFree(lpMungeInfo);

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

BOOL PASCAL CheckForDragBegin(HWND hwnd, int x, int y)
{
    RECT rc;
    int dxClickRect = GetSystemMetrics(SM_CXDRAG);
    int dyClickRect = GetSystemMetrics(SM_CYDRAG);

    if (dxClickRect < 4)
    {
        dxClickRect = dyClickRect = 4;
    }

    // See if the user moves a certain number of pixels in any direction

    SetRect(&rc, x - dxClickRect, y - dyClickRect, x + dxClickRect, y + dyClickRect);
    MapWindowRect(hwnd, HWND_DESKTOP, &rc); // client -> screen

    //
    //  SUBTLE!  We use PeekMessage+WaitMessage instead of GetMessage,
    //  because WaitMessage will return when there is an incoming
    //  SendMessage, whereas GetMessage does not.  This is important,
    //  because the incoming message might've been WM_CAPTURECHANGED.
    //

    SetCapture(hwnd);
    do {
        MSG32 msg32;
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
                if (IsWindow(hwnd) && !PtInRect(&rc, msg32.pt)) {
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
        else WaitMessage();

        // WM_CANCELMODE messages will unset the capture, in that
        // case I want to exit this loop
    } while (IsWindow(hwnd) && GetCapture() == hwnd);

    return FALSE;
}


/* Regular StrToInt; stops at first non-digit. */

int WINAPI StrToInt(LPCTSTR lpSrc)      // atoi()
{

#define ISDIGIT(c)  ((c) >= TEXT('0') && (c) <= TEXT('9'))

    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == TEXT('-')) {
        bNeg = TRUE;
        lpSrc++;
    }

    while (ISDIGIT(*lpSrc)) {
        n *= 10;
        n += *lpSrc - TEXT('0');
        lpSrc++;
    }
    return bNeg ? -n : n;
}

#ifdef UNICODE

//
// Wrappers for StrToInt
//

int WINAPI StrToIntA(LPCSTR lpSrc)      // atoi()
{
    LPWSTR lpString;
    INT    iResult;

    lpString = ProduceWFromA (CP_ACP, lpSrc);

    if (!lpString) {
        return 0;
    }

    iResult = StrToIntW(lpString);

    FreeProducedString (lpString);

    return iResult;

}

#else

//
// Stub W version when Built ANSI
//
#ifndef UNIX
int WINAPI StrToIntW(LPCWSTR lpSrc)      // atoi()
{
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return 0;
}
#endif /* !UNIX */

#endif



#undef StrToLong

#ifdef WIN32

//
// No need to Unicode this since it is not
// exported.
//

LONG WINAPI StrToLong(LPCTSTR lpSrc)    // atoi()
{
    return StrToInt(lpSrc);
}

#else

/* Regular StrToLong; stops at first non-digit. */

LONG WINAPI StrToLong(LPCSTR lpSrc)     // atoi()
{

#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')

    LONG n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == TEXT('-')) {
        bNeg = TRUE;
        lpSrc++;
    }

    while (ISDIGIT(*lpSrc)) {
        n *= 10;
        n += *lpSrc - TEXT('0');
        lpSrc++;
    }
    return bNeg ? -n : n;

}
#endif

#pragma code_seg(CODESEG_INIT)

//
// From zmouse.h in the Magellan SDK
//

#define MSH_MOUSEWHEEL TEXT("MSWHEEL_ROLLMSG")

// Class name for Magellan/Z MSWHEEL window
// use FindWindow to get hwnd to MSWHEEL
#define MOUSEZ_CLASSNAME  TEXT("MouseZ")           // wheel window class
#define MOUSEZ_TITLE      TEXT("Magellan MSWHEEL") // wheel window title

#define MSH_WHEELMODULE_CLASS (MOUSEZ_CLASSNAME)
#define MSH_WHEELMODULE_TITLE (MOUSEZ_TITLE)

#define MSH_SCROLL_LINES  TEXT("MSH_SCROLL_LINES_MSG")

#define DI_GETDRAGIMAGE TEXT("ShellGetDragImage")       // Copied from Shlobj.w

UINT g_msgMSWheel;
UINT g_ucScrollLines = 3;                        /* default */
int  gcWheelDelta;
UINT g_uDragImages;

void FAR PASCAL InitGlobalMetrics(WPARAM wParam)
{
    static BOOL fInitMouseWheel;
    static HWND hwndMSWheel;
    static UINT msgMSWheelGetScrollLines;

    if (!fInitMouseWheel)
    {
        fInitMouseWheel = TRUE;

        if (g_bRunOnNT || g_bRunOnMemphis)
            g_msgMSWheel = WM_MOUSEWHEEL;
        else
        {
            g_msgMSWheel = RegisterWindowMessage(MSH_MOUSEWHEEL);
            msgMSWheelGetScrollLines = RegisterWindowMessage(MSH_SCROLL_LINES);
            hwndMSWheel = FindWindow(MSH_WHEELMODULE_CLASS, MSH_WHEELMODULE_TITLE);
        }
    }

    g_uDragImages = RegisterWindowMessage(DI_GETDRAGIMAGE);

#ifndef UNIX
    if (g_bRunOnNT || g_bRunOnMemphis)
    {
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &g_ucScrollLines, 0);
    }
    else if (hwndMSWheel && msgMSWheelGetScrollLines)
    {
        g_ucScrollLines =
              (UINT)SendMessage(hwndMSWheel, msgMSWheelGetScrollLines, 0, 0);
    }
#endif

    // bug fix HACK: these are NOT members of USER's NONCLIENTMETRICS struct
    g_cxIcon = GetSystemMetrics(SM_CXICON);
    g_cyIcon = GetSystemMetrics(SM_CYICON);
    g_cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    g_cySmIcon = GetSystemMetrics(SM_CYSMICON);

    g_cxIconSpacing = GetSystemMetrics( SM_CXICONSPACING );
    g_cyIconSpacing = GetSystemMetrics( SM_CYICONSPACING );

    // Full window drag stays off if running remotely
    if (!g_bRemoteSession &&
        (wParam == 0 || wParam == SPI_SETDRAGFULLWINDOWS)) {
        SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, sizeof(g_fDragFullWindows), &g_fDragFullWindows, 0);
    }

    // Smooth scrolling stays off if running remotely
    if (!g_bRemoteSession) {
        HKEY hkey;

        g_fSmoothScroll = TRUE;

        if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
            DWORD dwSize = sizeof(g_fSmoothScroll);
            RegQueryValueEx(hkey, TEXT("SmoothScroll"), 0, NULL, (LPBYTE)&g_fSmoothScroll, &dwSize);
            RegCloseKey(hkey);
        }
    }

    if (g_bRemoteSession)
    {
        // Nobody should've turned these on
        ASSERT(g_fDragFullWindows == FALSE);
        ASSERT(g_fSmoothScroll == FALSE);
    }

    // BUGBUG: some of these are also not members of NONCLIENTMETRICS
    if ((wParam == 0) || (wParam == SPI_SETNONCLIENTMETRICS))
    {
        NONCLIENTMETRICS ncm;

        // REVIEW, make sure all these vars are used somewhere.
        g_cxEdge = GetSystemMetrics(SM_CXEDGE);
        g_cyEdge = GetSystemMetrics(SM_CYEDGE);
        g_cxBorder = GetSystemMetrics(SM_CXBORDER);
        g_cyBorder = GetSystemMetrics(SM_CYBORDER);
        g_cxScreen = GetSystemMetrics(SM_CXSCREEN);
        g_cyScreen = GetSystemMetrics(SM_CYSCREEN);
        g_cxFrame  = GetSystemMetrics(SM_CXFRAME);
        g_cyFrame  = GetSystemMetrics(SM_CYFRAME);

        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

        g_cxVScroll = g_cxScrollbar = (int)ncm.iScrollWidth;
        g_cyHScroll = g_cyScrollbar = (int)ncm.iScrollHeight;

        // this is true for 4.0 modules only
        // for 3.x modules user lies and adds one to these values
        // ASSERT(g_cxVScroll == GetSystemMetrics(SM_CXVSCROLL));
        // ASSERT(g_cyHScroll == GetSystemMetrics(SM_CYHSCROLL));

        g_cxIconMargin = g_cxBorder * 8;
        g_cyIconMargin = g_cyEdge;
        g_cyLabelSpace = g_cyIconMargin + (g_cyEdge);
        g_cxLabelMargin = g_cxEdge;

        g_cxDoubleClk = GetSystemMetrics(SM_CXDOUBLECLK);
        g_cyDoubleClk = GetSystemMetrics(SM_CYDOUBLECLK);
    }

#if defined(UNIX)
    g_dwHoverSelectTimeout = 0;
#elif defined(WINNT) && defined(SPI_GETMOUSEHOVERTIME)
    //NT 4.0 has this SPI_GETMOUSEHOVERTIME
    SystemParametersInfo(SPI_GETMOUSEHOVERTIME, 0, &g_dwHoverSelectTimeout, 0);
#else
    // For Win95, we get this from the registry directly.
    {
        HKEY  hkey;
        DWORD dwType;

        g_dwHoverSelectTimeout = 0;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Mouse"), 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
            DWORD dwSize = sizeof(g_dwHoverSelectTimeout);
            if((RegQueryValueEx(hkey, TEXT("MouseHoverTime"), 0, &dwType, (LPBYTE)&g_dwHoverSelectTimeout, &dwSize) != ERROR_SUCCESS) ||
               (dwType != REG_DWORD))
                g_dwHoverSelectTimeout = 0;

            RegCloseKey(hkey);
        }
    }
#endif
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
    g_clrInfoText = GetSysColor(COLOR_INFOTEXT);
    g_clrInfoBk = GetSysColor(COLOR_INFOBK);
    g_clr3DDkShadow = GetSysColor(COLOR_3DDKSHADOW);
    g_clr3DLight = GetSysColor(COLOR_3DLIGHT);

    g_hbrGrayText = GetSysColorBrush(COLOR_GRAYTEXT);
    g_hbrWindow = GetSysColorBrush(COLOR_WINDOW);
    g_hbrWindowText = GetSysColorBrush(COLOR_WINDOWTEXT);
    g_hbrWindowFrame = GetSysColorBrush(COLOR_WINDOWFRAME);
    g_hbrBtnFace = GetSysColorBrush(COLOR_BTNFACE);
    g_hbrBtnHighlight = GetSysColorBrush(COLOR_BTNHIGHLIGHT);
    g_hbrBtnShadow = GetSysColorBrush(COLOR_BTNSHADOW);
    g_hbrHighlight = GetSysColorBrush(COLOR_HIGHLIGHT);
    g_hfontSystem = GetStockObject(SYSTEM_FONT);
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

#define DT_SEARCHTIMEOUT    1000L       // 1 seconds

__inline BOOL IsISearchTimedOut(PISEARCHINFO pis)
{
    return GetMessageTime() - pis->timeLast > DT_SEARCHTIMEOUT &&
           !IsFlagSet(g_dwPrototype, PTF_NOISEARCHTO);

}

int FAR PASCAL GetIncrementSearchString(PISEARCHINFO pis, LPTSTR lpsz)
{
    if (IsISearchTimedOut(pis))
    {
        pis->iIncrSearchFailed = 0;
        pis->ichCharBuf = 0;
    }

    if (pis->ichCharBuf && lpsz) {
        lstrcpyn(lpsz, pis->pszCharBuf, pis->ichCharBuf + 1);
        lpsz[pis->ichCharBuf] = TEXT('\0');
    }
    return pis->ichCharBuf;
}

#if defined(FE_IME) 
// Now only Korean version is interested in incremental search with composition string.
BOOL FAR PASCAL IncrementSearchImeCompStr(PISEARCHINFO pis, BOOL fCompStr, LPTSTR lpszCompStr, LPTSTR FAR *lplpstr)
{
    BOOL fRestart = FALSE;

    if (!pis->fReplaceCompChar && IsISearchTimedOut(pis))
    {
        pis->iIncrSearchFailed = 0;
        pis->ichCharBuf = 0;
    }

    if (pis->ichCharBuf == 0)
    {
        fRestart = TRUE;
        pis->fReplaceCompChar = FALSE;
    }
    pis->timeLast = GetMessageTime();

    // Is there room for new character plus zero terminator?
    //
#ifdef UNICODE
    if (!pis->fReplaceCompChar && pis->ichCharBuf + 1 + 1 > pis->cbCharBuf)
#else
    if (!pis->fReplaceCompChar && pis->ichCharBuf + 2 + 1 > pis->cbCharBuf)
#endif
    {
        LPTSTR psz = ReAlloc(pis->pszCharBuf, sizeof(TCHAR)*(pis->cbCharBuf + 16));
        if (!psz)
            return fRestart;

        pis->cbCharBuf += 16;
        pis->pszCharBuf = psz;
    }

    if (pis->fReplaceCompChar)
    {
        if (lpszCompStr[0])
        {
#ifdef UNICODE
            pis->pszCharBuf[pis->ichCharBuf-1] = lpszCompStr[0];
#else
            pis->pszCharBuf[pis->ichCharBuf-2] = lpszCompStr[0];
            pis->pszCharBuf[pis->ichCharBuf-1] = lpszCompStr[1];
#endif
            pis->pszCharBuf[pis->ichCharBuf] = 0;
        }
        else
        {
#ifdef UNICODE
            pis->ichCharBuf--;
#else
            pis->ichCharBuf -= 2;
#endif
            pis->pszCharBuf[pis->ichCharBuf] = 0;
        }
    }
    else
    {
#ifdef UNICODE
        pis->pszCharBuf[pis->ichCharBuf++] = lpszCompStr[0];
#else
        pis->pszCharBuf[pis->ichCharBuf++] = lpszCompStr[0];
        pis->pszCharBuf[pis->ichCharBuf++] = lpszCompStr[1];
#endif
        pis->pszCharBuf[pis->ichCharBuf] = 0;
    }

    pis->fReplaceCompChar = (fCompStr && lpszCompStr[0]);

#ifdef UNICODE
    if (pis->ichCharBuf == 1 && pis->fReplaceCompChar)
#else
    if (pis->ichCharBuf == 2 && pis->fReplaceCompChar)
#endif
        fRestart = TRUE;

    *lplpstr = pis->pszCharBuf;

    return fRestart;

}
#endif FE_IME

#ifdef UNICODE
/*
 * Thunk for LVM_GETISEARCHSTRINGA
 */
int FAR PASCAL GetIncrementSearchStringA(PISEARCHINFO pis, UINT uiCodePage, LPSTR lpsz)
{
    if (IsISearchTimedOut(pis))
    {
        pis->iIncrSearchFailed = 0;
        pis->ichCharBuf = 0;
    }

    if (pis->ichCharBuf && lpsz) {
        ConvertWToAN( uiCodePage, lpsz, pis->ichCharBuf, pis->pszCharBuf, pis->ichCharBuf );
        lpsz[pis->ichCharBuf] = '\0';
    }
    return pis->ichCharBuf;
}
#endif

// Beep only on the first failure.

void FAR PASCAL IncrementSearchBeep(PISEARCHINFO pis)
{
    if (!pis->iIncrSearchFailed)
    {
        pis->iIncrSearchFailed = TRUE;
        MessageBeep(0);
    }
}

//
//  IncrementSearchString - Add or clear the search string
//
//      ch == 0:  Reset the search string.  Return value meaningless.
//
//      ch != 0:  Append the character to the search string, starting
//                a new search string if we timed out the last one.
//                lplpstr receives the string so far.
//                Return value is TRUE if a new search string was
//                created, or FALSE if we appended to an existing one.
//

BOOL FAR PASCAL IncrementSearchString(PISEARCHINFO pis, UINT ch, LPTSTR FAR *lplpstr)
{
    BOOL fRestart = FALSE;

    if (!ch) {
        pis->ichCharBuf =0;
        pis->iIncrSearchFailed = 0;
        return FALSE;
    }

    if (IsISearchTimedOut(pis))
    {
        pis->iIncrSearchFailed = 0;
        pis->ichCharBuf = 0;
    }

    if (pis->ichCharBuf == 0)
        fRestart = TRUE;

    pis->timeLast = GetMessageTime();

    // Is there room for new character plus zero terminator?
    //
    if (pis->ichCharBuf + 1 + 1 > pis->cbCharBuf)
    {
        LPTSTR psz = ReAlloc(pis->pszCharBuf, ((pis->cbCharBuf + 16) * sizeof(TCHAR)));
        if (!psz)
            return fRestart;

        pis->cbCharBuf += 16;
        pis->pszCharBuf = psz;
    }

    pis->pszCharBuf[pis->ichCharBuf++] = (TCHAR)ch;
    pis->pszCharBuf[pis->ichCharBuf] = 0;

    *lplpstr = pis->pszCharBuf;

    return fRestart;
}

// strips out the accelerators.  they CAN be the same buffers.
void PASCAL StripAccelerators(LPTSTR lpszFrom, LPTSTR lpszTo, BOOL fAmpOnly)
{

    BOOL fRet = FALSE;

    while ( *lpszTo = *lpszFrom ) {
#if !defined(UNICODE)  //  && defined(DBCS)
        if (IsDBCSLeadByte(*lpszFrom)) {
            (*((WORD FAR*)lpszTo)) = (*((WORD FAR *)lpszFrom));
            lpszTo += 2;
            lpszFrom += 2;
            continue;
        }
#endif
        if (!fAmpOnly && (g_fDBCSInputEnabled))
        {
            if (*lpszFrom == TEXT('(') && *(lpszFrom+1)==CH_PREFIX)
            {
                int i;
                LPTSTR psz = lpszFrom+2;

                for(i=0; i<2 && *psz;i++, psz=FastCharNext(psz))
                    ;


                if (*psz == '\0') {
                    *lpszTo = 0;
                    break;
                }
                else if (i == 2 && *psz == TEXT(')'))
                {
                    lpszTo--;
                    lpszFrom = psz+1;
                    continue;
                }
            }
        }

        if (*lpszFrom == TEXT('\t')) {
            *lpszTo = TEXT('\0');
            break;
        }

        if ( (*lpszFrom++ != CH_PREFIX) || (*lpszFrom == CH_PREFIX) ) {
            lpszTo++;
        }
    }
}


void ScrollShrinkRect(int x, int y, LPRECT lprc)
{
    if (lprc) {
        if (x > 0) {
            lprc->left += x;
        } else {
            lprc->right += x;
        }

        if (y > 0) {
            lprc->top += y;
        } else {
            lprc->bottom += y;
        }

    }
}



// common control info helpers
void FAR PASCAL CIInitialize(LPCONTROLINFO lpci, HWND hwnd, LPCREATESTRUCT lpcs)
{
    lpci->hwnd = hwnd;
    lpci->hwndParent = lpcs->hwndParent;
    lpci->style = lpcs->style;
    lpci->uiCodePage = CP_ACP;
    lpci->dwExStyle = lpcs->dwExStyle;

    lpci->bUnicode = lpci->hwndParent &&
                     SendMessage (lpci->hwndParent, WM_NOTIFYFORMAT,
                                 (WPARAM)lpci->hwnd, NF_QUERY) == NFR_UNICODE;

#ifdef KEYBOARDCUES
    if (lpci->hwndParent)
    {
        LRESULT lRes = SendMessage(lpci->hwndParent, WM_QUERYUISTATE, 0, 0);
            lpci->wUIState = LOWORD(lRes);
    }
#endif
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
        LRESULT uiResult;

        uiResult = SendMessage (lpci->hwndParent, WM_NOTIFYFORMAT,
                                (WPARAM)lpci->hwnd, NF_QUERY);

        lpci->bUnicode = BOOLIFY(uiResult == NFR_UNICODE);

        return uiResult;
    }
    return 0;
}

UINT CCSwapKeys(WPARAM wParam, UINT vk1, UINT vk2)
{
    if (wParam == vk1)
        return vk2;
    if (wParam == vk2)
        return vk1;
    return (UINT)wParam;
}

UINT RTLSwapLeftRightArrows(CONTROLINFO *pci, WPARAM wParam)
{
    if (pci->dwExStyle & RTL_MIRRORED_WINDOW) {
        return CCSwapKeys(wParam, VK_LEFT, VK_RIGHT);
    }
    return (UINT)wParam;
}

//
//  New for v5.01:
//
//  Accessibility (and some other callers, sometimes even us) relies on
//  a XXM_GETITEM call filling the buffer and not just redirecting the
//  pointer.  Accessibility is particularly screwed by this because they
//  live outside the process, so the redirected pointer means nothing
//  to them.  Here, we copy the result back into the app buffer and return
//  the raw pointer.  The caller will return the raw pointer back to the
//  app, so the answer is in two places, either the app buffer, or in
//  the raw pointer.
//
//  Usage:
//
//      if (nm.item.mask & LVIF_TEXT)
//          pitem->pszText = CCReturnDispInfoText(nm.item.pszText,
//                              pitem->pszText, pitem->cchTextMax);
//
LPTSTR CCReturnDispInfoText(LPTSTR pszSrc, LPTSTR pszDest, UINT cchDest)
{
    // Test pszSrc != pszDest first since the common case is that they
    // are equal.
    if (pszSrc != pszDest && !IsFlagPtr(pszSrc) && !IsFlagPtr(pszDest))
        StrCpyN(pszDest, pszSrc, cchDest);
    return pszSrc;
}

#define SUBSCROLLS 100
#define abs(x) ( ( x > 0 ) ? x : -x)


#define DEFAULT_MAXSCROLLTIME ((GetDoubleClickTime() / 2) + 1)  // Ensure >= 1
#define DEFAULT_MINSCROLL 8
int SmoothScrollWindow(PSMOOTHSCROLLINFO psi)
{
    int dx = psi->dx;
    int dy = psi->dy;
    LPCRECT lprcSrc = psi->lprcSrc;
    LPCRECT lprcClip = psi->lprcClip;
    HRGN hrgnUpdate = psi->hrgnUpdate;
    LPRECT lprcUpdate = psi->lprcUpdate;
    UINT fuScroll = psi->fuScroll;
    int iRet = SIMPLEREGION;
    RECT rcUpdate;
    RECT rcSrc;
    RECT rcClip;
    int xStep;
    int yStep;
    int iSlicesDone = 0;
    int iSlices;
    DWORD dwTimeStart, dwTimeNow;
    HRGN hrgnLocalUpdate;
    UINT cxMinScroll = psi->cxMinScroll;
    UINT cyMinScroll = psi->cyMinScroll;
    UINT uMaxScrollTime = psi->uMaxScrollTime;
    int iSubScrolls;
    PFNSMOOTHSCROLLPROC pfnScrollProc;

    if (!lprcUpdate)
        lprcUpdate = &rcUpdate;
    SetRectEmpty(lprcUpdate);

    if (psi->cbSize != sizeof(SMOOTHSCROLLINFO))
        return 0;

    // check the defaults
    if (!(psi->fMask & SSIF_MINSCROLL )
        || cxMinScroll == SSI_DEFAULT)
        cxMinScroll = DEFAULT_MINSCROLL;

    if (!(psi->fMask & SSIF_MINSCROLL)
        || cyMinScroll == SSI_DEFAULT)
        cyMinScroll = DEFAULT_MINSCROLL;

    if (!(psi->fMask & SSIF_MAXSCROLLTIME)
        || uMaxScrollTime == SSI_DEFAULT)
        uMaxScrollTime = DEFAULT_MAXSCROLLTIME;

    if (uMaxScrollTime < SUBSCROLLS)
        uMaxScrollTime = SUBSCROLLS;


    if ((!(fuScroll & SSW_EX_IGNORESETTINGS)) &&
        (!g_fSmoothScroll)) {
        fuScroll |= SSW_EX_IMMEDIATE;
    }

    if ((psi->fMask & SSIF_SCROLLPROC) && psi->pfnScrollProc) {
        pfnScrollProc = psi->pfnScrollProc;
    } else {
        pfnScrollProc = ScrollWindowEx;
    }

#ifdef ScrollWindowEx
#undef ScrollWindowEx
#endif

    if (fuScroll & SSW_EX_IMMEDIATE) {
        return pfnScrollProc(psi->hwnd, dx, dy, lprcSrc, lprcClip, hrgnUpdate,
                             lprcUpdate, LOWORD(fuScroll));
    }

    // copy input rects locally
    if (lprcSrc)  {
        rcSrc = *lprcSrc;
        lprcSrc = &rcSrc;
    }
    if (lprcClip) {
        rcClip = *lprcClip;
        lprcClip = &rcClip;
    }

    if (!hrgnUpdate)
        hrgnLocalUpdate = CreateRectRgn(0,0,0,0);
    else
        hrgnLocalUpdate = hrgnUpdate;

    //set up initial vars
    dwTimeStart = GetTickCount();

    if (fuScroll & SSW_EX_NOTIMELIMIT) {
        xStep = cxMinScroll * (dx < 0 ? -1 : 1);
        yStep = cyMinScroll * (dy < 0 ? -1 : 1);
    } else {
        iSubScrolls = (uMaxScrollTime / DEFAULT_MAXSCROLLTIME) * SUBSCROLLS;
        if (!iSubScrolls)
            iSubScrolls = SUBSCROLLS;
        xStep = dx / iSubScrolls;
        yStep = dy / iSubScrolls;
    }

    if (xStep == 0 && dx)
        xStep = dx < 0 ? -1 : 1;

    if (yStep == 0 && dy)
        yStep = dy < 0 ? -1 : 1;

    while (dx || dy) {
        int x,y;
        RECT rcTempUpdate;

        if (fuScroll & SSW_EX_NOTIMELIMIT) {
            x = xStep;
            y = yStep;
            if (abs(x) > abs(dx))
                x = dx;

            if (abs(y) > abs(dy))
                y = dy;

        } else {
            int iTimePerScroll = uMaxScrollTime / iSubScrolls;
            if (!iTimePerScroll)
                iTimePerScroll = 1;
            
            dwTimeNow = GetTickCount();

            iSlices = ((dwTimeNow - dwTimeStart) / iTimePerScroll) - iSlicesDone;
            if (iSlices < 0)
                iSlices = 0;


            do {

                int iRet = 0;

                iSlices++;
                if ((iSlicesDone + iSlices) <= iSubScrolls) {
                    x = xStep * iSlices;
                    y = yStep * iSlices;

                    // this could go over if we rounded ?Step up to 1(-1) above
                    if (abs(x) > abs(dx))
                        x = dx;

                    if (abs(y) > abs(dy))
                        y = dy;

                } else {
                    x = dx;
                    y = dy;
                }

                //DebugMsg(DM_TRACE, "SmoothScrollWindowCallback %d", iRet);

                if (x == dx && y == dy)
                    break;

                if ((((UINT)(abs(x)) >= cxMinScroll) || !x) &&
                    (((UINT)(abs(y)) >= cyMinScroll) || !y))
                    break;

            } while (1);
        }

        if (pfnScrollProc(psi->hwnd, x, y, lprcSrc, lprcClip, hrgnLocalUpdate, &rcTempUpdate, LOWORD(fuScroll)) == ERROR) {
            iRet = ERROR;
            goto Bail;
        }

        // we don't need to do this always because if iSlices >= iSlicesDone, we'll have scrolled blanks
        //if (iSlices < iSlicesDone)
        RedrawWindow(psi->hwnd, NULL, hrgnLocalUpdate, RDW_ERASE | RDW_ERASENOW | RDW_INVALIDATE);

        UnionRect(lprcUpdate, &rcTempUpdate, lprcUpdate);

        ScrollShrinkRect(x,y, (LPRECT)lprcSrc);
        ScrollShrinkRect(x,y, (LPRECT)lprcClip);

        dx -= x;
        dy -= y;
        iSlicesDone += iSlices;
    }

Bail:

    if (fuScroll & SW_SCROLLCHILDREN) {
        RedrawWindow(psi->hwnd, lprcUpdate, NULL, RDW_INVALIDATE);
    }

    if (hrgnLocalUpdate != hrgnUpdate)
        DeleteObject(hrgnLocalUpdate);

    return iRet;
}



typedef BOOL (WINAPI *PLAYSOUNDFN)(LPCTSTR lpsz, HANDLE hMod, DWORD dwFlags);
typedef UINT (WINAPI *UINTVOIDFN)();

TCHAR const c_szWinMMDll[] = TEXT("winmm.dll");
#ifdef UNICODE
char const c_szPlaySound[] = "PlaySoundW";
#else
char const c_szPlaySound[] = "PlaySoundA";
#endif
char const c_szwaveOutGetNumDevs[] = "waveOutGetNumDevs";
extern TCHAR const c_szExplorer[];

#define CCH_KEYMAX 256
BOOL g_fNeverPlaySound = FALSE;

void CCPlaySound(LPCTSTR lpszName)
{
    TCHAR szFileName[MAX_PATH];
    LONG cbSize = SIZEOF(szFileName);
    TCHAR szKey[CCH_KEYMAX];

    if (g_fNeverPlaySound)
        return;

    // check the registry first
    // if there's nothing registered, we blow off the play,
    // but we don't set the MM_DONTLOAD flag so taht if they register
    // something we will play it
    wsprintf(szKey, TEXT("AppEvents\\Schemes\\Apps\\.Default\\%s\\.current"), lpszName);
    if ((RegQueryValue(HKEY_CURRENT_USER, szKey, szFileName, &cbSize) == ERROR_SUCCESS) &&
        (cbSize > SIZEOF(szFileName[0]))) {

        PLAYSOUNDFN pfnPlaySound;
        UINTVOIDFN pfnwaveOutGetNumDevs;

        HANDLE hMM;
    
        hMM = GetModuleHandle(c_szWinMMDll);
        if (!hMM)
            hMM = LoadLibrary(c_szWinMMDll);
    
        if (!hMM)
            return;
    
        /// are there any devices?
        pfnwaveOutGetNumDevs = (UINTVOIDFN)GetProcAddress(hMM, c_szwaveOutGetNumDevs);
        pfnPlaySound = (PLAYSOUNDFN)GetProcAddress(hMM, c_szPlaySound);
        if (!pfnPlaySound || !pfnwaveOutGetNumDevs || !pfnwaveOutGetNumDevs()) {
            g_fNeverPlaySound = TRUE;
            return;
        }

        pfnPlaySound(szFileName, NULL, SND_FILENAME | SND_ASYNC);
    }
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
        OffsetWindowOrgEx(hdc, 
                          pt.x, 
                          pt.y, 
                          &pt);

        lres = SendMessage(hwndParent, WM_ERASEBKGND, (WPARAM) hdc, 0L);

        SetWindowOrgEx(hdc, pt.x, pt.y, NULL);
    }
    return(lres != 0);
}

HFONT CCGetHotFont(HFONT hFont, HFONT *phFontHot)
{
    if (!*phFontHot) {
        LOGFONT lf;

        // create the underline font
        GetObject(hFont, sizeof(lf), &lf);
#ifndef DONT_UNDERLINE
        lf.lfUnderline = TRUE;
#endif
        *phFontHot = CreateFontIndirect(&lf);
    }
    return *phFontHot;
}


HFONT CCCreateStatusFont(void)
{
    NONCLIENTMETRICS ncm;

    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    return CreateFontIndirect(&ncm.lfStatusFont);
}

void* CCLocalReAlloc(void* p, UINT uBytes)
{
    if (uBytes) {
        if (p) {
            return LocalReAlloc(p, uBytes, LMEM_MOVEABLE | LMEM_ZEROINIT);
        } else {
            return LocalAlloc(LPTR, uBytes);
        }
    } else {
        if (p)
            LocalFree(p);
        return NULL;
    }
}

/*----------------------------------------------------------
Purpose: This function provides the commctrl version info.  This
         allows the caller to distinguish running NT SUR vs.
         Win95 shell vs. Nashville, etc.

         This API was not supplied in Win95 or NT SUR, so
         the caller must GetProcAddress it.  If this fails,
         the caller is running on Win95 or NT SUR.

Returns: NO_ERROR
         ERROR_INVALID_PARAMETER if pinfo is invalid

Cond:    --
*/

// All we have to do is declare this puppy and CCDllGetVersion does the rest
// Note that we use VER_FILEVERSION_DW because comctl32 uses a funky
// version scheme
DLLVER_DUALBINARY(VER_FILEVERSION_DW, VER_PRODUCTBUILD_QFE);

//
// Translate the given font to a code page used for thunking text
//
UINT GetCodePageForFont (HFONT hFont)
{
#ifdef WINNT
    LOGFONT lf;
    TCHAR szFontName[MAX_PATH];
    CHARSETINFO csi;
    DWORD dwSize, dwType;
    HKEY hKey;


    if (!GetObject (hFont, sizeof(lf), &lf)) {
        return CP_ACP;
    }


    //
    // Check for font substitutes
    //

    lstrcpy (szFontName, lf.lfFaceName);

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes"),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = MAX_PATH * sizeof(TCHAR);
        RegQueryValueEx (hKey, lf.lfFaceName, NULL, &dwType,
                         (LPBYTE) szFontName, &dwSize);

        RegCloseKey (hKey);
    }


    //
    //  This is to fix office for locales that use non 1252 versions
    //  of Ms Sans Serif and Ms Serif.  These fonts incorrectly identify
    //  themselves as having an Ansi charset, so TranslateCharsetInfo will
    //  return the wrong value.
    //
    //  NT bug 260697: Office 2000 uses Tahoma.
    //
    if ((lf.lfCharSet == ANSI_CHARSET) &&
        (!lstrcmpi(L"Helv", szFontName) ||
         !lstrcmpi(L"Ms Sans Serif", szFontName) ||
         !lstrcmpi(L"Ms Serif", szFontName) ||
         !lstrcmpi(L"Tahoma", szFontName)))
    {
        return CP_ACP;
    }
    //
    //  This is to fix FE office95a and Pro. msofe95.dll sets wrong charset when create
    //  listview control. so TranslateCharsetInfo will return the wrong value.
    //  Korea  : DotumChe.
    //  Taiwan : New MingLight
    //  China  : SongTi

    if ((lf.lfCharSet == SHIFTJIS_CHARSET) &&
        (!lstrcmpi(L"\xb3cb\xc6c0\xccb4", lf.lfFaceName))        || // Korea
        (!lstrcmpi(L"\x65b0\x7d30\x660e\x9ad4", lf.lfFaceName))  || // Taiwan
        (!lstrcmpi(L"\x5b8b\x4f53", lf.lfFaceName)))                // PRC
    {
        return CP_ACP;
    }

    if (!TranslateCharsetInfo((DWORD FAR *) lf.lfCharSet, &csi, TCI_SRCCHARSET)) {
        return CP_ACP;
    }

    return csi.ciACP;
#else

    return CP_ACP;

#endif
}

typedef void (CALLBACK* NOTIFYWINEVENTPROC)(UINT, HWND, LONG, LONG_PTR);

#define DONOTHING_NOTIFYWINEVENT ((NOTIFYWINEVENTPROC)1)

// --------------------------------------------------------------------------
//
//  MyNotifyWinEvent()
//
//  This tries to get the proc address of NotifyWinEvent().  If it fails, we
//  remember that and do nothing.
//
//  NOTE TO NT FOLKS:
//  Don't freak out about this code.  It will do nothing on NT, nothing yet
//  that is.  Active Accessibility will be ported to NT for Service Pack #1
//  or at worst #2 after NT SUR ships, this code will work magically when
//  that is done/
//
// --------------------------------------------------------------------------
void MyNotifyWinEvent(UINT event, HWND hwnd, LONG idContainer, LONG_PTR idChild)
{
    static NOTIFYWINEVENTPROC s_pfnNotifyWinEvent = NULL;

    if (!s_pfnNotifyWinEvent)
    {
        HMODULE hmod;

        if (hmod = GetModuleHandle(TEXT("USER32")))
            s_pfnNotifyWinEvent = (NOTIFYWINEVENTPROC)GetProcAddress(hmod,
                "NotifyWinEvent");

        if (!s_pfnNotifyWinEvent)
            s_pfnNotifyWinEvent = DONOTHING_NOTIFYWINEVENT;
    }

    if (s_pfnNotifyWinEvent != DONOTHING_NOTIFYWINEVENT)
        (* s_pfnNotifyWinEvent)(event, hwnd, idContainer, idChild);
}


LONG GetMessagePosClient(HWND hwnd, LPPOINT ppt)
{
    LPARAM lParam;
    POINT pt;
    if (!ppt)
        ppt = &pt;
    
    lParam = GetMessagePos();
    ppt->x = GET_X_LPARAM(lParam);
    ppt->y = GET_Y_LPARAM(lParam);
    ScreenToClient(hwnd, ppt);

    return MAKELONG(ppt->x, ppt->y);
}


LPTSTR StrDup(LPCTSTR lpsz)
{
    LPTSTR lpszRet = (LPTSTR)LocalAlloc(LPTR, (lstrlen(lpsz) + 1) * sizeof(TCHAR));
    if (lpszRet) {
        lstrcpy(lpszRet, lpsz);
    }
    return lpszRet;
}

#ifdef UNICODE
LPSTR StrDupA(LPCSTR lpsz)
{
    LPSTR lpszRet = (LPSTR)LocalAlloc(LPTR, (lstrlenA(lpsz) + 1) * sizeof(CHAR));
    if (lpszRet) {
        lstrcpyA(lpszRet, lpsz);
    }
    return lpszRet;
}

#endif

HWND GetDlgItemRect(HWND hDlg, int nIDItem, LPRECT prc) //relative to hDlg
{
    HWND hCtrl = NULL;
    if (prc)
    {
        hCtrl = GetDlgItem(hDlg, nIDItem);
        if (hCtrl)
        {
            GetWindowRect(hCtrl, prc);
            MapWindowRect(NULL, hDlg, prc);
        }
        else
            SetRectEmpty(prc);
    }
    return hCtrl;
} 


/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.

*/
HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            hr = pfnri(g_hinst, szSection, NULL);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}


/*----------------------------------------------------------
Purpose: Install/uninstall user settings

*/
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    ASSERT(IS_VALID_STRING_PTRW(pszCmdLine, -1));

#ifdef DEBUG
    if (IsFlagSet(g_dwBreakFlags, BF_ONAPIENTER))
    {
        TraceMsg(TF_ALWAYS, "Stopping in DllInstall");
        DEBUG_BREAK;
    }
#endif

    if (bInstall)
    {
        // Delete any old registration entries, then add the new ones.
        // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
        // (The inf engine doesn't guarantee DelReg/AddReg order, that's
        // why we explicitly unreg and reg here.)
        //
        CallRegInstall("RegDll");
    }
    else
    {
        CallRegInstall("UnregDll");
    }

    return S_OK;    
}    



//---------------------------------------------------------------------------------------
void FAR PASCAL FlipRect(LPRECT prc)
{
    SWAP(prc->left, prc->top, int);
    SWAP(prc->right, prc->bottom, int);
}


//---------------------------------------------------------------------------------------
//
//  Returns previous window bits.

DWORD SetWindowBits(HWND hWnd, int iWhich, DWORD dwBits, DWORD dwValue)
{
    DWORD dwStyle;
    DWORD dwNewStyle;

    dwStyle = GetWindowLong(hWnd, iWhich);
    dwNewStyle = ( dwStyle & ~dwBits ) | (dwValue & dwBits);
    if (dwStyle != dwNewStyle) {
        dwStyle = SetWindowLong(hWnd, iWhich, dwNewStyle);
    }
    return dwStyle;
}

//---------------------------------------------------------------------------------------

BOOL CCDrawEdge(HDC hdc, LPRECT lprc, UINT edge, UINT flags, LPCOLORSCHEME lpclrsc)
{
    RECT    rc, rcD;
    UINT    bdrType;
    COLORREF clrTL, clrBR;    

    //
    // Enforce monochromicity and flatness
    //    

    // if (oemInfo.BitCount == 1)
    //    flags |= BF_MONO;
    if (flags & BF_MONO)
        flags |= BF_FLAT;    

    CopyRect(&rc, lprc);

    //
    // Draw the border segment(s), and calculate the remaining space as we
    // go.
    //
    if (bdrType = (edge & BDR_OUTER))
    {
DrawBorder:
        //
        // Get colors.  Note the symmetry between raised outer, sunken inner and
        // sunken outer, raised inner.
        //

        if (flags & BF_FLAT)
        {
            if (flags & BF_MONO)
                clrBR = (bdrType & BDR_OUTER) ? g_clrWindowFrame : g_clrWindow;
            else
                clrBR = (bdrType & BDR_OUTER) ? g_clrBtnShadow: g_clrBtnFace;
            
            clrTL = clrBR;
        }
        else
        {
            // 5 == HILIGHT
            // 4 == LIGHT
            // 3 == FACE
            // 2 == SHADOW
            // 1 == DKSHADOW

            switch (bdrType)
            {
                // +2 above surface
                case BDR_RAISEDOUTER:           // 5 : 4
                    clrTL = ((flags & BF_SOFT) ? g_clrBtnHighlight : g_clr3DLight);
                    clrBR = g_clr3DDkShadow;     // 1
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnHighlight;
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnShadow;
                    }                                            
                    break;

                // +1 above surface
                case BDR_RAISEDINNER:           // 4 : 5
                    clrTL = ((flags & BF_SOFT) ? g_clr3DLight : g_clrBtnHighlight);
                    clrBR = g_clrBtnShadow;       // 2
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnHighlight;
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnShadow;
                    }                                            
                    break;

                // -1 below surface
                case BDR_SUNKENOUTER:           // 1 : 2
                    clrTL = ((flags & BF_SOFT) ? g_clr3DDkShadow : g_clrBtnShadow);
                    clrBR = g_clrBtnHighlight;      // 5
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnShadow;
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnHighlight;                        
                    }
                    break;

                // -2 below surface
                case BDR_SUNKENINNER:           // 2 : 1
                    clrTL = ((flags & BF_SOFT) ? g_clrBtnShadow : g_clr3DDkShadow);
                    clrBR = g_clr3DLight;        // 4
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnShadow;
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnHighlight;                        
                    }
                    break;

                default:
                    return(FALSE);
            }
        }

        //
        // Draw the sides of the border.  NOTE THAT THE ALGORITHM FAVORS THE
        // BOTTOM AND RIGHT SIDES, since the light source is assumed to be top
        // left.  If we ever decide to let the user set the light source to a
        // particular corner, then change this algorithm.
        //
            
        // Bottom Right edges
        if (flags & (BF_RIGHT | BF_BOTTOM))
        {            
            // Right
            if (flags & BF_RIGHT)
            {       
                rc.right -= g_cxBorder;
                // PatBlt(hdc, rc.right, rc.top, g_cxBorder, rc.bottom - rc.top, PATCOPY);
                rcD.left = rc.right;
                rcD.right = rc.right + g_cxBorder;
                rcD.top = rc.top;
                rcD.bottom = rc.bottom;

                FillRectClr(hdc, &rcD, clrBR);
            }
            
            // Bottom
            if (flags & BF_BOTTOM)
            {
                rc.bottom -= g_cyBorder;
                // PatBlt(hdc, rc.left, rc.bottom, rc.right - rc.left, g_cyBorder, PATCOPY);
                rcD.left = rc.left;
                rcD.right = rc.right;
                rcD.top = rc.bottom;
                rcD.bottom = rc.bottom + g_cyBorder;

                FillRectClr(hdc, &rcD, clrBR);
            }
        }
        
        // Top Left edges
        if (flags & (BF_TOP | BF_LEFT))
        {
            // Left
            if (flags & BF_LEFT)
            {
                // PatBlt(hdc, rc.left, rc.top, g_cxBorder, rc.bottom - rc.top, PATCOPY);
                rc.left += g_cxBorder;

                rcD.left = rc.left - g_cxBorder;
                rcD.right = rc.left;
                rcD.top = rc.top;
                rcD.bottom = rc.bottom; 

                FillRectClr(hdc, &rcD, clrTL);
            }
            
            // Top
            if (flags & BF_TOP)
            {
                // PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, g_cyBorder, PATCOPY);
                rc.top += g_cyBorder;

                rcD.left = rc.left;
                rcD.right = rc.right;
                rcD.top = rc.top - g_cyBorder;
                rcD.bottom = rc.top;

                FillRectClr(hdc, &rcD, clrTL);
            }
        }
        
    }

    if (bdrType = (edge & BDR_INNER))
    {
        //
        // Strip this so the next time through, bdrType will be 0.
        // Otherwise, we'll loop forever.
        //
        edge &= ~BDR_INNER;
        goto DrawBorder;
    }

    //
    // Fill the middle & clean up if asked
    //
    if (flags & BF_MIDDLE)    
        FillRectClr(hdc, &rc, (flags & BF_MONO) ? g_clrWindow : g_clrBtnFace);

    if (flags & BF_ADJUST)
        CopyRect(lprc, &rc);

    return(TRUE);
}

//---------------------------------------------------------------------------------------
//CCInvalidateFrame -- SWP_FRAMECHANGED, w/o all the extra params
//
void CCInvalidateFrame(HWND hwnd)
{
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    return;
}

//---------------------------------------------------------------------------------------
// FlipPoint - flip the x and y coordinates of a point
//
void FlipPoint(LPPOINT lppt)
{
    SWAP(lppt->x, lppt->y, int);
}

//
//  When we want to turn a tooltip into an infotip, we set its
//  width to 300 "small pixels", where there are 72 small pixels
//  per inch when you are in small fonts mode.
//
//  Scale this value based on the magnification in effect
//  on the owner's monitor.  But never let the tooltip get
//  bigger than 3/4 of the screen.
//
void CCSetInfoTipWidth(HWND hwndOwner, HWND hwndToolTips)
{
    HDC hdc = GetDC(hwndOwner);
    int iWidth = MulDiv(GetDeviceCaps(hdc, LOGPIXELSX), 300, 72);
    int iMaxWidth = GetDeviceCaps(hdc, HORZRES) * 3 / 4;
    SendMessage(hwndToolTips, TTM_SETMAXTIPWIDTH, 0, min(iWidth, iMaxWidth));
    ReleaseDC(hwndOwner, hdc);
}

// Mirror a bitmap in a DC (mainly a text object in a DC)
//
// [samera]
//
void MirrorBitmapInDC( HDC hdc , HBITMAP hbmOrig )
{
  HDC     hdcMem;
  HBITMAP hbm;
  BITMAP  bm;


  if( !GetObject( hbmOrig , sizeof(BITMAP) , &bm ))
    return;

  hdcMem = CreateCompatibleDC( hdc );

  if( !hdcMem )
    return;

  hbm = CreateCompatibleBitmap( hdc , bm.bmWidth , bm.bmHeight );

  if( !hbm )
  {
    DeleteDC( hdcMem );
    return;
  }

  //
  // Flip the bitmap
  //
  SelectObject( hdcMem , hbm );
  SET_DC_RTL_MIRRORED(hdcMem);

  BitBlt( hdcMem , 0 , 0 , bm.bmWidth , bm.bmHeight ,
          hdc , 0 , 0 , SRCCOPY );

  SET_DC_LAYOUT(hdcMem,0);

  //
  // BUGBUG : The offset by 1 is to solve the off-by-one (in hdcMem) problem. Solved.
  // [samera]
  //
  BitBlt( hdc , 0 , 0 , bm.bmWidth , bm.bmHeight ,
          hdcMem , 0 , 0 , SRCCOPY );


  DeleteDC( hdcMem );
  DeleteObject( hbm );

  return;
}

// returns TRUE if handled
BOOL CCWndProc(CONTROLINFO* pci, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    if (uMsg >= CCM_FIRST && uMsg < CCM_LAST) {
        LRESULT lres = 0;
        switch (uMsg) {
        case CCM_SETUNICODEFORMAT:
            lres = pci->bUnicode;
            pci->bUnicode = BOOLFROMPTR(wParam);
            break;

        case CCM_GETUNICODEFORMAT:
            lres = pci->bUnicode;
            break;
            
        case CCM_SETVERSION:
            if (wParam <= COMCTL32_VERSION) {
                lres = pci->iVersion;
                pci->iVersion = (int)wParam;
            } else 
                lres = -1;
            break;
            
        case CCM_GETVERSION:
            lres = pci->iVersion;
            break;

        }
        
        ASSERT(plres);
        *plres = lres;
        
        return TRUE;
    }
    
    return FALSE;
}

#ifdef KEYBOARDCUES
// The return value tells if the state changed or not (TRUE == change)
BOOL NEAR PASCAL CCOnUIState(LPCONTROLINFO pControlInfo,
                                  UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    WORD wOldUIState = pControlInfo->wUIState;

    // That's the only message we handle
    if (WM_UPDATEUISTATE == uMessage)
    {
        switch (LOWORD(wParam))
        {
            case UIS_SET:
                pControlInfo->wUIState |= HIWORD(wParam);
                break;

            case UIS_CLEAR:
                pControlInfo->wUIState &= ~(HIWORD(wParam));
                break;
        }
    }

    // These message always need to be passed to DefWindowProc
    return (wOldUIState != pControlInfo->wUIState);
}

BOOL CCNotifyNavigationKeyUsage(LPCONTROLINFO pControlInfo, WORD wFlag)
{
    BOOL fRet = FALSE;

    // do something only if not already in keyboard mode
    if ((CCGetUIState(pControlInfo) & (UISF_HIDEFOCUS | UISF_HIDEACCEL)) != wFlag)
    {
        SendMessage(pControlInfo->hwndParent, WM_CHANGEUISTATE, 
            MAKELONG(UIS_CLEAR, wFlag), 0);

        pControlInfo->wUIState &= ~(HIWORD(wFlag));

        // we did the notify
        fRet = TRUE;
    }

    return fRet;
}

BOOL CCGetUIState(LPCONTROLINFO pControlInfo)
{
    return pControlInfo->wUIState;
}

#endif
