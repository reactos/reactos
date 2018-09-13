/*****************************************************************************
 *
 *	ftpwith.cpp - "With" procedures
 *
 *****************************************************************************/

#include "priv.h"

/*****************************************************************************
 *
 *	With_Hglob
 *
 *	Allocate a moveable HGLOBAL of the requested size, lock it, then call
 *	the callback.  On return, unlock it and get out.
 *
 *	Returns the allocated HGLOBAL, or 0.
 *
 *****************************************************************************/

HGLOBAL With_Hglob(UINT cb, HGLOBWITHPROC pfn, LPVOID pvRef)
{
    HGLOBAL hglob = GlobalAlloc(GHND, cb);
    if (hglob)
    {
	    LPVOID pv = GlobalLock(hglob);
	    if (pv)
        {
	        BOOL fRc = pfn(pv, pvRef);
	        GlobalUnlock(hglob);
	        if (!fRc)
            {
		        GlobalFree(hglob);
		        hglob = 0;
	        }
	    }
        else
        {
	        GlobalFree(hglob);
	        hglob = 0;
	    }
    }

    return hglob;
}

