/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/

/*
	strncpy.c
	NLS/DBCS-aware string class: strncpy method

	This file contains the implementation of the strncpy method
	for the STRING class.  It is separate so that clients of STRING which
	do not use this operator need not link to it.

	FILE HISTORY:
		gregj	04/08/93	Created
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

	NAME:		NLS_STR::strncpy

	SYNOPSIS:	Copy non-null-terminated string

	ENTRY:		pchSource - string to copy
				cbSource - number of bytes to copy

	EXIT:		If successful, contents of string overwritten.
				If failed, the original contents of the string remain.

	RETURNS:	Reference to self.

	HISTORY:
		gregj	04/08/93	Created

********************************************************************/

NLS_STR& NLS_STR::strncpy( const CHAR *pchSource, UINT cbSource )
{
	if ( cbSource == 0)
		pchSource = NULL;

	if ( pchSource == NULL )
	{
		if ( !IsOwnerAlloc() && !QueryAllocSize() )
		{
			if ( !Alloc(1) )
				ReportError( WN_OUT_OF_MEMORY );
			return *this;
		}

		UIASSERT( QueryAllocSize() > 0 );

		*_pchData = '\0';
		_cchLen = 0;
	}
	else
	{
		if ( !IsOwnerAlloc() )
		{
			if ( (UINT)QueryAllocSize() < cbSource + 1 )
			{
				CHAR * pchNew = new CHAR[cbSource + 1];

				if ( pchNew == NULL )
				{
					ReportError( WN_OUT_OF_MEMORY );
					return *this;
				}

				delete _pchData;
				_pchData = pchNew;
				_cbData = cbSource + 1;
			}
		}
		else
		{
			if ((UINT)QueryAllocSize() < cbSource + 1)
				cbSource = QueryAllocSize() - 1;
		}

		::strncpyf( _pchData, pchSource, cbSource );

		/*
		 * Get the new length of the string.  It may not necessarily be
		 * cbSource because if the string is getting truncated, cbSource
		 * might be halfway through a double-byte character.
		 */

		 _pchData[cbSource] = '\0';

		_cchLen = ::strlenf( _pchData );
		
	}

	IncVers();

	/* Reset the error state, since the string is now valid.
	 */	
	ReportError( WN_SUCCESS );
	return *this;
}
