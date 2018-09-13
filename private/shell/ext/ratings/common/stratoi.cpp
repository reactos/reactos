/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	stratoi.cxx
	NLS/DBCS-aware string class: atoi method

	This file contains the implementation of the atoi method
	for the STRING class.  It is separate so that clients of STRING which
	do not use this member function need not link to it.

	FILE HISTORY:
		markbl	06/04/91	Created

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

	NAME:		NLS_STR::atoi

	SYNOPSIS:	Returns *this in its integer numeric equivalent

	ENTRY:		With no arguments, parses from beginning of string.
				Given an ISTR, starts at that point within the string.

	EXIT:

	NOTES:		Uses C-Runtime atoi function

	HISTORY:
		markbl	06/04/91	Written
		beng	07/22/91	Callable on erroneous string; simplified CheckIstr

********************************************************************/

INT NLS_STR::atoi() const
{
	if (QueryError())
		return 0;

	return ::atoi( _pchData );
}


INT NLS_STR::atoi( const ISTR & istrStart ) const
{
	if (QueryError())
		return 0;

	CheckIstr( istrStart );

	return ::atoi( QueryPch(istrStart) );
}
