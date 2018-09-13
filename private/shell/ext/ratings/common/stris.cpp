/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	stris.cxx
	NLS/DBCS-aware string class: InsertStr method

	This file contains the implementation of the InsertStr method
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

	NAME:		NLS_STR::InsertStr

	SYNOPSIS:	Insert passed string into *this at istrStart

	ENTRY:

	EXIT:		If this function returns FALSE, ReportError has been
				called to report the error that occurred.

	RETURN:		TRUE on success, FALSE otherwise.

	NOTES:		If *this is not STR_OWNERALLOCed and the inserted string
				won't fit in the allocated space for *this, then *this
				will be reallocated.

	HISTORY:
		johnl	11/28/90	Created
		rustanl 04/14/91	Fixed new length calculation.  Report
							error if owner alloc'd and not enough
							space.
		beng	04/26/91	Replaced CB with INT
		beng	07/23/91	Allow on erroneous string;
							simplified CheckIstr

********************************************************************/

BOOL NLS_STR::InsertStr( const NLS_STR & nlsIns, ISTR & istrStart )
{
	if (QueryError() || !nlsIns)
		return FALSE;

	CheckIstr( istrStart );

	INT cbNewSize = strlen() + nlsIns.strlen() + 1 ; // include new null char

	if ( QueryAllocSize() < cbNewSize )
	{
		if ( IsOwnerAlloc())
		{
			// Big trouble!  Report error, and return failure.
			//
			UIASSERT( !"Owner alloc'd string not big enough" );
			ReportError( WN_OUT_OF_MEMORY );
			return FALSE;
		}

		// Attempt to allocate more memory
		//
		if ( !realloc( cbNewSize ) )
		{
			ReportError( WN_OUT_OF_MEMORY );
			return FALSE;
		}
	}

	::memmovef( (CHAR *)QueryPch(istrStart) + nlsIns.strlen(),
				(CHAR *)QueryPch(istrStart),
				::strlenf(QueryPch(istrStart) ) + 1 );
	::memmovef( (CHAR *)QueryPch(istrStart),
				(CHAR *)nlsIns.QueryPch(),
				nlsIns.strlen() );

	UIASSERT( cbNewSize >= 1 ); // should have been assigned something +1 above
	_cchLen = cbNewSize - 1;	// don't count null character here

	IncVers();
	UpdateIstr( &istrStart );		// This ISTR does not become invalid
									// after the string update
	return TRUE;
}
