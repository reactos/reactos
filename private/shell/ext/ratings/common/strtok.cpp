/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strtok.cxx
	NLS/DBCS-aware string class: strtok method

	This file contains the implementation of the strtok method
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

	NAME:		NLS_STR::strtok

	SYNOPSIS:	Basic strtok functionality.  Returns FALSE after the
				string has been traversed.

	ENTRY:

	EXIT:

	NOTES:		We don't update the version on the string since the
				::strtokf shouldn't cause DBCS problems.  It would also
				be painful on the programmer if on each call to strtok
				they had to update all of the ISTR associated with this
				string

				fFirst is required to be TRUE on the first call to
				strtok, it is FALSE afterwards (is defaulted to FALSE)

	CAVEAT:		Under windows, all calls to strtok must be done while
				processing a single message.  Otherwise another process
				my confuse it.

	HISTORY:
		johnl	11/26/90	Created
		beng	07/23/91	Allow on erroneous string

********************************************************************/

BOOL NLS_STR::strtok( 
	ISTR *pistrPos,
	const NLS_STR& nlsBreak,
	BOOL fFirst )
{
	if (QueryError())
		return FALSE;

	const CHAR * pchToken = ::strtokf( fFirst ? _pchData : NULL, (CHAR *)nlsBreak.QueryPch());

	if ( pchToken == NULL )
	{
		pistrPos->SetIB( strlen() );
		return FALSE;
	}

	pistrPos->SetIB((int) (pchToken - QueryPch()));
	return TRUE;
}
