/** FILE: cplib.h ********** Module Header ********************************
 *
 *  Control panel utility library routines for use by control panel applets.
 *  Common definitions, resource ids, typedefs, external declarations and
 *  library routine function prototypes.
 *
 * History:
 *  15:30 on Thur  25 Apr 1991  -by-    Steve Cathcart   [stevecat]
 *       Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992	-by-	Steve Cathcart   [stevecat]
 *	    Updated code to latest Win 3.1 sources
 *
 *  Copyright (C) 1990-1992 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                        Typedefs and Definitions
//==========================================================================
// NOTE: The following lines are used by applets to define items in their
//               resource files.  These are necessary to be compatible with some of
//               library routines.
//
// Resource String ids for Applets
#define INITS           16
#define CLASS           36
#define COPY            (CLASS + 4)

#define UTILS           64
#define INSTALLIT  196

#define FOO -1                  // for useless control ids

#define IDD_HELP        200             // Help control id

#define CP_ACCEL        100             // Keyboard Accelerator table

// End resource file definitions

#define PATHMAX         133         // path length max

#define MYNUL     (LPSTR) szNull

#define COLOR_SAVE        711

#define  NOSELECT -1        // indices for int Selected
#define  HOUR     0             // index into rDateTime, wDateTime, wRange
#define  MINUTE   1
#define  SECOND   2
#define  MONTH    3
#define  DAY      4
#define  YEAR     5
#define  WEEKDAY  6
#if 0
#define  UPTIME   6
#define  DOWNTIME 7
#define  UPDATE   8
#define  DOWNDATE 9
#endif

typedef BOOL (APIENTRY *BWNDPROC)(HWND, UINT, DWORD, LONG);

#ifndef NOARROWS
typedef struct
  {
    short lineup;             /* lineup/down, pageup/down are relative */
    short linedown;           /* changes.  top/bottom and the thumb    */
    short pageup;             /* elements are absolute locations, with */
    short pagedown;           /* top & bottom used as limits.          */
    short top;
    short bottom;
    short thumbpos;
    short thumbtrack;
    BYTE  flags;              /* flags set on return                   */
  } ARROWVSCROLL;
typedef ARROWVSCROLL NEAR     *NPARROWVSCROLL;
typedef ARROWVSCROLL FAR      *LPARROWVSCROLL;

#define UNKNOWNCOMMAND 1
#define OVERFLOW       2
#define UNDERFLOW      4

#endif

#define COPY_CANCEL        0
#define COPY_SELF         -1
#define COPY_NOCREATE     -2
#define COPY_DRIVEOPEN    -3
#define COPY_NODISKSPACE  -4
#define COPY_NOMEMORY     -5

//  AddStringToObject defines
#define ASO_GLOBAL  0x0001
#define ASO_FIXED   0x0002
#define ASO_EXACT   0x0004
#define ASO_COMPACT 0x0008

/* Help defines */
#define IDH_HELPFIRST                   5000
#define IDH_DLGFIRST      (IDH_HELPFIRST + 3000)
#define IDH_DLG_CONFLICT  (IDH_DLGFIRST + DLG_CONFLICT)
#define IDH_DLG_ADDFILE   (IDH_DLGFIRST + DLG_ADDFILE)

#define MENU_INDHELP     40

//==========================================================================
//                              Macros
//==========================================================================
#define GSM(SM) GetSystemMetrics(SM)
#define GDC(dc, index) GetDeviceCaps(dc, index)

#define LPMIS LPMEASUREITEMSTRUCT
#define LPDIS LPDRAWITEMSTRUCT
#define LPCIS LPCOMPAREITEMSTRUCT

#define LONG2POINT(l, pt)   (pt.y = (int) HIWORD(l),  pt.x = (int) LOWORD(l))

//==========================================================================
//                         External Declarations
//==========================================================================
/* exported from applets  */
extern HANDLE hModule;


/* exported from cplib  */
/* initapp.c  */
extern char szOnString[];               // separator printer/port in listboxes
extern char szSeparator[];              // separator filename printer desc
extern char szDefNullPort[];            // default null port name
									
extern char szCtlPanel[];
extern char szErrMem[];

extern char szBasePath[];               /* Path to WIN.INI directory */
extern char szWinIni[];                 /* Path to WIN.INI */
extern char szWinCom[];                 /* Path to WIN.COM directory */
extern char szSystemIniPath[];          /* Path to SYSTEM.INI */
extern char szCtlIni[];                 /* Path to CONTROL.INI */
extern char szControlHlp[];
extern char szSetupInfPath[];
extern char szSetupDir[];
extern char szSharedDir[];

extern char pszSysDir[];
extern char pszWinDir[];
extern char pszClose[];
extern char pszContinue[];

extern char szSYSTEMINI[];
extern char szSETUPINF[];
extern char szCONTROLINF[];

extern char BackSlash[];
extern char szFOT[];
extern char szDot[];

extern unsigned wMerge;                 /* MERGE SPEC FOR STRINGS */

/* utiltext.c */
extern char szGenErr[];
extern char szNull[];
extern char szComma[];
extern char szSpace[];

extern short wDateTime[];                   // values for first 7 date/time items
extern short wModulos[];                    // highest value for hour, minute, second
extern short wPrevDateTime[];               // only repaint fields if nec

/* Help stuff */
extern DWORD dwContext;
extern WORD  wHelpMessage;
extern WORD  wBrowseMessage;
extern WORD  wBrowseDoneMessage;


// Originally from cpprn.c
extern short nDisk;
extern char szDrv[];
extern char szDirOfSrc[];               	// Directory for File copy
extern WORD nConfID;                    	// For conflict dialog


//==========================================================================
//                          Function Prototypes
//==========================================================================
/* utiltext.c */

void GetDate (void);
void GetTime (void);
void SetDate (void);
void SetTime (void);

void SetDateTime (void);                // [stevecat] - new functions
void GetDateTime (void);

DWORD  AddStringToObject(DWORD dwStringObject, LPSTR lpszSrc, WORD wFlags);
LPSTR  BackslashTerm (LPSTR pszPath);
void   ErrMemDlg(HWND hParent);
HANDLE FindRHSIni (LPSTR pFile, LPSTR pSection, LPSTR pRHS);
int    GetSection (LPSTR lpFile, LPSTR lpSection, LPHANDLE hSection);
short  myatoi(LPSTR pszInt);
HANDLE StringToLocalHandle (LPSTR lpStr);

#ifdef LATER
void   ErrWinDlg(HWND hParent);
short  Copy(HWND hParent, char *szSrcFile, char *szDestFile);
#endif  //  LATER

/* util.c */

int    DoDialogBoxParam(int nDlg, HWND hParent, WNDPROC lpProc,
                                        DWORD dwHelpContext, DWORD dwParam);
void   HourGlass (BOOL bOn);
int    MyMessageBox(HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType, ...);
void   SendWinIniChange(LPSTR szSection);
int    strpos(LPSTR,char);
char   *strscan(char *, char *);
void   StripBlanks( char * );

/* arrow.c */
short ArrowVScrollProc(short wScroll, short nCurrent, LPARROWVSCROLL lpAVS);
BOOL  OddArrowWindow(HWND);

// initapp.c (new)       (Originally from control.c)
BOOL AppletInit();

// addfile.c (new)   (Originally from cpprn.c)
BOOL AddFileDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam);

// conflict.c (new)   (Originally from cpprn.c)
BOOL ConflictDlg(HWND hDlg, UINT message, DWORD wParam, LONG lParam);

#if DBG
void  DbgPrint( char *, ... );
void  DbgBreakPoint( void );
#endif

