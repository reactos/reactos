/*  CLIPBRD.H                                                               */
/*                                                                          */
/*  Copyright 1985-92, Microsoft Corporation                                */

#include <windows.h>
#include <port1632.h>
#include <commdlg.h>

#ifndef MAXSHORT
#define  MAXSHORT 0x7fff
#endif

#define SIZE_OF_WIN31_BITMAP_STRUCT 14       //Win 3.1 BITMAP structure is 14 bytes long
#define SIZE_OF_WIN31_METAFILEPICT_STRUCT 8  //Win 3.1 METAFILEPICT structure is 8 bytes long


#define PRIVATE_FORMAT(fmt)   ((fmt) >= 0xC000)

/* Header text string ids */
#define IDS_NAME            100  /* CF_TEXT to CF_OEMTEXT (1 to 7) are also used */
#define IDS_ERROR           102  /* as string ids.  Be sure to keep these    */
#define IDS_BINARY          103  /* different.                               */
#define IDS_CLEAR           104
#define IDS_FMTNOTSAV       105
#define IDS_DEFAULT         106
#define IDS_CANTDISPLAY     107  /* "Can't display data in this format" */
#define IDS_NOTRENDERED     108  /* "Application Couldn't render data"  */
#define IDS_HELPFILE        109  /* Clipbrd.hlp */
#define IDS_CLEARTITLE      110
#define IDS_CONFIRMCLEAR    111
#define IDS_ALREADYOPEN     112  /* OpenClipboard() fails */
#define IDS_INVALIDFILENAME 113  /* Filename is invalid */
#define IDS_OPENCAPTION     114  /* ID of File/Open dlg. caption string */
#define IDS_SAVECAPTION     115  /* ID of File/Save dlg. caption string */
#define IDS_MEMERROR        116
#define IDS_DEFEXTENSION    117  /* Default extension for clipboard files */
#ifdef JAPAN
#define IDS_ENOTVALIDFILE   130  /* ID of invalid file format messagebox*/
#endif

#define IDS_READERR        200  /* ID of base ReadClipboardFile error */
#define IDS_READFORMATERR   201
#define IDS_READOPENCLIPERR 202
#define IDS_FILTERTEXT      301  /* ID of filter string for File/Open   */

/* Dialogbox resource id */
#define ABOUTBOX        1
#define CONFIRMBOX   2

#define CDEFFMTS        8       /* Count of predifined clipboard formats    */
#define VPOSLAST        100     /* Highest vert scroll bar value */
#define HPOSLAST        100     /* Highest horiz scroll bar value */
#define CCHFMTNAMEMAX   79      /* Longest clipboard data fmt name, including
                                   terminator */
#define cLineAlwaysShow 3       /* # of "standard text height" lines to show
                                   when maximally scrolled down */
#define BUFFERLEN       200     /* String buffer length */
#define SMALLBUFFERLEN  90
#define IDSABOUT        1

#define CBMENU          1       /* Number for the Clipboard main menu  */
#define CBICON          2
#define CBACCEL         3

#define FILTERMAX   100         /* max len. of File/Open filter string */
#define CAPTIONMAX  30          /* len of caption text for above dlg.  */
#define PATHMAX     128         /* max. len of DOS pathname          */
#define MSGMAX      255

/* The menu ids */
#define CBM_AUTO        WM_USER
#define CBM_CLEAR       WM_USER+1
#define CBM_OPEN        WM_USER+2
#define CBM_SAVEAS      WM_USER+3
#define CBM_ABOUT       WM_USER+4
#define CBM_EXIT        WM_USER+5
#define CBM_HELP   0xFFFF    /* Standard numbers */
#define CBM_USEHELP     0xFFFC   /* Standard numbers */
#define CBM_SEARCH   0x0021

/*  Last parameter to SetDIBits() and GetDIBits() calls */

#define  DIB_RGB_COLORS   0
#define  DIB_PAL_COLORS     1

#define  IDCLEAR    IDOK

/* Structures for saving/loading clipboard data from disk */

#define     CLP_ID     0xC350
#define     CLP_NT_ID  0xC351

// Windows 3.1 used byte packing on structs. These structs are used in
// files common between NT and Win 3.1, therefore need byte packing.
// a-mgates 9/28/92
#ifndef RC_INVOKED
#pragma pack(1)
typedef struct {
    WORD magic;
    WORD FormatCount;
} FILEHEADER;

typedef struct {
   UINT  FormatID;
   DWORD DataLen;
   DWORD DataOffset;
   TCHAR Name[CCHFMTNAMEMAX];
} FORMATHEADER;
#pragma pack()
#endif


void NEAR PASCAL SaveClipboardToFile(HWND);
void NEAR PASCAL OpenClipboardFile(HWND);

BOOL MyOpenClipboard(HWND);
BOOL NEAR PASCAL ClearClipboard(HWND);
void NEAR PASCAL GetClipboardName(UINT fmt, LPSTR szName, INT iSize);

BOOL RenderFormat(FORMATHEADER *f,INT);
DWORD APIENTRY lread(INT fh, void FAR *pv, DWORD ul);
DWORD APIENTRY lwrite(INT fh, void FAR *pv, DWORD ul);

void UpdateCBMenu(HWND);
void ChangeCharDimensions(HWND,UINT,UINT);
void SetCharDimensions(HWND,HFONT);
void SaveOwnerScrollInfo(HWND);
void RestoreOwnerScrollInfo(HWND);

void SendOwnerMessage(UINT, WPARAM, LPARAM);
void SendOwnerSizeMessage(HWND, INT, INT, INT, INT);

void DrawStuff(HWND,PAINTSTRUCT *f);

void ClipbrdVScroll(HWND, WORD, WORD);
void ClipbrdHScroll(HWND, WORD, WORD);

UINT GetBestFormat(UINT);

LONG APIENTRY ClipbrdWndProc(HWND, UINT, WPARAM, LONG);
BOOL APIENTRY ConfirmDlgProc(HWND, UINT, WPARAM, LONG);

/* Far low mem situations. */
void FAR PASCAL MemErrorMessage(void);


/*****************************  global data  *******************************/

extern HINSTANCE   hInst;
extern TCHAR    szFileName[];
extern HWND   hwndMain;
extern TCHAR   szAppName[];
extern TCHAR   szFileSpecifier[];

/* variables for the new File Open,File SaveAs and Find Text dialogs */
#define CCH_szDefExt 8

extern OPENFILENAME OFN;
extern TCHAR szFileName [];
extern BOOL  fNTReadFileFormat;
extern TCHAR szLastDir  [];
extern TCHAR szFilterSpec[];         /* default filter spec. for above    */
extern TCHAR szCustFilterSpec[];     /* buffer for custom filters created */
extern UINT wHlpMsg;                 /* message used to invoke Help    */
extern TCHAR szOpenCaption [];       /* File open dialog caption text    */
extern TCHAR szSaveCaption [];       /* File Save as dialog caption text  */
extern TCHAR szDefExt      [CCH_szDefExt];       /* default file extension to use  */

#define MAXBITSPERPIXEL     24

extern BOOL    fAnythingToRender;
extern BOOL    fOwnerDisplay;
extern BOOL    fDisplayFormatChanged;

extern TCHAR    szAppName[];
extern TCHAR    szCaptionName[CAPTIONMAX];
extern TCHAR    szHelpFileName[20];

extern TCHAR    szMemErr[MSGMAX];

extern HWND    hwndNextViewer;
extern HWND    hwndMain;

extern HANDLE  hAccel;
extern HANDLE  hfontSys;
extern HANDLE  hfontOem;
extern HANDLE  hfontUni;

extern HBRUSH  hbrWhite;
extern HBRUSH  hbrBackground;

extern HMENU   hMainMenu;
extern HMENU   hDispMenu;

extern INT     OwnVerMin, OwnVerMax, OwnHorMin, OwnHorMax;
extern INT     OwnVerPos, OwnHorPos;

extern LONG    cyScrollLast;
extern LONG    cyScrollNow;
extern INT     cxScrollLast;
extern INT     cxScrollNow;

extern RECT    rcWindow;
extern UINT    cyLine, cxChar, cxMaxCharWidth;
extern UINT    cxMargin, cyMargin;

extern UINT    CurSelFormat;
extern UINT    rgfmt[];
