//*****************************************************************************
//
// HOOKS -
//
//     32bit stubs and thunks for 16bit hooks
//
//
// 01-07-92  NanduriR   Created.
//
//*****************************************************************************

#include "precomp.h"
#pragma hdrstop

MODNAME(wowhooks.c);

//*****************************************************************************
//
// Global data.  Data is valid only in WOW process context. DLL Data is not
// Shared amongst various processes. If a WOW hook is set WIN32 USER will call
// the hookproc in the context of the thread that has the hook set. This implies
// that all the global data in this DLL is acessible by every WOW thread.
//
// Just reaffirming: since USER will call the hookprocs in our process/thread
//                   context there is no need for this data to be available
//                   in shared memory.
//
//*****************************************************************************



HOOKPERPROCESSDATA vaHookPPData = { NULL, 0};

//
// In SetWindowsHook we return an index into this array if it was
// successful. Since NULL implies an error, we cannot use the Zeroth element
// in this array. So we set its 'InUse' flag to TRUE.
//


HOOKSTATEDATA vaHookStateData[] = {
           0,  TRUE, 0, NULL, (HKPROC)NULL,                0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc01,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc02,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc03,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc04,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc05,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc06,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc07,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc08,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc09,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc10,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc11,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc12,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc13,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc14,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc15,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc16,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc17,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc18,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc19,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc20,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc21,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc22,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc23,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc24,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc25,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc26,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc27,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc28,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc29,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc30,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc31,0, 0, 0, NULL,
           0, FALSE, 0, NULL, (HKPROC)WU32SubStdHookProc32,0, 0, 0, NULL
          };


HOOKPARAMS vHookParams = {0, 0, 0};

INT viCurrentHookStateDataIndex = 0;
                                 // this is used when 'DefHookProc' is called.



//*****************************************************************************
// Sub-Standard Hook Procs -
//
// Hook Stubs. The 'index' (the fourth parameter) is an index into the
// the HookStateData array. All needed info is available in this array.
// The hook stubs need to be exported.
//
//*****************************************************************************


LONG APIENTRY WU32SubStdHookProc01(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x01);
}

LONG APIENTRY WU32SubStdHookProc02(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x02);
}

LONG APIENTRY WU32SubStdHookProc03(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x03);
}

LONG APIENTRY WU32SubStdHookProc04(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x04);
}

LONG APIENTRY WU32SubStdHookProc05(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x05);
}

LONG APIENTRY WU32SubStdHookProc06(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x06);
}

LONG APIENTRY WU32SubStdHookProc07(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x07);
}

LONG APIENTRY WU32SubStdHookProc08(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x08);
}

LONG APIENTRY WU32SubStdHookProc09(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x09);
}

LONG APIENTRY WU32SubStdHookProc10(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x0a);
}

LONG APIENTRY WU32SubStdHookProc11(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x0b);
}

LONG APIENTRY WU32SubStdHookProc12(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x0c);
}

LONG APIENTRY WU32SubStdHookProc13(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x0d);
}

LONG APIENTRY WU32SubStdHookProc14(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x0e);
}

LONG APIENTRY WU32SubStdHookProc15(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x0f);
}
LONG APIENTRY WU32SubStdHookProc16(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x10);
}
LONG APIENTRY WU32SubStdHookProc17(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x11);
}
LONG APIENTRY WU32SubStdHookProc18(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x12);
}
LONG APIENTRY WU32SubStdHookProc19(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x13);
}
LONG APIENTRY WU32SubStdHookProc20(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x14);
}
LONG APIENTRY WU32SubStdHookProc21(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x15);
}
LONG APIENTRY WU32SubStdHookProc22(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x16);
}
LONG APIENTRY WU32SubStdHookProc23(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x17);
}
LONG APIENTRY WU32SubStdHookProc24(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x18);
}
LONG APIENTRY WU32SubStdHookProc25(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x19);
}
LONG APIENTRY WU32SubStdHookProc26(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x1a);
}
LONG APIENTRY WU32SubStdHookProc27(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x1b);
}
LONG APIENTRY WU32SubStdHookProc28(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x1c);
}
LONG APIENTRY WU32SubStdHookProc29(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x1d);
}
LONG APIENTRY WU32SubStdHookProc30(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x1e);
}
LONG APIENTRY WU32SubStdHookProc31(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x1f);
}
LONG APIENTRY WU32SubStdHookProc32(INT nCode, LONG wParam, LONG lParam)
{
    return WU32StdHookProc(nCode, wParam, lParam, 0x20);
}

//*****************************************************************************
// W32InitHookState:
//
// Initializes the global data. Note that this data is initialized for every
// Process that loads this DLL. However data that is visible only to the 'WOW'
// process is what we care about.
//
//*****************************************************************************


BOOL W32InitHookState(HANDLE hMod)
{
    INT      i;

    vaHookPPData.hMod = hMod;
    vaHookPPData.cHookProcs = sizeof(vaHookStateData) /
                                                 sizeof(vaHookStateData[0]);

    for (i = 0; i < vaHookPPData.cHookProcs; i++) {
         vaHookStateData[i].iIndex = (BYTE)i;
         vaHookStateData[i].hMod = hMod;
    }

    return TRUE;
}



//*****************************************************************************
// W32GetNotInUseHookStateData:
//
// Steps through the Global HookStateData and returns a pointer to an 'unused'
// element. This is called only when the Hook is being Set.
//
//*****************************************************************************


BOOL W32GetNotInUseHookStateData(LPHOOKSTATEDATA lpData)
{
    INT i;
    for (i = 0; i < vaHookPPData.cHookProcs; i++) {
         if (!vaHookStateData[i].InUse) {
             vaHookStateData[i].InUse = TRUE;
             *lpData = vaHookStateData[i];
             return TRUE;
         }
    }
    LOGDEBUG(LOG_ALWAYS, ("W32GetNotInUseHookStateData: All thunk hook procs in use.\n"));
    return FALSE;
}



//*****************************************************************************
// W32SetHookStateData:
//
// Writes into the global data structure at the specified index.
//
//*****************************************************************************

BOOL W32SetHookStateData(LPHOOKSTATEDATA lpData)
{
    vaHookStateData[lpData->iIndex] = *lpData;
    return TRUE;
}

//*****************************************************************************
// W32GetHookStateData:
//
// Retrieves data from the global data structure at the specified index.
//
//*****************************************************************************

BOOL W32GetHookStateData(LPHOOKSTATEDATA lpData)
{
    if ( lpData->iIndex >= 0 && lpData->iIndex < vaHookPPData.cHookProcs ) {
        *lpData = vaHookStateData[lpData->iIndex];
        return TRUE;
    } else {
        return FALSE;
    }
}

//*****************************************************************************
// W32GetThunkHookProc:
//
//     Its callled to find a 32stub for the hook that is being set.
//     Returns TRUE if successful else FALSE.
//     The data is partially updated to reflect the characteristics of the hook
//     that is being set.
//
//*****************************************************************************

BOOL W32GetThunkHookProc(INT iHook, DWORD Proc16, LPHOOKSTATEDATA lpData)
{
    register PTD ptd = CURRENTPTD();

    if (W32GetNotInUseHookStateData(lpData)) {
        lpData->iHook  = iHook;
        lpData->Proc16 = Proc16;
        lpData->TaskId = ptd->htask16 ;
        W32SetHookStateData(lpData);
        return TRUE;
    }
    else
        return FALSE;

}



//*****************************************************************************
// W32IsDuplicateHook:
//
//     Verifies if the given hook has already been set. This is to catch
//     certain apps which go on Setting the same hook without Unhooking the
//     previous hook.
//
//     Returns the 'stubindex' if hook already exists else 0;
//
//*****************************************************************************

INT W32IsDuplicateHook(INT iHook, DWORD Proc16, INT TaskId)
{
    INT i;
    for (i = 0; i < vaHookPPData.cHookProcs; i++) {
         if (vaHookStateData[i].InUse &&
                 vaHookStateData[i].iHook == iHook &&
                 vaHookStateData[i].TaskId == TaskId &&
                 vaHookStateData[i].Proc16 == Proc16      ) {
             return i;
         }
    }

    return 0;
}

//*****************************************************************************
// W32FreeHHook:
//
//     The state of the specified hook is set to 'Not in use'.
//     Returns hHook of the hook being freed.
//
//*****************************************************************************


HHOOK W32FreeHHook(INT iHook, DWORD Proc16)
{

    register PTD ptd = CURRENTPTD();
    INT i;
    for (i = 0; i < vaHookPPData.cHookProcs; i++) {
         if (vaHookStateData[i].InUse &&
             vaHookStateData[i].iHook == iHook &&
             vaHookStateData[i].TaskId == (INT)ptd->htask16 &&
             vaHookStateData[i].Proc16 == Proc16) {
             vaHookStateData[i].InUse = FALSE;
             return vaHookStateData[i].hHook;
         }
    }
    LOGDEBUG(LOG_ALWAYS, ("W32FreeHHook: Couldn't locate the specified hook."));
    return (HHOOK)NULL;
}




//*****************************************************************************
// W32FreeHHookOfIndex:
//
//     The state of the specified hook is set to 'Not in use'.
//     Returns hHook of the hook being freed.
//
//*****************************************************************************


HHOOK W32FreeHHookOfIndex(INT iFunc)
{
    register PTD ptd = CURRENTPTD();

    if (iFunc && iFunc < vaHookPPData.cHookProcs)
        if (vaHookStateData[iFunc].InUse &&
               vaHookStateData[iFunc].TaskId == (INT)ptd->htask16) {
        vaHookStateData[iFunc].InUse = FALSE;
        return vaHookStateData[iFunc].hHook;
    }
    LOGDEBUG(LOG_ALWAYS, ("W32FreeHHookOfIndex: Couldn't locate the specified hook."));
    return (HHOOK)NULL;
}




//*****************************************************************************
// W32GetHookParams:
//
//     Returns the 32bit hookparams of the current hook.
//
//*****************************************************************************


BOOL W32GetHookParams(LPHOOKPARAMS lpHookParams)
{
    if (lpHookParams) {
        *lpHookParams = vHookParams;
    }

    return (BOOL)lpHookParams;
}



//*****************************************************************************
// W32StdHookProc: (Standard Hook Proc)
//
//     All the stubs call this proc.
//     Return value is dependent on the hook type.
//
//*****************************************************************************

LONG APIENTRY WU32StdHookProc(INT nCode, LONG wParam, LONG lParam, INT iFunc)
{
    //
    // USER will call us in WOW context. Just Verify.
    //

    if (!vaHookStateData[iFunc].InUse) {
        // DbgPrint("WU32StdHookProc:Stub %04x called in WRONG process\n", iFunc);
        return FALSE;
    }

    //
    // USER can call us even if we have GP Faulted - so just ignore the input
    //

    if (CURRENTPTD()->dwFlags & TDF_IGNOREINPUT) {
        LOGDEBUG(LOG_ALWAYS,("WU32StdHookProc Ignoring Input\n"));
        WOW32ASSERTMSG(gfIgnoreInputAssertGiven,
                       "WU32StdHookProc: TDF_IGNOREINPUT hack was used, shouldn't be, "
                       "please email DOSWOW with repro instructions.  Hit 'g' to ignore this "
                       "and suppress this assertion from now on.\n");
        gfIgnoreInputAssertGiven = TRUE;
        return FALSE;
    }

    //
    // store the stubindex. If the hookproc calls the DefHookProc() we will
    // be able to findout the Hook Type.
    //

    viCurrentHookStateDataIndex = iFunc;

    //
    // store the hook params. These will be used if DefHookProc gets called
    // by the 16bit stub. We are interested only in lParam.
    //

    vHookParams.nCode = nCode;
    vHookParams.wParam = wParam;
    vHookParams.lParam = lParam;

    switch (vaHookStateData[iFunc].iHook) {
        case WH_CALLWNDPROC:
            return ThunkCallWndProcHook(nCode, wParam, (LPCWPSTRUCT)lParam,
                                                   &vaHookStateData[iFunc]);

        case WH_CBT:

            return ThunkCbtHook(nCode, wParam, lParam,
                                                   &vaHookStateData[iFunc]);


        case WH_KEYBOARD:
            return ThunkKeyBoardHook(nCode, wParam, lParam,
                                                   &vaHookStateData[iFunc]);

        case WH_MSGFILTER:

            // This code is TEMP only and must be fixed. ChandanC 5/2/92.
            if ((WORD)vaHookStateData[iFunc].TaskId !=
                       (WORD)((PTD)CURRENTPTD())->htask16)
                    break;

            WOW32ASSERT((WORD)vaHookStateData[iFunc].TaskId ==
                       (WORD)((PTD)CURRENTPTD())->htask16);
        case WH_SYSMSGFILTER:
        case WH_GETMESSAGE:

            return ThunkMsgFilterHook(nCode, wParam, (LPMSG)lParam,
                               &vaHookStateData[iFunc]);
            break;

        case WH_JOURNALPLAYBACK:
        case WH_JOURNALRECORD:
            return ThunkJournalHook(nCode, wParam, (LPEVENTMSG)lParam,
                                                   &vaHookStateData[iFunc]);

        case WH_DEBUG:
            return ThunkDebugHook(nCode, wParam, lParam,
                                                   &vaHookStateData[iFunc]);


        case WH_MOUSE:
            return ThunkMouseHook(nCode, wParam, (LPMOUSEHOOKSTRUCT)lParam,
                                                   &vaHookStateData[iFunc]);

        case WH_SHELL:
            return ThunkShellHook(nCode, wParam, lParam,
                                                   &vaHookStateData[iFunc]);

        default:
            LOGDEBUG(LOG_ALWAYS,("W32StdHookProc: Unknown Hook type."));
    }

    return (LONG)FALSE;

}



//*****************************************************************************
//
// ThunkHookProc for Hooks of type WH_CALLWNDPROC -
//
//     Return type is VOID.
//
//*****************************************************************************


LONG ThunkCallWndProcHook(INT nCode, LONG wParam, LPCWPSTRUCT lpCwpStruct,
                                                     LPHOOKSTATEDATA lpHSData)
{
    VPVOID      vp;
    PCWPSTRUCT16 pCwpStruct16;
    PARM16 Parm16;
    WM32MSGPARAMEX wm32mpex;
    BOOL   fMessageNeedsThunking;

    wm32mpex.Parm16.WndProc.wMsg   = (WORD) lpCwpStruct->message;
    wm32mpex.Parm16.WndProc.wParam = (WORD) lpCwpStruct->wParam;
    wm32mpex.Parm16.WndProc.lParam = (LONG) lpCwpStruct->lParam;

    fMessageNeedsThunking =  (lpCwpStruct->message < 0x400) &&
                                  (aw32Msg[lpCwpStruct->message].lpfnM32 != WM32NoThunking);
    // This call to stackalloc16() needs to occur before the call to the message
    // thunking function call below ((wm32mpex.lpfnM32)(&wm32mpex)) because the
    // message thunks for some messages may also call stackalloc16(). This will 
    // ensure proper nesting of stackalloc16() & stackfree16() calls.
    // Be sure allocation size matches stackfree16() size below
    vp = stackalloc16(sizeof(CWPSTRUCT16));

    if (fMessageNeedsThunking) {

        LOGDEBUG(3,("%04X (%s)\n", CURRENTPTD()->htask16, (aw32Msg[lpCwpStruct->message].lpszW32)));

        wm32mpex.fThunk = THUNKMSG;
        wm32mpex.hwnd   = lpCwpStruct->hwnd;
        wm32mpex.uMsg   = lpCwpStruct->message;
        wm32mpex.uParam = lpCwpStruct->wParam;
        wm32mpex.lParam = lpCwpStruct->lParam;
        wm32mpex.lpfnM32 = aw32Msg[wm32mpex.uMsg].lpfnM32;
        wm32mpex.pww = (PWW)NULL;
        wm32mpex.fFree = TRUE;

        // note: this may call stackalloc16() and/or callback into 16-bit code
        if (!(wm32mpex.lpfnM32)(&wm32mpex)) {
            LOGDEBUG(LOG_ALWAYS,("ThunkCallWndProcHook: cannot thunk 32-bit message %04x\n",
                    lpCwpStruct->message));
        }
    }

    // don't call GETMISCPTR(vp..) until after returning from the thunk function
    // above.  If the thunk function calls back into 16-bit code, the flat ptr
    // for vp could become invalid.
    GETMISCPTR(vp, pCwpStruct16);

    STOREWORD(pCwpStruct16->hwnd, GETHWND16(lpCwpStruct->hwnd));
    STOREWORD(pCwpStruct16->message, wm32mpex.Parm16.WndProc.wMsg  );
    STOREWORD(pCwpStruct16->wParam,  wm32mpex.Parm16.WndProc.wParam);
    STORELONG(pCwpStruct16->lParam,  wm32mpex.Parm16.WndProc.lParam);

    FLUSHVDMPTR(vp, sizeof(CWPSTRUCT16), pCwpStruct16);
    FREEVDMPTR(pCwpStruct16);

    Parm16.HookProc.nCode = (SHORT)nCode;
    Parm16.HookProc.wParam = (SHORT)wParam;
    Parm16.HookProc.lParam = vp;

    CallBack16(RET_HOOKPROC, &Parm16, lpHSData->Proc16, (PVPVOID)&wm32mpex.lReturn);

#ifdef DEBUG
    GETMISCPTR(vp, pCwpStruct16);

    if (pCwpStruct16->message != wm32mpex.Parm16.WndProc.wMsg)
        LOGDEBUG(LOG_ALWAYS,("ThunkCallWndProcHook: IN message != OUT message"));

    FREEVDMPTR(pCwpStruct16);
#endif

    if (fMessageNeedsThunking) {

        wm32mpex.fThunk = UNTHUNKMSG;

        // Note: If the thunk of this message called stackalloc16() this unthunk
        //       call should call stackfree16()
        (wm32mpex.lpfnM32)(&wm32mpex);
    }

    if(vp) {

        // this stackfree16() call must come after the above message unthunk
        // call to give the unthunk the opportunity to call stackfree16() also.
        // this will preserve proper nesting of stackalloc16 & stackfree16 calls
        stackfree16(vp, sizeof(CWPSTRUCT16));
    }

    return (LONG)FALSE;   // return value doesn't matter
}



//*****************************************************************************
//
// ThunkHookProc for Hooks of type WH_CBT -
//
//     Return type is BOOL.
//
//*****************************************************************************

LONG ThunkCbtHook(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn = FALSE;
    PARM16 Parm16;

    VPVOID vp;
    PMOUSEHOOKSTRUCT16 pMHStruct16;
    PCBTACTIVATESTRUCT16 pCbtAStruct16;
    VPCREATESTRUCT16     vpcs16;
    PCBT_CREATEWND16     pCbtCWnd16;
    WM32MSGPARAMEX       wm32mpex;

    Parm16.HookProc.nCode = (SHORT)nCode;
    Parm16.HookProc.wParam = (SHORT)wParam;
    Parm16.HookProc.lParam = lParam;

    switch(nCode) {
        case HCBT_MOVESIZE:

            // wParam = HWND, lParam = LPRECT

            Parm16.HookProc.wParam = GETHWND16(wParam);

            // be sure allocation size matches stackfree16() size below
            vp = stackalloc16(sizeof(RECT16));

            PUTRECT16(vp, (LPRECT)lParam);

            Parm16.HookProc.lParam = vp;

            break;

        case HCBT_MINMAX:

            // wParam = HWND, lParam = SW_*  --- a command

            Parm16.HookProc.wParam = GETHWND16(wParam);
            break;

        case HCBT_QS:

            // wParam = 0, lParam = 0

            break;

        case HCBT_CREATEWND:

            // This stackalloc16() call needs to occur before the WM32Create()
            // call below to ensure proper nesting of stackalloc16 & stackfree16
            // calls.
            // Be sure allocation size matches stackfree16() size(s) below
            vp = stackalloc16(sizeof(CBT_CREATEWND16));

            // wParam = HWND, lParam = LPCBT_CREATEWND

            wm32mpex.fThunk = THUNKMSG;
            wm32mpex.hwnd = (HWND)wParam;
            wm32mpex.uMsg = 0;
            wm32mpex.uParam = 0;
            wm32mpex.lParam = (LONG)((LPCBT_CREATEWND)lParam)->lpcs;
            wm32mpex.pww = (PWW)GetWindowLong((HWND)wParam, GWL_WOWWORDS);
            /*
             * WM32Create now requires that pww be initialized
             * WM32Create calls stackalloc16()!!
             */
            if (!wm32mpex.pww || !WM32Create(&wm32mpex)) {
                stackfree16(vp, sizeof(CBT_CREATEWND16));
                return FALSE;
            }
            vpcs16 = wm32mpex.Parm16.WndProc.lParam;
            lReturn = wm32mpex.lReturn;

            GETMISCPTR(vp, pCbtCWnd16);
            STOREDWORD(pCbtCWnd16->vpcs, vpcs16);
            STOREWORD(pCbtCWnd16->hwndInsertAfter,
            GETHWNDIA16(((LPCBT_CREATEWND)lParam)->hwndInsertAfter));

            Parm16.HookProc.wParam = GETHWND16(wParam);
            Parm16.HookProc.lParam = vp;

            FLUSHVDMPTR(vp, sizeof(CBT_CREATEWND16), pCbtCWnd16);
            FREEVDMPTR(pCbtCWnd16);
            break;

        case HCBT_DESTROYWND:

            // wParam = HWND, lParam = 0

            Parm16.HookProc.wParam = GETHWND16(wParam);
            break;

        case HCBT_ACTIVATE:

            // wParam = HWND, lParam = LPCBTACTIVATESTRUCT

            // be sure allocation size matches stackfree16() size below
            vp = stackalloc16(sizeof(CBTACTIVATESTRUCT16));

            GETMISCPTR(vp, pCbtAStruct16);
            PUTCBTACTIVATESTRUCT16(pCbtAStruct16, ((LPCBTACTIVATESTRUCT)lParam));

            Parm16.HookProc.wParam = GETHWND16(wParam);
            Parm16.HookProc.lParam = vp;

            FLUSHVDMPTR(vp, sizeof(CBTACTIVATESTRUCT16), pCbtAStruct16);
            FREEVDMPTR(pCbtAStruct16);
            break;

        case HCBT_CLICKSKIPPED:

            // wParam = mouse message, lParam = LPMOUSEHOOKSTRUCT

            // be sure allocation size matches stackfree16() size below
            vp = stackalloc16(sizeof(MOUSEHOOKSTRUCT16));

            GETMISCPTR(vp, pMHStruct16);
            PUTMOUSEHOOKSTRUCT16(pMHStruct16, (LPMOUSEHOOKSTRUCT)lParam);

            Parm16.HookProc.lParam = vp;

            FLUSHVDMPTR(vp, sizeof(MOUSEHOOKSTRUCT16), pMHStruct16);
            FREEVDMPTR(pMHStruct16);
            break;

        case HCBT_KEYSKIPPED:

            // wParam, lParam   -- keyup/down message params

            break;

        case HCBT_SYSCOMMAND:

            // wParam = SC_ syscomand, lParam = DWORD(x,y)

            break;

        case HCBT_SETFOCUS:

            // wParam = HWND, lParam = HWND

            Parm16.HookProc.wParam = GETHWND16(wParam);
            Parm16.HookProc.lParam = GETHWND16(lParam);
            break;

        default:
            LOGDEBUG(LOG_ALWAYS, ("ThunkCbtHook: Unknown HCBT_ code\n"));
            break;
    }

    CallBack16(RET_HOOKPROC, &Parm16, lpHSData->Proc16, (PVPVOID)&lReturn);

    switch(nCode) {
        case HCBT_MOVESIZE:

            GETRECT16(vp, (LPRECT)lParam);
            stackfree16(vp, sizeof(RECT16));
            break;

        case HCBT_CREATEWND:
            GETMISCPTR(vp, pCbtCWnd16);
            ((LPCBT_CREATEWND)lParam)->hwndInsertAfter =
                               HWNDIA32(FETCHWORD(pCbtCWnd16->hwndInsertAfter));
            FREEVDMPTR(pCbtCWnd16);
            wm32mpex.fThunk = UNTHUNKMSG;
            wm32mpex.lReturn = lReturn;
            WM32Create(&wm32mpex);  // this calls stackfree16()!!!
            lReturn = wm32mpex.lReturn;
            // this must be after call to WM32Create()
            stackfree16(vp, sizeof(CBT_CREATEWND16));
            break;


        case HCBT_ACTIVATE:

            GETMISCPTR(vp, pCbtAStruct16);
            GETCBTACTIVATESTRUCT16(pCbtAStruct16, (LPCBTACTIVATESTRUCT)lParam);
            FREEVDMPTR(pCbtAStruct16);
            stackfree16(vp, sizeof(CBTACTIVATESTRUCT16));
            break;

        case HCBT_CLICKSKIPPED:

            GETMISCPTR(vp, pMHStruct16);
            GETMOUSEHOOKSTRUCT16(pMHStruct16, (LPMOUSEHOOKSTRUCT)lParam);
            FREEVDMPTR(pMHStruct16);
            stackfree16(vp, sizeof(MOUSEHOOKSTRUCT16));
            break;

        // case HCBT_MINMAX:
        // case HCBT_QS:
        // case HCBT_DESTROYWND:
        // case HCBT_KEYSKIPPED:
        // case HCBT_SYSCOMMAND:
        // case HCBT_SETFOCUS:

        default:
            break;
    }

    // the value in LOWORD is the valid return value

    return (LONG)(BOOL)LOWORD(lReturn);
}



//*****************************************************************************
//
// ThunkHookProc for Hooks of type WH_KEYBOARD -
//
//     Return type is BOOL.
//
//*****************************************************************************

LONG ThunkKeyBoardHook(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn;
    PARM16 Parm16;

    Parm16.HookProc.nCode = (SHORT)nCode;
    Parm16.HookProc.wParam = (SHORT)wParam;
    Parm16.HookProc.lParam = lParam;

    CallBack16(RET_HOOKPROC, &Parm16, lpHSData->Proc16, (PVPVOID)&lReturn);

    // the value in LOWORD is the valid return value

    return (LONG)(BOOL)LOWORD(lReturn);
}



//*****************************************************************************
//
// ThunkHookProc for Hooks of type WH_GETMESSAGE -
//                                 WH_MSGFILTER -
//                                 WH_SYSMSGFILTER -
//
//     WARNING: May cause 16-bit memory movement, invalidating flat pointers.
//
//     Return type is BOOL.
//
//*****************************************************************************

LONG ThunkMsgFilterHook(INT nCode, LONG wParam, LPMSG lpMsg,
                                                     LPHOOKSTATEDATA lpHSData)
{
    VPVOID vp;
    PMSG16 pMsg16;
    PARM16 Parm16;
    WM32MSGPARAMEX wm32mpex;

    BOOL fHookModifiedLpMsg;
    HWND hwnd32;
    MSGPARAMEX mpex;
    BOOL fUseOld;
    static MSG msgSave;
    static int cRecurse = 0;
    static PARM16 Parm16Save;
    BOOL   fMessageNeedsThunking;

    // This should be removed when all thunk functions appropiately fill these
    // in, till then we must have these 3 lines before calling thunk functions !
    // ChandanC, 2/28/92
    //

    wm32mpex.Parm16.WndProc.wMsg   = (WORD) lpMsg->message;
    wm32mpex.Parm16.WndProc.wParam = (WORD) lpMsg->wParam;
    wm32mpex.Parm16.WndProc.lParam = (LONG) lpMsg->lParam;

    fMessageNeedsThunking =  (lpMsg->message < 0x400) &&
                                  (aw32Msg[lpMsg->message].lpfnM32 != WM32NoThunking);
    fThunkDDEmsg = FALSE;

    if (fMessageNeedsThunking) {
        LOGDEBUG(3,("%04X (%s)\n", CURRENTPTD()->htask16,
                                           (aw32Msg[lpMsg->message].lpszW32)));

        wm32mpex.fThunk = THUNKMSG;
        wm32mpex.hwnd   = lpMsg->hwnd;
        wm32mpex.uMsg   = lpMsg->message;
        wm32mpex.uParam = lpMsg->wParam;
        wm32mpex.lParam = lpMsg->lParam;
        wm32mpex.pww = (PWW)NULL;
        wm32mpex.fFree = FALSE;
        wm32mpex.lpfnM32 = aw32Msg[wm32mpex.uMsg].lpfnM32;
        if (!((wm32mpex.lpfnM32)(&wm32mpex))) {
            LOGDEBUG(LOG_ALWAYS,("ThunkMsgFilterHook: cannot thunk 32-bit message %04x\n",
                                                             lpMsg->message));
        }
    }

    fThunkDDEmsg = TRUE;

    //
    // Check to see if we've recursed into this routine. If so and the message
    // contents are the same, use the last message pointer we gave the 16
    // bit app rather than allocating a new one. Need this to solve the
    // SoundBits Browser problem. It checks to see if the lpmsg passed
    // is the same as it got last time - if so, it doesn't call this other
    // function that causes recursion.
    //

    fUseOld = FALSE;

    if (cRecurse != 0 && lpMsg->hwnd == msgSave.hwnd &&
                lpMsg->message == msgSave.message &&
                lpMsg->wParam == msgSave.wParam &&
                lpMsg->lParam == msgSave.lParam &&
                lpMsg->time == msgSave.time &&
                lpMsg->pt.x == msgSave.pt.x &&
                lpMsg->pt.y == msgSave.pt.y) {

        fUseOld = TRUE;

    } else {
        //
        // Not the same... reset this thing in case the count is screwed
        // up. Also remember this message, in case we do recurse.
        //

        cRecurse = 0;
        msgSave = *lpMsg;
    }

    if (!fUseOld) {
        vp = malloc16(sizeof(MSG16));
        if (vp == (VPVOID)NULL)
            return FALSE;

        GETMISCPTR(vp, pMsg16);
        STOREWORD(pMsg16->hwnd, GETHWND16((lpMsg)->hwnd));
        STOREWORD(pMsg16->message, wm32mpex.Parm16.WndProc.wMsg  );
        STOREWORD(pMsg16->wParam,  wm32mpex.Parm16.WndProc.wParam);
        STORELONG(pMsg16->lParam,  wm32mpex.Parm16.WndProc.lParam);
        STORELONG(pMsg16->time, (lpMsg)->time);
        STOREWORD(pMsg16->pt.x, (lpMsg)->pt.x);
        STOREWORD(pMsg16->pt.y, (lpMsg)->pt.y);

        Parm16.HookProc.nCode = (SHORT)nCode;
        Parm16.HookProc.wParam = (SHORT)wParam;
        Parm16.HookProc.lParam = vp;

        //
        // Remember Parm16 in case we need to use it again due to the recursion
        // case.
        //

        Parm16Save = Parm16;

    } else {

        //
        // Use old message contents.
        //

        Parm16 = Parm16Save;
        vp = (VPVOID)Parm16Save.HookProc.lParam;

        GETMISCPTR(vp, pMsg16);
    }

    FLUSHVDMPTR(vp, sizeof(MSG16), pMsg16);

    //
    // Count how many times we recurse through this hook proc. Need this count
    // to solve the SoundBits Browser problem. It checks to see if the lpMsg
    // passed is the same as it got last time - if so, it doesn't call this
    // other function that causes recursion.
    //

    cRecurse++;
    CallBack16(RET_HOOKPROC, &Parm16, lpHSData->Proc16, (PVPVOID)&wm32mpex.lReturn);
    cRecurse--;

    // set the correct return value BEFORE unthunking
    wm32mpex.lReturn = (LONG)(BOOL)LOWORD(wm32mpex.lReturn);

    //
    // Free the 32 pointer to 16 bit args and get it again in case underlying
    // memory movement occured.
    //

    FREEVDMPTR(pMsg16);
    GETMISCPTR(vp, pMsg16);

    fThunkDDEmsg = FALSE;

    // Theoretically an app can change the lpMsg and return a totall different
    // data altogether. In practice most apps don't do anything so it is pretty
    // expensive to go through the 'unthunk' procedure all the time. So here is
    // a compromise. We copy the message params only if the output message is
    // different from the input message.
    //
    // (davehart) Polaris PackRat modifies WM_KEYDOWN messages sent to its
    // multiline "Note" edit control in its Phone Book Entry dialog box.
    // It changes wParam (the VK_ code) from 0xD (VK_RETURN) to 0xA, which
    // is understood by the edit control to be identical to VK_RETURN but
    // won't cause the dialog box to close.  So we special-case here
    // WM_KEYDOWN where the hook proc changed the virtual key (in wParam)
    // and unthunk it properly.

    fHookModifiedLpMsg = (pMsg16->message != wm32mpex.Parm16.WndProc.wMsg) ||
                         (wm32mpex.Parm16.WndProc.wMsg == WM_KEYDOWN &&
                            pMsg16->wParam != wm32mpex.Parm16.WndProc.wParam) ||
                         (pMsg16->hwnd != GETHWND16(lpMsg->hwnd));


    if (fHookModifiedLpMsg) {
        mpex.Parm16.WndProc.hwnd = pMsg16->hwnd;
        mpex.Parm16.WndProc.wMsg = pMsg16->message;
        mpex.Parm16.WndProc.wParam = pMsg16->wParam;
        mpex.Parm16.WndProc.lParam = pMsg16->lParam,
        mpex.iMsgThunkClass = WOWCLASS_WIN16;
        hwnd32 = ThunkMsg16(&mpex);

        //
        // Free the 32 pointer to 16 bit args and get it again in case
        // underlying memory movement occured.
        //

        FREEVDMPTR(pMsg16);
        GETMISCPTR(vp, pMsg16);

        // reset flag if message thunking failed.

        if (!hwnd32)
            fHookModifiedLpMsg = FALSE;
    }

    if (fMessageNeedsThunking) {

        wm32mpex.fThunk = UNTHUNKMSG;
        (wm32mpex.lpfnM32)(&wm32mpex);

        //
        // Free the 32 pointer to 16 bit args and get it again in case
        // underlying memory movement occured.
        //

        FREEVDMPTR(pMsg16);
        GETMISCPTR(vp, pMsg16);
    }

    fThunkDDEmsg = TRUE;

    if (fHookModifiedLpMsg) {
	lpMsg->hwnd = hwnd32;
        lpMsg->message = mpex.uMsg;
        lpMsg->wParam =  mpex.uParam;
        lpMsg->lParam =  mpex.lParam;
        lpMsg->time      = FETCHLONG(pMsg16->time);
        lpMsg->pt.x      = FETCHSHORT(pMsg16->pt.x);
        lpMsg->pt.y      = FETCHSHORT(pMsg16->pt.y);
    }


    FREEVDMPTR(pMsg16);

    if (!fUseOld) {
        free16(vp);
    }

    // the value in LOWORD is the valid return value

    return (LONG)(BOOL)LOWORD(wm32mpex.lReturn);
}



//*****************************************************************************
//
// ThunkHookProc for Hooks of type WH_JOURNALPLAYBACK -
//                                 WH_JOUNRALRECORD -
//
//     Return type is DWORD.
//
//*****************************************************************************

LONG ThunkJournalHook(INT nCode, LONG wParam, LPEVENTMSG lpEventMsg,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn;
    VPVOID vp;
    PEVENTMSG16 pEventMsg16;
    PARM16 Parm16;

    if ( lpEventMsg ) {

        // be sure allocation size matches stackfree16() size below
        vp = stackalloc16(sizeof(EVENTMSG16));

        // The WIN32 EVENTMSG structure has an additional field 'hwnd', which
        // is not there in WIN31 EVENTMSG structure. This field can be ignored
        // without any repercussions.

        if (lpHSData->iHook == WH_JOURNALRECORD) {
            GETMISCPTR(vp, pEventMsg16);
            PUTEVENTMSG16(pEventMsg16, lpEventMsg);
            FLUSHVDMPTR(vp, sizeof(EVENTMSG16), pEventMsg16);
            FREEVDMPTR(pEventMsg16);
        }


    } else {
        // lpEventMsg can be NULL indicating that no message data is requested.
        // If this is the case, there is no need to copy data for either
        // journal record or playback.
        vp = (VPVOID)0;
    }

    Parm16.HookProc.nCode = (SHORT)nCode;
    Parm16.HookProc.wParam = (SHORT)wParam;
    Parm16.HookProc.lParam = vp;

    CallBack16(RET_HOOKPROC, &Parm16, lpHSData->Proc16, (PVPVOID)&lReturn);

    if ( lpEventMsg ) {
        GETMISCPTR(vp, pEventMsg16);

        if (lpHSData->iHook == WH_JOURNALPLAYBACK) {
            GetEventMessage16(pEventMsg16, lpEventMsg);
#ifdef FE_SB
            switch (lpEventMsg->message) {
                case WM_CHAR:
                case WM_CHARTOITEM:
                case WM_DEADCHAR:
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_MENUCHAR:
                case WM_SYSCHAR:
                case WM_SYSDEADCHAR:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_VKEYTOITEM:
                    // only virtual key, not use scan code
                    lpEventMsg->paramL &= 0x0ff;
            }
#endif // FE_SB

#ifdef  DEBUG
            if (MessageNeedsThunking(lpEventMsg->message)) {
                LOGDEBUG(LOG_ALWAYS, ("ThunkJournalHook: Playing back unexpected message 0x%x", lpEventMsg->message));
            }
#endif

        } else {
            WOW32ASSERT(lpHSData->iHook == WH_JOURNALRECORD);
            WOW32ASSERT(aw32Msg[FETCHWORD(pEventMsg16->message)].lpfnM32 == WM32NoThunking);

            // If the app modified the message then copy the new message info
            // (rather than copying it every time we only copy it if the
            // app change the message)
            if (FETCHWORD(pEventMsg16->message) != lpEventMsg->message) {
                GetEventMessage16(pEventMsg16, lpEventMsg);
            }
        }

        FREEVDMPTR(pEventMsg16);
        if(vp) {
            stackfree16(vp, sizeof(EVENTMSG16));
        }
    }

    return lReturn;
}



//*****************************************************************************
//
// ThunkHookProc for Hooks of type WH_DEBUG -
//
//     Return type is BOOL.
//
//*****************************************************************************

LONG ThunkDebugHook(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn;

    lReturn = TRUE;

    UNREFERENCED_PARAMETER(nCode);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(lpHSData);

    LOGDEBUG(LOG_ALWAYS, ("ThunkDebugHook:Not implemented.\n"));

    // the value in LOWORD is the valid return value

    return (LONG)(BOOL)LOWORD(lReturn);
}



//*****************************************************************************
//
// ThunkHookProc for Hooks of type WH_MOUSE -
//
//     Return type is BOOL.
//
//*****************************************************************************

LONG ThunkMouseHook(INT nCode, LONG wParam, LPMOUSEHOOKSTRUCT lpMHStruct,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn;
    VPVOID vp;
    PMOUSEHOOKSTRUCT16 pMHStruct16;
    PARM16 Parm16;

    // be sure allocation size matches stackfree16() size below
    vp = stackalloc16(sizeof(MOUSEHOOKSTRUCT16));

    GETMISCPTR(vp, pMHStruct16);
    PUTMOUSEHOOKSTRUCT16(pMHStruct16, lpMHStruct);

    Parm16.HookProc.nCode = (SHORT)nCode;
    Parm16.HookProc.wParam = (SHORT)wParam;
    Parm16.HookProc.lParam = vp;

    FLUSHVDMPTR(vp, sizeof(MOUSEHOOKSTRUCT16), pMHStruct16);
    FREEVDMPTR(pMHStruct16);

    CallBack16(RET_HOOKPROC, &Parm16, lpHSData->Proc16, (PVPVOID)&lReturn);

    GETMISCPTR(vp, pMHStruct16);
    GETMOUSEHOOKSTRUCT16(pMHStruct16, lpMHStruct);
    FREEVDMPTR(pMHStruct16);
    if(vp) {
       stackfree16(vp, sizeof(MOUSEHOOKSTRUCT16));
    }
    return (LONG)(BOOL)LOWORD(lReturn);
}



//*****************************************************************************
//
// ThunkHookProc for Hooks of type WH_SHELL -
//
//     Return value apparently is zero.
//
//*****************************************************************************

LONG ThunkShellHook(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn;
    PARM16 Parm16;

    Parm16.HookProc.nCode = (SHORT)nCode;
    Parm16.HookProc.lParam = lParam;

    switch (nCode) {
        case HSHELL_WINDOWCREATED:
        case HSHELL_WINDOWDESTROYED:
            Parm16.HookProc.wParam = (SHORT)GETHWND16(wParam);
            break;

        case HSHELL_ACTIVATESHELLWINDOW:
            // fall thru

        default:
            Parm16.HookProc.wParam = (SHORT)wParam;
            break;
    }

    CallBack16(RET_HOOKPROC, &Parm16, lpHSData->Proc16, (PVPVOID)&lReturn);

    // lReturn = 0?

    return (LONG)lReturn;
}



//*****************************************************************************
// W32UnhookHooks:
//
//     Scans the list of active hooks for those for the passed in handle.
//     Those that match are unhooked.
//
//*****************************************************************************

VOID W32UnhookHooks( HAND16 hMod16, BOOL fQueue )
{
    INT i;

    for (i = 0; i < vaHookPPData.cHookProcs; i++) {
        if (vaHookStateData[i].InUse ) {

            if ( !fQueue && ((HAND16)(vaHookStateData[i].hMod16) == hMod16) ) {
                //
                // Unhook this guy!
                //

                if (UnhookWindowsHookEx(vaHookStateData[i].hHook)) {
                    LOGDEBUG(7, ("W32FreeModuleHooks: Freed iHook (WH_*) %04x\n",
                                                vaHookStateData[i].iHook));
                }
                else {
                    LOGDEBUG(LOG_ALWAYS, ("W32FreeModuleHooks: ERROR Freeing iHook (WH_*) %04x\n",
                                                vaHookStateData[i].iHook));
                }

                // reset the state, even if Unhooking failed.

                vaHookStateData[i].TaskId = 0;
                vaHookStateData[i].InUse = FALSE;
            }
        }
    }
}

//*****************************************************************************
// W32FreeOwnedHooks -
//
//     Called during thread exit time. Frees all hooks set by specified thread.
//
//*****************************************************************************

BOOL W32FreeOwnedHooks(INT iTaskId)
{
    INT i;
    for (i = 0; i < vaHookPPData.cHookProcs; i++) {
         if (vaHookStateData[i].InUse &&
                                       vaHookStateData[i].TaskId == iTaskId) {
             if (UnhookWindowsHookEx(vaHookStateData[i].hHook)) {
                 LOGDEBUG(7, ("W32FreeOwnedHooks: Freed iHook (WH_*) %04x\n",
                                             vaHookStateData[i].iHook));
             }
             else {
                 LOGDEBUG(LOG_ALWAYS, ("W32FreeOwnedHooks: ERROR Freeing iHook (WH_*) %04x\n",
                                             vaHookStateData[i].iHook));
             }

             // reset the state, even if Unhooking failed.

             vaHookStateData[i].TaskId = 0;
             vaHookStateData[i].InUse = FALSE;
         }
    }

    return TRUE;
}


//*****************************************************************************
// W32StdDefHookProc: (Standard Def Hook Proc)
//
//     WU32DefHookProc is called here.
//     WARNING: May cause 16-bit memory movement, invalidating flat pointers.
//     Return value is the new lParam.
//
//*****************************************************************************

LONG APIENTRY WU32StdDefHookProc(INT nCode, LONG wParam, LONG lParam, INT iFunc)
{
    switch (vaHookStateData[iFunc].iHook) {
        case WH_CALLWNDPROC:
            return ThunkCallWndProcHook16(nCode, wParam, (VPVOID)lParam,
                                                   &vaHookStateData[iFunc]);

        case WH_CBT:

            return ThunkCbtHook16(nCode, wParam, (VPVOID)lParam,
                                                   &vaHookStateData[iFunc]);


        case WH_KEYBOARD:
            return ThunkKeyBoardHook16(nCode, wParam, lParam,
                                                   &vaHookStateData[iFunc]);

        case WH_MSGFILTER:
        case WH_SYSMSGFILTER:
        case WH_GETMESSAGE:

            return ThunkMsgFilterHook16(nCode, wParam, (VPVOID)lParam,
                               &vaHookStateData[iFunc]);

        case WH_JOURNALPLAYBACK:
        case WH_JOURNALRECORD:
            return ThunkJournalHook16(nCode, wParam, (VPVOID)lParam,
                                                   &vaHookStateData[iFunc]);

        case WH_DEBUG:
            return ThunkDebugHook16(nCode, wParam, lParam,
                                                   &vaHookStateData[iFunc]);


        case WH_MOUSE:
            return ThunkMouseHook16(nCode, wParam, (VPVOID)lParam,
                                                   &vaHookStateData[iFunc]);

        case WH_SHELL:
            return ThunkShellHook16(nCode, wParam, lParam,
                                                   &vaHookStateData[iFunc]);

        default:
            LOGDEBUG(LOG_ALWAYS,("WU32StdDefHookProc: Unknown Hook type.\n"));
    }

    return (LONG)FALSE;

}

//*****************************************************************************
//
// 16->32 ThunkHookProc for Hooks of type WH_CALLWNDPROC -
//
//     Return type is VOID.
//
//*****************************************************************************



LONG ThunkCallWndProcHook16(INT nCode, LONG wParam, VPVOID  vpCwpStruct,
                                                     LPHOOKSTATEDATA lpHSData)
{
    CWPSTRUCT CwpStruct;
    PCWPSTRUCT16 pCwpStruct16;
    MSGPARAMEX mpex;

    GETMISCPTR(vpCwpStruct, pCwpStruct16);

    mpex.Parm16.WndProc.hwnd = pCwpStruct16->hwnd;
    mpex.Parm16.WndProc.wMsg = pCwpStruct16->message;
    mpex.Parm16.WndProc.wParam = pCwpStruct16->wParam;
    mpex.Parm16.WndProc.lParam = pCwpStruct16->lParam,
    mpex.iMsgThunkClass = WOWCLASS_WIN16;

    mpex.hwnd = ThunkMsg16(&mpex);
    // memory may have moved
    FREEVDMPTR(pCwpStruct16);

    CwpStruct.message = mpex.uMsg;
    CwpStruct.wParam  = mpex.uParam;
    CwpStruct.lParam  = mpex.lParam;
    CwpStruct.hwnd    = mpex.hwnd;

    mpex.lReturn = CallNextHookEx(lpHSData->hHook, nCode, wParam,
                                                           (LPARAM)&CwpStruct);
    if (MSG16NEEDSTHUNKING(&mpex)) {
        mpex.uMsg   = CwpStruct.message;
        mpex.uParam = CwpStruct.wParam;
        mpex.lParam = CwpStruct.lParam;
        (mpex.lpfnUnThunk16)(&mpex);
    }

    return mpex.lReturn;
}


//*****************************************************************************
//
// 16->32 ThunkHookProc for Hooks of type WH_CBT -
//
//     Return type is BOOL.
//
//*****************************************************************************

LONG ThunkCbtHook16(INT nCode, LONG wParam, VPVOID lParam,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn = FALSE;
    WPARAM wParamNew;
    LPARAM lParamNew;
    MSGPARAMEX mpex;

    PMOUSEHOOKSTRUCT16   pMHStruct16;
    PCBTACTIVATESTRUCT16 pCbtAStruct16;
    PCBT_CREATEWND16     pCbtCWnd16;
    PRECT16              pRect16;

    MOUSEHOOKSTRUCT      MHStruct;
    RECT                 Rect;
    CBTACTIVATESTRUCT    CbtAStruct;
    CBT_CREATEWND        CbtCWnd;

    // SudeepB 28-May-1996 updated DaveHart 16-July-96 to fix limit assertions.
    //
    // Some apps like SureTrack project management package are passing
    // corrupted values in lParam. GETVDMPTR returns 0 for such an lParam.
    // Using this 0 on X86 causes corruption of IVT and on RISC causes an
    // AV in wow32 as zero is not a valid linear address.  Such apps get
    // away from such criminal acts on win3.1/Win95 as no thunking is needed
    // there.  So all the thunking below explicitly checks for GETVDMPTR
    // returning 0 and in that case just leaves lParam unthunked.

    wParamNew = (WPARAM) wParam;
    lParamNew = (LPARAM) lParam;

    switch(nCode) {

        //
        // These don't have a pointer in lParam.
        //

        case HCBT_SETFOCUS:           // wParam = HWND, lParam = HWND
            // fall through to set wParamNew & lParamNew,
            // this counts on HWND32 zero-extending.

        case HCBT_MINMAX:             // wParam = HWND, lParam = SW_*  --- a command
            // fall through to set wParamNew & lParamNew.
            // this counts on HWND32 zero-extending.

        case HCBT_DESTROYWND:         // wParam = HWND, lParam = 0
            // fall through to set wParamNew & lParamNew.
            // this counts on HWND32 zero-extending.
            WOW32ASSERTMSG((HWND32(0x1234) == (HWND)0x1234),
                "Code depending on HWND32 zero-extending needs revision.\n");

        case HCBT_QS:                 // wParam = 0, lParam = 0
        case HCBT_KEYSKIPPED:         // wParam = VK_ keycode, lParam = WM_KEYUP/DOWN lParam
        case HCBT_SYSCOMMAND:         // wParam = SC_ syscomand, lParam = DWORD(x,y) if mouse
            // lParamNew and wParamNew are initialized above with no thunking.
            break;

        //
        // These use lParam as a pointer.
        //

        case HCBT_MOVESIZE:           // wParam = HWND, lParam = LPRECT

            #if 0    // HWND32 is a no-op, wParamNew already initialized.
                wParamNew = (WPARAM) HWND32(wParam);
            #endif

            GETVDMPTR(lParam, sizeof(*pRect16), pRect16);
            if (pRect16) {
                GETRECT16(lParam, &Rect);
                lParamNew = (LPARAM)&Rect;
                FREEVDMPTR(pRect16);
            }
            break;


        case HCBT_CREATEWND:          // wParam = HWND, lParam = LPCBT_CREATEWND

            #if 0    // HWND32 is a no-op, wParamNew already initialized.
                wParamNew = (WPARAM) HWND32(wParam);
            #endif

            GETVDMPTR(lParam, sizeof(*pCbtCWnd16), pCbtCWnd16);
            if (pCbtCWnd16) {
                lParamNew = (LPARAM)&CbtCWnd;

                mpex.Parm16.WndProc.hwnd = LOWORD(wParam);
                mpex.Parm16.WndProc.wMsg = WM_CREATE;
                mpex.Parm16.WndProc.wParam = 0;
                mpex.Parm16.WndProc.lParam = FETCHDWORD(pCbtCWnd16->vpcs);
                mpex.iMsgThunkClass = 0;

                ThunkMsg16(&mpex);

                //
                // Memory movement can occur on the 16-bit side.
                //

                FREEVDMPTR(pCbtCWnd16);
                GETVDMPTR(lParam, sizeof(*pCbtCWnd16), pCbtCWnd16);

                (LONG)CbtCWnd.lpcs = mpex.lParam;
                CbtCWnd.hwndInsertAfter =
                               HWNDIA32(FETCHWORD(pCbtCWnd16->hwndInsertAfter));

                FREEVDMPTR(pCbtCWnd16);
            }
            break;

        case HCBT_ACTIVATE:           // wParam = HWND, lParam = LPCBTACTIVATESTRUCT

            #if 0    // HWND32 is a no-op, wParamNew already initialized.
                wParamNew = (WPARAM) HWND32(wParam);
            #endif

            GETVDMPTR(lParam, sizeof(*pCbtAStruct16), pCbtAStruct16);
            if (pCbtAStruct16) {
                lParamNew = (LPARAM)&CbtAStruct;
                GETCBTACTIVATESTRUCT16(pCbtAStruct16, &CbtAStruct);
                FREEVDMPTR(pCbtAStruct16);
            }

            break;

        case HCBT_CLICKSKIPPED:       // wParam = mouse message, lParam = LPMOUSEHOOKSTRUCT

            GETVDMPTR(lParam, sizeof(*pMHStruct16), pMHStruct16);
            if (pMHStruct16) {
                lParamNew = (LPARAM)&MHStruct;
                GETMOUSEHOOKSTRUCT16(pMHStruct16, &MHStruct);
                FREEVDMPTR(pMHStruct16);
            }
            break;

        default:
            LOGDEBUG(LOG_ALWAYS, ("ThunkCbtHook: Unknown HCBT_ code 0x%x\n", nCode));
            break;
    }

    //
    // Call the hook, memory movement can occur.
    //

    lReturn = CallNextHookEx(lpHSData->hHook, nCode, wParamNew, lParamNew);


    switch(nCode) {

        //
        // These don't have a pointer in lParam.
        //

        // case HCBT_SETFOCUS:           // wParam = HWND, lParam = HWND
        // case HCBT_MINMAX:             // wParam = HWND, lParam = SW_*  --- a command
        // case HCBT_DESTROYWND:         // wParam = HWND, lParam = 0
        // case HCBT_QS:                 // wParam = 0, lParam = 0
        // case HCBT_KEYSKIPPED:         // wParam = VK_ keycode, lParam = WM_KEYUP/DOWN lParam
        // case HCBT_SYSCOMMAND:         // wParam = SC_ syscomand, lParam = DWORD(x,y) if mouse
        //     break;

        //
        // These use lParam as a pointer.
        //

        case HCBT_MOVESIZE:           // wParam = HWND, lParam = LPRECT

            PUTRECT16(lParam, (LPRECT)lParamNew);
            break;

        case HCBT_CREATEWND:          // wParam = HWND, lParam = LPCBT_CREATEWND

            GETVDMPTR(lParam, sizeof(*pCbtCWnd16), pCbtCWnd16);
            if (pCbtCWnd16) {
                mpex.lParam = (LONG)CbtCWnd.lpcs;
                mpex.lReturn = lReturn;
                WOW32ASSERT(MSG16NEEDSTHUNKING(&mpex));
                (mpex.lpfnUnThunk16)(&mpex);
                lReturn = mpex.lReturn;

                STOREWORD(pCbtCWnd16->hwndInsertAfter,
                          GETHWNDIA16(((LPCBT_CREATEWND)lParamNew)->
                                                         hwndInsertAfter));
                FLUSHVDMPTR((VPVOID)lParam, sizeof(CBT_CREATEWND16), pCbtCWnd16);
                FREEVDMPTR(pCbtCWnd16);
            }
            break;


        case HCBT_ACTIVATE:           // wParam = HWND, lParam = LPCBTACTIVATESTRUCT

            GETVDMPTR(lParam, sizeof(*pCbtAStruct16), pCbtAStruct16);
            if (pCbtAStruct16) {
                PUTCBTACTIVATESTRUCT16(pCbtAStruct16, (LPCBTACTIVATESTRUCT)lParamNew);
                FLUSHVDMPTR((VPVOID)lParam, sizeof(CBTACTIVATESTRUCT16),
                                    pCbtAStruct16);
                FREEVDMPTR(pCbtAStruct16);
            }
            break;

        case HCBT_CLICKSKIPPED:       // wParam = mouse message, lParam = LPMOUSEHOOKSTRUCT

            GETVDMPTR(lParam, sizeof(*pMHStruct16), pMHStruct16);
            if (pMHStruct16) {
                PUTMOUSEHOOKSTRUCT16(pMHStruct16, (LPMOUSEHOOKSTRUCT)lParamNew);
                FLUSHVDMPTR((VPVOID)lParam, sizeof(MOUSEHOOKSTRUCT16),
                                    pMHStruct16);
                FREEVDMPTR(pMHStruct16);
            }
            break;

    }

    // the value in LOWORD is the valid return value

    return (LONG)(BOOL)LOWORD(lReturn);
}



//*****************************************************************************
//
// 16->32 ThunkHookProc for Hooks of type WH_KEYBOARD -
//
//     Return type is BOOL.
//
//*****************************************************************************

LONG ThunkKeyBoardHook16(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn;

    lReturn = CallNextHookEx(lpHSData->hHook, nCode, wParam,
                                                           (LPARAM)lParam);

    // the value in LOWORD is the valid return value

    return (LONG)(BOOL)LOWORD(lReturn);
}



//*****************************************************************************
//
// 16->32 ThunkHookProc for Hooks of type WH_GETMESSAGE -
//                                        WH_MSGFILTER -
//                                        WH_SYSMSGFILTER -
//
//     Return type is BOOL.
//
//*****************************************************************************

LONG ThunkMsgFilterHook16(INT nCode, LONG wParam, VPVOID vpMsg,
                                                     LPHOOKSTATEDATA lpHSData)
{
    PMSG16 pMsg16;
    MSG  Msg;
    MSGPARAMEX mpex;



    GETMISCPTR(vpMsg, pMsg16);

    fThunkDDEmsg = FALSE;

    mpex.Parm16.WndProc.hwnd = pMsg16->hwnd;
    mpex.Parm16.WndProc.wMsg = pMsg16->message;
    mpex.Parm16.WndProc.wParam = pMsg16->wParam;
    mpex.Parm16.WndProc.lParam = pMsg16->lParam;
    mpex.iMsgThunkClass = 0;

    ThunkMsg16(&mpex);

    //
    // Memory movement can occur on the 16-bit side.
    //

    FREEVDMPTR(pMsg16);
    GETMISCPTR(vpMsg, pMsg16);

    fThunkDDEmsg = TRUE;

    Msg.message   = mpex.uMsg;
    Msg.wParam    = mpex.uParam;
    Msg.lParam    = mpex.lParam;
    Msg.hwnd      = HWND32(FETCHWORD(pMsg16->hwnd));
    Msg.time      = FETCHLONG(pMsg16->time);
    Msg.pt.x      = FETCHSHORT(pMsg16->pt.x);
    Msg.pt.y      = FETCHSHORT(pMsg16->pt.y);

    FREEVDMPTR(pMsg16);

    mpex.lReturn = CallNextHookEx(lpHSData->hHook, nCode, wParam,
                                                           (LPARAM)&Msg);
    GETMISCPTR(vpMsg, pMsg16);

    if (MSG16NEEDSTHUNKING(&mpex)) {
        mpex.uMsg   = Msg.message;
        mpex.uParam = Msg.wParam;
        mpex.lParam = Msg.lParam;
        (mpex.lpfnUnThunk16)(&mpex);
	GETMISCPTR(vpMsg, pMsg16);
    }

    STORELONG(pMsg16->time, Msg.time);
    STOREWORD(pMsg16->pt.x, Msg.pt.x);
    STOREWORD(pMsg16->pt.y, Msg.pt.y);

    FLUSHVDMPTR(vpMsg, sizeof(MSG16), pMsg16);
    FREEVDMPTR(pMsg16);

    // the value in LOWORD is the valid return value

    return (LONG)(BOOL)LOWORD(mpex.lReturn);
}



//*****************************************************************************
//
// 16->32 ThunkHookProc for Hooks of type WH_JOURNALPLAYBACK -
//                                        WH_JOUNRALRECORD -
//
//     Return type is DWORD.
//
//*****************************************************************************

LONG ThunkJournalHook16(INT nCode, LONG wParam, VPVOID vpEventMsg,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn;
    PEVENTMSG16 pEventMsg16;
    EVENTMSG    EventMsg;
    LPEVENTMSG  lpEventMsg;

    if ( vpEventMsg ) {


        // The WIN32 EVENTMSG structure has an additional field 'hwnd', which
        // is not there in WIN31 EVENTMSG structure. This field can be ignored
        // without any repercussions.

        if (lpHSData->iHook == WH_JOURNALRECORD) {
            GETMISCPTR(vpEventMsg, pEventMsg16);
            GetEventMessage16(pEventMsg16, &EventMsg);
            EventMsg.hwnd = (HWND)0;
            FREEVDMPTR(pEventMsg16);
        }
        lpEventMsg = &EventMsg;


    } else {
        lpEventMsg = NULL;
    }


    lReturn = CallNextHookEx(lpHSData->hHook, nCode, wParam, (LPARAM)lpEventMsg );

    if ( vpEventMsg ) {

        if (lpHSData->iHook == WH_JOURNALPLAYBACK) {
            GETMISCPTR(vpEventMsg, pEventMsg16);
            PUTEVENTMSG16(pEventMsg16, &EventMsg);
            FLUSHVDMPTR(vpEventMsg, sizeof(EVENTMSG16), pEventMsg16);
            FREEVDMPTR(pEventMsg16);
        }

    }

    return lReturn;
}



//*****************************************************************************
//
// 16->32 ThunkHookProc for Hooks of type WH_DEBUG -
//
//     Return type is BOOL.
//
//*****************************************************************************

LONG ThunkDebugHook16(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn;

    lReturn = TRUE;

    UNREFERENCED_PARAMETER(nCode);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(lpHSData);

    LOGDEBUG(LOG_ALWAYS, ("ThunkDebugHook16:Not implemented.\n"));

    // the value in LOWORD is the valid return value

    return (LONG)(BOOL)LOWORD(lReturn);
}



//*****************************************************************************
//
// 16->32 ThunkHookProc for Hooks of type WH_MOUSE -
//
//     Return type is BOOL.
//
//*****************************************************************************

LONG ThunkMouseHook16(INT nCode, LONG wParam, VPVOID vpMHStruct,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn;
    PMOUSEHOOKSTRUCT16 pMHStruct16;
    MOUSEHOOKSTRUCT    MHStruct;

    GETMISCPTR(vpMHStruct, pMHStruct16);
    GETMOUSEHOOKSTRUCT16(pMHStruct16, &MHStruct);
    FREEVDMPTR(pMHStruct16);

    lReturn = CallNextHookEx(lpHSData->hHook, nCode, wParam,
                                                           (LPARAM)&MHStruct);

    GETMISCPTR(vpMHStruct, pMHStruct16);
    PUTMOUSEHOOKSTRUCT16(pMHStruct16, &MHStruct);
    FLUSHVDMPTR((VPVOID)vpMHStruct, sizeof(MOUSEHOOKSTRUCT16), pMHStruct16);
    FREEVDMPTR(pMHStruct16);

    return (LONG)(BOOL)LOWORD(lReturn);
}



//*****************************************************************************
//
// 16->32 ThunkHookProc for Hooks of type WH_SHELL -
//
//     Return value is apparently zero.
//
//*****************************************************************************

LONG ThunkShellHook16(INT nCode, LONG wParam, LONG lParam,
                                                     LPHOOKSTATEDATA lpHSData)
{
    LONG lReturn;

    switch (nCode) {
        case HSHELL_WINDOWCREATED:
        case HSHELL_WINDOWDESTROYED:
            wParam = (LONG)HWND32(wParam);
            break;

        case HSHELL_ACTIVATESHELLWINDOW:
            // fall thru

        default:
            break;
    }

    lReturn = CallNextHookEx(lpHSData->hHook, nCode, wParam,
                                                           (LPARAM)lParam);

    // lReturn = 0?

    return (LONG)lReturn;
}


//*****************************************************************************
// W32GetHookDDEMsglParam:
//
//     Returns the lParam of the actual hook message. called for dde messages
//     only. returns valid lParam else 0.
//
//*****************************************************************************


DWORD  W32GetHookDDEMsglParam()
{
    INT iFunc;
    LONG lParam;

    iFunc = viCurrentHookStateDataIndex;
    lParam  = vHookParams.lParam;

    if (lParam) {
        switch (vaHookStateData[iFunc].iHook) {
            case WH_CALLWNDPROC:
                lParam =  ((LPCWPSTRUCT)lParam)->lParam;
                break;

            case WH_MSGFILTER:
            case WH_SYSMSGFILTER:
            case WH_GETMESSAGE:
                lParam =  ((LPMSG)lParam)->lParam;
                break;

            default:
                lParam = 0;
        }
    }

    return lParam;

}

//*****************************************************************************
// GetEventMessage16:
//
//*****************************************************************************


VOID GetEventMessage16(PEVENTMSG16 pEventMsg16, LPEVENTMSG  lpEventMsg)
{
    lpEventMsg->message   = FETCHWORD(pEventMsg16->message);
    lpEventMsg->time      = FETCHLONG(pEventMsg16->time);
    if ((lpEventMsg->message >= WM_KEYFIRST) && (lpEventMsg->message <= WM_KEYLAST)) {
        // Key event
        lpEventMsg->paramL =  FETCHWORD(pEventMsg16->paramL);
        lpEventMsg->paramH =  FETCHWORD(pEventMsg16->paramH) & 0x8000;
        lpEventMsg->paramH |= (lpEventMsg->paramL & 0xFF00) >> 8;
        lpEventMsg->paramL &= 0xFF;
    }
    else {
        // Mouse event
        lpEventMsg->paramL = FETCHWORD(pEventMsg16->paramL);
        lpEventMsg->paramH = FETCHWORD(pEventMsg16->paramH);
    }
}
