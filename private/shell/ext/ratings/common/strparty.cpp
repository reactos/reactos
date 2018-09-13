/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strparty.cxx
	NLS/DBCS-aware string class: party support

	This file contains the implementation of the Party() and
	DonePartying() methods of NLS_STR, used for string operations
	outside the set supported by NLS_STR itself.

	FILE HISTORY:
		gregj	03/25/93	Created
*/

#include "npcommon.h"

extern "C"
{
	#include <netlib.h>
}

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <npassert.h>

#include <npstring.h>


/*******************************************************************

	NAME:		NLS_STR::Party

	SYNOPSIS:	Obtains read-write access to the string buffer, and
				disables standard member function access to the string.

	ENTRY:		No parameters

	EXIT:		Returns a pointer to the string, NULL if in an error state

	NOTES:		Use Party() with care.  Check your partying code to
				make sure it's DBCS-safe, doesn't overflow the string
				buffer, etc.

				Each Party() must be matched with a DonePartying() call.
				They cannot be nested.  It's probably not a good idea
				to leave a string in the Party()ing state for long.

	HISTORY:
		gregj	03/25/93	Created

********************************************************************/

CHAR *NLS_STR::Party()
{
	if (QueryError())
		return NULL;

	ReportError( WN_ACCESS_DENIED );	// keep other folks out
	return _pchData;					// OK, go party
}


/*******************************************************************

	NAME:		NLS_STR::DonePartying

	SYNOPSIS:	Releases read-write access to the string buffer, and
				re-enables standard member access.

	ENTRY:		cchNew - new string length (may be omitted, in which
						 case it's determined by strlenf())

	EXIT:		No return value

	NOTES:

	HISTORY:
		gregj	03/25/93	Created
        lens    03/16/94    Don't let Party/DonePartying pairs lose hard allocation errors.

********************************************************************/

VOID NLS_STR::DonePartying( INT cchNew )
{
	_cchLen = cchNew;			// store new length
	if (QueryError() == WN_ACCESS_DENIED ) {
    	ReportError( WN_SUCCESS );	// standard members can access now
    }
	IncVers();					// all ISTRs are invalid now
}


VOID NLS_STR::DonePartying( VOID )
{
	DonePartying( ::strlenf( _pchData ) );
}
