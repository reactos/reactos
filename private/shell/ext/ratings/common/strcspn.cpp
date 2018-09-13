/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strcspn.cxx
	NLS/DBCS-aware string class: strcspn method

	This file contains the implementation of the strcspn method
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

	NAME:		NLS_STR::strcspn

	SYNOPSIS:	Set membership.  Finds the first matching character
				in the passed string

	ENTRY:		pistrPos - destination for results
				nls	 - set of sought characters

	EXIT:		*pistrPos contains offset within "this" of element
				found (assuming it was successful); otherwise it
				is moved to the end of the string.

	RETURNS:	TRUE if any character found; FALSE otherwise

	NOTES:

	HISTORY:
		johnl	11/16/90	Written
		beng	07/23/91	Allow on erroneous strings; simplified CheckIstr

********************************************************************/

BOOL NLS_STR::strcspn( ISTR* pistrPos, const NLS_STR & nls ) const
{
	if (QueryError() || !nls)
		return FALSE;

	UpdateIstr( pistrPos );
	CheckIstr( *pistrPos );

	pistrPos->SetIB( ::strcspnf( QueryPch(), nls.QueryPch() ) );
	return *QueryPch( *pistrPos ) != '\0';
}


BOOL NLS_STR::strcspn( ISTR * pistrPos, const NLS_STR & nls,
						const ISTR& istrStart ) const
{
	if (QueryError() || !nls)
		return FALSE;

	UpdateIstr( pistrPos );
	CheckIstr( *pistrPos );
	CheckIstr( istrStart );

	pistrPos->SetIB( ::strcspnf( QueryPch(istrStart), nls.QueryPch() )
								 + istrStart.QueryIB()  );
	return *QueryPch( *pistrPos ) != '\0';
}
