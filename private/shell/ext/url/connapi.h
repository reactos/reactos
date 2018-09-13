//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//	CONNAPI.H - internal connection state APIs for O'Hare
//			
//

//	HISTORY:
//	
//	4/5/95		jeremys		Created.
//

#ifndef _CONNAPI_H_
#define _CONNAPI_H_

/*******************************************************************

	NAME:		InetDialHandler

	SYNOPSIS:	Prototype for function exported by autodial handler
				(function name can be anything)

	ENTRY:		hwndParent - parent window (can be NULL)
				pszEntryName - name of RNA entry (connectoid) to use
				dwFlags - reserved for future use.  This parameter must be
					zero.
				pdwRetCode - returns a RAS error code.  Only valid if return
					value from function is TRUE.
			
	EXIT:		returns TRUE if dial handler will process this call.  If TRUE,
				then a RAS error code (or ERROR_SUCCESS) is filled in in pdwRetCode.
				Returns FALSE if the dial handler does not want to process this
				call (e.g. it is turned off in other UI), in which case the
				default dialer will be used.

********************************************************************/
BOOL WINAPI InetDialHandler(HWND hwndParent,LPCSTR pszEntryName,DWORD dwFlags,LPDWORD pdwRetCode);
typedef BOOL (WINAPI * INETDIALHANDLER) (HWND, LPCSTR, DWORD, LPDWORD);


/*******************************************************************

	NAME:		InetEnsureConnected

	SYNOPSIS:	Dials to the Internet if not already connected

	ENTRY:		hwndParent - parent window
				dwFlags - reserved for future use.  This parameter must be
					zero.

	EXIT:		returns TRUE if a connection is made, or a connection already
				exists.  Returns FALSE if a connection was not made or the
				user cancelled.

	NOTES:		If a LAN is present and user has set preference to use LAN,
				then no dialing is done.

				Note that this API does not guarantee or verify that the Internet
				is on the other end of the connection -- it merely assures that
				an RNA or LAN connection is present.

				This function may be exposed to ISVs.

********************************************************************/

BOOL WINAPI InetEnsureConnected(HWND hwndParent,DWORD dwFlags);

#endif	// _CONNAPI_H_
