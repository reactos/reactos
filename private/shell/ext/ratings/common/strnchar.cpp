/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strnchar.cxx
	NLS/DBCS-aware string class:QueryNumChar method

	This file contains the implementation of the QueryNumChar method
	for the STRING class.  It is separate so that clients of STRING which
	do not use this operator need not link to it.

	FILE HISTORY:
		terryk	04/04/91	Creation

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


#ifdef EXTENDED_STRINGS
/*******************************************************************

	NAME:		NLS_STR::QueryNumChar

	SYNOPSIS:	return the total number of character within the string   

	RETURNS:	The number of logical character within the string

	NOTES:
		Treats erroneous string as having length 0

	HISTORY:
		terryk	04/04/91	Written
		beng	07/23/91	Allow on erroneous string

********************************************************************/

INT NLS_STR::QueryNumChar() const
{
	if (QueryError())
		return 0;

	ISTR  istrCurPos( *this );
	INT   cchCounter = 0;

	for ( ;
		this->QueryChar( istrCurPos ) != '\0';
		istrCurPos++, cchCounter ++ )
		;

	return cchCounter;
}


/*******************************************************************

	NAME:		NLS_STR::QueryTextLength

	SYNOPSIS:	Calculate length of text in CHARS, sans terminator

	RETURNS:	Count of CHARs

	NOTES:
		Compare QueryNumChar, which returns a number of glyphs.
		In a DBCS environment, this member will return 2 CHARS for
		each double-byte character, since a CHAR is there only 8 bits.

	HISTORY:
		beng	07/23/91	Created

********************************************************************/

INT NLS_STR::QueryTextLength() const
{
	return _cchLen / sizeof(CHAR);
}


/*******************************************************************

	NAME:		NLS_STR::QueryTextSize

	SYNOPSIS:	Calculate length of text in BYTES, including terminator

	RETURNS:	Count of BYTES

	NOTES:
		QueryTextSize returns the number of bytes needed to duplicate
		the string into a byte vector.

    HISTORY:
		beng	07/23/91	Created

********************************************************************/

INT NLS_STR::QueryTextSize() const
{
	return _cchLen+sizeof(CHAR);
}
#endif	// EXTENDED_STRINGS
