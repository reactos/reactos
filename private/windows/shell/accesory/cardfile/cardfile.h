/*
 * cardfile.h main include file for CARDFILE
 */
#include <windows.h>
#include "indexrc.h"
#include <commdlg.h>                            /* Standard dialogs */
#include "ecda.h"                /* OLE */

/*
 * WIN16/32 compatability
 */
#if defined(WIN32)
#define HFILE    HANDLE
#define HUGE
#define WPARAM  DWORD
#define MYPOINT POINTS
#define LPMYPOINT POINTS *
#define MYMAKEPOINT(l) MAKEPOINTS(l)
#define MYPOINTTOPOINT( pt, pts ) { (pt).x = (LONG)(pts).x ; \
                                    (pt).y = (LONG)(pts).y ; }
#define MyGetTextExtent( a, b, c, d ) GetTextExtentPoint( a, b, c, &d )
#ifndef JAPAN
#define IsDBCSLeadByte(a) a,FALSE
#endif
#define LockData(h) (h)
#define UnlockData(h) (h)
#else
#define HFILE   int
#define HUGE    huge
#define WPARAM  WORD
#define UINT    unsigned int
#define WNDPROC FARPROC
#define MYPOINT POINT
#define MYMAKEPOINT(l) MAKEPOINT(l)
#define MyGetTextExtent( a, b, c, d ) d = GetTextExtent( a, b, c )
#endif


#define KEYNAMESIZE     300
#define OBJSTRINGSMAX    64

#ifdef DEBUG
#define NOEXPORT
#else
#define NOEXPORT static
#endif

#define     ABOUT       100
#define     NEW         101
#define     OPEN        102
#define     SAVE        103
#define     SAVEAS      104
#define     PRINT       105
#define     PRINTALL    106
#define     MERGE       107
#define     EXIT        108
#define     PAGESETUP   109
#define     PRINTSETUP  110

#define     CCARDFILE   141
#define     PHONEBOOK   142
#define     VIEW        143

#define     UNDO        118
#define     HEADER      111
#define     RESTORE     112
#define     CUT         113
#define     COPY        114
#define     PASTE       115
#define     I_TEXT      116
#define     I_OBJECT    117
#define     IDM_SETFONT 120
#define     ADD         121
#define     CARDDELETE  122
#define     DUPLICATE   123
#define     DIAL        124
#define     GOTO        131
#define     FIND        132
#define     FINDNEXT    133
#define     IDM_PASTESPECIAL    134
#define     IDM_INSERTOBJECT    135


/* Use standard help numbers */
#define     ID_INDEX    301             //0xFFFF
#define     ID_SEARCH   302             //0x0021
#define     ID_USEHELP  303             //0XFFFC


#define     EDITWINDOW      200
#define     LEFTWINDOW      201
#define     RIGHTWINDOW     202
#define     SCROLLWINDOW    203
#define     LISTWINDOW      204
#define     CARDWINDOW      205

/* -------------------------------------------------------------------- */
/* Added conditional compilation for long filename support under OS/2   */
/* t-carlh - August, 1990                                               */
/* -------------------------------------------------------------------- */
#ifdef OS2
#define PATHMAX 260     /* Maximum OS/2 filename length */
#else
#if defined(WIN32)
#define PATHMAX MAX_PATH
#else
#define PATHMAX 128     /* Maximum filename length */
#endif
#endif

#define PT_LEN  50              /* max length of page setup strings */

#if DBG
extern TCHAR dbuf[100];
#endif

extern TCHAR szText[];          /* buffer for reading in card text, etc */
extern TCHAR DefaultNullStr[];

extern BOOL bAskForUpdate;

extern UINT wFRMsg;              /* Registered "Find/Replace" message */

extern TCHAR *szOpenMergeText;

extern HWND hListWnd;
extern HWND hCardWnd;
extern HWND hIndexWnd;
extern HWND hEditWnd;
extern HWND hLeftWnd;
extern HWND hRightWnd;
extern HWND hScrollWnd;
extern HWND hEditWnd;
extern HWND hDlgFind;
extern BOOL fInSaveAsDlg;
extern BOOL fValidate;
extern TCHAR szValidateFileWrite[];

extern HANDLE hAccel;
extern HANDLE hEditCurs;
extern HANDLE hIndexInstance;
extern HDC hMemoryDC;
extern HDC hDisplayDC;

#define INITPOINTSIZE 120      // initial point size of font in edit control
extern LOGFONT FontStruct;     // logical font in edit control
extern INT     iPointSize;     // current point size of FontStruct
extern HFONT   hFont;          // current font in edit control

extern INT EditWidth;
extern INT EditHeight;

extern INT xCardWnd;        /* size of the Card window */
extern INT yCardWnd;
extern INT cxHScrollBar;
extern INT cyHScrollBar;

extern int CardPhone;
extern int fCanPrint;
extern BOOL bPrinterSetupDone;
extern int fNeedToUpdateObject;
extern int fSettingText;

extern OPENFILENAME OFN;
extern FINDREPLACE FR;
extern PRINTDLG PD;

extern TCHAR szFileName[];
extern TCHAR TempFile[];
extern TCHAR szOpenCaption[];       /* File open dialog caption text       */
extern TCHAR szSaveCaption[];       /* File Save as dialog caption text    */
extern TCHAR szMergeCaption[];      /* File Merge dialog caption text      */

extern TCHAR chPageText[6][PT_LEN];
extern TCHAR SavedIndexLine[];
extern int DBcmd;
extern int ySpacing;


extern TCHAR szPrinter[];
extern TCHAR szMarginError[160];
extern TCHAR szDec[];
extern int fEnglish;               /* true if using English measure system  */

extern TCHAR CurIFile[PATHMAX];
extern TCHAR szSearch[40];
extern BOOL fCase, fReverse, fRepeatSearch;

extern FARPROC lpfnAbortProc;
extern FARPROC lpfnAbortDlgProc;
extern FARPROC lpfnPageDlgProc;
extern FARPROC lpPrinterSetDlg;
extern FARPROC lpDlgProc;
extern FARPROC lpfnOpen;
extern FARPROC lpfnSave;
extern FARPROC lpfnDial;
extern FARPROC lpEditWndProc;
extern FARPROC lpfnLinksDlg;           /* OLE */
extern FARPROC lpfnInvalidLink;        /* OLE */

extern TCHAR szCardView[25];
extern TCHAR szListView[25];

extern HANDLE fhMain;        /* file handle for source file                */
extern WORD   fFileType;     /* file type indicator (OLD_FORMAT, ANSI_FILE or UNICODE_FILE) */
extern int    fReadOnly;
extern TCHAR  szValidateFileWrite[];
extern BOOL   fValidate;     /* TRUE if validating on save       */
extern TCHAR  TempFile[];
extern int    fNoTempFile;
extern BYTE   chDKO[];       /* Unicode cardfile signature.(OLE) */
extern BYTE   chRRG[];       /* new cardfile signature.(OLE)     */
extern BYTE   chMGC[];       /* old cardfile signature.          */
extern TCHAR  SavedIndexLine[];

/* these must be in the same order as the openfile dialog filter */
#define UNICODE_FILE   1        /* new Unicode format type  */
#define ANSI_FILE      2        /* new OLE ANSI format type */
#define OLD_FORMAT     3        /* 3.0 Cardfile format type */


#define LINELENGTH  40
#define CARDLINES   11
#define CARDTEXTSIZE    ((LINELENGTH+1) * CARDLINES)
#define CARDCY      12

typedef struct tagTIME
{
    TCHAR szSep[2];          /* Separator character for date string */
    TCHAR sz1159[6];         /* string for AM */
    TCHAR sz2359[6];         /* string for PM */
    int   iTime;             /* time format */
    int   iTLZero;           /* lead zero for hour */
} TIME;

/* max size of short date format string */
#define MAX_FORMAT              12

typedef struct tagDATE
{
    TCHAR szFormat[MAX_FORMAT];
} DATE;

typedef struct
{
    BYTE     reserved[6];               /* bytes available for future use */
    DWORD    lfData;                    /* file offset of data */
    BYTE     flags;                     /* flags */
    CHAR     line[LINELENGTH+1];        /* 40 character lines plus null */
} CARDHEADERA;

typedef CARDHEADERA *PCARDHEADERA;
typedef CARDHEADERA far *LPCARDHEADERA;

typedef struct
{
    BYTE     reserved[6];               /* bytes available for future use */
    DWORD    lfData;                    /* file offset of data */
    BYTE     flags;                     /* flags */
    WCHAR    line[LINELENGTH+1];        /* 40 character lines plus null */
} CARDHEADERW;

typedef CARDHEADERW *PCARDHEADERW;
typedef CARDHEADERW far *LPCARDHEADERW;

#ifdef UNICODE
#define CARDHEADER   CARDHEADERW
#else
#define CARDHEADER   CARDHEADERA
#endif

typedef CARDHEADER *PCARDHEADER;
typedef CARDHEADER far *LPCARDHEADER;

// do something to get rid of warnings
#ifndef OLE_20
#define OLECHAR CHAR
#else
#define OLECHAR TCHAR
#endif

#define ODS OutputDebugString


// FOR PACKING/NON_PACKING PORTABILITY
#if !defined (WIN32)
#define SIZEOFCARDHEADERA sizeof(CARDHEADERA)
#define SIZEOFCARDHEADERW sizeof(CARDHEADERW)
#else
#define SIZEOFCARDHEADERW (11 + (LINELENGTH + 1) * sizeof(WCHAR))
#define SIZEOFCARDHEADERA (11 + LINELENGTH + 1)
#ifdef UNICODE
#define SIZEOFCARDHEADER  SIZEOFCARDHEADERW
#else
#define SIZEOFCARDHEADER  SIZEOFCARDHEADERA
#endif
#endif


typedef struct
{
    /*
     * For OLE, the BITMAP is just a special frozen object.
     *
     * If we're in compatibility mode (lpObject is an HBITMAP) the
     * selector of lpObject will be NULL.
     */
    DWORD       idObject;        /* What is the object name? */
    OBJECTTYPE  otObject;        /* What kind of object is this? */
    RECT        rcObject;        /* Where should the object be drawn? */
    LPOLEOBJECT lpObject;        /* Handle to the object */
    HANDLE      hText;
} CARD, FAR *PCARD;

extern DWORD idObjectMax;

extern int    cCards;        /* the current count of cards */
extern HANDLE hCards;        /* the handle to the buffer   */

extern INT CharFixWidth;    /* the character width  */
extern INT CharFixHeight;   /* the character height */
extern INT ExtLeading;      /* the external leading */

extern int CardWidth;
extern int CardHeight;
extern int fColor;

extern HBRUSH hbrCard;
extern HBRUSH hbrBorder;
extern HANDLE hArrowCurs;
extern HANDLE hWaitCurs;

extern CARDHEADER CurCardHead;
extern CARD       CurCard;

#define FNEW        0x01
#define FDIRTY      0x02
#define FTMPFILE    0x04

/* Return values from OleError: No problem, error with/without warning */
#define FOLEERROR_OK       0x00
#define FOLEERROR_GIVEN    0x01
#define FOLEERROR_NOTGIVEN 0x02

extern RECT dragRect;

extern int iTopScreenCard;
extern int iCurCard;
extern int iFirstCard;

#define LEFTMARGIN      8
#define TOPMARGIN       8
#define BOTTOMMARGIN    8

extern int cScreenHeads;
extern int cScreenCards;
extern int cFSHeads;        /* count of fully visible headers */

#define SBINDEX SB_HORZ

extern int ySpacing;
extern int xFirstCard;
extern int yFirstCard;

extern int EditMode;

extern TCHAR *pcComNum;
extern TCHAR *pcTonePulse;
extern TCHAR *pcSpeedNum;

extern TCHAR szMerge[3];       /* two characters for string insertion   */

extern BOOL fFileDirty;        /* does disk match current set of cards? */
extern BOOL fFullSize;

extern BOOL fInsertComplete;   /* If insertObject in progress, it is FALSE   */
                               /* Reset to TRUE when InsertObjectis complete */

#ifdef JAPAN    //KKBUGFIX  #3082: 02/02/1993: Disabling IME while Picture mode
extern BOOL fNowFocus;
#endif

#define INDEXICON   1
#define MAINACC     1
#define MTINDEX     1

/* prompts */
#define IDELCURCARD     0
#define ICREATEFILE     1
#define IOKTOSAVE       2
#define ICARDS          3
#define ICARD           4
#define IONECARD        5
#define IUNTITLED       6
#define ICARDDATA       7
#define IWARNING        8
#define INOTE           9
#define IPICKUPPHONE    10
#define ICARDFILE       11
#define IMARGINERR      12
#define IMERGE          13
#define IOPEN           14
#define ICARDVIEW       15
#define ILISTVIEW       16
#define IHELPFILE       18

/* errors */
#define ECANTDIAL       19
#define ECANTPRINTPICT  20
#define EINSMEMORY      21
#define EINVALIDFILE    23
#define ECLIPEMPTYTEXT  24
#define ENOTEXTSEL      25
#define EDISKFULLFILE   26
#define ECANTMAKETEMP   27
#define EINSMEMSAVE     28
#define EDISKFULLSAVE   29
#define EOPENTEMPSAVE   30
#define ECANTREADPICT   31
#define EINSMEMRUN      32
#define ENOTVALIDFILE   33
#define ECANTFIND       34
#define EINSMEMREAD     35
#define ECANTPRINT      36
#define ECLIPEMPTYPICT  37
#define ENOMODEM        38
#define ENOPICTURES     39
#define ECANTMAKEFILE   40
#define EMEMPRINT       41
#define EDISKPRINT      42
#define ISTRINGINSERT   43
#define IFILEEXTENSION  44
#define E_NOUNICODEFONT 45
#define E_UNICODETEXT   46

/**** WARNING!  DO NOT CHANGE THE ORDER OF THESE! ***/
#define IHEADER         50
#define IFOOTER         51
#define ILEFT           52
#define IRIGHT          53
#define ITOP            54
#define IBOT            55
/*** END OF WARNING ***/

#define IDS_LETTERS     60
/* A whole slew of E_ errors defined in ecda.h, starting at 0x100 */
/* A few W_ errors defined in ecda.h, starting at 0x200 */
#define E_CANT_REOPEN_FILE       61
#define E_FILEUPDATEFAILED       62
#define E_FILEWRITEFAILED        63
#define E_FILECANTWRITE          64
#define IDS_CANNOTQUIT           65
#define E_CANTSAVETOREADONLYFILE 66
#define W_OPENFILEFORREADONLY    67
#define E_PASTEDTEXTTOOLONG      68
#define E_FILESAVE               69

#define CAPTIONMAX  30          /* max size of captions in dialogs  */
#define OBJNAMEMAX  30          /* Length of IDS_OBJNAME w/o the %d */

#define IDS_FILTERSPEC    0x300   /* file open/save dlg. filter spec. string */
#define IDS_FILTERSPEC2   0x301
#define IDS_FILTERSPEC3   0x302
#define IDS_FILTERSPEC4   0x303
#define IDS_OPENDLGTITLE  0x304   /* caption of the open dialog              */
#define IDS_SAVEDLGTITLE  0x305   /* caption of the saveas dialog            */
#define IDS_MERGEDLGTITLE 0x306   /* caption of the merge dialog             */
#define IDS_LINKTITLE     0x307   /* Caption of the link repair dialog       */
#define IDS_VALIDATE      0x308   /* Validate check box text */
#define IDS_OBJNAME       0x309   /* Used to construct OLE object names */
#define IDS_UPDATEEMBOBJECT  0x30A
#define IDS_RETRYAFTERINSERT 0x30B
#define IDS_OBJECTCMD     0x30C
#define IDS_PICTUREFMT    0x30D
#define IDS_BITMAPFMT     0x30E
#define IDS_UPDATELINK    0x30F
#define IDS_OBJECTFMT     0x310
#define IDS_PopupVerbs    0x311
#define IDS_SingleVerb    0x312
#define IDS_Edit          0x313
#define IDS_NOOLESERVERS  0x314
#define IDS_SPACEISINCH   0x315
#define IDS_SPACEISCENTI  0x316

extern TCHAR szCardfile[40];
extern TCHAR szCardClass[];
extern TCHAR szWindows[];
extern TCHAR szDevice[];

extern TCHAR NotEnoughMem[160];
extern TCHAR szHelpFile[30];
extern TCHAR szUntitled[30];
extern TCHAR szWarning[30];
extern TCHAR szNote[30];
extern TCHAR szFileExtension[5];

extern int iTopCard;

extern TCHAR szServerFilter[];      /* default filter spec. for servers  */
extern TCHAR szFilterSpec[];        /* default filter spec. */
extern TCHAR szLinkCaption[];
extern TCHAR szLastDir[];

HANDLE hFind;
extern TCHAR szCustFilterSpec[];    /* buffer for custom filters created */
extern BOOL fValidDB;               /* Validate on/off in Save dialog box */

#include "funcs.h"
#include "uniconv.h"

