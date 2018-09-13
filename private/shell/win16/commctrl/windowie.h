#include <alias.h>
#define IDCLOSE         8       /* ;Internal 4.0 */
#define IDHELP          9       /* ;Internal 4.0 */
#define DS_3DLOOK           0x0004L     // ;Internal 4.0
#define DS_CONTROL          0x0400L     // ;Internal 4.0

/* 3D border styles */                                      // ;Internal 4.0
#define BDR_RAISEDOUTER 0x0001                              // ;Internal 4.0
#define BDR_SUNKENOUTER 0x0002                              // ;Internal 4.0
#define BDR_RAISEDINNER 0x0004                              // ;Internal 4.0
#define BDR_SUNKENINNER 0x0008                              // ;Internal 4.0
                                                            // ;Internal 4.0
#define BDR_OUTER       0x0003                              // ;Internal 4.0
#define BDR_INNER       0x000c                              // ;Internal 4.0
#define BDR_RAISED      0x0005                              // ;Internal 4.0
#define BDR_SUNKEN      0x000a                              // ;Internal 4.0
                                                            // ;Internal 4.0
#define BDR_VALID       0x000F                              // ;Internal 4.0
                                                            // ;Internal 4.0
#define EDGE_RAISED     (BDR_RAISEDOUTER | BDR_RAISEDINNER) // ;Internal 4.0
#define EDGE_SUNKEN     (BDR_SUNKENOUTER | BDR_SUNKENINNER) // ;Internal 4.0
#define EDGE_ETCHED     (BDR_SUNKENOUTER | BDR_RAISEDINNER) // ;Internal 4.0
#define EDGE_BUMP       (BDR_RAISEDOUTER | BDR_SUNKENINNER) // ;Internal 4.0
                                                            // ;Internal 4.0
/* Border flags */                                          // ;Internal 4.0
#define BF_LEFT         0x0001                              // ;Internal 4.0
#define BF_TOP          0x0002                              // ;Internal 4.0
#define BF_RIGHT        0x0004                              // ;Internal 4.0
#define BF_BOTTOM       0x0008                              // ;Internal 4.0
                                                            // ;Internal 4.0
#define BF_TOPLEFT      (BF_TOP | BF_LEFT)                  // ;Internal 4.0
#define BF_TOPRIGHT     (BF_TOP | BF_RIGHT)                 // ;Internal 4.0
#define BF_BOTTOMLEFT   (BF_BOTTOM | BF_LEFT)               // ;Internal 4.0
#define BF_BOTTOMRIGHT  (BF_BOTTOM | BF_RIGHT)              // ;Internal 4.0
#define BF_RECT         (BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM)  // ;Internal 4.0
                                                            // ;Internal 4.0
#define BF_DIAGONAL     0x0010                              // ;Internal 4.0
                                                            // ;Internal 4.0
#define BF_DIAGONAL_ENDTOPRIGHT     (BF_DIAGONAL | BF_TOP | BF_RIGHT)       // ;Internal 4.0
#define BF_DIAGONAL_ENDTOPLEFT      (BF_DIAGONAL | BF_TOP | BF_LEFT)        // ;Internal 4.0
#define BF_DIAGONAL_ENDBOTTOMLEFT   (BF_DIAGONAL | BF_BOTTOM | BF_LEFT)     // ;Internal 4.0
#define BF_DIAGONAL_ENDBOTTOMRIGHT  (BF_DIAGONAL | BF_BOTTOM | BF_RIGHT)    // ;Internal 4.0
                                                            // ;Internal 4.0
#define BF_MIDDLE       0x0800                              // ;Internal 4.0
#define BF_SOFT         0x1000                              // ;Internal 4.0
#define BF_ADJUST       0x2000                              // ;Internal 4.0
#define BF_FLAT         0x4000                              // ;Internal 4.0
#define BF_MONO         0x8000                              // ;Internal 4.0
                                                            // ;Internal 4.0
                                                            // ;Internal 4.0
BOOL WINAPI DrawEdge(HDC, LPRECT, UINT, UINT);              // ;Internal 4.0
/* flags for DrawFrameControl */                            // ;Internal 4.0
#define DFC_CAPTION             1                           // ;Internal 4.0
#define DFC_MENU                2                           // ;Internal 4.0
#define DFC_SCROLL              3                           // ;Internal 4.0
#define DFC_BUTTON              4                           // ;Internal 4.0
#define DFC_CACHE               0xFFFF                      // ;Internal 4.0
                                                            // ;Internal 4.0
#define DFCS_CAPTIONCLOSE       0x0000                      // ;Internal 4.0
#define DFCS_CAPTIONMIN         0x0001                      // ;Internal 4.0
#define DFCS_CAPTIONMAX         0x0002                      // ;Internal 4.0
#define DFCS_CAPTIONRESTORE     0x0003                      // ;Internal 4.0
#define DFCS_CAPTIONHELP        0x0004                      // ;Internal 4.0
#define DFCS_INMENU             0x0040                      // ;Internal 4.0
#define DFCS_INSMALL            0x0080                      // ;Internal 4.0
                                                            // ;Internal 4.0
#define DFCS_MENUARROW          0x0000                      // ;Internal 4.0
#define DFCS_MENUCHECK          0x0001                      // ;Internal 4.0
#define DFCS_MENUBULLET         0x0002                      // ;Internal 4.0
#define DFCS_MENUARROWRIGHT     0x0004                      // ;Internal 4.0
#if (WINVER >= 0x40A)                                       // ;Internal 4.1
#define DFCS_MENUARROWUP        0x0008                      // ;Internal 4.1
#define DFCS_MENUARROWDOWN      0x0010                      // ;Internal 4.1
#endif // (WINVER >= 0x40A)                                 // ;Internal 4.1
                                                            // ;Internal 4.0
#define DFCS_SCROLLMIN          0x0000                      // ;Internal 4.0
#define DFCS_SCROLLVERT         0x0000                      // ;Internal 4.0
#define DFCS_SCROLLMAX          0x0001                      // ;Internal 4.0
#define DFCS_SCROLLHORZ         0x0002                      // ;Internal 4.0
#define DFCS_SCROLLLINE         0x0004                      // ;Internal 4.0
                                                            // ;Internal 4.0
#define DFCS_SCROLLUP           0x0000                      // ;Internal 4.0
#define DFCS_SCROLLDOWN         0x0001                      // ;Internal 4.0
#define DFCS_SCROLLLEFT         0x0002                      // ;Internal 4.0
#define DFCS_SCROLLRIGHT        0x0003                      // ;Internal 4.0
#define DFCS_SCROLLCOMBOBOX     0x0005                      // ;Internal 4.0
#define DFCS_SCROLLSIZEGRIP     0x0008                      // ;Internal 4.0
// This file is contains 4.0 definitions from windows.h.  For win31 target
// platforms, we define a WINVER==0x030a and use this file to control the extensions
// to the 3.1 base. 

#define DFCS_SCROLLSIZEGRIPRIGHT 0x0010                         // ;Internal 4.0
                                                            // ;Internal 4.0
#define DFCS_BUTTONCHECK        0x0000                      // ;Internal 4.0
#define DFCS_BUTTONRADIOIMAGE   0x0001                      // ;Internal 4.0
#define DFCS_BUTTONRADIOMASK    0x0002                      // ;Internal 4.0
#define DFCS_BUTTONRADIO        0x0004                      // ;Internal 4.0
#define DFCS_BUTTON3STATE       0x0008                      // ;Internal 4.0
#define DFCS_BUTTONPUSH         0x0010                      // ;Internal 4.0
                                                            // ;Internal 4.0
#define DFCS_CACHEICON          0x0000                      // ;Internal 4.0
#define DFCS_CACHEBUTTONS       0x0001                      // ;Internal 4.0
                                                            // ;Internal 4.0
#define DFCS_INACTIVE           0x0100                      // ;Internal 4.0
#define DFCS_PUSHED             0x0200                      // ;Internal 4.0
#define DFCS_CHECKED            0x0400                      // ;Internal 4.0
#if (WINVER >= 0x40A)                                       // ;Internal 4.1
#define DFCS_TRANSPARENT        0x0800                      // ;Internal 4.1
#define DFCS_HOT                0x1000                      // ;Internal 4.1
#endif // (WINVER >= 0x40A)                                 // ;Internal 4.1
#define DFCS_ADJUSTRECT         0x2000                      // ;Internal 4.0
#define DFCS_FLAT               0x4000                      // ;Internal 4.0
#define DFCS_MONO               0x8000                      // ;Internal 4.0
//HANDLE WINAPI SetObjectOwner(HGDIOBJ, HANDLE);
#define SetObjectOwner(a, b) (void*)&(a)

#define SIF_RANGE           0x0001                          // ;Internal 4.0
#define SIF_PAGE            0x0002                          // ;Internal 4.0
#define SIF_POS             0x0004                          // ;Internal 4.0
#define SIF_DISABLENOSCROLL 0x0008                          // ;Internal 4.0
#define SIF_TRACKPOS        0x0010                          // ;Internal 4.0
#define SIF_ALL             (SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS)// ;Internal
#define SIF_RETURNOLDPOS    0x1000                          // ;Internal
#define SIF_NOSCROLL        0x2000                          // ;Internal
#define SIF_MASK            0x701F                          // ;Internal
                                                            // ;Internal 4.0
typedef struct tagSCROLLINFO                                // ;Internal 4.0
{                                                           // ;Internal 4.0
    DWORD   cbSize;                                         // ;Internal 4.0
    DWORD   fMask;                                          // ;Internal 4.0
    LONG    nMin;                                           // ;Internal 4.0
    LONG    nMax;                                           // ;Internal 4.0
    DWORD   nPage;                                          // ;Internal 4.0
    LONG    nPos;                                           // ;Internal 4.0
    LONG    nTrackPos;                                      // ;Internal 4.0
}                                                           // ;Internal 4.0
SCROLLINFO, FAR *LPSCROLLINFO;                              // ;Internal 4.0
typedef const SCROLLINFO FAR *LPCSCROLLINFO;                // ;Internal 4.0

//#define HELPINFO_WINDOW    0x0001
//#define HELPINFO_MENUITEM  0x0002
#define HELP_CONTEXTMENU    0x000a
#define HELP_WM_HELP        0x000c
typedef  struct  tagHELPINFO
{
    DWORD   cbSize;
    int     iContextType;
    int     iCtrlId;
    HANDLE  hItemHandle;
    DWORD   dwContextId;
    POINT   MousePos;
}
HELPINFO, FAR* LPHELPINFO;


#define WS_EX_TOOLWINDOW        0x00000080L     // ;Internal 4.0
#define WS_EX_CLIENTEDGE        0x00000200L     // ;Internal 4.0
#define WS_EX_STATICEDGE        0x00020000L     // ;Internal 4.0
#define WS_EX_WINDOWEDGE        0x00000100L     // ;Internal 4.0

