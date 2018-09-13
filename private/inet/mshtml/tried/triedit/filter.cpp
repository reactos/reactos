// filter.cpp : Implementation of filtering/parsing of the document
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#include "stdafx.h"

#include "triedit.h"
#include "Document.h"
#include "guids.h"
#include "HtmParse.h"

STDMETHODIMP CTriEditDocument::FilterIn(IUnknown *pUnkOld, IUnknown **ppUnkNew, DWORD dwFlags, BSTR bstrBaseURL)
{
    HGLOBAL hOld, hNew;
    HRESULT hr;
    IStream *pStmOld;
    UINT    chSize;
    ULARGE_INTEGER li;
    int cbSizeIn = -1;
    STATSTG stat;

    if (pUnkOld == NULL)
        return E_INVALIDARG;

    hr = pUnkOld->QueryInterface(IID_IStream, (void **) &pStmOld);
    if (hr != S_OK)
        return E_INVALIDARG;

    if ((hr = pStmOld->Stat(&stat, STATFLAG_NONAME)) == S_OK)
    {
        cbSizeIn = stat.cbSize.LowPart;
        _ASSERTE(stat.cbSize.HighPart == 0); // This will ensure that we don't have a doc larger than 4 gigabytes
    }

    if (GetHGlobalFromStream(pStmOld, &hOld) != S_OK)
    {
        pStmOld->Release();
        return E_INVALIDARG;
    }

    if (!(dwFlags & dwFilterUsePstmNew))
        *ppUnkNew = NULL;

    hr = DoFilter(hOld, &hNew, (IStream*) *ppUnkNew, dwFlags, modeInput, cbSizeIn, &chSize, bstrBaseURL);
#ifdef IE5_SPACING
    if (!(dwFlags & dwFilterNone) && hr == S_OK)
        SetFilterInDone(TRUE);
#endif //IE5_SPACING
    if (hr != S_OK)
    {
        pStmOld->Release();
        return hr;
    }

    if (!(dwFlags & dwFilterUsePstmNew))
    {
        _ASSERTE(hNew != NULL);
        hr = CreateStreamOnHGlobal(hNew, TRUE, (IStream **) ppUnkNew);
        if (FAILED(hr))
            GlobalFree(hNew);
    }
        
    if (SUCCEEDED(hr))
    {
        li.LowPart = chSize;
        li.HighPart = 0;
        (*((IStream**)ppUnkNew))->SetSize(li);
    }

    pStmOld->Release();
    return hr;
}

STDMETHODIMP CTriEditDocument::FilterOut(IUnknown *pUnkOld, IUnknown **ppUnkNew, DWORD dwFlags, BSTR bstrBaseURL)
{
    HGLOBAL hOld, hNew;
    HRESULT hr;
    IStream *pStmOld;
    UINT    chSize;
    ULARGE_INTEGER li;
    int cbSizeIn = -1;
    STATSTG stat;

    if (pUnkOld == NULL)
        return E_INVALIDARG;

    hr = pUnkOld->QueryInterface(IID_IStream, (void **) &pStmOld);
    if (hr != S_OK)
        return E_INVALIDARG;

    if ((hr = pStmOld->Stat(&stat, STATFLAG_NONAME)) == S_OK)
    {
        cbSizeIn = stat.cbSize.LowPart;
        _ASSERTE(stat.cbSize.HighPart == 0); // This will ensure that we don't have a doc larger than 4 gigabytes
    }

    if (GetHGlobalFromStream(pStmOld, &hOld) != S_OK)
    {
        pStmOld->Release();
        return E_INVALIDARG;
    }

    if (!(dwFlags & dwFilterUsePstmNew))
        *ppUnkNew = NULL;

    hr = DoFilter(hOld, &hNew, (IStream *) *ppUnkNew, dwFlags, modeOutput, cbSizeIn, &chSize, bstrBaseURL);
    if (hr != S_OK)
    {
        pStmOld->Release();
        return hr;
    }

    if (!(dwFlags & dwFilterUsePstmNew))
    {
        _ASSERTE(hNew != NULL);
        hr = CreateStreamOnHGlobal(hNew, TRUE, (IStream **) ppUnkNew);
        if (FAILED(hr))
            GlobalFree(hNew);
    }

    if (SUCCEEDED(hr))
    {
        li.LowPart = chSize;
        li.HighPart = 0;
        (*((IStream**)ppUnkNew))->SetSize(li);
    }

    pStmOld->Release();
    return hr;
}

HRESULT CTriEditDocument::DoFilter(HGLOBAL hOld, HGLOBAL *phNew, IStream *pStmNew, DWORD dwFlags, FilterMode mode, int cbSizeIn, UINT* pcbSizeOut, BSTR bstrBaseURL)
{
    HRESULT hr;
    HGLOBAL hgTokArray;
    UINT cMaxToken;

    // Create tokenizer if it hasn't yet been created
    if (m_pTokenizer == NULL)
    {
        hr = ::CoCreateInstance(CLSID_TriEditParse, NULL, CLSCTX_INPROC_SERVER, IID_ITokenGen, (void **)&m_pTokenizer);
        if (hr != S_OK)
            return hr;
    }

    _ASSERTE(m_pTokenizer != NULL);

    _ASSERTE(dwFilterDefaults == 0);
    if ((dwFlags & ~(dwFilterMultiByteStream|dwFilterUsePstmNew)) == dwFilterDefaults) // means that caller wants us to set the flags
    {
        dwFlags |= (dwFilterDTCs|dwFilterServerSideScripts|dwPreserveSourceCode);
    }

    hr = m_pTokenizer->hrTokenizeAndParse(hOld, phNew, pStmNew, dwFlags, mode, cbSizeIn, pcbSizeOut, m_pUnkTrident, &hgTokArray, &cMaxToken, &m_hgDocRestore, bstrBaseURL, 0/*dwReserved*/);

    if (hgTokArray != NULL)
    {
        GlobalFree(hgTokArray); // hrTokenizeAndParse() would have unlocked it.
    }

    return hr;
}


//	Parse the document for a charset specification.
//	They can take the following forms:
//		<META CHARSET=XXX>
//		<META HTTP_EQUIV CHARSET=XXX>
//		<META HTTP_EQUIV="Content-type" CONTENT="text/html; charset=XXX">
//		<META HTTP_EQUIV="Charset" CONTENT="text/html; charset=XXX">
//
//	Return S_OK if found, S_FALSE if not found.  Error on exceptional cases.

HRESULT CTriEditDocument::GetCharset(HGLOBAL hgUHTML, int cbSizeIn, BSTR* pbstrCharset)
{
	HRESULT hr = E_FAIL;
	HGLOBAL hgTokArray = NULL; // holds the token array
	UINT cMaxToken; // size of token array
	UINT cbSizeOut = 0; // init
	HGLOBAL hNew = NULL; // not really used. need to pass as params to hrTokenizeAndParse()
	int iArray = 0;
	TOKSTRUCT *pTokArray;
	BOOL fFoundContent = FALSE;
	BOOL fFoundCharset = FALSE;
	HRESULT	hrCharset = S_FALSE;	// This is the error code to return if no other error occurs

	_ASSERTE ( bstrIn );
	_ASSERTE ( pbstrCharset );
	_ASSERTE ( hgUHTML );

	if ( ( cbSizeIn <= 0 ) || ( NULL == hgUHTML ) )
		goto LRet;

	*pbstrCharset = NULL;

	// step 1. generate a token array
	// Create tokenizer if it hasn't yet been created
	if (m_pTokenizer == NULL)
	{
		CoCreateInstance(CLSID_TriEditParse, NULL, CLSCTX_INPROC_SERVER, IID_ITokenGen, (void **)&m_pTokenizer);
		if (m_pTokenizer == NULL)
		{
			hr = E_FAIL;
			goto LRet;
		}
	}

	hr = m_pTokenizer->hrTokenizeAndParse(	hgUHTML, &hNew, NULL, dwFilterNone, 
												modeInput, cbSizeIn, &cbSizeOut, m_pUnkTrident, 
												&hgTokArray, &cMaxToken, NULL, NULL, 
												PARSE_SPECIAL_HEAD_ONLY );

	if (hr != S_OK || hgTokArray == NULL)
		goto LRet;
	pTokArray = (TOKSTRUCT *) GlobalLock(hgTokArray);
	if (pTokArray == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto LRet;
	}

	// step 2. look for TokAttrib_CHARSET in the META tag it.
	iArray = 0;
	while (iArray < (int)cMaxToken) // we won't go this far and will get the META tag before we hit </head>
	{
		// If a META tag is found with a CONTENT attribute, explore it.
		if ( ( pTokArray[iArray].token.tok == TokAttrib_CONTENT )
			&& pTokArray[iArray].token.tokClass == tokAttr)
		{
			fFoundContent = TRUE;			
		}
		// If a META tag is fount with a CHARSET attribute, explor it as well.
		if ( ( pTokArray[iArray].token.tok == TokAttrib_CHARSET )
			&& pTokArray[iArray].token.tokClass == tokAttr)
		{
			fFoundCharset = TRUE;			
		}

		if ( ( fFoundContent || fFoundCharset )
			&& (   pTokArray[iArray].token.tokClass == tokValue
				|| pTokArray[iArray].token.tokClass == tokString
				)
			)
		{
			// get its value, put it in pbstrCharset and return
			int cwContent = pTokArray[iArray].token.ibTokMac-pTokArray[iArray].token.ibTokMin;
			WCHAR *pwContent = new WCHAR[cwContent+1];
			WCHAR* pwCharset = NULL; // This represents a movable pointer, not an allocation.

			if (pwContent != NULL)
			{
				pwContent[0]   = WCHAR('\0');
				WCHAR* pwcText = (WCHAR*)GlobalLock ( hgUHTML );
				if ( NULL == pwcText )
				{
					hr = E_OUTOFMEMORY;
					goto LRet;
				}
				memcpy(	(BYTE *)pwContent, 
						(BYTE *)&pwcText[pTokArray[iArray].token.ibTokMin], 
						cwContent*sizeof(WCHAR));
				pwContent[cwContent] = WCHAR('\0');

				GlobalUnlock ( hgUHTML );
				_wcslwr ( pwContent );

				if ( fFoundCharset )
				{
					pwCharset = pwContent;
				}

				// If it's a CONTENT attribute, this string actually contains something like
				// "text/html; charset=something".  We need to return only the "something" part.
				if ( fFoundContent )
				{
					// Find the "charset", case insensitive.
					pwCharset = wcsstr ( pwContent, L"charset" );

					// Find the equal sign following the charset
					if ( NULL != pwCharset )
					{
						pwCharset = wcsstr ( pwContent, L"=" );
					}

					// Find the charset name itself. There could be spaces between the = and the name.
					if ( NULL != pwCharset )
					{
						WCHAR wc = '\0';

						// Skip the equal sign we just found:
						pwCharset++;

						// Pick up a character.  It should never be \0, but could be for ill formed HTML.
						while ( WCHAR('\0') != ( wc = *pwCharset ) )
						{
							if ( iswspace(wc) || WCHAR('\'') == wc )
							{
								pwCharset++;
							}
							else
							{
								break;
							}
						}
					}

					// Now terminate the charset name.  It could have trailing spaces, a closing quote, a semi-colon, etc.
					if ( NULL != pwCharset )
					{
						pwCharset = wcstok ( pwCharset, L" \t\r\n\"\';" );	// First token not containing whitespace, quote or semicolon
					}
				}

				// If it was not found, try again.
				if ( NULL == pwCharset )
				{
					delete [] pwContent;
					fFoundContent = FALSE;
					fFoundCharset = FALSE;
					continue;
				}

				*pbstrCharset = SysAllocString(pwCharset);
				if (*pbstrCharset != NULL)
				{
					hrCharset = S_OK;
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
				delete [] pwContent;
			}
			break; // even if we didn't succeed in above allocations, we should quit because we already found the charset
		}
		iArray++;
	}

LRet:
	if (hgTokArray != NULL)
	{
		GlobalUnlock(hgTokArray);
		GlobalFree(hgTokArray);
	}
	if (hNew != NULL)
		GlobalFree(hNew); // hrTokenizeAndParse() would have unlocked it.

	// If no error occurred, return S_OK or S_FALSE indicating if the charset was found:
	if ( SUCCEEDED ( hr ) )
	{
		hr = hrCharset;
	}
	return(hr);

} /* CDocument::GetCharset() */


//	Given a stream, created on a global, find any META charset tag that might exist in it.
//	The input stream may be in Unicode or MBCS: Unicode streams MUST be byte-order prefixed.
//	The stream pos will not be affected by this operation, nor will its contents be changed.
//	If the stream is in Unicode, in either byte order, the charset "unicode" is returned,
//	because this routine is primarily of interest in converting streams to unicode.
//
//	If the input stream is empty, or if no META charset tag exists, return S_FALSE and NULL
//	for pbstrCharset.
//	If a charset tag is found, return S_OK and allocate a SysString for pbstrCharset.
//	The caller must call SysFreeString if pbstrCharset is returned.
//
HRESULT CTriEditDocument::GetCharsetFromStream(IStream* pStream, BSTR* pbstrCharset)
{
	HRESULT	hr			= S_OK;
	STATSTG	statStg		= {0};
	HGLOBAL hMem		= NULL;
	CHAR*	pbData		= NULL;
	WCHAR*	pwcUnicode	= NULL;
	HGLOBAL	hgUHTML		= NULL;
	UINT	cbNewSize	= 0;

	_ASSERTE ( pbstrCharset );
	*pbstrCharset = NULL;

	if (FAILED(hr = pStream->Stat(&statStg, STATFLAG_NONAME)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}

	if ( 0 == statStg.cbSize.LowPart )
	{
		return S_FALSE;
	}

	if (FAILED(hr = GetHGlobalFromStream(pStream, &hMem)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}

	pbData = (CHAR*)GlobalLock(hMem);
	if (NULL == pbData)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		_ASSERTE(pbData);
		return hr;		
	}

	// If the stream is already in Unicode, that's all we need to know.
	if ( 0xfffe == *((WCHAR*)pbData) )
	{
		*pbstrCharset = SysAllocString ( L"Unicode" );
		hr = S_OK;
		goto LRet;
	}

	// Convert the SBCS or MBCS stream to Unicode as ANSI.
	// This will be adequate for finding the charset META tag.

	cbNewSize = ::MultiByteToWideChar ( CP_ACP, 0, pbData, statStg.cbSize.LowPart, NULL, 0 );
	if ( 0 == cbNewSize )
	{
		hr = E_FAIL;
		goto LRet;
	}

	// Create the buffer to convert to.
	hgUHTML = GlobalAlloc ( GMEM_MOVEABLE|GMEM_ZEROINIT, (cbNewSize + 1) * sizeof(WCHAR) );
	_ASSERTE ( hgUHTML );
	if ( NULL == hgUHTML )
	{
		hr = E_OUTOFMEMORY;
		goto LRet;
	}

	pwcUnicode = (WCHAR*)GlobalLock ( hgUHTML );
	_ASSERTE ( pwcUnicode );
	if ( NULL == hgUHTML )
	{
		hr = E_OUTOFMEMORY;
		goto LRet;
	}

	// Create the wide string.
	cbNewSize = ::MultiByteToWideChar ( CP_ACP, 0, pbData, statStg.cbSize.LowPart, pwcUnicode, cbNewSize);
	if ( 0 == cbNewSize )
	{
		hr = E_FAIL;
		goto LRet;
	}

	hr = GetCharset ( hgUHTML, cbNewSize * sizeof(WCHAR), pbstrCharset );

LRet:
	if ( NULL != hgUHTML )	{
		GlobalUnlock ( hgUHTML );
		GlobalFree ( hgUHTML );
	}

	GlobalUnlock ( hMem );
	return hr;
}
