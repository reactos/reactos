/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMSGLB.C
 *  WOW32 16-bit message thunks
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wmsglb.c);


#ifdef DEBUG

MSGINFO amiLB[] = {
   {OLDLB_ADDSTRING,        "LB_ADDSTRING"},        // 0x0401
   {OLDLB_INSERTSTRING,     "LB_INSERTSTRING"},     // 0x0402
   {OLDLB_DELETESTRING,     "LB_DELETESTRING"},     // 0x0403
   {OLDLB_RESETCONTENT,     "LB_RESETCONTENT"},     // 0x0405
   {OLDLB_SETSEL,       "LB_SETSEL"},           // 0x0406
   {OLDLB_SETCURSEL,        "LB_SETCURSEL"},        // 0x0407
   {OLDLB_GETSEL,       "LB_GETSEL"},           // 0x0408
   {OLDLB_GETCURSEL,        "LB_GETCURSEL"},        // 0x0409
   {OLDLB_GETTEXT,      "LB_GETTEXT"},          // 0x040A
   {OLDLB_GETTEXTLEN,       "LB_GETTEXTLEN"},       // 0x040B
   {OLDLB_GETCOUNT,     "LB_GETCOUNT"},         // 0x040C
   {OLDLB_SELECTSTRING,     "LB_SELECTSTRING"},     // 0x040D
   {OLDLB_DIR,          "LB_DIR"},          // 0x040E
   {OLDLB_GETTOPINDEX,      "LB_GETTOPINDEX"},      // 0x040F
   {OLDLB_FINDSTRING,       "LB_FINDSTRING"},       // 0x0410
   {OLDLB_GETSELCOUNT,      "LB_GETSELCOUNT"},      // 0x0411
   {OLDLB_GETSELITEMS,      "LB_GETSELITEMS"},      // 0x0412
   {OLDLB_SETTABSTOPS,      "LB_SETTABSTOPS"},      // 0x0413
   {OLDLB_GETHORIZONTALEXTENT,  "LB_GETHORIZONTALEXTENT"},  // 0x0414
   {OLDLB_SETHORIZONTALEXTENT,  "LB_SETHORIZONTALEXTENT"},  // 0x0415
   {OLDLB_SETCOLUMNWIDTH,   "LB_SETCOLUMNWIDTH"},       // 0x0416
   {OLDLB_ADDFILE,      "LB_ADDFILE"},          // 0x0417
   {OLDLB_SETTOPINDEX,      "LB_SETTOPINDEX"},      // 0x0418
   {OLDLB_GETITEMRECT,      "LB_GETITEMRECT"},      // 0x0419
   {OLDLB_GETITEMDATA,      "LB_GETITEMDATA"},      // 0x041A
   {OLDLB_SETITEMDATA,      "LB_SETITEMDATA"},      // 0x041B
   {OLDLB_SELITEMRANGE,     "LB_SELITEMRANGE"},     // 0x041C
   {OLDLB_SETANCHORINDEX,   "LB_SETANCHORINDEX"},       // 0x041D
   {OLDLB_GETANCHORINDEX,   "LB_GETANCHORINDEX"},       // 0x041E
   {OLDLB_SETCARETINDEX,    "LB_SETCARETINDEX"},        // 0x041F
   {OLDLB_GETCARETINDEX,    "LB_GETCARETINDEX"},        // 0x0420
   {OLDLB_SETITEMHEIGHT,    "LB_SETITEMHEIGHT"},        // 0x0421
   {OLDLB_GETITEMHEIGHT,    "LB_GETITEMHEIGHT"},        // 0x0422
   {OLDLB_FINDSTRINGEXACT,  "LB_FINDSTRINGEXACT"},      // 0x0423
   {OLDLBCB_CARETON,        "LBCB_CARETON"},            // 0x0424
   {OLDLBCB_CARETOFF,        "LBCB_CARETOFF"},          // 0x0425
};

PSZ GetLBMsgName(WORD wMsg)
{
    INT i;
    register PMSGINFO pmi;

    for (pmi=amiLB,i=NUMEL(amiLB); i>0; i--,pmi++)
        if ((WORD)pmi->uMsg == wMsg)
        return pmi->pszMsgName;
    return GetWMMsgName(wMsg);
}

#endif


BOOL FASTCALL ThunkLBMsg16(LPMSGPARAMEX lpmpex)
{
    register PWW pww;
    WORD wMsg = lpmpex->Parm16.WndProc.wMsg;

    LOGDEBUG(7,("    Thunking 16-bit list box message %s(%04x)\n", (LPSZ)GetLBMsgName(wMsg), wMsg));

    wMsg -= WM_USER + 1;

    //
    // For app defined (control) messages that are out of range
    // return TRUE.
    //
    // ChandanC Sept-15-1992
    //

    if (wMsg < (LBCB_CARETOFF - LB_ADDSTRING + 1)) {

        switch(lpmpex->uMsg = wMsg + LB_ADDSTRING) {

        case LB_SELECTSTRING:
        case LB_FINDSTRING:
        case LB_FINDSTRINGEXACT:
        case LB_INSERTSTRING:
        case LB_ADDSTRING:
            if (!(pww = lpmpex->pww))
                    return FALSE;

            if (!(pww->style & (LBS_OWNERDRAWFIXED|LBS_OWNERDRAWVARIABLE)) ||
                 (pww->style & (LBS_HASSTRINGS))) {
                    GETPSZPTR(lpmpex->Parm16.WndProc.lParam, (LPSZ)lpmpex->lParam);
                }
            break;

        case LB_DIR:
            GETPSZPTR(lpmpex->Parm16.WndProc.lParam, (LPSZ)lpmpex->lParam);
            break;

        case LB_GETTEXT:
            if (NULL != (pww = lpmpex->pww)) {
                register PTHUNKTEXTDWORD pthkdword = (PTHUNKTEXTDWORD)lpmpex->MsgBuffer;

                // we set this as a flag to indicate that we retrieve a dword
                // instead of a string there. In case when hooks are installed
                // this code prevents RISC platforms from malfunctioning in
                // kernel (they have code like this:
                //   try {
                //       <assign to original ptr here>
                //   }
                //   except(1) {
                //       <put error message in debug>
                //   }
                // which causes this message not to return the proper value)
                // See walias.h for definition of THUNKTEXTDWORD structure as
                // well as MSGPARAMEX structure
                // this code is complemented in UnThunkLBMsg16
                //
                // Application: PeachTree Accounting v3.5

                pthkdword->fDWORD = (pww->style & (LBS_OWNERDRAWFIXED|LBS_OWNERDRAWVARIABLE)) &&
                                    !(pww->style & (LBS_HASSTRINGS));

                if (pthkdword->fDWORD) {
                    lpmpex->lParam = (LPARAM)(LPVOID)&pthkdword->dwDataItem;
                    break;
                }
            }
            else {
                register PTHUNKTEXTDWORD pthkdword = (PTHUNKTEXTDWORD)lpmpex->MsgBuffer;
                pthkdword->fDWORD = FALSE;
            }

            GETPSZPTR(lpmpex->Parm16.WndProc.lParam, (LPSZ)lpmpex->lParam);
            break;

        case LB_GETITEMRECT:
            lpmpex->lParam = (LONG)lpmpex->MsgBuffer;
            break;

        case LB_GETSELITEMS:
            (PVOID)lpmpex->lParam = STACKORHEAPALLOC(lpmpex->Parm16.WndProc.wParam * sizeof(INT),
                                                       sizeof(lpmpex->MsgBuffer), lpmpex->MsgBuffer);
            break;

        case LB_SETSEL:
            // sign extend
            {
                LPARAM lParam = lpmpex->Parm16.WndProc.lParam;
                lpmpex->lParam = (LOWORD(lParam) == 0xffff) ?
                                         INT32(LOWORD(lParam)) : (LONG)lParam;
            }
            break;

        case LB_SETTABSTOPS:
            //  apparently lParam is a pointer even if wParam == 1. Recorder passes
            //  the data so.    - nandurir

            {
                INT cItems = INT32(lpmpex->Parm16.WndProc.wParam);
                if (cItems > 0) {
                    (PVOID)lpmpex->lParam = STACKORHEAPALLOC(cItems * sizeof(INT),
                                   sizeof(lpmpex->MsgBuffer), lpmpex->MsgBuffer);
                    getintarray16((VPINT16)lpmpex->Parm16.WndProc.lParam, cItems, (LPINT)lpmpex->lParam);
                }
            }
            break;

        case LB_ADDSTRING + 3:
            if (!(CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_THUNKLBSELITEMRANGEEX)) {
               lpmpex->uMsg = 0;
            }
            break;

        }
    }
    return TRUE;
}


VOID FASTCALL UnThunkLBMsg16(LPMSGPARAMEX lpmpex)
{
    switch(lpmpex->uMsg) {

    case LB_GETTEXT:
        {
           register PTHUNKTEXTDWORD pthkdword = (PTHUNKTEXTDWORD)lpmpex->MsgBuffer;

           if ((pthkdword->fDWORD) && (lpmpex->lReturn != LB_ERR)) {
                // this is a dword, not a string
                // assign the dword as unaligned
                UNALIGNED DWORD *lpdwDataItem;

                GETVDMPTR((lpmpex->Parm16.WndProc.lParam), sizeof(DWORD), lpdwDataItem);
                *lpdwDataItem = pthkdword->dwDataItem;
                FREEVDMPTR(lpdwDataItem);
                break;
           }

        }

        // fall through to the common code


    case LB_ADDSTRING:      // BUGBUG 3-Jul-1991 JeffPar: for owner-draw list boxes, this can just be a 32-bit number
    case LB_DIR:
    case LB_FINDSTRING:     // BUGBUG 3-Jul-1991 JeffPar: for owner-draw list boxes, this can just be a 32-bit number
    case LB_FINDSTRINGEXACT:
    case LB_INSERTSTRING:
    case LB_SELECTSTRING:
        FREEPSZPTR((LPSZ)lpmpex->lParam);
        break;

    case LB_GETITEMRECT:
        if ((lpmpex->lParam) && (lpmpex->lReturn != -1L)) {
            putrect16((VPRECT16)lpmpex->Parm16.WndProc.lParam, (LPRECT)lpmpex->lParam);
        }
        break;

    case LB_GETSELITEMS:
        PUTINTARRAY16V((VPINT16)lpmpex->Parm16.WndProc.lParam, (INT)(lpmpex->lReturn), (LPINT)lpmpex->lParam);
        STACKORHEAPFREE((LPINT)lpmpex->lParam, lpmpex->MsgBuffer);
        break;

    case LB_SETTABSTOPS:
        if (lpmpex->Parm16.WndProc.wParam > 0) {
            STACKORHEAPFREE((LPINT)lpmpex->lParam, lpmpex->MsgBuffer);
        }
        break;
    }
}
