//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       msgtripl.cxx
//
//  Contents:   Recipient object (CTriple) implementation
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

#ifndef X_PADRC_H_
#define X_PADRC_H_
#include "padrc.h"
#endif

static HPEN hpenUnderline = 0;

#define	CTXCK_RecipOptions	1
#define CTXCK_AddToPAB		2

#ifdef DBCS
#define LangJpn	0x0411
#define LangKor	0x0412
#endif

#ifndef MAC
#ifdef WIN32
#define	RTEXPORT	WINAPI
#else
#define	RTEXPORT	_export __loadds
#endif
#else // MAC
#define RTEXPORT
#endif // MAC


// $MAC - Only supports 4 character format names
#ifndef MAC
#define CF_TRIPLE	TEXT("MsMail 4.0 Recipient")
#else
#define CF_TRIPLE	TEXT("XCGA")
#endif

TCHAR CTripData::s_szClipFormatTriple[] = CF_TRIPLE;

FORMATETC CTripData::s_rgformatetcTRIPOLE[] =
{
	{ 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
	{ 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }
};

#define	iFormatTriple		0
#define	iFormatText			1

#define cformatetcTRIPOLE (sizeof(s_rgformatetcTRIPOLE) / sizeof(FORMATETC))


#ifdef WIN32
#define OleStdMalloc(_ul)			CoTaskMemAlloc(_ul)
#define OleStdFree(_pv)				CoTaskMemFree(_pv)
#define OleStdFreeString(_sz, _pm)	CoTaskMemFree(_sz)
#ifdef MAC
LPSTR OleStdCopyString(LPSTR lpszSrc, LPMALLOC lpMalloc);
#endif
#else
LPVOID OleStdMalloc(ULONG ulSize);
void OleStdFree(LPVOID pmem);
ULONG OleStdGetSize(LPVOID pmem);
void OleStdFreeString(LPSTR lpsz, LPMALLOC lpMalloc);
LPSTR OleStdCopyString(LPSTR lpszSrc, LPMALLOC lpMalloc);
#endif


#define	FreeAdrlist(_pal)		FreeSRowSet((LPSRowSet) _pal)


#if DBG == 1
	CTriple * g_pTripObject = NULL;
#endif


/*
 *	OpenPAB
 *	
 *	Purpose:
 *		This function opens up the default Personal Address Book.
 *		Note: it does an OpenEntry() w/ MAPI_DEFERRED_ERRORS for
 *		better performance.  Error handling code for the caller
 *		of this function needs to be correct.
 *	
 *	Parameters:
 *		pab			pointer to Address Book
 *		ppabc		pointer to pointer to AB container
 *	
 *	Returns:
 *		hr
 */
HRESULT OpenPAB(LPADRBOOK pab, LPABCONT * ppabc)
{
	HRESULT			hr;
	ULONG			cbEid;
	LPENTRYID		peid;
	ULONG			ul;

	hr = pab->GetPAB(&cbEid, &peid);
	if (hr != S_OK)
		goto Error;

	hr = pab->OpenEntry(cbEid, peid,
					NULL, MAPI_DEFERRED_ERRORS, &ul, (LPUNKNOWN *) ppabc);
	
    MAPIFreeBuffer(peid);
	if (hr != S_OK)
		goto Error;

	Assert(ul == MAPI_ABCONT);
	return S_OK;

Error:
	RRETURN(g_LastError.SetLastError(hr, pab));
}

/*
 *	CopyBufferToRow
 *	
 *	Purpose:
 *		This copies the properties from a flat buffer to a row.
 *
 *		NOTE - Multi Value propertis not supported right now
 *	
 *		NOTE - tripole.c relies on specific behaviour of this function
 *	
 *	Parameters:
 *		pb			pointer to the buffer
 *		prwDst		destination row (with lpProps already allocated)
 *	
 *	Returns:
 *		sc
 */
VOID CALLBACK CopyBufferToRow(LPBYTE pb, LPSRow prwDst)
{
	ULONG			iCol;
	LPSPropValue	pvalSrc;
	LPSPropValue	pvalDst;
	LPBYTE			pbSrc;
	LPBYTE			pbDst;

	// Do the finicky copy of the items one-by-one.

	iCol = prwDst->cValues = *((ULONG *) pb);
	pvalSrc = (LPSPropValue) (pb + sizeof(ULONG));
	pvalDst = prwDst->lpProps;
	pbSrc = (LPBYTE) (pvalSrc + iCol);		// point past the rgSPropValue.
	pbDst = (LPBYTE) (pvalDst + iCol);		// point past the rgSPropValue.

	while (iCol--)
	{
		pbSrc = CopyPval(pvalSrc, pvalDst, pbSrc, &pbDst, imodeUnflatten);
		++pvalSrc;
		++pvalDst;
	}
}


// MLOLE: Reduced version used by TRIPCALL_GetNewStorage in mapidlg\tripole.c
//		  and by HrGetDataAttachFileContents in mapin\attole.c
/*
 *	StgCreateOnHglobal
 *
 *	Purpose:
 *		Smaller version of OleStdCreateStorageOnHGlobal for Capone use.
 *
 *	Arguments:
 *		None.
 *
 *	Returns:
 *		LPSTORAGE		Created storage, or NULL on error (E_OUTOFMEMORY).
 */
LPSTORAGE StgCreateOnHglobal(VOID)
{
	LPLOCKBYTES plb = NULL;
	LPSTORAGE pstg = NULL;

	// Create lockbytes on hglobal
	if (CreateILockBytesOnHGlobal(NULL, TRUE, &plb))
		return NULL;

	// Create storage on lockbytes
	StgCreateDocfileOnILockBytes(plb, STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE |
			STGM_CREATE | STGM_READWRITE, 0, &pstg);

	// Release our reference on the lockbytes and return the storage
	ReleaseInterface(plb);
	return pstg;
}


/*
 *	StuffRwsInHglobal
 *
 *	Purpose:
 *		Saves an LPSRowSet to a HGLOBAL
 *
 *	Parameters:
 *		prws			pointer to the row set
 *		pmedium			pmedium containing hglobal
 *
 *	Returns:
 *		hr
 */
HRESULT RTEXPORT StuffRwsInHglobal(LPSRowSet prws, LPSTGMEDIUM pmedium)
{
	ULONG			iRow;
	ULONG			cRows		= 0;
	ULONG			iCol;
	LPSPropValue	pvalSrc;
	LPSPropValue	pvalDst;
	LPBYTE			pb;
	LPBYTE			pbOld;
	ULONG			cb = 0;

	//TraceTag(tagTripole, "ScStuffRwsInHglobal");

	for (iRow = 0; iRow < prws->cRows; ++iRow)
	{
		pvalSrc = prws->aRow[iRow].lpProps;
		if (pvalSrc == NULL)
			continue;				// skip empty rows

		++cRows;
		cb += 2 * sizeof(ULONG);	// to store the size of buffer required
									// for each row and the number of props
		for (iCol = prws->aRow[iRow].cValues; iCol > 0; ++pvalSrc, --iCol)
		{
			cb += PAD4(CbForPropValue(pvalSrc)) + sizeof(SPropValue);
		}
	}

	pmedium->hGlobal = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, cb);
	if (!pmedium->hGlobal)
		return E_OUTOFMEMORY;
	pb = (BYTE *) GlobalLock(pmedium->hGlobal);
	Assert(pb);

	*((ULONG *) pb) = cRows;
	pb += sizeof(ULONG);

	for (iRow = 0; iRow < prws->cRows; ++iRow)
	{
		pvalSrc = prws->aRow[iRow].lpProps;
		if (pvalSrc == NULL)
			continue;				// skip empty rows

		pbOld = pb;
		pb += sizeof(ULONG);

		iCol = *((ULONG *) pb) = prws->aRow[iRow].cValues;
		pvalDst = (LPSPropValue) (pb + sizeof(ULONG));
		pb = (LPBYTE) (pvalDst + iCol);		// point past the rgSPropValue.

		while (iCol--)
		{
			pb = CopyPval(pvalSrc, pvalDst, pb, NULL, imodeFlatten);
			++pvalSrc;
			++pvalDst;
		}

		*((ULONG *) pbOld) = pb - pbOld - sizeof(ULONG);
	}

	GlobalUnlock(pmedium->hGlobal);
	return S_OK;
}



/*
 *	FindTripleProperties
 *	
 *	Purpose:
 *		This is some common code needed when creating a recipient
 *		in a TRIPLE object.
 *	
 *	Parameters:
 *		ptriple		pointer to triple object
 */
VOID CTriple::FindTripleProperties()
{
	ULONG			i;
	LPSPropValue	pval	= _rw.lpProps;

	_iName = LONG_MAX;
#ifdef	DEBUG
	_iEid = LONG_MAX;
#endif	
	_iType = LONG_MAX;			//$ REVIEW - Is it OK if this is missing?
	for (i = 0; i < _rw.cValues; i++)
	{
		if (pval[i].ulPropTag == PR_DISPLAY_NAME_A)
			_iName = i;
		else if (pval[i].ulPropTag == PR_ENTRYID)
			_iEid = i;
		else if (pval[i].ulPropTag == PR_DISPLAY_TYPE)
			_iType = i;
	}
	AssertSz(_iEid != LONG_MAX, "No EntryId in recipient");
}



/*
 *	T R I P P E R   I m p l e m e n t a t i o n
 */

/*
 *	GetClassID
 *
 *	Purpose:
 *		Returns an object's class identifier.
 *
 *	Parameters:
 *		ppersist		pointer to IPersist object
 *		pClassID		pointer to where to return the object's
 *						CLSID
 *
 *	Returns:
 *		hr
 */
HRESULT CTriple::GetClassID(LPCLSID pClassID)
{
	//TraceTag(tagTripole, "TRIPPER_GetClassID");

	*pClassID = CLSID_CTriple;

	return S_OK;
}


/*
 *	T R I P L E   I m p l e m e n t a t i o n
 */

/*
 *	Constructor CTriple
 *	
 *	Purpose:
 *		Creates a new class CTRIPLE.
 *	
 *	Parameters:
 *		prw			pointer to row respresenting an ADRENTRY
 *	
 *	Returns:
 *		CTriple *	The newly created CTriple.
 */
CTriple::CTriple(CPadMessage * pPad, LPSRow prw)
{
	//TraceTag(tagTripole, "TRIPLE_New");

	_cRef = 1;					// Start with one reference
	_pClientSite = NULL;
	_padvisesink = NULL;
	_pPad = pPad;
	if (prw)
	{
		_rw = *prw;
		FindTripleProperties();
	}

	_pstg = NULL;
	_fUnderline = TRUE;

	// Try to set up an OleAdviseHolder
	CreateOleAdviseHolder(&_poleadviseholder);

#if DBG == 1
	g_pTripObject = this;
#endif
}

/*
 *	QueryInterface
 *
 *	Purpose:
 *		Returns a pointer to the specified site.
 *
 *	Parameters:
 *		LPUNKNOWN *	 Object from which we want an interface.
 *		REFIID		 Interface we want.
 *		LPUNKNOWN *	 Interface we return.
 *
 *	Returns:
 *		HRESULT		 Error status.
 */
HRESULT CTriple::QueryInterface(REFIID riid,
								   LPVOID * ppvObj)
{
	//TraceTag(tagTripole, "TRIPLE_QueryInterface");

	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IOleObject))
	{
		*ppvObj = (IOleObject*)this;
	}
	else if (IsEqualIID(riid, IID_IViewObject))
	{
		*ppvObj = (IViewObject*)this;
	}
	else if (IsEqualIID(riid, IID_IPersist))
    {
		*ppvObj = (IPersist*)this;
	}
    else
	{
		//TraceTag(tagTripole, "Don't know interface %lx", riid->Data1);
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}

    (*(IUnknown**)ppvObj)->AddRef();

	return S_OK;
}

/*
 *	AddRef
 *
 *	Purpose:
 *		Increments reference count on the specified site.
 *
 *	Returns:
 *		ULONG			New value of reference count.
 */
ULONG CTriple::AddRef()
{
	//TraceTag(tagTripole, "TRIPLE_AddRef");

	return ++_cRef;
}

/*
 *	Release
 *
 *	Purpose:
 *		Decrements reference count on the specified site.  If count is
 *		decremented to zero, the object is freed.
 *
 *	Returns:
 *		ULONG			New value of reference count.
 */
ULONG CTriple::Release()
{
	ULONG cRef = --_cRef;

	//TraceTag(tagTripole, "TRIPLE_Release");

	if (!cRef)
	{
		//TraceTag(tagTripole, "TRIPLE_Release: freeing the triple");
#ifdef	DEBUG
		g_pTripObject = NULL;
#endif	
		MAPIFreeBuffer(_rw.lpProps);
		ReleaseInterface(_pClientSite);
		ReleaseInterface(_pstg);
		ReleaseInterface(_poleadviseholder);
		ReleaseInterface(_padvisesink);
		delete this;
	}

	AssertSz(cRef >= 0, "TRIPLE_Release: negative cRef");
	return cRef;
}



/*
 *	SetClientSite
 *	
 *	Purpose:
 *		Stores the IClientSite that this object is contained in.
 *	
 *	Parameters:
 *		pClientSite			the client site
 *	
 *	Returns:
 *		hresult
 */
HRESULT CTriple::SetClientSite(IOleClientSite * pClientSite)
{
	HRESULT			hr;
	CTripCall *		pTripCall;

	//TraceTag(tagTripole, "TRIPOBJECT_SetClientSite");

	if (_pClientSite)
	{
		ClearInterface(&_pClientSite);
	}

	if (!pClientSite)
		return S_OK;

	_pClientSite = pClientSite;
	_pClientSite->AddRef();

	hr = THR(pClientSite->QueryInterface(IID_IRichEditOleCallback, (void**)&pTripCall));

    if (hr == S_OK)
	{
		_fUnderline = pTripCall->GetUnderline();
		pTripCall->Release();
	}

	return S_OK;
}


/*
 *	GetClientSite
 *
 *	Purpose:
 *		Retrieves the IClientSite that this object is contained in.
 *
 *	Parameters:
 *		poleobject			this object
 *		ppClientSite		on return, a pointer to the client site
 *
 *	Returns:
 *		hresult
 */
HRESULT CTriple::GetClientSite(IOleClientSite ** ppClientSite)
{
	//TraceTag(tagTripole, "TRIPOBJECT_GetClientSite");

	*ppClientSite = _pClientSite;
    _pClientSite->AddRef();

	return S_OK;
}


/*
 *	SetHostNames
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
	//TraceTag(tagTripole, "TRIPOBJECT_SetHostNames");

	return S_OK;
}


/*
 *	Close
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::Close(DWORD dwSaveOption)
{
	//TraceTag(tagTripole, "TRIPOBJECT_Close");

	return S_OK;
}


/*
 *	SetMoniker
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk)
{
	//TraceTag(tagTripoleStub, "TRIPOBJECT_SetMoniker: NYI");
	return E_NOTIMPL;
}


/*
 *	GetMoniker
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
	//TraceTag(tagTripole, "TRIPOBJECT_GetMoniker");

	// I don't do Monikers

	return E_FAIL;
}


/*
 *	InitFromData
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::InitFromData(LPDATAOBJECT pDataObject, BOOL fCreation, DWORD dwReserved)
{
	//TraceTag(tagTripoleStub, "TRIPOBJECT_InitFromData: NYI");
	return E_NOTIMPL;
}


/*
 *	GetClipboardData
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::GetClipboardData(DWORD dwReserved, LPDATAOBJECT * ppDataObject)
{
	HRESULT			hr;
	SCODE			sc;
	LPSRowSet		prws;
    CTripData *     pTripleData;

	//TraceTag(tagTripole, "TRIPOBJECT_GetClipboardData");

	sc = MAPIAllocateBuffer(CbNewSRowSet(1), (void**)&prws);
	if (sc != S_OK)
		goto MemoryError;

	prws->cRows = 1;
	sc = CopyRow(NULL, &_rw, prws->aRow);
	if (sc != S_OK)
	{
		MAPIFreeBuffer(prws);
		goto MemoryError;
	}
	
    pTripleData = new CTripData(prws);
	if (!pTripleData)
	{
		FreeSRowSet(prws);
		goto MemoryError;
	}

	// Need to do the QueryInterface() to get a wrapped object

	hr = pTripleData->QueryInterface(IID_IDataObject, (void**)ppDataObject);

	// QueryInterface() does an AddRef() so we're up to two,
	// but the object we return should only be at one
	pTripleData->Release();

	RRETURN(hr);

MemoryError:
	RRETURN(E_OUTOFMEMORY);
}


/*
 *	DoVerb
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::DoVerb(LONG iVerb,
				LPMSG lpmsg, IOleClientSite * pActivateSite,
				LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
#ifdef LATER
	HRESULT			hr;
	LPADRBOOK		pab;
	ULONG			ulUIParam	= UlFromHwnd(GetParent(hwndParent));
	static SPropTagArray	taga	= {8, {PR_DISPLAY_NAME_A, PR_DISPLAY_TYPE,
								PR_OBJECT_TYPE, PR_ADDRTYPE, PR_ENTRYID,
								PR_SEARCH_KEY, PR_EMAIL_ADDRESS, PR_NULL}};

	
	//TraceTag(tagTripole, "TRIPOBJECT_DoVerb");

    pab = _pab;
	if (!pab)
	{
		//$ NYI - Do what if no _pab
		return S_OK;
	}

	if (iVerb == 0)
	{
		ULONG			ul;
		LONG			lRecipType;
		LPSPropValue	pval;
		BOOL			fRecipientType	= FALSE;
		DETAILSDATA 	dtd = {0};
		LPMAPIERROR		pme = NULL;
		HRESULT			hr;
		ULONG			cbEid;
		LPENTRYID		peid;

		dtd.ppme = &pme;
		dtd.pab = pab;
		dtd.ulUIParam = ulUIParam;
		dtd.cbEid = _rw.lpProps[_iEid].Value.bin.cb;
		dtd.peid = (LPENTRYID) _rw.lpProps[_iEid].Value.bin.lpb;

		// Do we have a PAB?
		dtd.fShowAddToPAB			= FALSE;
		hr = pab->GetPAB(&cbEid, &peid);
		if (hr == S_OK && peid)
		{
			dtd.fShowAddToPAB		= TRUE;
			MAPIFreeBuffer(peid);
		}

		dtd.fWantPmpReturned = TRUE;
		dtd.lDisplayType = -1;
		Assert(dtd.hr == S_OK);
		Assert(dtd.fNextPrevVisible == FALSE);
		Assert(dtd.fNewEntryInvoked == FALSE);
		Assert(dtd.pabd == NULL);
		Assert(dtd.dtret == 0);
		Assert(dtd.pmp == NULL);
		Assert(dtd.szDisplayName == NULL);
		Assert(dtd.lpfndismiss == NULL);
		Assert(dtd.lpvDismissContext == NULL);
		Assert(dtd.lpfnCustom == NULL);
		Assert(dtd.lpvButtonContext == NULL);
		Assert(dtd.szCustomText == NULL);

		DisplayDetailsDialog(&dtd);
		if (dtd.hr != S_OK || !dtd.fChanged
				|| GetWindowLong(hwndParent, GWL_STYLE) & ES_READONLY)
		{
			if (!dtd.hr)
				dtd.pmp->(dtd.pmp);

			if (dtd.hr && dtd.hr != MAPI_E_USER_CANCEL)
			{
				//TraceError("TRIPOBJECT_DoVerb", GetScode(dtd.hr));
				DisplayTheError(dtd.hr, dtd.ppme);
			}
			goto Cleanup;
		}

		// The user might have changed some of the properties, or the
		// properties may have been changed in the Address Book. If the
		// user OK'd the details dialog, get the new values

		pval = PvalFind(&_rw, PR_RECIPIENT_TYPE);
		if (pval)
		{
			fRecipientType = TRUE;
			lRecipType = pval->Value.l;
		}

		ul = taga.cValues;
		hr = dtd.pmp->GetProps(&taga, 0, &ul, &pval);
		Assert(ul == taga.cValues);
		if (FAILED(hr))
		{
			//ScReportErrorHrPmunk(hr, (LPMAPIUNK) dtd.pmp);
			dtd.pmp->Release();
			goto Cleanup;
		}

		dtd.pmp->Release();
		MAPIFreeBuffer(_rw.lpProps);
		if (fRecipientType)
		{
			pval[ul - 1].ulPropTag = PR_RECIPIENT_TYPE;
			pval[ul - 1].Value.l = lRecipType;
		}

		_rw.cValues = ul;
		_rw.lpProps = pval;
		FindTripleProperties();

		Edit_SetModify(hwndParent, TRUE);
		_padvisesink->OnViewChange(DVASPECT_CONTENT, -1);
	}
	else if (iVerb == 1)
	{
		LPADRENTRY		pae	= (LPADRENTRY) &_rw;

		hr = pab->RecipOptions(ulUIParam, 0, pae);
		if (hr != S_OK)
		{
			//if (hr != MAPI_E_USER_CANCEL)
				//ScReportErrorHrPmunk(hr, (LPMAPIUNK) pabTriple);
			goto Cleanup;
		}
		Assert(pae == (LPADRENTRY) &_rw);
		FindTripleProperties();
		Edit_SetModify(hwndParent, TRUE);
	}
	else if (iVerb == 2)
	{
		LPABCONT		pabcPAB;
		ENTRYLIST		el;
		SBinary			bin;

		if (OpenPAB(pab, &pabcPAB) != S_OK)
		{
			ReportLastErrorNull();
			goto Cleanup;
		}

		el.cValues = 1;
		el.lpbin = &bin;
		bin.cb = _rw.lpProps[_iEid].Value.bin.cb;
		bin.lpb = _rw.lpProps[_iEid].Value.bin.lpb;

		hr = pabcPAB->CopyEntries(pabcPAB, &el, ulUIParam,
					NULL, CREATE_CHECK_DUP_STRICT);
		if (hr != S_OK)
		{
			TraceError("TRIPOBJECT_DoVerb", GetScode(hr));
			ScReportErrorHrPmunk(hr, (LPMAPIUNK) pabcPAB);
		}
		ReleaseInterface(pabcPAB);
	}
	else
	{
		return OLEOBJ_S_INVALIDVERB;
	}

Cleanup:
	return S_OK;
#else
    RRETURN(E_NOTIMPL);
#endif
}


#ifdef LATER
/*
 *	EnumVerbsCallback
 *
 *	Purpose:
 *		Support function for the EnumVerbs() which will allocate
 *		the value for each of the elements
 *
 *	Arguments:
 *		iel			The index of the element
 *		pbDst		Pointer to the data to be munged
 *
 *	Returns:
 *		BOOL		TRUE on success
 */
BOOL CTRIPLE::EnumVerbsCallback(ULONG iel, LPVOID pbDst)
{
	OLEVERB *	poleverb = (OLEVERB *) pbDst;
	CHAR		rgch[40];
	UsesMakeOLESTR;

	AssertSz(iel < 3, "Know only three verbs");
	LoadString(g_hInstResource, STR_TripoleVerb0 + (UINT) iel, rgch,
				sizeof(rgch) / sizeof(CHAR));
#if defined (WIN32) && !defined (MAC)
	poleverb->lpszVerbName = OleStdMalloc(40 * sizeof(OLECHAR));
	if (!poleverb->lpszVerbName)
		return FALSE;
	lstrcpyW(poleverb->lpszVerbName, MakeOLESTR(rgch));
	return TRUE;
#else
	return (poleverb->lpszVerbName = OleStdCopyString(rgch, NULL))
														? TRUE : FALSE;
#endif
}
#endif

/*
 *	EnumVerbs
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::EnumVerbs(IEnumOLEVERB ** ppenumOleVerb)
{
#ifdef LATER
	static OLEVERB	oleverb[]	= {
						{0, NULL, 0, OLEVERBATTRIB_NEVERDIRTIES
							| OLEVERBATTRIB_ONCONTAINERMENU},
						{1, NULL, 0, OLEVERBATTRIB_NEVERDIRTIES
							| OLEVERBATTRIB_ONCONTAINERMENU},
						{2, NULL, 0, OLEVERBATTRIB_NEVERDIRTIES
							| OLEVERBATTRIB_ONCONTAINERMENU}};

	//TraceTag(tagTripole, "TRIPOBJECT_EnumVerbs");

	*ppenumOleVerb = (LPENUMOLEVERB) ENUMUNK_New(
						(LPIID) &IID_IEnumOLEVERB,
						3, sizeof(OLEVERB), &oleverb,
						EnumVerbsCallback);
	if (!*ppenumOleVerb)
		return E_OUTOFMEMORY;

	return S_OK;
#else
    RRETURN(OLEOBJ_E_NOVERBS);
#endif
}


/*
 *	Update
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::Update()
{
	//TraceTag(tagTripoleStub, "TRIPOBJECT_Update: NYI");
	return E_NOTIMPL;
}


/*
 *	IsUpToDate
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::IsUpToDate()
{
	//TraceTag(tagTripoleStub, "TRIPOBJECT_IsUpToDate: NYI");
	return E_NOTIMPL;
}


/*
 *	GetUserClassID
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::GetUserClassID(CLSID * pclsid)
{
	//TraceTag(tagTripole, "TRIPOBJECT_GetUserClassID");

	*pclsid = CLSID_CTriple;

	return S_OK;
}


/*
 *	GetUserType
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::GetUserType(DWORD dwFormOfType, LPOLESTR * lpszUserType)
{
	UINT	ids;
	INT		cch;
#if defined(WIN32) && !defined(MAC)
	TCHAR	szT[40];
#endif

	if (dwFormOfType == USERCLASSTYPE_APPNAME)
	{
		ids = IDS_ErrorCaptionMail;
		cch = 40;
	}
	else
	{
		ids = IDS_TripoleFullName;
		cch = 20;
	}

	*lpszUserType = (TCHAR *) OleStdMalloc(cch * sizeof(OLECHAR));
	if (*lpszUserType == NULL)
		return E_OUTOFMEMORY;

#if defined(WIN32) && !defined(MAC)
	LoadString(g_hInstResource, ids, szT, cch);
	lstrcpyW(*lpszUserType, szT);
#else	// !WIN32
	LoadString(g_hInstResource, ids, (char*)*lpszUserType, cch);
#endif	// !WIN32

	//TraceTag(tagTripole, "TRIPOBJECT_GetUserType");

	return S_OK;
}


/*
 *	SetExtent
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::SetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
	//TraceTag(tagTripole, "TRIPOBJECT_SetExtent");

	// The object's size is fixed

	return E_FAIL;
}


/*
 *	GetExtent
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::GetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
	HFONT			hfontOld;
	HDC				hdc;
	TEXTMETRIC		tm;
	LPSTR			szName;
	SIZE			size;
	SIZEL			sizel;

	//TraceTag(tagTripole, "TRIPOBJECT_GetExtent");

	hdc = GetWindowDC(NULL);
	Assert(hdc);

    hfontOld = SelectFont(hdc, GetStockObject(DEFAULT_GUI_FONT));

	if (_iName == LONG_MAX)
		szName = "";
	else
		szName = _rw.lpProps[_iName].Value.lpszA;

	GetTextExtentPointA(hdc, szName, lstrlenA(szName), &size);

	GetTextMetrics(hdc, &tm);

	sizel.cx = size.cx + 1;					// One pixel seems to be clipped
	sizel.cy = size.cy - tm.tmDescent;		// Same height as normal line (RAID 11516 was +1)



    lpsizel->cx = sizel.cx * 2540 / GetDeviceCaps(hdc, LOGPIXELSX);
    lpsizel->cy = sizel.cy * 2540 / GetDeviceCaps(hdc, LOGPIXELSY);

	SelectFont(hdc, hfontOld);
	ReleaseDC(NULL, hdc);
	return S_OK;
}


/*
 *	Advise
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::Advise(IAdviseSink * pAdvSink, DWORD * pdwConnection)
{
    if (_poleadviseholder)
		return _poleadviseholder->Advise(pAdvSink, pdwConnection);

	return OLE_E_ADVISENOTSUPPORTED;
}


/*
 *	Unadvise
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::Unadvise(DWORD dwConnection)
{
	if (_poleadviseholder)
		return _poleadviseholder->Unadvise(dwConnection);

	return E_FAIL;
}


/*
 *	EnumAdvise
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::EnumAdvise(LPENUMSTATDATA * ppenumAdvise)
{
	if (_poleadviseholder)
		return _poleadviseholder->EnumAdvise(ppenumAdvise);

	return OLE_E_ADVISENOTSUPPORTED;
}


/*
 *	GetMiscStatus
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::GetMiscStatus(DWORD dwAspect, DWORD FAR* pdwStatus)
{
	//TraceTag(tagTripole, "TRIPOBJECT_GetMiscStatus");

	*pdwStatus = 0;

	return S_OK;
}


/*
 *	SetColorScheme
 *
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTriple::SetColorScheme(LPLOGPALETTE lpLogpal)
{
	//TraceTag(tagTripoleStub, "TRIPOBJECT_SetColorScheme: NYI");
	return E_NOTIMPL;
}


/*
 *	Draw
 *	
 *	Purpose:
 *	
 *	Parameters:
 *	
 *	Returns:
 *		hresult
 */
HRESULT CTriple::Draw(DWORD dwDrawAspect,
                    LONG lindex,
                    void *pvAspect,
                    DVTARGETDEVICE *ptd,
                    HDC hdcTargetDev,
                    HDC hdcDraw,
                    LPCRECTL lprcBounds,
                    LPCRECTL lprcWBounds,
                    BOOL (STDMETHODCALLTYPE *pfnContinue)(DWORD dwContinue),
                    DWORD dwContinue)
{
	LPSTR			szName;
	TEXTMETRIC		tm;

	if (_iName == LONG_MAX)
		szName = "";
	else
   		szName = _rw.lpProps[_iName].Value.lpszA;

	SetTextAlign(hdcDraw, TA_BOTTOM);
	TextOutA(hdcDraw, lprcBounds->left, lprcBounds->bottom, szName, lstrlenA(szName));

	GetTextMetrics(hdcDraw, &tm);

	// Only draw the underline if we're told to
	if (_fUnderline)
	{
		if (!hpenUnderline)
		{
			hpenUnderline = CreatePen(PS_SOLID, 0,
									GetSysColor(COLOR_WINDOWTEXT));
			if (!hpenUnderline)
				return E_OUTOFMEMORY;
		}

		SelectPen(hdcDraw, hpenUnderline);
#ifndef DBCS
		MoveToEx(hdcDraw, lprcBounds->left, lprcBounds->bottom - tm.tmDescent + 1, NULL);
		LineTo(hdcDraw, lprcBounds->right, lprcBounds->bottom - tm.tmDescent + 1);
#else
		MoveToEx(hdcDraw, lprcBounds->left, lprcBounds->bottom-1 , NULL);
		LineTo(hdcDraw, lprcBounds->right, lprcBounds->bottom-1 );
#endif
	}

	return S_OK;
}


/*
 *	GetColorSet
 *	
 *	Purpose:
 *	
 *	Parameters:
 *	xxc
 *	Returns:
 *		hresult
 */
HRESULT CTriple::GetColorSet(DWORD dwDrawAspect, LONG lindex, void FAR* pvAspect,
				DVTARGETDEVICE FAR * ptd, HDC hicTargetDev,
				LPLOGPALETTE FAR* ppColorSet)
{
	//TraceTag(tagTripoleStub, "TRIPVIEW_GetColorSet: NYI");
	return E_NOTIMPL;
}


/*
 *	Freeze
 *	
 *	Purpose:
 *	
 *	Parameters:
 *	
 *	Returns:
 *		hresult
 */
HRESULT CTriple::Freeze(DWORD dwDrawAspect,
				LONG lindex, void FAR* pvAspect, DWORD FAR* pdwFreeze)
{
	//TraceTag(tagTripoleStub, "TRIPVIEW_Freeze: NYI");
	return E_NOTIMPL;
}


/*
 *	Unfreeze
 *	
 *	Purpose:
 *	
 *	Parameters:
 *	
 *	Returns:
 *		hresult
 */
HRESULT CTriple::Unfreeze(DWORD dwFreeze)
{
	//TraceTag(tagTripoleStub, "TRIPVIEW_Unfreeze: NYI");
	return E_NOTIMPL;
}


/*
 *	SetAdvise
 *	
 *	Purpose:
 *	
 *	Parameters:
 *	
 *	Returns:
 *		hresult
 */
HRESULT CTriple::SetAdvise(DWORD aspects, DWORD advf, LPADVISESINK pAdvSink)
{
	
	//TraceTag(tagTripole, "TRIPVIEW_SetAdvise");

	ReleaseInterface(_padvisesink);
	if (pAdvSink)
		pAdvSink->AddRef();
	_padvisesink = pAdvSink;

	return S_OK;
}


/*
 *	GetAdvise
 *	
 *	Purpose:
 *	
 *	Parameters:
 *	
 *	Returns:
 *		hresult
 */
HRESULT CTriple::GetAdvise(DWORD FAR* pAspects, DWORD FAR* pAdvf,
				LPADVISESINK FAR* ppAdvSink)
{
	//TraceTag(tagTripoleStub, "TRIPVIEW_GetAdvise: NYI");
	AssertSz(FALSE, "I'm easy to implement if I'm called");
	return E_NOTIMPL;
}


/*
 *	Constructor
 *	
 *	Purpose:
 *		Creates a new CTRIPDATA object.
 *	
 *	Parameters:
 *		prws		pointer to row set containing one or more objects
 *	
 *	Returns:
 *		TRIPDATA *	The newly created TRIPDATA.
 */
CTripData::CTripData(LPSRowSet prws)
{
	//TraceTag(tagTripole, "TRIPDATA_New");

	_cRef = 1;				// Start with one reference
	_prws = prws;

    if (s_rgformatetcTRIPOLE[iFormatText].cfFormat == 0)
    {
	    // Register our clipboard formats
	    s_rgformatetcTRIPOLE[iFormatText].cfFormat = CF_TEXT;
	    s_rgformatetcTRIPOLE[iFormatTriple].cfFormat =
					    RegisterClipboardFormat(s_szClipFormatTriple);
    }
}


/*
 *	QueryInterface
 *
 *	Purpose:
 *		Returns a pointer to the specified site.
 *
 *	Parameters:
 *		REFIID		 Interface we want.
 *		LPUNKNOWN *	 Interface we return.
 *
 *	Returns:
 *		HRESULT		 Error status.
 */
HRESULT CTripData::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	//TraceTag(tagTripole, "TRIPDATA_QueryInterface");

	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDataObject))
	{
		*ppvObj = (IDataObject*)this;
        AddRef();
	}
	else
	{
		//TraceTag(tagTripole, "Don't know interface %lx", riid->Data1);
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}
	return S_OK;
}

/*
 *  AddRef
 *
 *	Purpose:
 *		Increments reference count on the specified site.
 *
 *	Returns:
 *		ULONG			New value of reference count.
 */
ULONG CTripData::AddRef()
{
	//TraceTag(tagTripole, "TRIPDATA_AddRef");

	return ++_cRef;
}


/*
 *	Release
 *
 *	Purpose:
 *		Decrements reference count on the specified site.  If count is
 *		decremented to zero, the object is freed.
 *
 *	Returns:
 *		ULONG			New value of reference count.
 */
ULONG CTripData::Release()
{
	ULONG		cRef		= --_cRef;

	//TraceTag(tagTripole, "TRIPDATA_Release");

	if (!cRef)
	{
		//TraceTag(tagTripole, "TRIPDATA_Release: freeing the tripdata");
    	FreeSRowSet(_prws);
		delete this;
	}

	AssertSz(cRef >= 0, "TRIPDATA_Release: negative cRef");
	return cRef;
}


HRESULT CTripData::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
	//TraceTag(tagTripole, "TRIPDATA_GetData");
//$BUG
//	Naughty, naughty, naughty.  You weren't following the OLE2 spec.
//
//	The call to GetDataHereOrThere assumes that pmedium is
//	pre-initialized.  This is fine and dandy for a IDataObject::GetDataHere
//	call, but the OLE2 spec says that the pmedium isn't initialized
//	for IDataObject::GetData.  This fix should probably be taken in
//	for all clients, not just the Mac.
#ifdef MAC
	pmedium->tymed = TYMED_NULL;
	pmedium->pUnkForRelease = NULL;
#endif	// MAC
	return this->GetDataHereOrThere(pformatetcIn, pmedium);
}

HRESULT CTripData::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
	//TraceTag(tagTripole, "TRIPDATA_GetDataHere");

	return this->GetDataHereOrThere(pformatetc, pmedium);
}

/*
 *	GetDataHereOrThere
 *	
 *	Purpose:
 *		Both TRIPDATA_GetData() and TRIPDATA_GetDataHere() do
 *		basically the same thing, so I have them each call this
 *		function.
 *	
 *	Parameters:
 *		pdataobject		an IDataObject
 *		pformatetcIn	the format to get the data
 *		pmedium			where and how to give the data
 *	
 *	Returns:
 *		hr
 */
HRESULT CTripData::GetDataHereOrThere(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
	HRESULT			hr			= S_OK;
	UINT			cbName;
	ULONG			i;
	LPSPropValue	pval;
	LPSTR			pDst;

//	TraceTag(tagTripole, "GetDataHereOrThere");

	if (pformatetcIn->cfFormat == s_rgformatetcTRIPOLE[iFormatTriple].cfFormat)
	{
		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->pUnkForRelease = NULL;
		return (StuffRwsInHglobal(_prws, pmedium));
	}
	else if (pformatetcIn->cfFormat != s_rgformatetcTRIPOLE[iFormatText].cfFormat)
	{
#ifdef DEBUG
		TCHAR	szT[50];

		if (!GetClipboardFormatName(pformatetcIn->cfFormat, szT, sizeof(szT)))
			wsprintf(szT, L"%d", pformatetcIn->cfFormat);
		//TraceTag(tagTripole, "Don't know Clipboard Format %s", szT);
#endif
		return DATA_E_FORMATETC;
	}

	for (i = 0, cbName = 0; i < _prws->cRows; ++i)
	{
		pval = PvalFind(&_prws->aRow[i], PR_DISPLAY_NAME_A);
		Assert(pval);
		cbName += (lstrlenA(pval->Value.lpszA) + 2) * sizeof(TCHAR);
	}

	if (pmedium->tymed == TYMED_NULL ||
			(pmedium->tymed == TYMED_HGLOBAL && pmedium->hGlobal == NULL))
	{
		// This is easy, we can quit right after copying stuff
		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->hGlobal = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, cbName);
		pmedium->pUnkForRelease = NULL;
	}
	else if (pmedium->tymed == TYMED_HGLOBAL && pmedium->hGlobal != NULL)
	{
		// Caller wants us to fill his hGlobal
		// Realloc the destination to make sure there is enough room
        void *pTemp;
        pTemp = GlobalReAlloc(pmedium->hGlobal, cbName, 0);
        if (pTemp == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
		pmedium->hGlobal = pTemp;
	}
	else
		goto Cleanup;

	if (!pmedium->hGlobal)
	{
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	pDst = (LPSTR)GlobalLock(pmedium->hGlobal);
	for (i = 0; i < _prws->cRows; ++i)
	{
		if (i > 0)
		{
			*((LPSTR) pDst) = ';';
			pDst ++;
			*((LPSTR) pDst) = ' ';
			pDst ++;
		}

		pval = PvalFind(&_prws->aRow[i], PR_DISPLAY_NAME_A);
		Assert(pval);
		cbName = (lstrlenA(pval->Value. lpszA) + 1);;
		CopyMemory(pDst, pval->Value.lpszA, cbName);
		pDst += cbName - 1;
	}
	GlobalUnlock(pmedium->hGlobal);

Cleanup:
	RRETURN(hr);
}


HRESULT CTripData::QueryGetData(LPFORMATETC pformatetc)
{
	LONG		iformatetc;
	LPFORMATETC	pformatetcT	= s_rgformatetcTRIPOLE;
	CLIPFORMAT	cfFormat	= pformatetc->cfFormat;
	DWORD		tymed		= pformatetc->tymed;

	//TraceTag(tagTripole, "TRIPDATA_QueryGetData");

	for (iformatetc = 0; iformatetc < cformatetcTRIPOLE;
								++iformatetc, ++pformatetcT)
	{
		// Stop searching if we have compatible formats and mediums
		if (pformatetcT->cfFormat == cfFormat &&
					(pformatetcT->tymed & tymed))
			return S_OK;
	}

	return S_FALSE;
}

HRESULT CTripData::GetCanonicalFormatEtc(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
	//TraceTag(tagTripole, "TRIPDATA_GetCanonicalFormatEtc");

	return DATA_S_SAMEFORMATETC;
}

HRESULT CTripData::SetData(LPFORMATETC pformatetc, LPSTGMEDIUM  pmedium, BOOL fRelease)
{
	//TraceTag(tagTripoleStub, "TRIPDATA_SetData: NYI");

	return E_NOTIMPL;
}

HRESULT CTripData::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC * ppenumFormatEtc)
{
    RRETURN(THR(CreateFORMATETCEnum(s_rgformatetcTRIPOLE, cformatetcTRIPOLE, ppenumFormatEtc)));
}

HRESULT CTripData::DAdvise(LPFORMATETC pFormatetc, DWORD advf,
					LPADVISESINK pAdvSink, DWORD * pdwConnection)
{
	//TraceTag(tagTripoleStub, "TRIPDATA_Advise: NYI");

	return E_NOTIMPL;
}

HRESULT CTripData::DUnadvise(DWORD dwConnection)
{
	//TraceTag(tagTripoleStub, "TRIPDATA_Unadvise: NYI");

	return E_NOTIMPL;
}

HRESULT CTripData::EnumDAdvise(LPENUMSTATDATA * ppenumAdvise)
{
	//TraceTag(tagTripoleStub, "TRIPDATA_EnumAdvise: NYI");

	return E_NOTIMPL;
}


/*
 *	T R I P C A L L   I m p l e m e n t a t i o n
 */


/*
 *	Constructor CTripCall
 *	
 *	Parameters:
 *		pPad			    pointer to CPadMessage this belongs to
 *		hwndEdit			hwnd of this edit control
 *		fUnderline			do we want to underline the triples
 *	
 *	Returns:
 *		TRIPCALL *	The newly created TRIPCALL.
 */
CTripCall::CTripCall(   CPadMessage * pPad,
                        HWND hwndEdit,
                        BOOL fUnderline)
{

	_cRef = 1;					// Start with one reference
	_pPad = pPad;				// Don't need to refcount
	_hwndEdit = hwndEdit;
	_fUnderline = fUnderline;
}


/*
 *	QueryInterface
 *
 *	Purpose:
 *		Returns a pointer to the specified site.
 *
 *	Arguments:
 *		REFIID			Interface we want.
 *		LPUNKNOWN *		Interface we return.
 *
 *	Returns:
 *		HRESULT		 Error status.
 */
HRESULT CTripCall::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	if (IsEqualIID(riid, IID_IUnknown) ||
					IsEqualIID(riid, IID_IRichEditOleCallback))
	{
		*ppvObj = (IRichEditOleCallback*)this;
        AddRef();
		return S_OK;
	}
    else
    {
	    *ppvObj = NULL;
	    return E_NOINTERFACE;
    }
}


/*
 *	AddRef
 *
 *	Purpose:
 *		Increments reference count on the specified site.
 *
 *	Returns:
 *		ULONG			New value of reference count.
 */
ULONG CTripCall::AddRef()
{
	return ++_cRef;
}


/*
 *	Release
 *
 *	Purpose:
 *		Decrements reference count on the specified site.  If count is
 *		decremented to zero, the object is freed.
 *
 *	Returns:
 *		ULONG			New value of reference count.
 */
ULONG CTripCall::Release()
{
	ULONG cRef = --_cRef;

	if (!cRef)
	{

		// Free memory allocated for us
		delete this;
		//TraceTag(tagTripole, "TRIPCALL_Release: freeing the tripcall");
	}

	AssertSz(cRef >= 0, "TRIPCALL_Release: negative cRef");
	return cRef;
}


/*
 *	GetNewStorage
 *
 *	Purpose:
 *		Gets storage for a new object.
 *
 *	Arguments:
 *		LPSTORAGE FAR *		Where to return storage.
 *
 *	Returns:
 *		HRESULT				Error status.
 */
HRESULT CTripCall::GetNewStorage(LPSTORAGE FAR * ppstg)
{
	*ppstg = StgCreateOnHglobal();
	if (!*ppstg)
		return E_OUTOFMEMORY;
	return S_OK;
}


/*
 *	GetInPlaceContext
 *
 *	Purpose:
 *		Gets context information for an in place object.
 *
 *	Arguments:
 *		LPOLEINPLACEFRAME *		Frame window object.
 *		LPOLEINPLACEUIWINDOW *	Document window object.
 *		LPOLEINPLACEFRAMEINFO	Frame window information.
 *
 *	Returns:
 *		HRESULT					Error status.
 */
HRESULT CTripCall::GetInPlaceContext(LPOLEINPLACEFRAME FAR * ppipframe,
    							   LPOLEINPLACEUIWINDOW FAR* ppipuiDoc,
	    						   LPOLEINPLACEFRAMEINFO pipfinfo)
{
	return E_NOTIMPL;
}


/*
 *	ShowContainerUI
 *
 *	Purpose:
 *		Displays or hides REITP's container UI.
 *
 *	Arguments:
 *		BOOL			
 *
 *	Returns:
 *		HRESULT			Error status.
 */
HRESULT CTripCall::ShowContainerUI(BOOL fShow)
{
	return E_NOTIMPL;
}


HRESULT CTripCall::QueryInsertObject(LPCLSID pclsid, LPSTORAGE pstg, LONG cp)
{
	if (IsEqualIID(*pclsid, CLSID_CTriple))
		return S_OK;
	else
		return E_FAIL;
}


HRESULT CTripCall::DeleteObject(LPOLEOBJECT poleobj)
{
	return S_OK;
}


HRESULT CTripCall::QueryAcceptData(LPDATAOBJECT pdataobj, CLIPFORMAT *pcfFormat,
                                    DWORD reco, BOOL fReally,
		                            HGLOBAL hMetaPict)
{
	HRESULT			hr;
	ULONG			cRows;
	ULONG			iRow;
	ULONG			cb;
	LPBYTE			pb;
	SRow			rw;
	STGMEDIUM		stgmedium;

	//TraceTag(tagTripole, "TRIPCALL_QueryAcceptData");

	if (!*pcfFormat)
	{
		// default to text
		*pcfFormat = CF_TEXT;

		if (pdataobj->QueryGetData(&CTripData::s_rgformatetcTRIPOLE[iFormatTriple]) == S_OK)
		{
			// Goody, goody, this data object support CF_TRIPLE
			*pcfFormat = CTripData::s_rgformatetcTRIPOLE[iFormatTriple].cfFormat;
		}
	}
	else
	{
		if (*pcfFormat != CTripData::s_rgformatetcTRIPOLE[iFormatTriple].cfFormat
				 && *pcfFormat != CTripData::s_rgformatetcTRIPOLE[iFormatText].cfFormat)
			return DATA_E_FORMATETC;
	}

	if (*pcfFormat == CF_TEXT)
	{
		// let the richedit take care of text
		return S_OK;
	}

	// If I'm read-only, return Success and Richedit won't do anything
	if (GetWindowLong(_hwndEdit, GWL_STYLE) & ES_READONLY)
		return S_OK;

	if (!fReally)
	{
		// return that we'll import it ourselves
 		return S_FALSE;
	}

	hr = pdataobj->GetData(&CTripData::s_rgformatetcTRIPOLE[iFormatTriple], &stgmedium);
	if (hr != S_OK)
		RRETURN(hr);

    // STGFIX: t-gpease 8-13-97
    Assert(stgmedium.tymed == TYMED_HGLOBAL);

	pb = (BYTE*)GlobalLock(stgmedium.hGlobal);
	cRows = *((ULONG *) pb);
	pb += sizeof(ULONG);

	for (iRow = 0; iRow < cRows; ++iRow)
	{
		cb = *((ULONG *) pb);
		pb += sizeof(ULONG);

		hr = THR(MAPIAllocateBuffer(cb, (void**)&rw.lpProps));
		if (hr)
			goto Cleanup;

		CopyBufferToRow(pb, &rw);
		pb += cb;

		if (iRow > 0)
			Edit_ReplaceSel(_hwndEdit, TEXT("; "));

		//$ REVIEW - if we fail, do I want to do anything?

		_pPad->AddRecipientToWell(_hwndEdit, (LPADRENTRY) &rw, FALSE, FALSE);

		MAPIFreeBuffer(rw.lpProps);
	}

	hr = S_FALSE;

Cleanup:
	GlobalUnlock(stgmedium.hGlobal);
	GlobalFree(stgmedium.hGlobal);
	return hr;
}


HRESULT CTripCall::ContextSensitiveHelp(BOOL fEnterMode)
{
	return S_OK;
}


HRESULT CTripCall::GetClipboardData(CHARRANGE * pchrg,
					DWORD reco, LPDATAOBJECT * ppdataobj)
{
	HRESULT     hr;
	LPADRLIST	pal = NULL;
	CTripData *	ptripdata;

	//$ REVIEW - possible optimization - return E_NOTIMPL if no objects
	//				in selection

	// Exchange 18403: Need to prevent cut on read only
	if (reco == RECO_CUT &&
		(GetWindowStyle(_hwndEdit) & ES_READONLY))
		return E_NOTIMPL;

	hr = THR(_pPad->BuildSelectionAdrlist(&pal, _hwndEdit, pchrg));
	if (hr)
		goto Error;

	// this will gobble up the pal so I don't want to free it
	ptripdata = new CTripData((LPSRowSet) pal);
	if (!ptripdata)
		goto Error;

	// steal the refcount of the ptripdata
	*ppdataobj = (LPDATAOBJECT) ptripdata;

	return S_OK;

Error:
	FreeAdrlist(pal);
	RRETURN(E_FAIL);
}


HRESULT CTripCall::GetDragDropEffect(BOOL fDrag, DWORD grfKeyState, LPDWORD pdwEffect)
{
	if (fDrag)			// use the default
		return S_OK;

	if (GetWindowLong(_hwndEdit, GWL_STYLE) & ES_READONLY)
	{
		*pdwEffect = DROPEFFECT_NONE;
	}
	else
	{
		if ((grfKeyState & MK_CONTROL) || !(*pdwEffect & DROPEFFECT_MOVE))
			*pdwEffect = DROPEFFECT_COPY;
		else
			*pdwEffect = DROPEFFECT_MOVE;
	}

	return S_OK;
}


HRESULT CTripCall::GetContextMenu(WORD seltype,	LPOLEOBJECT poleobj, CHARRANGE * pchrg,
                                    HMENU * phmenu)
{
	return E_NOTIMPL;
}
