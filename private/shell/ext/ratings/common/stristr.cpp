/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	stristr.cxx
	NLS/DBCS-aware string class: stristr method

	This file contains the implementation of the stristr method
	for the STRING class.  It is separate so that clients of STRING which
	do not use this operator need not link to it.

	FILE HISTORY:
		beng	11/18/91	Separated from original monolithic .cxx
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

	NAME:	NLS_STR::stristr

	SYNOPSIS:	Same as strstr on case insensitive

	ENTRY:

	EXIT:

	NOTES:

	HISTORY:
		johnl	11/16/90	Written
		beng	07/23/91	Allow on erroneous string; simplified CheckIstr

********************************************************************/

BOOL NLS_STR::stristr( ISTR * pistrPos, const NLS_STR & nls ) const
{
	if (QueryError() || !nls)
		return FALSE;

	UpdateIstr( pistrPos );
	CheckIstr( *pistrPos );

	const CHAR * pchStrRes = ::stristrf( QueryPch(), nls.QueryPch() );

	if ( pchStrRes == NULL )
	{
		pistrPos->SetIB( strlen() );
		return FALSE;
	}

	pistrPos->SetIB((int) (pchStrRes - QueryPch()));
	return TRUE;
}


BOOL NLS_STR::stristr( ISTR *    pistrPos,
					   const NLS_STR & nls,
					   const ISTR    & istrStart ) const
{
	if (QueryError() || !nls)
		return FALSE;

	CheckIstr( istrStart );
	UpdateIstr( pistrPos );
	CheckIstr( *pistrPos );

	const CHAR * pchStrRes = ::stristrf(QueryPch(istrStart), nls.QueryPch() );

	if ( pchStrRes == NULL )
	{
		pistrPos->SetIB( strlen() );
		return FALSE;
	}

	pistrPos->SetIB((int) (pchStrRes - QueryPch()));
	return TRUE;
}
