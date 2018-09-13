/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/

/*
	strmisc.cxx
	Miscellaneous members of the string classes

	The NLS_STR and ISTR classes have many inline member functions
	which bloat clients, especially in debug versions.	This file
	gives those unhappy functions a new home.

	FILE HISTORY:
		beng	04/26/91	Created (relocated from string.hxx)
		gregj	05/22/92	Added ToOEM, ToAnsi methods

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


#ifdef DEBUG
/*******************************************************************

	NAME:		NLS_STR::CheckIstr

	SYNOPSIS:	Checks association between ISTR and NLS_STR instances

	ENTRY:		istr - ISTR to check against this NLS_STR

	NOTES:
		Does nothing in retail build.

	HISTORY:
		beng		07/23/91	Header added; removed redundant "nls" parameter.

********************************************************************/

VOID NLS_STR::CheckIstr( const ISTR& istr ) const
{
	UIASSERT( (istr).QueryPNLS() == this );
	UIASSERT( (istr).QueryVersion() == QueryVersion() );
}


VOID NLS_STR::IncVers()
{
	_usVersion++;
}


VOID NLS_STR::InitializeVers()
{
	_usVersion = 0;
}


VOID NLS_STR::UpdateIstr( ISTR *pistr ) const
{
	pistr->SetVersion( QueryVersion() );
}


USHORT NLS_STR::QueryVersion() const
{
	return _usVersion;
}


const CHAR * NLS_STR::QueryPch() const
{
	if (QueryError()) {
		UIASSERT(FALSE);
		return NULL;
	}

	return _pchData;
}


const CHAR * NLS_STR::QueryPch( const ISTR& istr ) const
{
	if (QueryError())
		return NULL;

	CheckIstr( istr );
	return _pchData+istr.QueryIB();
}


WCHAR NLS_STR::QueryChar( const ISTR& istr ) const
{
	if (QueryError())
		return 0;

	CheckIstr( istr );
	return *(_pchData+istr.QueryIB());
}
#endif	// DEBUG


/*******************************************************************

	NAME:		NLS_STR::ToOEM

	SYNOPSIS:	Convert string to OEM character set

	ENTRY:		No parameters

	EXIT:		String is in OEM character set

	RETURNS:

	NOTES:		If the string is already OEM, nothing happens.
				A string may be constructed as OEM by constructing
				as usual, then calling SetOEM().  Casemap conversion
				does NOT work on OEM strings!

	HISTORY:
		gregj	05/22/92	Created

********************************************************************/

VOID NLS_STR::ToOEM()
{
	if (IsOEM())
		return;			// string is already OEM

	SetOEM();

#ifdef WIN31
	::AnsiToOem( _pchData, _pchData );
#endif
}


/*******************************************************************

	NAME:		NLS_STR::ToAnsi

	SYNOPSIS:	Convert string to ANSI character set

	ENTRY:		No parameters

	EXIT:		String is in ANSI character set

	RETURNS:

	NOTES:		If the string is already ANSI (the default), nothing
				happens.

	HISTORY:
		gregj	05/22/92	Created

********************************************************************/

VOID NLS_STR::ToAnsi()
{
	if (!IsOEM())
		return;			// string is already ANSI

	SetAnsi();

#ifdef WIN31
	::OemToAnsi( _pchData, _pchData );
#endif
}


/*******************************************************************

	NAME:		NLS_STR::SetOEM

	SYNOPSIS:	Declares string to be in OEM character set

	ENTRY:		No parameters

	EXIT:		OEM flag set

	RETURNS:

	NOTES:		Use this method if you construct a string which is
				known to be in the OEM character set (e.g., it came
				back from a Net API).

	HISTORY:
		gregj	05/22/92	Created

********************************************************************/

VOID NLS_STR::SetOEM()
{
	_fsFlags |= SF_OEM;
}


/*******************************************************************

	NAME:		NLS_STR::SetAnsi

	SYNOPSIS:	Declares string to be in ANSI character set

	ENTRY:		No parameters

	EXIT:		OEM flag set

	RETURNS:

	NOTES:		This method is used primarily by NLS_STR itself,
				when an ANSI string is assigned to a previously
				OEM one.

	HISTORY:
		gregj	05/22/92	Created

********************************************************************/

VOID NLS_STR::SetAnsi()
{
	_fsFlags &= ~SF_OEM;
}


/*******************************************************************

	NAME:		NLS_STR::IsDBCSLeadByte

	SYNOPSIS:	Returns whether a character is a lead byte or not

	ENTRY:		ch - byte to check

	EXIT:		TRUE if "ch" is a lead byte

	RETURNS:

	NOTES:		This method works whether the string is OEM or ANSI.
				In a non-DBCS build, this function is inline and
				always returns FALSE.

	HISTORY:
		gregj	04/02/93	Created

********************************************************************/

BOOL NLS_STR::IsDBCSLeadByte( CHAR ch ) const
{
	return IS_LEAD_BYTE(ch);
}
