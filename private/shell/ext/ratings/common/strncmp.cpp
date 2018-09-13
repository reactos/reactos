/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strncmp.cxx
	NLS/DBCS-aware string class: strncmp method

	This file contains the implementation of the strncmp method
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

	NAME:		NLS_STR::strncmp

	SYNOPSIS:	Case sensitve string compare up to index position istrLen

	ENTRY:

	EXIT:

	NOTES:

	HISTORY:
		johnl	11/15/90	Written
		beng	07/23/91	Allow on erroneous string; simplified CheckIstr

********************************************************************/

INT NLS_STR::strncmp(
	const NLS_STR & nls,
	const ISTR	  & istrEnd ) const
{
	if (QueryError() || !nls)
		return 0;

	CheckIstr( istrEnd );

	return ::strncmpf( QueryPch(), nls.QueryPch(), istrEnd.QueryIB() );
}


INT NLS_STR::strncmp(
	const NLS_STR & nls,
	const ISTR	  & istrEnd,
	const ISTR	  & istrStart1 ) const
{
	if (QueryError() || !nls)
		return 0;

	UIASSERT( istrEnd.QueryIB() >= istrStart1.QueryIB() );
	CheckIstr( istrEnd );
	CheckIstr( istrStart1 );

	return ::strncmpf( QueryPch(istrStart1),
					   nls.QueryPch(),
					   istrEnd - istrStart1 );
}


INT NLS_STR::strncmp(
	const NLS_STR & nls,
	const ISTR	  & istrEnd,
	const ISTR	  & istrStart1,
	const ISTR	  & istrStart2 ) const
{
	if (QueryError() || !nls)
		return 0;

	UIASSERT( istrEnd.QueryIB() >= istrStart1.QueryIB() );
	UIASSERT( istrEnd.QueryIB() >= istrStart2.QueryIB() );
	CheckIstr( istrEnd );
	CheckIstr( istrStart1 );
	nls.CheckIstr( istrStart2 );

	return ::strncmpf( QueryPch(istrStart1),
					   nls.QueryPch(istrStart2),
					   istrEnd - istrStart1 );
}
