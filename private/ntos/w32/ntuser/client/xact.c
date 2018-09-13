/****************************** Module Header ******************************\
* Module Name: xact.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* DDE Manager transaction processing module
*
* Created: 11/3/91 Sanford Staab
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* DdeClientTransaction (DDEML API)
*
* Description:
* Initiates all DDE transactions.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
HDDEDATA DdeClientTransaction(
LPBYTE pData,
DWORD cbData,
HCONV hConv,
HSZ hszItem,
UINT wFmt,
UINT wType,
DWORD ulTimeout,
LPDWORD pulResult)
{
    MSG msg;
    PCL_INSTANCE_INFO pcii = NULL;
    HDDEDATA hRet = 0;
    PCL_CONV_INFO pci;
    PDDEMLDATA pdd = NULL;
    PXACT_INFO pxi;
    BOOL fStarted;
    PDDE_DATA pdde;

    EnterDDECrit;

    pci = (PCL_CONV_INFO)ValidateCHandle((HANDLE)hConv,
            HTYPE_CLIENT_CONVERSATION, HINST_ANY);
    if (pci == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    pcii = pci->ci.pcii;
    if (ulTimeout != TIMEOUT_ASYNC && GetClientInfo()->CI_flags & CI_IN_SYNC_TRANSACTION) {
        SetLastDDEMLError(pcii, DMLERR_REENTRANCY);
        goto Exit;
    }
    if (!(pci->ci.state & ST_CONNECTED)) {
        SetLastDDEMLError(pcii, DMLERR_NO_CONV_ESTABLISHED);
        goto Exit;
    }

    switch (wType) {
    case XTYP_POKE:
    case XTYP_ADVSTART:
    case XTYP_ADVSTART | XTYPF_NODATA:
    case XTYP_ADVSTART | XTYPF_ACKREQ:
    case XTYP_ADVSTART | XTYPF_NODATA | XTYPF_ACKREQ:
    case XTYP_REQUEST:
    case XTYP_ADVSTOP:
        if (hszItem == 0) {
            SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
            goto Exit;
        }
        break;

    case XTYP_EXECUTE: // just ignore wFmt & hszItem
        break;

    default:
        SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    pxi = DDEMLAlloc(sizeof(XACT_INFO));
    if (pxi == NULL) {
        SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
        goto Exit;
    }

    switch (wType) {
    case XTYP_EXECUTE:
    case XTYP_POKE:
        if ((LONG)cbData == -1L) {

            // We are accepting an existing data handle for export to another
            // app.

            pdd = (PDDEMLDATA)ValidateCHandle((HANDLE)pData,
                    HTYPE_DATA_HANDLE, HINST_ANY);
            if (pdd == NULL) {
InvParam:
                SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
                DDEMLFree(pxi);
                goto Exit;
            }

            // make sure data handle holds apropriate data for this transaction

            if ((pdd->flags & HDATA_EXECUTE && wType != XTYP_EXECUTE) ||
                    (!(pdd->flags & HDATA_EXECUTE) && wType == XTYP_EXECUTE)) {
                goto InvParam;
            }

            // To simplify life, use a copy if this handle is potentially
            // a relay or APPOWNED handle.

            if (pdd->flags & (HDATA_APPOWNED | HDATA_NOAPPFREE)) {
                pxi->hDDESent = CopyDDEData(pdd->hDDE, wType == XTYP_EXECUTE);
                if (!pxi->hDDESent) {
MemErr:
                    DDEMLFree(pxi);
                    SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
                    goto Exit;
                }
                USERGLOBALLOCK(pxi->hDDESent, pdde);
                if (pdde == NULL) {
                    FreeDDEData(pxi->hDDESent, TRUE, TRUE);
                    goto MemErr;
                }
                pdde->wStatus = DDE_FRELEASE;
                USERGLOBALUNLOCK(pxi->hDDESent);
            } else {
                pxi->hDDESent = pdd->hDDE;
            }

            // make sure handle has proper format

            if (wType == XTYP_POKE) {
                USERGLOBALLOCK(pxi->hDDESent, pdde);
                if (pdde == NULL) {
                    goto InvParam;
                }
                pdde->wFmt = (WORD)wFmt;
                USERGLOBALUNLOCK(pxi->hDDESent);
            }

        } else {  // Convert data in buffer into an apropriate hDDE

            if (wType == XTYP_POKE) {
                pxi->hDDESent = AllocAndSetDDEData(pData, cbData,
                        DDE_FRELEASE, (WORD)wFmt);
            } else {
                pxi->hDDESent = AllocAndSetDDEData(pData, cbData, 0, 0);
            }
            if (!pxi->hDDESent) {
                goto MemErr;
            }
        }
    }

    // FINALLY - start the transaction

    pxi->pcoi = (PCONV_INFO)pci;
    pxi->gaItem = LocalToGlobalAtom(LATOM_FROM_HSZ(hszItem)); // pxi copy
    pxi->wFmt = (WORD)wFmt;
    pxi->wType = (WORD)wType;

    switch (wType) {
    case XTYP_ADVSTART:
    case XTYP_ADVSTART | XTYPF_NODATA:
    case XTYP_ADVSTART | XTYPF_ACKREQ:
    case XTYP_ADVSTART | XTYPF_NODATA | XTYPF_ACKREQ:
        fStarted = ClStartAdvise(pxi);
        break;

    case XTYP_ADVSTOP:
        fStarted = ClStartUnadvise(pxi);
        break;

    case XTYP_EXECUTE:
        fStarted = ClStartExecute(pxi);
        break;

    case XTYP_POKE:
        fStarted = ClStartPoke(pxi);
        break;

    case XTYP_REQUEST:
        fStarted = ClStartRequest(pxi);
    }

    if (!fStarted) {
        // if we copied or allocated data - free it.
        if (pxi->hDDESent && (pdd == NULL || pxi->hDDESent != pdd->hDDE)) {
            FreeDDEData(pxi->hDDESent, FALSE, TRUE);     // free data copy
        }
        GlobalDeleteAtom(pxi->gaItem); // pxi copy
        DDEMLFree(pxi);
        goto Exit;
    }

    if (pdd != NULL && !(pdd->flags & (HDATA_NOAPPFREE | HDATA_APPOWNED))) {

        // invalidate given handle on success - unless we copied it because
        // the app will either be return ing it from a callback or potentially
        // using it again.

        DDEMLFree(pdd);
        DestroyHandle((HANDLE)pData);
    }

    if (ulTimeout == TIMEOUT_ASYNC) {

        // asynchronous transaction

        if (pulResult != NULL) {
            pxi->hXact = CreateHandle((ULONG_PTR)pxi, HTYPE_TRANSACTION,
                    InstFromHandle(pcii->hInstClient));
            *pulResult = HandleToUlong(pxi->hXact);
        }
        hRet = (HDDEDATA)TRUE;

    } else {

        // synchronous transaction

        GetClientInfo()->CI_flags |= CI_IN_SYNC_TRANSACTION;
        pcii->flags |= IIF_IN_SYNC_XACT;

        pxi->flags |= XIF_SYNCHRONOUS;
        NtUserSetTimer(pci->ci.hwndConv, TID_TIMEOUT, ulTimeout, NULL);

        LeaveDDECrit;
        CheckDDECritOut;

        GetMessage(&msg, (HWND)NULL, 0, 0);

        /*
         * stay in modal loop until a timeout happens.
         */
        while (msg.hwnd != pci->ci.hwndConv || msg.message != WM_TIMER ||
            (msg.wParam != TID_TIMEOUT)) {

            if (!CallMsgFilter(&msg, MSGF_DDEMGR))
                DispatchMessage(&msg);

            GetMessage(&msg, (HWND)NULL, 0, 0);
        }

        EnterDDECrit;

        NtUserKillTimer(pci->ci.hwndConv, TID_TIMEOUT);
        GetClientInfo()->CI_flags &= ~CI_IN_SYNC_TRANSACTION;
        pcii->flags &= ~IIF_IN_SYNC_XACT;

        if (pxi->flags & XIF_COMPLETE) {
            if (pulResult != NULL) {
                *pulResult = pxi->wStatus; // NACK status bits
            }
            switch (wType) {
            case XTYP_ADVSTART:
            case XTYP_ADVSTART | XTYPF_NODATA:
            case XTYP_ADVSTART | XTYPF_ACKREQ:
            case XTYP_ADVSTART | XTYPF_NODATA | XTYPF_ACKREQ:
            case XTYP_ADVSTOP:
            case XTYP_EXECUTE:
            case XTYP_POKE:
                hRet = (HDDEDATA)((pxi->wStatus & DDE_FACK) ? TRUE : FALSE);
                if (!hRet) {
                    if (pxi->wStatus & DDE_FBUSY) {
                        SetLastDDEMLError(pcii, DMLERR_BUSY);
                    } else {
                        SetLastDDEMLError(pcii, DMLERR_NOTPROCESSED);
                    }
                }
                break;

            case XTYP_REQUEST:
                if (pxi->hDDEResult == 0) {
                    hRet = (HDDEDATA)((pxi->wStatus & DDE_FACK) ? TRUE : FALSE);
                    if (!hRet) {
                        if (pxi->wStatus & DDE_FBUSY) {
                            SetLastDDEMLError(pcii, DMLERR_BUSY);
                        } else {
                            SetLastDDEMLError(pcii, DMLERR_NOTPROCESSED);
                        }
                    }
                    break;
                }
                // Note that if the incoming data didn't have the DDE_FRELEASE
                // bit set, the transaction code would have made a copy so
                // the app is free to keep is as long as he likes.

                hRet = InternalCreateDataHandle(pcii, (LPBYTE)pxi->hDDEResult, (DWORD)-1, 0,
                        HDATA_READONLY, 0, 0);
                pxi->hDDEResult = 0; // so cleanup doesn't free it.
            }

            (pxi->pfnResponse)((struct tagXACT_INFO *)pxi, 0, 0); // cleanup transaction

        } else {    // Timed out

            // abandon the transaction and make it asyncronous so it will
            // clean itself up when the response finally comes in.

            pxi->flags &= ~XIF_SYNCHRONOUS;
            pxi->flags |= XIF_ABANDONED;

            switch (wType) {
            case XTYP_ADVSTART:
            case XTYP_ADVSTART | XTYPF_NODATA:
            case XTYP_ADVSTART | XTYPF_ACKREQ:
            case XTYP_ADVSTART | XTYPF_NODATA | XTYPF_ACKREQ:
                SetLastDDEMLError(pcii, DMLERR_ADVACKTIMEOUT);
                break;
            case XTYP_ADVSTOP:
                SetLastDDEMLError(pcii, DMLERR_UNADVACKTIMEOUT);
                break;
            case XTYP_EXECUTE:
                SetLastDDEMLError(pcii, DMLERR_EXECACKTIMEOUT);
                break;
            case XTYP_POKE:
                SetLastDDEMLError(pcii, DMLERR_POKEACKTIMEOUT);
                break;
            case XTYP_REQUEST:
                SetLastDDEMLError(pcii, DMLERR_DATAACKTIMEOUT);
                break;
            }
            // cleanup of pxi happens when transaction actually completes.
        }
    }
    if (pci->ci.state & ST_FREE_CONV_RES_NOW) {
        /*
         * The conversation was terminated during the synchronous transaction
         * so we need to clean up now that we are out of the loop.
         */
         FreeConversationResources((PCONV_INFO)pci);
    }

Exit:
    /*
     * Because this API is capable of blocking DdeUninitialize(), we check
     * before exit to see if it needs to be called.
     */
    if (pcii != NULL &&
            (pcii->afCmd & APPCMD_UNINIT_ASAP) &&
            // !(pcii->flags & IIF_IN_SYNC_XACT) &&
            !pcii->cInDDEMLCallback) {
        DdeUninitialize(HandleToUlong(pcii->hInstClient));
        hRet = 0;
    }
    LeaveDDECrit;
    return (hRet);
}




/***************************************************************************\
* GetConvContext
*
* Description:
* Retrieves conversation context information from the DDEML client window
* given. pl points to a CONVCONTEXT structure.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
VOID GetConvContext(
HWND hwnd,
LONG *pl)
{
    int i;

    for (i = 0; i < sizeof(CONVCONTEXT); i += 4) {
        *pl++ = GetWindowLong(hwnd, GWL_CONVCONTEXT + i);
    }
}

/***************************************************************************\
* SetConvContext
*
* Description:
*
* History:
* 11-19-92 sanfords Created.
\***************************************************************************/
VOID SetConvContext(
HWND hwnd,
LONG *pl)
{
    int i;

    for (i = 0; i < sizeof(CONVCONTEXT); i += 4) {
        SetWindowLong(hwnd, GWL_CONVCONTEXT + i, *pl++);
    }
}




/***************************************************************************\
* DdeQueryConvInfo (DDEML API)
*
* Description:
* Retrieves detailed conversation information on a per conversation/
* transaction basis.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
UINT DdeQueryConvInfo(
HCONV hConv,
DWORD idTransaction,
PCONVINFO pConvInfo)
{
    PCONV_INFO pcoi;
    PXACT_INFO pxi;
    CONVINFO ci;
    UINT uiRet = 0;

    EnterDDECrit;

    if (!ValidateTransaction(hConv, (HANDLE)LongToHandle( idTransaction ), &pcoi, &pxi)) {
        goto Exit;
    }

    try {
        if (pConvInfo->cb > sizeof(CONVINFO)) {
            SetLastDDEMLError(pcoi->pcii, DMLERR_INVALIDPARAMETER);
            goto Exit;
        }
        ci.cb = pConvInfo->cb;
        ci.hConvPartner = 0; // no longer supported.
        ci.hszSvcPartner = NORMAL_HSZ_FROM_LATOM(pcoi->laService);
        ci.hszServiceReq = NORMAL_HSZ_FROM_LATOM(pcoi->laServiceRequested);
        ci.hszTopic = NORMAL_HSZ_FROM_LATOM(pcoi->laTopic);
        ci.wStatus = pcoi->state;
        ci.wLastError = (WORD)pcoi->pcii->LastError;
        if (pcoi->state & ST_CLIENT) {
            ci.hConvList = ((PCL_CONV_INFO)pcoi)->hConvList;
            GetConvContext(pcoi->hwndConv, (LONG *)&ci.ConvCtxt);
        } else {
            ci.hConvList = 0;
            if (pcoi->state & ST_ISLOCAL) {
                GetConvContext(pcoi->hwndPartner, (LONG *)&ci.ConvCtxt);
            } else {
                ci.ConvCtxt = DefConvContext;
            }
        }
        if (pxi == NULL) {
            ci.hUser = pcoi->hUser;
            ci.hszItem = 0;
            ci.wFmt = 0;
            ci.wType = 0;
            ci.wConvst = XST_CONNECTED;
        } else {
            ci.hUser = pxi->hUser;
            // BUG - not fixable - This will result in extra local atoms
            // since we can never know when he is done with them.
            ci.hszItem = NORMAL_HSZ_FROM_LATOM(GlobalToLocalAtom(pxi->gaItem));
            ci.wFmt = pxi->wFmt;
            ci.wType = pxi->wType;
            ci.wConvst = pxi->state;
        }
        ci.hwnd = pcoi->hwndConv;
        ci.hwndPartner = pcoi->hwndPartner;
        RtlCopyMemory((LPSTR)pConvInfo, (LPSTR)&ci, pConvInfo->cb);
    } except(W32ExceptionHandler(FALSE, RIP_WARNING)) {
        SetLastDDEMLError(pcoi->pcii, DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    uiRet = TRUE;

Exit:
    LeaveDDECrit;
    return (uiRet);
}


/***************************************************************************\
* DdeSetUserHandle (DDEML API)
*
* Description:
* Sets a user DWORD on a per conversation/transaction basis.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL DdeSetUserHandle(
HCONV hConv,
DWORD id,
DWORD_PTR hUser)
{
    PCONV_INFO pcoi;
    PXACT_INFO pxi;
    BOOL fRet = FALSE;

    EnterDDECrit;

    if (!ValidateTransaction(hConv, (HANDLE)LongToHandle( id ), &pcoi, &pxi)) {
        goto Exit;
    }
    if (pxi == NULL) {
        pcoi->hUser = hUser;
    } else {
        pxi->hUser = hUser;
    }
    fRet = TRUE;

Exit:
    LeaveDDECrit;
    return (fRet);
}



VOID AbandonTransaction(
PCONV_INFO pcoi,
PXACT_INFO pxi)
{
    if (pxi != NULL) {
        pxi->flags |= XIF_ABANDONED;
    } else {
        for (pxi = pcoi->pxiIn; pxi != NULL; pxi = pxi->next) {
            pxi->flags |= XIF_ABANDONED;
        }
    }
}



BOOL AbandonEnumerateProc(
HWND hwnd,
LPARAM idTransaction)
{
    PCONV_INFO pcoi;

    pcoi = (PCONV_INFO)GetWindowLongPtr(hwnd, GWLP_PCI);
    if (!pcoi || !(pcoi->state & ST_CLIENT)) {
        return(TRUE);
    }
    while (pcoi) {
        AbandonTransaction(pcoi, (PXACT_INFO)idTransaction);
        pcoi = pcoi->next;
    }
    return(TRUE);
}



/***************************************************************************\
* DdeAbandonTransaction (DDEML API)
*
* Description:
* Cancels application interest in completing an asynchronous transaction.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL DdeAbandonTransaction(
DWORD idInst,
HCONV hConv,
DWORD idTransaction)
{
    PCONV_INFO pcoi;
    PXACT_INFO pxi;
    PCL_INSTANCE_INFO pcii;
    BOOL fRet = FALSE;

    EnterDDECrit;

    pcii = ValidateInstance((HANDLE)LongToHandle( idInst ));

    if (hConv == 0 && idTransaction == 0) {
        EnumChildWindows(pcii->hwndMother, AbandonEnumerateProc, 0);
        goto Exit;
    }
    if (idTransaction == 0) {
        idTransaction = QID_SYNC;
    }
    if (!ValidateTransaction(hConv, (HANDLE)LongToHandle( idTransaction ), &pcoi, &pxi)) {
        goto Exit;
    }
    if (pcii == NULL || pcoi->pcii != pcii) {
        SetLastDDEMLError(pcoi->pcii, DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    AbandonTransaction(pcoi, pxi);
    fRet = TRUE;

Exit:
    LeaveDDECrit;
    return (fRet);
}




/***************************************************************************\
* UpdateLinkIfChanged
*
* Description:
*   Helper function for updating a link
*
* Returns: TRUE if pxi was used - ie fMustReallocPxi
*
* History:
* 3-11-92   sanfords    Created.
* 8-24-92   sanfords    added cLinksToGo
\***************************************************************************/
BOOL UpdateLinkIfChanged(
PADVISE_LINK paLink,
PXACT_INFO pxi,
PCONV_INFO pcoi,
PADVISE_LINK paLinkLast,
PBOOL pfSwapped,
DWORD cLinksToGo)
{
    ADVISE_LINK aLinkT;

    CheckDDECritIn;

    *pfSwapped = FALSE;
    if (paLink->state & ADVST_CHANGED && !(paLink->state & ADVST_WAITING)) {
        pxi->pfnResponse = SvRespAdviseDataAck;
        pxi->pcoi = pcoi;
        pxi->gaItem = LocalToGlobalAtom(paLink->laItem);    // pxi copy
        pxi->wFmt = paLink->wFmt;
        pxi->wType = paLink->wType;
        paLink->state &= ~ADVST_CHANGED;
        if (SvStartAdviseUpdate(pxi, cLinksToGo)) {
            if (pxi->wType & DDE_FACKREQ) {
                paLink->state |= ADVST_WAITING;
                /*
                 * swap paLink with the last non-moved link to make ack search find
                 * oldest updated format.
                 */
                if (paLink != paLinkLast) {
                    aLinkT = *paLink;
                    RtlMoveMemory(paLink, paLink + 1,
                            (PBYTE)paLinkLast - (PBYTE)paLink);
                    *paLinkLast = aLinkT;
                    *pfSwapped = TRUE;
                }
            }
            return(TRUE);
        } else {
            GlobalDeleteAtom(pxi->gaItem);  // pxi copy
            return(FALSE);
        }
    }
    return(FALSE);
}


/***************************************************************************\
* DdePostAdvise     (DDEML API)
*
* Description:
* Updates outstanding server advise links as needed.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL DdePostAdvise(
DWORD idInst,
HSZ hszTopic,
HSZ hszItem)
{
    PCL_INSTANCE_INFO pcii;
    PSVR_CONV_INFO psi;
    PXACT_INFO pxi;
    PADVISE_LINK paLink;
    BOOL fRet = FALSE, fSwapped, fFound;
    int iServer, iLink;
    PLINK_COUNT pLinkCount;
#if DBG
    int cLinks;
#endif

    EnterDDECrit;

    pcii = ValidateInstance((HANDLE)LongToHandle( idInst ));
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    if ((ValidateHSZ(hszTopic) == HSZT_INVALID) ||
            (ValidateHSZ(hszItem) == HSZT_INVALID)) {
        SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    /*
     * Initialize all link counters and check if any links qualify
     */
    fFound = FALSE;
    for (pLinkCount = pcii->pLinkCount;
            pLinkCount; pLinkCount = pLinkCount->next) {
        pLinkCount->Count = pLinkCount->Total;
        fFound |= pLinkCount->laTopic == LATOM_FROM_HSZ(hszTopic) &&
                  pLinkCount->laItem == LATOM_FROM_HSZ(hszItem);
    }
    if (!fFound && hszTopic && hszItem) {
        fRet = TRUE;
        goto Exit;
    }

    /*
     * preallocate incase we are low on memory.
     */
    pxi = DDEMLAlloc(sizeof(XACT_INFO));
    if (pxi == NULL) {
        SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
        fRet = FALSE;
        goto Exit;
    }

    /*
     * For each server window on the specified topic
     */
    for (iServer = 0; iServer < pcii->cServerLookupAlloc; iServer++) {
        if (hszTopic == 0 ||
                pcii->aServerLookup[iServer].laTopic == LATOM_FROM_HSZ(hszTopic)) {

            /*
             * For each conversation within that window
             */
            psi = (PSVR_CONV_INFO)GetWindowLongPtr(
                    pcii->aServerLookup[iServer].hwndServer, GWLP_PSI);
            UserAssert(psi != NULL && psi->ci.pcii == pcii);    // sanity check
            while (psi != NULL) {


                /*
                 * UpdateLinkIfChanged might leave the critical section so lock this conversation
                 */
                psi->ci.cLocks++;

                #if DBG
                /*
                 * Rememeber the number of links so we can assert if they change during the loop below
                 */
                cLinks = psi->ci.cLinks;
                #endif
                /*
                 * For each active link on the given item...
                 */
                for (paLink = psi->ci.aLinks, iLink = 0;
                        iLink < psi->ci.cLinks; paLink++, iLink++) {
                    if (hszItem == 0 ||
                            paLink->laItem == LATOM_FROM_HSZ(hszItem)) {

// Bit of a hack here. For FACKREQ links, we don't want the server to
// outrun the client so we set the ADVST_WAITING bit till the ack is
// received. When the ack comes in, the protocol code has to search
// the aLinks array again to locate the apropriate link state flags and
// clear the ADVST_WAITING flag. At that time, if the ADVST_CHANGED flag
// is set, it is cleared and another SvStartAdviseUpdate transaction
// is started to get the link up to date.  To complicate matters,
// the ACK contains no format information.  Thus we need to move
// the Link info to the end of the list so that the right format
// is updated when the ack comes in.

                        paLink->state |= ADVST_CHANGED;
                        if (UpdateLinkIfChanged(paLink, pxi, &psi->ci,
                                &psi->ci.aLinks[psi->ci.cLinks - 1],
                                &fSwapped, --paLink->pLinkCount->Count)) {
                            if (fSwapped) {
                                paLink--;
                            }
                            /*
                             * preallocate for next advise
                             */
                            pxi = DDEMLAlloc(sizeof(XACT_INFO));
                            if (pxi == NULL) {
                                SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
                                /*
                                 * Unlock the conversation
                                 */
                                psi->ci.cLocks--;
                                if ((psi->ci.cLocks == 0) && (psi->ci.state & ST_FREE_CONV_RES_NOW)) {
                                    RIPMSG1(RIP_ERROR, "DdePostAdvise: Conversation terminated. psi:%#p", psi);
                                    FreeConversationResources((PCONV_INFO)psi);
                                }
                                goto Exit;
                            }
                        }
                        /*
                         * We might have left the crit sect...
                         */
                        UserAssert(pcii == ValidateInstance((HANDLE)LongToHandle( idInst )));
                    }
                }
                #if DBG
                if (cLinks != psi->ci.cLinks) {
                    RIPMSG1(RIP_ERROR, "DdePostAdvise: cLinks changed. psi:%#p", psi);
                }
                #endif

                /*
                 * If the converstaion got nuked, stop working on this conversation chain.
                 */
                psi->ci.cLocks--;
                if ((psi->ci.cLocks == 0) && (psi->ci.state & ST_FREE_CONV_RES_NOW)) {
                    RIPMSG1(RIP_ERROR, "DdePostAdvise: Conversation terminated. psi:%#p", psi);
                    FreeConversationResources((PCONV_INFO)psi);
                    break;
                }

                psi = (PSVR_CONV_INFO)psi->ci.next;     // next conversation
            }
        }
    }
    DDEMLFree(pxi);
    fRet = TRUE;

Exit:
    /*
     * Because callbacks are capable of blocking DdeUninitialize(), we check
     * before exit to see if it needs to be called.
     */
    UserAssert(pcii == ValidateInstance((HANDLE)LongToHandle( idInst )));
    if (pcii != NULL &&
            pcii->afCmd & APPCMD_UNINIT_ASAP &&
            !(pcii->flags & IIF_IN_SYNC_XACT) &&
            !pcii->cInDDEMLCallback) {
        DdeUninitialize(HandleToUlong(pcii->hInstClient));
        fRet = TRUE;
    }
    LeaveDDECrit;
    return (fRet);
}


/***************************************************************************\
* LinkTransaction
*
* Description:
* Adds a transaction structure to the associated conversation's transaction
* queue.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
VOID LinkTransaction(
PXACT_INFO pxi)
{
    CheckDDECritIn;

    pxi->next = NULL;
    if (pxi->pcoi->pxiOut == NULL) {
        pxi->pcoi->pxiIn = pxi->pcoi->pxiOut = pxi;
    } else {
        pxi->pcoi->pxiIn->next = pxi;
        pxi->pcoi->pxiIn = pxi;
    }
#if DBG
    /*
     * Temporary check to find stress bug - make sure pxi list is not
     * looped on itself.  If it is, this loop will never exit and things
     * will get investigated. (sanfords)
     */
    {
        PXACT_INFO pxiT;

        for (pxiT = pxi->pcoi->pxiOut; pxiT != NULL; pxiT = pxiT->next) {
            ;
        }
    }
#endif // DBG
}


/***************************************************************************\
* UnlinkTransaction
*
* Description:
* Removes a transaction structure from the associated conversation's transaction
* queue.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
VOID UnlinkTransaction(
PXACT_INFO pxi)
{
    CheckDDECritIn;
    if (pxi == pxi->pcoi->pxiOut) {
        pxi->pcoi->pxiOut = pxi->next;
        if (pxi->next == NULL) {
            pxi->pcoi->pxiIn = NULL;
        }
    }
}


/***************************************************************************\
* ValidateTransaction
*
* Description:
* Common validation code for DDEML APIs that take a conversation handle
* and a transaction ID. *ppxi may be null on return if hXact was 0.
* Returns fSuccess.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL ValidateTransaction(
HCONV hConv,
HANDLE hXact,
PCONV_INFO *ppcoi,
PXACT_INFO *ppxi)
{
    PCL_INSTANCE_INFO pcii;

    *ppcoi = (PCONV_INFO)ValidateCHandle((HANDLE)hConv,
            HTYPE_CLIENT_CONVERSATION, HINST_ANY);
    if (*ppcoi == NULL) {
        *ppcoi = (PCONV_INFO)ValidateCHandle((HANDLE)hConv,
                HTYPE_SERVER_CONVERSATION, HINST_ANY);
    }
    if (*ppcoi == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        return (FALSE);
    }
    pcii = ValidateInstance((*ppcoi)->pcii->hInstClient);
    if (pcii != (*ppcoi)->pcii) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        return (FALSE);
    }

    if (hXact == (HANDLE)IntToPtr( QID_SYNC )) {
        *ppxi = NULL;
    } else {
        *ppxi = (PXACT_INFO)ValidateCHandle(hXact, HTYPE_TRANSACTION,
                InstFromHandle((*ppcoi)->pcii->hInstClient));
        if (*ppxi == NULL) {
            SetLastDDEMLError((*ppcoi)->pcii, DMLERR_INVALIDPARAMETER);
            return (FALSE);
        }
    }
    return (TRUE);
}
