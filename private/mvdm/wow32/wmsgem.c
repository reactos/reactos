/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMSGEM.C
 *  WOW32 16-bit message thunks
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wmsgem.c);

VPVOID  WordBreakProc16 = 0;

extern WBP W32WordBreakProc;

#ifdef DEBUG

MSGINFO amiEM[] = {
   {OLDEM_GETSEL,               "EM_GETSEL"},                       // 0x0400
   {OLDEM_SETSEL,               "EM_SETSEL"},                       // 0x0401
   {OLDEM_GETRECT,              "EM_GETRECT"},                      // 0x0402
   {OLDEM_SETRECT,              "EM_SETRECT"},                      // 0x0403
   {OLDEM_SETRECTNP,            "EM_SETRECTNP"},                    // 0x0404
   {OLDEM_SCROLL,               "EM_SCROLL"},                       // 0x0405
   {OLDEM_LINESCROLL,           "EM_LINESCROLL"},                   // 0x0406
   {OLDEM_GETMODIFY,            "EM_GETMODIFY"},                    // 0x0408
   {OLDEM_SETMODIFY,            "EM_SETMODIFY"},                    // 0x0409
   {OLDEM_GETLINECOUNT,         "EM_GETLINECOUNT"},                 // 0x040A
   {OLDEM_LINEINDEX,            "EM_LINEINDEX"},                    // 0x040B
   {OLDEM_SETHANDLE,            "EM_SETHANDLE"},                    // 0x040C
   {OLDEM_GETHANDLE,            "EM_GETHANDLE"},                    // 0x040D
   {OLDEM_GETTHUMB,             "EM_GETTHUMB"},                     // 0x040E
   {OLDEM_LINELENGTH,           "EM_LINELENGTH"},                   // 0x0411
   {OLDEM_REPLACESEL,           "EM_REPLACESEL"},                   // 0x0412
   {OLDEM_SETFONT,              "EM_SETFONT"},                      // 0x0413
   {OLDEM_GETLINE,              "EM_GETLINE"},                      // 0x0414
   {OLDEM_LIMITTEXT,            "EM_LIMITTEXT"},                    // 0x0415
   {OLDEM_CANUNDO,              "EM_CANUNDO"},                      // 0x0416
   {OLDEM_UNDO,                 "EM_UNDO"},                         // 0x0417
   {OLDEM_FMTLINES,             "EM_FMTLINES"},                     // 0x0418
   {OLDEM_LINEFROMCHAR,         "EM_LINEFROMCHAR"},                 // 0x0419
   {OLDEM_SETWORDBREAK,         "EM_SETWORDBREAK"},                 // 0x041A
   {OLDEM_SETTABSTOPS,          "EM_SETTABSTOPS"},                  // 0x041B
   {OLDEM_SETPASSWORDCHAR,      "EM_SETPASSWORDCHAR"},              // 0x041C
   {OLDEM_EMPTYUNDOBUFFER,      "EM_EMPTYUNDOBUFFER"},              // 0x041D
   {OLDEM_GETFIRSTVISIBLELINE,  "EM_GETFIRSTVISIBLELINE"},          // 0x041E
   {OLDEM_SETREADONLY,          "EM_SETREADONLY"},                  // 0x041F
   {OLDEM_SETWORDBREAKPROC,     "EM_SETWORDBREAKPROC"},             // 0x0420
   {OLDEM_GETWORDBREAKPROC,     "EM_GETWORDBREAKPROC"},             // 0x0421
   {OLDEM_GETPASSWORDCHAR,      "EM_GETPASSWORDCHAR"}               // 0x0422
};

PSZ GetEMMsgName(WORD wMsg)
{
    INT i;
    register PMSGINFO pmi;

    for (pmi=amiEM,i=NUMEL(amiEM); i>0; i--,pmi++) {
        if ((WORD)pmi->uMsg == wMsg)
            return pmi->pszMsgName;
    }
    return GetWMMsgName(wMsg);
}

#endif


BOOL FASTCALL ThunkEMMsg16(LPMSGPARAMEX lpmpex)
{
    WORD wMsg = lpmpex->Parm16.WndProc.wMsg;

    LOGDEBUG(7,("    Thunking 16-bit edit control message %s(%04x)\n", (LPSZ)GetEMMsgName(wMsg), wMsg));

    wMsg -= WM_USER;

    //
    // For app defined (control) messages that are out of range
    // return TRUE.
    //
    // ChandanC Sept-15-1992
    //

    if (wMsg < (EM_GETPASSWORDCHAR - EM_GETSEL + 1)) {
        switch(lpmpex->uMsg = wMsg + EM_GETSEL) {

        case EM_GETSEL:
            // 16 bit apps cannot pass non-zero values in wParam or lParam for this
            // message to NT since they will be considered long pointers.
            // This is a hack for ReportWin - MarkRi

            // NOTE: There is a case possible where the app is trying to pass
            // thru a GETSEL msg that NT has sent it in which case things get more
            // complicated but we haven't found an app YET that has this problem.
            lpmpex->uParam = 0 ;
            lpmpex->lParam = 0 ;
            break ;


        case EM_SETSEL:
            lpmpex->uParam = LOWORD(lpmpex->Parm16.WndProc.lParam);
            lpmpex->lParam = HIWORD(lpmpex->Parm16.WndProc.lParam);
            break;

        case EM_GETLINE:
            GETMISCPTR(lpmpex->Parm16.WndProc.lParam, (LPSZ)lpmpex->lParam);
            break;

        case EM_GETRECT:
            lpmpex->lParam = (LONG)lpmpex->MsgBuffer;
            break;

        case EM_LINESCROLL:
            lpmpex->uParam = INT32(HIWORD(lpmpex->Parm16.WndProc.lParam));
            lpmpex->lParam = INT32(LOWORD(lpmpex->Parm16.WndProc.lParam));
            break;

        case EM_SETHANDLE:
            lpmpex->uParam = (UINT)MAKELONG(lpmpex->Parm16.WndProc.wParam,
                                  LOWORD(lpmpex->pww->hModule) | 1);
             break;

        case EM_REPLACESEL:
            {   PSZ psz;
                int i;
                GETPSZPTR(lpmpex->Parm16.WndProc.lParam, psz);

                if (psz) {
                    i = strlen(psz)+1;
                    lpmpex->lParam = (LONG) LocalAlloc (LMEM_FIXED, i);
                    RtlCopyMemory ((PSZ)lpmpex->lParam, psz, i);
                }
                FREEPSZPTR(psz);
            }
            break;

        case EM_SETRECT:
        case EM_SETRECTNP:
            if (lpmpex->Parm16.WndProc.lParam) {
                lpmpex->lParam = (LONG)lpmpex->MsgBuffer;
                getrect16((VPRECT16)lpmpex->Parm16.WndProc.lParam, (LPRECT)lpmpex->lParam);
            }
            break;

        case EM_SETTABSTOPS:
            {
                INT cItems = INT32(lpmpex->Parm16.WndProc.wParam);
                if (cItems > 0) {
                    (PVOID)lpmpex->lParam = STACKORHEAPALLOC(cItems * sizeof(INT),
                                   sizeof(lpmpex->MsgBuffer), lpmpex->MsgBuffer);
                    getintarray16((VPINT16)lpmpex->Parm16.WndProc.lParam, cItems, (LPINT)lpmpex->lParam);
                }
            }
            break;

        case EM_SETWORDBREAKPROC:
            if (lpmpex->Parm16.WndProc.lParam) {

                LONG l;

                l = lpmpex->Parm16.WndProc.lParam;

                // mark the proc as WOW proc and save the high bits in the RPL
                MarkWOWProc (l,lpmpex->lParam);

                LOGDEBUG (0, ("WOW::WMSGEM.C: EM_SETWORDBREAKPROC: lpmpex->Parm16.WndProc.lParam = %08lx, new lpmpex->Parm16.WndProc.lParam = %08lx\n", lpmpex->Parm16.WndProc.lParam, lpmpex->lParam));

            }
            break;

        case EM_GETSEL + 0x07:
        case EM_GETSEL + 0x0F:
        case EM_GETSEL + 0x10:
            lpmpex->uMsg = 0;
            break;
        } // switch
    }
    return TRUE;
}


VOID FASTCALL UnThunkEMMsg16(LPMSGPARAMEX lpmpex)
{

    LPARAM lParam = lpmpex->Parm16.WndProc.lParam;
    LPARAM lParamNew = lpmpex->lParam;

    switch(lpmpex->uMsg) {

    case EM_SETSEL:

        // EM_SETSEL no longer positions the caret on NT as Win3.1 did.  The new
        // procedure is to post or send an EM_SETSEL message and then if you
        // want the caret to be scrolled into view you send an EM_SCROLLCARET
        // message.  This code will do this to emulate the Win 3.1 EM_SETSEL
        // correctly on NT.

       if (!lpmpex->Parm16.WndProc.wParam) {
           DWORD dwT;

           if (POSTMSG(dwT))
              PostMessage(lpmpex->hwnd, EM_SCROLLCARET, 0, 0 );
           else
              SendMessage(lpmpex->hwnd, EM_SCROLLCARET, 0, 0 );
       }
       break;

    case EM_GETHANDLE:
        lpmpex->lReturn = GETHMEM16(lpmpex->lReturn);
        break;

    case EM_GETRECT:
        if (lParamNew) {
            putrect16((VPRECT16)lParam, (LPRECT)lParamNew);
        }
        break;

    case EM_REPLACESEL:
        if (lParamNew) {
            LocalFree ((HLOCAL)lParamNew);
        }
        break;

    case EM_SETTABSTOPS:
        if (lpmpex->Parm16.WndProc.wParam > 0) {
            STACKORHEAPFREE((LPINT)lParamNew, lpmpex->MsgBuffer);
        }
        break;

    case EM_GETWORDBREAKPROC:
        if (lpmpex->lReturn) {
            if (IsWOWProc (lpmpex->lReturn)) {

                LOGDEBUG (0, ("WOW::WMSGEM.C: EM_GETWORDBREAKPROC: lReturn = %08lx ", lpmpex->lReturn));

                //Unmark the proc and restore the high bits from rpl field
                UnMarkWOWProc (lpmpex->lReturn,lpmpex->lReturn);

                LOGDEBUG (0, (" and new lReturn = %08lx\n", lpmpex->lReturn));
            }
            else {
                PARM16 Parm16;
                LONG   lReturn;

                if (!WordBreakProc16) {

                    W32WordBreakProc = (WBP)(lpmpex->lReturn);

                    Parm16.SubClassProc.iOrdinal = FUN_WOWWORDBREAKPROC;

                    if (!CallBack16(RET_SUBCLASSPROC, &Parm16, (VPPROC)NULL,
                                   (PVPVOID)&lReturn)) {
                                    WOW32ASSERT(FALSE);
                        WordBreakProc16 = lpmpex->lReturn;
                    }
                }
                else {
                    lpmpex->lReturn = WordBreakProc16;
                }
            }
        }
        break;
    }
}
