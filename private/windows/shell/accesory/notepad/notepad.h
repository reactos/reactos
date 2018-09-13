/* Notepad.h */

#define NOCOMM
#define NOSOUND
#include <windows.h>
#include <ole2.h>
#include <commdlg.h>
// we need this for CharSizeOf(), ByteCountOf(),
#include "uniconv.h"

/* handy debug macro */
#define ODS OutputDebugString

typedef enum _NP_FILETYPE {
   FT_UNKNOWN=-1,
   FT_ANSI=0,
   FT_UNICODE=1,
   FT_UNICODEBE=2,
   FT_UTF8=3,
} NP_FILETYPE;


#define BOM_UTF8_HALF        0xBBEF
#define BOM_UTF8_2HALF       0xBF


/* openfile filter for all text files */
#define FILE_TEXT         1


#define PT_LEN               40    /* max length of page setup strings */
#define CCHFILTERMAX         80    /* max. length of filter name buffers */

/* Menu IDs */
#define ID_APPICON           1 /* must be one for explorer to find this */
#define ID_ICON              2
#define ID_MENUBAR           1

/* Dialog IDs  */
#define IDD_ABORTPRINT       11
#define IDD_PAGESETUP        12
#define IDD_SAVEDIALOG       13    // template for save dialog
#define IDD_GOTODIALOG       14    // goto line number dialog

/* Control IDs */
#define IDC_FILETYPE         257   // listbox in save dialog
#define IDC_GOTO             258   // line number to goto
#define IDC_ENCODING         259   // static text in save dialog

/* Menu IDs */
#define M_OPEN               10
#define M_SAVE               1
#define M_SAVEAS             2
#define M_CUT                WM_CUT
#define M_COPY               WM_COPY
#define M_PASTE              WM_PASTE
#define M_CLEAR              WM_CLEAR
#define M_FIND               3
#define M_DOSEARCH           4
#define M_HELP               5
#define M_SELECTALL          7
#define M_FINDNEXT           8
#define M_NEW                9
#define M_ABOUT              11
#define M_DATETIME           12
#define M_PRINT              14
#define M_PRINTSETUP         31
#define M_PAGESETUP          32
#define M_UNDO               25
#define M_NOWW               26
#define M_WW                 27
#define M_EXIT               28
#define M_SETFONT            37

#define M_REPLACE            40
#define M_GOTO               41


#define ID_EDIT              15
#define ID_LISTBOX           16
#define ID_DIRECTORY         17
#define ID_PATH              18
#define ID_FILENAME          20
#define ID_PRINTER           21
#define ID_SETUP             22

#define ID_SEARCH            19
#define ID_SRCHFWD           10
#define ID_SRCHBACK          11
#define ID_SRCHCASE          12
#define ID_SRCHNOCASE        13
#define ID_PFREE             14

#define ID_HEADER            30
#define ID_FOOTER            31
#define ID_HEADER_LABEL      32
#define ID_FOOTER_LABEL      33

#define ID_ASCII             50
#define ID_UNICODE           51

#define IDS_DISKERROR        1
#define IDS_PA               2
#define IDS_FNF              3
#define IDS_CNF              4
#define IDS_FAE              5
#define IDS_OEF              6
#define IDS_ROM              7
#define IDS_YCNCTF           8
#define IDS_UE               9
#define IDS_SCBC             10
#define IDS_UNTITLED         11
#define IDS_NOTEPAD          12
#define IDS_LF               13
#define IDS_SF               14
#define IDS_RO               15
#define IDS_CFS              16
#define IDS_ERRSPACE         17
#define IDS_FTL              18
#define IDS_NN               19
#define IDS_COMMDLGINIT      23
#define IDS_PRINTDLGINIT     24
#define IDS_CANTPRINT        25
#define IDS_NVF              26
#define IDS_CREATEERR        30
#define IDS_NOWW             31
#define IDS_MERGE1           32
#define IDS_ANSI_EXT         33
#define IDS_HELPFILE         34
#define IDS_BADMARG          35
#define IDS_HEADER           36
#define IDS_FOOTER           37

#define IDS_LETTERS          42
#define IDS_FILEOPENFAIL     43
#define IDS_ANSITEXT         44
#define IDS_ALLFILES         45
#define IDS_OPENCAPTION      46
#define IDS_SAVECAPTION      47
#define IDS_CANNOTQUIT       48
#define IDS_LOADDRVFAIL      50
#define IDS_ACCESSDENY       51
#define IDS_ERRUNICODE       52


#define IDS_FONTTOOBIG       62
#define IDS_COMMDLGERR       63


#define IDS_LINEERROR        70  /* line number error     */
#define IDS_LINETOOLARGE     71  /* line number too large */

#define IDS_FT_ANSI          80  /* ascii              */
#define IDS_FT_UNICODE       81  /* unicode            */
#define IDS_FT_UNICODEBE     82  /* unicode big endian */
#define IDS_FT_UTF8          83  /* UTF-8 format       */

#define CSTRINGS             42  /* cnt of stringtable strings from .rc file */

#define CCHKEYMAX           128  /* max characters in search string */

#define CCHNPMAX              0  /* no limit on file size */

#define SETHANDLEINPROGRESS   0x0001 /* EM_SETHANDLE has been sent */
#define SETHANDLEFAILED       0x0002 /* EM_SETHANDLE caused EN_ERRSPACE */

/* Standard edit control style:
 * ES_NOHIDESEL set so that find/replace dialog doesn't undo selection
 * of text while it has the focus away from the edit control.  Makes finding
 * your text easier.
 */
#define ES_STD (WS_CHILD|WS_VSCROLL|WS_VISIBLE|ES_MULTILINE|ES_NOHIDESEL)

/* EXTERN decls for data */
extern NP_FILETYPE fFileType;     /* Flag indicating the type of text file */

extern BOOL fCase;                /* Flag specifying case sensitive search */
extern BOOL fReverse;             /* Flag for direction of search */
extern TCHAR szSearch[];
extern HWND hDlgFind;             /* handle to modeless FindText window */

extern HANDLE hEdit;
extern HANDLE hFont;
extern HANDLE hAccel;
extern HANDLE hInstanceNP;
extern HANDLE hStdCursor, hWaitCursor;
extern HWND   hwndNP, hwndEdit;

extern LOGFONT  FontStruct;
extern INT      iPointSize;

extern BOOL     fRunBySetup;

extern DWORD    dwEmSetHandle;

extern TCHAR    chMerge;

extern BOOL     fUntitled;
extern BOOL     fWrap;
extern TCHAR    szFileName[];
extern HANDLE   fp;

//
// Holds header and footer strings to be used in printing.
// use HEADER and FOOTER to index.
//
extern TCHAR    chPageText[2][PT_LEN]; // header and footer strings
#define HEADER 0
#define FOOTER 1
//
// Holds header and footer from pagesetupdlg during destroy.
// if the user hit ok, then keep.  Otherwise ignore.
//
extern TCHAR    chPageTextTemp[2][PT_LEN];

extern TCHAR    szNotepad[];
extern TCHAR   *szMerge;
extern TCHAR   *szUntitled, *szNpTitle, *szNN, *szErrSpace, *szBadMarg;
extern TCHAR   *szErrUnicode;
extern TCHAR  **rgsz[];          /* More strings. */
extern TCHAR   *szNVF;
extern TCHAR   *szFAE;
extern TCHAR   *szPDIE;
extern TCHAR   *szDiskError;
extern TCHAR   *szCREATEERR;
extern TCHAR   *szWE;
extern TCHAR   *szFTL;
extern TCHAR   *szINF;
extern TCHAR   *szFileOpenFail;
extern TCHAR   *szFNF;
extern TCHAR   *szNEDSTP;
extern TCHAR   *szNEMTP;
extern TCHAR   *szCFS;
extern TCHAR   *szPE;
extern TCHAR   *szCP;
extern TCHAR   *szACCESSDENY;
extern TCHAR   *szFontTooBig;
extern TCHAR   *szLoadDrvFail;
extern TCHAR   *szCommDlgErr;
extern TCHAR   *szCommDlgInitErr;
extern TCHAR   *szHelpFile;

extern TCHAR   *szFtAnsi;
extern TCHAR   *szFtUnicode;
extern TCHAR   *szFtUnicodeBe;
extern TCHAR   *szFtUtf8;

/* variables for the new File/Open and File/Saveas dialogs */
extern OPENFILENAME OFN;        /* passed to the File Open/save APIs */
extern TCHAR  szOpenFilterSpec[]; /* default open filter spec          */
extern TCHAR  szSaveFilterSpec[]; /* default save filter spec          */
extern TCHAR *szAnsiText;       /* part of the text for the above    */
extern TCHAR *szAllFiles;       /* part of the text for the above    */
extern FINDREPLACE FR;          /* Passed to FindText()        */
extern PAGESETUPDLG g_PageSetupDlg;
extern TCHAR  szPrinterName []; /* name of the printer passed to PrintTo */

extern NP_FILETYPE    g_ftOpenedAs;     /* file was opened           */
extern NP_FILETYPE    g_ftSaveAs;       /* file was saved as type    */

extern UINT   wFRMsg;           /* message used in communicating    */
                                /*   with Find/Replace dialog       */
extern UINT   wHlpMsg;          /* message used in invoking help    */

extern HMENU hSysMenuSetup;     /* Save Away for disabled Minimize   */

/* EXTERN procs */
/* procs in notepad.c */
VOID
PASCAL
SetPageSetupDefaults(
    VOID
    );

BOOL far PASCAL SaveAsDlgHookProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

LPTSTR PASCAL far PFileInPath (LPTSTR sz);

BOOL FAR CheckSave (BOOL fSysModal);
LRESULT FAR NPWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void FAR SetTitle (TCHAR *sz);
INT FAR  AlertBox (HWND hwndParent, TCHAR *szCaption, TCHAR *szText1,
                   TCHAR *szText2, UINT style);
void FAR NpWinIniChange (VOID);
void FAR FreeGlobalPD (void);
INT_PTR CALLBACK GotoDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

/* procs in npdate.c */
VOID FAR InsertDateTime (BOOL fCrlf);

/* procs in npfile.c */
BOOL FAR  SaveFile (HWND hwndParent, TCHAR *szFileSave, BOOL fSaveAs);
BOOL FAR  LoadFile (TCHAR *sz, INT type );
VOID FAR  New (BOOL  fCheck);
void FAR  AddExt (TCHAR *sz);
INT FAR   Remove (LPTSTR szFileName);
VOID FAR  AlertUser_FileFail( LPTSTR szFileName );

/* procs in npinit.c */
INT FAR  NPInit (HANDLE hInstance, HANDLE hPrevInstance,
                 LPTSTR lpCmdLine, INT cmdShow);
void FAR InitLocale (VOID);
void SaveGlobals( VOID );

/* procs in npmisc.c */
INT FAR  FindDlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL     Search (TCHAR *szSearch);
INT FAR  AboutDlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL FAR NpReCreate (LONG style);
LPTSTR   ForwardScan (LPTSTR lpSource, LPTSTR lpSearch, BOOL fCaseSensitive);


/* procs in npprint.c */
typedef enum _PRINT_DIALOG_TYPE {
   UseDialog,
   DoNotUseDialog,
   NoDialogNonDefault
} PRINT_DIALOG_TYPE;

INT    AbortProc( HDC hPrintDC, INT reserved );
INT_PTR AbortDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
INT    NpPrint( PRINT_DIALOG_TYPE type );
INT    NpPrintGivenDC( HDC hPrintDC );

UINT_PTR
CALLBACK
PageSetupHookProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

HANDLE GetPrinterDC (VOID);
HANDLE GetNonDefPrinterDC (VOID);
VOID   PrintIt(PRINT_DIALOG_TYPE type);


/* procs in nputf.c */

INT    IsTextUTF8   (LPSTR lpstrInputStream, INT iLen);
INT    IsInputTextUnicode(LPSTR lpstrInputStream, INT iLen);



// Help IDs for Notepad

#define NO_HELP                         ((DWORD) -1) // Disables Help for a control

#define IDH_PAGE_FOOTER                 1000
#define IDH_PAGE_HEADER                 1001
#define IDH_FILETYPE                    1002
#define IDH_GOTO                        1003

// Private message to track the HKL switch

#define PWM_CHECK_HKL                   (WM_APP + 1)

