/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strinsrt.cxx
	NLS/DBCS-aware string class: InsertParams method

	This file contains the implementation of the InsertParams method
	for the STRING class.  It is separate so that clients of STRING which
	do not use this operator need not link to it.

	FILE HISTORY:
		Johnl	01/31/91	Created
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


#define MAX_INSERT_PARAMS	9

/*******************************************************************

	NAME:		NLS_STR::InsertParams

	SYNOPSIS:	Fill in a message string from the resource file replacing
				the number parameters with the real text.

	ENTRY:		pchMessage is a pointer to the message text
				apnlsParamStrings is an array of pointers to NLS_STRs

				Example:

				*this = "Error %1 occurred, do %2, or %1 will happen again"
				apnlsParamStrings[0] = "696969"
				apnlsParamStrings[1] = "Something else"

				Return string = "Error 696969 occurred, do Something else or
				696969 will happen again"

	EXIT:		0 if successful, error code otherwise, one of:
					WN_OUT_OF_MEMORY

	NOTES:		The minimum parameter is 1, the maximum parameter is 9.
				The array of param strings must have a NULL to mark
				the end of the array.

	HISTORY:
		JohnL	01/30/91	Created
		beng	04/26/91	Uses WCHAR
		beng	07/23/91	Allow on erroneous string

********************************************************************/

#define PARAM_ESC  '%'

USHORT NLS_STR::InsertParams( const NLS_STR * apnlsParamStrings[] )
{
	if (QueryError())
		return (USHORT) QueryError();

	INT iNumParams = 0;	// Number of param strings in the array
						// Max string length of expanded message (include \0)
	INT iMaxMessLen = strlen() + 1;

	/* How many parameters were we passed?
	*/
	for ( ; apnlsParamStrings[iNumParams] != NULL ; iNumParams++ )
		;

	UIASSERT(iNumParams <= MAX_INSERT_PARAMS);
	if ( iNumParams > MAX_INSERT_PARAMS )
		return WN_OUT_OF_MEMORY;

	/* Determine total string length required for the expanded string
	 * and get out if we can't fulfill the request
	 */

	ISTR istrCurPos( *this );
	while ( 1 )
	{
		if ( !strchr( &istrCurPos, PARAM_ESC, istrCurPos ) )
			break;

		WCHAR wchParam = QueryChar( ++istrCurPos );

		if ( wchParam >= '1' && wchParam <= '9' )
		{
			INT iParamIndex = wchParam - '1';
			if ( iNumParams < iParamIndex )
				return WN_OUT_OF_MEMORY;

			iMaxMessLen += apnlsParamStrings[iParamIndex]->strlen() - 2;
		}
	}

	if ( iMaxMessLen > QueryAllocSize() )
	{
		if ( IsOwnerAlloc() )
			return WN_OUT_OF_MEMORY;
		else
			if ( !realloc( iMaxMessLen ) )
				return WN_OUT_OF_MEMORY;
	}

	/* Now do the parameter substitution
	 */

	istrCurPos.Reset();
	for (;;)
	{
		if ( !strchr( &istrCurPos, PARAM_ESC, istrCurPos ) )
			break;

		ISTR istrParamEsc( istrCurPos );
		WCHAR wchParam = QueryChar( ++istrCurPos );

		if ( wchParam >= '1' && wchParam <= '9' )
		{
			INT iParamIndex = wchParam - '1';

			if (iParamIndex < iNumParams) {
				ReplSubStr( *apnlsParamStrings[iParamIndex],
							istrParamEsc,
							++istrCurPos ) ;   // Replace #
				// Skip past entire substituted string
				istrCurPos.SetIB(istrParamEsc.QueryIB() +
								 apnlsParamStrings[iParamIndex]->strlen());
			}
			// else istrCurPos has been advanced past the out-of-range digit
		}
	}

	return 0;
}
