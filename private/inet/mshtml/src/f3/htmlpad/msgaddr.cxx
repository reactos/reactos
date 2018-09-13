//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       msgaddr.cxx
//
//  Contents:   Address list manipulation
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_MSG_HXX_
#define X_MSG_HXX_
#include "msg.hxx"
#endif

#ifndef X_MSGTRIPL_HXX_
#define X_MSGTRIPL_HXX_
#include "msgtripl.hxx"
#endif

#ifndef X_MSGGUID_H_
#define X_MSGGUID_H_
#include "msgguid.h"
#endif

#define		cchUnresolvedMax	256

typedef struct _editstreamcookie
{
	HRESULT         hr;
	LPADRLIST *		ppal;
	ULONG			cRecipTypes;
	ULONG			iRecipType;
	ULONG *			rgulDestComps;
	HWND *			rghwndEdit;
	LONG *			piae;
	char			rgch[cchUnresolvedMax];
	ULONG			cchBuf;
	BOOL			fTruncated;
	INT				nBrackets;
} EDITSTREAMCOOKIE;


static HRESULT AddUnresolvedName(EDITSTREAMCOOKIE * pesc);

static HRESULT SpecialAdrlistScan(LPADRLIST * ppal, LONG * piae,
				ULONG cRecipTypes, ULONG * rgulDestComps, BOOL fAboutToAdd);

static HRESULT GrowAdrlist(LPADRLIST * ppal, UINT caeToAdd);

static DWORD CALLBACK UnresolvedStream(DWORD dwCookie, LPBYTE pbBuff,
				LONG cb, LONG *pcb);



/*
 *	CPadMessage::DoCheckNames
 *
 *	Purpose:
 *		Implement Check Names command
 */
void CPadMessage::DoCheckNames()
{
    HRESULT hr;

    hr = THR_NOTRACE(GetAndCheckRecipients(TRUE));

    if (hr && hr != MAPI_E_USER_CANCEL)
        ShowError();
}


/*
 *	CPadMessage::GetAndCheckRecipients
 *
 *	Purpose:
 *		Resolves names for the message and get them into the _padrlist
 *
 *	Arguments:
 *		BOOL		Update the recipient wells?
 *
 *	Returns:
 *      hr          MAPI_E_USER_CANCEL if user cancelled operation
 */
HRESULT CPadMessage::GetAndCheckRecipients(BOOL fUpdateWells)
{
	HRESULT hr = S_OK;
	LPSPropValue pval;
	BOOL    fFoundFrom = FALSE;
	ULONG   iae;

	// Capone 4545
	// Name from resend note From well wiped out on Check Names
	if(_rghwndEdit[0])
		SendMessage(_rghwndEdit[0], EM_SETMODIFY, TRUE, 0);

	// Get the names out of the controls and into the adrlist structure
	hr = THR(ParseRecipients(TRUE));
	if (hr || !_padrlist)
		goto Cleanup;

	// Check if there is more than one name in the From well
	for (iae = 0; iae < _padrlist->cEntries; iae++)
	{
		// Ignore empty ADRENTRY's
		if (!_padrlist->aEntries[iae].rgPropVals)
			continue;

		pval = PvalFind((LPSRow) &_padrlist->aEntries[iae], PR_RECIPIENT_TYPE);
		AssertSz(pval, "ADRENTRY with no PR_RECIPIENT_TYPE found");
		if (pval)
		{
			if (pval->Value.ul == MAPI_ORIG)
			{
				if (fFoundFrom)
				{
					break;	// Already one found so there is more than one!
				}
				else
				{
					fFoundFrom = TRUE;
				}
			}
		}
	}

	// Early exit from loop indicates more than one name in From well
	if (iae < _padrlist->cEntries)
	{
		hr = g_LastError.SetLastError(E_FAIL);
		goto Cleanup;
	}

    // Open address book (_pab)
    hr = OpenAddrBook();
    if (hr)
    {
        ShowError();
        if (FAILED(hr))
            goto Cleanup;
    }

    // Ignore warnings 
    if (hr == MAPI_W_ERRORS_RETURNED)
        hr = S_OK;

    // Resolve recipient names
	hr = THR(_pab->ResolveName((ULONG)_hwnd, MAPI_DIALOG, NULL, _padrlist));
	if (FAILED(hr) && hr != MAPI_E_USER_CANCEL)
	{
		hr = g_LastError.SetLastError(hr, _pab);
		goto Cleanup;
	}

	// Get the names out of the adrlist structure and into the controls
	if (hr == MAPI_E_USER_CANCEL || fUpdateWells)
    {
		IGNORE_HR(DisplayRecipients(TRUE));
    }

Cleanup:

	// Remove From well type from _rghwndEdit and _rgulRecipTypes arrays

	// Capone 4311
	// Remove the From well entry from the _padrlist

	// Capone 10812
	// No resolve on send optimization (fUpdateWells == FALSE) would leave
	// MAPI_ORIG in the DIAL(_padrlist) so it can be used in ScNoteWriteMessage.

	if (fFoundFrom && (fUpdateWells || hr))
	{
		for (iae = 0; iae < _padrlist->cEntries; iae++)
		{
			// Ignore empty ADRENTRY's

			if (!_padrlist->aEntries[iae].rgPropVals)
				continue;

			pval = PvalFind((LPSRow) &_padrlist->aEntries[iae], PR_RECIPIENT_TYPE);
			AssertSz(pval, "ADRENTRY with no PR_RECIPIENT_TYPE found");
			if (pval && (pval->Value.ul == MAPI_ORIG))
			{
				_padrlist->aEntries[iae].cValues = 0;
				// Exchange² 30065 Memory leak
				MAPIFreeBuffer(_padrlist->aEntries[iae].rgPropVals);
				_padrlist->aEntries[iae].rgPropVals = NULL;
			}
		}
	}

    RRETURN1(hr, MAPI_E_USER_CANCEL);
}

/*
 *	ParseRecipients
 *	
 *	Purpose:
 *		Parse wells and build list of recipients
 *	
 *	Parameters:
 *		ppal		    returns pointer to list of recipients
 *      fIncludeFrom    include from well or not
 *	
 *	Returns:
 *		hr
 */
HRESULT CPadMessage::ParseRecipients(BOOL fIncludeFrom)
{
	int diRecipTypes = fIncludeFrom ? 0 : 1;

	RRETURN(THR(AddNamesToAdrlist(&_padrlist)));
}

/*
 *	DisplayRecipients
 *	
 *	Purpose:
 *		Display recipient list in the wells
 *	
 *	Parameters:
 *		pal				pointer to list of recipients
 *      fIncludeFrom    include from well or not
 *	
 *	Returns:
 *		hr
 */
HRESULT CPadMessage::DisplayRecipients(BOOL fIncludeFrom)
{
	HRESULT hr = S_OK;
	int i;
	int diRecipTypes = fIncludeFrom ? 0 : 1;
	HCURSOR hcursor;

	if (_padrlist)
	{
		hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

		for (i = (INT)_cRecipTypes - 1; i >= diRecipTypes; --i)
		{
			if(_rghwndEdit[i])
				SetWindowTextA(_rghwndEdit[i], "");
		}

		hr = THR(AddRecipientsToWells(_padrlist));

		SetCursor(hcursor);
	}

    RRETURN(hr);
}

/*
 *	AddRecipientsToWells
 *	
 *	Purpose:
 *		This function adds all the recipients in an ADRLIST
 *		to the recipient wells.
 *	
 *	Parameters:
 *		pal				pointer to list of recipients
 *	
 *	Returns:
 *		hr
 */
HRESULT CPadMessage::AddRecipientsToWells(LPADRLIST pal)
{
	HRESULT     	hr = S_OK;
	ULONG			iae;
	ULONG			i;
	LPSPropValue	pval;

	Assert(pal);
	Assert(_cRecipTypes > 0);

	for (iae = 0; iae < pal->cEntries; iae++)
	{
		// Ignore empty ADRENTRY's

		if (!pal->aEntries[iae].rgPropVals)
			continue;

		pval = PvalFind((LPSRow) &pal->aEntries[iae], PR_RECIPIENT_TYPE);
		AssertSz(pval, "ADRENTRY with no PR_RECIPIENT_TYPE found");
		if (pval)
		{
			for (i = 0; i < _cRecipTypes; ++i)
			{
				if (_rgulRecipTypes[i] == pval->Value.ul)
				{
					hr = THR(AddRecipientToWell(_rghwndEdit[i],
						 			&pal->aEntries[iae], TRUE, TRUE));
					if (hr != S_OK)
					{
						goto Cleanup;
					}

					break;
				}
			}
		}
	}

Cleanup:
	RRETURN(hr);
}


/*
 *	AddRecipientToWell
 *	
 *	Purpose:
 *		This function adds a recipient to a recipient well.
 *	
 *	Parameters:
 *		hwndEdit		hwnd of the recipient well to add the
 *						recipient to
 *		pae				pointer to an ADRENTRY
 *		fAddSemi		whether to add a semicolon between entries
 *		fCopyEntry		whether to copy the ADRENTRY or just use it
 *	
 *	Returns:
 *		hr
 */
HRESULT CPadMessage::AddRecipientToWell(HWND hwndEdit, LPADRENTRY pae,
									BOOL fAddSemi, BOOL fCopyEntry)
{
	HRESULT			hr = S_OK;
	LPSPropValue	pval;
	BOOL			fResolved	= FALSE;
	LPRICHEDITOLE	preole		= NULL;
	CTriple *		ptriple     = NULL;
	INT				cch;
	SRow			rwCopy;
	REOBJECT		reobj		= { 0 };

	// Check if this is a resolved or unresolved name

	pval = PvalFind((LPSRow) pae, PR_ENTRYID);
	if (pval && pval->Value.bin.cb != 0)
	{
		// Its a resolved name
		fResolved = TRUE;

		// Initialize the object information structure
		reobj.cbStruct = sizeof(REOBJECT);
		reobj.cp = REO_CP_SELECTION;
		reobj.clsid = CLSID_CTriple;
		reobj.dwFlags = REO_BELOWBASELINE | REO_INVERTEDSELECT |
						REO_DYNAMICSIZE | REO_DONTNEEDPALETTE;
		reobj.dvaspect = DVASPECT_CONTENT;

		Verify(SendMessage(hwndEdit, EM_GETOLEINTERFACE, 
								  	(WPARAM) NULL, (LPARAM) &preole));

		hr = THR(preole->GetClientSite(&reobj.polesite));
		if (hr != S_OK)
		{
			if (!fCopyEntry)
			{
				MAPIFreeBuffer(pae->rgPropVals);
				pae->rgPropVals = NULL;
			}
			goto MemoryError;
		}

		if (fCopyEntry)
		{
			hr = THR(CopyRow(NULL, (LPSRow) pae, &rwCopy));
			pae = (LPADRENTRY) &rwCopy;
			if (hr != S_OK)
				goto MemoryError;
		}

		ptriple = new CTriple(this, (LPSRow) pae);
		if (!ptriple)
		{
			MAPIFreeBuffer(pae->rgPropVals);
			pae->rgPropVals = NULL;
			goto MemoryError;
		}

		// from this point on, the triple owns the properties
		pae->rgPropVals = NULL;
	}

	if (fAddSemi && (cch = GetWindowTextLength(hwndEdit)) != 0)
	{
		SendMessageA(hwndEdit, EM_SETSEL, cch, cch);
		SendMessageA(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)"; ");
	}

	if (!fResolved)
	{
		// Its an unresolved name

		pval = PvalFind((LPSRow) pae, PR_DISPLAY_NAME_A);
		AssertSz(pval, "Recipient must have a Display Name");
		SendMessageA(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)pval->Value.lpszA);
	}
	else
	{
		// Its a resolved name

		hr = THR(ptriple->QueryInterface(IID_IOleObject, (void**)&reobj.poleobj));
		ptriple->Release();
		if (hr)
			goto MemoryError;

		if (reobj.poleobj->SetClientSite(reobj.polesite))
			goto MemoryError;

		// Report errors if Richedit fails
		hr = THR(preole->InsertObject(&reobj));
		if (FAILED(hr))
		{
			hr = g_LastError.SetLastError(E_FAIL);
			goto Error;
		}
	}

	goto Cleanup;

MemoryError:
	hr = g_LastError.SetLastError(E_OUTOFMEMORY);

Error:
	//TraceError("ScAddRecipientToWell", hr);

Cleanup:
	ReleaseInterface(reobj.poleobj);
	ReleaseInterface(reobj.polesite);
	ReleaseInterface(preole);

	RRETURN(hr);
}

/*
 *	SpecialAdrlistScan
 *	
 *	Purpose:
 *		This function scans through the ADRLIST beginning at *piae.
 *		When this function finds an empty ADRENTRY, it returns.
 *	
 *		When an ADRENTRY is found with a recipient type contained
 *		in rgulDestComps, the data (if any) is removed and we have our
 *		empty ADRENTRY.
 *	
 *		If no empty ADRENTRY is found, the function allocates a new
 *		ADRLIST with room for some more ADRENTRY's.
 *	
 *		If fAboutToAdd is false, this function continues through
 *		the entire ADRLIST, emptying the remaining ADRENTRY's.
 *	
 *	Parameters:
 *		ppal			pointer to LPADRLIST
 *		piae			pointer to ADRENTRY index
 *		cREcipTypes		count of recipients types in rgulDestComps
 *		rgulDestComps	recipient types to mark for removal
 *		fAboutToAdd		TRUE if a recipient is about to be added.
 *						FALSE when we're just finishing off the
 *						scan.
 *	
 *	Returns:
 *		sc
 */
static HRESULT SpecialAdrlistScan(LPADRLIST * ppal, LONG * piae,
				ULONG cRecipTypes, ULONG * rgulDestComps, BOOL fAboutToAdd)
{
	HRESULT			hr		= S_OK;
	ULONG			i;
	LPSPropValue	pval;
	LPADRLIST		pal		= *ppal;
	LPADRENTRY		pae;

	if (pal)
	{
		while (++(*piae) < (LONG) pal->cEntries)
		{
			pae = &pal->aEntries[*piae];

			if (!pae->rgPropVals)
			{
				if (!fAboutToAdd)
					continue;

				// found an empty ADRENTRY and ready to add to it
				goto Cleanup;
			}

			pval = PvalFind((LPSRow) pae, PR_RECIPIENT_TYPE);
			AssertSz(pval, "ADRENTRY with no PR_RECIPIENT_TYPE found");
			if (pval)
			{
				// Check to see if this ADRENTRY corresponds to one of our
				// recipient wells (by checking for a match in rgulDestComps.

				for (i = 0; i < cRecipTypes; ++i)
				{
					if (pval->Value.ul == rgulDestComps[i])
					{
						MAPIFreeBuffer(pae->rgPropVals);
						pae->rgPropVals = NULL;
						pae->cValues = 0;

						if (!fAboutToAdd)
							break;

						// created an empty ADRENTRY and ready to add to it
						goto Cleanup;
					}
				}
			}
		}
	}
	else
		*piae = 0;

	if (fAboutToAdd)
	{
		// We couldn't find (or create) an empty ADRENTRY so we'll
		// have to grow the ADRLIST (or create one) so we will
		// be able to add to pal->aEntries[*piae] on exit.

		//$ REVIEW - I'm adding 5 at a time. Sound reasonable?

		hr = THR(GrowAdrlist(ppal, 5));
	}

Cleanup:
	RRETURN(hr);
}

/*
 *	AddNamesToAdrlist
 *	
 *	Purpose:
 *		This function will add all the resolved and unresolved
 *		names from the edit controls to the ADRLIST
 *	
 *	Parameters:
 *		ppal			pointer to pointer to ADRLIST
 *		cRecipTypes		number of recipients types (which corresponds
 *						to the number of hwndEdit's and ulDestComps'
 *		rghwndEdit		hwnd's of the recipient wells
 *		rgulDestComps	recipient types for each recipient well
 *	
 *	Returns:
 *		hr
 */
HRESULT CPadMessage::AddNamesToAdrlist(LPADRLIST * ppal)
{
	ULONG				i;
	ULONG				iOb;
	ULONG				cOb;
	LPSPropValue		pval;
	LONG				iae		= -1;
	LPRICHEDITOLE		preole	= NULL;
	REOBJECT			reobj	= { 0 };
	EDITSTREAMCOOKIE	esc;
	const EDITSTREAM	es		= {(DWORD) &esc, 0, UnresolvedStream};

	reobj.cbStruct = sizeof(REOBJECT);

	esc.hr = S_OK;
	esc.ppal = ppal;
	esc.piae = &iae;
	esc.fTruncated = FALSE;
	esc.nBrackets = 0;

	// I only want to operate on the dirty edit controls

	esc.cRecipTypes = 0;
	esc.rgulDestComps = new ULONG[_cRecipTypes];
	esc.rghwndEdit = new HWND[_cRecipTypes];
	if (!esc.rgulDestComps || !esc.rghwndEdit)
		goto MemoryError;

	for (i = 0; i < _cRecipTypes; ++i)
	{
		if (_rghwndEdit[i] && SendMessage(_rghwndEdit[i], EM_GETMODIFY, 0, 0))
		{
			esc.rgulDestComps[esc.cRecipTypes] = _rgulRecipTypes[i];
			esc.rghwndEdit[esc.cRecipTypes++] = _rghwndEdit[i];
		}
	}

	for (i = 0; i < esc.cRecipTypes; ++i)
	{
        _fRecipientsDirty = TRUE;

		esc.iRecipType = i;

		// Add all the resolved names (stored as OLE objects) from
		// esc.rghwndEdit[i] to the ADRLIST

		Verify(SendMessage(esc.rghwndEdit[i], EM_GETOLEINTERFACE, 
										(WPARAM) NULL, (LPARAM) &preole));

		cOb = preole->GetObjectCount();

		for (iOb = 0; iOb < cOb; iOb++)
		{
			LPPERSIST	ppersist = NULL;

			if (preole->GetObject(iOb, &reobj, REO_GETOBJ_POLEOBJ) != S_OK)
				goto MemoryError;

			esc.hr = SpecialAdrlistScan(ppal, &iae, esc.cRecipTypes,
											esc.rgulDestComps, TRUE);
			if (esc.hr != S_OK)
				goto Cleanup;

			// BUGBUG: chirsf - Giant hack to get the ADRENTRY !!!
            esc.hr = CopyRow(NULL, &((CTriple*)reobj.poleobj)->_rw,
									(LPSRow) &(*ppal)->aEntries[iae]);
			if (esc.hr != S_OK)
				goto Cleanup;
			
            pval = PvalFind((LPSRow) &(*ppal)->aEntries[iae], PR_RECIPIENT_TYPE);
			AssertSz(pval, "ADRENTRY with no PR_RECIPIENT_TYPE found");
			if (pval)
				pval->Value.l = esc.rgulDestComps[i];

			ClearInterface(&reobj.poleobj);
		}

		ClearInterface(&preole);

		// Add all the unresolved names to the ADRLIST

		esc.cchBuf = 0;

		SendMessage(esc.rghwndEdit[i], EM_STREAMOUT, SF_TEXT, (LPARAM) &es);
		if (esc.hr == S_OK)
		{
			// Add whatever is left after the last semicolon

			// I can ignore the return value since it is als , m, m,m, m                                                                                             o
			// stuffed in esc.sc

			AddUnresolvedName(&esc);
		}

        Edit_SetModify(esc.rghwndEdit[i], FALSE);
	}

	SpecialAdrlistScan(ppal, &iae, esc.cRecipTypes, esc.rgulDestComps, FALSE);

Cleanup:
	if (esc.fTruncated)
		MessageBeep(MB_OK);

	delete esc.rgulDestComps;
	delete esc.rghwndEdit;
	ReleaseInterface(reobj.poleobj);
	ReleaseInterface(preole);
	RRETURN(esc.hr);

MemoryError:
	esc.hr = g_LastError.SetLastError(E_OUTOFMEMORY);
	goto Cleanup;
}



/*
 *	UnresolvedStream
 *	
 *	Purpose:
 *		this is the callback function handed to the edit control
 *		for outputing a stream of text.
 *	
 *	Parameters:
 *		dwCookie		cookie which contains a pointer to a
 *						EDITSTREAMCOOKIE structure
 *		pbBuff			a bunch of characters
 *		cb				count of bytes in pbBuff
 *		pcb				exit: count of bytes read (regardless of error)
 *	
 *	Returns:
 *		hr
 */
DWORD FAR CALLBACK UnresolvedStream(DWORD dwCookie, LPBYTE pbBuff, LONG cb,
						LONG *pcb)
{
	HRESULT hr = S_OK;
	EDITSTREAMCOOKIE *	pesc = (EDITSTREAMCOOKIE *) dwCookie;

	*pcb = cb;

	//$ BUG - broken for Unicode

	// The algorithm below will strip spaces off of the
	// beginning and end of each name

	while (cb)
	{
		cb--;

		if (*pbBuff == '[')
			++pesc->nBrackets;
		else if (*pbBuff == ']')
			pesc->nBrackets = max(0, pesc->nBrackets - 1);
		else if (*pbBuff == '\t')
			*pbBuff = ' ';

		if (!pesc->nBrackets && (*pbBuff == ';' || *pbBuff == '\r'))
		{
			hr = THR(AddUnresolvedName(pesc));
			if (hr != S_OK)
				goto err;
		}
		else if ((*pbBuff != ' ' && *pbBuff != '\n' && *pbBuff != '\r')
					|| pesc->cchBuf > 0)
		{
			if (pesc->cchBuf < cchUnresolvedMax - 1)
			{
				pesc->rgch[pesc->cchBuf++] = *pbBuff;
			}
			else
			{
				// Truncation has occurred so I want to beep
				pesc->fTruncated = TRUE;
			}
		}
		++pbBuff;
	}

err:
	*pcb -= cb;

	return (DWORD) hr;
}

/*
 *	AddUnresolvedName
 *	
 *	Purpose:
 *		This function adds an entry to the ADRLIST in pesc as a
 *		entry with only a PR_DISPLAY_NAME_A
 *	
 *	Parameters:
 *		pesc		ponter to EDITSTREAMCOOKIE data
 *	
 *	Returns:
 *		hr
 */
static HRESULT AddUnresolvedName(EDITSTREAMCOOKIE * pesc)
{
	HRESULT		hr;
	ADRENTRY	ae;

 	while (pesc->cchBuf > 0 && (pesc->rgch[pesc->cchBuf - 1] == ' '
								|| pesc->rgch[pesc->cchBuf - 1] == '\t'))
 		--pesc->cchBuf;

	if (pesc->cchBuf)
	{
		pesc->rgch[pesc->cchBuf] = '\0';
		ae.cValues = 2;
		hr = MAPIAllocateBuffer(2 * sizeof(SPropValue)
					+ pesc->cchBuf + 1, (void**)&ae.rgPropVals);
		if (hr != S_OK)
			goto Error;

		ae.rgPropVals[0].ulPropTag = PR_DISPLAY_NAME_A;
		ae.rgPropVals[0].Value.lpszA = (LPSTR) &ae.rgPropVals[2];
		ae.rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
		ae.rgPropVals[1].Value.l = pesc->rgulDestComps[pesc->iRecipType];

		lstrcpyA(ae.rgPropVals[0].Value.lpszA, pesc->rgch);

		hr = SpecialAdrlistScan(pesc->ppal, pesc->piae,
					pesc->cRecipTypes, pesc->rgulDestComps, TRUE);
		if (hr != S_OK)
			goto Error;

		(*pesc->ppal)->aEntries[*pesc->piae] = ae;

		pesc->cchBuf = 0;
	}

	return S_OK;

Error:
	pesc->hr = hr;
	RRETURN(hr);
}


/*
 *	GrowAdrlist
 *	
 *	Purpose:
 *		This function will grow an existing ADRLIST or create a new
 *		one. caeToAdd empty ADRENTRY's is our goal.
 *	
 *	Parameters:
 *		ppal			pointer to LPADRLIST (could be pointer to
 *						NULL)
 *		caeToAdd		count of ADRENTRY's to add
 *	
 *	Returns:
 *		sc
 */
static HRESULT GrowAdrlist(LPADRLIST * ppal, UINT caeToAdd)
{
	HRESULT         hr = S_OK;
	ULONG			i;
	UINT			cb;
	LPADRLIST		pal		= *ppal;
	LPADRENTRY		pae;

	cb = sizeof(ADRLIST) + caeToAdd * sizeof(ADRENTRY);
	if (pal)
		cb += (UINT) pal->cEntries * sizeof(ADRENTRY);

	hr = THR(MAPIAllocateBuffer(cb, (void**)&pal));
	if (hr)
	{
        hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	if (*ppal)
	{
		CopyMemory(pal, *ppal,
			sizeof(ADRLIST) + (UINT) (*ppal)->cEntries * sizeof(ADRENTRY));
		MAPIFreeBuffer(*ppal);
	}
	else
		pal->cEntries = 0;

	*ppal = pal;

	// Mark new ADRENTRY's as empty

	for (i = 0; i < caeToAdd; i++)
	{
		pae = &pal->aEntries[pal->cEntries++];

		pae->cValues = 0;
		pae->rgPropVals = NULL;
	}

Cleanup:
	RRETURN(hr);
}

/*
 *	AddRecipientToAdrlist
 *	
 *	Purpose:
 *		This functions add a recipient to a (possibly NULL) adrlist
 *	
 *	Parameters:
 *		ppal			pointer to LPADRLIST
 *		pae				pointer to ADRENTRY
 *		pulIndex		pointer where index of recipient in adrlist will be returned
 *						- can be null if caller not interested
 *	
 *	Returns:
 *		sc
 */
HRESULT AddRecipientToAdrlist(LPADRLIST * ppal, LPADRENTRY pae, ULONG *pulIndex)
{
	HRESULT	        hr = S_OK;
	ULONG			iae		= 0;
	LPADRLIST		pal		= *ppal;

	if (pal)
	{
		while (iae < pal->cEntries)
		{
			if (!pal->aEntries[iae++].rgPropVals)
			{
				pal->aEntries[iae - 1] = *pae;
				if (pulIndex)
					*pulIndex = iae - 1;
				return S_OK;
			}
		}
	}

	// We couldn't find an empty ADRENTRY so we'll
	// have to grow the ADRLIST (or create one)

	hr = THR(GrowAdrlist(ppal, 1));
	if (hr)
		goto Cleanup;

	(*ppal)->aEntries[(*ppal)->cEntries - 1] = *pae;
	if (pulIndex)
		*pulIndex = (*ppal)->cEntries - 1;

Cleanup:
	RRETURN(hr);
}

/*
 *	ScBuildSelectionAdrlist
 *	
 *	Purpose:
 *		This function will add all the resolved and unresolved
 *		names from the selection in an edit control to an ADRLIST
 *	
 *	Parameters:
 *		ppal			pointer to pointer to ADRLIST
 *		hwndEdit		hwnd of the edit control
 *		pchrg			CHARRANGE of the selection
 *	
 *	Returns:
 *		hr
 */
HRESULT CPadMessage::BuildSelectionAdrlist(LPADRLIST * ppal, HWND hwndEdit,
								CHARRANGE * pchrg)
{
	HRESULT	        hr          = S_OK;
	ULONG			iOb;
	ULONG			cOb;
	ADRENTRY		ae;
	ULONG			iae			= 0;
	LPRICHEDITOLE	preole		= NULL;
	REOBJECT		reobj		= { 0 };
	ULONG			cb;
	char			rgch[cchUnresolvedMax];
	LPBYTE			pbStart;
	LPBYTE			pbSel;
	ULONG			cchBuf		= 0;
	INT				nBrackets	= 0;
	BOOL			fTruncated	= FALSE;
	const ULONG		cchSel = pchrg->cpMax - pchrg->cpMin;
	TEXTRANGEA		txtrg;

	reobj.cbStruct = sizeof(REOBJECT);
	pbStart = pbSel = new BYTE[cchSel + 1];
	if (!pbSel)
		goto MemoryError;

    memset(pbSel, 0, cchSel + 1);

	// Add all the resolved names (stored as OLE objects) from
	// hwndEdit to the ADRLIST

	Verify(SendMessage(hwndEdit, EM_GETOLEINTERFACE, 
									(WPARAM) NULL, (LPARAM) &preole));

	cOb = preole->GetObjectCount();
	hr = THR(GrowAdrlist(ppal, (UINT) min(cOb, cchSel)));
	if (hr != S_OK)
		goto Cleanup;

	for (iOb = 0; iOb < cOb; iOb++)
	{
		hr = THR(preole->GetObject(iOb, &reobj,	REO_GETOBJ_POLEOBJ));
		if (hr != S_OK)
			goto MemoryError;

		if (reobj.cp >= pchrg->cpMax)
			break;

		if (reobj.cp >= pchrg->cpMin)
		{
			hr = THR(CopyRow(NULL, &((CTriple*)reobj.poleobj)->_rw,
									(LPSRow) &(*ppal)->aEntries[iae++]));
			if (hr != S_OK)
				goto Cleanup;
		}

		ClearInterface(&reobj.poleobj);
	}

	// Add all the unresolved names to the ADRLIST

	// Exchange 4369: Need to use the actual CHARRANGE passed in. Don't assume
	//				  the current selection is correct.
	txtrg.chrg = *pchrg;
	txtrg.lpstrText = (char*)pbSel;
	cb = SendMessageA(hwndEdit, EM_GETTEXTRANGE, 0, (LPARAM) &txtrg) + 1;

	//$ BUG - broken for Unicode

	// The algorithm below will strip spaces off of the
	// beginning and end of each name

	while (cb--)
	{
		if (*pbSel == '[')
			++nBrackets;
		else if (*pbSel == ']')
			nBrackets = max(0, nBrackets - 1);
		else if (*pbSel == '\t')
			*pbSel = ' ';

		if (*pbSel == '\0' ||
			(!nBrackets && (*pbSel == ';' || *pbSel == '\r')))
		{
 			while (cchBuf > 0 && (rgch[cchBuf - 1] == ' '
										|| rgch[cchBuf - 1] == '\t'))
 				--cchBuf;

			if (cchBuf)
			{
				rgch[cchBuf] = '\0';
				ae.cValues = 2;
				hr = THR(MAPIAllocateBuffer(2 * sizeof(SPropValue)
								+ cchBuf + 1, (void**)&ae.rgPropVals));
				if (hr != S_OK)
					goto MemoryError;

				ae.rgPropVals[0].ulPropTag = PR_DISPLAY_NAME_A;
				ae.rgPropVals[0].Value.lpszA = (LPSTR) &ae.rgPropVals[2];
				ae.rgPropVals[1].ulPropTag = PR_RECIPIENT_TYPE;
				ae.rgPropVals[1].Value.l = -1;

				lstrcpyA(ae.rgPropVals[0].Value.lpszA, rgch);

				if (iae == (*ppal)->cEntries)
				{
					hr = THR(GrowAdrlist(ppal, 5));
					if (hr != S_OK)
						goto Cleanup;
				}

				(*ppal)->aEntries[iae++] = ae;

				cchBuf = 0;
			}
		}
		else if (((*pbSel != ' ' && *pbSel != '\n' && *pbSel != '\r')
					|| cchBuf > 0) && !fTruncated)
		{
			if (cchBuf < cchUnresolvedMax - 1)
				rgch[cchBuf++] = *pbSel;
			else
				fTruncated = TRUE;
		}
		++pbSel;
	}

Cleanup:
	if (fTruncated)
		MessageBeep(MB_OK);

	if (*ppal)
		(*ppal)->cEntries = iae;

	delete pbStart;
	ReleaseInterface(reobj.poleobj);
	ReleaseInterface(preole);
	RRETURN(hr);

MemoryError:
	hr = g_LastError.SetLastError(hr);
	goto Cleanup;
}
