/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    wcomdlg.c

Abstract:

    32-bit support for thunking COMMDLG in WOW

Author:

    John Vert (jvert) 31-Dec-1992

Revision History:

    John Vert (jvert) 31-Dec-1992
        created

--*/
#include "precomp.h"
#pragma   hdrstop
#include <cderr.h>
#include <dlgs.h>
#include <wowcmndg.h>

MODNAME(wcommdlg.c);

//
// Debugging stuff...
//
#if DEBUG
void WCDDumpCHOOSECOLORData16(PCHOOSECOLORDATA16 p16);
void WCDDumpCHOOSECOLORData32(CHOOSECOLOR *p32);
void WCDDumpCHOOSEFONTData16(PCHOOSEFONTDATA16 p16);
void WCDDumpCHOOSEFONTData32(CHOOSEFONT *p32);
void WCDDumpFINDREPLACE16(PFINDREPLACE16 p16);
void WCDDumpFINDREPLACE32(FINDREPLACE *p32);
void WCDDumpOPENFILENAME16(POPENFILENAME16 p16);
void WCDDumpOPENFILENAME32(OPENFILENAME *p32);
void WCDDumpPRINTDLGData16(PPRINTDLGDATA16 p16);
void WCDDumpPRINTDLGData32(PRINTDLG *p32);

// macros to dump the 16 & 32 bit structs
#define WCDDUMPCHOOSECOLORDATA16(p16)  WCDDumpCHOOSECOLORData16(p16)

#define WCDDUMPCHOOSECOLORDATA32(p32)  WCDDumpCHOOSECOLORData32(p32)

#define WCDDUMPCHOOSEFONTDATA16(p16)   WCDDumpCHOOSEFONTData16(p16)

#define WCDDUMPCHOOSEFONTDATA32(p32)   WCDDumpCHOOSEFONTData32(p32)

#define WCDDUMPFINDREPLACE16(p16)      WCDDumpFINDREPLACE16(p16)

#define WCDDUMPFINDREPLACE32(p32)      WCDDumpFINDREPLACE32(p32)

#define WCDDUMPOPENFILENAME16(p16)     WCDDumpOPENFILENAME16(p16)

#define WCDDUMPOPENFILENAME32(p32)     WCDDumpOPENFILENAME32(p32)

#define WCDDUMPPRINTDLGDATA16(p16)     WCDDumpPRINTDLGData16(p16)

#define WCDDUMPPRINTDLGDATA32(p32)     WCDDumpPRINTDLGData32(p32)

#else // !DEBUG

#define WCDDUMPCHOOSECOLORDATA16(p16)
#define WCDDUMPCHOOSECOLORDATA32(p32)
#define WCDDUMPCHOOSEFONTDATA16(p16)
#define WCDDUMPCHOOSEFONTDATA32(p32)
#define WCDDUMPOPENFILENAME16(p16)
#define WCDDUMPOPENFILENAME32(p32)
#define WCDDUMPPRINTDLGDATA16(p16)
#define WCDDUMPPRINTDLGDATA32(p32)
#define WCDDUMPFINDREPLACE16(p16)
#define WCDDUMPFINDREPLACE32(p32)

#endif  // !DEBUG




// global data
WORD msgCOLOROK        = 0;
WORD msgFILEOK         = 0;
WORD msgWOWLFCHANGE    = 0;
WORD msgWOWDIRCHANGE   = 0;
WORD msgWOWCHOOSEFONT  = 0;
WORD msgSHAREVIOLATION = 0;
WORD msgFINDREPLACE    = 0;

/* external global stuff */
extern WORD gwKrnl386CodeSeg1;
extern WORD gwKrnl386CodeSeg2;
extern WORD gwKrnl386CodeSeg3;
extern WORD gwKrnl386DataSeg1;


ULONG dwExtError = 0;
#define SETEXTENDEDERROR(Code) (dwExtError=Code)

/*+++ For reference only -- which flags are set on output
#define FR_OUTPUTFLAGS (FR_DOWN          | FR_WHOLEWORD     | FR_MATCHCASE  | \
                        FR_FINDNEXT      | FR_REPLACE       | FR_REPLACEALL | \
                        FR_DIALOGTERM    | FR_SHOWHELP      | FR_NOUPDOWN   | \
                        FR_NOMATCHCASE   | FR_NOWHOLEWORD   | FR_HIDEUPDOWN | \
                        FR_HIDEMATCHCASE | FR_HIDEWHOLEWORD)

#define PD_OUTPUTFLAGS (PD_ALLPAGES    | PD_COLLATE    | PD_PAGENUMS | \
                        PD_PRINTTOFILE | PD_SELECTION)

#define FO_OUTPUTFLAGS (OFN_READONLY | OFN_EXTENSIONDIFFERENT)

#define CF_OUTPUTFLAGS (CF_NOFACESEL | CF_NOSIZESEL | CF_NOSTYLESEL)
---*/


// private typedefs and structs
typedef BOOL (APIENTRY* FILENAMEPROC)(LPOPENFILENAME);
typedef HWND (APIENTRY* FINDREPLACEPROC)(LPFINDREPLACE);


// exported by comdlg32.dll to allow us to ultimately keep 16-bit common dialog
// structs in sync with the UNICODE version maintained by comdlg32.
extern VOID Ssync_ANSI_UNICODE_Struct_For_WOW(HWND  hDlg,
                                              BOOL  fANSI_To_UNICODE,
                                              DWORD dwID);

// private function prototypes
VOID
Thunk_OFNstrs16to32(IN OPENFILENAME    *pOFN32,
                    IN POPENFILENAME16  pOFN16);

BOOL
Alloc_OFN32_strs(IN OPENFILENAME    *pOFN32,
                 IN POPENFILENAME16  pOFN16);

VOID
Free_OFN32_strs(IN OPENFILENAME *pOFN32);

PCOMMDLGTD
GetCommdlgTd(IN HWND Hwnd32);

HINSTANCE
ThunkCDTemplate16to32(IN     HAND16  hInst16,
                      IN     DWORD   hPrintTemp16,
                      IN     VPVOID  vpTemplateName,
                      IN     DWORD   dwFlags16,
                      IN OUT DWORD  *pFlags,
                      IN     DWORD   ETFlag,
                      IN     DWORD   ETHFlag,
                      OUT    PPRES   pRes,
                      OUT    PBOOL   fError);

VOID
FreeCDTemplate32(IN PRES      pRes,
                 IN HINSTANCE hInst,
                 IN BOOL      bETFlag,
                 IN BOOL      bETHFlag);

PRES
GetTemplate16(IN HAND16  hInstance,
              IN VPCSTR  TemplateName,
              IN BOOLEAN UseHandle);


HGLOBAL
ThunkhDevMode16to32(IN HAND16 hDevMode16);

VOID
ThunkhDevMode32to16(IN OUT HAND16 *phDevMode16,
                    IN     HANDLE  hDevMode32);

HANDLE
ThunkhDevNames16to32(IN HAND16 hDevNames16);

VOID
ThunkhDevNames32to16(IN OUT HAND16 *phDevNames16,
                     IN     HANDLE  hDevNames);

VOID
ThunkCHOOSECOLOR16to32(OUT CHOOSECOLOR         *pCC32,
                       IN  PCHOOSECOLORDATA16   pCC16);

VOID
ThunkCHOOSECOLOR32to16(OUT PCHOOSECOLORDATA16  pCC16,
                       IN  CHOOSECOLOR        *pCC32);

VOID
ThunkCHOOSEFONT16to32(OUT CHOOSEFONT        *pCF32,
                      IN  PCHOOSEFONTDATA16  pCF16);

VOID
ThunkCHOOSEFONT32to16(OUT PCHOOSEFONTDATA16  pCF16,
                      IN  CHOOSEFONT        *pCF32);

VOID
ThunkFINDREPLACE16to32(OUT FINDREPLACE    *pFR32,
                       IN  PFINDREPLACE16  pFR16);

VOID
ThunkFINDREPLACE32to16(OUT PFINDREPLACE16  pFR16,
                       IN  FINDREPLACE    *pFR32);

BOOL
ThunkOPENFILENAME16to32(OUT OPENFILENAME    *pOFN32,
                        IN  POPENFILENAME16  pOFN16);

VOID
ThunkOPENFILENAME32to16(OUT POPENFILENAME16  pOFN16,
                        IN  OPENFILENAME    *pOFN32,
                        IN  BOOLEAN          bUpperStrings);

VOID
ThunkPRINTDLG16to32(OUT PRINTDLG        *pPD32,
                    IN  PPRINTDLGDATA16  pPD16);

VOID
ThunkPRINTDLG32to16(IN  VPVOID    vppd,
                    OUT PRINTDLG *pPD32);

VOID
ThunkCDStruct16to32(IN HWND         hDlg,
                    IN CHOOSECOLOR *pCC,
                    IN VPVOID       vp);

VOID
ThunkCDStruct32to16(IN HWND         hDlg,
                    IN VPVOID       vp,
                    IN CHOOSECOLOR *pCC);

UINT APIENTRY
WCD32CommonDialogProc(HWND       hdlg,
                      UINT       uMsg,
                      WPARAM     uParam,
                      LPARAM     lParam,
                      PCOMMDLGTD pCTD,
                      VPVOID     vpfnHook);

UINT APIENTRY
WCD32PrintSetupDialogProc(HWND   hdlg,
                          UINT   uMsg,
                          WPARAM uParam,
                          LPARAM lParam);

UINT APIENTRY
WCD32DialogProc(HWND   hdlg,
                UINT   uMsg,
                WPARAM uParam,
                LPARAM lParam);

ULONG
WCD32GetFileName(IN PVDMFRAME    pFrame,
                 IN FILENAMEPROC Function);

ULONG
WCD32FindReplaceText(IN PVDMFRAME       pFrame,
                     IN FINDREPLACEPROC Function);

UINT APIENTRY
WCD32FindReplaceDialogProc(HWND   hdlg,
                           UINT   uMsg,
                           WPARAM uParam,
                           LPARAM lParam);

#define VALID_OFN16_FLAGS (OFN_READONLY              | \
                           OFN_OVERWRITEPROMPT       | \
                           OFN_HIDEREADONLY          | \
                           OFN_NOCHANGEDIR           | \
                           OFN_SHOWHELP              | \
                           OFN_ENABLEHOOK            | \
                           OFN_ENABLETEMPLATE        | \
                           OFN_ENABLETEMPLATEHANDLE  | \
                           OFN_NOVALIDATE            | \
                           OFN_ALLOWMULTISELECT      | \
                           OFN_EXTENSIONDIFFERENT    | \
                           OFN_PATHMUSTEXIST         | \
                           OFN_FILEMUSTEXIST         | \
                           OFN_CREATEPROMPT          | \
                           OFN_SHAREAWARE            | \
                           OFN_NOREADONLYRETURN      | \
                           OFN_NOTESTFILECREATE)

//
// unique message thunks
//

// This function thunks the private messages
//      msgCOLOROK
BOOL FASTCALL
WM32msgCOLOROK(LPWM32MSGPARAMEX lpwm32mpex)
{
    LPCHOOSECOLOR       pCC32;
    PCHOOSECOLORDATA16  pCC16;

    GETVDMPTR((VPVOID)lpwm32mpex->Parm16.WndProc.lParam,
              sizeof(CHOOSECOLORDATA16),
              pCC16);

    pCC32 = (LPCHOOSECOLOR)lpwm32mpex->lParam;

    if(pCC16 && pCC32) {

        if(lpwm32mpex->fThunk) {
            ThunkCHOOSECOLOR32to16(pCC16, pCC32);
        }
        else {
            ThunkCHOOSECOLOR16to32(pCC32, pCC16);
        }
    }
    else {
        return(FALSE);
    }

    FREEVDMPTR(pCC16);

    return (TRUE);
}




// This function thunks the private messages
//      msgFILEOK
BOOL FASTCALL
WM32msgFILEOK(LPWM32MSGPARAMEX lpwm32mpex)
{
    VPOPENFILENAME   vpof;
    POPENFILENAME16  pOFN16;
    OPENFILENAME    *pOFN32;


    vpof = (VPOPENFILENAME)(GetCommdlgTd(lpwm32mpex->hwnd)->vpData);

    pOFN32 = (OPENFILENAME *)lpwm32mpex->lParam;
    //
    // Approach sends its own fileok message when you click on its
    // secret listbox that it displays over lst1 sometimes.  It
    // sends NULL for the LPARAM instead of the address of the
    // openfilename structure.
    if(pOFN32 == NULL) {
        lpwm32mpex->Parm16.WndProc.lParam = (LPARAM)NULL;
        return(TRUE);
    }

    GETVDMPTR(vpof, sizeof(OPENFILENAME16), pOFN16);

    if (lpwm32mpex->fThunk) {
        UpdateDosCurrentDirectory(DIR_NT_TO_DOS);
        lpwm32mpex->Parm16.WndProc.lParam = (LPARAM)vpof;

        // sudeepb 12-Mar-1996
        //
        // The selected file name needs to be uppercased for brain dead
        // apps like symanatec QA 4.0. So changed the following parameter
        // in ThunkOpenFileName from FALSE to TRUE.
        ThunkOPENFILENAME32to16(pOFN16, pOFN32, TRUE);

    } else {
        ThunkOPENFILENAME16to32(pOFN32, pOFN16);
    }

    FREEVDMPTR(pOFN16);

    return (TRUE);
}




// This function thunks the private messages
//      msgWOWDIRCHANGE
BOOL FASTCALL
WM32msgWOWDIRCHANGE(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        UpdateDosCurrentDirectory(DIR_NT_TO_DOS);
    }

    return (TRUE);
}





// This function thunks the private message
//      msgWOWLFCHANGE
BOOL FASTCALL
WM32msgWOWLFCHANGE(LPWM32MSGPARAMEX lpwm32mpex)
{
    VPCHOOSEFONTDATA  vpcf;
    PCHOOSEFONTDATA16 pCF16;


    vpcf = (VPCHOOSEFONTDATA)(GetCommdlgTd(lpwm32mpex->hwnd)->vpData);

    GETVDMPTR(vpcf, sizeof(CHOOSEFONTDATA16), pCF16);
    WOW32ASSERT(pCF16);

    if (lpwm32mpex->fThunk) {
        PUTLOGFONT16(DWORD32(pCF16->lpLogFont),
                     sizeof(LOGFONT),
                     (LPLOGFONT)lpwm32mpex->lParam);
    }

    FREEVDMPTR(pCF16);

    return(TRUE);
}





// This function thunks the private message
//      msgSHAREVIOLATION
BOOL FASTCALL
WM32msgSHAREVIOLATION(LPWM32MSGPARAMEX lpwm32mpex)
{
    INT cb;
    PLONG plParamNew = &lpwm32mpex->Parm16.WndProc.lParam;


    if (lpwm32mpex->fThunk) {
        if (lpwm32mpex->lParam) {
            cb = strlen((LPSZ)lpwm32mpex->lParam)+1;
            if (!(*plParamNew = malloc16(cb))) {
                return(FALSE);
            }
            putstr16((VPSZ)*plParamNew, (LPSZ)lpwm32mpex->lParam, cb);
        }
    } else {
        if (*plParamNew) {
            free16((VPVOID) *plParamNew);
        }
    }

    return (TRUE);
}





// This function thunks the private messages
//      WM_CHOOSEFONT_GETLOGFONT
BOOL FASTCALL
WM32msgCHOOSEFONTGETLOGFONT(LPWM32MSGPARAMEX lpwm32mpex)
{
    LOGFONT LogFont32;

    // The mere fact that we access the buffer after allowing the 16-bit
    // hook proc to step in breaks Serif PagePlus app which wants it's
    // hook proc to always have a shot and commdlg to check the return value.

    // If hook proc returns TRUE, no further action is taken
    //
    // This is the message an app sends the dialog if it wants to find
    // out what font is currently selected.
    //
    // We thunk this by sending yet another hackorama message to comdlg32,
    // who will then fill in the 32-bit structure we pass in so we can
    // thunk it back to the 16-bit structure.  Then we return TRUE so
    // comdlg32 doesn't reference the 16-bit logfont.
    //
    if (!lpwm32mpex->fThunk && !lpwm32mpex->lReturn) {
        SendMessage(lpwm32mpex->hwnd, msgWOWCHOOSEFONT, 0, (LPARAM)&LogFont32);

        PUTLOGFONT16(lpwm32mpex->lParam, sizeof(LOGFONT), &LogFont32);

        lpwm32mpex->lReturn = TRUE;
    }

    return (TRUE);
}






//
// Dialog callback hook thunks
//
UINT APIENTRY
WCD32DialogProc(HWND hdlg,
                UINT uMsg,
                WPARAM uParam,
                LPARAM lParam)
/*++

Routine Description:

    This is the hook proc used by ChooseColor, ChooseFont, GetOpenFileName,
    GetSaveFileName, and PrintDlg.  It pulls the 16-bit callback
    out of the thread data and calls the common dialog proc to do
    the rest of the work.

--*/
{
    PCOMMDLGTD Td;

    Td = GetCommdlgTd(hdlg);
    if(Td) {
        return(WCD32CommonDialogProc(hdlg,
                                     uMsg,
                                     uParam,
                                     lParam,
                                     Td,
                                     Td->vpfnHook));
    } else {
        return(0);
    }
}




UINT APIENTRY
WCD32PrintSetupDialogProc(HWND hdlg,
                          UINT uMsg,
                          WPARAM uParam,
                          LPARAM lParam)
/*++

Routine Description:

    This is the hook proc used by PrintSetup.  It is only used when
    the Setup button on the Print dialog directly creates the PrintSetup
    dialog.  We find the correct TD by looking for the TD of our owner
    window (which is the print dialog)

    It calls the common dialog proc to do the rest of the work.

--*/

{
    PCOMMDLGTD pTD;

    pTD = CURRENTPTD()->CommDlgTd;
    if(pTD) {
        while (pTD->SetupHwnd != GETHWND16(hdlg)) {
            pTD = pTD->Previous;
            if(!pTD) {
                WOW32ASSERT(FALSE);
                return(0);
            }
        }

        return(WCD32CommonDialogProc(hdlg,
                                     uMsg,
                                     uParam,
                                     lParam,
                                     pTD,
                                     pTD->vpfnSetupHook));
    } else {
        return(0);
    }

}




UINT APIENTRY
WCD32FindReplaceDialogProc(HWND hdlg,
                           UINT uMsg,
                           WPARAM uParam,
                           LPARAM lParam)
/*++

Routine Description:

    This is the hook proc used by FindText and ReplaceText. It does cleanup
    on WM_DESTROY and calls WCD32CommonDialogProc to handle the 16-bit
    dialog hook callback on all messages, if needed.

--*/

{
    PFINDREPLACE16 pFR16;
    VPFINDREPLACE  vpfr;
    LPFINDREPLACE  pFR32;
    PCOMMDLGTD     ptdDlg;
    PCOMMDLGTD     ptdOwner;
    UINT           uRet = FALSE;

    // If the ptdDlg is invalid, do nothing.
    ptdDlg = GetCommdlgTd(hdlg);
    if (ptdDlg == NULL) {
        return(uRet);
    }

    if (uMsg != WM_DESTROY) {

        // this will be FALSE if the app didn't specify a 16-bit hookproc
        // we always set the 32-bit hookproc in ThunkFINDREPLACE16to32()
        if (ptdDlg->vpfnHook) {

           uRet = WCD32CommonDialogProc(hdlg,
                                        uMsg,
                                        uParam,
                                        lParam,
                                        ptdDlg,
                                        ptdDlg->vpfnHook);
        }
    }
    else {

        pFR32 = (LPFINDREPLACE)ptdDlg->pData32;

        // UnLink both per thread data structs from the list.
        ptdOwner = GetCommdlgTd(pFR32->hwndOwner);
        CURRENTPTD()->CommDlgTd = ptdDlg->Previous;
        WOW32ASSERT(ptdOwner->Previous == ptdDlg);

        vpfr = ptdDlg->vpData;

        GETVDMPTR(vpfr, sizeof(FINDREPLACE16), pFR16);

        // CleanUp template if used.
        FreeCDTemplate32((PRES)ptdDlg->pRes,
                         pFR32->hInstance,
                         DWORD32(pFR16->Flags) & FR_ENABLETEMPLATE,
                         DWORD32(pFR16->Flags) & FR_ENABLETEMPLATEHANDLE);


        FREEVDMPTR(pFR16);

        // Free the per thread data structs.
        free_w(ptdDlg);
        free_w(ptdOwner);

        // Free the 32-bit FINDREPLACE structure.
        free_w(pFR32->lpstrFindWhat);
        free_w(pFR32->lpstrReplaceWith);
        free_w(pFR32);
    }

    if (uMsg == WM_INITDIALOG) {
        // Force COMDLG32!FindReplaceDialogProc to handle WM_INITDIALOG.
        uRet = TRUE;
    }

    return(uRet);
}





UINT APIENTRY
WCD32CommonDialogProc(HWND hdlg,
                      UINT uMsg,
                      WPARAM uParam,
                      LPARAM lParam,
                      PCOMMDLGTD pCTD,
                      VPVOID vpfnHook)
/*++

Routine Description:

    This thunks the 32-bit dialog callback into a 16-bit callback
    This is the common code used by all the dialog callback thunks that
    actually calls the 16-bit callback.

--*/

{
    BOOL            fSuccess;
    LPFNM32         pfnThunkMsg;
    WM32MSGPARAMEX  wm32mpex;
    BOOL            fMessageNeedsThunking;

    // If the app has GP Faulted we don't want to pass it any more input
    // This should be removed when USER32 does clean up on task death so
    // it doesn't call us - mattfe june 24 92

    // LOGDEBUG(10, ("CommonDialogProc In: %lX %X %X %lX\n",
    //         (DWORD)hdlg,
    //         uMsg,
    //         uParam,
    //         lParam));

    if(CURRENTPTD()->dwFlags & TDF_IGNOREINPUT) {

        LOGDEBUG(6,
                 ("    WCD32OpenFileDialog Ignoring Input Messsage %04X\n",
                 uMsg));

        WOW32ASSERTMSG(gfIgnoreInputAssertGiven,
         "WCD32CommonDialogProc: TDF_IGNOREINPUT hack was used, shouldn't be, "
         "please email DaveHart with repro instructions.  Hit 'g' to ignore "
         "this and suppress this assertion from now on.\n");

        gfIgnoreInputAssertGiven = TRUE;
        goto SilentError;
    }

#if DBG
    if(pCTD==NULL) {
        LOGDEBUG(0,("    WCD32OpenFileDialog ERROR: pCTD==NULL\n"));
        goto Error;
    }

    // If pCTD->vpfnHook is NULL, then something is broken;  we
    // certainly can't continue because we don't know what 16-bit func to call
    if(!vpfnHook) {
        LOGDEBUG(0,("    WCD32OpenFileDialog ERROR: no hook proc for message %04x Dlg = %08lx\n", uMsg, hdlg ));
        goto Error;
    }
#endif

    wm32mpex.Parm16.WndProc.hwnd   = GETHWND16(hdlg);
    wm32mpex.Parm16.WndProc.wMsg   = (WORD)uMsg;
    wm32mpex.Parm16.WndProc.wParam = (WORD)uParam;
    wm32mpex.Parm16.WndProc.lParam = (LONG)lParam;
    wm32mpex.Parm16.WndProc.hInst  = (WORD)GetWindowLong(hdlg, GWL_HINSTANCE);

    // On Win3.1, the app & the system share the ptr to the same structure that
    // the app passed to the common dialog API.  Therefore, when one side makes
    // a change to the struct, the other is aware of the change.  This is not
    // the case on NT since we thunk the struct into a 32-bit ANSI struct which
    // is then thunked into a 32-bit UNICODE struct in the comdlg32 code.  We
    // attempt to synchronize all these structs by rethunking them for every
    // message sent to the 16-bit side & for every API call the app makes.
    // See sync code in callback16(), fastwow bopping code, and w32Dispatch().
    // ComDlg32 thunks UNICODEtoANSI before calling us & ASNItoUNICODE when we
    // return.  Ug!!!  Apparently a fair number of apps depend on this
    // behavior since we've debugged this problem about 6 times to date and
    // each time we have put in special hacks for each case.  With any luck
    // this should be a general fix.   08/97   CMJones

    if(uMsg < 0x400) {

        LOGDEBUG(3,
                 ("%04X (%s)\n",
                 CURRENTPTD()->htask16,
                 (aw32Msg[uMsg].lpszW32)));

        pfnThunkMsg = aw32Msg[uMsg].lpfnM32;

        if(uMsg == WM_INITDIALOG) {

            // The address of the 16-bit structure that the app passed to the
            // original common dialog API is passed in lParam in WM_INITDIALOG
            // messages in Win 3.1
            wm32mpex.Parm16.WndProc.lParam = lParam = (LPARAM)pCTD->vpData;
        }

    // Check for unique messages
    } else if(uMsg >= 0x400) {
        if (uMsg == msgFILEOK) {
            pfnThunkMsg = WM32msgFILEOK;
        } else if(uMsg == msgCOLOROK) {
            wm32mpex.Parm16.WndProc.lParam = (LPARAM)pCTD->vpData;
            pfnThunkMsg = WM32msgCOLOROK;
        } else if(uMsg == msgSHAREVIOLATION) {
            pfnThunkMsg = WM32msgSHAREVIOLATION;
        } else if(uMsg == msgWOWDIRCHANGE) {
            pfnThunkMsg = WM32msgWOWDIRCHANGE;
        } else if(uMsg == msgWOWLFCHANGE) {
            pfnThunkMsg = WM32msgWOWLFCHANGE;
        } else if(pCTD->Flags & WOWCD_ISCHOOSEFONT) {

            // special ChooseFont thunks to handle goofy GETLOGFONT message
            if(uMsg == WM_CHOOSEFONT_GETLOGFONT) {

                pfnThunkMsg = WM32msgCHOOSEFONTGETLOGFONT;

            } else if(uMsg == msgWOWCHOOSEFONT) {
                //
                // no wow app will expect this, so don't send it.
                //
                return(FALSE);
            } else {
                pfnThunkMsg = WM32NoThunking;
            }
        } else {
            pfnThunkMsg = WM32NoThunking;
        }
    }

    fMessageNeedsThunking = (pfnThunkMsg != WM32NoThunking);
    if(fMessageNeedsThunking) {
        wm32mpex.fThunk = THUNKMSG;
        wm32mpex.hwnd = hdlg;
        wm32mpex.uMsg = uMsg;
        wm32mpex.uParam = uParam;
        wm32mpex.lParam = lParam;
        wm32mpex.pww = NULL;
        wm32mpex.lpfnM32 = pfnThunkMsg;

        if(!(pfnThunkMsg)(&wm32mpex)) {
            LOGDEBUG(LOG_ERROR,("    WCD32OpenFileDialog ERROR: cannot thunk 32-bit message %04x\n", uMsg));
            goto Error;
        }
    } else {
        LOGDEBUG(6,("WCD32CommonDialogProc, No Thunking was required for the 32-bit message %s(%04x)\n", (LPSZ)GetWMMsgName(uMsg), uMsg));
    }

    // this call may cause 16-bit memory to move
    // this call will call 32->16 sync code before the callback & the 16->32
    // sync upon return from the callback
    fSuccess = CallBack16(RET_WNDPROC,
                          &wm32mpex.Parm16,
                          vpfnHook,
                          (PVPVOID)&wm32mpex.lReturn);

    // flat ptrs to 16-bit mem are now invalid due to possible memory movement

    // the callback function of a dialog is of type FARPROC whose return value
    // is of type 'int'. Since dx:ax is copied into lReturn in the above
    // CallBack16 call, we need to zero out the hiword, otherwise we will be
    // returning an erroneous value.
    wm32mpex.lReturn = (LONG)LOWORD(wm32mpex.lReturn);

    if(fMessageNeedsThunking) {
        wm32mpex.fThunk = UNTHUNKMSG;
        (pfnThunkMsg)(&wm32mpex);
    }

    if(!fSuccess)
        goto Error;

Done:
    // Uncomment this to receive message on exit
    // LOGDEBUG(10, ("CommonDialogProc Out: Return %lX\n", wm32mpex.lReturn));

    return wm32mpex.lReturn;

Error:
    LOGDEBUG(5,("    WCD32OpenFileDialog WARNING: cannot call back, using default message handling\n"));
SilentError:
    wm32mpex.lReturn = 0;
    goto Done;
}






ULONG FASTCALL
WCD32ExtendedError( IN PVDMFRAME pFrame )
/*++

Routine Description:

    32-bit thunk for CommDlgExtendedError()

Arguments:

    pFrame - Supplies 16-bit argument frame

Return Value:

    error code to be returned

--*/
{
    if (dwExtError != 0) {
        return(dwExtError);
    }
    return(CommDlgExtendedError());
}







ULONG FASTCALL
WCD32ChooseColor(PVDMFRAME pFrame)
/*++

Routine Description:

    This routine thunks the 16-bit ChooseColor common dialog to the 32-bit
    side.

Arguments:

    pFrame - Supplies 16-bit argument frame

Return Value:

    16-bit BOOLEAN to be returned.

--*/
{
    ULONG                   ul = GETBOOL16(FALSE);
    register PCHOOSECOLOR16 parg16;
    VPCHOOSECOLORDATA       vpcc;
    CHOOSECOLOR             CC32;
    PCHOOSECOLORDATA16      pCC16;
    PRES                    pRes = NULL;
    COMMDLGTD               ThreadData;
    COLORREF                CustColors32[16];  // on stack for DWORD alignment
    DWORD                   dwFlags16;
    BOOL                    fError = FALSE;


    GETARGPTR(pFrame, sizeof(CHOOSECOLOR16), parg16);
    vpcc = parg16->lpcc;

    SETEXTENDEDERROR( 0 );

    // invalidate this now
    FREEVDMPTR( parg16 );

    // initialize unique window message
    if (msgCOLOROK == 0) {
        if(!(msgCOLOROK = (WORD)RegisterWindowMessage(COLOROKSTRING))) {
            SETEXTENDEDERROR( CDERR_REGISTERMSGFAIL );
            LOGDEBUG(2,("WCD32ChooseColor:RegisterWindowMessage failed\n"));
            return(0);
        }
    }

    GETVDMPTR(vpcc, sizeof(CHOOSECOLORDATA16), pCC16);

    WCDDUMPCHOOSECOLORDATA16(pCC16);

    if(DWORD32(pCC16->lStructSize) != sizeof(CHOOSECOLORDATA16)) {
        SETEXTENDEDERROR( CDERR_STRUCTSIZE );
        FREEVDMPTR(pCC16);
        return(0);
    }

    RtlZeroMemory(&ThreadData, sizeof(COMMDLGTD));
    ThreadData.Previous = CURRENTPTD()->CommDlgTd;
    ThreadData.hdlg     = (HWND16)-1;
    ThreadData.pData32  = &CC32;
    ThreadData.Flags    = 0;
    if(DWORD32(pCC16->Flags) & CC_ENABLEHOOK) {
        ThreadData.vpfnHook = DWORD32(pCC16->lpfnHook);
        if(!ThreadData.vpfnHook) {
            SETEXTENDEDERROR(CDERR_NOHOOK);
            FREEVDMPTR(pCC16);
            return(0);
        }
        ThreadData.vpData   = vpcc;
    }
    else {
        STOREDWORD(pCC16->lpfnHook, 0);
    }

    RtlZeroMemory(&CC32, sizeof(CHOOSECOLOR));
    CC32.lpCustColors = CustColors32;
    ThunkCHOOSECOLOR16to32(&CC32, pCC16);
    dwFlags16 = DWORD32(pCC16->Flags);

    // this call invalidates flat ptrs to 16-bit memory
    CC32.hInstance = (HWND)ThunkCDTemplate16to32(WORD32(pCC16->hInstance),
                                                 0,
                                                 DWORD32(pCC16->lpTemplateName),
                                                 dwFlags16,
                                                 &(CC32.Flags),
                                                 CC_ENABLETEMPLATE,
                                                 CC_ENABLETEMPLATEHANDLE,
                                                 &pRes,
                                                 &fError);

    if(fError) {
        goto ChooseColorExit;
    }

    // invalidate flat ptrs to 16-bit memory
    FREEVDMPTR(pCC16);

    WCDDUMPCHOOSECOLORDATA32(&CC32);

    // Set this just before the calling into comdlg32.  This prevents the
    // synchronization stuff from firing until we actually need it.
    CURRENTPTD()->CommDlgTd = &ThreadData;

    // this call invalidates flat ptrs to 16-bit memory
    ul = GETBOOL16(ChooseColor(&CC32));

    CURRENTPTD()->CommDlgTd = ThreadData.Previous;

    if (ul) {

        WCDDUMPCHOOSECOLORDATA32(&CC32);

        GETVDMPTR(vpcc, sizeof(CHOOSECOLOR16), pCC16);
        ThunkCHOOSECOLOR32to16(pCC16, &CC32);

        WCDDUMPCHOOSECOLORDATA16(pCC16);
        FREEVDMPTR(pCC16);

    }

ChooseColorExit:

    FreeCDTemplate32(pRes,
                     (HINSTANCE)CC32.hInstance,
                     dwFlags16 & CC_ENABLETEMPLATE,
                     dwFlags16 & CC_ENABLETEMPLATEHANDLE);

    FREEVDMPTR(pCC16);

    return(ul);
}




VOID
ThunkCHOOSECOLOR16to32(OUT CHOOSECOLOR        *pCC32,
                       IN  PCHOOSECOLORDATA16  pCC16)
{
    COLORREF *pCustColors16;
    DWORD     Flags;


    if(pCC16 && pCC32) {

        pCC32->lStructSize = sizeof(CHOOSECOLOR);
        pCC32->hwndOwner   = HWND32(pCC16->hwndOwner);

        // hInstance thunked separately

        pCC32->rgbResult   = DWORD32(pCC16->rgbResult);

        if(pCC32->lpCustColors) {
            GETVDMPTR(pCC16->lpCustColors, 16*sizeof(COLORREF), pCustColors16);
            if(pCustColors16) {
                RtlCopyMemory(pCC32->lpCustColors,
                              pCustColors16,
                              16*sizeof(COLORREF));
            }
            FREEVDMPTR(pCustColors16);
        }

        // preserve the template flag state while copying flags
        // 1. save template flag state
        //     note: we never will have a 32-bit CC_ENABLETEMPLATE flag
        // 2. copy flags from 16-bit struct (add the WOWAPP flag)
        // 3. turn off all template flags
        // 4. restore original template flag state
        Flags         = pCC32->Flags & CC_ENABLETEMPLATEHANDLE;
        pCC32->Flags  = DWORD32(pCC16->Flags) | CD_WOWAPP;
        pCC32->Flags &= ~(CC_ENABLETEMPLATE | CC_ENABLETEMPLATEHANDLE);
        pCC32->Flags |= Flags;

        pCC32->lCustData   = DWORD32(pCC16->lCustData);

        if((DWORD32(pCC16->Flags) & CC_ENABLEHOOK) && DWORD32(pCC16->lpfnHook)){
            pCC32->lpfnHook = WCD32DialogProc;
        }

        // lpTemplateName32 is thunked separately
    }
}




VOID
ThunkCHOOSECOLOR32to16(OUT PCHOOSECOLORDATA16  pCC16,
                       IN  CHOOSECOLOR        *pCC32)
{
    COLORREF *pCustColors16;
    DWORD     Flags, Flags32;


    if(pCC16 && pCC32) {

        STOREDWORD(pCC16->rgbResult, pCC32->rgbResult);

        // preserve the template flag state while copying flags
        // 1. save template flag state
        // 2. copy flags from 32-bit struct
        // 3. turn off all template flags and the WOWAPP flag
        // 4. restore original template flag state
        Flags    = DWORD32(pCC16->Flags) & (CC_ENABLETEMPLATE |
                                            CC_ENABLETEMPLATEHANDLE);
        Flags32  = pCC32->Flags;
        Flags32 &= ~(CC_ENABLETEMPLATE | CC_ENABLETEMPLATEHANDLE | CD_WOWAPP);
        Flags32 |= Flags;
        STOREDWORD(pCC16->Flags, Flags32);

        GETVDMPTR(pCC16->lpCustColors, 16*sizeof(COLORREF), pCustColors16);
        if(pCustColors16) {
            RtlCopyMemory(pCustColors16,
                          pCC32->lpCustColors,
                          16*sizeof(COLORREF));
            FREEVDMPTR(pCustColors16);
        }
    }
}





ULONG FASTCALL
WCD32ChooseFont( PVDMFRAME pFrame )
/*++

Routine Description:

    This routine thunks the 16-bit ChooseFont common dialog to the 32-bit
    side.

Arguments:

    pFrame - Supplies 16-bit argument frame

Return Value:

    16-bit BOOLEAN to be returned.

--*/
{
    ULONG                   ul = GETBOOL16(FALSE);
    register PCHOOSEFONT16  parg16;
    VPCHOOSEFONTDATA        vpcf;
    CHOOSEFONT              CF32;
    LOGFONT                 LogFont32;
    PCHOOSEFONTDATA16       pCF16;
    PRES                    pRes = NULL;
    COMMDLGTD               ThreadData;
    DWORD                   dwFlags16;
    CHAR                    sStyle[2 * LF_FACESIZE];
    BOOL                    fError = FALSE;


    GETARGPTR(pFrame, sizeof(CHOOSEFONT16), parg16);
    vpcf = parg16->lpcf;

    SETEXTENDEDERROR( 0 );

    // invalidate this now
    FREEVDMPTR( parg16 );

    // initialize unique window messages
    if (msgWOWCHOOSEFONT == 0) {

        // private WOW<->comdlg32 message for handling WM_CHOOSEFONT_GETLOGFONT
        if(!(msgWOWCHOOSEFONT  =
                     (WORD)RegisterWindowMessage("WOWCHOOSEFONT_GETLOGFONT"))) {
            SETEXTENDEDERROR( CDERR_REGISTERMSGFAIL );
            LOGDEBUG(2,("WCD32ChooseFont:RegisterWindowMessage failed\n"));
            return(0);
        }
    }
    if (msgWOWLFCHANGE == 0) {

        // private message for thunking logfont changes
        if(!(msgWOWLFCHANGE = (WORD)RegisterWindowMessage("WOWLFChange"))) {
            SETEXTENDEDERROR( CDERR_REGISTERMSGFAIL );
            LOGDEBUG(2,("WCD32ChooseFont:RegisterWindowMessage 2 failed\n"));
            return(0);
        }
    }

    GETVDMPTR(vpcf, sizeof(CHOOSEFONTDATA16), pCF16);

    WCDDUMPCHOOSEFONTDATA16(pCF16);

    if(DWORD32(pCF16->lStructSize) != sizeof(CHOOSEFONTDATA16)) {
        SETEXTENDEDERROR( CDERR_STRUCTSIZE );
        FREEVDMPTR(pCF16);
        return(0);
    }

    RtlZeroMemory(&ThreadData, sizeof(COMMDLGTD));
    ThreadData.Previous = CURRENTPTD()->CommDlgTd;
    ThreadData.hdlg     = (HWND16)-1;
    ThreadData.pData32  = &CF32;
    ThreadData.Flags    = WOWCD_ISCHOOSEFONT;
    if(DWORD32(pCF16->Flags) & CF_ENABLEHOOK) {
        ThreadData.vpfnHook = DWORD32(pCF16->lpfnHook);
        if(!ThreadData.vpfnHook) {
            SETEXTENDEDERROR(CDERR_NOHOOK);
            FREEVDMPTR(pCF16);
            return(0);
        }
        ThreadData.vpData   = vpcf;
    }
    else {
        STOREDWORD(pCF16->lpfnHook, 0);
    }

    RtlZeroMemory(&CF32, sizeof(CHOOSEFONT));
    CF32.lpLogFont = &LogFont32;
    CF32.lpszStyle = sStyle;
    sStyle[0] = '\0';
    ThunkCHOOSEFONT16to32(&CF32, pCF16);
    dwFlags16 = DWORD32(pCF16->Flags);

    // this call invalidates flat ptrs to 16-bit memory
    CF32.hInstance = ThunkCDTemplate16to32(WORD32(pCF16->hInstance),
                                           0,
                                           DWORD32(pCF16->lpTemplateName),
                                           dwFlags16,
                                           &(CF32.Flags),
                                           CF_ENABLETEMPLATE,
                                           CF_ENABLETEMPLATEHANDLE,
                                           &pRes,
                                           &fError);

    if(fError) {
        goto ChooseFontExit;
    }

    // invalidate flat ptrs to 16-bit memory
    FREEVDMPTR(pCF16);

    WCDDUMPCHOOSEFONTDATA32(&CF32);

    // Set this just before the calling into comdlg32.  This prevents the
    // synchronization stuff from firing until we actually need it.
    CURRENTPTD()->CommDlgTd = &ThreadData;

    // this call invalidates flat ptrs to 16-bit memory
    ul = GETBOOL16(ChooseFont(&CF32));

    CURRENTPTD()->CommDlgTd = ThreadData.Previous;

    if (ul) {

        WCDDUMPCHOOSEFONTDATA32(&CF32);

        GETVDMPTR(vpcf, sizeof(CHOOSEFONT16), pCF16);
        ThunkCHOOSEFONT32to16(pCF16, &CF32);

        WCDDUMPCHOOSEFONTDATA16(pCF16);

    }

ChooseFontExit:

    FreeCDTemplate32(pRes,
                     CF32.hInstance,
                     dwFlags16 & CF_ENABLETEMPLATE,
                     dwFlags16 & CF_ENABLETEMPLATEHANDLE);

    FREEVDMPTR(pCF16);

    return(ul);
}





VOID
ThunkCHOOSEFONT16to32(OUT CHOOSEFONT        *pCF32,
                      IN  PCHOOSEFONTDATA16  pCF16)
{
    LPSTR lpstr;
    DWORD Flags;


    if(pCF16 && pCF32) {

        pCF32->lStructSize = sizeof(CHOOSEFONT);
        pCF32->hwndOwner   = HWND32(pCF16->hwndOwner);

        if(DWORD32(pCF16->Flags) & CF_PRINTERFONTS) {
            pCF32->hDC = HDC32(pCF16->hDC);
        }

        if(DWORD32(pCF16->lpLogFont) && pCF32->lpLogFont) {
            GETLOGFONT16(DWORD32(pCF16->lpLogFont), pCF32->lpLogFont);
        }

        pCF32->iPointSize  = INT32(pCF16->iPointSize);

        // preserve the template flag state while copying flags
        // 1. save template flag state
        //     note: we never will have a 32-bit CF_ENABLETEMPLATE flag
        // 2. copy flags from 16-bit struct (add the WOWAPP flag)
        // 3. turn off all template flags
        // 4. restore original template flag state
        Flags         = pCF32->Flags & CF_ENABLETEMPLATEHANDLE;
        pCF32->Flags  = DWORD32(pCF16->Flags) | CD_WOWAPP;
        pCF32->Flags &= ~(CF_ENABLETEMPLATE | CF_ENABLETEMPLATEHANDLE);
        pCF32->Flags |= Flags;

        pCF32->rgbColors   = DWORD32(pCF16->rgbColors);
        pCF32->lCustData   = DWORD32(pCF16->lCustData);

        if((DWORD32(pCF16->Flags) & CF_ENABLEHOOK) && pCF16->lpfnHook) {
            pCF32->lpfnHook = WCD32DialogProc;
        }

        // lpTemplateName32 is thunked separately
        // hInstance thunked separately

        // Note: we shouldn't have to free or re-alloc this since they
        //       will only need LF_FACESIZE bytes to handle the string
        GETPSZPTR(pCF16->lpszStyle, lpstr);
        if(lpstr && pCF32->lpszStyle) {
            if(DWORD32(pCF16->Flags) & CF_USESTYLE) {
                strcpy(pCF32->lpszStyle, lpstr);
            }
            FREEPSZPTR(lpstr);
        }

        pCF32->nFontType   = WORD32(pCF16->nFontType);
        pCF32->nSizeMin    = INT32(pCF16->nSizeMin);
        pCF32->nSizeMax    = INT32(pCF16->nSizeMax);
    }
}





VOID
ThunkCHOOSEFONT32to16(OUT PCHOOSEFONTDATA16  pCF16,
                      IN  CHOOSEFONT        *pCF32)
{
    LPSTR lpstr;
    DWORD Flags, Flags32;


    if(pCF16 && pCF32) {

        STOREWORD(pCF16->iPointSize, pCF32->iPointSize);
        STOREDWORD(pCF16->rgbColors, pCF32->rgbColors);
        STOREWORD(pCF16->nFontType,  pCF32->nFontType);

        // preserve the template flag state while copying flags
        // 1. save template flag state
        // 2. copy flags from 32-bit struct
        // 3. turn off all template flags and the WOWAPP flag
        // 4. restore original template flag state
        Flags    = DWORD32(pCF16->Flags) & (CF_ENABLETEMPLATE |
                                            CF_ENABLETEMPLATEHANDLE);
        Flags32  = pCF32->Flags;
        Flags32 &= ~(CF_ENABLETEMPLATE | CF_ENABLETEMPLATEHANDLE | CD_WOWAPP);
        Flags32 |= Flags;
        STOREDWORD(pCF16->Flags, Flags32);

        if(DWORD32(pCF16->lpLogFont) && pCF32->lpLogFont) {
            PUTLOGFONT16(DWORD32(pCF16->lpLogFont),
                         sizeof(LOGFONT),
                         pCF32->lpLogFont);
        }

        GETPSZPTR(pCF16->lpszStyle, lpstr);
        if(lpstr && pCF32->lpszStyle) {
            if(DWORD32(pCF16->Flags) & CF_USESTYLE) {
                strcpy(lpstr, pCF32->lpszStyle);
            }
            FREEPSZPTR(lpstr);
        }
    }
}







ULONG FASTCALL
WCD32PrintDlg(IN PVDMFRAME pFrame)
/*++

Routine Description:

    This routine thunks the 16-bit PrintDlg common dialog to the 32-bit
    side.

Arguments:

    pFrame - Supplies 16-bit argument frame

Return Value:

    16-bit BOOLEAN to be returned

--*/
{
    ULONG                  ul = GETBOOL16(FALSE);
    register PPRINTDLG16   parg16;
    VPPRINTDLGDATA         vppd;
    PRINTDLG               PD32;
    PPRINTDLGDATA16        pPD16;
    PRES                   hSetupRes = NULL;
    PRES                   hPrintRes = NULL;
    COMMDLGTD              ThreadData;
    DWORD                  dwFlags16;
    HMEM16                 hDM16;
    HMEM16                 hDN16;
    BOOL                   fError = FALSE;


    GETARGPTR(pFrame, sizeof(PRINTDLG16), parg16);
    vppd = parg16->lppd;

    // invalidate this now
    FREEARGPTR(parg16);

    SETEXTENDEDERROR(0);

    GETVDMPTR(vppd, sizeof(PRINTDLGDATA16), pPD16);

    WCDDUMPPRINTDLGDATA16(pPD16);

    if(DWORD32(pPD16->lStructSize) != sizeof(PRINTDLGDATA16)) {
        SETEXTENDEDERROR( CDERR_STRUCTSIZE );
        FREEVDMPTR(pPD16);
        return(0);
    }

    if(DWORD32(pPD16->Flags) & PD_RETURNDEFAULT) {
        // spec says these must be NULL
        if(WORD32(pPD16->hDevMode) || WORD32(pPD16->hDevNames)) {
            SETEXTENDEDERROR(PDERR_RETDEFFAILURE);
            FREEVDMPTR(pPD16);
            return(0);
        }
    }

    RtlZeroMemory((PVOID)&PD32, sizeof(PRINTDLG));
    RtlZeroMemory((PVOID)&ThreadData, sizeof(COMMDLGTD));
    ThreadData.Previous = CURRENTPTD()->CommDlgTd;
    ThreadData.hdlg     = (HWND16)-1;
    ThreadData.pData32  = (PVOID)&PD32;
    ThreadData.Flags    = 0;

    // this flag causes the system to put up the setup dialog rather
    // than the print dialog
    if(DWORD32(pPD16->Flags) & PD_PRINTSETUP) {
        if(DWORD32(pPD16->Flags) & PD_ENABLESETUPHOOK) {
            ThreadData.vpfnHook = DWORD32(pPD16->lpfnSetupHook);
            if(!ThreadData.vpfnHook) {
                SETEXTENDEDERROR(CDERR_NOHOOK);
                FREEVDMPTR(pPD16);
                return(0);
            }
            ThreadData.vpData = vppd;
            PD32.lpfnSetupHook = WCD32DialogProc;
        }
    } else {
        if (DWORD32(pPD16->Flags) & PD_ENABLEPRINTHOOK) {
            ThreadData.vpfnHook = DWORD32(pPD16->lpfnPrintHook);
            if(!ThreadData.vpfnHook) {
                SETEXTENDEDERROR(CDERR_NOHOOK);
                FREEVDMPTR(pPD16);
                return(0);
            }
            ThreadData.vpData = vppd;
            PD32.lpfnPrintHook = WCD32DialogProc;
        }
        if (DWORD32(pPD16->Flags) & PD_ENABLESETUPHOOK) {
            ThreadData.vpfnSetupHook = DWORD32(pPD16->lpfnSetupHook);
            if(!ThreadData.vpfnSetupHook) {
                SETEXTENDEDERROR(CDERR_NOHOOK);
                FREEVDMPTR(pPD16);
                return(0);
            }
            ThreadData.vpData    = vppd;
            ThreadData.SetupHwnd = (HWND16)1;
            PD32.lpfnSetupHook   = WCD32PrintSetupDialogProc;
        }
    }

    // lock the original 16-bit hDevMode & hDevNames so they won't get thrown
    // out by our thunking.  (we need to restore them to the original handles
    // if there is an error in PrintDlg() ).
    hDM16 = WORD32(pPD16->hDevMode);
    hDN16 = WORD32(pPD16->hDevNames);
    WOWGlobalLock16(hDM16);
    WOWGlobalLock16(hDN16);

    dwFlags16 = DWORD32(pPD16->Flags);

    // get a new 32-bit devmode struct
    PD32.hDevMode  = ThunkhDevMode16to32(WORD32(pPD16->hDevMode));

    // get a new 32-bit devnames struct
    PD32.hDevNames = ThunkhDevNames16to32(WORD32(pPD16->hDevNames));

    ThunkPRINTDLG16to32(&PD32, pPD16);

    GETVDMPTR(vppd, sizeof(PRINTDLGDATA16), pPD16);

    // this call invalidates flat ptrs to 16-bit memory
    PD32.hPrintTemplate
              = ThunkCDTemplate16to32(WORD32(pPD16->hInstance),
                                      MAKELONG(WORD32(pPD16->hPrintTemplate),1),
                                      DWORD32(pPD16->lpPrintTemplateName),
                                      dwFlags16,
                                      &(PD32.Flags),
                                      PD_ENABLEPRINTTEMPLATE,
                                      PD_ENABLEPRINTTEMPLATEHANDLE,
                                      &hPrintRes,
                                      &fError);

    if(fError) {
        goto PrintDlgError;
    }

    // memory may have moved - invalidate flat pointers now
    FREEVDMPTR(pPD16);

    GETVDMPTR(vppd, sizeof(PRINTDLGDATA16), pPD16);

    // this call invalidates flat ptrs to 16-bit memory
    PD32.hSetupTemplate
              = ThunkCDTemplate16to32(WORD32(pPD16->hInstance),
                                      MAKELONG(WORD32(pPD16->hSetupTemplate),1),
                                      DWORD32(pPD16->lpSetupTemplateName),
                                      dwFlags16,
                                      &(PD32.Flags),
                                      PD_ENABLESETUPTEMPLATE,
                                      PD_ENABLESETUPTEMPLATEHANDLE,
                                      &hSetupRes,
                                      &fError);

PrintDlgError:
    if(fError) {
        WOWGlobalUnlock16(hDM16);
        WOWGlobalUnlock16(hDN16);
        goto PrintDlgExit;
    }

    // memory may have moved - invalidate flat pointers now
    FREEVDMPTR(pPD16);

    WCDDUMPPRINTDLGDATA32(&PD32);

    // Set this just before the calling into comdlg32.  This prevents the
    // synchronization stuff from firing until we actually need it.
    CURRENTPTD()->CommDlgTd = &ThreadData;

    ul = GETBOOL16(PrintDlg(&PD32));

    CURRENTPTD()->CommDlgTd = ThreadData.Previous;

    // blow away our locks so these really can be free'd if needed
    WOWGlobalUnlock16(hDM16);
    WOWGlobalUnlock16(hDN16);

    if(ul) {

        WCDDUMPPRINTDLGDATA32(&PD32);

        // this call invalidates flat ptrs to 16-bit mem
        ThunkPRINTDLG32to16(vppd, &PD32);

        GETVDMPTR(vppd, sizeof(PRINTDLGDATA16), pPD16);

        WCDDUMPPRINTDLGDATA16(pPD16);

        // throw out the old ones if the structs were updated
        if(WORD32(pPD16->hDevMode) != hDM16) {
            WOWGlobalFree16(hDM16);
        }
        if(WORD32(pPD16->hDevNames) != hDN16) {
            WOWGlobalFree16(hDN16);
        }

    } else {

        // throw away any new hDevMode's & hDevNames that we might have created
        // as a result of our thunking & restore the originals
        GETVDMPTR(vppd, sizeof(PRINTDLGDATA16), pPD16);
        if(WORD32(pPD16->hDevMode) != hDM16) {
            WOWGlobalFree16(WORD32(pPD16->hDevMode));
            STOREWORD(pPD16->hDevMode, hDM16);
        }
        if(WORD32(pPD16->hDevNames) != hDN16) {
            WOWGlobalFree16(WORD32(pPD16->hDevNames));
            STOREWORD(pPD16->hDevNames, hDN16);
        }
    }

PrintDlgExit:

    WOWGLOBALFREE(PD32.hDevMode);
    WOWGLOBALFREE(PD32.hDevNames);

    if(PD32.hPrintTemplate) {
        FreeCDTemplate32(hPrintRes,
                         PD32.hPrintTemplate,
                         dwFlags16 & PD_ENABLEPRINTTEMPLATE,
                         dwFlags16 & PD_ENABLEPRINTTEMPLATEHANDLE);
    }

    if(PD32.hSetupTemplate) {
        FreeCDTemplate32(hSetupRes,
                         PD32.hSetupTemplate,
                         dwFlags16 & PD_ENABLESETUPTEMPLATE,
                         dwFlags16 & PD_ENABLESETUPTEMPLATEHANDLE);
    }

    FREEVDMPTR(pPD16);

    return(ul);
}


#define PD_TEMPLATEMASK32        (PD_ENABLEPRINTTEMPLATE         | \
                                  PD_ENABLESETUPTEMPLATE)

#define PD_TEMPLATEHANDLEMASK32  (PD_ENABLEPRINTTEMPLATEHANDLE   | \
                                  PD_ENABLESETUPTEMPLATEHANDLE)



VOID
ThunkPRINTDLG16to32(OUT PRINTDLG        *pPD32,
                    IN  PPRINTDLGDATA16  pPD16)
{
    DWORD  Flags;
    HANDLE h32New;
    LPVOID lp32New;
    LPVOID lp32Cur;

    if(pPD16 && pPD32) {

        pPD32->lStructSize = sizeof(PRINTDLG);
        pPD32->hwndOwner   = HWND32(pPD16->hwndOwner);

        // get a new 32-bit devmode thunked from the 16-bit one...
        if(h32New = ThunkhDevMode16to32(WORD32(pPD16->hDevMode))) {
            lp32New = GlobalLock(h32New);
            lp32Cur = GlobalLock(pPD32->hDevMode);

            // ...and copy it over the current 32-bit devmode struct
            if(lp32New && lp32Cur) {
                RtlCopyMemory(lp32Cur,
                              lp32New,
                              ((LPDEVMODE)lp32New)->dmSize);
                GlobalUnlock(pPD32->hDevMode);
                GlobalUnlock(h32New);
            }
            WOWGLOBALFREE(h32New);
        }

        // we assume that the DEVNAMES struct will never change

        // hDC filled on output only

        // preserve the template flag state while copying flags
        // 1. save original template flags
        //     note: we never set the 32-bit PD_ENABLExxxxTEMPLATE flags
        // 2. copy flags from 16-bit struct (and add WOWAPP flag)
        // 3. turn off all template flags
        // 4. restore original template flag state
        Flags         = pPD32->Flags & PD_TEMPLATEHANDLEMASK32;
        pPD32->Flags  = DWORD32(pPD16->Flags) | CD_WOWAPP;
        pPD32->Flags &= ~(PD_TEMPLATEMASK32 | PD_TEMPLATEHANDLEMASK32);
        pPD32->Flags |= Flags;

        pPD32->nFromPage   = WORD32(pPD16->nFromPage);
        pPD32->nToPage     = WORD32(pPD16->nToPage);
        pPD32->nMinPage    = WORD32(pPD16->nMinPage);
        pPD32->nMaxPage    = WORD32(pPD16->nMaxPage);
        pPD32->nCopies     = WORD32(pPD16->nCopies);
        pPD32->lCustData   = DWORD32(pPD16->lCustData);

        // hInstance thunked separately

        // hPrintTemplate & hSetupTemplate thunked separately
    }

}



#define PD_TEMPLATEMASK16  (PD_ENABLEPRINTTEMPLATE         | \
                            PD_ENABLESETUPTEMPLATE         | \
                            PD_ENABLEPRINTTEMPLATEHANDLE   | \
                            PD_ENABLESETUPTEMPLATEHANDLE)

VOID
ThunkPRINTDLG32to16(IN  VPVOID    vppd,
                    OUT PRINTDLG *pPD32)
{
    HAND16           hDevMode16;
    HAND16           hDevNames16;
    PPRINTDLGDATA16  pPD16;
    DWORD            Flags, Flags16;


    GETVDMPTR(vppd, sizeof(PRINTDLGDATA16), pPD16);

    if(pPD16 && pPD32) {

        if(pPD32->Flags & (PD_RETURNIC | PD_RETURNDC)) {
            STOREWORD(pPD16->hDC, GETHDC16(pPD32->hDC));
        }

        // thunk 32-bit DEVMODE structure back to 16-bit
        // hDevXXXX16 take care of RISC alignment problems
        hDevMode16  = WORD32(pPD16->hDevMode);
        hDevNames16 = WORD32(pPD16->hDevNames);

        // this call invalidates flat ptrs to 16-bit mem
        ThunkhDevMode32to16(&hDevMode16, pPD32->hDevMode);
        FREEVDMPTR(pPD16);

        GETVDMPTR(vppd, sizeof(PRINTDLGDATA16), pPD16);

        // this call invalidates flat ptrs to 16-bit mem
        ThunkhDevNames32to16(&hDevNames16, pPD32->hDevNames);
        FREEVDMPTR(pPD16);

        GETVDMPTR(vppd, sizeof(PRINTDLGDATA16), pPD16);

        STOREWORD(pPD16->hDevMode, hDevMode16);
        STOREWORD(pPD16->hDevNames, hDevNames16);

        // preserve the template flag state while copying flags
        // 1. save original template flags
        // 2. copy flags from 32-bit struct
        // 3. turn off all template flags and WOWAPP flag
        // 4. restore original template flag state
        Flags    = DWORD32(pPD16->Flags) & PD_TEMPLATEMASK16;
        Flags16  = pPD32->Flags;
        Flags16 &= ~(PD_TEMPLATEMASK16 | CD_WOWAPP);
        Flags16 |= Flags;
        STOREDWORD(pPD16->Flags, Flags16);

        STOREWORD(pPD16->nFromPage, GETUINT16(pPD32->nFromPage));
        STOREWORD(pPD16->nToPage,   GETUINT16(pPD32->nToPage));
        STOREWORD(pPD16->nMinPage,  GETUINT16(pPD32->nMinPage));
        STOREWORD(pPD16->nMaxPage,  GETUINT16(pPD32->nMaxPage));
        STOREWORD(pPD16->nCopies,   GETUINT16(pPD32->nCopies));
        FREEVDMPTR(pPD16);
    }
}





HGLOBAL
ThunkhDevMode16to32(IN HAND16 hDevMode16)
{
    INT         nSize;
    LPDEVMODE   lpdm32, pdm32;
    HGLOBAL     hDevMode32 = NULL;
    VPDEVMODE31 vpDevMode16;


    if (hDevMode16) {

        vpDevMode16 = GlobalLock16(hDevMode16, NULL);

        if(FETCHDWORD(vpDevMode16)) {

            if(pdm32 = ThunkDevMode16to32(vpDevMode16)) {

                nSize = FETCHWORD(pdm32->dmSize) +
                        FETCHWORD(pdm32->dmDriverExtra);

                hDevMode32 = WOWGLOBALALLOC(GMEM_MOVEABLE, nSize);

                if(lpdm32 = GlobalLock(hDevMode32)) {
                    RtlCopyMemory((PVOID)lpdm32, (PVOID)pdm32, nSize);
                    GlobalUnlock(hDevMode32);
                }

                free_w(pdm32);
            }
            GlobalUnlock16(hDevMode16);
        }
    }

    return(hDevMode32);
}





VOID
ThunkhDevMode32to16(IN OUT HAND16 *phDevMode16,
                    IN     HANDLE  hDevMode32)
/*++

Routine Description:

    This routine thunks a 32-bit DevMode structure back into the 16-bit one.
    It will reallocate the 16-bit global memory block as necessary.

    WARNING: This may cause 16-bit memory to move, invalidating flat pointers.

Arguments:

    hDevMode    - Supplies a handle to a movable global memory object that
                  contains a 32-bit DEVMODE structure

    phDevMode16 - Supplies a pointer to a 16-bit handle to a movable global
                  memory object that will return the 16-bit DEVMODE structure.
                  If the handle is NULL, the object will be allocated.  It
                  may also be reallocated if its current size is not enough.

Return Value:

    None

--*/
{
    UINT        CurrentSize;
    UINT        RequiredSize;
    VPDEVMODE31 vpDevMode16;
    LPDEVMODE   lpDevMode32;

    if (hDevMode32 == NULL) {
        *phDevMode16 = (HAND16)NULL;
        return;
    }

    lpDevMode32 = GlobalLock(hDevMode32);
    if (lpDevMode32==NULL) {
        *phDevMode16 = (HAND16)NULL;
        return;
    }

    RequiredSize = lpDevMode32->dmSize        +
                   lpDevMode32->dmDriverExtra +
                   sizeof(WOWDM31);  // see notes in wstruc.c

    if (*phDevMode16 == (HAND16)NULL) {
        vpDevMode16 = GlobalAllocLock16(GMEM_MOVEABLE,
                                        RequiredSize,
                                        phDevMode16);
    } else {
        vpDevMode16 = GlobalLock16(*phDevMode16, &CurrentSize);

        if (CurrentSize < RequiredSize) {
            GlobalUnlockFree16(vpDevMode16);
            vpDevMode16 = GlobalAllocLock16(GMEM_MOVEABLE,
                                            RequiredSize,
                                            phDevMode16);
        }
    }

    if(ThunkDevMode32to16(vpDevMode16, lpDevMode32, RequiredSize)) {
        GlobalUnlock16(*phDevMode16);
    }
    else {
        *phDevMode16 = (HAND16)NULL;
    }

    GlobalUnlock(hDevMode32);
}




HANDLE
ThunkhDevNames16to32(IN HAND16 hDevNames16)
{
    INT         nSize;
    HANDLE      hDN32 = NULL;
    LPDEVNAMES  pdn32;
    PDEVNAMES16 pdn16;


    if(FETCHDWORD(hDevNames16)) {

        VPDEVNAMES vpDevNames;

        vpDevNames = GlobalLock16(hDevNames16, &nSize);

        if(nSize) {

            GETVDMPTR(vpDevNames, sizeof(DEVNAMES16), pdn16);

            if(pdn16) {

                hDN32 = WOWGLOBALALLOC(GMEM_MOVEABLE, nSize);

                if(pdn32 = GlobalLock(hDN32)) {
                    RtlCopyMemory((PVOID)pdn32, (PVOID)pdn16, nSize);
                    GlobalUnlock(hDN32);
                } else {
                    LOGDEBUG(0, ("ThunkhDEVNAMES16to32, 32-bit allocation(s) failed!\n"));
                }

                FREEVDMPTR(pdn16);
            }
            GlobalUnlock16(hDevNames16);
        }

    }

    return(hDN32);
}




VOID
ThunkhDevNames32to16(IN OUT HAND16 *phDevNames16,
                     IN     HANDLE  hDevNames)
/*++

Routine Description:

    This routine thunks a 32-bit DevNames structure back into the 16-bit one.
    It will reallocate the 16-bit global memory block as necessary.

    WARNING: This may cause 16-bit memory to move, invalidating flat pointers.

Arguments:

    hDevNames - Supplies a handle to a movable global memory object that
               contains a 32-bit DEVNAMES structure

    phDevNames16 - Supplies a pointer to a 16-bit handle to a movable global
               memory object that will return the 16-bit DEVNAMES structure.
               If the handle is NULL, the object will be allocated.  It
               may also be reallocated if its current size is not enough.

Return Value:

    None

--*/
{
    UINT CurrentSize;
    UINT RequiredSize;
    UINT CopySize;
    UINT MaxOffset;
    PDEVNAMES16 pdn16;
    VPDEVNAMES DevNames16;
    LPDEVNAMES DevNames32;


    if (hDevNames==NULL) {
        *phDevNames16=(HAND16)NULL;
        return;
    }

    DevNames32 = GlobalLock(hDevNames);
    if (DevNames32==NULL) {
        *phDevNames16=(HAND16)NULL;
    }
    MaxOffset = max(max(DevNames32->wDriverOffset,DevNames32->wDeviceOffset),
                    DevNames32->wOutputOffset);

    // ProComm Plus copies 0x48 constant bytes after Print Setup.
    CopySize = MaxOffset + strlen((PCHAR)DevNames32+MaxOffset) + 1;
    RequiredSize = max(CopySize, 0x48);

    if (*phDevNames16==(HAND16)NULL) {
        DevNames16 = GlobalAllocLock16(GMEM_MOVEABLE,
                                       RequiredSize,
                                       phDevNames16);
    } else {
        DevNames16 = GlobalLock16(*phDevNames16, &CurrentSize);
        if (CurrentSize < RequiredSize) {
            GlobalUnlockFree16(DevNames16);
            DevNames16 = GlobalAllocLock16(GMEM_MOVEABLE,
                                           RequiredSize,
                                           phDevNames16);
        }
    }

    GETVDMPTR(DevNames16, RequiredSize, pdn16);
    if (pdn16==NULL) {
        *phDevNames16=(HAND16)NULL;
        GlobalUnlock(hDevNames);
        return;
    }
    RtlCopyMemory(pdn16,DevNames32,CopySize);
    FREEVDMPTR(pdn16);
    GlobalUnlock16(*phDevNames16);
    GlobalUnlock(hDevNames);
}






ULONG FASTCALL
WCD32GetOpenFileName( PVDMFRAME pFrame )
/*++

Routine Description:

    This routine thunks the 16-bit GetOpenFileName common dialog to the
    32-bit side.

Arguments:

    pFrame - Supplies 16-bit argument frame

Return Value:

    16-bit BOOLEAN to be returned.

--*/
{
    return(WCD32GetFileName(pFrame,GetOpenFileName));
}




ULONG FASTCALL
WCD32GetSaveFileName( PVDMFRAME pFrame )
/*++

Routine Description:

    This routine thunks the 16-bit GetOpenFileName common dialog to the
    32-bit side.

Arguments:

    pFrame - Supplies 16-bit argument frame

Return Value:

    16-bit BOOLEAN to be returned.

--*/
{
    return(WCD32GetFileName(pFrame,GetSaveFileName));
}





ULONG
WCD32GetFileName(IN PVDMFRAME pFrame,
                 IN FILENAMEPROC Function)
/*++

Routine Description:

    This routine is called by WCD32GetOpenFileName and WCD32GetSaveFileName.
    It does all the real thunking work.

Arguments:

    pFrame - Supplies 16-bit argument frame

    Function - supplies a pointer to the 32-bit function to call (either
               GetOpenFileName or GetSaveFileName)

Return Value:

    16-bit BOOLEAN to be returned.

--*/
{
    ULONG                       ul = 0;
    register PGETOPENFILENAME16 parg16;
    VPOPENFILENAME              vpof;
    OPENFILENAME                OFN32;
    POPENFILENAME16             pOFN16;
    COMMDLGTD                   ThreadData;
    PRES                        pRes = NULL;
    DWORD                       dwFlags16 = 0;
    USHORT                      cb;
    PBYTE                       lpcb;
    BOOL                        fError = FALSE;


    GETARGPTR(pFrame, sizeof(GETOPENFILENAME16), parg16);
    vpof = parg16->lpof;

    SETEXTENDEDERROR(0);

    // invalidate this now
    FREEARGPTR(parg16);

    // initialize unique window messages
    if (msgFILEOK == 0) {

        if(!(msgSHAREVIOLATION = (WORD)RegisterWindowMessage(SHAREVISTRING))) {
            SETEXTENDEDERROR( CDERR_REGISTERMSGFAIL );
            LOGDEBUG(2,("WCD32GetFileName:RegisterWindowMessage failed\n"));
            return(0);
        }
        if(!(msgFILEOK = (WORD)RegisterWindowMessage(FILEOKSTRING))) {
            SETEXTENDEDERROR( CDERR_REGISTERMSGFAIL );
            LOGDEBUG(2,("WCD32GetFileName:RegisterWindowMessage 2 failed\n"));
            return(0);
        }

        // initialize private WOW-comdlg32 message
        msgWOWDIRCHANGE = (WORD)RegisterWindowMessage("WOWDirChange");
    }

    GETVDMPTR(vpof, sizeof(OPENFILENAME16), pOFN16);

    WCDDUMPOPENFILENAME16(pOFN16);

    if(DWORD32(pOFN16->lStructSize) != sizeof(OPENFILENAME16)) {
        SETEXTENDEDERROR( CDERR_STRUCTSIZE );
        FREEVDMPTR(pOFN16);
        return(0);
    }

    RtlZeroMemory(&ThreadData, sizeof(COMMDLGTD));
    ThreadData.Previous = CURRENTPTD()->CommDlgTd;
    ThreadData.hdlg     = (HWND16)-1;
    ThreadData.pData32  = (PVOID)&OFN32;
    ThreadData.Flags    = WOWCD_ISOPENFILE;
    if(DWORD32(pOFN16->Flags) & OFN_ENABLEHOOK) {
        ThreadData.vpfnHook = DWORD32(pOFN16->lpfnHook);
        if(!ThreadData.vpfnHook) {
            SETEXTENDEDERROR(CDERR_NOHOOK);
            FREEVDMPTR(pOFN16);
            return(0);
        }
        ThreadData.vpData   = vpof;
    }
    RtlZeroMemory(&OFN32, sizeof(OPENFILENAME));

    if(!Alloc_OFN32_strs(&OFN32, pOFN16)) {
        SETEXTENDEDERROR(CDERR_MEMALLOCFAILURE);
        goto GetFileNameExit;
    }

    // On Win3.1, the system sets these flags in the app's struct under the
    // shown conditions so we need to update the 16-bit struct too.
    dwFlags16 = DWORD32(pOFN16->Flags);
    if(dwFlags16 & OFN_CREATEPROMPT) {
        dwFlags16 |= OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    }
    else if(dwFlags16 & OFN_FILEMUSTEXIST) {
        dwFlags16 |= OFN_PATHMUSTEXIST;
    }

    // A bug in Serif PagePlus 3.0 sets the high word to 0xFFFF which causes
    // the new moniker stuff in comdlg32 to break. #148137 - cmjones
    // VadimB: the mask below causes apps that do want lfn to break, so check
    // for those apps via the compat flag and let them go unpunished

    if ((dwFlags16 & OFN_LONGNAMES) &&
        (CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_ALLOWLFNDIALOGS)) {
          dwFlags16 = (dwFlags16 & VALID_OFN16_FLAGS) | OFN_LONGNAMES;
    }
    else {
          dwFlags16 &= VALID_OFN16_FLAGS;
    }

    STOREDWORD(pOFN16->Flags, dwFlags16);

    if(!ThunkOPENFILENAME16to32(&OFN32, pOFN16)) {
        SETEXTENDEDERROR(CDERR_MEMALLOCFAILURE);
        goto GetFileNameExit;
    }

    dwFlags16 = DWORD32(pOFN16->Flags);  // get updated flags

    // make sure the current directory is up to date
    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    // this call invalidates flat ptrs to 16-bit memory
    OFN32.hInstance = ThunkCDTemplate16to32(WORD32(pOFN16->hInstance),
                                            0,
                                            DWORD32(pOFN16->lpTemplateName),
                                            dwFlags16,
                                            &(OFN32.Flags),
                                            OFN_ENABLETEMPLATE,
                                            OFN_ENABLETEMPLATEHANDLE,
                                            &pRes,
                                            &fError);

    if(fError) {
        goto GetFileNameExit;
    }

    // memory may move - free flat pointers now
    FREEVDMPTR(pOFN16);

    WCDDUMPOPENFILENAME32(&OFN32);

    // Set this just before the calling into comdlg32.  This prevents the
    // synchronization stuff from firing until we actually need it.
    CURRENTPTD()->CommDlgTd = &ThreadData;

    // this call invalidates flat ptrs to 16-bit memory
    ul = GETBOOL16((*Function)(&OFN32));

    CURRENTPTD()->CommDlgTd = ThreadData.Previous;

    WCDDUMPOPENFILENAME32(&OFN32);

    UpdateDosCurrentDirectory(DIR_NT_TO_DOS);

    GETVDMPTR(vpof, sizeof(OPENFILENAME16), pOFN16);

    if (ul) {
        ThunkOPENFILENAME32to16(pOFN16, &OFN32, TRUE);

    }

    // else if the buffer is too small, lpstrFile contains the required buffer
    // size for the specified file
    else if (CommDlgExtendedError() == FNERR_BUFFERTOOSMALL) {

        SETEXTENDEDERROR(FNERR_BUFFERTOOSMALL);

        if(OFN32.lpstrFile && pOFN16->lpstrFile) {

            cb = *((PUSHORT)(OFN32.lpstrFile));  // is a WORD for comdlg32 too

            // 3 is the documented minimum size of the lpstrFile buffer
            GETVDMPTR(pOFN16->lpstrFile, 3, lpcb);

            // Win3.1 assumes that lpstrFile buffer is at least 3 bytes long
            // we'll try to be a little smarter than that...
            if(lpcb && (cb > pOFN16->nMaxFile)) {

                if(pOFN16->nMaxFile)
                    lpcb[0] = LOBYTE(cb);
                if(pOFN16->nMaxFile > 1)
                    lpcb[1] = HIBYTE(cb);
                if(pOFN16->nMaxFile > 2)
                    lpcb[2] = 0;  // Win3.1 appends a NULL

                FREEVDMPTR(lpcb);
            }
        }
    }

    WCDDUMPOPENFILENAME16(pOFN16);

GetFileNameExit:

    FreeCDTemplate32(pRes,
                     OFN32.hInstance,
                     dwFlags16 & OFN_ENABLETEMPLATE,
                     dwFlags16 & OFN_ENABLETEMPLATEHANDLE);

    Free_OFN32_strs(&OFN32);

    FREEVDMPTR(pOFN16);

    return(ul);
}




BOOL
ThunkOPENFILENAME16to32(OUT OPENFILENAME    *pOFN32,
                        IN  POPENFILENAME16  pOFN16)
/*++

Routine Description:

    This routine thunks a 16-bit OPENFILENAME structure to the 32-bit
    OPENFILENAME structure

Arguments:

    pOFN16 - Supplies a flat pointer to the 16-bit OPENFILENAME structure.

    pOFN32 - Supplies a pointer to the 32-bit OPENFILENAME structure.

Return Value:

    None.

--*/
{
    DWORD Flags;

    if(pOFN16 && pOFN32) {

        // Re-thunk all of the strings!!!
        // Persuasion 3.0 changes the various ptrs to strings depending on which
        // dialog buttons are pushed so we might have to dynamically re-alloc
        // some of the 32-bit string buffers.
        Thunk_OFNstrs16to32(pOFN32, pOFN16);

        pOFN32->lStructSize    = sizeof(OPENFILENAME);
        pOFN32->hwndOwner      = HWND32(pOFN16->hwndOwner);
        // hInstance thunked separately
        pOFN32->nMaxCustFilter = DWORD32(pOFN16->nMaxCustFilter);
        pOFN32->nFilterIndex   = DWORD32(pOFN16->nFilterIndex);
        pOFN32->nMaxFile       = DWORD32(pOFN16->nMaxFile);
        pOFN32->nMaxFileTitle  = DWORD32(pOFN16->nMaxFileTitle);

        // preserve the template flag state while copying flags
        // 1. save template flag state
        //     note: we never will have a 32-bit OFN_ENABLETEMPLATE flag
        //           we may or may not have a OFN_ENABLETEMPLATEHANDLE flag
        // 2. copy flags from 16-bit struct
        // 3. turn off all template flags
        // 4. restore original template flag state
        // 5. add the WOWAPP and no-long-names flags
        Flags          = pOFN32->Flags & OFN_ENABLETEMPLATEHANDLE;
        pOFN32->Flags  = DWORD32(pOFN16->Flags);
        pOFN32->Flags &= ~(OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE);
        pOFN32->Flags |= Flags;

        if ((pOFN32->Flags & OFN_LONGNAMES) &&
            (CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_ALLOWLFNDIALOGS)) {
            pOFN32->Flags |= CD_WOWAPP;
        }
        else {
            pOFN32->Flags |= (OFN_NOLONGNAMES | CD_WOWAPP);
        }

        pOFN32->nFileOffset    = WORD32(pOFN16->nFileOffset);
        pOFN32->nFileExtension = WORD32(pOFN16->nFileExtension);
        pOFN32->lCustData      = DWORD32(pOFN16->lCustData);
        if(DWORD32(pOFN16->Flags) & OFN_ENABLEHOOK) {
            pOFN32->lpfnHook   = WCD32DialogProc;
        }
        // lpTemplateName32 is thunked separately

        // This is a hack to fix a bug in Win3.1 commdlg.dll.
        // Win3.1 doesn't check nMaxFileTitle before copying the FileTitle str.
        // (see Win3.1 src's \\pucus\win31aro\src\sdk\commdlg\fileopen.c)
        // TaxCut'95 depends on the title string being returned.
        if(pOFN32->lpstrFileTitle) {

            // if nMaxFileTitle > 0, NT will copy lpstrFileTitle
            if(pOFN32->nMaxFileTitle == 0) {
                pOFN32->nMaxFileTitle = 13;  // 8.3 filename + NULL
            }
        }

        return(TRUE);
    }

    return(FALSE);
}





VOID
ThunkOPENFILENAME32to16(OUT POPENFILENAME16  pOFN16,
                        IN  OPENFILENAME    *pOFN32,
                        IN  BOOLEAN          bUpperStrings)
/*++

Routine Description:

    This routine thunks a 32-bit OPENFILENAME structure back to a 16-bit
    OPENFILENAME structure.

Arguments:

    pOFN32 - Supplies a pointer to the 32-bit OPENFILENAME struct.

    pOFN16 - Supplies a flat pointer to the 16-bit OPENFILENAME struct

Return Value:

    None.

--*/
{
    LPSTR lpstr;
    DWORD Flags, Flags32;


    if(pOFN16 && pOFN32) {

        STOREWORD(pOFN16->nFileOffset,    pOFN32->nFileOffset);
        STOREWORD(pOFN16->nFileExtension, pOFN32->nFileExtension);
        STOREDWORD(pOFN16->nFilterIndex,  pOFN32->nFilterIndex);

        // preserve the template flag state while copying flags
        // 1. save template flag state
        // 2. copy flags from 32-bit struct
        // 3. turn off all template flags and the WOWAPP flag
        // 4. restore original template flag state
        Flags    = DWORD32(pOFN16->Flags) & (OFN_ENABLETEMPLATE |
                                             OFN_ENABLETEMPLATEHANDLE);
        Flags32  = pOFN32->Flags;
        Flags32 &= ~(OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE | CD_WOWAPP);
        Flags32 |= Flags;
        STOREDWORD(pOFN16->Flags, Flags32);

        if(bUpperStrings && pOFN32->lpstrFile) {

            // Note we have to upcase the pOFN32 here because some apps
            // (notably QC/Win) do case-sensitive compares on the extension.
            // In Win3.1, the upcasing happens as a side-effect of the
            // OpenFile call.  Here we do it explicitly.
            CharUpperBuff(pOFN32->lpstrFile, strlen(pOFN32->lpstrFile));
        }
        GETPSZPTR(pOFN16->lpstrFile, lpstr);
        if(lpstr && pOFN32->lpstrFile) {
            strcpy(lpstr,  pOFN32->lpstrFile);
            FREEPSZPTR(lpstr);
        }

        GETPSZPTR(pOFN16->lpstrFilter, lpstr);
        if(lpstr && pOFN32->lpstrFilter) {
            Multi_strcpy(lpstr, pOFN32->lpstrFilter);
            FREEPSZPTR(lpstr);
        }

        GETPSZPTR(pOFN16->lpstrCustomFilter, lpstr);
        if(lpstr && pOFN32->lpstrCustomFilter) {
            Multi_strcpy(lpstr, pOFN32->lpstrCustomFilter);
            FREEPSZPTR(lpstr);
        }

        if(bUpperStrings && (pOFN32->lpstrFileTitle)) {

            // Not sure if we really need to upcase this or not, but I figure
            // somewhere there is an app that depends on this being uppercased
            // like Win3.1
            CharUpperBuff(pOFN32->lpstrFileTitle,
                          strlen(pOFN32->lpstrFileTitle));
        }
        GETPSZPTR(pOFN16->lpstrFileTitle , lpstr);
        if(lpstr && pOFN32->lpstrFileTitle) {
            strcpy(lpstr, pOFN32->lpstrFileTitle);
            FREEPSZPTR(lpstr);
        }

        // even though this is doc'd as being filled by the app only, Adobe
        // distiller depends on it being copied back to the app
        GETPSZPTR(pOFN16->lpstrInitialDir , lpstr);
        if(lpstr && pOFN32->lpstrInitialDir) {
            strcpy(lpstr, pOFN32->lpstrInitialDir);
            FREEPSZPTR(lpstr);
        }

        // who knows who depends on this
        GETPSZPTR(pOFN16->lpstrTitle, lpstr);
        if(lpstr && pOFN32->lpstrTitle) {
            strcpy(lpstr, pOFN32->lpstrTitle);
            FREEPSZPTR(lpstr);
        }
    }
}




BOOL
Alloc_OFN32_strs(IN OPENFILENAME    *pOFN32,
                 IN POPENFILENAME16  pOFN16)
{

    if(DWORD32(pOFN16->lpstrFilter)) {
        if(!(pOFN32->lpstrFilter =
                malloc_w_strcpy_vp16to32(DWORD32(pOFN16->lpstrFilter),
                                         TRUE,
                                         0))) {
            goto ErrorExit;
        }
    }

    if(DWORD32(pOFN16->lpstrCustomFilter)) {
        if(!(pOFN32->lpstrCustomFilter =
                malloc_w_strcpy_vp16to32(DWORD32(pOFN16->lpstrCustomFilter),
                                         TRUE,
                                         DWORD32(pOFN16->nMaxCustFilter) ))) {
            goto ErrorExit;
        }
    }

    if(DWORD32(pOFN16->lpstrFile)) {
        if(!(pOFN32->lpstrFile =
                malloc_w_strcpy_vp16to32(DWORD32(pOFN16->lpstrFile),
                                         FALSE,
                                         DWORD32(pOFN16->nMaxFile) ))) {
            goto ErrorExit;
        }
    }

    if(DWORD32(pOFN16->lpstrFileTitle)) {
        if(!(pOFN32->lpstrFileTitle =
                malloc_w_strcpy_vp16to32(DWORD32(pOFN16->lpstrFileTitle),
                                         FALSE,
                                         DWORD32(pOFN16->nMaxFileTitle) ))) {
            goto ErrorExit;
        }
    }

    if(DWORD32(pOFN16->lpstrInitialDir)) {
        if(!(pOFN32->lpstrInitialDir =
                malloc_w_strcpy_vp16to32(DWORD32(pOFN16->lpstrInitialDir),
                                         FALSE,
                                         0))) {
            goto ErrorExit;
        }
    }

    if(DWORD32(pOFN16->lpstrTitle)) {
        if(!(pOFN32->lpstrTitle =
                malloc_w_strcpy_vp16to32(DWORD32(pOFN16->lpstrTitle),
                                         FALSE,
                                         0))) {
            goto ErrorExit;
        }
    }

    if(DWORD32(pOFN16->lpstrDefExt)) {
        if(!(pOFN32->lpstrDefExt =
                malloc_w_strcpy_vp16to32(DWORD32(pOFN16->lpstrDefExt),
                                         FALSE,
                                         0))) {
            goto ErrorExit;
        }
    }

    return(TRUE);

ErrorExit:
    LOGDEBUG(0, ("Alloc_OFN32_strs, 32-bit allocation(s) failed!\n"));
    Free_OFN32_strs(pOFN32);
    return(FALSE);

}





VOID
Free_OFN32_strs(IN OPENFILENAME *pOFN32)
{
    if(pOFN32->lpstrFilter) {
        free_w((PVOID)pOFN32->lpstrFilter);
        pOFN32->lpstrFilter = NULL;
    }

    if(pOFN32->lpstrCustomFilter) {
        free_w((PVOID)pOFN32->lpstrCustomFilter);
        pOFN32->lpstrCustomFilter = NULL;
    }

    if(pOFN32->lpstrFile) {
        free_w((PVOID)pOFN32->lpstrFile);
        pOFN32->lpstrFile = NULL;
    }

    if(pOFN32->lpstrFileTitle) {
        free_w((PVOID)pOFN32->lpstrFileTitle);
        pOFN32->lpstrFileTitle = NULL;
    }

    if(pOFN32->lpstrInitialDir) {
        free_w((PVOID)pOFN32->lpstrInitialDir);
        pOFN32->lpstrInitialDir = NULL;
    }

    if(pOFN32->lpstrTitle) {
        free_w((PVOID)pOFN32->lpstrTitle);
        pOFN32->lpstrTitle = NULL;
    }

    if(pOFN32->lpstrDefExt) {
        free_w((PVOID)pOFN32->lpstrDefExt);
        pOFN32->lpstrDefExt = NULL;
    }
}




VOID
Thunk_OFNstrs16to32(IN OPENFILENAME    *pOFN32,
                    IN POPENFILENAME16  pOFN16)
{
   pOFN32->lpstrFilter
                  = ThunkStr16toStr32((LPSTR)pOFN32->lpstrFilter,
                                      DWORD32(pOFN16->lpstrFilter),
                                      MAX_PATH,
                                      TRUE);

   pOFN32->lpstrCustomFilter
                  = ThunkStr16toStr32(pOFN32->lpstrCustomFilter,
                                      DWORD32(pOFN16->lpstrCustomFilter),
                                      DWORD32(pOFN16->nMaxCustFilter),
                                      TRUE);

   pOFN32->lpstrFile
                  = ThunkStr16toStr32(pOFN32->lpstrFile,
                                      DWORD32(pOFN16->lpstrFile),
                                      DWORD32(pOFN16->nMaxFile),
                                      FALSE);

   pOFN32->lpstrFileTitle
                  = ThunkStr16toStr32(pOFN32->lpstrFileTitle,
                                      DWORD32(pOFN16->lpstrFileTitle),
                                      DWORD32(pOFN16->nMaxFileTitle),
                                      FALSE);

   pOFN32->lpstrInitialDir
                  = ThunkStr16toStr32((LPSTR)pOFN32->lpstrInitialDir,
                                      DWORD32(pOFN16->lpstrInitialDir),
                                      MAX_PATH,
                                      FALSE);

   pOFN32->lpstrTitle
                  = ThunkStr16toStr32((LPSTR)pOFN32->lpstrTitle,
                                      DWORD32(pOFN16->lpstrTitle),
                                      MAX_PATH,
                                      FALSE);

   pOFN32->lpstrDefExt
                  = ThunkStr16toStr32((LPSTR)pOFN32->lpstrDefExt,
                                      DWORD32(pOFN16->lpstrDefExt),
                                      10,
                                      FALSE);
}




ULONG FASTCALL
WCD32FindText(PVDMFRAME pFrame)
/*++

Routine Description:

    This routine thunks the 16-bit FindText common dialog to the
    32-bit side.

Arguments:

    pFrame - Supplies 16-bit argument frame

Return Value:

    16-bit BOOLEAN to be returned.

--*/
{
    return(WCD32FindReplaceText(pFrame, FindText));
}





ULONG FASTCALL
WCD32ReplaceText(PVDMFRAME pFrame)
/*++

Routine Description:

    This routine thunks the 16-bit ReplaceText common dialog to the
    32-bit side.

Arguments:

    pFrame - Supplies 16-bit argument frame

Return Value:

    16-bit BOOLEAN to be returned.

--*/
{
    return(WCD32FindReplaceText(pFrame, ReplaceText));
}




ULONG
WCD32FindReplaceText(IN PVDMFRAME       pFrame,
                     IN FINDREPLACEPROC Function)
/*++

Routine Description:

    This routine is called by WCD32FindText and WCD32RepalceText.
    It copies a 16-bit FINDREPLACE structure to a 32-bit structure.
    Two per thread data entries are maintained. One is indexed by the
    owner hwnd, the other is indexed by the dialog hwnd. The dialog is
    always hooked by WCD32FindReplaceDialogProc, which dispatches to the
    16-bit hookproc, and takes care of clean-up on  WM_DESTROY, with dialog
    per thread data providing context. WCD32UpdateFindReplaceTextAndFlags
    updates the 16-bit FINDREPLACE structure when called by the WOW message
    dispatching logic upon reciept of a WM_NOTIFYWOW message from COMDLG32.
    The owner per thread data provides context for this operation.

Arguments:

    pFrame - Supplies 16-bit argument frame

    Function - supplies a pointer to the 32-bit function to call (either
               FindText or RepalceText)

Return Value:

    16-bit BOOLEAN to be returned.

--*/
{
    register PFINDTEXT16  parg16;
    VPFINDREPLACE         vpfr;
    FINDREPLACE          *pFR32;
    PFINDREPLACE16        pFR16;
    PCOMMDLGTD            pTDDlg;
    PCOMMDLGTD            pTDOwner;
    HWND                  hwndDlg = NULL;
    DWORD                 dwFlags16 = 0;
    BOOL                  fError = FALSE;


    GETARGPTR(pFrame, sizeof(FINDREPLACE16), parg16);
    vpfr = parg16->lpfr;

    SETEXTENDEDERROR(0);

    // invalidate this now
    FREEVDMPTR( parg16 );

    GETVDMPTR(vpfr, sizeof(FINDREPLACE16), pFR16);

    WCDDUMPFINDREPLACE16(pFR16);

    if(DWORD32(pFR16->lStructSize) != sizeof(FINDREPLACE16)) {
        SETEXTENDEDERROR( CDERR_STRUCTSIZE );
        FREEVDMPTR(pFR16);
        return(0);
    }

    if(!DWORD32(pFR16->lpstrFindWhat) ||
       !WORD32(pFR16->wFindWhatLen) ||
       !IsWindow(HWND32(pFR16->hwndOwner))) {
        SETEXTENDEDERROR(FRERR_BUFFERLENGTHZERO);
        FREEVDMPTR(pFR16);
        return(0);
    }

    // check the hook proc
    if(DWORD32(pFR16->Flags) & FR_ENABLEHOOK) {
        if(!DWORD32(pFR16->lpfnHook)) {
            SETEXTENDEDERROR(CDERR_NOHOOK);
            FREEVDMPTR(pFR16);
            return(0);
        }
    }
    else {
        STOREDWORD(pFR16->lpfnHook, 0);
    }

    // WCD32UpdateFindReplaceTextAndFlags will update the 16-bit FINDREPLACE
    // struct and help thunk the WM_NOTIFYWOW message to the
    // "commdlg_FindReplace" registered message.
    if (msgFINDREPLACE == 0) {
        if(!(msgFINDREPLACE = (WORD)RegisterWindowMessage(FINDMSGSTRING))) {
            LOGDEBUG(2,("WCD32FindReplaceText:RegisterWindowMessage failed\n"));
            SETEXTENDEDERROR( CDERR_REGISTERMSGFAIL );
            FREEVDMPTR(pFR16);
            return(0);
        }
    }

    // Allocate the required memory
    // Note: these can't be alloc'd off our stack since FindText & ReplaceText
    //       eventually call CreateDialogIndirectParam which returns immediately
    //       after displaying the dialog box.
    pFR32 = (FINDREPLACE *)malloc_w_zero(sizeof(FINDREPLACE));
    if(pFR32) {
        pFR32->lpstrFindWhat = (LPTSTR)malloc_w(WORD32(pFR16->wFindWhatLen));
        pFR32->lpstrReplaceWith
                             = (LPTSTR)malloc_w(WORD32(pFR16->wReplaceWithLen));
        pTDDlg   = malloc_w_zero(sizeof(COMMDLGTD));
        pTDOwner = malloc_w_zero(sizeof(COMMDLGTD));
    }

    if(  (pFR32                    &&
          pFR32->lpstrFindWhat     &&
          pFR32->lpstrReplaceWith  &&
          pTDDlg                   &&
          pTDOwner) == FALSE) {

        LOGDEBUG(0, ("WCD32FindReplaceText, 32-bit allocation(s) failed!\n"));
        SETEXTENDEDERROR(CDERR_MEMALLOCFAILURE);
        goto FindReplaceError;
    }

    pTDDlg->pData32 = pTDOwner->pData32 = (PVOID)pFR32;
    pTDDlg->vpData  = pTDOwner->vpData  = vpfr;

    // Set the per thread data indicies
    pTDDlg->hdlg   = (HWND16)-1;
    pTDOwner->hdlg = GETHWND16(pFR16->hwndOwner);

    // save the hook proc if any
    if(DWORD32(pFR16->Flags) & FR_ENABLEHOOK) {
        pTDDlg->vpfnHook = pTDOwner->vpfnHook = DWORD32(pFR16->lpfnHook);
    }

    ThunkFINDREPLACE16to32(pFR32, pFR16);
    dwFlags16 = DWORD32(pFR16->Flags);

    // this call invalidates flat ptrs to 16-bit memory
    pFR32->hInstance = ThunkCDTemplate16to32(WORD32(pFR16->hInstance),
                                             0,
                                             DWORD32(pFR16->lpTemplateName),
                                             dwFlags16,
                                             &(pFR32->Flags),
                                             FR_ENABLETEMPLATE,
                                             FR_ENABLETEMPLATEHANDLE,
                                             &(PRES)(pTDDlg->pRes),
                                             &fError);

    if(fError) {
        goto FindReplaceError;
    }

    // invalidate flat ptrs to 16-bit memory
    FREEVDMPTR(pFR16);

    WCDDUMPFINDREPLACE32(pFR32);

    // Link both per thread data structs into the list
    // do this just before calling into comdlg32
    pTDDlg->Previous        = CURRENTPTD()->CommDlgTd;
    pTDOwner->Previous      = pTDDlg;
    CURRENTPTD()->CommDlgTd = pTDOwner;

    // this call invalidates flat ptrs to 16-bit memory
    hwndDlg = (*Function)(pFR32);

    if (hwndDlg) {
        pTDDlg->hdlg = (HWND16)hwndDlg;
    } else {

FindReplaceError:
        LOGDEBUG(0, ("WCD32FindReplaceText, Failed!\n"));
        if(pTDDlg) {

            CURRENTPTD()->CommDlgTd = pTDDlg->Previous;

            FreeCDTemplate32(pTDDlg->pRes,
                             pFR32->hInstance,
                             dwFlags16 & FR_ENABLETEMPLATE,
                             dwFlags16 & FR_ENABLETEMPLATEHANDLE);
            free_w(pTDDlg);
        }

        if(pFR32) {

            if(pFR32->lpstrFindWhat)
                free_w(pFR32->lpstrFindWhat);

            if(pFR32->lpstrReplaceWith)
                free_w(pFR32->lpstrReplaceWith);

            free_w(pFR32);
        }

        if(pTDOwner)
            free_w(pTDOwner);
    }

    return(GETHWND16(hwndDlg));
}





VOID
ThunkFINDREPLACE16to32(OUT FINDREPLACE    *pFR32,
                       IN  PFINDREPLACE16  pFR16)
/*++

Routine Description:

    This routine thunks a 16-bit FINDREPLACE structure to the 32-bit
    structure

Arguments:

    pFR32 - Supplies a pointer to the 32-bit FINDREPLACE structure.

    pFR16 - Supplies a pointer to the 16-bit FINDREPLACE structure.

Return Value:

    None.

--*/
{
    LPSTR  lpstr;
    DWORD  Flags;


    if(pFR16 && pFR32) {

        pFR32->lStructSize = sizeof(FINDREPLACE);
        pFR32->hwndOwner   = HWND32(pFR16->hwndOwner);

        // hInstance is thunked separately

        // preserve the template flag state while copying flags
        // 1. save template flag state
        //     note: we never will have a 32-bit FR_ENABLETEMPLATE flag
        // 2. copy flags from 16-bit struct (add the WOWAPP flag)
        // 3. turn off all template flags
        // 4. restore original template flag state
        Flags         = pFR32->Flags & FR_ENABLETEMPLATEHANDLE;
        pFR32->Flags  = DWORD32(pFR16->Flags) | CD_WOWAPP;
        pFR32->Flags &= ~(FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE);
        pFR32->Flags |= Flags;

        GETPSZPTR(pFR16->lpstrFindWhat, lpstr);
        if(lpstr && pFR32->lpstrFindWhat) {
            WOW32_strncpy(pFR32->lpstrFindWhat, lpstr, WORD32(pFR16->wFindWhatLen));
            FREEPSZPTR(lpstr);
        }

        GETPSZPTR(pFR16->lpstrReplaceWith, lpstr);
        if(lpstr && pFR32->lpstrReplaceWith) {
            WOW32_strncpy(pFR32->lpstrReplaceWith,
                    lpstr,
                    WORD32(pFR16->wReplaceWithLen));
            FREEPSZPTR(lpstr);
        }

        pFR32->wFindWhatLen    = WORD32(pFR16->wFindWhatLen);
        pFR32->wReplaceWithLen = WORD32(pFR16->wReplaceWithLen);
        pFR32->lCustData       = DWORD32(pFR16->lCustData);

        // we always put this WOW hook in so we can destroy the modeless dialog.
        // WCD32FindReplaceDialogPRoc will determine whether to really dispatch
        // to a 16-bit hookproc or not.  pFR16->lpfnHook will be NULL if there
        // isn't a 16-bit hook proc
        pFR32->lpfnHook  = WCD32FindReplaceDialogProc;
        pFR32->Flags    |= FR_ENABLEHOOK;

        // lpTemplateName32 is thunked separately
    }
}





VOID
ThunkFINDREPLACE32to16(OUT PFINDREPLACE16  pFR16,
                       IN  FINDREPLACE    *pFR32)
{
    LPSTR  lpstr;
    DWORD  Flags, Flags32;


    if(pFR16 && pFR32) {

        // Update the 16-bit structure.

        // preserve the template flag state while copying flags
        // 1. save template flag state
        // 2. copy flags from 32-bit struct
        // 3. turn off all template flags and the WOWAPP flag
        // 4. restore original template flag state
        Flags    = DWORD32(pFR16->Flags) & (FR_ENABLETEMPLATE |
                                            FR_ENABLETEMPLATEHANDLE);
        Flags32  = pFR32->Flags;
        Flags32 &= ~(FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE | CD_WOWAPP);
        Flags32 |= Flags;

        // we may have to turn off the hookproc flag if we added it in
        // ThunkFINDREPLACE16to32().
        if(!DWORD32(pFR16->lpfnHook)) {
            Flags32 &= ~FR_ENABLEHOOK;
        }
        STOREDWORD(pFR16->Flags, Flags32);

        GETPSZPTR(pFR16->lpstrFindWhat, lpstr);
        if(lpstr && pFR32->lpstrFindWhat) {
            WOW32_strncpy(lpstr, pFR32->lpstrFindWhat, WORD32(pFR16->wFindWhatLen));
            FREEPSZPTR(lpstr);
        }

        GETPSZPTR(pFR16->lpstrReplaceWith, lpstr);
        if(lpstr && pFR32->lpstrReplaceWith) {
            WOW32_strncpy(lpstr,
                    pFR32->lpstrReplaceWith,
                    WORD32(pFR16->wReplaceWithLen));
            FREEPSZPTR(lpstr);
        }
    }
}





LONG APIENTRY
WCD32UpdateFindReplaceTextAndFlags(HWND hwndOwner,
                                   LPARAM lParam)
{
    PCOMMDLGTD           ptdOwner;
    PFINDREPLACE16       pFR16;
    VPFINDREPLACE        vpfr;
    LPFINDREPLACE        pFR32 = (LPFINDREPLACE) lParam;
    LONG                 lRet = 0;


    ptdOwner = GetCommdlgTd(hwndOwner);
    WOW32ASSERT(ptdOwner);

    vpfr = ptdOwner->vpData;
    GETVDMPTR(vpfr, sizeof(FINDREPLACE16), pFR16);

    ThunkFINDREPLACE32to16(pFR16, pFR32);

    WCDDUMPFINDREPLACE16(pFR16);

    FREEVDMPTR(pFR16);

    return(vpfr);
}





PCOMMDLGTD
GetCommdlgTd(IN HWND Hwnd32)
/*++

Routine Description:

    Searches the thread's chain of commdlg data for the given 32-bit window.
    If the window is not already in the chain, it is added.

Arguments:

    Hwnd32 - Supplies the 32-bit hwnd that the dialog procedure was called
    with.

Return Value:

    Pointer to commdlg data.

--*/
{
    PCOMMDLGTD pTD;

    if ((pTD = CURRENTPTD()->CommDlgTd) == NULL) {
        return(NULL);
    }

    // look for the CommDlgTD struct for this dialog -- usually will be first
    // unless there are nested dialogs
    while (pTD->hdlg != GETHWND16(Hwnd32)) {

        pTD = pTD->Previous;

        // If Hwnd32 isn't in the list, we're probably getting called back
        // from user32 via WOWTellWOWThehDlg().  This means that the dialog
        // window was just created in user32.  Note that this can be either a
        // new dialog or a PrintSetup dialog.
        if (pTD==NULL) {

            pTD = CURRENTPTD()->CommDlgTd;

            while (pTD->hdlg != (HWND16)-1) {

                // Check to see if this is the first call for a PrintSetupHook.
                // It will share the same CommDlgTD as the PrintDlgHook.
                // Note: SetupHwnd will be 1 if this is the 1st time the user
                //       clicks the Setup button in the PrintDlg. Otherwise
                //       it will be the old Hwnd32 from the previous time he
                //       clicked the Setup button from within the same instance
                //       of the PrintDlg. Either way it is non-zero.
                if(pTD->SetupHwnd) {

                    // if the current CommDlgTD->hdlg is the owner of Hwnd32,
                    // we found the CommDlgTD for the PrintSetup dialog.
                    if(pTD->hdlg == GETHWND16(GetWindow(Hwnd32, GW_OWNER))) {
                        pTD->SetupHwnd = GETHWND16(Hwnd32);
                        return(pTD);
                    }
                }

                pTD = pTD->Previous;

                if(pTD == NULL) {
                    WOW32ASSERT(FALSE);
                    return(NULL);
                }
            }

            // set the hdlg for this CommDlgTD
            pTD->hdlg = GETHWND16(Hwnd32);
            return(pTD);
        }
    }

    return(pTD);
}





// Thunks 16-bit Common dialog templates to 32-bit
// Note: this calls back to 16-bit code causing possible 16-bit memory movement
// Note: GetTemplate16 call SETEXTENDEDERROR for *most* failures
HINSTANCE
ThunkCDTemplate16to32(IN     HAND16  hInst16,
                      IN     DWORD   hPT16,  // for PrintDlg only
                      IN     VPVOID  vpTemplateName,
                      IN     DWORD   dwFlags16,
                      IN OUT DWORD  *pFlags,
                      IN     DWORD   ETFlag,   // XX_ENABLETEMPLATE flag
                      IN     DWORD   ETHFlag,  // XX_ENABLETEMPLATEHANDLE flag
                      OUT    PPRES   pRes,
                      OUT    PBOOL   fError)
{
    // Note: struct->hInstance == NULL if neither xx_ENABLExxx flag is set
    HINSTANCE hInst32 = NULL;
    HAND16    hPrintTemp16 = (HAND16)NULL;


    SETEXTENDEDERROR( CDERR_NOTEMPLATE );  // most common error ret

    if(hPT16) {
        hPrintTemp16 = (HAND16)LOWORD(hPT16);
    }

    *pRes = NULL;
    if(dwFlags16 & ETFlag) {

        if(!vpTemplateName) {
            *fError = TRUE;
            return(NULL);
        }

        if(!hInst16) {
            SETEXTENDEDERROR( CDERR_NOHINSTANCE );
            *fError = TRUE;
            return(NULL);
        }

	    // Note: calls to GetTemplate16 may cause 16-bit memory to move
        *pRes = GetTemplate16(hInst16, vpTemplateName, FALSE);

	    if(*pRes == NULL) {
            *fError = TRUE;
            return(NULL);
        }

        hInst32 = (HINSTANCE)LockResource16(*pRes);

        if(!hInst32) {
            *fError = TRUE;
            SETEXTENDEDERROR( CDERR_LOCKRESFAILURE );
            return(NULL);
        }

        *pFlags &= ~ETFlag;
        *pFlags |= ETHFlag;

    } else if(dwFlags16 & ETHFlag) {

        // Win'95 does the following if !hInst && ETHFlag.
        // Note: the return val == FALSE in all cases except the last PD case
        //    CC  (0x00040) -> CDERR_NOTEMPLATE
        //    CF  (0x00020) -> No error (comdlg32 err = CDERR_LOCKRESFAILURE)
        //    FR  (0x02000) -> CDERR_LOCKRESFAILURE
        //    OFN (0x00080) -> CDERR_LOCKRESFAILURE
        //    PD  (0x10000) -> CDERR_LOCKRESFAILURE  (hInstance)
        //    PD  (0x20040) -> CDERR_LOCKRESFAILURE  (with PD_PRINTSETUP)
        //    PD  (0x20000) -> CDERR_LOCKRESFAILURE
        //
        // I think the error value is probably irrelavant since most of these
        // are pathological cases that only developers would see while building
        // and debugging their app.  In the cases where the Win'95 error code is
        // CDERR_LOCKRESFAILURE, comdlg32 sets it to CDERR_NOTEMPLATE (as we
        // now return for WOW) for 32-bit apps

        // one of the hTemplate's should always be set with the
        // ENABLETEMPLATEHANDLE flag

        // if it's a printdlg...
        if(hPT16) {

            // ...the hTemplate should be in either hPrintTemplate or
            // hPrintSetupTemplate
            if(!hPrintTemp16) {
                *fError = TRUE;
            }
        }

        // else for non-printdlg's, the hTemplate should be in hInstance
        else {
            if(!hInst16) {
                *fError = TRUE;
            }
        }

        if(*fError) {
            return(NULL);
        }

	    // Note: calls to GetTemplate16 may cause 16-bit memory to move
        if(hPT16) {
            hInst32 = (HINSTANCE) GetTemplate16(hPrintTemp16,(VPCSTR)NULL,TRUE);
        } else {
            hInst32 = (HINSTANCE) GetTemplate16(hInst16, (VPCSTR)NULL, TRUE);
        }
        if(!hInst32) {
            *fError = TRUE;
            return(NULL);
        }
        *pFlags |= ETHFlag;
    }

    SETEXTENDEDERROR( 0 ); // reset to no error

    return(hInst32);
}




VOID
FreeCDTemplate32(IN PRES      pRes,
                 IN HINSTANCE hInst,
                 IN BOOL      bETFlag,
                 IN BOOL      bETHFlag)
{
    if(pRes && bETFlag) {
        UnlockResource16(pRes);
        FreeResource16(pRes);
    } else if(hInst && bETHFlag) {
        free_w((PVOID)hInst);
    }
}



PRES
GetTemplate16(IN HAND16 hInstance,
              IN VPCSTR lpTemplateName,
              IN BOOLEAN UseHandle)
/*++

Routine Description:

    Finds and loads the specified 16-bit dialog template.

    WARNING: This may cause memory movement, invalidating flat pointers

Arguments:

    hInstance - Supplies the data block containing the dialog box template

    TemplateName - Supplies the name of the resource file for the dialog
        box template.  This may be either a null-terminated string or
        a numbered resource created with the MAKEINTRESOURCE macro.

    UseHandle - Indicates that hInstance identifies a pre-loaded dialog
        box template.  If this is TRUE, Templatename is ignored.

Return Value:

    success - A pointer to the loaded resource

    failure - NULL, dwLastError will be set.

--*/
{
    LPSZ    TemplateName=NULL;
    PRES    pRes;
    PBYTE   pDlg = NULL;
    INT     cb;
    INT     cb16;

    if (!UseHandle) {

        GETPSZIDPTR(lpTemplateName, TemplateName);

        // Both custom instance handle and the dialog template name are
        // specified.  Locate the 16-bit dialog resource in the specified
        // instance block and load it.
        pRes = FindResource16(hInstance,
                              TemplateName,
                              (LPSZ)RT_DIALOG);

        if (HIWORD(lpTemplateName) != 0) {
            FREEVDMPTR(TemplateName);
        }
        if (!pRes) {
            SETEXTENDEDERROR( CDERR_FINDRESFAILURE );
            return(NULL);
        }
        if (!(pRes = LoadResource16(hInstance,pRes))) {
            SETEXTENDEDERROR( CDERR_LOADRESFAILURE );
            return(NULL);
        }

        return(pRes);
    } else {

        VPVOID pDlg16;

        if (pDlg16 = RealLockResource16(hInstance, &cb16)) {
            cb = ConvertDialog16(NULL, pDlg16, 0, cb16);
            if (cb != 0) {
                if (pDlg = malloc_w(cb)) {
                    ConvertDialog16(pDlg, pDlg16, cb, cb16);
                }
            }
            GlobalUnlock16(hInstance);
        }
        else {
            SETEXTENDEDERROR( CDERR_LOCKRESFAILURE );
        }
        return((PRES)pDlg);
    }

}





// When an app calls a ComDlg API it passes a ptr to the appropriate structure.
// On Win3.1 the app & the system share a ptr to the same structure, so when
// either updates the struct, the other is aware of the change.  On NT we thunk
// the 16-bit struct to a 32-bit ANSI struct which is then thunked to a 32-bit
// UNICODE struct by the ComDlg32 code.  We need a mechanism to put all three
// structs in sync.  We attempt to do this by calling ThunkCDStruct32to16()
// from the WCD32xxxxDialogProc()'s (xxxx = Common OR FindReplace) for
// WM_INITDIALOG and WM_COMMAND messages before we callback the 16-bit hook
// proc.  We call ThunkCDStruct16to32() when we return from the 16-bit hook.
VOID
ThunkCDStruct16to32(IN HWND         hDlg,
                    IN CHOOSECOLOR *p32,
                    IN VPVOID       vp)
{

    PCHOOSECOLORDATA16  p16;

    GETVDMPTR(vp, sizeof(CHOOSECOLORDATA16), p16);

    if(p16) {

        switch(p16->lStructSize) {

            case sizeof(CHOOSECOLORDATA16):
                ThunkCHOOSECOLOR16to32(p32, p16);
                Ssync_ANSI_UNICODE_Struct_For_WOW(hDlg, TRUE, WOW_CHOOSECOLOR);
                break;

            case sizeof(CHOOSEFONTDATA16):
                ThunkCHOOSEFONT16to32((CHOOSEFONT *) p32,
                                      (PCHOOSEFONTDATA16) p16);
                Ssync_ANSI_UNICODE_Struct_For_WOW(hDlg, TRUE, WOW_CHOOSEFONT);
                break;

            case sizeof(FINDREPLACE16):
                ThunkFINDREPLACE16to32((FINDREPLACE *) p32,
                                       (PFINDREPLACE16) p16);
                // Find/Replace ANSI-UNICODE sync's are handled by
                // WCD32UpdateFindReplaceTextAndFlags() mechanism
                break;

            case sizeof(OPENFILENAME16):
                ThunkOPENFILENAME16to32((OPENFILENAME *) p32,
                                        (POPENFILENAME16) p16);
                Ssync_ANSI_UNICODE_Struct_For_WOW(hDlg, TRUE, WOW_OPENFILENAME);
                break;

            case sizeof(PRINTDLGDATA16):
                ThunkPRINTDLG16to32((PRINTDLG *) p32, (PPRINTDLGDATA16) p16);
                Ssync_ANSI_UNICODE_Struct_For_WOW(hDlg, TRUE, WOW_PRINTDLG);
                break;

        }

        FREEVDMPTR(p16);

    }
}





VOID
ThunkCDStruct32to16(IN HWND         hDlg,
                    IN VPVOID       vp,
                    IN CHOOSECOLOR *p32)
{

    PCHOOSECOLORDATA16  p16;

    GETVDMPTR(vp, sizeof(CHOOSECOLORDATA16), p16);

    if(p16) {

        switch(p16->lStructSize) {

            case sizeof(CHOOSECOLORDATA16):
                Ssync_ANSI_UNICODE_Struct_For_WOW(hDlg, FALSE, WOW_CHOOSECOLOR);
                ThunkCHOOSECOLOR32to16(p16, p32);
                break;

            case sizeof(CHOOSEFONTDATA16):
                Ssync_ANSI_UNICODE_Struct_For_WOW(hDlg, FALSE, WOW_CHOOSEFONT);
                ThunkCHOOSEFONT32to16((PCHOOSEFONTDATA16) p16,
                                      (CHOOSEFONT *) p32);
                break;

            case sizeof(FINDREPLACE16):
                // Find/Replace ANSI-UNICODE sync's are handled by
                // WCD32UpdateFindReplaceTextAndFlags() mechanism
                ThunkFINDREPLACE32to16((PFINDREPLACE16) p16,
                                       (FINDREPLACE *) p32);
                break;

            case sizeof(OPENFILENAME16):
                Ssync_ANSI_UNICODE_Struct_For_WOW(hDlg, FALSE, WOW_OPENFILENAME);
                ThunkOPENFILENAME32to16((POPENFILENAME16) p16,
                                        (OPENFILENAME *) p32,
                                        TRUE);
                break;

            case sizeof(PRINTDLGDATA16):
                Ssync_ANSI_UNICODE_Struct_For_WOW(hDlg, FALSE, WOW_PRINTDLG);
                ThunkPRINTDLG32to16(vp, (PRINTDLG *) p32);
                break;

        }

        FREEVDMPTR(p16);

    }
}




VOID Multi_strcpy(LPSTR dst, LPCSTR src)
/*++
  strcpy for string lists that have several strings that are separated by
  a null char and is terminated by two NULL chars.
--*/
{
    if(src && dst) {

        while(*src) {
            while(*dst++ = *src++)
                ;
        }
        *dst = '\0';
    }
}



INT Multi_strlen(LPCSTR str)
/*++
  strlen for string lists that have several strings that are separated by
  a null char and is terminated by two NULL chars.

  Returns len of str including all NULL *separators* but not the 2nd NULL
  terminator.  ie.  cat0dog00 would return len = 8;
--*/
{
    INT i = 0;

    if(str) {

        while(*str) {
            while(*str++)
                i++;
            i++;  // count the NULL separator
        }
    }

    return(i);
}




VOID Ssync_WOW_CommDlg_Structs(PCOMMDLGTD pTDIn,
                               BOOL f16to32,
                               DWORD dwThunkCSIP)
{
    HWND       hDlg;
    WORD       wCS16;
    PCOMMDLGTD pTDPrev;
    PCOMMDLGTD pTD = pTDIn;


    // we shouldn't sync for calls from krnl386 into wow32 (we found out)
    // eg. when kernel is handling segment not present faults etc.
    if(dwThunkCSIP) {

        wCS16 = HIWORD(dwThunkCSIP);

        if((wCS16 == gwKrnl386CodeSeg1) ||
           (wCS16 == gwKrnl386CodeSeg2) ||
           (wCS16 == gwKrnl386CodeSeg3))    {
                return;
        }
    }

    // since we don't have an hwnd to compare with we really don't know which
    // PCOMMDLGTD is the one we want -- so we have to sync them all.
    // This is only a problem for nested dialogs which is fairly rare.
    while(pTD) {

        // if this hasn't been initialized yet there is nothing to do
        if(pTD->hdlg == (HWND16)-1) {
            break;
        }

        hDlg = HWND32(pTD->hdlg);

        WOW32ASSERTMSG(hDlg,
                       ("WOW:Ssync_WOW_CommDlg_Structs: hDlg not found!\n"));

        //BlockWOWIdle(TRUE);

        if(f16to32) {
            ThunkCDStruct16to32(hDlg, (CHOOSECOLOR *)pTD->pData32, pTD->vpData);
        }
        else {
            ThunkCDStruct32to16(hDlg, pTD->vpData, (CHOOSECOLOR *)pTD->pData32);
        }

        //BlockWOWIdle(FALSE);

        pTDPrev = pTD->Previous;

        // multiple PCOMMDLGTD's in the list means 1 of 2 things:
        //   1. This is a find/replace text dialog
        //   2. This is a screwy nested dialog situation
        if(pTDPrev) {

            // 1. check for find/replace (it uses two PCOMMDLGTD structs and
            //    shares the same pData32 pointer with both)
            if(pTDPrev->pData32 == pTD->pData32) {

                // nothing to do -- they share the same data which was thunked
                // above so we'll go on to the next PCOMMDLGTD in the list
                pTD = pTDPrev->Previous;
            }

            // 2. there are nested dialogs lurking about & we need to sync
            //    each one!
            else {
                pTD = pTDPrev;
            }
        } else {
            break;
        }
    }
}




// There is a special case issue (we found) where certain dialog box
// API calls can pass a pszptr that is in a common dialog struct ie:
// GetDlgItemText(hDlg, id, OFN16->lpstrFile, size).  Our synchronization
// mechanism actually trashes OFN16->lpstrFile when we sync 32->16 upon
// returning from the API call.  To avoid this we will sync 16->32 upon
// returning from the API call (if needed as per the conditions below)
// before we sync 32->16 thus preserving the string returned in the 16-bit
// buffer.  The special case API's identified so far are:
//   GetDlgItemText, GetWindowText(), DlgDirSelectxxxx, and SendDlgItemMessage.
VOID Check_ComDlg_pszptr(PCOMMDLGTD ptd, VPVOID vp)
{
    VPVOID             vpData;
    POPENFILENAME16    p16;


    if(ptd) {

        vpData = ptd->vpData;
        if(vpData) {

            GETVDMPTR(vpData, sizeof(CHOOSECOLORDATA16), p16);

            if(p16) {

                switch(p16->lStructSize) {

                    // Only these 2 ComDlg structures have OUTPUT buffers.

                    case sizeof(CHOOSEFONTDATA16):
                        if((VPVOID)((PCHOOSEFONTDATA16)p16)->lpszStyle == vp) {
                            Ssync_WOW_CommDlg_Structs(ptd, w16to32, 0);
                        }
                        break;

                    case sizeof(OPENFILENAME16):
                        if(((VPVOID)p16->lpstrFilter       == vp) ||
                           ((VPVOID)p16->lpstrCustomFilter == vp) ||
                           ((VPVOID)p16->lpstrFile         == vp) ||
                           ((VPVOID)p16->lpstrFileTitle    == vp) ||
                           ((VPVOID)p16->lpstrInitialDir   == vp) ||
                           ((VPVOID)p16->lpstrTitle        == vp) ||
                           ((VPVOID)p16->lpstrDefExt       == vp))   {

                            Ssync_WOW_CommDlg_Structs(ptd, w16to32, 0);
                        }

                        break;

                } // end switch
            }
        }
    }
}




VOID FASTCALL WOWTellWOWThehDlg(HWND hDlg)
{

    if(CURRENTPTD()->CommDlgTd) {
        if(GetCommdlgTd(hDlg) == NULL) {
            WOW32WARNMSGF(FALSE,
                          ("WOW::WOWTellWOWThehDlg: No unassigned hDlgs\n"));
        }
    }

}





#ifdef DEBUG
void WCDDumpCHOOSECOLORData16(PCHOOSECOLORDATA16 p16)
{
    if (fLogFilter & FILTER_COMMDLG) {
        LOGDEBUG(10, ("CHOOSECOLORDATA16:\n"));
        LOGDEBUG(10, ("\tlStructSize      = %x\n",(p16)->lStructSize));
        LOGDEBUG(10, ("\thwndOwner        = %lx\n",(p16)->hwndOwner));
        LOGDEBUG(10, ("\thInstance        = %lx\n",(p16)->hInstance));
        LOGDEBUG(10, ("\trgbResult        = %lx\n",(p16)->rgbResult));
        LOGDEBUG(10, ("\tlpCustColors     = %lx\n",(p16)->lpCustColors));
        LOGDEBUG(10, ("\tFlags            = %lx\n",(p16)->Flags));
        LOGDEBUG(10, ("\tlCustData        = %lx\n",(p16)->lCustData));
        LOGDEBUG(10, ("\tlpfnHook         = %lx\n",(p16)->lpfnHook));
        LOGDEBUG(10, ("\tlpTemplateName = %lx\n",(p16)->lpTemplateName));
    }
}


void WCDDumpCHOOSECOLORData32(CHOOSECOLOR *p32)
{
    if (fLogFilter & FILTER_COMMDLG) {
        LOGDEBUG(10, ("CHOOSECOLORDATA32:\n"));
        LOGDEBUG(10, ("\tlStructSize      = %x\n",(p32)->lStructSize));
        LOGDEBUG(10, ("\thwndOwner        = %lx\n",(p32)->hwndOwner));
        LOGDEBUG(10, ("\thInstance        = %lx\n",(p32)->hInstance));
        LOGDEBUG(10, ("\trgbResult        = %lx\n",(p32)->rgbResult));
        LOGDEBUG(10, ("\tlpCustColors     = %lx\n",(p32)->lpCustColors));
        LOGDEBUG(10, ("\tFlags            = %lx\n",(p32)->Flags));
        LOGDEBUG(10, ("\tlCustData        = %lx\n",(p32)->lCustData));
        LOGDEBUG(10, ("\tlpfnHook         = %lx\n",(p32)->lpfnHook));
        LOGDEBUG(10, ("\tlpTemplateName = %lx\n",(p32)->lpTemplateName));
    }
}


void WCDDumpCHOOSEFONTData16(PCHOOSEFONTDATA16 p16)
{
    if (fLogFilter & FILTER_COMMDLG) {
        LOGDEBUG(10, ("CHOOSEFONT16:\n"));
        LOGDEBUG(10, ("\tlStructSize   = %lx\n",(p16)->lStructSize));
        LOGDEBUG(10, ("\thwndOwner     = %lx\n",(p16)->hwndOwner));
        LOGDEBUG(10, ("\thDC           = %lx\n",(p16)->hDC));
        LOGDEBUG(10, ("\tlpLogFont     = %lx\n",(p16)->lpLogFont));
        LOGDEBUG(10, ("\tiPointSize    = %x\n",(p16)->iPointSize));
        LOGDEBUG(10, ("\tiFlags        = %lx\n",(p16)->Flags));
        LOGDEBUG(10, ("\trbgColors     = %lx\n",(p16)->rgbColors));
        LOGDEBUG(10, ("\tlCustData     = %lx\n",(p16)->lCustData));
        LOGDEBUG(10, ("\tlpfnHook      = %lx\n",(p16)->lpfnHook));
        LOGDEBUG(10, ("\tlpTemplateName= %lx\n",(p16)->lpTemplateName));
        LOGDEBUG(10, ("\thInstance     = %lx\n",(p16)->hInstance));
        LOGDEBUG(10, ("\tlpszStyle     = %lx\n",(p16)->lpszStyle));
        LOGDEBUG(10, ("\tnFontType     = %x\n",(p16)->nFontType));
        LOGDEBUG(10, ("\tnSizeMin      = %x\n",(p16)->nSizeMin));
        LOGDEBUG(10, ("\tnSizeMax      = %x\n",(p16)->nSizeMax));
    }
}


void WCDDumpCHOOSEFONTData32(CHOOSEFONT *p32)
{
    if (fLogFilter & FILTER_COMMDLG) {
        LOGDEBUG(10, ("CHOOSEFONT32:\n"));
        LOGDEBUG(10, ("\tlStructSize   = %lx\n",(p32)->lStructSize));
        LOGDEBUG(10, ("\thwndOwner     = %lx\n",(p32)->hwndOwner));
        LOGDEBUG(10, ("\thDC           = %lx\n",(p32)->hDC));
        LOGDEBUG(10, ("\tlpLogFont     = %lx\n",(p32)->lpLogFont));
        LOGDEBUG(10, ("\tiPointSize    = %lx\n",(p32)->iPointSize));
        LOGDEBUG(10, ("\tiFlags        = %lx\n",(p32)->Flags));
        LOGDEBUG(10, ("\trbgColors     = %lx\n",(p32)->rgbColors));
        LOGDEBUG(10, ("\tlCustData     = %lx\n",(p32)->lCustData));
        LOGDEBUG(10, ("\tlpfnHook      = %lx\n",(p32)->lpfnHook));
        LOGDEBUG(10, ("\tlpTemplateName= %lx\n",(p32)->lpTemplateName));
        LOGDEBUG(10, ("\thInstance     = %lx\n",(p32)->hInstance));
        LOGDEBUG(10, ("\tlpszStyle     = %lx\n",(p32)->lpszStyle));
        LOGDEBUG(10, ("\tnFontType     = %x\n",(p32)->nFontType));
        LOGDEBUG(10, ("\tnSizeMin      = %lx\n",(p32)->nSizeMin));
        LOGDEBUG(10, ("\tnSizeMax      = %lx\n",(p32)->nSizeMax));
    }
}


void WCDDumpFINDREPLACE16(PFINDREPLACE16 p16)
{
    if (fLogFilter & FILTER_COMMDLG) {
        LOGDEBUG(10, ("FINDREPLACE16:\n"));
        LOGDEBUG(10, ("\tlStructSize      = %lx\n",(p16)->lStructSize));
        LOGDEBUG(10, ("\thwndOwner        = %x\n",(p16)->hwndOwner));
        LOGDEBUG(10, ("\thInstance        = %x\n",(p16)->hInstance));
        LOGDEBUG(10, ("\tFlags            = %x\n",(p16)->Flags));
        LOGDEBUG(10, ("\tlpstrFindWhat    = %lx\n",(p16)->lpstrFindWhat));
        LOGDEBUG(10, ("\tlpstrReplaceWith = %lx\n",(p16)->lpstrReplaceWith));
        LOGDEBUG(10, ("\twFindWhatLen     = %x\n",(p16)->wFindWhatLen));
        LOGDEBUG(10, ("\twReplaceWithLen  = %x\n",(p16)->wReplaceWithLen));
        LOGDEBUG(10, ("\tlCustData     = %lx\n",(p16)->lCustData));
        LOGDEBUG(10, ("\tlpfnHook      = %lx\n",(p16)->lpfnHook));
        LOGDEBUG(10, ("\tlpTemplateName= %lx\n",(p16)->lpTemplateName));
    }
}


void WCDDumpFINDREPLACE32(FINDREPLACE *p32)
{
    if (fLogFilter & FILTER_COMMDLG) {
        LOGDEBUG(10, ("FINDREPLACE32:\n"));
        LOGDEBUG(10, ("\tlStructSize      = %lx\n",(p32)->lStructSize));
        LOGDEBUG(10, ("\thwndOwner        = %x\n",(p32)->hwndOwner));
        LOGDEBUG(10, ("\thInstance        = %x\n",(p32)->hInstance));
        LOGDEBUG(10, ("\tFlags            = %x\n",(p32)->Flags));
        LOGDEBUG(10, ("\tlpstrFindWhat    = %s\n",(p32)->lpstrFindWhat));
        LOGDEBUG(10, ("\tlpstrReplaceWith = %s\n",(p32)->lpstrReplaceWith));
        LOGDEBUG(10, ("\twFindWhatLen     = %x\n",(p32)->wFindWhatLen));
        LOGDEBUG(10, ("\twReplaceWithLen  = %x\n",(p32)->wReplaceWithLen));
        LOGDEBUG(10, ("\tlCustData     = %lx\n",(p32)->lCustData));
        LOGDEBUG(10, ("\tlpfnHook      = %lx\n",(p32)->lpfnHook));
        LOGDEBUG(10, ("\tlpTemplateName= %lx\n",(p32)->lpTemplateName));
    }
}


void WCDDumpOPENFILENAME16(POPENFILENAME16 p16)
{
    if (fLogFilter & FILTER_COMMDLG) {
        LOGDEBUG(10, ("OPENFILENAME16:\n"));
        LOGDEBUG(10, ("\tlStructSize      = %x\n",(p16)->lStructSize));
        LOGDEBUG(10, ("\thwndOwner        = %lx\n",(p16)->hwndOwner));
        LOGDEBUG(10, ("\thInstance        = %lx\n",(p16)->hInstance));
        LOGDEBUG(10, ("\tlpstrFilter      = %lx\n",(p16)->lpstrFilter));
        LOGDEBUG(10, ("\tlpstrCustomFilter= %lx\n",(p16)->lpstrCustomFilter));
        LOGDEBUG(10, ("\tnMaxCustFilter   = %lx\n",(p16)->nMaxCustFilter));
        LOGDEBUG(10, ("\tnFilterIndex     = %lx\n",(p16)->nFilterIndex));
        LOGDEBUG(10, ("\tlpstrFile        = %lx\n",(p16)->lpstrFile));
        LOGDEBUG(10, ("\tnMaxFile         = %lx\n",(p16)->nMaxFile));
        LOGDEBUG(10, ("\tlpstrFileTitle   = %lx\n",(p16)->lpstrFileTitle));
        LOGDEBUG(10, ("\tnMaxFileTitle    = %lx\n",(p16)->nMaxFileTitle));
        LOGDEBUG(10, ("\tlpstrInitialDir  = %lx\n",(p16)->lpstrInitialDir));
        LOGDEBUG(10, ("\tlpstrTitle       = %lx\n",(p16)->lpstrTitle));
        LOGDEBUG(10, ("\tFlags            = %lx\n",(p16)->Flags));
        LOGDEBUG(10, ("\tnFileOffset      = %lx\n",(p16)->nFileOffset));
        LOGDEBUG(10, ("\tnFileExtension   = %lx\n",(p16)->nFileExtension));
        LOGDEBUG(10, ("\tlpstrDefExt      = %lx\n",(p16)->lpstrDefExt));
        LOGDEBUG(10, ("\tlCustData        = %lx\n",(p16)->lCustData));
        LOGDEBUG(10, ("\tlpfnHook         = %lx\n",(p16)->lpfnHook));
        LOGDEBUG(10, ("\tlpTemplateName   = %lx\n",(p16)->lpTemplateName));
    }
}


void WCDDumpOPENFILENAME32(OPENFILENAME *p32)
{
    if (fLogFilter & FILTER_COMMDLG) {
        LOGDEBUG(10, ("OPENFILENAME32:\n"));
        LOGDEBUG(10, ("\tlStructSize      = %x\n",(p32)->lStructSize));
        LOGDEBUG(10, ("\thwndOwner        = %lx\n",(p32)->hwndOwner));
        LOGDEBUG(10, ("\thInstance        = %lx\n",(p32)->hInstance));
        LOGDEBUG(10, ("\tlpstrFilter      = %s\n",(p32)->lpstrFilter));
        LOGDEBUG(10, ("\tlpstrCustomFilter= %s\n",(p32)->lpstrCustomFilter));
        LOGDEBUG(10, ("\tnMaxCustFilter   = %lx\n",(p32)->nMaxCustFilter));
        LOGDEBUG(10, ("\tnFilterIndex     = %lx\n",(p32)->nFilterIndex));
        LOGDEBUG(10, ("\tlpstrFile        = %s\n",(p32)->lpstrFile));
        LOGDEBUG(10, ("\tnMaxFile         = %lx\n",(p32)->nMaxFile));
        LOGDEBUG(10, ("\tlpstrFileTitle   = %s\n",(p32)->lpstrFileTitle));
        LOGDEBUG(10, ("\tnMaxFileTitle    = %lx\n",(p32)->nMaxFileTitle));
        LOGDEBUG(10, ("\tlpstrInitialDir  = %s\n",(p32)->lpstrInitialDir));
        LOGDEBUG(10, ("\tlpstrTitle       = %s\n",(p32)->lpstrTitle));
        LOGDEBUG(10, ("\tFlags            = %lx\n",(p32)->Flags));
        LOGDEBUG(10, ("\tnFileOffset      = %lx\n",(p32)->nFileOffset));
        LOGDEBUG(10, ("\tnFileExtension   = %lx\n",(p32)->nFileExtension));
        LOGDEBUG(10, ("\tlpstrDefExt      = %s\n",(p32)->lpstrDefExt));
        LOGDEBUG(10, ("\tlCustData        = %lx\n",(p32)->lCustData));
        LOGDEBUG(10, ("\tlpfnHook         = %lx\n",(p32)->lpfnHook));
        LOGDEBUG(10, ("\tlpTemplateName   = %lx\n",(p32)->lpTemplateName));
    }
}


void WCDDumpPRINTDLGData16(PPRINTDLGDATA16 p16)
{
    if (fLogFilter & FILTER_COMMDLG) {
        LOGDEBUG(10, ("PRINTDLGData16:\n"));
        LOGDEBUG(10, ("\tlStructSize      = %x\n",(p16)->lStructSize));
        LOGDEBUG(10, ("\thwndOwner        = %lx\n",(p16)->hwndOwner));
        LOGDEBUG(10, ("\thDevMode         = %lx\n",(p16)->hDevMode));
        LOGDEBUG(10, ("\thDevNames        = %lx\n",(p16)->hDevNames));
        LOGDEBUG(10, ("\thDC              = %lx\n",(p16)->hDC));
        LOGDEBUG(10, ("\tFlags            = %lx\n",(p16)->Flags));
        LOGDEBUG(10, ("\tnFromPage        = %d\n",(p16)->nFromPage));
        LOGDEBUG(10, ("\tnToPage          = %d\n",(p16)->nToPage));
        LOGDEBUG(10, ("\tnMinPage         = %d\n",(p16)->nMinPage));
        LOGDEBUG(10, ("\tnMaxPage         = %d\n",(p16)->nMaxPage));
        LOGDEBUG(10, ("\tnCopies          = %d\n",(p16)->nCopies));
        LOGDEBUG(10, ("\thInstance        = %lx\n",(p16)->hInstance));
        LOGDEBUG(10, ("\tlCustData        = %lx\n",(p16)->lCustData));
        LOGDEBUG(10, ("\tlpfnPrintHook    = %lx\n",(p16)->lpfnPrintHook));
        LOGDEBUG(10, ("\tlpfnSetupHook    = %lx\n",(p16)->lpfnSetupHook));
        LOGDEBUG(10, ("\tlpPrintTemplateName = %lx\n",(p16)->lpPrintTemplateName));
        LOGDEBUG(10, ("\tlpSetupTemplateName = %lx\n",(p16)->lpSetupTemplateName));
        LOGDEBUG(10, ("\thPrintTemplate   = %lx\n",(p16)->hPrintTemplate));
        LOGDEBUG(10, ("\thSetupTemplate   = %lx\n",(p16)->hSetupTemplate));
    }
}


void WCDDumpPRINTDLGData32(PRINTDLG *p32)
{
    if (fLogFilter & FILTER_COMMDLG) {
        LOGDEBUG(10, ("PRINTDLGData32:\n"));
        LOGDEBUG(10, ("\tlStructSize      = %x\n",(p32)->lStructSize));
        LOGDEBUG(10, ("\thwndOwner        = %lx\n",(p32)->hwndOwner));
        LOGDEBUG(10, ("\thDevMode         = %lx\n",(p32)->hDevMode));
        LOGDEBUG(10, ("\thDevNames        = %lx\n",(p32)->hDevNames));
        LOGDEBUG(10, ("\thDC              = %lx\n",(p32)->hDC));
        LOGDEBUG(10, ("\tFlags            = %lx\n",(p32)->Flags));
        LOGDEBUG(10, ("\tnFromPage        = %d\n",(p32)->nFromPage));
        LOGDEBUG(10, ("\tnToPage          = %d\n",(p32)->nToPage));
        LOGDEBUG(10, ("\tnMinPage         = %d\n",(p32)->nMinPage));
        LOGDEBUG(10, ("\tnMaxPage         = %d\n",(p32)->nMaxPage));
        LOGDEBUG(10, ("\tnCopies          = %d\n",(p32)->nCopies));
        LOGDEBUG(10, ("\thInstance        = %lx\n",(p32)->hInstance));
        LOGDEBUG(10, ("\tlCustData        = %lx\n",(p32)->lCustData));
        LOGDEBUG(10, ("\tlpfnPrintHook    = %lx\n",(p32)->lpfnPrintHook));
        LOGDEBUG(10, ("\tlpfnSetupHook    = %lx\n",(p32)->lpfnSetupHook));
        LOGDEBUG(10, ("\tlpPrintTemplateName = %lx\n",(p32)->lpPrintTemplateName));
        LOGDEBUG(10, ("\tlpSetupTemplateName = %lx\n",(p32)->lpSetupTemplateName));
        LOGDEBUG(10, ("\thPrintTemplate   = %lx\n",(p32)->hPrintTemplate));
        LOGDEBUG(10, ("\thSetupTemplate   = %lx\n",(p32)->hSetupTemplate));
    }
}

#endif  // DEBUG
