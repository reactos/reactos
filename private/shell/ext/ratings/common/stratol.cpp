/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	stratol.cxx
	NLS/DBCS-aware string class: atol method

	This file contains the implementation of the atol method
	for the STRING class.  It is separate so that clients of STRING which
	do not use this member function need not link to it.

	FILE HISTORY:
		beng	01/18/91	Separated from original monolithic .cxx
		beng	02/07/91	Uses lmui.hxx

*/

#include "npcommon.h"

extern "C"
{
	#include <netlib.h>
	#include <stdlib.h>
}

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <npassert.h>

#include <npstring.h>


/*******************************************************************

	NAME:		NLS_STR::atol

	SYNOPSIS:	Returns *this in its long numeric equivalent

	ENTRY:		With no arguments, parses from beginning of string.
				Given an ISTR, starts at that point within the string.

	EXIT:

	NOTES:		Uses C-Runtime atol function

	HISTORY:
		johnl	11/26/90	Written
		beng	07/22/91	Callable on erroneous string; simplified CheckIstr

********************************************************************/

LONG NLS_STR::atol() const
{
	if (QueryError())
		return 0;

	return ::atol( _pchData );
}


LONG NLS_STR::atol( const ISTR & istrStart ) const
{
	if (QueryError())
		return 0;

	CheckIstr( istrStart );

	return ::atol( QueryPch(istrStart) );
}
