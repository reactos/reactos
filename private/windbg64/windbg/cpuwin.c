/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    cpuwin.c

Abstract:

    This module contains the routines to manipulate the CPU Window

Author:

    William J. Heaton (v-willhe) 20-Jul-1992
    Griffith Wm. Kadnier (v-griffk) 10-Mar-1993

Environment:

    Win32, User Mode

--*/


/*
 *  Preprocessor
 */

#include "precomp.h"
#pragma hdrstop


#define EACOUNT  4  //for effective address display offsets

/*
 *  Global Memory (PROGRAM)
 */

extern  CXF     CxfIp; // for EA calcs v-griffk

/*
 *  Global Memory (FILE)
 */

static HWND     hWndCpu;
static PREGINFO pCpu;
static UINT     CpuCnt = 0;

static HWND     hWndFloat;
static PREGINFO pFloat;
static UINT     FloatCnt = 0;

static char   szValue[1024];








/*
 *  Prototypes
 */

VOID     AgeCpuValues( PREGINFO pReg, UINT nCnt);
PREGINFO CpuInitRegs(RT rtType, FT ftType, BOOL fFlagsAppend, UINT *cnt);
BOOL     CpuVerifyNew( PREGINFO pReg, UINT oln, UINT count);
PSTR     GetCpuString( PREGINFO pInfo, UINT nCnt, UINT PanelNumber, UINT Index);
PSTR     GetCpuValue( PREGINFO pInfo, UINT n);
PSTR     GetEA( UINT n);
BOOL     CPUSetValue( PPANE p, PREGINFO pReg );

/*
 *  Start of Code
 */

/***    GetFloatHWND
**
**  Synopsis:
**      hWnd = GetFloatHWND()
**
**  Entry:
**      None
**
**  Returns:
**      Pointer to the current Float window handle.
**
*/

HWND
GetFloatHWND(
    VOID
    )
{
#if defined( NEW_WINDOWING_CODE )
    return(g_DebuggerWindows.hwndFloat);
#else
    return(hWndFloat);
#endif
}


/***    GetCpuHWND
**
**  Synopsis:
**      hWnd = GetCpuHWND()
**
**  Entry:
**      None
**
**  Returns:
**      Pointer to the current Register window handle.
**
*/

HWND
GetCpuHWND(
    VOID
    )
{
#if defined( NEW_WINDOWING_CODE )
    return(g_DebuggerWindows.hwndCpu);
#else
    return(hWndCpu);
#endif
}


/***    CPUSetValue
**
**  Synopsis:
**      BOOL CPUSetValue( PPANE p )
**
**  Entry:
**      p  - Pane Information
**
**  Returns:
**      Pointer to the current Register window handle.
**
*/


BOOL
CPUSetValue(
    PPANE p,
    PREGINFO pReg
    )
{
    UINT iReg = p->CurIdx;

    BYTE        lpb[10];
    //BYTE        lpb[8];
    UCHAR       cNat;

    /*
    **  If we're not editing or not in the right pane
    **  its a no-op (a successful no-op)
    */

    if ( p->nCtrlId != ID_PANE_RIGHT) {
        return(TRUE);
    }
    if ( !p->Edit ) {
        return(TRUE);
    }

    //v-vadimp - strip the NAT bit
    if ((LppdCur->mptProcessorType == mptia64) && (pReg[iReg].type & fmtNat)  && (p->EditBuf[strlen(p->EditBuf)-2] == ' ')) 
    {
        cNat = p->EditBuf[strlen(p->EditBuf)-1];
        p->EditBuf[strlen(p->EditBuf)-2] = '\0';
    }

    /*
    **  Convert the character buffer into a byte buffer
    */

    if (CPUnformatMemory(lpb, p->EditBuf, pReg[iReg].cbits, pReg[iReg].type, 16) != EENOERROR) {
        return(FALSE);
    }

    /*
    **  Write back the register to the CPU
    */

    if (pReg[iReg].hFlag == -1) {
        OSDWriteRegister(LppdCur->hpid,
                         LptdCur->htid,
                         pReg[iReg].hReg,
                         lpb);
 
     // v-vadimp - write up the NAT bit
     if((LppdCur->mptProcessorType == mptia64) && (pReg[iReg].type & fmtNat)) {
        UINT NatReg, NatBit;
        ULONGLONG NatRegValue;
        if (pReg[iReg].hReg >= CV_IA64_IntZero && pReg[iReg].hReg <= CV_IA64_IntT22) {
            NatReg = CV_IA64_IntNats;
            NatBit = pReg[iReg].hReg - CV_IA64_IntZero;
        } else if (pReg[iReg].hReg >= CV_IA64_IntR32 && pReg[iReg].hReg <= CV_IA64_IntR95) {
            NatReg = CV_IA64_IntNats2;
            NatBit = pReg[iReg].hReg - CV_IA64_IntR32; 
        } else if (pReg[iReg].hReg >= CV_IA64_IntR96 && pReg[iReg].hReg <= CV_IA64_IntR127) {
            NatReg = CV_IA64_IntNats3;
            NatBit = pReg[iReg].hReg - CV_IA64_IntR96; 
        } else {
            DAssert(!"Unknown register with a NAT bit");
        }
        
        OSDReadRegister(LppdCur->hpid,LptdCur->htid,NatReg,&NatRegValue);
        
        switch (cNat) {
            case '1':
                NatRegValue |= (1 << NatBit);
                break;
            
            case '0':
                NatRegValue &= ~(1 << NatBit);
                break;
 
            default:
                Assert(!"Bad NAT Bit value");
                break;
        }
        
        OSDWriteRegister(LppdCur->hpid,LptdCur->htid,NatReg,&NatRegValue);
        }
    }

    else {
        OSDWriteFlag(LppdCur->hpid, LptdCur->htid, pReg[iReg].hFlag, lpb);
    }

    return(TRUE);

}   /* CPUSetValue */

/***    CpuVerifyNew
 *
 *      Purpose: Determine if a registerss result has changed since
 *               the last time a user saw it
 *
 *      Input:
 *      pvit    - A pointer to the vit
 *      oln     - Item number of interest
 *
 *      Output:
 *      Returns:
 *          TRUE/FALSE the item has changed
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


BOOL
CpuVerifyNew(
    PREGINFO pReg,
    UINT oln,
    UINT count
    )
{
    // No Registers, No change
    if ( pReg == 0) {
        return(FALSE);
    }

   // EA always get painted
    if ( oln >= count) {
        return(TRUE);
    }

    GetCpuString(pReg,count, ID_PANE_RIGHT, oln);


    // Do we have a string at all?
    if ( pReg[oln].pszValueP || pReg[oln].pszValueC) {

        // Do we have both strings?
        if ( pReg[oln].pszValueP && pReg[oln].pszValueC) {
            if ((!_strcmpi(pReg[oln].pszValueC, pReg[oln].pszValueP))
                 && (pReg[oln].fChanged == FALSE)) {

                return(FALSE);
            } else if ((!_strcmpi(pReg[oln].pszValueC, pReg[oln].pszValueP))
                         && (pReg[oln].fChanged == TRUE)) {
                pReg[oln].fChanged = FALSE;
                return(TRUE);
            }

       }

       // Nope, It changed
       pReg[oln].fChanged = TRUE;
       return(TRUE);
   }

   // Nope, No change
   return(FALSE);

}  /* CpuVerifyNew() */


/***    CPUEditProc
**
**  Synopsis:
**      long = CPUEditProc(hwnd, msg, wParam, lParam)
**
**  Entry:
**      hwnd    - handle to window to process message for
**      msg     - message to be processed
**      wParam  - information about message to be processed
**      lParam  - information about message to be processed
**
**  Returns:
**
**  Description:
**      MDI function to handle register window messages
**
*/

LRESULT
WINAPI
CPUEditProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PPANE     p     = (PPANE)lParam;
    PPANEINFO pInfo = (PPANEINFO)wParam;

    PREGINFO *ppReg = (p->Type == CPU_WIN) ? &pCpu     : &pFloat;
    PREGINFO  pReg  = (p->Type == CPU_WIN) ?  pCpu     :  pFloat;
    UINT     *pnCnt = (p->Type == CPU_WIN) ? &CpuCnt   : &FloatCnt;
    HWND     *phwnd = (p->Type == CPU_WIN) ? &hWndCpu  : &hWndFloat;

    UINT      i;

    switch (msg) {


        case WM_DESTROY:
            *phwnd = NULL;

            // No Break intended

        case WU_DBG_UNLOADEM:
        case WU_DBG_UNLOADEE:

            for ( i=0 ; i < *pnCnt ; i++) {
                if ( pReg[i].pszValueC ) {
                    free( pReg[i].pszValueC );
                }
                if ( pReg[i].pszValueP ) {
                    free( pReg[i].pszValueP );
                }
            }
            if ( pReg ) {
                free( pReg );
            }

            *ppReg = NULL;
            *pnCnt = 0;
            break;

        case WU_INVALIDATE:

            if (p != (PPANE)NULL) {
                InvalidateRect(p->hWndButton, NULL, TRUE);
                InvalidateRect(p->hWndLeft, NULL, TRUE);
                InvalidateRect(p->hWndRight, NULL, TRUE);
                UpdateWindow (p->hWndLeft);
                UpdateWindow (p->hWndRight);
            }
            break;


        case WU_INITDEBUGWIN:

            *phwnd = hwnd;
            SendMessage(p->hWndLeft, EM_SETREADONLY,(WPARAM)TRUE,0L);
            SendMessage(p->hWndRight,EM_SETREADONLY,(WPARAM)TRUE,0L);
            if ( !DbgFEmLoaded() ) {
                return(FALSE);
            }



                                // No Break intended


        case WU_DBG_LOADEM:

            if ( p->Type == CPU_WIN) {
                ULONG   ul;
                RT      rtType = rtCPU;

                if (g_contWorkspace_WkSp.m_bRegModeExt) {
                    rtType |= rtExtended;
                } else {
                    rtType |= rtRegular;
                }

                if (g_contWorkspace_WkSp.m_bRegModeMMU) {
                    rtType |= rtSpecial;
                    if (OSDGetDebugMetric(LppdCur->hpid, LptdCur->htid, mtrcExtMMU, &ul) == xosdNone &&
                            ul != 0) {
                        rtType |= rtKmode;
                    }
                }

                pCpu = CpuInitRegs(rtType, ftCPU, TRUE, &CpuCnt);
                SendMessage(p->hWndLeft,   LB_SETCOUNT, *pnCnt+EACOUNT, 0);
                SendMessage(p->hWndButton, LB_SETCOUNT, *pnCnt+EACOUNT, 0);
                SendMessage(p->hWndRight,  LB_SETCOUNT, *pnCnt+EACOUNT, 0);
            } else {
                RT rtType = rtFPU |
                         (g_contWorkspace_WkSp.m_bRegModeExt? rtExtended : rtRegular);
                pFloat = CpuInitRegs(rtType, ftFPU, TRUE, &FloatCnt);
                SendMessage(p->hWndLeft,   LB_SETCOUNT, *pnCnt, 0);
                SendMessage(p->hWndButton, LB_SETCOUNT, *pnCnt, 0);
                SendMessage(p->hWndRight,  LB_SETCOUNT, *pnCnt, 0);
            }
            p->MaxIdx = ((p->Type == CPU_WIN)?(*pnCnt + EACOUNT):*pnCnt);

            p->CurIdx = 0;

                                // No Break intended


        case WU_UPDATE: 

            // Is EM loaded?
            if ( !DbgFEmLoaded() ) {
                return(FALSE);
            }

            //v-vadimp - for Merced we need to reinit regs after branches since the number of stacked registers may have changed (not applicable to floating point regs?)
            if ( (p->Type == CPU_WIN) && (LppdCur->mptProcessorType == mptia64)) {

                //save the old values
                PREGINFO pOldCpu = pCpu;  
                UINT oldCpuCnt = CpuCnt;
                RT      rtType = rtCPU;
                ULONG   ul;
                UINT i,j;
                
                //get new reg info
                if (g_contWorkspace_WkSp.m_bRegModeExt) {
                    rtType |= rtExtended;
                } else {
                    rtType |= rtRegular;
                }

                if (g_contWorkspace_WkSp.m_bRegModeMMU) {
                    rtType |= rtSpecial;
                    if (OSDGetDebugMetric(LppdCur->hpid, LptdCur->htid, mtrcExtMMU, &ul) == xosdNone &&
                            ul != 0) {
                        rtType |= rtKmode;
                    }
                }

                pCpu = CpuInitRegs(rtType, ftCPU, TRUE, &CpuCnt); 

                //now go through the new list and copy all info about regs that were alread present - this will preserve correct register window update
                for(i=0; i<CpuCnt; i++) 
                    for(j=0; j<oldCpuCnt; j++) {
                        if( pCpu[i].hReg == pOldCpu[j].hReg ) {
                            memcpy((LPVOID)&pCpu[i],(LPVOID)&pOldCpu[j],sizeof(REGINFO));
                        }
                }
                
                //don't forget to free the old structure
                free(pOldCpu);

                //update the paneinfo
                SendMessage(p->hWndLeft,   LB_SETCOUNT, *pnCnt+EACOUNT, 0);
                SendMessage(p->hWndButton, LB_SETCOUNT, *pnCnt+EACOUNT, 0);
                SendMessage(p->hWndRight,  LB_SETCOUNT, *pnCnt+EACOUNT, 0);
                p->MaxIdx = ((p->Type == CPU_WIN)?(*pnCnt + EACOUNT):*pnCnt);
            }

            AgeCpuValues( *ppReg, *pnCnt);
            for ( i= p->TopIdx; i < (UINT)(p->TopIdx + p->PaneLines) ; i++) {
                if ( CpuVerifyNew(*ppReg,i,*pnCnt) ) {
                    PaneInvalidateItem( p->hWndRight, p, (WORD)i);
                 }
            }


            p->RightOk = TRUE;
            CheckPaneScrollBar(p, (WORD)((p->Type == CPU_WIN)?(*pnCnt + EACOUNT):*pnCnt));
            break;

        case WU_SETWATCH:
            if ( CPUSetValue(p, pReg) ) {
                p->RightOk = FALSE;
                UpdateDebuggerState(UPDATE_WINDOWS);
                return(TRUE);
            }
            break;

        case WU_INFO:
            i = pInfo->ItemId;

            pInfo->ReadOnly = TRUE;
            pInfo->NewText  = FALSE;
            pInfo->pBuffer  = GetCpuString(*ppReg,*pnCnt,pInfo->CtrlId,i);
            pInfo->pFormat  = NULL;     // No formatting allowed

            // If Its a Value Pane and Its a real register, allow edits
            // and....Set the Change History

            if ((pInfo->CtrlId == ID_PANE_RIGHT) && ((WORD)i < ((p->Type == CPU_WIN)?(p->MaxIdx - EACOUNT):p->MaxIdx))) {
                pInfo->ReadOnly = FALSE;
                if ( DbgFEmLoaded() ) {
                    if ( pReg[i].pszValueP && pReg[i].pszValueC) {
                        pInfo->NewText =  strcmp( pReg[i].pszValueP,pReg[i].pszValueC);
                    }
                }
            }

            return(TRUE);

        case WU_OPTIONS:
            return(TRUE);
    }

    return FALSE;
}                                       /* CPUEditProc() */


BOOL bDisplayReg(RD rd)  //TRUE if the reg qualifies for display

{
    if(LppdCur->mptProcessorType != mptia64) {
        return TRUE;
    }
 
    if (rd.rt & rtInvisible) {//first check if visible 
        return FALSE;
    }
    if (rd.rt & rtStacked)  {//if an IA64 stacked register check if it's on stack
        ULONGLONG ulIFS;
        OSDReadRegister(LppdCur->hpid,LptdCur->htid,CV_IA64_StIFS,&ulIFS);  // read the IFS
        if((rd.dwId - CV_IA64_IntR32) >= (ulIFS & 0x3F)) { //the low 6 bits is the register frame size
            return FALSE; //stacked not on the frame - no display
        } else {
            return TRUE;  //stacked on the frame - display
        }
    }
 return TRUE; //not IA64 stacked - display
 }



/***    CpuInitRegs()
**
**  Synopsis:
**      void = CpuInitRegs()
**
**  Entry:
**      none
**
**  Returns:
**      Nothing
**
**  Description:
**      This routine sets up our internal description of what a CPU window
**      will look like.  Several things must be noted here.
**
**      1.  This routine may be called multiple times if you load up a
**              new EM or if you change EM models.
**
**      2.  This routines requires that an EM be loaded in order to succeed
**
**      3.  This routine will use a set of flags to determine which set
**              of register is to be "formatted".
**
**      4.  Actual positioning of the register is still done in the
**              ViewCPU routine not here.
**
*/

PREGINFO
CpuInitRegs(
    RT rtType,
    FT ftType,
    BOOL fFlagsAppend,
    UINT *cnt
    )
{
    DWORD       dw;
    int         cRegs;
    int         cFlags;
    int         i;
    int         j;
    RD          rd;
    FD          fd;
    int         cSaveRegs = 0;
    int         cbSaveArea = 0;
    HTID        htid = (LptdCur != NULL) ? LptdCur->htid : (HTID)0;
    PREGINFO    LpCpuInfo = NULL;

    DAssert( LppdCur != NULL );
    DAssert( DbgFEmLoaded()  );

    /*
    **  Get the count of Register Description records in the EM
    */

    OSDGetDebugMetric(LppdCur->hpid, htid, mtrcCRegs, &dw);
    cRegs = (int) dw;
    OSDGetDebugMetric(LppdCur->hpid, htid, mtrcCFlags, &dw);
    cFlags = (int) dw;

    /*
    **  Now determine how many of them we really want to look at and
    **  display to the user
    */


    for (i=0; i<cRegs; i++) {
        Dbg( OSDGetRegDesc( LppdCur->hpid, htid, i, &rd) == xosdNone );
        if ( ((rd.rt & rtProcessMask) == (rtType & rtProcessMask)) &&
                ((rd.rt & rtType & rtGroupMask) != 0) &&
                bDisplayReg(rd) &&
                ((rtKmode & rd.rt) ? (rtKmode & rtType) : 1) ) {
            cSaveRegs += 1;
        }
    }

    for (i=0; i<cFlags; i++) {
        Dbg( OSDGetFlagDesc( LppdCur->hpid, htid, i, &fd) == xosdNone );
        if ( (fd.ft & ftType) == ftType ) {
            cSaveRegs += 1;
        }
    }

    /*
    **  Now allocate space for use to hold the information about
    **  the registers
    */

    LpCpuInfo = (PREGINFO) malloc(cSaveRegs * sizeof(*LpCpuInfo));

    /*
    **  Now fill in the elements of the information structure
    **  to the best of our ability
    */

    for (i=0, j=0; i<cRegs; i++) {
        Dbg( OSDGetRegDesc( LppdCur->hpid, htid, i, &rd) == xosdNone);
        if ( ((rd.rt & rtProcessMask) == (rtType & rtProcessMask)) &&
                ((rd.rt & rtType & rtGroupMask) != 0) &&
                bDisplayReg(rd) &&
                ((rtKmode & rd.rt) ? (rtKmode & rtType) : 1) ) {
            LpCpuInfo[j].lpsz = rd.lszName;
            LpCpuInfo[j].hReg = rd.dwId;
            LpCpuInfo[j].hFlag = (UINT) -1;
            LpCpuInfo[j].cbits = rd.dwcbits;
            LpCpuInfo[j].offValue = cbSaveArea;
            LpCpuInfo[j].pszValueC = NULL;
            LpCpuInfo[j].pszValueP = NULL;

            if ( (rd.rt & rtFmtTypeMask) == rtInteger ) {
                LpCpuInfo[j].type = fmtUInt | fmtZeroPad;
            } else if ( ( rd.rt & rtFmtTypeMask) == rtFloat) {
                LpCpuInfo[j].type = fmtFloat;
            } else if ( ( rd.rt & rtFmtTypeMask) == rtBit) {
                LpCpuInfo[j].type = fmtBit;
            } else {
                DAssert(FALSE);
            }

            if (rd.rt & rtNat) {
                LpCpuInfo[j].type |= fmtNat;
            }
 
            cbSaveArea += (rd.dwcbits + 7) / 8;
            j += 1;
        }
    }

    if (fFlagsAppend) {
        for (i=0; i<cFlags; i++) {
            Dbg( OSDGetFlagDesc( LppdCur->hpid, htid, i, &fd) == xosdNone );
            if ((fd.ft & ftType) == ftType ) {
                LpCpuInfo[j].lpsz = fd.lszName;
                LpCpuInfo[j].hReg = fd.dwId;
                LpCpuInfo[j].hFlag = i;
                LpCpuInfo[j].type = fmtUInt;
                LpCpuInfo[j].cbits = fd.dwcbits;
                LpCpuInfo[j].offValue = cbSaveArea;
                LpCpuInfo[j].pszValueC = NULL;
                LpCpuInfo[j].pszValueP = NULL;
                cbSaveArea += (fd.dwcbits + 7) / 8;
                j += 1;
            }
        }
    }

    DAssert( j == cSaveRegs );

    *cnt = cSaveRegs;
    return(LpCpuInfo);
}                                       /* CpuInitRegs() */


/*    AgeCpuValues
**
**
**  Synopsis:
**      void AgeCpuValues( PREGINFO pReg, UINT nCnt);
**
**      Purpose:
**        Age the Register value strings by moving it to the previous value.
**        If we have a previous value free it.  Null the current value.
**
**      Input:
**          pReg    - Pointer to the Register Info
**          nCnt    - Count of Registers
**
**      Output:
**        None
**
**      Exceptions:
**
**      Notes:
**
*/


VOID
AgeCpuValues(
    PREGINFO pReg,
    UINT nCnt
    )
{
    ULONG i;

    for ( i = 0; i < nCnt; i++) {

        if ( pReg[i].pszValueP ) {
            free(pReg[i].pszValueP);
        }

        // Move Current to Prev. and Null current

        pReg[i].pszValueP = pReg[i].pszValueC;
        pReg[i].pszValueC = NULL;
    }
}   // AgeCpuValues



/***    GetCpuString
 *
 *      Purpose:
 *        Get the String associated with a register index
 *
 *      Input:
 *        UINT PanelNumber - Panel whos string we need (BUTTON, LEFT, RIGHT)
 *        UINT n           - Index of the register
 *
 *      Output:
 *        PSTR Pointer to the buffer containing the string.
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


PSTR
GetCpuString(
    PREGINFO pInfo,
    UINT nMax,
    UINT PanelNumber,
    UINT nCnt
    )
{
    Unreferenced(nMax);

    if ( pInfo == NULL || LppdCur == NULL || LptdCur == NULL) {
        return(" ");
    }

    //  Focus in on the one we want
    switch (PanelNumber) {

        case ID_PANE_BUTTON:
            if ( nCnt < nMax ) {
                return("~");
            } else {
                return("~" );
            }

        case ID_PANE_LEFT:
            if ( nCnt < nMax ) {
                return(pInfo[nCnt].lpsz);
            }

            else if ( nCnt == nMax + 1 ) {
                return( "EA" );
            }

            else {
                return( " " );
            }

        case ID_PANE_RIGHT:
            if ( nCnt < nMax) {
                if ( pInfo[nCnt].pszValueC == NULL) {
                    pInfo[nCnt].pszValueC = _strdup(GetCpuValue(pInfo,nCnt));
                }
                return(pInfo[nCnt].pszValueC);
            }
            else {
                return GetEA( nCnt - nMax );
            }
    }

    return( " " );
}   // GetCpuString


/*    GetCpuValue
**
**
**  Synopsis:
**      void GetCpuValue( PREGINFO pReg, UINT nCnt);
**
**      Purpose:
**        Get the value string assoicated with the <nCnt>th register.
**
**      Input:
**          pReg    - Pointer to the Register Info
**          nCnt    - Count of Registers
**
**      Output:
**          Returns a pointer to a string.
**
**      Exceptions:
**
**      Notes:
**          Treat the string as READ-ONLY or else.
**
*/



PSTR
GetCpuValue(
    PREGINFO pInfo,
    UINT n
    )
{
    BYTE lpb[100];
    UINT digits;            // precision expected

    //  Read the Value
    if ( pInfo[n].hFlag == -1) {
        OSDReadRegister(LppdCur->hpid,LptdCur->htid,pInfo[n].hReg,&lpb);
    } else {
        OSDReadFlag(LppdCur->hpid,LptdCur->htid,pInfo[n].hFlag,&lpb);
    }

    //  Format the Value
    szValue[0] = 0;
    DAssert( sizeof(szValue) > (pInfo[n].cbits+3)/4 + 1);

    if (pInfo[n].type == fmtFloat) {
        if(LppdCur->mptProcessorType == mptia64) {
            digits = 55;
        } else {
            digits = 26;
        }
    } else if ((pInfo[n].type & fmtBasis) == fmtBit) {
        digits = pInfo[n].cbits + (pInfo[n].cbits+7)/8;  //v-vadimp bits plus a space for each byte
    } else {
        digits = (pInfo[n].cbits+3)/4 + 1;
    }

    CPFormatMemory(szValue,
                   digits,
                   lpb,
                   pInfo[n].cbits,
                   pInfo[n].type,
                   16
                   );
    if((LppdCur->mptProcessorType == mptia64) && (pInfo[n].type & fmtNat)) { // v-vadimp - pick up the NAT bit
        UINT NatReg, NatBit;
        LONGLONG NatRegValue;
        if (pInfo[n].hReg >= CV_IA64_IntZero && pInfo[n].hReg <= CV_IA64_IntT22) {
            NatReg = CV_IA64_IntNats;
            NatBit = pInfo[n].hReg - CV_IA64_IntZero;
        } else if (pInfo[n].hReg >= CV_IA64_IntR32 && pInfo[n].hReg <= CV_IA64_IntR95) {
            NatReg = CV_IA64_IntNats2;
            NatBit = pInfo[n].hReg - CV_IA64_IntR32; 
        } else if (pInfo[n].hReg >= CV_IA64_IntR96 && pInfo[n].hReg <= CV_IA64_IntR127) {
            NatReg = CV_IA64_IntNats3;
            NatBit = pInfo[n].hReg - CV_IA64_IntR96; 
        } else {
            DAssert(!"Unknown register with a NAT bit");
        }
        OSDReadRegister(LppdCur->hpid,LptdCur->htid,NatReg,&NatRegValue);
        if ((NatRegValue >> NatBit) & 0x1) {
            strcat(szValue," 1");
        } else {
            strcat(szValue," 0");
        }
    }

    return(szValue);

}   // GetCpuValue



/*    GetEA
**
**
**  Synopsis:
**      PSTR GetEA( UINT nCnt);
**
**      Purpose:
**        Get one of the EA for the current instruction.
**
**      Input:
**          nCnt    - The EA to Get (1-based)
**
**      Output:
**        None
**
**      Exceptions:
**
**      Notes:
**         A nCnt of zero returns blank....it simplifies the caller
**
*/

PSTR
GetEA(
    UINT n
    )
{
    SDI sds;
    ADDR AddrPC;

    if ( LppdCur && LptdCur && n > 0 ) {
        AddrPC = *SHpADDRFrompCXT(&CxfIp.cxt);
        SYFixupAddr(&AddrPC);

        sds.dop = dopEA|dopFlatAddr;
        sds.addr = AddrPC;
        OSDUnassemble(LppdCur->hpid, LptdCur->htid, &sds);

        switch ( n ) {

            case 1:
                if ( sds.ichEA0 != -1) {
                    return(&sds.lpch[sds.ichEA0]);
                }
                break;

            case 2:
                if ( sds.ichEA1 != -1) {
                    return(&sds.lpch[sds.ichEA1]);
                }
                break;

            case 3:
                if ( sds.ichEA2 != -1) {
                    return(&sds.lpch[sds.ichEA2]);
                }
                break;

            default:
                break;
        }
    }
    return( " " );
}
