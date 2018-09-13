/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strcmp.cxx
	NLS/DBCS-aware string class: strcmp method and equality operator

	This file contains the implementation of the strcmp method
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

	NAME:		NLS_STR::operator==

	SYNOPSIS:	Case-sensitive test for equality

	RETURNS:	TRUE if the two operands are equal (case sensitive);
				else FALSE

	NOTES:		An erroneous string matches nothing.

	HISTORY:
		johnl	11/11/90	Written
		beng	07/23/91	Allow on erroneous strings

********************************************************************/

BOOL NLS_STR::operator==( const NLS_STR & nls ) const
{
	if (QueryError() || !nls)
		return FALSE;

	return !::strcmpf( QueryPch(), nls.QueryPch() );
}


/*******************************************************************

	NAME:		NLS_STR::operator!=

	SYNOPSIS:	Case-sensitive test for inequality

	RETURNS:	TRUE if the two operands are unequal (case sensitive);
				else FALSE

	NOTES:		An erroneous string matches nothing.

	HISTORY:
		beng	07/23/91	Header added

********************************************************************/

BOOL NLS_STR::operator!=( const NLS_STR & nls ) const
{
	return ! operator==( nls );
}


/*******************************************************************

	NAME:		NLS_STR::strcmp

	SYNOPSIS:	Standard string compare with optional character indexes

	ENTRY:		nls					  - string against which to compare
				istrStart1 (optional) - index into "this"
				istrStart2 (optional) - index into "nls"

	RETURNS:	As the C runtime "strcmp".

	NOTES:		If either string is erroneous, return "match."
				This runs contrary to the eqop.

				Glock doesn't allow default parameters which require
				construction; hence this member is overloaded multiply.

	HISTORY:
		johnl	11/15/90	Written
		johnl	11/19/90	Changed to use ISTR, overloaded for
							different number of ISTRs
		beng	07/23/91	Allow on erroneous strings;
							simplified CheckIstr

********************************************************************/

int NLS_STR::strcmp( const NLS_STR & nls ) const
{
	if (QueryError() || !nls)
		return 0;

	return ::strcmpf( QueryPch(), nls.QueryPch() );
}

int NLS_STR::strcmp(
	const NLS_STR & nls,
	const ISTR	  & istrStart1 ) const
{
	if (QueryError() || !nls)
		return 0;

	CheckIstr( istrStart1 );

	return ::strcmpf( QueryPch(istrStart1) , nls.QueryPch() );
}

int NLS_STR::strcmp(
	const NLS_STR & nls,
	const ISTR	  & istrStart1,
	const ISTR	  & istrStart2 ) const
{
	if (QueryError() || !nls)
		return 0;

	CheckIstr( istrStart1 );
	nls.CheckIstr( istrStart2 );

	return ::strcmpf( QueryPch(istrStart1), nls.QueryPch(istrStart2) );
}
