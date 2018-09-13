/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/* BUFGLOB.CPP -- Implementation of GLOBAL_BUFFER class.
 *
 * History:
 *	03/24/93	gregj	Created
 */

#include "npcommon.h"
#include "buffer.h"

BOOL LOCAL_BUFFER::Alloc( UINT cbBuffer )
{
	_hMem = ::LocalAlloc( LMEM_MOVEABLE, cbBuffer );
	if (_hMem == NULL) {
		_npBuffer = NULL;
		_cb = 0;
		return FALSE;
	}

	_npBuffer = ::LocalLock( _hMem );
	_cb = cbBuffer;
	return TRUE;
}

BOOL LOCAL_BUFFER::Realloc( UINT cbNew )
{
	if (_hMem == NULL)
		return FALSE;

	::LocalUnlock( _hMem );

        HLOCAL hNew = ::LocalReAlloc( _hMem, cbNew, LMEM_MOVEABLE );
	if (hNew == NULL) {
		::LocalLock( _hMem );
		return FALSE;
	}

	_hMem = hNew;
	_npBuffer = ::LocalLock( _hMem );
	_cb = cbNew;
	return TRUE;
}

LOCAL_BUFFER::LOCAL_BUFFER( UINT cbInitial /* =0 */ )
  : BUFFER_BASE(),
	_hMem( NULL ),
	_npBuffer( NULL )
{
	if (cbInitial)
		Alloc( cbInitial );
}

LOCAL_BUFFER::~LOCAL_BUFFER()
{
	if (_hMem != NULL) {
		::LocalUnlock( _hMem );
		::LocalFree( _hMem );
		_hMem = NULL;
		_npBuffer = NULL;
	}
}
