/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
#include <stdlib.h>

/****************************************************************************

    EXTERNALS:

****************************************************************************/
//Handle to QcQp Colors Palette
extern HPALETTE hPal;

//Handle to instance data
extern HINSTANCE g_hInst;

//Handle to ini allocated memory
extern HANDLE hIniMem;

//Handle to ini file
extern int iniHandle;

//Handle to ini file
extern int iniHandle;

//Generic handles
extern HANDLE hGeneric1, hGeneric2;

//Generic words
extern WORD wGeneric1, wGeneric2;

//Generic __FAR pointer, (typically used for Xalloc/Xfree calls)
extern LPSTR lpGeneric1, lpGeneric2;

//Handle to edit control
extern HWND hwndActiveEdit;

// Handle to currently activated MDI debug window
extern HWND hwndDebug;

//Main window frame
extern HWND hwndFrame;

//Standard File Extensions
extern char szStarDotC[_MAX_EXT];
extern char szStarDotH[_MAX_EXT];
extern char szStarDotCPP[_MAX_EXT];
extern char szStarDotCXX[_MAX_EXT];
extern char szStarDotLib[_MAX_EXT];
extern char szStarDotObj[_MAX_EXT];
extern char szStarDotDef[_MAX_EXT];
extern char szStarDotMak[_MAX_EXT];
extern char szStarDotRc[_MAX_EXT];
extern char szStarDotDlg[_MAX_EXT];
extern char szStarDotIco[_MAX_EXT];
extern char szStarDotCur[_MAX_EXT];
extern char szStarDotBmp[_MAX_EXT];
extern char szStarDotFnt[_MAX_EXT];
extern char szStarDotRes[_MAX_EXT];
extern char szStarDotExe[_MAX_EXT];
extern char szStarDotCom[_MAX_EXT];
extern char szStarDotDLL[_MAX_EXT];
extern char szStarDotInc[_MAX_EXT];
extern char szStarDotStar[_MAX_EXT];

//Input/output variables
extern char szDefExt[_MAX_EXT];
extern char szDefPath[_MAX_PATH];

// temp variables
extern TCHAR szHelpFileName[_MAX_PATH];

//Temp variables for various modules
extern char szTmp[TMP_STRING_SIZE];

//Syntax Coloring
extern BOOL syntaxColors;

// Commands to execute on certain events
extern char szStopEventCmd[];

//Editor : Current view
extern int curView;


//Array position in ppszMakefileExtTab of currently selected extension
extern WORD nCurExt;

// Handles to Include and Library Environment strings.
extern HANDLE hEnvIncludeStr;
extern HANDLE hEnvLibraryStr;

//Help mode flag; TRUE = "ON"
extern BOOL bHelp;

// Handle to MDI client
extern HWND g_hwndMDIClient;

// Handle to currently activated child
extern HWND hwndActive;


#ifdef NEW_WINDOWING_CODE
// Structure containing all of the window handles
extern DEBUGGER_WINDOWS g_DebuggerWindows;
#endif

//Handle to accelerator table
extern HACCEL hMainAccTable;
extern HACCEL hCurrAccTable;
extern HACCEL hCmdWinAccTable;

//Keyboard Hooks functions
extern HHOOK hKeyHook;

//Replace/Find Data Structure
extern _FINDREPLACE FAR findReplace;
extern _FINDREPLACEMEM FAR frMem;

//Editor : Global documents struct
extern DOCREC Docs[MAX_DOCUMENTS];

//Editor : Global documents struct
extern VIEWREC  Views[MAX_VIEWS];

// Currently highlighted trace line (F8/F10)
extern TRACEINFO TraceInfo;

//Syntax memory line
extern char st[MAX_USER_LINE];

//Syntax memory line
extern char st[MAX_USER_LINE];

//CrLf
extern char CrLf[3];

//Number of different colors in a line
extern int nbColors;

//Screen colors saved
//Line len divided by 2 because different colors will be separated
//by spaces
extern COLORINFO colors[MAX_USER_LINE + 3]; //+ 3 =When selecting

// Flag that allows quickw.c to tell modify.c that it has called it
// and the expression is passed in the string pointed to by the
// public global var lpGeneric1
extern BOOL CalledFromQWatch;

//Emergency Flag (exit without processing WM_DESTROY and WM_CLOSE, or
//cancelled rec)
extern BOOL emergency;

//CheckSum in ini file
extern WORD checkSum;

//Record buffer overflow counter
extern WORD recordBufferOverflow;

//Current Help Id for Open, Merge, Save and Open project dialog box
extern WORD curHelpId;

//Empty string
extern char szNull[1];

//State of keyboard when key is down
extern BOOL isShiftDown;
extern BOOL isCtrlDown;

//Record stop mark
extern int stopMarkStatus;

//For modeless dialog box control
extern BOOL exitModelessDlg;

//Editor : Standard editor win proc and subclassing functions
extern WNDPROC lpfnEditProc;

//Debugger window view #
extern int cpuView;
extern int errorView;
extern int watchView;
extern int localsView;
extern int disasmView;
extern int cmdView;
extern int floatView;
extern int memView;
extern int callsView;

// Error Window : Various Variables
extern HWND hErrorsEdit;
extern PERRORNODE __FAR CurrentError; //CurrentError is used for F3/F4 handling. (Next/Previous error)
extern PERRORNODE __FAR FirstError, __FAR LastError;
extern int __FAR CurrentErrorIndex;
extern char __FAR ErrorBase[_MAX_PATH];

//Fonts variables
extern int __FAR fontsNb;
extern int __FAR fontSizesNb;
extern int __FAR fontCur;
extern int __FAR fontSizeCur;
extern LPLOGFONT fonts; //Variable size
extern LPINT     fontSizes; //Variable size
extern LPINT     fontSizesPoint; //Variable size
//extern LOGFONT   defaultFont;
extern LOGFONT   g_logfontDefault;

//Last position of cursor before undo
extern int mC;
extern int dL;

extern int  iDebugLevel;
extern EERADIX radix;

// Number of dialog/message boxes currently open
extern int FAR BoxCount;

//Option file full path name (qcwin.ini)
extern char FAR iniFileName[_MAX_PATH];

//Regular expression pattern (must be near)
extern struct patType *pat;

//TRUE after PostQuitMessage has been called.
extern BOOL QuitTheSystem;

//True when terminate application has been called
extern BOOL FAR TerminatedApp;

//Default ini mode
extern BOOL defaultIni;

//Checking date of files for reload
extern BOOL checkFileDate;

//Critical section for editor
extern BOOL editorIsCritical;

//Title bar data
extern TITLEBAR TitleBar;

//Current line expanded (tabs converted)
extern char el[MAX_USER_LINE * 2 + 1];
extern int elLen;

//Current line and current block
extern LPLINEREC pcl;
extern LPBLOCKDEF pcb;

//View tabs
extern BOOL viewTabs;

//Playing records mode
extern playingRecords;

//Windows Version
extern int winVer;

//Scroll origin, KEYBOARD or MOUSE
extern WORD scrollOrigin;

//Autotest mode for test suites
extern AUTORUN AutoRun;
extern BOOL AutoTest;
extern BOOL NoPopups;
extern BOOL RemoteRunning;
extern char * PszAutoRun;

// Low memory messages
extern char LowMemoryMsg[MAX_MSG_TXT];
extern char LowMemoryMsg2[MAX_MSG_TXT];

// WinDBG title text
extern char MainTitleText[MAX_MSG_TXT];

extern char fCaseSensitive;
extern char SuffixToAppend;

extern LPSTR LpszCommandLine;

extern LPTD LptdCur;    // Pointer to the current thread
extern LPPD LppdCur;    // Pointer to the current process
extern LPPD LppdFirst;  // Pointer to the first (or base) process


//

extern  BOOL    FAsmMode;   // Are we currently in assembly mode?

extern HINSTANCE  HModSH;      // Symbol handler DLL handle
extern HINSTANCE  HModEE;      // Expression evaluator DLL
extern HINSTANCE  HModTL;      // Transport layer DLL handle
extern HINSTANCE  HModEM;      // Execution model DLL handler

//
//  Current directories
//
extern char SrcFileDirectory[ MAX_PATH ];
extern char ExeFileDirectory[ MAX_PATH ];
extern char DocFileDirectory[ MAX_PATH ];
extern char UserDllDirectory[ MAX_PATH ];

extern BOOL FSourceOverlay;     /* TRUE -> overlay source windows */

extern BOOL fUseFrameContext;   /* TRUE -> use SetFrameContext registers */

extern HANDLE hEventIoctl;

extern OSVERSIONINFO OsVersionInfo;

extern EXCEPTION_FILTER_DEFAULT EfdPreset;

// menu that belongs to hwndFrame
extern HMENU hmenuMain;
// Dynamically loaded menu for the command window
extern HMENU hmenuCmdWin;

