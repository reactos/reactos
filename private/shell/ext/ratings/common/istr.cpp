/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	istr.cxx
	NLS/DBCS-aware string class: string index class

	This file contains the core implementation of the string
	indexer class.

	FILE HISTORY:
		gregj	03/30/93	Removed ISTR to separate module
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

	NAME:      ISTR::ISTR

	SYNOPSIS:  ISTR construction methods

	ENTRY:
		ISTR::ISTR( ISTR& ) - Copy passed ISTR (both string and positional
							  info is copied).

		ISTR::ISTR( IB, NLS_STR& ) - Private, create an ISTR with index
									 at IB for string NLS_STR

	EXIT:

	NOTES:

	HISTORY:
		johnl	11/20/90	Created

********************************************************************/

ISTR::ISTR( const ISTR& istr )
{
	*this = istr;
}


ISTR::ISTR( const NLS_STR& nls )
{
	*this = nls;
}


/*******************************************************************

	NAME:		ISTR::operator=

	SYNOPSIS:	Copy operator for the ISTR class

	ENTRY:

	EXIT:

	NOTES:

	HISTORY:
		Johnl	11/20/90	Created
		gregj	03/30/93	Allow assignment of NLS_STR to ISTR

********************************************************************/

ISTR& ISTR::operator=( const ISTR& istr )
{
	_ibString = istr._ibString;
	SetPNLS( (NLS_STR *) istr.QueryPNLS() );
#ifdef DEBUG
	_usVersion = istr._usVersion;
#endif
	return *this;
}


ISTR& ISTR::operator=( const NLS_STR& nls )
{
	_ibString = 0;
	SetPNLS( &nls );
#ifdef DEBUG
	_usVersion = nls.QueryVersion();
#endif
	return *this;
}
