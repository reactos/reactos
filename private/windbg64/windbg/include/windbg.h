/*++ BUILD Version: 0004    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Windbg.h

Abstract:

    Main header file for the Windbg debugger.

Author:

    David J. Gilman (davegi) 21-Apr-1992

Environment:

    Win32, User Mode

--*/

#if ! defined( _WINDBG_ )
#define _WINDBG_

#include <windef.h>

#ifndef RC_INVOKED
#include "od.h"
#endif

#define NOGDICAPMASKS
#define NOMETAFILE
#define NOSOUND
#define NOCOMM
#define NOKANJI
#define NOMINMAX

#define EXPORT

#define __FAR
#define __FASTCALL
#define _HUGE_

typedef unsigned short _segment;

#define GetWindowHandle(hWnd, nIndex) (HWND) GetWindowLongPtr(hWnd, nIndex)
#define GetClassHandle(hWnd, nIndex) (HWND) GetClassLongPtr(hWnd, nIndex)
#define SetWindowHandle(hWnd, nIndex, h) SetWindowLongPtr(hWnd, nIndex, (LONG_PTR)h)
#define MoveToX(a, b, c, d) MoveToEx(a, b, c, d)
#define SetBrushOrgX(a, b, c, d) SetBrushOrgEx(a, b, c, d)

typedef short * LPSHORT;

#define Unused(a) (void) a;

//Compilation options
#define DEBUGGING           // Shields all system with assertions
#undef  BRK_IF_ERROR        // Issue a BreakPoint after an ErrorBox
#undef  BRK_IF_FATAL_ERROR  // Issue a BreakPoint after a FatalErrorBox

/****************************************************************************

    GLOBAL LIMITS CONSTANTS:

****************************************************************************/
#define MAX_MSG_TXT         4096    //Max text width in message boxes
#define MAX_CMDLINE_TXT     8192    //Max size for command line
#define MAX_VAR_MSG_TXT     8192    //Max size of a message built at run-time
#define MAX_DOCUMENTS       64      //Max number of documents
#define MAX_VIEWS           64      //Max number of views
#ifndef NTBUG
#define DISK_BLOCK_SIZE     2048
#else
#define DISK_BLOCK_SIZE     4096    //Cluster size for disks I/O on text files
#endif
#define MAX_PICK_LIST       16      //Max of 'old' strings kept in replace and find
#define MAXFILENAMELEN      260     //Max # of chars in a filename
#define MAXFILENAMEPREFIX   256     //Prefix part of a filename
#define MAX_CLIPBOARD_SIZE  65536*8 //Clipboard limit (512k)
#define DEFAULT_TAB         8       //Default tabulation size
#define MAX_TAB_WIDTH       99      //Maximum tab size
#define TMP_STRING_SIZE     8192    //All purpose strings
#define MAX_EXPRESS_SIZE    1024    //Max text size of an expression (watch etc)
#define UNDOREDO_DEF_SIZE   4096L   //Size of undo/redo buffers
#define UNDOREDO_MAX_SIZE   65535L  //Maximum size of undo/redo buffers
#define BHD                 10      //Size of block header (see BLOCKDEF)
#define BLOCK_SIZE          (8192 - BHD) //Size of block
#define LHD                 4       //Size of line Header (see LINEREC)
#define MAX_LINE_SIZE       256     //Max inside length of editor line
#define MAX_USER_LINE       (MAX_LINE_SIZE - LHD - 1) //Max length of user line
#define MAX_LINE_NUMBER     65535   //Maximum line #
#define MAX_CHARS_IN_FONT   256     //Do you really need a comment ??
#define MEMBUF_SIZE         4096    //for memory operations: fi, c, m, s
#define IDM_FIRSTCHILD      30000   //for mdi default menu handling


/****************************************************************************

    GLOBAL TYPES AND DEFINES:

****************************************************************************/





//Undo/Redo : Type of action for record
#define NEXT_HAS_NO_STOP    2
#define HAS_NO_STOP         1
#define HAS_STOP            0

//Undo/Redo : Type of Action
#define INSERTSTREAM        0   // |
#define INSERTCHAR          1   // |
#define DELETESTREAM        2   // | bits 0,2
#define DELETECHAR          3   // |
#define REPLACECHAR         4   // |
#define STOPMARK            16  //   bit 4
#define USERMARK            32  //   bit 5
#define CANCELREC           64  //    bit 6 CANCELREC should never be on bit 0
#define ACTIONMASK          7   //To retrieve action
#define REPLACEDBCS         0x08

//Undo/Redo : To store Deletsream's col2 and line2
typedef struct {
    int line;
    BYTE col;
} U_COORD;

//Undo/Redo : To store InsertStream's len and chars
typedef struct {
    WORD len; //Actual len (full len)
    char chars[];
} STREAM;

//Undo/redo : Variant record
typedef union {
    U_COORD c;
    char ch;
    STREAM s;
} X;

//Undo/Redo : Structure of record definition
typedef struct {
    WORD prevLen; //MUST BE FIRST FIELD !(Previous rec length (full len))
    BYTE action; //Type of logical editing action
    BYTE col;
    int line;
    X x; //Variant part
} STREAMREC;
typedef STREAMREC *LPSTREAMREC;

//Undo/Redo : Size of variant components
#define HDR_INSERTSTREAM_SIZE (sizeof(STREAMREC) - sizeof(X) + sizeof(STREAM))
#define HDR_DELETESTREAM_SIZE (sizeof(STREAMREC) - sizeof(X) + sizeof(U_COORD))
#define HDR_DELETECHAR_SIZE (sizeof(STREAMREC) - sizeof(X))
#define HDR_INSERTCHAR_SIZE (sizeof(STREAMREC) - sizeof(X) + sizeof(char))

//Undo/Redo : States of engine
// - REC_STOPPED no more recording/playing
// - REC_HADOVERFLOW when buffer went full during an operation
// - REC_UNDO normal mode where inverse of edit action goes in Undo buffer
// - REC_REDO during undo, inverse of Undo edit actions will be saved in
//   Redo buffer
// - REC_CANNOTUNDO when undo buffer is empty
#define REC_STOPPED         0
#define REC_HADOVERFLOW     1
#define REC_UNDO            2
#define REC_REDO            3
#define REC_CANNOTUNDO      -32767

//Undo/redo : Information in a document
typedef struct {
    HANDLE h;           //Handle to editor undos/redos
    long bufferSize;    //In bytes
    long offset;        //Current undo/redo rec in buffer
    LPSTREAMREC pRec;   //Pointer to current undo/redo recorded
} UNDOREDOREC;
typedef UNDOREDOREC *LPUNDOREDOREC;

/*
**  Messages : User messages (See DEBUGGER.H for the other)
*/


#define WU_UPDATE               (WM_USER + 0)
#define WU_CLEAR                (WM_USER + 1)
#define WU_RELOADFILE           (WM_USER + 2)
#define WU_SETWATCH             (WM_USER + 3)
#define WU_EXPANDWATCH          (WM_USER + 4)
#define WU_INFO                 (WM_USER + 5)
#define WU_INITDEBUGWIN         (WM_USER + 7)
#define WU_SYNCALL              (WM_USER + 8)
#define WU_DBG_LOADEM           (WM_USER + 10)
#define WU_DBG_UNLOADEM         (WM_USER + 11)
#define WU_AUTORUN              (WM_USER + 12)
#define WU_DBG_LOADEE           (WM_USER + 13)
#define WU_DBG_UNLOADEE         (WM_USER + 14)
#define WU_CLR_FORECHANGE       (WM_USER + 15) //next two for windows that need to
#define WU_CLR_BACKCHANGE       (WM_USER + 16) //know about color changes
#define WU_OPTIONS              (WM_USER + 17)
#define WU_RESAVEFILE           (WM_USER + 18)
#define WU_INVALIDATE           (WM_USER + 19)
#define WU_LOG_REMOTE_MSG           (WM_USER + 20)
#define WU_LOG_REMOTE_CMD       (WM_USER + 21)



// States for autorun engine
typedef enum _AUTORUN {
    arNone = 0,
    arCmdline,
    arSource,
    arQuit
} AUTORUN;



//Status Bar : Display Text type
#define STATUS_INFOTEXT     0
#define STATUS_MENUTEXT     1


//Status Bar : Pens and Brushes colors
#define GRAYDARK                0x00808080



//Workspace : Basic window information
typedef struct {
    RECT coord;
    long style;
} WININFO;
typedef WININFO *LPWININFO;

//Editor & Project: Type of file kept
#define     EDITOR_FILE     0
#define     PROJECT_FILE    1

//Editor : Horizontal scroll ratio (1/5 of the window)
#define SCROLL_RATIO        5

//Editor : Ascii ctrl chars
#define CTRL_A                          1
#define CTRL_C                          3
#define CTRL_D                          4
#define CTRL_E                          5
#define CTRL_F                          6
#define CTRL_G                          7
#define CTRL_H                          8
#define CTRL_J                          10
#define CTRL_M                          13
#define CTRL_N                          14
//#define CTRL_Q                          17
#define CTRL_R                          18
#define CTRL_S                          19
#define CTRL_T                          20
#define CTRL_U                          21
#define CTRL_V                          22
#define CTRL_W                          23
#define CTRL_X                          24
#define CTRL_Y                          25
#define CTRL_Z                          26
#define CTRL_RIGHTBRACKET               29

//Editor : Escape
#define ESCAPE                          27

//Editor : Scan codes needed
#define RETURN_SCANCODE                 28
#define BACKSPACE_SCANCODE              14

//Editor : Code for No View
#define NOVIEW                          255

//Editor : Standard chars in text files
#define LF                              10
#define CR                              13
#define TAB                             9
#define BS                              8

//Editor : Status of a line

#define COMMENT_LINE            0x1  //This line is fully commented
#define MULTISTRING_LINE        0x2  //This line is a multiline string
#define TAGGED_LINE             0x4  //Tagged by the user
#define BRKPOINT_LINE           0x8  //Brk Point Commited
#define CURRENT_LINE            0x10 //Current line when debugging
#define UBP_LINE                0x20 //Uninstaiated BP
#define DASM_SOURCE_LINE        0x40 //Source line in disasm window
#define DASM_LABEL_LINE         0x80 //Label line in disasm window

//Editor : State when reading a file
#define END_OF_LINE         0
#define END_OF_FILE         1
#define END_ABORT           2

//Editor : Last line convention
#define LAST_LINE                       MAX_LINE_NUMBER + 1

//Color array for screen paint
typedef struct {
    int  nbChars;
    COLORREF fore;
    COLORREF back;
} COLORINFO;

//Editor : Line status action
typedef enum
{
    LINESTATUS_ON,
    LINESTATUS_OFF,
    LINESTATUS_TOGGLE
}
LINESTATUSACTION;

//Editor : Line definition
typedef struct {
    WORD Status;        //Status of line            |
    BYTE PrevLength;    //Length of previous line   | !! Size in constant LHD
    BYTE Length;        //Length of next line       |
    char Text[1];       //Text of line
} LINEREC;
typedef LINEREC UNALIGNED*  LPLINEREC;

#ifdef ALIGN
#pragma pack(4)
#endif

//Editor : Block definition
typedef struct tagBD {
    struct tagBD *PrevBlock;    //Previous block or NULL |
    struct tagBD *NextBlock;    //Next block or NULL     | !! Size in constant BHD
    int LastLineOffset;             //Size used              |
#ifdef ALIGN
    int  dummy;
#endif
    char Data[BLOCK_SIZE];
} BLOCKDEF;
typedef BLOCKDEF * LPBLOCKDEF;

#ifdef ALIGN
#pragma pack()
#endif


#ifdef NEW_WINDOWING_CODE
typedef enum {
    MINVAL_WINDOW = 0,
    DOC_WINDOW,
    WATCH_WINDOW,
    LOCALS_WINDOW,
    CPU_WINDOW,
    DISASM_WINDOW,
    CMD_WINDOW,
    FLOAT_WINDOW,
    MEMORY_WINDOW,
    QUICKW_WINDOW,
    CALLS_WINDOW,
    MAXVAL_WINDOW
} WIN_TYPES, * PWIN_TYPES;


typedef struct _DEBUGGER_WINDOWS {
    HWND    hwndWatch;
    HWND    hwndLocals;
    HWND    hwndCpu;
    HWND    hwndDisasm;
    HWND    hwndCmd;
    HWND    hwndFloat;
    HWND    hwndMemory;
    HWND    hwndQuickW;
    HWND    hwndCalls;
} DEBUGGER_WINDOWS, *PDEBUGGER_WINDOWS;

#endif //NEW_WINDOWING_CODE



//Editor : View definition
typedef struct _VIEWREC {
    int NextView;                       //Index of next view or (-1)
    int Doc;                            //Index of document
    HWND hwndFrame;                     //View Frame Window informations
    HWND hwndClient;                    //Client handle
    RECT rFrame;
    long X;                              //Cursor position X
    long Y;                              //Cursor position Y
    BOOL BlockStatus;                   //Selection On/Off
    long BlockXL;                        //Selection X left
    long BlockXR;                        //Selection Y left
    long BlockYL;                        //Selection X right
    long BlockYR;                        //Selection Y right
    BOOL hScrollBar;                    //Is there an horizontal scrollbar
    BOOL vScrollBar;                    //Is there a vertical scrollbar
    int scrollFactor;                   //0-100  Percent of scroll for view
    HFONT font;                         //Current font
    int charWidth[MAX_CHARS_IN_FONT];
    int charHeight;                     //Current char height
    int maxCharWidth;
    int aveCharWidth;
    int charSet;
    long iYTop;                          /* Current Top line Y iff != -1 */
    int Tmoverhang;

#define VIEW_PITCH_VARIABLE     0
#define VIEW_PITCH_DBCS_FIXED   1
#define VIEW_PITCH_ALL_FIXED    2
    WORD wViewPitch;
    WORD wCharSet;                      //This is for 'Memory Window'.
#define DBCS_CHAR           "\x82\xa0"
    int charWidthDBCS;                  //Assume all DBCS char have the same
    BOOL bDBCSOverWrite;                //If TRUE, DBCS char is treated as
                                        // two SBCS chars when "OverWrite" mode
                                        //This always TRUE now.
} VIEWREC;
typedef VIEWREC *LPVIEWREC;

//Document : Type of document (if you change those values, don't be
//surprise if windows coloring does not work anymore)
#define DOC_WIN             0
#define DUMMY_WIN           1
#define WATCH_WIN           2 // this MUST be here...Our magic # is 1 (-1) !!!!!
#define LOCALS_WIN          3
#define CPU_WIN             4
#define DISASM_WIN          5
#define COMMAND_WIN         6
#define FLOAT_WIN           7
#define MEMORY_WIN          8
#define QUICKW_WIN          9
#define CALLS_WIN          10

//Document & Environment : Language kind
#define NO_LANGUAGE         0
#define C_LANGUAGE          1
#define PASCAL_LANGUAGE     2
#define AUTO_LANGUAGE       3

//Document : Mode when opening files
#define MODE_OPENCREATE     0
#define MODE_CREATE         1
#define MODE_OPEN           2
#define MODE_DUPLICATE      3
#define MODE_RELOAD         4

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

//Document : Document definition
typedef struct {
    WORD            docType;                // Type of document
    char            szFileName[_MAX_PATH];    // File name
    FILETIME        time;                   // Time opened, saved
    BOOL            bChangeFileAsk;         // is there a dialog up ?
    long            NbLines;                // Current number of lines
    int             FirstView;              // Index of first view in Views
    LPBLOCKDEF      FirstBlock;         // Address of first block
    LPBLOCKDEF      LastBlock;          // Address of last block
    LPBLOCKDEF      CurrentBlock;       // Address of current block
    int             CurrentLine;            // Current line number
    int             CurrentLineOffset;      // Current line offset in block
    int             lineTop;                // Top line affected by a change
    int             lineBottom;             // Buttom line affected by a change
    WORD            language;               // C, Pascal or no language document
    BOOL            untitled;
    BOOL            ismodified;
                    
    BOOL            readOnly;               // Old "entire" doc readonly flag
                    
    BOOL            RORegionSet;            // Do we have a valid RO region?
    int             RoX2;
    int             RoY2;                   // Region max's-X1,Y1 always 0,0
                    
    BOOL            forcedOvertype;
    BOOL            forcedReadOnly;
    int             playCount;              // 0 in normal edit mode,
                                            // counts undos otherwise
    WORD            recType;
    UNDOREDOREC     undo;
    UNDOREDOREC     redo;
} DOCREC;
typedef DOCREC *LPDOCREC;

//Find/Replace : Type of pick list
#define FIND_PICK           0
#define REPLACE_PICK        1

//Find/Replace : Structure Definition
typedef struct {
    BOOL matchCase;                                     //Match Upper/Lower case
    BOOL regExpr;                                       //Regular expression
    BOOL wholeWord;
    BOOL goUp;                                          //Search direction
    char findWhat[MAX_USER_LINE + 1];                   //Input string
    char replaceWith[MAX_USER_LINE + 1];                //Output string
    int nbInPick[REPLACE_PICK + 1];                     //Number of strings
                                                        //in picklist
    HANDLE hPickList[REPLACE_PICK + 1][MAX_PICK_LIST];  //PickList for old
    int nbReplaced;                                     //Actual number of
                                                        //replacements
} _FINDREPLACE;

typedef struct {
    int leftCol;                //Start of string in line
    int rightCol;               //End of string in line
    int line;                   //Current line
    int stopLine;               //Where find/replace should end
    int stopCol;                //Where find/replace should end
    int nbReplaced;             //Number of occurences replaced
    BOOL oneLineDone;
    BOOL allFileDone;
    BOOL hadError;
    BOOL goUpCopy;
    BOOL allTagged;
    BOOL replacing;
    BOOL replaceAll;
    BOOL exitModelessFind;
    BOOL exitModelessReplace;
    HWND hDlgFindNextWnd;
    HWND hDlgConfirmWnd;
    DLGPROC lpFindNextProc;
    DLGPROC lpConfirmProc;
    BOOL firstFindNextInvoc;
    BOOL firstConfirmInvoc;
} _FINDREPLACEMEM;

//Error window : The error node list:
#define MAX_ERROR_TEXT      256

//Error window : Error node
typedef struct ERRORNODEtag
{
    char Text[MAX_ERROR_TEXT];
    int ErrorLine;// Line number of error in source file, -1 otherwise
    WORD flags;
    struct ERRORNODEtag *Next;
    struct ERRORNODEtag *Prev;
} ERRORNODE;
typedef ERRORNODE *PERRORNODE;


//Debugger : Debugging Mode
#define SOFT_DEBUG          0
#define HARD_DEBUG          1

//Debugger : Breakpoint buffer sizes
#define BKPT_LOCATION_SIZE 128
#define BKPT_WNDPROC_SIZE   128
#define BKPT_EXPR_SIZE      128

//Degugger : Breakpoints types
typedef enum
{
    BRK_AT_LOC,
    BRK_AT_LOC_EXPR_TRUE,
    BRK_AT_LOC_EXPR_CHGD,
    BRK_EXPR_TRUE,
    BRK_EXPR_CHGD,
    BRK_AT_WNDPROC,
    BRK_AT_WNDPROC_EXPR_TRUE,
    BRK_AT_WNDPROC_EXPR_CHGD,
    BRK_AT_WNDPROC_MSG_RECVD
} BREAKPOINTACTIONS;


//Debugger : Message classes
#define SPECIFICMESSAGE         0x0001
#define MOUSECLASS              0x0002
#define WINDOWCLASS             0x0004
#define INPUTCLASS              0x0008
#define SYSTEMCLASS             0x0010
#define INITCLASS               0x0020
#define CLIPBOARDCLASS          0x0040
#define DDECLASS                0x0080
#define NONCLIENTCLASS          0x0100
#define NOTSELECTED             0x0200
#define NOTSELECTEDMESSAGE      0xFFFF

//Debugger : Set Breakpoint structure definition
typedef struct {
    BREAKPOINTACTIONS nAction;
    char szLocation[BKPT_LOCATION_SIZE];
    char szWndProc[BKPT_WNDPROC_SIZE];
    char szExpression[BKPT_EXPR_SIZE];
    WORD wLength;
    WORD MessageClass;
    WORD Message;
} BRKPTSTRUC;

//Debugger : Current line in debuggee
typedef struct {
    int doc;
    int CurTraceLine;
} TRACEINFO;

//Title bar
typedef enum {TBM_UNKNOWN = -1, TBM_WORK, TBM_RUN, TBM_BREAK} TITLEBARMODE;
typedef struct _TITLEBAR {
    char ProgName[30];
    char UserTitle[30];
    char ModeWork[20];
    char ModeRun[20];
    char ModeBreak[20];
    TITLEBARMODE Mode;
    TITLEBARMODE TimerMode;
} TITLEBAR;

//Hardwired KD stuff
#define NT_ALT_KERNEL_NAME  "ntoskrnl"
#define NT_ALT_KRNLMP_NAME  "ntkrnlmp"
#define NT_KERNEL_NAME      "ntoskrnl.exe"
#define NT_KRNLMP_NAME      "ntkrnlmp.exe"
#define NT_USERDUMP_NAME    "UserDump"
#define NT_KERNELDUMP_NAME  "KernelDump"


/****************************************************************************

    HOTKEY DEFINES:

****************************************************************************/

#define IDH_CTRLC               100

/****************************************************************************

    RESOURCES DEFINES :

****************************************************************************/

//Edit control identifier
#define ID_EDIT 0xCAC

//Position of window menu
#define WINDOWMENU              4

//Position of file menu
#define FILEMENU                0

//Standard help id in dialogs
#define IDWINDBGHELP            100

//Toolbar control identifier
#define ID_TOOLBAR               100

//Window word values for child windows
#define GWW_EDIT                0
#define GWW_VIEW                (GWW_EDIT + sizeof(PVOID))

//Size of extra data for MDI child windows
#define CBWNDEXTRA              (2 + sizeof(PVOID))

/*
**  Include the defines which are used have numbers for string
**      resources.
*/

#include "..\include\res_str.h"


// Path of the last Src file opened from a file open dlg box
extern char g_szMRU_SRC_FILE_PATH[_MAX_PATH];

/****************************************************************************

    CALL BACKS:

****************************************************************************/
BOOL InitApplication(HINSTANCE);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MDIChildWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MDIPaneWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DLGPaneWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ChildWndProc(HWND, UINT, WPARAM, LPARAM);


//Call Back to Handle Edit Find Dialog BOX
INT_PTR CALLBACK DlgFind(HWND, UINT, WPARAM, LPARAM);

//Call Back to Handle Edit Replace Dialog BOX
INT_PTR CALLBACK DlgReplace(HWND, UINT, WPARAM, LPARAM);

//Call Back to Handle View Line Dialog BOX
INT_PTR CALLBACK DlgLine(HWND, UINT, WPARAM, LPARAM);

//Call Back to Handle View Function Dialog BOX
INT_PTR CALLBACK DlgFunction(HWND, UINT, WPARAM, LPARAM);

//Call Back to Handle View Function Dialog BOX
INT_PTR CALLBACK DlgTaskList(HWND, UINT, WPARAM, LPARAM);

//Call Back to Handle Debug QuickWatch Dialog BOX
INT_PTR CALLBACK DlgQuickW(HWND, UINT, WPARAM, LPARAM);

// Callback to handle memory window dialog box
INT_PTR CALLBACK DlgMemory(HWND, UINT, WPARAM, LPARAM);

// Callback to handle pane manager options box
INT_PTR CALLBACK DlgPaneOptions(HWND, UINT, WPARAM, LPARAM);

//Call Back to Handle Debug Set Break Point Message Dialog BOX
INT_PTR CALLBACK DlgMessage(HWND, UINT, WPARAM, LPARAM);

//Call Back to Handle Debug Set Breakpoint Dialog BOX
INT_PTR CALLBACK DlgSetBreak(HWND, UINT, WPARAM, LPARAM);

//Call Back to Debugger exceptions Options Dialog BOX
INT_PTR CALLBACK DlgDbugexcept(HWND, UINT, WPARAM, LPARAM);





// call back to handle user control buttons
LRESULT CALLBACK QCQPCtrlWndProc (HWND, UINT, WPARAM, LPARAM) ;

//Call Back to Handle File Open Merge Save and Open Project
UINT_PTR APIENTRY DlgFile(HWND, UINT, WPARAM, LPARAM);
UINT_PTR APIENTRY GetOpenFileNameHookProc(HWND, UINT, WPARAM, LPARAM);

//Call Back to Handle Project Edit Program List Dialog BOX
INT_PTR CALLBACK DlgEditProject(HWND, UINT, WPARAM, LPARAM);

//Call Back to Handle Edit Confirm Replace Dialog BOX
INT_PTR CALLBACK DlgConfirm(HWND, UINT, WPARAM, LPARAM);

//Call Back to Handle Edit Find Next Dialog BOX
INT_PTR CALLBACK DlgFindNext(HWND, UINT, WPARAM, LPARAM);

// Callback Function for Thread dialog
INT_PTR CALLBACK DlgThread(HWND, UINT, WPARAM, LPARAM);

// Callback Function for Process dialog
INT_PTR CALLBACK DlgProcess(HWND, UINT, WPARAM, LPARAM);

// Callback Function for BrowseDlg dialog
INT_PTR CALLBACK DlgAskBrowse(HWND, UINT, WPARAM, LPARAM);

// Callback Function for BrowseDlg dialog
INT_PTR CALLBACK DlgProcBadSymbols(HWND, UINT, WPARAM, LPARAM);

// Callback Function for BrowseDlg dialog
INT_PTR CALLBACK DlgProc_Adv_BadSymbols(HWND, UINT, WPARAM, LPARAM);





/*
**  Describe the set of possible thread and process states.
*/

typedef enum {
    tsInvalidState = -1,
    tsPreRunning,
    tsRunning,
    tsStopped,
    tsException1,
    tsException2,
    tsRipped,
    tsExited
} TSTATEX;

typedef enum {
    psInvalidState = -1,
    psNoProgLoaded,
    psPreRunning,   // haven't hit ldr BP
    psRunning,
    psStopped,
    psExited,
    psDestroyed,    // only used for ipid == 0
    psError         // damaged
} PSTATEX;


//
// See od.h for EXCEPTION_FILTER_DEFAULT type
//
//
typedef struct _excpt_node {
    struct _excpt_node *next;
    DWORD               dwExceptionCode;
    EXCEPTION_FILTER_DEFAULT efd;
    LPSTR               lpName;
    LPSTR               lpCmd;
    LPSTR               lpCmd2;
} EXCEPTION_LIST;


#define tfStepOver 2
/*
**  Create structures which will describe the set of processes and
**  threads in the system.  These are the Thread Descriptor and
**  Process Descriptor structures.
*/

typedef struct TD * LPTD;
typedef struct PD * LPPD;

typedef struct TD {
    HTID    htid;           // HTID for this thread
    LPPD    lppd;           // Pointer to Process descriptor for thread
    LPTD    lptdNext;       // Pointer to sibling threads
    UINT    itid;           // Index for this thread
    UINT    cStepsLeft;     // Number of steps left to run
    LPSTR   lpszCmdBuffer;  // Pointer to buffer containning command to execute
    LPSTR   lpszCmdPtr;     // Pointer to next command to be executed.
    UINT    flags;          // Flags on thread, used by step
    TSTATEX tstate;         // Thread state - enumeration
    UINT    fInFuncEval:1;  // Currently doing function evaluation?
    UINT    fFrozen:1;      // Frozen?
    UINT    fGoOnTerm:1;    // never stop on termination
    UINT    fRegisters:1;   // print registers on stop event?
    UINT    fDisasm:1;      // print disasm on stop event?
    DWORD64 TebBaseAddress; //
    LPSTR   lpszTname;      //
} TD;

typedef struct PD {
    HPID    hpid;           // This is the HPID for this process
    MPT     mptProcessorType;
    LPPD    lppdNext;       // Pointer to next LPPD on list
    LPTD    lptdList;       // Pointer to list of threads for process
    HPDS    hpds;           // Handle to SH process description structure
    UINT    ctid;           // Counter for tids
    UINT    ipid;           // Index for this process
    EXCEPTION_LIST *exceptionList;
    LPSTR   lpBaseExeName;  // Name of exe that started process
    PSTATEX pstate;         // Process state - enumeration
    HANDLE  hbptSaved;      // BP for deferred use
    UINT    fFrozen:1;      // Frozen?
    UINT    fPrecious:1;    // Don't delete this PD (only for ipid == 0?)
    UINT    fChild:1;       // go or don't on ldr BP
    UINT    fHasRun:1;      // has run after ldr BP
    UINT    fStopAtEntry:1; // Stop at app entry point?
    UINT    fInitialStep:1; // doing src step before entrypoint event
    RTL_USER_PROCESS_PARAMETERS64 ProcessParameters;
} PD;


#define IDS_SOURCE_WINDOW           100
#define IDS_DUMMY_WINDOW            101
#define IDS_WATCH_WINDOW            102
#define IDS_LOCALS_WINDOW           103
#define IDS_CPU_WINDOW              104
#define IDS_DISASSEMBLER_WINDOW     105
#define IDS_COMMAND_WINDOW          106
#define IDS_FLOAT_WINDOW            107
#define IDS_MEMORY_WINDOW           108
#define IDS_BREAKPOINT_LINE         109
#define IDS_CURRENT_LINE            110
#define IDS_CURRENTBREAK_LINE       111
#define IDS_UNINSTANTIATEDBREAK     112
#define IDS_TAGGED_LINE             113
#define IDS_TEXT_SELECTION          114
#define IDS_KEYWORD                 115
#define IDS_IDENTIFIER              116
#define IDS_COMMENT                 117
#define IDS_NUMBER                  118
#define IDS_REAL                    119
#define IDS_STRING                  120
#define IDS_ACTIVEEDIT              121
#define IDS_CHANGEHISTORY           122
#define IDS_CALLS_WINDOW            123

#define IDS_SELECT_ALL              124
#define IDS_CLEAR_ALL               125


// Name of the commonly used Dlls, these are normally not changed.
extern const char * const g_pszDLL_EXPR_EVAL;
extern const char * const g_pszDLL_EXEC_MODEL;
extern const char * const g_pszDLL_SYMBOL_HANDLER;


#endif // _WINDBG_
