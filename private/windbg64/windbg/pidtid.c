/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pidtid.c

Abstract:


Author:

    Griffith Wm. Kadnier (v-griffk)

Environment:

    Win32 - User

--*/


#include "precomp.h"
#pragma hdrstop


/****************************** DATA **************************************/

static LPPD LppdHead = NULL;
static int  ipid = 0;

/****************************** CODE **************************************/

VOID
SetIpid(
    int i
    )
{
    ipid = i;
}

VOID
RecycleIpid1(
    void
    )
{
    if (LppdHead
        && LppdHead->pstate != psDestroyed
        && !LppdHead->lppdNext
        && ipid == 2)
    {
        ipid = 1;
    }
}

LPPD
GetLppdHead(
    void
    )
/*++

Routine Description:

    Retrieves head of LPPD chain.

Arguments:

    None.

Return Value:

    Pointer to first PD in chain

--*/
{
    return LppdHead;
}


LPPD
CreatePd(
    HPID hpid
    )
/*++

Routine Description:

    This function will create a process descriptor block, fill in
    the defaults for the fields and return a pointer to the descriptor
    block.  No attempts are made to verify that the hpid (handle to
    process identifier) passed in is correct in any way.

Arguments:

    hpid    - Handle to process in the OSDebug world

Return Value:

    NULL on failure or a pointer to a PD structure

--*/
{
    LPPD        lppd;
    LPPD        lppdT;

    /*
    **  first allocate and verify the allocation succeeded
    */

    lppd = (LPPD) malloc(sizeof(PD));
    if (lppd == NULL) {
        return lppd;
    }
    memset(lppd, 0, sizeof(PD));

    /*
    **  Now fill in the default set of values for the structure
    **
    **          hpid == passed in value
    **          state == no program has yet been loaded here.
    **          lptdNext == No threads currently linked on this process.
    */

    lppd->hpid = hpid;
    lppd->pstate = psNoProgLoaded;
    lppd->lptdList = NULL;
    lppd->ctid = 0;
    lppd->ipid = ipid++;
    lppd->exceptionList=NULL;
    lppd->lpBaseExeName = NULL;
    lppd->fPrecious = FALSE;
    lppd->fChild = FALSE;
    lppd->fFrozen = FALSE;

    /*
    **  Now append it to the list of process descriptors
    */

    lppd->lppdNext = NULL;
    lppdT = LppdHead;
    if (lppdT == NULL) {
        LppdHead = lppd;
    } else {
        while (lppdT->lppdNext) {
            lppdT = lppdT->lppdNext;
        }
        lppdT->lppdNext = lppd;
    }

    /*
    **  Return the pointer to the process descriptor block allocated
    */

    return(lppd);
}                                       /* CreatePd() */


VOID
DestroyPd(
    LPPD lppd,
    BOOL fDestroyPrecious
    )
/*++

Routine Description:

    Remove a PD from the list and free it.

Arguments:

    lppd  - Supplies pointer to PD to be destroyed

Return Value:

    None.

--*/
{
    LPPD *lplppd;

    /*
     * Remove it from the chain
     */
    for (lplppd = &LppdHead; *lplppd; lplppd = &((*lplppd)->lppdNext) ) {
        if (*lplppd == lppd) {
            if (lppd->lpBaseExeName) {
                free(lppd->lpBaseExeName);
                lppd->lpBaseExeName = NULL;
            }
            if (lppd->fPrecious && !fDestroyPrecious) {
                lppd->pstate = psDestroyed;
            } else {
                *lplppd = lppd->lppdNext;
                free(lppd);
            }
            break;
        }
    }
}                                       /* DestroyPd() */


/***    LppdOfHpid
**
**  Synopsis:
**      lppd = LppdOfHpid(hpid)
**
**  Entry:
**      hpid - Process id handle to be found
**
**  Returns:
**      pointer to PD if hpid is found and NULL otherwise
**
**  Description:
**      This routine looks down the list of PDS looking for the specific
**      hpid.  If it is found then a pointer to that pd will be returned
**      otherwise NULL is returned
*/

LPPD
LppdOfHpid(
    HPID hpid
    )
{
    LPPD        lppd = LppdHead;

    while (lppd != NULL) {
        if (lppd->hpid == hpid) {
            break;
        }
        lppd = lppd->lppdNext;
    }

    return(lppd);
}                                       /* LppdOfHpid() */


/***    LppdOfIpid
**
**  Synopsis:
**      lppd = LppdOfIpid(ipid)
**
**  Entry:
**      ipid - Internal index number of the process
**
**  Returns:
**      Pointer to process structure if one exists else NULL
**
**  Description:
**
*/

LPPD
LppdOfIpid(
    UINT ipid
    )
{
    LPPD        lppd = LppdHead;

    while (lppd != NULL) {
        if (lppd->ipid == ipid) {
            break;
        }

        lppd = lppd->lppdNext;
    }

    return lppd;
}                                       /* LppdOfIpid() */


LPPD
ValidLppdOfIpid(
    UINT ipid
    )
/*++

Routine Description:

    Look up LPPD by its index #, ignoring destroyed lppd's

Arguments:

    ipid    - ordinal number of process

Return Value:

    LPPD or NULL

--*/
{
    LPPD lppd;

    for (lppd = LppdHead; lppd != NULL; lppd = lppd->lppdNext) {
        if (lppd->pstate != psDestroyed && lppd->ipid == ipid) {
            break;
        }
    }
    return lppd;
}


BOOL
GetFirstValidPDTD(
    LPPD *plppd,
    LPTD *plptd
    )
/*++

Routine Description:

    Find first non-destroyed LPPD/LPTD pair in list

Arguments:

    plppd   - returns lppd
    plptd   - returns lptd

Return Value:

    TRUE for success, FALSE of none found.  Values pointed to by
    args are not modified if search fails.

--*/
{
    LPPD lppd;

    for (lppd = LppdHead; lppd; lppd = lppd->lppdNext) {
        if (lppd->pstate != psDestroyed && lppd->lptdList) {
            *plppd = lppd;
            *plptd = lppd->lptdList;
            return TRUE;
        }
    }
    return FALSE;
}


/***    CreateTd
**
**  Synopsis:
**      lptd = CreateTd(lppd, htid)
**
**  Entry:
**      lppd - Pointer to process descriptor block to create thread in
**      htid - OSDebug thread handle to be added to process
**
**  Returns:
**      pointer to thread descriptor block on success, NULL on failure
**
**  Description:
**      This function will create a thread descriptor block for the
**      osdebug handle to a thread.  It will be added to the list of
**      threads for the specified process descriptor block.
*/


LPTD
CreateTd(
    LPPD lppd,
    HTID htid
    )
{
    LPTD            lptd;
    LPTD            lptd2;


    /*
    **  First allocate and verify success
    */

    lptd = (LPTD) malloc(sizeof(TD));
    if (lptd == NULL)
          return lptd;
    memset(lptd, 0, sizeof(TD));

    /*
    ** Now fill in the default set of values for a thread descriptor
    **
    */

    lptd->htid = htid;
    lptd->lppd = lppd;
    lptd->tstate = tsPreRunning;
    lptd->itid = lppd->ctid++;

    /*
    **  Now insert it in the thread list for the process
    */

    lptd->lptdNext = NULL;
    lptd2 = lppd->lptdList;
    if (lptd2 == NULL) {
        lppd->lptdList = lptd;
    } else {
        while (lptd2->lptdNext != NULL) {
            lptd2 = lptd2->lptdNext;
        }
        lptd2->lptdNext = lptd;
    }

    /*
    **
    */

    return lptd;
}                                       /* CreateTd() */


VOID
SetTdInfo(
    LPTD lptd
    )
{
// BUGBUG need portable structures here
    BYTE            buf[sizeof(IOCTLGENERIC)+sizeof(GET_TEB_ADDRESS)];
    PIOCTLGENERIC   pig = (PIOCTLGENERIC)&buf[0];
    PGET_TEB_ADDRESS Gta = (PGET_TEB_ADDRESS)pig->data;
    LPTD            lptdSave;
    DWORD           dw;


    if (g_contWorkspace_WkSp.m_bKernelDebugger) {
        lptd->lpszTname = NULL;
        lptd->TebBaseAddress = 0;
    } else {
        if (lptd->TebBaseAddress == 0) {
            lptdSave = LptdCur;
            LptdCur = lptd;
            pig->ioctlSubType = IG_GET_TEB_ADDRESS;
            pig->length = sizeof(GET_TEB_ADDRESS);
            OSDSystemService( LppdCur->hpid,
                              lptd->htid,
                              (SSVC) ssvcGeneric,
                              (LPV)pig,
                              pig->length + sizeof(IOCTLGENERIC),
                              &dw
                              );
            lptd->TebBaseAddress = Gta->Address;
            lptd->lpszTname = GetLastFrameFuncName();
            LptdCur = lptdSave;
        }
    }
}

VOID
SetPdInfo(
    LPPD lppd
    )
{
    // BUGBUG Need portable structures here
    BYTE            buf[sizeof(IOCTLGENERIC)+sizeof(RTL_USER_PROCESS_PARAMETERS64)];
    PIOCTLGENERIC   pig = (PIOCTLGENERIC)&buf[0];
    PRTL_USER_PROCESS_PARAMETERS64 Upp = (PRTL_USER_PROCESS_PARAMETERS64)pig->data;
    DWORD           dw;
    XOSD            xosd;

    pig->ioctlSubType = IG_GET_PROCESS_PARAMETERS;
    pig->length = sizeof(RTL_USER_PROCESS_PARAMETERS64);

    OSDGetDebugMetric( lppd->hpid,
                       0,
                       mtrcProcessorType,
                       &lppd->mptProcessorType
                       );

    if (!g_contWorkspace_WkSp.m_bKernelDebugger) {
        xosd = OSDSystemService( lppd->hpid,
                                 NULL,
                                 (SSVC) ssvcGeneric,
                                 (LPV)pig,
                                 pig->length + sizeof(IOCTLGENERIC),
                                 &dw
                                 );

        if (xosd == xosdNone) {
            memcpy(&lppd->ProcessParameters, Upp, sizeof(RTL_USER_PROCESS_PARAMETERS64));
        }
    }
}



/***    DestroyTd
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

VOID
DestroyTd(
    LPTD lptd
    )
{
    LPPD        lppd = lptd->lppd;
    LPTD        lptdT;

    if (lppd->lptdList == lptd) {
        lppd->lptdList = lptd->lptdNext;
    } else {
        lptdT = lppd->lptdList;
        while (lptdT->lptdNext != lptd) {
            lptdT = lptdT->lptdNext;
            Assert(lptdT != NULL);
        }
        lptdT->lptdNext = lptd->lptdNext;
    }

    free(lptd);
    return;
}                                       /* DestroyTd() */

/***    LptdOfLppdHtid
**
**  Synopsis:
**      lppd = LptdOfLppdHtid(lppd, htid)
**
**  Entry:
**      lppd - Process descripter table to look in for thread descriptor
**      htid - thread id handle to be found
**
**  Returns:
**      pointer to TD if htid is found and NULL otherwise
**
**  Description:
**      This routine looks down the list of TDs looking for the specific
**      htid.  If it is found then a pointer to that td will be returned
**      otherwise NULL is returned
*/

LPTD
LptdOfLppdHtid(
    LPPD lppd,
    HTID htid
    )
{
    LPTD    lptd;

    if (!lppd) {
        return NULL;
    }

    lptd = lppd->lptdList;

    while (lptd != NULL) {
        if (lptd->htid == htid) {
            return lptd;
        }
        lptd = lptd->lptdNext;
    }

    return(lptd);
}                                       /* LptdOfLppdHtid() */


/***    LptdOfLppdItid
**
**  Synopsis:
**      lptd = LptdOfLppdItid(lppd, itid)
**
**  Entry:
**      lppd - pointer to process
**      itid - index of thread
**
**  Returns:
**      pointer to thread if found else NULL
**
**  Description:
**      This function will locate a thread within the given process which
**      has the correct thread id.  This thread id is local to the shell
**      of the debugger and is not the same as a Htid.  If no such thread
**      is found then NULL will be returned
*/

LPTD
LptdOfLppdItid(
    LPPD lppd,
    UINT itid
    )
{
    LPTD        lptd = lppd->lptdList;

    while (lptd != NULL) {
        if (lptd->itid == itid)
            return lptd;
        lptd = lptd->lptdNext;
    }

    return lptd;
}                                       /* LptdOfLppdItid() */
