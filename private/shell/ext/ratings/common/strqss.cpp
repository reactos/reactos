/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strqss.cxx
	NLS/DBCS-aware string class: QuerySubStr method

	This file contains the implementation of the QuerySubStr method
	for the STRING class.  It is separate so that clients of STRING which
	do not use this operator need not link to it.

	FILE HISTORY:
		beng	01/18/91	Separated from original monolithic .cxx
		beng	02/07/91	Uses lmui.hxx

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


// Magic value used below
//
#define CB_ENTIRE_STRING (-1)


/*******************************************************************

	NAME:		NLS_STR::QuerySubStr

	SYNOPSIS:	Return a pointer to a new NLS_STR that contains the contents
				of *this from istrStart to:
				  istrStart end of string or
				  istrStart + istrEnd

	ENTRY:

	EXIT:

	RETURNS:	Pointer to newly alloc'd NLS_STR, or NULL if error

	NOTES:		The private method QuerySubStr(ISTR&, CB) is the worker
				method, the other two just check the parameters and
				pass the data. It is private since we can't allow the
				user to access the string on a byte basis

	CAVEAT:		Note that this method creates an NLS_STR that the client is
				responsible for deleting.

	HISTORY:
		johnl	11/26/90	Created
		beng	04/26/91	Replaced CB wth INT; broke out CB_ENTIRE_STRING
							magic value
		beng	07/23/91	Allow on erroneous string; simplified CheckIstr

********************************************************************/

NLS_STR * NLS_STR::QuerySubStr( const ISTR & istrStart, INT cbLen ) const
{
	if (QueryError())
		return NULL;

	CheckIstr( istrStart );

	INT cchStrLen = ::strlenf(QueryPch(istrStart) );
	INT cbCopyLen = ( cbLen == CB_ENTIRE_STRING || cbLen >= cchStrLen )
					? cchStrLen
					: cbLen;

	NLS_STR *pnlsNew = new NLS_STR( cbCopyLen + 1 );
	if ( pnlsNew == NULL )
		return NULL;

	if ( pnlsNew->QueryError() )
	{
		delete pnlsNew;
		return NULL;
	}

	::memcpyf( pnlsNew->_pchData, QueryPch(istrStart), cbCopyLen );
	*(pnlsNew->_pchData + cbCopyLen) = '\0';

	pnlsNew->_cchLen = cbCopyLen;

	return pnlsNew;
}


NLS_STR * NLS_STR::QuerySubStr( const ISTR & istrStart ) const
{
	return QuerySubStr( istrStart, CB_ENTIRE_STRING );
}


NLS_STR * NLS_STR::QuerySubStr( const ISTR  & istrStart,
								const ISTR  & istrEnd  ) const
{
	CheckIstr( istrEnd );
	UIASSERT( istrEnd.QueryIB() >= istrStart.QueryIB() );

	return QuerySubStr( istrStart, istrEnd - istrStart );
}
