#include "precomp.h"
#pragma hdrstop

#include "glue.h"
#include "resource.h"

extern HINSTANCE hInstance;

//--------------------------------------------------------------------------
// DMOLE.C
//
// Routines for OLE Remote Procedure Call debugging.
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------

#include <windows.h>
#include <stdlib.h>
#include <stddef.h>

typedef HANDLE HANDLE32;

#include "orpc_dbg.h"

//--------------------------------------------------------------------------
// Types
//--------------------------------------------------------------------------


typedef struct _OLEDIHDR                // OLD Debug Info Header
{
    BYTE        rgbSig[4];      // 4-byte signature, "MARB"
    BYTE        bVerMajor;      // major version
    BYTE        bVerMinor;      // minor version, in range 0-99
} OLEDIHDR;


typedef struct _OLECALLDI       // OLE Call Debug Info
{
    OLEDIHDR    oledihdr;
    DWORD       dwTerminator;   // terminating 4 bytes of zeroes
} OLECALLDI;


typedef struct _OLERETBLK       // OLE Return Block
{
    DWORD       cb;             // total size of following block
    GUID        guid;           // identifying guid
    DWORD       fGo;            // Go vs. Step
} OLERETBLK;


typedef struct _OLERETDI                // OLE Return Debug Info
{
    OLEDIHDR    oledihdr;
    OLERETBLK   oleretblk;
    DWORD       dwTerminator;       // terminating 4 bytes of zeroes
} OLERETDI;


//--------------------------------------------------------------------------
// Private Function decls
//--------------------------------------------------------------------------

BOOL
ReadOrpcDbgInfo(
    HTHDX,
    LPDEBUG_EVENT64,
    ORPC_DBG_ALL*
    );

VOID
ActionOleReturnEvent(
    DEBUG_EVENT64 *pde,
    HTHDX        hthd,
    DWORDLONG    unused,
    DWORDLONG    lparam
);


BOOL
FEnableOleRpc(
    HTHDX               hthd,
    COMPLETION_FUNCTION CompletionFunction,
    LPVOID              CompletionArg
    );

typedef struct
{
    LPCSTR segName;
    OLESEG segType;

} OLESEGNAMEMAP;

OLESEGNAMEMAP oleSegNames[] =
{
    { ".orpc", oleorpc },
    { ".olebrk", olebrk },
    { ".olefwd", olefwd },
};

//--------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------

BOOL        fOleRpc = FALSE;                    // Whether OLE RPC stepping is enabled
BOOL        fDirtyOleEnable = FALSE;

OLECALLDI       olecalldi =
{
    { "MARB", 1, 0 },
    0
};

OLERETDI        oleretdi =
{
    { "MARB", 1, 0 },
    { sizeof (OLERETBLK), { 0 }, FALSE }
};


GUID    guidRet =
{
    0x9CADE560L,    0x8F43, 0x101A, 0xB0, 0x7B,     0x00,
    0xDD,                   0x01,   0x11, 0x3F, 0x11
};

//--------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------



void
SetExceptionAddress(
    DEBUG_EVENT64*    pde,
    UOFFSET         addr
    )
{
    pde->u.Exception.ExceptionRecord.ExceptionAddress = addr;
}


void
MethodStepToOrpc(
    DEBUG_EVENT64*  event,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    )
/*++

Routine Description:

    We need to continue stepping till we are in the .orpc segment.  Once
    there, we need to find the nearest place on the frame with source, and
    run to that point.

--*/
{
    METHOD* method = (METHOD*)lparam;
    if (!FAddrInOle (hthd->hprc, PC(hthd))) {
        //
        //      if we have not reached the .orpc section yet, continue "looping"
        //

        SingleStep (hthd, method, TRUE, FALSE);
        return;
    }


#if 0

    //
    //      if theres no source, we want to find the nearest frame with source
    //      and run to that point.

    if (CanStep (hthd->hprc, hthd, 0) == CANSTEP_NO) {
        xosd = GetFrameWithSrc (hpid, htid, &addr);

        assert (xosd == xosdNone);

        SetCodeBreakpint (hpid, htid, bpnsStop, &addr);

        // register expected event to do this

        // add to queue

    }

#endif
}


VOID
ProcessOleEvent(
    DEBUG_EVENT64* pde,
    HTHDX          hthd
    )
{
    ORPC orpc = (ORPC) pde->u.Exception.ExceptionRecord.ExceptionCode;


    //
    //  There are two cases where we can receive an OLE event without having
    //  registered an event handler: ServerNotify and ClientNotify.
    //
    //  ServerNotify happens if a client makes an RPC call with OLE debugging
    //  enabled, so the server launches a debugger and raises the exception.
    //
    //  ClientNotify happens if a server returns from an RPC call with OLE
    //  debugging enabled, so the client launches a debugger and raises the
    //      exception.
    //

    switch (orpc) {
        case orpcClientNotify:

            DPRINT(1,("ProcessOleEvent:ClientNotify\n"));

                        //
            //  If the user said Step out of the server, then we want to
            //  step out of OLE.  If he said Go, then we can just run.

            if (FClientNotifyStep(hthd, pde)) {
                EXOP    exope;
                BOOL    fContinue = TRUE;
                METHOD* method;

                ConsumeAllProcessEvents(hthd->hprc, FALSE);

//              exope.fAllThreads = TRUE;
                exope.fSingleThread = FALSE;
//              exope.fCheckSrc = FALSE;
//              exope.fGoException = FALSE;
                exope.fStepOver = TRUE;
                exope.fInitialBP = FALSE;
//              exope.fFirstTime = TRUE;
//              exope.fGo = FALSE;
//              exope.fIgnoreOle = TRUE;

                assert(!FAddrInOle(hthd->hprc, PC(hthd)));


                method = (METHOD*) MHAlloc (sizeof (*method));

                method->notifyFunction = MethodStepToOrpc;
                method->lparam = (UINT_PTR)method;


                SingleStep (hthd, method, TRUE, FALSE);

#if 0
// BUGBUG kentf This cannot be done this way.  The debugger cannot be blocked
// BUGBUG waiting for a step to complete.  This must be done the usual way, by
// BUGBUG registering an expected event and initiating a step.

                // We start stepping till we get out of the
                // RaiseException call into the OLE .orpc section.
                while(fContinue && !FAddrInOle(hthd->hprc, PC(hthd)))
                {
                    fContinue = DMSStep(hthd->hprc->hpid, hthd->htid, exope);
//                  exope.fFirstTime = FALSE;
                }
#endif

                // Once we reach the .orpc section we continue stepping till
                // we get out of it to get to our ultimate destination.

                while (fContinue && FAddrInOle(hthd->hprc, PC(hthd)))
                {
                    fContinue = DMSStep(hthd->hprc->hpid, hthd->htid, exope);
//                  exope.fFirstTime = FALSE;
                }

                if (fContinue)
                {
//                  PCP pcp;
                    HPID hpid = hthd->hprc->hpid;
                    HTID htid = hthd->htid;

                    // If we dont have source continue looking up the callstack
                    // for the first frame with debug info.
#pragma message("NYI: dbceFrameWithSrc")
#if 0 // Not ready for MIPS
                    if ( CANSTEP_NO == CanStep(hthd->hprc, hthd, 0))
                    {
                        DWORD cFrames = 16;
                        ADDR addr;
                        XOSD xosd;

                        DMSRequest(dbceFrameWithSrc, hpid, htid, sizeof(DWORD), &cFrames);

                        xosd = DMSendRequestReply(
                            hpid,
                            htid,
                            dbceFrameWithSrc,
                            sizeof(DWORD),
                            &cFrames,
                            sizeof(ADDR),
                            &addr
                            );

                        if ( xosd == xosdNone ) {

                            if ( DMUSetCodeBreakPoint(hpid, htid, bpnsStop,
                                    &addr) == xosdNone)
                            {
                                // Run until we hit the breakpoint.
                                RegisterExpectedEvent(
                                    hthd->hprc,
                                    hthd,
                                    BREAKPOINT_DEBUG_EVENT,
                                    NO_SUBCLASS,
                                    DONT_NOTIFY,
                                    ActionOrpcSkipToSource,
                                    ConsumeOrpcSkipToSource,
                                    addr.addr.off
                                    );

                                // Execute call had "IgnoreTrace"..
                                ContinueThread(hthd);

                                return;
                            }
                        }
                    }
#endif

                    //
                    //  Send a step event to the IDE to make it stop.
                    //

                    SetExceptionAddress (&falseSSEvent, PC(hthd));
                    NotifyEM(&falseSSEvent, hthd, 0, 0);
                }
                return;
            }
            break;

        case orpcServerNotify:
            {
                ADDR                    addrFunc = {0};
                UOFFSET                 uoffFunc;
                LPBYTE                  lpbVtable;
                DWORD                   iMethod;
                DWORD                   cb;
                ORPC_DBG_ALL    orpc_all;
                BREAKPOINT*             pbp;

                DPRINT(1,("ProcessOleEvent:ServerNotify\n"));

                //
                //      If the call side didnt want us to stop, just continue
                //      execution.

                if ( !FServerNotifyStop(hthd, pde)) {
                    break;
                }

                //
                //      We now need to set a breakpoint at the address of the
                //      function which is actually getting called.
                //

                //
                //      Dereference the pointer to the Interface to get a
                //      pointer to the vtable.
                //

                if (!ReadOrpcDbgInfo(hthd, pde, &orpc_all)) {
                    assert(FALSE);
                    break;
                }

                if (!DbgReadMemory(hthd->hprc,
                                   (UINT_PTR)orpc_all.pInterface,
                                   &lpbVtable,
                                   sizeof(lpbVtable),
                                   &cb))
                {
                    assert(FALSE);
                    break;
                }

                //
                //      The _pMessage field points to an RPCOLEMESSAGE,
                //      which contains an iMethod field, the index of the method
                //      being called.
                //

                if (!DbgReadMemory(hthd->hprc,
                                   ((UOFFSET)orpc_all.pMessage + offsetof(RPCOLEMESSAGE, iMethod)),
                                   &iMethod,
                                   sizeof(iMethod),
                                   &cb))
                {
                    assert(FALSE);
                    break;
                }


                //
                //      Dereference into the vtable to get the ptr to the function
                //

                if (!DbgReadMemory(hthd->hprc,
                                   (DWORDLONG)lpbVtable + (iMethod*sizeof(LPVOID)),
                                   &uoffFunc,
                                   sizeof(uoffFunc),
                                   &cb))
                {
                    assert(FALSE);
                    break;
                }


                //
                //      We now have (in uoffFunc) the address of the actual
                //      virtual function which is going to get called!  Set a
                //      breakpoint there.
                //

                addrFunc.addr.off = uoffFunc;
                pbp = SetBP (hthd->hprc,
                             hthd,
                             bptpExec,
                             bpnsStop,
                             &addrFunc,
                             (HPID) INVALID
                             );

                if (!pbp) {
                    assert(FALSE);
                    break;
                }

                // Run until we hit the breakpoint.

                RegisterExpectedEvent (hthd->hprc,
                                       hthd,
                                       BREAKPOINT_DEBUG_EVENT,
                                       (UINT_PTR)pbp,
                                       DONT_NOTIFY,
                                       ActionOrpcServerNotify,
                                       FALSE,
                                       (UINT_PTR)pbp
                                       );
            }
            break;

        case orpcClientGetBufferSize:
        case orpcServerGetBufferSize:
            {
                ORPC_DBG_ALL orpc_all;
                ULONG cb = 0;
                ULONG cbRead;

                                //
                //      If we recieve an OLE notification without having an
                //      expected event for it, we just want to tell OLE
                //      we are not interested.

                if (!ReadOrpcDbgInfo(hthd, pde, &orpc_all) ) {
                    assert(FALSE);
                    break;
                }

                assert(orpc_all.lpcbBuffer != NULL);

                if (!DbgWriteMemory(hthd->hprc,
                                    (UINT_PTR)orpc_all.lpcbBuffer,
                                    &cb,
                                    sizeof(cb),
                                    &cbRead))
                {
                    assert(FALSE);
                }

                break;
            }

        case orpcClientFillBuffer:
        case orpcServerFillBuffer:
            assert(FALSE);  // can these notifications happen?
            break;

        default:
            assert(FALSE);
            break;
    }

    if (!masterEE.next)
    {
        RegisterExpectedEvent(hthd->hprc,
                              hthd,
                              GENERIC_DEBUG_EVENT,
                              NO_SUBCLASS,
                              DONT_NOTIFY,
                              NO_ACTION,
                              NO_CONSUME,
                              0);
    }

    ContinueThread(hthd);

} /* PROCESSOLEEVENT */


//--------------------------------------------------------------------------
// ActionOrpcClientGetBufferSize
//
// This is called if the user did a Step Into on a line which ended up
// calling an OLE Interface Proxy (code in a ".orpc" section).  We called
// FEnableOleRpc(hthd, TRUE), and did a Step Over.  Now, OLE is asking
// us how big a buffer to allocate for the information we want to pass to
// the remote debugger.
//--------------------------------------------------------------------------

VOID
ActionOrpcClientGetBufferSize (
    DEBUG_EVENT64 * pde,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    )
{
    ORPC_DBG_ALL orpc_all;
    LPSSMD lpssmd = (LPSSMD)lparam;

    DPRINT(1,("ActionOrpcClientGetBufferSize\n"));

    if ( ReadOrpcDbgInfo(hthd, pde, &orpc_all) ) {
        ULONG cb = sizeof(olecalldi);
        ULONG cbWrite;

        if ( DbgWriteMemory(hthd->hprc,
                            (UINT_PTR)orpc_all.lpcbBuffer,
                            &cb,
                            sizeof(cb),
                            &cbWrite) )
        {
            //
            //  Register for next event, the FillBuffer one.  Note, theres
            //  also still an expected event for the breakpoint right after
            //  the call into OLE.
            //

            RegisterExpectedEvent(hthd->hprc,
                                  hthd,
                                  OLE_DEBUG_EVENT,
                                  orpcClientFillBuffer,
                                  DONT_NOTIFY,
                                  ActionOrpcClientFillBuffer,
                                  NO_CONSUME,
                                  (UINT_PTR)lpssmd);
        }
    }

    // Resume execution

    ContinueThread(hthd);
}

//--------------------------------------------------------------------------
// ActionOrpcClientFillBuffer
//
// We already received an OrpcClientGetBufferSize notification, and now
// were being asked to fill in the buffer with data to be passed to the
// remote debugger.
//--------------------------------------------------------------------------

VOID
ActionOrpcClientFillBuffer (
    DEBUG_EVENT64 *   pde,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    )
{
    LPSSMD       lpssmd = (LPSSMD)lparam;
    DWORD        cb;
    ORPC_DBG_ALL orpc_all;

    DPRINT(1,("ActionOrpcClientFillBuffer\n"));

    if (ReadOrpcDbgInfo(hthd, pde, &orpc_all)) {
        assert(orpc_all.cbBuffer == sizeof(olecalldi));

        if (orpc_all.cbBuffer == sizeof(olecalldi)) {
            DbgWriteMemory (hthd->hprc,
                            (UINT_PTR)orpc_all.pvBuffer,
                            &olecalldi,
                            sizeof (olecalldi),
                            &cb
                            );

            //
            //  There should still be a registered event for the breakpoint
            //  right after returning from the call into OLE.
            //

            assert (masterEE.next);

            //
            //  Add another registered event for ClientNotify
            //

            RegisterExpectedEvent (hthd->hprc,
                                   hthd,
                                   OLE_DEBUG_EVENT,
                                   orpcClientNotify,
                                   DONT_NOTIFY,
                                   ActionOrpcClientNotify,
                                   NO_CONSUME,
                                   (UINT_PTR)lpssmd
                                   );
        }
    }

    if (!masterEE.next) // this would be quite unusual...
    {
        RegisterExpectedEvent(hthd->hprc,
                              hthd,
                              GENERIC_DEBUG_EVENT,
                              NO_SUBCLASS,
                              DONT_NOTIFY,
                              NO_ACTION,
                              NO_CONSUME,
                              0);
    }

    // Resume execution
    ContinueThread(hthd);
}



BOOL
FClientNotifyStep(
    HTHDX           hthd,
    LPDEBUG_EVENT64 pde
    )
/*++

Routine Description:

        If a ClientNotify notification has occurred, this function can be called
        to determine whether the server said to Step or Go.  Returns TRUE if the
        server said Step. Returns FALSE if the server said Go or there was no
        debugger attached to the other side.

--*/
{
    ORPC_DBG_ALL orpc_all;
    DWORD        cbRead;
    BOOL         fStep   = FALSE;
    OLERETDI     retdi;
    BOOL         fSucc;

    if (ReadOrpcDbgInfo(hthd, pde, &orpc_all)) {

        if (orpc_all.cbBuffer == sizeof (oleretdi)) {
            fSucc = DbgReadMemory (hthd->hprc,
                                   (UINT_PTR)orpc_all.pvBuffer,
                                   &retdi,
                                   orpc_all.cbBuffer,
                                   &cbRead
                                   );

            assert (fSucc);

//          assert (guid is our guid);

            fStep = !retdi.oleretblk.fGo;
        }

        return fStep;

#if 0

        //
        //      This code is for when the server returns back multiple
        //      oleretblks, which it doesnt currently do.


        //
        //  Loop through all debug info GUIDs, searching for the one we
        //  recognize, which will tell us whether the user did a Step or
        //  a Go on the server side.  If we dont find the GUID anywhere,
        //  then act as if he did a Go.
        //


        UOFFSET     uoffDebugInfo = (UOFFSET) orpc_all.pvBuffer;
        OLEDIHDR    oledihdr;
        OLERETBLK   oleretblk;

        if (orpc_all.cbBuffer >= sizeof(oledihdr) &&
            DbgReadMemory (hthd->hprc,
                           uoffDebugInfo,
                           &oledihdr,
                           sizeof(oledihdr),
                           &cbRead) &&
            memcmp(&oledihdr, &olecalldi.oledihdr, sizeof(oledihdr)) == 0)
        {
            uoffDebugInfo += sizeof(oledihdr);

            for (;;) {
                if (!DbgReadMemory(hthd->hprc,
                                   uoffDebugInfo,
                                   &oleretblk,
                                   sizeof(oleretblk),
                                   &cbRead)) {
                    break;
                }

                if (oleretblk.cb == 0) {
                    break;
                } else if (oleretblk.cb == sizeof(oleretblk) &&
                    memcmp(&oleretblk.guid, &guidRet, sizeof(guidRet)) == 0) {
                    // Found a match.  Remember whether the user said Go.
                    fStep = !oleretblk.fGo;
                    break;  // out of for loop
                } else {
                    // check next guid
                    uoffDebugInfo += oleretblk.cb;
                }
            }
        }

#endif

    }

    return fStep;
}

BOOL
FServerNotifyStop(
    HTHDX hthd,
    LPDEBUG_EVENT64 pde
    )
/*++

Routine Description:

        If a ServerNotify notification has occurred, this function can be called
        to determine whether the client asked us to Stop.  Returns TRUE if the
        client said Stop.  Returns FALSE otherwise.

--*/
{
    ORPC_DBG_ALL    orpc_all;
    DWORD           cbRead;
    BOOL            fStop = FALSE;

        //
    //  See if the client side passed us the information we were expecting.

    if ( ReadOrpcDbgInfo(hthd, pde, &orpc_all) ) {
        UOFFSET     uoffDebugInfo = (UOFFSET) orpc_all.pvBuffer;
        OLECALLDI   calldi;

        if (orpc_all.cbBuffer >= sizeof(olecalldi) &&
            DbgReadMemory(hthd->hprc,
                          uoffDebugInfo,
                          &calldi,
                          sizeof(calldi),
                          &cbRead) &&
            memcmp(&calldi.oledihdr, &olecalldi.oledihdr, sizeof(OLEDIHDR)) == 0)
        {
            // We are requiring both the major and minor versions match.
            fStop = TRUE;
        }
    }

    return fStop;
}


//--------------------------------------------------------------------------
// ActionOrpcClientNotify
//
// Earlier we received ClientGetBufferSize and ClientFillBuffer
// notifications, and then OLE made the remote call; now its notifying
// us that the remote call has completed.
//--------------------------------------------------------------------------

VOID
ActionOrpcClientNotify (
    LPDEBUG_EVENT64 pde,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    )
{
    LPSSMD lpssmd = (LPSSMD)lparam;

    DPRINT(1,("ActionOrpcClientNotify\n"));

    //
    //  Did user say Step or Go from the server side?
    //

    if (FClientNotifyStep(hthd, pde)) {
        //
        //      User said Step.  In this case, we want to stop immediately after
        //      the call, rather than continuing to step to the end of the
        //      source line.
        //

        // nothing really to do

    } else {
        //
        //      User said Go.  Clear the temporary breakpoint at the return
        //      address of the function call.
        //

        ConsumeAllProcessEvents(hthd->hprc, FALSE);
    }

    if (!masterEE.next) {
        RegisterExpectedEvent (hthd->hprc,
                               hthd,
                               GENERIC_DEBUG_EVENT,
                               NO_SUBCLASS,
                               DONT_NOTIFY,
                               NO_ACTION,
                               NO_CONSUME,
                               0
                               );
    }

    // Resume execution

    ContinueThread(hthd);
}

//--------------------------------------------------------------------------
// ConsumeOrpcSkipToSource
//
// Clear the temporary breakpoint we set.
//--------------------------------------------------------------------------

VOID
ConsumeOrpcSkipToSource (
    HPRCX hprc,
    HTHDX hthd,
    LPVOID lpvFunc
    )
{
    RemoveBP((BREAKPOINT*)lpvFunc);
}


//--------------------------------------------------------------------------
// ActionOrpcSkipToSource
//
// We received an OrpcClientNotify but we have no idea where in the users
// code the RPC call was made from. In this case we look up the callstack and
// set a BP on the first frame with source. We hit that BP.
//--------------------------------------------------------------------------

void
ActionOrpcSkipToSource(
    LPDEBUG_EVENT64 pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
{
    DPRINT(1,("ActionOrpcSkipToSource\n"));

    MoveIPToException(hthd, pde);

    // Clear the temporary breakpoint.
    ConsumeOrpcSkipToSource(hthd->hprc, hthd, (LPVOID)lparam);

    // Have we reached the breakpoint we set or some other breakpoint.

    if (PC(hthd) == lparam) {
        //
        //      Send a step event to the IDE to make is stop.
        //

        SetExceptionAddress (&falseSSEvent, PC(hthd));
        NotifyEM(&falseSSEvent, hthd, 0, 0);
    } else {
        // We reached some other breakpoint.
        ProcessBreakpointEvent(pde, hthd);
    }

    ConsumeAllProcessEvents(hthd->hprc, FALSE);
}

//--------------------------------------------------------------------------
// ConsumeOrpcServerNotify
//
// This is received when weve hit the breakpoint at the beginning of the
// server function which is being called, OR when some other notification
// causes that breakpoint event to be consumed.  We clear the breakpoint.
//--------------------------------------------------------------------------

VOID
ConsumeOrpcServerNotify(
    HPRCX hprc,
    HTHDX hthd,
    LPVOID lpvFunc
    )
{
    RemoveBP((BREAKPOINT*)lpvFunc);
}

//--------------------------------------------------------------------------
// AtOleHelperCall
//
// Check if we are at the call to a OLE  helper function.
// Returns TRUE if we are at a call to an OLE helper.
// if lpoleseg is not NULL it contains the type of the helper
// segment on return.
//--------------------------------------------------------------------------


BOOL
AtOleHelperCall (
    HTHDX hthd,
    UOFFSET uOffCurr,
    OLESEG *lpoleseg
    )
{
    INT fCall = INSTR_TRACE_BIT;
    INT fThunk = THUNK_NONE;
    int cThunk;
    UOFFSET uOffNext;
    UOFFSET uOffThunkDest;
    UOFFSET uOffDest = 0;


    uOffNext = uOffCurr;
//  IsCall(hthd, &uOffNext, &fCall, &uOffDest);
    assert(FALSE);

    if ( fCall == INSTR_IS_CALL )
    {
        OLESEG oleseg;

        // We should get the destination for the call.
        assert(uOffDest != 0);

        cThunk = 8;
        //Skip over any thunks.
        while ( --cThunk &&
                IsThunk(hthd, uOffDest, &fThunk, &uOffThunkDest, NULL) &&
                fThunk != THUNK_NONE) {

            uOffDest = uOffThunkDest;
        }

        // At this point we have skipped over the thunks and
        // eventual address is in uOffCurr. Check if that
        // address is in a helper segment.

        oleseg = OleSegFromAddr(hthd->hprc, uOffDest);

        if ( oleseg == olebrk || oleseg == olefwd ) {
            if ( lpoleseg ) {
                *lpoleseg = oleseg;
            }
            return TRUE;
        }
    }
    return (FALSE);
}

//----------------------------------------------------------------------------
// FSetupSkipWithHelperInfo
//
// We have reached the call to the ORPC debug helper function.
// Figure out where we are going and set the breakpoint there.
//
// The helper call should look like
// a) OleHelperCall(this, pmf);
// We will glean the "this" pointer and the pointer to mbr func from the stack.
// The PMF will be of the type which can only deal with single inheritance
// hierarchy.
// b) OleHelperCall(pfn)
// Here we just get the function address which is the only arg.
// Once the dest addr is known we set a temp bp there and continue
// execution expecting to hit the users server implementation...
//-----------------------------------------------------------------------------

BOOL
FSetupSkipWithHelperInfo (
    HTHDX hthd
    )
{
    UOFFSET uThis;
    UOFFSET uPMF; // For SI the sizeof(pmf) == 4
    UOFFSET uDest = 0;
    ADDR addrDest = {0};
    OLESEG oleseg;

    DbgGetThreadContext(hthd, &hthd->context);
    if ( !AtOleHelperCall(hthd, PC(hthd), &oleseg) )
    {
        assert(FALSE);
        return FALSE;
    }

    if ( oleseg == olebrk )
    {
        // The args are "this" and "pmf".

//BUGBUG kentf This is undoubtedly wrong on RISC.
        // Read the this pointer from the current ESP.
        if (!DbgReadMemory(hthd->hprc,
                           STACK_POINTER(hthd),
                           &uThis,
                           sizeof(uThis),
                           NULL))
        {
            assert(FALSE);
            return FALSE;
        }

        // read the PMF from (ESP + 4)
        if (!DbgReadMemory(hthd->hprc,
                           STACK_POINTER(hthd) + 4,
                           &uPMF,
                           sizeof(uPMF),
                           NULL))
        {
            assert(FALSE);
            return FALSE;
        }

        // Get the address where the call (this->*pmf)(...) will go to.
        if ((uThis != 0) &&  !GetPMFDest(hthd, uThis, uPMF, &uDest) )
        {
            return FALSE;
        }
    }
    else
    {
        // There is one argument which is just the function ptr.
        assert(oleseg == olefwd);

        // Read the function address from the current ESP.
        if (!DbgReadMemory(hthd->hprc,
                           STACK_POINTER(hthd),
                           &uDest,
                           sizeof(uDest),
                           NULL))
        {
            assert(FALSE);
            return FALSE;
        }
    }


    if ( uDest == 0 )
    {
        // For some reason this RPC call is going to fail without ever
        // reaching the users code. We are just going to continue
        // executing the server. However set an expected event for
        // a ServerGetBufferSize so we can tell the client side to stop.

        RegisterExpectedEvent(hthd->hprc,
                              hthd,
                              OLE_DEBUG_EVENT,
                              orpcServerGetBufferSize,
                              DONT_NOTIFY,
                              ActionOrpcServerGetBufferSize,
                              FALSE /*ConsumeOrpcServerGetBufferSize*/,
                              FALSE);
    } else {
        BREAKPOINT* pbp;
        // We now have ( in uDest ) the address of the call where we need to go.
        // . Set a breakpoint there.
        addrDest.addr.off = uDest;

        pbp = SetBP(hthd->hprc, hthd, bptpExec, bpnsStop, &addrDest, (HPID) INVALID);
        if (pbp == NULL) {
            assert(FALSE);
            return FALSE;
        }

        // Run until we hit the breakpoint.
        RegisterExpectedEvent(hthd->hprc,
                              hthd,
                              BREAKPOINT_DEBUG_EVENT,
                              (UINT_PTR)pbp,
                              DONT_NOTIFY,
                              ActionOrpcServerNotify,
                              FALSE /*ConsumeOrpcServerNotify*/,
                              (UINT_PTR)pbp);
    }


    return TRUE;
}

BOOL
GetInterruptReturnAddress(
    HTHDX           hthd,
    UOFFSET*        Address
    )
{

#if defined (TARGET_i386)

    DWORD   cb;
    BOOL    fSucc = FALSE;

    fSucc = DbgReadMemory (hthd->hprc,
                           STACK_POINTER (hthd),
                           Address,
                           sizeof (*Address),
                           &cb
                           );
    return fSucc;

#elif defined (TARGET_ALPHA) || defined (TARGET_AXP64)

    *Address = (UOFFSET)hthd->context.IntRa;
    return TRUE;

#elif defined (TARGET_IA64)

    *Address = (UOFFSET)hthd->context.BrRp;
    return TRUE;

#else

    #error Need to get interrupt return value.

#endif

}



VOID
OrpcServerNotifyContinue(
    HTHDX   hthd,
    LPVOID  Argument
    )
{
    BREAKPOINT*     Breakpoint = (BREAKPOINT*) Argument;

    RegisterExpectedEvent (hthd->hprc,
                           hthd,
                           BREAKPOINT_DEBUG_EVENT,
                           (UINT_PTR)Breakpoint,
                           DONT_NOTIFY,
                           ActionOleReturnEvent,
                           FALSE,
                           (UINT_PTR)Breakpoint
                           );

    //
    //      Send a step event to the IDE to make it stop
    //

    SetExceptionAddress(&falseSSEvent, PC(hthd));
    NotifyEM(&falseSSEvent, hthd, 0, 0);

    ConsumeAllProcessEvents(hthd->hprc, FALSE);
}


VOID
ActionOrpcServerNotify(
    LPDEBUG_EVENT64 pde,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    )
/*++

Routine Description:

    This is received when we have hit the breakpoint at the beginning of the
    server function which is being called.  (This is NOT called for the
    ServerNotify notification itself: the ServerNotify handling registered
    this function as its expected event.) We might come to this function
    multiple times before actually reaching the users server code. This
    happens when we encounter special helper functions put in by mfcans32
    and the standard dispatch implementation.

--*/
{
    ADDR                addrRet = {0};
    UOFFSET             Func = 0;
    UOFFSET             uoffRet;
    BREAKPOINT* pbp = (BREAKPOINT*) lparam;


    DPRINT(1,("ActionOrpcServerNotify\n"));

    if (pbp) {
        Func = GetAddrOff(pbp->addr);
    }

    //
    //  Move IP back to the exception.
    //

    MoveIPToException (hthd, pde);

    //
    //  Clear breakpoint at function beginning
    //

    ConsumeOrpcServerNotify (hthd->hprc, hthd, pbp);

    //
    //  Have we reached the breakpoint at the beginning of the server
    //  interface function, or some other breakpoint?
    //

    if (PC(hthd) != Func) {
        //
        //      We reached some other breakpoint.
        //

        ProcessBreakpointEvent(pde, hthd);
        ConsumeAllProcessEvents(hthd->hprc, FALSE);

    } else {
        //
        //      We have reached the breakpoint at the beginning of the server
        //      interface function which was called. Set a BP here so we
        //      can send data back to the client-side debugger when this RPC
        //      call is done.
        //

        VERIFY (GetInterruptReturnAddress (hthd, &uoffRet));


        addrRet.addr.off = uoffRet;
        pbp = SetBP (hthd->hprc,
                     hthd,
                     bptpExec,
                     bpnsStop,
                     &addrRet,
                     (HPID) INVALID);


        assert (pbp);

        PushOleRetAddr(hthd, uoffRet, STACK_POINTER(hthd));

        EnsureOleRpcStatus (hthd, OrpcServerNotifyContinue, pbp);
    }
}


VOID
ActionOrpcServerFillBuffer(
    LPDEBUG_EVENT64     pde,
    HTHDX               hthd,
    DWORDLONG           unused,
    DWORDLONG           lparam
    )
/*++

Routine Description:

    This is called after we have responded to OrpcServerGetBufferSize; we now
    fill in the buffer of info to pass back to the client debugger.

--*/
{
    BOOL         fGo = (BOOL) lparam;
    DWORD        cb;
    ORPC_DBG_ALL orpc_all;

    DPRINT(1,("ActionOrpcServerFillBuffer\n"));

    if (ReadOrpcDbgInfo (hthd, pde, &orpc_all)) {
        assert (orpc_all.cbBuffer == sizeof (oleretdi));

        if (orpc_all.cbBuffer == sizeof (oleretdi)) {
            //
            //  Fill in fields of the return block
            //

            oleretdi.oleretblk.guid = guidRet;  // memcpy(&oleretdi.oleretblk.guid, &guidRet, sizeof(guidRet));
            oleretdi.oleretblk.fGo = fGo;

            DbgWriteMemory (hthd->hprc,
                            (DWORDLONG)orpc_all.pvBuffer,
                            &oleretdi,
                            sizeof (oleretdi),
                            &cb
                            );
        }
    }

    if (!masterEE.next) {
        RegisterExpectedEvent (hthd->hprc,
                               hthd,
                               GENERIC_DEBUG_EVENT,
                               NO_SUBCLASS,
                               DONT_NOTIFY,
                               NO_ACTION,
                               NO_CONSUME,
                               0);
    }

    //
    //  Resume execution
    //

    ContinueThread(hthd);
}

//--------------------------------------------------------------------------
// ConsumeOrpcServerGetBufferSize
//
// This is called when the remote function has finished executing, and
// its time to send information back to the client debugger.
//--------------------------------------------------------------------------

VOID
ConsumeOrpcServerGetBufferSize(
    HPRCX hprc,
    HTHDX hthd,
    LPVOID lpv
    )
{
    // Placeholder for right now. Remove this function after
    // new scheme has stabilized.
}

//--------------------------------------------------------------------------
// ActionOrpcServerGetBufferSize
//
// This is called when the remote function has finished executing, and
// its time to send information back to the client debugger.
//--------------------------------------------------------------------------

VOID
ActionOrpcServerGetBufferSize(
    LPDEBUG_EVENT64 pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
{
    ORPC_DBG_ALL        orpc_all;

    DPRINT(1,("ActionOrpcServerGetBufferSize\n"));

    ConsumeOrpcServerGetBufferSize (hthd->hprc, hthd, (LPVOID)lparam);

        //
    //  specify count of bytes

    if (ReadOrpcDbgInfo(hthd, pde, &orpc_all)) {
        ULONG cb = sizeof(oleretdi);
        ULONG cbWrite;

        if (DbgWriteMemory (hthd->hprc,
                            (UINT_PTR)orpc_all.lpcbBuffer,
                            &cb,
                            sizeof (cb),
                            &cbWrite)
                            )
        {
            //
            //  Register for next event, the FillBuffer one.  Note, there is
            //  also still an expected event for the breakpoint right after
            //  the call into OLE.
            //

            RegisterExpectedEvent (hthd->hprc,
                                   hthd,
                                   OLE_DEBUG_EVENT,
                                   orpcServerFillBuffer,
                                   DONT_NOTIFY,
                                   ActionOrpcServerFillBuffer,
                                   NO_CONSUME,
                                   lparam
                                   );
        }

    }

    //
    //  Resume execution
    //

    ContinueThread(hthd);
}

//--------------------------------------------------------------------------
// CompOleAddr
//
// Returns -1, 0, or 1 to indicate the position of the given offset relative
// to the range defined by the given OLERG structure.
//--------------------------------------------------------------------------

int
WINAPIV
CompOleAddr(
    const void * lpuoffKey,
    const void * lpolergElem
    )
{
    if (*((LPUOFFSET)lpuoffKey) < ((OLERG *)lpolergElem)->uoffMin) {
        return -1;
    } else if (*((LPUOFFSET)lpuoffKey) >= ((OLERG *)lpolergElem)->uoffMax) {
        return 1;
    } else {
        return 0;
    }
}

//-------------------------------------------------------------------------
// GetOleSegType
//
//  Given a segment name returns an OLESEG enumerate indicating what kind of
//  OLE segment this is. If this is not one of the special OLE segment
//  returns olenone.
//--------------------------------------------------------------------------

OLESEG
GetOleSegType(
    LPVOID lpsegname
    )
{
    int cSegs = sizeof(oleSegNames)/sizeof(oleSegNames[0]);
    int i;

    for ( i = 0 ; i < cSegs ; i++ ) {
        int cbseg = _ftcslen(oleSegNames[i].segName);

        if (!_fmemcmp(lpsegname, oleSegNames[i].segName, cbseg)) {
            return oleSegNames[i].segType ;
        }
    }

    assert ( i == cSegs );
    return olenone;
}

//--------------------------------------------------------------------------
// OleSegFromAddr
//
// Determines whether the given address is inside a special OLE segment.
// We keep track of the special OLE segments when we get the DLL load
// notifications. If the given offset lies within a special OLE segment
// this function return the seg type. If the address doesnt belong to
// one of these segments, returns olenone.
//--------------------------------------------------------------------------

OLESEG
OleSegFromAddr (
    HPRCX hprc,
    UOFFSET uoff
    )
{
    OLERG * lpolerg = NULL;

    if (hprc->colerg > 0)
    {
        lpolerg = (OLERG *)bsearch(&uoff,
                                   hprc->rgolerg,
                                   hprc->colerg,
                                   sizeof(OLERG),
                                   CompOleAddr);
    }
    return ( lpolerg ? lpolerg->segType : olenone );
}


VOID
EnsureOleRpcStatus(
    HTHDX               hthd,
    COMPLETION_FUNCTION CompletionFunction,
    LPVOID              CompletionArg
    )
/*++

Routine Description:

    Ensure the OLE RPC Status is up to date.  Call the trojan if necessary.
    OLE Rpc debugging is on a per-process basis.  The thread that we passed in
    is used to call the trojan if necessary.  It must be stopped.

Arguments:

    hthd

    CompletionFunction

    CompletionArg

Return Value:

--*/
{

    if (hthd->hprc->OrpcDebugging == ORPC_START_DEBUGGING ||
            hthd->hprc->OrpcDebugging == ORPC_STOP_DEBUGGING) {
        FEnableOleRpc (hthd, CompletionFunction, CompletionArg);
    } else {
        CompletionFunction (hthd, CompletionArg);
    }

}

typedef struct _ENABLE_NEXT_DLL_ARG
{
    INT                 iDllIndex;
    LPVOID              CompletionArg;
    COMPLETION_FUNCTION CompletionFunction;

} ENABLE_NEXT_DLL_ARG;



VOID
ActionEnableNextDll(
    LPDEBUG_EVENT64 pde,
    HTHDX   hthd,
    DWORDLONG   rval,   // unused
    DWORDLONG lparam
    )
/*++

Routine Description:

        It is necessary to call the DllDebugObjectRPCHook function in every dll
        which exports it.

--*/
{
    ENABLE_NEXT_DLL_ARG*    EnableNextDllArg = (ENABLE_NEXT_DLL_ARG*) lparam;
    int                     i = EnableNextDllArg->iDllIndex;

    for ( ; i < hthd->hprc->cDllList; i++) {
        if (hthd->hprc->rgDllList[i].fValidDll &&
            hthd->hprc->rgDllList[i].lpvOleRpc) {

            EnableNextDllArg->iDllIndex = i + 1;

            CallFunction (hthd,
                          ActionEnableNextDll,
                          (LPARAM) EnableNextDllArg,
                          FALSE,
                          (UINT_PTR)hthd->hprc->rgDllList[i].lpvOleRpc,
                          2,
                          0,
                          hthd->hprc->OrpcDebugging == ORPC_START_DEBUGGING
                              ? 1 : 0
                          );
            break;
        }
    }


    if (i < hthd->hprc->cDllList) {
        ContinueProcess(hthd->hprc);
    }

    //
    //  Done looping.  Deallocate the EnableNextDllArg structure
    //  and call the completion function.
    //

    if (i >= hthd->hprc->cDllList) {
        if (hthd->hprc->OrpcDebugging == ORPC_START_DEBUGGING) {
            hthd->hprc->OrpcDebugging = ORPC_DEBUGGING;
        } else {
            hthd->hprc->OrpcDebugging = ORPC_NOT_DEBUGGING;
        }

        EnableNextDllArg->CompletionFunction (hthd, EnableNextDllArg->CompletionArg);
        MHFree (EnableNextDllArg);
    }
}


BOOL
FEnableOleRpc(
    HTHDX               hthd,
    COMPLETION_FUNCTION CompletionFunction,
    LPVOID              CompletionArg
    )
/*++

Routine Description:

        Turns OLE RPC tracing on or off.

--*/
{
    DWORD                cb;
    DEBUG_EVENT64          de;
    ENABLE_NEXT_DLL_ARG* EnableNextDllArg = NULL;

    //
    //  If we are turning OLE on, then make sure the registry contains the
    //  key which allows ClientNotify and ServerNotify to work.
    //

    if (hthd->hprc->OrpcDebugging == ORPC_START_DEBUGGING &&
                hthd->hprc->orpcKeyState == orpcKeyNotChecked) {

        CHAR*   szDebugObjectRPCEnabled;
        HKEY    hkey;

        if (IsChicago()) {
            szDebugObjectRPCEnabled =
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "DebugObjectRPCEnabled";
        } else {
            szDebugObjectRPCEnabled =
                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\"
                "DebugObjectRPCEnabled";
        }

        //
        //      Ensure that the registry contains the key which will
        //      cause the ClientNotify notification to be sent.  (First
        //      try to open instead of trying to create, in case we have
        //      permission to open but not to create.)
        //

        if ((RegOpenKey(HKEY_LOCAL_MACHINE, szDebugObjectRPCEnabled,
                &hkey) != 0) &&
            (RegCreateKey(HKEY_LOCAL_MACHINE, szDebugObjectRPCEnabled,
                &hkey) != 0))
        {
            /* Couldnt create the key: tell the user */
            char    sz[256];

            if (!LoadString(hInstance, IDS_CannotEnableORPC, sz, sizeof(sz))) {
                assert(FALSE);
            }

            SendDBCError(hthd->hprc, xosdNone, sz);

            hthd->hprc->orpcKeyState = orpcKeyFailed;
        } else {
            hthd->hprc->orpcKeyState = orpcKeyEnabled;
            RegCloseKey(hkey);
        }
    }


    if (hthd->hprc->orpcKeyState == orpcKeyFailed) {
        return FALSE;
    }

    de.dwProcessId = hthd->hprc->pid;
    de.dwThreadId = hthd->tid;

    EnableNextDllArg = (ENABLE_NEXT_DLL_ARG*) MHAlloc (sizeof (*EnableNextDllArg));

    assert (EnableNextDllArg);

    if (!EnableNextDllArg) {
        return FALSE;
    }

    EnableNextDllArg->CompletionFunction = CompletionFunction;
    EnableNextDllArg->CompletionArg = CompletionArg;
    EnableNextDllArg->iDllIndex = 0;

    ActionEnableNextDll (NULL, hthd, 0, (UINT_PTR)EnableNextDllArg);

    return TRUE;
}


//-------------------------------------------------------------------------
// ReadOrpcDbgInfo
//
// PURPOSE:
//      Given an exception record which corresponds to an OLE debug
//      notification, reads in the structure which has the information
//      passed to use by OLE.
//
// INPUT:
//      hthd = thread
//      The exception record
//      A pointer to a buffer into which the information will be read.

// RETURNS:
//      TRUE - succesfully read the information.
//      FALSE - Could not read the reqd information.
//
//--------------------------------------------------------------------------

static
BOOL
ReadOrpcDbgInfo(
    HTHDX hthd,
    LPDEBUG_EVENT64 pde,
    ORPC_DBG_ALL * pOrpcInfo
    )
{
    DWORD cb;
    EXCEPTION_RECORD64 * lpExcpt = &(pde->u.Exception.ExceptionRecord);

    assert(lpExcpt->NumberParameters == 1);

    if (!DbgReadMemory(hthd->hprc,
                       (lpExcpt->ExceptionInformation[0]),
                       pOrpcInfo,
                       sizeof(ORPC_DBG_ALL),
                       &cb)) {
        return FALSE;
    }

    if ( cb != sizeof(ORPC_DBG_ALL) ) {
        return FALSE;
    }

    return TRUE;
}

//--------------------------------------------------------------------------
// OrpcFromPthd
//
// PURPOSE:
//      Given an exception record, determine if this exception was thrown to
//      notify the debugger of a Ole RPC event. If it is return the
//              notifications code otherwise, return orpcNil.
//
//      Note, one possible return value is orpcUnrecognized, which means
//      theres a notification but its one with which this version of MSVC
//      is not familiar.
//
// INPUT:
//      hthd = thread
//      The exception record.
//
// OUTPUT:
//      returns orpcNil if this it not a notification, or some other orpc*
//      code if it is a notification.
//
//--------------------------------------------------------------------------

ORPC
OrpcFromPthd (
    HTHDX hthd,
    LPDEBUG_EVENT64 pde
    )
{
    EXCEPTION_RECORD64 *  lpExcpt;
    ORPC_DBG_ALL orpc_all;
    ORPC orpc;
    int i;
    OLENOT olenot;
    DWORD cb;

    static struct
    {
        ORPC    orpc;   // notification
        GUID    guid;   // GUID that goes with it
    } rgorpcguids[] =   // array of mappings from orpc to guid
    {
        orpcClientGetBufferSize,
        { /* 9ED14F80-9673-101A-B07B-00DD01113F11 */
            0x9ED14F80,
            0x9673,
            0x101A,
            0xB0,
            0x7B,
            0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11
        },

        orpcClientFillBuffer,
        { /* DA45F3E0-9673-101A-B07B-00DD01113F11 */
            0xDA45F3E0,
            0x9673,
            0x101A,
            0xB0,
            0x7B,
            0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11
        },

        orpcServerNotify,
        { /* 1084FA00-9674-101A-B07B-00DD01113F11 */
            0x1084FA00,
            0x9674,
            0x101A,
            0xB0,
            0x7B,
            0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11
        },

        orpcServerGetBufferSize,
        { /* 22080240-9674-101A-B07B-00DD01113F11 */
            0x22080240,
            0x9674,
            0x101A,
            0xB0,
            0x7B,
            0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11
        },

        orpcServerFillBuffer,
        { /* 2FC09500-9674-101A-B07B-00DD01113F11 */
            0x2FC09500,
            0x9674,
            0x101A,
            0xB0,
            0x7B,
            0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11
        },

        orpcClientNotify,
        { /* 4F60E540-9674-101A-B07B-00DD01113F11 */
            0x4F60E540,
            0x9674,
            0x101A,
            0xB0,
            0x7B,
            0x00, 0xDD, 0x01, 0x11, 0x3F, 0x11
        },
    };
    #define cguidMax (sizeof(rgorpcguids) / sizeof(rgorpcguids[0]))

    assert(pde->dwDebugEventCode == EXCEPTION_DEBUG_EVENT);

    // We only consider first-chance exceptions as valid OLE events.
    if ( !pde->u.Exception.dwFirstChance ) {
        return orpcNil;
    }

    lpExcpt = &(pde->u.Exception.ExceptionRecord);

    assert(lpExcpt->ExceptionCode == EXCEPTION_ORPC_DEBUG);

    if ( lpExcpt->ExceptionCode != EXCEPTION_ORPC_DEBUG ) {
        return orpcNil;
    }


    // All OLE debug notifications include one parameter.
    if ( lpExcpt->NumberParameters != 1) {
        return orpcNil;
    }

    // Read in the structure which holds the
    // information about this ORPC debug event.

    if (!ReadOrpcDbgInfo(hthd, pde, &orpc_all)) {
        return orpcNil;
    }

    // Read in the signature. {
    if (!DbgReadMemory(hthd->hprc,
                       (UINT_PTR)orpc_all.pSignature,
                       &olenot,
                       sizeof(olenot),
                       &cb)) {
        return orpcNil;
    }

    // check for signature
    if (memcmp(olenot.rgbSig, "MARB", sizeof(olenot.rgbSig)) != 0) {
        return orpcNil;
    }

    orpc = orpcUnrecognized;

    // check count of bytes: all currently existing ORPCs need a
    // count of 0
    if (olenot.cb == 0) {
        // check all GUIDs looking for a match
        for (i = 0; i < cguidMax; ++i) {
            if (memcmp(&olenot.guid, &rgorpcguids[i].guid, sizeof(GUID)) == 0) {
                orpc = rgorpcguids[i].orpc;
                break;
            }
        }
    }

    return orpc;
}


void
PushOleRetAddr(
    HTHDX       hthd,
    UOFFSET     uoffRet,
    UOFFSET     dwEsp
    )
{
    OLERET *    poleret;

    poleret = MHAlloc(sizeof(OLERET));
    if (poleret) {
        poleret->poleretNext = hthd->poleret;
        poleret->uoffRet     = uoffRet;
        poleret->dwEsp       = dwEsp;

        hthd->poleret = poleret;
    }
}

void
PopOleRetAddr(
    HTHDX hthd
    )
{
    OLERET *    poleret;

    if (hthd->poleret) {
        poleret = hthd->poleret;
        hthd->poleret = hthd->poleret->poleretNext;
        MHFree(poleret);
    }
}

UOFFSET
UoffOleRet(
    HTHDX       hthd
    )
{
    if (hthd->poleret) {
        return hthd->poleret->uoffRet;
    } else {
        return 0;
    }
}

UOFFSET
EspOleRet(
    HTHDX   hthd
    )
{
    if (hthd->poleret) {
        return hthd->poleret->dwEsp;
    } else {
        return 0;
    }
}


VOID
OleReturnEventContinue(
    HTHDX   hthd,
    LPVOID  unused
    );

VOID
ActionOleReturnEvent(
    DEBUG_EVENT64 *pde,
    HTHDX        hthd,
    DWORDLONG        unused,
    DWORDLONG    lparam
    )
/*++

Routine Description:

        If we are now at the return address of an OLE RPC call, then tell OLE to
        turn on OLE debugging, and register an expected event for a
        ServerGetBufferSize OLE notification, and continue running.


--*/
{
    if (PC(hthd) == UoffOleRet(hthd)) {
        //
        //      NOTE: We dont deal correctly with the case where the
        //      user sets a breakpoint at the same address.
        //

        ADDR    addr = {0};

        if ( STACK_POINTER(hthd) >= EspOleRet(hthd) ) {
            addr.addr.off = UoffOleRet(hthd);
            PopOleRetAddr(hthd);

            RemoveBP((BREAKPOINT*)lparam);

            //
            //      User may have turned off OLE debugging since this breakpoint
            //      was set, so check.
            //

            EnsureOleRpcStatus (hthd, OleReturnEventContinue, NULL);

        } else {
            //
            //  We hit the breakpoint at some recursive instance.
            //  Step over the instruction and continue execution.

            if (fOleRpc) {

#if 1
                assert(FALSE);
#else
                EXPECTED_EVENT *pexev;
                EXOP exope = {0};
                BOOL fOk;

                exope.fFirstTime = TRUE;

                pexev = SaveEvents();

                fOk = DMSStep(hthd->hprc->hpid, hthd->htid, exope);

                RestoreEvents(pexev);

                if ( fOk ) {
                    ContinueThread(hthd);
                    return;
                }
#endif
            } else {
                RemoveBP((BREAKPOINT*)lparam);
            }
        }
    }
}


VOID
OleReturnEventContinue(
    HTHDX   hthd,
    LPVOID  unused
    )
/*++

Routine Description;

    Continuation function from ActionOleReturnEvent ().  Check if we are
    still OLE debugging, and if so, setup for the OrpcServerGetBufferSize
    event.

Arguments:


Return Value:

--*/
{
    if (hthd->hprc->OrpcDebugging == ORPC_DEBUGGING) {
        RegisterExpectedEvent (hthd->hprc,
                               hthd,
                               OLE_DEBUG_EVENT,
                               orpcServerGetBufferSize,
                               DONT_NOTIFY,
                               ActionOrpcServerGetBufferSize,
                               FALSE,
                               TRUE
                               );

        ContinueThread(hthd);
    }
}

BOOL
CheckAndSetupForOrpcSection(
    HTHDX   hthd
    )
/*++

Routine Description:

    Check if we are stepping out of an ole server call back into the .orpc
    section.  If so, our "step" really wants to be a "go".  Setup for the go
    and return.

    This function must be called upon entry into StepOver and SingleStep and
    any other functions of this nature.


--*/
{
    ADDR    CurrentAddress;
    UOFFSET ReturnDestination;


    AddrFromHthdx (&CurrentAddress, hthd);

    if (IsRet (hthd, &CurrentAddress)) {
        ReturnDestination = GetReturnDestination (hthd);

        if (ReturnDestination == UoffOleRet (hthd)) {

            //
            // clean up the extra breakpoint !!??
            //

            PopOleRetAddr (hthd);

            //
            // check if user still has ole debugging on
            //

            RegisterExpectedEvent (hthd->hprc,
                                   hthd,
                                   OLE_DEBUG_EVENT,
                                   orpcServerGetBufferSize,
                                   DONT_NOTIFY,
                                   ActionOrpcServerGetBufferSize,
                                   FALSE,
                                   FALSE
                                   );

            //
            // ???
            //
            ContinueProcess (hthd->hprc);

            return TRUE;
        }
    }

    return FALSE;
}
