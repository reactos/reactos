/*** BRKPT0.c - breakpoint handlers                                     *
*
*   Copyright <C> 1989, Microsoft Corporation
*
*   Purpose:
*
*
*************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define BRKPOINT_STATUS (WORD)((lpBpp->bpstate & bpstateNotSet) ? UBP_LINE : BRKPOINT_LINE)

#include    "bpitypes.h"

#define atmNull (0)

extern SHF * Lpshf;
extern CXF  CxfIp;

#define assert(a)

//
//  Breakpoint flags
//
//#define BP_ONETHREAD    0x0001
//#define BP_CODE         0x0002
#define BP_DATA_READ    0x0004
#define BP_DATA_WRITE   0x0008
#define BP_EXECUTE      0x0010
#define BP_DATA_CHANGED 0x0020



#ifdef dcvotxs
#undef LOCAL
#define LOCAL
#endif // dcvotxs

BPSTATUS BPBindHbptAddr( HBPT hBpt, LPBPP lpBpp, BOOL fOnList, CXF *pCxf );
BPSTATUS BPBindHbptNoAddr( HBPT hBpt, LPBPP lpBpp, BOOL fOnList, CXF *pCxf );

LOCAL BPSTATUS  BPParseLineNumber(char *, LPBPP);
LOCAL BPSTATUS  BpGetHtm(LPBPP lpBpp, char * szBpt, BOOL fExpr);
LOCAL BPSTATUS  BPCheckExprLpBpp(LPBPP lpbpp, CXF cxf, BOOL * fStop);

//
//  BPS + 1 BPIS + 0 Msg + 1 XOSD + 1 Notification
//
BYTE    _BpsBuffer[ sizeof(BPS) + sizeof(BPIS) + sizeof(XOSD) + sizeof(DWORDLONG)];
#define BpsBuffer ((LPBPS)_BpsBuffer)

LOCAL VOID BPInitBps( LPBPP lpBpp, LPBPS Bps, BOOL Set );

BOOLEAN BPIsThunk(LPBPP lpBpp, TML TMLT );

BPSTATUS    BPGetTmList( BPP * const lpBpp, TML * const pTml, char *BpBuf, int BufSize );
VOID        SplitExpression( char *BpBuf, char **Source, char **Exe );
BPSTATUS    BPSearchContexts( LPBPP lpBpp, char * szAddrExpr, PTML  pTml, BOOL ImplementationOnly, BOOL LoadSymbols );
BPSTATUS    BPBindFromTmList( LPBPP lpBpp, PTML pTml, BOOL fOnList, char *BpBuf );
BPSTATUS BPRemoveHighLight( LPBPP lpBpp );
BPSTATUS BPAddHighLight( LPBPP lpBpp );


#define OPENCONTEXTOP   '{'
#define CLOSECONTEXTOP   '}'

/*************************  Local Variable Decls ***************************/
PLLI    PlliBpList;

#ifdef DOS5
HATOMTBL    hAtmTable;
#endif

LPPD    LppdOld;
LPTD    LptdOld;
int     MemoryBpCount = 0;  //  Keeps track of number of memory Breakpoints
static  BOOL SymbolLoadingEnabled = TRUE;

/*************************  Local Prototypes *******************************/

static BOOL FParseableSz(char * sz);
static void BPBoundFixContext( LPBPP pBpp );

BPSTATUS BPBindTML(
    PHTM    pHtm,
    LPBPP   lpBpp,
    BOOL    fOnList
    );

BPSTATUS BPSetPidTid (
    LPBPP   lpBpp
    );

BOOL
ParseBpMsg(
    LPBPP        lpBpp,
    LPMESSAGEMAP MsgMap
    );

BOOL GetMsg(
    LPBPP   lpBpp,
    CXF     cxf,
    DWORD   *Msg
    );

BOOL FindMsg(
    LPBPP        lpBpp,
    LPMESSAGEMAP MsgMap
    );

BOOL MatchMsgClass(
    LPBPP lpBpp,
    LPMESSAGEMAP MsgMap,
    DWORD Msg
    );

int
WINAPIV 
MsgCmp (
    const void *p1,
    const void *p2
    );

VOID SaveDebuggerPdTd (
    void
    );

VOID RestoreDebuggerPdTd (
    void
    );

VOID SetDebuggerPdTd(
    LPBPP lpBpp
    );

VOID BPReplace(
    LPBPP lpBpp
    );


/**********************  CODE   ********************************************/

/***    BPLLCmp
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

int
BPLLCmp(
    LPV lpv1,
    LPV lpv2,
    LONG l
    )
{
    LPBPP a = (LPBPP) lpv1;
    LPBPP b = (LPBPP) lpv2;

    //  Assert(a->iBreakpointNum != -1);
    //  Assert(b->iBreakpointNum != -1);

    Unreferenced( l );

    if (a->iBreakpointNum < b->iBreakpointNum) {
        return ( fCmpLT );
    }
    if (a->iBreakpointNum > b->iBreakpointNum) {
        return ( fCmpGT );
    }
    return( fCmpEQ );
}                                   /* BPLLCmp()*/


void
BPKillNode(
    PVOID pvArg
    )

{
    Assert(pvArg);

    LPBPP pBpp = (LPBPP) pvArg;

    if (pBpp->atmAddrCxt) {
        DeleteAtom(pBpp->atmAddrCxt);
        pBpp->atmAddrCxt = NULL;
    }
    if (pBpp->atmCxtDef) {
        DeleteAtom( pBpp->atmCxtDef );
        pBpp->atmCxtDef = NULL;
    }
    if (pBpp->atmData) {
        DeleteAtom(pBpp->atmData);
        pBpp->atmData = NULL;
    }
    if (pBpp->atmExpr) {
        DeleteAtom(pBpp->atmExpr);
        pBpp->atmExpr = NULL;
    }
    if ( pBpp->atmHighlightFile ) {
        DeleteAtom( pBpp->atmHighlightFile );
        pBpp->atmHighlightFile = NULL;
    }
    if ( pBpp->atmMsg ) {
        DeleteAtom( pBpp->atmMsg );
        pBpp->atmMsg = NULL;
    }
    if (pBpp->atmPassCount) {
        DeleteAtom(pBpp->atmPassCount);
        pBpp->atmPassCount = NULL;
    }
    if (pBpp->atmRangeSize) {
        DeleteAtom(pBpp->atmRangeSize);
        pBpp->atmRangeSize = NULL;
    }

    if (pBpp->szAddrExpr) {
        free(pBpp->szAddrExpr);
        pBpp->szAddrExpr = NULL;
    }
    if (pBpp->szCmdLine) {
        free(pBpp->szCmdLine);
        pBpp->szCmdLine = NULL;
    }

    if (pBpp->hTm) {
        EEFreeTM (&pBpp->hTm);
        pBpp->hTm = NULL;
    }
}


/***    BPInit
**
**  Synopsis:
**      bpstatus = BPInit()
**
**  Entry:
**      None
**
**  Returns:
**      BPNOERROR
**
**  Description:
**      Setup initializations needed for the breakpoint engine.
**
*/

BPSTATUS
BPInit()
{
#ifdef DOS5
    hAtmTable = WinCreateAtomTable(100, 10);
#endif
#if defined(WIN32) || defined(WINDOWS3)
    InitAtomTable(29);
#endif

    PlliBpList = LLPlliInit(sizeof(BPP), llfAscending, BPKillNode, BPLLCmp);
    return BPNOERROR;
}                                       /* BPInit() */


/***    BPTerm
**
**  Synopsis:
**      bpstatus = BPTerm()
**
**  Entry:
**      None
**
**  Returns:
**      BPNOERROR
**
**  Description:
**      This routine does all the clean-up required to run the
**      breakpoint handler interface
**
*/

BPSTATUS BPTerm()
{
    return BPNOERROR;
}                               /* BPTerm() */


/***    BPAddToList
**
**  Synopsis:
**      bpstatus = BPAddToList( hBpt, iBp )
**
**  Entry:
**      hBpt    - Handle to break point to be added onto the list
**      iBp     - index to assign to breakpoint
**
**  Returns:
**      bpstatus error code
**
**  Description:
**      This function is used to place a breakpoint onto the list of
**      breakpoints in the system.  It will also assign a breakpoint
**      number to the breakpoint.
*/

BPSTATUS
BPAddToList(
            HBPT hBpt,
            int iBp
            )
{
    LPBPP               lpBpp = (LPBPP) LLLpvFromPlle(hBpt);
    HBPT                hBpt2 = NULL;
    LPBPP               lpBpp2 = NULL;
    DWORD               micBreakpointNum = (DWORD) -1;

    if (iBp != -1) {
        if (BPHbptFromI( &hBpt2, iBp ) != BPNoMatch) {
            return BPAmbigous;
        }
    }

    /*
    **  Need to assign a breakpoint number to the item
    */

    Dbg( BPNextHbpt( &hBpt2, bptFirst ) == BPNOERROR);

    if (iBp == -1) {
        if (hBpt2 == NULL) {
            lpBpp->iBreakpointNum = 0;
        } else {
            do {
                lpBpp2 = (LPBPP) LLLpvFromPlle( hBpt2 );
                if ((lpBpp2->fMarkDelete) &&
                    (lpBpp2->iBreakpointNum > micBreakpointNum)) {
                    lpBpp->iBreakpointNum = lpBpp2->iBreakpointNum;
                    break;
                }

                if (micBreakpointNum + 1 < lpBpp2->iBreakpointNum) {
                    lpBpp->iBreakpointNum = micBreakpointNum+1;
                    break;
                }
                micBreakpointNum = lpBpp2->iBreakpointNum;
                Dbg( BPNextHbpt(&hBpt2, bptNext) == BPNOERROR);
            } while (hBpt2 != NULL);

            if (hBpt2 == NULL) {
                lpBpp->iBreakpointNum = micBreakpointNum + 1;
            }
        }
    } else {
        lpBpp->iBreakpointNum = iBp;
    }

    /*
    **  Mark as "to be added"
    */

    lpBpp->fNewMark = TRUE;
    lpBpp->fMarkAdd = TRUE;


    LLInsertPlleInLl( PlliBpList, hBpt, 0);
    return BPNOERROR;
}                                   /* BPAddToList() */


/***    BPChange
**
**  Synopsis:
**      bpstatus = BPChange( hBpt, iBp )
**
**  Entry:
**      hBpt    - Handle to new breakpoint
**      iBp     - index of breakpoint to change
**
**  Returns:
**      bpstatus error code
**
**  Description:
**      This function is used to change a breakpoint
*/

BPSTATUS
BPChange(
         HBPT hBpt,
         int iBp
         )
{
    BPSTATUS    BpStatus = BPNOERROR;
    LPBPP       lpBpp = NULL;
    HBPT        hBpt2 = NULL;
    LPBPP       lpBpp2 = NULL;

    if ( BPHbptFromI( &hBpt2, iBp ) == BPNoMatch) {

        BpStatus = BPNoMatch;

    } else {

        lpBpp2 = (LPBPP) LLLpvFromPlle( hBpt2 );

        if ( !lpBpp2 ) {

            BpStatus = BPError;

        } else {

            lpBpp = (LPBPP) LLLpvFromPlle( hBpt );

            if ( lpBpp ) {

                //
                //  If BP is already changed, get rid of previous
                //  replacement.
                //
                if ( lpBpp2->fMarkChange ) {
                    LLFDeletePlleFromLl( PlliBpList, lpBpp2->ChangeHbpt );
                }

                lpBpp2->fMarkChange   = TRUE;
                lpBpp2->ChangeHbpt    = hBpt;

                lpBpp->fMarkAdd       = TRUE;
                lpBpp->fMarkEnable    = lpBpp2->fMarkEnable;
                lpBpp->fMarkDisable   = lpBpp2->fMarkDisable;
                lpBpp->iBreakpointNum = lpBpp2->iBreakpointNum;
                lpBpp->fReplacement   = TRUE;

            } else {

                BpStatus = BPError;
            }

        }
    }

    return BpStatus;
}

/***    BPGetFinalHbpt
**
**  Synopsis:
**      bpstatus = BPGetFinalHbpt( hBpt, phBpt)
**
**  Entry:
**      hBpt    - breakpoint handle
**      phBpt   - pointer to final handle
**
**  Returns:
**      BPNOERROR if no error occurs and modifies *phBpt to point to
**      either the original breakpoint handle or to the replacement
**      breakpoint handle, if the given breakpoint is marked for
**      change.
**
**  Description:
**
*/

BPSTATUS
BPGetFinalHbpt(
               HBPT hBpt,
               HBPT * phBpt
               )
{
    LPBPP   lpBpp = (LPBPP) LLLpvFromPlle( hBpt );


    if ( lpBpp->fMarkChange ) {
        *phBpt = lpBpp->ChangeHbpt;
    } else {
        *phBpt = hBpt;
    }

    return BPNOERROR;
}


/***    BPNextHbpt
**
**  Synopsis:
**      bpstatus = BPNextHbpt(phBpt, iFunc)
**
**  Entry:
**      phBpt   - pointer to breakpoint handle
**      iFunc   - Function to be executed
**
**  Returns:
**      BPNOERROR if no error occurs and modifies *phBpt to point to
**      new breakpoint handle
**
**  Description:
**
*/

BPSTATUS
BPNextHbpt(
          HBPT * phBpt,
          enumBptSearchOrder iFunc
          )
{
    LPBPP   lpBpp = NULL;
    HBPT    hBpt = *phBpt;
    BOOL    Replacement;

    while ( TRUE ) {

        switch ( iFunc ) {
        case bptNext:
            // M00TODO verify that *phBpt is in list
            hBpt = LLPlleFindNext( PlliBpList, hBpt);
            break;

#ifdef DBLLINK
        case bptPrevious:
            // M00TODO verify that *phBpt is in list
            hBpt = LLPlleFindPrev( PlliBpList, hBpt);
            break;
#endif

        case bptFirst:
            hBpt = LLPlleFindNext( PlliBpList, (HBPT) NULL);
            break;

#ifdef DBLLINK
        case bptLast:
            hBpt = LLPlleFindPrev( PlliBpList, NULL);
            break;
#endif
        default:
            return BPError;
        }

        //
        //  Skip replacement BPs
        //
        if ( hBpt == NULL ) {
            break;
        } else {

            lpBpp = (LPBPP) LLLpvFromPlle( hBpt );
            Replacement = lpBpp->fReplacement;

            if ( !Replacement ) {
                break;
            }
        }
    }

    *phBpt = hBpt;

    return BPNOERROR;
}                                   /* BPNextHbpt() */


/***    BPDisable
**
**  Synopsis:
**      bpstatus = BPDisable(hbpt)
**
**  Entry:
**      hbpt - Handle to disable the breakpoint on
**
**  Returns:
**      breakpoint error code
**
**  Description:
**      This routine will mark a breakpoint for being disabled.  The disabling
**      does not actually occur until the BPCommit is done.
*/

BPSTATUS
BPDisable(
          HBPT hbpt
          )
{
    LPBPP   lpbpp = NULL;
    LPBPP   lpbpp2 = NULL;

    lpbpp = (LPBPP) LLLpvFromPlle( hbpt );

    if (lpbpp == NULL) {
        return( BPBadBPHandle );
    }

    lpbpp->fNewMark = TRUE;

    /*
    **  If fMarkEnable is set then the current state of the breakpoint
    **      is disable so just clear the "to be enabled" flag
    */

    if (lpbpp->fMarkEnable) {
        lpbpp->fMarkEnable = FALSE;
    } else {
        if (lpbpp->bpstate & bpstateEnabled) {
            lpbpp->fMarkDisable = TRUE;
        }
    }

    if ( lpbpp->fMarkChange ) {
        lpbpp2 = (LPBPP) LLLpvFromPlle( lpbpp->ChangeHbpt );
        if (lpbpp2->fMarkEnable) {
            lpbpp2->fMarkEnable  = FALSE;
        }
        lpbpp2->fMarkDisable = TRUE;

    }

    return( BPNOERROR );
}                               /* BPDisable() */

/***    BPEnable
**
**  Synopsis:
**      bpstatus = BPEnable(hbpt)
**
**  Entry:
**      hbpt - Handle to enable the breakpoint on
**
**  Returns:
**      breakpoint error code
**
**  Description:
**      This routine will mark a breakpoint for being enabled.  The enabling
**      does not actually occur until the BPCommit is done.
*/

BPSTATUS
BPEnable(
         HBPT hbpt
         )
{
    LPBPP       lpbpp = NULL;
    LPBPP       lpbpp2 = NULL;
    char        rgch[2000] = {0};
    char        rgch2[2000] = {0};
    int         err = 0;
    BPSTATUS    Ret = BPNOERROR;

    lpbpp = (LPBPP) LLLpvFromPlle( hbpt );

    if (lpbpp == NULL) {
        return( BPBadBPHandle );
    }

    lpbpp->fNewMark = TRUE;

    if ( ((lpbpp->bpstate & bpstateEnabled) && lpbpp->fMarkDisable ) ||
         !lpbpp->fMarkEnable ) {

        //
        //  If the breakpoint is not instantiated, we try to instantiate it
        //
        if ( !(lpbpp->bpstate & bpstateSet) ) {
            if ( DebuggeeActive() ) {
                Ret = BPBindHbpt( hbpt, &CxfIp );
            }
        }

        //
        //  If fMarkDisable is set then the current state of the breakpoint
        //  is enabled so just clear the "to be disabled" flag
        //
        if (lpbpp->fMarkDisable) {
            lpbpp->fMarkDisable = FALSE;
        } else {
            if (!(lpbpp->bpstate & bpstateEnabled)) {
                lpbpp->fMarkEnable = TRUE;
            }
        }

        if ( lpbpp->fMarkChange ) {
            lpbpp2 = (LPBPP) LLLpvFromPlle( lpbpp->ChangeHbpt );
            if (lpbpp2->fMarkDisable) {
                lpbpp2->fMarkDisable = FALSE;
            } else {
                lpbpp2->fMarkEnable = TRUE;
            }
        }

        //
        //  Reset count
        //
        if (lpbpp->atmPassCount != 0) {
            ZeroMemory(rgch, sizeof(rgch));
            GetAtomName(lpbpp->atmPassCount, rgch, sizeof(rgch)-1);
            lpbpp->iPassCount = CPGetNbr(rgch,
                                         radix,
                                         fCaseSensitive,
                                         &(lpbpp->Cxf),
                                         rgch2,
                                         &err,
                                         g_contWorkspace_WkSp.m_bMasmEval
                                         );
        }
    }

    return( Ret );
}



/***    BPParse
**
**  Purpose: To parse a breakpoint command.
**      This is used to validate that a breakpoint is syntaxactly correct.
**      This is used for the U,A,G,BP,and V commands.
**
**  Input:
**      szBpt   String defining the breakpoint
**      ppBpp   Pointer to pointer to parse break point structure
**      szMod   String defining the current module
**      szFile  String defining the current file
**
**  Output:
**      Error code for the parse operation
**      ppBpp will be filled in with a pointer to a BPP on success
**
**  Exceptions:
**
**  Notes:
**
*/

BPSTATUS
BPParse(
        HBPT *  phBpt,
        char *  szBpt,
        char *  szMod,
        char *  szFile,
        HPID        hPid
        )
{
    HBPT            hBpt = LLPlleCreate(PlliBpList);
    LPBPP           pBpp = NULL;
    BPSTATUS        Ret = BPNOERROR;
    char *          pNextToken;
    char            TokenBuffer[2000];
    char *          szInit = szBpt;
    char            rgch[2000] = {0};
    char            rgch2[2000] = {0};
    BOOLEAN         ParsedLineNumber = FALSE;
    int             err = 0;

    BOOLEAN         ASwitch = FALSE;
    BOOLEAN         RSwitch = FALSE;
    BOOLEAN         PSwitch = FALSE;
    BOOLEAN         CSwitch = FALSE;
    BOOLEAN         MSwitch = FALSE;
//  BOOLEAN         DSwitch = FALSE;
    BOOLEAN         QSwitch = FALSE;
    BOOLEAN         HSwitch = FALSE;
    BOOLEAN         TSwitch = FALSE;
    BOOLEAN         WSwitch = FALSE;

    if (hBpt == NULL) {
        return BPOOMemory;
    }

    pBpp = (LPBPP) LLLpvFromPlle(hBpt);

    //
    //  Initialize breakpoint structure
    //
    memset(pBpp, 0, sizeof(*pBpp));

    //
    //  By default all breakpoints are not set and enabled
    //
    pBpp->bpstate = bpstateNotSet | bpstateEnabled;
    pBpp->hPid = hPid;
    pBpp->iBreakpointNum = (USHORT) -1;
    pBpp->iHighlightLine = (USHORT) -1;
    pBpp->fTemporary = 0;
    pBpp->BpType = 0;

    //
    //  We are going to start parsing up the string.
    //  First skip over any leading whitespace and check to make sure that
    //  there was something specified.  I.e. "bp " is not a
    //  legal breakpoint command
    //
    pNextToken = szBpt;
    szBpt = CPSzToken(&pNextToken, TokenBuffer);
    if ( !szBpt || *szBpt == '\0') {

        //
        //  ERROR - empty string
        //
        Ret = BPCODEADDR;
        goto error;
    }

    //
    //  Now start walking through the string looking for all of the pieces
    //  of a breakpoint command.
    //
    //  For the most part we can tell what a piece is from its first character
    //
    //  The portions of a breakpoint command are:
    //      expression                      Address expression
    //      =expression                     Watch point (data change)
    //      ?expression                     Trace point (expression change)
    //      /R  /r                          Size of watch point
    //      /P  /p                          Pass count
    //      /C  /c                          Commands to execute
    //      /M  /m                          Window message
    //      ! not used /D  /d               Don't stop if true (window msg only)
    //      /Q  /q                          Quiet defer
    //      /T  /t                          Thread
    //      /W  /w                          WndProc breakpoint
    //      /A  /a                          Data breakpoint types
    //
    //  The following error checks must be done:
    //      1. Watchpoint and Tracepoints are mutually exclusive
    //      2. /R requires a Watchpoint command
    //

    while (szBpt && *szBpt) {
        switch (*szBpt++) {
        case '=':
            if (pBpp->atmExpr || pBpp->atmRangeSize) {
                Ret = BPBadDataSize;
                goto error;
            }
            if ((Ret = BpGetHtm(pBpp, szBpt, FALSE)) != BPNOERROR) {
                goto error;
            }
            pBpp->BpType = BP_DATA_CHANGED;
            break;

        case '?':
            if (pBpp->atmData) {
                Ret = BPBadDataSize;
                goto error;
            }
            if ((Ret = BpGetHtm(pBpp, szBpt, TRUE)) != BPNOERROR) {
                goto error;
            }
            break;

        case '/':
            switch (*szBpt) {
            /*
            **  Get the size of the watch point to be used.
                */

            case 'R':
            case 'r':
                if (RSwitch) {
                    Ret = BPBadCmdString;
                    goto error;
                }
                RSwitch = TRUE;
                if (pBpp->atmExpr) {
                    Ret = BPBadDataSize;
                    goto error;
                }
                szBpt++;
                if (!FParseableSz(szBpt)) {
                    Ret = BPBadDataSize;
                    goto error;
                }
                pBpp->atmRangeSize = AddAtom(szBpt);
                break;

            case 'P':
            case 'p':
                if (PSwitch) {
                    Ret = BPBadCmdString;
                    goto error;
                }
                PSwitch = TRUE;
                szBpt++;
                if (!FParseableSz(szBpt)) {
                    Ret = BPBadPassCount;
                    goto error;
                }
                pBpp->atmPassCount = AddAtom(szBpt);
                break;

            case 'A':
            case 'a':
                if (ASwitch) {
                    Ret = BPBadCmdString;
                    goto error;
                }
                ASwitch = TRUE;
                szBpt++;
                switch( tolower(*szBpt) ) {
                case 'e':
                    pBpp->BpType = BP_EXECUTE;
                    break;

                case 'w':
                    pBpp->BpType = BP_DATA_WRITE;
                    break;

                case 'r':
                    pBpp->BpType = BP_DATA_READ;
                    break;

                case 'c':
                    pBpp->BpType = BP_DATA_CHANGED;
                    break;

                default:
                    Ret = BPBadCmdString;
                    goto error;
                }
                break;

                case 'C':
                case 'c':
                    if (CSwitch) {
                        Ret = BPBadCmdString;
                        goto error;
                    }
                    CSwitch = TRUE;
                    szBpt++;
                    if (*szBpt != '\"') {
                        Ret = BPBadCmdString;
                        goto error;
                    }
                    if (szBpt[strlen(szBpt)-1] == '\"')
                        szBpt[strlen(szBpt)-1] = 0;
                    pBpp->szCmdLine  = _strdup( szBpt+1);
                    break;

                case 'M':
                case 'm':
                    szBpt++;
                    if ( MSwitch ) {
                        Ret = BPBadCmdString;
                        goto error;
                    }

                    pBpp->atmMsg    = AddAtom(szBpt);
                    pBpp->fWndProc  = TRUE;
                    pBpp->fMsg      = TRUE;
                    MSwitch         = TRUE;
                    break;

                case 'Q':
                case 'q':
                    if (QSwitch) {
                        Ret = BPBadCmdString;
                        goto error;
                    }
                    QSwitch = TRUE;
                    pBpp->fQuiet = TRUE;
                    break;


                case 'H':
                case 'h':
                    if (HSwitch) {
                        Ret = BPBadCmdString;
                        goto error;
                    }
                    HSwitch = TRUE;
                    szBpt++;
                    if (!FParseableSz(szBpt)) {
                        Ret = BPBadPassCount;
                        goto error;
                    }
                    pBpp->iPid = CPGetNbr( szBpt,
                        10,
                        fCaseSensitive,
                        &CxfIp,
                        rgch,
                        &err,
                        g_contWorkspace_WkSp.m_bMasmEval
                        );
                    Assert(strlen(rgch) < sizeof(rgch));
                    if ( err != EENOERROR ) {
                        Ret = BPBadProcess;
                        goto error;
                    }
                    pBpp->fProcess = TRUE;
                    break;

                case 'T':
                case 't':
                    if (TSwitch) {
                        Ret = BPBadCmdString;
                        goto error;
                    }
                    TSwitch = TRUE;
                    szBpt++;
                    if (!FParseableSz(szBpt)) {
                        Ret = BPBadPassCount;
                        goto error;
                    }
                    pBpp->iTid    = CPGetNbr( szBpt,
                        10,
                        fCaseSensitive,
                        &CxfIp,
                        rgch,
                        &err,
                        g_contWorkspace_WkSp.m_bMasmEval
                        );
                    if ( err != EENOERROR ) {
                        Ret = BPBadThread;
                        goto error;
                    }
                    pBpp->fThread = TRUE;
                    break;

                case 'W':
                case 'w':
                    if (WSwitch) {
                        Ret = BPBadCmdString;
                        goto error;
                    }
                    WSwitch = TRUE;
                    pBpp->fWndProc = TRUE;
                    szBpt++;
                    break;

                default:
                    Ret = BPBadOption;
                    goto error;
            }
            break;

          default:
              szBpt--;
              if ( ParsedLineNumber ) {
                  Ret = BPBadAddrExpr;
                  goto error;
              } else if ((Ret = BPParseLineNumber(szBpt, pBpp)) != BPNOERROR) {
                  goto error;
              }
              ParsedLineNumber = TRUE;
              break;
        }

        szBpt = CPSzToken(&pNextToken, TokenBuffer);
    }

    if ( !pBpp->fProcess ) {
        if ( hPid == 0 ) {
            pBpp->iPid = 0;
        } else {
            LPPD lppd = LppdOfHpid(hPid);
            if (lppd) {
                pBpp->iPid = lppd->ipid;
            }
        }
        pBpp->fProcess = TRUE;
    }

    //
    //  Do additional error checking
    //

    if (pBpp->atmRangeSize ) {
        if ( !pBpp->atmData) {
            //
            //  A range requires a data address
            //
            Ret = BPBadDataSize;
            goto error;
        } else {
           GetAtomName(pBpp->atmRangeSize, rgch, sizeof(rgch)-1);
           pBpp->cbDataSize = CPGetNbr(rgch,
                                       radix,
                                       fCaseSensitive,
                                       &(pBpp->Cxf),
                                       rgch2,
                                       &err,
                                       g_contWorkspace_WkSp.m_bMasmEval
                                       );
        }
    } else if ( pBpp->fMemory ) {

        //
        //  For memory breakpoints, the size defaults to 4
        //
        pBpp->atmRangeSize = AddAtom( "4" );
        pBpp->cbDataSize   = 4;
    }

    if ( pBpp->fWndProc ) {

        //
        //  No line numbers
        //
        if ( !pBpp->fCodeAddr ||
            *(pBpp->szAddrExpr) == '@' ||
            *(pBpp->szAddrExpr) == '.' ) {
            Ret = BPBadAddrExpr;
            goto error;
        }

        //
        //  Validate the message/message class if we can.
        //
        if ( pBpp->fMsg ) {

            XOSD        xosd;
            LPMESSAGEMAP    MsgMap = NULL;
            LPPD        lppd;
            LPTD        lptd;

            lppd = LppdOfIpid(pBpp->iPid);

            if ( lppd ) {

                //
                //  Get message list and parse the message.
                //
                lptd = lppd->lptdList;

                xosd = OSDGetMessageMap( lppd->hpid, lptd->htid, &MsgMap );

                if ( xosd == xosdNone ) {

                    if ( !ParseBpMsg( pBpp, MsgMap ) ) {
                        Ret = BPError;
                        goto error;
                    }
                }
            }
        }
    }

    //
    //  Make sure that we have an expression
    //
    if ( pBpp->fCodeAddr ) {
        if ( !pBpp->atmAddrCxt &&
             !pBpp->atmCxtDef  &&
             !pBpp->szAddrExpr ) {

            Ret = BPError;
            goto error;
        }
    }

    //
    //  Get the default context set
    //

    if ((szMod != NULL) || (szFile != NULL && szFile !=(char *)-1)) {
        sprintf(rgch, "{%s,%s,}", szMod == NULL ? "" : szMod,
                szFile == NULL ? "" : szFile);
        pBpp->atmCxtDef = AddAtom(rgch);
        if ((pBpp->atmHighlightFile == 0) && (szFile != NULL)) {
            pBpp->atmHighlightFile = AddAtom(szFile);
        }
    }

    //
    //  Return the pointer to the structure
    //

    *phBpt = hBpt;
    return Ret;

    //
    //  Come here in the event of an error to clean up the world
    //
error:
    BPKillNode(pBpp);
    MMFreeHmem( (HDEP) hBpt );
    return  Ret;
}

/*** BPParseLineNumber
*
* Purpose:
*   Parse the address portion of a breakpoint command.  Legal address
*   formats are:
*
*       {CONTEXT}@line
*       {CONTEXT}.line
*       .line
*       @line
*       {CONTEXT}addr-expr
*       addr-expr
*
* Input:
*
* Output:
*
* Exceptions:
*
*************************************************************************/

LOCAL
BPSTATUS
BPParseLineNumber(
                  char * sz,
                  LPBPP pBpp
                  )
{
    char        szCxt[2000] = {0};
    LPSTR       lpch = NULL;
    LPSTR       lpchAddr = NULL;
    LPSTR       lp0 = NULL;
    LPSTR       lp1 = NULL;
    LPSTR       lpBp = NULL;
    char        sfile[2000] = {0};
    DWORD       iLine = 0;
    BOOL        fNeedContext = FALSE;

    /*
    **  Step 1 is to look and see if a context operator is being given
    **  in the breakpoint command itself.  If so then we want to strip
    **  it off to look at the rest of the expression separately
    */

    if (*sz == OPENCONTEXTOP) {      /* check for a context operator here */

        lpBp = (LPSTR) strchr((LPBYTE) sz, CLOSECONTEXTOP);
        if (!lpBp) {
            return BPBadContextOp;
        }
        lpBp++;

        lp0 = (LPSTR) strchr((LPBYTE) sz, ',');

        if (!lp0 || lp0 >= lpBp) {

            // don't look past the end of the context op
            lp0 = sz + 1;

        } else {

            // skip function; if there is another comma, it
            // delimits a source file
            lp0++;

            lp1 = (PSTR) strchr((LPBYTE) lp0, ',');

            if (lp1 && lp1 < lpBp) {

                if (lp1 > lp0) {
                    memset(sfile, 0, sizeof(sfile));
                    strncpy(sfile, lp0, (size_t)(lp1-lp0));
                    pBpp->atmHighlightFile = AddAtom(sfile);
                }
                lp0 = lp1+1;
            }

            // refuse extra commas
            lp1 = (PSTR) strchr((LPBYTE) lp0, ',');
            if (lp1 && lp1 < lpBp) {
                return BPBadContextOp;
            }
        }

        strncpy(szCxt, sz, (size_t) (lpBp-sz));
        szCxt[lpBp - sz] = 0;

        /*
        **  Skip over the context operator and any whitespace following it
        */

        sz = CPSkipWhitespace( lpBp );

        /*
        **  Check for only a context operator
        */

        if (*sz == 0) {
            return BPBadContextOp;
        }
    } else if ( (lp0 = (LPSTR) strchr( (LPBYTE) sz, '!' )) ) {

        *lp0 = 0;
        strcpy( szCxt, "{,," );
        strcat( szCxt, sz );
        strcat( szCxt, "}" );
        Assert(strlen(szCxt) < sizeof(szCxt));

        sz = CPSkipWhitespace( lp0+1 );
    }

    pBpp->fHighlight = TRUE;

    lpchAddr = sz;

    /*
    **  Two possibilities now -- either its a line number construct or
    **      its an address.
    */

    if (*sz == '.') {
        *sz = '@';
        sz++;                   // Skip over the '.' character
        sz = CPSkipWhitespace( sz );// Skip over any whitespace
        if (*sz == 0) {
            return BPBadAddrExpr;
        }

        /*
        **  What is left should be a parsable expression
        */

        if (!FParseableSz(sz)) {
            return BPBadAddrExpr;
        }

        fNeedContext = TRUE;
    } else {
        if (!FParseableSz(sz)) {
            return BPBadAddrExpr;
        }

        fNeedContext = (strcspn(sz, "@") != strlen(sz));
    }

    /*
    **  Check for a line number expression -- defined as starting with
    **  an @ character
    */

    if (*lpchAddr == '@') {
        iLine = 0;
        for (lpch = lpchAddr+1; ('0' <= *lpch) && (*lpch <= '9'); lpch++) {
            iLine = iLine*10 + *lpch - '0';
        }
        if ( *lpch && *lpch != ' ' && *lpch != '\t' ) {
            return BPBadAddrExpr;
        }

        pBpp->iHighlightLine = iLine;
    }

    /*
    **  Now rebuild the expression to be evaluated
    */

    if (szCxt[0] != 0) {
        pBpp->atmAddrCxt = AddAtom(szCxt);
    }
    pBpp->szAddrExpr  = _strdup(lpchAddr);
    pBpp->fCodeAddr = TRUE;

    return BPNOERROR;
}                               /* BPParseLineNumber() */


/*** BPADDRFromTM
*
* Purpose: To get the address of an item represented by a TM
*
* Input:
*   phTM    - A pointer to a handle to a TM that represents
*           the item to get the address of
*
*   pwSegType - This indicates that the TM must be a member of this
*           segment type (Code, Data, Const).
*
*   pAddr   - The place to put the address when found
*
* Output:
*  Returns An Expr Eval Error, or EECATASTROPHIC if nothing was found
*
*          Modify pwSegType to reflect InfoFromTM response.
*
* Exceptions:
*
* Notes:
*
* This assumes that the debugger's current global process (LppdCur) is
* the one which owns the HTM.
*
*************************************************************************/
EESTATUS
BPADDRFromTM(
             PHTM phTM,
             LPDWORD pwSegType,
             LPADDR paddr
             )
{
    EESTATUS  Err = EENOERROR;
    RTMI RIT = {0};
    HTI hTI = NULL;
    PTI pTI = NULL;

    memset(paddr, 0, sizeof(ADDR));
    RIT.fAddr       = TRUE;
    RIT.fSegType    = TRUE;
    RIT.fValue      = TRUE;
    RIT.fSzBytes    = TRUE;

    // get the information
    if( !(Err = EEInfoFromTM(phTM, &RIT, &hTI)) ) {

        // get the TI
        assert(hTI != (HTI)NULL);
        pTI = (PTI) MMLpvLockMb (hTI);

        // if an address was given.
        if ( pTI->fResponse.fAddr ) {
            *paddr = pTI->AI;
            SYUnFixupAddr( paddr );
        }
        //if( pTI->fResponse.fAddr && (pTI->u.SegType & *pwSegType) ) {
        //    *paddr = pTI->AI;
        //     SYUnFixupAddr ( paddr );
        //    *pwSegType = pTI->u.SegType;
        //}
        else if(!pTI->fResponse.fAddr &&
            pTI->fResponse.fValue &&
            pTI->fResponse.fSzBytes &&
            pTI->cbValue >= sizeof(WORD) ) {

            // if a value was given
            // save the offset
            switch ( pTI->cbValue ) {

            case sizeof(BYTE):

                SetAddrOff ( paddr , *((CHAR *) pTI->Value) );
                break;

            default:
            case sizeof(WORD):

                SE_SetAddrOff ( paddr , *((WORD *) pTI->Value) );
                break;

            case sizeof(DWORD):

                // sizeof(SHOFF) == sizeof(DWORD) if ADDR_MIXED is defined
                SE_SetAddrOff ( paddr , *((SHOFF *) pTI->Value) );
                break;

            case sizeof(DWORDLONG):
                SetAddrOff ( paddr , *((DWORDLONG *) pTI->Value) );
                break;
            }

            // set the segment
            if( (*pwSegType & EEDATA) == EEDATA ) {
                ADDR addrData = {0};

                OSDGetAddr ( LppdCur->hpid, LptdCur->htid, adrData, &addrData );
                SetAddrSeg ( paddr, (SHSEG) GetAddrSeg ( addrData ) );
                SYUnFixupAddr ( paddr );
                *pwSegType &= EEDATA;
            }

            else if( (*pwSegType & EECODE) == EECODE ) {
                ADDR addrPC = {0};

                OSDGetAddr ( LppdCur->hpid, LptdCur->htid, adrPC, &addrPC );
                SetAddrSeg ( paddr, (SHSEG) GetAddrSeg ( addrPC ) );
                SYUnFixupAddr ( paddr );
                *pwSegType &= EECODE;
            }

            // assume bad address expression
            else {
                Err = EEBADADDR;
            }
        }
        else {
            Err = EEBADADDR;
        }

        // remove the TI
        MMbUnlockMb (hTI);
        EEFreeTI(&hTI);
    }

    // thou shalt never return a physical address from this function!
#ifdef DEBUGVER
    if ( !Err ) {
        assert ( ADDR_IS_LI ( *paddr ) );
    }
#endif
    return(Err);
}

/***    BPDelete
**
**  Synopsis:
**      bpstatus = BPDelete( hBpt )
**
**  Entry:
**      hBpt  - handle of breakpoint to be deleted
**
**  Returns:
**      breakpoint status code
**
**  Description:
**      This routine will take and delete the requested breakpoint handle.
**
*/

BPSTATUS
BPDelete(
         HBPT hBpt
         )
{
    LPBPP       lpBpp = NULL;
    LPBPP       lpBpp2 = NULL;

    lpBpp = (LPBPP) LLLpvFromPlle(hBpt);

    if (lpBpp == NULL) {
        return BPBadBPHandle;
    }

    lpBpp->fNewMark = TRUE;

    if (lpBpp->fMarkAdd) {

        if ( lpBpp->fMarkChange ) {
            LLFDeletePlleFromLl( PlliBpList, lpBpp->ChangeHbpt );
            lpBpp->ChangeHbpt  = NULL;
            lpBpp->fMarkChange = FALSE;
        }

        lpBpp->bpstate |= bpstateDeleted;
        BPRemoveHighLight( lpBpp );
        LLFDeletePlleFromLl( PlliBpList, hBpt );
    }
    else {
        lpBpp->fMarkDelete = TRUE;

        if ( lpBpp->fMarkChange ) {
            lpBpp2 = (LPBPP) LLLpvFromPlle( lpBpp->ChangeHbpt );
            lpBpp2->fMarkDelete = TRUE;
            lpBpp2->fMarkAdd    = FALSE;
        }

    }

    return BPNOERROR;

}                                   /* BPDelete() */


/***    BPDeleteAll
**
**  Synopsis:
**      bpstatus = BPDeleteAll( )
**
**  Entry:
**      None
**
**  Returns:
**      breakpoint status code
**
**  Description:
**      This routine will take and delete all breakpoints in the world
**
*/

BPSTATUS BPDeleteAll()
{
    HBPT        hBpt1 = NULL;
    HBPT        hBpt2 = NULL;

    Dbg( BPNextHbpt( &hBpt1, bptFirst) == BPNOERROR );

    while ( hBpt1 != NULL ) {
        /*
        **  Save the previous breakpoint
        */

        hBpt2 = hBpt1;

        /*
        **  Get the next one on the list incase this one disappears
        */

        Dbg( BPNextHbpt( &hBpt1, bptNext ) == BPNOERROR );

        /*
        **  Now delete the current one
        */

        Dbg( BPDelete( hBpt2 ) == BPNOERROR );
    }

    return BPNOERROR;
}                                   /* BPDeleteAll() */


/***    BPBindHbpt
**
**  Synopsis:
**      bpstatus = BPBindHbpt( hBpt )
**
**  Entry:
**      hBpt    - handle to breakpoint to be bound in
**
**  Returns:
**      status of the action
**
**  Description:
**
*/
BPSTATUS
BPBindHbpt(
           HBPT hBpt,
           CXF *pCxf
           )
{
    HBPT        hBpt2 = NULL;
    BOOL        fOnList = FALSE;
    LPBPP       lpBpp = NULL;
    BPSTATUS    Ret = BPNOERROR;

    //
    //  Check to see if this breakpoint is on the list of breakpoints
    //
    Dbg( BPNextHbpt( &hBpt2, bptFirst) == BPNOERROR);
    while (hBpt2 != NULL) {
        if (hBpt2 == hBpt) {
            fOnList = TRUE;
            break;
        }
        Dbg( BPNextHbpt( &hBpt2, bptNext ) == BPNOERROR );
    }

    lpBpp = (LPBPP) LLLpvFromPlle( hBpt );

    if ( lpBpp->fCodeAddr ) {
        Ret = BPBindHbptAddr( hBpt, lpBpp, fOnList, pCxf );
    } else {
        Ret = BPBindHbptNoAddr( hBpt, lpBpp, fOnList, pCxf );
    }


    return Ret;
}                                   /* BPBindHbpt */



BPSTATUS
BPBindHbptNoAddr(
                 HBPT hBpt,
                 LPBPP lpBpp,
                 BOOL fOnList,
                 CXF *pCxf
                 )
{
    char        rgch[2000] = {0};
    char        rgch2[2000] = {0};
    BPSTATUS    Ret = BPNOERROR;
    int         err = 0;
    XOSD        Xosd;
    int         cch;
    BOOLEAN     Ok;

    Ret = BPSetPidTid( lpBpp );
    if ( pCxf ) {
        lpBpp->Cxf = *pCxf;
    }

    if ( Ret == BPNOERROR ) {

        lpBpp->bpstate &= ~bpstateSets;
        lpBpp->bpstate |= bpstateVirtual;

        //
        //  If a memory breakpoint, try to bind the memory address expression
        //
        if ( !lpBpp->fMemory ) {

            Ok = TRUE;

        } else {

            GetAtomName(lpBpp->atmData, rgch, sizeof(rgch)-1);

            Ok = (CPGetAddress( rgch,
                                &cch,
                                &(lpBpp->AddrMem),
                                radix,
                                &(lpBpp->Cxf),
                                FALSE,
                                g_contWorkspace_WkSp.m_bMasmEval) == EENOERROR);

            if ( Ok ) {

                if (lpBpp->cbDataSize == 0) {
                    lpBpp->cbDataSize = 1;
                }

#if 0
                //
                //  Verify that the memory is readable.
                //
                lpBpp->Mem = (PBYTE)malloc( lpBpp->cbDataSize );

                if ( !lpBpp->Mem ) {
                    Ok = FALSE;
                } else {

                    OSDFixupAddr( lpBpp->hPid, NULL, &(lpBpp->AddrMem) );
                    Ok = ( OSDSetAddr( lpBpp->hPid,
                                       lpBpp->hTid,
                                       adrCurrent,
                                       &(lpBpp->AddrMem))
                                                            == xosdNone);

                    if ( Ok ) {
                        OSDUnFixupAddr( lpBpp->hPid, NULL, &(lpBpp->AddrMem) );
                        Ok = (OSDPtrace( osdReadBuf,
                                         lpBpp->cbDataSize,
                                         lpBpp->Mem,
                                         lpBpp->hPid,
                                         lpBpp->hTid)
                                              == (XOSD)lpBpp->cbDataSize);
                    }
                }
#endif
            }
        }

        if ( !Ok ) {
            lpBpp->bpstate &= ~bpstateSets;
            lpBpp->bpstate |= bpstateNotSet;

            Ret = BPError;
        }


        if ( Ok && fOnList && !lpBpp->fMarkAdd &&
                                         (lpBpp->bpstate & bpstateEnabled)) {

            BPInitBps( lpBpp, BpsBuffer, TRUE );
            Xosd = OSDBreakpoint( lpBpp->hPid, BpsBuffer );

            if ( Xosd == xosdNone ) {
                lpBpp->bpstate &= ~bpstateSets;
                lpBpp->bpstate |= bpstateSet;
                lpBpp->qwNotify = QwNotification(BpsBuffer)[0];

                if ( lpBpp->fMemory ) {
                    MemoryBpCount++;
                }

            } else {
                lpBpp->bpstate &= ~bpstateSets;
                lpBpp->bpstate |= bpstateNotSet;

                Ret = BPError;
            }
        }

        if (lpBpp->atmPassCount != 0) {

            GetAtomName(lpBpp->atmPassCount, rgch, sizeof(rgch)-1);
            lpBpp->iPassCount = CPGetNbr(rgch,
                                         radix,
                                         fCaseSensitive,
                                         &(lpBpp->Cxf),
                                         rgch2,
                                         &err,
                                         g_contWorkspace_WkSp.m_bMasmEval
                                         );
        }
    }

    return Ret;
}                               /* BPBindHbptNoAddr */



BPSTATUS
BPBindHbptAddr(
               HBPT hBpt,
               LPBPP lpBpp,
               BOOL fOnList,
               CXF *pCxf
               )
{
    BPSTATUS    Ret;
    LPMESSAGEMAP    MsgMap = NULL;
    char        BpBuf[2000] = {0};

    //
    //  Set Context fields in BP structure
    //
    Ret = BPSetPidTid( lpBpp );

    if ( Ret == BPNOERROR ) {

        //
        //  If a CXF was provided, use it.
        //
        if ( pCxf ) {
            lpBpp->Cxf = *pCxf;
        }

        //
        //  If this is a message BP and has not been parsed, load the message
        //  map and do the parsing now.
        //
        if ( lpBpp->fMsg && !lpBpp->fMsgParsed ) {
            if ( (OSDGetMessageMap( lpBpp->hPid, lpBpp->hTid, &MsgMap ) != xosdNone) ||
                 !ParseBpMsg( lpBpp, MsgMap ) ) {
                Ret = BPError;
            }
        }

        if ( Ret == BPNOERROR ) {

            //
            //  Get the list of TMs in which the breakpoint can bind.
            //
            SaveDebuggerPdTd();
            SetDebuggerPdTd( lpBpp );

            TML Tml = {0};
            Ret = BPGetTmList( lpBpp, &Tml, BpBuf, sizeof( BpBuf ) );

            RestoreDebuggerPdTd();

            if ( Ret == BPNOERROR ) {

                //
                //  Pick the TMs to use and bind the breakpoint(s).
                //
                Ret = BPBindFromTmList( lpBpp, &Tml, fOnList, BpBuf );
            }
        }
    }

    return Ret;
}                               /* BPBindHbptAddr */



BPSTATUS
BPGetTmList(
    BPP * const lpBpp,
    TML * const pTml,
    char * BpBuf,
    int BufSize
    )
{
    Assert(pTml);
#ifdef DBG
    // Sanity check to make sure we don't leak memory
    {
        TML tmlTemp = {0};
        if (memcmp(&tmlTemp, pTml, sizeof(tmlTemp))) {
            Assert(!"TML structures must be blank");
        }
    }
#endif

    BPSTATUS    Ret = BPNOERROR;
    EESTATUS    ERet;
    char        *szAddrExpr = NULL;
    DWORD        stringindex;
    TML         Tml = {0};
    char        *Source = NULL;
    char        *Exe = NULL;


    //
    //  Get expression to parse.
    //
    if ( lpBpp->atmAddrCxt ) {

        GetAtomName( lpBpp->atmAddrCxt, BpBuf, BufSize-1 );
        szAddrExpr = &BpBuf[strlen(BpBuf)];
        strncpy( szAddrExpr, lpBpp->szAddrExpr, BufSize-strlen(BpBuf) );

    } else if ( lpBpp->atmCxtDef ) {

        GetAtomName( lpBpp->atmCxtDef, BpBuf, BufSize-1 );
        szAddrExpr = &BpBuf[strlen(BpBuf)];
        strncpy( szAddrExpr, lpBpp->szAddrExpr, BufSize-strlen(BpBuf) );

    } else if ( lpBpp->szAddrExpr ) {

        strncpy( BpBuf, lpBpp->szAddrExpr, BufSize );
        szAddrExpr = BpBuf;

    } else {
        //
        //  No breakpoint expression!
        //
        Ret = BPError;
    }

    if ( Ret == BPNOERROR ) {

        //
        //  Try to parse the expression
        //
        ERet = EEParseBP( BpBuf, radix, fCaseSensitive, &(lpBpp->Cxf), &Tml, 0L, &stringindex, FALSE );
        Ret = (ERet == EENOERROR ) ? BPNOERROR : BPError;

        if ( Ret == BPError ) {

            EEFreeTML(&Tml);

            //
            //  If no context was specified and this is not a line
            //  number BP, then search all contexts.
            //
            if ( ( lpBpp->atmAddrCxt == 0 && *szAddrExpr != '@' )
                || lpBpp->atmCxtDef  != 0 || lpBpp->atmAddrCxt != 0 ) {

                Ret = BPSearchContexts( lpBpp,
                                        szAddrExpr,
                                        &Tml,
                                        TRUE,
                                        SymbolLoadingEnabled );
            }

        } else {

            //
            //  If There is a line number and there is a source file, but no
            //  module was specified, then look for any ambiguities (i.e. look
            //  for any modules that have the same source).
            //
            SplitExpression( BpBuf, &Source, &Exe );

            if ( *szAddrExpr == '@' && Source && !Exe ) {

                EEFreeTML(&Tml);
                Ret = BPSearchContexts( lpBpp, szAddrExpr, &Tml, FALSE,
                                                        SymbolLoadingEnabled );

            } else if ( lpBpp->atmAddrCxt == 0 && *szAddrExpr != '@' ) {

                //
                //  If what we got is a thunk, then look for the implementation and try to
                //  set the BP there instead.
                //
                if ( BPIsThunk( lpBpp, Tml ) ) {

                    TML tmlImplementation = {0};
                    Ret  = BPSearchContexts( lpBpp, szAddrExpr, &tmlImplementation, TRUE, TRUE );

                    if ( Ret == BPNOERROR ) {

                        //
                        //  Found implementation. Discard TM for thunk and use the
                        //  one for the implementation.
                        //
                        EEFreeTML(&Tml);
                        Tml = tmlImplementation;

                    } else {

                        //
                        //  Implementation not found, ask the user if she wants
                        //  to set the breakpoint in the thunk.
                        //
                        if ( BPTCanIUseThunk( BpBuf ) ) {

                            EEFreeTML( &tmlImplementation );
                            Ret = BPNOERROR;

                        }  else {

                            EEFreeTML( &Tml );
                            EEFreeTML( &tmlImplementation );
                            Ret = BPCancel;
                        }
                    }
                }
            }
        }
    }

    if ( Ret == BPNOERROR ) {
        *pTml = Tml;
    }

    return Ret;
}                               /* BPGetTmList */


VOID
SplitExpression(
                char *BpBuf,
                char **Source,
                char **Exe
                )
{
    char *p = NULL;
    char *q = NULL;

    *Source = NULL;
    *Exe    = NULL;

    if ( p = (PSTR) strchr( (LPBYTE) BpBuf, OPENCONTEXTOP ) ) {

        if ( p = (PSTR) strchr( (LPBYTE) p+1, ',' ) ) {

            p++;
            p += strspn( p, " \t" );
            q  = p + strcspn( p, /* { */ ",} " );

            if ( q > p ) {
                *Source = p;
            }

            q += strspn( q, " \t" );

            if ( *q == ',' ) {

                p  = q+1;
                p += strspn( p, " \t" );
                q  = p + strcspn( p, /* { */ "} " );

                if ( q > p ) {
                    *Exe = p;
                }
            }
        }
    }
}                           /* SplitExpression */

BOOL
SearchContexts(
    LPBPP            lpBpp,
    char *           rgch,
    char *           szAddrExpr,
    TML * const      pTml,
    PHTM * const     ppTmListArg,
    BOOL             ImplementationOnly,
    BOOL             fLoad
    )
//    SearchContexts
// Arguments:
//
//  Description:
//      We are going to search every context in existance for the address
//      expression that has been passed in.  We want to do this is a more
//      intellegent manner that was previously being used.  In order to
//      do this we are going search things in the following order:
//      1.  Search only those symbol files for which the symbols have already
//              been loaded in.
//      2.  If fLoad is set, then search the rest of the symbol files.  If
//              I can figure out a good way of doing this I really want to
//              do this search in load address order
//
//  Returns:
//      TRUE - success
//      FALSE - ERROR.
//          *ppTmList is set to NULL.
//          *pTml is zeroed out.
{
    Assert(lpBpp);
    Assert(rgch);
    Assert(szAddrExpr);
    Assert(pTml);
    Assert(ppTmListArg);
    Assert(NULL == *ppTmListArg);

#ifdef DBG
    // Sanity check to make sure we don't leak memory
    {
        TML tmlTemp = {0};
        if (memcmp(&tmlTemp, pTml, sizeof(tmlTemp))) {
            Assert(!"TML structures must be blank");
        }
    }
#endif

    PHTM        pTmList = *ppTmListArg;
    HEXE        hexe = NULL;
    char        *p = NULL;
    BOOLEAN     fUse;
    int         ichExe;
    EESTATUS    ERet;
    BPSTATUS    Ret;
    DWORD       stringindex;
    int         i;
    LSZ         ExeName = NULL;
    LPDEBUGDATA DebugData = NULL;
    int         iPass;
    BOOL        fFound = FALSE;

    p = rgch + strlen(rgch);

    for (iPass = 0; (iPass < 2) && !fFound; iPass++) {
        //
        //      If we don't want to load anything, then we don't need to do the
        //      second pass
        //

        if ((iPass == 1) && !fLoad) {
            break;
        }

        //
        //  enumerate every module to be searched
        //

        hexe = NULL;
        while ((( hexe = SHGetNextExe( hexe ) ) != 0) ) {
            if (iPass == 0) {
                //
                //  First pass -- we want to look only at those modules for which the
                //  symbols are already loaded
                //
                if (!SHIsDllLoaded(hexe)) {
                    continue;
                }
            } else {
                //
                //  Second pass -- we want to look only at those modules for which
                //      the symbols are NOT already loaded

                // BUGBUG - kcarlos
                // uncomment, but first verify that SHIsDllLoaded is doing what we expect
                if (/*SHIsDllLoaded(hexe)*/ FALSE) {
                    continue;
                }

                SHWantSymbols(hexe);
            }

            //
            //
            //

            *p = '\0';
            ExeName =  SHGetExeName( hexe );

            strcat( rgch, ExeName );
            ichExe          = strlen(rgch);
            rgch[ichExe]    = CLOSECONTEXTOP;
            rgch[ichExe+1]  = 0;

            strncat( &rgch[ ichExe+1 ], szAddrExpr, strlen(szAddrExpr) );

            TML Tml2 = {0};
            ERet = EEParseBP( rgch, radix, fCaseSensitive, &(lpBpp->Cxf), &Tml2, 0L,
                              &stringindex, FALSE);
            Ret = (ERet == EENOERROR ) ? BPNOERROR : BPError;

            if (BPError == Ret) {

                // An error occurred let's free the allocated memory
                EEFreeTML(&Tml2);

            } else {

                if (ImplementationOnly && BPIsThunk(lpBpp, Tml2)) {

                    // Nothing to do.
                    EEFreeTML(&Tml2);

                } else {
                    //
                    //  If interested in implementation, ignore thunks.
                    //

                    // (!ImplementationOnly || !BPIsThunk(lpBpp, Tml2))

                    if (NULL == pTmList) {

                        *pTml = Tml2;
                        pTmList = (PHTM) MMLpvLockMb(pTml->hTMList);

                    } else {

                        // Add the list from Tml2 to Tml
                        PHTM pTmList2 = (PHTM) MMLpvLockMb(Tml2.hTMList);

                        for (i=0; i < (int)Tml2.cTMListAct; i++ ) {
                            assert( Tml2.cTMListAct < pTml->cTMListMax );

                            pTmList[ pTml->cTMListAct ] = pTmList2[ i ];
                            pTml->cTMListAct++;
                            pTmList2[i] = (HDEP) NULL;
                        }

                        MMbUnlockMb(Tml2.hTMList);

                        Tml2.cTMListAct = 0;
                        EEFreeTML(&Tml2);
                    }
                }

                if (fLoad) {
                    fFound = TRUE;
                    break;
                }
            }
        }
    }

    *ppTmListArg = pTmList;

    if (NULL != pTmList) {
        // Success
        return TRUE;
    } else {
        // Error

#ifdef DBG
        // Sanity check to make sure we don't leak memory
        {
            TML tmlTemp = {0};
            if (memcmp(&tmlTemp, pTml, sizeof(tmlTemp))) {
                Assert(!"TML structures must be blank");
            }
        }
#endif

        return FALSE;
    }
}

BPSTATUS
BPSearchContexts(
                 LPBPP lpBpp,
                 char * szAddrExpr,
                 PTML  pTml,
                 BOOL ImplementationOnly,
                 BOOL LoadSymbols
                 )
{
    Assert(pTml);

#ifdef DBG
    // Sanity check to make sure we don't leak memory
    {
        TML tmlTemp = {0};
        if (memcmp(&tmlTemp, pTml, sizeof(tmlTemp))) {
            Assert(!"TML structures must be blank");
        }
    }
#endif

    BPSTATUS    Ret;
    char        rgch[2000] = {0};
    BOOL        Ok;
    char        *p = NULL;

    //
    //  Initialize our context buffer
    //
    if ( lpBpp->atmCxtDef != 0 || lpBpp->atmAddrCxt != 0 ) {

        if ( lpBpp->atmAddrCxt != 0 ) {
            GetAtomName( lpBpp->atmAddrCxt, rgch, sizeof(rgch)-1 );
        } else {
            GetAtomName( lpBpp->atmCxtDef, rgch, sizeof(rgch)-1 );
        }

        //
        //  If the context does not have a module, try binding
        //  in all known modules.
        //
        Ok = FALSE;
        if ( p = (PSTR) strchr( (LPBYTE) rgch, ',' ) ) {
            if ( p = strpbrk( p+1, ",}" ) ) {
                if ( *p == CLOSECONTEXTOP ) {
                    *p = ',';
                    *(p+1) = '\0';
                    Ok = TRUE;
                } else {
                    p++;
                    p += strspn( p, " \t" );

                    if ( *p == CLOSECONTEXTOP ) {
                        *p = '\0';
                        Ok = TRUE;
                    }
                }
            }
        }

        if ( !Ok ) {
            //
            //  Bogus context, get out
            //
            return BPError;
        }

    } else {
        strcpy( rgch, "{,," );
        Assert(strlen(rgch) < sizeof(rgch));
    }

    //
    //  Get all the exes in which this breakpoint can bind.
    //
    TML         Tml = {0};
    PHTM        pTmList = NULL;

    if (SearchContexts(lpBpp, rgch, szAddrExpr, &Tml,
        &pTmList, ImplementationOnly, TRUE)) {

        MMbUnlockMb( Tml.hTMList );
        *pTml   = Tml;
        Ret     = BPNOERROR;

    } else {

        Ret     = BPError;
    }

    return Ret;
}

BPSTATUS
BPBindFromTmList(
                 LPBPP lpBpp,
                 PTML pTml,
                 BOOL fOnList,
                 char *BpBuf
                 )
{
    BPSTATUS    Ret;
    PHTM        pTmList = NULL;
    LPBPP       lpBpp2 = NULL;
    HBPT        hBpt2 = {0};
    BOOL        fOnList2;
    char        rgch[2000] = {0};
    int         i;
    EEHSTR      eehstr;
    DWORD       cb;
    char *      pch = NULL;
    EERADIX     eeradix;

    Assert(pTml->hTMList);

    pTmList = (PHTM)MMLpvLockMb ( pTml->hTMList );

    if (pTml->cTMListAct <= 1)  {

        Ret = BPBindTML( &pTmList[0], lpBpp, fOnList );

    } else {

        //
        //  The BP could bind to more than 1 exe. Let the
        //  user pick the exe(s) to use.
        //
        Ret = (BPSTATUS)BPTResolve( BpBuf, (PVOID)pTml, &(lpBpp->Cxf), TRUE );

        if ( Ret == BPNOERROR ) {

            for (i=0; i < (int)pTml->cTMListAct; i++ ) {

                if ( i == (int)pTml->cTMListAct-1 ) {

                    lpBpp2   = lpBpp;
                    fOnList2 = fOnList;

                } else {

                    hBpt2  = LLPlleCreate(PlliBpList);

                    if ( hBpt2 == NULL ) {
                        Ret = BPError;
                        break;
                    }

                    lpBpp2 = (LPBPP) LLLpvFromPlle( hBpt2 );

                    memcpy( lpBpp2, lpBpp, sizeof( BPP ) );

                    if ( lpBpp->atmAddrCxt ) {
                        GetAtomName(lpBpp->atmAddrCxt, rgch, sizeof(rgch)-1);
                        lpBpp2->atmAddrCxt = AddAtom(rgch);
                    }

                    if ( lpBpp->szAddrExpr ) {
                        lpBpp2->szAddrExpr = _strdup( lpBpp->szAddrExpr );
                    }

                    if ( lpBpp->atmCxtDef ) {
                        GetAtomName(lpBpp->atmCxtDef, rgch, sizeof(rgch)-1);
                        lpBpp2->atmCxtDef = AddAtom(rgch);
                    }

                    if ( lpBpp->atmPassCount ) {
                        GetAtomName(lpBpp->atmPassCount, rgch, sizeof(rgch)-1);
                        lpBpp2->atmPassCount = AddAtom(rgch);
                    }

                    if ( lpBpp->szCmdLine ) {
                        lpBpp2->szCmdLine = _strdup( lpBpp->szCmdLine );
                    }

                    if ( lpBpp->atmExpr ) {
                        GetAtomName(lpBpp->atmExpr, rgch, sizeof(rgch)-1);
                        lpBpp2->atmExpr = AddAtom(rgch);
                    }

                    if ( lpBpp->atmData ) {
                        GetAtomName(lpBpp->atmData, rgch, sizeof(rgch)-1);
                        lpBpp2->atmData = AddAtom(rgch);
                    }

                    if ( lpBpp->atmRangeSize ) {
                        GetAtomName(lpBpp->atmRangeSize, rgch, sizeof(rgch)-1);
                        lpBpp2->atmRangeSize = AddAtom(rgch);
                    }

                    if ( lpBpp->atmHighlightFile ) {
                        GetAtomName(lpBpp->atmHighlightFile, rgch, sizeof(rgch)-1);
                        lpBpp2->atmHighlightFile = AddAtom(rgch);
                    }

                    Ret = BPAddToList( hBpt2, -1 );

                    if ( Ret != BPNOERROR ) {
                        break;
                    }

                    fOnList2 = TRUE;
                }

                //
                //  Replace the original address expression with the
                //  expression returned from the EE, which will be
                //  unambiguous.
                //
                eeradix = radix;
                Dbg(EEGetExprFromTM( &pTmList[i], &eeradix, &eehstr, &cb) == EENOERROR);
                if (eehstr) {
                    if ( pch = (PSTR) MMLpvLockMb( eehstr ) ) {
                        if ( lpBpp2->szAddrExpr ) {
                            free(lpBpp2->szAddrExpr);
                        }
                        lpBpp2->szAddrExpr = _strdup(pch);
                    }
                    MMbUnlockMb( eehstr );
                    EEFreeStr( eehstr );
                }

                Ret = BPBindTML( &pTmList[i], lpBpp2, fOnList2 );

                if ( Ret != BPNOERROR ) {
                    break;
                }
            }
        }
    }

    MMbUnlockMb( pTml->hTMList );
    EEFreeTML( pTml );

    return Ret;
}

BOOLEAN
BPIsThunk(
          LPBPP       lpBpp,
          TML         TMLT
          )
{

    PHTM        TMList = NULL;
    PHTM        pHtm = NULL;
    DWORD       bpsegtype   = EECODE;
    CXF         Cxf= {0};
    UOFFSET      Offset = 0;
    HSYM        hSym = NULL;
    BOOLEAN     IsThunk     = FALSE;

    TMList = (PHTM)MMLpvLockMb ( TMLT.hTMList );

    if (TMLT.cTMListAct == 1)  {

        pHtm = &TMList[0];

        //
        //  Evaluate the address for the breakpoint.
        //
        SHSetCxtMod( &lpBpp->addr, SHpCXTFrompCXF(&(lpBpp->Cxf)));

        if ( !EEvaluateTM(pHtm, SHhFrameFrompCXF(&(lpBpp->Cxf)), EEBPADDRESS) &&
             !BPADDRFromTM(pHtm, &bpsegtype, &(lpBpp->addr))) {

            memset( &Cxf, 0, sizeof(CXF) );
            if ( SHSetCxt( (LPADDR)(&(lpBpp->addr)), (PCXT)(&Cxf) ) ) {

                if (!ADDR_IS_LI (*SHpADDRFrompCXT (SHpCXTFrompCXF (&Cxf)))) {
                    OSDUnFixupAddr (lpBpp->hPid, NULL, SHpADDRFrompCXT (SHpCXTFrompCXF (&Cxf)));
                }

                //
                //  Get the symbol at the given address.
                //
                Offset = SHGetNearestHSYM( SHpADDRFrompCXT(SHpCXTFrompCXF(&Cxf)),
                                           SHHMODFrompCXT(SHpCXTFrompCXF(&Cxf)),
                                           EECODE,
                                           &hSym );

                if ( Offset == 0 ) {
                    if ( SHIsThunk( hSym ) ) {
                        IsThunk = TRUE;
                    }
                }
            }
        }
    }

    MMbUnlockMb( TMLT.hTMList );

    return IsThunk;
}




BPSTATUS
BPBindTML(
          PHTM    pHtm,
          LPBPP   lpBpp,
          BOOL    fOnList
          )
{
    DWORD       bpsegtype = EECODE;
    char        rgch[2000] = {0};
    char        rgch2[2000] = {0};
    BPSTATUS    BpRet = BPNOERROR;
    EESTATUS    ERet;
    int         err;
    XOSD        Xosd;
    BOOLEAN     Ok;
    int         cch;
    ADDR        Addr = {0};
    CXT         cxt = {0};
    FUNCTION_INFORMATION   FunctionInfo;

    SHSetCxtMod( &lpBpp->addr, SHpCXTFrompCXF(&(lpBpp->Cxf)));

    ERet = EEvaluateTM(pHtm, SHhFrameFrompCXF(&(lpBpp->Cxf)), EEBPADDRESS);
    BpRet = (ERet == EENOERROR) ? BPNOERROR : BPError;

    if (BPNOERROR == BpRet) {
        ERet = BPADDRFromTM(pHtm, &bpsegtype, &(lpBpp->addr));
        BpRet = (ERet == EENOERROR ) ? BPNOERROR : BPError;
    }

    if (BPNOERROR == BpRet) {

        //
        //  If a message breakpoint, we must set the breakpoint after
        //  the function prolog so we will be able to find the
        //  message value.
        //
        if ( lpBpp->fMsg ) {

            DWORD       dwLn;
            SHOFF       cbLn;
            SHOFF       dbLn;
            CXF         Cxf;
            UOFF32      Offset;
            HSYM        hSym;

            //
            //  Make sure that the address specified is a procedure.
            //
            Offset = 0;
            memset( &Cxf, 0, sizeof(CXF) );
            if ( SHSetCxt( (LPADDR)(&(lpBpp->addr)), (PCXT)&Cxf ) ) {

                if (!ADDR_IS_LI (*SHpADDRFrompCXT (SHpCXTFrompCXF (&Cxf)))) {
                    OSDUnFixupAddr (lpBpp->hPid, NULL, SHpADDRFrompCXT (SHpCXTFrompCXF (&Cxf)));
                }

                Offset = SHGetNearestHSYM( SHpADDRFrompCXT(SHpCXTFrompCXF(&Cxf)),
                                           SHHMODFrompCXT(SHpCXTFrompCXF(&Cxf)),
                                           EECODE,
                                           &hSym );
            }

            if ( Offset != 0 ) {
                BpRet = BPBadExpression;
            } else if (!SLLineFromAddr( &(lpBpp->addr), &dwLn, &cbLn, &dbLn ) ) {
                BpRet = BPBadExpression;
            } else {
                lpBpp->addr.addr.off += cbLn - dbLn + 1;
                if (!SLLineFromAddr( &(lpBpp->addr), &dwLn, &cbLn, &dbLn)) {
                    BpRet = BPBadExpression;
                } else {
                    lpBpp->iHighlightLine = dwLn;
                }
            }
        }

        if ( BpRet == BPNOERROR ) {

            SaveDebuggerPdTd();
            SetDebuggerPdTd( lpBpp );

            memset ( &cxt, 0, sizeof ( CXT ) );
            if (!SHSetCxt ( &lpBpp->addr, &cxt )) {

                BpRet = BPBadExpression;

            } else {

                //
                // make sure that we don't set a bp on the first
                // instruction of the function.  we must skip the prolog
                //
                Addr = lpBpp->addr;
                OSDFixupAddr( lpBpp->hPid, NULL, &Addr );

// v-vadimp - strange things in AddrPrologEnd for ia64, so just use ShIsInProlog for it
                if ( LppdOfHpid(lpBpp->hPid)->mptProcessorType != mptia64 &&
                        OSDGetFunctionInformation( lpBpp->hPid,
                                                0,
                                                &Addr,
                                                &FunctionInfo )
                                                == xosdNone) {

                    OFFSET Off = GetAddrOff(FunctionInfo.AddrPrologEnd);

                    if (LppdOfHpid(lpBpp->hPid)->mptProcessorType == mptdaxp ||
                            LppdOfHpid(lpBpp->hPid)->mptProcessorType == mptia64) {
                        Off &= ~3;
                    }
        
                    if ( Off > GetAddrOff( Addr ) ) {
                        GetAddrOff( Addr ) = Off;
                        OSDUnFixupAddr(lpBpp->hPid, NULL,  &Addr );
                        lpBpp->addr = Addr;
                    }

                } else {

                    if ( GetSrcMode_StatusBar() ) {
                        while( SHIsInProlog( &cxt ) ) {
                            cxt.addr.addr.off++;
                        }
                    }

                    lpBpp->addr = cxt.addr;
                }

                Addr = lpBpp->addr;

                if ( OSDFixupAddr( lpBpp->hPid, NULL, &Addr ) != xosdNone ) {
                    BpRet = BPBadExpression;
                }

            }

            RestoreDebuggerPdTd();

            if ( BpRet == BPNOERROR ) {

                //
                //  We have now successfully computed an address for the
                //  breakpoint.  Now if it is to be added -- do that and
                //  mark the breakpoint as having been added.
                //

                lpBpp->bpstate &= ~bpstateSets;
                lpBpp->bpstate |= bpstateVirtual;

                //
                //  If a memory breakpoint, try to bind the memory
                //  address expression
                //
                if ( !lpBpp->fMemory ) {

                    Ok = TRUE;

                } else {

                    //
                    // Make sure there is some size to the memory breakpoint
                    //

                    if (lpBpp->cbDataSize == 0) {
                        lpBpp->cbDataSize = 1;
                    }

#if 1

                    GetAtomName(lpBpp->atmData, rgch, sizeof(rgch)-1);
                    CPGetAddress( rgch,
                                  &cch,
                                  &(lpBpp->AddrMem),
                                  radix,
                                  &(lpBpp->Cxf),
                                  FALSE,
                                  g_contWorkspace_WkSp.m_bMasmEval);
                    Ok = TRUE;
#else

                    if ( LppdCur && (LppdCur->pstate == psPreRunning) ) {
                        //
                        //  At this point we most probably won't be able to
                        //  read memory (or evaluate addresses) so defer it
                        //  until we hit the address.
                        //
                        Ok = TRUE;

                    } else {

                        GetAtomName(lpBpp->atmData, rgch, sizeof(rgch)-1);
                        Ok = (CPGetAddress( rgch,
                                            &cch,
                                            &(lpBpp->AddrMem),
                                            radix,
                                            &(lpBpp->Cxf),
                                            FALSE,
                                            g_contWorkspace_WkSp.m_bMasmEval) == EENOERROR);

                        if ( !Ok ) {

                            //
                            //  Could not get the memory address. This might
                            //  be normal (e.g. trying to get the address
                            //  of a local variable in another function), so
                            //  we'll just defer it until we hit the address.
                            //
                            Ok = TRUE;
                        } else {

                            //
                            //  Verify that the memory is readable.
                            //
                            lpBpp->Mem = (PBYTE)malloc( lpBpp->cbDataSize );
                            if ( !lpBpp->Mem ) {

                                Ok = FALSE;

                            } else {

                                OSDFixupAddr( lpBpp->hPid, NULL, &(lpBpp->AddrMem) );
                                Ok = ( OSDSetAddr( lpBpp->hPid,
                                                   lpBpp->hTid,
                                                   adrCurrent,
                                                   &(lpBpp->AddrMem)) ==
                                                                     xosdNone);
                                if ( Ok ) {
                                    OSDUnFixupAddr(lpBpp->hPid, NULL,  &(lpBpp->AddrMem) );
                                    Ok = (OSDPtrace( osdReadBuf,
                                                     lpBpp->cbDataSize,
                                                     lpBpp->Mem,
                                                     lpBpp->hPid,
                                                     lpBpp->hTid) ==
                                                       (XOSD)lpBpp->cbDataSize);
                                }

                                if ( !Ok ) {
                                    free( lpBpp->Mem );
                                    lpBpp->Mem = NULL;
                                }

                            }

                        }

                        if ( !Ok ) {
                            lpBpp->bpstate &= ~bpstateSets;
                            lpBpp->bpstate |= bpstateNotSet;

                            BpRet = BPError;
                        }
                    }
#endif

                }


                if ( Ok &&
                     fOnList &&
                     !lpBpp->fMarkAdd &&
                     (lpBpp->bpstate & bpstateEnabled)) {

                    // M00TODO -- deal with single thread breakpoints

                    BPInitBps( lpBpp, BpsBuffer, TRUE );
                    Xosd = OSDBreakpoint( lpBpp->hPid, BpsBuffer );

                    if ( Xosd == xosdNone ) {
                        lpBpp->bpstate &= ~bpstateSets;
                        lpBpp->bpstate |= bpstateSet;
                        lpBpp->qwNotify = QwNotification(BpsBuffer)[0];

                        if ( lpBpp->fMemory ) {
                            MemoryBpCount++;
                        }

                    } else {
                        lpBpp->bpstate &= ~bpstateSets;
                        lpBpp->bpstate |= bpstateNotSet;
                        // lpBpp->fMarkAdd = TRUE;

                        BpRet = BPError;
                    }
                }

                if (BpRet == BPNOERROR ) {

                    BPBoundFixContext( lpBpp );

                    if (lpBpp->atmPassCount != 0) {

                        GetAtomName(lpBpp->atmPassCount, rgch, sizeof(rgch)-1);
                        lpBpp->iPassCount = CPGetNbr(rgch,
                                                     radix,
                                                     fCaseSensitive,
                                                     &(lpBpp->Cxf),
                                                     rgch2,
                                                     &err,
                                                     g_contWorkspace_WkSp.m_bMasmEval
                                                     );
                    }
                }
            }
        }
    }

    return BpRet;
}



BPSTATUS
BPSetPidTid (
             LPBPP   lpBpp
             )
{
    LPTD        lptd = NULL;
    LPPD        lppd = NULL;
    ADDR        Addr = {0};

    //
    //  Make sure that the process number is valid. Set the
    //  breakpoint process id.
    //

    if ( lpBpp->fProcess ) {

        //
        //  Get the process ID for the process
        //
        lppd = LppdOfIpid(lpBpp->iPid);

    } else {

        //
        // use the current process
        //
        lppd = LppdCur;

    }

    if (!lppd) {
        return BPBadProcess;
    }

    lpBpp->hPid = lppd->hpid;

    //
    //  Make sure that the thread number is valid. Set the
    //  breakpoint thread id.
    //
    //
    // Find the thread;
    //  if there is no match
    //   if using the current process, use current thread
    //   if not using current process, use the first thread in the process
    //

    lptd = NULL;

    if ( lpBpp->fThread ) {
        lptd = LptdOfLppdItid(lppd, lpBpp->iTid);
    }

    if (!lptd && lppd == LppdCur) {
        lptd = LptdCur;
    }

    if (!lptd) {
        lptd = lppd->lptdList;
    }

    if (!lptd) {
        return BPBadThread;
    }

    if ( lpBpp->fThread ) {
        lpBpp->hTid = lptd->htid;
    } else {
        lpBpp->hTid = 0;
    }

    //
    //  Obtain a context based on the process & thread
    //
    OSDGetAddr( lppd->hpid, lptd->htid, adrPC, &Addr );
    if ( (Addr.addr.off == 0) && (Addr.addr.seg == 0) ) {
        OSDGetAddr( lppd->hpid, lptd->htid, adrPC, &Addr );
    }
    if ( ! ADDR_IS_LI ( Addr ) ) {
        OSDUnFixupAddr (lppd->hpid, NULL, &Addr );
    }
    SHSetCxt( &Addr, SHpCxtFrompCxf( &(lpBpp->Cxf) ) );

    return BPNOERROR;
}



/***    BPFormatHbpt
**
**  Synopsis:
**      bpstatus = BPFormatHbpt(hBpt, lpb, cb, flags)
**
**  Entry:
**      hBpt    - Handle to breakpoint to be formated
**      lpb     - pointer to buffer to place string in
**      cb      - size of buffer in bytes
**      flags   - flags controling the formatting of the breakpoint
**
**  Returns:
**      Error code if needed and string in buffer pointed by lpb
**
**  Description:
**
*/

BPSTATUS
BPFormatHbpt(
    HBPT       hBpt,
    char *     lpb,
    DWORD      cb,
    DWORD      flags
    )
{
    char            rgch[2000] = {0};
    char            rgch2[2000] = {0};
    LPBPP           lpBpp = NULL;
    BOOL            fSpace = FALSE;


    // M00BUG -- verify hBpt is in list of breakpoints

    lpBpp = (LPBPP) LLLpvFromPlle(hBpt);

    /*
    **  Initialize to a null string
    */

    *lpb = 0;

    /*
    **  Check to see if item counts are to be included
    */

    if (flags & BPFCF_ITEM_COUNT) {
        sprintf(rgch, "%d ", lpBpp->iBreakpointNum);
        strcat(lpb, rgch);
    }

    /*
    **  If we are doing dialog box interfacing --- add 'a' and 'd' to mark
    **  items as to be added or to be deleted
    */

    if (flags & BPFCF_ADD_DELETE) {

        if ( lpBpp->fReplacement ) {
            strcat(lpb, "c");
        } else {
            strcat(lpb, " ");
        }

        if (lpBpp->fMarkAdd && !lpBpp->fReplacement) {
            strcat(lpb, "a");
        } else {
            strcat(lpb, " ");
        }

        if (lpBpp->fMarkDelete) {
            strcat(lpb, "d");
        } else {
            strcat(lpb, " ");
        }

        strcat(lpb, " ");
    }

    /*
    **
    */

    if (lpBpp->bpstate & bpstateEnabled) {
        if (lpBpp->fMarkDisable)
            strcat(lpb, "D");
        else
            strcat(lpb, "E");
    } else {
        if (lpBpp->fMarkEnable)
            strcat(lpb, "E");
        else
            strcat(lpb, "D");
    }

    switch ( lpBpp->bpstate & bpstateSets ) {
    case bpstateNotSet:
        strcat(lpb, "U ");
        break;

    case bpstateVirtual:
        if (lpBpp->bpstate & bpstateEnabled) {
            strcat(lpb, "V ");
        } else {
            strcat(lpb, "  ");
        }
        break;

    case bpstateSet:
        strcat(lpb, "  ");
        break;

    default:
        assert(FALSE);
        break;
    }

    /*
    **  Now check each field and append as appropriate
    */

    if (lpBpp->atmAddrCxt != 0) {
        assert(lpBpp->szAddrExpr != NULL);

        GetAtomName(lpBpp->atmAddrCxt, rgch, sizeof(rgch));
        BPShortenContext( rgch, rgch2 );
        if (strlen(rgch2) > cb) {
            return BPError;
        }
        strcat(lpb, rgch2);
        cb -= strlen(rgch2);

    } else if (lpBpp->atmCxtDef != 0) {

        GetAtomName(lpBpp->atmCxtDef, rgch, sizeof(rgch));
        BPShortenContext( rgch, rgch2 );
        if (strlen(rgch2) > cb) {
            return BPError;
        }
        strcat(lpb, rgch2);
        cb -= strlen(rgch2);
    }

    if (lpBpp->szAddrExpr != 0) {
        strcpy( rgch, lpBpp->szAddrExpr );
        if (strlen(rgch) > cb) {
            return BPError;
        }
        strcat(lpb, rgch);
        cb -= strlen(rgch);
        fSpace = TRUE;
    }


    /*
    **  Put out the expression if it exists
    */

    if (lpBpp->atmExpr != 0) {
        BPQueryExprOfHbpt( hBpt, rgch, sizeof(rgch));
        if (strlen(rgch)+5 > cb) {
            return BPError;
        }

        if (fSpace) {
            strcat(lpb, " ?\"");
            cb -= 4;
        } else {
            strcat(lpb, "?\"");
            cb -= 3;
        }

        strcat(lpb, rgch);
        strcat(lpb, "\"");
        cb -= strlen(rgch);
        fSpace = TRUE;
    }

    /*
    **  Put out the address expression if it exists
    */

    if (lpBpp->atmData != 0) {
        GetAtomName(lpBpp->atmData, rgch, sizeof(rgch));
        if (strlen(rgch)+5 > cb) {
            return BPError;
        }

        if (fSpace) {
            strcat(lpb, " =\"");
            cb -= 4;
        } else {
            strcat(lpb, "=\"");
            cb -= 3;
        }

        strcat(lpb, rgch);
        strcat(lpb, "\"");
        cb -= strlen(rgch);
        fSpace = TRUE;

    }

    /*
    **  Put out the range if it exists
    */

    if (lpBpp->atmRangeSize != 0) {
        BPQueryMemorySizeOfHbpt( hBpt, rgch, sizeof(rgch));
        if (strlen(rgch)+5 > cb) {
            return BPError;
        }

        if (fSpace) {
            strcat(lpb, " /R");
            cb -= 3;
        } else {
            strcat(lpb, "/R");
            cb -= 2;
        }

        strcat(lpb, rgch);
        cb -= strlen(rgch);
        fSpace = TRUE;
    }

    /*
    **
    */

    if (!lpBpp->fCodeAddr && (lpBpp->BpType &
        (BP_DATA_READ | BP_DATA_WRITE | BP_EXECUTE)) ) {
        if (fSpace) {
            strcat(lpb, " /A");
            cb -= 3;
        } else {
            strcat(lpb, "/A");
            cb -= 2;
        }

        if (lpBpp->BpType & BP_DATA_READ) {
            strcat(lpb, "r");
            cb--;
        }

        if (lpBpp->BpType & BP_DATA_WRITE) {
            strcat(lpb, "w");
            cb--;
        }

        if (lpBpp->BpType & BP_EXECUTE) {
            strcat(lpb, "e");
            cb--;
        }

        fSpace = TRUE;
    }

    //
    //  Add message if it exists
    //
    if ( lpBpp->fMsg && lpBpp->atmMsg ) {
        GetAtomName(lpBpp->atmMsg, rgch, sizeof(rgch));
        if (strlen(rgch)+5 > cb) {
            return BPError;
        }

        if (fSpace) {
            strcat(lpb, " /M");
            cb -= 3;
        } else {
            strcat(lpb, "/M");
            cb -= 2;
        }

        strcat(lpb, rgch);
        cb -= strlen(rgch);
        fSpace = TRUE;
    }

    /*
    **  Add the command set if one exists
    */

    if (lpBpp->szCmdLine ) {
        strcpy(rgch, lpBpp->szCmdLine);
        if (strlen(rgch)+5 > cb) {
            return BPError;
        }

        if (fSpace) {
            strcat( lpb, " /C\"");
            cb -= 5;
        } else {
            strcat( lpb, "/C\"");
            cb -= 4;
        }

        strcat(lpb, rgch);
        cb -= strlen(rgch);

        strcat(lpb, "\"");
        fSpace = TRUE;
    }

    /*
    **  Add pass count if exists
    */

    BPQueryPassCntOfHbpt( hBpt, rgch, sizeof(rgch)-1);
    if (rgch[0] != 0) {
        if (strlen(rgch)+5 > cb) {
            return BPError;
        }

        if (fSpace) {
            strcat( lpb, " /P");
            cb -= 3;
        } else {
            strcat( lpb, "/P");
            cb -= 2;
        }
        strcat( lpb, rgch );
        cb -= strlen(rgch);


        if ( !(flags & BPFCF_WRKSPACE) ) {
            BPQueryPassLeftOfHbpt( hBpt, rgch, sizeof(rgch)-1);
            if (rgch[0] != 0) {
                if (strlen(rgch)+5 > cb) {
                    return BPError;
                }

                strcat( lpb, "(");
                strcat( lpb, rgch );
                strcat( lpb, ")" );
                cb -= (strlen(rgch) + 2);
            }
        }
        fSpace = TRUE;
    }

    if ( BPQueryProcessOfHbpt( hBpt, rgch, sizeof(rgch) ) == BPNOERROR ) {
        if ( *rgch != '\0' ) {
            if (strlen(rgch)+5 > cb) {
                return BPError;
            }

            if (fSpace) {
                strcat( lpb, " /H");
                cb -= 3;
            } else {
                strcat( lpb, "/H");
                cb -= 2;
            }

            strcat(lpb, rgch);
            cb -= strlen(rgch);

            fSpace = TRUE;
        }
    }

    //
    //  Add the thread if specified
    //
    if (lpBpp->fThread ) {

        sprintf( rgch, "%d", lpBpp->iTid );
        if (strlen(rgch)+5 > cb) {
            return BPError;
        }

        if (fSpace) {
            strcat( lpb, " /T");
            cb -= 3;
        } else {
            strcat( lpb, "/T");
            cb -= 2;
        }

        strcat(lpb, rgch);
        cb -= strlen(rgch);

        fSpace = TRUE;
    }

    //
    //  Add the quiet defer switch if set
    //
    if (lpBpp->fQuiet) {

        if ( fSpace ) {
            strcat(lpb, " /Q");
            cb -= 3;
        } else {
            strcat(lpb, "/Q");
            cb -= 2;
        }

        fSpace = TRUE;
    }


    //
    //  Add the WndProc switch if set and caller wants it
    //
    if ( flags & BPFCF_WNDPROC ) {
        if (lpBpp->fWndProc) {
            if ( fSpace ) {
                strcat(lpb, " /W");
                cb -= 3;
            } else {
                strcat(lpb, "/W");
                cb -= 2;
            }

            fSpace = TRUE;
        }
    }

    return BPNOERROR;
}


/***    FParseableSz
**
**  Synopsis:
**      bool = FParseableSz(sz)
**
**  Entry:
**      sz      - string to be tested
**
**  Returns:
**      TRUE if it is a parsable expression else FALSE
**
**  Description:
**      This function will check to see if the expression evaluator can
**      actually parse the requested expression.  The expression is parsed
**      and then the resultant TM is freed
**
**  M00TODO -- did we use all of it.
*/

static
BOOL
FParseableSz(
             char * sz
             )
{
    HTM     hTM = {0};
    ULONG   ulch = 0;

    if (EEParse(sz, radix, fCaseSensitive, &hTM, &ulch) != EENOERROR) {
        return(FALSE);
    }
    EEFreeTM(&hTM);

    return TRUE;
}                                   /* FParseableSz() */

BPSTATUS
BPAddHighLight(
               LPBPP lpBpp
               )
{
    char    rgchSource[2000] = {0};
    BOOLEAN fRemoveHighLight    = TRUE;
    BOOLEAN fRemoveAsmHighLight = TRUE;
    INT     line = 0;
    ATOM    atm = 0;


    if (!(lpBpp->bpstate & bpstateNotSet)) {
        if (!BPCBGetSourceFromAddr( &(lpBpp->addr),
                                   rgchSource,
                                   sizeof(rgchSource),
                                   &line ) ) {
            if (lpBpp->fHighlight) {
                GetAtomName(lpBpp->atmHighlightFile,
                            rgchSource,
                            sizeof(rgchSource));
                BPCBSetHighlight((char *)rgchSource,
                                 (DWORD)(lpBpp->iHighlightLine),
                                 TRUE,
                                 FALSE,
                                 (WORD)BRKPOINT_STATUS);
                lpBpp->fHighlight = FALSE;
                DeleteAtom(lpBpp->atmHighlightFile);
                lpBpp->atmHighlightFile = atmNull;
                lpBpp->iHighlightLine = (USHORT) -1;
            }
            return BPNOERROR;
        }
        atm = AddAtom( rgchSource );
    }
    else {
        atm = lpBpp->atmHighlightFile;
        line = lpBpp->iHighlightLine;
        GetAtomName(lpBpp->atmHighlightFile, rgchSource, sizeof(rgchSource));
    }

    if (lpBpp->fHighlight) {
        if ( !lpBpp->fMarkDisable ) {
            //
            //  Should highlight line now
            //
            if (( atm != lpBpp->atmHighlightFile ) || ((DWORD)line != lpBpp->iHighlightLine)) {
                if (atm != lpBpp->atmHighlightFile) {
                    GetAtomName(lpBpp->atmHighlightFile,
                                rgchSource,
                                sizeof(rgchSource));
                    BPCBSetHighlight((char *)rgchSource,
                                     (DWORD)(lpBpp->iHighlightLine),
                                     FALSE,
                                     FALSE,
                                     (WORD)BRKPOINT_STATUS );
                    DeleteAtom(lpBpp->atmHighlightFile);
                }
                else {
                    BPCBSetHighlight((char *)rgchSource,
                                     (DWORD)(lpBpp->iHighlightLine),
                                     FALSE,
                                     FALSE,
                                     (WORD)BRKPOINT_STATUS);
                }
                BPCBSetHighlight((char *)rgchSource,
                                 (DWORD)line,
                                 TRUE,
                                 FALSE,
                                 (WORD)BRKPOINT_STATUS);
                lpBpp->atmHighlightFile = atm;
                lpBpp->iHighlightLine = line;
            }
            else {
                BPCBSetHighlight((char *)rgchSource,
                                 (DWORD)line,
                                 TRUE,
                                 FALSE,
                                 (WORD)BRKPOINT_STATUS);
            }
        }
        else {
            //
            //  Don't highlight now, but remember what must
            //  be hilighted.
            //
            lpBpp->atmHighlightFile = atm;
            lpBpp->iHighlightLine   = line;
        }
    }
    else {
        BPCBSetHighlight((char *)rgchSource,
                         (DWORD)line,
                         TRUE,
                         FALSE,
                         (WORD)BRKPOINT_STATUS );
        lpBpp->fHighlight = TRUE;
        lpBpp->atmHighlightFile = atm;
        lpBpp->iHighlightLine = line;
    }

    return BPNOERROR;
}

BPSTATUS
BPRemoveHighLight(
    LPBPP lpBpp
    )
{
    HBPT    hBpt1 = NULL;
    LPBPP   lpBpp1 = NULL;
    char    rgchSource[2000] = {0};
    char    rgchSource1[2000] = {0};
    BOOLEAN fRemoveHighLight    = TRUE;
    BOOLEAN fRemoveAsmHighLight = TRUE;
    INT     line = 0;
    char    drive[_MAX_DRIVE] ={0};
    char    dir[_MAX_DIR] = {0};
    char    fname[_MAX_FNAME] = {0};
    char    ext[_MAX_EXT] = {0};


    line = lpBpp->iHighlightLine;
    GetAtomName( lpBpp->atmHighlightFile, rgchSource, sizeof(rgchSource));
    _splitpath( rgchSource, drive, dir, fname, ext );
    wsprintf( rgchSource, "%s%s", fname, ext );

    //
    //  We should only remove the highlight if
    //  no other breakpoint is set on the same
    //  line.

    //
    //  Traverse the breakpoint list looking for someone with the
    //  same highlight.
    //
    hBpt1 = LLPlleFindNext( PlliBpList, (HBPT) NULL);
    while (hBpt1 != NULL) {
        lpBpp1 = (LPBPP) LLLpvFromPlle( hBpt1 );

        if ( lpBpp1->iHighlightLine == (DWORD)line &&
            !(lpBpp1->bpstate & bpstateDeleted)) {

            //
            //  If there is another breakpoint in same
            //  source, we don't remove source hilight
            //
            if ( fRemoveHighLight ) {
                GetAtomName( lpBpp1->atmHighlightFile, rgchSource1, sizeof(rgchSource1));
                _splitpath( rgchSource1, drive, dir, fname, ext );
                wsprintf( rgchSource1, "%s%s", fname, ext );
                if (_stricmp( rgchSource, rgchSource1 ) == 0) {
                    fRemoveHighLight = FALSE;
                }
            }

            //
            //  If there is another breakpoint in same
            //  address, we don't remove the asm hilight
            //
            if ( FAddrsEq( lpBpp->addr, lpBpp1->addr ) ) {
                fRemoveAsmHighLight = FALSE;
            }
        }

        hBpt1 = LLPlleFindNext( PlliBpList, hBpt1 );
    }

    //
    //  Remove the highlight if we're supposed to.
    //
    if ( fRemoveHighLight ) {
        BPCBSetHighlight(rgchSource, line, FALSE, FALSE, BRKPOINT_STATUS );
    }

    if ( fRemoveAsmHighLight ) {
        BPCBSetHighlight(rgchSource, line, FALSE, FALSE, UBP_LINE );
    }

  return BPNOERROR;

}

BPSTATUS
BPHighlightSourceFile(
                      char *sname
                      )
{
    HBPT                hBpt = NULL;
    LPBPP               lpBpp = NULL;
    char                rgchSource[2000] = {0};
    char                rgch[2000] = {0};
    char                drive[_MAX_DRIVE] = {0};
    char                dir[_MAX_DIR] = {0};
    char                fname[_MAX_FNAME] = {0};
    char                ext[_MAX_EXT] = {0};


    _splitpath( sname, drive, dir, fname, ext );
    wsprintf( rgch, "%s%s", fname, ext );
    hBpt = LLPlleFindNext( PlliBpList, (HBPT) NULL);
    SaveDebuggerPdTd();
    while (hBpt != NULL) {
        lpBpp = (LPBPP) LLLpvFromPlle( hBpt );
        GetAtomName( lpBpp->atmHighlightFile, rgchSource, sizeof(rgchSource));
        _splitpath( rgchSource, drive, dir, fname, ext );
        wsprintf( rgchSource, "%s%s", fname, ext );
        if (_stricmp(rgch,rgchSource)==0) {
            BPAddHighLight( lpBpp );
        }
        hBpt = LLPlleFindNext( PlliBpList, hBpt );
    }
    RestoreDebuggerPdTd();
    return BPNOERROR;
}

/***    BPCommit
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

BPSTATUS
BPCommit()
{
    HBPT                hBpt = NULL;
    HBPT                hBpt2 = NULL;
    LPBPP               lpBpp = NULL;
    char                rgchSource[2000] = {0};
    INT                 line;
    BOOLEAN             fHighLight;
    XOSD                Xosd;
    ADDR                AddrHbpt = {0};

    /*
    **  Get the first item and walk the list of all breakpoints
    **  looking for those marked for change
    */

    hBpt = LLPlleFindNext( PlliBpList, (HBPT) NULL);

    SaveDebuggerPdTd();

    while (hBpt != NULL) {

        lpBpp = (LPBPP) LLLpvFromPlle( hBpt );


        //
        //  If Marked for change, replace it
        //
        if ( lpBpp->fMarkChange && !lpBpp->fMarkDelete ) {
            BPReplace( lpBpp );
        }

        lpBpp->fNewMark = FALSE;

        if ( lpBpp->fMarkAdd    || lpBpp->fMarkDelete  ||
             lpBpp->fMarkEnable || lpBpp->fMarkDisable ) {

            SetDebuggerPdTd( lpBpp );
        }

        if (lpBpp->fMarkAdd) {

            assert( lpBpp->fMarkDelete == FALSE );

            /*
            **  Set it at the low level if the state is correct
            **
            **  Correct state requires:
            **      1.  That bpstateEnable be set
            **      2.  The state is virutal (i.e. we have an address)
            */

            if (lpBpp->bpstate == (bpstateVirtual | bpstateEnabled)) {

                if ( lpBpp->fMarkDisable ) {
                    Xosd = xosdNone;
                    lpBpp->bpstate = bpstateVirtual;
                } else {
                    BPInitBps( lpBpp, BpsBuffer, TRUE );
                    Xosd = OSDBreakpoint( lpBpp->hPid, BpsBuffer );
                    if ( Xosd == xosdNone ) {
                        lpBpp->bpstate &= ~bpstateSets;
                        lpBpp->bpstate |= bpstateSet;
                        lpBpp->qwNotify = QwNotification(BpsBuffer)[0];

                        if ( lpBpp->fMemory ) {
                            MemoryBpCount++;
                        }
                    }
                }

                if ( Xosd == xosdNone ) {
                    RestoreDebuggerPdTd();
                    BPAddHighLight( lpBpp ); //*
                }

                /*
                **  Clear the notation that it was marked as added
                */

                lpBpp->fMarkAdd     = FALSE;
                lpBpp->fMarkDisable = FALSE;

            } else if (lpBpp->bpstate == (bpstateNotSet | bpstateEnabled)) {
                if (lpBpp->iHighlightLine != (USHORT) -1) {
                    GetAtomName( lpBpp->atmHighlightFile, rgchSource, sizeof(rgchSource));
                    RestoreDebuggerPdTd();

                    BPCBSetHighlight(rgchSource, lpBpp->iHighlightLine, TRUE, FALSE, UBP_LINE );
                    lpBpp->fHighlight = TRUE;
                }
            }



        } else if (lpBpp->fMarkDelete) {

            if ( lpBpp->fMarkChange ) {
                LLFDeletePlleFromLl( PlliBpList, lpBpp->ChangeHbpt );
                lpBpp->fMarkChange = FALSE;
                lpBpp->ChangeHbpt  = NULL;
            }

            hBpt2 = hBpt;

            if (lpBpp->bpstate == (bpstateSet | bpstateEnabled)) {

                BPInitBps( lpBpp, BpsBuffer, FALSE );
                Dbg(OSDBreakpoint( lpBpp->hPid, BpsBuffer ) == xosdNone );
                if ( lpBpp->fMemory ) {
                    MemoryBpCount--;
                }
            }
                /**
                **  M00TODO -- other states
                */

            if ( lpBpp->fHighlight ) {

                //
                //  Remember our source, line & address. We'll need these after
                //  deleting the breakpoint, to see if someone has a
                //  brekpoint on the same source/line.
                //
                GetAtomName( lpBpp->atmHighlightFile, rgchSource, sizeof(rgchSource));
                line              = (int)(lpBpp->iHighlightLine);
                memcpy( &AddrHbpt, &(lpBpp->addr), sizeof( ADDR ) );
                lpBpp->fHighlight = FALSE;
                fHighLight        = TRUE;

            } else {
                fHighLight = FALSE;
            }

#if 0
            if (lpBpp->Mem != NULL ) {
                free( lpBpp->Mem );
                lpBpp->Mem = NULL;
            }
#endif

            hBpt = LLPlleFindNext( PlliBpList, hBpt);
            RestoreDebuggerPdTd();
            lpBpp->bpstate |= bpstateDeleted;
            BPRemoveHighLight( lpBpp ); //*
            LLFDeletePlleFromLl( PlliBpList, hBpt2 );
            continue;

            /*
            **  The breakpoint is marked as we are to make it disabled.
            **  The following steps must therefore be taken:
            **
            **  1.  If the breakpoint is currently set then it must be
            **      unset.
            **  2.  The internal state of the breakpoint must be re-marked
            **
            */

        } else if (lpBpp->fMarkDisable) {
            assert( lpBpp->fMarkEnable == FALSE);

            if ( lpBpp->bpstate == (bpstateSet | bpstateEnabled) ) {

                BPInitBps( lpBpp, BpsBuffer, FALSE );
                Dbg(OSDBreakpoint( lpBpp->hPid, BpsBuffer ) == xosdNone);

                lpBpp->bpstate &= ~bpstateSets;
                lpBpp->bpstate |= bpstateVirtual;

                if ( lpBpp->fMemory ) {
                    MemoryBpCount--;
                }

                RestoreDebuggerPdTd();
                if (BPCBGetSourceFromAddr( &(lpBpp->addr),
                                           rgchSource,
                                           sizeof(rgchSource),
                                           &line )) {
                    BPCBSetHighlight((char *)rgchSource,
                                     (DWORD)line,
                                     FALSE,
                                     FALSE,
                                     (WORD)BRKPOINT_STATUS);
                }
            }

            lpBpp->bpstate &= ~bpstateEnabled;
            lpBpp->fMarkDisable = FALSE;

            /*
            **  The breakpoint is marked to be enabled.  The following
            **  steps need to be taken.  Only enable if there is a live
            **  debuggee
            **
            **  1.  If the breakpoint does not have an address then
            **      we need to try and get one (???)
            **  2.  If we have an address then we need to set the breakpoint
            **  3.  The internal state of the breakpoint must be reset
            */
        } else if (lpBpp->fMarkEnable) {
            if (DebuggeeAlive()) {
                assert  ((lpBpp->bpstate & bpstateEnabled) == FALSE );

                if ( lpBpp->bpstate == bpstateNotSet ) {
                    BPBindHbpt( hBpt, NULL );
                }

                if ( lpBpp->bpstate == bpstateVirtual) {

                    BPInitBps( lpBpp, BpsBuffer, TRUE );
                    Dbg(OSDBreakpoint( lpBpp->hPid, BpsBuffer ) == xosdNone);

                    lpBpp->bpstate &= ~bpstateSets;
                    lpBpp->bpstate |= bpstateSet;
                    lpBpp->qwNotify = QwNotification(BpsBuffer)[0];

                    if ( lpBpp->fMemory ) {
                        MemoryBpCount++;
                    }
                }
                RestoreDebuggerPdTd();
                if (BPCBGetSourceFromAddr( &(lpBpp->addr), rgchSource,
                                                 sizeof(rgchSource), &line )) {
                    BPCBSetHighlight((char *)rgchSource,
                                     (DWORD)line,
                                     TRUE,
                                     FALSE,
                                     (WORD)BRKPOINT_STATUS);
                }
            }
            lpBpp->bpstate |= bpstateEnabled;
            lpBpp->fMarkEnable = FALSE;
        }

        /*
        **  Move to the next breakpoint
        */

        hBpt = LLPlleFindNext( PlliBpList, hBpt );
    }

    RestoreDebuggerPdTd();

    return BPNOERROR;
}                               /* BPCommit() */

/***    BPUnCommit
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


BPSTATUS
BPUnCommit()
{
    HBPT                hBpt = NULL;
    HBPT                hBpt2 = NULL;
    LPBPP               lpBpp = NULL;

    /*
    **  Get the first item and walk the list of all breakpoints looking
    **  for those marked for change
    */

    hBpt = LLPlleFindNext( PlliBpList, (HBPT) NULL );

    while ( hBpt != NULL ) {
        lpBpp = (LPBPP) LLLpvFromPlle( hBpt );

        if ( lpBpp->fNewMark ) {

            if ( lpBpp->fMarkChange ) {
                //
                //  Remove replacement BP
                //
                LLFDeletePlleFromLl( PlliBpList, lpBpp->ChangeHbpt );
                lpBpp->ChangeHbpt  = NULL;
                lpBpp->fMarkChange = FALSE;
            }

            if (lpBpp->fMarkAdd) {
                assert( llpBpp->fMarkDelete == FALSE );

                hBpt2 = hBpt;
                hBpt = LLPlleFindNext( PlliBpList, hBpt );

                LLFDeletePlleFromLl( PlliBpList, hBpt2 );
                continue;

            } else if (lpBpp->fMarkDelete) {
                /*
                **  We are retracting a deletion of a breakpoint.
                **  Just mark it as no longer being a canidate for
                **  deletion
                */

                lpBpp->fMarkDelete = FALSE;
            }

            //
            //  Fix the enable/disable marks
            //  kkk
            if (lpBpp->bpstate & bpstateEnabled) {
                lpBpp->fMarkEnable  = TRUE;
                lpBpp->fMarkDisable = FALSE;
            } else {
                lpBpp->fMarkEnable  = FALSE;
                lpBpp->fMarkDisable = TRUE;
            }

            lpBpp->fNewMark = FALSE;
        }
        /*
        ** Move to the next breakpoint
        */

        hBpt = LLPlleFindNext( PlliBpList, hBpt );
    }

    return BPNOERROR;
}                               /* BPUnCommit() */

/***    BPSetHpid
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

BPSTATUS
BPSetHpid(
          HBPT hBpt,
          HPID hPid
          )
{
    LPBPP       lpBpp = NULL;

    lpBpp = (LPBPP) LLLpvFromPlle( hBpt );
    lpBpp->hPid = hPid;

    return BPNOERROR;
}                               /* BPSetHpid() */


BPSTATUS
BPGetHpid(
          HBPT hBpt,
          HPID *phPid
          )
/*++

Routine Description:

    Retrieve the HPID associated with a breakpoint.

Arguments:

    hBpt  - Supplies a handle to the breakpoint to be queried
    phPid - Supplies a pointer to a buffer to copy the HPID to

Return Value:

    BPNOERROR

--*/
{
    LPBPP       lpBpp = NULL;

    lpBpp = (LPBPP) LLLpvFromPlle( hBpt );

    //
    //  If the breakpoint is not instantiated, the hPid may not be
    //  correct.
    //
    if ( (lpBpp->bpstate & bpstateSets) == bpstateNotSet ) {
        BPSetPidTid( lpBpp );
    }

    *phPid = lpBpp->hPid;

    return BPNOERROR;
}


BPSTATUS
BPGetIpid(
          HBPT hBpt,
          DWORD *piPid
          )
/*++

Routine Description:

    Retrieve the IPID associated with a breakpoint.

Arguments:

    hBpt  - Supplies a handle to the breakpoint to be queried
    piPid - Supplies a pointer to a buffer to copy the IPID to

Return Value:

    BPNOERROR

--*/
{
    LPBPP       lpBpp = NULL;

    lpBpp = (LPBPP) LLLpvFromPlle( hBpt );
    *piPid = lpBpp->iPid;

    return BPNOERROR;
}


BPSTATUS BPGetHtid(
          HBPT hBpt,
          HTID *phTid
          )
/*++

Routine Description:

    Retrieve the HTID associated with a breakpoint.

Arguments:

    hBpt  - Supplies a handle to the breakpoint to be queried
    phTid - Supplies a pointer to a buffer to copy the HTID to

Return Value:

    BPNOERROR

--*/
{
    LPBPP       lpBpp = NULL;

    lpBpp = (LPBPP) LLLpvFromPlle( hBpt );
    *phTid = lpBpp->fThread ? lpBpp->hTid : 0;

    return BPNOERROR;
}




/***    BPSetTmp
**
**  Synopsis:
**  bpstatus = BPSetTmp(lpaddr, hPid, hTid, phBpt)
**
**  Entry:
**  lpaddr  - Pointer to address structure to set breakpoint at
**  hPid    - Handle to PID to set breakpoint in
**  hTid    - Handlt to TID to set breakpoint in
**  phBpt   - Optional return location for handle to breakpoint set
**
**  Returns:
**  status of the operation.  BPNOERROR if no errors occured
**
**  Description:
**  This routine will create a breakpoint structure and add to the list
**  of all breakpoints.  The type of breakpoint to be created is of
**  class BP_TEMPORARY.  These will normally be cleared by the debugger
**  if another event occurs or the breakpoint is hit.
**
*/

BPSTATUS
BPSetTmp(
         LPADDR lpaddr,
         HPID hPid,
         HTID hTid,
         HBPT * phBpt
         )
{
    HBPT            hBpt = LLPlleCreate(PlliBpList);
    LPBPP           lpBpp = NULL;
    BPSTATUS        Ret = BPNOERROR;

    if (hBpt == NULL) {
        return BPOOMemory;
    }

    *phBpt = hBpt;
    lpBpp = (LPBPP) LLLpvFromPlle(hBpt);

    memset(lpBpp, 0, sizeof(*lpBpp));

    lpBpp->hPid      = hPid;
    lpBpp->hTid      = hTid;
    lpBpp->fTemporary = TRUE;
    lpBpp->addr      = *lpaddr;
    lpBpp->fCodeAddr = TRUE;

    SaveDebuggerPdTd();
    SetDebuggerPdTd( lpBpp );

    BPInitBps( lpBpp, BpsBuffer, TRUE );
    Dbg(OSDBreakpoint( lpBpp->hPid, BpsBuffer ) == xosdNone);
    RestoreDebuggerPdTd();

    lpBpp->bpstate = bpstateEnabled | bpstateSet;
    lpBpp->qwNotify = QwNotification(BpsBuffer)[0];

    LLInsertPlleInLl( PlliBpList, hBpt, 0);

    return Ret;
}                   /* BPSetTmp() */

/***    BPClearAllTmp
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

BPSTATUS
BPClearAllTmp(
              HPID hPid,
              HTID hTid
              )
{
    HBPT    hBpt = NULL;
    HBPT    hBpt2 = NULL;
    LPBPP   lpBpp = NULL;

    hBpt = LLPlleFindNext( PlliBpList, (HBPT) NULL);

    while (hBpt) {
        lpBpp = (LPBPP) LLLpvFromPlle( hBpt );

        if ( (lpBpp->fTemporary) &&
             (lpBpp->hPid == hPid) &&
             ( (hTid == 0) || (lpBpp->hTid == hTid) ) ) {

            BPInitBps( lpBpp, BpsBuffer, FALSE );
            Dbg(OSDBreakpoint( lpBpp->hPid, BpsBuffer ) == xosdNone);
            hBpt2 = hBpt;
            hBpt = LLPlleFindNext( PlliBpList, hBpt );

            LLFDeletePlleFromLl( PlliBpList, hBpt2 );

//          MMFreeHmem( (HDEP) hBpt2 ); // M00BUG -- free handle
        } else {

            hBpt = LLPlleFindNext( PlliBpList, hBpt );
        }
    }

    return BPNOERROR;
}                   /* BPClearAllTmp() */


/***    BPHbptFromI
**
**  Synopsis:
**  bpstatus = BPHbptFromI(phbpt, i)
**
**  Entry:
**  phbpt   - pointer to hbpt to be filled in
**  i   - breakpoint number to look for
**
**  Returns:
**  bpstatus code
**
**  Description:
**  This routine will look for a breakpoint which has the specified
**  breakpoint index number and return the handle to the breakpoint
**  if it is found.  The routine will find the first  breakpoints which
**  has the correct number if multiple exist on the list.
*/

BPSTATUS
BPHbptFromI(
            HBPT * phbpt,
            DWORD i
            )
{
    HBPT    hbpt = NULL;
    LPBPP   lpBpp = NULL;

    /*
    **
    */

    Dbg( BPNextHbpt( &hbpt, bptFirst ) == BPNOERROR );
    if (hbpt != NULL) {
    do {
        lpBpp = (LPBPP) LLLpvFromPlle( hbpt );
        if (lpBpp->iBreakpointNum == i) {
            *phbpt = hbpt;
            return( BPNOERROR );
        }
        Dbg( BPNextHbpt( &hbpt, bptNext ) == BPNOERROR );
    } while (hbpt != NULL);
    }

    return BPNoMatch;
}                                   /* BPHbptFromI() */


/***    BPIFromHbpt
**
**  Synopsis:
**      bpstatus = BPIFromHbpt(lpu, hBpt)
**
**  Entry:
**      lpu - pointer to DWORD to return the index in
**      hBpt - breakpoint handle to get the index of
**
**  Returns:
**      BPNOERROR
**
**  Description:
**      This routine will get the index from a breakpoint handle.
**
*/

BPSTATUS
BPIFromHbpt(
            DWORD * lpu,
            HBPT hBpt
            )
{
    LPBPP   lpBpp = NULL;

    lpBpp = (LPBPP) LLLpvFromPlle( hBpt );
    *lpu = lpBpp->iBreakpointNum;

    return BPNOERROR;
}                                   /* BPIFromHbpt() */

/***    BPHbptFromFileLine
**
**  Synopsis:
**      bpstatus = BPHbptFromFileLine(szFile, iLine, phbpt)
**
**  Entry:
**      szFile  - Source file name to clear breakpoint from
**      iLine   - Line number to clear breakpoint from
**      phbpt   - pointer to where to return found breakpoint handle
**
**  Returns:
**      BPNOERROR if one matching breakpoint was found, BPAmbigous if
**      multiple breakpoints were found and BPNoBreakpoint of no breakpoints
**      were found
**
**  Description:
**      This function will search through the list of breakpoints looking
**      for enabled breakpoints which match the requested file/line pair.
**      We return the handle to the first breakpoint found and continue
**      searching for a second in which case we notify of ambiguity.
**
**      The highlighting information is used to do this search.
*/


BPSTATUS
BPHbptFromFileLine(
                   char * szFile,
                   DWORD iLine,
                   HBPT * phbpt
                   )
{
    HBPT    hbpt = NULL;
    char    rgch[2000] = {0};
    char    rgchSource[2000] = {0};
    LPBPP   lpBpp = NULL;
    char    drive[_MAX_DRIVE] = {0};
    char    dir[_MAX_DIR] = {0};
    char    fname[_MAX_FNAME] = {0};
    char    ext[_MAX_EXT] = {0};

    *phbpt = NULL;

    _splitpath( szFile, drive, dir, fname, ext );
    wsprintf( rgchSource, "%s%s", fname, ext );

    //
    // Walk though the set of breakpoints.  Don't look at anything which
    //  is not enabled. Look for matching file/source
    //
    Dbg( BPNextHbpt( &hbpt, bptFirst) == BPNOERROR );

    while (hbpt != NULL) {
        lpBpp = (LPBPP) LLLpvFromPlle( hbpt );

        if ( (lpBpp->bpstate & bpstateEnabled) && lpBpp->fHighlight ) {

            if (lpBpp->iHighlightLine == iLine) {

                GetAtomName( lpBpp->atmHighlightFile, rgch, sizeof(rgch)-1);

                _splitpath( rgch, drive, dir, fname, ext );
                wsprintf( rgch, "%s%s", fname, ext );

                if ( _stricmp( rgch, rgchSource ) == 0) {
                    if (*phbpt == NULL) {
                        *phbpt = hbpt;
                    } else {
                        return BPAmbigous;
                    }
                }
            }
        }

        Dbg( BPNextHbpt( &hbpt, bptNext) == BPNOERROR );
    }

    return (*phbpt == NULL) ? BPNoBreakpoint : BPNOERROR;
}                                   /* BPHbptFromFileLine() */

/***    BPAddrFromHbpt
**
**  Synopsis:
**      bpstatus = BPAddrFromHbpt(hbpt, lpaddr)
**
**  Entry:
**      hbpt    - handle to breakpoint structure to get address from
**      lpaddr  - pointer to address record to return in
**
**  Returns:
**      BP error code status
**
**  Description:
**      This function will return the address field of a breakpoint.  This
**      is mainly used for functions such as go until command line parsing.
*/

BPSTATUS
BPAddrFromHbpt(
               HBPT hbpt,
               ADDR * lpaddr
               )
{
    LPBPP   lpBpp = (LPBPP) LLLpvFromPlle( hbpt );

    /*
    **  Verify that the handle given us is good
    */

    if (lpBpp == NULL) {
        return BPBadBPHandle;
    }

    /*
    **  breakpoint must be either Virtual or Set in order to have an
    **  address connected with it
    */

    if (lpBpp->bpstate & bpstateNotSet) {
        return BPError;
    }

    /*
    **  Copy over the address field and return
    */

    *lpaddr = lpBpp->addr;

    return BPNOERROR;

}                                   /* BPAddrFromHbpt() */

/***    BPFreeHbpt
**
**  Synopsis:
**      bpstatus = BPFreeHbpt(hbpt)
**
**  Entry:
**      hbpt    - Handle to breakpoint to be freed
**
**  Returns:
**      BP status code
**
**  Description:
**      This function may be used to free a breakpoint which has never been
**      placed onto the list of breakpoints.  Once it is placed on
**      the list of breakpoints you must used BPDelete and BP Commit
**      to remove and delete it.
*/

BPSTATUS
BPFreeHbpt(
           HBPT hbpt
           )
{
    // M00BUG -- verify not on PlliBpList

    MMFreeHmem( (HDEP) hbpt );

    return BPNOERROR;
}                               /* BPFreeHbpt() */


/***    BPHbptFromAddr
**
**  Synopsis:
**      bpstatus = BPHbptFromAddr(lpaddr, lphbpt)
**
**  Entry:
**      lpaddr  - pointer to address to compare against
**      lphbpt  - pointer to where to return the hbpt if found
**
**  Returns:
**      BP Status code -- BPNOERROR if one found, BPAmbigous if multiple
**      are found and BPNoBreakpoint if none are found
**
**  Description:
**      This function will go though the list of set breakpoints and
**      determine if there is a breakpoint at the given address
**
**      This assumes the current global process
*/

BPSTATUS
BPHbptFromAddr(
               ADDR * lpaddr,
               HBPT * lphbpt
               )
{
    HBPT    hbpt = NULL;
    LPBPP   lpBpp = NULL;

    /*
    **  Initialize in case of error
    */

    *lphbpt = NULL;
    SYUnFixupAddr( lpaddr );

    Dbg( BPNextHbpt( &hbpt, bptFirst) == BPNOERROR );

    while (hbpt != NULL) {
        lpBpp = (LPBPP) LLLpvFromPlle( hbpt );

        if (!(lpBpp->bpstate & bpstateDeleted))
           if ((lpBpp->bpstate & (bpstateEnabled | bpstateSet)) == (bpstateEnabled | bpstateSet)) {
               if ( ((lpaddr->mode.fFlat && lpBpp->addr.mode.fFlat) ||
                        (lpaddr->addr.seg == lpBpp->addr.addr.seg)) &&
                   (lpaddr->addr.off == lpBpp->addr.addr.off)) {
                   if (*lphbpt == NULL) {
                       *lphbpt = hbpt;
                   } else {
                       return BPAmbigous;
                   }
               }
           }

        Dbg( BPNextHbpt( &hbpt, bptNext) == BPNOERROR );
    }

    return (*lphbpt == NULL) ? BPNoBreakpoint : BPNOERROR;
}                                   /* BPHbptFromAddr() */


#ifdef ANIMATE_EXPRBP
extern BOOL MoveEditorToAddr(PADDR pAddr);

BOOL
HighlightBP(
            PADDR pAddr,
            BOOL Set
            )
{
    char szFname[_MAX_PATH] = {0};
    int FnameLine = 0;
    int doc = 0;
    LINESTATUSACTION action = {0};

    if ( BPCBGetSourceFromAddr(
            pAddr,
            szFname,
            sizeof(szFname), &FnameLine ) ) {

        if (FindDoc(szFname, &doc, TRUE))
        {
            if (Set) action = LINESTATUS_ON;
            else action = LINESTATUS_OFF;
            LineStatus(doc, FnameLine, BRKPOINT_LINE, action, FALSE, TRUE);
            return TRUE;
        }
    }

    return FALSE;
}

#endif


BPSTATUS
BPCheckHbpt(
    CXF cxf,
    LPFNBPCALLBACK pfn,
    HPID hPid,
    HTID hTid,
    DWORD64 qwNotify
    )
/*++

Routine Description:

    This function will go though the list of set breakpoints and
    determine if the conditions on a breakpoint has been hit.  All
    breakpoints will be evalutated.

Arguments:

    cxf     - context to evalute the breakpoint in

    pfn     - function to call with hBpt of breakpoint hit

    hPid -

    hTid -

    qwNotify -

Return Value:

    BP Status code --
        BPNoBreakpoint - if no breakpoint is set at this address
        BPTmpBreakpoint - if only temp breakpoints are set at this address
        BPPassBreakpoint - if set breakpoint is found but does not meet
                           the breakpoint criteria
        BPNOERROR - if a set breakpoint is found and all breakpoint
                    criteria are met

--*/
{
    HBPT        hBpt = NULL;
    LPBPP       lpBpp = NULL;
    BPSTATUS    bpstatus = BPNoBreakpoint;
    ADDR        addr = {0};
    BPSTATUS    bpstatusT = BPNOERROR;
    BOOL        f;
    DWORD       Msg;



#ifdef ANIMATE_EXPRBP
    static ADDR LastAddr = {0};
    static BOOL F = FALSE;
#endif


    addr = *(SHPAddrFromPCxf(&cxf));
    OSDUnFixupAddr(hPid, NULL, &addr );


#ifdef ANIMATE_EXPRBP
    if ( F ) {
        HighlightBP( &LastAddr, FALSE );
    } else {
        F = TRUE;
    }
    MoveEditorToAddr( &addr );
    HighlightBP( &addr, TRUE );
    LastAddr = addr;
#endif



    Dbg( BPNextHbpt( &hBpt, bptFirst) == BPNOERROR);

    while (hBpt != NULL) {
        lpBpp = (LPBPP) LLLpvFromPlle( hBpt );

        /*
        **  Is the breakpoint set?
        */

        if ((lpBpp->bpstate & bpstateSet) != 0) {

            ADDR    addrBp = lpBpp->addr;

            OSDFixupAddr( lpBpp->hPid, NULL, &addrBp );
            OSDFixupAddr( lpBpp->hPid, NULL, &addr );


            /*
            **  Check to see if we have an address for this breakpoint and
            **  if it is the same.
            */

            if (lpBpp->fCodeAddr &&
                ((emiAddr(addr) != emiAddr(addrBp)) ||
                 (GetAddrSeg(addr) != GetAddrSeg(addrBp)) ||
                 (GetAddrOff(addr) != GetAddrOff(addrBp)))) {

                goto miss;
            }

            //
            //  Check to see if we have a thread for this breakpoint
            //  and if it is the same.
            //
            if ( lpBpp->fThread &&
                 (lpBpp->hTid != LptdCur->htid)
               ) {

                if ( bpstatus == BPNoBreakpoint ) {
                    bpstatus = BPPassBreakpoint;
                }
                goto miss;
            }


            /*
            **  Check for an expression
            */

            if (lpBpp->fExpression) {
                if ((bpstatusT = BPCheckExprLpBpp(lpBpp, cxf, &f)) != BPNOERROR) {
                    if ( lpBpp->fCodeAddr ) {
                        pfn( hBpt, bpstatusT );
                        bpstatus = BPNOERROR;
                    } else {
                        bpstatus = BPPassBreakpoint;
                    }
                    goto miss;
                }
                if (!f) {
                    if (bpstatus == BPNoBreakpoint) {
                        bpstatus = BPPassBreakpoint;
                    }
                    goto miss;
                }
            }

            /*
            **  Check for memory change
            */

            if (lpBpp->fMemory && lpBpp->qwNotify != qwNotify) {
                bpstatus = BPNoBreakpoint;
                goto miss;
            }

#if 0
                if ((bpstatusT = BPCheckMemoryLpBpp(lpBpp, cxf, &f)) != BPNOERROR) {
                    if ( lpBpp->fCodeAddr ) {
                        pfn( hBpt, bpstatusT );
                        bpstatus = BPNOERROR;
                    } else {
                        bpstatus = BPPassBreakpoint;
                    }
                    goto miss;
                }
                if (!f) {
                    if (bpstatus == BPNoBreakpoint) {
                        bpstatus = BPPassBreakpoint;
                    }
                    goto miss;
                }
#endif

            /*
            **  Check for pass counts
            */

            if (lpBpp->iPassCount) {
                lpBpp->iPassCount -= 1;
                if (bpstatus == BPNoBreakpoint) {
                    bpstatus = BPPassBreakpoint;
                }
                goto miss;
            }

            //
            //  Check for message breakpoint
            //
            if ( lpBpp->fMsg ) {

                //
                //  Parse the message if not already parsed.
                //
                if ( !lpBpp->fMsgParsed ) {

                    if (bpstatus == BPNoBreakpoint) {
                        bpstatus = BPPassBreakpoint;
                    }
                    goto miss;
                }

                //
                //  Get the message.
                //
                if ( GetMsg( lpBpp, cxf, &Msg ) ) {

                    //
                    //  Determine if we got a match
                    //
                    if ( lpBpp->fMsgClass ) {
                        if ( !MatchMsgClass( lpBpp, lpBpp->MsgMap, Msg ) ) {
                            if ( bpstatus == BPNoBreakpoint ) {
                                bpstatus = BPPassBreakpoint;
                            }
                            goto miss;
                        }
                    } else {
                        if ( Msg != lpBpp->Msg )  {
                            if ( bpstatus == BPNoBreakpoint ) {
                                bpstatus = BPPassBreakpoint;
                            }
                            goto miss;
                        }
                    }
                } else {
                    if ( bpstatus == BPNoBreakpoint ) {
                        bpstatus = BPPassBreakpoint;
                    }
                    goto miss;
                }
            }


            /*
            **  Check for a temporary breakpoint
            */

            if (lpBpp->fTemporary) {

                //
                //  Process & thread should match, if they don't,
                //  we should skip it.
                //
                if (bpstatus != BPNOERROR) {
                    if ((lpBpp->hPid == hPid) &&
                        (lpBpp->hTid == hTid)) {
                        bpstatus = BPTmpBreakpoint;
                    } else {
                        bpstatus = BPPassBreakpoint;
                    }
                }
                goto miss;
            }


            /*
            **  We managed to get a breakpoint which met
            **  all of the criteria so call the callback function
            **  and alter the return code to say we really hit
            **  a breakpoint
            */

            pfn(hBpt, BPNOERROR);
            bpstatus = BPNOERROR;
        }

     miss:
        Dbg( BPNextHbpt( &hBpt, bptNext) == BPNOERROR );
    }
    return( bpstatus );
}                                   /* BPCheckHbpt() */



BOOL
ParseBpMsg(
           LPBPP       lpBpp,
           LPMESSAGEMAP    MsgMap
           )
{
    BOOL    Ok      = FALSE;
    DWORD   MsgMask = 0;
    char    Buffer[ 2000 ] = {0};
    char   *p = Buffer;

    //
    //  See if the Message atom refers to a message, if it
    //  doesn't then try to form a mask from it.
    //
    if ( !(Ok = FindMsg( lpBpp, MsgMap ) ) ) {

        //
        //  This is not a valid message, assume it is a CV-style
        //  message mask, which consists of characters specifying
        //  message classes:
        //
        //  m   -   mouse
        //  w   -   window
        //  n   -   input
        //  s   -   system
        //  i   -   init
        //  c   -   clipboard
        //  d   -   dde
        //  z   -   nonclient
        //
        GetAtomName( lpBpp->atmMsg, Buffer, sizeof(Buffer)-1);

        Ok = (*p != '\0');
        while ( (*p != '\0') && (*p != ' ') && (*p != '\t') ) {

            switch ( *p++ ) {

            case 'M':
            case 'm':
                MsgMask |= msgMaskMouse;
                break;

            case 'W':
            case 'w':
                MsgMask |= msgMaskWin;
                break;

            case 'N':
            case 'n':
                MsgMask |= msgMaskInput;
                break;

            case 'S':
            case 's':
                MsgMask |= msgMaskSys;
                break;

            case 'I':
            case 'i':
                MsgMask |= msgMaskInput;
                break;

            case 'C':
            case 'c':
                MsgMask |= msgMaskClip;
                break;

            case 'D':
            case 'd':
                MsgMask |= msgMaskDDE;
                break;

            case 'Z':
            case 'z':
                MsgMask |= msgMaskNC;
                break;

            default:
                Ok = FALSE;
                break;
            }
        }

        if ( Ok ) {
            lpBpp->fMsgClass = TRUE;
            lpBpp->Msg       = MsgMask;
        }
    }

    if ( Ok ) {
        lpBpp->fMsgParsed = TRUE;
        lpBpp->MsgMap     = MsgMap;
    }

    return Ok;
}



BOOL
GetMsg(
       LPBPP   lpBpp,
       CXF     cxf,
       DWORD   *Msg
       )
{
    HSYM        hSym = NULL;
    HTM         hTm = NULL;
    HTM         hTmChild = NULL;
    DWORD        End;
    EESTATUS    RetErr;
    UOFF32      Offset = 0;
    char       *Value = NULL;
    ULONG       ul = 0;
    EEHSTR      hValue  = 0;
    BOOL        Ok      = FALSE;
    CXF         Cxf     = cxf;

    if (!ADDR_IS_LI (*SHpADDRFrompCXT (SHpCXTFrompCXF (&Cxf)))) {
        OSDUnFixupAddr (lpBpp->hPid, NULL, SHpADDRFrompCXT (SHpCXTFrompCXF (&Cxf)));
    }

    //
    //  Get TM for the function
    //
    Offset = SHGetNearestHSYM( SHpADDRFrompCXT(SHpCXTFrompCXF(&Cxf)),
                               SHHMODFrompCXT(SHpCXTFrompCXF(&Cxf)),
                               EECODE,
                               &hSym );

    RetErr = EEGetTMFromHSYM( hSym,
                              &Cxf.cxt,
                              &hTm,
                              &End,
                              TRUE,
                              FALSE);

    if ( RetErr == EENOERROR ) {


        RetErr = EEGetChildTM( &hTm, 1, &hTmChild, &ul, FALSE, 10 );

        if ( RetErr == EENOERROR ) {

            RetErr = EEvaluateTM(
                                &hTmChild,
                                SHhFrameFrompCXF(&Cxf),
                                EEVERTICAL
                                );

            if ( RetErr == EENOERROR ) {

                RetErr = EEGetValueFromTM(
                                    &hTmChild,
                                    10,
                                    NULL,
                                    &hValue );

                if ( RetErr == EENOERROR) {

                    Value = (PSTR) MMLpvLockMb( hValue );
                    *Msg = atol( Value );
                    MMbUnlockMb( hValue );
                    Ok = TRUE;
                }
            }
        }

    }

    return Ok;
}


BOOL
FindMsg(
        LPBPP       lpBpp,
        LPMESSAGEMAP    MsgMap
        )
{
    LPMESSAGEINFO   MsgInfo = NULL;
    char        Buffer[ 2000 ] = {0};
    DWORD       i;
    BOOL        Ok = FALSE;

    GetAtomName( lpBpp->atmMsg, Buffer, sizeof(Buffer)-1);

    MsgInfo = MsgMap->lpMsgInfo;
    for (i = 0; i < MsgMap->dwCount; i++) {
        if ( !_stricmp( MsgInfo[i].lszMsgText, Buffer ) ) {
            lpBpp->fMsgParsed  = TRUE;
            lpBpp->Msg         = MsgInfo[i].dwMsg;
            Ok = TRUE;
            break;
        }
    }

    return Ok;
}




BOOL
MatchMsgClass(
              LPBPP           lpBpp,
              LPMESSAGEMAP    MsgMap,
              DWORD           Msg
              )
{
    MESSAGEINFO     Key = {0};
    LPMESSAGEINFO   MsgInfo = NULL;
    BOOL            Ok = FALSE;

    Key.dwMsg = Msg;
    MsgInfo = (LPMESSAGEINFO) bsearch( &Key, MsgMap->lpMsgInfo, MsgMap->dwCount,
                       sizeof( MsgMap->lpMsgInfo[0] ), MsgCmp );

    if ( MsgInfo ) {
        Ok = ( MsgInfo->dwMsgMask & lpBpp->Msg ) ? TRUE : FALSE;
    }

    return Ok;
}


int
WINAPIV
MsgCmp(
    const void *p1,
    const void *p2
    )
{
    LPMESSAGEINFO   MsgInfo1 = (LPMESSAGEINFO)p1;
    LPMESSAGEINFO   MsgInfo2 = (LPMESSAGEINFO)p2;

    if ( MsgInfo1->dwMsg < MsgInfo2->dwMsg ) {
        return -1;
    } else if ( MsgInfo1->dwMsg > MsgInfo2->dwMsg ) {
        return 1;
    } else {
        return 0;
    }
}


int
WINAPIV
MsgCmpName(
    const void *p1,
    const void *p2
    )
{
    LPMESSAGEINFO   MsgInfo1 = (LPMESSAGEINFO)p1;
    LPMESSAGEINFO   MsgInfo2 = (LPMESSAGEINFO)p2;

    return _stricmp( MsgInfo1->lszMsgText, MsgInfo2->lszMsgText );
}

/***    BPQueryCmdOfHbpt
**
**  Synopsis:
**      bpstatus = BPQueryCmdOfHbpt(hBpt, lpb, cb)
**
**  Entry:
**      hBpt    - Handle to breakpoint to query
**      lpb     - pointer to buffer to be filled in
**      cb      - number of characters in the buffer
**
**  Return:
**      break point error code
**
**  Description:
**      This command is used to query out the command string associated with
**      a breakpoint command.
*/


BPSTATUS
BPQueryCmdOfHbpt(
                 HBPT hBpt,
                 char * lpb,
                 DWORD cb
                 )
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );

    if (lpbpp == NULL) {
        return BPBadBPHandle;
    }

    if ( !lpbpp->szCmdLine) {
        *lpb = 0;
        return BPNOERROR;
    }

    strcpy(lpb, lpbpp->szCmdLine);

    return BPNOERROR;
}                               /* BPQueryCmdOfHbpt() */

/***    BPQueryBPTypeOfHbpt
**
**  Synopsis:
**      bpstatus = BPQueryBPTypeOfHbpt( hBpt, lpint );
**
**  Entry:
**      hBpt - handle of breakpoint to get the type of
**      lpint - pointer to location to return the type in
**
**  Return:
**      return bp status code
**
**  Description:
**      This function will return the breakpoint type for the requested
**      breakpoint type.
*/

BPSTATUS
BPQueryBPTypeOfHbpt(
                    HBPT hBpt,
                    int * lpint
                    )
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );
    int     j;

    //
    //  The CodeTypes & WndProcTypes arrays let us determine type
    //  type of a BP fast without the need for a switch statement
    //
    static  int CodeTypes[] = {
            -1,
            BPEXPRCHGD,
            BPEXPRTRUE,
            -1,
            BPLOC,
            BPLOCEXPRCHGD,
            BPLOCEXPRTRUE,
            -1
            };

    static  int WndProcTypes[] = {
            -1,
            -1,
            -1,
            -1,
            BPWNDPROC,
            BPWNDPROCEXPRCHGD,
            BPWNDPROCEXPRTRUE,
            -1
            };

    if (lpbpp == NULL) {
        return BPBadBPHandle;
    }

    //
    //  Form type index
    //
    j = (lpbpp->fCodeAddr*4) + (lpbpp->fExpression*2) + (lpbpp->fMemory);

    //
    //  Get the type from one of the Type arrays using the index
    //  generated.
    //
    if ( lpbpp->fMsg ) {

        //
        //  No indexing needed (only one type for this).
        //
        assert( lpbpp->fWndProc );
        assert( lpbpp->fCodeAddr );
        *lpint = BPWNDPROCMSGRCVD;

    } else if ( lpbpp->fWndProc ) {

        //
        //  Use WndProc array
        //
        assert( lpbpp->fCodeAddr );
        *lpint = WndProcTypes[j];

    } else {

        //
        //  Use the "normal" array
        //
        *lpint = CodeTypes[j];
    }

    assert( *lpint != -1 );

    return BPNOERROR;
}                                   /* BPQueryBPTypeOfHbpt() */

/***    BPQueryLocationOfHbpt
**
**  Synopsis:
**      bpstatus = BPQueryLocationOfHbpt( hBpt, lpb, cb);
**
**  Entry:
**      hBpt    - handle of breakpoint to get the location for
**      lpb     - pointer to location to return the location in
**      cb      - count of bytes for lpb
**
**  Return:
**      return bp status code
**
**  Description:
**      This function will return the location for the requested
**      breakpoint.
*/

BPSTATUS
BPQueryLocationOfHbpt(
                      HBPT hBpt,
                      char * lpb,
                      DWORD cb
                      )
{
    char    rgch[2000] = {0};
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );

    *lpb = 0;

    if (lpbpp == NULL) {
        return BPBadBPHandle;
    }

    if (!lpbpp->fCodeAddr) {
        return BPNOERROR;
    }

    /*
    **  Put in the default context if it exists
    */

    if ((lpbpp->atmCxtDef) != 0 && (lpbpp->szAddrExpr != NULL)) {
        GetAtomName( lpbpp->atmCxtDef, rgch, sizeof(rgch) );
        if (strlen( rgch ) > cb) {
            return BPError;
        }
        strcat(lpb, rgch);
        cb -= strlen(rgch);
    }

    /*
    **  Get the real context if specified
    */

    if (lpbpp->atmAddrCxt != 0) {
        assert( lpbpp->szAddrExpr != NULL);

        GetAtomName(lpbpp->atmAddrCxt, rgch, sizeof(rgch) );
        if (strlen(rgch) > cb) {
            return BPError;
        }

        strcat(lpb, rgch);
        cb -= strlen(rgch);
    }

    /*
    **  Get the full expression if specified
    */

    if (lpbpp->szAddrExpr != NULL) {
        strcpy( rgch, lpbpp->szAddrExpr );
        if (strlen(rgch) > cb) {
            return BPError;
        }

        strcat(lpb, rgch);
        cb -= strlen(rgch);
    }

    return BPNOERROR;
}                                   /* BPQueryLocationOfHbpt() */


/***    BPQueryExprOfHbpt
**
**  Synopsis:
**      bpstatus = BPQueryExprOfHbpt( hBpt, lpb, cb);
**
**  Entry:
**      hBpt    - handle of breakpoint to get the expresion for
**      lpb     - pointer to location to return the expresion in
**      cb      - count of bytes for lpb
**
**  Return:
**      return bp status code
**
**  Description:
**      This function will return the expresion for the requested
**      breakpoint.
*/

BPSTATUS
BPQueryExprOfHbpt(
                  HBPT hBpt,
                  char * lpb,
                  DWORD cb
                  )
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );

    if (lpbpp == NULL) {
        return BPBadBPHandle;
    }

    if (lpbpp->fExpression == FALSE) {
        return BPNOERROR;
    }

    assert(lpbpp->atmExpr != 0);

    GetAtomName(lpbpp->atmExpr, lpb, cb);

    return BPNOERROR;
}                                   /* BPQueryExprOfHbpt() */


/***    BPQueryMessageOfHbpt
**
**  Synopsis:
**      bpstatus = BPQueryMessageOfHbpt( hBpt, lpb, cb);
**
**  Entry:
**      hBpt    - handle of breakpoint to get the message for
**      lpb     - pointer to location to return the message in
**      cb      - count of bytes for lpb
**
**  Return:
**      return bp status code
**
**  Description:
**      This function will return the message for the requested
**      message breakpoint.
*/

BPSTATUS
BPQueryMessageOfHbpt(
                     HBPT hBpt,
                     char * lpb,
                     DWORD cb
                     )
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );

    if (lpbpp == NULL) {
        return BPBadBPHandle;
    }

    if ( !lpbpp->fMsg ) {
        return BPNOERROR;
    }

    assert(lpbpp->atmMsg != 0);

    GetAtomName(lpbpp->atmMsg, lpb, cb);

    return BPNOERROR;
}


/***    BPQueryMemoryOfHbpt
**
**  Synopsis:
**      bpstatus = BPQueryMemoryOfHbpt( hBpt, lpb, cb);
**
**  Entry:
**      hBpt    - handle of breakpoint to get the memory for
**      lpb     - pointer to location to return the memory in
**      cb      - count of bytes for lpb
**
**  Return:
**      return bp status code
**
**  Description:
**      This function will return the memory expression for the requested
**      breakpoint.
*/

BPSTATUS
BPQueryMemoryOfHbpt(
                    HBPT hBpt,
                    char * lpb,
                    DWORD cb
                    )
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );

    if (lpbpp == NULL) {
        return BPBadBPHandle;
    }

    if (lpbpp->fMemory == FALSE) {
        return BPNOERROR;
    }

    assert(lpbpp->atmData != 0);

    GetAtomName(lpbpp->atmData, lpb, cb);

    return BPNOERROR;
}                                   /* BPQueryMemoryOfHbpt() */


/***    BPQueryMemorySizeOfHbpt
**
**  Synopsis:
**      bpstatus = BPQueryMemorySizeOfHbpt( hBpt, lpb, cb);
**
**  Entry:
**      hBpt    - handle of breakpoint to get the memory cmp size for
**      lpb     - pointer to location to return the memory cmp size in
**      cb      - count of bytes for lpb
**
**  Return:
**      return bp status code
**
**  Description:
**      This function will return the memory compare size for the requested
**      breakpoint.
*/

BPSTATUS
BPQueryMemorySizeOfHbpt(
                        HBPT hBpt,
                        char * lpb,
                        DWORD cb
                        )
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );
    char    rgchT[2000] = {0};
    BPSTATUS    bpstatus = BPNOERROR;

    if (lpbpp == NULL) {
        return BPBadBPHandle;
    }

    if (lpbpp->fMemory == FALSE) {
        return BPNOERROR;
    }

    if ( (lpbpp->cbDataSize == 0) && (lpbpp->atmRangeSize != 0) ) {
        GetAtomName(lpbpp->atmRangeSize, lpb, cb);
    } else {
        sprintf(rgchT, "%d", lpbpp->cbDataSize);
        if (strlen(rgchT) >= cb) {
            bpstatus = BPOutOfSpace;
        } else {
            strcpy(lpb, rgchT);
        }
    }

    return bpstatus;
}                                   /* BPQueryMemorySizeOfHbpt() */


/***    BPQueryPassCntOfHbpt
**
**  Synopsis:
**      bpstatus = BPQueryPassCntOfHbpt( hBpt, lpb, cb);
**
**  Entry:
**      hBpt    - handle of breakpoint to get the pass count for
**      lpb     - pointer to location to return the pass count in
**      cb      - count of bytes for lpb
**
**  Return:
**      return bp status code
**
**  Description:
**      This function will return the pass count for the requested
**      breakpoint.
*/

BPSTATUS
BPQueryPassCntOfHbpt(
                     HBPT hBpt,
                     char * lpb,
                     DWORD cb
                     )
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );
    char    rgch[2000] = {0};

    /*
    **  Initialize to empty return string
    */

    *lpb = 0;

    if (lpbpp == NULL) {
        return BPBadBPHandle;
    }

    /*
    **  Check for a non-evaluated pass count expression
    */

    if (lpbpp->atmPassCount != 0) {
        assert( lpbpp->macPassCount == 0);

        GetAtomName(lpbpp->atmPassCount, rgch, sizeof(rgch));
        if (strlen(rgch) > cb) {
            return BPError;
        }

        strcpy(lpb, rgch);
    }

    /*
    **  Now check for an evaluated pass count
    */

    if (lpbpp->macPassCount != 0) {
        sprintf( rgch, "%d-%d", lpbpp->macPassCount, lpbpp->macPassCount - lpbpp->iPassCount);
        if (strlen(rgch) > cb) {
            return BPError;
        }

        strcpy(lpb, rgch);
    }

    return BPNOERROR;
}                                   /* BPQueryPassCntOfHbpt() */

/***    BPQueryPassleftOfHbpt
**
**  Synopsis:
**      bpstatus = BPQueryPassleftOfHbpt( hBpt, lpb, cb);
**
**  Entry:
**      hBpt    - handle of breakpoint to get the pass count for
**      lpb     - pointer to location to return the pass count in
**      cb      - count of bytes for lpb
**
**  Return:
**      return bp status code
**
**  Description:
**      This function will return the pass count left for the requested
**      breakpoint.
*/

BPSTATUS
BPQueryPassLeftOfHbpt(
                      HBPT hBpt,
                      char * lpb,
                      DWORD cb
                      )
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );
    char    rgch[2000] = {0};

    //
    //  Initialize to empty return string
    //
    *lpb = 0;

    if (lpbpp == NULL) {
        return BPBadBPHandle;
    }

    if ( lpbpp->atmPassCount != 0 ) {
        sprintf( rgch, "%d", lpbpp->iPassCount );

        if ( strlen(rgch) > cb ) {
            return BPError;
        }
        strcpy(lpb, rgch);
    }

    return BPNOERROR;
}



/***    BPQueryThreadOfHbpt
**
**  Synopsis:
**      bpstatus = BPQueryThreadOfHbpt( hBpt, lpb, cb);
**
**  Entry:
**      hBpt    - handle of breakpoint to get the thread for
**      lpb     - pointer to location to return the thread in
**      cb      - count of bytes for lpb
**
**  Return:
**      return bp status code
**
**  Description:
**      This function will return the threadt for the requested
**      breakpoint.
*/

BPSTATUS
BPQueryThreadOfHbpt(
                    HBPT hBpt,
                    char * lpb,
                    DWORD cb
                    )
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );
    char    rgch[2000] = {0};

    /*
    **  Initialize to empty return string
    */

    *lpb = 0;

    if (lpbpp == NULL) {
        return BPBadBPHandle;
    }

    if (lpbpp->fThread ) {
        sprintf( rgch, "%d", lpbpp->iTid );
        if (strlen(rgch) > cb) {
            return BPError;
        }

        strcpy(lpb, rgch);
    }


    return BPNOERROR;
}

BPSTATUS
BPQueryProcessOfHbpt(
                     HBPT hBpt,
                     char * lpb,
                     DWORD cb
                     )
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );
    char    rgch[2000] = {0};
    LPPD    lppd = NULL;
    BOOL    Ok = FALSE;

    /*
    **  Initialize to empty return string
    */

    *lpb = 0;

    if (lpbpp == NULL) {
        return BPBadBPHandle;
    }

    if (lpbpp->fProcess ) {

        sprintf( rgch, "%d", lpbpp->iPid );
        Ok = TRUE;

    } else if ( (lpbpp->bpstate & bpstateSets) != bpstateNotSet ) {

        //
        //  Get the process ID for the process
        //
        lppd = LppdOfHpid(lpbpp->hPid);
        if (lppd) {
            sprintf( rgch, "%d", lppd->ipid );
            Ok = TRUE;
        }
    }

    if ( Ok ) {
        if (strlen(rgch) > cb) {
            return BPError;
        }
        strcpy(lpb, rgch);
    }

    return BPNOERROR;
}

BPSTATUS
BPQueryHighlightLineOfHbpt(
                           HBPT hBpt,
                           DWORD *Line
                           )
{
    LPBPP       lpbpp = (LPBPP) LLLpvFromPlle( hBpt );
    BPSTATUS    Ret   = BPNOERROR;

    if ( lpbpp->fHighlight ) {
        *Line = lpbpp->iHighlightLine;
    } else {
        Ret = BPError;
    }

    return Ret;
}

/***    BpGetHtm
**
**  Synopsis:
**      bpstatus = BpGetHtm(lpBpp, szBpt, fExpr)
**
**  Entry:
**      lpBpp   - pointer to breakpoint structure to fill in
**      szBpt   - pointer to string containning the expression
**      fExpr   - TRUE if expr, FALSE if address
**
**  Return:
**      breakpoint status code
**
**  Description:
**      This function is used to get the expression portion of a breakpoint
**      expression and get the parse tree for the expression.  It will be
**      saved away in the breakpoint structure for later use.
*/

LOCAL
BPSTATUS
BpGetHtm(
         LPBPP lpBpp,
         char * szBpt,
         BOOL fExpr
         )
{
    DWORD       strIndex = 0;
    EESTATUS    Err;

    /*
    **  Check to see if the expression is enclosed in quotes, if so then
    **  strip them off.
    */

    if (*szBpt == '\"') {
        szBpt ++;
        if (szBpt[strlen(szBpt)-1] != '\"') {
        } else {
            szBpt[strlen(szBpt)-1] = 0;
        }
    }

    if (fExpr) {
        lpBpp->atmExpr = AddAtom(szBpt);
        lpBpp->fExpression = TRUE;
    } else {
        lpBpp->atmData = AddAtom(szBpt);
        lpBpp->fMemory = TRUE;
    }

    /*
    **  Attemp to parse it and check for an error return code
    */

    if ((Err = EEParse(szBpt, radix, fCaseSensitive, &lpBpp->hTm, &strIndex)) != EENOERROR) {
        return BPBadDataSize;
    }

    return BPNOERROR;
}                                       /* BpGetHtm() */

/***    BPCheckExprLpBpp
**
**  Synopsis:
**      bpstatus = BPCheckExprLpBpp(lpBpp)
**
**  Entry:
**      lpBpp   - pointer to breakpoint structure
**
**  Returns:
**      BP Status code
**
**  Description:
**      This function will evaluate the expression assocated with
**      the passed in breakpoint.
*/

BPSTATUS
BPCheckExprLpBpp(
                 LPBPP lpBpp,
                 CXF cxf,
                 BOOL * pf
                 )
{
    RTMI    ri = {0};
    HTI     hTi = NULL;
    PTI     pTi = NULL;
    DWORD   i;
    int     fStop = 0;

    /*
    **  get the TM for this expression
    */

    memset( &ri, 0, sizeof(ri) );
    ri.fValue = TRUE;
    ri.fSzBytes = TRUE;

    /*
    **  Evaluate the expression
    */

    if ( EEBindTM( &lpBpp->hTm, SHpCXTFrompCXF(&cxf), TRUE, TRUE) ||
         EEvaluateTM( &lpBpp->hTm, SHhFrameFrompCXF(&cxf), EEHORIZONTAL) ||
         EEInfoFromTM( &lpBpp->hTm, &ri, &hTi) ) {

        return BPBadExpression;
    }

    /*
    **  Or this thing together, if all zeros, then we will make a FALSE
    **  if any bits set if won't be a false.
    */

    pTi = (PTI) MMLpvLockMb ( hTi );

    if (pTi->fResponse.fValue &&
        pTi->fResponse.fSzBytes) {

        for ( i=0 ; i<pTi->cbValue ; i++) {
            fStop |= pTi->Value[i];
        }
    }

    MMbUnlockMb( hTi );
    EEFreeTI( &hTi );

    /*
    **  If there is no change or if the expression evaluator fails, don't stop
    */

    *pf = fStop;
    return BPNOERROR;
}                                   /* BPCheckExprLpBpp() */


BOOL
BPIsMarkedForDeletion(
                      IN HBPT hBpt
                      )
/*++

Routine Description:

    Determines if a breakpoint is flagged for deletion.

Arguments:

    hBpt    -   Supplies breakpoint handle

Return Value:

    BOOL    -   TRUE if the breakpoint is marked for deletion,
                FALSE otherwise.


--*/
{
    LPBPP       lpbpp = (LPBPP) LLLpvFromPlle( hBpt );
    BOOL        fMarked;

    fMarked = lpbpp->fMarkDelete;
    return fMarked;
}


BOOL
BPIsDisabled(
             IN HBPT hBpt
             )
/*++

Routine Description:

    Determines if a breakpoint is disabled

Arguments:

    hBpt    -   Supplies breakpoint handle

Return Value:

    BOOL    -   TRUE if the breakpoint is disabled
                FALSE otherwise.


--*/
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );
    BOOL    f;

    f = ((lpbpp->bpstate & bpstateSets) == bpstateSet);

    if (lpbpp->bpstate & bpstateEnabled) {
        f = lpbpp->fMarkDisable;
    } else {
        f = !lpbpp->fMarkEnable;
    }

    return f;
}

BOOL
BPIsInstantiated(
                 IN HBPT hBpt
                 )
/*++

Routine Description:

    Determines if a breakpoint is instantiated

Arguments:

    hBpt    -   Supplies breakpoint handle

Return Value:

    BOOL    -   TRUE if the breakpoint is instantiated
                FALSE otherwise.


--*/
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );
    BOOL        f;

    f = ((lpbpp->bpstate & bpstateSets) == bpstateSet);
    return f;
}


BOOL
BPUninstantiate(
                IN HBPT hBpt
                )
/*++

Routine Description:

    Uninstantiates a breakpoint

Arguments:

    hBpt    -   Supplies breakpoint handle

Return Value:

    BOOL    -   TRUE if the breakpoint is uninstantiated
                FALSE otherwise.


--*/
{
    LPBPP       lpBpp = (LPBPP) LLLpvFromPlle( hBpt );
    BOOL        f;

    if ((lpBpp->bpstate & bpstateSets) == bpstateSet) {

        BPRemoveHighLight( lpBpp );

        //
        //  Mark the breakpoint as uninstantiated.
        //
        lpBpp->bpstate &= ~bpstateSets;
        lpBpp->bpstate |= bpstateNotSet;
        lpBpp->fMarkAdd = TRUE;

        BPAddHighLight( lpBpp );

        //
        //  Remove physical breakpoint
        //
        BPInitBps( lpBpp, BpsBuffer, FALSE );
        // BUGBUG -kcarlos - The DBG is firing when the app exits, but Kentf
        //  believes that the assertion is incorrect.
        // Dbg(OSDBreakpoint( lpBpp->hPid, BpsBuffer ) == xosdNone);
        OSDBreakpoint( lpBpp->hPid, BpsBuffer );

        if ( lpBpp->fMemory ) {
            MemoryBpCount--;
#if 0
            if (lpBpp->Mem != NULL ) {
                free( lpBpp->Mem );
                lpBpp->Mem = NULL;
            }
#endif
        }

        f = TRUE;

    } else {

        lpBpp->bpstate &= ~bpstateSets;
        lpBpp->bpstate |= bpstateNotSet;
        f = FALSE;
    }

    return f;
}




BOOL
BPIsQuiet(
          IN HBPT hBpt
          )
/*++

Routine Description:

    Determines if a breakpoint is quiet

Arguments:

    hBpt    -   Supplies breakpoint handle

Return Value:

    BOOL    -   TRUE if the breakpoint is quiet
                FALSE otherwise.


--*/
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );
    BOOL        f;

    f = (BOOL)(lpbpp->fQuiet);
    return f;
}

VOID
BPSetQuiet(
           IN HBPT hBpt
           )
/*++

Routine Description:

    Sets the breakpoint to quiet

Arguments:

    hBpt    -   Supplies breakpoint handle

Return Value:

    None


--*/
{
    LPBPP   lpbpp = (LPBPP) LLLpvFromPlle( hBpt );

    lpbpp->fQuiet = TRUE;
}




PTSTR
BPShortenContext(
    IN  const TCHAR *pszSrc,
    OUT TCHAR *pszDst
    )
/*++

Routine Description:

    Formats a breakpoint context. The formatting consists
    of reducing the file specifications from full paths to
    the mere file names.

Arguments:

    pszSrc   -   Supplies the original context.
    pszDst   -   Supplies the buffer where the formatted context is placed.

Return Value:

    char * - Pointer to szDst if success. NULL if failure.


--*/

{
    TCHAR szSrcDup[_MAX_PATH] = {0};
    TCHAR szTmpFileExt[_MAX_EXT] = {0};
    TCHAR szModuleName[_MAX_FNAME] = {0};
    TCHAR szExeName[_MAX_FNAME] = {0};
    PTSTR pszCur = NULL;
    
    Assert(pszSrc);
    Assert(pszDst);
 
    if (!*pszSrc) {
        return NULL;
    }

    //
    // Copy the context starting from the end of the string since
    // we may have strings like {int foo::foo(char*, int, int), f:\drive\...\foomain.cpp, k:\\bin\\foo.exe}
    //

    // Make a copy so that we can mangle the copy
    _tcscpy(szSrcDup, pszSrc);

    // Duplicate previous functionality
    _tcscpy(pszDst, pszSrc);

    //
    // Start
    //

    // Point to the last char
    pszCur = szSrcDup + _tcslen(szSrcDup) -1;

    // Make sure that it is properly begun and terminated
    if (OPENCONTEXTOP != *szSrcDup || CLOSECONTEXTOP != *pszCur) {
        return NULL;
    }

    //
    // Get the EXE name
    //

    // Remove the last '}'
    *pszCur-- = 0;

    pszCur = _tcsrchr(szSrcDup, _T(',') );
    if (!pszCur) {

        // No commas?!?!?!
        // Assume whatever is in there, that it is a
        // filename. The old code did...
        
        _tsplitpath(pszCur, NULL, NULL, szExeName, szTmpFileExt);
        
        _tcscpy(pszDst, _T("{") );
        _tcscat(pszDst, szExeName);
        _tcscat(pszDst, szTmpFileExt);
        _tcscat(pszDst, _T("}") );
        
        return pszDst;
    }

    *pszCur++ = 0;
    _tsplitpath(pszCur, NULL, NULL, szExeName, szTmpFileExt);
    _tcscat(szExeName, szTmpFileExt);

    
    //
    // Get the module name
    //
    pszCur = _tcsrchr(szSrcDup, _T(',') );
    if (!pszCur) {
        // syntax error
        return NULL;
    }

    *pszCur++ = 0;
    _tsplitpath(pszCur, NULL, NULL, szModuleName, szTmpFileExt);
    _tcscat(szModuleName, szTmpFileExt);


    //
    // Build the short context
    //
    _tcscpy(pszDst, _T("{,") );
    _tcscat(pszDst, szModuleName);
    _tcscat(pszDst, ",");
    _tcscat(pszDst, szExeName);
    _tcscat(pszDst, _T("}") );

    return pszDst;
}



static
void
BPBoundFixContext(
    IN OUT  LPBPP pBpp
    )

/*++

Routine Description:

    Edits the contexts in a breakpoint to include the breakpoint
    bindings.

    We will be editing out the function name since this is causing us
    problems with C++ and should not be a problem until we get into
    the nested languages issues later. (i.e. PASCAL)

Arguments:

    pBpp        -   Supplies pointer to the breakpoint.

Return Value:

    none

--*/

{

    CXT     Cxt = {0};
    char *  p = NULL;
    char *  q = NULL;
    EEHSTR  eehstr;
    INT     Line;
    char    rgch[2000] = {0};
    char    File[2000] = {0};

    SaveDebuggerPdTd();
    SetDebuggerPdTd( pBpp );

    memset( &Cxt, 0, sizeof(Cxt ));
    SHSetCxt( &(pBpp->addr), &Cxt );

    Dbg( EEFormatCXTFromPCXT(&Cxt, &eehstr, HCXTFMT_No_Procedure) == EENOERROR);

    *rgch = '\0';

    if ( eehstr ) {

        p = (PSTR) MMLpvLockMb( eehstr );

        if ( *p ) {

            DAssert(*p == OPENCONTEXTOP);

            q = rgch;
            *q++ = *p++;

            while (*p != ',') {     /* Skip over the function name */
                p++;
            }
            strcpy(q, p);
        }

        MMbUnlockMb( eehstr );
        EEFreeStr( eehstr );

        if ( *rgch ) {
            if ( pBpp->atmAddrCxt ) {

                DeleteAtom(pBpp->atmAddrCxt);
                pBpp->atmAddrCxt = AddAtom(rgch);

            } else {

                if ( pBpp->atmCxtDef ) {
                    DeleteAtom(pBpp->atmCxtDef);
                }
                pBpp->atmCxtDef = AddAtom(rgch);
            }
        }
    }

    //
    //  Now set the highlight
    //
    *File = '\0';

    if ( BPCBGetSourceFromAddr( &(pBpp->addr),
                                File,
                                sizeof(File),
                                &Line ) ) {

        if ( pBpp->atmHighlightFile ) {
            DeleteAtom( pBpp->atmHighlightFile );
        }

        //
        //  Set the file to highlight
        //
        p = rgch;
        if ( p = (PSTR) strchr( (LPBYTE) p,',' ) ) {
            p++;
            q = (PSTR) strchr( (LPBYTE) p,',' );
            *q = '\0';

            pBpp->atmHighlightFile = AddAtom( p );
            pBpp->iHighlightLine   = Line;
            pBpp->fHighlight       = TRUE;
        }
    }

    RestoreDebuggerPdTd();
}


VOID
BPInitBps(
    LPBPP   lpBpp,
    LPBPS   Bps,
    BOOL    Set
    )
/*++

Routine Description:

    Set up the BPS structure to send to OSDebug.  This only handles one
    breakpoint, and does not currently support batches of breakpoints.

Arguments:

    lpBpp   - Supplies the breakpoint structure used by the BP engine to
            describe the breakpoint.  This will be translated here into a
            BPIS struct for OSDBreakpoint.

    Bps     - Returns a BPS structure containing the BPIS structure which
            will be prepared here.

    Set     - Supplies TRUE if the BP is to be set, FALSE if it is to
            be cleared.

Return Value:

    None

--*/
{
    LPBPIS Bpis;

    assert( lpBpp );
    assert( Bps   );

    ZeroMemory(_BpsBuffer,sizeof(_BpsBuffer)); //v-vadimp - BUGBUG what we really need to do is clear up the BPS structure

    Bpis = RgBpis(Bps);

    Bps->cbpis      = 1;
    Bps->fSet       = Set;

    Bpis->htid       = lpBpp->hTid;
    Bpis->fOneThd    = lpBpp->hTid ? TRUE : FALSE;

    QwNotification(Bps)[0] = lpBpp->qwNotify;

    //
    //  Note that we treat message breakpoints as bptpExec and let
    //  the shell do all the work.
    //
    if ( lpBpp->fCodeAddr ) {

        OSDFixupAddr(lpBpp->hPid, NULL, &(lpBpp->addr));

        if ( lpBpp->fMemory || lpBpp->fExpression ) {
            Bpis->bpns  = bpnsCheck;
        } else {
            Bpis->bpns  = bpnsStop;
        }
        Bpis->bptp      = bptpExec;
        Bpis->exec.addr = lpBpp->addr;

        OSDUnFixupAddr(lpBpp->hPid, NULL, &(lpBpp->addr));

    } else if ( lpBpp->fMemory ) {

        OSDFixupAddr(lpBpp->hPid, NULL, &(lpBpp->AddrMem));

        Bpis->bpns = bpnsStop;

        if (lpBpp->BpType & BP_EXECUTE) {
            Bpis->bptp      = bptpDataExec;
        } else if (lpBpp->BpType & BP_DATA_CHANGED) {
            Bpis->bptp      = bptpDataC;
        } else if (lpBpp->BpType & BP_DATA_WRITE) {
            Bpis->bptp      = bptpDataW;
        } else if (lpBpp->BpType & BP_DATA_READ) {
            Bpis->bptp      = bptpDataR;
        }
        Bpis->data.addr = lpBpp->AddrMem;
        Bpis->data.cb   = lpBpp->cbDataSize;

        OSDUnFixupAddr(lpBpp->hPid, NULL, &(lpBpp->AddrMem));

    } else if ( lpBpp->fExpression) {

        //
        // expression BP with no code or data address:
        // this is a walk BP.
        //

        memset(&Bpis->data.addr, 0, sizeof(ADDR));
        Bpis->bpns       = bpnsCheck;
        Bpis->data.cb    = 0;
        Bpis->bptp       = bptpDataR;

    }
}


VOID
SaveDebuggerPdTd()
{
    LppdOld = LppdCur;
    LptdOld = LptdCur;
}

VOID
RestoreDebuggerPdTd()
{
    LppdCur = LppdOld;
    LptdCur = LptdOld;
    if ( LppdCur && LptdCur ) {
        SetPidTid_StatusBar(LppdCur, LptdCur);
        UpdateDebuggerState(UPDATE_CONTEXT);
    }
}

VOID
SetDebuggerPdTd(
                LPBPP lpBpp
                )
{
    LPPD    lppd;

    lppd = LppdOfHpid(lpBpp->hPid);
    if (lppd) {
        LppdCur = lppd;
        LptdCur = LppdCur->lptdList;
        if ( LppdCur && LptdCur ) {
            SetPidTid_StatusBar(LppdCur, LptdCur);
            UpdateDebuggerState(UPDATE_CONTEXT);
        }
    }
}


VOID
BPReplace(
          LPBPP lpBpp
          )
{
    BPP     Bpp = {0};
    LPBPP   lpBppNew = NULL;

    if ( lpBpp->fMarkChange && lpBpp->ChangeHbpt != NULL ) {

        //
        //  If original is set, remove it
        //
        if ( !lpBpp->fMarkAdd && (lpBpp->bpstate & bpstateSet) ) {

            //
            //  Remove breakpoint
            //
            if (lpBpp->bpstate == (bpstateSet | bpstateEnabled)) {

                BPInitBps( lpBpp, BpsBuffer, FALSE );
                Dbg(OSDBreakpoint( lpBpp->hPid, BpsBuffer ) == xosdNone);
                if ( lpBpp->fMemory ) {
                    MemoryBpCount--;
                }
            }

            BPRemoveHighLight( lpBpp );
        }

        //
        //   Save original BP
        //
        Bpp = *lpBpp;

        //
        //   Get New one
        //
        lpBppNew = (LPBPP) LLLpvFromPlle( lpBpp->ChangeHbpt );

        //
        //  Copy new one over current one
        //
        *lpBpp = *lpBppNew;

        //
        //  Keep change flags
        //
        lpBpp->fMarkAdd     =   TRUE;
        lpBpp->fMarkDisable =   Bpp.fMarkDisable;
        lpBpp->fMarkEnable  =   Bpp.fMarkEnable;
        lpBpp->fReplacement =   FALSE;

        //
        //  Deallocate stuff
        //
        LLFDeletePlleFromLl( PlliBpList, Bpp.ChangeHbpt );

        BPKillNode(&Bpp);
    }
}


HMOD
BPFileNameToMod(
                char * FileName
                )
{
    HEXE    hexe = (HEXE)NULL;
    HMOD    hmod = NULL;


    while ( hexe = SHGetNextExe( hexe ) ) {

        hmod = (HMOD)NULL;

        while ( hmod = SHGetNextMod( hexe, hmod ) ) {

            if ( SLHsfFromFile( hmod, FileName ) ) {
                return hmod;
            }
        }
    }

    return (HMOD)NULL;
}


/***    BPUpdateMemory
**
**  Synopsis:
**      bpstatus = BPUpdateMemory( size )
**
**  Entry:
**      size    - size of memory changed
**
**  Returns:
**      BPSTATUS
**
**  Description:
**
*/

BPSTATUS
BPUpdateMemory(
               ULONG Size
               )
{
#if 1
    return BPNOERROR;
#else
    BPSTATUS    Ret = BPNOERROR;
    ADDR        AddrCur = {0};
    ADDR        AddrBp = {0};
    HBPT        hBpt = NULL;
    LPBPP       lpBpp= NULL;

    if ( MemoryBpCount > 0 ) {

        if ( LppdCur && LptdCur && LptdCur->tstate != tsRunning ) {

            //
            //  Get the address of the memory that was changed. Note that this
            //  assumes that nobody has changed the adrCurrent since the
            //  memory was changed.
            //
            OSDGetAddr( LppdCur->hpid, LptdCur->htid, adrCurrent, &AddrCur );

            //
            //  Loop thru all the breakpoints looking for instantiated Memory
            //  breakpoints.
            //
            Dbg( BPNextHbpt( &hBpt, bptFirst) == BPNOERROR);
            while (hBpt != NULL) {

                lpBpp = LLLpvFromPlle( hBpt );

                if ( (lpBpp->bpstate & bpstateSet) != 0 &&
                     lpBpp->fMemory // &&
                     //lpBpp->Mem ) if 0
                     {

                    //
                    //  If the breakpoint uses the memory that was changed,
                    //  update its buffer.
                    //
                    AddrBp = lpBpp->AddrMem;
                    OSDFixupAddr( lpBpp->hPid, NULL, &AddrBp );

                    if ( GetAddrSeg( AddrCur ) == GetAddrSeg( AddrBp ) &&
                         GetAddrOff( AddrCur ) >= GetAddrOff( AddrBp ) &&
                         (GetAddrOff( AddrCur ) + Size ) <=
                                 (GetAddrOff( AddrBp ) + lpBpp->cbDataSize) ) {

                        if ( OSDSetAddr( lpBpp->hPid,
                                         lpBpp->hTid,
                                         adrCurrent,
                                         &AddrBp) == xosdNone ) {
                            Dbg(OSDPtrace( osdReadBuf,
                                           lpBpp->cbDataSize,
                                           lpBpp->Mem,
                                           lpBpp->hPid,
                                           lpBpp->hTid) ==
                                 (XOSD)lpBpp->cbDataSize);
                        }
                    }
                }

                Dbg( BPNextHbpt( &hBpt, bptNext) == BPNOERROR );
            }
        }
    }

    return Ret;
#endif
}


VOID
BPSegLoad(
          ULONG Selector
          )
{
    HBPT        hBpt = NULL;
    LPBPP       lpBpp = NULL;
    int         Count;

    Dbg(BPNextHbpt( &hBpt, bptNext) == BPNOERROR);

    Count = 0;
    while (hBpt != NULL) {

        lpBpp = (LPBPP) LLLpvFromPlle( hBpt );

        if (lpBpp->bpstate == (bpstateVirtual | bpstateEnabled)) {
            lpBpp->fMarkAdd = TRUE;
            Count++;
        }

        Dbg(BPNextHbpt( &hBpt, bptNext) == BPNOERROR);
    }

    if ( Count > 0 ) {
        BPCommit();
    }
}

BOOL
BPSymbolLoading(
                BOOL    Enabled
                )
{
    BOOL f = SymbolLoadingEnabled;
    SymbolLoadingEnabled = Enabled;
    return f;
}

BOOL
BPSymbolsMayBeAvailable (
                         HBPT    hBpt
                         )
/*++

Routine Description:

    Returns TRUE if there is any possibility that we might be able to
    bind this BP with the currently loaded symbols.

Arguments:

    hBpt    -   Supplies breakpoint handle

Return Value:

    None


--*/
{
    LPBPP       lpBpp = (LPBPP) LLLpvFromPlle( hBpt );
    BOOL        Ret   = TRUE;
    char        rgch[ 2000 ] = {0};
    char       *p = NULL;
    char       *Module = NULL;
    HEXE        Hexe = NULL;
    LPDEBUGDATA DebugData = NULL;


    if ( lpBpp ) {

        if ( lpBpp->atmCxtDef != 0 || lpBpp->atmAddrCxt != 0 ) {

            rgch[0] = 0;

            if ( lpBpp->atmAddrCxt != 0 ) {
                GetAtomName( lpBpp->atmAddrCxt, rgch, sizeof(rgch)-1 );
            } else {
                GetAtomName( lpBpp->atmCxtDef, rgch, sizeof(rgch)-1 );
            }

            Module = NULL;
            SplitExpression( rgch, &p, &Module );

            if ( Module ) {

                p = strpbrk( Module, "} \t" );

                if ( p ) {

                    *p = 0;

                    Hexe = SHGethExeFromName( Module );

                    if ( !Hexe ) {

                        Ret = FALSE;

                    } else {

                        SHWantSymbols(Hexe);
                        Ret = SHSymbolsLoaded(Hexe, NULL);

                    }
                }
            }
        }
    }

    return Ret;
}

