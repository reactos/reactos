/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   Pbrush.h                                    *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  header for '.C' files                       *
*   date:   04/07/87 @ 15:30                            *
*                                                       *
********************************************************/
/* recompile filedlg.c, and saveimg.c */
/* #define DEMOVER */

/*this is for various pieces to distinguish the environment fromwShell*/
#define WINDOWS 1

/* this is the windows version number and is to be used for windows only */
#ifndef WIN32
#define WIN31                   /* Updated */

#define PUBLIC FAR PASCAL
#define PRIVATE NEAR PASCAL
#define REGISTER register
#else
#define REGISTER
#define PRIVATE
#define PUBLIC
#endif

typedef void (FAR *DPPROC)();

#define abs(x) (((x) > 0) ? (x) : -(x))

/*
 * IsPathSep(ch)
 *
 * Returns TRUE iff (ch) is a separator char in a path.  Where seperator char
 * means a character between different componets of a path.  In this case,
 * the '.' char is is considered to be part of the component filename, and NOT
 * a separator.
 */
#define IsPathSep(ch)   \
        ((ch) == TEXT('\\') || (ch) == TEXT('/') || (ch) == TEXT(':'))

/*
 * Name of the localized dialog font
 */
#define LOCAL_FONT_NAME     TEXT("MS Shell Dlg")

/* String table constants and lengths */
#define IDSname                 100
#define IDSuntitled             200
#define IDStitle                300
#define IDSpicture              400
#define IDSEdit                 401
#define IDSxiny                 500
#define IDSNotEnufMem           1000
#define IDSCantOpen             1001
#define IDSUnknownFmt           1500
#define IDSUnableHdr            1002
#define IDSBadHeader            1003
#define IDSBadData              1004
#define IDSCantAlloc            1005
#define IDSNotExist             1006
// #define IDSWriteOver          1007
#define IDSNotMemAvail          1008
#define IDSSaveTo               1009
#define IDSInvalidEntry         1010
#define IDSInvalidMargins       1011
#define IDSInvalidWidth         1012
#define IDSInvalidHeight        1013
#define IDSReadOnly             1014
#define IDSInvalidCopy          1015
#define IDSInvalidNumb          1016
#define IDSNotPrint             1017
#define IDSPrintFit             1018
#define IDSCantCreate           1019
#define IDSUnableSave           1021
#define IDSHdrSave              IDSUnableSave
#define IDSNoSaveAvail          1022
#define IDSPaste                1023
#define IDSNoDC                 1024
#define IDSNoBrush              1025
#define IDSNoMemAvail           1026
#define IDSNotDiskAvail         2026
#define IDSXlat                 1027
#define IDSInvalidRight         1028
#define IDSInvalidBottom        1029
#define IDSOnPort               1030
#define IDSNoVirtInit           1031
#define IDSNoCut                1032
#define IDSNoClipboardFormat    1033
#define IDSNoClipboard          1034
#define IDSNoPasteFrom          1035
#define IDSUnablePalette        1036
#define IDSUnableData           1037
#define IDSImageLarge           1038
#define IDSNoCopy               1039
#define IDSNotBitmap            1040
#define IDS3PlaneFile           1041
#define IDSSaveAs               1042
#define IDSFileOpen             1043
#define IDSPasteFrom            1044
#define IDSCopyTo               1045
#define IDSGetColors            1046
#define IDSSaveColors           1047
#define IDSPrintSpool           1048
#define IDSPrintFile            1049
#define IDSPrintDevice          1050
#define IDSPrintPercent         1051
#define IDSPrintDCErr           1052
#define IDSPrintInitErr         1053
#define IDSInvalidScale         1054
#define IDSPrinterChange        1055
#define IDSNameConflict         1056
#define IDSNoMSPPaste           1057
#define IDSInvFilename          1058
#define IDSLetters              1059

/* OLE modification -- mikel */
#define IDS_UPDATE              1060
#define IDS_SAVE                1061
#define E_FAILED_TO_REGISTER_DOCUMENT     1062
#define E_FAILED_TO_REGISTER_SERVER       1063
#define W_FAILED_TO_REVOKE_DOCUMENT       1064
#define W_FAILED_TO_REVOKE_SERVER         1065
#define E_SET_DIMENSIONS_UNSUPPORTED      1066
#define W_SET_DIMENSIONS_UNSUPPORTED      1067
#define W_SET_ITEM_NAME_UNSUPPORTED       1068
#define W_SET_TARGET_DEVICE_UNSUPPORTED   1069

/* Common dialog support */
#define IDS_ALLFILTER           1070
#define IDS_BMPFILTER           1071
#define IDS_MSPFILTER           1072
#define IDS_PCXFILTER           1073

#define IDS_MONOBMPFILTER       1074
#define IDS_16COLORFILTER       1075
#define IDS_256COLORFILTER      1076
#define IDS_24BITFILTER         1077

#define IDS_COLORFILTER                   1080
#define W_SAVING_LOWER_RESOLUTION1        1081
#define W_SAVING_LOWER_RESOLUTION2        1082
#define W_SAVING_LOWER_RESOLUTION3        1083
#define E_INVALID_ITEM_NAME               1084
#define IDSNoPrinters                     1085
#define IDSPrintDlgErr                    1086
#define IDS_MAYBEUPDATE                   1087
#define IDSTextPasteMsg                   1088
#define IDSTextPasteMsgZoomed             1089
#define IDSCANTPRINTGRAPHICS              1090
#define IDS_EXIT                          1091
#define IDS_EXITANDRETURN                 1092
#define IDS_RESETIMAGE                    1093
#define IDS_OBJECTUPDATE                  1094

#ifdef JAPAN // for Page layout settings change by intl
#define IDS_SPACEISINCH                         1100
#define IDS_SPACEISCENTI                        1101
   //  added by Hiraisi (BUG#2219/WIN31)
#define IDSCantCreateObj                        1102
#endif

#define APPNAMElen  7
#define UNTITLEDlen 18
#define WNDTITLElen 60
#ifdef JAPAN
#define TITLElen    20
#else
#define TITLElen    14
#endif
#define FILENAMElen MAX_PATH
#define PATHlen     MAX_PATH


/* dialog id's */
/* resource id's */
#include "pbdialog.h"
#include <commdlg.h>

/* window id's */
#define PARENTid 0
#define PAINTid  1
#define TOOLid   2
#define SIZEid   3
#define COLORid  4
#define MAXwnds  5

#define SHOWWINDOW 1
#define HIDEWINDOW 0
#define NOCHANGEWINDOW 2

/* file menu items */
#define FILEnew        101
#define FILEopen       102
#define FILEsave       103
#define FILEsaveas     104
#define FILEprint      105
#define FILEshow       106
#define FILEclear      107
#define FILEloadForPrint 108
#define FILEload       109
#define FILEexit       110
#define FILEpage       111
#define FILEprinter    112

/* OLE modification -- mikel */
#define FILEupdate     113

/* Cairo modification -- JonPa */
#define FILEinit       114

/* edit menu items */
#define EDITundo       201
#define EDITpaste      204
#define EDITcopyTo     205
#define EDITpasteFrom  206
#define EDITcutpict    207
#define EDITcopypict   208

/* font menu items */
#define FONTS          300

/* style menu items */
#define STYLEplain     401
#define STYLEbold      402
#define STYLEitalic    403
#define STYLEunderline 404
#define STYLEoutline   405
#define STYLEshadow    406
#if defined(JAPAN)||defined(TAIWAN)||defined(KOREA)  // added by Hiraisi  07 Apr. 1992 : jinwoo 11/9/92
#define STYLEvertical  407           // vertical writing
#endif

/* pick menu items */
#define PICKflipH      601
#define PICKflipV      602
#define PICKinverse    603
#define PICKsg         604
#define PICKtilt       605
#define PICKclear      606

/* misc menu items, now options*/
#define MISCzoomIn     701
#define MISCzoomOut    702
#define MISCbrush      703
#define MISCeditColor  704
#define MISCgetColor   705
#define MISCsaveColor  706
#define MISCmousePos   707
#define MISCZoom2In    708
#define MISCOmitPictureFormat   709

/* zoom in menus */
#define ZOOMundo    201
#define MENUundo    201
#define ZOOMaccept  900

/* tool removal menu */
#define WINDOWtool      801
#define WINDOWpalette   802
#define WINDOWall       803
#define WINDOWnone      804

/* help menu item*/
#define MENU_INDEX      1001
#define MENUabout       1002
#define MENU_USINGHELP  1003
#define MENU_SEARCH     1004

/* debug menu items */
#ifdef SPECIAL
#define  DEBUG_GRIDLINES      2001
#define  DEBUG_EYEDROPPER     2002
#define  DEBUG_IMAGEPALETTE   2003
#endif
#ifdef DEBUG
#define  DEBUG_GLOBALCOMPACT  2006
#endif

/* rgb color values */
#define BLACKrgb   0x00000000L
#define REDrgb     0x000000FFL
#define GREENrgb   0x0000FF00L
#define YELLOWrgb  0x0000FFFFL
#define BLUErgb    0x00FF0000L
#define MAGENTArgb 0x00FF00FFL
#define CYANrgb    0x00FFFF00L
#define WHITErgb   0x00FFFFFFL

/* tools */
#define SCISSORStool     0
#define PICKtool         1
#define AIRBRUSHtool     2
#define TEXTtool         3
#define COLORERASERtool  4
#define ERASERtool       5
#define ROLLERtool       6
#define BRUSHtool        7
#define CURVEtool        8
#define LINEtool         9
#define RECTFRAMEtool    10
#define RECTFILLtool     11
#define RNDRECTFRAMEtool 12
#define RNDRECTFILLtool  13
#define OVALFRAMEtool    14
#define OVALFILLtool     15
#define POLYFRAMEtool    16
#define POLYFILLtool     17
#define LCUNDOtool       18
#define ZOOMINtool       19
#define HANDtool        20
#ifdef  JAPAN
#define MAXfonts        32
#else
#define MAXfonts        15
#endif
#define MAXtools        18
#define MAXcolors       28
#define MAXpts          1000
#define MAXsize         39
#define MAXimageBands   200
#define MAXbandSize     0x4000
#define MAXmenus    6
#define MAXCLIENTNAME   100

/* Pick popup menu position */
#define MENUPOS_PICK   4

/* dimensions of child windows evenly divisible by ... */
#define PAINTdiv   8
#define TOOLdiv    9
#define SIZEdiv    11
#define COLORdiv   16
#define NUM_SIZES  8
#define SIZE_EXTX  58
#define SIZE_EXTY  86

/* draw tool operations */
#define XORop   0
#define FILLop  1
#define FRAMEop 2

/* brush values */
#define RECTbrush   0
#define OVALbrush   1
#define HORZbrush   2
#define VERTbrush   3
#define SLANTLbrush 4
#define SLANTRbrush 5
#define MAXbrush    6

/* FILE TYPE FLAGS */
#define PCXFILE         0
#define BITMAPFILE      1
#define BITMAPFILE4     2
#define BITMAPFILE8     3
#define BITMAPFILE24    4
#define MSPFILE         5
#define EMPTYFILE       6
#define UNKNOWN_TYPE    0xffff

/* raster op combination modes */
#define ROP_DSna  0x00220326L
#define ROP_DSPao 0x00EA02E9L
#define ROP_SPa   0x00C000CAL
#define ROP_S     0x00CC0020L
#define ROP_Sn    0x00330008L
#define ROP_DPx   0x005A0049L
#define ROP_DPxn  0x00A50065L
#define ROP_DPnx  0x00A50065L
#define ROP_DSo   0x00EE0086L
#define ROP_DSa   0x008800C6L
#define ROP_0     0x00000042L
#define ROP_DPa   0x00A000C9L
#define ROP_DSx   0x00660046L
#define ROP_PSa   0x00C000CAL
#define ROP_DPna  0x000A0329L
#define ROP_Dn    0x00550009L
#define ROP_DSnx  0x00990066L
#define ROP_DSPDxax  0x00E20746L
#define ROP_SPxn  0x00C3006AL

#define MASKROP(fore,back) (DWORD)(((back << 8) & 0xff000000) | fore)
/* constrain directions */
#define HORIZdir 1
#define VERTdir  2

/* application messages */
#define WM_HIDECURSOR WM_USER
#define WM_TERMINATE  (WM_USER + 1)
#define WM_CHANGEFONT (WM_USER + 2)
#define WM_ZOOMUNDO   (WM_USER + 3)
#define WM_ZOOMACCEPT (WM_USER + 4)
#define WM_SCROLLINIT (WM_USER + 5)
#define WM_SCROLLDONE (WM_USER + 6)
#define WM_SCROLLVIEW (WM_USER + 7)
#define WM_PICKFLIPH  (WM_USER + 8)
#define WM_PICKFLIPV  (WM_USER + 9)
#define WM_PICKINVERT (WM_USER + 10)
#define WM_PICKSG     (WM_USER + 11)
#define WM_PICKTILT   (WM_USER + 12)
#define WM_PICKCLEAR  (WM_USER + 13)
#define WM_MOUSEPOS   (WM_USER + 14)
#define WM_COPYTO     (WM_USER + 15)
#define WM_PASTEFROM  (WM_USER + 16)
#define WM_WHOLE      (WM_USER + 17)
#define WM_SHOWCURSOR (WM_USER + 18)
#define WM_MOUSESYS   (WM_USER + 19)
#define WM_ERRORMSG   (WM_USER + 20)
#define WM_SELECTTOOL (WM_USER + 21)
#define WM_OUTLINE    (WM_USER + 22)
#ifdef PENWIN
#define WM_CORRECTTEXT (WM_USER + 23)
#endif
#ifdef JAPAN //KKBUGFIX    // added by Hiraisi  04 Sep. 1992 (in Japan)
#define WM_RESETCARET  (WM_USER + 24)
#define WM_MOUSEWINDOW (WM_USER + 25)  // 11 Nov. 1992 (BUG#457/WIN31 in Japan)
#endif
#ifdef JAPAN                  // added by Hiraisi
#define WM_IME_CHAR   (WM_USER + 26)
#endif

#define PBM_INITFONTPRINT   (WM_USER + 27)

/* cursor types */
#define RECTcsr     0
#define OVALcsr     1
#define HORZcsr     2
#define VERTcsr     3
#define SLANTLcsr   4
#define SLANTRcsr   5
#define CROSScsr    6
#define BOXcsr      7
#define BOXCROSScsr 8
#define ROLLERcsr   9
#define IBEAMcsr    10
#define ZOOMINcsr   11
#define BOXXcsr     12

/* run length encoding equates */
#define MINcount 2
#define MAXcount 63
#define ESCbits  0xC0

/* file i/o equates */
#define PATHSIZE            MAX_PATH
//#define FILETYPE 0x0000
#define DIRTYPE 0xC010

/* coordinate limits */
#define MAXcoord 16383
#define MINcoord -16384

/* polygon flip direction */
#define FLIPh 0
#define FLIPv 1

/* height of file buffer */
#define FILEBUFFrows 1

/* WM_SIZE command type */
#define SIZEPAINTWND 5

/* character equates */
#define CR           13
#define LF           10
#define BS           8

/* file modes (used by FileDlg()) */
#define SAVEfile 0
#define LOADfile 1

/* misc. equates */
#define TEXTBUFFsize 2500
#define BETAendDate  559958400L
#define ROUNDdiv     3
#define ROUNDmax     30
#define MAXPROFILELEN   100
#define FILTERMAX   40
#define CAPTIONMAX  50
#define WARNMSGMAX  400


/* constants for dialog box captions */
#define  FILESAVE 0
#define  FILELOAD 1
#define  PASTEFROM 2
#define  COPYTO   3
#define  COLORLOAD   4
#define  COLORSAVE   5

/* registration key length */
#define KEYNAMESIZE     300
#define OBJSTRINGSMAX 64

/* maximum size a RISC machine can put through OLE (in bytes) */
#define MAX_286_OLE_BM_SIZE     (1024 * 1022)

/* structures */
typedef struct dhdr {
  BYTE manuf;
  BYTE hard;
  BYTE encod;
  BYTE bitpx;
  SHORT x1;
  SHORT y1;
  SHORT x2;
  SHORT y2;
  SHORT hRes;
  SHORT vRes;
  BYTE clrma[48];
  BYTE vMode;
  BYTE nPlanes;
  SHORT bplin;
  BYTE xtra[60];
} DHDR;

typedef struct paral {
  POINT topLeft;
  POINT topRight;
  POINT botLeft;
  POINT botRight;
} PARAL;

struct csstat   {
        unsigned allowed : 1;
        unsigned inrsrc : 1;
        unsigned noted : 1;
        unsigned captured : 1;
        };

typedef struct Bit1hdr
        {
        BYTE version;
        BYTE discardable;
        WORD type;
        WORD wid;
        WORD hgt;
        WORD widthBytes;
        BYTE planes;
        BYTE bitcount;
        WORD res1;
        WORD res2;
        } BITMAPFILEHEADER_VER1;

#ifndef NOEXTERN
/* win.ini entries*/
extern TCHAR winIniAppName[], winIniHelpName[],
                winIniHeightName[], winIniWidthName[], winIniClrName[];
extern TCHAR winIniKeyName[];

/* misc globals */
extern HPALETTE hPalette;
extern BITMAPFILEHEADER_VER1 BitmapHeader;
extern int helpnumber;
extern BOOL ddeInProgress;
extern HCURSOR hWaitCursor;
extern TCHAR pgmName[APPNAMElen],aboutStr[],wndTitle[];
extern TCHAR pgmTitle[TITLElen];
extern LPTSTR  pbrushWndClass[];
extern TCHAR fileName[],clipName[],tempName[];
extern TCHAR filePath[],clipPath[],colorPath[], szHelpPath[];
extern LPTSTR wildCard,namePtr,pathPtr;
extern TCHAR noFile[MAX_PATH];
extern TCHAR deviceStr[];
extern LPTSTR deviceName, driverName, portName;
extern BOOL bIsPrinterDefault;
extern int CurrentWindow;
extern int theTool,theSize,theForeg,theBackg,theBrush;
extern int cursTool;
extern DWORD *rgbColor;
extern DWORD colorColor[], bwColor[];
extern DWORD defltColor[], defltBW[];
extern LOGFONT theFont;
extern HINSTANCE hInst;
extern HACCEL hAccelTable;
extern HWND pbrushWnd[MAXwnds],mouseWnd,nextViewer,colorWnd,dlgWnd, hDlgModeless;
extern HWND zoomOutWnd;
extern RECT pbrushRct[MAXwnds],imageRect;
extern DHDR imageHdr;
extern BOOL inMagnify,clearFlag,mouseFlag,outline,shadow, bZoomedOut;
extern BOOL drawing,moving;
extern POINT polyPts[],aspect,csrPt;
extern int numPts,numFaces,fileMode, TOOLHELP[];
extern int sizeArray[MAXsize];
extern HBITMAP hToolbox;
extern WORD wFileType;

/* off-screen drawing buffers and related globals */
extern HDC imageDC,fileDC;
extern HBITMAP fileBitmap;
extern HANDLE imageBitmap[];
extern int imageWid,imageHgt,imageByteWid,imagePlanes,imagePixels;
extern int nNewImageWidth, nNewImageHeight, nNewImagePlanes, nNewImagePixels;
extern int imageBands,bandHgt;
extern int fileByteWid,fileWid,filePlanes,defaultWid,defaultHgt;
extern int paintWid,paintHgt,zoomWid,zoomHgt,zoomAmount;
extern RECT imageView,zoomView;
extern POINT viewOrg,viewExt;
extern BOOL updateFlag,imageFlag;
extern LPBYTE fileBuff;
extern HANDLE hfileBuff;        /* tempBuff removed FGS */

/* pick off-screen buffers and related globals */
extern HDC pickDC,saveDC,monoDC,clipbDC;
extern HBITMAP pickBM,saveBM,monoBM,clipbBM;
extern RECT pickRect;
extern POINT pickOffs;
extern int pickWid,pickHgt,pickMode;
extern BOOL TerminateKill;

/* drawing procs */
extern DPPROC DrawProc;
extern DPPROC dpArray[];
extern LPTSTR cuArray[];
extern LPTSTR DrawCursor;
extern TCHAR NotEnoughMem[128];

/* Common printer setup dialog vars */
extern PRINTDLG PD;

extern BOOL bFileLoaded;

/* pointers to callback functions */
extern WNDPROC lpInfoDlg,lpMouseDlg,lpBrushDlg;
extern WNDPROC lpColorDlg,lpFontMenu,lpPrintDlg,lpPageSetDlg,lpPrinterSetDlg;
extern WNDPROC lpFontInfo,lpFontInf2, lpHelpDlg, lpTiltBlt, lpNullWP;

/* screen dimension ratios */
extern int horzDotsMM,vertDotsMM;

extern BOOL FirstPrint;         /* should we reset print parms to default */

extern int nVertRes;            /* printer dpi, for pointsize calculation */
extern int nHorizRes;           /* printer dpi, for pointsize calculation */

extern int FontHeight;

extern struct   csstat CursorStat;

/* Dialog Caption variables */
extern int DlgCaptionNo;

extern int iMeasure;
extern TCHAR winIniOmitPictureFormat[];
extern int fOmitPictureFormat;
extern TCHAR szClientName[];
#endif

/* old DIB headers */
typedef struct tagO_BITMAPCOREHEADER {
        DWORD   bcSize;                 /* used to get to color table */
        WORD    bcWidth;
        WORD    bcHeight;
        WORD    bcPlanes;
        WORD    bcBitCount;
} O_BITMAPCOREHEADER;
typedef O_BITMAPCOREHEADER FAR *LPO_BITMAPCOREHEADER;
typedef O_BITMAPCOREHEADER *PO_BITMAPCOREHEADER;

typedef struct tagO_BITMAPINFO {
    O_BITMAPCOREHEADER  bmciHeader;
    RGBTRIPLE           bmciColors[1];
} O_BITMAPINFO;
typedef O_BITMAPINFO FAR *LPO_BITMAPINFO;
typedef O_BITMAPINFO *PO_BITMAPINFO;

typedef HANDLE HDIB;

#ifdef WIN16
typedef struct {    /*structure for holding time*/
    int hour;   /* 0 - 11 hours for analog clock */
    int hour12; /* 12 hour format */
    int hour24; /* 24 hour format */
    int minute;
    int second;
    int ampm;   /* 0 - AM , 1 - PM */
} TIME;

typedef struct {   /* structure for holding date */
        int month;
        int day;
        int year;
   int dayofweek;
} DATE;
#endif

extern BOOL bUserAbort;
extern HWND hDlgPrint;
extern int nSizeNum, nSizeDen;
extern int copies, quality;
extern TCHAR tempPath[];
extern HBITMAP hbmWork,hbmImage;
extern HDC     hdcWork,hdcImage;
extern RECT theBounds;
//extern int cntr, dir, wid, hgt, halfWid, halfHgt;
extern int UpdateCount;
extern BOOL bJustActivated;
extern BOOL bPrtCreateErr;
extern BOOL IsCanceled;
extern int SizeTable[];
extern int YPosTable[];
extern int hSizePrt, vSizePrt;
extern int hResPrt, vResPrt;
extern int xPelsPrt, yPelsPrt;
extern BOOL fStretch;
extern TCHAR szHeader[], szFooter[];
extern BOOL IsConstrained;
extern RECT zoomRect;

#include "pbdecl.h"
#include "uniconv.h"

extern TCHAR acDbgBfr[];
extern BOOL bExchanged;
extern RECT rDirty;


#if DBG
#   define DPRINT(p)    OutputDebugString p
#else
#   define DPRINT(p)
#endif
