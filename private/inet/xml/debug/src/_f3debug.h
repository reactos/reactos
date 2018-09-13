//+---------------------------------------------------------------------------
//
//  Microsoft Windows
// Copyright (c) 1993 - 1999 Microsoft Corporation. All rights reserved.*///
//  File:       _f3debug.h
//
//  Contents:   Misc internal debug definitions.
//
//----------------------------------------------------------------------------

//
// Shared macros
//

typedef void *  PV;
typedef char    CHAR;

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof(a[0]))

#ifdef tagError
#undef tagError
#endif

#ifdef tagAssertExit
#undef tagAssertExit
#endif

#ifdef tagAssertPop
#undef tagAssertPop
#endif

#define tagNull     ((TAG) 0)
#define tagMin      ((TAG) 1)
#define tagMax      ((TAG) 512)


/*
 *  TGTY
 *
 *  Tag type.  Possible values:
 *
 *      tgtyTrace       Trace points
 *      tgtyOther       Other TAG'd switch
 */

typedef int TGTY;

#define tgtyNull    0
#define tgtyTrace   1
#define tgtyOther   2

/*
 *  Flags in TGRC that are written to disk.
 */

enum TGRC_FLAG
{
    TGRC_FLAG_VALID =   0x00000001,
    TGRC_FLAG_DISK =    0x00000002,
    TGRC_FLAG_COM1 =    0x00000004,
    TGRC_FLAG_BREAK =   0x00000008,
#ifdef _MAC
    TGRC_FLAG_MAX =     LONG_MAX    // needed to force enum to be dword
#endif

};

#define TGRC_DEFAULT_FLAGS (TGRC_FLAG_VALID | TGRC_FLAG_COM1)

/*
 *  TGRC
 *
 *  Tag record.  Gives the current state of a particular TAG.
 *  This includes enabled status, owner and description, and
 *  tag type.
 *
 */

struct TGRC
{
    /* For trace points, enabled means output will get sent */
    /* to screen or disk.  For native/pcode switching, enabled */
    /* means the native version will get called. */

    BOOL    fEnabled;

    DWORD   ulBitFlags;     /* Flags */
    CHAR *  szOwner;        /* Strings passed at init ... */
    CHAR *  szDescrip;
    TGTY    tgty;           /* TAG type */

    BOOL    TestFlag(TGRC_FLAG mask)
                { return (ulBitFlags & mask) != 0; }
    void    SetFlag(TGRC_FLAG mask)
                { (ULONG&) ulBitFlags |= mask; }
    void    ClearFlag(TGRC_FLAG mask)
                { (ULONG&) ulBitFlags &= ~mask; }
    void    SetFlagValue(TGRC_FLAG mask, BOOL fValue)
                { fValue ? SetFlag(mask) : ClearFlag(mask); }
};

struct SYMBOL_INFO
{
    DWORD       dwOffset;
    char        achModule[12];  // Lengths are arbitrary.
    char        achSymbol[51];
};

#ifdef PRODUCT_96
#define STACK_WALK_DEPTH  5
#else
#define STACK_WALK_DEPTH  10
#endif

/*
 * MBOT
 *
 * Used by the assert code for passing information to the assert dialog
 *
 */

#define TOPURL_LENGTH 200

struct MBOT
{
    const char * szMessage;
    const char * szTitle;
    const char * szFile;
    char         achModule[50];
    char         achTopUrl[TOPURL_LENGTH];
    DWORD        dwLine;
    DWORD        pid;
    DWORD        tid;

    SYMBOL_INFO  asiSym[50];
    int          cSym;

    // DWORD        dwFlags;
    int          id;
    DWORD        dwErr;
};

// This constant is used to determine the number of symbols we use to print to
// the debugger and dialog for an assert.

#define SHORT_SYM_COUNT 5


/*
 *  DBGTHREADSTATE
 *
 *  Per-thread globals
 *
 */
struct DBGTHREADSTATE      // tag: pts
{
    DBGTHREADSTATE *   ptsNext;
    DBGTHREADSTATE *   ptsPrev;

    // Add globals below
    void *          pvRequest;              // Last pointer seen by pre-hook function.
    size_t          cbRequest;              // Last size seen by pre-hook function.
    int             cTrackDisable;          // Disable memory tracking count/flag
    int             cCoTrackDisable;        // Disable Co memory tracking count/flag
    int             iIndent;                // Indent for trace tags
    BOOL            fDirtyDlg;              // Debug UI dirty dialog flag
    BOOL            fSpyRegistered;         // IMallocSpy registered
    BOOL            fSpyAlloc;              // Allocation is from IMallocSpy
    int             mtSpy;                  // Meter to use for IMallocSpy
    int             mtSpyUser;              // Custom meter to use for IMallocSpy
    SYMBOL_INFO     aSymbols[STACK_WALK_DEPTH]; // stored symbols for realloc
    MBOT *          pmbot;                      // Used by assert dialog
    char            achTopUrl[TOPURL_LENGTH];   // Unicode string containing the
                                                //   topmost URL of the current
                                                //   page for this thread.
};

HRESULT DllThreadAttach();
void    DllThreadDetach(DBGTHREADSTATE * pts);

inline HRESULT EnsureThreadState()
{
    extern DWORD    g_dwTls;
    if (!TlsGetValue(g_dwTls))
        return DllThreadAttach();
    return S_OK;
}

inline DBGTHREADSTATE * DbgGetThreadState()
{
    extern DWORD    g_dwTls;
    return (DBGTHREADSTATE *)TlsGetValue(g_dwTls);
}

#undef TLS
#define TLS(x)      (DbgGetThreadState()->x)

/*
 *  CLockGlobals, CLockTrace, CLockResDlg
 *
 *  Classes which automatically lock/unlock global state
 *
 */
#ifndef _MAC
extern CRITICAL_SECTION     g_csTrace;
extern CRITICAL_SECTION     g_csResDlg;
extern CRITICAL_SECTION     g_csDebug;
extern CRITICAL_SECTION     g_csHeapHack;
extern CRITICAL_SECTION     g_csSpy;

class CLockGlobal       // tag: glock
{
public:
    CLockGlobal()   { EnterCriticalSection(&g_csDebug); }
    ~CLockGlobal()  { LeaveCriticalSection(&g_csDebug); }
};

class CLockTrace        // tag: tlock
{
public:
    CLockTrace()   { EnterCriticalSection(&g_csTrace); }
    ~CLockTrace()  { LeaveCriticalSection(&g_csTrace); }
};

class CLockResDlg       // tag: rlock
{
public:
    CLockResDlg()   { EnterCriticalSection(&g_csResDlg); }
    ~CLockResDlg()  { LeaveCriticalSection(&g_csResDlg); }
};

#undef LOCK_GLOBALS
#define LOCK_GLOBALS    CLockGlobal glock
#define LOCK_TRACE      CLockTrace  tlock
#define LOCK_RESDLG     CLockResDlg rlock
#else
#define LOCK_GLOBALS
#define LOCK_TRACE
#define LOCK_RESDLG
#endif


//
// Shared globals
//
extern BOOL                 g_fInit;
extern HINSTANCE            g_hinstMain;
extern HANDLE               g_hProcess;
extern TGRC                 mptagtgrc[];

extern TAG  tagMac;

extern TAG  tagError;
extern TAG  tagAssertPop;
extern TAG  tagAssertExit;
extern TAG  tagAssertStacks;
extern TAG  tagTestFailures;
extern TAG  tagTestFailuresIgnore;
extern TAG  tagRRETURN;
extern TAG  tagTraceCalls;
extern TAG  tagLeaks;
extern TAG  tagValidate;
extern TAG  tagSymbols;
extern TAG  tagSpySymbols;
extern TAG  tagTrackItf;
extern TAG  tagTrackItfVerbose;
extern TAG  tagOLEWatch;  // trace all OLE interface calls made.

extern int  g_cFFailCalled;
extern int  g_firstFailure;
extern int  g_cInterval;
extern BOOL g_fOSIsNT;
extern BOOL g_fAbnormalProcessTermination;

//
//  Shared function prototypes
//

HRESULT        GetTopURLForThread(DWORD tid, char * psz);

BOOL            ValidateInternalHeap();

BOOL            JustFailed();

VOID            SaveDefaultDebugState(void);
void            RestoreDefaultDebugState(void);
BOOL            IsTagEnabled(TAG tag);

BOOL            AreSymbolsEnabled();
void            MagicInit();
void            MagicDeinit();
int             GetStackBacktrace(int           iStart,
                                  int           cTotal,
                                  DWORD_PTR *   pdwEip,
                                  SYMBOL_INFO * psiSym);
void            GetStringFromSymbolInfo(DWORD_PTR   dwAddr,
                                        SYMBOL_INFO *pai,
                                        CHAR *       pszString);

int             hrvsnprintf(char * achBuf, int cchBuf, const char * pstrFmt, va_list valMarker);

char *          MtGetName(int mt);
char *          MtGetDesc(int mt);

#ifndef WIN16
typedef HANDLE EVENT_HANDLE;
typedef HANDLE THREAD_HANDLE;
#define CloseEvent CloseHandle
#define CloseThread CloseHandle
#define IF_WIN16(x)
#define IF_NOT_WIN16(x) x

#define WIN16API

#else

typedef W16HANDLE EVENT_HANDLE;
typedef W16HANDLE THREAD_HANDLE;
#define IF_WIN16(x) x
#define IF_NOT_WIN16(x)

#define WIN16API  WINAPI __loadds

#define wsprintfA   wsprintf
#define wsprintfW   wsprintf
#define wvsprintfA  wvsprintf
#define wvsprintfW  wvsprintf

#define sprintf wsprintf
#define _snprintf(a, b, c, d, e) wsprintf(a, c, d, e)
#define _vsnprintf(a, b, c, d) wvsprintf(a, c, d)
#endif

