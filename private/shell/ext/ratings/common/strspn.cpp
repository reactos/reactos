/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strspn.cxx
	NLS/DBCS-aware string class: strspn method

	This file contains the implementation of the strspn method
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

	NAME:		NLS_STR::strspn

	SYNOPSIS:	Find first char in *this that is not a char in arg. and puts
				the position in pistrPos.
				Returns FALSE when no characters do not match

	ENTRY:

	EXIT:

	NOTES:

	HISTORY:
		johnl	11/16/90	Written
		beng	07/23/91	Allow on erroneous string; simplified CheckIstr

********************************************************************/

BOOL NLS_STR::strspn( ISTR * pistrPos, const NLS_STR & nls ) const
{
	if (QueryError() || !nls)
		return FALSE;

	UpdateIstr( pistrPos );
	CheckIstr( *pistrPos );

	pistrPos->SetIB( ::strspnf( QueryPch(), nls.QueryPch() ) );
	return *QueryPch( *pistrPos ) != '\0';
}


BOOL NLS_STR::strspn( ISTR *	      pistrPos,
					  const NLS_STR & nls,
					  const ISTR    & istrStart ) const
{
	if (QueryError() || !nls)
		return FALSE;

	UpdateIstr( pistrPos );
	CheckIstr( istrStart );

	pistrPos->SetIB( ::strspnf(QueryPch( istrStart ), nls.QueryPch() ) +
					 istrStart.QueryIB()  );
	return *QueryPch( *pistrPos ) != '\0';
}
