/*****************************************************************************\
*                                                                             *
* toolhelp.h -  toolhelp.dll functions, types, and definitions                *
*                                                                             *
* Version 1.0								      *
*                                                                             *
* NOTE: windows.h must be #included first				      *
*                                                                             *
* Copyright (c) 1992-1994, Microsoft Corp.	All rights reserved.	      *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_TOOLHELP
#define _INC_TOOLHELP

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#ifndef _INC_WINDOWS    /* If included with 3.0 headers... */
#define LPCSTR      LPSTR
#define WINAPI      FAR PASCAL
#define CALLBACK    FAR PASCAL
#define UINT        WORD
#define HMODULE     HANDLE
#define HINSTANCE   HANDLE
#define HLOCAL      HANDLE
#define HGLOBAL     HANDLE
#define HTASK       HANDLE
#endif  /* _INC_WINDOWS */

/****** General symbols ******************************************************/
#define MAX_DATA        11

#define MAX_PATH16      255

#define MAX_MODULE_NAME 8 + 1
#define MAX_CLASSNAME   255

/****** Global heap walking ***************************************************/
typedef struct tagGLOBALINFO
{
    DWORD dwSize;
    WORD wcItems;
    WORD wcItemsFree;
    WORD wcItemsLRU;
} GLOBALINFO;

typedef struct tagGLOBALENTRY
{
    DWORD dwSize;
    DWORD dwAddress;
    DWORD dwBlockSize;
    HGLOBAL hBlock;
    WORD wcLock;
    WORD wcPageLock;
    WORD wFlags;
    BOOL wHeapPresent;
    HGLOBAL hOwner;
    WORD wType;
    WORD wData;
    DWORD dwNext;
    DWORD dwNextAlt;
} GLOBALENTRY;

/* GlobalFirst()/GlobalNext() flags */
#define GLOBAL_ALL      0
#define GLOBAL_LRU      1
#define GLOBAL_FREE     2

/* GLOBALENTRY.wType entries */
#define GT_UNKNOWN      0
#define GT_DGROUP       1
#define GT_DATA         2
#define GT_CODE         3
#define GT_TASK         4
#define GT_RESOURCE     5
#define GT_MODULE       6
#define GT_FREE         7
#define GT_INTERNAL     8
#define GT_SENTINEL     9
#define GT_BURGERMASTER 10

/* If GLOBALENTRY.wType==GT_RESOURCE, the following is GLOBALENTRY.wData: */
#define GD_USERDEFINED      0
#define GD_CURSORCOMPONENT  1
#define GD_BITMAP           2
#define GD_ICONCOMPONENT    3
#define GD_MENU             4
#define GD_DIALOG           5
#define GD_STRING           6
#define GD_FONTDIR          7
#define GD_FONT             8
#define GD_ACCELERATORS     9
#define GD_RCDATA           10
#define GD_ERRTABLE         11
#define GD_CURSOR           12
#define GD_ICON             14
#define GD_NAMETABLE        15
#define GD_MAX_RESOURCE     15

/* GLOBALENTRY.wFlags */
#define GF_PDB_OWNER        0x0100      /* Low byte is KERNEL flags */

BOOL    WINAPI GlobalInfo(GLOBALINFO FAR* lpGlobalInfo);
BOOL    WINAPI GlobalFirst(GLOBALENTRY FAR* lpGlobal, WORD wFlags);
BOOL    WINAPI GlobalNext(GLOBALENTRY FAR* lpGlobal, WORD wFlags);
BOOL    WINAPI GlobalEntryHandle(GLOBALENTRY FAR* lpGlobal, HGLOBAL hItem);
BOOL    WINAPI GlobalEntryModule(GLOBALENTRY FAR* lpGlobal, HMODULE hModule, WORD wSeg);
WORD    WINAPI GlobalHandleToSel(HGLOBAL hMem);

/****** Local heap walking ***************************************************/

typedef struct tagLOCALINFO
{
    DWORD dwSize;
    WORD wcItems;
} LOCALINFO;

typedef struct tagLOCALENTRY
{
    DWORD dwSize;
    HLOCAL hHandle;
    WORD wAddress;
    WORD wSize;
    WORD wFlags;
    WORD wcLock;
    WORD wType;
    WORD hHeap;
    WORD wHeapType;
    WORD wNext;
} LOCALENTRY;

/* LOCALENTRY.wHeapType flags */
#define NORMAL_HEAP     0
#define USER_HEAP       1
#define GDI_HEAP        2

/* LOCALENTRY.wFlags */
#define LF_FIXED        1
#define LF_FREE         2
#define LF_MOVEABLE     4

/* LOCALENTRY.wType */
#define LT_NORMAL                   0
#define LT_FREE                     0xff
#define LT_GDI_PEN                  1   /* LT_GDI_* is for GDI's heap */
#define LT_GDI_BRUSH                2
#define LT_GDI_FONT                 3
#define LT_GDI_PALETTE              4
#define LT_GDI_BITMAP               5
#define LT_GDI_RGN                  6
#define LT_GDI_DC                   7
#define LT_GDI_DISABLED_DC          8
#define LT_GDI_METADC               9
#define LT_GDI_METAFILE             10
#define LT_GDI_MAX                  LT_GDI_METAFILE
#define LT_USER_CLASS               1   /* LT_USER_* is for USER's heap */
#define LT_USER_WND                 2
#define LT_USER_STRING              3
#define LT_USER_MENU                4
#define LT_USER_CLIP                5
#define LT_USER_CBOX                6
#define LT_USER_PALETTE             7
#define LT_USER_ED                  8
#define LT_USER_BWL                 9
#define LT_USER_OWNERDRAW           10
#define LT_USER_SPB                 11
#define LT_USER_CHECKPOINT          12
#define LT_USER_DCE                 13
#define LT_USER_MWP                 14
#define LT_USER_PROP                15
#define LT_USER_LBIV                16
#define LT_USER_MISC                17
#define LT_USER_ATOMS               18
#define LT_USER_LOCKINPUTSTATE      19
#define LT_USER_HOOKLIST            20
#define LT_USER_USERSEEUSERDOALLOC  21
#define LT_USER_HOTKEYLIST          22
#define LT_USER_POPUPMENU           23
#define LT_USER_ICON                24
#define LT_USER_QMSG                26
#define LT_USER_VWININFO            27
#define LT_USER_SMS                 28
#define LT_USER_PROCESSINFO         29
#define LT_USER_ATTACHINFO          30
#define LT_USER_SYSCOLORTEMP        31
#define LT_USER_HANDLETABLE         32
#define LT_USER_SWITCHWNDINFO       33
#define LT_USER_HOOKMSG             34
#define LT_USER_MAX                 34

BOOL    WINAPI LocalInfo(LOCALINFO FAR* lpLocal, HGLOBAL hHeap);
BOOL    WINAPI LocalFirst(LOCALENTRY FAR* lpLocal, HGLOBAL hHeap);
BOOL    WINAPI LocalNext(LOCALENTRY FAR* lpLocal);

/****** Local 32-bit heap walking ********************************************/

typedef HANDLE HLOCAL32;

typedef struct tagLOCAL32INFO
{
    DWORD dwSize;
    DWORD dwMemReserved;
    DWORD dwMemCommitted;
    DWORD dwTotalFree;
    DWORD dwLargestFreeBlock;
    DWORD dwcFreeHandles;
} LOCAL32INFO;

typedef struct tagLOCAL32ENTRY
{
    DWORD dwSize;
    HLOCAL32 hHandle;
    DWORD dwAddress;
    DWORD dwSizeBlock;
    WORD wFlags;
    WORD wType;
    WORD hHeap;
    WORD wHeapType;
    DWORD dwNext;
    DWORD dwNextAlt;
} LOCAL32ENTRY;

/* LOCAL32ENTRY.wHeapType flags same as LOCALENTRY.wHeapType flags */

/* LOCAL32ENTRY.wFlags same as LOCALENTRY.wFlags */

/* LOCAL32ENTRY.wType same as LOCALENTRY.wType */

BOOL    WINAPI Local32Info(LOCAL32INFO FAR* lpli32, HGLOBAL hHeap);
BOOL    WINAPI Local32First(LOCAL32ENTRY FAR* lple32, HGLOBAL hHeap);
BOOL    WINAPI Local32Next(LOCAL32ENTRY FAR* lple32);

/****** Stack Tracing ********************************************************/

typedef struct tagSTACKTRACEENTRY
{
    DWORD dwSize;
    HTASK hTask;
    WORD wSS;
    WORD wBP;
    WORD wCS;
    WORD wIP;
    HMODULE hModule;
    WORD wSegment;
    WORD wFlags;
} STACKTRACEENTRY;

/* STACKTRACEENTRY.wFlags values */
#define FRAME_FAR       0
#define FRAME_NEAR      1

BOOL    WINAPI StackTraceFirst(STACKTRACEENTRY FAR* lpStackTrace, HTASK hTask);
BOOL    WINAPI StackTraceCSIPFirst(STACKTRACEENTRY FAR* lpStackTrace,
            WORD wSS, WORD wCS, WORD wIP, WORD wBP);
BOOL    WINAPI StackTraceNext(STACKTRACEENTRY FAR* lpStackTrace);

/****** Module list walking **************************************************/

typedef struct tagMODULEENTRY
{
    DWORD dwSize;
    char szModule[MAX_MODULE_NAME + 1];
    HMODULE hModule;
    WORD wcUsage;
    char szExePath[MAX_PATH16 + 1];
    WORD wNext;
} MODULEENTRY;

BOOL    WINAPI ModuleFirst(MODULEENTRY FAR* lpModule);
BOOL    WINAPI ModuleNext(MODULEENTRY FAR* lpModule);
HMODULE WINAPI ModuleFindName(MODULEENTRY FAR* lpModule, LPCSTR lpstrName);
HMODULE WINAPI ModuleFindHandle(MODULEENTRY FAR* lpModule, HMODULE hModule);

/****** Task list walking *****************************************************/

typedef struct tagTASKENTRY
{
    DWORD dwSize;
    HTASK hTask;
    HTASK hTaskParent;
    HINSTANCE hInst;
    HMODULE hModule;
    WORD wSS;
    WORD wSP;
    WORD wStackTop;
    WORD wStackMinimum;
    WORD wStackBottom;
    WORD wcEvents;
    HGLOBAL hQueue;
    char szModule[MAX_MODULE_NAME + 1];
    WORD wPSPOffset;
    HANDLE hNext;
} TASKENTRY;

BOOL    WINAPI TaskFirst(TASKENTRY FAR* lpTask);
BOOL    WINAPI TaskNext(TASKENTRY FAR* lpTask);
BOOL    WINAPI TaskFindHandle(TASKENTRY FAR* lpTask, HTASK hTask);
DWORD   WINAPI TaskSetCSIP(HTASK hTask, WORD wCS, WORD wIP);
DWORD   WINAPI TaskGetCSIP(HTASK hTask);
BOOL    WINAPI TaskSwitch(HTASK hTask, DWORD dwNewCSIP);

/****** Window Class enumeration **********************************************/

typedef struct tagCLASSENTRY
{
    DWORD dwSize;
    HMODULE hInst;              /* This is really an hModule */
    char szClassName[MAX_CLASSNAME + 1];
    WORD wNext;
} CLASSENTRY;

BOOL    WINAPI ClassFirst(CLASSENTRY FAR* lpClass);
BOOL    WINAPI ClassNext(CLASSENTRY FAR* lpClass);

/****** Information functions *************************************************/

typedef struct tagMEMMANINFO
{
    DWORD dwSize;
    DWORD dwLargestFreeBlock;
    DWORD dwMaxPagesAvailable;
    DWORD dwMaxPagesLockable;
    DWORD dwTotalLinearSpace;
    DWORD dwTotalUnlockedPages;
    DWORD dwFreePages;
    DWORD dwTotalPages;
    DWORD dwFreeLinearSpace;
    DWORD dwSwapFilePages;
    WORD wPageSize;
} MEMMANINFO;

BOOL    WINAPI MemManInfo(MEMMANINFO FAR* lpEnhMode);

typedef struct tagSYSHEAPINFO
{
    DWORD dwSize;
    WORD wUserFreePercent;
    WORD wGDIFreePercent;
    HGLOBAL hUserSegment;
    HGLOBAL hGDISegment;
    HGLOBAL hWndSegment;
    HGLOBAL hMenuSegment;
    HGLOBAL hGDI32Segment;
} SYSHEAPINFO;

BOOL    WINAPI SystemHeapInfo(SYSHEAPINFO FAR* lpSysHeap);

/****** Interrupt Handling ****************************************************/

/* Hooked interrupts */
#define INT_DIV0            0
#define INT_1               1
#define INT_3               3
#define INT_UDINSTR         6
#define INT_STKFAULT        12
#define INT_GPFAULT         13
#define INT_BADPAGEFAULT    14
#define INT_CTLALTSYSRQ     256

/* TOOLHELP Interrupt callbacks registered with InterruptRegister should
 *  always be written in assembly language.  The stack frame is not 
 *  compatible with high level language conventions.
 *
 *  This stack frame looks as follows to the callback.  All registers
 *  should be preserved across this callback to allow restarting fault.
 *               ------------
 *               |   Flags  |  [SP + 0Eh]
 *               |    CS    |  [SP + 0Ch]
 *               |    IP    |  [SP + 0Ah]
 *               |  Handle  |  [SP + 08h]
 *               |Exception#|  [SP + 06h]
 *               |    AX    |  [SP + 04h]  AX Saved to allow MakeProcInstance
 *               |  Ret CS  |  [SP + 02h]
 *       SP--->  |  Ret IP  |  [SP + 00h]
 *               ------------
 */
BOOL    WINAPI InterruptRegister(HTASK hTask, FARPROC lpfnIntCallback);
BOOL    WINAPI InterruptUnRegister(HTASK hTask);

/*  Notifications:
 *      When a notification callback is called, two parameters are passed
 *      in:  a WORD, wID, and another DWORD, dwData.  wID is one of
 *      the values NFY_* below.  Callback routines should ignore unrecog-
 *      nized values to preserve future compatibility.  Callback routines
 *      are also passed a dwData value.  This may contain data or may be
 *      a FAR pointer to a structure, or may not be used depending on
 *      which notification is being received.
 *
 *      In all cases, if the return value of the callback is TRUE, the
 *      notification will NOT be passed on to other callbacks.  It has
 *      been handled.  This should be used sparingly and only with certain
 *      notifications.  Callbacks almost always return FALSE.
 */

/* NFY_UNKNOWN:  An unknown notification has been returned from KERNEL.  Apps
 *  should ignore these.
 */
#define NFY_UNKNOWN         0

/* NFY_LOADSEG:  dwData points to a NFYLOADSEG structure */
#define NFY_LOADSEG         1
typedef struct tagNFYLOADSEG
{
    DWORD dwSize;
    WORD wSelector;
    WORD wSegNum;
    WORD wType;             /* Low bit set if data seg, clear if code seg */
    WORD wcInstance;        /* Instance count ONLY VALID FOR DATA SEG */
    LPCSTR lpstrModuleName;
} NFYLOADSEG;

/* NFY_FREESEG:  LOWORD(dwData) is the selector of the segment being freed */
#define NFY_FREESEG         2

/* NFY_STARTDLL:  dwData points to a NFYLOADSEG structure */
#define NFY_STARTDLL        3
typedef struct tagNFYSTARTDLL
{
    DWORD dwSize;
    HMODULE hModule;
    WORD wCS;
    WORD wIP;
} NFYSTARTDLL;

/* NFY_STARTTASK:  dwData is the CS:IP of the start address of the task */
#define NFY_STARTTASK       4

/* NFY_EXITTASK:  The low byte of dwData contains the program exit code */
#define NFY_EXITTASK        5

/* NFY_DELMODULE:  LOWORD(dwData) is the handle of the module to be freed */
#define NFY_DELMODULE       6

/* NFY_RIP:  dwData points to a NFYRIP structure */
#define NFY_RIP             7
typedef struct tagNFYRIP
{
    DWORD dwSize;
    WORD wIP;
    WORD wCS;
    WORD wSS;
    WORD wBP;
    WORD wExitCode;
} NFYRIP;

/* NFY_TASKIN:  No data.  Callback should do GetCurrentTask() */
#define NFY_TASKIN          8

/* NFY_TASKOUT:  No data.  Callback should do GetCurrentTask() */
#define NFY_TASKOUT         9

/* NFY_INCHAR:  Return value from callback is used.  If NULL, mapped to 'i' */
#define NFY_INCHAR          10

/* NFY_OUTSTR:  dwData points to the string to be displayed */
#define NFY_OUTSTR          11

/* NFY_LOGERROR:  dwData points to a NFYLOGERROR struct */
#define NFY_LOGERROR        12
typedef struct tagNFYLOGERROR
{
    DWORD dwSize;
    UINT wErrCode;
    void FAR* lpInfo;       /* Error code-dependent */
} NFYLOGERROR;

/* NFY_LOGPARAMERROR:  dwData points to a NFYLOGPARAMERROR struct */
#define NFY_LOGPARAMERROR   13
typedef struct tagNFYLOGPARAMERROR
{
    DWORD dwSize;
    UINT wErrCode;
    FARPROC lpfnErrorAddr;
    void FAR* FAR* lpBadParam;
} NFYLOGPARAMERROR;

/* NotifyRegister() flags */
#define NF_NORMAL       0
#define NF_TASKSWITCH   1
#define NF_RIP          2

typedef BOOL (CALLBACK* LPFNNOTIFYCALLBACK)(WORD wID, DWORD dwData);

BOOL    WINAPI NotifyRegister(HTASK hTask, LPFNNOTIFYCALLBACK lpfn, WORD wFlags);
BOOL    WINAPI NotifyUnRegister(HTASK hTask);

/****** Miscellaneous *********************************************************/

void    WINAPI TerminateApp(HTASK hTask, WORD wFlags);

/* TerminateApp() flag values */
#define UAE_BOX     0
#define NO_UAE_BOX  1

DWORD   WINAPI MemoryRead(WORD wSel, DWORD dwOffset, void FAR* lpBuffer, DWORD dwcb);
DWORD   WINAPI MemoryWrite(WORD wSel, DWORD dwOffset, void FAR* lpBuffer, DWORD dwcb);

typedef struct tagTIMERINFO
{
    DWORD dwSize;
    DWORD dwmsSinceStart;
    DWORD dwmsThisVM;
} TIMERINFO;

BOOL    WINAPI TimerCount(TIMERINFO FAR* lpTimer);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif

#endif /* !_INC_TOOLHELP */
