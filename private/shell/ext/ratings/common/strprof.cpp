/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strprof.c
	NLS/DBCS-aware string class:  GetPrivateProfileString method

	This file contains the implementation of the GetPrivateProfileString method
	for the NLS_STR class.  It is separate so that clients of NLS_STR who
	do not use this operator need not link to it.

	FILE HISTORY:
		04/08/93	gregj	Created
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

	NAME:		NLS_STR::GetPrivateProfileString

	SYNOPSIS:	Loads a string from an INI file.

	ENTRY:		pszFile - name of INI file to read.
				pszSection - name of section (excluding square brackets).
				pszKey - key name to retrieve.
				pszDefault - default value if key not found.

	EXIT:		String contains the value associated with the key.

	NOTES:		The string is truncated if it's being loaded into an
				owner-alloc string and doesn't entirely fit.

				No character-set assumptions are made about the string.
				If the character set of the string being loaded is
				different from the ambient set of the NLS_STR, use
				SetOEM() or SetAnsi() to make the NLS_STR correct.

	HISTORY:
		gregj	04/08/93	Created

********************************************************************/

VOID NLS_STR::GetPrivateProfileString( const CHAR *pszFile,
									   const CHAR *pszSection,
									   const CHAR *pszKey,
									   const CHAR *pszDefault /* = NULL */ )
{
	static CHAR szNull[] = "";

	if (QueryError())
		return;

	if (pszDefault == NULL)
		pszDefault = szNull;

	if (!IsOwnerAlloc() && !QueryAllocSize()) {
		if (!realloc( MAX_RES_STR_LEN )) {
			ReportError( WN_OUT_OF_MEMORY );
			return;
		}
	}

	INT cbCopied;

	for (;;) {						/* really just tries twice */
		cbCopied = ::GetPrivateProfileString( pszSection, pszKey,
							pszDefault, _pchData, _cbData, pszFile );

		if (IsOwnerAlloc() || cbCopied < QueryAllocSize() - 1 ||
			(QueryAllocSize() >= MAX_RES_STR_LEN))
			break;					/* string fit, or can't grow */

		if (!realloc( MAX_RES_STR_LEN ))
			break;					/* tried to grow, but couldn't */
	}

	_cchLen = cbCopied;
	IncVers();
}
