/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/* BUFFER.H -- Definition of BUFFER class.
 *
 * History:
 *	03/22/93	gregj	Created
 *	03/24/93	gregj	Created base class, GLOBAL_BUFFER
 *	10/06/93	gregj	Removed LOCAL_BUFFER and GLOBAL_BUFFER because
 *						they're incompatible with Win32.
 */

#ifndef _INC_BUFFER
#define _INC_BUFFER

/*************************************************************************

    NAME:		BUFFER_BASE

    SYNOPSIS:	Base class for transient buffer classes

    INTERFACE:	BUFFER_BASE()
					Construct with optional size of buffer to allocate.

				Resize()
					Resize buffer to specified size.  Returns TRUE if
					successful.

				QuerySize()
					Return the current size of the buffer in bytes.

				QueryPtr()
					Return a pointer to the buffer.

    PARENT:		None

    USES:		None

    CAVEATS:	This is an abstract class, which unifies the interface
				of BUFFER, GLOBAL_BUFFER, etc.

    NOTES:		In standard OOP fashion, the buffer is deallocated in
				the destructor.

    HISTORY:
		03/24/93	gregj	Created base class

**************************************************************************/

class BUFFER_BASE
{
protected:
	UINT _cb;

	virtual BOOL Alloc( UINT cbBuffer ) = 0;
	virtual BOOL Realloc( UINT cbBuffer ) = 0;

public:
	BUFFER_BASE()
		{ _cb = 0; }	// buffer not allocated yet
	~BUFFER_BASE()
		{ _cb = 0; }	// buffer size no longer valid
	BOOL Resize( UINT cbNew );
	UINT QuerySize() const { return _cb; };
};

#define GLOBAL_BUFFER	BUFFER

/*************************************************************************

    NAME:		BUFFER

    SYNOPSIS:	Wrapper class for new and delete

    INTERFACE:	BUFFER()
					Construct with optional size of buffer to allocate.

				Resize()
					Resize buffer to specified size.  Only works if the
					buffer hasn't been allocated yet.

				QuerySize()
					Return the current size of the buffer in bytes.

				QueryPtr()
					Return a pointer to the buffer.

    PARENT:		BUFFER_BASE

    USES:		operator new, operator delete

    CAVEATS:

    NOTES:		In standard OOP fashion, the buffer is deallocated in
				the destructor.

    HISTORY:
		03/24/93	gregj	Created

**************************************************************************/

class BUFFER : public BUFFER_BASE
{
protected:
	CHAR *_lpBuffer;

	virtual BOOL Alloc( UINT cbBuffer );
	virtual BOOL Realloc( UINT cbBuffer );

public:
	BUFFER( UINT cbInitial=0 );
	~BUFFER();
	LPVOID QueryPtr() const { return (LPVOID)_lpBuffer; }
	operator void*() const { return (void *)_lpBuffer; }
};

#define LOCAL_BUFFER	BUFFER

#endif	/* _INC_BUFFER */
