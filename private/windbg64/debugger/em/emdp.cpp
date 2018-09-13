/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    emdp.c

Abstract:

    This file contains the main driver for the native execution models
    supported by us.  This file additionally contains the machine
    independent portions of the execution model.  The machine dependent
    portions are in other files.

Author:

    Jim Schaad (jimsch) 05-23-92

Environment:

    Win32 -- User

Notes:

    The orginal source for this came from the CodeView group.

--*/

#include "emdp.h"
#include "simpldis.h"

#include "dbgver.h"


#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY


/*************************** DEFINES  *****************************/

#define CBBUFFERDEF 4096  //v-vadimp IA64 has a huge context record
#define CEXM_MDL_native 0x20
#define cchErrMax   50

/******************* TYPEDEFS and STRUCTURE ***********************/


/*********************** LOCAL DATA *******************************/

CRITICAL_SECTION csCache;

LPBYTE          LpSendBuf = NULL;
DWORD           CbSendBuf = 0;

DWORD           CbDmMsg = 0;
LPDM_MSG        LpDmMsg = NULL;

LPDBF           lpdbf = (LPDBF)NULL;

LPFNSVC         lpfnsvcTL = (LPFNSVC)NULL;

FNCALLBACKTL CallTL;
FNCALLBACKDB CallDB;
FNCALLBACKNT CallNT;

HLLI            llprc = (HLLI)NULL;

HPRC            hprcCurr = 0;
HPID            hpidCurr = 0;
PID             pidCurr  = 0;
PCPU_POINTERS   pointersCurr = 0;
MPT             mptCurr;

HTHD            hthdCurr = 0;
HTID            htidCurr = 0;
TID             tidCurr  = 0;

HLLI            HllEo = (HLLI) NULL;



///BUGBUG
// need to either agree on one name for the target dm dll
// or to implement a protocol to get the name via TL
// currently MSVC assumed host==target

#define DEFAULT_DMNAME  _T("dm" DM_TAIL)
//#define       DEFAULT_DMNAME  _T("dmkdx86" DM_TAIL)


#define DEFAULT_DMPARAMS _T("")

LOADDMSTRUCT LoadDmStruct = {
    NULL, NULL
};



/********************* EXTERNAL/GLOBAL DATA ************************/




/*********************** PROTOTYPES *******************************/



/************************** &&&&&& ********************************/

/*
 *  This is the description of all registers and flags for the
 *      machine being debugged.  These files are machine dependent.
 */





/*************************** CODE *****************************************/
LPTSTR
MHStrdup(
    LPTSTR lpstr
    )
{
    LPTSTR retstr = (LPTSTR) MHAlloc((_ftcslen(lpstr) + 1) * sizeof(TCHAR));
    assert(retstr);
    if (retstr) {
        _ftcscpy(retstr, lpstr);
    }
    return(retstr);
}

/**** DBGVersionCheck                                                   ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *      To export out version information to the debugger.                 *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *      NONE.                                                              *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *      Returns - A pointer to the standard version information.           *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *      Just returns a pointer to a static structure.                      *
 *                                                                         *
 ***************************************************************************/

#ifdef DEBUGVER
DEBUG_VERSION('E','M',"Execution Model")
#else
RELEASE_VERSION('E','M',"Execution Model")
#endif

DBGVERSIONCHECK()

/**** SENDCOMMAND - Send a command to the DM                            ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *      Send a DMF command to the DM.                                      *
 *                                                                         *
 *  INPUTS:                                                                *
 *      dmf - the command to send                                          *
 *      hpid - the process                                                 *
 *      htid - the thread                                                  *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *      xosd - error code indicating if command was sent successfully      *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *      Sending commands is asynchronous: this function may return before  *
 *      the DM has actually processed the command.                         *
 *                                                                         *
 ***************************************************************************/
XOSD
SendCommand (
    DMF dmf,
    HPID hpid,
    HTID htid
    )
{
    DBB dbb = {0};

    dbb.dmf  = dmf;
    dbb.hpid = hpid;
    dbb.htid = htid;

    return CallTL ( tlfDebugPacket, hpid, FIELD_OFFSET ( DBB, rgbVar ), (DWORD64)&dbb );
}


/**** SENDREQUEST - Send a request to the DM                            ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *      Send a DMF request to the DM.                                      *
 *                                                                         *
 *  INPUTS:                                                                *
 *      dmf - the request to send                                          *
 *      hpid - the process                                                 *
 *      htid - the thread                                                  *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *      xosd - error code indicating if request was sent successfully      *
 *      LpDmMsg - global buffer filled in with returned data               *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *      Unlike SendCommand, this function will wait for data to be         *
 *      returned from the DM before returning to the caller.               *
 *                                                                         *
 ***************************************************************************/
EMEXPORT XOSD
SendRequest (
    DMF dmf,
    HPID hpid,
    HTID htid
    )
{
    DBB     dbb = {0};
    XOSD    xosd;

    dbb.dmf  = dmf;
    dbb.hpid = hpid;
    dbb.htid = htid;

    xosd = CallTL ( tlfRequest, hpid, FIELD_OFFSET ( DBB, rgbVar ), (DWORD64)&dbb );

    if (xosd != xosdNone) {
        return xosd;
    }

    xosd = (XOSD) LpDmMsg->xosdRet;

    return xosd;
}


/**** SENDREQUESTX - Send a request with parameters to the DM           ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *      Send a DMF request and its parameter info to the DM.               *
 *                                                                         *
 *  INPUTS:                                                                *
 *      dmf - the request to send                                          *
 *      hpid - the process                                                 *
 *      htid - the thread                                                  *
 *      wLen - number of bytes in lpv                                      *
 *      lpv - pointer to additional info needed by the DM; contents are    *
 *          dependent on the DMF                                           *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *      xosd - error code indicating if request was sent successfully      *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *      Unlike SendCommand, this function will wait for data to be         *
 *      returned from the DM before returning to the caller.               *
 *                                                                         *
 ***************************************************************************/
EMEXPORT XOSD
SendRequestX (
    DMF dmf,
    HPID hpid,
    HTID htid,
    DWORD wLen,
    LPVOID lpv
    )
{
    LPDBB   lpdbb;
    XOSD    xosd;

    if (wLen + FIELD_OFFSET(DBB, rgbVar) > CbSendBuf) {
        if (LpSendBuf) {
            MHFree(LpSendBuf);
        }
        CbSendBuf = FIELD_OFFSET(DBB, rgbVar) + wLen;
        LpSendBuf = (LPBYTE) MHAlloc(CbSendBuf);
    }

    if (!LpSendBuf) {
        return xosdOutOfMemory;
    }

    ZeroMemory(LpSendBuf,CbSendBuf);
    lpdbb = (LPDBB)LpSendBuf;

    lpdbb->dmf  = dmf;
    lpdbb->hpid = hpid;
    lpdbb->htid = htid;
    _fmemcpy ( lpdbb->rgbVar, lpv, wLen );

    xosd = CallTL ( tlfRequest, hpid, FIELD_OFFSET ( DBB, rgbVar ) + wLen, (DWORD64)lpdbb );

    if (xosd != xosdNone) {
        return xosd;
    }

    xosd = (XOSD) LpDmMsg->xosdRet;

    return xosd;
}

// Helper function: repack dmfProgLoad/dmfSpawnOrphan arguments into the form:
//              DWORD dwChildFlags
//              TCHAR rgtchExe[]
//              TCHAR rgtchCmdLine[]
//              TCHAR rgtchDir[]
//
//      You probably want to free the lplpvPacket when you're done.
//
XOSD
RepackProgLoad (
    CONST LPPRL lpprl,
    LPVOID      *lplpvPacket,
    UINT        *pcb
    )
{
    BYTE*   lpb;
    LPTSTR  lszRemoteDir = _T("");

    assert (lpprl);
    assert (lplpvPacket);
    assert (pcb);

    // lszRemoteDir is allowed to be NULL, in which case we pass ""
    if (lpprl -> lszRemoteDir != NULL) {
        lszRemoteDir = lpprl -> lszRemoteDir;
    }

    *pcb = sizeof(DWORD);
    *pcb += _ftcslen(lpprl -> lszRemoteExe) + 1;
    *pcb += lpprl->lszCmdLine? (_ftcslen(lpprl -> lszCmdLine) + 1) : 1;

    *pcb += _ftcslen(lszRemoteDir) + 1;
    *pcb += sizeof (SPAWNORPHAN);

#if defined(_UNICODE)
#pragma message("MHAlloc and *lplpvPacket+ctch need work")
#endif
    *lplpvPacket = MHAlloc(*pcb);
    lpb = (BYTE*) *lplpvPacket;

    if (!*lplpvPacket) {
        return xosdOutOfMemory;
    }

//  REVIEW:  SwapEndian ( &dwChildFlags, sizeof ( dwChildFlags ) );

    memcpy (lpb, &(lpprl -> dwChildFlags), sizeof (lpprl -> dwChildFlags));
    lpb += sizeof(DWORD);

    _ftcscpy((CHAR*) lpb, lpprl -> lszRemoteExe);
    lpb += _ftcslen (lpprl -> lszRemoteExe) + 1;

    if (lpprl->lszCmdLine) {
        _ftcscpy((CHAR*) lpb, lpprl -> lszCmdLine);
        lpb += _ftcslen (lpprl -> lszCmdLine) + 1;
    } else {
        *lpb++ = 0;
    }

    _ftcscpy((CHAR*) lpb, lszRemoteDir);
    lpb += _ftcslen (lszRemoteDir) + 1;

    if (lpprl -> lpso) {
        memcpy ((CHAR*) lpb, lpprl -> lpso, sizeof (SPAWNORPHAN));
    } else {
        *lpb = 0;
    }

    return xosdNone;
}
XOSD
SpawnOrphan (
    HPID  hpid,
    DWORD  cb,
    LPSOS lpsos
    )

/*++

Routine Description:

    This routine is called to cause a program to be loaded by the
    debug monitor, but not debugged.

Arguments:

    hpid  - Supplies the OSDEBUG handle to the process to be loaded
    cb    - Length of the command line
    lpsos - Pointer to structure containning the command line

Return Value:

    xosd error code

--*/

{
    LPVOID lpb;
    XOSD xosd;

    xosd = RepackProgLoad(lpsos, &lpb, (UINT*) &cb);
    if (xosd != xosdNone) {
        return (xosd);
    }

    assert (lpsos -> lpso);

    xosd = SendRequestX ( dmfSpawnOrphan,
                          hpid,
                          NULL,
                          cb,
                          lpb
                          );

    MHFree(lpb);

    memcpy (lpsos -> lpso, LpDmMsg->rgb, sizeof (SPAWNORPHAN));

    return xosd;
}                               /* SpawnOrphan() */




XOSD
ProgramLoad (
    HPID  hpid,
    DWORD  cb,
    LPPRL lpprl
    )

/*++

Routine Description:

    This routine is called to cause a program to be loaded by the
    debug monitor.

Arguments:

    hpid  - Supplies the OSDEBUG handle to the process to be loaded
    cb    - Length of the command line
    lpprl - Pointer to structure containning the command line

Return Value:

    xosd error code

--*/

{
    XOSD  xosd = xosdNone;
    LPPRC lpprc;
    HPRC  hprc = HprcFromHpid(hpid);
    LPVOID lpb;
    lpprc = (LPPRC) LLLock ( hprc );

#if 0
    lpprc->efp  = efpNone;
#endif
    LLDestroy ( lpprc->llmdi );
    lpprc->llmdi = LLInit ( sizeof ( MDI ), llfNull, NULL, MDIComp );

    LLUnlock ( hprc );

    PurgeCache ();

    xosd = RepackProgLoad(lpprl, &lpb, (UINT*) &cb);
    if (xosd != xosdNone) {
        return (xosd);
    }
    assert (!lpprl -> lpso);

    xosd = SendRequestX (
        dmfProgLoad,
        hpid,
        NULL,
        cb,
        lpb
    );

    MHFree(lpb);

    if (xosd == xosdNone) {
        xosd = LpDmMsg->xosdRet;
        lpprc = (LPPRC) LLLock ( hprc );
        lpprc->stat = statStarted;
        LLUnlock ( hprc );
    }

    return xosd;
}                               /* ProgramLoad() */


/**** PROGRAMFREE - Terminate the program and free the pid              ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

XOSD
ProgramFree (
    HPID hpid,
    HTID htid
    )
{
    return SendRequest ( dmfProgFree, hpid, NULL );
}





XOSD
CompareAddrs(
    HPID hpid,
    HTID htid,
    LPCAS lpcas
    )
{
    ADDR a1 = *lpcas->lpaddr1;
    ADDR a2 = *lpcas->lpaddr2;
    XOSD xosd = xosdNone;
    int l;

    // if both are LI, see if they are comparable:
    if (ADDR_IS_LI(a1) && ADDR_IS_LI(a2)
          && emiAddr(a1) == emiAddr(a2)
          && GetAddrSeg(a1) == GetAddrSeg(a2))
    {
       if (GetAddrOff(a1) < GetAddrOff(a2)) {
           l = fCmpLT;
       } else if (GetAddrOff(a1) == GetAddrOff(a2)) {
           l = fCmpEQ;
       } else {
           l = fCmpGT;
       }
       *lpcas->lpResult = l;
    }

    else {

        // if neccessary, fixup addresses:
        if (ADDR_IS_LI(a1)) {
            FixupAddr(hpid, htid, &a1);
        }

        if (ADDR_IS_LI(a2)) {
            FixupAddr(hpid, htid, &a2);
        }


        // if real mode address, we can really compare
        if (ADDR_IS_REAL(a1) && ADDR_IS_REAL(a2)) {
            LONG64 ll =  ((GetAddrSeg(a1) << 4) + (GetAddrOff(a1) & 0xffff))
                - ((GetAddrSeg(a2) << 4) + (GetAddrOff(a2) & 0xffff));
            *lpcas->lpResult = (ll < 0) ? -1 : ((ll == 0) ? 0 : 1);
        }

        else if (ADDR_IS_FLAT(a1) != ADDR_IS_FLAT(a2)) {
            xosd = xosdInvalidParameter;
        }

        // if flat, ignore selectors
        else if (ADDR_IS_FLAT(a1)) {
            if (GetAddrOff(a1) < GetAddrOff(a2)) {
                l = fCmpLT;
            } else if (GetAddrOff(a1) == GetAddrOff(a2)) {
                l = fCmpEQ;
            } else {
                l = fCmpGT;
            }
            *lpcas->lpResult = l;
        }

        else if (GetAddrSeg(a1) == GetAddrSeg(a2)) {
            if (GetAddrOff(a1) < GetAddrOff(a2)) {
                l = fCmpLT;
            } else if (GetAddrOff(a1) == GetAddrOff(a2)) {
                l = fCmpEQ;
            } else {
                l = fCmpGT;
            }
            *lpcas->lpResult = l;
        }

        // not flat, different selectors
        else {
            xosd = xosdInvalidParameter;
        }

    }
    return xosd;
}



static BOOL fCacheDisabled = FALSE;

#define cbMaxCache CACHESIZE
typedef struct _MCI {
    WORD cb;
    HPID hpid;
    ADDR addr;
    BYTE rgb [ cbMaxCache ];
} MCI;  // Memory Cache Item

#define imciMax MAXCACHE
MCI FAR rgmci [ imciMax ] = {   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };

// Most recent used == 0, 2nd to last == 1, etc

int rgiUsage [ imciMax ] = {0};

void
InitUsage (
    void
    )
{
    int iUsage;

    for ( iUsage = 0; iUsage < imciMax; iUsage++ ) {
        rgiUsage [ iUsage ] = imciMax - ( iUsage + 1 );
    }
}

VOID
SetMostRecent (
    int imci
    )
{
    int i;

    if ( rgiUsage [ imci ] != 0 ) {
        for ( i = 0; i < imciMax; i++ ) {
            if ( rgiUsage [ i ] < rgiUsage [ imci ] ) {
                rgiUsage [ i ] ++;
            }
        }
        rgiUsage [ imci ] = 0;
    }
}

int
GetLeastRecent (
    VOID
    )
{
    int i;

    for ( i = 0; i < imciMax; i++ ) {
        assert ( rgiUsage [ i ] >= 0 && rgiUsage [ i ] < imciMax );
        if ( rgiUsage [ i ] == imciMax - 1 ) {
            return i;
        }
    }

    assert ( FALSE );

    return i;
}

VOID
SetLeastRecent (
    int imci
    )
{
    int i;

    if ( rgiUsage [ imci ] != imciMax - 1 ) {
        for ( i = 0; i < imciMax; i++ ) {
            if ( rgiUsage [ i ] > rgiUsage [ imci ] ) {
                rgiUsage [ i ] --;
            }
        }
        rgiUsage [ imci ] = imciMax-1;
    }
}


XOSD
ReadPhysical (
    HPID    hpid,
    DWORD   cb,
    LPBYTE  lpbDest,
    LPADDR  lpaddr,
    DWORD   iCache,
    LPDWORD lpcbr
    )
{
    LPDBB lpdbb;
    PRWP  prwp;
    WORD  wRet = 0;
    XOSD  xosd = xosdNone;

    if (!ValidHprcFromHpid(hpid)) {
        return xosdBadProcess;
    }

    lpdbb = (LPDBB)MHAlloc(FIELD_OFFSET(DBB, rgbVar) + sizeof(RWP));
    ZeroMemory(lpdbb,FIELD_OFFSET(DBB, rgbVar) + sizeof(RWP));
    prwp = (PRWP)lpdbb->rgbVar;

    lpdbb->dmf = dmfReadMem;
    lpdbb->hpid = hpid;
    lpdbb->htid = NULL;

    if ( cb + sizeof(DWORD) + FIELD_OFFSET(DM_MSG, rgb) > CbDmMsg ) {
        MHFree ( LpDmMsg );
        CbDmMsg = cb + sizeof ( DWORD ) + FIELD_OFFSET( DM_MSG, rgb );
        LpDmMsg = (LPDM_MSG) MHAlloc ( CbDmMsg );
        CallTL ( tlfSetBuffer, lpdbb->hpid, CbDmMsg, (DWORD64)LpDmMsg );
    }

    prwp->cb   = cb;
    prwp->addr = *lpaddr;

    xosd = CallTL(tlfRequest, lpdbb->hpid, FIELD_OFFSET(DBB, rgbVar) + sizeof(RWP), (DWORD64)lpdbb);

    if (xosd == xosdNone) {
        xosd = LpDmMsg->xosdRet;
        if (xosd == xosdNone) {
            *lpcbr = *( (LPDWORD) (LpDmMsg->rgb) );
            assert( *lpcbr <= cb );
            _fmemcpy ( lpbDest, LpDmMsg->rgb + sizeof ( DWORD ), *lpcbr );
        }
    }

    MHFree(lpdbb);

    return xosd;
}

XOSD
EnableCache (
    HPID  hpid,
    HTID  htid,
    BOOL  state
    )
{
    fCacheDisabled = state;

    if (fCacheDisabled) {
        PurgeCache();
    }

    return xosdNone;
}


void
PurgeCache (
    VOID
    )
{
    int imci;

    for ( imci = 0; imci < imciMax; imci++ ) {
        rgmci [ imci ].cb = 0;
    }
}

void
PurgeCacheHpid (
    HPID hpid
    )
{
    int imci;

    for ( imci = 0; imci < imciMax; imci++ ) {

        if ( rgmci [ imci ].hpid == hpid ) {
            rgmci [ imci ].cb = 0;
            SetLeastRecent ( imci );
        }
    }
}


XOSD
ReadForCacheIA64(
    HPID   hpid,
    DWORD  cbP,
    LPBYTE lpbDest,
    LPADDR lpaddr,
    LPDWORD lpcb
    )

/*++

Routine Description:

    This function will fill in a cache entry with the bytes requested
    to be read.  The function puts the bytes in both the cache and the
    memory buffer.

Arguments:

    hpid        - Supplies the process to do the read in

    cbP         - Supplies the number of bytes to be read

    lpbDest     - Supplies the buffer to place the bytes in

    lpaddr      - Supplies the address to read the bytes from

    lpcb        - Returns the number of bytes read

Return Value:

    XOSD error code

--*/

{
    DWORD       cbT = cbMaxCache; //v-vadimp - now we can request more than cache size, but will still read at most cache size
    DWORD       cb = cbP;
    DWORD       imci;
    DWORD       cbr;
    int         dbOverlap = 0;
    XOSD        xosd;
    ADDR        addrSave = *lpaddr;
    MCI *       pmci;

    /*
     *  Determine if the starting address is contained in a
     *  voided cache entry
     */

    for ( imci = 0, pmci = rgmci ; imci < imciMax; imci++, pmci++ ) {

        if ( (pmci->cb == 0) &&
             (pmci->hpid == hpid) &&
             (ADDR_IS_REAL( pmci->addr) == ADDR_IS_REAL( *lpaddr )) &&
             (GetAddrSeg ( pmci->addr ) == GetAddrSeg ( *lpaddr )) &&
             (GetAddrOff ( *lpaddr ) >= GetAddrOff ( pmci->addr )) &&
                         // Check required to detect arithmetic overflow cases.
             (GetAddrOff(*lpaddr) < GetAddrOff( pmci->addr ) + cbMaxCache) &&
             (GetAddrOff ( *lpaddr ) + cbP < GetAddrOff ( pmci->addr ) + cbMaxCache)
        ) {
            //v-vadimp - doesn't matter what was in this entry
            break;
        }
    }


    /*
     *  if we have not found a cache entry then just get one based on
     *  an LRU algorithm.
     */

    if ( imci == imciMax ) {
        imci = GetLeastRecent ( );
    }

    /*
     *  Do an actual read of memory from the debuggee
     */

        // some IA64 tricks to improve performance on Gambit
    ADDR backAddr = *lpaddr;
    if (GetAddrOff(*lpaddr) == 0) { // reads from 0 always fail
        return xosdGeneral;
    }
    // v-vadimp - this will read memory around the starting address, intended to improve
    // displaying values on the stack where we walk backwards, or disasm so first try reading lpaddr +/- cbT/2
    if (cb <= cbT/2) { //v-vadimp - if there is enough space to read before
        GetAddrOff(*lpaddr) = __max ((SOFFSET)(GetAddrOff(*lpaddr) - cbT/2), 0);
        dbOverlap = (DWORD)(GetAddrOff(addrSave) - GetAddrOff(*lpaddr));
        if (GetAddrOff(*lpaddr) != 0) {
            xosd = ReadPhysical ( hpid, cbT, rgmci [ imci ].rgb, lpaddr, imci, &cbr );
        } else { //fall thru to regular read if adjusted address is 0
            xosd = xosdGeneral;
        }
    } else {
        xosd = xosdGeneral;
    }
    // in case it fails - try the regular read, don't retry reads from 0
    if (xosd != xosdNone) {
        *lpaddr = backAddr;
        dbOverlap = 0;
        xosd = ReadPhysical ( hpid, cbT, rgmci [ imci ].rgb, lpaddr, imci, &cbr );
    }

    if ( xosd != xosdNone ) {
        return xosd;
    }

    /*
     *  If only a partial cache entry was read in then reset our read
     *  size variable.
     */

    if ( cbr < cbT ) {
        cbT = cbr;
    }

    /*
     *  touch the LRU table
     */

    SetMostRecent ( imci );

    /*
     *  set up the cache entry
     */

    assert(cbT <= cbMaxCache);
    rgmci [ imci ].cb = (WORD) cbT;
    rgmci [ imci ].addr = *lpaddr;
    rgmci [ imci ].hpid = hpid;
    *lpaddr = addrSave;

    /*
     *  compute the number of bytes read
     */

    cbT = min( cbP, (DWORD)rgmci[ imci ].cb - dbOverlap);

    /*
     *  copy from the cache entry to the users space
     */

    _fmemcpy ( lpbDest, &rgmci [ imci ].rgb [dbOverlap], cbT );

    /*
     *  return the number of bytes read
     */

    *lpcb = cbT;

    return xosdNone;
}                               /* ReadForCache() */

XOSD
ReadForCache(
    HPID   hpid,
    DWORD  cbP,
    LPBYTE lpbDest,
    LPADDR lpaddr,
    LPDWORD lpcb
    )

/*++

Routine Description:

    This function will fill in a cache entry with the bytes requested
    to be read.  The function puts the bytes in both the cache and the
    memory buffer.

Arguments:

    hpid        - Supplies the process to do the read in

    cbP         - Supplies the number of bytes to be read

    lpbDest     - Supplies the buffer to place the bytes in

    lpaddr      - Supplies the address to read the bytes from

    lpcb        - Returns the number of bytes read

Return Value:

    XOSD error code

--*/

{
    assert(cbP <= cbMaxCache);
    int         cb = cbP;
    DWORD       cbT  = cb < cbMaxCache ? cbMaxCache : cb;
    DWORD       imci;
    DWORD       cbr;
    int         dbOverlap = 0;
    XOSD        xosd;
    ADDR        addrSave = *lpaddr;
    MCI *       pmci;

    /*
     *  Determine if the starting address is contained in a
     *  voided cache entry
     */

    for ( imci = 0, pmci = rgmci ; imci < imciMax; imci++, pmci++ ) {

        if ( (pmci->cb == 0) &&
             (pmci->hpid == hpid) &&
             (ADDR_IS_REAL( pmci->addr) == ADDR_IS_REAL( *lpaddr )) &&
             (GetAddrSeg ( pmci->addr ) == GetAddrSeg ( *lpaddr )) &&
             (GetAddrOff ( *lpaddr ) >= GetAddrOff ( pmci->addr )) &&
                         // Check required to detect arithmetic overflow cases.
             (GetAddrOff(*lpaddr) < GetAddrOff( pmci->addr ) + cbMaxCache) &&
             (GetAddrOff ( *lpaddr ) + cbP < GetAddrOff ( pmci->addr ) + cbMaxCache)
        ) {
            dbOverlap = (int) (GetAddrOff ( pmci->addr ) - GetAddrOff ( *lpaddr ) );
            assert(dbOverlap <= cbMaxCache);
            GetAddrOff ( *lpaddr ) = GetAddrOff ( pmci->addr );
            break;
        }
    }


    /*
     *  if we have not found a cache entry then just get one based on
     *  an LRU algorithm.
     */

    if ( imci == imciMax ) {
        imci = GetLeastRecent ( );
    }

    /*
     *  Do an actual read of memory from the debuggee
     */

    xosd = ReadPhysical ( hpid, cbT, rgmci [ imci ].rgb, lpaddr, imci, &cbr );


    if ( xosd != xosdNone ) {
        return xosd;
    }

    /*
     *  If only a partial cache entry was read in then reset our read
     *  size variable.
     */

    if ( cbr < cbT ) {
        cbT = cbr;
    }

    /*
     * if we did not anything (or enough), then don't adjust
     */

    if ( (int)cbr + dbOverlap > 0 ) {
        cbT += dbOverlap;
    }

    /*
     *  touch the LRU table
     */

    SetMostRecent ( imci );

    /*
     *  set up the cache entry
     */

    assert(cbT <= cbMaxCache);
    rgmci [ imci ].cb = (WORD) cbT;
    rgmci [ imci ].addr = *lpaddr;
    rgmci [ imci ].hpid = hpid;
    GetAddrOff ( *lpaddr ) += cbT;
    *lpaddr = addrSave;

    /*
     *  compute the number of bytes read
     */

    cbT = (int)min( cbP, (DWORD)rgmci[ imci ].cb );

    /*
     *  copy from the cache entry to the users space
     */

    if ( dbOverlap >= 0 ) {
        dbOverlap = 0;
    }
    _fmemcpy ( lpbDest, rgmci [ imci ].rgb - dbOverlap, cbT );

    /*
     *  return the number of bytes read
     */

    *lpcb = cbT;

    return xosdNone;
}                               /* ReadForCache() */


int
GetCacheIndex(
    HPID   hpid,
    LPADDR lpaddr
    )

/*++

Routine Description:

    This routine is given a process and an address and will locate
    which cache entry (if any) the address is in.

Arguments:

    hpid        - Supplies the handle to the process
    lpaddr      - Supplies the address to look for

Return Value:

    The index of the cache entry containing the address or imciMax if
    no cache entry contains the address

--*/

{
    int imci;

    for ( imci = 0; imci < imciMax; imci++ ) {
        LPADDR lpaddrT = &rgmci [ imci ].addr;

        /*
         *   To be in the cache entry check:
         *
         *      1.  The cache entry contains bytes
         *      2.  The cache entry is for the correct process
         *      3.  The cache entry if for the correct segment
         *      4.  The requested offset is between the starting and
         *              ending points of the cache
         */

        if ( (rgmci [ imci ].cb != 0) &&
             (rgmci [ imci ].hpid == hpid) &&
             (ADDR_IS_REAL( *lpaddrT ) == ADDR_IS_REAL( *lpaddr )) &&
             (GetAddrSeg ( *lpaddrT ) == GetAddrSeg ( *lpaddr )) &&
             (GetAddrOff ( *lpaddrT ) <= GetAddrOff ( *lpaddr )) &&
             (GetAddrOff ( *lpaddrT ) + rgmci[ imci ].cb > GetAddrOff ( *lpaddr ))) {

            break;
        }
    }

    return imci;
}                               /* GetCacheIndex() */


int
ReadFromCache (
    HPID hpid,
    DWORD cb,
    LPBYTE lpbDest,
    LPADDR lpaddr
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hpid        - Supplies a handle to the process
    cb          - Supplies the count of bytes to read from the cache
    lpbDest     - Suppiies the pointer to store bytes at
    lpaddr      - Supplies pointer to address to read at

Return Value:

    > 0         - The number of bytes read from the cache
    == 0        - No cache entry for the address was found

--*/

{
    int imci;

    /*
     *  See if the address for the start of the read is current contained
     *  in one of the cached buffers.
     */

    imci = GetCacheIndex ( hpid, lpaddr );

    /*
     *  If the starting address is in a cache entry then read as many
     *  bytes as is possible from that cache entry.
     */

    if ( imci != imciMax ) {
        DWORD ibStart;
        DWORD  cbT;

        /*
         *  Compute the difference between the address for the cache start
         *      and the address for the read request start and then
         *      the number of bytes which can be read in
         */

        ibStart = (DWORD)( GetAddrOff ( *lpaddr ) - GetAddrOff ( rgmci[imci].addr ) );
        cbT = min ( cb, rgmci [ imci ].cb - ibStart );

        /*
         *   Preform the copy
         */

        _fmemcpy ( lpbDest, rgmci [ imci ].rgb + ibStart, cbT );

        /*
         *   Return the number of bytes copied.  If it is less than
         *      zero then for some reason the current cache was not
         *      filled to capacity.
         */

        return cbT;
    }

    return 0;
}                               /* ReadFromCache() */

XOSD
ReadBufferIA64 (
    HPID    hpid,
    HTID    htid,
    LPADDR  lpaddr,
    DWORD   cb,
    LPBYTE  lpbDest,
    LPDWORD lpcbRead
    )
/*++

Routine Description:

    This function is called in response to an emfReadBuf message.  The
    address to start the read at was set earlier and is stored in the
    adrCurrent Address Buffer.

Arguments:

    hpid        - Supplies the handle to the process to read memory for

    htid        - Supplies the handle to the thread to read memory for
                        (may be NULL)

    cb          - Supplies count of bytes to read

    lpbDest     - Supplies pointer to buffer to place bytes read

    lpcbRead    - Returns number of bytes read

Return Value:

    if >= 0 then it is the number of bytes actually read otherwise it
    is an xosdError code.

--*/

{
    XOSD        xosd = xosdNone;
    ADDR        addr;
    int         cbT = 0;
    int         cbRead = 0;
    HPRC        hprc = HprcFromHpid(hpid);
    LPPRC       lpprc;


    /*
     *  Retrieve the address to start the read at from the address buffer
     *  location
     */

    addr = *lpaddr;

    if (ADDR_IS_LI(addr)) {
        *lpcbRead = 0;
        return xosdBadAddress;      // can only do fixup addresses
    }

    /* If we are at the end of the memory address range and are trying to read
     * beyond the address range, just read till 0xFFFFFFFF
     */
    if ( cb != 0 && GetAddrOff(addr) + cb - 1 < GetAddrOff(addr)) {
        cb = (DWORD)(0 - GetAddrOff(addr));
    }

    /*
     *  Are we trying to read more bytes than is possible to store in
     *  a single cache?  If so then skip trying to hit the cache and
     *  go directly to asking the DM for the memory.
     *
     *  This generally is due to large memory dumps.
     */

    lpprc = (LPPRC) LLLock(hprc);
    if ( (lpprc->fRunning ) || (fCacheDisabled) ) { //v-vadimp - even if trying to read more than cache size try to read as much as possible from cache first
        LLUnlock(hprc);
        return ReadPhysical ( hpid, cb, lpbDest, &addr, MAXCACHE, lpcbRead );
    }
    LLUnlock(hprc);

    /*
     *  Read as much as possible from the set of cached memory reads.
     *  If cbT > 0 then bytes were read from a cache entry
     *  if cbT == 0 then no bytes were read in
     */

tryReadingCacheAgain:
    while ((cb != 0) &&
           ( cbT = ReadFromCache ( hpid, cb, lpbDest, &addr ) ) > 0 ) {
        cbRead += cbT;
        lpbDest += cbT;
        GetAddrOff ( addr ) += cbT;
        cb -= cbT;
    }

    /*
     *  If there are still bytes left to be read then get the cache
     *  routines to read them in and copy both to a cache and to the
     *  buffer.
     */

    if ( cb > 0 ) {
        xosd = ReadForCacheIA64 ( hpid, cb, lpbDest, &addr, (LPDWORD) &cbT );
        if (xosd == xosdNone) { //v-vadimp - might not have read everything (cb>sizeof(cache entry)) go and try reading from cache again
            cbRead += cbT;
            lpbDest += cbT;
            GetAddrOff ( addr ) += cbT;
            cb -= cbT;
            goto tryReadingCacheAgain;
        }
    }

    if (lpcbRead) {
        *lpcbRead = cbRead;
    }

    return xosd;
}                               /* ReadBuffer() */


XOSD
ReadBuffer (
    HPID    hpid,
    HTID    htid,
    LPADDR  lpaddr,
    DWORD   cb,
    LPBYTE  lpbDest,
    LPDWORD lpcbRead
    )
/*++

Routine Description:

    This function is called in response to an emfReadBuf message.  The
    address to start the read at was set earlier and is stored in the
    adrCurrent Address Buffer.

Arguments:

    hpid        - Supplies the handle to the process to read memory for

    htid        - Supplies the handle to the thread to read memory for
                        (may be NULL)

    cb          - Supplies count of bytes to read

    lpbDest     - Supplies pointer to buffer to place bytes read

    lpcbRead    - Returns number of bytes read

Return Value:

    if >= 0 then it is the number of bytes actually read otherwise it
    is an xosdError code.

--*/

{
    XOSD        xosd = xosdNone;
    ADDR        addr;
    int         cbT = 0;
    int         cbRead = 0;
    HPRC        hprc = HprcFromHpid(hpid);
    LPPRC       lpprc;


    /*
     *  Retrieve the address to start the read at from the address buffer
     *  location
     */

    if (MPTFromHprc(hprc) == mptia64) { //custom read procedure for IA64 with some perf tricks
        return ReadBufferIA64( hpid, htid, lpaddr, cb, lpbDest, lpcbRead );
    }

    addr = *lpaddr;

    if (ADDR_IS_LI(addr)) {
        *lpcbRead = 0;
        return xosdBadAddress;      // can only do fixup addresses
    }

    /* If we are at the end of the memory address range and are trying to read
     * beyond the address range, just read till 0xFFFFFFFF
     */
    if ( cb != 0 && GetAddrOff(addr) + cb - 1 < GetAddrOff(addr)) {
        cb = (DWORD)(0 - GetAddrOff(addr));
    }

    /*
     *  Are we trying to read more bytes than is possible to store in
     *  a single cache?  If so then skip trying to hit the cache and
     *  go directly to asking the DM for the memory.
     *
     *  This generally is due to large memory dumps.
     */

    lpprc = (LPPRC) LLLock(hprc);
    if ( (cb > cbMaxCache) || (lpprc->fRunning ) || (fCacheDisabled) ) {
        LLUnlock(hprc);
        return ReadPhysical ( hpid, cb, lpbDest, &addr, MAXCACHE, lpcbRead );
    }
    LLUnlock(hprc);

    /*
     *  Read as much as possible from the set of cached memory reads.
     *  If cbT > 0 then bytes were read from a cache entry
     *  if cbT == 0 then no bytes were read in
     */

    while ((cb != 0) &&
           ( cbT = ReadFromCache ( hpid, cb, lpbDest, &addr ) ) > 0 ) {
        cbRead += cbT;
        lpbDest += cbT;
        GetAddrOff ( addr ) += cbT;
        cb -= cbT;
    }

    /*
     *  If there are still bytes left to be read then get the cache
     *  routines to read them in and copy both to a cache and to the
     *  buffer.
     */

    if ( cb > 0 ) {
        xosd = ReadForCache ( hpid, cb, lpbDest, &addr, (LPDWORD) &cbT );
        if (xosd == xosdNone) {
            cbRead += cbT;
        }
    }

    if (lpcbRead) {
        *lpcbRead = cbRead;
    }

    return xosd;
}                               /* ReadBuffer() */



XOSD
WriteBufferCache (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    DWORD cb,
    LPBYTE lpb,
    LPDWORD lpdwBytesWritten
    )
{
    PurgeCacheHpid ( hpid );
    return WriteBuffer ( hpid, htid, lpaddr, cb, lpb, lpdwBytesWritten );
}



XOSD
WriteBuffer (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    DWORD cb,
    LPBYTE lpb,
    LPDWORD lpdwBytesWritten
    )

/*++

Routine Description:

    This routine is used to send a request to the Debug Monitor to
    do a write to the debuggees memory.

Arguments:

    hpid        - Supplies the handle to the process to write memory in

    htid        - Supplies a thead handle

    lpaddr      - Supplies debuggee address to write at

    cb          - Supplies the number of bytes to be written

    lpb         - Supplies a pointer to the buffer to write

    lpdwBytesWritten - Returns number of bytes actually written

Return Value:

    an XOSD error code

--*/
{
    LPRWP lprwp = (LPRWP) MHAlloc( FIELD_OFFSET( RWP, rgb ) + cb );
    XOSD  xosd;

    lprwp->cb   = cb;
    lprwp->addr = *lpaddr;

    _fmemcpy ( lprwp->rgb, lpb, cb );

    xosd = SendRequestX (dmfWriteMem,
                         hpid,
                         htid,
                         FIELD_OFFSET ( RWP, rgb ) + cb,
                         lprwp
                         );

    MHFree ( lprwp );

    if (xosd == xosdNone) {
        *lpdwBytesWritten = *((LPDWORD)(LpDmMsg->rgb));
    } else {
        // REVIEW: what about partial writes. The DM will still send
        // the exact number of bytes written. Should we return that
        // back in lpdwBytesWritten
    }

    //
    //  Notify the shell that we changed memory. An error here is not
    //  tragic, so we ignore the return code.  The shell uses this
    //  notification to update all its memory breakpoints.
    //
    CallDB (
        dbcMemoryChanged,
        hpid,
        NULL,
        CEXM_MDL_native,
        cb,
        (DWORD64)lpaddr
        );

    return xosd;
}                               /* WriteBuffer() */



typedef struct _EMIC {
    HEMI hemi;
    HPID hpid;
    WORD sel;
} EMIC; // EMI cache item

#define cemicMax 4

EMIC rgemic [ cemicMax ] = {0};

XOSD
FindEmi (
    HPID   hpid,
    LPADDR lpaddr
    )
{
    XOSD        xosd = xosdNone;
    WORD        sel = (WORD)GetAddrSeg ( *lpaddr );
    HPRC        hprc = HprcFromHpid(hpid);
    HLLI        llmdi = LlmdiFromHprc ( hprc );
    BOOL        fFound = FALSE;
    ULONG       iobj = 0;
    HMDI        hmdi;
    LPPRC       lpprc = (LPPRC) LLLock( hprc );

    if ((lpprc->dmi.fAlwaysFlat) || (sel == lpprc->selFlatCs) || (sel == lpprc->selFlatDs)) {
        ADDR_IS_FLAT(*lpaddr) = TRUE;
    }
    LLUnlock( hprc );

    for ( hmdi = LLNext ( llmdi, hmdiNull );
          hmdi != hmdiNull;
          hmdi = LLNext ( llmdi, hmdi ) ) {

        LPMDI   lpmdi = (LPMDI) LLLock ( hmdi );
        LPOBJD  rgobjd;

        assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

        if (ADDR_IS_FLAT(*lpaddr) && (!ADDR_IS_LI(*lpaddr))) {
            if (GetAddrOff(*lpaddr) < lpmdi->lpBaseOfDll ||
                GetAddrOff(*lpaddr) >= lpmdi->lpBaseOfDll+lpmdi->dwSizeOfDll) {
                //
                // can't be in this dll so look at the next one
                //
                LLUnlock ( hmdi );
                continue;
            }
        }

        if (lpmdi && lpmdi->cobj == -1) {
            if (GetSectionObjectsFromDM( hpid, lpmdi ) != xosdNone) {
                //
                // can't do it now; punt with hpid.
                //
                LLUnlock ( hmdi );
                break;
            }
        }

        rgobjd = &lpmdi->rgobjd[0];


        //  added && lpmdi->hemi to this conditional: the hemi can be null in
        //  the case that we recieved a dbcLoadModule, but did not end up
        //  loading the module  (a-math)

        for ( iobj = 0; iobj < lpmdi->cobj; iobj++ ) {
            if (((lpmdi->fFlatMode && ADDR_IS_FLAT(*lpaddr)) ||
                 (rgobjd[iobj].wSel == sel) && !ADDR_IS_FLAT(*lpaddr)) &&
                (rgobjd[iobj].offset <= GetAddrOff(*lpaddr)) &&
                (GetAddrOff(*lpaddr) < rgobjd[iobj].offset + rgobjd[iobj].cb) &&
                lpmdi->hemi) {

                fFound = TRUE;
                break;
            }
        }

        LLUnlock ( hmdi );

        // This break is here instead of in the "for" condition so
        //   that hmdi does not get advanced before we break

        if ( fFound ) {
            break;
        }
    }


    if ( !fFound ) {
        emiAddr ( *lpaddr ) = (HEMI) hpid;
    } else {
        emiAddr ( *lpaddr ) = (HEMI) HemiFromHmdi ( hmdi );

        if ( LLNext ( llmdi, hmdiNull ) != hmdi ) {

            // put the most recent hit at the head
            // this is an optimization to speed up the fixup/unfixup process
            LLRemove ( llmdi, hmdi );
            LLAddHead ( llmdi, hmdi );
        }
    }

    assert ( emiAddr ( *lpaddr ) != 0 );

    return xosd;
}

#pragma optimize ("", off)
XOSD
SetEmiFromCache (
    HPID   hpid,
    LPADDR lpaddr
    )
{
    XOSD xosd = xosdContinue;
#ifndef TARGET32
    int  iemic;

    for ( iemic = 0; iemic < cemicMax; iemic++ ) {

        if ( rgemic [ iemic ].hpid == hpid &&
             rgemic [ iemic ].sel  == GetAddrSeg ( *lpaddr ) ) {

            if ( iemic != 0 ) {
                EMIC emic = rgemic [ iemic ];
                int iemicT;

                for ( iemicT = iemic - 1; iemicT >= 0; iemicT-- ) {
                    rgemic [ iemicT + 1 ] = rgemic [ iemicT ];
                }
                rgemic [ 0 ] = emic;
            }

            xosd = xosdNone;
            emiAddr ( *lpaddr ) = rgemic [ 0 ].hemi;
            assert ( emiAddr ( *lpaddr ) != 0 );
            break;
        }
    }
#else
    Unreferenced( hpid );
    Unreferenced( lpaddr );
#endif // !TARGET32
    return xosd;
}
#pragma optimize ("", on)

XOSD
SetCacheFromEmi (
    HPID hpid,
    LPADDR lpaddr
    )
{
    int iemic;

    assert ( emiAddr ( *lpaddr ) != 0 );

    for ( iemic = cemicMax - 2; iemic >= 0; iemic-- ) {

        rgemic [ iemic + 1 ] = rgemic [ iemic ];
    }

    rgemic [ 0 ].hpid = hpid;
    rgemic [ 0 ].hemi = emiAddr ( *lpaddr );
    rgemic [ 0 ].sel  = (WORD)GetAddrSeg ( *lpaddr );

    return xosdNone;
}


/*** CleanCacheOfEmi
 *
 *  Purpose:
 *              To purge the emi cache
 *
 *  Notes:
 *              The emi cache must be purged whenever a RegisterEmi
 *              is done.  Unpredicable results can occur otherwise.
 *
 */
XOSD
CleanCacheOfEmi (
    void
    )
{
    int iemic;

    for ( iemic = 0; iemic < cemicMax; iemic++ ) {

        rgemic [ iemic ].hpid = NULL;
        rgemic [ iemic ].sel  = 0;
    }

    return xosdNone;
}



XOSD
SetEmi (
    HPID   hpid,
    LPADDR lpaddr
    )
{
    XOSD xosd = xosdNone;

    if ( emiAddr ( *lpaddr ) == 0 ) {

        //if (ADDR_IS_REAL(*lpaddr)) {
        //    emiAddr( *lpaddr ) = (HEMI) hpid;
        //    return xosd;
        //}

        if ( ( xosd = SetEmiFromCache ( hpid, lpaddr ) ) == xosdContinue ) {

            xosd = FindEmi ( hpid, lpaddr );
            if ( xosd == xosdNone ) {
                SetCacheFromEmi ( hpid, lpaddr );
            }
        }

        assert ( emiAddr ( *lpaddr ) != 0 );
    }

    return xosd;
}





/*
   Note: We are not guaranteed that the incoming address is actually
    on an instruction boundary.  When this happens, we derive the
    boundary and send back the difference in the return value and
    the address of the instruction previous to the DERIVED instruction
    in the address.

    Thus there are three classes of returns -

        ==0 - The incoming address was in fact on an instruction boundary
        > 0 - The case noted above
        < 0 - Error value - the most common "error" is that there is
                no previous instruction.

        When the return value >= 0, *lpaddr contains the address of the
            previous instruction.
*/


#define doffMax 60

static HPID hpidGPI = NULL;
static BYTE rgbGPI [ doffMax ];
static ADDR addrGPI;

XOSD
GPIBuildCache (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    )
{
    XOSD xosd   =  xosdBadAddress;
    int  fFound =  FALSE;
    ADDR addr   = *lpaddr;
    ADDR addrT;
    int  ib = 0;


    _fmemset ( rgbGPI, 0, doffMax );

    addrGPI = *lpaddr;
    hpidGPI = hpid;

    GetAddrOff ( addr ) -= (int) min ( (UOFFSET) doffMax, GetAddrOff ( *lpaddr ) );

    while ( !fFound && GetAddrOff ( addr ) < GetAddrOff ( *lpaddr ) ) {
        SDI  sdi;

        sdi.dop    = dopNone;
        sdi.addr   = addr;

        addrT = addr;

        Disasm ( hpid, htid, &sdi );

        addr = sdi.addr;

        rgbGPI [ ib ] = (BYTE) ( GetAddrOff ( addrGPI ) - GetAddrOff ( addr ) );

        if ( GetAddrOff ( addr ) == GetAddrOff ( *lpaddr ) ) {
            xosd   = xosdNone;
            *lpaddr= addrT;
            fFound = TRUE;
        }

        ib += 1;
    }

    // We haven't synced yet, so *lpaddr is probably pointing
    //  to something that isn't really synchronous

    if ( !fFound ) {
        xosd   = (XOSD) ( GetAddrOff ( *lpaddr ) - GetAddrOff ( addrT ) );
        GetAddrOff ( *lpaddr ) -= xosd;
        if ( GetAddrOff ( *lpaddr ) != 0 ) {
            (void) GetPrevInst ( hpid, htid, lpaddr );
        }
    }

    return xosd;
}


VOID
GPIShiftCache (
    LPADDR lpaddr,
    int *pib
    )
{
    int doff = (int) ( GetAddrOff ( addrGPI ) - GetAddrOff ( *lpaddr ) );
    int ib   = 0;

    *pib = 0;
    while ( ib < doffMax && rgbGPI [ ib ] != 0 ) {
        rgbGPI [ ib ] = (BYTE) max ( (int) rgbGPI [ ib ] - doff, 0 );

        if ( rgbGPI [ ib ] == 0 && *pib == 0 ) {
            *pib = ib;
        }

        ib += 1;
    }

    addrGPI = *lpaddr;
}

XOSD
GPIUseCache (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    )
{
    XOSD xosd   =  xosdBadAddress;
    int  fFound =  FALSE;
    ADDR addr   = *lpaddr;
    int  ib     =  0;
    int  ibCache=  0;
    int  ibMax  =  0;
    BYTE rgb [ doffMax ];


    GPIShiftCache ( lpaddr, &ibMax );

    _fmemset ( rgb, 0, doffMax );

    GetAddrOff ( addr ) -= (int) min ( (UOFFSET) doffMax, GetAddrOff ( *lpaddr ) );

    while ( !fFound && GetAddrOff ( addr ) < GetAddrOff ( *lpaddr ) ) {
        ADDR addrT;
        BYTE doff = (BYTE) ( GetAddrOff ( *lpaddr ) - GetAddrOff ( addr ) );

        // Attempt to align with the cache

        while ( doff < rgbGPI [ ibCache ] ) {
            ibCache += 1;
        }

        if ( doff == rgbGPI [ ibCache ] ) {

            // We have alignment with the cache

            addr  = *lpaddr;
            addrT = addr;
            GetAddrOff ( addrT ) -= rgbGPI [ ibMax - 1 ];
        }
        else {
            SDI  sdi;

            sdi.dop = dopNone;
            sdi.addr = addr;
            addrT = addr;

            Disasm ( hpid, htid, &sdi );


            addr = sdi.addr;

            rgb [ ib ] = (BYTE) ( GetAddrOff ( addrGPI ) - GetAddrOff ( addr ) );

            ib += 1;
        }

        if ( GetAddrOff ( addr ) == GetAddrOff ( *lpaddr ) ) {
            xosd   = xosdNone;
            *lpaddr= addrT;
            fFound = TRUE;
        }

    }

    // Rebuild the cache

    _fmemmove ( &rgbGPI [ ib - 1 ], &rgbGPI [ ibCache ], ibMax - ibCache );
    _fmemcpy  ( rgbGPI, rgb, ib - 1 );

    return xosd;
}

XOSD
GetPrevInst (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    )
{

    if ( GetAddrOff ( *lpaddr ) == 0 ) {

        return xosdBadAddress;
    }
    else if (
        hpid == hpidGPI &&
        GetAddrSeg ( *lpaddr ) == GetAddrSeg ( addrGPI ) &&
        GetAddrOff ( *lpaddr ) <  GetAddrOff ( addrGPI ) &&
        GetAddrOff ( *lpaddr ) >  GetAddrOff ( addrGPI ) - doffMax / 2
    ) {

        return GPIUseCache ( hpid, htid, lpaddr );
    }
    else {

        return GPIBuildCache ( hpid, htid, lpaddr );
    }
}


//
// Return xosdContinue if overlay is loaded
// Else return xosdNone
//
XOSD
FLoadedOverlay(
    HPID   hpid,
    LPADDR lpaddr
    )
{
    XOSD    xosd = xosdContinue;
    Unreferenced( hpid );
    Unreferenced( lpaddr );
    return xosd;
}



XOSD
SetupExecute(
    HPID       hpid,
    HTID       htid,
    LPHIND     lphind
    )
/*++

Routine Description:

    This function is used to set up a thread for doing function evaluation.
    The first thing it will do is to

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    HLLI                        hlli;
    HTHD                        hthd;
    HPRC                        hprc;
    LPTHD                       lpthd;
    LP_EXECUTE_OBJECT_EM        lpeo;
    XOSD                        xosd;

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }
    hthd = HthdFromHtid(hprc, htid);
    if (!hthd || hthd == hthdInvalid) {
        return xosdBadThread;
    }

    /*
     *  If the list of execute objects has not yet been setup then it
     *  needs to be setup now.
     */

    if (HllEo == 0) {
        HllEo = LLInit(sizeof(EXECUTE_OBJECT_EM), llfNull, NULL, NULL);
    }

    /*
     *  Allocate an execute object for this working item.
     */

    if ((hlli = LLCreate( HllEo )) == 0) {
        return xosdOutOfMemory;
    }
    lpeo = (LP_EXECUTE_OBJECT_EM) LLLock( hlli );
    lpeo->regs = MHAlloc(SizeOfContext(hpid));

    /*
     *  Ask the DM to allocate a handle on its end for its low level
     *  execute object.
     */

    xosd = SendRequest(dmfSetupExecute, hpid, htid );

    if (xosd != xosdNone) {
        LLUnlock( hlli );
        LLDelete( HllEo, hlli );
        return xosd;
    }

    lpeo->heoDm = *(HIND *) LpDmMsg->rgb;

    /*
     *  Get the current register set for the thread on which we are going
     *  to do the exeucte.
     */

    lpthd = (LPTHD) LLLock( hthd );

    lpeo->hthd = hthd;

    if (!( lpthd->drt & drtAllPresent )) {
        UpdateRegisters( hprc, hthd );
    }

    _fmemcpy( lpeo->regs, lpthd->regs, SizeOfContext(hpid));

    LLUnlock( hthd );

    /*
     *  Unlock the execute object and return its handle
     */

    LLUnlock( hlli );

    *lphind =  (HIND)hlli;

    return xosdNone;
}                               /* SetupExecute() */



XOSD
StartExecute(
    HPID       hpid,
    HIND       hind,
    LPEXECUTE_STRUCT lpes
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    XOSD                        xosd;
    LP_EXECUTE_OBJECT_EM        lpeo;
    HTHD                        hthd;
    HTID                        htid;
    HPRC                        hprc;

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }

    lpeo = (LP_EXECUTE_OBJECT_EM) LLLock( (HLLE)hind );
    hthd= lpeo->hthd;
    htid = HtidFromHthd(hthd),
    lpes->hindDm = lpeo->heoDm;
    FixupAddr(hpid, htid, &lpes->addr);
    LLUnlock( (HLLE)hind );

    /*
     *  Cause any changes to registers to be written back
     */

    UpdateChild(hpid, htid, dmfGo);

    /*
     *  Issue the command to the DM
     */

    xosd = SendRequestX(dmfStartExecute, hpid, htid,
                        sizeof(EXECUTE_STRUCT), lpes);


    return xosd;
}                               /* StartExecute() */



XOSD
CleanUpExecute(
    HPID hpid,
    HIND hind
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    LPTHD                       lpthd;
    LP_EXECUTE_OBJECT_EM        lpeo;

    lpeo = (LP_EXECUTE_OBJECT_EM) LLLock( (HLLE)hind );

    lpthd = (LPTHD) LLLock( lpeo->hthd );

    _fmemcpy( lpthd->regs, lpeo->regs, SizeOfContext(hpid));

    lpthd->drt = (DRT) (drtAllPresent | drtCntrlPresent | drtAllDirty | drtCntrlDirty);

    SendRequestX(dmfCleanUpExecute, hpid, HtidFromHthd(lpeo->hthd),
                 sizeof(HIND), &lpeo->heoDm);

    LLUnlock( (lpeo->hthd) );
    LLUnlock( (HLLE)hind );

    LLDelete( HllEo, (HLLE)hind );

    return xosdNone;

}                               /* CleanUpExecute() */


void
UpdateNLGStatus(
    HPID    hpid,
    HTID    htid
    )
{
    HPRC    hprc = HprcFromHpid (hpid);
    LPPRC   lpprc = (LPPRC)LLLock (hprc);
    HMDI    hmdi;
    XOSD    xosd = xosdNone;


    if (lpprc->dmi.fNonLocalGoto) {
        while ( hmdi = LLFind (lpprc->llmdi, NULL, NULL, emdiNLG )) {
            LPMDI   lpmdi = (LPMDI) LLLock ( hmdi );
            NLG     nlg = lpmdi->nlg;

            FixupAddr ( hpid, htid, &nlg.addrNLGDispatch );
            FixupAddr ( hpid, htid, &nlg.addrNLGDestination );
            FixupAddr ( hpid, htid, &nlg.addrNLGReturn );

            xosd = SendRequestX(dmfNonLocalGoto,
                                HpidFromHprc ( hprc ),
                                NULL,
                                sizeof ( nlg ),
                                &nlg
                                );

            lpmdi->fSendNLG = FALSE;
            LLUnlock ( hmdi );
        }
    }

    LLUnlock (hprc);
}



XOSD
EMFunc (
    EMF  emf,
    HPID hpid,
    HTID htid,
    DWORD64 wValue,
    DWORD64 lValue
    )

/*++

Routine Description:

    This is the main dispatch routine for processing of commands to the
    execution model.

Arguments:

    emf    - Supplies the function to be performed. (Execution Model Function )

    hpid   - Supplies The process to be used.

    htid   - Supplies The thread to be used.

    wValue - Supplies Info about the command

    lValue - Supplies Info about the command

Return Value:

    returns an XOSD error code

Other:

       Hpid and htid can never be invalid.  In some cases, they can be
       null.  The entries under P and T marked with an 0 indicate that
       the null value is valid for this function, an X indicates that
       it is invalid.

       Brief descriptions of the wValue and lValue


       EMF                 P   T   WVALUE          LVALUE

       emfGo               X   X   ----            ----
       emfShowDebuggee     X   0   ----            ----
       emfStop             X   0   ----            ----
       emfWriteBuf         X   0   #of bytes       pointer to buffer
       emfReadBuf          X   0   #of bytes       pointer to buffer
       emfSingleStep       X   X   ----            -----
       emfStepOver         X   X   ----            ----
       emfSetBreakPoint    X   X   ----            ----
       emfRemoveBreakPoint X   X   ----            ----
       emfSetWatchPoint    X   X   ----            ----
       emfRemoveWatchPoint X   X   ----            ----
       emfRangeStep,
       emfRangeOver,
       emfThreadStatus     X   X   ----            pointer to status buf.
       emfProcStatus       X   X   ----            pointer to status buf.
       emfFreeze           X   X   ----            ----
       emfThaw             X   X   ----            ----
       emfRegisterDBF      0   0   ----            pointer to dbf
       emfInit             0   0   ----            pointer to em serv.
       emfUnInit           0   0   ----            ----
       emfCreatePid        X   0   ----            ----
       emfDestroyPid       X   0   ----            ----
       emfDestroyTid       X   X   ----            ----
       emfDestroy          0   0   hem             ----
       emfIsValid          X   X   hem             ----
       emfSetAddr          X   X   ----            pointer to addr
       emfGetAddr          X   X   ----            pointer to addr
       emfRegValue         X   X   register index  pointer to buffer
       emfSetReg           X   X   register index  pointer to buffer
       emfSetFrameContext  X   X   frame
       emfFrameRegValue    X   X   register index  pointer to buffer
       emfFrameSetReg      X   X   register index  pointer to buffer
       emfSpawnOrphan      X   0
       emfProgramLoad      X   0   length          pntr to cmd line
       emfProgramFree      X   0   ----            ----
       emfDebugPacket      X   X   ----            pointer to buffer
       emfMetric           X   0   ----            pointer to metric
       emfUnassemble       X   X   ----            pointer to buffer
       emfAssemble         X   X   ----            pointer to buffer
       emfGetObjLength     X   X   ----            pointer to addr
       emfIOCTL            X   X   IOCTL type      pointer to data
       emfGetRegStruct     0   0   register index  pointer to buffer
       emfGetFlagStruct    0   0   flag index      pointer to buffer
       emfGetFlag          X   X   flag index      pointer to buffer
       emfSetFlag          X   X   flag index      pointer to data
       emfIsStackSetup     X   X   ----            pointer to addr
       emfCompareAddr      ?   ?   ----            pointerr to rglpaddr[2]
       emfSetupExecute     X   X   ----            pointer to handle
       emfStartExecute     X   -   Handle          pointer to execute_struct
       emfCleanUpExecute   X   0   Handle          -----
       emfLoadDllAck       X   0   ----            -----
       emfUnLoadDllAck     X   0   ----            pointer to MDI
       emfAttach           X   0   ----
       emfStackWalkSetup   X   X   PC In Prolog    pointer to stack walk data
       emfStackWalkNext    X   X   ----            pointer to stack walk data
       emfStackWalkCleanup X   X   ----            pointer to stack walk data
       emfDebugActive      X   0   ----            LPDBG_ACTIVE_STRUCT
       emfConnect          X   0   ----            ----
       emfDisconnect       X   0   ----            ----
       emfEnableCache      X   0   ----            ----
       emfGetMemInfo       X   0   sizeof MEMINFO  LPMEMINFO
       emfNewSymbolsLoaded X   0   ----            ----
       emfKernelLoaded     X   0   ----            ----

--*/

{
    XOSD xosd = xosdNone;


    switch ( emf ) {

    default:
        assert ( FALSE );
        xosd = xosdUnknown;
        break;

    case emfKernelLoaded:

        CallTL ( tlfReply, hpid, 0, 0 );
        xosd = xosdNone;
        break;

    case emfNewSymbolsLoaded:

        xosd = SendRequest ( dmfNewSymbolsLoaded, hpid, htid );
        break;

    case emfShowDebuggee :

        xosd = SendRequestX ( dmfSelect, hpid, htid, sizeof (wValue), &wValue );
        break;

    case emfStop:

        xosd = SendRequestX ( dmfStop, hpid, htid, sizeof(wValue), &wValue );
        break;

    case emfRegisterDBF:

        InitUsage ( );
        lpdbf = (LPDBF) lValue;
        break;

    case emfInit:

        llprc = LLInit ( sizeof ( PRC ), llfNull, PiDKill, PDComp );

        CallDB = ( (LPEMCB) lValue)->lpfnCallBackDB;
        CallTL = ( (LPEMCB) lValue)->lpfnCallBackTL;
        CallNT = ( (LPEMCB) lValue)->lpfnCallBackNT;

        LpDmMsg = (LPDM_MSG) MHAlloc ( CBBUFFERDEF );
        CbDmMsg = CBBUFFERDEF;
        CallTL ( tlfSetBuffer, hpid, CBBUFFERDEF, (DWORD64)LpDmMsg );

        break;

    case emfUnInit:

        /*
         * do any uninitialization for the EM itself
         */
        FreeEmErrorStrings ();

        CleanupDisassembler();
        MHFree(LpDmMsg);
        LpDmMsg = NULL;
        CbDmMsg = 0;

        break;

    case emfSetAddr:

        xosd = SetAddr( hpid, htid, (ADR) wValue, (LPADDR) lValue );
        break;

    case emfGetAddr:

        xosd = GetAddr( hpid, htid, (ADR) wValue, (LPADDR) lValue );
        break;

    case emfSpawnOrphan:

        xosd = SpawnOrphan ( hpid, (DWORD)wValue, (LPSOS) lValue);
        break;

    case emfProgramLoad:

        SyncHprcWithDM( hpid );
        xosd = ProgramLoad ( hpid, (DWORD)wValue, (LPPRL) lValue );
        break;

    case emfProgramFree:

        xosd = ProgramFree ( hpid, htid );
        break;

    case emfFixupAddr:

        xosd = FixupAddr ( hpid, htid, (LPADDR) lValue );
        break;

    case emfUnFixupAddr:

        xosd = UnFixupAddr ( hpid, htid, (LPADDR) lValue );
        break;

    case emfSetEmi:

        xosd = SetEmi ( hpid, (LPADDR) lValue );
        break;

    case emfMetric:

        xosd = DebugMetric ( hpid, htid, (DWORD)wValue, (LPLONG) lValue );
        break;

    case emfDebugPacket:
    {
        LPRTP lprtp = (LPRTP) lValue;

        xosd = DebugPacket(
                     lprtp->dbc,
                     lprtp->hpid,
                     lprtp->htid,
                     lprtp->cb,
                     (lprtp->cb == 0 ) ? NULL : (LPBYTE) lprtp->rgbVar
                     );
        break;
    }

    case emfSetMulti:

        xosd  = SendRequest ( (DMF) (wValue ? dmfSetMulti : dmfClearMulti),
                                                                  hpid, htid );
        break;

    case emfDebugger:

        xosd = SendRequestX ( dmfDebugger, hpid, htid, (DWORD)wValue, (LPVOID) lValue );
        break;

    case emfRegisterEmi:

        RegisterEmi ( hpid, htid, (LPREMI) lValue );
        break;

    case emfGetModel:
        *(WORD *)lValue = CEXM_MDL_native;
        break;

    case emfGetRegStruct:
        if (wValue >= CRgrd(hpid)) {
            xosd = xosdInvalidParameter;
        } else {
            *((RD FAR *) lValue) = Rgrd(hpid)[wValue];
        }
        break;

    case emfGetFlagStruct:

        if (wValue >= CRgfd(hpid)) {
            xosd = xosdInvalidParameter;
        } else {
            *((FD FAR *) lValue) = Rgfd(hpid)[wValue].fd;
        }
        break;

    case emfGetFlag:
        xosd = GetFlagValue( hpid, htid, (DWORD)wValue, (LPVOID) lValue );
        break;

    case emfSetFlag:
        xosd = SetFlagValue( hpid, htid, (DWORD)wValue, (LPVOID) lValue );
        break;

    case emfSetupExecute:
        xosd = SetupExecute(hpid, htid, (LPHIND) lValue);
        break;

    case emfStartExecute:
        xosd = StartExecute(hpid, (HIND) wValue, (LPEXECUTE_STRUCT) lValue);
        break;

    case emfCleanUpExecute:
        xosd = CleanUpExecute(hpid, (HIND) wValue);
        break;

    case emfSetPath:
        xosd = SetPath (hpid, htid, (DWORD)wValue, (LPTSTR)lValue);
        break;

    case emfGo:
        xosd = Go(hpid, htid, (LPEXOP)lValue);
        break;

    case emfSingleStep:
        xosd = SingleStep ( hpid, htid, (LPEXOP)lValue );
        break;

    case emfRangeStep:
        xosd = RangeStep(hpid, htid, (LPRSS)lValue);
        break;

    case emfReturnStep:
        xosd = ReturnStep(hpid, htid, (LPEXOP)lValue);
        break;

    case emfWriteMemory:
    {
        LPRWMS lprwms = (LPRWMS)lValue;
        xosd = WriteBufferCache ( hpid,
                                  htid,
                                  lprwms->lpaddr,
                                  lprwms->cbBuffer,
                                  (LPBYTE) lprwms->lpbBuffer,
                                  lprwms->lpcb
                                  );
        break;
    }

    case emfReadMemory:
    {
        LPRWMS lprwms = (LPRWMS)lValue;
        xosd = ReadBuffer ( hpid,
                            htid,
                            lprwms->lpaddr,
                            lprwms->cbBuffer,
                            (LPBYTE) lprwms->lpbBuffer,
                            lprwms->lpcb
                            );
        break;
    }

    case emfGetMemoryInfo:
        xosd = GetMemoryInfo(hpid, htid, (LPMEMINFO)lValue);
        break;

    case emfBreakPoint:
        xosd = HandleBreakpoints( hpid, (DWORD)wValue, (UINT_PTR)lValue );
        break;

    case emfProcessStatus:
        xosd = ProcessStatus(hpid, (LPPST)lValue);
        break;

    case emfThreadStatus:
        xosd = ThreadStatus(hpid, htid, (LPTST)lValue);
        break;

    case emfGetExceptionState:
        xosd = GetExceptionState(hpid, htid, (LPEXCEPTION_DESCRIPTION)lValue);
        break;

    case emfSetExceptionState:
        xosd = SetExceptionState(hpid, htid, (LPEXCEPTION_DESCRIPTION)lValue);
        break;

    case emfFreezeThread:
        xosd = FreezeThread(hpid, htid, (DWORD)wValue);
        break;

    case emfCreateHpid:
        xosd = CreateHprc ( hpid );

        if ( xosd == xosdNone ) {
            xosd = SendRequest ( dmfCreatePid, hpid, NULL );
            //
            //  We're allowed to have an HPID with no TL, so special-case
            //  the xosdInvalidParameter return code, which is returned
            //  by CallTL if there is no TL.
            //
            if ( xosd == xosdInvalidParameter ) {
                xosd = xosdNone;
            } else {
                SyncHprcWithDM( hpid );
            }
        }

        break;

    case emfDestroyHpid:
        {
            HPRC hprc = HprcFromHpid(hpid);
            xosd = SendRequest ( dmfDestroyPid, hpid, NULL );
            if ( xosd == xosdLineNotConnected || xosd == xosdInvalidParameter ) {
                //
                //  xosdLineNotConnected:  Communication line broke, we'll
                //      ignore this error.
                //
                //  xosdInvalidParameter:  We're allowed to have an HPID with
                //      no TL, so special case this return code, which is
                //      returned by CallTL if there is no TL.
                //
                xosd = xosdNone;
            }
            DestroyHprc ( hprc );
        }
        break;

    case emfDestroyHtid:
        {
            HPRC hprc = HprcFromHpid(hpid);
            DestroyHthd( HthdFromHtid( hprc, htid ));
        }
        break;

    case emfUnassemble:
        Disasm ( hpid, htid, (LPSDI) lValue );
        break;

    case emfAssemble:
        xosd = Assemble ( hpid, htid, (LPADDR) wValue, (LPTSTR) lValue );
        break;

    case emfGetReg:
        xosd = GetRegValue( hpid, htid, (DWORD)wValue, (LPVOID) lValue );
        break;

    case emfSetReg:
        xosd = SetRegValue( hpid, htid, (DWORD)wValue, (LPVOID) lValue );
        break;

    case emfDebugActive:
        xosd = SendRequestX(dmfDebugActive, hpid, htid, (DWORD)wValue, (LPVOID)lValue);
        if (xosd == xosdNone) {
            xosd = LpDmMsg->xosdRet;
        }
        SyncHprcWithDM( hpid );
        break;

    case emfGetMessageMap:
        *((LPMESSAGEMAP *)lValue) = &MessageMap;
        break;

    case emfGetMessageMaskMap:
        *((LPMASKMAP *)lValue) = &MaskMap;
        break;

    case emfGetModuleNameFromAddress:
        xosd = GetModuleNameFromAddress(hpid,
                                        wValue,
                                        (PTSTR) lValue
                                        );
        break;

    case emfGetModuleList:
        xosd = GetModuleList( hpid,
                              htid,
                              (LPTSTR)wValue,
                              (LPMODULE_LIST FAR *)lValue
                              );
        break;

    case emfCompareAddrs:
        xosd = CompareAddrs( hpid, htid, (LPCAS) lValue );
        break;

    case emfSetDebugMode:
#pragma message("Do something intelligent with emfSetDebugMode")
        xosd = xosdNone;
        break;

    case emfUnRegisterEmi:
        xosd = UnLoadFixups (hpid, (HEMI)lValue);
        break;

    case emfGetFrame:
        xosd = GetFrame(hpid, htid, (DWORD)wValue, (UINT_PTR)lValue);
        break;

    case emfGetObjLength:
        xosd = GetObjLength(hpid, (LPGOL)lValue);
        break;

    case emfGetFunctionInfo:
        xosd = GetFunctionInfo(hpid, (LPGFI)lValue);
        break;

    case emfGetPrevInst:
        xosd = BackDisasm(hpid, htid, (LPGPIS)lValue );
        break;

    case emfConnect:
        if (!LoadDmStruct.lpDmName) {
            LoadDmStruct.lpDmName = MHStrdup(DEFAULT_DMNAME);
        }
        if (!LoadDmStruct.lpDmParams) {
            LoadDmStruct.lpDmParams = MHStrdup(DEFAULT_DMPARAMS);
        }
        xosd = CallTL( tlfLoadDM, hpid, sizeof(LOADDMSTRUCT), (DWORD64)&LoadDmStruct);
        if (xosd == xosdNone) {
            xosd = SendRequest( dmfInit, hpid, htid );
        }
        break;

    case emfDisconnect:
        xosd = SendRequest( dmfUnInit, hpid, htid );
        break;

    case emfInfoReply:
        CallTL ( tlfReply, hpid, (DWORD)wValue, lValue );
        xosd = xosdNone;
        break;

    case emfSystemService:
        xosd = SystemService( hpid, htid, (DWORD)wValue, (LPSSS)lValue );
        break;

    case emfGetTimeStamp:
        xosd = GetTimeStamp (hpid, htid, (LPTCS) lValue);
        break;

    case emfSaveRegs:
        xosd = SaveRegs(hpid, htid, (LPHIND)lValue);
        break;

    case emfRestoreRegs:
        xosd = RestoreRegs(hpid, htid, (HIND)lValue);
        break;

    case emfSetup:

        {
            LPEMSS  lpemss = (LPEMSS)lValue;
            TCHAR   szModuleName[_MAX_PATH];
            TCHAR   szModuleParams[_MAX_PATH];

            if (lpemss->fLoad) {
                if (lpemss->pfnGetModuleInfo(szModuleName, szModuleParams)) {
                    if (LoadDmStruct.lpDmName) {
                        MHFree(LoadDmStruct.lpDmName);
                    }
                    if (LoadDmStruct.lpDmParams) {
                        MHFree(LoadDmStruct.lpDmParams);
                    }
                    LoadDmStruct.lpDmName = MHStrdup(szModuleName);
                    LoadDmStruct.lpDmParams = MHStrdup(szModuleParams);

                    xosd = xosdOutOfMemory;
                }
            }

            if (lpemss->fSave) {
                assert(0);
            }

        }
        break;

    }

    return xosd;
}                               /* EMFunc() */


/*
**
*/

int
WINAPI
DllMain(
    HINSTANCE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    Unreferenced(hModule);
    Unreferenced(dwReserved);

    switch (dwReason) {

      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
        break;

      case DLL_PROCESS_DETACH:

        if (LpSendBuf) {
            MHFree(LpSendBuf);
            CbSendBuf = 0;
        }
        if (LpDmMsg) {
            MHFree(LpDmMsg);
            CbDmMsg = 0;
        }

        CleanupDisassembler();

        DeleteCriticalSection(&csCache);
        break;

      case DLL_PROCESS_ATTACH:

        InitializeCriticalSection(&csCache);
                DisableThreadLibraryCalls(hModule);
        break;
    }

    return TRUE;
}



XOSD
DebugPacket (
    DBC dbc,
    HPID hpid,
    HTID htid,
    DWORD wValue,
    LPBYTE lpb
    )
{
    XOSD        xosd = xosdContinue;
    HPRC        hprc = HprcFromHpid ( hpid );
    HTHD        hthd = HthdFromHtid ( hprc, htid );
    LONG        emdi;
    LPTHD       lpthd;
    LPPRC       lpprc;
    DWORD64     wNewValue = wValue;

    if (hthd) {
        lpthd = (LPTHD) LLLock(hthd);
        lpthd->drt = drtNonePresent;
        LLUnlock(hthd);
    }


    /* Do any preprocessing on the packet before sending the notification
     * on to the debugger.  For example, the wValue and lValue might need
     * some munging.  Also, if the notification shouldn't be passed on to
     * the debugger, then set xosd = xosdNone or some other value other
     * than xosdContinue.
     */

    switch ( dbc ) {
    case dbceAssignPID:
        {
            LPPRC lpprc = (LPPRC) LLLock ( hprc );

            assert ( wValue == sizeof ( PID ) );
            lpprc->pid = *( (PID FAR *) lpb );
            lpprc->stat = statStarted;
            LLUnlock ( hprc );
        }
        xosd = xosdNone;
        break;

    case dbcCreateThread:
        lpprc = (LPPRC) LLLock(hprc);
        lpprc->fRunning = FALSE;
        LLUnlock(hprc);

        assert ( wValue == sizeof ( TCR ) );
        xosd = CreateThreadStruct ( hpid, *( (TID FAR *) lpb ), &htid );
        if ( ((LPTCR) lpb)->uoffTEB != 0 ) {
            HTHD  hthdT = HthdFromHtid ( hprc, htid );
            LPTHD lpthd = (LPTHD) LLLock ( hthdT );

            lpthd->uoffTEB = ((LPTCR) lpb)->uoffTEB;
            LLUnlock ( hthdT );
        }

        CallTL ( tlfReply, hpid, sizeof ( HTID ), (DWORD64)&htid );
        if ( xosd == xosdNone ) {
            xosd = xosdContinue;
        }
        break;

    case dbcNewProc:
        {
            HPRC  hprcT;
            HPID  hpidT;
            LPPRC lpprc;
            LPNPP lpnpp;

            /*
             * lpb points to an NPP (New Process Packet).  The PID is
             * the PID of the debuggee; fReallyNew indicates if this is
             * really a new process or if it already existed but hasn't
             * been seen before by OSDebug.
             */

            assert ( wValue == sizeof(NPP) );
            lpnpp = (LPNPP) lpb;

            // See EMCallBackDB in od.c

            CallDB ( dbcoNewProc,
                     hpid,
                     htid,
                     CEXM_MDL_native,
                     sizeof ( HPID ),
                     (DWORD64)&hpidT
                     );

            (void) CreateHprc ( hpidT );

            hprcT       = HprcFromHpid ( hpidT );
            lpprc       = (LPPRC) LLLock ( hprcT );
            lpprc->pid  = lpnpp->pid;
            lpprc->stat = statStarted;
            LLUnlock ( hprcT );

            CallTL ( tlfReply, hpid, sizeof ( HPID ), (DWORD64)&hpidT );

            SyncHprcWithDM( hpidT );

            wNewValue = (LPARAM)hpidT;
            // BUGBUG: why is a bool value being assigned to a pointer variable?
            // ANSWERANSWER: because the VC people were lazy.  It ends up in lParam
            //               in OSDCallbackFunc.
            lpb = (LPBYTE) (LONG) lpnpp->fReallyNew;
            if ( xosd == xosdNone ) {
                xosd = xosdContinue;
            }
        }
        break;


    case dbcThreadTerm:
        lpprc = (LPPRC) LLLock(hprc);
        lpprc->fRunning = FALSE;
        LLUnlock(hprc);
        lpthd = (LPTHD) LLLock(hthd);
        lpthd->fRunning = FALSE;
        LLUnlock(hthd);

    case dbcProcTerm:

        /*
         * For both of these notifications, the incoming wValue is
         * sizeof(ULONG), and lpb contains a ULONG which is the exit
         * code of the process or thread.  For the debugger, set
         * wValue = 0 and lValue = exit code.
         */

        assert ( wValue == sizeof(ULONG) );
        wNewValue = 0;
        lpb = (LPBYTE) (*(ULONG*)lpb);
        break;

    case dbcDeleteThread:

        lpthd = (LPTHD) LLLock(hthd);
        lpthd->tid    = (TID)-1;
        LLUnlock(hthd);

        assert ( wValue == sizeof(ULONG) );
        wNewValue = 0;
        lpb = (LPBYTE) (*(ULONG*)lpb);
        break;

    case dbcModLoad:
        lpprc = (LPPRC) LLLock(hprc);
        lpprc->fRunning = FALSE;
        LLUnlock(hprc);
        xosd = LoadFixups ( hpid, htid, (LPMODULELOAD) lpb );
        break;

    case dbcModFree:            /* Should use dbceModFree*               */
        assert(FALSE);
        break;

    case dbceModFree32:
        emdi = emdiBaseAddr;
    modFree:
        {
            HMDI    hmdi;
            LPMDI   lpmdi;
            HLLI    llmdi;

            llmdi = LlmdiFromHprc ( hprc );
            assert( llmdi );

            hmdi = LLFind( llmdi, 0, lpb, emdi);
            assert( hmdi );

            lpmdi = (LPMDI) LLLock( hmdi );
            lpb = (LPBYTE) lpmdi->hemi;
            LLUnlock( hmdi );
            LLUnlock(hprc);

            dbc = dbcModFree;
        }
        break;

    case dbceModFree16:
        emdi = emdiMTE;
        goto modFree;
        break;

    case dbcExecuteDone:
        lpprc = (LPPRC) LLLock(hprc);
        lpprc->fRunning = FALSE;
        LLUnlock(hprc);
        lpthd = (LPTHD) LLLock(hthd);
        lpthd->fRunning = FALSE;
        LLUnlock(hthd);
        break;

    case dbcStep:

    case dbcThreadBlocked:
    case dbcSignal:
    case dbcAsyncStop:
    case dbcBpt:
    case dbcEntryPoint:
    case dbcLoadComplete:
    case dbcCheckBpt:
        {
            LPBPR lpbpr = (LPBPR) lpb;
            LPTHD lpthd = (LPTHD) LLLock ( hthd );

            assert ( wValue == sizeof ( BPR ) );

            PurgeCache ( );
            lpprc = (LPPRC) LLLock(hprc);
            if (dbc != dbcCheckBpt) {
                lpprc->fRunning = FALSE;
                lpthd->fRunning = FALSE;
            }
            LLUnlock(hprc);

            CopyFrameRegs(hpid, lpthd, lpbpr);

            lpthd->fFlat         = lpbpr->fFlat;
            lpthd->fOff32        = lpbpr->fOff32;
            lpthd->fReal         = lpbpr->fReal;

            lpthd->drt = drtCntrlPresent;

            LLUnlock( hthd );

            wNewValue = lpbpr->qwNotify;
            lpb = NULL;
        }
        break;

    case dbcException:
        {
            LPEPR lpepr = (LPEPR) lpb;
            LPTHD lpthd = (LPTHD) LLLock ( hthd );
            ADDR  addr  = {0};

#if 0
            /*
              * This would be true if we did not pass parameters up
              */
            assert ( wValue == sizeof ( EPR ) );
#endif

            PurgeCache ( );
            lpprc = (LPPRC) LLLock(hprc);
            lpprc->fRunning = FALSE;
            LLUnlock(hprc);
            lpthd->fRunning = FALSE;


            CopyFrameRegs(hpid, lpthd, &lpepr->bpr);

            lpthd->fFlat        = lpepr->bpr.fFlat;
            lpthd->fOff32       = lpepr->bpr.fOff32;
            lpthd->fReal        = lpepr->bpr.fReal;

            lpthd->drt = drtCntrlPresent;

            LLUnlock( hthd );
        }
        break;

    case dbceCheckBpt:
        assert(FALSE);
        xosd = xosdNone;
        break;


    case dbcError:
        {
            static TCHAR sz[500];
            XOSD    xosdErr = *( (XOSD *)lpb );
            LPTSTR  str = (LPTSTR) (lpb + sizeof(XOSD));

            if (str[0]) {
                lpb = (LPBYTE) str;
            } else {
                _stprintf(sz, _T("DM%04d: %s"), xosdErr, EmError(xosdErr));
                lpb = (LPBYTE) sz;
            }
            wNewValue = xosdErr;
        }
        break;

    case dbceSegLoad:
        {
            SLI     sli;
            HMDI    hmdi;
            LPMDI   lpmdi;
            UINT    i;

            sli = *( (LPSLI) lpb );

            hmdi = LLFind( LlmdiFromHprc( hprc ), 0, &sli.mte,
                          (LONG) emdiMTE);

            assert( hmdi );

            lpmdi = (LPMDI) LLLock(hmdi );

            if (sli.wSegNo >= lpmdi->cobj) {
                i = lpmdi->cobj;
                lpmdi->cobj = sli.wSegNo+1;
                lpmdi->rgobjd = (OBJD *) MHRealloc(lpmdi->rgobjd,
                                        sizeof(OBJD)*lpmdi->cobj);
                memset(&lpmdi->rgobjd[i], 0, sizeof(OBJD)*(lpmdi->cobj - i));
            }
            lpmdi->rgobjd[ sli.wSegNo ].wSel = sli.wSelector;
            lpmdi->rgobjd[ sli.wSegNo ].wPad = 1;
            lpmdi->rgobjd[ sli.wSegNo ].cb = (DWORD) -1;

            LLUnlock( hmdi );

            //
            //  Let the shell know that a new segment was loaded, so it
            //  can try to instantiate virtual BPs.
            //
            xosd = CallDB( dbcSegLoad,
                           hpid,
                           htid,
                           CEXM_MDL_native,
                           0,
                           sli.wSelector
                           );
            xosd=xosdNone;

        }
        break;

    case dbceSegMove:
        {
            SLI     sli;
            HMDI    hmdi;
            LPMDI   lpmdi;

            sli = *( (LPSLI) lpb );

            hmdi = LLFind( LlmdiFromHprc( hprc ), 0, &sli.mte,
                          (LONG) emdiMTE);

            assert( hmdi );

            lpmdi = (LPMDI) LLLock(hmdi );

            assert(sli.wSegNo > 0 );
            if (sli.wSegNo < lpmdi->cobj) {
                lpmdi->rgobjd[ sli.wSegNo - 1 ].wSel = sli.wSelector;
            }

            LLUnlock( hmdi );
        }
        break;

    case dbcCanStep:
        {
            CANSTEP CanStep;
            ADDR origAddr;

            assert ( wValue == sizeof ( ADDR ) );

            UnFixupAddr( hpid, htid, (LPADDR) lpb);
            origAddr = *(LPADDR)lpb;

            xosd = CallDB(dbc,
                          hpid,
                          htid,
                          CEXM_MDL_native,
                          0,
                          (DWORD64)lpb
                          );

            if ( xosd != xosdNone ) {
                CanStep.Flags = CANSTEP_NO;
            } else {
                CanStep = *((CANSTEP*)lpb);
                if (CanStep.Flags == CANSTEP_YES) {
                    AdjustForProlog(hpid, htid, &origAddr, &CanStep);
                }
            }

            CallTL ( tlfReply, hpid, sizeof( CanStep ), (DWORD64)&CanStep );

            xosd = xosdNone;
        }
        break;

    case dbceGetOffsetFromSymbol:
        {
            ADDR addr = {0};
            xosd = CallDB(dbcGetExpression, hpid, htid, CEXM_MDL_native, (LPARAM)&addr, (DWORD64)lpb);
            if (xosd == xosdNone) {
                FixupAddr(hpid, htid, &addr);
            }
            CallTL( tlfReply, hpid, sizeof(addr.addr.off), (DWORD64)&addr.addr.off );
            xosd = xosdNone;
        }

        break;

    case dbceGetSymbolFromOffset:
        {
            LPTSTR p;
#if defined(UNICODE) || defined(_UNICODE)
#pragma message("SHAddrToPublicName needs UNICODE api")
#endif
#ifdef NT_BUILD_ONLY
            ADDR addr;
            LPTSTR fname = (LPTSTR) SHAddrToPublicName( (LPADDR)lpb, &addr );
#else
            LPTSTR fname = (LPTSTR) SHAddrToPublicName( (LPADDR)lpb );
#endif
            HTID vhtid;

            GetFrame( hpid, htid, 1, (DWORD_PTR)&vhtid );
            lpthd = (LPTHD) LLLock(hthd);
            if (fname) {
                p = (LPTSTR) MHAlloc( (_ftcslen(fname) + 16) * sizeof(TCHAR) );
                _ftcscpy(p,fname);
                MHFree(fname);
            } else {
                p = (LPTSTR) MHAlloc( 32 );
                _stprintf( p, _T("<unknown>0x%016x"), GetAddrOff(*(LPADDR)lpb) );
            }
            fname = p;
            p += _ftcslen(p) + 1;
            *(UNALIGNED PDWORD64)p = (DWORD64)lpthd->StackFrame.AddrReturn.Offset;
            LLUnlock(hthd);
            CallTL( tlfReply, hpid, (_ftcslen(fname)+1)*sizeof(TCHAR)+sizeof(DWORD64), (DWORD64)fname );
            MHFree( fname );
            xosd = xosdNone;
        }
        break;

    case dbceEnableCache:
        EnableCache( hpid, htid, *(LPDWORD)lpb );
        CallTL( tlfReply, hpid, 0, 0 );
        xosd = xosdNone;
        break;

    case dbceGetMessageMask:
        {
            DWORD dwMsgMask = GetMessageMask( *(LPDWORD)lpb );
            CallTL( tlfReply, hpid, sizeof(DWORD), (DWORD64)&dwMsgMask);
            xosd = xosdNone;
        }
        break;

    case dbcLastAddr:
        assert( wValue == sizeof( ADDR ) );

        UnFixupAddr( hpid, htid, (LPADDR) lpb );

        xosd = CallDB(dbc, hpid, htid, CEXM_MDL_native, 0, (DWORD64) lpb);

        if ( xosd == xosdNone ) {
            FixupAddr( hpid, htid, (LPADDR) lpb );
        }

        CallTL( tlfReply, hpid, sizeof(ADDR), (DWORD64)lpb);
        break;

    case dbceExceptionDuringStep:
        {
            DWORD cAddrsAllocated = 6, cbPacket;
            HTID vhtid = htid;
            LPEXHDLR lpexhdlr = (LPEXHDLR) MHAlloc(sizeof(EXHDLR)+sizeof(ADDR)*cAddrsAllocated);
            lpexhdlr->count = 0;
            GetFrameEH(hpid, htid, &lpexhdlr, &cAddrsAllocated);
            while (GetFrame(hpid, vhtid, 1, (DWORD_PTR)&vhtid)==xosdNone) {
                GetFrameEH(hpid, vhtid, &lpexhdlr, &cAddrsAllocated);
            }
            cbPacket = sizeof(EXHDLR)+(lpexhdlr->count * sizeof(ADDR));
            CallTL( tlfReply, hpid, cbPacket, (DWORD64) lpexhdlr);
            MHFree(lpexhdlr);
            xosd = xosdNone;
        }
        break;

    case dbceGetFunctionInformation:
        {
            GFI gfi;
            FUNCTION_INFORMATION FunctionInformation;
            gfi.lpaddr = (LPADDR)lpb;
            gfi.lpFunctionInformation = &FunctionInformation;
            xosd = GetFunctionInfo(hpid, &gfi);

            if (xosd != xosdNone) {
                ZeroMemory(&FunctionInformation, sizeof(FUNCTION_INFORMATION));
            }
            CallTL( tlfReply,
                    hpid,
                    sizeof(FUNCTION_INFORMATION),
                    (DWORD64)&FunctionInformation
                  );
        }
        break;

    default:
        break;
    }

    if ((xosd == xosdContinue) && (dbc < dbcMax) && (dbc != dbcModLoad)) {
        xosd = CallDB ( dbc, hpid, htid, CEXM_MDL_native, wNewValue, (DWORD64)lpb );
    }

    switch ( dbc ) {

    case dbcProcTerm:
        {
            LPPRC lpprc = (LPPRC) LLLock ( hprc );
            lpprc->stat = statDead;
            LLUnlock ( hprc );
        }
        break;

    case dbcThreadTerm:
        {
            LPTHD lpthd = (LPTHD) LLLock ( hthd );
            lpthd->fVirtual = TRUE;
            LLUnlock ( hthd );
        }
        break;

    case dbcDeleteProc:
        break;

    case dbcDeleteThread:
        break;

    case dbcCheckBpt:
    case dbcCheckWatchPoint:
    case dbcCheckMsgBpt:
        {
            DWORD wContinue = xosd;
            xosd = xosdNone;
            CallTL( tlfReply, hpid, sizeof(wContinue), (DWORD64)&wContinue);
        }
    }

    return xosd;
}                               /* DebugPacket() */


