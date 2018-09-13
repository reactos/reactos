//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       msgreply.cxx
//
//  Contents:   Reply functionality for PadMessage.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_MSG_HXX_
#define X_MSG_HXX_
#include "msg.hxx"
#endif

#ifndef X_MSGCIDX_H_
#define X_MSGCIDX_H_
#include "msgcidx.h"
#endif

//szRE_PREFIX and szFW_PREFIX have to have the same length
char szRE_PREFIX[] = "RE: ";
char szFW_PREFIX[] = "FW: ";


#define EXCLUDED_PROPS_ON_REPLY     33

SizedSPropTagArray (EXCLUDED_PROPS_ON_REPLY, sptExcludedProps) =
{
    EXCLUDED_PROPS_ON_REPLY,
    {
        PR_SENDER_NAME,
        PR_SENDER_ENTRYID,
        PR_SENDER_SEARCH_KEY,
        PR_SENDER_EMAIL_ADDRESS,
        PR_SENDER_ADDRTYPE,

        PR_RECEIVED_BY_NAME,
        PR_RECEIVED_BY_ENTRYID,
        PR_RECEIVED_BY_SEARCH_KEY,

        PR_SENT_REPRESENTING_NAME,
        PR_SENT_REPRESENTING_ENTRYID,
        PR_SENT_REPRESENTING_SEARCH_KEY,
        PR_SENT_REPRESENTING_EMAIL_ADDRESS,
        PR_SENT_REPRESENTING_ADDRTYPE,

        PR_RCVD_REPRESENTING_NAME,
        PR_RCVD_REPRESENTING_ENTRYID,
        PR_RCVD_REPRESENTING_SEARCH_KEY,

        PR_MESSAGE_FLAGS,
        PR_MESSAGE_RECIPIENTS,

        PR_READ_RECEIPT_ENTRYID,
        PR_REPORT_ENTRYID,

        PR_REPLY_RECIPIENT_ENTRIES,
        PR_REPLY_RECIPIENT_NAMES,

        PR_PARENT_KEY,

        PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED,

        PR_READ_RECEIPT_REQUESTED,

        PR_CLIENT_SUBMIT_TIME,
        PR_MESSAGE_DELIVERY_TIME,
        PR_MESSAGE_DOWNLOAD_TIME,

        PR_BODY,
        PR_SUBJECT,
        PR_SUBJECT_PREFIX,
        PR_MESSAGE_ATTACHMENTS,

        PR_HTML_BODY,
    }
};


//+---------------------------------------------------------------------------
//
//  Member:     CPadDoc::Reply
//
//  Synopsis:   Create a reply message and open a form on it
//
//----------------------------------------------------------------------------

HRESULT CPadMessage::Reply(eREPLYTYPE eReplyType, HWND hwndParent, LPCRECT prect)
{
    HRESULT             hr;
    LONG                cb;
    char *              szSubject = NULL;
    SPropValue          val[2] = {0};
    LPSPropProblemArray pProblems = NULL;
    LPMAPIFORM          pfrmReply = NULL;
    LPPERSISTMESSAGE    ppermsg = NULL;
    LPMAPIMESSAGESITE   pmsgsite = NULL;
    LPMAPIVIEWCONTEXT   pviewctx = NULL;
    LPMESSAGE           pmsg = NULL;
    ULONG               cbNewConvIdx = 0;
    LPBYTE              lpbNewConvIdx = NULL;
    IStream *           pStm;

    Assert(_pmsg);

    // Create new form

    hr = Factory.CreateInstance(NULL, IID_IMAPIForm, (LPVOID FAR *) &pfrmReply);
    if (hr)
    {
        g_LastError.SetLastError(E_OUTOFMEMORY);
        goto err;
    }

    hr = pfrmReply->QueryInterface(IID_IPersistMessage, (LPVOID *) &ppermsg);
    if (hr)
    {
        g_LastError.SetLastError(hr, pfrmReply);
        goto err;
    }

    // Create new message for reply

    hr = _pmsgsite->NewMessage(FALSE, NULL, ppermsg, &pmsg, &pmsgsite, &pviewctx);
    if (hr)
    {
        g_LastError.SetLastError(hr, _pmsgsite);
        goto err;
    }

    // Copy message to reply to into the new message

    hr = _pmsg->CopyTo(0, NULL, (LPSPropTagArray)&sptExcludedProps,
                        0, NULL, &IID_IMessage, pmsg, 0, &pProblems);
    if (hr)
    {
        g_LastError.SetLastError(hr, _pmsg);
        goto err;
    }

    if (pProblems)
    {
        //DebugTraceProblems("SmplForm: CopyTo returned ...", pProblems);
        //  if any of the errors is other than MAPI_E_COMPUTED, fail
        for(UINT ind = 0; ind < pProblems->cProblem; ++ind)
        {
            if (MAPI_E_COMPUTED != pProblems->aProblem[ind].scode)
            {
                hr = g_LastError.SetLastError(pProblems->aProblem[ind].scode);
                MAPIFreeBuffer(pProblems);
                pProblems = NULL;
                goto err;

            }
        }
        MAPIFreeBuffer(pProblems);
        pProblems = NULL;
    }

    // Compose reply / forward subject

    if (_pval && _pval[irtNormSubject].ulPropTag == PR_NORMALIZED_SUBJECT_A)
        cb = lstrlenA(_pval[irtNormSubject].Value.lpszA);
    else
        cb = 0;

    hr = MAPIAllocateBuffer(cb+lstrlenA(szRE_PREFIX)+1, (LPVOID FAR *) &szSubject);
    if (hr)
    {
        g_LastError.SetLastError(E_OUTOFMEMORY);
        goto err;
    }

    *szSubject = '\0';

    if (eREPLY == eReplyType || eREPLY_ALL == eReplyType)
    {
        lstrcatA(szSubject, szRE_PREFIX);
    }
    else
    {
        lstrcatA(szSubject, szFW_PREFIX);
    }

    if (cb > 0)
    {
        lstrcatA(szSubject, _pval[irtNormSubject].Value.lpszA);
    }

    val[0].Value.lpszA = szSubject;
    val[0].ulPropTag = PR_SUBJECT_A;

    // Compose reply / forward HTML body

    hr = pmsg->OpenProperty(PR_HTML_BODY, &IID_IStream,
                            STGM_READWRITE, MAPI_CREATE | MAPI_MODIFY,
                            (LPUNKNOWN FAR *) &pStm);
    if (hr)
    {
        g_LastError.SetLastError(hr, pmsg);
        goto err;
    }

    hr = ComposeReply(pStm);
    if (hr)
    {
        g_LastError.SetLastError(hr, pmsg);
        goto err;
    }

    /*
     * Create a conversation index for the reply msg based on that of ours
     *
     */
    if (!ScAddConversationIndex(_cbConvIdx, _lpbConvIdx,
                                &cbNewConvIdx, &lpbNewConvIdx))
    {
        val[1].ulPropTag = PR_CONVERSATION_INDEX;
        val[1].Value.bin.cb = cbNewConvIdx;
        val[1].Value.bin.lpb = lpbNewConvIdx;
    }
    else
    {
        val[1].ulPropTag = PR_NULL;
    }

    hr = pmsg->SetProps(2, val, &pProblems);

    MAPIFreeBuffer(lpbNewConvIdx);
    lpbNewConvIdx = NULL;

    MAPIFreeBuffer(szSubject);
    szSubject = NULL;

    if (hr)
    {
        g_LastError.SetLastError(hr, pmsg);
        goto err;
    }

    Assert(!pProblems);

#ifdef NEVER
    // if it's a reply, set the addressee
    if (eREPLY == eReplyType || eREPLY_ALL == eReplyType)
    {
        LPADRLIST pal = NULL;

        hr = MAPIAllocateBuffer(CbNewADRLIST(1), (LPVOID FAR *)&pal);
        if (hr)
        {
            _lsterr.SetLastError(E_OUTOFMEMORY);
            goto err;
        }
        hr = _pmsg->GetProps((LPSPropTagArray) &sptSender, 0, &cVal, &pval);
        if (hr) //treat warnings as an error, 'cause the props we ask for are required
        {
            _lsterr.SetLastError(hr, _pmsg);
            MAPIFreeBuffer(pal);
            goto err;
        }

        Assert(cVal == eDim);

        pval[eRecipType].ulPropTag = PR_RECIPIENT_TYPE;
        pval[eRecipType].Value.l = MAPI_TO;

        Assert(pval[eName].ulPropTag == PR_SENDER_NAME_A);
        pval[eName].ulPropTag = PR_DISPLAY_NAME_A;

        Assert(pval[eAddrType].ulPropTag == PR_SENDER_ADDRTYPE);
        pval[eAddrType].ulPropTag = PR_ADDRTYPE;

        Assert(pval[eEID].ulPropTag == PR_SENDER_ENTRYID);
        pval[eEID].ulPropTag = PR_ENTRYID;

        pal->aEntries[0].rgPropVals = pval;

        pal->cEntries = 1;
        pal->aEntries[0].cValues = eDim;

        hr = pmsg->ModifyRecipients(0, pal);
        FreePadrlist(pal); //this will also free pval
        pal = NULL;
        pval = NULL;
        if (hr)
        {
            _lsterr.SetLastError(hr, pmsg);
            goto err;
        }

        if (eReplyType == eREPLY_ALL)
        {
            hr = THR(MungeRecipients(pmsg, TRUE));
            if (hr)
            {
                _lsterr.SetLastError(hr, pmsg);
                goto err;
            }
        }
    }
#endif

    // Set list of recipients

    hr = THR(SetReplyForwardRecipients(pmsg, eReplyType));
    if (hr)
    {
        g_LastError.SetLastError(hr, pmsg);
        goto err;
    }

    // Now load and open new message

    hr = ppermsg->Load(pmsgsite, pmsg, 0, MSGFLAG_UNSENT );
    if (hr)
    {
        g_LastError.SetLastError(hr, ppermsg);
        goto err;
    }

    hr = pfrmReply->DoVerb(EXCHIVERB_OPEN, pviewctx, (ULONG)hwndParent, prect);
    if (hr)
    {
        g_LastError.SetLastError(hr, pfrmReply);
        pfrmReply->ShutdownForm(SAVEOPTS_NOSAVE);
        goto err;
    }

err:
    ReleaseInterface(pfrmReply);
    ReleaseInterface(ppermsg);
    ReleaseInterface(pmsgsite);
    ReleaseInterface(pviewctx);
    ReleaseInterface(pmsg);
    ReleaseInterface(pStm);
    RRETURN(hr);
}


/*
 *  CPadMessage::ZapIfMatch
 *
 *  Purpose:
 *      Compare and optionally zap address entry
 *
 *  Arguments:
 *      pae             Address entry to be zapped
 *      fSearchKey      Whether to use search key to compare
 *      cbFrom          What to compare against
 *      pbFrom          What to compare against
 *      fZap            Whether to zap the entry if it matches
 *      pfMatch         If non-null, indicates if match found
 *
 *  Returns:
 *      SCODE           The status
 */
HRESULT
CPadMessage::ZapIfMatch(
    LPADRENTRY pae,
    BOOL fSearchKey,
    ULONG cbFrom,
    LPBYTE pbFrom,
    BOOL fZap,
    LPBOOL pfMatch)
{
    ULONG           ulResult = 0;
    LPSPropValue    pval;

    if (!pae->rgPropVals)
        return S_OK;

    if (fSearchKey)
    {
        // Get the search key from the address entry and check if this is From.
        // Handle lame MAPI providers that don't give us one....even though it's
        // a required property!
        pval = PvalFind((LPSRow) pae, PR_SEARCH_KEY);
        if (pval)
            ulResult = (cbFrom == pval->Value.bin.cb) &&
                       !memcmp(pbFrom, pval->Value.bin.lpb, (size_t) cbFrom);
    }
    else    // Entry id
    {
        // Get the entry ID from the address entry and check if this is From.
        // Handle lame MAPI providers that don't give us one....even though it's
        // a required property!
        pval = PvalFind((LPSRow) pae, PR_ENTRYID);
        if (pval)
            _pses->CompareEntryIDs(cbFrom, (LPENTRYID) pbFrom,
                pval->Value.bin.cb, (LPENTRYID) pval->Value.bin.lpb, 0, &ulResult);
    }

    // If this is the From:, don't add it if the caller so desires
    if (fZap && ulResult)
    {
        MAPIFreeBuffer(pae->rgPropVals);
        pae->cValues = 0;
        pae->rgPropVals = NULL;
    }

    if (pfMatch)
    {
        // Tell the caller if match was found
        *pfMatch = !!ulResult;
    }

    return S_OK;
}


/*
 *  CPadMessage::GetReplieeAdrEntry
 *
 *  Purpose:
 *      Gets the repliee's address entry.
 *
 *  Arguments:
 *      pae             The resulting address entry
 */
static INT  rgivalSender[] =
{
    irtSenderName,
    irtSenderAddrType,
    irtSenderEntryid,
    irtSenderEmailAddress,
    irtSenderSearchKey,
};

static ULONG rgulPropTagRepAdrEntry[] =
{
    PR_DISPLAY_NAME,
    PR_ADDRTYPE,
    PR_ENTRYID,
    PR_EMAIL_ADDRESS,
    PR_SEARCH_KEY
};

#define cvalReplieeAdrEntry (sizeof(rgulPropTagRepAdrEntry) / sizeof(rgulPropTagRepAdrEntry[0]))

HRESULT
CPadMessage::GetReplieeAdrEntry(LPADRENTRY pae)
{
    HRESULT     hr;
    INT         itaga;
    SPropValue  rgval[cvalReplieeAdrEntry + 1];
    ADRENTRY    ae;

    // Capone 11811 Forgot to include recipient type in cValues
    ae.cValues = cvalReplieeAdrEntry + 1;
    ae.rgPropVals = rgval;

    // Set the recipient type
    rgval[0].ulPropTag = PR_RECIPIENT_TYPE;
    rgval[0].Value.ul = MAPI_TO;

    // Copy the individual props to build an adrentry
    // Capone 11811 Skip the recipient type property
    for (itaga = 1; itaga < cvalReplieeAdrEntry + 1; ++itaga)
    {
        rgval[itaga] = _pval[rgivalSender[itaga - 1]];
        rgval[itaga].ulPropTag =
            PROP_TAG(PROP_TYPE(rgval[itaga].ulPropTag),
                        PROP_ID(rgulPropTagRepAdrEntry[itaga - 1]));
    }

    // Copy the adrentry to the real destination
    hr = CopyRow(NULL, (LPSRow) &ae, (LPSRow) pae);

    RRETURN(hr);
}


/*
 *  CPadMessage::SetReplieeEntryId
 *
 *  Purpose:
 *      Adds the address entry into the address list.
 *
 *  Arguments:
 *      pal             The address list
 *      cbEid           The entry id size
 *      peid            The entry id
 *      cch             The name size
 *      rgch            The name
 */
HRESULT
CPadMessage::SetReplieeEntryId(
    LPADRLIST * ppal,
    ULONG cbEid,
    LPENTRYID peid,
    LPSTR szName,
    ULONG * piRepliee)
{
    HRESULT         hr;
    ULONG           cchName;
    ULONG           cbRepliee;
    ADRENTRY        aeRepliee;
    LPSPropValue    rgval = NULL;
    LPBYTE          pbEid;
    char*           pbch;

    // Allocate memory for PropVal array of 3 and byte buffer
    // for the entry id and display name.
    cchName = lstrlenA(szName) + 1;
    cbRepliee = 3 * sizeof(SPropValue) + cbEid + cchName;
    hr = THR(MAPIAllocateBuffer(cbRepliee, (void**)&rgval));
    if (hr)
    {
        g_LastError.SetLastError(hr);
        goto Cleanup;
    }

    // Put entry id into address entry
    pbEid = (LPBYTE) &rgval[3];
    memcpy(pbEid, peid, (size_t) cbEid);
    rgval[0].ulPropTag = PR_ENTRYID;
    rgval[0].Value.bin.cb = cbEid;
    rgval[0].Value.bin.lpb = pbEid;

    // Put display name into address entry
    pbch = (char*)(pbEid + cbEid);
    lstrcpyA(pbch, szName);
    rgval[1].ulPropTag = PR_DISPLAY_NAME;
    rgval[1].Value.lpszA = pbch;

    // Put recipient type into address entry
    rgval[2].ulPropTag = PR_RECIPIENT_TYPE;
    rgval[2].Value.l = MAPI_TO;

    aeRepliee.cValues = 3;
    aeRepliee.rgPropVals = rgval;

    // Add the sender to the To: field
    hr = THR(AddRecipientToAdrlist(ppal, &aeRepliee, piRepliee));

    // I'm not supposed to free rgval here!

Cleanup:
    RRETURN(hr);
}


/*
 *  ScSetReplyForwardRecipients
 *
 *  Purpose:
 *      Sets up recipient list for a new message that is being replied to or
 *      forwarded.
 *
 *  Arguments:
 *      pmsgResend      reply message
 *
 *  Returns:
 *      SCODE           The status
 *
 *  Notes:
 *      Since the original recipient list was excluded when this reply
 *      message was created, we should only include things we want.
 *      Below, "Replier" means the user replying, and Replyee means the
 *      person to whom the reply is sent (normally From, but can be overrided).
 */
HRESULT
CPadMessage::SetReplyForwardRecipients(LPMESSAGE pmsgResend, eREPLYTYPE eReplyType)
{
    HRESULT         hr = S_OK;
    LPADRLIST       pal = NULL;
    LPSPropValue    pval = NULL;
    ULONG           iae = 0;
    LPADRENTRY      pae = NULL;
    LPSPropValue    pvalRepliee;
    LPSPropValue    pvalReplieeSearchKey = NULL;
    LPSPropValue    pvalReplier;

    LPFLATENTRYLIST pfel = NULL;
    LPFLATENTRY     pfe;
    LPBYTE          pbfe;
    ULONG           ife;

    INT             cRepliee = 0;
    ULONG *         rgiRepliee = NULL;
    INT             iRepliee;

    LPSPropValue    pvalRepRecNames;
    LPSTR           szRepRecNames = NULL;

    if (eReplyType == eREPLY_ALL)
    {
        // Reply all - Get recipient rows
        hr = THR(GetMsgAdrlist(_pmsg, (LPSRowSet*)&pal, &g_LastError));
        if (hr)
        {
            goto Cleanup;
        }

        Assert(pal);
    }

    // Get name(s) for To: field from PR_REPLY_RECIPIENT_ENTRIES (if present)

    // Find out who is (are) the repliee(s)
    pvalRepliee = &_pval[irtReplyRecipientEntries];
    if (pvalRepliee->ulPropTag != PR_REPLY_RECIPIENT_ENTRIES)
    {
        AssertSz(pvalRepliee->Value.err == MAPI_E_NOT_FOUND, "No reply_recipient_entries");

        // Use Sender_XXX

        if (_pval[irtSenderSearchKey].ulPropTag
                                            == PR_SENDER_SEARCH_KEY)
        {
            pvalReplieeSearchKey = &_pval[irtSenderSearchKey];
            cRepliee = 1;
        }
    }
    else
    {
        // We have Reply_Recipient_Entries

        pfel = (LPFLATENTRYLIST) pvalRepliee->Value.bin.lpb;
        Assert(pvalRepliee->Value.bin.cb == CbFLATENTRYLIST(pfel));
        cRepliee = (INT) pfel->cEntries;
    }

    // Exchange² 29007
    // Expand fix for Exchange 10764 to Reply_Recipient_Entries.
    if (cRepliee)
    {
        rgiRepliee = (ULONG *) new ULONG[cRepliee];
        if (!rgiRepliee)
        {
            hr = _lsterr.SetLastError(E_OUTOFMEMORY);
            goto Cleanup;
        }
    }

    pvalReplier = &_pval[irtReceivedBySearchKey];
    if (pvalReplier->ulPropTag != PR_RECEIVED_BY_SEARCH_KEY)
    {
        pvalReplier = NULL;
    }

    // Put the current Repliee (From) or the Reply_Recipient_Entries
    // into the To well for Reply and Reply All

    if (eReplyType != eFORWARD)
    {
        if (pfel)   // Reply_Recipient_Entries
        {
            char * pchName;     // Current name
            char * pchNext;     // Next name
#if DBG == 1
            char * pchFirst;    // First name
            ULONG cchTotal;     // Length of names string
#endif // DEBUG

            // Get the reply_recipient_names
            pvalRepRecNames = &_pval[irtReplyRecipientNames];
            if (pvalRepRecNames->ulPropTag != PR_REPLY_RECIPIENT_NAMES)
              //$ BUGBUG -- Should this report an error?
                goto Cleanup;

            // Initialize vars
            szRepRecNames = new char(lstrlenA(pvalRepRecNames->Value.lpszA)+1);
            lstrcpyA(szRepRecNames, pvalRepRecNames->Value.lpszA);
            pchName = pchNext = szRepRecNames;
#if DBG == 1
            pchFirst = szRepRecNames;
            cchTotal = lstrlenA(szRepRecNames);
#endif // DEBUG

            // Loop through the reply_recipient_entries
            for (ife = 0, pbfe = pfel->abEntries;
                    ife < pfel->cEntries;
                    ife++,
                    pbfe += PAD4(CbFLATENTRY((LPFLATENTRY) pbfe)))
            {
                pfe = (LPFLATENTRY) pbfe;

                // Find the end of the current display name
                while (*pchNext != TEXT(';') && *pchNext != TEXT('\0'))
                {
#if DBG == 1
                    Assert((ULONG) (pchNext - pchFirst) < cchTotal);
#endif
                    pchNext++;
                }

                // Stringify the display name of the current entry id
                if (*pchNext == TEXT(';'))
                {
                    *pchNext = TEXT('\0');
                    pchNext++;
                }
#if DBG == 1
                else    // End of string; Last name for last entry
                {
                    Assert(ife + 1 == pfel->cEntries);
                }
#endif // DEBUG

                // Set the current entry id as a repliee
                hr = THR(SetReplieeEntryId(&pal, pfe->cb,
                        (LPENTRYID) pfe->abEntry, pchName, &rgiRepliee[ife]));
                if (hr)
                    goto Cleanup;

                // Start of next display name
                pchName = pchNext;
            }
        }
        else    // No Reply_Recipient_Entries
        {
            ADRENTRY        aeRepliee;

            // Pull out the values
            hr = THR(GetReplieeAdrEntry(&aeRepliee));
            if (hr)
                goto Cleanup;

            // Exchange² 6756
            // Handle a reply or reply all message with an "empty" address
            // list because the original message did not have a sender.
            if (PvalFind((LPSRow) &aeRepliee, PR_ENTRYID) ||
                PvalFind((LPSRow) &aeRepliee, PR_DISPLAY_NAME))
            {
                hr = THR(AddRecipientToAdrlist(&pal, &aeRepliee, rgiRepliee));
                if (hr)
                    goto Cleanup;
            }
            else
            {
                MAPIFreeBuffer(aeRepliee.rgPropVals);
            }
        }
    }

    // Remove Replier and Repliee from the wells. (Capone spec 1.12 8-42)

    if (eReplyType == eREPLY_ALL)
    {
        // Remove Replier (Me) from the To well.
        // Remove Replier (Me) and Repliee (From) from the Cc well.

        LPADRENTRY  paeMeInToList = NULL;
        ULONG       cEntriesInToList = 0;

        for (iae = 0, pae = pal->aEntries; iae < pal->cEntries; iae++, pae++)
        {
            // Can't let the store see a TRUE PR_RESPONSIBLITY here
            pval = PvalFind((LPSRow) pae, PR_RESPONSIBILITY);
            if (pval)
                pval->Value.b = FALSE;

            pval = PvalFind((LPSRow) pae, PR_RECIPIENT_TYPE);
            AssertSz(pval, "Address list entry w/o recipient type");
            // Capone 12398 Another missing required property!
            if (!pval)
            {
                // Kill and skip this entry
                MAPIFreeBuffer(pae->rgPropVals);
                pae->cValues = 0;
                pae->rgPropVals = NULL;
                continue;
            }

            switch (pval->Value.ul)
            {
            case MAPI_BCC:
                // Kill Bcc always
                MAPIFreeBuffer(pae->rgPropVals);
                pae->cValues = 0;
                pae->rgPropVals = NULL;
                break;

            case MAPI_CC:
                if (pfel)   // Reply_Recipient_Entries
                {
                    for (ife = 0, pbfe = pfel->abEntries;
                            ife < pfel->cEntries;
                            ife++,
                            pbfe += PAD4(CbFLATENTRY((LPFLATENTRY) pbfe)))
                    {
                        pfe = (LPFLATENTRY) pbfe;

                        // If this is the Repliee (From), have it removed
                        hr = THR(ZapIfMatch(pae, FALSE,
                                    pfe->cb, pfe->abEntry, TRUE, NULL));
                        if (hr)
                            goto Cleanup;
                    }
                }
                else if (pvalReplieeSearchKey)  // No Reply_Recipient_Entries
                {
                    // If this is the Repliee (From), have it removed
                    hr = THR(ZapIfMatch(pae, TRUE,
                            pvalReplieeSearchKey->Value.bin.cb,
                            pvalReplieeSearchKey->Value.bin.lpb, TRUE, NULL));
                    if (hr)
                        goto Cleanup;
                }

                // If this is the Replier (Me), have it removed
                if (pvalReplier)
                {
                    hr = THR(ZapIfMatch(pae, TRUE,
                        pvalReplier->Value.bin.cb, pvalReplier->Value.bin.lpb,
                        TRUE, NULL));
                    if (hr)
                        goto Cleanup;
                }
                break;

            case MAPI_TO:
                // If this is the Replier (Me), see if it needs to be
                // removed.
                // RAID 10764: Duplicate Repliees also need to be
                // removed from the To: list.
                // We know where the one we added is - we keep it

                // Exchange² 29007
                // Expand fix for Exchange 10764 to Reply_Recipient_Entries.
                for (iRepliee = 0; iRepliee < cRepliee; iRepliee++)
                    if (iae == (ULONG) rgiRepliee[iRepliee])
                        break;

                if (cRepliee && (iRepliee == cRepliee))
                {
                    BOOL fMatch = FALSE;

                    if (pfel)   // Reply_Recipient_Entries
                    {
                        for (ife = 0, pbfe = pfel->abEntries;
                                ife < pfel->cEntries;
                                ife++,
                                pbfe += PAD4(CbFLATENTRY((LPFLATENTRY) pbfe)))
                        {
                            pfe = (LPFLATENTRY) pbfe;

                            // If this is the Repliee (From), have it removed
                            hr = THR(ZapIfMatch(pae, FALSE,
                                        pfe->cb, pfe->abEntry, TRUE, &fMatch));
                            if (hr)
                                goto Cleanup;
                        }
                    }
                    else if (pvalReplieeSearchKey)  // No Reply_Recipient_Entries
                    {
                        // If this is the Repliee (From), have it removed
                        hr = THR(ZapIfMatch(pae, TRUE,
                                pvalReplieeSearchKey->Value.bin.cb,
                                pvalReplieeSearchKey->Value.bin.lpb, TRUE, &fMatch));
                        if (hr)
                            goto Cleanup;
                    }

                    // Current entry zapped so move on to the next one
                    if (fMatch)
                        break;
                }

                if (pvalReplier)
                {
                    BOOL fMatch;

                    if (paeMeInToList)
                    {
                        // We have found the replier in the To: well,
                        // if there are any more, they need to be zapped.
                        hr = THR(ZapIfMatch(pae, TRUE,
                                pvalReplier->Value.bin.cb,
                                pvalReplier->Value.bin.lpb,
                                TRUE /*fZap*/, &fMatch));
                        if (hr)
                            goto Cleanup;

                        // If entry did not get zapped, bump the To: entry
                        // count.
                        if (!fMatch)
                            cEntriesInToList++;
                    }
                    else
                    {
                        // See if we have the replier here. If so,
                        // remember the entry, but don't zap it yet.
                        hr = THR(ZapIfMatch(pae, TRUE,
                                pvalReplier->Value.bin.cb,
                                pvalReplier->Value.bin.lpb,
                                FALSE, &fMatch));
                        if (hr)
                            goto Cleanup;

                        if (fMatch)
                            paeMeInToList = pae;

                        // Match or no match, the entry is still in the
                        // To list, so bump our To list count.
                        cEntriesInToList++;
                    }
                }

                break;
            }
        }

        // If Replier (Me) is in TO well, and there are others too,
        // remove the Replier. The only case the Replier does not get
        // removed is when it's the only one in the list.

        if (paeMeInToList && cEntriesInToList > 1)
        {
            // Kill Replier's entry in the list
            MAPIFreeBuffer(paeMeInToList->rgPropVals);
            paeMeInToList->cValues = 0;
            paeMeInToList->rgPropVals = NULL;
        }
    }

    // Set new list of recipients

    if (pal && pal->cEntries)
    {
        // Record changes in recipient table
        hr = THR(pmsgResend->ModifyRecipients(MODRECIP_ADD, pal));
        if (hr)
		{
			g_LastError.SetLastError(hr, pmsgResend);
			goto Cleanup;
		}
	}

Cleanup:
    delete rgiRepliee;
    delete szRepRecNames;
    RRETURN(hr);
}

///     GetMsgAdrlist
//    retrieves recipients adrlist of a message
HRESULT GetMsgAdrlist (LPMESSAGE pmsg, LPSRowSet * ppRowSet, CLastError * plasterror)
{
    *ppRowSet = NULL;

    LPMAPITABLE pTable = NULL;
    HRESULT hr;

    hr = pmsg->GetRecipientTable (0, &pTable);
    if (!hr)
    {
        hr = HrQueryAllRows(pTable, NULL, NULL, NULL, 0, ppRowSet);
        if (hr)
        {
            plasterror->SetLastError(hr, pTable);
        }
    }
    else
    {
        plasterror->SetLastError(hr, pmsg);
    }

    pTable->Release();

    return hr;
}

HRESULT CPadMessage::ComposeReply(IStream * pStm)
{
    char *     pch;
    char *     pchStartBody;
    char *     pchReplyHeader = NULL;
    char       achTime[256];

    int        cbToWrite;
    int        cb;
    HRESULT    hr = S_OK;
    BOOL       fDeletePch = FALSE;

    if(_pObject && _pObject)
    {
        hr = THR(SaveAsString(&pch));
        if (hr)
            goto Cleanup;

        fDeletePch = TRUE;
    }
    else if (_pval && _pval[irtHtmlBody].ulPropTag == PR_HTML_BODY)
    {
        pch = _pval[irtHtmlBody].Value.lpszA;
    }
    else
    {
        hr = THR(GetHtmlBodyFromMsg(&pch));
        if (hr)
            goto Cleanup;

        fDeletePch = TRUE;
    }

    pchStartBody = strstr( pch, "<BODY");
    if(pchStartBody)
    {
        pchStartBody = strchr(pchStartBody,'>');
        if( pchStartBody == NULL )
        {
            pchStartBody = strstr( pch, "<body");
            pchStartBody = strchr(pchStartBody,'>');
            Assert(pchStartBody);
        }
        pchStartBody++;
    }
    else
    {
        pchStartBody = pch;
    }

    cbToWrite = pchStartBody - pch;

    if(cbToWrite)
    {
        hr = pStm->Write(pch,(ULONG) cbToWrite, NULL);
        if(hr)
            goto Cleanup;
    }

    // BUGBUG: chrisf - should properly define the length below instead of using 500
    // Need to be large enough to accomodate all litteral HTML below
    cb = 500;

    if (_pval[irtSenderName].ulPropTag == PR_SENDER_NAME_A)
         cb += lstrlenA(_pval[irtSenderName].Value.lpszA);

    if (_pval[irtTo].ulPropTag == PR_DISPLAY_TO_A)
         cb += lstrlenA(_pval[irtTo].Value.lpszA);

    if (_pval[irtCc].ulPropTag == PR_DISPLAY_CC_A)
         cb += lstrlenA(_pval[irtCc].Value.lpszA);

    if (_pval[irtSubject].ulPropTag == PR_SUBJECT_A)
         cb += lstrlenA(_pval[irtSubject].Value.lpszA);

    if (_pval[irtTime].ulPropTag == PR_CLIENT_SUBMIT_TIME)
    {
        FormatTime(&_pval[irtTime].Value.ft, achTime, sizeof(achTime));
        cb += lstrlenA(achTime);
    }
    else
    {
        achTime[0] = '\0';
    }

    pchReplyHeader = new char[cb];
    if(!pchReplyHeader)
        goto Cleanup;

    strcpy(pchReplyHeader,"<P>&nbsp<P><HR ALIGN=LEFT><ADDRESS><STRONG>From: </STRONG>");
    if (_pval[irtSenderName].ulPropTag == PR_SENDER_NAME_A)
        strcat(pchReplyHeader,_pval[irtSenderName].Value.lpszA);

    strcat(pchReplyHeader,"<BR><STRONG>Sent: </STRONG>");
    strcat(pchReplyHeader,achTime);

    strcat(pchReplyHeader,"<BR><STRONG>To: </STRONG>");
    if (_pval[irtTo].ulPropTag == PR_DISPLAY_TO_A)
        strcat(pchReplyHeader,_pval[irtTo].Value.lpszA);

    strcat(pchReplyHeader,"<BR><STRONG>Cc: </STRONG>");
    if (_pval[irtCc].ulPropTag == PR_DISPLAY_CC_A)
        strcat(pchReplyHeader,_pval[irtCc].Value.lpszA);

    strcat(pchReplyHeader,"<BR><STRONG>Subject: </STRONG>");
    if (_pval[irtSubject].ulPropTag == PR_SUBJECT_A)
        strcat(pchReplyHeader,_pval[irtSubject].Value.lpszA);

    strcat(pchReplyHeader,"<BR>&nbsp</ADDRESS>");

    cb = strlen(pchReplyHeader);

    hr = pStm->Write(pchReplyHeader,(ULONG) cb, NULL);
    if(hr)
        goto Cleanup;

    cb = strlen(pch);
    cbToWrite = cb - cbToWrite;

    hr = pStm->Write(pchStartBody,(ULONG) cbToWrite, NULL);
    if(hr)
        goto Cleanup;

Cleanup:
    delete pchReplyHeader;
    if (fDeletePch)
        delete pch;
    RRETURN(hr);
}

HRESULT
CPadMessage::SaveAsString(char ** ppchBuffer)
{
    IStream *   pStm;
    STATSTG     StatStg;
    LONG        cStmLength;
    char *      pch = NULL;
    HRESULT     hr;
    LARGE_INTEGER i64Start = {0, 0};

    hr = CreateStreamOnHGlobal(NULL, NULL, &pStm);
    if (hr)
        goto err;

    hr = CPadDoc::Save(pStm);
    if (hr)
        goto err;

    Verify(pStm->Stat(&StatStg,NULL) == S_OK);

    cStmLength = StatStg.cbSize.LowPart;

    pch = new char[cStmLength+1];
    if(!pch)
        goto err;

    hr = pStm->Seek(i64Start, STREAM_SEEK_SET, NULL);
    if (hr)
        goto err;

    hr = pStm->Read(pch, cStmLength, NULL);
    if (hr)
        goto err;

    pch[cStmLength] = '\0';

    *ppchBuffer = pch;

Cleanup:
    ReleaseInterface(pStm);
    RRETURN(hr);

err:
    delete pch;
    goto Cleanup;
}

HRESULT
CPadMessage::GetHtmlBodyFromMsg(char ** ppchBuffer)
{
    HRESULT     hr;
    STATSTG     StatStg;
    IStream *   pStm;
    LONG        cStmLength;
    char *      pch = NULL;
    LARGE_INTEGER i64Start = {0, 0};

    Assert(_pmsg);

    hr = THR(_pmsg->OpenProperty(PR_HTML_BODY, &IID_IStream,
                            STGM_READ, 0, (LPUNKNOWN FAR *) &pStm));

    if (hr)
        goto Cleanup;

    Verify(pStm->Stat(&StatStg,NULL) == S_OK);

    cStmLength = StatStg.cbSize.LowPart;

    pch = new char[cStmLength+1];
    if(!pch)
        goto err;

    hr = pStm->Seek(i64Start, STREAM_SEEK_SET, NULL);
    if (hr)
        goto err;

    hr = pStm->Read(pch, cStmLength, NULL);
    if (hr)
        goto err;

    pch[cStmLength] = '\0';

    *ppchBuffer = pch;

Cleanup:
    ReleaseInterface(pStm);
    RRETURN(hr);

err:
    delete pch;
    goto Cleanup;
}
