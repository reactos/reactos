//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996                    **
//*********************************************************************

#ifndef _RATINGS_H_
#define _RATINGS_H_

#include <winerror.h>

STDAPI RatingEnable(BOOL fEnable);
STDAPI RatingCheckUserAccess(LPCSTR pszUsername, LPCSTR pszURL,
                             LPCSTR pszRatingInfo, LPBYTE pData,
                             DWORD cbData, void **ppRatingDetails);
STDAPI RatingAccessDeniedDialog(HWND hDlg, LPCSTR pszUsername, LPCSTR pszContentDescription, void *pRatingDetails);
STDAPI RatingFreeDetails(void *pRatingDetails);
STDAPI RatingObtainCancel(HANDLE hRatingObtainQuery);
STDAPI RatingObtainQuery(LPCTSTR pszTargetUrl, DWORD dwUserData, void (*fCallback)(DWORD dwUserData, HRESULT hr, LPCTSTR pszRating, void *lpvRatingDetails), HANDLE *phRatingObtainQuery);
STDAPI RatingSetupUI(HWND hDlg, LPCSTR pszUsername);
#ifdef _INC_COMMCTRL
STDAPI RatingAddPropertyPage(PROPSHEETHEADER *ppsh);
#endif

STDAPI RatingEnabledQuery();
STDAPI RatingInit();
STDAPI_(void) RatingTerm();


#define S_RATING_ALLOW		S_OK
#define S_RATING_DENY		S_FALSE
#define S_RATING_FOUND		0x00000002
#define E_RATING_NOT_FOUND	0x80000001

/************************************************************************

IObtainRating interface

This interface is used to obtain the rating (PICS label) for a URL.
It is entirely up to the server to determine how to come up with the
label.  The ObtainRating call may be synchronous.

GetSortOrder returns a ULONG which is used to sort this rating helper
into the list of installed helpers.  The helpers are sorted in ascending
order, so a lower numbered helper will be called before a higher numbered
one.

************************************************************************/

DECLARE_INTERFACE_(IObtainRating, IUnknown)
{
	// *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

	STDMETHOD(ObtainRating) (THIS_ LPCTSTR pszTargetUrl, HANDLE hAbortEvent,
							 IMalloc *pAllocator, LPSTR *ppRatingOut) PURE;

	STDMETHOD_(ULONG,GetSortOrder) (THIS) PURE;
};

#define RATING_ORDER_REMOTESITE		0x80000000
#define RATING_ORDER_LOCALLIST		0xC0000000


#endif
// _RATINGS_H_

