/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/* BUFFER.CPP -- Implementation of BUFFER class.
 *
 * History:
 *	03/24/93	gregj	Created
 *	10/25/93	gregj	Use shell232.dll routines
 */

#include "npcommon.h"
#include "buffer.h"
#include <netlib.h>

BOOL BUFFER::Alloc( UINT cbBuffer )
{
	_lpBuffer = (LPSTR)::MemAlloc(cbBuffer);
	if (_lpBuffer != NULL) {
		_cb = cbBuffer;
		return TRUE;
	}
	return FALSE;
}

BOOL BUFFER::Realloc( UINT cbNew )
{
	LPVOID lpNew = ::MemReAlloc(_lpBuffer, cbNew);
	if (lpNew == NULL)
		return FALSE;

	_lpBuffer = (LPSTR)lpNew;
	_cb = cbNew;
	return TRUE;
}

BUFFER::BUFFER( UINT cbInitial /* =0 */ )
  : BUFFER_BASE(),
	_lpBuffer( NULL )
{
	if (cbInitial)
		Alloc( cbInitial );
}

BUFFER::~BUFFER()
{
	if (_lpBuffer != NULL) {
		::MemFree(_lpBuffer);
		_lpBuffer = NULL;
	}
}
