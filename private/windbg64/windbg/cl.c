/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cl.c

Abstract:

    This file contains the code required to do the actual walking of the
    debuggers stack (with help from the OSDEBUG layer).  The display of
    the data is done else where.

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

--*/

#include "precomp.h"
#pragma hdrstop



#define IsSegEqual(a, b) ((a) == (b))


CIS G_cisCallsInfo;

extern LPSHF    Lpshf;                  /* Pointer to SH entry structure */
extern EI       Ei;
#define Lpei    (&Ei)

/*** CLLookupAddress
**
**  Purpose: To lookup an address in the callback stack area
**
**  Input:
**      paddr   -  A pointer to an ADDR struct contining the address to
**              find in the calls stack
**
**  Output:
**      Returns The index into the calls stack containing the address
**
**  Exceptions:
**
**  Notes:    -1 returned on error
**
*/

int PASCAL CLLookupAddress ( ADDR addr )
{
    int ifme;

    for ( ifme = 0; ifme < (int) G_cisCallsInfo.cEntries; ifme++ ) {
        FME *pfme = &(G_cisCallsInfo.frame [ ifme ] );

        if ( pfme->symbol == (HSYM) NULL ) {
            continue;
        }

        if (!ADDR_IS_LI(pfme->addrProc)) {
            SYUnFixupAddr( &pfme->addrProc);
        }

        if (!ADDR_IS_LI(pfme->addrCSIP)) {
            SYUnFixupAddr( &pfme->addrCSIP);
        }

        if ( IsSegEqual (
            (ushort) GetAddrSeg ( pfme->addrProc ),
            (ushort) GetAddrSeg ( addr )
             ) &&
             GetAddrOff ( pfme->addrProc ) <= GetAddrOff ( addr ) &&
             GetAddrOff ( pfme->addrCSIP ) >= GetAddrOff ( addr ) ) {

            return ifme;
        }
    }

    return -1;
}                                       /* CLLookupAddress() */

/***    CLGetParams
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

char * CLGetParams ( PHTM phtm, FRAME *pframe, int *pcbMax, char *pch )
{
    ushort      i = 0;
    ushort      cParm = 0;
    char FAR *  lpch;
    SHFLAG      shflag = FALSE;
    HTM         htmParm;
    EEHSTR      hName;
    uint        strIndex;

    char *pchOrg = pch;

    if ( EEcParamTM ( phtm, &cParm, &shflag ) == EENOERROR ) {

        *pch++ = '('; // )
        *pcbMax -= 1;

        for ( i = 0; i < cParm; i++ ) {

            if ( *pcbMax > 0 &&
                EEGetParmTM ( phtm, (EERADIX) i, &htmParm, &strIndex, FALSE ) == EENOERROR ) {

                EEvaluateTM ( &htmParm, pframe, EEHORIZONTAL );
                if ( EEGetValueFromTM ( &htmParm, radix, (PEEFORMAT) "p", &hName ) ==
                      EENOERROR ) {
                    int ich;

                    lpch = MMLpvLockMb ( hName );
                    memcpy ( pch, lpch,
                          ( ich = min ( *pcbMax, (int)strlen ( lpch ) ) )
                          );
                    pch += ich;
                    *pcbMax -= ich + 1;
                    MMbUnlockMb ( hName );
                    EEFreeStr ( hName );

                    *pch++ = ',';
                    *pch++ = ' ';
                }
                EEFreeTM (&htmParm);
            }
        }

        if ( ( shflag == TRUE ) && ( *pcbMax > 3 ) ) {
            memcpy ( pch, "...", 3 );
            pch += 3;
            *pcbMax -= 3;
        }

        do {
            pch = CharPrev(pchOrg, pch);
            if (*pch != ' ' && *pch != ',') {
                pch = CharNext(pch);
                break;
            }
        } while (pch > pchOrg);
        *pch++ = ')';
    }

    return pch;
}                                       /* CLGetParams*/


/*** CLGetProcName
*
* Purpose: To format a line to be diplayed in the calls menu
*
* Input:
*   num     - The index into the stack frame
*   pch     - A pointer a buffer to put the string
*
* Output:
*  Returns .....
*
* Exceptions:
*
* Notes:
*  to prevent output buffer overruns (pch) , we need to check the length
*  during format. but instead of checking for length every time we xfer into
*  output buffer, we check at selected points in the code and make certain
*  safe assumptions eg the length passed to us will be able to accomodate
*  the symbol name totally etc. Thus we remain ok and save some code & time
*
*/


char * PASCAL CLGetProcName ( int ifme, char *pch, int cbMax, BOOL bSpecial )
{
    char *      pchT;
    DWORD64     dwDisplacement;
    int         len;
    HTM         htm;
    EEHSTR      hName;
    ushort      retval;
    CXT         cxt;
    ADDR        addrT;
    uint        strIndex;
    HDEP        hstr;
    char        szContext[MAX_USER_LINE];

    FME *       pfme = &(G_cisCallsInfo.frame [ ifme ]);
    HSYM        symbol = pfme->symbol;

    cbMax -= 2;
    pchT   = pch;

    memset ( &cxt, 0, sizeof ( cxt ) );
    if ( pfme->clt != cltNone ) {
        *SHpADDRFrompCXT ( &cxt ) = pfme->addrProc;
    }
    else {
        *SHpADDRFrompCXT ( &cxt ) = pfme->addrCSIP;
    }
    SHHMODFrompCXT ( &cxt )   = pfme->module;

    switch ( pfme->clt) {

      case cltNone:
        {
            ADDR addrT = pfme->addrCSIP;

            SYFixupAddr ( &addrT );
            EEFormatAddr( &addrT, pch, cbMax, g_contWorkspace_WkSp.m_bShowSegVal * EEFMT_SEG);
            len = strlen(pch);
            pchT += len;
            break;

        }
      case cltProc:

        Dbg(SHAddrFromHsym ( &addrT, symbol ));

        //SetAddrOff (
        //      SHpADDRFrompCXT ( &cxt ) ,
        //      GetAddrOff ( *SHpADDRFrompCXT ( &cxt ) ) + GetAddrOff ( addrT )
        //      );
        SetAddrOff (
              SHpADDRFrompCXT ( &cxt ) ,
              GetAddrOff ( addrT )
              );
        SHHPROCFrompCXT ( &cxt ) = (HPROC) symbol;
        SHHBLKFrompCXT( &cxt ) = (HBLK) symbol;
        goto MakeName;

      case cltBlk:

        SHHBLKFrompCXT ( &cxt )  = (HBLK) symbol;

      case cltPub:
MakeName:

        //
        // include the context string if the command is KB
        //
        if (bSpecial) {
            EEFormatCXTFromPCXT(&cxt, &hstr);
            BPShortenContext( (LPSTR)MMLpvLockMb(hstr), szContext);
            MMbUnlockMb(hstr);
            EEFreeStr(hstr);
            strcpy( pchT, szContext );
            pchT += strlen( szContext );
        }

        retval = EEGetTMFromHSYM ( symbol, &cxt, &htm, &strIndex, FALSE );

        Assert( retval != EECATASTROPHIC );

        if ( retval != EENOERROR ) {

            // try to get something from this miserable symbol.

            if (SHGetSymName(symbol, pch)) {
                return pch;
            } else {
                *pchT = 0;
                return pch;
            }
        }

        // should always be able to convert an hSym

        Assert ( retval == EENOERROR );

        if ( EEGetNameFromTM ( &htm, &hName ) == EENOERROR ) {

            char far * lpch = MMLpvLockMb ( hName );

            memcpy (
                pchT,
                lpch,
                len = min ( cbMax, (int) strlen (lpch) )
                );
            pchT += len;
            cbMax -= len;
            MMbUnlockMb ( hName );
            EEFreeStr ( hName );
        }

        //
        // calculate the displacement
        //
        if (bSpecial) {
            addrT = *SHpAddrFrompCxt(&cxt);
            SHAddrFromHsym(&addrT, symbol);
            SYFixupAddr(&addrT);
            dwDisplacement = GetAddrOff(pfme->addrCSIP) - GetAddrOff(addrT);
            if (dwDisplacement > 0) {
                pchT += wsprintf( pchT, "+0x%I64x", dwDisplacement );
            }
        }


        /* Get the argument values, but only for procs */

        if ( ( cbMax > 0 ) && pfme->clt == cltProc ) {

            fUseFrameContext = TRUE;
            OSDSetFrameContext( LppdCur->hpid, LptdCur->htid, ifme, 0 );
            pchT = CLGetParams ( &htm, &pfme->Frame, &cbMax, pchT );
            fUseFrameContext = FALSE;
        }

        EEFreeTM ( &htm );
        break;

    }

    *pchT = '\0';
    errno = 0;          /* clear out any I/O errors */
    return pch;
}                                       /* CLGetProcName() */

/*** CLSetProcAddr
*
*   Purpose: To set up the symbols part of the stack frame structure
*
*   Input:
*       pfme        addrCSIP must be set up.
*   Output:
*       pfme        The stack frame to fill in the result
*                   Elements:
*                       symbol
*                       clt
*                       addrProc
*                       module
*
*
*   Returns:
*
*   Exceptions:
*
*   Notes:
*
*/
void PASCAL NEAR CLSetProcAddr ( FME *pfme, int *fInProlog )
{
    CXT  cxt;
    ADDR addrT;

    *fInProlog = FALSE;
    pfme->addrProc = pfme->addrCSIP;

    memset ( &cxt, 0, sizeof ( CXT ) );
    SHSetCxt ( &pfme->addrProc, &cxt );
    //if ( !SHHMODFrompCXT ( &cxt ) ) {
        //pfme->symbol   = (HSYM) NULL;
        //pfme->clt = cltNone;
        //return;
    //}

    pfme->addrProc = *SHpADDRFrompCXT ( &cxt );
    pfme->module   =  SHHMODFrompCXT ( &cxt );

    // block also may have a name. If it does, use it as the symbol in the
    // calls stack.


    if ( SHHPROCFrompCXT ( &cxt ) ) {
        pfme->symbol = (HSYM) SHHPROCFrompCXT ( &cxt );
        pfme->clt    = cltProc;
        Dbg(SHAddrFromHsym ( &addrT, pfme->symbol ));
        SetAddrOff (
            &pfme->addrProc,
            GetAddrOff ( addrT )
        );
        *fInProlog = SHIsInProlog ( &cxt );
    }
    else if ( SHHBLKFrompCXT ( &cxt ) ) {
        HBLK      hblk;
        ADDR      addrT;
        char      rgch [ 100 ];

        hblk = SHHBLKFrompCXT ( &cxt );
        memset ( &addrT, 0, sizeof ( ADDR ) );

        Dbg(SHAddrFromHsym ( &addrT, (HSYM) hblk ));
        if ( SHGetSymName ( (HSYM) hblk, rgch ) != NULL ) {
            pfme->symbol = (HSYM) hblk;
            pfme->clt = cltBlk;
        }

        SetAddrOff (
                &pfme->addrProc,
                GetAddrOff ( addrT )
        );
    }
    else if ( PHGetNearestHsym (
                (PADDR)&pfme->addrCSIP,
                (HEXE)(SHpADDRFrompCXT( &cxt ) ->emi),
                //SHHexeFromHmod ( SHHMODFrompCXT ( &cxt ) ),
                (PHSYM) &pfme->symbol ) <
              0xFFFFFFFF ) {
        ADDR addrT;

        memset ( &addrT, 0, sizeof ( ADDR ) );

        Dbg(SHAddrFromHsym ( &addrT, pfme->symbol ));

        pfme->clt = cltPub;
        SetAddrOff ( &pfme->addrProc, GetAddrOff ( addrT ) );
    }
    else {
        pfme->symbol   = (HSYM) NULL;
        pfme->clt = cltNone;
    }
}                                       /* CLSetProcAddr() */

/*** CLGetWalkbackStack
*
*   Purpose: To set up the calls walkback structure
*
*   Input:
*
*   Output:
*   Returns:
*
*   Exceptions:
*
*   Notes:
*       The rules are:
*       BP must point to the previous BP on the stack.
*       The return address must be at BP+2.
*
*       Currently we don't support the _saveregs options or
*       _fastcall with stack checking on.
*
*       Also any function without symbolics are skipped in the
*       the trace back.
*/

void CLGetWalkbackStack ( LPPD lppd, LPTD lptd )
{
    STKSTR      stkStr;
    BOOL        fInProlog = FALSE;
    XOSD        xosd = xosdNone;
    int         ifme;
    BOOL        fDone = FALSE;
    FME *       pfme = &(G_cisCallsInfo.frame[0]);
    ADDR        addrData;

    /*
     *  Determine if the current Program Counter address is in
     *  the prolog of the current function.  This information is
     *  needed to start the stack walk operation.
     */

    OSDGetAddr(lppd->hpid, lptd->htid, adrPC, &pfme->addrCSIP);
#ifdef OSDEBUG4
    OSDUnFixupAddress(lppd->hpid, lptd->htid, &pfme->addrCSIP);
#else
    OSDPtrace(osdUnFixupAddr, 0, &pfme->addrCSIP, lppd->hpid, lptd->htid);
#endif
    CLSetProcAddr ( pfme, &fInProlog);

    for (ifme=0; ifme<ifmeMax+1 && xosd==xosdNone; ifme++) {
        if (ifme == 0) {
            xosd = OSDStackWalkSetup(lppd->hpid, lptd->htid, fInProlog, &stkStr);
        } else {
            xosd = OSDStackWalkNext( lppd->hpid, lptd->htid, &stkStr );
        }
        pfme = &(G_cisCallsInfo.frame[ifme]);
        pfme->addrCSIP = stkStr.addrPC;
#ifdef OSDEBUG4
        OSDUnFixupAddress(lppd->hpid, lptd->htid, &pfme->addrCSIP);
#else
        OSDPtrace(osdUnFixupAddr, 0, &pfme->addrCSIP, lppd->hpid, lptd->htid);
#endif
        CLSetProcAddr(pfme, &fInProlog );
        pfme->addrCSIP = stkStr.addrPC;
        pfme->addrRet = stkStr.addrRetAddr;
        if (ADDR_IS_LI(stkStr.addrFrame)) {
            SYFixupAddr( &stkStr.addrFrame );
        }
        pfme->Frame.mode = stkStr.addrFrame.mode;
        SetFrameBPSeg ( pfme->Frame , GetAddrSeg ( stkStr.addrFrame ) );
        SetFrameBPOff ( pfme->Frame , GetAddrOff ( stkStr.addrFrame ) );
        pfme->Frame.SS  = GetAddrSeg ( stkStr.addrFrame );
        OSDGetAddr ( LppdCur->hpid, LptdCur->htid, adrData, &addrData );
        pfme->Frame.DS = GetAddrSeg ( addrData );
        pfme->Frame.TID = LptdCur->htid;
        pfme->Frame.PID = LppdCur->hpid;
        pfme->ulParams[0] = stkStr.ulParams[0];
        pfme->ulParams[1] = stkStr.ulParams[1];
        pfme->ulParams[2] = stkStr.ulParams[2];
        pfme->ulParams[3] = stkStr.ulParams[3];
        pfme->fFar = stkStr.fFar;
    }

    OSDStackWalkCleanup(lppd->hpid, lptd->htid, &stkStr);

    G_cisCallsInfo.cEntries = ifme;

    return;
}                                       /* CLGetWalkBackstack() */

/*** CLGetFuncCXF
**
**   Purpose:  To get a frame given an address of a function on the
**      calls stack
**
**   Input:
**      paddr   - A pointer to the address of the function
**
**   Output:
**      pCXF    - A pointer to an empty CXF, This will be filled in with
**                the scope and frame of the function.
**   Returns:
**      A pointer to the CXF if successful, NULL otherwise.
**
**   Exceptions:
**
**   Notes: A NULL is returned if the function could not be found on
**          the calls stack, OR if the function has no symbolic info.
**
*/

PCXF LOADDS PASCAL CLGetFuncCXF ( PADDR paddr, PCXF pcxf )
{
    ADDR        addr;
    int         ifme;
    FME *       pfme = NULL;

    Assert ( ADDR_IS_LI (*paddr));
    addr = *paddr;
    memset ( pcxf, 0, sizeof ( CXF ) );

    CLGetWalkbackStack ( LppdCur, LptdCur );

    // get the stack element

    ifme = CLLookupAddress ( addr );
    if ( ifme != -1 ) {
        pfme = &(G_cisCallsInfo.frame [ ifme ] );
    }

    if ( pfme != NULL && ( pfme->clt == cltProc || pfme->clt == cltBlk ) ) {

        /*
         * Fill in the return address, then subtract 1 if ifme > 0, this
         *      is due to the fact that the instruction AFTER the call
         *      may not be in the same context as the call itself.
         */

        addr  = pfme->addrCSIP;
        if (ifme > 0) {
            SetAddrOff( &addr, GetAddrOff(addr) - 1);
        }
        SHSetCxt( &addr, &pcxf->cxt );

        // fill in the CXF

        pcxf->Frame     = pfme->Frame;
        pcxf->cxt.addr      = addr;

        return pcxf;
    }

    return NULL;
}
