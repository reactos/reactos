#include "onlypbr.h"
#undef NOKERNEL
#undef NOGDI
#include <windows.h>
#include "port1632.h"

#include "pbrush.h"

/* win.ini entries */
TCHAR winIniAppName[] = TEXT("Paintbrush"), winIniHeightName[] = TEXT("height"),
     winIniWidthName[] = TEXT("width"), winIniClrName[] = TEXT("clear"),
     winIniOmitPictureFormat[] = TEXT("OmitPictureFormat");
#if defined (JAPAN) & defined (IME_HIDDEN) // added by Hiraisi
TCHAR winIniImeHidden[] = TEXT("IMEHidden");
#endif


/* strings filled from resource file */
TCHAR pgmName[APPNAMElen];
TCHAR pgmTitle[TITLElen];

/* names of window classes */
LPTSTR pbrushWndClass[MAXwnds] = { TEXT("pbParent"),TEXT("pbPaint"),TEXT("pbTool"),TEXT("pbSize"),
                                   TEXT("pbColor") };

/* paint control variables */
int CurrentWindow = PAINTid;
int theTool,theSize,theForeg,theBackg,theBrush;
int cursTool;           /* the tool used to figure out which cursor,
                                        usually same as theTool except zoom and such */
DWORD *rgbColor;
DWORD colorColor[MAXcolors+2], bwColor[MAXcolors+2];
DWORD defltColor[MAXcolors] =
{
   RGB(255,255,255),RGB(0,0,0),
        RGB(192,192,192),RGB(128,128,128),
        RGB(255,0,0),RGB(128,0,0),
   RGB(255,255,0),RGB(128,128,0),
   RGB(0,255,0),RGB(0,128,0),
        RGB(0,255,255),RGB(0,128,128),
   RGB(0,0,255),RGB(0,0,128),
   RGB(255,0,255),RGB(128,0,128),
        RGB(255,255,128),RGB(128,128,64),
        RGB(0,255,128),RGB(0,64,64),
   RGB(128,255,255),RGB(0,128,255),
   RGB(128,128,255),RGB(0,64,128),
   RGB(255,0,128),RGB(64,0,128),
   RGB(255,128,64),RGB(128,64,0)
};

DWORD   defltBW[MAXcolors] =
{
   RGB(255,255,255), RGB(0,0,0),
   RGB(250,250,250), RGB(9,9,9),
   RGB(242,242,242), RGB(18,18,18),
   RGB(226,226,226), RGB(33,33,33),
   RGB(208,208,208), RGB(50,50,50),
   RGB(194,194,194), RGB(64,64,64),
   RGB(176,176,176), RGB(82,82,82),
   RGB(159,159,159), RGB(97,97,97),
   RGB(130,130,130), RGB(72,72,72),
   RGB(174,174,174), RGB(81,81,81),
   RGB(165,165,165), RGB(90,90,90),
   RGB(156,156,156), RGB(99,99,99),
   RGB(147,147,147), RGB(108,108,108),
   RGB(138,138,138), RGB(117,117,117)
};

LOGFONT theFont;

/* pbrush windows and their dimensions */
HWND pbrushWnd[MAXwnds];
HWND mouseWnd,colorWnd,dlgWnd, hDlgModeless = 0;
HWND zoomOutWnd;
RECT pbrushRct[MAXwnds];

/* off-screen drawing buffers and related globals */
HDC imageDC = (HDC )NULL;
HDC fileDC = (HDC )NULL;
HBITMAP fileBitmap;
HANDLE imageBitmap[MAXimageBands];
int imageWid,imageHgt,imageByteWid,imagePlanes,imagePixels;
int nNewImageWidth, nNewImageHeight, nNewImagePlanes, nNewImagePixels;
int imageBands,bandHgt;
int fileByteWid,fileWid,filePlanes,defaultWid,defaultHgt;
int paintWid,paintHgt,zoomWid,zoomHgt,zoomAmount;
RECT imageView,zoomView;
POINT viewOrg,viewExt;
BOOL updateFlag,imageFlag;
LPBYTE fileBuff;
HANDLE hfileBuff = (HANDLE )NULL;
HBITMAP hToolbox = (HBITMAP )NULL;

/* pick off-screen buffers and related globals */
HDC pickDC,saveDC,monoDC,clipbDC;
HBITMAP pickBM,saveBM,monoBM,clipbBM;
RECT pickRect;
POINT pickOffs;
int pickWid,pickHgt,pickMode;
BOOL TerminateKill = TRUE;

/* draw procs */
DPPROC dpArray[MAXtools] =
{
       (DPPROC) PickDP,
       (DPPROC) PickDP,
       AirBruDP,
       Text2DP,
       ColEraDP,
       EraserDP,
       RollerDP,
       BrushDP,
       CurveDP,
       LineDP,
       RectDP,
       RectDP,
       RndRctDP,
       RndRctDP,
       OvalDP,
       OvalDP,
       PolyDP,
       PolyDP
};

LPTSTR cuArray[MAXtools + 3];    /* cursor for each tool */
LPTSTR DrawCursor;
TCHAR NotEnoughMem[128];

/* pointers to callback functions */
WNDPROC lpInfoDlg=NULL;
WNDPROC lpBrushDlg=NULL,lpFontMenu,lpPrintDlg,lpPageSetDlg,lpPrinterSetDlg;
WNDPROC lpFontInfo,lpFontInf2, lpTiltBlt=NULL;
WNDPROC lpColorDlg = ColorDlg;
WNDPROC lpMouseDlg = MouseDlg;
WNDPROC lpNullWP = NullWP;

/* screen dimension ratios */
int horzDotsMM,vertDotsMM;

/* misc globals */
BITMAPFILEHEADER_VER1 BitmapHeader;
HINSTANCE hInst;
DHDR imageHdr;
BOOL inMagnify,clearFlag,mouseFlag,outline,shadow, bZoomedOut;

WORD wFileType;
BOOL drawing,moving;
POINT polyPts[MAXpts],aspect;
POINT csrPt = { -1, -1 };
int numPts,fileMode;
TCHAR fileName[MAX_PATH],clipName[MAX_PATH],tempName[MAX_PATH];
TCHAR filePath[MAX_PATH],clipPath[PATHlen],colorPath[MAX_PATH];
TCHAR deviceStr[100];
BOOL bIsPrinterDefault = TRUE;
int fileMode = 0;
LPTSTR wildCard = { TEXT("01234567890123456789") }; /* just allocate some space */
LPTSTR pathPtr = { TEXT("\0") };
LPTSTR namePtr = { TEXT("\0") };
TCHAR noFile[MAX_PATH];
DPPROC DrawProc;

RECT imageRect;
BOOL FirstPrint = TRUE; /* should we reset print parms to default */

int nVertRes, nHorizRes;
int FontHeight;

struct  csstat CursorStat;

int DlgCaptionNo;

/* Handle to logical palette */
HPALETTE hPalette;

/* OLE:  Is OLE present, and are we running as a server? */
BOOL fServer = FALSE;           /* Are we running -Embedded? */
BOOL fOLE = FALSE;
BOOL fLoading = FALSE;          /* Are we loading a file? */

/* Some of the 3.0 winapps prefer metafile over bitmap format but do not
 * Paste metafiles correctly. To suppress Copying CF_METAFILEPICT to clipbrd,
 * fOmitPictureFormat can be set(or reset) through the menu Options.OmitPictureFormat.
 * The option will be saved in the win.ini under OmitPictureFormat = 0/1;
 */
BOOL fOmitPictureFormat = FALSE; /* Omit metafile format during Edit.Copy/Cut? */

#ifdef PENWIN
BOOL fIPExists = FALSE;    /* Is there an insertion point? */
#endif

TCHAR acDbgBfr[80];

