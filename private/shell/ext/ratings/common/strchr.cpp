/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strchr.cxx
	NLS/DBCS-aware string class: strchr method

	This file contains the implementation of the strchr method
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

	NAME:		NLS_STR::strchr

	SYNOPSIS:	Puts the index of the first occurrence of ch in *this
				into istrPos.


	ENTRY:		pistrPos - points to ISTR in which to leave pos
				ch		 - character sought
				istrStart- staring point in string.  If omitted, start
							at beginning

	EXIT:		pistrPos

	RETURNS:	TRUE if character found; otherwise FALSE

	NOTES:		This routine only works for CHAR - not WCHAR.
				Hence it's useless for double-byte characters
				under MBCS.

	HISTORY:
		johnl	11/26/90	Written
		beng	07/22/91	Allow on erroneous strings; simplified CheckIstr

********************************************************************/

BOOL NLS_STR::strchr( ISTR * pistrPos, const CHAR ch ) const
{
	if ( QueryError() )
		return FALSE;

	UpdateIstr( pistrPos );
	CheckIstr( *pistrPos );

	const CHAR * pchStrRes = ::strchrf( QueryPch(), ch );

	if ( pchStrRes == NULL )
	{
		pistrPos->SetIB( strlen() );
		return FALSE;
	}

	pistrPos->SetIB((DWORD)(pchStrRes - QueryPch()));
	return TRUE;
}


BOOL NLS_STR::strchr( ISTR * pistrPos, const CHAR ch,
					  const ISTR & istrStart ) const
{
	if ( QueryError() )
		return FALSE;

	CheckIstr( istrStart );
	UpdateIstr( pistrPos );
	CheckIstr( *pistrPos );

	const CHAR * pchStrRes = ::strchrf( QueryPch(istrStart), ch );

	if ( pchStrRes == NULL )
	{
		pistrPos->SetIB( strlen() );
		return FALSE;
	}

	pistrPos->SetIB((DWORD)(pchStrRes - QueryPch()));
	return TRUE;
}
