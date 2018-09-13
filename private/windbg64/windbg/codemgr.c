/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    codemgr.c

Abstract:

    This file contains the majority of the interface code to the
    OSDebug API

Author:

    Jim Schaad (jimsch)
    Griffith Wm. Kadnier (v-griffk) 16-Jan-1993

Environment:

    Win32 user mode

--*/


#include "precomp.h"
#pragma hdrstop

#include "dbugexcp.h"

#define MAX_MAPPED_ROOTS        (5)

#ifdef __cplusplus
extern "C" {
#endif

extern AVS Avs;

#ifdef __cplusplus
} // extern "C" {
#endif

#include "include\cntxthlp.h"

extern HWND GetWatchHWND(void);


typedef struct _BROWSESTRUCT {
    LRESULT             Rslt;
    DWORD               DlgId;
    DLGPROC             DlgProc;
    LPSTR               pszFileName;
    DWORD               FnameSize;
    PFIND_SYM_FILE      pFindSymFileData;
} BROWSESTRUCT, *LPBROWSESTRUCT;


LRESULT
SendMessageNZ (
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
WINAPI
DlgFileSearchResolve(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );





extern  CVF Cvf;
extern  CRF Crf;
extern  KNF Knf;
extern  DBF Dbf;

extern EXCEPTION_LIST   *DefaultExceptionList;

EESTATUS    PASCAL MELoadEEParse(const char FAR *, EERADIX, SHFLAG, PHTM, LPDWORD);
XOSD OSDAPI OSDCallbackFunc(DWORD, HPID, HTID, DWORD64, DWORD64);


DBGSTATE DbgState = ds_normal;

CXF CxfIp;

HTL     Htl = 0;            /* Handle to transport layer    */
HEM     Hem = 0;            /* Handle to execution module   */
HPID    HpidBase;           /* Handle to base PID       */
HTID    HtidBase;           /* Handle to base TID       */

LPPD    LppdFirst = NULL;

extern LPSHF Lpshf;         /* Pointer to SH entry structure */
ATOM *  RgAtomMaskedNames = NULL;   /* Names of masked source files */
int CMacAtomMasked = 0;     /* Size of array        */
int CAtomMasked = 0;        /* Count of atoms in array  */

struct MpPair {
    ATOM    atomSrc;
    ATOM    atomTarget;
}  * RgAtomMappedNames = NULL;      /* Mapping from source to target names */
int CMacAtomMapped = 0;     /* Size of array        */
int CAtomMapped = 0;        /* Count of mappings in array   */

// Structure to map one root to another

struct MRootPair {
    DWORD   dwSrcLen;
    LPSTR   lpszSrcRoot;
    LPSTR   lpszTargetRoot;
} RgMappedRoots[MAX_MAPPED_ROOTS];
UINT CMappedRoots = 0;

//
// When a source file can be mapped to one or more open documents,
// the user may choose of to use a currently loaded document in 
// which case the return value is >=0. If the user chooses to use
// the source file that was specified by the debug information, then
// a value of 'MATCHLIST_USEFILEFROMIMAGE' is returned. If the user
// decides not to use any of these values then 'MATCHLIST_NONEMATCHED'
// is returned.
#define MATCHLIST_NONEMATCHED       (-1)
#define MATCHLIST_USEFILEFROMIMAGE  (-2)

static INT  MatchedList[MAX_DOCUMENTS];
static LONG lMatchCnt = 0;
static LONG lMatchIdx = 0;
static CHAR szFSSrcName[MAX_PATH];
static BOOL FAddToSearchPath = FALSE;
static BOOL FAddToRootMap = FALSE;

static CHAR  szBrowsePrompt[256];
static CHAR  szBrowseFname[256];
static BOOL  fBrowseAnswer;

/*
**  Expression Evaluator items
*/

CI  Ci = { sizeof(CI), 0, &Cvf, &Crf };
EXF Exf = {NULL, NULL, MELoadEEParse};
EI  Ei = {
    sizeof(EI), 0, &Exf
};

// If it isn't large enough, windbg can wedge during startup
// since it only has one thread, and none of the items in
// the queue can possibly be removed.
//
// This will usually occur if the back-end generates a lot of
// text output to the command window via DPRINT.
#ifdef DBG
#define QUEUE_SIZE      (1024 * 100)
#else
#define QUEUE_SIZE      (1024 * 10)
#endif
static DWORD64          RgbDbgMsgBuffer[QUEUE_SIZE];
static int              iDbgMsgBufferFront = 0;
static int              iDbgMsgBufferBack = 0;
static CRITICAL_SECTION csDbgMsgBuffer;


BOOL    FKilling       = FALSE;

HWND    HwndDebuggee = NULL;

/*********************** Prototypes *****************************************/

BOOL FLoadEmTl(BOOL *pfReconnecting);

BOOL    RootNameIsMapped(LPSTR, LPSTR, UINT);

BOOL    SrcNameIsMasked(ATOM);
BOOL    SrcNameIsMapped(ATOM, LSZ, UINT);
INT     MatchOpenedDoc(LPSTR, UINT);
BOOL    SrcSearchOnPath(LSZ, UINT, BOOL);
BOOL    SrcSearchOnRoot(LSZ, UINT);
BOOL    SrcBrowseForFile(LSZ, UINT);

BOOL    MiscBrowseForFile(
    LSZ     lpb,
    UINT    cb,
    LSZ     lpDir,
    UINT    cchDir,
    int     nDefExt,
    int     nIdDlgTitle,
    void    (*fnSetMapped)(LSZ, LSZ),
    LPOFNHOOKPROC lpfnHook,
    int     nExplorerExtensionTemplateName
    );

VOID    SrcSetMasked(LSZ);
VOID    SrcSetMapped(LSZ, LSZ);
VOID    ExeSetMapped(LSZ, LSZ);
void EnsureFocusDebuggee( void );

VOID     CmdMatchOpenedDocPrompt(BOOL, BOOL);
BOOL     CmdMatchOpenedDocInputString(LPSTR);


VOID
InitCodemgr(
    void
    )
/*++

Routine Description:

    Initialize private data for Codemgr.c

Arguments:

    none

Return Value:

    none

--*/
{
    InitializeCriticalSection(&csDbgMsgBuffer);
}


LRESULT
SendMessageNZ(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    Call SendMessage() if and only if the handle is non-zero

Arguments:

    Exactly the same as the Win32 SendMessage() call.


Return Value:

    The return from the SendMessage() or 0 if we didn't send it.

--*/
{
    if (hWnd) {
        return( SendMessage(hWnd, uMsg, wParam, lParam) );
    }

    else {
        return(0);
    }
}


BOOL PASCAL
DbgCommandOk(
    void
    )
/*++

Routine Description:

    This routine is called before issuing any debugger commands.
    it will validate will all the appropriate windows that no
    editing commands are current in progess which would cause the
    debugger to abort out.

Arguments:

    None.

Return Value:

    None.

--*/
{
    return TRUE;
}                   /* DbgOk() */


BOOL PASCAL
DbgFEmLoaded(
    void
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    return (Hem != 0);
}                   /* DbgFEmLoaded() */


void PASCAL
Go(
    void
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    EXOP exop = {0};


    if (OSDGo(LppdCur->hpid, LptdCur->htid, &exop) == xosdNone) {
        if (LppdCur->pstate == psPreRunning) {
            SetPTState(psInvalidState, tsRunning);
        } else {
            SetPTState(psRunning, tsRunning);
            LppdCur->fHasRun = TRUE;
        }
        EnsureFocusDebuggee();
    }

    return;
}                   /* Go() */


BOOL PASCAL
GoUntil(
    PADDR paddr
    )
/*++

Routine Description:

    Set temporary breakpoint and go
    Modelled after go_until in CV0.C

Arguments:


Return Value:


--*/
{
    HBPT    hBpt;

    if (BPSetTmp(paddr, LppdCur->hpid, LptdCur->htid, &hBpt) != BPNOERROR) {
        return FALSE;
    }
    LptdCur->fDisasm = TRUE;

    AuxPrintf(3, "GoUntil - doing the go!!!, BP set at 0x%X:0x%I64X",
                GetAddrSeg(*paddr),
                GetAddrOff(*paddr));
    Go();

    return TRUE;
}                   /* GoUntil() */


int
Step(
    int Overcalls,
    int StepMode
    )
/*++

Routine Description:

    Single step at source or assembler code level.

Arguments:


Return Value:


--*/
{
    DWORD    wLn     = 0;
    SHOFF   cbLn    = 0;
    SHOFF   dbLn    = 0;
    CXT     cxt;
    ADDR    addr;
    ADDR    addr2;
    EXOP    exop = {0};

    if (!DebuggeeAlive()) {
        AuxPrintf(1, "STEP - child is dead");
        return FALSE;
    }

    exop.fStepOver = (UCHAR) Overcalls;

    switch (StepMode) {
      case ASMSTEPPING: // step a machine instruction

        if (OSDSingleStep(LppdCur->hpid, LptdCur->htid, &exop) != xosdNone) {
            return FALSE;
        } else {
            // it is possible to do many steps before entering
            // the real debuggee.  Set the stop at entry flag
            // to ensure that we stop after leaving the loader.
            if (!LppdCur->fHasRun) {
                LppdCur->fStopAtEntry = TRUE;
            }

            SetPTState(psRunning, tsRunning);

            EnsureFocusDebuggee();
            return TRUE;
        }

      case SRCSTEPPING: // step a source line

        // If this is an initial step, the breakpoint
        // will be resolved at the entrypoint event.
        if (!LppdCur->fHasRun ) {
            LppdCur->fInitialStep = 1;
            Go();
            return TRUE;
        }

        OSDGetAddr(LppdCur->hpid, LptdCur->htid, adrPC, &addr);

        if (!ADDR_IS_LI( addr ) ) {
            SYUnFixupAddr ( &addr );
        }

        SHHmodFrompCxt(&cxt) = (HMOD) NULL;
        SHSetCxt(&addr, &cxt);

        if (!SHHmodFrompCxt(&cxt) ||
                        !SLLineFromAddr ( &addr, &wLn, &cbLn, &dbLn )) {

            exop.fInitialBP = TRUE;
            if (OSDSingleStep(LppdCur->hpid, LptdCur->htid, &exop)
                                                               != xosdNone) {
                return FALSE;
            } else {
                SetPTState(psRunning, tsRunning);
                EnsureFocusDebuggee();
                return TRUE;
            }

        } else {

            Assert( cbLn >= dbLn );
            if (cbLn < dbLn) {
                return FALSE;
            }

            SYFixupAddr(&addr);
            addr2 = addr;
            GetAddrOff(addr2) += cbLn - dbLn;
            exop.fInitialBP = TRUE;

            if (OSDRangeStep(LppdCur->hpid, LptdCur->htid,
                                           &addr, &addr2, &exop) != xosdNone) {
                return FALSE;
            } else {
                SetPTState(psRunning, tsRunning);
                EnsureFocusDebuggee();
                return TRUE;
            }
        }

      default:
        Assert(FALSE);
        break;
    }
    return FALSE;
}                   /* Step() */


BOOL PASCAL
DebuggeeRunning(
    void
    )
/*++

Routine Description:

    This will return TRUE iff the child debuggee current thread
    is actually executing code.

Arguments:


Return Value:

    TRUE if the debuggee is currently running, FALSE otherwise

--*/
{
    //
    //  The name of the function (and of SetDebugeeRunning) is
    //  ambiguous. Does this mean the current process/thread is
    //  running? Or any thread in the current process? Or
    //  any process being debugged?
    //
    //  From the way people use the function seems like its
    //  semantics are "Current thread is running" so it is
    //  implemented that way.
    //
    if ( LptdCur != NULL ) {
        return (LptdCur->tstate == tsRunning);
    }

    return FALSE;
}                   /* DebuggeeRunning() */


BOOL PASCAL
IsProcRunning(
    LPPD lppd
    )
/*++

Routine Description:

    Alternative to DebuggeeRunning(); determines whether process
    is actually running, not suspended.

Arguments:

    lppd  - Supplies pointer to process descriptor

Return Value:

    TRUE if running, FALSE if not

--*/
{
    LPTD lptd;
    if (lppd == NULL) {
        return FALSE;
    }
    if (!DebuggeeActive()) {
        return FALSE;
    }

    for (lptd = lppd->lptdList; lptd; lptd = lptd->lptdNext) {
        if (lptd->tstate != tsRunning) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL PASCAL
IsProcStopped(
    LPPD lppd
    )
/*++

Routine Description:

    Not precisely the opposite of IsProcRunning - this returns TRUE
    only if an existing thread is stopped, ignoring any thread which is
    being created.

Arguments:

    lppd  - Supplies pointer to process descriptor

Return Value:

    TRUE if running, FALSE if not

--*/
{
    LPTD lptd;
    if (lppd == NULL) {
        return FALSE;
    }
    if (!DebuggeeActive()) {
        return FALSE;
    }

    for (lptd = lppd->lptdList; lptd; lptd = lptd->lptdNext) {
        if (lptd->tstate != tsRunning && lptd->tstate != tsPreRunning) {
            return TRUE;
        }
    }

    return FALSE;
}


/*********************************************************************

    UI oriented general purpose functions for debugging sessions

*********************************************************************/


/***    GetExecutableFilename
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**  Address of passed buffer if successful, NULL otherwise
**
**  Description:
**  Gets the full path name of the current executable file -
**  from the project if there is one, or from the current
**  source file otherwise.
**
*/

PSTR PASCAL
GetExecutableFilename(
    PSTR pszExecutable,
    UINT uSize
    )
{
    Assert(pszExecutable);
    Assert(uSize > 0);

    PCSTR pszProgramName = g_Windbg_WkSp.GetCurrentProgramName(FALSE);

    if (pszProgramName) {
        strncpy(pszExecutable, pszProgramName, uSize);
        pszExecutable[uSize-1] = NULL;
        return pszExecutable;
    } else {
        return NULL;
    }
}


void PASCAL
BuildRelativeFilename(
                      LPSTR rgchSource,
                      LPSTR rgchBase,
                      LPSTR rgchDest,
                      int cbDest
                      )

/*++

Routine Description:

    Given a filename and base directory build the
    fully specified filename.

    Note that BaseDir can be a fully qualified path name
    (ie including filename) or just the directory.  If
    just the directory is passed it should contain the
    trailing '\\'.

Arguments:

    rgchSource   - Supplies the source name to build relative path for
    rgchBase     - Supplies the Base directory to build path relative to
    rgchDest     - Supplies a Buffer to place the absolute path name to
    cbDest       - Supplies the Length of buffer

Return Value:

    None.

--*/

{
    char *      pchDest;
    char *      pchSource = rgchSource;
    char *      pchBase = rgchBase;
    int         iDriveCur = _getdrive();
    char        rgchDirCur[_MAX_PATH];
    int         iDriveSrc;

    _getcwd(rgchDirCur, sizeof(rgchDirCur));

    /*
     *  Check to see if either of the passed in directories have
     *  drives specified.
     */

    if (!IsDBCSLeadByte(pchSource[0]) && pchSource[1] == ':') {
        iDriveSrc = toupper(pchSource[0]) - 'A' + 1;
        pchSource += 2;
        if (_chdrive( iDriveSrc ) == -1) {
            goto nextDrive;
        }
        pchBase = 0;
    } else {
    nextDrive:
        if (!IsDBCSLeadByte(pchBase[0]) && pchBase[1] == ':') {
            iDriveSrc = toupper(pchBase[0]) - 'A' + 1;
            pchBase += 2;
            if (_chdrive( iDriveSrc ) == -1) {
                iDriveSrc = iDriveCur;
            }
        } else {
            iDriveSrc = iDriveCur;
        }
    }

    rgchDest[0] = 'A' + iDriveSrc - 1;
    rgchDest[1] = ':';
    rgchDest[2] = '\\';
    pchDest = &rgchDest[2];

    /*
     *  Now check to see if either base is based at the root.  If not
     *  then we need to the get current directory for that drive.
     */

    if ((pchSource[0] == '\\') || (pchSource[0] == '/')) {
        pchSource ++;
        pchBase = NULL;
        cbDest -= 3;
    } else if ((pchBase != NULL) &&
               ((pchBase[0] == '\\') || (pchBase[0] == '/'))) {
        pchBase ++;
        cbDest -= 3;
    } else {
        Dbg(_getcwd(rgchDest, cbDest-1) != NULL);
        pchDest = CharPrev(rgchDest, rgchDest + strlen(rgchDest));
        if (*pchDest != '\\') {
            if (IsDBCSLeadByte(*pchDest)) {
                pchDest += 2;
            } else {
                pchDest++;
            }
            *pchDest = '\\';
        }
        cbDest = (int) (cbDest - (pchDest - rgchDest + 1));
    }

    /*
     * Now lets copy from the base to the destination looking for
     *       any funnyness in the path being copied.
     */

    if (pchBase != NULL) {
        char    ch;
        char *  pch = CharPrev(pchBase, pchBase + strlen(pchBase));
        if (pch == pchBase) {
            // Make sure the result is same as US code.
            pch = pchBase - 1;
        }

        while ((pch >= pchBase) &&
               ((*pch != '\\') && (*pch != '/'))) {
            if ((pch = CharPrev(pchBase, pch)) == pchBase) {
                // Make sure the result is same as US code.
                pch--;
            }
        }

        if ((*pch == '\\') || (*pch == '/')) {
            pch++;
            ch = *pch;
            *pch = 0;
        } else {
            ch = *(pch = CharNext(pch));
            pch = *pch ? pch : pch+1;
            *pch = 0;
        }

        while (*pchBase != 0) {
            if (*pchBase == '.') {
                if (pchBase[1] == '.') {
                    if ((pchBase[2] == '\\') || (pchBase[2] == '/')) {
                        /*
                         *  Found the string '..\' in the input, move up to the
                         *  next '\' unless the next character up is a ':'
                         */

                        pchDest = CharPrev(rgchDest, pchDest);
                        cbDest += ((IsDBCSLeadByte(*pchDest)) ? 2 : 1);
                        if (*pchDest == ':') {
                            pchDest++;
                            cbDest -= 1;
                        } else {
                            while (*pchDest != '\\') {
                                Assert(*pchDest != ':');
                                pchDest = CharPrev(rgchDest, pchDest);
                                cbDest += ((IsDBCSLeadByte(*pchDest)) ? 2 : 1);
                            }
                        }

                        pchBase += 3;
                    } else {
                        /*
                         *  Found the string '..X' where X was not '\', this
                         *  is "illegal" but copy it straight over
                         */

                        *++pchDest = *pchBase++;
                        *++pchDest = *pchBase++;
                        cbDest -= 2;
                    }
                } else if ((pchBase[1] == '\\') || (pchBase[1] == '/')) {
                    /*
                     * We just found the string '.\'  This is an ignore string
                     */

                    pchBase += 2;
                } else {
                    /*
                     * We just found the string '.X' where X was not '\', this
                     *      is legal and just copy over
                     */
                    *++pchDest = *pchBase++;
                    cbDest -= 1;
                }
            } else if (*pchBase == '/') {
                /*
                 * convert / to \
                 */

                *++pchDest = '\\';
                pchBase++;
                cbDest -= 1;
            } else {
                /*
                 * No funny characters
                 */

                if (IsDBCSLeadByte(*pchBase) && *(pchBase+1)) {
                    *++pchDest = *pchBase++;
                    cbDest -= 1;
                }
                *++pchDest = *pchBase++;
                cbDest -= 1;
            }
        }

        *pch = ch;
    }

    /*
     * Now lets copy from the source to the destination looking for
     *       any funnyness in the path being copied.
     */

    while (*pchSource != 0) {
        if (*pchSource == '.') {
            if (pchSource[1] == '.') {
                if ((pchSource[2] == '\\') || (pchSource[2] == '/')) {
                    /*
                     *  Found the string '..\' in the input, move up to the
                     *  next '\' unless the next character up is a ':'
                     */

                    pchDest = CharPrev(rgchDest, pchDest);
                    cbDest += ((IsDBCSLeadByte(*pchDest)) ? 2 : 1);
                    if (*pchDest == ':') {
                        pchDest++;
                        cbDest -= 1;
                    } else {
                        while (*pchDest != '\\') {
                            Assert(*pchDest != ':');
                            pchDest = CharPrev(rgchDest, pchDest);
                            cbDest += ((IsDBCSLeadByte(*pchDest)) ? 2 : 1);
                        }
                    }

                    pchSource += 3;
                } else {
                    /*
                     *  Found the string '..X' where X was not '\', this
                     *  is "illegal" but copy it straight over
                     */

                    *++pchDest = *pchSource++;
                    *++pchDest = *pchSource++;
                    cbDest -= 2;
                }
            } else if ((pchSource[1] == '\\') || (pchSource[1] == '/')) {
                /*
                 * We just found the string '.\'  This is an ignore string
                 */

                pchSource += 2;
            } else {
                /*
                 * We just found the string '.X' where X was not '\', this
                 *      is legal and just copy over
                 */
                *++pchDest = *pchSource++;
                cbDest -= 1;
            }
        } else {
            /*
             * No funny characters
             */

            if (IsDBCSLeadByte(*pchSource) && *(pchSource+1)) {
                *++pchDest = *pchSource++;
                cbDest -= 1;
            }
            *++pchDest = *pchSource++;
            cbDest -= 1;
        }
    }

    *++pchDest = 0;

    Dbg(_chdir(rgchDirCur) == 0);
    return;
}                   /* BuildRelativeFilename() */


void
PASCAL
EnsureFocusDebugger()
/*++

Routine Description:

    Set the foreground window to the debugger, if it wasn't already.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (AutoTest) {
        return;
    }

    HwndDebuggee = GetForegroundWindow();

    if ((HwndDebuggee != hwndFrame) && !IsChild(hwndFrame, HwndDebuggee)
        && !IsIconic(hwndFrame)) {

        SetForegroundWindow(hwndFrame);
    }
}                   /* EnsureFocusDebugger() */


void
EnsureFocusDebuggee(
    void
    )
/*++

Routine Description:

    Set the foreground window to the client, or out best guess as to
    what the client was

Arguments:

    None.

Return Value:

    None.

--*/
{
    return;
}                   /* EnsureFocusDebuggee() */


HWND
GetTopSourceWindow(
                   BOOL bAsmOk
                   )
/*++
Routine Description:
    Returns the handle of the topmost src. It can optionally
    consider the asm window as a src window.

Arguments:
    bAsmOk - Consider the asm window as a src window.

Return Value:
    SUCCESS - Returns the handle of a valid window. If bAsmOk is FALSE,
        then the return value will always be the handle of a doc
        window. If bAsmOk is TRUE, then the return value, can either
        be a doc or a src window.
    FAILURE - NULL.

--*/
{
    HWND hwnd = GetTopWindow(g_hwndMDIClient);
    int nView, nType;

    while ( hwnd ) {

        nView = GetWindowWord( hwnd, GWW_VIEW );

        // Valid view index?
        if ( nView >= 0 && nView < MAX_VIEWS ) {

            // Figure out what type of window we have
            if (Views[nView].Doc < -1) {
                nType = -Views[nView].Doc;
            } else {
                nType = Docs[Views[nView].Doc].docType;
            }

            if (bAsmOk && DISASM_WIN == nType // Consider the asm window a src window?
                || DOC_WIN == nType) { // We found a doc window?
                return hwnd;
            }
        }
        hwnd = GetNextWindow( hwnd, GW_HWNDNEXT );
    }

    // No src windows
    return NULL;
}


void
UpdateDebuggerState(
    UINT UpdateFlags
    )
/*++

Routine Description:

    According to the passed flags asks the various debug
    windows (Watch, Locals, etc) to update their displays.

    ??? Also take care of handling system state when debuggee dies.

Arguments:

    UpdateFlags -

Return Value:


--*/
{
    BOOL    bDebugeeActive;
    ADDR    addr = {0};
    int     indx;
    HWND    hwndTopSource;
    int     view;
    BOOL    bForceDisasm = FALSE;
    int     fGotNext = 0;

    bDebugeeActive = DebuggeeActive();


    //
    //  Get a current CS:IP for the expression evaluator
    //

    if ((UpdateFlags & UPDATE_CONTEXT) && bDebugeeActive) {

        Assert(LppdCur && LptdCur);
        if (LppdCur && LptdCur) {
            HTID vhtid;
            OSDGetAddr(LppdCur->hpid, LptdCur->htid, adrPC, &addr);
            SHChangeProcess(LppdCur->hpds);
            if ( ! ADDR_IS_LI ( addr ) ) {
                SYUnFixupAddr ( &addr );
            }
            memset(&CxfIp, 0, sizeof(CxfIp));
            SHSetCxt(&addr, SHpCxtFrompCxf( &CxfIp ) );
            SHhFrameFrompCXF(&CxfIp) = (HFRAME) LptdCur->htid;
        }
    }

    //
    //  Tell the expression evaluator to dump any cached values
    //

    if (EEInvalidateCache != NULL) {
        EEInvalidateCache();
    }

    if ( (UpdateFlags & UPDATE_CPU) && bDebugeeActive) {
        SendMessageNZ( GetCpuHWND(), WU_UPDATE, 0, 0L);
    }

    if ( (UpdateFlags & UPDATE_FLOAT) && bDebugeeActive) {
        SendMessageNZ( GetFloatHWND(), WU_UPDATE, 0, 0L);
    }


    if ( (UpdateFlags & UPDATE_LOCALS) && bDebugeeActive) {
        SendMessageNZ( GetLocalHWND(), WU_UPDATE, 0, 0L);
    }

    if ( UpdateFlags & UPDATE_WATCH) {

        if (UpdateFlags & UPDATE_SYMBOLS_CHANGED) {
            ReloadAllWatchVariables();
            SendMessageNZ( GetWatchHWND(), WU_INVALIDATE, 0, 0L);
        } else {
            SendMessageNZ( GetWatchHWND(), WU_UPDATE, 0, 0L);
        }
    }

    if ( UpdateFlags & UPDATE_CALLS) {
        SendMessageNZ( GetCallsHWND(), WU_UPDATE, 0, 0L);
    }



    //
    //
    //

    if (UpdateFlags & UPDATE_SOURCE) {

        if (!bDebugeeActive) {

            ClearAllDocStatus(BRKPOINT_LINE | CURRENT_LINE | UBP_LINE);

        } else {

            char SrcFname[_MAX_PATH];
            DWORD SrcLine;
            int doc;
            int  saveDoc = TraceInfo.doc;
            int  iViewCur = curView;

            //
            //  Clear out any existing current source line highlighting
            //

            if (TraceInfo.doc != -1) {
                if (Docs[TraceInfo.doc].FirstView != -1) {
                    LineStatus(TraceInfo.doc, TraceInfo.CurTraceLine,
                        CURRENT_LINE, LINESTATUS_OFF, FALSE, TRUE);
                }

                TraceInfo.doc = -1;
                TraceInfo.CurTraceLine = -1;
            }


            if (!GetCurrentSource(SrcFname, sizeof(SrcFname), &SrcLine)) {

                if (UpdateFlags & UPDATE_NOFORCE) {
                    bForceDisasm = FALSE;
                } else {
                    bForceDisasm = TRUE;
                }

            } else {

                AuxPrintf(1, "Got Source:%s, Line:%u", (LPSTR)SrcFname, SrcLine);

                if (UpdateFlags & UPDATE_NOFORCE) {
                    fGotNext = SrcMapSourceFilename(SrcFname, sizeof(SrcFname),
                        SRC_MAP_ONLY, FindDoc1,
                        FALSE); // Not user activated
                } else {
                    fGotNext = SrcMapSourceFilename(SrcFname, sizeof(SrcFname),
                        SRC_MAP_OPEN, FindDoc1,
                        FALSE); // Not user activated
                }

                //
                //
                //

                if (fGotNext > 0) {
                    // We have a new document.

                    Dbg( FindDoc1(SrcFname, &doc, TRUE) );

                    LineStatus(doc, SrcLine, CURRENT_LINE, LINESTATUS_ON, TRUE, TRUE);

                    TraceInfo.doc = doc;
                    TraceInfo.CurTraceLine = SrcLine;
                }
            }

        }

    }


    //
    // always update a memory window if "live" flag is set
    //

    for (indx = 0; indx < MAX_VIEWS; indx++) {
        if (Views[indx].Doc > -1) {
            if (Docs[Views[indx].Doc].docType == MEMORY_WIN) {
                memView = indx;
                if ((MemWinDesc[memView].fLive) ||
                    (UpdateFlags & UPDATE_MEMORY)) {
                    // check of valid views or sparse array
                    ViewMem(indx, TRUE);
                }
            }
        }
    }


    if ( disasmView != -1 // If it is already open, let's update it
        // Else let's see if we want to open it.
        || (bForceDisasm && !(g_contWorkspace_WkSp.m_dopDisAsmOpts & dopNeverOpenAutomatically) )
        || fGotNext < 0 || !GetSrcMode_StatusBar()) {

        //
        // We only open the disasm window if:
        // (1) Windbg requested it and "never open disasm" option is not checked.
        // (2) The respective src file could not be found
        // (3) We are in ASM mode

        if (disasmView == -1) {
            OpenDebugWindow(DISASM_WIN, FALSE); // Not user activated
        }

        if (disasmView != -1) {
            if (bDebugeeActive) {
                ViewDisasm(SHPAddrFromPCxf(&CxfIp), DISASM_PC);
            }
        }
    }

    EnableToolbarControls();
}                   // UpdateDebuggerState()


/***    SetDebugLines
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**  Given a doc, set the debug line highlights,
**  (ie breakpoints, current_line) that refer to that doc.
**
**  NOTE:           This can be called whether or not there is a current
**      debuggee.  When there isn't, only source line bps
**      are highlighted.
*/

void SetDebugLines(int doc, BOOL ResetTraceInfo)
{
    Unreferenced( doc );
    Unreferenced( ResetTraceInfo );

    BPHighlightSourceFile( Docs[doc].szFileName );
}                   /* SetDebugLines() */


/***    AdjustDebugLines
**
**  Synopsis:
**  void = AdjustDebugLines(DocNumber, StartLine, NumberLines, Added)
**
**  Entry:
**  DocNumber   - Index of document to have lines adjusted for
**  StartLine   - First line to be adjusted
**  NumberLines - Number of lines to adjust by
**  Added       - TRUE if lines were inserted FALSE if lines were deleted
**
**  Returns:
**  Nothing
**
**  Description:
**  Updates source/line breakpoint nodes when lines are
**  added/deleted to a file in the editor.  If Added is
**  TRUE the lines have been added otherwise they've been
**  deleted.  Also updates the TraceInfo var.
**    NOTE:         This is called from the editor every time a block is
**  added or deleted.
**  Insertions are always performed BEFORE the StartLine.
**  Deletions are always performed INCLUDING the StartLine.
**  StartLine is passed 0 relative.
**
**  Also note that for the TraceInfo, all we avoid is
**  having multiple trace lines.  If lines are added
**  or deleted to a file the current line will still
**  seem wrong as this info. comes from the debugging
**  info.
**
*/

void PASCAL AdjustDebugLines(int DocNumber, int StartLine, int NumberLines, BOOL Added)
{
    Unused(DocNumber);
    Unused(StartLine);
    Unused(NumberLines);
    Unused(Added);
}                   /* AdjustDebugLines() */


/*********************************************************************

    General Task Management Routines

    KillDebuggee
    AttachDebuggee
    RestartDebuggee

*********************************************************************/


BOOL
KillDebuggee(
    void
    )
/*++

Routine Description:

    This routine will check to see if a debuggee is currently
    loaded in the system.  If so it will kill the debuggee and
    any children.

Arguments:

    None.

Return Value:

    TRUE if the child was killed and FALSE otherwise

--*/
{
    MSG     msg;
    LPPD    lppd;
    HPID    hpid;
    BOOL    fTmp;
    BOOL    rVal = TRUE;


    /*
    **  Clear out any existing current source line highlighting
    */

    if (TraceInfo.doc != -1) {
        if (Docs[TraceInfo.doc].FirstView != -1) {
            LineStatus(TraceInfo.doc, TraceInfo.CurTraceLine,
            CURRENT_LINE, LINESTATUS_OFF, FALSE, TRUE);
        }

        TraceInfo.doc = -1;
        TraceInfo.CurTraceLine = -1;
    }

    FKilling = TRUE;
    fTmp = SetAutoRunSuppress(TRUE);

    for (;;) {

        /*
        **  See if there is anything to kill
        */

        lppd = GetLppdHead();
        while (lppd &&
                  (lppd->pstate == psDestroyed
                || lppd->pstate == psError
                || lppd->pstate == psNoProgLoaded))
        {
            lppd = lppd->lppdNext;
        }

        if (!lppd) {
            break;
        }

        hpid = lppd->hpid;
        BPTUnResolveAll(hpid);


        /*
         *  This is a synchronous call and we must
         *  pump callback messages through until the
         *  process has been deleted.
         */

        if ( (OSDProgramFree(hpid) != xosdNone)
                && (lppd->pstate != psDestroyed) ) {

            // if it got killed while we weren't looking,
            // there should already be a message in the queue:

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                ProcessQCQPMessage(&msg);
                lppd = LppdOfHpid( hpid );
                if ((lppd == NULL) || (lppd->pstate == psDestroyed)) {
                    break;
                }
            }

            // if it didn't go away, mark it as damaged:

            if (lppd && (lppd == LppdOfHpid(hpid))) {
                lppd->pstate = psError;
            }

        //
        // this is for the case in the kernel debugger where the init
        // of the dm fails (com port problems) and the createprocess
        // never happens.
        //
        } else if (lppd->pstate == psPreRunning) {
            lppd->pstate = psDestroyed;

        } else if ( (lppd->pstate != psDestroyed)
                 || (lppd != LppdOfHpid(hpid)) ) { // <--is this possible??

            while (GetMessage(&msg, NULL, 0, 0)) {
                ProcessQCQPMessage(&msg);
                lppd = LppdOfHpid( hpid );
                if ((lppd == NULL) || (lppd->pstate == psDestroyed))
                {
                    break;
                }
            }

        }

    }

    SetAutoRunSuppress(fTmp);
    FKilling = FALSE;


    if (rVal) {
        // if we succeeded in killing everything...
        SetIpid(1);
    }

    return( rVal );
}                   /* KillDebuggee() */


void
ClearDebuggee(
    void
    )
/*++

Routine Description:

    This function is called to implement Run.Stop Debugging.  It will
    kill any currently loaded debugee and unload all of the debugging DLLs.

Arguments:

    None.

Return Value:

    None.

--*/

{
    HCURSOR     hcursor;
    LPSTR       lpsz;
    XOSD        xosd;
    int         len;

    hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (DbgState == ds_init) {
        CmdLogVar(ERR_DbgState);
        return;
    }

    while (DebuggeeActive()) {
        KillDebuggee();
    }

    len = ModListGetSearchPath( NULL, 0 );
    if (len) {
        lpsz = (PSTR) malloc(len);
        ModListGetSearchPath( lpsz, len );
        ModListInit();
        ModListSetSearchPath( lpsz );
        free(lpsz);
    }
    ModListAdd( NT_KERNEL_NAME  , sheNone );

    if (LppdFirst) {
        if (LppdFirst->hpds) {
            SHDeleteProcess(LppdFirst->hpds);
        }
        OSDDestroyHpid( HpidBase );
        DestroyPd( LppdFirst, TRUE );
        LppdFirst = LppdCur = NULL;
        SetIpid(0);
    }

    if (HModEM != 0) {

        SendMessageNZ( GetCpuHWND(),   WU_DBG_UNLOADEM, 0, 0);  // Give'em a
        SendMessageNZ( GetFloatHWND(), WU_DBG_UNLOADEM, 0, 0);  // Chance

        OSDDeleteEM( Hem );
        FreeLibrary( HModEM );

        HModEM = 0;
        Hem = 0;
        HpidBase = 0;
        HtidBase = 0;
    }


    if (HModTL != 0) {
        xosd = OSDDeleteTL( Htl );

        FreeLibrary( HModTL );
        HModTL = 0;
        Htl = 0;
    }

    if (HModEE != 0) {

        SendMessageNZ( GetCpuHWND(),   WU_DBG_UNLOADEE, 0, 0);  // Give'em a change
        SendMessageNZ( GetFloatHWND(), WU_DBG_UNLOADEE, 0, 0);  // to Unload
        SendMessageNZ( GetLocalHWND(), WU_DBG_UNLOADEE, 0, 0);
        SendMessageNZ( GetWatchHWND(), WU_DBG_UNLOADEE, 0, 0);
        SendMessageNZ( GetCallsHWND(), WU_DBG_UNLOADEE, 0, 0);

        //
        //  Tell the  EE we are about to unloaded it.
        //

        EEUnload();

        //
        //  Unload the EE
        //

        FreeLibrary( HModEE );
        HModEE = 0;
        Ei.pStructExprAPI = &Exf;
    }

    if (HModSH != 0) {
        //
        //  Tell the symbol handler its about to be unloaded
        //

        SHUnloadSymbolHandler(FALSE);

        //
        //  Unload the symbol handler
        //

        FreeLibrary( HModSH );
        HModSH = 0;
        Lpshf = NULL;
    }

    EnableToolbarControls();

    memset( &CxfIp, 0, sizeof( CxfIp ) );
    SetCursor(hcursor);

    return;
}                           /* ClearDebuggee() */


void
DisconnectDebuggee(
    void
    )
/*++

Routine Description:

    This function is called to implement Run.Disconnect

Arguments:

    None.

Return Value:

    None.

--*/

{
    HCURSOR     hcursor;
    LPSTR       lpsz;
    LPPD        lppd;
    int         len;

    hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (DbgState == ds_init) {
        CmdLogVar(ERR_DbgState);
        return;
    }

    if (LppdFirst) {
        if (LppdFirst->hpds) {
            SHDeleteProcess(LppdFirst->hpds);
        }
        while (lppd = LppdFirst->lppdNext) {
            OSDDestroyHpid( lppd->hpid );
            DestroyPd( lppd, TRUE );
        }
        OSDDestroyHpid( HpidBase );
        DestroyPd( LppdFirst, TRUE );
    }

    len = ModListGetSearchPath( NULL, 0 );
    if (len) {
        lpsz = (PSTR) malloc(len);
        ModListGetSearchPath( lpsz, len );
        ModListInit();
        ModListSetSearchPath( lpsz );
        free( lpsz );
    }
    ModListAdd( NT_KERNEL_NAME, sheNone );

    SetIpid(0);
    LppdFirst   = NULL;
    LppdCur     = NULL;
    LptdCur     = NULL;

    if (HModTL != 0) {
        OSDDeleteTL( Htl );
        Htl = 0;
        FreeLibrary( HModTL );
        HModTL = 0;
    }

    if (HModEM != 0) {

        SendMessageNZ( GetCpuHWND(),   WU_DBG_UNLOADEM, 0, 0);  // Give'em a
        SendMessageNZ( GetFloatHWND(), WU_DBG_UNLOADEM, 0, 0);  // Chance

        OSDDeleteEM( Hem );
        FreeLibrary( HModEM );

        HModEM = 0;
        Hem = 0;
        HpidBase = 0;
        HtidBase = 0;
    }

    if (HModEE != 0) {

        SendMessageNZ( GetCpuHWND(),   WU_DBG_UNLOADEE, 0, 0);  // Give'em a change
        SendMessageNZ( GetFloatHWND(), WU_DBG_UNLOADEE, 0, 0);  // to Unload
        SendMessageNZ( GetLocalHWND(), WU_DBG_UNLOADEE, 0, 0);
        SendMessageNZ( GetWatchHWND(), WU_DBG_UNLOADEE, 0, 0);
        SendMessageNZ( GetCallsHWND(), WU_DBG_UNLOADEE, 0, 0);

        FreeLibrary( HModEE );
        HModEE = 0;
        Ei.pStructExprAPI = &Exf;
    }

    if (HModSH != 0) {
        LPFNSHSTOPBACKGROUND lpfn;
        LPFNSHUNINIT lpfn2;
        if (g_contWorkspace_WkSp.m_bShBackground) {
            lpfn = (LPFNSHSTOPBACKGROUND) GetProcAddress( HModSH, "SHStopBackground" );
            if (lpfn) {
                lpfn();
            }
        }
        lpfn2 = (LPFNSHUNINIT) GetProcAddress( HModSH, "SHUninit" );
        if (lpfn2) {
            lpfn2();
        }
        FreeLibrary( HModSH );
        HModSH = 0;
        Lpshf = NULL;
    }

    EnableToolbarControls();

    memset( &CxfIp, 0, sizeof( CxfIp ) );
    SetCursor(hcursor);

    return;
}                           /* DisconnectDebuggee() */


void
SetProcessExceptions(
    LPPD lppd
    )
{
    EXCEPTION_DESCRIPTION exd;
    EXCEPTION_LIST *List;

    //
    //  If we don't have a default exception list yet, load it.
    //
    if ( !DefaultExceptionList ) {

        //
        // Loop through all the exceptions known to OSDebug
        //
        exd.exc = exfFirst;
        OSDGetExceptionState(lppd->hpid, NULL, &exd);

        do {

            EXCEPTION_LIST *eList=
                               (EXCEPTION_LIST*)malloc(sizeof(EXCEPTION_LIST));
            Assert(eList);
            eList->next            = NULL;
            eList->dwExceptionCode = exd.dwExceptionCode;
            eList->efd             = exd.efd;
            eList->lpName          = MHStrdup(exd.rgchDescription);
            eList->lpCmd           = NULL;
            eList->lpCmd2          = NULL;

            InsertException( &DefaultExceptionList, eList );

            exd.exc = exfNext;

        } while (OSDGetExceptionState(lppd->hpid, NULL, &exd) == xosdNone);
    }

    if ( DefaultExceptionList ) {

        if ( lppd->ipid == 0 ) {

            //
            //  The exception list for process 0 is the default exception list.
            //
            lppd->exceptionList = DefaultExceptionList;

        } else {

            //
            //  All other processes get a copy of the default exception list.
            //
            List = DefaultExceptionList;

            while ( List ) {

                EXCEPTION_LIST *eList=(EXCEPTION_LIST*)malloc(sizeof(EXCEPTION_LIST));
                Assert(eList);
                eList->next            = NULL;
                eList->dwExceptionCode = List->dwExceptionCode;
                eList->efd             = List->efd;
                eList->lpName          = List->lpName ? MHStrdup(List->lpName) : NULL;
                eList->lpCmd           = List->lpCmd  ? MHStrdup(List->lpCmd)  : NULL;
                eList->lpCmd2          = List->lpCmd2 ? MHStrdup(List->lpCmd2) : NULL;

                InsertException( &lppd->exceptionList, eList);

                List = List->next;
            }
        }

        //
        //  Traverse the list and tell OSDebug not to ignore all exceptions
        //  that are not to be ignored (OSDebug will ignore all exceptions
        //  by default).
        //
        List = DefaultExceptionList;


        while ( List ) {

            exd.dwExceptionCode = List->dwExceptionCode;
            exd.efd = List->efd;
            exd.exc = exfSpecified;
            strncpy(exd.rgchDescription,
                    List->lpName? List->lpName: "",
                    EXCEPTION_STRING_SIZE);
            OSDSetExceptionState( lppd->hpid, NULL, &exd );

            List = List->next;
        }

        if (EfdPreset != -1) {
            exd.dwExceptionCode = 0;
            exd.efd = EfdPreset;
            exd.exc = exfDefault;
            OSDSetExceptionState(lppd->hpid, NULL, &exd);
        }
    }
}


void
ClearProcessExceptions(
    LPPD lppd
    )
{
    EXCEPTION_LIST *el, *elt;

    if ( lppd->ipid != 0 ) {

        //
        //  For all processes other than process 0, we must deallocate
        //  the list. Process 0 is special since its exception list is
        //  the default exception list, which does not go away.
        //
        for ( el = lppd->exceptionList; el; el = elt ) {

            elt = el->next;

            if ( el->lpName ) {
                MHFree( el->lpName );
            }

            if ( el->lpCmd ) {
                MHFree( el->lpCmd );
            }

            if ( el->lpCmd2 ) {
                MHFree( el->lpCmd2 );
            }

            MHFree(el);
        }
    }

    lppd->exceptionList = NULL;
}



VOID
GetDebugeePrompt(
    void
    )
{
    LPPROMPTMSG pm;
    DWORD dw;

    pm = (LPPROMPTMSG) malloc( sizeof(PROMPTMSG)+PROMPT_SIZE );
    if (!pm) {
        return;
    }
    memset( pm, 0, sizeof(PROMPTMSG)+PROMPT_SIZE );
    pm->len = PROMPT_SIZE;
    // kcarlos - BUGBUG -> BUGCAST
    //CmdGetDefaultPrompt( pm->szPrompt );
    CmdGetDefaultPrompt( (PSTR) pm->szPrompt );
    if (OSDSystemService( LppdCur->hpid,
                          NULL,
                          (SSVC) ssvcGetPrompt,
                          pm,
                          sizeof(PROMPTMSG)+PROMPT_SIZE,
                          &dw
                          ) == xosdNone) {
        // kcarlos - BUGBUG -> BUGCAST
        //CmdSetDefaultPrompt( pm->szPrompt );
        CmdSetDefaultPrompt( (PSTR) pm->szPrompt );
    }
    free( pm );
    return;
}


BOOL
ConnectDebugger(
    BOOL *pfReconnecting
    )
{
    //
    //  Check to see if we have already loaded an EM/DM pair and created
    //  the base process descriptor.  If not then we need to do so now.
    //
    char str[9];
    DWORD dw;

    *pfReconnecting = FALSE;

    if (LppdFirst == NULL) {

        if (!FLoadEmTl(pfReconnecting)) {
            return FALSE;
        }

        /*
         **  Hook this process up to the symbol handler
         */

        LppdCur->hpds = SHCreateProcess();
        SHSetHpid(HpidBase);

        if (g_contWorkspace_WkSp.m_bAlternateSS) {
            strcpy(str, "slowstep");
            OSDSystemService(LppdCur->hpid,
                             NULL,
                             (SSVC) ssvcCustomCommand,
                             str,
                             strlen(str)+1,
                             &dw
                             );
        }

    } else if (LppdCur == NULL) {

        LppdCur = LppdFirst;

    }

    SHChangeProcess(LppdCur->hpds);

    GetDebugeePrompt();

    return TRUE;
}                                   /* ConnectDebugger() */

int
FinishAttachDebuggee(
    DWORD   dwProcessId,
    HANDLE  hEventGo
    )
{
    XOSD    xosd;
    BOOL    fMakingRoot;
    MSG     msg;
    int     Errno;
    LPPD    lppd = NULL;
    BOOL    fTmp;
    DWORD   dwStatus = 0;
    DAP     Dap = { 0 };
    char    szStr[MAX_CMDLINE_TXT];

    OSDSetPath( LppdCur->hpid, g_contGlobalPreferences_WkSp.m_bSrchSysPathForExe, NULL );

    fMakingRoot = (LppdFirst->lppdNext == NULL
                && (LppdFirst->pstate == psNoProgLoaded));
    // not psDestroyed.  We don't restart proc 0 with an attach.

    if (fMakingRoot) {
        PCSTR pszProgramName = g_Windbg_WkSp.GetCurrentProgramName(FALSE);
        if (pszProgramName && *pszProgramName) {
            strcpy(szStr, pszProgramName);
            if (LpszCommandLine && *LpszCommandLine) {
                strcat(szStr, " ");
                strcat(szStr, LpszCommandLine);
            }
            if (!g_contWorkspace_WkSp.m_bKernelDebugger) {
                CmdLogVar(DBG_Losing_Command_Line, szStr);
                g_Windbg_WkSp.SetCurrentProgramName(pszProgramName);
            } else {
                g_Windbg_WkSp.SetCurrentProgramName(g_Windbg_WkSp.m_pszNoProgramLoaded);
            }
        }
    }

    Dap.dwProcessId = dwProcessId;
    Dap.hEventGo = hEventGo;

    xosd = OSDDebugActive(LppdFirst->hpid,
                          &Dap,
                          sizeof(Dap)
                          );

    if (xosd != xosdNone) {

        if (fMakingRoot) {
            DbgState = ds_error;
        }
        SetPTState(psNoProgLoaded, tsInvalidState);

        switch( xosd ) {

        case xosdAccessDenied:
            Errno = ERR_File_Read;
            break;

        case xosdOutOfMemory:
            Errno = ERR_Cannot_Allocate_Memory;
            break;

        default:
        case xosdBadFormat:
        case xosdUnknown:
            Errno = ERR_Cant_Load;
            break;

        }

        fTmp = SetAutoRunSuppress(TRUE);
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            ProcessQCQPMessage(&msg);
        }
        SetAutoRunSuppress(fTmp);

        sprintf(szStr, "process %d, status == %d", dwProcessId, dwStatus);
        ErrorBox(Errno, szStr);

        DbgState = ds_normal;

        if (fMakingRoot) {
            ClearDebuggee();
        }

        EnableToolbarControls();

        return FALSE;

    } else {

        /*
         *  Process messages until the loader breakpoint has been handled.
         */

        fTmp = SetAutoRunSuppress(TRUE);

        // wait for new process...
        if (fMakingRoot) {
            lppd = LppdFirst;
            lppd->pstate = psPreRunning;
        } else {
            HPID hpid = 0;
            while (GetMessage(&msg, NULL, 0, 0)) {
                ProcessQCQPMessage(&msg);
                if (msg.wParam == dbcNewProc) {
                    hpid = (HPID) msg.lParam;
                }
                if ((hpid != 0) &&
                    (lppd = LppdOfHpid((HPID) msg.lParam)) != NULL) {
                    break;
                }
            }
        }

        // wait for it to finish loading
        lppd->fChild     = FALSE;
        lppd->fHasRun    = FALSE;
        lppd->fInitialStep = FALSE;
        lppd->hbptSaved = NULL;
        lppd->fStopAtEntry = FALSE;

        while (GetMessage(&msg, NULL, 0, 0)) {
            ProcessQCQPMessage(&msg);
            if (lppd->pstate != psPreRunning) {
                break;
            }
        }

        SetAutoRunSuppress(fTmp);

        EnableToolbarControls();


        if (lppd->pstate != psRunning && lppd->pstate != psStopped) {
            return FALSE;
        } else {
            return TRUE;
        }
    }
}                                       /* FinishAttachDebuggee() */


BOOL
AttachDebuggee(
    DWORD   dwProcessId,
    HANDLE  hEventGo
    )
/*++

Routine Description:

    Debug an active process.  Tell osdebug to hook up to a
    running process.  If the process crashed, the system has
    provided an event handle to signal when the debugger is
    ready to field the second chance exception.


Arguments:


Return Value:


--*/
{
    BOOL    fReconnecting;

    /*
     **  Disable all of the buttons
     */

    EnableToolbarControls();

    //
    // connect the OSDebug components and wake up
    // the DM.
    //

    if (!ConnectDebugger(&fReconnecting)) {
        EnableToolbarControls();
        return FALSE;
    }

    if (fReconnecting) {
        FinishAttachDebuggee(0, NULL);
    }

    if (dwProcessId == 0) {
        return TRUE;
    } else {
        return FinishAttachDebuggee(dwProcessId, hEventGo);
    }
}



BOOL
RestartDebuggee(
    LPSTR ExeName,
    LPSTR Args
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    MSG     msg;
    int     Errno;
    XOSD    xosd;
    ULONG   ulFlags = 0;
    BOOL    fTmp;
    BOOL    fMakingRoot;
    LPPD    lppd = NULL;
    BOOL    fReconnecting;

    Assert(ExeName);

    /*
     **  Disable all of the buttons
     */

    EnableToolbarControls();

    if (!ConnectDebugger(&fReconnecting)) {
        EnableToolbarControls();
        return FALSE;
    }

    //
    // Allow the debugger to pick up any abandoned debuggees
    //
    if (fReconnecting) {
        if (FinishAttachDebuggee(0, NULL)) {
            Go();
        }
        return FALSE;
    }

    fMakingRoot = (LppdFirst->lppdNext == NULL) &&
                       ((LppdFirst->pstate == psNoProgLoaded) ||
                        (LppdFirst->pstate == psDestroyed));

    /*
     **  Mark as being in the original load so defer working with
     **  any breakpoints until all initial DLL load notifications are done
     */

    if (fMakingRoot) {
        DbgState = ds_init; //??
    }

    if (g_contWorkspace_WkSp.m_bDebugChildren) {
        ulFlags |= ulfMultiProcess;
    }

    if (g_contWorkspace_WkSp.m_bInheritHandles) {
        ulFlags |= ulfInheritHandles;
    }

    if (g_contWorkspace_WkSp.m_bWowVdm) {
        ulFlags |= ulfWowVdm;
    }

    if (AutoTest) {
        ulFlags |= ulfNoActivate;
    }

    OSDSetPath( LppdCur->hpid, g_contGlobalPreferences_WkSp.m_bSrchSysPathForExe, NULL );

    xosd = OSDProgramLoad(LppdCur->hpid,
                          ExeName,
                          Args,
                          NULL,
                          "WINDBG: ",
                          ulFlags
                          );

    if (xosd != xosdNone) {

        DbgState = ds_error;

        switch( xosd ) {
        case xosdFileNotFound:
            Errno = ERR_File_Not_Found;
            break;

        case xosdAccessDenied:
            Errno = ERR_File_Read;
            break;

        case xosdOpenFailed:
            Errno = ERR_File_Open;
            break;

        case xosdOutOfMemory:
            Errno = ERR_Cannot_Allocate_Memory;
            break;

        case xosdDumpInvalidFile:
            Errno = ERR_DumpInvalidFile;
            break;

        case xosdDumpWrongPlatform:
            Errno = ERR_DumpWrongPlatform;
            break;

        default:
        case xosdBadFormat:
        case xosdUnknown:
            Errno = ERR_Cant_Load;
            break;

        }

        ErrorBox(Errno, ExeName);

        DbgState = ds_normal;

        if (fMakingRoot) {
            ClearDebuggee();
        }

        EnableToolbarControls();
        InvalidateAllWindows();

        return FALSE;

    } else {

        /*
         *  Before draining message queue, ensure that thread state is right
         */


        fTmp = SetAutoRunSuppress(TRUE);

        // wait for new process...
        if (fMakingRoot) {

            //
            // we never see a dbcNewProc for this process,
            // so set things up here.
            //

            lppd = LppdFirst;
            lppd->pstate = psPreRunning;
            lppd->ctid    = 0;

            SetPdInfo(lppd);
            EESetTarget(lppd->mptProcessorType);

            DbgState = ds_normal;

        } else {

            //
            //  Process messages until the loader breakpoint has been handled,
            //  or the half started debuggee is killed.
            //

            HPID hpid = 0;
            while (GetMessage(&msg, NULL, 0, 0)) {
                ProcessQCQPMessage(&msg);
                if (msg.wParam == dbcNewProc) {
                    hpid = (HPID) msg.lParam;
                    DbgState = ds_normal;
                }
                if ((hpid != 0) &&
                    (lppd = LppdOfHpid((HPID) msg.lParam)) != NULL) {
                    break;
                }
            }
        }

        Assert(lppd);

        // wait for it to finish loading
        lppd->fChild     = FALSE;
        lppd->fHasRun    = FALSE;
        lppd->fInitialStep = FALSE;
        lppd->hbptSaved = NULL;
        lppd->fStopAtEntry = FALSE;

        while (GetMessage(&msg, NULL, 0, 0)) {
            ProcessQCQPMessage(&msg);
            if (lppd->pstate != psPreRunning) {
                break;
            }
        }

        //
        // do this again after dbcLoadComplete to pick up info
        // that wasn't available at first when debugging a crashdump...
        //
        SetPdInfo(lppd);

        SetAutoRunSuppress(fTmp);

        EnableToolbarControls();
        InvalidateAllWindows();

        if (g_contWorkspace_WkSp.m_bKernelDebugger &&
                 g_contKernelDbgPreferences_WkSp.m_bUseCrashDump) {
            //
            // lets be nice and do an automatic symbol reload
            //
            CmdExecuteLine( ".reload" );
        }

        return (LppdCur != NULL && LptdCur != NULL) &&
         (LptdCur->tstate == tsRunning || LptdCur->tstate == tsStopped ||
          LptdCur->tstate == tsException1 || LptdCur->tstate == tsException2);
    }
}                   /* RestartDebuggee() */


BOOL
DebuggeeAlive(
    void
    )
/*++

Routine Description:

    This function is used to determine if the debugger is currently
    active with a child process.

    See Also:
    DebuggeeActive()

Arguments:

    None.

Return Value:

    TRUE if there is currently a debuggee process and FALSE otherwise

--*/
{
    return GetLppdHead() != (LPPD)0;
}               /* DebuggeeAlive() */


BOOL
DebuggeeActive(
    void
    )
/*++

Routine Description:

    This function is used to determine if the debugger currently has
    a debuggee which is in a state where it is partially debugged.
    The difference between this and DebuggeeAlive is that if a debuggee
    has not been run or it has been terminated this will return FALSE
    while DebuggeeAlive will return TRUE.

    See Also:
    DebuggeeAlive

Arguments:

    None.

Return Value:

    TRUE if debuggee is in an active state and FALSE otherwise

--*/
{
    LPPD lppd;

    /*
     * If any process is loaded, we have a debuggee:
     */

    for (lppd = GetLppdHead(); lppd; lppd = lppd->lppdNext) {
        switch (lppd->pstate) {
          case psNoProgLoaded:
          case psExited:
          case psDestroyed:
          case psError:
            break;

          default:
            return TRUE;
        }
    }

    return FALSE;

}                   /* DebuggeeActive() */


/***    GetSourceFromAddress
**
**  Synopsis:
**  bool = GetSourceFromAddress(pADDR, SrcFname, SrcLen, pSrcLine)
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

BOOL
GetSourceFromAddress(
    PADDR   pADDR,
    PSTR    SrcFname,
    int     SrcLen,
    DWORD   *pSrcLine
    )
{
    ADDR    addr;
    DWORD   wLn;
    SHOFF   cbLn;
    SHOFF   dbLn;
    HSF     hsf;
    LPCH    lpchFname;
    CXT CXTT;
    char TmpSrcFname[_MAX_PATH];
    char Executable[_MAX_PATH];

    AuxPrintf(1, "GetSourceFromAddress - 0x%X:0x%I64X, emi:%X",
      GetAddrSeg(*pADDR),
      GetAddrOff(*pADDR),
      emiAddr(*pADDR)
      );

    if ( ! ADDR_IS_LI ( *pADDR ) ) {
        SYUnFixupAddr ( pADDR );
    }

    addr = *pADDR;

    SHHMODFrompCXT(&CXTT) = (HMOD) NULL;

    /*
    ** Translate the given addresss into a file and line.
    */

    SHSetCxt(pADDR, &CXTT);

    if (SHHMODFrompCXT(&CXTT)
      && SLLineFromAddr (&addr, &wLn, &cbLn, &dbLn)
      && (hsf = SLHsfFromPcxt (&CXTT)) ) {

        // Canonicalise the found file relative to the ProgramName

        lpchFname = SLNameFromHsf (hsf);
#if 0
        Assert(SrcLen > 0);
        _fmemcpy ( SrcFname, lpchFname + 1, min(SrcLen-1, *lpchFname));
        SrcFname[*lpchFname] = '\0';
#else
        _fmemcpy ( TmpSrcFname, lpchFname + 1, *lpchFname);
        TmpSrcFname[*lpchFname] = '\0';
        GetExecutableFilename(Executable, sizeof(Executable));
        BuildRelativeFilename(TmpSrcFname,
                              Executable,
                              SrcFname,
                              SrcLen);
#endif
        /// M00HACK
        {
            char * lpch = SrcFname + strlen(SrcFname);
            while (lpch > SrcFname && *lpch != '.'){
                lpch = CharPrev(SrcFname, lpch);
            }
            if (_stricmp(lpch, ".OBJ") == 0) {
                strcpy(lpch, ".C");
            }
        }
        /// M00HACK
        *pSrcLine = wLn;
        return TRUE;
    }

    return FALSE;
}                   /* GetSourceFromAddress() */

/***    GetCurrentSource
**
**  Synopsis:
**  bool = GetCurrentSource(SrcFname, SrcLen, pSrcLine)
**
**  Entry:
**  SrcFname :
**  SrcLen   :
**  pSrcLine :
**
**  Returns:
**
**  Description:
**
*/


BOOL
GetCurrentSource(
    PSTR SrcFname,
    int SrcLen,
    DWORD *pSrcLine
    )
{
    ADDR    addr;

    OSDGetAddr(LppdCur->hpid, LptdCur->htid, adrPC, &addr);
    return GetSourceFromAddress(&addr, SrcFname, SrcLen, pSrcLine);
}


/***    MoveEditorToAddr
**
**  Synopsis:
**  bool = MoveEditorToAddr(pAddr)
**
**  Entry:
**  pAddr - Address to move the editor to
**  bUserActivated - Indicates whether this action was initiated by the
**      user or by windbg. The value is to determine the Z order of
**      any windows that are opened.
**
**  Returns:
**  TRUE if successful and FALSE otherwise
**
**  Description:
**  This function will take the address in the structure pAddr and
**  move the editor so that the source line cooresponding to the address
**  will be in the front window and the cursor placed on that line
*/

BOOL
PASCAL
MoveEditorToAddr(
    PADDR   pAddr,
    BOOL    bUserActivated
    )
{
    char    szFname[_MAX_PATH];
    DWORD   FnameLine;
    int     doc = 0;
    BOOL    GotSource;

    if (!pAddr) {
        return FALSE;
    }

    GotSource = FALSE;
    if (GetSourceFromAddress( pAddr, szFname, sizeof(szFname), &FnameLine)) {
        GotSource = SrcMapSourceFilename(szFname, sizeof(szFname),
            SRC_MAP_OPEN, NULL, bUserActivated);

        switch( GotSource ) {
        case -2:
            return FALSE;

        case -1:
            return FALSE;

        case 0:
            return FALSE;

        case 1:
        case 2:
            GotSource = FindDoc(szFname, &doc, TRUE);
            break;

        default:
            Assert(FALSE);
            break;
        }

        if (!GotSource) {
            GotSource = ((AddFile(MODE_OPEN, DOC_WIN, (LPSTR)szFname, NULL, NULL, TRUE, -1, -1, bUserActivated) != -1) &&
                         FindDoc(szFname, &doc, bUserActivated));
        }
    }

    if (GotSource) {
        // Make sure window is visible
        ActivateMDIChild(Views[Docs[doc].FirstView].hwndFrame, TRUE);

        // And show the function
        GotoLine(Docs[doc].FirstView, FnameLine, TRUE);
    }

    return GotSource;
}                   /* MoveEditorToAddr() */

BOOL
ToggleInRange(
    int     thisView,
    DWORD   dwLineNumber,
    DWORD   Lines,
    BOOL    LoadSymbols,
    HBPT    hBpt,
    char   *rgch
    )
{
    BOOL        BpSet = FALSE;
    char        szCurLine[300];
    BPSTATUS    bpstatus;
    ULONG       LineBpt;
    BOOL        OldLoadSymbols;

    OldLoadSymbols = BPSymbolLoading( LoadSymbols );


    //
    //  See if there is a breakpoint on this file/line pair already.
    //  Note that we only check for existing breakpoints in the first
    //  line of the range, so that we'll toggle (i.e. clear) existing
    //  breakpoints only if clicking over a highlighted line.
    //
    bpstatus = BPHbptFromFileLine(rgch, dwLineNumber, &hBpt);
    if (bpstatus == BPNOERROR || bpstatus == BPAmbigous) {

        BPDelete(hBpt);
        BPCommit();
        BpSet = TRUE;

    } else {

        //
        // set a breakpoint on the next line that has code associated with it.
        //

        Assert(bpstatus == BPNoBreakpoint);

        for (; dwLineNumber < Lines; dwLineNumber++) {

            bpstatus = BPHbptFromFileLine(rgch, dwLineNumber, &hBpt);

            if (bpstatus == BPNOERROR || bpstatus == BPAmbigous) {
                // This line already has a breakpoint, place the cursor on this
                //  line, do nothing more
                if (BPQueryHighlightLineOfHbpt(hBpt, &LineBpt) == BPNOERROR) {
                    dwLineNumber = (DWORD)LineBpt;
                }
                PosXYCenter(thisView, Views[thisView].X, dwLineNumber-1, FALSE);
                BpSet = TRUE;
                break;
            }

            //
            // Set up a breakpoint node and a command string for the
            // current line
            // Make a current line BP command
            //
            sprintf(szCurLine, "%c%d", BP_LINELEADER, dwLineNumber);

            bpstatus = BPParse(&hBpt, szCurLine, NULL, rgch,
                LppdCur ? LppdCur->hpid : 0 );

            if (bpstatus == BPNOERROR) {

                Dbg(BPAddToList( hBpt, -1 ) == BPNOERROR);

                if (DebuggeeAlive()) {

                    bpstatus = BPBindHbpt( hBpt, NULL );

                    if (bpstatus != BPNOERROR) {
                        //
                        //  If this file is in a module that we have loaded,
                        //      then the line is not valid, so we discard the
                        //      breakpoint. Otherwise this file might belong
                        //      to a module that has not been loaded yet, so
                        //      we leave the uninstantiated BP to be resolved
                        //      later on.
                        //

                        if ( BPFileNameToMod( rgch ) || !LoadSymbols ) {
                            BPUnCommit();
                            if ( bpstatus != BPCancel ) {
                                continue;
                            }
                        }
                    }
                }
                BPCommit();
                if (BPQueryHighlightLineOfHbpt(hBpt, &LineBpt) == BPNOERROR) {
                    dwLineNumber = (DWORD)LineBpt;
                }
                PosXYCenter(thisView, Views[thisView].X, dwLineNumber-1, FALSE);
                BpSet = TRUE;
                break;
            }
        }
    }

    BPSymbolLoading( OldLoadSymbols );

    return BpSet;
}                                               /* ToggleInRange() */



/***    ToggleLocBP
**
**  Synopsis:
**  bool = ToggleLocBP()
**
**  Entry:
**  None
**
**  Returns:
**  TRUE if successful, FALSE otherwise.
**
**  Description:
**  Toggles the breakpoint at the current editor line.
**
*/

BOOL PASCAL
ToggleLocBP(
    void
    )
{
    char        szCurLine[300];
    char        rgch[300];
    HBPT        hBpt = NULL;
    BPSTATUS    bpstatus;
    ADDR        addr;
    ADDR        addr2;
    DWORD       dwLineNumber;
    int         thisView = curView;


    //
    // can't do this if windbg is the kernel debugger and the system is running
    //
    if ( g_contWorkspace_WkSp.m_bKernelDebugger && IsProcRunning(LppdCur) ) {
        CmdInsertInit();
        CmdLogFmt( "Cannot set breakpoints while the target system is running\r\n" );
        MessageBeep(0);
        return FALSE;
    }

    //
    // check first that a window is active
    //
    if (hwndActiveEdit == NULL) {
        return FALSE;
    }

    //
    // Could be the disassembler window
    //

    if (Views[thisView].Doc < 0) {

        //
        //  Must be in a src or disasm window to do this
        //
        return FALSE;

    } else if (Docs[Views[thisView].Doc].docType == DISASM_WIN) {

        if (!DisasmGetAddrFromLine(&addr, Views[thisView].Y)) {
            return FALSE;
        }

        //
        //   Check to see if breakpoint already at this address
        //
        addr2 = addr;
        bpstatus = BPHbptFromAddr(&addr2, &hBpt);
        if ((bpstatus == BPNOERROR) || (bpstatus == BPAmbigous)) {
            BPDelete(hBpt);
            BPCommit();
            return TRUE;
        }

        Assert( bpstatus == BPNoBreakpoint );

        EEFormatAddress(&addr, szCurLine, sizeof(szCurLine), 0);

        bpstatus =
            BPParse( &hBpt, szCurLine, NULL, NULL, LppdCur ? LppdCur->hpid : 0);

        if (bpstatus != BPNOERROR) {
            return FALSE;
        } else {

            Dbg(BPAddToList( hBpt, -1) == BPNOERROR);
            Assert(DebuggeeAlive());
            bpstatus = BPBindHbpt( hBpt, NULL );

            if (bpstatus != BPNOERROR) {
                BPUnCommit();
                return FALSE;
            }
            BPCommit();
            return TRUE;
        }

    } else if (Docs[Views[thisView].Doc].docType == DOC_WIN) {

        //
        //  Ok to do this in source win
        //

        //
        // Deal with any mapping of file names
        //
        strcpy(rgch, Docs[Views[thisView].Doc].szFileName);
        SrcBackMapSourceFilename(rgch, sizeof(rgch));

        // set the line number to the current line (where the caret is)
        dwLineNumber = Views[thisView].Y+1;

        if ( ToggleInRange(
            thisView,
            dwLineNumber,
            min( (DWORD)Docs[Views[thisView].Doc].NbLines+1, dwLineNumber+20 ),
            FALSE,
            hBpt,
            rgch ) ) {

            return TRUE;

        }

        if ( ToggleInRange(
            thisView,
            dwLineNumber,
            (DWORD)Docs[Views[thisView].Doc].NbLines+1,
            TRUE,
            hBpt,
            rgch ) ) {

            return TRUE;
        }
    }

    return FALSE;
}                               /* ToggleLocBP() */


/***    ContinueToCursor
**
**  Synopsis:
**  bool = ContinueToCursor()
**
**  Entry:
**  Nothing
**
**  Returns:
**  TRUE on success and FALSE on failure
**
**  Description:
**  Attemps to do a GoUntil to the address that corresponds to the
**  source line at the current cursor position in the editor
*/

BOOL PASCAL
ContinueToCursor(
    int     View,
    int     line
    )
{
    char        szCurLine[255];
    char        rgch[300];
    HBPT        hBpt;
    ADDR        addr;
    BPSTATUS    bpstatus;

    //
    // Check for active window
    //
    if (hwndActiveEdit == NULL) {
        return FALSE;
    }

    //
    //  If we get here then the debuggee must be alive.  If it is not
    //  also active then return FALSE.  We can't do anything in that case
    //
    Assert( DebuggeeAlive() );

    //
    // Check first for a disassembler window.
    //

    if (Views[View].Doc < 0) {
        //
        // Can't do this in a pane window
        //
        return FALSE;
    } else if (Docs[Views[View].Doc].docType == DISASM_WIN) {
        if (DisasmGetAddrFromLine(&addr, line)) {
            GoUntil(&addr);
            return TRUE;
        } else {
            return FALSE;
        }
    } else if (Docs[Views[View].Doc].docType != DOC_WIN) {
        //
        //  Must be in a source window in order to do this
        //
        return FALSE;
    }

    if (!DebuggeeActive()) {
        return FALSE;
    }

    sprintf(szCurLine, "%c%d", BP_LINELEADER, line + 1);

    //
    // Back map the current file name to the original file name
    //  if it has been changed
    //
    strcpy(rgch, Docs[Views[View].Doc].szFileName);
    SrcBackMapSourceFilename(rgch, sizeof(rgch));

    bpstatus = BPParse(&hBpt,
                       szCurLine,
                       NULL,
                       rgch,
                       LppdCur ? LppdCur->hpid : 0);
    if (bpstatus != BPNOERROR) {

        return FALSE;

    } else if ((bpstatus = BPBindHbpt( hBpt, &CxfIp )) == BPNOERROR) {

        Dbg( BPAddrFromHbpt( hBpt, &addr ) == BPNOERROR );
        Dbg( BPFreeHbpt( hBpt ) == BPNOERROR );
        GoUntil( &addr );

    } else if (!LppdCur->fHasRun) {

        // not yet at entrypoint: save it for later
        LppdCur->hbptSaved = (HANDLE)hBpt;
        Go();

    } else {

        // it should have bound
        Dbg( BPFreeHbpt( hBpt ) == BPNOERROR );
        return FALSE;
    }

    return TRUE;
}                   /* ContinueToCursor() */


void PASCAL
UpdateRadix(
    UINT newradix
    )
/*++

Routine Description:

    Change the global radix and update any windows that need to track radix.

Arguments:

    newradix  - Supplies new radix value: 8, 10 or 16

Return Value:

    None

--*/
{
    Assert(newradix == 8 || newradix == 10 || newradix == 16);

    if (newradix != radix) {
        radix = newradix;
        UpdateDebuggerState(UPDATE_RADIX);
    }

    return;
}                   /* UpdateRadix() */


int
GetQueueLength()
{
    int i;


    EnterCriticalSection(&csDbgMsgBuffer);

     //  OK so its not the length -- all I currently really care about
     //  is if there are any messages setting and waiting for me.

    i = iDbgMsgBufferFront - iDbgMsgBufferBack;
    LeaveCriticalSection(&csDbgMsgBuffer);
    return i;
}                               /* GetQueueLength() */


void
BPCallbackFunc(
    HBPT hBpt,
    BPSTATUS bpstatus
    )
/*++

Routine Description:

    This function will be called for each breakpoint which
    mets its criteria

Arguments:

    hBpt     - Supplies breakpoint which met the breakpoint criteria

    bpstatus - Supplies breakpoint status code during evaluation

Return Value:

    None

--*/
{
    ULONG       i;
    char        rgchT[256];

    if (bpstatus != BPNOERROR) {
        Dbg( BPIFromHbpt( &i, hBpt ) == BPNOERROR);

        CmdInsertInit();
        CmdLogFmt("Error checking breakpoint #%d\r\n", i);
    }

    return;
}                                       /* BPCallbackFunc() */


BOOL
AddQueueItemQuad(
    DWORD64 l
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    int         newBufferFront;
    BOOL        fEmpty;

    EnterCriticalSection(&csDbgMsgBuffer);

    newBufferFront = (iDbgMsgBufferFront + 1) % QUEUE_SIZE;

    while (newBufferFront == iDbgMsgBufferBack) {
        LeaveCriticalSection(&csDbgMsgBuffer);
        Sleep(1000);
        EnterCriticalSection(&csDbgMsgBuffer);
    }

    fEmpty = (GetQueueLength() == 0);
    RgbDbgMsgBuffer[iDbgMsgBufferFront] = l;

    iDbgMsgBufferFront = newBufferFront;

    LeaveCriticalSection(&csDbgMsgBuffer);

    return fEmpty;
}


DWORD64
GetQueueItemQuad(
    void
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD64    l;

    EnterCriticalSection(&csDbgMsgBuffer);

    while (iDbgMsgBufferFront == iDbgMsgBufferBack) {
        LeaveCriticalSection(&csDbgMsgBuffer);
        Sleep(1000);
        EnterCriticalSection(&csDbgMsgBuffer);
    }

    l = RgbDbgMsgBuffer[iDbgMsgBufferBack];

    iDbgMsgBufferBack = (iDbgMsgBufferBack + 1) % QUEUE_SIZE;

    LeaveCriticalSection(&csDbgMsgBuffer);

    return l;
}


LPTD
CBLptdOfHpidHtid(
    HPID hpid,
    HTID htid
    )
/*++

Routine Description:

    This is a specialized debugger version of this function.  It should
    only be called from the callback routine.  It will return a thread
    pointer if one can be found and otherwise will push the process
    and thread handles onto the save memory area.

Arguments:

    hpid    - osdebug handle to a process
    htid    - osdebug handle to a thread

Return Value:

    pointer to thread descriptor block if one exists for the thread
    and process handles.  Otherwise it will return NULL.

--*/
{
    LPPD    lppd;
    LPTD    lptd = NULL;

    lppd = LppdOfHpid(hpid);
    if (lppd) {
        lptd = LptdOfLppdHtid( lppd, htid );
    }
    AddQueueItemQuad((DWORD64) lptd);
    if (lptd == NULL) {
        AddQueueItemQuad((DWORD64) hpid);
        AddQueueItemQuad((DWORD64) htid);
    }

    return( lptd );
}                   /* CBLptdOfHpidHtid() */


XOSD
OSDAPI
OSDCallbackFunc(
    DWORD wMsg,
    HPID hpid,
    HTID htid,
    DWORD64 wParam,
    DWORD64 lParam
    )
/*++

Routine Description:

    This function posts a message to the main message pump to be
    processed in CmdNextExec.  All of the relevent data is posted
    as part of the message.  This must be done for the Win16 case
    as things such as memory allocation are not good here since this
    could called be non-syncronous.

Arguments:

    wMsg  - Callback message number (dbc*)

    hpid  - process ID for the message

    htid  - thread ID for the message

    wParam - Data about the message

    lParam - Data about the message

Return Value:

    xosdNone - never returns an error

--*/
{
    BOOL        fPostMsg = FALSE;

    LPBPR       lpbpr;
    LPINFOAVAIL lpinf;
    LPADDR      lpaddr;
    HSYM        hSym;
    UOFF32      uoff;
    CXF         Cxf;
    CANSTEP     *CanStep;
    FUNCTION_INFORMATION FunctionInfo;
    ADDR        Addr;
    DWORD       wLn;
    SHOFF       cbLn;
    SHOFF       dbLn;
    HPDS        HpdsOld;
    BPSTATUS    bpstatus;
    int         cch;


    switch ((DBC)wMsg) {
    case dbcInfoAvail:
    case dbcInfoReq:
        lpinf = (LPINFOAVAIL)lParam;
        fPostMsg = AddQueueItemQuad( wMsg );
        if ( (DBC)wMsg == dbcInfoAvail ) {
            AddQueueItemQuad( lpinf->fReply );
        } else {
            Assert( lpinf->fReply );
        }

        AddQueueItemQuad( lpinf->fUniCode );
        if (!lpinf->fUniCode) {
            AddQueueItemQuad( (DWORD64) MHStrdup( (PSTR) lpinf->buffer) );
        } else {
            int     l = lstrlenW( (LPWSTR) lpinf->buffer);
            LPWSTR  lpw = (LPWSTR) MHAlloc(2*l + 2);
            memcpy(lpw, lpinf->buffer, 2*l);
            lpw[l] = 0;
            AddQueueItemQuad( (DWORD64) lpw );
        }
        break;

    case dbcCheckWatchPoint:
    case dbcCheckMsgBpt:
    case dbcCheckBpt:
        //
        //  Do the check here rather than queueing it for the shell.
        //
        //
        // if the BP fires, the DM will notify us,
        // so don't tell the shell anything.
        //

        OSDGetAddr(hpid, htid, adrPC, &Addr);
        if ( ! ADDR_IS_LI ( Addr ) ) {
            SYUnFixupAddr ( &Addr );
        }
        memset(&Cxf, 0, sizeof(Cxf));

        HpdsOld = SHChangeProcess(LppdOfHpid(hpid)->hpds);
        SHSetCxt(&Addr, SHpCxtFrompCxf( &Cxf ) );
        SHhFrameFrompCXF(&Cxf) = (HFRAME) htid;
        bpstatus = BPCheckHbpt( Cxf, BPCallbackFunc, hpid, htid, wParam);
        SHChangeProcess(HpdsOld);

        Assert(bpstatus == BPPassBreakpoint || bpstatus == BPNOERROR);

        return (bpstatus == BPNOERROR);

    case dbcStep:

    case dbcBpt:
    case dbcSendBpt:

    case dbcWatchPoint:
    case dbcSendWatchPoint:

    case dbcMsgBpt:
    case dbcSendMsgBpt:

    case dbcAsyncStop:
    case dbcEntryPoint:
    case dbcLoadComplete:

        fPostMsg = AddQueueItemQuad( wMsg );
        CBLptdOfHpidHtid( hpid, htid );

        //
        // wParam contains the Breakpoint notify tag when relevant
        //
        AddQueueItemQuad(wParam);

        break;

    case dbcProcTerm:
        fPostMsg = AddQueueItemQuad( wMsg );
        AddQueueItemQuad( (DWORD64) hpid );
        AddQueueItemQuad((DWORD64) lParam);    // Save away exit code
        break;

    case dbcDeleteProc:
        fPostMsg = AddQueueItemQuad( wMsg );
        AddQueueItemQuad( (DWORD64) hpid );
        break;

    case dbcModFree:
        fPostMsg = AddQueueItemQuad( wMsg );
        CBLptdOfHpidHtid( hpid, htid );
        AddQueueItemQuad((DWORD64) lParam);
        break;

    case dbcModLoad:
        fPostMsg = AddQueueItemQuad( wMsg );
        CBLptdOfHpidHtid( hpid, htid );
        AddQueueItemQuad((DWORD64) MHStrdup((LSZ)lParam));    // remember name
        AddQueueItemQuad((DWORD64) wParam);    // TRUE if thread is stopped
        break;

    case dbcError:
        fPostMsg = AddQueueItemQuad( wMsg );
        AddQueueItemQuad((DWORD64) hpid);
        AddQueueItemQuad((DWORD64) htid);
        AddQueueItemQuad((DWORD64) wParam);
        AddQueueItemQuad((DWORD64)MHStrdup((LSZ) lParam));
        break;

    case dbcCanStep:
        lpaddr = (LPADDR) lParam;

        if ( (HPID)emiAddr( *lpaddr ) == hpid && ADDR_IS_LI(*lpaddr) ) {

            //
            //  This is magic, don't ask.
            //
            emiAddr( *lpaddr )    = 0;
            ADDR_IS_LI( *lpaddr ) = FALSE;

            OSDSetEmi(hpid,htid,lpaddr);
        }
        if ( (HPID)emiAddr( *lpaddr ) != hpid ) {
            SHWantSymbols( (HEXE)emiAddr( *lpaddr ) );
        }

        SHSetCxt( lpaddr, SHpCxtFrompCxf( &Cxf ) );

        CanStep = (CANSTEP*)lParam;

        uoff = SHGetNearestHSYM( lpaddr, Cxf.cxt.hMod , EECODE, &hSym );

        if ( uoff != CV_MAXOFFSET && SHIsThunk( hSym ) ) {

            CanStep->Flags = CANSTEP_THUNK;

        } else if (uoff == CV_MAXOFFSET &&
                        !SLLineFromAddr( lpaddr, &wLn, NULL, NULL)) {

            CanStep->Flags = CANSTEP_NO;

        } else {

            uoff = 0;
            Addr = Cxf.cxt.addr;
            SYFixupAddr( &Addr );

            //
            // v-vadimp - always use sapi's IsInProlog on IA64
            //
            if (LppdOfHpid(hpid)->mptProcessorType != mptia64 &&
                OSDGetFunctionInformation( hpid,
                                           NULL,
                                           &Addr,
                                           &FunctionInfo )
                    == xosdNone)
            {
                if (GetAddrOff(FunctionInfo.AddrPrologEnd) > GetAddrOff(Addr)) {
                    uoff = (UOFF32)(GetAddrOff(FunctionInfo.AddrPrologEnd) - GetAddrOff(Addr));
                }
            } else if ( GetSrcMode_StatusBar() ) {
                while( SHIsInProlog( &Cxf.cxt ) ) {
                    Cxf.cxt.addr.addr.off++;
                    uoff++;
                }
            }

            CanStep->Flags        = CANSTEP_YES;
            CanStep->PrologOffset = uoff;

        }

        return xosdNone;

    case dbcLastAddr:

        //
        //  We will return:
        //
        //  If SRC mode and have src for line - Last addr in line
        //  If SRC mode and no src for line   - Zero
        //  if ASM mode                       - Same addr
        //
        lpaddr = (LPADDR) lParam;

        if ( GetSrcMode_StatusBar() ) {

            if ( SLLineFromAddr( lpaddr, &wLn, &cbLn, &dbLn ) ) {

                Assert( cbLn >= dbLn );
                if (cbLn >= dbLn) {
                    lpaddr->addr.off += cbLn - dbLn;
                }
            } else {
                memset( lpaddr, 0, sizeof( ADDR ) );
            }
        }

        return xosdNone;


    case dbcFlipScreen:
        return xosdNone;

    case dbcException:
        {
            LPEPR lpepr;
            DWORD size = sizeof(EPR) + ((LPEPR)lParam)->NumberParameters * sizeof(DWORD);
            fPostMsg = AddQueueItemQuad( wMsg );
            CBLptdOfHpidHtid( hpid, htid);
            lpepr = (LPEPR) MHAlloc(size);
            memcpy(lpepr, (PVOID)lParam, size);
            AddQueueItemQuad((DWORD64)lpepr);
        }
        break;

    case dbcThreadTerm:
        fPostMsg = AddQueueItemQuad( wMsg );
        CBLptdOfHpidHtid( hpid, htid );
        AddQueueItemQuad((DWORD64) lParam);    // Save away exit code
        break;

    case dbcExecuteDone:
        AddQueueItemQuad( wMsg );
        PostMessage(hwndFrame, DBG_REFRESH, dbcExecuteDone, xosdNone);
        return xosdNone;

    case dbcNewProc:
        //
        // hpid belongs to root proc; new hpid is in wParam.
        //
        AddQueueItemQuad( wMsg );
        AddQueueItemQuad( wParam);
        PostMessage(hwndFrame, DBG_REFRESH, dbcNewProc, (LPARAM)wParam);
        return xosdNone;

    case (DBC)dbcRemoteQuit:
        fPostMsg = AddQueueItemQuad( wMsg );
        break;

    case (DBC)dbcMemoryChanged:
        fPostMsg = AddQueueItemQuad( wMsg );
        AddQueueItemQuad( lParam );
        break;

    case dbcSegLoad:
        fPostMsg = AddQueueItemQuad( wMsg );
        CBLptdOfHpidHtid( hpid, htid) ;
        AddQueueItemQuad(lParam);
        break;

    case dbcCreateThread:
        fPostMsg = AddQueueItemQuad( wMsg );
        CBLptdOfHpidHtid( hpid, htid);
        break;

    case dbcExitedFunction:
        //
        // if we are going to use this, we must read the return value now,
        // while the DM is blocked.  If the shell reads it, the thread will
        // already be running.
        //
        fPostMsg = AddQueueItemQuad( wMsg );
        CBLptdOfHpidHtid( hpid, htid);
        AddQueueItemQuad( 0 );   // <<-- return value goes here
        break;

    case dbcGetExpression:
        //
        // Evaluate an expression using the masm evaluator
        //
        lpaddr = (LPADDR)wParam;
        if (CPGetAddress((LPSTR)lParam,
                          &cch,
                          lpaddr,
                          /*radix*/10,
                          &CxfIp,
                          /*fCaseSensitive*/FALSE,
                          /*fSpecial*/TRUE) != 0) {
            lpaddr->addr.off = 0;
        }
        break;

    default:
        fPostMsg = AddQueueItemQuad( wMsg );
        CBLptdOfHpidHtid( hpid, htid);
        break;
    }

    if (fPostMsg) {
        PostMessage(hwndFrame, DBG_REFRESH, 0, 0);
    }
    return (xosdNone);
}                   /* OSDCallbackFunc() */


/***    FDebInit
**
**  Synopsis:
**  bool = FDebInit()
**
**  Entry:
**  none
**
**  Returns:
**  TRUE if the debugger was initialized successfully and FALSE
**  if the debugger failed to initialize
**
**  Description:
**  Initialize the OSDebug debugger.
**
*/


BOOL
FDebInit(
    void
    )
{

    if (OSDInit(&Dbf) != xosdNone) {
        return (FALSE);         /* Error during initialization  */
    }

    BPInit();

    SetPidTid_StatusBar(NULL, NULL);

    return (TRUE);
}                   /* FDebInit() */

/***    FDebTerm
**
**  Synopsis:
**  bool = FDebTerm()
**
**  Entry:
**  None
**
**  Returns:
**  TRUE if successful and FALSE otherwise
**
**  Description:
**  This routine will go back and do any clean up necessary to
**  kill all of the debugger dlls, debuggee processes and so forth.
*/

BOOL
FDebTerm(
    void
    )
{
    ClearDebuggee();
    OSDTerm();
    return TRUE;
}                   /* FDebTerm() */



BOOL
DllVersionMatch(
    HINSTANCE hMod,
    PCSTR  pName,
    PCSTR  pType,
    BOOL   fNoisy
    )
{
    DBGVERSIONPROC  pVerProc;
    BOOL            Ok = TRUE;
    LPAVS           pavs;

    pVerProc = (DBGVERSIONPROC)GetProcAddress(hMod, DBGVERSIONPROCNAME);
    if (!pVerProc) {

        Ok = FALSE;
        if (fNoisy) {
            ErrorBox(ERR_Not_Windbg_DLL, pName);
        }

    } else {

        pavs = (*pVerProc)();

        if (pType[0] != pavs->rgchType[0] || pType[1] != pavs->rgchType[1]) {

            Ok = FALSE;
            if (fNoisy) {
                ErrorBox(ERR_Wrong_DLL_Type,
                 pName, (LPSTR)pavs->rgchType, (LPSTR)pType);
            }

        } else if (Avs.rlvt != pavs->rlvt) {

            Ok = FALSE;
            if (fNoisy) {
                ErrorBox(ERR_Wrong_DLL_Version, pName,
                    pavs->rlvt, pavs->iRmj, Avs.rlvt, Avs.iRmj);
            }

        } else if (Avs.iRmj != pavs->iRmj) {

            Ok = FALSE;
            if (fNoisy) {
                ErrorBox(ERR_Wrong_DLL_Version, pName,
                    pavs->rlvt, pavs->iRmj, Avs.rlvt, Avs.iRmj);
            }

        }

    }

    return Ok;
}



HINSTANCE
LoadHelperDll(
    PCTSTR psz,
    PCTSTR pType,
    BOOL  fError
    )
/*++

Routine Description:

    Load a debugger DLL, verify that it is the correct type
    and version.

Arguments:

    psz    - Supplies string contianing name of DLL to be loaded
    pType  - Supplies type string
    fError - Supplies flag saying whether to display an error message
             on failure.

Returns:

    HMODULE to library or NULL

--*/
{
    HINSTANCE hMod;
    BOOL    fail = FALSE;

    hMod = LoadLibrary(psz);

    if (hMod == NULL) {
        fail = TRUE;
    }
    else if (!DllVersionMatch( hMod, psz, pType, fError ) ) {
        FreeLibrary( hMod );
        hMod = NULL;
        fail = TRUE;
    }

    if (fail && fError) {
        PTSTR pszErr = WKSP_FormatLastErrorMessage();
        ErrorBox(ERR_Cannot_Load_DLL, (LPSTR) psz, pszErr);
        free(pszErr);
    }

    return hMod;
}                   /* LoadHelperDll() */

/***    FLoadShEe
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

BOOL
FloadShEe(
    DWORD mach
    )
{
    LPFNSHINIT  lpfn2;
    LPFNEEINIT  lpfn3;
    PCSTR       pszDll;
    char *      szDllEEAuto;
    HCURSOR     hcursor;
    int         nErrno;
    
    
    CmdInsertInit();
    
    /*
    **  Place the hour glass icon onto the screen
    */
    
    hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    /*
    **
    */
    
    if (Lpshf == NULL) {
        pszDll = g_pszDLL_SYMBOL_HANDLER;
        Assert(pszDll);
        
        if ((HModSH = LoadHelperDll(pszDll, "SH", TRUE)) == 0) {
            SetCursor(hcursor);
            return( FALSE );
        }
        
        if ((lpfn2 = (LPFNSHINIT) GetProcAddress(HModSH, "SHInit")) == NULL) {
            nErrno = ERR_Invalid_Debugger_Dll;
            goto errRet;
        }
        
        if (lpfn2(&Lpshf, &Knf) == FALSE) {
            nErrno = ERR_Initializing_Debugger;
            goto errRet;
        }
    }
    
    pszDll = g_pszDLL_EXPR_EVAL;
    Assert(pszDll);
    
    /*
    **  Check for either a space or a tab following the name of the DLL
    **  if so then replace that character with a 0 and ignore the rest
    **  of the string
    */

    Assert(NULL == strchr( (PBYTE) pszDll, ' '));
    Assert(NULL == strchr( (PBYTE) pszDll, '\t'));
    
#if 0
    if ((lpch = (PSTR) strchr( (PBYTE) pszDll, ' ')) == NULL) {
        lpch = (PSTR) strchr( (PBYTE) pszDll, '\t');
    }
    
    if (lpch != NULL) {
        chSave = *lpch;
        *lpch = 0;
    }
#endif

    if ((HModEE = LoadHelperDll(pszDll, "EE", TRUE)) == 0) {
#if 0
        if (lpch != NULL) {
            *lpch = chSave;
        }
#endif
        SetCursor(hcursor);
        return FALSE;
    }
    
#if 0
    if (lpch != NULL) {
        *lpch = chSave;
    }
#endif
    
    if ((lpfn3 = (LPFNEEINIT) GetProcAddress(HModEE, "EEInitializeExpr")) == NULL) {
        nErrno = ERR_Invalid_Debugger_Dll;
        goto errRet;
    }
    lpfn3(&Ci, &Ei);
    
    /*
    **  Back fill any structures
    */
    
    CopyShToEe();
    
    /*
    **  Make sure that open windows find out about the new EE
    */
    
    SendMessageNZ( GetCpuHWND(),   WU_DBG_LOADEE, 0, 0);  // Give'em a change
    SendMessageNZ( GetFloatHWND(), WU_DBG_LOADEE, 0, 0);  // to Unload
    SendMessageNZ( GetLocalHWND(), WU_DBG_LOADEE, 0, 0);
    SendMessageNZ( GetWatchHWND(), WU_DBG_LOADEE, 0, 0);
    SendMessageNZ( GetCallsHWND(), WU_DBG_LOADEE, 0, 0);
    
    
    /*
    **  Set the mouse cursor back to the origial one
    */
    
    SetCursor(hcursor);
    
    // BUGBUG Unicode/ANSI support in EE?
    //EESetSuffix( SuffixToAppend );
    
    return(TRUE);
    
errRet:
    SetCursor( hcursor );
    ErrorBox( nErrno, pszDll );
    return FALSE;
}                   /* FLoadShEe() */


BOOL
FLoadEmTl(
    BOOL *pfReconnecting
    )
/*++

Routine Description:

    This routine is used to load the Execution Module and the
    Transport Layer DLLs

Arguments:

    None

Return Value:

    TRUE if the dlls were sucessfully loaded and FALSE otherwise.

--*/
{
    PCSTR       pszDllErr = NULL;
    EMFUNC      lpfnEm;
    TLFUNC      lpfnTl;
    HCURSOR     hcursor;
    int         nErrno;
    XOSD        xosd;
    
    
    CmdInsertInit();
    
    hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    if (g_Windbg_WkSp.GetSelected_TL_Dll_Name() == NULL) {
        SetCursor(hcursor);
        InformationBox(ERR_DLL_Transport_Unspecified);
        return( FALSE );
    }
    
    pszDllErr = g_Windbg_WkSp.GetSelected_TL_Dll_Name();
    
    if ((HModTL = LoadHelperDll(g_Windbg_WkSp.GetSelected_TL_Dll_Name(), "TL", TRUE)) == 0) {
        SetCursor(hcursor);
        return(FALSE);
    }
    
    /*
    **  Now get the entry point for the transport DLL
    */
    
    if ((lpfnTl = (TLFUNC) GetProcAddress(HModTL, "TLFunc")) == NULL) {
        nErrno = ERR_Invalid_Debugger_Dll;
        goto errRet;
    }
    
    xosd = OSDAddTL( lpfnTl, &Dbf, &Htl );
    
    if (xosd == xosdNone) {
        WKSPSetupTL( Htl );
    } else {
        Htl = NULL;
    }
    
    if (xosd == xosdNone) {
        xosd = OSDStartTL( Htl );
    }
    
    switch ( xosd ) {
        
    case xosdNone:
        break;                  // cool, we're set up.
        
    case xosdBadVersion:
        nErrno = ERR_Wrong_Remote_DLL_Version;
        goto errRet;
        break;
        
    case xosdCannotConnect:
        nErrno = ERR_Cannot_Connect;
        goto errRet;
        break;
        
    case xosdCantOpenComPort:
        nErrno = ERR_Cant_Open_Com_Port;
        pszDllErr = NULL;
        goto errRet;
        break;
        
    case xosdBadComParameters:
        nErrno = ERR_Bad_Com_Parameters;
        pszDllErr = NULL;
        goto errRet;
        break;
        
    case xosdBadPipeServer:
        nErrno = ERR_Bad_Pipe_Server;
        pszDllErr = NULL;
        goto errRet;
        break;
        
    case xosdBadPipeName:
        nErrno = ERR_Bad_Pipe_Name;
        pszDllErr = NULL;
        goto errRet;
        break;
        
    default:
        nErrno = ERR_Initializing_Debugger;
        goto errRet;
        break;
    }
    
    /*
    **
    */
    
    if (!FloadShEe(0)) {
        SetCursor(hcursor);
        return FALSE;
    }
    
    Assert(g_pszDLL_EXEC_MODEL);
    
    if ((HModEM = LoadHelperDll(g_pszDLL_EXEC_MODEL, "EM", TRUE)) == 0) {
        SetCursor(hcursor);
        nErrno = 0;          // LoadHelperDll already complained.
        goto errRet;
    }
    
    if ((lpfnEm = (EMFUNC) GetProcAddress(HModEM, "EMFunc")) == NULL) {
        nErrno = ERR_Invalid_Debugger_Dll;
        goto errRet;
    }
    
    if (OSDAddEM(lpfnEm, &Dbf, &Hem, emNative) != xosdNone) {
        nErrno = ERR_Initializing_Debugger;
        goto errRet;
    }
    
    WKSPSetupEM(Hem);
    
    
    //
    // in OSDEBUG4, EM will load the DM during the first emfCreateHpid
    //
    
    *pfReconnecting = FALSE;
    
    switch (OSDCreateHpid(OSDCallbackFunc, Hem, Htl, &HpidBase)) {
    case xosdNone:
        break;                  // all cool
        
    case xosdInUse:
        //
        // This is OK, but we need to pick up any existing processes
        // before we start a new one, so let the caller know what happened.
        //
        *pfReconnecting = TRUE;
        break;
        
        //case xosdBadRemoteVersion:
        //nErrno = ERR_Wrong_Remote_DLL_Version;
        //pszDllErr = szDllTL;
        //goto errRet;
        //break;
        
    case xosdCannotConnect:
        nErrno = ERR_Cannot_Connect;
        pszDllErr = g_Windbg_WkSp.GetSelected_TL_Dll_Name();
        goto errRet;
        break;
        
    default:
        nErrno = ERR_Initializing_Debugger;
        goto errRet;
        break;
    }
    
    /*
    **  Set up the Process Descriptor block for the base process
    **
    **  Clear out the pointer to the current thread descripter block.
    */
    
    SetIpid(0);
    LppdCur = LppdFirst = CreatePd( HpidBase );
    LppdFirst->fPrecious = TRUE;
    LptdCur = NULL;
    
    if (g_contWorkspace_WkSp.m_bShBackground) {
        LPFNSHSTARTBACKGROUND lpfn;
        lpfn = (LPFNSHSTARTBACKGROUND) GetProcAddress( HModSH, "SHStartBackground" );
        if (lpfn) {
            lpfn();
        }
    }
    
    /*
    **  Tell other people about the newly loaded EM
    */
    
    SendMessageNZ( GetCpuHWND(),   WU_DBG_LOADEM, 0, 0);
    SendMessageNZ( GetFloatHWND(), WU_DBG_LOADEM, 0, 0);
    
    SetCursor(hcursor);
    
    return TRUE ;
    
    
errRet:
    
    if (Hem) {
        OSDDeleteEM( Hem );
    }
    if (HModEM) {
        FreeLibrary(HModEM);
    }
    HModEM = NULL;
    lpfnEm = NULL;
    Hem    = 0;
    
    if (Htl) {
        OSDDeleteTL( Htl );
    }
    if (HModTL) {
        FreeLibrary(HModTL);
    }
    HModTL = NULL;
    lpfnTl = NULL;
    Htl    = 0;
    
    SetCursor( hcursor );
    
    if (nErrno != 0) {
        ErrorBox(nErrno, pszDllErr);
    }
    return FALSE;
}                   /* FLoadEmTl() */


/***    BPQuerySrcWinFls
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/



BOOL    PASCAL BPQuerySrcWinFls(char * pfls)
{
    Unreferenced( pfls );

    return FALSE;
}                   /* BPQuerySrcWinFls() */


/***    MELoadEEParse
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/


EESTATUS
PASCAL
MELoadEEParse(
    const char * lpb,
    EERADIX iRadix,
    SHFLAG shflag,
    PHTM phtm,
    LPDWORD lpus
    )
{
    if (!FloadShEe(mptUnknown)) {
        return (EESTATUS) -1;
    }

    return(EEParse(lpb, iRadix, shflag, phtm, lpus));
}                   /* MELoadEEParse() */


/***    BPCBGetSouceFromAddr
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

BOOL PASCAL BPCBGetSourceFromAddr(LPADDR pAddr, char FAR * rgchFile, int cchFile, int FAR * pLine)
{
    DWORD    iLine;
    if (GetSourceFromAddress(pAddr, rgchFile, cchFile, &iLine)) {
        *pLine = iLine;
        return TRUE;
    }

    return FALSE;
}                   /* BPCBGetSourceFromAddr() */


/***    SrcNameIsMasked
**
**  Synopsis:
**  bool = SrcNameIsMasked(atm)
**
**  Entry:
**  atm - Atom of file to look for in the set of masked files
**
**  Returns:
**  TRUE if a match was found and FALSE otherwise
**
**  Description:
**  This routine will look through the set of masked out file names
**  which are stored in an array of atoms looking for a match.  If
**  one is found the the name is considered to be masked out and TRUE
**  will be returned.  If no match is found then FALSE is returned.
*/

BOOL    SrcNameIsMasked(ATOM atm)
{
    int i;

    for (i=0; i<CAtomMasked; i++) {
    if (RgAtomMaskedNames[i] == atm) {
        return TRUE;
    }
    }

    return FALSE;
}                   /* SrcNameIsMasked() */


/***    SrcNameIsMapped
**
**  Synopsis:
**  bool = SrcNameIsMapped(atm, lpb, cb)
**
**  Entry:
**  atm - Atom for the file to be mapped from
**  lpb - buffer to return the mapped to name if mapping exists
**  cb  - size of lpb in bytes
**
**  Returns:
**  TRUE if a mapping was found and FALSE otherwise
**
**  Description:
**  This function will look through the set of mappings looking for
**  a match to the input file name (atm).  If a mapping is found then
**  the name of the map to file is returned in the buffer lpb.
*/


BOOL    SrcNameIsMapped(ATOM atm, LSZ lpb, UINT cb)
{
    int i;

    for (i=0; i<CAtomMapped; i++) {
    if (RgAtomMappedNames[i].atomSrc == atm) {
        GetAtomName(RgAtomMappedNames[i].atomTarget, lpb, cb);
        return TRUE;
    }
    }
    return FALSE;
}                   /* SrcNameIsMapped() */

/***    MatchOpenedDoc
**
**  Synopsis:
**  int = MatchOpenedDoc
**
**  Entry:
**  lpszSrc  - buffer containing entry & exit filename
**  cbSrc    - size of lpszSrc buffer in bytes
**
**  Returns:
** -1 - error occurred
**  0 - no match
**  1 - found and possibly mapping added
**
**  Description:
**  This routine searches through the doc list and matches doc
**  that has the same file name as lpszSrc.  If there is a match,
**  the file has not been mapped, and one of the following conditions
**  is true:
**   1. The file satisfies root mapping transformation
**   2. The file can be found along the source search path
**   3. The user agrees on the mapping
**  then a 1 will be returned.
*/
INT
MatchOpenedDoc(
   LPSTR lpszSrc,
   UINT cbSrc
)
{
   LPSTR    SrcFile = GetFileName(lpszSrc);
   CHAR     szDestName[MAX_PATH];
   int      doc;

   lMatchCnt = 0;
   if ( SrcFile ) {
      for (doc = 0; doc < MAX_DOCUMENTS; doc++) {
         if (Docs[doc].docType == DOC_WIN && Docs[doc].FirstView != -1 &&
             _stricmp(GetFileName(Docs[doc].szFileName), SrcFile) == 0) {
            // found a match just based on filename only
            strcpy(szDestName, Docs[doc].szFileName);
            if (SrcBackMapSourceFilename(szDestName, sizeof(szDestName)) == 0) {
               // file has no backward mapping
               if (RootNameIsMapped(lpszSrc, szDestName, sizeof(szDestName)) &&
                   _stricmp(szDestName, Docs[doc].szFileName) == 0) {
                  SrcSetMapped(lpszSrc, szDestName);
                  if (cbSrc <= strlen(szDestName)) {
                     return (-1);       // error
                  } else {
                     strcpy(lpszSrc, szDestName);
                     return (1);        // matched and mapped
                  }
               } else if (strcpy(szDestName, lpszSrc) &&
                          SrcSearchOnPath(szDestName, sizeof(szDestName), FALSE) &&
                          _stricmp(szDestName, Docs[doc].szFileName) == 0) {
                  SrcSetMapped(lpszSrc, szDestName);
                  if (cbSrc <= strlen(szDestName)) {
                     return (-1);       // error
                  } else {
                     strcpy(lpszSrc, szDestName);
                     return (1);        // matched and mapped
                  }
               } else { // ask user
                  MatchedList[lMatchCnt++] = doc;
               }
            }
         }
      }
   }
   if (lMatchCnt > 0 && !AutoTest) {
      if (NoPopups)     {
         MSG            msg;
matchopeneddocagain:
         sprintf( szBrowsePrompt, "File %s can be mapped to several opened documents.\n", lpszSrc );
         fBrowseAnswer = FALSE;
         CmdSetCmdProc( CmdMatchOpenedDocInputString, CmdMatchOpenedDocPrompt );
         CmdSetAutoHistOK(FALSE);
         CmdSetEatEOLWhitespace(FALSE);
         CmdDoPrompt( TRUE, TRUE );
         while (GetMessage( &msg, NULL, 0, 0 )) {
            ProcessQCQPMessage( &msg );
            if (fBrowseAnswer) {
                break;
            }
         }
         if (szBrowseFname[0]) {
            lMatchIdx = atoi(szBrowseFname);
            if (0 < lMatchIdx && lMatchIdx <= lMatchCnt) {
               SrcFile = Docs[MatchedList[lMatchIdx-1]].szFileName;
               SrcSetMapped(lpszSrc, SrcFile);
               if (cbSrc <= strlen(SrcFile))
                  return(-1);
               else {
                  strcpy(lpszSrc, SrcFile);
                  return(1);
               }
            } else {
               goto matchopeneddocagain;
            }
         }
         CmdSetDefaultCmdProc();
         CmdDoPrompt(TRUE, TRUE);
         return 0;
      }
      strcpy(szFSSrcName, lpszSrc);
      StartDialog( DLG_FSRESOLVE, DlgFileSearchResolve );
      if (lMatchIdx >= 0) {
         SrcFile = Docs[MatchedList[lMatchIdx]].szFileName;
         SrcSetMapped(lpszSrc, SrcFile);

         if (FAddToSearchPath)
            AddToSearchPath(SrcFile);
         else if (FAddToRootMap)
            RootSetMapped(lpszSrc, SrcFile);

         if (cbSrc <= strlen(SrcFile))
            return(-1);
         else {
            strcpy(lpszSrc, SrcFile);
            return(1);
         }
      }
   }
   return 0;
}  // MatchOpenedDoc


/***    SrcSearchOnPath
**
**  Synopsis:
**  bool = SrcSearchOnPath(lpb, cb)
**
**  Entry:
**  lpb  - buffer containing entry & exit filename
**  cb   - size of lpb in bytes
**
**  Returns:
**  TRUE if the file was found in the search path and FALSE otherwise
**
**  Description:
**  This routine will strip down to the base file name of the
**  source file the system is currently looking for and check for
**  the file in the current working directory, the exe's directory
**  and on the directories specified in the SourcePath tools.ini
**  variable.  If the file is found then the full path is returned
**  in lpb.
*/


BOOL 
SrcSearchOnPath(
    LSZ lpb, 
    UINT cb, 
    BOOL fAddToMap
    )
{
    TCHAR   szDrive[_MAX_DRIVE];
    TCHAR   szDir[_MAX_DIR];
    TCHAR   szFName[_MAX_FNAME];
    TCHAR   szExt[_MAX_EXT];
    TCHAR   rgch[_MAX_PATH];
    TCHAR   rgch2[_MAX_PATH];
    TCHAR   rgchFName[_MAX_FNAME+_MAX_EXT];
    BOOL    Found;
    TCHAR   *p;

    //
    //  Get file name
    //
    _splitpath(lpb, szDrive, szDir, szFName, szExt);
    strcpy(rgchFName, szFName);
    strcat(rgchFName, szExt);

    //
    //  Look in the current directory.
    //
    Found = FindNameOn(rgch, sizeof(rgch), ".", rgchFName);

    if ( !Found ) {

        //
        //  Not in current directory, look in EXE directory.
        //
        if ( g_Windbg_WkSp.GetCurrentProgramName(FALSE) ) {
            strcpy( rgch, g_Windbg_WkSp.GetCurrentProgramName(FALSE) );
            _splitpath(rgch, szDrive, szDir, szFName, szExt );
            strcpy( rgch2, szDrive );
            strcat( rgch2, szDir );
            p = CharPrev(rgch2, rgch2 + strlen(rgch2));
            if ( *p == '\\' ) {
                *p = '\0';
            }

            Found = FindNameOn(rgch, sizeof(rgch), rgch2, rgchFName);
        }

        if ( !Found ) {
            //
            //  Not in EXE directory, look along SourcePath.
            //
            Found = FindNameOn(rgch, sizeof(rgch), g_contPaths_WkSp.m_pszSourceCodeSearchPath, rgchFName);
        }
    }

    if ( Found ) {

        if (strlen(rgch) > cb) {

            //
            //  Not enough space in return buffer
            //
            Found = FALSE;

        } else {

            if (fAddToMap) {
               SrcSetMapped(lpb, rgch);
            }

            strcpy(lpb, rgch);
        }
    }

    return Found;
}                   /* SrcSearchOnPath() */

/***    SrcSearchOnRoot
**
**  Synopsis:
**  bool = SrcSearchOnRoot(lpb, cb)
**
**  Entry:
**  lpb  - buffer containing entry & exit filename
**  cb   - size of lpb in bytes
**
**  Returns:
**  TRUE if the file was found by root mapping and FALSE otherwise
**
**  Description:
**  This routine compares the given file's root with those stored
**  in the root mapping table.  If not found, it uses the debuggee's
**  drive to locate the source.
*/
BOOL SrcSearchOnRoot(LSZ lpb, UINT cb)
{
   char    rgch[_MAX_PATH];
   char    rgch2[_MAX_PATH];
   char    rgchDest[_MAX_PATH];
   BOOL    Found;
   char    *p;
   int     i;

   Found = RootNameIsMapped(lpb, rgchDest, sizeof(rgchDest));

#if 0
   if (!Found) {
      _splitpath(lpb, szDrive, szDir, szFName, szExt);
      Assert(szDir[0] == '\\');
      sprintf(rgch, "%s%s%s", (szDir[0] == '\\') ? szDir+1 : szDir,
              szFName, szExt);    // src without drive

      //
      //  Look into the debuggee's drive
      //
      if ( g_Windbg_WkSp.GetCurrentProgramName(FALSE) ) {
         if (_fullpath(rgch2, g_Windbg_WkSp.GetCurrentProgramName(FALSE), sizeof(rgch2)) != NULL) {
            // make sure drive letter exists and not the same as
            // the original one as it would have been searched
            if (rgch2[1] == ':' && _strnicmp(szDrive, rgch2, 2) != 0) {
               rgch2[2] = '\0';
               Found = FindNameOn(rgchDest, sizeof(rgchDest), rgch2, rgch);
            }
         }
      }
   }
#endif

   if ( Found ) {
      if (strlen(rgchDest) >= cb) {
         //
         //  Not enough space in return buffer
         //
         Found = FALSE;
      } else {
         SrcSetMapped(lpb, rgchDest);
         strcpy(lpb, rgchDest);
      }
   }
   return Found;
}                   /* SrcSearchOnRoot() */



BOOL
MiscBrowseForFile(
    LSZ     lpb,
    UINT    cb,
    LSZ     lpDir,
    UINT    cchDir,
    int     nDefExt,
    int     nIdDlgTitle,
    void    (*fnSetMapped)(LSZ, LSZ),
    LPOFNHOOKPROC lpfnHook,
    int     nExplorerExtensionTemplateName
    )
/*++

Routine Description:

    Tell the user that we can't find a file, and allow him to
    browse for a it using a standard File Open dialog.

Arguments:

    lpb     - Supplies name of requested file,
              Returns fully qualified path
    cb      - Supplies size of buffer at lpb
    lpDir   - Supplies directory to start browse in,
              Returns directory file was found in
    cchDir  - Supplies size of buffer at lpDir
    nDefExt - Supplies resource id for default extension filter
    nIdDlgTitle - Supplies res id for dialog title string
    fnSetMapped - Supplies pointer to function for mapping
                  requested name to returned name.
    lpfnHook - Pointer to a hook procedure for the additional
                controls or to customize the file open dlg.
    pszExplorerExtensionTemplateId - Resource Id to a dlg
                template that will be an extension of the
                typical explorer dlg. This simple adds
                controls to the file open dlg. 0 for none.

Return Value:

    If a file is found, return TRUE and copy the path into
    the buffer at lpb.

--*/
{
    CHAR    fname[_MAX_FNAME];
    CHAR    ext[_MAX_EXT];

    DWORD   dwFlags = OFN_FILEMUSTEXIST |
                    OFN_PATHMUSTEXIST |
                    OFN_HIDEREADONLY |
                    OFN_NOCHANGEDIR;

    char    rgchT[_MAX_PATH];
    char    CurrentDirectory[_MAX_PATH ];
    BOOL    Ret = FALSE;

    // Make sure we always use the new look.
    if (nExplorerExtensionTemplateName || lpfnHook) {
        dwFlags |= OFN_EXPLORER;
    }

    // Tell the file open dlg that we want to
    // append controls to it.
    if (nExplorerExtensionTemplateName) {
        dwFlags |= OFN_ENABLETEMPLATE;
    }

    Assert(strlen(lpb) < sizeof(rgchT));
    strcpy(rgchT, lpb);

    if (DLG_Browse_Filebox_Title != nIdDlgTitle) {
       _splitpath( rgchT, NULL, NULL, fname, ext );
       _makepath( rgchT, NULL, NULL, fname, ext );
    }

    GetCurrentDirectory( sizeof( CurrentDirectory ), CurrentDirectory );
    if ( *lpDir ) {
        SetCurrentDirectory( lpDir );
    }

    if (StartFileDlg(hwndFrame,
                     nIdDlgTitle,
                     nDefExt,
                     ID_BROWSE_HELP,
                     nExplorerExtensionTemplateName,
                     rgchT,
                     &dwFlags,
                     lpfnHook)
    ) {
         Assert( strlen(rgchT) < cb );

         if ( strlen(rgchT)+1 <= cb) {
            (*fnSetMapped)(lpb, rgchT);
            strcpy(lpb, rgchT);
            GetCurrentDirectory( cchDir, lpDir);
            Ret = TRUE;
        }
    }

    SetCurrentDirectory( CurrentDirectory );

    return Ret;
}                   /* MiscBrowseForFile() */

BOOL NEAR PASCAL
StringLogger(
    LPCSTR      szStr,
    BOOL        fFileLog,
    BOOL        fSendRemote,
    BOOL        fPrintLocal
);

VOID
CmdMatchOpenedDocPrompt(
    BOOL fRemote,
    BOOL fLocal
    )
{
   int   i;
   LPSTR lpszName;

    CmdInsertInit();
    StringLogger( szBrowsePrompt, TRUE, fRemote, fLocal );
    StringLogger( "Please select from one of the followings or <CR> for none:\n", TRUE, fRemote, fLocal );
    for (i=0; i < lMatchCnt; i++) {
       lpszName = Docs[MatchedList[i]].szFileName;
       sprintf(szBrowsePrompt, "%d. %s\n", i+1, lpszName);
       StringLogger( szBrowsePrompt, TRUE, fRemote, fLocal );
    }
}

BOOL
CmdMatchOpenedDocInputString(
    LPSTR lpsz
    )
{
    strcpy( szBrowseFname, lpsz );
    fBrowseAnswer = TRUE;
    CmdSetDefaultCmdProc();
    return TRUE;
}

VOID
CmdBrowsePrompt(
    BOOL fRemote,
    BOOL fLocal
    )
{
    CmdInsertInit();
    StringLogger( szBrowsePrompt, TRUE, fRemote, fLocal );
}

BOOL
CmdBrowseInputString(
    LPSTR lpsz
    )
{
    strcpy( szBrowseFname, lpsz );
    fBrowseAnswer = TRUE;
    CmdSetDefaultCmdProc();
    return TRUE;
}

int
InsertListViewItem(
    HWND hwndList,
    UINT uPos,
    LV_ITEM * lplvi,
    int nNumColumns,
    ...
    )
/*++

Routine Description:

    Inserts a root mapping pair into the list view control.

Arguments:

    hwndList    - List view control.

    uPos        - Position where the item is to be inserted.

    lplvi       - A prexisting item that is to be used as a template.

    nNumColumns - Number of text columns to be added.

    ...         - Text names of the columns

Return Value:

    Returns the index of the new item if successful, or -1 otherwise.

--*/
{
    Assert(nNumColumns);

    LV_ITEM             lvi;
    int                 nColIdx;
    int                 nIdx;
    va_list             marker;

    // Initialize variable arguments
    va_start(marker, nNumColumns);

    // Initialize LV_ITEM members that are common to all items.
    if (lplvi) {
        memcpy(&lvi, lplvi, sizeof(lvi));
    } else {
        memset(&lvi, 0, sizeof(lvi));
    }

    lvi.mask = LVIF_TEXT;

    //
    // Add name (main item)
    //
    lvi.iItem = uPos;
    lvi.pszText = va_arg(marker, PTSTR);
    lvi.iSubItem = 0;

    nIdx = ListView_InsertItem(hwndList, &lvi);
    Assert(-1 != nIdx);

    for (nColIdx = 1; nColIdx < nNumColumns; nColIdx++) {
        //
        // Add target (sub item)
        //
        ListView_SetItemText(hwndList, nIdx, nColIdx, va_arg( marker, PTSTR));
    }
    va_end( marker );

    return nIdx;
}

INT_PTR
CALLBACK
DlgProcBadSymbols(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // WARNING: On entry lpbs->pFindSymFileData->szImageFilePath contains the name of the image
    // we want to find symbols for. On exit, it will be blank if the user DID NOT choose
    // a symbol file. If the user chose a symbol file from the list or by browsing,
    // lpbs->pFindSymFileData->szImageFilePath will contain the path to the file.
    //
    static LPBROWSESTRUCT lpbs = {0};


    switch (message) {
    case WM_INITDIALOG :
        lpbs = (LPBROWSESTRUCT) lParam;
        Assert(lpbs);

        //
        // Initialize image list
        //
        {
            HWND                hwndImageList = GetDlgItem(hDlg, IDC_LIST_IMAGE_FILE);
            TCHAR               szColHdr[MAX_MSG_TXT];
            LV_COLUMN           lvc;

            Assert(hwndImageList);

            //
            // Set the extended style
            //
            ListView_SetExtendedListViewStyle(hwndImageList, LVS_EX_FULLROWSELECT);


            ///////////////////////////////////////////
            // Setup the column header for the list view
            ///////////////////////////////////////////

            //
            // Add Column headers
            //

            // Initialize the LV_COLUMN structure.
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt = LVCFMT_LEFT;
            lvc.pszText = szColHdr;
            // Make the column width half of the window width
            {
                RECT rc;
                GetClientRect(hwndImageList, &rc);
                lvc.cx = rc.right * 2;
            }

            // Add the 1st column hdr
            Dbg(LoadString(g_hInst, SYS_Image_Col_Hdr1,
                szColHdr, sizeof(szColHdr) / sizeof(TCHAR) ));
            lvc.iSubItem = 0;
            Dbg(ListView_InsertColumn(hwndImageList, lvc.iSubItem, &lvc) != -1);

            ///////////////////////////////////////////
            // Add the data to the list view
            ///////////////////////////////////////////

            //
            // Add the individual items
            //
            InsertListViewItem(hwndImageList,
                               1,
                               NULL,
                               1,
                               lpbs->pFindSymFileData->szImageFilePath);

            // Select the first item
            ListView_SetItemState(hwndImageList,
                                  0,
                                  LVIS_FOCUSED | LVIS_SELECTED,
                                  0x000F);
        }

        //
        // Initialize symbol list
        //
        {
            HWND                hwndSymbolList = GetDlgItem(hDlg, IDC_LIST_SYM_FILES);
            TCHAR               szColHdr[MAX_MSG_TXT];
            LV_COLUMN           lvc;

            Assert(hwndSymbolList);

            //
            // Set the extended style
            //
            ListView_SetExtendedListViewStyle(hwndSymbolList, LVS_EX_FULLROWSELECT);


            ///////////////////////////////////////////
            // Setup the column header for the list view
            ///////////////////////////////////////////

            //
            // Add Column headers
            //

            // Initialize the LV_COLUMN structure.
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt = LVCFMT_LEFT;
            lvc.pszText = szColHdr;
            // Make the column width half of the window width
            {
                RECT rc;
                GetClientRect(hwndSymbolList, &rc);
                lvc.cx = rc.right / 3;
            }

            // Add the 1st column hdr
            Dbg(LoadString(g_hInst, SYS_Bad_Sym_Col_Hdr1,
                szColHdr, sizeof(szColHdr) / sizeof(TCHAR) ));
            lvc.iSubItem = 0;
            Dbg(ListView_InsertColumn(hwndSymbolList, lvc.iSubItem, &lvc) != -1);

            // Add the 2nd col hdr
            Dbg(LoadString(g_hInst, SYS_Bad_Sym_Col_Hdr2,
                szColHdr, sizeof(szColHdr) / sizeof(TCHAR) ));
            lvc.iSubItem = 1;
            Dbg(ListView_InsertColumn(hwndSymbolList, lvc.iSubItem, &lvc) != -1);

            // Add the 3rd col hdr
            Dbg(LoadString(g_hInst, SYS_Bad_Sym_Col_Hdr3,
                szColHdr, sizeof(szColHdr) / sizeof(TCHAR) ));
            lvc.iSubItem = 2;
            // Make this one really wide.
            lvc.cx *= 6;
            Dbg(ListView_InsertColumn(hwndSymbolList, lvc.iSubItem, &lvc) != -1);

            ///////////////////////////////////////////
            // Add the data to the list view
            ///////////////////////////////////////////

            //
            // Add the individual items
            //
            if ( IsListEmpty( (PLIST_ENTRY) &lpbs->pFindSymFileData->LoadErr )
                && !lpbs->pFindSymFileData->LoadErr.hSymFile ) {

                // Nothing to list
                EnableWindow(hwndSymbolList, FALSE);
            } else {
                // We have some files to add
                EnableWindow(GetDlgItem(hDlg, IDC_BUT_LOAD), TRUE);


                int     nIdx = 0; // Position in list where it was actually placed.
                UINT    uPos = 0;
                PTSTR   pszSHE = NULL;
                PTSTR   pszExtendedSHE = NULL;
                PSYM_FILE_LOAD_ERR pLoadErr = &lpbs->pFindSymFileData->LoadErr;
                LVITEM  lvitem;
                LVITEM  lvitem2;
                LVITEM  lvitem3;


                do {
                    pszSHE = pLoadErr->pszSHE ? pLoadErr->pszSHE : "";

                    pszExtendedSHE = pLoadErr->pszExtendedSHE
                        ? pLoadErr->pszExtendedSHE : "";

                    nIdx = InsertListViewItem(
                                             hwndSymbolList,
                                             ListView_GetItemCount(hwndSymbolList),
                                             NULL,
                                             3,
                                             pLoadErr->szSymFilePath,
                                             pszSHE,
                                             pszExtendedSHE);

                    ZeroMemory(&lvitem, sizeof(lvitem));
                    ZeroMemory(&lvitem2, sizeof(lvitem2));
                    ZeroMemory(&lvitem3, sizeof(lvitem3));

                    lvitem.iItem = nIdx;
                    lvitem.mask = LVIF_PARAM;
                    lvitem.lParam = (LPARAM) pLoadErr;


                    lvitem2.iItem = nIdx;
                    lvitem2.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_INDENT
                        | LVIF_NORECOMPUTE | LVIF_PARAM | LVIF_STATE
                        | LVIF_DI_SETITEM;

                    lvitem3 = lvitem2;

                    Dbg( ListView_GetItem(hwndSymbolList, &lvitem2) );

                    Dbg( ListView_SetItem(hwndSymbolList, &lvitem) );

                    Dbg( ListView_GetItem(hwndSymbolList, &lvitem3) );

                    pLoadErr = pLoadErr->Flink;
                } while ( pLoadErr != &lpbs->pFindSymFileData->LoadErr );

                // Select the first item
                ListView_SetItemState(hwndSymbolList,
                                      0,
                                      LVIS_FOCUSED | LVIS_SELECTED,
                                      0x000F);
            }

            // Set focus manually
            SetFocus(hwndSymbolList);

            SetForegroundWindow( hDlg );
            MessageBeep( MB_ICONQUESTION );
            return FALSE;
        }

    case WM_COMMAND:
        switch (wParam) {
        default:
            // return as not handled
            return FALSE;

        case IDC_BUT_ADVANCED:
            DialogBox(g_hInst,
                      MAKEINTRESOURCE(DLG_BADSYMBOLS_ADV),
                      hDlg,
                      DlgProc_Adv_BadSymbols);
            return TRUE;

        case IDCANCEL:
            // End the dialog and return the result
            break;

        case IDC_BUT_LOAD:
            {
                HWND hwndSymbolList = GetDlgItem(hDlg, IDC_LIST_SYM_FILES);
                Assert(hwndSymbolList);
                int nCurSel = ListView_GetNextItem(hwndSymbolList, -1, LVNI_SELECTED);

                lpbs->pFindSymFileData->szImageFilePath[0] = NULL;

                if (nCurSel >= 0) {
                    char sz[MAX_PATH];

                    ListView_GetItemText(
                        hwndSymbolList,
                        nCurSel,
                        0,
                        lpbs->pFindSymFileData->szImageFilePath,
                        sizeof(lpbs->pFindSymFileData->szImageFilePath) / sizeof(TCHAR));
                }
            }
            // End the dialog and return the result
            break;

        case IDC_BUT_BROWSE:
            // End the dialog and return the result
            {
                OPENFILENAME ofn = {0};
                PTSTR pszFilter = NULL;
                TCHAR szTitle[200] = {0};
                TCHAR szNewSymFile[_MAX_PATH] = {0};
                static TCHAR szInitialDir[_MAX_PATH] = {0};

                if (!*szInitialDir) {
                    GetCurrentDirectory(sizeof(szInitialDir) / sizeof(TCHAR), szInitialDir);
                }

                //
                // Title
                Dbg(LoadString(g_hInst,
                               DLG_Browse_For_Symbols_Title,
                               szTitle,
                               sizeof(szTitle) / sizeof(TCHAR) ));

                // Add the abbreviated path to the title
                AdjustFullPathName(lpbs->pFindSymFileData->szImageFilePath,
                    szTitle + _tcslen(szTitle), 10);
                Assert(_tcslen(szTitle) < sizeof(szTitle) / sizeof(TCHAR) );

                //
                // File filters
                //
                {
                    const int MAX_EXTENSIONS = 3;
                    TCHAR szImageName[_MAX_FNAME] = {0};
                    // Include room for the ";"
                    TCHAR szCustFilter[(_MAX_FNAME + _MAX_EXT + 1) * MAX_EXTENSIONS] = {0};
                    PTSTR pszDefExtAllSyms = WKSP_DynaLoadString(g_hInst, DEF_Ext_All_SYMS);
                    PTSTR pszTypeFileAllSyms = WKSP_DynaLoadString(g_hInst, TYP_File_All_SYMS);
                    PTSTR pszDefExtCustSyms = WKSP_DynaLoadString(g_hInst, DEF_Ext_Cust_SYMS);
                    PTSTR pszTypeFileCustSyms = WKSP_DynaLoadString(g_hInst, TYP_File_Cust_SYMS);
                    PTSTR pszTmp;

                    // Build the specific filter
                    _tsplitpath(lpbs->pFindSymFileData->szImageFilePath, NULL, NULL, szImageName, NULL);

                    {
                        pszTmp = pszDefExtCustSyms;
                        for (int nCnt=0; *pszTmp; nCnt++, pszTmp += _tcslen(pszTmp) +1) {
                            if (nCnt) {
                                _tcscat(szCustFilter, ";");
                            }
                            _tcscat(szCustFilter, szImageName);
                            _tcscat(szCustFilter, pszTmp);
                        }
                        Assert(MAX_EXTENSIONS == nCnt);
                    }

                    pszFilter = pszTmp = (PTSTR) calloc(_tcslen(pszTypeFileCustSyms) + _tcslen(szCustFilter) *2
                        + _tcslen(pszTypeFileAllSyms) + _tcslen(pszDefExtAllSyms)
                        + 4 + 3 + 1, // 4 string terminator, plus " ()", plus the extra one at the end
                        sizeof(TCHAR));

                    _tcscpy(pszTmp, pszTypeFileCustSyms);
                    _tcscat(pszTmp, " (");
                    _tcscat(pszTmp, szCustFilter);
                    _tcscat(pszTmp, ")");
                    pszTmp += _tcslen(pszTmp) +1;

                    _tcscpy(pszTmp, szCustFilter);
                    pszTmp += _tcslen(pszTmp) +1;

                    _tcscpy(pszTmp, pszTypeFileAllSyms);
                    pszTmp += _tcslen(pszTmp) +1;

                    _tcscpy(pszTmp, pszDefExtAllSyms);

                    FREE_STR(pszDefExtAllSyms);
                    FREE_STR(pszTypeFileAllSyms);
                    FREE_STR(pszDefExtCustSyms);
                    FREE_STR(pszTypeFileCustSyms);
                }

                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hDlg;
                //ofn.hInstance;
                ofn.lpstrFilter = pszFilter;
                //ofn.lpstrCustomFilter;
                //ofn.nMaxCustFilter;
                //ofn.nFilterIndex;
                ofn.lpstrFile = szNewSymFile;
                ofn.nMaxFile = sizeof(szNewSymFile) / sizeof(TCHAR);
                //ofn.lpstrFileTitle;
                //ofn.nMaxFileTitle;
                ofn.lpstrInitialDir = szInitialDir;
                ofn.lpstrTitle = szTitle;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR
                    | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
                //ofn.nFileOffset;
                //ofn.nFileExtension;
                //ofn.lpstrDefExt;
                //ofn.lCustData;
                //ofn.lpfnHook;
                //ofn.lpTemplateName;

                if (!GetOpenFileName(&ofn)) {
                    // User canceled browse or an error occurred.
                    // Stay where we are.
                    return FALSE;
                } else {
                    // User selected a file
                    // Copy the drive letter here
                    TCHAR szDrive[_MAX_DRIVE] = {0};
                    TCHAR szDir[_MAX_DIR] = {0};

                    // Ask him if he wants to add this path to the sym search path
                    _tsplitpath(szNewSymFile, szDrive, szDir, NULL, NULL);
                    _tcscpy(szInitialDir, szDrive);
                    _tcscat(szInitialDir, szDir);

                    if (IDYES == VarMsgBox(hDlg, DLG_AddPathToSymSearchPath,
                        MB_ICONQUESTION | MB_YESNO, szInitialDir)) {

                        ModListAddSearchPath(szInitialDir);
                    }

                    // Place the data in the list head
                    _tcscpy(lpbs->pFindSymFileData->LoadErr.szSymFilePath, szNewSymFile);
                }

                FREE_STR(pszFilter);
            }
            break;
        }

        //
        // End the dialog
        lpbs->Rslt = wParam;
        EndDialog(hDlg, wParam);
        return TRUE;
    }

    return FALSE;
}

INT_PTR
CALLBACK
DlgProc_Adv_BadSymbols(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (message) {
    case WM_INITDIALOG :
        //
        // Set radio buttons
        //
        Assert(!g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors);

        CheckRadioButton(hDlg, IDC_RADIO_IGNORE_ERRORS, IDC_RADIO_NEVER_LOAD_BAD_SYMBOLS,
            g_contWorkspace_WkSp.m_bBrowseForSymsOnSymLoadErrors
            ? IDC_RADIO_PROMPT_ON_ERROR : IDC_RADIO_NEVER_LOAD_BAD_SYMBOLS);

        return FALSE;

    case WM_COMMAND:
        switch (wParam) {
        default:
            // return as not handled
            return FALSE;

        case IDOK:
            //
            // Dlg is about to end. Set any modified values.
            if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO_IGNORE_ERRORS)) {
                g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors = TRUE;
            } else if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO_PROMPT_ON_ERROR)) {
                g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors = FALSE;
                g_contWorkspace_WkSp.m_bBrowseForSymsOnSymLoadErrors = TRUE;
            } else if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO_NEVER_LOAD_BAD_SYMBOLS)) {
                g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors = FALSE;
                g_contWorkspace_WkSp.m_bBrowseForSymsOnSymLoadErrors = FALSE;
            } else {
                Assert(!"Not supposed to happen");
            }
            break;

        case IDCANCEL:
            // End the dialog and return the result
            break;
        }


        //
        // End the dialog
        EndDialog(hDlg, wParam);
        return TRUE;
    }

    return FALSE;
}

INT_PTR
CALLBACK
DlgBrowse(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static LPBROWSESTRUCT lpbs = {0};
    char szAskBrowse[_MAX_PATH];
    char szTemp[FILES_MENU_WIDTH + 1];

    switch (message) {
    case WM_INITDIALOG :
        lpbs = (LPBROWSESTRUCT) lParam;

        AdjustFullPathName( lpbs->pszFileName, szTemp,  FILES_MENU_WIDTH);
        strcpy(szAskBrowse, "Browse for: ");
        strcat(szAskBrowse, szTemp);
        strcat(szAskBrowse, " ?");
        SetDlgItemText(hDlg, IDC_STXT_BROWSE_FOR, szAskBrowse);

        SetForegroundWindow(hDlg);
        MessageBeep( 0 );
        return TRUE;

    case WM_COMMAND:
        switch (wParam) {
        case IDCANCEL:
        case IDOK:
        case IDC_STXT_BROWSE_FOR:
            lpbs->Rslt = wParam;
            EndDialog( hDlg,wParam );
            return TRUE;

        case IDWINDBGHELP :
            Dbg(WinHelp(hDlg,szHelpFileName, HELP_CONTEXT, ID_ASKBROWSE_HELP));
            return TRUE;
        }
        break;
    }

    return FALSE;
}

DWORD
BrowseThread(
    LPVOID lpv
    )
{
    HWND           hDlg;
    MSG            msg;
    LPBROWSESTRUCT lpbs = (LPBROWSESTRUCT)lpv;


    hDlg = CreateDialogParam( g_hInst, MAKEINTRESOURCE(lpbs->DlgId),
        NULL, lpbs->DlgProc, (LPARAM)lpbs );
    if (!hDlg) {
        return FALSE;
    }

    ShowWindow( hDlg, SW_SHOW );

    while ((lpbs->Rslt == (DWORD_PTR)-1) && (GetMessage (&msg, NULL, 0, 0))) {
        if (!IsDialogMessage( hDlg, &msg )) {
            TranslateMessage ( &msg );
            DispatchMessage ( &msg );
        }
    }

    return TRUE;
}

BOOL
SrcBrowseForFile(
    LSZ lpb,
    UINT cb
    )
{
    HANDLE        hThread;
    DWORD         id;
    BROWSESTRUCT  bs = {0};
    MSG           msg;

    /*
     *  Check to see if running from scripts -- if so then
     *  don't bring up this dialog and error boxes.
     */

    if (AutoTest) {
        return FALSE;
    }

    if (NoPopups) {
browseagain:
        sprintf( szBrowsePrompt, "Cannot load [ %s ] - Enter new path or <CR> to ignore ", lpb );
        fBrowseAnswer = FALSE;
        CmdSetCmdProc( CmdBrowseInputString, CmdBrowsePrompt );
        CmdSetAutoHistOK(FALSE);
        CmdSetEatEOLWhitespace(FALSE);
        CmdDoPrompt( TRUE, TRUE );
        while (GetMessage( &msg, NULL, 0, 0 )) {
            ProcessQCQPMessage( &msg );
            if (fBrowseAnswer) {
                break;
            }
        }
        if (szBrowseFname[0]) {
            if (FileExist( szBrowseFname )) {
                strcpy( lpb, szBrowseFname );
                return TRUE;
            } else {
                goto browseagain;
            }
        }

        CmdSetDefaultCmdProc();
        CmdDoPrompt(TRUE, TRUE);
        return FALSE;
    }

    EnsureFocusDebugger();

    bs.Rslt         = (DWORD_PTR)-1;
    bs.DlgId        = DLG_ASKSRCBROWSE;
    bs.DlgProc      = DlgBrowse;
    bs.pszFileName  = lpb;
    bs.FnameSize    = cb;

    hThread = CreateThread( NULL, 0, BrowseThread, (LPVOID)&bs, 0, &id );
    if (!hThread) {
        return FALSE;
    }

    WaitForSingleObject( hThread, INFINITE );

    if (bs.Rslt == IDCANCEL) {
        return FALSE;
    } else {
        return MiscBrowseForFile(lpb,
                                 cb,
                                 SrcFileDirectory,
                                 sizeof(SrcFileDirectory),
                                 DEF_Ext_C,
                                 DLG_Browse_Filebox_Title,
                                 SrcSetMapped,
                                 GetOpenFileNameHookProc,
                                 IDD_DLG_FILEOPEN_EXPLORER_EXTENSION);
    }
}                   /* SrcBrowseForFile() */


INT_PTR
ExeBrowseBadSym(
    PFIND_SYM_FILE  pFindSymFileData
    )
/*++

Routine Description:

Arguments:

    pszImageFile - Name of the image that we hope to find symbols for

    pFindSymFileData - Info returned by DBGHELP.DLL

Returns:

    IDCANCEL - User does not wish to load a corresponding sym file.

    IDC_BUT_LOAD - User selected a sym file from the list of files previously tested.

    IDC_BUT_BROWSE - New file selected.
--*/
{
    Assert(pFindSymFileData);

    HANDLE        hThread;
    DWORD         id;
    BROWSESTRUCT  bs = {0};


    if (arCmdline == AutoRun || NoPopups) {
        // Don't load symbols with any errors
        return IDCANCEL;
    }

    bs.Rslt             = (DWORD_PTR)-1;
    bs.DlgId            = DLG_BADSYMBOLS;
    bs.DlgProc          = DlgProcBadSymbols;
    //bs.szFileName;
    //bs.FnameSize;
    bs.pFindSymFileData   = pFindSymFileData;

    hThread = CreateThread( NULL, 0, BrowseThread, (LPVOID)&bs, 0, &id );
    if (!hThread) {
        return FALSE;
    }

    WaitForSingleObject( hThread, INFINITE );

    return bs.Rslt;
}

VOID
ExeSetMapped(
    LSZ lsz1,
    LSZ lsz2
    )
{
    // nobody home
}

/***    SrcSetMasked
**
**  Synopsis:
**  void = SrcSetMasked(lsz)
**
**  Entry:
**  lsz - Pointer to byte array for unfound name
**
**  Returns:
**  Nothing
**
**  Description:
**  This routine will take the entry name and set it as to be
**  masked out.  This allows use to prevent re-display of error
**  messages on the same file.
*/

VOID
SrcSetMasked(
    LSZ lsz
    )
{
    ATOM    atom;

    atom = AddAtom(lsz);
    if (atom == 0) {
      return;
    }

    if (RgAtomMaskedNames == NULL) {
        CMacAtomMasked = 10;
        RgAtomMaskedNames = (ATOM *) malloc(sizeof(ATOM)*CMacAtomMasked);
    } else if (CAtomMasked == CMacAtomMasked) {
        CMacAtomMasked += 10;
        RgAtomMaskedNames = (ATOM *) realloc(RgAtomMaskedNames, sizeof(ATOM)*CMacAtomMasked);
    }

    RgAtomMaskedNames[CAtomMasked] = atom;
    CAtomMasked += 1;
    return;
}                   /* SrcSetMasked() */

/***    SrcSetMapped
**
**  Synopsis:
**  void = SrcSetmapped(lsz1, lsz2)
**
**  Entry:
**  lsz1 - Name of file to be mapped from
**  lsz2 - Name of file to be mapped to
**
**  Returns:
**  Nothing
**
**  Description:
**  This function will setup a mapping of source file names from
**  file name lsz1 to lsz2.  This will allow for a fast reamapping
**  without haveing to do searches or ask the user a second time.
**
*/

VOID    
SrcSetMapped(
    LSZ lsz1, 
    LSZ lsz2
    )
{
   ATOM    atomSrc = AddAtom(lsz1);
   ATOM    atomTrg = AddAtom(lsz2);

   if (RgAtomMappedNames == NULL) {
      CMacAtomMapped = 10;
      RgAtomMappedNames = (struct MpPair *) malloc(sizeof(*RgAtomMappedNames)*CMacAtomMapped);
   } else if (CAtomMapped == CMacAtomMapped) {
      CMacAtomMapped += 10;
      RgAtomMappedNames = (struct MpPair *) realloc(RgAtomMappedNames, sizeof(*RgAtomMappedNames)*CMacAtomMapped);
   }

   RgAtomMappedNames[CAtomMapped].atomSrc = atomSrc;
   RgAtomMappedNames[CAtomMapped].atomTarget = atomTrg;
   CAtomMapped += 1;
}                   /* SrcSetMapped() */


/***    SrcMapSourceFilename
**
**  Synopsis:
**  int = SrcMapSourceFilename(lpszSrc, cbSrc, flags, bUserActivated)
**
**  Entry:
**  lpszSrc - Source buffer to map file in
**  cbSrc   - size of source buffer
**  flags   - flags to control behavior
**  bUserActivated - Indicates whether this action was initiated by the
**      user or by windbg. The value is to determine the Z order of
**      any windows that are opened.
**
**  Returns:
**  -2 - operation canceled
**  -1 - error occured
**  0 - No mapping done -- no source file mapped
**  1 - Mapping done -- no file openned
**  2 - Mapping done -- openned a new source window
**
**  Description:
**  This function will setup a mapping of source file names from
**  file name lsz1 to lsz2.  This will allow for a fast reamapping
**  without haveing to do searches or ask the user a second time.
**
*/

int
PASCAL
SrcMapSourceFilename(
    LPSTR lpszSrc,
    UINT cbSrc,
    int flags,
    FINDDOC lpFindDoc,
    BOOL bUserActivated
    )
{
    ATOM    atomFile;
    int     doc;

    /*
    **  Step 1.  Is this file actually in a source window.  If
    **      so then no changes need to be made to the source
    **      file name.
    */
    if (lpFindDoc == NULL) {
        lpFindDoc = FindDoc;
    }

    if ((*lpFindDoc)(lpszSrc, &doc, TRUE)) {
        return (1);
    }

    /*
    **  Step 2. Make sure file is not on the "I'don't want to hear about it
    **      list".  If file has been mapped or exists on disk, it would not
    **      not have been on this list.
    */

    atomFile = FindAtom(lpszSrc);
    if (atomFile != (ATOM) NULL && SrcNameIsMasked(atomFile)) {
        return 0;
    }

    /*
    **  Step 3. Now check to see if the file has been previously remapped
    **      or not
    */

    if ((atomFile != (ATOM) NULL) &&
        SrcNameIsMapped(atomFile, lpszSrc, cbSrc)) {
        if ((*lpFindDoc)(lpszSrc, &doc, TRUE)) {
            return 1;
        } else if (flags & SRC_MAP_OPEN) {
            if (AddFile(MODE_OPEN, DOC_WIN, lpszSrc, NULL, NULL, TRUE, -1, -1, bUserActivated) != -1) {
                return 2;
            }
            return -1;
        }
        return 0;
    }

    /*
    **   Step 4.  The file we are looking for is not on the "I don't
    **      want to hear about it list" and has no forward mapping.
    **      Let's consider opened file of the same name.
    */

    doc = MatchOpenedDoc(lpszSrc, cbSrc);
    if (doc != 0) {
        return(doc);
    }

    /*
    **  Step 5. Does the requested file in fact actually exist on
    **      the disk.  If so then we want to read in the file
    **      if requested and return the correct code.
    **
    **      This step does not need to be taken if we are just remapping
    **      the file name.  In this case we are just interested if we
    **      have done any type of file mapping on the name yet.
    */

    if ((flags & SRC_MAP_OPEN) && FileExist(lpszSrc)) {
        if (flags & SRC_MAP_OPEN) {
            if (AddFile(MODE_OPEN, DOC_WIN, lpszSrc, NULL, NULL, TRUE, -1, -1, bUserActivated) != -1) {
                return 2;
            } else {
                return -1;
            }
        }
        return 1;
    }

    /*
    **  Step 6. Now check to see if the file can be root map
    **      or not
    */

    if (SrcSearchOnRoot(lpszSrc, cbSrc)) {
        if ((*lpFindDoc)(lpszSrc, &doc, TRUE) ) {
            return 1;
        } else if (flags & SRC_MAP_OPEN) {
            if (AddFile(MODE_OPEN, DOC_WIN, lpszSrc, NULL, NULL, TRUE, -1, -1, bUserActivated) != -1) {
                return 2;
            }
            return -1;
        }
        return 0;
    }

    /*
    **  If we are only interested in checking for an existing re-mapping
    **  on the source file then we do not need to go any futher
    */

    if (flags & SRC_MAP_ONLY) {
       return 0;
    }

    /*
    **  Step 7. We must now search the source file path for the file.  This
    **      needs to include the cwd and exe directory for the file
    */

    if (SrcSearchOnPath(lpszSrc, cbSrc, TRUE) || SrcBrowseForFile(lpszSrc, cbSrc)) {
        if ((*lpFindDoc)(lpszSrc, &doc, TRUE)) {
            return 1;
        } else if (flags & SRC_MAP_OPEN) {
            if (AddFile(MODE_OPEN, DOC_WIN, lpszSrc, NULL, NULL, TRUE, -1, -1, bUserActivated) != -1) {
                return 2;
            }
            return -1;
        }
        return 0;
    } else {
        // Error occurred, couldn't find it or something.

        SrcSetMasked(lpszSrc); // add file to the I don't
                               // want to hear about it list.
        return -2;
    }
}                   /* SrcMapSourceFilename() */


/***    SrcBackMapSourceFilename
**
**  Synopsis:
**  int = SrcBackMapSourceFilename(lpszTarget, cbTarget)
**
**  Entry:
**  lpszTarget  - Source buffer to map file from
**  cbTarget    - size of source buffer
**
**  Returns:
**  0 - No mapping done -- no source file mapped
**  1 - Mapping done
**
**  Description:
**  This function will look from a mapping which goes to the
**  file lpszTarget and replace it with the source file.  Thus
**  this code does the opposit of SrcMapSourceFilename.
**
*/

int
SrcBackMapSourceFilename(
    LPSTR lpszTarget,
    UINT cbTarget
    )
{
    ATOM    atomTarget;
    int     i;

    /*
     * Look for the file name in the atom table.  If it can't be found
     * then there must not be any mapping for this file.
     */

    atomTarget = FindAtom(lpszTarget);

    if (atomTarget == (ATOM) NULL) {
        return 0;
    }

    for (i=0; i<CAtomMapped; i++) {
        if (RgAtomMappedNames[i].atomTarget == atomTarget) {
            GetAtomName(RgAtomMappedNames[i].atomSrc, lpszTarget, cbTarget);
            return 1;
        }
    }

    return 0;
}               /* SrcBackMapSourceFilename() */


VOID
SetPTState(
    PSTATEX pstate,
    TSTATEX tstate
    )
/*++

Routine Description:

    Set current process and thread states

Arguments:

    pstate      - Supplies new pstate, or -1 to leave it the same

    tstate      - Supplies new tstate, or -1 to leave it the same

Return Value:

    none

--*/
{
    if (pstate >= 0) {
        if (LppdCur != NULL) LppdCur->pstate = pstate;
    }
    if (tstate >= 0) {
        if (LptdCur != NULL) LptdCur->tstate = tstate;
    }
    EnableToolbarControls();
}               /* SetPTState() */



VOID
AsyncStop(
    void
    )

/*++

Routine Description:

    This routine will send a message to the DM to cause an ASYNC stop
    on the current thread.

Arguments:

    None

Return Value:

    None.

--*/

{
    /*
     *  If the current process is not running then we don't need
     *  to do an ASYNC stop
     */

    if (!LppdCur) {
        MessageBeep(MB_OK);
        CmdLogFmt("Cannot stop the current process\r\n");
        return;
    }

    /*
     *  Send down the ASYNC STOP message
     */

    OSDAsyncStop(LppdCur->hpid, 0);

    return;
}                               /* AsyncStop() */

void
FormatKdParams(
    LPSTR p
    )
{
    LPSTR lpsz;
    int   len;

#define append(s,n)    p=p+sprintf(p,s,n)

    append( "baudrate=%d ", g_contKernelDbgPreferences_WkSp.m_dwBaudRate );
    append( "port=%d ", g_contKernelDbgPreferences_WkSp.m_dwPort );
    append( "cache=%d ", g_contKernelDbgPreferences_WkSp.m_dwCache );
    append( "initialbp=%d ", g_contKernelDbgPreferences_WkSp.m_bInitialBp );
    append( "usemodem=%d ", g_contKernelDbgPreferences_WkSp.m_bUseModem );
    append( "goexit=%d ", g_contKernelDbgPreferences_WkSp.m_bGoExit );
    if (g_contKernelDbgPreferences_WkSp.m_pszCrashDump
        && *g_contKernelDbgPreferences_WkSp.m_pszCrashDump) {

        append( "crashdump=%s ", g_contKernelDbgPreferences_WkSp.m_pszCrashDump );

    }
    len = ModListGetSearchPath( NULL, 0 );
    if (len) {
        lpsz = (PSTR) malloc( len );
        ModListGetSearchPath( lpsz, len );
        append( "symbolpath=%s ", lpsz );
        free(lpsz);
    }
#undef append
}


//
// Root Mapping Routines
//

/***    RootSetMapped
**
**  Synopsis:
**  BOOL = RootSetMapped(lsz1, lsz2)
**
**  Entry:
**  lsz1 - Name of file to be mapped from
**  lsz2 - Name of file to be mapped to
**
**  Returns:
**  TRUE if mapping was successfully recorded
**
**  Description:
**  This function will setup a mapping of source root to target root.
**  This will allow for a fast reamapping without haveing to do
**  searches or ask the user a second time.
**
*/

BOOL
RootSetMapped(
    LSZ lpszSrcRoot,
    LSZ lpszTargetRoot
    )
{
    UINT     i;
    LPSTR    p, q;
    CHAR     chpSaved, chqSaved;

    if (lpszSrcRoot == NULL || lpszTargetRoot == NULL ||
        *lpszSrcRoot == '\0' || *lpszTargetRoot == '\0') {
        return FALSE;  // strlen is zero
    }

    if (!((lpszSrcRoot[1] == ':' ||
            (lpszSrcRoot[0] == '\\' && lpszSrcRoot[1] == '\\')) &&
            (lpszTargetRoot[1] == ':' ||
            (lpszTargetRoot[0] == '\\' && lpszTargetRoot[1] == '\\'))
        )
    ) {
        return FALSE;  // ignore non fullpath for now
    }

    //  Make both strings lower case so we don't have some problems
    _strlwr(lpszSrcRoot);
    _strlwr(lpszTargetRoot);

    p = lpszSrcRoot + strlen(lpszSrcRoot) - 1;
    q = lpszTargetRoot + strlen(lpszTargetRoot) - 1;

    while (p >= lpszSrcRoot && q >= lpszTargetRoot) {
        if (*(p--) != *(q--)) {
            // Move to last matched character
            p += 1;
            q += 1;

            //  If last matched char is a ':' then skip 1 character
            //  If last matched is ":\" then skip 2 characters
            if (*p == ':') {
                p++;
                q++;
                if (*p == '\\') {
                    p++;
                    q++;
                }
            }
            //  If last matched char is a "\" then skip 1 character
            else if (*p == '\\') {
                p++;
                q++;
            }
            //  If last matched is not a "\" then
            //  look for a slash and skip it
            //
            else {
                while (*p != '\\') {
                    p++;
                    q++;
                }
                p++;
                q++;
            }

            chpSaved = *p;
            chqSaved = *q;
            *p = *q = '\0';
            for (i=0; i<CMappedRoots; i++) {
                if (_stricmp(RgMappedRoots[i].lpszSrcRoot, lpszSrcRoot) == 0 &&
                    _stricmp(RgMappedRoots[i].lpszTargetRoot, lpszTargetRoot) == 0)
                {
                    *p = chpSaved;
                    *q = chqSaved;
                    return TRUE; // already there
                }
            }
            if (CMappedRoots >= MAX_MAPPED_ROOTS) {
                memcpy(RgMappedRoots,
                       &(RgMappedRoots[1]),
                       sizeof(struct MRootPair)*--CMappedRoots);
            }

            if ((RgMappedRoots[CMappedRoots].lpszSrcRoot = _strdup(lpszSrcRoot)) == NULL) {
                *p = chpSaved;
                *q = chqSaved;
                return FALSE;
            }
            if ((RgMappedRoots[CMappedRoots].lpszTargetRoot = _strdup(lpszTargetRoot)) == NULL) {
                free(RgMappedRoots[CMappedRoots].lpszSrcRoot);
                *p = chpSaved;
                *q = chqSaved;
                return FALSE;  // error - out of memory? - skip the mapping
            }

            RgMappedRoots[CMappedRoots++].dwSrcLen = strlen(lpszSrcRoot);
            *p = chpSaved;
            *q = chqSaved;
            return (TRUE);
        }

    }
return (FALSE);
}  /* RootSetMapped() */

/***    RootNameIsMapped
**
**  Synopsis:
**  BOOL = RootNameIsMapped(lpb, lpszDest, cbDest)
**
**  Entry:
**  lpb      - Name of file to try for root mapping
**  lpszDest - Name of file root mapped to
**  cbDest   - Size of lpszDest buffer
**
**  Returns:
**  TRUE if mapping was successfully done
**
**  Description:
**  This function will try to map the given source in lpb to it's mapped
**  location.  If file does exists, it will return TRUE and the new full
**  file path thru lpszDest.
**
*/

BOOL
RootNameIsMapped(
   LPSTR lpb,
   LPSTR lpszDest,
   UINT cbDest
)
{
   struct MRootPair  *lpRoot;
   char              rgch[_MAX_PATH];
   char              *p;
   INT              i;

   for (i=(INT)CMappedRoots-1; i >= 0; i--) {
      lpRoot = &(RgMappedRoots[i]);
      if (_strnicmp(lpRoot->lpszSrcRoot, lpb, lpRoot->dwSrcLen) == 0) {
         strcpy(rgch, lpRoot->lpszTargetRoot);
         p = rgch + strlen(rgch) - 1;
         if (*p == '\\') {
            *p = 0;
         }
         if (FindNameOn(lpszDest, cbDest, rgch, lpb+lpRoot->dwSrcLen)) {
            return(TRUE);
         }
      }
   }
   return(FALSE);
}

/***    GetRootNameMappings
**
**  Synopsis:
**  BOOL = GetRootNameMappings(String, Length)
**
**  Entry:
**  String - Address of the variable pointing to the multistring
**  Length - Address of Length of the multi-string
**
**  Returns:
**  TRUE if operation completed successfully
**
**  Description:
**  This function scans thru all the root mappings and returns
**  all the source and target roots in the form of a multi-string
**
*/
BOOL
GetRootNameMappings(
   LPSTR *String,
   DWORD *Length
)
{
   struct MRootPair  *lpRoot = RgMappedRoots;
   UINT              i;

   for (i=0; i<CMappedRoots; lpRoot++, i++) {
      if (!AddToMultiString(String, Length, lpRoot->lpszSrcRoot))
         return(FALSE);
      if (!AddToMultiString(String, Length, lpRoot->lpszTargetRoot))
         return(FALSE);
   }
   return(TRUE);
}


/***    SetRootNameMappings
**
**  Synopsis:
**  BOOL = SetRootNameMappings(String, Length)
**
**  Entry:
**  String - Pointer to the multistring
**  Length - Length of the multi-string
**
**  Returns:
**  TRUE if operation completed successfully
**
**  Description:
**  This function clears all the current root mappings
**  and reconstruct the root mappings table thru the given
**  multi-string.
**
*/
BOOL
SetRootNameMappings(
   LPSTR String,
   DWORD Length
)
{
   struct MRootPair  *lpRoot = RgMappedRoots;
   DWORD             Next = 0;
   UINT              i;
   LPSTR             lpsztmp1, lpsztmp2;

   for (i=0; i<CMappedRoots; lpRoot++, i++) {
      if (lpRoot->lpszSrcRoot)
         free(lpRoot->lpszSrcRoot);
      if (lpRoot->lpszTargetRoot)
         free(lpRoot->lpszTargetRoot);
   }
   CMappedRoots = 0;

   lpRoot = RgMappedRoots;
   while ((lpsztmp1 = GetNextStringFromMultiString(String, Length, &Next)) &&
          (lpsztmp2 = GetNextStringFromMultiString(String, Length, &Next))) {
      lpRoot->dwSrcLen = strlen(lpsztmp1);
      if ((lpRoot->lpszSrcRoot = _tcsdup(lpsztmp1)) == NULL)
         return (FALSE);
      if ((lpRoot->lpszTargetRoot = _tcsdup(lpsztmp2)) == NULL) {
         free(lpRoot->lpszSrcRoot);
         return (FALSE);
      }

      lpRoot++;
      CMappedRoots++;

      if (CMappedRoots >= MAX_MAPPED_ROOTS)
         return (GetNextStringFromMultiString(String, Length, &Next) == NULL);
      lpsztmp2 = NULL;
   }
   return (lpsztmp1 == NULL);
}

INT_PTR
WINAPI
DlgFileSearchResolve(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    HDC         hdc;
    int         i;
    int         j;
    int         Idx;
    int         LargestString = 0;
    SIZE        Size;
    HWND        hList;
    LPSTR       lpszName;
    CHAR        rgch[_MAX_PATH];

    static DWORD HelpArray[]=
    {
        ID_FSRESOLVE_LIST, IDH_FSRESOLVE_LIST,
        ID_FSRESOLVE_USE, IDH_FSRESOLVE_USE,
        ID_FSRESOLVE_ADDNONE, IDH_FSRESOLVE_ADD,
        ID_FSRESOLVE_ADDROOT, IDH_FSRESOLVE_ADD,
        ID_FSRESOLVE_ADDSOURCE, IDH_FSRESOLVE_ADD,
        0, 0
    };

    Unreferenced( lParam );

    switch( msg ) {

    case WM_INITDIALOG:

        SetDlgItemText(hDlg, 
                       ID_FSRESOLVE_STRING, 
                       szFSSrcName
                       );

        LoadString(g_hInst, DLG_ResolveFSCaption, rgch, sizeof(rgch));

        SetWindowText( hDlg, rgch );

        hList = GetDlgItem(hDlg, ID_FSRESOLVE_LIST);

        //
        // Add the name of the file that was obtained from the image.
        //
        Idx = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM) (LPCSTR) szFSSrcName);
        if (Idx >= LB_OKAY) {
            //
            // String successfully added to the list box.
            //
            SendMessage(hList, 
                        LB_SETITEMDATA, 
                        (WPARAM) Idx, 
                        (LPARAM) MATCHLIST_USEFILEFROMIMAGE
                        );
        }

        for (i=0; i < lMatchCnt; i++) {
            
            lpszName = Docs[MatchedList[i]].szFileName;
            
            Idx = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)lpszName);
            if (Idx >= LB_OKAY) {
                //
                // String successfully added to the list box.
                //
                SendMessage(hList, 
                            LB_SETITEMDATA, 
                            (WPARAM) Idx, 
                            (LPARAM) MatchedList[i]
                            );
            }
        }

        SendMessage(hList, LB_SETCURSEL, 0, 0L);

        CheckRadioButton(hDlg, ID_FSRESOLVE_ADDNONE, ID_FSRESOLVE_ADDSOURCE,
                         ID_FSRESOLVE_ADDNONE);

        lMatchIdx = MATCHLIST_NONEMATCHED;

        return TRUE;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                "windbg.hlp",
                HELP_WM_HELP,
                (ULONG_PTR) HelpArray 
                );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam,
                "windbg.hlp",
                HELP_CONTEXTMENU,
                (ULONG_PTR) HelpArray 
                );
        return TRUE;

    case WM_COMMAND:

        switch( LOWORD( wParam ) ) {
        case ID_FSRESOLVE_USE:
            Idx = SendDlgItemMessage(hDlg, 
                                     ID_FSRESOLVE_LIST,
                                     LB_GETCURSEL, 
                                     0, 
                                     0
                                     );

            lMatchIdx = SendDlgItemMessage(hDlg,
                                           ID_FSRESOLVE_LIST,
                                           LB_GETITEMDATA,
                                           (WPARAM) Idx,
                                           0
                                           );
            Assert(lMatchIdx < lMatchCnt);

            FAddToSearchPath = IsDlgButtonChecked( hDlg, ID_FSRESOLVE_ADDSOURCE );
            FAddToRootMap = IsDlgButtonChecked(hDlg, ID_FSRESOLVE_ADDROOT);
            if (FAddToSearchPath || FAddToRootMap) {
                Assert(FAddToSearchPath != FAddToRootMap);
                Assert(IsDlgButtonChecked(hDlg, ID_FSRESOLVE_ADDNONE) == FALSE);
            } else {
                Assert(IsDlgButtonChecked(hDlg, ID_FSRESOLVE_ADDNONE));
            }
            EndDialog(hDlg, TRUE);
            return TRUE;
            
            
        case IDCANCEL:        // none
            lMatchIdx = -1;
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

INT_PTR
CALLBACK
DlgProc_EditRootMapping(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    Dlg proc for the user to enter a source root mapping pair.

Arguments:

    lParam - Supplies a valid "MRootPair" struct.

Return Value:

    TRUE if the WM message was handled, FALSE if not.


--*/
{
    static struct MRootPair * lpMRootPair = NULL;

    static DWORD HelpArray[]=
    {
        IDC_EDIT_SRCROOT, IDH_EDIT_SRCROOT,
        IDC_EDIT_TARGETROOT, IDH_EDIT_TARGETROOT,
        0, 0
    };

    switch (uMsg) {
    case WM_INITDIALOG:
        Dbg(lParam);

        lpMRootPair = (struct MRootPair *) lParam;

        SetDlgItemText(hwndDlg, IDC_EDIT_SRCROOT, lpMRootPair->lpszSrcRoot);
        SetDlgItemText(hwndDlg, IDC_EDIT_TARGETROOT, lpMRootPair->lpszTargetRoot);

        // Limit the amount of text
        SendDlgItemMessage(hwndDlg, IDC_EDIT_SRCROOT, EM_LIMITTEXT, _MAX_PATH-1, 0);
        SendDlgItemMessage(hwndDlg, IDC_EDIT_TARGETROOT, EM_LIMITTEXT, _MAX_PATH-1, 0);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                "windbg.hlp",
                HELP_WM_HELP,
                (ULONG_PTR) HelpArray );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp ((HWND) wParam,
                 "windbg.hlp",
                 HELP_CONTEXTMENU,
                 (ULONG_PTR) HelpArray );
        return TRUE;

    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam);  // notification code
            WORD wID = LOWORD(wParam);          // item, control, or accelerator identifier
            HWND hwndCtl = (HWND) lParam;       // handle of control
            BOOL bEnabled;

            switch(wID) {
            case IDOK:
                GetDlgItemText(hwndDlg, IDC_EDIT_SRCROOT, lpMRootPair->lpszSrcRoot, MAX_PATH);
                GetDlgItemText(hwndDlg, IDC_EDIT_TARGETROOT, lpMRootPair->lpszTargetRoot, MAX_PATH);

                // Can't have blanks
                if (0 == strlen(lpMRootPair->lpszSrcRoot)
                    || 0 == strlen(lpMRootPair->lpszTargetRoot)) {

                    char sz[MAX_MSG_TXT];

                    LoadString(g_hInst, ERR_Entries_Cant_be_blank, sz, sizeof(sz));
                    MessageBox(hwndDlg, sz, NULL, MB_OK | MB_ICONWARNING | MB_APPLMODAL);
                } else {
                    EndDialog(hwndDlg, IDOK);
                }
                return TRUE;
                break;

            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                return TRUE;
                break;
            }
        }
        break;
    }

    return FALSE;
}


void
Enable_DlgProc_SrcSearchPath_Buttons(
    HWND hwndDlg
    )
/*++
Routine Description:

    Controls the enabling/disabling of the buttons add, edit, delete, up, down
    buttons.

Arguments:

    hwndDlg     - Dlg window

Return Value:

--*/
{
    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
    HWND hwndAdd = GetDlgItem(hwndDlg, IDC_BUT_ADD);
    HWND hwndEdit = GetDlgItem(hwndDlg, IDC_BUT_EDIT);
    HWND hwndDelete = GetDlgItem(hwndDlg, IDC_BUT_DELETE);
    HWND hwndMoveUp = GetDlgItem(hwndDlg, IDC_BUT_MOVE_UP);
    HWND hwndMovedown = GetDlgItem(hwndDlg, IDC_BUT_MOVE_DOWN);

    Assert(hwndList);
    Assert(hwndAdd);
    Assert(hwndEdit);
    Assert(hwndDelete);
    Assert(hwndMoveUp);
    Assert(hwndMovedown);


    int nCurSel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    int nCount = ListView_GetItemCount(hwndList);


    // As long as we don't exceed the max number of mappings.
    EnableWindow(hwndAdd, nCount < MAX_MAPPED_ROOTS);

    // As long as something is selected.
    EnableWindow(hwndEdit, (-1 != nCurSel));
    EnableWindow(hwndDelete, (-1 != nCurSel));

    // Something must be selected and it must not be at the top.
    EnableWindow(hwndMoveUp, (0 < nCurSel));

    // Something must be selected and it must not be at the bottom.
    EnableWindow(hwndMovedown, (-1 != nCurSel && nCurSel < nCount-1));
}


// This is not directly used by the prop sheet but by the IDD_DLG_SRC_SEARCH_PATH dlg
// that is invoked from the DlgProc_SourceFiles function by pressing the "Search Order" button.
INT_PTR
CALLBACK
DlgProc_SrcSearchPath(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    Dlg proc

--*/
{
    static DWORD HelpArray[]=
    {
       IDC_LIST1, IDH_MAP,
       IDC_BUT_ADD, IDH_ADD,
       IDC_BUT_EDIT, IDH_EDIT,
       IDC_BUT_DELETE, IDH_DEL,
       IDC_BUT_MOVE_UP, IDH_MOVEUP,
       IDC_BUT_MOVE_DOWN, IDH_MOVEDOWN,
       IDC_EDIT_DEF_SRC_SEARCH_PATH, IDH_DEFPATH,
       IDC_HELP_DEF_SRC_SEARCH_PATH, IDH_SEARCHPATH,
       0, 0
    };

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            HWND                hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
            char                szColHdr[MAX_MSG_TXT];
            UINT                u;
            LV_COLUMN           lvc;

            Assert(hwndList);

            // Limit the text to a multiple of MAX_PATH
            SendDlgItemMessage(hwndDlg, IDC_EDIT_DEF_SRC_SEARCH_PATH, EM_LIMITTEXT, MAX_PATH-1, 0);

            ListView_DeleteAllItems(hwndList);

            //
            // Set the extended style
            //
            ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);


            ///////////////////////////////////////////
            // Setup the column header for the list view
            ///////////////////////////////////////////

            //
            // Add Column headers
            //

            // Initialize the LV_COLUMN structure.
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 300;
            lvc.pszText = szColHdr;

            // Add the 1st column hdr
            Dbg(LoadString(g_hInst, SYS_SRC_ROOT_MAPPING_COL_HDR1,
                szColHdr, sizeof(szColHdr)));
            lvc.iSubItem = 0;
            Dbg(ListView_InsertColumn(hwndList, lvc.iSubItem, &lvc) != -1);

            // Add the 2nd col hdr
            Dbg(LoadString(g_hInst, SYS_SRC_ROOT_MAPPING_COL_HDR2,
                szColHdr, sizeof(szColHdr)));
            lvc.iSubItem = 1;
            Dbg(ListView_InsertColumn(hwndList, lvc.iSubItem, &lvc) != -1);

            ///////////////////////////////////////////
            // Add the data to the list view
            ///////////////////////////////////////////

            //
            // Add the individual items
            //
            for (u=0; u<CMappedRoots; u++) {
                InsertListViewItem(hwndList, 
                                   u, 
                                   NULL, 
                                   2,
                                   RgMappedRoots[u].lpszSrcRoot, 
                                   RgMappedRoots[u].lpszTargetRoot
                                   );
            }

            // Select the first item
            ListView_SetItemState(hwndList, 0, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);

            if (g_contPaths_WkSp.m_pszSourceCodeSearchPath) {
                
                SetDlgItemText(hwndDlg, 
                               IDC_EDIT_DEF_SRC_SEARCH_PATH, 
                               g_contPaths_WkSp.m_pszSourceCodeSearchPath
                               );
            }

            Enable_DlgProc_SrcSearchPath_Buttons(hwndDlg);
        }
        return TRUE;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                "windbg.hlp",
                HELP_WM_HELP,
                (ULONG_PTR) HelpArray );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp ((HWND) wParam,
                 "windbg.hlp",
                 HELP_CONTEXTMENU,
                 (ULONG_PTR) HelpArray );
        return TRUE;

    case WM_NOTIFY:
        // It has to be from the list view control. Its the
        // only thing on this Dlg box that would send a
        // WM_NOTIFY msg.
        Enable_DlgProc_SrcSearchPath_Buttons(hwndDlg);
        break;


    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam);  // notification code
            WORD wID = LOWORD(wParam);          // item, control, or accelerator identifier
            HWND hwndCtl = (HWND) lParam;       // handle of control
            BOOL bEnabled;

            switch(wID) {
            case IDC_BUT_ADD:
                if (BN_CLICKED == wNotifyCode) {
                    struct MRootPair mrp;
                    // These must be MAX_PATH or greater in size.
                    char szSrcRoot[MAX_PATH] = {0}, szTargetRoot[MAX_PATH] = {0};
                    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);

                    Assert(hwndList);

                    mrp.lpszSrcRoot = szSrcRoot;
                    mrp.lpszTargetRoot = szTargetRoot;

                    // Pass the values into the dlg
                    if (IDOK == DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_EDIT_SRC_ROOT_MAPPING),
                        hwndDlg, DlgProc_EditRootMapping, (LPARAM) &mrp)) {

                        Dbg(strlen(szSrcRoot) < sizeof(szSrcRoot));
                        Dbg(strlen(szTargetRoot) < sizeof(szTargetRoot));

                        InsertListViewItem(hwndList, ListView_GetItemCount(hwndList), NULL, 2,
                            szSrcRoot, szTargetRoot);
                    }

                    Enable_DlgProc_SrcSearchPath_Buttons(hwndDlg);
                    return TRUE;
                }
                break;

            case IDC_BUT_EDIT:
                if (BN_CLICKED == wNotifyCode) {
                    struct MRootPair mrp;
                    // These must be MAX_PATH or greater in size.
                    char szSrcRoot[MAX_PATH] = {0}, szTargetRoot[MAX_PATH] = {0};
                    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
                    int nCurSel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

                    Assert(hwndList);
                    Assert(-1 != nCurSel);

                    mrp.lpszSrcRoot = szSrcRoot;
                    mrp.lpszTargetRoot = szTargetRoot;

                    ListView_GetItemText(hwndList, nCurSel, 0, szSrcRoot, sizeof(szSrcRoot));
                    ListView_GetItemText(hwndList, nCurSel, 1, szTargetRoot, sizeof(szTargetRoot));

                    if (IDOK == DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_EDIT_SRC_ROOT_MAPPING),
                        hwndDlg, DlgProc_EditRootMapping, (LPARAM) &mrp)) {

                        Dbg(strlen(szSrcRoot) < sizeof(szSrcRoot));
                        Dbg(strlen(szTargetRoot) < sizeof(szTargetRoot));

                        ListView_SetItemText(hwndList, nCurSel, 0, szSrcRoot);
                        ListView_SetItemText(hwndList, nCurSel, 1, szTargetRoot);
                    }

                    Enable_DlgProc_SrcSearchPath_Buttons(hwndDlg);
                    return TRUE;
                }
                break;

            case IDC_BUT_DELETE:
                if (BN_CLICKED == wNotifyCode) {
                    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
                    Assert(hwndList);
                    int nCurSel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

                    ListView_DeleteItem(GetDlgItem(hwndDlg, IDC_LIST1), nCurSel);

                    Enable_DlgProc_SrcSearchPath_Buttons(hwndDlg);
                    return TRUE;
                }
                break;

            case IDC_BUT_MOVE_UP:
                if (BN_CLICKED == wNotifyCode) {
                    // These must be MAX_PATH or greater in size.
                    char szSrcRoot[MAX_PATH] = {0}, szTargetRoot[MAX_PATH] = {0};
                    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
                    int nCurSel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
                    LV_ITEM lvi;

                    Assert(hwndList);
                    Assert(-1 != nCurSel);

                    ListView_GetItemText(hwndList, nCurSel, 0, szSrcRoot, sizeof(szSrcRoot));
                    ListView_GetItemText(hwndList, nCurSel, 1, szTargetRoot, sizeof(szTargetRoot));

                    memset(&lvi, 0, sizeof(lvi));
                    lvi.iItem = 0;
                    lvi.iSubItem = 0;
                    lvi.mask = LVIF_STATE;
                    lvi.state = LVIS_CUT | LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED;
                    ListView_GetItem(hwndList, &lvi);

                    ListView_DeleteItem(hwndList, nCurSel);

                    InsertListViewItem(hwndList, nCurSel-1, &lvi, 2,
                        szSrcRoot, szTargetRoot);

                    Enable_DlgProc_SrcSearchPath_Buttons(hwndDlg);
                    return TRUE;
                }
                break;

            case IDC_BUT_MOVE_DOWN:
                if (BN_CLICKED == wNotifyCode) {
                    // These must be MAX_PATH or greater in size.
                    char szSrcRoot[MAX_PATH] = {0}, szTargetRoot[MAX_PATH] = {0};
                    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
                    int nCurSel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
                    LV_ITEM lvi;

                    Assert(hwndList);
                    Dbg(-1 != nCurSel);

                    ListView_GetItemText(hwndList, nCurSel, 0, szSrcRoot, sizeof(szSrcRoot));
                    ListView_GetItemText(hwndList, nCurSel, 1, szTargetRoot, sizeof(szTargetRoot));

                    memset(&lvi, 0, sizeof(lvi));
                    lvi.iItem = 0;
                    lvi.iSubItem = 0;
                    lvi.mask = LVIF_STATE;
                    lvi.state = LVIS_CUT | LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED;
                    ListView_GetItem(hwndList, &lvi);

                    ListView_DeleteItem(hwndList, nCurSel);

                    InsertListViewItem(hwndList, nCurSel+1, &lvi, 2,
                        szSrcRoot, szTargetRoot);

                    Enable_DlgProc_SrcSearchPath_Buttons(hwndDlg);
                    return TRUE;
                }
                break;

            case IDCANCEL:
                if (BN_CLICKED == wNotifyCode) {
                    EndDialog(hwndDlg, wID);
                    return TRUE;
                }
                break;

            case IDHELP:
                WinHelp( hwndDlg, "windbg.hlp", HELP_CONTEXT, IDH_SEARCHPATH );
                break;

            case IDOK:
                if (BN_CLICKED == wNotifyCode) {
                    unsigned int        tmp_int;
                    char sz[MAX_PATH];
                    LPSTR lpszBuffer = NULL;
                    int nCount = 0, i;
                    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);

                    Assert(hwndList);

                    GetDlgItemText(hwndDlg, IDC_EDIT_DEF_SRC_SEARCH_PATH, sz, sizeof(sz));
                    FREE_STR( g_contPaths_WkSp.m_pszSourceCodeSearchPath );
                    g_contPaths_WkSp.m_pszSourceCodeSearchPath = _strdup(sz);

                    // Now do the src root mapping
                    tmp_int = ListView_GetItemCount(hwndList);
                    nCount = min(MAX_MAPPED_ROOTS, tmp_int);

                    // Zero the whole thing out.
                    lpszBuffer = (LPSTR) calloc(MAX_PATH * nCount +1, sizeof(char));

                    if (!lpszBuffer) {
                        ErrorBox(ERR_Cannot_Allocate_Memory);
                    } else {
                        LPSTR lpsz = lpszBuffer;
                        int nLen = 0;

                        for (i=0; i< nCount; i++) {
                            ListView_GetItemText(hwndList, i, 0, lpsz, MAX_PATH);
                            nLen += strlen(lpsz) +1;
                            lpsz += strlen(lpsz) +1;
                            ListView_GetItemText(hwndList, i, 1, lpsz, MAX_PATH);
                            nLen += strlen(lpsz) +1;
                            lpsz += strlen(lpsz) +1;
                        }
                        //nLen++; // Final zero
                        Dbg(SetRootNameMappings(lpszBuffer, nLen));

                        FREE_STR( g_contPaths_WkSp.m_pszRootMappingPairs );
                        g_contPaths_WkSp.m_pszRootMappingPairs = lpszBuffer;
                    }

                    EndDialog(hwndDlg, wID);
                    return TRUE;
                }
                break;
            }
        }
        break;

    }

    return FALSE;
}
