// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#include "stdafx.h"
#include "DHTMLEd.h"
#include "DHTMLEdit.h"
#include "site.h"
#include "proxyframe.h"

#define AGENT_SIGNATURE (TEXT("Mozilla/4.0 (compatible; MSIE 5.01; DHTML Editing Control)"))


//	Check to see if a buffer starts with a byte order Unicode character.
//	This implementation is processor byte-order independant.
//
static BOOL StartsWithByteOrderMark ( LPVOID pvData )
{
	CHAR	*pchData	= (CHAR*)pvData;

#pragma warning(disable: 4310) // cast truncates constant value
	if ( ( (char)0xff == pchData[0] ) && ( (char)0xfe == pchData[1] ) )
#pragma warning(default: 4310) // cast truncates constant value
	{
		return TRUE;
	}
	return FALSE;
}


//	Given a pointer to a buffer assumed to hold at least two bytes,
//	write a Unicode byte order mark to it.
//	This implementation is processor byte-order independant.
//
static void InsertByteOrderMark ( LPVOID pvData )
{
	CHAR	*pchData	= (CHAR*)pvData;

#pragma warning(disable: 4310) // cast truncates constant value
	pchData[0] = (CHAR)0xff;
	pchData[1] = (CHAR)0xfe;
#pragma warning(default: 4310) // cast truncates constant value
}


HRESULT
CSite::HrFileToStream(LPCTSTR fileName, LPSTREAM* ppiStream)
{
	HRESULT hr = S_OK;
	HANDLE hFile = NULL;
	HGLOBAL hMem = NULL;
	DWORD cbData = 0;
	LPVOID pbData = NULL;
	BOOL memLocked = FALSE;
	DWORD bytesRead = 0;
	BOOL bResult = FALSE;
	BOOL  bfUnicode = FALSE;

	hFile = CreateFile(
				fileName,
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

	if(INVALID_HANDLE_VALUE == hFile)
	{
		DWORD ec = ::GetLastError();
		if ( ERROR_BAD_NETPATH == ec ) ec = ERROR_PATH_NOT_FOUND;
		hr = HRESULT_FROM_WIN32(ec);
		return hr;
	}

	cbData = GetFileSize(hFile, NULL);

	if (0xFFFFFFFF == cbData)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		goto cleanup;
	}


	// If the file is empty, create a zero length stream, but the global block must be non-zero in size.
	// VID98BUG 23121
	hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, ( 0 == cbData ) ? 2 : cbData );
#if _DEBUG
	size = GlobalSize(hMem);
#endif

	if (NULL == hMem)
	{
		_ASSERTE(hMem);
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	pbData = GlobalLock(hMem);

	_ASSERTE(pbData);

	if (NULL == pbData)
	{
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	bResult = ReadFile(hFile, pbData, cbData, &bytesRead, NULL) ; 

	_ASSERTE(bResult);

	if (FALSE == bResult)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		goto cleanup;
	}

	_ASSERTE(bytesRead == cbData);

	BfFlipBytesIfBigEndianUnicode ( (CHAR*)pbData, bytesRead );

	if ( IsUnicode ( pbData, (int)cbData ) )
	{
		bfUnicode = TRUE;
	}
	else
	{
		bfUnicode = FALSE;
	}


cleanup:


	::CloseHandle((HANDLE) hFile);

	if (hr != E_OUTOFMEMORY)
		memLocked = GlobalUnlock(hMem);

	_ASSERTE(FALSE == memLocked);

	if (SUCCEEDED(hr))
	{
		if (SUCCEEDED(hr = CreateStreamOnHGlobal(hMem, TRUE, ppiStream)))
		{
			ULARGE_INTEGER ui = {0};

			_ASSERTE(ppiStream);

			ui.LowPart = cbData;
			ui.HighPart = 0x00;
			hr = (*ppiStream)->SetSize(ui);

			_ASSERTE(SUCCEEDED(hr));
		}

		if ( SUCCEEDED ( hr ) )
		{
			SetSaveAsUnicode ( bfUnicode );
			if ( !bfUnicode )
			{
				hr = HrConvertStreamToUnicode ( *ppiStream );
				_ASSERTE(SUCCEEDED(hr));
			}
		}
	}
	else // if failed
	{
		hMem = GlobalFree(hMem);
		_ASSERTE(NULL == hMem);
	}

	return hr;
}


//	Determine which method of URL fetch to use.  We have one for https, and another for other protocols.
//
HRESULT
CSite::HrURLToStream(LPCTSTR szURL, LPSTREAM* ppiStream)
{
	HRESULT			hr = S_OK;
	URL_COMPONENTS	urlc;

	_ASSERTE ( szURL );
	_ASSERTE ( ppiStream );

	memset ( &urlc, 0, sizeof ( urlc ) );
	urlc.dwStructSize = sizeof ( urlc );

	hr = InternetCrackUrl ( szURL, 0, 0, &urlc );
	if ( SUCCEEDED ( hr ) )
	{
		if ( INTERNET_SCHEME_HTTPS == urlc.nScheme )
		{
			hr = HrSecureURLToStream ( szURL, ppiStream );
		}
		else
		{
			hr = HrNonSecureURLToStream ( szURL, ppiStream );
		}
	}
	return hr;
}



//	This version utilizes WinINet, which does not create cache files so is usable with https.
//	However, pluggable protocols cannot be stacked on the WinINet fucntions.
//
#define BUFFLEN 4096
HRESULT
CSite::HrSecureURLToStream(LPCTSTR szURL, LPSTREAM* ppiStream)
{
	HRESULT		hr		= S_OK;

	_ASSERTE ( szURL );
	_ASSERTE ( ppiStream );
	*ppiStream = NULL;

	// Create a new read/write stream:
	hr = CreateStreamOnHGlobal ( NULL, TRUE, ppiStream );

	if ( SUCCEEDED ( hr ) && *ppiStream )
	{
		CHAR			*pBuff			= NULL;
		DWORD			dwRead			= 0;
		ULONG			ulStreamLen		= 0;
		ULONG			ulStreamWrite	= 0;
		ULARGE_INTEGER	ui				= {0};
		BOOL			bfUnicode		= FALSE;

		pBuff = new CHAR[BUFFLEN];
		if ( NULL == pBuff )
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			HINTERNET hSession = InternetOpen ( AGENT_SIGNATURE, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
			if ( NULL == hSession )
			{
				// InternetOpen failed
				hr = HRESULT_FROM_WIN32 ( GetLastError () );
				_ASSERTE ( FAILED ( hr ) );	// Make sure error returned wasn't NO_ERR
			}
			else
			{
				// Do not allow redirects in the SFS control.
				DWORD dwFlags = m_pFR->GetControl()->IsSafeForScripting () ? INTERNET_FLAG_NO_AUTO_REDIRECT : 0;
				
				HINTERNET hFile = InternetOpenUrl ( hSession, szURL, NULL, 0, dwFlags, 0 );
				if ( NULL == hFile )
				{
					// InternetOpenURL failed
					hr = HRESULT_FROM_WIN32 ( GetLastError () );
					_ASSERTE ( FAILED ( hr ) );	// Make sure error returned wasn't NO_ERR
				}
				else
				{
					// Read in data and write it to the stream to return.
					while ( InternetReadFile ( hFile, pBuff, BUFFLEN, &dwRead ) )
					{
						if ( 0 == dwRead )
						{
							break;
						}
						hr = (*ppiStream)->Write ( pBuff, dwRead, &ulStreamWrite );
						_ASSERTE ( dwRead == ulStreamWrite );

						if ( SUCCEEDED ( hr ) )
						{
							ulStreamLen += ulStreamWrite;
						}
						else
						{
							// Failed to read the data.  Make sure the error is not overwritten.
							goto READFILE_BAILOUT;
						}
					}


					ui.LowPart	= ulStreamLen;
					ui.HighPart	= 0x00;
					hr = (*ppiStream)->SetSize(ui);

					hr = HrConvertStreamToUnicode ( *ppiStream );
					bfUnicode = ( S_FALSE == hr );
					if ( SUCCEEDED ( hr ) )
					{
						hr = S_OK;	// the S_FALSE result wouldn't make much sense to caller of this function.
					}
					SetSaveAsUnicode ( bfUnicode );

READFILE_BAILOUT:
					InternetCloseHandle ( hFile );
				}

				InternetCloseHandle ( hSession );
			}
			delete [] pBuff;
		}
	}

	// If an error is being returned, cleat the stream here.
	if ( FAILED ( hr ) && ( NULL != *ppiStream ) )
	{
		(*ppiStream)->Release();
		*ppiStream = NULL;
	}

	return hr;
}


//  This version utilizes URLMon, which makes stacking pluggable protocols possible.
// However, it cannot be used with https because it creates a cache file.
//
HRESULT
CSite::HrNonSecureURLToStream(LPCTSTR szURL, LPSTREAM* ppiStream)
{
	HRESULT		hr				= S_OK;
	IStream*	piStreamOrig	= NULL;

	*ppiStream = NULL;

	// Use the degenerate IBindStatusCallback implemented on the proxyframe exclusively
	// to provide IAuthenticate.
	IBindStatusCallback* piBSCB = NULL;
	m_pFR->QueryInterface ( IID_IBindStatusCallback, (void**)&piBSCB );

	m_pFR->ClearSFSRedirect ();

#ifdef LATE_BIND_URLMON_WININET
	PFNURLOpenBlockingStream pfnURLOpenBlockingStream = m_pFR->m_pfnURLOpenBlockingStream;
	_ASSERTE ( pfnURLOpenBlockingStream );
	hr = (*pfnURLOpenBlockingStream)( NULL, szURL, &piStreamOrig, 0, piBSCB );
#else
	hr = URLOpenBlockingStream ( NULL, szURL, &piStreamOrig, 0, piBSCB );
#endif // LATE_BIND_URLMON_WININET

	if ( NULL != piBSCB )
	{
		piBSCB->Release ();
		piBSCB = NULL;
	}

	// If SFSRedirect got set, this is the SFS control and a redirect was detected.  Abort for security!
	if ( m_pFR->GetSFSRedirect () )
	{
		if ( NULL != piStreamOrig )
		{
			piStreamOrig->Release ();
			piStreamOrig = NULL;
		}
		hr = DE_E_ACCESS_DENIED;
	}

	if ( SUCCEEDED ( hr ) )
	{
		ULONG	cbStreamSize	= 0;
		HGLOBAL	hGlob			= NULL;
		STATSTG stat;

		// TriEdit will call GetHGlobalFromStream on the stream, which will fail.
		// We need to recopy it into this process.
		if ((hr = piStreamOrig->Stat(&stat, STATFLAG_NONAME)) == S_OK)
		{
			cbStreamSize = stat.cbSize.LowPart;
			// If the file is empty, create a zero length stream, but the global block must be non-zero in size.
			hGlob = GlobalAlloc ( GHND, ( 0 == cbStreamSize ) ? 2 : cbStreamSize );
			if ( NULL == hGlob )
			{
				DWORD ec = ::GetLastError();
				hr = HRESULT_FROM_WIN32(ec);
			}
			else
			{
				void* pBuff = GlobalLock ( hGlob );
				if ( NULL == pBuff )
				{
					DWORD ec = ::GetLastError();
					hr = HRESULT_FROM_WIN32(ec);
				}
				else
				{
					ULONG	cbBytesRead = 0;

					hr = piStreamOrig->Read ( pBuff, cbStreamSize, &cbBytesRead );
					_ASSERTE ( SUCCEEDED ( hr ) );
					_ASSERTE ( cbBytesRead == cbStreamSize );

					if ( SUCCEEDED ( hr ) )
					{
						// We now have a global to creat a NEW stream from, locally.
						hr = CreateStreamOnHGlobal ( hGlob, TRUE, ppiStream );

						if ( SUCCEEDED ( hr ) )
						{
							// Convert it to Unicode if necessary.  Set SaveAsUnicode so it can be saved properly.
							hr = HrConvertStreamToUnicode ( *ppiStream );
							BOOL bfUnicode = ( S_FALSE == hr );
							if ( SUCCEEDED ( hr ) )
							{
								hr = S_OK;	// the S_FALSE result wouldn't make much sense to caller of this function.
							}
							SetSaveAsUnicode ( bfUnicode );
						}
					}
				}

				GlobalUnlock ( hGlob );
			}

			if ( FAILED ( hr ) )
			{
				GlobalFree ( hGlob );
			}
			piStreamOrig->Release();
		}
	}
	return hr;
}


//	Post V1.0 change:
//	The stream will now always be Unicode.
//	We should save the file as Unicode only if it was loaded as Unicode from File or URL,
//	otherwise convert to MBCS string.
//
HRESULT
CSite::HrStreamToFile(LPSTREAM pStream, LPCTSTR fileName)
{
	HRESULT	hr				= S_OK;
	HANDLE	hFile			= NULL;
	HGLOBAL hMem			= NULL;
	WCHAR	*pwcData		= NULL;
	DWORD	bytesWritten	= 0;
	BOOL	bResult			= FALSE;
	STATSTG	statStg			= {0};
            
	hFile = CreateFile(fileName,
				GENERIC_WRITE,
				FILE_SHARE_WRITE, 
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

    if (INVALID_HANDLE_VALUE == hFile)
	{
		DWORD ec = ::GetLastError();

		if ( ERROR_BAD_NETPATH == ec ) ec = ERROR_PATH_NOT_FOUND;
		hr = HRESULT_FROM_WIN32(ec);
		return hr;
    }

	if (FAILED(hr = pStream->Stat(&statStg, STATFLAG_NONAME)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}

	if (FAILED(hr = GetHGlobalFromStream(pStream, &hMem)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}

	pwcData = (WCHAR*)GlobalLock(hMem);
	if (NULL == pwcData)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		_ASSERTE(pwcData);
		return hr;		
	}

	_ASSERTE ( IsUnicode ( pwcData, statStg.cbSize.LowPart ) );

	// Should it be converted to MBCS?
	if ( GetSaveAsUnicode () )
	{
		bResult = WriteFile(hFile, pwcData, statStg.cbSize.LowPart, &bytesWritten, NULL);
		_ASSERTE(bytesWritten == statStg.cbSize.LowPart);
	}
	else
	{
		UINT cbOrigSize	= statStg.cbSize.LowPart / sizeof ( WCHAR );
		UINT cbNewSize	= 0;
		char *pchTemp	= NULL;

		// Substract one for the byte order mark if it begins the stream.  (It should.)
		if ( StartsWithByteOrderMark ( pwcData ) )
		{
			pwcData++;	// Skip the byte order mark WCHAR
			cbOrigSize--;
		}

		if ( NULL != m_piMLang )
		{
			DWORD dwMode	= 0;

			hr = m_piMLang->ConvertStringFromUnicode ( &dwMode, GetCurrentCodePage (), pwcData, &cbOrigSize, NULL, &cbNewSize );
			if ( S_FALSE == hr )
			{
				// This indicates that a conversion was not available.  Happens for default CP_ACP if test is typed into new page!
				hr = S_OK;
				goto fallback;
			}

			_ASSERTE ( 0 != cbNewSize );
			if ( SUCCEEDED ( hr ) )
			{
				pchTemp = new char [cbNewSize];
				_ASSERTE ( pchTemp );
				if ( NULL != pchTemp )
				{
					hr = m_piMLang->ConvertStringFromUnicode ( &dwMode, GetCurrentCodePage (), pwcData, &cbOrigSize, pchTemp, &cbNewSize );
					bResult = WriteFile(hFile, pchTemp, cbNewSize, &bytesWritten, NULL);
					_ASSERTE(bytesWritten == cbNewSize);
					delete [] pchTemp;
				}
			}
		}
		else
		{
fallback:
			cbNewSize = ::WideCharToMultiByte ( GetCurrentCodePage (), 0, pwcData, cbOrigSize, NULL, 0, NULL, NULL );
			_ASSERTE ( 0 != cbNewSize );

			pchTemp = new char [cbNewSize];
			_ASSERTE ( pchTemp );
			if ( NULL != pchTemp )
			{
				::WideCharToMultiByte ( GetCurrentCodePage (), 0, pwcData, cbOrigSize, pchTemp, cbNewSize, NULL, NULL );
				bResult = WriteFile(hFile, pchTemp, cbNewSize, &bytesWritten, NULL);
				_ASSERTE(bytesWritten == cbNewSize);
				delete [] pchTemp;
			}
		}
	}


	if (FALSE == bResult)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		goto cleanup;
	}



cleanup:

	::CloseHandle(hFile);
	// Reference count of hMem not checked here
	// since we can't assume how many times the
	// Stream has locked it
	GlobalUnlock(hMem); 
	return hr;
}


//	Post V1.0 change:
//	The stream is always Unicode now.
//
HRESULT
CSite::HrBstrToStream(BSTR bstrSrc, LPSTREAM* ppStream)
{
	HRESULT hr = S_OK;
	HGLOBAL hMem = NULL;
	ULONG	cbMBStr = 0;
	ULONG	cbBuff = 0;
	LPVOID	pStrDest = NULL;
	LPVOID	pCopyPos = NULL;
	ULARGE_INTEGER ui = {0};

	_ASSERTE(bstrSrc);
	_ASSERTE(ppStream);

	cbMBStr = SysStringLen ( bstrSrc ) * sizeof (OLECHAR);
	cbBuff  = cbMBStr;

	// If the Unicode string does not contain a byte order mark at the beginning, it is
	// misinterpreted by Trident.  When DocumentHTML was set with Japanese text, the
	// BSTR was fed in without the byte order mark and was misinterpreted. (Possibly as UTF-8?)
	// Now, the byte-order mark is prepended to all non-empty strings.

	if ( 2 <= cbMBStr )
	{
		if ( !StartsWithByteOrderMark ( bstrSrc ) )
		{
			cbBuff += 2;	// Reserve space for the byte order mark we'll add.
		}
	}

	// If the file is empty, create a zero length stream, but the global block must be non-zero in size.
	hMem = GlobalAlloc ( GMEM_MOVEABLE|GMEM_ZEROINIT, ( 0 == cbBuff ) ? 2 : cbBuff );

	_ASSERTE(hMem);

	if (NULL == hMem)
	{
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	pStrDest = GlobalLock(hMem);

	_ASSERTE(pStrDest);

	if (NULL == pStrDest)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		GlobalFree(hMem);
		goto cleanup;
	}

	// Insert the byte order mark if it is not already there
	pCopyPos = pStrDest;
	if ( cbMBStr != cbBuff )
	{
		InsertByteOrderMark ( pStrDest );
		pCopyPos = &((char*)pCopyPos)[2];	// Advance copy target two bytes.
	}
	memcpy ( pCopyPos, bstrSrc, cbMBStr );
	GlobalUnlock(hMem);

	if (FAILED(hr = CreateStreamOnHGlobal(hMem, TRUE, ppStream)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto cleanup;
	}

	_ASSERTE(ppStream);

	ui.LowPart = cbBuff;
	ui.HighPart = 0x00;

	hr = (*ppStream)->SetSize(ui);


	_ASSERTE((*ppStream));

cleanup:

	return hr;
}


//	Post V1.0 change:
//	The stream is expected to be in Unicode now.
//	Just copy the contents to a BSTR.
//	Exception: the stream can begin with FFFE (or theoretically FEFF, but then I think we'd be broken.)
//	If the byte order mark begins the stream, don't copy it to the BSTR UNLESS bfRetainByteOrderMark
//	is set.  This should be retained in the case where we're loading an interal BSTR to be returned
//	to the pluggable protocol.  If the byte order mark is missing in that case, IE5 does not properly
//	convert the string.
//
HRESULT
CSite::HrStreamToBstr(LPSTREAM pStream, BSTR* pBstr, BOOL bfRetainByteOrderMark)
{
	HRESULT hr			= S_OK;
	HGLOBAL hMem		= NULL;
	WCHAR	*pwcData	= NULL;
	STATSTG statStg		= {0};

	_ASSERTE(pStream);
	_ASSERTE(pBstr);

	*pBstr = NULL;

	if (FAILED(hr = GetHGlobalFromStream(pStream, &hMem)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}

	hr = pStream->Stat(&statStg, STATFLAG_NONAME);
	_ASSERTE(SUCCEEDED(hr));

	pwcData = (WCHAR*)GlobalLock(hMem);

	_ASSERTE(pwcData);
	
	if (NULL == pwcData)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		return hr;		
	}

	_ASSERTE ( IsUnicode ( pwcData, statStg.cbSize.LowPart ) );

	if ( !bfRetainByteOrderMark && StartsWithByteOrderMark ( pwcData ) )
	{
		pwcData++;	// Skip the first WCHAR
		statStg.cbSize.LowPart -= sizeof(WCHAR);	// This is a byte count rather than a WCHAR count.
	}

	*pBstr = SysAllocStringLen ( pwcData, statStg.cbSize.LowPart / sizeof(WCHAR) );
	
	GlobalUnlock(hMem); 
	return hr;
}


#ifdef _DEBUG_HELPER
static void ExamineStream ( IStream* piStream, char* pchNameOfStream )
{
	HGLOBAL hMem	= NULL;
	LPVOID	pvData	= NULL;
	HRESULT	hr		= S_OK;

	_ASSERTE ( pchNameOfStream );
	hr = GetHGlobalFromStream(piStream, &hMem);
	pvData = GlobalLock ( hMem );
	// Examine *(char*)pvData
	GlobalUnlock ( hMem );
}
#endif


HRESULT
CSite::HrFilter(BOOL bDirection, LPSTREAM pSrcStream, LPSTREAM* ppFilteredStream, DWORD dwFilterFlags)
{
	_ASSERTE(m_pObj);
	_ASSERTE(pSrcStream);
	_ASSERTE(ppFilteredStream);

	HRESULT hr		= S_OK;
	STATSTG statStg	= {0};

	// Test for the exceptional case of an empty stream.  Opening an empyt file can cause this.
	hr = pSrcStream->Stat(&statStg, STATFLAG_NONAME);
	_ASSERTE(SUCCEEDED(hr));

	if ( 0 == statStg.cbSize.HighPart && 0 == statStg.cbSize.LowPart )
	{
		*ppFilteredStream = pSrcStream;
		pSrcStream->AddRef ();
		return S_OK;
	}

	CComQIPtr<ITriEditDocument, &IID_ITriEditDocument> piTriEditDoc(m_pObj);
	CComQIPtr<IStream, &IID_IStream> piFilteredStream;
	DWORD dwTriEditFlags = 0;

#ifdef _DEBUG_HELPER
	ExamineStream ( pSrcStream, "pSrcStream" );
#endif

	if (dwFilterFlags == filterNone)
	{
		pSrcStream->AddRef();
		*ppFilteredStream = pSrcStream;
		return hr;
	}

	// dwTriEditFlags |= dwFilterMultiByteStream; // loading an ANSI Stream NOT ANY MORE.  The stream is ALWAYS Unicode now.

	if (dwFilterFlags & filterDTCs)
		dwTriEditFlags |= dwFilterDTCs;

	if (dwFilterFlags & filterASP)
		dwTriEditFlags |= dwFilterServerSideScripts;

	if (dwFilterFlags & preserveSourceCode)
		dwTriEditFlags |= dwPreserveSourceCode;

	if (dwFilterFlags & filterSourceCode)
		dwTriEditFlags |= filterSourceCode;

	if (!piTriEditDoc)
		return E_NOINTERFACE;

	CComBSTR bstrBaseURL;
	m_pFR->GetBaseURL ( bstrBaseURL );

	if (TRUE == bDirection)
	{
		if (FAILED(hr = piTriEditDoc->FilterIn(pSrcStream, (LPUNKNOWN*) &piFilteredStream, dwTriEditFlags, bstrBaseURL)))
		{
			goto cleanup;
		}
	}
	else
	{
		if (FAILED(hr = piTriEditDoc->FilterOut(pSrcStream, (LPUNKNOWN*) &piFilteredStream, dwTriEditFlags, bstrBaseURL)))
		{
			_ASSERTE(SUCCEEDED(hr));
			goto cleanup;
		}
	}

	*ppFilteredStream = piFilteredStream;

#ifdef _DEBUG_HELPER
	ExamineStream ( *ppFilteredStream, "*ppFilteredStream" );
#endif

	_ASSERTE((*ppFilteredStream));
	if (!(*ppFilteredStream))
	{
		hr = E_NOINTERFACE;
		goto cleanup;
	}

	(*ppFilteredStream)->AddRef();

cleanup:

	return hr;
}



// Attempts to open the file specified by the UNC path
// This method is a crude way of seeing if a given file
// is available and current permissions allow for opening
// Returns:
// S_OK is file is available and it can be opened for reading
// else
// HRESULT containing Win32 facility and error code from ::GetLastError()
HRESULT
CSite::HrTestFileOpen(BSTR path)
{
	USES_CONVERSION;
	HRESULT hr = S_OK;
	LPTSTR pFileName = NULL;
	HANDLE hFile = NULL;

	pFileName = OLE2T(path);

	_ASSERTE(pFileName);

	hFile = CreateFile(
				pFileName,
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

	if(INVALID_HANDLE_VALUE == hFile)
	{
		DWORD ec = ::GetLastError();

		if ( ERROR_BAD_NETPATH == ec ) ec = ERROR_PATH_NOT_FOUND;
		hr = HRESULT_FROM_WIN32(ec);
	}

	::CloseHandle(hFile);

	return hr;
}


//******************************************************************************************
//
//	Unicode Utilities
//
//	Post V1.0, we changed the internal data format from (unchecked, assumed) MBCS to Unicode.
//	The Stream and associated Trident is always Unicode.
//
//******************************************************************************************


//	This can be called without knowing if the stream is already Unicode or not.
//	Convert stream in place.  Assume the stream is created with CreateStreamOnHGlobal.
//	Convert without using ATL macros.  They give out at about 200KB.
//	If the stream was already Unicode, return S_FALSE.
//
HRESULT CSite::HrConvertStreamToUnicode ( IStream* piStream )
{
	HRESULT hr			= S_OK;
	HGLOBAL hMem		= NULL;
	LPVOID	pbData		= NULL;
	STATSTG statStg		= {0};
	UINT	cwcNewStr	= 0;
	WCHAR	*pwcUnicode	= NULL;

	_ASSERTE(piStream);

	// The stream MUST be created on a global
	if (FAILED(hr = GetHGlobalFromStream(piStream, &hMem)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}

	hr = piStream->Stat(&statStg, STATFLAG_NONAME);
	_ASSERTE(SUCCEEDED(hr));

	if ( 0 == statStg.cbSize.HighPart && 4 > statStg.cbSize.LowPart )
	{
		return S_FALSE;	// If it's not even four bytes long, leave as is.
	}

	pbData = GlobalLock(hMem);

	_ASSERTE(pbData);
	
	if (NULL == pbData)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		return hr;		
	}

	// If the stream was already Unicode, do nothing!
	if ( IsUnicode ( pbData, statStg.cbSize.LowPart ) )
	{
		hr = S_FALSE;
		goto exit;
	}
	
	// If IMultilanguage2 is available, try to determine its code page.
	if ( NULL != m_piMLang )
	{
		DetectEncodingInfo	rdei[8];
		int					nScores		= 8;
		DWORD				dwMode		= 0;
		UINT				uiInSize	= statStg.cbSize.LowPart;
		HRESULT				hrCharset	= E_FAIL;

		// Check to see if there's an embedded META charset tag.
		// Only if the appropriate TriEdit is installed will this work.
		// We need access to MLang to make sense out of the result, too.
		_ASSERTE ( m_pObj );
		CComQIPtr<ITriEditExtendedAccess, &IID_ITriEditExtendedAccess> pItex ( m_pObj );
		if ( pItex )
		{
			CComBSTR	bstrCodePage;

			hrCharset = pItex->GetCharsetFromStream ( piStream, &bstrCodePage );
			
			// If "Unicode" is returned, it's got to be bogus.
			// We would have mangled Unicode in the initial translation.
			// This turns out to be a not-so-rare special case.  Outlook produced such files.
			if ( S_OK == hrCharset )
			{
				MIMECSETINFO	mcsi;

				if ( 0 == _wcsicmp ( L"unicode", bstrCodePage ) )
				{
					hrCharset = S_FALSE;
				}
				else
				{
					hrCharset = m_piMLang->GetCharsetInfo ( bstrCodePage, &mcsi );
					if ( SUCCEEDED ( hrCharset ) )
					{
						m_cpCodePage = mcsi.uiInternetEncoding;
					}
				}
			}
			
		}

		// If we found the charset via GetCharsetFromStream, don't use MLang.
		if ( S_OK != hrCharset )
		{

			hr = m_piMLang->DetectCodepageInIStream ( MLDETECTCP_HTML, 0, piStream, rdei, &nScores );

			if ( FAILED ( hr ) )
			{
				goto fallback;	// Use default ANSI code page
			}

			m_cpCodePage = rdei[0].nCodePage;
		}

		hr = m_piMLang->ConvertStringToUnicode ( &dwMode, m_cpCodePage, (char*)pbData, &uiInSize, NULL, &cwcNewStr );
		_ASSERTE ( SUCCEEDED ( hr ) );
		if ( S_OK != hr )	// S_FALSE for conversion not supported (no such language pack), E_FAIL for internal error.
		{
			goto fallback;	// Use default ANSI code page
		}

		// Create the buffer to convert to.
		pwcUnicode = new WCHAR[cwcNewStr+1];	// One extra character for the byte order mark.
		_ASSERTE ( pwcUnicode );
		if ( NULL == pwcUnicode )
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		InsertByteOrderMark ( pwcUnicode );

		hr = m_piMLang->ConvertStringToUnicode ( &dwMode, m_cpCodePage, (char*)pbData, &uiInSize, &pwcUnicode[1], &cwcNewStr );
		_ASSERTE ( SUCCEEDED ( hr ) );
		if ( S_OK != hr )	// S_FALSE for conversion not supported (no such language pack), E_FAIL for internal error.
		{
			delete [] pwcUnicode;	// This will be reallocated.
			pwcUnicode = NULL;
			goto fallback;	// Use default ANSI code page
		}
	}
	else
	{
fallback:	// If we attempt to use MLang but fail, we must STILL convert to Unicode...

		// Set code page to default:
		m_cpCodePage = CP_ACP;

		// Count how many wide characters are required:
		cwcNewStr = ::MultiByteToWideChar(GetCurrentCodePage (), 0, (char*)pbData, statStg.cbSize.LowPart, NULL, 0);
		_ASSERTE ( 0 != cwcNewStr );
		if ( 0 == cwcNewStr )
		{
#ifdef _DEBUG
			DWORD dwError = GetLastError ();
			_ASSERTE ( 0 == dwError );
#endif
			goto exit;
		}

		// Create the buffer to convert to.
		pwcUnicode = new WCHAR[cwcNewStr+1];	// One extra character for the byte order mark.
		_ASSERTE ( pwcUnicode );
		if ( NULL == pwcUnicode )
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		InsertByteOrderMark ( pwcUnicode );

		// Create the wide string.  Write starting at position [1], preserving the byte order character.
		cwcNewStr = ::MultiByteToWideChar(GetCurrentCodePage (), 0, (char*)pbData, statStg.cbSize.LowPart, &pwcUnicode[1], cwcNewStr);
		if ( 0 == cwcNewStr )
		{
#ifdef _DEBUG
			DWORD dwError = GetLastError ();
			_ASSERTE ( 0 == dwError );
#endif
			goto exit1;
		}
	}

	// We've successfully read the data in, now replace the stream.  pwcUnicode contains the data.
	ULARGE_INTEGER ui;
	ui.LowPart = (cwcNewStr+1) * 2;	// + 1 for the byte order mark at the beginning.
	ui.HighPart = 0x00;
	hr = piStream->SetSize ( ui );
	_ASSERTE ( SUCCEEDED ( hr ) );
	if ( SUCCEEDED ( hr ) )
	{
		GlobalUnlock(hMem);
		pbData = GlobalLock(hMem);
		memcpy ( pbData, pwcUnicode, (cwcNewStr+1) * 2 );	// Copy string + byte order mark

		// Reposition the mark to the beginning of the stream
		LARGE_INTEGER	liIn	= {0};
		ULARGE_INTEGER	uliOut	= {0};
		piStream->Seek ( liIn, STREAM_SEEK_SET, &uliOut );
	}

exit1:
	delete [] pwcUnicode;

exit:
	GlobalUnlock(hMem);
	return hr;
}



//	Test the buffer to see if it contains a Unicode string.  It's assumed to if:
//	It starts with the byte order marker FFFE or
//	It contains NULL bytes before the last four bytes.
//	If it's less than or equal to four bytes, do not consider it Unicode.
//
BOOL CSite::IsUnicode ( void* pData, int cbSize )
{
	BOOL	bfUnicode	= FALSE;
	CHAR	*pchData	= (CHAR*)pData;

	if ( 4 < cbSize )
	{

	#pragma warning(disable: 4310) // cast truncates constant value
		if ( ( (char)0xff == pchData[0] ) && ( (char)0xfe == pchData[1] ) )
			bfUnicode = TRUE;
		if ( ( (char)0xfe == pchData[0] ) && ( (char)0xff == pchData[1] ) )
	#pragma warning(default: 4310) // cast truncates constant value
		{
			// Reverse order Unicode?  Will this be encountered?
			_ASSERTE ( ! ( (char)0xfe == pchData[0] ) && ( (char)0xff == pchData[1] ) );
			bfUnicode = FALSE;
		}

		if ( ! bfUnicode )
		{
			bfUnicode = FALSE;

			for ( int i = 0; i < cbSize - 4; i++ )
			{
				if ( 0 == pchData[i] )
				{
					bfUnicode = TRUE;
					break;
				}
			}
		}
	}
	return bfUnicode;
}


//	Given a buffer of characters, detect whether its a BigEndian Unicode stream by the first word (FEFF).
//	If not, return FALSE.
//	If so, flip all words to LittleEndian order (FFFE) and return true.
//	Note: this is a storage convention, not an encoding.  This may be encountered in disk files, not in downloads.
//	A Unicode stream should contain an even number of bytes!  If not, we'll assert, but continue.
//
BOOL CSite::BfFlipBytesIfBigEndianUnicode ( CHAR* pchData, int cbSize )
{
	_ASSERTE ( pchData );

	// See if it's Unicode stored in reverse order.
#pragma warning(disable: 4310) // cast truncates constant value
	if ( ( (CHAR)0xFE == pchData[0] ) && ( (CHAR)0xFF == pchData[1] ) )
#pragma warning(default: 4310) // cast truncates constant value
	{
		// A Unicode stream must contain an even number of characters.
		_ASSERTE ( 0 != ( cbSize & 1 ) );

		// This stream is populated with reversed Unicode.  Flip it in place.
		// Subtract 1 from initial byte count to avoid overrunning odd length buffer.
		CHAR chTemp = '\0';
		for ( int iPos = 0; iPos < cbSize - 1; iPos += 2 )
		{
			chTemp = pchData[iPos];
			pchData[iPos] = pchData[iPos+1];
			pchData[iPos+1] = chTemp;
		}
		return TRUE;
	}
	return FALSE;
}

