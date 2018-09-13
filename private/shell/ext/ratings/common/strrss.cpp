/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strrss.cxx
	NLS/DBCS-aware string class: strrss method

	This file contains the implementation of the strrss method
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

	NAME:		NLS_STR::ReplSubStr

	SYNOPSIS:	Replace the substring starting at istrStart with the
				passed nlsRepl string.

				If both a start and end is passed, then the operation is
				equivalent to a DelSubStr( start, end ) and an
				InsertSubStr( start ).

				If just a start is passed in, then the operation is
				equivalent to DelSubStr( start ), concat new string to end.

				The ReplSubStr( NLS_STR&, istrStart&, INT cbDel) method is
				private.

	ENTRY:

	EXIT:

	NOTES:

	HISTORY:
		johnl	11/29/90	Created
		beng	04/26/91	Replaced CB with INT
		beng	07/23/91	Allow on erroneous string; simplified CheckIstr

********************************************************************/

VOID NLS_STR::ReplSubStr( const NLS_STR & nlsRepl, ISTR& istrStart )
{
	if (QueryError() || !nlsRepl)
		return;

	CheckIstr( istrStart );

	DelSubStr( istrStart );
	strcat( nlsRepl );
}


VOID NLS_STR::ReplSubStr( const NLS_STR& nlsRepl,
						  ISTR& istrStart,
						  const ISTR& istrEnd )
{
	CheckIstr( istrEnd );
	UIASSERT( istrEnd.QueryIB() >= istrStart.QueryIB() );

	ReplSubStr( nlsRepl, istrStart, istrEnd - istrStart );
}


VOID NLS_STR::ReplSubStr( const NLS_STR& nlsRepl,
						  ISTR& istrStart,
						  INT cbToBeDeleted )
{
	if (QueryError() || !nlsRepl)
		return;

	CheckIstr( istrStart );

	INT cbRequired = strlen() - cbToBeDeleted + nlsRepl.strlen() + 1;
	if ( !IsOwnerAlloc() && QueryAllocSize() < cbRequired )
	{
		if ( !realloc( cbRequired ) )
		{
			ReportError( WN_OUT_OF_MEMORY );
			return;
		}
	}
	else
		UIASSERT( QueryAllocSize() >= cbRequired );

	CHAR * pchStart = (CHAR *)QueryPch(istrStart) + cbToBeDeleted;
	::memmovef( pchStart + nlsRepl.strlen()-cbToBeDeleted,
				pchStart,
				::strlenf( pchStart ) + 1 );
	::memmovef( (CHAR *)QueryPch(istrStart),
				nlsRepl._pchData,
				nlsRepl.strlen() );

	_cchLen = strlen() + nlsRepl.strlen() - cbToBeDeleted;

	IncVers();
	UpdateIstr( &istrStart );
}
