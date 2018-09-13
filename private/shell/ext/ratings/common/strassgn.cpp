/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	strassgn.cxx
	NLS/DBCS-aware string class: assignment operator

	This file contains the implementation of the assignment operator
	for the STRING class.  It is separate so that clients of STRING which
	do not use this operator need not link to it.

	FILE HISTORY:
		beng	01/18/91	Separated from original monolithic .cxx
		beng	02/07/91	Uses lmui.hxx
		beng	07/26/91	Replaced min with local inline
		gregj	04/02/93	Do buffer overflow checks for OWNERALLOC strings
							instead of asserting
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

	NAME:		NLS_STR::operator=

	SYNOPSIS:	Assignment operator

	ENTRY:		Either NLS_STR or CHAR*.

	EXIT:		If successful, contents of string overwritten.
				If failed, the original contents of the string remain.

	RETURNS:	Reference to self.

	HISTORY:
		beng	10/23/90	Created
		johnl	11/13/90	Added UIASSERTion checks for using bad
							strings
		beng	02/05/91	Uses CHAR * instead of PCH
		Johnl	03/06/91	Removed assertion check for *this
							being valid
		johnl	04/12/91	Resets error variable on PCH assignment
							if successful.
		beng	07/22/91	Allow assignment of an erroneous string;
							reset error on nls assignment as well
		gregj	04/02/93	Do buffer overflow checks for OWNERALLOC strings
							instead of asserting

********************************************************************/

NLS_STR& NLS_STR::operator=( const NLS_STR& nlsSource )
{
	if ( this == &nlsSource )
		return *this;

	if (!nlsSource)
	{
		// Assignment of an erroneous string
		//
		ReportError((unsigned short)nlsSource.QueryError());
		return *this;
	}

	INT cbToCopy = nlsSource.strlen();

	if ( !IsOwnerAlloc() )
	{
		if ( QueryAllocSize() < nlsSource.strlen()+1 )
		{
			/* Don't use Realloc because we want to retain the contents
			 * of the string if we fail to get the memory.
			 */
			CHAR * pchNew = new CHAR[nlsSource.strlen()+1];

			if ( pchNew == NULL )
			{
				ReportError( WN_OUT_OF_MEMORY );
				return *this;
			}

			delete _pchData;
			_pchData = pchNew;
			_cbData = nlsSource.strlen()+1;
		}

	}
	else
	{
        if (::fDBCSEnabled) {
    		if (QueryAllocSize() <= cbToCopy) {
    			cbToCopy = QueryAllocSize() - 1;	/* leave room for the null */
    			const CHAR *p = nlsSource.QueryPch();
    			while (p < nlsSource.QueryPch() + cbToCopy)
    				p += nlsSource.IsDBCSLeadByte(*p) ? 2 : 1;
    			if (p - nlsSource.QueryPch() != cbToCopy)	/* last char was DB */
    				cbToCopy--;								/* don't copy lead byte either */
    		}
        }
        else {
    		if (QueryAllocSize() <= cbToCopy)
	    		cbToCopy = QueryAllocSize() - 1;
        }
	}

	if (nlsSource.IsOEM())
		SetOEM();
	else
		SetAnsi();

	::memcpyf( _pchData, nlsSource.QueryPch(), cbToCopy );	/* copy string data */
	_pchData[cbToCopy] = '\0';		/* terminate the string */
	_cchLen = cbToCopy;
	IncVers();

	/* Reset the error state, since the string is now valid.
	 */
	ReportError( WN_SUCCESS );
	return *this;
}


NLS_STR& NLS_STR::operator=( const CHAR *pchSource )
{
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
		INT iSourceLen = ::strlenf( pchSource );
		INT cbToCopy;

		if ( !IsOwnerAlloc() )
		{
			if ( QueryAllocSize() < iSourceLen + 1 )
			{
				CHAR * pchNew = new CHAR[iSourceLen + 1];

				if ( pchNew == NULL )
				{
					ReportError( WN_OUT_OF_MEMORY );
					return *this;
				}

				delete _pchData;
				_pchData = pchNew;
				_cbData = iSourceLen + 1;
			}
			cbToCopy = iSourceLen;
		}
		else
		{
			if (QueryAllocSize() <= iSourceLen) {
                if (::fDBCSEnabled) {
    				cbToCopy = QueryAllocSize() - 1;	/* leave room for the null */
    				const CHAR *p = pchSource;
    				while (p < pchSource + cbToCopy)
    					p += IsDBCSLeadByte(*p) ? 2 : 1;
    				if (p - pchSource != cbToCopy)		/* last char was DB */
    					cbToCopy--;						/* don't copy lead byte either */
    			}
                else
	    			cbToCopy = QueryAllocSize() - 1;
            }
			else
				cbToCopy = iSourceLen;
		}

		::memcpyf( _pchData, pchSource, cbToCopy );
		_pchData[cbToCopy] = '\0';		/* terminate the string */
		_cchLen = cbToCopy;
	}

	IncVers();

	/* Reset the error state, since the string is now valid.
	 */
	ReportError( WN_SUCCESS );
	return *this;
}


#ifdef EXTENDED_STRINGS
/*******************************************************************

	NAME:		NLS_STR::CopyFrom()

	SYNOPSIS:	Assignment method which returns an error code

	ENTRY:
		nlsSource - source argument, either a nlsstr or char vector.
		achSource

	EXIT:
		Copied argument into this.  Error code of string set.

	RETURNS:
		Error code of string - WN_SUCCESS if successful.

	NOTES:
		If the CopyFrom fails, the current string will retain its
		original contents and error state.

	HISTORY:
		beng	09/18/91	Created
		beng	09/19/91	Added content-preserving behavior

********************************************************************/

APIERR NLS_STR::CopyFrom( const NLS_STR & nlsSource )
{
	if (!nlsSource)
		return nlsSource.QueryError();

	*this = nlsSource;

	APIERR err = QueryError();
	if (err)
		Reset();
	else {
		if (nlsSource.IsOEM())
			SetOEM();
		else
			SetAnsi();
	}
	return err;
}


APIERR NLS_STR::CopyFrom( const CHAR * achSource )
{
	*this = achSource;

	APIERR err = QueryError();
	if (err)
		Reset();
	return err;
}
#endif	// EXTENDED_STRINGS
