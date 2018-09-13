/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    data.c

Abstract:

    This file contains declarations for global data items

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

--*/

#include "precomp.h"
#pragma hdrstop


/****************************************************************************

    GLOBAL VARIABLES :

****************************************************************************/

// Version Info:
#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUGVER
DEBUG_VERSION('W','D',"WinDBG Shell, DEBUG")
#else
RELEASE_VERSION('W','D',"WinDBG Shell")
#endif

#ifdef __cplusplus
} // extern "C" {
#endif

// Handle to main window
HWND hwndFrame  = NULL;

// Handle to MDI client
HWND g_hwndMDIClient = NULL;

// Handle to currently activated child
HWND hwndActive = NULL;

// Handle to edit control
HWND hwndActiveEdit  = NULL;

// Handle to currently activated MDI debug window
HWND hwndDebug = NULL;

//Handle to instance data
HINSTANCE g_hInst;

//Generic words
WORD wGeneric1, wGeneric2;

//Help mode flag; TRUE = "ON"
BOOL bHelp = FALSE;

//Help file name
char szHelpFileName[_MAX_PATH];

//Handle to accelerator table
HACCEL hMainAccTable;
HACCEL hCurrAccTable;
HACCEL hCmdWinAccTable;

//Handle to .INI file
int iniHandle;

//Syntax Coloring
BOOL syntaxColors = TRUE;


// Commands to execute on certain events
char szStopEventCmd[MAX_CMDLINE_TXT] = "";

//Standard Dos File Extensions
char szStarDotC[_MAX_EXT];
char szStarDotH[_MAX_EXT];
char szStarDotCPP[_MAX_EXT];
char szStarDotCXX[_MAX_EXT];
char szStarDotExe[_MAX_EXT];
char szStarDotCom[_MAX_EXT];
char szStarDotStar[_MAX_EXT];

//Temporary string variable used by all modules
char szTmp[TMP_STRING_SIZE];

//Editor : Current view
int curView = -1;

//Array position in ppszMakefileExtTab of currently selected extension
WORD nCurExt;

// Handles to Include and Library Environment strings.
HANDLE hEnvIncludeStr = (HANDLE)NULL;
HANDLE hEnvLibraryStr = (HANDLE)NULL;

//Keyboard Hooks functions
HHOOK   hKeyHook;

//Find & Replace Data Structure
_FINDREPLACE FAR findReplace;
_FINDREPLACEMEM FAR frMem;

//Editor : Global documents struct
DOCREC Docs[MAX_DOCUMENTS];

//Editor : Global documents struct
VIEWREC Views[MAX_VIEWS];

#ifdef NEW_WINDOWING_CODE
// Structure containing all of the window handles
DEBUGGER_WINDOWS g_DebuggerWindows;
#endif

//Currently highlighted trace line (F8/F10)
TRACEINFO TraceInfo = {-1, 0};

//CrLf
char CrLf[3] = "\r\n";

//Number of different colors in a line
int nbColors;

//Screen colors saved
//Line len divided by 2 because different colors will be separated
//by spaces
COLORINFO colors[MAX_USER_LINE + 3]; //+ 3 =When selecting

//CheckSum in ini file
WORD checkSum;

//Record buffer overflow counter
WORD recordBufferOverflow;

//Current Help Id for Open, Merge, Save and Open project dialog box
WORD curHelpId;

//Empty string
char szNull[1] = "";

//State of keyboard when key is down
BOOL isShiftDown;
BOOL isCtrlDown;

//Record stop mark
int stopMarkStatus = HAS_STOP;

//For modeless dialog box control
BOOL exitModelessDlg;

//Editor : Standard editor win proc and subclassing functions
WNDPROC lpfnEditProc;

//Debugger window view #
int disasmView = -1;
int cmdView = -1;
int memView = -1;
int callsView = -1;

//Fonts variables
int __FAR   fontsNb = 0;
int __FAR   fontSizesNb = 0;
int __FAR   fontCur = 0;
int __FAR   fontSizeCur = 0;
LPLOGFONT   fonts = NULL; //Variable size
LPINT       fontSizes = NULL; //Variable size
LPINT       fontSizesPoint = NULL; //Variable size
//LOGFONT     defaultFont;
LOGFONT     g_logfontDefault;

//Last position of cursor before undo
int mC;
int dL;

// Number of dialog/message boxes currently open
int FAR BoxCount;

int iDebugLevel = 0;
EERADIX radix = 16;

//Option file full path name (qcwin.ini)
char FAR iniFileName[_MAX_PATH];

//Regular expression pattern (must be near)
struct patType *pat = NULL;

//TRUE after PostQuitMessage has been called.
BOOL QuitTheSystem = FALSE;

//True when terminate application has been called
BOOL FAR TerminatedApp = FALSE;

//Default ini mode
BOOL defaultIni = FALSE;

//Checking date of files for reload
BOOL checkFileDate = FALSE;

//Critical section for editor
BOOL editorIsCritical = FALSE;

//Title bar data
TITLEBAR TitleBar;

//Current line expanded (tabs converted)
char el[MAX_USER_LINE * 2 + 1];
int elLen;

//Current line and current block
LPLINEREC pcl;
LPBLOCKDEF pcb;

//View tabs
BOOL viewTabs = FALSE;

//Playing records mode
BOOL playingRecords = FALSE;

//Windows Version
int winVer;

//Scroll origin, KEYBOARD or MOUSE
WORD scrollOrigin;

//Autotest mode for test suites
AUTORUN AutoRun = arNone;
BOOL AutoTest = FALSE;
BOOL NoPopups = FALSE;
BOOL RemoteRunning = FALSE;
LPSTR PszAutoRun = NULL;

// Low memory messages
char LowMemoryMsg[MAX_MSG_TXT];
char LowMemoryMsg2[MAX_MSG_TXT];

// WinDBG title text
char MainTitleText[MAX_MSG_TXT];

char fCaseSensitive = 1;
char SuffixToAppend = '\0';

//
LPSTR   LpszCommandLine = iniFileName;

LPTD    LptdCur=NULL;   // Pointer to the current thread
LPPD    LppdCur=NULL;   // Pointer to the current process

HINSTANCE HModSH = 0;     // Symbol handler DLL handle
HINSTANCE HModEE = 0;     // Expression evaluator DLL
HINSTANCE HModTL = 0;     // Transport layer DLL handle
HINSTANCE HModEM = 0;     // Execution model DLL handler

BOOL    FSourceOverlay = TRUE;

//
// Used to determine if the Current or the most recently set
// Frame CONTEXT should be used for reading register variables.
//
BOOL    fUseFrameContext = FALSE;

//
// event to signal the completion of an ioctl
//
HANDLE hEventIoctl;

CRITICAL_SECTION     csLog;

OSVERSIONINFO OsVersionInfo;

EXCEPTION_FILTER_DEFAULT EfdPreset = (EXCEPTION_FILTER_DEFAULT) -1;


int REArg;
int RESize;
RE_OPCODE *REip;
struct patType *REPat;
char szOpenExeArgs[_MAX_PATH];

// menu that belongs to hwndFrame
HMENU hmenuMain;
// Dynamically loaded menu for the command window
HMENU hmenuCmdWin;



// Name of the commonly used Dlls, these are normally not changed.
const char * const g_pszDLL_EXPR_EVAL           = "eecxx.dll";
const char * const g_pszDLL_EXEC_MODEL          = "em.dll";
const char * const g_pszDLL_SYMBOL_HANDLER      = "shcv.dll";
