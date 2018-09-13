/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strcat.cxx
	NLS/DBCS-aware string class: strcat method

	This file contains the implementation of the strcat method
	for the STRING class.  It is separate so that clients of STRING which
	do not use this operator need not link to it.

	FILE HISTORY:
		beng	01/18/91	Separated from original monolithic .cxx
		beng	02/07/91	Uses lmui.hxx
		beng	07/26/91	Replaced min with local inline
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

	NAME:		NLS_STR::strcat

	SYNOPSIS:	Concantenate string

	ENTRY:		nlsSuffix - appended to end of string
				- or -
				pszSuffix - appended to end of string

	EXIT:

	NOTES:		String doesn't change if a memory allocation failure occurs
				Currently checks to see if we need to reallocate the
				string (but we have to traverse it to determine the
				actual storage required).  We may want to change
				this.

	HISTORY:
		johnl	11/13/90	Written
		beng	07/22/91	Allow on erroneous strings
		gregj	07/05/94	Added LPCSTR overload

********************************************************************/

NLS_STR & NLS_STR::strcat( const NLS_STR & nlsSuffix )
{
	if (QueryError() || !nlsSuffix)
		return *this;

	if ( QueryAllocSize() < (strlen() + nlsSuffix.strlen() + 1) )
	{
		if (IsOwnerAlloc() || !realloc( strlen() + nlsSuffix.strlen() + 1 ))
		{
			ReportError( WN_OUT_OF_MEMORY );
			return *this;
		}
	}

	::strcatf( _pchData, nlsSuffix.QueryPch() );
	_cchLen += nlsSuffix.strlen();

	return *this;
}


NLS_STR & NLS_STR::strcat( LPCSTR pszSuffix )
{
	if (QueryError())
		return *this;

	UINT cbSuffix = ::strlenf(pszSuffix);
	if ( (UINT)QueryAllocSize() < (strlen() + cbSuffix + 1) )
	{
		if (IsOwnerAlloc() || !realloc( strlen() + cbSuffix + 1 ))
		{
			ReportError( WN_OUT_OF_MEMORY );
			return *this;
		}
	}

	::strcatf( _pchData, pszSuffix );
	_cchLen += cbSuffix;

	return *this;
}


#ifdef EXTENDED_STRINGS
/*******************************************************************

    NAME:	NLS_STR::Append

    SYNOPSIS:	Append a string to the end of current string

    ENTRY:	nlsSuffix - appended to end of string

    EXIT:

    RETURNS:

    NOTES:	Little more than a wrapper around strcat.

    HISTORY:
	beng	    22-Jul-1991     Created (parallel of AppendChar)

********************************************************************/

APIERR NLS_STR::Append( const NLS_STR &nlsSuffix )
{
    strcat(nlsSuffix);
    return QueryError();
}


/*******************************************************************

    NAME:	NLS_STR::AppendChar

    SYNOPSIS:	Append a single character to the end of current string

    ENTRY:	wch - appended to end of string

    EXIT:

    RETURNS:	0 if successful

    NOTES:
	CODEWORK: This member would do well to skip the "strcat" step
	and append directly to the subject string.

    HISTORY:
	beng	    23-Jul-1991     Created

********************************************************************/

APIERR NLS_STR::AppendChar( WCHAR wch )
{
#if defined(UNICODE)
    STACK_NLS_STR(nlsTemp, 1);

    nlsTemp._pchData[0] = (CHAR)wch;
    nlsTemp._pchData[1] = 0;
    nlsTemp._cchLen = sizeof(CHAR); // since it's really in bytes

#else
    STACK_NLS_STR(nlsTemp, 2);

    if (HIBYTE(wch) == 0)
    {
	// Single-byte character
	nlsTemp._pchData[0] = LOBYTE(wch);
	nlsTemp._pchData[1] = '\0';
	nlsTemp._cchLen = sizeof(CHAR);
    }
    else
    {
	// Double-byte character
	nlsTemp._pchData[0] = HIBYTE(wch); // lead byte
	nlsTemp._pchData[1] = LOBYTE(wch);
	nlsTemp._pchData[2] = '\0';
	nlsTemp._cchLen = 2*sizeof(CHAR);
    }

#endif

    strcat(nlsTemp);
    return QueryError();
}
#endif	// EXTENDED_STRINGS


/*******************************************************************

	NAME:		NLS_STR::operator+=

	SYNOPSIS:	Append a string to the end of current string

	ENTRY:		wch - character to append

	EXIT:

	RETURNS:

	NOTES:		Little more than a wrapper around strcat.

	HISTORY:
		beng	07/23/91	Header added
		gregj	03/25/93	Added WCHAR version to replace AppendChar
		gregj	07/13/94	NLS_STR version was identical to strcat, so inlined

********************************************************************/

NLS_STR & NLS_STR::operator+=( WCHAR wch )
{
#if defined(UNICODE)
	STACK_NLS_STR(nlsTemp, 1);

	nlsTemp._pchData[0] = (CHAR)wch;
	nlsTemp._pchData[1] = 0;
	nlsTemp._cchLen = sizeof(CHAR); // since it's really in bytes

#else
	STACK_NLS_STR(nlsTemp, 2);

	if (HIBYTE(wch) == 0)
	{
		// Single-byte character
		nlsTemp._pchData[0] = LOBYTE(wch);
		nlsTemp._pchData[1] = '\0';
		nlsTemp._cchLen = sizeof(CHAR);
	}
	else
	{
		// Double-byte character
		nlsTemp._pchData[0] = HIBYTE(wch); // lead byte
		nlsTemp._pchData[1] = LOBYTE(wch);
		nlsTemp._pchData[2] = '\0';
		nlsTemp._cchLen = 2*sizeof(CHAR);
	}

#endif

    strcat(nlsTemp);
    return *this;
}


/*******************************************************************

	NAME:		NLS_STR::realloc

	SYNOPSIS:	Reallocate an NLS_STR to the passed count of bytes, copying
				the current contents to the reallocated string.

	ENTRY:		cb - number of bytes desired in string storage

	EXIT:
		Returns TRUE if successful:

			_pchData points to allocated storage of "cb" bytes.
			_cbData set to cb.
			Old storage is copied

		Returns FALSE upon allocation failure, the string is preserved

	NOTES:
		A string will never be downsized (i.e., realloc can only be used
		to increase the size of a string).  If a request comes in to make
		the string smaller, it will be ignored, and TRUE will be returned.

		DO NOT CALL REALLOC ON AN OWNERALLOCED STRING!!  You will cause
		an assertion error if you do.

	HISTORY:
		johnl	11/11/90	Created
		beng	04/26/91	Changed USHORT parm to INT

********************************************************************/

BOOL NLS_STR::realloc( INT cb )
{
	UIASSERT( !IsOwnerAlloc() );
	UIASSERT( cb != 0 );

	if ( cb <= QueryAllocSize() )
		return TRUE;

	CHAR * pchNewMem = new CHAR[cb];

	if (pchNewMem == NULL)
		return FALSE;

	::memcpyf( pchNewMem, _pchData, min( cb-1, QueryAllocSize() ) );
	delete _pchData;
	_pchData = pchNewMem;
	_cbData = cb;
	*( _pchData + cb - 1 ) = '\0';

	return TRUE;
}
