/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strrchr.cxx
	NLS/DBCS-aware string class: strrchr method

	This file contains the implementation of the strrchr method
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

	NAME:		NLS_STR::strrchr

	SYNOPSIS:	Puts the index of the last occurrence of ch in *this into
				istrPos.

	ENTRY:

	EXIT:

	NOTES:

	HISTORY:
		johnl	11/26/90	Written
		beng	07/23/91	Allow on erroneous string; update CheckIstr

********************************************************************/

BOOL NLS_STR::strrchr( ISTR * pistrPos, const CHAR ch ) const
{
	if (QueryError())
		return FALSE;

	UpdateIstr( pistrPos );
	CheckIstr( *pistrPos );

	const CHAR * pchStrRes = ::strrchrf( QueryPch(), ch );

	if ( pchStrRes == NULL )
	{
		pistrPos->SetIB( strlen() );
		return FALSE;
	}

	pistrPos->SetIB((int) (pchStrRes - QueryPch()));
	return TRUE;
}


BOOL NLS_STR::strrchr(
	ISTR *pistrPos,
	const CHAR ch,
	const ISTR& istrStart ) const
{
	if (QueryError())
		return FALSE;

	CheckIstr( istrStart );
	UpdateIstr( pistrPos );
	CheckIstr( *pistrPos );

	const CHAR * pchStrRes = ::strrchrf(QueryPch(istrStart), ch );

	if ( pchStrRes == NULL )
	{
		pistrPos->SetIB( strlen() );
		return FALSE;
	}

	pistrPos->SetIB((int) (pchStrRes - QueryPch()));
	return TRUE;
}
