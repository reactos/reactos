/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strdss.cxx
	NLS/DBCS-aware string class: DelSubStr method

	This file contains the implementation of the DelSubStr method
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


/*******************************************************************

	NAME:		NLS_STR::DelSubStr

	SYNOPSIS:	Collapse the string by removing the characters from
				istrStart to:
				  istrStart  to the end of string
				  istrStart + istrEnd
				The string is not reallocated

	ENTRY:

	EXIT:		Modifies istrStart

	NOTES:		The method DelSubStr( ISTR&, CB) is private and does
				the work.

	HISTORY:
		johnl	11/26/90	Created
		beng	04/26/91	Replaced CB with INT
		beng	07/23/91	Allow on erroneous strings; simplified CheckIstr

********************************************************************/

VOID NLS_STR::DelSubStr( ISTR & istrStart, INT cbLen )
{
	if (QueryError())
		return;

	CheckIstr( istrStart );

	// cbLen == -1 means delete to end of string
	if ( cbLen == -1 )
		*(_pchData + istrStart.QueryIB() ) = '\0';
	else
	{
		INT cbNewEOS = 1 + ::strlenf( QueryPch(istrStart) + cbLen );

		::memmovef( (CHAR *)QueryPch(istrStart),
					(CHAR *)QueryPch(istrStart) + cbLen,
					cbNewEOS );
	}

	_cchLen = ::strlenf( QueryPch() );

	IncVers();
	UpdateIstr( &istrStart );
}


VOID NLS_STR::DelSubStr( ISTR & istrStart )
{
	if (QueryError())
		return;

	DelSubStr( istrStart, -1 );
}


VOID NLS_STR::DelSubStr( ISTR & istrStart, const ISTR & istrEnd  )
{
	if (QueryError())
		return;

	CheckIstr( istrEnd );
	UIASSERT( istrEnd.QueryIB() >= istrStart.QueryIB() );

	DelSubStr( istrStart, istrEnd - istrStart );
}
