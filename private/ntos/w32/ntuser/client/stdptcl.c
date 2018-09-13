/****************************** Module Header ******************************\
* Module Name: stdptcl.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* DDE Manager DDE protocol transaction management functions
*
* NITTY GRITTY GUCK of DDE
*
* Created: 11/3/91 Sanford Staab
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/*

    StartFunctions:
        These are used to fill in a preallocated pxi with transaction
        specific data. They then start the desired transaction and
        link the pxi into the conversation's transaction queue.
        fSuccess is return ed. On error, SetLastDDEMLError is called
        by these functions and the pxi is untouched, ready for
        reuse on a subsequent call. Note that the pxi->gaItem field
        is a global atom and needs to be deleted by the caller on
        failure as apropriate. Success implies the transaction
        is started successfully.

    RespFunctions:
        These are called via the pxi->pfnRespnose field in response
        to expected DDE messages. If the msg parameter is 0, these
        functions assume transaction cleanup is being done. FALSE
        is only return ed if CBR_BLOCK was returned from a callback.

    SpontFunctions:
        These are called in response to a spontaneous (unexpected) DDE
        message. These functions may create a pxi and link it into the
        conversation's transaction queue to properly handle expected
        replies. FALSE is only return ed if CBR_BLOCK was returned
        from a callback.

    The prefixes Sv and Cl indicate which side of the DDE conversation
    is doing the work.

    Weaknesses: Can't deal well with failed PostMessage() or
                 lParam acessing/allocation failures. Hoping these
                 are rare enough (ie never) to not matter. If they
                 do fail, the tracking layer will eventually shut down
                 the conversation.

*/

//--------------------------------ADVISE-------------------------------//

/***************************************************************************\
* ClStartAdvise
*
* Description:
* CLIENT side Advise link processing
* Post WM_DDE_ADVISE message
* Link pxi for responding WM_DDE_ACK message.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL ClStartAdvise(
PXACT_INFO pxi)
{
    DWORD dwError;

    //
    // protocol quirk: DDE_FRELEASE is always assumed set in a WM_DDE_ADVISE
    // message. We set it here just in case the dork on the other end
    // pays attention to it.
    //
    pxi->hDDESent = AllocAndSetDDEData(NULL, sizeof(DDE_DATA),
            (WORD)(((pxi->wType << 12) & (DDE_FDEFERUPD | DDE_FACKREQ)) | DDE_FRELEASE),
            pxi->wFmt);
    if (!pxi->hDDESent) {
        SetLastDDEMLError(pxi->pcoi->pcii, DMLERR_MEMORY_ERROR);
        return (FALSE);
    }

    IncGlobalAtomCount(pxi->gaItem); // message copy
    dwError = PackAndPostMessage(pxi->pcoi->hwndPartner, 0, WM_DDE_ADVISE,
            pxi->pcoi->hwndConv, 0, HandleToUlong(pxi->hDDESent), pxi->gaItem);
    if (dwError) {
        SetLastDDEMLError(pxi->pcoi->pcii, dwError);
        WOWGLOBALFREE(pxi->hDDESent);
        pxi->hDDESent = 0;
        GlobalDeleteAtom(pxi->gaItem); // message copy
        return (FALSE);
    }

    pxi->state = XST_ADVSENT;
    pxi->pfnResponse = (FNRESPONSE)ClRespAdviseAck;
    LinkTransaction(pxi);
    return (TRUE);
}


/***************************************************************************\
* SvSpontAdvise
*
* Description:
* SERVER side WM_DDE_ADVISE processing
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL SvSpontAdvise(
PSVR_CONV_INFO psi,
LPARAM lParam)
{
    UINT_PTR uiHi;
    HANDLE hDDE;
    WORD wFmt, wStatus;
    ULONG_PTR dwRet = 0;
    DWORD dwError;
    LATOM la;

    UnpackDDElParam(WM_DDE_ADVISE, lParam, (PUINT_PTR)&hDDE, &uiHi);
    if (psi->ci.pcii->afCmd & CBF_FAIL_ADVISES) {
        goto Ack;
    }

    if (!ExtractDDEDataInfo(hDDE, &wStatus, &wFmt)) {
        goto Ack;
    }

    if (wStatus & DDE_FDEFERUPD) {
        wStatus &= ~DDE_FACKREQ;   // warm links shouldn't have this flag set
    }

    la = GlobalToLocalAtom((GATOM)uiHi);
    dwRet = (ULONG_PTR)DoCallback(psi->ci.pcii,
        XTYP_ADVSTART,
        wFmt, psi->ci.hConv,
        NORMAL_HSZ_FROM_LATOM(psi->ci.laTopic),
        NORMAL_HSZ_FROM_LATOM(la),
        (HDDEDATA)0, 0, 0);
    DeleteAtom(la);

    // check CBR_BLOCK case

    if (dwRet == (ULONG_PTR)CBR_BLOCK) {
        return (FALSE);
    }

    if (dwRet) {
        //
        // If we fail to add the link internally, dwRet == 0 -> NACK
        //
        dwRet = AddLink((PCONV_INFO)psi, (GATOM)uiHi, wFmt,
                (WORD)(wStatus & (WORD)(DDE_FDEFERUPD | DDE_FACKREQ)));
        if (dwRet) {
            MONLINK(psi->ci.pcii, TRUE, wStatus & DDE_FDEFERUPD, psi->ci.laService,
                    psi->ci.laTopic, (GATOM)uiHi, wFmt, TRUE,
                    (HCONV)psi->ci.hwndConv, (HCONV)psi->ci.hwndPartner);
        }
    }

Ack:
    if (dwRet) {
        WOWGLOBALFREE(hDDE); // hOptions - NACK -> HE frees it.
    }
    // IncGlobalAtomCount((GATOM)uiHi);         // message copy - reuse
    dwError = PackAndPostMessage(psi->ci.hwndPartner, WM_DDE_ADVISE, WM_DDE_ACK,
            psi->ci.hwndConv, lParam, dwRet ? DDE_FACK : 0, uiHi);
    if (dwError) {
        SetLastDDEMLError(psi->ci.pcii, dwError);
        GlobalDeleteAtom((ATOM)uiHi); // message copy
    }

    return (TRUE);
}



/***************************************************************************\
* ClRespAdviseAck
*
* Description:
* Client's response to an expected Advise Ack.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL ClRespAdviseAck(
PXACT_INFO pxi,
UINT msg,
LPARAM lParam)
{
    UINT_PTR uiLo, uiHi;

    if (msg) {
        if (msg != WM_DDE_ACK) {
            return (SpontaneousClientMessage((PCL_CONV_INFO)pxi->pcoi, msg, lParam));
        }

        UnpackDDElParam(WM_DDE_ACK, lParam, &uiLo, &uiHi);
#if DBG
        if ((GATOM)uiHi != pxi->gaItem) {
            return (SpontaneousClientMessage((PCL_CONV_INFO)pxi->pcoi, msg, lParam));
        }
#endif

        GlobalDeleteAtom((ATOM)uiHi); // message copy

        pxi->state = XST_ADVACKRCVD;
        pxi->wStatus = (WORD)uiLo;

        if (pxi->wStatus & DDE_FACK) {
            if (AddLink(pxi->pcoi, pxi->gaItem, pxi->wFmt,
                    (WORD)((pxi->wType << 12) & (DDE_FACKREQ | DDE_FDEFERUPD)))) {
                //
                // only server side reports links on local conversations.
                //
                if (!(pxi->pcoi->state & ST_ISLOCAL)) {
                    MONLINK(pxi->pcoi->pcii, TRUE, (WORD)uiLo & DDE_FDEFERUPD,
                            pxi->pcoi->laService, pxi->pcoi->laTopic, pxi->gaItem,
                            pxi->wFmt, FALSE, (HCONV)pxi->pcoi->hwndPartner,
                            (HCONV)pxi->pcoi->hwndConv);
                }
            } else {
                pxi->wStatus = 0;  // memory failure - fake a NACK.
            }
        } else {
            WOWGLOBALFREE(pxi->hDDESent);  // Nack free.
        }

        if (TransactionComplete(pxi,
                (pxi->wStatus & DDE_FACK) ? (HDDEDATA)1L : (HDDEDATA)0L)) {
            goto Cleanup;
        }
    } else {
Cleanup:
        GlobalDeleteAtom(pxi->gaItem); // pxi copy
        UnlinkTransaction(pxi);
        DDEMLFree(pxi);
    }
    if (msg) {
        FreeDDElParam(msg, lParam);
    }
    return (TRUE);
}

//-------------------------ADVISE LINK UPDATE--------------------------//


/***************************************************************************\
* SvStartAdviseUpdate
*
* Description:
* Starts a single link update transaction. The return value is TRUE only
* if pxi was queued.
*
* History:
* 11-19-91 sanfords Created.
* 8-24-92  sanfords Added cLinksToGo
\***************************************************************************/
BOOL SvStartAdviseUpdate(
PXACT_INFO pxi,
DWORD cLinksToGo)
{
    HDDEDATA hData = NULL;
    PDDE_DATA pdde;
    DWORD dwError;
    HANDLE hDDE;
    LATOM al;

    CheckDDECritIn;

    if (pxi->wType & DDE_FDEFERUPD) {
        hDDE = 0;
    } else {
        al = GlobalToLocalAtom(pxi->gaItem);
        hData = DoCallback(pxi->pcoi->pcii,
                           XTYP_ADVREQ,
                           pxi->wFmt,
                           pxi->pcoi->hConv,
                           NORMAL_HSZ_FROM_LATOM(pxi->pcoi->laTopic),
                           NORMAL_HSZ_FROM_LATOM(al),
                           (HDDEDATA)0,
                           MAKELONG(cLinksToGo, 0),
                           0);
        DeleteAtom(al);
        if (!hData) {
            // app doesn't honor the advise.
            return (FALSE); // reuse pxi
        }
        hDDE = UnpackAndFreeDDEMLDataHandle(hData, FALSE);
        if (!hDDE) {

            /*
             * failed - must be execute type data
             */
            InternalFreeDataHandle(hData, FALSE);
            SetLastDDEMLError(pxi->pcoi->pcii, DMLERR_DLL_USAGE);
            return (FALSE);
        }
        /*
         * Set fAckReq bit apropriately - note APPOWNED handles will already
         * have the fAckReq bit set so this will not change their state.
         */
        USERGLOBALLOCK(hDDE, pdde);
        if (pdde == NULL) {
            return (FALSE);
        }
        if (pdde->wFmt != pxi->wFmt) {

            /*
             * bogus data - wrong format!
             */
            USERGLOBALUNLOCK(hDDE);
            InternalFreeDataHandle(hData, FALSE);
            SetLastDDEMLError(pxi->pcoi->pcii, DMLERR_DLL_USAGE);
            return (FALSE);
        }
        if (!(pdde->wStatus & DDE_FRELEASE)) {
            pxi->wType |= DDE_FACKREQ; // dare not allow neither flag set!
        }
        pdde->wStatus |= (pxi->wType & DDE_FACKREQ);
        USERGLOBALUNLOCK(hDDE);
    }

    IncGlobalAtomCount(pxi->gaItem); // message copy
    dwError = PackAndPostMessage(pxi->pcoi->hwndPartner, 0, WM_DDE_DATA,
            pxi->pcoi->hwndConv, 0, HandleToUlong(hDDE), pxi->gaItem);
    if (dwError) {
        if (hData) {
            InternalFreeDataHandle(hData, FALSE);
        }
        SetLastDDEMLError(pxi->pcoi->pcii, dwError);
        GlobalDeleteAtom(pxi->gaItem); // message copy
        return (FALSE);
    }

    pxi->state = XST_ADVDATASENT;
    if (pxi->wType & DDE_FACKREQ) {
        pxi->hDDESent = hDDE;
        pxi->pfnResponse = (FNRESPONSE)SvRespAdviseDataAck;
        LinkTransaction(pxi);
        return (TRUE); // prevents reuse - since its queued.
    } else {
        return (FALSE); // causes pxi to be reused for next advdata message.
    }
}



/***************************************************************************\
* ClSpontAdviseData
*
* Description:
* Handles WM_DDE_DATA messages that are not request data.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL ClSpontAdviseData(
PCL_CONV_INFO pci,
LPARAM lParam)
{
    UINT_PTR uiHi;
    DWORD dwError;
    HANDLE hDDE = 0;
    HDDEDATA hData, hDataReturn;
    PDDE_DATA pdde;
    WORD wFmt;
    WORD wStatus;
    LATOM la;
    PADVISE_LINK paLink;
    int iLink;

    UnpackDDElParam(WM_DDE_DATA, lParam, (PUINT_PTR)&hDDE, &uiHi);
    UserAssert(!hDDE || GlobalSize(hDDE));
    wFmt = 0;
    wStatus = 0;
    hDataReturn = 0;
    la = GlobalToLocalAtom((GATOM)uiHi);
    if (hDDE) {
        USERGLOBALLOCK(hDDE, pdde);
        if (pdde == NULL) {
            hData = 0;
        } else {
            wFmt = pdde->wFmt;
            wStatus = pdde->wStatus;
            USERGLOBALUNLOCK(hDDE);

            /*
             * if data is coming in, create a data handle for the app
             */
            hData = InternalCreateDataHandle(pci->ci.pcii, (LPBYTE)hDDE,
                    (DWORD)-1, 0, HDATA_NOAPPFREE | HDATA_READONLY, 0, 0);
        }
        if (hData) {
            hDataReturn = DoCallback(pci->ci.pcii, XTYP_ADVDATA,
                    wFmt, pci->ci.hConv,
                    NORMAL_HSZ_FROM_LATOM(pci->ci.laTopic),
                    NORMAL_HSZ_FROM_LATOM(la),
                    hData, 0, 0);
            if (hDataReturn != CBR_BLOCK) {
                UnpackAndFreeDDEMLDataHandle(hData, FALSE);
                if (((ULONG_PTR)hDataReturn & DDE_FACK) || !(wStatus & DDE_FACKREQ)) {
                    /*
                     * Nacked Advise data with fAckReq set is server's
                     * responsibility to free!
                     */
                    FreeDDEData(hDDE, FALSE, TRUE);
                }
            }
        }
    } else {
        /*
         * WARM LINK CASE
         *
         * Search through the client's link info to find what formats this
         * puppy is on. We let the client know for each format being supported
         * on this item that is warm-linked. The last hDataReturn determines
         * the ACK returned - for lack of a better method.
         */
        for (paLink = pci->ci.aLinks, iLink = 0; iLink < pci->ci.cLinks; iLink++, paLink++) {
            if ((paLink->laItem == la) && (paLink->wType & DDE_FDEFERUPD)) {
                hDataReturn = DoCallback(pci->ci.pcii, XTYP_ADVDATA,
                        paLink->wFmt, pci->ci.hConv,
                        NORMAL_HSZ_FROM_LATOM(pci->ci.laTopic),
                        NORMAL_HSZ_FROM_LATOM(la),
                        0, 0, 0);
                if (hDataReturn == CBR_BLOCK) {
                    DeleteAtom(la);
                    return (FALSE);
                }
            }
        }
    }
    DeleteAtom(la);
    if (hDataReturn == CBR_BLOCK) {
        return (FALSE);
    }

    if (wStatus & DDE_FACKREQ) {

        (ULONG_PTR)hDataReturn &= ~DDE_FACKRESERVED;
        // reuse uiHi
        if (dwError = PackAndPostMessage(pci->ci.hwndPartner, WM_DDE_DATA,
                WM_DDE_ACK, pci->ci.hwndConv, lParam, (UINT_PTR)hDataReturn, uiHi)) {
            SetLastDDEMLError(pci->ci.pcii, dwError);
        }
    } else {
        GlobalDeleteAtom((ATOM)uiHi); // data message copy
        FreeDDElParam(WM_DDE_DATA, lParam); // not reused so free it.
    }
    return (TRUE);
}




/***************************************************************************\
* SvRespAdviseDataAck
*
* Description:
* Handles expected Advise Data ACK message.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL SvRespAdviseDataAck(
PXACT_INFO pxi,
UINT msg,
LPARAM lParam)
{
    UINT_PTR uiLo, uiHi;
    int iLink;
    PADVISE_LINK paLink;
    PXACT_INFO pxiNew;
    LATOM la;
    BOOL fSwapped;
#if DBG
    int cLinks;
#endif

    if (msg) {
        if (msg != WM_DDE_ACK) {
            return (SpontaneousServerMessage((PSVR_CONV_INFO)pxi->pcoi, msg, lParam));
        }
        UnpackDDElParam(WM_DDE_ACK, lParam, &uiLo, &uiHi);
        if ((GATOM)uiHi != pxi->gaItem) {
            RIPMSG0(RIP_ERROR, "DDE Protocol violation: Data ACK had wrong item");
            return (SpontaneousServerMessage((PSVR_CONV_INFO)pxi->pcoi, msg, lParam));
        }

        GlobalDeleteAtom((ATOM)uiHi); // message copy
        FreeDDElParam(WM_DDE_ACK, lParam);

        if (!((uiLo & DDE_FACK) && pxi->hDDESent)) {
            FreeDDEData(pxi->hDDESent, FALSE, TRUE);
        }

        #if DBG
        /*
         * Rememeber the number of links so we can assert if they change during the loop below
         */
        cLinks = pxi->pcoi->cLinks;
        #endif
        /*
         * locate link info and clear ADVST_WAITING bit
         */
        la = GlobalToLocalAtom((GATOM)uiHi);
        paLink = pxi->pcoi->aLinks;
        for (iLink = 0; iLink < pxi->pcoi->cLinks; iLink++, paLink++) {
            if (paLink->laItem == la &&
                    paLink->state & ADVST_WAITING) {
                paLink->state &= ~ADVST_WAITING;
                /*
                 * We have to allocate pxiNew because it may become linked
                 * into pcoi->pxiIn.
                 */
                pxiNew = (PXACT_INFO)DDEMLAlloc(sizeof(XACT_INFO));

                if (pxiNew && !UpdateLinkIfChanged(paLink, pxiNew, pxi->pcoi,
                        &pxi->pcoi->aLinks[pxi->pcoi->cLinks - 1], &fSwapped,
                        CADV_LATEACK)) {
                    /*
                     * Not used, free it.
                     */
                    DDEMLFree(pxiNew);
                }
                break;
            }
        }
        #if DBG
        if (cLinks != pxi->pcoi->cLinks) {
            RIPMSG1(RIP_ERROR, "SvRespAdviseDataAck: cLinks changed. pxi:%#p", pxi);
        }
        #endif

        DeleteAtom(la);
    }
    GlobalDeleteAtom(pxi->gaItem); // pxi copy
    UnlinkTransaction(pxi);
    DDEMLFree(pxi);
    return (TRUE);
}



//------------------------------UNADVISE-------------------------------//

/***************************************************************************\
* ClStartUnadvise
*
* Description:
* Starts a WM_DDE_UNADVISE transaction.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL ClStartUnadvise(
PXACT_INFO pxi)
{
    DWORD dwError;

    IncGlobalAtomCount(pxi->gaItem); // message copy
    dwError = PackAndPostMessage(pxi->pcoi->hwndPartner, 0, WM_DDE_UNADVISE,
            pxi->pcoi->hwndConv, 0, pxi->wFmt, pxi->gaItem);
    if (dwError) {
        SetLastDDEMLError(pxi->pcoi->pcii, dwError);
        GlobalDeleteAtom(pxi->gaItem); // message copy
        return (FALSE);
    }

    //
    // only server side reports links on local conversations.
    //
    if (!(pxi->pcoi->state & ST_ISLOCAL)) {
        MONLINK(pxi->pcoi->pcii, FALSE, 0,
                pxi->pcoi->laService, pxi->pcoi->laTopic, pxi->gaItem,
                pxi->wFmt, FALSE, (HCONV)pxi->pcoi->hwndPartner,
                (HCONV)pxi->pcoi->hwndConv);
    }
    pxi->state = XST_UNADVSENT;
    pxi->pfnResponse = (FNRESPONSE)ClRespUnadviseAck;
    LinkTransaction(pxi);
    return (TRUE);
}
/***************************************************************************\
* CloseTransaction
*
* Description:
* Remove all outstanding pxi coresponding to the transaction
* that will be closed in responds to a WM_DDE_UNADVISE message.
*
* History:
* 6-4-96 clupu Created.
\***************************************************************************/
void CloseTransaction(
    PCONV_INFO pci,
    ATOM       atom)
{
    PXACT_INFO pxi;
    PXACT_INFO pxiD;

    pxi = pci->pxiOut;

    while (pxi && (pxi->gaItem == atom)) {
        pxiD = pxi;
        pxi  = pxi->next;
        DDEMLFree(pxiD);
    }
    pci->pxiOut = pxi;

    if (pxi == NULL) {
        pci->pxiIn = NULL;
        return;
    }

    while (pxi->next) {
        if (pxi->next->gaItem == atom) {
            pxiD = pxi->next;
            pxi->next = pxiD->next;
            DDEMLFree(pxiD);
        } else
            pxi = pxi->next;
    }
    pci->pxiIn = pxi;
}

/***************************************************************************\
* SvSpontUnadvise
*
* Description:
* Responds to a WM_DDE_UNADVISE message.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL SvSpontUnadvise(
PSVR_CONV_INFO psi,
LPARAM lParam)
{
    ULONG_PTR dwRet = 0;
    DWORD dwError;
    INT iLink;
    PADVISE_LINK aLink;
    LATOM la;

    la = GlobalToLocalAtom((GATOM)HIWORD(lParam));

    CloseTransaction(&psi->ci, HIWORD(lParam));

    for (aLink = psi->ci.aLinks, iLink = 0; iLink < psi->ci.cLinks;) {

        if (la == 0 || aLink->laItem == la &&
                (LOWORD(lParam) == 0 || LOWORD(lParam) == aLink->wFmt)) {

            if (!(psi->ci.pcii->afCmd & CBF_FAIL_ADVISES)) {
                /*
                 * Only do the callbacks if he wants them.
                 */
                dwRet = (ULONG_PTR)DoCallback(psi->ci.pcii,
                    (WORD)XTYP_ADVSTOP, aLink->wFmt, psi->ci.hConv,
                    NORMAL_HSZ_FROM_LATOM(psi->ci.laTopic),
                    NORMAL_HSZ_FROM_LATOM(la),
                    (HDDEDATA)0, 0L, 0L);
                if (dwRet == (ULONG_PTR)CBR_BLOCK) {
                    DeleteAtom(la);
                    return(FALSE);
                }
            }
            /*
             * Notify any DDESPY apps.
             */
            MONLINK(psi->ci.pcii, TRUE, 0, psi->ci.laService,
                    psi->ci.laTopic, HIWORD(lParam), aLink->wFmt, TRUE,
                    (HCONV)psi->ci.hwndConv, (HCONV)psi->ci.hwndPartner);
            /*
             * Remove link info
             */
            DeleteAtom(aLink->laItem);  // aLink copy
            DeleteLinkCount(psi->ci.pcii, aLink->pLinkCount);
            if (--psi->ci.cLinks) {
                memmove((LPSTR)aLink, (LPSTR)(aLink + 1),
                        sizeof(ADVISE_LINK) * (psi->ci.cLinks - iLink));
            }
        } else {
            aLink++;
            iLink++;
        }
    }

    DeleteAtom(la);

    /*
     * Now ACK the unadvise message.
     */
    dwError = PackAndPostMessage(psi->ci.hwndPartner, 0,
            WM_DDE_ACK, psi->ci.hwndConv, 0, DDE_FACK, HIWORD(lParam));
    if (dwError) {
        SetLastDDEMLError(psi->ci.pcii, dwError);
        GlobalDeleteAtom((ATOM)HIWORD(lParam));      // message copy
        // FreeDDElParam(WM_DDE_UNADVISE, lParam);   // no unpack needed
    }

    return (TRUE);
}

/***************************************************************************\
* ClRespUnadviseAck
*
* Description:
* Client's response to an expected Unadvise Ack.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL ClRespUnadviseAck(
PXACT_INFO pxi,
UINT msg,
LPARAM lParam)
{
    UINT_PTR uiLo, uiHi;
    LATOM al;
    PADVISE_LINK aLink;
    int iLink;

    if (msg) {
        if (msg != WM_DDE_ACK) {
            return (SpontaneousClientMessage((PCL_CONV_INFO)pxi->pcoi, msg, lParam));
        }

        UnpackDDElParam(WM_DDE_ACK, lParam, &uiLo, &uiHi);
        if ((GATOM)uiHi != pxi->gaItem) {
            return (SpontaneousClientMessage((PCL_CONV_INFO)pxi->pcoi, msg, lParam));
        }

        al = GlobalToLocalAtom((ATOM)uiHi);
        for (aLink = pxi->pcoi->aLinks, iLink = 0;
                iLink < pxi->pcoi->cLinks;
                    ) {
            if (aLink->laItem == al &&
                    (pxi->wFmt == 0 || aLink->wFmt == pxi->wFmt)) {
                DeleteAtom(al);  // aLink copy
                if (--pxi->pcoi->cLinks) {
                    memmove((LPSTR)aLink, (LPSTR)(aLink + 1),
                            sizeof(ADVISE_LINK) * (pxi->pcoi->cLinks - iLink));
                }
            } else {
                aLink++;
                iLink++;
            }
        }
        DeleteAtom(al);  // local copy
        GlobalDeleteAtom((ATOM)uiHi);   // message copy

        pxi->state = XST_UNADVACKRCVD;
        pxi->wStatus = (WORD)uiLo;
        if (TransactionComplete(pxi, (HDDEDATA)1)) {
            goto Cleanup;
        }
    } else {
Cleanup:
        GlobalDeleteAtom(pxi->gaItem);   // pxi copy
        UnlinkTransaction(pxi);
        if (pxi->hXact) {
            DestroyHandle(pxi->hXact);
        }
        DDEMLFree(pxi);
    }
    if (msg) {
        FreeDDElParam(msg, lParam);
    }
    return (TRUE);
}


//-------------------------------EXECUTE-------------------------------//


/***************************************************************************\
* MaybeTranslateExecuteData
*
* Description:
* Translates DDE execute data if needed.
*
* History:
* 1/28/92 sanfords created
\***************************************************************************/
HANDLE MaybeTranslateExecuteData(
HANDLE hDDE,
BOOL fUnicodeFrom,
BOOL fUnicodeTo,
BOOL fFreeSource)
{
    PSTR pstr;
    PWSTR pwstr;
    DWORD cb;
    HANDLE hDDEnew;

    if (fUnicodeFrom && !fUnicodeTo) {
        // translate data from UNICODE to ANSII
        cb = (DWORD)(GlobalSize(hDDE) >> 1);
        hDDEnew = UserGlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, cb);
        USERGLOBALLOCK(hDDEnew, pstr);
        USERGLOBALLOCK(hDDE, pwstr);
        if (pstr != NULL && pwstr != NULL) {
            WCSToMB(pwstr, -1, &pstr, cb, FALSE);
        }
        if (pwstr) {
            USERGLOBALUNLOCK(hDDE);
        }
        if (pstr) {
            USERGLOBALUNLOCK(hDDEnew);
        }
    } else if (!fUnicodeFrom && fUnicodeTo) {
        // translate data from ANSII to UNICODE
        cb = (DWORD)(GlobalSize(hDDE) << 1);
        hDDEnew = UserGlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, cb);
        USERGLOBALLOCK(hDDEnew, pwstr);
        USERGLOBALLOCK(hDDE, pstr);
        if (pwstr != NULL && pstr != NULL) {
            MBToWCS(pstr, -1, &pwstr, cb, FALSE);
        }
        if (pstr) {
            USERGLOBALUNLOCK(hDDE);
        }
        if (pwstr) {
            USERGLOBALUNLOCK(hDDEnew);
        }
    } else {
        return (hDDE); // no translation needed.
    }
    if (fFreeSource) {
        WOWGLOBALFREE(hDDE);
    }
    return (hDDEnew);
}


/***************************************************************************\
* ClStartExecute
*
* Description:
* Starts an execute transaction.
*
* History:
* 11-19-91 sanfords Created.
* 1/28/92 sanfords added UNICODE support.
\***************************************************************************/
BOOL ClStartExecute(
PXACT_INFO pxi)
{
    DWORD dwError;

    pxi->hDDESent = MaybeTranslateExecuteData(pxi->hDDESent,
            pxi->pcoi->pcii->flags & IIF_UNICODE,
            pxi->pcoi->state & ST_UNICODE_EXECUTE,
            TRUE);

    dwError = PackAndPostMessage(pxi->pcoi->hwndPartner, 0, WM_DDE_EXECUTE,
            pxi->pcoi->hwndConv, 0, 0, HandleToUlong(pxi->hDDESent));
    if (dwError) {
        SetLastDDEMLError(pxi->pcoi->pcii, dwError);
        return (FALSE);
    }
    pxi->state = XST_EXECSENT;
    pxi->pfnResponse = (FNRESPONSE)ClRespExecuteAck;
    LinkTransaction(pxi);
    return (TRUE);
}


/***************************************************************************\
* SvSpontExecute
*
* Description:
* Responds to a WM_DDE_EXECUTE message.
*
* History:
* 11-19-91 sanfords Created.
* 1/28/92 sanfords added UNICODE support.
\***************************************************************************/
BOOL SvSpontExecute(
PSVR_CONV_INFO psi,
LPARAM lParam)
{
    HANDLE hDDE, hDDEx;
    ULONG_PTR dwRet = 0;
    DWORD dwError;
    HDDEDATA hData = 0;

    hDDEx = hDDE = (HANDLE)lParam; // UnpackDDElParam(msg, lParam, NULL, &hDDE);
    if (psi->ci.pcii->afCmd & CBF_FAIL_EXECUTES) {
        goto Ack;
    }

    /*
     * Note that if unicode translation is needed, we use the translated
     * handle for the callback and then destroy it but the ACK is always
     * the original hDDE so that the protocol isn't violated:
     *
     * DDE COMMANDMENT #324: Thou shalt pass back the exact same data
     * handle in an execute ACK that you were given by the execute
     * message.
     */
    hDDEx = MaybeTranslateExecuteData(hDDE,
            psi->ci.state & ST_UNICODE_EXECUTE,
            psi->ci.pcii->flags & IIF_UNICODE,
            FALSE);

    hData = InternalCreateDataHandle(psi->ci.pcii, (LPBYTE)hDDEx, (DWORD)-1, 0,
        HDATA_EXECUTE | HDATA_READONLY | HDATA_NOAPPFREE, 0, 0);
    if (!hData) {
        SetLastDDEMLError(psi->ci.pcii, DMLERR_MEMORY_ERROR);
        goto Ack;
    }

    dwRet = (ULONG_PTR)DoCallback(psi->ci.pcii,
            XTYP_EXECUTE, 0, psi->ci.hConv,
            NORMAL_HSZ_FROM_LATOM(psi->ci.laTopic), 0, hData, 0, 0);
    UnpackAndFreeDDEMLDataHandle(hData, TRUE);

    if (dwRet == (ULONG_PTR)CBR_BLOCK) {
        if (hDDEx != hDDE) {
            WOWGLOBALFREE(hDDEx);
        }
        return (FALSE);
    }

Ack:
    dwRet &= ~DDE_FACKRESERVED;
    dwError = PackAndPostMessage(psi->ci.hwndPartner, WM_DDE_EXECUTE,
            WM_DDE_ACK, psi->ci.hwndConv, lParam, (DWORD)dwRet, HandleToUlong(hDDE));
    if (dwError) {
        SetLastDDEMLError(psi->ci.pcii, dwError);
    }

    if (hDDEx != hDDE) {
        WOWGLOBALFREE(hDDEx);
    }

    return (TRUE);
}



/***************************************************************************\
* ClRespExecuteAck
*
* Description:
* Responds to a WM_DDE_ACK in response to a WM_DDE_EXECUTE message.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL ClRespExecuteAck(
PXACT_INFO pxi,
UINT msg,
LPARAM lParam)
{
    UINT_PTR uiLo, uiHi;

    if (msg) {
        if (msg != WM_DDE_ACK) {
            return (SpontaneousClientMessage((PCL_CONV_INFO)pxi->pcoi, msg, lParam));
        }

        UnpackDDElParam(WM_DDE_ACK, lParam, &uiLo, &uiHi);
        if (uiHi != HandleToUlong(pxi->hDDESent)) {
            return (SpontaneousClientMessage((PCL_CONV_INFO)pxi->pcoi, msg, lParam));
        }

        WOWGLOBALFREE((HANDLE)uiHi);

        pxi->state = XST_EXECACKRCVD;
        pxi->wStatus = (WORD)uiLo;

        if (TransactionComplete(pxi, (HDDEDATA)(pxi->wStatus & DDE_FACK ? 1 : 0))) {
            goto Cleanup;
        }
    } else {
Cleanup:
        GlobalDeleteAtom(pxi->gaItem); // pxi copy
        UnlinkTransaction(pxi);
        if (pxi->hXact) {
            DestroyHandle(pxi->hXact);
        }
        DDEMLFree(pxi);
    }
    if (msg) {
        FreeDDElParam(msg, lParam);
    }
    return (TRUE);
}



//----------------------------------POKE-------------------------------//


/***************************************************************************\
* ClStartPoke
*
* Description:
* Initiates a poke transaction.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL ClStartPoke(
PXACT_INFO pxi)
{
    DWORD dwError;

    IncGlobalAtomCount(pxi->gaItem); // message copy
    dwError = PackAndPostMessage(pxi->pcoi->hwndPartner, 0, WM_DDE_POKE,
            pxi->pcoi->hwndConv, 0, HandleToUlong(pxi->hDDESent), pxi->gaItem);
    if (dwError) {
        SetLastDDEMLError(pxi->pcoi->pcii, dwError);
        GlobalDeleteAtom(pxi->gaItem); // message copy
        return (FALSE);
    }

    pxi->state = XST_POKESENT;
    pxi->pfnResponse = (FNRESPONSE)ClRespPokeAck;
    LinkTransaction(pxi);
    return (TRUE);
}


/***************************************************************************\
* SvSpontPoke
*
* Description:
* Handles WM_DDE_POKE messages.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL SvSpontPoke(
PSVR_CONV_INFO psi,
LPARAM lParam)
{
    UINT_PTR uiHi;
    HANDLE hDDE = 0;
    HDDEDATA hData;
    ULONG_PTR dwRet = 0;
    DWORD dwError;
    WORD wFmt, wStatus;
    LATOM al;

    // See what we have

    UnpackDDElParam(WM_DDE_DATA, lParam, (PUINT_PTR)&hDDE, &uiHi);

    if (!(psi->ci.pcii->afCmd & CBF_FAIL_POKES)) {
        if (!hDDE) {
            goto Ack;
        }
        if (!ExtractDDEDataInfo(hDDE, &wStatus, &wFmt)) {
            FreeDDEData(hDDE, FALSE, TRUE);             // free message data
            goto Ack;
        }

        hData = InternalCreateDataHandle(psi->ci.pcii, (LPBYTE)hDDE, (DWORD)-1, 0,
                HDATA_NOAPPFREE | HDATA_READONLY, 0, 0);
        if (!hData) {
            SetLastDDEMLError(psi->ci.pcii, DMLERR_MEMORY_ERROR);
            FreeDDEData(hDDE, FALSE, TRUE);       // free message data
            goto Ack;                             // Nack it.
            return(TRUE);
        }

        al = GlobalToLocalAtom((GATOM)uiHi);
            dwRet = (ULONG_PTR)DoCallback(psi->ci.pcii, XTYP_POKE,
                    wFmt, psi->ci.hConv,
                    NORMAL_HSZ_FROM_LATOM(psi->ci.laTopic),
                    NORMAL_HSZ_FROM_LATOM(al),
                    hData, 0, 0);
        DeleteAtom(al);
        UnpackAndFreeDDEMLDataHandle(hData, FALSE);
    }
    if (dwRet == (ULONG_PTR)CBR_BLOCK) {

        // Note: this code makes an app that return s CBR_BLOCK unable to
        // access the data after the callback return .

        return (FALSE);
    }
    if (dwRet & DDE_FACK) {
        FreeDDEData(hDDE, FALSE, TRUE);
    }

Ack:
    dwRet &= ~DDE_FACKRESERVED;
    dwError = PackAndPostMessage(psi->ci.hwndPartner, WM_DDE_POKE, WM_DDE_ACK,
            psi->ci.hwndConv, lParam, dwRet, uiHi);
    if (dwError) {
        SetLastDDEMLError(psi->ci.pcii, dwError);
    }
    return (TRUE);
}


/***************************************************************************\
* ClRespPokeAck
*
* Description:
* Response to a WM_DDE_ACK message in response to a WM_DDE_POKE message.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL ClRespPokeAck(
PXACT_INFO pxi,
UINT msg,
LPARAM lParam)
{
    UINT_PTR uiLo, uiHi;

    if (msg) {
        if (msg != WM_DDE_ACK) {
            return (SpontaneousClientMessage((PCL_CONV_INFO)pxi->pcoi, msg, lParam));
        }

        UnpackDDElParam(WM_DDE_ACK, lParam, &uiLo, &uiHi);
        if ((GATOM)uiHi != pxi->gaItem) {
            return (SpontaneousClientMessage((PCL_CONV_INFO)pxi->pcoi, msg, lParam));
        }

        GlobalDeleteAtom((ATOM)uiHi); // message copy

        pxi->state = XST_POKEACKRCVD;
        pxi->wStatus = (WORD)uiLo;

        if (!((WORD)uiLo & DDE_FACK)) {
            //
            // NACKs make it our business to free the poked data.
            //
            FreeDDEData(pxi->hDDESent, FALSE, TRUE);
        }

        if (TransactionComplete(pxi,
                (HDDEDATA)(pxi->wStatus & DDE_FACK ? 1 : 0))) {
            goto Cleanup;
        }
    } else {
Cleanup:
        GlobalDeleteAtom(pxi->gaItem); // pxi copy
        UnlinkTransaction(pxi);
        if (pxi->hXact) {
            DestroyHandle(pxi->hXact);
        }
        DDEMLFree(pxi);
    }
    if (msg) {
        FreeDDElParam(msg, lParam);
    }
    return (TRUE);
}


//-------------------------------REQUEST-------------------------------//

/***************************************************************************\
* ClStartRequest
*
* Description:
* Start a request transaction.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL ClStartRequest(
PXACT_INFO pxi)
{
    DWORD dwError;

    IncGlobalAtomCount(pxi->gaItem); // message copy
    dwError = PackAndPostMessage(pxi->pcoi->hwndPartner, 0, WM_DDE_REQUEST,
            pxi->pcoi->hwndConv, 0, pxi->wFmt, pxi->gaItem);
    if (dwError) {
        SetLastDDEMLError(pxi->pcoi->pcii, dwError);
        GlobalDeleteAtom(pxi->gaItem); // message copy
        return (FALSE);
    }

    pxi->state = XST_REQSENT;
    pxi->pfnResponse = (FNRESPONSE)ClRespRequestData;
    LinkTransaction(pxi);
    return (TRUE);
}



/***************************************************************************\
* SvSpontRequest
*
* Description:
* Respond to a WM_DDE_REQUEST message.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL SvSpontRequest(
PSVR_CONV_INFO psi,
LPARAM lParam)
{
    HANDLE hDDE = 0;
    HDDEDATA hDataRet;
    WORD wFmt, wStatus;
    DWORD dwError;
    LATOM la;

    if (psi->ci.pcii->afCmd & CBF_FAIL_REQUESTS) {
        goto Nack;
    }
    // See what we have

    // UnpackDDElParam(lParam, WM_DDE_REQUEST, .... Requests arn't packed
    wFmt = LOWORD(lParam);
    la = GlobalToLocalAtom((GATOM)HIWORD(lParam));
    hDataRet = DoCallback(psi->ci.pcii, XTYP_REQUEST,
            wFmt, psi->ci.hConv,
            NORMAL_HSZ_FROM_LATOM(psi->ci.laTopic),
            NORMAL_HSZ_FROM_LATOM(la),
            (HDDEDATA)0, 0, 0);
    DeleteAtom(la);

    if (hDataRet == CBR_BLOCK) {
        return (FALSE);
    }

    if (hDataRet) {

        hDDE = UnpackAndFreeDDEMLDataHandle(hDataRet, FALSE);
        if (!hDDE) {
            SetLastDDEMLError(psi->ci.pcii, DMLERR_DLL_USAGE);
            goto Nack;
        }
        if (!ExtractDDEDataInfo(hDDE, &wStatus, &wFmt)) {
            SetLastDDEMLError(psi->ci.pcii, DMLERR_DLL_USAGE);
            goto Nack;
        }
        if (!(wStatus & DDE_FRELEASE)) {
            // Its APPOWNED or relayed from another server - only safe
            // thing to do is use a copy.
            hDDE = CopyDDEData(hDDE, FALSE);
            if (!hDDE) {
                SetLastDDEMLError(psi->ci.pcii, DMLERR_MEMORY_ERROR);
                goto Nack;
            }
        }

        // Keep it simple, DDEML servers never ask for acks from requests.

        wStatus = DDE_FRELEASE | DDE_FREQUESTED;
        AllocAndSetDDEData((LPBYTE)hDDE, (DWORD)-1, wStatus, wFmt);

        // just reuse HIWORD(lParam) (aItem) - message copy
        if (dwError = PackAndPostMessage(psi->ci.hwndPartner, WM_DDE_REQUEST,
                WM_DDE_DATA, psi->ci.hwndConv, 0, HandleToUlong(hDDE), HIWORD(lParam))) {
            SetLastDDEMLError(psi->ci.pcii, dwError);
            GlobalDeleteAtom(HIWORD(lParam)); // message copy
        }

    } else {
Nack:
        // just reuse HIWORD(lParam) (aItem) - message copy
        dwError = PackAndPostMessage(psi->ci.hwndPartner, WM_DDE_REQUEST,
                WM_DDE_ACK, psi->ci.hwndConv, 0, 0, HIWORD(lParam));
        if (dwError) {
            SetLastDDEMLError(psi->ci.pcii, dwError);
            GlobalDeleteAtom(HIWORD(lParam)); // message copy
        }
    }

    return (TRUE);
}


/***************************************************************************\
* ClRespRequestData
*
* Description:
* Handles response to either a WM_DDE_ACK or WM_DDE_DATA in response to
* a WM_DDE_REQUEST message.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL ClRespRequestData(
PXACT_INFO pxi,
UINT msg,
LPARAM lParam)
{
    UINT_PTR uiLo, uiHi;
    WORD wFmt, wStatus;
    DWORD dwError;

    if (msg) {
        switch (msg) {
        case WM_DDE_DATA:
            UnpackDDElParam(WM_DDE_DATA, lParam, (PUINT_PTR)&pxi->hDDEResult, &uiHi);
            if (!pxi->hDDEResult) {
                // must be an advise data message with NODATA.
                return (ClSpontAdviseData((PCL_CONV_INFO)pxi->pcoi, lParam));
            }
            if (!ExtractDDEDataInfo(pxi->hDDEResult, &wStatus, &wFmt)) {
                return (ClSpontAdviseData((PCL_CONV_INFO)pxi->pcoi, lParam));
            }
            if (!(wStatus & DDE_FREQUESTED)) {
                // must be advise data
                return (ClSpontAdviseData((PCL_CONV_INFO)pxi->pcoi, lParam));
            }
            if (wStatus & DDE_FACKREQ) {

                // if DDE_FRELEASE is not set, and this is a synchronous
                // transaction, we need to make a copy here so the user
                // can free at his leisure.

                // reuse uiHi - message copy
                dwError = PackAndPostMessage(pxi->pcoi->hwndPartner,
                        WM_DDE_DATA, WM_DDE_ACK, pxi->pcoi->hwndConv, 0,
                        pxi->wFmt == wFmt && pxi->gaItem == (GATOM)uiHi ?
                            DDE_FACK : 0, uiHi);
                if (dwError) {
                    SetLastDDEMLError(pxi->pcoi->pcii, dwError);
                }
            } else {
                GlobalDeleteAtom((GATOM)uiHi);     // message copy
            }
            if (wFmt != pxi->wFmt || (GATOM)uiHi != pxi->gaItem) {
                /*
                 * BOGUS returned data!  Just free it and make it look like
                 * a NACK
                 */
                FreeDDEData(pxi->hDDEResult, FALSE, TRUE);
                pxi->hDDEResult = 0;
                if (TransactionComplete(pxi, 0)) {
                    goto Cleanup;
                }
            } else {
                if (TransactionComplete(pxi, (HDDEDATA)-1)) {
                    goto Cleanup;
                }
            }
            break;

        case WM_DDE_ACK:
            UnpackDDElParam(WM_DDE_ACK, lParam, &uiLo, &uiHi);
            if ((GATOM)uiHi != pxi->gaItem) {
                return(SpontaneousClientMessage((PCL_CONV_INFO)pxi->pcoi, msg, lParam));
            }
            pxi->state = XST_DATARCVD;
            pxi->wStatus = (WORD)uiLo;
            GlobalDeleteAtom((GATOM)uiHi); // message copy
            if (TransactionComplete(pxi, 0)) {
                goto Cleanup;
            }
            break;

        default:
            return (SpontaneousClientMessage((PCL_CONV_INFO)pxi->pcoi, msg, lParam));
        }

    } else {

Cleanup:
        GlobalDeleteAtom(pxi->gaItem); // pxi copy
        if (pxi->hDDEResult) {
            FreeDDEData(pxi->hDDEResult, FALSE, TRUE);  // free message data
        }
        UnlinkTransaction(pxi);
        DDEMLFree(pxi);
    }
    if (msg) {
        FreeDDElParam(msg, lParam);
    }
    return (TRUE);
}

//----------------------SPONTANEOUS CLIENT MESSAGE---------------------//

/***************************************************************************\
* SpontaneousClientMessage
*
* Description:
* General unexpected message client side handler.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL SpontaneousClientMessage(
PCL_CONV_INFO pci,
UINT msg,
LPARAM lParam)
{
    switch (msg) {
    case WM_DDE_DATA:
        return (ClSpontAdviseData(pci, lParam));
        break;

    default:
        DumpDDEMessage(!(pci->ci.state & ST_INTRA_PROCESS), msg, lParam);
        ShutdownConversation((PCONV_INFO)pci, TRUE);
        return (TRUE);
    }
}

//----------------------SPONTANEOUS SERVER MESSAGE---------------------//

/***************************************************************************\
* SpontaneousServerMessage
*
* Description:
* General unexpected message server side handler.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL SpontaneousServerMessage(
PSVR_CONV_INFO psi,
UINT msg,
LPARAM lParam)
{
    switch (msg) {
    case WM_DDE_ADVISE:
        return (SvSpontAdvise(psi, lParam));
        break;

    case WM_DDE_UNADVISE:
        return (SvSpontUnadvise(psi, lParam));
        break;

    case WM_DDE_EXECUTE:
        return (SvSpontExecute(psi, lParam));
        break;

    case WM_DDE_POKE:
        return (SvSpontPoke(psi, lParam));
        break;

    case WM_DDE_REQUEST:
        return (SvSpontRequest(psi, lParam));
        break;

    default:
        DumpDDEMessage(!(psi->ci.state & ST_INTRA_PROCESS), msg, lParam);

        /*
         * It use to call ShutdownConversation here. Don't call it
         * anymore. Fix for bugs: 49063, 70906
         */
        //ShutdownConversation((PCONV_INFO)psi, TRUE);
        return (TRUE);
    }
}



//-------------------------HELPER FUNCTIONS----------------------------//



/***************************************************************************\
* AllocAndSetDDEData
*
* Description:
* Worker function to create a data handle of size cb with wStatus and
* wFmt initialized. If cb == -1 pSrc is assumed to be a valid hDDE
* that is to have its data set.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
HANDLE AllocAndSetDDEData(
LPBYTE pSrc,
DWORD cb,
WORD wStatus,
WORD wFmt) // a 0 format implied execute data
{
    HANDLE hDDE;
    DWORD cbOff;
    PDDE_DATA pdde;
    DWORD fCopyIt;

    if (cb == -1) {
        hDDE = (HANDLE)pSrc;
        cb = (DWORD)GlobalSize(hDDE);
        fCopyIt = FALSE;
    } else {
        hDDE = UserGlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE | GMEM_ZEROINIT,
                (wFmt ? (cb + 4) : cb));
        fCopyIt = (pSrc != NULL);
    }
    if (hDDE == NULL) {
        return(0);
    }
    USERGLOBALLOCK(hDDE, pdde);
    if (pdde == NULL) {
        WOWGLOBALFREE(hDDE);
        return (0);
    }
    if (wFmt) {
        pdde->wStatus = wStatus;
        pdde->wFmt = wFmt;
        cbOff = 4;
    } else {
        cbOff = 0;
    }
    if (fCopyIt) {
        RtlCopyMemory((PBYTE)pdde + cbOff, pSrc, cb);
    }
    USERGLOBALUNLOCK(hDDE);

    return (hDDE);
}



/***************************************************************************\
* PackAndPostMessage
*
* Description:
* Worker function to provide common functionality. An error code is
* return ed on failure. 0 on success.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
DWORD PackAndPostMessage(
HWND hwndTo,
UINT msgIn,
UINT msgOut,
HWND hwndFrom,
LPARAM lParam,
UINT_PTR uiLo,
UINT_PTR uiHi)
{
    DWORD retval;

    lParam = ReuseDDElParam(lParam, msgIn, msgOut, uiLo, uiHi);
    if (!lParam) {
        return (DMLERR_MEMORY_ERROR);
    }
    CheckDDECritIn;
    LeaveDDECrit;
    CheckDDECritOut;

    retval = (DWORD)PostMessage(hwndTo, msgOut, (WPARAM)hwndFrom, lParam);
    switch (retval) {
    case FAIL_POST:
#if (FAIL_POST != FALSE)
#error FAIL_POST must be defined as PostMessage's failure return value.
#endif
        FreeDDElParam(msgOut, lParam);
        RIPMSG0(RIP_WARNING, "PostMessage failed.");
        /* Fall through */

    case FAILNOFREE_POST:
        retval = DMLERR_POSTMSG_FAILED;
        break;

    default:
#if (FAKE_POST != TRUE)
#error FAKE_POST must be defined as PostMessage's success return value.
#endif
        UserAssert(retval == TRUE);
        retval = 0;
    }

    EnterDDECrit;
    return (retval);
}



/***************************************************************************\
* ExtractDDEDataInfo
*
* Description:
* Worker function to retrieve wStatus and wFmt from a standard DDE data
* handle - NOT FOR EXECUTE HANDLES.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL ExtractDDEDataInfo(
HANDLE hDDE,
LPWORD pwStatus,
LPWORD pwFmt)
{
    PDDE_DATA pdde;

    USERGLOBALLOCK(hDDE, pdde);
    if (pdde == NULL) {
        return (FALSE);
    }
    *pwStatus = pdde->wStatus;
    *pwFmt = pdde->wFmt;
    USERGLOBALUNLOCK(hDDE);
    return (TRUE);
}



/***************************************************************************\
* TransactionComplete
*
* Description:
* Called when a response function completes a transaction. pxi->wStatus,
* pxi->flags, pxi->wFmt, pxi->gaItem, pxi->hXact, and hData are expected
* to be set apropriately for a XTYP_XACT_COMPLETE callback.
*
* fCleanup is returned - TRUE implies the calling function needs to
* cleanup its pxi before returning.  (fAsync case.)
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL TransactionComplete(
PXACT_INFO pxi,
HDDEDATA hData)
{
    LATOM al;
    BOOL fMustFree;

    if (pxi->flags & XIF_ABANDONED) {
        UserAssert(!(pxi->flags & XIF_SYNCHRONOUS));
        return (TRUE);
    }
    pxi->flags |= XIF_COMPLETE;
    if (pxi->flags & XIF_SYNCHRONOUS) {
        PostMessage(pxi->pcoi->hwndConv, WM_TIMER, TID_TIMEOUT, 0);
        return (FALSE);
    } else {
        if (hData == (HDDEDATA)(-1)) {
            fMustFree = TRUE;
            hData = InternalCreateDataHandle(pxi->pcoi->pcii,
                (LPBYTE)pxi->hDDEResult, (DWORD)-1, 0,
                HDATA_NOAPPFREE | HDATA_READONLY, 0, 0);
        } else {
            fMustFree = FALSE;
        }
        al = GlobalToLocalAtom(pxi->gaItem);

        if (!(pxi->wStatus & DDE_FACK)) {
            if (pxi->wStatus & DDE_FBUSY) {
                SetLastDDEMLError(pxi->pcoi->pcii, DMLERR_BUSY);
            } else {
                SetLastDDEMLError(pxi->pcoi->pcii, DMLERR_NOTPROCESSED);
            }
        }

        /*
         * During the callback the app may disconnect or otherwise kill
         * this conversation so we unlink the pxi FIRST so cleanup code
         * doesn't destroy it before this transaction code exits.
         */
        UnlinkTransaction(pxi);

        DoCallback(
            pxi->pcoi->pcii,
            (WORD)XTYP_XACT_COMPLETE,
            pxi->wFmt,
            pxi->pcoi->hConv,
            NORMAL_HSZ_FROM_LATOM(pxi->pcoi->laTopic),
            (HSZ)al,
            hData,
            HandleToUlong(pxi->hXact),
            (DWORD)pxi->wStatus);
        DeleteAtom(al);
        if (fMustFree) {
            InternalFreeDataHandle(hData, FALSE);
            pxi->hDDEResult = 0;
        }

        /*
         * during the callback is the only time the app has to access the
         * transaction information.   pxi->hXact will be invalid once he
         * returns.
         */
        if (pxi->hXact) {
            DestroyHandle(pxi->hXact);
            pxi->hXact = 0;
        }
        return (TRUE);
    }
}



/***************************************************************************\
* UnpackAndFreeDDEMLDataHandle
*
* Description:
* Removes DDEML data handle wrapping from a DDE data handle. If the
* data handle is APPOWNED the wrapping is NOT freed. The hDDE is
* return ed or 0 on failure. If fExec is FALSE, this call fails on
* HDATA_EXECUTE type handles.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
HANDLE UnpackAndFreeDDEMLDataHandle(
HDDEDATA hData,
BOOL fExec)
{
    PDDEMLDATA pdd;
    HANDLE hDDE;

    CheckDDECritIn;

    if (hData == 0) {
        return (0);
    }
    pdd = (PDDEMLDATA)ValidateCHandle((HANDLE)hData, HTYPE_DATA_HANDLE,
            HINST_ANY);
    if (pdd == NULL) {
        return (0);
    }
    if (!fExec && pdd->flags & HDATA_EXECUTE) {
        return (0);
    }

    hDDE = pdd->hDDE;
    if (pdd->flags & HDATA_APPOWNED) {
        return (hDDE); // don't destroy appowned data handles
    }
    DDEMLFree(pdd);
    DestroyHandle((HANDLE)hData);
    return (hDDE);
}
