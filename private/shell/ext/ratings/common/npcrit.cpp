/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1993-1994			**/
/*****************************************************************/ 

/* npcrit.c -- Implementation of critical section classes.
 *
 * History:
 *	11/01/93	gregj	Created
 */

#include "npcommon.h"
#include <npcrit.h>
#include <npassert.h>

/*
 * Very simple interlock routines, used to stop race conditions when
 * initializing and de-initializing critical sections.  Do NOT use
 * these for anything other than infrequent extremely short-term locks, 
 * since WaitForInterlock contains a spin loop with a millisecond delay!
 */
BYTE InterlockedSet(volatile BYTE *pByte)
{
	BYTE bRet;
	_asm {
		mov		edi, pByte
		mov		al, 1
		xchg	[edi], al		/* store non-zero value, get what was there before */
		mov		bRet, al
	}
	return bRet;
}

void WaitForInterlock(volatile BYTE *pByte)
{
	for (;;) {
		BYTE bAlreadyOwned = InterlockedSet(pByte);	/* attempt to grab the interlock */
		if (!bAlreadyOwned)				/* is someone else in there? */
			break;						/* nope, we now own it */
		Sleep(1);						/* yield to whomever owns it, then try again */
	}
}

void ReleaseInterlock(volatile BYTE *pByte)
{
	*pByte = 0;							/* clear the interlock to release others */
}

#if 0
// Remove CRITSEC code but keep for a while before deleting.
/*******************************************************************

    NAME:		CRITSEC::Init

    SYNOPSIS:	Initializes a global critical section object

    ENTRY:		pszName - name for the critical section

    EXIT:		No return value

    NOTES:		Currently pszName is not used;  it will be used
				for named mutexes later.

    HISTORY:
		gregj	11/01/93	Created

********************************************************************/

void CRITSEC::Init(char *pszName)
{
	WaitForInterlock(&_bInterlock);
	if (!_fInitialized) {
		::InitializeCriticalSection(&_critsec);
#ifdef DEBUG
		_wClaimCount = 0;
#endif
		_fInitialized = 1;
	}
	ReleaseInterlock(&_bInterlock);
	_cClients++;
}


/*******************************************************************

    NAME:		CRITSEC::Term

    SYNOPSIS:	Cleans up resources allocated for a critical section

    ENTRY:		No parameters

    EXIT:		No return value

    NOTES:		This function should be callled at process attach.
				It will take care of making sure it only deletes
				the critical section when the last process using
				it calls Term().

    HISTORY:
		gregj	11/01/93	Created

********************************************************************/

void CRITSEC::Term()
{
	WaitForInterlock(&_bInterlock);
	BOOL fShouldCleanUp = (--_cClients == 0);
	if (fShouldCleanUp) {
		::DeleteCriticalSection(&_critsec);
		_fInitialized = 0;
	}
	ReleaseInterlock(&_bInterlock);
}


#ifdef DEBUG		/* in retail, these are inline */
/*******************************************************************

    NAME:		CRITSEC::Enter

    SYNOPSIS:	Enters a critical section

    ENTRY:		No parameters

    EXIT:		No return value;  critical section is owned by
				the calling thread

    NOTES:		This function is private, and is invoked indirectly
				by the friend class TAKE_CRITSEC.

    HISTORY:
		gregj	11/01/93	Created

********************************************************************/

void CRITSEC::Enter()
{
#ifdef DEBUG
	UIASSERT(_fInitialized != 0);
#endif

	::EnterCriticalSection(&_critsec);

#ifdef DEBUG
	_wClaimCount++;
#endif
}


/*******************************************************************

    NAME:		CRITSEC::Leave

    SYNOPSIS:	Leaves a critical section

    ENTRY:		No parameters

    EXIT:		No return value;  critical section is released

    NOTES:		This function is private, and is invoked indirectly
				by the friend class TAKE_CRITSEC.

    HISTORY:
		gregj	11/01/93	Created

********************************************************************/

void CRITSEC::Leave()
{
#ifdef DEBUG
	UIASSERT(_fInitialized != 0);
	UIASSERT(_wClaimCount > 0);
	_wClaimCount--;
#endif

	::LeaveCriticalSection(&_critsec);
}
#endif	/* DEBUG */
#endif	/* 0 */

