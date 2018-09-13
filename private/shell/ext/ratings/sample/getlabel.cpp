//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996                    **
//*********************************************************************

#include "project.h"


CSampleObtainRating::CSampleObtainRating()
{
    m_cRef = 1; 
    DllAddRef();
}

CSampleObtainRating::~CSampleObtainRating()
{
    DllRelease();
}


STDMETHODIMP CSampleObtainRating::QueryInterface(REFIID riid, void **ppvObject)
{
	if (IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_IObtainRating)) {
		*ppvObject = (void *)this;
		AddRef();
		return NOERROR;
	}
	*ppvObject = NULL;
	return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CSampleObtainRating::AddRef(void)
{
	return ++m_cRef;
}


STDMETHODIMP_(ULONG) CSampleObtainRating::Release(void)
{
	if (!--m_cRef) {
		delete this;
		return 0;
	}
	else
		return m_cRef;
}


/* The sample rating obtainer reads the rating for the site from
 * a .INI file (ratings.ini) which looks like this:
 *
 * [Ratings]
 * http://www.msn.com=l 0 s 0 n 0 v 0
 * http://www.playboy.com=l 3 s 4 n 4 v 0
 *
 * For this sample implementation, the URL must match exactly with
 * an entry in the file.
 */
const TCHAR szRatingTemplate[] =
    "(PICS-1.0 \"http://www.rsac.org/ratingsv01.html\" l by \"Sample Rating Obtainer\" for \"%s\" on \"1996.04.16T08:15-0500\" exp \"1997.03.04T08:15-0500\" r (%s))";


STDMETHODIMP CSampleObtainRating::ObtainRating(THIS_ LPCTSTR pszTargetUrl, HANDLE hAbortEvent,
							 IMalloc *pAllocator, LPSTR *ppRatingOut)
{
	TCHAR szRating[18];	/* big enough for "l 0 s 0 n 0 v 0" */
	UINT cchCopied;
	
	cchCopied = GetPrivateProfileString("Allow", pszTargetUrl, "", szRating, sizeof(szRating), "ratings.ini");
	if (cchCopied > 0) {
		return S_RATING_ALLOW;		/* explicitly allow access */
	}

	cchCopied = GetPrivateProfileString("Deny", pszTargetUrl, "", szRating, sizeof(szRating), "ratings.ini");
	if (cchCopied > 0) {
		return S_RATING_DENY;		/* explicitly deny access */
	}

	cchCopied = GetPrivateProfileString("Ratings", pszTargetUrl, "", szRating, sizeof(szRating), "ratings.ini");
	if (cchCopied == 0) {
		return E_RATING_NOT_FOUND;		/* rating not found */
	}

	LPSTR pBuffer = (LPSTR)pAllocator->Alloc(sizeof(szRatingTemplate) + lstrlen(pszTargetUrl) + lstrlen(szRating));
	if (pBuffer == NULL)
		return E_OUTOFMEMORY;

	::wsprintf(pBuffer, szRatingTemplate, pszTargetUrl, szRating);

	*ppRatingOut = pBuffer;

	return S_RATING_FOUND;
}


/* We want the sample provider to override any HTTP rating bureau
 * which might be installed, so we return a sort order value which
 * is less than the one used by that provider (0x80000000).
 */
STDMETHODIMP_(ULONG) CSampleObtainRating::GetSortOrder(THIS)
{
	return 0x40000000;	/* before rating bureau */
}
