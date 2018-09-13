/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strupr.cxx
	NLS/DBCS-aware string class: strupr method

	This file contains the implementation of the strupr method
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

	NAME:		NLS_STR::strupr

	SYNOPSIS:	Convert *this lower case letters to upper case

	ENTRY:

	EXIT:

	NOTES:

	HISTORY:
		johnl	11/26/90	Written
		beng	07/23/91	Allow on erroneous string

********************************************************************/

NLS_STR& NLS_STR::strupr()
{
	if (!QueryError())
		::struprf( _pchData );

	return *this;
}
