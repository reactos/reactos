/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	istraux.cpp
	NLS/DBCS-aware string class: secondary methods of index class

	This file contains the implementation of the auxiliary methods
	for the ISTR class.  It is separate so that clients of STRING which
	do not use this operator need not link to it.

	FILE HISTORY:
		beng	01/18/91	Separated from original monolithic .cxx
		beng	02/07/91	Uses lmui.hxx
		beng	04/26/91	Relocated some funcs from string.hxx
		gregj	03/25/93	Ported to Chicago environment
		gregj	04/02/93	Use NLS_STR::IsDBCSLeadByte()
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

	NAME:		ISTR::Reset

	SYNOPSIS:	Reset the ISTR so the index is 0;
				updates the version number of the string.

	ENTRY:

	EXIT:

	NOTES:

	HISTORY:
		Johnl	11/28/90	Created

********************************************************************/

VOID ISTR::Reset()
{
	_ibString = 0;
#ifdef DEBUG
	_usVersion = QueryPNLS()->QueryVersion();
#endif
}


/*******************************************************************

	NAME:		ISTR::operator-

	SYNOPSIS:	Returns the difference in CB between the two ISTR

	ENTRY:

	EXIT:

	NOTES:

	HISTORY:
		Johnl	11/28/90	Created

********************************************************************/

INT ISTR::operator-( const ISTR& istr2 ) const
{
	UIASSERT( QueryPNLS() == istr2.QueryPNLS() );

	return ( QueryIB() - istr2.QueryIB() );
}


/*******************************************************************

	NAME:		ISTR::operator++

	SYNOPSIS:	Increment the ISTR to the next logical character

	ENTRY:

	EXIT:

	NOTES:		Stops if we are at the end of the string

	HISTORY:
		Johnl	11/28/90	Created
		beng	07/23/91	Simplified CheckIstr

********************************************************************/

ISTR& ISTR::operator++()
{
	QueryPNLS()->CheckIstr( *this );
	CHAR c = *(QueryPNLS()->QueryPch() + QueryIB());
	if ( c != '\0' )
	{
		SetIB( QueryIB() + (QueryPNLS()->IsDBCSLeadByte(c) ? 2 : 1) );
	}
	return *this;
}


/*******************************************************************

	NAME:		ISTR::operator+=

	SYNOPSIS:	Increment the ISTR to the nth logical character

	NOTES:		Stops if we are at the end of the string

	HISTORY:
		Johnl	01/14/90	Created

********************************************************************/

VOID ISTR::operator+=( INT iChars )
{
	while ( iChars-- )
		operator++();
}


/*******************************************************************

	NAME:		ISTR::operator==

	SYNOPSIS:	Equality operator

	RETURNS:	TRUE if the two ISTRs are equivalent.

	NOTES:		Only valid between two ISTRs of the same string.

	HISTORY:
		beng	07/22/91	Header added

********************************************************************/

BOOL ISTR::operator==( const ISTR& istr ) const
{
	UIASSERT( QueryPNLS() == istr.QueryPNLS() );
	return QueryIB() == istr.QueryIB();
}


/*******************************************************************

	NAME:		ISTR::operator>

	SYNOPSIS:	Greater-than operator

	RETURNS:	TRUE if this ISTR points further into the string
				than the argument.

	NOTES:		Only valid between two ISTRs of the same string.

	HISTORY:
		beng	07/22/91	Header added

********************************************************************/

BOOL ISTR::operator>( const ISTR& istr )  const
{
	UIASSERT( QueryPNLS() == istr.QueryPNLS() );
	return QueryIB() > istr.QueryIB();
}


/*******************************************************************

	NAME:		ISTR::operator<

	SYNOPSIS:	Lesser-than operator

	RETURNS:	TRUE if this ISTR points less further into the string
				than the argument.

	NOTES:		Only valid between two ISTRs of the same string.

	HISTORY:
		beng	07/22/91	Header added

********************************************************************/

BOOL ISTR::operator<( const ISTR& istr )  const
{
	UIASSERT( QueryPNLS() == istr.QueryPNLS() );
	return QueryIB() < istr.QueryIB();
}
