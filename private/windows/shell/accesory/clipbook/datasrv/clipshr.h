#ifndef _WINDOWS_
#include <windows.h>
#endif

#ifndef _INC_COMMDLG
#include <commdlg.h>
#endif

// added for winball - clausgi
extern UINT cf_link;
extern UINT cf_objectlink;
extern UINT cf_linkcopy;
extern UINT cf_objectlinkcopy;

// end additions

#define PRIVATE_FORMAT(fmt)	((fmt) >= 0xC000)

/* Header text string ids */
#define IDS_NAME            100  /* CF_TEXT to CF_OEMTEXT (1 to 7) are also used */
#define IDS_OVERWRITE	    101
#define IDS_ERROR           102  /* as string ids.  Be sure to keep these    */
#define IDS_BINARY          103  /* different.                               */
#define IDS_CLEAR	          104
#define IDS_FMTNOTSAV       105
#define IDS_DEFAULT         106
#define IDS_CANTDISPLAY     107  /* "Can't display data in this format" */
#define IDS_NOTRENDERED     108  /* "Application Couldn't render data"  */
#define IDS_HELPFILE        109  /* Clipbrd.hlp */
#define IDS_ALREADYOPEN     112  /* OpenClipboard() fails */
#define IDS_INVALIDFILENAME 113  /* Filename is invalid */
#define IDS_OPENCAPTION     114  /* ID of File/Open dlg. caption string */
#define IDS_SAVECAPTION     115  /* ID of File/Save dlg. caption string */
#define IDS_FILTERTEXT      116  /* ID of filter string for File/Open	*/
#define IDS_ALLFILES        117  /* ID of filter string for All Files	*/
#define IDS_MEMERROR        118

#define IDS_READERR 	    200  /* ID of base ReadClipboardFile error */
#define IDS_READFORMATERR   201
#define IDS_READOPENCLIPERR 202

/* Dialogbox resource id */
#define ABOUTBOX        1
#define CONFIRMBOX	2

/* Other constants */
#define CDEFFMTS        8       /* Count of predifined clipboard formats    */
#define VPOSLAST        100     /* Highest vert scroll bar value */
#define HPOSLAST        100     /* Highest horiz scroll bar value */
#define CCHFMTNAMEMAX   79      /* Longest clipboard data fmt name, including
                                   terminator */
#define cLineAlwaysShow 3       /* # of "standard text height" lines to show
                                   when maximally scrolled down */
#define BUFFERLEN       160      /* String buffer length */
#define SMALLBUFFERLEN  90
#define IDSABOUT        1

#define CBMENU		1	/* Number for the Clipboard main menu  */

#define FILTERMAX   100		/* max len. of File/Open filter string */
#define CAPTIONMAX  30		/* len of caption text for above dlg.  */
#define PATHMAX     128 	/* max. len of DOS pathname	       */

/* The menu ids */
#define CBM_AUTO        WM_USER
#define CBM_CLEAR       WM_USER+1
#define CBM_OPEN        WM_USER+2
#define CBM_SAVEAS      WM_USER+3
#define CBM_ABOUT       WM_USER+4
#define CBM_EXIT        WM_USER+5

// winball add-ons

#define CBM_SHAREAS		WM_USER+6
#define	CBM_IMPORT		WM_USER+7

// end winball add-ons

#define CBM_HELP	0xFFFF	 /* Standard numbers */
#define CBM_USEHELP     0xFFFC   /* Standard numbers */
#define CBM_SEARCH	0x0021

/*  Last parameter to SetDIBits() and GetDIBits() calls */

#define  DIB_RGB_COLORS   0
#define  DIB_PAL_COLORS	  1

#define  IDCLEAR 	IDOK

/* Structures for saving/loading clipboard data from disk */

#define      CLP_ID  0xC350
#define   CLP_NT_ID  0xC351
#define CLPBK_NT_ID  0xC352

typedef struct
   {
   WORD        magic;
   WORD        FormatCount;
   } FILEHEADER;


// Format header
typedef struct
   {
   DWORD FormatID;
   DWORD DataLen;
   DWORD DataOffset;
   WCHAR  Name[CCHFMTNAMEMAX];
   } FORMATHEADER;

// Windows 3.1-type structures - Win31 packed on byte boundaries.
#pragma pack(1)
typedef struct
   {
   WORD FormatID;
   DWORD DataLen;
   DWORD DataOffset;
   char Name[CCHFMTNAMEMAX];
   } OLDFORMATHEADER;

// Windows 3.1 BITMAP struct - used to save Win 3.1 .CLP files
typedef struct {
   WORD bmType;
   WORD bmWidth;
   WORD bmHeight;
   WORD bmWidthBytes;
   BYTE bmPlanes;
   BYTE bmBitsPixel;
   LPVOID bmBits;
   } WIN31BITMAP;

// Windows 3.1 METAFILEPICT struct
typedef struct {
   WORD mm;
   WORD xExt;
   WORD yExt;
   WORD hMF;
   } WIN31METAFILEPICT;

#pragma pack()


BOOL NEAR PASCAL OpenClipboardFile( HWND, LPTSTR );
void NEAR PASCAL GetClipboardName(int fmt, LPTSTR szName, int iSize);
BOOL ReadFormatHeader(HANDLE, FORMATHEADER *,unsigned);
unsigned ReadFileHeader(HANDLE);

LONG FAR PASCAL ClipbrdWndProc(HWND, WORD, WORD, LONG);
BOOL FAR PASCAL ConfirmDlgProc(HWND, WORD, WORD, LONG);

// Clipboard open/close with synchronization
BOOL SyncOpenClipboard(HWND);
BOOL SyncCloseClipboard(void);

/* Far low mem situations. */
void FAR PASCAL MemErrorMessage(void);


/*****************************  global data  *******************************/
// extern OFSTRUCT	ofStruct;
extern HWND  hwndMain;
extern TCHAR szAppName[];
extern TCHAR szFileSpecifier[];

/* variables for the new File Open,File SaveAs and Find Text dialogs */

extern TCHAR  szSaveFileName [];
extern TCHAR  szLastDir  [];
extern TCHAR  szFilterSpec [];    /* default filter spec. for above	 */
extern int    wHlpMsg;            /* message used to invoke Help	 */
extern TCHAR  szOpenCaption [];   /* File open dialog caption text	 */
extern TCHAR  szSaveCaption [];   /* File Save as dialog caption text  */
