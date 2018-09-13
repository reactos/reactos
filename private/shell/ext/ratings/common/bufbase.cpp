/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/* BUFBASE.CPP -- Implementation of BUFFER_BASE class.
 *
 * History:
 *	03/24/93	gregj	Created base class
 *
 */

#include "npcommon.h"
#include "buffer.h"

// The following code would be nice in OOP fashion, but since the
// derived class's virtuals aren't available until after the derived
// class's constructor is done, this Alloc() call will not go anywhere.
// Therefore each derived class must stick the if statement in its
// constructor.
#if 0
BUFFER_BASE::BUFFER_BASE( UINT cbInitial /* =0 */ )
  : _cb( 0 )		// buffer not allocated yet
{
	if (cbInitial)
		Resize( cbInitial );
}
#endif

BOOL BUFFER_BASE::Resize( UINT cbNew )
{
	BOOL fSuccess;

	if (QuerySize() == 0)
		fSuccess = Alloc( cbNew );
	else {
		fSuccess = Realloc( cbNew );
	}
	if (fSuccess)
		_cb = cbNew;
	return fSuccess;
}
