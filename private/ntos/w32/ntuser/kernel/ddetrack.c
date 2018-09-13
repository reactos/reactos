/****************************** Module Header *******************************
* Module Name: ddetrack.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module handles tracking of DDE conversations for use in emulating
* DDE shared memory.
*
* History:
* 9-3-91      sanfords  Created
* 21-Jan-1992 IanJa     ANSI/Unicode netralized (null op)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PPUBOBJ gpPublicObjectList;

#define TRACE_DDE(str)          TAGMSG0(DBGTAG_DDE, str)
#define TRACE_DDE1(s, a)        TAGMSG1(DBGTAG_DDE, (s), (a))
#define TRACE_DDE2(s, a, b)     TAGMSG2(DBGTAG_DDE, (s), (a), (b))
#define TRACE_DDE3(s, a, b, c)  TAGMSG3(DBGTAG_DDE, (s), (a), (b), (c))

BOOL NewConversation(PDDECONV *ppdcNewClient, PDDECONV *ppdcNewServer,
        PWND pwndClient, PWND pwndServer);
PDDECONV FindDdeConv(PWND pwndProp, PWND pwndPartner);
BOOL AddConvProp(PWND pwndUs, PWND pwndThem, DWORD flags, PDDECONV pdcNew,
        PDDECONV pdcPartner);
FNDDERESPONSE xxxUnexpectedServerPost;
FNDDERESPONSE xxxUnexpectedClientPost;
FNDDERESPONSE xxxAdvise;
FNDDERESPONSE xxxAdviseAck;
FNDDERESPONSE xxxAdviseData;
FNDDERESPONSE xxxAdviseDataAck;
DWORD Unadvise(PDDECONV pDdeConv);
FNDDERESPONSE xxxUnadviseAck;
DWORD Request(PDDECONV pDdeConv);
FNDDERESPONSE xxxRequestAck;
FNDDERESPONSE xxxPoke;
FNDDERESPONSE xxxPokeAck;
FNDDERESPONSE xxxExecute;
FNDDERESPONSE xxxExecuteAck;
DWORD SpontaneousTerminate(PDWORD pmessage, PDDECONV pDdeConv);
FNDDERESPONSE DupConvTerminate;

HANDLE AnticipatePost(PDDECONV pDdeConv, FNDDERESPONSE fnResponse,
        HANDLE hClient, HANDLE hServer, PINTDDEINFO pIntDdeInfo, DWORD flags);
PXSTATE Createpxs(FNDDERESPONSE fnResponse, HANDLE hClient, HANDLE hServer,
        PINTDDEINFO pIntDdeInfo, DWORD flags);
DWORD AbnormalDDEPost(PDDECONV pDdeConv, DWORD message);
DWORD xxxCopyDdeIn(HANDLE hSrc, PDWORD pflags, PHANDLE phDirect, PINTDDEINFO *ppi);
DWORD xxxCopyAckIn(PDWORD pmessage, LPARAM *plParam, PDDECONV pDdeConv, PINTDDEINFO *ppIntDdeInfo);
HANDLE xxxCopyDDEOut(PINTDDEINFO pIntDdeInfo, PHANDLE phDirect);
BOOL FreeListAdd(PDDECONV pDdeConv, HANDLE hClient, DWORD flags);
VOID xxxFreeListFree(PFREELIST pfl);
VOID PopState(PDDECONV pDdeConv);
PDDECONV UnlinkConv(PDDECONV pDdeConv);

VOID FreeDDEHandle(PDDECONV pDdeConv, HANDLE hClient, DWORD flags);
DWORD ClientFreeDDEHandle(HANDLE hClient, DWORD flags);
DWORD ClientGetDDEFlags(HANDLE hClient, DWORD flags);
DWORD xxxClientCopyDDEIn1(HANDLE hClient, DWORD flags, PINTDDEINFO *ppi);
HANDLE xxxClientCopyDDEOut1(PINTDDEINFO pIntDdeInfo);
DWORD xxxClientCopyDDEOut2(PINTDDEINFO pIntDdeInfo);

PPUBOBJ IsObjectPublic(HANDLE hObj);
BOOL AddPublicObject(UINT format, HANDLE hObj, W32PID pid);
BOOL RemovePublicObject(UINT format, HANDLE hObj);
BOOL GiveObject(UINT format, HANDLE hObj, W32PID pid);
#if DBG
VOID ValidatePublicObjectList(VOID);
#define MSG_SENT    0
#define MSG_POST    1
#define MSG_RECV    2
#define MSG_PEEK    3
VOID TraceDdeMsg(UINT msg, HWND hwndFrom, HWND hwndTo, UINT code);
#else
#define ValidatePublicObjectList()
#define TraceDdeMsg(m, h1, h2, c)
#endif // DBG

/*
 *  The Big Picture:
 *
 *  When a WM_DDE_ACK message is SENT, it implies the begining of a DDE
 *    Conversation.  The tracking layer creates DDECONV structure for each
 *    window in volved in the conversation and cross links the structures.
 *    Thus a unique window pair identifies a conversation.  Each window has
 *    its DDECONV structure attached to it via a private property.
 *
 *  As DDE messages are posted, the tracking layer copies data into the
 *    CSR server side of USER into a INTDDEINFO structure.  This structure
 *    contains flags which direct how the data is to be freed when the
 *    time comes.  This info is placed within an XSTATE structure along
 *    with context infomation.  A pointer to the XSTATE structure is
 *    placed in the lParam of the message and the MSB of message is set
 *    for special processing when the message is recieved on the other side.
 *
 *  If the message posted requires a responding message to follow the DDE
 *    protocol, a XSTATE structure is created and attached to DDECONV
 *    structure associated with the window that is expected to post the message.
 *    The XSTATE structure directs the tracking layer so that it knows the
 *    context of the message when it is posted and also includes any
 *    information needed for proper freeing of extra DDE data.
 *
 *  When the message is extracted from the queue either by a hook, peek,
 *    or by GetMessage, the id is checked to see if it lies in the special
 *    range.  If so, the XSTATE structure pointed to by the lParam is
 *    operated on.  This causes the data to be copied from the CSR server
 *    side of USER to the target process context.  Once this is done, the
 *    XSTATE structure may or may not be freed depending on flags and
 *    the message is restored to a proper DDE message form ready to be
 *    used by the target process.  Since the message id is changed back,
 *    subsequent peeks or hooks to the message will not result in duplicated
 *    processing of the message.
 *
 *  During the course of come transactions it becomes evident that an object
 *    on the opposite side process needs to be freed.  This is done
 *    asynchronously by inserting the object that needs freeing along with
 *    associated flags into a freeing list which is tied to the DDECONV
 *    structure associated with the window on the opposite side.  Whenever
 *    a DDE messages is posted, this freeing list is checked and processed.
 *
 *  When a WM_DDE_TERMINATE message is finally recieved, flags are set
 *    in the DDECONV structure indicating that the conversation is terminating.
 *    This alters the way the mapping layer handles DDE messages posted.
 *    When the responding side posts a WM_DDE_TERMINATE, the DDECONV structures
 *    and all associated information is freed and unlinked from the windows
 *    concerned.
 *
 *  Should a DDE window get destroyed before proper termination, the
 *    xxxDDETrackWindowDying function is called to make sure proper termination
 *    is done prior to the window being destroyed.
 */


/************************************************************************
* xxxDDETrackSendHook
*
* Called when a DDE message is passed to SendMessage().
*
* Returns fSendOk.
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
BOOL xxxDDETrackSendHook(
PWND pwndTo,
DWORD message,
WPARAM wParam,
LPARAM lParam)
{
    PWND pwndServer;
    PDDECONV pdcNewClient, pdcNewServer;

    if (MonitorFlags & MF_SENDMSGS) {
        DDEML_MSG_HOOK_DATA dmhd;

        dmhd.cbData = 0;    // Initiate and Ack sent messages have no data.
        dmhd.uiLo = LOWORD(lParam);     // they arn't packed either.
        dmhd.uiHi = HIWORD(lParam);
        xxxMessageEvent(pwndTo, message, wParam, lParam, MF_SENDMSGS, &dmhd);
    }

    if (PtiCurrent()->ppi == GETPWNDPPI(pwndTo)) {
        /*
         * Skip monitoring of all intra-process conversations.
         */
        return(TRUE);
    }

    if (message != WM_DDE_ACK) {
        if (message == WM_DDE_INITIATE) {
            return TRUE;     // this is cool
        }
        return(FALSE);
    }

    pwndServer = ValidateHwnd((HWND)wParam);
    if (pwndServer == NULL) {
        return(FALSE);
    }

    pdcNewServer = FindDdeConv(pwndServer, pwndTo);
    if (pdcNewServer != NULL) {
        RIPMSG2(RIP_WARNING,
                "DDE protocol violation - non-unique window pair (%#p:%#p)",
                PtoH(pwndTo), PtoH(pwndServer));
        /*
         * Duplicate Conversation case:
         *  Don't allow the ACK to pass, post a terminate to the server
         *  to shut down the duplicate on his end.
         */
        AnticipatePost(pdcNewServer, DupConvTerminate, NULL, NULL, NULL, 0);
        _PostMessage(pwndServer, WM_DDE_TERMINATE, (WPARAM)PtoH(pwndTo), 0);
        return(FALSE);
    }

    if (!NewConversation(&pdcNewClient, &pdcNewServer, pwndTo, pwndServer)) {
        return(FALSE);
    }

    TRACE_DDE2("%#p->%#p DDE Conversation started", PtoH(pwndTo), wParam);
    return(TRUE);
}


/************************************************************************
* AddConvProp
*
* Helper for xxxDDETrackSendHook - associates a new DDECONV struct with
* a window and initializes it.
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
BOOL AddConvProp(
PWND pwndUs,
PWND pwndThem,
DWORD flags,
PDDECONV pdcNew,
PDDECONV pdcPartner)
{
    PDDECONV pDdeConv;
    PDDEIMP pddei;

    pDdeConv = (PDDECONV)_GetProp(pwndUs, PROP_DDETRACK, PROPF_INTERNAL);
    Lock(&(pdcNew->snext), pDdeConv);
    Lock(&(pdcNew->spwnd), pwndUs);
    Lock(&(pdcNew->spwndPartner), pwndThem);

    /*
     * Assert to catch stress bug.
     */
    UserAssert(pdcPartner != (PDDECONV)(-1));

    Lock(&(pdcNew->spartnerConv), pdcPartner);
    pdcNew->spxsIn = NULL;
    pdcNew->spxsOut = NULL;
    pdcNew->flags = flags;
    pddei = (PDDEIMP)_GetProp((flags & CXF_IS_SERVER) ?
            pwndThem : pwndUs, PROP_DDEIMP, PROPF_INTERNAL);
    if (pddei != NULL) {    // This can be NULL if a bad WOW app has been
        pddei->cRefConv++;  // allowed through for compatability.
    }
    pdcNew->pddei = pddei;

    HMLockObject(pdcNew);         // lock for property
    InternalSetProp(pwndUs, PROP_DDETRACK, pdcNew, PROPF_INTERNAL);
    return(TRUE);
}


/************************************************************************
* UnlinkConv
*
* Unlinks a DDECONV structure from the property list it is associated with.
*
* returns pDdeConv->snext
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
PDDECONV UnlinkConv(
PDDECONV pDdeConv)
{
    PDDECONV pdcPrev, pdcT, pDdeConvNext;

    /*
     * Already unlinked
     */
    if (pDdeConv->spwnd == NULL) {
        return(NULL);
    }
    TRACE_DDE1("UnlinkConv(%#p)", pDdeConv);

    pdcT = (PDDECONV)_GetProp(pDdeConv->spwnd,
            PROP_DDETRACK, PROPF_INTERNAL);
    if (pdcT == NULL) {
        return(NULL);             // already unlinked
    }

    pdcPrev = NULL;
    while (pdcT != pDdeConv) {
        pdcPrev = pdcT;
        pdcT = pdcT->snext;
        if (pdcT == NULL) {
            return(NULL);        // already unlinked
        }
    }

    if (pdcPrev == NULL) {
        if (pDdeConv->snext == NULL) {
            // last one out removes the property
            InternalRemoveProp(pDdeConv->spwnd, PROP_DDETRACK, PROPF_INTERNAL);
        } else {
            // head conv unlinked - update prop
            InternalSetProp(pDdeConv->spwnd, PROP_DDETRACK, pDdeConv->snext,
                    PROPF_INTERNAL);
        }
    } else {
        Lock(&(pdcPrev->snext), pDdeConv->snext);
    }
    pDdeConvNext = Unlock(&(pDdeConv->snext));
    HMUnlockObject(pDdeConv);      // unlock for property detachment
    return(pDdeConvNext);
}


/************************************************************************
* xxxDDETrackPostHook
*
* Hook function for handling posted DDE messages.
*
* returns post action code - DO_POST, FAKE_POST, FAIL_POST.
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
DWORD xxxDDETrackPostHook(
PUINT pmessage,
PWND pwndTo,
WPARAM wParam,
LPARAM *plParam,
BOOL fSent)
{
    PWND pwndFrom;
    PDDECONV pDdeConv = NULL;
    DWORD dwRet;
    TL tlpDdeConv;
    PFREELIST pfl, *ppfl;
    DWORD MFlag;

    CheckLock(pwndTo);

    MFlag = fSent ? MF_SENDMSGS : MF_POSTMSGS;
    if (MonitorFlags & MFlag) {
        DDEML_MSG_HOOK_DATA dmhd;

        switch (*pmessage ) {
        case WM_DDE_DATA:
        case WM_DDE_POKE:
        case WM_DDE_ADVISE:
        case WM_DDE_EXECUTE:
        case WM_DDE_ACK:
            ClientGetDDEHookData(*pmessage, *plParam, &dmhd);
            break;

        default:
            // WM_DDE_REQUEST
            // WM_DDE_TERMINATE
            // WM_DDE_UNADVISE
            dmhd.cbData = 0;
            dmhd.uiLo = LOWORD(*plParam);
            dmhd.uiHi = HIWORD(*plParam);
        }
        xxxMessageEvent(pwndTo, *pmessage, wParam, *plParam, MFlag,
                &dmhd);
    }

    if (PtiCurrent()->ppi == GETPWNDPPI(pwndTo)) {
        /*
         * skip all intra-process conversation tracking.
         */
        dwRet = DO_POST;
        goto Exit;
    }

    if (*pmessage == WM_DDE_INITIATE) {
        RIPMSG2(RIP_WARNING,
                "DDE Post failed (%#p:%#p) - WM_DDE_INITIATE posted",
                wParam, PtoH(pwndTo));
        dwRet = FAIL_POST;
        goto Exit;
    }

    pwndFrom = ValidateHwnd((HWND)wParam);
    if (pwndFrom == NULL) {
        /*
         * This is a post AFTER a window has been destroyed.  This is not
         * expected except in the case where xxxDdeTrackWindowDying()
         * is posting a cleanup terminate.
         */
        dwRet = *pmessage == WM_DDE_TERMINATE ? DO_POST : FAKE_POST;
        goto Exit;
    }

    /*
     * locate conversation info.
     */
    pDdeConv = FindDdeConv(pwndFrom, pwndTo);
    if (pDdeConv == NULL) {
        if (*pmessage != WM_DDE_TERMINATE &&
                (GETPTI(pwndFrom)->TIF_flags & TIF_16BIT) &&
                (pwndTo->head.rpdesk == pwndFrom->head.rpdesk)) {
            /*
             * If a WOW app bypasses initiates and posts directly to
             * a window on the same desktop, let it sneak by here.
             *
             * This allows some evil apps such as OpenEngine and CA-Cricket
             * to get away with murder.
             *
             * TERMINATES out of the blue however may be due to an app
             * posting its WM_DDE_TERMINATE after it has destroyed its
             * window.  Since window destruction would have generated the
             * TERMINATE already, don't let it through here.
             */
            NewConversation(&pDdeConv, NULL, pwndFrom, pwndTo);
        }
        if (pDdeConv == NULL) {
            RIPMSG2(RIP_VERBOSE, "Can't find DDE conversation for (%#p:%#p).",
                    wParam, PtoH(pwndTo));
            dwRet = *pmessage == WM_DDE_TERMINATE ? FAKE_POST : FAIL_POST;
            goto Exit;
        }
    }

    if (fSent && pDdeConv->spartnerConv->spxsOut != NULL) {
        /*
         * Sent DDE messages will not work if any posted DDE messages are
         * in the queue because this will violate the message ordering rule.
         */
        RIPMSG0(RIP_VERBOSE,
                "Sent DDE message failed - queue contains a previous post.");
        dwRet = FAIL_POST;
        goto Exit;
    }

    /*
     * The tracking layer never did allow multiple threads to handle
     * the same DDE conversation but win95 shipped and some apps
     * got out there that did just this.  We will let it slide for
     * 4.0 apps only so that when they rev their app, they will see
     * that they were wrong.
     */
    if (PtiCurrent() != GETPTI(pDdeConv) &&
            PtiCurrent()->dwExpWinVer != VER40) {
        RIPERR0(ERROR_WINDOW_OF_OTHER_THREAD,
                RIP_ERROR,
                "Posting DDE message from wrong thread!");

        dwRet = FAIL_POST;
        goto Exit;
    }

    ThreadLockAlways(pDdeConv, &tlpDdeConv);

    /*
     * If the handle we're using is in the free list, remove it
     */
    ppfl = &pDdeConv->pfl;
    while (*ppfl != NULL) {
        if ((*ppfl)->h == (HANDLE)*plParam) {
            /* Let's stop to check this out */
            UserAssert((*ppfl)->h == (HANDLE)*plParam);
            *ppfl = (*ppfl)->next;
        } else {
            ppfl = &(*ppfl)->next;
        }
    }
    pfl = pDdeConv->pfl;
    pDdeConv->pfl = NULL;
    xxxFreeListFree(pfl);

    if (*pmessage != WM_DDE_TERMINATE &&
            (pDdeConv->flags & (CXF_TERMINATE_POSTED | CXF_PARTNER_WINDOW_DIED))) {
        dwRet = FAKE_POST;
        goto UnlockExit;
    }

    if (pDdeConv->spxsOut == NULL) {
        if (pDdeConv->flags & CXF_IS_SERVER) {
            dwRet = xxxUnexpectedServerPost((PDWORD)pmessage, plParam, pDdeConv);
        } else {
            dwRet = xxxUnexpectedClientPost((PDWORD)pmessage, plParam, pDdeConv);
        }
    } else {
        dwRet = (pDdeConv->spxsOut->fnResponse)(pmessage, plParam, pDdeConv);
    }

UnlockExit:

    ThreadUnlock(&tlpDdeConv);

Exit:

    if (dwRet == FAKE_POST && !((PtiCurrent())->TIF_flags & TIF_INCLEANUP)) {
        /*
         * We faked the post so do a client side cleanup here so that we
         * don't make it appear there is a leak in the client app.
         */
        DWORD flags = XS_DUMPMSG;
        /*
         * The XS_DUMPMSG tells FreeDDEHandle to also free the atoms
         * associated with the data - since a faked post would make the app
         * think that the receiver was going to cleanup the atoms.
         * It also tells FreeDDEHandle to pay attention to the
         * fRelease bit when freeing the data - this way, loaned data
         * won't be destroyed.
         */

        switch (*pmessage & 0xFFFF) {
        case WM_DDE_UNADVISE:
        case WM_DDE_REQUEST:
            goto DumpMsg;

        case WM_DDE_ACK:
            flags |= XS_PACKED;
            goto DumpMsg;

        case WM_DDE_ADVISE:
            flags |= XS_PACKED | XS_HIHANDLE;
            goto DumpMsg;

        case WM_DDE_DATA:
        case WM_DDE_POKE:
            flags |= XS_DATA | XS_LOHANDLE | XS_PACKED;
            goto DumpMsg;

        case WM_DDE_EXECUTE:
            flags |= XS_EXECUTE;
            // fall through
DumpMsg:
            if (pDdeConv != NULL) {
                TRACE_DDE("xxxDdeTrackPostHook: dumping message...");
                FreeDDEHandle(pDdeConv, (HANDLE)*plParam, flags);
                dwRet = FAILNOFREE_POST;
            }
        }
    }
#if DBG
    if (fSent) {
        TraceDdeMsg(*pmessage, (HWND)wParam, PtoH(pwndTo), MSG_SENT);
    } else {
        TraceDdeMsg(*pmessage, (HWND)wParam, PtoH(pwndTo), MSG_POST);
    }
    if (dwRet == FAKE_POST) {
        TRACE_DDE("...FAKED!");
    } else if (dwRet == FAIL_POST) {
        TRACE_DDE("...FAILED!");
    } else if (dwRet == FAILNOFREE_POST) {
        TRACE_DDE("...FAILED, DATA FREED!");
    }
#endif // DBG
    return(dwRet);
}

void xxxCleanupDdeConv(
PWND pwndProp)
{
    PDDECONV pDdeConv;

Restart:

    CheckCritIn();

    pDdeConv = (PDDECONV)_GetProp(pwndProp, PROP_DDETRACK, PROPF_INTERNAL);
    
    while (pDdeConv != NULL) {
        if ((pDdeConv->flags & (CXF_IS_SERVER | CXF_TERMINATE_POSTED | CXF_PARTNER_WINDOW_DIED))
                == (CXF_IS_SERVER | CXF_TERMINATE_POSTED | CXF_PARTNER_WINDOW_DIED) &&
            
            (pDdeConv->spartnerConv->flags & CXF_TERMINATE_POSTED)) {
            
            /*
             * clean up client side objects on this side
             */
            BOOL fUnlockDdeConv;
            TL tlpDdeConv;

            RIPMSG1(RIP_VERBOSE, "xxxCleanupDdeConv %p", pDdeConv);

            fUnlockDdeConv = (pDdeConv->pfl != NULL);
            if (fUnlockDdeConv) {
                PFREELIST pfl;

                ThreadLockAlways(pDdeConv, &tlpDdeConv);

                pfl = pDdeConv->pfl;
                pDdeConv->pfl = NULL;
                xxxFreeListFree(pfl);
            }

            FreeDdeConv(pDdeConv->spartnerConv);
            FreeDdeConv(pDdeConv);

            if (fUnlockDdeConv) {
                ThreadUnlock(&tlpDdeConv);
            }
            
            /*
             * Take it back from the top. The list might have changed
             * if we left the critical section
             */
            goto Restart;
        }
        
        pDdeConv = pDdeConv->snext;
    }
}


/************************************************************************
* xxxDDETrackGetMessageHook
*
* This routine is used to complete an inter-process copy from the
* CSRServer context to the target context.  pmsg->lParam is a
* pxs that is used to obtain the pIntDdeInfo needed to
* complete the copy.  The pxs is either filled with the target side
* direct handle or is freed depending on the message and its context.
*
* The XS_FREEPXS bit of the flags field of the pxs tells this function
* to free the pxs when done.
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
VOID xxxDDETrackGetMessageHook(
PMSG pmsg)
{
    PXSTATE pxs;
    HANDLE hDirect;
    DWORD flags;
    BOOL fUnlockDdeConv;
    TL tlpDdeConv, tlpxs;

    TraceDdeMsg(pmsg->message, (HWND)pmsg->wParam, pmsg->hwnd, MSG_RECV);

    if (pmsg->message == WM_DDE_TERMINATE) {
        PWND pwndFrom, pwndTo;
        PDDECONV pDdeConv;

        pwndTo = ValidateHwnd(pmsg->hwnd);
        
        /*
         * We should get the pwnd even if the partner is destroyed in order
         * to clean up the DDE objects now.  Exiting now would work, but would
         * leave the conversation objects locked and present until the To window
         * gets destroyed, which seems excessive.
         */
        pwndFrom = RevalidateCatHwnd((HWND)pmsg->wParam);
        
        if (pwndTo == NULL) {
            TRACE_DDE("TERMINATE ignored, invalid window(s).");
            return;
        
        } else if (pwndFrom == NULL) {
            
CleanupAndExit:            
            /*
             * Do this only for appcompat
             */
            if (GetAppCompatFlags2(VERMAX) & GACF2_DDE) {
                xxxCleanupDdeConv(pwndTo);
            } else {
                TRACE_DDE("TERMINATE ignored, invalid window(s).");
            }
            return;
        }
        
        /*
         * locate conversation info.
         */
        pDdeConv = FindDdeConv(pwndTo, pwndFrom);
        if (pDdeConv == NULL) {
            /*
             * Must be a harmless extra terminate.
             */
            TRACE_DDE("TERMINATE ignored, conversation not found.");
            return;
        }

        if (pDdeConv->flags & CXF_TERMINATE_POSTED &&
                pDdeConv->spartnerConv->flags & CXF_TERMINATE_POSTED) {

            /*
             * clean up client side objects on this side
             */
            fUnlockDdeConv = FALSE;
            if (pDdeConv->pfl != NULL) {
                PFREELIST pfl;

                fUnlockDdeConv = TRUE;
                ThreadLockAlways(pDdeConv, &tlpDdeConv);
                pfl = pDdeConv->pfl;
                pDdeConv->pfl = NULL;
                xxxFreeListFree(pfl);
            }

            TRACE_DDE2("DDE conversation (%#p:%#p) closed",
                    (pDdeConv->flags & CXF_IS_SERVER) ? pmsg->wParam : (ULONG_PTR)pmsg->hwnd,
                    (pDdeConv->flags & CXF_IS_SERVER) ? (ULONG_PTR)pmsg->hwnd : pmsg->wParam);

            FreeDdeConv(pDdeConv->spartnerConv);
            FreeDdeConv(pDdeConv);

            if (fUnlockDdeConv) {
                ThreadUnlock(&tlpDdeConv);
            }
        }

        goto CleanupAndExit;
    }

    pxs = (PXSTATE)HMValidateHandleNoRip((HANDLE)pmsg->lParam, TYPE_DDEXACT);
    if (pxs == NULL) {
        /*
         * The posting window has died and the pxs was freed so this
         * message shouldn't be bothered with...map to WM_NULL.
         */
        pmsg->lParam = 0;
        pmsg->message = WM_NULL;
        return;
    }
    flags = pxs->flags;

    ThreadLockAlways(pxs, &tlpxs);
    pmsg->lParam = (LPARAM)xxxCopyDDEOut(pxs->pIntDdeInfo, &hDirect);
    if (pmsg->lParam == (LPARAM)NULL) {
        /*
         * Turn this message into a terminate - we failed to copy the
         * message data out which implies we are too low on memory
         * to continue the conversation.  Shut it down now before
         * other problems pop up that this failure will cause.
         */
        pmsg->message = WM_DDE_TERMINATE;
        RIPMSG0(RIP_WARNING, "DDETrack: couldn't copy data out, terminate faked.");
    }
    if (ThreadUnlock(&tlpxs) == NULL) {
        return;
    }

    if (flags & XS_FREEPXS) {
        FreeDdeXact(pxs);
        return;
    }

    /*
     * The only reason XS_FREEPXS isn't set is because we don't know which
     * side frees the data till an ACK comes back, thus one of the client
     * handles in pxs is already set via xxxDDETrackPostHook().  The one thats
     * not yet set gets set here.
     */

    if (pxs->hClient == NULL) {
        TRACE_DDE1("Saving %#p into hClient", hDirect);
        pxs->hClient = hDirect;
    } else {
        TRACE_DDE1("Saving %#p into hServer.", hDirect);
        pxs->hServer = hDirect;
    }
    return;
}



/************************************************************************
* xxxDDETrackWindowDying
*
* Called when a window with PROP_DDETRACK is destroyed.
*
* This posts a terminate to the partner window and sets up for proper
* terminate post fake from other end.
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
VOID xxxDDETrackWindowDying(
PWND pwnd,
PDDECONV pDdeConv)
{
    TL tlpDdeConv, tlpDdeConvNext;

    UNREFERENCED_PARAMETER(pwnd);

    CheckLock(pwnd);
    CheckLock(pDdeConv);

    TRACE_DDE2("xxxDDETrackWindowDying(%#p, %#p)", PtoH(pwnd), pDdeConv);

    while (pDdeConv != NULL) {

        PFREELIST pfl;

        /*
         * If there are any active conversations for this window
         * start termination if not already started.
         */
        if (!(pDdeConv->flags & CXF_TERMINATE_POSTED)) {
            /*
             * Win9x doesn't do any tracking. This breaks some apps that
             *  destroy the window first and then post the terminate. The
             *  other side gets two terminates.
             */
            if (!(GACF2_NODDETRKDYING & GetAppCompatFlags2(VER40))
                || (pDdeConv->spwndPartner == NULL)
                || !(GACF2_NODDETRKDYING
                        & GetAppCompatFlags2ForPti(GETPTI(pDdeConv->spwndPartner), VER40))) {

                /*
                 * CXF_TERMINATE_POSTED would have been set if the window had died.
                 */
                _PostMessage(pDdeConv->spwndPartner, WM_DDE_TERMINATE,
                        (WPARAM)PtoH(pDdeConv->spwnd), 0);
                // pDdeConv->flags |= CXF_TERMINATE_POSTED;  set by PostHookProc
            } else {
                RIPMSG2(RIP_WARNING, "xxxDDETrackWindowDying(GACF2_NODDETRKDYING) not posting terminate from %#p to %#p\r\n",
                        pwnd, pDdeConv->spwndPartner);
            }
        }

        /*
         * now fake that the other side already posted a terminate since
         * we will be gone.
         */
        pDdeConv->spartnerConv->flags |=
                CXF_TERMINATE_POSTED | CXF_PARTNER_WINDOW_DIED;

        ThreadLock(pDdeConv->snext, &tlpDdeConvNext);
        ThreadLockAlways(pDdeConv, &tlpDdeConv);

        pfl = pDdeConv->pfl;
        pDdeConv->pfl = NULL;

        if (pDdeConv->flags & CXF_PARTNER_WINDOW_DIED) {

            ThreadUnlock(&tlpDdeConv);
            /*
             * he's already gone, free up conversation tracking data
             */
            FreeDdeConv(pDdeConv->spartnerConv);
            FreeDdeConv(pDdeConv);
        } else {
            UnlinkConv(pDdeConv);
            ThreadUnlock(&tlpDdeConv);
        }
        xxxFreeListFree(pfl);

        pDdeConv = ThreadUnlock(&tlpDdeConvNext);
    }
}



/************************************************************************
* xxxUnexpectedServerPost
*
* Handles Server DDE messages not anticipated. (ie spontaneous or abnormal)
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
DWORD xxxUnexpectedServerPost(
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    switch (*pmessage) {
    case WM_DDE_TERMINATE:
        return(SpontaneousTerminate(pmessage, pDdeConv));

    case WM_DDE_DATA:
        return(xxxAdviseData(pmessage, plParam, pDdeConv));

    case WM_DDE_ACK:

        /*
         * Could be an extra NACK due to timeout problems, just fake it.
         */
        TRACE_DDE("xxxUnexpectedServerPost: dumping ACK data...");
        FreeDDEHandle(pDdeConv, (HANDLE)*plParam, XS_PACKED);
        return(FAILNOFREE_POST);

    case WM_DDE_ADVISE:
    case WM_DDE_UNADVISE:
    case WM_DDE_REQUEST:
    case WM_DDE_POKE:
    case WM_DDE_EXECUTE:
        return(AbnormalDDEPost(pDdeConv, *pmessage));
    }
    return 0;
}



/************************************************************************
* xxxUnexpectedClientPost
*
*
* Handles Client DDE messages not anticipated. (ie spontaneous or abnormal)
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
DWORD xxxUnexpectedClientPost(
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    switch (*pmessage) {
    case WM_DDE_TERMINATE:
        return(SpontaneousTerminate(pmessage, pDdeConv));

    case WM_DDE_ACK:

        /*
         * Could be an extra NACK due to timeout problems, just fake it.
         */
        TRACE_DDE("xxxUnexpectedClientPost: dumping ACK data...");
        FreeDDEHandle(pDdeConv, (HANDLE)*plParam, XS_PACKED);
        return(FAILNOFREE_POST);

    case WM_DDE_DATA:
        return(AbnormalDDEPost(pDdeConv, *pmessage));

    case WM_DDE_ADVISE:
        return(xxxAdvise(pmessage, plParam, pDdeConv));

    case WM_DDE_UNADVISE:
        return(Unadvise(pDdeConv));

    case WM_DDE_REQUEST:
        return(Request(pDdeConv));

    case WM_DDE_POKE:
        return(xxxPoke(pmessage, plParam, pDdeConv));

    case WM_DDE_EXECUTE:
        return(xxxExecute(pmessage, plParam, pDdeConv));
    }
    return 0;
}



/************************************************************************
*                   ADVISE TRANSACTION PROCESSING                       *
\***********************************************************************/



DWORD xxxAdvise(            // Spontaneous Client transaction = WM_DDE_ADVISE
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    PINTDDEINFO pIntDdeInfo;
    HANDLE hDirect;
    DWORD flags, dwRet;

    CheckLock(pDdeConv);

    TRACE_DDE("xxxAdvise");
    flags = XS_PACKED | XS_LOHANDLE;
    dwRet = xxxCopyDdeIn((HANDLE)*plParam, &flags, &hDirect, &pIntDdeInfo);
    if (dwRet == DO_POST) {
        UserAssert(pIntDdeInfo != NULL);
        *pmessage |= MSGFLAG_DDE_MID_THUNK;
        *plParam = (LPARAM)AnticipatePost(pDdeConv->spartnerConv, xxxAdviseAck,
             hDirect, NULL, pIntDdeInfo, flags);
        if (*plParam == 0) {
            dwRet = FAILNOFREE_POST;
        }
    }
    return dwRet;
}

/*
 * If its inter-process:
 *
 * xxxDDETrackGetMessageHook() fills in hServer from pIntDdeInfo when WM_DDE_ADVISE
 * is received. pIntDdeInfo is then freed. The hServer handle is saved into the
 * pxs structure pointed to by lParam is a direct data structure since
 * packed DDE messages are always assumed to have the packing handle freed.
 */


DWORD xxxAdviseAck(         // Server response to advise - WM_DDE_ACK expected
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    PXSTATE pxsFree;
    PINTDDEINFO pIntDdeInfo;
    DWORD dwRet;

    CheckLock(pDdeConv);

    if (*pmessage != WM_DDE_ACK) {
        return(xxxUnexpectedServerPost(pmessage, plParam, pDdeConv));
    }

    TRACE_DDE("xxxAdviseAck");

    dwRet = xxxCopyAckIn(pmessage, plParam, pDdeConv, &pIntDdeInfo);
    if (dwRet != DO_POST) {
        return dwRet;
    }
    UserAssert(pIntDdeInfo != NULL);

    pxsFree = pDdeConv->spxsOut;
    if (pIntDdeInfo->DdePack.uiLo & DDE_FACK) {

        /*
         * positive ack implies server accepted the hOptions data - free from
         * client at postmessage time.
         */
        TRACE_DDE("xxxAdviseAck: +ACK delayed freeing data from client");
        FreeListAdd(pDdeConv->spartnerConv, pxsFree->hClient, pxsFree->flags & ~XS_PACKED);
    } else {
        // Shouldn't this be freed directly?
        TRACE_DDE("xxxAdviseAck: -ACK delayed freeing data from server");
        FreeListAdd(pDdeConv, pxsFree->hServer, pxsFree->flags & ~XS_PACKED);
    }

    PopState(pDdeConv);
    return(DO_POST);
}



/************************************************************************
*                  ADVISE DATA TRANSACTION PROCESSING                   *
\***********************************************************************/



DWORD xxxAdviseData(        // spontaneous from server - WM_DDE_DATA
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    DWORD flags, dwRet;
    PINTDDEINFO pIntDdeInfo;
    HANDLE hDirect;
    PXSTATE pxs;

    CheckLock(pDdeConv);

    TRACE_DDE("xxxAdviseData");

    flags = XS_PACKED | XS_LOHANDLE | XS_DATA;

    dwRet = xxxCopyDdeIn((HANDLE)*plParam, &flags, &hDirect, &pIntDdeInfo);
    if (dwRet == DO_POST) {
        UserAssert(pIntDdeInfo != NULL);
        TRACE_DDE1("xxxAdviseData: wStatus = %x",
                ((PDDE_DATA)(pIntDdeInfo + 1))->wStatus);
        if (!(((PDDE_DATA)(pIntDdeInfo + 1))->wStatus & (DDE_FACK | DDE_FRELEASE))) {
            RIPMSG0(RIP_ERROR, "DDE protocol violation - no RELEASE or ACK bit set - setting RELEASE.");
            ((PDDE_DATA)(pIntDdeInfo + 1))->wStatus |= DDE_FRELEASE;
        }
        if (((PDDE_DATA)(pIntDdeInfo + 1))->wStatus & DDE_FRELEASE) {
            /*
             * giving it away
             */
            if (IsObjectPublic(pIntDdeInfo->hIndirect) != NULL) {
                RIPMSG0(RIP_ERROR, "DDE Protocol violation - giving away a public GDI object.");
                UserFreePool(pIntDdeInfo);
                return(FAILNOFREE_POST);
            }
            if (GiveObject(((PDDE_DATA)(pIntDdeInfo + 1))->wFmt,
                    pIntDdeInfo->hIndirect,
                    (W32PID)(GETPTI(pDdeConv->spwndPartner)->ppi->W32Pid))) {
                flags |= XS_GIVEBACKONNACK;
            }
            flags |= XS_FRELEASE;
        } else {
            /*
             * on loan
             */
            if (AddPublicObject(((PDDE_DATA)(pIntDdeInfo + 1))->wFmt,
                        pIntDdeInfo->hIndirect,
                        (W32PID)(GETPTI(pDdeConv->spwnd)->ppi->W32Pid))) {
                flags |= XS_PUBLICOBJ;
            }
        }

        *pmessage |= MSGFLAG_DDE_MID_THUNK;
        if (((PDDE_DATA)(pIntDdeInfo + 1))->wStatus & DDE_FACK) {
            *plParam = (LPARAM)AnticipatePost(pDdeConv->spartnerConv,
                xxxAdviseDataAck, NULL, hDirect, pIntDdeInfo, flags);
        } else {
            TRACE_DDE("xxxAdviseData: dumping non Ackable data...");
            UserAssert(hDirect != (HANDLE)*plParam);
            FreeDDEHandle(pDdeConv, hDirect, flags & ~XS_PACKED);
            pxs = Createpxs(NULL, NULL, NULL, pIntDdeInfo, flags | XS_FREEPXS);
            if (pxs != NULL) {
                pxs->head.pti = GETPTI(pDdeConv->spwndPartner);
            }
            *plParam = (LPARAM)PtoH(pxs);
        }
        if (*plParam == 0) {
            dwRet = FAILNOFREE_POST;
        }
    }
    return dwRet;
}


/*
 * If its inter-process:
 *
 * xxxDDETrackGetMessageHook() completes the copy from pIntDdeInfo when WM_DDE_DATA
 * is received. pIntDdeInfo is then freed. The hServer handle saved into the
 * pxs structure pointed to by lParam is a directdata structure since
 * packed DDE messages are always assumed to have the packing handle freed
 * by the receiving app.
 * For the !fAckReq case, the pxs is freed due to the XS_FREEPXS flag.
 */


DWORD xxxAdviseDataAck(     // Client response to advise data - WM_DDE_ACK expected
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    PXSTATE pxsFree;
    PINTDDEINFO pIntDdeInfo;
    DWORD dwRet;

    CheckLock(pDdeConv);

    /*
     * This is also used for request data ack processing.
     */
    if (*pmessage != WM_DDE_ACK) {
        return(xxxUnexpectedClientPost(pmessage, plParam, pDdeConv));
    }

    TRACE_DDE("xxxAdviseDataAck");

    dwRet = xxxCopyAckIn(pmessage, plParam, pDdeConv, &pIntDdeInfo);
    if (dwRet != DO_POST) {
        return dwRet;
    }
    UserAssert(pIntDdeInfo != NULL);

    pxsFree = pDdeConv->spxsOut;
    TRACE_DDE3("xxxAdviseDataAck:pxs.hClient(%#p), hServer(%#p), wStatus(%x)",
            pxsFree->hClient, pxsFree->hServer, pIntDdeInfo->DdePack.uiLo);
    if (pIntDdeInfo->DdePack.uiLo & DDE_FACK) {

        /*
         * positive ack implies client accepted the data - free from
         * server at postmessage time iff FRELEASE was set in data msg.
         */
        if (pxsFree->flags & XS_FRELEASE) {
            TRACE_DDE("xxxAdviseDataAck: +ACK delayed server data free");
            FreeListAdd(pDdeConv->spartnerConv, pxsFree->hServer,
                    pxsFree->flags & ~XS_PACKED);
        } else {
            /*
             * Ack w/out fRelease bit means client is done with data.
             */
            TRACE_DDE1("xxxAdviseDataAck: Freeing %#p. (+ACK)",
                    pxsFree->hClient);
            UserAssert(pxsFree->hClient != (HANDLE)*plParam);
            FreeDDEHandle(pDdeConv, pxsFree->hClient, pxsFree->flags & ~XS_PACKED);
        }

    } else {
        TRACE_DDE1("xxxAdviseDataAck: Freeing %#p. (-ACK)",
                pxsFree->hClient);
        FreeDDEHandle(pDdeConv, pxsFree->hClient, pxsFree->flags & ~XS_PACKED);
        UserAssert(pxsFree->hClient != (HANDLE)*plParam);
    }
    PopState(pDdeConv);
    return(DO_POST);
}



/************************************************************************
*                   UNADVISE TRANSACTION PROCESSING                     *
\***********************************************************************/



DWORD Unadvise(          // Spontaneous client transaction = WM_DDE_UNADVISE
PDDECONV pDdeConv)
{
    TRACE_DDE("Unadvise");
    if (AnticipatePost(pDdeConv->spartnerConv, xxxUnadviseAck, NULL, NULL, NULL, 0)) {
        return(DO_POST);
    } else {
        return(FAIL_POST);
    }
}



DWORD xxxUnadviseAck(      // Server response to unadvise - WM_DDE_ACK expected
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    DWORD dwRet;
    PINTDDEINFO pIntDdeInfo;
    CheckLock(pDdeConv);

    if (*pmessage != WM_DDE_ACK) {
        return(xxxUnexpectedServerPost(pmessage, plParam, pDdeConv));
    }
    TRACE_DDE("xxxUnadviseAck");
    dwRet = xxxCopyAckIn(pmessage, plParam, pDdeConv, &pIntDdeInfo);
    if (dwRet != DO_POST) {
        return dwRet;
    }
    UserAssert(pIntDdeInfo != NULL);
    PopState(pDdeConv);
    return(DO_POST);
}



/************************************************************************
*                   REQUEST TRANSACTION PROCESSING                      *
\***********************************************************************/

DWORD Request(       // Spontaneous Client transaction - WM_DDE_REQUEST
PDDECONV pDdeConv)
{
    TRACE_DDE("Request");
    if (AnticipatePost(pDdeConv->spartnerConv, xxxRequestAck, NULL, NULL, NULL, 0)) {
        return(DO_POST);
    } else {
        return(FAIL_POST);
    }
}



DWORD xxxRequestAck(    // Server response - WM_DDE_ACK or WM_DDE_DATA expected
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    PXSTATE pxsFree;
    DWORD flags;
    PINTDDEINFO pIntDdeInfo;
    HANDLE hDirect;
    DWORD dwStatus, dwRet;

    CheckLock(pDdeConv);

    TRACE_DDE("xxxRequestAck or xxxAdviseData");
    switch (*pmessage) {
    case WM_DDE_DATA:

        /*
         * This is very close to advise data handling - the only catch
         * is that if the fRequest bit is clear this IS advise data.
         */
        flags = XS_PACKED | XS_LOHANDLE | XS_DATA;

        dwStatus = ClientGetDDEFlags((HANDLE)*plParam, flags);

        if (!(dwStatus & DDE_FREQUESTED)) {

            /*
             * Its NOT a request Ack - it must be advise data
             */
            return(xxxAdviseData(pmessage, plParam, pDdeConv));
        }

        pxsFree = pDdeConv->spxsOut;
        dwRet = xxxCopyDdeIn((HANDLE)*plParam, &flags, &hDirect, &pIntDdeInfo);
        if (dwRet == DO_POST) {
            UserAssert(pIntDdeInfo != NULL);
            if (!(((PDDE_DATA)(pIntDdeInfo + 1))->wStatus & (DDE_FACK | DDE_FRELEASE))) {
                RIPMSG0(RIP_ERROR, "DDE protocol violation - no RELEASE or ACK bit set - setting RELEASE.");
                ((PDDE_DATA)(pIntDdeInfo + 1))->wStatus |= DDE_FRELEASE;
            }
            if (dwStatus & DDE_FRELEASE) {
                /*
                 * giving it away
                 */
                if (IsObjectPublic(pIntDdeInfo->hIndirect) != NULL) {
                    RIPMSG0(RIP_ERROR, "DDE Protocol violation - giving away a public GDI object.");
                    UserFreePool(pIntDdeInfo);
                    return FAILNOFREE_POST;
                }
                if (GiveObject(((PDDE_DATA)(pIntDdeInfo + 1))->wFmt,
                        pIntDdeInfo->hIndirect,
                        (W32PID)GETPTI(pDdeConv->spwndPartner)->ppi->W32Pid)) {
                    flags |= XS_GIVEBACKONNACK;
                }
                flags |= XS_FRELEASE;
            } else {
                /*
                 * on loan
                 */
                if (AddPublicObject(((PDDE_DATA)(pIntDdeInfo + 1))->wFmt,
                            pIntDdeInfo->hIndirect,
                            (W32PID)GETPTI(pDdeConv->spwnd)->ppi->W32Pid)) {
                    flags |= XS_PUBLICOBJ;
                }
            }
            *pmessage |= MSGFLAG_DDE_MID_THUNK;
            if (dwStatus & DDE_FACK) {
                *plParam = (LPARAM)AnticipatePost(pDdeConv->spartnerConv,
                    xxxAdviseDataAck, NULL, hDirect, pIntDdeInfo, flags);
            } else {
                TRACE_DDE("xxxRequestAck: Delayed freeing non-ackable request data");
                FreeListAdd(pDdeConv, hDirect, flags & ~XS_PACKED);
                pxsFree = Createpxs(NULL, NULL, NULL, pIntDdeInfo, flags | XS_FREEPXS);
                if (pxsFree != NULL) {
                    pxsFree->head.pti = GETPTI(pDdeConv->spwndPartner);
                }
                *plParam = (LPARAM)PtoH(pxsFree);
            }

            if (*plParam != 0) {
                PopState(pDdeConv);
            } else {
                dwRet = FAILNOFREE_POST;
            }
        }
        return dwRet;

    case WM_DDE_ACK:        // server NACKs request
        dwRet = xxxCopyAckIn(pmessage, plParam, pDdeConv, &pIntDdeInfo);
        if (dwRet != DO_POST) {
            return dwRet;
        }
        UserAssert(pIntDdeInfo != NULL);
        PopState(pDdeConv);
        return(DO_POST);

    default:
        return(xxxUnexpectedServerPost(pmessage, plParam, pDdeConv));
    }
}



/************************************************************************
*                     POKE TRANSACTION PROCESSING                       *
\***********************************************************************/



DWORD xxxPoke(          // spontaneous client transaction - WM_DDE_POKE
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    DWORD flags, dwRet;
    PINTDDEINFO pIntDdeInfo;
    HANDLE hDirect;

    CheckLock(pDdeConv);

    TRACE_DDE("xxxPoke");
    flags = XS_PACKED | XS_LOHANDLE | XS_DATA;
    dwRet = xxxCopyDdeIn((HANDLE)*plParam, &flags, &hDirect, &pIntDdeInfo);
    if (dwRet == DO_POST) {
        UserAssert(pIntDdeInfo != NULL);
        if (((PDDE_DATA)(pIntDdeInfo + 1))->wStatus & DDE_FRELEASE) {
            /*
             * giving it away
             */
            if (IsObjectPublic(pIntDdeInfo->hIndirect) != NULL) {
                RIPMSG0(RIP_ERROR, "DDE Protocol violation - giving away a public GDI object.");
                UserFreePool(pIntDdeInfo);
                return FAILNOFREE_POST;
            }
            if (GiveObject(((PDDE_DATA)(pIntDdeInfo + 1))->wFmt,
                    pIntDdeInfo->hIndirect,
                    (W32PID)GETPTI(pDdeConv->spwndPartner)->ppi->W32Pid)) {
                flags |= XS_GIVEBACKONNACK;
            }
            flags |= XS_FRELEASE;
        } else {
            /*
             * on loan
             */
            /*
             * fAck bit is ignored and assumed on.
             */
            if (AddPublicObject(((PDDE_DATA)(pIntDdeInfo + 1))->wFmt,
                        pIntDdeInfo->hIndirect,
                        (W32PID)GETPTI(pDdeConv->spwnd)->ppi->W32Pid)) {
                flags |= XS_PUBLICOBJ;
            }
        }
        *pmessage |= MSGFLAG_DDE_MID_THUNK;
        *plParam = (LPARAM)AnticipatePost(pDdeConv->spartnerConv, xxxPokeAck,
             hDirect, NULL, pIntDdeInfo, flags);
        if (*plParam == 0) {
            dwRet = FAILNOFREE_POST;
        }
    }
    return dwRet;
}


/*
 * If its inter-process:
 *
 * xxxDDETrackGetMessageHook() fills in hServer from pIntDdeInfo when WM_DDE_ADVISE
 * is received.  pIntDdeInfo is then freed.  The hServer handle saved into the
 * pxs structure pointer to by lParam is a directdata structure since
 * packed DDE messages are always assumed to have the packing handle freed
 * by the receiving app.
 * For the !fAckReq case, the pxs is also freed due to the XS_FREEPXS flag.
 */


DWORD xxxPokeAck(       // Server response to poke data - WM_DDE_ACK expected
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    PXSTATE pxsFree;
    PINTDDEINFO pIntDdeInfo;
    DWORD dwRet;

    CheckLock(pDdeConv);

    if (*pmessage != WM_DDE_ACK) {
        return(xxxUnexpectedServerPost(pmessage, plParam, pDdeConv));
    }

    TRACE_DDE("xxxPokeAck");

    dwRet = xxxCopyAckIn(pmessage, plParam, pDdeConv, &pIntDdeInfo);
    if (dwRet != DO_POST) {
        return dwRet;
    }
    UserAssert(pIntDdeInfo != NULL);

    pxsFree = pDdeConv->spxsOut;
    if (pIntDdeInfo->DdePack.uiLo & DDE_FACK) {
        // positive ack implies server accepted the data - free from
        // client at postmessage time iff fRelease was set in poke message.
        if (pxsFree->flags & XS_FRELEASE) {
            TRACE_DDE("xxxPokeAck: delayed freeing client data");
            FreeListAdd(pDdeConv->spartnerConv, pxsFree->hClient,
                    pxsFree->flags & ~XS_PACKED);
        }
    } else {
        // Nack means that sender is responsible for freeing it.
        // We must free it in the receiver's context for him.
        TRACE_DDE("xxxPokeAck: freeing Nacked data");
        UserAssert(pxsFree->hServer != (HANDLE)*plParam);
        FreeDDEHandle(pDdeConv, pxsFree->hServer, pxsFree->flags & ~XS_PACKED);
    }
    PopState(pDdeConv);
    return(DO_POST);
}



/************************************************************************
*                   EXECUTE TRANSACTION PROCESSING                      *
\***********************************************************************/

DWORD xxxExecute(       // spontaneous client transaction - WM_DDE_EXECUTE
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    DWORD flags, dwRet;
    PINTDDEINFO pIntDdeInfo;
    HANDLE hDirect;

    CheckLock(pDdeConv);

    TRACE_DDE("xxxExecute");

    flags = XS_EXECUTE;
    if (!TestWF(pDdeConv->spwnd, WFANSIPROC) &&
            !TestWF(pDdeConv->spwndPartner, WFANSIPROC)) {
        flags |= XS_UNICODE;
    }
    dwRet = xxxCopyDdeIn((HANDLE)*plParam, &flags, &hDirect, &pIntDdeInfo);
    if (dwRet == DO_POST) {
        UserAssert(pIntDdeInfo != NULL);
        *pmessage |= MSGFLAG_DDE_MID_THUNK;
        *plParam = (LPARAM)AnticipatePost(pDdeConv->spartnerConv, xxxExecuteAck,
                hDirect, NULL, pIntDdeInfo, flags);
        /*
         * Check for != 0 to make sure the AnticipatePost() succeeded.
         */
        if (*plParam != 0) {

            /*
             * In the execute case it is likely that the postee will want to activate
             * itself and come on top (OLE 1.0 is an example). In this case, allow
             * both the postee and the poster to foreground activate for the next
             * activate (poster because it will want to activate itself again
             * probably, once the postee is done.)
             */
            GETPTI(pDdeConv->spwnd)->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "xxxExecute set TIF %#p", GETPTI(pDdeConv->spwnd));
            GETPTI(pDdeConv->spwndPartner)->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "xxxExecute set TIF %#p", GETPTI(pDdeConv->spwndPartner));
        } else {
            dwRet = FAILNOFREE_POST;
        }

    }
    return dwRet;
}


/*
 * xxxDDETrackGetMessageHook() fills in hServer from pIntDdeInfo when WM_DDE_EXECUTE
 * is received.  pIntDdeInfo is then freed.
 */


DWORD xxxExecuteAck(       // Server response to execute data - WM_DDE_ACK expected
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    PXSTATE pxsFree;
    PINTDDEINFO pi;
    DWORD flags = XS_PACKED | XS_FREESRC | XS_EXECUTE;
    DWORD dwRet;

    CheckLock(pDdeConv);

    if (*pmessage != WM_DDE_ACK) {
        return(xxxUnexpectedServerPost(pmessage, plParam, pDdeConv));
    }

    TRACE_DDE("xxxExecuteAck");
    dwRet = xxxCopyDdeIn((HANDLE)*plParam, &flags, NULL, &pi);
    if (dwRet == DO_POST) {
        UserAssert(pi != NULL);
        /*
         * the server must respond to the execute with an ack containing the
         * same handle it was given.
         */
        pi->DdePack.uiHi = (ULONG_PTR)pDdeConv->spxsOut->hClient;
        pi->hDirect = NULL;
        pi->cbDirect = 0;
        *pmessage |= MSGFLAG_DDE_MID_THUNK;
        pxsFree = Createpxs(NULL, NULL, NULL, pi, XS_PACKED | XS_FREEPXS);
        if (pxsFree != NULL) {
            pxsFree->head.pti = GETPTI(pDdeConv->spwndPartner);
        }
        *plParam = (LPARAM)PtoH(pxsFree);
        if (*plParam != 0) {
            PopState(pDdeConv);
        } else {
            dwRet = FAILNOFREE_POST;
        }
    }
    return dwRet;
}



/************************************************************************
*                  TERMINATE TRANSACTION PROCESSING                     *
\***********************************************************************/



DWORD SpontaneousTerminate(
PDWORD pmessage,
PDDECONV pDdeConv)
{
    TRACE_DDE("SpontaneousTerminate");
    if (pDdeConv->flags & CXF_TERMINATE_POSTED) {
        return(FAKE_POST);
    } else {
        pDdeConv->flags |= CXF_TERMINATE_POSTED;
        *pmessage |= MSGFLAG_DDE_MID_THUNK;
        return(DO_POST);
    }
}

/*
 * The xxxDDETrackGetMessageHook() function restores the *pmessage value.
 * Unless a spontaneous terminate from the other app has already
 * arrived, it will note that CXF_TERMINATE_POSTED is NOT set on
 * both sides so no action is taken.
 */


/************************************************************************
*               DUPLICATE CONVERSATION TERMINATION                      *
\***********************************************************************/

/*
 * This routine is called when a DDE server window sent a WM_DDE_ACK
 * message to a client window which is already engaged in a conversation
 * with that server window.  We swallow the ACK and post a terminate to
 * the server window to shut this conversation down.  When the server
 * posts the terminate, this function is called to basically fake
 * a sucessful post.  Thus the client is never bothered while the
 * errant server thinks the conversation was connected and then
 * imediately terminated.
 */
DWORD DupConvTerminate(       // WM_DDE_TERMINATE expected
PDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv)
{
    CheckLock(pDdeConv);

    TRACE_DDE("DupConvTerminate");

    if (*pmessage != WM_DDE_TERMINATE) {
        return(xxxUnexpectedServerPost(pmessage, plParam, pDdeConv));
    }

    PopState(pDdeConv);
    return(FAKE_POST);
}



/************************************************************************
*               HELPER ROUTINES FOR TRANSACTION TRACKING                *
\***********************************************************************/



/************************************************************************
* AnticipatePost
*
* Allocates, fills and links XSTATE structures.
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
HANDLE AnticipatePost(
PDDECONV pDdeConv,
FNDDERESPONSE fnResponse,
HANDLE hClient,
HANDLE hServer,
PINTDDEINFO pIntDdeInfo,
DWORD flags)
{
    PXSTATE pxs;

    pxs = Createpxs(fnResponse, hClient, hServer, pIntDdeInfo, flags);
    if (pxs != NULL) {
        pxs->head.pti = pDdeConv->head.pti;
        if (pDdeConv->spxsOut == NULL) {
            UserAssert(pDdeConv->spxsIn == NULL);
            Lock(&(pDdeConv->spxsOut), pxs);
            Lock(&(pDdeConv->spxsIn), pxs);
        } else {
            UserAssert(pDdeConv->spxsIn != NULL);
            Lock(&(pDdeConv->spxsIn->snext), pxs);
            Lock(&(pDdeConv->spxsIn), pxs);
        }
#if 0
        {
            int i;
            HANDLEENTRY *phe;

            for (i = 0, phe = gSharedInfo.aheList;
                i <= (int)giheLast;
                    i++) {
                if (phe[i].bType == TYPE_DDEXACT) {
                    UserAssert(((PXSTATE)(phe[i].phead))->snext != pDdeConv->spxsOut);
                }
                if (phe[i].bType == TYPE_DDECONV &&
                        (PDDECONV)phe[i].phead != pDdeConv) {
                    UserAssert(((PDDECONV)(phe[i].phead))->spxsOut != pDdeConv->spxsOut);
                    UserAssert(((PDDECONV)(phe[i].phead))->spxsIn != pDdeConv->spxsOut);
                }
            }
        }
#endif
    }
    return(PtoH(pxs));
}



/************************************************************************
* Createpxs
*
* Allocates and fills XSTATE structures.
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
PXSTATE Createpxs(
FNDDERESPONSE fnResponse,
HANDLE hClient,
HANDLE hServer,
PINTDDEINFO pIntDdeInfo,
DWORD flags)
{
    PXSTATE pxs;

    pxs = HMAllocObject(PtiCurrent(), NULL, TYPE_DDEXACT, sizeof(XSTATE));
    if (pxs == NULL) {
#if DBG
        RIPMSG0(RIP_WARNING, "Unable to alloc DDEXACT");
#endif
        return(NULL);
    }
    pxs->snext = NULL;
    pxs->fnResponse = fnResponse;
    pxs->hClient = hClient;
    pxs->hServer = hServer;
    pxs->pIntDdeInfo = pIntDdeInfo;
    pxs->flags = flags;
    ValidatePublicObjectList();
    UserAssert(pxs->head.cLockObj == 0);
    return(pxs);
}




/************************************************************************
* AbnormalDDEPost
*
* This is the catch-all routine for wierd cases
*
* returns post action code - DO_POST, FAKE_POST, FAIL_POST.
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
DWORD AbnormalDDEPost(
PDDECONV pDdeConv,
DWORD message)
{

#if DBG
    if (message != WM_DDE_TERMINATE) {
        RIPMSG2(RIP_WARNING,
                "DDE Post failed (%#p:%#p) - protocol violation.",
                PtoH(pDdeConv->spwnd), PtoH(pDdeConv->spwndPartner));
    }
#endif // DBG

    // shutdown this conversation by posting a terminate on
    // behalf of this guy, then fail all future posts but
    // fake a successful terminate.

    if (!(pDdeConv->flags & CXF_TERMINATE_POSTED)) {
        _PostMessage(pDdeConv->spwndPartner, WM_DDE_TERMINATE,
                (WPARAM)PtoH(pDdeConv->spwnd), 0);
        // pDdeConv->flags |= CXF_TERMINATE_POSTED; Set by post hook proc
    }
    return(message == WM_DDE_TERMINATE ? FAKE_POST : FAIL_POST);
}



/************************************************************************
* NewConversation
*
* Worker function used to create a saimese pair of DDECONV structures.
*
* Returns fCreateOk
*
* History:
* 11-5-92    sanfords    Created
\***********************************************************************/
BOOL NewConversation(
PDDECONV *ppdcNewClient,
PDDECONV *ppdcNewServer,
PWND pwndClient,
PWND pwndServer)
{
    PDDECONV pdcNewClient;
    PDDECONV pdcNewServer;

    pdcNewClient = HMAllocObject(GETPTI(pwndClient), NULL,
            TYPE_DDECONV, sizeof(DDECONV));
    if (pdcNewClient == NULL) {
        return(FALSE);
    }

    pdcNewServer = HMAllocObject(GETPTI(pwndServer), NULL,
            TYPE_DDECONV, sizeof(DDECONV));
    if (pdcNewServer == NULL) {
        HMFreeObject(pdcNewClient);     // we know it's not locked.
        return(FALSE);
    }

    AddConvProp(pwndClient, pwndServer, 0, pdcNewClient, pdcNewServer);
    AddConvProp(pwndServer, pwndClient, CXF_IS_SERVER, pdcNewServer,
            pdcNewClient);

    if (ppdcNewClient != NULL) {
        *ppdcNewClient = pdcNewClient;
    }
    if (ppdcNewServer != NULL) {
        *ppdcNewServer = pdcNewServer;
    }
    return(TRUE);
}


/************************************************************************
* FindDdeConv
*
* Locates the pDdeConv associated with pwndProp, and pwndPartner.
* Only searches pwndProp's property list.
*
* History:
* 3-31-91   sanfords    Created
\***********************************************************************/
PDDECONV FindDdeConv(
PWND pwndProp,
PWND pwndPartner)
{
    PDDECONV pDdeConv;

    pDdeConv = (PDDECONV)_GetProp(pwndProp, PROP_DDETRACK, PROPF_INTERNAL);
    while (pDdeConv != NULL && pDdeConv->spwndPartner != pwndPartner) {
        pDdeConv = pDdeConv->snext;
    }

    return(pDdeConv);
}



/************************************************************************
* xxxCopyAckIn
*
* A common occurance helper function
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
DWORD xxxCopyAckIn(
LPDWORD pmessage,
LPARAM *plParam,
PDDECONV pDdeConv,
PINTDDEINFO * ppIntDdeInfo)
{
    PINTDDEINFO pIntDdeInfo;
    DWORD flags, dwRet;
    PXSTATE pxs;

    CheckLock(pDdeConv);

    flags = XS_PACKED | XS_FREESRC;
    dwRet = xxxCopyDdeIn((HANDLE)*plParam, &flags, NULL, ppIntDdeInfo);
    if (dwRet == DO_POST) {
        UserAssert(*ppIntDdeInfo != NULL);
        pIntDdeInfo = *ppIntDdeInfo;
        if (pDdeConv->spxsOut->flags & XS_GIVEBACKONNACK &&
                !(((PDDE_DATA)(pIntDdeInfo + 1))->wStatus & DDE_FACK)) {
            GiveObject(((PDDE_DATA)(pDdeConv->spxsOut->pIntDdeInfo + 1))->wFmt,
                    pDdeConv->spxsOut->pIntDdeInfo->hIndirect,
                    (W32PID)GETPTI(pDdeConv->spwndPartner)->ppi->W32Pid);
        }
        if (pDdeConv->spxsOut->flags & XS_PUBLICOBJ) {
            RemovePublicObject(((PDDE_DATA)(pDdeConv->spxsOut->pIntDdeInfo + 1))->wFmt,
                    pDdeConv->spxsOut->pIntDdeInfo->hIndirect);
            pDdeConv->spxsOut->flags &= ~XS_PUBLICOBJ;
        }
        pxs = Createpxs(NULL, NULL, NULL, pIntDdeInfo, flags | XS_FREEPXS);
        if (pxs != NULL) {
            pxs->head.pti = GETPTI(pDdeConv->spwndPartner);
        }
        *plParam = (LPARAM)PtoH(pxs);
        if (*plParam == 0) {
            return FAILNOFREE_POST;
        }
        *pmessage |= MSGFLAG_DDE_MID_THUNK;
    }
    return dwRet;
}



/************************************************************************
* FreeListAdd
*
* Adds a CSR Client handle to the free list associated with pDdeConv.
* This allows us to make sure stuff is freed that isn't in a context
* we have access at the time we know it must be freed.
*
* returns fSuccess
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
BOOL FreeListAdd(
PDDECONV pDdeConv,
HANDLE hClient,
DWORD flags)
{
    PFREELIST pfl;

    pfl = (PFREELIST)UserAllocPool(sizeof(FREELIST), TAG_DDE1);
    if (!pfl) {
        return(FALSE);
    }
    TRACE_DDE2("FreeListAdd: %x for thread %x.", hClient,
            pDdeConv->head.pti->pEThread->Cid.UniqueThread);
    pfl->h = hClient;
    pfl->flags = flags;
    pfl->next = pDdeConv->pfl;
    pDdeConv->pfl = pfl;
    return(TRUE);
}


/************************************************************************
* FreeDDEHandle
*
* Frees contents DDE client side handle - delayed free if a WOW process.
*
* History:
* 7-28-94    sanfords    Created
\***********************************************************************/
VOID FreeDDEHandle(
PDDECONV pDdeConv,
HANDLE hClient,
DWORD flags)
{
    if (PtiCurrent()->TIF_flags & TIF_16BIT) {
        TRACE_DDE1("FreeDDEHandle: (WOW hack) delayed Freeing %#p.", hClient);
        FreeListAdd(pDdeConv, hClient, flags);
    } else {
        TRACE_DDE1("FreeDDEHandle: Freeing %#p.", hClient);
        ClientFreeDDEHandle(hClient, flags);
    }
}



/************************************************************************
* xxxFreeListFree
*
* Frees contents of the free list associated with pDdeConv.
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
VOID FreeListFree(
    PFREELIST pfl)
{
    PFREELIST pflPrev;

    CheckCritIn();

    UserAssert(pfl != NULL);

    while (pfl != NULL) {
        pflPrev = pfl;
        pfl = pfl->next;
        UserFreePool(pflPrev);
    }
}


VOID xxxFreeListFree(
PFREELIST pfl)
{
    PFREELIST pflPrev;
    BOOL      fInCleanup;
    TL        tlPool;

    CheckCritIn();

    if (pfl == NULL) {
        return;
    }

    fInCleanup = (PtiCurrent())->TIF_flags & TIF_INCLEANUP;

    while (pfl != NULL) {

        ThreadLockPoolCleanup(PtiCurrent(), pfl, &tlPool, FreeListFree);

        if (!fInCleanup) {
            TRACE_DDE1("Freeing %#p from free list.\n", pfl->h);
            ClientFreeDDEHandle(pfl->h, pfl->flags);
        }

        ThreadUnlockPoolCleanup(PtiCurrent(), &tlPool);

        pflPrev = pfl;
        pfl = pfl->next;
        UserFreePool(pflPrev);
    }
}


/************************************************************************
* PopState
*
* Frees spxsOut from pDdeConv and handles empty queue case.
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/
VOID PopState(
PDDECONV pDdeConv)
{
    PXSTATE pxsNext, pxsFree;
    TL tlpxs;

    UserAssert(pDdeConv->spxsOut != NULL);
#if 0
    {
        int i;
        HANDLEENTRY *phe;

        for (i = 0, phe = gSharedInfo.aheList;
            i <= giheLast;
                i++) {
            if (phe[i].bType == TYPE_DDEXACT) {
                UserAssert(((PXSTATE)(phe[i].phead))->snext != pDdeConv->spxsOut);
            }
        }
    }
#endif
    UserAssert(!(pDdeConv->spxsOut->flags & XS_FREEPXS));
    UserAssert(pDdeConv->spxsIn != NULL);
    UserAssert(pDdeConv->spxsIn->snext == NULL);

    ThreadLockAlways(pDdeConv->spxsOut, &tlpxs);              // hold it fast
    pxsNext = pDdeConv->spxsOut->snext;
    pxsFree = Lock(&(pDdeConv->spxsOut), pxsNext);      // lock next into head
    if (pxsNext == NULL) {
        UserAssert(pDdeConv->spxsIn == pxsFree);
        Unlock(&(pDdeConv->spxsIn));                // queue is empty.
    } else {
        Unlock(&(pxsFree->snext));                  // clear next ptr
    }
    pxsFree = ThreadUnlock(&tlpxs);                     // undo our lock
    if (pxsFree != NULL) {
        FreeDdeXact(pxsFree);                           // cleanup.
    }
}


VOID FreeDdeConv(
PDDECONV pDdeConv)
{

    TRACE_DDE1("FreeDdeConv(%#p)", pDdeConv);

    if (!(pDdeConv->flags & CXF_TERMINATE_POSTED) &&
            !HMIsMarkDestroy(pDdeConv->spwndPartner)) {
        _PostMessage(pDdeConv->spwndPartner, WM_DDE_TERMINATE,
                (WPARAM)PtoH(pDdeConv->spwnd), 0);
        // pDdeConv->flags |= CXF_TERMINATE_POSTED; set by PostHookProc
    }

    if (pDdeConv->spartnerConv != NULL &&
            GETPTI(pDdeConv)->TIF_flags & TIF_INCLEANUP) {
        /*
         * Fake that the other side already posted a terminate.
         * This prevents vestigal dde structures from hanging
         * around after thread cleanup if the conversation structure
         * is destroyed before the associated window.
         */
        pDdeConv->spartnerConv->flags |= CXF_TERMINATE_POSTED;
    }

    UnlinkConv(pDdeConv);

    if (pDdeConv->pddei != NULL) {
        pDdeConv->pddei->cRefConv--;
        if (pDdeConv->pddei->cRefConv == 0 && pDdeConv->pddei->cRefInit == 0) {
            SeDeleteClientSecurity(&pDdeConv->pddei->ClientContext);
            UserFreePool(pDdeConv->pddei);
        }
        pDdeConv->pddei = NULL;
    }

    Unlock(&(pDdeConv->spartnerConv));
    Unlock(&(pDdeConv->spwndPartner));
    Unlock(&(pDdeConv->spwnd));

    if (!HMMarkObjectDestroy((PHEAD)pDdeConv))
        return;

    while (pDdeConv->spxsOut) {
        PopState(pDdeConv);
    }

    HMFreeObject(pDdeConv);
}



/***************************************************************************\
* xxxCopyDdeIn
*
* Description:
*   Copies DDE data from the CSR client to the CSR server side.
*   Crosses the CSR barrier as many times as is needed to get all the data
*   through the CSR window.
*
* History:
* 11-1-91   sanfords    Created.
\***************************************************************************/
DWORD xxxCopyDdeIn(
HANDLE hSrc,
PDWORD pflags,
PHANDLE phDirect,
PINTDDEINFO *ppi)
{
    DWORD dwRet;
    PINTDDEINFO pi;

    dwRet = xxxClientCopyDDEIn1(hSrc, *pflags, ppi);
    pi = *ppi;
    TRACE_DDE2(*pflags & XS_FREESRC ?
            "Copying in and freeing %#p(%#p)" :
            "Copying in %#p(%#p)",
            hSrc, pi ? pi->hDirect : 0);

    if (dwRet == DO_POST) {
        UserAssert(*ppi != NULL);
        *pflags = pi->flags;
        TRACE_DDE3("xxxCopyDdeIn: uiLo=%x, uiHi=%x, hDirect=%#p",
                pi->DdePack.uiLo, pi->DdePack.uiHi, pi->hDirect);
        if (phDirect != NULL) {
            *phDirect = pi->hDirect;
        }
    }
#if DBG
      else {
        RIPMSG0(RIP_WARNING, "Unable to alloc DDE INTDDEINFO");
    }
#endif

    return(dwRet);
}



/***********************************************************************\
* xxxCopyDDEOut
*
* Returns: the apropriate client side handle for lParam or NULL on
* failure. (Since only TERMINATES should have 0 here)
*
* 11/7/1995 Created SanfordS
\***********************************************************************/

HANDLE xxxCopyDDEOut(
PINTDDEINFO pi,
PHANDLE phDirect)   // receives the target client side GMEM handle.
{
    HANDLE hDst;

    TRACE_DDE3("xxxCopyDDEOut: cbDirect=%x, cbIndirect=%x, flags=%x",
            pi->cbDirect, pi->cbIndirect, pi->flags);
    hDst = xxxClientCopyDDEOut1(pi);
    TRACE_DDE3("xxxCopyDDEOut: uiLo=%x, uiHi=%x, hResult=%#p",
            pi->DdePack.uiLo, pi->DdePack.uiHi, hDst);
    if (hDst != NULL) {
        if (phDirect != NULL) {
            TRACE_DDE1("xxxCopyDDEOut: *phDirect=%#p", pi->hDirect);
            *phDirect = pi->hDirect;
        }
    }
    return(hDst);
}



/*
 * This API is used to set the QOS associated with a potential DDE client window.
 * It should be called prior to sending a WM_DDE_INITIATE message and the qos set
 * will hold until the WM_DDE_INITIATE send or broadcast returns.
 */
BOOL _DdeSetQualityOfService(
PWND pwndClient,
CONST PSECURITY_QUALITY_OF_SERVICE pqosNew,
PSECURITY_QUALITY_OF_SERVICE pqosOld)
{
    PSECURITY_QUALITY_OF_SERVICE pqosUser;
    PSECURITY_QUALITY_OF_SERVICE pqosAlloc = NULL;
    BOOL fRet;

    /*
     * ASSUME: calling process is owner of pwndClient - ensured in thunk.
     */
    pqosUser = (PSECURITY_QUALITY_OF_SERVICE)InternalRemoveProp(pwndClient,
            PROP_QOS, PROPF_INTERNAL);
    if (pqosUser == NULL) {
        if (RtlEqualMemory(pqosNew, &gqosDefault, sizeof(SECURITY_QUALITY_OF_SERVICE))) {
            return(TRUE);           // no PROP_QOS property implies default QOS
        }
        pqosAlloc = (PSECURITY_QUALITY_OF_SERVICE)UserAllocPoolZInit(
                sizeof(SECURITY_QUALITY_OF_SERVICE), TAG_DDE2);
        if (pqosAlloc == NULL) {
            return(FALSE);          // memory allocation failure - can't change from default
        }
        pqosUser = pqosAlloc;
    }
    *pqosOld = *pqosUser;
    *pqosUser = *pqosNew;

    fRet = InternalSetProp(pwndClient, PROP_QOS, pqosUser, PROPF_INTERNAL);
    if ((fRet == FALSE) && (pqosAlloc != NULL)) {
        UserFreePool(pqosAlloc);
    }

    return fRet;
}


/*
 * This is a private API for NetDDE's use.  It extracts the QOS associated with an
 * active DDE conversation.  Intra-process conversations always are set to the default
 * QOS.
 */
BOOL _DdeGetQualityOfService(
PWND pwndClient,
PWND pwndServer,
PSECURITY_QUALITY_OF_SERVICE pqos)
{
    PDDECONV pDdeConv;
    PSECURITY_QUALITY_OF_SERVICE pqosClient;

    if (pwndServer == NULL) {
        /*
         * Special case to support DDEML-RAW conversations that need to get
         * the QOS prior to initiation completion.
         */
        pqosClient = _GetProp(pwndClient, PROP_QOS, PROPF_INTERNAL);
        if (pqosClient == NULL) {
            *pqos = gqosDefault;
        } else {
            *pqos = *pqosClient;
        }
        return(TRUE);
    }
    if (GETPWNDPPI(pwndClient) == GETPWNDPPI(pwndServer)) {
        *pqos = gqosDefault;
        return(TRUE);
    }
    pDdeConv = FindDdeConv(pwndClient, pwndServer);
    if (pDdeConv == NULL) {
        return(FALSE);
    }
    if (pDdeConv->pddei == NULL) {
        return(FALSE);
    }
    *pqos = pDdeConv->pddei->qos;
    return(TRUE);
}



BOOL _ImpersonateDdeClientWindow(
    PWND pwndClient,
    PWND pwndServer)
{
    PDDECONV pDdeConv;
    NTSTATUS Status;

    /*
     * Locate token used in the conversation
     */
    pDdeConv = FindDdeConv(pwndClient, pwndServer);
    if (pDdeConv == NULL || pDdeConv->pddei == NULL)
        return(FALSE);

    /*
     * Stick the token into the dde server thread
     */
    Status = SeImpersonateClientEx(&pDdeConv->pddei->ClientContext,
            PsGetCurrentThread());
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");
        return FALSE;
    }
    return TRUE;
}




VOID FreeDdeXact(
PXSTATE pxs)
{
    if (!HMMarkObjectDestroy(pxs))
        return;

#if 0
    {
        int i;
        HANDLEENTRY *phe;

        for (i = 0, phe = gSharedInfo.aheList;
            i <= giheLast;
                i++) {
            if (phe[i].bType == TYPE_DDEXACT) {
                UserAssert(((PXSTATE)(phe[i].phead))->snext != pxs);
            }
            if (phe[i].bType == TYPE_DDECONV) {
                UserAssert(((PDDECONV)(phe[i].phead))->spxsOut != pxs);
                UserAssert(((PDDECONV)(phe[i].phead))->spxsIn != pxs);
            }
        }
    }
    UserAssert(pxs->head.cLockObj == 0);
    UserAssert(pxs->snext == NULL);
#endif

    if (pxs->pIntDdeInfo != NULL) {
        /*
         * free any server-side GDI objects
         */
        if (pxs->pIntDdeInfo->flags & (XS_METAFILEPICT | XS_ENHMETAFILE)) {
            GreDeleteServerMetaFile(pxs->pIntDdeInfo->hIndirect);
        }
        if (pxs->flags & XS_PUBLICOBJ) {
            RemovePublicObject(((PDDE_DATA)(pxs->pIntDdeInfo + 1))->wFmt,
                    pxs->pIntDdeInfo->hIndirect);
            pxs->flags &= ~XS_PUBLICOBJ;
        }
        UserFreePool(pxs->pIntDdeInfo);
    }

    HMFreeObject(pxs);
    ValidatePublicObjectList();
}



PPUBOBJ IsObjectPublic(
HANDLE hObj)
{
    PPUBOBJ ppo;

    for (ppo = gpPublicObjectList; ppo != NULL; ppo = ppo->next) {
        if (ppo->hObj == hObj) {
            break;
        }
    }
    return(ppo);
}



BOOL AddPublicObject(
UINT format,
HANDLE hObj,
W32PID pid)
{
    PPUBOBJ ppo;

    switch (format) {
    case CF_BITMAP:
    case CF_DSPBITMAP:
    case CF_PALETTE:
        break;

    default:
        return(FALSE);
    }

    ppo = IsObjectPublic(hObj);
    if (ppo == NULL) {
        ppo = UserAllocPool(sizeof(PUBOBJ), TAG_DDE4);
        if (ppo == NULL) {
            return(FALSE);
        }
        ppo->count = 1;
        ppo->hObj = hObj;
        ppo->pid = pid;
        ppo->next = gpPublicObjectList;
        gpPublicObjectList = ppo;
        GiveObject(format, hObj, OBJECT_OWNER_PUBLIC);
    } else {
        ppo->count++;
    }
    return(TRUE);
}



BOOL RemovePublicObject(
UINT format,
HANDLE hObj)
{
    PPUBOBJ ppo, ppoPrev;

    switch (format) {
    case CF_BITMAP:
    case CF_DSPBITMAP:
    case CF_PALETTE:
        break;

    default:
        return(FALSE);
    }

    for (ppoPrev = NULL, ppo = gpPublicObjectList;
            ppo != NULL;
                ppoPrev = ppo, ppo = ppo->next) {
        if (ppo->hObj == hObj) {
            break;
        }
    }
    if (ppo == NULL) {
        UserAssert(FALSE);
        return(FALSE);
    }
    ppo->count--;
    if (ppo->count == 0) {
        GiveObject(format, hObj, ppo->pid);
        if (ppoPrev != NULL) {
            ppoPrev->next = ppo->next;
        } else {
            gpPublicObjectList = ppo->next;
        }
        UserFreePool(ppo);
    }
    return(TRUE);
}


BOOL
GiveObject(
    UINT format,
    HANDLE hObj,
    W32PID pid)
{
    switch (format) {
    case CF_BITMAP:
    case CF_DSPBITMAP:
        GreSetBitmapOwner(hObj, pid);
        return(TRUE);

    case CF_PALETTE:
        GreSetPaletteOwner(hObj, pid);
        return(TRUE);

    default:
        return(FALSE);
    }
}

#if DBG
VOID ValidatePublicObjectList()
{
    PPUBOBJ ppo;
    int i, count;
    HANDLEENTRY *phe;

    for (count = 0, ppo = gpPublicObjectList;
            ppo != NULL;
                ppo = ppo->next) {
        count += ppo->count;
    }
    for (i = 0, phe = gSharedInfo.aheList;
        i <= (int)giheLast;
            i++) {
        if (phe[i].bType == TYPE_DDEXACT) {
            if (((PXSTATE)(phe[i].phead))->flags & XS_PUBLICOBJ) {
                UserAssert(((PXSTATE)(phe[i].phead))->pIntDdeInfo != NULL);
                UserAssert(IsObjectPublic(((PXSTATE)
                        (phe[i].phead))->pIntDdeInfo->hIndirect) != NULL);
                count--;
            }
        }
    }
    UserAssert(count == 0);
}


VOID TraceDdeMsg(
UINT msg,
HWND hwndFrom,
HWND hwndTo,
UINT code)
{
    LPSTR szMsg, szType;

    msg = msg & 0xFFFF;

    switch (msg) {
    case WM_DDE_INITIATE:
        szMsg = "INITIATE";
        break;

    case WM_DDE_TERMINATE:
        szMsg = "TERMINATE";
        break;

    case WM_DDE_ADVISE:
        szMsg = "ADVISE";
        break;

    case WM_DDE_UNADVISE:
        szMsg = "UNADVISE";
        break;

    case WM_DDE_ACK:
        szMsg = "ACK";
        break;

    case WM_DDE_DATA:
        szMsg = "DATA";
        break;

    case WM_DDE_REQUEST:
        szMsg = "REQUEST";
        break;

    case WM_DDE_POKE:
        szMsg = "POKE";
        break;

    case WM_DDE_EXECUTE:
        szMsg = "EXECUTE";
        break;

    default:
        szMsg = "BOGUS";
        UserAssert(msg >= WM_DDE_FIRST && msg <= WM_DDE_LAST);
        break;
    }

    switch (code) {
    case MSG_SENT:
        szType = "[sent]";
        break;

    case MSG_POST:
        szType = "[posted]";
        break;

    case MSG_RECV:
        szType = "[received]";
        break;

    case MSG_PEEK:
        szType = "[peeked]";
        break;

    default:
        szType = "[bogus]";
        UserAssert(FALSE);
        break;
    }

    RIPMSG4(RIP_VERBOSE,
            "%#p->%#p WM_DDE_%s %s",
            hwndFrom, hwndTo, szMsg, szType);
}
#endif //DBG
