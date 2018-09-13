/****************************** Module Header ******************************\
* Module Name: callback.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* DDE Manager callback related functions
*
* Created: 11/11/91 Sanford Staab
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* DoCallback
*
* Description:
* Performs a synchronous callback to the given instance's callback proc.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
HDDEDATA DoCallback(
PCL_INSTANCE_INFO pcii,
WORD wType,
WORD wFmt,
HCONV hConv,
HSZ hsz1,
HSZ hsz2,
HDDEDATA hData,
ULONG_PTR dw1,
ULONG_PTR dw2)
{
    HDDEDATA hDataRet;
    PCLIENTINFO pci;

    CheckDDECritIn;


    /*
     * Zombie conversations don't generate callbacks!
     */
    if (hConv && TypeFromHandle(hConv) == HTYPE_ZOMBIE_CONVERSATION) {
        return(0);
    }

    pci = GetClientInfo();
    pci->cInDDEMLCallback++;

    pcii->cInDDEMLCallback++;
    LeaveDDECrit;
    CheckDDECritOut;

    /*
     * Bug 246472 - joejo
     * fixup all DDE Callbacks since some apps make their callbacks
     * C-Style instead of PASCAL.
     */
    hDataRet = UserCallDDECallback(*pcii->pfnCallback, (UINT)wType, (UINT)wFmt, hConv, hsz1, hsz2,
            hData, dw1, dw2);
    
    EnterDDECrit;
    pcii->cInDDEMLCallback--;
    pci->cInDDEMLCallback--;

    if (!(pcii->afCmd & APPCLASS_MONITOR) && pcii->MonitorFlags & MF_CALLBACKS) {
        PEVENT_PACKET pep;

        pep = (PEVENT_PACKET)DDEMLAlloc(sizeof(EVENT_PACKET) - sizeof(DWORD) +
                sizeof(MONCBSTRUCT));
        if (pep != NULL) {

            pep->EventType =    MF_CALLBACKS;
            pep->fSense =       TRUE;
            pep->cbEventData =  sizeof(MONCBSTRUCT);

#define pcbs ((MONCBSTRUCT *)&pep->Data)
            pcbs->cb =      sizeof(MONCBSTRUCT);
            pcbs->dwTime =  NtGetTickCount();
            pcbs->hTask =   (HANDLE)LongToHandle( pcii->tid );
            pcbs->dwRet =   HandleToUlong(hDataRet);
            pcbs->wType =   wType;
            pcbs->wFmt =    wFmt;
            pcbs->hConv =   hConv;
            pcbs->hsz1 =    (HSZ)LocalToGlobalAtom(LATOM_FROM_HSZ(hsz1));
            pcbs->hsz2 =    (HSZ)LocalToGlobalAtom(LATOM_FROM_HSZ(hsz2));
            pcbs->hData =   hData;
            pcbs->dwData1 = dw1;
            pcbs->dwData2 = dw2;
            if (((wType == XTYP_CONNECT) || (wType == XTYP_WILDCONNECT)) && dw1) {
                RtlCopyMemory(&pcbs->cc, (PVOID)dw1, sizeof(CONVCONTEXT));
            }

            LeaveDDECrit;

                if (wType & XCLASS_DATA) {
                    if (hDataRet && hDataRet != CBR_BLOCK) {
                        pcbs->cbData = DdeGetData(hDataRet, (LPBYTE)pcbs->Data, 32, 0);
                    }
                } else if (hData) {
                    pcbs->cbData = DdeGetData(hData, (LPBYTE)pcbs->Data, 32, 0);
                }

                Event(pep);

            EnterDDECrit;

            GlobalDeleteAtom(LATOM_FROM_HSZ(pcbs->hsz1));
            GlobalDeleteAtom(LATOM_FROM_HSZ(pcbs->hsz2));
            DDEMLFree(pep);
#undef pcbs
        }
    }
    return (hDataRet);
}



/***************************************************************************\
* _ClientEventCallback
*
* Description:
* Called from the server side to perform event callbacks.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
DWORD _ClientEventCallback(
PCL_INSTANCE_INFO pcii,
PEVENT_PACKET pep)
{
    HDDEDATA hData;

    EnterDDECrit;

    switch (pep->EventType) {
    case 0: // MonitorFlags change event - everybody gets it
        pcii->MonitorFlags = pep->Data;
        break;

    case MF_CALLBACKS:
        {
            MONCBSTRUCT mcb;

            mcb = *((MONCBSTRUCT *)&pep->Data);
            mcb.hsz1 = NORMAL_HSZ_FROM_LATOM(GlobalToLocalAtom((GATOM)(ULONG_PTR)mcb.hsz1));
            mcb.hsz2 = NORMAL_HSZ_FROM_LATOM(GlobalToLocalAtom((GATOM)(ULONG_PTR)mcb.hsz2));
            if (    mcb.wType == XTYP_REGISTER ||
                    mcb.wType == XTYP_UNREGISTER) {
                mcb.hsz2 = INST_SPECIFIC_HSZ_FROM_LATOM((LATOM)(ULONG_PTR)mcb.hsz2);
            }
            hData = InternalCreateDataHandle(pcii, (LPSTR)&mcb,
                    pep->cbEventData, 0,
                    HDATA_NOAPPFREE | HDATA_READONLY | HDATA_EXECUTE, 0, 0);
            if (hData) {
                DoCallback(pcii, (WORD)XTYP_MONITOR, 0, 0, 0, 0, hData, 0L,
                        pep->EventType);
                InternalFreeDataHandle((HDDEDATA)hData, TRUE);
                DeleteAtom(LATOM_FROM_HSZ(mcb.hsz1));
                DeleteAtom(LATOM_FROM_HSZ(mcb.hsz2));
            }
        }
        break;

    case MF_LINKS:
        {
            MONLINKSTRUCT ml;

            ml = *((MONLINKSTRUCT *)&pep->Data);
            ml.hszSvc = NORMAL_HSZ_FROM_LATOM(GlobalToLocalAtom((GATOM)(ULONG_PTR)ml.hszSvc));
            ml.hszTopic = NORMAL_HSZ_FROM_LATOM(GlobalToLocalAtom((GATOM)(ULONG_PTR)ml.hszTopic));
            ml.hszItem = NORMAL_HSZ_FROM_LATOM(GlobalToLocalAtom((GATOM)(ULONG_PTR)ml.hszItem));
            hData = InternalCreateDataHandle(pcii, (LPSTR)&ml,
                    pep->cbEventData, 0,
                    HDATA_NOAPPFREE | HDATA_READONLY | HDATA_EXECUTE, 0, 0);
            if (hData) {
                DoCallback(pcii, (WORD)XTYP_MONITOR, 0, 0, 0, 0, hData, 0L,
                        pep->EventType);
                InternalFreeDataHandle((HDDEDATA)hData, TRUE);
                DeleteAtom(LATOM_FROM_HSZ(ml.hszSvc));
                DeleteAtom(LATOM_FROM_HSZ(ml.hszTopic));
                DeleteAtom(LATOM_FROM_HSZ(ml.hszItem));
            }
        }
        break;

    case MF_CONV:
        {
            MONCONVSTRUCT mc;

            mc = *((MONCONVSTRUCT *)&pep->Data);
            mc.hszSvc = NORMAL_HSZ_FROM_LATOM(GlobalToLocalAtom((GATOM)(ULONG_PTR)mc.hszSvc));
            mc.hszTopic = NORMAL_HSZ_FROM_LATOM(GlobalToLocalAtom((GATOM)(ULONG_PTR)mc.hszTopic));
            hData = InternalCreateDataHandle(pcii, (LPSTR)&mc,
                    pep->cbEventData, 0,
                    HDATA_NOAPPFREE | HDATA_READONLY | HDATA_EXECUTE, 0, 0);
            if (hData) {
                DoCallback(pcii, (WORD)XTYP_MONITOR, 0, 0, 0, 0, hData, 0L,
                        pep->EventType);
                InternalFreeDataHandle((HDDEDATA)hData, TRUE);
                DeleteAtom(LATOM_FROM_HSZ(mc.hszSvc));
                DeleteAtom(LATOM_FROM_HSZ(mc.hszTopic));
            }
        }
        break;

    case MF_HSZ_INFO:
        if (!(pcii->flags & IIF_UNICODE)) {
            LPSTR pszAnsi;
            /*
             * Translate HSZ string back into ANSI
             */
            if (WCSToMB(((PMONHSZSTRUCT)&pep->Data)->str,
                    ((int)pep->cbEventData - (int)((PMONHSZSTRUCT)&pep->Data)->cb) / sizeof(WCHAR),
                    &pszAnsi,
                    (int)pep->cbEventData - (int)((PMONHSZSTRUCT)&pep->Data)->cb,
                    TRUE)) {
                strcpy(((PMONHSZSTRUCTA)&pep->Data)->str, pszAnsi);
                UserLocalFree(pszAnsi);
            }
            ((PMONHSZSTRUCT)&pep->Data)->cb = sizeof(MONHSZSTRUCTA);
        }
        // fall through
    case MF_SENDMSGS:
    case MF_POSTMSGS:
        if (pep->EventType == MF_POSTMSGS) {
            PMONMSGSTRUCT pmms = (PMONMSGSTRUCT)&pep->Data;
            BYTE buf[32];

            /*
             * We may need to translate the Execute string to/from
             * UNICODE depending on what type of monitor this is
             * going to.
             */
            if (pmms->wMsg == WM_DDE_EXECUTE) {
                BOOL fUnicodeText;
                int flags;

                flags = (IS_TEXT_UNICODE_UNICODE_MASK |
                        IS_TEXT_UNICODE_REVERSE_MASK |
                        (IS_TEXT_UNICODE_NOT_UNICODE_MASK &
                        (~IS_TEXT_UNICODE_ILLEGAL_CHARS)) |
                        IS_TEXT_UNICODE_NOT_ASCII_MASK);
#ifdef ISTEXTUNICODE_WORKS
                fUnicodeText = RtlIsTextUnicode(pmms->dmhd.Data,
                        min(32, pmms->dmhd.cbData), &flags);
#else
                fUnicodeText = (*(LPSTR)pmms->dmhd.Data == '\0');
#endif

                if (pcii->flags & IIF_UNICODE && !fUnicodeText) {
                    /* Ascii->UNICODE */
                    RtlMultiByteToUnicodeN((LPWSTR)buf, 32, NULL,
                            (LPSTR)&pmms->dmhd.Data,
                            min(32, pmms->dmhd.cbData));
                    RtlCopyMemory(&pmms->dmhd.Data, buf, 32);
                } else if (!(pcii->flags & IIF_UNICODE) && fUnicodeText) {
                    /* UNICODE->Ascii */
                    RtlUnicodeToMultiByteN((LPSTR)buf, 32, NULL,
                            (LPWSTR)&pmms->dmhd.Data,
                            min(32, pmms->dmhd.cbData));
                    RtlCopyMemory(&pmms->dmhd.Data, buf, 32);
                }
            }
        }
    case MF_ERRORS:
        hData = InternalCreateDataHandle(pcii, (LPSTR)&pep->Data,
                pep->cbEventData, 0,
                HDATA_NOAPPFREE | HDATA_READONLY | HDATA_EXECUTE, 0, 0);
        if (hData) {
            DoCallback(pcii, (WORD)XTYP_MONITOR, 0, 0, 0, 0, hData, 0L,
                    pep->EventType);
            InternalFreeDataHandle((HDDEDATA)hData, TRUE);
        }
        break;
    }

    LeaveDDECrit;
    return (0);
}



/***************************************************************************\
* EnableEnumProc
*
* Description:
* Helper function for applying pees->wCmd to each client and server
* DDEML window.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL EnableEnumProc(
HWND hwnd,
PENABLE_ENUM_STRUCT pees)
{
    PCONV_INFO pcoi;

    for (pcoi = (PCONV_INFO)GetWindowLongPtr(hwnd, GWLP_PCI);
                pcoi != NULL; pcoi = pcoi->next) {
        pcoi->cLocks++;
        *pees->pfRet |= SetEnableState(pcoi, pees->wCmd);
        if (pees->wCmd2) {
            /*
             * Only let ES_CHECKQUEUEONCE be done on one window but
             * don't stop the wCmd from getting to all the other
             * windows.
             */
            if (SetEnableState(pcoi, pees->wCmd2) &&
                    pees->wCmd2 == EC_CHECKQUEUEONCE) {
                pees->wCmd2 = 0;
            }
        }
        pcoi->cLocks--;
        if (pcoi->cLocks == 0 && pcoi->state & ST_FREE_CONV_RES_NOW) {
            FreeConversationResources(pcoi);
            break;
        }
    }
    return (TRUE);
}


/***************************************************************************\
* DdeEnableCallback (DDEML API)
*
* Description:
* Turns on and off asynchronous callbacks (BLOCKABLE).
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL DdeEnableCallback(
DWORD idInst,
HCONV hConv,
UINT wCmd)
{
    BOOL fRet = FALSE;
    PCL_INSTANCE_INFO pcii;
    PCONV_INFO pcoi;
    ENABLE_ENUM_STRUCT ees;

    EnterDDECrit;

    pcii = (PCL_INSTANCE_INFO)ValidateInstance((HANDLE)LongToHandle( idInst ));
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    switch (wCmd) {
    case EC_QUERYWAITING:
    case EC_DISABLE:
    case EC_ENABLEONE:
    case EC_ENABLEALL:
        break;

    default:
        SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    if (hConv) {
        pcoi = (PCONV_INFO)ValidateCHandle((HANDLE)hConv,
                HTYPE_CLIENT_CONVERSATION, InstFromHandle(idInst));
        if (pcoi == NULL) {
            pcoi = (PCONV_INFO)ValidateCHandle((HANDLE)hConv,
                    HTYPE_SERVER_CONVERSATION, InstFromHandle(idInst));
        }
        if (pcoi == NULL) {
            SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
            goto Exit;
        }
        pcoi->cLocks++;
        fRet = SetEnableState(pcoi, wCmd);
        switch (wCmd) {
        case EC_ENABLEALL:
        case EC_ENABLEONE:
            CheckForQueuedMessages(pcoi);
        }
        pcoi->cLocks--;
        if (pcoi->cLocks == 0 && pcoi->state & ST_FREE_CONV_RES_NOW) {
            FreeConversationResources(pcoi);
        }
    } else {
        if (wCmd == EC_ENABLEONE) {
            wCmd = EC_ENABLEONEOFALL;
        }
        switch (wCmd) {
        case EC_ENABLEONEOFALL:
            pcii->ConvStartupState = ST_BLOCKNEXT | ST_BLOCKALLNEXT;
            break;

        case EC_DISABLE:
            pcii->ConvStartupState = ST_BLOCKED;
            break;

        case EC_ENABLEALL:
            pcii->ConvStartupState = 0;
            break;
        }
        ees.pfRet = &fRet;
        ees.wCmd = (WORD)wCmd;
        switch (wCmd) {
        case EC_ENABLEALL:
            ees.wCmd2 = EC_CHECKQUEUE;
            break;

        case EC_ENABLEONEOFALL:
            ees.wCmd2 = EC_CHECKQUEUEONCE;
            break;

        default:
            ees.wCmd2 = 0;
        }
        EnumChildWindows(pcii->hwndMother, (WNDENUMPROC)EnableEnumProc,
                (LPARAM)&ees);
    }

Exit:
    LeaveDDECrit;
    return (fRet);
}



/***************************************************************************\
* SetEnableState
*
* Description:
* Sets the given conversation's enable state based on the EC_ flag
* given.
*
* Returns: fSuccess/fProcessed.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL SetEnableState(
PCONV_INFO pcoi,
UINT wCmd)
{
    BOOL fRet = TRUE;

    switch (wCmd) {
    case EC_CHECKQUEUEONCE:
    case EC_CHECKQUEUE:
        fRet = CheckForQueuedMessages(pcoi);
        break;

    case EC_QUERYWAITING:
        fRet = !(pcoi->dmqOut == NULL ||
                (pcoi->dmqOut->next == NULL &&
                GetClientInfo()->CI_flags & CI_PROCESSING_QUEUE));
        break;

    case EC_DISABLE:
        pcoi->state |= ST_BLOCKED;
        pcoi->state &= ~(ST_BLOCKNEXT | ST_BLOCKALLNEXT);
        break;

    case EC_ENABLEONE:
        pcoi->state &= ~ST_BLOCKED;
        pcoi->state |= ST_BLOCKNEXT;
        break;

    case EC_ENABLEONEOFALL:
        pcoi->state &= ~ST_BLOCKED;
        pcoi->state |= (ST_BLOCKNEXT | ST_BLOCKALLNEXT);
        break;

    case EC_ENABLEALL:
        pcoi->state &= ~(ST_BLOCKED | ST_BLOCKNEXT | ST_BLOCKALLNEXT);
        break;

    default:
        return(FALSE);
    }
    return (fRet);
}




/***************************************************************************\
* _ClientGetDDEHookData
*
* Description:
* Callback from server to extract data from lParam and place it into
* the pdmhd for use by DDESPY apps. This does a very similar thing
* to the CopyDDEDataIn/Out apis but this only grabs a limited amount
* of the data suitable for posting to the DDESPY app(s). This should
* be merged with the Copy APIs eventually.
*
* History:
* 12-16-91 sanfords Created.
\***************************************************************************/
DWORD _ClientGetDDEHookData(
UINT message,
LPARAM lParam,
PDDEML_MSG_HOOK_DATA pdmhd)
{
    PBYTE pb;
    HANDLE hDDE;

    UnpackDDElParam(message, lParam, &pdmhd->uiLo, &pdmhd->uiHi);
    switch (message) {
    case WM_DDE_DATA:
    case WM_DDE_POKE:
    case WM_DDE_ADVISE:
        hDDE = (HANDLE)pdmhd->uiLo;
        break;

    case WM_DDE_EXECUTE:
        hDDE = (HANDLE)pdmhd->uiHi;
        break;

    case WM_DDE_ACK:
    case WM_DDE_REQUEST:
    case WM_DDE_UNADVISE:
    case WM_DDE_TERMINATE:
        pdmhd->cbData = 0;
        return (1);
    }

    pdmhd->cbData = (DWORD)UserGlobalSize(hDDE);
    if (pdmhd->cbData) {
        USERGLOBALLOCK(hDDE, pb);
        if (pb == NULL) {
            pdmhd->cbData = 0;
        } else {
            RtlCopyMemory(&pdmhd->Data, pb, min(pdmhd->cbData,
                    sizeof(DDEML_MSG_HOOK_DATA) -
                    FIELD_OFFSET(DDEML_MSG_HOOK_DATA, Data)));
            USERGLOBALUNLOCK(hDDE);
        }
    }
    return (1);
}
