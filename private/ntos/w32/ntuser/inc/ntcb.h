/****************************** Module Header ******************************\
* Module Name: ntcb.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Kernel mode sending stubs
*
* 07-06-91 ScottLu      Created.
\***************************************************************************/

// If SERVER is UNICODE
//   Copy UNICODE -> UNICODE
//   or Copy ANSI -> UNICODE

// prototypes to client side functions only called by these stubs

// ddetrack.c

DWORD   _ClientCopyDDEIn1(HANDLE hClient, PINTDDEINFO pi);
VOID   _ClientCopyDDEIn2(PINTDDEINFO pi);
HANDLE _ClientCopyDDEOut1(PINTDDEINFO pi);
BOOL xxxClientCopyDDEIn2(PINTDDEINFO pi);
BOOL FixupDdeExecuteIfNecessary(HGLOBAL *phCommands, BOOL fNeedUnicode);
BOOL   _ClientCopyDDEOut2(PINTDDEINFO pi);
BOOL   _ClientFreeDDEHandle(HANDLE hDDE, DWORD flags);
DWORD  _ClientGetDDEFlags(HANDLE hDDE, DWORD flags);


typedef struct _GENERICHOOKHEADER {
    DWORD nCode;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
} GENERICHOOKHEADER, * LPGENERICHOOKHEADER;

#ifdef RECVSIDE
ULONG_PTR CallHookWithSEH(GENERICHOOKHEADER *pmsg, LPVOID pData, LPDWORD pFlags, ULONG_PTR retval) {

    try {
        retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
                pmsg->nCode,
                pmsg->wParam,
                pData,
                pmsg->xParam);

    } except ((*pFlags & HF_GLOBAL) ? W32ExceptionHandler(FALSE, RIP_WARNING) : EXCEPTION_CONTINUE_SEARCH) {
        RIPMSG0(RIP_WARNING, "Hook Faulted");
        *pFlags |= HF_HOOKFAULTED;
    }

    return retval;
}
#endif // RECVSIDE

#if DBG
#ifdef SENDSIDE
__inline void CheckPublicDC (LPSTR lpszStr, HDC hdc)
{
    W32PID pid;
    pid = GreGetObjectOwner((HOBJ)hdc, DC_TYPE);
    if(pid == OBJECT_OWNER_PUBLIC) {
        RIPMSG1(RIP_ERROR, lpszStr, hdc);
    }
}
#endif // SENDSIDE
#endif


/**************************************************************************\
* fnOUTDWORDDWORD
*
* 14-Aug-1992 mikeke    created
\**************************************************************************/

typedef struct _FNOUTDWORDDWORDMSG {
    PWND pwnd;
    UINT msg;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNOUTDWORDDWORDMSG;

#ifdef SENDSIDE
SMESSAGECALL(OUTDWORDDWORD)
{
    SETUPPWND(FNOUTDWORDDWORD)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNOUTDWORDDWORD)

        LPDWORD lpdwW = (LPDWORD)wParam;
        LPDWORD lpdwL = (LPDWORD)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNOUTDWORDDWORD);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            try {
                *lpdwW = ProbeAndReadUlong((LPDWORD)pcbs->pOutput);
                *lpdwL = ProbeAndReadUlong((LPDWORD)pcbs->pOutput + 1);
            } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                MSGERROR();
            }
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnOUTDWORDDWORD");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTDWORDDWORD, FNOUTDWORDDWORDMSG)
{
    DWORD adwOut[2];
    BEGINRECV(0, adwOut, sizeof(adwOut));

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            &adwOut[0],
            &adwOut[1],
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnOUTDWORDINDWORD
*
* 04-May-1993 IanJa     created (for MN_FINDMENUWINDOWFROMPOINT)
\**************************************************************************/

typedef struct _FNOUTDWORDINDWORDMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNOUTDWORDINDWORDMSG;

#ifdef SENDSIDE
SMESSAGECALL(OUTDWORDINDWORD)
{
    SETUPPWND(FNOUTDWORDINDWORD)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNOUTDWORDINDWORD)

        LPDWORD lpdwW = (LPDWORD)wParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNOUTDWORDINDWORD);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            try {
                *lpdwW = ProbeAndReadUlong((LPDWORD)pcbs->pOutput);
            } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                MSGERROR();
            }
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnOUTDWORDINDWORD");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTDWORDINDWORD, FNOUTDWORDINDWORDMSG)
{
    DWORD dwOut;
    BEGINRECV(0, &dwOut, sizeof(dwOut));

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            &dwOut,
            CALLDATA(lParam),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnOPTOUTLPDWORDOPTOUTLPDWORD
*
* 25-Nov-1992 JonPa    created
\**************************************************************************/

typedef struct _FNOPTOUTLPDWORDOPTOUTLPDWORDMSG {
    PWND pwnd;
    UINT msg;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNOPTOUTLPDWORDOPTOUTLPDWORDMSG;

#ifdef SENDSIDE
SMESSAGECALL(OPTOUTLPDWORDOPTOUTLPDWORD)
{
    SETUPPWND(FNOPTOUTLPDWORDOPTOUTLPDWORD)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNOPTOUTLPDWORDOPTOUTLPDWORD)

        LPDWORD lpdwW = (LPDWORD)wParam;
        LPDWORD lpdwL = (LPDWORD)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNOPTOUTLPDWORDOPTOUTLPDWORD);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            try {
                if (lpdwW != NULL)
                    *lpdwW = ProbeAndReadUlong((LPDWORD)pcbs->pOutput);
                if (lpdwL != NULL)
                    *lpdwL = ProbeAndReadUlong((LPDWORD)pcbs->pOutput + 1);
            } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                MSGERROR();
            }
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnOPTOUTLPDWORDOPTOUTLPDWORD");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOPTOUTLPDWORDOPTOUTLPDWORD, FNOPTOUTLPDWORDOPTOUTLPDWORDMSG)
{
    DWORD adwOut[2];
    BEGINRECV(0, adwOut, sizeof(adwOut));

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            &adwOut[0],
            &adwOut[1],
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnDWORDOPTINLPMSG
*
* 03-30-92 scottlu      Created
\**************************************************************************/

typedef struct _FNDWORDOPTINLPMSGMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPMSG pmsgstruct;
    ULONG_PTR xParam;
    PROC xpfnProc;
    MSG msgstruct;
} FNDWORDOPTINLPMSGMSG;

#ifdef SENDSIDE
SMESSAGECALL(DWORDOPTINLPMSG)
{
    SETUPPWND(FNDWORDOPTINLPMSG)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNDWORDOPTINLPMSG)

        LPMSG pmsgstruct = (LPMSG)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        COPYSTRUCTOPT(msgstruct);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNDWORDOPTINLPMSG);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnDWORDOPTINLPMSG");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnDWORDOPTINLPMSG, FNDWORDOPTINLPMSGMSG)
{
    BEGINRECV(0, NULL, 0);

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            PCALLDATAOPT(msgstruct),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnCOPYGLOBALDATA
*
* 6-20-92 Sanfords created
\**************************************************************************/

typedef struct _FNCOPYGLOBALDATAMSG {
    CAPTUREBUF CaptureBuf;
    DWORD cbSize;
    PBYTE pData;
} FNCOPYGLOBALDATAMSG;

#ifdef SENDSIDE
SMESSAGECALL(COPYGLOBALDATA)
{
    PBYTE pData = (PBYTE)lParam;

    SETUPPWND(FNCOPYGLOBALDATA)

    UNREFERENCED_PARAMETER(msg);
    UNREFERENCED_PARAMETER(xParam);
    UNREFERENCED_PARAMETER(xpfnProc);
    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSENDCAPTURE(FNCOPYGLOBALDATA, 1, wParam, TRUE)

        if (pData == 0) {
            MSGERROR();
        }

        MSGDATA()->cbSize = (DWORD)wParam;
        LARGECOPYBYTES(pData, (DWORD)wParam);

        LOCKPWND();
        MAKECALLCAPTURE(FNCOPYGLOBALDATA);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnCOPYGLOBALDATA");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnCOPYGLOBALDATA, FNCOPYGLOBALDATAMSG)
{
    PBYTE p;

    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = (ULONG_PTR)GlobalAlloc(GMEM_MOVEABLE, CALLDATA(cbSize));
    if (p = GlobalLock((HANDLE)retval)) {

        memcpy(p, (PVOID)CALLDATA(pData), CALLDATA(cbSize));
        USERGLOBALUNLOCK((HANDLE)retval);

    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnCOPYDATA
*
* 7-14-92 Sanfords created
\**************************************************************************/

typedef struct _FNCOPYDATAMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    HWND hwndFrom;
    BOOL fDataPresent;
    COPYDATASTRUCT cds;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNCOPYDATAMSG;

#ifdef SENDSIDE
SMESSAGECALL(COPYDATA)
{
    HWND hwndFrom = (HWND)wParam;
    PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;
    DWORD cCapture, cbCapture;

    SETUPPWND(FNCOPYDATA)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    if (pcds == NULL) {
        cCapture = cbCapture = 0;
    } else {
        cCapture = 1;
        cbCapture = pcds->cbData;
    }
    BEGINSENDCAPTURE(FNCOPYDATA, cCapture, cbCapture, TRUE);

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->hwndFrom = hwndFrom;
        if (pcds != NULL) {
            MSGDATA()->fDataPresent = TRUE;
            MSGDATA()->cds = *pcds;
            LARGECOPYBYTES2(pcds->lpData, cbCapture, cds.lpData);
        } else {
            MSGDATA()->fDataPresent = FALSE;
        }
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALLCAPTURE(FNCOPYDATA);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnCOPYDATA");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnCOPYDATA, FNCOPYDATAMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = CALLPROC(CALLDATA(xpfnProc))(
        CALLDATA(pwnd),
        CALLDATA(msg),
        CALLDATA(hwndFrom),
        CALLDATA(fDataPresent) ? PCALLDATA(cds) : NULL,
        CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* fnSENTDDEMSG
*
* 11-5-92 Sanfords created
*
*   This thunks DDE messages that SHOULD be posted.  It will only work for
*   WOW apps.  This thunking is strictly for WOW compatability.  No 32 bit
*   app should be allowed to get away with this practice because it opens
*   the DDE protocol up to deadlocks.
\**************************************************************************/

typedef struct _FNSENTDDEMSGMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
    BOOL fIsUnicodeProc;
} FNSENTDDEMSGMSG;

#ifdef SENDSIDE
SMESSAGECALL(SENTDDEMSG)
{
    MSG msgs;

    SETUPPWND(FNSENTDDEMSG)

    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNSENTDDEMSG)

        msg &= ~MSGFLAG_DDE_SPECIAL_SEND;
        if (msg & MSGFLAG_DDE_MID_THUNK) {
            /*
             * complete the thunking here.
             */
            msgs.hwnd = HW(pwnd);
            msgs.message = msg & ~MSGFLAG_DDE_MID_THUNK;
            msgs.wParam = wParam;
            msgs.lParam = lParam;
            xxxDDETrackGetMessageHook((PMSG)&msgs);

            MSGDATA()->pwnd = (PWND)((PBYTE)PW(msgs.hwnd) -
                    pci->ulClientDelta);
            MSGDATA()->msg = msgs.message;
            MSGDATA()->wParam = msgs.wParam;
            MSGDATA()->lParam = msgs.lParam;
        } else {
            MSGDATA()->pwnd = pwndClient;
            MSGDATA()->msg = msg;
            MSGDATA()->wParam = wParam;
            MSGDATA()->lParam = lParam;
        }
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->fIsUnicodeProc = !(dwSCMSFlags & SCMS_FLAGS_ANSI);

        LOCKPWND();
        MAKECALL(FNSENTDDEMSG);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnSENTDDEMSG");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnSENTDDEMSG, FNSENTDDEMSGMSG)
{
    BEGINRECV(0, NULL, 0);

    /*
     * A DDE message may have been sent via CallWindowProc due to subclassing.
     * Since IsWindowUnicode() cannot properly tell what proc a message will
     * ultimately reach, we make sure that the Ansi/Unicode form of any
     * WM_DDE_EXECUTE data is correct for the documented convention and
     * translate it as necessary.
     */
    if (CALLDATA(msg) == WM_DDE_EXECUTE) {
        BOOL fHandleChanged;

        fHandleChanged = FixupDdeExecuteIfNecessary((HGLOBAL *)PCALLDATA(lParam),
                CALLDATA(fIsUnicodeProc) &&
                IsWindowUnicode((HWND)CALLDATA(wParam)));
        /*
         * BUGBUG:
         * If the app didn't allocate this DDE memory GMEM_MOVEABLE,
         * the fixup may require the handle value to change.
         * If this happens things will fall appart when the other side
         * or the tracking layer tries to free the old handle value.
         */
        UserAssert(!fHandleChanged);
    }
    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnDWORD
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNDWORDMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNDWORDMSG;

#ifdef SENDSIDE
SMESSAGECALL(DWORD)
{
    SETUPPWND(FNDWORD)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNDWORD)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNDWORD);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnDWORD");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnDWORD, FNDWORDMSG)
{
    BEGINRECV(0, NULL, 0);

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINWPARAMCHAR
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINWPARAMCHARMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINWPARAMCHARMSG;

#ifdef SENDSIDE
SMESSAGECALL(INWPARAMCHAR)
{
    SETUPPWND(FNINWPARAMCHAR)

    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNINWPARAMCHAR)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;

        /*
         * WM_CHARTOITEM has an index in the hi-word of wParam
         */
        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            if (msg == WM_CHARTOITEM || msg == WM_MENUCHAR) {
                WPARAM dwT = wParam & 0xFFFF;                // mask of caret pos
                RtlWCSMessageWParamCharToMB(msg, &dwT);     // convert key portion
                UserAssert(HIWORD(dwT) == 0);
                wParam = MAKELONG(LOWORD(dwT),HIWORD(wParam));  // rebuild pos & key wParam
            } else {
                RtlWCSMessageWParamCharToMB(msg, &wParam);
            }
        }

        MSGDATA()->wParam = wParam;

        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNDWORD);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINWPARAMCHAR");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
/*
 * The fnDWORD routine is used for this message
 */
#endif // RECVSIDE

/**************************************************************************\
* fnINWPARAMDBCSCHAR
*
* 12-Feb-1996 hideyukn   Created
\**************************************************************************/

typedef struct _FNINWPARAMDBCSCHARMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
    BOOL  bAnsi;
} FNINWPARAMDBCSCHARMSG;

#ifdef SENDSIDE
SMESSAGECALL(INWPARAMDBCSCHAR)
{
    SETUPPWND(FNINWPARAMDBCSCHAR)

    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNINWPARAMDBCSCHAR)

        MSGDATA()->pwnd  = pwndClient;
        MSGDATA()->msg   = msg;
        MSGDATA()->bAnsi = dwSCMSFlags & SCMS_FLAGS_ANSI;

        /*
         * wParam in WM_CHAR/EM_SETPASSWORDCHAR should be converted to ANSI
         * ,if target is ANSI.
         */
        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            RtlWCSMessageWParamCharToMB(msg, &wParam);
        }

        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINWPARAMDBCSCHAR);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINWPARAMDBCSCHAR");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINWPARAMDBCSCHAR, FNINWPARAMDBCSCHARMSG)
{
    BOOL bAnsiWndProc;

    BEGINRECV(0,NULL,0);

        bAnsiWndProc = CALLDATA(bAnsi);

        if (bAnsiWndProc) {

            PKERNEL_MSG  pmsgDbcsCB = GetCallBackDbcsInfo();
            WPARAM wParam         = pmsg->wParam;
            BOOL  bDbcsMessaging = FALSE;

            //
            // Check wParam has Dbcs character or not..
            //
            if (IS_DBCS_MESSAGE(pmsg->wParam)) {

                if (pmsg->wParam & WMCR_IR_DBCSCHAR) {

                    //
                    // This is reply for WM_IME_REPORT:IR_DBCSCHAR, then
                    // We send DBCS chararcter at one time...
                    // (Do not need to send twice for DBCS LeadByte and TrailByte).
                    //
                    // Validation for wParam.. (mask off the secret bit).
                    //
                    wParam = (pmsg->wParam & 0x0000FFFF);

                } else {

                    //
                    // Mark the wParam keeps Dbcs character..
                    //
                    bDbcsMessaging = TRUE;

                    //
                    // Backup current message. this backupped message will be used
                    // when Apps peek (or get) message from thier WndProc.
                    // (see GetMessageA(), PeekMessageA()...)
                    //
                    // pmsgDbcsCB->hwnd    = HW(pmsg->pwnd);
                    // pmsgDbcsCB->message = pmsg->msg;
                    // pmsgDbcsCB->wParam  = pmsg->wParam;
                    // pmsgDbcsCB->lParam  = pmsg->lParam;
                    // pmsgDbcsCB->time    = pmsg->time;
                    // pmsgDbcsCB->pt      = pmsg->pt;
                    //
                    COPY_MSG_TO_KERNELMSG(pmsgDbcsCB,(PMSG)pmsg);

                    //
                    // pwnd should be converted to hwnd.
                    //
                    pmsgDbcsCB->hwnd = HW(pmsg->pwnd);

                    //
                    // DbcsLeadByte will be sent below soon, we just need DbcsTrailByte
                    // for further usage..
                    //
                    pmsgDbcsCB->wParam = (pmsg->wParam & 0x000000FF);

                    //
                    // Pass the LeadingByte of the DBCS character to an ANSI WndProc.
                    //
                    wParam = (pmsg->wParam & 0x0000FF00) >> 8;
                }
            }

            //
            // Forward Dbcs LeadingByte or Sbcs character to Apps WndProc.
            //
            retval = CALLPROC(CALLDATA(xpfnProc))(
                    CALLDATA(pwnd),
                    CALLDATA(msg),
                    wParam,
                    CALLDATA(lParam),
                    CALLDATA(xParam) );

            //
            // Check we need to send trailing byte or not, if the wParam has Dbcs character.
            //
            if (bDbcsMessaging && pmsgDbcsCB->wParam) {

                //
                // If an app didn't peek (or get) the trailing byte from within
                // WndProc, and then pass the DBCS TrailingByte to the ANSI WndProc here
                // pmsgDbcsCB->wParam has DBCS TrailingByte here.. see above..
                //
                wParam = KERNEL_WPARAM_TO_WPARAM(pmsgDbcsCB->wParam);

                //
                // Invalidate cached message.
                //
                pmsgDbcsCB->wParam = 0;

                retval = CALLPROC(CALLDATA(xpfnProc))(
                        CALLDATA(pwnd),
                        CALLDATA(msg),
                        wParam,
                        CALLDATA(lParam),
                        CALLDATA(xParam) );
            } else {

                //
                // If an app called Get/PeekMessageA from its
                // WndProc, do not do anything.
                //
            }

        } else {

            //
            // Only LOWORD of WPARAM is valid for WM_CHAR....
            //  (Mask off DBCS messaging information.)
            //
            pmsg->wParam &= 0x0000FFFF;

            retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
                    pmsg->pwnd,
                    pmsg->msg,
                    pmsg->wParam,
                    pmsg->lParam,
                    pmsg->xParam);
        }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINOUTDRAGMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
    DROPSTRUCT ds;
} FNINOUTDRAGMSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTDRAG)
{
    SETUPPWND(FNINOUTDRAG)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNINOUTDRAG)

        LPDROPSTRUCT pds = (LPDROPSTRUCT)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->ds = *pds;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINOUTDRAG);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            OUTSTRUCT(pds, DROPSTRUCT);
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnINOUTDRAG");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTDRAG, FNINOUTDRAGMSG)
{
    BEGINRECV(0, &pmsg->ds, sizeof(pmsg->ds));

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            &pmsg->ds,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnGETTEXTLENGTHS
*
* Gets the Unicode & ANSI lengths
* Internally, lParam pints to the ANSI length in bytes and the return value
* is the Unicode length in bytes.  However, the public definition is maintained
* on the  client side, where lParam is not used and either ANSI or Unicode is
* returned.
*
* 10-Feb-1992 IanJa    Created
\**************************************************************************/

typedef struct _FNGETTEXTLENGTHSMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNGETTEXTLENGTHSMSG;

#ifdef SENDSIDE
SMESSAGECALL(GETTEXTLENGTHS)
{
    SETUPPWND(FNGETTEXTLENGTHS)

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNGETTEXTLENGTHS)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNGETTEXTLENGTHS);
        UNLOCKPWND();
        CHECKRETURN();

        /*
         * ANSI client wndproc returns us cbANSI.  We want cchUnicode,
         * so we guess cchUnicode = cbANSI. (It may be less if
         * multi-byte characters are involved, but it will never be more).
         * Save cbANSI in *lParam in case the server ultimately returns
         * the length to an ANSI caller.
         *
         * Unicode client wndproc returns us cchUnicode.  If we want to know
         * cbANSI, we must guess how many 'ANSI' chars we would need.
         * We guess cbANSI = cchUnicode * 2. (It may be this much if all
         * 'ANSI' characters are multi-byte, but it will never be more).
         *
         * Return cchUnicode (server code is all Unicode internally).
         * Put cbANSI in *lParam to be passed along within the server in case
         * we ultimately need to return it to the client.
         *
         * NOTE: this will sometimes cause text lengths to be misreported
         * up to twice the real length, but that is expected to be harmless.
         * This will only * happen if an app sends WM_GETcode TEXTLENGTH to a
         * window with an ANSI client-side wndproc, or a ANSI WM_GETTEXTLENGTH
         * is sent to a Unicode client-side wndproc.
         */

    TRACECALLBACKMSG("SfnGETTEXTLENGTHS");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnGETTEXTLENGTHS, FNGETTEXTLENGTHSMSG)
{
    BEGINRECV(0, NULL, 0);

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            0,                      // so we don't pass &cbAnsi to apps
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPCREATESTRUCTMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    CREATESTRUCT cs;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINLPCREATESTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPCREATESTRUCT)
{
    PCREATESTRUCTEX pcreatestruct = (PCREATESTRUCTEX)lParam;
    DWORD cbName = 0, cbClass = 0;
    DWORD cCapture = 0;
    BOOL fAnsiReceiver = (dwSCMSFlags & SCMS_FLAGS_ANSI);

    SETUPPWND(FNINLPCREATESTRUCT)

    UNREFERENCED_PARAMETER(psms);

    /*
     * Compute ANSI capture lengths.  Don't capture if
     * the strings are in the client's address space.
     */
    if (pcreatestruct) {
        if (pcreatestruct->cs.lpszName &&
                ((BOOL)pcreatestruct->strName.bAnsi != fAnsiReceiver ||
                IS_SYSTEM_ADDRESS((PVOID)pcreatestruct->cs.lpszName))) {
            CALC_SIZE_IN(cbName, &pcreatestruct->strName);
            cCapture++;
        }
        if (IS_PTR(pcreatestruct->cs.lpszClass) &&
                ((BOOL)pcreatestruct->strClass.bAnsi != fAnsiReceiver ||
                IS_SYSTEM_ADDRESS((PVOID)pcreatestruct->cs.lpszClass))) {
            CALC_SIZE_IN(cbClass, &pcreatestruct->strClass);
            cCapture++;
        }
    }

    BEGINSENDCAPTURE(FNINLPCREATESTRUCT, cCapture, cbName + cbClass, TRUE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;  // this could be NULL in WOW apps!

        if (pcreatestruct != NULL) {
            MSGDATA()->cs = pcreatestruct->cs;

            // Make it a "Large" copy because it could be an Edit control
            if (cbName) {
                if (!pcreatestruct->strName.bAnsi) {
                    if (*(PWORD)pcreatestruct->cs.lpszName == 0xffff) {

                        /*
                         * Copy out an ordinal of the form 0xffff, ID.
                         * If the receiver is ANSI, skip the first 0xff.
                         */
                        if (fAnsiReceiver) {
                            if (!NT_SUCCESS(CaptureCallbackData(&mp->CaptureBuf,
                                    (PBYTE)pcreatestruct->cs.lpszName + 1,
                                    3, (PVOID *)&mp->cs.lpszName)))
                                goto errorexit;
                        } else {
                            if (!NT_SUCCESS(CaptureCallbackData(&mp->CaptureBuf,
                                    (PBYTE)pcreatestruct->cs.lpszName,
                                    4, (PVOID *)&mp->cs.lpszName)))
                                goto errorexit;
                        }
                    } else if (fAnsiReceiver) {
                        LARGECOPYSTRINGLPWSTRA(&pcreatestruct->strName, cs.lpszName);
                    } else {
                        LARGECOPYSTRINGLPWSTR(&pcreatestruct->strName, cs.lpszName);
                    }
                } else {
                    if (*(PBYTE)pcreatestruct->cs.lpszName == 0xff) {

                        /*
                         * Copy out an ordinal of the form 0xff, ID.
                         * If the receiver is UNICODE, expand the 0xff to 0xffff.
                         */
                        if (fAnsiReceiver) {
                            if (!NT_SUCCESS(CaptureCallbackData(&mp->CaptureBuf,
                                    (PBYTE)pcreatestruct->cs.lpszName,
                                    3, (PVOID *)&mp->cs.lpszName)))
                                goto errorexit;
                        } else {
                            DWORD dwOrdinal;

                            dwOrdinal = MAKELONG(0xffff,
                                    (*(DWORD UNALIGNED *)pcreatestruct->cs.lpszName >> 8));
                            if (!NT_SUCCESS(CaptureCallbackData(&mp->CaptureBuf,
                                    &dwOrdinal,
                                    4, (PVOID *)&mp->cs.lpszName)))
                                goto errorexit;
                        }
                    } else if (fAnsiReceiver) {
                        LARGECOPYSTRINGLPSTR(&pcreatestruct->strName, cs.lpszName);
                    } else {
                        LARGECOPYSTRINGLPSTRW(&pcreatestruct->strName, cs.lpszName);
                    }
                }
            }
            if (cbClass) {
                if (!pcreatestruct->strClass.bAnsi) {
                    if (fAnsiReceiver) {
                        LARGECOPYSTRINGLPWSTRA(&pcreatestruct->strClass, cs.lpszClass);
                    } else {
                        LARGECOPYSTRINGLPWSTR(&pcreatestruct->strClass, cs.lpszClass);
                    }
                } else {
                    if (fAnsiReceiver) {
                        LARGECOPYSTRINGLPSTR(&pcreatestruct->strClass, cs.lpszClass);
                    } else {
                        LARGECOPYSTRINGLPSTRW(&pcreatestruct->strClass, cs.lpszClass);
                    }
                }
            }
        }

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALLCAPTURE(FNINLPCREATESTRUCT);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINLPCREATESTRUCT");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPCREATESTRUCT, FNINLPCREATESTRUCTMSG)
{
    LPARAM lParam;

    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    if (pmsg->lParam != 0) {
        if ((ULONG_PTR)pmsg->cs.lpszName > gHighestUserAddress)
            pmsg->cs.lpszName = REBASEPTR(pmsg->pwnd, pmsg->cs.lpszName);
        if ((ULONG_PTR)pmsg->cs.lpszClass > gHighestUserAddress)
            pmsg->cs.lpszClass = REBASEPTR(pmsg->pwnd, pmsg->cs.lpszClass);
        lParam = (LPARAM)&pmsg->cs;

        if ((pmsg->cs.lpCreateParams != NULL) &&
            (TestWF(pmsg->pwnd, WEFMDICHILD))) {
               // Note -- do not test the flag in cs.dwExStyle -- it gets zapped for Old UI apps, like Quicken
            ((LPMDICREATESTRUCT)(pmsg->cs.lpCreateParams))->szClass = pmsg->cs.lpszClass;
            ((LPMDICREATESTRUCT)(pmsg->cs.lpCreateParams))->szTitle = pmsg->cs.lpszName;
        }
    } else
        lParam = 0;


    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            lParam,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINLPMDICREATESTRUCT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPMDICREATESTRUCTMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    MDICREATESTRUCT mdics;
    ULONG_PTR xParam;
    PROC xpfnProc;
    int szClass;
    int szTitle;
} FNINLPMDICREATESTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPMDICREATESTRUCT)
{
    PMDICREATESTRUCTEX pmdicreatestruct = (PMDICREATESTRUCTEX)lParam;
    DWORD cbTitle = 0, cbClass = 0;
    DWORD cCapture = 0;
    BOOL fAnsiReceiver = (dwSCMSFlags & SCMS_FLAGS_ANSI);

    SETUPPWND(FNINLPMDICREATESTRUCT)

    UNREFERENCED_PARAMETER(psms);

    /*
     * Compute ANSI capture lengths.  Don't capture if
     * the strings are in the client's address space and
     * are Unicode.
     */
    if (pmdicreatestruct->mdics.szTitle &&
            (IS_SYSTEM_ADDRESS((PVOID)pmdicreatestruct->mdics.szTitle) ||
            ((BOOL)pmdicreatestruct->strTitle.bAnsi != fAnsiReceiver))) {
        CALC_SIZE_IN(cbTitle, &pmdicreatestruct->strTitle);
        cCapture = 1;
    }
    if (IS_PTR(pmdicreatestruct->mdics.szClass) &&
            (IS_SYSTEM_ADDRESS((PVOID)pmdicreatestruct->mdics.szClass) ||
            ((BOOL)pmdicreatestruct->strClass.bAnsi != fAnsiReceiver))) {
        CALC_SIZE_IN(cbClass, &pmdicreatestruct->strClass);
        cCapture++;
    }

    BEGINSENDCAPTURE(FNINLPMDICREATESTRUCT, cCapture, cbTitle + cbClass, TRUE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->mdics = pmdicreatestruct->mdics;

        if (cbTitle) {
            if (!pmdicreatestruct->strTitle.bAnsi) {
                if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPWSTRA(&pmdicreatestruct->strTitle, mdics.szTitle);
                } else {
                    LARGECOPYSTRINGLPWSTR(&pmdicreatestruct->strTitle, mdics.szTitle);
                }
            } else {
                if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPSTR(&pmdicreatestruct->strTitle, mdics.szTitle);
                } else {
                    LARGECOPYSTRINGLPSTRW(&pmdicreatestruct->strTitle, mdics.szTitle);
                }
            }
        }
        if (cbClass) {
            if (!pmdicreatestruct->strClass.bAnsi) {
                if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPWSTRA(&pmdicreatestruct->strClass, mdics.szClass);
                } else {
                    LARGECOPYSTRINGLPWSTR(&pmdicreatestruct->strClass, mdics.szClass);
                }
            } else {
                if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPSTR(&pmdicreatestruct->strClass, mdics.szClass);
                } else {
                    LARGECOPYSTRINGLPSTRW(&pmdicreatestruct->strClass, mdics.szClass);
                }
            }
        }
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALLCAPTURE(FNINLPMDICREATESTRUCT);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINLPMDICREATESTRUCT");
    ENDSENDCAPTURE(DWORD,0);
    DBG_UNREFERENCED_PARAMETER(wParam);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPMDICREATESTRUCT, FNINLPMDICREATESTRUCTMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            &pmsg->mdics,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINPAINTCLIPBRD
*
* lParam is a supposed to be a Global Handle to DDESHARE memory.
*
* 22-Jul-1991 johnc     Created
\**************************************************************************/

typedef struct _FNINPAINTCLIPBRDMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    PAINTSTRUCT ps;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINPAINTCLIPBRDMSG;

#ifdef SENDSIDE
SMESSAGECALL(INPAINTCLIPBRD)
{
    PWND pwndDCOwner;

    /*
     * We need to check clipboard access rights because the app could
     * get the clipboard owner's window handle by enumeration etc and
     * send this message
     */

    SETUPPWND(FNINPAINTCLIPBRD)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNINPAINTCLIPBRD)

        LPPAINTSTRUCT pps = (LPPAINTSTRUCT)lParam;

        if (RtlAreAllAccessesGranted(PpiCurrent()->amwinsta,
                WINSTA_ACCESSCLIPBOARD)) {

            MSGDATA()->pwnd = pwndClient;
            MSGDATA()->msg = msg;
            MSGDATA()->wParam = wParam;
            MSGDATA()->ps = *pps;
            MSGDATA()->xParam = xParam;
            MSGDATA()->xpfnProc = xpfnProc;

            /*
             * We can't just set the owner of the DC and pass the original DC
             * because currently GDI won't let you query the current owner
             * and we don't know if it is a public or privately owned DC
             */
            pwndDCOwner = _WindowFromDC(pps->hdc);
            MSGDATA()->ps.hdc = _GetDC(pwndDCOwner);

            LOCKPWND();
            MAKECALL(FNINPAINTCLIPBRD);
            UNLOCKPWND();
            CHECKRETURN();

            _ReleaseDC(MSGDATA()->ps.hdc);
        }

    TRACECALLBACKMSG("SfnINPAINTCLIPBRD");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINPAINTCLIPBRD, FNINPAINTCLIPBRDMSG)
{
    LPPAINTSTRUCT lpps;

    BEGINRECV(0, NULL, 0);

    lpps = (LPPAINTSTRUCT)GlobalAlloc(GMEM_FIXED | GMEM_DDESHARE, sizeof(PAINTSTRUCT));
    UserAssert(lpps);

    if (lpps) {
        *lpps = pmsg->ps;

        UserAssert(lpps->hdc);

        retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
                pmsg->pwnd,
                pmsg->msg,
                pmsg->wParam,
                lpps,
                pmsg->xParam);

        GlobalFree((HGLOBAL)lpps);
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINSIZECLIPBRD
*
* lParam is a supposed to be a Global Handle to DDESHARE memory.
*
* 11-Jun-1992 sanfords  Created
\**************************************************************************/

typedef struct _FNINSIZECLIPBRDMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    RECT rc;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINSIZECLIPBRDMSG;

#ifdef SENDSIDE
SMESSAGECALL(INSIZECLIPBRD)
{
    /*
     * We need to check clipboard access rights because the app could
     * get the clipboard owner's window handle by enumeration etc and
     * send this message
     */

    SETUPPWND(FNINSIZECLIPBRD)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNINSIZECLIPBRD)

        LPRECT prc = (LPRECT)lParam;

        if (RtlAreAllAccessesGranted(PpiCurrent()->amwinsta,
                WINSTA_ACCESSCLIPBOARD)) {

            MSGDATA()->pwnd = pwndClient;
            MSGDATA()->msg = msg;
            MSGDATA()->wParam = wParam;
            MSGDATA()->rc = *prc;
            MSGDATA()->xParam = xParam;
            MSGDATA()->xpfnProc = xpfnProc;

            LOCKPWND();
            MAKECALL(FNINSIZECLIPBRD);
            UNLOCKPWND();
            CHECKRETURN();
        }

    TRACECALLBACKMSG("SfnINSIZECLIPBRD");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINSIZECLIPBRD, FNINSIZECLIPBRDMSG)
{
    LPRECT lprc;

    BEGINRECV(0, NULL, 0);

    lprc = (LPRECT)GlobalAlloc(GMEM_FIXED | GMEM_DDESHARE, sizeof(RECT));
    UserAssert(lprc);

    if (lprc) {
        *lprc = pmsg->rc;

        retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
                pmsg->pwnd,
                pmsg->msg,
                pmsg->wParam,
                lprc,
                pmsg->xParam);

        GlobalFree((HGLOBAL)lprc);
    }

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* fnINDESTROYCLIPBRD
*
* Special handler so we can call ClientEmptyClipboard on client
*
* 01-16-93 scottlu  Created
\**************************************************************************/

typedef struct _FNINDESTROYCLIPBRDMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINDESTROYCLIPBRDMSG;

#ifdef SENDSIDE
SMESSAGECALL(INDESTROYCLIPBRD)
{
    SETUPPWND(FNINDESTROYCLIPBRD)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNINDESTROYCLIPBRD)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINDESTROYCLIPBRD);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINDESTROYCLIPBRD");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINDESTROYCLIPBRD, FNINDESTROYCLIPBRDMSG)
{
    void ClientEmptyClipboard(void);

    BEGINRECV(0, NULL, 0);

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            pmsg->lParam,
            pmsg->xParam);

    /*
     * Now empty the client side clipboard cache.
     * Don't do this if this is a 16bit app.  We don't want to clear out the
     * clipboard just because one app is going away.  All of the 16bit apps
     * share one clipboard.
     */
    if ((GetClientInfo()->CI_flags & CI_16BIT) == 0) {
        ClientEmptyClipboard();
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINOUTLPSCROLLINFOMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    SCROLLINFO info;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINOUTLPSCROLLINFOMSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTLPSCROLLINFO)
{
    SETUPPWND(FNINOUTLPSCROLLINFO)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNINOUTLPSCROLLINFO)

        LPSCROLLINFO pinfo = (LPSCROLLINFO)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->info = *pinfo;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINOUTLPSCROLLINFO);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            OUTSTRUCT(pinfo, SCROLLINFO);
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnINOUTLPSCROLLINFO");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTLPSCROLLINFO, FNINOUTLPSCROLLINFOMSG)
{
    BEGINRECV(0, &pmsg->info, sizeof(pmsg->info));

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            &pmsg->info,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINOUTLPPOINT5MSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    POINT5 point5;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINOUTLPPOINT5MSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTLPPOINT5)
{
    SETUPPWND(FNINOUTLPPOINT5)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNINOUTLPPOINT5)

        LPPOINT5 ppoint5 = (LPPOINT5)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->point5 = *ppoint5;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINOUTLPPOINT5);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
             OUTSTRUCT(ppoint5, POINT5);
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnINOUTLPPOINT5");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTLPPOINT5, FNINOUTLPPOINT5MSG)
{
    BEGINRECV(0, &pmsg->point5, sizeof(POINT5));

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            &pmsg->point5,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINOUTLPRECTMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    RECT rect;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINOUTLPRECTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTLPRECT)
{
    SETUPPWND(FNINOUTLPRECT)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNINOUTLPRECT)

        LPRECT prect = (LPRECT)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->rect = *prect;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINOUTLPRECT);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            OUTSTRUCT(prect, RECT);
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnINOUTLPRECT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTLPRECT, FNINOUTLPRECTMSG)
{
    BEGINRECV(0, &pmsg->rect, sizeof(pmsg->rect));

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            &pmsg->rect,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 11-25-92 ScottLu      Created.
\**************************************************************************/

typedef struct _FNINOUTNCCALCSIZEMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
    union {
        RECT rc;
        struct {
            NCCALCSIZE_PARAMS params;
            WINDOWPOS pos;
        } p;
    } u;
} FNINOUTNCCALCSIZEMSG;

typedef struct _OUTNCCALCSIZE {
    NCCALCSIZE_PARAMS params;
    WINDOWPOS pos;
} OUTNCCALCSIZE, *POUTNCCALCSIZE;

#ifdef SENDSIDE
SMESSAGECALL(INOUTNCCALCSIZE)
{
    SETUPPWND(FNINOUTNCCALCSIZE)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNINOUTNCCALCSIZE)

        LPWINDOWPOS lppos;
        UINT cbCallback;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        /*
         * If wParam != 0, lParam points to a NCCALCSIZE_PARAMS structure,
         * otherwise it points to a rectangle.
         */
        if (wParam != 0) {
            MSGDATA()->u.p.params = *((LPNCCALCSIZE_PARAMS)lParam);
            MSGDATA()->u.p.pos = *(MSGDATA()->u.p.params.lppos);
            cbCallback = sizeof(FNINOUTNCCALCSIZEMSG);
        } else {
            MSGDATA()->u.rc = *((LPRECT)lParam);
            cbCallback = FIELD_OFFSET(FNINOUTNCCALCSIZEMSG, u) +
                    sizeof(RECT);
        }

        /*
         * Don't use the MAKECALL macro so we can
         * select the callback data size
         */
        LOCKPWND();
        LeaveCrit();
        Status = (DWORD)KeUserModeCallback(
            FI_FNINOUTNCCALCSIZE,
            mp,
            cbCallback,
            &pcbs,
            &cbCBStatus);
        EnterCrit();
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            try {
                ProbeForRead(pcbs->pOutput, pcbs->cbOutput, sizeof(DWORD));
                if (wParam != 0) {
                    lppos = ((LPNCCALCSIZE_PARAMS)lParam)->lppos;
                    *((LPNCCALCSIZE_PARAMS)lParam) =
                            ((POUTNCCALCSIZE)pcbs->pOutput)->params;
                    *lppos = ((POUTNCCALCSIZE)pcbs->pOutput)->pos;
                    ((LPNCCALCSIZE_PARAMS)lParam)->lppos = lppos;
                } else {
                    *((LPRECT)lParam) = *(PRECT)pcbs->pOutput;
                }
            } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                MSGERROR();
            }
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnINOUTNCCALCSIZE");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTNCCALCSIZE, FNINOUTNCCALCSIZEMSG)
{
    BEGINRECV(0, &pmsg->u, sizeof(pmsg->u));

    if (CALLDATA(wParam) != 0)
        CALLDATA(u.p.params).lppos = PCALLDATA(u.p.pos);

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            (LPARAM)&pmsg->u,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 9/30/94 Sanfords created
\**************************************************************************/

typedef struct _FNINOUTSTYLECHANGEMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    STYLESTRUCT ss;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINOUTSTYLECHANGEMSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTSTYLECHANGE)
{
    SETUPPWND(FNINOUTSTYLECHANGE)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNINOUTSTYLECHANGE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->ss = *((LPSTYLESTRUCT)lParam);

        LOCKPWND();
        MAKECALL(FNINOUTSTYLECHANGE);
        UNLOCKPWND();
        CHECKRETURN();

        if (msg == WM_STYLECHANGING)
            OUTSTRUCT(((LPSTYLESTRUCT)lParam), STYLESTRUCT);

    TRACECALLBACKMSG("SfnINOUTSTYLECHANGE");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTSTYLECHANGE, FNINOUTSTYLECHANGEMSG)
{
    BEGINRECV(0, &pmsg->ss, sizeof(pmsg->ss));

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPARAM)&pmsg->ss,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNOUTLPRECTMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNOUTLPRECTMSG;

#ifdef SENDSIDE
SMESSAGECALL(OUTLPRECT)
{
    SETUPPWND(FNOUTLPRECT)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNOUTLPRECT)

        LPRECT prect = (LPRECT)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNOUTLPRECT);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            OUTSTRUCT(prect, RECT);
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnOUTLPRECT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTLPRECT, FNOUTLPRECTMSG)
{
    RECT rc;

    BEGINRECV(0, &rc, sizeof(rc));

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            &rc,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPCOMPAREITEMSTRUCTMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    COMPAREITEMSTRUCT compareitemstruct;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINLPCOMPAREITEMSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPCOMPAREITEMSTRUCT)
{
    SETUPPWND(FNINLPCOMPAREITEMSTRUCT)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNINLPCOMPAREITEMSTRUCT)

        LPCOMPAREITEMSTRUCT pcompareitemstruct = (LPCOMPAREITEMSTRUCT)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->compareitemstruct = *pcompareitemstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINLPCOMPAREITEMSTRUCT);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINLPCOMPAREITEMSTRUCT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPCOMPAREITEMSTRUCT, FNINLPCOMPAREITEMSTRUCTMSG)
{
    BEGINRECV(0, NULL, 0);

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            &(pmsg->compareitemstruct),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPDELETEITEMSTRUCTMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    DELETEITEMSTRUCT deleteitemstruct;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINLPDELETEITEMSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPDELETEITEMSTRUCT)
{
    SETUPPWND(FNINLPDELETEITEMSTRUCT)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNINLPDELETEITEMSTRUCT)

        LPDELETEITEMSTRUCT pdeleteitemstruct = (LPDELETEITEMSTRUCT)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->deleteitemstruct = *pdeleteitemstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINLPDELETEITEMSTRUCT);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINLPDELETEITEMSTRUCT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPDELETEITEMSTRUCT, FNINLPDELETEITEMSTRUCTMSG)
{
    BEGINRECV(0, NULL, 0);

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            &(pmsg->deleteitemstruct),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* FNINHLPSTRUCT
*
* 06-08-92 SanfordS Created
\**************************************************************************/

typedef struct _FNINLPHLPSTRUCTMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPHLP lphlp;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINLPHLPSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPHLPSTRUCT)
{
    LPHLP lphlp = (LPHLP)lParam;

    SETUPPWND(FNINLPHLPSTRUCT)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSENDCAPTURE(FNINLPHLPSTRUCT, 1, lphlp->cbData, TRUE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        COPYBYTES(lphlp, lphlp->cbData);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALLCAPTURE(FNINLPHLPSTRUCT);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINLPHLPSTRUCT");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPHLPSTRUCT, FNINLPHLPSTRUCTMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            FIXUP(lphlp),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

#ifndef WINHELP4

/**************************************************************************\
* FNINHELPINFOSTRUCT
*
* 06-08-92 SanfordS Created
\**************************************************************************/

typedef struct _FNINLPHELPFINFOSTRUCTMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPHELPINFO lphlp;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINLPHELPINFOSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPHELPINFOSTRUCT)
{
    LPHELPINFO lphlp = (LPHELPINFO)lParam;

    SETUPPWND(FNINLPHELPINFOSTRUCT)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSENDCAPTURE(FNINLPHELPINFOSTRUCT, 1, lphlp->cbSize, TRUE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        COPYBYTES(lphlp, lphlp->cbSize);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALLCAPTURE(FNINLPHELPINFOSTRUCT);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINLPHELPINFOSTRUCT");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPHELPINFOSTRUCT, FNINLPHELPINFOSTRUCTMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            FIXUP(lphlp),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE
#endif // WINHELP4

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPDRAWITEMSTRUCTMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    DRAWITEMSTRUCT drawitemstruct;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINLPDRAWITEMSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPDRAWITEMSTRUCT)
{
    SETUPPWND(FNINLPDRAWITEMSTRUCT)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNINLPDRAWITEMSTRUCT)

        LPDRAWITEMSTRUCT pdrawitemstruct = (LPDRAWITEMSTRUCT)lParam;
        HDC hdcOriginal = (HDC)NULL;

        /*
         * Make sure that this is not an OLE inter-process DrawItem
         */
        if (GreGetObjectOwner((HOBJ)pdrawitemstruct->hDC, DC_TYPE) !=
                W32GetCurrentPID()) {
            if (pdrawitemstruct->hDC) {
                PWND pwndItem;

                pwndItem = _WindowFromDC(pdrawitemstruct->hDC);

                if (pwndItem) {
                    hdcOriginal = pdrawitemstruct->hDC;
                    pdrawitemstruct->hDC = _GetDC(pwndItem);
                }
            }
        }


        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->drawitemstruct = *pdrawitemstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINLPDRAWITEMSTRUCT);
        UNLOCKPWND();
        CHECKRETURN();

        if (hdcOriginal) {
            _ReleaseDC(pdrawitemstruct->hDC);
            pdrawitemstruct->hDC = hdcOriginal;
        }
    TRACECALLBACKMSG("SfnINLPDRAWITEMSTRUCT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPDRAWITEMSTRUCT, FNINLPDRAWITEMSTRUCTMSG)
{
    BEGINRECV(0, NULL, 0);

    if (pmsg->drawitemstruct.hDC == NULL)
        MSGERRORCODE(ERROR_INVALID_HANDLE);

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            &(pmsg->drawitemstruct),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINOUTLPMEASUREITEMSTRUCT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINOUTLPMEASUREITEMSTRUCTMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    MEASUREITEMSTRUCT measureitemstruct;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINOUTLPMEASUREITEMSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTLPMEASUREITEMSTRUCT)
{
    SETUPPWND(FNINOUTLPMEASUREITEMSTRUCT)

    BEGINSEND(FNINOUTLPMEASUREITEMSTRUCT)

        PMEASUREITEMSTRUCT pmeasureitemstruct = (PMEASUREITEMSTRUCT)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg & ~MSGFLAG_MASK;
        MSGDATA()->wParam = wParam;
        MSGDATA()->measureitemstruct = *pmeasureitemstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINOUTLPMEASUREITEMSTRUCT);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            OUTSTRUCT(pmeasureitemstruct, MEASUREITEMSTRUCT);
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnINOUTLPMEASUREITEMSTRUCT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTLPMEASUREITEMSTRUCT, FNINOUTLPMEASUREITEMSTRUCTMSG)
{
    BEGINRECV(0, &pmsg->measureitemstruct, sizeof(pmsg->measureitemstruct));

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            &pmsg->measureitemstruct,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINSTRING
*
* 22-Jul-1991 mikeke    Created
* 27-Jan-1992 IanJa     Unicode/ANSI
\**************************************************************************/

typedef struct _FNINSTRINGMSG {
    CAPTUREBUF CaptureBuf;
    PWND       pwnd;
    UINT       msg;
    WPARAM     wParam;
    ULONG_PTR   xParam;
    PROC       xpfnProc;
    LPTSTR     pwsz;
} FNINSTRINGMSG;

#ifdef SENDSIDE
SMESSAGECALL(INSTRING)
{
    PLARGE_STRING pstr = (PLARGE_STRING)lParam;
    DWORD         cbCapture;
    DWORD         cCapture;
    BOOL          fAnsiReceiver = (dwSCMSFlags & SCMS_FLAGS_ANSI);

    SETUPPWND(FNINSTRING)

    UNREFERENCED_PARAMETER(psms);

    /*
     * Compute ANSI capture lengths.  Don't capture if
     * the strings are in the client's address space and
     * of the correct type.
     */
    if (pstr &&
        (IS_SYSTEM_ADDRESS((PVOID)pstr->Buffer) ||
        ((BOOL)pstr->bAnsi != fAnsiReceiver))) {

        cCapture = 1;
        CALC_SIZE_IN(cbCapture, pstr);

    } else {

        cbCapture = 0;
        cCapture  = 0;
    }

    BEGINSENDCAPTURE(FNINSTRING, cCapture, cbCapture, TRUE)

        MSGDATA()->pwnd   = pwndClient;
        MSGDATA()->msg    = msg;
        MSGDATA()->wParam = wParam;

        if (cCapture) {

            if (!pstr->bAnsi) {

                if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPWSTRA(pstr, pwsz);
                } else {
                    LARGECOPYSTRINGLPWSTR(pstr, pwsz);
                }

            } else {

                if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPSTR(pstr, pwsz);
                } else {
                    LARGECOPYSTRINGLPSTRW(pstr, pwsz);
                }
            }

        } else {

            MSGDATA()->pwsz = (pstr ? pstr->Buffer : NULL);
        }

        MSGDATA()->xParam   = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALLCAPTURE(FNINSTRING);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINSTRING");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINSTRING, FNINSTRINGMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            pmsg->pwsz,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINSTRINGNULL
*
* Server-side stub translates Unicode to ANSI if required.
*
* 22-Jul-1991 mikeke    Created
* 28-Jan-1992 IanJa     Unicode/ANSI  (Server translate to ANSI if rquired)
\**************************************************************************/

typedef struct _FNINSTRINGNULLMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
    LPTSTR pwsz;
} FNINSTRINGNULLMSG;

#ifdef SENDSIDE
SMESSAGECALL(INSTRINGNULL)
{
    PLARGE_STRING pstr = (PLARGE_STRING)lParam;
    DWORD cbCapture;
    DWORD cCapture;
    BOOL fAnsiReceiver = (dwSCMSFlags & SCMS_FLAGS_ANSI);

    SETUPPWND(FNINSTRINGNULL)

    UNREFERENCED_PARAMETER(psms);

    cCapture = 0;
    cbCapture = 0;
    if (pstr) {

        /*
         * Compute ANSI capture lengths.  Don't capture if
         * the strings are in the client's address space and
         * of the correct type.
         */
        if (IS_SYSTEM_ADDRESS((PVOID)pstr->Buffer) ||
                (BOOL)pstr->bAnsi != fAnsiReceiver) {
            cCapture = 1;
            CALC_SIZE_IN(cbCapture, pstr);
        }
    }

    BEGINSENDCAPTURE(FNINSTRINGNULL, cCapture, cbCapture, TRUE)


        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        if (cCapture) {
            if (!pstr->bAnsi) {
                if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPWSTRA(pstr, pwsz);
                } else {
                    LARGECOPYSTRINGLPWSTR(pstr, pwsz);
                }
            } else {
                if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPSTR(pstr, pwsz);
                } else {
                    LARGECOPYSTRINGLPSTRW(pstr, pwsz);
                }
            }
        } else
            MSGDATA()->pwsz = pstr ? pstr->Buffer : NULL;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALLCAPTURE(FNINSTRINGNULL);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINSTRINGNULL");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINSTRINGNULL, FNINSTRINGNULLMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            pmsg->pwsz,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 27-May-1997 GregoryW  Created
\**************************************************************************/

typedef struct _FNINLPKDRAWSWITCHWNDMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
    LPWSTR pwsz;
    RECT rcRect;
} FNINLPKDRAWSWITCHWNDMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPKDRAWSWITCHWND)
{
    PLARGE_UNICODE_STRING pstr = &((LPKDRAWSWITCHWND *)lParam)->strName;
    DWORD cbCapture;
    DWORD cCapture = 1;  // Always capture the string
    PWND pwndDCOwner;
    HDC hdcSwitch;
    COLORREF clrOldText, clrOldBk;
    HFONT hOldFont;
    BOOL fAnsiReceiver = FALSE;  // The string is always Unicode

    SETUPPWND(FNINLPKDRAWSWITCHWND)

    UNREFERENCED_PARAMETER(psms);
    UNREFERENCED_PARAMETER(dwSCMSFlags);

    CALC_SIZE_IN(cbCapture, pstr);

    BEGINSENDCAPTURE(FNINLPKDRAWSWITCHWND, cCapture, cbCapture, TRUE)

        LARGECOPYSTRINGLPWSTR(pstr, pwsz);

        pwndDCOwner = _WindowFromDC((HDC)wParam);
        hdcSwitch = _GetDC(pwndDCOwner);
        clrOldText = GreSetTextColor(hdcSwitch, SYSRGB(BTNTEXT));
        clrOldBk   = GreSetBkColor(hdcSwitch, SYSRGB(3DFACE));
        hOldFont = GreSelectFont(hdcSwitch, gpsi->hCaptionFont);

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = (WPARAM)hdcSwitch;
        MSGDATA()->rcRect = ((LPKDRAWSWITCHWND *)lParam)->rcRect;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALLCAPTURE(FNINLPKDRAWSWITCHWND);
        UNLOCKPWND();

        GreSelectFont(hdcSwitch, hOldFont);
        GreSetBkColor(hdcSwitch, clrOldBk);
        GreSetTextColor(hdcSwitch, clrOldText);
        _ReleaseDC(hdcSwitch);

        CHECKRETURN();

    TRACECALLBACKMSG("SfnINLPKDRAWSWITCHWND");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPKDRAWSWITCHWND, FNINLPKDRAWSWITCHWNDMSG)
{
    DRAWTEXTPARAMS  dtp;

    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    dtp.cbSize = sizeof(dtp);
    dtp.iLeftMargin = 0;
    dtp.iRightMargin = 0;
    retval = DrawTextExW(
                 (HDC)pmsg->wParam,
                 pmsg->pwsz,
                 -1,
                 &(pmsg->rcRect),
                 DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE,
                 &dtp
                 );
    ENDRECV();
}
#endif // RECVSIDE

typedef struct _FNINDEVICECHANGEMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
    LPTSTR pwsz;
    BOOL fAnsi;
} FNINDEVICECHANGEMSG;

#ifdef SENDSIDE
SMESSAGECALL(INDEVICECHANGE)
{
    PVOID pstr = (PVOID)lParam;
    DWORD cbCapture;
    BOOL fAnsiReceiver = (dwSCMSFlags & SCMS_FLAGS_ANSI);
    BOOL fPtr    = (BOOL)((wParam & 0x8000) == 0x8000);

    SETUPPWND(FNINDEVICECHANGE)

    UNREFERENCED_PARAMETER(psms);

    cbCapture = 0;
    if (fPtr && (pstr != NULL)) {

        /*
         * Compute ANSI capture lengths.  Don't capture if
         * the strings are in the client's address space and
         * of the correct type.
         */
        if (IS_SYSTEM_ADDRESS((PVOID)pstr)) {
            cbCapture = *((DWORD *)pstr);
        }
    }

    BEGINSENDCAPTURE(FNINDEVICECHANGE, 1, cbCapture, TRUE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        if (cbCapture) {
           LARGECOPYBYTES2(pstr, *((DWORD *)pstr), pwsz);
        } else {
           MSGDATA()->pwsz = (LPTSTR)pstr;
        }

        MSGDATA()->fAnsi = fAnsiReceiver;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALLCAPTURE(FNINDEVICECHANGE);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINDEVICECHANGE");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINDEVICECHANGE, FNINDEVICECHANGEMSG)
{

    struct _DEV_BROADCAST_HEADER *pHdr;
    PDEV_BROADCAST_PORT_A pPortA = NULL;
    PDEV_BROADCAST_PORT_W pPortW;
    PDEV_BROADCAST_DEVICEINTERFACE_A pInterfaceA = NULL;
    PDEV_BROADCAST_DEVICEINTERFACE_W pInterfaceW;
    PDEV_BROADCAST_HANDLE pHandleA = NULL;
    PDEV_BROADCAST_HANDLE pHandleW;

    int iStr, iSize;
    LPSTR lpStr;
    LPARAM lParam;

    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    if (!(lParam = (LPARAM)pmsg->pwsz) || !(pmsg->wParam & 0x8000) || !pmsg->fAnsi) {
        goto shipit;
    }

    pHdr = (struct _DEV_BROADCAST_HEADER *)lParam;
    switch (pHdr->dbcd_devicetype) {
    case DBT_DEVTYP_PORT:
        pPortW = (PDEV_BROADCAST_PORT_W)lParam;
        iStr = wcslen(pPortW->dbcp_name);
        iSize = FIELD_OFFSET(DEV_BROADCAST_PORT_A, dbcp_name) + DBCS_CHARSIZE*(iStr+1);
        pPortA = UserLocalAlloc(0, iSize);
        if (pPortA == NULL)
            MSGERROR();
        RtlCopyMemory(pPortA, pPortW, FIELD_OFFSET(DEV_BROADCAST_PORT_A, dbcp_name));
        lpStr = pPortA->dbcp_name;
        if (iStr) {
            WCSToMB(pPortW->dbcp_name, -1, &lpStr, iStr, FALSE);
        }
        lpStr[iStr] = 0;
        pPortA->dbcp_size = iSize;
        lParam = (LPARAM)pPortA;
        break;
    case DBT_DEVTYP_DEVICEINTERFACE:
        pInterfaceW = (PDEV_BROADCAST_DEVICEINTERFACE_W)lParam;
        iStr = wcslen(pInterfaceW->dbcc_name);
        iSize = FIELD_OFFSET(DEV_BROADCAST_DEVICEINTERFACE_A, dbcc_name) + DBCS_CHARSIZE*(iStr+1);
        pInterfaceA = UserLocalAlloc(0, iSize);
        if (pInterfaceA == NULL)
            MSGERROR();
        RtlCopyMemory(pInterfaceA, pInterfaceW, FIELD_OFFSET(DEV_BROADCAST_DEVICEINTERFACE_A, dbcc_name));
        lpStr = pInterfaceA->dbcc_name;
        if (iStr) {
            WCSToMB(pInterfaceW->dbcc_name, -1, &lpStr, iStr, FALSE);
        }
        lpStr[iStr] = 0;
        pInterfaceA->dbcc_size = iSize;
        lParam = (LPARAM)pInterfaceA;
        break;
    case DBT_DEVTYP_HANDLE:
        pHandleW = (PDEV_BROADCAST_HANDLE)lParam;
        if ((pmsg->wParam != DBT_CUSTOMEVENT) || (pHandleW->dbch_nameoffset < 0)) break;
        iStr = wcslen((LPWSTR)(pHandleW->dbch_data+pHandleW->dbch_nameoffset));
        iSize = pHandleW->dbch_size;
        /*
         * MB size can't be bigger than UNICODE size
         */
        pHandleA = UserLocalAlloc(0, iSize);
        if (pHandleA == NULL)
            MSGERROR();
        RtlCopyMemory(pHandleA, pHandleW, FIELD_OFFSET(DEV_BROADCAST_HANDLE, dbch_data)+ pHandleW->dbch_nameoffset);
        lpStr = pHandleA->dbch_data+pHandleA->dbch_nameoffset;
        if (iStr) {
            WCSToMB((LPWSTR)(pHandleW->dbch_data+pHandleW->dbch_nameoffset), -1, &lpStr, iStr, FALSE);
        }
        lpStr[iStr] = 0;
        pHandleA->dbch_size = iSize;
        lParam = (LPARAM)pHandleA;


        break;
    }
shipit:
    retval = (ULONG_PTR)CALLPROC(pmsg->xpfnProc)(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            lParam,
            pmsg->xParam);

    if (pInterfaceA) UserLocalFree(pInterfaceA);
    if (pPortA) UserLocalFree(pPortA);
    if (pHandleA) UserLocalFree(pHandleA);

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* fnOUTSTRING
*
* Warning this message copies but does not count the NULL in retval
* as in WM_GETTEXT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNOUTSTRINGMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
    PBYTE pOutput;
    DWORD cbOutput;
} FNOUTSTRINGMSG;

#ifdef SENDSIDE
SMESSAGECALL(OUTSTRING)
{
    PLARGE_STRING pstr = (PLARGE_STRING)lParam;
    DWORD cbCapture;
    BOOL fAnsiReceiver = (dwSCMSFlags & SCMS_FLAGS_ANSI);
    BOOL bInflateWParam = FALSE;

    SETUPPWND(FNOUTSTRING)

    CALC_SIZE_OUT_STRING(cbCapture, pstr);

    BEGINSENDCAPTURE(FNOUTSTRING, 1, cbCapture, FALSE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;

        /*
         * Need to wParam MBCS bytes may be required to form wParam Unicode bytes
         */
        if (fAnsiReceiver && !(pstr->bAnsi)) {
            /*
             * Unicode -> Ansi
             */
            MSGDATA()->wParam = (wParam * sizeof(WCHAR));
            PtiCurrent()->TIF_flags |= TIF_ANSILENGTH;
            bInflateWParam = TRUE;
        } else {
            /*
             * if wParam is already adjusted for ANSI, we need to re-adjust for Unicode...
             *
             * This logic is for following cases...
             *
             * +========+===============+=============+================+=============+
             * |WndProc |Unicode WndProc->Ansi WndProc->Unicode WndProc->Ansi WndProc|
             * +--------+---------------+-------------+----------------+-------------+
             * |Length  |      X        ->  (X * 2)   ->       X       ->  (X * 2)   |
             * +--------+---------------+-------------+----------------+-------------+
             */
            if (!fAnsiReceiver && (PtiCurrent()->TIF_flags & TIF_ANSILENGTH)) {
                /* adjust limit also... */
                MSGDATA()->wParam = wParam = (wParam / sizeof(WCHAR));
                PtiCurrent()->TIF_flags &= ~TIF_ANSILENGTH;
            } else {
                MSGDATA()->wParam = wParam;
            }
        }
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        RESERVEBYTES(cbCapture, pOutput, cbOutput);

        LOCKPWND();
        MAKECALLCAPTURE(FNOUTSTRING);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            if (retval) {
                /*
                 * Non-zero retval means some text to copy out.  Do not copy out
                 * more than the requested byte count 'wParam'.
                 */
                COPYOUTLPWSTRLIMIT(pstr, (int)wParam);
            } else {
                /*
                 * A dialog function returning FALSE means no text to copy out,
                 * but an empty string also has retval == 0: put a null char in
                 * pstr for the latter case.
                 */
                if (wParam != 0) {
                    if (pstr->bAnsi) {
                         *(PCHAR)pstr->Buffer = 0;
                    } else {
                         *(PWCHAR)pstr->Buffer = 0;
                    }
                }
            }
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnOUTSTRING");
    ENDSENDCAPTUREOUTSTRING(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTSTRING, FNOUTSTRINGMSG)
{
    BYTE abOutput[CALLBACKSTACKLIMIT];

    BEGINRECV(0, NULL, pmsg->cbOutput);
    FIXUPPOINTERS();
    if (pmsg->cbOutput <= CALLBACKSTACKLIMIT)
        CallbackStatus.pOutput = abOutput;
    else
        CallbackStatus.pOutput = pmsg->pOutput;

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPARAM)CallbackStatus.pOutput,
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINCNTOUTSTRING
*
* Does NOT NULL terminate string
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINCNTOUTSTRING {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
    WORD cchMax;
    PBYTE pOutput;
    DWORD cbOutput;
} FNINCNTOUTSTRINGMSG;

#ifdef SENDSIDE
SMESSAGECALL(INCNTOUTSTRING)
{
    PLARGE_STRING pstr = (PLARGE_STRING)lParam;
    DWORD cbCapture;
    WORD cchOriginal;
    BOOL fAnsiReceiver = (dwSCMSFlags & SCMS_FLAGS_ANSI);

    SETUPPWND(FNINCNTOUTSTRING)

    CALC_SIZE_OUT(cbCapture, pstr);

    BEGINSENDCAPTURE(FNINCNTOUTSTRING, 1, cbCapture, FALSE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        cchOriginal = (WORD)pstr->MaximumLength;
        if (!pstr->bAnsi)
            cchOriginal /= sizeof(WCHAR);

        MSGDATA()->cchMax = (WORD)min(cchOriginal, 0xffff);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        RESERVEBYTES(cbCapture, pOutput, cbOutput);

        LOCKPWND();
        MAKECALLCAPTURE(FNINCNTOUTSTRING)
        UNLOCKPWND();
        CHECKRETURN();

        /*
         * We don't want to do the copy out of the sender died or if
         * this message was just sent as part of a CALLWNDPROC hook processing
         */
        BEGINCOPYOUT()
            if (retval) {
                /*
                 * Non-zero retval means some text to copy out.  Do not copy out
                 * more than the requested char count 'wParam'.
                 */
                COPYOUTLPWSTRLIMIT(pstr, (int)cchOriginal);
            } else {
                /*
                 * A dialog function returning FALSE means no text to copy out,
                 * but an empty string also has retval == 0: put a null char in
                 * pstr for the latter case.
                 */
                if (pstr->bAnsi) {
                    *(PCHAR)pstr->Buffer = 0;
                } else {
                    *(PWCHAR)pstr->Buffer = 0;
                }
            }
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnINCNTOUTSTRING");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINCNTOUTSTRING, FNINCNTOUTSTRINGMSG)
{
    BYTE abOutput[CALLBACKSTACKLIMIT];

    BEGINRECV(0, NULL, pmsg->cbOutput);
    FIXUPPOINTERS();
    if (pmsg->cbOutput <= CALLBACKSTACKLIMIT)
        CallbackStatus.pOutput = abOutput;
    else
        CallbackStatus.pOutput = pmsg->pOutput;

    *(LPWORD)CallbackStatus.pOutput = CALLDATA(cchMax);

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPSTR)CallbackStatus.pOutput,
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINCNTOUTSTRINGNULL
*
* wParam specifies the maximum number of bytes to copy
* the string is NULL terminated
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINCNTOUTSTRINGNULL {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
    PBYTE pOutput;
    DWORD cbOutput;
} FNINCNTOUTSTRINGNULLMSG;

#ifdef SENDSIDE
SMESSAGECALL(INCNTOUTSTRINGNULL)
{
    PLARGE_STRING pstr = (PLARGE_STRING)lParam;
    DWORD cbCapture;
    BOOL fAnsiReceiver = (dwSCMSFlags & SCMS_FLAGS_ANSI);

    SETUPPWND(FNINCNTOUTSTRINGNULL)

    CALC_SIZE_OUT(cbCapture, pstr);

    BEGINSENDCAPTURE(FNINCNTOUTSTRINGNULL, 1, cbCapture, FALSE)

        if (wParam < 2) {   // However unlikely, this prevents a possible GP
            MSGERROR();     // on the server side.
        }

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        RESERVEBYTES(cbCapture, pOutput, cbOutput);

        LOCKPWND();
        MAKECALLCAPTURE(FNINCNTOUTSTRINGNULL)
        UNLOCKPWND();
        CHECKRETURN();

        /*
         * We don't want to do the copy out of the sender died or if
         * this message was just sent as part of a CALLWNDPROC hook processing
         */
        BEGINCOPYOUT()
            if (pcbs->cbOutput != 0) {

                /*
                 * Buffer changed means some text to copy out.  Do not copy out
                 * more than the requested byte count 'wParam'.
                 */
                COPYOUTLPWSTRLIMIT(pstr, (int)wParam);
            }
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnINCNTOUTSTRINGNULL");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINCNTOUTSTRINGNULL, FNINCNTOUTSTRINGNULLMSG)
{
    BYTE abOutput[CALLBACKSTACKLIMIT];

    BEGINRECV(0, NULL, pmsg->cbOutput);
    FIXUPPOINTERS();
    if (pmsg->cbOutput <= CALLBACKSTACKLIMIT)
        CallbackStatus.pOutput = abOutput;
    else
        CallbackStatus.pOutput = pmsg->pOutput;

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPSTR)CallbackStatus.pOutput,
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnPOUTLPINT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNPOUTLPINTMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
    PBYTE pOutput;
    DWORD cbOutput;
} FNPOUTLPINTMSG;

#ifdef SENDSIDE
SMESSAGECALL(POUTLPINT)
{
    DWORD cbCapture;
    LPINT pint = (LPINT)lParam;

    SETUPPWND(FNPOUTLPINT)

    cbCapture = (UINT)wParam * sizeof(INT);

    BEGINSENDCAPTURE(FNPOUTLPINT, 1, cbCapture, FALSE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        /*
         * Hooks should see the buffer content
         */
        if (dwSCMSFlags & SCMS_FLAGS_INONLY) {
            MSGDATA()->cbOutput = cbCapture;
            LARGECOPYBYTES2(pint, cbCapture, pOutput);
        } else {
            RESERVEBYTES(cbCapture, pOutput, cbOutput);
        }

        LOCKPWND();
        MAKECALLCAPTURE(FNPOUTLPINT);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            try {
                ProbeForRead(pcbs->pOutput, pcbs->cbOutput, sizeof(DWORD));
                memcpy(pint, pcbs->pOutput, cbCapture);
            } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                MSGERROR();
            }
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnPOUTLPINT");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnPOUTLPINT, FNPOUTLPINTMSG)
{
    BYTE abOutput[CALLBACKSTACKLIMIT];

    BEGINRECV(0, NULL, pmsg->cbOutput);
    FIXUPPOINTERS();
    if (pmsg->cbOutput <= CALLBACKSTACKLIMIT)
        CallbackStatus.pOutput = abOutput;
    else
        CallbackStatus.pOutput = pmsg->pOutput;

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPINT)CallbackStatus.pOutput,
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnPOPTINLPUINT
*
* NOTE!!! -- This function actually thunks arrays of INTs (32bit) and not
* WORDs (16bit).  The name was left the same to prevent a global rebuild
* of client and server.  The name should be changed to fnPOPTINLPINT as
* soon as we ship the beta!  The corresponding callforward function in
* cf2.h should also have its name changed.
*
* 22-Jul-1991 mikeke    Created
* 07-Jan-1993 JonPa     Changed to pass INTs instead of WORDs
\**************************************************************************/

typedef struct _FNPOPTINLPUINTMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPWORD pw;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNPOPTINLPUINTMSG;

#ifdef SENDSIDE
SMESSAGECALL(POPTINLPUINT)
{
    LPWORD pw = (LPWORD)lParam;
    DWORD cCapture, cbCapture;

    SETUPPWND(FNPOPTINLPUINT);

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    if (lParam) {
        cCapture = 1;
        cbCapture = (UINT)wParam * sizeof(UINT);
    } else {
        cCapture = cbCapture = 0;
    }

    BEGINSENDCAPTURE(FNPOPTINLPUINT, cCapture, cbCapture, TRUE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        COPYBYTESOPT(pw, cbCapture);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALLCAPTURE(FNPOPTINLPUINT);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnPOPTINLPUINT");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnPOPTINLPUINT, FNPOPTINLPUINTMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPDWORD)FIRSTFIXUPOPT(pw),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINOUTLPWINDOWPOS (for WM_WINDOWPOSCHANGING message)
*
* 08-11-91 darrinm      Created.
\**************************************************************************/

typedef struct _FNINOUTLPWINDOWPOSMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    WINDOWPOS wp;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINOUTLPWINDOWPOSMSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTLPWINDOWPOS)
{
    SETUPPWND(FNINOUTLPWINDOWPOS)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNINOUTLPWINDOWPOS)

        LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->wp = *pwp;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINOUTLPWINDOWPOS);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            OUTSTRUCT(pwp, WINDOWPOS);
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnINOUTLPWINDOWPOS");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTLPWINDOWPOS, FNINOUTLPWINDOWPOSMSG)
{
    BEGINRECV(0, &pmsg->wp, sizeof(pmsg->wp));

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            pmsg->pwnd,
            pmsg->msg,
            pmsg->wParam,
            PCALLDATA(wp),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* fnINLPWINDOWPOS (for WM_WINDOWPOSCHANGED message)
*
* 08-11-91 darrinm      Created.
\**************************************************************************/

typedef struct _FNINLPWINDOWPOSMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    WINDOWPOS wp;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINLPWINDOWPOSMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPWINDOWPOS)
{
    SETUPPWND(FNINLPWINDOWPOS)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNINLPWINDOWPOS)

        LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->wp = *pwp;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINLPWINDOWPOS);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnINLPWINDOWPOS");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPWINDOWPOS, FNINLPWINDOWPOSMSG)
{
    BEGINRECV(0, NULL, 0);

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            PCALLDATA(wp),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE




/**************************************************************************\
* fnINOUTNEXTMENU
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINOUTNEXTMENUMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    MDINEXTMENU mnm;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINOUTNEXTMENUMSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTNEXTMENU)
{
    SETUPPWND(FNINOUTNEXTMENU)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNINOUTNEXTMENU)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->mnm = *((PMDINEXTMENU)lParam);

        LOCKPWND();
        MAKECALL(FNINOUTNEXTMENU);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            OUTSTRUCT(((PMDINEXTMENU)lParam), MDINEXTMENU);
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnINOUTNEXTMENU");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTNEXTMENU, FNINOUTNEXTMENUMSG)
{
    BEGINRECV(0, &pmsg->mnm, sizeof(pmsg->mnm));

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            &CALLDATA(mnm),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnHkINLPCBTCREATESTRUCT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CREATESTRUCTDATA {
    CREATESTRUCT cs;
    HWND hwndInsertAfter;
} CREATESTRUCTDATA;

typedef struct _FNHKINLPCBTCREATESTRUCTMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    CREATESTRUCTDATA d;
    PROC xpfnProc;
    BOOL bAnsi;
} FNHKINLPCBTCREATESTRUCTMSG;

#ifdef SENDSIDE
LRESULT fnHkINLPCBTCREATESTRUCT(
    UINT msg,
    WPARAM wParam,
    LPCBT_CREATEWND pcbt,
    PROC xpfnProc,
    BOOL fAnsiReceiver)
{
    DWORD cbTitle = 0, cbClass = 0;
    DWORD cCapture = 0;
    CREATESTRUCTDATA csdOut;
    PCREATESTRUCTEX pcreatestruct;
    PWND pwnd = _GetDesktopWindow();

    SETUPPWND(FNHKINLPCBTCREATESTRUCT)

    /*
     * Compute ANSI capture lengths.  Don't capture if
     * the strings are in the client's address space.
     */
    pcreatestruct = (PCREATESTRUCTEX)pcbt->lpcs;
    if (pcreatestruct->cs.lpszName &&
            ((BOOL)pcreatestruct->strName.bAnsi != fAnsiReceiver ||
            IS_SYSTEM_ADDRESS((PVOID)pcreatestruct->cs.lpszName))) {
        CALC_SIZE_IN(cbTitle, &pcreatestruct->strName);
        cCapture++;
    }
    if (IS_PTR(pcreatestruct->cs.lpszClass) &&
            ((BOOL)pcreatestruct->strClass.bAnsi != fAnsiReceiver ||
            IS_SYSTEM_ADDRESS((PVOID)pcreatestruct->cs.lpszClass))) {
        CALC_SIZE_IN(cbClass, &pcreatestruct->strClass);
        cCapture++;
    }

    BEGINSENDCAPTURE(FNHKINLPCBTCREATESTRUCT, cCapture, cbTitle + cbClass, TRUE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->d.cs = *(pcbt->lpcs);

        if (cbTitle) {
            if (!pcreatestruct->strName.bAnsi) {
                if (*(PWORD)pcreatestruct->cs.lpszName == 0xffff) {

                    /*
                     * Copy out an ordinal of the form 0xffff, ID.
                     * If the receiver is ANSI, skip the first 0xff.
                     */
                    if (fAnsiReceiver) {
                        if (!NT_SUCCESS(CaptureCallbackData(&mp->CaptureBuf,
                                (PBYTE)pcreatestruct->cs.lpszName + 1,
                                3, (PVOID *)&mp->d.cs.lpszName)))
                            goto errorexit;
                    } else {
                        if (!NT_SUCCESS(CaptureCallbackData(&mp->CaptureBuf,
                                (PBYTE)pcreatestruct->cs.lpszName,
                                4, (PVOID *)&mp->d.cs.lpszName)))
                            goto errorexit;
                    }
                } else if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPWSTRA(&pcreatestruct->strName, d.cs.lpszName);
                } else {
                    LARGECOPYSTRINGLPWSTR(&pcreatestruct->strName, d.cs.lpszName);
                }
            } else {
                if (*(PBYTE)pcreatestruct->cs.lpszName == 0xff) {

                    /*
                     * Copy out an ordinal of the form 0xff, ID.
                     * If the receiver is UNICODE, expand the 0xff to 0xffff.
                     */
                    if (fAnsiReceiver) {
                        if (!NT_SUCCESS(CaptureCallbackData(&mp->CaptureBuf,
                                (PBYTE)pcreatestruct->cs.lpszName,
                                3, (PVOID *)&mp->d.cs.lpszName)))
                            goto errorexit;
                    } else {
                        DWORD dwOrdinal;

                        dwOrdinal = MAKELONG(0xffff,
                                (*(DWORD UNALIGNED *)pcreatestruct->cs.lpszName >> 8));
                        if (!NT_SUCCESS(CaptureCallbackData(&mp->CaptureBuf,
                                &dwOrdinal,
                                4, (PVOID *)&mp->d.cs.lpszName)))
                            goto errorexit;
                    }
                } else if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPSTR(&pcreatestruct->strName, d.cs.lpszName);
                } else {
                    LARGECOPYSTRINGLPSTRW(&pcreatestruct->strName, d.cs.lpszName);
                }
            }
        }
        if (cbClass) {
            if (!pcreatestruct->strClass.bAnsi) {
                if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPWSTRA(&pcreatestruct->strClass, d.cs.lpszClass);
                } else {
                    LARGECOPYSTRINGLPWSTR(&pcreatestruct->strClass, d.cs.lpszClass);
                }
            } else {
                if (fAnsiReceiver) {
                    LARGECOPYSTRINGLPSTR(&pcreatestruct->strClass, d.cs.lpszClass);
                } else {
                    LARGECOPYSTRINGLPSTRW(&pcreatestruct->strClass, d.cs.lpszClass);
                }
            }
        }

        MSGDATA()->d.hwndInsertAfter = pcbt->hwndInsertAfter;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->bAnsi = fAnsiReceiver;

        LOCKPWND();
        MAKECALLCAPTURE(FNHKINLPCBTCREATESTRUCT);
        UNLOCKPWND();
        CHECKRETURN();

        /*
         * Probe output data
         */
        OUTSTRUCT(&csdOut, CREATESTRUCTDATA);

        // MS Visual C centers its dialogs with the CBT_CREATEHOOK
        pcbt->hwndInsertAfter = csdOut.hwndInsertAfter;
        pcbt->lpcs->x  = csdOut.cs.x;
        pcbt->lpcs->y  = csdOut.cs.y;
        pcbt->lpcs->cx = csdOut.cs.cx;
        pcbt->lpcs->cy = csdOut.cs.cy;

    TRACECALLBACK("SfnHkINLPCBTCREATESTRUCT");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPCBTCREATESTRUCT, FNHKINLPCBTCREATESTRUCTMSG)
{
    CBT_CREATEWND cbt;

    BEGINRECV(0, &pmsg->d, sizeof(pmsg->d));
    FIXUPPOINTERS();

    cbt.lpcs = &pmsg->d.cs;
    cbt.hwndInsertAfter = pmsg->d.hwndInsertAfter;
    if ((ULONG_PTR)pmsg->d.cs.lpszName > gHighestUserAddress)
        pmsg->d.cs.lpszName = REBASEPTR(pmsg->pwnd, pmsg->d.cs.lpszName);
    if ((ULONG_PTR)pmsg->d.cs.lpszClass > gHighestUserAddress)
        pmsg->d.cs.lpszClass = REBASEPTR(pmsg->pwnd, pmsg->d.cs.lpszClass);

    if (pmsg->bAnsi) {
        retval = DispatchHookA(
                pmsg->msg,
                pmsg->wParam,
                (LPARAM)&cbt,
                (HOOKPROC)pmsg->xpfnProc);
    } else {
        retval = DispatchHookW(
                pmsg->msg,
                pmsg->wParam,
                (LPARAM)&cbt,
                (HOOKPROC)pmsg->xpfnProc);
    }

    pmsg->d.hwndInsertAfter = cbt.hwndInsertAfter;
    pmsg->d.cs.x  = cbt.lpcs->x;
    pmsg->d.cs.y  = cbt.lpcs->y;
    pmsg->d.cs.cx = cbt.lpcs->cx;
    pmsg->d.cs.cy = cbt.lpcs->cy;

    ENDRECV();
}
#endif // RECVSIDE

#ifdef REDIRECTION

/**************************************************************************\
* fnHkINLPPOINT
*
* 29-Jan-1999 clupu    Created
\**************************************************************************/

typedef struct _FNHKINLPPOINTMSG {
    DWORD nCode;
    WPARAM wParam;
    POINT pt;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNHKINLPPOINTMSG;

#ifdef SENDSIDE
LRESULT fnHkINLPPOINT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN OUT LPPOINT ppt,
    IN ULONG_PTR xParam,
    IN PROC xpfnProc)
{
    SETUP(FNHKINLPPOINT)

    BEGINSEND(FNHKINLPPOINT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->pt = *ppt;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKINLPPOINT);
        CHECKRETURN();

        /*
         * Probe output data
         */
        OUTSTRUCT(ppt, POINT);

    TRACECALLBACK("SfnHkINLPPOINT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPPOINT, FNHKINLPPOINTMSG)
{
    BEGINRECV(0, &pmsg->pt, sizeof(POINT));

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->nCode,
            pmsg->wParam,
            PCALLDATA(pt),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

#endif // REDIRECTION


/**************************************************************************\
* fnHkINLPRECT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINLPRECTMSG {
    DWORD nCode;
    WPARAM wParam;
    RECT rect;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNHKINLPRECTMSG;

#ifdef SENDSIDE
LRESULT fnHkINLPRECT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN OUT LPRECT prect,
    IN ULONG_PTR xParam,
    IN PROC xpfnProc)
{
    SETUP(FNHKINLPRECT)

    BEGINSEND(FNHKINLPRECT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->rect = *prect;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKINLPRECT);
        CHECKRETURN();

        /*
         * Probe output data
         */
        OUTSTRUCT(prect, RECT);

    TRACECALLBACK("SfnHkINLPRECT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPRECT, FNHKINLPRECTMSG)
{
    BEGINRECV(0, &pmsg->rect, sizeof(RECT));

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->nCode,
            pmsg->wParam,
            PCALLDATA(rect),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINDWORDMSG {
    GENERICHOOKHEADER ghh;
    DWORD flags;
    LPARAM lParam;
} FNHKINDWORDMSG;

#ifdef SENDSIDE
LRESULT fnHkINDWORD(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPARAM lParam,
    IN ULONG_PTR xParam,
    IN PROC xpfnProc,
    IN OUT LPDWORD lpFlags)
{
    SETUP(FNHKINDWORD)

    BEGINSEND(FNHKINDWORD)

        MSGDATA()->ghh.nCode = nCode;
        MSGDATA()->ghh.wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->ghh.xParam = xParam;
        MSGDATA()->ghh.xpfnProc = xpfnProc;
        MSGDATA()->flags = *lpFlags;

        MAKECALL(FNHKINDWORD);
        CHECKRETURN();

        /*
         * Probe output data
         */
        OUTBITMASK(lpFlags, DWORD, HF_HOOKFAULTED);

    TRACECALLBACK("SfnHkINDWORD");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINDWORD, FNHKINDWORDMSG)
{
    BEGINRECV(0, &pmsg->flags, sizeof(pmsg->flags));

    retval = CallHookWithSEH((LPGENERICHOOKHEADER)pmsg, (LPVOID)pmsg->lParam, &pmsg->flags, retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINLPMSGDATA {
    MSG msg;
    DWORD flags;
} FNHKINLPMSGDATA;

typedef struct _FNHKINLPMSGMSG {
    GENERICHOOKHEADER ghh;
    FNHKINLPMSGDATA d;
} FNHKINLPMSGMSG;

#ifdef SENDSIDE
LRESULT fnHkINLPMSG(
    DWORD nCode,
    WPARAM wParam,
    LPMSG pmsg,
    ULONG_PTR xParam,
    PROC xpfnProc,
    BOOL bAnsi,
    LPDWORD lpFlags)
{
    SETUP(FNHKINLPMSG)
    WPARAM wParamOriginal;

    BEGINSEND(FNHKINLPMSG)

        MSGDATA()->ghh.nCode = nCode;
        MSGDATA()->ghh.wParam = wParam;

        MSGDATA()->d.msg = *pmsg;
        if (((WM_CHAR == pmsg->message) || (WM_SYSCHAR == pmsg->message)) && bAnsi) {
            wParamOriginal = pmsg->wParam;
            RtlWCSMessageWParamCharToMB(pmsg->message, &(MSGDATA()->d.msg.wParam));
        }

        MSGDATA()->ghh.xParam = xParam;
        MSGDATA()->ghh.xpfnProc = xpfnProc;
        MSGDATA()->d.flags = *lpFlags;

        MAKECALL(FNHKINLPMSG);
        CHECKRETURN();

        /*
         * Probe output data
         */
        try {
            ProbeForRead(pcbs->pOutput, sizeof(FNHKINLPMSGDATA), sizeof(DWORD));
            *pmsg = ((FNHKINLPMSGDATA *)pcbs->pOutput)->msg;
            COPY_FLAG(*lpFlags, ((FNHKINLPMSGDATA *)pcbs->pOutput)->flags, HF_HOOKFAULTED);
        } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
            MSGERROR();
        }

        if (((WM_CHAR == pmsg->message) || (WM_SYSCHAR == pmsg->message)) && bAnsi) {
            /*
             * LATER, DBCS should be handled correctly.
             */
            /*
             * If the ANSI hook didn't change the wParam we sent it, restore
             * the Unicode value we started with, otherwise we just collapse
             * Unicode chars to an ANSI codepage (best visual fit or ?)
             * The rotten "Intellitype" point32.exe does this.
             */
            if (MSGDATA()->d.msg.wParam == pmsg->wParam) {
                pmsg->wParam = wParamOriginal;
            } else {
                RtlMBMessageWParamCharToWCS(pmsg->message, &pmsg->wParam);
            }
        }

    TRACECALLBACK("SfnHkINLPMSG");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPMSG, FNHKINLPMSGMSG)
{
    BEGINRECV(0, &pmsg->d, sizeof(pmsg->d));

    /*
     * LATER, DBCS should be handled correctly.
     */

    retval = CallHookWithSEH((LPGENERICHOOKHEADER)pmsg, &pmsg->d.msg, &pmsg->d.flags, retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINLPMOUSEHOOKSTRUCTEXMSG {
    GENERICHOOKHEADER ghh;
    DWORD flags;
    MOUSEHOOKSTRUCTEX mousehookstructex;
} FNHKINLPMOUSEHOOKSTRUCTEXMSG;

#ifdef SENDSIDE
LRESULT fnHkINLPMOUSEHOOKSTRUCTEX(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPMOUSEHOOKSTRUCTEX pmousehookstructex,
    IN ULONG_PTR xParam,
    IN PROC xpfnProc,
    IN OUT LPDWORD lpFlags)
{
    SETUP(FNHKINLPMOUSEHOOKSTRUCTEX)

    BEGINSEND(FNHKINLPMOUSEHOOKSTRUCTEX)

        MSGDATA()->ghh.nCode = nCode;
        MSGDATA()->ghh.wParam = wParam;
        MSGDATA()->mousehookstructex = *pmousehookstructex;
        MSGDATA()->ghh.xParam = xParam;
        MSGDATA()->ghh.xpfnProc = xpfnProc;
        MSGDATA()->flags = *lpFlags;

        MAKECALL(FNHKINLPMOUSEHOOKSTRUCTEX);
        CHECKRETURN();

        /*
         * Probe output data
         */
        OUTBITMASK(lpFlags, DWORD, HF_HOOKFAULTED);

    TRACECALLBACK("SfnHkINLPMOUSEHOOKSTRUCTEX");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPMOUSEHOOKSTRUCTEX, FNHKINLPMOUSEHOOKSTRUCTEXMSG)
{
    BEGINRECV(0, &pmsg->flags, sizeof(pmsg->flags));

    retval = CallHookWithSEH((LPGENERICHOOKHEADER)pmsg, &pmsg->mousehookstructex, &pmsg->flags, retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* kbdll
*
* 06-Jun-1996 clupu    Created
\**************************************************************************/

typedef struct _FNHKINLPKBDLLHOOKSTRUCTMSG {
    GENERICHOOKHEADER ghh;
    KBDLLHOOKSTRUCT   kbdllhookstruct;
} FNHKINLPKBDLLHOOKSTRUCTMSG;

#ifdef SENDSIDE
LRESULT fnHkINLPKBDLLHOOKSTRUCT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPKBDLLHOOKSTRUCT pkbdllhookstruct,
    IN ULONG_PTR xParam,
    IN PROC xpfnProc)
{
    SETUP(FNHKINLPKBDLLHOOKSTRUCT)

    BEGINSEND(FNHKINLPKBDLLHOOKSTRUCT)

        MSGDATA()->ghh.nCode       = nCode;
        MSGDATA()->ghh.wParam      = wParam;
        MSGDATA()->kbdllhookstruct = *pkbdllhookstruct;
        MSGDATA()->ghh.xParam      = xParam;
        MSGDATA()->ghh.xpfnProc    = xpfnProc;

        MAKECALL(FNHKINLPKBDLLHOOKSTRUCT);
        CHECKRETURN();

    TRACECALLBACK("SfnHkINLPKBDLLHOOKSTRUCT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPKBDLLHOOKSTRUCT, FNHKINLPKBDLLHOOKSTRUCTMSG)
{
    BEGINRECV(0, &pmsg->kbdllhookstruct, sizeof(pmsg->kbdllhookstruct));

    retval = (DWORD)CALLPROC(pmsg->ghh.xpfnProc)(
            pmsg->ghh.nCode,
            pmsg->ghh.wParam,
            &pmsg->kbdllhookstruct,
            pmsg->ghh.xParam);

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* msll
*
* 06-Jun-1996 clupu    Created
\**************************************************************************/

typedef struct _FNHKINLPMSLLHOOKSTRUCTMSG {
    GENERICHOOKHEADER ghh;
    MSLLHOOKSTRUCT    msllhookstruct;
} FNHKINLPMSLLHOOKSTRUCTMSG;

#ifdef SENDSIDE
LRESULT fnHkINLPMSLLHOOKSTRUCT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPMSLLHOOKSTRUCT pmsllhookstruct,
    IN ULONG_PTR xParam,
    IN PROC xpfnProc)
{
    SETUP(FNHKINLPMSLLHOOKSTRUCT)

    BEGINSEND(FNHKINLPMSLLHOOKSTRUCT)

        MSGDATA()->ghh.nCode      = nCode;
        MSGDATA()->ghh.wParam     = wParam;
        MSGDATA()->msllhookstruct = *pmsllhookstruct;
        MSGDATA()->ghh.xParam     = xParam;
        MSGDATA()->ghh.xpfnProc   = xpfnProc;

        MAKECALL(FNHKINLPMSLLHOOKSTRUCT);
        CHECKRETURN();

    TRACECALLBACK("SfnHkINLPMSLLHOOKSTRUCT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPMSLLHOOKSTRUCT, FNHKINLPMSLLHOOKSTRUCTMSG)
{
    BEGINRECV(0, &pmsg->msllhookstruct, sizeof(pmsg->msllhookstruct));

    retval = (DWORD)CALLPROC(pmsg->ghh.xpfnProc)(
            pmsg->ghh.nCode,
            pmsg->ghh.wParam,
            &pmsg->msllhookstruct,
            pmsg->ghh.xParam);

    ENDRECV();
}
#endif // RECVSIDE

#ifdef REDIRECTION
/**************************************************************************\
* ht
*
* 21-Jan-1999 clupu    Created
\**************************************************************************/

typedef struct _FNHKINLPHTHOOKSTRUCTMSG {
    GENERICHOOKHEADER ghh;
    HTHOOKSTRUCT      hthookstruct;
} FNHKINLPHTHOOKSTRUCTMSG;

#ifdef SENDSIDE
LRESULT fnHkINLPHTHOOKSTRUCT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN OUT LPHTHOOKSTRUCT phthookstruct,
    IN ULONG_PTR xParam,
    IN PROC xpfnProc)
{
    SETUP(FNHKINLPHTHOOKSTRUCT)

    BEGINSEND(FNHKINLPHTHOOKSTRUCT)

        MSGDATA()->ghh.nCode      = nCode;
        MSGDATA()->ghh.wParam     = wParam;
        MSGDATA()->hthookstruct   = *phthookstruct;
        MSGDATA()->ghh.xParam     = xParam;
        MSGDATA()->ghh.xpfnProc   = xpfnProc;

        MAKECALL(FNHKINLPHTHOOKSTRUCT);
        CHECKRETURN();

        /*
         * Probe output data
         */
        if (phthookstruct != NULL)
            OUTSTRUCT(phthookstruct, HTHOOKSTRUCT);

    TRACECALLBACK("SfnHkINLPHTHOOKSTRUCT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPHTHOOKSTRUCT, FNHKINLPHTHOOKSTRUCTMSG)
{
    BEGINRECV(0, &pmsg->hthookstruct, sizeof(pmsg->hthookstruct));

    retval = (DWORD)CALLPROC(pmsg->ghh.xpfnProc)(
            pmsg->ghh.nCode,
            pmsg->ghh.wParam,
            &pmsg->hthookstruct,
            pmsg->ghh.xParam);

    ENDRECV();
}
#endif // RECVSIDE

#endif // REDIRECTION

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKOPTINLPEVENTMSGMSG {
    DWORD nCode;
    WPARAM wParam;
    LPEVENTMSGMSG peventmsgmsg;
    ULONG_PTR xParam;
    PROC xpfnProc;
    EVENTMSG eventmsgmsg;
} FNHKOPTINLPEVENTMSGMSG;

#ifdef SENDSIDE
LRESULT fnHkOPTINLPEVENTMSG(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN OUT LPEVENTMSGMSG peventmsgmsg,
    IN ULONG_PTR xParam,
    IN PROC xpfnProc)
{
    SETUP(FNHKOPTINLPEVENTMSG)

    BEGINSEND(FNHKOPTINLPEVENTMSG)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        COPYSTRUCTOPT(eventmsgmsg);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKOPTINLPEVENTMSG);
        CHECKRETURN();

        /*
         * Probe output data
         */
        if (peventmsgmsg != NULL)
            OUTSTRUCT(peventmsgmsg, EVENTMSG);

    TRACECALLBACK("SfnHkOPTINLPEVENTMSG");
    ENDSEND(DWORD,-1);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkOPTINLPEVENTMSG, FNHKOPTINLPEVENTMSGMSG)
{
    PHOOK phk;

    BEGINRECV(-1, &pmsg->eventmsgmsg, sizeof(pmsg->eventmsgmsg));

    if (pmsg->wParam) {
        phk = (PHOOK)HMValidateHandle((HANDLE)pmsg->wParam, TYPE_HOOK);

        if (phk != NULL) {
            /*
             * The HF_NEEDHC_SKIP bit is passed on from the pti when we need to
             * pass on a HC_SKIP
             */
            if ((phk->flags & HF_NEEDHC_SKIP) &&
                    (HIWORD(pmsg->nCode) == WH_JOURNALPLAYBACK)) {
                UserAssert(LOWORD(pmsg->nCode) == HC_GETNEXT);
                CALLPROC(pmsg->xpfnProc)(
                    MAKELONG(HC_SKIP, HIWORD(pmsg->nCode)),
                    0,
                    0,
                    pmsg->xParam);
            }

            /*
             * Make sure the hook wasn't free'd during the last call to the app
             */
            if (HMIsMarkDestroy(phk)) {
                retval = (DWORD)-1;
                goto AllDoneHere;
            }
        }
    }

    pmsg->wParam = 0;

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->nCode,
            pmsg->wParam,
            PCALLDATAOPT(eventmsgmsg),
            pmsg->xParam);

AllDoneHere:
    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

/*
 * Create a structure big enough to hold the larges item LPARAM points to.
 */
typedef union _DEBUGLPARAM {
    MSG msg;                // WH_GETMESSAGE, WH_MSGFILTER, WH_SYSMSGFILTER
    CWPSTRUCT cwp;          // WH_CALLWNDPROC
    CWPRETSTRUCT cwpret;    // WH_CALLWNDPROCRET
    MOUSEHOOKSTRUCTEX mhs;  // WH_MOUSE, HCBT_CLICKSKIPPED
    EVENTMSG em;            // WH_JOURNALRECORD, WH_JOURNALPLAYBACK
    CBTACTIVATESTRUCT as;   // HCBT_ACTIVATE
    CBT_CREATEWND cw;       // HCBT_CREATEWND
    RECT rc;                // HCBT_MOVESIZE
} DEBUGLPARAM;


typedef struct _FNHKINLPDEBUGHOOKSTRUCTMSG {
    DWORD nCode;
    WPARAM wParam;
    DEBUGHOOKINFO debughookstruct;
    DEBUGLPARAM dbgLParam;
    DWORD cbDbgLParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNHKINLPDEBUGHOOKSTRUCTMSG;

#ifdef SENDSIDE
LRESULT fnHkINLPDEBUGHOOKSTRUCT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPDEBUGHOOKINFO pdebughookstruct,
    IN ULONG_PTR xParam,
    IN PROC xpfnProc)
{
    SETUP(FNHKINLPDEBUGHOOKSTRUCT)

    BEGINSEND(FNHKINLPDEBUGHOOKSTRUCT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->debughookstruct = *pdebughookstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->cbDbgLParam = GetDebugHookLParamSize(wParam, pdebughookstruct);

        switch(wParam) {
        case WH_MOUSE_LL:
        case WH_KEYBOARD_LL:
            return 0;
         }

        /*
         * if LPARAM in the debug hook points to struct then copy it over
         */
        if (MSGDATA()->cbDbgLParam) {
            try {
                RtlCopyMemory(&MSGDATA()->dbgLParam, (BYTE *)pdebughookstruct->lParam,
                        MSGDATA()->cbDbgLParam);
            } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                MSGERROR();
            }
        }

        MAKECALL(FNHKINLPDEBUGHOOKSTRUCT);
        CHECKRETURN();

    TRACECALLBACK("SfnHkINLPDEBUGHOOKSTRUCT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPDEBUGHOOKSTRUCT, FNHKINLPDEBUGHOOKSTRUCTMSG)
{
    BEGINRECV(0, NULL, 0);

    if (pmsg->cbDbgLParam) {
        pmsg->debughookstruct.lParam = (LPARAM)&(pmsg->dbgLParam);
    }

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->nCode,
            pmsg->wParam,
            &(pmsg->debughookstruct),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

DWORD GetDebugHookLParamSize(
    IN WPARAM wParam,
    IN PDEBUGHOOKINFO pdebughookstruct)
{
    DWORD cbDbgLParam = 0;

    switch(wParam) {
    case WH_MSGFILTER:
    case WH_SYSMSGFILTER:
    case WH_GETMESSAGE:
        cbDbgLParam = sizeof(MSG);
        break;

    case WH_CALLWNDPROC:
        cbDbgLParam = sizeof(CWPSTRUCT);
        break;

    case WH_CALLWNDPROCRET:
        cbDbgLParam = sizeof(CWPRETSTRUCT);
        break;

    case WH_MOUSE:
        cbDbgLParam = sizeof(MOUSEHOOKSTRUCTEX);
        break;

    case WH_JOURNALRECORD:
    case WH_JOURNALPLAYBACK:
        cbDbgLParam = sizeof(EVENTMSG);
        break;

    case WH_CBT:
        switch (pdebughookstruct->code) {
        case HCBT_ACTIVATE:
            cbDbgLParam = sizeof(CBTACTIVATESTRUCT);
            break;
        case HCBT_CLICKSKIPPED:
            cbDbgLParam = sizeof(MOUSEHOOKSTRUCTEX);
            break;
        case HCBT_CREATEWND:
            cbDbgLParam = sizeof(CBT_CREATEWND);
            break;
        case HCBT_MOVESIZE:
            cbDbgLParam = sizeof(RECT);
            break;
        }
        break;

    case WH_SHELL:
        if (pdebughookstruct->code == HSHELL_GETMINRECT) {
            cbDbgLParam = sizeof(RECT);
        }
        break;
    }
    return cbDbgLParam;
}

/**************************************************************************\
* fnHkINLPCBTACTIVATESTRUCT
*
* 17-Mar-1992 jonpa    Created
\**************************************************************************/

typedef struct _FNHKINLPCBTACTIVATESTRUCTMSG {
    DWORD nCode;
    WPARAM wParam;
    CBTACTIVATESTRUCT cbtactivatestruct;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNHKINLPCBTACTIVATESTRUCTMSG;

#ifdef SENDSIDE
LRESULT fnHkINLPCBTACTIVATESTRUCT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPCBTACTIVATESTRUCT pcbtactivatestruct,
    IN ULONG_PTR xParam,
    IN PROC xpfnProc)
{
    SETUP(FNHKINLPCBTACTIVATESTRUCT)

    BEGINSEND(FNHKINLPCBTACTIVATESTRUCT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->cbtactivatestruct = *pcbtactivatestruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKINLPCBTACTIVATESTRUCT);
        CHECKRETURN();

    TRACECALLBACK("SfnHkINLPCBTACTIVATESTRUCT");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPCBTACTIVATESTRUCT, FNHKINLPCBTACTIVATESTRUCTMSG)
{
    BEGINRECV(0, NULL, 0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->nCode,
            pmsg->wParam,
            &(pmsg->cbtactivatestruct),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* ClientLoadMenu
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTLOADMENUMSG {
    CAPTUREBUF CaptureBuf;
    HANDLE hmod;
    UNICODE_STRING strName;
} CLIENTLOADMENUMSG;

#ifdef SENDSIDE
PMENU xxxClientLoadMenu(
    IN HANDLE hmod,
    IN PUNICODE_STRING pstrName)
{
    DWORD cCapture, cbCapture;

    SETUP(CLIENTLOADMENU)

    if (pstrName->MaximumLength) {
        cCapture = 1;
        cbCapture = pstrName->MaximumLength;
    } else
        cCapture = cbCapture = 0;

    BEGINSENDCAPTURE(CLIENTLOADMENU, cCapture, cbCapture, TRUE)

        MSGDATA()->hmod = hmod;
        COPYSTRINGID(strName);

        MAKECALLCAPTURE(CLIENTLOADMENU);
        CHECKRETURN();

        retval = (ULONG_PTR)HtoP((HMENU)retval);

    TRACECALLBACK("ClientLoadMenu");
    ENDSENDCAPTURE(PMENU,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientLoadMenu, CLIENTLOADMENUMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = (ULONG_PTR)LoadMenu(
            CALLDATA(hmod) ? CALLDATA(hmod) : hmodUser,
            (LPTSTR)FIXUPSTRINGID(strName));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* xxxClientLoadImage
*
* 28-Aug-1995 ChrisWil    Created
\**************************************************************************/

typedef struct _CLIENTLOADIMAGEMSG {
    CAPTUREBUF     CaptureBuf;
    UNICODE_STRING strModName;
    UNICODE_STRING strName;
    UINT           uImageType;
    int            cxDesired;
    int            cyDesired;
    UINT           LR_flags;
    BOOL           fWallpaper;
} CLIENTLOADIMAGEMSG;

#ifdef SENDSIDE
HANDLE xxxClientLoadImage(
    IN PUNICODE_STRING pstrName,
    IN ATOM            atomModName,
    IN WORD            wImageType,
    IN int             cxDesired,
    IN int             cyDesired,
    IN UINT            LR_flags,
    IN BOOL            fWallpaper)
{
    DWORD           cCapture;
    DWORD           cbCapture;
    WCHAR           awszModName[MAX_PATH];
    UNICODE_STRING  strModName;
    PUNICODE_STRING pstrModName = &strModName;

    SETUP(CLIENTLOADIMAGE)

    if (pstrName->MaximumLength) {
        cCapture  = 1;
        cbCapture = pstrName->MaximumLength;
    } else {
        cCapture  =
        cbCapture = 0;
    }
    if (atomModName && atomModName != atomUSER32) {
        UserGetAtomName(atomModName, awszModName, MAX_PATH);
        RtlInitUnicodeString(&strModName, awszModName);
    } else {
        strModName.Length = strModName.MaximumLength = 0;
        strModName.Buffer = NULL;
    }
    if (pstrModName->MaximumLength) {
        cCapture++;
        cbCapture += pstrModName->MaximumLength;
    }

    BEGINSENDCAPTURE(CLIENTLOADIMAGE, cCapture, cbCapture, TRUE)

        COPYSTRINGOPT(strModName);
        COPYSTRINGID(strName);
        MSGDATA()->uImageType = (UINT)wImageType;
        MSGDATA()->cxDesired  = cxDesired;
        MSGDATA()->cyDesired  = cyDesired;
        MSGDATA()->LR_flags   = LR_flags;
        MSGDATA()->fWallpaper = fWallpaper;

        MAKECALLCAPTURE(CLIENTLOADIMAGE);
        CHECKRETURN();

        if (retval && (wImageType != IMAGE_BITMAP)) {
            retval = (ULONG_PTR)HMRevalidateHandle((HANDLE)retval);
        }

    TRACECALLBACK("ClientLoadImage");
    ENDSENDCAPTURE(PCURSOR,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientLoadImage, CLIENTLOADIMAGEMSG)
{
    HMODULE hmod;
    LPTSTR  filepart;
    LPTSTR  lpszName;
    TCHAR   szFullPath[MAX_PATH];

    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    if (hmod = (HMODULE)FIXUPSTRINGIDOPT(strModName)) {

        if ((hmod = GetModuleHandle((LPTSTR)hmod)) == NULL) {
            MSGERROR();
        }
    }

    /*
     * Find the file.  This normalizes the filename.
     */
    lpszName = (LPTSTR)FIXUPSTRINGID(strName);

    if (CALLDATA(fWallpaper)) {

        if (!SearchPath(NULL,
                       lpszName,
                       TEXT(".bmp"),
                       ARRAY_SIZE(szFullPath),
                       szFullPath,
                       &filepart)) {

            MSGERROR();
        }

        lpszName = szFullPath;
    }

    retval = (ULONG_PTR)LoadImage(hmod,
                              lpszName,
                              CALLDATA(uImageType),
                              CALLDATA(cxDesired),
                              CALLDATA(cyDesired),
                              CALLDATA(LR_flags));

    ENDRECV();
}
#endif // RECVSIDE

/***********************************************************************\
* xxxClientCopyImage
*
* Returns: hIconCopy - note LR_flags could cause this to be the same as
*       what came in.
*
* 11/3/1995 Created SanfordS
\***********************************************************************/

typedef struct _CLIENTCOPYIMAGEMSG {
    HANDLE         hImage;
    UINT           uImageType;
    int            cxDesired;
    int            cyDesired;
    UINT           LR_flags;
} CLIENTCOPYIMAGEMSG;

#ifdef SENDSIDE
HANDLE xxxClientCopyImage(
    IN HANDLE          hImage,
    IN UINT            uImageType,
    IN int             cxDesired,
    IN int             cyDesired,
    IN UINT            LR_flags)
{
    SETUP(CLIENTCOPYIMAGE)

    BEGINSEND(CLIENTCOPYIMAGE)

        MSGDATA()->hImage     = hImage;
        MSGDATA()->uImageType = uImageType;
        MSGDATA()->cxDesired  = cxDesired;
        MSGDATA()->cyDesired  = cyDesired;
        MSGDATA()->LR_flags   = LR_flags;

        MAKECALL(CLIENTCOPYIMAGE);
        CHECKRETURN();

        if (retval && (uImageType != IMAGE_BITMAP)) {
            retval = (ULONG_PTR)HMRevalidateHandle((HANDLE)retval);
        }

    TRACECALLBACK("ClientCopyImage");
    ENDSEND(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientCopyImage, CLIENTCOPYIMAGEMSG)
{
    BEGINRECV(0, NULL, 0);

    retval = (ULONG_PTR)InternalCopyImage(CALLDATA(hImage),
                                      CALLDATA(uImageType),
                                      CALLDATA(cxDesired),
                                      CALLDATA(cyDesired),
                                      CALLDATA(LR_flags));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTGETLISTBOXSTRINGMSG {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfn;
    PBYTE pOutput;
    DWORD cbOutput;
} CLIENTGETLISTBOXSTRINGMSG;

#ifdef SENDSIDE
DWORD ClientGetListboxString(
    IN PWND pwnd,
    IN UINT msg,
    IN WPARAM wParam,
    OUT PVOID pdata,
    IN ULONG_PTR xParam,
    IN PROC xpfn,
    IN DWORD dwSCMSFlags,
    IN BOOL bNotString,
    IN PSMS psms)
{
    DWORD cbCapture;
    DWORD cchRet;
    PLARGE_STRING pstr;
    BOOL fAnsiReceiver = (dwSCMSFlags & SCMS_FLAGS_ANSI);

    SETUPPWND(CLIENTGETLISTBOXSTRING)

    CheckLock(pwnd);

    pstr = (PLARGE_STRING)pdata;
    cbCapture = pstr->MaximumLength;

    BEGINSENDCAPTURE(CLIENTGETLISTBOXSTRING, 1, cbCapture, FALSE)

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfn = xpfn;

        RESERVEBYTES(cbCapture, pOutput, cbOutput);

        LOCKPWND();
        MAKECALLCAPTURE(CLIENTGETLISTBOXSTRING);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            if (bNotString) {
                /*
                 * This is a 4-byte "object" for ownerdraw listboxes without
                 * the LBS_HASSTRINGS style.
                 */
                OUTSTRUCT((PULONG_PTR)pstr->Buffer, ULONG_PTR);
            } else {
                COPYOUTLPWSTRLIMIT(pstr,
                        pstr->bAnsi ? (int)pstr->MaximumLength :
                        (int)pstr->MaximumLength / sizeof(WCHAR));
            }

            cchRet = pstr->Length;
            if (!pstr->bAnsi)
                cchRet *= sizeof(WCHAR);
            if (!bNotString && retval != LB_ERR && retval > cchRet) {
                RIPMSG2(RIP_WARNING, "GetListBoxString: limit %lX chars to %lX\n",
                        retval, cchRet);
                retval = cchRet;
            }
        ENDCOPYOUT()

    TRACECALLBACK("ClientGetListboxString");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientGetListboxString, CLIENTGETLISTBOXSTRINGMSG)
{
    BYTE abOutput[CALLBACKSTACKLIMIT];

    BEGINRECV(0, NULL, pmsg->cbOutput);
    FIXUPPOINTERS();
    if (pmsg->cbOutput <= CALLBACKSTACKLIMIT)
        CallbackStatus.pOutput = abOutput;
    else
        CallbackStatus.pOutput = pmsg->pOutput;

    retval = (DWORD)_ClientGetListboxString(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPSTR)CallbackStatus.pOutput,
            CALLDATA(xParam),
            CALLDATA(xpfn));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTLOADLIBRARYMSG {
    CAPTUREBUF CaptureBuf;
    UNICODE_STRING strLib;
    BOOL       bWx86KnownDll;
} CLIENTLOADLIBRARYMSG;

#ifdef SENDSIDE
HANDLE ClientLoadLibrary(
    IN PUNICODE_STRING pstrLib,
    IN BOOL bWx86KnownDll)
{
    SETUP(CLIENTLOADLIBRARY)

    BEGINSENDCAPTURE(CLIENTLOADLIBRARY, 1, pstrLib->MaximumLength, TRUE)

        MSGDATA()->bWx86KnownDll = bWx86KnownDll;
        COPYSTRING(strLib);

        MAKECALLCAPTURE(CLIENTLOADLIBRARY);
        CHECKRETURN();

    TRACECALLBACK("ClientLoadLibrary");
    ENDSENDCAPTURE(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientLoadLibrary, CLIENTLOADLIBRARYMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

#if defined(WX86)

    if (CALLDATA(bWx86KnownDll)) {

        //
        // If we haven't done so yet, setup this process to use Wx86
        // to emulate x86 hook procedures. Wx86 once loaded into the client
        // process is NEVER freed. This avoids having to load\free wx86 every
        // time a client hook dll is loaded, and hence every time a
        // sethook\unsethook cycle occurs.
        //


        RtlEnterCriticalSection(&gcsWx86Load);

        if (Wx86LoadCount > 0) {
            Wx86LoadCount++;
            }
        else if (!Wx86LoadCount){
            Wx86LoadCount++;
            hWx86Dll = LoadLibraryExW(L"Wx86.dll", NULL, 0);
            if (hWx86Dll) {
                pfnWx86HookCallBack = (PVOID)GetProcAddress(hWx86Dll,
                                                            "Wx86HookCallBack"
                                                            );

                pfnWx86LoadX86Dll = (PVOID)GetProcAddress(hWx86Dll,
                                                          "Wx86LoadX86Dll"
                                                          );


                pfnWx86FreeX86Dll = (PVOID)GetProcAddress(hWx86Dll,
                                                          "Wx86FreeX86Dll"
                                                          );
                }


            if (!hWx86Dll || !pfnWx86HookCallBack || !pfnWx86LoadX86Dll || !pfnWx86FreeX86Dll) {
                pfnWx86HookCallBack = NULL;
                pfnWx86LoadX86Dll = NULL;
                pfnWx86FreeX86Dll = NULL;
                Wx86LoadCount = -1;
                retval = 0;
                RtlLeaveCriticalSection(&gcsWx86Load);
                goto CLLFail;
                }
            }

           //
           // Wx86LoadCount == -1, means load has previously failed
           // and can't load the x86 dll.
           //
        else {
            retval = 0;
            RtlLeaveCriticalSection(&gcsWx86Load);
            goto CLLFail;
            }

        RtlLeaveCriticalSection(&gcsWx86Load);





        retval = (ULONG_PTR)pfnWx86LoadX86Dll((LPTSTR)FIXUPSTRING(strLib),
                                          LOAD_WITH_ALTERED_SEARCH_PATH
                                          );

        }
    else {
        retval = (ULONG_PTR)LoadLibraryEx((LPTSTR)FIXUPSTRING(strLib),
                                      NULL,
                                      LOAD_WITH_ALTERED_SEARCH_PATH
                                      );
        }

#else

    retval = (ULONG_PTR)LoadLibraryEx((LPTSTR)FIXUPSTRING(strLib), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

#endif


#if defined(WX86)
CLLFail:
#endif

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTFREELIBRARYMSG {
    HANDLE hmod;
} CLIENTFREELIBRARYMSG;

#ifdef SENDSIDE
BOOL ClientFreeLibrary(
    IN HANDLE hmod)
{
    SETUP(CLIENTFREELIBRARY)

    BEGINSEND(CLIENTFREELIBRARY)

        MSGDATA()->hmod = hmod;

        MAKECALL(CLIENTFREELIBRARY);
        CHECKRETURN();

    TRACECALLBACK("ClientFreeLibrary");
    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientFreeLibrary, CLIENTFREELIBRARYMSG)
{
    BEGINRECV(0, NULL, 0);

#if defined(WX86)

    //
    // if the Hook Module is an X86 image, notify Wx86, so that
    // it can clear its hook cache.
    //

    if (RtlImageNtHeader(pmsg->hmod)->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {

        RtlEnterCriticalSection(&gcsWx86Load);

        if (Wx86LoadCount > 0) {
            pfnWx86HookCallBack(0, NULL);
            retval = (DWORD)pfnWx86FreeX86Dll(pmsg->hmod);
            if (!--Wx86LoadCount) {
                FreeLibrary(hWx86Dll);
                }
            }
        else {
            retval = 0; // FALSE
            }

        RtlLeaveCriticalSection(&gcsWx86Load);

        }
    else {
        retval = (DWORD)FreeLibrary(pmsg->hmod);
        }

#else

    retval = (DWORD)FreeLibrary(pmsg->hmod);

#endif

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* xxxClientGetCharsetInfo
*
* 96-06-11  IanJa     Created
\**************************************************************************/

typedef struct _CLIENTGETCHARSETINFOMSG {
    LCID lcid;
    CHARSETINFO cs;
} CLIENTGETCHARSETINFOMSG;

#ifdef SENDSIDE
BOOL xxxClientGetCharsetInfo(
    IN LCID lcid,
    OUT PCHARSETINFO pcs)
{
    SETUP(CLIENTGETCHARSETINFO)

    BEGINSEND(CLIENTGETSCHARSETINFO)

        MSGDATA()->lcid = lcid;

        MAKECALL(CLIENTGETCHARSETINFO);
        CHECKRETURN();

        OUTSTRUCT(pcs, CHARSETINFO);

    TRACECALLBACK("ClientGetCharsetInfo");
    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientGetCharsetInfo, CLIENTGETCHARSETINFOMSG)
{
    BEGINRECV(0, &pmsg->cs, sizeof(CHARSETINFO));

    // TCI_SRCLOCALE = 0x1000
    // Sundown: lcid value should be zero-extended in the TCI_SRCLOCALE case.
    retval = (DWORD)TranslateCharsetInfo((DWORD *)ULongToPtr( pmsg->lcid ), &pmsg->cs, TCI_SRCLOCALE);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ClientFreeDDEHandle
*
* 9-29-91 sanfords     Created.
\**************************************************************************/

typedef struct _CLIENTFREEDDEHANDLEMSG {
    HANDLE hClient;
    DWORD flags;
} CLIENTFREEDDEHANDLEMSG;

#ifdef SENDSIDE
DWORD ClientFreeDDEHandle(
    IN HANDLE hClient,
    IN DWORD flags)
{
    SETUP(CLIENTFREEDDEHANDLE)

    BEGINSEND(CLIENTFREEDDEHANDLE)

        MSGDATA()->hClient = hClient;
        MSGDATA()->flags = flags;

        MAKECALL(CLIENTFREEDDEHANDLE);
        CHECKRETURN();

    TRACECALLBACK("ClientFreeDDEHandle");
    ENDSEND(DWORD, 0);
}
#endif // SENDSIDE



#ifdef RECVSIDE
RECVCALL(ClientFreeDDEHandle, CLIENTFREEDDEHANDLEMSG)
{
    BEGINRECV(0, NULL, 0);
    _ClientFreeDDEHandle(CALLDATA(hClient), CALLDATA(flags));
    ENDRECV();
}
#endif // RECVSIDE




/**************************************************************************\
* ClientGetDDEFlags
*
* This function is used to get a peek at the wStatus flags packed within
* DDE handles - this could either be within the DdePack structure directly
* or within the direct data handle given or referenced via the DdePack
* structure.  flags is used to figure out the right thing to do.
*
* 9-29-91 sanfords     Created.
\**************************************************************************/

typedef struct _CLIENTGETDDEFLAGSMSG {
    HANDLE hClient;
    DWORD flags;
} CLIENTGETDDEFLAGSMSG;

#ifdef SENDSIDE
DWORD ClientGetDDEFlags(
    IN HANDLE hClient,
    IN DWORD flags)
{
    SETUP(CLIENTGETDDEFLAGS)

    BEGINSEND(CLIENTGETDDEFLAGS)

        MSGDATA()->hClient = hClient;
        MSGDATA()->flags = flags;

        MAKECALL(CLIENTGETDDEFLAGS);
        CHECKRETURN();

    TRACECALLBACK("ClientGetDDEFlags");
    ENDSEND(DWORD, 0);
}
#endif // SENDSIDE



#ifdef RECVSIDE
RECVCALL(ClientGetDDEFlags, CLIENTGETDDEFLAGSMSG)
{
    BEGINRECV(0, NULL, 0);
    retval = _ClientGetDDEFlags(CALLDATA(hClient), CALLDATA(flags));
    ENDRECV();
}
#endif // RECVSIDE



/************************************************************************
* ClientCopyDDEIn1
*
* History:
* 10-22-91    sanfords    Created
\***********************************************************************/

typedef struct _CLIENTCOPYDDEIN1MSG {
    HANDLE hClient;      // client side DDE handle - non-0 on initial call
    DWORD flags;
} CLIENTCOPYDDEIN1MSG;

#ifdef SENDSIDE
DWORD xxxClientCopyDDEIn1(
    HANDLE hClient,
    DWORD flags,
    PINTDDEINFO *ppi)
{
    PINTDDEINFO pi;
    INTDDEINFO IntDdeInfo;

    SETUP(CLIENTCOPYDDEIN1)

    BEGINSEND(CLIENTCOPYDDEIN1)

        retval = FAIL_POST;
        *ppi = NULL;
        MSGDATA()->hClient = hClient;
        MSGDATA()->flags = flags;

        MAKECALL(CLIENTCOPYDDEIN1);
        CHECKRETURN();

        if (retval != DO_POST) {
            MSGERROR();
        }

        try {
            OUTSTRUCT(&IntDdeInfo, INTDDEINFO);

            pi = (PINTDDEINFO)UserAllocPool(
                    sizeof(INTDDEINFO) + IntDdeInfo.cbDirect +
                    IntDdeInfo.cbIndirect, TAG_DDE);

            if (pi != NULL) {
                *ppi = pi;
                *pi = IntDdeInfo;

                if (IntDdeInfo.cbDirect) {
                    RtlCopyMemory((PBYTE)pi + sizeof(INTDDEINFO),
                            IntDdeInfo.pDirect,
                            IntDdeInfo.cbDirect);
                }

                if (IntDdeInfo.cbIndirect) {
                    RtlCopyMemory((PBYTE)pi + sizeof(INTDDEINFO) +
                                IntDdeInfo.cbDirect,
                            IntDdeInfo.pIndirect,
                            IntDdeInfo.cbIndirect);
                }

                xxxClientCopyDDEIn2(pi);

            } else {
                retval = FAILNOFREE_POST;
            }
        } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
            if (pi != NULL)
                UserFreePool(pi);
            retval = FAILNOFREE_POST;
            MSGERROR();
        }

    TRACECALLBACK("ClientCopyDDEIn1");
    ENDSEND(DWORD, retval);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientCopyDDEIn1, CLIENTCOPYDDEIN1MSG)
{
    INTDDEINFO IntDdeInfo;

    BEGINRECV(0, &IntDdeInfo, sizeof(INTDDEINFO));

    IntDdeInfo.flags = CALLDATA(flags);
    retval = _ClientCopyDDEIn1(CALLDATA(hClient), &IntDdeInfo);

    ENDRECV();
}
#endif // RECVSIDE


/************************************************************************
* ClientCopyDDEIn2
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/

typedef struct _CLIENTCOPYDDEIN2MSG {
    INTDDEINFO IntDdeInfo;
} CLIENTCOPYDDEIN2MSG;

#ifdef SENDSIDE
BOOL xxxClientCopyDDEIn2(
    PINTDDEINFO pi)
{
    SETUP(CLIENTCOPYDDEIN2)

    BEGINSEND(CLIENTCOPYDDEIN2)

        MSGDATA()->IntDdeInfo = *pi;

        MAKECALL(CLIENTCOPYDDEIN2);
        CHECKRETURN();

    TRACECALLBACK("ClientCopyDDEIn2");
    ENDSEND(BOOL, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientCopyDDEIn2, CLIENTCOPYDDEIN2MSG)
{
    BEGINRECV(0, NULL, 0);

    _ClientCopyDDEIn2(PCALLDATA(IntDdeInfo));

    ENDRECV();
}
#endif // RECVSIDE



/************************************************************************
* ClientCopyDDEOut2
*
* History:
* 10-22-91    sanfords    Created
\***********************************************************************/

typedef struct _CLIENTCOPYDDEOUT2MSG {
    INTDDEINFO IntDdeInfo;
} CLIENTCOPYDDEOUT2MSG;

#ifdef SENDSIDE
DWORD xxxClientCopyDDEOut2(
    PINTDDEINFO pi)
{
    SETUP(CLIENTCOPYDDEOUT2)

    BEGINSEND(CLIENTCOPYDDEOUT2)

        MSGDATA()->IntDdeInfo = *pi;

        MAKECALL(CLIENTCOPYDDEOUT2);
        /*
         * This read is covered by a try/except in ClientCopyDDEOut1.
         */
        pi->hDirect = MSGDATA()->IntDdeInfo.hDirect;
        CHECKRETURN();

    TRACECALLBACK("ClientCopyDDEOut2");
    ENDSEND(DWORD, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientCopyDDEOut2, CLIENTCOPYDDEOUT2MSG)
{
    BEGINRECV(0, NULL, 0);

    retval = _ClientCopyDDEOut2(PCALLDATA(IntDdeInfo));

    ENDRECV();
}
#endif // RECVSIDE

/************************************************************************
* ClientCopyDDEOut1
*
* History:
* 10-22-91    sanfords    Created
\***********************************************************************/

typedef struct _CLIENTCOPYDDEOUT1MSG {
    INTDDEINFO IntDdeInfo;
} CLIENTCOPYDDEOUT1MSG;

#ifdef SENDSIDE
HANDLE xxxClientCopyDDEOut1(
    PINTDDEINFO pi)
{
    INTDDEINFO IntDdeInfo;

    SETUP(CLIENTCOPYDDEOUT1)

    BEGINSEND(CLIENTCOPYDDEOUT1)

        MSGDATA()->IntDdeInfo = *pi;

        MAKECALL(CLIENTCOPYDDEOUT1);
        CHECKRETURN();

        if (retval) {
            try {
                OUTSTRUCT(&IntDdeInfo, INTDDEINFO);

                if (pi->cbDirect) {
                    RtlCopyMemory(IntDdeInfo.pDirect,
                            (PBYTE)pi + sizeof(INTDDEINFO),
                            pi->cbDirect);
                }

                if (pi->cbIndirect) {
                    RtlCopyMemory(IntDdeInfo.pIndirect,
                            (PBYTE)pi + sizeof(INTDDEINFO) + pi->cbDirect,
                            pi->cbIndirect);
                }

                if (IntDdeInfo.hDirect != NULL) {
                    BOOL fSuccess = xxxClientCopyDDEOut2(&IntDdeInfo);
                    if (fSuccess && IntDdeInfo.flags & XS_EXECUTE) {
                        /*
                         * In case value was changed by Execute Fixup.
                         */
                        retval = (ULONG_PTR)IntDdeInfo.hDirect;
                    }
                }
                *pi = IntDdeInfo;
            } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                retval = 0;
                MSGERROR();
            }
        }

    TRACECALLBACK("ClientCopyDDEOut1");
    ENDSEND(HANDLE, 0);
}
#endif // SENDSIDE



#ifdef RECVSIDE
RECVCALL(ClientCopyDDEOut1, CLIENTCOPYDDEOUT1MSG)
{
    BEGINRECV(0, &pmsg->IntDdeInfo, sizeof(INTDDEINFO));

    retval = (ULONG_PTR)_ClientCopyDDEOut1(&pmsg->IntDdeInfo);

    ENDRECV();
}
#endif // RECVSIDE



/**************************************************************************\
* ClientEventCallback
*
* 11-11-91  sanfords    Created
\**************************************************************************/

typedef struct _CLIENTEVENTCALLBACKMSG {
    CAPTUREBUF CaptureBuf;
    PVOID pcii;
    PVOID pep;
} CLIENTEVENTCALLBACKMSG;

#ifdef SENDSIDE
DWORD ClientEventCallback(
    IN PVOID pcii,
    IN PEVENT_PACKET pep)
{
    DWORD cbCapture = pep->cbEventData +
            sizeof(EVENT_PACKET) - sizeof(DWORD);

    SETUP(CLIENTEVENTCALLBACK)

    BEGINSENDCAPTURE(CLIENTEVENTCALLBACK, 1, cbCapture, TRUE)

        MSGDATA()->pcii = pcii;
        COPYBYTES(pep, cbCapture);

        MAKECALLCAPTURE(CLIENTEVENTCALLBACK);
        CHECKRETURN();

    TRACECALLBACK("ClientEventCallback");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientEventCallback, CLIENTEVENTCALLBACKMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    _ClientEventCallback(CALLDATA(pcii), (PEVENT_PACKET)FIXUP(pep));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ClientGetDDEHookData
*
* 11-11-91  sanfords    Created
\**************************************************************************/

typedef struct _CLIENTGETDDEHOOKDATAMSG {
    UINT message;
    LPARAM lParam;
    DDEML_MSG_HOOK_DATA dmhd;
} CLIENTGETDDEHOOKDATAMSG;

#ifdef SENDSIDE
DWORD ClientGetDDEHookData(
    IN UINT message,
    IN LPARAM lParam,
    OUT PDDEML_MSG_HOOK_DATA pdmhd)
{
    SETUP(CLIENTGETDDEHOOKDATA)

    BEGINSEND(CLIENTGETDDEHOOKDATA)

        MSGDATA()->lParam = lParam;
        MSGDATA()->message = message;

        MAKECALL(CLIENTGETDDEHOOKDATA);
        CHECKRETURN();

        OUTSTRUCT(pdmhd, DDEML_MSG_HOOK_DATA);

    TRACECALLBACK("ClientGetDDEHookData");
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientGetDDEHookData, CLIENTGETDDEHOOKDATAMSG)
{
    BEGINRECV(0, &pmsg->dmhd, sizeof(DDEML_MSG_HOOK_DATA));

    _ClientGetDDEHookData(CALLDATA(message), CALLDATA(lParam),
            (PDDEML_MSG_HOOK_DATA)&pmsg->dmhd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
*
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTCHARTOWCHARMSG {
    WORD CodePage;
    WORD wch;
} CLIENTCHARTOWCHARMSG;

#ifdef SENDSIDE
WCHAR xxxClientCharToWchar(
    IN WORD CodePage,
    IN WORD wch)
{
    SETUP(CLIENTCHARTOWCHAR)

    BEGINSEND(CLIENTCHARTOWCHAR)

        MSGDATA()->CodePage = CodePage;
        MSGDATA()->wch = wch;

        MAKECALL(CLIENTCHARTOWCHAR);
        CHECKRETURN();

    TRACECALLBACK("ClientCharToWchar");
    ENDSEND(WCHAR, L'_');
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientCharToWchar, CLIENTCHARTOWCHARMSG)
{
    char ach[2];
    WCHAR wch = L'_';

    BEGINRECV(0, NULL, 0);

    ach[0] = LOBYTE(CALLDATA(wch));
    ach[1] = HIBYTE(CALLDATA(wch));

    MultiByteToWideChar(
            CALLDATA(CodePage),                // CP_THREAD_ACP, 437, 850 etc.
            MB_PRECOMPOSED | MB_USEGLYPHCHARS, // visual map to precomposed
            ach, ach[1] ? 2 : 1,               // source & length
            &wch,                              // destination
            1);                                // max poss. precomposed length

    retval = (DWORD)wch;

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
*
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTFINDMNEMCHARMSG {
    CAPTUREBUF CaptureBuf;
    UNICODE_STRING strSrc;
    WCHAR ch;
    BOOL fFirst;
    BOOL fPrefix;
} CLIENTFINDMNEMCHARMSG;

#ifdef SENDSIDE
int xxxClientFindMnemChar(
    IN PUNICODE_STRING pstrSrc,
    IN WCHAR ch,
    IN BOOL fFirst,
    IN BOOL fPrefix)
{
    SETUP(CLIENTFINDMNEMCHAR)

    BEGINSENDCAPTURE(CLIENTFINDMNEMCHAR, 1, pstrSrc->MaximumLength, TRUE)

        MSGDATA()->ch = ch;
        MSGDATA()->fFirst = fFirst;
        MSGDATA()->fPrefix = fPrefix;
        COPYSTRING(strSrc);

        MAKECALLCAPTURE(CLIENTFINDMNEMCHAR);
        CHECKRETURN();

    TRACECALLBACK("ClientFindMnemChar");
    ENDSENDCAPTURE(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientFindMnemChar, CLIENTFINDMNEMCHARMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = (DWORD)FindMnemChar((LPWSTR)FIXUPSTRING(strSrc),
            CALLDATA(ch), CALLDATA(fFirst), CALLDATA(fPrefix));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* xxxClientPSMTextOut
*
* Called when a client-side LanguagePack (LPK) is installed
*
* 18-Sep-1996 GregoryW  Created
* 11-Dec-1997 SamerA    Calling LPK with user-mode accessible DC
\**************************************************************************/

typedef struct _CLIENTPSMTEXTOUTMSG {
    CAPTUREBUF CaptureBuf;
    UNICODE_STRING strSrc;
    HDC hdc;
    int xLeft;
    int yTop;
    int cch;
    DWORD dwFlags;
} CLIENTPSMTEXTOUTMSG;

#ifdef SENDSIDE
void xxxClientPSMTextOut(
    IN HDC hdc,
    IN int xLeft,
    IN int yTop,
    IN PUNICODE_STRING pstrSrc,
    IN int cch,
    IN DWORD dwFlags)
{
    SETUPDC(CLIENTPSMTEXTOUT)

    /*
     * Make sure this routine is called when a client LanguagePack (LPK)
     * is installed.
     */
    UserAssert(CALL_LPK(PtiCurrentShared()));

    BEGINSENDCAPTUREVOIDDC(CLIENTPSMTEXTOUT, 1, pstrSrc->MaximumLength, TRUE)

#if DBG
    CheckPublicDC ("xxxClientPSMTextOut: Public DC passed to LPK. hdcUse=%lX", hdcUse);
#endif

        MSGDATA()->hdc = hdcUse;
        MSGDATA()->xLeft = xLeft;
        MSGDATA()->yTop = yTop;
        MSGDATA()->cch = cch;
        MSGDATA()->dwFlags = dwFlags;
        COPYSTRING(strSrc);

        MAKECALLCAPTUREDC(CLIENTPSMTEXTOUT);

        CHECKRETURN();

    TRACECALLBACK("ClientPSMTextOut");
    ENDSENDCAPTUREVOIDDC();
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientPSMTextOut, CLIENTPSMTEXTOUTMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    PSMTextOut(CALLDATA(hdc), CALLDATA(xLeft), CALLDATA(yTop),
        (LPWSTR)FIXUPSTRING(strSrc), CALLDATA(cch), CALLDATA(dwFlags));

    retval = 0;
    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* xxxClientLpkDrawTextEx
*
* Called when a client-side LanguagePack (LPK) is installed
*
* 18-Sep-1996 GregoryW  Created
* 11-Dec-1997 SamerA    Calling LPK with user-mode accessible DC
\**************************************************************************/

typedef struct _CLIENTLPKDRAWTEXTEXMSG {
    CAPTUREBUF CaptureBuf;
    UNICODE_STRING strSrc;
    HDC hdc;
    int xLeft;
    int yTop;
    int nCount;
    BOOL fDraw;
    UINT wFormat;
    DRAWTEXTDATA DrawInfo;
    UINT bAction;
    int iCharSet;
} CLIENTLPKDRAWTEXTEXMSG;

#ifdef SENDSIDE
int xxxClientLpkDrawTextEx(
    IN HDC hdc,
    IN int xLeft,
    IN int yTop,
    IN LPCWSTR lpsz,
    IN int nCount,
    IN BOOL fDraw,
    IN UINT wFormat,
    IN LPDRAWTEXTDATA lpDrawInfo,
    IN UINT bAction,
    IN int iCharSet)
{
    SETUPDC(CLIENTLPKDRAWTEXTEX)
    UNICODE_STRING strSrc;
    UNICODE_STRING *pstrSrc   = &strSrc;

    /*
     * Make sure this routine is called when a client LanguagePack (LPK)
     * is installed.
     */
    UserAssert(CALL_LPK(PtiCurrentShared()));


    RtlInitUnicodeString(pstrSrc, lpsz);

    BEGINSENDCAPTUREDC(CLIENTLPKDRAWTEXTEX, 1, pstrSrc->MaximumLength, TRUE)

#if DBG
    CheckPublicDC ("xxxClientLpkDrawTextEx: Public DC passed to LPK. hdcUse=%lX", hdcUse);
#endif

        MSGDATA()->hdc = hdcUse;
        MSGDATA()->xLeft = xLeft;
        MSGDATA()->yTop = yTop;
        MSGDATA()->nCount = nCount;
        MSGDATA()->fDraw = fDraw;
        MSGDATA()->wFormat = wFormat;
        MSGDATA()->DrawInfo = *lpDrawInfo;
        MSGDATA()->bAction = bAction;
        MSGDATA()->iCharSet = iCharSet;
        COPYSTRING(strSrc);

        MAKECALLCAPTUREDC(CLIENTLPKDRAWTEXTEX);

        CHECKRETURN();

    TRACECALLBACK("ClientLpkDrawTextEx");
    ENDSENDCAPTUREDC(int, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientLpkDrawTextEx, CLIENTLPKDRAWTEXTEXMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = (*fpLpkDrawTextEx)(CALLDATA(hdc), CALLDATA(xLeft), CALLDATA(yTop),
        (LPWSTR)FIXUPSTRING(strSrc), CALLDATA(nCount), CALLDATA(fDraw),
        CALLDATA(wFormat), PCALLDATA(DrawInfo), CALLDATA(bAction), CALLDATA(iCharSet));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* xxxClientExtTextOutW
*
* Called when a client-side LanguagePack (LPK) is installed
*
* 26-Jan-1997 GregoryW  Created
* 11-Dec-1997 SamerA    Calling LPK with user-mode accessible DC
\**************************************************************************/

typedef struct _CLIENTEXTTEXTOUTW {
    CAPTUREBUF CaptureBuf;
    UNICODE_STRING strSrc;
    HDC hdc;
    int x;
    int y;
    int flOpts;
    RECT rcl;
    UINT cwc;
    BOOL fNullRect;
} CLIENTEXTTEXTOUTWMSG;

#ifdef SENDSIDE
BOOL xxxClientExtTextOutW(
    IN HDC hdc,
    IN int x,
    IN int y,
    IN int flOpts,
    IN RECT *prcl,
    IN LPCWSTR pwsz,
    IN UINT cwc,
    IN INT *pdx)
{
    SETUPDC(CLIENTEXTTEXTOUTW)
    UNICODE_STRING strSrc;
    UNICODE_STRING *pstrSrc = &strSrc;

    /*
     * Make sure this routine is called when a client LanguagePack (LPK)
     * is installed.
     */
    UserAssert(CALL_LPK(PtiCurrentShared()));


    RtlInitUnicodeString(pstrSrc, pwsz);

    BEGINSENDCAPTUREDC(CLIENTEXTTEXTOUTW, 1, pstrSrc->MaximumLength, TRUE)

#if DBG
    CheckPublicDC ("xxxClientExtTextOutW: Public DC passed to LPK. hdcUse=%lX", hdcUse);
#endif

        MSGDATA()->hdc = hdcUse;
        MSGDATA()->x = x;
        MSGDATA()->y = y;
        MSGDATA()->flOpts = flOpts;
        /* In order not to pass a NULL ptr */
        if( prcl ){
            MSGDATA()->rcl = *prcl;
            MSGDATA()->fNullRect=TRUE;
        }
        else {
            MSGDATA()->fNullRect=FALSE;
        }
        MSGDATA()->cwc = cwc;
        COPYSTRING(strSrc);

        MAKECALLCAPTUREDC(CLIENTEXTTEXTOUTW);

        CHECKRETURN();

    TRACECALLBACK("ClientExtTextOutW");
    ENDSENDCAPTUREDC(BOOL, 0);

    UNREFERENCED_PARAMETER(pdx);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientExtTextOutW, CLIENTEXTTEXTOUTWMSG)
{
    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = ExtTextOutW(CALLDATA(hdc), CALLDATA(x), CALLDATA(y),
        CALLDATA(flOpts), (CALLDATA(fNullRect)) ? PCALLDATA(rcl) : NULL , (LPWSTR)FIXUPSTRING(strSrc),
        CALLDATA(cwc), NULL);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* xxxClientGetTextExtentPointW
*
* Called when a client-side LanguagePack (LPK) is installed
*
* 06-Feb-1997 GregoryW  Created
* 19-Jan-1998 SamerA    EIP_ERROR if a public DC is passed other than hdcGray
\**************************************************************************/

typedef struct _CLIENTGETTEXTEXTENTPOINTW {
    CAPTUREBUF CaptureBuf;
    UNICODE_STRING strSrc;
    HDC hdc;
    int cch;
    SIZE size;
} CLIENTGETTEXTEXTENTPOINTWMSG;

#ifdef SENDSIDE
BOOL xxxClientGetTextExtentPointW(
    IN HDC hdc,
    IN LPCWSTR lpstr,
    IN int cch,
    OUT PSIZE psize)
{
    SETUPDC(CLIENTGETTEXTEXTENTPOINTW)
    UNICODE_STRING strSrc;
    UNICODE_STRING *pstrSrc = &strSrc;

    /*
     * Make sure this routine is called when a client LanguagePack (LPK)
     * is installed.
     */
    UserAssert(CALL_LPK(PtiCurrentShared()));

    RtlInitUnicodeString(pstrSrc, lpstr);

    BEGINSENDCAPTUREDC(CLIENTGETTEXTEXTENTPOINTW, 1, pstrSrc->MaximumLength, TRUE)

#if DBG
    CheckPublicDC ("xxxGetTextExtentPointW: Public DC passed to LPK. hdcUse=%lX", hdcUse);
#endif

        MSGDATA()->hdc = hdcUse;
        MSGDATA()->cch = cch;
        COPYSTRING(strSrc);

        MAKECALLCAPTUREDC(CLIENTGETTEXTEXTENTPOINTW);

        CHECKRETURN();

        OUTSTRUCT(psize, SIZE);

    TRACECALLBACK("ClientGetTextExtentPointW");
    ENDSENDCAPTUREDC(BOOL, 0);

}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientGetTextExtentPointW, CLIENTGETTEXTEXTENTPOINTWMSG)
{
    BEGINRECV(0, &pmsg->size, sizeof(SIZE));
    FIXUPPOINTERS();

    retval = GetTextExtentPointW(CALLDATA(hdc), (LPWSTR)FIXUPSTRING(strSrc),
        CALLDATA(cch), PCALLDATA(size));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
*
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTADDFONTRESOURCEWMSG {
    CAPTUREBUF CaptureBuf;
    UNICODE_STRING strSrc;
    DWORD dwFlags;
    DESIGNVECTOR   dv;
} CLIENTADDFONTRESOURCEWMSG;

#ifdef SENDSIDE
int xxxClientAddFontResourceW(
    IN PUNICODE_STRING pstrSrc,
    IN DWORD dwFlags,
    IN DESIGNVECTOR *pdv)
{
    SETUP(CLIENTADDFONTRESOURCEW)

    BEGINSENDCAPTURE(CLIENTADDFONTRESOURCEW, 1, pstrSrc->MaximumLength, TRUE)

        COPYSTRING(strSrc);
        MSGDATA()->dwFlags = dwFlags;

        if (pdv && pdv->dvNumAxes) {
            MSGDATA()->dv = *pdv;
        } else {
            MSGDATA()->dv.dvNumAxes = 0;
        }

        MAKECALLCAPTURE(CLIENTADDFONTRESOURCEW);
        CHECKRETURN();

    TRACECALLBACK("ClientAddFontResourceW");
    ENDSENDCAPTURE(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE


RECVCALL(ClientAddFontResourceW, CLIENTADDFONTRESOURCEWMSG)
{
    DWORD AddFont(LPWSTR, DWORD, DESIGNVECTOR*);

    BEGINRECV(0, NULL, 0);
    FIXUPPOINTERS();

    retval = GdiAddFontResourceW((LPWSTR)FIXUPSTRING(strSrc),
                                  CALLDATA(dwFlags), CALLDATA(dv).dvNumAxes ? &CALLDATA(dv) : NULL);

    ENDRECV();
}
#endif // RECVSIDE



/******************************Public*Routine******************************\
*
* FontSweep()
*
* History:
*  23-Oct-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



#ifdef SENDSIDE
VOID ClientFontSweep(VOID)
{
    PVOID p;
    ULONG cb;

    LeaveCrit();
    KeUserModeCallback(
        FI_CLIENTFONTSWEEP,
        NULL,
        0,
        &p,
        &cb);
    EnterCrit();
    return;
}
#endif // SENDSIDE

#ifdef RECVSIDE

DWORD __ClientFontSweep(
    PVOID p)
{
    UNREFERENCED_PARAMETER(p);
    vFontSweep();
    return NtCallbackReturn(NULL, 0, STATUS_SUCCESS);
}
#endif // RECVSIDE


/******************************Public*Routine******************************\
*
* VOID ClientLoadLocalT1Fonts(VOID)
* very similar to above, only done for t1 fonts
*
* History:
*  25-Apr-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



#ifdef SENDSIDE
VOID ClientLoadLocalT1Fonts(VOID)
{
    PVOID p;
    ULONG cb;

    LeaveCrit();
    KeUserModeCallback(
        FI_CLIENTLOADLOCALT1FONTS,
        NULL,
        0,
        &p,
        &cb);
    EnterCrit();
    return;
}
#endif // SENDSIDE

#ifdef RECVSIDE



DWORD __ClientLoadLocalT1Fonts(
    PVOID p)
{
    UNREFERENCED_PARAMETER(p);
    vLoadLocalT1Fonts();
    return NtCallbackReturn(NULL, 0, STATUS_SUCCESS);
}
#endif // RECVSIDE



/******************************Public*Routine******************************\
*
* VOID ClientLoadRemoteT1Fonts(VOID)
* very similar to above, only done for t1 fonts
*
* History:
*  25-Apr-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



#ifdef SENDSIDE
VOID ClientLoadRemoteT1Fonts(VOID)
{
    PVOID p;
    ULONG cb;

    LeaveCrit();
    KeUserModeCallback(
        FI_CLIENTLOADREMOTET1FONTS,
        NULL,
        0,
        &p,
        &cb);
    EnterCrit();
    return;
}
#endif // SENDSIDE

#ifdef RECVSIDE



DWORD __ClientLoadRemoteT1Fonts(
    PVOID p)
{
    UNREFERENCED_PARAMETER(p);
    vLoadRemoteT1Fonts();
    return NtCallbackReturn(NULL, 0, STATUS_SUCCESS);
}
#endif // RECVSIDE

/**************************************************************************\
* pppUserModeCallback
*
* Same as xxxUserModeCallback except not leaving/re-entering critical section
*
* 12/9/97 LingyunW     Copied from xxxUserModeCallback
\**************************************************************************/
#ifdef SENDSIDE
NTSTATUS pppUserModeCallback (ULONG uApi, PVOID pIn, ULONG cbIn, PVOID pOut, ULONG cbOut)
{
    NTSTATUS Status;
    PVOID pLocalOut;
    ULONG cbLocalOut;

    /*
     * Call the client
     */
    Status = KeUserModeCallback(uApi, pIn, cbIn, &pLocalOut, &cbLocalOut);

    /*
     * If it failed, bail
     */
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    /*
     * If we didn't get the right amount of data, fail.
     */
    if (cbLocalOut != cbOut) {
        RIPMSG3(RIP_WARNING, "pppUserModeCallback: uAPi: %#lx cbOut: %#lx cbLocalOut: %#lx",
                uApi, cbOut, cbLocalOut);
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * If we were expecting some data, copy it.
     */
    if (cbOut != 0) {
        try {
            ProbeForRead(pLocalOut, cbLocalOut, sizeof(DWORD));
            RtlCopyMemory(pOut, pLocalOut, cbLocalOut);
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            RIPMSG2(RIP_WARNING, "pppUserModeCallback: uAPi: %#lx Exception: %#lx", uApi, GetExceptionCode());
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    return Status;
}
#endif // SENDSIDE

/******************************Public*Routine******************************\
* ClientPrinterThunk
*
* Callback used as the kernel-to-user transport layer.
*
* Note: User critical section is not held by the caller.
*
* History:
*  22-Jun-1997 -by- Gilman Wong [gilmanw]
*  11/13/97 -by- Lingyun Wang [lingyunw] clean up
*
* Wrote it.
\**************************************************************************/

#define CLIENTPRINTERTHUNKMSG UMTHDR

#ifdef SENDSIDE
DWORD ClientPrinterThunk(PVOID pvIn, ULONG cjIn, PVOID pvOut, ULONG cjOut)
{
    NTSTATUS Status;

    /*
     * (Temporarly..) we return failure if we are holding USERK's crit section
     */
    if (ExIsResourceAcquiredExclusiveLite(gpresUser)
            || (ExIsResourceAcquiredSharedLite(gpresUser) != 0)) {
        RIPMSG0(RIP_ERROR, "ClientPrinterThunk: Holding USERK critical section!");
        return 0xffffffff;
    }

    /*
     * The pvIn buffer must have at least a CLIENTPRINTERTHUNK header.
     */
    UserAssertMsg1(cjIn >= sizeof(CLIENTPRINTERTHUNKMSG), "ClientPrinterThunk: incorrect cjIn:%#lx", cjIn);

    /*
     * Set the private cjOut.  The receive-side uses this to allocate
     *  a return buffer.
     */
    ((CLIENTPRINTERTHUNKMSG *) pvIn)->ulReserved1      = cjOut;
    ((CLIENTPRINTERTHUNKMSG *) pvIn)->ulReserved2 = 0;


    /*
     * Do the callback.
     */
    Status = pppUserModeCallback(FI_CLIENTPRINTERTHUNK, pvIn, cjIn, pvOut, cjOut);

    return (NT_SUCCESS(Status) ? 0 : 0xFFFFFFFF);
}
#endif // SENDSIDE

#ifdef RECVSIDE
DWORD __ClientPrinterThunk(CLIENTPRINTERTHUNKMSG *pMsg)
{
    PVOID pv;
    ULONG aul[526];
    NTSTATUS Status;

    /*
     * Check that the local buffer is big enough.
     */
    if (pMsg->ulReserved1 <= sizeof(aul)) {
        pv = (PVOID) aul;
        /*
         * Call GDI to process command.
         */
        if (GdiPrinterThunk((UMTHDR *) pMsg, pv, pMsg->ulReserved1) != GPT_ERROR) {
            Status = STATUS_SUCCESS;
        } else {
            RIPMSG0(RIP_WARNING, "ClientPrinterThunk failed");
            Status = STATUS_UNSUCCESSFUL;
        }
    } else {
        RIPMSG0(RIP_WARNING, "ClientPrinterThunk: buffer too big!");
        Status = STATUS_NO_MEMORY;
    }


    /*
     * Return to kernel.
     */
    if (NT_SUCCESS(Status)) {
        return UserCallbackReturn(pv, pMsg->ulReserved1, Status);
    } else {
        return UserCallbackReturn(NULL, 0, Status);
    }
}
#endif // RECVSIDE

/**************************************************************************\
*
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
VOID ClientNoMemoryPopup(VOID)
{
    PVOID p;
    ULONG cb;

    LeaveCrit();
    KeUserModeCallback(
        FI_CLIENTNOMEMORYPOPUP,
        NULL,
        0,
        &p,
        &cb);
    EnterCrit();
    return;
}
#endif // SENDSIDE

#ifdef RECVSIDE

DWORD __ClientNoMemoryPopup(
    PVOID p)
{
    WCHAR szNoMem[200];

    UNREFERENCED_PARAMETER(p);

    if (LoadStringW(hmodUser, STR_NOMEMBITMAP, szNoMem,
            sizeof(szNoMem) / sizeof(WCHAR))) {
        MessageBoxW(GetActiveWindow(), szNoMem, NULL, MB_OK);
    }

    return NtCallbackReturn(NULL, 0, STATUS_SUCCESS);
}
#endif // RECVSIDE

/**************************************************************************\
* ClientThreadSetup
*
* Callback to the client to perform thread initialization.
*
* 04-07-95 JimA         Created.
\**************************************************************************/

#ifdef SENDSIDE
NTSTATUS ClientThreadSetup(VOID)
{
    PVOID p;
    ULONG cb;
    NTSTATUS Status;

    LeaveCrit();
    Status = KeUserModeCallback(
        FI_CLIENTTHREADSETUP,
        NULL,
        0,
        &p,
        &cb);
    EnterCrit();
    return Status;
}
#endif // SENDSIDE

#ifdef RECVSIDE
DWORD __ClientThreadSetup(
    PVOID p)
{
    BOOL fSuccess;
    BOOL ClientThreadSetup(VOID);

    UNREFERENCED_PARAMETER(p);

    fSuccess = ClientThreadSetup();
    return NtCallbackReturn(NULL, 0,
            fSuccess ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}
#endif // RECVSIDE

/**************************************************************************\
* ClientDeliverUserApc
*
* Callback to the client to handle a user APC.  This is needed to
* ensure that a thread will exit promptly when terminated.
*
* 08-12-95 JimA         Created.
\**************************************************************************/

#ifdef SENDSIDE
VOID ClientDeliverUserApc(VOID)
{
    PVOID p;
    ULONG cb;

    LeaveCrit();
    KeUserModeCallback(
        FI_CLIENTDELIVERUSERAPC,
        NULL,
        0,
        &p,
        &cb);
    EnterCrit();
}
#endif // SENDSIDE

#ifdef RECVSIDE
DWORD __ClientDeliverUserApc(
    PVOID p)
{
    UNREFERENCED_PARAMETER(p);
    return NtCallbackReturn(NULL, 0, STATUS_SUCCESS);
}
#endif // RECVSIDE


/**************************************************************************\
* ClientImmLoadLayout
*
* 29-Jan-1996 wkwok   Created
\**************************************************************************/

typedef struct _CLIENTIMMLOADLAYOUTMSG {
    HKL hKL;
} CLIENTIMMLOADLAYOUTMSG;

#ifdef SENDSIDE
BOOL ClientImmLoadLayout(
    IN HKL hKL,
    OUT PIMEINFOEX piiex)
{
    SETUP(CLIENTIMMLOADLAYOUT)

    BEGINSEND(CLIENTIMMLOADLAYOUT)

        MSGDATA()->hKL = hKL;

        MAKECALL(CLIENTIMMLOADLAYOUT);
        CHECKRETURN();

        if (retval)
            OUTSTRUCT(piiex, IMEINFOEX);

    TRACECALLBACK("ClientImmLoadLayout");
    ENDSEND(BOOL, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientImmLoadLayout, CLIENTIMMLOADLAYOUTMSG)
{
    IMEINFOEX iiex;

    BEGINRECV(0, &iiex, sizeof(iiex));

    retval = fpImmLoadLayout(CALLDATA(hKL), &iiex);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ClientImmProcessKey
*
* 03-Mar-1996 TakaoK   Created
\**************************************************************************/

typedef struct _CLIENTIMMPROCESSKEYMSG {
    HWND hWnd;
    HKL  hkl;
    UINT uVKey;
    LPARAM lParam;
    DWORD dwHotKeyID;
} CLIENTIMMPROCESSKEYMSG;

#ifdef SENDSIDE
DWORD ClientImmProcessKey(
    IN HWND hWnd,
    IN HKL  hkl,
    IN UINT uVKey,
    IN LPARAM lParam,
    IN DWORD dwHotKeyID)
{
    SETUP(CLIENTIMMPROCESSKEY)

    UserAssert(IS_IME_ENABLED());

    BEGINSEND(CLIENTIMMPROCESSKEY)

        MSGDATA()->hWnd = hWnd,
        MSGDATA()->hkl = hkl;
        MSGDATA()->uVKey = uVKey;
        MSGDATA()->lParam = lParam;
        MSGDATA()->dwHotKeyID = dwHotKeyID;

        MAKECALL(CLIENTIMMPROCESSKEY);
        CHECKRETURN();

    TRACECALLBACK("ClientImmProcessKey");
    ENDSEND(DWORD, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientImmProcessKey, CLIENTIMMPROCESSKEYMSG)
{
    BEGINRECV(0, NULL, 0);

    retval = fpImmProcessKey(CALLDATA(hWnd),
                CALLDATA(hkl),
                CALLDATA(uVKey),
                CALLDATA(lParam),
                CALLDATA(dwHotKeyID));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnIMECONTROL
*
* 22-Apr-1996 wkwok    Created
\**************************************************************************/

typedef struct _FNIMECONTROL {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    union {
        PCANDIDATEFORM pCandForm;
        PCOMPOSITIONFORM pCompForm;
        PLOGFONTA pLogFontA;
        PLOGFONTW pLogFontW;
        PSOFTKBDDATA pSoftKbdData;
        LPARAM lParam;
    } u;
    ULONG_PTR xParam;
    PROC xpfnProc;
    PBYTE pOutput;
    DWORD cbOutput;
} FNIMECONTROLMSG;

#ifdef SENDSIDE
void CopyLogFontAtoW(
    PLOGFONTW pdest,
    PLOGFONTA psrc)
{
    LPSTR lpstrFont = (LPSTR)(&psrc->lfFaceName);
    LPWSTR lpstrFontW = (LPWSTR)(&pdest->lfFaceName);

    memcpy((LPBYTE)pdest, psrc, sizeof(LOGFONTA) - LF_FACESIZE);
    memset(pdest->lfFaceName, 0, LF_FACESIZE * sizeof(WCHAR));
    MBToWCS(lpstrFont, -1, &lpstrFontW, LF_FACESIZE, FALSE);
}

SMESSAGECALL(IMECONTROL)
{
    DWORD cCapture, cbCapture;

    SETUPPWND(FNIMECONTROL)

    switch (wParam) {
        case IMC_GETCANDIDATEPOS:
        case IMC_SETCANDIDATEPOS:
            cCapture  = 1;
            cbCapture = sizeof(CANDIDATEFORM);
            break;

        case IMC_GETCOMPOSITIONWINDOW:
        case IMC_SETCOMPOSITIONWINDOW:
            cCapture  = 1;
            cbCapture = sizeof(COMPOSITIONFORM);
            break;

        case IMC_GETCOMPOSITIONFONT:
        case IMC_SETCOMPOSITIONFONT:
        case IMC_GETSOFTKBDFONT:
            cCapture  = 1;
            cbCapture = (dwSCMSFlags & SCMS_FLAGS_ANSI)
                      ? sizeof(LOGFONTA) : sizeof(LOGFONTW) ;
            break;

        case IMC_SETSOFTKBDDATA:
            cCapture  = 1;
            cbCapture = FIELD_OFFSET(SOFTKBDDATA, wCode[0])
                      + ((PSOFTKBDDATA)lParam)->uCount * sizeof(WORD) * 256;

            break;

        default:
            cCapture  = 0;
            cbCapture = 0;
            break;
    }

    BEGINSENDCAPTURE(FNIMECONTROL, cCapture, cbCapture, TRUE);

        MSGDATA()->pwnd     = pwndClient;
        MSGDATA()->msg      = msg;
        MSGDATA()->wParam   = wParam;
        MSGDATA()->u.lParam = lParam;
        MSGDATA()->xParam   = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();

        switch (wParam) {

        case IMC_GETCANDIDATEPOS:
        case IMC_GETCOMPOSITIONWINDOW:
        case IMC_GETCOMPOSITIONFONT:
        case IMC_GETSOFTKBDFONT:

            RESERVEBYTES(cbCapture, pOutput, cbOutput);
            MAKECALLCAPTURE(FNIMECONTROL);
            UNLOCKPWND();
            CHECKRETURN();

            BEGINCOPYOUT()
                try {
                    ProbeForRead(pcbs->pOutput, pcbs->cbOutput, sizeof(DWORD));
                    switch (wParam) {
                    case IMC_GETCANDIDATEPOS:
                    case IMC_GETCOMPOSITIONWINDOW:
                        memcpy((LPBYTE)lParam, pcbs->pOutput, cbCapture);
                        break;

                    case IMC_GETCOMPOSITIONFONT:
                    case IMC_GETSOFTKBDFONT:
                        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
                            CopyLogFontAtoW((PLOGFONTW)lParam, (PLOGFONTA)pcbs->pOutput);
                        }
                        else {
                            memcpy((LPBYTE)lParam, pcbs->pOutput, cbCapture);
                        }
                        break;
                    }
                } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                    MSGERROR();
                }
            ENDCOPYOUT()

            break;

        case IMC_SETCANDIDATEPOS:
        case IMC_SETCOMPOSITIONWINDOW:
        case IMC_SETCOMPOSITIONFONT:
        case IMC_SETSOFTKBDDATA:
            if (wParam == IMC_SETCANDIDATEPOS) {
                PCANDIDATEFORM pCandForm = (PCANDIDATEFORM)lParam;
                LARGECOPYBYTES2(pCandForm, sizeof(CANDIDATEFORM), u.pCandForm);
            }
            else if (wParam == IMC_SETCOMPOSITIONWINDOW) {
                PCOMPOSITIONFORM pCompForm = (PCOMPOSITIONFORM)lParam;
                LARGECOPYBYTES2(pCompForm, sizeof(COMPOSITIONFORM), u.pCompForm);
            }
            else if (wParam == IMC_SETCOMPOSITIONFONT) {
                if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
                    LOGFONTA LogFontA;
                    LPSTR  lpstrFontA = LogFontA.lfFaceName;
                    LPWSTR lpstrFontW = ((PLOGFONTW)lParam)->lfFaceName;

                    memcpy(&LogFontA, (PBYTE)lParam, sizeof(LOGFONTA)-LF_FACESIZE);
                    memset(lpstrFontA, 0, LF_FACESIZE * sizeof(CHAR));
                    WCSToMB(lpstrFontW, -1, &lpstrFontA, LF_FACESIZE, FALSE);
                    LARGECOPYBYTES2(&LogFontA, sizeof(LOGFONTA), u.pLogFontA);
                }
                else {
                    PLOGFONTW pLogFontW = (PLOGFONTW)lParam;
                    LARGECOPYBYTES2(pLogFontW, sizeof(LOGFONTW), u.pLogFontW);
                }
            }
            else if (wParam == IMC_SETSOFTKBDDATA) {
                PSOFTKBDDATA pSoftKbdData;

                if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
                    PWORD pCodeA;
                    PWSTR pCodeW;
                    CHAR  ch[2];
                    PSTR  pch = (PSTR)&ch;
                    UINT  i;

                    pSoftKbdData = (PSOFTKBDDATA)UserAllocPool(cbCapture, TAG_IME);
                    if (pSoftKbdData == NULL)
                        MSGERROR();

                    pCodeA = &pSoftKbdData->wCode[0][0];
                    pCodeW = (PWSTR)&((PSOFTKBDDATA)lParam)->wCode[0][0];

                    pSoftKbdData->uCount = ((PSOFTKBDDATA)lParam)->uCount;

                    i = pSoftKbdData->uCount * 256;

                    while (i--) {
                        pch[1] = '\0';
                        WCSToMBEx(THREAD_CODEPAGE(), pCodeW, 1, &pch, 2, FALSE);
                        if (pch[1]) {
                            *pCodeA = MAKEWORD(pch[1], pch[0]);
                        } else {
                            *pCodeA = MAKEWORD(pch[0], 0);
                        }
                        pCodeA++; pCodeW++;
                    }

                    LARGECOPYBYTES2(pSoftKbdData, cbCapture, u.pSoftKbdData);

                    UserFreePool(pSoftKbdData);
                }
                else {
                    pSoftKbdData = (PSOFTKBDDATA)lParam;
                    LARGECOPYBYTES2(pSoftKbdData, cbCapture, u.pSoftKbdData);
                }
            }

            /*
             * Fall thur.
             */

        default:
            MAKECALLCAPTURE(FNIMECONTROL);
            UNLOCKPWND();
            CHECKRETURN();
            break;
        }


    TRACECALLBACKMSG("SfnINSTRINGNULL");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnIMECONTROL, FNIMECONTROLMSG)
{
    BYTE abOutput[CALLBACKSTACKLIMIT];
    LPARAM lParam;

    BEGINRECV(0, NULL, pmsg->cbOutput);
    FIXUPPOINTERS();
    if (pmsg->cbOutput <= CALLBACKSTACKLIMIT)
        CallbackStatus.pOutput = abOutput;
    else
        CallbackStatus.pOutput = pmsg->pOutput;

    switch (CALLDATA(wParam)) {
        case IMC_GETCANDIDATEPOS:
        case IMC_GETCOMPOSITIONWINDOW:
        case IMC_GETCOMPOSITIONFONT:
        case IMC_GETSOFTKBDFONT:
            lParam = (LPARAM)CallbackStatus.pOutput;
            break;

        case IMC_SETCANDIDATEPOS:
        case IMC_SETCOMPOSITIONWINDOW:
        case IMC_SETCOMPOSITIONFONT:
        case IMC_SETSOFTKBDDATA:
            lParam = FIRSTFIXUP(u.lParam);
            break;

       default:
            lParam = CALLDATA(u.lParam);
            break;
    }

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            lParam,
            CALLDATA(xParam));


    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnIMEREQUEST
*
* 22-Apr-1996     Created
\**************************************************************************/

#ifdef LATER
typedef struct _FNIMEREQUEST {
    CAPTUREBUF CaptureBuf;
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    union {
        LPCANDIDATEFORM         pCandidateForm;
        LPLOGFONTA              pLogFontA;
        LPLOGFONTW              pLogFontW;
        LPCOMPOSITIONFORM       pCompositionForm;
        LPRECONVERTSTRING       pReconvertString;
        LPPrivateIMECHARPOSITION pImeCharPosition;
        LPARAM                  lParam;
    } u;
    ULONG_PTR xParam;
    PROC xpfnProc;
    PBYTE pOutput;
    DWORD cbOutput;
    BOOL fAnsi;
} FNIMEREQUESTMSG;

#ifdef SENDSIDE

SMESSAGECALL(IMEREQUEST)
{
    DWORD cCapture, cbCapture;

    SETUPPWND(FNIMEREQUEST)

    //
    // IMEREQUEST assumes the callback is within the thread
    // (see MESSAGECALL(IMEREQUEST) in kernel/ntstubs.c.)
    //
    // All the data pointed by lParam should point the valid
    // client side address. Thus all the validation and copy
    // (if needed) will be done in the receiver side.
    //
    UserAssert(psms == NULL || psms->ptiSender == psms->ptiReceiver);

    switch (wParam) {
    case IMR_CANDIDATEWINDOW:
        cCapture  = 1;
        cbCapture = sizeof(CANDIDATEFORM);
        break;

    case IMR_COMPOSITIONWINDOW:
        cCapture = 1;
        cbCapture = sizeof(COMPOSITIONFORM);
        break;

    case IMR_CONFIRMRECONVERTSTRING:
    case IMR_RECONVERTSTRING:
    case IMR_DOCUMENTFEED:
    case IMR_QUERYCHARPOSITION:
        cCapture = 0;
        cbCapture = 0;
        break;

    case IMR_COMPOSITIONFONT:   // only the exception to the rule above.
        cCapture = 1;
        cbCapture = (dwSCMSFlags & SCMS_FLAGS_ANSI) ? sizeof(LOGFONTA) : sizeof(LOGFONTW);
        break;

    default:
        UserAssert(FALSE);
        cCapture  = 0;
        cbCapture = 0;
        break;
    }

    BEGINSENDCAPTURE(FNIMEREQUEST, cCapture, cbCapture, TRUE);

        MSGDATA()->pwnd     = pwndClient;
        MSGDATA()->msg      = msg;
        MSGDATA()->wParam   = wParam;
        MSGDATA()->u.lParam = lParam;
        MSGDATA()->xParam   = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->fAnsi    = (dwSCMSFlags & SCMS_FLAGS_ANSI);

        LOCKPWND();

        //
        // Preparation
        //

        switch (wParam) {
        case IMR_COMPOSITIONFONT:
            RESERVEBYTES(cbCapture, pOutput, cbOutput);
            break;
        }

        MAKECALLCAPTURE(FNIMEREQUEST);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            try {
                switch (wParam) {
                case IMR_COMPOSITIONFONT:
                    ProbeForRead(pcbs->pOutput, pcbs->cbOutput, sizeof(DWORD));
                    if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
                        CopyLogFontAtoW((PLOGFONTW)lParam, (PLOGFONTA)pcbs->pOutput);
                    }
                    else {
                        memcpy((LPBYTE)lParam, pcbs->pOutput, cbCapture);
                    }
                    break;
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                MSGERROR();
            }
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnIMEREQUEST");
    ENDSENDCAPTURE(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnIMEREQUEST, FNIMEREQUESTMSG)
{
    BYTE abOutput[CALLBACKSTACKLIMIT];
    LPVOID pvNew = NULL;
    LPARAM lParam;

    BEGINRECV(0, NULL, pmsg->cbOutput);
    FIXUPPOINTERS();
    if (pmsg->cbOutput <= CALLBACKSTACKLIMIT)
        CallbackStatus.pOutput = abOutput;
    else
        CallbackStatus.pOutput = pmsg->pOutput;

    lParam = CALLDATA(u.lParam);

    switch (CALLDATA(wParam)) {
    case IMR_COMPOSITIONWINDOW:
    case IMR_CANDIDATEWINDOW:
//        lParam = CALLDATA(lParam);
        break;

    case IMR_COMPOSITIONFONT:
        lParam = (LPARAM)CallbackStatus.pOutput;
        break;

    case IMR_QUERYCHARPOSITION:
        if (CALLDATA(fAnsi)) {
            LPPrivateIMECHARPOSITION lpCharPos;

            pvNew = UserLocalAlloc(0, sizeof(PrivateIMECHARPOSITION));
            if (pvNew == NULL) {
                goto error_return;
            }
            lpCharPos = pvNew;
            *lpCharPos = *CALLDATA(u.pImeCharPosition);
            lpCharPos->dwCharPos = lpCharPos->dwCharPositionA;
        }
        break;

    case IMR_RECONVERTSTRING:
    case IMR_CONFIRMRECONVERTSTRING:
    case IMR_DOCUMENTFEED:
        // Real W/A conversion may be needed.
        if (CALLDATA(fAnsi) && lParam) {
            PRECONVERTSTRING Source = (LPRECONVERTSTRING)lParam;
            // Do conversion.
            DWORD dwNewSize = ImmGetReconvertTotalSize(((LPRECONVERTSTRING)lParam)->dwSize, FROM_IME, TRUE);
            if (dwNewSize == 0) {
                goto error_return;
            }

            pvNew = UserLocalAlloc(0, dwNewSize);
            if (pvNew == NULL) {
                goto error_return;
            }
            lParam = (LPARAM)pvNew;

            #define lpReconv ((LPRECONVERTSTRING)lParam)
            // setup the information in the allocated structure
            lpReconv->dwVersion = 0;
            lpReconv->dwSize = dwNewSize;
            if (CALLDATA(wParam) == IMR_CONFIRMRECONVERTSTRING) {
                ImmReconversionWorker(lpReconv, (LPRECONVERTSTRING)lParam, TRUE, CP_ACP);
            }
        }
        break;

   default:
        lParam = CALLDATA(u.lParam);
        break;
    }

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            lParam,
            CALLDATA(xParam));

    switch (CALLDATA(wParam)) {
    case IMR_RECONVERTSTRING:
    case IMR_DOCUMENTFEED:
        if (CALLDATA(fAnsi)) {
            retval = ImmGetReconvertTotalSize((DWORD)retval, FROM_APP, TRUE);
            if (lParam) {
                retval = ImmReconversionWorker((LPRECONVERTSTRING)CALLDATA(u.lParam), (LPRECONVERTSTRING)pvNew, FALSE, CP_ACP);
            }
        }
        break;
    }

    if (pvNew) {
        UserLocalFree(pvNew);
    }
error_return:
    ENDRECV();
}

#undef lpReconv

#endif // RECVSIDE

#endif

/**************************************************************************\
* fnGETDBCSTEXTLENGTHS (DBCS-aware Version)
*
* Gets the Unicode & ANSI lengths
* Internally, lParam pints to the ANSI length in bytes and the return value
* is the Unicode length in bytes.  However, the public definition is maintained
* on the  client side, where lParam is not used and either ANSI or Unicode is
* returned.
*
* 14-Mar-1996 HideyukN  Created
\**************************************************************************/

#if (WM_GETTEXTLENGTH - WM_GETTEXT) != 1
#error "WM_GETTEXT Messages no longer 1 apart. Error in code."
#endif
#if (LB_GETTEXTLEN - LB_GETTEXT) != 1
#error "LB_GETTEXT Messages no longer 1 apart. Error in code."
#endif
#if (CB_GETLBTEXTLEN - CB_GETLBTEXT) != 1
#error "CB_GETLBTEXT Messages no longer 1 apart. Error in code."
#endif

typedef struct _FNGETDBCSTEXTLENGTHSMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNGETDBCSTEXTLENGTHSMSG;

#ifdef SENDSIDE
SMESSAGECALL(GETDBCSTEXTLENGTHS)
{
    BOOL fAnsiSender   = !!(BOOL)lParam;
    BOOL fAnsiReceiver = ((dwSCMSFlags & SCMS_FLAGS_ANSI) != 0);
    LPVOID pfnSavedWndProc = pwnd->lpfnWndProc;

    SETUPPWND(FNGETDBCSTEXTLENGTHS)

    BEGINSEND(FNGETDBCSTEXTLENGTHS)

    UserAssert((fAnsiReceiver & 1) == fAnsiReceiver && (fAnsiSender & 1) == fAnsiSender);

    MSGDATA()->pwnd = pwndClient;
    MSGDATA()->msg = msg;
    MSGDATA()->wParam = wParam;
    MSGDATA()->xParam = xParam;
    MSGDATA()->xpfnProc = xpfnProc;

    LOCKPWND();
    MAKECALL(FNGETTEXTLENGTHS);
    UNLOCKPWND();
    CHECKRETURN1();

    /*
     * ANSI client wndproc returns us cbANSI.  We want cchUnicode,
     * so we guess cchUnicode = cbANSI. (It may be less if
     * multi-byte characters are involved, but it will never be more).
     * Save cbANSI in *lParam in case the server ultimately returns
     * the length to an ANSI caller.
     *
     * Unicode client wndproc returns us cchUnicode.  If we want to know
     * cbANSI, we must guess how many 'ANSI' chars we would need.
     * We guess cbANSI = cchUnicode * 2. (It may be this much if all
     * 'ANSI' characters are multi-byte, but it will never be more).
     *
     * Return cchUnicode (server code is all Unicode internally).
     * Put cbANSI in *lParam to be passed along within the server in case
     * we ultimately need to return it to the client.
     *
     * NOTE: this will sometimes cause text lengths to be misreported
     * up to twice the real length, but that is expected to be harmless.
     * This will only * happen if an app sends WM_GETcode TEXTLENGTH to a
     * window with an ANSI client-side wndproc, or a ANSI WM_GETTEXTLENGTH
     * is sent to a Unicode client-side wndproc.
     */

    BEGINCOPYOUT()

        //
        // retval can be [CB|LB]_ERR (-1) or [CB|LB]_ERRSPACE (-2)
        // then, it should be grater then zero. otherwise we can handle
        // it as error, or zero length string.
        //
        if ((LONG)retval > 0) {

            //
            // Check we need to Ansi <-> Unicode conversion.
            //
            if (fAnsiSender != fAnsiReceiver) {
                if (pwnd->lpfnWndProc != pfnSavedWndProc) {
                    // The window procedure is changed during the first callback.
                    // Let's take a guess for the worst case.
                    RIPMSG1(RIP_WARNING, "GETTEXTLENGTHS(pwnd=%x): The subclass status of winproc changed during 1st callback.",
                            pwnd);
                    retval *= 2;
                }
                else {
                    BOOL bNotString = FALSE; // default is string....

                    if (msg != WM_GETTEXTLENGTH) {
                        DWORD dw;

                        if (!RevalidateHwnd(HW(pwnd))) {
                            MSGERROR1();
                        }

                        //
                        // Get window style.
                        //
                        dw = pwnd->style;

                        if (msg == LB_GETTEXTLEN) {
                            //
                            // See if the control is ownerdraw and does not have the LBS_HASSTRINGS
                            // style.
                            //
                            bNotString =  (!(dw & LBS_HASSTRINGS) &&
                                            (dw & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)));
                        } else if (msg == CB_GETLBTEXTLEN) {
                            //
                            // See if the control is ownerdraw and does not have the CBS_HASSTRINGS
                            // style.
                            //
                            bNotString = (!(dw & CBS_HASSTRINGS) &&
                                           (dw & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)));
                        } else {
                            MSGERROR1();
                        }

                        //
                        // if so, the length should be ULONG_PTR.
                        //
                        if (bNotString) {
                            retval = sizeof(ULONG_PTR);
                        }
                    }

                    //
                    // if the target data is "string", get it, and compute the length
                    //
                    if (!bNotString) {
                        if (PtiCurrent()->TIF_flags & TIF_INGETTEXTLENGTH) {
                            if (fAnsiSender) {
                                UserAssert(!fAnsiReceiver);
                                //
                                // retval has Unicode character count, guessed DBCS length.
                                //
                                retval *= 2;
                            }
                        } else {
                            //
                            // fAnsiReceiver == 1, retval has MBCS character count.
                            // fAnsiReceiver == 0, retval has Unicode character count.
                            //
                            // Add 1 to make room for zero-terminator.
                            //
                            DWORD cchText   = (DWORD)retval + 1;
                            DWORD cbCapture = cchText;

                            SETUPPWND(FNOUTSTRING)

                            PtiCurrent()->TIF_flags |= TIF_INGETTEXTLENGTH;

                            //
                            // if reciver is Unicode, The buffder should be reserved as musg as
                            // (TextLength * sizeof(WCHAR).
                            //
                            if (!fAnsiReceiver) {
                                cbCapture *= sizeof(WCHAR);
                            }

                            BEGINSENDCAPTURE(FNOUTSTRING, 1, cbCapture, FALSE)

                                MSGDATA()->pwnd = pwndClient;

                                //
                                // Use (msg-1) for sending the WM_GETTEXT, LB_GETTEXT or CB_GETLBTEXT
                                // since the above precompiler checks passed.
                                //
                                MSGDATA()->msg = msg-1;

                                if (msg == WM_GETTEXTLENGTH) {
                                    //
                                    // WM_GETTEXT:
                                    //    wParam = cchTextMax; // number of character to copy.
                                    //    lParam = lpszText;   // address of buffer for text.
                                    //
                                    MSGDATA()->wParam = cchText;
                                } else {
                                    //
                                    // LB_GETTEXT:
                                    // CB_GETLBTEXT:
                                    //    wParam = index;      // item index
                                    //    lParam = lpszText;   // address of buffer for text.
                                    //
                                    MSGDATA()->wParam = wParam;
                                }

                                MSGDATA()->xParam = xParam;
                                MSGDATA()->xpfnProc = xpfnProc;

                                RESERVEBYTES(cbCapture, pOutput, cbOutput);

                                LOCKPWND();
                                MAKECALLCAPTURE(FNOUTSTRING);
                                UNLOCKPWND();
                                CHECKRETURN();

                                BEGINCOPYOUT()
                                        //
                                        // retval can be [CB|LB]_ERR (-1) or [CB|LB]_ERRSPACE (-2)
                                        // then, it should be grater then zero.
                                        //
                                        if ((LONG)retval > 0) {
                                        /*
                                         * Non-zero retval means some text to copy out.
                                         */
                                        CALC_SIZE_STRING_OUT((LONG)retval);
                                    }
                                ENDCOPYOUT()

                                PtiCurrent()->TIF_flags &= ~TIF_INGETTEXTLENGTH;

                            TRACECALLBACKMSG("SfnOUTSTRING");
                            ENDSENDCAPTURE(DWORD,0);
                        }
                    }
                }
            }
        }
    ENDCOPYOUT()

    TRACECALLBACKMSG("SfnGETDBCSTEXTLENGTHS");
    ENDSEND1(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
/*
 * The fnGETTEXTLENGTHS routine is used for this message (see... client\dispcb.tpl)
 */
#endif // RECVSIDE

/***************************************************************************\
* xxxClientMonitorEnumProc
*
* Calls the client callback given to EnumDisplayMonitors.
*
* History:
* 05-Sep-1996 adams     Created.
\***************************************************************************/

typedef struct _CLIENTMONITORENUMPROCMSG {
    HMONITOR        hMonitor;
    HDC             hdcMonitor;
    RECT            rc;
    LPARAM          dwData;
    MONITORENUMPROC xpfnProc;
} CLIENTMONITORENUMPROCMSG;

#ifdef SENDSIDE
BOOL xxxClientMonitorEnumProc(
    HMONITOR        hMonitor,
    HDC             hdcMonitor,
    LPRECT          lprc,
    LPARAM          dwData,
    MONITORENUMPROC xpfnProc)
{
    SETUP(CLIENTMONITORENUMPROC)

    BEGINSEND(CLIENTMONITORENUMPROCMSG)

        MSGDATA()->hMonitor = hMonitor;
        MSGDATA()->hdcMonitor = hdcMonitor;
        MSGDATA()->rc = *lprc;
        MSGDATA()->dwData = dwData;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(CLIENTMONITORENUMPROC);
        CHECKRETURN();

    TRACECALLBACK("SxxxClientMonitorEnumProc");
    ENDSEND(BOOL,FALSE);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientMonitorEnumProc, CLIENTMONITORENUMPROCMSG)
{
    BEGINRECV(FALSE, NULL, 0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            CALLDATA(hMonitor),
            CALLDATA(hdcMonitor),
            PCALLDATA(rc),
            CALLDATA(dwData));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* xxxUserModeCallback
*
* Generic kernel callback stub
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
#ifdef SENDSIDE
NTSTATUS xxxUserModeCallback (ULONG uApi, PVOID pIn, ULONG cbIn, PVOID pOut, ULONG cbOut)
{
    NTSTATUS Status;
    PVOID pLocalOut;
    ULONG cbLocalOut;

    /*
     * Call the client
     */
    LeaveCrit();
    Status = KeUserModeCallback(uApi, pIn, cbIn, &pLocalOut, &cbLocalOut);
    EnterCrit();

    /*
     * If it failed, bail
     */
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    /*
     * If we didn't get the right amount of data, fail.
     */
    if (cbLocalOut != cbOut) {
        RIPMSG3(RIP_WARNING, "xxxUserModeCallback: uAPi: %#lx cbOut: %#lx cbLocalOut: %#lx",
                uApi, cbOut, cbLocalOut);
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * If we were expecting some data, copy it.
     */
    if (cbOut != 0) {
        try {
            ProbeForRead(pLocalOut, cbLocalOut, sizeof(DWORD));
            RtlCopyMemory(pOut, pLocalOut, cbLocalOut);
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            RIPMSG2(RIP_WARNING, "xxxUserModeCallback: uAPi: %#lx Exception: %#lx", uApi, GetExceptionCode());
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    return Status;
}
#endif // SENDSIDE

/**************************************************************************\
* fnINOUTMENUGETOBJECT
*
* 11/12/96 GerardoB     Created
\**************************************************************************/
typedef struct _FNINOUTMENUGETOBJECTMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    MENUGETOBJECTINFO mngoi;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNINOUTMENUGETOBJECTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTMENUGETOBJECT)
{
    SETUPPWND(FNINOUTMENUGETOBJECT)

    UNREFERENCED_PARAMETER(dwSCMSFlags);

    BEGINSEND(FNOUTDWORDINDWORD)

    PMENUGETOBJECTINFO pmngoi = (PMENUGETOBJECTINFO)lParam;

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->mngoi = *pmngoi;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNINOUTMENUGETOBJECT);
        UNLOCKPWND();
        CHECKRETURN();

        BEGINCOPYOUT()
            try {
                UserAssert(pcbs->cbOutput == sizeof(pmngoi->pvObj));
                ProbeForRead(pcbs->pOutput, sizeof(pmngoi->pvObj), sizeof(DWORD));
                pmngoi->pvObj = *((PVOID *)(pcbs->pOutput));
            } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                MSGERROR();
            }
        ENDCOPYOUT()

    TRACECALLBACKMSG("SfnINOUTMENUGETOBJECT");
    ENDSEND(DWORD, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTMENUGETOBJECT, FNINOUTMENUGETOBJECTMSG)
{
    BEGINRECV(0, &(pmsg->mngoi.pvObj), sizeof(pmsg->mngoi.pvObj));

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            PCALLDATA(mngoi),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnLOGONNOTIFY
*
* 2/1/97   JerrySh      Created
\**************************************************************************/
typedef struct _FNLOGONNOTIFYMSG {
    PWND pwnd;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    POWERSTATEPARAMS psParams;
    ULONG_PTR xParam;
    PROC xpfnProc;
} FNLOGONNOTIFYMSG;

#ifdef SENDSIDE
SMESSAGECALL(LOGONNOTIFY)
{
    SETUPPWND(FNLOGONNOTIFY)

    UNREFERENCED_PARAMETER(dwSCMSFlags);
    UNREFERENCED_PARAMETER(psms);

    BEGINSEND(FNLOGONNOTIFY)

        if (wParam == LOGON_POWERSTATE)
            MSGDATA()->psParams = *((PPOWERSTATEPARAMS)lParam);

        MSGDATA()->pwnd = pwndClient;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        LOCKPWND();
        MAKECALL(FNLOGONNOTIFY);
        UNLOCKPWND();
        CHECKRETURN();

    TRACECALLBACKMSG("SfnLOGONNOTIFY");
    ENDSEND(DWORD, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnLOGONNOTIFY, FNLOGONNOTIFYMSG)
{
    LPARAM lParam;

    BEGINRECV(0, NULL, 0);

    lParam = (CALLDATA(wParam) == LOGON_POWERSTATE) ? (LPARAM)&CALLDATA(psParams) : CALLDATA(lParam);

    retval = (ULONG_PTR)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(pwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            lParam,
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* xxxClientCallWinEventProc
*
* cf. Win'97 Call32BitEventProc() in user_40\user32\user.c
*
* 1996-10-18 IanJa     Created
\**************************************************************************/

typedef struct _CLIENTCALLWINEVENTPROCMSG {
    WINEVENTPROC  pfn;
    HWINEVENTHOOK hWinEventHook;
    DWORD         event;
    HWND          hwnd;
    LONG          idObject;
    LONG          idChild;
    DWORD         idEventThread;
    DWORD         dwmsEventTime;
} CLIENTCALLWINEVENTPROCMSG;

#ifdef SENDSIDE
BOOL xxxClientCallWinEventProc(
    WINEVENTPROC pfn,
    PEVENTHOOK pEventHook,
    PNOTIFY pNotify)
{
    SETUP(CLIENTCALLWINEVENTPROC)

    BEGINSEND(CLIENTCALLWINEVENTPROC)

        MSGDATA()->pfn = pfn;
        MSGDATA()->hWinEventHook = (HWINEVENTHOOK)PtoH(pEventHook);
        MSGDATA()->hwnd = pNotify->hwnd;
        MSGDATA()->event = pNotify->event;
        MSGDATA()->idObject = pNotify->idObject;
        MSGDATA()->idChild = pNotify->idChild;
        MSGDATA()->idEventThread = pNotify->idSenderThread;
        MSGDATA()->dwmsEventTime = pNotify->dwEventTime;

        MAKECALL(CLIENTCALLWINEVENTPROC);
        CHECKRETURN();

    TRACECALLBACK("xxxClientCallWinEventProc");
    ENDSEND(BOOL, FALSE);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientCallWinEventProc, CLIENTCALLWINEVENTPROCMSG)
{
    BEGINRECV(FALSE, NULL, 0);

    retval = (DWORD)CALLPROC(pmsg->pfn)(
            CALLDATA(hWinEventHook),
            CALLDATA(event),
            CALLDATA(hwnd),
            CALLDATA(idObject),
            CALLDATA(idChild),
            CALLDATA(idEventThread),
            CALLDATA(dwmsEventTime));

    ENDRECV();

}
#endif // RECVSIDE



/**************************************************************************\
* WOWGetProcModule
*
* 3/25/97 FritzS created
\**************************************************************************/

typedef struct _CLIENTWOWGETPROCMODULEMSG {
    WNDPROC_PWND pfn;
} CLIENTWOWGETPROCMODULEMSG;



#ifdef SENDSIDE
WORD xxxClientWOWGetProcModule(
    WNDPROC_PWND pfn)
{
    SETUP(CLIENTWOWGETPROCMODULE)

    BEGINSEND(CLIENTWOWGETPROCMODULE)

        MSGDATA()->pfn = pfn;

        MAKECALL(CLIENTWOWGETPROCMODULE);
        CHECKRETURN();

    TRACECALLBACK("xxxWOWGetProcModule");
    ENDSEND(WORD, FALSE);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientWOWGetProcModule, CLIENTWOWGETPROCMODULEMSG)
{
    ULONG ulReal;
    BEGINRECV(0, NULL, 0);

    if ((pfnWowGetProcModule == NULL) || !IsWOWProc(CALLDATA(pfn))) {
        retval = 0;
    } else {
        UnMarkWOWProc(CALLDATA(pfn),ulReal);
        retval = (pfnWowGetProcModule)(ulReal);
    }

    ENDRECV();

}
#endif // RECVSIDE
