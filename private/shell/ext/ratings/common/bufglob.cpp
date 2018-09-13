/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/* BUFGLOB.CPP -- Implementation of GLOBAL_BUFFER class.
 *
 * History:
 *	03/22/93	gregj	Created
 *	03/24/93	gregj	Renamed from plain BUFFER, derived from BUFFER_BASE
 *
 */

#include "npcommon.h"
#include "buffer.h"

BOOL GLOBAL_BUFFER::Alloc( UINT cbBuffer )
{
	_hMem = ::GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, cbBuffer );
	if (_hMem == NULL) {
		_lpBuffer = NULL;
		_cb = 0;
		return FALSE;
	}

	_lpBuffer = ::GlobalLock( _hMem );
	_cb = cbBuffer;
	return TRUE;
}

BOOL GLOBAL_BUFFER::Realloc( UINT cbNew )
{
	if (_hMem == NULL)
		return FALSE;

	::GlobalUnlock( _hMem );

        HGLOBAL hNew = ::GlobalReAlloc( _hMem, cbNew, GMEM_MOVEABLE );
	if (hNew == NULL) {
		::GlobalLock( _hMem );
		return FALSE;
	}

	_hMem = hNew;
	_lpBuffer = ::GlobalLock( _hMem );
	_cb = cbNew;
	return TRUE;
}

GLOBAL_BUFFER::GLOBAL_BUFFER( UINT cbInitial /* =0 */ )
  : BUFFER_BASE(),
	_hMem( NULL ),
	_lpBuffer( NULL )
{
	if (cbInitial)
		Alloc( cbInitial );
}

GLOBAL_BUFFER::~GLOBAL_BUFFER()
{
	if (_hMem != NULL) {
		::GlobalUnlock( _hMem );
		::GlobalFree( _hMem );
		_hMem = NULL;
		_lpBuffer = NULL;
	}
}
