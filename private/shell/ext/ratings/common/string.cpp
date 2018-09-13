/*****************************************************************/ 
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/*
	string.cxx
	NLS/DBCS-aware string class: essential core methods

	This file contains those routines which every client of
	the string classes will always need.

	Most of the implementation has been exploded into other files,
	so that an app linking to string doesn't end up dragging the
	entire string runtime library along with it.

	FILE HISTORY:
		beng	10/23/90	Created
		johnl	12/11/90	Remodeled beyond all recognizable form
		beng	01/18/91	Most methods relocated into other files
		beng	02/07/91	Uses lmui.hxx
		beng	07/26/91	Replaced min with local inline
		gregj	03/30/93	Removed ISTR to separate module
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

	NAME:		NLS_STR::NLS_STR

	SYNOPSIS:	Constructor for NLS_STR

	ENTRY:		NLS_STR takes many (too many) ctor forms.

	EXIT:		String constructed

	NOTES:
		The default constructor creates an empty string.

	HISTORY:
		beng	10/23/90	Created
		beng	04/26/91	Replaced 'CB' and USHORT with INT
		beng	07/22/91	Uses member-init ctor forms

********************************************************************/

NLS_STR::NLS_STR()
	: _pchData(0),
	  _cbData(0),
	  _cchLen(0),
	  _fsFlags(0)
{
	if ( !Alloc(1) )
		return;

	*_pchData = '\0';
	InitializeVers();
}


NLS_STR::NLS_STR( INT cchInitLen )
	: _pchData(0),
	  _cbData(0),
	  _cchLen(0),
	  _fsFlags(0)
{
	if (!Alloc(cchInitLen+1))
		return;

	::memsetf( _pchData, '\0', cchInitLen );

	_cchLen = 0;

	InitializeVers();
}


NLS_STR::NLS_STR( const CHAR * pchInit )
	: _pchData(0),
	  _cbData(0),
	  _cchLen(0),
	  _fsFlags(0)
{
	if (pchInit == NULL)
	{
		if (!Alloc(1))
			ReportError( WN_OUT_OF_MEMORY );
		else
		{
			*_pchData = '\0';
		}
		return;
	}

	INT iSourceLen = ::strlenf( pchInit );

	if ( !Alloc( iSourceLen + 1 ) )
		return;

	::strcpyf( _pchData, pchInit );

	_cchLen = iSourceLen;

	InitializeVers();
}


NLS_STR::NLS_STR( const NLS_STR & nlsInit )
	: _pchData(0),
	  _cbData(0),
	  _cchLen(0),
	  _fsFlags(0)
{
	UIASSERT( !nlsInit.QueryError() );

	if (!Alloc( nlsInit.strlen()+1 ) )
		return;

	::memcpyf( _pchData, nlsInit.QueryPch(), nlsInit.strlen()+1 );

	_cchLen = nlsInit.strlen();

	InitializeVers();
}


#ifdef EXTENDED_STRINGS
NLS_STR::NLS_STR( const CHAR * pchInit, INT iTotalLen )
	: _pchData(0),
	  _cbData(0),
	  _cchLen(0),
	  _fsFlags(0)
{
	if (pchInit == NULL)
	{
		if (!Alloc( 1 + iTotalLen ))
			return;
		*_pchData = '\0';
	}
	else
	{
		_cchLen = ::strlenf( pchInit );
		if ( _cchLen > iTotalLen )
		{
			_cchLen = 0;
			ReportError( WN_OUT_OF_MEMORY );
			return;
		}

		if ( !Alloc( iTotalLen ) )
		{
			_cchLen = 0;
			return;
		}

		::memcpyf( _pchData, pchInit, _cchLen+1 );
	}

	InitializeVers();
}
#endif	// EXTENDED_STRINGS


NLS_STR::NLS_STR( unsigned stralloc, CHAR *pchInit, INT cbSize )
	: _pchData(0),
	  _cbData(0),
	  _cchLen(0),
	  _fsFlags(SF_OWNERALLOC)
{
	UIASSERT( stralloc == STR_OWNERALLOC || stralloc == STR_OWNERALLOC_CLEAR);
	UIASSERT( pchInit != NULL );

	if ( stralloc == STR_OWNERALLOC_CLEAR )
	{
		UIASSERT( cbSize > 0 );
		*(_pchData = pchInit ) = '\0';
		_cchLen = 0;
	}
	else
	{
		_pchData = pchInit;
		_cchLen = ::strlenf( pchInit );
	}

	if ( cbSize == -1 )
		_cbData = _cchLen + 1;
	else
		_cbData = cbSize;

	InitializeVers();
}


#ifdef EXTENDED_STRINGS
NLS_STR::NLS_STR( unsigned stralloc, CHAR *pchBuff, INT cbSize,
				  const CHAR *pchInit )
	: _pchData(0),
	  _cbData(0),
	  _cchLen(0),
	  _fsFlags(SF_OWNERALLOC)
{
	UIASSERT( stralloc == STR_OWNERALLOC );
	UIASSERT( stralloc != STR_OWNERALLOC_CLEAR );
	UIASSERT( pchBuff != NULL || pchInit != NULL );
	UIASSERT( cbSize > 0 && ::strlenf( pchInit ) <= cbSize );

	UNREFERENCED( stralloc );

	_pchData = pchBuff;

	INT cbToCopy = min( ::strlenf( pchInit ), cbSize - 1 );
	::memcpyf( _pchData, pchInit, cbToCopy );
	*(_pchData + cbToCopy) = '\0';

	_cchLen = cbToCopy;
	_cbData = cbSize;

	InitializeVers();
}
#endif


/*******************************************************************

	NAME:		NLS_STR::~NLS_STR

	SYNOPSIS:	Destructor for NLS_STR

	ENTRY:

	EXIT:		Storage deallocated, if not owner-alloc

	HISTORY:
		beng	10/23/90	Created
		beng	07/22/91	Zeroes only in debug version

********************************************************************/

NLS_STR::~NLS_STR()
{
	if ( !IsOwnerAlloc() )
		delete _pchData;

#if defined(DEBUG)
	_pchData = NULL;
	_cchLen  = 0;
	_cbData = 0;
#endif
}


/*******************************************************************

	NAME:		NLS_STR::Alloc

	SYNOPSIS:	Common code for constructors.

	ENTRY:
		cb - number of bytes desired in string storage

	EXIT:
		Returns TRUE if successful:

			_pchData points to allocated storage of "cb" bytes.
			_cbData set to cb.
			Allocated storage set to 0xF2 in debug version

		Returns FALSE upon allocation failure.

	NOTES:

	HISTORY:
		beng	10/23/90	Created
		johnl	12/11/90	Updated as per code review
		beng	04/26/91	Changed USHORT parm to INT

********************************************************************/

BOOL NLS_STR::Alloc( INT cb )
{
	UIASSERT( cb != 0 );

	_pchData = new CHAR[cb];
	if (_pchData == NULL)
	{
		// For now, assume not enough memory.
		//
		ReportError(WN_OUT_OF_MEMORY);
		return FALSE;
	}

#ifdef DEBUG
	::memsetf(_pchData, 0xf2, cb);
#endif
	_cbData = cb;

	return TRUE;
}


/*******************************************************************

	NAME:		NLS_STR::Reset

	SYNOPSIS:	Attempts to clear the error state of the string

	ENTRY:		String is in error state

	EXIT:		If recoverable, string is correct again

	RETURNS:	TRUE if successful; FALSE otherwise

	NOTES:
		An operation on a string may fail, if this occurs, the error
		flag is set and you can't use the string until the flag
		is cleared.  By calling Reset, you can clear the flag,
		thus allowing you to get access to the string again.  The
		string will be in a consistent state.  Reset will return
		FALSE if the string couldn't be restored (for example, after
		construction failure).

    HISTORY:
		Johnl	12/12/90	Created

********************************************************************/

BOOL NLS_STR::Reset()
{
	UIASSERT( QueryError() ) ;	// Make sure an error exists

	if ( QueryError() == WN_OUT_OF_MEMORY && _pchData != NULL )
	{
		ReportError( WN_SUCCESS );
		return TRUE;
	}

	return FALSE;
}

