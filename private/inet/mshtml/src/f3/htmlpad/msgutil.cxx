//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       paddoc.cxx
//
//  Contents:   CPadDoc class.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_MSG_HXX_
#define X_MSG_HXX_
#include "msg.hxx"
#endif

/*
 *	PvalFind
 *	
 *	Purpose:
 *		Given the row prw, finds the first property whose tag is
 *		ulPropTag.  
 *	
 *	Arguments:
 *		prw			The row to seek.
 *		ulPropTag	The property tag we're seeking.
 *	
 *	Returns:
 *		A pointer to the SPropValue with the desired property tag, or
 *		NULL if no such SPropValue was found.
 */
LPSPropValue PvalFind(LPSRow prw, ULONG ulPropTag)
{
	UINT			ival;
	LPSPropValue	pval;

	ival = (UINT) prw->cValues;
	pval = prw->lpProps;
	if (PROP_TYPE(ulPropTag) != PT_NULL)
	{
		while (ival--)
		{
			if (pval->ulPropTag == ulPropTag)
				return pval;
			++pval;
		}
	}
	else	// Ignore property type (error)
	{
		ULONG ulPropId = PROP_ID(ulPropTag);

		while (ival--)
		{
			if (PROP_ID(pval->ulPropTag) == ulPropId)
				return pval;
			++pval;
		}
	}
	return NULL;
}


/*
 *	FreeSRowSet
 *
 *	Purpose:
 *		Frees an SRowSet structure and the rows therein
 *
 *	Parameters:
 *		LPSRowSet		The row set to free
 */

VOID FreeSRowSet(LPSRowSet prws)
{
	ULONG irw;

	if (!prws)
		return;

	// Free each row
	for (irw = 0; irw < prws->cRows; irw++)
		MAPIFreeBuffer(prws->aRow[irw].lpProps);

	// Free the top level structure
	MAPIFreeBuffer(prws);
}


/*
 *	CopyRow
 *	
 *	Purpose:
 *		This function copies the properties of one row to another. If <pv> is
 *		NULL then MAPIAllocateBuffer is used to allocate the storage for
 *		<prwDst>. If <pv> is non NULL it is assumed to be a pointer to a
 *		MAPIAllocateBuffer(d) chunk of memory so MAPIAllocateMore is used to
 *		allocate the storage for <prwDst>.
 *	
 *	Parameters:
 *		pv			in		chunk of memory
 *		prwSrc		in		source row
 *		prwDst		out		destination row
 *	
 *	Returns:
 *		sc
 */
HRESULT CopyRow(LPVOID pv, LPSRow prwSrc, LPSRow prwDst)
{
	HRESULT	        hr = S_OK;
	ULONG			iCol;
	LPSPropValue	pvalSrc;
	LPSPropValue	pvalDst;
	BYTE *			pb;
	ULONG			cb;

	if (prwSrc->cValues == 0)
	{
		// Handle the empty row case (Raid 3300)

		prwDst->cValues = 0;
		prwDst->lpProps = NULL;
		return S_OK;
	}

	// Compute the size needed to store the row in its entirety.

	pvalSrc = prwSrc->lpProps;
	for (iCol = prwSrc->cValues, cb = 0L; iCol > 0; ++pvalSrc, --iCol)
	{
		cb += PAD4(CbForPropValue(pvalSrc)) + sizeof (SPropValue);
	}

	// Allocate enough room for the new copy of the SPropValue.

	if (pv)
		hr = THR(MAPIAllocateMore(cb, pv, (void**)&prwDst->lpProps));
	else
		hr = THR(MAPIAllocateBuffer(cb, (void**)&prwDst->lpProps));

	if (hr)
		goto Cleanup;

	// Do the finicky copy of the items one-by-one.

	pvalDst = prwDst->lpProps;
	pvalSrc = prwSrc->lpProps;
	iCol = prwDst->cValues = prwSrc->cValues;
	pb = (BYTE *) (pvalDst + iCol);		// point past the rgSPropValue.

	while (iCol--)
	{
		pb = CopyPval(pvalSrc, pvalDst, pb, NULL, imodeCopy);
		++pvalSrc;
		++pvalDst;
	}
	
Cleanup:
    RRETURN(hr);    
}


/*
 *	CbForPropValue
 *	
 *	Purpose:
 *		Determines how many bytes are needed for storage of the
 *		property pv, in addition to the space taken up by the SPropValue
 *		itself. 
 *	Arguments:
 *		pv		The SPropValue of the property.
 *	Returns:
 *		A count of the bytes needed to store this thing
 */
ULONG CbForPropValue(LPSPropValue pval)
{
	ULONG	cb;
	
	switch (PROP_TYPE(pval->ulPropTag))
	{
		// Types whose values fit in the 64-bit Value of the property.
	  case PT_I2:
	  case PT_I8:
	  case PT_LONG:
	  case PT_R4:
	  case PT_DOUBLE:
	  case PT_CURRENCY:
	  case PT_ERROR:
	  case PT_BOOLEAN:
	  case PT_SYSTIME:
	  case PT_APPTIME:
	  case PT_NULL:
		return 0;
	  case PT_BINARY:
		cb = pval->Value.bin.cb;
		break;
	  case PT_STRING8:
		cb = (lstrlenA(pval->Value.lpszA) + 1) * sizeof (CHAR);
		break;
	  case PT_UNICODE:
		cb = (lstrlenW(pval->Value.lpszW) + 1) * sizeof (WCHAR);
		break;
	  case PT_CLSID:
		cb = sizeof (GUID);
		break;

		// Multi-valued properties.
	  case PT_MV_I2:
		cb = pval->Value.MVi.cValues * sizeof (short int);
		break;
	  case PT_MV_LONG:
		cb = pval->Value.MVl.cValues * sizeof (LONG);
		break;
	  case PT_MV_R4:
		cb = pval->Value.MVflt.cValues * sizeof (float);
		break;
	  case PT_MV_DOUBLE:
	  case PT_MV_APPTIME:
		cb = pval->Value.MVdbl.cValues * sizeof (double);
		break;
	  case PT_MV_CURRENCY:
		cb = pval->Value.MVcur.cValues * sizeof (CURRENCY);
		break;
	  case PT_MV_SYSTIME:
		cb = pval->Value.MVat.cValues * sizeof (FILETIME);
		break;
//$	case PT_MV_I8:
//		cb = pval->Value.MVli.cValues * sizeof (LARGE_INTEGER);
		break;
	  case PT_MV_BINARY:
		{
		//	Multi-valued binaries are copied in memory into a single
		//	allocated buffer in the following way:
		//
		//		cb1, pb1 ... cbn, pbn, b1,0, b1,1 ... b2,0 b2,1 ...
		//
		//	The cbn and pbn parameters form the SBinary array that
		//	will be pointed to by pvalDst->Value.MVbin.lpbin.
		//	The remainder of the allocation is used to store the binary
		//	data for each of the elements of the array.  Thus pb1 points
		//	to the b1,0, etc.

			SBinaryArray *	pbina = &pval->Value.MVbin;
			ULONG			uliValue;
			SBinary *		pbinSrc;

			cb = pbina->cValues * sizeof (SBinary);

			for (uliValue = 0, pbinSrc = pbina->lpbin;
					uliValue < pbina->cValues;
					uliValue++, pbinSrc++)
			{
				cb += pbinSrc->cb;
			}
			break;
		}
	  case PT_MV_STRING8:
		{
		//	Multi-valued STRING8 properties are copied into a single
		//	allocated block of memory in the following way:
		//
		//		|          Allocated buffer             |
		//		|---------------------------------------|
		//		| pszA1, pszA2 ... | szA1[], szA2[] ... |
		//		|------------------|--------------------|
		//		|   LPSTR array    |     String data    |
		//
		//	Where pszAn are the elements of the LPSTR array pointed
		//	to by pvalDst->Value.MVszA.  Each pszAn points
		//	to its corresponding string, szAn, stored later in the
		//	buffer.  The szAn are stored starting at the first byte
		//	past the end of the LPSTR array.

			SLPSTRArray *	pSLPSTRArray = &pval->Value.MVszA;
			ULONG			uliValue;
			LPSTR *			pszASrc;

			//	Figure out the size of the buffer we need

			cb = pSLPSTRArray->cValues * sizeof (LPSTR);

			for (uliValue = 0, pszASrc = pSLPSTRArray->lppszA;
					uliValue < pSLPSTRArray->cValues;
					uliValue++, pszASrc++)
			{
				cb += (lstrlenA(*pszASrc) + 1) * sizeof (CHAR);
			}
			break;
		}

	  case PT_MV_UNICODE:
		{
		//	Multi-valued UNICODE properties are copied into a single
		//	allocated block of memory in the following way:
		//
		//		|          Allocated buffer             |
		//		|---------------------------------------|
		//		| pszW1, pszW2 ... | szW1[], szW2[] ... |
		//		|------------------|--------------------|
		//		|   LPWSTR array   |     String data    |
		//
		//	Where pszWn are the elements of the LPWSTR array pointed
		//	to by pvalDst->Value.MVszW.  Each pszWn points
		//	to its corresponding string, szWn, stored later in the
		//	buffer.  The szWn are stored starting at the first byte
		//	past the end of the LPWSTR array.

			SWStringArray *	pSWStringArray = &pval->Value.MVszW;
			ULONG			uliValue;
			LPWSTR *		pszWSrc;

			//	Figure out the size of the buffer we need

			cb = pSWStringArray->cValues * sizeof (LPWSTR);

			for (uliValue = 0, pszWSrc = pSWStringArray->lppszW;
					uliValue < pSWStringArray->cValues;
					uliValue++, pszWSrc++)
			{
				cb += (lstrlenW(*pszWSrc) + 1) * sizeof (WCHAR);
			}
			break;
		}
	  case PT_UNSPECIFIED:
	  case PT_OBJECT:
	  default:
		cb = 0;
		break;
	}
	return cb;
}


/*
 *	CopyPropValue
 *	
 *	Purpose:
 *		Copies the property from pvSrc to pvDst. This is done by copying
 *		the LPSPropValue at pvSrc to pvDst, and copying any external data
 *		to pbIn.
 *	Arguments:
 *		pvSrc		Source property
 *		pvDst		Destination property
 *		pb			Pointer to buffer area for strings, binaries, &c.
 *		ppbDest		(for imodeUnflatten) pb for destination row
 *		imode		One of:
 *						imodeCopy		- straight row copy
 *						imodeFlatten	- flatten out row to buffer
 *						imodeUnflatten	- unflatten buffer to row
 *	
 *	Returns:
 *		The updated PB, which should be on an aligned boundary.
 */
LPBYTE CopyPval(LPSPropValue pvalSrc, LPSPropValue pvalDst,
			   LPBYTE pb, LPBYTE * ppbDest, UINT imode)
{
	ULONG			cb;
	LPBYTE			pbSrc;
	LPBYTE FAR *	ppbDst;

#if defined(_ALPHA_)
	// Bug 26150
	CopyMemory(pvalDst, pvalSrc, sizeof(SPropValue));
#else
	*pvalDst = *pvalSrc;
#endif
	switch (PROP_TYPE(pvalDst->ulPropTag))
	{
		//	Types whose values fit in the 64-bit Value of the property
	  case PT_I2:
	  case PT_I8:
	  case PT_LONG:
	  case PT_R4:
	  case PT_DOUBLE:
	  case PT_CURRENCY:
	  case PT_ERROR:
	  case PT_BOOLEAN:
	  case PT_SYSTIME:
	  case PT_APPTIME:
	  case PT_NULL:
		return pb;
	}
	
	switch (PROP_TYPE(pvalDst->ulPropTag))
	{
	  case PT_BINARY:
		pbSrc	= pvalSrc->Value.bin.lpb;
		ppbDst	= &pvalDst->Value.bin.lpb;
		break;

	  case PT_STRING8:
		pbSrc	= (LPBYTE) pvalSrc->Value.lpszA;
		ppbDst	= (LPBYTE *) &pvalDst->Value.lpszA;
		break;

	  case PT_UNICODE:
		pbSrc	= (LPBYTE) pvalSrc->Value.lpszW;
		ppbDst	= (LPBYTE *) &pvalDst->Value.lpszW;
		break;

	  case PT_CLSID:
		pbSrc	= (LPBYTE) pvalSrc->Value.lpguid;
		ppbDst	= (LPBYTE *) &pvalDst->Value.lpguid;
		break;

	  case PT_MV_I2:
		pbSrc	= (LPBYTE) pvalSrc->Value.MVi.lpi;
		ppbDst	= (LPBYTE *) &pvalDst->Value.MVi.lpi;
		break;

	  case PT_MV_LONG:
		pbSrc	= (LPBYTE) pvalSrc->Value.MVl.lpl;
		ppbDst	= (LPBYTE *) &pvalDst->Value.MVl.lpl;
		break;

	  case PT_MV_R4:
		pbSrc	= (LPBYTE) pvalSrc->Value.MVflt.lpflt;
		ppbDst	= (LPBYTE *) &pvalDst->Value.MVflt.lpflt;
		break;

	  case PT_MV_DOUBLE:
	  case PT_MV_APPTIME:
		pbSrc	= (LPBYTE) pvalSrc->Value.MVdbl.lpdbl;
		ppbDst	= (LPBYTE *) &pvalDst->Value.MVdbl.lpdbl;
		break;

	  case PT_MV_CURRENCY:
		pbSrc	= (LPBYTE) pvalSrc->Value.MVcur.lpcur;
		ppbDst	= (LPBYTE *) &pvalDst->Value.MVcur.lpcur;
		break;

	  case PT_MV_SYSTIME:
		pbSrc	= (LPBYTE) pvalSrc->Value.MVat.lpat;
		ppbDst	= (LPBYTE *) &pvalDst->Value.MVat.lpat;
		break;

//$	  case PT_MV_I8:
// 		pbSrc	= (LPBYTE) pvalSrc->Value.MVli.lpli;
// 		ppbDst	= (LPBYTE *) &pvalDst->Value.MVli.lpli;
// 		break;

	  case PT_MV_BINARY:
		{
		//	Multi-valued binaries are copied in memory into a single
		//	allocated buffer in the following way:
		//
		//		cb1, pb1 ... cbn, pbn, b1,0, b1,1 ... b2,0 b2,1 ...
		//
		//	The cbn and pbn parameters form the SBinary array that
		//	will be pointed to by pvalDst->Value.MVbin.lpbin.
		//	The remainder of the allocation is used to store the binary
		//	data for each of the elements of the array.  Thus pb1 points
		//	to the b1,0, etc.

		//	Multi-valued binaries are flattened in memory into a single
		//	allocated buffer in the following way:
		//
		//		cb1, ib1 ... cbn, ibn, b1,0, b1,1 ... b2,0 b2,1 ...
		//
		//	The cbn and ibn parameters form the SBinary array that
		//	will be pointed to by pvalDst->Value.MVbin.lpbin.
		//	The remainder of the allocation is used to store the binary
		//	data for each of the elements of the array.  Thus ib1 contains
		//	the offset from the beginning of the allocated buffer
		//	to the b1,0, etc.
		//	The pvalDst->Value.MVbin.lpbin contains the size of the allocated
		//	buffer.

			SBinaryArray *	pbina = &pvalSrc->Value.MVbin;
			ULONG			iVal;
			SBinary *		pbinSrc;
			SBinary *		pbinDst;

			if (imode == imodeCopy)
			{
				//	And copy it all in

				pbinDst = (LPSBinary) pb;
				pvalDst->Value.MVbin.lpbin = pbinDst;
				pb = (LPBYTE) (pbinDst + pbina->cValues);

				for (iVal = 0, pbinSrc = pbina->lpbin;
						iVal < pbina->cValues;
						iVal++, pbinSrc++, pbinDst++ )
				{
					pbinDst->cb = pbinSrc->cb;
					pbinDst->lpb = pb;
					CopyMemory(pb, pbinSrc->lpb, (UINT) pbinSrc->cb);
					pb += pbinSrc->cb;
				}
			}
			else if (imode == imodeFlatten)
			{
				ULONG		ib;

				//	Copy the data into the buffer and set offsets
				//	to them in the SBinary array at the beginning of the buffer

				pbinDst = (LPSBinary) pb;
				pb = (LPBYTE) (pbinDst + pbina->cValues);
				ib = pbina->cValues * sizeof(SBinary);

				//	Copy the data into the buffer and set offsets
				//	to them in the SBinary array at the beginning of the buffer

				for (iVal = 0, pbinSrc = pbina->lpbin;
						iVal < pbina->cValues;
						iVal++, pbinSrc++, pbinDst++ )
				{
					pbinDst->cb = pbinSrc->cb;
					pbinDst->lpb = (LPBYTE) ib;
					CopyMemory(pb, pbinSrc->lpb, (UINT) pbinSrc->cb);
					pb += pbinSrc->cb;
					ib += pbinSrc->cb;
				}

				// Flattening loses pointers so we use the pointer value to
				// hold the total size.
				pvalDst->Value.MVbin.lpbin = (LPSBinary) ib;
			}
			else
			{
				ULONG		cbTotal;
				ULONG 		ulBase;

				// Pick up the size of the entire block and copy it over
				cbTotal = (ULONG) pbina->lpbin;
                if (cbTotal)
				{
					CopyMemory(*ppbDest, pb, cbTotal);

					//	Convert the offsets to data back into pointers to
					//	strings

					pvalDst->Value.MVbin.lpbin = pbinDst = (LPSBinary) *ppbDest;
					ulBase = (ULONG) pbinDst;
					for (iVal = 0;
							iVal < pbina->cValues;
							iVal++, pbinDst++)
					{
						pbinDst->lpb = (LPBYTE) (ulBase + (ULONG) pbinDst->lpb);
					}

					//	Bump the source and destination pointers
					*ppbDest += cbTotal;
					pb += cbTotal;
				}
			}
			return pb;
		}

	  case PT_MV_STRING8:
		{
		//	Multi-valued STRING8 properties are copied into a single
		//	allocated block of memory in the following way:
		//
		//		|          Allocated buffer             |
		//		|---------------------------------------|
		//		| pszA1, pszA2 ... | szA1[], szA2[] ... |
		//		|------------------|--------------------|
		//		|   LPSTR array    |     String data    |
		//
		//	Where pszAn are the elements of the LPSTR array pointed
		//	to by pvalDst->Value.MVszA.  Each pszAn points
		//	to its corresponding string, szAn, stored later in the
		//	buffer.  The szAn are stored starting at the first byte
		//	past the end of the LPSTR array.

		//	Multi-valued STRING8 properties are flattened into a single
		//	allocated block of memory in the following way:
		//
		//		|          Allocated buffer             |
		//		|---------------------------------------|
		//		| ibA1, ibA2 ...   | szA1[], szA2[] ... |
		//		|------------------|--------------------|
		//		|   LPSTR array    |     String data    |
		//
		//	Where ibAn are the elements of the LPSTR array pointed
		//	to by pvalDst->Value.MVszA.  Each ibAn contains an offset
		//	the beginning of the allocated buffer 
		//	to its corresponding string, szAn, stored later in the
		//	buffer.  The szAn are stored starting at the first byte
		//	past the end of the LPSTR array.
		//	The pvalDst->Value.MVszA contains the size of the allocated
		//	buffer.

			SLPSTRArray *	pSLPSTRArray = &pvalSrc->Value.MVszA;
			ULONG			iVal;
			LPSTR *			pszSrc;
			LPSTR *			pszDst;

			if (imode == imodeCopy)
			{
				//	Allocate the buffer to hold the strings

				pszDst = (LPSTR *) pb;
				pvalDst->Value.MVszA.lppszA = pszDst;
				pb = (LPBYTE) (pszDst + pSLPSTRArray->cValues);

				//	Copy the strings into the buffer and set pointers
				//	to them in the LPSTR array at the beginning of the buffer

				for (iVal = 0, pszSrc = pSLPSTRArray->lppszA;
						iVal < pSLPSTRArray->cValues;
						iVal++, pszSrc++, pszDst++ )
				{
					cb = (lstrlenA(*pszSrc) + 1) * sizeof (CHAR);

					*pszDst = (LPSTR) pb;
					CopyMemory(pb, (LPBYTE) *pszSrc, (UINT) cb);
					pb += cb;
				}
			}
			else if (imode == imodeFlatten)
			{
				ULONG		ib;

				//	Copy the strings into the buffer and set offsets
				//	to them in the LPSTR array at the beginning of the buffer

				pszDst = (LPSTR *) pb;
				pb = (LPBYTE) (pszDst + pSLPSTRArray->cValues);
				ib = pSLPSTRArray->cValues * sizeof(LPSTR *);

				//	Copy the strings into the buffer and set offsets
				//	to them in the LPSTR array at the beginning of the buffer

				for (iVal = 0, pszSrc = pSLPSTRArray->lppszA;
						iVal < pSLPSTRArray->cValues;
						iVal++, pszSrc++, pszDst++ )
				{
					cb = (lstrlenA(*pszSrc) + 1) * sizeof (CHAR);

					*pszDst = (LPSTR) ib;
					CopyMemory(pb, (LPBYTE) *pszSrc, (UINT) cb);
					pb += cb;
					ib += cb;
				}

				// Flattening loses pointers so we use the pointer value to
				// hold the total size.
				pvalDst->Value.MVszA.lppszA = (LPSTR *) ib;
			}
			else
			{
				ULONG		cbTotal;
				ULONG 		ulBase;

				// Pick up the size of the entire block and copy it over
				cbTotal = (ULONG) pSLPSTRArray->lppszA;
                if (cbTotal)
				{
					CopyMemory(*ppbDest, pb, cbTotal);

					//	Convert the offsets to strings back into pointers to
					//	strings

					pvalDst->Value.MVszA.lppszA = pszDst = (LPSTR *) *ppbDest;
					ulBase = (ULONG) pszDst;
					for (iVal = 0;
							iVal < pSLPSTRArray->cValues;
							iVal++, pszDst++)
					{
						*pszDst = (LPSTR) (ulBase + (ULONG) *pszDst);
					}

					//	Bump the source and destination pointers
					*ppbDest += cbTotal;
					pb += cbTotal;
				}
			}
			return pb;
		}

	  case PT_MV_UNICODE:
		{
		//	Multi-valued UNICODE properties are copied into a single
		//	allocated block of memory in the following way:
		//
		//		|          Allocated buffer             |
		//		|---------------------------------------|
		//		| pszW1, pszW2 ... | szW1[], szW2[] ... |
		//		|------------------|--------------------|
		//		|   LPWSTR array   |     String data    |
		//
		//	Where pszWn are the elements of the LPWSTR array pointed
		//	to by pvalDst->Value.MVszW.  Each pszWn points
		//	to its corresponding string, szWn, stored later in the
		//	buffer.  The szWn are stored starting at the first byte
		//	past the end of the LPWSTR array.

		//	Multi-valued UNICODE properties are flattened into a single
		//	allocated block of memory in the following way:
		//
		//		|          Allocated buffer             |
		//		|---------------------------------------|
		//		| ibW1, ibW2 ...   | szW1[], szW2[] ... |
		//		|------------------|--------------------|
		//		|   LPSTR array    |     String data    |
		//
		//	Where ibWn are the elements of the LPWSTR array pointed
		//	to by pvalDst->Value.MVszW.  Each ibWn contains an offset
		//	the beginning of the allocated buffer 
		//	to its corresponding string, szWn, stored later in the
		//	buffer.  The szWn are stored starting at the first byte
		//	past the end of the LPWSTR array.
		//	The pvalDst->Value.MVszW contains the size of the allocated
		//	buffer.

			SWStringArray *	pSWStringArray = &pvalSrc->Value.MVszW;
			ULONG			iVal;
			LPWSTR *		pszSrc;
			LPWSTR *		pszDst;

			if (imode == imodeCopy)
			{
				//	Allocate the buffer to hold the strings

				pszDst = (LPWSTR *) pb;
				pvalDst->Value.MVszW.lppszW = pszDst;
				pb = (LPBYTE) (pszDst + pSWStringArray->cValues);

				//	Copy the strings into the buffer and set pointers
				//	to them in the LPWSTR array at the beginning of the buffer

				for (iVal = 0, pszSrc = pSWStringArray->lppszW;
						iVal < pSWStringArray->cValues;
						iVal++, pszSrc++, pszDst++ )
				{
					cb = (lstrlenW(*pszSrc) + 1) * sizeof (WCHAR);

					*pszDst = (LPWSTR) pb;
					CopyMemory(pb, (LPBYTE) *pszSrc, cb);
					pb += cb;
				}
			}
			else if (imode == imodeFlatten)
			{
				ULONG		ib;

				//	Copy the strings into the buffer and set offsets
				//	to them in the LPWSTR array at the beginning of the buffer

				pszDst = (LPWSTR *) pb;
				pb = (LPBYTE) (pszDst + pSWStringArray->cValues);
				ib = pSWStringArray->cValues * sizeof(LPWSTR *);

				//	Copy the strings into the buffer and set offsets
				//	to them in the LPWSTR array at the beginning of the buffer

				for (iVal = 0, pszSrc = pSWStringArray->lppszW;
						iVal < pSWStringArray->cValues;
						iVal++, pszSrc++, pszDst++ )
				{
					cb = (lstrlenW(*pszSrc) + 1) * sizeof (WCHAR);

					*pszDst = (LPWSTR) ib;
					CopyMemory(pb, (LPBYTE) *pszSrc, (UINT) cb);
					pb += cb;
					ib += cb;
				}

				// Flattening loses pointers so we use the pointer value to
				// hold the total size.
				pvalDst->Value.MVszW.lppszW = (LPWSTR *) ib;
			}
			else
			{
				ULONG		cbTotal;
				ULONG		ulBase;

				// Pick up the size of the entire block and copy it over
				cbTotal = (ULONG) pSWStringArray->lppszW;
                if (cbTotal)
				{
					CopyMemory(*ppbDest, pb, cbTotal);

					//	Convert the offsets to strings back into pointers to
					//	strings

					pvalDst->Value.MVszW.lppszW = pszDst = (LPWSTR *) *ppbDest;
					ulBase = (ULONG) pszDst;
					for (iVal = 0;
							iVal < pSWStringArray->cValues;
							iVal++, pszDst++)
					{
						*pszDst = (LPWSTR) (ulBase + (ULONG) *pszDst);
					}

					//	Bump the source and destination pointers
					*ppbDest += cbTotal;
					pb += cbTotal;
				}
			}
			return pb;
		}

	  case PT_UNSPECIFIED:
	  case PT_OBJECT:
	  default:
		return pb;
	}

	if (imode == imodeUnflatten)
	{
		cb = pvalSrc->Value.bin.cb;
		*ppbDst = *ppbDest;
		CopyMemory(*ppbDest, pb, (size_t) cb);
		*ppbDest += cb;
	}
	else
	{
		cb = CbForPropValue(pvalSrc);
		if (imode == imodeFlatten)
		{
			pvalDst->Value.bin.cb = cb;
		}
		else
		{
			Assert(imode == imodeCopy);
			*ppbDst = pb;
		}

		CopyMemory(pb, pbSrc, (size_t) cb);
	}
	
	return pb + PAD4(cb);	// make sure next one will be dword-aligned
}


