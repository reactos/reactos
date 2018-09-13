/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/

/* NPCRIT.H -- Definition of CRITSEC classes.
 *
 * History:
 *	gregj	11/01/93	Created
 *  lens    02/25/94    Modified to use CRITICAL_SECTIONs Reinitialize().
 *                      Took out spin-loop interlock.
 */

#ifndef _INC_CRITSEC
#define _INC_CRITSEC

#ifndef RC_INVOKED
#pragma pack(1)
#endif

extern "C"
{

/* extern DECLSPEC_IMPORT
VOID
WINAPI
ReinitializeCriticalSection(LPCRITICAL_SECTION lpcs); - in windefs.h */

extern DECLSPEC_IMPORT
VOID
WINAPI
UninitializeCriticalSection(LPCRITICAL_SECTION lpcs);

// codework: remove following and make MEMWATCH use critical sections.
#ifdef DEBUG
void WaitForInterlock(volatile BYTE *pByte);
void ReleaseInterlock(volatile BYTE *pByte);
#endif	/* DEBUG */

}

/*************************************************************************

    NAME:		CRITSEC

    SYNOPSIS:	Class wrapper for global critical section

    INTERFACE:	Init(pszName)
					Initializes the critical section.

				Term()
					Cleans up the critical section.

	PRIVATE:	Enter()
					Enter the critical section, waiting for others
					to leave if necessary.

				Leave()
					Leave the critical section, unblocking other waiters.

    PARENT:		None

    USES:		None

    CAVEATS:	This class is not initialized with its constructor because
				it should be instantiated at global scope, which introduces
				constructor-linker problems.  Instead, its fields should be
				initialized to all zeroes, and Init() should be called at
				process attach time.  Term() should be called at process
				detach.

    NOTES:		The class's initialization takes care of synchronizing
				itself to protect against multiple simultaneous inits.

    HISTORY:
		11/01/93	gregj	Created
        02/25/94    lens    Modified to use CRITICAL_SECTION directly.

**************************************************************************/

class CRITSEC
{
friend class TAKE_CRITSEC;

private:
	CRITICAL_SECTION _critsec;

public:
	void Enter() { ::EnterCriticalSection(&_critsec); }
	void Leave() { ::LeaveCriticalSection(&_critsec); }
#ifndef WINNT
	void Init() { ::ReinitializeCriticalSection(&_critsec); }
	void Term() { /* codework: add ::UninitializeCriticalSection(&_critsec); */}
#endif /* WINNT */
};


/*************************************************************************

    NAME:		TAKE_CRITSEC

    SYNOPSIS:	Class wrapper to take a critical section.

    INTERFACE:	TAKE_CRITSEC(critsec)
					Construct with the global CRITICAL_SECTION object to take.

				~TAKE_CRITSEC()
					Destructor automatically releases the critical section.

				Release()
					Releases the critical section manually.

				Take()
					Takes the critical section manually.

    PARENT:		None

    USES:		None

    CAVEATS:	None

    NOTES:		Instantiate one of these classes in a block of code
				when you want that block of code to be protected
				against re-entrancy.
                The Take() and Release() functions should rarely be necessary,
                and must be used in matched pairs with Release() called first.

    HISTORY:
		11/01/93	gregj	Created

**************************************************************************/

class TAKE_CRITSEC
{
private:
	CRITSEC & const _critsec;

public:
	void Take(void) { _critsec.Enter(); }
	void Release(void) { _critsec.Leave(); }
	TAKE_CRITSEC(CRITSEC& critsec) : _critsec(critsec) { Take(); }
	~TAKE_CRITSEC() { Release(); }
};


/*************************************************************************

    NAME:		TAKE_MUTEX

    SYNOPSIS:	Class wrapper to take a mutex.

    INTERFACE:	TAKE_MUTEX(hMutex)
					Construct with the mutex handle to take.

				~TAKE_MUTEX()
					Destructor automatically releases the mutex.

				Release()
					Releases the mutex manually.

				Take()
					Takes the mutex manually.

    PARENT:		None

    USES:		None

    CAVEATS:	None

    NOTES:		Instantiate one of these classes in a block of code
				when you want that block of code to be protected
				against re-entrancy.
                The Take() and Release() functions should rarely be necessary,
                and must be used in matched pairs with Release() called first.

    HISTORY:
		09/27/94	lens	Created

**************************************************************************/

class TAKE_MUTEX
{
private:
	HANDLE const _hMutex;

public:
	void Take(void) { WaitForSingleObject(_hMutex, INFINITE); }
	void Release(void) { ReleaseMutex(_hMutex); }
	TAKE_MUTEX(HANDLE hMutex) : _hMutex(hMutex) { Take(); }
	~TAKE_MUTEX() { Release(); }
};

#ifndef RC_INVOKED
#pragma pack()
#endif

#endif	/* _INC_BUFFER */
