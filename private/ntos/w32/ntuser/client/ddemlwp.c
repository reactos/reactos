/****************************** Module Header ******************************\
* Module Name: ddemlwp.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* DDE Manager client side window procedures
*
* Created: 11/3/91 Sanford Staab
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

VOID ProcessDDEMLInitiate(PCL_INSTANCE_INFO pcii, HWND hwndClient,
        GATOM aServer, GATOM aTopic);

/***************************************************************************\
* DDEMLMotherWndProc
*
* Description:
* Handles WM_DDE_INITIATE messages for DDEML and holds all the other windows
* for a DDEML instance.
*
* History:
* 12-29-92 sanfords Created.
\***************************************************************************/
LRESULT DDEMLMotherWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message) {
    case UM_REGISTER:
    case UM_UNREGISTER:
        return(ProcessRegistrationMessage(hwnd, message, wParam, lParam));

    case WM_DDE_INITIATE:
        ProcessDDEMLInitiate((PCL_INSTANCE_INFO)GetWindowLongPtr(hwnd, GWLP_PCI),
                (HWND)wParam, (ATOM)LOWORD(lParam), (ATOM)HIWORD(lParam));
        return(0);

    }
    return(DefWindowProc(hwnd, message, wParam, lParam));
}



/***************************************************************************\
* ProcessDDEMLInitiate
*
* Description:
*
*   WM_DDE_INITIATE messages are processed here.
*
* History:
* 12-29-92   sanfords    Created.
\***************************************************************************/
VOID ProcessDDEMLInitiate(
PCL_INSTANCE_INFO pcii,
HWND hwndClient,
GATOM aServer,
GATOM aTopic)
{
    CONVCONTEXT cc = {
        sizeof(CONVCONTEXT),
        0,
        0,
        CP_WINANSI,
        0L,
        0L,
        {
            sizeof(SECURITY_QUALITY_OF_SERVICE),
            SecurityImpersonation,
            SECURITY_STATIC_TRACKING,
            TRUE
        }
    };
    BOOL flags = ST_INLIST;
    BOOL fWild;
    HDDEDATA hData;
    HWND hwndServer;
    PSERVER_LOOKUP psl;
    PHSZPAIR php;
    HSZPAIR hp[2];
    LATOM laService, laFree1 = 0;
    LATOM laTopic, laFree2 = 0;
    PSVR_CONV_INFO psi;
    LATOM *plaNameService;
    PWND pwndClient;
    PCLS pcls;

    if (pcii == NULL) {
        return;     // we aren't done being initiated yet.
    }

    EnterDDECrit;

    if (pcii->afCmd & CBF_FAIL_CONNECTIONS || !IsWindow(hwndClient)) {
        goto Exit;
    }

    pwndClient = ValidateHwnd(hwndClient);
    if (pwndClient == NULL) goto Exit;

    pcls = (PCLS)REBASEALWAYS(pwndClient, pcls);
    if (!TestWF(pwndClient, WFANSIPROC)) {
        if (pcls->atomClassName == gpsi->atomSysClass[ICLS_DDEMLCLIENTW]) {
            flags |= ST_ISLOCAL;
        }
    } else {
        if (pcls->atomClassName == gpsi->atomSysClass[ICLS_DDEMLCLIENTA]) {
            flags |= ST_ISLOCAL;
        }
    }

    if (flags & ST_ISLOCAL) {
        /*
         * Make sure other guy allows self-connections if that's what this is.
         */
        if (pcii->hInstServer == (HANDLE)GetWindowLongPtr(hwndClient, GWLP_SHINST)) {
            if (pcii->afCmd & CBF_FAIL_SELFCONNECTIONS) {
                goto Exit;
            }
            flags |= ST_ISSELF;
        }

        GetConvContext(hwndClient, (LONG *)&cc);
        if (GetWindowLong(hwndClient, GWL_CONVSTATE) & CLST_SINGLE_INITIALIZING) {
            flags &= ~ST_INLIST;
        }
    } else {
        NtUserDdeGetQualityOfService(hwndClient, NULL, &cc.qos);
    }

/***************************************************************************\
*
* Server window creation is minimized by only creating one window per
* Instance/Service/Topic set. This should be all that is needed and
* duplicate connections (ie where the server/client window pair is identical
* to another conversation) should not happen. However, if some dumb
* server app attempts to create a duplicate conversation by having
* duplicate service/topic pairs passed back from a XTYP_WILD_CONNECT
* callback we will not honor the request.
*
* The INSTANCE_INFO structure holds a pointer to an array of SERVERLOOKUP
* structures each entry of which references the hwndServer that supports
* all conversations on that service/topic pair. The hwndServer windows
* in turn have window words that reference the first member in a linked
* list of SVR_CONV_INFO structures, one for each conversation on that
* service/topic pair.
*
\***************************************************************************/

    laFree1 = laService = GlobalToLocalAtom(aServer);
    laFree2 = laTopic = GlobalToLocalAtom(aTopic);

    plaNameService = pcii->plaNameService;
    if (!laService && pcii->afCmd & APPCMD_FILTERINITS && *plaNameService == 0) {
        /*
         * no WILDCONNECTS to servers with no registered names while filtering.
         */
        goto Exit;
    }
    if ((pcii->afCmd & APPCMD_FILTERINITS) && laService) {
        /*
         * if we can't find the aServer in this instance's service name
         * list, don't bother the server.
         */
        while (*plaNameService != 0 && *plaNameService != laService) {
            plaNameService++;
        }
        if (*plaNameService == 0) {
            goto Exit;
        }
    }
    hp[0].hszSvc = NORMAL_HSZ_FROM_LATOM(laService);
    hp[0].hszTopic = NORMAL_HSZ_FROM_LATOM(laTopic);
    hp[1].hszSvc = 0;
    hp[1].hszTopic = 0;
    fWild = !laService || !laTopic;

    hData = DoCallback(pcii,
        (WORD)(fWild ? XTYP_WILDCONNECT : XTYP_CONNECT),
        0,
        (HCONV)0,
        hp[0].hszTopic,
        hp[0].hszSvc,
        (HDDEDATA)0,
        flags & ST_ISLOCAL ? (ULONG_PTR)&cc : 0,
        (DWORD)(flags & ST_ISSELF) ? 1 : 0);

    if (!hData) {
        goto Exit;
    }

    if (fWild) {
        php = (PHSZPAIR)DdeAccessData(hData, NULL);
        if (php == NULL) {
            goto Exit;
        }
    } else {
        php = hp;
    }

    while (php->hszSvc && php->hszTopic) {

        psi = (PSVR_CONV_INFO)DDEMLAlloc(sizeof(SVR_CONV_INFO));
        if (psi == NULL) {
            break;
        }

        laService = LATOM_FROM_HSZ(php->hszSvc);
        laTopic = LATOM_FROM_HSZ(php->hszTopic);

        hwndServer = 0;
        if (pcii->cServerLookupAlloc) {
            int i;
            /*
             * See if there already exists a server window for this
             * aServer/aTopic pair
             */
            for (i = pcii->cServerLookupAlloc; i; i--) {
                if (pcii->aServerLookup[i - 1].laService == laService &&
                        pcii->aServerLookup[i - 1].laTopic == laTopic) {
                    PSVR_CONV_INFO psiT;
                    PCONV_INFO pcoi;

                    hwndServer = pcii->aServerLookup[i - 1].hwndServer;
                    /*
                     * Now make sure this window isn't some bogus idiot
                     * trying to create a second conversation from the
                     * same client window that is already talking to
                     * our existing server window.
                     */
                    psiT = (PSVR_CONV_INFO)GetWindowLongPtr(hwndServer, GWLP_PSI);
                    for (pcoi = &psiT->ci; pcoi != NULL; pcoi = pcoi->next) {
                        if (pcoi->hwndPartner == hwndClient) {
                            hwndServer = NULL;
                            break;
                        }
                    }
                    break;
                }
            }
        }

        if (hwndServer == 0) {

            // no server window exists - make one.

            LeaveDDECrit;
            if (pcii->flags & IIF_UNICODE) {
                hwndServer = CreateWindowW((LPWSTR)(gpsi->atomSysClass[ICLS_DDEMLSERVERW]),
                                          L"",
                                          WS_CHILD,
                                          0, 0, 0, 0,
                                          pcii->hwndMother,
                                          (HMENU)0,
                                          0,
                                          (LPVOID)NULL);
            } else {
                hwndServer = CreateWindowA((LPSTR)(gpsi->atomSysClass[ICLS_DDEMLSERVERA]),
                                          "",
                                          WS_CHILD,
                                          0, 0, 0, 0,
                                          pcii->hwndMother,
                                          (HMENU)0,
                                          0,
                                          (LPVOID)NULL);
            }
            EnterDDECrit;

            if (hwndServer == 0) {
                DDEMLFree(psi);
                break;
            }
            // SetWindowLongPtr(hwndServer, GWLP_PSI, (LONG)NULL); // Zero init.

            // put the window into the lookup list

            if (pcii->aServerLookup == NULL) {
                psl = (PSERVER_LOOKUP)DDEMLAlloc(sizeof(SERVER_LOOKUP));
            } else {
                psl = (PSERVER_LOOKUP)DDEMLReAlloc(pcii->aServerLookup,
                        sizeof(SERVER_LOOKUP) * (pcii->cServerLookupAlloc + 1));
            }
            if (psl == NULL) {
                RIPMSG1(RIP_WARNING, "ProcessDDEMLInitiate:hwndServer (%x) destroyed due to low memory.", hwndServer);
                NtUserDestroyWindow(hwndServer);
                DDEMLFree(psi);
                break;
            }

            IncLocalAtomCount(laService); // for SERVER_LOOKUP
            psl[pcii->cServerLookupAlloc].laService = laService;
            IncLocalAtomCount(laTopic); // for SERVER_LOOKUP
            psl[pcii->cServerLookupAlloc].laTopic = laTopic;
            psl[pcii->cServerLookupAlloc].hwndServer = hwndServer;
            pcii->aServerLookup = psl;
            pcii->cServerLookupAlloc++;
            // DumpServerLookupTable("After addition:", hwndServer, psl, pcii->cServerLookupAlloc);
        }

        psi->ci.next = (PCONV_INFO)GetWindowLongPtr(hwndServer, GWLP_PSI);
        SetWindowLongPtr(hwndServer, GWLP_PSI, (LONG_PTR)psi);
        psi->ci.pcii = pcii;
        // psi->ci.hUser = 0;
        psi->ci.hConv = (HCONV)CreateHandle((ULONG_PTR)psi,
                HTYPE_SERVER_CONVERSATION, InstFromHandle(pcii->hInstClient));
        psi->ci.laService = laService;
        IncLocalAtomCount(laService); // for server window
        psi->ci.laTopic = laTopic;
        IncLocalAtomCount(laTopic); // for server window
        psi->ci.hwndPartner = hwndClient;
        psi->ci.hwndConv = hwndServer;
        psi->ci.state = (WORD)(flags | ST_CONNECTED | pcii->ConvStartupState);
        SetCommonStateFlags(hwndClient, hwndServer, &psi->ci.state);
        psi->ci.laServiceRequested = laFree1;
        IncLocalAtomCount(psi->ci.laServiceRequested); // for server window
        // psi->ci.pxiIn = NULL;
        // psi->ci.pxiOut = NULL;
        // psi->ci.dmqIn = NULL;
        // psi->ci.dmqOut = NULL;
        // psi->ci.aLinks = NULL;
        // psi->ci.cLinks = 0;
        // psi->ci.cLocks = 0;

        LeaveDDECrit;
        CheckDDECritOut;
        SendMessage(hwndClient, WM_DDE_ACK, (WPARAM)hwndServer,
                MAKELONG(LocalToGlobalAtom(laService), LocalToGlobalAtom(laTopic)));
        EnterDDECrit;

        if (!(pcii->afCmd & CBF_SKIP_CONNECT_CONFIRMS)) {
            DoCallback(pcii,
                    (WORD)XTYP_CONNECT_CONFIRM,
                    0,
                    psi->ci.hConv,
                    (HSZ)laTopic,
                    (HSZ)laService,
                    (HDDEDATA)0,
                    0,
                    (flags & ST_ISSELF) ? 1L : 0L);
        }

        MONCONV((PCONV_INFO)psi, TRUE);

        if (!(flags & ST_INLIST)) {
            break;      // our partner's only gonna take the first one anyway.
        }
        php++;
    }

    if (fWild) {
        DdeUnaccessData(hData);
        InternalFreeDataHandle(hData, FALSE);
    }

Exit:
    DeleteAtom(laFree1);
    DeleteAtom(laFree2);
    LeaveDDECrit;
    return;
}

/***************************************************************************\
* DDEMLClientWndProc
*
* Description:
* Handles DDE client messages for DDEML.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
LRESULT DDEMLClientWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PCL_CONV_INFO pci, pciNew;
    LONG lState;
    LRESULT lRet = 0;
    PWND pwnd;
    PCLS pcls;

    EnterDDECrit;

    pci = (PCL_CONV_INFO)GetWindowLongPtr(hwnd, GWLP_PCI);
    UserAssert(pci == NULL || pci->ci.hwndConv == hwnd);

    switch (message) {
    case WM_DDE_ACK:
        lState = GetWindowLong(hwnd, GWL_CONVSTATE);
        if (lState != CLST_CONNECTED) {

            // Initiation mode

            pciNew = (PCL_CONV_INFO)DDEMLAlloc(sizeof(CL_CONV_INFO));
            if (pciNew == NULL ||
                    (pci != NULL && lState == CLST_SINGLE_INITIALIZING)) {
                PostMessage((HWND)wParam, WM_DDE_TERMINATE, (WPARAM)hwnd, 0);
                goto Exit;
            }

            // PCL_CONV_INFO initialization

            pciNew->ci.pcii = ValidateInstance((HANDLE)GetWindowLongPtr(hwnd, GWLP_CHINST));

            if (pciNew->ci.pcii == NULL) {
                DDEMLFree(pciNew);
                goto Exit;
            }

            pciNew->ci.next = (PCONV_INFO)pci; // pci may be NULL
            //
            // Seting GWLP_PCI gives feedback to ConnectConv() which issued
            // the WM_DDE_INITIATE message.
            //
            SetWindowLongPtr(hwnd, GWLP_PCI, (LONG_PTR)pciNew);
            // pciNew->hUser = 0; // Zero init.

            // BUG: If this fails we can have some nasty problems
            pciNew->ci.hConv = (HCONV)CreateHandle((ULONG_PTR)pciNew,
                    HTYPE_CLIENT_CONVERSATION, InstFromHandle(pciNew->ci.pcii->hInstClient));

            pciNew->ci.laService = GlobalToLocalAtom(LOWORD(lParam)); // pci copy
            GlobalDeleteAtom(LOWORD(lParam));
            pciNew->ci.laTopic = GlobalToLocalAtom(HIWORD(lParam)); // pci copy
            GlobalDeleteAtom(HIWORD(lParam));
            pciNew->ci.hwndPartner = (HWND)wParam;
            pciNew->ci.hwndConv = hwnd;
            pciNew->ci.state = (WORD)(ST_CONNECTED | ST_CLIENT |
                    pciNew->ci.pcii->ConvStartupState);
            SetCommonStateFlags(hwnd, (HWND)wParam, &pciNew->ci.state);

            pwnd = ValidateHwnd((HWND)wParam);

            if (pwnd == NULL) goto Exit;
            pcls = (PCLS)REBASEALWAYS(pwnd, pcls);

            if (!TestWF(pwnd, WFANSIPROC)) {
                if (pcls->atomClassName == gpsi->atomSysClass[ICLS_DDEMLSERVERW]) {
                    pciNew->ci.state |= ST_ISLOCAL;
                }
            } else {
                if (pcls->atomClassName == gpsi->atomSysClass[ICLS_DDEMLSERVERA]) {
                    pciNew->ci.state |= ST_ISLOCAL;
                }
            }

            // pciNew->ci.laServiceRequested = 0; // Set by InitiateEnumerationProc()
            // pciNew->ci.pxiIn = 0;
            // pciNew->ci.pxiOut = 0;
            // pciNew->ci.dmqIn = 0;
            // pciNew->ci.dmqOut = 0;
            // pciNew->ci.aLinks = NULL;
            // pciNew->ci.cLinks = 0;
            // pciNew->ci.cLocks = 0;
            goto Exit;
        }
        // fall through to handle posted messages here.

    case WM_DDE_DATA:
        ProcessAsyncDDEMsg((PCONV_INFO)pci, message, (HWND)wParam, lParam);
        goto Exit;

    case WM_DDE_TERMINATE:
    case WM_DESTROY:
        {
            ProcessTerminateMsg((PCONV_INFO)pci, (HWND)wParam);
            break;
        }
    }

    lRet = DefWindowProc(hwnd, message, wParam, lParam);

Exit:
    LeaveDDECrit;
    return (lRet);
}




/***************************************************************************\
* DDEMLServerWndProc
*
* Description:
* Handles DDE server messages.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
LRESULT DDEMLServerWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PSVR_CONV_INFO psi;
    LRESULT lRet = 0;

    EnterDDECrit;

    psi = (PSVR_CONV_INFO)GetWindowLongPtr(hwnd, GWLP_PSI);
    UserAssert(psi == NULL || psi->ci.hwndConv == hwnd);

    switch (message) {
    case WM_DDE_REQUEST:
    case WM_DDE_POKE:
    case WM_DDE_ADVISE:
    case WM_DDE_EXECUTE:
    case WM_DDE_ACK:
    case WM_DDE_UNADVISE:
        ProcessAsyncDDEMsg((PCONV_INFO)psi, message, (HWND)wParam, lParam);
        goto Exit;

    case WM_DDE_TERMINATE:
    case WM_DESTROY:
        ProcessTerminateMsg((PCONV_INFO)psi, (HWND)wParam);
        break;
    }
    lRet = DefWindowProc(hwnd, message, wParam, lParam);
Exit:
    LeaveDDECrit;
    return (lRet);
}




/***************************************************************************\
* ProcessTerminateMsg
*
* Description:
* Handles WM_DDE_TERMINATE messages for both sides.
*
* History:
* 11-26-91 sanfords Created.
\***************************************************************************/
PCONV_INFO ProcessTerminateMsg(
PCONV_INFO pcoi,
HWND hwndFrom)
{
    while (pcoi != NULL && pcoi->hwndPartner != hwndFrom) {
        pcoi = pcoi->next;
    }
    if (pcoi != NULL) {
        pcoi->state |= ST_TERMINATE_RECEIVED;
        ShutdownConversation(pcoi, TRUE);
    }
    return (pcoi);
}



/***************************************************************************\
* ProcessAsyncDDEMsg
*
* Description:
* Handles incoming DDE messages by either calling ProcessSyncDDEMessage()
* if the conversation is able to handle callbacks, or by queuing the
* incoming message into the conversations message queue. Doing this
* allows simpler code in that no message is processed unless the code
* can perform synchronous callbacks.
*
* History:
* 11-26-91 sanfords Created.
\***************************************************************************/
VOID ProcessAsyncDDEMsg(
PCONV_INFO pcoi,
UINT msg,
HWND hwndFrom,
LPARAM lParam)
{
    PDDE_MESSAGE_QUEUE pdmq;
#if DBG
    HWND hwndT = pcoi->hwndConv;
#endif // DBG

    while (pcoi != NULL && pcoi->hwndPartner != hwndFrom) {
        pcoi = pcoi->next;
    }
    if (pcoi == NULL) {
        RIPMSG3(RIP_WARNING,
                "Bogus DDE message %x received from %x by %x. Dumping.",
                msg, hwndFrom, hwndT);
        DumpDDEMessage(FALSE, msg, lParam);
        return ;
    }
    if (pcoi->state & ST_CONNECTED) {

        if (pcoi->dmqOut == NULL &&
                !(pcoi->state & ST_BLOCKED)
//                && !PctiCurrent()->cInDDEMLCallback
                ) {

            if (ProcessSyncDDEMessage(pcoi, msg, lParam)) {
                return; // not blocked, ok to return.
            }
        }

        // enter into queue

        pdmq = DDEMLAlloc(sizeof(DDE_MESSAGE_QUEUE));
        if (pdmq == NULL) {

            // insufficient memory - we can't process this msg - we MUST
            // terminate.

            if (pcoi->state & ST_CONNECTED) {
                PostMessage(pcoi->hwndPartner, WM_DDE_TERMINATE,
                        (WPARAM)pcoi->hwndConv, 0);
                pcoi->state &= ~ST_CONNECTED;
            }
            DumpDDEMessage(!(pcoi->state & ST_INTRA_PROCESS), msg, lParam);
            return ;
        }
        pdmq->pcoi = pcoi;
        pdmq->msg = msg;
        pdmq->lParam = lParam;
        pdmq->next = NULL;

        // dmqOut->next->next->next->dmqIn->NULL

        if (pcoi->dmqIn != NULL) {
            pcoi->dmqIn->next = pdmq;
        }
        pcoi->dmqIn = pdmq;
        if (pcoi->dmqOut == NULL) {
            pcoi->dmqOut = pcoi->dmqIn;
        }
        pcoi->cLocks++;
        CheckForQueuedMessages(pcoi);
        pcoi->cLocks--;
        if (pcoi->cLocks == 0 && pcoi->state & ST_FREE_CONV_RES_NOW) {
            FreeConversationResources(pcoi);
        }
    } else {
        DumpDDEMessage(!(pcoi->state & ST_INTRA_PROCESS), msg, lParam);
    }
}







/***************************************************************************\
* CheckForQueuedMessages
*
* Description:
* Handles processing of DDE messages held in the given conversaion's
* DDE message queue.
*
* Returns: fProcessed.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL CheckForQueuedMessages(
PCONV_INFO pcoi)
{
    PDDE_MESSAGE_QUEUE pdmq;
    BOOL fRet = FALSE;
    PCLIENTINFO pci;

    CheckDDECritIn;

    if (pcoi->state & ST_PROCESSING) {      // recursion prevention
        return(FALSE);
    }

    UserAssert(pcoi->cLocks);

    pci = GetClientInfo();

    pcoi->state |= ST_PROCESSING;
    while (!(pcoi->state & ST_BLOCKED) &&
                pcoi->dmqOut != NULL &&
                !pci->cInDDEMLCallback) {
        pci->CI_flags |= CI_PROCESSING_QUEUE;
        if (ProcessSyncDDEMessage(pcoi, pcoi->dmqOut->msg, pcoi->dmqOut->lParam)) {
            fRet = TRUE;
            pdmq = pcoi->dmqOut;
            pcoi->dmqOut = pcoi->dmqOut->next;
            if (pcoi->dmqOut == NULL) {
                pcoi->dmqIn = NULL;
            }
            DDEMLFree(pdmq);
        }
        pci->CI_flags &= ~CI_PROCESSING_QUEUE;
    }
    pcoi->state &= ~ST_PROCESSING;
    return(fRet);
}




/***************************************************************************\
* DumpDDEMessage
*
* Description:
* Used to clean up resources referenced by DDE messages that for some
* reason could not be processed.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
VOID DumpDDEMessage(
BOOL fFreeData,
UINT msg,
LPARAM lParam)
{
    UINT_PTR uiLo, uiHi;

    RIPMSG2(RIP_WARNING, "Dump DDE msg %x lParam %x", msg, lParam);

    switch (msg) {
    case WM_DDE_ACK:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
    case WM_DDE_ADVISE:
        UnpackDDElParam(msg, lParam, &uiLo, &uiHi);
        switch (msg) {
        case WM_DDE_DATA:
        case WM_DDE_POKE:
            if (uiLo) {
                if (fFreeData) {
                    FreeDDEData((HANDLE)uiLo, FALSE, TRUE);
                }
                GlobalDeleteAtom((ATOM)uiHi);
            }
            break;

        case WM_DDE_ADVISE:
            if (uiLo) {
                if (fFreeData) {
                    FreeDDEData((HANDLE)uiLo, FALSE, TRUE);
                }
                GlobalDeleteAtom((ATOM)uiHi);
            }
            break;

        case WM_DDE_ACK:
            // could be EXEC Ack - cant know what to do exactly.
            break;
        }
        FreeDDElParam(msg, lParam);
        break;

    case WM_DDE_EXECUTE:
        if (fFreeData) {
            WOWGLOBALFREE((HANDLE)lParam);
        }
        break;

    case WM_DDE_REQUEST:
    case WM_DDE_UNADVISE:
        GlobalDeleteAtom((ATOM)HIWORD(lParam));
        break;
    }
}




/***************************************************************************\
* ProcessSyncDDEMessage
*
* Description:
* Handles processing of a received DDE message. TRUE is returned if
* the message was handled. FALSE implies CBR_BLOCK.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL ProcessSyncDDEMessage(
PCONV_INFO pcoi,
UINT msg,
LPARAM lParam)
{
    BOOL fNotBlocked = TRUE;
    PCL_INSTANCE_INFO pcii;
    ENABLE_ENUM_STRUCT ees;
    BOOL fRet;

    CheckDDECritIn;

    /*
     * lock the conversation so its resources don't go away till we are
     * done with them.  This function could generate a callback which could
     * disconnect the conversation.
     */
    pcoi->cLocks++;

    if (pcoi->state & ST_BLOCKNEXT) {
        pcoi->state ^= ST_BLOCKNEXT | ST_BLOCKED;
    }
    if (pcoi->state & ST_BLOCKALLNEXT) {
        ees.pfRet = &fRet;
        ees.wCmd = EC_DISABLE;
        ees.wCmd2 = 0;
        EnumChildWindows(pcoi->pcii->hwndMother, (WNDENUMPROC)EnableEnumProc,
                (LPARAM)&ees);
    }

    if (pcoi->state & ST_CONNECTED) {
        if (pcoi->pxiOut == NULL) {
            if (pcoi->state & ST_CLIENT) {
                fNotBlocked = SpontaneousClientMessage((PCL_CONV_INFO)pcoi, msg, lParam);
            } else {
                fNotBlocked = SpontaneousServerMessage((PSVR_CONV_INFO)pcoi, msg, lParam);
            }
        } else {
            UserAssert(pcoi->pxiOut->hXact == (HANDLE)0 ||
                    ValidateCHandle(pcoi->pxiOut->hXact, HTYPE_TRANSACTION,
                    HINST_ANY)
                    == (ULONG_PTR)pcoi->pxiOut);
            fNotBlocked = (pcoi->pxiOut->pfnResponse)(pcoi->pxiOut, msg, lParam);
        }
    } else {
        DumpDDEMessage(!(pcoi->state & ST_INTRA_PROCESS), msg, lParam);
    }
    if (!fNotBlocked) {
        pcoi->state |= ST_BLOCKED;
        pcoi->state &= ~ST_BLOCKNEXT;
    }

    pcii = pcoi->pcii;  // save this incase unlocking makes pcoi go away.

    pcoi->cLocks--;
    if (pcoi->cLocks == 0 && pcoi->state & ST_FREE_CONV_RES_NOW) {
        FreeConversationResources(pcoi);
    }

    /*
     * Because callbacks are capable of blocking DdeUninitialize(), we check
     * before exit to see if it needs to be called.
     */
    if (pcii->afCmd & APPCMD_UNINIT_ASAP &&
            !(pcii->flags & IIF_IN_SYNC_XACT) &&
            !pcii->cInDDEMLCallback) {
        DdeUninitialize(HandleToUlong(pcii->hInstClient));
        return(FALSE);
    }
    return (fNotBlocked);
}
