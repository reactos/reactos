/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMSGSBM.C
 *  WOW32 16-bit message thunks for SCROLLBARs
 *
 *  History:
 *  Created 10-Jun-1992 by Bob Day (bobday)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wmsgbm.c);


#ifdef DEBUG

MSGINFO amiSBM[] = {
   {OLDSBM_SETPOS,      "SBM_SETPOS"},                          // 0x0400
   {OLDSBM_GETPOS,      "SBM_GETPOS"},                          // 0x0401
   {OLDSBM_SETRANGE,    "SBM_SETRANGE"},                        // 0x0402
   {OLDSBM_GETRANGE,    "SBM_GETRANGE"},                        // 0x0403
   {OLDSBM_ENABLEARROWS,"SBM_ENABLE_ARROWS"},                   // 0x0404
};

PSZ GetSBMMsgName(WORD wMsg)
{
    INT i;
    register PMSGINFO pmi;

    for (pmi=amiSBM,i=NUMEL(amiSBM); i>0; i--,pmi++)
        if ((WORD)pmi->uMsg == wMsg)
        return pmi->pszMsgName;
    return GetWMMsgName(wMsg);
}

#endif


BOOL FASTCALL ThunkSBMMsg16(LPMSGPARAMEX lpmpex)
{
    WORD wMsg = lpmpex->Parm16.WndProc.wMsg;
    LOGDEBUG(7,("    Thunking 16-bit scrollbar message %s(%04x)\n", (LPSZ)GetSBMMsgName(wMsg), wMsg));

    wMsg -= WM_USER;

    //
    // For app defined (control) messages that are out of range
    // return TRUE.
    //
    // ChandanC Sept-15-1992
    //


    if (wMsg < (SBM_ENABLE_ARROWS - SBM_SETPOS + 1)) {
        switch(lpmpex->uMsg = wMsg + SBM_SETPOS) {
    
            // The following messages should not require thunking, because
            // they contain no pointers, handles, or rearranged message parameters,
            // so consequently they are not documented in great detail here:
            //
            // SBM_SETPOS   (requires minimal thunking)
            // SBM_GETPOS
            // SBM_ENABLE_ARROWS
            //
    
            case SBM_GETRANGE:
    
                //
                // Changed semantics for this message to support 32-bit
                // scroll bar ranges (vs. 16-bit).
                //
                // Win16:
                //   posMin = LOWORD(SendMessage(hwnd, SBM_GETRANGE, 0, 0));
                //   posMax = HIWORD(SendMessage(hwnd, SBM_GETRANGE, 0, 0));
                //
                // Win32:
                //   SendMessage(hwnd, SBM_GETRANGE,
                //               (WPARAM) &posMin, (LPARAM) &posMax);
                //
    
                // Allocate buffers for 32-bit scrollbar proc to put
                // posMin and posMax in.
    
                lpmpex->uParam = (UINT)lpmpex->MsgBuffer;
                lpmpex->lParam = (LONG)((UINT *)lpmpex->uParam + 1);
                break;
    
            case SBM_SETRANGE:
    
                //
                // Changed semantics to support 32-bit scroll bar range:
                //
                // Win16:
                //   SendMessage(hwnd, SBM_SETRANGE, fRedraw, MAKELONG(posMin, posMax);
                //
                // Win32:
                //   SendMessage(hwnd, fRedraw ? SBM_SETRANGE : SBM_SETRANGEREDRAW,
                //               posMin, posMax);
                //
    
                if (lpmpex->Parm16.WndProc.wParam)
                    lpmpex->uMsg = SBM_SETRANGEREDRAW;
    
                lpmpex->uParam = INT32(LOWORD(lpmpex->Parm16.WndProc.lParam));
                lpmpex->lParam = INT32(HIWORD(lpmpex->Parm16.WndProc.lParam));
                break;
    
            case SBM_SETPOS:
                lpmpex->uParam = INT32(lpmpex->Parm16.WndProc.wParam);      // sign-extend the position
                break;
        }
    }
    return TRUE;
}


VOID FASTCALL UnThunkSBMMsg16(LPMSGPARAMEX lpmpex)
{
    switch (lpmpex->uMsg) {

        case SBM_GETRANGE:

            if (lpmpex->uParam && lpmpex->lParam) {
                lpmpex->lReturn = MAKELONG(*(UINT *)lpmpex->uParam, 
                                                     *(UINT *)lpmpex->lParam);
            }
            break;
    }

}
