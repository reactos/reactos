/****************************** Module Header ******************************\
* Module Name: connect.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* DDE Manager conversation connection functions
*
* Created: 11/3/91 Sanford Staab
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "nddeagnt.h"

//#define TESTING
#ifdef TESTING
ULONG
DbgPrint(
    PCH Format,
    ...
    );
VOID
DbgUserBreakPoint(
    VOID
    );

BOOL ValidateConvList(
HCONVLIST hConvList)
{
    PCONVLIST pcl;
    PCL_CONV_INFO pci;
    PXACT_INFO pxi;
    int i;
    BOOL fMatch;

    if (hConvList == 0) {
        return(TRUE);
    }
    pcl = (PCONVLIST)ValidateCHandle((HANDLE)hConvList,
                                     HTYPE_CONVERSATION_LIST,
                                     HINST_ANY);
    for (i = 0; i < pcl->chwnd; i++) {
        /*
         * all windows in the list are valid
         */
        if (!IsWindow(pcl->ahwnd[i])) {
            DebugBreak();
        }
        pci = (PCL_CONV_INFO)GetWindowLongPtr(pcl->ahwnd[i], GWLP_PCI);
       /*
        * All windows have at least one convinfo associated with them.
        */
        if (pci == NULL) {
            DebugBreak();
        }
        fMatch = FALSE;
        while (pci != NULL) {
            /*
             * All non-zombie conversations have hConvList set correctly.
             */
            if (pci->hConvList != hConvList &&
                    TypeFromHandle(pci->ci.hConv) != HTYPE_ZOMBIE_CONVERSATION) {
                DebugBreak();
            }
            /*
             * All conversations have hConvList clear or set correctly.
             */
            if (pci->hConvList != 0 && pci->hConvList != hConvList) {
                DebugBreak();
            }
            /*
             * At least 1 of the conversations references the list
             */
            if (pci->hConvList == hConvList) {
                fMatch = TRUE;
            }
            for (pxi = pci->ci.pxiOut; pxi; pxi = pxi->next) {
                if ((PCL_CONV_INFO)pxi->pcoi != pci) {
                    DebugBreak();
                }
            }
            pci = (PCL_CONV_INFO)pci->ci.next;
        }
        if (!fMatch) {
            /*
             * At least 1 of the conversations references the list
             */
            DebugBreak;
        }
    }
    return(TRUE);
}

VOID ValidateAllConvLists()
{
    ApplyFunctionToObjects(HTYPE_CONVERSATION_LIST, HINST_ANY,
            (PFNHANDLEAPPLY)ValidateConvList);
}

#else // TESTING
#define ValidateConvList(h)
#define ValidateAllConvLists()
#endif // TESTING

CONVCONTEXT TempConvContext;
CONVCONTEXT DefConvContext = {
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

typedef struct tagINIT_ENUM {
    HWND hwndClient;
    HWND hwndSkip;
    LONG lParam;
    LATOM laServiceRequested;
    LATOM laTopic;
    HCONVLIST hConvList;
    DWORD clst;
} INIT_ENUM, *PINIT_ENUM;


BOOL InitiateEnumerationProc(HWND hwndTarget, PINIT_ENUM pie);
VOID DisconnectConv(PCONV_INFO pcoi);


/***************************************************************************\
* DdeConnect (DDEML API)
*
* Description:
* Initiates a DDE conversation.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
HCONV DdeConnect(
    DWORD idInst,
    HSZ hszService,
    HSZ hszTopic,
    PCONVCONTEXT pCC)
{
    PCL_INSTANCE_INFO pcii;
    PCL_CONV_INFO pci;
    HCONV hConvRet = 0;
    HWND hwndTarget = 0;
    LATOM aNormalSvcName = 0;

    EnterDDECrit;

    if (!ValidateConnectParameters((HANDLE)LongToHandle( idInst ), &pcii, &hszService, hszTopic,
            &aNormalSvcName, &pCC, &hwndTarget, 0)) {
        goto Exit;
    }
    pci = ConnectConv(pcii, LATOM_FROM_HSZ(hszService), LATOM_FROM_HSZ(hszTopic),
            hwndTarget,
            (pcii->afCmd & CBF_FAIL_SELFCONNECTIONS) ? pcii->hwndMother : 0,
            pCC, 0, CLST_SINGLE_INITIALIZING);
    if (pci == NULL) {
        SetLastDDEMLError(pcii, DMLERR_NO_CONV_ESTABLISHED);
        goto Exit;
    } else {
        hConvRet = pci->ci.hConv;
    }

Exit:
    if (aNormalSvcName) {
        GlobalDeleteAtom(aNormalSvcName);
    }
    LeaveDDECrit;
    return (hConvRet);
}



/***************************************************************************\
* DdeConnectList (DDEML API)
*
* Description:
* Initiates DDE conversations with multiple servers or adds unique servers
* to an existing conversation list.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
HCONVLIST DdeConnectList(
    DWORD idInst,
    HSZ hszService,
    HSZ hszTopic,
    HCONVLIST hConvList,
    PCONVCONTEXT pCC)
{
    PCL_INSTANCE_INFO pcii;
    PCONV_INFO pcoi, pcoiNew, pcoiExisting, pcoiNext;
    HCONVLIST hConvListRet = 0;
    HWND hwndTarget = 0;
    LATOM aNormalSvcName = 0;
    PCONVLIST pcl = NULL;
    HCONVLIST hConvListOld;
    int i;

    CheckDDECritOut;

    EnterDDECrit;

    if (!ValidateConnectParameters((HANDLE)LongToHandle( idInst ), &pcii, &hszService, hszTopic,
            &aNormalSvcName, &pCC, &hwndTarget, hConvList)) {
        goto Exit;
    }

    ValidateConvList(hConvList);

    hConvListOld = hConvList;
    pcoi = (PCONV_INFO)ConnectConv(pcii,
            LATOM_FROM_HSZ(hszService),
            LATOM_FROM_HSZ(hszTopic),
            hwndTarget,
            (pcii->afCmd & (CBF_FAIL_SELFCONNECTIONS | CBF_FAIL_CONNECTIONS)) ?
                pcii->hwndMother : 0,
            pCC,
            hConvListOld,
            CLST_MULT_INITIALIZING);

    if (pcoi == NULL) {
        /*
         * no new connections made
         */
        SetLastDDEMLError(pcii, DMLERR_NO_CONV_ESTABLISHED);
        hConvListRet = hConvListOld;
        goto Exit;
    }

    /*
     * allocate or reallocate the hConvList hwnd list for later addition
     * If we already have a valid list, reuse the handle so we don't have
     * to alter the preexisting pcoi->hConvList values.
     */
    if (hConvListOld == 0) {
        pcl = (PCONVLIST)DDEMLAlloc(sizeof(CONVLIST));
        if (pcl == NULL) {
            SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
            DisconnectConv(pcoi);
            goto Exit;
        }
        // pcl->chwnd = 0; LPTR zero inits.

        hConvList = (HCONVLIST)CreateHandle((ULONG_PTR)pcl,
                HTYPE_CONVERSATION_LIST, InstFromHandle(pcii->hInstClient));
        if (hConvList == 0) {
            DDEMLFree(pcl);
            SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
            DisconnectConv(pcoi);
            goto Exit;
        }
    } else {
        pcl = (PCONVLIST)GetHandleData((HANDLE)hConvList);
        pcl = DDEMLReAlloc(pcl, sizeof(CONVLIST) + sizeof(HWND) * pcl->chwnd);
        if (pcl == NULL) {
            SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
            hConvListRet = hConvListOld;
            DisconnectConv(pcoi);
            goto Exit;
        }
        SetHandleData((HANDLE)hConvList, (ULONG_PTR)pcl);
    }

    ValidateConvList(hConvListOld);

    if (hConvListOld) {
        /*
         * remove duplicates from new conversations
         *
         * Although we tried to prevent duplicates from happening
         * within the initiate enumeration code, wild initiates or
         * servers responding with different service names than
         * requested could cause duplicates.
         */

        /* For each client window... */

        for (i = 0; i < pcl->chwnd; i++) {

        /* For each existing conversation in that window... */

            for (pcoiExisting = (PCONV_INFO)
                        GetWindowLongPtr(pcl->ahwnd[i], GWLP_PCI);
                    pcoi != NULL && pcoiExisting != NULL;
                    pcoiExisting = pcoiExisting->next) {

                if (!(pcoiExisting->state & ST_CONNECTED))
                    continue;

        /* For each new conversation... */

                for (pcoiNew = pcoi; pcoiNew != NULL; pcoiNew = pcoiNext) {

                    pcoiNext = pcoiNew->next;

        /* see if the new conversation duplicates the existing one */

                    if (!(pcoiNew->state & ST_CONNECTED))
                        continue;

                    UserAssert(((PCL_CONV_INFO)pcoiExisting)->hwndReconnect);
                    UserAssert(((PCL_CONV_INFO)pcoiNew)->hwndReconnect);

                    if (((PCL_CONV_INFO)pcoiExisting)->hwndReconnect ==
                                ((PCL_CONV_INFO)pcoiNew)->hwndReconnect &&
                            pcoiExisting->laTopic == pcoiNew->laTopic &&
                            pcoiExisting->laService == pcoiNew->laService) {
                        /*
                         * duplicate conversation - disconnection causes an unlink
                         */
                        if (pcoiNew == pcoi) {
                            /*
                             * We are freeing up the head of the list,
                             * Reset the head to the next guy.
                             */
                            pcoi = pcoiNext;
                        }
                        ValidateConvList(hConvList);
                        ShutdownConversation(pcoiNew, FALSE);
                        ValidateConvList(hConvList);
                        break;
                    }
                }
            }
        }

        for (pcoiExisting = pcoi; pcoiExisting != NULL; pcoiExisting = pcoiExisting->next) {
            /*
             * if these are all zombies - we DONT want to link it in!
             * This is possible because ShutdownConversation() leaves the critical section
             * and could allow responding terminates to come through.
             */
            if (pcoiExisting->state & ST_CONNECTED) {
                goto FoundOne;
            }
        }
        pcoi = NULL;    // abandon this guy - he will clean up in time.
FoundOne:
        /*
         * add new pcoi (if any are left) hwnd to ConvList hwnd list.
         */
        if (pcoi != NULL) {
            UserAssert(pcoi->hwndConv);
            pcl->ahwnd[pcl->chwnd] = pcoi->hwndConv;
            pcl->chwnd++;
            hConvListRet = hConvList;
        } else {
            hConvListRet = hConvListOld;
            if (!hConvListOld) {
                DestroyHandle((HANDLE)hConvList);
            }
        }


    } else {    // no hConvListOld

        UserAssert(pcoi->hwndConv);
        pcl->ahwnd[0] = pcoi->hwndConv;
        pcl->chwnd = 1;
        hConvListRet = hConvList;
    }

    if (pcoi != NULL) {
        /*
         * set hConvList field for all remaining new conversations.
         */
        UserAssert(hConvListRet);
        for (pcoiNew = pcoi; pcoiNew != NULL; pcoiNew = pcoiNew->next) {
            if (pcoiNew->state & ST_CONNECTED) {
                ((PCL_CONV_INFO)pcoiNew)->hConvList = hConvListRet;
            }
        }
    }

Exit:
    if (aNormalSvcName) {
        DeleteAtom(aNormalSvcName);
    }
    ValidateConvList(hConvListRet);
    LeaveDDECrit;
    return (hConvListRet);
}




/***************************************************************************\
* DdeReconnect (DDEML API)
*
* Description:
* Attempts to reconnect an externally (from the server) terminated
* client side conversation.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
HCONV DdeReconnect(
    HCONV hConv)
{
    PCL_INSTANCE_INFO pcii;
    PCL_CONV_INFO pci, pciNew;
    HCONV hConvRet = 0;
    CONVCONTEXT cc;

    EnterDDECrit;

    pcii = PciiFromHandle((HANDLE)hConv);
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    pci = (PCL_CONV_INFO)ValidateCHandle((HANDLE)hConv,
            HTYPE_CLIENT_CONVERSATION, HINST_ANY);
    if (pci == NULL) {
        SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    if (pci->ci.state & ST_CONNECTED) {
        goto Exit;
    }

    GetConvContext(pci->ci.hwndConv, (LONG *)&cc);
    pciNew = ConnectConv(pcii, pci->ci.laService, pci->ci.laTopic,
            pci->hwndReconnect, 0, &cc, 0, CLST_SINGLE_INITIALIZING);
    if (pciNew == NULL) {
        SetLastDDEMLError(pcii, DMLERR_NO_CONV_ESTABLISHED);
        goto Exit;
    } else {
        hConvRet = pciNew->ci.hConv;
        if (pci->ci.cLinks) {
            PXACT_INFO pxi;
            int iLink;
            PADVISE_LINK paLink;

            /*
             * reestablish advise links
             */

            for (paLink = pci->ci.aLinks, iLink = pci->ci.cLinks;
                    iLink; paLink++, iLink--) {

                pxi = (PXACT_INFO)DDEMLAlloc(sizeof(XACT_INFO));
                if (pxi == NULL) {
                    break;              // abort relinking
                }
                pxi->pcoi = (PCONV_INFO)pciNew;
                pxi->gaItem = LocalToGlobalAtom(paLink->laItem); // pxi copy
                pxi->wFmt = paLink->wFmt;
                pxi->wType = (WORD)((paLink->wType >> 12) | XTYP_ADVSTART);
                if (ClStartAdvise(pxi)) {
                    pxi->flags |= XIF_ABANDONED;
                } else {
                    GlobalDeleteAtom(pxi->gaItem);
                    DDEMLFree(pxi);
                }
            }
        }
    }

Exit:
    LeaveDDECrit;
    return (hConvRet);
}


/***************************************************************************\
* ValidateConnectParameters
*
* Description:
* worker function to handle common validation code.
*
* Note that paNormalSvcName is set to the atom value created upon extracting
* a normal HSZ from an InstanceSpecific HSZ.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL ValidateConnectParameters(
    HANDLE hInst,
    PCL_INSTANCE_INFO *ppcii, // set if valid hInst
    HSZ *phszService, // altered if InstSpecific HSZ
    HSZ hszTopic,
    LATOM *plaNormalSvcName, // set to atom that needs freeing when done
    PCONVCONTEXT *ppCC, // set to point to DefConvContext if NULL
    HWND *phwndTarget, // set if hszService is InstSpecific
    HCONVLIST hConvList)
{
    DWORD hszType;
    BOOL fError = FALSE;

    *ppcii = ValidateInstance(hInst);
    if (*ppcii == NULL) {
        return (FALSE);
    }
    hszType = ValidateHSZ(*phszService);
    if (hszType == HSZT_INVALID || ValidateHSZ(hszTopic) == HSZT_INVALID) {
        SetLastDDEMLError(*ppcii, DMLERR_INVALIDPARAMETER);
        return (FALSE);
    }
    if (hszType == HSZT_INST_SPECIFIC) {
        *phwndTarget = ParseInstSpecificAtom(LATOM_FROM_HSZ(*phszService),
            plaNormalSvcName);
        if (*plaNormalSvcName == 0) {
            SetLastDDEMLError(*ppcii, DMLERR_SYS_ERROR);
            return (FALSE);
        }
        *phszService = NORMAL_HSZ_FROM_LATOM(*plaNormalSvcName);
    }
    if (*ppCC == NULL) {
        *ppCC = &DefConvContext;
        if ((*ppcii)->flags & IIF_UNICODE) {
            (*ppCC)->iCodePage = CP_WINUNICODE;
        } else {
            (*ppCC)->iCodePage = CP_WINANSI;
        }
    } else try {
        if ((*ppCC)->cb > sizeof(CONVCONTEXT)) {
            SetLastDDEMLError(*ppcii, DMLERR_INVALIDPARAMETER);
            fError = TRUE;
        } else if ((*ppCC)->cb < sizeof(CONVCONTEXT)) {
            TempConvContext = DefConvContext;
            /*
             * we can use this static temp because we are synchronized.
             */
            RtlCopyMemory(&TempConvContext, *ppCC, (*ppCC)->cb);
            *ppCC = &TempConvContext;
        }
    } except(W32ExceptionHandler(FALSE, RIP_WARNING)) {
        SetLastDDEMLError(*ppcii, DMLERR_INVALIDPARAMETER);
        fError = TRUE;
    }
    if (fError) {
        return(FALSE);
    }
    if (hConvList != 0 &&
            !ValidateCHandle((HANDLE)hConvList, HTYPE_CONVERSATION_LIST,
            (DWORD)InstFromHandle((*ppcii)->hInstClient))) {
        return (FALSE);
    }
    return (TRUE);
}



/***************************************************************************\
* ConnectConv
*
* Description:
* Work function for all Connect cases.
*
* Method:
*
* To reduce the number of windows we use and to simplify how client
* windows handle multiple WM_DDE_ACK messages during initiation, a
* single client window can handle many conversations, each with
* a different server window.
*
* The client window is created and set to a initiation state via the
* GWL_CONVSTATE window word. Initiates are then sent to enumerated server
* window candidates.
* The GWL_CONVSTATE value is used by the DDEML mother windows
* to determine if only one or several ACKs are desired to minimize
* unnessary message traffic.
*
* The client window GWL_CONVCONTEXT? window words are also used by
* Event Windows to pass context information.
*
* Note that all client and server windows are children of the mother
* window. This reduces the number of top level windows that
* WM_DDE_INITIATES need to hit.
*
* Each WM_DDE_ACK that is received by a client window while in the
* initiation state causes it to create a CL_CONV_INFO structure,
* partially initialize it, and link it into its list of CL_CONV_INFO
* structures. The head of the list is pointed to by the GWLP_PCI
* client window word.
*
* After each WM_DDE_INITIALIZE is sent, the GWLP_PCI value is checked
* to see if it exists and needs initialization to be completed. If
* this is the case the init code knows that at least one ACK was
* received in response to the WM_DDE_INITIALIZE send. The
* initialization of each CL_CONV_INFO struct that needs it is then completed.
*
* Once the broadcasting of WM_DDE_INITIALIZE is done, the init code
* then sets the GWL_CONVSTATE value in the client window to indicate that
* initialization is complete.
*
* Returns:
* The head pci to the client window or NULL if no connections made it.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
PCL_CONV_INFO ConnectConv(
    PCL_INSTANCE_INFO pcii,
    LATOM laService,
    LATOM laTopic,
    HWND hwndTarget, // 0 implies broadcast
    HWND hwndSkip, // 0 implies no skips - avoids self-connections.
    PCONVCONTEXT pCC,
    HCONVLIST hConvList,
    DWORD clst)
{
    INIT_ENUM ie;
    PCL_CONV_INFO pci;
    PCONV_INFO pcoi;
    GATOM gaService, gaTopic;

    CheckDDECritIn;

    if (hwndTarget && hwndTarget == hwndSkip) {
        return(NULL);
    }

    LeaveDDECrit;
    CheckDDECritOut;

    if (pcii->flags & IIF_UNICODE) {
        ie.hwndClient = CreateWindowW((LPWSTR)(gpsi->atomSysClass[ICLS_DDEMLCLIENTW]),
                                     L"",
                                     WS_CHILD,
                                     0, 0, 0, 0,
                                     pcii->hwndMother,
                                     (HMENU)0,
                                     (HANDLE)0,
                                     (LPVOID)NULL);
    } else {
        ie.hwndClient = CreateWindowA((LPSTR)(gpsi->atomSysClass[ICLS_DDEMLCLIENTA]),
                                     "",
                                     WS_CHILD,
                                     0, 0, 0, 0,
                                     pcii->hwndMother,
                                     (HMENU)0,
                                     (HANDLE)0,
                                     (LPVOID)NULL);
    }

    EnterDDECrit;

    if (ie.hwndClient == 0) {
        return (NULL);
    }

    if (pCC != NULL) {
        if (!NtUserDdeSetQualityOfService(ie.hwndClient, &(pCC->qos), NULL)) {
            SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
            goto Error;
        }
    }
    /*
     * Note that a pci will be created and allocated for each ACK recieved.
     */
    SetConvContext(ie.hwndClient, (LONG *)pCC);
    SetWindowLong(ie.hwndClient, GWL_CONVSTATE, clst);
    SetWindowLongPtr(ie.hwndClient, GWLP_SHINST, (LONG_PTR)pcii->hInstServer);
    SetWindowLongPtr(ie.hwndClient, GWLP_CHINST, (LONG_PTR)pcii->hInstClient);

    gaService = LocalToGlobalAtom(laService);
    gaTopic = LocalToGlobalAtom(laTopic);
    ie.lParam = MAKELONG(gaService, gaTopic);
    if (!hwndTarget) {
        ie.hwndSkip = hwndSkip;
        ie.laServiceRequested = laService;
        ie.laTopic = laTopic;
        ie.hConvList = hConvList;
        ie.clst = clst;
    }

    LeaveDDECrit;

    if (hwndTarget) {
        SendMessage(hwndTarget, WM_DDE_INITIATE, (WPARAM)ie.hwndClient,
                ie.lParam);
    } else {
        /*
         * Send this message to the nddeagnt app first so it can start
         * the netdde services BEFORE we do an enumeration of windows.
         * This lets things work the first time.  NetDDEAgent caches
         * service status so this is the fastest way to do this.
         */
        HWND hwndAgent = FindWindowW(SZ_NDDEAGNT_CLASS, SZ_NDDEAGNT_TITLE);
        if (hwndAgent) {
            SendMessage(hwndAgent,
                WM_DDE_INITIATE, (WPARAM)ie.hwndClient, ie.lParam);
        }
        EnumWindows((WNDENUMPROC)InitiateEnumerationProc, (LPARAM)&ie);
    }

    EnterDDECrit;
    /*
     * hConvList may have been destroyed during the enumeration but we are
     * done with it now so no need to revalidate.
     */

#if DBG
    {
        WCHAR sz[10];

        if (gaService && GlobalGetAtomName(gaService, sz, 10) == 0) {
            RIPMSG1(RIP_ERROR, "Bad Service Atom after Initiate phase: %lX", (DWORD)gaService);
        }
        if (gaTopic && GlobalGetAtomName(gaTopic, sz, 10) == 0) {
            RIPMSG1(RIP_ERROR, "Bad Topic Atom after Initiate phase: %lX", (DWORD)gaTopic);
        }
    }
#endif // DBG

    GlobalDeleteAtom(gaService);
    GlobalDeleteAtom(gaTopic);

    //
    // Get the first pci allocated when a WM_DDE_ACK was recieved.
    //
    pci = (PCL_CONV_INFO)GetWindowLongPtr(ie.hwndClient, GWLP_PCI);
    if (pci == NULL) {
Error:
        LeaveDDECrit;
        NtUserDestroyWindow(ie.hwndClient);
        EnterDDECrit;
        return (NULL);
    }

    SetWindowLong(ie.hwndClient, GWL_CONVSTATE, CLST_CONNECTED);
    if (hwndTarget) {
        /*
         * If hwndTarget was NULL, the enumeration proc took care of this.
         */
        pci->hwndReconnect = hwndTarget;
        UserAssert(pci->ci.next == NULL);
        pci->ci.laServiceRequested = laService;
        IncLocalAtomCount(laService); // pci copy
    }

    if (pcii->MonitorFlags & MF_CONV) {
        for (pcoi = (PCONV_INFO)pci; pcoi; pcoi = pcoi->next) {
            MONCONV(pcoi, TRUE);
        }
    }
    return (pci);
}


/*
 * Undoes the work of ConnectConv()
 */
VOID DisconnectConv(
PCONV_INFO pcoi)
{
    PCONV_INFO pcoiNext;

    for (; pcoi; pcoi = pcoiNext) {
        pcoiNext = pcoi->next;
        ShutdownConversation(pcoi, FALSE);
    }
}


/***************************************************************************\
* InitiateEnumerationProc (FILE LOCAL)
*
* Description:
* Function used via EnumWindows to enumerate all server window candidates
* during DDE initiation. The enumeration allows DDEML to know what
* window WM_DDE_INITIATE was sent to so that it can be remembered for
* possible reconnection later. (The window that receives the WM_DDE_INITIATE
* message is not necessarily going to be the server window.)
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
BOOL InitiateEnumerationProc(
    HWND hwndTarget,
    PINIT_ENUM pie)
{
    PCL_CONV_INFO pci;

    CheckDDECritOut;

    if (hwndTarget == pie->hwndSkip) {
        return (TRUE);
    }

    if (pie->hConvList && pie->laTopic && pie->laServiceRequested) {
        /*
         * Head off duplicates BEFORE we send the WM_DDE_INITIATE messages!
         */
        PCONVLIST pcl;
        PCONV_INFO pcoiExisting;
        int i;

        EnterDDECrit;
        /*
         * We revalidate hConvList here because we left the critical section.
         */
        pcl = (PCONVLIST)ValidateCHandle((HANDLE)pie->hConvList,
                HTYPE_CONVERSATION_LIST, HINST_ANY);
        if (pcl != NULL) {
            for (i = 0; i < pcl->chwnd; i++) {
                for (pcoiExisting = (PCONV_INFO)GetWindowLongPtr(pcl->ahwnd[i], GWLP_PCI);
                        pcoiExisting != NULL;
                        pcoiExisting = pcoiExisting->next) {
                    if (pcoiExisting->state & ST_CONNECTED &&
                            ((PCL_CONV_INFO)pcoiExisting)->hwndReconnect == hwndTarget &&
                            pcoiExisting->laTopic == pie->laTopic &&
                            pcoiExisting->laService == pie->laServiceRequested) {
                        LeaveDDECrit;
                        return(TRUE);
                    }
                }
            }
        }
        LeaveDDECrit;
    }

    CheckDDECritOut;

    SendMessage(hwndTarget, WM_DDE_INITIATE, (WPARAM)pie->hwndClient,
            pie->lParam);

    EnterDDECrit;

    //
    // During the initiate process, any acks received cause more pci's
    // to become linked together under the same hwndClient. Once
    // the SendMessage() returns, we set the parts of the new pci's
    // that hold initiate context information.
    //
    pci = (PCL_CONV_INFO)GetWindowLongPtr(pie->hwndClient, GWLP_PCI);
    if (pci == NULL) {
        LeaveDDECrit;
        return (TRUE);
    }

    while (pci != NULL) {
        if (pci->hwndReconnect == 0) {  // this one needs updating
            pci->hwndReconnect = hwndTarget;
            if (pie->laServiceRequested) {
                pci->ci.laServiceRequested = pie->laServiceRequested;
                IncLocalAtomCount(pie->laServiceRequested); // pci copy
            }
        }
        if (pie->clst == CLST_SINGLE_INITIALIZING) {
            break;
        }
        pci = (PCL_CONV_INFO)pci->ci.next;
    }
    LeaveDDECrit;
    return (pie->clst == CLST_MULT_INITIALIZING);
}




/***************************************************************************\
* SetCommonStateFlags()
*
* Description:
*   Common client/server worker function
*
* History:
* 05-12-91 sanfords Created.
\***************************************************************************/
VOID SetCommonStateFlags(
HWND hwndUs,
HWND hwndThem,
PWORD pwFlags)
{
    DWORD pidUs, pidThem;

    GetWindowThreadProcessId(hwndUs, &pidUs);
    GetWindowThreadProcessId(hwndThem, &pidThem);
    if (pidUs == pidThem) {
        *pwFlags |= ST_INTRA_PROCESS;
    }

    if (IsWindowUnicode(hwndUs) && IsWindowUnicode(hwndThem)) {
        *pwFlags |= ST_UNICODE_EXECUTE;
    }
}




/***************************************************************************\
* DdeQueryNextServer (DDEML API)
*
* Description:
* Enumerates conversations within a list.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
HCONV DdeQueryNextServer(
    HCONVLIST hConvList,
    HCONV hConvPrev)
{
    HCONV hConvRet = 0;
    PCONVLIST pcl;
    HWND *phwnd;
    int i;
    PCL_CONV_INFO pci;
    PCL_INSTANCE_INFO pcii;

    EnterDDECrit;

    pcl = (PCONVLIST)ValidateCHandle((HANDLE)hConvList,
            HTYPE_CONVERSATION_LIST, HINST_ANY);
    if (pcl == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    if (!pcl->chwnd) {      // empty list
        goto Exit;
    }
    pcii = PciiFromHandle((HANDLE)hConvList);
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    pcii->LastError = DMLERR_NO_ERROR;

    do {

        hConvRet = 0;

        if (hConvPrev == 0) {
            pci = (PCL_CONV_INFO)GetWindowLongPtr(pcl->ahwnd[0], GWLP_PCI);
            if (pci == NULL) {
                goto Exit;  // Must have all conversations zombied.
            }
            hConvPrev = hConvRet = pci->ci.hConv;
            continue;
        }

        pci = (PCL_CONV_INFO)ValidateCHandle((HANDLE)hConvPrev,
                HTYPE_CLIENT_CONVERSATION, InstFromHandle(hConvList));
        if (pci == NULL) {
            pci = (PCL_CONV_INFO)ValidateCHandle((HANDLE)hConvPrev,
                    HTYPE_ZOMBIE_CONVERSATION, InstFromHandle(hConvList));
            if (pci == NULL) {
                SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
                break;
            } else {
                goto ZombieSkip;
            }
        }

        if (pci->hConvList != hConvList) {
            SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
            break;
        }

ZombieSkip:

        if (pci->ci.next == NULL) {

            /*
             * end of list for this window, go to next window
             */
            for (phwnd = pcl->ahwnd, i = 0; (i + 1) < pcl->chwnd; i++) {
                if (phwnd[i] == pci->ci.hwndConv) {
                    pci = (PCL_CONV_INFO)GetWindowLongPtr(phwnd[i + 1], GWLP_PCI);
                    if (pci == NULL) {
                        break;
                    }
                    hConvPrev = hConvRet = pci->ci.hConv;
                    break;
                }
            }
        } else {

            hConvPrev = hConvRet = pci->ci.next->hConv; // next conv for this window.
        }

    } while (hConvRet && TypeFromHandle(hConvRet) == HTYPE_ZOMBIE_CONVERSATION);
Exit:
    LeaveDDECrit;
    return (hConvRet);
}





/***************************************************************************\
* DdeDisconnect (DDEML API)
*
* Description:
* Terminates a conversation.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL DdeDisconnect(
    HCONV hConv)
{
    BOOL fRet = FALSE;
    PCONV_INFO pcoi;
    PCL_INSTANCE_INFO pcii;

    CheckDDECritOut;
    EnterDDECrit;

    pcoi = (PCONV_INFO)ValidateCHandle((HANDLE)hConv,
            HTYPE_CLIENT_CONVERSATION, HINST_ANY);
    if (pcoi == NULL) {
        pcoi = (PCONV_INFO)ValidateCHandle((HANDLE)hConv,
                HTYPE_SERVER_CONVERSATION, HINST_ANY);
    }
    if (pcoi == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    pcii = PciiFromHandle((HANDLE)hConv);
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    if (pcoi->state & ST_CONNECTED) {
        ShutdownConversation(pcoi, FALSE);
    }
    fRet = TRUE;

Exit:
    LeaveDDECrit;
    return (fRet);
}


/***************************************************************************\
* DdeDisconnectList (DDEML API)
*
* Description:
* Terminates all conversations in a conversation list and frees the list.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL DdeDisconnectList(
    HCONVLIST hConvList)
{
    BOOL fRet = FALSE;
    PCL_INSTANCE_INFO pcii;
    PCONVLIST pcl;
    PCONV_INFO pcoi, pcoiNext;
    int i;

    CheckDDECritOut;
    EnterDDECrit;

    pcl = (PCONVLIST)ValidateCHandle((HANDLE)hConvList,
            HTYPE_CONVERSATION_LIST, HINST_ANY);
    if (pcl == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    ValidateConvList(hConvList);
    pcii = PciiFromHandle((HANDLE)hConvList);
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    for(i = pcl->chwnd - 1; i >= 0; i--) {
        pcoi = (PCONV_INFO)GetWindowLongPtr(pcl->ahwnd[i], GWLP_PCI);
        while (pcoi != NULL && pcoi->state & ST_CONNECTED) {
            pcoiNext = pcoi->next;
            ShutdownConversation(pcoi, FALSE);  // may unlink pcoi!
            pcoi = pcoiNext;
        }
    }

    DestroyHandle((HANDLE)hConvList);
    DDEMLFree(pcl);
    fRet = TRUE;

Exit:
    LeaveDDECrit;
    return (fRet);
}




/***************************************************************************\
* ShutdownConversation
*
* Description:
* This function causes an imediate termination of the given conversation
* and generates apropriate callbacks to notify the application.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
VOID ShutdownConversation(
    PCONV_INFO pcoi,
    BOOL fMakeCallback)
{
    CheckDDECritIn;

    if (pcoi->state & ST_CONNECTED) {
        pcoi->state &= ~ST_CONNECTED;

        if (IsWindow(pcoi->hwndPartner)) {
            PostMessage(pcoi->hwndPartner, WM_DDE_TERMINATE,
                    (WPARAM)pcoi->hwndConv, 0);
        }
        if (fMakeCallback && !(pcoi->pcii->afCmd & CBF_SKIP_DISCONNECTS)) {
            DoCallback(pcoi->pcii, (WORD)XTYP_DISCONNECT, 0, pcoi->hConv,
                    0, 0, 0, 0, (pcoi->state & ST_ISSELF) ? 1L : 0L);
        }
        MONCONV(pcoi, FALSE);
    }

    FreeConversationResources(pcoi);
}



/***************************************************************************\
* UnlinkConvFromOthers
*
* Description:
*
* Helper function to handle ugly cross dependency removal.  If we are
* unlinking a conversation that is going zombie, fGoingZombie is TRUE;
*
* Conversations that are going zombie are in phase 1 of a 2 phase unlink.
* Phase 1 unlinks do not remove the pcoi from its hwnd's list.
* All unlinks should result in:
*   pcoi->hConvList = 0;
*   hConvList/aServerLookup no longer refrences pcoi->hwndConv unless
*       one of the pcoi's related to hwndConv is still active.
*
*
* History:
*  3-2-92 sanfords Created.
\***************************************************************************/
VOID UnlinkConvFromOthers(
PCONV_INFO pcoi,
BOOL gGoingZombie)
{
    PCONV_INFO pcoiPrev, pcoiFirst, pcoiNow;
    PCONVLIST pcl;
    int i, cActiveInList = 0;
#ifdef TESTING
    DWORD path = 0;
#define ORPATH(x) path |= x;
#else
#define ORPATH(x)
#endif // TESTING

    CheckDDECritIn;

    /*
     * Scan pcoi linked list to get key pointers.
     */
    pcoiPrev = NULL;
    pcoiFirst = pcoiNow = (PCONV_INFO)GetWindowLongPtr(pcoi->hwndConv, GWLP_PCI);

#ifdef TESTING
    /*
     * verify that pcoi is in the conv list for this window.
     */
    while (pcoiNow != NULL) {
        if (pcoiNow == pcoi) {
            goto FoundIt;
        }
        pcoiNow = pcoiNow->next;
    }
    DebugBreak();
FoundIt:
    pcoiNow = pcoiFirst;
#endif // TESTING

    UserAssert(pcoiFirst);
    while (pcoiNow != NULL) {
        if (TypeFromHandle(pcoiNow->hConv) != HTYPE_ZOMBIE_CONVERSATION) {
            ORPATH(1);
            cActiveInList++;
        }
        if (pcoiNow->next == pcoi) {
            pcoiPrev = pcoiNow;
        }
        pcoiNow = pcoiNow->next;
    }

    ValidateAllConvLists();

    /*
     * Unlink conversation unless its going Zombie.
     */
    if (!gGoingZombie) {
        ORPATH(2);
        if (TypeFromHandle(pcoi->hConv) != HTYPE_ZOMBIE_CONVERSATION) {
            ORPATH(4);
            cActiveInList--;
        }

        if (pcoiPrev == NULL) {
            ORPATH(8);
            pcoiFirst = pcoi->next;
            SetWindowLongPtr(pcoi->hwndConv, GWLP_PCI, (LONG_PTR)pcoiFirst);
        } else {
            pcoiPrev->next = pcoi->next;
        }
    }

    UserAssert(pcoiFirst != NULL || !cActiveInList);

    if (cActiveInList == 0) {
        ORPATH(0x10);
        if (pcoi->state & ST_CLIENT) {
            ORPATH(0x20);
            if (((PCL_CONV_INFO)pcoi)->hConvList) {
                /*
                 * Remove pcoi's hwnd from its hConvList.
                 */
                pcl = (PCONVLIST)GetHandleData((HANDLE)((PCL_CONV_INFO)pcoi)->hConvList);
                for (i = 0; i < pcl->chwnd; i++) {
                    if (pcl->ahwnd[i] == pcoi->hwndConv) {
                        ORPATH(0x40);
                        pcl->chwnd--;
                        UserAssert(pcl->ahwnd[pcl->chwnd]);
                        pcl->ahwnd[i] = pcl->ahwnd[pcl->chwnd];
                        ValidateConvList(((PCL_CONV_INFO)pcoi)->hConvList);
                        break;
                    }
                }
                ORPATH(0x80);
            }
        } else {  // SERVER
            /*
             * remove server window from the service/topic lookup table.
             */
            ORPATH(0x100);
            for (i = 0; i < pcoi->pcii->cServerLookupAlloc; i++) {
                if (pcoi->pcii->aServerLookup[i].hwndServer == pcoi->hwndConv) {
                    ORPATH(0x200);
                    
                    /*
                     * Check for appcompat hack
                     */
                    if (GetAppCompatFlags2(VERMAX) & GACF2_DDE) {
                        DeleteAtom(pcoi->pcii->aServerLookup[i].laService); // delete laService
                        DeleteAtom(pcoi->pcii->aServerLookup[i].laTopic);   // delete laTopic
                    }
                    
                    if (--(pcoi->pcii->cServerLookupAlloc)) {
                        ORPATH(0x400);
                        pcoi->pcii->aServerLookup[i] =
                                pcoi->pcii->aServerLookup[pcoi->pcii->cServerLookupAlloc];
                    } else {
                        DDEMLFree(pcoi->pcii->aServerLookup);
                        pcoi->pcii->aServerLookup = NULL;
                    }
                    break;
                }
            }
        }
    }
#ifdef TESTING
      else {
        /*
         * make sure at this point we have at least one non-zombie
         */
        pcoiNow = pcoiFirst;
        while (pcoiNow != NULL) {
            if (TypeFromHandle(pcoiNow->hConv) != HTYPE_ZOMBIE_CONVERSATION) {
                goto Out;
            }
            pcoiNow = pcoiNow->next;
        }
        DebugBreak();
Out:
        ;
    }
#endif // TESTING

    ValidateAllConvLists();
    ORPATH(0x800);

    /*
     * In any case remove hConvList references from client conversation.
     */
    if (pcoi->state & ST_CLIENT) {
#ifdef TESTING
        /*
         * Verify that the hConvList that is being removed, doesn't reference
         * this window.
         */
        if (((PCL_CONV_INFO)pcoi)->hConvList && !cActiveInList) {
            BOOL fFound = FALSE;

            pcl = (PCONVLIST)GetHandleData((HANDLE)((PCL_CONV_INFO)pcoi)->hConvList);
            for (i = 0; i < pcl->chwnd; i++) {
                if (pcl->ahwnd[i] == pcoi->hwndConv) {
                    fFound = TRUE;
                    break;
                }
            }
            UserAssert(!fFound);
        }
#endif // TESTING
        ((PCL_CONV_INFO)pcoi)->hConvList = 0;
        pcoi->state &= ~ST_INLIST;
    }

    /*
     * last one out turns out the lights.
     */
    if (pcoiFirst == NULL) {
        /*
         * If the pcoi list is empty, this window can go away.
         */
        LeaveDDECrit;
        NtUserDestroyWindow(pcoi->hwndConv);
        EnterDDECrit;
    }
}





/***************************************************************************\
* FreeConversationResources
*
* Description:
* Used when: Client window is disconnected by app, Server window is
* disconnected by either side, or when a conversation is disconnected
* at Uninitialize time.
*
* This function releases all resources held by the pcoi and unlinks it
* from its host window pcoi chian. pcoi is freed once this return s.
*
* History:
* 12-21-91 sanfords Created.
\***************************************************************************/
VOID FreeConversationResources(
    PCONV_INFO pcoi)
{
    PADVISE_LINK paLink;
    PDDE_MESSAGE_QUEUE pdmq;
    PXACT_INFO pxi;

    CheckDDECritIn;

    /*
     * Don't free resources on locked conversations.
     */
    if (pcoi->cLocks > 0) {
        pcoi->state |= ST_FREE_CONV_RES_NOW;
        return;
    }

    /*
     * Don't free resources if a synchronous transaction is in effect!
     */
    pxi = pcoi->pxiOut;
    while (pxi != NULL) {
        if (pxi->flags & XIF_SYNCHRONOUS) {
            /*
             * This conversation is in a synchronous transaction.
             * Shutdown the modal loop FIRST, then call this when
             * the loop exits.
             */
            PostMessage(pcoi->hwndConv, WM_TIMER, TID_TIMEOUT, 0);
            pcoi->state |= ST_FREE_CONV_RES_NOW;
            return;
        }
        pxi = pxi->next;
    }

    /*
     * If this is an Intra-Process conversation that hasn't yet received
     * a terminate message, make it a zombie.  We will call this routine
     * again once the terminate arrives or when WaitForZombieTerminate() has
     * timed out waiting.
     */
    if (pcoi->state & ST_INTRA_PROCESS && !(pcoi->state & ST_TERMINATE_RECEIVED)) {
        DestroyHandle((HANDLE)pcoi->hConv);
        pcoi->hConv = (HCONV)CreateHandle((ULONG_PTR)pcoi, HTYPE_ZOMBIE_CONVERSATION,
                InstFromHandle(pcoi->hConv));
        UnlinkConvFromOthers(pcoi, TRUE);
        return;
    }

    /*
     * remove any transactions left in progress
     */
    while (pcoi->pxiOut != NULL) {
        (pcoi->pxiOut->pfnResponse)(pcoi->pxiOut, 0, 0);
    }

    /*
     * Throw away any incoming queued DDE messages.
     */
    while (pcoi->dmqOut != NULL) {

        pdmq = pcoi->dmqOut;
        DumpDDEMessage(!(pcoi->state & ST_INTRA_PROCESS), pdmq->msg, pdmq->lParam);
        pcoi->dmqOut = pcoi->dmqOut->next;
        if (pcoi->dmqOut == NULL) {
            pcoi->dmqIn = NULL;
        }
        DDEMLFree(pdmq);
    }

    //
    // Remove all link info
    //
    paLink = pcoi->aLinks;
    while (pcoi->cLinks) {
        if (pcoi->state & ST_CLIENT) {
            MONLINK(pcoi->pcii, FALSE, paLink->wType & XTYPF_NODATA,
                    pcoi->laService, pcoi->laTopic,
                    LocalToGlobalAtom(paLink->laItem),
                    paLink->wFmt, FALSE,
                    (HCONV)pcoi->hwndPartner, (HCONV)pcoi->hwndConv);
        } else {
            MONLINK(pcoi->pcii, FALSE, paLink->wType & XTYPF_NODATA,
                    pcoi->laService, pcoi->laTopic,
                    LocalToGlobalAtom(paLink->laItem),
                    paLink->wFmt, TRUE,
                    (HCONV)pcoi->hwndConv, (HCONV)pcoi->hwndPartner);
        }
        if (!(pcoi->state & ST_CLIENT)) {
            DeleteLinkCount(pcoi->pcii, paLink->pLinkCount);
        }
        DeleteAtom(paLink->laItem); // link structure copy
        paLink++;
        pcoi->cLinks--;
    }
    if (pcoi->aLinks) {
        DDEMLFree(pcoi->aLinks);
    }

    //
    // free atoms associated with this conv
    //
    DeleteAtom(pcoi->laService);
    DeleteAtom(pcoi->laTopic);
    if (pcoi->laServiceRequested) {
        DeleteAtom(pcoi->laServiceRequested);
    }

    UnlinkConvFromOthers(pcoi, FALSE);

    /*
     * invalidate app's conversation handle
     */
    DestroyHandle((HANDLE)pcoi->hConv);

    DDEMLFree(pcoi);
}



BOOL WaitForZombieTerminate(
HANDLE hData)
{
    PCONV_INFO pcoi;
    MSG msg;
    HWND hwnd;
    BOOL fTerminated;
    DWORD fRet = 0;

    CheckDDECritOut;
    EnterDDECrit;

    fTerminated = FALSE;
    while ((pcoi = (PCONV_INFO)ValidateCHandle(hData,
            HTYPE_ZOMBIE_CONVERSATION, InstFromHandle(hData))) != NULL &&
            !(pcoi->state & ST_TERMINATE_RECEIVED)) {
        hwnd = pcoi->hwndConv;
        LeaveDDECrit;
        while (PeekMessage(&msg, hwnd, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
            DispatchMessage(&msg);
            if (msg.message == WM_DDE_TERMINATE) {
                fTerminated = TRUE;
            }
        }
        if (!fTerminated) {
            fRet = MsgWaitForMultipleObjectsEx(0, NULL, 100, QS_POSTMESSAGE, 0);
            if (fRet == 0xFFFFFFFF) {
                RIPMSG0(RIP_WARNING, "WaitForZombieTerminate: I give up - faking terminate.");
                ProcessTerminateMsg(pcoi, pcoi->hwndPartner);
                EnterDDECrit;
                return(FALSE);
            }
        }
        EnterDDECrit;
    }
    LeaveDDECrit;
    return(TRUE);
}
